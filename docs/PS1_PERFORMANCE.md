# PS1 speed and streaming: where it stands

## Round 5 (Nov 8): block baking tried on the ground, measured, not kept
**Short answer: 30 fps is not reachable by baking, and a solid 25 is not either. Nothing from the bake is in the build.** The experiment is saved as `tools/perf/round5_floor_cells.patch` (a diff of `src/world.c`; apply it with patch, build with `-DFB_OFF=1` for the old path in the same code).

What was built (8 MB only): the city's ground storey cut into cells of 4 x 4 tiles. Each cell is baked once (one cell a frame) into a list of its corners (shared corners once, hill heights baked in) and its quads (a tile, or a 2x2 block, as four corner indices), and marks the tiles it takes so the old loops skip them. A frame runs a visible cell's corners straight from the cell through the GTE (three at a time), then walks the quads with the same tests as before: quads behind the eye still go to the cutter, off-screen ones are dropped by a screen-side test, the shades come from the existing floor-light cache. Water, indoors and tiles under a roof stay on the old path. Re-bake keys: `world_stamp`, `bld_gen`, window x/y, zone, `gh_gen`. 2 MB: off (no RAM for it).

Measured (helper off, `AB_LOCK4`, first 600 life frames, 8 MB, `abcfg.py ... 2 1`):

| camera, 8 MB | mean hb before | after | late (4+ vblanks) before / after |
|---|---|---|---|
| Sims | 756 | 738 | 46% / 35% |
| zoomed in | 924 | 922 | 100% / 100% |
| zoomed out | 906 | 862 | 97% / 92% |
| first person | 781 | 800 (worse) | 69% / 69% |

2 MB: unchanged by design (the pool does not fit, and the code alone is 3 KB where 2 MB has 1.5 KB to give above the 10 KB floor). Pixel comparison of frozen frames was not done, because the result was not worth keeping.

