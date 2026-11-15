# DS world version 7 (d79)

Note: DS worlds before d79 were world version 3 (bm_gen was never set), not 6. A d78 save has opt[38] = 0 and still loads as version 3 with its old town.

## What it does
- bm_gen = 7 at boot, on a new world and on a new life (a new life after an old save rerolls the world as gen 7). The save writes opt[38] = 7 for gen 7 lives. src/bigmap.c is unchanged.
- ports/dsi/source/ds_town7.h is the DS side. Plan homes from LotPlan: ranch, L (with garage wing), box, two-storey (loft and stairs), cottage, terrace pair, mansion. Yards: walk, porch, drive, front hedge row with gaps, mailbox, yard tree, street trees, back-yard patio, bbq, beds, swing. Rooms by docs/HOUSING_DESIGN.md 4c: kitD (divided: bed and bath along the back, living | kitchen and dining), kitR (ranch), kitC (cottage / terrace home).
- North-facing lots are built south and turned over (flip7), as the PS1 mirrors. Streets, pavements and corners come from the plan's road bits (st7/world7); far lots and the ring use the plan footprints (far_foot7, far_lot); the Map lot picture (draw_lot) uses the plan's footprint, wing, drive and door side.
- Cars only run where the street runs the whole way; citizens spawn on the plan's streets.

## Differences from the PS1
- The DS plot is 13 x 13 cells, one cell per PS1 tile, with walls on cell edges: rooms are roomier than the PS1's.
- 2 x 2 baths hold a toilet, a bath and a plant (a corner fixture would have no free side); the sink is only in the bigger baths.
- The far land is dropped when the frame is over POLY7 (2450) quads, to hold the frame rate.

## Numbers (melonDS, tools/dsemu/tb.sh live scene)
- d78: 29.6 fps avg (29-30), 2973-3201 quads sent.
- d79: 29.0-29.5 fps avg (26-30), 2440-2670 quads sent (before the POLY7 budget: 27.8 fps, 2891-3119 quads; the cost was the far lots and the extra street ground, not the CPU: the ARM9 profile is the same as d78).

