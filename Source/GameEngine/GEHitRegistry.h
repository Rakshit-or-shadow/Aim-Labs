#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GEHitRegistry.generated.h"

/** World list of every shootable actor (anything implementing IGEHittable). */
UCLASS()
class GAMEENGINE_API UGEHitRegistry : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Add an actor that implements IGEHittable. Safe to call twice. */
	void Register(AActor* Actor);
	void Unregister(AActor* Actor);

	/** All currently registered actors (stale entries are skipped by the caller). */
	const TArray<TWeakObjectPtr<AActor>>& GetHittables() const { return Hittables; }

private:
	TArray<TWeakObjectPtr<AActor>> Hittables;
};
