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

void AGEHUD::DrawPlaying()
{
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

	const FLinearColor ModeColor = GEAimModes::GetColor(Aim->GetMode());

	// ---------- timer pill (top center) ----------
	{
		const float W = Px(240);
		const float H = Px(96);
		const float X = CenterX - W * 0.5f;
		const float Y = Px(28);
		DrawPanel(X, Y, W, H, 1.0f);

		// last 5 seconds of a timed mode (not during the countdown): the timer pulses red
		FLinearColor TimerColor = GEStyle::TextWhite;
		if (Aim->IsLowOnTime() && !Aim->IsCountingDown())
		{
			const float Pulse = 0.5f + 0.5f * FMath::Sin(NowTime * 10.0f);
			TimerColor = FMath::Lerp(GEStyle::TextWhite, GEStyle::Red, Pulse);
		}
		DrawLabel(Aim->GetTimerText(), CenterX, Y + Px(14), TimerColor, 46, 0.5f, true);

		// progress bar
		const float BarW = W - Px(40);
		const float BarY = Y + H - Px(18);
		DrawBox(X + Px(20), BarY, BarW, Px(4), FLinearColor(1, 1, 1, 0.1f));
		DrawBox(X + Px(20), BarY, BarW * Aim->GetTimerFraction(), Px(4), ModeColor);
	}

	// ---------- stats card (top left) ----------
	{
		const TArray<FGEStatLine> Stats = Aim->GetLiveStats();
		const float X = Px(36);
		const float Y = Px(28);
		const float W = Px(300);
		const float RowH = Px(40);
		const float H = Px(62) + Stats.Num() * RowH;
		DrawPanel(X, Y, W, H, 1.0f);
		DrawBox(X, Y, Px(4), H, ModeColor); // accent stripe

		DrawLabel(GEAimModes::GetName(Aim->GetMode()).ToUpper(), X + Px(24), Y + Px(16), ModeColor, 20);
		for (int32 i = 0; i < Stats.Num(); i++)
		{
			const float RowY = Y + Px(54) + i * RowH;
			DrawLabel(Stats[i].Label, X + Px(24), RowY + Px(4), GEStyle::Active().TextBody, 20);
			DrawLabel(Stats[i].Value, X + W - Px(20), RowY, GEStyle::TextWhite, 26, 1.0f, true);
		}
	}

	// ---------- hints (bottom left) ----------
	DrawLabel(TEXT("R  restart        TAB  pause"), Px(40), Canvas->ClipY - Px(56),
		GEStyle::WithAlpha(GEStyle::Active().TextBody, 0.8f), 17);

	// ---------- message under the crosshair (Reflex) ----------
	const FString Message = Aim->GetCenterMessage();
	if (!Message.IsEmpty())
	{
		const bool bEarly = Message.StartsWith(TEXT("Too"));
		const float Pulse = 0.65f + 0.35f * FMath::Sin(NowTime * 6.0f);
		const FLinearColor Color = bEarly ? GEStyle::Red : GEStyle::WithAlpha(GEStyle::TextWhite, Pulse);
		DrawLabel(Message, CenterX, CenterY + Px(70), Color, 30, 0.5f);
	}

	DrawAimPopups();
	DrawCountdown(ModeColor);
	DrawAimHitMarker();
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
	// white X that fades out over 0.15 s after a hit
	const float Age = NowTime - Aim->LastHitTime;
	if (Age >= 0.15f)
	{
		return;
	}
	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;
	const float Inner = Px(9);
	const float Outer = Px(19);
	const FLinearColor Color = GEStyle::WithAlpha(GEStyle::TextWhite, 1.0f - Age / 0.15f);

	DrawLine(CenterX - Outer, CenterY - Outer, CenterX - Inner, CenterY - Inner, Color, 2.0f);
	DrawLine(CenterX + Inner, CenterY + Inner, CenterX + Outer, CenterY + Outer, Color, 2.0f);
	DrawLine(CenterX + Outer, CenterY - Outer, CenterX + Inner, CenterY - Inner, Color, 2.0f);
	DrawLine(CenterX - Inner, CenterY + Inner, CenterX - Outer, CenterY + Outer, Color, 2.0f);
}

