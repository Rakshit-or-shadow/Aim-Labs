#include "GEShotTour.h"
#include "GEGameMode.h"
#include "GEHUD.h"
#include "GECharacter.h"
#include "GETargetManager.h"
#include "GETargetSphere.h"
#include "GETrainingManager.h"
#include "GEWeaponComponent.h"
#include "GEBot.h"
#include "GESaveGame.h"
#include "GEStyle.h"
#include "GEAimModes.h"
#include "RayTraceMath.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "UnrealClient.h"
#include "HighResScreenshot.h"
#include "Misc/StringOutputDevice.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	constexpr float HardTimeout = 470.0f;  // seconds, whole tour
	constexpr float StepTimeout = 40.0f;   // seconds, one step
	constexpr float BootTimeout = 30.0f;   // seconds, waiting for the menus to exist
}

FGEShotTour::FGEShotTour(AGEGameMode* InOwner)
	: Owner(InOwner)
{
	FString Dir;
	if (!FParse::Value(FCommandLine::Get(), TEXT("ShotDir="), Dir) || Dir.IsEmpty())
	{
		Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ShotTour"));
	}
	OutDir = FPaths::ConvertRelativePathToFull(Dir);
	IFileManager::Get().MakeDirectory(*OutDir, true);
	FFileHelper::SaveStringToFile(TEXT("SHOTTOUR start\n"), *FPaths::Combine(OutDir, TEXT("tour_log.txt")));
}

// ---------------------------------------------------------------- accessors
APlayerController* FGEShotTour::PC() const { return Owner && Owner->GetWorld() ? Owner->GetWorld()->GetFirstPlayerController() : nullptr; }
AGEHUD* FGEShotTour::HUD() const { APlayerController* P = PC(); return P ? Cast<AGEHUD>(P->GetHUD()) : nullptr; }
AGECharacter* FGEShotTour::Char() const { APlayerController* P = PC(); return P ? Cast<AGECharacter>(P->GetPawn()) : nullptr; }
AGETargetManager* FGEShotTour::Aim() const { return Owner ? Owner->GetAimManager() : nullptr; }
UGETrainingManager* FGEShotTour::Train() const { return UGETrainingManager::Get(Owner); }
UGESaveGame* FGEShotTour::Save() const { UGETrainingManager* T = Train(); return T ? T->GetSaveGame() : nullptr; }

void FGEShotTour::Log(const FString& Line) const
{
	UE_LOG(LogTemp, Display, TEXT("SHOTTOUR %s"), *Line);
	FFileHelper::SaveStringToFile(Line + TEXT("\n"), *FPaths::Combine(OutDir, TEXT("tour_log.txt")),
		FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}

void FGEShotTour::Skip(const FString& Name, const FString& Why) const
{
	Log(FString::Printf(TEXT("SHOTTOUR SKIPPED %s: %s"), *Name, *Why));
}

void FGEShotTour::Finish(const FString& Why)
{
	if (bFinished) { return; }
	bFinished = true;
	Log(FString::Printf(TEXT("finished: %s after %.1f s, %d shots"), *Why, TotalTime, ShotCounter));
	FPlatformMisc::RequestExit(false);
}

void FGEShotTour::Add(const FString& Name, float Settle, TFunction<void()> Enter, TFunction<bool()> Cond)
{
	FStep S;
	S.Name = Name; S.Settle = Settle; S.Enter = MoveTemp(Enter); S.Cond = MoveTemp(Cond);
	Steps.Add(MoveTemp(S));
}

void FGEShotTour::Capture(const FString& Name)
{
	++ShotCounter;
	const FString File = FString::Printf(TEXT("%02d_%s.png"), ShotCounter, *Name);
	const FString Path = FPaths::Combine(OutDir, File);
	// The window can be smaller than the desktop allows, so render the shot offscreen at exactly 1920x1080 (HUD included).
	bool bHighRes = false;
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		FStringOutputDevice Ar;
		if (GetHighResScreenshotConfig().ParseConsoleCommand(FString::Printf(TEXT("1920x1080 filename=\"%s\""), *Path), Ar))
		{
			bHighRes = GEngine->GameViewport->Viewport->TakeHighResScreenShot();
		}
	}
	if (!bHighRes)
	{
		Log(TEXT("high-res shot unavailable, using a normal screenshot"));
		FScreenshotRequest::RequestScreenshot(Path, true, false);
	}
	ShotWait = 4; // frames to let the viewport write the file
	Log(FString::Printf(TEXT("shot %s"), *File));
}

