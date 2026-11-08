# BORE 2: what to do next
Per port: PS1_ROADMAP.md, DSI_ROADMAP.md (end), DREAMCAST.md (end). Expansion packs and placeholders: EXPANSION_PACKS.md.

Plain list, most important first. Tick a line when it is done.
Detail for each port: DSI_ROADMAP.md (DS), DREAMCAST.md (Dreamcast), PS1_PERFORMANCE.md (PS1 speed).

## Saved but not finished (work in progress branches on GitHub)
- [ ] `wip-ds-fight`: DS fight game (BORTUA FIGHTER): new files ds_fight.c, ds_fight_logic.h, mk_dsfight.py. Finish it, test in melonDS, make the next dNN ROM, add the arcade's fifth cabinet.
- [x] ARCADE pack (PS1): done in main. 14 games (5 reworked, 9 new), floor, prize counter, challenges, high scores, medals, ARCADE2.OVL slot, save v46. Left: see docs/EXPANSION_PACKS.md.

## 0. Streets: nothing on a road (Nov 8, all ports)
- [x] **The bug the user sent twice** (`docs/dc_furniture.png`, the old picture): it was the Dreamcast furniture test hook (`-DDC_SHOWITEMS`), which set the six items down in a row on the row south of the Sim, and the test Sim stood on a crossing, so a swirl rug, a console, the cabinet and both lamps stood in the road (the old picture is that test; the real generator was already almost clean). Now the hook puts them on the nearest free plot floor and holds the Sim there (`world_tile_class`). Before and after at the same crossing, PS1 and DS too: `docs/street_props_before_after.png`; new furniture picture: `docs/dc_furniture.png`.
- [x] **One definition of road / kerb / plot** (`src/world.c`: `road_rects`, `world_tile_class`: TC_ROAD, TC_KERB, TC_PLOT, from the map's own road bits, a river block's bridge counts as road). `street_purge` runs after every block (generators, kept tiles of old saves): nothing stays on a road tile. It replaces `street_clean` (which only took lamps and bins off). The yard bench that slipped to the road edge is fixed at its source.
- [x] **Road test** (`ports/dreamcast/test/roadaudit.sh`, `-DROAD_AUDIT=1`): makes every block of every town and city, every highway and a grid of open country (1955 windows, about 31,300 blocks, 589,477 things) and fails if anything stands on a road tile, if a street strip is not painted road, or if road is painted where the map has no street. Before: 1 on a road (a bench in a house's yard); after: 0, 0, 0, PASS. With the clean-up off (`-DROAD_NOPURGE=1`) it also gives 0 now (the generators are clean); before the bench fix the same run FAILED with 1. DS: `-DDS_ROAD_AUDIT` makes all 3,837 lots of every town: 61,960 things, 0 off the 13 x 13 plot (the street is outside it).
- [x] Dreamcast lamp shades: a collar on the pole and a shade that narrows to the top (two boxes), not a cube on a stick.
- [ ] Left: the camera's perspective (poles near the screen's edge lean, the same on every port; a longer lens would straighten them but changes the framing of everything). A vehicle the player parks, or a thing placed in build mode on the lot's own pavement row, is the player's choice and is not checked. The PS1 test build has no room for the sweep (AUTO_CITY alone leaves under 5 KB), so the test runs on the Dreamcast build; the code is the same.

