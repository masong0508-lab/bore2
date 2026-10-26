// BORE 2 for the DSi, step D1: the neighbourhood.
// Bottom screen: the 2D neighbourhood map. Tap a lot with the stylus (or use the D-pad) to pick it.
// Top screen: that lot in 3D. L/R change town, A/B zoom, START re-rolls the world.
// Fixed point only; the world is bigmap.c, shared with the PS1 build (nothing of it is stored).
#include <nds.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "bigmap.h"

int bsprintf(char *o, const char *f, ...) { va_list a; va_start(a, f); int r = vsprintf(o, f, a); va_end(a); return r; }
int bsnprintf(char *o, unsigned cap, const char *f, ...) { va_list a; va_start(a, f); int r = vsnprintf(o, cap, f, a); va_end(a); return r; }
void boot(const char *s) { (void)s; }
void loading(const char *w, int p) { (void)w; (void)p; }

// ---------- the bottom screen: a 256 x 256 16-bit bitmap ----------
static u16 *scr, *scr_sub, *hud;
#define C15(r, g, b) (RGB15(r, g, b) | BIT(15))
static void px(int x, int y, u16 c) { if ((unsigned)x < 256 && (unsigned)y < 192) scr[y * 256 + x] = c; }
static void rect(int x, int y, int w, int h, u16 c) {
    if (x < 0) { w += x; x = 0; } if (y < 0) { h += y; y = 0; } if (x + w > 256) w = 256 - x; if (y + h > 192) h = 192 - y; if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++) { u16 *p = scr + (y + j) * 256 + x; for (int i = 0; i < w; i++) p[i] = c; }
}
static const char *const FONT[] = {   // 3 x 5 glyphs, five rows of three bits each: A-Z, 0-9, then : - . / ,
    "010101111101101", "110101110101110", "011100100100011", "110101101101110", "111100110100111", "111100110100100", "011100101101011",
    "101101111101101", "111010010010111", "001001001101010", "101101110101101", "100100100100111", "101111111101101", "110101101101101",
    "010101101101010", "110101110100100", "010101101111011", "110101110101101", "011100010001110", "111010010010010", "101101101101111",
    "101101101101010", "101101111111101", "101101010101101", "101101010010010", "111001010100111",
    "111101101101111", "010110010010111", "110001010100111", "110001010001110", "101101111001001", "111100110001110", "011100111101111",
    "111001010010010", "111101111101111", "111101111001110",
    "000010000010000", "000000111000000", "000000000000010", "001001010100100", "000000000010100" };
static int glyph(char c) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return 26 + c - '0';
    switch (c) { case ':': return 36; case '-': return 37; case '.': return 38; case '/': return 39; case ',': return 40; }
    return -1;
}
static void text(int x, int y, int s, u16 c, const char *t) {
    for (; *t; t++, x += 4 * s) { int g = glyph(*t); if (g < 0) continue;
        for (int r = 0; r < 5; r++) for (int q = 0; q < 3; q++) if (FONT[g][r * 3 + q] == '1') rect(x + q * s, y + r * s, s, s, c); }
}

// ---------- the map ----------
#define MW 32                 // blocks across the map window, 8 pixels each
#define MH 20                 // blocks down (160 pixels; the text lines go below)
static int cur_set, sel_x, sel_y, seed = 1234;
static u16 lot_rgb(const BInfo *b) {
    if (b->terrain == BT_SEA) return C15(4, 10, 22);
    if (b->lot == LOT_RIVER) return C15(8, 16, 28);
    if (b->lot == LOT_WILD || b->lot == LOT_EMPTY) switch (b->terrain) {
        case BT_BEACH: return C15(27, 25, 15); case BT_GRASS: return C15(9, 19, 7); case BT_FOREST: return C15(4, 13, 5);
        case BT_DESERT: return C15(26, 22, 11); default: return C15(14, 14, 14); }
    if (b->lot == LOT_PARK || b->lot == LOT_SQUARE || b->lot == LOT_SKATE) return C15(12, 25, 9);
    if (b->lot == LOT_FARM) return C15(22, 24, 8);
    if (b->lot <= LOT_APART) return C15(27, 17, 8);                 // homes
    if (b->lot == LOT_TOWER || b->lot == LOT_OFFICE || b->lot == LOT_HOTEL) return C15(18, 20, 26);
    return C15(28, 8, 8);                                            // the places people go
}
static void draw_map(void) {
    const Settle *s = bm_set(cur_set); int x0 = s->x - MW / 2, y0 = s->y - MH / 2;
    for (int j = 0; j < MH; j++) for (int i = 0; i < MW; i++) {
        BInfo b; int bx = x0 + i, by = y0 + j;
        if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N) { rect(i * 8, j * 8, 8, 8, 0); continue; }
        bm_block(bx, by, &b);
        rect(i * 8, j * 8, 8, 8, lot_rgb(&b));
        if (b.terrain != BT_SEA) {   // the streets: along the top and left of the block
            if (b.road & 1) rect(i * 8, j * 8, 8, 1, C15(9, 9, 10));
            if (b.road & 2) rect(i * 8, j * 8, 1, 8, C15(9, 9, 10));
        }
        if (bx == sel_x && by == sel_y) { rect(i * 8, j * 8, 8, 1, 31 | 31 << 5 | 31 << 10 | BIT(15)); rect(i * 8, j * 8 + 7, 8, 1, 0xFFFF); rect(i * 8, j * 8, 1, 8, 0xFFFF); rect(i * 8 + 7, j * 8, 1, 8, 0xFFFF); }
    }
    rect(0, 160, 256, 32, C15(3, 3, 5));
    BInfo b; bm_block(sel_x, sel_y, &b);
    char t[48];
    sprintf(t, "%s  %d/%d", s->name, cur_set + 1, bm_nset()); text(4, 164, 2, C15(31, 31, 22), t);
    sprintf(t, "%s  %d,%d", bm_lot_name(b.lot), sel_x, sel_y); text(4, 178, 2, C15(24, 28, 31), t);
}

