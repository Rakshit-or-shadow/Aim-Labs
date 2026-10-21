// GEStyle.h
// All the colors and built-in engine assets the aim trainer uses, in one place.
#pragma once

#include "CoreMinimal.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Fonts/CompositeFont.h"
#include "UObject/UObjectGlobals.h"

namespace GEStyle
{
	// ---------- accent colors ----------
	inline const FLinearColor Cyan(0.0f, 0.85f, 1.0f);
	inline const FLinearColor Magenta(1.0f, 0.15f, 0.55f);
	inline const FLinearColor Orange(1.0f, 0.5f, 0.05f);
	inline const FLinearColor Green(0.15f, 1.0f, 0.4f);
	inline const FLinearColor Purple(0.6f, 0.3f, 1.0f);
	inline const FLinearColor Gold(1.0f, 0.78f, 0.2f);
	inline const FLinearColor Red(1.0f, 0.25f, 0.25f);

	// ---------- UI colors ----------
	inline const FLinearColor TextWhite(1.0f, 1.0f, 1.0f);
	inline const FLinearColor TextDim(0.62f, 0.66f, 0.74f);
	inline const FLinearColor PanelFill(0.02f, 0.025f, 0.045f, 0.86f);
	inline const FLinearColor PanelBorder(1.0f, 1.0f, 1.0f, 0.08f);
	inline const FLinearColor ButtonFill(0.06f, 0.07f, 0.1f, 0.78f);

	// ---------- arena colors (lit surfaces) ----------
	inline const FLinearColor WallColor(0.035f, 0.04f, 0.055f);
	inline const FLinearColor FloorColor(0.03f, 0.033f, 0.045f);
	inline const FLinearColor BackWallColor(0.045f, 0.05f, 0.07f);

	// ---------- built-in engine assets ----------
	/** Lit material with a "Color" parameter (solid shapes). */
	inline const TCHAR* LitMaterialPath = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
	/** Unlit (glowing) material with a "Color" parameter; values above 1.0 bloom. */
	inline const TCHAR* GlowMaterialPath = TEXT("/Engine/EngineDebugMaterials/LevelColorationUnlitMaterial.LevelColorationUnlitMaterial");
	inline const TCHAR* SphereMeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	inline const TCHAR* CubeMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");

	// ---------- settings screen ----------
	constexpr int32 NumSettings = 7; // Sensitivity, Field of View, Volume, Crosshair, UI Theme, Scanlines, Glitch

	// ---------- crosshair colors (index is saved in UGESaveGame::CrosshairColor) ----------
	constexpr int32 NumCrosshairColors = 5;

	inline FLinearColor GetCrosshairColor(int32 Index)
	{
		switch (Index)
		{
		case 1:  return Cyan;
		case 2:  return TextWhite;
		case 3:  return Magenta;
		case 4:  return Gold;
		default: return Green;
		}
	}

	inline FString GetCrosshairColorName(int32 Index)
	{
		switch (Index)
		{
		case 1:  return TEXT("Cyan");
		case 2:  return TEXT("White");
		case 3:  return TEXT("Pink");
		case 4:  return TEXT("Gold");
		default: return TEXT("Green");
		}
	}

	/** Makes a color brighter than 1.0 so it glows with bloom. */
	inline FLinearColor Glow(const FLinearColor& Color, float Strength)
	{
		return FLinearColor(Color.R * Strength, Color.G * Strength, Color.B * Strength, 1.0f);
	}

