#include "GEHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GECharacter.h"
#include "GEWeaponComponent.h"
#include "GETrainingManager.h"
#include "GEFlickTypes.h"
#include "GameFramework/PlayerController.h"

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
		const FVector Screen = Project(P.WorldLocation + FVector(0, 0, Age * 60.0f));
		if (Screen.Z <= 0.0f)
		{
			continue; // behind the camera
		}
		float W = 0.0f, H = 0.0f;
		GetTextSize(P.Text, W, H, GEngine->GetMediumFont(), 1.4f);
		FLinearColor Col = P.Color;
		Col.A = FMath::Clamp(Alpha, 0.0f, 1.0f);
		DrawText(P.Text, Col, Screen.X - W * 0.5f, Screen.Y - H, GEngine->GetMediumFont(), 1.4f);
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

void AGEHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	// Screen center.
	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;

	const float Size = Crosshair.Size;
	const float Thick = Crosshair.Thickness;
	// Phase 2: the gap widens with the weapon's current spread.
	const AGECharacter* GECharacter = Cast<AGECharacter>(GetOwningPawn());
	const UGEWeaponComponent* Weapon = GECharacter ? GECharacter->GetWeaponComponent() : nullptr;
	float Gap = Crosshair.Gap;
	if (Weapon)
	{
		// Convert the spread angle into screen pixels using the current FOV.
		const float HalfFov = FMath::DegreesToRadians(FMath::Max(Weapon->GetCurrentFov(), 1.0f) * 0.5f);
		const float SpreadPx = FMath::Tan(FMath::DegreesToRadians(Weapon->GetCurrentSpreadDeg())) / FMath::Tan(HalfFov) * CenterX;
		Gap += SpreadPx;
	}
	const bool bScoped = Weapon && Weapon->IsScoped();
	const float HalfThick = Thick * 0.5f;

	// Each line is a thin rectangle. (X, Y) is the rectangle's top-left corner.
	if (bScoped)
	{
		DrawScope(CenterX, CenterY);
	}
	else
	{
	// Left line
	DrawOutlinedRect(CenterX - Gap - Size, CenterY - HalfThick, Size, Thick);
	// Right line
	DrawOutlinedRect(CenterX + Gap, CenterY - HalfThick, Size, Thick);
	// Top line
	DrawOutlinedRect(CenterX - HalfThick, CenterY - Gap - Size, Thick, Size);
	// Bottom line
	DrawOutlinedRect(CenterX - HalfThick, CenterY + Gap, Thick, Size);
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

void AGEHUD::DrawOutlinedRect(float X, float Y, float W, float H)
{
	const float O = Crosshair.OutlineThickness;

	// Draw the outline first (a slightly bigger rectangle) so the colored line sits on top.
	if (O > 0.0f)
	{
		DrawRect(Crosshair.OutlineColor, X - O, Y - O, W + 2.0f * O, H + 2.0f * O);
	}
	DrawRect(Crosshair.Color, X, Y, W, H);
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
