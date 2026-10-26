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
static void rect(int x, int y, int w, int h, u16 c) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) px(x + i, y + j, c); }
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

int main(void) {
    powerOn(POWER_ALL_2D | POWER_3D_CORE | POWER_MATRIX); irqInit(); irqEnable(IRQ_VBLANK);
    videoSetMode(MODE_0_3D);
    videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    vramSetBankA(VRAM_A_TEXTURE); vramSetBankB(VRAM_B_TEXTURE); vramSetBankC(VRAM_C_SUB_BG);
    int bg = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0); scr = bgGetGfxPtr(bg);
    glInit(); glEnable(GL_ANTIALIAS); glClearColor(14, 20, 31, 31); glClearPolyID(63); glClearDepth(0x7FFF); glViewport(0, 0, 255, 191);
    bm_dials_default(); bm_new(seed);
    cur_set = bm_capital(); { const Settle *s = bm_set(cur_set); sel_x = s->x; sel_y = s->y; }
    int yaw = 0, dist = 13000, pitch = 3200, frames = 0, dirty = 1;
    while (1) {
        scanKeys(); int held = keysHeld(), down = keysDown();
        const Settle *s = bm_set(cur_set);
        if (down & (KEY_L | KEY_R)) { cur_set = (cur_set + ((down & KEY_R) ? 1 : bm_nset() - 1)) % bm_nset(); s = bm_set(cur_set); sel_x = s->x; sel_y = s->y; dirty = 1; }
        if (down & KEY_START) { seed = seed * 1103515245u + 12345 + frames; bm_new((u32)seed); cur_set = bm_capital(); s = bm_set(cur_set); sel_x = s->x; sel_y = s->y; dirty = 1; }
        if (down & KEY_LEFT) { sel_x--; dirty = 1; } if (down & KEY_RIGHT) { sel_x++; dirty = 1; }
        if (down & KEY_UP) { sel_y--; dirty = 1; } if (down & KEY_DOWN) { sel_y++; dirty = 1; }
        if (held & KEY_A) { if (dist > 3000) dist -= 60; } if (held & KEY_B) { if (dist < 12000) dist += 60; }
        if (held & KEY_TOUCH) {   // the stylus: tap a lot
            touchPosition tp; touchRead(&tp);
            if (tp.py < MH * 8 && tp.px < MW * 8) { int nx = s->x - MW / 2 + tp.px / 8, ny = s->y - MH / 2 + tp.py / 8; if (nx != sel_x || ny != sel_y) { sel_x = nx; sel_y = ny; dirty = 1; } }
        }
        if (dirty) { draw_map(); dirty = 0; }
        yaw += 80; polys = 0;
        BInfo b; bm_block(sel_x, sel_y, &b);
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspectivef32(degreesToAngle(50), 256 * 4096 / 192, 4096 / 8, 4096 * 40);
        glMatrixMode(GL_POSITION); glLoadIdentity();
        glTranslatef32(0, 0, -dist); glRotateXi(pitch); glRotateYi(yaw);
        glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(1));
        glBegin(GL_QUADS);
        draw_lot(&b, yaw);
        glEnd();
        glFlush(0); swiWaitForVBlank(); frames++;
    }
    return 0;
}
