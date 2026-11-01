#pragma GCC optimize("Os")
// gotchi_act.c - the BOREAGOTCHI! pack's names, foods, tastes and hands-on actions (part of the overlay GOTCHI.OVL; see gotchi_act.h)
#include "gotchi_act.h"
#include "sim.h"

static u32 xr = 0x2468ACEu;
static int rr(void) { xr = xr * 1103515245u + 12345u + (u32)clock_min; return (int)((xr >> 16) & 255); }

static const char *const spNm[GSP_N] = { "BLOB", "CAT", "BUNNY", "BIRD", "FROG", "DINO", "FISH", "BEAR", "GHOST", "ROBOT", "SHROOM", "SLIME", "BAT", "SNAIL", "OCTO", "ALIEN", "UNICORN", "DRAGON", "PHOENIX", "MOONLING" };
static const char *const giv[32] = { "BLOBERT", "SNORLOCK", "MEH-CHAN", "DOZY", "SIR SIGH", "YAWNIE", "PUDDLE", "KEVIN", "NIBBLES", "GRUMP", "WOBBLE", "SPROCKET",
    "TOASTY", "MOCHI", "BEANS", "ZIGGY", "PICKLE", "DUMPLING", "FUZZ", "CLANK", "BOOP", "SQUISH", "MUNCH", "BISCUIT", "GLORP", "NOODLE", "PIP", "WAFFLES", "TWIG", "DOUGH", "ORBIT", "SNOOZE" };
static const char *const persNm[8] = { "SHY", "BOLD", "GREEDY", "LAZY", "PLAYFUL", "GENTLE", "CHEEKY", "CLEVER" };
static const char *const persNote[8] = { "SLOW TO TRUST. LOYAL ONCE IT DOES", "LOVES A BATTLE. FUSSES WHEN SAD", "ALWAYS HUNGRY. LOVES ANY FOOD", "SLEEPS IN. NEVER GETS BORED", "BORED FAST. LOVES GAMES", "RARELY SICK. BONDS SLOWLY", "FIBS A LOT AND THROWS FITS", "LEARNS FAST. EASY TO TRAIN" };
static const char *const formNm[GFORMS] = { "SUPER", "SLOB", "CHILL", "STAR", "CHAMP", "ROUND" };
static const char *const stageNm[6] = { "EGG", "BABY", "CHILD", "TEEN", "ADULT", "ELDER" };
static const char *const colNm[8] = { "GREEN", "PINK", "BLUE", "ORANGE", "PURPLE", "TEAL", "RED", "GOLD" };
static const char *const accNm[NACC] = { "NOTHING", "BOW", "TOP HAT", "SHADES", "SCARF", "FLOWER", "CROWN", "PARTY HAT" };
static const u8 accCost[NACC] = { 0, 20, 40, 40, 30, 20, 100, 30 };
static const char *const foodNm[NFOOD] = { "RICE BALL", "MISO SOUP", "NOODLES", "GRILLED FISH", "VEG STEW", "BERRY PIE", "COOKIE", "CANDY", "FRUIT", "ICE CREAM", "POPCORN", "PUDDING" };
static const u8 foodCost[NFOOD] = { 0, 0, 0, 5, 5, 10, 0, 5, 0, 10, 5, 10 };
static const char *const lvNm[6] = { "NEW PARENT", "CARING", "ATTENTIVE", "BLOB WHISPERER", "BREEDER", "GOTCHI MASTER" };
static const char *const achNm[NACH] = { "FIRST HATCH", "ALL GROWN UP", "STAR QUALITY", "COLLECTOR", "FIRST FAMILY", "GRANDPARENT", "FULL HOUSE", "RARE KIND",
    "SHINY", "REMEMBERED", "PLAYMATES", "DYNASTY", "GOOD CARER", "BLOBDEX STAR", "TWO FAMILIES", "GOTCHI MASTER" };
