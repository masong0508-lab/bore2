# Object scale

How big a thing is in the game, against a person. Made with `tools/object_scale.py` (it reads `src/items.c` and the `items_*.inc` models, so this page can be made again).

## The units

- One tile is 256 world units = **1.13 m**. A person is 384 units = 1.5 tiles = **1.7 m** (16 pixels of height).
- A model is drawn on an 8 x 8 grid per tile: one model unit = 1/8 tile = **14 cm** across. One pixel of height = **10.6 cm**. The wall is 18 px = 1.9 m, the door is about 17 px.
- `foot` is how many tiles the thing takes (`F_L2` two along, `F_W2` two across). The model must fit its footprint: a model may not be longer than its tiles.

## The things a home has

| thing | real size (across x along x high) | tiles | model now | height now |
|---|---|---|---|---|
| single bed (SINGLE BED 0xF6, new worlds) | 0.9 x 1.9 x 0.55 m | 1 x 2 | 0.99 x 2.12 m | 1.27 m with the head board |
| old one-tile bed (BED, `S`, old worlds only) | (a cot) | 1 x 1 | 1.13 x 1.13 m | 1.27 m |
| family bed (0xEC) | 1.6 x 2.0 | 2 x 2 | 1.69 x 2.12 m, centred | 1.27 m |
| four poster bed (0x93, 0xEB) | 1.7 x 2.1 x 2.0 m | 2 x 2 | 1.69 x 2.12 m, centred | 1.7 m (it was one tile) |
| bunk bed | 0.9 x 1.9 x 1.6 m | 1 x 1 | 1.13 x 1.13 m | 1.59 m (it was 1.4 m) |
| toddler bed | 0.7 x 1.4 | 1 x 2 | 1.13 x 1.69 m | 0.85 m |
| nightstand (0x96, 0xF0) | 0.5 x 0.4 x 0.55 m | 1 x 1 | 0.85 x 0.71 m | 0.85 m (it was 1.17 m) |
| wardrobe (0x97, 0xF3) | 1.0 x 0.6 x 1.9 m | 1 x 1 | 1.13 x 0.71 m | 1.91 m (it was 2.1 and 2.5 m) |
| grandfather clock | 0.5 x 0.4 x 1.9 m | 1 x 1 | 0.56 x 0.56 m | 1.91 m (it was 2.3 m: through the ceiling) |
| sofa | 2.0 x 0.9 x 0.85 m | 1 x 1 (one module, rows of them) | 1.13 x 1.13 m | 1.06 m |
| loveseat, armchair | 1.4 x 0.9 / 0.9 x 0.9 | 1 x 1 | 1.13 x 0.85 m | 0.95 m |
| dining table | 1.2 x 1.2 x 0.75 m | 1 x 1 | 1.13 x 1.13 m (was 0.85 deep) | 0.85 m |
| counter | 1.0 x 0.6 x 0.9 m | 1 x 1 | 1.13 x 1.13 m (was 0.85 deep) | 0.85 m |
| fridge | 0.7 x 0.7 x 1.8 m | 1 x 1 | 0.85 x 0.85 m | 1.70 m |
| toilet | 0.4 x 0.7 x 0.8 m | 1 x 1 | 0.56 x 0.85 m | 0.85 m |
| bath | 1.7 x 0.7 x 0.55 m | 1 x 1 | 1.13 x 0.85 m | 0.85 m |
| shower | 0.9 x 0.9 x 2.0 m | 1 x 1 | 1.13 x 1.13 m | 1.9 m |
| TV | 1.0 x 0.5 x 1.2 m | 1 x 1 | 0.85 x 0.85 m | 1.27 m |
| bookshelf | 0.9 x 0.3 x 1.8 m | 1 x 1 | 0.85 x 0.56 m | 1.91 m |
| desk | 1.2 x 0.6 x 0.75 m | 1 x 1 | 1.13 x 0.85 m | 1.27 m with the computer |
| chair | 0.5 x 0.5 x 0.9 m | 1 x 1 | 0.85 x 0.71 m | 0.95 m |

Notes.

