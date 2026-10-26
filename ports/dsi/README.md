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

## Verified so far (the shared core on a normal computer)
`ports/host` builds the platform-free world generator (`src/bigmap.c`, unchanged) with plain gcc and prints a generated world: 21 settlements, sea, forest, hills, towns. That proves the world code does not depend on the PS1, which is the first requirement for the DSi/3DS ports. Run `cd ports/host && make run`.

Next shared-core steps, in order: `look.c` (names and looks), `pop.c` (the people), `sim.c`, `save.c`; each added to the host build with a small shim, then the drawing goes through `plat_*` headers.

## D1 status (built and run here)
The DS build chain in tools/dsbuild (clang + libnds 1.8.2) and the headless melonDS runner in tools/dsemu now build and run this port.
Tested in the emulator (DS mode, FreeBIOS): bottom screen shows the 2D neighbourhood map from bigmap.c, a stylus tap picks a lot, the top screen draws that lot in 3D
(houses with pitched roofs, backyard pools and fences, towers, shops, parks, people). L/R change town, A/B zoom, START re-rolls the world.
Still to do: real DSi hardware flags (ndstool -g with the DSi unit code), the sim, saves, vehicles, and the other zones.