static const char *const achHow[NACH] = { "HATCH AN EGG", "RAISE A GROWN-UP", "RAISE A STAR FORM", "24 FORMS IN THE BLOBDEX", "A PAIR LAYS AN EGG", "A GRANDCHILD IS BORN", "FIVE PETS IN ONE TOY", "HATCH A RARE KIND",
    "A GOLD OR MUTANT PET", "A PET PASSES ON", "10 PLAYDATES", "FIVE GENERATIONS", "CARE SCORE OF 500", "60 FORMS IN THE BLOBDEX", "AN EGG FROM TWO FAMILIES", "REACH CARE LEVEL 5" };
static const char *const gameNm[NGAME] = { "WHICH WAY", "COPY ME", "CATCH", "HIGH LOW" };
static const char *const roomNm[6] = { "SUNNY ROOM", "SEA ROOM", "BERRY ROOM", "MINT ROOM", "NIGHT ROOM", "GOLD ROOM" };
static const char *const dayNm[7] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };

const char *gx_species(int sp) { return spNm[sp % GSP_N]; }
const char *gx_given(int nm) { return giv[nm & 31]; }
const char *gx_pers(int p) { return persNm[p & 7]; }
const char *gx_pers_note(int p) { return persNote[p & 7]; }
const char *gx_form(int f) { return formNm[f % GFORMS]; }
const char *gx_stage(int s) { return stageNm[s % 6]; }
const char *gx_colour(int c) { return colNm[c & 7]; }
const char *gx_acc(int a) { return accNm[a % NACC]; }
int gx_acc_cost(int a) { return accCost[a % NACC]; }
const char *gx_food(int f) { return foodNm[f % NFOOD]; }
int gx_food_cost(int f) { return foodCost[f % NFOOD]; }
const char *gx_level(int lv) { return lvNm[lv % 6]; }
const char *gx_ach(int i) { return achNm[i % NACH]; }
const char *gx_ach_how(int i) { return achHow[i % NACH]; }
const char *gx_game(int g) { return gameNm[g % NGAME]; }
const char *gx_room(int r) { return roomNm[r % 6]; }
void gx_name(const Blob *b, char *out) {
    out[0] = 0; if (b->stage == GS_ELDER) strcpy(out, "OLD "); else if (b->stage == GS_ADULT) { strcpy(out, formNm[b->form % GFORMS]); strcat(out, " "); } else if (b->stage == GS_TEEN) strcpy(out, b->form ? "SLOUCHY " : "ANGSTY ");
    strcat(out, b->stage == GS_EGG ? "EGG" : spNm[b->sp % GSP_N]); if (b->stage == GS_BABY) strcat(out, " BABY");
}
const char *gx_want(const Blob *b) {
    if (b->stage == GS_EGG) return "STILL AN EGG";
    return b->hun == 0 ? "IT IS HUNGRY" : (b->fl & GF_TANTRUM) ? "TANTRUM! CUDDLE OR SCOLD IT" : b->hap == 0 ? "IT IS BORED" : b->poop ? "IT MADE A MESS" : (b->fl & GF_SICK) ? "IT FEELS SICK" :
           ((b->fl & GF_SLEEP) && !(b->fl & GF_DARK)) ? "TURN THE LIGHT OFF" : (b->fl & GF_FAKE) ? "IT IS FIBBING. SCOLD IT" : "ALL GOOD";
}
void gx_diary(const Diary *d, char *out) {
    static const char *const vb[16] = { "", "HATCHED", "GREW UP A BIT", "BECAME A GROWN-UP", "PASSED ON. REMEMBERED", "BROUGHT A GIFT", "HAD A BIRTHDAY", "HAD A TANTRUM", "WAS ILL", "IS A NEW EGG", "HAD A PLAYDATE", "IS A RARE MUTANT", "WAS LOOKED AFTER BY MUM OR DAD", "FELL IN LOVE", "VISITED A HOUSEMATE", "" };
    const Line *l = gotchi_line(d->id); char w[24];
    if (d->kind == GD_AWARD) { sprintf(out, "%s %02d  AWARD: %s", dayNm[(d->day / 24) % 7], d->day % 24, achNm[d->id % NACH]); return; }
    strcpy(w, l ? giv[l->nm & 31] : "A PET"); sprintf(out, "%s %02d  %s %s", dayNm[(d->day / 24) % 7], d->day % 24, w, vb[d->kind & 15]);
}

