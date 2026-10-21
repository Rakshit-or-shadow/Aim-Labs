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

/**
 * The glowing frame on the back wall = the ONE source of truth for where aim targets may appear.
 * The arena is never rotated: +X is forward (towards the wall), +Y right, +Z up. All values are world space.
 */
struct FGEPlayRegion
{
	FVector Center = FVector::ZeroVector;   // middle of the frame, on the frame plane
	double HalfWidth = 0.0;                 // INNER half width (Y) of the frame
	double HalfHeight = 0.0;                // INNER half height (Z) of the frame
	double WallX = 0.0;                     // world X of the wall face the targets must stay in front of
	FVector ViewerXY = FVector::ZeroVector; // where the player's eye is horizontally (the pad), Z unused
};

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

	/** The playable region = the inside of the glowing frame (see FGEPlayRegion). */
	FGEPlayRegion GetPlayRegion() const;

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
