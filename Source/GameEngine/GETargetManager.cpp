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
#include "DrawDebugHelpers.h"
#include "Engine/GameViewportClient.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"
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

	NeedsPlacement.Init(false, 3);

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

	// headless self test: UnrealEditor-Cmd <project> <map> -game -nullrhi -GEAimSelfTest
	if (FParse::Param(FCommandLine::Get(), TEXT("GEAimSelfTest")))
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([this]()
		{
			const bool bOk = RunSelfTest();
			UE_LOG(LogTemp, Display, TEXT("GEAimSelfTest: %s"), bOk ? TEXT("ALL PASS") : TEXT("FAILED"));
			FPlatformMisc::RequestExit(false);
		}), 1.0f, false);
	}
}

void AGETargetManager::SetArena(AGEArena* InArena)
{
	Arena = InArena;
	RefreshBounds();
	if (Bounds.bValid)
	{
		SetActorLocation(Bounds.Center);
	}
	ShowIdleTargets();
}

// =====================================================================
//                    ENTERING / LEAVING THE ARENA
// =====================================================================

bool AGETargetManager::EnterArena(ACharacter* InPlayer)
{
	if (bInArena)
	{
		return true; // already on the pad
	}

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

	// the target region comes from the arena frame, seen from the player's real eye height
	EyeZ = PlayerCamera ? PlayerCamera->GetComponentLocation().Z : Pad.Z + 60.0;
	bHaveEyeZ = true;
	RefreshBounds();
	SetActorLocation(Bounds.Center);
	return true;
}

