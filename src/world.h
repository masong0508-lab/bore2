// world: the BORE lot (40x40 tiles of floor, walls and items) in 3D.
#pragma once
#include "common.h"

extern char lifeMap[MH][MW + 1];     // the map letters (BORE): '.' floor, 'W' wall, 'w' low wall, 'D' door, items...
extern u8   floorMap[MH][MW];        // floor style per tile
extern u8   wallMap[MH][MW];         // wallpaper per tile (0..NWP-1 patterns, NWP.. the KHLVH set)
#define NWALLS (14 + 89 + 6)   // (the last six: brick, clapboard, stucco, shingle, beadboard, stone: appended so old maps keep their walls)
#define NFLOORS 33   // BORE's 14 floors + GRASS, PAVING, SOIL, SAND, WATER, ROCK, ASPHALT and the road lines (outdoors)
#define FL_GRASS 14
#define FL_PAVING 15
#define FL_SOIL 16
#define FL_SAND 17
#define FL_WATER 18
#define FL_ROCK 19
#define FL_ASPHALT 20
#define FL_LINE_H 21   // a road's centre line, along x
#define FL_LINE_V 22
#define FL_PATIO 23    // brick paving, a deck, pool tile, gravel (appended: old maps keep their floors)
#define FL_DECK 24
#define FL_POOLTILE 25
#define FL_GRAVEL 26   // ... along y
#define FL_ZEB_A 27    // a crosswalk across a street that runs along x (bars along x), and its twin
#define FL_ZEB_B 28
#define FL_EDGE_N 29   // the white line along a street's edge: north, south, west, east
#define FL_EDGE_S 30
#define FL_EDGE_W 31
#define FL_EDGE_E 32
#define FL_NO4(f) (((f) >= FL_LINE_H && (f) <= FL_LINE_V) || (f) >= FL_ZEB_A)   // floors with no 2x2-block texture
#define FL_IS_ROAD(f) ((f) == FL_ASPHALT || (f) == FL_LINE_H || (f) == FL_LINE_V || (f) >= FL_ZEB_A)
// the three zones: the GBA lot, the island (Castaway) and the desert town
enum { ZONE_HOME, ZONE_ISLAND, ZONE_TOWN, ZONE_CITY, ZONE_N };
extern int world_zone;
const char *world_zone_name(int z);
void world_season_skin(int season, int wet);     // the outdoor floors dressed for the season (0..3) and for the weather: wet 1 rain / hail, 2 snow lying

void world_switch(int z);                        // keeps the current zone's tiles, loads (or generates) the other
void world_reset_all(void);                      // a new life: every zone is generated afresh

// the homes the generators build (houses, cottages, townhouses, two-storeys): where a household lives
typedef struct { u8 x0, y0, x1, y1; u8 dx, dy; u16 bx, by; u8 unit; } Home;   // the floor inside the walls (tiles), and the door tile (on the south wall; outside is dy + 1); in the open world, its block and which home of the block it is
#define MAXHOMES 64
int world_home_count(int zone);
const Home *world_home(int zone, int i);
int world_home_find(int zone, int bx, int by, int unit);   // the open world: the home of that block's unit, if it is in the window (-1)

// ---- SAN BOREAS (the open world, ZONE_CITY): the map is a 5x5-block window onto it, kept round the player ----
extern int world_wbx, world_wby;
extern int world_stamp;                          // changes whenever the map's tiles do (a window move, a new zone, build mode)                 // the window's top-left block (-1 before the world is first seen)
extern int isl_size, isl_wild, isl_jungle, isl_rocks;   // CREATE A WORLD's island: SIZE (0..4: very small .. very big; 2 as it always was), COAST, JUNGLE, HILLS (0..2; 1)
void isl_dials_default(void);
void world_isl_preview(u32 seed, u16 *px);            // that seed's island from above, 128 x 128 pixels
void world_new_seed(u32 seed);                   // a new life: a new world
u32  world_seed(void);                           // this world's seed (the memory card)
void world_open_at(int bx, int by);              // build the window round that block
int  world_stream(int ptx, int pty, int *dtx, int *dty);   // keep it round the player's tile: 1 if it moved (everything shifts by -dtx, -dty tiles)
int  world_stream_now(int ptx, int pty, int *dtx, int *dty, int now);   // now 0: the move may wait for a frame with time to spare (world_stream_want)
extern int world_stream_want, world_stream_hb;                           // (a move is waiting; what one costs lately)
void world_arrival(int set, int *px, int *py);   // open the window at a town and say where to stand (the street by its square)
// the family's own lot (home.c): one block of SAN BOREAS, kept as they build it, every storey (with the properties' tiles)
void world_own_set(int bx, int by, const u8 *occ);   // bought (occ: adults, kids, teens, elders, a couple, babies: it comes furnished for them; 0: a property of theirs, as it is)
void world_own_clear(void);                      // sold
int  world_own_lot(int tx, int ty);              // that tile of the window is on the lot (build mode)
int  world_own_door(int *tx, int *ty);           // where the family stands on getting home (0: the lot is out of the window)
int  world_own_block(int *wx, int *wy);          // the lot's block in the window (0: not in it)
// the family's properties (home.c): up to 8 more blocks they own but do not live in, kept as they leave them
int  world_prop_add(int bx, int by);             // bought as a property (0: no room on the list); called again for it: sold
int  world_prop_from_home(void);                 // the family moves out: their lot becomes a property, as it is (0: no room)
void world_prop_to_home(int bx, int by, const u8 *occ);   // ... and into a property: it is their lot now (furnished for them if nothing on it changed)
int  world_prop_lot(int tx, int ty);             // that tile of the window is on a block of theirs, a property or the home (build mode): not 0

