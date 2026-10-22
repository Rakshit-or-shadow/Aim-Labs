#include "GEHUD.h"
#include "GEUIKit.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GECharacter.h"
#include "GEWeaponComponent.h"
#include "GETrainingManager.h"
#include "GEFlickTypes.h"
#include "GECoachTypes.h"
#include "GameFramework/PlayerController.h"
#include "GESaveGame.h"
#include "GEStyle.h"
#include "Engine/Font.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------------
// Phase 3: Training Center overlay, popups and clickable menu (key M).
// ---------------------------------------------------------------------------
void AGEHUD::DrawTrainingOverlay()
{
	UGETrainingManager* Manager = UGETrainingManager::Get(this);
	if (!Manager || !Manager->GetCenter())
	{
		return;
	}
	DrawPopups(*Manager);

	// Small hint + live stats while a session runs.
	DrawText(TEXT("T: Training Center   M: Menu"), FLinearColor(0.7f, 0.9f, 1.0f), 20.0f, 20.0f, GEngine->GetSmallFont());
	if (Manager->IsSessionRunning() && !Manager->IsMenuOpen())
	{
		const FGETrainingStats& S = Manager->GetStats();
		DrawText(FString::Printf(TEXT("Kills %d   Acc %.0f%%   HS %.0f%%"), S.Kills, S.GetAccuracyPercent(), S.GetHeadshotPercent()),
			FLinearColor::Yellow, 20.0f, 42.0f, GEngine->GetSmallFont());
	}
	if (Manager->IsMenuOpen())
	{
		DrawTrainingMenu(*Manager);
	}
}

void AGEHUD::DrawPopups(UGETrainingManager& Manager)
{
	const float Now = GetWorld()->GetTimeSeconds();
	for (const FGEPopup& P : Manager.GetPopups())
	{
		const float Age = Now - P.SpawnTime;
		const float Alpha = 1.0f - Age / Manager.GetPopupDuration();
		// Text floats upward while it fades out.
		const FVector ScreenPos = Project(P.WorldLocation + FVector(0, 0, Age * 60.0f));
		if (ScreenPos.Z <= 0.0f)
		{
			continue; // behind the camera
		}
		float W = 0.0f, H = 0.0f;
		GetTextSize(P.Text, W, H, GEngine->GetMediumFont(), 1.4f);
		FLinearColor Col = P.Color;
		Col.A = FMath::Clamp(Alpha, 0.0f, 1.0f);
		DrawText(P.Text, Col, ScreenPos.X - W * 0.5f, ScreenPos.Y - H, GEngine->GetMediumFont(), 1.4f);
	}
}

void AGEHUD::DrawButton(const FString& Label, const FName& Name, float X, float Y, float W, float H, bool bSelected)
{
	float MX = -1.0f, MY = -1.0f;
	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->GetMousePosition(MX, MY);
	}
	const bool bHover = MX >= X && MX <= X + W && MY >= Y && MY <= Y + H;
	FLinearColor Fill = bSelected ? FLinearColor(0.1f, 0.5f, 0.8f, 0.95f) : FLinearColor(0.15f, 0.15f, 0.18f, 0.95f);
	if (bHover) { Fill += FLinearColor(0.15f, 0.15f, 0.15f, 0.0f); }
	DrawRect(Fill, X, Y, W, H);
	float TW = 0.0f, TH = 0.0f;
	GetTextSize(Label, TW, TH, GEngine->GetSmallFont(), 1.0f);
	DrawText(Label, FLinearColor::White, X + (W - TW) * 0.5f, Y + (H - TH) * 0.5f, GEngine->GetSmallFont());
	AddHitBox(FVector2D(X, Y), FVector2D(W, H), Name, true);
}

