// GEHUDScreens.cpp
// The menu flow and every screen of AGEHUD: Main Menu, Aim Trainer select, Settings, Results,
// Pause and the in-round aim HUD. The old Training Center menu lives in GEHUD.cpp.
//
// HOW THE FLOW WORKS (read this first):
//   * AGEHUD::Screen says which screen we are on (EGEScreen).
//   * Tick() reads the keyboard/mouse and calls the "action" functions (GoToMainMenu, StartAimMode...).
//   * DrawHUD() (in GEHUD.cpp) calls the DrawXxx function of the current screen.
//   * The aim arena (AGETargetManager) is where the player stands during the Main Menu and aim rounds:
//     the floating showcase targets are behind the menus and the camera drifts slowly.
//   * SetScreen() -> ApplyInputState() sets cursor, input mode and blocks movement/look/firing in menus.

#include "GEHUD.h"
#include "GEUIKit.h"
#include "GECharacter.h"
#include "GEWeaponComponent.h"
#include "GETargetManager.h"
#include "GETrainingManager.h"
#include "GESaveGame.h"
#include "GEAimModes.h"
#include "GEStyle.h"
#include "GESounds.h"
#include "GEFlickTypes.h"
#include "GECoachTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	/** Smooth 0 -> 1 curve (fast start, gentle end) used for intro animations. */
	float EaseOut(float T)
	{
		T = FMath::Clamp(T, 0.0f, 1.0f);
		return 1.0f - FMath::Pow(1.0f - T, 3.0f);
	}

	// Main menu entries (index = SelectedItem).
	constexpr int32 MenuAim = 0;
	constexpr int32 MenuTraining = 1;
	constexpr int32 MenuFlick = 2;
	constexpr int32 MenuFreeRoam = 3;
	constexpr int32 MenuSettings = 4;
	constexpr int32 MenuQuit = 5;
	constexpr int32 NumMenuItems = 6;

	// Pause entries.
	constexpr int32 PauseResume = 0;
	constexpr int32 PauseRestart = 1;
	constexpr int32 PauseSettings = 2;
	constexpr int32 PauseMainMenu = 3;

	// Settings ranges.
	constexpr float MinFov = 60.0f;
	constexpr float MaxFov = 120.0f;
	constexpr float FovStep = 5.0f;
	constexpr float SensStep = 0.1f;
	constexpr float VolumeStep = 0.1f;
}

// =====================================================================
//                               HELPERS
// =====================================================================

UGESaveGame* AGEHUD::GetSave() const
{
	const UGETrainingManager* Training = UGETrainingManager::Get(this);
	return Training ? Training->GetSaveGame() : nullptr;
}

float AGEHUD::GetVolume() const
{
	const UGESaveGame* Save = GetSave();
	return Save ? Save->Volume : 0.7f;
}

AGETargetManager* AGEHUD::GetAim() const
{
	return AGETargetManager::Get(this);
}

AGECharacter* AGEHUD::GetGECharacter() const
{
	return Cast<AGECharacter>(GetOwningPawn());
}

bool AGEHUD::IsMenuScreen() const
{
	switch (Screen)
	{
	case EGEScreen::MainMenu:
	case EGEScreen::ModeSelect:
	case EGEScreen::Settings:
	case EGEScreen::Results:
	case EGEScreen::Pause:
		return true;
	default:
		return false;
	}
}

bool AGEHUD::ScreenNeedsArena() const
{
	switch (Screen)
	{
	case EGEScreen::MainMenu:
	case EGEScreen::ModeSelect:
	case EGEScreen::Playing:
		return true;
	case EGEScreen::Results:
		return ResultKind == EGEResultKind::Aim;
	case EGEScreen::Pause:
		return PauseReturn == EGEScreen::Playing;
	case EGEScreen::Settings:
		return SettingsReturn == EGEScreen::MainMenu || (SettingsReturn == EGEScreen::Pause && PauseReturn == EGEScreen::Playing);
	default:
		return false;
	}
}

bool AGEHUD::AllowTrainingHotkeys() const
{
	if (!bFlowActive)
	{
		return true; // other levels: the old behavior
	}
	return Screen == EGEScreen::FreeRoam || Screen == EGEScreen::TrainingCenter || Screen == EGEScreen::FlickOptimizer;
}

int32 AGEHUD::GetItemCount() const
{
	switch (Screen)
	{
	case EGEScreen::MainMenu:   return NumMenuItems;
	case EGEScreen::ModeSelect: return GEAimModes::NumModes + 1; // 4 modes + Back
	case EGEScreen::Settings:   return GEStyle::NumSettings + 1; // settings rows + Back
	case EGEScreen::Results:    return 2;                        // Play Again, Main Menu
	case EGEScreen::Pause:      return GetPauseItems().Num();
	default:                    return 0;
	}
}

TArray<int32> AGEHUD::GetPauseItems() const
{
	TArray<int32> Items;
	Items.Add(PauseResume);
	if (PauseReturn == EGEScreen::Playing)
	{
		Items.Add(PauseRestart); // only an aim round can be restarted
	}
	Items.Add(PauseSettings);
	Items.Add(PauseMainMenu);
	return Items;
}

// =====================================================================
//                         FLOW: STARTING AND SYNCING
// =====================================================================

void AGEHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ApplyFov();

	if (!bFlowChecked)
	{
		StartFlow();
		return;
	}
	if (!bFlowActive)
	{
		return; // this level has no aim arena: old behavior, no menus
	}

	SyncWithGame();
	HandleKeys();
}

void AGEHUD::StartFlow()
{
	if (!GetWorld() || !GetWorld()->HasBegunPlay())
	{
		return;
	}
	AGETargetManager* Aim = GetAim();
	if (Aim == nullptr)
	{
		bFlowChecked = true; // no arena in this level: free roam only
		return;
	}
	// wait until the manager, the player pawn and the save game exist
	if (!Aim->HasActorBegunPlay() || GetOwningPawn() == nullptr || GetSave() == nullptr)
	{
		return;
	}
	bFlowChecked = true;
	bFlowActive = true;
	GoToMainMenu(); // the game starts on the Main Menu
}

void AGEHUD::SyncWithGame()
{
	AGETargetManager* Aim = GetAim();
	UGETrainingManager* Training = UGETrainingManager::Get(this);
	if (Aim == nullptr)
	{
		return;
	}

	// An aim round was started from outside the menus (console: ge.aim.start).
	if (Aim->IsRunning() && (Screen == EGEScreen::MainMenu || Screen == EGEScreen::ModeSelect
		|| (Screen == EGEScreen::Settings && SettingsReturn == EGEScreen::MainMenu)))
	{
		ResultKind = EGEResultKind::Aim;
		SetScreen(EGEScreen::Playing, 0);
	}
	// The player left the arena from outside (console: ge.aim.stop) while a screen needs it.
	else if (!Aim->IsModeActive() && ScreenNeedsArena())
	{
		GoToMainMenu();
	}

	// An aim round ended by itself -> Results.
	if (Screen == EGEScreen::Playing && Aim->IsRoundFinished())
	{
		ResultKind = EGEResultKind::Aim;
		SetScreen(EGEScreen::Results, 0);
	}

	// A Training Center / Flick session was stopped -> Results (with the coach tips).
	const bool bRunning = Training && Training->IsSessionRunning();
	if (bWasSessionRunning && !bRunning)
	{
		ShowTrainingResults();
	}
	bWasSessionRunning = bRunning;
}

void AGEHUD::ApplyFov()
{
	const UGESaveGame* Save = GetSave();
	const AGECharacter* Character = GetGECharacter();
	if (Save && Character && Character->GetWeaponComponent())
	{
		// the weapon component writes this base FOV (plus the scope zoom) to the camera every frame
		Character->GetWeaponComponent()->SetBaseFov(FMath::Clamp(Save->FieldOfView, MinFov, MaxFov));
	}
}

// =====================================================================
//                         FLOW: SCREEN CHANGES
// =====================================================================

void AGEHUD::SetScreen(EGEScreen NewScreen, int32 NewSelection)
{
	Screen = NewScreen;
	SelectedItem = NewSelection;
	ApplyInputState();
}

void AGEHUD::ApplyInputState()
{
	APlayerController* PC = GetOwningPlayerController();
	if (PC == nullptr)
	{
		return;
	}
	const bool bMenu = IsMenuScreen();

	// cursor + input mode: menus see the mouse, gameplay hides it and captures it
	PC->bShowMouseCursor = bMenu;
	PC->bEnableClickEvents = false;
	if (bMenu)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}

	// no walking or looking behind a menu (Reset first: the ignore flags are counters)
	PC->ResetIgnoreMoveInput();
	PC->ResetIgnoreLookInput();
	if (bMenu)
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	// weapons: off in menus and inside the aim arena (the manager fires its own shots there)
	if (const AGECharacter* Character = GetGECharacter())
	{
		if (UGEWeaponComponent* Weapons = Character->GetWeaponComponent())
		{
			const AGETargetManager* Aim = GetAim();
			Weapons->SetFiringEnabled(!bMenu && !(Aim && Aim->IsModeActive()));
		}
	}
}

// =====================================================================
//                              FLOW: ACTIONS
// =====================================================================

void AGEHUD::GoToMainMenu()
{
	UGETrainingManager* Training = UGETrainingManager::Get(this);
	AGETargetManager* Aim = GetAim();

	// leave the Training Center cleanly: stop the session, close its menu, walk back
	if (Training)
	{
		if (Training->IsSessionRunning())
		{
			Training->StopSession();
		}
		if (Training->IsMenuOpen())
		{
			Training->ToggleMenu();
		}
		if (Training->IsInTrainingCenter())
		{
			if (AGECharacter* Character = GetGECharacter())
			{
				Training->ToggleTeleport(Character);
			}
		}
	}
	bWasSessionRunning = false;

	// stand on the arena pad with the floating showcase targets behind the menu
	if (Aim)
	{
		Aim->ShowShowcase();
	}
	SetScreen(EGEScreen::MainMenu, 0);
}

