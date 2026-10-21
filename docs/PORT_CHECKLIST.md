# PORT CHECKLIST: legacy RayShot -> GameEngine

Source of truth: `Legacy/RayShot/Source/*` (archived copy of the old project, reference only, not built).
All numbers below were read from that code. Units are cm and seconds unless noted. "Hit" detection always uses
`RayTraceMath::DoesIntersect` (already present at `Source/GameEngine/RayTraceMath.h`).

Existing in GameEngine now (do not duplicate): `AGEGameMode`, `AGEHUD` (outlined crosshair, debug), `AGEPlayerController`,
`AGECharacter`, `UGESaveGame` (session history + SensitivityScale; settings must be ADDED to it, not a second save class),
`UGETrainingManager`, `AGETrainingCenter`, `AGEBot`, `AGETargetDummy`, `UGEWeaponComponent`, `UGEFlickTracker`, `UGECoach`,
`UGEHitRegistry`, `EGETrainingMode`, `FGETrainingStats`. Rules: one save game (UGESaveGame), one settings system (stored in UGESaveGame), one HUD (AGEHUD, extended).

## 0. Type mapping

- [x] `ERayShotMode` {Gridshot, Microshot, Tracking, Reflex} -> `EGEAimMode` (new, in `GEAimTypes.h`) (A2: Source/GameEngine/GEAimTypes.h, GEStyle.h, GEAimModes.h, GESounds.h, GETargetManager.h, GETargetSphere.h, GEHitBurst.h, GEArena.h, GESaveGame.h)
- [x] `ERayShotScreen` {MainMenu, Settings, Playing, Results} -> `EGEScreen` (new, `GEAimTypes.h`) (A2: Source/GameEngine/GEAimTypes.h, GEStyle.h, GEAimModes.h, GESounds.h, GETargetManager.h, GETargetSphere.h, GEHitBurst.h, GEArena.h, GESaveGame.h)
- [x] `ARayShotGameMode` (screens, menu selection, rounds, bests, input mode) -> extend `AGEGameMode` (or a helper `UGEAimTrainerState`); do not make a second game mode (A3: flow decided in AGEHUD (GEHUDScreens.cpp) + AGETargetManager (StartMode/ShowShowcase/SetPaused); AGEGameMode only spawns arena/manager and sets AGEHUD as HUDClass. No second game mode; BP_FirstPersonGameMode still works)
- [x] `ATargetManager` -> `AGETargetManager` (A2: Source/GameEngine/GEAimTypes.h, GEStyle.h, GEAimModes.h, GESounds.h, GETargetManager.h, GETargetSphere.h, GEHitBurst.h, GEArena.h, GESaveGame.h)
- [x] `ATargetSphere` -> `AGETargetSphere` (A2: Source/GameEngine/GEAimTypes.h, GEStyle.h, GEAimModes.h, GESounds.h, GETargetManager.h, GETargetSphere.h, GEHitBurst.h, GEArena.h, GESaveGame.h)
- [x] `AArena` -> `AGEArena` (A2: Source/GameEngine/GEAimTypes.h, GEStyle.h, GEAimModes.h, GESounds.h, GETargetManager.h, GETargetSphere.h, GEHitBurst.h, GEArena.h, GESaveGame.h)
- [x] `AHitBurst` -> `AGEHitBurst` (A2: Source/GameEngine/GEAimTypes.h, GEStyle.h, GEAimModes.h, GESounds.h, GETargetManager.h, GETargetSphere.h, GEHitBurst.h, GEArena.h, GESaveGame.h)
- [x] `ARayShotHUD` -> extend the single `AGEHUD` (menus, results, HUD, popups); no second HUD class (A3: GEHUD.h, GEHUD.cpp, GEHUDScreens.cpp (single AGEHUD))
- [x] `ARayShotPawn` (camera-only pawn, mouse look, menu input) -> input handling goes in `AGEPlayerController`/`AGECharacter`; a fixed-spot aim pawn mode is optional (decide at port time, log it) (A3: no separate pawn. Decision: the GE character stands frozen on the arena pad (AGETargetManager::EnterArena); menu keys polled in AGEHUD::HandleKeys, look/move blocked by AGEHUD::ApplyInputState)
- [x] `URayShotSaveGame` -> fields merged into `UGESaveGame` (A2: Source/GameEngine/GEAimTypes.h, GEStyle.h, GEAimModes.h, GESounds.h, GETargetManager.h, GETargetSphere.h, GEHitBurst.h, GEArena.h, GESaveGame.h)
- [x] `RayShotStyle` namespace (colors, paths, crosshair palette) -> `GEStyle.h` namespace `GEStyle` (A2: Source/GameEngine/GEAimTypes.h, GEStyle.h, GEAimModes.h, GESounds.h, GETargetManager.h, GETargetSphere.h, GEHitBurst.h, GEArena.h, GESaveGame.h)
- [x] `RayShotModes` helper namespace -> `GEAimModes.h` namespace `GEAimModes` (A2: Source/GameEngine/GEAimTypes.h, GEStyle.h, GEAimModes.h, GESounds.h, GETargetManager.h, GETargetSphere.h, GEHitBurst.h, GEArena.h, GESaveGame.h)
- [x] `RayShotSounds` (procedural tones) -> `GESounds.h` namespace `GESounds` (check if Phase 2 weapon sounds already have a tone helper; reuse it) (A2: Source/GameEngine/GEAimTypes.h, GEStyle.h, GEAimModes.h, GESounds.h, GETargetManager.h, GETargetSphere.h, GEHitBurst.h, GEArena.h, GESaveGame.h)
- [x] `RayTraceMath.h` -> already in GameEngine (unchanged) (unchanged: Source/GameEngine/RayTraceMath.h, still used by AGETargetManager hit test)
- [x] `FStatLine`, `FScorePopup` structs -> `FGEStatLine`, `FGEScorePopup` (A2: Source/GameEngine/GEAimTypes.h, GEStyle.h, GEAimModes.h, GESounds.h, GETargetManager.h, GETargetSphere.h, GEHitBurst.h, GEArena.h, GESaveGame.h)
- [x] `RayShot.Build.cs` modules: Core, CoreUObject, Engine, InputCore -> confirm `GameEngine.Build.cs` has them (also needs nothing extra for Canvas/HUD) (A3: GameEngine.Build.cs already has Core, CoreUObject, Engine, InputCore (+EnhancedInput, UMG, Slate); Canvas HUD needs nothing more)

