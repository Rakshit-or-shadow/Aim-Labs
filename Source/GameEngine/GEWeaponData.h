#pragma once

#include "CoreMinimal.h"
#include "GEWeaponData.generated.h"

/** One recoil step: how far the camera kicks for the Nth bullet of a spray (degrees). */
USTRUCT(BlueprintType)
struct FGERecoilStep
{
	GENERATED_BODY()

	/** Degrees the view kicks UP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Recoil")
	float Pitch = 0.0f;

	/** Degrees the view kicks sideways (+ right, - left). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Recoil")
	float Yaw = 0.0f;

	FGERecoilStep() {}
	FGERecoilStep(float InPitch, float InYaw) : Pitch(InPitch), Yaw(InYaw) {}
};

/**
 * All the numbers for one weapon (Phase 2). Units: cm, seconds, degrees.
 * Defaults for the 5 weapons are built in UGEWeaponComponent::BuildDefaultWeapons.
 */
USTRUCT(BlueprintType)
struct FGEWeaponData
{
	GENERATED_BODY()

	// ---- Identity ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Weapon")
	FString Name = TEXT("Weapon");

	/** Hold to keep firing (true) or one shot per click (false). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Weapon")
	bool bFullAuto = false;

	// ---- Damage ----
	/** Damage per bullet / pellet before the zone multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Damage")
	float Damage = 25.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Damage")
	float HeadMultiplier = 4.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Damage")
	float BodyMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Damage")
	float LegMultiplier = 0.75f;
	/** Max distance a shot can travel (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Damage")
	float MaxRange = 10000.0f;

	// ---- Fire rate / ammo ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Ammo")
	float RoundsPerMinute = 400.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Ammo")
	int32 MagazineSize = 12;
	/** Spare bullets outside the magazine at the start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Ammo")
	int32 StartReserve = 48;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Ammo")
	float ReloadTime = 1.5f;
	/** Bullets fired per trigger pull (shotgun > 1). Each is its own ray. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Ammo", meta = (ClampMin = "1"))
	int32 Pellets = 1;

	// ---- Accuracy (cone half-angles in degrees) ----
	/** Spread of the very first shot while standing / walking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Accuracy")
	float FirstShotSpread = 0.05f;
	/** Extra spread each shot adds while spraying. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Accuracy")
	float SpreadPerShot = 0.5f;
	/** Cap on the spray spread. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Accuracy")
	float MaxSpraySpread = 3.0f;
	/** Spray spread removed per second once recovery starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Accuracy")
	float SprayRecoveryPerSecond = 8.0f;
	/** Seconds after a shot before spread starts recovering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Accuracy")
	float SprayRecoveryDelay = 0.1f;
	/** Extra spread at full run speed (none at or below StillSpeedThreshold). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Accuracy")
	float RunSpread = 4.0f;
	/** Extra spread while in the air. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Accuracy")
	float AirSpread = 6.0f;
	/** Speed (cm/s) up to which the shooter counts as "still". Walk speed is ~216, so walking is accurate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Accuracy")
	float StillSpeedThreshold = 250.0f;
	/** Spread multiplier while crouched. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Accuracy")
	float CrouchSpreadMultiplier = 0.7f;
	/** Spread multiplier while fully aimed down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Accuracy")
	float AdsSpreadMultiplier = 0.6f;
	/** Extra random cone for each pellet (shotgun). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Accuracy")
	float PelletSpread = 0.0f;

	// ---- Recoil ----
	/** Kick for bullet 0, 1, 2 ... of a spray. The last step repeats if the spray is longer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Recoil")
	TArray<FGERecoilStep> RecoilPattern;
	/** Seconds without shooting before the pattern restarts at step 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Recoil")
	float RecoilResetTime = 0.35f;

	// ---- Aim down sights ----
	/** Camera FOV (degrees) when fully zoomed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|ADS")
	float AdsFov = 70.0f;
	/** If true, a scope overlay is drawn when zoomed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|ADS")
	bool bHasScope = false;

	// ---- Viewmodel (basic shapes) ----
	/** Body box size in cm (X = length forward). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Viewmodel")
	FVector BodySize = FVector(24.0f, 4.0f, 7.0f);
	/** Barrel length and thickness in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Viewmodel")
	float BarrelLength = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Viewmodel")
	float BarrelThickness = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Viewmodel")
	FLinearColor Color = FLinearColor(0.08f, 0.08f, 0.1f);
	/** Sound pitch (Hz of the tone part of the shot sound). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Audio")
	float ShotToneHz = 180.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GE|Audio")
	float ShotSoundLength = 0.12f;
};
