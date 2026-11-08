// dog_ui: the CREATE-A-DOG screen, an overlay (DOG.OVL). Five tabs of rows like the Create-A-Bore screen: sliders, choices, colours, traits, a name.
// The dog is drawn by items_draw_dog (the same model that follows you about); START keeps it, O leaves it as it was.
#include "dog.h"
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "ovl.h"
#include "items.h"
#include <psxgpu.h>
#include <psxpad.h>
#include <stdio.h>
#include <string.h>

enum { K_SLIDER, K_CHOICE, K_COLOUR, K_PIPS, K_NAME, K_DONE };
typedef struct { const char *label; u8 kind, id, n; const char *const *names; } Row;
static const char *const EARS[8] = { "FLOPPY", "POINTY", "ROUND", "LONG", "FOLDED", "BAT", "TINY", "NONE" };
static const char *const TAILS[7] = { "STRAIGHT", "CURLED", "BUSHY", "STUB", "WHIP", "FLUFFY", "UP" };
static const char *const FURS[4] = { "SHORT", "MEDIUM", "FLUFFY", "SHAGGY" };
static const char *const PATS[DOG_NPAT] = { "SOLID", "SPOTS", "EYE PATCH", "SADDLE", "SOCKS", "BELLY", "STRIPES", "TUXEDO", "MASK" };
static const char *const EYEC[8] = { "BROWN", "DARK", "BLUE", "GREEN", "TEAL", "GOLD", "AMBER", "BLACK" };
static const char *const EYES[6] = { "ROUND", "SLEEPY", "SPARKLE", "WINK", "SQUINT", "BIG" };
static const char *const OFFON[2] = { "OFF", "ON" };
static const char *const MOUTHS[4] = { "CLOSED", "TONGUE", "SMILE", "PANTING" };
static const char *const COLLARS[8] = { "NONE", "PLAIN", "BANDANA", "BOW TIE", "BELL", "SPIKES", "SCARF", "CHAIN" };
static const char *const HATS[8] = { "NONE", "CAP", "PARTY HAT", "CROWN", "BOW", "ANTLERS", "TOP HAT", "FLOWER" };
static const char *const GLASSES[4] = { "NONE", "ROUND", "SHADES", "MONOCLE" };
static const char *const OUTFITS[6] = { "NONE", "SWEATER", "CAPE", "VEST", "BACKPACK", "LIFE JACKET" };
static const char *const TRN[DOG_TR] = { "PLAYFUL", "LAZY", "LOYAL", "HUNGRY", "BRAVE" };
static const char *const NAMES[32] = { "BUDDY", "MAX", "BELLA", "ROCKY", "LUNA", "COOKIE", "DUKE", "PEPPER", "MOCHI", "BEAR", "NOODLE", "SCOUT", "WAFFLES", "TANK", "ZIGGY", "BISCUIT",
    "COCO", "OREO", "TOBY", "DAISY", "SADIE", "JACK", "PEANUT", "MILO", "ROXY", "BOLT", "FIFI", "GIZMO", "HANK", "PICKLES", "SPARKY", "BORE" };
#define NTAB 5
static const Row R0[] = { { "BREED", K_CHOICE, DL_BREED, DOG_NBREED, dog_breed_name }, { "SIZE", K_SLIDER, DL_SIZE, 9, 0 }, { "LEG LENGTH", K_SLIDER, DL_LEGS, 9, 0 }, { "BODY LENGTH", K_SLIDER, DL_BODY, 9, 0 },
    { "HEAD SIZE", K_SLIDER, DL_HEAD, 9, 0 }, { "SNOUT", K_SLIDER, DL_SNOUT, 9, 0 }, { "EARS", K_CHOICE, DL_EARS, 8, EARS }, { "EAR SIZE", K_SLIDER, DL_EARSZ, 9, 0 }, { "TAIL", K_CHOICE, DL_TAIL, 7, TAILS },
    { "TAIL LENGTH", K_SLIDER, DL_TAILSZ, 9, 0 }, { "FUR", K_CHOICE, DL_FUR, 4, FURS } };
static const Row R1[] = { { "COAT", K_COLOUR, DL_C1, DOG_NPAL, 0 }, { "2ND COLOUR", K_COLOUR, DL_C2, DOG_NPAL, 0 }, { "PATTERN", K_CHOICE, DL_PAT, DOG_NPAT, PATS }, { "PATTERN COLOUR", K_COLOUR, DL_PATC, DOG_NPAL, 0 }, { "NOSE COLOUR", K_COLOUR, DL_NOSEC, DOG_NPAL, 0 } };
static const Row R2[] = { { "EYE COLOUR", K_CHOICE, DL_EYEC, 8, EYEC }, { "EYE SIZE", K_SLIDER, DL_EYESZ, 9, 0 }, { "EYE STYLE", K_CHOICE, DL_EYEST, 6, EYES }, { "BROW SPOTS", K_CHOICE, DL_BROWS, 2, OFFON },
    { "MOUTH", K_CHOICE, DL_MOUTH, 4, MOUTHS }, { "NOSE SIZE", K_SLIDER, DL_NOSESZ, 9, 0 } };