void AGEHUD::DrawTrainingMenu(UGETrainingManager& Manager)
{
	const float PanelW = 640.0f, PanelH = 570.0f;
	const float PX = (Canvas->ClipX - PanelW) * 0.5f;
	const float PY = (Canvas->ClipY - PanelH) * 0.5f;
	DrawRect(FLinearColor(0.02f, 0.02f, 0.04f, 0.85f), PX, PY, PanelW, PanelH);
	DrawText(TEXT("TRAINING CENTER"), FLinearColor::White, PX + 20.0f, PY + 12.0f, GEngine->GetMediumFont(), 1.3f);

	const float BH = 34.0f;
	const float BW = 110.0f;
	const float Gap = 8.0f;
	float Y = PY + 60.0f;

	// Mode row
	DrawText(TEXT("Mode"), FLinearColor::Yellow, PX + 20.0f, Y, GEngine->GetSmallFont());
	Y += 20.0f;
	const EGETrainingMode Modes[4] = { EGETrainingMode::Static, EGETrainingMode::Strafing, EGETrainingMode::Peek, EGETrainingMode::Flick };
	for (int32 i = 0; i < 4; ++i)
	{
		DrawButton(UGETrainingManager::ModeToString(Modes[i]), *FString::Printf(TEXT("Mode:%d"), (int32)Modes[i]), PX + 20.0f + i * (BW + Gap), Y, BW, BH, Manager.GetMode() == Modes[i]);
	}
	Y += BH + 14.0f;

	// Difficulty row
	DrawText(TEXT("Difficulty"), FLinearColor::Yellow, PX + 20.0f, Y, GEngine->GetSmallFont());
	Y += 20.0f;
	const TCHAR* DiffNames[3] = { TEXT("Easy"), TEXT("Medium"), TEXT("Hard") };
	for (int32 i = 0; i < 3; ++i)
	{
		DrawButton(DiffNames[i], *FString::Printf(TEXT("Diff:%d"), i), PX + 20.0f + i * (BW + Gap), Y, BW, BH, (int32)Manager.GetDifficulty() == i);
	}
	Y += BH + 14.0f;

	// Weapon row
	DrawText(TEXT("Weapon"), FLinearColor::Yellow, PX + 20.0f, Y, GEngine->GetSmallFont());
	Y += 20.0f;
	if (UGEWeaponComponent* Weapon = Manager.GetPlayerWeapon())
	{
		const TArray<FGEWeaponData>& Weapons = Weapon->GetWeapons();
		const float WBW = (PanelW - 40.0f - Gap * (Weapons.Num() - 1)) / FMath::Max(Weapons.Num(), 1);
		for (int32 i = 0; i < Weapons.Num(); ++i)
		{
			DrawButton(Weapons[i].Name, *FString::Printf(TEXT("Wpn:%d"), i), PX + 20.0f + i * (WBW + Gap), Y, WBW, BH, Manager.GetWeaponIndex() == i);
		}
	}
	Y += BH + 18.0f;

	// Start / Stop and Close
	DrawButton(Manager.IsSessionRunning() ? TEXT("STOP") : TEXT("START"), TEXT("Toggle"), PX + 20.0f, Y, 160.0f, 40.0f, Manager.IsSessionRunning());
	DrawButton(TEXT("Close (M)"), TEXT("Close"), PX + 190.0f, Y, 120.0f, 40.0f, false);
	Y += 56.0f;

	// Stats
	const FGETrainingStats& S = Manager.GetStats();
	DrawText(Manager.IsSessionRunning() ? TEXT("Session stats (running)") : TEXT("Last session stats"), FLinearColor::Yellow, PX + 20.0f, Y, GEngine->GetSmallFont());
	Y += 22.0f;
	DrawText(FString::Printf(TEXT("Kills: %d     Headshot kills: %.0f%%     Time: %.0f s"), S.Kills, S.GetHeadshotPercent(), S.Duration), FLinearColor::White, PX + 20.0f, Y, GEngine->GetSmallFont());
	Y += 20.0f;
	DrawText(FString::Printf(TEXT("Accuracy: %.0f%% (%d/%d)     First-shot accuracy: %.0f%% (%d/%d)"), S.GetAccuracyPercent(), S.Hits, S.Shots, S.GetFirstShotAccuracyPercent(), S.FirstShotHits, S.FirstShots), FLinearColor::White, PX + 20.0f, Y, GEngine->GetSmallFont());
	Y += 20.0f;
	DrawText(FString::Printf(TEXT("Avg time to kill: %.2f s"), S.GetAverageTimeToKill()), FLinearColor::White, PX + 20.0f, Y, GEngine->GetSmallFont());
	Y += 30.0f;

	// Phase 4: mouse sensitivity (stored) and DPI.
	DrawText(FString::Printf(TEXT("Sensitivity x%.2f  =  %.1f cm/360"), Manager.GetSensitivityScale(), Manager.GetCm360()), FLinearColor::Yellow, PX + 20.0f, Y + 5.0f, GEngine->GetSmallFont());
	DrawButton(TEXT("-"), TEXT("SensDown"), PX + 330.0f, Y, 40.0f, 26.0f, false);
	DrawButton(TEXT("+"), TEXT("SensUp"), PX + 376.0f, Y, 40.0f, 26.0f, false);
	Y += 34.0f;
	DrawText(FString::Printf(TEXT("Mouse DPI (assumed): %d"), Manager.GetMouseDpi()), FLinearColor::Yellow, PX + 20.0f, Y + 5.0f, GEngine->GetSmallFont());
	DrawButton(TEXT("-100"), TEXT("DpiDown"), PX + 330.0f, Y, 40.0f, 26.0f, false);
	DrawButton(TEXT("+100"), TEXT("DpiUp"), PX + 376.0f, Y, 40.0f, 26.0f, false);

	// Phase 4: flick results panel beside the menu, shown after a flick session.
	if (Manager.GetMode() == EGETrainingMode::Flick && !Manager.IsSessionRunning() && Manager.HasFlickSummary())
	{
		DrawFlickResults(Manager, FMath::Min(PX + PanelW + 10.0f, Canvas->ClipX - 440.0f), PY);
	}

	// Phase 5: coach panel on the left of the menu, shown after any finished session.
	if (!Manager.IsSessionRunning() && Manager.HasCoachReport())
	{
		DrawCoachPanel(Manager, FMath::Max(PX - 450.0f, 10.0f), PY);
	}
}

