// GEAimModes.h
// Helper functions about the aim modes (names, colors, descriptions, score text).
#pragma once

#include "CoreMinimal.h"
#include "GEAimTypes.h"
#include "GEStyle.h"

namespace GEAimModes
{
	constexpr int32 NumModes = 4;

	/** Mode name, also used as the key in the save game's best-score map. */
	inline FString GetName(EGEAimMode Mode)
	{
		switch (Mode)
		{
		case EGEAimMode::Gridshot:  return TEXT("Gridshot");
		case EGEAimMode::Microshot: return TEXT("Microshot");
		case EGEAimMode::Tracking:  return TEXT("Tracking");
		case EGEAimMode::Reflex:    return TEXT("Reflex");
		}
		return TEXT("Unknown");
	}

	/** Accent color used for the menu, HUD and targets of each mode. */
	inline FLinearColor GetColor(EGEAimMode Mode)
	{
		switch (Mode)
		{
		case EGEAimMode::Gridshot:  return GEStyle::Cyan;
		case EGEAimMode::Microshot: return GEStyle::Orange;
		case EGEAimMode::Tracking:  return GEStyle::Green;
		case EGEAimMode::Reflex:    return GEStyle::Purple;
		}
		return GEStyle::Cyan;
	}

	/** Short description lines shown in the main menu. */
	inline TArray<FString> GetDescription(EGEAimMode Mode)
	{
		TArray<FString> Lines;
		switch (Mode)
		{
		case EGEAimMode::Gridshot:
			Lines.Add(TEXT("Three targets on a 5x5 grid."));
			Lines.Add(TEXT("Click them as fast as you can."));
			Lines.Add(TEXT("Trains: speed + target switching"));
			break;
		case EGEAimMode::Microshot:
			Lines.Add(TEXT("One tiny target in random spots."));
			Lines.Add(TEXT("Every click has to be precise."));
			Lines.Add(TEXT("Trains: precision"));
			break;
		case EGEAimMode::Tracking:
			Lines.Add(TEXT("A target that strafes unpredictably."));
			Lines.Add(TEXT("Keep your crosshair glued to it."));
			Lines.Add(TEXT("Trains: smooth tracking"));
			break;
		case EGEAimMode::Reflex:
			Lines.Add(TEXT("Wait for the target to appear,"));
			Lines.Add(TEXT("then flick to it instantly."));
			Lines.Add(TEXT("Trains: reaction time"));
			break;
		}
		return Lines;
	}

	inline FString GetDurationText(EGEAimMode Mode)
	{
		switch (Mode)
		{
		case EGEAimMode::Tracking: return TEXT("45 seconds");
		case EGEAimMode::Reflex:   return TEXT("10 targets");
		default:                   return TEXT("60 seconds");
		}
	}

	/** For Reflex a SMALLER score (reaction time) is better. */
	inline bool LowerIsBetter(EGEAimMode Mode)
	{
		return Mode == EGEAimMode::Reflex;
	}

	/** Just the number part of a score, e.g. "54" or "62.3". */
	inline FString FormatScoreNumber(EGEAimMode Mode, float Score)
	{
		switch (Mode)
		{
		case EGEAimMode::Tracking: return FString::Printf(TEXT("%.1f"), Score);
		case EGEAimMode::Reflex:   return FString::Printf(TEXT("%.0f"), Score);
		default:                   return FString::Printf(TEXT("%d"), FMath::RoundToInt(Score));
		}
	}

	/** The unit that goes after the number. */
	inline FString GetScoreUnit(EGEAimMode Mode)
	{
		switch (Mode)
		{
		case EGEAimMode::Tracking: return TEXT("% on target");
		case EGEAimMode::Reflex:   return TEXT("ms average");
		default:                   return TEXT("hits");
		}
	}

	/** A short version, e.g. "54 hits" or "231 ms". */
	inline FString FormatScore(EGEAimMode Mode, float Score)
	{
		switch (Mode)
		{
		case EGEAimMode::Tracking: return FString::Printf(TEXT("%.1f%%"), Score);
		case EGEAimMode::Reflex:   return FString::Printf(TEXT("%.0f ms"), Score);
		default:                   return FString::Printf(TEXT("%d hits"), FMath::RoundToInt(Score));
		}
	}
}
