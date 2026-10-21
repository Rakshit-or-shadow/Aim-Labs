#include "GEWeaponComponent.h"
#include "RayTraceMath.h"
#include "GEHitRegistry.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "DrawDebugHelpers.h"

namespace
{
	/** Speed range (cm/s) above StillSpeedThreshold over which run spread grows to its full value. */
	const float SpeedRangeForFullSpread = 250.0f;
	/** Longest tracer drawn when a shot hits nothing (cm). */
	const float MissTracerLength = 8000.0f;
	/** Pitch limit for the recoil kick so the camera never flips (degrees). */
	const float MaxLookPitch = 85.0f;
	/** Sample rate of the generated sounds. */
	const int32 SoundSampleRate = 22050;
}

UGEWeaponComponent::UGEWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	BuildDefaultWeapons();
}

// --------------------------------------------------------------------------
// Weapon definitions (5 original weapons). Edit numbers here to tune.
// --------------------------------------------------------------------------
void UGEWeaponComponent::BuildDefaultWeapons()
{
	Weapons.Reset();

	// 1. Sidekick: semi-auto sidearm, very accurate on the first shot.
	{
		FGEWeaponData W;
		W.Name = TEXT("Sidekick");
		W.bFullAuto = false;
		W.Damage = 25.0f; W.HeadMultiplier = 4.0f; W.BodyMultiplier = 1.0f; W.LegMultiplier = 0.75f;
		W.RoundsPerMinute = 400.0f; W.MagazineSize = 12; W.StartReserve = 48; W.ReloadTime = 1.6f;
		W.FirstShotSpread = 0.03f; W.SpreadPerShot = 0.7f; W.MaxSpraySpread = 3.0f;
		W.SprayRecoveryPerSecond = 7.0f; W.SprayRecoveryDelay = 0.12f;
		W.RunSpread = 4.0f; W.AirSpread = 6.0f;
		W.RecoilPattern = { FGERecoilStep(1.2f, 0.0f), FGERecoilStep(1.4f, 0.2f), FGERecoilStep(1.5f, -0.2f) };
		W.RecoilResetTime = 0.3f;
		W.AdsFov = 75.0f;
		W.BodySize = FVector(16.0f, 3.5f, 6.0f); W.BarrelLength = 6.0f; W.BarrelThickness = 2.0f;
		W.Color = FLinearColor(0.12f, 0.12f, 0.14f);
		W.ShotToneHz = 220.0f; W.ShotSoundLength = 0.10f;
		Weapons.Add(W);
	}
	// 2. Rattler: fast SMG, big spread.
	{
		FGEWeaponData W;
		W.Name = TEXT("Rattler");
		W.bFullAuto = true;
		W.Damage = 17.0f; W.HeadMultiplier = 3.0f; W.BodyMultiplier = 1.0f; W.LegMultiplier = 0.8f;
		W.RoundsPerMinute = 850.0f; W.MagazineSize = 30; W.StartReserve = 120; W.ReloadTime = 1.9f;
		W.FirstShotSpread = 0.2f; W.SpreadPerShot = 0.3f; W.MaxSpraySpread = 5.0f;
		W.SprayRecoveryPerSecond = 6.0f; W.SprayRecoveryDelay = 0.08f;
		W.RunSpread = 5.0f; W.AirSpread = 8.0f;
		W.RecoilPattern = {
			FGERecoilStep(0.5f, 0.0f), FGERecoilStep(0.6f, 0.1f), FGERecoilStep(0.7f, 0.2f), FGERecoilStep(0.7f, 0.3f),
			FGERecoilStep(0.7f, 0.2f), FGERecoilStep(0.6f, -0.1f), FGERecoilStep(0.6f, -0.3f), FGERecoilStep(0.6f, -0.4f),
			FGERecoilStep(0.5f, -0.2f), FGERecoilStep(0.5f, 0.2f), FGERecoilStep(0.5f, 0.4f), FGERecoilStep(0.5f, 0.3f) };
		W.RecoilResetTime = 0.25f;
		W.AdsFov = 72.0f;
		W.BodySize = FVector(22.0f, 4.5f, 8.0f); W.BarrelLength = 8.0f; W.BarrelThickness = 2.2f;
		W.Color = FLinearColor(0.15f, 0.18f, 0.12f);
		W.ShotToneHz = 160.0f; W.ShotSoundLength = 0.08f;
		Weapons.Add(W);
	}
	// 3. Longarm: assault rifle. One-shot headshot (35 x 3 = 105), strong recoil.
	{
		FGEWeaponData W;
		W.Name = TEXT("Longarm");
		W.bFullAuto = true;
		W.Damage = 35.0f; W.HeadMultiplier = 3.0f; W.BodyMultiplier = 1.0f; W.LegMultiplier = 0.8f;
		W.RoundsPerMinute = 600.0f; W.MagazineSize = 25; W.StartReserve = 100; W.ReloadTime = 2.4f;
		W.FirstShotSpread = 0.05f; W.SpreadPerShot = 0.4f; W.MaxSpraySpread = 4.0f;
		W.SprayRecoveryPerSecond = 5.0f; W.SprayRecoveryDelay = 0.1f;
		W.RunSpread = 5.5f; W.AirSpread = 8.0f;
		W.RecoilPattern = {
			FGERecoilStep(1.0f, 0.0f), FGERecoilStep(1.3f, 0.1f), FGERecoilStep(1.5f, 0.3f), FGERecoilStep(1.6f, 0.5f),
			FGERecoilStep(1.5f, 0.4f), FGERecoilStep(1.3f, -0.2f), FGERecoilStep(1.2f, -0.6f), FGERecoilStep(1.1f, -0.8f),
			FGERecoilStep(1.0f, -0.5f), FGERecoilStep(0.9f, 0.3f), FGERecoilStep(0.9f, 0.7f), FGERecoilStep(0.9f, 0.6f) };
		W.RecoilResetTime = 0.4f;
		W.AdsFov = 60.0f;
		W.BodySize = FVector(30.0f, 4.5f, 9.0f); W.BarrelLength = 14.0f; W.BarrelThickness = 2.4f;
		W.Color = FLinearColor(0.2f, 0.16f, 0.1f);
		W.ShotToneHz = 130.0f; W.ShotSoundLength = 0.13f;
		Weapons.Add(W);
	}
	// 4. Eagle Eye: scoped marksman rifle, slow, very accurate when still.
	{
		FGEWeaponData W;
		W.Name = TEXT("Eagle Eye");
		W.bFullAuto = false;
		W.Damage = 90.0f; W.HeadMultiplier = 3.0f; W.BodyMultiplier = 1.0f; W.LegMultiplier = 0.8f;
		W.RoundsPerMinute = 48.0f; W.MagazineSize = 5; W.StartReserve = 25; W.ReloadTime = 3.0f;
		W.MaxRange = 30000.0f;
		W.FirstShotSpread = 0.02f; W.SpreadPerShot = 1.0f; W.MaxSpraySpread = 2.0f;
		W.SprayRecoveryPerSecond = 3.0f; W.SprayRecoveryDelay = 0.5f;
		W.RunSpread = 9.0f; W.AirSpread = 12.0f; W.StillSpeedThreshold = 120.0f;
		W.AdsSpreadMultiplier = 0.1f;
		W.RecoilPattern = { FGERecoilStep(3.0f, 0.2f) };
		W.RecoilResetTime = 0.6f;
		W.AdsFov = 25.0f; W.bHasScope = true;
		W.BodySize = FVector(36.0f, 4.0f, 8.0f); W.BarrelLength = 18.0f; W.BarrelThickness = 2.0f;
		W.Color = FLinearColor(0.1f, 0.14f, 0.2f);
		W.ShotToneHz = 100.0f; W.ShotSoundLength = 0.18f;
		Weapons.Add(W);
	}
	// 5. Scatter: pump shotgun, 9 pellets each its own ray.
	{
		FGEWeaponData W;
		W.Name = TEXT("Scatter");
		W.bFullAuto = false;
		W.Damage = 11.0f; W.HeadMultiplier = 2.0f; W.BodyMultiplier = 1.0f; W.LegMultiplier = 0.8f;
		W.RoundsPerMinute = 70.0f; W.MagazineSize = 6; W.StartReserve = 30; W.ReloadTime = 2.6f;
		W.Pellets = 9; W.PelletSpread = 3.5f; W.MaxRange = 6000.0f;
		W.FirstShotSpread = 0.2f; W.SpreadPerShot = 1.0f; W.MaxSpraySpread = 2.0f;
		W.SprayRecoveryPerSecond = 4.0f; W.SprayRecoveryDelay = 0.4f;
		W.RunSpread = 3.0f; W.AirSpread = 4.0f;
		W.AdsSpreadMultiplier = 0.8f;
		W.RecoilPattern = { FGERecoilStep(4.0f, 0.3f), FGERecoilStep(4.5f, -0.3f) };
		W.RecoilResetTime = 0.6f;
		W.AdsFov = 78.0f;
		W.BodySize = FVector(28.0f, 5.0f, 8.0f); W.BarrelLength = 16.0f; W.BarrelThickness = 3.0f;
		W.Color = FLinearColor(0.25f, 0.1f, 0.08f);
		W.ShotToneHz = 90.0f; W.ShotSoundLength = 0.2f;
		Weapons.Add(W);
	}
}

