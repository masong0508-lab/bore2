// traits.c: see traits.h. Plain C, no game headers: the host test (test/traits_test.c) runs this very file.
#define TR_DEFS
#include "traits.h"
#include <string.h>
#include <stdio.h>
u8 tr_cheat_opt = 0, tr_cheat_ok, tr_unl; u32 tr_earned; u8 tr_wait[10];
u16 tr_recent[8], tr_ed_pick[TR_PICK]; u8 tr_ed_wish, tr_staged;   // the creator's picks (CREATE.OVL), and the traits chosen last

char tr_pnm[TR_NPOOL][TR_SLOTS][TR_NLEN];
TrState tr_null, tr_pool[TR_NPOOL], *tr_stage_p;
void tr_clear(TrState *t) { if (t != &tr_null) memset(t, 0, sizeof *t); }
void tr_fx_add(TrState *t, int hook, int tier) { if (hook <= 0 || hook >= E_N || t == &tr_null) return; int v = t->fx[hook] + tier; t->fx[hook] = (s8)CLAMP(v, -127, 127); }
void tr_apply_id(TrState *t, int id, const u8 *hooks, const s8 *tiers) {
    int cheat = id >= TR_FIRST_CHEAT;
    for (int i = 0; i < 3; i++) {
        if (!hooks[i]) continue;
        if (!cheat) { tr_fx_add(t, hooks[i], tiers[i]); continue; }
        int k = 0; while (k < 6 && t->ch[k]) k++;
        if (k < 6) { t->ch[k] = hooks[i]; t->cm[k] = tiers[i]; if (tr_cheat_opt) { tr_fx_add(t, hooks[i], tiers[i]); t->f |= TF_CHON; } }
    }
    if (cheat) t->f |= TF_CHEAT;
}
void tr_cheat_refresh(TrState *t, int now_on) {   // the effects are added or taken off again (a cheat trait's part is in ch / cm); TF_CHON: they are in fx now
    if (t == &tr_null || now_on == !!(t->f & TF_CHON)) return;
    if (now_on) t->f |= TF_CHON; else t->f &= (u8)~TF_CHON;
    for (int k = 0; k < 6; k++) if (t->ch[k]) tr_fx_add(t, t->ch[k], now_on ? t->cm[k] : -t->cm[k]);
}
int tr_nslots(int stage) { if (stage < 0) stage = 0; if (stage > 4) stage = 4; return tr_stage_slots[stage] + (stage >= TR_UNL_STAGE ? tr_unl : 0); }
int tr_unl_check(int wishes_done, int job_level) {
    int n = 0;
    while (tr_unl < TR_NUNL) {
        int v = tr_unl_kind[tr_unl] == UK_JOB ? job_level : wishes_done;
        if (v < tr_unl_thr[tr_unl]) break;
        tr_unl++; n++;
    }
    return n;
}
int tr_give(TrState *t, int id) {
    if (t == &tr_null) return 0;
    int i = id >= TR_FIRST_RWD ? TR_NEARN + id - TR_FIRST_RWD : id - TR_FIRST_EARN, k;
    if (i < 0 || i >= TR_NEARN + TR_NRWD) return 0;
    for (k = 0; k < TR_SLOTS; k++) if (t->tid[k] == id) return 0;
    int ns = tr_nslots(tr_stage_get(t));
    for (k = 0; k < ns && k < TR_SLOTS; k++) if (!t->tid[k]) break;
    if (k >= ns || k >= TR_SLOTS) {
        if (t->f & TF_AUTO) {   // a household member: the won trait takes the place of the weakest one their personality gave (the last of those), and the effects are made again (TF_PEND)
            for (k = (ns < TR_SLOTS ? ns : TR_SLOTS) - 1; k >= 0; k--) if (t->tid[k] && t->tid[k] < TR_FIRST_EARN) break;
            if (k < 0) return 0;
            t->tid[k] = (u16)id; t->f |= TF_PEND; return 1;
        }
        tr_wait[i >> 3] |= (u8)(1 << (i & 7)); return -1;
    }
    tr_wait[i >> 3] &= (u8)~(1 << (i & 7));
    t->tid[k] = (u16)id;
    for (int e = 0; e < 3; e++) tr_fx_add(t, tr_small[i][e], (s8)tr_small[i][3 + e]);
    return 1;
}
void tr_regive(TrState *t) {
    if (t == &tr_null || (t->f & TF_AUTO)) return;   // (the waiting prizes are the played Bore's; a member's win never waits)
    for (int i = 0; i < TR_NEARN + TR_NRWD; i++) if (tr_wait[i >> 3] >> (i & 7) & 1) tr_give(t, i < TR_NEARN ? TR_FIRST_EARN + i : TR_FIRST_RWD + i - TR_NEARN);
}
int tr_ev_mem(int ev) { return (ev >= EV_COOKED && ev <= EV_KIND) || ev == EV_LUCKY || ev == EV_DAYS || (ev >= EV_BANDLV && ev <= EV_SK_FARM); }
int tr_wd_count(const TrState *t) { int n = 0; for (int k = 0; k < 8; k++) { u8 b = t->wd[k]; while (b) { n += b & 1; b >>= 1; } } return n; }
#define WDONE(t, w) ((t)->wd[(w) >> 3] & (1 << ((w) & 7)))
int tr_wish_prog(const TrState *t, int *need) {
    int w = t->wish; if (!w) return 0;
    const WishDef *d = &tr_wish[w]; if (need) *need = d->thr;
    int v = t->ev[d->ev]; if (!tr_ev_abs[d->ev]) v -= t->wbase; if (v < 0) v = 0;
    return v > d->thr ? d->thr : v;
}
void tr_wish_pick(TrState *t, int w) { if (t == &tr_null) return; t->wish = (u8)(w > 0 && w <= WISH_N && !WDONE(t, w) ? w : 0); t->wbase = t->wish ? t->ev[tr_wish[t->wish].ev] : 0; }
static void check(TrState *t, int ev) {
    for (int i = 0; i < TR_NEARN; i++) if (tr_earn[i].ev == ev && t->ev[ev] >= tr_earn[i].thr) {
        int r = tr_give(t, TR_FIRST_EARN + i);
        if (r) tr_won(t, r);
    }
    int w = t->wish;
    if (w && tr_wish[w].ev == ev) {
        int need, p = tr_wish_prog(t, &need);
        if (p >= need && !WDONE(t, w)) {
            t->wd[w >> 3] |= (u8)(1 << (w & 7)); t->title = (u8)w; t->wish = 0;
            tr_reward(t, tr_wish[w].rk, tr_wish[w].ra, w);
            if (t->f & TF_AUTO) { t->f |= TF_PEND; return; }   // a member: the next wish is chosen from their personality (traits_auto.c)
            for (int k = 1; k < WISH_N; k++) { int n = (w + k - 1) % WISH_N + 1; if (!WDONE(t, n)) { tr_wish_pick(t, n); break; } }   // the next wish waits
        }
    }
}
int tr_ev(TrState *t, int ev, int n) { if (ev <= 0 || ev >= EV_N || t == &tr_null) return 0; int v = t->ev[ev] + n; t->ev[ev] = (u8)(v > 255 ? 255 : v); check(t, ev); return t->ev[ev]; }
void tr_ev_quiet(TrState *t, int ev, int v) { if (ev > 0 && ev < EV_N && t != &tr_null) t->ev[ev] = (u8)(v > 255 ? 255 : v < 0 ? 0 : v); }
void tr_ev_set(TrState *t, int ev, int v) { if (ev <= 0 || ev >= EV_N || t == &tr_null) return; if (v > 255) v = 255; if (v > t->ev[ev]) { t->ev[ev] = (u8)v; check(t, ev); } }

int tr_scale_f(const TrState *t, int e, int v) { int p = t->fx[e] * tr_unit[e]; if (p < -100) p = -100; return v * (100 + p) / 100; }
int tr_chance_f(const TrState *t, int e, int base) { int p = base + t->fx[e] * tr_unit[e]; return p < 0 ? 0 : p > 100 ? 100 : p; }
