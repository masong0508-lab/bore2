# PS1 speed tools

How the city's frame time is measured (used for docs/PS1_PERFORMANCE.md). All of it runs in the test emulator, headless.

**The test emulator** (`/home/claude/tools/ds/b/bin/duckstation-regtest`, the path `test/ds.sh` and everything here calls). After a new machine:
`tools/perf/restore.sh` unpacks the built one from the `ps1-toolchain` branch (and installs the compiler and PSn00bSDK in `/opt/psn00bsdk` if they are
missing, with `/opt/mipsel` as a link to it). `tools/perf/build_runner.sh` builds it from source instead:
- DuckStation `a003832e33bb66857a5d9fe2d0ca11e9463771ed` (2023-04-29), the regression runner only, headless (no Qt, GL, Vulkan, SDL, audio output).
  The first build's commit was never written down; `regtest_host.cpp` is the same file from e5039055 (2023-01-17) to 5b980daf (2023-08-15), and the
  frozen-street readout (`AUTO_CITY AB_LOCK4 AB_FREEZE=150`, 5200 frames) gives the documented numbers on this one (world 357 hb, 19.81 fps from the need).
- Three patches to `src/duckstation-regtest/regtest_host.cpp`, in this order (`patch -p1 -d src/duckstation-regtest < ...`): `regtest_base.patch`
  (written again in November 2026 from what the two later patches show of it: the first one was lost), `regtest_pcsample.patch`, `regtest_card_wav.patch`.
  The base: `DSSCRIPT=<file>` with lines `FRAME button` (held 8 frames; b = cross, a = circle, y = square, x = triangle, l r l2 r2 l3 r3, start, select,
  up down left right) and `FRAME shot` (saved as `frame_<100000 + emulator frame>.png`); FRAME counts from the `GAMESTART` line of the log (the first frame
  after 20 with the CPU in RAM at 0x10000 or above: that is the BIOS's own shell starting, frame 34 with this BIOS, about 700 frames before the game's
  code runs; the scripts' times have always counted from there). `DSRAM=<file>`: the RAM at the end (2 MB, or 8 MB with `DS8MB=1`, which also turns the
  8 MB on). `DSCROP=None` and `DSRAW=1` (what `FULL=1 test/ds.sh` sets): nothing cropped, and saved pixel for pixel (320x240). `PCLOG` lines every 100 frames.