// --------------------------------------------------------------------------
// Setup
// --------------------------------------------------------------------------
void UGEWeaponComponent::Init(UCameraComponent* InCamera)
{
	Camera = InCamera;
	if (Weapons.Num() == 0)
	{
		BuildDefaultWeapons();
	}
	MagAmmo.SetNum(Weapons.Num());
	ReserveAmmo.SetNum(Weapons.Num());
	RefillAllAmmo();

	if (Camera)
	{
		BaseFov = Camera->FieldOfView;
		CreateViewmodel();
	}
	CurrentIndex = 0;
	ApplyViewmodelShape();
	OnWeaponChanged.Broadcast(CurrentIndex);
}

void UGEWeaponComponent::CreateViewmodel()
{
	AActor* Owner = GetOwner();
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CylMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	ViewmodelRoot = NewObject<USceneComponent>(Owner, TEXT("GE_ViewmodelRoot"));
	ViewmodelRoot->SetupAttachment(Camera);
	ViewmodelRoot->RegisterComponent();
	ViewmodelRoot->SetRelativeLocation(ViewmodelHipOffset);

	// Helper to make one mesh piece with no collision and no shadow.
	auto MakePiece = [&](const TCHAR* Name, UStaticMesh* Mesh) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner, Name);
		C->SetStaticMesh(Mesh);
		C->SetupAttachment(ViewmodelRoot);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->RegisterComponent();
		if (BaseMat)
		{
			C->SetMaterial(0, UMaterialInstanceDynamic::Create(BaseMat, C));
		}
		return C;
	};
	GunBody = MakePiece(TEXT("GE_GunBody"), CubeMesh);
	GunBarrel = MakePiece(TEXT("GE_GunBarrel"), CylMesh);
	FlashShape = MakePiece(TEXT("GE_MuzzleShape"), SphereMesh);
	if (UMaterialInstanceDynamic* M = Cast<UMaterialInstanceDynamic>(FlashShape->GetMaterial(0)))
	{
		M->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.85f, 0.3f));
	}
	FlashShape->SetVisibility(false);

	FlashLight = NewObject<UPointLightComponent>(Owner, TEXT("GE_MuzzleLight"));
	FlashLight->SetupAttachment(FlashShape);
	FlashLight->SetIntensity(6000.0f);
	FlashLight->SetLightColor(FLinearColor(1.0f, 0.7f, 0.3f));
	FlashLight->SetAttenuationRadius(700.0f);
	FlashLight->SetCastShadows(false);
	FlashLight->RegisterComponent();
	FlashLight->SetVisibility(false);
}

