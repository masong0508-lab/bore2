// gotchi.c - see gotchi.h. Every game minute each creature ages, gets hungry, makes a mess, falls asleep at night and calls for you;
// leave a call too long and that is a CARE MISTAKE. How it grows up depends on the mistakes, its discipline, its snacks and its BOND.
#include "gotchi.h"
#include "sim.h"
#include "save.h"

Toy gtoy[GT_MAX];
int sim_want_gotchi;
int gotchi_time;
static u32 dex[2];
static u32 grng = 0x1234567u;
static int gr(void) { grng = grng * 1103515245u + 12345u; return (int)((grng >> 16) & 255); }

static const char *const spNm[GSP_N] = { "BLOB", "CAT", "BUNNY", "BIRD", "FROG", "DINO", "FISH", "BEAR", "GHOST", "ROBOT", "SHROOM", "SLIME", "BAT", "SNAIL", "OCTO", "ALIEN" };
static const char *const giv[32] = { "BLOBERT", "SNORLOCK", "MEH-CHAN", "DOZY", "SIR SIGH", "YAWNIE", "PUDDLE", "KEVIN", "NIBBLES", "GRUMP", "WOBBLE", "SPROCKET",
    "TOASTY", "MOCHI", "BEANS", "ZIGGY", "PICKLE", "DUMPLING", "FUZZ", "CLANK", "BOOP", "SQUISH", "MUNCH", "BISCUIT", "GLORP", "NOODLE", "PIP", "WAFFLES", "TWIG", "DOUGH", "ORBIT", "SNOOZE" };
static const char *const formPre[3][4] = { { "", "", "", "" }, { "ANGSTY ", "SLOUCHY ", "", "" }, { "SUPER ", "SLOB ", "CHILL ", "STAR " } };
const char *gotchi_given(int nm) { return giv[nm & 31]; }
const char *gotchi_species(int sp) { return spNm[sp % GSP_N]; }
void gotchi_name(const Blob *b, char *out) {
    const char *pre = b->stage == GS_ADULT ? formPre[2][b->form & 3] : b->stage == GS_TEEN ? formPre[1][b->form & 1] : "";
    const char *sp = b->stage == GS_EGG ? "EGG" : spNm[b->sp % GSP_N];
    const char *tail = b->stage == GS_BABY ? " BABY" : "";
    strcpy(out, pre); strcat(out, sp); strcat(out, tail);
}

void gotchi_reset(void) { memset(gtoy, 0, sizeof gtoy); dex[0] = dex[1] = 0; }
void gotchi_dex_see(int sp, int form) { int i = (sp % GSP_N) * 4 + (form & 3); dex[i >> 5] |= 1u << (i & 31); }
int gotchi_dex_has(int sp, int form) { int i = (sp % GSP_N) * 4 + (form & 3); return (dex[i >> 5] >> (i & 31)) & 1; }
int gotchi_dex_count(void) { int n = 0; for (int i = 0; i < GSP_N * 4; i++) n += (dex[i >> 5] >> (i & 31)) & 1; return n; }

