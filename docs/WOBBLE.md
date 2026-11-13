# Wobble in the 3D engine: what was measured, what was fixed

Measured with `-DAUTO_CITY=1 -DCAM_PROBE=1 -DPLACE_MAX=1300` (add `-DCAM_OLD=1` for the old following), a script that holds UP, and
`tools/perf/camjudder.py RAM ELF`: where the camera was at each life frame and when (in vblanks).

## Checked, and fine
- The GTE: projection scale 230, so one world unit is 0.15 pixel at the normal zoom and 0.38 pixel at the closest. The view matrix (4.12) and the
  integer view-space coordinates move a vertex by under 0.1 pixel. Pixel snapping of the GTE's output is the only vertex rounding left and the PS1 has no
  sub-pixel place to put it. Not worth changing.
- Still camera: the frozen city (`AB_FREEZE`) draws the same picture every frame.
- Texture swim: cut pieces (ca97cfb, 02002c7).

## Found and fixed: the follow camera went by drawn frames, not by time
`camera_apply` moved the focus a sixth of the gap per DRAWN FRAME (a third on a rider), while the game runs 3 or 4 vblanks a frame. A 4-vblank frame
moved the people a third further than a 3-vblank one but the camera the same, so the camera sped up and slowed down against the walker in every walk.
The whole-unit division also stalled the camera up to 5 units short of the target. Now: 8.8 fixed point, a share of the gap per VBLANK that passed.

The title room's sway and the Bore's turn went by the frame count (`title_t`), so a slow frame made them stutter. Now by vblanks (`title_ph`).

| walking, 508 frames | before | after |
|---|---|---|
| change of camera speed, frame to frame, mean (units/vblank) | 0.38 | 0.21 |
| ... 95% of frames under | 2.75 | 0.67 |
| ... worst | 4.40 | 4.50 (one 5-vblank frame) |

Pictures: `docs/wobble_title_after.png` (the title room, four moments).

## DS needs
`ports/dsi` has its own camera. Do the same there: follow by the vblanks (or milliseconds) since the last frame, not a fixed share per frame, in 8.8
(`share per tick 18/256, rider 38/256`, applied once per elapsed tick; snap when under 1.5 units), and drive the title sway from the clock, not a counter.
