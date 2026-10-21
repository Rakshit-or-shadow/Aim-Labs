#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GEHittable.h"
#include "GETrainingTypes.h"
#include "GEBot.generated.h"

class UStaticMeshComponent;

/**
 * Training bot (Phase 3). A humanoid made of basic shapes with sphere hitboxes:
 * head (small), chest x2, legs x2. 100 HP. Dies, then respawns after RespawnDelay.
 * Always faces the player (yaw only). Behaviour depends on the training mode:
 *   Static   - stands still
 *   Strafing - moves left / right along LateralAxis for random times (ADAD)
 *   Peek     - steps out from behind cover along PeekDirection, waits, hides again
 * Shots are NOT blocked by cover (the project bans line traces), so a Peek bot
 * simply cannot be shot while it is hidden (CanBeShot() == false).
 * Actor origin is at the feet.
 */
UCLASS()
class GAMEENGINE_API AGEBot : public AActor, public IGEHittable
{
	GENERATED_BODY()

public:
	AGEBot();

	/** Call right after spawning. Home = current actor location. */
	void Configure(EGETrainingMode InMode, const FGEDifficultyPreset& InPreset, const FVector& InLateralAxis, const FVector& InPeekDirection);

	// IGEHittable
	virtual void GetHitSpheres(TArray<FGEHitSphere>& OutSpheres) const override;
	virtual bool CanBeShot() const override;
	virtual bool ApplyShotDamage(float Damage, EGEHitZone Zone, AActor* DamageInstigator) override;

	UFUNCTION(BlueprintPure, Category = "GE|Bot")
	bool IsAlive() const { return bAlive; }
	/** World time of the last (re)spawn. Phase 4 uses this as the "target appeared" time. */
	UFUNCTION(BlueprintPure, Category = "GE|Bot")
	float GetSpawnTime() const { return SpawnTime; }
	/** World time of the first hit since the last spawn, or -1 if not hit yet. Used for time-to-kill. */
	float GetFirstHitTime() const { return FirstHitTime; }
	void SetFirstHitTime(float T) { FirstHitTime = T; }

	/** If false the bot stays dead instead of respawning (Phase 4 single targets). */
	void SetRespawnEnabled(bool bEnabled) { bRespawnEnabled = bEnabled; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "GE|Bot")
	float MaxHealth = 100.0f;
	UPROPERTY(EditAnywhere, Category = "GE|Bot")
	float RespawnDelay = 1.5f;

	/** Hitbox layout relative to the feet (cm). */
	UPROPERTY(EditAnywhere, Category = "GE|Bot|Hitbox")
	FVector HeadOffset = FVector(0, 0, 165);
	UPROPERTY(EditAnywhere, Category = "GE|Bot|Hitbox")
	float HeadRadius = 11.0f;
	UPROPERTY(EditAnywhere, Category = "GE|Bot|Hitbox")
	FVector ChestOffset = FVector(0, 0, 130);
	UPROPERTY(EditAnywhere, Category = "GE|Bot|Hitbox")
	FVector Chest2Offset = FVector(0, 0, 100);
	UPROPERTY(EditAnywhere, Category = "GE|Bot|Hitbox")
	float BodyRadius = 20.0f;
	UPROPERTY(EditAnywhere, Category = "GE|Bot|Hitbox")
	FVector LegOffset = FVector(0, 0, 60);
	UPROPERTY(EditAnywhere, Category = "GE|Bot|Hitbox")
	FVector Leg2Offset = FVector(0, 0, 25);
	UPROPERTY(EditAnywhere, Category = "GE|Bot|Hitbox")
	float LegRadius = 15.0f;

	/** Strafing stays within +/- this distance of home (cm). */
	UPROPERTY(EditAnywhere, Category = "GE|Bot|Movement")
	float StrafeRange = 250.0f;
	/** How far a Peek bot steps out from behind its cover (cm). */
	UPROPERTY(EditAnywhere, Category = "GE|Bot|Movement")
	float PeekDistance = 130.0f;
	/** A Peek bot can be shot once it is at least this far out (cm). Below this it is hidden. */
	UPROPERTY(EditAnywhere, Category = "GE|Bot|Movement")
	float PeekExposedThreshold = 70.0f;

private:
	enum class EPeekState : uint8 { Hidden, SteppingOut, Exposed, SteppingBack };

	void Respawn();
	void FacePlayer();
	void TickStrafe(float Dt);
	void TickPeek(float Dt);
	void ApplyOffset();

	UStaticMeshComponent* AddPart(const TCHAR* Name, class UStaticMesh* Mesh, const FVector& Location, const FVector& SizeCm, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	/** Color for each entry of Parts (applied in BeginPlay via dynamic materials). */
	TArray<FLinearColor> PartColors;

	EGETrainingMode Mode = EGETrainingMode::Static;
	FGEDifficultyPreset Preset;
	FVector HomeLocation = FVector::ZeroVector;
	FVector LateralAxis = FVector::RightVector;
	FVector PeekDirection = FVector::RightVector;

	float Health = 100.0f;
	bool bAlive = true;
	bool bRespawnEnabled = true;
	float SpawnTime = 0.0f;
	float FirstHitTime = -1.0f;
	FTimerHandle RespawnTimer;

	// Strafe state
	float CurrentOffset = 0.0f;	// distance from home along the axis (strafe: +/-, peek: 0..PeekDistance)
	float StrafeDir = 0.0f;		// -1, 0, +1
	float PhaseTimeLeft = 0.0f;

	// Peek state
	EPeekState PeekState = EPeekState::Hidden;
};