void FGEShotTour::NextStep()
{
	Index++;
	StepTime = 0.0f;
	StepWall = 0.0f;
	if (Steps.IsValidIndex(Index) && Steps[Index].Enter)
	{
		Steps[Index].Enter();
	}
}

// ---------------------------------------------------------------- tick
void FGEShotTour::Tick(float Dt)
{
	if (bFinished) { return; }
	TotalTime += Dt;
	if (TotalTime > HardTimeout) { Finish(TEXT("hard timeout")); return; }

	// wait until the HUD flow, arena and save exist
	if (!bBuilt)
	{
		BootTime += Dt;
		AGEHUD* H = HUD();
		if (H && H->bFlowActive && Aim() && Aim()->HasActorBegunPlay() && Char() && Save())
		{
			Build();
			bBuilt = true;
			if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
			{
				const FIntPoint Size = GEngine->GameViewport->Viewport->GetSizeXY();
				Log(FString::Printf(TEXT("viewport %d x %d"), Size.X, Size.Y));
			}
		}
		else if (BootTime > BootTimeout)
		{
			Skip(TEXT("everything"), TEXT("menus never became ready"));
			Finish(TEXT("boot failed"));
		}
		return;
	}

	TickAuto(Dt);

	if (bMoveForward)
	{
		if (AGECharacter* C = Char())
		{
			C->AddMovementInput(C->GetActorForwardVector(), 1.0f);
		}
	}

	if (ShotWait >= 0)
	{
		if (--ShotWait < 0) { NextStep(); }
		return;
	}

	if (Index < 0)
	{
		NextStep();
		return;
	}
	if (Index >= Steps.Num())
	{
		Finish(TEXT("all steps done"));
		return;
	}

	FStep& S = Steps[Index];
	StepWall += Dt;
	if (!S.Cond || S.Cond())
	{
		StepTime += Dt;
	}
	if (StepWall > StepTimeout)
	{
		Skip(S.Name.IsEmpty() ? TEXT("(step)") : S.Name, TEXT("condition not met within 40 s, moving on"));
		NextStep();
		return;
	}
	if (StepTime >= S.Settle)
	{
		if (!S.Name.IsEmpty())
		{
			Capture(S.Name); // advances a few frames later
		}
		else
		{
			NextStep();
		}
	}
}

// ---------------------------------------------------------------- autoplayer
void FGEShotTour::AimViewAt(const FVector& Target, float Dt)
{
	APlayerController* P = PC();
	if (!P || !P->PlayerCameraManager) { return; }
	const FVector From = P->PlayerCameraManager->GetCameraLocation();
	const FRotator Want = (Target - From).Rotation();
	P->SetControlRotation(FMath::RInterpTo(P->GetControlRotation(), Want, Dt, AimSpeed));
}

void FGEShotTour::TickAuto(float Dt)
{
	SinceFire += Dt;
	switch (Auto)
	{
	case EAuto::AimClicks: AutoAimClicks(Dt, false); break;
	case EAuto::AimTrack:  AutoAimClicks(Dt, true); break;
	case EAuto::Bots:
	case EAuto::Flick:     AutoBots(Dt); break;
	default: break;
	}
}

