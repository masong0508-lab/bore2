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
#include "../src/persona.h"
#include <stdio.h>
#include <string.h>

static int toasts, rewards, last_rk;
void tr_toast(const char *s) { (void)s; toasts++; }
void tr_reward(TrState *t, int kind, int arg, int wish) { (void)t; (void)arg; (void)wish; rewards++; last_rk = kind; }
static int won_n, won_last; void tr_won(TrState *t, int r) { (void)t; won_n++; won_last = r; }

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
        static TrState t; tr_clear(&t); tr_stage_set(&t, 3); rewards = 0;
        const WishDef *d = &tr_wish[w];
        int good = d->ev > 0 && d->ev < EV_N && d->thr > 0 && d->rk > 0 && tr_wish_name[w][0] && tr_wish_desc[w][0];
        tr_wish_pick(&t, w);
        int need, p0 = tr_wish_prog(&t, &need);
        if (tr_ev_abs[d->ev]) tr_ev_set(&t, d->ev, d->thr - 1 > 0 ? d->thr - 1 : 0); else tr_ev(&t, d->ev, d->thr - 1);
        if (d->thr > 1 && rewards) good = 0;               // not done one short of the goal
        if (tr_ev_abs[d->ev]) tr_ev_set(&t, d->ev, d->thr); else tr_ev(&t, d->ev, 1);
        if (rewards != 1 || !(t.wd[w >> 3] & (1 << (w & 7)))) good = 0;
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
        static TrState t; tr_clear(&t); tr_stage_set(&t, 3); tr_unl = 0;
        for (int k = 0; k < 6; k++) t.tid[k] = (u16)(1 + k);
        CHK(gr_ok, gr_tot, tr_give(&t, TR_FIRST_EARN) == -1 && t.tid[6] == 0, "a won trait must not use a locked slot");
        tr_unl = 1; CHK(gr_ok, gr_tot, tr_give(&t, TR_FIRST_EARN) == 1 && t.tid[6] == TR_FIRST_EARN, "a won trait into the unlocked slot 7");
        CHK(gr_ok, gr_tot, tr_give(&t, TR_FIRST_EARN) == 0, "no second copy");
        static TrState a; tr_clear(&a); a.f |= TF_AUTO; tr_stage_set(&a, 3);
        tr_ev(&a, EV_COOKED, 50); CHK(gr_ok, gr_tot, a.ev[EV_COOKED] == 50 && t.ev[EV_COOKED] == 0, "an assigned member counts its own events, not the others'");
        // a won trait that found every slot full waits (tr_wait) and comes in when a slot opens: not before, and once
        static TrState w; tr_clear(&w); tr_stage_set(&w, 3); tr_unl = 0; memset(tr_wait, 0, sizeof tr_wait);
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
    // ---- a town person's personality, and the household's own state (docs/TRAITS.md, "Town people" and "Everyone has their own") ----
    int pe_ok = 0, pe_tot = 0, st_ok = 0, st_tot = 0, hh_ok = 0, hh_tot = 0;
    {   // persona_of: from the id alone, the same every time; five bars that add up to 25, between 0 and 10; the sign is what the bars say (so the Bore made on move-in reads the same sign); kids want GROW UP
        int signn[12] = { 0 }; static u8 seenb[4096][5]; int nb = 0;
        for (u32 seed = 1; seed <= 60000; seed++) for (int stage = 1; stage <= 3; stage += 2) {
            Persona a, b; persona_of(seed * 2654435761u, stage, &a); persona_of(seed * 2654435761u, stage, &b);
            int sum = 0, rng = 1; for (int i = 0; i < 5; i++) { sum += a.bar[i]; rng &= a.bar[i] <= 10; }
            CHK(pe_ok, pe_tot, !memcmp(&a, &b, sizeof a), "persona not stable for %u", seed);
            CHK(pe_ok, pe_tot, sum == 25 && rng, "bars %d %d %d %d %d (sum %d)", a.bar[0], a.bar[1], a.bar[2], a.bar[3], a.bar[4], sum);
            CHK(pe_ok, pe_tot, persona_sign(a.bar) == a.sign && a.sign < 12, "sign %d is not what the bars say (%d)", a.sign, persona_sign(a.bar));
            CHK(pe_ok, pe_tot, stage < 2 ? a.asp == 5 : a.asp < 5, "aspiration %d at stage %d", a.asp, stage);
            CHK(pe_ok, pe_tot, a.ltw < 4, "ltw");
            if (stage == 3) { signn[a.sign]++; int have = 0; for (int k = 0; k < nb; k++) if (!memcmp(seenb[k], a.bar, 5)) { have = 1; break; } if (!have && nb < 4096) memcpy(seenb[nb++], a.bar, 5); }
        }
        int minn = 1 << 30; for (int i = 0; i < 12; i++) if (signn[i] < minn) minn = signn[i];
        CHK(pe_ok, pe_tot, minn > 60000 / 12 / 2, "every sign appears (the rarest %d of 60000)", minn);
        CHK(pe_ok, pe_tot, nb > 150, "people differ inside a sign: %d different bar sets", nb);
        printf("persona: 60000 ids x 2 stages stable; the rarest sign %d of 60000; %d different bar sets\n", minn, nb);
    }
    {   // the same person, the same traits: in town (their personality straight from the id) and on move-in (the Bore made from it: bars copied, the sign read back from the bars, the aspiration)
        for (u32 seed = 7; seed < 2007; seed++) {
            Persona p; persona_of(seed * 40503u, 3, &p);
            TrPers town = { { p.bar[0], p.bar[1], p.bar[2], p.bar[3], p.bar[4] }, p.sign, p.asp, 3, seed * 977u };
            u8 bore_tr[5]; for (int i = 0; i < 5; i++) bore_tr[i] = p.bar[i];             // cit_persona: the Bore's bars
            TrPers move = { { bore_tr[0], bore_tr[1], bore_tr[2], bore_tr[3], bore_tr[4] }, (u8)persona_sign(bore_tr), p.asp, 3, seed * 977u };   // tr_pers_of: the sign read back (look_sign_of)
            u16 a[TR_SLOTS] = { 0 }, b[TR_SLOTS] = { 0 };
            tr_auto_pick(&town, 0, 0, a, 6); tr_auto_pick(&move, 0, 0, b, 6);
            CHK(st_ok, st_tot, !memcmp(a, b, sizeof a) && a[5], "town and move-in traits differ for %u", seed);
            TrState t; tr_clear(&t); tr_stage_set(&t, 3); t.f |= TF_AUTO; int w1 = tr_auto_wish(&town, &t), w2 = tr_auto_wish(&move, &t);
            CHK(st_ok, st_tot, w1 == w2 && w1 > 0 && tr_ev_mem(tr_wish[w1].ev), "town and move-in wish differ for %u (%d %d)", seed, w1, w2);
        }
        // the very same person built twice by different routes lands on the same set of 6 traits for each of 12 signs x 5 aspirations
        for (int sg = 0; sg < 12; sg++) for (int asp = 0; asp < 5; asp++) {
            TrPers t1 = { { 0 }, (u8)sg, (u8)asp, 3, 99u + (u32)sg }; for (int i = 0; i < 5; i++) t1.bar[i] = pers_sign_tr[sg][i];
            u8 br[5]; for (int i = 0; i < 5; i++) br[i] = pers_sign_tr[sg][i];
            CHK(st_ok, st_tot, persona_sign(br) == sg, "sign %d table is not nearest to itself", sg);
        }
    }
    {   // a household of eight: every member has their own state, counters, earned traits and wish
        static TrState hh[8]; u16 inh[2]; (void)inh; tr_unl = 0; memset(tr_wait, 0, sizeof tr_wait);
        static const u8 stg[8] = { 3, 3, 2, 1, 4, 3, 3, 1 };
        u16 first_wish[8];
        for (int m = 0; m < 8; m++) {
            TrState *t = &hh[m]; tr_clear(t); t->f |= TF_AUTO; tr_stage_set(t, stg[m]);
            Persona p; persona_of(0x1234567u + (u32)m * 7919u, stg[m], &p);
            TrPers tp = { { p.bar[0], p.bar[1], p.bar[2], p.bar[3], p.bar[4] }, p.sign, p.asp, (u8)stg[m], tr_name_seed("MEM", "BER") + (u32)m };
            tr_auto_pick(&tp, 0, 0, t->tid, tr_nslots(stg[m])); tr_rebuild(t);
            if (stg[m] >= 1) tr_wish_pick(t, tr_auto_wish(&tp, t));
            first_wish[m] = t->wish;
            int ns = tr_nslots(stg[m]), n = 0; for (int k = 0; k < TR_SLOTS; k++) n += t->tid[k] != 0;
            CHK(hh_ok, hh_tot, n == ns, "member %d has %d traits, wanted %d", m, n, ns);
            CHK(hh_ok, hh_tot, stg[m] < 1 || (t->wish && tr_ev_mem(tr_wish[t->wish].ev)), "member %d has no countable personal wish", m);
        }
        int diff = 0; for (int m = 1; m < 8; m++) diff += memcmp(hh[m].tid, hh[0].tid, sizeof hh[0].tid) != 0;
        CHK(hh_ok, hh_tot, diff >= 6, "members have their own trait sets (%d of 7 differ from the first)", diff);
        // counters are each member's own
        for (int m = 0; m < 8; m++) tr_ev(&hh[m], EV_COOKED, m + 1);
        for (int m = 0; m < 8; m++) CHK(hh_ok, hh_tot, hh[m].ev[EV_COOKED] == m + 1, "member %d counter %d, wanted %d", m, hh[m].ev[EV_COOKED], m + 1);
        // an earned trait: the one who reaches the count wins it (taking the weakest assigned trait's place), nobody else
        int goal_i = -1; for (int i = 0; i < TR_NEARN; i++) if (tr_earn[i].ev == EV_COOKED && tr_sname(i)[0]) { goal_i = i; break; }
        if (goal_i >= 0) {
            int thr = tr_earn[goal_i].thr, id = TR_FIRST_EARN + goal_i; won_n = 0;
            for (int m = 0; m < 8; m++) for (int k = 0; k < TR_SLOTS; k++) hh[m].tid[k] = hh[m].tid[k] == (u16)id ? 0 : hh[m].tid[k];   // (not one of them already has it from the personality)
            for (int m = 0; m < 8; m++) { tr_rebuild(&hh[m]); int ns = tr_nslots(stg[m]); for (int k = 0; k < ns; k++) if (!hh[m].tid[k]) { u16 tmp[TR_SLOTS]; memcpy(tmp, hh[m].tid, sizeof tmp); (void)tmp; } }
            u16 keep[TR_SLOTS]; memcpy(keep, hh[2].tid, sizeof keep); u16 last_reg = 0; int lk = -1; { int ns = tr_nslots(stg[2]); for (int k = ns - 1; k >= 0; k--) if (hh[2].tid[k] && hh[2].tid[k] < TR_FIRST_EARN) { last_reg = hh[2].tid[k]; lk = k; break; } }
            tr_ev(&hh[2], EV_COOKED, thr);   // member 2 (a teen) cooks enough
            int has2 = 0; for (int k = 0; k < TR_SLOTS; k++) has2 |= hh[2].tid[k] == id;
            int others = 0; for (int m = 0; m < 8; m++) if (m != 2) for (int k = 0; k < TR_SLOTS; k++) others |= hh[m].tid[k] == id;
            CHK(hh_ok, hh_tot, has2 && !others, "the earned trait goes to the member who earned it (has %d, others %d)", has2, others);
            CHK(hh_ok, hh_tot, !(hh[2].f & TF_PEND) || lk >= 0, "a full member takes the weakest trait's place and is marked to rebuild");
            CHK(hh_ok, hh_tot, won_n >= 1 && won_last == 1, "the win was told");
            // after the game's rebuild the effects hold the new trait and not the old one
            if (lk >= 0) { CHK(hh_ok, hh_tot, hh[2].tid[lk] == id && last_reg != id, "the weakest assigned trait (slot %d) was replaced", lk); tr_rebuild(&hh[2]); hh[2].f &= (u8)~TF_PEND;
              int moved = 0; for (int k = 0; k < 3; k++) if (tr_small[goal_i][k]) moved |= probe(&hh[2], tr_small[goal_i][k]) != 0;
              CHK(hh_ok, hh_tot, moved, "the earned trait's effect is on after the rebuild"); }
            (void)keep;
            memcpy(hh[2].tid, keep, sizeof keep); tr_rebuild(&hh[2]);   // back as it was for the rest
        }
        // a personal wish: done by the member whose it is, paid once, the next one is chosen from the personality and is a new one
        { int m = 0; TrState *t = &hh[m]; int w = t->wish; rewards = 0;
          if (w) {
              const WishDef *d = &tr_wish[w]; int already = t->ev[d->ev];
              if (tr_ev_abs[d->ev]) tr_ev_set(t, d->ev, d->thr); else tr_ev(t, d->ev, d->thr - (already - t->wbase > 0 ? already - t->wbase : 0) + 1);
              CHK(hh_ok, hh_tot, rewards == 1, "a member's wish pays once (%d)", rewards);
              CHK(hh_ok, hh_tot, (t->wd[w >> 3] >> (w & 7) & 1) && !t->wish && (t->f & TF_PEND), "the member's wish is marked done and the next one waits for the personality");
              int others = 0; for (int o = 1; o < 8; o++) others |= (hh[o].wd[w >> 3] >> (w & 7)) & 1;
              CHK(hh_ok, hh_tot, !others, "nobody else's wish was done");
              Persona p; persona_of(0x1234567u, stg[0], &p); TrPers tp = { { p.bar[0], p.bar[1], p.bar[2], p.bar[3], p.bar[4] }, p.sign, p.asp, (u8)stg[0], 0 };
              int nw2 = tr_auto_wish(&tp, t); CHK(hh_ok, hh_tot, nw2 != w && nw2 > 0, "the next wish (%d) is a new one (done %d)", nw2, w);
          } }
        // the whole household's progress: every state is its own bytes; a copy of all of them (what the save keeps, rle packed per member) is identical
        { static TrState back[8]; memcpy(back, hh, sizeof hh); int same = !memcmp(back, hh, sizeof hh); CHK(hh_ok, hh_tot, same, "save round trip");
          for (int m = 0; m < 8; m++) { static TrState old; memset(&old, 0, sizeof old); memcpy(&old, &hh[m], TR_SIZE_V51);   // an old (version 50 or 51) save keeps only the first part: the counters start at zero
            CHK(hh_ok, hh_tot, !memcmp(old.tid, hh[m].tid, sizeof old.tid) && old.ev[EV_COOKED] == 0, "an old state loads as the first part"); } }
        CHK(hh_ok, hh_tot, sizeof(TrState) == TR_SIZE_V51 + EV_N + 8 && TR_SIZE_V51 == 180, "TrState is the old 180 bytes + counters + wishes (%d, %d)", (int)sizeof(TrState), (int)TR_SIZE_V51);
        printf("household: 8 members, own traits (%d differ), own counters, earned and personal wish checked\n", diff);
    }
    int tw_ok = 0, tw_tot = 0;
    {   // STABLE TOWNIES (docs/TRAITS.md): a crowd townie is a pure function of the save's world seed and the id. Same pair, same person; another save, another person; the save keeps only the seed
        static u32 seen[60000], srt[60000]; int clash = 0; u32 ws = 0xC0FFEE42u;
        for (u32 id = 0; id < 60000; id++) {
            u32 a = persona_town_seed(ws, id), b = persona_town_seed(ws, id), o = persona_town_seed(ws ^ 0x9E3779B9u, id);
            TownDna da, db; persona_town_dna(a, &da); persona_town_dna(b, &db);
            Persona pa, pb; persona_of(a, 3, &pa); persona_of(b, 3, &pb);
            CHK(tw_ok, tw_tot, a == b && !memcmp(&da, &db, sizeof da) && !memcmp(&pa, &pb, sizeof pa), "townie %u is not stable", id);
            CHK(tw_ok, tw_tot, a != 0 && da.age < 4 && da.pers < 4 && da.off >= -45 && da.off <= 45 && da.first < (1u << 24) && da.last < (1u << 24), "townie %u dna out of range", id);
            seen[id] = a; clash += o == a;
        }
        CHK(tw_ok, tw_tot, clash == 0, "%d ids gave the same seed in another save", clash);
        memcpy(srt, seen, sizeof srt);
        int dup = 0; for (int i = 1; i < 60000; i++) for (int j = i; j > 0 && srt[j] < srt[j - 1]; j--) { u32 t = srt[j]; srt[j] = srt[j - 1]; srt[j - 1] = t; }
        for (int i = 1; i < 60000; i++) dup += srt[i] == srt[i - 1];
        CHK(tw_ok, tw_tot, dup == 0, "%d ids share a seed", dup);
        int same_name = 0, same_pers = 0;
        for (u32 id = 0; id < 5000; id++) { TownDna x, y; persona_town_dna(persona_town_seed(ws, id), &x); persona_town_dna(persona_town_seed(ws + 1, id), &y); same_name += x.first == y.first && x.last == y.last;
            Persona p, q; persona_of(persona_town_seed(ws, id), 3, &p); persona_of(persona_town_seed(ws + 1, id), 3, &q); same_pers += !memcmp(&p, &q, sizeof p); }
        CHK(tw_ok, tw_tot, same_name == 0 && same_pers < 5000 / 20, "another save's townies are the same people (%d names, %d personalities of 5000)", same_name, same_pers);
        // save / load round trip: the save holds the world seed (4 bytes as sv_u32 / ld_u32 write them; every save since version 2 has it) and nothing else about them
        u8 card[4] = { (u8)ws, (u8)(ws >> 8), (u8)(ws >> 16), (u8)(ws >> 24) };
        u32 back = (u32)card[0] | (u32)card[1] << 8 | (u32)card[2] << 16 | (u32)card[3] << 24;
        // (an old save, version 52 or older, has that same world seed: its townies are made the same way, so loading it gives each of them the identity a new boot of that world gives)
        // (a person who moved in: the Bore is made from the same seed in cit_bore, so their bars and name dice are the town ones)
        for (u32 id = 0; id < 50; id++) { u32 sd = persona_town_seed(ws, id), sb = persona_town_seed(back, id); TownDna x, y; persona_town_dna(sd, &x); persona_town_dna(sb, &y);
            Persona t, m; persona_of(sd, 3, &t); persona_of(sb, 3, &m);
            CHK(tw_ok, tw_tot, sd == sb && !memcmp(&x, &y, sizeof x) && !memcmp(&t, &m, sizeof t), "townie %u changed over a save", id); }
        printf("stable townies: 60000 ids: same seed twice, %d clashes with another save, %d duplicate seeds; save round trip; old save; move-in\n", clash, dup);
    }
    bad += (pe_tot - pe_ok) + (st_tot - st_ok) + (hh_tot - hh_ok) + (sl_tot - sl_ok) + (un_tot - un_ok) + (au_tot - au_ok) + (cp_tot - cp_ok) + (ef_tot - ef_ok) + (rt_tot - rt_ok) + (gr_tot - gr_ok) + (ag_tot - ag_ok) + (tw_tot - tw_ok);
    printf("\n%-10s %6s %6s\n", "group", "traits", "pass");
    for (int g = 0; g < 4; g++) printf("%-10s %6d %6d\n", gn[g], tot[g], ok[g]);
    printf("%-10s %6d %6d   (reaching the count grants the trait)\n", "earn-run", etot, eok);
    printf("%-10s %6d %6d   (counter, goal, reward once)\n", "wishes", WISH_N, wok);
    printf("%-10s %6d %6d   (6 slots, 1/3/4 for the young, unlocked from TEEN)\n", "slots", sl_tot, sl_ok);
    printf("%-10s %6d %6d   (in order, wishes and job level, no repeat)\n", "unlocks", un_tot, un_ok);
    printf("%-10s %6d %6d   (a won trait: first open slot, never a locked one, each member counts their own)\n", "won-slots", gr_tot, gr_ok);
    printf("%-10s %6d %6d   (same person, same traits; full; SOCIAL for the sociable; people differ)\n", "assign", au_tot, au_ok);
    printf("%-10s %6d %6d   (no opposites, no duplicates, 3 per category at most, no cheats)\n", "compat", cp_tot, cp_ok);
    printf("%-10s %6d %6d   (every assigned trait moves a number through the game's accessor)\n", "wired", ef_tot, ef_ok);
    printf("%-10s %6d %6d   (the ids alone give the same effects: what the save keeps)\n", "roundtrip", rt_tot, rt_ok);
    printf("%-10s %6d %6d   (growing up keeps and adds; a child inherits one or two)\n", "growth", ag_tot, ag_ok);
    printf("%-10s %6d %6d   (a town person's personality: the same from the id, five bars add up to 25, the sign is the bars', all 12 signs, kids GROW UP)\n", "persona", pe_tot, pe_ok);
    printf("%-10s %6d %6d   (the same person gives the same traits in town and the day they move in)\n", "same-trait", st_tot, st_ok);
    printf("%-10s %6d %6d   (8 members: own state, own counters, own earned traits, own wish, save round trip)\n", "household", hh_tot, hh_ok);
    printf("%-10s %6d %6d   (a crowd townie from the world seed and the id alone: same every time, another save differs, save round trip, old save, move-in)\n", "stable", tw_tot, tw_ok);
    int fail = bad + (etot - eok) + (WISH_N - wok);
    printf("%s: %d failures\n", fail ? "FAIL" : "PASS", fail);
    return fail != 0;
}
