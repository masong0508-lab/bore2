// arcade: the ARCADE's cabinets. Each cabinet has its own game: BORTUA FIGHTER (fight.c), and from BORE on the GBA BORE BREAKER,
// SNACK CATCH, WHACK-A-BORE and HIGH STRIKER. Games pay out TICKETS; the PRIZE COUNTER (the arcade's till) swaps them for prizes.
#pragma once
#include "common.h"

enum { AG_FIGHTER, AG_BREAKER, AG_CATCH, AG_WHACK, AG_STRIKER, AG_NGAME, AG_PRIZES = 99 };
int  arcade_game_at(int x, int y);           // which game the cabinet on tile (x, y) plays
const char *arcade_game_name(int g);
int  arcade_price(int g);
void arcade_open(int g);                     // paid already (AG_PRIZES: the prize counter)
int  arcade_update(int held, int pressed, int steps);   // 1 while it runs
void arcade_draw(void);
extern int arcade_tix;                       // tickets in your pocket (saved with the life)