void FGEShotTour::AutoAimClicks(float Dt, bool bTrackOnly)
{
	AGETargetManager* A = Aim();
	APlayerController* P = PC();
	if (!A || !P || !P->PlayerCameraManager || !A->IsRunning() || A->IsCountingDown() || A->IsPaused()) { SeenFor = 0.0f; return; }

	const FVector Eye = P->PlayerCameraManager->GetCameraLocation();
	const FVector Fwd = P->PlayerCameraManager->GetCameraRotation().Vector();
	AGETargetSphere* Best = nullptr;
	float BestDot = -2.0f;
	for (TActorIterator<AGETargetSphere> It(Owner->GetWorld()); It; ++It)
	{
		if (!It->bActive) { continue; }
		const float D = FVector::DotProduct(Fwd, (It->GetActorLocation() - Eye).GetSafeNormal());
		if (D > BestDot) { BestDot = D; Best = *It; }
	}
	if (!Best) { SeenFor = 0.0f; return; }

	const FVector Pos = Best->GetActorLocation();
	if (!bTrackOnly && !Pos.Equals(LastTargetPos, 1.0f)) { SeenFor = 0.0f; LastTargetPos = Pos; } // tracking: the target always moves
	SeenFor += Dt;
	if (SeenFor < Reaction) { return; }

	AimViewAt(Pos, Dt);
	if (bTrackOnly || !bAutoFire) { return; }

	double T = 0.0;
	if (SinceFire >= FireInterval && RayTraceMath::DoesIntersect(Pos, Best->Radius, Eye, Fwd, T))
	{
		SinceFire = 0.0f;
		A->HandleFire(); // the game's own first-shot ray + RayTraceMath hit test
	}
}

void FGEShotTour::AutoBots(float Dt)
{
	AGECharacter* C = Char();
	APlayerController* P = PC();
	UGEWeaponComponent* W = C ? C->GetWeaponComponent() : nullptr;
	if (!C || !P || !P->PlayerCameraManager || !W) { return; }
	UGETrainingManager* T = Train();
	if (!T || !T->IsSessionRunning()) { W->FireReleased(); SeenFor = 0.0f; return; }

	const FVector Eye = P->PlayerCameraManager->GetCameraLocation();
	const FVector Fwd = P->PlayerCameraManager->GetCameraRotation().Vector();

	FVector AimPoint = FVector::ZeroVector;
	float Radius = 0.0f;
	float BestDot = -2.0f;
	bool bFound = false;
	for (TActorIterator<AGEBot> It(Owner->GetWorld()); It; ++It)
	{
		if (!It->IsAlive() || !It->CanBeShot()) { continue; }
		TArray<FGEHitSphere> Spheres;
		It->GetHitSpheres(Spheres);
		if (Spheres.Num() == 0) { continue; }
		// head when asked, otherwise the highest body sphere
		const FGEHitSphere* Pick = &Spheres[0];
		for (const FGEHitSphere& S : Spheres)
		{
			if (bPreferHead && S.Zone == EGEHitZone::Head) { Pick = &S; break; }
			if (!bPreferHead && S.Zone == EGEHitZone::Body && (Pick->Zone != EGEHitZone::Body || S.Center.Z > Pick->Center.Z)) { Pick = &S; }
		}
		float D = FVector::DotProduct(Fwd, (Pick->Center - Eye).GetSafeNormal());
		if (bPreferFar) { D = static_cast<float>(FVector::Dist(Pick->Center, Eye)); } // farthest bot wins
		if (!bFound || D > BestDot) { BestDot = D; AimPoint = Pick->Center; Radius = Pick->Radius; bFound = true; }
	}
	if (!bFound) { W->FireReleased(); SeenFor = 0.0f; return; }

	if (!AimPoint.Equals(LastTargetPos, 1.0f)) { SeenFor = 0.0f; LastTargetPos = AimPoint; }
	SeenFor += Dt;
	if (SeenFor < Reaction) { return; }

	AimViewAt(AimPoint, Dt);
	double Hit = 0.0;
	W->FireReleased(); // semi-auto weapons need a fresh press; the weapon enforces its own fire rate
	if (bAutoFire && RayTraceMath::DoesIntersect(AimPoint, Radius, Eye, Fwd, Hit))
	{
		W->FirePressed();
	}
}

