// GEShotTour.h
// -ShotTour only: a scripted, input-free tour through every screen and feature of the game that saves
// a screenshot at each stop (into <Saved>/ShotTour, or -ShotDir=<folder>). Never active in normal play.
// It drives the real HUD flow and the real game systems; the tour autoplayer only moves the view and
// presses fire, every hit is still decided by RayTraceMath::DoesIntersect inside the game.
#pragma once

#include "CoreMinimal.h"

class AGEGameMode;
class AGEHUD;
class AGECharacter;
class AGETargetManager;
class UGETrainingManager;
class UGESaveGame;
class APlayerController;

struct FGEShotTour
{
	explicit FGEShotTour(AGEGameMode* InOwner);

	/** Called every frame by AGEGameMode while -ShotTour is on. */
	void Tick(float DeltaSeconds);

private:
	struct FStep
	{
		FString Name;                 // shot name (no number / extension); empty = no shot
		float Settle = 1.0f;          // seconds (counted while Cond is true) before the shot
		TFunction<void()> Enter;      // runs once when the step starts
		TFunction<bool()> Cond;       // optional: the settle timer only runs while this is true
	};

	enum class EAuto : uint8 { None, AimClicks, AimTrack, Bots, Flick };

	AGEGameMode* Owner = nullptr;
	TArray<FStep> Steps;
	int32 Index = -1;
	float StepTime = 0.0f;
	float StepWall = 0.0f;
	float TotalTime = 0.0f;
	float BootTime = 0.0f;
	int32 ShotCounter = 0;
	int32 ShotWait = -1;          // frames left after a screenshot request
	bool bBuilt = false;
	bool bFinished = false;
	FString OutDir;

	// autoplayer
	EAuto Auto = EAuto::None;
	float AimSpeed = 14.0f;
	float FireInterval = 0.45f;
	float Reaction = 0.15f;
	bool bPreferHead = false;
	bool bAutoFire = true;
	bool bPreferFar = false;
	FRotator LaneRot = FRotator::ZeroRotator;
	float SinceFire = 0.0f;
	float SeenFor = 0.0f;
	FVector LastTargetPos = FVector::ZeroVector;

	// scripted movement
	bool bMoveForward = false;

	// accessors
	APlayerController* PC() const;
	AGEHUD* HUD() const;
	AGECharacter* Char() const;
	AGETargetManager* Aim() const;
	UGETrainingManager* Train() const;
	UGESaveGame* Save() const;

	void Log(const FString& Line) const;
	void Build();
	void Add(const FString& Name, float Settle, TFunction<void()> Enter = nullptr, TFunction<bool()> Cond = nullptr);
	void Skip(const FString& Name, const FString& Why) const;
	void Capture(const FString& Name);
	void Finish(const FString& Why);
	void AddFakeHistory();
	void NextStep();

	void TickAuto(float Dt);
	void AutoAimClicks(float Dt, bool bTrackOnly);
	void AutoBots(float Dt);
	void AimViewAt(const FVector& Target, float Dt);

	/** Sets every setting the tour touched back to its default. */
	void ResetSettings();
	void BuildAimSteps(int32 ModeIndex);
};
