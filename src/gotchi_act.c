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
    "SHINY", "REMEMBERED", "PLAYMATES", "DYNASTY", "GOOD CARER", "BLOBDEX STAR", "TWO FAMILIES", "GOTCHI MASTER", "3 DAY STREAK", "WEEK STREAK", "SKILLED", "TOP SCORE" };
static const char *const achHow[NACH] = { "HATCH AN EGG", "RAISE A GROWN-UP", "RAISE A STAR FORM", "24 FORMS IN THE BLOBDEX", "A PAIR LAYS AN EGG", "A GRANDCHILD IS BORN", "FIVE PETS IN ONE TOY", "HATCH A RARE KIND",
    "A GOLD OR MUTANT PET", "A PET PASSES ON", "10 PLAYDATES", "FIVE GENERATIONS", "CARE SCORE OF 500", "60 FORMS IN THE BLOBDEX", "AN EGG FROM TWO FAMILIES", "REACH CARE LEVEL 5", "ALL GOALS 3 DAYS RUNNING", "ALL GOALS 7 DAYS RUNNING", "A SKILL OF 60", "GRADE S IN A GAME" };
static const char *const gameNm[NGAME] = { "WHICH WAY", "COPY ME", "CATCH", "HIGH LOW", "BEAT", "HOP", "STACK", "WHACK" };
static const u8 gameSk[NGAME] = { 2, 1, 0, 1, 2, 0, 1, 0 };   // the skill each game trains: 0 speed 1 brains 2 charm
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
    strcat(out, b->stage == GS_EGG ? ((b->form & 128) ? "MYSTERY EGG" : "EGG") : spNm[b->sp % GSP_N]); if (b->stage == GS_BABY) strcat(out, " BABY");
}
const char *gx_want(const Blob *b) {
    if (b->stage == GS_EGG) return "STILL AN EGG";
    return b->hun == 0 ? "IT IS HUNGRY" : (b->fl & GF_TANTRUM) ? "TANTRUM! CUDDLE OR SCOLD IT" : b->hap == 0 ? "IT IS BORED" : b->poop ? "IT MADE A MESS" : (b->fl & GF_SICK) ? "IT FEELS SICK" :
           ((b->fl & GF_SLEEP) && !(b->fl & GF_DARK)) ? "TURN THE LIGHT OFF" : (b->fl & GF_FAKE) ? "IT IS FIBBING. SCOLD IT" : "ALL GOOD";
}
void gx_diary(const Diary *d, char *out) {   // the diary tells it as a story: who, what kind, what happened
    static const char *const vb[32] = { "", "HATCHED", "GREW UP A BIT", "BECAME A GROWN-UP", "PASSED ON. REMEMBERED", "BROUGHT A GIFT", "HAD A BIRTHDAY", "HAD A TANTRUM", "WAS ILL", "IS A NEW EGG", "HAD A PLAYDATE", "IS A RARE MUTANT", "WAS LOOKED AFTER BY MUM OR DAD", "FELL IN LOVE", "VISITED A HOUSEMATE", "",
        "HAD A WISH COME TRUE", "TRAINED HARD", "BEAT ITS BEST SCORE", "MADE A BEST FRIEND", "MISSES A FRIEND", "HAD A SURPRISE", "KEPT THE DAILY GOALS", "", "", "", "", "", "", "", "", "" };
    int kd = d->kind & 31, sg = d->kind >> 5; const Line *l = gotchi_line(d->id); char w[32], t[40]; sprintf(t, "%s %02d  ", dayNm[(d->day / 24) % 7], d->day % 24);
    if (kd == GD_AWARD) { sprintf(out, "%sAWARD: %s", t, achNm[d->id % NACH]); return; }
    if (kd == GD_DAY) { sprintf(out, "%sGOODNIGHT. %d CARES TODAY", t, d->id); return; }
    if (kd == GD_EGG && sg) { sprintf(out, "%sA VISITOR LEFT A MYSTERY EGG", t); return; }
    if (l) sprintf(w, "%s THE %s", giv[l->nm & 31], spNm[l->sp % GSP_N]); else strcpy(w, "A PET");
    if (kd == GD_BEST) { sprintf(out, "%s%s BEAT ITS BEST AT %s", t, w, gameNm[sg]); return; }
    if (kd == GD_TRAIN && sg) { sprintf(out, "%s%s TRAINED ITS %s", t, w, gx_skill(sg - 1)); return; }
    if (kd == GD_GROW) { sprintf(out, "%s%s %s", t, w, sg == GS_CHILD ? "IS A CHILD NOW" : sg == GS_TEEN ? "IS A TEEN NOW" : sg == GS_ELDER ? "IS GETTING OLD" : "GREW UP A BIT"); return; }
    if (kd == GD_ADULT) { sprintf(out, "%s%s GREW UP: A %s ADULT", t, w, formNm[sg % GFORMS]); return; }
    sprintf(out, "%s%s %s", t, w, vb[kd & 31]);
}

