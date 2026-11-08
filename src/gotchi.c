#pragma GCC optimize("Os")
// gotchi.c - see gotchi.h. Every game minute each creature ages, gets hungry, makes a mess, falls asleep at night and calls for you;
// leave a call too long and that is a CARE MISTAKE. How it grows up depends on the mistakes, its discipline, its snacks, its weight and its BOND.
// FAMILIES: grown-ups who love each other become a pair and lay eggs; the egg inherits kind, colour, personality and rare mutations from
// both; parents look after their young, brothers and sisters play, housemates' pets visit; the old pass on and leave a memorial and a tree.
// (Kept small: it is resident. The names, the foods and the hands-on actions are in the overlay, gotchi_act.c.)
#include "gotchi.h"
#include "sim.h"
#include "save.h"

Toy gtoy[GT_MAX];
Line gline[GL_N];
Diary gdiary[GD_N];
u8 gtal[GL_N], gbest[8];
u32 gach;
int sim_want_gotchi;
int gotchi_time;
static u32 dex[9];   // 0..127: sp * 6 + form seen; 128..: sp * 8 + colour seen
static u32 grng = 0x1234567u;
u16 gt_h, gt_done;
u8 gsum_pend, gsum_c, gsum_p, gsum_h;
static u16 d_cared, d_plays, d_hatched; static int d_ok, d_sum = -1;   // what the day started with (the 21:00 summary is the difference)
static u8 dn;                                        // diary entries written (it wraps round GD_N)
static int gr(void) { grng = grng * 1103515245u + 12345u; return (int)((grng >> 16) & 255); }

void gotchi_reset(void) { gt_h = gt_done = 0; gsum_pend = 0; d_ok = 0; d_sum = -1; memset(gtoy, 0, sizeof gtoy); memset(gline, 0, sizeof gline); memset(gdiary, 0, sizeof gdiary); memset(gtal, 0, sizeof gtal); memset(gbest, 0, sizeof gbest); memset(dex, 0, sizeof dex); gach = 0; dn = 0; }
static int dbit(int i, int set) { u32 *w = &dex[i >> 5]; if (set) *w |= 1u << (i & 31); return (*w >> (i & 31)) & 1; }   // dex[] then cdex[] are one run of bits
void gotchi_dex_see(int sp, int form) { dbit(sp * GFORMS + form, 1); }
int gotchi_dex_has(int sp, int form) { return dbit(sp * GFORMS + form, 0); }
int gotchi_dex_count(void) { int n = 0; for (int i = 0; i < GSP_N * GFORMS; i++) n += dbit(i, 0); return n; }
void gotchi_cdex_see(int sp, int col) { dbit(128 + sp * 8 + col, 1); }
int gotchi_cdex_has(int sp, int col) { return dbit(128 + sp * 8 + col, 0); }

Toy *gotchi_toy(int simi) { if (simi < 0 || simi >= nsims) return 0; for (int i = 0; i < GT_MAX; i++) if (gtoy[i].own && gtoy[i].uid == sims[simi].uid) return &gtoy[i]; return 0; }
int gotchi_sim_of(const Toy *t) { for (int i = 0; i < nsims; i++) if (sims[i].uid == t->uid) return i; return -1; }
int gotchi_any(void) { for (int i = 0; i < GT_MAX; i++) if (gtoy[i].own && gotchi_sim_of(&gtoy[i]) >= 0) return 1; return 0; }
int gotchi_count(const Toy *t) { int n = 0; for (int i = 0; i < GB_N; i++) n += t->b[i].on; return n; }
int gotchi_needs(const Blob *b) {
    return b->on && b->stage > GS_EGG && (b->hun == 0 || b->hap == 0 || b->poop || (b->fl & (GF_SICK | GF_FAKE | GF_TANTRUM)) || ((b->fl & GF_SLEEP) && !(b->fl & GF_DARK)));
}
int gotchi_calling(const Toy *t) { int n = 0; for (int i = 0; i < GB_N; i++) n += gotchi_needs(&t->b[i]); return n; }
static const u16 xpAt[6] = { 0, 30, 100, 250, 500, 900 };
int gotchi_level(const Toy *t) { int l = 0; for (int i = 1; i < 6; i++) if (t->xp >= xpAt[i]) l = i; return l; }
int gotchi_next_xp(const Toy *t) { int l = gotchi_level(t); return l >= 5 ? 0 : xpAt[l + 1]; }
void gotchi_xp(Toy *t, int n) {
    { int si = gotchi_sim_of(t); if (si >= 0) n = tr_scale(sims[si].tr, E_GOTCHI_XP, n); }   // E_GOTCHI_XP
    int l0 = gotchi_level(t); t->xp = (u16)MIN(60000, t->xp + n); t->cared++;
    if (gotchi_level(t) > l0) { char b[40]; sprintf(b, "BLOB CARE LEVEL %d", gotchi_level(t)); sim_note(b); }
}
void gotchi_mood(const Toy *t, int kind) { int i = gotchi_sim_of(t); if (i >= 0) sim_gotchi_mood(i, kind); }
void gotchi_diary(int kind, int id) { Diary *d = &gdiary[dn++ % GD_N]; d->day = (u16)(clock_day * 24 + clock_min / 60); d->kind = (u8)kind; d->id = (u8)id; }

