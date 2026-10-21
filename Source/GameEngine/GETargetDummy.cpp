#include "GETargetDummy.h"
#include "GEHitRegistry.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"

AGETargetDummy::AGETargetDummy()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// Visual parts: head, chest, pelvis, two leg blobs. Mesh scale 1.0 = 100 cm diameter.
	struct FPart { const TCHAR* Name; FVector Offset; float Radius; };
	const FPart Defs[] = {
		{ TEXT("Head"), HeadOffset, HeadRadius },
		{ TEXT("Chest"), ChestOffset, BodyRadius },
		{ TEXT("Pelvis"), PelvisOffset, BodyRadius },
		{ TEXT("Leg"), LegOffset, LegRadius },
		{ TEXT("LowerLeg"), LowerLegOffset, LegRadius },
	};
	for (const FPart& D : Defs)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(D.Name);
		C->SetupAttachment(Root);
		if (SphereMesh.Succeeded())
		{
			C->SetStaticMesh(SphereMesh.Object);
		}
		C->SetRelativeLocation(D.Offset);
		C->SetRelativeScale3D(FVector(D.Radius * 2.0f / 100.0f));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision); // my ray math does the hit detection
		C->SetCastShadow(false);
	}
}

void AGETargetDummy::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	if (UGEHitRegistry* Reg = GetWorld()->GetSubsystem<UGEHitRegistry>())
	{
		Reg->Register(this);
	}
}

void AGETargetDummy::EndPlay(const EEndPlayReason::Type Reason)
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

void AGETargetDummy::GetHitSpheres(TArray<FGEHitSphere>& Out) const
{
	const FTransform T = GetActorTransform();
	Out.Add(FGEHitSphere(T.TransformPosition(HeadOffset), HeadRadius, EGEHitZone::Head));
	Out.Add(FGEHitSphere(T.TransformPosition(ChestOffset), BodyRadius, EGEHitZone::Body));
	Out.Add(FGEHitSphere(T.TransformPosition(PelvisOffset), BodyRadius, EGEHitZone::Body));
	Out.Add(FGEHitSphere(T.TransformPosition(LegOffset), LegRadius, EGEHitZone::Leg));
	Out.Add(FGEHitSphere(T.TransformPosition(LowerLegOffset), LegRadius, EGEHitZone::Leg));
}

bool AGETargetDummy::ApplyShotDamage(float Damage, EGEHitZone Zone, AActor* DamageInstigator)
{
	if (!bAlive)
	{
		return false;
	}
	Health -= Damage;
	// Small floating number so the damage is visible while testing.
	DrawDebugString(GetWorld(), GetActorLocation() + FVector(0, 0, 200), FString::Printf(TEXT("%.0f"), Damage), nullptr,
		Zone == EGEHitZone::Head ? FColor::Red : FColor::White, 0.6f);
	if (Health <= 0.0f)
	{
		bAlive = false;
		SetActorHiddenInGame(true);
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &AGETargetDummy::Respawn, RespawnDelay, false);
		return true;
	}
	return false;
}

void AGETargetDummy::Respawn()
{
	Health = MaxHealth;
	bAlive = true;
	SetActorHiddenInGame(false);
}
