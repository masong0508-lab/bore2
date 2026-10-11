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
int  home_buy(int bx, int by);         // 1: bought (the old home in SAN BOREAS is sold first)
const char *home_address(int bx, int by, char *out);   // "214 MAPLE ST"
const char *home_kind(int lot);        // "COTTAGE", "EMPTY LOT"
const char *home_blurb(int lot);       // what you get
int  home_is_here(void);               // the one you play is at home right now
void home_reset(void);                 // a new life: THE MAN BASE again
