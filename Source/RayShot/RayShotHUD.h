// RayShotHUD.h
// Draws everything on screen: menus, crosshair, timer, stats and results.
// Uses only Unreal's Canvas (rectangles, lines and text), so no UI assets are needed.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RayShotTypes.h"
#include "RayShotHUD.generated.h"

class ARayShotGameMode;
class ATargetManager;
class UFont;

UCLASS()
class RAYSHOT_API ARayShotHUD : public AHUD
{
    GENERATED_BODY()

public:
    ARayShotHUD();

    // called every frame to draw on the screen
    virtual void DrawHUD() override;

private:
    // number font (all digits are the same width, so timers don't wobble)
    UPROPERTY()
    TObjectPtr<UFont> MonoFont;

    // ---------- one function per screen ----------
    void DrawPlaying(ARayShotGameMode* GameMode, ATargetManager* Manager);
    void DrawMainMenu(ARayShotGameMode* GameMode);
    void DrawSettings(ARayShotGameMode* GameMode);
    void DrawResults(ARayShotGameMode* GameMode, ATargetManager* Manager);

    // ---------- pieces of the playing screen ----------
    void DrawCrosshair(FLinearColor Color);
    void DrawHitMarker(ATargetManager* Manager);
    void DrawCountdown(ATargetManager* Manager, FLinearColor ModeColor);
    void DrawPopups(ATargetManager* Manager);

    // ---------- widgets ----------
    // a menu button; also handles mouse hover. Returns true if the mouse is over it
    bool DrawButton(ARayShotGameMode* GameMode, int32 Index, float X, float Y, float W, float H,
                    const FString& Label, const FString& RightText, FLinearColor Accent, float Alpha);
    void DrawSlider(float X, float Y, float W, float Fraction, FLinearColor Accent, float Alpha);

    // ---------- drawing helpers ----------
    void DrawBox(float X, float Y, float W, float H, FLinearColor Color);
    void DrawPanel(float X, float Y, float W, float H, float Alpha);
    void DrawGradient(float X, float Y, float W, float H, FLinearColor From, FLinearColor To, bool bHorizontal);
    // PixelSize = text height at 1080p. AlignX: 0 = left, 0.5 = center, 1 = right
    void DrawLabel(const FString& Text, float X, float Y, FLinearColor Color, float PixelSize,
                   float AlignX = 0.0f, bool bMono = false, bool bShadow = true);
    void DrawGlowLabel(const FString& Text, float X, float Y, FLinearColor Color, FLinearColor GlowColor,
                       float PixelSize, float AlignX = 0.0f, bool bMono = false);
    FVector2D MeasureText(const FString& Text, float PixelSize, bool bMono = false);
    UFont* GetFont(bool bMono) const;
    float GetTextScale(UFont* Font, float PixelSize) const;

    // converts a size designed for 1080p to the real screen size
    float Px(float Value) const { return Value * UIScale; }

    // time since the current screen appeared (for intro animations)
    float ScreenAge() const;

    // ---------- state ----------
    float UIScale = 1.0f;
    float FrameDeltaTime = 0.0f;
    float NowTime = 0.0f;

    ERayShotScreen LastScreen = ERayShotScreen::Playing;
    float ScreenStartTime = 0.0f;

    // per-button highlight amount (0 = normal, 1 = selected), animated smoothly
    TArray<float> ButtonAnim;

    // mouse
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    float LastMouseX = -1.0f;
    float LastMouseY = -1.0f;
    bool bHasMouse = false;
    bool bMouseMoved = false;
};
