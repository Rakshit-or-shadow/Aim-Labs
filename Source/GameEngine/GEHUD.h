#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GEAimTypes.h"
#include "GEHUD.generated.h"

/** Look of the crosshair. All sizes are in screen pixels. */
USTRUCT(BlueprintType)
struct FGECrosshairSettings
{
	GENERATED_BODY()

	/** Fallback color of the four crosshair lines (the saved crosshair color setting overrides it). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair")
	FLinearColor Color = FLinearColor::White;

	/** Color of the outline drawn around each line (helps on bright backgrounds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair")
	FLinearColor OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.75f);

	/** Length of each of the four lines. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair", meta = (ClampMin = "1.0"))
	float Size = 9.0f;

	/** Width of each line. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair", meta = (ClampMin = "1.0"))
	float Thickness = 2.0f;

	/** Empty space between the screen center and the start of each line. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair", meta = (ClampMin = "0.0"))
	float Gap = 5.0f;

	/** How many pixels the outline extends past the line on every side. 0 = no outline. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE|Crosshair", meta = (ClampMin = "0.0"))
	float OutlineThickness = 1.0f;
};

struct FGEUIKit;

/** What the Results screen is showing. */
enum class EGEResultKind : uint8
{
	Aim,       // one of the 4 aim modes
	Training,  // a Training Center session (bots)
	Flick      // a Flick Optimizer session
};

/**
 * The ONE HUD of the game. It draws everything and runs the menu flow:
 *
 *   Main Menu -> Aim Trainer (mode select) -> round (countdown, play) -> Results
 *             -> Training Center / Flick Optimizer (the old Phase 3-5 systems)
 *             -> Free Roam (normal movement and weapons)
 *             -> Settings (sensitivity, FOV, volume, crosshair color)
 *   Tab while playing = Pause (Resume / Restart / Settings / Main Menu).
 *
 * The current screen is EGEScreen (see GEAimTypes.h). The flow only starts in a level that has the
 * aim arena (Lvl_IntroRoom); in other levels the HUD just draws the crosshair and weapon info.
 *
 * The code is split in two files: GEHUD.cpp (crosshair, Training Center overlay, DrawHUD) and
 * GEHUDScreens.cpp (flow/input and every menu screen).
 */
UCLASS(Blueprintable)
class GAMEENGINE_API AGEHUD : public AHUD
{
	GENERATED_BODY()

	/** The UI component library draws through this HUD's private text helpers. */
	friend struct FGEUIKit;

public:
	AGEHUD();

	/** Called every frame by the engine to draw the HUD. */
	virtual void DrawHUD() override;
	/** Runs the menu flow (keys, screen changes) and applies the FOV setting. */
	virtual void Tick(float DeltaSeconds) override;

	EGEScreen GetScreen() const { return Screen; }
	/** True when T (Training Center teleport) and M (its menu) may be used right now. */
	bool AllowTrainingHotkeys() const;

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
	// =====================================================================
	//  GEHUD.cpp : gameplay HUD
	// =====================================================================
	/** Crosshair + weapon info + Training Center overlay (free roam, Training Center, Flick Optimizer). */
	void DrawGameplayHUD();
	/** Outlined "+" crosshair with a center dot. ExtraGap widens it (weapon spread). */
	void DrawCrosshairAt(float CX, float CY, const FLinearColor& Color, float ExtraGap);

	// Phase 3 Training Center UI
	void DrawTrainingOverlay();
	void DrawTrainingMenu(class UGETrainingManager& Manager);
	void DrawPopups(class UGETrainingManager& Manager);
	/** Phase 4: per-direction table, overshoot and sensitivity suggestion. */
	void DrawFlickResults(class UGETrainingManager& Manager, float X, float Y);
	/** Phase 5: the coach panel (3 tips + trends) after any session. */
	void DrawCoachPanel(class UGETrainingManager& Manager, float X, float Y);
	/** Draws text word-wrapped to MaxWidth. Returns the Y below the last line. */
	float DrawWrappedText(const FString& Text, const FLinearColor& Color, float X, float Y, float MaxWidth);
	/** Draws a clickable button and registers its hit box. */
	void DrawButton(const FString& Label, const FName& Name, float X, float Y, float W, float H, bool bSelected);

	void DrawWeaponInfo(const class UGEWeaponComponent& Weapon);
	void DrawHitMarker(const class UGEWeaponComponent& Weapon, float CX, float CY);
	void DrawScope(float CX, float CY);

	// =====================================================================
	//  GEHUDScreens.cpp : flow and input
	// =====================================================================
	void StartFlow();                          // first tick: decide if menus exist in this level
	void SyncWithGame();                       // keep Screen in line with the aim manager (console commands etc.)
	void HandleKeys();                         // keyboard + mouse for the menu screens
	void SetScreen(EGEScreen NewScreen, int32 NewSelection);
	void ApplyInputState();                    // cursor, input mode, movement/look/firing for the current screen
	void ApplyFov();                           // saved FOV -> weapon/camera, every frame
	bool IsMenuScreen() const;                 // screens that show the mouse cursor
	bool ScreenNeedsArena() const;             // screens where the player stands in the aim arena
	int32 GetItemCount() const;
	TArray<int32> GetPauseItems() const;       // 0 Resume, 1 Restart, 2 Settings, 3 Main Menu

