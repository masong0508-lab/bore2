// arcade_res.c - the part of the ARCADE that stays in BORE2.EXE (the floor and the games are ARCADE.OVL and ARCADE2.OVL): tickets, the fight
// record, the saved block, what passes from one arcade overlay to the next, and which cabinet plays what (the pie menus and saves need them).
#include "arcade.h"
#include "fight.h"
#include "save.h"
int arcade_tix, fight_wins, fight_champ, fight_best;
ArcSave arc; ArcRun arun;
int arcade_game_at(int x, int y) { return (x * 3 + y * 2) % AG_NGAME; }
const char *arcade_game_name(int g) { return g == AG_PRIZES ? "THE PRIZES" : "AN ARCADE GAME"; }
int arcade_price(int g) { return 0; }
const char *arcade_file(int g) { return g == AG_FIGHTER || g >= AG_PRIZES ? "ARCADE.OVL" : "ARCADE2.OVL"; }
const char *arcade_chain(int *arg) { int c = arun.chain; arun.chain = 0; *arg = arun.g; return c ? (c == 1 ? "ARCADE.OVL" : "ARCADE2.OVL") : 0; }
void arcade_save(void) { sv_rle(&arc, sizeof arc); }
void arcade_load(void) { ld_rle(&arc, sizeof arc); }
