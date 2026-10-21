// RayShotSaveGame.h
// Everything we save to disk: best scores and settings.
// Unreal writes it to  RayShot/Saved/SaveGames/RayShotSave.sav
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RayShotSaveGame.generated.h"

UCLASS()
class RAYSHOT_API URayShotSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    // best score per mode, key = mode name (e.g. "Gridshot")
    UPROPERTY()
    TMap<FString, float> BestScores;

    // settings
    UPROPERTY()
    float Sensitivity = 2.5f;

    UPROPERTY()
    float FieldOfView = 103.0f;

    UPROPERTY()
    float Volume = 0.7f; // 0 = muted, 1 = full

    UPROPERTY()
    int32 CrosshairColor = 0; // index into RayShotStyle::GetCrosshairColor
};
