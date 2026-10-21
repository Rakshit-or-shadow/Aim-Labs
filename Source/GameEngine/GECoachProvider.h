#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GECoachTypes.h"
#include "GECoachProvider.generated.h"

UINTERFACE(MinimalAPI)
class UGECoachProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything that can turn session data into coaching tips (Phase 5).
 *
 * TODO (future): an LLM-backed provider (e.g. UGELlmCoach) can implement this same
 * interface: serialize FGECoachInput to JSON, send it to a model, parse the reply into
 * FGECoachTip entries. It is NOT implemented; the offline rule-based UGECoach is the only provider.
 */
class GAMEENGINE_API IGECoachProvider
{
	GENERATED_BODY()

public:
	/** Fills OutTips with candidate tips (any number, any order). The caller ranks and trims them. */
	virtual void GenerateTips(const FGECoachInput& Input, TArray<FGECoachTip>& OutTips) const = 0;
};