float AGEHUD::DrawWrappedText(const FString& Text, const FLinearColor& Color, float X, float Y, float MaxWidth)
{
	TArray<FString> Words;
	Text.ParseIntoArray(Words, TEXT(" "), true);
	FString Line;
	float LineH = 14.0f;
	for (const FString& Word : Words)
	{
		const FString Test = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
		float TW = 0.0f;
		GetTextSize(Test, TW, LineH, GEngine->GetSmallFont(), 1.0f);
		if (TW > MaxWidth && !Line.IsEmpty())
		{
			DrawText(Line, Color, X, Y, GEngine->GetSmallFont());
			Y += LineH + 2.0f;
			Line = Word;
		}
		else
		{
			Line = Test;
		}
	}
	if (!Line.IsEmpty())
	{
		DrawText(Line, Color, X, Y, GEngine->GetSmallFont());
		Y += LineH + 2.0f;
	}
	return Y;
}

void AGEHUD::DrawCoachPanel(UGETrainingManager& Manager, float X, float Y)
{
	const FGECoachReport& R = Manager.GetCoachReport();
	const float W = 440.0f;
	const float TextW = W - 28.0f;
	DrawRect(FLinearColor(0.02f, 0.02f, 0.04f, 0.9f), X, Y, W, 570.0f);
	DrawText(TEXT("AI COACH"), FLinearColor::White, X + 14.0f, Y + 10.0f, GEngine->GetMediumFont(), 1.2f);
	float LY = Y + 44.0f;

	DrawText(TEXT("Top 3 tips"), FLinearColor::Yellow, X + 14.0f, LY, GEngine->GetSmallFont());
	LY += 22.0f;
	for (int32 i = 0; i < R.Tips.Num(); ++i)
	{
		LY = DrawWrappedText(FString::Printf(TEXT("%d. %s"), i + 1, *R.Tips[i].Text), FLinearColor(0.4f, 1.0f, 0.5f), X + 14.0f, LY, TextW);
		LY += 10.0f;
	}
	LY += 8.0f;
	DrawText(TEXT("Trends"), FLinearColor::Yellow, X + 14.0f, LY, GEngine->GetSmallFont());
	LY += 22.0f;
	for (const FString& Line : R.Trends)
	{
		LY = DrawWrappedText(Line, FLinearColor::White, X + 14.0f, LY, TextW);
		LY += 4.0f;
	}
}

