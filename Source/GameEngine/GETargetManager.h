// GETargetManager.h
// Runs the aim trainer: moves the player onto the arena pad, plays a round of any mode
// (countdown, targets, shots, score, timer) and puts the player back when it is stopped.
// Between rounds it shows floating "showcase" targets.
//
// HOW TO USE (menus in a later stage call these):
//     AGETargetManager* Manager = AGETargetManager::Get(this);
//     Manager->StartMode(EGEAimMode::Gridshot);   // teleport to arena, freeze movement, 3-2-1-GO
//     Manager->StopMode();                        // leave: restores position and movement
// Console (for testing):  ge.aim.start <0-3 or mode name>   and   ge.aim.stop
// Keys (Tab, R, Enter...) are handled by AGEHUD, which owns the menu flow.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GEAimTypes.h"
#include "GETargetManager.generated.h"

class AGETargetSphere;
class AGEArena;
class ACharacter;
class UCameraComponent;
class UGEWeaponComponent;
class UGESaveGame;

/** Broadcast when a round ends by itself (time up or all Reflex targets done). Not sent by StopMode. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGEOnAimRoundFinished, EGEAimMode, Mode, float, Score, bool, bNewBest);

UCLASS()
class GAMEENGINE_API AGETargetManager : public AActor
{
	GENERATED_BODY()

public:
	AGETargetManager();

	virtual void Tick(float DeltaTime) override;

	/** Finds the manager that the game mode spawned (null outside the intro room). */
	static AGETargetManager* Get(const UObject* WorldContext);

	// ---------- settings (you can change these in the editor) ----------
	UPROPERTY(EditAnywhere, Category = "Aim") int32 GridSize = 5;            // Gridshot: 5 x 5 grid
	UPROPERTY(EditAnywhere, Category = "Aim") double GridSpacing = 120.0;    // cm between grid cells
	UPROPERTY(EditAnywhere, Category = "Aim") double AreaHalfWidth = 300.0;  // random-spawn area (cm)
	UPROPERTY(EditAnywhere, Category = "Aim") double AreaHalfHeight = 200.0;
	UPROPERTY(EditAnywhere, Category = "Aim") int32 ReflexTargets = 10;      // Reflex: targets per round
	UPROPERTY(EditAnywhere, Category = "Aim") float CountdownLength = 3.0f;  // seconds of 3-2-1 before a round
	/** How far in front of the player pad (cm, arena +X) the target area is centered. */
	UPROPERTY(EditAnywhere, Category = "Aim") double TargetDistance = 1000.0;

	/** Fired when a round finishes by itself. */
	UPROPERTY(BlueprintAssignable, Category = "Aim")
	FGEOnAimRoundFinished OnRoundFinished;

	// ---------- API for the menus ----------

	/** Tells the manager which arena to use (the game mode calls this once). */
	void SetArena(AGEArena* InArena) { Arena = InArena; }

	/**
	 * Starts a mode. The first call teleports the player onto the arena pad, freezes
	 * movement (aim only) and remembers where the player was. Calling it again while
	 * already in the arena just restarts the round (Play Again).
	 * Returns false if it cannot start (no player, no arena, or player is in the Training Center).
	 */
	bool StartMode(EGEAimMode NewMode, ACharacter* Player = nullptr);

	/** Leaves the arena: ends the round without a score, restores position, aim and movement. */
	void StopMode();

	/**
	 * Menu showcase: puts the player on the pad (if not there already), ends any round and shows the
	 * floating idle targets. While in this state the view drifts slowly (menu camera drift).
	 * Returns false if the player cannot enter the arena (no player, or in the Training Center).
	 */
	bool ShowShowcase(ACharacter* Who = nullptr);

	/** Pause menu: freezes the round (timers, targets, clicks) without ending it. */
	void SetPaused(bool bPause) { bPaused = bPause; }
	bool IsPaused() const { return bPaused; }

	/** True from StartMode until StopMode (countdown, playing and the results moment). */
	bool IsModeActive() const { return bInArena; }

	/** Called by the character's fire input in an aim mode: fires the weapon's first-shot ray. */
	void HandleFire();

	/** Starts the round logic only (no teleport). Normally you call StartMode instead. */
	void StartRound(EGEAimMode NewMode);
	/** Ends the round without a score and shows the idle targets. */
	void StopRound();

	/** Called every frame while playing (used by Tracking). Direction need not be normalized. */
	void UpdateAim(const FVector& Origin, const FVector& Direction, float DeltaTime);

	/** Called when the player clicks. Hit test is RayTraceMath::DoesIntersect. */
	void HandleClick(const FVector& Origin, const FVector& Direction);

	// ---------- info for the HUD / menus ----------
	bool IsRunning() const { return bRunning; }
	bool IsCountingDown() const { return bRunning && bCountingDown; }
	/** True after a round ended by itself, until the next StartMode / StopMode. */
	bool IsRoundFinished() const { return bRoundFinished; }
	bool WasNewBest() const { return bNewBest; }
	float GetLastScore() const { return LastScore; }
	float GetCountdownLeft() const { return CountdownLeft; }
	float GetTimeSinceGo() const;
	EGEAimMode GetMode() const { return Mode; }

	float GetScore() const;
	float GetAccuracy() const;
	FString GetTimerText() const;          // "42.3" or "3 / 10"
	float GetTimerFraction() const;        // 1 = full, 0 = done (for the progress bar)
	bool IsLowOnTime() const;              // last 5 seconds
	TArray<FGEStatLine> GetLiveStats() const;
	TArray<FGEStatLine> GetResultLines() const;
	FString GetCenterMessage() const;      // e.g. "Wait for it..." in Reflex
	const TArray<FGEScorePopup>& GetPopups() const { return Popups; }
	/** Saved best for a mode. Returns false if it was never played. */
	bool GetBestScore(EGEAimMode ForMode, float& OutBest) const;

	float LastHitTime = -100.0f; // world time of the last hit (for the hit marker)