Toy *gotchi_toy(int simi) { if (simi < 0 || simi >= nsims) return 0; for (int i = 0; i < GT_MAX; i++) if (gtoy[i].own && gtoy[i].uid == sims[simi].uid) return &gtoy[i]; return 0; }
int gotchi_sim_of(const Toy *t) { for (int i = 0; i < nsims; i++) if (sims[i].uid == t->uid) return i; return -1; }
int gotchi_any(void) { for (int i = 0; i < GT_MAX; i++) if (gtoy[i].own && gotchi_sim_of(&gtoy[i]) >= 0) return 1; return 0; }
int gotchi_count(const Toy *t) { int n = 0; for (int i = 0; i < GB_N; i++) n += t->b[i].on; return n; }
int gotchi_needs(const Blob *b) {
    return b->on && b->stage > GS_EGG && (b->hun == 0 || b->hap == 0 || b->poop || (b->fl & GF_SICK) || ((b->fl & GF_SLEEP) && !(b->fl & GF_DARK)) || (b->fl & GF_FAKE));
}
int gotchi_calling(const Toy *t) { int n = 0; for (int i = 0; i < GB_N; i++) n += gotchi_needs(&t->b[i]); return n; }
const char *gotchi_want(const Blob *b) {
    if (b->stage == GS_EGG) return "STILL AN EGG";
    return b->hun == 0 ? "IT IS HUNGRY" : b->hap == 0 ? "IT IS BORED" : b->poop ? "IT MADE A MESS" : (b->fl & GF_SICK) ? "IT FEELS SICK" :
           ((b->fl & GF_SLEEP) && !(b->fl & GF_DARK)) ? "TURN THE LIGHT OFF" : (b->fl & GF_FAKE) ? "IT IS FIBBING. SCOLD IT" : "ALL GOOD";
}
static const u16 xpAt[6] = { 0, 30, 100, 250, 500, 900 };
static const char *const lvNm[6] = { "NEW PARENT", "CARING", "ATTENTIVE", "BLOB WHISPERER", "BREEDER", "GOTCHI MASTER" };
int gotchi_level(const Toy *t) { int l = 0; for (int i = 1; i < 6; i++) if (t->xp >= xpAt[i]) l = i; return l; }
const char *gotchi_level_name(int lv) { return lvNm[lv % 6]; }
int gotchi_next_xp(const Toy *t) { int l = gotchi_level(t); return l >= 5 ? 0 : xpAt[l + 1]; }
static void xp(Toy *t, int n) {
    int l0 = gotchi_level(t); t->xp = (u16)MIN(60000, t->xp + n); t->cared++;
    if (gotchi_level(t) > l0) { char b[40]; sprintf(b, "BLOB CARE LEVEL %d: %s", gotchi_level(t), lvNm[gotchi_level(t)]); sim_note(b); }
}
void gotchi_mood(const Toy *t, int kind) { int i = gotchi_sim_of(t); if (i >= 0) sim_gotchi_mood(i, kind); }

static void fresh(Blob *b, int sp) { memset(b, 0, sizeof *b); b->on = 1; b->sp = (u8)(sp % GSP_N); b->nm = (u8)(gr() & 31); b->hun = 4; b->hap = 4; b->wt = 5; }
int gotchi_buy(int simi) {
    if (simi < 0 || simi >= nsims || gotchi_toy(simi)) return 0;
    int k = -1; for (int i = 0; i < GT_MAX; i++) if (!gtoy[i].own || gotchi_sim_of(&gtoy[i]) < 0) { k = i; break; }
    if (k < 0) return 0;
    Toy *t = &gtoy[k]; memset(t, 0, sizeof *t); t->own = 1; t->uid = sims[simi].uid;
    fresh(&t->b[0], gr()); return 1;
}
int gotchi_egg(Toy *t, int slot) { if (slot < 0 || slot >= GB_N || t->b[slot].on) return 0; fresh(&t->b[slot], gr()); return 1; }

