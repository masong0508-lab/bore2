// create_ui: CREATE A BORE, an overlay (CREATE.OVL). The flow of The Sims 3's Create-A-Sim (docs/CAS_SIMS3DS_WORKFLOW.md):
// seven steps down a rail (BASICS, LOOKS, HAIR, CLOTHES, STYLE, PERSONALITY, SUMMARY), category tabs inside a step, one choice
// at a time with LEFT / RIGHT, the Bore turning and the camera moving to what you are changing, a dice, BACK and NEXT, and a
// summary with a question before it is kept. It edits main.c's cas_edit (the Bore on the studio's table); main draws the studio
// and the Bore from the camera numbers set here (cas_*) and handles the name keyboard. It sits in the crowd's arena, so it
// costs no memory the rest of the time. update returns: 1 stay, 0 leave (nothing kept), 2 keep, 3 / 4 type the first / last name.
#include "look.h"
#include "gfx.h"
#include "ui.h"
#include "audio.h"
#include "ovl.h"
#include <psxgpu.h>
#include <psxpad.h>
#include <stdio.h>
#include <string.h>

extern Bore cas_edit;
extern int cas_fx, cas_fy, cas_dist, cas_pitch, cas_turn, cas_step, cas_go, cas_rs;
#define E cas_edit

enum { K_PICK, K_PICKZ, K_COL, K_SLIDE, K_AGE, K_FIRST, K_LAST, K_SIGN, K_ASP, K_LTW, K_TRAIT, K_THEME, K_RAND };   // PICKZ: a pick whose 0 is NONE (the dice leaves it off half the time)
typedef struct { const char *l; u8 id, k; } Rw;
typedef struct { const char *nm; const Rw *r; u8 n, cam; } Cat;
typedef struct { const char *nm; s8 icon; const Cat *c; u8 n; } Stp;
enum { CAM_BODY, CAM_FACE, CAM_HEAD, CAM_TORSO, CAM_LEGS, CAM_FEET };
#define P(l, id) { l, id, K_PICK }
#define Z(l, id) { l, id, K_PICKZ }
#define C(l, id) { l, id, K_COL }
#define S(l, id) { l, id, K_SLIDE }

// ---- the steps: what each category holds (every row of the old seven tabs is here) ----
static const Rw rMe[] = { { "FIRST NAME", 0, K_FIRST }, { "LAST NAME", 0, K_LAST }, { "AGE", 0, K_AGE }, P("SHAPE", LK_SHAPE), C("SKIN", LK_SKIN), S("SKIN TONE", LK_TONE), { "RANDOMIZE", 0, K_RAND } };
static const Rw rBody[] = { S("HEIGHT", LK_HEIGHT), S("WEIGHT", LK_WEIGHT), S("TORSO", LK_TORSO), S("ARMS", LK_ARMS), S("STANCE", LK_STANCE), S("BUTT", LK_BUTT), S("NECK", LK_NECK),
    S("NECK WIDTH", LK_NECKW), S("SHOULDERS", LK_SHOULW), S("CHEST", LK_CHESTW), S("BELLY", LK_BELLYW), S("WAIST", LK_WAISTW), S("HIPS", LK_HIPW) };
