// TargetSphere.h
// One sphere target you can shoot.
// Looks: a colored shaded sphere lit by its own small colored light,
// with a "pop" animation when it appears.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TargetSphere.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

UCLASS()
class RAYSHOT_API ATargetSphere : public AActor
{
    GENERATED_BODY()

public:
    ATargetSphere();

    virtual void Tick(float DeltaTime) override;

    // radius in cm - used by my ray-sphere math (the hit detection)
    UPROPERTY(EditAnywhere, Category = "Target")
    double Radius = 30.0;

    // movement speed + direction (only used in Tracking mode)
    FVector Velocity = FVector::ZeroVector;

    // only active (visible) targets can be hit
    bool bActive = false;

    // gently float up and down (used for the menu showcase)
    bool bBobbing = false;

    // changes the radius and resizes the mesh to match
    void SetRadius(double NewRadius);

    // changes the target's color (sphere + its light)
    void SetColor(FLinearColor NewColor);
    FLinearColor GetColor() const { return CurrentColor; }

    // shows / hides the target (showing it plays the pop animation)
    void SetActive(bool bNewActive);

    // moves the target and plays the pop animation
    void MoveTo(const FVector& NewLocation);

    // restarts the "pop" grow animation
    void PopIn();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category = "Target")
    TObjectPtr<USceneComponent> SceneRoot;

    // everything you see is attached to this, so we can bob it up and down
    // without moving the real (hit detection) position
    UPROPERTY(VisibleAnywhere, Category = "Target")
    TObjectPtr<USceneComponent> Visual;

    // the visible sphere (only for looks - NOT used for hit detection)
    UPROPERTY(VisibleAnywhere, Category = "Target")
    TObjectPtr<UStaticMeshComponent> Mesh;

    // small colored light just in front of the sphere = the "glow"
    UPROPERTY(VisibleAnywhere, Category = "Target")
    TObjectPtr<UPointLightComponent> GlowLight;

    // our own copy of the material, so we can recolor it
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> Material;

    FLinearColor CurrentColor = FLinearColor(0.0f, 0.85f, 1.0f); // cyan

    float PopTime = 10.0f;  // seconds since the pop animation started
    float BobPhase = 0.0f;  // random so targets don't bob in sync

    // sets the actor scale from Radius and the pop animation
    void UpdateScale();
};