void AGEHUD::DrawCountdown(const FLinearColor& ModeColor)
{
	const AGETargetManager* Aim = GetAim();
	if (Aim == nullptr)
	{
		return;
	}
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

		DrawLabel(TEXT("GET READY"), CenterX, CenterY - Px(250), GEStyle::WithAlpha(GEStyle::Active().TextBody, 0.9f), 24, 0.5f);

		const FString Text = FString::FromInt(Number);
		const FVector2D TextSize = MeasureText(Text, Size, true);
		DrawGlowLabel(Text, CenterX, CenterY - Px(150) - TextSize.Y * 0.5f,
			GEStyle::WithAlpha(GEStyle::TextWhite, Alpha), GEStyle::WithAlpha(ModeColor, Alpha), Size, 0.5f, true);
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
			GEStyle::WithAlpha(GEStyle::TextWhite, Alpha), GEStyle::WithAlpha(GEStyle::Green, Alpha), Size, 0.5f);
	}
}

void AGEHUD::DrawAimPopups()
{
	const AGETargetManager* Aim = GetAim();
	if (Aim == nullptr)
	{
		return;
	}
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
		const FLinearColor Color = GEStyle::WithAlpha(FMath::Lerp(Popup.Color, GEStyle::TextWhite, 0.4f), 1.0f - Life);
		DrawLabel(Popup.Text, ScreenPos.X, ScreenPos.Y, Color, 24, 0.5f, true);
	}
}

// =====================================================================
//                       MAIN MENU / MODE SELECT
// =====================================================================

void AGEHUD::DrawBackdrop(float Alpha1, float Alpha2, bool bHorizontal)
{
	DrawGradient(0, 0, Canvas->ClipX, Canvas->ClipY,
		FLinearColor(0.008f, 0.01f, 0.02f, Alpha1), FLinearColor(0.008f, 0.01f, 0.02f, Alpha2), bHorizontal);
}

void AGEHUD::DrawMainMenu()
{
	const float Age = ScreenAge();
	const float Intro = EaseOut(Age / 0.4f);
	const float Left = Px(120);

	// dark on the left (behind the buttons), see-through on the right (the showcase targets)
	DrawBackdrop(0.93f, 0.3f, true);

	// ---------- title ----------
	DrawGlowLabel(TEXT("RAYSHOT"), Left - Px(30) * (1.0f - Intro), Px(110),
		GEStyle::WithAlpha(GEStyle::TextWhite, Intro), GEStyle::WithAlpha(GEStyle::Active().AccentPrimary, Intro), 120);
	DrawBox(Left, Px(262), Px(90) * Intro, Px(5), GEStyle::Active().AccentPrimary);
	DrawLabel(TEXT("AIM TRAINER   //   HIT DETECTION BY MY OWN RAY TRACER"), Left, Px(284),
		GEStyle::WithAlpha(GEStyle::Active().TextBody, Intro), 19);

	// ---------- buttons ----------
	struct FItem { const TCHAR* Label; const TCHAR* Right; FLinearColor Color; };
	const FItem Items[NumMenuItems] = {
		{ TEXT("AIM TRAINER"),     TEXT("4 MODES"), GEStyle::Cyan },
		{ TEXT("TRAINING CENTER"), TEXT(""),        GEStyle::Green },
		{ TEXT("FLICK OPTIMIZER"), TEXT(""),        GEStyle::Magenta },
		{ TEXT("FREE ROAM"),       TEXT(""),        GEStyle::Gold },
		{ TEXT("SETTINGS"),        TEXT(""),        GEStyle::Active().TextBody },
		{ TEXT("QUIT"),            TEXT(""),        GEStyle::Red },
	};
	const float ButtonW = Px(600);
	const float ButtonH = Px(62);
	const float Gap = Px(12);
	float Y = Px(360);
	for (int32 i = 0; i < NumMenuItems; i++)
	{
		if (i == MenuSettings)
		{
			Y += Px(18); // little gap before Settings / Quit
		}
		const float ButtonIntro = EaseOut((Age - 0.05f * i) / 0.3f); // buttons slide in one after another
		const float X = Left - Px(40) * (1.0f - ButtonIntro);
		DrawMenuButton(i, X, Y, ButtonW, ButtonH, Items[i].Label, Items[i].Right, Items[i].Color, ButtonIntro);
		Y += ButtonH + Gap;
	}

	DrawInfoCard(SelectedItem);

	// ---------- footer ----------
	DrawLabel(TEXT("W / S  or mouse to choose      ENTER / click to select      TAB = back / pause"), Left, Canvas->ClipY - Px(64),
		GEStyle::Active().TextBody, 17);
	DrawLabel(TEXT("GameEngine"), Canvas->ClipX - Px(120), Canvas->ClipY - Px(64),
		GEStyle::WithAlpha(GEStyle::Active().TextBody, 0.6f), 17, 1.0f);
}

