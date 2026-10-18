// TargetManager.h
// Runs a round of any mode: countdown, moving the targets, handling shots,
// the score and the round timer. Between rounds it shows floating "showcase" targets.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RayShotTypes.h"
#include "TargetManager.generated.h"

class ATargetSphere;

// one row of stats, e.g. Label = "Accuracy", Value = "93.5%"
struct FStatLine
{
    FString Label;
    FString Value;
};

// floating text that appears where you hit a target (e.g. "+1" or "231 ms")
struct FScorePopup
{
    FVector Location;
    float SpawnTime = 0.0f;
    FString Text;
    FLinearColor Color;
};

UCLASS()
class RAYSHOT_API ATargetManager : public AActor
{
    GENERATED_BODY()

public:
    ATargetManager();

    virtual void Tick(float DeltaTime) override;

    // ---------- settings (you can change these in the editor) ----------
    UPROPERTY(EditAnywhere, Category = "Aim") int32 GridSize = 5;            // Gridshot: 5 x 5 grid
    UPROPERTY(EditAnywhere, Category = "Aim") double GridSpacing = 120.0;    // cm between grid cells
    UPROPERTY(EditAnywhere, Category = "Aim") double AreaHalfWidth = 300.0;  // random-spawn area (cm)
    UPROPERTY(EditAnywhere, Category = "Aim") double AreaHalfHeight = 200.0;
    UPROPERTY(EditAnywhere, Category = "Aim") int32 ReflexTargets = 10;      // Reflex: targets per round
    UPROPERTY(EditAnywhere, Category = "Aim") float CountdownLength = 3.0f;  // seconds of 3-2-1 before a round

    // ---------- called by the game mode / player ----------
    void StartRound(ERayShotMode NewMode);
    void StopRound();   // quit a round early (no score)

    // called every frame while playing (used by Tracking)
    void UpdateAim(const FVector& Origin, const FVector& Direction, float DeltaTime);

    // called when the player clicks
    void HandleClick(const FVector& Origin, const FVector& Direction);

    // ---------- info for the HUD ----------
    bool IsRunning() const { return bRunning; }
    bool IsCountingDown() const { return bRunning && bCountingDown; }
    float GetCountdownLeft() const { return CountdownLeft; }
    float GetTimeSinceGo() const;
    ERayShotMode GetMode() const { return Mode; }

    float GetScore() const;
    float GetAccuracy() const;
    FString GetTimerText() const;          // "42.3" or "3 / 10"
    float GetTimerFraction() const;        // 1 = full, 0 = done (for the progress bar)
    bool IsLowOnTime() const;              // last 5 seconds
    TArray<FStatLine> GetLiveStats() const;
    TArray<FStatLine> GetResultLines() const;
    FString GetCenterMessage() const;      // e.g. "Wait for it..." in Reflex
    const TArray<FScorePopup>& GetPopups() const { return Popups; }

    float LastHitTime = -100.0f; // world time of the last hit (for the hit marker)

protected:
    virtual void BeginPlay() override;

private:
    // all targets (we reuse them instead of spawning new ones)
    UPROPERTY()
    TArray<TObjectPtr<ATargetSphere>> Targets;

    // ---------- round state ----------
    ERayShotMode Mode = ERayShotMode::Gridshot;
    bool bRunning = false;
    float RoundLength = 60.0f; // seconds (0 = no time limit)
    float TimeLeft = 0.0f;
    float ElapsedTime = 0.0f;
    int32 Hits = 0;
    int32 Shots = 0;

    // countdown
    bool bCountingDown = false;
    float CountdownLeft = 0.0f;
    int32 LastCountdownNumber = 0;
    float GoTime = -100.0f;

    // Tracking
    float OnTargetTime = 0.0f;
    float DirectionTimer = 0.0f;
    bool bTrackingOnTarget = false;

    // Reflex
    bool bReflexVisible = false;
    float ReflexWaitTimer = 0.0f;
    float ReflexAppearTime = 0.0f;
    float TooEarlyTime = -100.0f;
    int32 EarlyClicks = 0;
    TArray<float> ReactionTimesMs;

    // floating hit texts
    TArray<FScorePopup> Popups;

    // ---------- helpers ----------
    ATargetSphere* FindHitTarget(const FVector& Origin, const FVector& Direction) const;
    FVector GetFreeGridPoint() const;
    FVector GetRandomAreaPoint() const;
    void PickNewTrackingDirection(ATargetSphere* Target);
    void UpdateCountdown(float DeltaTime);
    void UpdateTracking(float DeltaTime);
    void UpdateReflex(float DeltaTime);
    void OnTargetHit(ATargetSphere* Target);
    void AddPopup(const FVector& Location, const FString& Text, const FLinearColor& Color);
    void FinishRound();
    void HideAllTargets();
    void ShowIdleTargets();
    float GetAverageReaction() const;
    float GetVolume() const;
    float Now() const;
};
