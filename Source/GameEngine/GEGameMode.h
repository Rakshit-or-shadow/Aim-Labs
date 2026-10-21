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

protected:
	virtual void BeginPlay() override;

	/** Phase 2 test targets: spawned in front of the player start on the intro map. */
	UPROPERTY(EditDefaultsOnly, Category = "GE|Targets")
	TArray<FVector> DummyOffsets = { FVector(800, -250, 0), FVector(1400, 250, 0), FVector(2200, -400, 0), FVector(3000, 400, 0) };
};
