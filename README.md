# BORE 2 (PlayStation)

The sequel to BORE (GBA), in true 3D. The voxel people (6x4x8 blocks, built by the GBA game's own buildLook), the items
(the GBA's textured boxes), the 40x40 lots, 103 wallpapers and all 33 jukebox songs are on the disc.

## Play it
`bore2.cue` + `bore2.bin` (about 6 MB, every song included) in DuckStation, PCSX-ReARMed or on a real PlayStation (burn the cue/bin).
Every push to GitHub builds the disc: open Actions, the newest green run, and download **bore2_disc** under Artifacts.

Controls: stick / d-pad move, X uses the thing in front (fridge, bed, shower, sofa, TV, radio, the DeadSet...), talk to a
housemate with X, O gets up, SQUARE runs, TRIANGLE game speed, SELECT switches Bore, START the menu (TRAVEL to the island
and the desert town, BUILD MODE, CREATE A BORE, JUKEBOX). Camera: L1 R1 orbit, L2 R2 zoom, the right stick orbits and tilts.

## Build it
1. Install PSn00bSDK (https://github.com/Lameguy64/PSn00bSDK/releases) so `mipsel-none-elf-gcc` and `cmake` work.
2. In a terminal:

       export PATH=/path/to/psn00bsdk/bin:$PATH
       export PSN00BSDK_LIBS=/path/to/psn00bsdk/lib/libpsn00b
       cmake --preset default
       cmake --build build --target iso

3. Open `build/bore2.cue` in DuckStation.

## The music
All 33 jukebox songs are SEQUENCES, not recordings: each is its note events plus small instrument samples in the PS1 sound chip's
own format, in one file, `assets/SONGS.BIN` (3.6 MB). `src/audio.c` loads one song's samples into sound memory and a 480 Hz timer
interrupt keys the notes on, so the sound chip does the playing. `tools/build_music.py <bore-gba-main> assets` rebuilds it with the
GBA game's own converter (`tools/xm2gba.py`), so the songs play the same notes, volumes and pans as on the GBA.
The creature code: `python3 tools/port_look.py <bore-gba-main>/source/main.c src/look_port.inc`.

## Folders
- `src/` the game (C). `look_port.inc` is the GBA creature code ported by `tools/port_look.py`.
- `assets/` voices, the songs (`SONGS.BIN`), the song list.
- `tools/` porting, hair texture and audio tools; `xaenc.c` (XA encoder using libpsxav); `lrrun.c` (headless emulator runner for screenshots).
- `test/` scripts used to take emulator screenshots and measure FPS.

## Known gaps
No memory-card saves, the 80x80 map's RAM use is estimated and not yet measured on a disc build, wants/fears only partly done, tested only in an emulator.

## Crowds and parties
THROW A PARTY (pause menu, or the phone) brings up to 60 guests, who walk in, dance round you and go home after three hours.
Every person in a crowd is the full model, like your own Bore: `src/crowd.c` keeps a compact copy of each model (~2 KB) in the
RAM the voice clips used before they went into the sound chip, draws the few biggest on screen live in 3D, and shows the rest as
sprites of that same model, rendered into spare video memory and redrawn round the crowd as the frame's time allows.

## The city
BORE CITY is an 80x80 map: a 5x5 grid of 16-tile blocks, 23 homes in four kinds (HOUSE, COTTAGE, TOWNHOUSES, TWO-STOREY with a loft), two parks, the square, a diner, a gym with an arcade, an office and a skate plaza. `world_gen_city` in `src/world.c` places them from the `lots[][]` table; to add a home kind, write a `city_*` function and give it a table entry.

## DS port (in progress)
The game code is split so a Nintendo DS build can sit beside the PS1 one:
- `src/platform/compat_math.*` the PS1 SDK maths (rotations, matrices, sin, sqrt) in plain C, tested against the real values.
- Still PS1-only: `gfx.c` (GTE drawing), `audio.c` (SPU and CD music), the pad code in `main.c`, and the GTE calls in `items.c`.
- Plan: `src/platform/nds/gfx_nds.c` on libnds' 3D engine (it sorts depth itself, so no ordering table, and gouraud colour is free), BlocksDS to build.
