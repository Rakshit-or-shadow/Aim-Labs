// RayShotModes.h
// Helper functions about the modes (names, colors, descriptions, score text).
// Kept separate from RayShotTypes.h so Unreal's code generator only sees the enums.
#pragma once

#include "CoreMinimal.h"
#include "RayShotTypes.h"
#include "RayShotStyle.h"

namespace RayShotModes
{
    constexpr int32 NumModes = 4;

    inline FString GetName(ERayShotMode Mode)
    {
        switch (Mode)
        {
        case ERayShotMode::Gridshot:  return TEXT("Gridshot");
        case ERayShotMode::Microshot: return TEXT("Microshot");
        case ERayShotMode::Tracking:  return TEXT("Tracking");
        case ERayShotMode::Reflex:    return TEXT("Reflex");
        }
        return TEXT("Unknown");
    }

    // accent color used for the menu, HUD and targets of each mode
    inline FLinearColor GetColor(ERayShotMode Mode)
    {
        switch (Mode)
        {
        case ERayShotMode::Gridshot:  return RayShotStyle::Cyan;
        case ERayShotMode::Microshot: return RayShotStyle::Orange;
        case ERayShotMode::Tracking:  return RayShotStyle::Green;
        case ERayShotMode::Reflex:    return RayShotStyle::Purple;
        }
        return RayShotStyle::Cyan;
    }

    // short description lines shown in the main menu
    inline TArray<FString> GetDescription(ERayShotMode Mode)
    {
        TArray<FString> Lines;
        switch (Mode)
        {
        case ERayShotMode::Gridshot:
            Lines.Add(TEXT("Three targets on a 5x5 grid."));
            Lines.Add(TEXT("Click them as fast as you can."));
            Lines.Add(TEXT("Trains: speed + target switching"));
            break;
        case ERayShotMode::Microshot:
            Lines.Add(TEXT("One tiny target in random spots."));
            Lines.Add(TEXT("Every click has to be precise."));
            Lines.Add(TEXT("Trains: precision"));
            break;
        case ERayShotMode::Tracking:
            Lines.Add(TEXT("A target that strafes unpredictably."));
            Lines.Add(TEXT("Keep your crosshair glued to it."));
            Lines.Add(TEXT("Trains: smooth tracking"));
            break;
        case ERayShotMode::Reflex:
            Lines.Add(TEXT("Wait for the target to appear,"));
            Lines.Add(TEXT("then flick to it instantly."));
            Lines.Add(TEXT("Trains: reaction time"));
            break;
        }
        return Lines;
    }

    inline FString GetDurationText(ERayShotMode Mode)
    {
        switch (Mode)
        {
        case ERayShotMode::Tracking: return TEXT("45 seconds");
        case ERayShotMode::Reflex:   return TEXT("10 targets");
        default:                     return TEXT("60 seconds");
        }
    }

    // for Reflex a SMALLER score (reaction time) is better
    inline bool LowerIsBetter(ERayShotMode Mode)
    {
        return Mode == ERayShotMode::Reflex;
    }

    // just the number part of a score, e.g. "54" or "62.3"
    inline FString FormatScoreNumber(ERayShotMode Mode, float Score)
    {
        switch (Mode)
        {
        case ERayShotMode::Tracking: return FString::Printf(TEXT("%.1f"), Score);
        case ERayShotMode::Reflex:   return FString::Printf(TEXT("%.0f"), Score);
        default:                     return FString::Printf(TEXT("%d"), FMath::RoundToInt(Score));
        }
    }

    // the unit that goes after the number
    inline FString GetScoreUnit(ERayShotMode Mode)
    {
        switch (Mode)
        {
        case ERayShotMode::Tracking: return TEXT("% on target");
        case ERayShotMode::Reflex:   return TEXT("ms average");
        default:                     return TEXT("hits");
        }
    }

    // a short version, e.g. "54 hits" or "231 ms"
    inline FString FormatScore(ERayShotMode Mode, float Score)
    {
        switch (Mode)
        {
        case ERayShotMode::Tracking: return FString::Printf(TEXT("%.1f%%"), Score);
        case ERayShotMode::Reflex:   return FString::Printf(TEXT("%.0f ms"), Score);
        default:                     return FString::Printf(TEXT("%d hits"), FMath::RoundToInt(Score));
        }
    }
}
