// ds_traits_test.c: host test of the DS port's traits (ports/dsi/source/ds_trfx.h): the very file the game includes.
// For every trait the DS offers: its effects are put on a person and the DS accessor that reads each hook must give a different number than for a person with no traits.
// Build and run: bash ports/dsi/test/ds_traits_test.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TR_HOST
#include "../source/ds_trfx.h"
void tr_toast(const char *s) { (void)s; }
static int rew_n, rew_k;
void tr_reward(TrState *t, int kind, int arg, int wish) { (void)t; (void)arg; (void)wish; rew_n++; rew_k = kind; }

static int fails, checks;
#define OK(c, ...) do { checks++; if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

// one probe per hook: the DS accessor that reads it, run on a fresh state with the hook's tier, against the same on an empty state. Returns 1 when the DS number moved.
static int probe(int e, int tier) {
    TrState z, t; memset(&z, 0, sizeof z); memset(&t, 0, sizeof t); t.fx[e] = (s8)tier;
    TrCtx c0, c1; memset(&c0, 0, sizeof c0);
    for (int n = 0; n < 6; n++) {
        if (e == TRF_DEC[n]) { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_fall(&z, n, 0); b += trf_fall(&t, n, 0); } return a != b; }
        if (e == TRF_GAIN[n]) return trf_gain(&z, n, 0, 40) != trf_gain(&t, n, 0, 40);
    }
    if (e >= E_ACT_EAT && e <= E_ACT_COFFEE) { for (int n = 0; n < 6; n++) if (trf_gain(&z, n, e, 40) != trf_gain(&t, n, e, 40)) return 1; return 0; }
    if (e == E_NIGHT_REST || e == E_DAY_REST) { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_fall(&z, 2, e == E_NIGHT_REST); b += trf_fall(&t, 2, e == E_NIGHT_REST); } return a != b; }
    for (int j = 0; j < 6; j++) if (e == TRF_PAY[j]) return trf_pay(&z, j, 100) != trf_pay(&t, j, 100);
    switch (e) {
    case E_SK_FARM: { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_farm_sk(&z, 3); b += trf_farm_sk(&t, 3); } return a != b; }
    case E_MOOD_BASE: case E_NIGHT_MOOD: case E_DAY_MOOD: case E_INDOORS: case E_OUTDOORS: case E_FEAR_HEIGHT: case E_LOVE_MOOD: case E_PET_JOY: case E_COLD_TOL: case E_HEAT_TOL:
    case E_FEAR_DARK: case E_FEAR_WATER: case E_CROWD: case E_ALONE: {
        for (int m = 0; m < 1 << 8; m++) { c1.night = m & 1; c1.out = m >> 1 & 1; c1.up = m >> 2 & 1; c1.partner = m >> 3 & 1; c1.pet = m >> 4 & 1; c1.swim = m >> 5 & 1; c1.near = (m >> 6 & 1) * 2 + (m >> 7 & 1); c1.season = (m & 2) ? 3 : 1; if (c1.season == 3 && m & 4) c1.season = 1;
            if (trf_mood_pts(&z, &c1) != trf_mood_pts(&t, &c1)) return 1; if (m > 1 && 0) break; }
        for (int s = 1; s < 4; s += 2) for (int m = 0; m < 256; m++) { c1.night = m & 1; c1.out = m >> 1 & 1; c1.up = m >> 2 & 1; c1.partner = m >> 3 & 1; c1.pet = m >> 4 & 1; c1.swim = m >> 5 & 1; c1.near = (m >> 6 & 1) * 2 + (m >> 7 & 1); c1.season = s; if (trf_mood_pts(&z, &c1) != trf_mood_pts(&t, &c1)) return 1; }
        return 0; }
    case E_MOOD_MIN: return trf_mood_floor(&z) != trf_mood_floor(&t);
    case E_REL_GAIN: { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_rel(&z, 1, 0); b += trf_rel(&t, 1, 0); } return a != b; }
    case E_REL_LOSS: { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_rel(&z, -1, 0); b += trf_rel(&t, -1, 0); } return a != b; }
    case E_ROMANCE: { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_rel(&z, 1, 1); b += trf_rel(&t, 1, 1); } return a != b; }
    case E_CHAT_OK: case E_JOKE_OK: case E_FLIRT_OK: case E_HUG_OK: case E_ENGAGE: case E_CHARM: return trf_chance(&z, e == E_CHARM ? E_CHAT_OK : e, 70) != trf_chance(&t, e == E_CHARM ? E_CHAT_OK : e, 70);
    case E_JOB_PAY: return trf_pay(&z, 0, 100) != trf_pay(&t, 0, 100);
    case E_JOB_PERF: return trf_perf(&z, 100) != trf_perf(&t, 100);
    case E_JOB_PROMO: { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_promo(&z); b += trf_promo(&t); } return a != b; }
    case E_JOB_STRIKE: { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_strike(&z); b += trf_strike(&t); } return a != b; }
    case E_INCOME: return trf_cash(&z, 1000) != trf_cash(&t, 1000);
    case E_SPEND: return trf_cash(&z, -1000) != trf_cash(&t, -1000);
    case E_SHOP_DISC: return trf_shop(&z, 100) != trf_shop(&t, 100);
    case E_WALK_SPEED: { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_walk(&z); b += trf_walk(&t); } return a != b; }
    case E_VEH_SPEED: { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_veh(&z); b += trf_veh(&t); } return a != b; }
    case E_PAY_FARM: return trf_farm_pay(&z, 100) != trf_farm_pay(&t, 100);
    case E_PET_FOOD: { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_dog_fall(&z); b += trf_dog_fall(&t); } return a != b; }
    case E_GOTCHI_XP: { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_gxp(&z, 3); b += trf_gxp(&t, 3); } return a != b; }
    case E_GOTCHI_GIFT: return trf_gift(&z, 100) != trf_gift(&t, 100);
    case E_ARC_SCORE: return trf_arc_score(&z, 100) != trf_arc_score(&t, 100);
    case E_ARC_TIX: return trf_arc_tix(&z, 100) != trf_arc_tix(&t, 100);
    case E_FIGHT_DMG: return trf_fight_pw(&z, 100) != trf_fight_pw(&t, 100);
    case E_FIGHT_DEF: return trf_fight_hurt(&z, 100) != trf_fight_hurt(&t, 100);
    case E_LUCK: return trf_luck_pct(&z, 50) != trf_luck_pct(&t, 50);
    case E_LOOT: return trf_loot(&z, 100) != trf_loot(&t, 100);
    case E_SPARK: return trf_spark_pct(&z, 20) != trf_spark_pct(&t, 20);
    case E_AGE_SPEED: { int a = 0, b = 0; for (int i = 0; i < 100; i++) { a += trf_age(&z); b += trf_age(&t); } return a != b; }
    case E_ASP_GAIN: return trf_asp_gain(&z, 100) != trf_asp_gain(&t, 100);
    case E_ASP_KEEP: return trf_asp_loss(&z, 100) != trf_asp_loss(&t, 100);
    }
    return -1;   // no probe written for this hook
}

