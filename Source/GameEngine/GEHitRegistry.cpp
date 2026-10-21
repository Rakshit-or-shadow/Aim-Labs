#include "GEHitRegistry.h"

void UGEHitRegistry::Register(AActor* Actor)
{
	if (Actor)
	{
		Hittables.AddUnique(Actor);
	}
}

void UGEHitRegistry::Unregister(AActor* Actor)
{
	Hittables.Remove(Actor);
}