// ---- one game minute of one creature ----
static int night(void) { return clock_min >= 20 * 60 || clock_min < 6 * 60; }
static void tick(Toy *t, Blob *b) {
    if (!b->on) return;
    if (b->age < 65000) b->age++;
    if (b->stage == GS_EGG) { if (b->age >= 20) { b->stage = GS_BABY; b->age = 0; b->hun = 3; b->hap = 3; gotchi_dex_see(b->sp, 0); t->hatched++; gotchi_mood(t, GM_HATCH); sim_note("A BOREAGOTCHI HATCHED!"); } return; }
    int n = night(), sl = (b->fl & GF_SLEEP) != 0;
    if (n && !sl && b->stage >= GS_CHILD) b->fl |= GF_SLEEP; else if (!n && sl) b->fl &= (u8)~(GF_SLEEP | GF_DARK);
    sl = (b->fl & GF_SLEEP) != 0;
    int lv = gotchi_level(t);
    int rate = (b->stage == GS_BABY ? 30 : b->stage == GS_CHILD ? 45 : 60) + lv * 4; if (sl) rate *= 3;   // a good carer's creatures stay full for longer
    if (b->age % rate == 0 && b->hun) b->hun--;
    if (b->age % (rate + 10) == 0 && b->hap) b->hap--;
    if (!sl && b->meal && b->age % 90 == 45 && b->poop < 3) b->poop++;
    if (b->poop && b->age % 60 == 30 && ((gr() * 100) >> 8) < 35 - lv * 4) b->fl |= GF_SICK;
    if (!(b->fl & GF_FAKE) && !gotchi_needs(b) && !sl && b->age % 150 == 75 && (gr() & 1)) b->fl |= GF_FAKE;
    if (gotchi_needs(b)) { b->fl |= GF_CALL; if (++b->needT >= 15 + lv * 2) { b->needT = 0; if (b->miss < 99) b->miss++; if (b->bond) b->bond--; b->fl &= (u8)~GF_FAKE; gotchi_mood(t, GM_WORRY); } }
    else { b->fl &= (u8)~GF_CALL; b->needT = 0; if (!sl && b->hun >= 3 && b->hap >= 3 && b->age % 120 == 0 && b->bond < 100) b->bond++; }
    static const u16 grow[5] = { 0, 240, 720, 1440, 0 };
    if (b->stage < GS_ADULT && b->age >= grow[b->stage]) {
        b->stage++; b->age = 0;
        if (b->stage == GS_TEEN) b->form = b->miss <= 2 ? 0 : 1;
        if (b->stage == GS_ADULT) {
            b->form = (b->miss <= 1 && b->bond >= 80 && b->disc >= 50) ? 3 : (b->snk > b->meal && b->snk >= 6) ? 2 : (b->miss <= 2 && b->disc >= 50) ? 0 : 1;
            gotchi_dex_see(b->sp, b->form);
            if (b->form == 0 || b->form == 3) { gotchi_mood(t, GM_PROUD); sim_note(b->form == 3 ? "A RARE STAR FORM! WHAT A PROUD PARENT" : "A SUPER BORE! WHAT A PROUD PARENT"); }
            else sim_note("YOUR BOREAGOTCHI GREW UP");
        }
        b->miss = 0;
    }
    if (b->hun == 0 && b->hap == 0 && (b->fl & GF_SICK) && b->miss >= 12) { b->on = 0; gotchi_mood(t, GM_MISS); sim_note("A BOREAGOTCHI WENT BACK TO ITS HOME PLANET"); }
}
const char *gotchi_feed(Toy *t, Blob *b, int snack) {
    if (b->stage == GS_EGG) return "IT IS STILL AN EGG";
    if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (!snack) { if (b->hun >= 4) return "IT REFUSES. IT IS FULL"; b->hun++; if (b->wt < 99) b->wt++; if (b->meal < 250) b->meal++; if (b->bond < 100 && !(gr() & 3)) b->bond++; xp(t, 1); return "NOM NOM. A MEAL"; }
    if (b->hap < 4) b->hap++; if (b->wt < 98) b->wt += 2; if (b->snk < 250) b->snk++; xp(t, 1); return "CRUNCH. A SNACK. SO RELAXED";
}
const char *gotchi_light(Toy *t, Blob *b) { b->fl ^= GF_DARK; return (b->fl & GF_DARK) ? ((b->fl & GF_SLEEP) ? "LIGHTS OUT. GOOD NIGHT" : "LIGHTS OUT") : "LIGHTS ON"; }
const char *gotchi_medicine(Toy *t, Blob *b) { if (!(b->fl & GF_SICK)) return "IT IS NOT SICK. IT SPITS IT OUT"; b->fl &= (u8)~GF_SICK; if (b->bond < 100) b->bond++; xp(t, 2); return "ALL BETTER"; }
const char *gotchi_clean(Toy *t, Blob *b) { if (!b->poop) return "NOTHING TO CLEAN"; b->poop = 0; if (b->bond < 100) b->bond++; xp(t, 2); return "FLUSHED"; }
const char *gotchi_scold(Toy *t, Blob *b) {
    if (b->fl & GF_FAKE) { b->fl &= (u8)~GF_FAKE; b->disc = (u8)(b->disc + 25 > 100 ? 100 : b->disc + 25); xp(t, 3); return "IT KNOWS BETTER NOW. DISCIPLINE UP"; }
    if (b->hap) b->hap--; return "IT DID NOTHING WRONG. IT IS SAD NOW";
}
const char *gotchi_game(Toy *t, Blob *b, int won) {
    if (!won) { xp(t, 1); return "IT LOST. STILL FUN"; }
    if (b->hap < 4) b->hap++; if (b->wt > 1) b->wt--; if (b->bond < 100) b->bond += 2; xp(t, 2); t->wins++;
    int i = gotchi_sim_of(t); if (i >= 0) { sim_gotchi_fun(i, 6); if (!(gr() & 3)) sim_gotchi_mood(i, GM_BUDDY); }
    return "IT WON! SO HAPPY";
}
static int power(const Blob *b) { return (b->hun + b->hap) * 3 + b->disc / 8 + b->stage * 6 + b->bond / 10 - b->miss * 2 - (b->fl & GF_SICK ? 10 : 0) + (b->form == 0 || b->form == 3 ? 4 : 0); }
const char *gotchi_friends(Toy *t, int a, int c, int battle) {
    Blob *x = &t->b[a], *y = &t->b[c];
    if (a == c || !x->on || !y->on || x->stage == GS_EGG || y->stage == GS_EGG) return "THEY BOTH NEED TO HAVE HATCHED";
    if ((x->fl | y->fl) & GF_SLEEP) return "SHHH. SOMEONE IS ASLEEP";
    if (battle) {
        int px = power(x) + gr() % 12, py = power(y) + gr() % 12; Blob *w = px >= py ? x : y, *l = px >= py ? y : x;
        if (w->bond < 97) w->bond += 3; if (l->hap) l->hap--; w->disc = (u8)MIN(100, w->disc + 6); xp(t, 3); t->wins++;
        return w == x ? "YOUR FIRST ONE WON THE BATTLE!" : "YOUR SECOND ONE WON THE BATTLE!";
    }
    if (x->hap < 4) x->hap++; if (y->hap < 4) y->hap++; x->bond = (u8)MIN(100, x->bond + 4); y->bond = (u8)MIN(100, y->bond + 4); xp(t, 3);
    int i = gotchi_sim_of(t); if (i >= 0) sim_gotchi_fun(i, 8);
    if (x->stage == GS_ADULT && y->stage == GS_ADULT && x->bond >= 60 && y->bond >= 60 && !(gr() & 1)) {   // two grown-ups who love each other: an egg
        for (int s = 0; s < GB_N; s++) if (!t->b[s].on) { fresh(&t->b[s], (gr() & 3) == 0 ? gr() : (gr() & 1 ? x->sp : y->sp)); t->b[s].gen = (u8)MIN(99, MAX(x->gen, y->gen) + 1); return "THEY PLAYED... AND LEFT AN EGG!"; }
    }
    return "A LOVELY PLAYDATE. BOTH ARE HAPPY";
}

