#include "GEFlickTracker.h"

void UGEFlickTracker::Reset()
{
	Records.Reset();
	Current = FGEFlickTargetRecord();
	bActive = false;
}

EGEFlickDirection UGEFlickTracker::DirectionFromDeltas(float YawDelta, float PitchDelta)
{
	// Compass bearing measured clockwise from "Up": 0 = up, 90 = right, 180 = down, -90 = left.
	const float Bearing = FMath::RadiansToDegrees(FMath::Atan2(YawDelta, PitchDelta));
	int32 Index = FMath::RoundToInt(Bearing / 45.0f);
	Index = ((Index % GENumFlickDirections) + GENumFlickDirections) % GENumFlickDirections;
	return (EGEFlickDirection)Index;
}

void UGEFlickTracker::BeginTarget(float Now, const FVector& ViewLocation, const FRotator& ControlRotation, const FVector& TargetAimPoint)
{
	Current = FGEFlickTargetRecord();
	bActive = true;
	StartTime = Now;
	StartYaw = ControlRotation.Yaw;
	StartPitch = ControlRotation.Pitch;

	const FVector ToTarget = TargetAimPoint - ViewLocation;
	const FRotator TargetRot = ToTarget.Rotation();
	Current.InitialYaw = FRotator::NormalizeAxis(TargetRot.Yaw - ControlRotation.Yaw);
	Current.InitialPitch = FRotator::NormalizeAxis(TargetRot.Pitch - ControlRotation.Pitch);
	Current.InitialAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ControlRotation.Vector(), ToTarget.GetSafeNormal()), -1.0f, 1.0f)));
	Current.Direction = DirectionFromDeltas(Current.InitialYaw, Current.InitialPitch);

	// First sample = the start (0, 0).
	Current.Path.Add(FGEPathSample());
}

void UGEFlickTracker::Sample(float Now, const FRotator& ControlRotation)
{
	if (!bActive)
	{
		return;
	}
	FGEPathSample S;
	S.Time = Now - StartTime;
	S.Yaw = FRotator::NormalizeAxis(ControlRotation.Yaw - StartYaw);
	S.Pitch = FRotator::NormalizeAxis(ControlRotation.Pitch - StartPitch);
	Current.Path.Add(S);
}

void UGEFlickTracker::NotifyShot(float Now)
{
	if (bActive && Current.TimeToFirstShot < 0.0f)
	{
		Current.TimeToFirstShot = Now - StartTime;
	}
}

void UGEFlickTracker::NotifyHit(float Now)
{
	if (bActive && !Current.bHit)
	{
		Current.bHit = true;
		Current.TimeToHit = Now - StartTime;
		if (Current.TimeToFirstShot < 0.0f)
		{
			Current.TimeToFirstShot = Current.TimeToHit;
		}
	}
}

void UGEFlickTracker::AbortTarget()
{
	bActive = false;
	Current = FGEFlickTargetRecord();
}

void UGEFlickTracker::EndTarget(float Now, bool bTimedOut)
{
	if (!bActive)
	{
		return;
	}
	Current.bTimedOut = bTimedOut;
	Analyse(Current);
	Records.Add(Current);
	bActive = false;
	Current = FGEFlickTargetRecord();
}

void UGEFlickTracker::Analyse(FGEFlickTargetRecord& Rec) const
{
	const float Length = FMath::Sqrt(Rec.InitialYaw * Rec.InitialYaw + Rec.InitialPitch * Rec.InitialPitch);
	if (Length < KINDA_SMALL_NUMBER)
	{
		return;
	}
	// Unit vector along the flick, in (yaw, pitch) degrees.
	const float UY = Rec.InitialYaw / Length;
	const float UP = Rec.InitialPitch / Length;

	// Progress along the flick in degrees (Length = exactly at the target). Only look up to the hit moment.
	float Peak = 0.0f;
	float Extreme = 0.0f;
	int32 Dir = 0;
	int32 Corrections = 0;
	const float H = Settings.CorrectionHysteresisDeg;
	for (const FGEPathSample& S : Rec.Path)
	{
		if (Rec.bHit && S.Time > Rec.TimeToHit)
		{
			break;
		}
		const float P = S.Yaw * UY + S.Pitch * UP;
		Peak = FMath::Max(Peak, P);

		// Reversal detection with hysteresis so mouse jitter is not counted.
		if (Dir == 0)
		{
			if (P - Extreme > H) { Dir = 1; Extreme = P; }
			else if (Extreme - P > H) { Dir = -1; Extreme = P; }
		}
		else if (Dir == 1)
		{
			if (P > Extreme) { Extreme = P; }
			else if (Extreme - P > H) { Dir = -1; Extreme = P; ++Corrections; }
		}
		else
		{
			if (P < Extreme) { Extreme = P; }
			else if (P - Extreme > H) { Dir = 1; Extreme = P; ++Corrections; }
		}
	}
	Rec.Corrections = Corrections;

	// Overshoot is only meaningful for a decent-sized flick that ended in a hit.
	if (Rec.bHit && Rec.InitialAngle >= Settings.MinAngleForOvershoot)
	{
		Rec.OvershootPercent = FMath::Clamp((Peak / Length - 1.0f) * 100.0f, -100.0f, 100.0f);
		Rec.bOvershootValid = true;
	}
}

