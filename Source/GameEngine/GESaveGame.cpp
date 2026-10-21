#include "GESaveGame.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* GESaveSlotName = TEXT("GE_Profile");
}

UGESaveGame* UGESaveGame::LoadOrCreate()
{
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
	return UGameplayStatics::SaveGameToSlot(this, GESaveSlotName, 0);
}

void UGESaveGame::AddSession(const FGESessionRecord& Record)
{
	Sessions.Add(Record);
	if (Sessions.Num() > MaxSessions)
	{
		Sessions.RemoveAt(0, Sessions.Num() - MaxSessions);
	}
}
