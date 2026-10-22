# Release report: RayShot v1.0

## What was built
- Settings: ProjectName RayShot, CompanyName Rakshit Singh, ProjectVersion 1.0.0, Shipping, pak file, MapsToCook = Lvl_RayShot only (Lvl_IntroRoom is not cooked; the Training Center is spawned by C++ in Lvl_RayShot).
- Added DirectoriesToAlwaysCook for /Game/UI/Fonts (fonts are loaded by path in code, so they would not be cooked otherwise), /Game/FirstPerson/Blueprints and /Game/Input.
- RunUAT BuildCookRun succeeded on the first attempt (about 400 s).
- Launcher renamed to `RayShot.exe`; the real game exe is still `GameEngine/Binaries/Win64/GameEngine-Win64-Shipping.exe` (renaming it would break the launcher). The .pdb was removed from the zip.
- Added LICENSE (MIT, Rakshit Singh), release notes, itch page text, screenshot checklist, README sections. Fixed README's stale startup map and status line.

## Output
- Zip: `C:\Users\raksh\OneDrive\Documents\rayshot\Builds\RayShot-v1.0-Windows.zip`, 320.2 MB (54 entries; contains RayShot.exe, GameEngine-Win64-Shipping.exe, GameEngine-Windows.pak 11.9 MB).
- Unzipped folder: `...\Builds\Windows`.

## Test results
- BUILD SUCCESSFUL, exit code 0.
- Exe, pak, Lvl_RayShot.umap and all 4 font assets (Rajdhani x3, Share Tech Mono) are in the cooked manifest.
- Launched `RayShot.exe -nullrhi -nosound -unattended`: the process was still running after 25 s (no crash), then I killed it.
- No log file was produced (Shipping builds disable file logging), so missing fonts, missing map and runtime errors could NOT be checked in a log.
- **Visuals were not verified**: nothing was seen on screen, and gameplay, UI and fonts rendering are unchecked.

## Issues
- Zip made with PowerShell Compress-Archive, which may store backslash paths; Windows Explorer opens it fine, other tools may differ.
- Release notes list "About 4 GB RAM" as an unmeasured estimate.

## What Rakshit must do manually
1. Unzip and play the build once: check the main menu, fonts, all 4 modes, Training Center, saving.
2. Capture screenshots/video per `docs/release/SCREENSHOT_CHECKLIST.md` into `docs/screenshots/`.
3. Create the GitHub Release (tag v1.0) and attach the zip; upload to itch.io using `docs/release/ITCH_PAGE.md`.
4. Confirm the MIT LICENSE file (I added it) and the RAM figure.
5. Optionally package a shipping build with a rebuilt-from-clean tree if the zip size (320 MB, mostly the engine exe) matters.
