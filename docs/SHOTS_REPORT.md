# Screenshot tour report

## What was added
- `-ShotTour` mode (`Source/GameEngine/GEShotTour.h/.cpp`, started from `AGEGameMode::Tick`). It drives the real HUD flow and game systems on a script, takes 68 screenshots and exits by itself (hard timeout 470 s; a step that waits more than 40 s is logged as `SHOTTOUR SKIPPED` and skipped). Log: `tour_log.txt` next to the shots.
- Tour autoplayer (only under `-ShotTour`): turns the view toward the nearest target and fires. Aim-mode shots go through `AGETargetManager::HandleFire`, bot shots through the weapon component; every hit is still decided by `RayTraceMath::DoesIntersect`, no line traces. It only fires when its own `DoesIntersect` test passes.
- Save safety: under `-ShotTour` `UGESaveGame::LoadOrCreate` returns a fresh profile (default sensitivity, FOV 103) and `Save()` writes nothing. A fake 3-session history is fed in for the coach, in memory only.
- Other tour-only changes: aim rounds shortened (9-12 s, Reflex 6 targets), the real mouse cursor is ignored by the HUD, shots are rendered offscreen at 1920x1080 via the high-res screenshot path (the OS clamped the window to 1423x889).
- Output folder via `-ShotDir=<folder>` (default `<Saved>/ShotTour`).
- `docs/FEATURE_INVENTORY.md`, `docs/SCREENSHOT_COVERAGE.md`, README (Screenshots with 8 hero images and a collapsed gallery, How to play, Download), `docs/release/ITCH_PAGE.md` (which 5 to upload), `docs/release/SCREENSHOT_CHECKLIST.md`.

## What ran
- Editor build in `-game` mode: 4 iterations. Fixed: wide-arena step left the player outside the room, weapon shots missed muzzle flash/hit marker (now captured right after a hit), the real mouse hovered buttons, strafing/peek/flick view rotation.
- Packaged Shipping build (RunUAT BuildCookRun, pak), 2 packagings (the second after a tour fix). Last run: `RayShot.exe -ShotTour -windowed -ResX=1920 -ResY=1080 -nosound`, 68 shots, no skips, about 190 s, exited by itself.
- Final set = first packaged run, with shots 25-30 (microshot, tracking, reflex) replaced by the second packaged run after fixing a tour autoplayer bug (Tracking was 0% on target; now about 97%) and making the Reflex shot wait for a visible target. All images checked: 1920x1080, none blank or near-flat (Pillow check), and looked at on contact sheets.

## Good shots
Main menu (4 themes), arena wide view, mode select, all 5 settings sections, countdown, Gridshot / Microshot / Tracking / Reflex playing and results, pause menus, Eagle Eye scope, the coach screens, flick results.

## Doubtful / recapture by hand
- Strafing and peek bot shots (62, 63): bot small or at the screen edge.
- Scatter pellet spread (56) and spread crosshair: not clearly visible in stills.
- Flick in progress (65): a target was just hit; no live target in frame.
- Bot death (52): only the headshot hit marker; the dying itself is not clear.
- Live "Kills / Acc / HS" HUD line (small, in the weapon shots) is not clearly legible; the stats are in the menu shots (59).
- The Training Center is very bright / overexposed in the game itself, and the F3 readout and ammo text are small and faint on it.
- Not in the game, so not shown: quit confirmation, score streak, crosshair styles. Saves and console debug cannot be captured.
- Contact sheets were 640 px per image, so small text was not read at full size.

## Zip
`C:\Users\raksh\OneDrive\Documents\rayshot\Builds\RayShot-v1.0-Windows.zip`, 335,794,272 bytes (320.2 MB), 54 entries. It was built from the same packaged build that ran the tour (the launcher is `RayShot.exe`). It includes the `-ShotTour` code, which does nothing without that flag.

## Not done / manual
- Nothing was uploaded, tagged or released. Create the GitHub Release and the itch.io page yourself.
- Screenshots are in `docs/screenshots` (19 MB total, under the 60 MB limit); `Builds`, `Binaries`, `Intermediate`, `Saved` and `DerivedDataCache` stay out of git.
