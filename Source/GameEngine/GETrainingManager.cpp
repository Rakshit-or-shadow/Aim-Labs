#include "GETrainingManager.h"
#include "GEBot.h"
#include "GECharacter.h"
#include "GETrainingCenter.h"
#include "GEWeaponComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
	const int32 PingSampleRate = 22050;
}

UGETrainingManager* UGETrainingManager::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UGETrainingManager>() : nullptr;
}

void UGETrainingManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Difficulty presets (index = EGEDifficulty). Tune the numbers here.
	Presets.Reset();

	FGEDifficultyPreset Easy;
	Easy.Name = TEXT("Easy");
	Easy.StrafeSpeed = 200.0f; Easy.StrafeMinTime = 0.8f; Easy.StrafeMaxTime = 1.6f; Easy.StrafePauseChance = 0.45f;
	Easy.PeekSpeed = 300.0f; Easy.PeekExposedTime = 1.6f; Easy.PeekHiddenTime = 1.5f;
	Presets.Add(Easy);

	FGEDifficultyPreset Medium;
	Medium.Name = TEXT("Medium");
	Medium.StrafeSpeed = 350.0f; Medium.StrafeMinTime = 0.5f; Medium.StrafeMaxTime = 1.1f; Medium.StrafePauseChance = 0.3f;
	Medium.PeekSpeed = 450.0f; Medium.PeekExposedTime = 1.0f; Medium.PeekHiddenTime = 1.2f;
	Presets.Add(Medium);

	FGEDifficultyPreset Hard;
	Hard.Name = TEXT("Hard");
	Hard.StrafeSpeed = 520.0f; Hard.StrafeMinTime = 0.2f; Hard.StrafeMaxTime = 0.6f; Hard.StrafePauseChance = 0.1f;
	Hard.PeekSpeed = 650.0f; Hard.PeekExposedTime = 0.5f; Hard.PeekHiddenTime = 0.9f;
	Presets.Add(Hard);
}

void UGETrainingManager::Deinitialize()
{
	UnbindFromWeapon();
	Super::Deinitialize();
}

const FGEDifficultyPreset& UGETrainingManager::GetCurrentPreset() const
{
	const int32 Index = FMath::Clamp((int32)Difficulty, 0, Presets.Num() - 1);
	return Presets[Index];
}

APlayerController* UGETrainingManager::GetPC() const
{
	return UGameplayStatics::GetPlayerController(GetWorld(), 0);
}

UGEWeaponComponent* UGETrainingManager::GetPlayerWeapon() const
{
	const AGECharacter* Char = Cast<AGECharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	return Char ? Char->GetWeaponComponent() : nullptr;
}

FString UGETrainingManager::ModeToString(EGETrainingMode M)
{
	switch (M)
	{
	case EGETrainingMode::Static: return TEXT("Static");
	case EGETrainingMode::Strafing: return TEXT("Strafing");
	case EGETrainingMode::Peek: return TEXT("Peek");
	default: return TEXT("Flick");
	}
}

// --------------------------------------------------------------------------
// Tick: keep the weapon events bound (the pawn may spawn after us), run the session clock, drop old popups.
// --------------------------------------------------------------------------
void UGETrainingManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!BoundWeapon.IsValid())
	{
		BindToWeapon();
	}
	if (bSessionRunning)
	{
		Stats.Duration += DeltaTime;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	Popups.RemoveAll([&](const FGEPopup& P) { return Now - P.SpawnTime > PopupDuration; });
}

void UGETrainingManager::BindToWeapon()
{
	UGEWeaponComponent* Weapon = GetPlayerWeapon();
	if (Weapon)
	{
		Weapon->OnShotHit.AddUniqueDynamic(this, &UGETrainingManager::HandleShotHit);
		Weapon->OnShotFired.AddUniqueDynamic(this, &UGETrainingManager::HandleShotFired);
		BoundWeapon = Weapon;
	}
}

void UGETrainingManager::UnbindFromWeapon()
{
	if (UGEWeaponComponent* Weapon = BoundWeapon.Get())
	{
		Weapon->OnShotHit.RemoveDynamic(this, &UGETrainingManager::HandleShotHit);
		Weapon->OnShotFired.RemoveDynamic(this, &UGETrainingManager::HandleShotFired);
	}
	BoundWeapon.Reset();
}

