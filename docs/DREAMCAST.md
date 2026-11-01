# Dreamcast port: where it stands

Status: **groundwork only. Nothing here builds a Dreamcast game yet.** The PS1 build is untouched by it.

## Why it is not further on

The Dreamcast needs KallistiOS and an `sh-elf-gcc` cross compiler. This workspace has neither, and its network is limited to a short allowlist, so nothing for the Dreamcast can be compiled or run here. Anything written for it now is untested. Everything below is a plan to build on, not a result.

## What carries over almost as is (plain C, no PS1 calls)

| Part | Files | Notes |
|---|---|---|
| The world from one seed | `src/bigmap.c`, `src/bigmap.h` | Already shared with the DSi build. |
| Population, wants, careers, stories | `src/pop.c`, `src/wants.c`, `src/story.c` (data) | They use `Sim` and `Bore` only. |
| Sim logic | `src/sim.c` | Calls the UI only through `sim_note` and the `sim_want_*` flags. |
| Look/Bore data | `src/look.c`, `src/look.h` | Face art uses `TexRef`, which needs a small texture shim. |
| Cutscene data | `src/cutscene.c` (the film tables) | The engine part draws through `gfx_*`. |

## What must be rewritten (the PS1-only layer)

| PS1 layer | Dreamcast replacement |
|---|---|
| `gfx.c`: GTE camera, ordering table, packet buffers | PVR via KOS `pvr_*`: submit vertices to the opaque and punch-through lists. The PVR sorts and does perspective-correct textures itself, so the OT depth tricks go away. |
| `voxel.c`: GTE joint matrices | Same maths on the SH-4 FPU (`mat_*` and `fipr`); the joint tree stays. |
| `audio.c`: SPU, XA streaming, CD seqs | AICA via KOS `snd_stream`, with the songs as ADPCM. The DS music pipeline (`tools/dsmusic`) is the closest tool. |
| `pad.c` | `maple` controllers, same buttons (`CONT_*`). |
| `mcard.c`, `save.c` | VMU saves via `vmu_pkg`. The save format itself is plain bytes and carries over. |
| Overlays (`ovl.c`) | Not needed: the Dreamcast has 16 MB of RAM. Link everything in one binary. |

## Order of work

1. `ports/dreamcast/platform.h`: the list of functions the shared code needs from a platform (`gfx_*`, `audio_*`, `pad_*`, `save_*`). The PS1 files already provide them, so this is mostly a header.
2. A KOS `Makefile` plus a program that opens the PVR, clears the screen and draws one voxel Bore. This is the first thing that can run in an emulator (Flycast).
3. Move `voxel.c` and `look.c` over. Then the town, then the life loop.
4. Sound last.

## Rule for the PS1

Dreamcast work goes under `ports/dreamcast/` and uses `#ifdef BORE_DC` where it must touch shared files. The PS1 `CMakeLists.txt` does not build it, so the PS1 disc cannot change because of it.