// ---- lines: who is whose ----
Line *gotchi_line(int id) { if (id) for (int i = 0; i < GL_N; i++) if (gline[i].id == id) return &gline[i]; return 0; }
static Line *line_new(void) {
    static u8 nid; int k = -1;
    for (int i = 0; i < GL_N && k < 0; i++) if (!gline[i].id) k = i;
    for (int i = 0; i < GL_N && k < 0; i++) if (!(gline[i].fc & 128)) k = i;      // the memorials of the oldest are forgotten first
    if (k < 0) return 0;
    do nid = (u8)(nid % 250 + 1); while (gotchi_line(nid));
    memset(&gline[k], 0, sizeof(Line)); gline[k].id = nid; return &gline[k];
}
int gotchi_slot_of(const Toy *t, int id) { if (id) for (int i = 0; i < GB_N; i++) if (t->b[i].on && t->k[i].id == id) return i; return -1; }

// ---- a new creature: the egg (the overlay hands down the parents' traits) ----
static void traits(Kin *k) { int l = gr() % 12, d = gr() % 11; k->pers = (u8)(gr() & 7); k->col = (u8)(gr() % 7); k->taste = (u8)(l << 4 | (d >= l ? d + 1 : d)); k->bday = (u8)(gt_h % 168); }
void gotchi_xinit(Toy *t, int slot) { Xt *x = &t->x[slot]; memset(x, 0, sizeof *x); x->nrg = 90; x->hyg = 100; }
static void fresh(Toy *t, int slot, int sp) {
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot];
    memset(b, 0, sizeof *b); memset(k, 0, sizeof *k); b->on = 1; b->sp = (u8)(sp % 16); b->nm = (u8)(gr() & 31); b->hun = 4; b->hap = 4; b->wt = 5;
    traits(k); gotchi_xinit(t, slot);
    Line *l = line_new();
    if (l) { l->sp = b->sp; l->nm = b->nm; l->fc = (u8)(128 | k->col << 3); l->pg = k->pers; k->id = l->id; }
}
int gotchi_buy(int simi) {
    if (simi < 0 || simi >= nsims || gotchi_toy(simi)) return 0;
    int k = -1; for (int i = 0; i < GT_MAX; i++) if (!gtoy[i].own || gotchi_sim_of(&gtoy[i]) < 0) { k = i; break; }
    if (k < 0) return 0;
    Toy *t = &gtoy[k]; memset(t, 0, sizeof *t); t->own = 1; t->uid = sims[simi].uid; t->room = (u8)(gr() % 6);
    fresh(t, 0, gr()); if (simi == player) { d_cared = d_plays = d_hatched = 0; d_ok = 1; } return 1;
}
int gotchi_egg(Toy *t, int slot) { if (slot < 0 || slot >= GB_N || t->b[slot].on) return 0; fresh(t, slot, gr()); return 1; }

