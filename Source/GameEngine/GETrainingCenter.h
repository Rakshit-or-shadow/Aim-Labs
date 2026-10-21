#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GETrainingTypes.h"
#include "GETrainingCenter.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/**
 * Layout numbers for the Training Center. Local space: X = down the lane (away from
 * the player spawn), Y = sideways, Z = up, Z 0 = floor top. Units: cm.
 */
USTRUCT(BlueprintType)
struct FGETrainingLayout
{
	GENERATED_BODY()

	/** Floor rectangle: from FloorMinX to FloorMaxX, +/- FloorHalfWidth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	float FloorMinX = -1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	float FloorMaxX = 7000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	float FloorHalfWidth = 1500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	float WallHeight = 500.0f;

	/** Distance markers in metres (1 m = 100 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	TArray<int32> MarkerDistancesMeters = { 10, 20, 30, 40 };

	/** Cover walls: X / Y of each wall's centre. Peek bots hide behind these. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	TArray<FVector2D> CoverWalls = { FVector2D(1500, -300), FVector2D(2500, 300), FVector2D(3500, -300) };
	/** Cover wall size (X thickness, Y width, Z height). 160 wide hides a bot completely. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	FVector CoverSize = FVector(30.0f, 160.0f, 220.0f);
	/** How far behind the wall (cm) a peek bot stands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	float PeekBotGapBehindCover = 60.0f;

	/** Where Static / Strafing bots stand (X / Y). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	TArray<FVector2D> OpenBotPositions = { FVector2D(1000, -150), FVector2D(2000, 150), FVector2D(3000, -100), FVector2D(4000, 100) };

	/** Extra boxes for flavour (X / Y centre, 100 cm cubes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	TArray<FVector2D> Crates = { FVector2D(700, -550), FVector2D(1200, 600), FVector2D(500, 450) };

	/** Point lights along the lane. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	float LightIntensity = 15000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	float LightHeight = 450.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Layout")
	float LightSpacing = 1800.0f;
};

/**
 * The Training Center (Phase 3), built entirely from C++ at BeginPlay out of basic
 * shapes: floor, walls, shooting lane with 10/20/30/40 m markers, cover walls,
 * crates and lights. Also builds a portal marker near the player start.
 *
 * DECISION: a new .umap cannot be created without the editor, so this actor is spawned
 * by AGEGameMode far away from the start inside Lvl_IntroRoom. Press T to go there.
 */
UCLASS()
class GAMEENGINE_API AGETrainingCenter : public AActor
{
	GENERATED_BODY()

public:
	AGETrainingCenter();

	/** Call before the actor finishes spawning (deferred spawn): where to build the portal marker. */
	void SetPortal(const FVector& WorldLocation, float FacingYaw)
	{
		PortalLocation = WorldLocation;
		PortalYaw = FacingYaw;
		bHasPortal = true;
	}

	/** Fills OutSlots with bot positions (world space) for the given mode. */
	void GetBotSlots(EGETrainingMode Mode, TArray<FGEBotSlot>& OutSlots) const;

	/** Where the player appears when teleporting in (standing, looking down the lane). */
	void GetPlayerSpawn(FVector& OutLocation, FRotator& OutRotation) const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "GE|Layout")
	FGETrainingLayout Layout;

private:
	UStaticMeshComponent* AddBox(const FVector& LocalCenter, const FVector& SizeCm, const FLinearColor& Color, bool bCollide, float YawDeg = 0.0f, bool bGlow = false);
	void AddText(const FString& Text, const FVector& LocalLocation, float YawDeg, float WorldSize, const FColor& Color);
	void AddLight(const FVector& LocalLocation);
	void BuildHub();
	void BuildLane();
	void BuildPortal();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseMaterial;
	/** Unlit material for neon strips (values above 1 bloom). */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowMaterial;

	int32 PartCounter = 0;
	bool bHasPortal = false;
	FVector PortalLocation = FVector::ZeroVector;
	float PortalYaw = 0.0f;
};
