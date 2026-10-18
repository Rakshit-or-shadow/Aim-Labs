// RayShotTypes.h
// Small shared types: the game modes and the menu screens.
#pragma once

#include "CoreMinimal.h"
#include "RayShotTypes.generated.h"

// the four aim training modes
UENUM()
enum class ERayShotMode : uint8
{
    Gridshot,   // 3 targets on a grid, click as many as you can
    Microshot,  // 1 tiny target, precision clicking
    Tracking,   // 1 moving target, keep your crosshair on it
    Reflex      // 1 target appears after a random delay, click it fast
};

// which screen the player is looking at
UENUM()
enum class ERayShotScreen : uint8
{
    MainMenu,
    Settings,
    Playing,
    Results
};