void gx_diary_mem(const Diary *d, char *out) {   // one line of a pet's own memories (no name: the page is about it)
    static const char *const vb[32] = { "", "HATCHED", "GREW UP A BIT", "GREW UP", "PASSED ON", "BROUGHT A GIFT", "BIRTHDAY", "HAD A TANTRUM", "WAS ILL", "LAID AN EGG", "HAD A PLAYDATE", "RARE MUTANT", "WAS LOOKED AFTER", "FELL IN LOVE", "VISITED A FRIEND", "",
        "WISH GRANTED", "TRAINED HARD", "NEW BEST SCORE", "BEST FRIEND", "MISSED A FRIEND", "A SURPRISE", "GOALS KEPT", "", "", "", "", "", "", "", "", "" };
    int kd = d->kind & 31, sg = d->kind >> 5; sprintf(out, "%s %02d  ", dayNm[(d->day / 24) % 7], d->day % 24); char *e = out + strlen(out);
    if (kd == GD_BEST) sprintf(e, "NEW BEST AT %s", gameNm[sg]); else if (kd == GD_TRAIN && sg) sprintf(e, "TRAINED %s", gx_skill(sg - 1)); else strcpy(e, vb[kd]);
}
// ---- actions ----
static void up(u8 *v, int max, int n) { int x = *v + n; *v = (u8)(x > max ? max : x < 0 ? 0 : x); }
static int owner(Toy *t) { return gotchi_sim_of(t); }
static void tch(Toy *t, int s) { t->x[s].lone = 0; }
int gx_fx; static char gmsg[48]; static int gmsgOn;
const char *gx_pending(void) { if (!gmsgOn) return 0; gmsgOn = 0; return gmsg; }
int gx_fxs(void) { int f = gx_fx; gx_fx = 0; return f; }
// ---- today's goals: three of six kinds (meals, games, a bath, training, cuddles, a wish), the same for every pet of the toy; all three keep the streak going ----
static const u8 gTarget[6] = { 3, 2, 1, 2, 3, 1 };
static const char *const gNm[6] = { "FEED 3 MEALS", "PLAY 2 GAMES", "GIVE A BATH", "TRAIN TWICE", "CUDDLE 3 TIMES", "GRANT A WISH" };
const char *gx_goal_name(int type) { return gNm[type % 6]; }
int gx_goal_type(const Toy *t, int i) { return ((int)t->gday % 6 + i * 2) % 6; }
int gx_goal_target(int type) { return gTarget[type % 6]; }
int gx_goal_have(const Toy *t, int i) { return MIN(t->gp[gx_goal_type(t, i)], gTarget[gx_goal_type(t, i)]); }
int gx_goals_done(const Toy *t) { int n = 0; for (int i = 0; i < 3; i++) n += gx_goal_have(t, i) >= gTarget[gx_goal_type(t, i)]; return n; }
void gx_goals_roll(Toy *t) {   // a new day: the counts start again; a missed day ends the streak
    if (t->gday == (u16)day_count && (t->gflag & 128)) return; int dd = day_count - (int)t->gday; if (dd < 0) dd = 0;
    if (t->gflag & 128) { if (dd > 7) t->ghist = 0; else for (int k = 0; k < dd; k++) t->ghist = (u8)((t->ghist << 1 | (k == 0 && t->gwon == t->gday + 1)) & 127); if (!(dd == 1 && t->gwon == t->gday + 1)) t->streak = 0; }
    if (dd || !(t->gflag & 128)) { memset(t->gp, 0, 6); t->gflag = 128; }
    t->gday = (u16)day_count;
}
static int cashup(Toy *t, int n) { if (owner(t) == player) { cash += n; return 1; } return 0; }
static void gpadd(Toy *t, int kind) {
    gx_goals_roll(t); if (t->gp[kind] < 250) t->gp[kind]++;
    for (int i = 0; i < 3; i++) if (gx_goal_type(t, i) == kind && t->gp[kind] == gTarget[kind] && !(t->gflag & (2 << i))) { t->gflag |= (u8)(2 << i); cashup(t, 10); gotchi_xp(t, 3); gx_fx = 2; strcpy(gmsg, "GOAL DONE! +$10"); gmsgOn = 1; }
    if (!(t->gflag & 1) && gx_goals_done(t) == 3) {
        t->gflag |= 1; t->gwon = (u16)(day_count + 1); if (t->streak < 250) t->streak++; if (t->streak > t->best) t->best = t->streak;
        int g = 20 + 10 * MIN(t->streak, 10); cashup(t, g); gotchi_xp(t, 5); gx_fx = 3; sprintf(gmsg, "ALL GOALS! STREAK %d. +$%d", t->streak, g); gmsgOn = 1;
        if (t->streak >= 3) gx_award(16); if (t->streak >= 7) gx_award(17); for (int e = 0; e < GB_N; e++) if (t->b[e].on && t->streak % 3 == 0) { gotchi_diary(GD_STREAK, t->k[e].id); break; } }
}
// ---- wishes: each pet wants one thing at a time, by its nature; giving it is worth far more than a plain care ----
static const char *const wNm[7] = { "", "A MEAL", "A GAME", "A BATH", "TRAINING", "A CUDDLE", "A NAP" };
const char *gx_wish_name(int w) { return wNm[(w & 7) % 7]; }
static int wish_hit(Toy *t, int s, int kind) {
    Xt *x = &t->x[s]; Blob *b = &t->b[s]; if ((x->wish & 7) != kind) return 0;
    x->wish = 0; up(&b->bond, 100, 3); up(&b->hap, 4, 1); gotchi_xp(t, 3); gpadd(t, 5); gotchi_diary(GD_WISH, t->k[s].id); gx_fx = 1; int si = owner(t); if (si >= 0) sim_gotchi_mood(si, GM_BUDDY); return 1;
}
// ---- moodlets: what is going on inside, in a word or two ----
int gx_mood(const Toy *t, int s) {
    const Blob *b = &t->b[s]; const Xt *x = &t->x[s];
    int m = (b->hun * 625 + b->hap * 750 + x->nrg * 20 + x->hyg * 15 + b->bond * 10) / 100; if (b->fl & GF_SICK) m -= 20; if (b->fl & GF_TANTRUM) m -= 15; if (x->lone >= 12) m -= 8;
    return m < 0 ? 0 : m > 100 ? 100 : m;
}
const char *gx_mood_word(int m) { return m >= 80 ? "GREAT" : m >= 60 ? "GOOD" : m >= 40 ? "OKAY" : m >= 20 ? "CRANKY" : "MISERABLE"; }
int gx_moodlets(const Toy *t, int s, const char **o, u8 *good) {   // up to three, the bad ones first
    const Blob *b = &t->b[s]; const Xt *x = &t->x[s]; const Kin *k = &t->k[s]; int n = 0, sl = (b->fl & GF_SLEEP) != 0;
#define ML(txt, g) do { if (n < 3) { o[n] = txt; good[n++] = (u8)(g); } } while (0)
    if (b->stage == GS_EGG) return 0;
    if (b->fl & GF_SICK) ML("SICK", 0); if (b->fl & GF_TANTRUM) ML("TANTRUM", 0); if (b->hun == 0) ML("STARVING", 0); else if (b->hun == 1) ML("HUNGRY", 0);
    if (!sl && x->nrg < 25) ML("TIRED", 0); if (x->hyg < 30) ML("SMELLY", 0); if (x->lone >= (k->pers == GP_PLAYFUL ? 6 : k->pers == GP_SHY ? 20 : 12)) ML("LONELY", 0); if (b->hap <= 1) ML("BORED", 0); if (b->wt >= 25) ML("STUFFED", 0);
    if (b->bond >= 70) ML("LOVED", 1); if (x->nrg >= 80 && !sl) ML("RESTED", 1); if (x->hyg >= 85) ML("SQUEAKY CLEAN", 1); if (b->hap >= 4) ML("JOLLY", 1); if (b->hun >= 4) ML("FULL", 1); if (b->disc >= 70) ML("PROUD", 1); if (x->bff && x->fnd >= 10) ML("BEST FRIEND", 1);
    return n;
}
const char *gx_skill(int i) { static const char *const n[3] = { "SPEED", "BRAINS", "CHARM" }; return n[i % 3]; }
const char *gx_talent(int tal) { static const char *const n[4] = { "NO TALENT", "SPEEDY", "SMARTY", "CHARMER" }; return n[tal & 3]; }
const char *gx_stage_note(int st) {
    static const char *const n[6] = { "A COZY EGG. IT WILL HATCH SOON", "NEEDS FOOD OFTEN. NEVER TIRES. MESSY", "LOVES GAMES. TIRES QUICKLY", "MOODY AND CHEEKY. TALKS BACK", "STEADY. FAMILIES START HERE", "WISE BUT TIRES EASILY. A GOOD FRIEND" };
    return n[st % 6];
}
int gx_game_skill(int g) { return gameSk[g % NGAME]; }