protected:
	virtual void BeginPlay() override;

private:
	// all targets (we reuse them instead of spawning new ones)
	UPROPERTY()
	TArray<TObjectPtr<AGETargetSphere>> Targets;

	UPROPERTY()
	TObjectPtr<AGEArena> Arena;

	/** The player while in the arena. */
	UPROPERTY()
	TObjectPtr<ACharacter> Player;
	UPROPERTY()
	TObjectPtr<UCameraComponent> PlayerCamera;
	UPROPERTY()
	TObjectPtr<UGEWeaponComponent> PlayerWeapons;

	// where to put the player back
	bool bInArena = false;
	bool bPaused = false;
	FVector SavedLocation = FVector::ZeroVector;
	FRotator SavedControlRotation = FRotator::ZeroRotator;

	// ---------- round state ----------
	EGEAimMode Mode = EGEAimMode::Gridshot;
	bool bRunning = false;
	bool bRoundFinished = false;
	bool bNewBest = false;
	float LastScore = 0.0f;
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
	TArray<FGEScorePopup> Popups;

	// ---------- helpers ----------
	AGETargetSphere* FindHitTarget(const FVector& Origin, const FVector& Direction) const;
	FVector GetFreeGridPoint() const;
	FVector GetRandomAreaPoint() const;
	void PickNewTrackingDirection(AGETargetSphere* Target);
	void UpdateCountdown(float DeltaTime);
	void UpdateTracking(float DeltaTime);
	void UpdateReflex(float DeltaTime);
	void OnTargetHit(AGETargetSphere* Target);
	void AddPopup(const FVector& Location, const FString& Text, const FLinearColor& Color);
	void FinishRound();
	void HideAllTargets();
	void ShowIdleTargets();
	float GetAverageReaction() const;
	float GetVolume() const;
	float Now() const;
	UGESaveGame* GetSave() const;
	bool GetAimRay(FVector& OutOrigin, FVector& OutDirection) const;
	void FreezePlayer(bool bFreeze);
	/** Moves the player onto the arena pad (first call only). Returns false if it cannot. */
	bool EnterArena(ACharacter* Who);
};