void AGEHUD::StartAimMode(EGEAimMode Mode)
{
	AGETargetManager* Aim = GetAim();
	if (Aim && Aim->StartMode(Mode))
	{
		ResultKind = EGEResultKind::Aim;
		SetScreen(EGEScreen::Playing, 0);
	}
}

void AGEHUD::StartFreeRoam()
{
	if (AGETargetManager* Aim = GetAim())
	{
		Aim->StopMode(); // back to where the player was, movement and shooting restored
	}
	SetScreen(EGEScreen::FreeRoam, 0);
}

void AGEHUD::StartTrainingScreen(bool bFlick)
{
	UGETrainingManager* Training = UGETrainingManager::Get(this);
	AGECharacter* Character = GetGECharacter();
	if (Training == nullptr || Training->GetCenter() == nullptr || Character == nullptr)
	{
		StartFreeRoam(); // no Training Center in this level
		return;
	}

	if (AGETargetManager* Aim = GetAim())
	{
		Aim->StopMode();
	}

	// Flick Optimizer = the Training Center in flick mode; Training Center = any other mode
	const EGETrainingMode Wanted = bFlick ? EGETrainingMode::Flick
		: (Training->GetMode() == EGETrainingMode::Flick ? EGETrainingMode::Static : Training->GetMode());
	Training->SetMode(Wanted);

	SetScreen(bFlick ? EGEScreen::FlickOptimizer : EGEScreen::TrainingCenter, 0);
	if (!Training->IsInTrainingCenter())
	{
		Training->ToggleTeleport(Character);
	}
	if (!Training->IsMenuOpen())
	{
		Training->ToggleMenu(); // the old Training Center menu (cursor on, mouse look off)
	}
	bWasSessionRunning = Training->IsSessionRunning();
}

void AGEHUD::OpenPause()
{
	PauseReturn = Screen;
	if (Screen == EGEScreen::Playing)
	{
		if (AGETargetManager* Aim = GetAim())
		{
			Aim->SetPaused(true);
		}
	}
	SetScreen(EGEScreen::Pause, 0);
}

void AGEHUD::ResumeFromPause()
{
	if (PauseReturn == EGEScreen::Playing)
	{
		if (AGETargetManager* Aim = GetAim())
		{
			Aim->SetPaused(false);
		}
	}
	SetScreen(PauseReturn, 0);
}

void AGEHUD::PlayAgain()
{
	if (ResultKind == EGEResultKind::Aim)
	{
		if (AGETargetManager* Aim = GetAim())
		{
			StartAimMode(Aim->GetMode());
		}
		return;
	}

	// Training Center / Flick: same settings, new session
	if (UGETrainingManager* Training = UGETrainingManager::Get(this))
	{
		SetScreen(ResultReturn, 0);
		Training->StartSession();
		bWasSessionRunning = Training->IsSessionRunning();
	}
}

void AGEHUD::ShowTrainingResults()
{
	UGETrainingManager* Training = UGETrainingManager::Get(this);
	if (Training == nullptr)
	{
		return;
	}
	if (Screen != EGEScreen::FreeRoam && Screen != EGEScreen::TrainingCenter && Screen != EGEScreen::FlickOptimizer)
	{
		return;
	}
	if (Training->IsMenuOpen())
	{
		Training->ToggleMenu(); // close the old menu: the Results screen takes over the cursor
	}
	ResultReturn = Screen;
	ResultKind = (Training->GetMode() == EGETrainingMode::Flick) ? EGEResultKind::Flick : EGEResultKind::Training;
	SetScreen(EGEScreen::Results, 0);
}

void AGEHUD::GoBack()
{
	UGETrainingManager* Training = UGETrainingManager::Get(this);
	switch (Screen)
	{
	case EGEScreen::MainMenu:
		break; // nothing to go back to
	case EGEScreen::ModeSelect:
		SetScreen(EGEScreen::MainMenu, MenuAim);
		break;
	case EGEScreen::Settings:
		if (UGESaveGame* Save = GetSave()) { Save->Save(); }
		if (SettingsReturn == EGEScreen::Pause)
		{
			SetScreen(EGEScreen::Pause, GetPauseItems().Find(PauseSettings));
		}
		else
		{
			SetScreen(EGEScreen::MainMenu, MenuSettings);
		}
		break;
	case EGEScreen::Results:
		GoToMainMenu();
		break;
	case EGEScreen::Pause:
		ResumeFromPause();
		break;
	case EGEScreen::Playing:
		OpenPause();
		break;
	default: // FreeRoam, TrainingCenter, FlickOptimizer
		if (Training && Training->IsMenuOpen())
		{
			Training->ToggleMenu(); // Tab closes the Training Center menu first
		}
		else
		{
			OpenPause();
		}
		break;
	}
}

void AGEHUD::MoveSelection(int32 Direction)
{
	const int32 Count = GetItemCount();
	if (Count <= 0)
	{
		return;
	}
	SelectedItem = (SelectedItem + Direction + Count) % Count; // wraps around
	GESounds::PlayClick(this, GetVolume());
}

void AGEHUD::ActivateSelected()
{
	switch (Screen)
	{
	case EGEScreen::MainMenu:
		GESounds::PlayClick(this, GetVolume());
		switch (SelectedItem)
		{
		case MenuAim:      SetScreen(EGEScreen::ModeSelect, 0); break;
		case MenuTraining: StartTrainingScreen(false); break;
		case MenuFlick:    StartTrainingScreen(true); break;
		case MenuFreeRoam: StartFreeRoam(); break;
		case MenuSettings: SettingsReturn = EGEScreen::MainMenu; SetScreen(EGEScreen::Settings, 0); break;
		case MenuQuit:     UKismetSystemLibrary::QuitGame(this, GetOwningPlayerController(), EQuitPreference::Quit, false); break;
		}
		break;

	case EGEScreen::ModeSelect:
		if (SelectedItem < GEAimModes::NumModes)
		{
			StartAimMode((EGEAimMode)SelectedItem);
		}
		else
		{
			GoBack();
		}
		break;

	case EGEScreen::Settings:
		if (SelectedItem < GEStyle::NumSettings)
		{
			ChangeSetting(SelectedItem, +1); // a setting row = increase
		}
		else
		{
			GoBack();
		}
		break;

	case EGEScreen::Results:
		GESounds::PlayClick(this, GetVolume());
		if (SelectedItem == 0) { PlayAgain(); } else { GoToMainMenu(); }
		break;

	case EGEScreen::Pause:
	{
		const TArray<int32> Items = GetPauseItems();
		if (!Items.IsValidIndex(SelectedItem))
		{
			break;
		}
		GESounds::PlayClick(this, GetVolume());
		switch (Items[SelectedItem])
		{
		case PauseResume:
			ResumeFromPause();
			break;
		case PauseRestart:
			if (AGETargetManager* Aim = GetAim())
			{
				Aim->SetPaused(false);
				StartAimMode(Aim->GetMode());
			}
			break;
		case PauseSettings:
			SettingsReturn = EGEScreen::Pause;
			SetScreen(EGEScreen::Settings, 0);
			break;
		case PauseMainMenu:
			GoToMainMenu();
			break;
		}
		break;
	}
	default:
		break;
	}
}

void AGEHUD::ChangeSetting(int32 Index, int32 Direction)
{
	UGESaveGame* Save = GetSave();
	UGETrainingManager* Training = UGETrainingManager::Get(this);
	if (Save == nullptr || Training == nullptr)
	{
		return;
	}

	switch (Index)
	{
	case 0: // sensitivity: GE's existing look-sensitivity multiplier (1.0 = default)
	{
		const float NewScale = FMath::RoundToFloat((Training->GetSensitivityScale() + SensStep * Direction) * 10.0f) / 10.0f;
		Training->SetSensitivityScale(NewScale); // clamps and saves by itself
		break;
	}
	case 1: // field of view
		Save->FieldOfView = FMath::Clamp(Save->FieldOfView + FovStep * Direction, MinFov, MaxFov);
		break;
	case 2: // volume
		Save->Volume = FMath::Clamp(FMath::RoundToFloat((Save->Volume + VolumeStep * Direction) * 10.0f) / 10.0f, 0.0f, 1.0f);
		break;
	case 3: // crosshair color (cycles)
		Save->CrosshairColor = (Save->CrosshairColor + Direction + GEStyle::NumCrosshairColors) % GEStyle::NumCrosshairColors;
		break;
	case 4: // UI theme (cycles; applied immediately, the HUD also re-reads it every frame)
		Save->UITheme = (Save->UITheme + Direction + GEStyle::NumThemes) % GEStyle::NumThemes;
		GEStyle::SetActiveTheme(Save->UITheme);
		break;
	case 5: // scanlines on/off
		Save->bScanlines = !Save->bScanlines;
		break;
	case 6: // glitch on/off
		Save->bGlitch = !Save->bGlitch;
		break;
	}
	Save->Save(); // saved on every change
	GESounds::PlayClick(this, GetVolume());
}

// =====================================================================
//                              FLOW: KEYS
// =====================================================================

