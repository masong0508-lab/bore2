# Dreamcast port: where it stands

The real PS1 game code (`src/`) runs on the Dreamcast. Only the platform layer is new (`ports/dreamcast/`).
`src/` edits are only under `#ifdef BORE_DC`; `ports/dreamcast/ps1_check.sh` shows the PS1 Release image is unchanged
(last check: `_end` 0x801fcd04, image sha1 82df98002af9767d, the same with and without the Dreamcast edits).
Build, disc and emulator steps: `ports/dreamcast/README.md`.

## What works now
| Part | How |
|---|---|
| Game, menus, rules | the PS1 sources, unchanged, overlays linked in |
| Picture | PS1 GPU ordering tables turned into PowerVR vertices (`dc_gpu.c`), 640x480, **24-bit colour** (no dithering, no banding) |
| Semi-transparency | only texels with the STP bit blend, the rest is solid, as on the PS1 (bus shelters now right) |
| Brightness | PS1 textures can be lit up to 2x; the PowerVR stops at 1x, so a second added pass makes up the rest (roads, people now as bright as the PS1) |
| Sound | the PS1 sound code (`audio.c`) unchanged, on a software SPU (`dc_spu.c`: ADPCM, gaussian interpolation, ADSR, reverb) mixed to the AICA at 44.1 kHz. Music, voices and the volume sliders work |
| Disc | `make disc` builds `bore2_dc.gdi` (boots in flycast). Songs and voices are read from the disc (`dc_cd.c`) |
| Saves | on the VMU (`dc_vmu.c`). Save, load, options and story come back (tested). Icon has 3 frames (look, blink, grin) |
| VMU screen | your Bore's face (follows the mood), name, clock and 4 needs |
| Options > EXTRAS | a Dreamcast-only tab: COLOURS (VIVID / CLASSIC PS1), TEXTURES (SHARP / SMOOTH), JUMP PACK, VMU SCREEN |

## Pictures
PS1 in DuckStation (left) and the Dreamcast (right), same city moment, after the brightness and blend fixes:
![city](dc_vs_ps1_city2.png)

Colours: CLASSIC PS1 (left) and VIVID (right, the default). Vivid = 1.45x saturation, 1.05x brightness, on shading and textures:
![vivid](dc_classic_vs_vivid.png)

Textures: SHARP (left) and SMOOTH (right, bilinear on the 3D world only; menus stay crisp):
![smooth](dc_sharp_vs_smooth.png)

The EXTRAS tab, the VMU save icon (3 frames) and the VMU screen:
![extras](dc_options_extras.png)
![icon](dc_vmu_icon.png) ![vmu](dc_vmu_screen.png)

Older shots: `dc_vs_ps1_title.png`, `dc_vs_ps1_city.png`.

### Roofs: a bug found (not fixed yet)
The `AUTO_HOME` test (buy a house, build, the roof tool) on both. Top: PS1 (DuckStation). Bottom: Dreamcast.
On the PS1 the ROOF tool shows the whole clay roof. On the Dreamcast the roof shows for a moment, then the camera pulls out
and the roof is gone (the panel still says THE ROOF). Walls, floors, windows and the furnished rooms match.
![roof](dc_vs_ps1_roof_tool.png)
How: PS1 `cmake -DCMAKE_C_FLAGS="-DAUTO_HOME=1 -DAB_LOCK4=1"`, `ninja iso_lite`, DuckStation with START at 1100 and X at 1180;
Dreamcast `make AUTO="AUTO_HOME=1 -DAB_LOCK4=1 -DDC_NOSPEED -DDC_CLASSIC" disc`, script `ports/dreamcast/test/home_dc.txt`.
Note: a Dreamcast test frame is about 2 vblanks, a DuckStation frame 1, so pair shots by the clock on the HUD.

## Speed (measured in flycast, which only roughly copies real Dreamcast timing)
SAN BOREAS street, normal play (`AUTO_CITY` build, 1500-2500 frames in):
| Step | fps |
|---|---|
| start of this round | 22-28 |
| faster sound mixer, no 64-bit divides | 26-29 |
| a late frame no longer waits for the next vblank | 41-48 |
| vertices written straight into PowerVR lists | 39-49 (CPU 230-360 of 262 hblanks a frame) |
So: about 45 fps, not yet a steady 60. Where the time goes (profiler, `make PROF=600`): sound mixing 17%, GTE 11%,
ordering-table walk 15%, the rest is the game itself.

## How shots and numbers are taken
`make O=obj_x AUTO="AUTO_CITY=1 -DAB_LOCK4=1 -DAB_FREEZE=150 -DDC_NOSPEED" disc` then
`dcrun flycast_libretro.so obj_x/bore2_dc.gdi 1000 NAME [script]`. `DC_CLASSIC`, `DC_SMOOTH` pick the Extras for a test.
`DCRUN_VMU=1` draws the VMU screen into the picture. The log (fps, CPU, lists) prints at the end.
PS1 side: `DS8MB=1 test/ds.sh bore2_lite.cue NAME 3500 script`.

## REAL TIME (OPTIONS > GAME)
The Dreamcast reads its own clock chip (`dc_rtc_min` in `dc_plat.c`, KOS `rtc_unix_secs`, read twice a second at most): switching
REAL TIME on asks nothing (the PlayStation has no clock and asks the time). The clock, weekday and season (by the month) follow the
Dreamcast's clock; flycast gives it the host's time. Left: the option on, no question; right: SAN BOREAS at the real 18:38.
![real time](realtime_dc.png)

## Still to do
- Side by side with DuckStation: inside a house, pie menu, Buy/Build, night, first person, far view, a park.
- Roofs (new ask): fix the roof-tool bug above first; then check shops, towers, flats, near and far, and the cut-away when inside or
  upstairs; then DC-only better roofs (ridges, overhangs, tiles with SMOOTH filtering, chimneys, fade cut-away, far roofs with fog).
- The far buildings' window textures use the PS1 texture window (GP0 E2): not done in `dc_gpu.c` yet.
- Steady 60 fps: sound on the AICA's own channels, a float GTE, cheaper OT walk.
- Upgrades: sub-pixel vertices, depth buffer, full far view with fog, bigger crowds, smooth lighting.
- Extras: Jump Pack rumble events, Boreagotchi on the VMU, DC catalogue set and outfits, shadows, water/glass, night glow, 4 controllers.
- Button names on screen are still the PS1's (X = A, O = B, SQUARE = X, TRIANGLE = Y).
