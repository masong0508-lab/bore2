// world: the BORE lot (40x40 tiles of floor, walls and items) in 3D.
#pragma once
#include "common.h"

extern char lifeMap[MH][MW + 1];     // the map letters (BORE): '.' floor, 'W' wall, 'w' low wall, 'D' door, items...
extern u8   floorMap[MH][MW];        // floor style per tile
extern u8   wallMap[MH][MW];         // wallpaper per tile (0..NWP-1 patterns, NWP.. the KHLVH set)
#define NWALLS (14 + 89)
#define NFLOORS 23   // BORE's 14 floors + GRASS, PAVING, SOIL, SAND, WATER, ROCK, ASPHALT and the road lines (outdoors)
#define FL_GRASS 14
#define FL_PAVING 15
#define FL_SOIL 16
#define FL_SAND 17
#define FL_WATER 18
#define FL_ROCK 19
#define FL_ASPHALT 20
#define FL_LINE_H 21   // a road's centre line, along x
#define FL_LINE_V 22   // ... along y
// the three zones: the GBA lot, the island (Castaway) and the desert town
enum { ZONE_HOME, ZONE_ISLAND, ZONE_TOWN, ZONE_CITY, ZONE_N };
extern int world_zone;
const char *world_zone_name(int z);
void world_switch(int z);                        // keeps the current zone's tiles, loads (or generates) the other
void world_reset_all(void);                      // a new life: every zone is generated afresh

// the homes the generators build (houses, cottages, townhouses, two-storeys): where a household lives
typedef struct { u8 x0, y0, x1, y1; u8 dx, dy; u16 bx, by; u8 unit; } Home;   // the floor inside the walls (tiles), and the door tile (on the south wall; outside is dy + 1); in the open world, its block and which home of the block it is
#define MAXHOMES 64
int world_home_count(int zone);
const Home *world_home(int zone, int i);
int world_home_find(int zone, int bx, int by, int unit);   // the open world: the home of that block's unit, if it is in the window (-1)

// ---- SAN BOREAS (the open world, ZONE_CITY): the map is a 5x5-block window onto it, kept round the player ----
extern int world_wbx, world_wby;                 // the window's top-left block (-1 before the world is first seen)
void world_new_seed(u32 seed);                   // a new life: a new world
void world_open_at(int bx, int by);              // build the window round that block
int  world_stream(int ptx, int pty, int *dtx, int *dty);   // keep it round the player's tile: 1 if it moved (everything shifts by -dtx, -dty tiles)
void world_arrival(int set, int *px, int *py);   // open the window at a town and say where to stand (the street by its square)

void world_init(void);
void world_touched(void);                        // the map changed (build mode): the floor shading is worked out again
void world_gen(void);                            // BORE's default lot: THE MAN BASE
void world_gen_studio(void);                     // the creator's little room (a dresser with a mirror, a lamp, a rug)
void world_backup(void);                         // keep the live lot aside while the studio is shown ...
void world_restore(void);                        // ... and bring it back
void world_draw(int fx, int fy, int t, int cutaway);   // fx,fy = the tile the camera cares about (walls in front of it are cut low)
int  world_walkable(int mx, int my);
int  world_floor_y(int mx, int my);              // the height you stand at (crates, funboxes)
char world_at(int mx, int my);
int  world_item_facing(int mx, int my);          // 0 S, 1 E, 2 N, 3 W: away from the wall, towards open floor
int  world_along(int mx, int my, char ch);
void world_find(char ch, int *mx, int *my);      // the first tile holding ch (-1 if none)
const char *world_wall_name(int wp);
const char *world_floor_name(int fl);
int  world_room_score(int mx, int my);           // how nice the place you stand in is (0..100): furniture nearby
int  world_count(char ch);

// ---- storeys and roofs: the ground plus four floors above (5 in all), and a roof over every enclosed building ----
#define NUP 4
#define NLEVELS (NUP + 1)
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
