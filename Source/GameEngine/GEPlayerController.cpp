#include "GEPlayerController.h"
#include "GETrainingManager.h"

AGEPlayerController::AGEPlayerController()
{
	// Intentionally empty: the Blueprint child configures input and camera manager.
}

float AGEPlayerController::GetSensitivityScale() const
{
	const UGETrainingManager* Manager = UGETrainingManager::Get(this);
	return Manager ? Manager->GetSensitivityScale() : 1.0f;
}

void AGEPlayerController::AddYawInput(float Val)
{
	Super::AddYawInput(Val * GetSensitivityScale());
}

void AGEPlayerController::AddPitchInput(float Val)
{
	Super::AddPitchInput(Val * GetSensitivityScale());
}
