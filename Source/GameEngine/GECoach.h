#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GECoachProvider.h"
#include "GECoach.generated.h"

/**
 * Offline rule-based coach (Phase 5). No internet, no API.
 * Each rule is a small function in GECoach.cpp that may add one tip; thresholds are named constants there.
 * BuildReport() ranks the tips by severity, keeps exactly 3 (filling with generic tips) and adds trend lines.
 */
UCLASS()
class GAMEENGINE_API UGECoach : public UObject, public IGECoachProvider
{
	GENERATED_BODY()

public:
	/** IGECoachProvider: runs every rule. */
	virtual void GenerateTips(const FGECoachInput& Input, TArray<FGECoachTip>& OutTips) const override;

	/** Tips (exactly 3) + trends for the report. */
	void BuildReport(const FGECoachInput& Input, FGECoachReport& OutReport) const;

	/** Trend sentences across the last (up to 5) sessions of the same mode. */
	void BuildTrends(const FGECoachInput& Input, TArray<FString>& OutTrends) const;

private:
	// ---- Rules (each adds at most one tip) ----
	void RuleOverFlick(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const;
	void RuleUnderFlick(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const;
	void RuleMovingFirstShots(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const;
	void RuleHeadshotByDistance(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const;
	void RuleLowFirstShotAccuracy(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const;
	void RuleSlowTimeToKill(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const;
	void RuleSlowFirstShot(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const;
	void RuleTooManyCorrections(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const;
};