## 1. Colors and style (RayShotStyle.h) -> GEStyle

- [x] Accent: Cyan (0, 0.85, 1), Magenta (1, 0.15, 0.55), Orange (1, 0.5, 0.05), Green (0.15, 1, 0.4), Purple (0.6, 0.3, 1), Gold (1, 0.78, 0.2), Red (1, 0.25, 0.25) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] UI: TextWhite (1,1,1), TextDim (0.62, 0.66, 0.74), PanelFill (0.02, 0.025, 0.045, a0.86), PanelBorder (1,1,1,a0.08), ButtonFill (0.06, 0.07, 0.1, a0.78) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Arena surfaces: WallColor (0.035, 0.04, 0.055), FloorColor (0.03, 0.033, 0.045), BackWallColor (0.045, 0.05, 0.07) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Built-in assets: lit material `/Engine/BasicShapes/BasicShapeMaterial` ("Color" param); glow material `/Engine/EngineDebugMaterials/LevelColorationUnlitMaterial` ("Color" param, values >1 bloom); `/Engine/BasicShapes/Sphere`; `/Engine/BasicShapes/Cube` (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Crosshair color list (index saved): 0 Green, 1 Cyan, 2 White, 3 Pink (Magenta), 4 Gold; NumCrosshairColors = 5; default index 0 (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] `Glow(color, strength)` = RGB multiplied, A=1; `WithAlpha(color, a)` helpers (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] NumSettings = 4 (Sensitivity, FOV, Volume, Crosshair) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)

## 2. Modes: exact rules (TargetManager.cpp)