// ---------- the top screen: one lot in 3D ----------
#define T 330                  // a tile in 4.12 fixed point (1.0 = 4096), so a 13-tile lot is about a unit wide
#define LOTN 13
static int polys;
static inline int tl(int t) { return (t - LOTN) * T; }   // tile coordinate -> centred model coordinate
static void qd(int x0, int y0, int z0, int x1, int y1, int z1, int x2, int y2, int z2, int x3, int y3, int z3, u16 c) {
    glColor(c); glVertex3v16(x0, -y0, z0); glVertex3v16(x1, -y1, z1); glVertex3v16(x2, -y2, z2); glVertex3v16(x3, -y3, z3); polys += 2;
}
static u16 shade(u16 c, int k) {   // k/8 of the colour
    int r = (c & 31) * k / 8, g = ((c >> 5) & 31) * k / 8, b = ((c >> 10) & 31) * k / 8;
    return (u16)(RGB15(r > 31 ? 31 : r, g > 31 ? 31 : g, b > 31 ? 31 : b) | BIT(15));
}
static void ground(int x0, int z0, int x1, int z1, int y, u16 c) { qd(x0, y, z0, x1, y, z0, x1, y, z1, x0, y, z1, c); }
static void block(int x0, int z0, int x1, int z1, int y0, int y1, u16 wall, u16 roof) {   // a box (y up is negative in this view: we flip below)
    qd(x0, y0, z0, x1, y0, z0, x1, y1, z0, x0, y1, z0, wall);
    qd(x1, y0, z0, x1, y0, z1, x1, y1, z1, x1, y1, z0, shade(wall, 6));
    qd(x1, y0, z1, x0, y0, z1, x0, y1, z1, x1, y1, z1, wall);
    qd(x0, y0, z1, x0, y0, z0, x0, y1, z0, x0, y1, z1, shade(wall, 6));
    qd(x0, y1, z0, x1, y1, z0, x1, y1, z1, x0, y1, z1, roof);
}
static void gable(int x0, int z0, int x1, int z1, int y, int ridge, u16 roof) {   // a pitched roof, ridge along x
    int zm = (z0 + z1) / 2;
    qd(x0, y, z0, x1, y, z0, x1, y + ridge, zm, x0, y + ridge, zm, roof);
    qd(x0, y + ridge, zm, x1, y + ridge, zm, x1, y, z1, x0, y, z1, shade(roof, 6));
    glColor(shade(roof, 5)); glVertex3v16(x0, y, z0); glVertex3v16(x0, y, z1); glVertex3v16(x0, y + ridge, zm); glVertex3v16(x0, y + ridge, zm); polys++;
    glColor(shade(roof, 5)); glVertex3v16(x1, y, z0); glVertex3v16(x1, y + ridge, zm); glVertex3v16(x1, y, z1); glVertex3v16(x1, y + ridge, zm); polys++;
}
static void tree(int x, int z, int s) {
    block(x - s / 4, z - s / 4, x + s / 4, z + s / 4, 0, -s / 2, C15(14, 8, 4), C15(14, 8, 4));
    block(x - s / 2, z - s / 2, x + s / 2, z + s / 2, -s / 2, -s, C15(5, 16, 5), C15(7, 20, 6));
}
static void person(int x, int z, u16 c, int yaw) {   // a person: one quad that faces the camera
    int s = T, hx = (s * sinLerp(yaw)) >> 13, hz = (s * cosLerp(yaw)) >> 13;
    qd(x - hx, 0, z - hz, x + hx, 0, z + hz, x + hx, -s * 2, z + hz, x - hx, -s * 2, z - hz, c);
}
static void draw_lot(const BInfo *b, int yaw) {
    u32 h = b->h; int k = b->lot;
    u16 grass = b->terrain == BT_DESERT ? C15(24, 20, 10) : b->terrain == BT_ROCKY ? C15(13, 13, 13) : b->terrain == BT_BEACH ? C15(26, 24, 14) : C15(11, 20, 8);
    if (b->terrain == BT_SEA) { grass = C15(5, 12, 24); }
    for (int z = 0; z < LOTN; z++) for (int x = 0; x < LOTN; x++) ground(tl(x * 2), tl(z * 2), tl(x * 2 + 2), tl(z * 2 + 2), 0, ((x + z) & 1) ? shade(grass, 7) : grass);
    ground(tl(-2), tl(-2), tl(LOTN * 2 + 2), tl(0), 0, C15(8, 8, 9));    // the street along the top
    ground(tl(-2), tl(0), tl(0), tl(LOTN * 2 + 2), 0, C15(8, 8, 9));     // and the left
    if (k == LOT_WILD || k == LOT_EMPTY || k == LOT_RIVER) { for (int i = 0; i < 5; i++) tree(tl(3 + (int)((h >> (i * 4)) & 15) * 2 % 22), tl(3 + (int)((h >> (i * 4 + 2)) & 15) * 2 % 22), T * 2); return; }
    if (k == LOT_PARK || k == LOT_SQUARE || k == LOT_SKATE) { for (int i = 0; i < 9; i++) tree(tl(3 + (int)((h >> (i * 3)) & 7) * 3), tl(3 + (int)((h >> (i * 3 + 1)) & 7) * 3), T * 2); return; }
    int w = 10 + (int)(h & 3) * 2, d = 8 + (int)((h >> 2) & 3) * 2, x0 = 4 + (24 - w) / 2 / 2 * 2 - 4, z0 = 4;
    int st = bm_storeys(b); int hh = (k >= LOT_SHOP ? 2 : 2) * st + (st == 1 ? 2 : 0);
    u16 walls[6] = { C15(26, 22, 16), C15(24, 14, 10), C15(22, 24, 26), C15(26, 26, 18), C15(16, 22, 14), C15(24, 20, 22) };
    u16 wall = walls[(h >> 8) % 6];
    if (k >= LOT_SHOP && k != LOT_PARK) wall = k == LOT_TOWER || k == LOT_OFFICE || k == LOT_HOTEL ? C15(16, 20, 26) : C15(27, 12, 10);
    int X0 = tl(x0), X1 = tl(x0 + w), Z0 = tl(z0), Z1 = tl(z0 + d), Y = -(hh * T);
    block(X0, Z0, X1, Z1, 0, Y, wall, wall);
    if (k <= LOT_APART) gable(X0, Z0, X1, Z1, Y, -T * 2, C15(18, 8, 6));
    else ground(X0, Z0, X1, Z1, Y - 24, C15(14, 14, 15));
    if (k <= LOT_APART) {   // a fence and a pool behind, or a garden
        int bk = (int)((h >> 12) % 6);
        if (bk == 0) ground(tl(x0 + 1), tl(z0 + d + 2), tl(x0 + w - 1), tl(z0 + d + 6), -24, C15(6, 18, 28));
        else if (bk == 1) for (int i = 0; i < 3; i++) tree(tl(x0 + 2 + i * 3), tl(z0 + d + 4), T);
        if ((h >> 15) % 5 < 3) { int fz = tl(z0 + d + 8); for (int i = 0; i < 6; i++) block(tl(x0) + i * (X1 - X0) / 6, fz, tl(x0) + i * (X1 - X0) / 6 + T / 3, fz + T / 4, 0, -T / 2, C15(28, 26, 20), C15(28, 26, 20)); }
    }
    for (int i = 0; i < 6; i++) person(tl(4 + i * 3), tl(LOTN * 2 - 3 + (i & 1) * 2), C15(6 + i * 4, 10, 25 - i * 3), yaw);
}

// ---------- the lot: LIVE, BUY and BUILD (the top screen is the live lot, the bottom screen is the tool: catalogue or plan) ----------
#define GN 13                  // the buildable plot: 13 x 13 cells, each 2 units
enum { M_MAP, M_LIVE, M_BUY, M_BUILD, M_CAS };
typedef struct { const char *name; int price; u8 h; u16 col; u8 kind; } Item;   // kind: 0 box, 1 flat (rug), 2 plant
static const Item ITEMS[5][4] = {
    { {"CHAIR", 80, 2, C15(24, 16, 8), 0}, {"ARMCHAIR", 450, 2, C15(10, 14, 24), 0}, {"SOFA", 900, 2, C15(24, 8, 8), 0}, {"BENCH", 150, 1, C15(20, 14, 8), 0} },
    { {"DINING", 300, 2, C15(22, 15, 8), 0}, {"DESK", 220, 2, C15(18, 18, 20), 0}, {"COFFEE", 120, 1, C15(16, 10, 6), 0}, {"COUNTER", 260, 3, C15(26, 26, 24), 0} },
    { {"TV", 600, 3, C15(3, 3, 4), 0}, {"COMPUTER", 900, 2, C15(20, 20, 22), 0}, {"STEREO", 350, 2, C15(6, 6, 8), 0}, {"FRIDGE", 700, 5, C15(28, 28, 30), 0} },
    { {"LAMP", 60, 4, C15(31, 28, 14), 0}, {"RUG", 100, 0, C15(22, 8, 12), 1}, {"PLANT", 40, 3, C15(5, 18, 6), 2}, {"STATUE", 500, 4, C15(24, 24, 22), 0} },
    { {"BED", 800, 2, C15(20, 24, 31), 0}, {"BATHTUB", 650, 2, C15(28, 30, 31), 0}, {"TOILET", 250, 2, C15(30, 30, 30), 0}, {"SINK", 200, 3, C15(26, 28, 30), 0} } };
