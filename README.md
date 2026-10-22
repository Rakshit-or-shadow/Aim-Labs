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

![Main menu with the neon arena behind it](docs/screenshots/hero/01-main-menu.png)
*Main menu with the neon arena behind it*

![The arena from a wide angle](docs/screenshots/hero/02-arena.png)
*The arena from a wide angle*

![Gridshot in the Cyan theme](docs/screenshots/hero/03-gridshot-cyan.png)
*Gridshot in the Cyan theme*

![Tracking: the target turns green while the crosshair is on it](docs/screenshots/hero/04-tracking.png)
*Tracking: the target turns green while the crosshair is on it*

![Results with rank and personal best](docs/screenshots/hero/05-results.png)
*Results with rank and personal best*

![Eagle Eye scope view in the Training Center](docs/screenshots/hero/06-eagle-eye-scope.png)
*Eagle Eye scope view in the Training Center*

![Microshot: one tiny target](docs/screenshots/hero/07-microshot.png)
*Microshot: one tiny target*

![Results with the offline AI Coach](docs/screenshots/hero/08-ai-coach.png)
*Results with the offline AI Coach*

<details>
<summary>Full gallery (68 screenshots)</summary>

![01_main_menu_red](docs/screenshots/01_main_menu_red.jpg)
![02_main_menu_selected_training](docs/screenshots/02_main_menu_selected_training.jpg)
![03_main_menu_theme_cyan](docs/screenshots/03_main_menu_theme_cyan.jpg)
![04_main_menu_theme_yellow](docs/screenshots/04_main_menu_theme_yellow.jpg)
![05_main_menu_theme_green](docs/screenshots/05_main_menu_theme_green.jpg)
![06_arena_wide_view](docs/screenshots/06_arena_wide_view.jpg)
![07_mode_select_gridshot](docs/screenshots/07_mode_select_gridshot.jpg)
![08_mode_select_microshot](docs/screenshots/08_mode_select_microshot.jpg)
![09_mode_select_tracking](docs/screenshots/09_mode_select_tracking.jpg)
![10_mode_select_reflex](docs/screenshots/10_mode_select_reflex.jpg)
![11_settings_mouse_sensitivity](docs/screenshots/11_settings_mouse_sensitivity.jpg)
![12_settings_video_fov](docs/screenshots/12_settings_video_fov.jpg)
![13_settings_audio_volume](docs/screenshots/13_settings_audio_volume.jpg)
![14_settings_crosshair_color](docs/screenshots/14_settings_crosshair_color.jpg)
![15_settings_interface_theme](docs/screenshots/15_settings_interface_theme.jpg)
![16_settings_interface_scanlines](docs/screenshots/16_settings_interface_scanlines.jpg)
![17_settings_interface_glitch_off](docs/screenshots/17_settings_interface_glitch_off.jpg)
![18_gridshot_countdown](docs/screenshots/18_gridshot_countdown.jpg)
![19_gridshot_playing](docs/screenshots/19_gridshot_playing.jpg)
![20_gridshot_pause_menu](docs/screenshots/20_gridshot_pause_menu.jpg)
![21_gridshot_hud_theme_cyan](docs/screenshots/21_gridshot_hud_theme_cyan.jpg)
![22_gridshot_hud_theme_yellow](docs/screenshots/22_gridshot_hud_theme_yellow.jpg)
![23_gridshot_hud_theme_green](docs/screenshots/23_gridshot_hud_theme_green.jpg)
![24_gridshot_results_new_best](docs/screenshots/24_gridshot_results_new_best.jpg)
![25_microshot_playing](docs/screenshots/25_microshot_playing.jpg)
![26_microshot_results](docs/screenshots/26_microshot_results.jpg)
![27_tracking_playing](docs/screenshots/27_tracking_playing.jpg)
![28_tracking_results](docs/screenshots/28_tracking_results.jpg)
![29_reflex_waiting](docs/screenshots/29_reflex_waiting.jpg)
![30_reflex_playing](docs/screenshots/30_reflex_playing.jpg)
![31_reflex_results](docs/screenshots/31_reflex_results.jpg)
![32_gridshot_results_no_new_best](docs/screenshots/32_gridshot_results_no_new_best.jpg)
![33_mode_select_with_personal_bests](docs/screenshots/33_mode_select_with_personal_bests.jpg)
![34_free_roam_standing](docs/screenshots/34_free_roam_standing.jpg)
![35_pause_menu_free_roam](docs/screenshots/35_pause_menu_free_roam.jpg)
![36_training_center_menu](docs/screenshots/36_training_center_menu.jpg)
![37_training_center_menu_strafing_hard_scatter](docs/screenshots/37_training_center_menu_strafing_hard_scatter.jpg)
![38_movement_standing](docs/screenshots/38_movement_standing.jpg)
![39_movement_walking_speed_readout](docs/screenshots/39_movement_walking_speed_readout.jpg)
![40_movement_running](docs/screenshots/40_movement_running.jpg)
![41_movement_crouching](docs/screenshots/41_movement_crouching.jpg)
![42_movement_jump_midair](docs/screenshots/42_movement_jump_midair.jpg)
![43_weapon_sidekick_firing](docs/screenshots/43_weapon_sidekick_firing.jpg)
![44_weapon_sidekick_ads](docs/screenshots/44_weapon_sidekick_ads.jpg)
![45_weapon_sidekick_reload](docs/screenshots/45_weapon_sidekick_reload.jpg)
![46_weapon_rattler_firing](docs/screenshots/46_weapon_rattler_firing.jpg)
![47_weapon_rattler_ads](docs/screenshots/47_weapon_rattler_ads.jpg)
![48_weapon_rattler_reload](docs/screenshots/48_weapon_rattler_reload.jpg)
![49_weapon_longarm_firing](docs/screenshots/49_weapon_longarm_firing.jpg)
![50_weapon_longarm_ads](docs/screenshots/50_weapon_longarm_ads.jpg)
![51_weapon_longarm_reload](docs/screenshots/51_weapon_longarm_reload.jpg)
![52_bot_killed_headshot](docs/screenshots/52_bot_killed_headshot.jpg)
![53_weapon_eagle_eye_firing](docs/screenshots/53_weapon_eagle_eye_firing.jpg)
![54_weapon_eagle_eye_ads](docs/screenshots/54_weapon_eagle_eye_ads.jpg)
![55_weapon_eagle_eye_reload](docs/screenshots/55_weapon_eagle_eye_reload.jpg)
![56_weapon_scatter_firing](docs/screenshots/56_weapon_scatter_firing.jpg)
![57_weapon_scatter_ads](docs/screenshots/57_weapon_scatter_ads.jpg)
![58_weapon_scatter_reload](docs/screenshots/58_weapon_scatter_reload.jpg)
![59_training_menu_session_stats](docs/screenshots/59_training_menu_session_stats.jpg)
![60_training_results_and_coach](docs/screenshots/60_training_results_and_coach.jpg)
![61_training_menu_coach_panel](docs/screenshots/61_training_menu_coach_panel.jpg)
![62_bots_strafing_hard](docs/screenshots/62_bots_strafing_hard.jpg)
![63_bots_peek_hard](docs/screenshots/63_bots_peek_hard.jpg)
![64_flick_optimizer_start](docs/screenshots/64_flick_optimizer_start.jpg)
![65_flick_in_progress](docs/screenshots/65_flick_in_progress.jpg)
![66_flick_results_and_coach](docs/screenshots/66_flick_results_and_coach.jpg)
![67_flick_analysis_table](docs/screenshots/67_flick_analysis_table.jpg)
![68_main_menu_final](docs/screenshots/68_main_menu_final.jpg)

</details>

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
