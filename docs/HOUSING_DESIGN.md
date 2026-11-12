# Housing and town design (world version 7)

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
