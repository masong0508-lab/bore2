// home: where the family lives. THE MAN BASE at first; then, if they buy one, a home of their own in SAN BOREAS: a house, a
// cottage or a two-storey that is for sale (it comes furnished for them), or an empty lot to build on. That block of the open
// world is theirs from then on (world.c keeps it just as they build it, every storey), and it sells again when they move on.
#pragma once
#include "common.h"

typedef struct { u16 bx, by; u8 lot, set; int price; } HomeAd;   // a home for sale
extern int  home_city;                 // 1: the family's home is in SAN BOREAS (0: THE MAN BASE)
extern int  home_bx, home_by;          // its block
extern int  home_paid;                 // what it cost (and what it sells for when they move again)
extern int  home_move_asked;           // a home was just bought: main moves the family in
int  home_for_sale(int bx, int by);    // a house, cottage or two-storey nobody lives in, or an empty lot, in a town
int  home_price(int bx, int by);
int  home_list(int cbx, int cby, HomeAd *out, int n);   // the homes for sale nearest that block, nearest first
enum { HB_KEEP, HB_SELL, HB_PROP };     // how to buy: move in and keep the old home as a property (sold if the list is full);
                                       // move in and sell the old home; or keep it as a property and live where you are
int  home_buy(int bx, int by, int how);   // 1: bought
// PROPERTIES: up to 8 more places the family owns but does not live in (a house, a cottage, a two-storey, an empty lot).
// Theirs to visit and build on; they sell back for what was paid, and the family can move into one (the old home becomes a property)
#define PROP_MAX 8
typedef struct { s16 bx, by; int paid; } Prop;
extern Prop props[PROP_MAX];
extern int  nprops;
int  prop_find(int bx, int by);        // which property that block is (-1: not one)
int  prop_here(void);                  // the property the one you play stands on (-1: none)
int  prop_move_in(int i);              // 1: the family lives there now (main moves them in)
int  prop_sell(int i);                 // 1: sold, for what was paid
void prop_save(void);
void prop_load(void);                  // (save version 42 on; older saves have none)
const char *home_address(int bx, int by, char *out);   // "214 MAPLE ST"
const char *home_kind(int lot);        // "COTTAGE", "EMPTY LOT"
const char *home_blurb(int lot);       // what you get
int  home_is_here(void);               // the one you play is at home right now
void home_reset(void);                 // a new life: THE MAN BASE again
int  home_start(void);                 // a new life, once the household exists: give it a house in SAN BOREAS's capital (1: done; main moves it in)
