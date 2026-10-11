// wants.c - wants, fears and the aspiration meter. See wants.h and DESIGN.md.
#include "wants.h"
#include "audio.h"
#include "world.h"

int wants_flash_t; char wants_flash[32];

// ---- what each want is worth, and what it is called ----
static const u16 wPts[W_N] = { 0, 500, 500, 500, 500, 800, 500, 800, 800, 1000, 1500, 3000, 1000, 1500, 5000, 3000, 1500, 1500, 1500, 1500, 3000, 3000, 1000, 1500, 1000, 1000, 1000, 1500, 1000, 4000 };
static const u16 fPts[F_N] = { 0, 1500, 1000, 3000, 1000, 2000, 1000 };
static const char *const wNm[W_N] = { "", "EAT A MEAL", "SLEEP IN A BED", "TAKE A SHOWER", "HAVE SOME FUN", "READ A BOOK", "DRINK A COFFEE", "GO FOR A RUN",
    "MAKE A PHONE CALL", "CHAT WITH SOMEONE", "CHAT WITH", "MAKE A FRIEND", "GO TO WORK", "GET PAID", "GET PROMOTED", "HAVE $", "BUY SOMETHING",
    "COOKING SKILL", "BODY SKILL", "FUN SKILL", "VISIT THE ISLAND", "VISIT THE TOWN", "EAT OUT", "GO FOR A SWIM", "WORK OUT AT A GYM", "GO SHOPPING", "SEE A FILM", "A NIGHT OUT", "DO HOMEWORK", "HAVE A BABY" };
static const char *const fNm[F_N] = { "", "PASS OUT", "WET MYSELF", "GET DEMOTED", "A BAD DAY AT WORK", "UNPAID BILLS", "BE TURNED DOWN" };

// ---- who wants what: a weight per kind, per aspiration (FORTUNE KNOWLEDGE POPULARITY PLEASURE HOME GROW UP) ----
static const u8 aspW[AS_N][W_N] = {
 //  -  eat slp shw fun rd cof run ph chat cw frnd work paid prm cash buy ck bd fn isl twn din swm gym shp flm ngt hw baby
    { 0, 1,  1,  1,  1,  1, 1,  1,  1, 1,   1, 2,   4,   5,   5,  5,   4,  1, 1, 1, 1,  1,  1,  1,  1,  4,  1,  1,  3, 1 },   // FORTUNE
    { 0, 1,  1,  1,  1,  5, 2,  2,  1, 1,   1, 1,   2,   1,   2,  1,   1,  4, 4, 4, 3,  3,  1,  1,  2,  1,  2,  0,  5, 1 },   // KNOWLEDGE
    { 0, 1,  1,  2,  2,  1, 1,  1,  4, 5,   5, 5,   1,   1,   2,  1,   1,  1, 1, 2, 2,  2,  3,  3,  2,  2,  2,  5,  2, 2 },   // POPULARITY
    { 0, 4,  2,  1,  5,  1, 4,  3,  1, 2,   2, 1,   0,   1,   1,  1,   2,  2, 1, 2, 3,  3,  5,  5,  1,  3,  4,  4,  1, 1 },   // PLEASURE
    { 0, 3,  3,  1,  2,  1, 1,  1,  2, 2,   5, 3,   1,   2,   1,  2,   4,  3, 1, 1, 1,  1,  1,  1,  1,  3,  2,  1,  3, 5 },   // HOME
    { 0, 2,  2,  1,  5,  4, 0,  3,  1, 3,   4, 3,   0,   0,   0,  0,   1,  1, 2, 3, 2,  2,  2,  4,  2,  1,  4,  0,  5, 0 },   // GROW UP
};
static const u8 aspF[AS_N][F_N] = {
    { 0, 1, 1, 5, 4, 5, 1 }, { 0, 2, 2, 2, 2, 1, 1 }, { 0, 1, 2, 1, 1, 1, 5 }, { 0, 3, 3, 1, 1, 1, 2 }, { 0, 2, 2, 2, 2, 4, 2 }, { 0, 2, 3, 0, 0, 0, 3 } };

