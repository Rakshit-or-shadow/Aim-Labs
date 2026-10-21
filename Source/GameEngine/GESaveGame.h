#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GETrainingTypes.h"
#include "GEFlickTypes.h"
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

	/** History cap. */
	static constexpr int32 MaxSessions = 200;

private:
	UPROPERTY()
	TArray<FGESessionRecord> Sessions;
};
