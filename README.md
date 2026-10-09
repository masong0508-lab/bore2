# BORE 2 (PlayStation)

The sequel to BORE (GBA), in true 3D. The voxel people (6x4x8 blocks, built by the GBA game's own buildLook), the items
(the GBA's textured boxes), the 40x40 lots, 103 wallpapers and all 33 jukebox songs are on the disc.

## Play it
`bore2.cue` + `bore2.bin` (full disc, 350 MB, all the music as XA streams) in DuckStation, PCSX-ReARMed or on a real
PlayStation (burn the cue/bin). `bore2_lite.cue/.bin` is the same game without the music (1.7 MB) for quick tests.

Controls: stick / d-pad move, X uses the thing in front (fridge, bed, shower, sofa, TV, radio, the DeadSet...), talk to a
housemate with X, O gets up, SQUARE runs, TRIANGLE game speed, SELECT switches Bore, START the menu (TRAVEL to the island
and the desert town, BUILD MODE, CREATE A BORE, JUKEBOX). Camera: L1 R1 orbit, L2 R2 zoom, the right stick orbits and tilts.

## Build it (quick start, lite disc, no music)
1. Install PSn00bSDK (https://github.com/Lameguy64/PSn00bSDK/releases) so `mipsel-none-elf-gcc` and `cmake` work.
2. In a terminal:

       export PATH=/path/to/psn00bsdk/bin:$PATH
       export PSN00BSDK_LIBS=/path/to/psn00bsdk/lib/libpsn00b
       cmake --preset default
       cmake --build build --target iso_lite

3. Open `build/bore2_lite.cue` in DuckStation.

The repo has no music files (they are 350 MB). Making them is the "full disc" step below.

## Build it (all options)
PSn00bSDK (mipsel-none-elf GCC) + CMake + Ninja:
    cmake --preset default && cmake --build build --target iso        # the full disc (needs assets/MUSIC*.XA)
    cmake --build build --target iso_lite                              # without the music
The music: `python3 tools/build_audio.py <bore-gba-main> assets` (ffmpeg with libopenmpt, and `xaenc` from tools/).
The creature code: `python3 tools/port_look.py <bore-gba-main>/source/main.c src/look_port.inc`.

## Folders
- `src/` the game (C). `look_port.inc` is the GBA creature code ported by `tools/port_look.py`.
- `assets/` voices, song list, textures (small). The big music files are made by `tools/build_audio.py`.
- `tools/` porting, hair texture and audio tools; `xaenc.c` (XA encoder using libpsxav); `lrrun.c` (headless emulator runner for screenshots).
- `test/` scripts used to take emulator screenshots and measure FPS.

## Known gaps
About 20 FPS in life mode (target 30), no memory-card saves, wants/fears only partly done, tested only in an emulator.

## DS port (in progress)
The game code is split so a Nintendo DS build can sit beside the PS1 one:
- `src/platform/compat_math.*` the PS1 SDK maths (rotations, matrices, sin, sqrt) in plain C, tested against the real values.
- Still PS1-only: `gfx.c` (GTE drawing), `audio.c` (SPU and CD music), the pad code in `main.c`, and the GTE calls in `items.c`.
- Plan: `src/platform/nds/gfx_nds.c` on libnds' 3D engine (it sorts depth itself, so no ordering table, and gouraud colour is free), BlocksDS to build.
