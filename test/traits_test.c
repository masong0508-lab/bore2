// traits_test.c: host-side test of the trait engine. It compiles src/traits.c (the game's own file) and the generated data, and for every trait
// (600 regular, the earned and reward ones, the cheats) and every wish checks that it has an effect, applies it to a fresh test person and
// asks the SAME accessor the game uses (tr_scale, tr_add, tr_chance, tr_floor) whether the targeted number moved from the baseline.
// Build and run:  sh test/traits_test.sh
#ifndef TR_HOST
#define TR_HOST
#endif
#ifndef TR_BIG
#define TR_BIG
#endif
#include "../src/traits.h"
#include <stdio.h>
#include <string.h>

static int toasts, rewards, last_rk;
void tr_toast(const char *s) { (void)s; toasts++; }
void tr_reward(TrState *t, int kind, int arg, int wish) { (void)t; (void)arg; (void)wish; rewards++; last_rk = kind; }

static int probe(const TrState *t, int h) {   // the number the hook changes, through the game's accessor; returns (new - baseline)
    switch (tr_mode[h]) {
    case M_PCT: return tr_scale(t, h, 1000) - 1000;
    case M_ADD: return tr_add(t, h);
    case M_CHN: return tr_chance(t, h, 50) - 50;
    default:    return tr_floor(t) + 100;
    }
}
static const char *grp(int id) { return id <= TR_NREG ? "regular" : id < TR_FIRST_RWD ? "earned" : id < TR_FIRST_CHEAT ? "reward" : "cheat"; }