static int kin(int si) { return sim_gotchi_stat(si, 1) >= 20 && sim_gotchi_stat(si, 0) > -40; }   // is that Bore up to caring for anyone?
static void pets_minute(void) {
    for (int k = 0; k < GT_MAX; k++) {
        Toy *t = &gtoy[k]; if (!t->own) continue; int si = gotchi_sim_of(t); if (si < 0) continue;
        for (int i = 0; i < GB_N; i++) {
            Blob *b = &t->b[i]; if (!b->on) continue;
            tick(t, b);
            // hijinks: a starving or miserable owner starves their pet of mood too, and a happy one lifts it
            { int food = sim_gotchi_stat(si, 1), mood = sim_gotchi_stat(si, 0);
              if (b->stage > GS_EGG && !(b->fl & GF_SLEEP)) {
                  if (food < 15 && !(gr() & 15) && b->hun) { b->hun--; if (si == player && !(gr() & 7)) sim_note("YOUR PET IS HUNGRY TOO. IT FEELS WHAT YOU FEEL"); }
                  if (mood < -40 && b->age % 25 == 0 && b->hap) b->hap--;
                  if (mood > 50 && b->age % 40 == 0) { if (b->hap < 4) b->hap++; if (b->bond < 100 && !(gr() & 3)) b->bond++; } } }
            if (si != player && kin(si) && gotchi_needs(b) && !(gr() & 15)) {   // a housemate looks after their own
                if (b->hun == 0) gotchi_feed(t, b, 0); else if (b->poop) gotchi_clean(t, b); else if (b->fl & GF_SICK) gotchi_medicine(t, b);
                else if (b->fl & GF_FAKE) gotchi_scold(t, b); else if (b->hap == 0) { b->hap = 2; }
            }
        }
    }
    for (int k = 0; k < GT_MAX; k++) {   // and the other way: pets in a bad way drag their owner down, happy ones lift them
        Toy *t = &gtoy[k]; if (!t->own) continue; int si = gotchi_sim_of(t); if (si < 0 || clock_min % 60) continue;
        int n = 0, glum = 0, glad = 0; for (int i = 0; i < GB_N; i++) { Blob *b = &t->b[i]; if (!b->on || b->stage == GS_EGG) continue; n++; if (b->hun == 0 && b->hap == 0) glum++; else if (b->hap >= 3 && b->hun >= 2) glad++; }
        if (glum) sim_gotchi_fun(si, -3 * glum); else if (n >= 2 && glad == n) sim_gotchi_fun(si, 3);
    }
}
void gotchi_minute(void) {
    if (gotchi_time != 2) pets_minute();
    static int beep; Toy *me = gotchi_toy(player);
    if (gotchi_time != 2 && me && gotchi_calling(me) && ++beep >= 60) { beep = 0; sim_note("YOUR BOREAGOTCHI IS BEEPING"); }
}
void gotchi_live(int steps) {
    if (gotchi_time != 1) return; static int a, b; a += steps; b += steps;
    while (a >= 60) { a -= 60; pets_minute(); }
    while (b >= 300) { b -= 300; sim_gotchi_absorb(player); }
}
const char *gotchi_lunch(Toy *t, Blob *b) {
    int si = gotchi_sim_of(t); if (b->stage == GS_EGG) return "IT IS STILL AN EGG"; if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (sim_gotchi_stat(si, 1) < 12) return "YOU ARE TOO HUNGRY TO SHARE. EAT FIRST!";
    if (b->hun >= 4) return "IT IS FULL. KEEP YOUR LUNCH"; sim_gotchi_food(si, -15); b->hun = 4; if (b->hap < 4) b->hap++; b->bond = (u8)MIN(100, b->bond + 3); if (b->meal < 250) b->meal++; xp(t, 2); sim_gotchi_mood(si, GM_SHARED);
    return "YOU GAVE IT YOUR LUNCH. YOUR TUMMY GROWLS";
}
const char *gotchi_nuzzle(Toy *t, Blob *b) {
    int si = gotchi_sim_of(t), h = clock_min / 60 + clock_day * 24 + 1; if (b->stage == GS_EGG) return "IT IS STILL AN EGG";
    if (t->pad == (u8)h) return "IT ALREADY NUZZLED YOU THIS HOUR"; t->pad = (u8)h; sim_gotchi_fun(si, 6); sim_gotchi_mood(si, GM_BUDDY); if (b->bond < 100) b->bond++; xp(t, 1);
    return "ALL GOOD. IT NUZZLES YOU. AWW"; }

