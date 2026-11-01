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
