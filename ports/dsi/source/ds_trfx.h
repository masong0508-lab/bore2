// ds_trfx.h: the Nintendo DS side of the PS1's traits (docs/TRAITS.md, docs/DS_TRAITS.md).
// Plain C on TrState and ints only: main.c includes it, and the host test (ports/dsi/test/ds_traits_test.c) includes the very same file, so the numbers the
// test checks are the numbers the game uses. The engine (src/traits.c, traits_auto.c, traits_data.h) is the PS1's own, compiled for the DS in ds_trdata.c.
//
// THE DS POOL. A trait is OFFERED on the DS only when every effect it has is a hook the DS reads (TRF_SUP below) and, for an earned trait, its counter is one the DS
// raises (TRF_EVSUP). So nothing offered is cosmetic. The hooks the DS has no system for are listed in docs/DS_TRAITS.md; traits that use one are left out.
#ifndef DS_TRFX_H
#define DS_TRFX_H
#include <string.h>
#ifndef TR_HOST
#define TR_HOST
#endif
#define TR_BIG
#include "traits.c"        // the PS1's engine, as it is (-I src): one translation unit with the DS game, so the data is there once
#include "traits_auto.c"
#include "persona.c"       // (D76) the PS1's persona_of / persona_town_seed: a townie's personality from the world seed and their unique id

// DS need index (0 hunger, 1 fun, 2 energy, 3 social, 4 bladder, 5 hygiene) -> the PS1's hook
static const u8 TRF_DEC[6] = { E_DEC_FOOD, E_DEC_FUN, E_DEC_REST, E_DEC_SOCIAL, E_DEC_WC, E_DEC_CLEAN };
static const u8 TRF_GAIN[6] = { E_GAIN_FOOD, E_GAIN_FUN, E_GAIN_REST, E_GAIN_SOCIAL, E_GAIN_WC, E_GAIN_CLEAN };
// the DS's six jobs (Line Cook, Shop Clerk, Lifeguard, Barista, Teacher, Office Temp) -> the PS1's career pay hook
static const u8 TRF_PAY[6] = { E_PAY_FOOD, E_PAY_RETAIL, E_PAY_SWIM, E_PAY_FOOD, E_PAY_TEACH, E_PAY_BIZ };

// hooks the DS reads (80 of 107)
static const u8 TRF_SUP[] = {
    E_DEC_FOOD, E_DEC_WC, E_DEC_REST, E_DEC_CLEAN, E_DEC_FUN, E_DEC_SOCIAL,
    E_GAIN_FOOD, E_GAIN_WC, E_GAIN_REST, E_GAIN_CLEAN, E_GAIN_FUN, E_GAIN_SOCIAL,
    E_ACT_EAT, E_ACT_SLEEP, E_ACT_SHOWER, E_ACT_SIT, E_ACT_FUN, E_ACT_READ, E_ACT_RUN, E_ACT_SWIM, E_ACT_TALK, E_ACT_PHONE, E_ACT_COFFEE,
    E_SK_FARM,
    E_MOOD_BASE, E_MOOD_MIN,
    E_REL_GAIN, E_REL_LOSS, E_CHAT_OK, E_JOKE_OK, E_CHARM, E_FLIRT_OK, E_ROMANCE, E_HUG_OK, E_LOVE_MOOD, E_ENGAGE,
    E_JOB_PAY, E_JOB_PERF, E_JOB_PROMO, E_JOB_STRIKE, E_PAY_FOOD, E_PAY_RETAIL, E_PAY_SWIM, E_PAY_BIZ, E_PAY_TEACH, E_PAY_FARM,
    E_INCOME, E_SPEND, E_SHOP_DISC,
    E_WALK_SPEED, E_VEH_SPEED,
    E_NIGHT_REST, E_DAY_REST, E_NIGHT_MOOD, E_DAY_MOOD, E_COLD_TOL, E_HEAT_TOL,
    E_CROWD, E_ALONE, E_FEAR_WATER, E_FEAR_DARK, E_FEAR_HEIGHT, E_INDOORS, E_OUTDOORS,
    E_PET_FOOD, E_PET_JOY, E_GOTCHI_XP, E_GOTCHI_GIFT,
    E_ARC_SCORE, E_ARC_TIX,
    E_FIGHT_DMG, E_FIGHT_DEF,
    E_LUCK, E_LOOT, E_SPARK,
    E_AGE_SPEED, E_ASP_GAIN, E_ASP_KEEP,
};
// counters the DS raises (40 of 55)
static const u8 TRF_EVSUP[] = {
    EV_COOKED, EV_ATEOUT, EV_SNACKS, EV_SLEPT, EV_SHOWERS, EV_FUNS, EV_FILMS, EV_BOOKS, EV_COFFEES, EV_RUNS, EV_SWIMS, EV_CALLS, EV_GATHERS,
    EV_CHATS, EV_CARES, EV_JOKES, EV_HUGS, EV_FLIRTS, EV_KIND, EV_PAYDAYS, EV_PROMOS, EV_LUCKY, EV_DAYS, EV_EARNED, EV_CASH, EV_JOBLV, EV_FRIENDS, EV_FAMILY,
    EV_DISTRICTS, EV_FIGHTS, EV_CHAMP, EV_ARCPLAYS, EV_TIX, EV_HOME, EV_VEHICLES, EV_PARTNER, EV_HATCHED, EV_GELDER, EV_PET, EV_SK_FARM,
};
#define TRF_NSUP ((int)sizeof TRF_SUP)
#define TRF_NEVSUP ((int)sizeof TRF_EVSUP)

