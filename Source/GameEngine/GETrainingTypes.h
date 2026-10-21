#pragma once

#include "CoreMinimal.h"
#include "GETrainingTypes.generated.h"

/** What the bots do during a training session (Phase 3). Flick is reserved for Phase 4. */
UENUM(BlueprintType)
enum class EGETrainingMode : uint8
{
	Static,		// bots stand still
	Strafing,	// bots move left / right at random (ADAD)
	Peek,		// bots step out from behind cover, then hide again
	Flick		// Phase 4: single targets appear at random angles (no bots are spawned by the manager)
};

UENUM(BlueprintType)
enum class EGEDifficulty : uint8
{
	Easy,
	Medium,
	Hard
};

/** Bot behaviour numbers for one difficulty. Units: cm, seconds. */
USTRUCT(BlueprintType)
struct FGEDifficultyPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Difficulty")
	FString Name = TEXT("Medium");

	/** Sideways speed while strafing (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Difficulty")
	float StrafeSpeed = 350.0f;
	/** A strafe burst lasts a random time between these two values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Difficulty")
	float StrafeMinTime = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Difficulty")
	float StrafeMaxTime = 1.1f;
	/** Chance (0-1) that the bot stops for a moment between bursts (a short pause makes it easier to hit). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Difficulty")
	float StrafePauseChance = 0.3f;

	/** Speed of stepping out of / back into cover (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Difficulty")
	float PeekSpeed = 400.0f;
	/** How long the bot stays fully out (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Difficulty")
	float PeekExposedTime = 1.0f;
	/** How long the bot stays hidden behind cover (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Difficulty")
	float PeekHiddenTime = 1.2f;
};

/**
 * Session statistics. Definitions (documented decisions):
 *  - Shots     = trigger pulls (a shotgun blast counts as ONE shot).
 *  - Hits      = trigger pulls where at least one bullet / pellet hit a bot.
 *  - Headshot% = kills whose killing bullet hit the head / total kills.
 *  - Time to kill = seconds from the FIRST HIT on that bot (since it last spawned) to its death.
 *  - First-shot accuracy = hit rate of the first shot of each spray (spread fully recovered).
 */
USTRUCT(BlueprintType)
struct FGETrainingStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GE|Stats")
	int32 Kills = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Stats")
	int32 HeadshotKills = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Stats")
	int32 Shots = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Stats")
	int32 Hits = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Stats")
	int32 FirstShots = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Stats")
	int32 FirstShotHits = 0;
	/** Sum of every kill's time to kill (seconds). */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Stats")
	float TotalTimeToKill = 0.0f;
	/** Length of the session so far (seconds). */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Stats")
	float Duration = 0.0f;

	/** 0-100 */
	float GetHeadshotPercent() const { return Kills > 0 ? 100.0f * HeadshotKills / Kills : 0.0f; }
	/** 0-100 */
	float GetAccuracyPercent() const { return Shots > 0 ? 100.0f * Hits / Shots : 0.0f; }
	/** 0-100 */
	float GetFirstShotAccuracyPercent() const { return FirstShots > 0 ? 100.0f * FirstShotHits / FirstShots : 0.0f; }
	/** Seconds. 0 when there were no kills. */
	float GetAverageTimeToKill() const { return Kills > 0 ? TotalTimeToKill / Kills : 0.0f; }
};

/** One floating text popup (e.g. "HEADSHOT") that the HUD draws at a world position. */
USTRUCT()
struct FGEPopup
{
	GENERATED_BODY()

	FVector WorldLocation = FVector::ZeroVector;
	FString Text;
	FLinearColor Color = FLinearColor::White;
	float SpawnTime = 0.0f;
};

/** Where a bot stands in the Training Center (world space). */
struct FGEBotSlot
{
	FVector Location = FVector::ZeroVector;
	/** Unit vector the bot strafes along. */
	FVector LateralAxis = FVector::RightVector;
	/** Unit vector pointing from cover toward the open lane (peek direction). */
	FVector PeekDirection = FVector::RightVector;
};
