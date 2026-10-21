#include "GEHUD.h"
#include "Engine/Canvas.h"

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
	const float Gap = Crosshair.Gap;
	const float HalfThick = Thick * 0.5f;

	// Each line is a thin rectangle. (X, Y) is the rectangle's top-left corner.
	// Left line
	DrawOutlinedRect(CenterX - Gap - Size, CenterY - HalfThick, Size, Thick);
	// Right line
	DrawOutlinedRect(CenterX + Gap, CenterY - HalfThick, Size, Thick);
	// Top line
	DrawOutlinedRect(CenterX - HalfThick, CenterY - Gap - Size, Thick, Size);
	// Bottom line
	DrawOutlinedRect(CenterX - HalfThick, CenterY + Gap, Thick, Size);
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
