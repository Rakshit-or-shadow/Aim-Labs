// RayShotHUD.cpp

#include "RayShotHUD.h"
#include "RayShotGameMode.h"
#include "RayShotSaveGame.h"
#include "RayShotModes.h"
#include "RayShotStyle.h"
#include "TargetManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

// smooth 0 -> 1 curve (fast start, gentle end) used for intro animations
static float HUDEaseOut(float T)
{
    T = FMath::Clamp(T, 0.0f, 1.0f);
    return 1.0f - FMath::Pow(1.0f - T, 3.0f);
}

ARayShotHUD::ARayShotHUD()
{
    static ConstructorHelpers::FObjectFinder<UFont> Mono(TEXT("/Engine/EngineFonts/DroidSansMono.DroidSansMono"));
    if (Mono.Succeeded())
    {
        MonoFont = Mono.Object;
    }
}

// =====================================================================
//                               MAIN DRAW
// =====================================================================

void ARayShotHUD::DrawHUD()
{
    Super::DrawHUD();

    if (Canvas == nullptr)
    {
        return;
    }

    ARayShotGameMode* GameMode = GetWorld()->GetAuthGameMode<ARayShotGameMode>();
    if (GameMode == nullptr)
    {
        return;
    }
    ATargetManager* Manager = GameMode->GetManager();

    // everything is designed for a 1080p screen and scaled to the real size
    UIScale = Canvas->ClipY / 1080.0f;
    FrameDeltaTime = GetWorld()->GetDeltaSeconds();
    NowTime = GetWorld()->GetTimeSeconds();

    // where is the mouse, and did it move this frame?
    bHasMouse = PlayerOwner && PlayerOwner->GetMousePosition(MouseX, MouseY);
    bMouseMoved = bHasMouse &&
        (FMath::Abs(MouseX - LastMouseX) > 0.5f || FMath::Abs(MouseY - LastMouseY) > 0.5f);

    // a new screen appeared -> restart its intro animation
    if (GameMode->Screen != LastScreen)
    {
        LastScreen = GameMode->Screen;
        ScreenStartTime = NowTime;
        ButtonAnim.Empty();
    }

    // reset; DrawButton sets it again if the mouse is over a button
    GameMode->HoveredItem = -1;

    switch (GameMode->Screen)
    {
    case ERayShotScreen::Playing:  DrawPlaying(GameMode, Manager);  break;
    case ERayShotScreen::MainMenu: DrawMainMenu(GameMode);          break;
    case ERayShotScreen::Settings: DrawSettings(GameMode);          break;
    case ERayShotScreen::Results:  DrawResults(GameMode, Manager);  break;
    }

    if (bHasMouse)
    {
        LastMouseX = MouseX;
        LastMouseY = MouseY;
    }
}

float ARayShotHUD::ScreenAge() const
{
    return NowTime - ScreenStartTime;
}

// =====================================================================
//                               PLAYING
// =====================================================================