int wants_skill_level(int raw) { int l = raw / 400; return l > 10 ? 10 : l; }
static int other_sim(const Sim *s) {
    if (nsims < 2) return -1;
    int me = (int)(s - sims), k = (int)(look_rand() % (nsims - 1)); if (k >= me) k++;
    return k;
}
static int has_want(const Sim *s, int kind, int arg) {
    for (int i = 0; i < 4; i++) if (s->want[i].kind == kind && (kind != W_CHAT_WITH || s->want[i].arg == arg)) return 1;
    return 0;
}
static void roll_want(Sim *s, int slot) {
    int asp = s->bore.asp % AS_N, stage = s->bore.stage;
    for (int tries = 0; tries < 24; tries++) {
        int w = 1 + (int)(look_rand() % (W_N - 1)), weight = aspW[asp][w];
        // the personality's own wants, on top of the aspiration's
        if (w == W_FUN && s->bore.tr[TR_PLAY] >= 7) weight += 3;
        if (w == W_CHAT && s->bore.tr[TR_OUT] >= 7) weight += 3;
        if (w == W_RUN && s->bore.tr[TR_ACT] >= 7) weight += 3;
        if (w == W_SHOWER && s->bore.tr[TR_NEAT] >= 7) weight += 3;
        if (w == W_CHAT_WITH && s->bore.tr[TR_NICE] >= 7) weight += 2;
        // what the age can want
        if (stage <= AG_CHILD && (w == W_WORK || w == W_PAID || w == W_PROMO || w == W_CASH || w == W_COFFEE || w == W_NIGHT)) weight = 0;
        if (stage == AG_ELDER && w == W_RUN) weight = 0;
        if (w == W_CHAT_WITH || w == W_FRIEND) { if (nsims < 2) weight = 0; }
        if (w == W_ISLAND && (story_visit[1] || world_zone == 1)) weight = 0;
        if (w == W_TOWN && (story_visit[2] || world_zone == 2)) weight = 0;
        if (w == W_PROMO && job_level >= 5) weight = 0;
        if (w == W_HOMEWORK && (stage != AG_CHILD && stage != AG_TEEN)) weight = 0;     // (only the ones at school)
        if (w == W_BABY && (stage != AG_ADULT || nsims >= MAX_SIMS || baby_due >= 0)) weight = 0;
        if (stage == AG_BABY) weight = 0;                                              // (a baby wants nothing it could do itself)
        if (weight == 0 || (int)(look_rand() % 6) >= weight) continue;
        int arg = 0;
        if (w == W_CHAT_WITH) arg = other_sim(s);
        if (w == W_FRIEND) { arg = other_sim(s); if (arg >= 0 && s->rel[arg] >= 50) continue; }
        if (w == W_CASH) { arg = (int)(cash / 5000) + 1; if (arg > 40) continue; }                 // in 5000s: the next step up
        if (w == W_SK_COOK) arg = wants_skill_level(s->skill_cook) + 1;
        if (w == W_SK_BODY) arg = wants_skill_level(s->skill_body) + 1;
        if (w == W_SK_FUN) arg = wants_skill_level(s->skill_fun) + 1;
        if ((w == W_SK_COOK || w == W_SK_BODY || w == W_SK_FUN) && arg > 10) continue;
        if (has_want(s, w, arg)) continue;
        s->want[slot].kind = (u8)w; s->want[slot].arg = (u8)arg; s->want[slot].age = 0;
        s->want[slot].pts = (u16)(w == W_CASH ? 1500 + arg * 100 : (w >= W_SK_COOK && w <= W_SK_FUN) ? 800 + arg * 200 : wPts[w]);
        return;
    }
    s->want[slot].kind = W_FUN; s->want[slot].arg = 0; s->want[slot].pts = wPts[W_FUN]; s->want[slot].age = 0;
}
static void roll_fear(Sim *s, int slot) {
    int asp = s->bore.asp % AS_N;
    for (int tries = 0; tries < 24; tries++) {
        int f = 1 + (int)(look_rand() % (F_N - 1));
        if ((int)(look_rand() % 6) >= aspF[asp][f]) continue;
        int dup = 0; for (int i = 0; i < 2; i++) if (i != slot && s->fear[i].kind == f) dup = 1;
        if (dup) continue;
        s->fear[slot].kind = (u8)f; s->fear[slot].pts = fPts[f]; s->fear[slot].age = 0; return;
    }
    s->fear[slot].kind = F_PASSOUT; s->fear[slot].pts = fPts[F_PASSOUT];
}
void wants_roll_all(Sim *s) {
    for (int i = 0; i < 4; i++) s->want[i].kind = 0;
    for (int i = 0; i < 4; i++) roll_want(s, i);
    for (int i = 0; i < 2; i++) { s->fear[i].kind = 0; roll_fear(s, i); }
    s->asp_pts = 4000;
}
void wants_reroll(Sim *s) {   // the stale half rolls again
    for (int i = 0; i < 4; i++) if (s->want[i].kind == 0 || s->want[i].age > 12 * 60 || (look_rand() & 1)) roll_want(s, i);
    for (int i = 0; i < 2; i++) if (look_rand() & 1) roll_fear(s, i);
}