// ---------------------------------------------------------------- helpers
void FGEShotTour::ResetSettings()
{
	UGESaveGame* S = Save();
	if (!S) { return; }
	if (UGETrainingManager* T = Train()) { T->SetSensitivityScale(1.0f); }
	S->FieldOfView = 103.0f;
	S->Volume = 0.7f;
	S->CrosshairColor = 0;
	S->UITheme = 0;
	S->bScanlines = false;
	S->bGlitch = true;
	GEStyle::SetActiveTheme(0);
}

void FGEShotTour::AddFakeHistory()
{
	UGESaveGame* S = Save();
	if (!S) { return; }
	// A believable past (tour only, never saved): three Static / Longarm sessions with slowly improving numbers.
	const int32 Shots[3] = { 96, 104, 112 };
	const int32 Hits[3] = { 38, 47, 58 };
	const int32 Kills[3] = { 7, 9, 12 };
	for (int32 i = 0; i < 3; i++)
	{
		FGESessionRecord R;
		R.Date = FDateTime::Now() - FTimespan::FromDays(3 - i);
		R.Mode = EGETrainingMode::Static;
		R.Difficulty = EGEDifficulty::Medium;
		R.WeaponIndex = 2;
		R.WeaponName = TEXT("Longarm");
		R.SensitivityScale = 1.0f;
		FGETrainingStats& St = R.Stats;
		St.Kills = Kills[i]; St.HeadshotKills = 2 + i; St.Shots = Shots[i]; St.Hits = Hits[i];
		St.FirstShots = 24; St.FirstShotHits = 9 + 2 * i;
		St.TotalTimeToKill = 1.9f * Kills[i]; St.Duration = 60.0f;
		St.ShotsMoving = 30; St.HitsMoving = 8 + i * 2; St.FirstShotsMoving = 8; St.FirstShotHitsMoving = 2 + i;
		St.HitsByDistance = { 20 + i * 3, 12 + i * 2, 5 + i, 1 };
		St.HeadHitsByDistance = { 5 + i, 3, 1, 0 };
		S->AddSession(R);
	}
}

