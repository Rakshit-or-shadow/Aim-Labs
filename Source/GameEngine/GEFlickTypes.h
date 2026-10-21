#pragma once

#include "CoreMinimal.h"
#include "GEFlickTypes.generated.h"

/** The 8 directions a flick can go (from where the crosshair was to where the target appeared). Order = clockwise from Up. */
UENUM(BlueprintType)
enum class EGEFlickDirection : uint8
{
	Up,
	UpRight,
	Right,
	DownRight,
	Down,
	DownLeft,
	Left,
	UpLeft
};

/** How many flick directions exist. */
constexpr int32 GENumFlickDirections = 8;

/** Readable name of a direction ("Up-Right" ...). */
inline FString GEFlickDirectionToString(EGEFlickDirection Dir)
{
	static const TCHAR* Names[GENumFlickDirections] = { TEXT("Up"), TEXT("Up-Right"), TEXT("Right"), TEXT("Down-Right"), TEXT("Down"), TEXT("Down-Left"), TEXT("Left"), TEXT("Up-Left") };
	return Names[FMath::Clamp((int32)Dir, 0, GENumFlickDirections - 1)];
}

/** Sensitivity constants (Phase 4). */
namespace GESensitivity
{
	/** ASSUMED mouse DPI. Editable in the menu (saved). */
	constexpr int32 DefaultMouseDpi = 800;
	/** ASSUMED degrees the view turns per mouse count at sensitivity scale 1.0 (the BP look path is unknown, so this is a guess). Only affects the displayed cm/360 number, not the feel. */
	constexpr float BaseDegreesPerCount = 0.07f;
	constexpr float MinScale = 0.1f;
	constexpr float MaxScale = 5.0f;
	/** Each +/- click multiplies / divides the scale by this. */
	constexpr float ButtonStep = 1.05f;
	constexpr int32 DpiStep = 100;
	constexpr int32 MinDpi = 100;
	constexpr int32 MaxDpi = 32000;
}

/** Tunables for flick mode. Units: cm, seconds, degrees. */
struct FGEFlickSettings
{
	/** Targets appear between these distances from the player (cm). */
	float MinDistance = 1000.0f;
	float MaxDistance = 2500.0f;
	/** Targets appear within +/- this many degrees of the start direction. */
	float MaxAzimuthDeg = 75.0f;
	/** Chance a target floats above the ground, and the max height (cm), for pitch variety. */
	float RaiseChance = 0.5f;
	float MaxRaise = 120.0f;
	/** A target that is not killed within this time counts as a miss. */
	float TargetTimeout = 3.0f;
	/** Pause between a target ending and the next appearing. */
	float NextTargetDelay = 0.4f;
	/** Height above the feet that the crosshair angle is measured to (chest). */
	float AimHeight = 140.0f;
	/** Training Center floor limits in the center's local space (cm). */
	float AreaHalfWidth = 1300.0f;
	float AreaMinX = -800.0f;
	float AreaMaxX = 6800.0f;

	/** Flicks smaller than this (degrees) are ignored for overshoot. */
	float MinAngleForOvershoot = 5.0f;
	/** Overshoot within +/- this percent counts as "on target". */
	float OvershootDeadbandPercent = 3.0f;
	/** A path reversal bigger than this (degrees) counts as a correction. */
	float CorrectionHysteresisDeg = 1.0f;
	/** Suggestion needs at least this many valid flicks, this average overshoot (percent) and this share of flicks pointing the same way. */
	int32 MinSamplesForSuggestion = 5;
	float MinAvgOvershootForSuggestion = 5.0f;
	float MinConsistency = 0.6f;
	/** Largest suggested sensitivity change (percent). */
	float MaxSuggestedChangePercent = 30.0f;
};

/** One mouse-path sample: time since the target appeared and the view change since then (degrees). */
struct FGEPathSample
{
	float Time = 0.0f;
	float Yaw = 0.0f;
	float Pitch = 0.0f;
};

/** Everything recorded for one target. */
struct FGEFlickTargetRecord
{
	/** Angle between crosshair and target at appearance (degrees), and its yaw / pitch parts. */
	float InitialAngle = 0.0f;
	float InitialYaw = 0.0f;
	float InitialPitch = 0.0f;
	EGEFlickDirection Direction = EGEFlickDirection::Up;
	/** Seconds after appearing, or -1 if it never happened. */
	float TimeToFirstShot = -1.0f;
	float TimeToHit = -1.0f;
	bool bHit = false;
	bool bTimedOut = false;
	/** Signed percent of the initial angle: + = went past the target, - = stopped short. Valid only if bOvershootValid. */
	float OvershootPercent = 0.0f;
	bool bOvershootValid = false;
	/** Direction changes along the flick axis. */
	int32 Corrections = 0;
	/** The mouse path, one sample per frame. */
	TArray<FGEPathSample> Path;
};

/** Results for one of the 8 directions. */
USTRUCT(BlueprintType)
struct FGEFlickDirStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	int32 Targets = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	int32 Hits = 0;
	/** Sum of time-to-hit of the hit targets (seconds). */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	float TotalHitTime = 0.0f;

	/** 0-100 */
	float GetAccuracyPercent() const { return Targets > 0 ? 100.0f * Hits / Targets : 0.0f; }
	/** Seconds, 0 if nothing was hit. */
	float GetAverageTime() const { return Hits > 0 ? TotalHitTime / Hits : 0.0f; }
};

/** Result of a whole flick session. */
USTRUCT(BlueprintType)
struct FGEFlickSummary
{
	GENERATED_BODY()

	/** Always GENumFlickDirections entries (index = EGEFlickDirection). */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	TArray<FGEFlickDirStats> Directions;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	int32 TotalTargets = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	int32 TotalHits = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	float AverageTimeToHit = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	float AverageTimeToFirstShot = 0.0f;
	/** Signed average overshoot (percent of the flick). + = overshoot, - = undershoot. */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	float AverageOvershootPercent = 0.0f;
	/** How many flicks were used for the overshoot numbers, and the share that overshot / undershot (0-1). */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	int32 OvershootSamples = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	float OvershootShare = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	float UndershootShare = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	float AverageCorrections = 0.0f;

	/** True if the data shows a consistent over/undershoot. */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	bool bHasSuggestion = false;
	/** Suggested change of sensitivity in percent (negative = lower sensitivity). */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	float SuggestedChangePercent = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	float CurrentCm360 = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	float SuggestedCm360 = 0.0f;
	/** The sensitivity scale that would apply the suggestion. */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Flick")
	float SuggestedScale = 0.0f;

	float GetAccuracyPercent() const { return TotalTargets > 0 ? 100.0f * TotalHits / TotalTargets : 0.0f; }
};
