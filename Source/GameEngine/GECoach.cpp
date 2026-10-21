#include "GECoach.h"

namespace
{
	// ---- Thresholds (tune here) ----
	/** Flick: a direction needs this many valid flicks before we judge it. */
	constexpr int32 MinFlicksPerDirection = 3;
	/** Flick: average overshoot (percent of the flick) that counts as a habit. */
	constexpr float OverFlickPercent = 8.0f;
	constexpr float UnderFlickPercent = 8.0f;
	/** Moving: needs this many first shots while moving, and this many percentage points worse than standing. */
	constexpr int32 MinMovingFirstShots = 4;
	constexpr float MovingAccuracyGapPoints = 15.0f;
	/** Moving: if there are no standing shots to compare, flag moving first-shot accuracy below this (percent). */
	constexpr float MovingAbsoluteLowPercent = 30.0f;
	/** Headshot by distance: near = buckets below FarBucketStart, far = FarBucketStart and up (index 2 = 30 m). */
	constexpr int32 FarBucketStart = 2;
	constexpr int32 MinHitsPerRange = 5;
	constexpr float HeadshotDropPoints = 15.0f;
	/** First-shot accuracy below this (percent) with at least MinFirstShots first shots. */
	constexpr float LowFirstShotPercent = 40.0f;
	constexpr int32 MinFirstShots = 5;
	/** Time to kill (seconds) above this is slow; needs MinKills kills. */
	constexpr float SlowTimeToKill = 1.2f;
	constexpr int32 MinKills = 3;
	/** Flick: average time before the first shot (seconds) above this is slow. */
	constexpr float SlowFirstShotTime = 0.55f;
	/** Flick: average direction changes per flick above this is a lot. */
	constexpr float TooManyCorrections = 1.5f;
	/** How many past sessions the trends look at. */
	constexpr int32 TrendSessions = 5;
	/** Trends smaller than these are called "steady". */
	constexpr float TrendMinPercent = 1.0f;
	constexpr float TrendMinMs = 10.0f;

	void AddTip(TArray<FGECoachTip>& Out, const FString& Text, float Severity)
	{
		FGECoachTip Tip;
		Tip.Text = Text;
		Tip.Severity = Severity;
		Out.Add(Tip);
	}

	/** Average signed overshoot per direction (valid flicks only). */
	void DirectionOvershoot(const TArray<FGEFlickTargetRecord>& Records, float OutAvg[GENumFlickDirections], int32 OutCount[GENumFlickDirections])
	{
		float Sum[GENumFlickDirections] = {};
		for (int32 i = 0; i < GENumFlickDirections; ++i) { OutCount[i] = 0; OutAvg[i] = 0.0f; }
		for (const FGEFlickTargetRecord& R : Records)
		{
			if (R.bOvershootValid)
			{
				const int32 D = FMath::Clamp((int32)R.Direction, 0, GENumFlickDirections - 1);
				Sum[D] += R.OvershootPercent;
				OutCount[D]++;
			}
		}
		for (int32 i = 0; i < GENumFlickDirections; ++i)
		{
			OutAvg[i] = OutCount[i] > 0 ? Sum[i] / OutCount[i] : 0.0f;
		}
	}
}

// --------------------------------------------------------------------------
// Rules
// --------------------------------------------------------------------------

