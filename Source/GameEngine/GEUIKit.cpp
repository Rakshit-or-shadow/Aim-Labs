// GEUIKit.cpp : see GEUIKit.h
#include "GEUIKit.h"
#include "GEHUD.h"
#include "GESaveGame.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "RenderResource.h"
#include "HAL/IConsoleManager.h"
#include "Math/RandomStream.h"

using namespace GEStyle;

FGEUIKit::FGEUIKit(AGEHUD* InHud)
	: Hud(InHud)
{
	Canvas = Hud ? Hud->Canvas : nullptr;
	if (Canvas)
	{
		W = Canvas->ClipX;
		H = Canvas->ClipY;
		Scale = H / 1080.0f;
	}
	Th = &GEStyle::Active();
}

bool& FGEUIKit::GalleryOn()
{
	static bool bOn = false;
	return bOn;
}

static FAutoConsoleCommand GEUIGalleryCmd(
	TEXT("ge.ui.gallery"),
	TEXT("Toggle the UI component gallery (every component in every state)."),
	FConsoleCommandDelegate::CreateLambda([]() { FGEUIKit::GalleryOn() = !FGEUIKit::GalleryOn(); }));

// =====================================================================
//  primitives
// =====================================================================

void FGEUIKit::Rect(float X, float Y, float RW, float RH, const FLinearColor& Color) const
{
	if (Hud && RW > 0.0f && RH > 0.0f && Color.A > 0.0f)
	{
		Hud->DrawRect(Color, X, Y, RW, RH);
	}
}