static const Row R3[] = { { "COLLAR", K_CHOICE, DL_COLLAR, 8, COLLARS }, { "COLLAR COLOUR", K_COLOUR, DL_COLLARC, DOG_NPAL, 0 }, { "HAT", K_CHOICE, DL_HAT, 8, HATS }, { "HAT COLOUR", K_COLOUR, DL_HATC, DOG_NPAL, 0 },
    { "GLASSES", K_CHOICE, DL_GLASS, 4, GLASSES }, { "OUTFIT", K_CHOICE, DL_OUTFIT, 6, OUTFITS }, { "OUTFIT COLOUR", K_COLOUR, DL_OUTC, DOG_NPAL, 0 } };
static const Row R4[] = { { "PLAYFUL", K_PIPS, 0, 11, 0 }, { "LAZY", K_PIPS, 1, 11, 0 }, { "LOYAL", K_PIPS, 2, 11, 0 }, { "HUNGRY", K_PIPS, 3, 11, 0 }, { "BRAVE", K_PIPS, 4, 11, 0 }, { "NAME", K_NAME, 0, 32, 0 }, { "KEEP THIS DOG", K_DONE, 0, 0, 0 } };
static const Row *const TABR[NTAB] = { R0, R1, R2, R3, R4 };
static const u8 TABN[NTAB] = { 11, 5, 6, 7, 7 };
static const char *const TABNM[NTAB] = { "BODY", "COAT", "FACE", "WEAR", "TRAITS AND NAME" };
static const s8 TABIC[NTAB] = { IC_HEART, IC_BRUSH, IC_SMILE, IC_SHIRT, IC_STAR };

static DogLook ed;
static int tab, row, turn, anim, nameix;
static const char *msg; static int msgT;