void UGEWeaponComponent::ApplyViewmodelShape()
{
	if (!GunBody || !GunBarrel || !FlashShape || Weapons.Num() == 0)
	{
		return;
	}
	const FGEWeaponData& W = GetCurrentWeapon();

	GunBody->SetRelativeScale3D(W.BodySize / 100.0f);
	GunBody->SetRelativeLocation(FVector(W.BodySize.X * 0.5f, 0, 0));

	// Cylinder mesh stands along Z, so pitch it 90 degrees to point it forward.
	GunBarrel->SetRelativeRotation(FRotator(90.0f, 0, 0));
	GunBarrel->SetRelativeScale3D(FVector(W.BarrelThickness / 100.0f, W.BarrelThickness / 100.0f, W.BarrelLength / 100.0f));
	GunBarrel->SetRelativeLocation(FVector(W.BodySize.X + W.BarrelLength * 0.5f, 0, W.BodySize.Z * 0.25f));

	FlashShape->SetRelativeScale3D(FVector(0.05f));
	FlashShape->SetRelativeLocation(FVector(W.BodySize.X + W.BarrelLength + 2.0f, 0, W.BodySize.Z * 0.25f));

	if (UMaterialInstanceDynamic* M = Cast<UMaterialInstanceDynamic>(GunBody->GetMaterial(0)))
	{
		M->SetVectorParameterValue(TEXT("Color"), W.Color);
	}
	if (UMaterialInstanceDynamic* M = Cast<UMaterialInstanceDynamic>(GunBarrel->GetMaterial(0)))
	{
		M->SetVectorParameterValue(TEXT("Color"), W.Color * 0.7f);
	}
}

