# RAYSHOT UI Spec (derived from ArtSource/FigmaUI style guide)

Source pages viewed: 01_cover, 02_getting_started, 03_foundations, 04_buttons_controls, 05_navigation_tabs. No backgrounds/ or icons/ folders exist. The kit is a STYLE GUIDE: angular HUD, chamfered corners, neon-on-near-black, glow. Original RAYSHOT names only.

Hex values below were read from 03_foundations at small size; treat all as "est." (estimated from tiny labels/swatches) and eyeball once in engine.

## 1. Primitive palette (est.)
| Name | Hex | | Name | Hex |
|---|---|---|---|---|
| Red/300 | #FF8A84 | | Ink/950 | #050505 |
| Red/500 | #F75049 | | Ink/900 | #0E0E17 |
| Red/700 | #8E2A26 | | Ink/800 | #222325 |
| Red/900 | #2A1214 | | Ink/700 | #2E3036 |
| Cyan/300 | #B3FBFF | | Navy/500 | #192838 |
| Cyan/500 | #5EFEFF | | Steel/300 | #D6D0D0 |
| Cyan/700 | #1E7F86 | | Steel/400 | #B0C8C9 |
| Cyan/900 | #0C2629 | | Steel/600 | #747474 |
| Yellow/300 | #FFC672 | | White / Black | #FFFFFF / #000000 |
| Yellow/500 | #FBC706 | | Violet/500 | #9020F5 |
| Yellow/600 | #F0B537 | | Indigo/700 | #211B78 |
| Yellow/700 | #8A6A0C | | Blue/500 | #2570D4 |
| Yellow/900 | #2A2204 | | Orange/500 | #F8932E |
| Green/300 | #AED5B2 | | Teal/500 | #00FFE6 |
| Green/500 | #1DE083 | | Green/700 | #127A47 |
| Green/900 | #082318 | | | |