static const char *const CATN[5] = { "SEATS", "TABLES", "TECH", "DECO", "ROOMS" };
static const u16 FLOORC[5] = { C15(22, 16, 9), C15(26, 26, 24), C15(10, 16, 24), C15(14, 22, 12), C15(8, 8, 10) };
static const u16 WALLC[4] = { C15(30, 28, 22), C15(28, 20, 16), C15(20, 24, 30), C15(30, 30, 30) };
static void set_mode(int m);
static void sim3d(int i, int yaw);
typedef struct { u8 x, y, c, i, rot; } Obj;
static Obj objs[48]; static int nobj, funds = 20000;
static u8 flr[GN][GN], hwl[GN + 1][GN], vwl[GN][GN + 1];     // floors (0 none, else 1..5), walls on the cell edges (0 none, else 1..4)
static int mode = M_MAP, cat, pick = -1, pcx = 6, pcy = 6, prot, tool, pal, lot_bx = -1, lot_by = -1, cam = 3000, pitch = 3200, msg_t;
static const char *msg = "";
typedef struct { int x, y, tx, ty, need[6], acc[6], act, act_left, act_amt, act_obj, face, rel; u8 skin, hair, hcol, shirt; } Sim;   // x, y in 1/16 cells; needs 0..100: hunger, fun, energy, social, bladder, hygiene
static Sim sims[3]; static int cur_sim, minutes = 11 * 60 + 40;
static const char *const SNAME[3] = { "ASHLEY NEWBIE", "RONALD BACON", "KIT FARLOW" };
static const u16 SKINC[6] = { C15(30, 24, 20), C15(27, 20, 15), C15(22, 15, 10), C15(16, 10, 6), C15(11, 7, 4), C15(29, 22, 17) };
static const u16 HAIRC[6] = { C15(3, 3, 4), C15(14, 8, 4), C15(26, 20, 8), C15(24, 8, 4), C15(24, 24, 24), C15(10, 14, 28) };
static const u16 SHIRTC[6] = { C15(26, 26, 26), C15(24, 8, 8), C15(8, 14, 26), C15(8, 22, 10), C15(28, 22, 8), C15(16, 8, 22) };
static const char *const HAIRN[6] = { "FLAT TOP", "ROUND", "SPIKES", "LONG", "BRAIDS", "BALD" };
static void say(const char *m) { msg = m; msg_t = 90; }
static int obj_at(int x, int y) { for (int i = 0; i < nobj; i++) if (objs[i].x == x && objs[i].y == y) return i; return -1; }
static void lot_reset(void) {
    memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; funds = 3500; pick = -1; mode = M_LIVE; cat = 0; tool = 0; pal = 1;
    for (int y = 4; y < 10; y++) for (int x = 3; x < 10; x++) flr[y][x] = 1;                       // a little starter room
    for (int x = 3; x < 10; x++) { hwl[4][x] = 1; hwl[10][x] = 1; } for (int y = 4; y < 10; y++) { vwl[y][3] = 1; vwl[y][10] = 1; } hwl[10][6] = 0;   // (a gap in the front wall: the door)
    static const u8 st[][5] = { {4, 5, 0, 2, 0}, {6, 5, 2, 0, 2}, {8, 5, 4, 0, 0}, {9, 9, 3, 0, 0}, {4, 8, 4, 1, 0}, {8, 8, 0, 2, 0} };   // x, y, cat, item, rot: sofa-ish set
    static const u8 st2[][5] = { {4, 5, 4, 0, 0}, {6, 5, 2, 0, 2}, {8, 5, 2, 3, 0}, {4, 8, 0, 2, 1}, {9, 8, 4, 2, 0}, {9, 6, 4, 1, 0} };
    (void)st; for (int i = 0; i < 6; i++) { objs[nobj].x = st2[i][0]; objs[nobj].y = st2[i][1]; objs[nobj].c = st2[i][2]; objs[nobj].i = st2[i][3]; objs[nobj].rot = st2[i][4]; nobj++; }
    for (int i = 0; i < 3; i++) { Sim *m = &sims[i]; m->x = (6 + i) * 16; m->y = 11 * 16; m->tx = m->x; m->ty = m->y; m->act = -1; m->act_left = 0;
        for (int n = 0; n < 6; n++) { m->need[n] = 90 - (n * 17 + i * 11) % 60; m->acc[n] = 0; } m->rel = 0; if (i) { m->skin = (u8)(i * 2); m->hair = (u8)(i); m->hcol = (u8)(i * 2); m->shirt = (u8)(i + 2); } }
}
static void bar(int x, int y, int w, int v, u16 c) { rect(x, y, w, 6, C15(4, 4, 6)); rect(x + 1, y + 1, (w - 2) * v / 100, 4, c); }
static void button(int x, int y, int w, int h, const char *t, u16 c, int on) {
    rect(x, y, w, h, on ? C15(31, 31, 20) : C15(2, 2, 4)); rect(x + 1, y + 1, w - 2, h - 2, on ? c : shade(c, 4));
    text(x + 4, y + h / 2 - 2, 1, on ? C15(2, 2, 4) : C15(31, 31, 31), t);
}
static int hit(int x, int y, int w, int h, int tx, int ty) { return tx >= x && tx < x + w && ty >= y && ty < y + h; }
#define PX 2                  // the plan view: 13 cells of 12 pixels
#define PY 22
static void plan(int ghost) {
    for (int y = 0; y < GN; y++) for (int x = 0; x < GN; x++) {
        rect(PX + x * 12, PY + y * 12, 12, 12, flr[y][x] ? FLOORC[flr[y][x] - 1] : (((x + y) & 1) ? C15(9, 18, 7) : C15(11, 20, 8)));
        rect(PX + x * 12, PY + y * 12, 12, 1, C15(0, 0, 0) | 0); }
    for (int y = 0; y <= GN; y++) for (int x = 0; x < GN; x++) if (hwl[y][x]) rect(PX + x * 12, PY + y * 12 - 1, 13, 3, WALLC[hwl[y][x] - 1]);
    for (int y = 0; y < GN; y++) for (int x = 0; x <= GN; x++) if (vwl[y][x]) rect(PX + x * 12 - 1, PY + y * 12, 3, 13, WALLC[vwl[y][x] - 1]);
    for (int i = 0; i < nobj; i++) { const Item *it = &ITEMS[objs[i].c][objs[i].i]; rect(PX + objs[i].x * 12 + 2, PY + objs[i].y * 12 + 2, 8, 8, it->col); }
    if (ghost) { rect(PX + pcx * 12, PY + pcy * 12, 12, 1, 0xFFFF); rect(PX + pcx * 12, PY + pcy * 12 + 11, 12, 1, 0xFFFF); rect(PX + pcx * 12, PY + pcy * 12, 1, 12, 0xFFFF); rect(PX + pcx * 12 + 11, PY + pcy * 12, 1, 12, 0xFFFF); }
}
static void draw_ui(void) {
    rect(0, 0, 256, 192, C15(5, 6, 9));
    const char *tn[4] = { "MAP", "LIVE", "BUY", "BUILD" };
    for (int i = 0; i < 4; i++) button(i * 64, 0, 62, 16, tn[i], C15(8, 14, 24), mode == i);
    char t[40];
    sprintf(t, "FUNDS %d", funds); text(4, 182, 1, C15(31, 31, 20), t);
    sprintf(t, "%d:%02d", (minutes / 60) % 24, minutes % 60); text(224, 182, 1, C15(24, 28, 31), t);
    if (msg_t > 0) text(70, 182, 1, C15(31, 20, 12), msg);
    if (mode == M_LIVE) {
    } else if (mode == M_BUY && pick < 0) {
        for (int i = 0; i < 5; i++) button(2 + i * 51, 20, 49, 14, CATN[i], C15(24, 14, 8), cat == i);
        for (int i = 0; i < 4; i++) { const Item *it = &ITEMS[cat][i]; int x = 4 + (i & 1) * 125, y = 42 + (i >> 1) * 64;
            rect(x, y, 120, 58, C15(2, 2, 4)); rect(x + 2, y + 2, 116, 54, C15(10, 12, 18)); rect(x + 8, y + 8, 34, 34, it->col); rect(x + 12, y + 12, 26, 26, shade(it->col, 6));
            text(x + 48, y + 12, 1, C15(31, 31, 31), it->name); sprintf(t, "%d", it->price); text(x + 48, y + 28, 1, funds >= it->price ? C15(31, 31, 18) : C15(31, 10, 10), t); }
    } else if (mode == M_BUY) {
        plan(1); const Item *it = &ITEMS[cat][pick];
        text(166, 26, 1, C15(31, 31, 31), it->name); sprintf(t, "%d", it->price); text(166, 38, 1, C15(31, 31, 18), t);
        button(164, 60, 88, 22, "PLACE (A)", C15(10, 22, 10), 1); button(164, 90, 88, 22, "TURN (R)", C15(10, 14, 24), 0); button(164, 120, 88, 22, "BACK (B)", C15(24, 10, 10), 0);
        text(164, 150, 1, C15(20, 24, 28), "TAP THE PLAN"); text(164, 160, 1, C15(20, 24, 28), "OR USE D-PAD");
    } else if (mode == M_BUILD) {
        plan(1);
        const char *tl2[4] = { "FLOOR", "WALL", "ERASE", "SELL" };
        for (int i = 0; i < 4; i++) button(164, 22 + i * 20, 88, 17, tl2[i], C15(24, 14, 8), tool == i);
        int n = tool == 0 ? 5 : tool == 1 ? 4 : 0;
        for (int i = 0; i < n; i++) { rect(164 + i * 17, 108, 15, 15, pal == i + 1 ? 0xFFFF : C15(2, 2, 4)); rect(166 + i * 17, 110, 11, 11, tool == 0 ? FLOORC[i] : WALLC[i]); }
        text(164, 130, 1, C15(20, 24, 28), tool == 1 ? "TAP NEAR A CELL EDGE" : tool == 3 ? "TAP FURNITURE" : "TAP CELLS");
    }
}
static int lot_touch(int tx, int ty) {   // a stylus tap on the plan: the cell (or -1)
    if (tx < PX || ty < PY || tx >= PX + GN * 12 || ty >= PY + GN * 12) return -1;
    return ((ty - PY) / 12) * GN + (tx - PX) / 12;
}
static void ui_touch(int tx, int ty) {
    for (int i = 0; i < 4; i++) if (hit(i * 64, 0, 62, 16, tx, ty)) {
        set_mode(i); pick = -1;
        return; }
    if (mode == M_BUY && pick < 0) {
        for (int i = 0; i < 5; i++) if (hit(2 + i * 51, 20, 49, 14, tx, ty)) cat = i;
        for (int i = 0; i < 4; i++) if (hit(4 + (i & 1) * 125, 42 + (i >> 1) * 64, 120, 58, tx, ty)) { if (funds >= ITEMS[cat][i].price) { pick = i; prot = 0; } else say("NOT ENOUGH FUNDS"); }
    } else if (mode == M_BUY) {
        int c = lot_touch(tx, ty);
        if (c >= 0) { int nx = c % GN, ny = c / GN; if (nx == pcx && ny == pcy) goto place; pcx = nx; pcy = ny; return; }
        if (hit(164, 60, 88, 22, tx, ty)) goto place;
        if (hit(164, 90, 88, 22, tx, ty)) prot = (prot + 1) & 3;
        if (hit(164, 120, 88, 22, tx, ty)) pick = -1;
    } else if (mode == M_BUILD) {
        for (int i = 0; i < 4; i++) if (hit(164, 22 + i * 20, 88, 17, tx, ty)) { tool = i; if (pal < 1) pal = 1; if (tool == 1 && pal > 4) pal = 4; }
        int n = tool == 0 ? 5 : tool == 1 ? 4 : 0;
        for (int i = 0; i < n; i++) if (hit(164 + i * 17, 108, 15, 15, tx, ty)) pal = i + 1;
        int c = lot_touch(tx, ty);
        if (c >= 0) { int x = c % GN, y = c / GN; pcx = x; pcy = y;
            if (tool == 0) flr[y][x] = (u8)pal;
            else if (tool == 2) { flr[y][x] = 0; }
            else if (tool == 3) { int o = obj_at(x, y); if (o >= 0) { funds += ITEMS[objs[o].c][objs[o].i].price / 2; objs[o] = objs[--nobj]; say("SOLD"); } }
            else if (tool == 1 || tool == 2) {}
            if (tool == 1) { int fx = (tx - PX) % 12, fy = (ty - PY) % 12, d[4] = { fy, 11 - fy, fx, 11 - fx }, b = 0; for (int k = 1; k < 4; k++) if (d[k] < d[b]) b = k;
                u8 *e = b == 0 ? &hwl[y][x] : b == 1 ? &hwl[y + 1][x] : b == 2 ? &vwl[y][x] : &vwl[y][x + 1]; *e = *e ? 0 : (u8)pal; }
            if (tool == 2) { hwl[y][x] = 0; hwl[y + 1][x] = 0; vwl[y][x] = 0; vwl[y][x + 1] = 0; } }
    }
    return;
place:
    if (obj_at(pcx, pcy) >= 0) { say("SOMETHING IS THERE"); return; }
    if (nobj >= 48) { say("LOT IS FULL"); return; }
    if (funds < ITEMS[cat][pick].price) { say("NOT ENOUGH FUNDS"); pick = -1; return; }
    funds -= ITEMS[cat][pick].price; objs[nobj].x = (u8)pcx; objs[nobj].y = (u8)pcy; objs[nobj].c = (u8)cat; objs[nobj].i = (u8)pick; objs[nobj].rot = (u8)prot; nobj++; say("BOUGHT"); 
}
static void obj3d(int x, int y, const Item *it, int rot, int ghost) {
    int x0 = tl(x * 2) + T / 3, x1 = tl(x * 2 + 2) - T / 3, z0 = tl(y * 2) + T / 3, z1 = tl(y * 2 + 2) - T / 3, hh = it->h * T / 2 + 20; (void)ghost;
    if (it->kind == 1) { ground(x0 - T / 4, z0 - T / 4, x1 + T / 4, z1 + T / 4, -50, it->col); return; }
    if (it->kind == 2) { tree((x0 + x1) / 2, (z0 + z1) / 2, T * 3 / 2); return; }
    block(x0, z0, x1, z1, -40, -hh, it->col, shade(it->col, 10));
    int fx = rot == 1 ? x1 : rot == 3 ? x0 : (x0 + x1) / 2, fz = rot == 0 ? z1 : rot == 2 ? z0 : (z0 + z1) / 2;   // a dark dot on the front
    ground(fx - T / 8, fz - T / 8, fx + T / 8, fz + T / 8, -hh - 8, C15(3, 3, 4));
}
static void draw_build(int yaw) {
    for (int z = 0; z < GN; z++) for (int x = 0; x < GN; x++) ground(tl(x * 2), tl(z * 2), tl(x * 2 + 2), tl(z * 2 + 2), 0, ((x + z) & 1) ? C15(9, 18, 7) : C15(11, 20, 8));
    ground(tl(-2), tl(-2), tl(GN * 2 + 2), tl(0), 0, C15(8, 8, 9)); ground(tl(-2), tl(GN * 2), tl(GN * 2 + 2), tl(GN * 2 + 2), 0, C15(8, 8, 9));
    for (int y = 0; y < GN; y++) for (int x = 0; x < GN; x++) if (flr[y][x]) ground(tl(x * 2), tl(y * 2), tl(x * 2 + 2), tl(y * 2 + 2), -40, FLOORC[flr[y][x] - 1]);
    int wh = 2 * T;   // low walls, so you can see in
    for (int y = 0; y <= GN; y++) for (int x = 0; x < GN; x++) if (hwl[y][x]) block(tl(x * 2), tl(y * 2) - T / 8, tl(x * 2 + 2), tl(y * 2) + T / 8, -40, -wh, WALLC[hwl[y][x] - 1], WALLC[hwl[y][x] - 1]);
    for (int y = 0; y < GN; y++) for (int x = 0; x <= GN; x++) if (vwl[y][x]) block(tl(x * 2) - T / 8, tl(y * 2), tl(x * 2) + T / 8, tl(y * 2 + 2), -40, -wh, WALLC[vwl[y][x] - 1], WALLC[vwl[y][x] - 1]);
    for (int i = 0; i < nobj; i++) obj3d(objs[i].x, objs[i].y, &ITEMS[objs[i].c][objs[i].i], objs[i].rot, 0);
    for (int i = 0; i < 3; i++) sim3d(i, yaw);
    if (mode == M_BUY || mode == M_BUILD) {   // the cursor
        ground(tl(pcx * 2), tl(pcy * 2), tl(pcx * 2 + 2), tl(pcy * 2 + 2), -80, C15(31, 31, 31));
        if (mode == M_BUY && pick >= 0) obj3d(pcx, pcy, &ITEMS[cat][pick], prot, 1);
    }
}

