// HitBurst.h
// Hit effect: glowing sparks fly out from the hit spot and shrink away,
// plus a quick flash of colored light.
// (Made only with basic shapes, so no particle assets are needed.)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GEHitBurst.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class GAMEENGINE_API AGEHitBurst : public AActor
{
	GENERATED_BODY()

public:
	AGEHitBurst();

	virtual void Tick(float DeltaTime) override;

	// call right after spawning: sets the color and size of the burst
	void Setup(FLinearColor Color, float TargetRadius);

private:
	// the little glowing sparks
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Pieces;

	// the flash of light
	UPROPERTY()
	TObjectPtr<UPointLightComponent> Flash;

	// which way each spark flies, and how far
	TArray<FVector> Directions;
	TArray<float> Distances;

	float Age = 0.0f;         // seconds since spawn
	float Lifetime = 0.4f;    // how long the effect lasts
	float StartScale = 0.1f;  // size of each spark
	float FlashIntensity = 60.0f;
};