// --------------------------------------------------------------------------
// Queries
// --------------------------------------------------------------------------
const FGEWeaponData& UGEWeaponComponent::GetCurrentWeapon() const
{
	static const FGEWeaponData Fallback;
	return Weapons.IsValidIndex(CurrentIndex) ? Weapons[CurrentIndex] : Fallback;
}

int32 UGEWeaponComponent::GetMagAmmo() const
{
	return MagAmmo.IsValidIndex(CurrentIndex) ? MagAmmo[CurrentIndex] : 0;
}

int32 UGEWeaponComponent::GetReserveAmmo() const
{
	return ReserveAmmo.IsValidIndex(CurrentIndex) ? ReserveAmmo[CurrentIndex] : 0;
}

float UGEWeaponComponent::GetCurrentSpreadDeg() const
{
	const FGEWeaponData& W = GetCurrentWeapon();
	float Spread = W.FirstShotSpread + SpraySpread;

	if (const ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		// Moving faster than "still" adds spread (walking stays accurate).
		const float Speed = Char->GetVelocity().Size2D();
		if (Speed > W.StillSpeedThreshold)
		{
			Spread += W.RunSpread * FMath::Clamp((Speed - W.StillSpeedThreshold) / SpeedRangeForFullSpread, 0.0f, 1.0f);
		}
		// In the air: big penalty.
		if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
		{
			if (!Move->IsMovingOnGround())
			{
				Spread += W.AirSpread;
			}
		}
		if (Char->bIsCrouched)
		{
			Spread *= W.CrouchSpreadMultiplier;
		}
	}
	Spread *= FMath::Lerp(1.0f, W.AdsSpreadMultiplier, ZoomAlpha);
	return Spread;
}

float UGEWeaponComponent::GetCurrentFov() const
{
	return FMath::Lerp(BaseFov, GetCurrentWeapon().AdsFov, ZoomAlpha);
}

bool UGEWeaponComponent::IsScoped() const
{
	return GetCurrentWeapon().bHasScope && ZoomAlpha > 0.85f;
}

float UGEWeaponComponent::GetTimeSinceHit() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() - LastHitTime : 999.0f;
}

void UGEWeaponComponent::RefillAllAmmo()
{
	for (int32 i = 0; i < Weapons.Num() && i < MagAmmo.Num(); ++i)
	{
		MagAmmo[i] = Weapons[i].MagazineSize;
		ReserveAmmo[i] = Weapons[i].StartReserve;
	}
	bReloading = false;
}

void UGEWeaponComponent::SetFiringEnabled(bool bEnabled)
{
	bFiringEnabled = bEnabled;
	if (!bEnabled)
	{
		bTriggerHeld = false;
	}
}

// --------------------------------------------------------------------------
// Input
// --------------------------------------------------------------------------
void UGEWeaponComponent::FirePressed()
{
	bTriggerHeld = true;
	TryFire();
}

void UGEWeaponComponent::FireReleased()
{
	bTriggerHeld = false;
}

void UGEWeaponComponent::ReloadPressed()
{
	StartReload();
}

void UGEWeaponComponent::SelectWeapon(int32 Index)
{
	if (!Weapons.IsValidIndex(Index) || Index == CurrentIndex)
	{
		return;
	}
	bReloading = false; // switching cancels a reload
	CurrentIndex = Index;
	SpraySpread = 0.0f;
	SprayCount = 0;
	NextFireTime = GetWorld()->GetTimeSeconds() + EquipTime;
	ApplyViewmodelShape();
	PlayTone(300.0f, 0.05f, 0.5f, 0.3f);
	OnWeaponChanged.Broadcast(CurrentIndex);
}