	// actions
	void GoToMainMenu();
	void StartAimMode(EGEAimMode Mode);
	void StartFreeRoam();
	void StartTrainingScreen(bool bFlick);
	void OpenPause();
	void ResumeFromPause();
	void PlayAgain();
	void GoBack();
	void ActivateSelected();
	void MoveSelection(int32 Direction);
	void ChangeSetting(int32 Index, int32 Direction);
	void ShowTrainingResults();

	// helpers for the screens
	class UGESaveGame* GetSave() const;
	float GetVolume() const;
	class AGETargetManager* GetAim() const;
	class AGECharacter* GetGECharacter() const;

	// =====================================================================
	//  GEHUDScreens.cpp : drawing the screens
	// =====================================================================
	void DrawMainMenu();
	void DrawModeSelect();
	void DrawSettings();
	void DrawResults();
	void DrawPause();
	void DrawPlaying();
	void DrawCountdown(const FLinearColor& ModeColor);
	void DrawAimHitMarker();
	void DrawAimPopups();
	void DrawBackdrop(float Alpha1, float Alpha2, bool bHorizontal);
	void DrawInfoCard(int32 Selected);

	// widgets
	/** Mouse hit test for menu item Index (updates HoveredItem / SelectedItem). Drawing is done by GEUIKit. */
	bool HitTestItem(int32 Index, float X, float Y, float W, float H);
	/** Height (real px) that DrawWrappedLabel would use for this text. */
	float MeasureWrapped(const FString& Text, float MaxWidth, float PixelSize);

	// drawing helpers (sizes are designed for 1080p and scaled by Px)
	void DrawBox(float X, float Y, float W, float H, FLinearColor Color);
	void DrawPanel(float X, float Y, float W, float H, float Alpha);
	void DrawGradient(float X, float Y, float W, float H, FLinearColor From, FLinearColor To, bool bHorizontal);
	void DrawLabel(const FString& Text, float X, float Y, FLinearColor Color, float PixelSize,
		float AlignX = 0.0f, bool bMono = false, bool bShadow = true);
	void DrawGlowLabel(const FString& Text, float X, float Y, FLinearColor Color, FLinearColor GlowColor,
		float PixelSize, float AlignX = 0.0f, bool bMono = false);
	/** Word-wrapped label. MaxWidth is in real pixels. Returns the Y below the last line. */
	float DrawWrappedLabel(const FString& Text, float X, float Y, float MaxWidth, FLinearColor Color, float PixelSize);
	FVector2D MeasureText(const FString& Text, float PixelSize, bool bMono = false);
	UFont* GetFont(bool bMono);
	/** Loads Rajdhani / Share Tech Mono once (falls back to Roboto / DroidSansMono) and logs which. */
	void EnsureFonts();
	/** A copy of the font made at the right size, so text is sharp instead of stretched. */
	UFont* GetSizedFont(bool bMono, float PixelSize, float& OutScale);
	float Px(float Value) const { return Value * UIScale; }
	float ScreenAge() const { return NowTime - ScreenStartTime; }

	// =====================================================================
	//  state
	// =====================================================================
	EGEScreen Screen = EGEScreen::FreeRoam;
	EGEScreen SettingsReturn = EGEScreen::MainMenu; // where Settings -> Back goes
	EGEScreen PauseReturn = EGEScreen::FreeRoam;    // what Resume goes back to
	EGEScreen ResultReturn = EGEScreen::FreeRoam;   // training results: screen to return to on Play Again
	EGEResultKind ResultKind = EGEResultKind::Aim;
	int32 SelectedItem = 0;
	int32 HoveredItem = -1;
	bool bFlowChecked = false;   // StartFlow ran
	bool bFlowActive = false;    // this level has the aim arena, so menus are on
	bool bWasSessionRunning = false;

	// screen intro animation + mouse
	EGEScreen LastDrawnScreen = EGEScreen::FreeRoam;
	float ScreenStartTime = 0.0f;
	TArray<float> ButtonAnim;
	// low-time warning on the round timer (starts a short glitch once)
	bool bWasLowTime = false;
	float LowTimeStartTime = -100.0f;
	float UIScale = 1.0f;
	float FrameDeltaTime = 0.0f;
	float NowTime = 0.0f;
	bool bHasMouse = false;
	bool bMouseMoved = false;
	float MouseX = 0.0f, MouseY = 0.0f, LastMouseX = 0.0f, LastMouseY = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UFont> MonoFont;
	/** UI text font: Rajdhani SemiBold, or null to use the engine font. */
	UPROPERTY(Transient)
	TObjectPtr<UFont> UIFont;
	bool bFontsLoaded = false;
	/** Cached sized copies of the fonts (key = size bucket * 2 + mono flag). */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UFont>> SizedFonts;
};
