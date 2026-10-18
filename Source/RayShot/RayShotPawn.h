// RayShotPawn.h
// The player: a camera that stands still and turns with the mouse.
// It also reads the keyboard/mouse for the menus.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RayShotPawn.generated.h"

class UCameraComponent;
class APlayerController;
class ARayShotGameMode;

UCLASS()
class RAYSHOT_API ARayShotPawn : public APawn
{
    GENERATED_BODY()

public:
    ARayShotPawn();

    virtual void Tick(float DeltaTime) override;
    virtual void PossessedBy(AController* NewController) override;

    // look straight ahead again (called at the start of each round)
    void ResetAim();

    // the player's eyes
    UPROPERTY(VisibleAnywhere, Category = "Aim")
    TObjectPtr<UCameraComponent> Camera;

protected:
    virtual void BeginPlay() override;

private:
    // where we are looking (in degrees)
    float Yaw = 0.0f;
    float Pitch = 0.0f;

    // input handling for each kind of screen
    void TickPlaying(APlayerController* PlayerController, ARayShotGameMode* GameMode, float DeltaTime);
    void TickMenu(APlayerController* PlayerController, ARayShotGameMode* GameMode);
};
