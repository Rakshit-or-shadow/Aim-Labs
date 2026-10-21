# FIX3 report: RayShot start level + aim target region

## Task 1: open straight into RayShot
- **Route taken: (a) new headless level.** `Content/RayShot/Lvl_RayShot.umap` was created with `UnrealEditor-Cmd -run=pythonscript`: one PlayerStart, a plain 100x100 m floor, a directional light, GameMode override `BP_FirstPersonGameMode`. No template wall, text or props.
- `Config/DefaultEngine.ini`: `EditorStartupMap` and `GameDefaultMap` now `/Game/RayShot/Lvl_RayShot`.
- `AGEGameMode::BeginPlay` only built the aim arena + Training Center when the map name contained `Lvl_IntroRoom`. It now builds them in any level with a PlayerStart; the Phase 2 test dummies stay intro-room only. The HUD flow already starts on the Main Menu when the aim manager exists (`AGEHUD::StartFlow` -> `GoToMainMenu`, cursor + UI input, gameplay input blocked, M/Tab return), so nothing changed there.
- Training Center and the aim arena are reached from the menu as before. `Lvl_IntroRoom` is untouched (its GameMode change was committed first, not reverted).

## Task 2: aim targets
### Root causes
1. **Region did not match the frame.** The manager was centered at the player's eye height (about Z 160, read from the camera) while the glowing frame was hard-coded around absolute Z 320 (30..610). The Gridshot grid (5x5, 120 cm, +-240 plus radius 30) and the random area were centered on the eye, so the lowest row sat on or under the floor/frame bottom (cut off, or invisible: the "empty box with 12 hits"), and the grid was nearly as tall as the frame.
2. **Numbers in several places.** Frame size (Arena), AreaHalfWidth/Height, GridSpacing, TargetDistance (Manager) were independent constants. Nothing checked radius, wall depth or camera view.
3. **No overlap rule and no retry.** Grid used a 1 cm "taken" test; Microshot/Reflex had none. A failed grid search fell back to the center point, on top of another target.
4. **ADS** zoom changed the FOV the region is seen at.

### Changes
- `AGEArena::GetPlayRegion()` (`FGEPlayRegion`) is the one source of truth: the INNER size, center and plane of the frame, and the wall X. The frame is now built from the same constants (inner 880x480, centered Z 260, so it sits around eye level instead of 160 cm above it).
- `AGETargetManager::RefreshBounds()` builds `FGEAimBounds` from it every round / on entering the arena: the frame is projected from the real eye onto the target plane (`WallInset` = 120 cm in front of the wall), then shrunk if needed so it fits the camera at the lowest FOV (60) at the real aspect ratio (conservative: horizontal FOV kept).
- Every sphere centre: Y/Z within region minus radius minus 8 cm margin; X = min(plane, wall - radius - margin). Gridshot grid cells, Microshot, Reflex, Tracking (bounce + clamp with the same bounds) and the menu showcase targets all use it. Old `GridSpacing/AreaHalf*/TargetDistance` properties removed.
- No overlap: `FindSpawnPoint` rejects spots within r1+r2+4 cm of another visible target or on the spot just left; 50 random tries, then a full grid scan, else the target waits and `RetryPlacements()` tries again every tick (also during the countdown). Reflex retries the same way.
- Gridshot keeps 3, Microshot 1, Tracking 1, Reflex 1. Hit detection is still `RayTraceMath::DoesIntersect`.
- ADS is ignored while in the arena (`AGECharacter::AimPressed`).
- Spheres still pop in (scale 0 -> 1 on `SetActive(true)`); they are now always moved before being shown, never while visible.
- Console: `ge.aim.debug` (white = frame, green = target plane, yellow = per-target centre bounds), `ge.aim.selftest`. Headless switch `-GEAimSelfTest` runs the test one second after BeginPlay and exits.

### Selftest result (headless)
`UnrealEditor-Cmd GameEngine.uproject -game -nullrhi -unattended -nosound -GEAimSelfTest -log` ->
```
Gridshot PASS / Microshot PASS / Tracking PASS / Reflex PASS (2000 iterations x 16:9, 21:9, 32:9 each)
GEAimSelfTest: ALL PASS
```
Checks per iteration: inside shrunk bounds, inside the frame as seen from the eye (independent perspective check), whole sphere in front of the wall, no overlap, correct alive count.

## Task 3
- Pop-in: already a 0.18 s scale-up from ~0; verified no path shows a target before it is placed.
- HUD behind menus: already fine, `DrawGameplayHUD`/crosshair are only called for free roam / Training Center / Playing; menu and Results screens draw only their own UI. No change.

## How to test by hand
1. Open the project (it opens `Lvl_RayShot`), press Play: Main Menu appears, cursor visible, no tutorial wall.
2. Pick a mode. Run `ge.aim.debug` to see the frame and bounds; spheres must always lie inside the white rectangle, fully in front of the wall.
3. Play Gridshot for a minute at FOV 60 and FOV 120, and in a very wide window; the box should never be empty.
4. `ge.aim.selftest` from the menu (no round running).

## Not verified visually
Everything was checked headless (build, selftest, level loads with `BP_FirstPersonGameMode`). I did not look at the game: the menu on start, the new frame proportions, the eye-height offset on screen, and pop-in need a quick look in the editor. No retained screenshots were taken (`production/qa/evidence/`).

## Decisions made unattended
- Reshaped the frame (880x480 centered Z 260, was 920x580 centered Z 320) so it sits at eye level and fits at FOV 60.
- Disabled ADS in the arena rather than tracking FOV changes.
- Left `Lvl_IntroRoom` as is (still loads, still works) instead of deleting it.
