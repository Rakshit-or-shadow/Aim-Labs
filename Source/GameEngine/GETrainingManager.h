#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GEHittable.h"
#include "GETrainingTypes.h"
#include "GETrainingCenter.h"
#include "GEFlickTypes.h"
#include "GECoachTypes.h"
#include "GETrainingManager.generated.h"

class UGEFlickTracker;
class UGESaveGame;
class UGECoach;
class AGEBot;
class AGECharacter;
class UGEWeaponComponent;

/** Fired when a session starts / stops. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGEOnTrainingSession);
/** Fired when a bot is killed during a session. bHeadshot = the killing bullet hit the head. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGEOnBotKilled, AGEBot*, Bot, bool, bHeadshot);

/**
 * Training session manager (Phase 3). One per world (a world subsystem), so any
 * code can reach it with UGETrainingManager::Get(this).
 *
 * Owns: mode / difficulty / weapon choice, the session on/off state, the bots,
 * the session statistics (hooked to the player's weapon events), headshot popups,
 * the menu open/closed state and the T teleport to the Training Center.
 *
 * Phase 4 (Flick Optimizer) hints:
 *  - mode EGETrainingMode::Flick makes the manager spawn NO bots; call SpawnFlickTarget() yourself.
 *  - listen to OnSessionStarted / OnSessionStopped / OnBotKilled, and read GetStats().
 *  - the menu lives in AGEHUD (DrawTrainingMenu); add buttons there and call the setters here.
 */
UCLASS()
class GAMEENGINE_API UGETrainingManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Convenience lookup. Returns null for worlds without the subsystem. */
	static UGETrainingManager* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGETrainingManager, STATGROUP_Tickables); }

	// ---- Events ----
	UPROPERTY(BlueprintAssignable, Category = "GE|Training")
	FGEOnTrainingSession OnSessionStarted;
	UPROPERTY(BlueprintAssignable, Category = "GE|Training")
	FGEOnTrainingSession OnSessionStopped;
	UPROPERTY(BlueprintAssignable, Category = "GE|Training")
	FGEOnBotKilled OnBotKilled;

	// ---- Setup (called by the Training Center actor) ----
	void RegisterCenter(AGETrainingCenter* InCenter) { Center = InCenter; }
	AGETrainingCenter* GetCenter() const { return Center.Get(); }

	// ---- Choices. Changing one while a session runs restarts the session (stats reset). ----
	UFUNCTION(BlueprintCallable, Category = "GE|Training")
	void SetMode(EGETrainingMode NewMode);
	UFUNCTION(BlueprintCallable, Category = "GE|Training")
	void SetDifficulty(EGEDifficulty NewDifficulty);
	/** Also switches the player's weapon immediately. */
	UFUNCTION(BlueprintCallable, Category = "GE|Training")
	void SetWeaponIndex(int32 NewIndex);

	UFUNCTION(BlueprintPure, Category = "GE|Training")
	EGETrainingMode GetMode() const { return Mode; }
	UFUNCTION(BlueprintPure, Category = "GE|Training")
	EGEDifficulty GetDifficulty() const { return Difficulty; }
	UFUNCTION(BlueprintPure, Category = "GE|Training")
	int32 GetWeaponIndex() const { return WeaponIndex; }
	const FGEDifficultyPreset& GetCurrentPreset() const;

	// ---- Session ----
	UFUNCTION(BlueprintCallable, Category = "GE|Training")
	void StartSession();
	UFUNCTION(BlueprintCallable, Category = "GE|Training")
	void StopSession();
	UFUNCTION(BlueprintPure, Category = "GE|Training")
	bool IsSessionRunning() const { return bSessionRunning; }
	UFUNCTION(BlueprintPure, Category = "GE|Training")
	const FGETrainingStats& GetStats() const { return Stats; }

	// ---- Menu (key M) ----
	UFUNCTION(BlueprintCallable, Category = "GE|Training")
	void ToggleMenu();
	UFUNCTION(BlueprintPure, Category = "GE|Training")
	bool IsMenuOpen() const { return bMenuOpen; }

	// ---- Teleport (key T) ----
	/** Teleports the character into the Training Center, or back to where it was. */
	void ToggleTeleport(AGECharacter* Character);
	UFUNCTION(BlueprintPure, Category = "GE|Training")
	bool IsInTrainingCenter() const { return bInTrainingCenter; }

	// ---- Single targets (Phase 4 flick mode) ----
	/**
	 * Spawns one static bot with its feet at WorldLocation, facing the player.
	 * It does not respawn when killed (it just stays dead, hidden). The manager destroys it on StopSession
	 * or when you call DestroyFlickTarget. Stats and OnBotKilled work for it like any other bot.
	 */
	AGEBot* SpawnFlickTarget(const FVector& WorldLocation);
	void DestroyFlickTarget(AGEBot* Bot);

	// ---- Popups (read by the HUD) ----
	void AddPopup(const FVector& WorldLocation, const FString& Text, const FLinearColor& Color);
	const TArray<FGEPopup>& GetPopups() const { return Popups; }
	float GetPopupDuration() const { return PopupDuration; }

	/** The player's weapon component, or null. */
	UGEWeaponComponent* GetPlayerWeapon() const;

	// ---- Phase 4: Flick Optimizer ----
	/** Summary of the last finished flick session (valid when HasFlickSummary()). Phase 5 coach: read this. */
	const FGEFlickSummary& GetLastFlickSummary() const { return LastFlickSummary; }
	bool HasFlickSummary() const { return bHasFlickSummary; }
	/** The tracker with the per-target records (path, overshoot...). */
	UGEFlickTracker* GetFlickTracker() const { return FlickTracker; }
	/** The saved profile (session history + sensitivity). Phase 5 coach: GetSaveGame()->GetSessions(). */
	UGESaveGame* GetSaveGame() const { return SaveGame; }

	// ---- Phase 4: mouse sensitivity (stored in the save file) ----
	/** Multiplier applied to look input by AGEPlayerController. 1.0 = unchanged. */
	float GetSensitivityScale() const { return SensitivityScale; }
	void SetSensitivityScale(float NewScale);
	int32 GetMouseDpi() const { return MouseDpi; }
	void SetMouseDpi(int32 NewDpi);
	/** Approximate centimetres of mouse travel for a full 360 turn, from DPI and scale (see GESensitivity). */
	float GetCm360() const { return Cm360ForScale(SensitivityScale); }
	float Cm360ForScale(float Scale) const;
	/** Sets the scale from the last flick suggestion. */
	void ApplyFlickSuggestion();

	// ---- Phase 5: AI Coach ----
	/** Tips + trends for the last finished session (valid when HasCoachReport()). */
	const FGECoachReport& GetCoachReport() const { return CoachReport; }
	bool HasCoachReport() const { return CoachReport.bValid; }

	/** Display names. */
	static FString ModeToString(EGETrainingMode M);

