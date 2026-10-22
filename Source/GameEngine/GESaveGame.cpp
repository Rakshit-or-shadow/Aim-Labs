#include "GESaveGame.h"
#include "GEAimModes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	const TCHAR* GESaveSlotName = TEXT("GE_Profile");
}

bool UGESaveGame::IsTourMode()
{
	static const bool bTour = FParse::Param(FCommandLine::Get(), TEXT("ShotTour"));
	return bTour;
}

UGESaveGame* UGESaveGame::LoadOrCreate()
{
	if (IsTourMode())
	{
		return Cast<UGESaveGame>(UGameplayStatics::CreateSaveGameObject(UGESaveGame::StaticClass())); // never touch the real profile
	}
	if (UGameplayStatics::DoesSaveGameExist(GESaveSlotName, 0))
	{
		if (UGESaveGame* Loaded = Cast<UGESaveGame>(UGameplayStatics::LoadGameFromSlot(GESaveSlotName, 0)))
		{
			return Loaded;
		}
	}
	return Cast<UGESaveGame>(UGameplayStatics::CreateSaveGameObject(UGESaveGame::StaticClass()));
}

bool UGESaveGame::Save()
{
	if (IsTourMode())
	{
		return true; // -ShotTour: nothing is saved
	}
	return UGameplayStatics::SaveGameToSlot(this, GESaveSlotName, 0);
}

bool UGESaveGame::GetBestScore(EGEAimMode Mode, float& OutBest) const
{
	if (const float* Found = BestScores.Find(GEAimModes::GetName(Mode)))
	{
		OutBest = *Found;
		return true;
	}
	return false;
}

bool UGESaveGame::SubmitScore(EGEAimMode Mode, float Score)
{
	float OldBest = 0.0f;
	const bool bHadBest = GetBestScore(Mode, OldBest);
	// Reflex: smaller (faster) is better. Everything else: bigger is better.
	const bool bBetter = GEAimModes::LowerIsBetter(Mode) ? (Score < OldBest) : (Score > OldBest);
	if (!bHadBest || bBetter)
	{
		BestScores.Add(GEAimModes::GetName(Mode), Score);
		return true;
	}
	return false;
}

void UGESaveGame::AddSession(const FGESessionRecord& Record)
{
	Sessions.Add(Record);
	if (Sessions.Num() > MaxSessions)
	{
		Sessions.RemoveAt(0, Sessions.Num() - MaxSessions);
	}
}