void AGEHUD::DrawFlickResults(UGETrainingManager& Manager, float X, float Y)
{
	const FGEFlickSummary& F = Manager.GetLastFlickSummary();
	const float W = 430.0f;
	DrawRect(FLinearColor(0.02f, 0.02f, 0.04f, 0.9f), X, Y, W, 400.0f);
	DrawText(TEXT("FLICK RESULTS"), FLinearColor::White, X + 14.0f, Y + 10.0f, GEngine->GetMediumFont(), 1.2f);
	float LY = Y + 40.0f;
	DrawText(FString::Printf(TEXT("Targets %d   Hit %d (%.0f%%)   Avg time %.2f s"), F.TotalTargets, F.TotalHits, F.GetAccuracyPercent(), F.AverageTimeToHit), FLinearColor::White, X + 14.0f, LY, GEngine->GetSmallFont());
	LY += 24.0f;

	// Per-direction table
	DrawText(TEXT("Direction"), FLinearColor::Yellow, X + 14.0f, LY, GEngine->GetSmallFont());
	DrawText(TEXT("Hit/Tot"), FLinearColor::Yellow, X + 150.0f, LY, GEngine->GetSmallFont());
	DrawText(TEXT("Acc"), FLinearColor::Yellow, X + 240.0f, LY, GEngine->GetSmallFont());
	DrawText(TEXT("Avg time"), FLinearColor::Yellow, X + 320.0f, LY, GEngine->GetSmallFont());
	LY += 20.0f;
	for (int32 i = 0; i < F.Directions.Num(); ++i)
	{
		const FGEFlickDirStats& D = F.Directions[i];
		const FLinearColor Col = D.Targets > 0 ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f);
		DrawText(GEFlickDirectionToString((EGEFlickDirection)i), Col, X + 14.0f, LY, GEngine->GetSmallFont());
		DrawText(FString::Printf(TEXT("%d/%d"), D.Hits, D.Targets), Col, X + 150.0f, LY, GEngine->GetSmallFont());
		DrawText(D.Targets > 0 ? FString::Printf(TEXT("%.0f%%"), D.GetAccuracyPercent()) : FString(TEXT("-")), Col, X + 240.0f, LY, GEngine->GetSmallFont());
		DrawText(D.Hits > 0 ? FString::Printf(TEXT("%.2f s"), D.GetAverageTime()) : FString(TEXT("-")), Col, X + 320.0f, LY, GEngine->GetSmallFont());
		LY += 19.0f;
	}
	LY += 8.0f;
	DrawText(FString::Printf(TEXT("Avg overshoot: %+.1f%%  (%d flicks)   Corrections/flick: %.1f"), F.AverageOvershootPercent, F.OvershootSamples, F.AverageCorrections), FLinearColor::White, X + 14.0f, LY, GEngine->GetSmallFont());
	LY += 22.0f;
	if (F.bHasSuggestion)
	{
		DrawText(FString::Printf(TEXT("Suggestion: %s sensitivity by %.0f%%"), F.SuggestedChangePercent < 0.0f ? TEXT("lower") : TEXT("raise"), FMath::Abs(F.SuggestedChangePercent)), FLinearColor(0.4f, 1.0f, 0.5f), X + 14.0f, LY, GEngine->GetSmallFont());
		LY += 20.0f;
		DrawText(FString::Printf(TEXT("%.1f cm/360  ->  %.1f cm/360"), F.CurrentCm360, F.SuggestedCm360), FLinearColor(0.4f, 1.0f, 0.5f), X + 14.0f, LY, GEngine->GetSmallFont());
		DrawButton(TEXT("Apply"), TEXT("ApplySens"), X + 320.0f, LY - 4.0f, 90.0f, 26.0f, false);
	}
	else
	{
		DrawText(TEXT("No consistent over/undershoot - keep your sensitivity."), FLinearColor(0.8f, 0.8f, 0.8f), X + 14.0f, LY, GEngine->GetSmallFont());
	}
}

void AGEHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);
	UGETrainingManager* Manager = UGETrainingManager::Get(this);
	if (!Manager || !Manager->IsMenuOpen())
	{
		return;
	}
	// Box names look like "Mode:1", "Diff:2", "Wpn:3", "Toggle", "Close".
	const FString Name = BoxName.ToString();
	FString Kind, Value;
	if (!Name.Split(TEXT(":"), &Kind, &Value))
	{
		Kind = Name;
	}
	const int32 Index = FCString::Atoi(*Value);
	if (Kind == TEXT("Mode")) { Manager->SetMode((EGETrainingMode)Index); }
	else if (Kind == TEXT("Diff")) { Manager->SetDifficulty((EGEDifficulty)Index); }
	else if (Kind == TEXT("Wpn")) { Manager->SetWeaponIndex(Index); }
	else if (Kind == TEXT("Toggle")) { Manager->IsSessionRunning() ? Manager->StopSession() : Manager->StartSession(); }
	else if (Kind == TEXT("SensDown")) { Manager->SetSensitivityScale(Manager->GetSensitivityScale() / GESensitivity::ButtonStep); }
	else if (Kind == TEXT("SensUp")) { Manager->SetSensitivityScale(Manager->GetSensitivityScale() * GESensitivity::ButtonStep); }
	else if (Kind == TEXT("DpiDown")) { Manager->SetMouseDpi(Manager->GetMouseDpi() - GESensitivity::DpiStep); }
	else if (Kind == TEXT("DpiUp")) { Manager->SetMouseDpi(Manager->GetMouseDpi() + GESensitivity::DpiStep); }
	else if (Kind == TEXT("ApplySens")) { Manager->ApplyFlickSuggestion(); }
	else if (Kind == TEXT("Close")) { Manager->ToggleMenu(); }
}

AGEHUD::AGEHUD()
{
	// Monospace font for numbers (timer, scores).
	static ConstructorHelpers::FObjectFinder<UFont> Mono(TEXT("/Engine/EngineFonts/DroidSansMono.DroidSansMono"));
	if (Mono.Succeeded())
	{
		MonoFont = Mono.Object;
	}
}

void AGEHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	// Everything in the menus is designed for 1080p and scaled to the real screen size.
	UIScale = Canvas->ClipY / 1080.0f;
	if (const UGESaveGame* ThemeSave = GetSave())
	{
		GEStyle::SetActiveTheme(ThemeSave->UITheme); // the UI Theme setting applies live
	}
	FrameDeltaTime = GetWorld()->GetDeltaSeconds();
	NowTime = GetWorld()->GetTimeSeconds();

	// Where is the mouse, and did it move this frame?
	bHasMouse = PlayerOwner && !UGESaveGame::IsTourMode() && PlayerOwner->GetMousePosition(MouseX, MouseY); // -ShotTour: the real mouse must not hover buttons
	bMouseMoved = bHasMouse && (FMath::Abs(MouseX - LastMouseX) > 0.5f || FMath::Abs(MouseY - LastMouseY) > 0.5f);

	// A new screen appeared: restart its intro animation.
	if (Screen != LastDrawnScreen)
	{
		LastDrawnScreen = Screen;
		ScreenStartTime = NowTime;
		ButtonAnim.Empty();
	}

	// Reset; DrawMenuButton sets it again if the mouse is over a button.
	HoveredItem = -1;

	if (!bFlowActive)
	{
		// A level without the aim arena: just the normal gameplay HUD.
		DrawGameplayHUD();
	}
	else
	{
		switch (Screen)
		{
		case EGEScreen::MainMenu:   DrawMainMenu();   break;
		case EGEScreen::ModeSelect: DrawModeSelect(); break;
		case EGEScreen::Settings:   DrawSettings();   break;
		case EGEScreen::Results:    DrawResults();    break;
		case EGEScreen::Playing:    DrawPlaying();    break;
		case EGEScreen::Pause:
			if (PauseReturn == EGEScreen::Playing)
			{
				DrawPlaying(); // the frozen round stays visible behind the pause card
			}
			DrawPause();
			break;
		default:                    DrawGameplayHUD(); break; // FreeRoam, TrainingCenter, FlickOptimizer
		}
	}

	if (bHasMouse)
	{
		LastMouseX = MouseX;
		LastMouseY = MouseY;
	}

	// Stage B3: optional scanlines + screen-change glitch on top of everything (menus only)
	if (bFlowActive)
	{
		FGEUIKit(this).Overlay(ScreenAge());
	}

	// Stage B3: debug gallery of every UI component (console: ge.ui.gallery)
	if (FGEUIKit::GalleryOn())
	{
		FGEUIKit(this).DrawGallery(NowTime);
	}
}

