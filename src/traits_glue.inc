// traits_glue.inc: included by sim.c just before tick_needs. The game's side of the trait engine (docs/TRAITS.md): the hooks that need sim.c's own
// state (need falls, mood conditions), the watchers that turn skill gains, cash and the game's own numbers into counters, and the wish rewards.
#include "arcade.h"
#include "citizens.h"
#include "gotchi.h"
#include "home.h"
#include <stddef.h>

void tr_toast(const char *s) { sim_note(s); }
static int char_lvl(const Sim *s) { return CLAMP(SKL(s->skill_char) + tr_add(&s->tr, E_CHARM), 0, 20); }   // E_CHARM: the charisma it acts with
static int luck_roll(Sim *s, int base) { int ok = (int)(look_rand() % 100) < tr_chance(&s->tr, E_LUCK, base); if (ok && base < 50) tr_ev(&s->tr, EV_LUCKY, 1); return ok; }   // E_LUCK
void tr_reward(TrState *t, int kind, int arg, int wish) {   // a lifetime wish is done: its reward changes something real
    Sim *s = (Sim *)((char *)t - offsetof(Sim, tr));
    char b[40];
    switch (kind) {
        case RK_CASH: cash += arg; snprintf(b, sizeof b, "LIFETIME WISH DONE! +$%d", arg); break;
        case RK_TRAIT: tr_give(t, arg); snprintf(b, sizeof b, "LIFETIME WISH DONE! NEW TRAIT"); break;
        case RK_OUTFIT: look_theme(&s->bore, arg); look_build(&s->bore); snprintf(b, sizeof b, "LIFETIME WISH DONE! NEW OUTFIT"); break;
        case RK_FOOD: groceries += arg; snprintf(b, sizeof b, "LIFETIME WISH DONE! +%d MEALS", arg); break;
        case RK_BUS: bus_pass = MAX(bus_pass, day_count) + arg; snprintf(b, sizeof b, "LIFETIME WISH DONE! BUS PASS +%d DAYS", arg); break;
        case RK_TOKENS: arc.tok = (u16)MIN(9999, arc.tok + arg); snprintf(b, sizeof b, "LIFETIME WISH DONE! +%d TOKENS", arg); break;
        default: s->asp_pts = MIN(ASP_MAX, s->asp_pts + arg); snprintf(b, sizeof b, "LIFETIME WISH DONE! WANTS +%d", arg); break;
    }
    (void)wish; sim_note(b); sim_moodlet(s, ML_PROUD); audio_voice(VOICE_WIN);
}
static int need_fall(Sim *s, int n, int r, int np) {   // E_DEC_*: the fall of need n this step. r: the plain rate, np: the NEEDS option. In 16ths with the rest carried, so 3 a step can still change by 10%
    int p = tr_pct(&s->tr, E_DEC0 + n);
    if (n == N_REST) p += tr_pct(&s->tr, is_night() ? E_NIGHT_REST : E_DAY_REST);
    if (!p) return r * np / 100;
    if (p < -100) p = -100;
    int r16 = r * np * 16 / 100 * (100 + p) / 100 + s->tr.nacc[n];
    s->tr.nacc[n] = (u8)(r16 & 15); return r16 >> 4;
}
static int trait_mood(const Sim *s) {   // the mood points from traits: flat, by the time, the place, the weather, the company
    const TrState *t = &s->tr; int m = tr_add(t, E_MOOD_BASE), night = is_night(), out = sim_outdoors(s);
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
static const u8 actHook[ACT_GATHER + 1] = { 0, E_ACT_EAT, 0, E_ACT_SLEEP, E_ACT_SHOWER, E_ACT_SIT, E_ACT_FUN, E_ACT_TALK, 0, 0, E_ACT_COFFEE, E_ACT_READ, E_ACT_RUN, E_ACT_PHONE, 0, 0, E_ACT_SWIM, 0, 0 };
static void need_gains(Sim *s, const int *snap) {   // E_GAIN_*, E_ACT_*: what an action put into a need this step, scaled
    int ah = actHook[s->action];
    for (int i = 0; i < 7; i++) { int d = s->need[i] - snap[i]; if (d <= 0) continue;
        int p = tr_pct(&s->tr, E_GAIN0 + i) + (ah ? tr_pct(&s->tr, ah) : 0); if (!p) continue; if (p < -100) p = -100;
        s->need[i] = snap[i] + d * (100 + p) / 100; }
}
static void tr_watch(Sim *s, int si) {   // every 8th step: skill gains scaled (E_SK_*), the skill levels and the game's own numbers counted for the wishes
    TrState *t = &s->tr; int raw[9] = { s->skill_cook, s->skill_body, s->skill_fun, s->skill_logic, s->skill_char, s->skill_creat, s->skill_mech, s->skill_farm, 0 };
    { int b = 0; for (int k = 0; k < BS_N; k++) b += s->skill_band[k]; raw[8] = b; }
    int *ptr[9] = { &s->skill_cook, &s->skill_body, &s->skill_fun, &s->skill_logic, &s->skill_char, &s->skill_creat, &s->skill_mech, &s->skill_farm, &s->skill_band[0] };
    for (int k = 0; k < 9; k++) {
        int d = raw[k] - (int)t->skp[k], p = tr_pct(t, E_SK0 + k);
        if (d > 0 && d < 400 && p) { if (p < -100) p = -100; int tot = d * p + (int)t->skr[k] * (p < 0 ? -1 : 1); int x = tot / 100; t->skr[k] = (u8)ABS(tot % 100); if (x) { *ptr[k] += x; if (k < 8 && *ptr[k] < 0) *ptr[k] = 0; raw[k] += x; } }
        t->skp[k] = (u16)MIN(raw[k], 65535);
    }
    for (int k = 0; k < 8; k++) tr_ev_set(t, EV_SK_COOK + k, SKL(raw[k]));
    { int b = 0; for (int k = 0; k < BS_N; k++) b = MAX(b, SKL(s->skill_band[k])); tr_ev_set(t, EV_BANDLV, b); }
    tr_ev_set(t, EV_DAYS, day_count / 5);
    if (si != player) return;
    int fr = 0; for (int j = 0; j < nsims; j++) if (j != si && s->rel[j] >= 50) fr++;
    tr_ev_set(t, EV_JOBLV, job_level); tr_ev_set(t, EV_PAYDAYS, story_paid); tr_ev_set(t, EV_FIGHTS, fight_wins); tr_ev_set(t, EV_CHAMP, fight_champ);
    tr_ev_set(t, EV_ARCPLAYS, arc.plays); tr_ev_set(t, EV_TIX, arcade_tix / 10); tr_ev_set(t, EV_PARTIES, pk_ext[PKX_PARTY]); tr_ev_set(t, EV_HOME, home_city + nprops);
    tr_ev_set(t, EV_COLLEGE, col_lvl); tr_ev_set(t, EV_SPACE, space_done); tr_ev_set(t, EV_VEHICLES, __builtin_popcount(veh_own)); tr_ev_set(t, EV_PET, pet_kind ? 1 : 0);
    tr_ev_set(t, EV_FRIENDS, fr + citizens_friends()); tr_ev_set(t, EV_FAMILY, nsims); tr_ev_set(t, EV_PARTNER, s->partner ? 1 : 0);
    { int z = 0; for (int k = 0; k < 4; k++) z += story_visit[k] ? 1 : 0; tr_ev_set(t, EV_DISTRICTS, z); }
    tr_ev_set(t, EV_EARNED, (int)MIN(tr_earned / 500, 255u)); tr_ev_set(t, EV_CASH, (int)CLAMP(cash / 1000, 0, 255));
    { Toy *g = gotchi_toy(si); if (g) { int el = 0; for (int k = 0; k < GB_N; k++) if (g->b[k].on && g->b[k].stage == GS_ELDER) el = 1; tr_ev_set(t, EV_HATCHED, g->hatched); tr_ev_set(t, EV_GELDER, el); } }
}
static long tr_cash_prev = -1;
static void tr_cash_watch(void) {   // E_INCOME, E_SPEND: whatever changed the money since the last look, more or less of it
    if (tr_cash_prev == -1 || !nsims) { tr_cash_prev = cash; return; }
    long d = cash - tr_cash_prev; if (!d) return;
    const TrState *t = &sims[player].tr; int p;
    if (d > 0) { tr_earned = MIN(tr_earned + (u32)d, 4000000000u); p = tr_pct(t, E_INCOME); }
    else p = tr_pct(t, E_SPEND);
    if (p) { if (p < -100) p = -100; cash += d * p / 100; if (cash < 0) cash = 0; }
    tr_cash_prev = cash;
}
static int tr_ev_player(int ev) { return nsims ? tr_ev(&sims[player].tr, ev, 1) : 0; }
void tr_new_life(void) { tr_cash_prev = -1; memset(tr_wdone, 0, sizeof tr_wdone); tr_earned = 0; }
void sim_cheat_opt(int on) { tr_cheat_opt = (u8)on; for (int i = 0; i < nsims; i++) tr_cheat_refresh(&sims[i].tr, on); }