void AGEHUD::HandleKeys()
{
	APlayerController* PC = GetOwningPlayerController();
	if (PC == nullptr)
	{
		return;
	}

	// Tab = back / pause. Esc does the same in a packaged game (in Play-In-Editor Esc stops the
	// session before we see it, so Tab is the documented key).
	if (PC->WasInputKeyJustPressed(EKeys::Tab) || PC->WasInputKeyJustPressed(EKeys::Escape))
	{
		GoBack();
		return;
	}

	// R = restart the round (Playing) or Play Again (Results)
	if (PC->WasInputKeyJustPressed(EKeys::R))
	{
		if (Screen == EGEScreen::Playing)
		{
			if (AGETargetManager* Aim = GetAim())
			{
				StartAimMode(Aim->GetMode());
			}
			return;
		}
		if (Screen == EGEScreen::Results)
		{
			PlayAgain();
			return;
		}
	}

	if (!IsMenuScreen())
	{
		return; // W/A/S/D are for walking outside the menus
	}

	const bool bUp = PC->WasInputKeyJustPressed(EKeys::W) || PC->WasInputKeyJustPressed(EKeys::Up);
	const bool bDown = PC->WasInputKeyJustPressed(EKeys::S) || PC->WasInputKeyJustPressed(EKeys::Down);
	const bool bLeft = PC->WasInputKeyJustPressed(EKeys::A) || PC->WasInputKeyJustPressed(EKeys::Left);
	const bool bRight = PC->WasInputKeyJustPressed(EKeys::D) || PC->WasInputKeyJustPressed(EKeys::Right);
	const bool bEnter = PC->WasInputKeyJustPressed(EKeys::Enter) || PC->WasInputKeyJustPressed(EKeys::SpaceBar);

	if (bUp)   { MoveSelection(-1); }
	if (bDown) { MoveSelection(+1); }

	if (Screen == EGEScreen::Settings && SelectedItem < GEStyle::NumSettings)
	{
		if (bLeft)  { ChangeSetting(SelectedItem, -1); }
		if (bRight) { ChangeSetting(SelectedItem, +1); }
	}
	else if (Screen == EGEScreen::Results)
	{
		if (bLeft || bRight) { MoveSelection(bRight ? +1 : -1); } // two buttons side by side
	}

	if (bEnter)
	{
		ActivateSelected();
		return;
	}

	// mouse: left click activates the hovered button, right click lowers a hovered setting
	if (HoveredItem >= 0)
	{
		if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			SelectedItem = HoveredItem;
			ActivateSelected();
		}
		else if (PC->WasInputKeyJustPressed(EKeys::RightMouseButton)
			&& Screen == EGEScreen::Settings && HoveredItem < GEStyle::NumSettings)
		{
			SelectedItem = HoveredItem;
			ChangeSetting(HoveredItem, -1);
		}
	}
}

// =====================================================================
//                         PLAYING (aim round HUD)
// =====================================================================
// Stage B4: readability first. Every module sits on a dark semi-transparent backing
// (FGEUIKit::HudModule) so text stays readable on bright or busy backgrounds.

void AGEHUD::DrawPlaying()
{
	FGEUIKit Kit(this);
	const GEStyle::FGETheme& Th = GEStyle::Active();
	AGETargetManager* Aim = GetAim();
	const UGESaveGame* Save = GetSave();
	const FLinearColor CrosshairColor = GEStyle::GetCrosshairColor(Save ? Save->CrosshairColor : 0);
	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;
	if (Aim == nullptr)
	{
		DrawCrosshairAt(CenterX, CenterY, CrosshairColor, 0.0f);
		return;
	}

	// last 5 seconds of a timed mode (not during the countdown): warning colors + one short glitch
	const bool bLow = Aim->IsLowOnTime() && !Aim->IsCountingDown();
	if (bLow && !bWasLowTime)
	{
		LowTimeStartTime = NowTime;
	}
	bWasLowTime = bLow;

	// ---------- timer module (top center, mono) ----------
	{
		const float W = Px(260);
		const float H = Px(92);
		const float X = CenterX - W * 0.5f;
		const float Y = Px(28);
		Kit.HudModule(X, Y, W, H, bLow);

		FLinearColor TimerColor = Th.TextStrong;
		if (bLow)
		{
			const float Pulse = 0.5f + 0.5f * FMath::Sin(NowTime * 10.0f);
			TimerColor = FMath::Lerp(Th.TextStrong, Th.StatusDanger, Pulse);
		}
		Kit.Text(Aim->GetTimerText(), CenterX, Y + Px(8), TimerColor, 52.0f, 0.5f, true);
		Kit.ProgressBar(X + Px(20), Y + H - Px(22), W - Px(40), Px(6), Aim->GetTimerFraction(), bLow ? Th.StatusDanger : Th.AccentPrimary);
	}

	// ---------- stats module (left) ----------
	{
		const TArray<FGEStatLine> Stats = Aim->GetLiveStats();
		const float X = Px(36);
		const float Y = Px(28);
		const float W = Px(300);
		const float RowH = Px(36);
		const float H = Px(58) + Stats.Num() * RowH;
		Kit.HudModule(X, Y, W, H);
		Kit.Rect(X, Y, Px(3), H, Th.AccentPrimary); // accent stripe

		Kit.Text(GEAimModes::GetName(Aim->GetMode()).ToUpper(), X + Px(20), Y + Px(14), Th.TextPrimary, GEStyle::Type::LabelL);
		for (int32 i = 0; i < Stats.Num(); i++)
		{
			Kit.StatRow(X + Px(20), Y + Px(52) + i * RowH, W - Px(40), Stats[i].Label, Stats[i].Value);
		}
	}

	// ---------- mode banner (while the round starts) ----------
	if (Aim->IsCountingDown() || (Aim->IsRunning() && Aim->GetTimeSinceGo() < 1.5f))
	{
		const float W = Px(520);
		const float H = Px(74);
		const float X = CenterX - W * 0.5f;
		const float Y = Px(144);
		Kit.HudModule(X, Y, W, H, true);
		Kit.Rect(X, Y, Px(3), H, Th.AccentPrimary);
		Kit.Text(GEAimModes::GetName(Aim->GetMode()).ToUpper() + TEXT("   //   ") + GEAimModes::GetDurationText(Aim->GetMode()).ToUpper(),
			CenterX, Y + Px(10), Th.TextStrong, GEStyle::Type::H3, 0.5f);
		const TArray<FString> Description = GEAimModes::GetDescription(Aim->GetMode());
		if (Description.Num() > 0)
		{
			Kit.Text(Description[0], CenterX, Y + Px(42), Th.TextBody, GEStyle::Type::BodyM, 0.5f);
		}
	}

	// ---------- hints (bottom left) ----------
	{
		const float X = Px(36);
		const float H = Px(40);
		const float Y = Canvas->ClipY - Px(150);
		Kit.HudModule(X, Y, Px(250), H);
		Kit.Text(TEXT("R  RESTART      TAB  PAUSE"), X + Px(16), Y + Px(11), Th.TextBody, GEStyle::Type::LabelM);
	}

	// ---------- message under the crosshair (Reflex) ----------
	const FString Message = Aim->GetCenterMessage();
	if (!Message.IsEmpty())
	{
		const bool bEarly = Message.StartsWith(TEXT("Too"));
		const float Pulse = 0.65f + 0.35f * FMath::Sin(NowTime * 6.0f);
		const FLinearColor Color = bEarly ? Th.StatusDanger : GEStyle::WithAlpha(Th.TextStrong, Pulse);
		const float TextW = Kit.Measure(Message, GEStyle::Type::H3).X;
		Kit.HudModule(CenterX - TextW * 0.5f - Px(24), CenterY + Px(62), TextW + Px(48), Px(52), bEarly);
		Kit.Text(Message, CenterX, CenterY + Px(70), Color, GEStyle::Type::H3, 0.5f);
	}

	DrawAimPopups();
	DrawCountdown(Th.AccentPrimary);
	DrawAimHitMarker();

	// the short glitch when the timer enters the last seconds (respects the Screen glitch setting)
	if (bLow && Save && Save->bGlitch)
	{
		Kit.Glitch(NowTime - LowTimeStartTime);
	}

	if (Screen == EGEScreen::Playing)
	{
		DrawCrosshairAt(CenterX, CenterY, CrosshairColor, 0.0f); // last, so it is on top
	}
}

void AGEHUD::DrawAimHitMarker()
{
	const AGETargetManager* Aim = GetAim();
	if (Aim == nullptr)
	{
		return;
	}
	// X that fades out over 0.15 s after a hit (dark outline first so it reads on bright walls)
	const float Age = NowTime - Aim->LastHitTime;
	if (Age >= 0.15f)
	{
		return;
	}
	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;
	const float Inner = Px(9);
	const float Outer = Px(19);
	const float Fade = 1.0f - Age / 0.15f;
	const FLinearColor Dark = GEStyle::WithAlpha(GEStyle::Active().BgSunken, 0.8f * Fade);
	const FLinearColor Color = GEStyle::WithAlpha(GEStyle::Active().TextStrong, Fade);

	for (int32 Pass = 0; Pass < 2; Pass++)
	{
		const FLinearColor& C = (Pass == 0) ? Dark : Color;
		const float Thick = (Pass == 0) ? 4.0f : 2.0f;
		DrawLine(CenterX - Outer, CenterY - Outer, CenterX - Inner, CenterY - Inner, C, Thick);
		DrawLine(CenterX + Inner, CenterY + Inner, CenterX + Outer, CenterY + Outer, C, Thick);
		DrawLine(CenterX + Outer, CenterY - Outer, CenterX + Inner, CenterY - Inner, C, Thick);
		DrawLine(CenterX - Inner, CenterY + Inner, CenterX - Outer, CenterY + Outer, C, Thick);
	}
}

void AGEHUD::DrawCountdown(const FLinearColor& ModeColor)
{
	const AGETargetManager* Aim = GetAim();
	if (Aim == nullptr)
	{
		return;
	}
	const GEStyle::FGETheme& Th = GEStyle::Active();
	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;

	if (Aim->IsCountingDown())
	{
		const float Left = Aim->GetCountdownLeft();
		const int32 Number = FMath::CeilToInt(Left);

		// each number starts big and settles; it fades out at the end of its second
		const float InSecond = Left - FMath::FloorToFloat(Left); // 1 -> 0 during each second
		const float Size = 150.0f * (1.0f + 0.35f * FMath::Pow(InSecond, 4.0f));
		const float Alpha = FMath::Clamp(InSecond * 4.0f, 0.0f, 1.0f);

		DrawLabel(TEXT("GET READY"), CenterX, CenterY - Px(250), GEStyle::WithAlpha(Th.TextBody, 0.9f), GEStyle::Type::H3, 0.5f);

		const FString Text = FString::FromInt(Number);
		const FVector2D TextSize = MeasureText(Text, Size, true);
		DrawGlowLabel(Text, CenterX, CenterY - Px(150) - TextSize.Y * 0.5f,
			GEStyle::WithAlpha(Th.TextStrong, Alpha), GEStyle::WithAlpha(ModeColor, Alpha), Size, 0.5f, true);
		return;
	}

	// "GO!" for 0.6 seconds after the countdown
	const float SinceGo = Aim->GetTimeSinceGo();
	if (Aim->IsRunning() && SinceGo < 0.6f)
	{
		const float Alpha = 1.0f - SinceGo / 0.6f;
		const float Size = 130.0f * (1.0f + 0.4f * SinceGo);
		const FVector2D TextSize = MeasureText(TEXT("GO!"), Size);
		DrawGlowLabel(TEXT("GO!"), CenterX, CenterY - Px(150) - TextSize.Y * 0.5f,
			GEStyle::WithAlpha(Th.TextStrong, Alpha), GEStyle::WithAlpha(Th.StatusSuccess, Alpha), Size, 0.5f);
	}
}

