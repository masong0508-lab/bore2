# BORE 2 (PlayStation)

The sequel to BORE (GBA), in true 3D. The voxel people (6x4x8 blocks, built by the GBA game's own buildLook), the items
(the GBA's textured boxes), the 40x40 lots, 103 wallpapers and all 33 jukebox songs are on the disc.

## Play it
**[disc/BORE2.chd](disc/BORE2.chd)**: the whole game in one file (about 4 MB, every song included). Download it and open it in
DuckStation (File > Start File). Turn on *Settings > Console > Enable 8MB RAM* for the bigger world: the game finds the extra RAM
by itself, and still runs on a normal 2 MB PlayStation.

For a real PlayStation or PCSX-ReARMed, burn or open `bore2.cue` + `bore2.bin` from `cmake --build build --target iso`.
Every push to GitHub builds both: open Actions, the newest green run, and download **BORE2_chd** or **bore2_disc** under Artifacts.

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

3. Open `build/bore2.cue` in DuckStation. (`--target chd` also makes the one-file `build/BORE2.chd`: `tools/mkchd.py`.)

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

## The city and places to go
SAN BOREAS is a whole state worked out from the save's seed (`src/bigmap.c`): towns, cities and villages, and on their blocks homes
and 19 kinds of place: shops (grocery, clothes, electronics, furniture, music), diners, cafes, restaurants, bars, gyms, pools, spas,
salons, cinemas, galleries, libraries, clinics, arcades, inns, hotels, offices, towers and schools. `src/world.c` builds each one's
inside (`lot_*`), `src/places.c` knows their names and opening hours, and the pie menu (X) sells what each one sells: a meal, a latte,
a drink or a round, a film, a haircut, a massage, a checkup, a swim, a room for the night, groceries for home, an outfit.

Jobs (START, JOBS) are at real places too: 14 careers (13 in town, and the old factory) with their own shifts, days, pay and six titles each (DISHWASHER to
RESTAURATEUR, INTERN to CEO, ORDERLY to MEDICAL LEGEND...). Go there (the boss pays the taxi) and X on a counter, desk or machine.

## Growing up
Every Bore grows up a day at a time: BABY, CHILD, TEEN, ADULT, ELDER (OPTIONS, LIFE: off, short, normal or long lives).
On a birthday, X on the fridge or the stove bakes a cake, and BLOW CANDLES on it makes the birthday a party (with confetti).
Two grown-ups who get on can START A FAMILY (X on one of them); or ADOPT on the phone. A baby takes after its parents, cries
when it needs something (FEED, CHANGE, PLAY, CUDDLE, PUT TO BED) and the grown-ups come running; a crib comes with the first one.
Kids and teens catch the school bus at 8 on weekdays and are home at 3; homework at a desk or a bookshelf keeps the grade up
(the SCHOOL tab in the Bore panel). At the end of the elder years they pass on (or not: OLD AGE, LIVE ON) and a stone stands in the
yard. The family tree (partner, parent, child, sibling, grandparent) is in the panel's PEOPLE tab. The rest of SAN BOREAS grows up
too (`src/pop.c`): its people have birthdays, babies, leave home, pass on, and new families move into the empty homes.

## DS port (in progress)
The game code is split so a Nintendo DS build can sit beside the PS1 one:
- `src/platform/compat_math.*` the PS1 SDK maths (rotations, matrices, sin, sqrt) in plain C, tested against the real values.
- Still PS1-only: `gfx.c` (GTE drawing), `audio.c` (SPU and CD music), the pad code in `main.c`, and the GTE calls in `items.c`.
- Plan: `src/platform/nds/gfx_nds.c` on libnds' 3D engine (it sorts depth itself, so no ordering table, and gouraud colour is free), BlocksDS to build.
