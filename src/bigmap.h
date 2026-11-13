// bigmap: SAN BOREAS, the open world. 1024 x 1024 blocks of 16 tiles: 16384 tiles a side (a tile is about 1.2 m, so
// about 19.7 km a side, near 390 square km: more than GTA III, Vice City, San Andreas, IV and V put together).
// Nothing of it is stored. Every block is worked out from its coordinates and the save's seed when it is needed: the
// land and the sea, the towns and cities and their names, the highways between them, and what stands on every lot.
// A new life rolls a new seed, so every save is a new world.
#pragma once
#include "common.h"

#define BM_N    1024                  // blocks a side
#define BM_BLK  18                    // tiles a block
#define BM_LOT0 5                     // a block's lot starts 3 tiles in (the street runs along its top and left)

enum { BT_SEA, BT_BEACH, BT_GRASS, BT_FOREST, BT_DESERT, BT_ROCKY };
enum { LOT_WILD, LOT_HOUSE, LOT_COTTAGE, LOT_TOWN, LOT_TWO, LOT_APART,            // nothing; the homes (APART: a block of flats, 2 a storey)
       LOT_SHOP, LOT_DINER, LOT_GYM, LOT_OFFICE, LOT_INN, LOT_POOL, LOT_SCHOOL, LOT_ARCADE, LOT_TOWER, LOT_HOTEL,   // the places people go (TOWER: offices, storeys of them)
       LOT_PARK, LOT_SQUARE, LOT_SKATE, LOT_EMPTY, LOT_FARM,
       LOT_CAFE, LOT_BAR, LOT_CINEMA, LOT_SALON, LOT_RESTAURANT, LOT_CLINIC, LOT_LIBRARY, LOT_SPA, LOT_GALLERY,   // more places to go
       LOT_GAS, LOT_CONV,                                                          // a gas station (pumps, a shop, vehicles for sale) and a corner convenience store
       LOT_RIVER,                                                                 // a river across a town: water, walks and bridges (not a place)
       LOT_SPACE, LOT_OBS,                                                        // the space centre (mission control and a launch pad) and an observatory: only at the far spaceport
       LOT_N };
// a block's road bits: 1 street along the top, 2 along the left, 4 the corner. A MEGA block (a 2x2 group made one place: a big park, a mall, a downtown plaza)
// has BR_MEGA, its place in the group (BR_PART: 0 top-left, 1 top-right, 2 bottom-left, 3 bottom-right) and what it is (BR_KIND: MK_). On a RIVER block BR_MEGA means it runs north-south.
#define BR_MEGA 8
#define BR_PART(r) (((r) >> 4) & 3)
#define BR_KIND(r) (((r) >> 6) & 3)
enum { MK_PARK, MK_MALL, MK_PLAZA, MK_CAMPUS };   // (CAMPUS: a business park of offices, hotels and shops round a paved court)
enum { ST_CITY, ST_TOWN, ST_VILLAGE, ST_SPACEPORT };

typedef struct {
    u8  terrain;                      // BT_
    u8  lot;                          // LOT_
    u8  set;                          // the settlement it belongs to (255: open country)
    u8  road;                         // 1: a street along the top edge, 2: along the left edge, 4: the top-left corner is paved
    u32 h;                            // the block's own dice
} BInfo;

typedef struct { u16 x, y; u8 r, kind; char name[18]; } Settle;

