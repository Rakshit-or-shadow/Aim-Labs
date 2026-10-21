// Arena.h
// Builds the neon training room around the player when the game starts:
// dark walls, glowing strips, a floor grid, lights and screen effects (bloom).
// Everything is made from Unreal's basic cube, so no art assets are needed.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GEArena.generated.h"

class UStaticMesh;
class UMaterialInterface;

UCLASS()
class GAMEENGINE_API AGEArena : public AActor
{
	GENERATED_BODY()

public:
	AGEArena();

	/**
	 * Turns the neon post-process look (bloom, vignette, no motion blur) on or off.
	 * It is OFF by default so the rest of the level (intro room, Training Center) keeps its normal look;
	 * the target manager switches it on while an aim mode is active.
	 */
	void SetAimLookEnabled(bool bEnabled);

protected:
	virtual void BeginPlay() override;

private:
	/** The unbound post-process volume made in BeginPlay (disabled until an aim mode runs). */
	UPROPERTY()
	TObjectPtr<class APostProcessVolume> PostProcess;

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