Why it gains so little. (1) The world is not floor-bound: the profile after baking is as flat as before. Floors were about 60 hb of the 760. (2) The PlayStation has no data cache: every read of a corner, a quad record or a transformed vertex costs 5 or 6 cycles. A replay loop that reads 25 words a quad is about 260 cycles a quad whatever is baked, and an off-screen quad still costs about 100. The old path is about 1400 cycles a quad, so each visible quad does win, but only about 70 floor quads are drawn a frame. (3) The other kinds of geometry would gain the same order: walls (37 prims), items (96) and roofs have more work per prim but many more dynamic cases (cut-away walls, see-through buildings, doors, animated items). A full bake of everything might take 100 to 130 hb off the Sims camera at best (mean about 670 to 690): that keeps a 20 solid (786) with room for a bigger crowd or longer view, but is far from 524 (30 fps), and 25 is not a locked rate (2.4 vblanks).
Learned, for the next try: (a) `setVector` is a macro that evaluates its first argument three times, so `setVector(&v[n++], ...)` is wrong; (b) quads behind the eye must be dropped before the cutter, and only the ones the loops would have tried may reach it (a first version sent 130 a frame there and cost 90 hb); (c) the gain is largest zoomed out (44 hb, most floor in view) and negative in first person, where nearly everything is cut anyway; (d) a test build with under about 5.5 KB free past `_end` hangs at boot.
Test tool added: `-DAB_TR1=1` (with `AB_TRACE`: keeps only each frame's need, 640 frames; 3.4 KB less RAM in a test build; `trtable.py` reads either layout).
What is left: the cutter (95 hb) and the items' boxes are the biggest flat parts. The one structural idea still standing is the street's fixed furniture (benches, lamps, bins, trees) as pre-lit world-space box lists per block, 8 MB only, one GTE batch per block. Free RAM on a plain 2 MB build is unchanged (11.5 KB).

## Round 4 (Nov 8): measured, tried, nothing kept yet
Tool added: `tools/perf/trtable.py RAM ELF` (held-camera row from an `AB_TRACE` + `AB_LOCK4` run: fps, mean hb, over 786, late, worst; `FROM=n TO=m` picks frames, e.g. a frozen stretch). Run each camera with `abcfg.py` (arg 4 = 1 turns the helper off).

**Read this before measuring.** With the helper ON, two builds that differ by a few hb drift apart (the helper settles at different steps): one frozen street frame came out at 789 hb (late every frame) in one run and 562 hb in another, with the same picture. For A/B of real cost use the helper OFF (`abcfg.py DISC CAM 2 1`): frame by frame identical, and a result is real if the mean moves. Use helper ON only for the final table.
Also: in this harness 8 MB and 2 MB cost the same with the helper off (Sims camera 794 vs 795 hb, same people near: 3.7). The 8 MB gap seen with the helper on comes from the helper's history, not from the crowd.

Tried and measured (helper off, mean hb, Sims / zoomed in; base 795 / 926), none kept:
- Projecting the cutter's corners and midpoints on the GTE (identity matrix while a cut is drawn): 803 / 952 and 795 / 940 (worse; the matrix save/restore and the GTE wait cost more than the two divides).
- Throwing a back-facing face out in nc_quad before projecting it: 795 / 924 (no change: the faces asked for are almost all front-facing).
- Nearer car LOD (2800 to 1800): no change (766 vs 763).
Counts, Sims camera, per frame: 91 cut quads become 66 pieces (floor 15 to 14, walls 58 to 47, flat 15 to 4); zoomed in, 112 quads, 259 piece calls, 617 projections. The cutter is flat work: no single step to save, so fewer pieces needs a different rule, not tuning.
Spikes: with the helper on, 53 of 925 frames were over 786 and only 4 of them had a logic spike (over 150 hb); 48 are heavy streets (world 500 to 680 hb with the helper off). Spreading the planner and route searches would not move the late share.
Not done: shared-corner/merged floor quads (merging gains nothing: a cut piece is already smaller than a tile wherever a tile is cut), baked block geometry (the camera moves every frame, so only colours can be baked, and those already are), the household model cache (in the city route there is one full model on screen), 30 fps (mean 600+ against 524).

## Round 3 (Nov 8): strict warp back, a locked 20, spikes, light
Measured the held-camera way (`AB_LOCK4`, the same city frame by frame, 960 city frames a run, helper on, default VIEW). "over 786" is the share of frames whose own cost is past 3 vblanks; "late" is the share that really take 4 or more vblanks (cost plus the flip). Worst is the dearest frame after the first two of a life (the life's start builds the city behind the loading fade). Frame rate is what the frames would give if each takes at least the lock (before: 2 vblanks, so light frames ran at 30 between the 20s; now 3).

| camera, RAM | before: fps / mean hb / over 786 / late / worst | now: fps / mean hb / over 786 / late / worst |
|---|---|---|
| Sims, 2 MB | 19.2 / 650 / 6.2% / 12.2% / 1275+ | 19.6 / 606 / 2.9% / 5.7% / 1000 |
| zoomed in, 2 MB | 19.6 / 655 / 5.4% / 8.8% / 1075 | 19.6 / 621 / 2.9% / 6.2% / 1130 |
| zoomed out, 2 MB | 19.4 / 648 / 6.8% / 11.1% / 1145 | 19.5 / 622 / 3.7% / 7.6% / 985 |
| first person, 2 MB | 27.1 (30 and 20 mixed) / 420 / 0.6% / 1.8% / 1025 | 19.9 locked / 461 / 0.7% / 1.6% / 895 |
| Sims, 8 MB | (the old build did not run as 8 MB in this harness: same as 2 MB) | 19.3 / 628 / 6.2% / 10.2% / 1150 |
| zoomed in, 8 MB | same | 19.0 / 643 / 9.9% / 15.5% / 1070 |
| zoomed out, 8 MB | same | 19.3 / 635 / 5.6% / 11.6% / 1150 |
| first person, 8 MB | same | 19.9 locked / 461 / 0.7% / 1.6% / 895 |

With 8 MB the town holds more people near you (4.8 strangers in view against 1.7; the crowd costs 55 hb against 14), which is why 8 MB is a little dearer. Real-time runs of the same route (not held) drift apart between builds and can not be compared frame by frame; they show the lock: before, 5 to 20% of city frames ended at 2 vblanks (30) between the 20s (that is the flicker); now none do.

**1. Strict texture-warp cutting back** (`NC_WDIV` 6, `NC_WW` 44, `NC_WH` 36, as before the loosening). Normal play cuts a textured piece again while its far corners are more than 1/6 deeper than its near ones and it is wider than 44 or taller than 36 px: the affine texture is then under a pixel off at a piece's middle, so floors do not swim as the camera moves. The loose rule (1/4, 96 x 72) is used only on the speed helper's very last step (`nc_loose`), and only for pieces 1100 or more units from the eye. Check: `docs/ps1_warp_strict.png`, a frozen frame with the camera slid by whole tiles (`AB_FREEZE` with `AB_PAN`), cut as now, as last round, and cut very fine (`AB_PAR="0=16,1=12,2=9"`): the strict picture is nearer the fine one than the loose one is (mean difference 2.4 against 2.7 to 4.4, and less of it on the floor). Cost of strict: about 25 hb a frame on the Sims camera. A cut now goes across the way the depth changes (not the longer side): the same accuracy with fewer pieces (zoomed in 37 hb cheaper, other cameras equal).

**2. A locked rate.** The speed helper now owns the frame rate (`h_lock`): 3 vblanks a frame whenever it holds 20, so a light frame no longer ends a vblank early and flips the picture between 30 and 20. 30 (2 vblanks) is tried only after 256 frames in which at most 2 ran over 500 hb and at least 600 frames since the last change; it gives up only the far view's steps, and goes back to 20 after 6 frames that cannot hold it. Nothing in the city reaches that today (first person is 25% over 500 hb), so first person is a locked 20 with more detail than before (the helper used to give up everything to chase 30). The helper also keeps more margin at 20: a step is given up on 2 late-ish frames in 16 (was 3), when the mean passes 646 (was 756) or when over 3% of the last 64 frames were late (`late_w`), and given back only under 596 and under 1% late. The OPTIONS text for FRAME RATE is unchanged (STEADY 30 now means 30 where it holds, else a locked 20); in menus and other states the cap is as before.

**3. Spikes.** The test trace (`AB_TRACE`, every frame's need and its logic, world and people parts) showed 24 city frames in 966 with a logic spike of 130 to 400 hb (a housemate's route search is 150 to 400), and most frames past 786 were either those or a slow stretch in a heavy street (world 560 to 600 hb against 340). A housemate's new decision (`ai_think`, which may start a search) now waits, up to 90 steps, for a lighter frame (`sim_load_hi`: last frame over 580 hb; the player is never held). Logic spikes: 24 to 10 a run. Not done: the townsfolk planner's own spikes (up to 240 hb), the window's move (up to 270 hb, already done in spare time when it fits), save/load and menu opens (they sit behind fades and pauses; not measured here).

**4. Light.** The day curve was a line with a step in it (full day from 6:00 with a 6% jump) and the same warm-less blue sky all day. Dawn is now 5:00 to 6:30 and dusk 19:30 to 22:00 on a smooth curve, the horizon goes gold and rose in the middle of each (no weather) and the sky top a little. Two integer curves a frame, no extra cost, and the baked floor light only re-keys as the curve moves (a few times an hour). `docs/ps1_daylight.png`: before above, after below, at 5:30, 6:00, 19:00, 20:30, 21:30 (first person, to see the sky).

**What did not work** (measured, left out): one division instead of two in the cutter's projection (no change: the divide runs while the CPU does other work; its samples are waiting for the result); letting small faces skip the cutter by a screen-size test before entering it (20 hb dearer: more inline code in a 4 KB instruction cache and different pieces); an error-based cutting rule (cut until the texture's error is under a fraction of a pixel, whatever the size): as dear as the 1/6 rule at the same quality. The cutter is 95 hb a frame on the Sims camera and 155 zoomed in; the profile is flat everywhere else, so the missing 100 hb a frame is not in one place. What is still a real candidate: pre-built block geometry on 8 MB (floor and wall pieces kept per block), fewer cutter pieces by cutting along the floor's two axes once per tile and sharing corners between tiles, and the household's three full models (87 hb).

**Test tools added**: `AB_TRACE=1` (per-frame need and parts in `dbg_tr`), `AB_PAN=n` with `AB_FREEZE` (the frozen camera slides n tiles every 160 frames), `AB_PAR="7=n"` in an `AB_CFG` build (the clock fixed at n x 6 minutes), `AB_PAR="0=,1=,2="` (the cut's depth ratio and size). RAM: 11.5 KB free past `_end` on a plain 2 MB build (12.5 before: the helper's counters); an 8 MB disc boots to the title.

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
- **Things stay put.** The speed helper shrinks the full-detail range to 5 tiles in town, and nothing outdoors past it was drawn, so benches, lamps and fountains a few steps away blinked in and out as you walked. Outdoor things are now drawn out to the FULL DETAIL setting whatever the helper does (`world_item_r`; about 0.8 fps in town). The floor's texture is also cut against the PlayStation's flat texture mapping further out (`NC_NEAR_TP` 1500 to 2600 units, about 0.6 fps), so it swims less as you walk. The rest of the PS1 wobble (whole-pixel corners) is the hardware's; in DuckStation, PGXP removes it.
- Far view, second pass: block heights in a table round the eye (49 x 49 blocks, or 129 x 129 with 8 MB, filled as asked), the far ground's lattice heights kept, far buildings read their lot's level pad. With 8 MB the height-corner cache is 8192 entries. Third pass: the far street bands are tested against the view (half of them were behind you) and drawn to 160 m, not 450 (past that they are under a pixel). First person with a 1 km view: 4.7 to 11.8 fps; the default camera with the far view forced on (radius 10, helper off): 8.75 to 12 fps; the far pass 824 to 273 hblanks.
- Walkers ask `world_covered` (is this floor tile under the far end of a bed or a bench?) for every tile they look at, every step; on the ground floor the answer is now kept as bits until the map changes (`mark_dirty` now bumps `world_stamp` too). About 17 hblanks.
- Where town frames go now (default camera, helper on, 18 fps, 75% within 3 vblanks): logic 112 (townsfolk 77), floor and walls and items 328, roofs 80, people 154 (of which the crowd's sprite renders, which fill the frame up to the next vblank), HUD 58. The slow frames are the ones whose own need is past 785 hblanks, where the sprite fill then runs to the 4th vblank. No single part is big enough left to win much alone: the next real step is the renderer rework in the list below (light worked out once per tile and kept; static geometry kept per block), and the logic at 30 steps a second.
- Free RAM past `_end` is now about 19 KB (it was 43): the two caches and the new option text.

## The near world reworked: light baked, cars, the cutter (city street, default camera, helper on)
Same route, 2 MB, normal runs at three lengths (3100 / 3700 / 4300 frames). "Need" is a frame's cost without the crowd's spare-time sprite renders (what decides whether it makes 3 vblanks); "fit" is the share of frames whose need is within 3 vblanks (785 hblanks).

| | fps | mean need (hb) | fit 3 vblanks | the world (hb) |
|---|---|---|---|---|
| before | 18.31 / 18.06 / 18.36 | 740 / 745 / 721 | 78 / 75 / 80% | 418 / 425 / 421 |
| 1. floor light baked | 18.38 / 18.18 / 18.44 | 734 / 739 / 715 | 78 / 76 / 81% | 413 / 420 / 416 |
| all below | **18.80 / 18.62 / 18.80** | **692 / 691 / 682** | **84 / 86 / 88%** | **366 / 371 / 376** |

With 8 MB (DuckStation regtest with `DS8MB=1`, 3700 frames): 18.07 before, 18.62 after.

**A cleaner A/B: `AB_LOCK4`.** A faster renderer changes how many frames end on the 4th vblank, which changes the game steps the route gets, so two builds soon see a different street (the townsfolk cost moved by 8 hblanks between builds with the same logic) and a few hblanks of gain hide in that. Built with `-DAB_LOCK4=1` (test builds only, in `gfx_flip`), every frame takes 4 vblanks and runs 3 game steps whatever it costs, so both builds see the very same city frame by frame (their logic readouts match to the hblank) and the difference in the frames' own cost is the change's. 5200 frames, about 960 city frames:

| step (each on top of the last) | mean need | fit 3 vblanks | the world |
|---|---|---|---|
| before | 763 | 71% | 424 |
| floor light baked + the cutter's corners projected once | 734 | 77% | 396 |
| cars: drawn only on the screen, each box's corners transformed once | 690 | 87% | 391 (cars count with the people) |
| ground heights without checks in the draw, the street's things past the range as a list | 687 | 87% | 384 |
| the profiler's timer read inline | 679 | 88% | 377 |

What was done:
1. **The floor's light, baked** (`flc_*` in `world.c`). A ground tile's (or 2x2 block's) four corner shades (its slope's light, the checker, the contact shading by the walls, the grass's and the paving's tint) are kept until the light steps (`gfx_light_key()`: the ambient and sun strength the day sets, and the grade), the map or its shading changes (`world_stamp`, `ao_gen`), the heights change (`gh_gen`) or the window moves; a tile seen again is a table read and one call that writes the packet with colours from the grade table (`gfx_grade_lut()`). 16 x 16 tiles with 2 MB (1.25 KB: the town's 11 x 11 range fits), 32 x 32 with 8 MB (`xalloc`). Only the ground storey; the colour of a tile cut at the near plane is still worked out (it is a few tiles). **It gains only about 5 hblanks**: the light was not where the cost lived. `gfx_light` and the grading together were about 3% of the CPU, and the walls' and items' light was already worked out once a frame per direction. Baking the walls' tops and skirting the same way was tried and measured no gain, so it was left out; the roofs' light was already kept per slope.
2. **The near-plane cutter projects each corner once** (`nc_proj`). A piece cut in two used to project all its corners again (two divisions each); now a corner is projected when it is made and the pieces keep it, and a piece with no corner behind the plane is not copied through the clipper. Same output. About 23 hblanks.
3. **Cars** (`items_draw_vehicle`, `sk_box` in `items.c`; step 3, found by the profiler). The traffic and parked cars (up to 14 + the parked ones, each 15 to 25 boxes) were drawn without any test against the view, every face through `gfx_quad_f` (4 corners transformed per face, 20 a box). Now a car off the screen is skipped whole (a ball of 660 round it, lifted onto the hills as the model is), and a box's 8 corners are transformed once and each face tested on the CPU before it is coloured. Same output. About 44 hblanks of need: the biggest single gain.
4. **Static per-tile work kept** (step 2): the ground heights read straight from their cache through the draw (`gh_live`: no zone, window and range checks per corner), and the street's things past the full-detail range (benches, lamps, bins) kept as a bit a tile until the map, its buildings (`bld_gen`) or the window change, so the loop reads 8 tiles at a time instead of testing every tile of a 25 x 25 square. About 7 hblanks.
5. **The profiler's own cost**: `PROF_BEGIN`/`PROF_END` time every floor tile, wall and item, and each called `VSync(1)`; they now read the hblank counter inline (the difference of two readings is the same). About 7 hblanks.

**Pictures**: `AB_FREEZE=N` (test builds, `main.c`) stops the city after N life frames and keeps drawing it; with `AB_LOCK4` two builds then stand on the very same frame. Laid over each other at three freeze points (a crossing close up, a street with cars and the crowd), the only pixels that differ are the falling blossom (it moves with the frame count) and one crowd sprite re-rendered a frame later. First person and zoomed in all the way were run too (same route, 3700 frames, normal runs; their shots show nothing amiss): first person (`AUTO_CAMTEST=4`) 24.3 to 25.9 fps, zoomed in all the way (`AUTO_CAMTEST=2`, where the cutter works hardest) 16.7 to 18.2 fps (the world 488 to 401 hblanks, 55% to 80% of frames within 3 vblanks).

**RAM**: a plain Release build has 13.7 KB free past `_end` on 2 MB (it was 16.2 KB): the floor cache 1.25 KB, the rest code. The street's things' bits (800 bytes) are in the PlayStation's 1 KB scratchpad at 0x1F800000, which nothing else uses (the SDK's MDEC decoder would; the game does not use it). Both 2 MB and 8 MB boot to the title and run the city (regtest `DS8MB=1`).

**Not done, and why**:
- *Per-block prebuilt packet lists.* The camera moves every frame in the city, so a block's packets would have to be transformed again anyway; what could be kept is the world-space geometry (roof corners, item boxes), and that needs several KB a block: only 8 MB has it, and the gain left is small (the roof corners and item box set-up are 10 to 15 hblanks together). Worth doing only for 8 MB, after the logic.
- *Wall pieces kept per tile*: a wall tile's neighbour tests cost about 1 hblank a frame in town; not worth the RAM.
- *The cutter's view-space transform on the GTE*: it would round differently from the C it replaces when the hills lift the ground (`gfx_lift`), so it was left in C.
- *The crowd's sprite fill* (`citizens.c`, being reworked elsewhere): when a frame's need drops, the fill takes the freed time up to 30 hblanks short of the vblank and sometimes overshoots it; about 0.4 fps is lost that way before and after (fit 86% "should" be 19.1 fps).
- Where town frames go now (normal run, 3700): logic 120, the world 371 (layer-0 tiles 280: floor 96, walls 37, items 97; roofs 75), people and cars 144 (with the sprite fill), HUD 58. What is left in the world is flat again: the transforms, the cutter (about 40 hblanks), the items' boxes (about 50), the roofs' corners and gables (about 40).

## What "Crash Bandicoot style" still needs (not done)
Crash got its speed by baking everything that does not change and streaming fixed-size zones. What that means here, in the order it should be done:
1. **Bake the light.** Done for the floor (above); it was worth 5 hblanks, not the 2.5 hblanks a primitive first thought: the cost is the transform, the clipping and the per-tile tests, not the light.
2. **Per-block prebuilt packet lists.** Walls, roofs and ground of a block are static: build a block's list once (the far skyline in `far.c` already works this way) and copy it, so a block costs one copy, not a pass over its tiles. (See "Not done" above: with the camera moving it is the world-space geometry that can be kept, which needs 8 MB.)
3. **Slice the logic.** Done for the townsfolk a different way (not halves: a scheduler that keeps the people who know you at full rate; see "The townsfolk's scheduler" below). The household's needs are still worked out every step (8 sims, about 22 hb).
4. **Stream by block from the disc.** The city's tables are one set in RAM. Split the street grid into block files with the next ring read in the background (the CD is already read in the background for music). This needs 40 to 60 KB for a block buffer, which means freeing it first from `lvpool_small` (96 KB) or `seqbuf` (45 KB). Free RAM now: about 28 KB past `_end` (keep that above 8 KB or boot crashes at "AUDIO+DISC 60%").

## The townsfolk's scheduler: 5 Bores or 500, the same time a frame, and the ones who know you first
The townsfolk (`citizens_update` in `citizens.c`) cost 81 to 86 hblanks a frame in the city (73 to 83 before the near-world rework), and the cost grew with every catch-up step a slow frame forced (each step was a pass over everyone). Far people were "made cheaper" by skipping their steps (every 2nd past 14 tiles, every 4th past 26), which also meant they lived at half or a quarter speed: a chore, a walk, a shift took them two or four times as long. The new scheduler thinks about each person as often as they matter, but every person lives every step.

**The tiers** (worked out once a frame for everyone in the map window):
| tier | who | how often they think |
|---|---|---|
| 0 busy with you | chatting, fighting you, making peace, at your party, coming over to you, the one in front of you, anyone within 3 tiles | every logic step |
| 1 know you (or you them) | met you (any interaction: the pie menu, a punch, joining a fight, a peacemaker, a party guest), anyone with feelings about you either way (friends, rivals, exes: `rel`/`rom`), your moved-out Bores, your block's neighbours, coworkers at your job | every logic step, wherever they are in the window |
| 2 strangers near or in sight | within 10 tiles, or on the screen out to 16 | once a frame, living the frame's 1 to 3 steps in one go |
| 3 strangers far | the rest | every 4th frame (past 26 tiles every 8th), living all the steps since |

Meeting someone marks them for good: `F_MET` in `Cit.flg`, and in SAN BOREAS `Per.met` (it rides in the top bit of the saved `hp` byte, so the save format is unchanged and older saves read as "not met").

**Living several steps in one call** (`step_core(c, i, n)`): timers run down by the steps, a walk covers the ground of k steps in one move when the way is clear (checked every 24 units along it, so no corner a single step could not pass is cut, and only once the walker faces where it is going; turning is still a step at a time), `go`/`scripted` stop short of their point, the clock check (`decide`) runs if its moment fell inside the steps, the speech bubble and cool-downs run out by the steps. A call returns the steps it used; what is left is lived on the next call, so time is never lost, only grouped (`step_one` takes up to 8 strides a call; past 64 owed steps the rest would be dropped, which happens for well under 1% of steps even in the worst test).

**The budget** (`CIT_BUDGET` 64 thinking calls a frame, about half an hblank each): tier 0 always; then tier 1 every step if it fits, else once a frame, every 2nd, ... (round robin, each living the steps it missed); then tier 2, then tier 3 (their periods stretched the same way). A frame that builds a new body (the crowd model, about 200 hb, `CIT_BUILD_COST`) has less to give out, and a clock (`CIT_HB_CAP`, 80 hb) puts off the strangers in a frame that has run long (they go first next frame, from a different place in the list each frame, and never past half a second behind). Bodies are now only built for people within 18 tiles of the camera (the rest never needed one: they are never drawn).

**Big decisions spread over frames**: a route search (a 25 x 25 breadth-first fill) or a look round the home for something to do costs one of `CIT_THINK` (4) tokens a frame; the last one is kept for tiers 0 and 1; whoever finds none stands and thinks until the next frame. A search that found no way is not repeated until the walker is on another tile (some homes put a chore where it cannot be reached; the old code searched again every step, hundreds of times a minute). Going to bed looks for the bed once, not every step. In the household (`sim.c`), one sim makes a real decision a step: two who come free together are a 60th of a second apart.

**Off the map** (`pop.c`): the people of SAN BOREAS who know you are kept on a list (128; the strongest feelings, if more). `pop_tick` brings 8 of them into the window a frame before its usual 40 others (all 128 in 16 frames, where a stranger's turn comes round every 90 frames); if every body is taken, a stranger out of sight makes room for someone who knows you. Their days already follow the clock (`pop_where`); a coarse clock now keeps the rest going: once a game hour one of them who has not seen you for two days may phone (at most a call every three hours; it warms them to you and lifts your social need), and at midnight those not seen for three days drift (friendship cools by 1 a day, a grudge eases by 1, romance cools after five days; never back to strangers). Out on the streets, someone who knows you and sees you out comes over to say hello, once each time they are out (`CM_GREET`).

**Measured** (AUTO_CITY Release, the city route at three lengths, on top of the near-world rework above; "before" is that rework alone):
| | before | now |
|---|---|---|
| townsfolk a frame (hb) | 86 / 84 / 81 | 50 / 46 / 45 |
| all game logic a frame (hb) | 121 / 120 / 117 | 88 / 82 / 81 |
| frames a second, whole stay | 18.80 / 18.62 / 18.80 | 19.78 / 20.72 / 21.53 |
| frames within 3 vblanks | 84 / 86 / 88% | 95 / 96 / 97% |
| townsfolk: 95% of frames under | 240 hb | 100 / 100 / 80 hb |
| townsfolk: the worst frame (a body built) | 275 hb | 243 hb |
| thinking calls a frame | 42 | 6.7 to 8.5 |
| steps actually lived by the townsfolk (a 4300-frame run) | 38,600 | 93,000 (all of them) |

Part of the gain is the catch-up steps: a cheaper frame runs fewer of them (2.99 steps a frame before, 2.72 to 2.93 now). Measured before the near-world rework went in, the same change was 83 / 77 / 73 to 51 / 49 / 49 hblanks for the townsfolk (18.3 to 18.9-19.2 fps). (The traffic's slice reads 3 hb higher only because the interrupts that used to land in the longer townsfolk slice now land in it; it does no more work.)

**Scaling** (CIT_PEOPLE=5: PACKED, 999 homes on 2 MB, BIG households; 45 to 78 people live in the window; 4300 frames):
| | before | now |
|---|---|---|
| townsfolk a frame | 99 hb | 68 hb |
| townsfolk: 95% of frames under | 240 hb | 200 hb |
| frames a second | 19.02 | 21.93 |
| everyone in SAN BOREAS knows you (CIT_KNOWN=1; about 50 in the window, all tier 1) | - | 81 hb, 19.41 fps: every one of them thought about every frame |

**Behaviour checked** with `CIT_PROBE` builds (`tools/perf/citprobe.py` prints the run's counters and a snapshot every 512 steps) against `CIT_FLAT` (the same code with everyone thought about every step, one step at a time): chores 29 / 28, route searches 23 / 24 (none failing), walks ending held up 62 / 64%, ending by the time 7 / 8%, held-up steps a walk 1.8 / 1.9. With `CIT_STAND` people who know you came over to say hello in the street; with `CIT_LIFE` three days off the map moved all 158 people on the list (drift) and the phone rang. The share of steps spent walking differs from run to run (15 to 23%) with who happens to be in the window (shop workers walk slowly in tight rooms); the old scheduler's far people lived a quarter to a half of their steps, so they did fewer chores and walked less in a day than they should. Test hooks (Release, with AUTO_CITY): `CIT_PEOPLE=n` (the PEOPLE dial), `CIT_KNOWN=n` (every nth person knows you), `CIT_STAND=frame` (stop the stroll there, and now and then bring three people who know you near), `CIT_LIFE` (with `CIT_KNOWN`: three days and a day's hours of the off-map clock at the start).

**Checked again like a player would** (`AUTO_LE=3` with AUTO_CITY and `-DPLACE_MAX=1300`, no one put near you): a home bought in the capital, four people met out on the street and bought drinks (the pie menu: FRIENDLY..., BUY DRINK) until they liked you at 42, a walk round town passing each of them on the pavement 5 tiles off, four days in the farthest town at full speed, then home and the walk again. The phone rang 3 to 4 times in the four days ("CAY CALLED TO CATCH UP"); on the walk 4 of them set off to say hello and 3 got to you ("HEY! IT'S YOU!"). It found a slip: the greeting's walk had its time and steps written inside a comment, so the time never ran down, and it went in a straight line (a bench, a lamp or a fence between held it up). Now it walks round things (`nav`, then straight up for the last 2 tiles) and its steps count. Counters: `dbg_greet` (set off, arrived, gave up: time, held up, you too far).

**RAM**: a plain Release build ends at 0x1fd5d4, 10.5 KB free (13.7 with the near-world rework alone, 16.4 before both). The new per-person state (`owe`, `flg`) sits in what was padding in `Cit`, so the crowd's table did not grow; the known list is 384 bytes; the rest is code. To stay above 10 KB the townsfolk's step code, route walking and party dancing are built small (`COLD`, -Os): they now run a few dozen times a frame, not hundreds, and the measured cost did not change.

**Not done**: the household's own needs still tick every step (22 hb; all 8 sims know you, so they stay at full rate by the rule above). The body builds (about 200 hb, in `crowd.c`/`voxel.c`) cannot be split over frames without touching the crowd renderer; they are now rarer and the strangers make room for them. The island and the town (50 people, no `pop.c`) use the same tiers but have no off-map life.

## Toward a steady 20: the frame's spare time, honest readouts, RAM
**New readouts** (`ramprof.py`; test builds):
- *Frame periods*: how many vblanks each life frame really took. The old "fit 3 vblanks" figure was optimistic: its "need" left out the work after the sprite fill (the sprites and shadows drawn after it), and frames the fill itself pushed over were not seen. At the start of this round the city route had 95 / 96 / 97% "fit" but 8.1 / 6.1 / 4.8% of its frames really took 4 vblanks or more.
- *Need* is now measured, not predicted: the frame's time less the work done only because it fitted the spare time before the vblank (`cit_spare_hb`).
- *By the clock, like for like* (with `AB_LOCK4`): the vblanks each frame would have taken from its need, and from its flip with the spare-time work: an fps that two builds can be compared on over the very same route. Also: the household's (`sim_step`) cost a frame as a histogram, the people's drawing split (crowd, its spare-time work, household, traffic), how long the flip waited for the GPU (never, at the default camera: the CPU is the limit), and the slow frames now say what their need was.

**1. Bodies and sprite renders in the frame's spare time** (`citizens.c`).
- A body (the full voxel model, ~190 hb: `voxel_prepare`'s shaping 124 of it, the look 35, the face 18, the store 5, the crowd model 7; measured over 22 builds) was built in the update, nearest first, a whole one in whichever frame wanted it. Now only someone who would be drawn this very frame and has none is built there; anyone else who wants one (coming into range, off the screen, or whose stored model was let go and is drawn plain meanwhile) waits for a frame with the time to spare, where `citizens_draw` builds them before the sprite renders. After a second and a half without such a frame the nearest is built in the update anyway. Builds in the update went from 25 to 6 frames of the like-for-like route; the townsfolk's 95th percentile 100 to 80 hb, and the strangers put off by the clock 254 to 89.
- The sprite fill checks the clock before each render, against what is left to do after it (the last frame's time from that point to the flip, the most of it lately), instead of a count worked out up front; someone's first sprite (they are drawn plain until they have one) also waits for time, but never more than 3 frames; one stale sprite at least every 6th frame as before. The vblank aimed for is the one the frame's own work reaches (never sooner than the FRAME RATE option lets it), judged from the least it could still need: filling a frame on to the next vblank when it would have made this one costs a whole vblank, and the first version of this change did exactly that to most of the 30 fps frames (153 to 24 on the route). Frames the spare-time work pushed past their vblank: 24 to 8 (like for like).
- Route searches ask about a blocked tile once (marked when first asked, not again from each side): the household's 80 x 80 search (`sim_find_path`) and the townsfolk's 25 x 25 one; same tiles in the same order. Only about a tenth of a long search: the household's searches are still the worst single frames (15 a route, 7 of them over 100 hb, the worst 940).
- Measured: like for like, need 19.34 fps either way (the work moved, it did not shrink), flip 19.21 to 19.27. Normal runs (3100 / 3700 / 4300 frames): 19.78 / 20.72 / 21.53 to 20.14 / 21.00 / 21.95 fps; frames taking 4 vblanks or more 8.1 / 6.1 / 4.8% to 6.4 / 5.1 / 4.0%. With 8 MB (3700): 20.82 to 21.00 fps, 6.3 to 5.0%. Pictures: a frozen city (`AB_FREEZE` 300 and 700, `CIT_FLAT` so both builds' townsfolk think alike) differs from before only in the falling blossom (it follows the frame count). Without `CIT_FLAT` the townsfolk's routes drift apart from the first build that moves (a body built a frame later changes who is in front of you, and so who thinks every step).

**RAM** (plain Release, 2 MB): 10.8 KB free past `_end` before this round (10.6 with the phone map's properties, merged meanwhile), 19.0 KB now:
- `far.c`: the far land's cell table moved with the eye through a 4 KB copy of itself; it is now shifted in place (each row read before it is written).
- `voxel.c`: `voxel_prepare`'s working tables (the corners WC 4 KB, the padded blocks and the corner-shaping caches 1.8 KB) live only inside it: they now sit in the route finder's table (`bfs_par`, 12.8 KB), which is already lent the same way to the floor drawing and the roof rebuild (none of them runs inside another). Checked: the same pictures (this move alone on the old code, frozen at 700 life frames: only the blossom differs).
- The new code above costs about 1.5 KB.

**A word on normal runs.** The city route is chaotic: the player is left alone and YOUR BORE ALONE takes over, so who walks where depends on the frame timing, and two builds soon walk different streets (the same build can take a cheap route with a third of its frames at 30 fps, or an expensive one with none; the household's searches and the world's cost differ by 40 to 70 hb between the two). Normal-run fps between builds differ by up to 2 fps from that alone. The like-for-like "by the clock" figure under `AB_LOCK4` is the one to compare builds on; normal runs show whether the real thing agrees.

**2. The speed helper, retuned** (`helper_frame` in `main.c`). It aimed at 30 fps (a 500 hb load) whatever the FRAME RATE option said, and counted the crowd's spare-time sprite fill as load, so in a town it sat at its most the whole time. Now:
- **A ladder** of what is given up, in this order: the far view a step at a time (trees and detail, reach, then all of it), the full-detail range a tile at a time down to 5 (the street's things stay out to FULL DETAIL), then the crowd (half; a quarter, the nearest in full 3D only). The crowd's size used to be cut with the far view (8 people drawn at its most); it is now the last thing to go.
- **The target follows the FRAME RATE option**: a steady 20 (786 hb) with STEADY 20; 30 (524) with STEADY 30 (the default) or AS FAST AS IT CAN, as long as 30 can be held at all. Where even the whole ladder does not hold 30 (a city street), it settles for a steady 20 and gives back whatever fits under that; it tries 30 again once the frames clearly fit it (mean under 464 hb for 240 frames).
- **Load is the frame's need** (its time less the spare-time work) plus any wait for the GPU. A step is given up when 3 frames in 16 run past the target (one alone is a spike: a route search, a block coming into the window; giving detail up does not help it) or when the mean does; a step is given back after a quiet spell (240 frames for a far-view step, 120 for the others) with at most one slow frame in the last 32, and only if its cost fits 60 hb under the target. The cost is learned each time a step is given back (the mean 32 frames later against before; the far view's from its own timer). A step taken again within 300 frames of being given back waits twice as long next time (halved again once it holds 900 frames): no flicker.
- Measured (normal runs, 3700 frames except where said): default camera, the same as before (it still cannot hold a steady 20 there: 8% of the frames' need is over 3 vblanks even with everything given up, so it stays at its most); like for like 19.27 to 19.16 fps (the first second in the city: it gives the far view up first, then the range, where the old one cut both at once). First person (STEADY 30): 26.9 to 27.6 fps (85% of frames at 30). First person with STEADY 20 (5200 frames): 19.7 fps, 3.2% of frames over 3 vblanks, the crowd mostly back (step 0.6 of 2) and the range 5.7 tiles, 16 steps given up and 7 back in 1280 frames; the far view stays off (it would cost more than the room). Zoomed in, zoomed out: as the default camera.

**3. Walkability kept, card colours, the far streets' heights kept.**
- `world_walkable` (asked by every walker, every route search and every straight-line check, for every tile, every step) reads the ground floor's maps straight (it went through `world_at` and `world_at_l` for each tile), a bare floor tile's answer being `world_covered`'s kept bit. (Keeping its own answers as bits was tried too: checked against the plain answer, no difference in 105,832 calls, and about 0.07 fps more like for like, but 1.9 KB of RAM; left out for the RAM.)
- `ui.c`: a card's edge and its shadow are one colour top to bottom; the blend that rounds a card's corners now skips its divisions for them (the HUD's cards are drawn about 6 times a frame, 4 rounded boxes each). The HUD measured by parts (a test build): the Bore's card 15 hb, the needs 8.5, the clock and money 8, the want, story and notes 8, the season, weather and blossom 5.4, the portraits 3.3.
- `far.c`: the far streets' bands start and end on a block's corner or 3 tiles off it; their heights are kept by block (25 x 25 round the eye, 3.7 KB, the table wrapping round as the eye moves, only the row or column coming in forgotten; keyed on the land's epoch, `world_gh_epoch`, and the big map's edit stamp). They were four calls into the land's height code a band, about a hundred bands a frame in first person with a long view; the 2 MB height table thrashed on them (146 misses a frame, 49 now). The cone test's counter is now a test build's only.
- RAM: the six texture builders' own buffers (the far windows and tree cards, the soft shadow, the ghost, the items' materials, the walls' and floors' textures, the roof shingles: 9.5 KB kept for good, each used once or now and then) build in the upper half of the atlas's 32 KB upload scratch (`atlas_scratch_hi`; the upload copies into the lower half, and the route searches and the roof rebuild that also borrow it use only the lower half). Plain Release, 2 MB: 20.8 KB free past `_end` (it was down to 9.2 KB after this step's tables and the other changes merged meanwhile).
- Measured: like for like (city street) 19.16 to 19.19 fps by the clock; first person with the 1 km view (helper off) 13.23 to 13.86 fps on 2 MB (the far streets 189 to 131 hb), 13.82 to 13.86 with 8 MB. Pictures (frozen, first person with the far view): only the blossom and two of the crowd's idle poses (they follow the frame count) differ.

**4. A sprite render that does not fit is forced half as often while the helper has the crowd cut.** A crowd sprite render is a whole person drawn (`voxel_draw`, 93 hb on average over 986 draws, 71 quads after culling, the worst 154; with the impostor's set-up and the model's unpacking about 135). In a town frame the spare time (about 100 hb) never fits one, so the sprites there were refreshed by the guarantee alone (a stale one every 6th frame, someone's first sprite at least every 3rd), and each of those pushed its frame past the vblank: 27 stale and 4 first-sprite renders in the 961 frames of the like-for-like route, against 182 that fitted. At the helper's crowd steps (it has already cut the crowd for speed) the stale guarantee is every 12th frame (`cit_stale_n`): 15 forced, like for like 19.19 to 19.27. A new readout counts the renders (in spare time / forced first / forced stale).

## Where it stands after this round
Like for like (the same route frame by frame, `AB_LOCK4`, 5200 frames, "by the clock" fps from each frame's flip; 2 MB unless said; the helper on, as a player has it, except where it says off):

| | before this round | now |
|---|---|---|
| default camera | 19.21 | 19.24 (runs of one build differ by about 0.05) |
| zoomed in all the way | 19.14 | 19.24 |
| zoomed out all the way | 19.24 | 19.28 |
| first person | 26.03 | 26.56 |
| first person, 1 km far view (helper off) | 12.85 | 13.58 |
| default camera, 1 km far view (helper off) | 14.16 | 14.17 |
| default camera, 8 MB | 19.21 | 19.28 |
| first person, 1 km far view, 8 MB | 13.21 | 13.76 |
| frames the spare-time work pushed past their vblank (default camera) | 24 | 6 to 8 |

Normal runs (3700 frames) of the final build: default camera 19.17 fps (10% of frames took 4 vblanks), zoomed in 19.09, zoomed out 19.14, first person 27.28 (83% of frames at 30), first person 1 km far view 13.82, default camera far view 14.24, STEADY 20 at the default camera 19.18; 8 MB default camera 19.19. These walk the expensive route (see "A word on normal runs"): the numbers at the top of this round (19.8 to 21.5 at the default camera) were the cheap one, so the two cannot be set side by side.

Plain Release, 2 MB: 20.8 KB free past `_end` with this round's changes (10.8 at its start, 9.2 at its lowest with the merges); 15.6 KB after the FARM LIFE work merged at its end. Both 2 MB and 8 MB boot to the title.

**Why the city street still is not a steady 20.** With everything the helper can give up (far view off, 5-tile range, 8 people drawn) the frames' need at the default camera is 666 hb on the mean and 7 to 8% of frames need more than 3 vblanks (786 hb). They are not spikes: most are 790 to 850 hb, the heavier stretches of the route (the world 450 to 500 hb, the people 180 to 230). What is left in a frame, from the profile of the busy time: the world 41% (the per-tile loop 7%, the roofs about 11%, the near-plane cutter 8%, the floor 5%, the walls 4%), the street's things and furniture 13% (cars 5% of it), people 10% (the player's own full model alone is about 90 hb when it is drawn full), the HUD 50 hb (the Bore's card 15, the needs 8.5, the clock 8, the want and notes 8). It is flat again: no part is big enough to win the last 50 hb alone.

**Not done, and why:**
- *The HUD's packets kept from frame to frame* (redraw only what changed): about 30 hb to win, but the blossom, the weather and the notes' slide are drawn between the cards (moving them on top changes the picture), the cache needs a region of each packet buffer kept for it, and its key is half the game's state. Left for a round of its own.
- *The near-plane cutter's colours packed four to a word, its projection by one division*: worth maybe 10 hb; the projection would round differently from the C it replaces.
- *A cheaper person* (`voxel_draw`, 93 hb): it is already corners-once, colours from a table; what is left is per face. Faster renders would let the town's spare time fit sprite refreshes again.
- *Window shifts* (a block coming into the map window: about 1% of the route's frames, 1150 hb each) and *the household's long route searches* (6 frames in 960 over 440 hb, the worst 1190): spreading either over frames needs the map or the search kept half-done across frames (the search's table is lent to the drawing every frame).
- *The far view in first person*: the far land is now 320 hb (the streets 131, the map window's own ground past the detail 108, the land past it about 80), the far buildings 119, the trees 65: 13.6 to 13.9 fps. The next step there is the land kept as a grid of shared corners (each corner transformed and hazed once, not by each of its four quads), and the map window's ground cells walked by their visible span only; neither fits the time left in this round. The helper gives the far view up in first person (27 fps) and, with STEADY 20, does not give it back (it would cost more than the room).
- *A bigger height table on 2 MB*: the far view's 1024-entry table now misses 49 times a frame in first person (146 before); 2048 entries would be 6 KB of RAM.

How to measure any of this: tools/perf/README.md.

## Things in their places, the ground to the window's edge, and every camera measured (this round)
Pictures: `docs/street_lamps_before_after.png` (the Sims camera and zoomed in, the same frozen moment) and `docs/city_view_before_after.png`
(left before, right after: the Sims camera, zoomed in, zoomed out, first person).

**What was wrong, and the fixes (root causes in the shared code):**
- *Street lamps in the road.* `gen_block` put them at block tiles (3,3), (10,3), (3,10) and a bin at (6,3): in the lanes and on the
  crossings, left from when the street was 3 tiles wide (it is `BM_LOT0` = 5, the pavement row/column 5). They now stand on the pavement at
  its kerb edge, turned to face the street (`F_KERB`: a pole at the tile's street-side edge, people walk past it), by the corner, half way
  and at the far crossing, never within two tiles of another lamp or a bus stop. Bins only where people can step round them. A lot's tree,
  plant or bin left on the pavement moves a tile in (`kerb_clear`). Lamps and bins an older generator left on the road and a save kept are
  taken off it (`street_clean`). The river's bridge carries the pavement, with a lamp at each end. `tools/perf/placecheck.py` (any city RAM
  dump) lists things on the road: 69 before (48 lamps, 9 bins), only the gas station's forecourt things after.
- *Everything outdoors slid against the ground on the hills.* The ground floor, walls, people, cars and the crowd stood on the hills'
  ground (`world_gh`), but items (lamps, benches, bins, trees, planters, fountains, furniture: `items_draw` after `lift_to(0)`) were drawn
  at the flat's height. In a town on a hill (the AUTO_CITY town stands about 530 units up) every item was drawn 2 tiles under the ground, so
  on the screen it sat off its tile and slid against the floor as the camera moved: most of the "off kilter" look (a planter seemed to
  stand on the crossing). Items now stand at the middle of their tile on `world_gh` (`item_lift`), upstairs on the storey's pad.
  `gfx_set_object` no longer adds the drawing's lift on top of the ground under the object (one ground height: a bus at its stop or a car
  drawn while a lift was set would have stood twice as high).
- *First person on a hill looked at the ground from below.* The eye was left at the flat's height (under the hillside: the street showed as
  a ceiling). It now stands on the ground under you, and the third-person camera picks up from there.
- *Benches* on their own lie along the walk they stand on (they always lay along x).
- *The lean of tall poles* near the screen's edges is the ordinary perspective of a camera looking down 56 degrees with a 70-degree view
  (verticals meet below the screen), the same in the Dreamcast port; the per-object matrices are rotations about y only (no shear, uniform
  scale). What made it look wrong was the poles standing 2 tiles under the ground (above). A longer lens (a narrower view with the camera
  further back) would straighten it more; not done, it changes the framing of everything.
- *The ground ended a few steps away.* With the far view off (chosen, or given up by the speed helper, which it always is in a street),
  nothing was drawn past the full-detail range (5 tiles in a town): the sky showed through and the street's things past the range stood in
  it. Now the window's own ground is always drawn to its edge (`ground_win`: 8 x 8-tile cells on the hills, the corners transformed once,
  about 10 hb).
- Cheaper: roofs far off in 4 x 4 quads where they are one plane (2 x 2 past 7 tiles as before, 4 x 4 past 13), the pad read once a frame
  (it was read twice), no contact shadows on the street's things past the range.
- Tried and dropped: the near-plane cutter's warp splits as one even grid projected by the GTE (no recursion): more pieces, no faster.
- A test hook, `AB_CFG` (`tools/perf/abcfg.py`): camera, view distance and helper patched into the disc image, so one build runs the whole
  table. The camera now holds the whole run (the old route switched to a high look over the roofs at step 2000 whatever was asked, so the
  older "zoomed in" and "zoomed out" numbers were mostly that camera).

**Every camera at every VIEW DISTANCE** (like for like: `AB_LOCK4`, the same route, 5200 frames, fps "by the clock" from each frame's flip):

**2 MB, speed helper on (as a player has it)**: before / after

| camera | OFF | 250 M | 500 M | 1 KM | 1.5 KM | 2 KM |
|---|---|---|---|---|---|---|
| the Sims camera | 19.51 / 19.59 | 19.39 / 19.55 | 19.39 / 19.55 | 19.39 / 19.55 | 19.39 / 19.55 | 19.39 / 19.55 |
| zoomed in | 19.25 / 19.37 | 19.23 / 19.33 | 19.23 / 19.33 | 19.23 / 19.33 | 19.23 / 19.33 | 19.23 / 19.33 |
| zoomed out | 19.31 / 19.46 | 19.26 / 19.41 | 19.26 / 19.41 | 19.26 / 19.41 | 19.26 / 19.41 | 19.26 / 19.41 |
| first person | 26.83 / 28.14 | 26.62 / 28.11 | 26.62 / 28.11 | 26.62 / 28.11 | 26.62 / 28.11 | 26.62 / 28.11 |

**2 MB, speed helper off**: before / after

| camera | OFF | 250 M | 500 M | 1 KM | 1.5 KM | 2 KM |
|---|---|---|---|---|---|---|
| the Sims camera | 18.06 / 18.93 | 15.91 / 17.15 | 15.91 / 17.15 | 15.91 / 17.15 | 15.91 / 17.15 | 15.91 / 17.15 |
| zoomed in | 17.54 / 18.69 | 13.59 / 14.01 | 13.59 / 14.01 | 13.59 / 14.01 | 13.59 / 14.01 | 13.59 / 14.01 |
| zoomed out | 15.01 / 14.91 | 14.11 / 14.36 | 14.11 / 14.36 | 14.11 / 14.36 | 14.11 / 14.36 | 14.11 / 14.36 |
| first person | 24.23 / 26.14 | 14.37 / 14.74 | 14.08 / 14.31 | 13.81 / 14.06 | 13.31 / 13.65 | 12.24 / 12.50 |

**8 MB, speed helper on**: before / after

| camera | OFF | 250 M | 500 M | 1 KM | 1.5 KM | 2 KM |
|---|---|---|---|---|---|---|
| the Sims camera | 19.50 / 19.59 | 19.40 / 19.55 | 19.40 / 19.55 | 19.40 / 19.55 | 19.40 / 19.55 | 19.40 / 19.55 |
| zoomed in | 19.06 / 19.37 | 19.23 / 19.33 | 19.23 / 19.33 | 19.23 / 19.33 | 19.23 / 19.33 | 19.23 / 19.33 |
| zoomed out | 19.31 / 19.46 | 19.26 / 19.41 | 19.26 / 19.41 | 19.26 / 19.41 | 19.26 / 19.41 | 19.26 / 19.41 |
| first person | 26.70 / 28.26 | 26.60 / 28.11 | 26.60 / 28.11 | 26.60 / 28.11 | 26.60 / 28.11 | 26.60 / 28.11 |

**8 MB, speed helper off**: before / after

| camera | OFF | 250 M | 500 M | 1 KM | 1.5 KM | 2 KM |
|---|---|---|---|---|---|---|
| the Sims camera | 18.06 / 18.93 | 15.81 / 17.26 | 15.81 / 17.26 | 15.81 / 17.26 | 15.81 / 17.26 | 15.81 / 17.26 |
| zoomed in | 17.54 / 18.69 | 13.72 / 14.15 | 13.72 / 14.15 | 13.72 / 14.15 | 13.72 / 14.15 | 13.72 / 14.15 |
| zoomed out | 15.16 / 15.09 | 14.15 / 14.35 | 14.15 / 14.35 | 14.15 / 14.35 | 14.15 / 14.35 | 14.15 / 14.35 |
| first person | 24.24 / 26.11 | 14.43 / 14.85 | 14.15 / 14.47 | 13.86 / 14.15 | 13.43 / 13.77 | 12.43 / 12.74 |

The one step back: zoomed out with the helper off and the far view OFF (15.01 to 14.91): the ground past the range is drawn there now, where
the sky showed before. First person "before" looked at the underside of the ground for much of the route (the eye under the hill). With the
helper on, the VIEW DISTANCE makes no difference in a street: the helper gives the far view up at once (it costs more than the frame has), so
every column reads the same.

**25 frames a second: not reached, except in first person.** The third-person cameras run at 19.3 to 19.6 with everything the helper can give
up. 25 fps needs about 60% of frames within 2 vblanks (524 hb); their mean need is 640 hb at the Sims camera (logic 82, the world 364:
floor 67, walls 53, items 73, roofs 95; people 143, the HUD 50). No part is more than about 15% of the frame, so no single change gets there:
it needs the renderer kept as built geometry (walls, roofs and ground per block, transformed and emitted without the per-tile work: 8 MB has
the room, 2 MB has about 11 KB free) and the logic at 2 steps a frame. First person runs at 28 with the far view given up; with the far view on
it is 12.5 to 14.8 (the window's ground cells, the far streets and the far buildings: about 450 hb).

**RAM**: plain Release, 2 MB: 10.7 KB free past `_end` (`ground_win`'s corner tables use the route finder's table, lent as the floor drawing
does). 8 MB runs the same (`DS8MB=1`, every camera).

**Left to do**: the block entering the map window still costs about 1150 hb in one frame (about 1% of frames): making the 4 new blocks one a
frame needs the window's maps kept half-built; per-block visibility worked out ahead (occlusion), built geometry per block, and a cheap far
view for first person. The DS port has no copy of the street generator (its lots are made per lot), so nothing to fix there.

**RAM, REAL TIME (2026-11-01)**: plain Release, 2 MB: 10.2 KB free past `_end` (9.0 KB before). The REAL TIME clock costs about 2.4 KB resident (its SET CLOCK screen is the overlay CLOCK.OVL); paid for by lending the route queue (`bfs_q`, already lent to the room fill and the save) to the route walk-back (`rx`/`ry`, 1280 bytes), the room fill's seen-bits (801) and the neighbourhood pick's reservoirs (384); the boot log in the neighbourhood model's room (640, unused until after boot); the housemates' LOD cache sized to MAX_SIMS (16 to 8: 448); `dbg_slow` only in AUTO_CITY builds (512).

## Long view and the 25 fps round (2026-11-01)
Asked for: a much longer draw distance, Crash Bandicoot style, and 25 fps or better at every camera and every VIEW DISTANCE, on 2 MB and 8 MB.
**Result: first person makes it (26.8 to 27.4 with the far view drawn, any VIEW DISTANCE). The three third-person cameras do not: they sit at 19.7 to 20.0 (a steady 20) and cannot get to 25. Why is at the end.**

**What was done**
1. *Blocks loaded over frames.* This was already so (a block a frame, roofs queued, the move itself in spare time); measured on the city route the worst window move is 250 hb, not 1150. What was left was the household's route search (worst frame 557 hb of logic): it is now A* (`route_astar` in `sim.c`: the tiles nearest the goal first, buckets by estimate, a route that needs a detour of hundreds of tiles says "no route"), and a long path is straightened only for about 40 hb of the frame, the rest follows the search's own runs. Worst household frame 557 to 379 hb; the searches look at a few hundred tiles, not the whole window. Routes can differ by a tile from the old breadth-first ones, so a city route that is walked after this build is not the same street frame by frame as before it (all "before" numbers below were taken on the older route).
2. *Pre-built static geometry.* Not built as a per-block packet list; see "Not done". What was baked instead: the far land's lattice heights (kept by lattice point), the far building list packed to 5 bytes a building (it was 8: **7 KB of RAM back**) and a tallest-building byte per far cell so a cell too far for even its tallest building to show is passed over whole.
3. *Fewer logic steps.* Looked at, not changed: the townsfolk's scheduler (last round) already does one pass a frame for strangers and every step only for people who know you; the household's 8 sims are all "know you", so a step stays 21 hb (18 sim, 37 townsfolk, 9 traffic for 3 steps). Nothing was cut that would change who thinks when.
4. *Hide blocks behind buildings.* Not done (see below). The far view skips what cannot show instead: land groups and buildings outside the view's cone, a far cell whose tallest building is under two pixels, and 4 x 4 groups of land are tested for the cone as one.
5. *A cheaper far view for first person* (`far_lite`, the speed helper's LITE step): the ground and the buildings only (no streets, no trees), buildings flat from 100 m, still fading into the haze, still the whole VIEW DISTANCE. The land is drawn in blocks on the world's own grid, 2 x 2 groups (308 m) past 350 m and 4 x 4 (616 m) past 800 m (the full far view: 600 m and 1200 m), one quad each from the lattice's alternate corners, the colour of the block's middle cells; a block is never re-aligned as you move, so it does not shimmer. Far land at 1 km in first person: 219 hb to 69 (lite); lite at 2 km: 190 to 106. The map's own ground past the detail range is drawn by `ground_win` with the haze on, so there is no seam where the map ends.
6. *The speed helper* (`helper_frame`, `helper_apply` in `main.c`). The far view's steps now are: as chosen, three shorter ones, LITE, and only then the full-detail range, the crowd, and, last of all, the far view gone (and that last step only when the frames really are slow: the mean over target + 30 hb, not target - 30). LITE keeps the whole VIEW DISTANCE (it used to be capped at 1 km). So in a town the far view stays drawn (in LITE) at every VIEW DISTANCE as long as the frames fit; the VIEW DISTANCE row says "SIMPLIFIED" when it is on LITE and "CUT" when gone.
7. *The HUD drawn only when it changes.* The needs card and the Bore's card are kept (`gfx_keep_begin/end` in `gfx.c`): each is drawn once into its own corner at the end of the packet buffer (5 KB of it, not cleared between the two frames that share it) and linked into the chain again while its key (every number and word on it) is the same. HUD 51 to about 35 hb (the clock, the money, the weather and the notes are still drawn each frame). Not on the Dreamcast (`BORE_DC`: no keep).

**Other things that gave frames**
- *The near-plane cutter* (`nc_piece`, 5 to 14% of all CPU in a street): a textured piece is cut again for the PlayStation's flat texture mapping while its far corners are over 1/4 deeper than its near ones (was 1/6) and it is over 96 x 72 px (was 44 x 36); a piece is cut for the GPU's 1023 x 511 limit only past 880 x 460 (was 560 x 330). On a frozen city frame (`AB_FREEZE`, no route drift) the world went from 452 to 426 hb at the Sims camera and from 480 to 412 at the zoomed-in camera. The floor's texture swims a little more close to the camera. The knobs are test-patchable (`AB_PAR`, `tools/perf/abcfg.py`).
- First person lite far view and everything else in it is cheaper per primitive than the full far view: the far land was already about 1 hb a quad; the cost left is the count.

**Measured** (`AB_LOCK4`, same route, 5200 frames, fps "by the clock" from each frame's need; the helper on, as a player has it). Before = the build at the start of this round (also on the older household route). After = this build.

2 MB, VIEW DISTANCE OFF / 250 M / 1 KM / 2 KM:
| camera | before (any VIEW: the helper cut the far view) | after OFF | 250 M | 1 KM | 2 KM |
|---|---|---|---|---|---|
| the Sims camera | 20.84 | 20.04 | 19.71 | 19.71 | 19.71 |
| zoomed in | 19.34 | 19.89 | 19.94 | 19.94 | 19.94 |
| zoomed out | 20.52 | 19.95 | 19.71 | 19.71 | 19.71 |
| first person | 27.47 (far view gone) | 27.43 | 27.30 | 27.43 | 26.79 |

First person with the far view held at full (helper off): 250 M / 1 KM / 2 KM: before - / 14.46 / 12.90, after 17.84 / 15.60 / 14.55. The Sims camera, helper off, 1 KM or 2 KM: 17.54 (before 15.91 to 17.15). The full far view is still under 25 in first person: it is what the helper's LITE step is for.

8 MB, helper on (same code, `DS8MB=1`): the Sims camera 20.04 / 19.71 / 19.71 (OFF / 1 KM / 2 KM), zoomed in 19.90 / 19.94 / 19.94, zoomed out 19.95 / 19.73 / 19.73, first person 27.43 / 27.43 / 26.77. The same as 2 MB: nothing in this round uses the extra RAM.

**Which still miss 25, and why.** The Sims camera, zoomed in and zoomed out: 19.7 to 20.0. Frames are whole vblanks (60, 30, 20, 15 a second), so 25 means about 60% of frames within 2 vblanks (524 hb); the mean need of a town frame is 650 hb, 5% of frames are under 524. What is in a frame at the Sims camera (helper on, range 5, crowd cut): game logic 70, the world 400 (the floor, walls and things 290, the roofs 72, LITE far 40), the people 150 (the player's own full model is about 85), the HUD 36. It is flat: no part is over 15%, and the largest single one (the cutter) was cut by about a fifth. To get to 25 there is about 130 hb to find, which is more than every part but the world's tile loop; it needs the renderer kept as built geometry (item 2, 8 MB only) and the player's model at a lower detail, not tuning. First person is the one that fits: its world is 220 hb with the helper's cuts.

**Not done, and why**
- *Per-block prebuilt packet lists (item 2) and ahead-of-time block visibility (item 4).* The camera moves every frame, so a block's packets would be transformed again anyway; what could be kept is the world-space geometry (roof corners, item boxes), a few KB a block, and the profile says the gain is 25 to 40 hb. Visibility ahead of time (which blocks a building row hides from a street camera) would cut the far view only; the far view is already cheap in LITE. Both want 8 MB for their tables and a build step for 2 MB; not started.
- *Logic at fewer steps* (item 3), see above.
- The clock card and the weather are not kept (a third kept piece costs 1.3 KB of the packet buffer, which leaves the busiest frames (a party, 54 KB of 58) too little room).

**RAM (2 MB, plain Release)**: 12.8 KB free past `_end` (the far buildings' list went from 8 to 5 bytes each, 7.2 KB, paying for the new code and the start of this round, which was already down to 8.8 KB). 8 MB runs the same code.

**Tools**: `tools/perf/abcfg.py DISC CAM VIEW [HELPER_OFF [PIN]]` (PIN holds the far view's step: 1 as chosen, 2 to 4 shorter, 5 LITE, 6 none; with it the helper does not run) and `AB_PAR="k=v,..."` for the tuning bytes (`AB_PAR(k, default)` in `world.c`); `ramprof.py` prints the packet buffer's peak. For a clean A/B of a renderer change build with `-DAB_FREEZE=250` and compare the world's hb: the city stands still after 250 frames, so both builds draw the very same frames (the normal route drifts by about 1 fps between builds). Pictures: `docs/ps1_hud_kept.png` (the kept HUD cards in the city, two moments), `docs/ps1_far_lite.png` (a low camera in a street, far view gone on the left, LITE at 2 km on the right: in a street the buildings fill the view, so the two look alike apart from the sky's edge; nothing odd at the join where the map's ground meets the far land).


## Hair (11 new styles, built as a few shaped boxes)
Hair is no longer coloured cells on the head grid. `src/hairmesh.inc` draws it as a handful of shaped boxes (6 to 14 a style) on the sculpted head (`hair_draw`, called from `voxel_draw` beside the face art). Each box is a `HP` row (x, y, z extents, a top and bottom inset for bevels and steps, a texture and flags) in a table shared by every person, so a style costs bytes of ROM and nothing per person. Built as 8 corners, one `gfx_xform`, five quads (top and four sides), lit per face and tinted dark at the underside and light at the tips. The old cell hair (`hair_block` and its caches) is gone, which also drops the per-person cell count.
- Styles 0 to 14 keep their meaning in saved looks; 15 to 25 are new (BUZZ, SIDE PART, SLICKED, BRAIDS, WAVY, HORSESHOE, THINNING, MESSY, TOP KNOT, CURTAINS, MULLET). `NHAIR` is 26, the save is unchanged (look[] is stored raw, so `VERSION` did not change). `look_port.inc` is generated from the GBA game but is now hand-edited for hair: slot 13 marks style blocks, `hairVox[]` maps the new styles to the old builder, `hairTones` is retuned (brown, blonde, auburn, grey and the dyes) and `hairAge()` greys elders and pales babies.
- Fringe, sideburns, hair over the ears, back and nape, top volume. Hats hide the crown pieces (`HC`), a helmet hides them all; beards and moustaches are mesh pieces too and still work with every style.
- Textures are small generated grey 16x16 maps (`hair_textures()` in voxel.c) so one set takes any colour; `src/hairtex.inc` and `tools/make_hairtex.py` are gone.
- Test hooks: `-DHAIR_DEV=330` (Create-a-Sim camera up to the head), `-DHAIR_AB15` (dice make only the old 15 styles, so an A/B meets the same people).
- Measured (AUTO_CITY + AB_LOCK4, same route, new build with HAIR_AB15 against the old build): 3100 frames 19.78 / 19.75 fps (flip), 5200 frames 19.80 / 19.75; no change worth naming. RAM (2 MB, plain Release): 15.9 KB free past `_end`, against 13 KB before (the old hair code and its caches were 5 KB).
- Dreamcast: runs unchanged under its GPU layer; the port's Makefile had fallen behind the PS1 source list (gotchi_act, arc_core, ag_*, arc2, clock_ui, pack_ui), now added, with the arcade's two overlays' `ag_fn` tables merged in `dc_plat.c`.
