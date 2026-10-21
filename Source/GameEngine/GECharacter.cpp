#include "GECharacter.h"
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
	}
}

void AGECharacter::OnCharacterReady_Implementation()
{
	// Default does nothing. Later phases / Blueprints override this.
}
