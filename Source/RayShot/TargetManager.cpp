// TargetManager.cpp

#include "TargetManager.h"
#include "TargetSphere.h"
#include "HitBurst.h"
#include "RayTraceMath.h"
#include "RayShotGameMode.h"
#include "RayShotModes.h"
#include "RayShotStyle.h"
#include "RayShotSounds.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ATargetManager::ATargetManager()
{
    // we need Tick for timers and moving targets
    PrimaryActorTick.bCanEverTick = true;

    // an empty root - the targets are placed around this point
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ATargetManager::BeginPlay()
{
    Super::BeginPlay();

    // spawn 3 targets once and reuse them in every mode
    for (int32 i = 0; i < 3; i++)
    {
        ATargetSphere* Target = GetWorld()->SpawnActor<ATargetSphere>(
            ATargetSphere::StaticClass(), GetActorLocation(), FRotator::ZeroRotator);
        if (Target)
        {
            Targets.Add(Target);
        }
    }

    // floating targets behind the main menu
    ShowIdleTargets();
}

// =====================================================================
//                         STARTING / STOPPING
// =====================================================================

void ATargetManager::StartRound(ERayShotMode NewMode)
{
    if (Targets.Num() < 3)
    {
        return; // targets not spawned yet
    }

    // reset everything
    Mode = NewMode;
    bRunning = true;
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
    for (ATargetSphere* Target : Targets)
    {
        Target->Velocity = FVector::ZeroVector;
        Target->bBobbing = false;
    }

    FLinearColor ModeColor = RayShotModes::GetColor(Mode);

    // set up the targets for this mode (they are visible during the countdown)
    switch (Mode)
    {
    case ERayShotMode::Gridshot:
        RoundLength = 60.0f;
        for (ATargetSphere* Target : Targets)
        {
            Target->SetRadius(30.0);
            Target->SetColor(ModeColor);
            Target->SetActorLocation(GetFreeGridPoint());
            Target->SetActive(true);
        }
        break;

    case ERayShotMode::Microshot:
        RoundLength = 60.0f;
        Targets[0]->SetRadius(12.0); // tiny!
        Targets[0]->SetColor(ModeColor);
        Targets[0]->SetActorLocation(GetRandomAreaPoint());
        Targets[0]->SetActive(true);
        break;

    case ERayShotMode::Tracking:
        RoundLength = 45.0f;
        Targets[0]->SetRadius(35.0);
        Targets[0]->SetColor(RayShotStyle::Magenta); // turns green while you're on it
        Targets[0]->SetActorLocation(GetActorLocation());
        Targets[0]->SetActive(true);
        break;

    case ERayShotMode::Reflex:
        RoundLength = 0.0f; // no timer, the round ends after ReflexTargets targets
        Targets[0]->SetRadius(30.0);
        Targets[0]->SetColor(ModeColor);
        break;
    }

    TimeLeft = RoundLength;
}

void ATargetManager::StopRound()
{
    bRunning = false;
    bCountingDown = false;
    ShowIdleTargets();
}

void ATargetManager::FinishRound()
{
    bRunning = false;
    float FinalScore = GetScore();

    ShowIdleTargets();

    // tell the game mode, which saves the best score and shows results
    ARayShotGameMode* GameMode = GetWorld()->GetAuthGameMode<ARayShotGameMode>();
    if (GameMode)
    {
        GameMode->OnRoundFinished(FinalScore);
    }
}

void ATargetManager::HideAllTargets()
{
    for (ATargetSphere* Target : Targets)
    {
        Target->SetActive(false);
    }
}

void ATargetManager::ShowIdleTargets()
{
    // three big floating targets in a row (they can't be shot - no round is running)
    const FLinearColor Colors[3] = { RayShotStyle::Cyan, RayShotStyle::Purple, RayShotStyle::Orange };

    for (int32 i = 0; i < Targets.Num() && i < 3; i++)
    {
        ATargetSphere* Target = Targets[i];
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

void ATargetManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // remove old popups (older than 0.9 s)
    float CurrentTime = Now();
    Popups.RemoveAll([CurrentTime](const FScorePopup& Popup)
    {
        return CurrentTime - Popup.SpawnTime > 0.9f;
    });

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

    if (Mode == ERayShotMode::Tracking)
    {
        UpdateTracking(DeltaTime);
    }
    else if (Mode == ERayShotMode::Reflex)
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

void ATargetManager::UpdateCountdown(float DeltaTime)
{
    CountdownLeft -= DeltaTime;

    // beep once for each number: 3, 2, 1
    int32 Number = FMath::CeilToInt(CountdownLeft);
    if (CountdownLeft > 0.0f && Number != LastCountdownNumber)
    {
        LastCountdownNumber = Number;
        RayShotSounds::PlayCountdown(this, GetVolume());
    }

    // GO!
    if (CountdownLeft <= 0.0f)
    {
        bCountingDown = false;
        GoTime = Now();
        RayShotSounds::PlayRoundStart(this, GetVolume());

        if (Mode == ERayShotMode::Tracking)
        {
            PickNewTrackingDirection(Targets[0]);
        }
        else if (Mode == ERayShotMode::Reflex)
        {
            ReflexWaitTimer = FMath::RandRange(1.0f, 2.5f);
        }
    }
}

void ATargetManager::UpdateTracking(float DeltaTime)
{
    ATargetSphere* Target = Targets[0];

    // directions of the target area
    FVector Center = GetActorLocation();
    FVector Right = GetActorRightVector();
    FVector Up = GetActorUpVector();

    // move the target
    FVector NewLocation = Target->GetActorLocation() + Target->Velocity * DeltaTime;

    // where is it inside the area? (X = sideways, Y = up/down)
    FVector Offset = NewLocation - Center;
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

void ATargetManager::PickNewTrackingDirection(ATargetSphere* Target)
{
    float Angle = FMath::RandRange(0.0f, 2.0f * PI);
    float Speed = FMath::RandRange(150.0f, 350.0f); // cm per second

    FVector Right = GetActorRightVector();
    FVector Up = GetActorUpVector();
    Target->Velocity = (Right * FMath::Cos(Angle) + Up * FMath::Sin(Angle)) * Speed;

    DirectionTimer = FMath::RandRange(0.4f, 1.2f);
}

void ATargetManager::UpdateReflex(float DeltaTime)
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

void ATargetManager::UpdateAim(const FVector& Origin, const FVector& Direction, float DeltaTime)
{
    if (!bRunning || bCountingDown || Mode != ERayShotMode::Tracking)
    {
        return;
    }

    // is the crosshair on the target right now? (my ray tracer math again)
    bool bOnTarget = FindHitTarget(Origin, Direction) != nullptr;
    if (bOnTarget)
    {
        OnTargetTime += DeltaTime;
    }

    // green while you're on it, pink when you're off (only recolor when it changes)
    if (bOnTarget != bTrackingOnTarget)
    {
        bTrackingOnTarget = bOnTarget;
        Targets[0]->SetColor(bOnTarget ? RayShotStyle::Green : RayShotStyle::Magenta);
    }
}

// =====================================================================
//                               SHOOTING
// =====================================================================

void ATargetManager::HandleClick(const FVector& Origin, const FVector& Direction)
{
    if (!bRunning || bCountingDown || Mode == ERayShotMode::Tracking)
    {
        return; // no shooting during the countdown, and Tracking doesn't use clicks
    }

    // Reflex: clicking before the target shows up is "too early"
    if (Mode == ERayShotMode::Reflex && !bReflexVisible)
    {
        EarlyClicks++;
        TooEarlyTime = Now();
        ReflexWaitTimer = FMath::RandRange(1.0f, 2.5f); // restart the wait
        RayShotSounds::PlayMiss(this, GetVolume());
        return;
    }

    Shots++;

    ATargetSphere* HitTarget = FindHitTarget(Origin, Direction);
    if (HitTarget == nullptr)
    {
        RayShotSounds::PlayMiss(this, GetVolume());
        return;
    }

    Hits++;
    OnTargetHit(HitTarget);
}

ATargetSphere* ATargetManager::FindHitTarget(const FVector& Origin, const FVector& Direction) const
{
    FVector RayDirection = Direction.GetSafeNormal();

    // same idea as calculateColor in my C code: find the CLOSEST sphere the ray hits
    double ClosestT = TNumericLimits<double>::Max(); // like INFINITY in my C code
    ATargetSphere* ClosestTarget = nullptr;

    for (ATargetSphere* Target : Targets)
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

void ATargetManager::OnTargetHit(ATargetSphere* Target)
{
    LastHitTime = Now();
    FVector HitLocation = Target->GetActorLocation();

    // hit effect + sound
    AHitBurst* Burst = GetWorld()->SpawnActor<AHitBurst>(
        AHitBurst::StaticClass(), HitLocation, FRotator::ZeroRotator);
    if (Burst)
    {
        Burst->Setup(Target->GetColor(), (float)Target->Radius);
    }
    RayShotSounds::PlayHit(this, GetVolume());

    // what happens next depends on the mode
    switch (Mode)
    {
    case ERayShotMode::Gridshot:
        AddPopup(HitLocation, TEXT("+1"), Target->GetColor());
        Target->MoveTo(GetFreeGridPoint()); // jump to a different empty cell
        break;

    case ERayShotMode::Microshot:
        AddPopup(HitLocation, TEXT("+1"), Target->GetColor());
        Target->MoveTo(GetRandomAreaPoint());
        break;

    case ERayShotMode::Reflex:
    {
        // stop the stopwatch
        float ReactionMs = (Now() - ReflexAppearTime) * 1000.0f;
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

    case ERayShotMode::Tracking:
        break; // (not used - Tracking has no clicking)
    }
}

void ATargetManager::AddPopup(const FVector& Location, const FString& Text, const FLinearColor& Color)
{
    FScorePopup Popup;
    Popup.Location = Location;
    Popup.SpawnTime = Now();
    Popup.Text = Text;
    Popup.Color = Color;
    Popups.Add(Popup);
}

// =====================================================================
//                              POSITIONS
// =====================================================================

FVector ATargetManager::GetFreeGridPoint() const
{
    // the grid is centered on this actor and faces the way this actor faces
    FVector Center = GetActorLocation();
    FVector Right = GetActorRightVector();
    FVector Up = GetActorUpVector();
    double Half = (GridSize - 1) / 2.0;

    // try random cells until we find an empty one
    for (int32 Attempt = 0; Attempt < 50; Attempt++)
    {
        int32 Col = FMath::RandRange(0, GridSize - 1);
        int32 Row = FMath::RandRange(0, GridSize - 1);
        FVector Point = Center + Right * (Col - Half) * GridSpacing
                               + Up    * (Row - Half) * GridSpacing;

        // check that no active target is already in this cell
        bool bTaken = false;
        for (ATargetSphere* Target : Targets)
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

FVector ATargetManager::GetRandomAreaPoint() const
{
    double X = FMath::RandRange(-AreaHalfWidth, AreaHalfWidth);
    double Y = FMath::RandRange(-AreaHalfHeight, AreaHalfHeight);
    return GetActorLocation() + GetActorRightVector() * X + GetActorUpVector() * Y;
}

// =====================================================================
//                             SCORE + HUD TEXT
// =====================================================================

float ATargetManager::GetAccuracy() const
{
    if (Shots == 0)
    {
        return 0.0f; // avoid dividing by zero
    }
    return 100.0f * (float)Hits / (float)Shots;
}

float ATargetManager::GetAverageReaction() const
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

float ATargetManager::GetScore() const
{
    switch (Mode)
    {
    case ERayShotMode::Gridshot:
    case ERayShotMode::Microshot:
        return (float)Hits;

    case ERayShotMode::Tracking:
        // percent of the time the crosshair was on the target
        return ElapsedTime > 0.0f ? 100.0f * OnTargetTime / ElapsedTime : 0.0f;

    case ERayShotMode::Reflex:
        return GetAverageReaction();
    }
    return 0.0f;
}

float ATargetManager::GetTimeSinceGo() const
{
    return Now() - GoTime;
}

FString ATargetManager::GetTimerText() const
{
    if (RoundLength > 0.0f)
    {
        return FString::Printf(TEXT("%.1f"), TimeLeft);
    }
    return FString::Printf(TEXT("%d / %d"), ReactionTimesMs.Num(), ReflexTargets);
}

float ATargetManager::GetTimerFraction() const
{
    if (RoundLength > 0.0f)
    {
        return FMath::Clamp(TimeLeft / RoundLength, 0.0f, 1.0f);
    }
    return 1.0f - (float)ReactionTimesMs.Num() / (float)FMath::Max(ReflexTargets, 1);
}

bool ATargetManager::IsLowOnTime() const
{
    return bRunning && !bCountingDown && RoundLength > 0.0f && TimeLeft < 5.0f;
}

TArray<FStatLine> ATargetManager::GetLiveStats() const
{
    TArray<FStatLine> Lines;

    switch (Mode)
    {
    case ERayShotMode::Gridshot:
    case ERayShotMode::Microshot:
        Lines.Add({ TEXT("Hits"), FString::FromInt(Hits) });
        Lines.Add({ TEXT("Accuracy"), FString::Printf(TEXT("%.1f%%"), GetAccuracy()) });
        break;

    case ERayShotMode::Tracking:
        Lines.Add({ TEXT("On target"), FString::Printf(TEXT("%.1f%%"), GetScore()) });
        break;

    case ERayShotMode::Reflex:
        Lines.Add({ TEXT("Last"), ReactionTimesMs.Num() > 0 ? FString::Printf(TEXT("%.0f ms"), ReactionTimesMs.Last()) : FString(TEXT("-")) });
        Lines.Add({ TEXT("Average"), ReactionTimesMs.Num() > 0 ? FString::Printf(TEXT("%.0f ms"), GetAverageReaction()) : FString(TEXT("-")) });
        Lines.Add({ TEXT("Early"), FString::FromInt(EarlyClicks) });
        break;
    }

    return Lines;
}

FString ATargetManager::GetCenterMessage() const
{
    if (!bRunning || bCountingDown || Mode != ERayShotMode::Reflex)
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

TArray<FStatLine> ATargetManager::GetResultLines() const
{
    TArray<FStatLine> Lines;

    switch (Mode)
    {
    case ERayShotMode::Gridshot:
    case ERayShotMode::Microshot:
        Lines.Add({ TEXT("Hits"), FString::FromInt(Hits) });
        Lines.Add({ TEXT("Shots fired"), FString::FromInt(Shots) });
        Lines.Add({ TEXT("Accuracy"), FString::Printf(TEXT("%.1f%%"), GetAccuracy()) });
        Lines.Add({ TEXT("Hits per second"), FString::Printf(TEXT("%.2f"), ElapsedTime > 0.0f ? Hits / ElapsedTime : 0.0f) });
        break;

    case ERayShotMode::Tracking:
        Lines.Add({ TEXT("Time on target"), FString::Printf(TEXT("%.1f s"), OnTargetTime) });
        Lines.Add({ TEXT("Round length"), FString::Printf(TEXT("%.0f s"), ElapsedTime) });
        Lines.Add({ TEXT("On target"), FString::Printf(TEXT("%.1f%%"), GetScore()) });
        break;

    case ERayShotMode::Reflex:
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

float ATargetManager::GetVolume() const
{
    ARayShotGameMode* GameMode = GetWorld()->GetAuthGameMode<ARayShotGameMode>();
    return GameMode ? GameMode->GetVolume() : 0.7f;
}

float ATargetManager::Now() const
{
    return GetWorld()->GetTimeSeconds();
}