void AGEHUD::DrawAimPopups()
{
	const AGETargetManager* Aim = GetAim();
	if (Aim == nullptr)
	{
		return;
	}
	const GEStyle::FGETheme& Th = GEStyle::Active();
	for (const FGEScorePopup& Popup : Aim->GetPopups())
	{
		const float Age = NowTime - Popup.SpawnTime;
		const float Life = FMath::Clamp(Age / 0.9f, 0.0f, 1.0f);

		// float upward in the world, then turn it into a screen position
		const FVector WorldPos = Popup.Location + FVector(0.0f, 0.0f, 50.0f + 90.0f * EaseOut(Life));
		const FVector ScreenPos = Project(WorldPos);
		if (ScreenPos.Z <= 0.0f)
		{
			continue; // behind the camera
		}
		const float Fade = 1.0f - Life;
		DrawGlowLabel(Popup.Text, ScreenPos.X, ScreenPos.Y, GEStyle::WithAlpha(Th.TextStrong, Fade),
			GEStyle::WithAlpha(Th.StatusSuccess, Fade), GEStyle::Type::DataL, 0.5f, true);
	}
}

// =====================================================================
//                       MAIN MENU / MODE SELECT
// =====================================================================

void AGEHUD::DrawBackdrop(float Alpha1, float Alpha2, bool bHorizontal)
{
	const FLinearColor Base = GEStyle::Active().BgSunken;
	DrawGradient(0, 0, Canvas->ClipX, Canvas->ClipY, GEStyle::WithAlpha(Base, Alpha1), GEStyle::WithAlpha(Base, Alpha2), bHorizontal);
}

void AGEHUD::DrawMainMenu()
{
	FGEUIKit Kit(this);
	const GEStyle::FGETheme& Th = GEStyle::Active();
	const float Age = ScreenAge();
	const float Intro = EaseOut(Age / 0.4f);
	const float Left = Px(120);

	// dark on the left (behind the title and nav), see-through on the right (the showcase targets)
	DrawBackdrop(0.9f, 0.2f, true);
	Kit.ScreenChrome(TEXT("RAYSHOT // MAIN MENU"));

	// ---------- tag, title, subtitle ----------
	const float Slide = Px(30) * (1.0f - Intro);
	Kit.Rect(Left - Slide, Px(150), Px(36), Px(2), Th.StatusInfo);
	Kit.Text(TEXT("RAKSHIT SINGH // 2026"), Left + Px(48) - Slide, Px(140), GEStyle::WithAlpha(Th.StatusInfo, Intro), GEStyle::Type::LabelL);
	Kit.GlowText(TEXT("RAYSHOT"), Left - Slide, Px(170), GEStyle::WithAlpha(Th.TextStrong, Intro), GEStyle::WithAlpha(Th.AccentPrimary, Intro), 140.0f);
	Kit.Rect(Left, Px(332), Px(120) * Intro, Px(4), Th.AccentPrimary);
	Kit.Text(TEXT("AIM, TRACK AND FLICK. HIT DETECTION BY A CUSTOM RAY TRACER."), Left, Px(350), GEStyle::WithAlpha(Th.TextBody, Intro), GEStyle::Type::BodyL);

	// ---------- nav items ----------
	struct FItem { const TCHAR* Label; const TCHAR* Right; };
	const FItem Items[NumMenuItems] = {
		{ TEXT("AIM TRAINER"),     TEXT("4 MODES") },
		{ TEXT("TRAINING CENTER"), TEXT("") },
		{ TEXT("FLICK OPTIMIZER"), TEXT("") },
		{ TEXT("FREE ROAM"),       TEXT("") },
		{ TEXT("SETTINGS"),        TEXT("") },
		{ TEXT("QUIT"),            TEXT("") },
	};
	const float ItemW = Px(480);
	const float ItemH = Px(52);
	const float Gap = Px(10);
	float Y = Px(412);
	for (int32 i = 0; i < NumMenuItems; i++)
	{
		if (i == MenuSettings)
		{
			Y += Px(14); // little gap before Settings / Quit
		}
		const float ItemIntro = EaseOut((Age - 0.05f * i) / 0.3f); // items slide in one after another
		const float X = Left - Px(40) * (1.0f - ItemIntro);
		const bool bHover = HitTestItem(i, X, Y, ItemW, ItemH);
		const bool bSel = (SelectedItem == i);
		Kit.NavItem(X, Y, ItemW, ItemH, i + 1, Items[i].Label, bSel ? EGENavState::Selected : (bHover ? EGENavState::Hover : EGENavState::Default));
		if (Items[i].Right[0] != 0)
		{
			Kit.TextCentered(Items[i].Right, X + ItemW - Px(24), Y, ItemH, bSel ? Th.TextOnAccent : Th.TextMuted, GEStyle::Type::DataM, 1.0f, true);
		}
		Y += ItemH + Gap;
	}

	DrawInfoCard(SelectedItem);

	// ---------- footer ----------
	Kit.Text(TEXT("W / S  OR MOUSE TO CHOOSE      ENTER / CLICK TO SELECT      TAB = BACK / PAUSE"), Canvas->ClipX * 0.5f, Canvas->ClipY - Px(84),
		Th.TextMuted, GEStyle::Type::LabelS, 0.5f);
}

void AGEHUD::DrawInfoCard(int32 Selected)
{
	FGEUIKit Kit(this);
	const GEStyle::FGETheme& Th = GEStyle::Active();
	const float Intro = EaseOut(ScreenAge() / 0.4f);
	const float CardW = Px(500);
	const float CardX = Canvas->ClipX - Px(120) - CardW + Px(30) * (1.0f - Intro);
	const float CardY = Px(412);
	const float TextX = CardX + Px(36);

	// what to tell the player about the highlighted entry
	FString Title;
	TArray<FString> Lines;
	switch (Selected)
	{
	case MenuAim:
		Title = TEXT("AIM TRAINER");
		Lines = { TEXT("Gridshot, Microshot, Tracking, Reflex."), TEXT("Timed rounds with personal bests."), TEXT("Trains: aim, speed, reaction") };
		break;
	case MenuTraining:
		Title = TEXT("TRAINING CENTER");
		Lines = { TEXT("Fight bots with your 5 weapons."), TEXT("Static, strafing and peeking targets."), TEXT("The AI Coach gives you 3 tips") };
		break;
	case MenuFlick:
		Title = TEXT("FLICK OPTIMIZER");
		Lines = { TEXT("Flick to targets in every direction."), TEXT("Measures overshoot and speed."), TEXT("Suggests a better sensitivity") };
		break;
	case MenuFreeRoam:
		Title = TEXT("FREE ROAM");
		Lines = { TEXT("Walk around the room."), TEXT("All 5 weapons, shoot the dummies."), TEXT("T = Training Center, M = its menu") };
		break;
	case MenuSettings:
		Title = TEXT("SETTINGS");
		Lines = { TEXT("Mouse, video, audio, crosshair"), TEXT("and interface options.") };
		break;
	default:
		Title = TEXT("QUIT");
		Lines = { TEXT("See you next session.") };
		break;
	}

	const float CardH = Px(270);
	Kit.Panel(CardX, CardY, CardW, CardH, 12.0f, GECut::BR, true);
	Kit.Rect(CardX, CardY, CardW, Px(3), Th.AccentPrimary);
	Kit.GlowText(Title, TextX, CardY + Px(28), Th.TextStrong, Th.AccentPrimary, GEStyle::Type::H2);
	for (int32 i = 0; i < Lines.Num(); i++)
	{
		const bool bTrains = (i == Lines.Num() - 1 && Lines.Num() == 3);
		Kit.Text(Lines[i], TextX, CardY + Px(100) + i * Px(34), bTrains ? Th.StatusInfo : Th.TextBody, GEStyle::Type::BodyL);
	}
	const float Pulse = 0.6f + 0.4f * FMath::Sin(NowTime * 4.0f);
	Kit.Text(TEXT("PRESS ENTER OR CLICK"), TextX, CardY + CardH - Px(46), GEStyle::WithAlpha(Th.TextPrimary, Pulse), GEStyle::Type::LabelM);
}

