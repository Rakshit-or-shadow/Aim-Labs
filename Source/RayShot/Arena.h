// Arena.h
// Builds the neon training room around the player when the game starts:
// dark walls, glowing strips, a floor grid, lights and screen effects (bloom).
// Everything is made from Unreal's basic cube, so no art assets are needed.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Arena.generated.h"

class UStaticMesh;
class UMaterialInterface;

UCLASS()
class RAYSHOT_API AArena : public AActor
{
    GENERATED_BODY()

public:
    AArena();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> LitMaterial;   // normal shaded surfaces

    UPROPERTY()
    TObjectPtr<UMaterialInterface> GlowMaterial;  // glowing neon surfaces

    // adds one box. Center and Size are in cm. bGlow = neon (unlit) instead of a normal surface
    void AddBox(const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bGlow);

    // adds one point light
    void AddLight(const FVector& Location, const FLinearColor& Color, float Candelas, float Radius, bool bShadows);

    // bloom, vignette, no motion blur, slightly punchier colors
    void AddPostProcess();
};
