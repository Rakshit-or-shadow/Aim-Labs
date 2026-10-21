#include "GEGameMode.h"
#include "GEHUD.h"
#include "GETargetDummy.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"

AGEGameMode::AGEGameMode()
{
	// Draw the crosshair HUD by default. A Blueprint child can still override this.
	HUDClass = AGEHUD::StaticClass();

	// Pawn and controller classes are left to the Blueprint child
	// (BP_FirstPersonCharacter / BP_FirstPersonPlayerController), because the C++
	// bases have no camera or input of their own.
}

void AGEGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Spawn a few test dummies in front of the first PlayerStart (intro room only).
	UWorld* World = GetWorld();
	if (!World || !World->GetMapName().Contains(TEXT("Lvl_IntroRoom")))
	{
		return;
	}
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		const FTransform T = It->GetActorTransform();
		for (const FVector& Offset : DummyOffsets)
		{
			// Offsets are relative to the start; Z is lowered ~90 cm so the dummy's feet touch the floor.
			FVector Loc = T.TransformPositionNoScale(Offset);
			Loc.Z = T.GetLocation().Z - 90.0f;
			const FRotator FaceStart = (T.GetLocation() - Loc).Rotation();
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			World->SpawnActor<AGETargetDummy>(AGETargetDummy::StaticClass(), Loc, FRotator(0, FaceStart.Yaw, 0), Params);
		}
		break; // only the first start
	}
}