static u8 trf_hok[E_N], trf_evok[EV_N], trf_off[(TR_MAXID + 8) / 8], trf_woff[(WISH_N + 8) / 8];
static u16 trf_catn[TR_NCAT];   // offered traits by category (EARNED: the won ones, earned and prize)
static int trf_ready;
static inline int trf_on(int id) { return id > 0 && id <= TR_MAXID && (trf_off[id >> 3] >> (id & 7) & 1); }
static inline int trf_won(int w) { return w > 0 && w <= WISH_N && (trf_woff[w >> 3] >> (w & 7) & 1); }
static const u8 *trf_effp(int id) {   // an id's effects: hook x3, tier x3 (traits_auto.c's eff_of)
    if (id >= TR_FIRST_EARN && id < TR_FIRST_EARN + TR_NEARN) return tr_small[id - TR_FIRST_EARN];
    if (id >= TR_FIRST_RWD && id < TR_FIRST_RWD + TR_NRWD) return tr_small[TR_NEARN + id - TR_FIRST_RWD];
    return tr_eff[id];
}
static int trf_all_ok(const u8 *e) { for (int k = 0; k < 3; k++) if (e[k] && !trf_hok[e[k]]) return 0; return e[0] != 0; }
static void trf_setoff(int id) { trf_off[id >> 3] |= (u8)(1 << (id & 7)); }
static void trf_init(void) {
    if (trf_ready) return; trf_ready = 1;
    memset(trf_hok, 0, sizeof trf_hok); memset(trf_evok, 0, sizeof trf_evok); memset(trf_off, 0, sizeof trf_off); memset(trf_woff, 0, sizeof trf_woff); memset(trf_catn, 0, sizeof trf_catn);
    for (int i = 0; i < TRF_NSUP; i++) trf_hok[TRF_SUP[i]] = 1;
    for (int i = 0; i < TRF_NEVSUP; i++) trf_evok[TRF_EVSUP[i]] = 1;
    for (int w = 1; w <= WISH_N; w++) if (trf_evok[tr_wish[w].ev] && (tr_wish[w].rk != RK_TRAIT || trf_all_ok(tr_small[TR_NEARN + tr_wish[w].ra - TR_FIRST_RWD]))) trf_woff[w >> 3] |= (u8)(1 << (w & 7));
    for (int id = 1; id <= TR_NREG; id++) if (trf_all_ok(tr_eff[id])) { trf_setoff(id); trf_catn[tr_cat[id] & 15]++; }
    for (int i = 0; i < TR_NEARN; i++) { int id = TR_FIRST_EARN + i; if (tr_earn[i].ev && trf_evok[tr_earn[i].ev] && trf_all_ok(tr_small[i])) { trf_setoff(id); trf_catn[15]++; } }
    for (int i = 0; i < TR_NRWD; i++) { int id = TR_FIRST_RWD + i, fromw = 0;
        for (int w = 1; w <= WISH_N; w++) if (trf_won(w) && tr_wish[w].rk == RK_TRAIT && tr_wish[w].ra == id) fromw = 1;
        if (fromw && trf_all_ok(tr_small[TR_NEARN + i])) { trf_setoff(id); trf_catn[15]++; } }
    for (int id = TR_FIRST_CHEAT; id <= TR_MAXID; id++) if (trf_all_ok(tr_eff[id])) trf_setoff(id);
}
// A wish prize that is a trait the DS does not offer is dropped (the wish is not offered either: see trf_init)

