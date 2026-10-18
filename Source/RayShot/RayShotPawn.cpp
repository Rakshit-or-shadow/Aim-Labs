// RayShotPawn.cpp

#include "RayShotPawn.h"
#include "RayShotGameMode.h"
#include "RayShotSaveGame.h"
#include "TargetManager.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Engine/World.h"

ARayShotPawn::ARayShotPawn()
{
    // we read the mouse every frame
    PrimaryActorTick.bCanEverTick = true;

    // the camera is the whole player
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    RootComponent = Camera;
    Camera->SetFieldOfView(103.0f);
}

void ARayShotPawn::BeginPlay()
{
    Super::BeginPlay();

    // stand in a fixed shooting spot, 3 m above the floor, looking forward (+X)
    SetActorLocation(FVector(0.0, 0.0, 300.0));
    ResetAim();
}

void ARayShotPawn::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // show/hide the mouse cursor depending on the current screen
    ARayShotGameMode* GameMode = GetWorld()->GetAuthGameMode<ARayShotGameMode>();
    if (GameMode)
    {
        GameMode->UpdateInputMode();
    }
}

void ARayShotPawn::ResetAim()
{
    Yaw = 0.0f;
    Pitch = 0.0f;
    SetActorRotation(FRotator::ZeroRotator);
}

void ARayShotPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    ARayShotGameMode* GameMode = GetWorld()->GetAuthGameMode<ARayShotGameMode>();
    if (PlayerController == nullptr || GameMode == nullptr || GameMode->SaveData == nullptr)
    {
        return;
    }

    // apply the field of view setting
    Camera->SetFieldOfView(GameMode->SaveData->FieldOfView);

    if (GameMode->Screen == ERayShotScreen::Playing)
    {
        TickPlaying(PlayerController, GameMode, DeltaTime);
    }
    else
    {
        TickMenu(PlayerController, GameMode);
    }
}

void ARayShotPawn::TickPlaying(APlayerController* PlayerController, ARayShotGameMode* GameMode, float DeltaTime)
{
    // ---------- 1. look around with the mouse ----------
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    PlayerController->GetInputMouseDelta(MouseX, MouseY);

    float Sensitivity = GameMode->SaveData->Sensitivity;
    Yaw += MouseX * Sensitivity;
    Pitch += MouseY * Sensitivity;
    Pitch = FMath::Clamp(Pitch, -89.0f, 89.0f); // don't flip upside down

    SetActorRotation(FRotator(Pitch, Yaw, 0.0f));

    // ---------- 2. aim + shoot ----------
    ATargetManager* Manager = GameMode->GetManager();
    if (Manager)
    {
        // the ray: from the camera, straight ahead (like my ray_generation)
        FVector Origin = Camera->GetComponentLocation();
        FVector Direction = Camera->GetForwardVector();

        Manager->UpdateAim(Origin, Direction, DeltaTime); // Tracking uses this

        if (PlayerController->WasInputKeyJustPressed(EKeys::LeftMouseButton))
        {
            Manager->HandleClick(Origin, Direction);
        }
    }

    // ---------- 3. R = restart, Tab/Esc = back to menu ----------
    if (PlayerController->WasInputKeyJustPressed(EKeys::R))
    {
        GameMode->StartMode(GameMode->CurrentMode);
    }
    else if (PlayerController->WasInputKeyJustPressed(EKeys::Tab) ||
             PlayerController->WasInputKeyJustPressed(EKeys::Escape))
    {
        GameMode->Back();
    }
}

void ARayShotPawn::TickMenu(APlayerController* PlayerController, ARayShotGameMode* GameMode)
{
    // ---------- slow camera drift behind the menus (looks alive) ----------
    float Time = GetWorld()->GetTimeSeconds();
    SetActorRotation(FRotator(FMath::Sin(Time * 0.35f) * 1.5f, FMath::Sin(Time * 0.22f) * 3.0f, 0.0f));

    // ---------- keyboard ----------
    if (PlayerController->WasInputKeyJustPressed(EKeys::W) ||
        PlayerController->WasInputKeyJustPressed(EKeys::Up))
    {
        GameMode->MoveSelection(-1);
    }
    if (PlayerController->WasInputKeyJustPressed(EKeys::S) ||
        PlayerController->WasInputKeyJustPressed(EKeys::Down))
    {
        GameMode->MoveSelection(+1);
    }
    if (PlayerController->WasInputKeyJustPressed(EKeys::A) ||
        PlayerController->WasInputKeyJustPressed(EKeys::Left))
    {
        GameMode->ChangeSetting(-1);
    }
    if (PlayerController->WasInputKeyJustPressed(EKeys::D) ||
        PlayerController->WasInputKeyJustPressed(EKeys::Right))
    {
        GameMode->ChangeSetting(+1);
    }
    if (PlayerController->WasInputKeyJustPressed(EKeys::Enter) ||
        PlayerController->WasInputKeyJustPressed(EKeys::SpaceBar))
    {
        GameMode->ActivateSelected();
        return;
    }
    if (PlayerController->WasInputKeyJustPressed(EKeys::Tab) ||
        PlayerController->WasInputKeyJustPressed(EKeys::Escape))
    {
        GameMode->Back();
        return;
    }
    // R on the results screen = play the same mode again
    if (GameMode->Screen == ERayShotScreen::Results && PlayerController->WasInputKeyJustPressed(EKeys::R))
    {
        GameMode->StartMode(GameMode->CurrentMode);
        return;
    }

    // ---------- mouse (the HUD tells us which button is under the mouse) ----------
    if (GameMode->HoveredItem >= 0)
    {
        if (PlayerController->WasInputKeyJustPressed(EKeys::LeftMouseButton))
        {
            GameMode->SelectedItem = GameMode->HoveredItem;
            GameMode->ActivateSelected();
        }
        else if (PlayerController->WasInputKeyJustPressed(EKeys::RightMouseButton))
        {
            // right-click a setting to lower it
            GameMode->SelectedItem = GameMode->HoveredItem;
            GameMode->ChangeSetting(-1);
        }
    }
}