static const Rw rLimb[] = { S("LEGS", LK_LEGW), S("THIGHS", LK_THIGHW), S("CALVES", LK_CALFW), S("FEET", LK_FOOTSZ), S("ARM WIDTH", LK_ARMW), S("UPPER ARMS", LK_UARMW), S("FOREARMS", LK_FARMW), S("HANDS", LK_HANDSZ) };
static const Rw rEyes[] = { P("EYES", LK_EYES), C("EYE COLOUR", LK_EYECOL), S("EYE SIZE", LK_EYESZ), S("EYE SPACING", LK_EYESP), S("EYE HEIGHT", LK_EYEHT), S("EYE SHADE", LK_EYETONE), P("BROWS", LK_BROW), Z("GLASSES", LK_GLASS) };
static const Rw rMouth[] = { P("MOUTH", LK_MOUTH), S("MOUTH WIDTH", LK_MOUTHW), S("MOUTH HEIGHT", LK_MOUTHHT), P("NOSE", LK_NOSE), Z("CHEEKS", LK_CHEEK), S("JAW", LK_JAWW) };
static const Rw rMore[] = { P("EARS", LK_EARS), S("EAR LIFT", LK_EARLF), S("EAR WIDTH", LK_EARWID), Z("MAKE-UP", LK_X_MAKEUP), Z("LIPS", LK_X_LIPS), Z("MORE MARKS", LK_X_MARKS) };
static const Rw rHair[] = { P("STYLE", LK_HSTYLE), C("COLOUR", LK_HCOL), S("HAIR TONE", LK_HTONE) };
static const Rw rBeard[] = { Z("BEARD", LK_BEARD) };
static const Rw rHat[] = { Z("HAT", LK_HAT), P("HAT COLOUR", LK_HATCOL) };
static const Rw rSets[] = { { "OUTFIT", LK_X_THEME, K_THEME }, C("COLOUR 1", LK_X_COLA), C("COLOUR 2", LK_X_COLB) };
static const Rw rTop[] = { C("TOP", LK_TOP), S("TOP TONE", LK_TTONE), P("TOP STYLE", LK_TOPSTY), Z("TOP PATTERN", LK_X_TOPPAT), P("PATTERN SHADE", LK_X_PATTONE) };
static const Rw rLegs[] = { C("BOTTOM", LK_BOT), S("BOTTOM TONE", LK_BTONE), P("BOTTOM STYLE", LK_BOTSTY), Z("BOTTOM PATTERN", LK_X_BOTPAT), P("SHOES", LK_SHOE) };
static const Rw rWHead[] = { Z("HEAD", LK_X_HEAD), Z("FACE", LK_X_FACE), Z("EARS", LK_X_EARS), Z("NECK", LK_X_NECK) };
static const Rw rWBody[] = { Z("BACK", LK_X_BACK), Z("SHOULDER", LK_X_SHOULDER), Z("HOLDING", LK_X_HAND), Z("WAIST", LK_X_WAIST), Z("WRISTS", LK_X_WRIST), Z("FEET", LK_X_FEET) };
static const Rw rParts[] = { Z("TAIL", LK_TAIL), Z("HORNS", LK_HORNS), Z("BACK PART", LK_BACK), Z("HANDS", LK_CLAWS), Z("ANTENNAE", LK_ANTENNA), Z("PATTERN", LK_PATTERN), Z("PAINT", LK_PATCOL),
    Z("ANIMAL EARS", LK_FEARS), Z("MUZZLE", LK_MUZZLE), Z("FUR TAIL", LK_FTAIL), S("TAIL LENGTH", LK_TAILLEN), S("HORN SIZE", LK_HORNSZ), S("WING SIZE", LK_WINGSZ),
    S("ANTENNA LENGTH", LK_ANTLEN), S("ANTENNA SPREAD", LK_ANTSPR) };
static const Rw rMind[] = { { "STAR SIGN", 0, K_SIGN }, { "NEAT", LK_TRAIT0, K_TRAIT }, { "OUTGOING", LK_TRAIT0 + 1, K_TRAIT }, { "ACTIVE", LK_TRAIT0 + 2, K_TRAIT },
    { "PLAYFUL", LK_TRAIT0 + 3, K_TRAIT }, { "NICE", LK_TRAIT0 + 4, K_TRAIT } };