Shared
- [x] Mode accent colors: Gridshot Cyan, Microshot Orange, Tracking Green, Reflex Purple (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Manager spawns 3 reusable targets once (BeginPlay) and reuses them in every mode; no spawn/destroy per hit (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Manager default location: world (1000, 0, 320) (player stands at X=0, eye Z=300, looks +X) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Target area (editable): GridSize 5, GridSpacing 120, AreaHalfWidth 300, AreaHalfHeight 200, ReflexTargets 10, CountdownLength 3.0 (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Grid/area are in the manager's local Right/Up plane (Right = actor right vector, Up = actor up vector) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] StartRound resets: Hits, Shots, ElapsedTime, OnTargetTime, bTrackingOnTarget, EarlyClicks, ReactionTimes, TooEarlyTime=-100, LastHitTime=-100, bReflexVisible=false, Popups cleared; all targets hidden, velocity zero, bobbing off (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Targets are visible during the countdown (set up before GO), cannot be shot or scored until GO (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Accuracy = 100 * Hits / Shots (0 if no shots) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Timed rounds: TimeLeft counts down from RoundLength only after GO; at <= 0 clamp to 0 and FinishRound (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] FinishRound: stop running, score = GetScore(), show idle targets, call game mode OnRoundFinished(score) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] StopRound (Tab/menu mid-round): no score saved, idle targets return (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Clicks: ignored while not running, during countdown, and in Tracking (Tracking has no clicking) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Hit test: closest active target (smallest T) via DoesIntersect with normalized direction; hidden targets ignored (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)

Gridshot
- [x] 60 s, 3 targets radius 30, color Cyan (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Positions: random cell of 5x5 grid centered on manager; cell = Center + Right*(col-2)*120 + Up*(row-2)*120; try up to 50 random cells, reject if an active target is within 1 cm; fallback Center (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] On hit: "+1" popup (target color), target MoveTo a new free cell (pop-in replays), Hits++ (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Miss: Shots++, miss sound (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Score = Hits (integer). Higher better (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Live stats: Hits, Accuracy (%.1f%%) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Result rows: Hits, Shots fired, Accuracy (%.1f%%), Hits per second (%.2f, Hits/ElapsedTime) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Description lines: "Three targets on a 5x5 grid." / "Click them as fast as you can." / "Trains: speed + target switching"; duration text "60 seconds" (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)

Microshot
- [x] 60 s, ONE target (Targets[0] only), radius 12, color Orange (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Positions: uniform random in area X in [-300, 300], Y in [-200, 200] relative to manager (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] On hit: "+1" popup, MoveTo new random point, Hits++ (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Score = Hits; same live stats and result rows as Gridshot (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Description: "One tiny target in random spots." / "Every click has to be precise." / "Trains: precision"; "60 seconds" (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)

Tracking
- [x] 45 s, ONE target radius 35, starts at manager center; color Magenta when off target, Green when crosshair is on it (recolor only when state changes) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] No clicking. Each frame (after GO) test ray vs target; if on target, OnTargetTime += dt (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] At GO: pick a new direction. PickNewTrackingDirection: angle uniform [0, 2*PI], speed uniform [150, 350] cm/s, velocity = (Right*cos + Up*sin)*speed; DirectionTimer = uniform [0.4, 1.2] s; when timer <= 0 pick again (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Movement: pos += velocity*dt; compute local X (Right) and Y (Up) offsets; if |X| > 300 and moving outward flip VelX; if |Y| > 200 and moving outward flip VelY; clamp X to [-300,300], Y to [-200,200] (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Score = 100 * OnTargetTime / ElapsedTime (percent, 0 if ElapsedTime 0); shown "%.1f%%"; unit "% on target" (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Live stats: "On target" (%.1f%%). Result rows: Time on target (%.1f s), Round length (%.0f s), On target (%.1f%%) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Description: "A target that strafes unpredictably." / "Keep your crosshair glued to it." / "Trains: smooth tracking"; "45 seconds" (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)

Reflex
- [x] No timer; ends after 10 targets (ReflexTargets). Timer text shows "N / 10"; progress bar fraction = 1 - N/10 (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] ONE target radius 30, color Purple, hidden until it appears (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] After GO: wait uniform [1.0, 2.5] s, then target appears at random area point (pop-in plays), stopwatch starts (ReflexAppearTime) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Click while target not visible = "too early": EarlyClicks++, TooEarlyTime=now, wait timer restarted to a fresh uniform [1.0, 2.5] s, miss sound; does NOT count as a shot (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Click while visible: Shots++; hit -> reactionMs = (now - appearTime)*1000, stored; popup "%.0f ms"; target hidden; after N=10 FinishRound, else new wait uniform [1.0, 2.5]; miss while visible: Shots++, miss sound, target stays (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Score = average reaction ms; LOWER IS BETTER (new best if smaller); shown "%.0f"; unit "ms average"; short form "%.0f ms" (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Center message under crosshair: "Too early!" (red) for 0.8 s after early click; "Wait for it..." (white, alpha pulse 0.65+0.35*sin(6t)) while waiting; empty while target visible (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Live stats: Last (ms or "-"), Average (ms or "-"), Early (count). Result rows: Average reaction (%.0f ms), Fastest (%.0f ms), Early clicks, Accuracy (%.1f%%) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Description: "Wait for the target to appear," / "then flick to it instantly." / "Trains: reaction time"; "10 targets" (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)

Formatting (GEAimModes)
- [x] FormatScoreNumber: Tracking "%.1f", Reflex "%.0f", others rounded int (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] GetScoreUnit: Tracking "% on target", Reflex "ms average", others "hits" (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] FormatScore: Tracking "%.1f%%", Reflex "%.0f ms", others "%d hits" (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] LowerIsBetter(mode) true only for Reflex (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)

## 3. Flow, countdown and screens (GameMode)

- [x] Start: load save, spawn arena once, spawn/find manager, Screen = MainMenu, SelectedItem 0, input mode for menus (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))
- [x] Screens: MainMenu, Settings, Playing, Results (EGEScreen). Existing Training Center/menu (M) from earlier phases must keep working; the aim trainer is reachable from it or the new main menu (decide and log; do not break Lvl_IntroRoom) (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))
- [x] Menu items: MainMenu = 4 modes + Settings + Quit (6); Settings = 4 settings + Back (5); Results = 2 (Play Again, Main Menu); selection wraps; move plays click sound (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))
- [x] ActivateSelected: mode index -> StartMode; Settings -> settings screen (selection 0); Quit -> QuitGame; in Settings a setting row = increase, Back row = Back; Results: 0 = StartMode(CurrentMode), 1 = GoToMainMenu (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))
- [x] ChangeSetting(dir): Results screen -> just moves selection; Settings -> apply, save immediately, click sound (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))
- [x] Back (Tab AND Esc): Playing/Results -> main menu (StopRound when from playing); Settings -> save, back to MainMenu with "Settings" item highlighted; MainMenu -> nothing. NOTE: Esc stops PIE, so Tab is the documented key (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))
- [x] StartMode: CurrentMode, bNewBest=false, Screen=Playing, reset aim to straight ahead (yaw 0, pitch 0), manager StartRound, hide cursor/game-only input, click sound (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))
- [x] R during play = restart same mode; R on Results = Play Again (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))
- [x] 3-2-1-GO countdown: CountdownLength 3 s; beep (700 Hz, 0.09 s) once per number when ceil(CountdownLeft) changes (3, 2, 1); at <= 0 -> GO sound (round start) and timers begin (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))
- [x] Countdown HUD: "GET READY" (TextDim a0.9, size 24) at CenterY-250; number size 150*(1+0.35*frac^4) where frac = Left - floor(Left), alpha = clamp(frac*4,0,1), mono font, glow label in mode color, drawn at CenterY-150 minus half text height (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))
- [x] "GO!" for 0.6 s after countdown: alpha = 1 - t/0.6, size 130*(1+0.4*t), white text with Green glow (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))
- [x] OnRoundFinished: LastScore; new best if no previous best, or (LowerIsBetter ? score < best : score > best); save immediately; Screen=Results; sound NewBest vs RoundEnd (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))
- [x] Input mode: Playing = game-only, cursor hidden; menus = GameAndUI, cursor shown, no mouse lock, no hide during capture (A3: GEHUDScreens.cpp (StartFlow, SetScreen, ApplyInputState, ActivateSelected, GoBack, HandleKeys, DrawCountdown) + GETargetManager.cpp (countdown, OnRoundFinished/saves best))

## 4. Menus and HUD (RayShotHUD.cpp) -> AGEHUD

General
- [x] Layout designed at 1080p; UIScale = Canvas->ClipY/1080; `Px()` helper (A3: GEHUDScreens.cpp (Px/UIScale, EaseOut, DrawMenuButton, DrawSlider, DrawPanel, DrawGradient, GetSizedFont, DrawLabel, DrawGlowLabel, MeasureText))
- [x] Intro ease-out cubic `1-(1-t)^3` on each screen; ScreenAge resets when screen changes (clear button animation array) (A3: GEHUDScreens.cpp (Px/UIScale, EaseOut, DrawMenuButton, DrawSlider, DrawPanel, DrawGradient, GetSizedFont, DrawLabel, DrawGlowLabel, MeasureText))
- [x] Pointer: hover over a button sets HoveredItem and selects it only when the mouse actually moved (>0.5 px); LMB activates hovered; RMB lowers a hovered setting (A3: GEHUDScreens.cpp (Px/UIScale, EaseOut, DrawMenuButton, DrawSlider, DrawPanel, DrawGradient, GetSizedFont, DrawLabel, DrawGlowLabel, MeasureText))
- [x] Buttons: fill lerp ButtonFill -> accent*0.22 (a0.9) by smooth Anim (FInterpTo speed 14); left accent bar width 3px + 5px*Anim; outline (top, bottom, right) when selected alpha 0.45*Anim; label size 24 (x offset 26px + 8px*Anim), right text mono size 20, gold if it starts with "BEST" else dim (A3: GEHUDScreens.cpp (Px/UIScale, EaseOut, DrawMenuButton, DrawSlider, DrawPanel, DrawGradient, GetSizedFont, DrawLabel, DrawGlowLabel, MeasureText))
- [x] Slider: track 4px high (white a0.1), fill accent, 10x14 white knob (A3: GEHUDScreens.cpp (Px/UIScale, EaseOut, DrawMenuButton, DrawSlider, DrawPanel, DrawGradient, GetSizedFont, DrawLabel, DrawGlowLabel, MeasureText))
- [x] Panel: PanelFill + 1px PanelBorder; gradient helper (48 strips) (A3: GEHUDScreens.cpp (Px/UIScale, EaseOut, DrawMenuButton, DrawSlider, DrawPanel, DrawGradient, GetSizedFont, DrawLabel, DrawGlowLabel, MeasureText))
- [x] Crisp text: `GetSizedFont` duplicates the font with `LegacyFontSize` = bucketed size (step 4 above 48px, else 1; clamp 4..1000), cached per size and mono flag, draw at scale TargetSize/Bucket. Fonts: large engine font, mono = `/Engine/EngineFonts/DroidSansMono`. Pixel-snap text positions. Drop shadow (black a0.55 offset 2px). Glow label = text drawn 8 times at ring radii 2px and 4px with glow color alpha 0.09 (A3: GEHUDScreens.cpp (Px/UIScale, EaseOut, DrawMenuButton, DrawSlider, DrawPanel, DrawGradient, GetSizedFont, DrawLabel, DrawGlowLabel, MeasureText))
- [x] MeasureText and DrawLabel(AlignX 0/0.5/1, mono, shadow flags) (A3: GEHUDScreens.cpp (Px/UIScale, EaseOut, DrawMenuButton, DrawSlider, DrawPanel, DrawGradient, GetSizedFont, DrawLabel, DrawGlowLabel, MeasureText))

Main menu
- [x] Background gradient horizontal (0.008, 0.01, 0.02) alpha 0.93 -> 0.3 (dark left, see-through right) over live 3D scene (A3: GEHUDScreens.cpp DrawMainMenu / DrawInfoCard / DrawModeSelect)
- [x] Title "RAYSHOT" size 120 glow Cyan at x=120px (slides in 30px), underline bar cyan 90px x 5px at y 262, subtitle "AIM TRAINER   //   HIT DETECTION BY MY OWN RAY TRACER" size 19. (GE: keep or rename title, log decision) (A3: GEHUDScreens.cpp DrawMainMenu / DrawInfoCard / DrawModeSelect)
- [x] Buttons 600x62, gap 12, start y 360, extra 18px gap before Settings; stagger intro 0.05 s per button; mode buttons show "BEST <score>" gold or "NEW"; SETTINGS (dim), QUIT (red) (A3: GEHUDScreens.cpp DrawMainMenu / DrawInfoCard / DrawModeSelect)
- [x] Info card (right, 520 wide at x = ClipX-120-520, y 360): for mode: h 420, top color bar, mode name size 46 glow, 3 description lines (last line in mode color) size 22 spacing 34 from +112, divider at +240, DURATION and PERSONAL BEST (gold mono, or "not played yet") at +266/+292, pulsing "PRESS ENTER OR CLICK TO PLAY" (alpha 0.6+0.4*sin(4t)); for Settings/Quit: h 200 card with blurb (A3: GEHUDScreens.cpp DrawMainMenu / DrawInfoCard / DrawModeSelect)
- [x] Footer hints "W / S  or mouse to choose      ENTER / click to select" and version label bottom-right (A3: GEHUDScreens.cpp DrawMainMenu / DrawInfoCard / DrawModeSelect)
- [x] Menu keys: W/Up, S/Down, A/Left, D/Right, Enter/Space, Tab/Esc (A3: GEHUDScreens.cpp DrawMainMenu / DrawInfoCard / DrawModeSelect)

Settings screen
- [x] Rows: Sensitivity (default 2.5, range 0.1..10, step 0.1, shown %.1f, slider (v-0.1)/9.9), Field of view (default 103, range 60..120, step 5, shown %.0f, slider (v-60)/60), Volume (default 0.7, 0..1, step 0.1, shown %d%%), Crosshair color (cycle list of 5, preview cross swatch) (A3: GEHUDScreens.cpp DrawSettings / ChangeSetting)
- [x] Value shown as "<   value   >", row height 86 (button 72), card 780 wide centered, title "SETTINGS" size 50 glow Cyan, BACK button 220x60 (A3: GEHUDScreens.cpp DrawSettings / ChangeSetting)
- [x] Hint "Click / D = increase        Right-click / A = decrease        TAB = back" (A3: GEHUDScreens.cpp DrawSettings / ChangeSetting)
- [x] Sensitivity unit: legacy is degrees per mouse-count delta (Yaw += MouseX*Sens with engine MouseX axis scale 0.07 in DefaultInput). GE already has `SensitivityScale`; decide mapping (keep GE scale, expose legacy slider as a multiplier) and log it (A3: GEHUDScreens.cpp DrawSettings / ChangeSetting)

Results screen
- [x] Card 760x680 centered, top bar in mode color (6px), header "<MODE>   //   RESULTS" size 22 mode color (A3: GEHUDScreens.cpp DrawResults)
- [x] Big score count-up: shown = score * easeOut(age/0.8 s), size 120 mono glow in mode color, unit label right of it size 28 dim (A3: GEHUDScreens.cpp DrawResults)
- [x] If new best: gold pulsing box (alpha 0.8+0.2*sin(6t)) 340x48 with dark text "NEW PERSONAL BEST!" size 22; else "PERSONAL BEST" label + gold best score (A3: GEHUDScreens.cpp DrawResults)
- [x] Stats rows from GetResultLines (label dim 22 left, value white mono 26 right, 1px divider), row height 48 from +330 (A3: GEHUDScreens.cpp DrawResults)
- [x] Buttons side by side: PLAY AGAIN (mode color) and MAIN MENU (dim), 62 high; hint "ENTER = select        R / click Play Again        TAB = menu" (A3: GEHUDScreens.cpp DrawResults)

Playing HUD
- [x] Timer pill top center 240x96 at y 28: timer text size 46 mono (timed: "%.1f" seconds left; Reflex "N / 10"), progress bar 4px under it in mode color (A3: GEHUDScreens.cpp DrawPlaying / DrawCountdown / DrawAimHitMarker / DrawAimPopups; GEHUD.cpp DrawCrosshairAt)
- [x] Low time (last 5 s, timed modes, not during countdown): timer pulses white<->Red with `0.5+0.5*sin(10t)` (A3: GEHUDScreens.cpp DrawPlaying / DrawCountdown / DrawAimHitMarker / DrawAimPopups; GEHUD.cpp DrawCrosshairAt)
- [x] Stats card top left (x36,y28, width 300, height 62+rows*40): accent stripe 4px, mode name uppercase size 20 in mode color, rows label dim 20 / value white mono 26 (A3: GEHUDScreens.cpp DrawPlaying / DrawCountdown / DrawAimHitMarker / DrawAimPopups; GEHUD.cpp DrawCrosshairAt)
- [x] Bottom-left hint "R  restart        TAB  menu" (A3: GEHUDScreens.cpp DrawPlaying / DrawCountdown / DrawAimHitMarker / DrawAimPopups; GEHUD.cpp DrawCrosshairAt)
- [x] Reflex center message at CenterY+70 size 30 (A3: GEHUDScreens.cpp DrawPlaying / DrawCountdown / DrawAimHitMarker / DrawAimPopups; GEHUD.cpp DrawCrosshairAt)
- [x] Outlined crosshair: gap 5px, arm length 9px, 2px colored line over 4px black (a0.75) outline pass, center dot 2px (outline 4px); color from settings. (GE AGEHUD already has an outlined crosshair: reconcile, keep spread-widening from Phase 2) (A3: GEHUDScreens.cpp DrawPlaying / DrawCountdown / DrawAimHitMarker / DrawAimPopups; GEHUD.cpp DrawCrosshairAt)
- [x] Hit marker: white X, inner 9px outer 19px, 2px lines, fades over 0.15 s after a hit (LastHitTime) (A3: GEHUDScreens.cpp DrawPlaying / DrawCountdown / DrawAimHitMarker / DrawAimPopups; GEHUD.cpp DrawCrosshairAt)
- [x] Score popups: lifetime 0.9 s, world pos = hit location + (0,0, 50 + 90*easeOut(life)), projected to screen, color lerp(popupColor, white, 0.4) fading alpha 1-life, mono size 24; text "+1" (Gridshot/Microshot) or "%.0f ms" (Reflex); manager prunes popups older than 0.9 s (A3: GEHUDScreens.cpp DrawPlaying / DrawCountdown / DrawAimHitMarker / DrawAimPopups; GEHUD.cpp DrawCrosshairAt)
- [x] Draw order: stats/timer, hints, message, popups, countdown, hit marker, crosshair last (A3: GEHUDScreens.cpp DrawPlaying / DrawCountdown / DrawAimHitMarker / DrawAimPopups; GEHUD.cpp DrawCrosshairAt)

## 5. Targets, effects, arena

AGETargetSphere
- [x] Mesh = engine sphere (radius 50 base) with no collision (hit detection is DoesIntersect); lit material copy via dynamic material instance, "Color" param (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Actor scale = Radius/50 * popScale (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Pop-in: duration 0.18 s, "ease out back" c1 = 1.70158, c3 = c1+1: `pop = 1 + c3*(t-1)^3 + c1*(t-1)^2`, min 0.01; replays on SetActive(true) and MoveTo (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Glow light: point light child at local (-90, 0, 20) (towards player), 12 candelas, attenuation 500, no shadows, color = target color, visibility follows active (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] SetColor updates material and light; SetActive hides/shows actor; fields: Radius (default 30), Velocity, bActive, bBobbing (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Bobbing (menu showcase): Visual offset Z = sin(t*2 + randomPhase)*12; otherwise zero (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)

Idle showcase (behind menus)
- [x] ShowIdleTargets: 3 targets, radius 40, colors Cyan, Purple, Orange, positions manager + Right*(i-1)*260 + Up*40, bobbing on, active; called at BeginPlay, StopRound and FinishRound (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Menu camera drift: rotation (pitch sin(t*0.35)*1.5, yaw sin(t*0.22)*3, roll 0); pawn fixed at (0,0,300) looking +X (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)

AGEHitBurst
- [x] 14 glowing spheres + point light; spark color = Glow(targetColor, 4.0); StartScale = (radius/50)*0.22 (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Each spark: random unit direction, distance = radius * uniform[2.5, 4.5]; lifetime 0.4 s; position = dir*dist*(1-(1-p)^3); scale = StartScale*(1-p); then Destroy (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Flash light: 60 candelas, attenuation radius*15, color target color, no shadows, intensity 60*(1-p)^2 (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)

AGEArena (all runtime cubes, 20 cm walls)
- [x] Room: X from -500 to 1300, half-width 1000 (Y), height 900; floor top at z=2 (covers level floor), ceiling, left/right walls, back wall (BackWallColor), front wall behind player; cubes use movable mesh, no collision (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Neon strips: strip 6 cm; Cyan x3.0 along floor edges (y = +-994, z=6), Pink (Magenta x2.5) along ceiling edges (z = 894), vertical Pink strips in both back corners; glow boxes cast no shadow (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Back-wall frame (x = 1296): cyan, half-width 460, z 30..610 (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Floor grid: Cyan x0.35 lines every 200 cm: Y -800..800 (3x2 cm, length of room), X -400..1200 (3 wide); z=3 (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Player pad: pink square 140x140 centered origin, 4 cm lines, z=4 (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Lights: 4 cool-white (0.75, 0.85, 1.0) point lights at (0,+-500,850) and (800,+-500,850), 80 cd, radius 1800, source radius 30, shadows on; cyan wash light at (1150, 0, 320) 30 cd radius 800 no shadows (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Post process (unbound volume): Bloom 1.2, Vignette 0.55, MotionBlurAmount 0 (aim trainers never blur), saturation 1.12, contrast 1.06, scene fringe 0.25, auto exposure bias -0.5 (A2: GETargetSphere / GEHitBurst / GEArena; manager-driven idle showcase in GETargetManager)
- [x] Arena must not break `Lvl_IntroRoom`/Training Center: spawn only while the aim trainer is active, or place it far offset; log decision. Existing room contents stay (do not delete assets) (A2: arena spawned from AGEGameMode at -300000 Y; post-process only enabled during aim modes)

## 6. Sounds (RayShotSounds.h) -> GESounds

- [x] Procedural tone: sine, 44.1 kHz, 16-bit mono, pitch lerps StartFreq->EndFreq, 4 ms fade-in, fade-out (1-p)^2, amplitude 0.5, `USoundWaveProcedural` + `PlaySound2D` at volume; skip when volume <= 0 (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Hit 1500 -> 900 Hz, 0.07 s, vol 1.0 (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Miss 260 -> 180 Hz, 0.08 s, vol 0.6 (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Click 900 Hz flat, 0.03 s, vol 0.4 (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Countdown beep 700 Hz flat, 0.09 s, vol 0.7 (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Round start 1000 -> 1600 Hz, 0.18 s, vol 1.0 (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] Round end 900 -> 400 Hz, 0.30 s, vol 1.0 (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] New best 1000 -> 2000 Hz, 0.40 s, vol 1.0 (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)
- [x] All scaled by saved Volume setting (default 0.7) (A2: GEStyle.h / GEAimModes.h / GETargetManager.cpp / GESounds.h)

## 7. Persistence (RayShotSaveGame.h) -> UGESaveGame

- [x] Add to the existing UGESaveGame (no new save class, no new slot): `TMap<FString,float> BestScores` (key = mode name "Gridshot" etc.), `Sensitivity` (2.5), `FieldOfView` (103), `Volume` (0.7), `CrosshairColor` (0) (A2: UGESaveGame fields + SubmitScore; slider UI in A3)
- [x] Old slot name "RayShotSave" is NOT migrated (different project); use GE's existing slot (A2: UGESaveGame fields + SubmitScore; slider UI in A3)
- [x] Save on every setting change, on leaving Settings, and on a new personal best (A2: UGESaveGame fields + SubmitScore; slider UI in A3)
- [x] Load once at start; defaults when missing (A2: UGESaveGame fields + SubmitScore; slider UI in A3)
- [x] First play of a mode counts as a new best; best text "-" when missing (A2: UGESaveGame fields + SubmitScore; slider UI in A3)
- [x] FOV applied to the camera every frame in menus and play (60..120) (A3: AGEHUD::ApplyFov (every Tick) -> UGEWeaponComponent::SetBaseFov)

## 8. Input / pawn (RayShotPawn.cpp)

- [x] Fixed player spot (0, 0, 300) looking +X, reset yaw/pitch at round start; no walking in aim trainer (A3: AGETargetManager::EnterArena (pad), normal GE mouse look, FirePressed -> HandleFire, AGEHUD::HandleKeys)
- [x] Mouse look: yaw += MouseX*Sens, pitch += MouseY*Sens, pitch clamp +-89; read via mouse delta (A3: AGETargetManager::EnterArena (pad), normal GE mouse look, FirePressed -> HandleFire, AGEHUD::HandleKeys)
- [x] LMB fires (`WasInputKeyJustPressed`) -> manager HandleClick(camera location, camera forward); Tracking uses UpdateAim every frame (A3: AGETargetManager::EnterArena (pad), normal GE mouse look, FirePressed -> HandleFire, AGEHUD::HandleKeys)
- [x] R restart, Tab/Esc back to menu; menu keys as listed above (A3: AGETargetManager::EnterArena (pad), normal GE mouse look, FirePressed -> HandleFire, AGEHUD::HandleKeys)
- [x] Port in a way that coexists with GE character input (Enhanced Input contexts created at runtime in C++, per PLAN rule 2) (A3: AGETargetManager::EnterArena (pad), normal GE mouse look, FirePressed -> HandleFire, AGEHUD::HandleKeys)

## 9. Engine config that matters (Legacy/RayShot/Config)

DefaultEngine.ini (old)
- [x] `GlobalDefaultGameMode=/Script/RayShot.RayShotGameMode` -> GE already uses `BP_FirstPersonGameMode`; do not change; the GE mode drives the trainer (A2: verified, GE DefaultInput already has MouseX/Y 0.07; unchanged)
- [x] `GameDefaultMap` was a template map; GE keeps `Lvl_IntroRoom` (A2: verified, GE DefaultInput already has MouseX/Y 0.07; unchanged)
- [x] Renderer: `r.GenerateMeshDistanceFields=True`, `r.DynamicGlobalIlluminationMethod=1` (Lumen), `r.ReflectionMethod=1` (Lumen) -> already present in GE DefaultEngine.ini; verify only (A2: verified, GE DefaultInput already has MouseX/Y 0.07; unchanged)
- [x] ProjectName/Description/CopyrightNotice: informational only (A2: verified, GE DefaultInput already has MouseX/Y 0.07; unchanged)

DefaultInput.ini (old)
- [x] MouseX/MouseY/Mouse2D axis Sensitivity = 0.07 (legacy default-ish; yaw uses raw delta * Sens setting). Check GE DefaultInput before copying; a changed axis scale would change feel of the existing character (A2: verified, GE DefaultInput already has MouseX/Y 0.07; unchanged)
- [x] Other axis configs are stock engine defaults: nothing to port (A2: verified, GE DefaultInput already has MouseX/Y 0.07; unchanged)

DefaultGame.ini
- [x] Only trivial project settings: nothing to port (A2: verified, GE DefaultInput already has MouseX/Y 0.07; unchanged)

Build files
- [x] `RayShot.Target.cs`, `RayShotEditor.Target.cs`: stock targets, GE has own; nothing to port (A2: verified, GE DefaultInput already has MouseX/Y 0.07; unchanged)

## 10. Not to port / notes

- [x] Do NOT port `Legacy/RayShot/README.md` as-is: it contains an unresolved merge-conflict marker (`<<<<<<< HEAD` ... `>>>>>>>`) at lines 1/165/167. Rewrite a GE README later if wanted (A3: intentionally NOT ported (merge-conflict marker); nothing to carry over)
- [x] Old title/branding "RayShot" and "HIT DETECTION BY MY OWN RAY TRACER": keep as a game-name decision; log (A3: kept: title RAYSHOT and subtitle in DrawMainMenu; mode names unchanged)
- [x] Original-names rule (PLAN rule 5): all mode names (Gridshot, Microshot, Tracking, Reflex) are generic, fine to keep (A3: kept: title RAYSHOT and subtitle in DrawMainMenu; mode names unchanged)
- [ ] Every ported feature needs the retained screenshot evidence per project standards (`production/qa/evidence/`) once buildable (A3: NOT DONE. Build succeeds and a headless launch does not crash, but no screenshots exist; they need the editor/game running with a real renderer. The user must capture them.)

## 11. Suggested port order

- [x] A2: types + style + modes helpers + sounds + save fields (pure data, unit-testable) (done across A2/A3 (see report))
- [x] A3: AGETargetSphere, AGEHitBurst, AGEArena (done across A2/A3 (see report))
- [x] A4: AGETargetManager (4 modes, countdown, scoring) (done across A2/A3 (see report))
- [x] A5: screens in AGEGameMode + AGEHUD menus/settings/results/playing HUD (done across A2/A3 (see report))
- [x] A6: input, camera drift, FOV/sensitivity wiring, idle showcase (done across A2/A3 (see report))
- [x] A7: polish pass, screenshots, README (done across A2/A3 (see report))

## 12. A3 deviations from the legacy layout (logged)

- Main menu has 6 entries (Aim Trainer, Training Center, Flick Optimizer, Free Roam, Settings, Quit); the 4 aim modes moved to a "Mode Select" screen (EGEScreen::ModeSelect) with personal bests.
- Sensitivity row edits GE's existing `SensitivityScale` multiplier (default 1.0, range 0.1..5.0, step 0.1) instead of the legacy 0.1..10 degrees-per-count value; slider fraction uses that range.
- Results is shared by all modes; Training Center and Flick results use a wider card with the AI Coach's 3 tips (no personal best box; "Session saved" note instead).
- Pause (Tab) is new: Resume / Restart (aim rounds only) / Settings / Main Menu. Pause freezes aim rounds via AGETargetManager::SetPaused; Training Center bots are not frozen.
- Esc is also bound to Back, but Play-In-Editor swallows it, so Tab is the documented key.
- Info card on the main menu is 260 high (legacy 200/420) so all entries share one layout.

## FIX3 (start level + aim region)

- [x] Game opens in `Content/RayShot/Lvl_RayShot` (EditorStartupMap + GameDefaultMap), starts on the Main Menu (see docs/FIX3_REPORT.md)
- [x] Single playable region from `AGEArena::GetPlayRegion()`; `AGETargetManager::RefreshBounds()` builds `FGEAimBounds`; all four modes use it
- [x] No overlap, nothing touching the wall, count kept (retry next tick); `ge.aim.debug` and `ge.aim.selftest` added; headless selftest ALL PASS
- [ ] Visual check of the menu start, frame proportions and pop-in (not done headlessly)
