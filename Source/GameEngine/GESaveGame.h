#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GETrainingTypes.h"
#include "GEFlickTypes.h"
#include "GEAimTypes.h"
#include "GESaveGame.generated.h"

/** One finished training session as stored on disk. */
USTRUCT(BlueprintType)
struct FGESessionRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GE|Save")
	FDateTime Date;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Save")
	EGETrainingMode Mode = EGETrainingMode::Static;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Save")
	EGEDifficulty Difficulty = EGEDifficulty::Medium;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Save")
	int32 WeaponIndex = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Save")
	FString WeaponName;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Save")
	FGETrainingStats Stats;
	/** Sensitivity scale used during the session. */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Save")
	float SensitivityScale = 1.0f;
	/** Only filled when Mode == Flick. */
	UPROPERTY(BlueprintReadOnly, Category = "GE|Save")
	bool bHasFlick = false;
	UPROPERTY(BlueprintReadOnly, Category = "GE|Save")
	FGEFlickSummary Flick;
};

/**
 * Save file (Phase 4): the session history plus the stored mouse sensitivity.
 * Phase 5 coach: UGESaveGame::LoadOrCreate()->GetSessions() (oldest first),
 * or UGETrainingManager::GetSaveGame()->GetSessions() for the live copy.
 */
UCLASS()
class GAMEENGINE_API UGESaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Loads the profile from disk, or makes a new empty one. */
	static UGESaveGame* LoadOrCreate();
	/** Writes the profile to disk. Returns success. */
	bool Save();

	/** Appends a session (oldest are dropped past MaxSessions). */
	void AddSession(const FGESessionRecord& Record);
	/** All saved sessions, oldest first. */
	const TArray<FGESessionRecord>& GetSessions() const { return Sessions; }

	/** Look sensitivity multiplier (1.0 = default). */
	UPROPERTY()
	float SensitivityScale = 1.0f;
	UPROPERTY()
	int32 MouseDpi = GESensitivity::DefaultMouseDpi;

	// ---- Aim trainer settings and bests (merged from the old RayShot save) ----
	// Sensitivity is NOT duplicated here: the aim trainer uses SensitivityScale above.
	// Old saves simply load with these defaults.

	/** Best score per aim mode. Key = GEAimModes::GetName(mode), e.g. "Gridshot". */
	UPROPERTY()
	TMap<FString, float> BestScores;
	/** Field of view in degrees (60..120). */
	UPROPERTY()
	float FieldOfView = 103.0f;
	/** Sound volume 0..1 for the aim-trainer sounds. */
	UPROPERTY()
	float Volume = 0.7f;
	/** Index into GEStyle::GetCrosshairColor (0 = green). */
	UPROPERTY()
	int32 CrosshairColor = 0;
	/** UI theme: 0 Red (default), 1 Cyan, 2 Yellow, 3 Green. Old saves load as 0. */
	UPROPERTY()
	int32 UITheme = 0;
	/** Faint scanline overlay on every screen. Default OFF; old saves load as OFF. */
	UPROPERTY()
	bool bScanlines = false;
	/** Short colour-split glitch flash on screen changes. Default ON; old saves load as ON. */
	UPROPERTY()
	bool bGlitch = true;

	/** Gets the saved best for a mode. Returns false if the mode was never played. */
	bool GetBestScore(EGEAimMode Mode, float& OutBest) const;
	/**
	 * Records a finished round. Returns true if it is a new personal best
	 * (the first play of a mode always counts). Does NOT write to disk: call Save().
	 */
	bool SubmitScore(EGEAimMode Mode, float Score);

	/** History cap. */
	static constexpr int32 MaxSessions = 200;

private:
	UPROPERTY()
	TArray<FGESessionRecord> Sessions;
};
