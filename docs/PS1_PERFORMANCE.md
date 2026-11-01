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

How to measure any of this: tools/perf/README.md.
