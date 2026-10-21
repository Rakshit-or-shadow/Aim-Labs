# OVERNIGHT PLAN (saved verbatim from user)

PROJECT: UE 5.8 C++ project at C:\Users\raksh\OneDrive\Documents\rayshot\GameEngine (own git repo, main, no remote).
PHASE 0 DONE (commit 2107d80): Source/GameEngine/, AGECharacter, AGEGameMode, AGEPlayerController, AGEHUD (outlined crosshair). Existing first-person BPs (BP_FirstPersonCharacter/GameMode/PlayerController) reparented to them. RayTraceMath.h copied in, unused yet. Keep this approach.
Keep working: Content/FirstPerson, Content/Input (IA_Move, IA_Jump, IA_Look, IA_MouseLook, IMC_Default, IMC_MouseLook), startup map /Game/DemoTemplate/_Core/Lvl_IntroRoom.

RULES
1. Don't delete/move/rename existing assets. Don't break Lvl_IntroRoom or the current character.
2. New inputs (Fire, ADS, Reload, Walk, Crouch, weapon slots 1-5, mouse wheel, Menu) created as UInputAction/UInputMappingContext at runtime in C++, added alongside IMC_Default. No editor asset work.
3. Hit detection uses MY ray-sphere intersection RayTraceMath::DoesIntersect for EVERY shot. Bots/targets use sphere hitboxes. No line traces for shots.
4. Prefix all new C++ types with "GE". Simple, beginner-friendly, well-commented C++. Tunables in data structs or named constants.
5. Original names only: no real game's names, maps, weapon names, art or sounds. The range is called "Training Center".
6. Never: push, touch C:\.git, run git outside this folder, install software.

FIRST: save this as docs/PLAN.md and commit. Create docs/NIGHT_REPORT.md; after every phase append: phase, DONE/FAILED/SKIPPED, 3-line summary, build result, commit hash, manual steps. Commit each time.

NEVER WAIT FOR USER: never ask questions. Choose reasonably, log under "Decisions I made" in NIGHT_REPORT.md, continue. If something needs the editor, write click-by-click steps under "Manual steps for Rakshit" and continue with the rest. Editor is closed.

SAVE USAGE: coordinator keeps messages short. Run EACH phase in ONE fresh subagent with model: sonnet (never opus). haiku for report updates/commits. Each subagent gets docs/PLAN.md and its single phase: implement -> build -> fix -> commit -> return summary of 10 lines max. No /team-* skills, director reviews, web search, /reverse-document. One /code-review only, in FINISH. Build once per phase plus rebuilds only to fix errors.

BUILD: "C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GameEngineEditor Win64 Development -Project="C:/Users/raksh/OneDrive/Documents/rayshot/GameEngine/GameEngine.uproject" -WaitMutex
If a phase fails after 3 fix attempts: git switch -c broken/phase-N, commit, git switch main, mark FAILED with exact error, continue only if next phase doesn't depend on it. If files locked, wait 2 min and retry once. Commit each phase: "Phase N: <name>".

PHASE 1: Tactical movement. Tune CharacterMovement from C++ (AGECharacter): run ~540 cm/s, Shift walk (~40%), Ctrl crouch (lower camera, slower), Space jump with low air control, high acceleration and braking so counter-strafing stops fast. Expose GetCurrentSpeed() and IsOnGround(). HUD: small speed readout (debug, toggleable).

PHASE 2: Weapons. Data-driven FGEWeaponData + UGEWeaponComponent on AGECharacter. 5 original weapons: sidearm (semi-auto, accurate); SMG (fast fire, high spread); rifle (1-shot headshot on bots, strong recoil); marksman (slow, scoped zoom, very accurate when still); shotgun (several pellets, each a separate ray). Per weapon: head/body/leg damage multipliers, fire rate, magazine + reload (R), first-shot accuracy, spread growing while spraying and recovering, recoil pattern kicking camera, movement and jump inaccuracy (accurate only standing still or walking), right-click zoom/ADS. Switch with 1-5 and mouse wheel. Every shot is a ray from camera (+spread) tested with RayTraceMath::DoesIntersect against all hitbox spheres; closest wins. Visuals: simple viewmodel of basic shapes attached to first-person camera/arms, muzzle flash (brief light + glowing shape), short tracer line, procedural sounds (USoundWaveProcedural). HUD: ammo (mag/reserve), weapon name, crosshair widening with spread, hit marker.

PHASE 3: Training Center. New level "Lvl_TrainingCenter" from C++-spawned actors (basic shapes + Content/LevelPrototyping meshes): hub, shooting lane with distance markers (10/20/30/40 m), cover walls. If a new .umap can't be created without the editor, spawn it as an area INSIDE Lvl_IntroRoom (far from start), entered by pressing T or a portal; log the decision. Bots (AGEBot): humanoid of basic shapes, sphere hitboxes: head (small), chest (2 spheres), legs (2 spheres). HP 100, zone damage, death then respawn after 1.5 s, headshot feedback (sound + popup "HEADSHOT"). Modes: Static, Strafing (random ADAD), Peek (step out of cover, then hide). Difficulty Easy/Medium/Hard (speed, strafe timing, peek time). Session stats: kills, headshot %, accuracy, avg time-to-kill, first-shot accuracy. Canvas menu (M): choose mode, difficulty, weapon, start/stop, see stats.

PHASE 4: Flick Optimizer. Training Center mode: single bots/targets appear at random angles; shoot fast. Per shot record: angle crosshair->target at appearance, time to first shot, time to hit, mouse path (every frame), overshoot/undershoot, corrections (direction changes), flick direction (8 dirs). After session show per-direction accuracy and avg time, avg overshoot %, sensitivity suggestion (consistent overshoot -> lower; undershoot -> higher) with current and suggested cm/360. Mouse sensitivity setting in menu (stored). Save session history with UGESaveGame: date, mode, stats.

PHASE 5: AI Coach. UGECoach: offline rule-based (no internet/API). After each session reads Phase 3-4 stats + history, shows 3 specific tips on a results screen (e.g. over-flick right 12% -> lower sensitivity; first shots while moving inaccurate -> counter-strafe; headshot % drops past 30 m -> practice 30 m lane). Trends across sessions ("accuracy +6% over last 5 sessions", "time-to-kill improved 80 ms"). Rules in clear commented functions, thresholds as named constants. Add interface IGECoachProvider with TODO for a future LLM provider (don't implement).

FINISH: /code-review (sonnet subagent) on all new code; fix only high-priority; build; commit. Write README.md (features, every key, how to build, structure). Final NIGHT_REPORT.md: summary of phases, full controls, manual steps, "what to test first" step-by-step. Commit. Then stop.