static void keep(void) {
    pet_dog = ed; if (!pet_kind || pet_kind == 2) { pet_kind = 1; pet_hun = 100; pet_zone = -1; }
    sim_note("YOUR NEW DOG FOLLOWS YOU HOME"); audio_voice(VOICE_YAHOO);
}
void dog_open(int arg) {
    if (arg == 1) { ed = dog_edit; tab = dog_tab; row = dog_row; }   // (back from the typing screen)
    else if (pet_kind == 1) ed = pet_dog; else dog_default(&ed);
    if (arg != 1) { tab = 0; row = 0; } turn = 0; anim = 0; msg = 0; msgT = 0; nameix = 0;
    for (int i = 0; i < 32; i++) if (!strcmp(NAMES[i], ed.name)) nameix = i;
}
static int tr_total(void) { int t = 0; for (int i = 0; i < DOG_TR; i++) t += ed.tr[i]; return t; }
static void change(const Row *r, int d) {
    if (r->kind == K_SLIDER || r->kind == K_CHOICE || r->kind == K_COLOUR) {
        int v = (ed.look[r->id] + r->n + d) % r->n;
        if (r->id == DL_BREED) dog_breed(&ed, v); else ed.look[r->id] = (u8)v;
    } else if (r->kind == K_PIPS) {
        int v = ed.tr[r->id] + d; if (v < 0 || v > 10) return;
        if (d > 0 && tr_total() >= 30) { msg = "TRAITS HOLD 30 POINTS IN ALL"; msgT = 90; audio_voice(VOICE_NAH); return; }
        ed.tr[r->id] = (u8)v;
    } else if (r->kind == K_NAME) { nameix = (nameix + 32 + d) % 32; strcpy(ed.name, NAMES[nameix]); }
}
int dog_update(int hd, int pr, int steps) {
    anim += steps; if (msgT > 0) { msgT -= steps; if (msgT <= 0) msg = 0; }
    if (hd & PAD_L2) turn -= 40 * steps; if (hd & PAD_R2) turn += 40 * steps;
    int n = TABN[tab]; const Row *rows = TABR[tab];
    if (pr & PAD_UP) row = (row + n - 1) % n; if (pr & PAD_DOWN) row = (row + 1) % n;
    if (pr & PAD_L1) { tab = (tab + NTAB - 1) % NTAB; row = 0; } if (pr & PAD_R1) { tab = (tab + 1) % NTAB; row = 0; }
    if (pr & PAD_LEFT) change(&rows[row], -1); if (pr & PAD_RIGHT) change(&rows[row], 1);
    if (pr & PAD_SQUARE) { dog_random(&ed, (u32)anim * 7919u + (u32)turn); for (int i = 0; i < 32; i++) if (!strcmp(NAMES[i], ed.name)) nameix = i; audio_voice(VOICE_FINISHED); }
    if (pr & PAD_TRIANGLE) { dog_default(&ed); nameix = 0; }
    if ((pr & PAD_CROSS) && rows[row].kind == K_DONE) { keep(); return 0; }
    if ((pr & PAD_CROSS) && rows[row].kind == K_NAME) { dog_edit = ed; dog_tab = tab; dog_row = row; dog_want_name = 1; dog_resume = 1; return 0; }   // the typing screen (main.c), then back here
    if (pr & PAD_START) { keep(); return 0; }
    if (pr & PAD_CIRCLE) return 0;
    return 1;
}
void dog_draw(void) {
    // the studio: a warm backdrop, the dog on a little round rug, turning
    gfx_set_clear(14, 16, 24); gfx_set_ambient(100, 100); gfx_sky(0x6a5040u, 0xb09070u);
    { int bx = 0, bz = -(6 * TILE), yaw = 2048, pitch = 220, dist = 480; int fx = bx - 105, fy = -55;
      cam.at.vx = fx; cam.at.vy = fy; cam.at.vz = bz;
      cam.eye.vx = fx - ((isin(yaw) * ((dist * icos(pitch)) >> 12)) >> 12); cam.eye.vz = bz - ((icos(yaw) * ((dist * icos(pitch)) >> 12)) >> 12); cam.eye.vy = fy - ((dist * isin(pitch)) >> 12);
      gfx_camera_update(); gfx_set_fog(0, 0);
      VECTOR p; setVector(&p, bx, 0, bz);
      gfx_shadow(&p, 150);
      items_draw_dog(bx, bz, (2048 + 380 + turn / 4) & 4095, 0, &ed, anim * 20); }
    int top = ui_window(166, 4, 150, 232, TABNM[tab], TABIC[tab]);
    const Row *rows = TABR[tab]; int n = TABN[tab], first = MAX(0, row - 7), shown = MIN(n, first + 9); char b[40];
    for (int i = first; i < shown; i++) {
        const Row *r = &rows[i]; int y = top + (i - first) * 19, sel = i == row;
        if (sel) { ui_round(170, y - 1, 142, 19, 6, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(170, y + 2, 2, 13, 38, 146, 222, 0); }
        ui_text(176, y + 1, 1, r->label, sel ? UI_BLUE2 : UI_INK);
        if (sel && r->kind != K_DONE) { ui_text(292, y + 1, 1, "<", UI_BLUE); ui_text(302, y + 1, 1, ">", UI_BLUE); }
        if (r->kind == K_SLIDER) ui_slider(176, y + 9, 128, ed.look[r->id], 9, sel);
        else if (r->kind == K_PIPS) ui_pips(176, y + 10, ed.tr[r->id], 10, UI_AMBER);
        else if (r->kind == K_COLOUR) { int cr, cg, cb; dog_pal(ed.look[r->id], &cr, &cg, &cb);
            ui_round(176, y + 9, 100, 8, 3, UI_RGB(cr, cg, cb), UI_RGB(cr * 3 / 4, cg * 3 / 4, cb * 3 / 4), 0); ui_text(180, y + 9, 1, dog_pal_name[ed.look[r->id] % DOG_NPAL], (cr + cg + cb) > 380 ? UI_INK : UI_WHITE); }
        else if (r->kind == K_CHOICE) { const char *v = r->id == DL_BREED ? dog_breed_name[ed.look[DL_BREED] % DOG_NBREED] : r->names[ed.look[r->id] % r->n]; ui_text(286 - ui_tw(1, v), y + 1, 1, v, UI_SOFT); }
        else if (r->kind == K_NAME) ui_text(240 - ui_tw(1, ed.name) / 2, y + 1, 1, ed.name, UI_BLUE2);
        else if (r->kind == K_DONE) ui_text(176, y + 1, 1, "", UI_INK);
    }
    if (tab == 4) { sprintf(b, "TRAIT POINTS %d / 30", tr_total()); ui_text(176, top + 7 * 19 + 2, 1, b, UI_SOFT); }
    if (msg) { int w = ui_tw(1, msg) + 16; ui_round(160 - w / 2, 190, w, 16, 6, UI_WHITE, UI_PANEL, 0); ui_textc(160, 195, 1, msg, UI_INK); }
    ui_hints(6, 190, "L2 R2 TURN", 0); ui_hints(6, 204, "SQ RANDOM|TRI RESET", 0); ui_hints(6, 218, tab == 4 ? "L1 R1 TAB|X TYPE NAME|START KEEP|O LEAVE" : "L1 R1 TAB|START KEEP|O LEAVE", 0);
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, dog_open, dog_update, dog_draw };
