#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GEGameMode.generated.h"

/**
 * Base C++ game mode.
 *
 * Derives from AGameModeBase (the same parent BP_FirstPersonGameMode had), so
 * reparenting the Blueprint changes no match-flow behaviour. Its only job right
 * now is to make AGEHUD the default HUD. The Blueprint keeps choosing the pawn
 * and player controller classes.
 */
UCLASS(Blueprintable)
class GAMEENGINE_API AGEGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGEGameMode();

	/** The aim-trainer manager (spawned in the intro room), or null in other levels. */
	class AGETargetManager* GetAimManager() const { return AimManager; }

protected:
	virtual void BeginPlay() override;

	/** Aim trainer: how far (cm, along world Y) from the player start the aim arena is built. Negative = -Y. */
	UPROPERTY(EditDefaultsOnly, Category = "GE|Aim")
	float AimArenaOffsetY = -300000.0f;

	UPROPERTY(Transient)
	TObjectPtr<class AGEArena> AimArena;

	UPROPERTY(Transient)
	TObjectPtr<class AGETargetManager> AimManager;

	/** Phase 2 test targets: spawned in front of the player start on the intro map. */
	UPROPERTY(EditDefaultsOnly, Category = "GE|Targets")
	TArray<FVector> DummyOffsets = { FVector(800, -250, 0), FVector(1400, 250, 0), FVector(2200, -400, 0), FVector(3000, 400, 0) };

	/** Phase 3: how far (cm, along world Y) from the player start the Training Center is built. */
	UPROPERTY(EditDefaultsOnly, Category = "GE|Training")
	float TrainingCenterOffsetY = 200000.0f;
};