static const Rw rWish[] = { { "WISH", 0, K_ASP }, { "LIFE GOAL", 0, K_LTW } };
#define CAT(nm, a, cam) { nm, a, sizeof a / sizeof a[0], cam }
static const Cat cBasics[] = { CAT("ME", rMe, CAM_BODY), CAT("BODY", rBody, CAM_BODY), CAT("LIMBS", rLimb, CAM_BODY) };
static const Cat cLooks[] = { CAT("EYES", rEyes, CAM_FACE), CAT("FACE", rMouth, CAM_FACE), CAT("MORE", rMore, CAM_FACE) };
static const Cat cHair[] = { CAT("HAIR", rHair, CAM_HEAD), CAT("BEARD", rBeard, CAM_FACE), CAT("HAT", rHat, CAM_HEAD) };
static const Cat cClothes[] = { CAT("SETS", rSets, CAM_BODY), CAT("TOP", rTop, CAM_TORSO), CAT("LEGS", rLegs, CAM_LEGS) };
static const Cat cStyle[] = { CAT("HEAD", rWHead, CAM_HEAD), CAT("BODY", rWBody, CAM_BODY), CAT("PARTS", rParts, CAM_BODY) };
static const Cat cMind[] = { CAT("TRAITS", rMind, CAM_BODY), CAT("WISH", rWish, CAM_BODY) };
#define NSTEP 7
#define SUMMARY 6
static const Stp stp[NSTEP] = { { "BASICS", IC_PEOPLE, cBasics, 3 }, { "LOOKS", IC_SMILE, cLooks, 3 }, { "HAIR", IC_BRUSH, cHair, 3 }, { "CLOTHES", IC_SHIRT, cClothes, 3 },
    { "STYLE", IC_STAR, cStyle, 3 }, { "PERSONALITY", IC_HEART, cMind, 2 }, { "SUMMARY", IC_TICK, 0, 0 } };
static const char *const aspDesc[AS_PICK] = { "WORK HARD, GET PAID, MOVE UP AND SAVE.", "READ, LEARN SKILLS AND DO HOMEWORK.", "MAKE FRIENDS, CHAT AND GO TO PARTIES.",
    "EAT WELL, HAVE FUN AND GO OUT.", "BUY A HOME, COOK AND HAVE A BABY." };
static const s16 camT[6][3] = { { 660, 190, 360 }, { 420, 292, 170 }, { 480, 280, 140 }, { 560, 215, 300 }, { 600, 115, 300 }, { 500, 55, 350 } };   // distance, height looked at, tilt

// ---- where things are (320 x 240, everything inside 22..298 x 16..224) ----
#define PX 150
#define PW 148
#define PY 16
#define PH 208
#define CX0 177        // the rows' left edge
#define CW 118         // and their width
#define RY0 58         // the first row
#define RH 24
#define NVIS 6

static int t, step, cat, row, seen, dirty, modal, msel, sumsel, inlife, user, cur_turn, hold, hdir, c_dist, c_fy, c_pitch;

static const Cat *curcat(void) { return &stp[step].c[cat]; }
static const Rw *currow(void) { return &curcat()->r[row]; }
static void go(int s, int c) { step = s < 0 ? 0 : s >= NSTEP ? NSTEP - 1 : s; cat = c; row = 0; seen |= 1 << step; user = 0; hold = 0; if (step == SUMMARY) sumsel = 6; }
static void cr_open(int a) {
    inlife = a; t = step = cat = row = dirty = modal = msel = user = cur_turn = hold = hdir = 0; seen = 1; sumsel = 6;
    c_dist = camT[0][0]; c_fy = camT[0][1]; c_pitch = camT[0][2]; cas_step = 0;
}
static int row_cam(void) {
    if (step == SUMMARY) return CAM_BODY;
    const Rw *r = currow(); if (r->id == LK_SHOE || r->id == LK_X_FEET) return CAM_FEET;
    return curcat()->cam;
}
static int turn_target(void) {
    if (step == SUMMARY) return (t * 9) & 4095;
    switch (currow()->id) {
        case LK_HSTYLE: return (t * 9) & 4095;                                   // the hair turns all the way round
        case LK_BACK: case LK_TAIL: case LK_FTAIL: case LK_BUTT: case LK_X_BACK: case LK_TAILLEN: case LK_WINGSZ: return 2048;   // things on the back: show the back
        case LK_EARS: case LK_EARLF: case LK_EARWID: case LK_FEARS: case LK_X_EARS: case LK_HORNS: return 800;   // ears and horns: a three-quarter view
        default: return 0;
    }
}
static void ease(int *c, int tg) { int d = tg - *c; if (d > -3 && d < 3) *c = tg; else *c += d / 5 ? d / 5 : d > 0 ? 1 : -1; }