void ARayShotHUD::DrawPlaying(ARayShotGameMode* GameMode, ATargetManager* Manager)
{
    if (Manager == nullptr)
    {
        DrawCrosshair(GameMode->GetCrosshairColor());
        return;
    }

    const FLinearColor ModeColor = RayShotModes::GetColor(Manager->GetMode());
    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;

    // ---------- timer pill (top center) ----------
    {
        const float W = Px(240);
        const float H = Px(96);
        const float X = CenterX - W * 0.5f;
        const float Y = Px(28);
        DrawPanel(X, Y, W, H, 1.0f);

        // last 5 seconds: the timer pulses red
        FLinearColor TimerColor = RayShotStyle::TextWhite;
        if (Manager->IsLowOnTime())
        {
            float Pulse = 0.5f + 0.5f * FMath::Sin(NowTime * 10.0f);
            TimerColor = FMath::Lerp(RayShotStyle::TextWhite, RayShotStyle::Red, Pulse);
        }
        DrawLabel(Manager->GetTimerText(), CenterX, Y + Px(14), TimerColor, 46, 0.5f, true);

        // progress bar
        const float BarW = W - Px(40);
        const float BarY = Y + H - Px(18);
        DrawBox(X + Px(20), BarY, BarW, Px(4), FLinearColor(1, 1, 1, 0.1f));
        DrawBox(X + Px(20), BarY, BarW * Manager->GetTimerFraction(), Px(4), ModeColor);
    }

    // ---------- stats card (top left) ----------
    {
        TArray<FStatLine> Stats = Manager->GetLiveStats();
        const float X = Px(36);
        const float Y = Px(28);
        const float W = Px(300);
        const float RowH = Px(40);
        const float H = Px(62) + Stats.Num() * RowH;
        DrawPanel(X, Y, W, H, 1.0f);
        DrawBox(X, Y, Px(4), H, ModeColor); // accent stripe

        DrawLabel(RayShotModes::GetName(Manager->GetMode()).ToUpper(), X + Px(24), Y + Px(16), ModeColor, 20);

        for (int32 i = 0; i < Stats.Num(); i++)
        {
            float RowY = Y + Px(54) + i * RowH;
            DrawLabel(Stats[i].Label, X + Px(24), RowY + Px(4), RayShotStyle::TextDim, 20);
            DrawLabel(Stats[i].Value, X + W - Px(20), RowY, RayShotStyle::TextWhite, 26, 1.0f, true);
        }
    }

    // ---------- hints (bottom left) ----------
    DrawLabel(TEXT("R  restart        TAB  menu"), Px(40), Canvas->ClipY - Px(56),
              RayShotStyle::WithAlpha(RayShotStyle::TextDim, 0.8f), 17);

    // ---------- message under the crosshair (Reflex) ----------
    FString Message = Manager->GetCenterMessage();
    if (!Message.IsEmpty())
    {
        bool bEarly = Message.StartsWith(TEXT("Too"));
        float Pulse = 0.65f + 0.35f * FMath::Sin(NowTime * 6.0f);
        FLinearColor Color = bEarly ? RayShotStyle::Red : RayShotStyle::WithAlpha(RayShotStyle::TextWhite, Pulse);
        DrawLabel(Message, CenterX, CenterY + Px(70), Color, 30, 0.5f);
    }

    DrawPopups(Manager);
    DrawCountdown(Manager, ModeColor);
    DrawHitMarker(Manager);
    DrawCrosshair(GameMode->GetCrosshairColor()); // last, so it's on top
}

void ARayShotHUD::DrawCrosshair(FLinearColor Color)
{
    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;
    const float Gap = Px(5);
    const float Len = Px(9);
    const FLinearColor Outline(0.0f, 0.0f, 0.0f, 0.75f);

    // draw everything twice: thick black outline first, then the color on top
    for (int32 Pass = 0; Pass < 2; Pass++)
    {
        bool bOutline = (Pass == 0);
        FLinearColor C = bOutline ? Outline : Color;
        float Thick = bOutline ? 4.0f : 2.0f;

        DrawLine(CenterX - Gap - Len, CenterY, CenterX - Gap, CenterY, C, Thick);
        DrawLine(CenterX + Gap, CenterY, CenterX + Gap + Len, CenterY, C, Thick);
        DrawLine(CenterX, CenterY - Gap - Len, CenterX, CenterY - Gap, C, Thick);
        DrawLine(CenterX, CenterY + Gap, CenterX, CenterY + Gap + Len, C, Thick);

        float Dot = bOutline ? 4.0f : 2.0f;
        DrawBox(CenterX - Dot * 0.5f, CenterY - Dot * 0.5f, Dot, Dot, C);
    }
}

void ARayShotHUD::DrawHitMarker(ATargetManager* Manager)
{
    // white X that fades out over 0.15 s after a hit
    float Age = NowTime - Manager->LastHitTime;
    if (Age >= 0.15f)
    {
        return;
    }

    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;
    const float Inner = Px(9);
    const float Outer = Px(19);
    FLinearColor Color = RayShotStyle::WithAlpha(RayShotStyle::TextWhite, 1.0f - Age / 0.15f);

    DrawLine(CenterX - Outer, CenterY - Outer, CenterX - Inner, CenterY - Inner, Color, 2.0f);
    DrawLine(CenterX + Inner, CenterY + Inner, CenterX + Outer, CenterY + Outer, Color, 2.0f);
    DrawLine(CenterX + Outer, CenterY - Outer, CenterX + Inner, CenterY - Inner, Color, 2.0f);
    DrawLine(CenterX - Inner, CenterY + Inner, CenterX - Outer, CenterY + Outer, Color, 2.0f);
}

