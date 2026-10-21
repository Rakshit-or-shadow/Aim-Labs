// GETargetManager.cpp

#include "GETargetManager.h"
#include "GETargetSphere.h"
#include "GEHitBurst.h"
#include "GEArena.h"
#include "GEGameMode.h"
#include "GESaveGame.h"
#include "GETrainingManager.h"
#include "GEWeaponComponent.h"
#include "GEAimModes.h"
#include "GEStyle.h"
#include "GESounds.h"
#include "RayTraceMath.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

AGETargetManager::AGETargetManager()
{
	// we need Tick for timers and moving targets
	PrimaryActorTick.bCanEverTick = true;

	// an empty root - the targets are placed around this point
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

AGETargetManager* AGETargetManager::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	AGEGameMode* GameMode = World ? World->GetAuthGameMode<AGEGameMode>() : nullptr;
	return GameMode ? GameMode->GetAimManager() : nullptr;
}

void AGETargetManager::BeginPlay()
{
	Super::BeginPlay();

	// spawn 3 targets once and reuse them in every mode
	for (int32 i = 0; i < 3; i++)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AGETargetSphere* Target = GetWorld()->SpawnActor<AGETargetSphere>(
			AGETargetSphere::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params);
		if (Target)
		{
			Targets.Add(Target);
		}
	}

	// floating targets behind the menus
	ShowIdleTargets();
}

// =====================================================================
//                    ENTERING / LEAVING THE ARENA
// =====================================================================

bool AGETargetManager::StartMode(EGEAimMode NewMode, ACharacter* InPlayer)
{
	if (Arena == nullptr || Targets.Num() < 3)
	{
		return false;
	}

	if (!bInArena)
	{
		// who is playing?
		ACharacter* Who = InPlayer ? InPlayer : Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		if (Who == nullptr)
		{
			return false;
		}
		// do not mix with the Training Center (it has its own teleport)
		const UGETrainingManager* Training = UGETrainingManager::Get(this);
		if (Training && Training->IsInTrainingCenter())
		{
			return false;
		}

		Player = Who;
		PlayerCamera = Who->FindComponentByClass<UCameraComponent>();
		PlayerWeapons = Who->FindComponentByClass<UGEWeaponComponent>();

		// remember where to come back to
		APlayerController* PC = Cast<APlayerController>(Who->GetController());
		SavedLocation = Who->GetActorLocation();
		SavedControlRotation = PC ? PC->GetControlRotation() : Who->GetActorRotation();

		// stand on the pink pad (floor top is 2 cm above the arena origin)
		Who->UnCrouch();
		const float HalfHeight = Who->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const FVector Pad = Arena->GetActorLocation() + FVector(0.0, 0.0, 2.0 + HalfHeight + 2.0);
		Who->TeleportTo(Pad, FRotator::ZeroRotator, false, true);
		FreezePlayer(true);

		// shooting is handled by us (first-shot ray, no ammo, no spread, no recoil)
		if (PlayerWeapons)
		{
			PlayerWeapons->SetFiringEnabled(false);
		}

		Arena->SetAimLookEnabled(true);
		bInArena = true;

		// center the target area at the player's eye height, TargetDistance in front of the pad
		const double EyeZ = PlayerCamera ? PlayerCamera->GetComponentLocation().Z : Pad.Z + 60.0;
		const FVector ArenaOrigin = Arena->GetActorLocation();
		SetActorLocation(FVector(ArenaOrigin.X + TargetDistance, ArenaOrigin.Y, EyeZ));
	}

	// aim straight ahead (+X) at the start of every round
	if (Player)
	{
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			PC->SetControlRotation(FRotator::ZeroRotator);
		}
	}

	StartRound(NewMode);
	GESounds::PlayClick(this, GetVolume());
	return true;
}

void AGETargetManager::StopMode()
{
	if (!bInArena)
	{
		return;
	}

	StopRound();
	bInArena = false;
	bRoundFinished = false;

	if (Arena)
	{
		Arena->SetAimLookEnabled(false);
	}

	if (Player)
	{
		// back to where we were, with movement and shooting restored
		Player->TeleportTo(SavedLocation, SavedControlRotation, false, true);
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			PC->SetControlRotation(SavedControlRotation);
		}
		FreezePlayer(false);
		if (PlayerWeapons)
		{
			PlayerWeapons->SetFiringEnabled(true);
		}
	}
	Player = nullptr;
	PlayerCamera = nullptr;
	PlayerWeapons = nullptr;
}

