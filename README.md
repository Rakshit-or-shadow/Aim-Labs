# RayShot

**An aim trainer built in Unreal Engine 5, where every shot is checked by the ray-sphere intersection code from my own C ray tracer.**

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.8-0E1128?logo=unrealengine)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)
![Platform](https://img.shields.io/badge/platform-Windows-0078D6)
![License](https://img.shields.io/badge/license-MIT-green)

<!-- Add a screenshot or GIF here once you have one, e.g.: ![RayShot gameplay](docs/gameplay.gif) -->

RayShot is a neon-styled aim trainer with four training modes, saved high scores, and a fully custom HUD. It is written entirely in C++ and needs **no art, audio or UI assets**: the arena, targets, effects, menus and sounds are all generated in code at runtime.

---

## The idea: my ray tracer is the hit detection

Before this project I wrote a [ray tracer in C](https://github.com/Rakshit-or-shadow/Ray-Tracer) that renders spheres by shooting a ray through every pixel and solving a quadratic to see which sphere it hits first.

An aim trainer asks the exact same question: *"when the player clicks, which sphere does the ray from the camera hit first?"*

So RayShot **turns off Unreal's built-in collision on every target** and uses my `doesIntersect` function instead, ported from `spheres.c` to Unreal's `FVector`:

```cpp
// RayTraceMath.h  (ported from my C ray tracer)
FVector V = RayPos - SphereCenter;
double A = FVector::DotProduct(RayDir, RayDir);
double B = 2.0 * FVector::DotProduct(RayDir, V);
double C = FVector::DotProduct(V, V) - Radius * Radius;
double Discriminant = B * B - 4.0 * A * C;   // < 0  ->  miss
```

Every click runs the same "find the closest hit" loop that `calculateColor` uses in the C renderer. In **Tracking** mode it runs every single frame to measure how long your crosshair stays on the target.

---

## Features

**Four training modes**

| Mode | What you do | Trains | Score |
|---|---|---|---|
| **Gridshot** | 3 targets on a 5x5 grid, click as many as you can in 60 s | speed + target switching | hits |
| **Microshot** | 1 tiny target in random spots, 60 s | precision | hits |
| **Tracking** | keep your crosshair on a target that strafes unpredictably, 45 s | smooth tracking | % time on target |
| **Reflex** | wait for a target to appear, then flick to it (10 targets) | reaction time | average ms (lower is better) |

**Game feel**
- 3, 2, 1, GO! countdown before every round
- Targets pop in with a bouncy animation and glow in their mode's color
- Hit effects: glowing sparks, a flash of light, floating "+1" / reaction-time popups
- Hit marker, outlined crosshair, timer that pulses red in the last 5 seconds
- Procedurally generated sound effects (synthesized sine-wave beeps, no audio files)

**Presentation**
- Neon arena built from code: dark room, glowing edge strips, floor grid, lights
- Bloom, vignette and color grading; motion blur disabled so flicks stay sharp
- Animated main menu with per-mode info cards, settings screen with sliders, results screen with a count-up score

**Persistence**
- Personal best for every mode, plus settings (sensitivity, field of view, volume, crosshair color), saved to disk with Unreal's `USaveGame`

---

## Controls

| Key | In game | In menus |
|---|---|---|
| Mouse | aim | hover buttons |
| Left click | shoot | select / increase a setting |
| Right click | | decrease a setting |
| W / S, arrow keys | | move selection |
| A / D, arrow keys | | change a setting |
| Enter / Space | | select |
| R | restart round | play again (results screen) |
| Tab | back to menu | back |

> In the Unreal editor, **Esc** stops Play-In-Editor, so use **Tab** to get back to the menu while testing.

---

## Getting started

### Requirements
- **Windows 10/11**
- **Unreal Engine 5.8** (Epic Games Launcher)
- **Visual Studio 2022 or newer** with these workloads:
  - *Game development with C++*
  - *Desktop development with C++*
  - MSVC build tools **v14.38 or newer** and a Windows 10/11 SDK

### Build and run
1. Clone the repo:
   ```bash
   git clone https://github.com/Rakshit-or-shadow/Aim-labs.git
   ```
2. Double-click **`RayShot.uproject`**.
3. When Unreal says *"The following modules are missing or built with a different engine version... Rebuild?"*, click **Yes**. The first build takes a few minutes.
4. Press **Play** in the editor.

**Alternative (Visual Studio):** right-click `RayShot.uproject` and choose **Generate Visual Studio project files**, open `RayShot.sln`, select **Development Editor / Win64**, and press **F5**.

> **Code changes not showing up?** Unreal only offers to rebuild when the compiled module is *missing*. Close the editor and delete the `Binaries/` folder (or build from Visual Studio) to force a fresh build.

---

## Project structure

```
RayShot/
├── RayShot.uproject
├── Config/                     engine, game and input settings
├── Content/                    the level (Untitled.umap)
└── Source/RayShot/
    ├── RayTraceMath.h          ray-sphere intersection, ported from my C ray tracer
    ├── TargetManager.*         round logic for all 4 modes: countdown, spawning, shots, scoring
    ├── TargetSphere.*          a target: mesh, glow light, pop-in animation
    ├── RayShotPawn.*           the player camera, mouse look, keyboard/mouse input
    ├── RayShotGameMode.*       screens, menus, settings, saving best scores
    ├── RayShotHUD.*            everything on screen (menus, crosshair, timer, results)
    ├── Arena.*                 builds the neon room, lights and post-processing
    ├── HitBurst.*              spark + light-flash hit effect
    ├── RayShotSaveGame.h       what gets saved to disk
    ├── RayShotSounds.h         procedural sound effects
    ├── RayShotStyle.h          colors and built-in materials used everywhere
    ├── RayShotModes.h          mode names, colors, descriptions, score formatting
    └── RayShotTypes.h          shared enums (modes, screens)
```

### How a frame works
1. **`RayShotPawn`** reads the mouse, rotates the camera, and on a click passes the camera position and forward direction (the ray) to the target manager.
2. **`TargetManager`** tests that ray against every active target with `RayTraceMath::DoesIntersect`, keeps the closest hit, and updates the score.
3. **`RayShotGameMode`** switches screens and saves a new personal best when a round ends.
4. **`RayShotHUD`** draws the current screen with Unreal's Canvas (rectangles, lines and text only).

### No assets needed
Everything visual uses content that ships with Unreal Engine:
- Shapes: the engine's basic sphere and cube meshes
- Materials: `BasicShapeMaterial` (lit) and `LevelColorationUnlitMaterial` (glowing), recolored at runtime with dynamic material instances
- Fonts: Roboto and DroidSansMono
- Sound: sine waves generated sample by sample into a `USoundWaveProcedural`

Your save file lives at `Saved/SaveGames/RayShotSave.sav`. Delete it to reset all high scores and settings.

---

## Roadmap
- [ ] Ray-traced "shot review" screen rendered by the original C ray tracer
- [ ] More modes (e.g. moving-target flicks, strafing targets in 3D)
- [ ] Stats history and graphs over time
- [ ] Packaged Windows build on the Releases page

---

## License

RayShot's source code is released under the [MIT License](LICENSE.md).
Unreal Engine and its built-in content are property of Epic Games and are covered by the Unreal Engine EULA.

## Author

**Rakshit Singh**, Computing Science, University of Alberta
GitHub: [@Rakshit-or-shadow](https://github.com/Rakshit-or-shadow)