void UGEWeaponComponent::CycleWeapon(int32 Direction)
{
	if (Weapons.Num() == 0)
	{
		return;
	}
	const int32 Next = (CurrentIndex + Direction + Weapons.Num()) % Weapons.Num();
	SelectWeapon(Next);
}

// --------------------------------------------------------------------------
// Firing
// --------------------------------------------------------------------------
bool UGEWeaponComponent::TryAimTrainerShot(FVector& OutOrigin, FVector& OutDirection)
{
	if (!Camera || !GetWorld())
	{
		return false;
	}
	const FGEWeaponData& W = GetCurrentWeapon();
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextFireTime)
	{
		return false; // still between shots of this weapon
	}
	NextFireTime = Now + 60.0f / FMath::Max(W.RoundsPerMinute, 1.0f);
	LastShotTime = Now;

	// the first-shot ray: straight through the crosshair, no spread
	OutOrigin = Camera->GetComponentLocation();
	OutDirection = Camera->GetForwardVector();

	// looks and sound only (no ammo use, no spread growth, no recoil)
	FlashEndTime = Now + MuzzleFlashTime;
	if (FlashShape) { FlashShape->SetVisibility(true); }
	if (FlashLight) { FlashLight->SetVisibility(true); }
	KickBack = ViewmodelKickBack;
	PlayTone(W.ShotToneHz, W.ShotSoundLength, 0.7f, 0.35f);
	return true;
}

void UGEWeaponComponent::TryFire()
{
	if (!bFiringEnabled || bReloading || !Camera || !GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextFireTime)
	{
		return;
	}
	if (GetMagAmmo() <= 0)
	{
		if (GetReserveAmmo() > 0)
		{
			StartReload();
		}
		else
		{
			PlayTone(700.0f, 0.03f, 0.3f, 0.3f); // dry click
			NextFireTime = Now + 0.3f;
		}
		return;
	}
	FireOneShot();
}

void UGEWeaponComponent::FireOneShot()
{
	const FGEWeaponData& W = GetCurrentWeapon();
	const float Now = GetWorld()->GetTimeSeconds();

	const bool bFirstShot = (SprayCount == 0);
	const float Spread = GetCurrentSpreadDeg();

	MagAmmo[CurrentIndex]--;
	NextFireTime = Now + 60.0f / FMath::Max(W.RoundsPerMinute, 1.0f);
	LastShotTime = Now;

	// One ray per pellet, from the camera along its forward direction (+ random spread cone).
	const FVector Origin = Camera->GetComponentLocation();
	const FVector Forward = Camera->GetForwardVector();
	bool bAnyHit = false;
	for (int32 i = 0; i < W.Pellets; ++i)
	{
		const float Cone = Spread + (W.Pellets > 1 ? W.PelletSpread : 0.0f);
		const FVector Dir = FMath::VRandCone(Forward, FMath::DegreesToRadians(Cone));
		FireRay(W, Origin, Dir, Cone, bAnyHit);
	}

	OnShotFired.Broadcast(CurrentIndex, bFirstShot, Spread);

	// Spread grows, recoil kicks the camera, then the pattern advances.
	SpraySpread = FMath::Min(SpraySpread + W.SpreadPerShot, W.MaxSpraySpread);
	ApplyRecoil(W);
	SprayCount++;

	// Effects: flash, viewmodel kick, sound.
	FlashEndTime = Now + MuzzleFlashTime;
	if (FlashShape) { FlashShape->SetVisibility(true); }
	if (FlashLight) { FlashLight->SetVisibility(true); }
	KickBack = ViewmodelKickBack;
	PlayTone(W.ShotToneHz, W.ShotSoundLength, 0.7f, 0.6f);
}

