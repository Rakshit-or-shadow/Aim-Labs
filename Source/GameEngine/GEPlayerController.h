#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GEPlayerController.generated.h"

/**
 * Base C++ player controller.
 *
 * Minimal on purpose: BP_FirstPersonPlayerController already adds the input
 * mapping contexts (IMC_Default, IMC_MouseLook), sets the camera manager and
 * creates the touch-controls widget in its graph. This class is the place for
 * future controller-level C++ (e.g. pause, fire requests) without touching that.
 */
UCLASS(Blueprintable)
class GAMEENGINE_API AGEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGEPlayerController();
};