int main(void) {
    trf_init();
    // 1. every supported hook has a probe that sees it move (for both signs)
    int hookpass = 0;
    for (int i = 0; i < TRF_NSUP; i++) { int e = TRF_SUP[i], up = probe(e, 4), dn = probe(e, -4);
        int ok = up == 1 && (dn == 1 || e == E_MOOD_MIN); hookpass += ok; printf("HOOK %-3d %-14s %s\n", e, tr_ev_name ? "" : "", ok ? "PASS" : (up < 0 ? "NOPROBE" : "FAIL"));
        OK(ok, "hook %d (%s) does not move a DS number (up %d dn %d)", e, tr_hook_name[e], up, dn); }
    printf("hooks: %d of %d supported hooks move a DS number\n", hookpass, TRF_NSUP);
    // 2. every offered trait: each effect's hook is supported, and the DS number of its main hook moves in the trait's direction
    int noff = 0, nfail = 0, cat[16] = { 0 }, ncheat = 0, nearn = 0, nprz = 0;
    for (int id = 1; id <= TR_MAXID; id++) {
        if (!trf_on(id)) continue;
        noff++; if (id >= TR_FIRST_CHEAT) ncheat++; else if (id >= TR_FIRST_RWD) nprz++; else if (id >= TR_FIRST_EARN) nearn++; else cat[tr_cat[id] & 15]++;
        const u8 *e = trf_effp(id); int ok = 1;
        for (int k = 0; k < 3; k++) if (e[k]) { int h = e[k]; if (!trf_hok[h]) ok = 0; if (probe(h, (s8)e[3 + k]) != 1) ok = 0; }
        TrState t; memset(&t, 0, sizeof t); t.tid[0] = (u16)id; trf_rebuild(&t); if (t.tid[0] != id) ok = 0;
        int moved = 0; for (int h = 1; h < E_N; h++) if (t.fx[h] && trf_hok[h]) moved++;
        if (id < TR_FIRST_CHEAT || 1) { if (!moved && !(id >= TR_FIRST_CHEAT)) ok = 0; }
        if (id >= TR_FIRST_CHEAT) { TrState c; memset(&c, 0, sizeof c); tr_cheat_opt = 1; c.tid[0] = (u16)id; trf_rebuild(&c); tr_apply_id(&c, id, e, (const s8 *)(e + 3)); }
        if (!ok) { nfail++; printf("FAIL trait %d\n", id); }
    }
    tr_cheat_opt = 0;
    printf("offered: %d regular, %d earned, %d prize, %d cheat (%d of 600 regular)\n", noff - ncheat - nearn - nprz, nearn, nprz, ncheat, noff - ncheat - nearn - nprz);
    for (int c = 0; c < 15; c++) printf("  cat %2d %-12s %d\n", c, tr_catname[c], cat[c]);
    OK(nfail == 0, "%d offered traits do not move a DS number", nfail);
    printf("traits: %d offered, %d fail\n", noff, nfail);
    printf("\n%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
