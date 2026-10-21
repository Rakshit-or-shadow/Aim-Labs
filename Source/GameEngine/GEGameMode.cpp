#include "GEGameMode.h"
#include "GEHUD.h"

AGEGameMode::AGEGameMode()
{
	// Draw the crosshair HUD by default. A Blueprint child can still override this.
	HUDClass = AGEHUD::StaticClass();

	// Pawn and controller classes are left to the Blueprint child
	// (BP_FirstPersonCharacter / BP_FirstPersonPlayerController), because the C++
	// bases have no camera or input of their own.
}