void AGEHUD::DrawInfoCard(int32 Selected)
{
	const float Intro = EaseOut(ScreenAge() / 0.4f);
	const float CardW = Px(520);
	const float CardX = Canvas->ClipX - Px(120) - CardW;
	const float CardY = Px(360);
	const float TextX = CardX + Px(40);

	// what to tell the player about the highlighted entry
	FString Title;
	TArray<FString> Lines;
	FLinearColor Accent = GEStyle::Cyan;
	switch (Selected)
	{
	case MenuAim:
		Title = TEXT("AIM TRAINER");
		Lines = { TEXT("Gridshot, Microshot, Tracking, Reflex."), TEXT("Timed rounds with personal bests."), TEXT("Trains: aim, speed, reaction") };
		Accent = GEStyle::Cyan;
		break;
	case MenuTraining:
		Title = TEXT("TRAINING CENTER");
		Lines = { TEXT("Fight bots with your 5 weapons."), TEXT("Static, strafing and peeking targets."), TEXT("The AI Coach gives you 3 tips") };
		Accent = GEStyle::Green;
		break;
	case MenuFlick:
		Title = TEXT("FLICK OPTIMIZER");
		Lines = { TEXT("Flick to targets in every direction."), TEXT("Measures overshoot and speed."), TEXT("Suggests a better sensitivity") };
		Accent = GEStyle::Magenta;
		break;
	case MenuFreeRoam:
		Title = TEXT("FREE ROAM");
		Lines = { TEXT("Walk around the room."), TEXT("All 5 weapons, shoot the dummies."), TEXT("T = Training Center, M = its menu") };
		Accent = GEStyle::Gold;
		break;
	case MenuSettings:
		Title = TEXT("SETTINGS");
		Lines = { TEXT("Sensitivity, field of view, volume"), TEXT("and crosshair color.") };
		Accent = GEStyle::Active().TextBody;
		break;
	default:
		Title = TEXT("QUIT");
		Lines = { TEXT("See you next session.") };
		Accent = GEStyle::Red;
		break;
	}

	const float CardH = Px(260);
	DrawPanel(CardX, CardY, CardW, CardH, Intro);
	DrawBox(CardX, CardY, CardW, Px(5), GEStyle::WithAlpha(Accent, Intro));
	DrawGlowLabel(Title, TextX, CardY + Px(36), GEStyle::TextWhite, Accent, 40);
	for (int32 i = 0; i < Lines.Num(); i++)
	{
		DrawLabel(Lines[i], TextX, CardY + Px(110) + i * Px(34), (i == Lines.Num() - 1 && Lines.Num() == 3) ? Accent : GEStyle::TextWhite, 22);
	}
	const float Pulse = 0.6f + 0.4f * FMath::Sin(NowTime * 4.0f);
	DrawLabel(TEXT("PRESS ENTER OR CLICK"), TextX, CardY + CardH - Px(48), GEStyle::WithAlpha(Accent, Pulse), 18);
}