// ---- the PS1 life stage (AG_BABY..AG_ELDER = 0..4) from the DS's six (baby, toddler, child, teen, adult, elder) ----
static inline int trf_stage(int ds_stage) { static const u8 M[6] = { 0, 1, 1, 2, 3, 4 }; return M[ds_stage < 0 ? 0 : ds_stage > 5 ? 5 : ds_stage]; }
static inline int trf_nslots(int ds_stage) { return tr_nslots(trf_stage(ds_stage)); }

// ---- the effects of a state: only offered traits count (everything else is dropped from the slots) ----
static void trf_rebuild(TrState *t) {
    trf_init(); memset(t->fx, 0, sizeof t->fx); memset(t->ch, 0, sizeof t->ch); memset(t->cm, 0, sizeof t->cm); t->f &= (u8)~(TF_CHEAT | TF_CHON);
    for (int k = 0; k < TR_SLOTS; k++) { int id = t->tid[k]; if (!id) continue; if (!trf_on(id)) { t->tid[k] = 0; continue; } const u8 *e = trf_effp(id); tr_apply_id(t, id, e, (const s8 *)(e + 3)); }
}
static int trf_clash(int a, int b) { return tr_clash(a, b); }   // (the PS1's rule: same main hook, or one raises what the other lowers; every effect of an offered trait is a real DS hook)

// ---- a person's traits from their personality: traits_auto.c's tr_auto_pick, over the DS pool ----
static u32 trf_mix(u32 a, u32 b) { u32 h = a * 2654435761u ^ (b + 0x9e3779b9u) * 40503u; h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; return h; }
static s16 trf_sc[TR_FIRST_RWD + TR_NRWD];
static int trf_auto_pick(const TrPers *p, const u16 *inherit, int ninh, u16 *tid, int nslots) {
    int cs[15], ncat[15], i, k, added = 0, ninhn = 0; trf_init();
    if (nslots > TR_SLOTS) nslots = TR_SLOTS;
    for (int c = 0; c < 15; c++) { int v = 0; for (int b = 0; b < 5; b++) v += tr_pk[c][b] * ((int)p->bar[b] - 5); v += 4 * tr_pasp[p->asp % 6][c] + 3 * tr_psign[(p->sign % 12) & 3][c]; cs[c] = v; ncat[c] = 0; }
    for (k = 0; k < TR_SLOTS; k++) if (tid[k] && tid[k] < TR_FIRST_CHEAT) ncat[tr_cat_of(tid[k])]++;
    for (i = 1; i < TR_FIRST_RWD + TR_NRWD; i++) {
        trf_sc[i] = -9999;
        if (!trf_on(i)) continue;
        int c = tr_cat_of(i), v;
        if (i < TR_FIRST_EARN) v = 4 * cs[c]; else { v = 2 * cs[c]; if (v < 60) continue; }
        v += (int)(trf_mix(p->seed, (u32)i) % 24u);
        for (k = 0; k < ninh; k++) if (inherit[k] == i) v += 40;
        trf_sc[i] = (s16)v;
    }
    for (int slot = 0; slot < nslots; slot++) {
        if (tid[slot]) continue;
        int best = 0, bv = -9000;
        for (i = 1; i < TR_FIRST_RWD + TR_NRWD; i++) {
            if (trf_sc[i] <= bv || trf_sc[i] <= -9000) continue;
            int c = tr_cat_of(i), ok = ncat[c] < 3, isinh = 0;
            for (k = 0; k < ninh; k++) isinh |= inherit[k] == i;
            if (isinh && ninhn >= 2) ok = 0;
            for (k = 0; ok && k < TR_SLOTS; k++) if (tid[k] && (tid[k] == i || tr_clash(i, tid[k]))) ok = 0;
            if (ok) { best = i; bv = trf_sc[i]; }
        }
        if (!best) break;
        { int isinh = 0; for (k = 0; k < ninh; k++) isinh |= inherit[k] == best; ninhn += isinh; }
        tid[slot] = (u16)best; ncat[tr_cat_of(best)]++; added++;
    }
    return added;
}
static const char *const TRG_SIGN[12] = { "Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo", "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces" };
static const char *const TRG_BAR[5] = { "Neat", "Outgoing", "Active", "Playful", "Nice" };
static int trf_auto_wish(const TrPers *p, const TrState *t) {   // (D74) the wish this person's personality pulls to most, of those the DS offers and a Sim can count themselves
    int cs[15], best = 0, bv = -99999; trf_init();
    for (int c = 0; c < 15; c++) { int v = 0; for (int b = 0; b < 5; b++) v += tr_pk[c][b] * ((int)p->bar[b] - 5); cs[c] = v + 4 * tr_pasp[p->asp % 6][c] + 3 * tr_psign[(p->sign % 12) & 3][c]; }
    for (int w = 1; w <= WISH_N; w++) { int ev = tr_wish[w].ev; if (!trf_won(w) || !tr_ev_mem(ev) || !trf_evok[ev] || ((t->wd[w >> 3] >> (w & 7)) & 1)) continue;
        if (tr_ev_abs[ev] && t->ev[ev] >= tr_wish[w].thr) continue; int v = 4 * cs[tr_wish_cat[w] % 15] + (int)(trf_mix(p->seed, 1000u + (u32)w) % 24u); if (v > bv) { bv = v; best = w; } }
    return best;
}
// (D76) a person's personality from their own seed (persona_town_seed(world seed, unique id)): the same call the PS1 makes for a crowd townie (citizens.c cit_persona)
static void trf_pers_from_seed(u32 sd, int ds_stage, TrPers *p) {
    Persona q; int st = trf_stage(ds_stage); persona_of(sd, st, &q);
    for (int b = 0; b < 5; b++) p->bar[b] = q.bar[b];
    p->sign = q.sign; p->asp = q.asp; p->stage = (u8)st; p->seed = sd;
}
// the DS's Sims made before D76 (and Create-a-Sim ones) have no personality seed: theirs comes from the name: they come from the name (deterministic, like the star sign), and a child's from its parents' (see trg_pers in main.c)
static void trf_pers_from_name(const char *first, const char *last, int ds_stage, TrPers *p) {
    u32 sd = tr_name_seed(first, last);
    for (int b = 0; b < 5; b++) p->bar[b] = (u8)(trf_mix(sd, 100u + (u32)b) % 11u);
    p->sign = (u8)(trf_mix(sd, 200u) % 12u); p->asp = (u8)(trf_mix(sd, 300u) % 6u); p->stage = (u8)trf_stage(ds_stage); p->seed = sd;
}

