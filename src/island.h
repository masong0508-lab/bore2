// island: CASTAWAY ISLAND and the castaway's life. world.c makes the island itself (a new one every life: its coast, beaches,
// meadows, jungle, rocky hills, a pond, an islet out at sea, and the secrets hidden round it); here is what the family gathers
// there (wood, stone, leaves, food), what they make from it at the campfire, and the island's eight secrets.
#pragma once
#include "common.h"

enum { G_NONE, G_WOOD, G_STONE, G_LEAVES, G_BERRIES, G_COCONUT, G_FISH, G_DIG, G_WRECK, G_BOTTLE, G_JOURNAL, G_IDOL, G_SHELL, G_GOLD };   // what gathering gets
enum { SEC_BOTTLE, SEC_TREASURE, SEC_IDOL, SEC_WRECK, SEC_SHELLS, SEC_JOURNAL, SEC_ISLET, SEC_GOLD, SEC_N };
#define ISL_SHELLS 6
extern int isl_wood, isl_stone, isl_leaves, isl_food;   // the castaway's stores
extern int isl_spear;                  // 1: a spear (fishing is quicker, and catches more)
extern int isl_shells;                 // seashells picked up
extern u16 isl_secrets;                // a bit for each secret found
extern int isl_craft_asked;            // the campfire's CRAFT: main opens the list
extern u8  isl_dock_x, isl_dock_y, isl_islet_x, isl_islet_y;   // (world.c) where a raft leaves the island from, and where it lands on the islet
extern u8  isl_fire_x, isl_fire_y;     // (world.c) the camp's fire: things made are put by it
void isl_reset(void);                  // a new life
int  isl_secret_count(void);
const char *isl_secret_name(int sec);
void isl_found(int sec);               // a secret found (once): a note and a fanfare
#define ISL_NCRAFT 6
typedef struct { const char *name; u8 wood, stone, leaves; s8 icon; const char *blurb; } Craft;
extern const Craft isl_crafts[ISL_NCRAFT];
int  isl_can_make(int i);              // there is enough for it (and it is not made already)
int  isl_make(int i);                  // make it, by the campfire: 1 made, 0 not enough (or made already), -1 no room for it
void isl_save(void);                   // the memory card (save.h)
void isl_load(void);