static int rnd(int n) { return (int)(look_rand() % (u32)n); }
static void pick_random(const Rw *r) {
    int cnt = look_count(r->id), v = 0;
    if (r->k == K_PICKZ && (rnd(3) != 0)) v = 0;
    else for (int k = 0; k < 20; k++) { v = rnd(cnt); if (look_allowed(&E, r->id, v)) break; v = 0; }
    E.look[r->id] = (u8)v;
}
static void dice(void) {
    if (step == SUMMARY) { audio_voice(VOICE_NAH); return; }
    if (step == 0) look_random(&E);                                              // the whole Bore, name too
    else if (step == 5) { look_set_sign(&E, rnd(12)); E.asp = (u8)rnd(AS_PICK); E.ltw = (u8)rnd(LTN); }
    else { for (int c = 0; c < stp[step].n; c++) { const Cat *ct = &stp[step].c[c]; for (int i = 0; i < ct->n; i++) { const Rw *r = &ct->r[i]; if (r->k == K_PICK || r->k == K_PICKZ || r->k == K_COL) pick_random(r); } }
           look_fix(&E); look_build(&E); }
    dirty = 1; audio_voice(VOICE_LAUGH);
}
static void change(const Rw *r, int dir) {
    int cnt, v;
    switch (r->k) {
        case K_AGE: { int st = E.stage; do st = (st + AG_N + dir) % AG_N; while (st == AG_BABY); E.stage = (u8)st; look_fix(&E); look_build(&E); break; }   // (a baby cannot be played)
        case K_SIGN: look_set_sign(&E, (look_sign_of(&E) + 12 + dir) % 12); break;
        case K_ASP: E.asp = (u8)((E.asp + AS_PICK + dir) % AS_PICK); break;
        case K_LTW: E.ltw = (u8)((E.ltw + LTN + dir) % LTN); break;
        case K_TRAIT: v = E.tr[r->id - LK_TRAIT0] + dir; E.tr[r->id - LK_TRAIT0] = (u8)(v < 0 ? 0 : v > 10 ? 10 : v); break;
        case K_SLIDE: v = ((E.look[r->id] + 4) % 9) + dir; v = v < 0 ? 0 : v > 8 ? 8 : v; E.look[r->id] = (u8)((v + 5) % 9); look_build(&E); break;   // (stored 0 is the middle: shown as 0 = least .. 8 = most, stopping at both ends)
        case K_FIRST: case K_LAST: case K_RAND: return;
        default:
            cnt = look_count(r->id); v = E.look[r->id];
            for (int k = 0; k < cnt; k++) { v = (v + cnt + dir) % cnt; if (look_allowed(&E, r->id, v)) break; }
            E.look[r->id] = (u8)v;
            if (r->k == K_THEME) look_theme(&E, v);                              // an outfit: dressed all at once
            look_build(&E);
    }
    dirty = 1; audio_voice(VOICE_AGREE);
}
static void tab_move(int d) {   // L1 / R1: the next category, and past the last one the next step
    int nc = stp[step].n;
    if (step == SUMMARY) { if (d < 0) go(SUMMARY - 1, 0); return; }
    if (d > 0 && cat + 1 < nc) { cat++; row = 0; user = 0; }
    else if (d < 0 && cat > 0) { cat--; row = 0; user = 0; }
    else if (d > 0) go(step + 1, 0);
    else if (step > 0) go(step - 1, stp[step - 1].n - 1);
}

