# PS1: what it needs next (updated Nov 8)
See also ROADMAP.md, EXPANSION_PACKS.md and PS1_PERFORMANCE.md (measured numbers).
1. (Done Nov 8) Strict texture-warp cutting is back; the speed helper keeps a locked 20 (30 only after a long clean spell); housemate route searches wait for lighter frames; smooth dawn and dusk. See PS1_PERFORMANCE.md, "Round 3".
1b. (Tried Nov 8, not kept) Baking the ground into cells on 8 MB: Sims camera -18 hb, zoomed out -44, zoomed in 0, first person +19. 30 fps (524 hb) and a locked 25 are not reachable that way. See PS1_PERFORMANCE.md, "Round 5" and tools/perf/round5_floor_cells.patch.
2. Still late: 3 to 4% of Sims-camera frames run over 3 vblanks (10 to 15% with 8 MB's bigger crowd, zoomed in). The profile is flat; the cutter is 95 hb (155 zoomed in). Ideas, best first: pre-built block geometry on 8 MB; fewer cutter pieces by sharing corners between floor tiles; the household's three full models (87 hb); the townsfolk planner's spikes (240 hb); a 30 fps first person needs 100 hb less.
3. Arcade rework from branch `wip-arcade`; finish, test, bump the save VERSION, merge with the Boreagotchi save change.
4. Placeholders (COMING SOON cards, reserved slots and ids) for everything unfinished, see EXPANSION_PACKS.md.
5. Test old saves on a real memory card (Boreagotchi v45, REAL TIME v44).
6. Bus facing at stops, car pitch on real hills, gas-station and first-person vehicle shots.
7. Clock card kept between frames (needs packet room).
Always: more than 10 KB free past `_end` on 2 MB, Release builds only, old saves load.

## Nov 8 update: hair, safe area, discs
- **Hair**: AFRO, CURLS, TOP KNOT, THINNING, HORSESHOE, SIDE TAIL, BUN and the beards redrawn so each reads close and far (`docs/hair_ps1_styles.png`, all 26 styles front and three-quarter; `docs/hair_ps1_dye.png`, the 8 colours). Same piece counts, so fps and RAM are unchanged (`_end` 0x801fc5e4, about 14.5 KB free on 2 MB). Checked on 8 MB (`DS8MB=1`) and old saves (`test/oldsave.sh build`, `oldsave2.sh build "46 06393a1"`) still load. Test hooks: `-DHAIR_AUTO=24 -DHAIR_DEV=400` (every style then every colour, close camera).
- **Safe area**: DuckStation crops the picture to 292x224. HUD, windows, bars and hint pills now sit 8 px in from every edge (life HUD, KINGDOM bars, create, build mode, arcade, fight, space, dog, Boreagotchi, pack screens). Check: `FULL=1 test/ds.sh ...` gives the whole 320x240; `-DSAFE_LINT=1` logs anything inside the margin.
- **Not done**: Boreagotchi hatch from real play and the 21:00 summary shot; pre-lit box lists (Round 5 idea) were not measured, so still open; Dreamcast packs shelf and arcade lot were not reached in flycast.