void AGETargetManager::FreezePlayer(bool bFreeze)
{
	if (Player == nullptr)
	{
		return;
	}
	UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	if (Movement == nullptr)
	{
		return;
	}
	if (bFreeze)
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement(); // aim only: no walking, no jumping, no gravity
	}
	else
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Falling); // lands on the floor by itself
	}
}

void AGETargetManager::HandleFire()
{
	if (!bInArena || PlayerWeapons == nullptr)
	{
		return;
	}
	// the weapon gives us its first-shot ray (exact crosshair line) and applies its fire rate
	FVector Origin, Direction;
	if (PlayerWeapons->TryAimTrainerShot(Origin, Direction))
	{
		HandleClick(Origin, Direction);
	}
}

bool AGETargetManager::GetAimRay(FVector& OutOrigin, FVector& OutDirection) const
{
	if (PlayerCamera == nullptr)
	{
		return false;
	}
	OutOrigin = PlayerCamera->GetComponentLocation();
	OutDirection = PlayerCamera->GetForwardVector();
	return true;
}

// =====================================================================
//                         STARTING / STOPPING ROUNDS
// =====================================================================

void AGETargetManager::StartRound(EGEAimMode NewMode)
{
	if (Targets.Num() < 3)
	{
		return; // targets not spawned yet
	}

	// reset everything
	Mode = NewMode;
	bRunning = true;
	bRoundFinished = false;
	bNewBest = false;
	LastScore = 0.0f;
	Hits = 0;
	Shots = 0;
	ElapsedTime = 0.0f;
	OnTargetTime = 0.0f;
	bTrackingOnTarget = false;
	EarlyClicks = 0;
	ReactionTimesMs.Empty();
	TooEarlyTime = -100.0f;
	LastHitTime = -100.0f;
	bReflexVisible = false;
	Popups.Empty();

	// start the 3-2-1 countdown
	bCountingDown = true;
	CountdownLeft = CountdownLength;
	LastCountdownNumber = 0;
	GoTime = -100.0f;

	HideAllTargets();
	for (AGETargetSphere* Target : Targets)
	{
		Target->Velocity = FVector::ZeroVector;
		Target->bBobbing = false;
	}

	const FLinearColor ModeColor = GEAimModes::GetColor(Mode);

	// set up the targets for this mode (they are visible during the countdown)
	switch (Mode)
	{
	case EGEAimMode::Gridshot:
		RoundLength = 60.0f;
		for (AGETargetSphere* Target : Targets)
		{
			Target->SetRadius(30.0);
			Target->SetColor(ModeColor);
			Target->SetActorLocation(GetFreeGridPoint());
			Target->SetActive(true);
		}
		break;

	case EGEAimMode::Microshot:
		RoundLength = 60.0f;
		Targets[0]->SetRadius(12.0); // tiny!
		Targets[0]->SetColor(ModeColor);
		Targets[0]->SetActorLocation(GetRandomAreaPoint());
		Targets[0]->SetActive(true);
		break;

	case EGEAimMode::Tracking:
		RoundLength = 45.0f;
		Targets[0]->SetRadius(35.0);
		Targets[0]->SetColor(GEStyle::Magenta); // turns green while you're on it
		Targets[0]->SetActorLocation(GetActorLocation());
		Targets[0]->SetActive(true);
		break;

	case EGEAimMode::Reflex:
		RoundLength = 0.0f; // no timer, the round ends after ReflexTargets targets
		Targets[0]->SetRadius(30.0);
		Targets[0]->SetColor(ModeColor);
		break;
	}

	TimeLeft = RoundLength;
}

void AGETargetManager::StopRound()
{
	bRunning = false;
	bCountingDown = false;
	ShowIdleTargets();
}

