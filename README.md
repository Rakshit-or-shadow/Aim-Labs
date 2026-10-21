# GameEngine - Tactical Aim Trainer (UE 5.8, C++)

A first-person aim trainer built on the Unreal Engine 5.8 First Person template.
All new code is C++ with the `GE` prefix. Hit detection never uses line traces:
every shot is a ray tested against sphere hitboxes with `RayTraceMath::DoesIntersect`.

## Features

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

## Controls

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

Menu: choose mode, difficulty, weapon, START/STOP, view stats, change sensitivity (+/-) and mouse DPI,
view Flick results and the AI Coach panel after a session.

## How to build

Close the Unreal Editor first, then run (UE 5.8 installed in the default location):

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GameEngineEditor Win64 Development -Project="C:/Users/raksh/OneDrive/Documents/rayshot/GameEngine/GameEngine.uproject" -WaitMutex
```

Then open `GameEngine.uproject`. The startup map is `/Game/DemoTemplate/_Core/Lvl_IntroRoom`.

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
| GEHUD | Crosshair, ammo, hit marker, scope, popups, menu, flick and coach panels |

## Notes

- No line traces anywhere: shots are not blocked by walls or cover (a hidden peeking bot is simply unshootable).
- Hit math lives in `RayTraceMath`; bots and dummies expose sphere hitboxes through `IGEHittable`.
- No new .umap could be created without the editor, so the Training Center is spawned by C++ inside
  `Lvl_IntroRoom`, 200000 cm along +Y from the PlayerStart. Press T to go there (a cyan portal is near the start).
- cm/360 assumes a mouse DPI (default 800, adjustable in the menu) and a guessed degrees-per-count, so it is approximate.
- Original names only; the range is called "Training Center".
