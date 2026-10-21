#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GEHittable.h"
#include "GEWeaponData.h"
#include "GEWeaponComponent.generated.h"

class UCameraComponent;
class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class USoundWaveProcedural;

/** Fired once per trigger pull. bFirstShot = first bullet of a spray (spread had fully recovered). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGEOnShotFired, int32, WeaponIndex, bool, bFirstShot, float, SpreadDegrees);
/** Fired for every bullet / pellet that hits a hittable. Zone tells head / body / leg. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FGEOnShotHit, AActor*, Target, EGEHitZone, Zone, float, Damage, bool, bKilled);
/** Fired when the active weapon changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGEOnWeaponChanged, int32, WeaponIndex);

/**
 * Player weapon logic (Phase 2): 5 weapons, ammo, reload, spread, recoil, ADS,
 * viewmodel, muzzle flash, tracer, procedural sounds.
 * Every shot is a ray tested with RayTraceMath::DoesIntersect against the
 * sphere hitboxes of all IGEHittable actors (no line traces).
 * Lives on AGECharacter. The character forwards input to the On...Input functions.
 */
UCLASS(ClassGroup = (GE), meta = (BlueprintSpawnableComponent))
class GAMEENGINE_API UGEWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGEWeaponComponent();

	/** Called by AGECharacter::BeginPlay once the camera is known. */
	void Init(UCameraComponent* InCamera);

	// ---- Events for other systems (stats, bots, UI) ----
	UPROPERTY(BlueprintAssignable, Category = "GE|Weapon")
	FGEOnShotFired OnShotFired;
	UPROPERTY(BlueprintAssignable, Category = "GE|Weapon")
	FGEOnShotHit OnShotHit;
	UPROPERTY(BlueprintAssignable, Category = "GE|Weapon")
	FGEOnWeaponChanged OnWeaponChanged;

	// ---- Input entry points (called by the character) ----
	void FirePressed();
	void FireReleased();
	void AimPressed() { bAimHeld = true; }
	void AimReleased() { bAimHeld = false; }
	void ReloadPressed();
	void SelectWeapon(int32 Index);
	void CycleWeapon(int32 Direction);

	// ---- Queries ----
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	int32 GetCurrentIndex() const { return CurrentIndex; }
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	const FGEWeaponData& GetCurrentWeapon() const;
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	const TArray<FGEWeaponData>& GetWeapons() const { return Weapons; }
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	int32 GetMagAmmo() const;
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	int32 GetReserveAmmo() const;
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	bool IsReloading() const { return bReloading; }
	/** Current cone half-angle (degrees) a shot would use right now. */
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	float GetCurrentSpreadDeg() const;
	/** 0 = hip, 1 = fully zoomed. */
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	float GetZoomAlpha() const { return ZoomAlpha; }
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	float GetCurrentFov() const;
	/** True when the scope overlay should be drawn. */
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	bool IsScoped() const;
	/** Seconds since the last hit marker event (large if none). */
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	float GetTimeSinceHit() const;
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	bool WasLastHitHeadshot() const { return bLastHitHead; }
	UFUNCTION(BlueprintPure, Category = "GE|Weapon")
	bool WasLastHitKill() const { return bLastHitKill; }

	/** Refill every weapon's magazine and reserve (for training sessions). */
	UFUNCTION(BlueprintCallable, Category = "GE|Weapon")
	void RefillAllAmmo();

	/**
	 * Aim-trainer shot: gives the camera's exact forward ray (no spread) and uses the current
	 * weapon's fire rate, muzzle flash and kick. No ammo, no recoil. Returns false while on cooldown.
	 */
	bool TryAimTrainerShot(FVector& OutOrigin, FVector& OutDirection);

	/** Lets menus turn shooting off. */
	UFUNCTION(BlueprintCallable, Category = "GE|Weapon")
	void SetFiringEnabled(bool bEnabled);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	/** All weapons. Filled with the 5 defaults in the constructor; edit in code or Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GE|Weapon")
	TArray<FGEWeaponData> Weapons;

	/** Seconds before the next weapon can fire after switching. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GE|Weapon")
	float EquipTime = 0.3f;
	/** How fast ADS zoom blends in/out (per second). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GE|Weapon")
	float ZoomSpeed = 12.0f;
	/** Seconds the muzzle flash light/shape stays on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GE|Weapon")
	float MuzzleFlashTime = 0.05f;
	/** Seconds the tracer line stays visible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GE|Weapon")
	float TracerTime = 0.06f;
	/** Hip and ADS viewmodel positions relative to the camera (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GE|Weapon")
	FVector ViewmodelHipOffset = FVector(32.0f, 16.0f, -14.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GE|Weapon")
	FVector ViewmodelAdsOffset = FVector(28.0f, 0.0f, -9.0f);
	/** How far the viewmodel slides back per shot (cm) and how fast it returns. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GE|Weapon")
	float ViewmodelKickBack = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GE|Weapon")
	float ViewmodelKickRecovery = 18.0f;

private:
	void BuildDefaultWeapons();
	void TryFire();
	void FireOneShot();
	void FireRay(const FGEWeaponData& W, const FVector& Origin, const FVector& AimDir, float SpreadDeg, bool& bAnyHit);
	void StartReload();
	void FinishReload();
	void CreateViewmodel();
	void ApplyViewmodelShape();
	void UpdateViewmodel(float Dt);
	void ApplyRecoil(const FGEWeaponData& W);
	void NotifyHit(bool bHead, bool bKill);

	// Procedural sound helpers
	void PlayTone(float Hz, float Length, float NoiseMix, float Volume);

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ViewmodelRoot;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> GunBody;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> GunBarrel;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FlashShape;
	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> FlashLight;

	TArray<int32> MagAmmo;
	TArray<int32> ReserveAmmo;

	int32 CurrentIndex = 0;
	bool bFiringEnabled = true;
	bool bTriggerHeld = false;
	bool bAimHeld = false;
	bool bReloading = false;
	float ReloadEndTime = 0.0f;
	float NextFireTime = 0.0f;
	float LastShotTime = -100.0f;
	float SpraySpread = 0.0f;
	int32 SprayCount = 0;
	float ZoomAlpha = 0.0f;
	float BaseFov = 90.0f;
	float FlashEndTime = 0.0f;
	float KickBack = 0.0f;
	float LastHitTime = -100.0f;
	bool bLastHitHead = false;
	bool bLastHitKill = false;
};