// ---- the hooks: every one is one small function, read where the DS game has its number ----
static inline int trf_p(const TrState *t, int e) { int p = t->fx[e] * tr_unit[e]; return p < -100 ? -100 : p; }
static inline int trf_sc_round(int v, int p) { if (p < -100) p = -100; int x = v * (100 + p); return x >= 0 ? (x + 50) / 100 : -((-x + 50) / 100); }   // v scaled by p percent, to the nearest
static inline int trf_scale(const TrState *t, int e, int v) { return t->fx[e] ? trf_sc_round(v, trf_p(t, e)) : v; }
static int trf_rate(TrState *t, int slot, int p) { if (p < -100) p = -100; int x = t->skr[slot] + 100 + p; t->skr[slot] = (u8)(x % 100); return x / 100; }   // a steady rate of 1 a tick, changed by p percent, the rest carried
static int trf_scale_acc(TrState *t, int slot, int e, int v) { int p = trf_p(t, e); if (!p || v <= 0) return v; int x = v * (100 + p) + t->skr[slot]; t->skr[slot] = (u8)(x % 100); return x / 100; }   // a small amount scaled, the rest carried

// NEEDS (sims_step): the points a need falls at this tick of its clock. E_DEC_*, and at night / by day the energy: E_NIGHT_REST / E_DAY_REST
static int trf_fall(TrState *t, int n, int night) {
    int p = trf_p(t, TRF_DEC[n]); if (n == 2) p += trf_p(t, night ? E_NIGHT_REST : E_DAY_REST);
    if (!p) return 1; if (p < -100) p = -100; int x = t->nacc[n] + 100 + p; t->nacc[n] = (u8)(x % 100); return x / 100;
}
// what an action gives a need (start_act / start_soc): E_GAIN_* of the need and E_ACT_* of the kind of action (ah: 0 none)
static int trf_gain(const TrState *t, int n, int ah, int amt) { int p = trf_p(t, TRF_GAIN[n]) + (ah ? trf_p(t, ah) : 0); return p ? trf_sc_round(amt, p) : amt; }
// MOVING: -1 skip this step, 0 as always, +1 one more step (E_WALK_SPEED on foot, E_VEH_SPEED on the skateboard)
static int trf_stepmod(TrState *t, int slot, int e) { int p = trf_p(t, e); if (!p) return 0; int a = t->skr[slot] + (p < 0 ? -p : p); t->skr[slot] = (u8)(a % 100); return a >= 100 ? (p > 0 ? 1 : -1) : 0; }
static inline int trf_walk(TrState *t) { return trf_stepmod(t, 0, E_WALK_SPEED); }
static inline int trf_veh(TrState *t) { return trf_stepmod(t, 1, E_VEH_SPEED); }
// SOCIAL: the friendship a talk changes (d, a whole point), E_REL_GAIN / E_REL_LOSS and, for a love act, E_ROMANCE
static int trf_rel(TrState *t, int d, int romantic) {
    if (!d) return 0; int p = d > 0 ? trf_p(t, E_REL_GAIN) + (romantic ? trf_p(t, E_ROMANCE) : 0) : trf_p(t, E_REL_LOSS); if (p < -100) p = -100;
    int ad = d < 0 ? -d : d, slot = d > 0 ? 2 : 3, x = ad * (100 + p) + t->skr[slot]; t->skr[slot] = (u8)(x % 100); int r = x / 100; return d < 0 ? -r : r;
}
// does an act work? the hook (E_CHAT_OK ...), its usual chance, charisma (E_CHARM) 3 a level. rnd: any random number
static int trf_chance(const TrState *t, int e, int base) { int c = tr_chance(t, e, base) + tr_add(t, E_CHARM) * 3; return c < 0 ? 0 : c > 100 ? 100 : c; }
static inline int trf_works(const TrState *t, int e, int base, unsigned rnd) { return (int)(rnd % 100u) < trf_chance(t, e, base); }
// MOOD (mood()): points from the trait, by the time, the place, the company. Context in a small struct the game fills
typedef struct { u8 night, out, up, partner, pet, swim, near, season; } TrCtx;   // near: others in reach; season 0..3 (3 winter, 1 summer)
static int trf_mood_pts(const TrState *t, const TrCtx *c) {
    int m = tr_add(t, E_MOOD_BASE) + tr_add(t, c->night ? E_NIGHT_MOOD : E_DAY_MOOD) + tr_add(t, c->out ? E_OUTDOORS : E_INDOORS);
    if (c->up) m += tr_add(t, E_FEAR_HEIGHT); if (c->partner) m += tr_add(t, E_LOVE_MOOD); if (c->pet) m += tr_add(t, E_PET_JOY);
    if (c->out) { if (c->season == 3) m += tr_add(t, E_COLD_TOL); else if (c->season == 1) m += tr_add(t, E_HEAT_TOL); if (c->night) m += tr_add(t, E_FEAR_DARK); }
    if (c->swim) m += tr_add(t, E_FEAR_WATER);
    if (c->near >= 2) m += tr_add(t, E_CROWD); else if (!c->near) m += tr_add(t, E_ALONE);
    return m / 2;   // (the PS1's mood runs -100..100, the DS's 0..100)
}
static inline int trf_mood_floor(const TrState *t) { return (tr_floor(t) + 100) / 2; }   // E_MOOD_MIN: the lowest the mood may go (0 when none)
// WORK: pay (E_JOB_PAY and the career's own), the performance (E_JOB_PERF), promotion progress (E_JOB_PROMO), strikes (E_JOB_STRIKE)
static int trf_pay(const TrState *t, int job0, int base) { int p = trf_p(t, E_JOB_PAY) + trf_p(t, TRF_PAY[job0 % 6]); return p ? trf_sc_round(base, p) : base; }
static inline int trf_perf(const TrState *t, int perf) { return trf_scale(t, E_JOB_PERF, perf); }
static inline int trf_promo(TrState *t) { return trf_rate(t, 4, trf_p(t, E_JOB_PROMO)); }
static inline int trf_strike(TrState *t) { return trf_rate(t, 5, trf_p(t, E_JOB_STRIKE)); }
// MONEY: what a change in funds is after E_INCOME (more earned) or E_SPEND (more spent); a shop price after E_SHOP_DISC
static long trf_cash(const TrState *t, long d) { int p = trf_p(t, d > 0 ? E_INCOME : E_SPEND); return p ? (long)trf_sc_round((int)d, p) : d; }
static inline int trf_shop(const TrState *t, int price) { int v = trf_scale(t, E_SHOP_DISC, price); return v < 1 ? 1 : v; }
static inline int trf_farm_pay(const TrState *t, int v) { return trf_scale(t, E_PAY_FARM, v); }
static inline int trf_farm_sk(TrState *t, int v) { return trf_scale_acc(t, 6, E_SK_FARM, v); }
// PETS, GAMES: the dog's hunger tick (E_PET_FOOD), Boreagotchi care xp and gifts, the arcade's score and tickets
static inline int trf_dog_fall(TrState *t) { return trf_rate(t, 7, trf_p(t, E_PET_FOOD)); }
static inline int trf_gxp(TrState *t, int n) { return trf_scale_acc(t, 8, E_GOTCHI_XP, n); }
static inline int trf_gift(const TrState *t, int v) { return trf_scale(t, E_GOTCHI_GIFT, v); }
static inline int trf_arc_score(const TrState *t, int v) { return trf_scale(t, E_ARC_SCORE, v); }
static inline int trf_arc_tix(const TrState *t, int v) { return trf_scale(t, E_ARC_TIX, v); }
static inline int trf_fight_pw(const TrState *t, int pw) { return trf_scale(t, E_FIGHT_DMG, pw); }
static inline int trf_fight_hurt(const TrState *t, int dmg) { return trf_scale(t, E_FIGHT_DEF, dmg); }
// LUCK: a chance in percent (E_LUCK), found money (E_LOOT), inspiration (E_SPARK)
static inline int trf_luck_pct(const TrState *t, int base) { return tr_chance(t, E_LUCK, base); }
static inline int trf_loot(const TrState *t, int v) { return trf_scale(t, E_LOOT, v); }
static inline int trf_spark_pct(const TrState *t, int base) { return tr_chance(t, E_SPARK, base); }
// LIFE: growing up a day (E_AGE_SPEED: the years a day adds), wants (E_ASP_GAIN) and the aspiration kept (E_ASP_KEEP)
static int trf_age(TrState *t) { int p = trf_p(t, E_AGE_SPEED); if (!p) return 1; int x = t->ageacc + 100 + p; t->ageacc = (u8)(x % 100); return x / 100; }
static inline int trf_asp_gain(const TrState *t, int pts) { return trf_scale(t, E_ASP_GAIN, pts); }
static inline int trf_asp_loss(const TrState *t, int pts) { return trf_scale(t, E_ASP_KEEP, pts); }

