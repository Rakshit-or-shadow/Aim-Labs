# Night Report

## Decisions I made

- Phase 2: shots are not blocked by walls (no line traces allowed); recoil sets control rotation directly; 4 AGETargetDummy spawn in Lvl_IntroRoom.
- Phase 3: no new .umap (can't without editor): Training Center is spawned from C++ 200000 cm along +Y from PlayerStart in Lvl_IntroRoom; T teleports in/out, cyan portal near start. Cover does not block shots. TTK = first hit on bot to kill. Changing mode/difficulty/weapon restarts session.
- Phase 4: cm/360 assumes DPI 800 and 0.07 deg/count at scale 1.0 (guess, in GEFlickTypes.h) so displayed cm/360 is approximate. Sensitivity scaling is in AGEPlayerController::AddYawInput/AddPitchInput. Suggestion needs >=5 flicks, 5% overshoot, 60% consistent, capped +-30%.

## Manual steps for Rakshit
- If sensitivity +/- has no effect in play, the BP's IA_Look path may not call Controller->AddYawInput; check BP_FirstPersonCharacter look graph.
- Set your real mouse DPI in the M menu.

## Phases
- Phase 0: DONE (commit 2107d80) - C++ base, HUD crosshair, BPs reparented.
- Phase 1: DONE (commit af31493) - Tactical movement: run 540, Shift walk 40%, Ctrl crouch, low air control, high accel/braking, GetCurrentSpeed()/IsOnGround(), F3 speed readout. Build: Succeeded. Manual steps: none (untested in play).
- Phase 2: DONE (commit d6f49c2) - 5 weapons (Sidekick, Rattler, Longarm, Eagle Eye, Scatter), RayTraceMath hit detection, viewmodel, flash, tracer, sounds, HUD. Build: Succeeded. Manual steps: none (untested in play).
- Phase 3: DONE (commit 78d2355) - Training Center, AGEBot (static/strafe/peek, Easy/Medium/Hard), stats, M menu, T teleport. Build: Succeeded (2nd attempt). Manual steps: none (untested in play).
- Phase 4: DONE (commit 0dc9b8f) - Flick mode, UGEFlickTracker, per-direction results, sensitivity suggestion + cm/360, UGESaveGame history. Build: Succeeded. Manual steps: see above.
- Phase 5: DONE (commit 2b0324d) - UGECoach offline rule-based (3 tips + trends), IGECoachProvider (LLM TODO only), HUD AI COACH panel. Build: Succeeded. Manual steps: none (untested in play).

## FINAL SUMMARY

### Phases
- Phase 0: C++ base classes, HUD crosshair, BPs reparented.
- Phase 1: tactical movement (run/walk/crouch, F3 speed readout).
- Phase 2: 5 weapons, RayTraceMath hit detection, viewmodel, effects, HUD.
- Phase 3: Training Center (inside Lvl_IntroRoom), bots, modes, difficulties, stats, M menu, T teleport.
- Phase 4: Flick Optimizer, sensitivity suggestion, save history.
- Phase 5: offline AI Coach with tips and trends.
- Finish: code review. One high-priority bug found and fixed: the weapon input actions (fire, aim, reload,
  wheel, slots 1-5) were created and mapped but never bound, so the weapons could not be used. Now bound in
  AGECharacter::SetupPlayerInputComponent. Build: Succeeded. Other files were skimmed; no other crashes found.

### Full controls
WASD move, mouse look, Space jump, Left Shift walk, Left Ctrl crouch, LMB fire, RMB ADS, R reload, 1-5 weapons,
mouse wheel cycle weapons, F3 speed readout, T Training Center in/out, M menu.

### Manual steps for Rakshit
- Close the editor before building. Open GameEngine.uproject and play Lvl_IntroRoom.
- If sensitivity +/- has no effect, the BP's IA_Look path may not call Controller->AddYawInput; check the look graph in BP_FirstPersonCharacter.
- Set your real mouse DPI in the M menu (cm/360 is approximate).
- Everything was built but never played during the overnight run; expect small tuning needs (recoil, spread, bot speeds).

### What to test first
1. Build (see README) with the editor closed, then open GameEngine.uproject.
2. Play Lvl_IntroRoom. Check WASD, mouse look, Space, Shift walk, Ctrl crouch (camera lowers), F3 speed readout.
3. Check the crosshair and that the gun viewmodel is attached to the view.
4. Shoot the dummies; press 1-5 and the mouse wheel to change weapons; check R reload, RMB zoom, hit marker, ammo HUD.
5. Press T to enter the Training Center (T again returns).
6. Press M; pick Static, Medium and a weapon; click START; shoot bots; click STOP and read stats.
7. Run a Flick session, then STOP: view Flick results and the AI Coach panel (tips and trends).
8. Click sensitivity + / - and confirm the mouse look speed changes; restart the game and confirm it is remembered.
