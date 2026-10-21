#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GEHittable.h"
#include "GETargetDummy.generated.h"

/**
 * Simple stationary target made of basic shapes. Has sphere hitboxes
 * (head / body / legs) and no Unreal collision. Respawns after it dies.
 * Actor origin is at the feet.
 */
UCLASS()
class GAMEENGINE_API AGETargetDummy : public AActor, public IGEHittable
{
	GENERATED_BODY()

public:
	AGETargetDummy();

	// IGEHittable
	virtual void GetHitSpheres(TArray<FGEHitSphere>& OutSpheres) const override;
	virtual bool CanBeShot() const override { return bAlive; }
	virtual bool ApplyShotDamage(float Damage, EGEHitZone Zone, AActor* DamageInstigator) override;

	UFUNCTION(BlueprintPure, Category = "GE|Target")
	bool IsAlive() const { return bAlive; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(EditAnywhere, Category = "GE|Target")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, Category = "GE|Target")
	float RespawnDelay = 1.5f;

	/** Hitbox layout, relative to the feet (cm). */
	UPROPERTY(EditAnywhere, Category = "GE|Target")
	FVector HeadOffset = FVector(0, 0, 165);
	UPROPERTY(EditAnywhere, Category = "GE|Target")
	float HeadRadius = 11.0f;
	UPROPERTY(EditAnywhere, Category = "GE|Target")
	FVector ChestOffset = FVector(0, 0, 130);
	UPROPERTY(EditAnywhere, Category = "GE|Target")
	FVector PelvisOffset = FVector(0, 0, 100);
	UPROPERTY(EditAnywhere, Category = "GE|Target")
	float BodyRadius = 20.0f;
	UPROPERTY(EditAnywhere, Category = "GE|Target")
	FVector LegOffset = FVector(0, 0, 60);
	UPROPERTY(EditAnywhere, Category = "GE|Target")
	FVector LowerLegOffset = FVector(0, 0, 25);
	UPROPERTY(EditAnywhere, Category = "GE|Target")
	float LegRadius = 15.0f;

private:
	void Respawn();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	float Health = 100.0f;
	bool bAlive = true;
	FTimerHandle RespawnTimer;
};