void AGETargetManager::FinishRound()
{
	bRunning = false;
	bRoundFinished = true;
	LastScore = GetScore();

	ShowIdleTargets();

	// save the best score (the first play of a mode counts as a new best)
	bNewBest = false;
	if (UGESaveGame* Save = GetSave())
	{
		bNewBest = Save->SubmitScore(Mode, LastScore);
		Save->Save();
	}

	if (bNewBest)
	{
		GESounds::PlayNewBest(this, GetVolume());
	}
	else
	{
		GESounds::PlayRoundEnd(this, GetVolume());
	}

	OnRoundFinished.Broadcast(Mode, LastScore, bNewBest);
}

void AGETargetManager::HideAllTargets()
{
	for (AGETargetSphere* Target : Targets)
	{
		Target->SetActive(false);
	}
}

void AGETargetManager::ShowIdleTargets()
{
	// three big floating targets in a row (they can't be shot - no round is running)
	const FLinearColor Colors[3] = { GEStyle::Cyan, GEStyle::Purple, GEStyle::Orange };

	for (int32 i = 0; i < Targets.Num() && i < 3; i++)
	{
		AGETargetSphere* Target = Targets[i];
		Target->Velocity = FVector::ZeroVector;
		Target->SetRadius(40.0);
		Target->SetColor(Colors[i]);
		Target->SetActorLocation(GetActorLocation()
		                         + GetActorRightVector() * (i - 1) * 260.0
		                         + GetActorUpVector() * 40.0);
		Target->bBobbing = true;
		Target->SetActive(true);
	}
}

// =====================================================================
//                              EVERY FRAME
// =====================================================================

void AGETargetManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// remove old popups (older than 0.9 s)
	const float CurrentTime = Now();
	Popups.RemoveAll([CurrentTime](const FGEScorePopup& Popup)
	{
		return CurrentTime - Popup.SpawnTime > 0.9f;
	});

	if (!bInArena)
	{
		return;
	}

	// keys while in the arena: Tab = leave, R = restart this mode
	if (Player)
	{
		if (const APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			if (PC->WasInputKeyJustPressed(EKeys::Tab))
			{
				StopMode();
				return;
			}
			if (PC->WasInputKeyJustPressed(EKeys::R))
			{
				StartMode(Mode);
				return;
			}
		}
	}

	if (!bRunning)
	{
		return;
	}

	if (bCountingDown)
	{
		UpdateCountdown(DeltaTime);
		return; // nothing else happens until "GO!"
	}

	ElapsedTime += DeltaTime;

	if (Mode == EGEAimMode::Tracking)
	{
		UpdateTracking(DeltaTime);

		// Tracking checks the crosshair ray every frame
		FVector Origin, Direction;
		if (GetAimRay(Origin, Direction))
		{
			UpdateAim(Origin, Direction, DeltaTime);
		}
	}
	else if (Mode == EGEAimMode::Reflex)
	{
		UpdateReflex(DeltaTime);
	}

	// timed modes: count down
	if (RoundLength > 0.0f)
	{
		TimeLeft -= DeltaTime;
		if (TimeLeft <= 0.0f)
		{
			TimeLeft = 0.0f;
			FinishRound();
		}
	}
}

void AGETargetManager::UpdateCountdown(float DeltaTime)
{
	CountdownLeft -= DeltaTime;

	// beep once for each number: 3, 2, 1
	const int32 Number = FMath::CeilToInt(CountdownLeft);
	if (CountdownLeft > 0.0f && Number != LastCountdownNumber)
	{
		LastCountdownNumber = Number;
		GESounds::PlayCountdown(this, GetVolume());
	}

	// GO!
	if (CountdownLeft <= 0.0f)
	{
		bCountingDown = false;
		GoTime = Now();
		GESounds::PlayRoundStart(this, GetVolume());

		if (Mode == EGEAimMode::Tracking)
		{
			PickNewTrackingDirection(Targets[0]);
		}
		else if (Mode == EGEAimMode::Reflex)
		{
			ReflexWaitTimer = FMath::RandRange(1.0f, 2.5f);
		}
	}
}