## 1. PS1 (do first, small and visible)
- [ ] Floor textures swim near the camera: the last speed round loosened the texture-warp cutting. Put the strict setting back for normal play (a loose one only as the speed helper's last step, far from the camera). Check with a frozen frame, moving a tile at a time.
- [ ] City still about 20 fps on the Sims, zoomed-in and zoomed-out cameras (first person is 27). Goal 25. About 130 hb is missing from a 650 hb frame. Ideas: pre-built geometry per block on 8 MB, a cheaper tile loop (about 290 hb), a cheaper player model (about 85 hb), hide blocks behind buildings.
- [ ] Clock card is not kept between frames (no packet room).
- [x] Boreagotchi 3 (save v48): needs (energy, clean), moodlets, wishes, goals and streaks, 8 mini-games (WHACK is the newest) with skills, scripted first day, surprises, 32-line diary. See docs/GOTCHI_V48.md. Old v40/v41 saves load (test/oldsave.sh). Left: v45-47 save test on a real card, DC port of the new rules, sounds are pitched voices only. DS port done at D64 (bore2_dsi_d64.nds, see docs/DSI_ROADMAP.md): everything except tantrums, 20 kinds / 6 forms and per-pet memories by id. BRAINS manners bonus in train is done.
- [x] Boreagotchi first-run art on PS1 (title, OPEN IT! gift box and toy reveal, egg card, hatching, new starfield, cue sounds). See docs/GOTCHI_V48.md and docs/GOTCHI_FIRST_ART.md; shots docs/gotchi_first_after_1.png to _3.png. Left: the DS port (reuse src/gotchi_first_art.h or docs/gotchi_first_art.json), the egg you carry in the pet view still uses the old dot art, sounds are pitched voices only (no new samples), Dreamcast not checked.
- [x] Boreagotchi eggs redrawn on PS1 (pet view, basket, home, tree, guide, toy LCD, hatching: one design, three sizes, seven patterns, crack stages, wobble; data in docs/gotchi_first_art.json for the DS). Shots docs/gotchi_egg_before_*.png and gotchi_egg_after_*.png, notes in docs/GOTCHI_V48.md and docs/GOTCHI_FIRST_ART.md. Left: splice the same eggs into the DS Boreagotchi (wip-gotchi), and the 3D toy prop's 8x8 egg texture (items) still uses the old picture.
- [x] Sims-style fonts and centred text on PS1 and Dreamcast (save v49): six styles (BODY, READ, HEAD, NUM, FUN, BIG) in `assets/FONTS.BIN`, font-select API, measure-then-centre helper (`gfx_text_box`, `ui_text_box`), friendly Title/Sentence case, OPTIONS > SCREEN > TEXT LOOK (FRIENDLY / EXTRA CLEAR / CAPITALS), Sims-style toasts, no blinking title hint, logo left out behind options (their text was being cut off by the packet buffer). Shots docs/fonts_*.png, notes docs/FONTS.md. Left: a pass over the overlay screens not shot (shops, party, super, dog, prison, space, farm, skysea, paradise), the thought bubble tail, align the DS with FONTS.md.
- [ ] Old-save load test for the Boreagotchi save (version 45) and REAL TIME (version 44) on a real memory card.
- [ ] Bus facing at stops, car pitch on real hills, first-person and gas-station vehicle shots.
- [x] Boreagotchi PS1 text overlaps fixed (commit 9b4fc5c). Test hook -DAUTO_GVIEWS and test/gviews.sh step through every screen; the disc image does not rebuild when only an overlay changes, the script forces it.

## 2. DS
Newest ROM: bore2_dsi_d67.nds (d66 plus the new Create-a-Sim in the way of The Sims 3 for the 3DS). Everything is melonDS-only (no real DS or DSi tried).
- [x] D67 Create-a-Sim like The Sims 3 (3DS): six steps (Basics, Looks, Hair, Outfit, Traits, Done), name keyboard, Looks pads and sliders, 26 hair styles, dye and paint, hats, 12 traits, 5 lifetime wishes, summary and confirm; 3D Sim on a stand with a camera that follows the step. Old saves load (new 'CX' block). Notes docs/CAS_SIMS3DS_DS_NOTES.md, shots docs/ds_shots/cas_*.png. Left: Capture Face, outfit categories, shoes, eye colours, real DS.
- [x] D65 fonts and centred text: five 1-bit faces (BODY, HEAD, SMALL, NUM, TITLE) with a font-select API, a measure-then-centre helper (`text_box`, `fit_font`) used on every screen, plumbob, thought bubble, mood words, toasts, playful phrasing; dyslexia-safe glyphs (serifed I, slashed zero). Before | after shots docs/ds_shots/fonts_*.png. Text only, 30 fps as D64, old saves load. Left: real DS, a few Boreagotchi sub-screens on hand offsets, arcade in-game text beyond Breaker, align with the PS1 docs/FONTS.md. See docs/DSI_ROADMAP.md.
- [ ] Skate plaza is not a steady 30 (26-30 in spots).
- [ ] Textures on bought furniture and the Map's house picture.
- [ ] BORTUA FIGHTER (see wip-ds-fight).
- [ ] Band and DJ: needs a two-deck audio path first.
- [ ] `wip-gotchi`: DS Boreagotchi rules are written in ds_games.c (never built); the new screens are in ports/dsi/wip/ui_update.c, ui_top.c, ui_bottom.c, not spliced in. Splice in, add view names and icons, draw hats, build, fix errors, test scenes, shots, 30 fps, make d55. Missing before that work: new foods and games, family home, accessories, awards, diary, playdates, grandparents on the tree, the Toy Box and the shop. Fix the clipped labels on the family page (docs/gotchi_ds_tree.png).
- [x] New arcade games on the DS: all 13 PS1 games run on the DS (d57), 30 fps. Left: Bortua Fighter, two-player, challenges.
- [ ] A DSi-type ROM for the extra memory and CPU (cannot be tested here).
- [ ] Try on a real DS or DSi.

## 3. Dreamcast
About 31-52 fps in the city in the emulator. 60 needs under 262 hblanks per frame (now about 420-500).
- [ ] Speed: a direct PVR path for the world and people; SH-4 FIPR/FTRV matrix maths; sound moved to the AICA; cheaper vertex writing (23%).
- [ ] Upgrades: sub-pixel vertices (needs BORE_DC changes where the game builds whole-pixel packets), a real depth buffer, far view with fog, bigger crowds, smooth lighting, 16:9.
- [ ] Side-by-side checks against DuckStation: pie menu, night, first person, far view, a park, roofs on shops, towers and flats.
- [ ] Extras: Boreagotchi rewards carrying back from the VMU, Dreamcast furniture set and outfits, translucent water and glass, night glow, unlockables gallery, pads 3 and 4, second player in the mini-games, a test with a real second pad.
- [ ] Re-check that the PS1 image is the same with and without the Dreamcast edits (ports/dreamcast/ps1_check.sh).
- [ ] Try on a real Dreamcast.

## 4. Arcade (PS1, then DS)
- [x] Improve the 5 cabinets, add 9 new games, arcade lot, prize counter, rewards (PS1 done; DS has the games, scores and prizes). Left: DS lot extras (token machine, wall board, townsfolk).

## 5. Held for later
- [ ] Missy's voice-over and FMV.
- [ ] Thought bubbles on the DS.
- [ ] DS stand-ins needed when sim.c comes into the DS port.
- [ ] Crowd model build spikes (occasional slow frames).
- [ ] Android emulator tests.

## Tools you will need again (they live in /tmp and are lost when a session ends)
- Dreamcast: `ports/dreamcast/toolchain/restore.sh` (branch `dc-toolchain`).
- DS: `tools/dsbuild/restore.sh` (branch `ds-toolchain`).
- PS1: PSn00bSDK, DuckStation regtest (`test/ds.sh`), `tools/perf/*`.

## Rules to keep
- Keep more than 10 KB free past `_end` on a 2 MB PS1; check 8 MB too.
- PS1 test builds must be Release.
- Commit and push with `git pull --rebase` first.
- Old saves must keep loading (bump VERSION in src/save.c for new blocks).
- Keep the game's logic deep: people who know the player always get full thinking.

## Getting each ROM (GitHub Actions: repo > Actions tab > pick a workflow > Run workflow)
- `build` : PS1 disc (BORE2.chd, BORE2-base.chd, patches, memory report).
- `build-ds` : the DS ROM bore2_dsi_<commit>.nds.
- `build-dreamcast` : the Dreamcast disc bore2_dc.gdi (+ .elf).
The DS and Dreamcast ones were written but NOT run yet (no way to run Actions from the build session): if one fails, open its log; the likely causes are the apt package names, the restore script, or the Dreamcast make needing files the disc step expects. Each unpacks its saved toolchain from the `ds-toolchain` / `dc-toolchain` branch.