- OpenBIOS: `pcsx-redux/nugget` (pcsx-redux's `src/mips`) `821a3ff0248a85bb0330221794f3b9556c078a6e` (2026-10-18), `make -C openbios PREFIX=mipsel-none-elf`,
  copied to `bios/openbios.bin` beside the runner (it takes the BIOS it finds there). Do not take a newer one without testing `test/oldsave2.sh`: with
  nugget f044c6b (2026-11-11, pcsx-redux efd0eb17; it needs `OPENBIOS_SHELL=shell` to build) the version 50 test build crashed while loading and the runner hung.
- Baseline on this runner (today's main, `-DAUTO_CITY=1 -DAB_LOCK4=1`, `city.txt`, 5200 frames, 2 MB): 950 life frames, mean cpu 706 hb, world 397 hb,
  19.36 fps from the need. Frozen street (`-DAB_FREEZE=150` too): 951 frames, mean cpu 582 hb, world 357 hb, 19.81 fps from the need.

1. **Test build**: `cmake -S . -B /tmp/c_cut -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/opt/psn00bsdk/lib/libpsn00b/cmake/sdk.cmake -DPSN00BSDK_TC=/opt/psn00bsdk -DCMAKE_C_FLAGS=-DAUTO_CITY=1`
   (`AUTO_CITY` starts a life in SAN BOREAS by itself, the same seed every run. It must be a Release build, or the boot runs out of memory.)
2. **Profiler patch** for the DuckStation regtest runner (`regtest_pcsample.patch`, applied to `src/duckstation-regtest/regtest_host.cpp`):
   with `DSPROF=<file>` set it samples the CPU's program counter every 4001 cycles and writes `address count` lines at the end
   (`DSPROFFROM=<frame>` skips the boot).
3. `ab.sh NAME [frames]`: builds, runs `city.txt` for that many frames, then prints the whole-run readout (`ramprof.py` reads the
   game's own counters out of the RAM dump by symbol: fps over the whole stay, mean cost of each part of the frame, prims per
   part, how many frames fit 3 vblanks) and, with `TOP=30`, the busiest functions (`pcsym.py`). `pclines.py ELF PCFILE func...` goes
   down to source lines.
4. Compare like with like: run each build at 3 lengths (`RUN=_3100 ab.sh NAME 3100`, 3700, 4300). One run varies a few percent
   with where the route happens to be.
5. **A/B without the route drifting**: add `-DAB_LOCK4=1` to the test build's flags (with `AUTO_CITY=1`). Every frame then takes 4 vblanks and
   runs 3 game steps whatever it costs, so two builds see the same city frame by frame (their logic readouts match) and the mean need,
   the share within 3 vblanks and the world's hblanks compare like with like in one run (5200 frames gives about 960 city frames).
   The fps it reads is 15 by design; measure the real fps with normal runs as in 4.
6. **Pictures**: add `-DAB_FREEZE=N` too (with `AB_LOCK4`): after N life frames the city stands still and is still drawn, so shots of
   two builds taken any time after that (`ds.sh` with a script of `NNNN shot` lines) can be laid over each other. Only the falling
   blossom (it follows the frame count) and a crowd sprite refreshed a frame sooner or later should differ.
7. **8 MB**: `DS8MB=1` before `ds.sh` (or `runbuild.sh`) runs DuckStation with its 8 MB RAM on (`ram8` is then 1 in the game).
8. **Real frame times**: `ramprof.py` prints how many vblanks each life frame really took, the frames' *need* (their time less the work done only
   in their spare time), and, with `AB_LOCK4`, "by the clock" fps from the need and from the flip: compare two builds on those (the same route,
   frame by frame). It also prints the household's route-search frames (`sim_step` histogram), the people's drawing split and the slowest frames.
9. **The townsfolk** (`citprobe.py RAM ELF`, with a `-DCIT_PROBE=1` build): what they did over the run (steps lived and ground walked by mode, route searches, chores, walks and how they ended, greetings) and a snapshot every 512 steps; build the same with `-DCIT_FLAT=1` to compare against everyone thought about every step. `ramprof.py` also prints the townsfolk's tiers, thinking calls and worst frame.
10. **A memory card that stays, the sound, the sound chip** (`regtest_card_wav.patch`, on top of the profiler patch; then `ninja duckstation-regtest`
   in the runner's build): `DSCARD=<file>` makes card 1 that file, kept from run to run (made and formatted if missing), so a save made by one
   build can be loaded by another (`test/oldsave.sh`); `DSWAV=<file.wav>` writes everything the SPU plays (44.1 kHz stereo; `test/audiocheck.py`
   measures the AUDIO sliders with it); `DSSPU=<file>` writes the SPU's registers (1F801C00..1F801DFF) at the end. Without them it runs as before.
11. **Test builds and RAM**: a test hook's code and data come out of the room past `_end`; under about 7 KB the boot or a busy frame can crash (the
   stack runs into the SDK's interrupt tables, and the log then fills with "Unhandled instruction": cap it, e.g. `| head -c 30000000`).
   `-DPLACE_MAX=1300` gives a test build about 3 KB back (the AUTO_CITY world has 1256 places of the 1800 kept for).
12. **Lean test hooks** (`-DAUTO_CITY=1 -DAUTO_LE=n`, main.c `le_tick`; results in `dbg_le`): 1 a home and properties, then the phone's map; 2 the AUDIO
   sliders (with `DSWAV`, then `test/audiocheck.py`); 3 the townsfolk who know you (hellos and calls, `dbg_greet`); 4 FARM LIFE lived through the pie
   menus for four days (the farm job, crops, animals, chores, the frost, a save: load that card with today's plain build and `test/oldsave_load.txt`);
   5 the FARM BOOK's pages (press R1 in the script; `-DAUTO_LE5_SAVE=1` saves first); 6 farm-stand customers (`dbg_buy`: asked for, sent, got there,
   gave up). Use `-DPLACE_MAX=1300` with 3, 4 and 6.
13. **Every camera and VIEW DISTANCE from one build** (`-DAUTO_CITY=1 -DAB_LOCK4=1 -DAB_CFG=1`): `abcfg.py DISC.bin CAM VIEW [HELPER_OFF]` sets the
   camera (1 the Sims camera as it starts, 2 zoomed in all the way, 3 zoomed out all the way, 4 first person), the VIEW DISTANCE (0..5) and the speed
   helper (1 = off) in a copy of the disc image, so a table of every camera at every view distance needs one build. The camera chosen holds for the
   whole run (the plain route switches to a high look over the roofs at step 2000 whatever was asked, so `AUTO_CAMTEST` numbers before this were
   mostly that camera).
14. **Where the street's things stand** (`placecheck.py RAM ELF`, any AUTO_CITY run): things on the road (asphalt, lines, crossings), things on the
   kerb-side pavement, tall things side by side.
15. **Per-frame trace, panned freeze, clock** (`-DAB_TRACE=1`: `dbg_tr[1000][4]` = need/5, logic/2, world/3, people/2 of each life frame; `-DAB_FREEZE=N -DAB_PAN=4`: after the freeze the camera slides a tile every 160 emu frames; `AB_PAR="7=n"` with `AB_CFG`: the clock held at n x 6 minutes). A table the held-camera way: run each camera with `abcfg.py`, read `dbg_tr` from the RAM dump, and report the mean, the share over 786 hb, the share needing 4 or more vblanks ((need + 40) / 263) and the dearest frame after the first two.

16. **`-DAB_TR1=1`** (with `AB_TRACE`): `dbg_tr` keeps only each frame's need (640 frames, 640 bytes) instead of 4 values for 1000 frames: a test build that carries new code needs the RAM. `trtable.py` reads either. `round5_floor_cells.patch` is the saved floor-baking experiment (PS1_PERFORMANCE.md, Round 5); `-DFB_OFF=1` turns it off in the same code.

17. **Safe area**: `FULL=1 test/ds.sh ...` shows the whole 320x240 (DuckStation normally crops to 292x224). `-DSAFE_LINT=1` (or 2) logs text and boxes inside 8 px of an edge (`safe_n`, `safe_log`).
