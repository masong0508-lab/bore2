// traits_glue.inc: included by sim.c just before tick_needs. The game's side of the trait engine (docs/TRAITS.md): the hooks that need sim.c's own
// state (need falls, mood conditions), the watchers that turn skill gains, cash and the game's own numbers into counters, and the wish rewards.
#include "arcade.h"
#include "citizens.h"
#include "gotchi.h"
#include "home.h"
#include <stddef.h>

void tr_toast(const char *s) { sim_note(s); }
static int char_lvl(const Sim *s) { return CLAMP(SKL(s->skill_char) + tr_add(s->tr, E_CHARM), 0, 20); }   // E_CHARM: the charisma it acts with
static int luck_roll(Sim *s, int base) { int ok = (int)(look_rand() % 100) < tr_chance(s->tr, E_LUCK, base); if (ok && base < 50) tr_ev(s->tr, EV_LUCKY, 1); return ok; }   // E_LUCK
void tr_reward(TrState *t, int kind, int arg, int wish) {   // a lifetime wish is done: its reward changes something real
    Sim *s = &sims[player]; for (int i = 0; i < nsims; i++) if (sims[i].tr == t) s = &sims[i];
    int full = 0; char b[40]; static const char *const what[] = { "+$%d", "NEW TRAIT", "NEW OUTFIT", "+%d MEALS", "BUS PASS +%d DAYS", "+%d TOKENS", "WANTS +%d" };
    switch (kind) {
        case RK_CASH: cash += arg; break;
        case RK_TRAIT: if (tr_give(t, arg) < 0) full = 1; break;
        case RK_OUTFIT: look_theme(&s->bore, arg); look_build(&s->bore); break;
        case RK_FOOD: groceries += arg; break;
        case RK_BUS: bus_pass = MAX(bus_pass, day_count) + arg; break;
        case RK_TOKENS: arc.tok = (u16)MIN(9999, arc.tok + arg); break;
        default: s->asp_pts = MIN(ASP_MAX, s->asp_pts + arg); break;
    }
    { char f[24]; snprintf(f, sizeof f, "WISH DONE! %s", full ? "TRAIT WAITS FOR A SLOT" : what[CLAMP(kind - 1, 0, 6)]); snprintf(b, sizeof b, f, arg); }
    (void)wish; sim_note(b); sim_moodlet(s, ML_PROUD); audio_voice(VOICE_WIN);
}
static int need_fall(Sim *s, int n, int r, int np) {   // E_DEC_*: the fall of need n this step. r: the plain rate, np: the NEEDS option. In 16ths with the rest carried, so 3 a step can still change by 10%
    int p = tr_pct(s->tr, E_DEC0 + n);
    if (n == N_REST) p += tr_pct(s->tr, is_night() ? E_NIGHT_REST : E_DAY_REST);
    if (!p) return r * np / 100;
    if (p < -100) p = -100;
    int r16 = r * np * 16 / 100 * (100 + p) / 100 + s->tr->nacc[n];
    s->tr->nacc[n] = (u8)(r16 & 15); return r16 >> 4;
}
static __attribute__((noinline)) int trait_mood(const Sim *s) {   // the mood points from traits: flat, by the time, the place, the weather, the company
    const TrState *t = s->tr; int m = tr_add(t, E_MOOD_BASE), night = is_night(), out = sim_outdoors(s);
    m += tr_add(t, night ? E_NIGHT_MOOD : E_DAY_MOOD);
    m += tr_add(t, out ? E_OUTDOORS : E_INDOORS);
    if (s->lvl > 0) m += tr_add(t, E_FEAR_HEIGHT);
    if (s->partner) m += tr_add(t, E_LOVE_MOOD);
    if (pet_kind) m += tr_add(t, E_PET_JOY);
    if (out) { if (sim_weather()) m += tr_add(t, E_RAIN_TOL); int se = sim_season(); if (se == 3) m += tr_add(t, E_COLD_TOL); else if (se == 1) m += tr_add(t, E_HEAT_TOL); if (night) m += tr_add(t, E_FEAR_DARK); }
    if (s->action == ACT_SWIM) m += tr_add(t, E_FEAR_WATER);
    { int near = 0; for (int j = 0; j < nsims; j++) { const Sim *o = &sims[j]; if (o == s || o->away || o->lvl != s->lvl) continue; int dx = (o->x - s->x) >> 6, dz = (o->z - s->z) >> 6; if (dx * dx + dz * dz < 900) near++; }
      if (near >= 2) m += tr_add(t, E_CROWD); else if (!near) m += tr_add(t, E_ALONE); }
    return m;
}
u8 tr_pend_flag; u16 tr_pend_cool;   // a member needs traits assigned (CREATE.OVL, open(3)): main.c runs it when it is quiet
static void tr_pend_set(void) { if (!tr_pend_flag) { tr_pend_flag = 1; if (tr_pend_cool < 24) tr_pend_cool = 24; } }   // (a short wait, so every member scanned in the same 8 steps is assigned in one load)
static void tr_scan(Sim *s) {   // every 8th step, each Bore: a new member gets a state and is marked for assigning, a new life stage re-assigns, a name missing is filled
    if (s->tr == &tr_null) { TrState *t = tr_alloc(); if (t == &tr_null) return; s->tr = t; tr_stage_set(t, s->bore.stage); t->f |= TF_AUTO | TF_PEND; tr_pend_set(); return; }
    TrState *t = s->tr; int st = s->bore.stage;
    if (tr_stage_get(t) != st) { tr_stage_set(t, st); if (t->f & TF_AUTO) t->f |= TF_PEND; }
    if (!(t->f & TF_PEND)) for (int k = 0; k < TR_SLOTS; k++) if (t->tid[k] && (t->tid[k] < TR_FIRST_EARN || t->tid[k] >= TR_FIRST_CHEAT) && !tr_names_of(t)[k][0]) { t->f |= TF_PEND; break; }
    if (t->f & TF_PEND) tr_pend_set();
}
static const u8 actHook[ACT_GATHER + 1] = { 0, E_ACT_EAT, 0, E_ACT_SLEEP, E_ACT_SHOWER, E_ACT_SIT, E_ACT_FUN, E_ACT_TALK, 0, 0, E_ACT_COFFEE, E_ACT_READ, E_ACT_RUN, E_ACT_PHONE, 0, 0, E_ACT_SWIM, 0, 0 };
static void need_gains(Sim *s, const int *snap) {   // E_GAIN_*, E_ACT_*: what an action put into a need this step, scaled
    int ah = actHook[s->action];
    for (int i = 0; i < 7; i++) { int d = s->need[i] - snap[i]; if (d <= 0) continue;
        int p = tr_pct(s->tr, E_GAIN0 + i) + (ah ? tr_pct(s->tr, ah) : 0); if (!p) continue; if (p < -100) p = -100;
        s->need[i] = snap[i] + d * (100 + p) / 100; }
}
static int skl_of(int raw) { return SKL(raw); }
static const u16 skOff[9] = { offsetof(Sim, skill_cook), offsetof(Sim, skill_body), offsetof(Sim, skill_fun), offsetof(Sim, skill_logic), offsetof(Sim, skill_char), offsetof(Sim, skill_creat), offsetof(Sim, skill_mech), offsetof(Sim, skill_farm), offsetof(Sim, skill_band) };
static __attribute__((noinline)) void tr_watch(Sim *s, int si) {   // every 8th step: skill gains scaled (E_SK_*), the skill levels and the game's own numbers counted for the wishes
    TrState *t = s->tr;
    for (int k = 0; k < 9; k++) {
        int *pp = (int *)((char *)s + skOff[k]), raw = *pp;
        if (k == 8) { raw = 0; for (int b = 0; b < BS_N; b++) raw += s->skill_band[b]; }
        int d = raw - (int)t->skp[k], p = tr_pct(t, E_SK0 + k);
        if (d > 0 && d < 400 && p) { if (p < -100) p = -100; int tot = d * p + (int)t->skr[k] * (p < 0 ? -1 : 1); int x = tot / 100; t->skr[k] = (u8)ABS(tot % 100); if (x) { *pp += x; if (k < 8 && *pp < 0) *pp = 0; raw += x; } }
        t->skp[k] = (u16)MIN(raw, 65535);
        if (k < 8) tr_ev_set(t, EV_SK_COOK + k, skl_of(raw));
    }
    { int b = 0; for (int k = 0; k < BS_N; k++) b = MAX(b, skl_of(s->skill_band[k])); tr_ev_set(t, EV_BANDLV, b); }
    tr_ev_set(t, EV_DAYS, day_count / 5);
    if (si != player) return;
    if (t != &tr_null && !(t->f & TF_AUTO)) {   // the extra slots: the household's, from the wishes done and the job level
        int wd = 0; for (int k = 0; k < 8; k++) wd += __builtin_popcount(tr_wdone[k]);
        int n = tr_unl_check(wd, job_level);
        if (n) { char b[32]; snprintf(b, sizeof b, "TRAIT SLOT %d UNLOCKED!", TR_BASE + tr_unl); sim_note(b); audio_voice(VOICE_WIN);
                 for (int j = 0; j < nsims; j++) if (sims[j].tr != &tr_null && (sims[j].tr->f & TF_AUTO)) { sims[j].tr->f |= TF_PEND; tr_pend_set(); } }
        if (n || !((walk_tick >> 3) & 63)) tr_regive(t);
    }
    int fr = 0; for (int j = 0; j < nsims; j++) if (j != si && s->rel[j] >= 50) fr++;
    int dist = 0; for (int k = 0; k < 4; k++) dist += story_visit[k] ? 1 : 0;
    int el = 0, hat = 0; { Toy *g = gotchi_toy(si); if (g) { for (int k = 0; k < GB_N; k++) if (g->b[k].on && g->b[k].stage == GS_ELDER) el = 1; hat = g->hatched; } }
    int v[EV_PET - EV_DAYS + 1] = { 0, (int)MIN(tr_earned / 500, 255u), (int)CLAMP(cash / 1000, 0, 255), job_level, fr + citizens_friends(), nsims, dist, fight_wins, fight_champ,
        arc.plays, arcade_tix / 10, pk_ext[PKX_PARTY], home_city + nprops, col_lvl, space_done, __builtin_popcount(veh_own), s->partner ? 1 : 0, hat, el, pet_kind ? 1 : 0 };
    if (t->f & TF_BASE) { for (int k = EV_EARNED; k <= EV_PET; k++) tr_ev_set(t, k, v[k - EV_DAYS]); }
    else { for (int k = EV_EARNED; k <= EV_PET; k++) tr_ev_quiet(t, k, v[k - EV_DAYS]); t->f |= TF_BASE; }   // (what a life starts with is not won)
    tr_ev_set(t, EV_PAYDAYS, story_paid);
}
static long tr_cash_prev = -1;
static void tr_cash_watch(void) {   // E_INCOME, E_SPEND: whatever changed the money since the last look, more or less of it
    if (tr_cash_prev == -1 || !nsims) { tr_cash_prev = cash; return; }
    long d = cash - tr_cash_prev; if (!d) return;
    const TrState *t = sims[player].tr; int p;
    if (d > 0) { tr_earned = MIN(tr_earned + (u32)d, 4000000000u); p = tr_pct(t, E_INCOME); }
    else p = tr_pct(t, E_SPEND);
    if (p) { if (p < -100) p = -100; cash += d * p / 100; if (cash < 0) cash = 0; }
    tr_cash_prev = cash;
}
static int tr_ev_player(int ev) { return nsims ? tr_ev(sims[player].tr, ev, 1) : 0; }
TrState *tr_alloc(void) {   // a pool state no Sim uses (a Bore that left frees its own)
    for (int k = 0; k < TR_NPOOL; k++) { TrState *p = &tr_pool[k]; int used = p == tr_stage_p;
        for (int i = 0; i < nsims; i++) used |= sims[i].tr == p;
        if (!used) { tr_clear(p); return p; } }
    return &tr_null;
}
void tr_new_life(void) { memset(tr_pool, 0, sizeof tr_pool); memset(tr_ev_a, 0, sizeof tr_ev_a); memset(tr_pnm, 0, sizeof tr_pnm); tr_unl = 0; memset(tr_wait, 0, sizeof tr_wait); tr_pend_flag = 0; tr_pend_cool = 0; tr_stage_p = 0; tr_staged = 0; tr_cash_prev = -1; memset(tr_wdone, 0, sizeof tr_wdone); tr_earned = 0; }
void sim_cheat_opt(int on) { tr_cheat_opt = (u8)on; for (int i = 0; i < nsims; i++) tr_cheat_refresh(sims[i].tr, on); }
void tr_take_stage(Sim *s) { if (!tr_staged) { if (s->tr == &tr_null) s->tr = tr_alloc(); } else { tr_staged = 0; if (tr_stage_p) { s->tr = tr_stage_p; tr_stage_p = 0; tr_cheat_refresh(s->tr, tr_cheat_opt); } } if (s->tr != &tr_null) tr_stage_set(s->tr, s->bore.stage); }