void AGETargetManager::UpdateTracking(float DeltaTime)
{
	AGETargetSphere* Target = Targets[0];

	// directions of the target area
	const FVector Center = GetActorLocation();
	const FVector Right = GetActorRightVector();
	const FVector Up = GetActorUpVector();

	// move the target
	const FVector NewLocation = Target->GetActorLocation() + Target->Velocity * DeltaTime;

	// where is it inside the area? (X = sideways, Y = up/down)
	const FVector Offset = NewLocation - Center;
	double X = FVector::DotProduct(Offset, Right);
	double Y = FVector::DotProduct(Offset, Up);

	// current speed split into sideways and up/down parts
	double VelX = FVector::DotProduct(Target->Velocity, Right);
	double VelY = FVector::DotProduct(Target->Velocity, Up);

	// bounce off the edges of the area
	if ((X > AreaHalfWidth && VelX > 0.0) || (X < -AreaHalfWidth && VelX < 0.0))
	{
		VelX = -VelX;
	}
	if ((Y > AreaHalfHeight && VelY > 0.0) || (Y < -AreaHalfHeight && VelY < 0.0))
	{
		VelY = -VelY;
	}
	X = FMath::Clamp(X, -AreaHalfWidth, AreaHalfWidth);
	Y = FMath::Clamp(Y, -AreaHalfHeight, AreaHalfHeight);

	Target->Velocity = Right * VelX + Up * VelY;
	Target->SetActorLocation(Center + Right * X + Up * Y);

	// change direction every so often so it's hard to predict
	DirectionTimer -= DeltaTime;
	if (DirectionTimer <= 0.0f)
	{
		PickNewTrackingDirection(Target);
	}
}

void AGETargetManager::PickNewTrackingDirection(AGETargetSphere* Target)
{
	const float Angle = FMath::RandRange(0.0f, 2.0f * PI);
	const float Speed = FMath::RandRange(150.0f, 350.0f); // cm per second

	const FVector Right = GetActorRightVector();
	const FVector Up = GetActorUpVector();
	Target->Velocity = (Right * FMath::Cos(Angle) + Up * FMath::Sin(Angle)) * Speed;

	DirectionTimer = FMath::RandRange(0.4f, 1.2f);
}

void AGETargetManager::UpdateReflex(float DeltaTime)
{
	if (bReflexVisible)
	{
		return; // waiting for the player to click it
	}

	// count down until the target appears
	ReflexWaitTimer -= DeltaTime;
	if (ReflexWaitTimer <= 0.0f)
	{
		Targets[0]->SetActorLocation(GetRandomAreaPoint());
		Targets[0]->SetActive(true); // also plays the pop animation
		bReflexVisible = true;
		ReflexAppearTime = Now(); // start the reaction stopwatch
	}
}

void AGETargetManager::UpdateAim(const FVector& Origin, const FVector& Direction, float DeltaTime)
{
	if (!bRunning || bCountingDown || Mode != EGEAimMode::Tracking)
	{
		return;
	}

	// is the crosshair on the target right now? (my ray tracer math again)
	const bool bOnTarget = FindHitTarget(Origin, Direction) != nullptr;
	if (bOnTarget)
	{
		OnTargetTime += DeltaTime;
	}

	// green while you're on it, pink when you're off (only recolor when it changes)
	if (bOnTarget != bTrackingOnTarget)
	{
		bTrackingOnTarget = bOnTarget;
		Targets[0]->SetColor(bOnTarget ? GEStyle::Green : GEStyle::Magenta);
	}
}

// =====================================================================
//                               SHOOTING
// =====================================================================

void AGETargetManager::HandleClick(const FVector& Origin, const FVector& Direction)
{
	if (!bRunning || bCountingDown || Mode == EGEAimMode::Tracking)
	{
		return; // no shooting during the countdown, and Tracking doesn't use clicks
	}

	// Reflex: clicking before the target shows up is "too early"
	if (Mode == EGEAimMode::Reflex && !bReflexVisible)
	{
		EarlyClicks++;
		TooEarlyTime = Now();
		ReflexWaitTimer = FMath::RandRange(1.0f, 2.5f); // restart the wait
		GESounds::PlayMiss(this, GetVolume());
		return;
	}

	Shots++;

	AGETargetSphere* HitTarget = FindHitTarget(Origin, Direction);
	if (HitTarget == nullptr)
	{
		GESounds::PlayMiss(this, GetVolume());
		return;
	}

	Hits++;
	OnTargetHit(HitTarget);
}

