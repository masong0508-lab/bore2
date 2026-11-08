# BORE 2 interface flow (PS1 and Dreamcast; the DS can copy the model)

One navigation model for every menu, so a player learns it once. Shots: `docs/ui_ps1_*_before_after.png` (before | after), `docs/ui_dc_*.png`.

## The model

| button | does |
|---|---|
| D-PAD up / down | move the cursor (wraps). Held: repeats, faster the longer it is held |
| D-PAD left / right | change the chosen value; on a list, a page at a time (jukebox) |
| X | confirm, or step a value forward |
| O | back out **one** level (a sub-screen goes to its menu, a menu goes to the game) |
| START | close everything, back to the game (or the title) |
| L1 / R1 | previous / next page (tabs) |
| L2 / R2 | jump to the previous / next group in a long list |
| SQUARE | reset the page (OPTIONS) / the screen's own second action, always named in the hint bar |
| SELECT | the phone |

Hold-to-repeat: the press fires at once, the first repeat after 20 vblanks (a third of a second), then every 8 vblanks, every 4 after one second, every 2 after two seconds. It counts vblanks, so the pace is the same at 20 frames a second or 60. Code: `nav_update`, `NAVP(b)` in `src/main.c`.

Sounds (one small set, `audio_ui(k)` in `src/audio.c`, the game's voices at a high pitch and low volume): move tick, page chirp, confirm click, back, no, value changed. Played by the main loop for every menu: a d-pad move ticks, a state change with O / START plays back, with X plays confirm, a menu opening from play chirps.

Hints bar (`ui_hints`): the buttons that work on the chosen row, in this order: the page keys, the action (`X DO IT`, `DPAD CHANGE`), `O BACK`, `START CLOSE`.

Panels: `ui_window` (blue header with the screen's icon, name left, page name as a pill on the right), a card inside the safe area (8 px from each edge), plain short text, rows 17 to 19 px high, label left and value right, a scroll bar when a list does not fit, `n of m` under the rail.

## OPTIONS (src/main.c, "OPTIONS")

A rail of pages down the left (L1 R1), rows on the right, a plain line about the chosen row under them, the hints bar under that. The page and the row of each page are remembered while the game is on.

| page | rows |
|---|---|
| GAME | needs, your Bore alone, ghosts, message time, thought bubbles, reset (room for new rows: the cheat traits row goes here) |
| SCREEN | PICTURE: preset, view distance, full detail, far buildings, far trees, haze. PEOPLE: crowd size, full 3D, sprites. LIGHT: shadows, floor shading, wall light, dithering. SCREEN: screen shape, frame rate, speed helper, show FPS, text look. reset |
| SOUND | master, music, sound volume, title music, game music, shuffle, reset |
| CONTROLS | camera, height, turn speed, zoom tilts, wall view, reset; BUTTONS: a read-only guide |
| TIME | day length, lifespan, old age, town grows up, real time, clock, set the clock (PS1), reset |
| SAVE | save game, load game (the same slot list as the pause menu), reset all options (X twice) |
| EXTRAS | aggression, who joins, rage, who lunges, peacemakers, traffic cheat, Boreagotchi time, reset |
| ABOUT | game, console, memory (2 MB or 8 MB), frame rate when you last played, save format, how to play |
| DREAMCAST | (Dreamcast only, after ABOUT) the gallery and the Dreamcast switches |

A knob is a row of the page table `optTab` plus an entry in the `O_` enum, `optNm`, `optV`, `optN`, `optBlurb` (one plain line) and `OPT_DEFAULTS`. Row kinds: choice (`< value >`), slider, switch, button (`X`), information, group name (not selectable). Live preview: a change to the picture redraws the world behind at once; the world is otherwise kept still. A change shows its news (a green tick card) in the description box: "Preset High: picture set", "Screen page reset".

## Responsiveness (how it works)

* The menus used to redraw the whole city behind the panel every frame (about 700 hblanks): 20 frames a second, so a press could wait three frames. Now `menu_bg()` draws the city and the dimming for two frames (one for each half of the double buffer) and then keeps them: the next frames only repaint the panel on top (`gfx_keep`: no clear, no world; `ui_still`: no new shadows or dimming, which would pile up). A frame is one or two vblanks.
* A menu whose card changes size (the phone's) asks for a fresh backdrop when it changes; the phone also draws live while it slides up.
* Friendly-case lookup (Title and Sentence case) scanned a word list for every word of every text: nearly half a menu frame. It is now a hash table (`font.c`).
* The Dreamcast builds its picture afresh every frame (50 fps) and keeps nothing: `menu_bg` always draws live there.

## Other screens in this model

Pause, phone (apps grid, three across), travel, jobs, homes, crafting, goals, Bore panel, switch sim, help, save slots, jukebox (scroll bar, left / right a page), the people list: kept backdrop, held d-pad, the sounds. Build mode keeps its own cursor (it moves a tile in the 3D lot); the catalogue is tabs of items (R3 next tab, L1 R1 pick) with the dots that show the tab.

## Test (tools: `-DAUTO_CITY=1 -DAUTO_UI=1`)

`ui_test_tick` in `src/main.c` scripts the pad through the pause menu, the phone, OPTIONS, build, the jukebox and the people list, and counts per menu: frames, hblanks of CPU (average and worst), vblanks per frame, presses sent and how many moved something, and what holding a button for two seconds moved. Read `ui_res` (10 ints a menu) from the RAM dump; `-DUI_STAY=1` leaves the pause menu open for the profiler.

### Numbers (PS1, `AUTO_CITY=1 AUTO_UI=1`, city street, same script; hb = CPU hblanks a frame, vb = vblanks a frame; hold = moves in 2 s of a held button)

| menu | before hb avg / worst | before vb | before hold | after hb avg / worst | after vb | after hold |
|---|---|---|---|---|---|---|
| pause | 887 / 1062 | 3.00 | 1 | 148 / 797 | 1.04 | 20 |
| phone | 973 / 1121 | 3.00 | 2 | 240 / 802 | 1.28 | 38 |
| options | 1126 / 1469 | 3.00 | 2 | 164 / 894 | 1.03 | 20 |
| build | 1507 / 1799 | 3.00 | 3 | 1005 / 1260 | 3.00 | 5 |
| jukebox | 1651 / 20534 | 3.00 | 1 | 458 / 19602 | 1.06 | 20 |
| people | 923 / 2155 | 3.00 | 1 | 344 / 1598 | 2.01 | 20 |
| city (no menu) | 695 / 954 | 3.00 | | 653 / 855 | 3.00 | |

The worst frame of each menu is its first two (the backdrop). The jukebox's 19,600 is a song loading from the disc (R1 skips). A menu frame is now one vblank (60 per second) instead of three. Presses sent and moved: every press moved something except values at the end of a slider. The city frame is no slower (the text speed-up helps it a little).