extern int bm_packed;                 // the TOWNS option: 0 ROOMY (parks and green gaps, towns far apart), 1 PACKED (built up, towns close)
extern char bm_cap_name[18];          // the capital's name (empty: BORE CITY): a set world names its own
// CREATE A WORLD's dials (MAINLAND and 2 for the rest: SAN BOREAS as it always was)
enum { BL_MAINLAND, BL_ISLANDS, BL_BIGISLE, BL_CONTINENT, BL_LAKES, BL_N };
extern int bm_land, bm_sea, bm_clim, bm_hills;   // LAND (BL_), SEA 0..4 (low..high), CLIMATE 0..4 (desert..jungle), HILLS 0..4 (flat..mountains)
extern int bm_ncity, bm_ntown, bm_nvil, bm_capsz;  // how many cities (besides the capital), towns, villages (2 6 12); the capital's size 0..5 (2 as always)
#define BM_MAXCITY 8
#define BM_MAXTOWN 20
#define BM_MAXVIL 32
void bm_dials_default(void);
extern int bm_gen;                    // how worlds are made: 3 now (2: as before the roomy towns, for older saves)
void bm_rename_clear(void);           // towns renamed by the player: none (a new life)
void bm_rename(int i, const char *nm); // town i (0 the capital) is called that from now on (set before bm_new: it takes then)
const char *bm_renamed(int i);        // its new name ("": not renamed)
#define BM_NSET_MAX 62
// the neighbourhood editor's changes (kept across bm_new; bm_ovr_clear for a new life)
enum { BO_NONE = 0, BO_GROUND = 1, BO_LOT = 2, BO_ROAD = 4, BO_TERR = 8, BO_ELEV = 16 };
void bm_ovr_clear(void);
int  bm_ovr_set(int bx, int by, int what, int val);   // 0: the table is full
void bm_ovr_unset(int bx, int by, int what);
int  bm_ovr_delta(int bx, int by);   // the land raised (+) or lowered (-) there, in steps
int  bm_hill(int bx, int by);
int  bm_hill_t(int bx, int by, int *town);   // ... and whether the block is built on (its lot is a level pad)        // the ground a block's middle stands at, in world units (the hills you walk on)
int  bm_height(int bx, int by);      // the ground there for the 3D model: 0 sea level, up in elevation points (hills)
int  bm_ovr_n(void);
int  bm_ovr_next(int *it, int *bx, int *by, int *f, u8 *v);   // *it starts at 0; v: ground, building, road, territory, height change
void bm_new(u32 seed);                // a new world
u32  bm_seed(void);
void bm_block(int bx, int by, BInfo *b);
int  bm_terrain(int bx, int by);
int  bm_units(const BInfo *b);        // homes on that block
int  bm_storeys(const BInfo *b);      // how tall what stands there is (1 .. 5)
int  bm_is_place(int lot);            // somewhere people work or go out to
const char *bm_lot_name(int lot);
int  bm_nset(void);
int  bm_nseg(void);                   // the highways (for the map)
void bm_seg(int i, int *c, int *a, int *b, int *vert);
const Settle *bm_set(int i);
int  bm_capital(void);
int  bm_set_at(int bx, int by);       // the town that block is in (255: open country)                // BORE CITY's index
const char *bm_street(int set, int bx, int by, int unit, char *out);   // "214 MAPLE ST"
const char *bm_place_name(int lot, u32 h, char *out);   // "BOREMART", "THE SLEEPY INN"
const char *bm_state_name(void);      // "SAN BOREAS"
u32 bm_hash(int a, int b, u32 s);

// ---- THE TOWN PLAN (worlds made with bm_gen 7 on; docs/HOUSING_DESIGN.md). Older worlds keep their towns: every block of theirs
// faces south and has no neighbourhood. A town is laid out from its main crossing (the top-left corner of its square's block: the
// highways come in there). Avenues run on every 4th line both ways from it; the 4 x 4 blocks between them are one NEIGHBOURHOOD
// of one kind, which sets its streets and what stands on its lots. bm_block() gives the streets and the lots; these give the rest. ----
enum { NK_NONE,                       // not a block of a planned town (open country, the sea, a world from before the plan)
       NK_CORE,                       // the city centre: towers, offices, hotels, flats, shops; the full grid of streets
       NK_MAIN,                       // shopping streets, with flats and terraces behind; the full grid
       NK_CIVIC,                      // a big park in the middle, the school, library, clinic and pool beside it; the full grid
       NK_URBAN,                      // terraces and flats: a street on every 2nd row, one cross street
       NK_SUBURB,                     // detached houses: a street on every 2nd row only, the lots back to back
       NK_ESTATE,                     // mansions on every 2nd lot, garden lots between; streets as a suburb's
       NK_GREEN };                    // farms, woods or a common, a few cottages on the avenues; no streets inside