void ARayShotHUD::DrawCountdown(ATargetManager* Manager, FLinearColor ModeColor)
{
    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;

    if (Manager->IsCountingDown())
    {
        float Left = Manager->GetCountdownLeft();
        int32 Number = FMath::CeilToInt(Left);

        // each number starts big and settles; fades out at the end of its second
        float InSecond = Left - FMath::FloorToFloat(Left); // 1 -> 0 during each second
        float Size = 150.0f * (1.0f + 0.35f * FMath::Pow(InSecond, 4.0f));
        float Alpha = FMath::Clamp(InSecond * 4.0f, 0.0f, 1.0f);

        DrawLabel(TEXT("GET READY"), CenterX, CenterY - Px(250), RayShotStyle::WithAlpha(RayShotStyle::TextDim, 0.9f), 24, 0.5f);

        FString Text = FString::FromInt(Number);
        FVector2D TextSize = MeasureText(Text, Size, true);
        DrawGlowLabel(Text, CenterX, CenterY - Px(150) - TextSize.Y * 0.5f,
                      RayShotStyle::WithAlpha(RayShotStyle::TextWhite, Alpha),
                      RayShotStyle::WithAlpha(ModeColor, Alpha), Size, 0.5f, true);
        return;
    }

    // "GO!" for half a second after the countdown
    float SinceGo = Manager->GetTimeSinceGo();
    if (Manager->IsRunning() && SinceGo < 0.6f)
    {
        float Alpha = 1.0f - SinceGo / 0.6f;
        float Size = 130.0f * (1.0f + 0.4f * SinceGo);
        FVector2D TextSize = MeasureText(TEXT("GO!"), Size);
        DrawGlowLabel(TEXT("GO!"), CenterX, CenterY - Px(150) - TextSize.Y * 0.5f,
                      RayShotStyle::WithAlpha(RayShotStyle::TextWhite, Alpha),
                      RayShotStyle::WithAlpha(RayShotStyle::Green, Alpha), Size, 0.5f);
    }
}

void ARayShotHUD::DrawPopups(ATargetManager* Manager)
{
    for (const FScorePopup& Popup : Manager->GetPopups())
    {
        float Age = NowTime - Popup.SpawnTime;
        float Life = FMath::Clamp(Age / 0.9f, 0.0f, 1.0f);

        // float upward in the world, then turn it into a screen position
        FVector WorldPos = Popup.Location + FVector(0.0f, 0.0f, 50.0f + 90.0f * HUDEaseOut(Life));
        FVector ScreenPos = Project(WorldPos);
        if (ScreenPos.Z <= 0.0f)
        {
            continue; // behind the camera
        }

        FLinearColor Color = RayShotStyle::WithAlpha(FMath::Lerp(Popup.Color, RayShotStyle::TextWhite, 0.4f), 1.0f - Life);
        DrawLabel(Popup.Text, ScreenPos.X, ScreenPos.Y, Color, 24, 0.5f, true);
    }
}

// =====================================================================
//                              MAIN MENU
// =====================================================================

