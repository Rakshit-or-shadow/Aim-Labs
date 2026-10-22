# RayShot v1.0

RayShot is a first-person aim trainer for Windows built in Unreal Engine 5.8 (C++). Every shot is
resolved by a custom ray-sphere intersection ported from the author's own C ray tracer. There are no
Unreal line traces anywhere in the hit detection.

## Features

- **4 aim modes**: Gridshot (60 s), Microshot (60 s), Tracking (45 s, no clicking) and Reflex (10 targets, reaction times).
  Countdown, score popups, personal bests and an S/A/B/C/D rank on the results screen.
- **5 weapons**: Sidekick, Rattler, Longarm, Eagle Eye (scoped) and Scatter, with magazines, reload, spread, recoil and aim down sights.
- **Tactical movement**: fast run, walk, crouch, low air control, counter-strafing.
- **Training Center** with a shooting lane, cover and bots (Static, Strafing, Peek, Flick; Easy, Medium, Hard) and session stats.
- **Flick Optimizer**: per-direction accuracy and time, overshoot/undershoot, sensitivity suggestion.
- **Offline AI Coach**: rule-based tips and trends across sessions. No internet needed.
- **Saves and personal bests** stored locally.
- **Cyberpunk UI** with 4 colour themes (Red, Cyan, Yellow, Green), scanlines and screen glitch options.

## Controls

| Input | Action |
|---|---|
| W A S D | Move |
| Mouse | Look |
| Space | Jump |
| Left Shift (hold) | Walk |
| Left Ctrl (hold) | Crouch |
| Left Mouse | Fire |
| Right Mouse (hold) | Aim down sights |
| R | Reload (restart in aim modes) |
| 1 - 5 | Select weapon |
| Mouse wheel | Previous / next weapon |
| T | Teleport to the Training Center and back |
| M | Training Center menu |
| Tab | Back / pause in menus |
| W/S, A/D, Enter, mouse click | Navigate menus |

## System requirements

- Windows 10 / 11, 64-bit
- A GPU with DirectX 12 support
- About 4 GB RAM (estimate, not measured)

## How to run

1. Unzip `RayShot-v1.0-Windows.zip`.
2. Run `RayShot.exe` in the unzipped folder.
3. Pick a mode in the menu and play.

## Known limitations

- The build is unsigned: Windows SmartScreen shows a warning. Click **More info -> Run anyway**.
- Walls and cover do not block bullets (no line traces; a hidden peeking bot is simply unshootable).
- Settings sliders are click-only (not draggable).
- cm/360 is approximate (it assumes a mouse DPI, default 800, and an estimated degrees-per-count).

## Credits

- Fonts: Rajdhani and Share Tech Mono, SIL Open Font License.
- Unreal Engine by Epic Games.
- Code, design and ray-sphere math by Rakshit Singh.

## License

MIT (see `LICENSE`).
