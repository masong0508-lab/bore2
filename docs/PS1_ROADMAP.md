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
