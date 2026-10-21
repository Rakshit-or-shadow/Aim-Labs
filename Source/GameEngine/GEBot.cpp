#include "GEBot.h"
#include "GEHitRegistry.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UStaticMeshComponent* AGEBot::AddPart(const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& SizeCm, const FLinearColor& Color)
{
	UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	C->SetupAttachment(Root);
	if (Mesh)
	{
		C->SetStaticMesh(Mesh);
	}
	C->SetRelativeLocation(Location);
	C->SetRelativeScale3D(SizeCm / 100.0f); // basic shapes are 100 cm wide at scale 1
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision); // my ray math does the hit detection
	C->SetCastShadow(false);
	Parts.Add(C);
	PartColors.Add(Color);
	return C;
}

AGEBot::AGEBot()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Sphere = SphereMesh.Succeeded() ? SphereMesh.Object : nullptr;
	UStaticMesh* Cube = CubeMesh.Succeeded() ? CubeMesh.Object : nullptr;

	const FLinearColor Skin(0.8f, 0.55f, 0.4f);
	const FLinearColor Shirt(0.75f, 0.1f, 0.08f);
	const FLinearColor Pants(0.08f, 0.1f, 0.2f);

	// Visual parts (sizes in cm). They roughly match the hitbox spheres below.
	AddPart(TEXT("Head"), Sphere, HeadOffset, FVector(HeadRadius * 2.0f), Skin);
	AddPart(TEXT("Torso"), Cube, FVector(0, 0, 117), FVector(28, 44, 66), Shirt);
	AddPart(TEXT("ArmL"), Cube, FVector(0, -29, 115), FVector(12, 12, 58), Skin);
	AddPart(TEXT("ArmR"), Cube, FVector(0, 29, 115), FVector(12, 12, 58), Skin);
	AddPart(TEXT("LegL"), Cube, FVector(0, -12, 42), FVector(18, 18, 84), Pants);
	AddPart(TEXT("LegR"), Cube, FVector(0, 12, 42), FVector(18, 18, 84), Pants);
}

void AGEBot::Configure(EGETrainingMode InMode, const FGEDifficultyPreset& InPreset, const FVector& InLateralAxis, const FVector& InPeekDirection)
{
	Mode = InMode;
	Preset = InPreset;
	LateralAxis = InLateralAxis.GetSafeNormal();
	PeekDirection = InPeekDirection.GetSafeNormal();
	HomeLocation = GetActorLocation();
	CurrentOffset = 0.0f;
	StrafeDir = 0.0f;
	PhaseTimeLeft = 0.0f; // strafe: picks a direction on the first tick
	PeekState = EPeekState::Hidden;
	// Peek bots start hidden for a random fraction of the hidden time so they do not all peek together.
	PhaseTimeLeft = (Mode == EGETrainingMode::Peek) ? FMath::FRandRange(0.2f, 1.0f) * Preset.PeekHiddenTime : 0.0f;
}

void AGEBot::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	SpawnTime = GetWorld()->GetTimeSeconds();
	if (HomeLocation.IsZero())
	{
		HomeLocation = GetActorLocation();
	}

	// Give every part its own color through a dynamic material.
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMat)
	{
		for (int32 i = 0; i < Parts.Num(); ++i)
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(BaseMat, this);
			M->SetVectorParameterValue(TEXT("Color"), PartColors[i]);
			Parts[i]->SetMaterial(0, M);
		}
	}

	if (UGEHitRegistry* Reg = GetWorld()->GetSubsystem<UGEHitRegistry>())
	{
		Reg->Register(this);
	}
}

void AGEBot::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* W = GetWorld())
	{
		if (UGEHitRegistry* Reg = W->GetSubsystem<UGEHitRegistry>())
		{
			Reg->Unregister(this);
		}
	}
	Super::EndPlay(Reason);
}

void AGEBot::GetHitSpheres(TArray<FGEHitSphere>& Out) const
{
	const FTransform T = GetActorTransform();
	Out.Add(FGEHitSphere(T.TransformPosition(HeadOffset), HeadRadius, EGEHitZone::Head));
	Out.Add(FGEHitSphere(T.TransformPosition(ChestOffset), BodyRadius, EGEHitZone::Body));
	Out.Add(FGEHitSphere(T.TransformPosition(Chest2Offset), BodyRadius, EGEHitZone::Body));
	Out.Add(FGEHitSphere(T.TransformPosition(LegOffset), LegRadius, EGEHitZone::Leg));
	Out.Add(FGEHitSphere(T.TransformPosition(Leg2Offset), LegRadius, EGEHitZone::Leg));
}