/** Flick: you overshoot targets in one direction -> lower sensitivity. */
void UGECoach::RuleOverFlick(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const
{
	float Avg[GENumFlickDirections]; int32 Count[GENumFlickDirections];
	DirectionOvershoot(Input.FlickRecords, Avg, Count);
	int32 Worst = -1;
	for (int32 i = 0; i < GENumFlickDirections; ++i)
	{
		if (Count[i] >= MinFlicksPerDirection && Avg[i] >= OverFlickPercent && (Worst < 0 || Avg[i] > Avg[Worst])) { Worst = i; }
	}
	if (Worst >= 0)
	{
		AddTip(Out, FString::Printf(TEXT("You over-flick %s by %.0f%% on average. Lower your sensitivity a little (the flick results show a suggested value)."),
			*GEFlickDirectionToString((EGEFlickDirection)Worst), Avg[Worst]), 1.0f + Avg[Worst] / 30.0f);
	}
}

/** Flick: you stop short of targets in one direction -> raise sensitivity. */
void UGECoach::RuleUnderFlick(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const
{
	float Avg[GENumFlickDirections]; int32 Count[GENumFlickDirections];
	DirectionOvershoot(Input.FlickRecords, Avg, Count);
	int32 Worst = -1;
	for (int32 i = 0; i < GENumFlickDirections; ++i)
	{
		if (Count[i] >= MinFlicksPerDirection && Avg[i] <= -UnderFlickPercent && (Worst < 0 || Avg[i] < Avg[Worst])) { Worst = i; }
	}
	if (Worst >= 0)
	{
		AddTip(Out, FString::Printf(TEXT("You stop short on %s flicks by %.0f%% on average. Raise your sensitivity a little or commit to a longer swipe."),
			*GEFlickDirectionToString((EGEFlickDirection)Worst), -Avg[Worst]), 1.0f + (-Avg[Worst]) / 30.0f);
	}
}

/** First shots fired while moving are much less accurate than standing ones -> counter-strafe. */
void UGECoach::RuleMovingFirstShots(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const
{
	const FGETrainingStats& S = Input.Latest.Stats;
	if (S.FirstShotsMoving < MinMovingFirstShots) { return; }
	const float MovingAcc = 100.0f * S.FirstShotHitsMoving / S.FirstShotsMoving;
	const int32 StillShots = S.FirstShots - S.FirstShotsMoving;
	const int32 StillHits = S.FirstShotHits - S.FirstShotHitsMoving;
	bool bFire = false;
	float Gap = 0.0f;
	if (StillShots >= MinMovingFirstShots)
	{
		Gap = 100.0f * StillHits / StillShots - MovingAcc;
		bFire = Gap >= MovingAccuracyGapPoints;
	}
	else if (MovingAcc < MovingAbsoluteLowPercent)
	{
		Gap = MovingAbsoluteLowPercent - MovingAcc;
		bFire = true;
	}
	if (bFire)
	{
		AddTip(Out, FString::Printf(TEXT("Your first shots while moving hit only %.0f%% (%d/%d). Counter-strafe: tap the opposite key to stop, then shoot."),
			MovingAcc, S.FirstShotHitsMoving, S.FirstShotsMoving), 1.0f + Gap / 50.0f);
	}
}

/** Headshot share drops with distance -> practice the far lane. */
void UGECoach::RuleHeadshotByDistance(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const
{
	const FGETrainingStats& S = Input.Latest.Stats;
	if (S.HitsByDistance.Num() < GENumDistanceBuckets || S.HeadHitsByDistance.Num() < GENumDistanceBuckets) { return; }
	int32 NearHits = 0, NearHead = 0, FarHits = 0, FarHead = 0;
	for (int32 i = 0; i < GENumDistanceBuckets; ++i)
	{
		if (i < FarBucketStart) { NearHits += S.HitsByDistance[i]; NearHead += S.HeadHitsByDistance[i]; }
		else { FarHits += S.HitsByDistance[i]; FarHead += S.HeadHitsByDistance[i]; }
	}
	if (NearHits < MinHitsPerRange || FarHits < MinHitsPerRange) { return; }
	const float NearPct = 100.0f * NearHead / NearHits;
	const float FarPct = 100.0f * FarHead / FarHits;
	if (NearPct - FarPct >= HeadshotDropPoints)
	{
		const int32 FarMeters = FMath::RoundToInt(FarBucketStart * GECoachTuning::BucketSizeMeters) + 10;
		AddTip(Out, FString::Printf(TEXT("Your headshot rate drops from %.0f%% up close to %.0f%% past %d m. Practice the %d m lane."),
			NearPct, FarPct, FarMeters - 10, FarMeters), 1.0f + (NearPct - FarPct) / 100.0f);
	}
}

/** The first bullet of each spray misses a lot -> slow down and aim before firing. */
void UGECoach::RuleLowFirstShotAccuracy(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const
{
	const FGETrainingStats& S = Input.Latest.Stats;
	if (S.FirstShots < MinFirstShots) { return; }
	const float Acc = S.GetFirstShotAccuracyPercent();
	if (Acc < LowFirstShotPercent)
	{
		AddTip(Out, FString::Printf(TEXT("First-shot accuracy is only %.0f%%. Let the crosshair settle on the target before the first click."), Acc),
			1.0f + (LowFirstShotPercent - Acc) / 100.0f);
	}
}

/** Kills take long after the first hit -> aim for the head / burst less. */
void UGECoach::RuleSlowTimeToKill(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const
{
	const FGETrainingStats& S = Input.Latest.Stats;
	if (S.Kills < MinKills) { return; }
	const float Ttk = S.GetAverageTimeToKill();
	if (Ttk > SlowTimeToKill)
	{
		AddTip(Out, FString::Printf(TEXT("Average time to kill is %.2f s. Aim higher (head and upper chest) and keep your crosshair on the target between shots."), Ttk),
			0.8f + FMath::Min((Ttk - SlowTimeToKill) / 2.0f, 0.5f));
	}
}

/** Flick: long delay before the first shot -> commit to the flick sooner. */
void UGECoach::RuleSlowFirstShot(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const
{
	if (!Input.Latest.bHasFlick) { return; }
	const float T = Input.Latest.Flick.AverageTimeToFirstShot;
	if (T > SlowFirstShotTime)
	{
		AddTip(Out, FString::Printf(TEXT("It takes you %.2f s to fire after a target appears. Flick and click in one motion instead of waiting."), T),
			0.7f + FMath::Min((T - SlowFirstShotTime), 0.5f));
	}
}

/** Flick: many direction changes per flick -> one clean swipe. */
void UGECoach::RuleTooManyCorrections(const FGECoachInput& Input, TArray<FGECoachTip>& Out) const
{
	if (!Input.Latest.bHasFlick) { return; }
	const float C = Input.Latest.Flick.AverageCorrections;
	if (C > TooManyCorrections)
	{
		AddTip(Out, FString::Printf(TEXT("You correct %.1f times per flick. Aim for one clean swipe, then a tiny adjustment."), C),
			0.8f + FMath::Min((C - TooManyCorrections) / 4.0f, 0.5f));
	}
}

void UGECoach::GenerateTips(const FGECoachInput& Input, TArray<FGECoachTip>& OutTips) const
{
	RuleOverFlick(Input, OutTips);
	RuleUnderFlick(Input, OutTips);
	RuleMovingFirstShots(Input, OutTips);
	RuleHeadshotByDistance(Input, OutTips);
	RuleLowFirstShotAccuracy(Input, OutTips);
	RuleSlowTimeToKill(Input, OutTips);
	RuleSlowFirstShot(Input, OutTips);
	RuleTooManyCorrections(Input, OutTips);
}

// --------------------------------------------------------------------------
// Report
// --------------------------------------------------------------------------
void UGECoach::BuildReport(const FGECoachInput& Input, FGECoachReport& OutReport) const
{
	OutReport = FGECoachReport();

	TArray<FGECoachTip> Tips;
	GenerateTips(Input, Tips);
	// Highest severity first.
	Tips.Sort([](const FGECoachTip& A, const FGECoachTip& B) { return A.Severity > B.Severity; });

	// Generic fallbacks fill the list up to 3 when fewer rules fired (lowest severity, so real tips stay first).
	TArray<FGECoachTip> Fallbacks;
	AddTip(Fallbacks, TEXT("Warm up with a few slow, precise shots on the 10 m lane before you speed up."), 0.1f);
	AddTip(Fallbacks, TEXT("Keep your crosshair at head height while you move so you only need small corrections."), 0.09f);
	AddTip(Fallbacks, TEXT("Short, steady sessions beat long tired ones. Take a break after about 15 minutes."), 0.08f);
	for (const FGECoachTip& F : Fallbacks)
	{
		if (Tips.Num() >= GECoachTipCount) { break; }
		Tips.Add(F);
	}
	Tips.SetNum(GECoachTipCount);

	OutReport.Tips = Tips;
	BuildTrends(Input, OutReport.Trends);
	OutReport.bValid = true;
}

/** Compares the oldest and newest of the last up to 5 sessions of the same mode. */
void UGECoach::BuildTrends(const FGECoachInput& Input, TArray<FString>& OutTrends) const
{
	TArray<const FGESessionRecord*> Window;
	for (int32 i = Input.History.Num() - 1; i >= 0 && Window.Num() < TrendSessions; --i)
	{
		if (Input.History[i].Mode == Input.Latest.Mode) { Window.Insert(&Input.History[i], 0); }
	}
	if (Window.Num() < 2)
	{
		OutTrends.Add(TEXT("Play at least two sessions of this mode to see your trends."));
		return;
	}
	const FGETrainingStats& First = Window[0]->Stats;
	const FGETrainingStats& Last = Window.Last()->Stats;
	const int32 N = Window.Num();

	const float AccDelta = Last.GetAccuracyPercent() - First.GetAccuracyPercent();
	if (FMath::Abs(AccDelta) < TrendMinPercent) { OutTrends.Add(FString::Printf(TEXT("Accuracy steady over the last %d sessions."), N)); }
	else { OutTrends.Add(FString::Printf(TEXT("Accuracy %+.0f%% over the last %d sessions."), AccDelta, N)); }

	if (First.Kills > 0 && Last.Kills > 0)
	{
		const float TtkDeltaMs = (Last.GetAverageTimeToKill() - First.GetAverageTimeToKill()) * 1000.0f;
		if (FMath::Abs(TtkDeltaMs) < TrendMinMs) { OutTrends.Add(TEXT("Time to kill steady.")); }
		else if (TtkDeltaMs < 0.0f) { OutTrends.Add(FString::Printf(TEXT("Time to kill improved %.0f ms."), -TtkDeltaMs)); }
		else { OutTrends.Add(FString::Printf(TEXT("Time to kill got slower by %.0f ms."), TtkDeltaMs)); }

		const float HsDelta = Last.GetHeadshotPercent() - First.GetHeadshotPercent();
		if (FMath::Abs(HsDelta) < TrendMinPercent) { OutTrends.Add(TEXT("Headshot % steady.")); }
		else { OutTrends.Add(FString::Printf(TEXT("Headshot %% %+.0f%% over the last %d sessions."), HsDelta, N)); }
	}
}
