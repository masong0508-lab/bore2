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
    int tot[4] = { 0 }, ok[4] = { 0 }, bad = 0; tr_cheat_opt = 1; tr_unl = TR_NUNL;
    static const char *const gn[4] = { "regular", "earned", "reward", "cheat" };
    for (int id = 1; id <= TR_MAXID; id++) {
        const u8 *e = tr_eff[id];
        if (id > TR_NREG && id < TR_FIRST_EARN) continue;
        if (id >= TR_FIRST_EARN && id < TR_FIRST_RWD && id - TR_FIRST_EARN >= 0 && tr_sname(id - TR_FIRST_EARN)[0] == 0) continue;   // spare slot
        if (id >= TR_FIRST_RWD && id < TR_FIRST_RWD + TR_NRWD && tr_sname(TR_NEARN + id - TR_FIRST_RWD)[0] == 0) continue;
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
        if (!tr_sname(i)[0]) continue;
        etot++;
        static TrState t; tr_clear(&t); tr_stage_set(&t, 3);
        tr_ev(&t, tr_earn[i].ev, tr_earn[i].thr - 1 > 0 ? tr_earn[i].thr - 1 : 0);
        int had = 0; for (int k = 0; k < TR_SLOTS; k++) if (t.tid[k] == TR_FIRST_EARN + i) had = 1;
        tr_ev(&t, tr_earn[i].ev, 1);
        int has = 0; for (int k = 0; k < TR_SLOTS; k++) if (t.tid[k] == TR_FIRST_EARN + i) has = 1;
        int moved = 0; for (int h = 1; h < E_N; h++) if (t.fx[h]) moved = 1;
        // (other earned traits on the same counter may come at the same count: the one asked for must be among them or the slots full)
        if ((has || tr_earn[i].thr <= 1) && moved) eok++; else printf("FAIL earned %s: counter %s %d (has %d moved %d had %d)\n", tr_sname(i), tr_ev_name[tr_earn[i].ev], tr_earn[i].thr, has, moved, had);
    }
    // the wishes: each has a real counter, a goal, a reward; completing it pays once
    int wok = 0;
    for (int w = 1; w <= WISH_N; w++) {
        memset(tr_wdone, 0, sizeof tr_wdone);
        static TrState t; tr_clear(&t); tr_stage_set(&t, 3); memset(tr_ev_a, 0, sizeof tr_ev_a); rewards = 0;
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

    // ---- slots, unlocks, the household (docs/TRAITS.md, Slots and Family) ----
    int sl_ok = 0, sl_tot = 0, un_ok = 0, un_tot = 0, au_ok = 0, au_tot = 0, cp_ok = 0, cp_tot = 0, ef_ok = 0, ef_tot = 0, rt_ok = 0, rt_tot = 0, gr_ok = 0, gr_tot = 0, ag_ok = 0, ag_tot = 0;
    #define CHK(cnt, tot, cond, ...) do { (tot)++; if (cond) (cnt)++; else { printf("FAIL " __VA_ARGS__); printf("\n"); } } while (0)
    {   // slot counts: 6 for a grown-up, fewer for the young, the unlocked ones from TEEN on
        tr_unl = 0;
        static const int want0[5] = { 1, 3, 4, 6, 6 };
        for (int st = 0; st < 5; st++) CHK(sl_ok, sl_tot, tr_nslots(st) == want0[st], "slots stage %d: %d, wanted %d", st, tr_nslots(st), want0[st]);
        for (int u = 1; u <= TR_NUNL; u++) { tr_unl = (u8)u; CHK(sl_ok, sl_tot, tr_nslots(3) == 6 + u && tr_nslots(4) == 6 + u && tr_nslots(2) == 4 + u && tr_nslots(1) == 3 && tr_nslots(0) == 1, "slots with %d unlocked", u); }
        CHK(sl_ok, sl_tot, TR_BASE == 6 && TR_SLOTS == 10 && TR_NUNL == 4, "constants");
        tr_unl = 0;
    }
    {   // unlock conditions: in order, from the wishes done and the job level
        static const struct { int wishes, job, want; } c[] = { { 0, 0, 0 }, { 1, 0, 1 }, { 2, 0, 1 }, { 3, 0, 2 }, { 3, 4, 2 }, { 3, 5, 3 }, { 9, 5, 3 }, { 10, 5, 4 }, { 10, 0, 2 }, { 0, 5, 0 }, { 61, 5, 4 } };
        for (unsigned i = 0; i < sizeof c / sizeof c[0]; i++) { tr_unl = 0; int n = tr_unl_check(c[i].wishes, c[i].job); CHK(un_ok, un_tot, tr_unl == c[i].want && n == c[i].want, "unlock wishes %d job %d: %d, wanted %d", c[i].wishes, c[i].job, tr_unl, c[i].want); }
        tr_unl = 1; CHK(un_ok, un_tot, tr_unl_check(1, 0) == 0 && tr_unl == 1, "unlock is monotonic and does not repeat");
        for (int k = 0; k < TR_NUNL; k++) CHK(un_ok, un_tot, tr_unl_text[k][0] != 0, "unlock %d has a text", k);
        tr_unl = 0;
    }
    {   // a won trait goes into the first free open slot; full: -1; a locked slot is not used; an auto member counts nothing
        static TrState t; tr_clear(&t); tr_stage_set(&t, 3); tr_unl = 0; memset(tr_ev_a, 0, sizeof tr_ev_a);
        for (int k = 0; k < 6; k++) t.tid[k] = (u16)(1 + k);
        CHK(gr_ok, gr_tot, tr_give(&t, TR_FIRST_EARN) == -1 && t.tid[6] == 0, "a won trait must not use a locked slot");
        tr_unl = 1; CHK(gr_ok, gr_tot, tr_give(&t, TR_FIRST_EARN) == 1 && t.tid[6] == TR_FIRST_EARN, "a won trait into the unlocked slot 7");
        CHK(gr_ok, gr_tot, tr_give(&t, TR_FIRST_EARN) == 0, "no second copy");
        static TrState a; tr_clear(&a); a.f |= TF_AUTO; tr_stage_set(&a, 3); memset(tr_ev_a, 0, sizeof tr_ev_a);
        tr_ev(&a, EV_COOKED, 50); CHK(gr_ok, gr_tot, tr_ev_a[EV_COOKED] == 0 && !a.tid[0], "an assigned member neither counts nor wins");
        // a won trait that found every slot full waits (tr_wait) and comes in when a slot opens: not before, and once
        static TrState w; tr_clear(&w); tr_stage_set(&w, 3); tr_unl = 0; memset(tr_wait, 0, sizeof tr_wait); memset(tr_ev_a, 0, sizeof tr_ev_a);
        for (int k = 0; k < 6; k++) w.tid[k] = (u16)(1 + k);
        tr_ev(&w, tr_earn[0].ev, tr_earn[0].thr);
        CHK(gr_ok, gr_tot, tr_wait[0] & 1 && w.tid[6] == 0, "a full Bore keeps the won trait waiting");
        tr_regive(&w); CHK(gr_ok, gr_tot, w.tid[6] == 0 && (tr_wait[0] & 1), "still waiting while the slots are shut");
        tr_unl = 2; tr_regive(&w); CHK(gr_ok, gr_tot, w.tid[6] == TR_FIRST_EARN && !(tr_wait[0] & 1), "it comes in when slot 7 opens");
        tr_unl = 0;
    }
    {   // the personality -> traits mapping
        static u16 seen_sets[64][TR_SLOTS]; int nseen = 0;
        int cat_hits[3] = { 0 }, cat_n[3] = { 0 };
        for (int sg = 0; sg < 12; sg++) for (int asp = 0; asp < 6; asp++) for (int v = 0; v < 24; v++) {
            TrPers p; for (int b = 0; b < 5; b++) p.bar[b] = (u8)(((v * 7 + b * 3 + sg * 5 + asp) * 2654435761u >> 11) % 11);
            p.sign = (u8)sg; p.asp = (u8)asp; p.stage = 3; { char f[12], l[12]; sprintf(f, "N%d", v * 12 + sg); sprintf(l, "L%d", asp * 31 + v); p.seed = tr_name_seed(f, l); }
            for (int slots = 3; slots <= 10; slots += slots == 3 ? 3 : 4) {
                u16 a[TR_SLOTS] = { 0 }, b2[TR_SLOTS] = { 0 };
                tr_auto_pick(&p, 0, 0, a, slots); tr_auto_pick(&p, 0, 0, b2, slots);
                CHK(au_ok, au_tot, !memcmp(a, b2, sizeof a), "determinism sg %d asp %d v %d", sg, asp, v);
                int n = 0; for (int k = 0; k < TR_SLOTS; k++) n += a[k] != 0;
                CHK(au_ok, au_tot, n == slots, "got %d traits, wanted %d (sg %d asp %d v %d)", n, slots, sg, asp, v);
                int clash = 0, dup = 0, cheat = 0, over = 0, catn[15] = { 0 };
                for (int i = 0; i < TR_SLOTS; i++) if (a[i]) { if (a[i] >= TR_FIRST_CHEAT) cheat = 1; catn[tr_cat_of(a[i])]++;
                    for (int j = i + 1; j < TR_SLOTS; j++) if (a[j]) { if (a[i] == a[j]) dup = 1; if (tr_clash(a[i], a[j])) clash = 1; } }
                for (int c = 0; c < 15; c++) over |= catn[c] > 3;
                CHK(cp_ok, cp_tot, !clash && !dup && !cheat && !over, "set not compatible: clash %d dup %d cheat %d over %d (sg %d asp %d v %d)", clash, dup, cheat, over, sg, asp, v);
                // every assigned trait has a wired effect: applied to a fresh Bore it moves a number through the game's own accessor
                static TrState t, base; tr_clear(&base); tr_clear(&t); tr_stage_set(&t, 3); memcpy(t.tid, a, sizeof a); tr_rebuild(&t);
                int moved = 1; for (int i = 0; i < TR_SLOTS; i++) if (a[i]) { const u8 *e = a[i] >= TR_FIRST_EARN && a[i] < TR_FIRST_RWD + TR_NRWD ? tr_small[a[i] >= TR_FIRST_RWD ? TR_NEARN + a[i] - TR_FIRST_RWD : a[i] - TR_FIRST_EARN] : tr_eff[a[i]];
                    int hit = 0; for (int k = 0; k < 3; k++) if (e[k] && probe(&t, e[k]) != probe(&base, e[k])) hit = 1; moved &= hit; }
                CHK(ef_ok, ef_tot, moved, "an assigned trait has no effect (sg %d asp %d v %d)", sg, asp, v);
                // the save keeps only the ids: rebuilding from them gives the same effects
                static TrState r; tr_clear(&r); tr_stage_set(&r, 3); memcpy(r.tid, t.tid, sizeof r.tid); tr_rebuild(&r);
                CHK(rt_ok, rt_tot, !memcmp(r.fx, t.fx, sizeof r.fx), "rebuild from ids differs");
                if (slots == 6 && nseen < 64 && (sg + asp + v) % 17 == 0) memcpy(seen_sets[nseen++], a, sizeof a);
                if (slots == 6) {   // what a strong bar pulls in: SOCIAL, ROMANCE or NIGHT AND DAY for the sociable; BODY, OUTDOORS or FIGHTING for the active; HOME, WORK or MIND for the neat homebody
                    static const u8 want[3][3] = { { 2, 3, 11 }, { 0, 8, 14 }, { 7, 4, 1 } };
                    int hit[3] = { p.bar[1] >= 9, p.bar[2] >= 9, p.bar[0] >= 9 && p.bar[1] <= 3 };
                    for (int w = 0; w < 3; w++) if (hit[w]) { cat_n[w]++; for (int i = 0; i < TR_SLOTS; i++) if (a[i] && (tr_cat_of(a[i]) == want[w][0] || tr_cat_of(a[i]) == want[w][1] || tr_cat_of(a[i]) == want[w][2])) { cat_hits[w]++; break; } }
                }
            }
        }
        CHK(au_ok, au_tot, cat_n[0] > 5 && cat_hits[0] * 100 >= cat_n[0] * 85, "sociable people get SOCIAL, ROMANCE or NIGHT traits: %d of %d", cat_hits[0], cat_n[0]);
        CHK(au_ok, au_tot, cat_n[1] > 5 && cat_hits[1] * 100 >= cat_n[1] * 85, "active people get BODY, OUTDOORS or FIGHTING traits: %d of %d", cat_hits[1], cat_n[1]);
        CHK(au_ok, au_tot, cat_n[2] > 3 && cat_hits[2] * 100 >= cat_n[2] * 85, "neat homebodies get HOME, WORK or MIND traits: %d of %d", cat_hits[2], cat_n[2]);
        int dist = 0; for (int i = 0; i < nseen; i++) { int same = 0; for (int j = 0; j < i; j++) if (!memcmp(seen_sets[i], seen_sets[j], sizeof seen_sets[i])) same = 1; dist += !same; }
        CHK(au_ok, au_tot, dist * 4 >= nseen * 3, "people differ: %d different sets of %d", dist, nseen);
        printf("auto: sociable %d/%d, active %d/%d, neat homebody %d/%d; %d different sets in %d people\n", cat_hits[0], cat_n[0], cat_hits[1], cat_n[1], cat_hits[2], cat_n[2], dist, nseen);
    }
    {   // growing up keeps what they have and adds; a child takes up to two traits from a parent
        TrPers p = { { 8, 9, 6, 7, 8 }, 4, 2, 1, 12345u }; u16 a[TR_SLOTS] = { 0 }, par[TR_SLOTS] = { 0 };
        tr_auto_pick(&p, 0, 0, a, 3); u16 before[TR_SLOTS]; memcpy(before, a, sizeof a);
        tr_auto_pick(&p, 0, 0, a, 6);
        int kept = 1; for (int k = 0; k < 3; k++) kept &= a[k] == before[k]; int n6 = 0; for (int k = 0; k < TR_SLOTS; k++) n6 += a[k] != 0;
        CHK(ag_ok, ag_tot, kept && n6 == 6, "growing up keeps the old traits and fills the new slots (kept %d, n %d)", kept, n6);
        tr_auto_pick(&(TrPers){ { 2, 1, 2, 3, 9 }, 7, 4, 3, 777u }, 0, 0, par, 6);
        u16 ch[TR_SLOTS] = { 0 }; tr_auto_pick(&(TrPers){ { 5, 5, 5, 5, 5 }, 0, 5, 1, 4242u }, par, 6, ch, 3);
        int inh = 0; for (int k = 0; k < 3; k++) for (int j = 0; j < 6; j++) if (ch[k] && ch[k] == par[j]) inh++;
        CHK(ag_ok, ag_tot, inh >= 1 && inh <= 2, "a child inherits one or two of a parent's traits (%d)", inh);
        u16 ch2[TR_SLOTS] = { 0 }; tr_auto_pick(&(TrPers){ { 5, 5, 5, 5, 5 }, 0, 5, 1, 4242u }, par, 6, ch2, 3);
        CHK(ag_ok, ag_tot, !memcmp(ch, ch2, sizeof ch), "inheritance is deterministic");
    }
    bad += (sl_tot - sl_ok) + (un_tot - un_ok) + (au_tot - au_ok) + (cp_tot - cp_ok) + (ef_tot - ef_ok) + (rt_tot - rt_ok) + (gr_tot - gr_ok) + (ag_tot - ag_ok);
    printf("\n%-10s %6s %6s\n", "group", "traits", "pass");
    for (int g = 0; g < 4; g++) printf("%-10s %6d %6d\n", gn[g], tot[g], ok[g]);
    printf("%-10s %6d %6d   (reaching the count grants the trait)\n", "earn-run", etot, eok);
    printf("%-10s %6d %6d   (counter, goal, reward once)\n", "wishes", WISH_N, wok);
    printf("%-10s %6d %6d   (6 slots, 1/3/4 for the young, unlocked from TEEN)\n", "slots", sl_tot, sl_ok);
    printf("%-10s %6d %6d   (in order, wishes and job level, no repeat)\n", "unlocks", un_tot, un_ok);
    printf("%-10s %6d %6d   (a won trait: first open slot, never a locked one, assigned members count nothing)\n", "won-slots", gr_tot, gr_ok);
    printf("%-10s %6d %6d   (same person, same traits; full; SOCIAL for the sociable; people differ)\n", "assign", au_tot, au_ok);
    printf("%-10s %6d %6d   (no opposites, no duplicates, 3 per category at most, no cheats)\n", "compat", cp_tot, cp_ok);
    printf("%-10s %6d %6d   (every assigned trait moves a number through the game's accessor)\n", "wired", ef_tot, ef_ok);
    printf("%-10s %6d %6d   (the ids alone give the same effects: what the save keeps)\n", "roundtrip", rt_tot, rt_ok);
    printf("%-10s %6d %6d   (growing up keeps and adds; a child inherits one or two)\n", "growth", ag_tot, ag_ok);
    int fail = bad + (etot - eok) + (WISH_N - wok);
    printf("%s: %d failures\n", fail ? "FAIL" : "PASS", fail);
    return fail != 0;
}