// ---- one game minute of one creature ----
static int night(void) { return clock_min >= 20 * 60 || clock_min < 6 * 60; }
static void pass_on(Toy *t, int slot, int home) {
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot]; Line *l = gotchi_line(k->id);
    if (l) l->fc = (u8)((l->fc & ~128) | 64 | (b->stage >= GS_ADULT ? b->form : 0));
    b->on = 0; if (k->mate) t->k[k->mate - 1].mate = 0; gotchi_diary(GD_PASS, k->id);
    for (int i = 0; i < GB_N; i++) if (t->b[i].on && t->b[i].stage > GS_EGG) { if (t->b[i].hap > 1) t->b[i].hap--; if (t->x[i].bff == slot + 1) { t->x[i].bff = 0; t->x[i].fnd = 0; t->b[i].hap = 0; gotchi_diary(GD_MISS, t->k[i].id); } }   // the family grieves; a best friend most of all
    gotchi_mood(t, GM_MISS); sim_note(home ? "A BOREAGOTCHI WENT BACK TO ITS HOME PLANET" : "A BOREAGOTCHI PASSED ON. ITS FAMILY REMEMBERS");
}
void gotchi_hatch(Toy *t, int slot) {
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot]; Line *l = gotchi_line(k->id); b->stage = GS_BABY; b->age = 0; b->hun = 3; b->hap = 3; gotchi_dex_see(b->sp, 0); gotchi_cdex_see(b->sp, k->col); t->hatched++; gotchi_mood(t, GM_HATCH);
    if (l) { l->sp = b->sp; l->fc = (u8)((l->fc & 0xC0) | k->col << 3); }
    gotchi_diary(GD_HATCH, k->id); sim_note(k->mut || b->sp >= 16 ? "A RARE BOREAGOTCHI HATCHED!" : "A BOREAGOTCHI HATCHED!");
}
static void tick(Toy *t, int slot) {
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot]; if (!b->on) return;
    if (b->age < 65000) b->age++;
    if (b->stage == GS_EGG) { if (b->age >= 20) gotchi_hatch(t, slot); return; }
    Xt *x = &t->x[slot];
    int n = night(), sl = (b->fl & GF_SLEEP) != 0, p = k->pers, st = b->stage;
    if (!sl && st >= GS_CHILD && (n || x->nrg < 8)) b->fl |= GF_SLEEP;                                  // night, or worn out: a nap by day
    else if (sl && !n && x->nrg >= (p == GP_LAZY ? 90 : 60)) b->fl &= (u8)~(GF_SLEEP | GF_DARK);      // rested (the lazy sleep in)
    sl = (b->fl & GF_SLEEP) != 0;
    int lv = gotchi_level(t);
    {   // the new needs: energy (sleep refills it, being up and hungry drains it) and cleanness (mess and mud wear it down); each stage has its own pace
        static const u8 nP[6] = { 0, 16, 9, 11, 12, 8 }, hP[6] = { 0, 14, 18, 20, 26, 20 };
        if (sl) { if (x->nrg < 100 && b->hun && ((b->fl & GF_DARK) || (b->age & 1))) x->nrg++; if ((x->wish & 7) == 6) { x->wish = 0; if (b->bond < 100) b->bond++; } }
        else { if (x->nrg && st > GS_BABY && (b->age % nP[st] == 0 || (!b->hun && b->age % 6 == 0))) x->nrg--; if (x->hyg && b->age % hP[st] == 0) x->hyg--; }   // (a baby never tires)
        if (b->poop && x->hyg && b->age % 6 == 0) x->hyg--;
        if (b->age % 60 == 0 && x->lone < 255) x->lone++;
        if (x->lone >= (p == GP_PLAYFUL ? 6 : p == GP_SHY ? 20 : 12) && !sl) { if (b->age % 45 == 0 && b->hap) b->hap--; if (b->age % 180 == 0 && b->bond) b->bond--; }   // lonely
        if (x->hyg < 25 && b->age % 120 == 0 && b->bond) b->bond--;
        if (st == GS_TEEN && b->age % 360 == 180 && b->disc) b->disc--;                                  // teens push back
        if (!(x->wish & 7)) { if (!sl && b->age % 80 == 40 && !(gr() & 3)) { static const u8 pw[8] = { 5, 4, 1, 6, 2, 3, 2, 4 }; int w = (gr() & 1) ? pw[p] : 1 + gr() % 6; if (st == GS_BABY && w != 1) w = 5; x->wish = (u8)w; } }
        else if (b->age % 60 == 0) { x->wish += 8; if (x->wish >= 80) { x->wish = 0; if (b->bond) b->bond--; } }   // a wish unmet for ten hours is forgotten
    }
    int rate = (b->stage == GS_BABY ? 30 : b->stage == GS_CHILD ? 45 : 60) + lv * 4; if (p == GP_GREEDY) rate = rate * 3 / 4; if (sl) rate *= 3;
    int hrate = rate + 10; if (p == GP_PLAYFUL) hrate -= 12; if (p == GP_LAZY) hrate += 15; if (x->nrg < 20 || x->hyg < 20) hrate = hrate * 2 / 3;   // tired or grubby: grumpier
    if (b->age % rate == 0 && b->hun) b->hun--;
    if (b->age % hrate == 0 && b->hap) b->hap--;
    if (!sl && b->meal && b->age % 90 == 45 && b->poop < 3) b->poop++;
    if ((b->poop || x->hyg < 20) && b->age % 60 == 30 && ((gr() * 100) >> 8) < (b->poop ? 35 : 18) - lv * 4 - (p == GP_GENTLE ? 12 : 0)) b->fl |= GF_SICK;
    if (!(b->fl & GF_FAKE) && !gotchi_needs(b) && !sl && b->age % 150 == 75 && ((gr() & 1) || (p == GP_CHEEKY && (gr() & 1)))) b->fl |= GF_FAKE;
    if (b->hap == 0 && !sl && !(b->fl & GF_TANTRUM) && (p == GP_CHEEKY || p == GP_BOLD || st == GS_TEEN) && !(gr() & 31)) { b->fl |= GF_TANTRUM; gotchi_diary(GD_TANTRUM, k->id); }
    if (gotchi_needs(b)) { b->fl |= GF_CALL; if (++b->needT >= 15 + lv * 2) { b->needT = 0; if (b->miss < 99) b->miss++; if (b->bond) b->bond--; b->fl &= (u8)~GF_FAKE; gotchi_mood(t, GM_WORRY); } }
    else { b->fl &= (u8)~GF_CALL; b->needT = 0; if (!sl && b->hun >= 3 && b->hap >= 3 && b->age % (p == GP_GENTLE ? 90 : 120) == 0 && b->bond < 100) b->bond++; }
    static const u16 grow[5] = { 0, 240, 720, 1440, 2880 };
    if (b->stage < GS_ELDER && b->age >= grow[b->stage]) {
        b->stage++; b->age = 0; if (b->stage == GS_CHILD) b->form &= 127;   // (the mystery egg's mark, form bit 7 on an egg, stays through the baby and is dropped here)
        if (b->stage == GS_TEEN) b->form = b->miss <= 2 ? 0 : 1;
        if (b->stage == GS_ADULT) {
            b->form = (b->miss <= 1 && b->bond >= 80 && b->disc >= 50) ? 3 : (b->snk > b->meal && b->snk >= 6) ? 2 : (b->wt >= 22 && b->miss <= 3) ? 5 : (b->miss <= 2 && b->disc >= 50 && b->wt <= 14) ? 4 : (b->miss <= 2 && b->disc >= 50) ? 0 : 1;
            gotchi_dex_see(b->sp, b->form); { Line *l = gotchi_line(k->id); if (l) { int s2 = x->sk[0] >= x->sk[1] ? (x->sk[0] >= x->sk[2] ? 0 : 2) : (x->sk[1] >= x->sk[2] ? 1 : 2); l->fc = (u8)((l->fc & 0xF8) | b->form); gtal[l - gline] = (u8)(x->sk[s2] > 9 ? (s2 + 1) | (x->sk[s2] / 4) << 2 : 0); } }
            gotchi_diary(GD_ADULT | b->form << 5, k->id);
            if (b->form == 0 || b->form == 3 || b->form == 4) { gotchi_mood(t, GM_PROUD); sim_note(b->form == 3 ? "A RARE STAR FORM! WHAT A PROUD PARENT" : "A SUPER BORE! WHAT A PROUD PARENT"); }
            else sim_note("YOUR BOREAGOTCHI GREW UP");
        } else { if (b->stage == GS_ELDER) sim_note("A BOREAGOTCHI IS GROWING OLD. TREASURE IT"); gotchi_diary(GD_GROW | b->stage << 5, k->id); }
        b->miss = 0;
    }
    if (b->stage == GS_ELDER && b->age >= 1440 + b->bond * 6 + (b->form == 3 ? 720 : 0) + lv * 40) { pass_on(t, slot, 0); return; }
    if (b->hun == 0 && b->hap == 0 && (b->fl & GF_SICK) && b->miss >= 12) pass_on(t, slot, 1);
}
// parents look after their young
static void kin_care(Toy *t) {
    for (int i = 0; i < GB_N; i++) {
        Blob *b = &t->b[i]; if (!b->on || b->stage < GS_BABY || b->stage > GS_TEEN || (b->fl & GF_SLEEP) || !gotchi_needs(b) || (gr() & 7)) continue;
        const Line *l = gotchi_line(t->k[i].id); if (!l) continue;
        for (int w = 0; w < 2; w++) { int s = gotchi_slot_of(t, w ? l->dad : l->mom); if (s < 0) continue; Blob *p = &t->b[s]; if (p->stage < GS_TEEN || (p->fl & GF_SLEEP) || p->hun == 0) continue;
            if (b->hun == 0) b->hun = 2; else if (b->poop) b->poop = 0; else if (b->fl & GF_SICK) b->fl &= (u8)~GF_SICK; else if (b->fl & GF_TANTRUM) b->fl &= (u8)~GF_TANTRUM; else if (b->hap == 0) b->hap = 2; else break;
            if (b->bond < 100) b->bond++; if (p->hap < 4 && !(gr() & 3)) p->hap++; if (!(gr() & 15)) gotchi_diary(GD_CARED, t->k[i].id); break; }
    }
}
static int kinok(int si) { return sim_gotchi_stat(si, 1) >= 20 && sim_gotchi_stat(si, 0) > -40; }   // is that Bore up to caring for anyone?
static void pets_minute(void) {
    if (clock_min % 60 == 0) gt_h++;
    { Toy *me = gotchi_toy(player);   // a gentle end of day: what was done, written in the diary and shown once on the toy
      if (me && clock_min == 6 * 60) { d_cared = me->cared; d_plays = me->plays; d_hatched = me->hatched; d_ok = 1; d_sum = -1; }
      if (me && d_ok && clock_min == 21 * 60 && d_sum != clock_day) { d_sum = clock_day; gsum_c = (u8)MIN(255, me->cared - d_cared); gsum_p = (u8)MIN(255, me->plays - d_plays); gsum_h = (u8)MIN(255, me->hatched - d_hatched); gsum_pend = 1; gotchi_diary(GD_DAY, gsum_c); } }
    for (int k = 0; k < GT_MAX; k++) {
        Toy *t = &gtoy[k]; if (!t->own) continue; int si = gotchi_sim_of(t); if (si < 0) continue;
        for (int i = 0; i < GB_N; i++) {
            Blob *b = &t->b[i]; if (!b->on) continue;
            tick(t, i); if (!b->on) continue;
            { int food = sim_gotchi_stat(si, 1), mood = sim_gotchi_stat(si, 0);   // an owner who starves or is miserable passes it on; a happy one lifts the pet
              if (b->stage > GS_EGG && !(b->fl & GF_SLEEP)) {
                  if (food < 15 && !(gr() & 15) && b->hun) { b->hun--; if (si == player && !(gr() & 7)) sim_note("YOUR PET IS HUNGRY TOO"); }
                  if (mood < -40 && b->age % 25 == 0 && b->hap) b->hap--;
                  if (mood > 50 && b->age % 40 == 0) { if (b->hap < 4) b->hap++; if (b->bond < 100 && !(gr() & 3)) b->bond++; } } }
            if (si != player && kinok(si) && gotchi_needs(b) && !(gr() & 15)) { b->hun |= !b->hun; b->poop = 0; b->hap |= !b->hap; b->fl &= (u8)~(GF_SICK | GF_FAKE | GF_TANTRUM); if (t->x[i].hyg < 30) t->x[i].hyg = 60; if (t->x[i].nrg < 20 && !(b->fl & GF_SLEEP)) t->x[i].nrg = 40; }   // a housemate looks after their own
        }
        kin_care(t);
    }
    for (int k = 0; k < GT_MAX; k++) {   // and the other way: pets in a bad way drag their owner down, happy ones lift them
        Toy *t = &gtoy[k]; if (!t->own) continue; int si = gotchi_sim_of(t); if (si < 0 || clock_min % 60) continue;
        int n = 0, glum = 0, glad = 0; for (int i = 0; i < GB_N; i++) { Blob *b = &t->b[i]; if (!b->on || b->stage == GS_EGG) continue; n++; if (b->hun == 0 && b->hap == 0) glum++; else if (b->hap >= 3 && b->hun >= 2) glad++; }
        if (glum) sim_gotchi_fun(si, -3 * glum); else if (n >= 2 && glad == n) sim_gotchi_fun(si, 3);
    }
}
void gotchi_minute(void) {
    if (gotchi_time != 2) pets_minute();
    static int beep; Toy *me = gotchi_toy(player);   // (it does not nag: one beep per three game hours, never in the night, and not for a fib)
    if (!me || !gotchi_calling(me)) beep = 0;
    else if (gotchi_time != 2 && ++beep >= 180 && !night()) { beep = 0; sim_note("YOUR BOREAGOTCHI WANTS YOU"); }
}
void gotchi_live(int steps) {
    if (gotchi_time != 1) return; static int a, b; a += steps; b += steps;
    while (a >= 60) { a -= 60; pets_minute(); }
    while (b >= 300) { b -= 300; sim_gotchi_absorb(player); }
}