int main(void) {
    int tot[4] = { 0 }, ok[4] = { 0 }, bad = 0; tr_cheat_opt = 1;
    static const char *const gn[4] = { "regular", "earned", "reward", "cheat" };
    for (int id = 1; id <= TR_MAXID; id++) {
        const u8 *e = tr_eff[id];
        if (id > TR_NREG && id < TR_FIRST_EARN) continue;
        if (id >= TR_FIRST_EARN && id < TR_FIRST_RWD && id - TR_FIRST_EARN >= 0 && tr_sname[id - TR_FIRST_EARN][0] == 0) continue;   // spare slot
        if (id >= TR_FIRST_RWD && id < TR_FIRST_CHEAT && tr_sname[TR_NEARN + id - TR_FIRST_RWD][0] == 0) continue;
        if (id > TR_FIRST_RWD + TR_NRWD - 1 && id < TR_FIRST_CHEAT) continue;
        int g = id <= TR_NREG ? 0 : id < TR_FIRST_RWD ? 1 : id < TR_FIRST_CHEAT ? 2 : 3;
        tot[g]++;
        const char *nm = tr_names + tr_nameoff[id];
        int good = 1, n = 0; static TrState base, t;
        tr_clear(&base); tr_clear(&t);
        s8 m[3]; for (int k = 0; k < 3; k++) m[k] = (s8)e[3 + k];
        for (int k = 0; k < 3; k++) if (e[k]) n++;
        if (!n || !nm[0]) { printf("FAIL %s %d %s: no effect or no name\n", grp(id), id, nm); bad++; continue; }
        tr_apply_id(&t, id, e, m);
        for (int k = 0; k < 3; k++) {
            if (!e[k]) continue;
            int h = e[k];
            int a = probe(&base, h), b = probe(&t, h);
            if (a == b) { printf("FAIL %s %d %s: hook %s did not move (%d)\n", grp(id), id, nm, tr_hook_name[h], b); good = 0; }
        }
        // names unique
        for (int j = 1; j < id; j++) { const char *o = tr_names + tr_nameoff[j]; if (o[0] && !strcmp(o, nm)) { printf("FAIL %d %s: duplicate of %d\n", id, nm, j); good = 0; } }
        if (good) ok[g]++; else bad++;
    }
    // the earned traits: reaching the count in the game's own counter grants the trait and changes its effect
    int eok = 0, etot = 0;
    for (int i = 0; i < TR_NEARN; i++) {
        if (!tr_sname[i][0]) continue;
        etot++;
        static TrState t; tr_clear(&t);
        tr_ev(&t, tr_earn[i].ev, tr_earn[i].thr - 1 > 0 ? tr_earn[i].thr - 1 : 0);
        int had = 0; for (int k = 5; k < TR_SLOTS; k++) if (t.tid[k] == TR_FIRST_EARN + i) had = 1;
        tr_ev(&t, tr_earn[i].ev, 1);
        int has = 0; for (int k = 5; k < TR_SLOTS; k++) if (t.tid[k] == TR_FIRST_EARN + i) has = 1;
        int moved = 0; for (int h = 1; h < E_N; h++) if (t.fx[h]) moved = 1;
        // (other earned traits on the same counter may come at the same count: the one asked for must be among them or the slots full)
        if ((has || tr_earn[i].thr <= 1) && moved) eok++; else printf("FAIL earned %s: counter %s %d (has %d moved %d had %d)\n", tr_sname[i], tr_ev_name[tr_earn[i].ev], tr_earn[i].thr, has, moved, had);
    }
    // the wishes: each has a real counter, a goal, a reward; completing it pays once
    int wok = 0;
    for (int w = 1; w <= WISH_N; w++) {
        memset(tr_wdone, 0, sizeof tr_wdone);
        static TrState t; tr_clear(&t); rewards = 0;
        const WishDef *d = &tr_wish[w];
        int good = d->ev > 0 && d->ev < EV_N && d->thr > 0 && d->rk > 0 && tr_wish_name[w][0] && tr_wish_desc[w][0];
        tr_wish_pick(&t, w);
        int need, p0 = tr_wish_prog(&t, &need);
        if (tr_ev_abs[d->ev]) tr_ev_set(&t, d->ev, d->thr - 1 > 0 ? d->thr - 1 : 0); else tr_ev(&t, d->ev, d->thr - 1);
        if (d->thr > 1 && rewards) good = 0;               // not done one short of the goal
        if (tr_ev_abs[d->ev]) tr_ev_set(&t, d->ev, d->thr); else tr_ev(&t, d->ev, 1);
        if (rewards != 1 || !(tr_wdone[w >> 3] & (1 << (w & 7)))) good = 0;
        tr_ev(&t, d->ev, 5);                                  // and once only
        if (rewards != 1) good = 0;
        if (d->rk == RK_TRAIT && (d->ra < TR_FIRST_RWD || d->ra >= TR_FIRST_RWD + TR_NRWD)) good = 0;
        if (good) wok++; else printf("FAIL wish %d %s (p0 %d need %d rewards %d)\n", w, tr_wish_name[w], p0, need, rewards);
    }
    // the cheat switch: effects out and back
    { static TrState t; tr_clear(&t); tr_cheat_opt = 1; int id = TR_FIRST_CHEAT; tr_apply_id(&t, id, tr_eff[id], (s8 *)(tr_eff[id] + 3));
      int h = tr_eff[id][0], a = probe(&t, h); tr_cheat_opt = 0; tr_cheat_refresh(&t, 0); int b = probe(&t, h); tr_cheat_opt = 1; tr_cheat_refresh(&t, 1); int c = probe(&t, h);
      printf("cheat switch: on %d, off %d, on again %d, cheating flag %d\n", a, b, c, tr_cheating(&t)); if (!(a != 0 && b == 0 && c == a)) bad++; }
    printf("\n%-10s %6s %6s\n", "group", "traits", "pass");
    for (int g = 0; g < 4; g++) printf("%-10s %6d %6d\n", gn[g], tot[g], ok[g]);
    printf("%-10s %6d %6d   (reaching the count grants the trait)\n", "earn-run", etot, eok);
    printf("%-10s %6d %6d   (counter, goal, reward once)\n", "wishes", WISH_N, wok);
    int fail = bad + (etot - eok) + (WISH_N - wok);
    printf("%s: %d failures\n", fail ? "FAIL" : "PASS", fail);
    return fail != 0;
}
