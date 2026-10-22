# RAYSHOT / GameEngine - Tactical Aim Trainer (UE 5.8, C++)

A first-person aim trainer built on the Unreal Engine 5.8 First Person template.
All new code is C++ with the `GE` prefix. Hit detection never uses line traces:
every shot is a ray tested against sphere hitboxes with `RayTraceMath::DoesIntersect`
(the ray tracer math comes from the author's own C ray tracer).

Status: v1.0 packaged for Windows. Visuals were not verified headlessly (see docs/RELEASE_REPORT.md).

## Download

Windows build (v1.0): https://github.com/Rakshit-or-shadow/Aim-Labs/releases

Unzip `RayShot-v1.0-Windows.zip` and run `RayShot.exe`. The build is unsigned, so Windows SmartScreen may
warn: click "More info" then "Run anyway". Requires Windows 10/11 64-bit and a DirectX 12 GPU.
Release notes: `docs/release/RELEASE_NOTES_v1.0.md`.

## Screenshots

![Main menu](docs/screenshots/main-menu.png)
![Gridshot](docs/screenshots/mode-gridshot.png)
![Training Center](docs/screenshots/training-center.png)
![Flick Optimizer](docs/screenshots/flick-optimizer.png)
![Settings](docs/screenshots/settings.png)

(Placeholders: the images are added to `docs/screenshots/` after capture.)

## How to play

Pick one of the four aim modes in the menu, or press T to go to the Training Center. WASD move, mouse look,
left mouse fires, right mouse aims, R reloads, 1-5 select a weapon. The full controls tables are in the Controls section below.

## How it works

Every shot is a ray tested against sphere hitboxes. Unreal line traces are not used. With ray start `P`,
normalized direction `d`, sphere center `S` and radius `r`, let `V = P - S`. A point on the ray is `P + t*d`, and
it is on the sphere when `|V + t*d|^2 = r^2`, which gives the quadratic `A*t^2 + B*t + C = 0` with

- `A = d . d`
- `B = 2 (d . V)`
- `C = V . V - r^2`

The discriminant `B^2 - 4AC` decides the result: negative means a miss. Otherwise the roots
`t = (-B -/+ sqrt(disc)) / 2A` are tried smaller first, and the first one that is >= 0 is the hit distance.
The code is in `Source/GameEngine/RayTraceMath.h` (ported from the author's C ray tracer), and aim targets, bots and
dummies all use it.

## Build from source

Requirements: Unreal Engine 5.8 and the Visual Studio C++ workload ("Game development with C++"). Close the Unreal Editor, then:

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GameEngineEditor Win64 Development -Project="C:/Users/raksh/OneDrive/Documents/rayshot/GameEngine/GameEngine.uproject" -WaitMutex
```

Adjust the paths to your install, then open `GameEngine.uproject`. Packaging: see `docs/RELEASE_REPORT.md`.

## Features

- 4 aim modes ported from the old RayShot project: Gridshot (60 s, 3 targets on a 5x5 grid), Microshot (60 s, one tiny target),
  Tracking (45 s, keep the crosshair on a moving target, no clicking) and Reflex (10 targets, reaction times).
  3-2-1-GO countdown, score popups, hit bursts, personal bests, S/A/B/C/D rank on the results screen.
- Cyberpunk UI drawn with the HUD canvas (GEUIKit): chamfered panels, nav items, diamond tiles, sliders, toggles,
  scanline and glitch overlays, 4 colour themes (Red, Cyan, Yellow, Green, switchable in Settings).
- Settings (saved in `UGESaveGame`): sensitivity, field of view, volume, crosshair colour, UI theme, scanlines, screen glitch.

- Tactical movement: fast run, walk, crouch, low air control, snappy counter-strafing.
- 5 original weapons: Sidekick (sidearm), Rattler (SMG), Longarm (rifle), Eagle Eye (scoped marksman), Scatter (shotgun).
  Headshot/body/leg multipliers, magazines and reload, spread that grows while spraying, recoil patterns,
  movement and jump inaccuracy, right-click zoom/ADS.
- Procedural viewmodel, muzzle flash, tracer lines and generated sounds (no imported assets).
- Dummies in Lvl_IntroRoom and the Training Center (hub, shooting lane with 10/20/30/40 m markers, cover).
- Bots with sphere hitboxes (head, chest x2, legs x2). Modes: Static, Strafing, Peek, Flick.
  Difficulties: Easy, Medium, Hard.
- Session stats: kills, headshot %, accuracy, first-shot accuracy, average time-to-kill.
- Flick Optimizer: per-direction (8) accuracy and time, overshoot/undershoot, corrections, sensitivity suggestion
  with cm/360, saved session history (UGESaveGame).
- Offline AI Coach: 3 specific tips and trends across sessions after each session (rule based, no internet).

## Screens

Main Menu, Mode Select (4 aim modes with bests), Settings, Playing (aim HUD), Results, Pause, Training Center menu,
Flick Optimizer results, AI Coach panel. Debug: console `ge.ui.gallery` draws every UI component;
`ge.aim.start <0-3 or name>` and `ge.aim.stop` start/leave an aim mode.

## Controls

Gameplay (Free Roam, Training Center):

| Input | Action |
|---|---|
| W A S D | Move |
| Mouse | Look |
| Space | Jump |
| Left Shift (hold) | Walk (about 40% speed, accurate shooting) |
| Left Ctrl (hold) | Crouch |
| Left Mouse | Fire (hold for automatic weapons) |
| Right Mouse (hold) | Aim down sights / scope |
| R | Reload |
| 1 - 5 | Select weapon |
| Mouse wheel up / down | Previous / next weapon |
| F3 | Toggle speed readout |
| T | Teleport to the Training Center / back |
| M | Open/close the Training Center menu (mouse is freed; click buttons) |

Menus (Main Menu, Mode Select, Settings, Results, Pause):

| Input | Action |
|---|---|
| W / S or Up / Down | Move selection |
| A / D or Left / Right | Change a setting value |
| Enter or Space | Confirm |
| Left click / right click | Select, confirm, or increase / decrease a setting (sliders are not draggable) |
| Tab (or Esc outside the editor) | Back / pause (Esc is swallowed by Play-In-Editor) |
| R | Restart the round (Playing) or Play Again (Results) |

Training Center menu: choose mode, difficulty, weapon, START/STOP, view stats, change sensitivity (+/-) and mouse DPI,
view Flick results and the AI Coach panel after a session.

## How to build

Close the Unreal Editor first, then run (UE 5.8 installed in the default location):

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GameEngineEditor Win64 Development -Project="C:/Users/raksh/OneDrive/Documents/rayshot/GameEngine/GameEngine.uproject" -WaitMutex
```

Then open `GameEngine.uproject`. The startup map is `/Game/RayShot/Lvl_RayShot`.

## Credits

- Hit detection math derives from the author's own C ray tracer (ray-sphere intersection in `RayTraceMath.h`).
- UI layout and style use a Figma UI kit as design inspiration only; no Figma assets are shipped.
- Fonts: Rajdhani and Share Tech Mono (SIL Open Font License, texts in `ArtSource/Fonts`).
- All weapon, bot and level names are original. Legacy project archived in `Legacy/RayShot`.

## Project structure (Source/GameEngine)

| File | Purpose |
|---|---|
| GameEngine.Build.cs / GameEngine.h/.cpp | Module setup |
| RayTraceMath.h | Ray-sphere intersection used for every shot |
| GEHittable.h | `IGEHittable` interface + hit zone/sphere types |
| GEHitRegistry | World subsystem listing all shootable actors |
| GECharacter | Movement tuning, crouch camera, runtime input actions and bindings |
| GEPlayerController | Applies the stored mouse sensitivity scale to look input |
| GEGameMode | Spawns test dummies and the Training Center in Lvl_IntroRoom |
| GEWeaponData.h | `FGEWeaponData` weapon definition struct |
| GEWeaponComponent | Ammo, spread, recoil, firing, ADS, viewmodel, sounds |
| GETargetDummy | Simple shootable dummy |
| GEBot | Humanoid bot with sphere hitboxes; static/strafe/peek behavior |
| GETrainingCenter | Spawns hub, lane, cover and portal from basic shapes |
| GETrainingTypes.h | Modes, difficulty presets, session stats |
| GETrainingManager | World subsystem: sessions, stats, menu state, teleport, flick loop, sensitivity |
| GEFlickTypes.h / GEFlickTracker | Flick records, path sampling, overshoot analysis, sensitivity suggestion |
| GESaveGame | Saved profile: sensitivity, DPI, session history |
| GECoachTypes.h / GECoachProvider.h / GECoach | Rule-based coach; `IGECoachProvider` has a TODO for a future LLM provider |
| GEHUD / GEHUDScreens | Crosshair, ammo, hit marker, popups, all screens and the screen flow, input/cursor state |
| GEAimTypes.h / GEAimModes.h | Aim mode and screen enums, per-mode rules |
| GETargetManager / GETargetSphere / GEHitBurst / GEArena | Aim mode logic, reusable targets, hit effect, arena room |
| GEStyle.h / GEUIKit | Colour themes, type scale, procedural UI component kit |
| GESounds.h | Procedural UI/aim tones |

## Notes

- No line traces anywhere: shots are not blocked by walls or cover (a hidden peeking bot is simply unshootable).
- Hit math lives in `RayTraceMath`; bots and dummies expose sphere hitboxes through `IGEHittable`.
- No new .umap could be created without the editor, so the Training Center is spawned by C++ inside
  `Lvl_IntroRoom`, 200000 cm along +Y from the PlayerStart. Press T to go there (a cyan portal is near the start).
- cm/360 assumes a mouse DPI (default 800, adjustable in the menu) and a guessed degrees-per-count, so it is approximate.
- Original names only; the range is called "Training Center".
