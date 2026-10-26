// gotchi: the BOREAGOTCHI! expansion pack's data and rules (the screens are the overlay GOTCHI.OVL, gotchi_ui.c, so the pack stays a
// separate file on the disc). A toy costs $5000 at an electronics shop and belongs to one Bore of the family; each toy holds up to
// EIGHT creatures (eggs, babies ... grown-ups), 16 kinds, each growing into one of four forms by how it was looked after.
#pragma once
#include "common.h"

#define GT_MAX 6
#define GB_N 8
#define GSP_N 16
#define GOTCHI_PRICE 5000
#define GOTCHI_EGG 150
enum { GS_EGG, GS_BABY, GS_CHILD, GS_TEEN, GS_ADULT };
enum { GF_SICK = 1, GF_SLEEP = 2, GF_DARK = 4, GF_CALL = 8, GF_FAKE = 16 };
enum { GM_BUDDY, GM_HATCH, GM_PROUD, GM_WORRY, GM_MISS, GM_HOOKED, GM_SHARED };   // the moods it gives its owner
typedef struct { u8 on, stage, form, sp, nm, hun, hap, disc, wt, poop, fl, miss, needT, bond, snk, meal, gen; u16 age; } Blob;
typedef struct { u8 uid, own, sel, pad; u16 xp, hatched, wins, cared; Blob b[GB_N]; } Toy;
extern Toy gtoy[GT_MAX];
extern int sim_want_gotchi;
extern int gotchi_time;                    // 0 the game clock, 1 LIVE in hand (the toy runs on real time while you hold it), 2 off (they wait)

void  gotchi_reset(void);
void  gotchi_minute(void);                 // from the game clock: every creature lives on
void  gotchi_save(void); void gotchi_load(int ver);
Toy  *gotchi_toy(int simi);                // the toy that sim owns (0: none)
int   gotchi_sim_of(const Toy *t);         // its owner's sim index, -1 when gone
int   gotchi_buy(int simi);                // a toy and its first egg for that sim (0 when it has one already)
int   gotchi_any(void);                    // does anyone in the family own one?
int   gotchi_count(const Toy *t);          // creatures in it (eggs too)
int   gotchi_calling(const Toy *t);        // how many of them need you
int   gotchi_level(const Toy *t);          // care skill 0..5
const char *gotchi_level_name(int lv);
int   gotchi_next_xp(const Toy *t);        // xp for the next level (0: the top)
int   gotchi_needs(const Blob *b);
const char *gotchi_want(const Blob *b);    // what it is asking for
int   gotchi_egg(Toy *t, int slot);        // a new egg in an empty slot (paid by the caller)
void  gotchi_name(const Blob *b, char *out);   // "SUPER FROG"
const char *gotchi_given(int nm);
const char *gotchi_species(int sp);
// the actions: each returns what to say
const char *gotchi_feed(Toy *t, Blob *b, int snack);
const char *gotchi_light(Toy *t, Blob *b);
const char *gotchi_medicine(Toy *t, Blob *b);
const char *gotchi_clean(Toy *t, Blob *b);
const char *gotchi_scold(Toy *t, Blob *b);
const char *gotchi_game(Toy *t, Blob *b, int won);    // after a mini-game
const char *gotchi_friends(Toy *t, int a, int c, int battle);   // a playdate or a battle between two of them
void  gotchi_mood(const Toy *t, int kind);
// the Blobdex
void  gotchi_dex_see(int sp, int form);
int   gotchi_dex_has(int sp, int form);
int   gotchi_dex_count(void);
void  gotchi_live(int steps);                // called while the handheld is open: LIVE time and the hijinks
const char *gotchi_lunch(Toy *t, Blob *b); // give it your own lunch
const char *gotchi_nuzzle(Toy *t, Blob *b); // it cheers you up
const char *gotchi_house(int simi, int what);   // objects around the house: 0 feed them all, 1 lights, 2 tuck in