// ---------------------------------------------------------------- the script
void FGEShotTour::BuildAimSteps(int32 ModeIndex)
{
	const EGEAimMode Mode = (EGEAimMode)ModeIndex;
	const FString N = GEAimModes::GetName(Mode).ToLower();
	auto IsGo = [this]() { AGETargetManager* A = Aim(); return A && A->IsRunning() && !A->IsCountingDown(); };
	auto IsResults = [this]() { AGEHUD* H = HUD(); return H && H->Screen == EGEScreen::Results; };

	switch (Mode)
	{
	case EGEAimMode::Gridshot:
		Add(N + TEXT("_countdown"), 1.4f, [this, Mode]() {
			Aim()->TourRoundLength = 12.0f; Auto = EAuto::AimClicks; AimSpeed = 14.0f; FireInterval = 0.42f; Reaction = 0.12f; bAutoFire = true;
			HUD()->StartAimMode(Mode); });
		Add(N + TEXT("_playing"), 3.2f, nullptr, IsGo);
		Add(N + TEXT("_pause_menu"), 0.9f, [this]() { HUD()->OpenPause(); });
		Add(TEXT(""), 0.2f, [this]() { HUD()->ResumeFromPause(); });
		Add(N + TEXT("_hud_theme_cyan"), 0.6f, [this]() { Save()->UITheme = 1; });
		Add(N + TEXT("_hud_theme_yellow"), 0.6f, [this]() { Save()->UITheme = 2; });
		Add(N + TEXT("_hud_theme_green"), 0.6f, [this]() { Save()->UITheme = 3; });
		Add(N + TEXT("_results_new_best"), 1.8f, [this]() { Save()->UITheme = 0; }, IsResults);
		break;
	case EGEAimMode::Microshot:
		Add(TEXT(""), 0.1f, [this, Mode]() {
			Aim()->TourRoundLength = 9.0f; Auto = EAuto::AimClicks; AimSpeed = 12.0f; FireInterval = 0.6f; Reaction = 0.2f; bAutoFire = true;
			HUD()->StartAimMode(Mode); });
		Add(N + TEXT("_playing"), 3.5f, nullptr, IsGo);
		Add(N + TEXT("_results"), 1.8f, nullptr, IsResults);
		break;
	case EGEAimMode::Tracking:
		Add(TEXT(""), 0.1f, [this, Mode]() {
			Aim()->TourRoundLength = 9.0f; Auto = EAuto::AimTrack; AimSpeed = 13.0f; Reaction = 0.05f;
			HUD()->StartAimMode(Mode); });
		Add(N + TEXT("_playing"), 4.0f, nullptr, IsGo);
		Add(N + TEXT("_results"), 1.8f, nullptr, IsResults);
		break;
	case EGEAimMode::Reflex:
		Add(TEXT(""), 0.1f, [this, Mode]() {
			Aim()->ReflexTargets = 6; Auto = EAuto::AimClicks; AimSpeed = 22.0f; FireInterval = 0.2f; Reaction = 0.28f; bAutoFire = true;
			HUD()->StartAimMode(Mode); });
		Add(N + TEXT("_waiting"), 0.5f, nullptr, IsGo);
		Add(N + TEXT("_playing"), 0.4f, nullptr, [this]() {
			AGETargetManager* A = Aim();
			if (!A || !A->IsRunning() || A->IsCountingDown() || A->GetTimeSinceGo() < 3.5f) { return false; }
			for (TActorIterator<AGETargetSphere> It(Owner->GetWorld()); It; ++It) { if (It->bActive) { return true; } }
			return false; });
		Add(N + TEXT("_results"), 1.8f, nullptr, IsResults);
		break;
	}
}