void ARayShotHUD::DrawMainMenu(ARayShotGameMode* GameMode)
{
    const float Age = ScreenAge();
    const float Intro = HUDEaseOut(Age / 0.4f);
    const float Left = Px(120);

    // dark on the left (behind the buttons), see-through on the right
    DrawGradient(0, 0, Canvas->ClipX, Canvas->ClipY,
                 FLinearColor(0.008f, 0.01f, 0.02f, 0.93f), FLinearColor(0.008f, 0.01f, 0.02f, 0.3f), true);

    // ---------- title ----------
    DrawGlowLabel(TEXT("RAYSHOT"), Left - Px(30) * (1.0f - Intro), Px(110),
                  RayShotStyle::WithAlpha(RayShotStyle::TextWhite, Intro),
                  RayShotStyle::WithAlpha(RayShotStyle::Cyan, Intro), 120);
    DrawBox(Left, Px(262), Px(90) * Intro, Px(5), RayShotStyle::Cyan);
    DrawLabel(TEXT("AIM TRAINER   //   HIT DETECTION BY MY OWN RAY TRACER"), Left, Px(284),
              RayShotStyle::WithAlpha(RayShotStyle::TextDim, Intro), 19);

    // ---------- buttons ----------
    const float ButtonW = Px(600);
    const float ButtonH = Px(62);
    const float Gap = Px(12);
    float Y = Px(360);

    for (int32 i = 0; i < RayShotModes::NumModes + 2; i++)
    {
        // little gap before Settings / Quit
        if (i == RayShotModes::NumModes)
        {
            Y += Px(18);
        }

        // buttons slide in one after another
        float ButtonIntro = HUDEaseOut((Age - 0.05f * i) / 0.3f);
        float X = Left - Px(40) * (1.0f - ButtonIntro);

        if (i < RayShotModes::NumModes)
        {
            ERayShotMode Mode = (ERayShotMode)i;
            FString Best = GameMode->HasBestScore(Mode) ? TEXT("BEST  ") + GameMode->GetBestScoreText(Mode) : FString(TEXT("NEW"));
            DrawButton(GameMode, i, X, Y, ButtonW, ButtonH, RayShotModes::GetName(Mode).ToUpper(), Best,
                       RayShotModes::GetColor(Mode), ButtonIntro);
        }
        else if (i == RayShotModes::NumModes)
        {
            DrawButton(GameMode, i, X, Y, ButtonW, ButtonH, TEXT("SETTINGS"), TEXT(""), RayShotStyle::TextDim, ButtonIntro);
        }
        else
        {
            DrawButton(GameMode, i, X, Y, ButtonW, ButtonH, TEXT("QUIT"), TEXT(""), RayShotStyle::Red, ButtonIntro);
        }

        Y += ButtonH + Gap;
    }

    // ---------- info card for the selected item (right side) ----------
    const float CardW = Px(520);
    const float CardX = Canvas->ClipX - Px(120) - CardW;
    const float CardY = Px(360);
    const int32 Selected = GameMode->SelectedItem;

    if (Selected < RayShotModes::NumModes)
    {
        ERayShotMode Mode = (ERayShotMode)Selected;
        FLinearColor ModeColor = RayShotModes::GetColor(Mode);
        const float CardH = Px(420);

        DrawPanel(CardX, CardY, CardW, CardH, Intro);
        DrawBox(CardX, CardY, CardW, Px(5), RayShotStyle::WithAlpha(ModeColor, Intro));

        float TextX = CardX + Px(40);
        DrawGlowLabel(RayShotModes::GetName(Mode).ToUpper(), TextX, CardY + Px(36),
                      RayShotStyle::TextWhite, ModeColor, 46);

        // description
        TArray<FString> Lines = RayShotModes::GetDescription(Mode);
        for (int32 i = 0; i < Lines.Num(); i++)
        {
            bool bLast = (i == Lines.Num() - 1);
            DrawLabel(Lines[i], TextX, CardY + Px(112) + i * Px(34),
                      bLast ? ModeColor : RayShotStyle::TextWhite, 22);
        }

        // divider
        DrawBox(TextX, CardY + Px(240), CardW - Px(80), Px(1), RayShotStyle::PanelBorder);

        // duration + personal best
        DrawLabel(TEXT("DURATION"), TextX, CardY + Px(266), RayShotStyle::TextDim, 16);
        DrawLabel(RayShotModes::GetDurationText(Mode), TextX, CardY + Px(292), RayShotStyle::TextWhite, 28);

        DrawLabel(TEXT("PERSONAL BEST"), TextX + Px(230), CardY + Px(266), RayShotStyle::TextDim, 16);
        if (GameMode->HasBestScore(Mode))
        {
            DrawLabel(GameMode->GetBestScoreText(Mode), TextX + Px(230), CardY + Px(292), RayShotStyle::Gold, 28, 0.0f, true);
        }
        else
        {
            DrawLabel(TEXT("not played yet"), TextX + Px(230), CardY + Px(296), RayShotStyle::TextDim, 22);
        }

        // call to action
        float Pulse = 0.6f + 0.4f * FMath::Sin(NowTime * 4.0f);
        DrawLabel(TEXT("PRESS ENTER OR CLICK TO PLAY"), TextX, CardY + CardH - Px(54),
                  RayShotStyle::WithAlpha(ModeColor, Pulse), 18);
    }
    else
    {
        const float CardH = Px(200);
        bool bSettings = (Selected == RayShotModes::NumModes);
        FLinearColor Accent = bSettings ? RayShotStyle::TextDim : RayShotStyle::Red;

        DrawPanel(CardX, CardY, CardW, CardH, Intro);
        DrawBox(CardX, CardY, CardW, Px(5), Accent);
        DrawLabel(bSettings ? TEXT("SETTINGS") : TEXT("QUIT"), CardX + Px(40), CardY + Px(36), RayShotStyle::TextWhite, 40);
        DrawLabel(bSettings ? TEXT("Sensitivity, field of view, volume") : TEXT("See you next session."),
                  CardX + Px(40), CardY + Px(104), RayShotStyle::TextDim, 22);
        if (bSettings)
        {
            DrawLabel(TEXT("and crosshair color."), CardX + Px(40), CardY + Px(138), RayShotStyle::TextDim, 22);
        }
    }

    // ---------- footer ----------
    DrawLabel(TEXT("W / S  or mouse to choose      ENTER / click to select"), Left, Canvas->ClipY - Px(64),
              RayShotStyle::TextDim, 17);
    DrawLabel(TEXT("RayShot v0.3"), Canvas->ClipX - Px(120), Canvas->ClipY - Px(64),
              RayShotStyle::WithAlpha(RayShotStyle::TextDim, 0.6f), 17, 1.0f);
}

// =====================================================================
//                               SETTINGS
// =====================================================================

