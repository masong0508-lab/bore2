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
       LOT_N };
// a block's road bits: 1 street along the top, 2 along the left, 4 the corner. A MEGA block (a 2x2 group made one place: a big park, a mall, a downtown plaza)
// has BR_MEGA, its place in the group (BR_PART: 0 top-left, 1 top-right, 2 bottom-left, 3 bottom-right) and what it is (BR_KIND: MK_). On a RIVER block BR_MEGA means it runs north-south.
#define BR_MEGA 8
#define BR_PART(r) (((r) >> 4) & 3)
#define BR_KIND(r) (((r) >> 6) & 3)
enum { MK_PARK, MK_MALL, MK_PLAZA, MK_CAMPUS };   // (CAMPUS: a business park of offices, hotels and shops round a paved court)
enum { ST_CITY, ST_TOWN, ST_VILLAGE };

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
