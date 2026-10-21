// GEUIKit.h
// Stage B3: the procedural UI component library. Everything is drawn with the HUD canvas
// (rects, triangles, lines and text), nothing is a texture. Colors come from GEStyle themes,
// fonts from the HUD (Rajdhani / Share Tech Mono).
//
// HOW TO USE (from inside an AGEHUD draw function):
//     FGEUIKit Kit(this);
//     Kit.Button(X, Y, W, H, TEXT("Start"), EGEButtonStyle::Primary, EGEButtonState::Hover);
//
// CONVENTIONS
//   - Positions and sizes you pass in (X, Y, W, H) are REAL screen pixels (like the rest of the HUD).
//   - Fixed sizes inside the kit (chamfer cuts, strokes, text) are 1080p design values that the kit
//     scales by screen height / 1080 itself.
//   - Text sizes are 1080p design pixels (GEStyle::Type::*), exactly like AGEHUD::DrawLabel.
//   - The kit only DRAWS. Hit boxes / hover detection stay with the caller (state is passed in).
#pragma once

#include "CoreMinimal.h"
#include "GEStyle.h"

class AGEHUD;
class UCanvas;

/** Which corners of a panel/button are cut. Combine with |. */
namespace GECut
{
	constexpr uint8 None = 0, TL = 1, TR = 2, BR = 4, BL = 8;
}

enum class EGEButtonStyle : uint8 { Primary, Secondary, Ghost };
enum class EGEButtonState : uint8 { Default, Hover, Pressed, Disabled };
enum class EGENavState : uint8 { Default, Hover, Selected };
enum class EGEToastKind : uint8 { Info, Success, Warning, Danger };

/** Short-lived drawing context. Make one per draw function: FGEUIKit Kit(this); */
struct GAMEENGINE_API FGEUIKit
{
	/** Build number printed in the bottom-left "fluff" text. */
	static constexpr int32 BuildNumber = 1021;

	explicit FGEUIKit(AGEHUD* InHud);

	// ------------------------------------------------------------------ primitives
	/** Scales a 1080p design size to this screen. */
	float S(float DesignPx) const { return DesignPx * Scale; }
	/** Filled rectangle (real pixels). */
	void Rect(float X, float Y, float W, float H, const FLinearColor& Color) const;
	/** Filled triangle. */
	void Tri(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Color) const;
	/** Line with thickness in real pixels. */
	void Line(const FVector2D& A, const FVector2D& B, const FLinearColor& Color, float Thickness) const;
	/** Corner points (clockwise from top-left) of a rectangle whose chosen corners are cut by Cut pixels. */
	static void ShapePoints(float X, float Y, float W, float H, float Cut, uint8 CutMask, TArray<FVector2D>& Out);
	/** Filled chamfered rectangle (a real cut shape made of triangles). Cut is in real pixels. */
	void ChamferFill(float X, float Y, float W, float H, float Cut, uint8 CutMask, const FLinearColor& Color) const;
	/** Outline of the same shape. */
	void ChamferOutline(float X, float Y, float W, float H, float Cut, uint8 CutMask, const FLinearColor& Color, float Thickness) const;
	/** Soft glow around a rectangle: stacked translucent frames fading outwards. */
	void GlowRect(float X, float Y, float W, float H, const FLinearColor& Color, float Spread) const;
	/** Text. Size = 1080p design px, AlignX 0 left / 0.5 centre / 1 right. Y is the top of the text. */
	void Text(const FString& Str, float X, float Y, const FLinearColor& Color, float Size, float AlignX = 0.0f, bool bMono = false) const;
	/** Same, but vertically centred in the band [Y, Y+H]. */
	void TextCentered(const FString& Str, float X, float Y, float H, const FLinearColor& Color, float Size, float AlignX = 0.0f, bool bMono = false) const;
	/** Big-title text with a soft glow (the text drawn several times with offsets at low alpha). */
	void GlowText(const FString& Str, float X, float Y, const FLinearColor& Color, const FLinearColor& Glow, float Size, float AlignX = 0.0f, bool bMono = false) const;
	/** Width/height in real pixels. */
	FVector2D Measure(const FString& Str, float Size, bool bMono = false) const;