// ============ the Sims-style game: LIVE (status on the top screen, the lot and its icons on the bottom, touch to play) ============
typedef struct { const char *lab; u8 need; u8 amt; u8 dur; } Act;
static const Act ACT[5][4] = {
    { {"SIT", 2, 25, 90}, {"RELAX", 2, 35, 120}, {"NAP", 2, 60, 150}, {"SIT", 2, 20, 90} },
    { {"SIT", 1, 10, 60}, {"LOOK", 1, 5, 40}, {"LOOK", 1, 5, 40}, {"COOK", 0, 35, 120} },
    { {"WATCH TV", 1, 60, 150}, {"PLAY", 1, 50, 150}, {"LISTEN", 1, 35, 120}, {"EAT", 0, 70, 120} },
    { {"LOOK", 1, 3, 30}, {"LOOK", 1, 3, 30}, {"WATER", 1, 10, 60}, {"ADMIRE", 1, 15, 60} },
    { {"SLEEP", 2, 90, 240}, {"BATHE", 5, 90, 180}, {"USE", 4, 95, 100}, {"WASH", 5, 30, 80} } };
static const char *const NEEDN[6] = { "HUNGER", "FUN", "ENERGY", "SOCIAL", "BLADDER", "HYGIENE" };
static int menu_open, popup, popup_i, speed = 1, yaw_t = 4096, live_yaw = 4096, live_dist = 8500, camx, camz, cas_tab, cas_done, cas_stage;
static const u16 BG_L = C15(18, 25, 31), PAN = C15(8, 16, 28), PANB = C15(2, 7, 20), PANH = C15(5, 12, 26), TXT = C15(31, 31, 31);

