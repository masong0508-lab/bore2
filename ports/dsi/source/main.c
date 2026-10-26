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
static u16 *scr;
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
enum { M_MAP, M_LIVE, M_BUY, M_BUILD };
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
typedef struct { u8 x, y, c, i, rot; } Obj;
static Obj objs[48]; static int nobj, funds = 20000;
static u8 flr[GN][GN], hwl[GN + 1][GN], vwl[GN][GN + 1];     // floors (0 none, else 1..5), walls on the cell edges (0 none, else 1..4)
static int mode = M_MAP, cat, pick = -1, pcx = 6, pcy = 6, prot, tool, pal, lot_bx = -1, lot_by = -1, cam = 3000, pitch = 3200, msg_t;
static const char *msg = "";
typedef struct { int x, y, tx, ty, need[3]; } Sim;           // x, y in 1/16 cells
static Sim sims[3]; static int cur_sim, minutes = 8 * 60;
static void say(const char *m) { msg = m; msg_t = 90; }
static int obj_at(int x, int y) { for (int i = 0; i < nobj; i++) if (objs[i].x == x && objs[i].y == y) return i; return -1; }
static void lot_reset(void) {
    memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; funds = 20000; pick = -1; mode = M_LIVE; cat = 0; tool = 0; pal = 1;
    for (int y = 4; y < 10; y++) for (int x = 3; x < 10; x++) flr[y][x] = 1;                       // a little starter room so there is something to see
    for (int x = 3; x < 10; x++) { hwl[4][x] = 1; hwl[10][x] = 1; } for (int y = 4; y < 10; y++) { vwl[y][3] = 1; vwl[y][10] = 1; } hwl[10][6] = 0;   // (a gap in the front wall: the door)
    for (int i = 0; i < 3; i++) { sims[i].x = (5 + i) * 16; sims[i].y = 11 * 16; sims[i].tx = sims[i].x; sims[i].ty = sims[i].y; for (int n = 0; n < 3; n++) sims[i].need[n] = 70 - i * 10 - n * 8; }
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
        for (int i = 0; i < 3; i++) { button(6 + i * 82, 22, 78, 18, i == 0 ? "ALEX" : i == 1 ? "SAM" : "KIT", C15(20, 10, 22), cur_sim == i); }
        const char *nn[3] = { "HUNGER", "ENERGY", "FUN" }; u16 nc[3] = { C15(28, 18, 6), C15(8, 22, 12), C15(12, 14, 31) };
        for (int n = 0; n < 3; n++) { text(10, 56 + n * 22, 1, C15(31, 31, 31), nn[n]); bar(60, 54 + n * 22, 150, sims[cur_sim].need[n], nc[n]); }
        text(10, 130, 1, C15(20, 24, 28), "TAP A SIM TO SWITCH. USE BUY AND BUILD"); text(10, 140, 1, C15(20, 24, 28), "TO FURNISH THE LOT. L/R TURN THE VIEW.");
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
        if (i == M_MAP) { mode = M_MAP; pick = -1; } else { mode = i; pick = -1; }
        return; }
    if (mode == M_LIVE) { for (int i = 0; i < 3; i++) if (hit(6 + i * 82, 22, 78, 18, tx, ty)) cur_sim = i; }
    else if (mode == M_BUY && pick < 0) {
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
    for (int i = 0; i < 3; i++) person(sims[i].x * 2 * T / 16 - GN * T + T / 2 * 0 + T, sims[i].y * 2 * T / 16 - GN * T + T, i == cur_sim ? C15(31, 31, 8) : C15(10 + i * 8, 8, 28 - i * 8), yaw);
    if (mode == M_BUY || mode == M_BUILD) {   // the cursor
        ground(tl(pcx * 2), tl(pcy * 2), tl(pcx * 2 + 2), tl(pcy * 2 + 2), -80, C15(31, 31, 31));
        if (mode == M_BUY && pick >= 0) obj3d(pcx, pcy, &ITEMS[cat][pick], prot, 1);
    }
}
static void sims_step(int frames) {
    for (int i = 0; i < 3; i++) { Sim *s = &sims[i];
        if (s->x == s->tx && s->y == s->ty) { if (((frames + i * 37) & 63) == 0) { s->tx = (1 + (frames * 7 + i * 5) % 11) * 16; s->ty = (1 + (frames * 3 + i * 9) % 12) * 16; } }
        else { if (s->x < s->tx) s->x++; else if (s->x > s->tx) s->x--; else if (s->y < s->ty) s->y++; else s->y--; }
        if ((frames & 255) == 0) for (int n = 0; n < 3; n++) { if (s->need[n] > 5) s->need[n]--; } }
    if ((frames & 15) == 0) minutes++;
}

