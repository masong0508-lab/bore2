// arcade: the ARCADE. A floor of cabinets, a token machine, a score wall and a prize counter (arcade.c, the overlay ARCADE.OVL), and
// fourteen games: BORTUA FIGHTER (fight.c) and, in ag_*.c, BORE BREAKER, SNACK CATCH, WHACK-A-BORE, HIGH STRIKER, BORE BLOCKS, STAR BORE,
// TURBO BORE, MUNCH MAZE, HOOPS, SKEE-BORE, AIR HOCKEY, BEAT BORE and CLAW GRAB. Games cost TOKENS (bought with cash) and pay out TICKETS;
// the PRIZE COUNTER swaps tickets for prizes. Only the saved state (arcade_res.c, ArcSave) lives in BORE2.EXE.
#pragma once
#include "common.h"

enum { AG_FIGHTER, AG_BREAKER, AG_CATCH, AG_WHACK, AG_STRIKER, AG_BLOCKS, AG_SHMUP, AG_RACER, AG_MAZE, AG_HOOPS, AG_SKEE, AG_HOCKEY,
       AG_RHYTHM, AG_CLAW, AG_NGAME, AG_PRIZES = 99 };
int  arcade_game_at(int x, int y);           // which game the cabinet on tile (x, y) plays
const char *arcade_game_name(int g);
int  arcade_price(int g);                    // cash at the door (none: the games run on tokens)
void arcade_open(int g);                     // a cabinet's game (the floor opens at that cabinet) or AG_PRIZES (at the prize counter)
int  arcade_update(int held, int pressed, int steps);   // 1 while it runs
void arcade_draw(void);
void arcade_save(void); void arcade_load(void);          // the saved block (save version 45 on)
extern int arcade_tix;                                     // tickets in your pocket (saved with the life)
// what is saved: the three best scores of each game (score in the low 17 bits, the initials in 5 bits each above), the prizes owned
// and the achievements (bits), your tokens, games played, which games you have played
typedef struct { u32 hs[AG_NGAME][3]; u32 prz, ach; u16 tok, plays, pm; } ArcSave;
extern ArcSave arc;
// what travels between the overlays (the floor is ARCADE.OVL, the games ARCADE2.OVL: one ends, main loads the next) and what a challenge needs
typedef struct { u8 chain, g, stake, role, res, prev, cur; s8 sim; u16 foe; char nm[10]; } ArcRun;
extern ArcRun arun;                         // chain: 1 the floor, 2 the games, then g (a game, or 100 + a cabinet for the floor); res: how a challenge ended (1 won, 2 lost)
const char *arcade_file(int g);             // which overlay a cabinet's game is in
const char *arcade_chain(int *arg);         // an arcade overlay ended: the next one to load and its argument (0: leave the arcade)