void UGEWeaponComponent::FireRay(const FGEWeaponData& W, const FVector& Origin, const FVector& Dir, float SpreadDeg, bool& bAnyHit)
{
	UWorld* World = GetWorld();
	UGEHitRegistry* Registry = World ? World->GetSubsystem<UGEHitRegistry>() : nullptr;

	double BestT = W.MaxRange;
	AActor* BestActor = nullptr;
	IGEHittable* BestHittable = nullptr;
	EGEHitZone BestZone = EGEHitZone::Body;

	if (Registry)
	{
		TArray<FGEHitSphere> Spheres;
		for (const TWeakObjectPtr<AActor>& Weak : Registry->GetHittables())
		{
			AActor* Actor = Weak.Get();
			if (!Actor || Actor == GetOwner())
			{
				continue;
			}
			IGEHittable* Hittable = Cast<IGEHittable>(Actor);
			if (!Hittable || !Hittable->CanBeShot())
			{
				continue;
			}
			Spheres.Reset();
			Hittable->GetHitSpheres(Spheres);
			for (const FGEHitSphere& S : Spheres)
			{
				double T = 0.0;
				// My ray-sphere test decides every hit. Closest one wins.
				if (RayTraceMath::DoesIntersect(S.Center, S.Radius, Origin, Dir, T) && T < BestT)
				{
					BestT = T;
					BestActor = Actor;
					BestHittable = Hittable;
					BestZone = S.Zone;
				}
			}
		}
	}

	// Tracer from the muzzle to the hit point (or far away on a miss).
	const FVector Muzzle = FlashShape ? FlashShape->GetComponentLocation() : Origin;
	const FVector End = Origin + Dir * (BestActor ? (float)BestT : FMath::Min(W.MaxRange, MissTracerLength));
	DrawDebugLine(World, Muzzle, End, FColor(255, 235, 140), false, TracerTime, 0, 0.6f);

	if (BestActor && BestHittable)
	{
		float Mult = W.BodyMultiplier;
		if (BestZone == EGEHitZone::Head) { Mult = W.HeadMultiplier; }
		else if (BestZone == EGEHitZone::Leg) { Mult = W.LegMultiplier; }
		const float Damage = W.Damage * Mult;

		const bool bKilled = BestHittable->ApplyShotDamage(Damage, BestZone, GetOwner());
		OnShotHit.Broadcast(BestActor, BestZone, Damage, bKilled);
		NotifyHit(BestZone == EGEHitZone::Head, bKilled);
		bAnyHit = true;
	}
}

void UGEWeaponComponent::NotifyHit(bool bHead, bool bKill)
{
	LastHitTime = GetWorld()->GetTimeSeconds();
	bLastHitHead = bHead;
	bLastHitKill = bKill;
	PlayTone(bHead ? 1800.0f : 1000.0f, 0.07f, 0.0f, 0.35f);
}

void UGEWeaponComponent::ApplyRecoil(const FGEWeaponData& W)
{
	if (W.RecoilPattern.Num() == 0)
	{
		return;
	}
	const FGERecoilStep& Step = W.RecoilPattern[FMath::Min(SprayCount, W.RecoilPattern.Num() - 1)];
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (AController* C = Pawn->GetController())
		{
			// Kick the control rotation, which the camera follows (same as mouse input).
			FRotator Rot = C->GetControlRotation();
			Rot.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Rot.Pitch) + Step.Pitch, -MaxLookPitch, MaxLookPitch);
			Rot.Yaw += Step.Yaw;
			C->SetControlRotation(Rot);
		}
	}
}

void UGEWeaponComponent::StartReload()
{
	if (bReloading || !GetWorld() || !MagAmmo.IsValidIndex(CurrentIndex))
	{
		return;
	}
	const FGEWeaponData& W = GetCurrentWeapon();
	if (MagAmmo[CurrentIndex] >= W.MagazineSize || ReserveAmmo[CurrentIndex] <= 0)
	{
		return;
	}
	bReloading = true;
	ReloadEndTime = GetWorld()->GetTimeSeconds() + W.ReloadTime;
	PlayTone(450.0f, 0.05f, 0.6f, 0.3f);
}

void UGEWeaponComponent::FinishReload()
{
	bReloading = false;
	const FGEWeaponData& W = GetCurrentWeapon();
	const int32 Needed = W.MagazineSize - MagAmmo[CurrentIndex];
	const int32 Taken = FMath::Min(Needed, ReserveAmmo[CurrentIndex]);
	MagAmmo[CurrentIndex] += Taken;
	ReserveAmmo[CurrentIndex] -= Taken;
	PlayTone(600.0f, 0.05f, 0.6f, 0.3f);
}

