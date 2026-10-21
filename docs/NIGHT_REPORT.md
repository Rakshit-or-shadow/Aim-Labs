# Night Report

## Decisions I made

- Phase 2: shots are not blocked by walls (no line traces allowed); recoil sets control rotation directly; 4 AGETargetDummy spawn in Lvl_IntroRoom.

## Manual steps for Rakshit

## Phases
- Phase 0: DONE (commit 2107d80) - C++ base, HUD crosshair, BPs reparented.
- Phase 1: DONE (commit af31493) - Tactical movement: run 540, Shift walk 40%, Ctrl crouch, low air control, high accel/braking, GetCurrentSpeed()/IsOnGround(), F3 speed readout. Build: Succeeded. Manual steps: none (untested in play).
- Phase 2: DONE (commit d6f49c2) - 5 weapons (Sidekick, Rattler, Longarm, Eagle Eye, Scatter), RayTraceMath hit detection, viewmodel, flash, tracer, sounds, HUD. Build: Succeeded. Manual steps: none (untested in play).