// ---- the save: only the toys that exist, only the creatures that live (a quarter of what it was). v48 adds each creature's Xt, each toy's goals, the lines' talents, the best scores and a longer diary ----
void gotchi_save(void) {
    int nt = 0, nl = 0; for (int k = 0; k < GT_MAX; k++) nt += gtoy[k].own != 0; for (int i = 0; i < GL_N; i++) nl += gline[i].id != 0;
    for (int i = 0; i < 9; i++) sv_u32(dex[i]); sv_u32(gach); sv_u8(dn); sv_u8(nt);
    for (int k = 0; k < GT_MAX; k++) { Toy *t = &gtoy[k]; if (!t->own) continue; int m = 0; for (int i = 0; i < GB_N; i++) m |= t->b[i].on << i;
        sv_u8(t->uid); sv_u8(t->sel); sv_u8(t->room); sv_u8(t->deco); sv_u8(t->accs); sv_u16(t->xp); sv_u16(t->hatched); sv_u16(t->wins); sv_u16(t->cared); sv_u16(t->plays); sv_u8(m);
        for (int i = 0; i < GB_N; i++) if (m >> i & 1) { sv_n(&t->b[i], sizeof(Blob)); sv_n(&t->k[i], sizeof(Kin)); sv_n(&t->x[i], sizeof(Xt)); }
        sv_n(t->gp, 6); sv_u8(t->streak); sv_u8(t->best); sv_u8(t->gflag); sv_u8(t->ghist); sv_u16(t->gday); sv_u16(t->gwon); }
    sv_u8(nl); for (int i = 0; i < GL_N; i++) if (gline[i].id) sv_n(&gline[i], sizeof(Line));
    sv_n(gdiary, sizeof gdiary); sv_n(gtal, sizeof gtal); sv_n(gbest, sizeof gbest);
}
static void upgrade(Toy *t) {   // an old Boreagotchi: kin and a line for each creature that lives
    for (int i = 0; i < GB_N; i++) { Blob *b = &t->b[i]; if (!b->on) continue; Kin *k = &t->k[i]; memset(k, 0, sizeof *k); traits(k); if (b->stage == GS_ADULT && b->age > 900) b->age = 900;   // (a grown-up from before: years of adulthood ahead)
       
        Line *l = line_new(); if (l) { l->sp = b->sp; l->nm = b->nm; l->fc = (u8)(128 | k->col << 3 | (b->stage >= GS_ADULT ? b->form & 7 : 0)); l->pg = (u8)(k->pers | MIN(31, b->gen) << 3); k->id = l->id; }
        if (b->stage > GS_EGG) gotchi_cdex_see(b->sp, k->col); }
}
void gotchi_load(int ver) {
    gotchi_reset(); if (ver < 21) return;
    if (ver >= 45) {
        for (int i = 0; i < 9; i++) dex[i] = ld_u32(); gach = ld_u32(); dn = (u8)ld_u8(); int nt = ld_u8();
        for (int n = 0; n < nt && n < GT_MAX; n++) { Toy *t = &gtoy[n]; t->own = 1; t->uid = (u8)ld_u8(); t->sel = (u8)ld_u8(); t->room = (u8)ld_u8(); t->deco = (u8)ld_u8(); t->accs = (u8)ld_u8(); t->xp = (u16)ld_u16(); t->hatched = (u16)ld_u16(); t->wins = (u16)ld_u16(); t->cared = (u16)ld_u16(); t->plays = (u16)ld_u16(); int m = ld_u8();
            for (int i = 0; i < GB_N; i++) if (m >> i & 1) { ld_n(&t->b[i], sizeof(Blob)); ld_n(&t->k[i], sizeof(Kin)); if (ver >= 48) ld_n(&t->x[i], sizeof(Xt)); else { gotchi_xinit(t, i); t->x[i].nrg = 80; t->x[i].hyg = 80; } }
            if (ver >= 48) { ld_n(t->gp, 6); t->streak = (u8)ld_u8(); t->best = (u8)ld_u8(); t->gflag = (u8)ld_u8(); t->ghist = (u8)ld_u8(); t->gday = (u16)ld_u16(); t->gwon = (u16)ld_u16(); } }
        int nl = ld_u8(); for (int i = 0; i < nl && i < GL_N; i++) ld_n(&gline[i], sizeof(Line));
        if (ver >= 48) { ld_n(gdiary, sizeof gdiary); ld_n(gtal, sizeof gtal); ld_n(gbest, sizeof gbest); }
        else { Diary od[16]; ld_n(od, sizeof od); for (int i = 1; i <= 16 && i <= dn; i++) gdiary[(dn - i) % GD_N] = od[(dn - i) % 16]; }   // (the old ring of 16, laid into the longer one so the newest stays the newest)
        return;
    }
    u32 od[2]; od[0] = ld_u32(); od[1] = ld_u32();
    for (int sp = 0; sp < 16; sp++) for (int f = 0; f < 4; f++) { int i = sp * 4 + f; if ((od[i >> 5] >> (i & 31)) & 1) gotchi_dex_see(sp, f); }
    for (int k = 0; k < (ver >= 22 ? GT_MAX : 6); k++) { Toy *t = &gtoy[k]; t->own = (u8)ld_u8(); t->uid = (u8)ld_u8(); t->sel = (u8)ld_u8(); t->xp = (u16)ld_u16(); t->hatched = (u16)ld_u16(); t->wins = (u16)ld_u16(); t->cared = (u16)ld_u16();
        for (int i = 0; i < GB_N; i++) ld_n(&t->b[i], sizeof(Blob));
        if (t->own) { t->room = (u8)(k % 6); upgrade(t); for (int i = 0; i < GB_N; i++) { gotchi_xinit(t, i); t->x[i].nrg = 80; t->x[i].hyg = 80; } } }
}
const char *gotchi_house(int simi, int what) {
    Toy *t = gotchi_toy(simi); if (!t) return "NO BOREAGOTCHI";
    int n = 0;
    for (int i = 0; i < GB_N; i++) { Blob *b = &t->b[i]; if (!b->on || b->stage == GS_EGG) continue;
        if (what == 0) { if (b->hun < 4 && !(b->fl & GF_SLEEP)) { b->hun++; if (b->wt < 99) b->wt++; if (b->meal < 250) b->meal++; n++; } }
        else if (what == 1) { b->fl ^= GF_DARK; n++; }
        else if ((b->fl & GF_SLEEP) && !(b->fl & GF_DARK)) { b->fl |= GF_DARK; if (b->bond < 100) b->bond++; n++; } }
    if (n) gotchi_xp(t, 1);
    return what == 0 ? (n ? "THEY ALL TUCKED IN. NOM NOM" : "NOBODY IS HUNGRY") : what == 1 ? "CLICK. LIGHTS CHANGED" : (n ? "TUCKED IN. SWEET DREAMS" : "NOBODY NEEDS TUCKING IN");
}
int gotchi_diary_pos(void) { return dn; }
