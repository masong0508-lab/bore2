# BORE 2: what to do next

Plain list, most important first. Tick a line when it is done.
Detail for each port: DSI_ROADMAP.md (DS), DREAMCAST.md (Dreamcast), PS1_PERFORMANCE.md (PS1 speed).

## Saved but not finished (work in progress branches on GitHub)
- [ ] `wip-ds-fight`: DS fight game (BORTUA FIGHTER): new files ds_fight.c, ds_fight_logic.h, mk_dsfight.py. Finish it, test in melonDS, make the next dNN ROM, add the arcade's fifth cabinet.
- [ ] `wip-arcade`: PS1 arcade rework (src/arcade.c, arcade_res.c, audio.c, CMakeLists.txt). New games and cabinets, partly done. Finish, test in DuckStation, check RAM (more than 10 KB free past _end on 2 MB), bump the save VERSION, merge save.c with the Boreagotchi change.

## 1. PS1 (do first, small and visible)
- [ ] Floor textures swim near the camera: the last speed round loosened the texture-warp cutting. Put the strict setting back for normal play (a loose one only as the speed helper's last step, far from the camera). Check with a frozen frame, moving a tile at a time.
- [ ] City still about 20 fps on the Sims, zoomed-in and zoomed-out cameras (first person is 27). Goal 25. About 130 hb is missing from a 650 hb frame. Ideas: pre-built geometry per block on 8 MB, a cheaper tile loop (about 290 hb), a cheaper player model (about 85 hb), hide blocks behind buildings.
- [ ] Clock card is not kept between frames (no packet room).
- [ ] Old-save load test for the Boreagotchi save (version 45) and REAL TIME (version 44) on a real memory card.
- [ ] Bus facing at stops, car pitch on real hills, first-person and gas-station vehicle shots.
- [ ] Boreagotchi: overlapping text in the family tree hint bar and its labels (docs/gotchi_4_1.png). Check every Boreagotchi screen.

## 2. DS
Newest ROM: bore2_dsi_d54.nds. Everything is melonDS-only (no real DS or DSi tried).
- [ ] Skate plaza is not a steady 30 (26-30 in spots).
- [ ] Textures on bought furniture and the Map's house picture.
- [ ] BORTUA FIGHTER (see wip-ds-fight).
- [ ] Band and DJ: needs a two-deck audio path first.
- [ ] Boreagotchi on the DS is missing: new foods and games, family home, accessories, awards, diary, playdates, grandparents on the tree, the Toy Box and the shop. Fix the clipped labels on the family page (docs/gotchi_ds_tree.png).
- [ ] New arcade games on the DS after the PS1 ones are done.
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
- [ ] Improve the 5 cabinets, add 8 or more new games, a bigger arcade lot with a prize counter and wall leaderboard, townsfolk who play and challenge you, rewards that reach skills and mood. See wip-arcade.

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