void ARayShotHUD::DrawSettings(ARayShotGameMode* GameMode)
{
    URayShotSaveGame* Save = GameMode->SaveData;
    if (Save == nullptr)
    {
        return;
    }

    const float Intro = HUDEaseOut(ScreenAge() / 0.35f);

    DrawGradient(0, 0, Canvas->ClipX, Canvas->ClipY,
                 FLinearColor(0.008f, 0.01f, 0.02f, 0.9f), FLinearColor(0.008f, 0.01f, 0.02f, 0.7f), false);

    // ---------- card ----------
    const float CardW = Px(780);
    const float RowH = Px(86);
    const float CardH = Px(150) + RayShotStyle::NumSettings * RowH + Px(110);
    const float CardX = (Canvas->ClipX - CardW) * 0.5f;
    const float CardY = (Canvas->ClipY - CardH) * 0.5f + Px(30) * (1.0f - Intro);

    DrawPanel(CardX, CardY, CardW, CardH, Intro);
    DrawBox(CardX, CardY, CardW, Px(5), RayShotStyle::Cyan);
    DrawGlowLabel(TEXT("SETTINGS"), CardX + Px(48), CardY + Px(36), RayShotStyle::TextWhite, RayShotStyle::Cyan, 50);

    // ---------- rows ----------
    const float RowX = CardX + Px(32);
    const float RowW = CardW - Px(64);

    for (int32 i = 0; i < RayShotStyle::NumSettings; i++)
    {
        float RowY = CardY + Px(130) + i * RowH;

        FString Label;
        FString Value;
        float Fraction = -1.0f; // -1 = no slider

        switch (i)
        {
        case 0:
            Label = TEXT("Sensitivity");
            Value = FString::Printf(TEXT("%.1f"), Save->Sensitivity);
            Fraction = (Save->Sensitivity - 0.1f) / 9.9f;
            break;
        case 1:
            Label = TEXT("Field of view");
            Value = FString::Printf(TEXT("%.0f"), Save->FieldOfView);
            Fraction = (Save->FieldOfView - 60.0f) / 60.0f;
            break;
        case 2:
            Label = TEXT("Volume");
            Value = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Save->Volume * 100.0f));
            Fraction = Save->Volume;
            break;
        default:
            Label = TEXT("Crosshair color");
            Value = RayShotStyle::GetCrosshairColorName(Save->CrosshairColor);
            break;
        }

        // the row itself is a button (click = increase, right-click = decrease)
        DrawButton(GameMode, i, RowX, RowY, RowW, RowH - Px(14), Label, TEXT("<   ") + Value + TEXT("   >"),
                   RayShotStyle::Cyan, Intro);

        if (Fraction >= 0.0f)
        {
            DrawSlider(RowX + Px(28), RowY + RowH - Px(28), RowW - Px(56), Fraction, RayShotStyle::Cyan, Intro);
        }
        else
        {
            // crosshair preview swatch
            FLinearColor Swatch = RayShotStyle::GetCrosshairColor(Save->CrosshairColor);
            float SwatchX = RowX + RowW * 0.5f;
            float SwatchY = RowY + (RowH - Px(14)) * 0.5f;
            DrawBox(SwatchX - Px(10), SwatchY - Px(1), Px(20), Px(2), Swatch);
            DrawBox(SwatchX - Px(1), SwatchY - Px(10), Px(2), Px(20), Swatch);
        }
    }

    // ---------- back button ----------
    float BackY = CardY + Px(130) + RayShotStyle::NumSettings * RowH + Px(12);
    DrawButton(GameMode, RayShotStyle::NumSettings, RowX, BackY, Px(220), Px(60), TEXT("BACK"), TEXT(""),
               RayShotStyle::TextDim, Intro);

    // ---------- hints ----------
    DrawLabel(TEXT("Click / D = increase        Right-click / A = decrease        TAB = back"),
              Canvas->ClipX * 0.5f, CardY + CardH + Px(28), RayShotStyle::TextDim, 17, 0.5f);
}

// =====================================================================
//                               RESULTS
// =====================================================================