static int modal_update(int pr) {
    if (pr & (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT)) msel ^= 1;
    if (pr & PAD_CIRCLE) { modal = 0; return 1; }
    if (pr & PAD_CROSS) { if (msel == 0) return modal == 1 ? 2 : 0; modal = 0; }
    return 1;
}
static int cr_update(int held, int pr, int steps) {
    t++; if (steps < 1) steps = 1;
    if (cas_go >= 0) { go(cas_go, 0); cas_go = -1; }
    if (modal) return modal_update(pr);
    int n = stp[step].n ? curcat()->n : 0;
    if (step == SUMMARY) {
        if (pr & PAD_UP) sumsel = (sumsel + 6) % 7;       // the six steps (go back and change one), then KEEP (6)
        if (pr & PAD_DOWN) sumsel = (sumsel + 1) % 7;
        if (pr & PAD_CROSS) { if (sumsel < 6) go(sumsel, 0); else { modal = 1; msel = 0; } }
        if (pr & PAD_CIRCLE) go(SUMMARY - 1, 0);
    } else {
        int d = !!(pr & PAD_RIGHT) - !!(pr & PAD_LEFT);
        if (!d) { int h = !!(held & PAD_RIGHT) - !!(held & PAD_LEFT); if (h && h == hdir) { if (++hold > 18 && !(hold & 3)) d = h; } else hold = 0; hdir = h; } else { hdir = d; hold = 0; }
        if (pr & PAD_UP) { row = (row + n - 1) % n; user = 0; }
        if (pr & PAD_DOWN) { row = (row + 1) % n; user = 0; }
        const Rw *r = currow();
        if (d) change(r, d);
        if (pr & PAD_TRIANGLE) {
            if (r->k == K_FIRST) { char b[NAME_LEN + 4]; look_first_gen(look_rand(), b); strncpy(E.first, b, NAME_LEN - 1); E.first[NAME_LEN - 1] = 0; dirty = 1; audio_voice(VOICE_LAUGH); }
            else if (r->k == K_LAST) { char b[NAME_LEN + 4]; look_last_gen(look_rand(), b); strncpy(E.last, b, NAME_LEN - 1); E.last[NAME_LEN - 1] = 0; dirty = 1; audio_voice(VOICE_LAUGH); }
            else dice();
        }
        if (pr & PAD_SQUARE) return 3;
        if (pr & PAD_L1) tab_move(-1);
        if (pr & PAD_R1) tab_move(1);
        if (pr & PAD_CROSS) {
            if (r->k == K_FIRST) return 3;
            if (r->k == K_LAST) return 4;
            if (r->k == K_RAND) { look_random(&E); dirty = 1; audio_voice(VOICE_LAUGH); }   // (the whole Bore, name too)
            else { go(step + 1, 0); audio_voice(VOICE_AGREE); }
        }
        if (pr & PAD_CIRCLE) { if (step > 0) go(step - 1, 0); else if (dirty) { modal = 2; msel = 1; } else return 0; }
    }
    if (pr & (PAD_START | PAD_SELECT)) { if (step == SUMMARY) { modal = 1; msel = 0; } else go(SUMMARY, 0); }
    if (step == SUMMARY && (pr & PAD_L1)) go(SUMMARY - 1, 0);
    // the camera and the turn
    if (held & PAD_L2) user -= 40 * steps;
    if (held & PAD_R2) user += 40 * steps;
    user += cas_rs / 3;
    int cm = row_cam(), tg = turn_target();
    for (int i = 0; i < steps; i++) {
        ease(&c_dist, camT[cm][0]); ease(&c_fy, camT[cm][1]); ease(&c_pitch, camT[cm][2]);
        int d = ((tg - cur_turn + 2048) & 4095) - 2048; cur_turn += d / 6 ? d / 6 : d > 0 ? 1 : d < 0 ? -1 : 0;
    }
    cas_dist = c_dist; cas_fy = c_fy; cas_pitch = c_pitch; cas_fx = c_dist * 35 / 100; cas_turn = (cur_turn + user) & 4095; cas_step = step;
    return 1;
}