void gotchi_save(void) {
    sv_u32(dex[0]); sv_u32(dex[1]);
    for (int k = 0; k < GT_MAX; k++) { Toy *t = &gtoy[k]; sv_u8(t->own); sv_u8(t->uid); sv_u8(t->sel); sv_u16(t->xp); sv_u16(t->hatched); sv_u16(t->wins); sv_u16(t->cared);
        for (int i = 0; i < GB_N; i++) sv_n(&t->b[i], sizeof(Blob)); }
}
void gotchi_load(int ver) {
    gotchi_reset(); if (ver < 21) return;
    dex[0] = ld_u32(); dex[1] = ld_u32();
    for (int k = 0; k < (ver >= 22 ? GT_MAX : 6); k++) { Toy *t = &gtoy[k]; t->own = (u8)ld_u8(); t->uid = (u8)ld_u8(); t->sel = (u8)ld_u8(); t->xp = (u16)ld_u16(); t->hatched = (u16)ld_u16(); t->wins = (u16)ld_u16(); t->cared = (u16)ld_u16();
        for (int i = 0; i < GB_N; i++) ld_n(&t->b[i], sizeof(Blob)); }
}
const char *gotchi_house(int simi, int what) {
    Toy *t = gotchi_toy(simi); if (!t) return "NO BOREAGOTCHI";
    int n = 0;
    for (int i = 0; i < GB_N; i++) { Blob *b = &t->b[i]; if (!b->on || b->stage == GS_EGG) continue;
        if (what == 0) { if (b->hun < 4 && !(b->fl & GF_SLEEP)) { b->hun++; if (b->wt < 99) b->wt++; if (b->meal < 250) b->meal++; n++; } }
        else if (what == 1) { b->fl ^= GF_DARK; n++; }
        else if ((b->fl & GF_SLEEP) && !(b->fl & GF_DARK)) { b->fl |= GF_DARK; if (b->bond < 100) b->bond++; n++; } }
    if (n) xp(t, 1);
    return what == 0 ? (n ? "THEY ALL TUCKED IN. NOM NOM" : "NOBODY IS HUNGRY") : what == 1 ? "CLICK. LIGHTS CHANGED" : (n ? "TUCKED IN. SWEET DREAMS" : "NOBODY NEEDS TUCKING IN");
}