const char *gx_feed(Toy *t, int slot, int food) {
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot]; int snack = food >= 6; Xt *x = &t->x[slot];
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
    gotchi_xp(t, love ? 2 : 1); tch(t, slot); up(&x->nrg, 100, snack ? 6 : 4); if (!snack) gpadd(t, 0);
    if (wish_hit(t, slot, 1)) return "ITS WISH! IT LOVES YOU";
    return love ? "ITS FAVOURITE! HAPPY WIGGLES" : snack ? "CRUNCH. A SNACK. SO RELAXED" : "NOM NOM. A MEAL";
}
const char *gx_lunch(Toy *t, int slot) {
    Blob *b = &t->b[slot]; int si = owner(t); if (b->stage == GS_EGG) return "IT IS STILL AN EGG"; if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (sim_gotchi_stat(si, 1) < 12) return "YOU ARE TOO HUNGRY TO SHARE. EAT FIRST!";
    if (b->hun >= 4) return "IT IS FULL. KEEP YOUR LUNCH"; sim_gotchi_food(si, -15); b->hun = 4; up(&b->hap, 4, 1); up(&b->bond, 100, 3); up(&b->meal, 250, 1); gotchi_xp(t, 2); sim_gotchi_mood(si, GM_SHARED); tch(t, slot); gpadd(t, 0);
    return "YOU GAVE IT YOUR LUNCH. YOUR TUMMY GROWLS";
}
const char *gx_light(Toy *t, int slot) {   // lights out helps it sleep; by day, with a tired pet, it is a nap (it wakes when rested)
    Blob *b = &t->b[slot]; Xt *x = &t->x[slot]; b->fl ^= GF_DARK; tch(t, slot);
    if ((b->fl & GF_DARK) && !(b->fl & GF_SLEEP) && b->stage >= GS_CHILD && x->nrg < 70) { b->fl |= GF_SLEEP; wish_hit(t, slot, 6); return "LIGHTS OUT. IT TAKES A NAP"; }
    if (!(b->fl & GF_DARK) && (b->fl & GF_SLEEP) && x->nrg < 60 && !(clock_min >= 20 * 60 || clock_min < 6 * 60)) { b->fl &= (u8)~GF_SLEEP; return "LIGHTS ON. IT WAKES UP, YAWNING"; }
    return (b->fl & GF_DARK) ? ((b->fl & GF_SLEEP) ? "LIGHTS OUT. GOOD NIGHT" : "LIGHTS OUT") : "LIGHTS ON"; }