// --------------------------------------------------------------------------
// Statistics. Weapon events order for one trigger pull: OnShotHit (once per bullet that hit), then OnShotFired.
// So hits only set bPendingHit and the shot event turns it into one shot (+ one hit if pending).
// --------------------------------------------------------------------------
void UGETrainingManager::HandleShotHit(AActor* Target, EGEHitZone Zone, float Damage, bool bKilled)
{
	AGEBot* Bot = Cast<AGEBot>(Target);
	if (!Bot)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();

	// Headshot feedback works even outside a session.
	if (Zone == EGEHitZone::Head)
	{
		AddPopup(Bot->GetActorLocation() + FVector(0, 0, 200), TEXT("HEADSHOT"), FLinearColor(1.0f, 0.55f, 0.1f));
		PlayPing(2600.0f, 0.12f, 0.35f);
	}

	if (!bSessionRunning)
	{
		return;
	}
	bPendingHit = true;
	if (Bot->GetFirstHitTime() < 0.0f)
	{
		Bot->SetFirstHitTime(Now); // time-to-kill starts at the first hit on this bot
	}
	if (bKilled)
	{
		const bool bHeadshot = (Zone == EGEHitZone::Head);
		Stats.Kills++;
		if (bHeadshot)
		{
			Stats.HeadshotKills++;
		}
		Stats.TotalTimeToKill += Now - Bot->GetFirstHitTime();
		OnBotKilled.Broadcast(Bot, bHeadshot);
	}
}

void UGETrainingManager::HandleShotFired(int32 WeaponIndexFired, bool bFirstShot, float SpreadDegrees)
{
	if (bSessionRunning)
	{
		Stats.Shots++;
		if (bPendingHit)
		{
			Stats.Hits++;
		}
		if (bFirstShot)
		{
			Stats.FirstShots++;
			if (bPendingHit)
			{
				Stats.FirstShotHits++;
			}
		}
	}
	bPendingHit = false;
}

// --------------------------------------------------------------------------
// Choices
// --------------------------------------------------------------------------
void UGETrainingManager::SetMode(EGETrainingMode NewMode)
{
	Mode = NewMode;
	if (bSessionRunning) { StartSession(); }
}

void UGETrainingManager::SetDifficulty(EGEDifficulty NewDifficulty)
{
	Difficulty = NewDifficulty;
	if (bSessionRunning) { StartSession(); }
}

void UGETrainingManager::SetWeaponIndex(int32 NewIndex)
{
	WeaponIndex = NewIndex;
	if (UGEWeaponComponent* Weapon = GetPlayerWeapon())
	{
		Weapon->SelectWeapon(WeaponIndex);
	}
}

// --------------------------------------------------------------------------
// Session
// --------------------------------------------------------------------------
void UGETrainingManager::StartSession()
{
	DestroyAllBots();
	Stats = FGETrainingStats();
	bPendingHit = false;
	bSessionRunning = true;

	if (UGEWeaponComponent* Weapon = GetPlayerWeapon())
	{
		Weapon->RefillAllAmmo();
		Weapon->SelectWeapon(WeaponIndex);
	}
	SpawnSessionBots();
	OnSessionStarted.Broadcast();
}

void UGETrainingManager::StopSession()
{
	if (!bSessionRunning)
	{
		return;
	}
	bSessionRunning = false;
	DestroyAllBots();
	OnSessionStopped.Broadcast(); // Stats stay readable until the next session starts
}

void UGETrainingManager::SpawnSessionBots()
{
	AGETrainingCenter* TC = Center.Get();
	if (!TC || Mode == EGETrainingMode::Flick)
	{
		return; // Flick mode: Phase 4 spawns its own single targets
	}
	TArray<FGEBotSlot> Slots;
	TC->GetBotSlots(Mode, Slots);
	for (const FGEBotSlot& Slot : Slots)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AGEBot* Bot = GetWorld()->SpawnActor<AGEBot>(AGEBot::StaticClass(), Slot.Location, FRotator::ZeroRotator, Params);
		if (Bot)
		{
			Bot->Configure(Mode, GetCurrentPreset(), Slot.LateralAxis, Slot.PeekDirection);
			Bots.Add(Bot);
		}
	}
}

