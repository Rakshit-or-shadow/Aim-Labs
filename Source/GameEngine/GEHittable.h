#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GEHittable.generated.h"

/** Which body part a hitbox sphere belongs to. Decides the damage multiplier. */
UENUM(BlueprintType)
enum class EGEHitZone : uint8
{
	Head,
	Body,
	Leg
};

/** One sphere hitbox in WORLD space. Shots are tested against these with RayTraceMath::DoesIntersect. */
USTRUCT(BlueprintType)
struct FGEHitSphere
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "GE|Hitbox")
	FVector Center = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "GE|Hitbox")
	float Radius = 10.0f;

	UPROPERTY(BlueprintReadWrite, Category = "GE|Hitbox")
	EGEHitZone Zone = EGEHitZone::Body;

	FGEHitSphere() {}
	FGEHitSphere(const FVector& InCenter, float InRadius, EGEHitZone InZone)
		: Center(InCenter), Radius(InRadius), Zone(InZone) {}
};

UINTERFACE(MinimalAPI)
class UGEHittable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement this on any actor that can be shot (bots, target dummies).
 * The actor must also register itself with UGEHitRegistry in BeginPlay
 * (Registry->Register(this)) and Unregister in EndPlay.
 */
class GAMEENGINE_API IGEHittable
{
	GENERATED_BODY()

public:
	/** Append this actor's hitbox spheres (world space, current positions). */
	virtual void GetHitSpheres(TArray<FGEHitSphere>& OutSpheres) const = 0;

	/** Return false while dead / hidden so shots pass through. */
	virtual bool CanBeShot() const { return true; }

	/** Apply already-multiplied damage. Return true if this shot killed the target. */
	virtual bool ApplyShotDamage(float Damage, EGEHitZone Zone, AActor* DamageInstigator) = 0;
};
