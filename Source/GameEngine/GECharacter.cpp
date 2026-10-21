#include "GECharacter.h"

AGECharacter::AGECharacter()
{
	// Nothing here on purpose: the Blueprint child sets up camera, mesh and input.
}

void AGECharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = Stats.MaxHealth;
	OnCharacterReady();
}

void AGECharacter::OnCharacterReady_Implementation()
{
	// Default does nothing. Later phases / Blueprints override this.
}