	/** Same color, different transparency. */
	inline FLinearColor WithAlpha(const FLinearColor& Color, float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Alpha);
	}

	// =====================================================================
	//  UI THEME SYSTEM (kit tokens from docs/UI_SPEC.md; hex values are estimates)
	// =====================================================================

	/** The four kit themes. RED = default UI, CYAN = info/secondary, YELLOW = bests/rewards/warnings, GREEN = success. */
	enum class EGETheme : uint8 { Red = 0, Cyan, Yellow, Green };
	constexpr int32 NumThemes = 4;

	/** One color per kit token ("accent/primary" -> AccentPrimary, ...). */
	struct FGETheme
	{
		FLinearColor AccentPrimary, AccentStrong, AccentDim, AccentSecondary;
		FLinearColor BgBase, BgRaised, BgSunken, BgTint, BgAccentSubtle, BgAccentHover, BgScrim;
		FLinearColor TextPrimary, TextStrong, TextHighlight, TextBody, TextMuted, TextOnAccent;
		FLinearColor BorderDefault, BorderSubtle, BorderFaint, BorderHighlight;
		FLinearColor StatusDanger, StatusSuccess, StatusWarning, StatusInfo;
		FLinearColor EffectGlow;
	};

	/** 0xRRGGBB to a canvas color. Canvas colors are used as-is, so there is no gamma conversion. */
	inline FLinearColor Hex(uint32 Rgb, float Alpha = 1.0f)
	{
		return FLinearColor(((Rgb >> 16) & 0xFF) / 255.0f, ((Rgb >> 8) & 0xFF) / 255.0f, (Rgb & 0xFF) / 255.0f, Alpha);
	}

	/** Builds a theme from its accent-dependent colors; everything else is shared by all themes. */
	inline FGETheme MakeTheme(uint32 Primary, uint32 Strong, uint32 Dim, uint32 Secondary, uint32 Tint)
	{
		FGETheme T;
		T.AccentPrimary = Hex(Primary);
		T.AccentStrong = Hex(Strong);
		T.AccentDim = Hex(Dim);
		T.AccentSecondary = Hex(Secondary);
		T.BgBase = Hex(0x0C0D14);
		T.BgRaised = Hex(0x121218);
		T.BgSunken = Hex(0x050505);
		T.BgTint = Hex(Tint);
		T.BgAccentSubtle = Hex(Primary, 0.12f);
		T.BgAccentHover = Hex(Primary, 0.30f);
		T.BgScrim = Hex(0x192838, 0.80f);
		T.TextPrimary = Hex(Primary);
		T.TextStrong = Hex(Strong);
		T.TextHighlight = Hex(Secondary);
		T.TextBody = Hex(0xB0C8C9);
		T.TextMuted = Hex(0x747474);
		T.TextOnAccent = Hex(0x0E0E17);
		T.BorderDefault = Hex(Primary);
		T.BorderSubtle = Hex(Primary, 0.30f);
		T.BorderFaint = Hex(0x222325);
		T.BorderHighlight = Hex(Secondary);
		T.StatusDanger = Hex(0xF75049);
		T.StatusSuccess = Hex(0x1DE083);
		T.StatusWarning = Hex(0xFBC706);
		T.StatusInfo = Hex(0x5EFEFF);
		T.EffectGlow = Hex(Primary, 0.40f);
		return T;
	}

	/** The theme table (built once). */
	inline const FGETheme& GetTheme(EGETheme Id)
	{
		static const FGETheme Themes[NumThemes] = {
			MakeTheme(0xF75049, 0xFF8A84, 0x8E2A26, 0x5EFEFF, 0x2A1214), // Red
			MakeTheme(0x5EFEFF, 0xB3FBFF, 0x1E7F86, 0xFBC706, 0x0C2629), // Cyan
			MakeTheme(0xFBC706, 0xFFC672, 0x8A6A0C, 0x5EFEFF, 0x2A2204), // Yellow
			MakeTheme(0x1DE083, 0xAED5B2, 0x127A47, 0xFBC706, 0x082318), // Green
		};
		return Themes[FMath::Clamp((int32)Id, 0, NumThemes - 1)];
	}

	inline FString GetThemeName(int32 Index)
	{
		switch (Index)
		{
		case 1:  return TEXT("Cyan");
		case 2:  return TEXT("Yellow");
		case 3:  return TEXT("Green");
		default: return TEXT("Red");
		}
	}

	/** The theme the whole UI is using right now (set from the saved UI Theme setting, so it applies live). */
	inline EGETheme& ActiveThemeId() { static EGETheme Id = EGETheme::Red; return Id; }
	inline void SetActiveTheme(int32 Index) { ActiveThemeId() = (EGETheme)FMath::Clamp(Index, 0, NumThemes - 1); }
	/** Tokens of the active theme: GEStyle::Active().AccentPrimary etc. */
	inline const FGETheme& Active() { return GetTheme(ActiveThemeId()); }

	// ---------- type scale (pixels at 1080p; scale with screen height / 1080) ----------
	namespace Type
	{
		constexpr float DisplayXL = 84.0f, DisplayL = 60.0f;
		constexpr float H1 = 48.0f, H2 = 32.0f, H3 = 24.0f, Title = 20.0f;
		constexpr float BodyL = 18.0f, BodyM = 16.0f, BodyS = 14.0f;
		constexpr float LabelL = 16.0f, LabelM = 14.0f, LabelS = 12.0f; // SemiBold, UPPERCASE
		constexpr float DataL = 24.0f, DataM = 14.0f, DataS = 12.0f;    // Share Tech Mono
	}

	// ---------- spacing (4px grid) and strokes (px at 1080p) ----------
	namespace Space
	{
		constexpr float XXS = 2.0f, XS = 4.0f, SM = 8.0f, MD = 12.0f, LG = 16.0f, XL = 24.0f, XXL = 32.0f, XXXL = 48.0f, XXXXL = 64.0f;
	}
	namespace Stroke
	{
		constexpr float Hairline = 0.5f, Thin = 1.0f, Focus = 2.0f;
	}
	/** Scales a 1080p design size to the real screen height. */
	inline float Scaled(float DesignPx, float ScreenHeight) { return DesignPx * ScreenHeight / 1080.0f; }

	// ---------- fonts ----------
	/** Imported FontFace assets (made by Tools/import_ui.py). */
	inline const TCHAR* FontPathMedium = TEXT("/Game/UI/Fonts/FF_Rajdhani_Medium.FF_Rajdhani_Medium");
	inline const TCHAR* FontPathSemiBold = TEXT("/Game/UI/Fonts/FF_Rajdhani_SemiBold.FF_Rajdhani_SemiBold");
	inline const TCHAR* FontPathBold = TEXT("/Game/UI/Fonts/FF_Rajdhani_Bold.FF_Rajdhani_Bold");
	inline const TCHAR* FontPathMono = TEXT("/Game/UI/Fonts/FF_ShareTechMono_Regular.FF_ShareTechMono_Regular");

	/**
	 * Makes a runtime UFont out of an imported FontFace asset (no UFont asset needed).
	 * Returns nullptr if the FontFace asset is missing, so the caller can use a fallback.
	 */
	inline UFont* MakeRuntimeFont(UObject* Outer, const TCHAR* FaceAssetPath)
	{
		const UFontFace* Face = LoadObject<UFontFace>(nullptr, FaceAssetPath);
		if (Face == nullptr)
		{
			return nullptr;
		}
		UFont* Font = NewObject<UFont>(Outer);
		Font->FontCacheType = EFontCacheType::Runtime;
		FTypefaceEntry Entry(FName(TEXT("Default")));
		Entry.Font = FFontData(Face);
		Font->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Add(Entry);
		return Font;
	}
}