static void panel(int x, int y, int w, int h, const char *title) {
    rect(x + 1, y, w - 2, h, PANB); rect(x, y + 1, w, h - 2, PANB); rect(x + 2, y + 2, w - 4, h - 4, PAN);
    if (title) { rect(x + 2, y + 2, w - 4, 16, PANH); text(x + 10, y + 5, 2, TXT, title); }
}
static void backdrop(void) {
    rect(0, 0, 256, 192, BG_L);
    for (int y = 0; y < 192; y += 16) for (int x = (y / 16 & 1) * 16; x < 256; x += 32) rect(x + 6, y + 6, 4, 4, C15(22, 28, 31));
}
static void face(int x, int y, int sz, const Sim *m) {   // a head and shoulders portrait, sz pixels square
    rect(x, y, sz, sz, C15(22, 22, 24)); int u = sz / 8;
    rect(x + u, y + sz - 2 * u, sz - 2 * u, 2 * u, SHIRTC[m->shirt]);
    rect(x + 2 * u, y + 2 * u, 4 * u, 5 * u, SKINC[m->skin]);
    u16 hc = HAIRC[m->hcol];
    switch (m->hair) {
        case 0: rect(x + 2 * u, y + u, 4 * u, u + u / 2, hc); rect(x + u + u / 2, y + 2 * u, u, 2 * u, hc); break;
        case 1: rect(x + u + u / 2, y + u, 5 * u, 2 * u, hc); break;
        case 2: for (int k = 0; k < 4; k++) rect(x + 2 * u + k * u, y + u / 2, u / 2 + 1, 2 * u, hc); rect(x + 2 * u, y + 2 * u, 4 * u, u / 2, hc); break;
        case 3: rect(x + u + u / 2, y + u, 5 * u, 2 * u, hc); rect(x + u + u / 2, y + 3 * u, u, 4 * u, hc); rect(x + 6 * u + u / 2, y + 3 * u, u, 4 * u, hc); break;
        case 4: rect(x + 2 * u, y + u + u / 2, 4 * u, u, hc); for (int k = 0; k < 4; k++) rect(x + 2 * u + k * u, y + 2 * u, u / 2 + 1, 3 * u, hc); break;
        default: break; }
    rect(x + 3 * u, y + 4 * u, u / 2 + 1, u / 2 + 1, C15(2, 2, 3)); rect(x + 5 * u, y + 4 * u, u / 2 + 1, u / 2 + 1, C15(2, 2, 3)); rect(x + 3 * u + u / 2, y + 6 * u, 2 * u, u / 2 + 1, C15(20, 6, 6));
}
static int mood(const Sim *m) { int s = 0; for (int n = 0; n < 6; n++) s += m->need[n]; return s / 6; }
static u16 need_col(int v) { return v > 55 ? C15(8, 26, 8) : v > 28 ? C15(28, 24, 6) : C15(30, 8, 6); }
static const char *DAYN[7] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
static const char *relname(int r) { return r < 3 ? "ACQUAINTANCE" : r < 6 ? "FRIEND" : "BEST FRIEND"; }
static void draw_top_status(void) {   // the top screen while you live: the sim's mood, the time and money, and the needs (or the bio of another sim)
    scr = scr_sub; backdrop(); const Sim *m = &sims[cur_sim];
    char t[48];
    if (cur_sim == 0) {
        face(8, 8, 56, m);
        panel(70, 6, 180, 60, SNAME[0]);
        text(78, 28, 1, TXT, "MOOD"); rect(112, 26, 70, 9, C15(24, 28, 31)); rect(113, 27, 68 * mood(m) / 100, 7, need_col(mood(m)));
        int mn = minutes % 1440, h12 = (mn / 60) % 12 ? (mn / 60) % 12 : 12;
        sprintf(t, "%s %d:%02d %s", DAYN[(minutes / 1440) % 7], h12, mn % 60, mn / 60 >= 12 ? "PM" : "AM"); text(78, 42, 1, TXT, t);
        sprintf(t, "FUNDS %d", funds); text(78, 54, 1, C15(31, 31, 20), t);
        panel(6, 72, 244, 114, "NEEDS");
        static const int order[6] = { 0, 1, 2, 3, 4, 5 };
        for (int k = 0; k < 6; k++) { int n = order[k], cx = 14 + (k & 1) * 120, cy = 94 + (k >> 1) * 30;
            rect(cx, cy, 14, 14, C15(18, 24, 31)); text(cx + 4, cy + 4, 1, PANB, "-"); rect(cx + 3, cy + 3, 8, 8, need_col(m->need[n]));
            text(cx + 18, cy, 1, TXT, NEEDN[n]); rect(cx + 18, cy + 7, 84, 8, C15(2, 6, 14)); rect(cx + 19, cy + 8, 82 * m->need[n] / 100, 6, need_col(m->need[n])); }
    } else {
        char tt[40]; sprintf(tt, "SIM BIO - %s", SNAME[cur_sim]); panel(6, 6, 244, 180, tt);
        face(16, 30, 64, m);
        for (int k = 0; k < 4; k++) { rect(94 + k * 38, 30, 32, 32, C15(18, 24, 31)); text(106 + k * 38, 40, 2, PANB, "?"); }
        panel(94, 70, 148, 60, 0);
        panel(94, 136, 148, 40, 0); text(102, 150, 1, TXT, "-"); text(110, 150, 1, TXT, relname(m->rel));
        sprintf(t, "%d", mood(m)); text(16, 100, 1, TXT, "MOOD"); rect(16, 110, 64, 8, C15(24, 28, 31)); rect(17, 111, 62 * mood(m) / 100, 6, need_col(mood(m)));
    }
}
static void hud_clear(void) { memset(hud, 0, 256 * 192 * 2); }
static void draw_hud(void) {
    scr = hud; hud_clear();
    rect(2, 2, 26, 26, PANB); rect(3, 3, 24, 24, C15(14, 22, 28)); rect(8, 12, 14, 10, C15(26, 14, 10)); rect(6, 8, 18, 5, C15(24, 6, 6)); rect(13, 16, 4, 6, C15(8, 8, 10));   // MAP: a little house
    for (int i = 0; i < 3; i++) { int x = 168 + i * 29; rect(x, 2, 27, 27, PANB); rect(x + 1, 3, 25, 25, C15(14, 22, 28)); rect(x + 3, 22, 21, 4, C15(2, 6, 14)); rect(x + 4, 23, 4 + i * 5, 2, C15(8, 26, 8)); }   // wants
    rect(0, 158, 34, 34, PANB); face(2, 160, 30, &sims[0]);
    rect(36, 168, 24, 22, PANB); rect(37, 169, 22, 20, PAN); text(42, 176, 1, TXT, speed == 1 ? ">" : speed == 2 ? ">>" : ">>>");
    rect(104, 178, 48, 14, PANB); rect(105, 179, 46, 13, PAN); for (int i = 0; i < 3; i++) rect(118 + i * 8, 184, 4, 4, TXT);
    if (menu_open) { const char *mn[3] = { "BUY", "BUILD", "MAP" }; for (int i = 0; i < 3; i++) { rect(66 + i * 46, 150, 44, 24, PANB); rect(67 + i * 46, 151, 42, 22, PANH); text(72 + i * 46, 158, 1, TXT, mn[i]); } }
    if (popup) {
        const char *nm, *lab; if (popup == 1) { const Obj *o = &objs[popup_i]; nm = ITEMS[o->c][o->i].name; lab = ACT[o->c][o->i].lab; } else { nm = SNAME[popup_i]; lab = "CHAT"; }
        rect(56, 100, 144, 56, PANB); rect(57, 101, 142, 54, PAN); text(64, 106, 2, TXT, nm);
        rect(64, 126, 62, 24, C15(8, 20, 8)); text(70, 135, 1, TXT, lab); rect(132, 126, 62, 24, C15(24, 8, 8)); text(138, 135, 1, TXT, "CANCEL");
    }
    if (msg_t > 0) { rect(70, 40, 116, 14, PANB); text(76, 45, 1, TXT, msg); }
}
// ---- picking: a stylus tap on the 3D lot -> the cell under it (the view is a camera we know, so we undo it) ----
static int pick_cell(int sx, int sy, int *cx, int *cy) {
    float c = cosLerp(live_yaw) / 4096.0f, s = sinLerp(live_yaw) / 4096.0f, cp = cosLerp(pitch) / 4096.0f, sp = sinLerp(pitch) / 4096.0f;
    float nx = (sx - 128) / 128.0f, ny = (96 - sy) / 96.0f, dx = nx * 0.4663f * 1.3333f, dy = ny * 0.4663f, dz = -1.0f;
    float ox = 0, oy = 0, oz = (float)live_dist;
    float ay = dy * cp + dz * sp, az = -dy * sp + dz * cp, by = oy * cp + oz * sp, bz = -oy * sp + oz * cp;
    float ddx = dx * c - az * s, ddz = dx * s + az * c, oox = ox * c - bz * s + camx, ooz = ox * s + bz * c + camz;
    if (ay > -0.0001f && ay < 0.0001f) return 0;
    float t = -by / ay; float hx = oox + t * ddx, hz = ooz + t * ddz;
    *cx = (int)((hx / T + GN) / 2.0f + 100) - 100; *cy = (int)((hz / T + GN) / 2.0f + 100) - 100;
    return 1;
}
static void start_act(Sim *m, int obj, int sim) {   // walk to it, then do it
    if (sim >= 0) { m->tx = sims[sim].x; m->ty = sims[sim].y; m->act = 3 + 100; m->act_obj = sim; m->act_amt = 50; m->face = 120; }
    else { const Obj *o = &objs[obj]; const Act *a = &ACT[o->c][o->i]; m->tx = o->x * 16; m->ty = o->y * 16; m->act = a->need; m->act_amt = a->amt; m->face = a->dur; m->act_obj = obj; }
}
static void sims_step(int frames) {
    for (int i = 0; i < 3; i++) { Sim *m = &sims[i];
        if (m->act_left > 0) {
            for (int n = 0; n < 6; n++) if (n == (m->act >= 100 ? m->act - 100 : m->act)) { m->acc[n] += m->act_amt; while (m->acc[n] >= m->face) { m->acc[n] -= m->face; if (m->need[n] < 100) m->need[n]++; } }
            m->act_left--; if (m->act_left == 0) { m->act = -1; if (i == 0) say("DONE"); }
        } else if (m->x != m->tx || m->y != m->ty) {
            if (m->x < m->tx) m->x++; else if (m->x > m->tx) m->x--; else if (m->y < m->ty) m->y++; else m->y--;
            if (m->x == m->tx && m->y == m->ty && m->act >= 0) {
                m->act_left = m->face;
                if (m->act >= 100) { Sim *o = &sims[m->act_obj]; o->act_left = m->face; o->act = 3 + 100; o->act_amt = 0; m->act = 3; if (m->rel + 0 >= 0) o->rel++; }
            }
        } else if (i > 0 && ((frames + i * 37) & 127) == 0) { m->tx = (1 + (frames * 7 + i * 5) % 11) * 16; m->ty = (1 + (frames * 3 + i * 9) % 12) * 16; m->act = -1; }
        if (((frames + i * 9) % 70) == 0) for (int n = 0; n < 6; n++) if (m->need[n] > 4 && n != 3) m->need[n]--;
        if (((frames + i * 9) % 140) == 0 && m->need[3] > 4) m->need[3]--;
    }
    if ((frames % 8) == 0) minutes++;
}
static void sim3d(int i, int yaw) {
    const Sim *m = &sims[i];
    int x = m->x * 2 * T / 16 - GN * T + T, z = m->y * 2 * T / 16 - GN * T + T;
    int hx = (T / 3 * cosLerp(yaw)) >> 12, hz = (T / 3 * sinLerp(yaw)) >> 12, gx = hx * 3 / 4, gz = hz * 3 / 4;
    int y0 = -40, y1 = y0 - T * 7 / 10, y2 = y1 - T * 6 / 10, y3 = y2 - T * 6 / 10;
    qd(x - hx, y0, z - hz, x + hx, y0, z + hz, x + hx, y1, z + hz, x - hx, y1, z - hz, C15(4, 4, 8));                 // legs
    qd(x - hx, y1, z - hz, x + hx, y1, z + hz, x + hx, y2, z + hz, x - hx, y2, z - hz, SHIRTC[m->shirt]);             // shirt
    qd(x - gx, y2, z - gz, x + gx, y2, z + gz, x + gx, y3, z + gz, x - gx, y3, z - gz, SKINC[m->skin]);               // head
    if (m->hair != 5) qd(x - gx, y3 - T / 5, z - gz, x + gx, y3 - T / 5, z + gz, x + gx, y3, z + gz, x - gx, y3, z - gz, HAIRC[m->hcol]);   // hair
    if (i == cur_sim) { int py = y3 - T * 7 / 10, d = T / 5; qd(x, py - d, z, x + d * 2 / 3 * hx / (T / 3 + 1), py, z + d * 2 / 3 * hz / (T / 3 + 1), x, py + d, z, x - d * 2 / 3 * hx / (T / 3 + 1), py, z - d * 2 / 3 * hz / (T / 3 + 1), C15(6, 28, 6)); }   // the plumbob
}
// ---- Create-a-Sim: the head on the top screen, the choices on the bottom ----
static void draw_head(int yaw) {
    const Sim *m = &sims[0]; u16 sk = SKINC[m->skin], hc = HAIRC[m->hcol], sh = SHIRTC[m->shirt];
    block(-1100, -520, 1100, 520, 300, 1900, sh, shade(sh, 6));     // shoulders
    block(-300, -300, 300, 300, 0, 320, shade(sk, 7), sk);         // neck
    block(-700, -700, 700, 700, 0, -1500, sk, shade(sk, 7));       // head
    qd(-420, -880, 702, -140, -880, 702, -140, -1040, 702, -420, -1040, 702, C15(30, 30, 30)); qd(140, -880, 702, 420, -880, 702, 420, -1040, 702, 140, -1040, 702, C15(30, 30, 30));   // eyes
    qd(-330, -890, 704, -210, -890, 704, -210, -1030, 704, -330, -1030, 704, C15(8, 4, 3)); qd(210, -890, 704, 330, -890, 704, 330, -1030, 704, 210, -1030, 704, C15(8, 4, 3));
    qd(-50, -600, 702, 50, -600, 702, 80, -820, 702, -80, -820, 702, shade(sk, 6));   // nose
    qd(-260, -330, 702, 260, -330, 702, 260, -420, 702, -260, -420, 702, C15(24, 6, 8));   // mouth
    switch (m->hair) {
        case 0: block(-760, -760, 760, 600, -1460, -1950, hc, hc); break;
        case 1: block(-780, -780, 780, 700, -1400, -1850, hc, hc); block(-780, -780, 780, -300, -300, -1850, hc, hc); break;
        case 2: for (int k = 0; k < 4; k++) block(-640 + k * 400, -300, -400 + k * 400, 300, -1460, -2250 + (k & 1) * 150, hc, hc); break;
        case 3: block(-780, -780, 780, 700, -1400, -1850, hc, hc); block(-800, -800, 800, -380, 900, -1850, hc, hc); block(-820, -380, -700, 400, 700, -1500, hc, hc); block(700, -380, 820, 400, 700, -1500, hc, hc); break;
        case 4: block(-740, -740, 740, 400, -1460, -1700, hc, hc); for (int k = 0; k < 7; k++) block(-760 + k * 230, 600, -680 + k * 230, 700, -1600, -2150, hc, hc); block(-800, -200, -700, 300, -200, 500, hc, hc); block(700, -200, 800, 300, -200, 500, hc, hc); break;
        default: break; }
}
static void draw_cas(void) {   // the bottom screen: icon tabs down the left, a 3 x 2 grid of choices, PLAY
    scr = scr_sub; backdrop();
    const char *tn[4] = { "HAIR", "COLOR", "SKIN", "SHIRT" };
    panel(0, 4, 56, 184, 0);
    for (int i = 0; i < 4; i++) { rect(6, 12 + i * 40, 44, 34, cas_tab == i ? C15(8, 26, 8) : C15(4, 10, 24)); rect(8, 14 + i * 40, 40, 30, cas_tab == i ? C15(10, 30, 10) : C15(6, 14, 28)); text(12, 26 + i * 40, 1, TXT, tn[i]); }
    static const char *ttl[4] = { "HAIR STYLES", "HAIR COLOR", "SKIN TONE", "SHIRT" };
    panel(62, 4, 190, 142, ttl[cas_tab]);
    Sim tmp = sims[0];
    for (int k = 0; k < 6; k++) { int x = 68 + (k % 3) * 60, y = 26 + (k / 3) * 56; int on = (cas_tab == 0 ? tmp.hair : cas_tab == 1 ? tmp.hcol : cas_tab == 2 ? tmp.skin : tmp.shirt) == k;
        rect(x, y, 54, 50, on ? C15(10, 28, 8) : C15(28, 31, 31)); rect(x + 2, y + 2, 50, 46, C15(20, 26, 31));
        Sim v = tmp; if (cas_tab == 0) v.hair = (u8)k; else if (cas_tab == 1) v.hcol = (u8)k; else if (cas_tab == 2) v.skin = (u8)k; else v.shirt = (u8)k;
        face(x + 9, y + 3, 36, &v); if (cas_tab == 0) text(x + 6, y + 42, 1, PANB, HAIRN[k]); }
    rect(62, 152, 190, 34, PANB); rect(64, 154, 186, 30, C15(8, 22, 8)); text(120, 164, 2, TXT, "PLAY");
}
static void cas_touch(int tx, int ty) {
    for (int i = 0; i < 4; i++) if (hit(6, 12 + i * 40, 44, 34, tx, ty)) cas_tab = i;
    for (int k = 0; k < 6; k++) if (hit(68 + (k % 3) * 60, 26 + (k / 3) * 56, 54, 50, tx, ty)) { Sim *m = &sims[0]; if (cas_tab == 0) m->hair = (u8)k; else if (cas_tab == 1) m->hcol = (u8)k; else if (cas_tab == 2) m->skin = (u8)k; else m->shirt = (u8)k; }
    if (hit(62, 152, 190, 34, tx, ty)) cas_done = 1;
}
static void set_mode(int m) {
    mode = m;
    if (m == M_LIVE) { lcdMainOnBottom(); } else { lcdMainOnTop(); scr = hud; hud_clear(); }
}
static void live_touch(int tx, int ty) {
    if (popup) {
        if (hit(64, 126, 62, 24, tx, ty)) { if (popup == 1) start_act(&sims[0], popup_i, -1); else start_act(&sims[0], 0, popup_i); popup = 0; }
        else if (hit(132, 126, 62, 24, tx, ty)) popup = 0;
        return; }
    if (menu_open) { menu_open = 0;
        if (hit(66, 150, 44, 24, tx, ty)) { set_mode(M_BUY); pick = -1; } else if (hit(112, 150, 44, 24, tx, ty)) { set_mode(M_BUILD); } else if (hit(158, 150, 44, 24, tx, ty)) set_mode(M_MAP);
        return; }
    if (hit(2, 2, 26, 26, tx, ty)) { set_mode(M_MAP); return; }
    if (hit(104, 178, 48, 14, tx, ty)) { menu_open = 1; return; }
    if (hit(36, 168, 24, 22, tx, ty)) { speed = speed == 1 ? 2 : speed == 2 ? 3 : 1; return; }
    if (hit(0, 158, 34, 34, tx, ty)) { cur_sim = 0; return; }
    int cx, cy; if (!pick_cell(tx, ty, &cx, &cy)) return;
    for (int i = 0; i < 3; i++) { int sx = (sims[i].x + 8) / 16, sy = (sims[i].y + 8) / 16; if (sx == cx && sy == cy) { cur_sim = i; if (i) { popup = 2; popup_i = i; } return; } }
    int o = obj_at(cx, cy); if (o >= 0) { popup = 1; popup_i = o; return; }
    if (cx >= 0 && cy >= 0 && cx < GN && cy < GN) { cur_sim = 0; sims[0].tx = cx * 16; sims[0].ty = cy * 16; sims[0].act = -1; sims[0].act_left = 0; }
}