AGETargetSphere* AGETargetManager::FindHitTarget(const FVector& Origin, const FVector& Direction) const
{
	const FVector RayDirection = Direction.GetSafeNormal();

	// find the CLOSEST sphere the ray hits
	double ClosestT = TNumericLimits<double>::Max();
	AGETargetSphere* ClosestTarget = nullptr;

	for (AGETargetSphere* Target : Targets)
	{
		if (Target == nullptr || !Target->bActive)
		{
			continue; // hidden targets can't be hit
		}

		double T = 0.0;
		if (RayTraceMath::DoesIntersect(Target->GetActorLocation(), Target->Radius,
		                                Origin, RayDirection, T) && T < ClosestT)
		{
			ClosestT = T;
			ClosestTarget = Target;
		}
	}

	return ClosestTarget;
}

void AGETargetManager::OnTargetHit(AGETargetSphere* Target)
{
	LastHitTime = Now();
	const FVector HitLocation = Target->GetActorLocation();

	// hit effect + sound
	AGEHitBurst* Burst = GetWorld()->SpawnActor<AGEHitBurst>(
		AGEHitBurst::StaticClass(), HitLocation, FRotator::ZeroRotator);
	if (Burst)
	{
		Burst->Setup(Target->GetColor(), (float)Target->Radius);
	}
	GESounds::PlayHit(this, GetVolume());

	// what happens next depends on the mode
	switch (Mode)
	{
	case EGEAimMode::Gridshot:
		AddPopup(HitLocation, TEXT("+1"), Target->GetColor());
		Target->MoveTo(GetFreeGridPoint()); // jump to a different empty cell
		break;

	case EGEAimMode::Microshot:
		AddPopup(HitLocation, TEXT("+1"), Target->GetColor());
		Target->MoveTo(GetRandomAreaPoint());
		break;

	case EGEAimMode::Reflex:
	{
		// stop the stopwatch
		const float ReactionMs = (Now() - ReflexAppearTime) * 1000.0f;
		ReactionTimesMs.Add(ReactionMs);
		AddPopup(HitLocation, FString::Printf(TEXT("%.0f ms"), ReactionMs), Target->GetColor());

		Target->SetActive(false);
		bReflexVisible = false;

		if (ReactionTimesMs.Num() >= ReflexTargets)
		{
			FinishRound(); // done all targets
		}
		else
		{
			ReflexWaitTimer = FMath::RandRange(1.0f, 2.5f);
		}
		break;
	}

	case EGEAimMode::Tracking:
		break; // (not used - Tracking has no clicking)
	}
}

void AGETargetManager::AddPopup(const FVector& Location, const FString& Text, const FLinearColor& Color)
{
	FGEScorePopup Popup;
	Popup.Location = Location;
	Popup.SpawnTime = Now();
	Popup.Text = Text;
	Popup.Color = Color;
	Popups.Add(Popup);
}

// =====================================================================
//                              POSITIONS
// =====================================================================

FVector AGETargetManager::GetFreeGridPoint() const
{
	// the grid is centered on this actor and faces the way this actor faces
	const FVector Center = GetActorLocation();
	const FVector Right = GetActorRightVector();
	const FVector Up = GetActorUpVector();
	const double Half = (GridSize - 1) / 2.0;

	// try random cells until we find an empty one
	for (int32 Attempt = 0; Attempt < 50; Attempt++)
	{
		const int32 Col = FMath::RandRange(0, GridSize - 1);
		const int32 Row = FMath::RandRange(0, GridSize - 1);
		const FVector Point = Center + Right * (Col - Half) * GridSpacing
		                             + Up    * (Row - Half) * GridSpacing;

		// check that no active target is already in this cell
		bool bTaken = false;
		for (AGETargetSphere* Target : Targets)
		{
			if (Target && Target->bActive && FVector::Dist(Target->GetActorLocation(), Point) < 1.0)
			{
				bTaken = true;
				break;
			}
		}

		if (!bTaken)
		{
			return Point;
		}
	}

	return Center; // fallback (should almost never happen)
}

