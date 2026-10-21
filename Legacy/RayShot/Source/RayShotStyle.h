// RayShotStyle.h
// All the colors and built-in materials the game uses, in one place.
// Change a color here and it changes everywhere.
#pragma once

#include "CoreMinimal.h"

namespace RayShotStyle
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

    // ---------- built-in engine materials ----------
    // lit material with a "Color" parameter (used for solid shapes)
    inline const TCHAR* LitMaterialPath = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
    // unlit (glowing) material with a "Color" parameter; colors above 1.0 glow (bloom)
    inline const TCHAR* GlowMaterialPath = TEXT("/Engine/EngineDebugMaterials/LevelColorationUnlitMaterial.LevelColorationUnlitMaterial");
    inline const TCHAR* SphereMeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
    inline const TCHAR* CubeMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");

    // ---------- settings screen ----------
    constexpr int32 NumSettings = 4; // Sensitivity, Field of View, Volume, Crosshair

    // ---------- crosshair colors (chosen in Settings) ----------
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

    // makes a color brighter than 1.0 so it glows with bloom
    inline FLinearColor Glow(const FLinearColor& Color, float Strength)
    {
        return FLinearColor(Color.R * Strength, Color.G * Strength, Color.B * Strength, 1.0f);
    }

    // same color, different transparency
    inline FLinearColor WithAlpha(const FLinearColor& Color, float Alpha)
    {
        return FLinearColor(Color.R, Color.G, Color.B, Alpha);
    }
}
