# Housing and town design (world version 7)

Round 2 (roofs, lawns) is section 9 at the end.

Why: towns looked random and packed. Every block had a street on all sides, one lot type was rolled per block
(a gym beside a cottage beside a junk lot), one house in four was a mansion, and every house was the same box in the
same spot of its block. This file turns real planning and house-design practice into rules the generator follows.

Scale: a tile is about 1.2 m. A block is 18 tiles (21.6 m). A street is 5 tiles (6 m). A lot is 13 x 13 tiles.

## 1. What the sources say

**The neighbourhood** (Perry's neighbourhood unit; city blocks; coving)
- Big roads run round the edge of a neighbourhood. Streets inside are local and fewer.
- The school and the park sit in the middle. At least 10% of the land is open space.
- Shops sit at the edge, on the big roads and their corners.
- A block is long. It holds two rows of lots, back to back. Every house faces a street. Back yards meet in the middle.
- The centre of a town is a tight grid. The edge is looser.
- Coving: houses do not stand on one straight line. The setback drifts in and out along the street. About 25% less road, more green.

**The lot** (zoning setback tables; Prairie Trail pattern book)
- Front yard 20-30 ft in a suburb, 10-15 ft in a village, about 0-10 ft for terraces. Side yard 5-10 ft. Rear 10-20 ft. The house covers at most 40% of the lot.
- The garage stands back from the front wall (18 ft or more). The driveway is at one side, 10 ft wide at most.
- A porch may reach 8 ft into the front yard. A walk leads from the door to the street.
- Fences go on the sides and the back. The front is open or has a low hedge.
- A corner lot has two fronts: a fence or hedge runs along the side street.

**The house** (Georgia ranch-house study; Springfield residential guidelines; floor-plan practice)
- Shapes: compact (nearly square), linear (at least 2:1, long side to the street), L (a wing at one end), two-storey box.
- The door is centred on a two-storey box and on a cottage. It is off-centre on a long ranch.
- Zones: public rooms (living, dining) by the front door. Private rooms (beds, bath) grouped away from it. Service rooms (kitchen, garage) together.
- Rooms are no longer than 1.5 times their width. Windows follow the rooms and come in twos and threes.

**The yard** (WSU "Functional Landscape")
- Front = public: lawn, the walk, a tree or two, low planting by the wall.
- Back = private: a square patio by the house, play space, a garden in the sun.
- Side = service: bins and storage, by the garage.

**Variety without chaos** (anti-monotony ordinances; pattern books)
- No two alike fronts on neighbouring lots. At most 20% alike on one street.
- "Different" means roof form (gable or hip, or turned 90 degrees), wall material, colour, window grouping or porch.
- Yet a street shares one character: the same few materials and roof colours.

## 2. The town plan (`bigmap.c`, `bm_gen` 7; older saves keep their towns)

- **Avenues** on every 4th line, both ways. The 4 x 4 blocks between them are one **neighbourhood** of one kind (`bm_nb`).
- **Kinds**: CORE (city centre), MAIN (shopping streets), CIVIC (school, library, clinic, pool, park), URBAN (terraces and flats),
  SUBURB (detached houses), ESTATE (big houses in grounds), GREEN (a common, woods or farms with a few cottages).
- The kind comes from the ring (how far out) and the neighbourhood's dice. Centres are CORE or MAIN; then URBAN; then SUBURB; ESTATE and GREEN at the edge.
- **Streets by kind** (block row j and column i inside the neighbourhood, 0..3):
  - CORE, MAIN, CIVIC: the full grid.
  - URBAN: streets on rows 0 and 2, a cross street at column 2.
  - SUBURB, ESTATE: streets on rows 0 and 2 only. Blocks are 4 lots long and 2 deep, back to back.
  - GREEN: the avenues only. The inside is green.
- The blocks round a town's square (the landmarks) always keep the full grid.
- **Facing**: a lot faces the street on its south side. With none there it faces north, onto its own street. With neither, nothing is built on it (it is green).
- **Lots by kind**: homes only among homes. A pocket park or the school in the middle. A corner shop on an avenue corner.
  A tidy lot for sale now and then. No farms, junk lots or wild blocks between houses: farms and woods are grouped in GREEN.
- Mansions stand only in ESTATE, every second lot, with garden lots between.
- Big places (malls, plazas, campuses) only in CORE and MAIN. A big park may sit in any kind.

## 3. The lot

- The house stays inside the block's 13 x 13 lot. Where a block has no street on a side, that 5-tile strip is lawn, hedge and trees: the space between neighbours.
- **Front yard depth**: URBAN 1-2 tiles, SUBURB 3-5, ESTATE 5-6. It drifts by one tile from lot to lot along a street (a slow wave, not dice).
- A pavement runs on both sides of a street.
- Street trees stand at an even spacing along the pavement. Yard trees stand in set places (a pair in front, one shade tree at the back).
- **Driveway** at one side, 2 tiles wide, from the street to the garage. A car may stand on it.
- **Fence or hedge** on the sides and back of the back yard. The front is open, or has a low hedge with a gap at the walk.
- **Back yard**: a patio at the back wall, then one of: pool, garden rows, play lawn. A shed in a back corner.
- A corner lot gets a hedge along the side street.

## 4. The house

| Type | Lot kind | Body (tiles) | Door | Notes |
|---|---|---|---|---|
| Compact | COTTAGE | 7-8 x 6-7 | centre | one main room and a bath |
| Linear ranch | HOUSE | 11-12 x 5-6 | off-centre | long side to the street |
| L (garage wing) | HOUSE | 8 x 7 + wing 4 x 5 | off-centre | the wing stands 2 tiles back from the front wall |
| Box | HOUSE | 9-10 x 7-8 | off-centre | the plain house |
| Two-storey | TWO | 9 x 8 | centre | windows matched left and right |
| Terrace pair | TOWN | 2 x (7 x 9) | each its own | close to the street |
| Mansion | HOUSE in ESTATE | 11-13 x 10-12 | centre | fountain court in front |

- Rooms: public at the front by the door, private at the back, as before. The kitchen end is the garage end.
- Windows follow the rooms: one per back room, a pair by the front door.
- Neighbours never share a type and roof form. Type, mirror (garage left or right) and roof form step along the street by a fixed rule, so two alike never touch.
- A neighbourhood keeps one palette: 2 wall materials and 2 roof colours.

## 4b. Inside: rooms, not rows of furniture

Furniture used to be strung along the walls in runs. Each room is now built from **groups** placed for what the room is for
(room-layout practice: focal point, conversation group, clearances).

- Every room has a **focal point** and the furniture faces it.
- **Living room**: the TV (or fireplace, or the big window) on one wall. The sofa faces it 2-3 tiles away, a coffee table between, an armchair at a right angle, a rug under the group, a lamp or plant in a corner. Seats face each other or sit at right angles, never all in a line along one wall.
- **Dining**: the table in the middle of its zone with a chair on each side, a clear tile all round. It sits between the kitchen and the living group.
- **Kitchen**: one L or one straight run on the wall: fridge at an end, then counter, sink, counter, stove. A free tile in front of all of it. The kitchen is next to the dining table.
- **Bedroom**: the bed's head against the wall opposite the door (or the longest free wall). A nightstand on each side of a double bed, one beside a single. The wardrobe or dresser on another wall with a free tile in front. A desk and chair under the window in a child's or teen's room.
- **Bathroom**: toilet, sink and bath or shower along the walls, the sink nearest the door, a mirror over the sink. One free tile in front of each.
- **Study**: desk and chair facing the room or the window, bookcase behind or beside, an armchair with a lamp.
- **Hall and doors**: the tile inside every door and a path between all doors stay clear (at least 1 tile wide).
- Things that belong together are placed together (table + chairs, bed + nightstands, sofa + coffee table + TV, desk + chair). Nothing stands alone in the middle without a partner.
- Each room has one floor and one wallpaper. Wet rooms (kitchen, bath) get tile.
- A big front room is split into zones (living | dining | kitchen) in that order from the door side to the garage side, not mixed.

## 4c. What was built (rooms round 7, `src/lots.inc`)

Pictures: `docs/rooms7_before.png`, `docs/rooms7_after.png` (the same 12 homes, drawn top-down by `ports/host/lotsim/rooms.py`; the red wedge is the side a thing faces).
In the game: `docs/rooms7_ps1_*.png`, `docs/rooms7_dc_*.png`, DS `docs/ds_shots/rooms7_*.png`. Numbers: `docs/lots_audit_rooms7.txt`.

**What was wrong before** (seen in the "before" sheet):
- A bedroom was one row on the back wall: bed, dresser, toy box, bunk. The rest of the room was empty.
- A front room had the sofa on one wall and the telly on the far wall, 6 to 8 tiles away. The fridge and a counter stood apart between two doors.
- Most homes had no dining table and no stove. Pianos, guitars and easels were dotted along the front wall.
- A one-room home was one ring: bed, dresser, shower, toilet, fridge, all side by side. The toilet stood in the open room.
- Nothing was turned on purpose: a chair or a sofa faced where chance put it.

**How a room is made now:**
- A **group** is a few things that belong together, drawn against a wall, each piece turned the way it is used, with the tiles it must keep clear.
- A **recipe** lists the groups a kind of room gets, and the walls to try for each, in order. Groups and recipes are small tables, not code.
- A group is only put where it fits, where the room stays in one piece, and where everything in the room can still be reached. If not, the next wall is tried, then a smaller group.
- Groups go down in two passes: first what a group must have, later its extras (second sofa, armchair, nightstand, lamp). So an extra never takes the place of a need.
- The room is read from its door. The living end is the bigger side of the door, the kitchen end the smaller side.

**Per room:**
- **Living group**: telly on a wall, a rug, a coffee table, the sofa facing the telly 2 to 3 tiles away. Extras: second sofa, an armchair at a right angle, hifi or books or a fire beside the telly, a lamp or plant in the corner. Small rooms: telly, rug, sofa.
- **Dining set**: table with a chair on three sides and a way round it; two tables and four chairs for a household of four or more. Small rooms: a small table on a wall with two chairs, or one chair.
- **Kitchen run**: fridge, stove, sink side by side on one wall, clear tiles in front. Extras: microwave or coffee maker, a counter. The run is at the kitchen end, the dining set beside it.
- **Bedroom**: bed head on the wall opposite the door (or the next free wall), a nightstand each side, a rug at the foot, a dresser or wardrobe on another wall. Cot beside the parents. Bunks and a toy box for kids, a desk and chair; a desk with a computer for a teen.
- **Bathroom**: toilet and shower down the wall away from the door, sink and mirror nearest the door, tiled floor. In a divided house it is a narrow room (2 wide) at one end of the back row, so the bedrooms get the width.
- **One-room homes** (cottage, townhouse, two-storey, flat): a bathroom is walled off with its own door in a back corner (right across a narrow flat). Bed in the alcove beside it, kitchen below the bathroom, living group at the other side.
- **Study**: desk, chair, computer, bookcases, an armchair with a lamp. **Games room / gym** (mansions): as before, spread on the walls.
- **Inn and hotel rooms**: bed with two nightstands and a rug, a wardrobe, the telly.
- **Mansion upstairs**: same recipes (bedroom, study or games room, bathroom, a lounge with the living group).
- **DS** (`gen_house`, ROM d78): the same rules laid out by hand for its one house plan. A d77 save loads in d78.

**What every home has now** (world seed 1, 647 lived-in homes; before -> after):
- Homes with no shower: 453 -> 0. No stove: 509 -> 0. No dining table: 611 -> 184. No bed, toilet, fridge: 0 -> 0.
- Beds asked for by the households: 900 beds, 310 bunks, 81 cots. Before, 703 / 287 / 52 were placed (the rest were dropped when a wall was full). Now 968 / 308 / 81 (the extra beds are guest beds).
- Hobby and fun things are about as many as before (piano, guitar, easel, chess, aquarium, computer). Fewer bookcases, hifis, dressers, lamps and plants: the rooms are fuller with the things above.

**Limits:**
- Flats are 3 and 4 tiles wide. A 3-wide flat with three sleepers has little room: 2 homes of 647 have no seat.
- 184 homes have no dining table (small one-room homes).
- No bathtubs and almost no counters now: there is rarely a free wall for them.
- Desks are not put under windows. Zones are not split by floor colour.
- An oak can stand inside a mansion's back room (the yard code plants it there; not part of this round).

**Saves:** the save VERSION is unchanged. A home the player owns is stored tile by tile, so it keeps its rooms. Any other home is made fresh when you walk in, so in an old save it is furnished the new way.

## 5. One plan, every view

`bm_lot_plan()` works out a lot's facing, house type, body, wing, door and driveway from the block alone.
The near generator, the far view, the map and the DS port all ask it, so they agree.

## 6. Checks

- `ports/host/lotsim`: the lot audit (overlap, doors, reach) must stay clean, also for north-facing lots.
- `ports/host/townmap`: `make check` (worlds of versions 2 to 6 come out block for block as before), `./townmap stats SEED 7`
  (lots, homes, places against the caps, streets, green, landlocked lots, landmarks, the plan's own rules), `make pics`.
- Road audit (Dreamcast test build): nothing on a street tile.
- Counts per world: homes and places stay under the game's caps.
- PS1: more than 12 KB free past `_end` on 2 MB; old saves load; speed does not drop.

## 7. Stage 1 as built (`src/bigmap.c`, the plan engine; new lives still start as version 6 until stage 2)

Pictures, the same world as version 6 (left) and 7 (right): `docs/plan7_city.png`, `docs/plan7_town.png`, `docs/plan7_village.png`.

**The API** (`src/bigmap.h`): `bm_nb(bx, by)` the neighbourhood kind, `bm_lot_face(bx, by)` the facing, `bm_lot_plan(bx, by, &b, &plan)` the house.
`bm_block()` gives the streets and the lot as before.

**How a town is laid out**
- From its main crossing: the top-left corner of the square's block. The highways come in on those two lines, so they are avenues.
  Avenues are the lines `dx % 4 == 0` and `dy % 4 == 0` from the square (not map columns), so every town has the same heart.
- A neighbourhood's kind comes from its ring and its SECTOR (one of 8 wedges of the town, turned by the town's place), not from dice per
  neighbourhood. A town gets a shopping side, a green wedge, an estate side. Table `g7ring`.
- A village (and the spaceport's town) is all one kind (SUBURB with the village list): cottages inside, farms on its outer blocks
  (beyond 3/4 of its radius), one lot for sale, no corner shops or parks beyond its landmark cross.
- One big place at most per neighbourhood, always its middle four blocks: a plaza or campus (CORE), a mall (MAIN), a park (CIVIC always;
  URBAN, SUBURB and GREEN sometimes). In a suburb the park cuts the inner street into two dead ends.
- Lots are stepped through lists (`g7pool`) by `dx + 7 * dy`, so neighbours along a street and across it differ.
- PACKED towns: terraces where the suburbs were, suburbs where the green was.

**Where it differs from sections 2 to 4, and why**
- MAIN has streets on rows 0 and 2 (all cross streets); CIVIC is laid out as URBAN. Not the full grid: shops then front both sides of an
  avenue, and the middle of the city was still one big grid otherwise. CORE and the landmark blocks keep the full grid.
- CORE is the four neighbourhoods at the crossing plus those with ring under 52 (6 in the default capital).
- ESTATE front yard is 3 to 4, not 5 to 6, and the mansion is 11-12 x 8-9, not 11-13 x 10-12: the lot is 13 deep and its last row stays clear.
- The front yard depth counts from the lot's street edge (its first row is the pavement). Deep houses pull the yard in so the body fits.
- Bodies keep off the block's pavement column when the block has a street on its left; a terrace pair is then 6 + 7 wide.
- Many more blocks are green than the first draft had: the game holds about 1000 to 1234 households, so a world needs about 1250 home
  units, not 2100. The room went to GREEN and ESTATE neighbourhoods and middle parks (houses further apart).
- Road bit 4 (only the block's top-left corner is paved) is now set where the heart's last two streets meet at a corner. Stage 2 must paint it.
- No school in a suburb's middle (the landmarks and CIVIC have the schools); the code had to stay small.
- A farm or park may have no street (LF_NONE). Everything else always has one.

**Stage 2 must honour**
- `bm_lot_face`: 4 lots in 10 face NORTH (door on the north wall, path to the block's own pavement row). Every generator needs a turned form.
- The pavement row and column only where the block has that street; a pavement on the far side of each street too (the pictures show both).
- A river block's bridge only where the block has the street that crosses (`road & 2` for a west-east river, `& 1` for north-south).
- LOT_WILD inside a town is now common (grounds between mansions, woods, the fringe): dress it by `bm_nb` (ESTATE: lawn and trees).
- LOT_EMPTY should be the tidy lot for sale. LOT_FARM on a block with no street is a field.
- RAM (PS1, 2 MB): `_end` 0x801fc900 before, 0x801fcff0 now (the plan inside `bm_block`: 1776 bytes; 12,304 free). The linker drops the three
  API functions until the game calls them: with them `_end` is 0x801fd5e8 (1528 more; 10,776 free), so stage 2 has to move about 2 KB out first.
## 8. Stage 2 as built (`src/lots7.inc`, `world.c gen_block0`, `far.c`; new lives are world version 7, save version 54)
- **Built south, turned north.** `house7` builds every plan-driven home as if it looked south; a lot whose plan faces north is then turned over by `mirror7`
  (rows 5..17 of the lot, the ground, floor, wall and every storey, the north/south bits of turned items, the lot's `Home`s with `Home.fn = 1`; the street row
  keeps its pavement). Every other kind of lot with a north plan (shops, parks, places) goes through the same mirror. `LF_NONE` lots are built south.
- **`Home.fn`, `home_out(hm)`** (+1 south, -1 north): the door's outside. Used by everything that walked "dy + 1" (people leaving, arriving, visiting, sleeping,
  `put_household`, `world_own_door`, the lot audit). People walk out of a north door.
- Houses from `LotPlan`: body, L garage wing (a solid block with its door on the drive), porch, path, two-tile concrete drive (to the garage door in a ranch's
  front wall, to the wing, or back along the house), planters, lamp, mailbox, low hedge across the front with a gap at the walk (suburb, estate, green; open fronts
  in the centre), hedges down the sides, a yard tree on the side away from the drive, a shed in the back corner (back yards 4 rows or more), the old back yard
  (pool, garden, patio, lawn). Terraces are two homes with their own doors. Wall material comes from the neighbourhood palette (two materials a neighbourhood).
  The roof is the engine's, from the walls (the plan's `roof`/`rcol` are not used yet).
- Street trees: one every four tiles along a street's pavement, on open grass. Pavement only where the block has the street; the far side's pavement where the
  next block has its street; the 5x5 corner for road bit 4.
- `far.c`: each far building keeps its footprint (`fb_ft`, 4 bytes) from the plan (a home's body, a terrace or an L with its second half, a place turned over for
  a north face), so near and far agree. 9.6 KB.
- Cottage body is 8x7 to 9x8 now (the room needs it); the estate mansion stays 11-12 x 8-9 (deeper made the back rooms fail the audit).
- Tools: `ports/host/lotsim` `GEN=7 ./lotsim audit|at|area|kinds|sets SEED ...` (`area` draws a stretch of blocks as a picture: `docs/plan7_tiles_town.png`);
  `docs/lots_audit_gen7.txt`; PS1 shots `docs/plan7_ps1_*.png` (an `AUTO_CITY` build with `AUTO_AT_BX/BY/TX/TY/PITCH/DIST` stands at a place).
- **Round 2 (PS1):** a river block has its bridge (asphalt, pavement, lamps, the road rectangle) only where the block has the crossing street (`river_br`: version 7,
  road bit 2 for a west-east river, bit 1 for a north-south one); otherwise the water runs on to the block's edge. LOT_EMPTY is a mown lot with a sign and a tree at
  each back corner (`empty7`); a LOT_FARM with no street (`LF_NONE`) is a field of crop rows (`field7`); LOT_WILD in an ESTATE neighbourhood is grounds (`estate7`: lawn, trees in a
  loose grid, a path across, now and then a pond); other WILD blocks keep the open-country look. The map screens read `bm_block`'s road bits, so they show the new streets as they
  are (the block map honours road bit 4; the diorama's `d_road` draws bits 1 and 2 only). Roofs stay the engine's, from the walls: the plan's `roof` and `rcol` are not used.
- **Not done:** a parked car on the drive (cars are traffic, not map items), roof form and colour from the plan, neighbourhood labels on the map screen, DS and Dreamcast.
- **RAM (PS1):** `_end` 0x801fbe48 (16,824 free; main before: 0x801f71c0).

- **Speed (PS1, frozen city, like for like):** the extra cost at a suburb street is not per building. It follows the walls, upper storeys and roofs in view. Spaced suburbs cost the same
  as gen 6 or less (7-home 3x3: gen 7 460 cpu / 246 world against 739 / 558; spot 640,758 486 vs 513; 630,764 494 vs 617). A view full of two-storey houses and terraces is dearer
  (spot 636,760: 860 against 589; upper storeys 194 hb, roofs 156 hb): content-dependent. Mansions have an upstairs only half the time. Capping storeys in URBAN two-storey clusters is open.
  A moving walk through the capital: gen 7 mean 641 hb, worst frame 1286; gen 6 mean 661, worst 1254. No generation hitch beyond gen 6.
- **Road bit 4 corner:** the 5x5 corner asphalt is now one of `road_rects`' street rectangles, so `world_tile_class`, the purge and the generator agree (road audit 0 off-strip).

## Sources

- Neighbourhood unit: https://en.wikipedia.org/wiki/Neighbourhood_unit
- City block: https://en.wikipedia.org/wiki/City_block
- Coving: https://en.wikipedia.org/wiki/Coving_(urban_planning)
- Setbacks: https://buildability.us/blog/setbacks-explained-front-side-rear
- Prairie Trail pattern book: https://www.prairietrailankeny.com/wp-content/uploads/2013/03/B_Community_Patterns.pdf
- Springfield residential design guidelines: https://www.springfield-ma.gov/planning/fileadmin/Planning_files/forms/Downloads/Design_Guidelines_Residential_Architecture.pdf
- Ranch houses in Georgia: https://gadnr.org/sites/default/files/hpd/pdf/RanchHousesinGeorgiaTwo.pdf
- Floor planning principles: https://build-construct.com/architecture/floor-planning-principles-of-functional-space-layout-and-room-arrangement-in-residential-design/
- Anti-monotony code (Carol Stream): https://codelibrary.amlegal.com/codes/carolstream/latest/carolstream_il/0-0-0-1774
- Functional landscape (WSU): https://wpcdn.web.wsu.edu/extension/uploads/sites/57/2014/03/Functional-Landscape.pdf
- Room layouts: https://build-construct.com/architecture/room-layouts-furniture-arrangement-traffic-patterns-and-space-optimization-strategies/

## 9. Round 2 (roofs from the plan, lawns)
- A home's roof comes from its plan (bm_lot_plan: roof, pal, rcol): hip or gable by the plan, colour from the neighbourhood palette, steeper pitch on two-storey, L, box and mansion. In roof_region (world.c), no new RAM. Version 7 only.
- A home lot (and a park) in a sandy town gets a lawn instead of bare sand, so a street is not green on one lot and yellow on the next.
- Picture: docs/gen7_roofs_lawns_before_after.png (left before, right after; blocks 410,464 411,464 413,464 of world 0x5EED1234).
- Tooling: townmap list SEED GEN prints every block to choose shots from; AUTO_AT_TAB places count from the first life frame.
- Known: the PS1 AUTO_CITY test build at the Object scale commit already overflows 2 MB RAM and crashes after about 100 frames; these shots use a copy with ARENA_BYTES cut (test only).
- Not done: gables turned to the street (RF_FRONT), parked cars, neighbourhood names on the map screen (main.c), a storey cap in busy streets.
