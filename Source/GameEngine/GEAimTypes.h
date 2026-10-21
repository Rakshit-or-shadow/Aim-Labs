// GEAimTypes.h
// Small shared types for the aim trainer: the four modes and the menu screens,
// plus two plain structs the HUD reads (a stat row and a floating score text).
#pragma once

#include "CoreMinimal.h"
#include "GEAimTypes.generated.h"

/** The four aim training modes. */
UENUM(BlueprintType)
enum class EGEAimMode : uint8
{
	Gridshot,   // 3 targets on a grid, click as many as you can
	Microshot,  // 1 tiny target, precision clicking
	Tracking,   // 1 moving target, keep your crosshair on it
	Reflex      // 1 target appears after a random delay, click it fast
};

/** Which aim-trainer screen the player is looking at (used by the menus in a later stage). */
UENUM(BlueprintType)
enum class EGEScreen : uint8
{
	MainMenu,
	Settings,
	Playing,
	Results
};

/** One row of stats, e.g. Label = "Accuracy", Value = "93.5%". */
struct FGEStatLine
{
	FString Label;
	FString Value;
};

/** Floating text that appears where you hit a target (e.g. "+1" or "231 ms"). */
struct FGEScorePopup
{
	FVector Location = FVector::ZeroVector;
	float SpawnTime = 0.0f;
	FString Text;
	FLinearColor Color = FLinearColor::White;
};
