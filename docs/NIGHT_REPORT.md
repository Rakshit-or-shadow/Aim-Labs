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