const char *gx_medicine(Toy *t, int slot) { Blob *b = &t->b[slot]; if (!(b->fl & GF_SICK)) return "IT IS NOT SICK. IT SPITS IT OUT"; b->fl &= (u8)~GF_SICK; up(&b->bond, 100, 1); gotchi_xp(t, 2); tch(t, slot); return "ALL BETTER"; }
const char *gx_clean(Toy *t, int slot) { Blob *b = &t->b[slot]; if (!b->poop) return "NOTHING TO CLEAN"; b->poop = 0; up(&b->bond, 100, 1); up(&t->x[slot].hyg, 100, 10); gotchi_xp(t, 2); tch(t, slot); return "FLUSHED"; }
const char *gx_scold(Toy *t, int slot) {
    Blob *b = &t->b[slot]; int cl = t->k[slot].pers == GP_CLEVER ? 10 : 0;
    if (b->fl & GF_FAKE) { b->fl &= (u8)~GF_FAKE; tch(t, slot); up(&b->disc, 100, 25 + cl); gotchi_xp(t, 3); return "IT KNOWS BETTER NOW. DISCIPLINE UP"; }
    if (b->fl & GF_TANTRUM) { b->fl &= (u8)~GF_TANTRUM; up(&b->disc, 100, 15 + cl); if (b->hap) b->hap--; gotchi_xp(t, 2); return "IT STOPS THE FIT. A BIT SNIFFLY"; }
    if (b->hap) b->hap--; return "IT DID NOTHING WRONG. IT IS SAD NOW";
}
const char *gx_pet(Toy *t, int slot) {
    Blob *b = &t->b[slot]; int si = owner(t), h = clock_min / 60 + clock_day * 24 + 1; if (b->stage == GS_EGG) return "IT IS STILL AN EGG";
    if (b->fl & GF_TANTRUM) { b->fl &= (u8)~GF_TANTRUM; up(&b->hap, 4, 1); up(&b->bond, 100, 2); gotchi_xp(t, 2); tch(t, slot); gpadd(t, 4); wish_hit(t, slot, 5); return "A CUDDLE CALMS IT DOWN. AWW"; }
    if (gotchi_needs(b)) return gx_want(b);
    if (t->pad == (u8)h) return "IT ALREADY NUZZLED YOU THIS HOUR"; t->pad = (u8)h; sim_gotchi_fun(si, 6); sim_gotchi_mood(si, GM_BUDDY); up(&b->bond, 100, 1); gotchi_xp(t, 1); tch(t, slot); gpadd(t, 4);
    if (wish_hit(t, slot, 5)) return "ITS WISH! IT NUZZLES YOU";
    return "ALL GOOD. IT NUZZLES YOU. AWW";
}
static char rmsg[48]; int gx_grade;
const char *gx_play_check(Toy *t, int slot, int train) {   // 0 when it can play (or train); else why not
    Blob *b = &t->b[slot]; Xt *x = &t->x[slot]; int m = gx_mood(t, slot);
    if (b->stage == GS_EGG) return "IT IS STILL AN EGG"; if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (x->nrg < (train ? 30 : 10)) return train ? "TOO TIRED TO TRAIN. LET IT REST" : "TOO TIRED TO PLAY. LIGHTS OUT?";
    if (train && b->hun < 2) return "TOO HUNGRY TO TRAIN. FEED IT FIRST";
    if (train && b->stage < GS_CHILD) return "BABIES ARE TOO SMALL TO TRAIN";
    if (m < 20) return "TOO MISERABLE. CARE FOR IT FIRST";
    if (train && m < 40) return "NOT IN THE MOOD TO TRAIN";
    if (train && t->k[slot].pers == GP_LAZY && !(rr() & 3)) return "THE LAZY ONE SAYS NO. TRY AGAIN";
    return 0;
}
const char *gx_game_result(Toy *t, int slot, int game, int pct, int train) {
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot]; Xt *x = &t->x[slot]; int g = pct >= 90 ? 4 : pct >= 70 ? 3 : pct >= 50 ? 2 : pct >= 30 ? 1 : 0, sk = gameSk[game % NGAME], m = gx_mood(t, slot), si = owner(t), pr = k->pers;
    static const char *const gn[5] = { "D. TRY AGAIN", "C. NOT BAD", "B. NICE", "A! GREAT", "S! AMAZING" };
    t->plays++; gx_grade = g; tch(t, slot);
    int cost = train ? 12 : 6; if (b->wt >= 20) cost += 3; if (pr == GP_LAZY) cost += 2; x->nrg = (u8)(x->nrg > cost ? x->nrg - cost : 0); up(&x->hyg, 100, train ? -8 : -4);
    if (train) { if (b->hun > 1) b->hun--; int gain = 1 + g * 2; if (pr == GP_CLEVER) gain++; if (pr == GP_LAZY && gain > 1) gain--; gain = gain * (50 + m) / 100 + (g > 0); if (gain < 1) gain = 1; up(&x->sk[sk], 100, gain);
        up(&b->disc, 100, 2 + g * 3 + (pr == GP_CLEVER ? 3 : 0) + x->sk[1] / 20); if (g < 2) up(&b->hap, 4, -1); if (g >= 2) up(&b->bond, 100, 1); gotchi_xp(t, 2 + g); gpadd(t, 3); wish_hit(t, slot, 4); gotchi_diary(GD_TRAIN | (sk + 1) << 5, k->id); }
    else {
        up(&b->hap, 4, (g >= 2) + (g >= 4) + (pr == GP_PLAYFUL && g >= 1)); if (g >= 2 && b->wt > 1) b->wt--; up(&b->bond, 100, (g >= 1) + (g >= 3) * 2 + (pr == GP_PLAYFUL ? 1 : 0) + (m >= 80));
        up(&x->sk[sk], 100, g >= 2 ? 1 : 0); gotchi_xp(t, 1 + g); if (g >= 2) t->wins++; if (g >= 1) { gpadd(t, 1); wish_hit(t, slot, 2); }
        if (si >= 0) { sim_gotchi_fun(si, 2 + g * 2); if (g >= 3 && !(rr() & 3)) sim_gotchi_mood(si, GM_BUDDY); }
        if (g >= 3) { int c = (g == 4 ? 10 : 5) + x->sk[2] / 20; cashup(t, c); sprintf(rmsg, "GRADE %s +$%d", gn[g], c); } }
    if (!train || g < 3) sprintf(rmsg, "%sGRADE %s", train ? "TRAINED. " : "", gn[g]);
    else sprintf(rmsg, "TRAINED! GRADE %s", gn[g]);
    if (g >= 3 && !train) { int c = (g == 4 ? 10 : 5) + x->sk[2] / 20; sprintf(rmsg, "GRADE %s +$%d", gn[g], c); }
    if (g == 4) gx_award(19);
    if (x->sk[0] >= 60 || x->sk[1] >= 60 || x->sk[2] >= 60) gx_award(18);
    if (game >= 0 && game < 8 && pct > gbest[game]) { if (gbest[game]) { strcat(rmsg, ". NEW BEST!"); gx_fx = 4; } if (gbest[game] || pct >= 50) gotchi_diary(GD_BEST | game << 5, k->id); gbest[game] = (u8)pct; }
    return rmsg;
}
static void friend2(Toy *t, int i, int j, int n) {   // two pets of one toy grow closer: each keeps one best friend
    Xt *a = &t->x[i], *c = &t->x[j];
    if (a->bff != j + 1) { if (a->fnd < 8 || !a->bff) { a->bff = (u8)(j + 1); a->fnd = 1; } } else if (a->fnd < 100) { a->fnd = (u8)MIN(100, a->fnd + n); }
    if (c->bff != i + 1) { if (c->fnd < 8 || !c->bff) { c->bff = (u8)(i + 1); c->fnd = 1; } } else if (c->fnd < 100) { c->fnd = (u8)MIN(100, c->fnd + n); }
    if (a->bff == j + 1 && c->bff == i + 1 && a->fnd >= 10 && a->fnd - n < 10) gotchi_diary(GD_FRIEND, t->k[i].id);
}
static int power(const Blob *b, const Kin *k) { return (b->hun + b->hap) * 3 + b->disc / 8 + b->stage * 5 + b->bond / 10 - b->miss * 2 - (b->fl & GF_SICK ? 10 : 0) + ((b->form & 127) == 0 || b->form == 3 || b->form == 4 ? 4 : 0) + (k->pers == GP_BOLD ? 5 : 0); }
const char *gx_meet(Toy *t, int a, Toy *u, int c, int battle) {
    Blob *x = &t->b[a], *y = &u->b[c]; Kin *kx = &t->k[a], *ky = &u->k[c];
    if (x != y && x->on && y->on && x->stage > GS_EGG && y->stage > GS_EGG && !((x->fl | y->fl) & GF_SLEEP) && (t->x[a].nrg < 10 || u->x[c].nrg < 10)) return "TOO TIRED TO PLAY. LET THEM REST";
    if (x == y || !x->on || !y->on || x->stage == GS_EGG || y->stage == GS_EGG) return "THEY BOTH NEED TO HAVE HATCHED";
    if ((x->fl | y->fl) & GF_SLEEP) return "SHHH. SOMEONE IS ASLEEP";
    if (battle) {
        int px = power(x, kx) + rr() % 12, py = power(y, ky) + rr() % 12; Blob *w = px >= py ? x : y, *l = px >= py ? y : x;
        up(&w->bond, 100, 3); if (l->hap) l->hap--; up(&w->disc, 100, 6); gotchi_xp(t, 3); t->wins++; gotchi_diary(GD_PLAY, kx->id);
        return w == x ? "YOUR PET WON THE BATTLE!" : "THE OTHER PET WON THE BATTLE!";
    }
    int rel = gx_kin(kx->id, ky->id), sh = (kx->pers == GP_SHY) + (ky->pers == GP_SHY); tch(t, a); if (u != t || c != a) tch(u, c); if (t == u) friend2(t, a, c, 2); up(&t->x[a].nrg, 100, -4); up(&t->x[a].sk[2], 100, 1);
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


const char *gx_bath(Toy *t, int slot) {
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot]; Xt *x = &t->x[slot]; int pr = k->pers;
    if (b->stage == GS_EGG) return "IT IS STILL AN EGG"; if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (x->hyg >= 90) return "IT IS ALREADY SQUEAKY CLEAN";
    if (x->nrg < 5) return "TOO TIRED FOR A BATH";
    x->hyg = 100; b->poop = b->poop > 1 ? b->poop - 1 : 0; up(&x->nrg, 100, -3); up(&b->bond, 100, 1); gotchi_xp(t, 2); tch(t, slot); gpadd(t, 2);
    if (wish_hit(t, slot, 3)) return "ITS WISH! SPLASH! SO CLEAN";
    if (pr == GP_CHEEKY || pr == GP_LAZY) { up(&b->hap, 4, -1); return "SPLASH! IT GRUMBLES BUT IS CLEAN"; }
    if (pr == GP_GENTLE || pr == GP_CLEVER) { up(&b->hap, 4, 1); return "SPLASH! IT LOVES BATH TIME"; }
    return "SPLASH! SQUEAKY CLEAN";
}
// ---- surprises while you hold the toy: the screen shows them, a press reacts ----
int gx_event_roll(Toy *t, int slot) {   // 0 nothing; 1 a fly, 2 a coin, 3 a shooting star, 4 a storm, 5 a visitor
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot]; if (!b->on || b->stage < GS_BABY || (b->fl & GF_SLEEP) || gotchi_needs(b)) return 0;
    int r = rr() % 10; if (k->pers == GP_CHEEKY && r < 2) return 1; if (k->pers == GP_GENTLE && r == 9) return 5;
    return r < 3 ? 1 : r < 5 ? 2 : r < 6 ? 3 : r < 8 ? 4 : 5;
}
const char *gx_event_do(Toy *t, int slot, int kind, int ok) {   // the reaction (ok: pressed in time)
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot]; Xt *x = &t->x[slot]; static char em[48]; int ch = x->sk[2] / 12;
    gotchi_diary(GD_EVENT, k->id); gx_fx = ok ? 5 : 0;
    switch (kind) {
    case 1: if (ok) { up(&b->bond, 100, 2); up(&b->hap, 4, 1); return "SWAT! IT LAUGHS AT YOU"; } up(&x->hyg, 100, -6); return "THE FLY LANDED ON IT. YUCK";
    case 2: if (ok) { int c = 5 + rr() % 10 + ch; cashup(t, c); sprintf(em, "FOUND A COIN! +$%d", c); return em; } return "THE COIN ROLLED AWAY";
    case 3: if (ok) { if (!(x->wish & 7)) x->wish = (u8)(1 + rr() % 6); up(&b->hap, 4, 1); up(&x->nrg, 100, 15); gotchi_xp(t, 2); return "YOU WISHED ON A STAR. IT FEELS HOPEFUL"; } return "THE STAR FADED";
    case 4: if (ok) { up(&b->bond, 100, 3); b->fl &= (u8)~GF_TANTRUM; gotchi_xp(t, 2); tch(t, slot); return "THUNDER! YOU HELD IT CLOSE. SO BRAVE"; } up(&b->hap, 4, -1); return "THUNDER! IT HID, SCARED";
    default: if (ok) { int c = 8 + rr() % 12 + ch; cashup(t, c); up(&b->hap, 4, 1); sprintf(em, "A VISITOR GAVE A GIFT. +$%d", c);
                       if (!(rr() % 25)) { for (int e = 0; e < GB_N; e++) if (!t->b[e].on) { gotchi_egg(t, e); t->b[e].form = 128; gotchi_diary(GD_EGG | 1 << 5, t->k[e].id); return "A VISITOR LEFT A MYSTERY EGG!"; } } return em; } return "THE VISITOR WAVED AND WENT";
    }
}
// ---- one-press care: what to give, without opening a list ----
static int liked(const Kin *k, int f) { return (k->taste & 15) != f || k->pers == GP_GREEDY; }
const char *gx_quick_feed(Toy *t, int slot) {   // its favourite if it is free or cheap, else the first free food it does not hate; a snack when it is full but sad
    Blob *b = &t->b[slot]; Kin *k = &t->k[slot]; int snack = b->hun >= 4 && b->hap < 4, base = snack ? 6 : 0, f = -1, fav = k->taste >> 4;
    if (fav >= base && fav < base + 6 && gx_food_cost(fav) <= 5 && gx_food_cost(fav) <= cash) f = fav;
    for (int i = 0; i < 6 && f < 0; i++) if (gx_food_cost(base + i) == 0 && liked(k, base + i)) f = base + i;
    if (f < 0) f = base;
    return gx_feed(t, slot, f);
}
int gx_urgent(const Blob *b) {   // what a pet needs most: 0 nothing, 1 medicine, 2 clean, 3 cuddle, 4 scold a fib, 5 food, 6 lights out, 7 a game
    if (!b->on || b->stage == GS_EGG) return 0;
    if (b->fl & GF_SICK) return 1; if (b->poop) return 2; if (b->fl & GF_TANTRUM) return 3; if ((b->fl & GF_FAKE) && !(b->fl & GF_SLEEP)) return 4;
    if ((b->fl & GF_SLEEP) && !(b->fl & GF_DARK)) return 6; if (b->fl & GF_SLEEP) return 0; if (b->hun == 0 || b->hun < 3) return 5; if (b->hap < 2) return 7; return 0;
}