void ARayShotHUD::DrawResults(ARayShotGameMode* GameMode, ATargetManager* Manager)
{
    const ERayShotMode Mode = GameMode->CurrentMode;
    const FLinearColor ModeColor = RayShotModes::GetColor(Mode);
    const float Age = ScreenAge();
    const float Intro = HUDEaseOut(Age / 0.4f);

    DrawGradient(0, 0, Canvas->ClipX, Canvas->ClipY,
                 FLinearColor(0.008f, 0.01f, 0.02f, 0.9f), FLinearColor(0.008f, 0.01f, 0.02f, 0.65f), false);

    // ---------- card ----------
    const float CardW = Px(760);
    const float CardH = Px(680);
    const float CardX = (Canvas->ClipX - CardW) * 0.5f;
    const float CardY = (Canvas->ClipY - CardH) * 0.5f + Px(40) * (1.0f - Intro);
    const float TextX = CardX + Px(48);

    DrawPanel(CardX, CardY, CardW, CardH, Intro);
    DrawBox(CardX, CardY, CardW, Px(6), ModeColor);

    DrawLabel(RayShotModes::GetName(Mode).ToUpper() + TEXT("   //   RESULTS"), TextX, CardY + Px(38), ModeColor, 22);

    // ---------- big score (counts up during the intro) ----------
    float ShownScore = GameMode->LastScore * HUDEaseOut(Age / 0.8f);
    FString Number = RayShotModes::FormatScoreNumber(Mode, ShownScore);
    FVector2D NumberSize = MeasureText(Number, 120, true);
    DrawGlowLabel(Number, TextX, CardY + Px(80), RayShotStyle::TextWhite, ModeColor, 120, 0.0f, true);
    DrawLabel(RayShotModes::GetScoreUnit(Mode), TextX + NumberSize.X + Px(18), CardY + Px(80) + NumberSize.Y - Px(58),
              RayShotStyle::TextDim, 28);

    // ---------- personal best ----------
    const float BestY = CardY + Px(250);
    if (GameMode->bNewBest)
    {
        float Pulse = 0.8f + 0.2f * FMath::Sin(NowTime * 6.0f);
        DrawBox(TextX, BestY, Px(340), Px(48), RayShotStyle::WithAlpha(RayShotStyle::Gold, Pulse));
        DrawLabel(TEXT("NEW PERSONAL BEST!"), TextX + Px(170), BestY + Px(11),
                  FLinearColor(0.06f, 0.04f, 0.0f), 22, 0.5f, false, false);
    }
    else
    {
        DrawLabel(TEXT("PERSONAL BEST"), TextX, BestY + Px(4), RayShotStyle::TextDim, 16);
        DrawLabel(GameMode->GetBestScoreText(Mode), TextX + Px(180), BestY, RayShotStyle::Gold, 26, 0.0f, true);
    }

    // ---------- stats rows ----------
    if (Manager)
    {
        TArray<FStatLine> Lines = Manager->GetResultLines();
        const float RowH = Px(48);
        for (int32 i = 0; i < Lines.Num(); i++)
        {
            float RowY = CardY + Px(330) + i * RowH;
            DrawLabel(Lines[i].Label, TextX, RowY + Px(4), RayShotStyle::TextDim, 22);
            DrawLabel(Lines[i].Value, CardX + CardW - Px(48), RowY, RayShotStyle::TextWhite, 26, 1.0f, true);
            DrawBox(TextX, RowY + RowH - Px(8), CardW - Px(96), Px(1), RayShotStyle::PanelBorder);
        }
    }

    // ---------- buttons side by side ----------
    const float ButtonY = CardY + CardH - Px(100);
    const float ButtonW = (CardW - Px(96) - Px(16)) * 0.5f;
    DrawButton(GameMode, 0, TextX, ButtonY, ButtonW, Px(62), TEXT("PLAY AGAIN"), TEXT(""), ModeColor, Intro);
    DrawButton(GameMode, 1, TextX + ButtonW + Px(16), ButtonY, ButtonW, Px(62), TEXT("MAIN MENU"), TEXT(""),
               RayShotStyle::TextDim, Intro);

    DrawLabel(TEXT("ENTER = select        R / click Play Again        TAB = menu"),
              Canvas->ClipX * 0.5f, CardY + CardH + Px(28), RayShotStyle::TextDim, 17, 0.5f);
}

// =====================================================================
//                               WIDGETS
// =====================================================================

