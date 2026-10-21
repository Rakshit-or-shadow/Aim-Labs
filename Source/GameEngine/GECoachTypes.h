#pragma once

#include "CoreMinimal.h"
#include "GESaveGame.h"
#include "GEFlickTypes.h"
#include "GECoachTypes.generated.h"

/** Numbers shared by the stat recording and the coach rules (Phase 5). Thresholds for single rules live in GECoach.cpp. */
namespace GECoachTuning
{
	/** Player horizontal speed (cm/s) above which a shot counts as "while moving". Walk is about 216, run about 540. */
	constexpr float MovingSpeed = 250.0f;
	/** Distance bucket size in metres (buckets are about 10 / 20 / 30 / 40 m). */
	constexpr float BucketSizeMeters = 10.0f;
}

/** One piece of advice. Higher Severity = shown first. */
USTRUCT(BlueprintType)
struct FGECoachTip
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GE|Coach")
	FString Text;
	/** Roughly 0..2. Used only for ranking. */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Coach")
	float Severity = 0.0f;
};

/** What the coach looks at: the finished session plus the history. Plain data, no engine objects, easy to unit test. */
struct FGECoachInput
{
	/** The session that just finished (equal to History.Last()). */
	FGESessionRecord Latest;
	/** All saved sessions, oldest first, including Latest. */
	TArray<FGESessionRecord> History;
	/** Per-target flick records of the latest session (empty unless it was a Flick session). */
	TArray<FGEFlickTargetRecord> FlickRecords;
};

/** The coach output shown on the results screen. */
USTRUCT(BlueprintType)
struct FGECoachReport
{
	GENERATED_BODY()

	/** Exactly GECoachTipCount tips when bValid. */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Coach")
	TArray<FGECoachTip> Tips;
	/** Short trend sentences ("Accuracy +6% over the last 5 sessions"). */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Coach")
	TArray<FString> Trends;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Coach")
	bool bValid = false;
};

/** How many tips the coach returns. */
constexpr int32 GECoachTipCount = 3;
