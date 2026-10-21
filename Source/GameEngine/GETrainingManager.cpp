#include "GETrainingManager.h"
#include "GEBot.h"
#include "GECharacter.h"
#include "GETrainingCenter.h"
#include "GEWeaponComponent.h"
#include "GEFlickTracker.h"
#include "GESaveGame.h"
#include "GECoach.h"
#include "GameFramework/Character.h"
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

	// Phase 4: flick tracker and the saved profile (sensitivity + history).
	FlickTracker = NewObject<UGEFlickTracker>(this);
	FlickTracker->Settings = FlickSettings;
	SaveGame = UGESaveGame::LoadOrCreate();
	if (SaveGame)
	{
		SensitivityScale = FMath::Clamp(SaveGame->SensitivityScale, GESensitivity::MinScale, GESensitivity::MaxScale);
		MouseDpi = FMath::Clamp(SaveGame->MouseDpi, GESensitivity::MinDpi, GESensitivity::MaxDpi);
	}

	// Phase 5: the offline coach runs whenever a session stops.
	Coach = NewObject<UGECoach>(this);
	OnSessionStopped.AddDynamic(this, &UGETrainingManager::HandleSessionStopped);

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
		if (Mode == EGETrainingMode::Flick)
		{
			TickFlick(GetWorld()->GetTimeSeconds());
		}
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

	// Phase 5: bucket this bullet hit by distance (about 10 / 20 / 30 / 40 m) for the coach.
	if (const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
	{
		const float Meters = FVector::Dist2D(Player->GetActorLocation(), Bot->GetActorLocation()) / 100.0f;
		const int32 Bucket = FMath::Clamp(FMath::RoundToInt(Meters / GECoachTuning::BucketSizeMeters) - 1, 0, GENumDistanceBuckets - 1);
		Stats.HitsByDistance.SetNumZeroed(GENumDistanceBuckets);
		Stats.HeadHitsByDistance.SetNumZeroed(GENumDistanceBuckets);
		Stats.HitsByDistance[Bucket]++;
		if (Zone == EGEHitZone::Head) { Stats.HeadHitsByDistance[Bucket]++; }
	}
	if (FlickTracker && Bot == CurrentFlickTarget)
	{
		FlickTracker->NotifyHit(Now); // flick: time to hit
	}
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
	if (bSessionRunning && FlickTracker && CurrentFlickTarget)
	{
		FlickTracker->NotifyShot(GetWorld()->GetTimeSeconds()); // flick: time to first shot
	}
	if (bSessionRunning)
	{
		// Phase 5: was the player moving fast enough to be inaccurate?
		const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		const bool bMoving = Player && Player->GetVelocity().Size2D() > GECoachTuning::MovingSpeed;

		Stats.Shots++;
		if (bMoving) { Stats.ShotsMoving++; }
		if (bPendingHit)
		{
			Stats.Hits++;
			if (bMoving) { Stats.HitsMoving++; }
		}
		if (bFirstShot)
		{
			Stats.FirstShots++;
			if (bMoving) { Stats.FirstShotsMoving++; }
			if (bPendingHit)
			{
				Stats.FirstShotHits++;
				if (bMoving) { Stats.FirstShotHitsMoving++; }
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
	Stats.HitsByDistance.SetNumZeroed(GENumDistanceBuckets);
	Stats.HeadHitsByDistance.SetNumZeroed(GENumDistanceBuckets);
	CoachReport = FGECoachReport();
	bRecordSaved = false;
	bPendingHit = false;
	bSessionRunning = true;

	// Phase 4: reset flick state. The first target appears after a short delay.
	CurrentFlickTarget = nullptr;
	bHasFlickSummary = false;
	LastFlickSummary = FGEFlickSummary();
	if (FlickTracker) { FlickTracker->Reset(); }
	FlickNextSpawnTime = GetWorld()->GetTimeSeconds() + 1.0f;
	if (bInTrainingCenter && Center.IsValid())
	{
		FlickAnchorYaw = Center->GetActorRotation().Yaw;
	}
	else if (APlayerController* StartPC = GetPC())
	{
		FlickAnchorYaw = StartPC->GetControlRotation().Yaw;
	}

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
	FinishSession();
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

// --------------------------------------------------------------------------
// Phase 4: Flick Optimizer
// --------------------------------------------------------------------------

/** Runs the one-target-at-a-time loop: spawn, record the mouse, end on kill or timeout, wait, repeat. */
void UGETrainingManager::TickFlick(float Now)
{
	APlayerController* PC = GetPC();
	if (!PC || !FlickTracker)
	{
		return;
	}
	// While the menu is open the flick drill pauses (the current target is thrown away, not counted as a miss).
	if (bMenuOpen)
	{
		if (CurrentFlickTarget)
		{
			FlickTracker->AbortTarget();
			DestroyFlickTarget(CurrentFlickTarget);
			CurrentFlickTarget = nullptr;
		}
		FlickNextSpawnTime = Now + 1.0f;
		return;
	}

	if (CurrentFlickTarget)
	{
		FlickTracker->Sample(Now, PC->GetControlRotation());
		const bool bDead = !CurrentFlickTarget->IsAlive();
		const bool bTimedOut = (Now - FlickTargetStartTime) >= FlickSettings.TargetTimeout;
		if (bDead || bTimedOut)
		{
			FlickTracker->EndTarget(Now, !bDead);
			DestroyFlickTarget(CurrentFlickTarget);
			CurrentFlickTarget = nullptr;
			FlickNextSpawnTime = Now + FlickSettings.NextTargetDelay;
		}
	}
	else if (Now >= FlickNextSpawnTime)
	{
		SpawnNextFlickTarget(Now);
	}
}

/** Picks a random spot on the ground in front of the player (wide yaw range, varied height) and spawns a target there. */
void UGETrainingManager::SpawnNextFlickTarget(float Now)
{
	APlayerController* PC = GetPC();
	const ACharacter* Char = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PC || !Char)
	{
		return;
	}
	const FVector Origin = Char->GetActorLocation();
	const float GroundZ = Origin.Z - Char->GetSimpleCollisionHalfHeight();
	const AGETrainingCenter* TC = Center.Get();
	const bool bKeepInsideArena = bInTrainingCenter && TC;

	// Try a few random spots; reject those outside the Training Center floor.
	FVector Feet = Origin + FRotator(0.0f, FlickAnchorYaw, 0.0f).Vector() * FlickSettings.MinDistance;
	Feet.Z = GroundZ;
	for (int32 Try = 0; Try < 12; ++Try)
	{
		const float Azimuth = FMath::FRandRange(-FlickSettings.MaxAzimuthDeg, FlickSettings.MaxAzimuthDeg);
		const float Distance = FMath::FRandRange(FlickSettings.MinDistance, FlickSettings.MaxDistance);
		FVector Candidate = Origin + FRotator(0.0f, FlickAnchorYaw + Azimuth, 0.0f).Vector() * Distance;
		Candidate.Z = GroundZ;
		if (bKeepInsideArena)
		{
			const FVector Local = TC->GetActorTransform().InverseTransformPosition(Candidate);
			if (FMath::Abs(Local.Y) > FlickSettings.AreaHalfWidth || Local.X < FlickSettings.AreaMinX || Local.X > FlickSettings.AreaMaxX)
			{
				continue;
			}
		}
		Feet = Candidate;
		break;
	}
	// Some targets float for vertical (pitch) variety.
	if (FMath::FRand() < FlickSettings.RaiseChance)
	{
		Feet.Z += FMath::FRandRange(0.0f, FlickSettings.MaxRaise);
	}

	AGEBot* Target = SpawnFlickTarget(Feet);
	if (!Target)
	{
		return;
	}
	CurrentFlickTarget = Target;
	FlickTargetStartTime = Now;

	FVector ViewLoc;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewLoc, ViewRot);
	FlickTracker->BeginTarget(Now, ViewLoc, PC->GetControlRotation(), Feet + FVector(0.0f, 0.0f, FlickSettings.AimHeight));
}

/** Called when a session stops: builds the flick summary and appends a record to the save file. */
void UGETrainingManager::FinishSession()
{
	const bool bFlick = (Mode == EGETrainingMode::Flick) && FlickTracker;
	if (bFlick)
	{
		FlickTracker->AbortTarget(); // an unfinished target is not counted
		LastFlickSummary = FlickTracker->BuildSummary(SensitivityScale, GetCm360());
		bHasFlickSummary = LastFlickSummary.TotalTargets > 0;
	}
	CurrentFlickTarget = nullptr;

	if (SaveGame && (Stats.Shots > 0 || bHasFlickSummary))
	{
		FGESessionRecord Record;
		Record.Date = FDateTime::Now();
		Record.Mode = Mode;
		Record.Difficulty = Difficulty;
		Record.WeaponIndex = WeaponIndex;
		if (const UGEWeaponComponent* Weapon = GetPlayerWeapon())
		{
			if (Weapon->GetWeapons().IsValidIndex(WeaponIndex))
			{
				Record.WeaponName = Weapon->GetWeapons()[WeaponIndex].Name;
			}
		}
		Record.Stats = Stats;
		Record.SensitivityScale = SensitivityScale;
		Record.bHasFlick = bHasFlickSummary;
		if (bHasFlickSummary) { Record.Flick = LastFlickSummary; }
		SaveGame->AddSession(Record);
		bRecordSaved = true;
	}
	SaveProfile();
}

/** Phase 5: build the coach report from the session that was just saved plus the history. */
void UGETrainingManager::HandleSessionStopped()
{
	CoachReport = FGECoachReport();
	if (!Coach || !SaveGame || !bRecordSaved || SaveGame->GetSessions().Num() == 0)
	{
		return;
	}
	FGECoachInput Input;
	Input.History = SaveGame->GetSessions(); // oldest first, latest is the last entry
	Input.Latest = Input.History.Last();
	if (Mode == EGETrainingMode::Flick && FlickTracker)
	{
		Input.FlickRecords = FlickTracker->GetRecords();
	}
	Coach->BuildReport(Input, CoachReport);
}

void UGETrainingManager::SaveProfile()
{
	if (SaveGame)
	{
		SaveGame->SensitivityScale = SensitivityScale;
		SaveGame->MouseDpi = MouseDpi;
		SaveGame->Save();
	}
}

float UGETrainingManager::Cm360ForScale(float Scale) const
{
	// counts for a full turn = 360 / (degrees per count); inches = counts / DPI; 1 inch = 2.54 cm.
	const float DegreesPerCount = GESensitivity::BaseDegreesPerCount * FMath::Max(Scale, KINDA_SMALL_NUMBER);
	return (360.0f / DegreesPerCount) / FMath::Max(MouseDpi, 1) * 2.54f;
}

void UGETrainingManager::SetSensitivityScale(float NewScale)
{
	SensitivityScale = FMath::Clamp(NewScale, GESensitivity::MinScale, GESensitivity::MaxScale);
	SaveProfile();
}

void UGETrainingManager::SetMouseDpi(int32 NewDpi)
{
	MouseDpi = FMath::Clamp(NewDpi, GESensitivity::MinDpi, GESensitivity::MaxDpi);
	SaveProfile();
}

void UGETrainingManager::ApplyFlickSuggestion()
{
	if (bHasFlickSummary && LastFlickSummary.bHasSuggestion)
	{
		SetSensitivityScale(LastFlickSummary.SuggestedScale);
	}
}