bool ARayShotHUD::DrawButton(ARayShotGameMode* GameMode, int32 Index, float X, float Y, float W, float H,
                             const FString& Label, const FString& RightText, FLinearColor Accent, float Alpha)
{
    // make sure we have an animation value for this button
    while (ButtonAnim.Num() <= Index)
    {
        ButtonAnim.Add(0.0f);
    }

    // ---------- mouse ----------
    bool bHovered = bHasMouse && MouseX >= X && MouseX <= X + W && MouseY >= Y && MouseY <= Y + H;
    if (bHovered)
    {
        GameMode->HoveredItem = Index;
        if (bMouseMoved)
        {
            GameMode->SelectedItem = Index; // moving the mouse over a button selects it
        }
    }

    // ---------- smooth highlight animation ----------
    bool bSelected = (GameMode->SelectedItem == Index);
    float& Anim = ButtonAnim[Index];
    Anim = FMath::FInterpTo(Anim, bSelected ? 1.0f : 0.0f, FrameDeltaTime, 14.0f);

    // ---------- background ----------
    FLinearColor SelectedFill(Accent.R * 0.22f, Accent.G * 0.22f, Accent.B * 0.22f, 0.9f);
    FLinearColor Fill = FMath::Lerp(RayShotStyle::ButtonFill, SelectedFill, Anim);
    Fill.A *= Alpha;
    DrawBox(X, Y, W, H, Fill);

    // accent bar on the left (grows when selected)
    DrawBox(X, Y, Px(3) + Px(5) * Anim, H, RayShotStyle::WithAlpha(Accent, (0.35f + 0.65f * Anim) * Alpha));

    // thin outline when selected
    if (Anim > 0.01f)
    {
        FLinearColor Border = RayShotStyle::WithAlpha(Accent, 0.45f * Anim * Alpha);
        DrawBox(X, Y, W, Px(1), Border);
        DrawBox(X, Y + H - Px(1), W, Px(1), Border);
        DrawBox(X + W - Px(1), Y, Px(1), H, Border);
    }

    // ---------- text ----------
    FVector2D LabelSize = MeasureText(Label, 24);
    FLinearColor LabelColor = FMath::Lerp(FLinearColor(0.85f, 0.88f, 0.94f), RayShotStyle::TextWhite, Anim);
    LabelColor.A = Alpha;
    DrawLabel(Label, X + Px(26) + Px(8) * Anim, Y + (H - LabelSize.Y) * 0.5f, LabelColor, 24);

    if (!RightText.IsEmpty())
    {
        FVector2D RightSize = MeasureText(RightText, 20, true);
        FLinearColor RightColor = RightText.StartsWith(TEXT("BEST")) ? RayShotStyle::Gold : RayShotStyle::TextDim;
        RightColor.A = Alpha * (0.75f + 0.25f * Anim);
        DrawLabel(RightText, X + W - Px(22), Y + (H - RightSize.Y) * 0.5f, RightColor, 20, 1.0f, true);
    }

    return bHovered;
}

void ARayShotHUD::DrawSlider(float X, float Y, float W, float Fraction, FLinearColor Accent, float Alpha)
{
    Fraction = FMath::Clamp(Fraction, 0.0f, 1.0f);

    // track, filled part, knob
    DrawBox(X, Y, W, Px(4), FLinearColor(1.0f, 1.0f, 1.0f, 0.1f * Alpha));
    DrawBox(X, Y, W * Fraction, Px(4), RayShotStyle::WithAlpha(Accent, Alpha));
    DrawBox(X + W * Fraction - Px(5), Y - Px(5), Px(10), Px(14), RayShotStyle::WithAlpha(RayShotStyle::TextWhite, Alpha));
}

// =====================================================================
//                           DRAWING HELPERS
// =====================================================================

void ARayShotHUD::DrawBox(float X, float Y, float W, float H, FLinearColor Color)
{
    if (W <= 0.0f || H <= 0.0f || Color.A <= 0.0f)
    {
        return;
    }
    DrawRect(Color, X, Y, W, H);
}

void ARayShotHUD::DrawPanel(float X, float Y, float W, float H, float Alpha)
{
    // dark see-through background
    DrawBox(X, Y, W, H, RayShotStyle::WithAlpha(RayShotStyle::PanelFill, RayShotStyle::PanelFill.A * Alpha));

    // thin light border
    FLinearColor Border = RayShotStyle::WithAlpha(RayShotStyle::PanelBorder, RayShotStyle::PanelBorder.A * Alpha);
    DrawBox(X, Y, W, Px(1), Border);
    DrawBox(X, Y + H - Px(1), W, Px(1), Border);
    DrawBox(X, Y, Px(1), H, Border);
    DrawBox(X + W - Px(1), Y, Px(1), H, Border);
}

void ARayShotHUD::DrawGradient(float X, float Y, float W, float H, FLinearColor From, FLinearColor To, bool bHorizontal)
{
    // a gradient made of many thin strips
    const int32 Steps = 48;
    for (int32 i = 0; i < Steps; i++)
    {
        float T = (i + 0.5f) / Steps;
        FLinearColor Color = FMath::Lerp(From, To, T);

        if (bHorizontal)
        {
            float StripX = X + W * i / Steps;
            float StripW = W * (i + 1) / Steps - W * i / Steps;
            DrawBox(StripX, Y, StripW, H, Color);
        }
        else
        {
            float StripY = Y + H * i / Steps;
            float StripH = H * (i + 1) / Steps - H * i / Steps;
            DrawBox(X, StripY, W, StripH, Color);
        }
    }
}

