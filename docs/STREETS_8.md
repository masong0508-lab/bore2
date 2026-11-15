# World version 8: street variety and street signs

New worlds are version 8 (`bm_gen` 8, save version 55, DS `opt[38]` 8). Version 7 and older saves keep their towns exactly (townmap `make check` for 2 to 6; a gen 7 save reads as gen 7).
Everything is in `src/bigmap.c` (`g8_*`), so PS1, Dreamcast and DS (it compiles bigmap.c) follow one plan.

## How the streets are made

The engine keeps its 18 x 18 block, the 5 tile street strip along a block's top and left, and the 5 x 5 junction square at a block corner (traffic, kerbs, lots, far view, the DS map all rely on it).
Variety is made of what can be varied inside that: which block edges carry a street.

- Each neighbourhood (4 x 4 blocks between avenues) takes one of 20 street shapes (`g8tA/B/C`, 24 bits each: the 12 inside H edges and 12 inside V edges), turned over by two dice.
  Suburb and estate: classic row, dead ends from either side (with a turning head), a bend, an S jog, a staircase that runs diagonally, a ring road round the middle four blocks (the park or plaza that sits there),
  comb lanes off an avenue, T and L shapes. Urban and civic keep the grid more often, with T, comb and ring variants. Green: nothing, a spur, an L.
- The avenues step one block aside (-1, 0, +1) on a stretch one neighbourhood long (`g8_av`, half the stretches). Where two displaced avenues meet, and where an inside street meets a displaced
  avenue, edges are added (`g8_edge`) so everything still joins up. Downtown (city core, main street) and villages keep the plain grid; nothing moves next to them or at the town's edge.
- A street that ends gets a paved turning head (the existing corner bit, 5 x 5), only where the street that arrives is really there (not cut by a big place). A bend gets the same square.
- Lots front the street along the top of their block or the one below; a block with neither (the green at a dead end, inside a jog) is a park (three in four) or woods.
- No block gets a new size: 18 x 18 stays, so "14/18/24 tile blocks" are not possible without changing lots, traffic and far view. Spans of 18, 36 and 54 come from streets skipping rows, and 36 x 36 from the ring and the big places.
  Roundabouts and plazas at crossings were not done: a junction square is crossed straight by cars on their arcs, an island in it would be in their way.

## Checks

- `ports/host/townmap streets SEED 8` (new): the street graph of every town: pieces not joined to the main crossing, dead ends, bends, tees, crossings, blocks with no street to front on, lots on a street that is not in the main piece (`LISTEND=1` lists the ends and bends).
  Seeds 0x5EED1234, 1, 0xC0FFEE, 7: 0 orphan pieces (gen 7 has 5 to 24 at the town edge).
- `townmap make check` (gen 2 to 6 unchanged), `lotsim GEN=8 audit` (overlap and door as gen 7), `trafsim` (no more violations than gen 7; bends and heads are junction squares with 2 or 1 arm, cars turn round at a head).
- Dreamcast `roadaudit_seeds.sh`: nothing on a road, no street strip unpainted.

## Street signs (PS1 and Dreamcast, version 8 towns)

`bm_junction(bx, by, hn, vn)` gives the streets that meet at a block's north-west corner and their names (`bm_street_line`: one name per street line; an avenue that steps aside keeps its name; addresses use the same names).
`draw_streetsigns` (main.c) finds the three nearest junctions within 13 tiles every 16 frames, projects the corner pole, and draws a green plate with the two names (the existing body font, capitals) as 2D text:
5 to 9 sprites a plate, no VRAM or texture, nothing beyond 13 tiles or behind the camera. `-DSIGN_TEST` draws at any distance. DS: not done (the DS has no 3D street).

## Round 2 results (d84)
- Checks: townmap make check (gen 2 to 6) same; gen 2 to 7 sums identical to 5963b0a over 4 seeds, roomy and packed; townmap streets 0 orphans on 4 seeds; lotsim gen 8 audit as gen 7; trafsim 3 seeds gen 7 vs 8 equal except 3 "person in a car" readings in one gen 8 window (a car braking for a walker).
- Dreamcast roadaudit_seeds (5EED1234, 1, C0FFEE, 7A11CE): PASS, ON-ROAD 0 on all four.
- PS1 Release `_end` 0x801f405c (limit 0x801fb000), signs included. Saves: version 55 reads as gen 8, 54 and older as before; DS ds_oldsave PASS (d78 save loads as gen 3), ds_flow save/load PASS on a gen 8 world.
- DS: ROM bore2_dsi_d84.nds (a7.bin from tools/dsbuild), ds_town7_audit PASS (162 homes), ds_traffic PASS. No street signs on DS (no 3D street).
- Pictures: docs/gen8_town_map.png (whole city, gen 8), docs/gen8_dc_*.png (Dreamcast, list ports/dreamcast/test/gen8_places.inc, -DAUTO_AT_G8=1). Limit: camera spots are not exactly on the turning heads and few cars show.
