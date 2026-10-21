// TargetSphere.cpp

#include "GETargetSphere.h"
#include "GEStyle.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Scene.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

// how long the pop-in animation lasts (seconds)
static const float TargetPopDuration = 0.18f;

AGETargetSphere::AGETargetSphere()
{
	// we tick for the pop + bob animations
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	RootComponent = SceneRoot;

	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual"));
	Visual->SetupAttachment(SceneRoot);

	// ---------- the sphere ----------
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Visual);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // MY ray tracer math does the hit detection

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(GEStyle::SphereMeshPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LitMaterial(GEStyle::LitMaterialPath);
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}
	if (LitMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, LitMaterial.Object);
	}

	// ---------- the glow light ----------
	// placed a little in front of the sphere (towards the player, who is at -X),
	// so it lights up the front of the sphere and the wall around it
	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(Visual);
	GlowLight->SetMobility(EComponentMobility::Movable);
	GlowLight->SetRelativeLocation(FVector(-90.0f, 0.0f, 20.0f)); // local units (scaled with the target)
}

void AGETargetSphere::BeginPlay()
{
	Super::BeginPlay();

	// light settings
	GlowLight->SetIntensityUnits(ELightUnits::Candelas);
	GlowLight->SetIntensity(12.0f);
	GlowLight->SetAttenuationRadius(500.0f);
	GlowLight->SetCastShadows(false);

	// make a copy of the material that we can recolor
	Material = Mesh->CreateAndSetMaterialInstanceDynamic(0);

	BobPhase = FMath::FRand() * 2.0f * PI;

	SetColor(CurrentColor);
	UpdateScale();
}

void AGETargetSphere::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// pop animation
	if (PopTime < TargetPopDuration)
	{
		PopTime += DeltaTime;
		UpdateScale();
	}

	// float gently up and down (menu showcase only)
	if (bBobbing)
	{
		float Time = GetWorld()->GetTimeSeconds();
		Visual->SetRelativeLocation(FVector(0.0f, 0.0f, FMath::Sin(Time * 2.0f + BobPhase) * 12.0f));
	}
	else if (!Visual->GetRelativeLocation().IsZero())
	{
		Visual->SetRelativeLocation(FVector::ZeroVector);
	}
}

void AGETargetSphere::UpdateScale()
{
	// 0 -> 1 over the pop duration
	float T = FMath::Clamp(PopTime / TargetPopDuration, 0.0f, 1.0f);

	// "ease out back": grows a little past full size, then settles (feels bouncy)
	const float C1 = 1.70158f;
	const float C3 = C1 + 1.0f;
	float Pop = 1.0f + C3 * FMath::Pow(T - 1.0f, 3.0f) + C1 * FMath::Pow(T - 1.0f, 2.0f);
	Pop = FMath::Max(Pop, 0.01f);

	// built-in sphere has radius 50, so scale it to match our radius
	SetActorScale3D(FVector(Radius / 50.0 * Pop));
}

void AGETargetSphere::SetRadius(double NewRadius)
{
	Radius = NewRadius;
	UpdateScale();
}

void AGETargetSphere::SetColor(FLinearColor NewColor)
{
	CurrentColor = NewColor;

	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), CurrentColor);
	}
	if (GlowLight)
	{
		GlowLight->SetLightColor(CurrentColor);
	}
}

void AGETargetSphere::SetActive(bool bNewActive)
{
	bActive = bNewActive;
	SetActorHiddenInGame(!bActive);
	GlowLight->SetVisibility(bActive);

	if (bActive)
	{
		PopIn();
	}
}

void AGETargetSphere::MoveTo(const FVector& NewLocation)
{
	SetActorLocation(NewLocation);
	PopIn();
}

void AGETargetSphere::PopIn()
{
	PopTime = 0.0f;
	UpdateScale();
}
