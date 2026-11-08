// traits_auto.c: a household member's traits come from their personality (docs/TRAITS.md, "Family"). Plain C, no game headers: CREATE.OVL and the host test
// (test/traits_test.c) compile this very file. Nothing here is stored: the same person always gives the same traits (the result is kept in TrState.tid and the save).
#ifndef TR_BIG
#define TR_BIG
#endif
#include "traits.h"
#include <string.h>

static const u8 *eff_of(int id) {   // an id's effects: hook x3, tier x3
    if (id >= TR_FIRST_EARN && id < TR_FIRST_EARN + TR_NEARN) return tr_small[id - TR_FIRST_EARN];
    if (id >= TR_FIRST_RWD && id < TR_FIRST_RWD + TR_NRWD) return tr_small[TR_NEARN + id - TR_FIRST_RWD];
    return tr_eff[id];
}
int tr_clash(int a, int b) {   // two traits that cannot be had together: the same main effect, or one helps what the other hurts
    const u8 *x = eff_of(a), *y = eff_of(b);
    if (x[0] == y[0]) return 1;
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) if (x[i] && x[i] == y[j] && ((s8)x[3 + i] > 0) != ((s8)y[3 + j] > 0)) return 1;
    return 0;
}
void tr_rebuild(TrState *t) {   // every effect again, from the ids
    memset(t->fx, 0, sizeof t->fx); memset(t->ch, 0, sizeof t->ch); memset(t->cm, 0, sizeof t->cm); t->f &= (u8)~(TF_CHEAT | TF_CHON);
    for (int k = 0; k < TR_SLOTS; k++) { int id = t->tid[k]; if (!id) continue; const u8 *e = eff_of(id); tr_apply_id(t, id, e, (const s8 *)(e + 3)); }
}
u32 tr_name_seed(const char *first, const char *last) {   // the person's id seed: their name
    u32 h = 2166136261u;
    for (const char *s = first; *s; s++) h = (h ^ (u8)*s) * 16777619u;
    h = (h ^ 32) * 16777619u;
    for (const char *s = last; *s; s++) h = (h ^ (u8)*s) * 16777619u;
    return h;
}
static u32 mix(u32 a, u32 b) { u32 h = a * 2654435761u ^ (b + 0x9e3779b9u) * 40503u; h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; return h; }
int tr_cat_of(int id) {   // the trait's category 0..14 (an earned or prize trait: the category of its first effect)
    if (id >= TR_FIRST_EARN && id < TR_FIRST_CHEAT) return tr_hook_cat[eff_of(id)[0]] % 15;
    return tr_cat[id] & 15;
}
static s16 sc[TR_FIRST_RWD + TR_NRWD];
int tr_auto_pick(const TrPers *p, const u16 *inherit, int ninh, u16 *tid, int nslots) {
    int cs[15], ncat[15], i, k, added = 0, ninhn = 0;
    if (nslots > TR_SLOTS) nslots = TR_SLOTS;
    for (int c = 0; c < 15; c++) {   // what this person pulls towards, by category
        int v = 0;
        for (int b = 0; b < 5; b++) v += tr_pk[c][b] * ((int)p->bar[b] - 5);
        v += 4 * tr_pasp[p->asp % 6][c] + 3 * tr_psign[(p->sign % 12) & 3][c];
        cs[c] = v; ncat[c] = 0;
    }
    for (k = 0; k < TR_SLOTS; k++) if (tid[k] && tid[k] < TR_FIRST_CHEAT) ncat[tr_cat_of(tid[k])]++;
    for (i = 1; i < TR_FIRST_RWD + TR_NRWD; i++) {
        sc[i] = -9999;
        if (i < TR_FIRST_EARN) { }
        else if (i < TR_FIRST_EARN + TR_NEARN) { if (!tr_earn[i - TR_FIRST_EARN].ev) continue; }
        else if (i < TR_FIRST_RWD) continue;
        int c = tr_cat_of(i), v;
        if (i < TR_FIRST_EARN) v = 4 * cs[c];
        else { v = 2 * cs[c]; if (v < 60) continue; }   // an earned trait only for a strong personality
        v += (int)(mix(p->seed, (u32)i) % 24u);
        for (k = 0; k < ninh; k++) if (inherit[k] == i) v += 40;
        sc[i] = (s16)v;
    }
    for (int slot = 0; slot < nslots; slot++) {
        if (tid[slot]) continue;
        int best = 0, bv = -9000;
        for (i = 1; i < TR_FIRST_RWD + TR_NRWD; i++) {
            if (sc[i] <= bv || sc[i] <= -9000) continue;
            int c = tr_cat_of(i), ok = ncat[c] < 3, isinh = 0;
            for (k = 0; k < ninh; k++) isinh |= inherit[k] == i;
            if (isinh && ninhn >= 2) ok = 0;
            for (k = 0; ok && k < TR_SLOTS; k++) if (tid[k] && (tid[k] == i || tr_clash(i, tid[k]))) ok = 0;
            if (ok) { best = i; bv = sc[i]; }
        }
        if (!best) break;
        { int isinh = 0; for (k = 0; k < ninh; k++) isinh |= inherit[k] == best; ninhn += isinh; }
        tid[slot] = (u16)best; ncat[tr_cat_of(best)]++; added++;
    }
    return added;
}
