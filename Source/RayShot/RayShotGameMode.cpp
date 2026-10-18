// RayShotGameMode.cpp

#include "RayShotGameMode.h"
#include "RayShotPawn.h"
#include "RayShotHUD.h"
#include "RayShotSaveGame.h"
#include "RayShotSounds.h"
#include "RayShotModes.h"
#include "RayShotStyle.h"
#include "TargetManager.h"
#include "Arena.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

// name of the save file (Saved/SaveGames/RayShotSave.sav)
static const TCHAR* RayShotSaveSlot = TEXT("RayShotSave");

ARayShotGameMode::ARayShotGameMode()
{
    // use our camera pawn and our HUD
    DefaultPawnClass = ARayShotPawn::StaticClass();
    HUDClass = ARayShotHUD::StaticClass();
}

void ARayShotGameMode::StartPlay()
{
    Super::StartPlay();

    LoadSaveData();
    SpawnArena();  // the neon room
    GetManager();  // spawns the target area

    Screen = ERayShotScreen::MainMenu;
    SelectedItem = 0;
    UpdateInputMode();
}

// =====================================================================
//                                MENUS
// =====================================================================

int32 ARayShotGameMode::GetNumMenuItems() const
{
    switch (Screen)
    {
    case ERayShotScreen::MainMenu: return RayShotModes::NumModes + 2; // modes + Settings + Quit
    case ERayShotScreen::Settings: return RayShotStyle::NumSettings + 1;            // settings + Back
    case ERayShotScreen::Results:  return 2;                          // Play Again + Main Menu
    default:                       return 0;
    }
}

void ARayShotGameMode::MoveSelection(int32 Direction)
{
    int32 NumItems = GetNumMenuItems();
    if (NumItems == 0)
    {
        return;
    }

    // wrap around at the top and bottom
    SelectedItem = (SelectedItem + Direction + NumItems) % NumItems;
    RayShotSounds::PlayClick(this, GetVolume());
}

void ARayShotGameMode::ActivateSelected()
{
    if (Screen == ERayShotScreen::MainMenu)
    {
        if (SelectedItem < RayShotModes::NumModes)
        {
            StartMode((ERayShotMode)SelectedItem);
        }
        else if (SelectedItem == RayShotModes::NumModes) // Settings
        {
            Screen = ERayShotScreen::Settings;
            SelectedItem = 0;
            RayShotSounds::PlayClick(this, GetVolume());
        }
        else // Quit
        {
            UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(),
                                           EQuitPreference::Quit, false);
        }
    }
    else if (Screen == ERayShotScreen::Settings)
    {
        if (SelectedItem < RayShotStyle::NumSettings)
        {
            ChangeSetting(+1); // clicking a setting increases it
        }
        else
        {
            Back();
        }
    }
    else if (Screen == ERayShotScreen::Results)
    {
        if (SelectedItem == 0)
        {
            StartMode(CurrentMode); // play again
        }
        else
        {
            GoToMainMenu();
        }
    }
}

void ARayShotGameMode::ChangeSetting(int32 Direction)
{
    // on the results screen, left/right just moves between the two buttons
    if (Screen == ERayShotScreen::Results)
    {
        MoveSelection(Direction);
        return;
    }

    if (Screen != ERayShotScreen::Settings || SaveData == nullptr)
    {
        return;
    }

    switch (SelectedItem)
    {
    case 0: // sensitivity: steps of 0.1
        SaveData->Sensitivity = FMath::Clamp(SaveData->Sensitivity + 0.1f * Direction, 0.1f, 10.0f);
        break;
    case 1: // field of view: steps of 5 degrees
        SaveData->FieldOfView = FMath::Clamp(SaveData->FieldOfView + 5.0f * Direction, 60.0f, 120.0f);
        break;
    case 2: // volume: steps of 10%
        SaveData->Volume = FMath::Clamp(SaveData->Volume + 0.1f * Direction, 0.0f, 1.0f);
        break;
    case 3: // crosshair color: cycle through the list
        SaveData->CrosshairColor = (SaveData->CrosshairColor + Direction + RayShotStyle::NumCrosshairColors)
                                   % RayShotStyle::NumCrosshairColors;
        break;
    default:
        return; // "Back" isn't a setting
    }

    WriteSaveData(); // save right away
    RayShotSounds::PlayClick(this, GetVolume());
}

void ARayShotGameMode::Back()
{
    switch (Screen)
    {
    case ERayShotScreen::Playing:
    case ERayShotScreen::Results:
        GoToMainMenu();
        break;

    case ERayShotScreen::Settings:
        WriteSaveData();
        Screen = ERayShotScreen::MainMenu;
        SelectedItem = RayShotModes::NumModes; // keep "Settings" highlighted
        RayShotSounds::PlayClick(this, GetVolume());
        break;

    case ERayShotScreen::MainMenu:
        break; // nothing to go back to
    }
}

// =====================================================================
//                                ROUNDS
// =====================================================================

