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

/** Movement tunables (Phase 1). Units: cm and seconds. Applied in the constructor and BeginPlay so they override the Blueprint. */
USTRUCT(BlueprintType)
struct FGEMovementSettings
{
	GENERATED_BODY()

	/** Normal run speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Movement")
	float RunSpeed = 540.0f;

	/** Walk speed as a fraction of RunSpeed (hold Shift). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Movement", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float WalkSpeedFraction = 0.4f;

	/** Crouch speed as a fraction of RunSpeed (hold Ctrl). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Movement", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float CrouchSpeedFraction = 0.3f;

	/** Ground acceleration. High = reaches full speed quickly. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Movement")
	float Acceleration = 4000.0f;

	/** Deceleration when no input. High = counter-strafing stops fast. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Movement")
	float BrakingDeceleration = 5000.0f;

	/** Ground friction. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Movement")
	float GroundFriction = 8.0f;

	/** Steering control while in the air (0 = none, 1 = full). Low on purpose. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AirControl = 0.05f;

	/** Jump launch speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Movement")
	float JumpZVelocity = 450.0f;

	/** Gravity multiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Movement")
	float GravityScale = 1.6f;

	/** Capsule half height while crouched (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Movement")
	float CrouchedHalfHeight = 56.0f;

	/** How fast the camera moves to its new height (interp speed). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Movement")
	float CameraHeightInterpSpeed = 12.0f;
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

	/** Horizontal speed in cm/s. */
	UFUNCTION(BlueprintPure, Category = "GE|Movement")
	float GetCurrentSpeed() const;

	/** True when standing on walkable ground. */
	UFUNCTION(BlueprintPure, Category = "GE|Movement")
	bool IsOnGround() const;

	/** True while the debug speed readout is switched on (F3). */
	UFUNCTION(BlueprintPure, Category = "GE|Movement")
	bool IsSpeedReadoutVisible() const { return bShowSpeedReadout; }

	/** The weapon logic component (Phase 2). */
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	class UGEWeaponComponent* GetWeaponComponent() const { return Weapons; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	/** Movement tunables. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Movement")
	FGEMovementSettings Movement;

	/** Designer-tunable stats. Edit in the Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Stats")
	FGECharacterStats Stats;

	/** Health right now. Set to Stats.MaxHealth when play begins. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "GE|Stats")
	float CurrentHealth = 0.0f;

private:
	friend struct FGEShotTour;
	/** Copies the Movement struct into CharacterMovement. */
	void ApplyMovementSettings();
	/** Creates the runtime input actions and mapping context. */
	void CreateRuntimeInput();

	void WalkPressed() { bWalkHeld = true; }
	void WalkReleased() { bWalkHeld = false; }
	void CrouchPressed() { Crouch(); }
	void CrouchReleased() { UnCrouch(); }
	void ToggleSpeedReadout() { bShowSpeedReadout = !bShowSpeedReadout; }

	UPROPERTY(Transient)
	TObjectPtr<class UInputAction> WalkAction;
	UPROPERTY(Transient)
	TObjectPtr<class UInputAction> CrouchAction;
	UPROPERTY(Transient)
	TObjectPtr<class UInputAction> SpeedReadoutAction;
	UPROPERTY(Transient)
	TObjectPtr<class UInputMappingContext> GEMappingContext;

	/** Weapon input actions (Phase 2). Slots 1-5 are in WeaponSlotActions. */
	UPROPERTY(Transient)
	TObjectPtr<class UInputAction> FireAction;
	UPROPERTY(Transient)
	TObjectPtr<class UInputAction> AimAction;
	UPROPERTY(Transient)
	TObjectPtr<class UInputAction> ReloadAction;
	UPROPERTY(Transient)
	TObjectPtr<class UInputAction> WheelUpAction;
	UPROPERTY(Transient)
	TObjectPtr<class UInputAction> WheelDownAction;
	UPROPERTY(Transient)
	TArray<TObjectPtr<class UInputAction>> WeaponSlotActions;

	UPROPERTY(Transient)
	TObjectPtr<class UInputAction> TeleportAction;
	UPROPERTY(Transient)
	TObjectPtr<class UInputAction> MenuAction;
	void TeleportPressed();
	void MenuPressed();

	void FirePressed();
	void FireReleased();
	void AimPressed();
	void AimReleased();
	void ReloadPressed();
	void WheelUp();
	void WheelDown();
	void SelectSlot(int32 Index);

	/** Owns ammo, spread, recoil, firing and the viewmodel. */
	UPROPERTY(VisibleAnywhere, Category = "GE|Weapon")
	TObjectPtr<class UGEWeaponComponent> Weapons;

	/** The BP camera, found by name "FirstPersonCamera". */
	UPROPERTY(Transient)
	TObjectPtr<class USceneComponent> CameraComponent;

	/** Camera relative Z when standing (read at BeginPlay). */
	float BaseCameraZ = 0.0f;
	bool bWalkHeld = false;
	bool bShowSpeedReadout = false;
};