Alpha tints: Red/A10, A30, A60 (#F75049 at 10/30/60%); Cyan/A10, A30, A60 (#5EFEFF); Yellow/A10, A30, A60 (#FBC706); Green/A10, A30, A60 (#1DE083). Kit rule: translucency lives in tint tokens, never baked into a fill's paint opacity.

## 2. Theme tokens (target for RAYSHOT, 4 themes)
Shared across themes (est.): bg/base #0C0D14 (near-black navy), bg/raised #121218, bg/sunken #050505, bg/scrim #192838 at ~80%, text/body #B0C8C9, text/muted #747474, text/white #FFFFFF, text/on-accent #0E0E17, border/faint #222325, border/neutral #4A4C52.

| Token | RED (default UI) | CYAN (info/secondary) | YELLOW (PB/rewards/warn) | GREEN (success) |
|---|---|---|---|---|
| accent/primary | #F75049 | #5EFEFF | #FBC706 | #1DE083 |
| accent/strong | #FF8A84 | #B3FBFF | #FFC672 | #AED5B2 |
| accent/dim | #8E2A26 | #1E7F86 | #8A6A0C | #127A47 |
| accent/secondary | #5EFEFF | #FBC706 | #5EFEFF | #FBC706 |
| bg/base | #0C0D14 | #0C0D14 | #0C0D14 | #0C0D14 |
| bg/raised | #121218 | #121218 | #121218 | #121218 |
| bg/sunken | #050505 | #050505 | #050505 | #050505 |
| bg/tint | #2A1214 | #0C2629 | #2A2204 | #082318 |
| bg/accent-subtle | #F75049 @12% | #5EFEFF @10% | #FBC706 @10% | #1DE083 @10% |
| bg/accent-hover | #F75049 @30% | #5EFEFF @30% | #FBC706 @30% | #1DE083 @30% |
| bg/scrim | #192838 @80% | same | same | same |
| text/primary | #F75049 | #5EFEFF | #FBC706 | #1DE083 |
| text/strong | #FF8A84 | #B3FBFF | #FFC672 | #AED5B2 |
| text/highlight | #5EFEFF | #FBC706 | #5EFEFF | #FBC706 |
| text/body | #B0C8C9 | #B0C8C9 | #B0C8C9 | #B0C8C9 |
| text/muted | #747474 | #747474 | #747474 | #747474 |
| text/on-accent | #0E0E17 | #0E0E17 | #0E0E17 | #0E0E17 |
| border/default | #F75049 (1px) | #5EFEFF | #FBC706 | #1DE083 |
| border/subtle | accent @30% | accent @30% | accent @30% | accent @30% |
| border/faint | #222325 | #222325 | #222325 | #222325 |
| border/highlight | #5EFEFF | #FBC706 | #5EFEFF | #FBC706 |
| status/danger | #F75049 | #F75049 | #F75049 | #F75049 |
| status/success | #1DE083 | #1DE083 | #1DE083 | #1DE083 |
| status/warning | #FBC706 | #FBC706 | #FBC706 | #FBC706 |
| status/info | #5EFEFF | #5EFEFF | #5EFEFF | #5EFEFF |
| effect/glow | #F75049 @~40% | #5EFEFF @~40% | #FBC706 @~40% | #1DE083 @~40% |

Theme use in RAYSHOT: RED = default UI (menus, chrome, buttons); CYAN = info/secondary (stats, labels, coach tips, hints); YELLOW = personal bests, rewards, warnings (NEW PERSONAL BEST, low time); GREEN = success (hit confirms, goal met, positive deltas). Implementation idea: an FGETheme struct holding one FLinearColor per token with 4 static instances; HUD code fetches tokens from the active theme and a region (e.g. PB banner) may draw with another theme. No hardcoded colors.

## 3. Typography
Fonts: Rajdhani (UI voice, Medium/SemiBold) and Share Tech Mono (data). Mostly UPPERCASE with positive letter spacing for labels.
| Style | Font | Size/line |
|---|---|---|
| Display/XL | Rajdhani SemiBold | 64/80 |
| Display/L | Rajdhani SemiBold | 60/64 |
| Heading/H1 | Rajdhani SemiBold | 48/44 |
| Heading/H2 | Rajdhani SemiBold | 32/36 |
| Heading/H3 | Rajdhani Medium | 24/28 |
| Title/Card | Rajdhani Medium | 20/24 |
| Body/L, M, S | Rajdhani Medium | 18/24, 16/22, 14/18 |
| Label/L, M, S | Rajdhani SemiBold | 16/20, 14/16, 10/11 |
| Micro/Tag, Micro/Fluff | Rajdhani SemiBold | 8/10, 6/8 |
| Data/L, M, S | Share Tech Mono | 24/28, 14/18, 10/12 |
Sizes are est. Scale with screen height; use GetSizedFont for crisp text. If the fonts cannot be imported, fall back to the engine font with caps and letter spacing.

## 4. Spacing, stroke, effects
- Spacing, 4px grid: 2XS 2, XS 4, SM 8, MD 12, LG 16, XL 24, 2XL 32, 3XL 48, 4XL 64 (est.).
- Strokes: hairline 0.5 for fluff, 1px components, 2px focus.
- Chamfer: 6-14px cut. Actions (buttons): bottom-right corner. Menu items/nav: top-right corner. Cards/inputs chamfered too. Tier tabs: small bottom-right cut (est.).
- Glow: GLOW/SOFT, GLOW/STRONG, GLOW/ALT (secondary colour), GLOW/TEXT (text bloom as on the cover title), BLUR/GLASS (translucent panel). Glow = accent-coloured soft outer shadow (~8-16px) on hover/active and headline text.
- Scanlines/HUD fluff: the kit has no literal scanline layer; it uses thin HUD lines with a notch and a short bright accent segment at top and bottom of the screen, small mono data strings, a barcode strip, a version tag top-right and a diamond logo top-left. For RAYSHOT add an optional very faint (3-5% alpha) 2px horizontal line pattern.
- Background: dark cityscape photo under heavy dark tint with red haze (cover); flat bg/base on content pages.

## 5. Components
**Buttons** (Primary / Secondary / Ghost; two sizes about 140x40 and 180x52; caps label + chevron; bottom-right chamfer). States Default / Hover / Pressed / Disabled.
- Primary: Default solid accent/primary with on-accent text; Hover lighter accent/strong + glow; Pressed uses accent/secondary (cyan) fill; Disabled accent/dim fill, dim text.
- Secondary: Default bg/accent-subtle fill + 1px accent border; Hover glow + stronger tint; Pressed secondary-colour border and text; Disabled faint border, dim text.
- Ghost: Default text only with chevron; Hover bg/accent-subtle; Pressed bg/accent-hover + border; Disabled dim text.
- Icon button: ~40px chamfered square; default tinted, hover border, active solid accent with dark icon, disabled dim.
**Icons**: 16px stroke glyphs: chevron right/down/left, close, plus, check, external link, diamond, warning, lock, list, user, mail, signal bars, crosshair, menu.
**Checkbox**: ~16px square, 1px border, caps label; checked = accent border with filled inner square; disabled dim.
**Toggle ON/OFF**: two-segment control; active half filled accent with dark text, inactive half outlined.
**Slider**: small caps label top-left, mono value top-right (025/050/075/100), 1px track, accent fill, vertical-bar thumb with glow.
**Nav item** (pause-menu): wide bar ~260x40, top-right chamfer, mono index "01" + caps label + trailing chevron. Default outlined; Hover tinted fill + chevron; Active solid accent fill, dark text, glow; Disabled faint grey.
**Header tabs**: icon + caps label; inactive grey with faint underline; active accent icon box + label + 2px accent underline.
**T1-T4 tier tabs**: ~36px chamfered squares; inactive faint, hover outlined, active solid accent.
**Page indicator**: "01 / 18 PAGE" with a short accent bar on a hairline.
**Diamond category tile**: ~80px square rotated 45 degrees, upright icon, caps caption below; default outline, hover tint + glow, active solid accent with dark icon.
**Panels/cards**: bg/raised (grey ~#222325 on the guide page), 1px border/subtle (teal-grey), 6-10px chamfer, optional dashed outer frame; numbered rows ("01" yellow, cyan caps title, muted body).
**Frame lines**: 1px accent/dim line with a small chamfer notch and a bright ~80px accent segment; use at screen top/bottom and as a heading underline.

## 6. Layout ideas per Figma page
- 01 cover: huge glowing caps title left (about 15% from left), lighter subtitle below, small cyan eyebrow above, muted paragraph, version tag top-right, HUD lines top/bottom, barcode/serial fluff bottom-left, dark photo with vignette.
- 02 getting started: vertical stack of full-width panels with number + title left and body text; yellow eyebrow "02 // GUIDE".
- 03 foundations: eyebrow "01 // SECTION", heading, one-line description, hairline, then content grid (4 theme columns).
- 04 buttons/controls: dense grids on dark panels with dashed outlines; sliders in a 4-up row.
- 05 navigation: vertical nav list in a panel; tabs row; diamond tiles in a row; page counter.

## 7. Screen mapping (RAYSHOT)
| Screen | Layout | Components / theme |
|---|---|---|
| Main Menu | Cover layout: dark backdrop (city image or floating-target showcase), big glowing RAYSHOT title left, subtitle, vertical nav list under it (6 items, indexed 01-06), version tag top-right, HUD lines top/bottom, serial fluff | Nav items (active solid), frame lines, Display/XL + GLOW/TEXT; RED |
| Mode Select | Eyebrow "02 // MODE" + H1; 4 diamond tiles or wide cards, one per mode, best score in Data/L, T1-T4 tabs if difficulty tiers are used; Back ghost bottom-left, Play primary bottom-right | Diamond tiles, panels, tier tabs, page indicator; RED, bests in YELLOW |
| Settings | Header tabs (Gameplay / Audio / Crosshair); rows with sliders, toggles, checkbox; crosshair preview panel on the right; Apply/Back buttons | Sliders, toggles, checkboxes, header tabs, secondary/ghost buttons; RED with CYAN hints |
| Results | Big heading, stat panels with Data/L numbers (accuracy, hits, avg), NEW PERSONAL BEST banner, Play Again (primary) / Menu (secondary) | Panels, Display text, buttons; YELLOW banner for PB, GREEN for improvements, RED chrome |
| Pause | Scrim (bg/scrim) over frozen scene, nav list (Resume, Restart, Settings, Main Menu) with chevrons | Nav items, scrim, frame lines; RED |
| In-game HUD | Minimal: timer top-center Data/L, score/accuracy in Data/M with micro labels, very faint frame lines at edges, countdown Display/XL with glow, popups Label/L, crosshair unchanged | Type + frame lines only; CYAN stats, YELLOW low-time/PB, GREEN hit marker |
| Training Center menu | Left nav of options, right panel with description and bot/difficulty selectors | Nav items, T1-T4 tabs for difficulty, toggles, panels; RED, CYAN info |
| Flick results | Results layout plus per-flick breakdown rows (mono data, mini bars for reaction/overshoot) | Panels, Data/M rows, slider-style bars; CYAN data, YELLOW best |
| Coach panel | Getting-started style numbered rows: yellow "01/02/03", cyan caps title, muted body; 3 tips | Numbered panel rows, CYAN theme, border/subtle |

## 8. Implementation notes
- Canvas drawing in AGEHUD: chamfered rect via triangles or two rects plus a corner triangle; glow via 2-3 expanded low-alpha outlines; colours from a central theme struct.
- text/muted (#747474) on bg/base is about 4:1 contrast; use for secondary info only.
