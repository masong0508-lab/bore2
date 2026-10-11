// wants: the heart of The Sims 2, BORE 2 style. Every person has 4 wants and 2 fears that come from their aspiration,
// their age and their personality; doing a want pays aspiration points, a fear coming true takes them; the meter runs
// red / green / gold / PLATINUM (mood pinned at full) and decays. See DESIGN.md section 4.
#pragma once
#include "sim.h"

enum { W_NONE, W_EAT, W_SLEEP, W_SHOWER, W_FUN, W_READ, W_COFFEE, W_RUN, W_PHONE, W_CHAT, W_CHAT_WITH, W_FRIEND,
       W_WORK, W_PAID, W_PROMO, W_CASH, W_BUY, W_SK_COOK, W_SK_BODY, W_SK_FUN, W_ISLAND, W_TOWN, W_DINER, W_SWIM, W_GYM, W_SHOP, W_FILM, W_NIGHT, W_N };
enum { F_NONE, F_PASSOUT, F_PEE, F_DEMOTED, F_BADDAY, F_BROKE, F_REJECT, F_N };
enum { ASP_RED, ASP_GREEN, ASP_GOLD, ASP_PLATINUM };
#define ASP_MAX 12000

void wants_roll_all(Sim *s);                  // a fresh set (a new person, a new life)
void wants_reroll(Sim *s);                    // waking, arriving somewhere: the empty and the stale slots roll again
void wants_event(Sim *s, int kind, int arg);  // something happened to s (arg: the other sim, an amount, a level)
void wants_fear(Sim *s, int kind);            // something bad happened to s
void wants_minute(Sim *s);                    // once a game minute: decay, expiry, the money and skill checks
int  wants_level(const Sim *s);               // ASP_*
int  wants_desperate(const Sim *s);           // rock bottom: acting out
void wants_text(const Sim *s, int slot, char *out);   // "CHAT WITH MOSS" (out: 32 chars)
void fears_text(const Sim *s, int slot, char *out);
int  wants_skill_level(int raw);              // a skill counter -> 0..10
extern int wants_flash_t; extern char wants_flash[32];   // the last "WANT DONE +1500" / "FEAR -1000", for the HUD