void AGEHUD::DrawModeSelect()
{
	FGEUIKit Kit(this);
	const GEStyle::FGETheme& Th = GEStyle::Active();
	const AGETargetManager* Aim = GetAim();
	const float Age = ScreenAge();
	const float Intro = EaseOut(Age / 0.4f);
	const float Left = Px(120);

	DrawBackdrop(0.9f, 0.45f, true);
	Kit.ScreenChrome(TEXT("AIM TRAINER // MODE SELECT"));

	const float Slide = Px(30) * (1.0f - Intro);
	Kit.GlowText(TEXT("AIM TRAINER"), Left - Slide, Px(110), GEStyle::WithAlpha(Th.TextStrong, Intro), GEStyle::WithAlpha(Th.AccentPrimary, Intro), GEStyle::Type::DisplayL);
	Kit.Rect(Left, Px(196), Px(120) * Intro, Px(4), Th.AccentPrimary);
	Kit.Text(TEXT("CHOOSE A MODE"), Left, Px(214), GEStyle::WithAlpha(Th.TextBody, Intro), GEStyle::Type::LabelL);
	Kit.PageIndicator(Left + Px(190), Px(206), FMath::Min(SelectedItem, GEAimModes::NumModes - 1) + 1, GEAimModes::NumModes, TEXT("MODE"));

	// ---------- 4 diamond tiles ----------
	const float Radius = Px(92);
	const float TileGap = Px(240);
	const float TileY = Px(520);
	for (int32 i = 0; i < GEAimModes::NumModes; i++)
	{
		const EGEAimMode Mode = (EGEAimMode)i;
		const float TileIntro = EaseOut((Age - 0.06f * i) / 0.3f);
		const float CX = Left + Px(100) + i * TileGap;
		const float CY = TileY + Px(30) * (1.0f - TileIntro);
		HitTestItem(i, CX - Radius * 0.8f, CY - Radius * 0.8f, Radius * 1.6f, Radius * 1.6f + Px(60));
		Kit.DiamondTile(CX, CY, Radius, GEAimModes::GetName(Mode), SelectedItem == i);

		// personal best in YELLOW under the tile name
		float Best = 0.0f;
		const bool bHasBest = Aim && Aim->GetBestScore(Mode, Best);
		Kit.Text(bHasBest ? GEAimModes::FormatScore(Mode, Best) : FString(TEXT("NEW")), CX, CY + Radius + Px(38),
			bHasBest ? Th.StatusWarning : Th.TextMuted, GEStyle::Type::DataM, 0.5f, true);
	}

	// ---------- back ----------
	{
		const float BX = Left;
		const float BY = Px(790);
		const float BW = Px(220);
		const float BH = Px(54);
		const bool bHover = HitTestItem(GEAimModes::NumModes, BX, BY, BW, BH);
		Kit.Button(BX, BY, BW, BH, TEXT("BACK"), EGEButtonStyle::Secondary,
			(SelectedItem == GEAimModes::NumModes || bHover) ? EGEButtonState::Hover : EGEButtonState::Default, false);
	}

	// ---------- description card for the highlighted mode ----------
	if (SelectedItem < GEAimModes::NumModes)
	{
		const EGEAimMode Mode = (EGEAimMode)SelectedItem;
		const float CardW = Px(500);
		const float CardX = Canvas->ClipX - Px(120) - CardW + Px(30) * (1.0f - Intro);
		const float CardY = Px(300);
		const float CardH = Px(500);
		const float TextX = CardX + Px(36);
		const float InnerW = CardW - Px(72);

		Kit.Panel(CardX, CardY, CardW, CardH, 12.0f, GECut::BR, true);
		Kit.Rect(CardX, CardY, CardW, Px(3), Th.AccentPrimary);
		Kit.Text(FString::Printf(TEXT("%02d"), SelectedItem + 1), TextX, CardY + Px(24), Th.StatusWarning, GEStyle::Type::DataL, 0.0f, true);
		Kit.GlowText(GEAimModes::GetName(Mode).ToUpper(), TextX, CardY + Px(56), Th.TextStrong, Th.AccentPrimary, GEStyle::Type::H1);

		const TArray<FString> Lines = GEAimModes::GetDescription(Mode);
		for (int32 i = 0; i < Lines.Num(); i++)
		{
			const bool bLast = (i == Lines.Num() - 1);
			Kit.Text(Lines[i], TextX, CardY + Px(132) + i * Px(32), bLast ? Th.StatusInfo : Th.TextBody, GEStyle::Type::BodyL);
		}

		Kit.StatRow(TextX, CardY + Px(262), InnerW, TEXT("Duration"), GEAimModes::GetDurationText(Mode).ToUpper());
		float Best = 0.0f;
		const bool bHasBest = Aim && Aim->GetBestScore(Mode, Best);
		Kit.Text(TEXT("PERSONAL BEST"), TextX, CardY + Px(318), Th.TextMuted, GEStyle::Type::LabelM);
		Kit.Text(bHasBest ? GEAimModes::FormatScore(Mode, Best) : FString(TEXT("NOT PLAYED YET")), TextX, CardY + Px(342),
			bHasBest ? Th.StatusWarning : Th.TextMuted, GEStyle::Type::H2, 0.0f, true);

		const float Pulse = 0.6f + 0.4f * FMath::Sin(NowTime * 4.0f);
		Kit.Text(TEXT("PRESS ENTER OR CLICK TO PLAY"), TextX, CardY + CardH - Px(52), GEStyle::WithAlpha(Th.TextPrimary, Pulse), GEStyle::Type::LabelM);
	}

	Kit.Text(TEXT("W / S  OR MOUSE TO CHOOSE      ENTER / CLICK TO PLAY      TAB = BACK"), Canvas->ClipX * 0.5f, Canvas->ClipY - Px(84),
		Th.TextMuted, GEStyle::Type::LabelS, 0.5f);
}

// =====================================================================
//                               SETTINGS
// =====================================================================

void AGEHUD::DrawSettings()
{
	const UGESaveGame* Save = GetSave();
	const UGETrainingManager* Training = UGETrainingManager::Get(this);
	if (Save == nullptr || Training == nullptr)
	{
		return;
	}
	FGEUIKit Kit(this);
	const GEStyle::FGETheme& Th = GEStyle::Active();

	const float Intro = EaseOut(ScreenAge() / 0.35f);
	DrawBackdrop(0.9f, 0.8f, false);
	Kit.ScreenChrome(TEXT("SETTINGS"));

	// which of the 5 numbered cards holds the focused row (items 0..6 -> sections 0..4)
	static const int32 SectionOfItem[GEStyle::NumSettings] = { 0, 1, 2, 3, 4, 4, 4 };
	const int32 Section = (SelectedItem >= 0 && SelectedItem < GEStyle::NumSettings) ? SectionOfItem[SelectedItem] : 4;

	// ---------- card grid: 3 + (1 + wide 1) ----------
	const float CardW = Px(540);
	const float Gutter = Px(24);
	const float GridW = CardW * 3 + Gutter * 2;
	const float X0 = (Canvas->ClipX - GridW) * 0.5f;
	const float Row1Y = Px(218) + Px(24) * (1.0f - Intro);
	const float Row2Y = Row1Y + Px(296);
	const float CardH = Px(272);

	// ---------- title, tabs, page indicator ----------
	Kit.GlowText(TEXT("SETTINGS"), X0, Px(84), Th.TextStrong, Th.AccentPrimary, GEStyle::Type::H1);
	TArray<FString> Tabs;
	Tabs.Add(TEXT("MOUSE")); Tabs.Add(TEXT("VIDEO")); Tabs.Add(TEXT("AUDIO")); Tabs.Add(TEXT("CROSSHAIR")); Tabs.Add(TEXT("INTERFACE"));
	Kit.HeaderTabs(X0 + Px(280), Px(104), Tabs, Section);
	Kit.PageIndicator(X0 + GridW - Px(190), Px(100), Section + 1, 5, TEXT("SECTION"));

	// small helpers: card frame + numbered heading
	auto DrawCardFrame = [&](float X, float Y, float W, int32 Number, const TCHAR* Heading, bool bActive)
	{
		Kit.Panel(X, Y, W, CardH, 12.0f, GECut::BR, bActive);
		Kit.Text(FString::Printf(TEXT("%02d"), Number), X + Px(28), Y + Px(20), Th.StatusWarning, GEStyle::Type::DataL, 0.0f, true);
		Kit.Text(Heading, X + Px(82), Y + Px(24), Th.StatusInfo, GEStyle::Type::Title);
		Kit.Rect(X + Px(28), Y + Px(62), W - Px(56), Px(1), Th.BorderFaint);
	};
	// a row of color swatches; returns nothing, the selected one gets a glow + outline
	auto DrawSwatches = [&](float X, float Y, int32 Count, int32 Selected, bool bFocused, TFunctionRef<FLinearColor(int32)> ColorOf)
	{
		const float Size = Px(34);
		const float SwGap = Px(12);
		for (int32 i = 0; i < Count; i++)
		{
			const float SX = X + i * (Size + SwGap);
			if (i == Selected)
			{
				Kit.GlowRect(SX, Y, Size, Size, Th.EffectGlow, Px(8));
			}
			Kit.ChamferFill(SX, Y, Size, Size, Px(7), GECut::BR, ColorOf(i));
			Kit.ChamferOutline(SX - Px(4), Y - Px(4), Size + Px(8), Size + Px(8), Px(8), GECut::BR,
				i == Selected ? (bFocused ? Th.AccentStrong : Th.TextBody) : FLinearColor::Transparent, Px(1.5f));
		}
	};

	const float Inset = Px(28);
	const float InnerW = CardW - Inset * 2;

	// ---------- 01 MOUSE ----------
	{
		const float X = X0, Y = Row1Y;
		DrawCardFrame(X, Y, CardW, 1, TEXT("MOUSE"), Section == 0);
		const float Sens = Training->GetSensitivityScale();
		const float Fraction = (Sens - GESensitivity::MinScale) / (GESensitivity::MaxScale - GESensitivity::MinScale);
		HitTestItem(0, X + Inset, Y + Px(86), InnerW, Px(60));
		Kit.Slider(X + Inset, Y + Px(96), InnerW, TEXT("Sensitivity"), Fraction, FString::Printf(TEXT("%.1f"), Sens), SelectedItem == 0);
		Kit.Text(TEXT("1.0 IS THE DEFAULT. USE A / D OR CLICK."), X + Inset, Y + Px(176), Th.TextMuted, GEStyle::Type::LabelS);
	}
	// ---------- 02 VIDEO ----------
	{
		const float X = X0 + CardW + Gutter, Y = Row1Y;
		DrawCardFrame(X, Y, CardW, 2, TEXT("VIDEO"), Section == 1);
		const float Fraction = (Save->FieldOfView - MinFov) / (MaxFov - MinFov);
		HitTestItem(1, X + Inset, Y + Px(86), InnerW, Px(60));
		Kit.Slider(X + Inset, Y + Px(96), InnerW, TEXT("Field of view"), Fraction, FString::Printf(TEXT("%.0f"), Save->FieldOfView), SelectedItem == 1);
		Kit.Text(TEXT("WIDER SEES MORE, NARROWER IS CLOSER."), X + Inset, Y + Px(176), Th.TextMuted, GEStyle::Type::LabelS);
	}
	// ---------- 03 AUDIO ----------
	{
		const float X = X0 + (CardW + Gutter) * 2, Y = Row1Y;
		DrawCardFrame(X, Y, CardW, 3, TEXT("AUDIO"), Section == 2);
		HitTestItem(2, X + Inset, Y + Px(86), InnerW, Px(60));
		Kit.Slider(X + Inset, Y + Px(96), InnerW, TEXT("Master volume"), Save->Volume, FString::Printf(TEXT("%03d"), FMath::RoundToInt(Save->Volume * 100.0f)), SelectedItem == 2);
		Kit.Text(TEXT("HIT, KILL AND MENU SOUNDS."), X + Inset, Y + Px(176), Th.TextMuted, GEStyle::Type::LabelS);
	}
	// ---------- 04 CROSSHAIR (swatches + live preview) ----------
	{
		const float X = X0, Y = Row2Y;
		DrawCardFrame(X, Y, CardW, 4, TEXT("CROSSHAIR"), Section == 3);
		HitTestItem(3, X + Inset, Y + Px(80), Px(290), Px(130));
		Kit.Text(TEXT("COLOR"), X + Inset, Y + Px(88), SelectedItem == 3 ? Th.TextStrong : Th.TextBody, GEStyle::Type::LabelM);
		Kit.Text(GEStyle::GetCrosshairColorName(Save->CrosshairColor).ToUpper(), X + Inset + Px(260), Y + Px(86), Th.TextPrimary, GEStyle::Type::DataM, 1.0f, true);
		DrawSwatches(X + Inset + Px(4), Y + Px(130), GEStyle::NumCrosshairColors, Save->CrosshairColor, SelectedItem == 3,
			[](int32 i) { return GEStyle::GetCrosshairColor(i); });

		// live preview box
		const float PW = Px(150), PH = Px(130);
		const float PX = X + CardW - Inset - PW;
		const float PY = Y + Px(80);
		Kit.ChamferFill(PX, PY, PW, PH, Px(8), GECut::BR, Th.BgSunken);
		Kit.ChamferOutline(PX, PY, PW, PH, Px(8), GECut::BR, Th.BorderFaint, Px(1));
		Kit.Text(TEXT("PREVIEW"), PX + PW * 0.5f, PY + Px(8), Th.TextMuted, GEStyle::Type::LabelS, 0.5f);
		DrawCrosshairAt(PX + PW * 0.5f, PY + PH * 0.55f, GEStyle::GetCrosshairColor(Save->CrosshairColor), 0.0f);
	}
	// ---------- 05 INTERFACE (theme swatches, scanlines, glitch) ----------
	{
		const float X = X0 + CardW + Gutter, Y = Row2Y;
		const float WideW = CardW * 2 + Gutter;
		DrawCardFrame(X, Y, WideW, 5, TEXT("INTERFACE"), Section == 4);
		const float ColW = (WideW - Inset * 2 - Gutter * 2) / 3.0f;
		const float C0 = X + Inset;
		const float C1 = C0 + ColW + Gutter;
		const float C2 = C1 + ColW + Gutter;

		// UI theme
		HitTestItem(4, C0, Y + Px(80), ColW, Px(130));
		Kit.Text(TEXT("UI THEME"), C0, Y + Px(88), SelectedItem == 4 ? Th.TextStrong : Th.TextBody, GEStyle::Type::LabelM);
		Kit.Text(GEStyle::GetThemeName(Save->UITheme).ToUpper(), C0 + ColW, Y + Px(86), Th.TextPrimary, GEStyle::Type::DataM, 1.0f, true);
		DrawSwatches(C0 + Px(4), Y + Px(130), GEStyle::NumThemes, Save->UITheme, SelectedItem == 4,
			[](int32 i) { return GEStyle::GetTheme((GEStyle::EGETheme)i).AccentPrimary; });

		// scanlines
		HitTestItem(5, C1, Y + Px(80), ColW, Px(130));
		Kit.Toggle(C1, Y + Px(88), ColW, TEXT("Scanlines"), Save->bScanlines, SelectedItem == 5);
		Kit.Text(TEXT("FAINT CRT LINES ON MENUS."), C1, Y + Px(140), Th.TextMuted, GEStyle::Type::LabelS);

		// glitch
		HitTestItem(6, C2, Y + Px(80), ColW, Px(130));
		Kit.Toggle(C2, Y + Px(88), ColW, TEXT("Screen glitch"), Save->bGlitch, SelectedItem == 6);
		Kit.Text(TEXT("SHORT FLASH WHEN SCREENS CHANGE."), C2, Y + Px(140), Th.TextMuted, GEStyle::Type::LabelS);
	}

	// ---------- back button ----------
	{
		const float BX = X0;
		const float BY = Row2Y + CardH + Px(28);
		const float BW = Px(220);
		const float BH = Px(54);
		const bool bHover = HitTestItem(GEStyle::NumSettings, BX, BY, BW, BH);
		Kit.Button(BX, BY, BW, BH, TEXT("BACK"), EGEButtonStyle::Secondary,
			(SelectedItem == GEStyle::NumSettings || bHover) ? EGEButtonState::Hover : EGEButtonState::Default, false);
		Kit.Text(TEXT("CLICK / D = INCREASE      RIGHT-CLICK / A = DECREASE      TAB = BACK"), X0 + GridW, BY + Px(18), Th.TextMuted, GEStyle::Type::LabelS, 1.0f);
	}
}

