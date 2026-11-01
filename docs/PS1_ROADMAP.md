# PS1: what it needs next (updated Nov 1)
See also ROADMAP.md, EXPANSION_PACKS.md and PS1_PERFORMANCE.md (measured numbers).
1. Put the strict texture-warp cutting back: floor textures swim slightly near the camera after the last speed round.
2. City 20 fps -> 25 on the Sims, zoomed-in and zoomed-out cameras (first person is 27): about 130 hb of a 650 hb frame. Ideas: 8 MB pre-built block geometry, a cheaper tile loop (about 290 hb), a cheaper player model (about 85 hb), hide blocks behind buildings.
3. Arcade rework from branch `wip-arcade`; finish, test, bump the save VERSION, merge with the Boreagotchi save change.
4. Placeholders (COMING SOON cards, reserved slots and ids) for everything unfinished, see EXPANSION_PACKS.md.
5. Test old saves on a real memory card (Boreagotchi v45, REAL TIME v44).
6. Bus facing at stops, car pitch on real hills, gas-station and first-person vehicle shots.
7. Clock card kept between frames (needs packet room).
Always: more than 10 KB free past `_end` on 2 MB, Release builds only, old saves load.