bool AGETargetManager::StartMode(EGEAimMode NewMode, ACharacter* InPlayer)
{
	if (Arena == nullptr || Targets.Num() < 3)
	{
		return false;
	}
	if (!EnterArena(InPlayer))
	{
		return false;
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

bool AGETargetManager::ShowShowcase(ACharacter* InPlayer)
{
	if (Arena == nullptr || Targets.Num() < 3)
	{
		return false;
	}
	if (!EnterArena(InPlayer))
	{
		return false;
	}
	bPaused = false;
	bRoundFinished = false;
	StopRound(); // hides nothing, shows the floating idle targets
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
	bPaused = false;
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
	bPaused = false;
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
	NeedsPlacement.Init(false, Targets.Num());
	RefreshBounds();
	for (AGETargetSphere* Target : Targets)
	{
		Target->Velocity = FVector::ZeroVector;
		Target->bBobbing = false;
	}

	const FLinearColor ModeColor = GEAimModes::GetColor(Mode);
	int32 Wanted = 1;

	// set up the targets for this mode (they are visible during the countdown)
	switch (Mode)
	{
	case EGEAimMode::Gridshot:
		RoundLength = 60.0f;
		Wanted = 3;
		for (AGETargetSphere* Target : Targets)
		{
			Target->SetRadius(30.0);
			Target->SetColor(ModeColor);
		}
		break;

	case EGEAimMode::Microshot:
		RoundLength = 60.0f;
		Targets[0]->SetRadius(12.0); // tiny!
		Targets[0]->SetColor(ModeColor);
		break;

	case EGEAimMode::Tracking:
		RoundLength = 45.0f;
		Targets[0]->SetRadius(35.0);
		Targets[0]->SetColor(GEStyle::Magenta); // turns green while you're on it
		break;

	case EGEAimMode::Reflex:
		RoundLength = 0.0f; // no timer, the round ends after ReflexTargets targets
		Targets[0]->SetRadius(30.0);
		Targets[0]->SetColor(ModeColor);
		Wanted = 0;         // the target only appears after the random wait (UpdateReflex)
		break;
	}

	// place the targets; any that fail are retried every tick (RetryPlacements)
	for (int32 i = 0; i < Wanted; i++)
	{
		NeedsPlacement[i] = true;
		TryPlace(i, false);
	}

	if (TourRoundLength > 0.0f && RoundLength > 0.0f)
	{
		RoundLength = TourRoundLength; // -ShotTour only: short rounds
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
	constexpr double IdleRadius = 40.0;
	constexpr double BobAmplitude = 12.0; // AGETargetSphere bobs its mesh by this much

	NeedsPlacement.Init(false, Targets.Num());
	for (int32 i = 0; i < Targets.Num() && i < 3; i++)
	{
		AGETargetSphere* Target = Targets[i];
		Target->Velocity = FVector::ZeroVector;
		Target->SetRadius(IdleRadius);
		Target->SetColor(Colors[i]);

		FVector Location = GetActorLocation();
		if (Bounds.bValid)
		{
			const double UW = FMath::Max(Bounds.UsableHalfWidth(IdleRadius) - BobAmplitude, 0.0);
			const double UH = FMath::Max(Bounds.UsableHalfHeight(IdleRadius) - BobAmplitude, 0.0);
			Location = FVector(Bounds.DepthFor(IdleRadius),
				Bounds.Center.Y + FMath::Clamp((i - 1) * 260.0, -UW, UW),
				Bounds.Center.Z + FMath::Clamp(40.0, -UH, UH));
		}
		Target->SetActorLocation(Location);
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

	if (bDebugDraw)
	{
		DrawDebug();
	}

	if (!bInArena)
	{
		return;
	}

	// menu camera drift: while only the showcase is visible, the view sways slowly
	if (!bRunning && !bRoundFinished && Player)
	{
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			PC->SetControlRotation(FRotator(FMath::Sin(CurrentTime * 0.35f) * 1.5f, FMath::Sin(CurrentTime * 0.22f) * 3.0f, 0.0f));
		}
	}

	// pause menu: nothing moves, no timer runs
	if (bPaused)
	{
		return;
	}

	if (!bRunning)
	{
		return;
	}

	RetryPlacements(); // a target that found no free spot last tick tries again

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
	if (!Target->bActive || !Bounds.bValid)
	{
		return; // still waiting for a spot (RetryPlacements)
	}

	// the same shrunken bounds as spawning: the sphere stays fully inside the frame
	const double UW = FMath::Max(Bounds.UsableHalfWidth(Target->Radius), 0.0);
	const double UH = FMath::Max(Bounds.UsableHalfHeight(Target->Radius), 0.0);

	// move the target (Y = sideways, Z = up/down)
	const FVector NewLocation = Target->GetActorLocation() + Target->Velocity * DeltaTime;
	double X = NewLocation.Y - Bounds.Center.Y;
	double Y = NewLocation.Z - Bounds.Center.Z;
	double VelX = Target->Velocity.Y;
	double VelY = Target->Velocity.Z;

	// bounce off the edges, then clamp so even a long frame can't push it out
	if ((X > UW && VelX > 0.0) || (X < -UW && VelX < 0.0))
	{
		VelX = -VelX;
	}
	if ((Y > UH && VelY > 0.0) || (Y < -UH && VelY < 0.0))
	{
		VelY = -VelY;
	}
	X = FMath::Clamp(X, -UW, UW);
	Y = FMath::Clamp(Y, -UH, UH);

	Target->Velocity = FVector(0.0, VelX, VelY);
	Target->SetActorLocation(FVector(Bounds.DepthFor(Target->Radius), Bounds.Center.Y + X, Bounds.Center.Z + Y));

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

	Target->Velocity = FVector(0.0, FMath::Cos(Angle), FMath::Sin(Angle)) * Speed;

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
		// if no free spot is found the timer stays at <= 0 and this retries next tick
		if (TryPlace(0, false)) // also plays the pop animation
		{
			bReflexVisible = true;
			ReflexAppearTime = Now(); // start the reaction stopwatch
		}
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
	if (!bRunning || bPaused || bCountingDown || Mode == EGEAimMode::Tracking)
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
		RespawnTarget(Target); // jump to a different empty cell
		break;

	case EGEAimMode::Microshot:
		AddPopup(HitLocation, TEXT("+1"), Target->GetColor());
		RespawnTarget(Target);
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

// The lowest FOV the Settings screen allows (GEHUDScreens.cpp MinFov). The target region must fit the view at this FOV.
static constexpr double GELowestFov = 60.0;
// A gap (cm) kept between two spheres on top of their radii.
static constexpr double GESphereGap = 4.0;

void AGETargetManager::RefreshBounds()
{
	if (Arena == nullptr)
	{
		return;
	}

	// 1) the frame on the back wall is the truth
	const FGEPlayRegion Frame = Arena->GetPlayRegion();

	FGEAimBounds B;
	B.Margin = 8.0;
	B.WallX = Frame.WallX;
	B.Eye = FVector(Frame.ViewerXY.X, Frame.ViewerXY.Y, bHaveEyeZ ? EyeZ : Frame.Center.Z);
	B.FrameCenter = Frame.Center;
	B.FrameHalfWidth = Frame.HalfWidth;
	B.FrameHalfHeight = Frame.HalfHeight;
	B.PlaneX = Frame.WallX - WallInset;

	const double FrameDist = Frame.Center.X - B.Eye.X;
	const double PlaneDist = B.PlaneX - B.Eye.X;
	if (FrameDist < 1.0 || PlaneDist < 1.0)
	{
		return;
	}

	// 2) project the frame from the eye onto the target plane: a sphere on the plane that stays inside the
	//    projected rectangle also LOOKS inside the glowing frame (targets sit a bit in front of the wall)
	const double K = PlaneDist / FrameDist;

	// 3) make sure the frame is inside the camera view at the lowest FOV (conservative: horizontal FOV kept,
	//    so a wider screen can only show more). If it is not, the region shrinks about its center.
	double Aspect = AspectOverride;
	if (Aspect <= 0.0)
	{
		Aspect = 16.0 / 9.0;
		FVector2D Size = FVector2D::ZeroVector;
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->GetViewportSize(Size);
			if (Size.X > 1.0 && Size.Y > 1.0)
			{
				Aspect = Size.X / Size.Y;
			}
		}
	}
	Aspect = FMath::Clamp(Aspect, 0.5, 4.0);
	const double TanHalf = FMath::Tan(FMath::DegreesToRadians(GELowestFov * 0.5));
	const double ViewHalfW = TanHalf * FrameDist * 0.95;
	const double ViewHalfH = TanHalf / Aspect * FrameDist * 0.95;
	const double OffY = FMath::Abs(Frame.Center.Y - B.Eye.Y);
	const double OffZ = FMath::Abs(Frame.Center.Z - B.Eye.Z);
	const double FitW = (ViewHalfW - OffY) / Frame.HalfWidth;
	const double FitH = (ViewHalfH - OffZ) / Frame.HalfHeight;
	const double Fit = FMath::Clamp(FMath::Min3(1.0, FitW, FitH), 0.25, 1.0);

	B.Center = FVector(B.PlaneX,
		B.Eye.Y + (Frame.Center.Y - B.Eye.Y) * K,
		B.Eye.Z + (Frame.Center.Z - B.Eye.Z) * K);
	B.HalfWidth = Frame.HalfWidth * K * Fit;
	B.HalfHeight = Frame.HalfHeight * K * Fit;
	B.bValid = true;
	Bounds = B;
}

// =====================================================================
//                              POSITIONS
// =====================================================================

bool AGETargetManager::FindSpawnPoint(int32 SelfIndex, double Radius, bool bGrid, const FVector* Avoid, FVector& Out) const
{
	if (!Bounds.bValid)
	{
		return false;
	}
	const double UW = Bounds.UsableHalfWidth(Radius);
	const double UH = Bounds.UsableHalfHeight(Radius);
	if (UW <= 0.0 || UH <= 0.0)
	{
		return false; // the sphere does not fit at all
	}
	const double X = Bounds.DepthFor(Radius);

	// no overlap with the other visible targets (and not on the spot it just left)
	auto IsFree = [&](const FVector& P)
	{
		if (Avoid && FVector::Dist(P, *Avoid) < Radius * 2.0)
		{
			return false;
		}
		for (int32 j = 0; j < Targets.Num(); j++)
		{
			const AGETargetSphere* Other = Targets[j];
			if (j == SelfIndex || Other == nullptr || !Other->bActive)
			{
				continue;
			}
			if (FVector::Dist(P, Other->GetActorLocation()) < Radius + Other->Radius + GESphereGap)
			{
				return false;
			}
		}
		return true;
	};

	const int32 N = FMath::Max(GridSize, 2);
	auto GridPoint = [&](int32 Col, int32 Row)
	{
		return FVector(X,
			Bounds.Center.Y + ((double)Col / (N - 1) * 2.0 - 1.0) * UW,
			Bounds.Center.Z + ((double)Row / (N - 1) * 2.0 - 1.0) * UH);
	};

	// random tries
	for (int32 Attempt = 0; Attempt < 50; Attempt++)
	{
		const FVector P = bGrid
			? GridPoint(FMath::RandRange(0, N - 1), FMath::RandRange(0, N - 1))
			: FVector(X, Bounds.Center.Y + FMath::RandRange(-UW, UW), Bounds.Center.Z + FMath::RandRange(-UH, UH));
		if (IsFree(P))
		{
			Out = P;
			return true;
		}
	}

	// safe fallback: walk every grid cell in order
	for (int32 Row = 0; Row < N; Row++)
	{
		for (int32 Col = 0; Col < N; Col++)
		{
			const FVector P = GridPoint(Col, Row);
			if (IsFree(P))
			{
				Out = P;
				return true;
			}
		}
	}
	return false;
}

bool AGETargetManager::TryPlace(int32 Index, bool bAvoidOld)
{
	if (!Targets.IsValidIndex(Index) || !Bounds.bValid)
	{
		return false;
	}
	AGETargetSphere* Target = Targets[Index];

	FVector Spot;
	if (Mode == EGEAimMode::Tracking)
	{
		Spot = FVector(Bounds.DepthFor(Target->Radius), Bounds.Center.Y, Bounds.Center.Z); // starts in the middle
	}
	else
	{
		const FVector Old = Target->GetActorLocation();
		if (!FindSpawnPoint(Index, Target->Radius, Mode == EGEAimMode::Gridshot, bAvoidOld ? &Old : nullptr, Spot))
		{
			if (NeedsPlacement.IsValidIndex(Index))
			{
				NeedsPlacement[Index] = true; // try again next tick
			}
			return false;
		}
	}

	Target->SetActorLocation(Spot);
	Target->SetActive(true); // pops in (scale 0 -> 1), so it never shows half-grown
	if (NeedsPlacement.IsValidIndex(Index))
	{
		NeedsPlacement[Index] = false;
	}
	return true;
}

void AGETargetManager::RespawnTarget(AGETargetSphere* Target)
{
	const int32 Index = Targets.IndexOfByKey(Target);
	if (Index == INDEX_NONE)
	{
		return;
	}
	Target->SetActive(false); // frees its old spot for the overlap check
	if (NeedsPlacement.IsValidIndex(Index))
	{
		NeedsPlacement[Index] = true;
	}
	TryPlace(Index, true);
}

void AGETargetManager::RetryPlacements()
{
	if (!bRunning)
	{
		return;
	}
	for (int32 i = 0; i < NeedsPlacement.Num(); i++)
	{
		if (NeedsPlacement[i])
		{
			TryPlace(i, false);
		}
	}
}

int32 AGETargetManager::GetExpectedActiveCount() const
{
	switch (Mode)
	{
	case EGEAimMode::Gridshot:  return 3;
	case EGEAimMode::Microshot: return 1;
	case EGEAimMode::Tracking:  return 1;
	case EGEAimMode::Reflex:    return bReflexVisible ? 1 : 0;
	}
	return 0;
}

FString AGETargetManager::ValidateTargets(int32 ExpectedActive) const
{
	constexpr double Tol = 0.01;
	int32 Active = 0;
	for (int32 i = 0; i < Targets.Num(); i++)
	{
		const AGETargetSphere* T = Targets[i];
		if (T == nullptr || !T->bActive)
		{
			continue;
		}
		Active++;
		const FVector L = T->GetActorLocation();
		const double R = T->Radius;

		// inside the bounds shrunk by the radius + margin
		if (FMath::Abs(L.Y - Bounds.Center.Y) > Bounds.UsableHalfWidth(R) + Tol
			|| FMath::Abs(L.Z - Bounds.Center.Z) > Bounds.UsableHalfHeight(R) + Tol)
		{
			return FString::Printf(TEXT("target %d outside bounds (Y %.1f Z %.1f)"), i, L.Y - Bounds.Center.Y, L.Z - Bounds.Center.Z);
		}
		// whole sphere in front of the wall
		if (L.X + R > Bounds.WallX - Bounds.Margin + Tol)
		{
			return FString::Printf(TEXT("target %d touches the wall (X+R %.1f, wall %.1f)"), i, L.X + R, Bounds.WallX);
		}
		// independent check: seen from the eye, the sphere lies inside the glowing frame
		const double Scale = (Bounds.FrameCenter.X - Bounds.Eye.X) / (L.X - Bounds.Eye.X);
		const double PY = Bounds.Eye.Y + (L.Y - Bounds.Eye.Y) * Scale;
		const double PZ = Bounds.Eye.Z + (L.Z - Bounds.Eye.Z) * Scale;
		if (FMath::Abs(PY - Bounds.FrameCenter.Y) + R * Scale > Bounds.FrameHalfWidth
			|| FMath::Abs(PZ - Bounds.FrameCenter.Z) + R * Scale > Bounds.FrameHalfHeight)
		{
			return FString::Printf(TEXT("target %d not inside the frame on screen"), i);
		}
		// no overlap
		for (int32 j = i + 1; j < Targets.Num(); j++)
		{
			const AGETargetSphere* O = Targets[j];
			if (O && O->bActive && FVector::Dist(L, O->GetActorLocation()) < R + O->Radius)
			{
				return FString::Printf(TEXT("targets %d and %d overlap"), i, j);
			}
		}
	}
	if (Active != ExpectedActive)
	{
		return FString::Printf(TEXT("%d targets alive, expected %d"), Active, ExpectedActive);
	}
	return FString();
}

void AGETargetManager::DrawDebug() const
{
	UWorld* World = GetWorld();
	if (World == nullptr || !Bounds.bValid)
	{
		return;
	}
	auto Rect = [&](double X, const FVector& C, double HW, double HH, const FColor& Color)
	{
		const FVector A(X, C.Y - HW, C.Z - HH), B(X, C.Y + HW, C.Z - HH), D(X, C.Y + HW, C.Z + HH), E(X, C.Y - HW, C.Z + HH);
		DrawDebugLine(World, A, B, Color, false, 0.0f, 0, 2.0f);
		DrawDebugLine(World, B, D, Color, false, 0.0f, 0, 2.0f);
		DrawDebugLine(World, D, E, Color, false, 0.0f, 0, 2.0f);
		DrawDebugLine(World, E, A, Color, false, 0.0f, 0, 2.0f);
	};
	// white: the inside of the glowing frame (playable region as seen on the wall)
	Rect(Bounds.FrameCenter.X, Bounds.FrameCenter, Bounds.FrameHalfWidth, Bounds.FrameHalfHeight, FColor::White);
	// green: the target plane region (before shrinking by a radius)
	Rect(Bounds.PlaneX, Bounds.Center, Bounds.HalfWidth, Bounds.HalfHeight, FColor::Green);
	// yellow: where the center of each visible target may go (bounds shrunk by ITS radius + margin)
	for (const AGETargetSphere* T : Targets)
	{
		if (T && T->bActive)
		{
			Rect(Bounds.DepthFor(T->Radius), Bounds.Center, Bounds.UsableHalfWidth(T->Radius), Bounds.UsableHalfHeight(T->Radius), FColor::Yellow);
		}
	}
}

bool AGETargetManager::RunSelfTest()
{
	if (Arena == nullptr || Targets.Num() < 3)
	{
		UE_LOG(LogTemp, Error, TEXT("ge.aim.selftest: FAIL - no arena / targets (run it in the RayShot level)"));
		return false;
	}
	if (bRunning)
	{
		UE_LOG(LogTemp, Warning, TEXT("ge.aim.selftest: a round is running, stop it first (Tab / ge.aim.stop)."));
		return false;
	}

	constexpr int32 Iterations = 2000;
	const double Aspects[3] = { 16.0 / 9.0, 21.0 / 9.0, 32.0 / 9.0 }; // 16:9, ultrawide, super ultrawide
	bool bAllPass = true;

	for (int32 m = 0; m < GEAimModes::NumModes; m++)
	{
		const EGEAimMode TestMode = (EGEAimMode)m;
		FString Failure;

		for (int32 a = 0; a < 3 && Failure.IsEmpty(); a++)
		{
			AspectOverride = Aspects[a];
			StartRound(TestMode);
			bCountingDown = false; // skip the 3-2-1
			if (TestMode == EGEAimMode::Tracking)
			{
				PickNewTrackingDirection(Targets[0]);
			}

			for (int32 It = 0; It < Iterations && Failure.IsEmpty(); It++)
			{
				switch (TestMode)
				{
				case EGEAimMode::Gridshot:
					RespawnTarget(Targets[FMath::RandRange(0, 2)]);
					break;
				case EGEAimMode::Microshot:
					RespawnTarget(Targets[0]);
					break;
				case EGEAimMode::Tracking:
					if (It % 37 == 0)
					{
						PickNewTrackingDirection(Targets[0]);
					}
					UpdateTracking(FMath::FRandRange(0.001f, 0.12f)); // includes long hitches
					break;
				case EGEAimMode::Reflex:
					Targets[0]->SetActive(false);
					bReflexVisible = false;
					ReflexWaitTimer = 0.0f;
					UpdateReflex(0.016f);
					break;
				}
				const FString Err = ValidateTargets(GetExpectedActiveCount());
				if (!Err.IsEmpty())
				{
					Failure = FString::Printf(TEXT("iteration %d, aspect %.2f: %s"), It, Aspects[a], *Err);
				}
			}
			StopRound();
		}

		if (Failure.IsEmpty())
		{
			UE_LOG(LogTemp, Display, TEXT("ge.aim.selftest: %s PASS (%d iterations x 3 aspect ratios)"), *GEAimModes::GetName(TestMode), Iterations);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ge.aim.selftest: %s FAIL - %s"), *GEAimModes::GetName(TestMode), *Failure);
			bAllPass = false;
		}
	}

	AspectOverride = 0.0;
	RefreshBounds();
	ShowIdleTargets();
	return bAllPass;
}

static FAutoConsoleCommandWithWorld GEAimDebugCommand(
	TEXT("ge.aim.debug"),
	TEXT("Toggle drawing of the playable region (white = frame, green = target plane, yellow = per-target center bounds)."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (AGETargetManager* Manager = AGETargetManager::Get(World))
		{
			Manager->ToggleDebugDraw();
		}
	}));

static FAutoConsoleCommandWithWorld GEAimSelfTestCommand(
	TEXT("ge.aim.selftest"),
	TEXT("Check all 4 aim modes 2000 times: inside the frame, no overlap, off the wall, right target count."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (AGETargetManager* Manager = AGETargetManager::Get(World))
		{
			Manager->RunSelfTest();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ge.aim.selftest: FAIL - no aim manager in this level"));
		}
	}));

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
