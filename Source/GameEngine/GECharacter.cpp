#include "GECharacter.h"
#include "GEWeaponComponent.h"
#include "GETrainingManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

AGECharacter::AGECharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Allow crouching, then apply the movement numbers.
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	Weapons = CreateDefaultSubobject<UGEWeaponComponent>(TEXT("GEWeapons"));
	ApplyMovementSettings();
}

void AGECharacter::ApplyMovementSettings()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = Movement.RunSpeed;
	Move->MaxWalkSpeedCrouched = Movement.RunSpeed * Movement.CrouchSpeedFraction;
	Move->MaxAcceleration = Movement.Acceleration;
	Move->BrakingDecelerationWalking = Movement.BrakingDeceleration;
	Move->GroundFriction = Movement.GroundFriction;
	Move->AirControl = Movement.AirControl;
	Move->JumpZVelocity = Movement.JumpZVelocity;
	Move->GravityScale = Movement.GravityScale;
	Move->SetCrouchedHalfHeight(Movement.CrouchedHalfHeight);
}

void AGECharacter::BeginPlay()
{
	Super::BeginPlay();

	// Apply again so these values win over anything the Blueprint set.
	ApplyMovementSettings();

	// Find the Blueprint's camera by name.
	TArray<UCameraComponent*> Cameras;
	GetComponents<UCameraComponent>(Cameras);
	for (UCameraComponent* Cam : Cameras)
	{
		if (Cam && Cam->GetFName() == FName(TEXT("FirstPersonCamera")))
		{
			CameraComponent = Cam;
		}
	}
	if (!CameraComponent && Cameras.Num() > 0)
	{
		CameraComponent = Cameras[0];
	}
	if (CameraComponent)
	{
		BaseCameraZ = CameraComponent->GetRelativeLocation().Z;
	}
	if (Weapons)
	{
		Weapons->Init(Cast<UCameraComponent>(CameraComponent));
	}

	CurrentHealth = Stats.MaxHealth;
	OnCharacterReady();
}

void AGECharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Walk lowers max speed; crouch uses MaxWalkSpeedCrouched automatically.
	GetCharacterMovement()->MaxWalkSpeed = bWalkHeld ? Movement.RunSpeed * Movement.WalkSpeedFraction : Movement.RunSpeed;

	// Smoothly move the camera back toward its normal relative height.
	if (CameraComponent)
	{
		FVector Loc = CameraComponent->GetRelativeLocation();
		Loc.Z = FMath::FInterpTo(Loc.Z, BaseCameraZ, DeltaSeconds, Movement.CameraHeightInterpSpeed);
		CameraComponent->SetRelativeLocation(Loc);
	}
}

void AGECharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	// The capsule shrinks and drops instantly. Cancel the snap so Tick can ease it down.
	if (CameraComponent)
	{
		FVector Loc = CameraComponent->GetRelativeLocation();
		Loc.Z += HalfHeightAdjust;
		CameraComponent->SetRelativeLocation(Loc);
	}
}

void AGECharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (CameraComponent)
	{
		FVector Loc = CameraComponent->GetRelativeLocation();
		Loc.Z -= HalfHeightAdjust;
		CameraComponent->SetRelativeLocation(Loc);
	}
}

float AGECharacter::GetCurrentSpeed() const
{
	return GetVelocity().Size2D();
}

bool AGECharacter::IsOnGround() const
{
	return GetCharacterMovement()->IsMovingOnGround();
}

