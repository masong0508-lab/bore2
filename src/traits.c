// traits.c: see traits.h. Plain C, no game headers: the host test (test/traits_test.c) runs this very file.
#include "traits.h"
#include <string.h>
#include <stdio.h>
u8 tr_cheat_opt = 0, tr_cheat_ok, tr_wdone[8]; u32 tr_earned;

void tr_clear(TrState *t) { memset(t, 0, sizeof *t); }
void tr_fx_add(TrState *t, int hook, int tier) { if (hook <= 0 || hook >= E_N) return; int v = t->fx[hook] + tier; t->fx[hook] = (s8)CLAMP(v, -127, 127); }
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
    if (now_on == !!(t->f & TF_CHON)) return;
    if (now_on) t->f |= TF_CHON; else t->f &= (u8)~TF_CHON;
    for (int k = 0; k < 6; k++) if (t->ch[k]) tr_fx_add(t, t->ch[k], now_on ? t->cm[k] : -t->cm[k]);
}
const char *tr_small_name(int id) { int i = id >= TR_FIRST_RWD ? TR_NEARN + id - TR_FIRST_RWD : id - TR_FIRST_EARN; return i >= 0 && i < TR_NEARN + TR_NRWD ? tr_sname(i) : ""; }
int tr_give(TrState *t, int id) {
    int i = id >= TR_FIRST_RWD ? TR_NEARN + id - TR_FIRST_RWD : id - TR_FIRST_EARN, k;
    if (i < 0 || i >= TR_NEARN + TR_NRWD) return 0;
    for (k = TR_PICK; k < TR_SLOTS; k++) if (t->tid[k] == id) return 0;
    for (k = TR_PICK; k < TR_SLOTS; k++) if (!t->tid[k]) break;
    if (k >= TR_SLOTS) return 0;
    t->tid[k] = (u16)id;
    for (int e = 0; e < 3; e++) tr_fx_add(t, tr_small[i][e], (s8)tr_small[i][3 + e]);
    return 1;
}
int tr_wish_prog(const TrState *t, int *need) {
    int w = t->wish; if (!w) return 0;
    const WishDef *d = &tr_wish[w]; if (need) *need = d->thr;
    int v = t->ev[d->ev]; if (!tr_ev_abs[d->ev]) v -= t->wbase; if (v < 0) v = 0;
    return v > d->thr ? d->thr : v;
}
void tr_wish_pick(TrState *t, int w) { t->wish = (u8)(w > 0 && w <= WISH_N && !(tr_wdone[w >> 3] & (1 << (w & 7))) ? w : 0); t->wbase = t->wish ? t->ev[tr_wish[t->wish].ev] : 0; }
static void check(TrState *t, int ev) {
    for (int i = 0; i < TR_NEARN; i++) if (tr_earn[i].ev == ev && t->ev[ev] >= tr_earn[i].thr) {
        if (tr_give(t, TR_FIRST_EARN + i)) { char b[40]; snprintf(b, sizeof b, "NEW TRAIT: %s", tr_sname(i)); tr_toast(b); }
    }
    int w = t->wish;
    if (w && tr_wish[w].ev == ev) {
        int need, p = tr_wish_prog(t, &need);
        if (p >= need && !(tr_wdone[w >> 3] & (1 << (w & 7)))) {
            tr_wdone[w >> 3] |= (u8)(1 << (w & 7)); t->title = (u8)w; t->wish = 0;
            tr_reward(t, tr_wish[w].rk, tr_wish[w].ra, w);
        }
    }
}
int tr_ev(TrState *t, int ev, int n) { if (ev <= 0 || ev >= EV_N) return 0; int v = t->ev[ev] + n; t->ev[ev] = (u8)(v > 255 ? 255 : v); check(t, ev); return t->ev[ev]; }
void tr_ev_set(TrState *t, int ev, int v) { if (ev <= 0 || ev >= EV_N) return; if (v > 255) v = 255; if (v > t->ev[ev]) { t->ev[ev] = (u8)v; check(t, ev); } }

int tr_scale_f(const TrState *t, int e, int v) { int p = t->fx[e] * tr_unit[e]; if (p < -100) p = -100; return v * (100 + p) / 100; }
int tr_chance_f(const TrState *t, int e, int base) { int p = base + t->fx[e] * tr_unit[e]; return p < 0 ? 0 : p > 100 ? 100 : p; }