int main(void) {
    powerOn(POWER_ALL_2D | POWER_3D_CORE | POWER_MATRIX); irqInit(); irqEnable(IRQ_VBLANK);
    videoSetMode(MODE_0_3D);
    videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    vramSetBankA(VRAM_A_TEXTURE); vramSetBankB(VRAM_B_TEXTURE); vramSetBankC(VRAM_C_SUB_BG);
    int bg = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0); scr = bgGetGfxPtr(bg);
    glInit(); glEnable(GL_ANTIALIAS); glClearColor(14, 20, 31, 31); glClearPolyID(63); glClearDepth(0x7FFF); glViewport(0, 0, 255, 191);
    bm_dials_default(); bm_new(seed);
    cur_set = bm_capital(); { const Settle *s = bm_set(cur_set); sel_x = s->x; sel_y = s->y; }
    int yaw = 0, dist = 13000, frames = 0, dirty = 1, lastsel = -1; unsigned lastsig = 0;
    while (1) {
        scanKeys(); int held = keysHeld(), down = keysDown(); touchPosition tp = {0}; if (held & KEY_TOUCH) touchRead(&tp);
        const Settle *s = bm_set(cur_set); msg_t = msg_t > 0 ? msg_t - 1 : 0;
        if (mode == M_MAP) {
            if (down & (KEY_L | KEY_R)) { cur_set = (cur_set + ((down & KEY_R) ? 1 : bm_nset() - 1)) % bm_nset(); s = bm_set(cur_set); sel_x = s->x; sel_y = s->y; dirty = 1; }
            if (down & KEY_START) { seed = seed * 1103515245u + 12345 + frames; bm_new((u32)seed); cur_set = bm_capital(); s = bm_set(cur_set); sel_x = s->x; sel_y = s->y; dirty = 1; }
            if (down & KEY_LEFT) { sel_x--; dirty = 1; } if (down & KEY_RIGHT) { sel_x++; dirty = 1; }
            if (down & KEY_UP) { sel_y--; dirty = 1; } if (down & KEY_DOWN) { sel_y++; dirty = 1; }
            if (held & KEY_X) { if (dist > 3000) dist -= 60; } if (held & KEY_Y) { if (dist < 12000 + 4000) dist += 60; }
            int enter = down & KEY_A;
            if (down & KEY_TOUCH) { if (tp.py < MH * 8 && tp.px < MW * 8) { int nx = s->x - MW / 2 + tp.px / 8, ny = s->y - MH / 2 + tp.py / 8; if (nx == sel_x && ny == sel_y) enter = 1; else { sel_x = nx; sel_y = ny; dirty = 1; } } }
            else if ((held & KEY_TOUCH) && tp.py < MH * 8 && tp.px < MW * 8) { int nx = s->x - MW / 2 + tp.px / 8, ny = s->y - MH / 2 + tp.py / 8; if (nx != sel_x || ny != sel_y) { sel_x = nx; sel_y = ny; dirty = 1; } }
            if (enter) { BInfo b; bm_block(sel_x, sel_y, &b); if (b.terrain == BT_SEA) say("CANNOT BUILD AT SEA"); else { if (lot_bx != sel_x || lot_by != sel_y) { lot_reset(); lot_bx = sel_x; lot_by = sel_y; } mode = M_LIVE; dirty = 1; } }
            if (dirty) { draw_map(); dirty = 0; }
            if (msg_t > 0) text(4, 150, 1, C15(31, 20, 12), msg);   // (drawn over the map's corner)
            yaw += 80; pitch = 3200; dist = dist < 3000 ? 3000 : dist;
        } else {
            if (down & KEY_TOUCH) { ui_touch(tp.px, tp.py); lastsel = -1; lastsig = ~lastsig; }
            if (mode == M_MAP) dirty = 1;
            if (mode == M_BUY && pick >= 0) {
                if (down & KEY_LEFT) pcx = pcx > 0 ? pcx - 1 : 0; if (down & KEY_RIGHT) pcx = pcx < GN - 1 ? pcx + 1 : GN - 1;
                if (down & KEY_UP) pcy = pcy > 0 ? pcy - 1 : 0; if (down & KEY_DOWN) pcy = pcy < GN - 1 ? pcy + 1 : GN - 1;
                if (down & KEY_R) prot = (prot + 1) & 3;
                if (down & KEY_A) { /* place */ int c = pcy * GN + pcx; int tx = PX + (c % GN) * 12 + 2, ty = PY + (c / GN) * 12 + 2; ui_touch(tx, ty); }
                if (down & KEY_B) pick = -1;
            } else if (mode == M_BUILD) {
                if (down & KEY_LEFT) pcx = pcx > 0 ? pcx - 1 : 0; if (down & KEY_RIGHT) pcx = pcx < GN - 1 ? pcx + 1 : GN - 1;
                if (down & KEY_UP) pcy = pcy > 0 ? pcy - 1 : 0; if (down & KEY_DOWN) pcy = pcy < GN - 1 ? pcy + 1 : GN - 1;
                if ((down & KEY_A) && tool == 0) flr[pcy][pcx] = (u8)pal;
            } else { if (down & KEY_L) yaw -= 4096; if (down & KEY_R) yaw += 4096; }
            if (held & KEY_X) { if (dist > 6000) dist -= 80; } if (held & KEY_Y) { if (dist < 20000) dist += 80; }
            if (mode != M_MAP) { sims_step(frames);
                unsigned sig = (unsigned)(mode * 7 + cat * 13 + pick * 17 + pcx * 31 + pcy * 37 + prot * 41 + tool * 43 + pal * 47 + cur_sim * 53 + funds * 59 + nobj * 61 + minutes * 67 + (msg_t > 0) * 71 + msg_t * 73);
                for (int n = 0; n < 3; n++) sig = sig * 31 + (unsigned)sims[cur_sim].need[n];
                if (sig != lastsig || (down & KEY_TOUCH)) { draw_ui(); lastsig = sig; } }
            else { draw_map(); }
            pitch = 3800;
        }
        polys = 0;
        BInfo b; bm_block(sel_x, sel_y, &b);
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspectivef32(degreesToAngle(50), 256 * 4096 / 192, 4096 / 8, 4096 * 40);
        glMatrixMode(GL_POSITION); glLoadIdentity();
        glTranslatef32(0, 0, -dist); glRotateXi(pitch); glRotateYi(yaw);
        glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(1));
        glBegin(GL_QUADS);
        if (mode == M_MAP) draw_lot(&b, yaw); else draw_build(yaw);
        glEnd();
        glFlush(0); swiWaitForVBlank(); frames++;
    }
    return 0;
}