FVector AGETargetManager::GetRandomAreaPoint() const
{
	const double X = FMath::RandRange(-AreaHalfWidth, AreaHalfWidth);
	const double Y = FMath::RandRange(-AreaHalfHeight, AreaHalfHeight);
	return GetActorLocation() + GetActorRightVector() * X + GetActorUpVector() * Y;
}

// =====================================================================
//                             SCORE + HUD TEXT
// =====================================================================

float AGETargetManager::GetAccuracy() const
{
	if (Shots == 0)
	{
		return 0.0f; // avoid dividing by zero
	}
	return 100.0f * (float)Hits / (float)Shots;
}

float AGETargetManager::GetAverageReaction() const
{
	if (ReactionTimesMs.Num() == 0)
	{
		return 0.0f;
	}

	float Total = 0.0f;
	for (float Time : ReactionTimesMs)
	{
		Total += Time;
	}
	return Total / ReactionTimesMs.Num();
}

float AGETargetManager::GetScore() const
{
	switch (Mode)
	{
	case EGEAimMode::Gridshot:
	case EGEAimMode::Microshot:
		return (float)Hits;

	case EGEAimMode::Tracking:
		// percent of the time the crosshair was on the target
		return ElapsedTime > 0.0f ? 100.0f * OnTargetTime / ElapsedTime : 0.0f;

	case EGEAimMode::Reflex:
		return GetAverageReaction();
	}
	return 0.0f;
}

float AGETargetManager::GetTimeSinceGo() const
{
	return Now() - GoTime;
}

FString AGETargetManager::GetTimerText() const
{
	if (RoundLength > 0.0f)
	{
		return FString::Printf(TEXT("%.1f"), TimeLeft);
	}
	return FString::Printf(TEXT("%d / %d"), ReactionTimesMs.Num(), ReflexTargets);
}

float AGETargetManager::GetTimerFraction() const
{
	if (RoundLength > 0.0f)
	{
		return FMath::Clamp(TimeLeft / RoundLength, 0.0f, 1.0f);
	}
	return 1.0f - (float)ReactionTimesMs.Num() / (float)FMath::Max(ReflexTargets, 1);
}

bool AGETargetManager::IsLowOnTime() const
{
	return bRunning && !bCountingDown && RoundLength > 0.0f && TimeLeft < 5.0f;
}

TArray<FGEStatLine> AGETargetManager::GetLiveStats() const
{
	TArray<FGEStatLine> Lines;

	switch (Mode)
	{
	case EGEAimMode::Gridshot:
	case EGEAimMode::Microshot:
		Lines.Add({ TEXT("Hits"), FString::FromInt(Hits) });
		Lines.Add({ TEXT("Accuracy"), FString::Printf(TEXT("%.1f%%"), GetAccuracy()) });
		break;

	case EGEAimMode::Tracking:
		Lines.Add({ TEXT("On target"), FString::Printf(TEXT("%.1f%%"), GetScore()) });
		break;

	case EGEAimMode::Reflex:
		Lines.Add({ TEXT("Last"), ReactionTimesMs.Num() > 0 ? FString::Printf(TEXT("%.0f ms"), ReactionTimesMs.Last()) : FString(TEXT("-")) });
		Lines.Add({ TEXT("Average"), ReactionTimesMs.Num() > 0 ? FString::Printf(TEXT("%.0f ms"), GetAverageReaction()) : FString(TEXT("-")) });
		Lines.Add({ TEXT("Early"), FString::FromInt(EarlyClicks) });
		break;
	}

	return Lines;
}

FString AGETargetManager::GetCenterMessage() const
{
	if (!bRunning || bCountingDown || Mode != EGEAimMode::Reflex)
	{
		return TEXT("");
	}

	if (Now() - TooEarlyTime < 0.8f)
	{
		return TEXT("Too early!");
	}
	if (!bReflexVisible)
	{
		return TEXT("Wait for it...");
	}
	return TEXT("");
}