void world_init(void);
void world_touched(void);                        // the map changed (build mode): the floor shading is worked out again
void world_gen(void);                            // BORE's default lot: THE MAN BASE
void world_gen_studio(void);                     // the creator's little room (a dresser with a mirror, a lamp, a rug)
int  world_backup(void);                         // keep the live lot aside while the studio is shown (0: no room just now) ...
void world_restore(void);                        // ... and bring it back
void world_draw(int fx, int fy, int t, int cutaway);   // fx,fy = the tile the camera cares about (walls in front of it are cut low)
int  world_walkable(int mx, int my);
void world_doors_reset(void);                    // doors: forget who stood where last frame ...
void world_door_mark(int tx, int ty);            // ... and say who stands where now (a door beside them slides open)
int  world_floor_y(int mx, int my);              // the height you stand at (crates, funboxes)
char world_at(int mx, int my);
int  world_covered(int mx, int my);              // 1: the far end of a long thing (a bed) from a neighbouring tile lies here
int  world_item_facing(int mx, int my);          // 0 S, 1 E, 2 N, 3 W: away from the wall, towards open floor
int  world_along(int mx, int my, char ch);
void world_find(char ch, int *mx, int *my);      // the first tile holding ch (-1 if none)
const char *world_wall_name(int wp);
const char *world_floor_name(int fl);
int  world_room_score(int mx, int my);           // how nice the place you stand in is (0..100): furniture nearby
int  world_count(char ch);

// ---- storeys and roofs: the ground plus five floors above (6 in all), and a roof over every enclosed building ----
#define NUP 5
#define NLEVELS (NUP + 1)
int world_gh(int x, int z);                      // the hills: the ground under a world point (units up; 0 on the flat)
int world_gh_c(int X, int Y);                    // ... at a tile corner
int world_hills_on(void);
void world_gh_reset(void);
extern int world_lvl;                            // the storey world_at / world_walkable read (the Bore being moved, the build cursor)
extern int world_view_lvl;                       // storeys above this are not drawn (the Sims 2 floor view)
extern int world_roof_show;                      // 1: the roof over the focused building shows too (build mode's roof tool)
extern int world_roof_style, world_roof_col, world_roof_pitch;   // HIP / GABLE / FLAT, the shingle colour, LOW / MID / STEEP
#define NROOF_STYLE 3
#define NROOF_COL 6
#define NROOF_PITCH 3
const char *world_roof_style_name(int s);
const char *world_roof_col_name(int c);
const char *world_roof_pitch_name(int p);
char world_at_l(int l, int mx, int my);          // any storey (' ' = open air on the upper floors)
char *world_cell(int l, int mx, int my);         // build mode: the tile itself on storey l
u8  *world_floor_cell(int l, int mx, int my);
u8  *world_wall_cell(int l, int mx, int my);
void world_cam_push(int l);                      // draw what follows as if it stood on storey l (the camera drops by l storeys)
void world_cam_pop(void);

// the memory card (save.c)
void world_save(int with_home);                  // the lots, the window (with_home: THE MAN BASE is the family's home, so its tiles are kept too)
void world_load(void);                           // (after world_new_seed and world_reset_all)
void world_place(void);                          // ... then go to where the save was
