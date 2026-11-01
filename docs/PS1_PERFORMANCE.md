# PS1 speed and streaming: where it stands

## Measured (SAN BOREAS street view, test emulator, same route every run)
Three run lengths each, so the route's own variation shows. The smallest draw range and the far view off throughout (the controller pins there; see below).

| | before | now |
|---|---|---|
| frames a second, whole stay | 15.2 / 14.9 / 15.3 | 18.8 / 18.9 / 19.6 |
| mean CPU a frame (hblanks) | 916 / 936 / 912 | 730 / 733 / 710 |
| frames done within 3 vblanks (20 fps or better) | 16 / 12 / 21% | 83 / 85 / 87% |

A frame is 262 hblanks at 60 Hz; 20 fps is 786, 30 fps is 524. Mean prims a frame: about 300.

Where a frame goes now (mean hblanks): game logic 158 (sim steps 57, the townsfolk 92, traffic 10), the world 353 (floor 117, walls 33, items 58, roofs 72 and the skyline 22), people and pets 156, the HUD 57.

## What was done (and why those)
A PC sampler in the test emulator (tools/perf) showed a flat profile with one clear lump, so this was cutting waste, not a new algorithm:
1. **The near-plane / texture-warp cutter (`nc_piece`) was 12.5% of all CPU.** Every split recomputed all four corners of the piece from the whole quad (12 interpolations of 8 fields). It now cuts a piece in two from the piece's own corners (two midpoints, shifts only). Same output, about half the cost: the biggest single gain (15 to 17.3 fps by itself).
2. **Ground heights are kept, not rebuilt each frame.** The hills' height at each of the window's corners is computed once and kept until the window moves, the city is remade, or the land is edited (`bm_hill_reset` clears it). The lookup is inline.
3. **Looking only at tiles the camera can see.** The view position is linear in the tile coordinates, so each screen edge cuts a row of tiles at one place (`tile_row_span`). The floor, wall/item and roof loops now visit only the span of each row, never reading the map for the rest. Flat roofs (drawn from their first tile wherever it is) are kept by a small per-row table. Output is identical (same prim counts).
4. **Roofs: the ground height is looked up only for roofs that are drawn.**
5. **Townsfolk far from you move less often** (every 4th step past 26 tiles, every 2nd past 14; was every 2nd past 14).
6. **A sim's room score** (a 25-tile look at its surroundings) every 8th step, not every step; `world_covered` skips bare-floor neighbours without a call.

## Two things found that matter for the next step
- **The shed controller can never be satisfied in a town.** It sheds detail when a frame takes more than 500 hblanks (a 30 fps target) and restores below 400. In the city a frame never gets near 500, so the draw range sits at its smallest (5 tiles) and the far view and trees are off the whole time (mean shed 3.98 of 4). Its load figure also counts the crowd's sprite renders, which deliberately fill the time left before the next vblank, so a frame that runs at 30 fps measures 522 and counts as slow anyway. Retuning it to a 20 fps target using the cost *without* the sprite fill was tried: it changed nothing in this scene (still pinned, a stricter A/B shows no difference) and was left out. The real fix is the next point.
- **The cost is flat.** After the above, no function is above 6% of CPU. What is left is per-primitive overhead (lighting, colour grading, ambient shading on every face, about 2.5 hblanks a primitive against roughly 0.3 for a bare GTE quad) and the game logic (21% of a frame, and it grows with the catch-up steps a slow frame forces: a slow frame runs up to 3 steps, which makes the next frame slower).