void UGETrainingManager::DestroyAllBots()
{
	for (AGEBot* Bot : Bots) { if (Bot) { Bot->Destroy(); } }
	Bots.Reset();
	for (AGEBot* Bot : FlickBots) { if (Bot) { Bot->Destroy(); } }
	FlickBots.Reset();
}

AGEBot* UGETrainingManager::SpawnFlickTarget(const FVector& WorldLocation)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGEBot* Bot = GetWorld()->SpawnActor<AGEBot>(AGEBot::StaticClass(), WorldLocation, FRotator::ZeroRotator, Params);
	if (Bot)
	{
		Bot->Configure(EGETrainingMode::Static, GetCurrentPreset(), FVector::RightVector, FVector::RightVector);
		Bot->SetRespawnEnabled(false);
		FlickBots.Add(Bot);
	}
	return Bot;
}

void UGETrainingManager::DestroyFlickTarget(AGEBot* Bot)
{
	FlickBots.Remove(Bot);
	if (Bot) { Bot->Destroy(); }
}

// --------------------------------------------------------------------------
// Menu: free the mouse, block look + shooting while open.
// --------------------------------------------------------------------------
void UGETrainingManager::ToggleMenu()
{
	APlayerController* PC = GetPC();
	if (!PC)
	{
		return;
	}
	bMenuOpen = !bMenuOpen;
	if (UGEWeaponComponent* Weapon = GetPlayerWeapon())
	{
		Weapon->SetFiringEnabled(!bMenuOpen);
	}
	PC->SetIgnoreLookInput(bMenuOpen);
	PC->bShowMouseCursor = bMenuOpen;
	PC->bEnableClickEvents = bMenuOpen; // lets AHUD hit boxes receive the clicks
	if (bMenuOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

// --------------------------------------------------------------------------
// Teleport (T): save the spot, jump to the Training Center; T again goes back.
// --------------------------------------------------------------------------
void UGETrainingManager::ToggleTeleport(AGECharacter* Character)
{
	AGETrainingCenter* TC = Center.Get();
	if (!Character || !TC)
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(Character->GetController());

	FVector TargetLocation;
	FRotator TargetRotation;
	if (!bInTrainingCenter)
	{
		SavedLocation = Character->GetActorLocation();
		SavedRotation = PC ? PC->GetControlRotation() : Character->GetActorRotation();
		TC->GetPlayerSpawn(TargetLocation, TargetRotation);
	}
	else
	{
		TargetLocation = SavedLocation;
		TargetRotation = SavedRotation;
	}

	Character->TeleportTo(TargetLocation, TargetRotation, false, true);
	Character->GetCharacterMovement()->StopMovementImmediately();
	if (PC)
	{
		PC->SetControlRotation(TargetRotation);
	}
	bInTrainingCenter = !bInTrainingCenter;
}

// --------------------------------------------------------------------------
// Popups and sound
// --------------------------------------------------------------------------
void UGETrainingManager::AddPopup(const FVector& WorldLocation, const FString& Text, const FLinearColor& Color)
{
	FGEPopup P;
	P.WorldLocation = WorldLocation;
	P.Text = Text;
	P.Color = Color;
	P.SpawnTime = GetWorld()->GetTimeSeconds();
	Popups.Add(P);
}

/** Short decaying sine "ping" as 16-bit PCM, played as a 2D sound. */
void UGETrainingManager::PlayPing(float Hz, float Length, float Volume)
{
	const int32 NumSamples = FMath::Max(1, FMath::RoundToInt(Length * PingSampleRate));
	TArray<int16> Samples;
	Samples.SetNumUninitialized(NumSamples);
	for (int32 i = 0; i < NumSamples; ++i)
	{
		const float T = (float)i / PingSampleRate;
		const float Envelope = FMath::Exp(-4.0f * i / NumSamples);
		Samples[i] = (int16)(FMath::Clamp(FMath::Sin(2.0f * PI * Hz * T) * Envelope * Volume, -1.0f, 1.0f) * 32000.0f);
	}
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(GetTransientPackage());
	Wave->SetSampleRate(PingSampleRate);
	Wave->NumChannels = 1;
	Wave->Duration = Length;
	Wave->SoundGroup = SOUNDGROUP_Default;
	Wave->bLooping = false;
	Wave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), NumSamples * sizeof(int16));
	UGameplayStatics::PlaySound2D(GetWorld(), Wave);
}