FGEFlickSummary UGEFlickTracker::BuildSummary(float CurrentScale, float CurrentCm360) const
{
	const FGEFlickSettings& S = Settings;
	FGEFlickSummary Out;
	Out.Directions.SetNum(GENumFlickDirections);
	Out.CurrentCm360 = CurrentCm360;
	Out.SuggestedCm360 = CurrentCm360;
	Out.SuggestedScale = CurrentScale;

	float SumHitTime = 0.0f, SumFirstShot = 0.0f, SumOvershoot = 0.0f;
	int32 FirstShotCount = 0, Over = 0, Under = 0, Corr = 0;
	for (const FGEFlickTargetRecord& R : Records)
	{
		FGEFlickDirStats& D = Out.Directions[(int32)R.Direction];
		D.Targets++;
		Out.TotalTargets++;
		Corr += R.Corrections;
		if (R.bHit)
		{
			D.Hits++;
			D.TotalHitTime += R.TimeToHit;
			Out.TotalHits++;
			SumHitTime += R.TimeToHit;
		}
		if (R.TimeToFirstShot >= 0.0f)
		{
			SumFirstShot += R.TimeToFirstShot;
			++FirstShotCount;
		}
		if (R.bOvershootValid)
		{
			Out.OvershootSamples++;
			SumOvershoot += R.OvershootPercent;
			if (R.OvershootPercent > S.OvershootDeadbandPercent) { ++Over; }
			else if (R.OvershootPercent < -S.OvershootDeadbandPercent) { ++Under; }
		}
	}
	Out.AverageTimeToHit = Out.TotalHits > 0 ? SumHitTime / Out.TotalHits : 0.0f;
	Out.AverageTimeToFirstShot = FirstShotCount > 0 ? SumFirstShot / FirstShotCount : 0.0f;
	Out.AverageCorrections = Out.TotalTargets > 0 ? (float)Corr / Out.TotalTargets : 0.0f;

	if (Out.OvershootSamples > 0)
	{
		Out.AverageOvershootPercent = SumOvershoot / Out.OvershootSamples;
		Out.OvershootShare = (float)Over / Out.OvershootSamples;
		Out.UndershootShare = (float)Under / Out.OvershootSamples;
	}

	// Suggestion: only when enough flicks AND most of them go the same way.
	const float Avg = Out.AverageOvershootPercent;
	const bool bConsistentOver = Avg >= S.MinAvgOvershootForSuggestion && Out.OvershootShare >= S.MinConsistency;
	const bool bConsistentUnder = Avg <= -S.MinAvgOvershootForSuggestion && Out.UndershootShare >= S.MinConsistency;
	if (Out.OvershootSamples >= S.MinSamplesForSuggestion && (bConsistentOver || bConsistentUnder) && Avg > -90.0f)
	{
		// Moving Avg% too far means the view turns (1+Avg/100)x too much, so scale sensitivity by 1/(1+Avg/100).
		const float Factor = FMath::Clamp(1.0f / (1.0f + Avg / 100.0f), 1.0f - S.MaxSuggestedChangePercent / 100.0f, 1.0f + S.MaxSuggestedChangePercent / 100.0f);
		Out.bHasSuggestion = true;
		Out.SuggestedChangePercent = (Factor - 1.0f) * 100.0f;
		Out.SuggestedScale = CurrentScale * Factor;
		Out.SuggestedCm360 = CurrentCm360 / Factor; // more sensitivity = fewer cm per turn
	}
	return Out;
}
