// items: BORE's room items (textured boxes) in 3D, keyed by their map letter.
#pragma once
#include "dog.h"
#include "common.h"

void items_init(void);                                   // uploads every material to the atlas
// draw the item letter ch standing on tile (mx,my). rot = facing 0..3 (0 = south, towards the camera at the default view),
// along = 1 when a rail / ledge / bench runs along x. t = the frame counter (lava lamp, aquarium, fire animate)
void items_draw(char ch, int mx, int my, int rot, int along, int t);
int  items_ramp_h(char ch, int facing, int lx, int lv);   // (old: 0)
int  items_ramp_at(int mx, int my, int lx, int lv);       // a ramp's surface height at (lx, lv) of tile (mx, my), the run it is part of counted (0: no ramp)
int  items_ramp_lip(int mx, int my, int *dir);            // this tile holds a run's lip: 1 (2 a quarter pipe's), *dir its way (0 N 1 W 2 S 3 E)
int  items_ramp_rot(char c, int facing);
void items_draw_vehicle(int x, int z, int yaw, int y, int type, int colour, int wheel_phase);   // a vehicle under the driver (VEH_ types: sim.h)
void items_draw_pet(int x, int z, int yaw, int y, int kind, int colour, int phase);   // a dog (1) or a cat (2); the feet move with phase
void items_draw_dog(int x, int z, int yaw, int y, const DogLook *d, int phase);   // a dog from its look (dog.h)
void items_draw_bus(int x, int z, int yaw, int y);
int  bus_here(int mx, int my);                           // 1 while a bus is standing at the stop on that tile
void items_draw_board(int x, int z, int yaw, int y, int roll, int pitch, int spin);   // the ridden board (turned by a flip)
int  items_blocking(char ch);                            // 1: a wall of a thing you cannot walk through
int  items_is_item(char ch);                             // 1: a placeable item letter
const char *items_name(char ch);
int  items_height(char ch);                              // the top of the item in world units (for standing on crates etc.)
int  items_hit(char ch, int rot, int along, int cu, int cv, int r);   // does a body (a square of half-width r) at (cu, cv) in the item's tile, world units from its corner, touch one of its solid boxes? -1: no footprint data (ramps, stairs): the whole tile
int  items_rotates(char ch);                             // 1: it faces away from the wall
int  items_along(char ch);                               // 1: rails, ledges, benches, barriers link up along a line
int  items_count(void);
char items_at(int i);                                    // the i-th placeable letter (the build menu order)
int  items_cost(char ch);
// BORE 2's second furniture pack lives at 0x90..0xBD (tools/make_items2.py); 0x8E..0x8F are kept free
// the family's own things (letters past the old ASCII ones: the map has no plain letters left)
#define CH_CRIB  ((char)0x81)                            // a baby sleeps in it
#define CH_BED4  ((char)0xEB)                            // a four-poster bed
#define CH_BEDF  ((char)0xEC)                            // a family bed (a couple, or a parent and a little one)
#define CH_BEDT  ((char)0xED)                            // a toddler bed
#define CH_BASS  ((char)0xEE)                            // a bassinet (a baby sleeps in it, like the crib)
#define CH_CRAD  ((char)0xEF)                            // a cradle
#define CH_CAKE  ((char)0x82)                            // a birthday cake on its party table (baked at the fridge)
#define CH_GRAVE ((char)0x83)                            // where an elder of the family rests
// the island's own (Castaway): its secrets, and what a castaway lives off
#define CH_BOTTLE  ((char)0x84)                          // a message in a bottle, washed up on a beach
#define CH_DIG     ((char)0x85)                          // a mound of sand with an X on it: dig
#define CH_IDOL    ((char)0x86)                          // a stone tiki head, deep in the jungle
#define CH_WRECK   ((char)0x87)                          // a shipwreck on the shore
#define CH_SHELL   ((char)0x88)                          // a seashell
#define CH_BUSH    ((char)0x89)                          // a berry bush
#define CH_FISH    ((char)0x8A)                          // a fishing spot: a post and a net
#define CH_RAFT    ((char)0x8B)                          // a raft (made at the campfire)
#define CH_GOLD    ((char)0x8C)                          // the golden idol on its plinth (out on the islet)
#define CH_JOURNAL ((char)0x8D)                          // the old hermit's journal on a stump
