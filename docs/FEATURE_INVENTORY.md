# Feature inventory (RayShot v1.0)

Built from `README.md`, `docs/PORT_CHECKLIST.md`, `docs/UI_SPEC.md`, `docs/PLAN.md`, the night reports and the
source (`GEHUD*.cpp`, `GEGameMode`, `GECharacter`, `GEWeaponComponent`, `GETrainingManager`, `GEFlickTracker`,
`GECoach`, `GEAimModes.h`, `GEStyle.h`, `GESaveGame.h`). One line per feature, with the screen or state where it is visible.
IDs are used by `docs/SCREENSHOT_COVERAGE.md`. Nothing here is invented: a feature that the code does not have is listed at the end.

## A. Menus and screens

| ID | Feature | Where visible |
|---|---|---|
| A01 | Main menu: title, tagline, 6 entries (Aim Trainer, Training Center, Flick Optimizer, Free Roam, Settings, Quit), info card per entry, arena with floating showcase targets behind it | Main Menu |
| A02 | Main menu selection highlight and the info card changing with the selected entry | Main Menu |
| A03 | Mode select: 4 diamond tiles, info card (name, description, duration, personal best), Back | Aim Trainer > mode select |
| A04 | Personal bests shown on tiles and info card ("NOT PLAYED YET" before, scores after) | Mode select, before and after playing |
| A05 | Settings screen, section MOUSE: sensitivity slider (1.0 = default) | Settings |
| A06 | Settings, section VIDEO: field of view slider (60 to 120) | Settings |
| A07 | Settings, section AUDIO: master volume slider | Settings |
| A08 | Settings, section CROSSHAIR: colour swatches (5) and a live preview | Settings |
| A09 | Settings, section INTERFACE: UI theme swatches (4), scanlines toggle, screen glitch toggle | Settings |
| A10 | Pause menu over an aim round (Resume, Restart, Settings, Main Menu), round frozen behind it | Pause (Playing) |
| A11 | Pause menu from free roam (no Restart) | Pause (Free Roam) |
| A12 | Results screen for an aim round: big score, S/A/B/C/D rank, stat rows, Play Again / Main Menu | Results (aim) |
| A13 | "NEW PERSONAL BEST" badge | Results (aim, first play) |
| A14 | Results with no new best (shows the existing personal best instead) | Results (aim, replay) |
| A15 | Results for a Training Center session with the AI Coach panel (3 tips + trends) | Results (training) |
| A16 | Results for a Flick Optimizer session with flick stats and AI Coach panel | Results (flick) |
| A17 | UI theme Red (default), Cyan, Yellow, Green on the main menu | Main Menu |
| A18 | UI theme applied to the in-round HUD | Playing |
| A19 | Scanlines overlay (setting) | Any menu screen with the setting on |
| A20 | Screen-change glitch flash and its toggle | Settings (toggle); transient on screen changes |
| A21 | Quit entry (quits the game) | Main Menu (visible; the action itself is not capturable) |

## B. Aim trainer

| ID | Feature | Where visible |
|---|---|---|
| B01 | 3-2-1-GO countdown with a mode banner (name, duration, description) | Playing, first seconds |
| B02 | Gridshot: 3 targets on a 5x5 grid, hits, timer, accuracy | Playing |
| B03 | Microshot: one tiny target | Playing |
| B04 | Tracking: one moving target, "on target" percentage, target changes colour when tracked | Playing |
| B05 | Reflex: "Wait for it..." message, target appears after a random delay, reaction times (last / average / early clicks) | Playing |
| B06 | Round HUD: timer with progress bar, live stats panel, restart/pause hint, crosshair | Playing |
| B07 | Score popups ("+1", reaction ms) at hit position, hit marker, hit burst particles, target glow | Playing |
| B08 | Low-time warning (last 5 s: timer turns red, short glitch) | Playing, Tracking |
| B09 | Neon arena room (glowing frame, grid floor, bloom, lights) | Main menu, Playing |
| B10 | Wide view of the arena (high FOV) | Arena, no menu |