// ---- the DS item's kind of action, from its label (what the PS1's actHook gives per action): the E_ACT_* hook, 0 none ----
static int trf_act_hook(const char *lab, int need) {
    static const char *const SIT[] = { "Sit", "Lounge", "Rock", "Relax", "Sunbathe", "Warm Up", 0 }, *const RUN[] = { "A Run", "Lift Weights", "Punch the Bag", 0 },
        *const SHW[] = { "Shower", "Take a Bath", "Soak", "Wash", "Wash Hands", 0 }, *const SLP[] = { "Sleep", "Nap", 0 };
    #define IN(L) ({ int f_ = 0; for (int q_ = 0; (L)[q_]; q_++) if (!strcmp(lab, (L)[q_])) f_ = 1; f_; })
    if (need == 0) return E_ACT_EAT;
    if (need == 2) { if (!strcmp(lab, "A Cup")) return E_ACT_COFFEE; if (IN(SLP)) return E_ACT_SLEEP; if (IN(SIT)) return E_ACT_SIT; return 0; }
    if (need == 3) return !strcmp(lab, "Call") ? E_ACT_PHONE : E_ACT_TALK;
    if (need == 5) return IN(SHW) ? E_ACT_SHOWER : 0;
    if (need == 1) { if (!strcmp(lab, "Read")) return E_ACT_READ; if (IN(RUN)) return E_ACT_RUN; if (!strcmp(lab, "Swim")) return E_ACT_SWIM; return E_ACT_FUN; }
    #undef IN
    return 0;
}
#endif
