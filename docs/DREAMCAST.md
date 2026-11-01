# Dreamcast port: where it stands

Status: **first boot works** (toolchain, emulator, hello, title, SAN BOREAS in 3D). Not yet the full game. The PS1 build is untouched (no file under `src/` changed; `ports/dreamcast` is not in `CMakeLists.txt`). Build and emulator steps: `ports/dreamcast/README.md`.

## What works (seen in flycast, HLE BIOS, 640x480)
| Stage | Screenshot | Notes |
|---|---|---|
| Hello | `docs/dc_hello.png` | KOS + PowerVR, text from the shared 8x8 font in an ARGB4444 texture |
| Title | `docs/dc_title.png` | the real BORE 2 logo (`logo_rects.h`, 1057 rectangles), sky, a turning blocky Bore, START |
| SAN BOREAS | `docs/dc_life.png` | `bm_new`/`bm_block` of `src/bigmap.c`: terrain, hills (corner-averaged heights), streets with lane dashes, lots and buildings by type and storeys, trees, the capital BORE CITY; D-pad walk, camera follows |

Shared code compiled unchanged so far: `bigmap.c`. Measured: the readout shows 60 FPS in the emulator (that shows the PVR keeps the frame pace; the emulator's CPU timing is not the real SH-4's, so real-hardware numbers are still to be taken). The PS1 build runs at 320x240 with an ordering table (its FRAME RATE option offers steady 30 or 20; I did not measure it here). This port: 640x480, per-pixel depth buffer (no sort errors), sub-pixel vertices, perspective-correct by the PVR.

## Not done yet (honest list)
- The people: `look.c` / `pop.c` are not linked yet (they need an atlas/texture stand-in and `voxel.c`'s drawing through a DC `gfx` layer); the Bore shown is a three-box stand-in. Next: `dc_atlas` stub, then `pop_new` and drawing the Bore's `vox` cells.
- 1:1 with the PS1: sim, menus, saves, audio not ported. Plan as in the table below.
- The new asks (60 fps with bilinear filtering and far fog, VMU screen / Boreagotchi / VMU saves with icon, Jump Pack rumble, Dreamcast catalogue items, shadows/water/glow, 4 pads, 16:9, extras page) are all still to do; each goes behind `BORE_DC` / `ports/dreamcast` only. The PVR already gives the depth buffer, perspective correction and 640x480 in this build; fog, bilinear textures and the rest come with textured polygons.

## Carry-over plan (unchanged)
| PS1 layer | Dreamcast replacement |
|---|---|
| `gfx.c` GTE camera, ordering table | `dc_gfx.c`: PVR polygons with the depth buffer (done for flat/gouraud; textures next) |
| `voxel.c` joints | same maths on the SH-4 FPU |
| `audio.c` | AICA through `snd_stream` |
| `pad.c` | maple `CONT_*` (done in `dc_pad`) |
| `mcard.c`, `save.c` | VMU via `vmu_pkg` |
| Overlays | not needed: 16 MB RAM |

Rule for the PS1: Dreamcast work stays under `ports/dreamcast/` and `tools/dcrun.c`; any edit of `src/` would be `#ifdef BORE_DC` only, and a plain PS1 Release build must keep the same `_end`.