- A tile is 1.13 m, so the one-tile things (sofa, counter, table) are *modules*: a row of two or three makes the real piece. The model is a little bigger than life across so it reads, and never taller than the real thing.
- Beds are the one thing a tile cannot hold: a 1.9 m bed is two tiles. New worlds (generation 7) put the SINGLE BED (0xF6, `F_L2`) in a bedroom whose foot is clear (`fz_fixbeds` falls back to the old one-tile bed when it is not, so no room ever fails). Old worlds (generation 6 and before) are byte for byte what they were, so a saved house does not change.
- Sleeping on a two-tile bed lies you in the middle of it (half a tile along and across).
- Four poster and family bed are drawn 1.5 tiles wide and centred on their 2 x 2 footprint (they were 2 tiles wide and one tile long on a 1-tile footprint, which poked through the wall).
- `lifeSize[]` in `src/items.c` still squeezes some old things in height at run time (TV, shower, chair, tree...). Not touched: those squeezes are part of the saved look.

## Every item

`foot` is the tiles, `model` the drawn size, `h` the height (the run-time squeeze `(life)` when there is one).

| char | name | foot | model (tiles) | model (m) | h (px) | h (m) |
|---|---|---|---|---|---|---|
| 21 | ARCADE CABINET | 1x1 | 0.50 x 0.75 | 0.56 x 0.85 | 16 | 1.70 |
| 22 | STREET LAMP | 1x1 | 0.50 x 0.38 | 0.56 x 0.42 | 40 (life) | 4.24 |
| 23 | CRATE | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 16 | 1.70 |
| 24 | PIANO | 1x1 | 1.00 x 0.62 | 1.13 x 0.71 | 12 | 1.27 |
| 25 | LOG BENCH | 1x1 | 1.00 x 0.38 | 1.13 x 0.42 | 4 (life) | 0.42 |
| 26 | OUTHOUSE | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 14 | 1.48 |
| 28 | GUITAR | 1x1 | 0.50 x 0.25 | 0.56 x 0.28 | 15 | 1.59 |
| 29 | EASEL | 1x1 | 0.75 x 0.62 | 0.85 x 0.71 | 13 | 1.38 |
| 2A | ROCK | 1x1 | 0.62 x 0.50 | 0.71 x 0.56 | 6 | 0.64 |
| 2B | BUS STOP | 1x1 | 0.75 x 0.12 | 0.85 x 0.14 | 22 (life) | 2.33 |
| 2C | BATHTUB | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 8 | 0.85 |
| 2D | SINK | 1x1 | 0.75 x 0.62 | 0.85 x 0.71 | 10 | 1.06 |
| 2F | STOVE | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 11 | 1.17 |
| 30 | SPRING | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 3 | 0.32 |
| 31 | KICKER | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 12 | 1.27 |
| 33 | OAK TREE | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 36 (life) | 3.82 |
| 35 | Q PIPE | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 24 | 2.54 |
| 36 | PINE TREE | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 40 (life) | 4.24 |
| 37 | SHOP SHELF | 1x1 | 1.00 x 0.50 | 1.13 x 0.56 | 10 | 1.06 |
| 38 | FOR SALE SIGN | 1x1 | 0.75 x 0.12 | 0.85 x 0.14 | 11 | 1.17 |
| 39 | LAUNCH | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 18 | 1.91 |
| 3A | MICROWAVE | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 10 | 1.06 |
| 3B | ARMCHAIR | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 9 | 0.95 |
| 3C | BUNK BED | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 15 | 1.59 |
| 3D | RAIL | 1x1 | 1.00 x 0.50 | 1.13 x 0.56 | 4 (life) | 0.42 |
| 3E | HOT TUB | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 8 (life) | 0.85 |
| 3F | CHESS TABLE | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 5 | 0.53 |
| 40 | BOAT | 1x1 | 1.00 x 0.50 | 1.13 x 0.56 | 13 | 1.38 |
| 41 | SOUND SYSTEM | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 14 | 1.48 |
| 42 | SKATEBOARD | 1x1 | 0.50 x 0.75 | 0.56 x 0.85 | 4 | 0.42 |
| 43 | SOFA | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 10 | 1.06 |
| 44 | DOOR | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 1 | 0.11 |
| 46 | FRIDGE | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 16 | 1.70 |
| 47 | WATER PIPE | 1x1 | 0.62 x 0.50 | 0.71 x 0.56 | 20 | 2.12 |
| 48 | SHOWER | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 18 (life) | 1.91 |
| 49 | PHONE | 1x1 | 0.50 x 0.25 | 0.56 x 0.28 | 8 | 0.85 |
| 4A | JERSEY | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 7 (life) | 0.74 |
| 4B | PICNIC | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 7 (life) | 0.74 |
| 4C | LEDGE | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 4 (life) | 0.42 |
| 4D | MANUAL PAD | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 2 (life) | 0.21 |
| 4E | BENCH | 1x1 | 1.00 x 0.50 | 1.13 x 0.56 | 8 (life) | 0.85 |
| 4F | BARREL | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 8 (life) | 0.85 |
| 50 | SPAWN | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 1 | 0.11 |
| 51 | DEADSET VYBE | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 14 | 1.48 |
| 52 | RADIO | 1x1 | 0.75 x 0.38 | 0.85 x 0.42 | 8 | 0.85 |
| 53 | BED | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 12 | 1.27 |
| 54 | TOILET | 1x1 | 0.50 x 0.75 | 0.56 x 0.85 | 8 | 0.85 |
| 55 | BEANBAG | 1x1 | 0.75 x 1.00 | 0.85 x 1.13 | 9 | 0.95 |
| 56 | LAVA LAMP | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 6 (life) | 0.64 |
| 58 | FUNBOX | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 4 (life) | 0.42 |
| 59 | TRASH CAN | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 8 (life) | 0.85 |
| 5A | PLANTER | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 5 (life) | 0.53 |
| 5B | COMPUTER | 1x1 | 1.00 x 0.62 | 1.13 x 0.71 | 10 | 1.06 |
| 5D | WEIGHT BENCH | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 10 | 1.06 |
| 5E | STAIRS UP | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 16 | 1.70 |
| 5F | BASKETBALL HOOP | 1x1 | 0.75 x 0.62 | 0.85 x 0.71 | 28 (life) | 2.97 |
| 60 | MIRROR | 1x1 | 0.75 x 0.12 | 0.85 x 0.14 | 17 | 1.80 |
| 61 | COMMUNITY FLAG | 1x1 | 0.62 x 0.12 | 0.71 x 0.14 | 28 (life) | 2.97 |
| 62 | BOOKSHELF | 1x1 | 0.75 x 0.50 | 0.85 x 0.56 | 18 | 1.91 |
| 63 | COFFEE MAKER | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 12 | 1.27 |
| 64 | DESK | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 12 | 1.27 |
| 67 | PALM TREE | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 40 (life) | 4.24 |
| 68 | CHAIR | 1x1 | 0.75 x 0.62 | 0.85 x 0.71 | 9 (life) | 0.95 |
| 69 | DRESSER | 1x1 | 0.75 x 0.62 | 0.85 x 0.71 | 10 | 1.06 |
| 6A | JUKEBOX | 1x1 | 0.50 x 0.62 | 0.56 x 0.71 | 13 | 1.38 |
| 6B | SKATE FLAG | 1x1 | 0.62 x 0.12 | 0.71 x 0.14 | 28 (life) | 2.97 |
| 6C | FLOOR LAMP | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 17 | 1.80 |
| 6D | TREADMILL | 1x1 | 0.75 x 1.00 | 0.85 x 1.13 | 12 | 1.27 |
| 6E | TOY BOX | 1x1 | 0.75 x 0.50 | 0.85 x 0.56 | 7 | 0.74 |
| 6F | FIREPLACE | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 18 | 1.91 |
| 70 | HOUSEPLANT | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 14 | 1.48 |
| 71 | AQUARIUM | 1x1 | 0.75 x 0.62 | 0.85 x 0.71 | 15 | 1.59 |
| 72 | RUG | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 1 | 0.11 |
| 73 | CAMPFIRE | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 8 | 0.85 |
| 74 | DINING TABLE | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 8 | 0.85 |
| 75 | CACTUS | 1x1 | 0.50 x 0.25 | 0.56 x 0.28 | 14 (life) | 1.48 |
| 76 | TV | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 12 (life) | 1.27 |
| 78 | WORK MARKER | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 1 | 0.11 |
| 79 | COUNTER | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 8 | 0.85 |
| 7A | TENT | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 11 (life) | 1.17 |
| 7B | DOLLHOUSE | 1x1 | 0.75 x 0.62 | 0.85 x 0.71 | 10 | 1.06 |
| 7C | STAIRS | 1x1 | 0.50 x 0.75 | 0.56 x 0.85 | 4 | 0.42 |
| 7D | FOUNTAIN | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 20 (life) | 2.12 |
| 7E | POOL LOUNGER | 1x1 | 1.00 x 0.50 | 1.13 x 0.56 | 5 | 0.53 |
| 81 | CRIB | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 12 | 1.27 |
| 82 | FUEL PUMP | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 20 | 2.12 |
| 83 | GRAVESTONE | 1x1 | 0.75 x 0.38 | 0.85 x 0.42 | 12 | 1.27 |
| 84 | MESSAGE IN A BOTTLE | 1x1 | 0.62 x 0.25 | 0.71 x 0.28 | 2 | 0.21 |
| 85 | SANDY MOUND | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 2 | 0.21 |
| 86 | TIKI IDOL | 1x1 | 0.75 x 0.50 | 0.85 x 0.56 | 14 | 1.48 |
| 87 | SHIPWRECK | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 12 | 1.27 |
| 88 | SEASHELL | 1x1 | 0.25 x 0.25 | 0.28 x 0.28 | 1 | 0.11 |
| 89 | BERRY BUSH | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 5 | 0.53 |
| 8A | FISHING SPOT | 1x1 | 0.50 x 0.12 | 0.56 x 0.14 | 10 | 1.06 |
| 8B | RAFT | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 11 | 1.17 |
| 8C | GOLDEN IDOL | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 11 | 1.17 |
| 8D | OLD JOURNAL | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 4 | 0.42 |
| 90 | LOVESEAT | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 9 | 0.95 |
| 91 | RECLINER | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 10 | 1.06 |
| 92 | BAR STOOL | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 8 | 0.85 |
| 93 | FOUR POSTER BED | 2x2 | 1.50 x 1.88 | 1.69 x 2.12 | 16 | 1.70 |
| 94 | HAMMOCK | 1x1 | 1.00 x 0.50 | 1.13 x 0.56 | 12 | 1.27 |
| 95 | COFFEE TABLE | 1x1 | 0.75 x 0.50 | 0.85 x 0.56 | 4 | 0.42 |
| 96 | NIGHTSTAND | 1x1 | 0.75 x 0.62 | 0.85 x 0.71 | 8 | 0.85 |
| 97 | WARDROBE | 1x1 | 1.00 x 0.62 | 1.13 x 0.71 | 18 | 1.91 |
| 98 | FILING CABINET | 1x1 | 0.50 x 0.62 | 0.56 x 0.71 | 13 | 1.38 |
| 99 | STORAGE CHEST | 1x1 | 0.75 x 0.62 | 0.85 x 0.71 | 6 | 0.64 |
| 9A | KITCHEN ISLAND | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 8 | 0.85 |
| 9B | WORKBENCH | 1x1 | 1.00 x 0.50 | 1.13 x 0.56 | 9 | 0.95 |
| 9C | DISHWASHER | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 7 | 0.74 |
| 9D | WASHING MACHINE | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 13 | 1.38 |
| 9E | BBQ GRILL | 1x1 | 0.75 x 0.50 | 0.85 x 0.56 | 10 | 1.06 |
| 9F | WATER COOLER | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 15 | 1.59 |
| A0 | DRUM KIT | 1x1 | 1.00 x 0.62 | 1.13 x 0.71 | 8 | 0.85 |
| A1 | POOL TABLE | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 7 | 0.74 |
| A2 | TELESCOPE | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 12 | 1.27 |
| A3 | SYNTH | 1x1 | 1.00 x 0.50 | 1.13 x 0.56 | 6 | 0.64 |
| A4 | FOOSBALL | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 8 | 0.85 |
| A5 | DARTBOARD | 1x1 | 0.75 x 0.12 | 0.85 x 0.14 | 14 | 1.48 |
| A6 | SLOT MACHINE | 1x1 | 0.50 x 0.62 | 0.56 x 0.71 | 16 | 1.70 |
| A7 | RECORD PLAYER | 1x1 | 1.00 x 0.50 | 1.13 x 0.56 | 8 | 0.85 |
| A8 | MASSAGE CHAIR | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 12 | 1.27 |
| A9 | PUNCHING BAG | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 14 | 1.48 |
| AA | GRANDFATHER CLOCK | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 18 | 1.91 |
| AB | STATUE | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 18 | 1.91 |
| AC | VASE | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 8 | 0.85 |
| AD | CHRISTMAS TREE | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 17 | 1.80 |
| AE | FRAMED ART | 1x1 | 0.75 x 0.12 | 0.85 x 0.14 | 14 | 1.48 |
| AF | GLOBE | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 8 | 0.85 |
| B0 | CANDELABRA | 1x1 | 0.75 x 0.25 | 0.85 x 0.28 | 12 | 1.27 |
| B1 | SUIT OF ARMOR | 1x1 | 0.50 x 0.38 | 0.56 x 0.42 | 17 | 1.80 |
| B2 | FLOWER BED | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 4 | 0.42 |
| B3 | HEDGE | 1x1 | 1.00 x 0.50 | 1.13 x 0.56 | 10 | 1.06 |
| B4 | PICKET FENCE | 1x1 | 1.00 x 0.12 | 1.13 x 0.14 | 11 | 1.17 |
| B5 | MAILBOX | 1x1 | 0.62 x 0.38 | 0.71 x 0.42 | 11 (life) | 1.17 |
| B6 | BIRDBATH | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 11 | 1.17 |
| B7 | GARDEN GNOME | 1x1 | 0.25 x 0.25 | 0.28 x 0.28 | 11 | 1.17 |
| B8 | TRAMPOLINE | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 8 (life) | 0.85 |
| B9 | SWING SET | 1x1 | 1.00 x 0.25 | 1.13 x 0.28 | 24 (life) | 2.54 |
| BA | SANDBOX | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 3 | 0.32 |
| BB | DOG HOUSE | 1x1 | 1.00 x 0.88 | 1.13 x 0.99 | 9 | 0.95 |
| BC | FIRE PIT | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 5 | 0.53 |
| BD | BEACH UMBRELLA | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 19 | 2.01 |
| BE | GOTCHI DOCK | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 11 | 1.17 |
| BF | EGG SHELF | 1x1 | 1.00 x 0.62 | 1.13 x 0.71 | 14 | 1.48 |
| C0 | BLOB CABINET | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 17 | 1.80 |
| C1 | BLOB PLUSH | 1x1 | 0.50 x 0.62 | 0.56 x 0.71 | 11 | 1.17 |
| C2 | BLOB BALL | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 6 | 0.64 |
| C3 | BLOB BED | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 6 | 0.64 |
| C4 | FOOD BOWL | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 3 | 0.32 |
| C5 | PIXEL LAMP | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 12 | 1.27 |
| C6 | TROPHY | 1x1 | 0.75 x 0.50 | 0.85 x 0.56 | 12 | 1.27 |
| C7 | DISCO BALL | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 15 | 1.59 |
| C8 | BUFFET TABLE | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 10 | 1.06 |
| C9 | PUNCH BOWL | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 9 | 0.95 |
| CA | SPEAKER STACK | 1x1 | 0.75 x 0.62 | 0.85 x 0.71 | 10 | 1.06 |
| CB | BALLOONS | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 13 | 1.38 |
| CC | DJ BOOTH | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 8 | 0.85 |
| CD | KARAOKE STAGE | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 11 | 1.17 |
| CE | PHOTO BOOTH | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 13 | 1.38 |
| CF | PRACTICE DRUMS | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 7 | 0.74 |
| D0 | PRACTICE GUITAR | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 11 | 1.17 |
| D1 | PRACTICE KEYS | 1x1 | 1.00 x 0.50 | 1.13 x 0.56 | 8 | 0.85 |
| D2 | PRACTICE MIC | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 11 | 1.17 |
| D3 | SMALL TELESCOPE | 1x1 | 0.88 x 0.75 | 0.99 x 0.85 | 10 | 1.06 |
| D4 | HOBBY TELESCOPE | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 15 | 1.59 |
| D5 | RESEARCH TELESCOPE | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 15 | 1.59 |
| D6 | OBSERVATORY DOME | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 13 | 1.38 |
| D7 | RADIO DISH ARRAY | 1x1 | 1.00 x 0.62 | 1.13 x 0.71 | 15 | 1.59 |
| D8 | LAUNCH ROCKET | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 26 | 2.76 |
| D9 | MISSION CONSOLE | 1x1 | 1.00 x 0.62 | 1.13 x 0.71 | 10 | 1.06 |
| DA | EASEL | 1x1 | 0.75 x 0.50 | 0.85 x 0.56 | 13 | 1.38 |
| DB | CROP BED | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 7 | 0.74 |
| DC | HEN COOP | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 10 | 1.06 |
| DD | DAIRY COW | 1x1 | 0.50 x 1.00 | 0.56 x 1.13 | 9 | 0.95 |
| DE | FARM STAND | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 10 | 1.06 |
| DF | PICKET FENCE | 1x1 | 1.00 x 0.38 | 1.13 x 0.42 | 9 | 0.95 |
| E0 | MAILBOX | 1x1 | 0.88 x 0.50 | 0.99 x 0.56 | 11 (life) | 1.17 |
| E1 | HEDGE | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 9 | 0.95 |
| E2 | FLOWER BED | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 9 | 0.95 |
| E3 | GARDEN GNOME | 1x1 | 0.25 x 0.38 | 0.28 x 0.42 | 11 | 1.17 |
| E4 | BIRD BATH | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 10 | 1.06 |
| E5 | DIVING BOARD | 1x1 | 1.00 x 0.50 | 1.13 x 0.56 | 6 | 0.64 |
| E6 | POOL LADDER | 1x1 | 0.25 x 0.50 | 0.28 x 0.56 | 9 | 0.95 |
| E7 | LIFEGUARD CHAIR | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 20 | 2.12 |
| E8 | BEACH UMBRELLA | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 19 | 2.01 |
| E9 | BEACH TOWEL | 1x1 | 0.75 x 1.00 | 0.85 x 1.13 | 3 | 0.32 |
| EA | BBQ GRILL | 1x1 | 0.88 x 0.62 | 0.99 x 0.71 | 13 | 1.38 |
| EB | FOUR POSTER BED | 2x2 | 1.50 x 1.88 | 1.69 x 2.12 | 20 | 2.12 |
| EC | FAMILY BED | 2x2 | 1.50 x 1.88 | 1.69 x 2.12 | 12 | 1.27 |
| ED | TODDLER BED | 1x2 | 1.00 x 1.50 | 1.13 x 1.69 | 8 | 0.85 |
| EE | BASSINET | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 14 | 1.48 |
| EF | CRADLE | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 8 | 0.85 |
| F0 | NIGHTSTAND | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 9 | 0.95 |
| F1 | TOY CHEST | 1x1 | 1.00 x 0.62 | 1.13 x 0.71 | 11 | 1.17 |
| F2 | ROCKING HORSE | 1x1 | 0.75 x 1.00 | 0.85 x 1.13 | 13 | 1.38 |
| F3 | WARDROBE | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 18 | 1.91 |
| F4 | PLAYPEN | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 8 | 0.85 |
| F5 | CHANGING TABLE | 1x1 | 1.00 x 0.75 | 1.13 x 0.85 | 11 | 1.17 |
| F6 | SINGLE BED | 1x2 | 0.88 x 1.88 | 0.99 x 2.12 | 12 | 1.27 |
| F7 | DREAMCAST | 1x1 | 0.75 x 0.88 | 0.85 x 0.99 | 3 | 0.32 |
| F8 | VMU ON A STAND | 1x1 | 0.50 x 0.50 | 0.56 x 0.56 | 8 | 0.85 |
| F9 | SWIRL RUG | 1x1 | 1.00 x 1.00 | 1.13 x 1.13 | 1 | 0.11 |
| FA | DC ARCADE CABINET | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 13 | 1.38 |
| FB | ORANGE SWIRL LAMP | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 16 | 1.70 |
| FC | BLUE SWIRL LAMP | 1x1 | 0.75 x 0.75 | 0.85 x 0.85 | 16 | 1.70 |