## Tests
- Audit: `bash ports/dsi/test/ds_town7_audit.sh` (build -DDS_T7_AUDIT): 186 plan homes of the map (all 7 types, both facings): 0 without bed, toilet or fridge, 0 overlaps, 0 off the plot, 0 with the street not reachable from the front doors, 0 bed/toilet/fridge/stove/sink/bath without a free side. It found four layout faults (2 x 2 bath corner, dining set shutting a 5-deep ranch's kitchen, a chair at the bed's foot, a plant in the living room's only gap), all fixed.
- Save round trip (melonDS, AUTO_SCENE=140, scenes 143 write / 144 read): a gen 7 save reloads as bm_gen 7; the same build with -DDS_OLDSAVE (writes opt[38] = 0 as a d78 did) reloads as bm_gen 3.
- ds_town.sh (stable townies): same checksum after write and reload, different for another seed.
- ds_music.sh: the POKE of the runner does not reach the game (the emulated ARM9 cache) and the default /tmp/dsrun has no SPULOG, so it said FAILED (6) on d78 too. Now: `HOOK1=1 ROM2=<-DDS_MUSIC_TEST=2 build> bash ports/dsi/test/ds_music.sh <-DDS_MUSIC_TEST=1 build> "" <dsrun with SPULOG>`: PASS (sample and synth parts).
- Test hooks: -DDS_AT_BX/BY (start lot), AUTO_SCENE 171-173 camera shots (high, middle, close).

## Shots
docs/ds_shots/plan7_suburb_street.png, plan7_suburb_top.png, plan7_town_centre.png, plan7_interior.png, plan7_north_house_top.png, plan7_north_house_close.png.

## Left
- Upper-storey (loft) audit; ds_oldsave.sh with a real d78 build; the lot picture for terrace and L shapes has not been looked at in the emulator.

## D80
- Room engine (ds_town7.h, h7_pl / h7_ok): things go in by what they are and how big the item table says, against a wall, turned the way they are used; every put keeps all free cells reachable from the front door and a free side for every thing, or is taken back. Strict audit (t7b, 11 checks) all 0 on 186 homes.
- Roofs from the plan (plan_roof in main.c, the PS1's roof_region): hip for RF_HIP and mansions, gable otherwise, colour (pal + rcol * 3) % 6, pitch 170 for compact and ranch, 250 for the rest. Used in the lot, the Map picture and the far lots. Old (gen 3) towns keep the dice.
- Lawn rule as the PS1's gen_block: in a gen 7 world a home (cottage .. two-storey) and a park are lawn even in a sandy town (live lot ground, far lots, Map picture).
- ds_items.h regenerated from the PS1 item table (object scale: four poster 2 x 2, nightstand, wardrobe, clock, bunk heights).
- Test hooks: -DDS_TOUR (+ -DDS_TOUR_ROOF to show roofs), -DDS_ACT_TEST (every item used by a sim: 170 used, 8 decor reached with no effect, 11 refused by age), -DDS_ACT_HOMES (every thing in every plan home reached from the front door); tools/dsemu/fuzz.py (random keys and touches, stall detector), lotdump.py.

## Town layout: the maps, the model and the far view draw what the world builds (PS1, after 1d58d31)
Picture: docs/town_model_before_after.png (the moving-in screen's model, same block and camera: left before, right after).
- One source: `bm_lot_foot(bx, by, &b, f, &plan)` (bigmap.c) gives the walls of what stands on a lot as block tiles f[0..3] (f[4] = 1 for a home): a home's body from the town plan (`bm_lot_plan`), else the lot's table row `lfoot`, turned over (rows 5 .. 17) when the lot faces north (not inside a mega block). 0 = nothing built (park, field, pool deck, empty lot). `bm_lot_home` is the home half (older worlds: the body their old generator builds). `world_roof_of(bx, by, &b, ft, plan, &sty, &col, &pit)` (world.c) is the roof rule roof_region uses: sty 0 hip, 1 gable, 2 flat, 3 gable turned to the street; `world_roof_rise`, `world_roof_rgb`, `world_plan_wall_rgb` give its height and colours.
- Far view (far.c): footprints, storeys, roof form (gable, hip, front gable, flat), roof colour and a plan home's wall material now come from those; fields have no barn, pools no building, observatory/space boxes are the real ones.
- Town model (main.c dio_*): streets are the real 5 tiles with pavements and corner paving (none inside a mega block), the river has banks and a bridge only where the street crosses it, farm = crop rows plus a barn, field = crop rows only, pool = paved deck, and every building is drawn from the plan: off-centre, set back, a home's garage wing, drive, door, roof form and colours. Computed per block when drawn (`dio_bld`), no table kept: BSS is what it was.
- World fix found on the way: a north-facing cinema had its back wall across its street door (lots7.inc mirror7 now leaves the street-door row alone).
- Tests: `ports/host/lotsim plan SEED` (every home: body, door, drive against the plan: 0 differ), `lotsim foot SEED` (every building's generated walls against `bm_lot_foot`: 0 differ, GENs 5 to 7), `ports/host/townmap make check` (gen 2 to 6 pictures unchanged). Test hook AUTO_NHOOD_BX/BY/PITCH/DIST/YAW opens the moving-in screen on a block (with AUTO_CITY).
### What DS needs (ports/dsi was not touched)
- Take `bm_lot_foot`, `bm_lot_home`, `world_roof_of` (and `roof_pick`) with bigmap.c/world.c; far_new_world and the lot-picture/Map code should draw the walls f[0..3] and the roof (sty/col/pit) from them instead of a fixed footprint table (`foot`/`has_bld`): no barn on a field, no building on a pool, north-facing lots turned over, an L's wing and a terrace's second half inside the body's box.
- A far gable: two slopes up to a ridge along the longer side (sty 1) or north to south (sty 3), the two end walls carried up to it; a hip stays four slopes up to a point. Rise = half the short side * roofPitch / TILE (`world_roof_rise`).
- The Map and lot-picking pictures: a plan home's wall is `world_plan_wall_rgb((pal * 2 + wall) % 6)`, its roof `world_roof_rgb(col)`.
- Take lots7.inc's mirror7 change (cinema door).

## D81 (brought up to what the PS1 and Dreamcast have, main at 323308b)
- ds_items.h regenerated with the repo's generator (mk_dsitems.py now reads items_hedge.inc): new CLIPPED HEDGE (id 205, PS1 0x8E) and HEDGE PIER (206, 0x8F), one box each on the DS (greens of pHg, body 12 high, cap top colour), never for sale (tab 255). Dining table and counter fill their tile.
- Yards (ds_town7.h): rules of HOUSING_DESIGN 11: style from q.pal & 3 (0 hedge, piers, short side hedges; 1 hedge, piers; 2 picket fence; 3 hedge on half the lots), lawn on some lots, hedge on cell row 11 except the walk (dxc +- 1) and the drive, piers at 1, 12, dxc +- 2, dv - 1, dv + 2, side hedges on cells 1 and 12 over rows by1 + 1 .. 10, none in the back yard, yard tree placed first, hedge7 pocket rule (h7_hedge). Fields (gen 7, no street) have crop rows and hedgerows with gaps (y % 3 == 0), no barn.
- Roofs: front gables (RF_FRONT on compact, box, two-storey; roof_shape sty 4, ridge front to back). Parked cars on drives (five homes in eight, nose in, up to six, same rules as pcars_* in src/traffic.c). Off-centre homes, bent streets and dead ends came with the shared bigmap.c and need no DS code (audit 178 homes all 0).
- Camera follow by elapsed vblanks in 8.8 (18/256 a tick, snap under 4 units = 1.5 PS1 units). The DS title is 2D, so there is no title sway to move to the clock.
- Neighbourhood names (bm_nb_name) on the Map card; Map lot picture: no building on a pool (deck), crop rows on a field, north-facing buildings turned; far view: no barn on a field, no building on a pool.
- Found on the way: the yard's things come last in the object list and ran out of polygon budget first (the rooms used it all), so yards were bare. Rooms now stop at 2550 quads, yard things go on to 2900.
- Not done: far-storey LOD (b_lod, lod_up); world_roof_of / world_roof_rgb / world_plan_wall_rgb (world.c is not in the DS build; the DS keeps its own plan_roof and wall table); texture quad cutting (the DS texture unit is hardware, no warp to fix); a hedged-estate shot; ds_music.sh (needs special builds).
- Numbers (tools/dsemu/tb.sh): live 28.9 fps avg (26-30), street/town 28.6, 2470-2730 quads. Tests: ds_town7_audit PASS (178 homes, hedges on 59), ds_town.sh same as before, save round trip gen 7 -> 7 and DS_OLDSAVE -> 3, fuzz seeds 1-3 no stall.
- Shots: docs/ds_shots/d81_*.png.

## D82
- Roof and wall parity with the PS1, mirrored in main.c (world.c is not in the DS build): roof_dice = roof_pick (hip / gable / flat / front gable, colour, pitch), plan homes take the LotPlan roof, the six neighbourhood wall materials (walls 7..12), far.c wall colours for far lots and the Map picture.
- Roofs are cut per rectangle and per storey (cell_top, roof_draw): the L garage wing and the loft now have their own roofs. Mansion stairs moved to column 5 (the flight hit the door). The forced gable on two-storey homes is gone.
- Audit: check 11 "loft unreachable"; the generator checks loft reachability (h7_okl). ds_town7_audit PASS, 178 homes.
- Map lot picture: no stale close-up distance on entering the Map; the terrace shows both homes.
- Decor things with no effect (lamp, rug, fence, lamp post, hedge, towels, puppy, dog bowl) now raise a need (mk_dsitems.py).
- fps: next_cell() ran a full BFS over the 3x3 patch on every call; it now stops at the start (identical steps). sims_step 89 ms/s -> 13 ms/s. Benches 30.0 fps avg.
- Far lots stay one box each; a window-band texture (no polygons) for non-house lots. Real storey LOD not needed. No texture warp: the DS texture unit is perspective-correct.
- Tests: ds_flow.sh (buy/save/load, job), ds_items_test.sh (decor, 16 island/stairs things, every thing in every home), ds_oldsave.sh (real d78 ROM save -> gen 3), ds_music.sh PASS (SPULOG runner).
- Shots: docs/ds_shots/d82_*.png.

## D83
- Double bed: the DS already had it (a couple share the 2 x 2 Family Bed, I_FAMBED, with a nightstand each side and the single bed as the fallback; flip7 moves a thing by its footprint, so a turned-over 2 x 2 bed needs no shift like the PS1's mirror7 had). New: `bed_fix7` (main.c): a saved lot whose four poster / family / single bed no longer fits (far cells meet a wall, a thing, the plot's edge or no floor, from when those items were smaller) gets the one-cell bed instead, on every storey. Old saves load: ds_oldsave.sh with a real d78 build (02071f2) PASS.
- Parked cars are solid (`pcar_solid`, asked by obj_solid): a cell a quarter or more of a parked car stands on is not walkable; never the front door's column. The cars are made once per lot (`pk_build`, reset in nb_load).
- Floor texture UV (PS1 Round H1): not applicable. The DS never bakes 2 x 2 floor blocks; its patterns are a texture coordinate from the world position (ITX in qd), the same size everywhere.
- Moving traffic (PS1 docs/PS1_CITY_LIFE.md A9, adapted to the DS's whole-patch streets): lanes right hand (east y 16, west y 14, south x 14, north x 16 of the slot's 5-cell street; the vertical lanes were left hand before), arc turns (right R 1 cell, left R 3 cells, drawn turned: `rblock`), a stop line 12 units before the junction square, first come first served claim of the square (`car_claim`: cars going straight along one street share it, a turning car has it alone), the way out must be clear, cars yield to the townsfolk and the household (`car_person`, also in the square) and keep a gap (`car_blocked`), they leave at the patch's edge and come back at the far edge (24+ cells from the lot; slots 18 cells, patch edge at -18 and 36) at 1/64 per step, a car held 900 steps goes. A turn is only taken where the street across runs on.
- The first version let two cars frozen past CARN block the others and a finished turn start another: both fixed (found by the audit below).
- Test: ports/dsi/test/ds_traffic.sh (car_audit in DSTEST builds): 7777 steps, 6.6 cars on the road: 0 overlaps, 0 off-lane, 0 junction conflicts, 0 cars held 20 s (first run: 0, 575, 0, 2859).
- fps: live and street benches 30.0 avg, 2473-2518 quads. ds_town7_audit PASS (178), ds_flow PASS, ds_items PASS (act, island, every thing in every home), ds_town A/B/A same, ds_music PASS (SPULOG runner), fuzz seeds 1-3 no stall.
- Shots: docs/ds_shots/d83_*.png (Map, nine homes, double bed close, street with traffic, close floor and wall, zoomed out lot).