int gx_urgent2(const Toy *t, int s) {   // as gx_urgent, plus: 8 a bath, 9 a nap
    const Blob *b = &t->b[s]; const Xt *x = &t->x[s]; int u = gx_urgent(b); if (u) return u; if (!b->on || b->stage == GS_EGG || (b->fl & GF_SLEEP)) return 0;
    if (x->nrg < 20 && b->stage >= GS_CHILD) return 9; if (x->hyg < 30) return 8; return 0;
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
    { int tl = gtal[pick(pm - gline, pd - gline)]; if (tl & 3) t->x[slot].sk[(tl & 3) - 1] = (u8)(6 + (tl >> 2) / 2); }   // a parent's talent runs in the family
    return 1;
}
static int abit(int i) { return i < 16 ? i : i + 5; }   // (the first sixteen awards are bits 0-15, the tutorial owns 16-20, the newer ones 21 up)
int gx_has(int i) { return (gach >> abit(i)) & 1; }
void gx_award(int bit) {
    int ab = abit(bit); if (gach & (1u << ab)) return; gach |= 1u << ab;
    if (!gotchi_toy(player)) return; int g = 50 + bit * 10; cash += g; char b[48]; sprintf(b, "BOREAGOTCHI AWARD! +$%d", g); sim_note(b); sim_gotchi_mood(player, 2);
}
int gx_score(void) {
    int s = 0, n = 0; for (int k = 0; k < GT_MAX; k++) if (gtoy[k].own) { const Toy *t = &gtoy[k]; s += t->xp / 2 + t->hatched * 5 + t->wins * 2 + t->plays; }
    for (int i = 0; i < GL_N; i++) if (gline[i].id) s += (gline[i].pg >> 3) * 8 + ((gline[i].fc & 64) ? 5 : 0);
    for (int i = 0; i < NACH; i++) n += gx_has(i); for (int k = 0; k < GT_MAX; k++) if (gtoy[k].own) for (int i = 0; i < GB_N; i++) if (gtoy[k].b[i].on) s += (gtoy[k].x[i].sk[0] + gtoy[k].x[i].sk[1] + gtoy[k].x[i].sk[2]) / 10;
    return s + n * 25 + gotchi_dex_count() * 3;
}
static int awake(const Blob *b, int minstage) { return b->on && b->stage >= minstage && !(b->fl & GF_SLEEP); }
static void hourly(Toy *t, int hh) {   // one game hour of a family: brothers and sisters play, pairs form and lay eggs, birthdays, gifts, a cold goes round
    int si = gotchi_sim_of(t);
    for (int i = 0; i < GB_N; i++) for (int j = i + 1; j < GB_N; j++) {
        Blob *x = &t->b[i], *y = &t->b[j]; Kin *a = &t->k[i], *c = &t->k[j];
        if (awake(x, GS_BABY) && awake(y, GS_BABY) && x->stage < GS_ELDER && y->stage < GS_ELDER && gx_kin(a->id, c->id) && !(rr() & 3)) {   // kin play
            up(&x->hap, 4, 1); up(&y->hap, 4, 1); up(&x->bond, 100, 1); up(&y->bond, 100, 1); friend2(t, i, j, 1); }
        if (!awake(x, GS_ADULT) || !awake(y, GS_ADULT) || x->bond < 60 || y->bond < 60) continue;
        if (!a->mate && !c->mate && !gx_kin(a->id, c->id) && !(rr() & 3)) { a->mate = (u8)(j + 1); c->mate = (u8)(i + 1); gotchi_diary(GD_BOND, a->id); sim_note("TWO BOREAGOTCHIS FELL IN LOVE"); continue; }
        if (a->mate == j + 1 && x->hun >= 2 && y->hun >= 2 && x->hap >= 3 && y->hap >= 3 && (rr() & 7) < 2) {
            int s = -1; for (int e = 0; e < GB_N; e++) if (!t->b[e].on) { s = e; break; }
            if (s >= 0) { gx_egg_of(t, s, a->id, c->id); gotchi_diary(GD_EGG, t->k[s].id); sim_note("THE PAIR LAID AN EGG! A NEW FAMILY MEMBER"); } }
    }
    for (int i = 0; i < GB_N; i++) { Blob *b = &t->b[i]; Kin *k = &t->k[i]; if (!awake(b, GS_CHILD)) continue;
        if (hh % 168 == k->bday && b->age > 300) { up(&b->hap, 4, 4); up(&b->bond, 100, 2); gotchi_diary(GD_BDAY, k->id); if (t->x[i].bd < 250) t->x[i].bd++; if (si >= 0) { sim_gotchi_fun(si, 6); if (si == player) sim_note("IT IS A BOREAGOTCHI BIRTHDAY!"); } }
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
    int n = 0; for (int i = 0; i < NACH; i++) n += gx_has(i); return n;
}
