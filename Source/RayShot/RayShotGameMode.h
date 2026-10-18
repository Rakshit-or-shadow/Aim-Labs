// RayShotGameMode.h
// The "brain" of the game: which screen is showing, menus, settings,
// starting rounds and saving best scores to disk.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RayShotTypes.h"
#include "RayShotGameMode.generated.h"

class ATargetManager;
class URayShotSaveGame;

UCLASS()
class RAYSHOT_API ARayShotGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ARayShotGameMode();

    virtual void StartPlay() override;

    // ---------- current state ----------
    ERayShotScreen Screen = ERayShotScreen::MainMenu;
    ERayShotMode CurrentMode = ERayShotMode::Gridshot;
    int32 SelectedItem = 0;  // highlighted menu button
    int32 HoveredItem = -1;  // button under the mouse (-1 = none), set by the HUD
    bool bNewBest = false;   // did the last round beat the best score?
    float LastScore = 0.0f;

    // settings + best scores (loaded from disk)
    UPROPERTY()
    TObjectPtr<URayShotSaveGame> SaveData;

    // ---------- menus ----------
    int32 GetNumMenuItems() const;        // how many buttons the current screen has
    void MoveSelection(int32 Direction);  // -1 = up, +1 = down
    void ActivateSelected();              // Enter / click
    void ChangeSetting(int32 Direction);  // -1 = lower, +1 = higher (settings screen)
    void Back();                          // Tab / Esc

    // ---------- rounds ----------
    void StartMode(ERayShotMode Mode);
    void GoToMainMenu();
    void OnRoundFinished(float Score);    // called by TargetManager

    // ---------- best scores ----------
    bool HasBestScore(ERayShotMode Mode) const;
    float GetBestScore(ERayShotMode Mode) const;
    FString GetBestScoreText(ERayShotMode Mode) const;

    // ---------- settings ----------
    FLinearColor GetCrosshairColor() const;

    // ---------- helpers ----------
    ATargetManager* GetManager();
    void UpdateInputMode(); // shows the mouse cursor in menus, hides it while playing
    float GetVolume() const;

private:
    UPROPERTY()
    TObjectPtr<ATargetManager> Manager;

    void LoadSaveData();
    void WriteSaveData();
    void SpawnArena();
};