static void flash(const Sim *s, const char *what, int pts) {
    if ((int)(s - sims) != player) return;
    sprintf(wants_flash, "%s %d", what, pts); wants_flash_t = 200;
}
static void gain(Sim *s, int pts) { s->asp_pts += pts; if (s->asp_pts > ASP_MAX) s->asp_pts = ASP_MAX; if (s->asp_pts < 0) s->asp_pts = 0; }

void wants_event(Sim *s, int kind, int arg) {
    for (int i = 0; i < 4; i++) {
        int k = s->want[i].kind; if (k != kind) continue;
        if (k == W_CHAT_WITH && s->want[i].arg != arg) continue;
        if (k == W_FRIEND && s->want[i].arg != arg) continue;
        if (k == W_CASH && cash < (long)s->want[i].arg * 5000) continue;
        if ((k == W_SK_COOK || k == W_SK_BODY || k == W_SK_FUN) && arg < s->want[i].arg) continue;
        int pts = s->want[i].pts;
        gain(s, pts); flash(s, "WANT DONE", pts);
        if ((int)(s - sims) == player) audio_voice(VOICE_YAHOO);
        s->want[i].kind = 0; roll_want(s, i);
        if (k == W_CHAT) for (int j = 0; j < 4; j++) if (s->want[j].kind == W_CHAT) { s->want[j].kind = 0; roll_want(s, j); }   // (one chat is enough)
    }
}
void wants_fear(Sim *s, int kind) {
    for (int i = 0; i < 2; i++) if (s->fear[i].kind == kind) {
        int pts = s->fear[i].pts;
        gain(s, -pts); flash(s, "FEAR CAME TRUE", -pts);
        if ((int)(s - sims) == player) audio_voice(VOICE_CRY);
        roll_fear(s, i);
    }
}
void wants_minute(Sim *s) {
    // decay (per game hour: platinum 250, gold 190, green 60, red 90; it slows near the bottom)
    static const u8 dec[4] = { 90, 60, 190, 250 };
    int lv = wants_level(s);
    if (((clock_min + (int)(s - sims)) & 3) == 0) { int d = dec[lv]; if (s->asp_pts < 600) d = 20; s->asp_pts -= (d + 7) / 15; if (s->asp_pts < 0) s->asp_pts = 0; }
    for (int i = 0; i < 4; i++) { if (s->want[i].age < 60000) s->want[i].age++; if (s->want[i].age >= 2 * 24 * 60) { s->want[i].kind = 0; roll_want(s, i); } }   // two days unfulfilled: forgotten
    // the wants the clock can see on its own
    for (int i = 0; i < 4; i++) {
        int k = s->want[i].kind;
        if (k == W_CASH && cash >= (long)s->want[i].arg * 5000) wants_event(s, W_CASH, 0);
        if (k == W_SK_COOK && wants_skill_level(s->skill_cook) >= s->want[i].arg) wants_event(s, W_SK_COOK, 10);
        if (k == W_SK_BODY && wants_skill_level(s->skill_body) >= s->want[i].arg) wants_event(s, W_SK_BODY, 10);
        if (k == W_SK_FUN && wants_skill_level(s->skill_fun) >= s->want[i].arg) wants_event(s, W_SK_FUN, 10);
    }
}
int wants_level(const Sim *s) { return s->asp_pts >= 10000 ? ASP_PLATINUM : s->asp_pts >= 7000 ? ASP_GOLD : s->asp_pts >= 3000 ? ASP_GREEN : ASP_RED; }
int wants_desperate(const Sim *s) { return s->asp_pts < 500; }

void wants_text(const Sim *s, int slot, char *out) {
    int k = s->want[slot].kind, a = s->want[slot].arg;
    if (k == W_CHAT_WITH || k == W_FRIEND) sprintf(out, "%s %s", k == W_FRIEND ? "BEFRIEND" : wNm[k], (a >= 0 && a < nsims) ? sims[a].bore.first : "SOMEONE");
    else if (k == W_CASH) sprintf(out, "HAVE $%d", a * 5000);
    else if (k >= W_SK_COOK && k <= W_SK_FUN) sprintf(out, "%s %d", wNm[k], a);
    else strcpy(out, wNm[k]);
}
void fears_text(const Sim *s, int slot, char *out) { strcpy(out, fNm[s->fear[slot].kind]); }