## The VIEW DISTANCE setting, and what the far view costs (added with the options renovation)
- **What a view distance really shows depends on the camera.** The far pass only draws what the screen can see: the top edge of the screen meets the ground at a distance set by the camera's angle. The default Sims camera sees ground to about 46 m and buildings to about 137 m, however many kilometres are chosen; a low camera or first person sees as far as the setting. The options screen now says so: with VIEW DISTANCE selected it shows how far the last frame's far view really reached (`far_reach_m()`), and the first label is "NEAR: 40 M" (it used to say 50 M, which was the map's own window).
- **The far view was the most expensive thing in the city** (824 hblanks a frame at the default camera, with the full-detail radius at 10 tiles; the whole frame 1704). That is why the speed controller always turned it off in town, and why a chosen "1 KM" showed nothing past the near map. Measured and cut (same pictures, byte for byte, as they are caches and skipped work):
  1. **Ground heights past the window were worked out from noise every frame** (about 20% of all CPU). A 1024-entry table keyed by world tile now keeps them (`gh_raw` in `world.c`), and the block hill cache in `bigmap.c` went from 64 to 512 entries with a real hash (it thrashed). `world_gh` skips the three corners that weigh nothing when asked on a tile corner (the far ground and roads always are).
  2. **The far roads scanned all 49 x 49 blocks twice a frame**; they now scan only the blocks within reach of the eye, and clear their marks four bytes at a time.
  3. **Far ground cells 24 tiles or more from the eye** are drawn as one quad to every 2 x 2 cells.
  The far pass now costs about 400 hblanks (land 250, buildings 90, trees 60), so far view on at the default camera is 11 fps, not 8.7. It is still too much for a steady 20, which is why:
- **SPEED HELPER** (OPTIONS > GRAPHICS, on by default): the controller that pulls the view in when frames run slow. Off, the game draws exactly what was chosen. It also learns what the far view costs and does not bring it back unless there is room for it (before, it came back every half minute for a few frames and was cut again). When it has cut the view, the VIEW DISTANCE row says so.
- Frame rates with the helper on (city street, start of life, same route; shed is pinned at its most so the far view is off): default camera 18.96 fps (90% of frames within 3 vblanks), zoomed in all the way (R2) 17.95 (76%; the big near tiles are cut more by the near-plane cutter, floor 177 hb against 118), zoomed out all the way (L2) 19.03 (89%), first person 26.9 (99%).
- **First person with the far view on is the worst case, and is not fixed**: 4.7 fps at 1 km (3300 hblanks: far land 2000, buildings 410, trees 170), against 22 fps with it off. A level camera sees to the full setting, so the far pass has 300 quads to do, and 55% of the CPU is the land's noise (`vn`, `bm_hash`, `elev`, `bm_terrain`, `bm_hill_t`) for corners the caches have not seen yet as the view sweeps. The helper cuts the far view there (27 fps, but the view ends at 41 m). What it needs: a baked far height and ground-type grid (about 16 KB for an 8-block lattice) so a corner is a table read, and the far land kept as strips instead of rebuilt each frame. Not done: it needs the RAM first.
- The far roads table is rebuilt only when the big map changes (`bm_edit_stamp`), not on every `world_touched` (a door, an item): before, any of those cost 2401 `bm_block` calls.
- Zoomed in all the way (R2) costs 8% more than the default camera: the near ground tiles are bigger than the GPU's 1023-pixel limit and must be cut (`big` in `nc_piece`); the warp budget is not what limits it (checked: lowering `NC_BUDGET` changed nothing).
- Free RAM past `_end` is now about 28 KB (it was 43): the two caches and the new option text.

## What "Crash Bandicoot style" still needs (not done)
Crash got its speed by baking everything that does not change and streaming fixed-size zones. What that means here, in the order it should be done:
1. **Bake the light.** A floor tile's or a wall's colour is the same from one frame to the next except as the sun moves. Keep each tile's four graded corner colours (with its ambient shading) in a table keyed by the light's step, so a tile costs a table read, not `gfx_light` + `gfx_grade_tex` + the ambient pass. The per-tile cost is where the flat 2.5 hblanks a primitive lives.
2. **Per-block prebuilt packet lists.** Walls, roofs and ground of a block are static: build a block's list once (the far skyline in `far.c` already works this way) and copy it, so a block costs one copy, not a pass over its tiles.
3. **Slice the logic.** Run the townsfolk in two halves on alternate steps and the sims' needs on a slower clock, so catch-up steps stop costing a full pass each.
4. **Stream by block from the disc.** The city's tables are one set in RAM. Split the street grid into block files with the next ring read in the background (the CD is already read in the background for music). This needs 40 to 60 KB for a block buffer, which means freeing it first from `lvpool_small` (96 KB) or `seqbuf` (45 KB). Free RAM now: about 28 KB past `_end` (keep that above 8 KB or boot crashes at "AUDIO+DISC 60%").

How to measure any of this: tools/perf/README.md.
