# BORE 2 DSi: roadmap

Where we are (D10): dual-screen Sims-DS style flow. Title (Household Occupants / Start-Load), Create-a-Sim (12 hairstyles,
beards, faces, stylus pad, RGB paint, 26 sliders face and body, Height, Max x1/x2/x3 cheat), Map with a drag neighborhood
editor, LIVE (needs, actions, wandering sims), drag Buy/Build, 20 modelled furniture items.
Not done: saving, most PS1 systems, sound, real-hardware testing.

## Phase A: make what exists solid (next, 1-2 sessions)
1. Test every screen on real DSi / melonDS GUI (title, CAS pad and sliders, drag buy/build, map editor). I only checked parts in the headless emulator.
2. Save/load: lot contents (floors, walls, objects), sims (looks + sliders + needs), funds, map edits, cheat setting. Use FAT (DSi SD) or SRAM-less file via libfat.
3. Sims in the lot: draw the finer face sliders (eyes, nose, mouth, jaw) on the in-lot model; walk/animate limbs; sit/sleep poses on the new furniture.
4. Live mode polish: wall cutaway by camera angle, window and door models, roof toggle, day/night tint, shadows under sims, object hover highlight.
5. Action menu on the new items (it uses the old 4x5 table; each of the 20 items needs correct actions and a use-spot, e.g. sit on the sofa seat, sleep in the bed).
6. Fix text: kerning, a 2nd font size, scrolling lists.

## Phase B: core Sims loop (3-5 sessions)
1. Needs that actually matter: mood bar affects speed/actions, low needs force actions, game over / sad states.
2. Money: bills, a job (careers from PS1 `src/places.c` / jobs), paycheck, carpool.
3. Time: calendar, day/night, speed buttons (1x/2x/3x), sleep skip.
4. Autonomy: sims pick actions by need (from `wants.c` wants and fears), queue of actions with cancel icons (like Sims 3 DS).
5. Relationships: talk, joke, hug, friends list, relationship bars; family tree.
6. More sims per household (3 slots exist), swap with the plumbob buttons, create more in CAS.

## Phase C: bring over from the PS1 game (ordered by value / cost)
| PS1 feature | source | DSi cost | Notes |
|---|---|---|---|
| Buy catalogue: 8 tabs, ~100 items, second pack of 46 | items.c, items_more.inc, tools/make_items2.py | medium | Generate a compact item table; each needs a model (see below) |
| Build tools: room, doors/windows, floor, wallpaper, roof, 5 storeys | world.c, main.c | medium | Room tool + door/window next; storeys last |
| San Boreas map: towns, 19 place types, hours | bigmap.c, places.c, world.c lot_* | already shared (bigmap.c); lots need layouts | Start with 4 lot types: shop, diner, park, gym |
| Jobs / careers (14) | places.c, story.c | small | Table-driven, text and numbers |
| Wants and fears | wants.c | small | Shows as thought icons; fits Sims 3 DS wishes |
| Growing up, families, babies, adopting | pop.c, main.c | medium-large | Need age-based models (kid/baby/elder scales: the Height slider helps) |
| Home buying and travel | home.c | small-medium | Uses map editor ownership |
| Castaway Island | island.c, nature.c | large | Separate world mode; DS-friendly terrain |
| Crowds, parties | crowd.c | medium | Sprite crowds suit DS |
| Music: band, DJ, jukebox | band.c, dj_ui.c, audio.c | large | Needs ARM7 sound; start with simple tracker |
| Skating, arcade, fight, heist, gotchi, farm | skate/arcade/fight/heist_ui/gotchi/farm_ui | large each | Mini-games: one at a time, port last; arcade and gotchi first (small) |
| Dog | dog.c | small-medium | Another little model |
| Story / goals | story.c | small | Goal list screen on the bottom |
| Ghosts, traffic | ghost.c, traffic.c | small | Cosmetic world life |

## Phase D: DSi-specific
- Use the 16 MB RAM and DSi camera (face to hair/skin colour? optional), 2x CPU clock (set in header flags).
- Wi-Fi visits / trading Sims between DSis (stretch).
- DSi menu icon, banner, proper header (ndstool banner), soft-reset, sleep mode.

## Phase E: the PS1 side (still open)
Hills/mountains and decor visible from the air, crash fixes, proportion pass, next packs, save and crash flow testing.

## Order I'd do next
1. Item actions + use-spots for the 20 modelled items.  2. Save/load.  3. In-lot face detail + limb animation.
4. Room tool and door/window.  5. Jobs + money loop.  6. Wants/fears.  7. Lot types (shop, diner).  8. Mini-games.