// ---- actions ----
static void up(u8 *v, int max, int n) { int x = *v + n; *v = (u8)(x > max ? max : x < 0 ? 0 : x); }
static int owner(Toy *t) { return gotchi_sim_of(t); }
const char *gx_feed(Toy *t, int slot, int food) {
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot]; int snack = food >= 6;
    if (b->stage == GS_EGG) return "IT IS STILL AN EGG";
    if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (!snack && b->hun >= 4) return "IT REFUSES. IT IS FULL";
    if (snack && b->hap >= 4 && b->hun >= 4) return "IT IS FULL AND HAPPY";
    if (cash < gx_food_cost(food)) return "NOT ENOUGH CASH FOR THAT";
    cash -= gx_food_cost(food);
    if ((k->taste & 15) == food && k->pers != GP_GREEDY) { if (b->hap) b->hap--; return "YUCK! IT HATES THAT. IT SPITS IT OUT"; }
    int love = (k->taste >> 4) == food;
    if (!snack) { b->hun++; up(&b->wt, 99, 1); up(&b->meal, 250, 1); if (rr() & 3) { } else up(&b->bond, 100, 1); }
    else { up(&b->hap, 4, 1); up(&b->wt, 99, 2); up(&b->snk, 250, 1); }
    if (love || k->pers == GP_GREEDY) { up(&b->hap, 4, 1); up(&b->bond, 100, 2); }
    gotchi_xp(t, love ? 2 : 1);
    return love ? "ITS FAVOURITE! HAPPY WIGGLES" : snack ? "CRUNCH. A SNACK. SO RELAXED" : "NOM NOM. A MEAL";
}
const char *gx_lunch(Toy *t, int slot) {
    Blob *b = &t->b[slot]; int si = owner(t); if (b->stage == GS_EGG) return "IT IS STILL AN EGG"; if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (sim_gotchi_stat(si, 1) < 12) return "YOU ARE TOO HUNGRY TO SHARE. EAT FIRST!";
    if (b->hun >= 4) return "IT IS FULL. KEEP YOUR LUNCH"; sim_gotchi_food(si, -15); b->hun = 4; up(&b->hap, 4, 1); up(&b->bond, 100, 3); up(&b->meal, 250, 1); gotchi_xp(t, 2); sim_gotchi_mood(si, GM_SHARED);
    return "YOU GAVE IT YOUR LUNCH. YOUR TUMMY GROWLS";
}
const char *gx_light(Toy *t, int slot) { Blob *b = &t->b[slot]; (void)t; b->fl ^= GF_DARK; return (b->fl & GF_DARK) ? ((b->fl & GF_SLEEP) ? "LIGHTS OUT. GOOD NIGHT" : "LIGHTS OUT") : "LIGHTS ON"; }
const char *gx_medicine(Toy *t, int slot) { Blob *b = &t->b[slot]; if (!(b->fl & GF_SICK)) return "IT IS NOT SICK. IT SPITS IT OUT"; b->fl &= (u8)~GF_SICK; up(&b->bond, 100, 1); gotchi_xp(t, 2); return "ALL BETTER"; }
const char *gx_clean(Toy *t, int slot) { Blob *b = &t->b[slot]; if (!b->poop) return "NOTHING TO CLEAN"; b->poop = 0; up(&b->bond, 100, 1); gotchi_xp(t, 2); return "FLUSHED"; }
const char *gx_scold(Toy *t, int slot) {
    Blob *b = &t->b[slot]; int cl = t->k[slot].pers == GP_CLEVER ? 10 : 0;
    if (b->fl & GF_FAKE) { b->fl &= (u8)~GF_FAKE; up(&b->disc, 100, 25 + cl); gotchi_xp(t, 3); return "IT KNOWS BETTER NOW. DISCIPLINE UP"; }
    if (b->fl & GF_TANTRUM) { b->fl &= (u8)~GF_TANTRUM; up(&b->disc, 100, 15 + cl); if (b->hap) b->hap--; gotchi_xp(t, 2); return "IT STOPS THE FIT. A BIT SNIFFLY"; }
    if (b->hap) b->hap--; return "IT DID NOTHING WRONG. IT IS SAD NOW";
}
const char *gx_pet(Toy *t, int slot) {
    Blob *b = &t->b[slot]; int si = owner(t), h = clock_min / 60 + clock_day * 24 + 1; if (b->stage == GS_EGG) return "IT IS STILL AN EGG";
    if (b->fl & GF_TANTRUM) { b->fl &= (u8)~GF_TANTRUM; up(&b->hap, 4, 1); up(&b->bond, 100, 2); gotchi_xp(t, 2); return "A CUDDLE CALMS IT DOWN. AWW"; }
    if (gotchi_needs(b)) return gx_want(b);
    if (t->pad == (u8)h) return "IT ALREADY NUZZLED YOU THIS HOUR"; t->pad = (u8)h; sim_gotchi_fun(si, 6); sim_gotchi_mood(si, GM_BUDDY); up(&b->bond, 100, 1); gotchi_xp(t, 1);
    return "ALL GOOD. IT NUZZLES YOU. AWW";
}
const char *gx_game_end(Toy *t, int slot, int won, int game) {
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot]; (void)game; t->plays++;
    if (!won) { gotchi_xp(t, 1); up(&b->hap, 4, k->pers == GP_PLAYFUL ? 1 : 0); return "IT LOST. STILL FUN"; }
    up(&b->hap, 4, 1); if (b->wt > 1) b->wt--; up(&b->bond, 100, k->pers == GP_PLAYFUL ? 4 : 2); if (k->pers == GP_CLEVER) up(&b->disc, 100, 4); gotchi_xp(t, 2); t->wins++;
    int i = owner(t); if (i >= 0) { sim_gotchi_fun(i, 6); if (!(rr() & 3)) sim_gotchi_mood(i, GM_BUDDY); }
    return "IT WON! SO HAPPY";
}
static int power(const Blob *b, const Kin *k) { return (b->hun + b->hap) * 3 + b->disc / 8 + b->stage * 5 + b->bond / 10 - b->miss * 2 - (b->fl & GF_SICK ? 10 : 0) + (b->form == 0 || b->form == 3 || b->form == 4 ? 4 : 0) + (k->pers == GP_BOLD ? 5 : 0); }
const char *gx_meet(Toy *t, int a, Toy *u, int c, int battle) {
    Blob *x = &t->b[a], *y = &u->b[c]; Kin *kx = &t->k[a], *ky = &u->k[c];
    if (x == y || !x->on || !y->on || x->stage == GS_EGG || y->stage == GS_EGG) return "THEY BOTH NEED TO HAVE HATCHED";
    if ((x->fl | y->fl) & GF_SLEEP) return "SHHH. SOMEONE IS ASLEEP";
    if (battle) {
        int px = power(x, kx) + rr() % 12, py = power(y, ky) + rr() % 12; Blob *w = px >= py ? x : y, *l = px >= py ? y : x;
        up(&w->bond, 100, 3); if (l->hap) l->hap--; up(&w->disc, 100, 6); gotchi_xp(t, 3); t->wins++; gotchi_diary(GD_PLAY, kx->id);
        return w == x ? "YOUR PET WON THE BATTLE!" : "THE OTHER PET WON THE BATTLE!";
    }
    int rel = gx_kin(kx->id, ky->id), sh = (kx->pers == GP_SHY) + (ky->pers == GP_SHY);
    up(&x->hap, 4, 1); up(&y->hap, 4, 1); up(&x->bond, 100, 4 + rel - sh); up(&y->bond, 100, 4 + rel - sh); t->plays++; if (u != t) u->plays++; gotchi_xp(t, 3); gotchi_diary(GD_PLAY, kx->id);
    int si = owner(t), sj = owner(u); if (si >= 0) sim_gotchi_fun(si, 8); if (sj >= 0 && sj != si) sim_gotchi_fun(sj, 6);
    if (rel) return "FAMILY PLAYS TOGETHER. A LOVELY GAME";
    if (x->stage >= GS_ADULT && y->stage >= GS_ADULT && x->bond >= 60 && y->bond >= 60) {
        if (t == u && !kx->mate && !ky->mate) { kx->mate = (u8)(c + 1); ky->mate = (u8)(a + 1); gotchi_diary(GD_BOND, kx->id); sim_gotchi_mood(si, GM_BUDDY); return "THEY FELL IN LOVE! A NEW PAIR"; }
        if ((t != u || kx->mate == c + 1) && (rr() & 1)) {
            Toy *h = t; int s = -1; for (int e = 0; e < GB_N && s < 0; e++) if (!t->b[e].on) s = e;
            if (s < 0 && u != t) { h = u; for (int e = 0; e < GB_N && s < 0; e++) if (!u->b[e].on) s = e; }
            if (s >= 0) { gx_egg_of(h, s, kx->id, ky->id); gotchi_diary(GD_EGG, h->k[s].id); if (t != u) gx_award(14); return t != u ? "TWO FAMILIES MET... AND LEFT AN EGG!" : "THEY PLAYED... AND LEFT AN EGG!"; }
            return "A LOVELY PLAYDATE. NO ROOM FOR AN EGG"; }
    }
    return "A LOVELY PLAYDATE. BOTH ARE HAPPY";
}
const char *gx_dress(Toy *t, int slot, int acc) {
    Kin *k = &t->k[slot]; if (t->b[slot].stage == GS_EGG) return "AN EGG HAS NO HEAD YET";
    if (acc && !(t->accs & (1 << acc))) { if (cash < accCost[acc]) return "NOT ENOUGH CASH"; cash -= accCost[acc]; t->accs |= (u8)(1 << acc); }
    k->acc = (u8)acc; up(&t->b[slot].hap, 4, 1); gotchi_xp(t, 1); return acc ? "LOOKING GOOD!" : "ALL NATURAL";
}