void ARayShotGameMode::StartMode(ERayShotMode Mode)
{
    CurrentMode = Mode;
    bNewBest = false;
    Screen = ERayShotScreen::Playing;
    SelectedItem = 0;

    // look straight ahead at the start of every round
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (PlayerController)
    {
        ARayShotPawn* Player = Cast<ARayShotPawn>(PlayerController->GetPawn());
        if (Player)
        {
            Player->ResetAim();
        }
    }

    // the manager runs the 3-2-1 countdown and plays the start sound on "GO!"
    if (GetManager())
    {
        GetManager()->StartRound(Mode);
    }

    UpdateInputMode();
    RayShotSounds::PlayClick(this, GetVolume());
}

void ARayShotGameMode::GoToMainMenu()
{
    if (GetManager())
    {
        GetManager()->StopRound();
    }

    Screen = ERayShotScreen::MainMenu;
    SelectedItem = 0;
    UpdateInputMode();
}

void ARayShotGameMode::OnRoundFinished(float Score)
{
    LastScore = Score;
    bNewBest = false;

    if (SaveData)
    {
        FString Key = RayShotModes::GetName(CurrentMode);

        // is this better than the saved best?
        bool bBetter = false;
        if (!HasBestScore(CurrentMode))
        {
            bBetter = true; // first time playing this mode
        }
        else if (RayShotModes::LowerIsBetter(CurrentMode))
        {
            bBetter = Score < GetBestScore(CurrentMode);
        }
        else
        {
            bBetter = Score > GetBestScore(CurrentMode);
        }

        if (bBetter)
        {
            SaveData->BestScores.Add(Key, Score); // Add() also replaces an old value
            bNewBest = true;
            WriteSaveData();
        }
    }

    Screen = ERayShotScreen::Results;
    SelectedItem = 0;
    UpdateInputMode();

    if (bNewBest)
    {
        RayShotSounds::PlayNewBest(this, GetVolume());
    }
    else
    {
        RayShotSounds::PlayRoundEnd(this, GetVolume());
    }
}

// =====================================================================
//                         BEST SCORES + SETTINGS
// =====================================================================

bool ARayShotGameMode::HasBestScore(ERayShotMode Mode) const
{
    return SaveData && SaveData->BestScores.Contains(RayShotModes::GetName(Mode));
}

float ARayShotGameMode::GetBestScore(ERayShotMode Mode) const
{
    if (!HasBestScore(Mode))
    {
        return 0.0f;
    }
    return SaveData->BestScores[RayShotModes::GetName(Mode)];
}

FString ARayShotGameMode::GetBestScoreText(ERayShotMode Mode) const
{
    if (!HasBestScore(Mode))
    {
        return TEXT("-");
    }
    return RayShotModes::FormatScore(Mode, GetBestScore(Mode));
}

FLinearColor ARayShotGameMode::GetCrosshairColor() const
{
    return RayShotStyle::GetCrosshairColor(SaveData ? SaveData->CrosshairColor : 0);
}

// =====================================================================
//                               SAVING
// =====================================================================

void ARayShotGameMode::LoadSaveData()
{
    if (UGameplayStatics::DoesSaveGameExist(RayShotSaveSlot, 0))
    {
        SaveData = Cast<URayShotSaveGame>(UGameplayStatics::LoadGameFromSlot(RayShotSaveSlot, 0));
    }

    // no save file yet (first time playing) -> start with defaults
    if (SaveData == nullptr)
    {
        SaveData = Cast<URayShotSaveGame>(
            UGameplayStatics::CreateSaveGameObject(URayShotSaveGame::StaticClass()));
    }
}

void ARayShotGameMode::WriteSaveData()
{
    if (SaveData)
    {
        UGameplayStatics::SaveGameToSlot(SaveData, RayShotSaveSlot, 0);
    }
}

// =====================================================================
//                               HELPERS
// =====================================================================

void ARayShotGameMode::SpawnArena()
{
    // only one arena
    if (UGameplayStatics::GetActorOfClass(GetWorld(), AArena::StaticClass()) == nullptr)
    {
        GetWorld()->SpawnActor<AArena>(AArena::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    }
}

ATargetManager* ARayShotGameMode::GetManager()
{
    if (Manager == nullptr)
    {
        // use a TargetManager placed in the level if there is one...
        Manager = Cast<ATargetManager>(
            UGameplayStatics::GetActorOfClass(GetWorld(), ATargetManager::StaticClass()));

        // ...otherwise spawn one 10 m in front of the player
        if (Manager == nullptr)
        {
            Manager = GetWorld()->SpawnActor<ATargetManager>(
                ATargetManager::StaticClass(), FVector(1000.0, 0.0, 320.0), FRotator::ZeroRotator);
        }
    }
    return Manager;
}

void ARayShotGameMode::UpdateInputMode()
{
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (PlayerController == nullptr)
    {
        return;
    }

    if (Screen == ERayShotScreen::Playing)
    {
        // playing: hide the cursor, mouse turns the camera
        PlayerController->SetInputMode(FInputModeGameOnly());
        PlayerController->bShowMouseCursor = false;
    }
    else
    {
        // menus: show the cursor so you can click buttons
        FInputModeGameAndUI MenuInput;
        MenuInput.SetHideCursorDuringCapture(false);
        MenuInput.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PlayerController->SetInputMode(MenuInput);
        PlayerController->bShowMouseCursor = true;
    }
}

float ARayShotGameMode::GetVolume() const
{
    return SaveData ? SaveData->Volume : 0.7f;
}