void AGEHUD::DrawModeSelect()
{
	const AGETargetManager* Aim = GetAim();
	const float Age = ScreenAge();
	const float Intro = EaseOut(Age / 0.4f);
	const float Left = Px(120);

	DrawBackdrop(0.93f, 0.3f, true);

	DrawGlowLabel(TEXT("AIM TRAINER"), Left - Px(30) * (1.0f - Intro), Px(110),
		GEStyle::WithAlpha(GEStyle::TextWhite, Intro), GEStyle::WithAlpha(GEStyle::Active().AccentPrimary, Intro), 100);
	DrawBox(Left, Px(240), Px(90) * Intro, Px(5), GEStyle::Active().AccentPrimary);
	DrawLabel(TEXT("CHOOSE A MODE"), Left, Px(262), GEStyle::WithAlpha(GEStyle::Active().TextBody, Intro), 19);

	// ---------- 4 mode buttons + Back ----------
	const float ButtonW = Px(600);
	const float ButtonH = Px(62);
	const float Gap = Px(12);
	float Y = Px(330);
	for (int32 i = 0; i <= GEAimModes::NumModes; i++)
	{
		const float ButtonIntro = EaseOut((Age - 0.05f * i) / 0.3f);
		const float X = Left - Px(40) * (1.0f - ButtonIntro);
		if (i < GEAimModes::NumModes)
		{
			const EGEAimMode Mode = (EGEAimMode)i;
			float Best = 0.0f;
			const FString BestText = (Aim && Aim->GetBestScore(Mode, Best)) ? TEXT("BEST  ") + GEAimModes::FormatScore(Mode, Best) : FString(TEXT("NEW"));
			DrawMenuButton(i, X, Y, ButtonW, ButtonH, GEAimModes::GetName(Mode).ToUpper(), BestText, GEAimModes::GetColor(Mode), ButtonIntro);
		}
		else
		{
			Y += Px(18);
			DrawMenuButton(i, X, Y, ButtonW, ButtonH, TEXT("BACK"), TEXT(""), GEStyle::Active().TextBody, ButtonIntro);
		}
		Y += ButtonH + Gap;
	}

	// ---------- info card for the highlighted mode ----------
	const float CardW = Px(520);
	const float CardX = Canvas->ClipX - Px(120) - CardW;
	const float CardY = Px(330);
	if (SelectedItem < GEAimModes::NumModes)
	{
		const EGEAimMode Mode = (EGEAimMode)SelectedItem;
		const FLinearColor ModeColor = GEAimModes::GetColor(Mode);
		const float CardH = Px(420);
		const float TextX = CardX + Px(40);

		DrawPanel(CardX, CardY, CardW, CardH, Intro);
		DrawBox(CardX, CardY, CardW, Px(5), GEStyle::WithAlpha(ModeColor, Intro));
		DrawGlowLabel(GEAimModes::GetName(Mode).ToUpper(), TextX, CardY + Px(36), GEStyle::TextWhite, ModeColor, 46);

		const TArray<FString> Lines = GEAimModes::GetDescription(Mode);
		for (int32 i = 0; i < Lines.Num(); i++)
		{
			const bool bLast = (i == Lines.Num() - 1);
			DrawLabel(Lines[i], TextX, CardY + Px(112) + i * Px(34), bLast ? ModeColor : GEStyle::TextWhite, 22);
		}
		DrawBox(TextX, CardY + Px(240), CardW - Px(80), Px(1), GEStyle::PanelBorder);

		DrawLabel(TEXT("DURATION"), TextX, CardY + Px(266), GEStyle::Active().TextBody, 16);
		DrawLabel(GEAimModes::GetDurationText(Mode), TextX, CardY + Px(292), GEStyle::TextWhite, 28);

		DrawLabel(TEXT("PERSONAL BEST"), TextX + Px(230), CardY + Px(266), GEStyle::Active().TextBody, 16);
		float Best = 0.0f;
		if (Aim && Aim->GetBestScore(Mode, Best))
		{
			DrawLabel(GEAimModes::FormatScore(Mode, Best), TextX + Px(230), CardY + Px(292), GEStyle::Gold, 28, 0.0f, true);
		}
		else
		{
			DrawLabel(TEXT("not played yet"), TextX + Px(230), CardY + Px(296), GEStyle::Active().TextBody, 22);
		}

		const float Pulse = 0.6f + 0.4f * FMath::Sin(NowTime * 4.0f);
		DrawLabel(TEXT("PRESS ENTER OR CLICK TO PLAY"), TextX, CardY + CardH - Px(54), GEStyle::WithAlpha(ModeColor, Pulse), 18);
	}

	DrawLabel(TEXT("W / S  or mouse to choose      ENTER / click to play      TAB = back"), Left, Canvas->ClipY - Px(64),
		GEStyle::Active().TextBody, 17);
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

	const float Intro = EaseOut(ScreenAge() / 0.35f);
	DrawBackdrop(0.9f, 0.7f, false);

	// ---------- card ----------
	const float CardW = Px(780);
	const float RowH = Px(86);
	const float CardH = Px(150) + GEStyle::NumSettings * RowH + Px(110);
	const float CardX = (Canvas->ClipX - CardW) * 0.5f;
	const float CardY = (Canvas->ClipY - CardH) * 0.5f + Px(30) * (1.0f - Intro);

	DrawPanel(CardX, CardY, CardW, CardH, Intro);
	DrawBox(CardX, CardY, CardW, Px(5), GEStyle::Active().AccentPrimary);
	DrawGlowLabel(TEXT("SETTINGS"), CardX + Px(48), CardY + Px(36), GEStyle::TextWhite, GEStyle::Active().AccentPrimary, 50);

	// ---------- rows ----------
	const float RowX = CardX + Px(32);
	const float RowW = CardW - Px(64);
	for (int32 i = 0; i < GEStyle::NumSettings; i++)
	{
		const float RowY = CardY + Px(130) + i * RowH;

		FString Label;
		FString Value;
		float Fraction = -1.0f; // -1 = no slider
		switch (i)
		{
		case 0:
		{
			const float Sens = Training->GetSensitivityScale();
			Label = TEXT("Sensitivity");
			Value = FString::Printf(TEXT("%.1f"), Sens);
			Fraction = (Sens - GESensitivity::MinScale) / (GESensitivity::MaxScale - GESensitivity::MinScale);
			break;
		}
		case 1:
			Label = TEXT("Field of view");
			Value = FString::Printf(TEXT("%.0f"), Save->FieldOfView);
			Fraction = (Save->FieldOfView - MinFov) / (MaxFov - MinFov);
			break;
		case 2:
			Label = TEXT("Volume");
			Value = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Save->Volume * 100.0f));
			Fraction = Save->Volume;
			break;
		case 3:
			Label = TEXT("Crosshair color");
			Value = GEStyle::GetCrosshairColorName(Save->CrosshairColor);
			break;
		case 4:
			Label = TEXT("UI theme");
			Value = GEStyle::GetThemeName(Save->UITheme);
			break;
		case 5:
			Label = TEXT("Scanlines");
			Value = Save->bScanlines ? TEXT("On") : TEXT("Off");
			break;
		default:
			Label = TEXT("Screen glitch");
			Value = Save->bGlitch ? TEXT("On") : TEXT("Off");
			break;
		}

		// the row itself is a button (click = increase, right-click = decrease)
		DrawMenuButton(i, RowX, RowY, RowW, RowH - Px(14), Label, TEXT("<   ") + Value + TEXT("   >"), GEStyle::Active().AccentPrimary, Intro);

		if (Fraction >= 0.0f)
		{
			DrawSlider(RowX + Px(28), RowY + RowH - Px(28), RowW - Px(56), Fraction, GEStyle::Active().AccentPrimary, Intro);
		}
	}

	// ---------- back button ----------
	const float BackY = CardY + Px(130) + GEStyle::NumSettings * RowH + Px(12);
	DrawMenuButton(GEStyle::NumSettings, RowX, BackY, Px(220), Px(60), TEXT("BACK"), TEXT(""), GEStyle::Active().TextBody, Intro);

	// ---------- live crosshair preview (beside the card) ----------
	{
		const float PW = Px(240);
		const float PX = CardX + CardW + Px(30);
		const float PY = CardY;
		DrawPanel(PX, PY, PW, PW, Intro);
		DrawLabel(TEXT("PREVIEW"), PX + PW * 0.5f, PY + Px(14), GEStyle::Active().TextBody, 16, 0.5f);
		DrawCrosshairAt(PX + PW * 0.5f, PY + PW * 0.5f + Px(8), GEStyle::GetCrosshairColor(Save->CrosshairColor), 0.0f);
	}

	DrawLabel(TEXT("Click / D = increase        Right-click / A = decrease        TAB = back"),
		Canvas->ClipX * 0.5f, CardY + CardH + Px(28), GEStyle::Active().TextBody, 17, 0.5f);
}