## C. Weapons (UGEWeaponComponent, used in Free Roam and the Training Center)

| ID | Feature | Where visible |
|---|---|---|
| C01 | Sidekick (semi-auto sidearm) firing: muzzle flash, tracer, hit marker | Training Center |
| C02 | Rattler (SMG) firing | Training Center |
| C03 | Longarm (rifle) firing | Training Center |
| C04 | Eagle Eye (scoped marksman) firing | Training Center |
| C05 | Scatter (shotgun, 9 pellets, wide spread) firing | Training Center |
| C06 | Right-click zoom / ADS for each weapon | Training Center |
| C07 | Eagle Eye scope overlay (black border, scope view, thin crosshair) | Training Center |
| C08 | Reload in progress ("Reloading..." in the ammo readout) | Training Center |
| C09 | Ammo counter / weapon name readout (bottom right) | Free Roam, Training Center |
| C10 | Crosshair gap widening with spread | Training Center |
| C11 | Hit marker, headshot popup, bot kill | Training Center |
| C12 | Procedural first-person viewmodel (one per weapon) | Training Center |

## D. Movement (UGECharacter)

| ID | Feature | Where visible |
|---|---|---|
| D01 | Standing | Free Roam / Training Center |
| D02 | Walking (Shift, about 40% speed) | Training Center |
| D03 | Running (fast run) | Training Center |
| D04 | Crouching (lower camera) | Training Center |
| D05 | Jumping, in the air (low air control) | Training Center |
| D06 | F3 speed readout (speed, Ground / Air) | Free Roam / Training Center |

## E. Training Center

| ID | Feature | Where visible |
|---|---|---|
| E01 | Training Center menu: mode (Static / Strafing / Peek / Flick), difficulty (Easy / Medium / Hard), weapon, START/STOP, sensitivity and DPI buttons | Training Center |
| E02 | Last / running session stats in the menu (kills, headshot %, accuracy, first-shot accuracy, time to kill) | Training Center menu |
| E03 | Hub, shooting lane with 10 / 20 / 30 / 40 m markers, cover, portal | Training Center |
| E04 | Static bots (humanoid, sphere hitboxes) | Training Center session |
| E05 | Strafing bots | Training Center session |
| E06 | Peek bots (step out from behind cover) | Training Center session |
| E07 | Bot hit and killed, "HEADSHOT" popup, respawn | Training Center session |
| E08 | Live session stats on the HUD (Kills / Acc / HS) | Training Center session |
| E09 | Sensitivity with cm/360 and mouse DPI | Training Center menu |

## F. Flick Optimizer and AI Coach

| ID | Feature | Where visible |
|---|---|---|
| F01 | Flick Optimizer start (Training Center menu in Flick mode) | Flick Optimizer |
| F02 | A flick session: single targets appear at random angles | Flick Optimizer |
| F03 | Flick results: targets, accuracy, average time to hit, overshoot, corrections, sensitivity advice | Results (flick) |
| F04 | Per-direction table (8 directions: hits, accuracy, average time) and sensitivity suggestion | Flick Optimizer menu panel |
| F05 | AI Coach: top 3 tips and trends over sessions (rule based, offline) | Results (training / flick), Training Center menu panel |

## G. Other

| ID | Feature | Where visible |
|---|---|---|
| G01 | Free Roam in the main level (neon gate, floor, weapon, HUD) | Free Roam |
| G02 | Saves: personal bests, settings, session history (local save slot `GE_Profile`) | Not a screen (visible indirectly as bests and settings) |
| G03 | Debug: `ge.ui.gallery` UI component gallery, `ge.aim.*` console commands | Console only |

## Not in the code (so not shown, not claimed)

- No quit confirmation dialog: Quit exits at once.
- No score streak / combo display.
- No crosshair style setting (only colour); no separate "high score table" beyond the personal best per mode.
- No settings slider dragging (click / keys only).