enum { LF_NONE, LF_SOUTH, LF_NORTH }; // which way a lot fronts: onto the street along the next block down, or onto its own block's street
enum { HT_NONE, HT_COMPACT, HT_RANCH, HT_L, HT_BOX, HT_TWO, HT_TERRACE, HT_MANSION };   // the house on a home lot
enum { RF_SIDE, RF_FRONT, RF_HIP };   // its roof: a gable with the ridge along the street, a gable turned to the street, hipped
// A lot's plan. Every tile is a tile of the block (0 .. BM_BLK-1; the lot is BM_LOT0 .. BM_BLK-1 both ways), already turned for
// the facing, walls included. For a lot that is not a home (type HT_NONE) only `face` is set, the rest is 0.
typedef struct {
    u8 face;                          // LF_: the door's side. LF_NONE: no street to front on (only where the player took one away: build it as LF_SOUTH)
    u8 type;                          // HT_
    u8 x0, y0, x1, y1;                // the body. HT_TERRACE: the first of the two homes
    u8 wx0, wy0, wx1, wy1;            // the wing, x1 0 if none. HT_L: the garage wing, 2 tiles back from the front wall, sharing the body's
                                      // side wall (the two overlap by that one column). HT_TERRACE: the second home (it shares the first's side wall)
    u8 dx, dy;                        // the front door, in the body's front wall (the path runs from it straight to the street)
    u8 d2x;                           // HT_TERRACE: the second home's door (same row), else 0
    u8 drv;                           // the driveway's west column (it is 2 wide), 0 if none. It runs from the lot's street edge to the first
                                      // wall in its way (the garage end of a ranch, the wing of an L), or beside the house to the body's back line
    u8 yard;                          // the front yard's depth: rows between the lot's street edge and the front wall (the slow wave along the street)
    u8 mir;                           // 1: mirrored (the garage, driveway and door on the west side instead of the east)
    u8 roof;                          // RF_
    u8 pal;                           // the neighbourhood's palette 0..7 (two wall materials, two roof colours)...
    u8 wall, rcol;                    // ... and which of its two walls and two roof colours this house has (0 or 1 each)
} LotPlan;
const char *bm_nb_name(int bx, int by, char *out);   // the neighbourhood's name ("MAPLE HEIGHTS") into out (24 bytes); 0 outside a planned town
int  bm_nb(int bx, int by);           // NK_: the kind of the neighbourhood that block lies in
int  bm_lot_face(int bx, int by);     // LF_: LF_SOUTH if the block below has a street along its top, else LF_NORTH if this block has one, else LF_NONE
void bm_lot_plan(int bx, int by, const BInfo *b, LotPlan *p);   // b: that block, from bm_block()
// What every map, model and far view draws of a lot's building comes from these two (the world's own generators are checked against them: ports/host/lotsim `plan` and `foot`).
int  bm_lot_home(int bx, int by, const BInfo *b, LotPlan *p);   // a home lot's plan: the town plan in a version 7 town, else the body its old generator (city_house ...) builds. 0 if the lot is not a home
int  bm_lot_foot(int bx, int by, const BInfo *b, u8 *f, LotPlan *pp);        // the building on the lot, as block tiles f[0..3] = x0 y0 x1 y1 (walls included, turned for the lot's facing; a home with a wing or a second half: the lot of both); f[4] 1 if it is a home. 0: nothing built there (a park, a field, a pool, an empty lot). pp (or 0): the lot's plan as far as it goes (its face for every lot in a planned town, the whole plan for a home)