// =====================================================================
//                                PAUSE
// =====================================================================

void AGEHUD::DrawPause()
{
	const float Intro = EaseOut(ScreenAge() / 0.25f);
	DrawBox(0, 0, Canvas->ClipX, Canvas->ClipY, FLinearColor(0.008f, 0.01f, 0.02f, 0.6f * Intro));

	const TArray<int32> Items = GetPauseItems();
	const float CardW = Px(520);
	const float ButtonH = Px(62);
	const float Gap = Px(12);
	const float CardH = Px(130) + Items.Num() * (ButtonH + Gap) + Px(40);
	const float CardX = (Canvas->ClipX - CardW) * 0.5f;
	const float CardY = (Canvas->ClipY - CardH) * 0.5f;

	DrawPanel(CardX, CardY, CardW, CardH, Intro);
	DrawBox(CardX, CardY, CardW, Px(5), GEStyle::Active().AccentPrimary);
	DrawGlowLabel(TEXT("PAUSED"), CardX + CardW * 0.5f, CardY + Px(30), GEStyle::TextWhite, GEStyle::Active().AccentPrimary, 54, 0.5f);

	for (int32 i = 0; i < Items.Num(); i++)
	{
		const TCHAR* Label = TEXT("");
		FLinearColor Accent = GEStyle::Active().AccentPrimary;
		switch (Items[i])
		{
		case PauseResume:   Label = TEXT("RESUME");    Accent = GEStyle::Green; break;
		case PauseRestart:  Label = TEXT("RESTART");   Accent = GEStyle::Active().AccentPrimary; break;
		case PauseSettings: Label = TEXT("SETTINGS");  Accent = GEStyle::Active().TextBody; break;
		case PauseMainMenu: Label = TEXT("MAIN MENU"); Accent = GEStyle::Red; break;
		}
		DrawMenuButton(i, CardX + Px(40), CardY + Px(120) + i * (ButtonH + Gap), CardW - Px(80), ButtonH, Label, TEXT(""), Accent, Intro);
	}

	DrawLabel(TEXT("W / S  or mouse to choose      ENTER / click to select      TAB = resume"),
		Canvas->ClipX * 0.5f, CardY + CardH + Px(28), GEStyle::Active().TextBody, 17, 0.5f);
}

