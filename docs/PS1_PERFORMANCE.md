# PS1 speed and streaming: where it stands

## Measured (this session)
In the test emulator, SAN BOREAS street view runs at about 13 to 15 frames a second with 240 to 290 primitives and roughly 27 KB of packets a frame (the in-game counter, L3). The world's own cost is the biggest part (about 560 to 650 timer ticks in the `W` figure; sims and buildings are smaller).

## What was done
- Nothing that makes the city faster yet. The only change touching speed is that each packet buffer is 8 KB smaller (58 KB). The busiest frames measured before used under 54 KB, so this should not drop primitives; if `gfx_overflow` ever counts up, raise it again and find the bytes elsewhere.

## What "Crash Bandicoot style" needs (not done)
1. **Chunked world data.** The city is one set of tables in RAM. Split the street grid into zone chunks on the disc (a few blocks each) with the next chunk read in the background while the player walks (the CD is already read in the background for music).
2. **Prebuilt draw lists per chunk.** Walls, roofs and ground for a chunk are fixed. Build them once into a compact list (already done for far buildings in `far.c`) so a chunk costs a copy of packets, not a pass over tiles.
3. **A hard cap on work per frame**, as now, but driven by the chunk lists.
4. **Memory.** The arena (156 KB) is shared with the crowd and the overlays; a chunk buffer needs its own 40 to 60 KB, which means freeing that much from `lvpool_small` (96 KB) or `seqbuf` (45 KB) first.

This is several sessions of work on the core renderer, so it was not started. Step 4 comes first, because the game is within 20 KB of running out of memory.
