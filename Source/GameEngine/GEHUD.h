#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GEHUD.generated.h"

/** Look of the crosshair. All sizes are in screen pixels. */
USTRUCT(BlueprintType)
struct FGECrosshairSettings
{
	GENERATED_BODY()

	/** Main color of the four crosshair lines. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair")
	FLinearColor Color = FLinearColor::White;

	/** Color of the outline drawn around each line (helps on bright backgrounds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair")
	FLinearColor OutlineColor = FLinearColor::Black;

	/** Length of each of the four lines. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair", meta = (ClampMin = "1.0"))
	float Size = 10.0f;

	/** Width of each line. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair", meta = (ClampMin = "1.0"))
	float Thickness = 2.0f;

	/** Empty space between the screen center and the start of each line. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair", meta = (ClampMin = "0.0"))
	float Gap = 4.0f;

	/** How many pixels the outline extends past the line on every side. 0 = no outline. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair", meta = (ClampMin = "0.0"))
	float OutlineThickness = 1.0f;
};

/**
 * Simple Canvas HUD that draws an outlined "+" crosshair with a gap in the
 * middle, at the center of the screen.
 */
UCLASS(Blueprintable)
class GAMEENGINE_API AGEHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** Called every frame by the engine to draw the HUD. */
	virtual void DrawHUD() override;

protected:
	/** Crosshair appearance. Edit in a Blueprint child's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair")
	FGECrosshairSettings Crosshair;

	/** Seconds the hit marker stays on screen. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|HUD")
	float HitMarkerDuration = 0.25f;

	/** Phase 3: receives clicks on the menu buttons (hit boxes added in DrawTrainingMenu). */
	virtual void NotifyHitBoxClick(FName BoxName) override;

private:
	// Phase 3 Training Center UI
	void DrawTrainingOverlay();
	void DrawTrainingMenu(class UGETrainingManager& Manager);
	void DrawPopups(class UGETrainingManager& Manager);
	/** Phase 4: per-direction table, overshoot and sensitivity suggestion. */
	void DrawFlickResults(class UGETrainingManager& Manager, float X, float Y);
	/** Draws a clickable button and registers its hit box. */
	void DrawButton(const FString& Label, const FName& Name, float X, float Y, float W, float H, bool bSelected);

	void DrawWeaponInfo(const class UGEWeaponComponent& Weapon);
	void DrawHitMarker(const class UGEWeaponComponent& Weapon, float CX, float CY);
	void DrawScope(float CX, float CY);

	/** Draws one filled rectangle plus its outline behind it. */
	void DrawOutlinedRect(float X, float Y, float W, float H);
};
