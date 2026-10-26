# BORE 2 for the Nintendo DSi (port, step D0)

Status: **written but not yet built or run.** The workspace it was written in had no DS toolchain (devkitARM) and no DS emulator, and the libnds it could fetch needs the `calico` library, so even a syntax check could not run. Expect a few compile fixes on the first real build.

## Build (on a computer with devkitPro)
1. Install devkitPro, then `dkp-pacman -S nds-dev` (devkitARM, libnds, calico, ndstool).
2. `cd ports/dsi && make` gives `bore2_dsi.nds`.
3. The ROM must be flagged as DSi-only (DSi unit code), and run on a DSi or a DSi-capable loader/emulator. The exact `ndstool` flag for that is to be checked against the installed ndstool's help; `main.c` also stops with a message when it is not on a DSi (`isDSiMode()`).

## What step D0 does
One lot (13 x 13 tiles), a house box and 6 billboard people on the 3D screen; the bottom screen shows the polygon count (budget 2000) and the GPU's own polygon/vertex counters. This is the measuring spike from `docs/PORTING_DS_3DS.md` section 8.7. Read the numbers off a real DSi and write them into `docs/COSTS.md`.

## Next
- D1: share the platform-free game core from `src/` (sim, world, people) through `plat_*` headers (Phase 0).
- The shared sprite baker for people.