// =====================================================================
//                                PAUSE
// =====================================================================

void AGEHUD::DrawPause()
{
	FGEUIKit Kit(this);
	const GEStyle::FGETheme& Th = GEStyle::Active();
	const float Intro = EaseOut(ScreenAge() / 0.25f);
	DrawBox(0, 0, Canvas->ClipX, Canvas->ClipY, GEStyle::WithAlpha(Th.BgSunken, 0.7f * Intro));
	Kit.ScreenChrome(TEXT("PAUSED"));

	const TArray<int32> Items = GetPauseItems();
	const float CardW = Px(520);
	const float ItemH = Px(54);
	const float Gap = Px(10);
	const float CardH = Px(150) + Items.Num() * (ItemH + Gap) + Px(30);
	const float CardX = (Canvas->ClipX - CardW) * 0.5f;
	const float CardY = (Canvas->ClipY - CardH) * 0.5f + Px(20) * (1.0f - Intro);

	Kit.Panel(CardX, CardY, CardW, CardH, 14.0f, GECut::BR, true);
	Kit.Rect(CardX, CardY, CardW, Px(3), Th.AccentPrimary);
	Kit.GlowText(TEXT("PAUSED"), CardX + CardW * 0.5f, CardY + Px(24), Th.TextStrong, Th.AccentPrimary, GEStyle::Type::H1, 0.5f);
	Kit.Text(TEXT("TAB TO RESUME"), CardX + CardW * 0.5f, CardY + Px(92), Th.TextMuted, GEStyle::Type::LabelS, 0.5f);

	for (int32 i = 0; i < Items.Num(); i++)
	{
		const TCHAR* Label = TEXT("");
		switch (Items[i])
		{
		case PauseResume:   Label = TEXT("Resume");    break;
		case PauseRestart:  Label = TEXT("Restart");   break;
		case PauseSettings: Label = TEXT("Settings");  break;
		case PauseMainMenu: Label = TEXT("Main menu"); break;
		}
		const float X = CardX + Px(40);
		const float Y = CardY + Px(124) + i * (ItemH + Gap);
		const float W = CardW - Px(80);
		const bool bHover = HitTestItem(i, X, Y, W, ItemH);
		Kit.NavItem(X, Y, W, ItemH, i + 1, Label, SelectedItem == i ? EGENavState::Selected : (bHover ? EGENavState::Hover : EGENavState::Default));
	}

	Kit.Text(TEXT("W / S  OR MOUSE TO CHOOSE      ENTER / CLICK TO SELECT"), Canvas->ClipX * 0.5f, CardY + CardH + Px(24), Th.TextMuted, GEStyle::Type::LabelS, 0.5f);
}

// =====================================================================
//                               RESULTS
// =====================================================================

namespace
{
	/** 0 = S, 1 = A, 2 = B, 3 = C, 4 = D from four thresholds (best first). */
	int32 RankIndex(float Value, const float (&Limits)[4], bool bLowerIsBetter)
	{
		for (int32 i = 0; i < 4; i++)
		{
			if (bLowerIsBetter ? (Value <= Limits[i]) : (Value >= Limits[i]))
			{
				return i;
			}
		}
		return 4;
	}

	/** Rank thresholds per aim mode (placeholder tuning values, easy to change here). */
	int32 AimRank(EGEAimMode Mode, float Score)
	{
		static const float Gridshot[4] = { 90.0f, 75.0f, 60.0f, 45.0f };
		static const float Microshot[4] = { 60.0f, 48.0f, 36.0f, 24.0f };
		static const float Tracking[4] = { 85.0f, 70.0f, 55.0f, 40.0f };
		static const float Reflex[4] = { 220.0f, 260.0f, 300.0f, 360.0f }; // milliseconds, lower is better
		switch (Mode)
		{
		case EGEAimMode::Microshot: return RankIndex(Score, Microshot, false);
		case EGEAimMode::Tracking:  return RankIndex(Score, Tracking, false);
		case EGEAimMode::Reflex:    return RankIndex(Score, Reflex, true);
		default:                    return RankIndex(Score, Gridshot, false);
		}
	}

	int32 PercentRank(float Percent)
	{
		static const float Limits[4] = { 90.0f, 80.0f, 65.0f, 50.0f };
		return RankIndex(Percent, Limits, false);
	}
}

