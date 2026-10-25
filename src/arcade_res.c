// arcade_res.c - the part of the ARCADE that stays in BORE2.EXE (the games themselves are ARCADE.OVL): tickets, the fight record, and
// which cabinet plays what (the pie menus and the saves need them while you are out in the town).
#include "arcade.h"
#include "fight.h"
int arcade_tix, fight_wins, fight_champ, fight_best;
const char *const arcade_gname[AG_NGAME] = { "BORTUA FIGHTER", "BORE BREAKER", "SNACK CATCH", "WHACK-A-BORE", "HIGH STRIKER" };
const u8 arcade_gprice[AG_NGAME] = { 2, 2, 1, 1, 1 };
int arcade_game_at(int x, int y) { return (x * 3 + y * 2) % AG_NGAME; }
const char *arcade_game_name(int g) { return g >= 0 && g < AG_NGAME ? arcade_gname[g] : "THE PRIZES"; }
int arcade_price(int g) { return g >= 0 && g < AG_NGAME ? arcade_gprice[g] : 0; }
