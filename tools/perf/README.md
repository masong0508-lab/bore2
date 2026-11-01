# PS1 speed tools

How the city's frame time is measured (used for docs/PS1_PERFORMANCE.md). All of it runs in the test emulator, headless.

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