void AGECharacter::CreateRuntimeInput()
{
	if (GEMappingContext)
	{
		return;
	}
	WalkAction = NewObject<UInputAction>(this, TEXT("GE_Walk"));
	CrouchAction = NewObject<UInputAction>(this, TEXT("GE_Crouch"));
	SpeedReadoutAction = NewObject<UInputAction>(this, TEXT("GE_SpeedReadout"));

	GEMappingContext = NewObject<UInputMappingContext>(this, TEXT("GE_MappingContext"));
	GEMappingContext->MapKey(WalkAction, EKeys::LeftShift);
	GEMappingContext->MapKey(CrouchAction, EKeys::LeftControl);
	GEMappingContext->MapKey(SpeedReadoutAction, EKeys::F3);

	// Weapons: LMB fire, RMB aim (hold), R reload, 1-5 slots, mouse wheel cycles.
	FireAction = NewObject<UInputAction>(this, TEXT("GE_Fire"));
	AimAction = NewObject<UInputAction>(this, TEXT("GE_Aim"));
	ReloadAction = NewObject<UInputAction>(this, TEXT("GE_Reload"));
	WheelUpAction = NewObject<UInputAction>(this, TEXT("GE_WheelUp"));
	WheelDownAction = NewObject<UInputAction>(this, TEXT("GE_WheelDown"));
	GEMappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
	GEMappingContext->MapKey(AimAction, EKeys::RightMouseButton);
	GEMappingContext->MapKey(ReloadAction, EKeys::R);
	GEMappingContext->MapKey(WheelUpAction, EKeys::MouseScrollUp);
	GEMappingContext->MapKey(WheelDownAction, EKeys::MouseScrollDown);
	// Training Center: T teleports in / out, M opens the menu.
	TeleportAction = NewObject<UInputAction>(this, TEXT("GE_Teleport"));
	MenuAction = NewObject<UInputAction>(this, TEXT("GE_Menu"));
	GEMappingContext->MapKey(TeleportAction, EKeys::T);
	GEMappingContext->MapKey(MenuAction, EKeys::M);
	const FKey SlotKeys[5] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five };
	for (int32 i = 0; i < 5; ++i)
	{
		UInputAction* Slot = NewObject<UInputAction>(this, *FString::Printf(TEXT("GE_Slot%d"), i + 1));
		GEMappingContext->MapKey(Slot, SlotKeys[i]);
		WeaponSlotActions.Add(Slot);
	}
}

void AGECharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Keep whatever the parent / Blueprint sets up.
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	CreateRuntimeInput();

	// Add our mapping context alongside IMC_Default (our keys do not overlap).
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsystem->AddMappingContext(GEMappingContext, 1);
			}
		}
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(WalkAction, ETriggerEvent::Started, this, &AGECharacter::WalkPressed);
		EIC->BindAction(WalkAction, ETriggerEvent::Completed, this, &AGECharacter::WalkReleased);
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &AGECharacter::CrouchPressed);
		EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AGECharacter::CrouchReleased);
		EIC->BindAction(SpeedReadoutAction, ETriggerEvent::Started, this, &AGECharacter::ToggleSpeedReadout);
		EIC->BindAction(TeleportAction, ETriggerEvent::Started, this, &AGECharacter::TeleportPressed);
		EIC->BindAction(MenuAction, ETriggerEvent::Started, this, &AGECharacter::MenuPressed);
	}
}

void AGECharacter::OnCharacterReady_Implementation()
{
	// Default does nothing. Later phases / Blueprints override this.
}

void AGECharacter::FirePressed() { if (Weapons) { Weapons->FirePressed(); } }
void AGECharacter::FireReleased() { if (Weapons) { Weapons->FireReleased(); } }
void AGECharacter::AimPressed() { if (Weapons) { Weapons->AimPressed(); } }
void AGECharacter::AimReleased() { if (Weapons) { Weapons->AimReleased(); } }
void AGECharacter::ReloadPressed() { if (Weapons) { Weapons->ReloadPressed(); } }
void AGECharacter::WheelUp() { if (Weapons) { Weapons->CycleWeapon(-1); } }
void AGECharacter::WheelDown() { if (Weapons) { Weapons->CycleWeapon(1); } }
void AGECharacter::SelectSlot(int32 Index) { if (Weapons) { Weapons->SelectWeapon(Index); } }

void AGECharacter::TeleportPressed() { if (UGETrainingManager* M = UGETrainingManager::Get(this)) { M->ToggleTeleport(this); } }
void AGECharacter::MenuPressed() { if (UGETrainingManager* M = UGETrainingManager::Get(this)) { M->ToggleMenu(); } }