void FGEShotTour::Build()
{
	auto InState = [this](EGEScreen Scr) { return TFunction<bool()>([this, Scr]() { AGEHUD* H = HUD(); return H && H->Screen == Scr; }); };

	// ---------- menus ----------
	Add(TEXT("main_menu_red"), 2.2f);
	Add(TEXT("main_menu_selected_training"), 0.7f, [this]() { HUD()->SelectedItem = 1; });
	Add(TEXT("main_menu_theme_cyan"), 0.8f, [this]() { HUD()->SelectedItem = 0; Save()->UITheme = 1; });
	Add(TEXT("main_menu_theme_yellow"), 0.8f, [this]() { Save()->UITheme = 2; });
	Add(TEXT("main_menu_theme_green"), 0.8f, [this]() { Save()->UITheme = 3; });
	Add(TEXT("arena_wide_view"), 1.5f, [this]() {
		Save()->UITheme = 0;
		Save()->FieldOfView = 120.0f;
		if (AGEHUD* H = HUD()) { H->Screen = EGEScreen::FreeRoam; }
		if (APlayerController* P = PC()) { P->SetControlRotation(FRotator(-2.0f, 0.0f, 0.0f)); } });
	Add(TEXT(""), 0.1f, [this]() {
		ResetSettings();
		if (AGEHUD* H = HUD()) { H->GoToMainMenu(); } });

	for (int32 i = 0; i < 4; i++)
	{
		Add(TEXT("mode_select_") + GEAimModes::GetName((EGEAimMode)i).ToLower(), 1.3f, [this, i]() {
			if (i == 0) { HUD()->SetScreen(EGEScreen::ModeSelect, 0); } else { HUD()->SelectedItem = i; } });
	}

	// ---------- settings: every section, each setting visibly changed ----------
	Add(TEXT("settings_mouse_sensitivity"), 1.3f, [this]() {
		HUD()->SettingsReturn = EGEScreen::MainMenu; HUD()->SetScreen(EGEScreen::Settings, 0);
		for (int32 k = 0; k < 5; k++) { HUD()->ChangeSetting(0, +1); } });
	Add(TEXT("settings_video_fov"), 0.9f, [this]() { HUD()->SelectedItem = 1; for (int32 k = 0; k < 2; k++) { HUD()->ChangeSetting(1, +1); } });
	Add(TEXT("settings_audio_volume"), 0.9f, [this]() { HUD()->SelectedItem = 2; for (int32 k = 0; k < 3; k++) { HUD()->ChangeSetting(2, -1); } });
	Add(TEXT("settings_crosshair_color"), 0.9f, [this]() { HUD()->SelectedItem = 3; HUD()->ChangeSetting(3, +1); HUD()->ChangeSetting(3, +1); });
	Add(TEXT("settings_interface_theme"), 0.9f, [this]() { HUD()->SelectedItem = 4; HUD()->ChangeSetting(4, +1); });
	Add(TEXT("settings_interface_scanlines"), 0.9f, [this]() { HUD()->SelectedItem = 5; HUD()->ChangeSetting(5, +1); });
	Add(TEXT("settings_interface_glitch_off"), 0.9f, [this]() { HUD()->SelectedItem = 6; HUD()->ChangeSetting(6, +1); });
	Add(TEXT(""), 0.1f, [this]() { ResetSettings(); HUD()->GoToMainMenu(); });

	// ---------- aim modes ----------
	for (int32 m = 0; m < 4; m++) { BuildAimSteps(m); }

	// second Gridshot with a weak autoplayer: a result that is NOT a personal best
	Add(TEXT(""), 0.1f, [this]() {
		Aim()->TourRoundLength = 8.0f; Auto = EAuto::AimClicks; AimSpeed = 3.0f; FireInterval = 2.0f; Reaction = 0.8f;
		HUD()->StartAimMode(EGEAimMode::Gridshot); });
	Add(TEXT("gridshot_results_no_new_best"), 1.8f, nullptr, InState(EGEScreen::Results));
	Add(TEXT("mode_select_with_personal_bests"), 1.3f, [this]() { Auto = EAuto::None; HUD()->GoToMainMenu(); HUD()->SetScreen(EGEScreen::ModeSelect, 0); });

	// ---------- free roam ----------
	Add(TEXT("free_roam_standing"), 1.6f, [this]() { HUD()->GoToMainMenu(); HUD()->StartFreeRoam(); });
	Add(TEXT("pause_menu_free_roam"), 0.9f, [this]() { HUD()->OpenPause(); });
	Add(TEXT(""), 0.2f, [this]() { HUD()->ResumeFromPause(); });

	// ---------- Training Center: menu, movement ----------
	Add(TEXT("training_center_menu"), 1.8f, [this]() {
		Train()->SetMode(EGETrainingMode::Static); Train()->SetDifficulty(EGEDifficulty::Easy); Train()->SetWeaponIndex(2);
		HUD()->StartTrainingScreen(false); });
	Add(TEXT("training_center_menu_strafing_hard_scatter"), 0.8f, [this]() {
		Train()->SetMode(EGETrainingMode::Strafing); Train()->SetDifficulty(EGEDifficulty::Hard); Train()->SetWeaponIndex(4); });
	Add(TEXT(""), 0.2f, [this]() {
		Train()->SetMode(EGETrainingMode::Static); Train()->SetDifficulty(EGEDifficulty::Easy); Train()->SetWeaponIndex(2);
		if (Train()->IsMenuOpen()) { Train()->ToggleMenu(); }
		HUD()->SetScreen(EGEScreen::TrainingCenter, 0); });
	Add(TEXT("movement_standing"), 1.2f, [this]() { if (APlayerController* P = PC()) { LaneRot = P->GetControlRotation(); } });
	Add(TEXT("movement_walking_speed_readout"), 1.4f, [this]() { Char()->bShowSpeedReadout = true; Char()->bWalkHeld = true; bMoveForward = true; });
	Add(TEXT("movement_running"), 1.4f, [this]() { Char()->bWalkHeld = false; });
	Add(TEXT("movement_crouching"), 1.0f, [this]() { bMoveForward = false; Char()->Crouch(); });
	Add(TEXT("movement_jump_midair"), 0.30f, [this]() { Char()->UnCrouch(); bMoveForward = true; Char()->Jump(); });
	Add(TEXT(""), 1.2f, [this]() { bMoveForward = false; Char()->bShowSpeedReadout = false; });

	// ---------- Training Center: weapons on bots ----------
	Add(TEXT(""), 0.6f, [this]() {
		AddFakeHistory();
		Auto = EAuto::Bots; AimSpeed = 16.0f; Reaction = 0.15f; bPreferHead = false; bAutoFire = true;
		Train()->StartSession(); });
	static const TCHAR* WeaponNames[5] = { TEXT("sidekick"), TEXT("rattler"), TEXT("longarm"), TEXT("eagle_eye"), TEXT("scatter") };
	// a frame where a shot has just landed: muzzle flash + hit marker are on screen
	auto JustHit = [this]() { AGECharacter* C = Char(); return C && C->GetWeaponComponent() && C->GetWeaponComponent()->GetTimeSinceHit() < 0.05f; };
	for (int32 w = 0; w < 5; w++)
	{
		const FString WN = WeaponNames[w];
		Add(TEXT(""), 1.0f, [this, w]() {
			Char()->GetWeaponComponent()->AimReleased(); bPreferHead = false; bPreferFar = false; Train()->SetWeaponIndex(w); });
		Add(FString::Printf(TEXT("weapon_%s_firing"), *WN), 0.0f, nullptr, JustHit);
		Add(TEXT(""), 1.2f, [this, w]() { bPreferFar = (w == 3); Char()->GetWeaponComponent()->AimPressed(); });
		Add(FString::Printf(TEXT("weapon_%s_ads"), *WN), 0.0f, nullptr, JustHit);
		Add(FString::Printf(TEXT("weapon_%s_reload"), *WN), 0.7f, [this]() {
			bPreferFar = false; bAutoFire = false; Char()->GetWeaponComponent()->FireReleased(); Char()->GetWeaponComponent()->AimReleased(); Char()->GetWeaponComponent()->ReloadPressed(); });
		Add(TEXT(""), 0.1f, [this]() { bAutoFire = true; });
		if (w == 2)
		{
			// a bot dying: Longarm headshots
			Add(TEXT(""), 0.1f, [this]() { bPreferHead = true; });
			Add(TEXT("bot_killed_headshot"), 0.15f, nullptr, [this]() {
				for (TActorIterator<AGEBot> It(Owner->GetWorld()); It; ++It) { if (!It->IsAlive()) { return true; } }
				return false; });
		}
	}
	Add(TEXT("training_menu_session_stats"), 1.0f, [this]() { Train()->SetWeaponIndex(2); Train()->ToggleMenu(); });
	Add(TEXT(""), 0.1f, [this]() { Train()->ToggleMenu(); Auto = EAuto::None; Char()->GetWeaponComponent()->FireReleased(); Train()->StopSession(); });
	Add(TEXT("training_results_and_coach"), 2.0f, nullptr, InState(EGEScreen::Results));
	Add(TEXT("training_menu_coach_panel"), 1.4f, [this]() { HUD()->SetScreen(EGEScreen::TrainingCenter, 0); if (!Train()->IsMenuOpen()) { Train()->ToggleMenu(); } });

	// ---------- other bot types ----------
	for (int32 k = 0; k < 2; k++)
	{
		const EGETrainingMode BotMode = (k == 0) ? EGETrainingMode::Strafing : EGETrainingMode::Peek;
		Add(TEXT(""), 0.3f, [this, BotMode]() {
			HUD()->SetScreen(EGEScreen::TrainingCenter, 0);
			if (Train()->IsMenuOpen()) { Train()->ToggleMenu(); }
			if (APlayerController* P = PC()) { P->SetControlRotation(LaneRot); }
			Train()->SetMode(BotMode); Train()->SetDifficulty(EGEDifficulty::Hard); Train()->SetWeaponIndex(0);
			bPreferHead = false; bPreferFar = false; bAutoFire = true; Auto = EAuto::Bots; Reaction = 0.2f;
			Train()->StartSession(); });
		Add(TEXT(""), 2.5f);
		Add(k == 0 ? TEXT("bots_strafing_hard") : TEXT("bots_peek_hard"), 0.0f, nullptr, [this]() {
			APlayerController* P = PC();
			if (!P || !P->PlayerCameraManager) { return false; }
			const FVector Eye = P->PlayerCameraManager->GetCameraLocation();
			const FVector Fwd = P->PlayerCameraManager->GetCameraRotation().Vector();
			for (TActorIterator<AGEBot> It(Owner->GetWorld()); It; ++It)
			{
				if (It->IsAlive() && It->CanBeShot() && FVector::DotProduct(Fwd, (It->GetActorLocation() + FVector(0, 0, 100) - Eye).GetSafeNormal()) > 0.9f) { return true; }
			}
			return false; });
		Add(TEXT(""), 0.1f, [this]() { Auto = EAuto::None; Char()->GetWeaponComponent()->FireReleased(); Train()->StopSession(); });
	}

	// ---------- Flick Optimizer ----------
	Add(TEXT("flick_optimizer_start"), 1.5f, [this]() {
		Auto = EAuto::None;
		HUD()->GoToMainMenu(); HUD()->StartTrainingScreen(true);
		Train()->SetDifficulty(EGEDifficulty::Medium); });
	Add(TEXT(""), 0.1f, [this]() {
		if (Train()->IsMenuOpen()) { Train()->ToggleMenu(); }
		if (APlayerController* P = PC()) { P->SetControlRotation(LaneRot); }
		Train()->SetWeaponIndex(0); bPreferHead = false; bPreferFar = false; bAutoFire = true; AimSpeed = 14.0f; Reaction = 0.12f; Auto = EAuto::Flick;
		Train()->StartSession(); });
	Add(TEXT(""), 4.0f);
	Add(TEXT("flick_in_progress"), 0.0f, nullptr, [this]() {
		APlayerController* P = PC();
		if (!P || !P->PlayerCameraManager) { return false; }
		const FVector Eye = P->PlayerCameraManager->GetCameraLocation();
		const FVector Fwd = P->PlayerCameraManager->GetCameraRotation().Vector();
		for (TActorIterator<AGEBot> It(Owner->GetWorld()); It; ++It)
		{
			if (It->IsAlive() && FVector::DotProduct(Fwd, (It->GetActorLocation() + FVector(0, 0, 100) - Eye).GetSafeNormal()) > 0.8f) { return true; }
		}
		return false; });
	Add(TEXT(""), 20.0f);
	Add(TEXT("flick_results_and_coach"), 2.0f, [this]() { Auto = EAuto::None; Char()->GetWeaponComponent()->FireReleased(); Train()->StopSession(); }, InState(EGEScreen::Results));
	Add(TEXT("flick_analysis_table"), 1.5f, [this]() { HUD()->SetScreen(EGEScreen::FlickOptimizer, 0); if (!Train()->IsMenuOpen()) { Train()->ToggleMenu(); } });

	// ---------- end ----------
	Add(TEXT("main_menu_final"), 1.2f, [this]() { if (Train()->IsMenuOpen()) { Train()->ToggleMenu(); } HUD()->GoToMainMenu(); });
}
