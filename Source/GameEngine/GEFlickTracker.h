#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GEFlickTypes.h"
#include "GEFlickTracker.generated.h"

/**
 * Records what the mouse does for every flick target (Phase 4) and turns the records into a summary.
 * Owned by UGETrainingManager. Call order for one target:
 *   BeginTarget -> Sample (every frame) / NotifyShot / NotifyHit -> EndTarget (or AbortTarget).
 * Pure data logic: it never touches actors, so it is easy to unit test.
 */
UCLASS()
class GAMEENGINE_API UGEFlickTracker : public UObject
{
	GENERATED_BODY()

public:
	/** Forget everything. */
	void Reset();

	/** A target appeared. TargetAimPoint = world point the angle is measured to. */
	void BeginTarget(float Now, const FVector& ViewLocation, const FRotator& ControlRotation, const FVector& TargetAimPoint);
	/** Call every frame while a target is active (records the mouse path). */
	void Sample(float Now, const FRotator& ControlRotation);
	void NotifyShot(float Now);
	void NotifyHit(float Now);
	/** Target finished (killed or timed out). Computes overshoot and corrections and stores the record. */
	void EndTarget(float Now, bool bTimedOut);
	/** Throw the current target away without recording it (menu opened, session stopped). */
	void AbortTarget();

	bool IsTargetActive() const { return bActive; }
	const TArray<FGEFlickTargetRecord>& GetRecords() const { return Records; }

	/** Direction of a flick with these yaw / pitch deltas (+yaw = right, +pitch = up). */
	static EGEFlickDirection DirectionFromDeltas(float YawDelta, float PitchDelta);

	/** Builds the session summary. CurrentScale / CurrentCm360 describe the sensitivity used. */
	FGEFlickSummary BuildSummary(float CurrentScale, float CurrentCm360) const;

	/** Tunables (angles, thresholds). */
	FGEFlickSettings Settings;

private:
	/** Fills OvershootPercent / Corrections from the path. */
	void Analyse(FGEFlickTargetRecord& Rec) const;

	FGEFlickTargetRecord Current;
	bool bActive = false;
	float StartTime = 0.0f;
	float StartYaw = 0.0f;
	float StartPitch = 0.0f;

	TArray<FGEFlickTargetRecord> Records;
};
