#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GECharacter.generated.h"

/**
 * Tunable stats for a GE character. Kept in one struct so designers can edit
 * them in the Blueprint's Class Defaults and later phases can add fields here.
 */
USTRUCT(BlueprintType)
struct FGECharacterStats
{
	GENERATED_BODY()

	/** Health the character starts with and can never exceed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Stats", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;
};

/**
 * Base C++ class for the player character.
 *
 * Deliberately minimal: BP_FirstPersonCharacter already owns the camera
 * ("FirstPersonCamera"), the arms mesh ("FirstPersonMesh") and all Enhanced
 * Input bindings in its graph. This class adds NO components and NO input so
 * nothing in the Blueprint is duplicated or overridden. It only provides stats
 * and hook functions that later phases (shooting, damage) will build on.
 */
UCLASS(Abstract, Blueprintable)
class GAMEENGINE_API AGECharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGECharacter();

	/** Returns the current health value. */
	UFUNCTION(BlueprintPure, Category = "GE|Stats")
	float GetCurrentHealth() const { return CurrentHealth; }

	/** Returns the stats struct (max health, etc.). */
	UFUNCTION(BlueprintPure, Category = "GE|Stats")
	const FGECharacterStats& GetStats() const { return Stats; }

	/**
	 * Hook called once at the end of BeginPlay, after stats are initialised.
	 * Override in C++ (OnCharacterReady_Implementation) or in Blueprint.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "GE|Hooks")
	void OnCharacterReady();

protected:
	virtual void BeginPlay() override;

	/** Designer-tunable stats. Edit in the Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Stats")
	FGECharacterStats Stats;

	/** Health right now. Set to Stats.MaxHealth when play begins. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GE|Stats")
	float CurrentHealth = 0.0f;
};