void FGEUIKit::Tri(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Color) const
{
	if (Canvas == nullptr || Color.A <= 0.0f)
	{
		return;
	}
	FCanvasTriangleItem Item(A, B, C, GWhiteTexture);
	Item.SetColor(Color);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void FGEUIKit::Line(const FVector2D& A, const FVector2D& B, const FLinearColor& Color, float Thickness) const
{
	if (Canvas == nullptr || Color.A <= 0.0f)
	{
		return;
	}
	FCanvasLineItem Item(A, B);
	Item.SetColor(Color);
	Item.LineThickness = FMath::Max(1.0f, Thickness);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void FGEUIKit::ShapePoints(float X, float Y, float PW, float PH, float Cut, uint8 Mask, TArray<FVector2D>& P)
{
	P.Reset();
	// clockwise from the top-left corner; a cut corner becomes two points
	if (Mask & GECut::TL) { P.Add(FVector2D(X, Y + Cut)); P.Add(FVector2D(X + Cut, Y)); } else { P.Add(FVector2D(X, Y)); }
	if (Mask & GECut::TR) { P.Add(FVector2D(X + PW - Cut, Y)); P.Add(FVector2D(X + PW, Y + Cut)); } else { P.Add(FVector2D(X + PW, Y)); }
	if (Mask & GECut::BR) { P.Add(FVector2D(X + PW, Y + PH - Cut)); P.Add(FVector2D(X + PW - Cut, Y + PH)); } else { P.Add(FVector2D(X + PW, Y + PH)); }
	if (Mask & GECut::BL) { P.Add(FVector2D(X + Cut, Y + PH)); P.Add(FVector2D(X, Y + PH - Cut)); } else { P.Add(FVector2D(X, Y + PH)); }
}

void FGEUIKit::ChamferFill(float X, float Y, float PW, float PH, float Cut, uint8 Mask, const FLinearColor& Color) const
{
	if (PW <= 0.0f || PH <= 0.0f || Color.A <= 0.0f)
	{
		return;
	}
	if (Mask == GECut::None)
	{
		Rect(X, Y, PW, PH, Color);
		return;
	}
	TArray<FVector2D> P;
	ShapePoints(X, Y, PW, PH, Cut, Mask, P);
	// the shape is convex, so a triangle fan from its centre fills it
	const FVector2D Centre(X + PW * 0.5f, Y + PH * 0.5f);
	for (int32 i = 0; i < P.Num(); i++)
	{
		Tri(Centre, P[i], P[(i + 1) % P.Num()], Color);
	}
}

void FGEUIKit::ChamferOutline(float X, float Y, float PW, float PH, float Cut, uint8 Mask, const FLinearColor& Color, float Thickness) const
{
	TArray<FVector2D> P;
	ShapePoints(X, Y, PW, PH, Cut, Mask, P);
	for (int32 i = 0; i < P.Num(); i++)
	{
		Line(P[i], P[(i + 1) % P.Num()], Color, Thickness);
	}
}

void FGEUIKit::GlowRect(float X, float Y, float GW, float GH, const FLinearColor& Color, float Spread) const
{
	// stacked translucent frames, each a little further out and a little fainter
	const int32 Layers = 6;
	const float Step = Spread / Layers;
	for (int32 i = 0; i < Layers; i++)
	{
		const float Out = Step * (i + 1);
		const float Fade = 1.0f - (float)i / Layers;
		const FLinearColor C = WithAlpha(Color, Color.A * 0.22f * Fade * Fade);
		const float T = Step + 0.5f;
		Rect(X - Out, Y - Out, GW + Out * 2.0f, T, C);
		Rect(X - Out, Y + GH + Out - T, GW + Out * 2.0f, T, C);
		Rect(X - Out, Y - Out + T, T, GH + Out * 2.0f - T * 2.0f, C);
		Rect(X + GW + Out - T, Y - Out + T, T, GH + Out * 2.0f - T * 2.0f, C);
	}
}

void FGEUIKit::Text(const FString& Str, float X, float Y, const FLinearColor& Color, float Size, float AlignX, bool bMono) const
{
	if (Hud)
	{
		Hud->DrawLabel(Str, X, Y, Color, Size, AlignX, bMono, false);
	}
}

FVector2D FGEUIKit::Measure(const FString& Str, float Size, bool bMono) const
{
	return Hud ? Hud->MeasureText(Str, Size, bMono) : FVector2D::ZeroVector;
}

void FGEUIKit::TextCentered(const FString& Str, float X, float Y, float BandH, const FLinearColor& Color, float Size, float AlignX, bool bMono) const
{
	const FVector2D M = Measure(Str, Size, bMono);
	Text(Str, X, Y + (BandH - M.Y) * 0.5f, Color, Size, AlignX, bMono);
}

void FGEUIKit::GlowText(const FString& Str, float X, float Y, const FLinearColor& Color, const FLinearColor& Glow, float Size, float AlignX, bool bMono) const
{
	// glow: the same text drawn many times around itself, faintly, in three rings
	for (int32 Ring = 1; Ring <= 3; Ring++)
	{
		const float Radius = S(2.5f * Ring);
		const FLinearColor C = WithAlpha(Glow, 0.07f * Color.A / Ring);
		for (int32 i = 0; i < 8; i++)
		{
			const float A = i * PI / 4.0f;
			Text(Str, X + FMath::Cos(A) * Radius, Y + FMath::Sin(A) * Radius, C, Size, AlignX, bMono);
		}
	}
	Text(Str, X, Y, Color, Size, AlignX, bMono);
}

// =====================================================================
//  panels and buttons
// =====================================================================

void FGEUIKit::Panel(float X, float Y, float PW, float PH, float CutDesign, uint8 Mask, bool bAccentBorder) const
{
	const float Cut = S(CutDesign);
	ChamferFill(X, Y, PW, PH, Cut, Mask, Th->BgRaised);
	ChamferOutline(X, Y, PW, PH, Cut, Mask, bAccentBorder ? Th->BorderSubtle : Th->BorderFaint, S(1.0f));
}

void FGEUIKit::Button(float X, float Y, float BW, float BH, const FString& Label, EGEButtonStyle Style, EGEButtonState State, bool bChevron) const
{
	const float Cut = S(FMath::Clamp(BH / Scale * 0.22f, 6.0f, 14.0f));
	const uint8 Mask = GECut::BR;
	const bool bDisabled = State == EGEButtonState::Disabled;
	const bool bHover = State == EGEButtonState::Hover;
	const bool bPressed = State == EGEButtonState::Pressed;
	if (bPressed)
	{
		Y += S(1.0f); // sinks a pixel when pressed
	}

	FLinearColor Fill = FLinearColor::Transparent, Border = FLinearColor::Transparent, TextColor = Th->TextBody;
	switch (Style)
	{
	case EGEButtonStyle::Primary:
		Fill = bDisabled ? Th->BorderFaint : (bPressed ? Th->AccentDim : (bHover ? Th->AccentStrong : Th->AccentPrimary));
		TextColor = bDisabled ? Th->TextMuted : (bPressed ? Th->TextStrong : Th->TextOnAccent);
		break;
	case EGEButtonStyle::Secondary:
		Fill = bDisabled ? FLinearColor::Transparent : (bPressed ? WithAlpha(Th->AccentDim, 0.6f) : (bHover ? Th->BgAccentHover : Th->BgAccentSubtle));
		Border = bDisabled ? Th->BorderFaint : (bHover || bPressed ? Th->AccentStrong : Th->AccentPrimary);
		TextColor = bDisabled ? Th->TextMuted : Th->TextPrimary;
		break;
	case EGEButtonStyle::Ghost:
		Fill = bDisabled ? FLinearColor::Transparent : (bPressed ? Th->BgAccentHover : (bHover ? Th->BgAccentSubtle : FLinearColor::Transparent));
		Border = bHover || bPressed ? Th->BorderSubtle : FLinearColor::Transparent;
		TextColor = bDisabled ? Th->TextMuted : (bHover || bPressed ? Th->TextStrong : Th->TextBody);
		break;
	}

	if (bHover && !bDisabled && Style != EGEButtonStyle::Ghost)
	{
		GlowRect(X, Y, BW, BH, Th->EffectGlow, S(8.0f));
	}
	ChamferFill(X, Y, BW, BH, Cut, Mask, Fill);
	if (Border.A > 0.0f)
	{
		ChamferOutline(X, Y, BW, BH, Cut, Mask, Border, S(1.0f));
	}

	TextCentered(Label.ToUpper(), X + S(20.0f), Y, BH, TextColor, Type::LabelL);

	if (bChevron)
	{
		// ">" made of two lines
		const float CX = X + BW - S(26.0f) - (bHover ? 0.0f : S(2.0f));
		const float CY = Y + BH * 0.5f;
		const float A = S(5.0f);
		Line(FVector2D(CX - A * 0.6f, CY - A), FVector2D(CX + A * 0.6f, CY), TextColor, S(2.0f));
		Line(FVector2D(CX + A * 0.6f, CY), FVector2D(CX - A * 0.6f, CY + A), TextColor, S(2.0f));
	}
}

void FGEUIKit::NavItem(float X, float Y, float NW, float NH, int32 Index, const FString& Label, EGENavState State) const
{
	const float Cut = S(10.0f);
	const uint8 Mask = GECut::TR;
	const bool bSel = State == EGENavState::Selected;

	if (bSel)
	{
		GlowRect(X, Y, NW, NH, Th->EffectGlow, S(10.0f));
		ChamferFill(X, Y, NW, NH, Cut, Mask, Th->AccentPrimary);
	}
	else
	{
		ChamferFill(X, Y, NW, NH, Cut, Mask, State == EGENavState::Hover ? Th->BgAccentSubtle : FLinearColor::Transparent);
		ChamferOutline(X, Y, NW, NH, Cut, Mask, State == EGENavState::Hover ? Th->BorderDefault : Th->BorderSubtle, S(1.0f));
	}

	const FLinearColor NumColor = bSel ? WithAlpha(Th->TextOnAccent, 0.7f) : Th->TextMuted;
	const FLinearColor LabelColor = bSel ? Th->TextOnAccent : (State == EGENavState::Hover ? Th->TextStrong : Th->TextBody);
	TextCentered(FString::Printf(TEXT("%02d"), Index), X + S(18.0f), Y, NH, NumColor, Type::DataM, 0.0f, true);
	TextCentered(Label.ToUpper(), X + S(58.0f), Y, NH, LabelColor, Type::LabelL);
}

float FGEUIKit::HeaderTabs(float X, float Y, const TArray<FString>& Labels, int32 Active) const
{
	float Cursor = X;
	const float Gap = S(28.0f);
	for (int32 i = 0; i < Labels.Num(); i++)
	{
		const FString Up = Labels[i].ToUpper();
		const bool bAct = i == Active;
		const float TW = Measure(Up, Type::LabelL).X;
		Text(Up, Cursor, Y, bAct ? Th->TextStrong : Th->TextMuted, Type::LabelL);
		if (bAct)
		{
			Rect(Cursor, Y + S(26.0f), TW, S(2.0f), Th->AccentPrimary);
		}
		Cursor += TW + Gap;
	}
	return Cursor - Gap - X;
}

float FGEUIKit::TierTabs(float X, float Y, int32 Active, int32 Count) const
{
	const float TW = S(48.0f), TH = S(30.0f), Gap = S(6.0f);
	for (int32 i = 0; i < Count; i++)
	{
		const float PX = X + i * (TW + Gap);
		const bool bAct = i == Active;
		ChamferFill(PX, Y, TW, TH, S(7.0f), GECut::TR, bAct ? Th->AccentPrimary : Th->BgAccentSubtle);
		if (!bAct)
		{
			ChamferOutline(PX, Y, TW, TH, S(7.0f), GECut::TR, Th->BorderSubtle, S(1.0f));
		}
		TextCentered(FString::Printf(TEXT("T%d"), i + 1), PX + TW * 0.5f, Y, TH, bAct ? Th->TextOnAccent : Th->TextBody, Type::LabelM, 0.5f);
	}
	return Count * (TW + Gap) - Gap;
}

void FGEUIKit::PageIndicator(float X, float Y, int32 Current, int32 Total, const FString& Word) const
{
	const FString Cur = FString::Printf(TEXT("%02d"), Current);
	const FString Tot = FString::Printf(TEXT(" / %02d  "), Total);
	float Cursor = X;
	Text(Cur, Cursor, Y, Th->TextStrong, Type::DataL, 0.0f, true);
	Cursor += Measure(Cur, Type::DataL, true).X;
	Text(Tot, Cursor, Y + S(8.0f), Th->TextMuted, Type::DataM, 0.0f, true);
	Cursor += Measure(Tot, Type::DataM, true).X;
	Text(Word.ToUpper(), Cursor, Y + S(8.0f), Th->TextMuted, Type::LabelM);
}

void FGEUIKit::DiamondTile(float CX, float CY, float R, const FString& Label, bool bSelected) const
{
	const FVector2D Top(CX, CY - R), Right(CX + R, CY), Bottom(CX, CY + R), Left(CX - R, CY);
	if (bSelected)
	{
		// glow: bigger translucent diamonds stacked behind
		for (int32 i = 6; i >= 1; i--)
		{
			const float G = R + S(2.5f) * i;
			const FLinearColor C = WithAlpha(Th->AccentPrimary, 0.035f * (7 - i));
			Tri(FVector2D(CX, CY - G), FVector2D(CX + G, CY), FVector2D(CX, CY + G), C);
			Tri(FVector2D(CX, CY - G), FVector2D(CX, CY + G), FVector2D(CX - G, CY), C);
		}
	}
	const FLinearColor Fill = bSelected ? Th->BgTint : Th->BgRaised;
	Tri(Top, Right, Bottom, Fill);
	Tri(Top, Bottom, Left, Fill);
	const FLinearColor Edge = bSelected ? Th->AccentPrimary : Th->BorderSubtle;
	const float T = S(bSelected ? 2.0f : 1.0f);
	Line(Top, Right, Edge, T); Line(Right, Bottom, Edge, T); Line(Bottom, Left, Edge, T); Line(Left, Top, Edge, T);

	// small inner diamond as the icon placeholder
	const float r = R * 0.28f;
	const FLinearColor Ic = bSelected ? Th->AccentStrong : Th->AccentDim;
	Tri(FVector2D(CX, CY - r), FVector2D(CX + r, CY), FVector2D(CX, CY + r), Ic);
	Tri(FVector2D(CX, CY - r), FVector2D(CX, CY + r), FVector2D(CX - r, CY), Ic);

	Text(Label.ToUpper(), CX, CY + R + S(10.0f), bSelected ? Th->TextStrong : Th->TextBody, Type::LabelM, 0.5f);
}

// =====================================================================
//  controls
// =====================================================================

void FGEUIKit::Slider(float X, float Y, float SW, const FString& Label, float Fraction, const FString& ValueText, bool bFocused) const
{
	Fraction = FMath::Clamp(Fraction, 0.0f, 1.0f);
	Text(Label.ToUpper(), X, Y, bFocused ? Th->TextStrong : Th->TextBody, Type::LabelM);
	Text(ValueText, X + SW, Y - S(2.0f), Th->TextPrimary, Type::DataM, 1.0f, true);

	const float TrackY = Y + S(30.0f);
	Rect(X, TrackY - S(1.0f), SW, S(2.0f), Th->BorderFaint);
	Rect(X, TrackY - S(1.0f), SW * Fraction, S(2.0f), Th->AccentPrimary);
	// vertical thumb
	const float TX = X + SW * Fraction;
	if (bFocused)
	{
		GlowRect(TX - S(2.0f), TrackY - S(10.0f), S(4.0f), S(20.0f), Th->EffectGlow, S(6.0f));
	}
	Rect(TX - S(2.0f), TrackY - S(10.0f), S(4.0f), S(20.0f), bFocused ? Th->AccentStrong : Th->AccentPrimary);
}

void FGEUIKit::Toggle(float X, float Y, float TW, const FString& Label, bool bOn, bool bFocused) const
{
	const float SwW = S(76.0f), SwH = S(28.0f);
	TextCentered(Label.ToUpper(), X, Y, SwH, bFocused ? Th->TextStrong : Th->TextBody, Type::LabelM);
	const float SX = X + TW - SwW;
	if (bOn)
	{
		ChamferFill(SX, Y, SwW, SwH, S(7.0f), GECut::BR, Th->AccentPrimary);
		TextCentered(TEXT("ON"), SX + SwW * 0.5f, Y, SwH, Th->TextOnAccent, Type::LabelM, 0.5f);
	}
	else
	{
		ChamferFill(SX, Y, SwW, SwH, S(7.0f), GECut::BR, Th->BgSunken);
		ChamferOutline(SX, Y, SwW, SwH, S(7.0f), GECut::BR, Th->BorderFaint, S(1.0f));
		TextCentered(TEXT("OFF"), SX + SwW * 0.5f, Y, SwH, Th->TextMuted, Type::LabelM, 0.5f);
	}
	if (bFocused)
	{
		ChamferOutline(SX - S(3.0f), Y - S(3.0f), SwW + S(6.0f), SwH + S(6.0f), S(8.0f), GECut::BR, Th->AccentStrong, S(1.0f));
	}
}

void FGEUIKit::Checkbox(float X, float Y, const FString& Label, bool bChecked, bool bFocused) const
{
	const float B = S(22.0f);
	ChamferFill(X, Y, B, B, S(6.0f), GECut::BR, bChecked ? Th->AccentPrimary : Th->BgSunken);
	ChamferOutline(X, Y, B, B, S(6.0f), GECut::BR, bChecked ? Th->AccentStrong : (bFocused ? Th->AccentPrimary : Th->BorderSubtle), S(1.0f));
	if (bChecked)
	{
		Line(FVector2D(X + B * 0.22f, Y + B * 0.52f), FVector2D(X + B * 0.42f, Y + B * 0.72f), Th->TextOnAccent, S(2.0f));
		Line(FVector2D(X + B * 0.42f, Y + B * 0.72f), FVector2D(X + B * 0.78f, Y + B * 0.28f), Th->TextOnAccent, S(2.0f));
	}
	TextCentered(Label.ToUpper(), X + B + S(12.0f), Y, B, bFocused ? Th->TextStrong : Th->TextBody, Type::LabelM);
}

void FGEUIKit::StatRow(float X, float Y, float RW, const FString& Label, const FString& Value, bool bHighlight) const
{
	Text(Label.ToUpper(), X, Y, Th->TextMuted, Type::LabelM);
	Text(Value, X + RW, Y - S(1.0f), bHighlight ? Th->TextHighlight : Th->TextStrong, Type::DataL * 0.75f, 1.0f, true);
	Rect(X, Y + S(26.0f), RW, S(1.0f), Th->BorderFaint);
}

void FGEUIKit::ProgressBar(float X, float Y, float PW, float PH, float Fraction, const FLinearColor& Color) const
{
	Fraction = FMath::Clamp(Fraction, 0.0f, 1.0f);
	Rect(X, Y, PW, PH, Th->BgSunken);
	Rect(X, Y, PW * Fraction, PH, Color);
	// segment ticks every 10%
	for (int32 i = 1; i < 10; i++)
	{
		Rect(X + PW * i / 10.0f, Y, S(1.0f), PH, WithAlpha(Th->BgSunken, 0.8f));
	}
	Rect(X, Y, PW, S(1.0f), Th->BorderFaint);
	Rect(X, Y + PH - S(1.0f), PW, S(1.0f), Th->BorderFaint);
}

float FGEUIKit::Toast(float X, float Y, float TW, EGEToastKind Kind, const FString& Title, const FString& Message) const
{
	const FLinearColor C = Kind == EGEToastKind::Success ? Th->StatusSuccess
		: Kind == EGEToastKind::Warning ? Th->StatusWarning
		: Kind == EGEToastKind::Danger ? Th->StatusDanger : Th->StatusInfo;
	const float TH = S(64.0f);
	ChamferFill(X, Y, TW, TH, S(10.0f), GECut::BR, Th->BgRaised);
	ChamferOutline(X, Y, TW, TH, S(10.0f), GECut::BR, WithAlpha(C, 0.45f), S(1.0f));
	Rect(X, Y, S(4.0f), TH, C);
	Text(Title.ToUpper(), X + S(18.0f), Y + S(9.0f), C, Type::LabelM);
	Text(Message, X + S(18.0f), Y + S(32.0f), Th->TextBody, Type::BodyS);
	return TH;
}

// =====================================================================
//  chrome and effects
// =====================================================================

void FGEUIKit::FrameLines() const
{
	const float M = S(40.0f), D = S(10.0f);
	// one line: straight, an angled step in/out, a straight run, step back, straight to the end
	auto DrawFrame = [&](float Y, float Dir, bool bBrightAtStart)
	{
		const float B1 = W * 0.62f, B2 = W * 0.74f;
		const FLinearColor C = WithAlpha(Th->AccentPrimary, 0.35f);
		const float T = S(1.0f);
		Line(FVector2D(M, Y), FVector2D(B1, Y), C, T);
		Line(FVector2D(B1, Y), FVector2D(B1 + D, Y + Dir * D), C, T);
		Line(FVector2D(B1 + D, Y + Dir * D), FVector2D(B2, Y + Dir * D), C, T);
		Line(FVector2D(B2, Y + Dir * D), FVector2D(B2 + D, Y), C, T);
		Line(FVector2D(B2 + D, Y), FVector2D(W - M, Y), C, T);
		// bright accent segment
		const float SegLen = S(180.0f);
		const float SegX = bBrightAtStart ? M : W - M - SegLen;
		Rect(SegX, Y - S(1.0f), SegLen, S(2.0f), Th->AccentPrimary);
	};
	DrawFrame(S(30.0f), +1.0f, true);
	DrawFrame(H - S(30.0f), -1.0f, false);
}

void FGEUIKit::CornerTags(const FString& LeftLabel, const FString& VersionTag) const
{
	const float M = S(40.0f), Y = S(44.0f);
	// small diamond icon
	const float r = S(6.0f);
	const FVector2D C(M + r, Y + S(9.0f));
	Tri(FVector2D(C.X, C.Y - r), FVector2D(C.X + r, C.Y), FVector2D(C.X, C.Y + r), Th->AccentPrimary);
	Tri(FVector2D(C.X, C.Y - r), FVector2D(C.X, C.Y + r), FVector2D(C.X - r, C.Y), Th->AccentPrimary);
	Text(LeftLabel.ToUpper(), M + r * 2.0f + S(10.0f), Y, Th->TextBody, Type::LabelS);
	Text(VersionTag.ToUpper(), W - M, Y, Th->TextMuted, Type::DataS, 1.0f, true);
}

void FGEUIKit::Fluff() const
{
	const float M = S(40.0f);
	const float BaseY = H - S(112.0f);

	// barcode: bars with deterministic pseudo-random widths (same every frame)
	float X = M;
	uint32 Seed = 0x9E3779B9u;
	for (int32 i = 0; i < 38; i++)
	{
		Seed = Seed * 1664525u + 1013904223u;
		const float BarW = S(1.0f + (Seed >> 24) % 3);
		const float Gap = S(1.0f + ((Seed >> 16) & 3));
		Rect(X, BaseY, BarW, S(16.0f), WithAlpha(Th->TextMuted, 0.8f));
		X += BarW + Gap;
	}
	Text(FString::Printf(TEXT("RAYSHOT build %d"), BuildNumber), M, BaseY + S(20.0f), Th->TextMuted, Type::DataS, 0.0f, true);
	Text(TEXT("RAY-SPHERE HIT ENGINE v1"), M, BaseY + S(36.0f), Th->TextMuted, Type::DataS, 0.0f, true);
	Text(TEXT("RAKSHIT SINGH"), M, BaseY + S(52.0f), Th->TextMuted, Type::DataS, 0.0f, true);
}

void FGEUIKit::Scanlines() const
{
	const float Step = S(4.0f), Thick = S(2.0f);
	const FLinearColor C(0.0f, 0.0f, 0.0f, 0.10f);
	for (float Y = 0.0f; Y < H; Y += Step)
	{
		Rect(0.0f, Y, W, Thick, C);
	}
}

void FGEUIKit::Glitch(float ScreenAge) const
{
	const float Duration = 0.15f;
	if (ScreenAge < 0.0f || ScreenAge > Duration)
	{
		return;
	}
	// a few horizontal bands, shifted left in the primary colour and right in the secondary colour
	FRandomStream Rng(1234 + (int32)(ScreenAge * 90.0f));
	const float Strength = 1.0f - ScreenAge / Duration;
	for (int32 i = 0; i < 7; i++)
	{
		const float BandY = Rng.FRand() * H;
		const float BandH = S(3.0f + Rng.FRand() * 26.0f);
		const float Off = S((Rng.FRand() - 0.5f) * 60.0f);
		const float BandX = Rng.FRand() * W * 0.5f;
		const float BandW = W * (0.25f + Rng.FRand() * 0.5f);
		Rect(BandX - Off, BandY, BandW, BandH, WithAlpha(Th->AccentPrimary, 0.14f * Strength));
		Rect(BandX + Off, BandY + S(1.0f), BandW, BandH, WithAlpha(Th->TextHighlight, 0.10f * Strength));
	}
	Rect(0.0f, 0.0f, W, H, WithAlpha(Th->AccentPrimary, 0.04f * Strength));
}

void FGEUIKit::ScreenChrome(const FString& LeftLabel) const
{
	FrameLines();
	CornerTags(LeftLabel);
	Fluff();
}

void FGEUIKit::Overlay(float ScreenAge) const
{
	if (Hud == nullptr)
	{
		return;
	}
	const UGESaveGame* Save = Hud->GetSave();
	if (Save == nullptr)
	{
		return;
	}
	if (Save->bScanlines)
	{
		Scanlines();
	}
	if (Save->bGlitch)
	{
		Glitch(ScreenAge);
	}
}

// =====================================================================
//  gallery
// =====================================================================

void FGEUIKit::DrawGallery(float Time) const
{
	if (Canvas == nullptr)
	{
		return;
	}
	Rect(0.0f, 0.0f, W, H, Th->BgBase);
	FrameLines();
	CornerTags(TEXT("UI GALLERY  //  ge.ui.gallery"), TEXT("B3"));
	Fluff();

	const float Left = S(80.0f);
	const FLinearColor Cap = Th->TextMuted;

	// ---- buttons: 3 styles x 4 states ----
	Text(TEXT("BUTTONS  (default / hover / pressed / disabled)"), Left, S(86.0f), Cap, Type::LabelS);
	const TCHAR* StyleNames[3] = { TEXT("Primary"), TEXT("Secondary"), TEXT("Ghost") };
	for (int32 s = 0; s < 3; s++)
	{
		for (int32 st = 0; st < 4; st++)
		{
			Button(Left + st * S(210.0f), S(110.0f) + s * S(66.0f), S(196.0f), S(50.0f), StyleNames[s],
				(EGEButtonStyle)s, (EGEButtonState)st);
		}
	}

	// ---- nav items ----
	Text(TEXT("NAV ITEMS  (default / hover / selected)"), Left, S(322.0f), Cap, Type::LabelS);
	const TCHAR* NavLabels[3] = { TEXT("Play"), TEXT("Training"), TEXT("Settings") };
	for (int32 i = 0; i < 3; i++)
	{
		NavItem(Left + i * S(280.0f), S(346.0f), S(264.0f), S(52.0f), i + 1, NavLabels[i], (EGENavState)i);
	}

	// ---- tabs ----
	Text(TEXT("HEADER TABS / TIER TABS / PAGE INDICATOR"), Left, S(426.0f), Cap, Type::LabelS);
	TArray<FString> Tabs = { TEXT("Overview"), TEXT("Stats"), TEXT("History") };
	HeaderTabs(Left, S(450.0f), Tabs, (int32)(Time * 0.7f) % 3);
	TierTabs(Left + S(420.0f), S(448.0f), (int32)(Time) % 4);
	PageIndicator(Left + S(760.0f), S(444.0f), 1 + (int32)(Time * 0.5f) % 4, 4);

	// ---- controls ----
	Text(TEXT("SLIDER / TOGGLE / CHECKBOX / PROGRESS / STATS"), Left, S(520.0f), Cap, Type::LabelS);
	const float Anim = 0.5f + 0.5f * FMath::Sin(Time);
	Slider(Left, S(548.0f), S(400.0f), TEXT("Master volume"), Anim, FString::Printf(TEXT("%03d"), FMath::RoundToInt(Anim * 100.0f)), true);
	Slider(Left + S(450.0f), S(548.0f), S(400.0f), TEXT("Sensitivity"), 0.3f, TEXT("0.3"), false);
	Toggle(Left, S(632.0f), S(400.0f), TEXT("Scanlines"), true, false);
	Toggle(Left + S(450.0f), S(632.0f), S(400.0f), TEXT("Glitch"), false, true);
	Checkbox(Left, S(686.0f), TEXT("Show hit markers"), true);
	Checkbox(Left + S(450.0f), S(686.0f), TEXT("Show timer"), false, true);
	ProgressBar(Left, S(740.0f), S(400.0f), S(14.0f), Anim, Th->AccentPrimary);
	ProgressBar(Left + S(450.0f), S(740.0f), S(400.0f), S(14.0f), 0.65f, Th->StatusSuccess);
	StatRow(Left, S(782.0f), S(400.0f), TEXT("Accuracy"), TEXT("87.5%"));
	StatRow(Left, S(816.0f), S(400.0f), TEXT("Best score"), TEXT("4210"), true);
	StatRow(Left + S(450.0f), S(782.0f), S(400.0f), TEXT("Hits"), TEXT("132"));
	StatRow(Left + S(450.0f), S(816.0f), S(400.0f), TEXT("Misses"), TEXT("19"));

	// ---- right column ----
	const float RX = S(1060.0f);
	Text(TEXT("DIAMOND TILES  (normal / selected)"), RX, S(86.0f), Cap, Type::LabelS);
	DiamondTile(RX + S(110.0f), S(210.0f), S(70.0f), TEXT("Gridshot"), false);
	DiamondTile(RX + S(340.0f), S(210.0f), S(70.0f), TEXT("Tracking"), true);
	DiamondTile(RX + S(570.0f), S(210.0f), S(70.0f), TEXT("Reflex"), false);

	Text(TEXT("PANEL"), RX, S(322.0f), Cap, Type::LabelS);
	Panel(RX, S(346.0f), S(300.0f), S(90.0f), 12.0f, GECut::TR | GECut::BR, true);
	Text(TEXT("PANEL WITH CUTS"), RX + S(20.0f), S(380.0f), Th->TextBody, Type::LabelM);

	Text(TEXT("TOASTS  (info / success / warning / danger)"), RX, S(458.0f), Cap, Type::LabelS);
	const EGEToastKind Kinds[4] = { EGEToastKind::Info, EGEToastKind::Success, EGEToastKind::Warning, EGEToastKind::Danger };
	const TCHAR* Titles[4] = { TEXT("Info"), TEXT("New best"), TEXT("Warning"), TEXT("Error") };
	const TCHAR* Msgs[4] = { TEXT("Training session saved."), TEXT("Gridshot: 4210"), TEXT("FOV is very high."), TEXT("Could not save profile.") };
	for (int32 i = 0; i < 4; i++)
	{
		Toast(RX, S(482.0f) + i * S(74.0f), S(520.0f), Kinds[i], Titles[i], Msgs[i]);
	}

	Text(TEXT("GLOW TITLE"), RX, S(790.0f), Cap, Type::LabelS);
	GlowText(TEXT("RAYSHOT"), RX, S(808.0f), Th->TextStrong, Th->AccentPrimary, Type::DisplayL);

	// glitch demo: replays every 3 seconds; scanlines always on in the gallery
	Scanlines();
	Glitch(FMath::Fmod(Time, 3.0f));
}
