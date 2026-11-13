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