TArray<FGEStatLine> AGETargetManager::GetResultLines() const
{
	TArray<FGEStatLine> Lines;

	switch (Mode)
	{
	case EGEAimMode::Gridshot:
	case EGEAimMode::Microshot:
		Lines.Add({ TEXT("Hits"), FString::FromInt(Hits) });
		Lines.Add({ TEXT("Shots fired"), FString::FromInt(Shots) });
		Lines.Add({ TEXT("Accuracy"), FString::Printf(TEXT("%.1f%%"), GetAccuracy()) });
		Lines.Add({ TEXT("Hits per second"), FString::Printf(TEXT("%.2f"), ElapsedTime > 0.0f ? Hits / ElapsedTime : 0.0f) });
		break;

	case EGEAimMode::Tracking:
		Lines.Add({ TEXT("Time on target"), FString::Printf(TEXT("%.1f s"), OnTargetTime) });
		Lines.Add({ TEXT("Round length"), FString::Printf(TEXT("%.0f s"), ElapsedTime) });
		Lines.Add({ TEXT("On target"), FString::Printf(TEXT("%.1f%%"), GetScore()) });
		break;

	case EGEAimMode::Reflex:
	{
		float Fastest = 0.0f;
		if (ReactionTimesMs.Num() > 0)
		{
			Fastest = ReactionTimesMs[0];
			for (float Time : ReactionTimesMs)
			{
				Fastest = FMath::Min(Fastest, Time);
			}
		}
		Lines.Add({ TEXT("Average reaction"), FString::Printf(TEXT("%.0f ms"), GetAverageReaction()) });
		Lines.Add({ TEXT("Fastest"), FString::Printf(TEXT("%.0f ms"), Fastest) });
		Lines.Add({ TEXT("Early clicks"), FString::FromInt(EarlyClicks) });
		Lines.Add({ TEXT("Accuracy"), FString::Printf(TEXT("%.1f%%"), GetAccuracy()) });
		break;
	}
	}

	return Lines;
}

UGESaveGame* AGETargetManager::GetSave() const
{
	// share the training manager's live save so there is only ONE copy in memory
	const UGETrainingManager* Training = UGETrainingManager::Get(this);
	return Training ? Training->GetSaveGame() : nullptr;
}

bool AGETargetManager::GetBestScore(EGEAimMode ForMode, float& OutBest) const
{
	const UGESaveGame* Save = GetSave();
	return Save && Save->GetBestScore(ForMode, OutBest);
}

float AGETargetManager::GetVolume() const
{
	const UGESaveGame* Save = GetSave();
	return Save ? FMath::Clamp(Save->Volume, 0.0f, 1.0f) : 0.7f;
}

float AGETargetManager::Now() const
{
	return GetWorld()->GetTimeSeconds();
}

// =====================================================================
//                        CONSOLE COMMANDS (testing)
// =====================================================================

static FAutoConsoleCommandWithWorldAndArgs GEAimStartCommand(
	TEXT("ge.aim.start"),
	TEXT("Start an aim mode: ge.aim.start <0..3 or Gridshot|Microshot|Tracking|Reflex>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		AGETargetManager* Manager = AGETargetManager::Get(World);
		if (Manager == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("ge.aim.start: no aim arena in this level (use Lvl_IntroRoom)."));
			return;
		}
		EGEAimMode NewMode = EGEAimMode::Gridshot;
		if (Args.Num() > 0)
		{
			if (Args[0].IsNumeric())
			{
				NewMode = (EGEAimMode)FMath::Clamp(FCString::Atoi(*Args[0]), 0, GEAimModes::NumModes - 1);
			}
			else
			{
				for (int32 i = 0; i < GEAimModes::NumModes; i++)
				{
					if (GEAimModes::GetName((EGEAimMode)i).Equals(Args[0], ESearchCase::IgnoreCase))
					{
						NewMode = (EGEAimMode)i;
					}
				}
			}
		}
		if (!Manager->StartMode(NewMode))
		{
			UE_LOG(LogTemp, Warning, TEXT("ge.aim.start: could not start (leave the Training Center first)."));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GEAimStopCommand(
	TEXT("ge.aim.stop"),
	TEXT("Leave the aim arena and go back to where you were."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (AGETargetManager* Manager = AGETargetManager::Get(World))
		{
			Manager->StopMode();
		}
	}));
