# BORE 2: porting to Nintendo DS and 3DS (pipeline and roadmap)

Plain summary: **3DS first, DS second.** The 3DS has far more power than the PS1, so most of the game moves over with few cuts. The DS has less of everything, so it needs a smaller version of the game.

## 1. What we have (the PS1 build)
- C code on PSn00bSDK. 2 MB RAM, about 28 KB free. Overlays (expansion packs) load from the disc into a shared 82 KB arena.
- Three layers in the code:
  1. **Game logic (portable as is):** sim.c, pop.c, look.c (names and looks), bigmap.c and world generation (bm_*), save.c, wants.c, the pack data tables, the overlay UIs' logic.
  2. **Rendering (PS1 only):** gfx.c, gfx_fast.h (GTE maths, the ordering table, GPU primitives), far.c (skyline and sea), voxel.c drawing, items.c and world.c drawing calls, atlas.c (VRAM).
  3. **Platform (PS1 only):** audio.c (SPU, CD audio, CD file reads), ovl.c (CD overlays), the pad, the memory card, VSync timing.
- Both DS and 3DS are little-endian ARM, like the PS1's MIPS. The `u8/u16/u32` types and fixed-point maths carry over. Saves and generated worlds will be the same bytes.

## 2. Target limits
| | PS1 (now) | 3DS | DS |
|---|---|---|---|
| CPU | 33 MHz | 268 MHz (New 3DS 804 MHz) | 67 MHz ARM9 |
| RAM | 2 MB | 128 MB (New: 256) | 4 MB (DSi mode: 16 MB) |
| Screens | 320x240 | 400x240 top, 320x240 bottom (+3D depth) | 256x192 x2 |
| GPU | polygons, no z-buffer, ordering table | PICA200: z-buffer, shaders, real textures | fixed pipeline: 2048 polygons / 6144 vertices a frame, 4 MB... only 656 KB of VRAM |
| Storage | CD | SD card, any file size | cartridge (ROM size is the limit), SD with a flash card |
| Toolchain | PSn00bSDK | devkitARM + libctru + citro3d | devkitARM + libnds (+ NitroFS) |

DSi-enhanced mode (16 MB) is worth considering for the DS build; plain DS (4 MB) is the stretch goal.

## 3. The plan, in phases

### Phase 0: get the code ready (in this repo, no new platform yet)
- Put every platform call behind a small header: `plat_gfx.h`, `plat_audio.h`, `plat_input.h`, `plat_file.h`, `plat_save.h`. Right now the game calls GTE and GPU functions directly in many files.
- Find and list every GTE use (`gte_*`, `gfx_xform`, `ApplyMatrixLV`, `SquareRoot0`). Wrap them in one math header with a plain-C fixed-point version beside the PS1 one. The C version is the one the ports will use first.
- Replace direct CD calls (`audio_cd_load`, `audio_cd_peek`) with `plat_file_read(name, buf, len)`.
- Add a host build (Linux, SDL) of the game that uses the plain-C maths and a simple software rasteriser. Benefit: faster testing than the PS1 emulator, and the same host build tests both ports' logic.
- Keep the save format versioned as now (VERSION 39). Do not change it for the ports.

