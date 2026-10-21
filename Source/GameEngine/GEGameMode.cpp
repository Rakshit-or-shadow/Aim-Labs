#include "GEGameMode.h"
#include "GEHUD.h"
#include "GETargetDummy.h"
#include "GETrainingCenter.h"
#include "GEArena.h"
#include "GETargetManager.h"
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

	// The aim arena + Training Center are built in every level that has a PlayerStart (RayShot level and
	// the old intro room). The Phase 2 test dummies only appear in the intro room.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const bool bIntroRoom = World->GetMapName().Contains(TEXT("Lvl_IntroRoom"));
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		const FTransform T = It->GetActorTransform();
		if (bIntroRoom)
		{
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
		}
		// Training Center: built far away (decision: no new .umap without the editor). Press T to go there.
			const FVector Far(T.GetLocation().X, T.GetLocation().Y + TrainingCenterOffsetY, T.GetLocation().Z - 90.0f);
			const FTransform CenterT(FRotator::ZeroRotator, Far);
			if (AGETrainingCenter* TC = World->SpawnActorDeferred<AGETrainingCenter>(AGETrainingCenter::StaticClass(), CenterT))
			{
				FVector PortalLoc = T.TransformPositionNoScale(FVector(250, -350, 0));
				PortalLoc.Z = T.GetLocation().Z - 90.0f;
				TC->SetPortal(PortalLoc, T.GetRotation().Rotator().Yaw);
				TC->FinishSpawning(CenterT);
			}

		// Aim trainer arena: built far away along -Y (spawned from code, no map edit).
		// The manager is placed 1000 cm in front of the arena's player pad.
		{
			const FVector ArenaOrigin(T.GetLocation().X, T.GetLocation().Y + AimArenaOffsetY, T.GetLocation().Z - 90.0f);
			FActorSpawnParameters ArenaParams;
			ArenaParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AimArena = World->SpawnActor<AGEArena>(AGEArena::StaticClass(), ArenaOrigin, FRotator::ZeroRotator, ArenaParams);
			AimManager = World->SpawnActor<AGETargetManager>(AGETargetManager::StaticClass(),
				ArenaOrigin + FVector(1000.0, 0.0, 320.0), FRotator::ZeroRotator, ArenaParams);
			if (AimManager)
			{
				AimManager->SetArena(AimArena);
			}
		}
			break; // only the first start
	}
}
