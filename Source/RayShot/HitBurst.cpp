// HitBurst.cpp

#include "HitBurst.h"
#include "RayShotStyle.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Scene.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AHitBurst::AHitBurst()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(RayShotStyle::SphereMeshPath);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMaterial(RayShotStyle::GlowMaterialPath);

    // create 14 small glowing spheres
    const int32 NumPieces = 14;
    for (int32 i = 0; i < NumPieces; i++)
    {
        FName PieceName = FName(*FString::Printf(TEXT("Piece%d"), i));
        UStaticMeshComponent* Piece = CreateDefaultSubobject<UStaticMeshComponent>(PieceName);
        Piece->SetupAttachment(RootComponent);
        Piece->SetMobility(EComponentMobility::Movable);
        Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Piece->SetCastShadow(false);

        if (SphereMesh.Succeeded())
        {
            Piece->SetStaticMesh(SphereMesh.Object);
        }
        if (GlowMaterial.Succeeded())
        {
            Piece->SetMaterial(0, GlowMaterial.Object);
        }

        Pieces.Add(Piece);
    }

    // a bright light that fades out quickly
    Flash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));
    Flash->SetupAttachment(RootComponent);
    Flash->SetMobility(EComponentMobility::Movable);
}

void AHitBurst::Setup(FLinearColor Color, float TargetRadius)
{
    // bigger targets -> bigger burst
    StartScale = (TargetRadius / 50.0f) * 0.22f;

    for (UStaticMeshComponent* Piece : Pieces)
    {
        // random direction + distance for this spark
        Directions.Add(FMath::VRand());
        Distances.Add(TargetRadius * FMath::RandRange(2.5f, 4.5f));

        // glowing version of the target color (above 1.0 = bloom)
        UMaterialInstanceDynamic* SparkMaterial = Piece->CreateAndSetMaterialInstanceDynamic(0);
        if (SparkMaterial)
        {
            SparkMaterial->SetVectorParameterValue(TEXT("Color"), RayShotStyle::Glow(Color, 4.0f));
        }

        Piece->SetRelativeScale3D(FVector(StartScale));
    }

    // light flash in the target color
    Flash->SetIntensityUnits(ELightUnits::Candelas);
    Flash->SetIntensity(FlashIntensity);
    Flash->SetLightColor(Color);
    Flash->SetAttenuationRadius(TargetRadius * 15.0f);
    Flash->SetCastShadows(false);
}

void AHitBurst::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // not set up yet
    if (Directions.Num() != Pieces.Num())
    {
        return;
    }

    Age += DeltaTime;
    float LifeProgress = Age / Lifetime; // 0 -> 1

    // effect finished: remove it
    if (LifeProgress >= 1.0f)
    {
        Destroy();
        return;
    }

    // fast at the start, slowing down at the end ("ease out")
    float Travel = 1.0f - FMath::Pow(1.0f - LifeProgress, 3.0f);

    // move each spark outward and shrink it
    for (int32 i = 0; i < Pieces.Num(); i++)
    {
        Pieces[i]->SetRelativeLocation(Directions[i] * Distances[i] * Travel);
        Pieces[i]->SetRelativeScale3D(FVector(StartScale * (1.0f - LifeProgress)));
    }

    // fade the light out
    Flash->SetIntensity(FlashIntensity * (1.0f - LifeProgress) * (1.0f - LifeProgress));
}