// ---- families ----
static int par_of(const Line *a, int c) {                  // is c a parent or grandparent of a?
    for (int i = 0; i < 2; i++) { int p = i ? a->dad : a->mom; if (!p) continue; if (p == c) return 1; const Line *l = gotchi_line(p); if (l && (l->mom == c || l->dad == c)) return 1; }
    return 0;
}
int gx_kin(int a, int c) {
    const Line *x = gotchi_line(a), *y = gotchi_line(c); if (a == c) return 1; if (!x || !y) return 0;
    if (par_of(x, c) || par_of(y, a)) return 1;
    return (x->mom && (x->mom == y->mom || x->mom == y->dad)) || (x->dad && (x->dad == y->dad || x->dad == y->mom));
}
int gx_children(int id, int *out, int max) { int n = 0; for (int i = 0; i < GL_N; i++) if (gline[i].id && id && (gline[i].mom == id || gline[i].dad == id) && n < max) out[n++] = gline[i].id; return n; }
static int pick(int a, int b) { return (rr() & 1) ? a : b; }
int gx_egg_of(Toy *t, int slot, int mom, int dad) {   // an egg: kind, colour, temper and rare mutations come from the two parents
    if (!gotchi_egg(t, slot)) return 0;
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot]; Line *pm = gotchi_line(mom), *pd = gotchi_line(dad), *l = gotchi_line(k->id); if (!pm || !pd) return 1;
    b->gen = (u8)MIN(31, MAX(pm->pg >> 3, pd->pg >> 3) + 1); b->sp = (u8)pick(pm->sp, pd->sp);
    if (!(rr() & 7)) b->sp = (u8)(rr() % 16);
    if (((pm->mut | pd->mut) || ((pm->fc & 7) == 3 && (pd->fc & 7) == 3)) && !(rr() & 7)) b->sp = (u8)(16 + (rr() & 3));   // a rare kind
    { int r = rr() % 10, cm = (pm->fc >> 3) & 7, cd = (pd->fc >> 3) & 7; k->col = (u8)(r < 5 ? cm : r < 9 ? cd : rr() % 7); if (cm == 7 && cd == 7) k->col = 7; else if (!(rr() % 24)) k->col = 7; }
    if (rr() % 10 < 6) k->pers = (u8)(pick(pm->pg, pd->pg) & 7);
    k->mut = (u8)(((pm->mut & (rr() & 7)) | (pd->mut & (rr() & 7))) & 7); if (!(rr() & 15)) k->mut |= (u8)(1 << (rr() % 3));
    if (l) { l->sp = b->sp; l->fc = (u8)(128 | k->col << 3); l->pg = (u8)(k->pers | b->gen << 3); l->mom = (u8)mom; l->dad = (u8)dad; l->mut = k->mut; }
    return 1;
}
void gx_award(int bit) {
    if (gach & (1u << bit)) return; gach |= 1u << bit;
    if (!gotchi_toy(player)) return; int g = 50 + bit * 10; cash += g; char b[48]; sprintf(b, "BOREAGOTCHI AWARD! +$%d", g); sim_note(b); sim_gotchi_mood(player, 2);
}
int gx_score(void) {
    int s = 0, n = 0; for (int k = 0; k < GT_MAX; k++) if (gtoy[k].own) { const Toy *t = &gtoy[k]; s += t->xp / 2 + t->hatched * 5 + t->wins * 2 + t->plays; }
    for (int i = 0; i < GL_N; i++) if (gline[i].id) s += (gline[i].pg >> 3) * 8 + ((gline[i].fc & 64) ? 5 : 0);
    for (int i = 0; i < NACH; i++) n += (gach >> i) & 1; return s + n * 25 + gotchi_dex_count() * 3;
}
static int awake(const Blob *b, int minstage) { return b->on && b->stage >= minstage && !(b->fl & GF_SLEEP); }
static void hourly(Toy *t, int hh) {   // one game hour of a family: brothers and sisters play, pairs form and lay eggs, birthdays, gifts, a cold goes round
    int si = gotchi_sim_of(t);
    for (int i = 0; i < GB_N; i++) for (int j = i + 1; j < GB_N; j++) {
        Blob *x = &t->b[i], *y = &t->b[j]; Kin *a = &t->k[i], *c = &t->k[j];
        if (awake(x, GS_BABY) && awake(y, GS_BABY) && x->stage < GS_ELDER && y->stage < GS_ELDER && gx_kin(a->id, c->id) && !(rr() & 3)) {   // kin play
            up(&x->hap, 4, 1); up(&y->hap, 4, 1); up(&x->bond, 100, 1); up(&y->bond, 100, 1); }
        if (!awake(x, GS_ADULT) || !awake(y, GS_ADULT) || x->bond < 60 || y->bond < 60) continue;
        if (!a->mate && !c->mate && !gx_kin(a->id, c->id) && !(rr() & 3)) { a->mate = (u8)(j + 1); c->mate = (u8)(i + 1); gotchi_diary(GD_BOND, a->id); sim_note("TWO BOREAGOTCHIS FELL IN LOVE"); continue; }
        if (a->mate == j + 1 && x->hun >= 2 && y->hun >= 2 && x->hap >= 3 && y->hap >= 3 && (rr() & 7) < 2) {
            int s = -1; for (int e = 0; e < GB_N; e++) if (!t->b[e].on) { s = e; break; }
            if (s >= 0) { gx_egg_of(t, s, a->id, c->id); gotchi_diary(GD_EGG, t->k[s].id); sim_note("THE PAIR LAID AN EGG! A NEW FAMILY MEMBER"); } }
    }
    for (int i = 0; i < GB_N; i++) { Blob *b = &t->b[i]; Kin *k = &t->k[i]; if (!awake(b, GS_CHILD)) continue;
        if (hh % 168 == k->bday && b->age > 300) { up(&b->hap, 4, 4); up(&b->bond, 100, 2); gotchi_diary(GD_BDAY, k->id); if (si >= 0) { sim_gotchi_fun(si, 6); if (si == player) sim_note("IT IS A BOREAGOTCHI BIRTHDAY!"); } }
        else if (b->bond >= 50 && b->hap >= 3 && !(rr() % 14) && si >= 0) {
            int g = 20 + rr() % 40; if (si == player) { cash += g; char s[48]; sprintf(s, "YOUR BOREAGOTCHI BROUGHT A GIFT: $%d", g); sim_note(s); } sim_gotchi_fun(si, 4); gotchi_diary(GD_GIFT, k->id); }
        else if (!(b->fl & GF_SICK) && k->pers != GP_GENTLE && !(rr() % 20)) {
            for (int j = 0; j < GB_N; j++) if (j != i && (t->b[j].fl & GF_SICK) && t->b[j].on) { b->fl |= GF_SICK; gotchi_diary(GD_ILL, k->id); break; } }
    }
}
static void visits(void) {   // housemates' pets visit each other
    int a = rr() % GT_MAX, c = rr() % GT_MAX; Toy *x = &gtoy[a], *y = &gtoy[c]; if (a == c || !x->own || !y->own) return;
    int i = rr() % GB_N, j = rr() % GB_N; Blob *p = &x->b[i], *q = &y->b[j]; if (!awake(p, GS_CHILD) || !awake(q, GS_CHILD) || p->stage == GS_ELDER) return;
    up(&p->hap, 4, 1); up(&q->hap, 4, 1); up(&p->bond, 100, 1); up(&q->bond, 100, 1); x->plays++; y->plays++;
    gotchi_diary(GD_VISIT, x->k[i].id); int sa = gotchi_sim_of(x), sc = gotchi_sim_of(y); if (sa >= 0) sim_gotchi_fun(sa, 3); if (sc >= 0) sim_gotchi_fun(sc, 3);
    if (sa == player || sc == player) sim_note("YOUR BOREAGOTCHI HAD A PLAYDATE WITH A HOUSEMATE'S");
    if (p->stage >= GS_ADULT && q->stage >= GS_ADULT && p->bond >= 60 && q->bond >= 60 && !gx_kin(x->k[i].id, y->k[j].id) && !(rr() & 3)) {   // and the families grow together
        Toy *h = sa == player ? x : y; for (int e = 0; e < GB_N; e++) if (!h->b[e].on) { gx_egg_of(h, e, x->k[i].id, y->k[j].id); gotchi_diary(GD_EGG, h->k[e].id); sim_note("TWO FAMILIES MET AND LEFT AN EGG!"); gx_award(14); break; } }
}
int gx_check(void) {
    int sum = 0, big = 0, ad = 0, star = 0, any = 0, hh = gt_done; if (gt_h - gt_done > 24) hh = gt_h - 24;
    for (; hh < gt_h; hh++) { for (int k = 0; k < GT_MAX; k++) if (gtoy[k].own && gotchi_sim_of(&gtoy[k]) >= 0) hourly(&gtoy[k], hh); if (hh & 1) visits(); }
    gt_done = gt_h;
    for (int k = 0; k < GT_MAX; k++) if (gtoy[k].own) { const Toy *t = &gtoy[k]; any = 1; sum += t->plays; if (t->hatched) gx_award(0); if (gotchi_level(t) >= 5) gx_award(15);
        int n = 0; for (int i = 0; i < GB_N; i++) { const Blob *b = &t->b[i]; if (!b->on) continue; n++; if (b->stage >= GS_ADULT) { ad = 1; if (b->form == 3) star = 1; } if (b->gen >= 4) big = 1; if (t->k[i].mut || t->k[i].col == 7) gx_award(8); }
        if (n >= 5) gx_award(6); }
    if (!any) return 0;
    if (ad) gx_award(1); if (star) gx_award(2); if (gotchi_dex_count() >= 24) gx_award(3); if (big) gx_award(11); if (sum >= 10) gx_award(10); if (gx_score() >= 500) gx_award(12); if (gotchi_dex_count() >= 60) gx_award(13);
    for (int i = 0; i < GL_N; i++) if (gline[i].id) { const Line *l = &gline[i]; if (l->mom) gx_award(4); if ((l->pg >> 3) >= 2) gx_award(5); if (l->sp >= 16) gx_award(7); if (l->fc & 64) gx_award(9); }
    int n = 0; for (int i = 0; i < NACH; i++) n += (gach >> i) & 1; return n;
}
