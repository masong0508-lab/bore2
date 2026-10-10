// world: the BORE lot (40x40 tiles of floor, walls and items) in 3D.
#pragma once
#include "common.h"

extern char lifeMap[MH][MW + 1];     // the map letters (BORE): '.' floor, 'W' wall, 'w' low wall, 'D' door, items...
extern u8   floorMap[MH][MW];        // floor style per tile
extern u8   wallMap[MH][MW];         // wallpaper per tile (0..NWP-1 patterns, NWP.. the KHLVH set)
#define NWALLS (14 + 89)
#define NFLOORS 20   // BORE's 14 floors + GRASS, PAVING, SOIL, SAND, WATER, ROCK (outdoors)
#define FL_GRASS 14
#define FL_PAVING 15
#define FL_SOIL 16
#define FL_SAND 17
#define FL_WATER 18
#define FL_ROCK 19
// the three zones: the GBA lot, the island (Castaway) and the desert town
enum { ZONE_HOME, ZONE_ISLAND, ZONE_TOWN, ZONE_CITY, ZONE_N };
extern int world_zone;
const char *world_zone_name(int z);
void world_switch(int z);                        // keeps the current zone's tiles, loads (or generates) the other
void world_reset_all(void);                      // a new life: every zone is generated afresh

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
