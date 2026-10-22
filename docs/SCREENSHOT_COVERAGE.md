# Screenshot coverage

All shots are 1920x1080 from the packaged Shipping build (`RayShot.exe -ShotTour`), saved as `docs/screenshots/NN_name.jpg` (1600 px wide) plus 8 hero PNGs in `docs/screenshots/hero/`. IDs refer to `docs/FEATURE_INVENTORY.md`.

"Verified" means I looked at the image, on contact sheets downscaled to 640 px per image (not every one at full size). Small text (ammo readout, F3 readout, flick table) was not read at full resolution.

| Feature | Screenshot | Verified | Notes |
|---|---|---|---|
| A01 Main menu | 01_main_menu_red | yes | Title, 6 entries, info card, arena behind |
| A02 Selection + info card | 02_main_menu_selected_training | yes |  |
| A03 Mode select | 07_mode_select_gridshot, 08_, 09_, 10_ | yes | One shot per mode tile |
| A04 Personal bests | 33_mode_select_with_personal_bests | yes | Bests shown on tiles and card; the before-state ("NOT PLAYED YET") is in 07-10 |
| A05 Settings MOUSE | 11_settings_mouse_sensitivity | yes | Sensitivity changed to 1.5 |
| A06 Settings VIDEO | 12_settings_video_fov | yes | FOV changed to 113 |
| A07 Settings AUDIO | 13_settings_audio_volume | yes | Volume lowered |
| A08 Settings CROSSHAIR | 14_settings_crosshair_color | yes | Colour changed to white, preview updated |
| A09 Settings INTERFACE | 15_settings_interface_theme, 16_, 17_ | yes | Theme cyan, scanlines on, glitch off |
| A10 Pause (round) | 20_gridshot_pause_menu | yes | Round visible behind the card |
| A11 Pause (free roam) | 35_pause_menu_free_roam | yes | No Restart entry |
| A12 Results (aim) | 24_gridshot_results_new_best, 26_, 28_, 31_ | yes | All 4 modes with rank |
| A13 New personal best | 24_gridshot_results_new_best | yes |  |
| A14 No new best | 32_gridshot_results_no_new_best | yes | Shows the existing best (25 hits) |
| A15 Training results + Coach | 60_training_results_and_coach | yes | Rank B, 3 tips, trends (fake history fed under -ShotTour, never saved) |
| A16 Flick results + Coach | 66_flick_results_and_coach | yes |  |
| A17 Themes on main menu | 01_, 03_, 04_, 05_ (Red, Cyan, Yellow, Green) | yes |  |
| A18 Theme on in-game HUD | 21_, 22_, 23_gridshot_hud_theme_* | yes | The accent change is subtle on the HUD (stripe, timer bar) |
| A19 Scanlines | 16_settings_interface_scanlines | yes (subtle) | Faint lines, as designed |
| A20 Glitch | 17_settings_interface_glitch_off (toggle); 27_tracking_playing (low-time glitch bars) | partly | The screen-change flash is transient and was not captured on its own |
| A21 Quit | 01_main_menu_red | partly | Entry visible; the quit action cannot be captured |
| B01 Countdown | 18_gridshot_countdown | yes | GET READY 2, mode banner |
| B02 Gridshot | 19_gridshot_playing | yes | Hits, popups, timer, accuracy |
| B03 Microshot | 25_microshot_playing | yes |  |
| B04 Tracking | 27_tracking_playing | yes | Target turns green when tracked, about 97% on target |
| B05 Reflex | 29_reflex_waiting, 30_reflex_playing | yes | Wait message, then a target and reaction times |
| B06 Round HUD | 19_, 25_, 27_ | yes |  |
| B07 Popups / hit marker / burst | 19_gridshot_playing, 21_, 22_ | yes | Burst particles visible in 21/22 |
| B08 Low-time warning | 27_tracking_playing | yes | Timer 5.0 with glitch bars |
| B09 Neon arena | 06_arena_wide_view, 18_, 19_ | yes |  |
| B10 Wide view | 06_arena_wide_view | yes | FOV 120 |
| C01 Sidekick | 43_weapon_sidekick_firing | yes | Hit marker; bot far away |
| C02 Rattler | 46_weapon_rattler_firing | yes | Red hit marker |
| C03 Longarm | 49_weapon_longarm_firing | yes | Bot far away, small |
| C04 Eagle Eye | 53_weapon_eagle_eye_firing | yes | Bot far away, small |
| C05 Scatter | 56_weapon_scatter_firing | doubtful | Weapon visible, but the pellet spread is not clearly visible |
| C06 ADS | 44_, 47_, 50_, 57_ (_ads) | yes |  |
| C07 Eagle Eye scope | 54_weapon_eagle_eye_ads | yes | Scope view with a bot |
| C08 Reload | 45_, 48_, 51_, 55_, 58_ (_reload) | yes | Reloading text in the ammo readout (small, bottom right) |
| C09 Ammo readout | 43_ to 58_ | yes (small) | Small text bottom right, low contrast on the white floor |
| C10 Spread crosshair | 43_, 46_, 56_ | doubtful | Crosshair gap changes are not obvious in stills |
| C11 Hit marker / headshot / kill | 52_bot_killed_headshot | yes | Orange headshot hit marker; bot far away |
| C12 Viewmodels | 43_ to 58_ | yes | Procedural gun shape visible |
| D01 Standing | 38_movement_standing | yes |  |
| D02 Walking | 39_movement_walking_speed_readout | yes |  |
| D03 Running | 40_movement_running | yes |  |
| D04 Crouching | 41_movement_crouching | yes | Lower camera |
| D05 Jumping | 42_movement_jump_midair | yes | Higher camera, readout says Air |
| D06 F3 readout | 39_ to 42_ | yes (small) | Yellow text bottom left, faint on the white floor |
| E01 Training menu | 36_training_center_menu, 37_training_center_menu_strafing_hard_scatter | yes | Mode, difficulty and weapon highlighted |
| E02 Menu stats | 59_training_menu_session_stats, 61_training_menu_coach_panel | yes |  |
| E03 Lane, hub, cover | 38_ to 42_ | yes | 10/20/30/40 m markers |
| E04 Static bots | 43_ to 58_ | yes |  |
| E05 Strafing bots | 62_bots_strafing_hard | doubtful | Bot small / at the screen edge |
| E06 Peek bots | 63_bots_peek_hard | doubtful | Bot small / at the screen edge |
| E07 Bot hit / killed | 52_bot_killed_headshot | partly | Headshot hit marker shown; the death itself is not clear in a still |
| E08 Live stats on HUD | 59_training_menu_session_stats | partly | Session stats are in the menu; the small Kills / Acc / HS HUD line is not clearly visible in the weapon shots |
| E09 Sensitivity / DPI | 36_training_center_menu | yes (small text) |  |
| F01 Flick start | 64_flick_optimizer_start | yes |  |
| F02 Flick in progress | 65_flick_in_progress | partly | A target was just hit (headshot popup); no live target in frame |
| F03 Flick results | 66_flick_results_and_coach | yes |  |
| F04 Direction table | 67_flick_analysis_table | yes (small text) | Per-direction table and suggestion |
| F05 AI Coach | 60_, 61_, 66_ | yes |  |
| G01 Free roam | 34_free_roam_standing | yes | Bright level, weapon and ammo readout |
| G02 Saves | - | not capturable | Not a screen; the tour deliberately never writes saves |
| G03 Debug console | - | not capturable | Console only (ge.ui.gallery, ge.aim.*) |

Also captured: `68_main_menu_final`. Not in the game, so not shown: quit confirmation, score streaks, crosshair styles (only colours).