UFont* ARayShotHUD::GetFont(bool bMono) const
{
    if (bMono && MonoFont)
    {
        return MonoFont;
    }
    return GEngine ? GEngine->GetLargeFont() : nullptr;
}

UFont* ARayShotHUD::GetSizedFont(bool bMono, float PixelSize, float& OutScale)
{
    // WHY: Unreal's HUD draws a font at its built-in "LegacyFontSize" and then STRETCHES
    // the letters to the size we ask for. Stretching 24px letters to 120px makes them blurry.
    // FIX: make a copy of the font whose built-in size IS the size we need, and draw it
    // at scale 1. The letters are then rendered sharp at the real size.

    UFont* BaseFont = GetFont(bMono);
    float TargetSize = PixelSize * UIScale; // real size on this screen
    if (BaseFont == nullptr)
    {
        OutScale = 1.0f;
        return nullptr;
    }

    // round to "buckets" so animated text (countdown) doesn't create a new font every frame
    int32 Step = (TargetSize > 48.0f) ? 4 : 1;
    int32 Bucket = FMath::Clamp(FMath::RoundToInt(TargetSize / Step) * Step, 4, 1000);

    // one cached copy per size (and per regular / mono font)
    int32 Key = Bucket * 2 + (bMono ? 1 : 0);
    UFont* SizedFont = nullptr;
    if (TObjectPtr<UFont>* Found = SizedFonts.Find(Key))
    {
        SizedFont = *Found;
    }
    else
    {
        SizedFont = DuplicateObject<UFont>(BaseFont, this);
        SizedFont->LegacyFontSize = Bucket;
        SizedFonts.Add(Key, SizedFont);
    }

    // tiny leftover scale (at most a few %) from the rounding - not visible
    OutScale = TargetSize / Bucket;
    return SizedFont;
}

FVector2D ARayShotHUD::MeasureText(const FString& Text, float PixelSize, bool bMono)
{
    float Scale = 1.0f;
    UFont* Font = GetSizedFont(bMono, PixelSize, Scale);
    float Width = 0.0f;
    float Height = 0.0f;
    GetTextSize(Text, Width, Height, Font, Scale);
    return FVector2D(Width, Height);
}

void ARayShotHUD::DrawLabel(const FString& Text, float X, float Y, FLinearColor Color, float PixelSize,
                            float AlignX, bool bMono, bool bShadow)
{
    if (Text.IsEmpty() || Color.A <= 0.0f)
    {
        return;
    }

    float Scale = 1.0f;
    UFont* Font = GetSizedFont(bMono, PixelSize, Scale);

    float Width = 0.0f;
    float Height = 0.0f;
    GetTextSize(Text, Width, Height, Font, Scale);

    // snap to whole pixels - text between pixels looks soft
    float DrawX = FMath::RoundToFloat(X - Width * AlignX);
    Y = FMath::RoundToFloat(Y);

    // soft drop shadow for readability
    if (bShadow)
    {
        DrawText(Text, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f * Color.A), DrawX + Px(2), Y + Px(2), Font, Scale);
    }
    DrawText(Text, Color, DrawX, Y, Font, Scale);
}

void ARayShotHUD::DrawGlowLabel(const FString& Text, float X, float Y, FLinearColor Color, FLinearColor GlowColor,
                                float PixelSize, float AlignX, bool bMono)
{
    if (Text.IsEmpty() || Color.A <= 0.0f)
    {
        return;
    }

    float Scale = 1.0f;
    UFont* Font = GetSizedFont(bMono, PixelSize, Scale);

    float Width = 0.0f;
    float Height = 0.0f;
    GetTextSize(Text, Width, Height, Font, Scale);

    // snap to whole pixels - text between pixels looks soft
    float DrawX = FMath::RoundToFloat(X - Width * AlignX);
    Y = FMath::RoundToFloat(Y);

    // glow: the text drawn faintly in a circle around itself, in two tight rings
    FLinearColor Glow = RayShotStyle::WithAlpha(GlowColor, 0.09f * Color.A);
    for (int32 Ring = 1; Ring <= 2; Ring++)
    {
        float Radius = Px(2.0f * Ring);
        for (int32 i = 0; i < 8; i++)
        {
            float Angle = i * PI / 4.0f;
            DrawText(Text, Glow, DrawX + FMath::Cos(Angle) * Radius, Y + FMath::Sin(Angle) * Radius, Font, Scale);
        }
    }

    DrawText(Text, Color, DrawX, Y, Font, Scale);
}