bool AGEBot::CanBeShot() const
{
	if (!bAlive)
	{
		return false;
	}
	// Cover does not block shots (no line traces), so a hidden peeker is simply unshootable.
	if (Mode == EGETrainingMode::Peek && CurrentOffset < PeekExposedThreshold)
	{
		return false;
	}
	return true;
}

bool AGEBot::ApplyShotDamage(float Damage, EGEHitZone Zone, AActor* DamageInstigator)
{
	if (!bAlive)
	{
		return false;
	}
	// Damage already includes the weapon's head / body / leg multiplier.
	Health -= Damage;
	if (Health <= 0.0f)
	{
		bAlive = false;
		SetActorHiddenInGame(true);
		if (bRespawnEnabled)
		{
			GetWorldTimerManager().SetTimer(RespawnTimer, this, &AGEBot::Respawn, RespawnDelay, false);
		}
		return true;
	}
	return false;
}

void AGEBot::Respawn()
{
	Health = MaxHealth;
	bAlive = true;
	CurrentOffset = 0.0f;
	StrafeDir = 0.0f;
	PhaseTimeLeft = 0.0f;
	PeekState = EPeekState::Hidden;
	if (Mode == EGETrainingMode::Peek)
	{
		PhaseTimeLeft = FMath::FRandRange(0.2f, 1.0f) * Preset.PeekHiddenTime;
	}
	SpawnTime = GetWorld()->GetTimeSeconds();
	FirstHitTime = -1.0f;
	SetActorLocation(HomeLocation);
	SetActorHiddenInGame(false);
}

void AGEBot::FacePlayer()
{
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
		SetActorRotation(FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f));
	}
}

void AGEBot::ApplyOffset()
{
	const FVector& Axis = (Mode == EGETrainingMode::Peek) ? PeekDirection : LateralAxis;
	SetActorLocation(HomeLocation + Axis * CurrentOffset);
}

void AGEBot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bAlive)
	{
		return;
	}
	FacePlayer();

	if (Mode == EGETrainingMode::Strafing)
	{
		TickStrafe(DeltaSeconds);
	}
	else if (Mode == EGETrainingMode::Peek)
	{
		TickPeek(DeltaSeconds);
	}
}

void AGEBot::TickStrafe(float Dt)
{
	// Random "ADAD": every burst picks a random direction and a random duration (or a short pause).
	PhaseTimeLeft -= Dt;
	if (PhaseTimeLeft <= 0.0f)
	{
		if (FMath::FRand() < Preset.StrafePauseChance)
		{
			StrafeDir = 0.0f;
			PhaseTimeLeft = FMath::FRandRange(0.1f, 0.3f);
		}
		else
		{
			StrafeDir = (FMath::FRand() < 0.5f) ? -1.0f : 1.0f;
			PhaseTimeLeft = FMath::FRandRange(Preset.StrafeMinTime, FMath::Max(Preset.StrafeMinTime, Preset.StrafeMaxTime));
		}
	}

	CurrentOffset += StrafeDir * Preset.StrafeSpeed * Dt;
	if (FMath::Abs(CurrentOffset) > StrafeRange)
	{
		// Hit the edge of the strafe area: turn around.
		CurrentOffset = FMath::Clamp(CurrentOffset, -StrafeRange, StrafeRange);
		StrafeDir = -StrafeDir;
	}
	ApplyOffset();
}

void AGEBot::TickPeek(float Dt)
{
	switch (PeekState)
	{
	case EPeekState::Hidden:
		PhaseTimeLeft -= Dt;
		if (PhaseTimeLeft <= 0.0f)
		{
			PeekState = EPeekState::SteppingOut;
		}
		break;
	case EPeekState::SteppingOut:
		CurrentOffset += Preset.PeekSpeed * Dt;
		if (CurrentOffset >= PeekDistance)
		{
			CurrentOffset = PeekDistance;
			PeekState = EPeekState::Exposed;
			PhaseTimeLeft = Preset.PeekExposedTime;
		}
		break;
	case EPeekState::Exposed:
		PhaseTimeLeft -= Dt;
		if (PhaseTimeLeft <= 0.0f)
		{
			PeekState = EPeekState::SteppingBack;
		}
		break;
	case EPeekState::SteppingBack:
		CurrentOffset -= Preset.PeekSpeed * Dt;
		if (CurrentOffset <= 0.0f)
		{
			CurrentOffset = 0.0f;
			PeekState = EPeekState::Hidden;
			PhaseTimeLeft = Preset.PeekHiddenTime * FMath::FRandRange(0.7f, 1.3f);
		}
		break;
	}
	ApplyOffset();
}
