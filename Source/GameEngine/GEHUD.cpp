#include "GEHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GECharacter.h"
#include "GEWeaponComponent.h"

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
