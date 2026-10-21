#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GEPlayerController.generated.h"

/**
 * Base C++ player controller.
 *
 * BP_FirstPersonPlayerController adds the input mapping contexts and camera manager.
 * Phase 4: look input goes through AddYawInput / AddPitchInput (the Blueprint's
 * "Add Controller Yaw/Pitch Input" nodes end up here), so we scale it by the stored
 * mouse sensitivity (UGETrainingManager::GetSensitivityScale).
 */
UCLASS(Blueprintable)
class GAMEENGINE_API AGEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGEPlayerController();

	virtual void AddYawInput(float Val) override;
	virtual void AddPitchInput(float Val) override;

private:
	float GetSensitivityScale() const;
};