// ---- drawing ----
static void ctext(int cx, int y, int w, const char *s, u32 c) { if (ui_tw(1, s) <= w) ui_textc(cx, y, 1, s, c); else ui_textfit(cx - w / 2, y, w, s, c); }
static void rail(void) {
    ui_round(PX + 3, 38, 22, 184, 10, UI_HILITE, UI_PANEL, 0);
    for (int i = 0; i < NSTEP; i++) {
        int cy = 50 + i * 26, cx = PX + 14, on = i == step, sn = seen >> i & 1;
        if (on) { ui_disc(cx, cy, 23, UI_BLUE2); ui_disc(cx, cy, 19, UI_BLUE); ui_icon(cx - 6, cy - 6, stp[i].icon, UI_WHITE); }
        else { ui_disc(cx, cy, 18, sn ? UI_RGB(120, 200, 245) : UI_RGB(200, 208, 220)); ui_icon(cx - 6, cy - 6, stp[i].icon, sn ? UI_WHITE : UI_SOFT); }
        if (sn && !on) ui_disc(cx + 7, cy - 7, 6, UI_GREEN);
    }
}
static void value_line(const Rw *r, int y, int sel) {
    int mid = CX0 + CW / 2 - 1, k = r->k; char b[40]; u32 ink = sel ? UI_BLUE2 : UI_INK;
    if (k == K_SLIDE) { ui_slider(CX0 + 14, y + 1, CW - 30, (E.look[r->id] + 4) % 9, 9, sel); return; }
    if (k == K_TRAIT) { int v = E.tr[r->id - LK_TRAIT0]; for (int i = 0; i < 10; i++) { u32 c = i < v ? UI_AMBER : UI_CARD2; gfx_rect(CX0 + 14 + i * 9, y + 2, 7, 7, (int)(c & 255), (int)(c >> 8 & 255), (int)(c >> 16 & 255), 0); } return; }
    if (k == K_COL) {
        int cnt = look_count(r->id), cur = E.look[r->id], n = cnt < 7 ? cnt : 7, st = cnt <= 7 ? 0 : cur - 3;
        if (st < 0) st = 0; if (st > cnt - n) st = cnt - n;
        int x0 = mid - (n * 13) / 2;
        for (int i = 0; i < n; i++) { int v = st + i, x = x0 + i * 13;
            if (v == cur) { gfx_rect(x - 2, y - 1, 15, 13, 26, 112, 190, 0); gfx_rect(x - 1, y, 13, 11, 255, 255, 255, 0); }
            gfx_rect_c(x, v == cur ? y + 1 : y + 2, 11, v == cur ? 8 : 7, look_option_colour(r->id, v), 0); }
        return;
    }
    const char *s = 0;
    switch (k) {
        case K_AGE: s = look_stage_name(E.stage); break;
        case K_FIRST: s = E.first; break;
        case K_LAST: s = E.last; break;
        case K_RAND: s = "PRESS X: A NEW BORE"; break;
        case K_SIGN: s = look_sign_name(look_sign_of(&E)); break;
        case K_ASP: s = look_asp_name(E.asp); break;
        case K_LTW: sprintf(b, "GOAL %d", E.ltw + 1); s = b; break;
        default: s = look_option_name(r->id, E.look[r->id]); if (!s) { sprintf(b, "%d OF %d", E.look[r->id] + 1, look_count(r->id)); s = b; }
    }
    ctext(mid, y + 2, CW - 24, s, ink);
}
static void draw_rows(void) {
    const Cat *ct = curcat(); int n = ct->n, first = row - NVIS / 2 + 1; if (first > n - NVIS) first = n - NVIS; if (first < 0) first = 0;
    for (int i = first; i < n && i < first + NVIS; i++) {
        const Rw *r = &ct->r[i]; int y = RY0 + (i - first) * RH, sel = i == row;
        if (sel) { ui_round(CX0, y, CW, RH - 1, 6, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(CX0, y + 3, 2, RH - 7, 38, 146, 222, 0); }
        ui_text(CX0 + 6, y + 2, 1, r->l, sel ? UI_BLUE2 : UI_SOFT);
        if (r->k == K_FIRST || r->k == K_LAST || r->k == K_RAND) ui_icon(CX0 + CW - 16, y, IC_DICE, sel ? UI_BLUE : UI_LINE);
        if (r->k != K_FIRST && r->k != K_LAST && r->k != K_RAND) { ui_text(CX0 + 4, y + 12, 1, "<", sel ? UI_BLUE : UI_LINE); ui_text(CX0 + CW - 11, y + 12, 1, ">", sel ? UI_BLUE : UI_LINE); }
        value_line(r, y + 11, sel);
    }
    if (n > NVIS) { int th = NVIS * RH * NVIS / n, ty = RY0 + (NVIS * RH - th) * first / (n - NVIS); gfx_rect(CX0 + CW + 1, RY0, 2, NVIS * RH, 200, 208, 220, 0); gfx_rect(CX0 + CW + 1, ty, 2, th, 38, 146, 222, 0); }
    if (step == 5 && cat == 1) {   // the wish: what it means
        int y = RY0 + 2 * RH + 8; ui_round(CX0, y, CW, 52, 6, UI_PANEL, UI_CARD, 0);
        ctext(CX0 + CW / 2 - 1, y + 5, CW - 8, look_asp_name(E.asp), UI_BLUE2);
        ui_wrapc(CX0 + 6, y + 18, CW - 12, 3, aspDesc[E.asp % AS_PICK], UI_INK);
    }
}
static void chips(void) {
    int nc = stp[step].n, w = (CW + 4 - (nc - 1) * 2) / nc;
    for (int i = 0; i < nc; i++) { int x = CX0 - 2 + i * (w + 2), on = i == cat;
        ui_round(x, 39, w, 15, 6, on ? UI_RGB(70, 176, 238) : UI_PANEL, on ? UI_BLUE2 : UI_CARD2, 0);
        ctext(x + w / 2, 43, w - 2, stp[step].c[i].nm, on ? UI_WHITE : UI_INK); }
}
static void draw_summary(void) {
    char b[48]; int y = 40;
    sprintf(b, "%s %s", E.first, E.last);
    if (ui_tw(2, b) <= CW - 4) ui_textc(CX0 + CW / 2, y, 2, b, UI_INK); else ctext(CX0 + CW / 2, y + 3, CW - 4, b, UI_INK);
    sprintf(b, "%s, %s", look_stage_name(E.stage), look_sign_name(look_sign_of(&E))); ctext(CX0 + CW / 2, y + 17, CW, b, UI_SOFT);
    sprintf(b, "WISH: %s", look_asp_name(E.asp)); ctext(CX0 + CW / 2, y + 27, CW, b, UI_BLUE2);
    gfx_rect(CX0, y + 38, CW, 1, 196, 206, 220, 0);
    for (int i = 0; i < 6; i++) {
        int ry = y + 42 + i * 17, sel = sumsel == i; const char *v = "";
        switch (i) {
            case 0: v = look_shape_name(E.look[LK_SHAPE]); break;
            case 1: v = look_option_name(LK_EYES, E.look[LK_EYES]); break;
            case 2: v = look_option_name(LK_HSTYLE, E.look[LK_HSTYLE]); break;
            case 3: v = look_option_name(LK_X_THEME, E.look[LK_X_THEME]); break;
            case 4: { int c = 0; for (int k = LK_X_HEAD; k <= LK_X_NECK; k++) c += E.look[k] != 0; c += E.look[LK_X_BACK] != 0; c += E.look[LK_X_HAND] != 0; c += E.look[LK_X_WAIST] != 0; sprintf(b, "%d ITEMS", c); v = b; } break;
            default: v = look_sign_name(look_sign_of(&E));
        }
        if (sel) { ui_round(CX0, ry - 1, CW, 16, 5, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(CX0, ry + 2, 2, 10, 38, 146, 222, 0); }
        ui_icon(CX0 + 4, ry, stp[i].icon, sel ? UI_BLUE : UI_SOFT);
        ui_text(CX0 + 19, ry + 2, 1, i == 5 ? "TRAITS" : stp[i].nm, sel ? UI_BLUE2 : UI_INK);
        if (v && v[0]) { int w = ui_tw(1, v); if (w > 50) { ui_textfit(CX0 + CW - 52, ry + 2, 50, v, UI_SOFT); } else ui_text(CX0 + CW - 4 - w, ry + 2, 1, v, UI_SOFT); }
    }
    ui_button(CX0, 185, CW, 18, inlife ? "KEEP THIS LOOK" : "KEEP THIS BORE", IC_TICK, sumsel == 6);
}
static void draw_modal(void) {
    ui_dim();
    int top = ui_window(40, 74, 240, 96, modal == 1 ? "ALL DONE?" : "LEAVE?", IC_HELP);
    ui_wrapc(52, top + 4, 216, 2, modal == 1 ? (inlife ? "KEEP THIS NEW LOOK FOR YOUR BORE?" : "KEEP THIS BORE AND GO ON?") : "THIS BORE WILL NOT BE KEPT.", UI_INK);
    ui_button(52, top + 34, 104, 18, modal == 1 ? "YES, KEEP IT" : "YES, LEAVE", IC_TICK, msel == 0);
    ui_button(164, top + 34, 104, 18, modal == 1 ? "NO, GO BACK" : "NO, STAY", IC_BX + 1, msel == 1);
}
static void cr_draw(void) {
    char b[40];
    // the name card, and what the pad does
    sprintf(b, "%s %s", E.first, E.last);
    ui_card(22, 16, 124, 32);
    if (ui_tw(2, b) <= 112) ui_textc(84, 20, 2, b, UI_INK); else ctext(84, 22, 112, b, UI_INK);
    sprintf(b, "%s, %s", look_stage_name(E.stage), look_sign_name(look_sign_of(&E))); ctext(84, 37, 112, b, UI_SOFT);
    ui_card(22, 192, 124, 32);
    ui_hints(84, 196, step == SUMMARY ? "X PICK|O BACK" : "X NEXT|O BACK", 1);
    ui_hints(84, 209, "TRI DICE|L2 R2 TURN", 1);
    // the panel
    sprintf(b, "%d/%d %s", step + 1, NSTEP, stp[step].nm);
    ui_window_s(PX, PY, PW, PH, b);
    rail();
    if (step == SUMMARY) draw_summary();
    else {
        chips(); draw_rows();
        const Rw *r = currow();
        ui_hints(CX0 + CW / 2, 208, r->k == K_FIRST || r->k == K_LAST ? "X TYPE|TRI NEW" : r->k == K_RAND ? "X ROLL|L1 R1 TAB" : "L1 R1 TAB", 1);
    }
#ifdef CAS_DBG
    { sprintf(b, "D%d F%d X%d P%d T%d", cas_dist, cas_fy, cas_fx, cas_pitch, cas_turn); ui_text(24, 60, 1, b, UI_WHITE); }
#endif
    if (modal) draw_modal();
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, cr_open, cr_update, cr_draw };