### Phase 1: 3DS port (target: playable base game)
1. Project skeleton: devkitARM, libctru, citro3d. Build a `.3dsx` (and later a `.cia`).
2. Implement `plat_gfx` on citro3d: a flat/gouraud/textured triangle path that replaces the ordering table. Use the real depth buffer, so the "draw order" tricks (OT_FLOOR, far haze slots) turn into depth values.
3. Textures: the atlas (atlas.c) becomes one or two real textures in RAM. The 4-bit/8-bit CLUT tricks become plain RGBA/RGB565.
4. Voxel people: draw the same voxel cells as quads. No sprite-render-to-texture needed; the crowd sprite/LOD system can stay as a quality setting.
5. World and far view: same data, but raise the limits (window size, far range, no packet budget). The far skyline and sea become a normal draw.
6. Screens: top = the 3D view, bottom = the HUD, phone, needs, map. The phone menus and expansion screens are a natural fit for the touch screen.
7. Audio: the music/voice files become normal audio files streamed from SD (decoded with libctru's audio). Skip CD/XA.
8. Packs: no PPF patching and no overlay arena needed. Each expansion is a file on the SD card loaded on demand, or simply built into one program. Keep the "NOT INSTALLED" screen for missing pack files.
9. Save to SD; read old PS1 saves only through an export/import tool (see section 5).

### Phase 2: 3DS polish
- Stereoscopic 3D (depth slider): free with a z-buffer; render each eye.
- New 3DS speed mode and bigger limits, with a plain 3DS fallback.
- Higher-resolution textures and longer view distance, since the PS1 limits are gone.
- Touch controls: tap lots on the neighborhood map, tap-to-move.

### Phase 3: DS port (target: a smaller but real BORE 2)
This is a re-make of the rendering, not a copy.
- **Polygon budget:** about 2048 triangles on screen at once. The PS1 build draws far more. Use: a small world window (about 10-12 tiles round the player), box-only far buildings, sprite people (render once, show as quads, the PS1 crowd sprite idea), and cut trees to a few polygons.
- **Memory:** 4 MB total for code, world window, textures and sprites. Likely needs the world window cut and generated bigger-on-demand. The DSi 16 MB mode gives room for the full simulation.
- **Textures:** 656 KB of VRAM split between textures, sprites and the 2D screen. Use 4-bit/8-bit paletted textures (the same as the PS1 atlas, so the art pipeline carries over).
- **Screens:** top = the 3D game, bottom = HUD and touch menus.
- **Packs:** put the expansions on the cartridge (NitroFS) and load them on demand, like the PS1 overlays. That part of the design carries over almost one-to-one.
- **Likely cuts:** large expansions (SKY AND SEA at 53 KB of code plus the real-world flight view), the crash/destruction system, the biggest city sizes, and long draw distance. These are decided by measuring, after phase 1.

### Phase 4: release
- Test on emulators (melonDS or DeSmuME for DS; a Citra-family emulator for 3DS) and on real hardware. Emulators are not enough for speed and memory limits.
- Package: `.nds` (and DSi `.dsi` if used), `.3dsx` and `.cia`.
- Keep one README with the same pack list as the PS1 release.

## 4. Order of work (suggested)
1. Finish the PS1 build (crash testing, flight and city visuals). The ports start from a stable game.
2. Phase 0 refactor, with the host build. This is the work that makes both ports cheaper.
3. 3DS base game, then expansions one at a time, easiest first (FARM, PARADISE, GOTCHI, DJ, PARTY, DOG), then PRISON, ARCADE, KINGDOM, SPACE, SKYSEA.
4. DS: base game first with the smaller renderer; only add expansions the memory allows.

## 5. Things to decide
- **Plain DS, or DSi-enhanced only?** DSi-enhanced (16 MB) is much easier.
- **Old saves:** convert PS1 memory-card saves to the new format with a small tool, or start fresh on each system.
- **One codebase or two?** Recommendation: one game core, separate `ports/ps1`, `ports/3ds`, `ports/ds` folders for the platform layers.
- **Art and sound rights:** check that all sounds, music and fonts can be redistributed on a new system.
- **Release channel:** homebrew only, or also the usual flashcart/emulator sites. It affects how the DS file is packaged.

## 6. Risks
- **DS memory and polygons** are the biggest risk. The DS version may have to be a reduced game. Decide after a spike (a single week to draw one lot with people and measure).
- **Hidden PS1 assumptions** in the code: the ordering-table draw order, the GTE's 16-bit range limits, the 1 KB data cache, 2 KB scratch tricks. Phase 0 finds them early.
- **Timing:** the game logic assumes a PS1 vblank rate (60 or 30). Keep the logic's own time step separate from drawing in the host build.
- **Licences and tools change:** check the current state of devkitPro, libctru and emulators before starting.

## 7. First small steps (when you say go)
1. Add the `plat_*` headers and the plain-C maths. Make the PS1 build still pass its tests.
2. Build the Linux host build and run one lot with people, so the shared code runs somewhere other than the PS1.
3. Make a 3DS "hello" that draws that same lot with citro3d.