// =====================================================================
//                               RESULTS
// =====================================================================

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

	// ---------- gather what to show ----------
	FLinearColor Accent = GEStyle::Active().AccentPrimary;
	FString Header;
	float Score = 0.0f;
	FString Unit;
	TArray<FGEStatLine> Lines;
	EGEAimMode Mode = EGEAimMode::Gridshot;

	if (bAim)
	{
		Mode = Aim->GetMode();
		Accent = GEAimModes::GetColor(Mode);
		Header = GEAimModes::GetName(Mode).ToUpper() + TEXT("   //   RESULTS");
		Score = Aim->GetLastScore();
		Unit = GEAimModes::GetScoreUnit(Mode);
		Lines = Aim->GetResultLines();
	}
	else if (ResultKind == EGEResultKind::Training)
	{
		const FGETrainingStats& S = Training->GetStats();
		Accent = GEStyle::Green;
		Header = TEXT("TRAINING CENTER   //   RESULTS");
		Score = (float)S.Kills;
		Unit = TEXT("kills");
		Lines.Add({ TEXT("Accuracy"), FString::Printf(TEXT("%.0f%%  (%d / %d)"), S.GetAccuracyPercent(), S.Hits, S.Shots) });
		Lines.Add({ TEXT("Headshot kills"), FString::Printf(TEXT("%.0f%%"), S.GetHeadshotPercent()) });
		Lines.Add({ TEXT("First-shot accuracy"), FString::Printf(TEXT("%.0f%%  (%d / %d)"), S.GetFirstShotAccuracyPercent(), S.FirstShotHits, S.FirstShots) });
		Lines.Add({ TEXT("Avg time to kill"), FString::Printf(TEXT("%.2f s"), S.GetAverageTimeToKill()) });
		Lines.Add({ TEXT("Session time"), FString::Printf(TEXT("%.0f s"), S.Duration) });
	}
	else
	{
		Accent = GEStyle::Magenta;
		Header = TEXT("FLICK OPTIMIZER   //   RESULTS");
		if (Training->HasFlickSummary())
		{
			const FGEFlickSummary& F = Training->GetLastFlickSummary();
			Score = (float)F.TotalHits;
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
	DrawBackdrop(0.9f, 0.65f, false);

	const float LeftW = Px(760);
	const float CardW = bTips ? Px(1320) : LeftW;
	const float CardH = Px(720);
	const float CardX = (Canvas->ClipX - CardW) * 0.5f;
	const float CardY = (Canvas->ClipY - CardH) * 0.5f + Px(40) * (1.0f - Intro);
	const float TextX = CardX + Px(48);
	const float ColW = LeftW - Px(96); // width of the left column

	DrawPanel(CardX, CardY, CardW, CardH, Intro);
	DrawBox(CardX, CardY, CardW, Px(6), Accent);
	DrawLabel(Header, TextX, CardY + Px(38), Accent, 22);

	// ---------- big score (counts up during the intro) ----------
	const float ShownScore = Score * EaseOut(Age / 0.8f);
	const FString Number = bAim ? GEAimModes::FormatScoreNumber(Mode, ShownScore) : FString::FromInt(FMath::RoundToInt(ShownScore));
	const FVector2D NumberSize = MeasureText(Number, 120, true);
	DrawGlowLabel(Number, TextX, CardY + Px(80), GEStyle::TextWhite, Accent, 120, 0.0f, true);
	DrawLabel(Unit, TextX + NumberSize.X + Px(18), CardY + Px(80) + NumberSize.Y - Px(58), GEStyle::Active().TextBody, 28);

	// ---------- personal best (aim modes) ----------
	const float BestY = CardY + Px(250);
	if (bAim)
	{
		if (Aim->WasNewBest())
		{
			const float Pulse = 0.8f + 0.2f * FMath::Sin(NowTime * 6.0f);
			DrawBox(TextX, BestY, Px(340), Px(48), GEStyle::WithAlpha(GEStyle::Gold, Pulse));
			DrawLabel(TEXT("NEW PERSONAL BEST!"), TextX + Px(170), BestY + Px(11), FLinearColor(0.06f, 0.04f, 0.0f), 22, 0.5f, false, false);
		}
		else
		{
			float Best = 0.0f;
			DrawLabel(TEXT("PERSONAL BEST"), TextX, BestY + Px(4), GEStyle::Active().TextBody, 16);
			DrawLabel(Aim->GetBestScore(Mode, Best) ? GEAimModes::FormatScore(Mode, Best) : FString(TEXT("-")),
				TextX + Px(180), BestY, GEStyle::Gold, 26, 0.0f, true);
		}
	}
	else
	{
		DrawLabel(TEXT("Session saved to your history."), TextX, BestY + Px(4), GEStyle::Active().TextBody, 18);
	}

	// ---------- stats rows ----------
	const float RowH = Px(46);
	for (int32 i = 0; i < Lines.Num(); i++)
	{
		const float RowY = CardY + Px(320) + i * RowH;
		DrawLabel(Lines[i].Label, TextX, RowY + Px(4), GEStyle::Active().TextBody, 22);
		DrawLabel(Lines[i].Value, TextX + ColW, RowY, GEStyle::TextWhite, 26, 1.0f, true);
		DrawBox(TextX, RowY + RowH - Px(8), ColW, Px(1), GEStyle::PanelBorder);
	}

	// ---------- AI Coach tips (Training Center / Flick Optimizer) ----------
	if (bTips)
	{
		const float TipsX = CardX + LeftW + Px(16);
		const float TipsW = CardW - LeftW - Px(64);
		DrawBox(TipsX - Px(24), CardY + Px(40), Px(1), CardH - Px(80), GEStyle::PanelBorder);
		DrawGlowLabel(TEXT("AI COACH"), TipsX, CardY + Px(32), GEStyle::TextWhite, GEStyle::Green, 40);
		DrawLabel(TEXT("YOUR TOP 3 TIPS"), TipsX, CardY + Px(100), GEStyle::Active().TextBody, 16);

		float TipY = CardY + Px(134);
		if (Training->HasCoachReport())
		{
			const FGECoachReport& Report = Training->GetCoachReport();
			for (int32 i = 0; i < Report.Tips.Num() && i < 3; i++)
			{
				DrawLabel(FString::FromInt(i + 1), TipsX, TipY, GEStyle::Green, 28, 0.0f, true);
				TipY = DrawWrappedLabel(Report.Tips[i].Text, TipsX + Px(36), TipY + Px(2), TipsW - Px(36), GEStyle::TextWhite, 21);
				TipY += Px(22);
			}
			if (Report.Trends.Num() > 0)
			{
				DrawLabel(TEXT("TRENDS"), TipsX, TipY + Px(4), GEStyle::Active().TextBody, 16);
				TipY += Px(34);
				for (int32 i = 0; i < Report.Trends.Num() && i < 3; i++)
				{
					TipY = DrawWrappedLabel(Report.Trends[i], TipsX, TipY, TipsW, GEStyle::Active().TextBody, 18);
					TipY += Px(8);
				}
			}
		}
		else
		{
			DrawWrappedLabel(TEXT("No coach tips yet. Play a longer session to get tips."), TipsX, TipY, TipsW, GEStyle::Active().TextBody, 21);
		}
	}

	// ---------- buttons side by side ----------
	const float ButtonY = CardY + CardH - Px(100);
	const float ButtonW = (ColW - Px(16)) * 0.5f;
	DrawMenuButton(0, TextX, ButtonY, ButtonW, Px(62), TEXT("PLAY AGAIN"), TEXT(""), Accent, Intro);
	DrawMenuButton(1, TextX + ButtonW + Px(16), ButtonY, ButtonW, Px(62), TEXT("MAIN MENU"), TEXT(""), GEStyle::Active().TextBody, Intro);

	DrawLabel(TEXT("ENTER = select        R / click Play Again        TAB = main menu"),
		Canvas->ClipX * 0.5f, CardY + CardH + Px(28), GEStyle::Active().TextBody, 17, 0.5f);
}

// =====================================================================
//                               WIDGETS
// =====================================================================

bool AGEHUD::DrawMenuButton(int32 Index, float X, float Y, float W, float H, const FString& Label,
	const FString& RightText, FLinearColor Accent, float Alpha)
{
	while (ButtonAnim.Num() <= Index)
	{
		ButtonAnim.Add(0.0f);
	}

	// ---------- mouse ----------
	const bool bHovered = bHasMouse && MouseX >= X && MouseX <= X + W && MouseY >= Y && MouseY <= Y + H;
	if (bHovered)
	{
		HoveredItem = Index;
		if (bMouseMoved)
		{
			SelectedItem = Index; // moving the mouse over a button selects it
		}
	}

	// ---------- smooth highlight animation ----------
	const bool bSelected = (SelectedItem == Index);
	float& Anim = ButtonAnim[Index];
	Anim = FMath::FInterpTo(Anim, bSelected ? 1.0f : 0.0f, FrameDeltaTime, 14.0f);

	// ---------- background ----------
	const FLinearColor SelectedFill(Accent.R * 0.22f, Accent.G * 0.22f, Accent.B * 0.22f, 0.9f);
	FLinearColor Fill = FMath::Lerp(GEStyle::ButtonFill, SelectedFill, Anim);
	Fill.A *= Alpha;
	DrawBox(X, Y, W, H, Fill);

	// accent bar on the left (grows when selected)
	DrawBox(X, Y, Px(3) + Px(5) * Anim, H, GEStyle::WithAlpha(Accent, (0.35f + 0.65f * Anim) * Alpha));

	// thin outline when selected
	if (Anim > 0.01f)
	{
		const FLinearColor Border = GEStyle::WithAlpha(Accent, 0.45f * Anim * Alpha);
		DrawBox(X, Y, W, Px(1), Border);
		DrawBox(X, Y + H - Px(1), W, Px(1), Border);
		DrawBox(X + W - Px(1), Y, Px(1), H, Border);
	}

	// ---------- text ----------
	const FVector2D LabelSize = MeasureText(Label, 24);
	FLinearColor LabelColor = FMath::Lerp(FLinearColor(0.85f, 0.88f, 0.94f), GEStyle::TextWhite, Anim);
	LabelColor.A = Alpha;
	DrawLabel(Label, X + Px(26) + Px(8) * Anim, Y + (H - LabelSize.Y) * 0.5f, LabelColor, 24);

	if (!RightText.IsEmpty())
	{
		const FVector2D RightSize = MeasureText(RightText, 20, true);
		FLinearColor RightColor = RightText.StartsWith(TEXT("BEST")) ? GEStyle::Gold : GEStyle::Active().TextBody;
		RightColor.A = Alpha * (0.75f + 0.25f * Anim);
		DrawLabel(RightText, X + W - Px(22), Y + (H - RightSize.Y) * 0.5f, RightColor, 20, 1.0f, true);
	}
	return bHovered;
}

void AGEHUD::DrawSlider(float X, float Y, float W, float Fraction, FLinearColor Accent, float Alpha)
{
	Fraction = FMath::Clamp(Fraction, 0.0f, 1.0f);
	// track, filled part, knob
	DrawBox(X, Y, W, Px(4), FLinearColor(1.0f, 1.0f, 1.0f, 0.1f * Alpha));
	DrawBox(X, Y, W * Fraction, Px(4), GEStyle::WithAlpha(Accent, Alpha));
	DrawBox(X + W * Fraction - Px(5), Y - Px(5), Px(10), Px(14), GEStyle::WithAlpha(GEStyle::TextWhite, Alpha));
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