protected:
	/** Presets for Easy / Medium / Hard. Filled in Initialize; tweak here. */
	UPROPERTY()
	TArray<FGEDifficultyPreset> Presets;

	/** Seconds a popup stays visible. */
	float PopupDuration = 0.9f;

private:
	UFUNCTION()
	void HandleShotHit(AActor* Target, EGEHitZone Zone, float Damage, bool bKilled);
	UFUNCTION()
	void HandleShotFired(int32 WeaponIndexFired, bool bFirstShot, float SpreadDegrees);

	// Flick mode
	void TickFlick(float Now);
	void SpawnNextFlickTarget(float Now);
	void FinishSession();	// builds the flick summary and saves the session record
	void SaveProfile();
	/** Phase 5: runs the coach when a session stops. */
	UFUNCTION()
	void HandleSessionStopped();

	void BindToWeapon();
	void UnbindFromWeapon();
	void SpawnSessionBots();
	void DestroyAllBots();
	void PlayPing(float Hz, float Length, float Volume);
	APlayerController* GetPC() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGEBot>> Bots;
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGEBot>> FlickBots;

	UPROPERTY(Transient)
	TObjectPtr<UGEFlickTracker> FlickTracker;
	UPROPERTY(Transient)
	TObjectPtr<UGESaveGame> SaveGame;
	UPROPERTY(Transient)
	TObjectPtr<AGEBot> CurrentFlickTarget;

	UPROPERTY(Transient)
	TObjectPtr<UGECoach> Coach;
	FGECoachReport CoachReport;
	/** True when FinishSession saved a record for the session that just stopped. */
	bool bRecordSaved = false;

	FGEFlickSettings FlickSettings;
	FGEFlickSummary LastFlickSummary;
	bool bHasFlickSummary = false;
	float FlickTargetStartTime = 0.0f;
	float FlickNextSpawnTime = 0.0f;
	float FlickAnchorYaw = 0.0f;

	float SensitivityScale = 1.0f;
	int32 MouseDpi = GESensitivity::DefaultMouseDpi;

	TWeakObjectPtr<AGETrainingCenter> Center;
	TWeakObjectPtr<UGEWeaponComponent> BoundWeapon;

	TArray<FGEPopup> Popups;
	FGETrainingStats Stats;

	EGETrainingMode Mode = EGETrainingMode::Static;
	EGEDifficulty Difficulty = EGEDifficulty::Medium;
	int32 WeaponIndex = 0;

	bool bSessionRunning = false;
	bool bMenuOpen = false;
	bool bInTrainingCenter = false;
	/** True when a bullet of the trigger pull being processed hit a bot (hit events arrive before the shot event). */
	bool bPendingHit = false;

	FVector SavedLocation = FVector::ZeroVector;
	FRotator SavedRotation = FRotator::ZeroRotator;
};