void AGEHUD::DrawGameplayHUD()
{
	// Screen center.
	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;

	// Phase 2: the gap widens with the weapon's current spread.
	const AGECharacter* GECharacter = Cast<AGECharacter>(GetOwningPawn());
	const UGEWeaponComponent* Weapon = GECharacter ? GECharacter->GetWeaponComponent() : nullptr;
	float ExtraGap = 0.0f;
	if (Weapon)
	{
		// Convert the spread angle into screen pixels using the current FOV.
		const float HalfFov = FMath::DegreesToRadians(FMath::Max(Weapon->GetCurrentFov(), 1.0f) * 0.5f);
		ExtraGap = FMath::Tan(FMath::DegreesToRadians(Weapon->GetCurrentSpreadDeg())) / FMath::Tan(HalfFov) * CenterX;
	}
	const bool bScoped = Weapon && Weapon->IsScoped();

	// Crosshair color comes from the Settings screen (saved). Falls back to the class default.
	FLinearColor CrosshairColor = Crosshair.Color;
	if (const UGESaveGame* Save = GetSave())
	{
		CrosshairColor = GEStyle::GetCrosshairColor(Save->CrosshairColor);
	}

	if (bScoped)
	{
		DrawScope(CenterX, CenterY);
	}
	else
	{
		DrawCrosshairAt(CenterX, CenterY, CrosshairColor, ExtraGap);
	}

	if (Weapon)
	{
		DrawWeaponInfo(*Weapon);
		DrawHitMarker(*Weapon, CenterX, CenterY);
	}

	DrawTrainingOverlay();

	// Debug speed readout (toggle with F3).
	if (const AGECharacter* Char = Cast<AGECharacter>(GetOwningPawn()))
	{
		if (Char->IsSpeedReadoutVisible())
		{
			const FString Text = FString::Printf(TEXT("Speed: %.0f cm/s  %s"), Char->GetCurrentSpeed(), Char->IsOnGround() ? TEXT("Ground") : TEXT("Air"));
			DrawText(Text, FLinearColor::Yellow, 20.0f, Canvas->ClipY - 50.0f, GEngine->GetSmallFont());
		}
	}
}

void AGEHUD::DrawCrosshairAt(float CX, float CY, const FLinearColor& Color, float ExtraGap)
{
	const float Gap = Crosshair.Gap + ExtraGap;
	const float Len = Crosshair.Size;
	const float O = Crosshair.OutlineThickness;

	// Draw everything twice: a thick dark outline first, then the colored lines on top.
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		const bool bOutline = (Pass == 0);
		if (bOutline && O <= 0.0f)
		{
			continue;
		}
		const FLinearColor C = bOutline ? Crosshair.OutlineColor : Color;
		const float Thick = Crosshair.Thickness + (bOutline ? 2.0f * O : 0.0f);

		DrawLine(CX - Gap - Len, CY, CX - Gap, CY, C, Thick);
		DrawLine(CX + Gap, CY, CX + Gap + Len, CY, C, Thick);
		DrawLine(CX, CY - Gap - Len, CX, CY - Gap, C, Thick);
		DrawLine(CX, CY + Gap, CX, CY + Gap + Len, C, Thick);

		const float Dot = bOutline ? 4.0f : 2.0f; // center dot
		DrawRect(C, CX - Dot * 0.5f, CY - Dot * 0.5f, Dot, Dot);
	}
}