int main(void) {
    powerOn(POWER_ALL_2D | POWER_3D_CORE | POWER_MATRIX); irqInit(); irqEnable(IRQ_VBLANK);
    videoSetMode(MODE_5_3D | DISPLAY_BG3_ACTIVE);
    videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    vramSetBankA(VRAM_A_TEXTURE); vramSetBankB(VRAM_B_TEXTURE); vramSetBankC(VRAM_C_SUB_BG); vramSetBankD(VRAM_D_MAIN_BG);
    int bg = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0); scr_sub = scr = bgGetGfxPtr(bg);
    int bgm = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0); hud = bgGetGfxPtr(bgm); bgSetPriority(bgm, 0);
    glInit(); glEnable(GL_ANTIALIAS); glClearColor(14, 20, 31, 31); glClearPolyID(63); glClearDepth(0x7FFF); glViewport(0, 0, 255, 191); REG_BG0CNT = BG_PRIORITY(1);
    bm_dials_default(); bm_new(seed);
    cur_set = bm_capital(); { const Settle *s = bm_set(cur_set); sel_x = s->x; sel_y = s->y; }
    lot_reset(); sims[0].skin = 2; sims[0].hair = 4; sims[0].hcol = 0; sims[0].shirt = 0; mode = M_CAS; lcdMainOnTop(); hud_clear();
    int yaw = 0, dist = 13000, frames = 0, dirty = 1; unsigned lastsig = 0; int lastmode = -1;
    while (1) {
        scanKeys(); int held = keysHeld(), down = keysDown(); touchPosition tp = {0}; if (held & KEY_TOUCH) touchRead(&tp);
        const Settle *s = bm_set(cur_set); msg_t = msg_t > 0 ? msg_t - 1 : 0;
        unsigned sig = 0; int forced = (mode != lastmode); lastmode = mode;
        if (mode == M_CAS) {
            if (down & KEY_TOUCH) cas_touch(tp.px, tp.py);
            if (forced || (down & KEY_TOUCH)) draw_cas();
            if (cas_done) { cas_done = 0; set_mode(M_MAP); dirty = 1; }
            yaw = sinLerp(frames * 90) >> 1; pitch = 0; dist = 5400;
        } else if (mode == M_MAP) {
            if (down & (KEY_L | KEY_R)) { cur_set = (cur_set + ((down & KEY_R) ? 1 : bm_nset() - 1)) % bm_nset(); s = bm_set(cur_set); sel_x = s->x; sel_y = s->y; dirty = 1; }
            if (down & KEY_START) { seed = seed * 1103515245u + 12345 + frames; bm_new((u32)seed); cur_set = bm_capital(); s = bm_set(cur_set); sel_x = s->x; sel_y = s->y; dirty = 1; }
            if (down & KEY_LEFT) { sel_x--; dirty = 1; } if (down & KEY_RIGHT) { sel_x++; dirty = 1; }
            if (down & KEY_UP) { sel_y--; dirty = 1; } if (down & KEY_DOWN) { sel_y++; dirty = 1; }
            if (held & KEY_X) { if (dist > 3000) dist -= 60; } if (held & KEY_Y) { if (dist < 16000) dist += 60; }
            int enter = down & KEY_A;
            if (down & KEY_TOUCH) { if (tp.py < MH * 8 && tp.px < MW * 8) { int nx = s->x - MW / 2 + tp.px / 8, ny = s->y - MH / 2 + tp.py / 8; if (nx == sel_x && ny == sel_y) enter = 1; else { sel_x = nx; sel_y = ny; dirty = 1; } } }
            else if ((held & KEY_TOUCH) && tp.py < MH * 8 && tp.px < MW * 8) { int nx = s->x - MW / 2 + tp.px / 8, ny = s->y - MH / 2 + tp.py / 8; if (nx != sel_x || ny != sel_y) { sel_x = nx; sel_y = ny; dirty = 1; } }
            if (enter) { BInfo b; bm_block(sel_x, sel_y, &b); if (b.terrain == BT_SEA) say("CANNOT BUILD AT SEA"); else { if (lot_bx != sel_x || lot_by != sel_y) { Sim keep = sims[0]; lot_reset(); sims[0].skin = keep.skin; sims[0].hair = keep.hair; sims[0].hcol = keep.hcol; sims[0].shirt = keep.shirt; lot_bx = sel_x; lot_by = sel_y; } cur_sim = 0; set_mode(M_LIVE); forced = 1; lastsig = ~lastsig; } }
            if (forced) dirty = 1;
            if (dirty) { scr = scr_sub; draw_map(); dirty = 0; }
            if (msg_t > 0) text(4, 150, 1, C15(31, 20, 12), msg);
            yaw += 80; pitch = 3200;
        } else if (mode == M_LIVE) {
            if (down & KEY_TOUCH) live_touch(tp.px, tp.py);
            if (down & KEY_L) yaw_t -= 4096; if (down & KEY_R) yaw_t += 4096;
            if (held & KEY_X) { if (live_dist > 5000) live_dist -= 80; } if (held & KEY_Y) { if (live_dist < 14000) live_dist += 80; }
            live_yaw += (yaw_t - live_yaw) / 4; if (yaw_t - live_yaw > -4 && yaw_t - live_yaw < 4) live_yaw = yaw_t;
            for (int k = 0; k < speed; k++) sims_step(frames * speed + k);
            int tx = sims[cur_sim].x * 2 * T / 16 - GN * T + T, tz = sims[cur_sim].y * 2 * T / 16 - GN * T + T; camx += (tx - camx) / 8; camz += (tz - camz) / 8;
            yaw = live_yaw; dist = live_dist; pitch = 4800;
            sig = (unsigned)(cur_sim * 7 + minutes * 13 + popup * 17 + popup_i * 19 + menu_open * 23 + speed * 29 + msg_t * 31 + funds);
            for (int n = 0; n < 6; n++) sig = sig * 31 + (unsigned)sims[cur_sim].need[n]; sig = sig * 31 + (unsigned)sims[cur_sim].rel;
            if (sig != lastsig || forced) { draw_top_status(); draw_hud(); lastsig = sig; }
        } else {   // BUY / BUILD: the live lot on the top screen, the catalogue or plan on the bottom
            if (down & KEY_TOUCH) ui_touch(tp.px, tp.py);
            if (mode == M_BUY && pick >= 0) {
                if (down & KEY_LEFT) pcx = pcx > 0 ? pcx - 1 : 0; if (down & KEY_RIGHT) pcx = pcx < GN - 1 ? pcx + 1 : GN - 1;
                if (down & KEY_UP) pcy = pcy > 0 ? pcy - 1 : 0; if (down & KEY_DOWN) pcy = pcy < GN - 1 ? pcy + 1 : GN - 1;
                if (down & KEY_R) prot = (prot + 1) & 3;
                if (down & KEY_A) { int c = pcy * GN + pcx; ui_touch(PX + (c % GN) * 12 + 2, PY + (c / GN) * 12 + 2); }
                if (down & KEY_B) pick = -1;
            } else if (mode == M_BUILD) {
                if (down & KEY_LEFT) pcx = pcx > 0 ? pcx - 1 : 0; if (down & KEY_RIGHT) pcx = pcx < GN - 1 ? pcx + 1 : GN - 1;
                if (down & KEY_UP) pcy = pcy > 0 ? pcy - 1 : 0; if (down & KEY_DOWN) pcy = pcy < GN - 1 ? pcy + 1 : GN - 1;
                if ((down & KEY_A) && tool == 0) flr[pcy][pcx] = (u8)pal;
            }
            if (held & KEY_X) { if (dist > 6000) dist -= 80; } if (held & KEY_Y) { if (dist < 20000) dist += 80; }
            sims_step(frames);
            if (mode != M_MAP) {
                sig = (unsigned)(mode * 7 + cat * 13 + pick * 17 + pcx * 31 + pcy * 37 + prot * 41 + tool * 43 + pal * 47 + funds * 59 + nobj * 61 + (msg_t > 0) * 71 + msg_t * 73 + forced);
                if (sig != lastsig || (down & KEY_TOUCH)) { scr = scr_sub; backdrop(); draw_ui(); lastsig = sig; } }
            yaw = 3000; pitch = 3800; dist = 13000;
        }
        polys = 0;
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspectivef32(degreesToAngle(50), 256 * 4096 / 192, 4096 / 8, 4096 * 40);
        glMatrixMode(GL_POSITION); glLoadIdentity();
        glTranslatef32(0, 0, -dist); glRotateXi(pitch); glRotateYi(yaw);
        if (mode == M_LIVE) glTranslatef32(-camx, 0, -camz); else if (mode == M_CAS) glTranslatef32(0, -600, 0);
        glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(1));
        glBegin(GL_QUADS);
        if (mode == M_CAS) draw_head(yaw);
        else if (mode == M_MAP) { BInfo b; bm_block(sel_x, sel_y, &b); draw_lot(&b, yaw); }
        else draw_build(yaw);
        glEnd();
        glFlush(0); swiWaitForVBlank(); frames++;
    }
    return 0;
}