// --------------------------------------------------------------------------
// Per-frame update
// --------------------------------------------------------------------------
void UGEWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Camera || !GetWorld() || Weapons.Num() == 0)
	{
		return;
	}
	const FGEWeaponData& W = GetCurrentWeapon();
	const float Now = GetWorld()->GetTimeSeconds();

	// Reload finished?
	if (bReloading && Now >= ReloadEndTime)
	{
		FinishReload();
	}

	// Hold-to-fire for automatic weapons.
	if (bTriggerHeld && W.bFullAuto)
	{
		TryFire();
	}

	// Spread recovers after a short delay; the recoil pattern restarts after a pause.
	if (Now - LastShotTime > W.SprayRecoveryDelay)
	{
		SpraySpread = FMath::Max(0.0f, SpraySpread - W.SprayRecoveryPerSecond * DeltaTime);
	}
	if (Now - LastShotTime > W.RecoilResetTime)
	{
		SprayCount = 0;
	}

	// ADS zoom blend and camera FOV.
	ZoomAlpha = FMath::FInterpTo(ZoomAlpha, bAimHeld ? 1.0f : 0.0f, DeltaTime, ZoomSpeed);
	Camera->SetFieldOfView(GetCurrentFov());

	// Muzzle flash off.
	if (Now >= FlashEndTime)
	{
		if (FlashShape && FlashShape->IsVisible()) { FlashShape->SetVisibility(false); }
		if (FlashLight && FlashLight->IsVisible()) { FlashLight->SetVisibility(false); }
	}

	UpdateViewmodel(DeltaTime);
}

void UGEWeaponComponent::UpdateViewmodel(float Dt)
{
	if (!ViewmodelRoot)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	KickBack = FMath::FInterpTo(KickBack, 0.0f, Dt, ViewmodelKickRecovery);

	FVector Pos = FMath::Lerp(ViewmodelHipOffset, ViewmodelAdsOffset, ZoomAlpha);
	Pos.X -= KickBack;

	// Reload animation: tilt the gun down and back up.
	FRotator Rot = FRotator::ZeroRotator;
	if (bReloading)
	{
		const float Total = FMath::Max(GetCurrentWeapon().ReloadTime, 0.01f);
		const float Progress = FMath::Clamp(1.0f - (ReloadEndTime - Now) / Total, 0.0f, 1.0f);
		const float Dip = FMath::Sin(Progress * PI);
		Rot.Pitch = -40.0f * Dip;
		Pos.Z -= 6.0f * Dip;
	}
	ViewmodelRoot->SetRelativeLocationAndRotation(Pos, Rot);

	// Hide the gun when looking through a scope.
	ViewmodelRoot->SetVisibility(!IsScoped(), true);
	if (FlashShape && Now >= FlashEndTime) { FlashShape->SetVisibility(false); }
}

// --------------------------------------------------------------------------
// Procedural sound: a decaying tone mixed with noise, generated as 16-bit PCM.
// --------------------------------------------------------------------------
void UGEWeaponComponent::PlayTone(float Hz, float Length, float NoiseMix, float Volume)
{
	if (!GetWorld() || Length <= 0.0f)
	{
		return;
	}
	const int32 NumSamples = FMath::Max(1, FMath::RoundToInt(Length * SoundSampleRate));
	TArray<int16> Samples;
	Samples.SetNumUninitialized(NumSamples);
	for (int32 i = 0; i < NumSamples; ++i)
	{
		const float T = (float)i / SoundSampleRate;
		const float Envelope = FMath::Exp(-5.0f * i / NumSamples);
		const float Tone = FMath::Sin(2.0f * PI * Hz * T);
		const float Noise = FMath::FRandRange(-1.0f, 1.0f);
		const float Value = (Tone * (1.0f - NoiseMix) + Noise * NoiseMix) * Envelope * Volume;
		Samples[i] = (int16)(FMath::Clamp(Value, -1.0f, 1.0f) * 32000.0f);
	}

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(GetTransientPackage());
	Wave->SetSampleRate(SoundSampleRate);
	Wave->NumChannels = 1;
	Wave->Duration = Length;
	Wave->SoundGroup = SOUNDGROUP_Default;
	Wave->bLooping = false;
	Wave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), NumSamples * sizeof(int16));
	UGameplayStatics::PlaySound2D(this, Wave);
}

float UGEWeaponComponent::GetReloadProgress() const
{
	if (!bReloading || GetWorld() == nullptr)
	{
		return 0.0f;
	}
	const float Total = FMath::Max(GetCurrentWeapon().ReloadTime, 0.01f);
	return FMath::Clamp(1.0f - (ReloadEndTime - GetWorld()->GetTimeSeconds()) / Total, 0.0f, 1.0f);
}