void AGEHUD::DrawWeaponInfo(const UGEWeaponComponent& Weapon)
{
	const FGEWeaponData& W = Weapon.GetCurrentWeapon();
	const float X = Canvas->ClipX - 280.0f;
	const float Y = Canvas->ClipY - 110.0f;

	DrawText(W.Name, FLinearColor::White, X, Y, GEngine->GetMediumFont(), 1.3f);
	const FString Ammo = Weapon.IsReloading() ? FString(TEXT("Reloading...")) : FString::Printf(TEXT("%d / %d"), Weapon.GetMagAmmo(), Weapon.GetReserveAmmo());
	const FLinearColor AmmoColor = (Weapon.GetMagAmmo() == 0 && !Weapon.IsReloading()) ? FLinearColor::Red : FLinearColor::Yellow;
	DrawText(Ammo, AmmoColor, X, Y + 32.0f, GEngine->GetMediumFont(), 1.6f);

	// Small slot list: current weapon in brackets.
	FString Slots;
	for (int32 i = 0; i < Weapon.GetWeapons().Num(); ++i)
	{
		Slots += (i == Weapon.GetCurrentIndex()) ? FString::Printf(TEXT("[%d] "), i + 1) : FString::Printf(TEXT("%d "), i + 1);
	}
	DrawText(Slots, FLinearColor(0.7f, 0.7f, 0.7f), X, Y + 70.0f, GEngine->GetSmallFont());
}

void AGEHUD::DrawHitMarker(const UGEWeaponComponent& Weapon, float CX, float CY)
{
	const float Age = Weapon.GetTimeSinceHit();
	if (Age > HitMarkerDuration)
	{
		return;
	}
	const float Alpha = 1.0f - Age / HitMarkerDuration;
	FLinearColor Col = Weapon.WasLastHitKill() ? FLinearColor(1.0f, 0.2f, 0.2f) : (Weapon.WasLastHitHeadshot() ? FLinearColor(1.0f, 0.6f, 0.1f) : FLinearColor::White);
	Col.A = Alpha;
	const float In = 6.0f;
	const float Out = 14.0f;
	// Four short diagonal lines forming an X around the crosshair.
	DrawLine(CX - Out, CY - Out, CX - In, CY - In, Col, 2.0f);
	DrawLine(CX + Out, CY - Out, CX + In, CY - In, Col, 2.0f);
	DrawLine(CX - Out, CY + Out, CX - In, CY + In, Col, 2.0f);
	DrawLine(CX + Out, CY + Out, CX + In, CY + In, Col, 2.0f);
}

void AGEHUD::DrawScope(float CX, float CY)
{
	// Black everywhere except a centered square, plus thin crosshair lines.
	const float Half = FMath::Min(Canvas->ClipX, Canvas->ClipY) * 0.42f;
	const FLinearColor Black(0, 0, 0, 1);
	DrawRect(Black, 0, 0, CX - Half, Canvas->ClipY);
	DrawRect(Black, CX + Half, 0, Canvas->ClipX - (CX + Half), Canvas->ClipY);
	DrawRect(Black, CX - Half, 0, Half * 2.0f, CY - Half);
	DrawRect(Black, CX - Half, CY + Half, Half * 2.0f, Canvas->ClipY - (CY + Half));
	DrawLine(CX - Half, CY, CX + Half, CY, FLinearColor::Black, 1.0f);
	DrawLine(CX, CY - Half, CX, CY + Half, FLinearColor::Black, 1.0f);
	DrawRect(FLinearColor::Red, CX - 1.5f, CY - 1.5f, 3.0f, 3.0f);
}