	// ------------------------------------------------------------------ panels and buttons
	/** Dark panel with a hairline border and a chamfer (default: top-right + bottom-right cut). */
	void Panel(float X, float Y, float W, float H, float CutDesign = 10.0f, uint8 CutMask = GECut::BR, bool bAccentBorder = false) const;
	/** Action button (bottom-right cut) with optional trailing ">" chevron. */
	void Button(float X, float Y, float W, float H, const FString& Label, EGEButtonStyle Style, EGEButtonState State, bool bChevron = true) const;
	/** Nav item "01 LABEL" (top-right cut): outline -> tint -> solid selected with glow. */
	void NavItem(float X, float Y, float W, float H, int32 Index, const FString& Label, EGENavState State) const;
	/** Header tabs in a row with an underline under the active one. Returns the total width drawn. */
	float HeaderTabs(float X, float Y, const TArray<FString>& Labels, int32 Active) const;
	/** T1..T4 tier tabs. Returns the total width drawn. */
	float TierTabs(float X, float Y, int32 Active, int32 Count = 4) const;
	/** "01 / 04 PAGE" */
	void PageIndicator(float X, float Y, int32 Current, int32 Total, const FString& Word = TEXT("PAGE")) const;
	/** Diamond category tile centred on (CX, CY) with Radius (real px), label below. */
	void DiamondTile(float CX, float CY, float Radius, const FString& Label, bool bSelected) const;

	// ------------------------------------------------------------------ controls
	/** "MASTER VOLUME            075" label row + track + vertical thumb. Fraction 0..1. Needs about 44 design px of height. */
	void Slider(float X, float Y, float W, const FString& Label, float Fraction, const FString& ValueText, bool bFocused = false) const;
	/** ON / OFF toggle with a label to its left (label starts at X, switch is right-aligned to X+W). */
	void Toggle(float X, float Y, float W, const FString& Label, bool bOn, bool bFocused = false) const;
	void Checkbox(float X, float Y, const FString& Label, bool bChecked, bool bFocused = false) const;
	/** Label on the left (muted), mono value on the right, hairline underneath. */
	void StatRow(float X, float Y, float W, const FString& Label, const FString& Value, bool bHighlight = false) const;
	/** Segmented progress bar. */
	void ProgressBar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Color) const;
	/** Toast with a coloured stripe. Height is returned (real px) so you can stack them. */
	float Toast(float X, float Y, float W, EGEToastKind Kind, const FString& Title, const FString& Message) const;

	// ------------------------------------------------------------------ screen chrome and effects
	/** Hairline frame lines across the top and bottom of the screen with angled breaks and a bright accent segment. */
	void FrameLines() const;
	/** Top-left tag (small diamond + label) and top-right version tag. */
	void CornerTags(const FString& LeftLabel, const FString& VersionTag = TEXT("V1.0")) const;
	/** Bottom-left micro text + barcode (original text). */
	void Fluff() const;
	/** Faint 2px scanline overlay across the whole screen. */
	void Scanlines() const;
	/** Short colour-split flash. Pass the seconds since the screen changed; draws nothing after 0.15s. */
	void Glitch(float ScreenAge) const;
	/** Everything a normal screen wants: frame lines + corner tags + fluff. */
	void ScreenChrome(const FString& LeftLabel) const;
	/** Always-on final overlay (scanlines/glitch if enabled in settings). Called by AGEHUD::DrawHUD. */
	void Overlay(float ScreenAge) const;

	// ------------------------------------------------------------------ debug gallery
	/** Draws every component in every state (console: ge.ui.gallery). */
	void DrawGallery(float TimeSeconds) const;
	static bool& GalleryOn();

private:
	AGEHUD* Hud = nullptr;
	UCanvas* Canvas = nullptr;
	float Scale = 1.0f;
	float W = 0.0f, H = 0.0f;
	const GEStyle::FGETheme* Th = nullptr;
};
