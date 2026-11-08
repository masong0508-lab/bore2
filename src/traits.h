// traits: the trait / effect-hook engine (docs/TRAITS.md). A trait is a short list of (hook, tier) effects; the sum for each person is kept in
// TrState.fx, so a hook costs one array read where the game uses it. The data is made by tools/make_traits.py (src/traits_data.h).
#pragma once
#ifdef TR_HOST
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8; typedef int16_t s16; typedef int32_t s32;
#define CLAMP(v,lo,hi) ((v)<(lo)?(lo):(v)>(hi)?(hi):(v))
#else
#include "common.h"
#endif
#include "traits_data.h"

#define TR_SLOTS 10      // 0..4 chosen in the creator (3, five for adults and elders), 5..9 earned or won
#define TR_PICK 5
#define TR_NLEN 15
enum { TF_CHEAT = 1, TF_CHON = 2 };
typedef struct {
    u16 tid[TR_SLOTS];
    s8  fx[E_N];                 // the sum of every effect's tier, by hook
    u8  ev[EV_N];                // lifetime counters (saturate at 255)
    u8  ch[6]; s8 cm[6];         // the cheat traits' effects (kept apart: the OPTIONS switch takes them out of fx and puts them back)
    char tn[TR_PICK][TR_NLEN];   // the chosen traits' names (the Life page shows them without the big table)
    u8  wish, wbase, title, f;   // lifetime wish 1..WISH_N (0 none), its counter at the start, the last wish done, flags
    u16 skp[9]; u8 skr[9]; u8 ageacc; u8 nacc[7]; s8 mc;   // skill gain watch, the aging and need-fall remainders
} TrState;

extern u8 tr_cheat_opt;          // OPTIONS > GAME > CHEAT TRAITS: 1 on
extern u8 tr_cheat_ok;           // the confirm was said yes once
extern u8 tr_wdone[8];           // wishes done (a bit each), this family
extern u32 tr_earned;            // dollars earned in all (the income watch)

// ---- the hooks: one array read each ----
static inline int tr_pct(const TrState *t, int e) { return t->fx[e] * tr_unit[e]; }
int tr_scale_f(const TrState *t, int e, int v);                                                                                                   // (out of line: the divide stays out of every call site)
static inline int tr_scale(const TrState *t, int e, int v) { return t->fx[e] ? tr_scale_f(t, e, v) : v; }   // v scaled by the hook (percent)
static inline int tr_add(const TrState *t, int e) { return t->fx[e] * tr_unit[e]; }                                                                           // flat points or levels
int tr_chance_f(const TrState *t, int e, int base);
static inline int tr_chance(const TrState *t, int e, int base) { return t->fx[e] ? tr_chance_f(t, e, base) : base; }                                  // a chance in percent
static inline int tr_floor(const TrState *t) { int m = t->fx[E_MOOD_MIN]; return m > 0 ? CLAMP(-100 + 10 * m, -100, 100) : -100; }                          // the lowest the mood may go
static inline int tr_cheating(const TrState *t) { return (t->f & TF_CHEAT) && tr_cheat_opt; }

void tr_clear(TrState *t);
void tr_fx_add(TrState *t, int hook, int tier);                    // add one effect
void tr_apply_id(TrState *t, int id, const u8 *hooks, const s8 *tiers);   // a trait's three effects (cheats are kept apart)
int  tr_give(TrState *t, int id);                                   // an earned / reward trait: into a free slot 5..9, effects on
void tr_cheat_refresh(TrState *t, int now_on);                      // the OPTIONS switch changed: cheat effects out or in
const char *tr_small_name(int id);
int  tr_ev(TrState *t, int ev, int n);                             // count n events (returns the new count)
void tr_ev_set(TrState *t, int ev, int v);                         // a level / state read off the game: raised, never lowered
int  tr_wish_prog(const TrState *t, int *need);                    // the wish's progress and goal (0 when none)
void tr_wish_pick(TrState *t, int w);
// the game provides these (sim.c; a host test stubs them)
void tr_toast(const char *s);
void tr_reward(TrState *t, int kind, int arg, int wish);