void AGEHUD::DrawResults()
{
	const AGETargetManager* Aim = GetAim();
	UGETrainingManager* Training = UGETrainingManager::Get(this);
	if (Training == nullptr)
	{
		return;
	}
	const bool bAim = (ResultKind == EGEResultKind::Aim);
	if (bAim && Aim == nullptr)
	{
		return;
	}
	FGEUIKit Kit(this);
	const GEStyle::FGETheme& Th = GEStyle::Active();

	// ---------- gather what to show ----------
	FString Header;
	float Score = 0.0f;
	FString Unit;
	TArray<FGEStatLine> Lines;
	EGEAimMode Mode = EGEAimMode::Gridshot;
	int32 Rank = 4;

	if (bAim)
	{
		Mode = Aim->GetMode();
		Header = GEAimModes::GetName(Mode).ToUpper() + TEXT("   //   RESULTS");
		Score = Aim->GetLastScore();
		Unit = GEAimModes::GetScoreUnit(Mode);
		Lines = Aim->GetResultLines();
		Rank = AimRank(Mode, Score);
	}
	else if (ResultKind == EGEResultKind::Training)
	{
		const FGETrainingStats& S = Training->GetStats();
		Header = TEXT("TRAINING CENTER   //   RESULTS");
		Score = (float)S.Kills;
		Unit = TEXT("kills");
		Rank = PercentRank(S.GetAccuracyPercent());
		Lines.Add({ TEXT("Accuracy"), FString::Printf(TEXT("%.0f%%  (%d / %d)"), S.GetAccuracyPercent(), S.Hits, S.Shots) });
		Lines.Add({ TEXT("Headshot kills"), FString::Printf(TEXT("%.0f%%"), S.GetHeadshotPercent()) });
		Lines.Add({ TEXT("First-shot accuracy"), FString::Printf(TEXT("%.0f%%  (%d / %d)"), S.GetFirstShotAccuracyPercent(), S.FirstShotHits, S.FirstShots) });
		Lines.Add({ TEXT("Avg time to kill"), FString::Printf(TEXT("%.2f s"), S.GetAverageTimeToKill()) });
		Lines.Add({ TEXT("Session time"), FString::Printf(TEXT("%.0f s"), S.Duration) });
	}
	else
	{
		Header = TEXT("FLICK OPTIMIZER   //   RESULTS");
		if (Training->HasFlickSummary())
		{
			const FGEFlickSummary& F = Training->GetLastFlickSummary();
			Score = (float)F.TotalHits;
			Rank = PercentRank(F.GetAccuracyPercent());
			Lines.Add({ TEXT("Targets"), FString::Printf(TEXT("%d"), F.TotalTargets) });
			Lines.Add({ TEXT("Accuracy"), FString::Printf(TEXT("%.0f%%"), F.GetAccuracyPercent()) });
			Lines.Add({ TEXT("Avg time to hit"), FString::Printf(TEXT("%.2f s"), F.AverageTimeToHit) });
			Lines.Add({ TEXT("Avg overshoot"), FString::Printf(TEXT("%+.1f%%"), F.AverageOvershootPercent) });
			Lines.Add({ TEXT("Corrections per flick"), FString::Printf(TEXT("%.1f"), F.AverageCorrections) });
			Lines.Add({ TEXT("Sensitivity"), F.bHasSuggestion
				? FString::Printf(TEXT("%s by %.0f%%"), F.SuggestedChangePercent < 0.0f ? TEXT("lower") : TEXT("raise"), FMath::Abs(F.SuggestedChangePercent))
				: FString(TEXT("keep it")) });
		}
		Unit = TEXT("targets hit");
	}

	const bool bTips = !bAim; // Training and Flick results also show the AI Coach's tips

	// ---------- card ----------
	const float Age = ScreenAge();
	const float Intro = EaseOut(Age / 0.4f);
	DrawBackdrop(0.9f, 0.75f, false);
	Kit.ScreenChrome(TEXT("RESULTS"));

	const float LeftW = Px(760);
	const float CardW = bTips ? Px(1320) : LeftW;
	const float CardH = Px(740);
	const float CardX = (Canvas->ClipX - CardW) * 0.5f;
	const float CardY = (Canvas->ClipY - CardH) * 0.5f + Px(30) * (1.0f - Intro);
	const float TextX = CardX + Px(48);
	const float ColW = LeftW - Px(96); // width of the left column

	Kit.Panel(CardX, CardY, CardW, CardH, 16.0f, GECut::BR, true);
	Kit.Rect(CardX, CardY, CardW, Px(3), Th.AccentPrimary);
	Kit.Text(Header, TextX, CardY + Px(34), Th.TextPrimary, GEStyle::Type::LabelL);

	// ---------- big score (counts up during the intro, mono) ----------
	const float ShownScore = Score * EaseOut(Age / 0.8f);
	const FString Number = bAim ? GEAimModes::FormatScoreNumber(Mode, ShownScore) : FString::FromInt(FMath::RoundToInt(ShownScore));
	const float NumberSize = 130.0f;
	const FVector2D NumberDims = Kit.Measure(Number, NumberSize, true);
	Kit.GlowText(Number, TextX, CardY + Px(66), Th.TextStrong, Th.AccentPrimary, NumberSize, 0.0f, true);
	Kit.Text(Unit.ToUpper(), TextX + NumberDims.X + Px(16), CardY + Px(66) + NumberDims.Y - Px(44), Th.TextBody, GEStyle::Type::LabelL);

	// ---------- rank letter (S / A / B / C / D) ----------
	{
		static const TCHAR* Letters[5] = { TEXT("S"), TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D") };
		const FLinearColor RankColors[5] = { Th.StatusWarning, Th.StatusSuccess, Th.StatusInfo, Th.TextBody, Th.StatusDanger };
		const float RX = TextX + ColW - Px(50);
		Kit.Text(TEXT("RANK"), RX, CardY + Px(34), Th.TextMuted, GEStyle::Type::LabelM, 0.5f);
		Kit.GlowText(Letters[Rank], RX, CardY + Px(58), RankColors[Rank], RankColors[Rank], 110.0f, 0.5f);
	}

	// ---------- personal best banner / value ----------
	const float BestY = CardY + Px(246);
	if (bAim)
	{
		if (Aim->WasNewBest())
		{
			const float Pulse = 0.5f + 0.5f * FMath::Sin(NowTime * 6.0f);
			const float BW = Px(380), BH = Px(46);
			Kit.GlowRect(TextX, BestY, BW, BH, GEStyle::WithAlpha(Th.StatusWarning, 0.25f + 0.2f * Pulse), Px(10));
			Kit.ChamferFill(TextX, BestY, BW, BH, Px(10), GECut::BR, Th.StatusWarning);
			Kit.TextCentered(TEXT("NEW PERSONAL BEST"), TextX + BW * 0.5f, BestY, BH, Th.TextOnAccent, GEStyle::Type::Title, 0.5f);
		}
		else
		{
			float Best = 0.0f;
			Kit.Text(TEXT("PERSONAL BEST"), TextX, BestY + Px(8), Th.TextMuted, GEStyle::Type::LabelM);
			Kit.Text(Aim->GetBestScore(Mode, Best) ? GEAimModes::FormatScore(Mode, Best) : FString(TEXT("-")),
				TextX + Px(190), BestY + Px(2), Th.StatusWarning, GEStyle::Type::DataL, 0.0f, true);
		}
	}
	else
	{
		Kit.Text(TEXT("SESSION SAVED TO YOUR HISTORY."), TextX, BestY + Px(10), Th.TextMuted, GEStyle::Type::LabelM);
	}

	// ---------- stat rows ----------
	const float RowH = Px(44);
	for (int32 i = 0; i < Lines.Num(); i++)
	{
		Kit.StatRow(TextX, CardY + Px(316) + i * RowH, ColW, Lines[i].Label, Lines[i].Value);
	}

	// ---------- AI Coach tips as cards (Training Center / Flick Optimizer) ----------
	if (bTips)
	{
		const float TipsX = CardX + LeftW + Px(16);
		const float TipsW = CardW - LeftW - Px(64);
		Kit.Rect(TipsX - Px(24), CardY + Px(40), Px(1), CardH - Px(80), Th.BorderFaint);
		Kit.GlowText(TEXT("AI COACH"), TipsX, CardY + Px(28), Th.TextStrong, Th.StatusInfo, GEStyle::Type::H2);
		Kit.Text(TEXT("YOUR TOP 3 TIPS"), TipsX, CardY + Px(76), Th.TextMuted, GEStyle::Type::LabelM);

		float TipY = CardY + Px(108);
		if (Training->HasCoachReport())
		{
			const FGECoachReport& Report = Training->GetCoachReport();
			const float TextW = TipsW - Px(84);
			for (int32 i = 0; i < Report.Tips.Num() && i < 3; i++)
			{
				const float CardTipH = MeasureWrapped(Report.Tips[i].Text, TextW, GEStyle::Type::BodyL) + Px(28);
				Kit.Panel(TipsX, TipY, TipsW, CardTipH, 8.0f, GECut::BR, true);
				Kit.Rect(TipsX, TipY, Px(3), CardTipH, Th.StatusSuccess);
				Kit.Text(FString::Printf(TEXT("%02d"), i + 1), TipsX + Px(16), TipY + Px(12), Th.StatusWarning, GEStyle::Type::DataL, 0.0f, true);
				DrawWrappedLabel(Report.Tips[i].Text, TipsX + Px(68), TipY + Px(14), TextW, Th.TextBody, GEStyle::Type::BodyL);
				TipY += CardTipH + Px(12);
			}
			if (Report.Trends.Num() > 0)
			{
				Kit.Text(TEXT("TRENDS"), TipsX, TipY + Px(2), Th.TextMuted, GEStyle::Type::LabelM);
				TipY += Px(30);
				for (int32 i = 0; i < Report.Trends.Num() && i < 3; i++)
				{
					TipY = DrawWrappedLabel(Report.Trends[i], TipsX, TipY, TipsW, Th.TextBody, GEStyle::Type::BodyM);
					TipY += Px(6);
				}
			}
		}
		else
		{
			DrawWrappedLabel(TEXT("No coach tips yet. Play a longer session to get tips."), TipsX, TipY, TipsW, Th.TextBody, GEStyle::Type::BodyL);
		}
	}

	// ---------- buttons side by side ----------
	const float ButtonY = CardY + CardH - Px(96);
	const float ButtonW = (ColW - Px(16)) * 0.5f;
	const float ButtonH = Px(58);
	const bool bHoverPlay = HitTestItem(0, TextX, ButtonY, ButtonW, ButtonH);
	const bool bHoverMenu = HitTestItem(1, TextX + ButtonW + Px(16), ButtonY, ButtonW, ButtonH);
	Kit.Button(TextX, ButtonY, ButtonW, ButtonH, TEXT("PLAY AGAIN"), EGEButtonStyle::Primary,
		(SelectedItem == 0 || bHoverPlay) ? EGEButtonState::Hover : EGEButtonState::Default);
	Kit.Button(TextX + ButtonW + Px(16), ButtonY, ButtonW, ButtonH, TEXT("MAIN MENU"), EGEButtonStyle::Secondary,
		(SelectedItem == 1 || bHoverMenu) ? EGEButtonState::Hover : EGEButtonState::Default);

	Kit.Text(TEXT("ENTER = SELECT        R / CLICK PLAY AGAIN        TAB = MAIN MENU"),
		Canvas->ClipX * 0.5f, CardY + CardH + Px(24), Th.TextMuted, GEStyle::Type::LabelS, 0.5f);
}

// =====================================================================
//                               WIDGETS
// =====================================================================

bool AGEHUD::HitTestItem(int32 Index, float X, float Y, float W, float H)
{
	const bool bHovered = bHasMouse && MouseX >= X && MouseX <= X + W && MouseY >= Y && MouseY <= Y + H;
	if (bHovered)
	{
		HoveredItem = Index;
		if (bMouseMoved)
		{
			SelectedItem = Index; // moving the mouse over an item selects it
		}
	}
	return bHovered;
}

float AGEHUD::MeasureWrapped(const FString& Text, float MaxWidth, float PixelSize)
{
	// same wrapping rule as DrawWrappedLabel, but nothing is drawn
	TArray<FString> Words;
	Text.ParseIntoArray(Words, TEXT(" "), true);
	const float LineH = MeasureText(TEXT("Ag"), PixelSize).Y + Px(4);
	FString Line;
	int32 NumLines = 0;
	for (const FString& Word : Words)
	{
		const FString Test = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
		if (MeasureText(Test, PixelSize).X > MaxWidth && !Line.IsEmpty())
		{
			NumLines++;
			Line = Word;
		}
		else
		{
			Line = Test;
		}
	}
	if (!Line.IsEmpty())
	{
		NumLines++;
	}
	return NumLines * LineH;
}


// =====================================================================
//                           DRAWING HELPERS
// =====================================================================

void AGEHUD::DrawBox(float X, float Y, float W, float H, FLinearColor Color)
{
	if (W <= 0.0f || H <= 0.0f || Color.A <= 0.0f)
	{
		return;
	}
	DrawRect(Color, X, Y, W, H);
}

void AGEHUD::DrawPanel(float X, float Y, float W, float H, float Alpha)
{
	// dark see-through background
	DrawBox(X, Y, W, H, GEStyle::WithAlpha(GEStyle::PanelFill, GEStyle::PanelFill.A * Alpha));

	// thin light border
	const FLinearColor Border = GEStyle::WithAlpha(GEStyle::PanelBorder, GEStyle::PanelBorder.A * Alpha);
	DrawBox(X, Y, W, Px(1), Border);
	DrawBox(X, Y + H - Px(1), W, Px(1), Border);
	DrawBox(X, Y, Px(1), H, Border);
	DrawBox(X + W - Px(1), Y, Px(1), H, Border);
}

void AGEHUD::DrawGradient(float X, float Y, float W, float H, FLinearColor From, FLinearColor To, bool bHorizontal)
{
	// a gradient made of many thin strips
	const int32 Steps = 48;
	for (int32 i = 0; i < Steps; i++)
	{
		const float T = (i + 0.5f) / Steps;
		const FLinearColor Color = FMath::Lerp(From, To, T);
		if (bHorizontal)
		{
			DrawBox(X + W * i / Steps, Y, W * (i + 1) / Steps - W * i / Steps, H, Color);
		}
		else
		{
			DrawBox(X, Y + H * i / Steps, W, H * (i + 1) / Steps - H * i / Steps, Color);
		}
	}
}

void AGEHUD::EnsureFonts()
{
	if (bFontsLoaded)
	{
		return;
	}
	bFontsLoaded = true;

	// UI voice: Rajdhani SemiBold. Data: Share Tech Mono. Both are runtime fonts made from the
	// FontFace assets in /Game/UI/Fonts. If an asset is missing we keep Roboto / DroidSansMono.
	UFont* Rajdhani = GEStyle::MakeRuntimeFont(this, GEStyle::FontPathSemiBold);
	UFont* ShareTech = GEStyle::MakeRuntimeFont(this, GEStyle::FontPathMono);
	if (Rajdhani)
	{
		UIFont = Rajdhani;
	}
	if (ShareTech)
	{
		MonoFont = ShareTech;
	}
	UE_LOG(LogTemp, Log, TEXT("GE UI fonts: UI = %s, numbers = %s"),
		Rajdhani ? TEXT("Rajdhani SemiBold (imported)") : TEXT("FALLBACK Roboto"),
		ShareTech ? TEXT("Share Tech Mono (imported)") : TEXT("FALLBACK DroidSansMono"));
}

UFont* AGEHUD::GetFont(bool bMono)
{
	EnsureFonts();
	if (bMono && MonoFont)
	{
		return MonoFont;
	}
	if (!bMono && UIFont)
	{
		return UIFont;
	}
	return GEngine ? GEngine->GetLargeFont() : nullptr;
}

UFont* AGEHUD::GetSizedFont(bool bMono, float PixelSize, float& OutScale)
{
	// WHY: Unreal's HUD draws a font at its built-in "LegacyFontSize" and then STRETCHES the letters
	// to the size we ask for. Stretching 24px letters to 120px makes them blurry.
	// FIX: make a copy of the font whose built-in size IS the size we need and draw it at scale ~1.
	UFont* BaseFont = GetFont(bMono);
	const float TargetSize = PixelSize * UIScale; // real size on this screen
	if (BaseFont == nullptr)
	{
		OutScale = 1.0f;
		return nullptr;
	}

	// round to "buckets" so animated text (the countdown) does not create a new font every frame
	const int32 Step = (TargetSize > 48.0f) ? 4 : 1;
	const int32 Bucket = FMath::Clamp(FMath::RoundToInt(TargetSize / Step) * Step, 4, 1000);

	// one cached copy per size (and per regular / mono font)
	const int32 Key = Bucket * 2 + (bMono ? 1 : 0);
	UFont* SizedFont = nullptr;
	if (const TObjectPtr<UFont>* Found = SizedFonts.Find(Key))
	{
		SizedFont = *Found;
	}
	else
	{
		SizedFont = DuplicateObject<UFont>(BaseFont, this);
		SizedFont->LegacyFontSize = Bucket;
		SizedFonts.Add(Key, SizedFont);
	}

	OutScale = TargetSize / Bucket; // tiny leftover scale (a few %) from the rounding
	return SizedFont;
}

FVector2D AGEHUD::MeasureText(const FString& Text, float PixelSize, bool bMono)
{
	float Scale = 1.0f;
	UFont* Font = GetSizedFont(bMono, PixelSize, Scale);
	float Width = 0.0f;
	float Height = 0.0f;
	GetTextSize(Text, Width, Height, Font, Scale);
	return FVector2D(Width, Height);
}

void AGEHUD::DrawLabel(const FString& Text, float X, float Y, FLinearColor Color, float PixelSize,
	float AlignX, bool bMono, bool bShadow)
{
	if (Text.IsEmpty() || Color.A <= 0.0f)
	{
		return;
	}
	float Scale = 1.0f;
	UFont* Font = GetSizedFont(bMono, PixelSize, Scale);

	float Width = 0.0f;
	float Height = 0.0f;
	GetTextSize(Text, Width, Height, Font, Scale);

	// snap to whole pixels: text between pixels looks soft
	const float DrawX = FMath::RoundToFloat(X - Width * AlignX);
	Y = FMath::RoundToFloat(Y);

	// soft drop shadow for readability
	if (bShadow)
	{
		DrawText(Text, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f * Color.A), DrawX + Px(2), Y + Px(2), Font, Scale);
	}
	DrawText(Text, Color, DrawX, Y, Font, Scale);
}

