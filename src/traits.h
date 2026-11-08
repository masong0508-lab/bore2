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

#define TR_SLOTS 10      // slots 1..6 (index 0..5) are the normal ones, 7..10 (index 6..9) unlock with progress (tr_unl); any kind of trait goes in any open slot, a cheat only in 7..10
#define TR_PICK TR_SLOTS
#define TR_NLEN 15
// TrState.f: TF_AUTO: the traits were assigned from the personality (a household member: not editable, no counters, no wish); TF_PEND: assign again (a new member, a new life stage, a new slot);
// bits 4..6: the life stage (AG_*) the slot count follows
enum { TF_CHEAT = 1, TF_CHON = 2, TF_BASE = 4, TF_AUTO = 8, TF_STAGE = 16 * 7, TF_PEND = 128 };
typedef struct {
    u16 tid[TR_SLOTS];
    s8  fx[E_N];                 // the sum of every effect's tier, by hook
    u8  ch[6]; s8 cm[6];         // the cheat traits' effects (kept apart: the OPTIONS switch takes them out of fx and puts them back)
    u8  wish, wbase, title, f;   // lifetime wish 1..WISH_N (0 none), its counter at the start, the last wish done, flags
    u16 skp[9]; u8 skr[9]; u8 ageacc; u8 nacc[7]; s8 mc;   // skill gain watch, the aging and need-fall remainders
} TrState;

extern u8 tr_ev_a[EV_N];          // lifetime counters (saturate at 255), one family-wide set: the Bores that have traits count into it
extern u8 tr_unl;                // unlocked extra slots 0..TR_NUNL (slots 7..10): the household's, saved
extern u8 tr_cheat_opt;          // OPTIONS > GAME > CHEAT TRAITS: 1 on
extern u8 tr_cheat_ok;           // the confirm was said yes once
extern u8 tr_wdone[8];           // wishes done (a bit each), this family
extern u32 tr_earned;
extern u16 tr_recent[8];         // the traits picked last (creator: RECENT)
extern u16 tr_ed_pick[TR_PICK];  // the creator's picks while it runs
// Only a Bore made in the creator has traits: Sim.tr points at one of two states in tr_pool, or at tr_null (all zero, never written) for everyone else.
#define TR_NPOOL 9      // every household member (8) and the creator's staged Bore
extern TrState tr_null, tr_pool[TR_NPOOL], *tr_stage_p;
extern char tr_pnm[TR_NPOOL][TR_SLOTS][TR_NLEN];   // the traits' names, by pool state and slot (the Life panel shows them without the big table)
#define tr_names_of(t) (tr_pnm[(t) - tr_pool])
TrState *tr_alloc(void);          // a free state, cleared; tr_null when both are taken
extern u8 tr_ed_wish, tr_staged; // ... the wish, and 1: sims[MAX_SIMS - 1].tr holds a new Bore's traits (main.c moves them to the Bore it adds)            // dollars earned in all (the income watch)

// ---- the hooks: one array read each ----
static inline int tr_pct(const TrState *t, int e) { return t->fx[e] * tr_unit[e]; }
int tr_scale_f(const TrState *t, int e, int v);                                                                                                   // (out of line: the divide stays out of every call site)
static inline int tr_scale(const TrState *t, int e, int v) { return t->fx[e] ? tr_scale_f(t, e, v) : v; }   // v scaled by the hook (percent)
static inline int tr_add(const TrState *t, int e) { return t->fx[e] * tr_unit[e]; }                                                                           // flat points or levels
int tr_chance_f(const TrState *t, int e, int base);
static inline int tr_chance(const TrState *t, int e, int base) { return t->fx[e] ? tr_chance_f(t, e, base) : base; }                                  // a chance in percent
static inline int tr_floor(const TrState *t) { int m = t->fx[E_MOOD_MIN]; return m > 0 ? CLAMP(-100 + 10 * m, -100, 100) : -100; }                          // the lowest the mood may go
static inline int tr_cheating(const TrState *t) { return (t->f & TF_CHEAT) && tr_cheat_opt; }

static inline int tr_stage_get(const TrState *t) { return (t->f & TF_STAGE) >> 4; }
static inline void tr_stage_set(TrState *t, int st) { t->f = (u8)((t->f & ~TF_STAGE) | ((st & 7) << 4)); }
int  tr_nslots(int stage);                                          // the slots a life stage has now: the normal ones, and the household's unlocked ones from TEEN on
int  tr_unl_check(int wishes_done, int job_level);                 // the unlock conditions against the progress: unlocks in order, returns how many opened now
void tr_clear(TrState *t);
void tr_fx_add(TrState *t, int hook, int tier);                    // add one effect
void tr_apply_id(TrState *t, int id, const u8 *hooks, const s8 *tiers);   // a trait's three effects (cheats are kept apart)
int  tr_give(TrState *t, int id);                                   // an earned / reward trait: into the first free open slot, effects on (1 given, 0 had it or cannot, -1 no free slot)
void tr_cheat_refresh(TrState *t, int now_on);                      // the OPTIONS switch changed: cheat effects out or in
void tr_ev_quiet(TrState *t, int ev, int v);   // a counter takes a value read off the game without winning anything (the start of a life)
int  tr_ev(TrState *t, int ev, int n);                             // count n events (returns the new count)
void tr_ev_set(TrState *t, int ev, int v);                         // a level / state read off the game: raised, never lowered
int  tr_wish_prog(const TrState *t, int *need);                    // the wish's progress and goal (0 when none)
void tr_wish_pick(TrState *t, int w);
// the game provides these (sim.c; a host test stubs them)
void tr_toast(const char *s);
void tr_reward(TrState *t, int kind, int arg, int wish);
// ---- traits_auto.c (CREATE.OVL, the host test): a household member's traits from their personality
typedef struct { u8 bar[5]; u8 sign, asp, stage; u32 seed; } TrPers;   // the 5 personality bars (NEAT OUTGOING ACTIVE PLAYFUL NICE, 0..10), the star sign 0..11, the aspiration, the life stage, the id seed
int  tr_clash(int a, int b);
int  tr_cat_of(int id);
u32  tr_name_seed(const char *first, const char *last);
void tr_rebuild(TrState *t);                                                      // every effect again, from the ids (cheats apart)
int  tr_auto_pick(const TrPers *p, const u16 *inherit, int ninh, u16 *tid, int nslots);   // fills the empty slots below nslots (tid[10], the kept ones stay); returns how many it added
