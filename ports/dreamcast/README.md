# BORE 2 on the Dreamcast

The PS1 game's own code (`../../src`) on a Dreamcast platform layer. What works and what is next: `docs/DREAMCAST.md`.

## Files
- `dc_gpu.c` the PS1 GPU: VRAM in RAM, ordering tables to PowerVR vertex lists, texture pages (updated cell by cell), the colour grade, VSync.
- `dc_psx.c` the GTE and its maths library in C (PS1 fixed point). `psx/` PSn00bSDK headers for the Dreamcast.
- `dc_spu.c` the PS1 SPU in software, mixed to the AICA. `dc_audio_*.h` let `audio.c` run unchanged (generated from `src/audio.h`).
- `dc_cd.c` the PS1 CD calls on the disc's ISO9660. `dc_vmu.c` memory card calls on the VMU. `dc_plat.c` pad, RAM, overlays.
- `dc_extra.c` Dreamcast-only: the VMU screen, the OPTIONS > EXTRAS tab. `dc_prof.c` a sampling profiler for test builds.
- `mkdisc.py` builds the GD-ROM image. `test/` scripts: `newlife.txt`, `dcopts.txt`, `dcgallery.txt` (build with `-DDC_OPTS_TEST=500 -DDC_SHOWITEMS=1`), `roadaudit.sh` (the road test: nothing may stand on a road tile; fails if it does), `vmuls.py` (list a VMU image, draw its icons), `profsym.py`.

## Build
```
ports/dreamcast/toolchain/restore.sh          # once: sh-elf, KallistiOS, flycast core, dcrun into /tmp/dc
source /tmp/dc/environ.sh
cd ports/dreamcast && make disc               # obj/bore2_dc.gdi (+ obj/bore2_dc.elf)
make O=obj_city AUTO="AUTO_CITY=1" disc       # a test build in its own folder (the PS1's test hooks)
make PROF=600 ...                             # the profiler: frames 600-1200, top places in the log
```
The `.gdi` is the one to play: the songs and voices are on the disc (an `.elf` alone runs silent).

## Emulator (headless)
`/tmp/dc/pack/bin/dcrun /tmp/dc/pack/bin/flycast_libretro.so obj/bore2_dc.gdi 1000 /tmp/shot [script]`
Script lines: `frame button` (b = A, a = B, y = X, x = Y, l2 / r2 triggers, start, up...) or `frame shot`.
It saves PPM shots, the sound (`_audio.raw`, 44.1 kHz stereo) and prints the port's log. A VMU and a Jump Pack are plugged in
(VMU file in `$DCRUN_SYS/dc/vmu_save_A1.bin`); `DCRUN_VMU=1` shows the VMU screen in the picture.

## Controls
A = CROSS, B = CIRCLE, X = SQUARE, Y = TRIANGLE, triggers = L1 / R1 (both = L2 + R2), START + both triggers = SELECT.

## Toolchain from source
See `toolchain/` (binutils 2.46, gcc 15.2, newlib 4.6 from GitHub mirrors, KallistiOS master; about 1.5 hours on 2 cores).