void AGEHUD::DrawGlowLabel(const FString& Text, float X, float Y, FLinearColor Color, FLinearColor GlowColor,
	float PixelSize, float AlignX, bool bMono)
{
	if (Text.IsEmpty() || Color.A <= 0.0f)
	{
		return;
	}
	float Scale = 1.0f;
	UFont* Font = GetSizedFont(bMono, PixelSize, Scale);

	float Width = 0.0f;
	float Height = 0.0f;
	GetTextSize(Text, Width, Height, Font, Scale);

	const float DrawX = FMath::RoundToFloat(X - Width * AlignX);
	Y = FMath::RoundToFloat(Y);

	// glow: the text drawn faintly around itself, in two tight rings of 8
	const FLinearColor Glow = GEStyle::WithAlpha(GlowColor, 0.09f * Color.A);
	for (int32 Ring = 1; Ring <= 2; Ring++)
	{
		const float Radius = Px(2.0f * Ring);
		for (int32 i = 0; i < 8; i++)
		{
			const float Angle = i * PI / 4.0f;
			DrawText(Text, Glow, DrawX + FMath::Cos(Angle) * Radius, Y + FMath::Sin(Angle) * Radius, Font, Scale);
		}
	}
	DrawText(Text, Color, DrawX, Y, Font, Scale);
}

float AGEHUD::DrawWrappedLabel(const FString& Text, float X, float Y, float MaxWidth, FLinearColor Color, float PixelSize)
{
	TArray<FString> Words;
	Text.ParseIntoArray(Words, TEXT(" "), true);
	const float LineH = MeasureText(TEXT("Ag"), PixelSize).Y + Px(4);

	FString Line;
	for (const FString& Word : Words)
	{
		const FString Test = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
		if (MeasureText(Test, PixelSize).X > MaxWidth && !Line.IsEmpty())
		{
			DrawLabel(Line, X, Y, Color, PixelSize);
			Y += LineH;
			Line = Word;
		}
		else
		{
			Line = Test;
		}
	}
	if (!Line.IsEmpty())
	{
		DrawLabel(Line, X, Y, Color, PixelSize);
		Y += LineH;
	}
	return Y;
}
