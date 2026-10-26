// BORE 2 for the DSi, step D1: the neighbourhood.
// Bottom screen: the 2D neighbourhood map. Tap a lot with the stylus (or use the D-pad) to pick it.
// Top screen: that lot in 3D. L/R change town, A/B zoom, START re-rolls the world.
// Fixed point only; the world is bigmap.c, shared with the PS1 build (nothing of it is stored).
#define PMN 28
static int o_aa = 1, o_out, o_shd = 1, o_det = 1, o_fps30, o_bri = 4, o_sky, o_chk = 1, o_spin = 2, o_turbo, o_cam = 1, o_spd, o_page, opt_on, o_dirty = 1, o_lit = 1, lit_on, lit_sgn = -1;
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
#include "font8.h"
static int tw(int s, const char *t) {   // the width of a string in pixels
    int w = 0; for (; *t; t++) { int c = (unsigned char)*t; if (c < 32 || c > 126) continue; int m = 0; for (int r = 0; r < 8; r++) { int v = FONT8[c - 32][r]; for (int x = 7; x >= 0; x--) if (v >> (7 - x) & 1) { if (x + 1 > m) m = x + 1; } } w += (c == 32 ? 4 : m + 1) + (s >= 2); }
    return w;
}
static void text(int x, int y, int s, u16 c, const char *t) {   // s 1: normal, s 2: bold; a soft shadow under it
    int dark = ((c & 31) + ((c >> 5) & 31) + ((c >> 10) & 31)) > 45; u16 sh = C15(0, 2, 8);
    for (; *t; t++) { int ch = (unsigned char)*t; if (ch < 32 || ch > 126) continue; int m = 0;
        for (int r = 0; r < 8; r++) { int v = FONT8[ch - 32][r]; for (int q = 0; q < 8; q++) if (v >> (7 - q) & 1) { if (q + 1 > m) m = q + 1; } }
        for (int r = 0; r < 8; r++) { int v = FONT8[ch - 32][r]; for (int q = 0; q < 8; q++) if (v >> (7 - q) & 1) {
            if (dark) { px(x + q + 1, y + r - 1, sh); if (s >= 2) px(x + q + 2, y + r - 1, sh); }
            px(x + q, y + r - 2, c); if (s >= 2) px(x + q + 1, y + r - 2, c); } }
        x += (ch == 32 ? 4 : m + 1) + (s >= 2); }
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
static int hit(int x, int y, int w, int h, int tx, int ty);
static int ne_on, ne_tool, ne_lot_i, ne_g_i, ne_road = 1, ne_last_x = -1, ne_last_y;
static const u8 NELOT[] = { LOT_HOUSE, LOT_COTTAGE, LOT_TOWN, LOT_TWO, LOT_APART, LOT_SHOP, LOT_DINER, LOT_OFFICE, LOT_PARK, LOT_SCHOOL, LOT_GYM, LOT_CAFE, LOT_TOWER, LOT_EMPTY, LOT_FARM };
static const u8 NEG[] = { BT_GRASS, BT_FOREST, BT_DESERT, BT_ROCKY, BT_BEACH, BT_SEA };
static const char *const NEGN[] = { "Grass", "Forest", "Desert", "Rocky", "Beach", "Sea" };
static const char *const NETN[7] = { "Lot", "Ground", "Road", "Up", "Down", "Wild", "Undo" };
static int ne_near(int bx, int by) { int bi = 0, bd = 1 << 30; for (int i = 0; i < bm_nset(); i++) { const Settle *s2 = bm_set(i); int dx = bx - s2->x, dy = by - s2->y, d = dx * dx + dy * dy; if (d < bd) { bd = d; bi = i; } } return bi; }
static void ne_cell(int bx, int by) {   // the tool on one block, the same changes the PS1 neighbourhood editor makes
    if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N) return;
    BInfo b0; bm_block(bx, by, &b0);
    switch (ne_tool) {
        case 1: bm_ovr_set(bx, by, BO_GROUND, NEG[ne_g_i]); break;
        case 3: case 4: { int dl = bm_ovr_delta(bx, by) + (ne_tool == 3 ? 2 : -2); dl = dl < -8 ? -8 : dl > 7 ? 7 : dl; bm_ovr_unset(bx, by, BO_GROUND); bm_ovr_set(bx, by, BO_ELEV, dl & 15); } break;
        case 2: bm_ovr_set(bx, by, BO_ROAD, ne_road ? 3 : 0); break;
        case 0: { int lot = NELOT[ne_lot_i]; if (b0.set == 255) { bm_ovr_set(bx, by, BO_TERR, ne_near(bx, by)); bm_ovr_set(bx, by, BO_ROAD, 3); } bm_ovr_set(bx, by, BO_LOT, lot); } break;
        case 5: bm_ovr_set(bx, by, BO_TERR, 255); bm_ovr_set(bx, by, BO_ROAD, 0); bm_ovr_set(bx, by, BO_LOT, LOT_WILD); break;
        case 6: bm_ovr_set(bx, by, BO_NONE, 0); break;
    }
}
static void chip(int x, int y, int w, int h, const char *t, int on) {
    rect(x, y, w, h, on ? C15(31, 31, 20) : C15(2, 4, 12)); rect(x + 1, y + 1, w - 2, h - 2, on ? C15(10, 24, 10) : C15(6, 12, 26)); text(x + w / 2 - tw(1, t) / 2, y + 3, 1, C15(31, 31, 31), t);
}
static void map_chip(int px_, int py_) {   // a tap on the strip under the map
    if (!ne_on) { if (hit(212, 162, 40, 13, px_, py_)) ne_on = 1; return; }
    for (int i = 0; i < 7; i++) if (hit(2 + i * 36, 162, 34, 13, px_, py_)) ne_tool = i;
    if (hit(2, 177, 110, 13, px_, py_)) { if (ne_tool == 0) ne_lot_i = (ne_lot_i + 1) % (int)sizeof NELOT; else if (ne_tool == 1) ne_g_i = (ne_g_i + 1) % 6; else if (ne_tool == 2) ne_road ^= 1; }
    if (hit(212, 177, 40, 13, px_, py_)) ne_on = 0;
}
#define DS_POP_MAX 2000                // the most bores you can meet, talk to and take into a household on this map (the PS1 has 3700)
#define DS_FAM_MAX 640                 // the most households (the PS1 has 874 to 1748); every home on the map belongs to one of them
typedef struct { u16 bx, by; u8 fam, per; } PopB;
static PopB popb[DS_FAM_MAX]; static int pop_nb, pop_fam, pop_per, pop_born;
static void pop_build(void) {   // the homes nearest the capital (then the next town, and so on) fill the household list until the caps are hit
    pop_nb = pop_fam = pop_per = pop_born = 0;
    for (int si = 0; si < bm_nset() && pop_per < DS_POP_MAX && pop_fam < DS_FAM_MAX; si++) { const Settle *st = bm_set(si); int r = st->r + 1; if (r > 14) r = 14;
        for (int d = 0; d <= r && pop_per < DS_POP_MAX && pop_fam < DS_FAM_MAX; d++) for (int yy = -d; yy <= d; yy++) for (int xx = -d; xx <= d; xx++) {
            if ((xx < 0 ? -xx : xx) != d && (yy < 0 ? -yy : yy) != d) continue;
            int bx = st->x + xx, by = st->y + yy; if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N || pop_nb >= DS_FAM_MAX) continue;
            BInfo b; bm_block(bx, by, &b); if (b.terrain == BT_SEA || b.lot < LOT_HOUSE || b.lot > LOT_APART) continue;
            int fam = bm_units(&b); if (fam < 1) fam = 1; if (fam > DS_FAM_MAX - pop_fam) fam = DS_FAM_MAX - pop_fam; if (fam < 1) continue;
            int per = 0; for (int f = 0; f < fam; f++) per += 1 + (int)((b.h >> (f & 15) * 2) & 3) % 4; if (per > DS_POP_MAX - pop_per) per = DS_POP_MAX - pop_per; if (per < fam) fam = per; if (fam < 1) continue;
            popb[pop_nb].bx = (u16)bx; popb[pop_nb].by = (u16)by; popb[pop_nb].fam = (u8)(fam > 255 ? 255 : fam); popb[pop_nb].per = (u8)(per > 255 ? 255 : per); pop_nb++; pop_fam += fam; pop_per += per; } }
}
static int pop_at(int bx, int by, int *fam) { for (int i = 0; i < pop_nb; i++) if (popb[i].bx == bx && popb[i].by == by) { if (fam) *fam = popb[i].fam; return popb[i].per; } if (fam) *fam = 0; return 0; }
static void town_pick(int si) {   // the nearest home inside that settlement (so a new life starts in a town, with a street and neighbours)
    const Settle *st = bm_set(si); sel_x = st->x; sel_y = st->y;
    for (int d = 0; d <= 16; d++) for (int yy = -d; yy <= d; yy++) for (int xx = -d; xx <= d; xx++) { if ((xx < 0 ? -xx : xx) != d && (yy < 0 ? -yy : yy) != d) continue;
        int bx = st->x + xx, by = st->y + yy; if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N) continue; BInfo b; bm_block(bx, by, &b);
        if (b.terrain != BT_SEA && b.set != 255 && b.lot >= LOT_HOUSE && b.lot <= LOT_APART) { sel_x = bx; sel_y = by; return; } }
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
    if (!ne_on) {
        sprintf(t, "%s  %d/%d", s->name, cur_set + 1, bm_nset()); text(4, 164, 2, C15(31, 31, 22), t);
        { int fm; int pp = pop_at(sel_x, sel_y, &fm); if (pp) sprintf(t, "%s %dh %d bores", bm_lot_name(b.lot), fm, pp); else sprintf(t, "%s  %d,%d", bm_lot_name(b.lot), sel_x, sel_y); text(4, 178, 2, C15(24, 28, 31), t); sprintf(t, "%d/%d", pop_per + pop_born, DS_POP_MAX); text(176, 164, 1, C15(24, 28, 31), t); }
        chip(212, 162, 40, 13, "Edit", 0);
    } else {
        for (int i = 0; i < 7; i++) chip(2 + i * 36, 162, 34, 13, NETN[i], ne_tool == i);
        const char *v = ne_tool == 0 ? bm_lot_name(NELOT[ne_lot_i]) : ne_tool == 1 ? NEGN[ne_g_i] : ne_tool == 2 ? (ne_road ? "Add road" : "Remove road") : 0;
        if (v) chip(2, 177, 110, 13, v, 0); else text(4, 179, 1, C15(24, 28, 31), "Drag across the map");
        text(118, 179, 1, C15(24, 28, 31), v ? "tap = next" : "");
        chip(212, 177, 40, 13, "Done", 0);
    }
}

// ---------- the top screen: one lot in 3D ----------
#define T 330                  // a tile in 4.12 fixed point (1.0 = 4096), so a 13-tile lot is about a unit wide
#define LOTN 13
static int polys;
static inline int tl(int t) { return (t - LOTN) * T; }   // tile coordinate -> centred model coordinate
static void qd(int x0, int y0, int z0, int x1, int y1, int z1, int x2, int y2, int z2, int x3, int y3, int z3, u16 c) {
    if (lit_on) {   // lit: set the material from the colour, then the normal (which computes the lit colour); no glColor after it
        float ux = (float)(x1 - x0), uy = (float)(y0 - y1), uz = (float)(z1 - z0), vx = (float)(x3 - x0), vy = (float)(y0 - y3), vz = (float)(z3 - z0);
        float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx, l = nx * nx + ny * ny + nz * nz;
        glMaterialf(GL_DIFFUSE, (u16)(c | BIT(15))); glMaterialf(GL_AMBIENT, (u16)(RGB15(((c & 31) * 11) >> 4, (((c >> 5) & 31) * 11) >> 4, (((c >> 10) & 31) * 11) >> 4)));
        if (l > 0) { float k = 511.0f * lit_sgn / __builtin_sqrtf(l); glNormal(NORMAL_PACK((int)(nx * k), (int)(ny * k), (int)(nz * k))); }
    } else glColor(c);
    glVertex3v16(x0, -y0, z0); glVertex3v16(x1, -y1, z1); glVertex3v16(x2, -y2, z2); glVertex3v16(x3, -y3, z3); polys += 2;
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
static void tree(int x, int z, int s) {   // a trunk, three tiers of leaves, a bright top
    block(x - s / 8, z - s / 8, x + s / 8, z + s / 8, 0, -s * 5 / 10, C15(13, 8, 4), C15(13, 8, 4));
    block(x - s / 2, z - s / 2, x + s / 2, z + s / 2, -s * 4 / 10, -s * 7 / 10, C15(4, 15, 5), C15(6, 19, 6));
    block(x - s * 36 / 100, z - s * 36 / 100, x + s * 36 / 100, z + s * 36 / 100, -s * 7 / 10, -s * 95 / 100, C15(5, 17, 5), C15(8, 22, 7));
}
static void person(int x, int z, u16 c, int yaw) {   // a person: one quad that faces the camera
    int s = T, hx = (s * sinLerp(yaw)) >> 13, hz = (s * cosLerp(yaw)) >> 13;
    qd(x - hx, 0, z - hz, x + hx, 0, z + hz, x + hx, -s * 2, z + hz, x - hx, -s * 2, z - hz, c);
}
static void draw_lot(const BInfo *b, int yaw) {
    u32 h = b->h; int k = b->lot;
    u16 grass = b->terrain == BT_DESERT ? C15(24, 20, 10) : b->terrain == BT_ROCKY ? C15(13, 13, 13) : b->terrain == BT_BEACH ? C15(26, 24, 14) : C15(11, 20, 8);
    if (b->terrain == BT_SEA) { grass = C15(5, 12, 24); }
    for (int z = 0; z < LOTN; z++) for (int x = 0; x < LOTN; x++) ground(tl(x * 2), tl(z * 2), tl(x * 2 + 2), tl(z * 2 + 2), 0, (((x + z) & 1) && o_chk) ? shade(grass, 7) : grass);
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
    { int wn = (X1 - X0) / 5; for (int r = 0; r < st; r++) for (int k = 0; k < 3; k++) { int wx = X0 + wn * (1 + k * 2 - (k == 1 && r == 0 ? 0 : 0)); if (r == 0 && k == 1) { block(wx, Z1, wx + wn * 7 / 10, Z1 + 14, 0, -T * 3 / 2, C15(12, 7, 4), C15(12, 7, 4)); continue; } block(wx, Z1, wx + wn * 7 / 10, Z1 + 14, -T * (1 + r * 2) , -T * (2 + r * 2), C15(17, 24, 31), C15(17, 24, 31)); } }   // front door and windows
    block((X0 + X1) / 2 + T, Z0 + T / 2, (X0 + X1) / 2 + T * 3 / 2, Z0 + T, Y, Y - T * 3 / 2, C15(12, 6, 5), C15(12, 6, 5));   // a chimney
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
enum { M_MAP, M_LIVE, M_BUY, M_BUILD, M_CAS, M_TITLE };
typedef struct { const char *name; int price; u8 h; u16 col; u8 kind; } Item;   // kind: 0 box, 1 flat (rug), 2 plant
static const Item ITEMS[7][4] = {
    { {"Chair", 80, 2, C15(24, 16, 8), 0}, {"Armchair", 450, 2, C15(10, 14, 24), 0}, {"Sofa", 900, 2, C15(24, 8, 8), 0}, {"Bench", 150, 1, C15(20, 14, 8), 0} },
    { {"Dining", 300, 2, C15(22, 15, 8), 0}, {"Desk", 220, 2, C15(18, 18, 20), 0}, {"Coffee", 120, 1, C15(16, 10, 6), 0}, {"Counter", 260, 3, C15(26, 26, 24), 0} },
    { {"TV", 600, 3, C15(3, 3, 4), 0}, {"Computer", 900, 2, C15(20, 20, 22), 0}, {"Stereo", 350, 2, C15(6, 6, 8), 0}, {"Fridge", 700, 5, C15(28, 28, 30), 0} },
    { {"Lamp", 60, 4, C15(31, 28, 14), 0}, {"Rug", 100, 0, C15(22, 8, 12), 1}, {"Plant", 40, 3, C15(5, 18, 6), 2}, {"Statue", 500, 4, C15(24, 24, 22), 0} },
    { {"Bed", 800, 2, C15(20, 24, 31), 0}, {"Bathtub", 650, 2, C15(28, 30, 31), 0}, {"Toilet", 250, 2, C15(30, 30, 30), 0}, {"Sink", 200, 3, C15(26, 28, 30), 0} },
    { {"Crib", 400, 3, C15(28, 24, 16), 0}, {"Toy Box", 150, 2, C15(31, 20, 6), 0}, {"Books", 250, 4, C15(18, 12, 6), 0}, {"Rocker", 300, 2, C15(22, 14, 10), 0} },
    { {"Fence", 40, 2, C15(28, 26, 20), 0}, {"Lamp Post", 120, 6, C15(6, 6, 8), 0}, {"Hedge", 60, 2, C15(6, 18, 6), 0}, {"Flowers", 30, 0, C15(28, 10, 14), 1} } };
static const u8 ITMASK[28] = { 62, 62, 62, 62, 62, 60, 62, 56, 62, 60, 62, 62, 63, 63, 63, 63, 62, 62, 62, 62, 3, 7, 60, 48, 63, 63, 63, 63 };   // which life stages may use each item (bit 0 baby .. bit 5 elder)
static const char *const CATN[7] = { "Seat", "Table", "Tech", "Deco", "Room", "Kids", "Yard" };
static const u16 FLOORC[5] = { C15(22, 16, 9), C15(26, 26, 24), C15(10, 16, 24), C15(14, 22, 12), C15(8, 8, 10) };
static const u16 WALLC[5] = { C15(30, 28, 22), C15(28, 20, 16), C15(20, 24, 30), C15(30, 30, 30), C15(13, 8, 5) };   // 5 = a door
static int mark_x = -1, mark_y, mark_t;
static void set_mode(int m);
static int cas_mult = 1;
static void eff_pm(const u8 *pm, int *p) { for (int q = 0; q < PMN; q++) { int v = 50 + (pm[q] - 50) * cas_mult; p[q] = v; } }
static u32 lot_seed = 1;
static int camx, camz, door_cx, door_cy;
static int gframes, cam_on, cam_pan, cam_free, live_pitch = 4300, cam_deff = 9000, bb_yaw = 3000, bb_pitch = 3900, bb_dist = 12500, cam_lx, cam_ly, cam_drag;
static void sim3d(int i, int yaw);
typedef struct { u8 x, y, c, i, rot; } Obj;
static Obj objs[400]; static int nobj, funds = 20000;
static u8 flr[GN][GN], hwl[GN + 1][GN], vwl[GN][GN + 1];     // floors (0 none, else 1..5), walls on the cell edges (0 none, else 1..4)
static int mode = M_MAP, cat, pick = -1, pcx = 6, pcy = 6, prot, tool, pal, lot_bx = -1, lot_by = -1, cam = 3000, pitch = 3200, msg_t;
static const char *msg = "";
typedef struct { int x, y, tx, ty, wx, wy, need[6], acc[6], act, act_left, act_amt, act_obj, face, rel, dirn, age, soc, tgt; u8 skin, hair, hcol, shirt, beard, fc, pm[PMN]; u16 cc[3]; } Sim;   // x, y in 1/16 cells; needs 0..100: hunger, fun, energy, social, bladder, hygiene
#define SIM_MAX 6
static Sim sims[SIM_MAX]; static int cur_sim, nsim = 3, ctl, minutes = 11 * 60 + 40;
static char sname[SIM_MAX][20] = { "Ashley Newbie", "Ronald Bacon", "Kit Farlow" };
static const char *const STGN[6] = { "Baby", "Toddler", "Child", "Teen", "Adult", "Elder" };   // the life stages: 0-2, 3-5, 6-12, 13-17, 18-59, 60+ years (a year is a day on the clock)
static int stage_of(int a) { return a < 3 ? 0 : a < 6 ? 1 : a < 13 ? 2 : a < 18 ? 3 : a < 60 ? 4 : 5; }
static const int STSC[6] = { 36, 50, 68, 88, 100, 95 };   // how tall each stage is, in percent
static u8 ilim[SIM_MAX][SIM_MAX];   // how many times two household members have talked today
static const u16 SKINC[6] = { C15(30, 24, 20), C15(27, 20, 15), C15(22, 15, 10), C15(16, 10, 6), C15(11, 7, 4), C15(29, 22, 17) };
static const u16 HAIRC[6] = { C15(3, 3, 4), C15(14, 8, 4), C15(26, 20, 8), C15(24, 8, 4), C15(24, 24, 24), C15(10, 14, 28) };
static const u16 SHIRTC[6] = { C15(26, 26, 26), C15(24, 8, 8), C15(8, 14, 26), C15(8, 22, 10), C15(28, 22, 8), C15(16, 8, 22) };
static inline u16 skin_of(const Sim *m) { return m->cc[0] ? m->cc[0] : SKINC[m->skin]; }
static inline u16 hair_of(const Sim *m) { return m->cc[1] ? m->cc[1] : HAIRC[m->hcol]; }
static inline u16 shirt_of(const Sim *m) { return m->cc[2] ? m->cc[2] : SHIRTC[m->shirt]; }
static const char *const HAIRN[12] = { "Short", "Bob", "Spikes", "Long", "Braids", "Bald", "Buzz", "Mohawk", "Afro", "Ponytail", "Fringe", "Pigtails" };
static const char *const BEARDN[6] = { "None", "Stubble", "Mustache", "Goatee", "Beard", "Long" };
static const char *const FACEN[6] = { "Classic", "Round", "Long", "Big Eyes", "Sleepy", "Freckles" };
static void say(const char *m) { msg = m; msg_t = 90; }
static int obj_at(int x, int y) { for (int i = 0; i < nobj; i++) if (objs[i].x == x && objs[i].y == y) return i; return -1; }
static int spawn_x[3], spawn_y[3];

// ---------- the open street world (D16): the lot you live on is the middle of a 3x3 patch of lot slots; walk off it and the next lot is there ----------
#define SLC 18                         // a slot is 18 cells (36 tiles): the lot (13 cells), a pavement, a 3-cell road, a pavement
static int nb_ok; static BInfo nb[3][3]; static u8 nb_pop[3][3], nb_fam[3][3];
static int fdiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }
static void nb_load(void) { for (int dy = 0; dy < 3; dy++) for (int dx = 0; dx < 3; dx++) { int x = lot_bx + dx - 1, y = lot_by + dy - 1;
    if (x < 0 || y < 0 || x >= BM_N || y >= BM_N) { memset(&nb[dy][dx], 0, sizeof(BInfo)); nb[dy][dx].terrain = BT_SEA; nb_pop[dy][dx] = 0; nb_fam[dy][dx] = 0; }
    else { bm_block(x, y, &nb[dy][dx]); int f; nb_pop[dy][dx] = (u8)pop_at(x, y, &f); nb_fam[dy][dx] = (u8)f; } } }
static void city_spawn(void); static void city_shift(int dx, int dy);
static int far_house(const BInfo *b) { return b->terrain != BT_SEA && b->lot >= LOT_HOUSE && b->lot != LOT_PARK && b->lot != LOT_SQUARE && b->lot != LOT_SKATE && b->lot != LOT_EMPTY && b->lot != LOT_RIVER; }
static void far_foot(const BInfo *b, int *x0, int *z0, int *x1, int *z1) { int w = 10 + (int)(b->h & 3) * 2, d = 8 + (int)((b->h >> 2) & 3) * 2, t0 = 4 + (24 - w) / 2 / 2 * 2 - 4; *x0 = t0 / 2; *x1 = (t0 + w + 1) / 2; *z0 = 2; *z1 = (4 + d + 1) / 2; }
static int far_solid(int wx, int wy) {   // a cell outside the plot: open street and grass, except the sea and the buildings of the lots round it
    int sx = fdiv(wx, SLC), sy = fdiv(wy, SLC); if (sx == 0 && sy == 0) return 0; if (sx < -1 || sx > 1 || sy < -1 || sy > 1) return 1;
    const BInfo *b = &nb[sy + 1][sx + 1]; if (b->terrain == BT_SEA) return 1; if (!far_house(b)) return 0;
    int lx = wx - sx * SLC, ly = wy - sy * SLC, x0, z0, x1, z1; far_foot(b, &x0, &z0, &x1, &z1); return lx >= x0 && lx < x1 && ly >= z0 && ly < z1;
}
static void gen_house(void) {   // a roomy starter home from the lot's seed: bedroom + bathroom at the back, living room + kitchen/dining at the front, real doors between, furnished with space to walk
    u32 h = lot_seed * 2654435761u + 12345; int mir = (h >> 3) & 1, w = 11 + ((h >> 5) & 1), d = 9, x0 = ((h >> 7) & 1), y0 = 1, x1 = x0 + w, y1 = y0 + d;
    int ym = y0 + 4, xm = x0 + 7, xk = x0 + 7, xd = x0 + 3; u8 ow = (u8)(1 + ((h >> 9) & 3)), iw = 4;
    #define MC(xx) (mir ? x0 + x1 - 1 - (xx) : (xx))
    #define ME(ee) (mir ? x0 + x1 - (ee) : (ee))
    #define FL(xa, ya, xb, yb, t) for (int yy = (ya); yy < (yb); yy++) for (int xx = (xa); xx < (xb); xx++) flr[yy][MC(xx)] = (u8)(t)
    FL(x0, y0, xm, ym, 4); FL(xm, y0, x1, ym, 3); FL(x0, ym, xk, y1, 1); FL(xk, ym, x1, y1, 2);
    for (int x = x0; x < x1; x++) { hwl[y0][MC(x)] = ow; hwl[y1][MC(x)] = ow; }
    for (int y = y0; y < y1; y++) { vwl[y][ME(x0)] = ow; vwl[y][ME(x1)] = ow; }
    for (int x = x0; x < x1; x++) hwl[ym][MC(x)] = iw;
    for (int y = y0; y < ym; y++) vwl[y][ME(xm)] = iw;
    for (int y = ym; y < y1; y++) if (y != ym + 2) vwl[y][ME(xk)] = 0;   // living room and kitchen share one open space
    hwl[ym][MC(x0 + 2)] = 5; hwl[ym][MC(xm + 1)] = 5;                       // bedroom door, bathroom door
    hwl[y1][MC(xd)] = 5;                                                    // the front door
    #define PUT(xx, yy, cc, ii, rr) do { objs[nobj].x = (u8)MC(xx); objs[nobj].y = (u8)(yy); objs[nobj].c = (cc); objs[nobj].i = (ii); objs[nobj].rot = (u8)(mir && ((rr) & 1) ? (rr) ^ 2 : (rr)); nobj++; } while (0)
    PUT(x0, y0, 4, 0, 0); PUT(x0 + 1, y0, 3, 0, 0); PUT(xm - 1, y0, 1, 1, 0); PUT(x0 + 3, y0 + 2, 3, 1, 0); PUT(xm - 1, ym - 1, 3, 2, 0); PUT(x0, ym - 1, 3, 3, 0);   // bed, lamp, desk+computer, rug, plant, statue
    PUT(xm, y0, 4, 2, 0); PUT(xm + 1, y0, 4, 3, 0); PUT(x1 - 1, y0, 4, 1, 0); PUT(x1 - 1, ym - 1, 3, 2, 0);                                                        // toilet, sink, bathtub, plant
    PUT(x0, ym + 2, 2, 0, 1); PUT(x0 + 3, ym + 2, 0, 2, 3); PUT(x0 + 2, ym + 2, 1, 2, 0); PUT(x0 + 2, y1 - 1, 0, 1, 2); PUT(x0, y1 - 1, 3, 2, 0); PUT(x0, ym, 3, 0, 0); PUT(x0 + 5, y1 - 1, 3, 1, 0);   // tv, sofa, coffee table, armchair, plant, lamp, rug
    PUT(xk, ym, 2, 3, 0); PUT(xk + 1, ym, 1, 3, 0); PUT(xk + 2, ym, 1, 3, 0); PUT(xk + 2, ym + 3, 1, 0, 0); PUT(xk + 1, ym + 3, 0, 0, 1); PUT(xk + 3, ym + 3, 0, 0, 3);   // fridge, counters, dining table and two chairs
    PUT(x0 + 5, y0 + 3, 2, 2, 1);                                           // a stereo
    pcx = MC(xd); pcy = y1 + 1; door_cx = MC(xd); door_cy = y1; { static const int sp[3] = { 1, 4, 5 }; for (int k = 0; k < 3; k++) { spawn_x[k] = MC(x0 + sp[k]); spawn_y[k] = y1 - 3; } }
    #undef PUT
    #undef FL
}
static void lot_reset(void) {
    memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; funds = 3500; pick = -1; mode = M_LIVE; cat = 0; tool = 0; pal = 1;
    gen_house(); nsim = 3; ctl = 0; cur_sim = 0; memset(ilim, 0, sizeof ilim); strcpy(sname[0], "Ashley Newbie"); strcpy(sname[1], "Ronald Bacon"); strcpy(sname[2], "Kit Farlow");
    for (int i = 0; i < 3; i++) { Sim *m = &sims[i]; m->x = spawn_x[i] * 16; m->y = spawn_y[i] * 16; m->tx = m->x; m->ty = m->y; m->wx = m->x; m->wy = m->y; m->act = -1; m->act_left = 0; m->age = i == 0 ? 24 : i == 1 ? 31 : 9; m->soc = -1; m->tgt = 0;
        for (int n = 0; n < 6; n++) { m->need[n] = 90 - (n * 17 + i * 11) % 60; m->acc[n] = 0; } m->rel = 0; for (int q = 0; q < PMN; q++) m->pm[q] = 50; m->cc[0] = m->cc[1] = m->cc[2] = 0; if (i) { m->skin = (u8)(i * 2); m->hair = (u8)(i); m->hcol = (u8)(i * 2); m->shirt = (u8)(i + 2); m->hair = (u8)(6 + i * 2); m->beard = (u8)(i == 1 ? 3 : 0); m->fc = (u8)i; } }
}

typedef struct { int bx, by, nobj, pcx, pcy, dcx, dcy; u8 flr[GN][GN], hwl[GN + 1][GN], vwl[GN][GN + 1]; Obj objs[400]; } LotSave;
static LotSave lsv[6]; static int lsv_n, lsv_next;
static void lot_save(void) { if (lot_bx < 0) return; int k = -1; for (int i = 0; i < lsv_n; i++) if (lsv[i].bx == lot_bx && lsv[i].by == lot_by) k = i; if (k < 0) { if (lsv_n < 6) k = lsv_n++; else { k = lsv_next; lsv_next = (lsv_next + 1) % 6; } }
    LotSave *q = &lsv[k]; q->bx = lot_bx; q->by = lot_by; q->nobj = nobj; q->pcx = pcx; q->pcy = pcy; q->dcx = door_cx; q->dcy = door_cy; memcpy(q->flr, flr, sizeof flr); memcpy(q->hwl, hwl, sizeof hwl); memcpy(q->vwl, vwl, sizeof vwl); memcpy(q->objs, objs, sizeof objs); }
static void lot_enter(int bx, int by) {   // the lot you walk into: what you built there last time, or a new home from its own seed
    lot_bx = bx; lot_by = by; lot_seed = (u32)(bx * 73856093 ^ by * 19349663); pick = -1;
    for (int i = 0; i < lsv_n; i++) if (lsv[i].bx == bx && lsv[i].by == by) { LotSave *q = &lsv[i]; nobj = q->nobj; pcx = q->pcx; pcy = q->pcy; door_cx = q->dcx; door_cy = q->dcy; memcpy(flr, q->flr, sizeof flr); memcpy(hwl, q->hwl, sizeof hwl); memcpy(vwl, q->vwl, sizeof vwl); memcpy(objs, q->objs, sizeof objs); nb_load(); return; }
    memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; gen_house(); nb_load(); }
static void lot_cross(void) {   // the sim you are playing left the lot's slot: the next lot becomes the middle one and everything slides back by one slot
    const Sim *m = &sims[ctl]; int dx = fdiv(m->x + 8, 16 * SLC), dy = fdiv(m->y + 8, 16 * SLC); if (!dx && !dy) return;
    int nx = lot_bx + dx, ny = lot_by + dy; if (nx < 0 || ny < 0 || nx >= BM_N || ny >= BM_N) return;
    lot_save(); lot_enter(nx, ny);
    for (int i = 0; i < nsim; i++) { Sim *s2 = &sims[i]; int ax = dx * SLC * 16, ay = dy * SLC * 16; s2->x -= ax; s2->y -= ay; s2->tx -= ax; s2->ty -= ay; s2->wx -= ax; s2->wy -= ay; }
    city_shift(dx, dy); camx -= dx * 36 * T; camz -= dy * 36 * T; mark_x -= dx * SLC; mark_y -= dy * SLC; }
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
static int drag, drag_card, dsx, dsy, drag_item, wall_val, move_obj = -1, ui_ver, last_tx, last_ty;
static void draw_ui(void) {
    rect(0, 0, 256, 192, C15(5, 6, 9));
    const char *tn[4] = { "Map", "Live", "Buy", "Build" };
    for (int i = 0; i < 4; i++) button(i * 64, 0, 62, 16, tn[i], C15(8, 14, 24), mode == i);
    char t[40];
    sprintf(t, "Funds %d", funds); text(4, 182, 1, C15(31, 31, 20), t);
    sprintf(t, "%d:%02d", (minutes / 60) % 24, minutes % 60); text(120, 182, 1, C15(24, 28, 31), t);
    if (msg_t > 0) text(70, 182, 1, C15(31, 20, 12), msg);
    { const char *cc[7] = { "<", ">", "^", "v", "+", "-", "Rst" }; for (int k = 0; k < 7; k++) { int cx = 164 + (k < 4 ? k * 22 : (k - 4) * 29), cy = k < 4 ? 164 : 178; rect(cx, cy, k < 4 ? 20 : 27, 12, C15(2, 4, 12)); rect(cx + 1, cy + 1, k < 4 ? 18 : 25, 10, C15(8, 16, 28)); text(cx + 4, cy + 2, 1, C15(31, 31, 31), cc[k]); } }
    if (mode == M_BUY && pick < 0) {
        for (int i = 0; i < 7; i++) button(2 + i * 36, 20, 34, 14, CATN[i], C15(24, 14, 8), cat == i);
        for (int i = 0; i < 4; i++) { const Item *it = &ITEMS[cat][i]; int x = 4 + (i & 1) * 125, y = 42 + (i >> 1) * 64;
            rect(x, y, 120, 58, drag == 1 && drag_item == i ? C15(31, 31, 20) : C15(2, 2, 4)); rect(x + 2, y + 2, 116, 54, C15(10, 12, 18)); rect(x + 8, y + 8, 34, 34, it->col); rect(x + 12, y + 12, 26, 26, shade(it->col, 6));
            text(x + 48, y + 12, 1, C15(31, 31, 31), it->name); sprintf(t, "%d", it->price); text(x + 48, y + 28, 1, funds >= it->price ? C15(31, 31, 18) : C15(31, 10, 10), t);
            { int mk = ITMASK[cat * 4 + i], lo = 0, hi = 5; while (!((mk >> lo) & 1)) lo++; while (!((mk >> hi) & 1)) hi--; sprintf(t, "%s-%s", STGN[lo], STGN[hi]); text(x + 8, y + 46, 1, C15(20, 24, 28), t); } }
    } else if (mode == M_BUY) {
        plan(1); const Item *it = &ITEMS[cat][pick];
        text(166, 26, 1, C15(31, 31, 31), it->name); sprintf(t, "%d", it->price); text(166, 38, 1, C15(31, 31, 18), t);
        button(164, 60, 88, 22, "Place", C15(10, 22, 10), 1); button(164, 90, 88, 22, "Turn", C15(10, 14, 24), 0); button(164, 120, 88, 22, "Back", C15(24, 10, 10), 0);
        text(164, 150, 1, C15(20, 24, 28), "Drag on the plan"); text(164, 160, 1, C15(20, 24, 28), "lift to drop it");
    } else if (mode == M_BUILD) {
        plan(1);
        const char *tl2[5] = { "Floor", "Wall", "Erase", "Move", "Sell" };
        for (int i = 0; i < 5; i++) button(164, 22 + i * 17, 88, 15, tl2[i], C15(24, 14, 8), tool == i);
        int n = tool == 0 ? 5 : tool == 1 ? 5 : 0;
        for (int i = 0; i < n; i++) { rect(164 + i * 17, 112, 15, 15, pal == i + 1 ? 0xFFFF : C15(2, 2, 4)); rect(166 + i * 17, 114, 11, 11, tool == 0 ? FLOORC[i] : WALLC[i]); }
        text(164, 134, 1, C15(20, 24, 28), tool == 1 ? "Drag along edges" : tool == 3 ? "Drag furniture" : tool == 4 ? "Tap furniture" : "Drag to paint");
    }
}
static int lot_touch(int tx, int ty) {   // a stylus tap on the plan: the cell (or -1)
    if (tx < PX || ty < PY || tx >= PX + GN * 12 || ty >= PY + GN * 12) return -1;
    return ((ty - PY) / 12) * GN + (tx - PX) / 12;
}
static u8 *edge_at(int tx, int ty) {   // the wall slot nearest the stylus
    int c = lot_touch(tx, ty); if (c < 0) return 0; int x = c % GN, y = c / GN, fx = (tx - PX) % 12, fy = (ty - PY) % 12, d[4] = { fy, 11 - fy, fx, 11 - fx }, b = 0;
    for (int k = 1; k < 4; k++) if (d[k] < d[b]) b = k;
    return b == 0 ? &hwl[y][x] : b == 1 ? &hwl[y + 1][x] : b == 2 ? &vwl[y][x] : &vwl[y][x + 1];
}
static void place_here(void) {
    if (obj_at(pcx, pcy) >= 0) { say("Something is there"); return; }
    if (nobj >= 400) { say("Lot is full"); return; }
    if (funds < ITEMS[cat][pick].price) { say("Not enough funds"); pick = -1; return; }
    funds -= ITEMS[cat][pick].price; objs[nobj].x = (u8)pcx; objs[nobj].y = (u8)pcy; objs[nobj].c = (u8)cat; objs[nobj].i = (u8)pick; objs[nobj].rot = (u8)prot; nobj++; say("Bought"); ui_ver++;
}
static void build_apply(int c) {   // paint or erase one cell
    int x = c % GN, y = c / GN; pcx = x; pcy = y;
    if (tool == 0 && flr[y][x] != pal) { flr[y][x] = (u8)pal; ui_ver++; }
    else if (tool == 2) { flr[y][x] = 0; hwl[y][x] = 0; hwl[y + 1][x] = 0; vwl[y][x] = 0; vwl[y][x + 1] = 0; ui_ver++; }
}
static void ui_down(int tx, int ty) {
    last_tx = tx; last_ty = ty; drag = 0; drag_card = 0;
    for (int i = 0; i < 4; i++) if (hit(i * 64, 0, 62, 16, tx, ty)) { set_mode(i); pick = -1; return; }
    if (mode == M_BUY && pick < 0) {
        for (int i = 0; i < 7; i++) if (hit(2 + i * 36, 20, 34, 14, tx, ty)) cat = i;
        for (int i = 0; i < 4; i++) if (hit(4 + (i & 1) * 125, 42 + (i >> 1) * 64, 120, 58, tx, ty)) { if (funds >= ITEMS[cat][i].price) { drag = 1; drag_item = i; dsx = tx; dsy = ty; } else say("Not enough funds"); }
    } else if (mode == M_BUY) {
        int c = lot_touch(tx, ty);
        if (c >= 0) { pcx = c % GN; pcy = c / GN; drag = 2; return; }
        if (hit(164, 60, 88, 22, tx, ty)) place_here();
        if (hit(164, 90, 88, 22, tx, ty)) prot = (prot + 1) & 3;
        if (hit(164, 120, 88, 22, tx, ty)) pick = -1;
    } else if (mode == M_BUILD) {
        for (int i = 0; i < 5; i++) if (hit(164, 22 + i * 17, 88, 15, tx, ty)) { tool = i; if (pal < 1) pal = 1; if (tool == 1 && pal > 5) pal = 5; }
        int n = tool == 0 ? 5 : tool == 1 ? 5 : 0;
        for (int i = 0; i < n; i++) if (hit(164 + i * 17, 112, 15, 15, tx, ty)) pal = i + 1;
        int c = lot_touch(tx, ty);
        if (c >= 0) { int x = c % GN, y = c / GN; pcx = x; pcy = y;
            if (tool == 0 || tool == 2) { drag = 3; build_apply(c); }
            else if (tool == 1) { u8 *e = edge_at(tx, ty); if (e) { wall_val = *e ? 0 : pal; *e = (u8)wall_val; drag = 4; ui_ver++; } }
            else if (tool == 3) { int o = obj_at(x, y); if (o >= 0) { drag = 5; move_obj = o; } }
            else if (tool == 4) { int o = obj_at(x, y); if (o >= 0) { funds += ITEMS[objs[o].c][objs[o].i].price / 2; objs[o] = objs[--nobj]; say("Sold"); ui_ver++; } } }
    }
}
static void ui_hold(int tx, int ty) {
    last_tx = tx; last_ty = ty;
    if (drag == 1) { int dx = tx - dsx, dy = ty - dsy; if (dx < 0) dx = -dx; if (dy < 0) dy = -dy; if (dx + dy > 6) { pick = drag_item; prot = 0; drag = 2; drag_card = 1; ui_ver++; } }
    else if (drag == 2) { int c = lot_touch(tx, ty); if (c >= 0 && (c % GN != pcx || c / GN != pcy)) { pcx = c % GN; pcy = c / GN; } }
    else if (drag == 3) { int c = lot_touch(tx, ty); if (c >= 0) build_apply(c); }
    else if (drag == 4) { u8 *e = edge_at(tx, ty); if (e && *e != wall_val) { *e = (u8)wall_val; ui_ver++; } }
    else if (drag == 5) { int c = lot_touch(tx, ty); if (c >= 0 && move_obj >= 0 && obj_at(c % GN, c / GN) < 0) { objs[move_obj].x = (u8)(c % GN); objs[move_obj].y = (u8)(c / GN); pcx = c % GN; pcy = c / GN; ui_ver++; } }
}
static void ui_up(void) {
    if (drag == 1) { pick = drag_item; prot = 0; ui_ver++; }                                   // a tap on a card: pick it, then drag on the plan
    else if (drag == 2) { if (lot_touch(last_tx, last_ty) >= 0) place_here(); else if (drag_card) { pick = -1; ui_ver++; } }
    drag = 0; drag_card = 0; move_obj = -1;
}
static int ob_x, ob_z, ob_s, ob_rot, ob_hs = 100;
static const u8 HSC[28] = { 140, 140, 140, 140, 200, 200, 200, 200, 200, 200, 180, 200, 200, 100, 170, 200, 150, 200, 170, 200, 150, 100, 200, 150, 100, 200, 90, 100 };   // the cell being drawn: its corner, its size / 8, its rotation
static void ob(int lx0, int lz0, int lx1, int lz1, int h0, int h1, u16 c) {   // a part in cell units (0..8, front = +z), heights in quarter-tiles
    int ax, az, bx2, bz;
    switch (ob_rot & 3) { case 1: ax = lz0; az = 80 - lx0; bx2 = lz1; bz = 80 - lx1; break; case 2: ax = 80 - lx0; az = 80 - lz0; bx2 = 80 - lx1; bz = 80 - lz1; break; case 3: ax = 80 - lz0; az = lx0; bx2 = 80 - lz1; bz = lx1; break; default: ax = lx0; az = lz0; bx2 = lx1; bz = lz1; }
    int X0 = ob_x + (ax < bx2 ? ax : bx2) * ob_s / 10, X1 = ob_x + (ax < bx2 ? bx2 : ax) * ob_s / 10, Z0 = ob_z + (az < bz ? az : bz) * ob_s / 10, Z1 = ob_z + (az < bz ? bz : az) * ob_s / 10;
    int hq = T / 4; block(X0, Z0, X1, Z1, -40 - h0 * hq / 10 * ob_hs / 100, -40 - h1 * hq / 10 * ob_hs / 100, c, shade(c, 10));
}
#define LC(r, g, b) C15(r, g, b)
static void obj3d(int x, int y, const Item *it, int rot, int ghost) {   // every item is a little model built from several boxes (coordinates 0..80 across the cell, heights in tenths of a quarter-tile)
    (void)ghost; int id = (int)(it - &ITEMS[0][0]); u16 c = it->col, d = shade(c, 6), l = shade(c, 10), wd = LC(16, 10, 5), dk = LC(3, 3, 4);
    ob_x = tl(x * 2) + T / 6; ob_z = tl(y * 2) + T / 6; ob_s = (2 * T - T / 3) / 8; ob_rot = rot; ob_hs = id >= 0 && id < 28 ? HSC[id] : 100;
    if (!o_det || (mode == M_LIVE && (abs(ob_x - camx) + abs(ob_z - camz) > 3800))) { if (it->kind == 1) ob(0, 0, 80, 80, 0, 3, c); else ob(8, 8, 72, 72, 0, it->h * 10 + 6, c); return; }
    if (it->kind == 1) { ob(0, 0, 80, 80, 0, 2, d); ob(6, 6, 74, 74, 0, 4, c); ob(14, 14, 66, 66, 0, 5, l); return; }   // rug: border and centre
    if (o_shd) { u16 sh = LC(6, 5, 4); ob_x -= 6; ob_z -= 4; ob(0, 0, 80, 80, -3, 0, sh); ob_x += 6; ob_z += 4; }   // a soft dark shadow on the floor
    switch (id) {
    case 0: for (int k = 0; k < 4; k++) ob(k % 2 ? 62 : 8, k / 2 ? 62 : 8, k % 2 ? 72 : 18, k / 2 ? 72 : 18, 0, 20, d); ob(6, 6, 74, 74, 20, 26, c); ob(6, 6, 74, 18, 26, 62, l); break;   // chair
    case 1: ob(0, 0, 80, 80, 0, 12, d); ob(8, 20, 72, 80, 12, 24, l); ob(0, 0, 80, 20, 12, 50, c); ob(0, 20, 12, 80, 12, 36, c); ob(68, 20, 80, 80, 12, 36, c); break;   // armchair
    case 2: ob(0, 4, 80, 80, 0, 12, d); ob(4, 4, 76, 24, 12, 48, c); ob(4, 24, 40, 76, 12, 26, l); ob(40, 24, 76, 76, 12, 26, l); ob(0, 24, 10, 80, 12, 38, c); ob(70, 24, 80, 80, 12, 38, c); break;   // sofa
    case 3: ob(0, 20, 80, 60, 20, 26, wd); ob(2, 22, 10, 30, 0, 20, d); ob(70, 22, 78, 30, 0, 20, d); ob(2, 50, 10, 58, 0, 20, d); ob(70, 50, 78, 58, 0, 20, d); break;   // bench
    case 4: ob(4, 4, 76, 76, 28, 34, c); for (int k = 0; k < 4; k++) ob(k % 2 ? 62 : 10, k / 2 ? 62 : 10, k % 2 ? 70 : 18, k / 2 ? 70 : 18, 0, 28, d); ob(30, 30, 50, 50, 34, 38, LC(30, 30, 29)); ob(34, 34, 46, 46, 38, 40, LC(31, 22, 8)); break;   // dining table with a plate
    case 5: ob(0, 8, 80, 72, 26, 32, c); ob(0, 8, 18, 72, 0, 26, d); ob(62, 8, 80, 72, 0, 26, d); ob(2, 10, 16, 40, 10, 18, dk); ob(46, 14, 66, 30, 32, 34, LC(28, 28, 22)); break;   // desk with drawers and paper
    case 6: ob(6, 14, 74, 66, 12, 17, c); for (int k = 0; k < 4; k++) ob(k % 2 ? 62 : 8, k / 2 ? 56 : 18, k % 2 ? 72 : 18, k / 2 ? 64 : 26, 0, 12, d); ob(30, 30, 50, 46, 17, 24, LC(8, 22, 10)); break;   // coffee table and a plant bowl
    case 7: ob(0, 8, 80, 72, 0, 28, d); ob(0, 4, 80, 76, 28, 33, l); ob(8, 72, 36, 74, 6, 24, c); ob(44, 72, 72, 74, 6, 24, c); ob(16, 4, 28, 16, 33, 40, LC(20, 20, 22)); break;   // kitchen counter
    case 8: ob(4, 30, 76, 54, 0, 14, wd); ob(8, 30, 72, 52, 14, 50, dk); ob(10, 52, 70, 54, 16, 48, LC(8, 16, 24)); ob(24, 54, 56, 56, 6, 10, LC(10, 10, 12)); break;   // tv on a stand
    case 9: ob(4, 4, 76, 40, 0, 20, d); ob(6, 4, 74, 40, 20, 24, c); ob(20, 6, 60, 12, 24, 52, dk); ob(22, 12, 58, 14, 26, 50, LC(6, 16, 28)); ob(16, 48, 64, 70, 20, 23, LC(14, 14, 16)); break;   // computer: desk, monitor, keyboard
    case 10: ob(4, 14, 76, 66, 0, 22, c); ob(10, 66, 36, 68, 4, 20, LC(10, 10, 12)); ob(44, 66, 70, 68, 4, 20, LC(10, 10, 12)); ob(14, 70, 32, 72, 8, 16, LC(16, 20, 28)); ob(50, 70, 66, 72, 8, 16, LC(16, 20, 28)); ob(8, 14, 72, 22, 22, 25, l); break;   // stereo with two speakers
    case 11: ob(8, 8, 72, 72, 0, 70, c); ob(8, 72, 72, 74, 0, 28, shade(c, 7)); ob(8, 72, 72, 74, 30, 66, shade(c, 8)); ob(60, 74, 64, 76, 34, 56, dk); ob(60, 74, 64, 76, 8, 24, dk); ob(8, 8, 72, 72, 70, 72, l); break;   // fridge with two doors and handles
    case 12: ob(24, 24, 56, 56, 0, 3, d); ob(36, 36, 44, 44, 3, 36, LC(18, 16, 8)); ob(16, 16, 64, 64, 36, 56, c); ob(22, 22, 58, 58, 56, 58, LC(31, 31, 24)); break;   // lamp
    case 14: ob(24, 24, 56, 56, 0, 16, LC(16, 9, 5)); ob(18, 18, 62, 62, 16, 36, c); ob(26, 26, 54, 54, 36, 48, l); ob(34, 34, 46, 46, 48, 54, shade(c, 12)); break;   // plant
    case 15: ob(10, 10, 70, 70, 0, 14, LC(20, 20, 19)); ob(14, 14, 66, 66, 14, 18, LC(24, 24, 22)); ob(28, 28, 52, 52, 18, 46, c); ob(24, 28, 28, 36, 30, 40, c); ob(52, 28, 56, 36, 30, 40, c); ob(30, 30, 50, 50, 46, 58, l); break;   // statue on a pedestal
    case 16: ob(0, 0, 80, 80, 0, 10, wd); ob(2, 14, 78, 78, 10, 20, c); ob(8, 6, 72, 30, 20, 26, LC(31, 31, 31)); ob(0, 0, 80, 6, 0, 40, wd); break;   // bed with headboard and pillow
    case 17: ob(0, 4, 80, 76, 0, 22, c); ob(8, 12, 72, 68, 22, 24, LC(8, 20, 30)); ob(34, 0, 46, 6, 22, 34, LC(20, 20, 22)); break;   // bathtub with water
    case 18: ob(26, 34, 54, 76, 0, 14, c); ob(24, 32, 56, 78, 14, 17, l); ob(26, 6, 54, 30, 0, 32, c); ob(30, 8, 50, 12, 32, 36, LC(20, 20, 22)); break;   // toilet
    case 20: ob(4, 4, 76, 76, 3, 8, wd); ob(8, 8, 72, 72, 8, 12, LC(28, 28, 31)); ob(4, 4, 76, 10, 8, 34, c); ob(4, 70, 76, 76, 8, 34, c); ob(4, 4, 10, 76, 8, 34, c); ob(70, 4, 76, 76, 8, 34, c); ob(4, 4, 12, 12, 0, 40, d); ob(68, 4, 76, 12, 0, 40, d); ob(4, 68, 12, 76, 0, 40, d); ob(68, 68, 76, 76, 0, 40, d); break;   // a crib
    case 21: ob(10, 20, 70, 70, 0, 14, c); ob(8, 18, 72, 72, 14, 17, l); ob(20, 30, 34, 44, 17, 27, LC(31, 8, 8)); ob(40, 40, 54, 54, 17, 25, LC(8, 16, 31)); ob(44, 24, 58, 38, 17, 22, LC(8, 26, 8)); break;   // a toy box with blocks
    case 22: ob(6, 44, 74, 76, 0, 58, c); for (int r = 0; r < 3; r++) for (int k = 0; k < 5; k++) ob(10 + k * 12, 66, 20 + k * 12, 76, 6 + r * 17, 19 + r * 17, (r + k) & 1 ? LC(28, 8, 8) : (r + k) % 3 == 0 ? LC(8, 14, 28) : LC(10, 24, 10)); break;   // a bookshelf
    case 23: ob(12, 10, 68, 18, 0, 3, wd); ob(12, 62, 68, 70, 0, 3, wd); ob(18, 20, 62, 60, 8, 14, c); ob(18, 56, 62, 66, 14, 44, c); ob(14, 20, 22, 60, 14, 26, d); ob(58, 20, 66, 60, 14, 26, d); break;   // a rocking chair
    case 24: for (int k = 0; k < 4; k++) ob(k * 24, 34, k * 24 + 8, 46, 0, 26, c); ob(0, 36, 80, 44, 8, 12, d); ob(0, 36, 80, 44, 17, 21, d); break;   // a picket fence along the cell: posts and two rails (turn it to run the other way)
    case 25: ob(34, 34, 46, 46, 0, 4, d); ob(38, 38, 42, 42, 4, 72, c); ob(30, 30, 50, 50, 72, 80, LC(31, 28, 14)); break;   // a lamp post
    case 26: ob(0, 20, 80, 60, 0, 22, c); ob(4, 24, 76, 56, 22, 26, l); break;   // a hedge
    case 27: ob(0, 0, 80, 80, 0, 2, LC(8, 18, 6)); for (int k = 0; k < 5; k++) ob(8 + (k % 3) * 24, 8 + (k / 3) * 36 + (k & 1) * 6, 20 + (k % 3) * 24, 20 + (k / 3) * 36 + (k & 1) * 6, 2, 8, k & 1 ? c : LC(31, 28, 8)); break;   // a flower bed
    case 19: ob(32, 24, 48, 48, 0, 20, d); ob(10, 14, 70, 56, 20, 26, c); ob(18, 20, 62, 50, 26, 27, LC(8, 20, 30)); ob(36, 8, 44, 16, 26, 38, LC(20, 20, 22)); break;   // sink on a pedestal
    default: ob(4, 4, 76, 76, 0, it->h * 10, c); break; }
}
static int door_open;
static void wall_seg(int x0, int z0, int x1, int z1, int v, int vert, int wh) {   // one wall piece (or a door) on a cell edge; vert: it runs along z
    u16 c = WALLC[v - 1], cap = shade(c, 10), base = shade(c, 6), frame = C15(11, 7, 4); int th = T / 16, ps = T / 6;
    if (v == 5) {   // a door: frame, lintel, and a solid leaf that is shut, or swung open while a sim is at it
        u16 lf = C15(17, 11, 6), gold = C15(30, 26, 8); int lin = T * 7 / 2, lh2 = wh < lin ? wh : lin, hi = door_open;
        if (!vert) {
            block(x0, z0, x0 + ps, z1, -40, -wh, frame, shade(frame, 10)); block(x1 - ps, z0, x1, z1, -40, -wh, frame, shade(frame, 10));
            if (wh > lin + T / 2) block(x0 + ps, z0, x1 - ps, z1, -lin, -wh, c, cap); block(x0 + ps, z0 + 2, x1 - ps, z1 - 2, -40, -45, shade(frame, 7), shade(frame, 9));
            if (!hi) { block(x0 + ps, z0 + 4, x1 - ps, z1 - 4, -40, -lh2, lf, shade(lf, 10)); block(x0 + ps + 40, z0 - 4, x1 - ps - 40, z0, -T / 3, -lh2 + T / 3, shade(lf, 7), shade(lf, 8)); block(x0 + ps + 40, z1, x1 - ps - 40, z1 + 4, -T / 3, -lh2 + T / 3, shade(lf, 7), shade(lf, 8));
                block(x1 - ps - 70, z1, x1 - ps - 40, z1 + 16, -T * 3 / 2, -T * 3 / 2 - 34, gold, gold); block(x1 - ps - 70, z0 - 16, x1 - ps - 40, z0, -T * 3 / 2, -T * 3 / 2 - 34, gold, gold); }
            else { block(x0 + ps, z0 - T * 9 / 10, x0 + ps + th, z0, -40, -lh2, lf, shade(lf, 10)); block(x0 + ps + th, z0 - T * 3 / 4, x0 + ps + th + 16, z0 - T * 3 / 4 + 30, -T * 3 / 2, -T * 3 / 2 - 34, gold, gold); }
        } else {
            block(x0, z0, x1, z0 + ps, -40, -wh, frame, shade(frame, 10)); block(x0, z1 - ps, x1, z1, -40, -wh, frame, shade(frame, 10));
            if (wh > lin + T / 2) block(x0, z0 + ps, x1, z1 - ps, -lin, -wh, c, cap); block(x0 + 2, z0 + ps, x1 - 2, z1 - ps, -40, -45, shade(frame, 7), shade(frame, 9));
            if (!hi) { block(x0 + 4, z0 + ps, x1 - 4, z1 - ps, -40, -lh2, lf, shade(lf, 10)); block(x0 - 4, z0 + ps + 40, x0, z1 - ps - 40, -T / 3, -lh2 + T / 3, shade(lf, 7), shade(lf, 8)); block(x1, z0 + ps + 40, x1 + 4, z1 - ps - 40, -T / 3, -lh2 + T / 3, shade(lf, 7), shade(lf, 8));
                block(x1, z1 - ps - 70, x1 + 16, z1 - ps - 40, -T * 3 / 2, -T * 3 / 2 - 34, gold, gold); block(x0 - 16, z1 - ps - 70, x0, z1 - ps - 40, -T * 3 / 2, -T * 3 / 2 - 34, gold, gold); }
            else { block(x0 - T * 9 / 10, z0 + ps, x0, z0 + ps + th, -40, -lh2, lf, shade(lf, 10)); }
        }
        return; }
    if (o_det) block(x0 - 3, z0 - 3, x1 + 3, z1 + 3, -40, -40 - T / 7, base, shade(base, 9));                       // skirting board
    block(x0, z0, x1, z1, -40 - T / 7, -wh, c, cap);                                                       // the wall, with a pale cap
    (void)vert; (void)th;
}


// ---------- the living city (D18): citizens stroll the pavements, cars drive the roads, everyone follows the clock ----------
#define NCIT 12
#define NCAR 8
typedef struct { int x, y, dir, stop; u8 on, stage, skin, shirt, hair, pants; u16 id; } Cit;
typedef struct { u16 id; u8 rel, talks, taken; } CRel;
typedef struct { int x, y, dx, dy; u16 col; } Car;
static Cit cits[NCIT]; static CRel crt[64]; static Car cars[NCAR];
static const char *const FNAMES[12] = { "Mia", "Leo", "Zoe", "Max", "Ivy", "Sam", "Ana", "Kai", "Eli", "Ola", "Dex", "Una" };
static const char *const LNAMES[8] = { "Park", "Reyes", "Ford", "Nash", "Cole", "Vega", "Hart", "Lund" };
static u32 hsh(u32 a) { a ^= a >> 16; a *= 0x7feb352dU; a ^= a >> 15; a *= 0x846ca68bU; a ^= a >> 16; return a; }
static CRel *crel(u16 id) { int h = id & 63; for (int k = 0; k < 64; k++) { CRel *c = &crt[(h + k) & 63]; if (c->id == id) return c; if (!c->id) { c->id = id; return c; } } return &crt[h]; }
static void cit_name(const Cit *c, char *o) { sprintf(o, "%s %s", FNAMES[c->id % 12], LNAMES[(c->id / 12) % 8]); }
static int cit_street(int x, int y) { int lx = ((x % SLC) + SLC) % SLC, ly = ((y % SLC) + SLC) % SLC; return (lx >= 13 || ly >= 13) && !far_solid(x, y); }
static int cit_vis(const Cit *c) { int hr = (minutes / 60) % 24; return c->on && hr >= 6 && hr < 23 && ((hr * 5 + c->id) % 7) < 5; }   // asleep indoors at night, out and about by day
static void city_spawn(void) {
    int sl[9], ns = 0; for (int j = 0; j < 9; j++) if (nb[j / 3][j % 3].set != 255 && nb[j / 3][j % 3].terrain != BT_SEA) sl[ns++] = j;
    for (int k = 0; k < NCIT; k++) { Cit *c = &cits[k]; c->on = 0; c->stop = 0; if (!ns) continue; int j = (k < 6 && nb[1][1].set != 255) ? 4 : sl[k % ns], sy = j / 3 - 1, sx = j % 3 - 1;
        u32 h = hsh((u32)(lot_bx + sx) * 73856093u ^ (u32)(lot_by + sy) * 19349663u ^ (u32)k * 83492791u); c->id = (u16)((h >> 8) | 1); if (crel(c->id)->taken) continue;
        int r = (int)(h % 100); c->stage = (u8)(r < 16 ? 2 : r < 30 ? 3 : r < 86 ? 4 : 5);
        int lx = 13 + (int)((h >> 8) % 5), ly = (int)((h >> 12) % 18); if ((h >> 20) & 1) { int t = lx; lx = ly; ly = t; }
        int cx = sx * SLC + lx, cy = sy * SLC + ly; if (far_solid(cx, cy)) continue;
        c->x = cx * 16; c->y = cy * 16; c->dir = (int)((h >> 24) & 3); c->skin = (u8)((h >> 5) % 6); c->shirt = (u8)((h >> 9) % 6); c->hair = (u8)((h >> 13) % 6); c->pants = (u8)((h >> 17) % 4); c->on = 1; }
    for (int k = 0; k < NCAR; k++) { Car *c = &cars[k]; if (!ns) { c->x = c->y = -9999; c->dx = c->dy = 0; continue; } u32 h = hsh((u32)k * 977u + (u32)lot_bx * 31u + (u32)lot_by * 7u); int j = (int)((k >> 1) % 3) - 1, hz = k & 1, fw = (int)((k >> 2) & 1); int at = (int)(h % (SLC * 3 * 16)) - SLC * 16;
        if (hz) { c->y = (fw ? 16 : 14) * 16 + j * SLC * 16; c->x = at; c->dx = fw ? 1 : -1; c->dy = 0; } else { c->x = (fw ? 16 : 14) * 16 + j * SLC * 16; c->y = at; c->dx = 0; c->dy = fw ? 1 : -1; }
        { static const u16 cc[6] = { C15(28, 6, 6), C15(8, 12, 28), C15(28, 26, 6), C15(10, 24, 10), C15(26, 26, 28), C15(4, 4, 6) }; c->col = cc[h % 6]; } }
}
static void city_step(int frames) {
    static const int DXs[4] = { 1, 0, -1, 0 }, DYs[4] = { 0, 1, 0, -1 };
    for (int k = 0; k < NCIT; k++) { Cit *c = &cits[k]; if (!c->on) continue; if (c->stop > 0) { c->stop--; continue; } if (!cit_vis(c) || ((frames + k) & 1)) continue;
        if (!(c->x & 15) && !(c->y & 15)) { int cx = c->x / 16, cy = c->y / 16, d = c->dir; if (hsh((u32)(frames * 31 + k)) % 100 < 30) d = (d + (hsh((u32)frames + (u32)k) & 1 ? 1 : 3)) & 3;
            for (int t = 0; t < 4; t++) { int dd = (d + t) & 3; if (cit_street(cx + DXs[dd], cy + DYs[dd])) { c->dir = dd; break; } } }
        { int nx = c->x + DXs[c->dir], ny = c->y + DYs[c->dir]; if (!(c->x & 15) && !(c->y & 15) && !cit_street(c->x / 16 + DXs[c->dir], c->y / 16 + DYs[c->dir])) continue; c->x = nx; c->y = ny; } }
    for (int k = 0; k < NCAR; k++) { Car *c = &cars[k]; c->x += c->dx; c->y += c->dy; int lo = -SLC * 16, hi = SLC * 2 * 16; if (c->x < lo) c->x = hi - 1; if (c->x >= hi) c->x = lo; if (c->y < lo) c->y = hi - 1; if (c->y >= hi) c->y = lo; }
}
static void city_shift(int dx, int dy) {
    for (int k = 0; k < NCIT; k++) { cits[k].x -= dx * SLC * 16; cits[k].y -= dy * SLC * 16; }
    for (int k = 0; k < NCAR; k++) { cars[k].x -= dx * SLC * 16; cars[k].y -= dy * SLC * 16; int w = SLC * 3 * 16, lo = -SLC * 16; if (cars[k].x < lo) cars[k].x += w; if (cars[k].x >= lo + w) cars[k].x -= w; if (cars[k].y < lo) cars[k].y += w; if (cars[k].y >= lo + w) cars[k].y -= w; }
}
static void draw_city(void) {
    static const u16 SK[6] = { C15(30, 24, 20), C15(27, 20, 15), C15(22, 15, 10), C15(16, 10, 6), C15(11, 7, 4), C15(29, 22, 17) };
    static const u16 SH[6] = { C15(26, 26, 26), C15(24, 8, 8), C15(8, 14, 26), C15(8, 22, 10), C15(28, 22, 8), C15(16, 8, 22) };
    static const u16 HR[6] = { C15(3, 3, 4), C15(14, 8, 4), C15(26, 20, 8), C15(24, 8, 4), C15(25, 25, 25), C15(10, 14, 28) };
    for (int k = 0; k < NCIT; k++) { const Cit *c = &cits[k]; if (!cit_vis(c) || polys > 2800) continue; int sc = STSC[c->stage], x = c->x * 2 * T / 16 - GN * T + T, z = c->y * 2 * T / 16 - GN * T + T, sw = (((c->x + c->y) >> 3) & 1) ? 30 : -30;
        u16 pn = c->pants == 0 ? C15(6, 7, 12) : c->pants == 1 ? C15(10, 10, 14) : c->pants == 2 ? C15(14, 10, 6) : C15(8, 12, 8);
        block(x - 70 * sc / 100, z - 40 * sc / 100 + sw, x + 70 * sc / 100, z + 40 * sc / 100 + sw, 0, -300 * sc / 100, pn, pn);
        block(x - 90 * sc / 100, z - 50 * sc / 100, x + 90 * sc / 100, z + 50 * sc / 100, -300 * sc / 100, -590 * sc / 100, SH[c->shirt], SH[c->shirt]);
        block(x - 60 * sc / 100, z - 60 * sc / 100, x + 60 * sc / 100, z + 60 * sc / 100, -590 * sc / 100, -800 * sc / 100, SK[c->skin], c->stage == 5 ? C15(25, 25, 26) : HR[c->hair]); }
    for (int k = 0; k < NCAR; k++) { const Car *c = &cars[k]; if (polys > 2950) break; int x = c->x * 2 * T / 16 - GN * T + T, z = c->y * 2 * T / 16 - GN * T + T, lx = c->dx ? 300 : 150, lz = c->dx ? 150 : 300, cx2 = c->dx ? 130 : 130, cz = c->dx ? 130 : 130;
        block(x - lx, z - lz, x + lx, z + lz, -20, -150, c->col, shade(c->col, 10)); block(x - lx + (c->dx ? 90 : 0) + (c->dx ? 0 : 0) - (c->dx ? 0 : 0), z - lz + (c->dx ? 0 : 90), x + lx - (c->dx ? 90 : 0), z + lz - (c->dx ? 0 : 90), -150, -270, C15(20, 26, 31), C15(24, 24, 26)); (void)cx2; (void)cz; }
}
static void far_lot(const BInfo *b, int ox, int oz, int pop, int yaw) {   // a neighbour's lot: flat ground, a plain house or a few trees (cheap: the polygon budget is the DS's limit)
    u32 h = b->h; int k = b->lot;
    u16 grass = b->terrain == BT_DESERT ? C15(24, 20, 10) : b->terrain == BT_ROCKY ? C15(13, 13, 13) : b->terrain == BT_BEACH ? C15(26, 24, 14) : C15(11, 20, 8);
    ground(ox + tl(0), oz + tl(0), ox + tl(26), oz + tl(26), -2, grass);
    if (!far_house(b)) { int n = (k == LOT_PARK || k == LOT_SQUARE) ? 4 : 2; for (int i = 0; i < n; i++) tree(ox + tl(3 + (int)((h >> (i * 4)) & 15) * 3 / 2 % 20), oz + tl(3 + (int)((h >> (i * 4 + 2)) & 15) * 3 / 2 % 20), T * 2); return; }
    int w = 10 + (int)(h & 3) * 2, d = 8 + (int)((h >> 2) & 3) * 2, x0 = 4 + (24 - w) / 2 / 2 * 2 - 4, z0 = 4;
    int st = bm_storeys(b); int hh = 2 * st + (st == 1 ? 2 : 0);
    u16 walls[6] = { C15(26, 22, 16), C15(24, 14, 10), C15(22, 24, 26), C15(26, 26, 18), C15(16, 22, 14), C15(24, 20, 22) }; u16 wall = walls[(h >> 8) % 6];
    if (k >= LOT_SHOP) wall = k == LOT_TOWER || k == LOT_OFFICE || k == LOT_HOTEL ? C15(16, 20, 26) : C15(27, 12, 10);
    int X0 = ox + tl(x0), X1 = ox + tl(x0 + w), Z0 = oz + tl(z0), Z1 = oz + tl(z0 + d), Y = -(hh * T);
    block(X0, Z0, X1, Z1, 0, Y, wall, wall);
    { int wn = (X1 - X0) / 5; block(X0 + wn * 2, Z1, X0 + wn * 2 + wn * 7 / 10, Z1 + 14, 0, -T * 3 / 2, C15(12, 7, 4), C15(12, 7, 4)); block(X0 + wn, Z1, X0 + wn + wn * 7 / 10, Z1 + 14, -T, -T * 2, C15(17, 24, 31), C15(17, 24, 31)); block(X0 + wn * 3, Z1, X0 + wn * 3 + wn * 7 / 10, Z1 + 14, -T, -T * 2, C15(17, 24, 31), C15(17, 24, 31)); }
    if (k <= LOT_APART) gable(X0, Z0, X1, Z1, Y, -T * 2, C15(18, 8, 6)); else ground(X0, Z0, X1, Z1, Y - 24, C15(14, 14, 15));
    for (int i = 0; i < pop && i < 3; i++) person(ox + tl(4 + i * 3), oz + tl(z0 + d + 3), C15(6 + i * 4, 10, 25 - i * 3), yaw);   // the bores who live there
}
static void draw_world(int yaw) {   // LIVE: all round the lot, the lots next door, the streets and the land (a 3x3 patch of slots)
    ground(tl(-40), tl(-40), tl(76), tl(76), 90, C15(7, 16, 6));
    for (int j = -1; j <= 1; j++) { int o = 36 * j;   // roads both ways
        ground(tl(-36), tl(28 + o), tl(72), tl(34 + o), 0, C15(7, 7, 9)); ground(tl(28 + o), tl(-36), tl(34 + o), tl(72), 0, C15(7, 7, 9)); }
    for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) { int ox = 36 * i, oz = 36 * j;   // pavements in the stretches between the roads
        ground(tl(ox), tl(oz + 26), tl(ox + 26), tl(oz + 28), -4, C15(22, 22, 22)); ground(tl(ox), tl(oz + 34), tl(ox + 26), tl(oz + 36), -4, C15(22, 22, 22));
        ground(tl(ox + 26), tl(oz + 26), tl(ox + 28), tl(oz + 28), -4, C15(22, 22, 22)); ground(tl(ox + 34), tl(oz + 26), tl(ox + 36), tl(oz + 28), -4, C15(22, 22, 22)); ground(tl(ox + 26), tl(oz + 34), tl(ox + 28), tl(oz + 36), -4, C15(22, 22, 22)); ground(tl(ox + 34), tl(oz + 34), tl(ox + 36), tl(oz + 36), -4, C15(22, 22, 22));
        ground(tl(ox + 26), tl(oz), tl(ox + 28), tl(oz + 26), -4, C15(22, 22, 22)); ground(tl(ox + 34), tl(oz), tl(ox + 36), tl(oz + 26), -4, C15(22, 22, 22)); }
    for (int x = 0; x < 36; x += 6) { ground(tl(x), tl(31) - 14, tl(x) + T * 2, tl(31) + 14, -6, C15(28, 26, 8)); ground(tl(31) - 14, tl(x), tl(31) + 14, tl(x) + T * 2, -6, C15(28, 26, 8)); }   // lane dashes by this lot
}
static void draw_far_all(int yaw) {   // last, so the lot, the sims and the citizens always get the polygons first
    for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) { if (!i && !j) continue; const BInfo *b = &nb[j + 1][i + 1];
        if (b->terrain == BT_SEA) { ground(tl(36 * i), tl(36 * j), tl(36 * i + 36), tl(36 * j + 36), -8, C15(5, 12, 24)); continue; }
        if (polys > 2700) continue; far_lot(b, 36 * i * T, 36 * j * T, nb_pop[j + 1][i + 1], yaw); }
}
static void draw_build(int yaw) {
    ground(tl(0), tl(0), tl(GN * 2), tl(GN * 2), 0, C15(11, 20, 8));   // the lawn: one quad and big darker squares (every polygon counts on the DS)
    if (o_chk) for (int z = 0; z < GN; z += 2) for (int x = 0; x < GN; x += 2) if (((x + z) / 2) & 1) { int e = x + 2 > GN ? GN : x + 2, f = z + 2 > GN ? GN : z + 2; ground(tl(x * 2), tl(z * 2), tl(e * 2), tl(f * 2), -2, C15(9, 18, 7)); }
    if (mode == M_LIVE) draw_world(yaw); else {
    ground(tl(-14), tl(-9), tl(40), tl(-2), 0, C15(7, 7, 9)); ground(tl(-14), tl(28), tl(40), tl(35), 0, C15(7, 7, 9));   // two proper streets, three cells wide
    block(tl(-14), tl(-2), tl(40), tl(0), 0, -14, C15(20, 20, 20), C15(24, 24, 24)); block(tl(-14), tl(26), tl(40), tl(28), 0, -14, C15(20, 20, 20), C15(24, 24, 24));   // pavements
    for (int x = -13; x < 40; x += 4) { block(tl(x), tl(-6) - 14, tl(x) + T * 2, tl(-6) + 14, 0, -6, C15(28, 26, 8), C15(28, 26, 8)); block(tl(x), tl(31) - 14, tl(x) + T * 2, tl(31) + 14, 0, -6, C15(28, 26, 8), C15(28, 26, 8)); }   // lane dashes
    ground(tl(-14), tl(-9), tl(40), tl(35), 90, C15(7, 16, 6));   // grass all round, so the lot does not float
    for (int i = 0; i < 5; i++) { tree(tl(-6 + (i * 7) % 5 * 1), tl(2 + i * 5), T * 2); tree(tl(GN * 2 + 3 - (i * 5) % 4), tl(1 + i * 5), T * 2); }
    }
    ground(tl(0), tl(26) - 30, tl(GN * 2), tl(26), -3, C15(14, 24, 10));   // a low hedge line along the back of the plot (no more posts sticking out)
    block(tl(door_cx * 2) + T / 2, tl(door_cy * 2 + 1), tl(door_cx * 2 + 2) - T / 2, tl(26), 0, -26, C15(19, 18, 16), C15(23, 22, 20));   // a path from the front door to the pavement
    { static u8 dn[GN][GN]; memset(dn, 0, sizeof dn);   // floors as the biggest same-type rectangles: no seams for the edge anti-aliasing to show the grass through
      for (int y = 0; y < GN; y++) for (int x = 0; x < GN; x++) { int t = flr[y][x]; if (!t || dn[y][x]) continue; int w = 1, d = 1;
        while (x + w < GN && flr[y][x + w] == t && !dn[y][x + w]) w++;
        for (;;) { if (y + d >= GN) break; int ok = 1; for (int k = 0; k < w; k++) if (flr[y + d][x + k] != t || dn[y + d][x + k]) ok = 0; if (!ok) break; d++; }
        for (int j = 0; j < d; j++) for (int k = 0; k < w; k++) dn[y + j][x + k] = 1;
        ground(tl(x * 2) - 4, tl(y * 2) - 4, tl((x + w) * 2) + 4, tl((y + d) * 2) + 4, -40, FLOORC[t - 1]); } }
    int th = T / 16, whf = T * 21 / 5, whl = T * 2; float cs = cosLerp(yaw) / 4096.0f, sn = sinLerp(yaw) / 4096.0f; int tx0 = mode == M_LIVE ? camx : 0, tz0 = mode == M_LIVE ? camz : 0;   // thin walls (a skirting board, the wall, a pale cap) and proper doors
    for (int y = 0; y <= GN; y++) for (int x = 0; x < GN; x++) { int v = hwl[y][x]; if (!v) continue; int e = x + 1; if (v != 5) while (e < GN && hwl[y][e] == v) e++;   // runs of the same wall are one piece (the polygon budget)
        if (v == 5) { door_open = 0; for (int q = 0; q < nsim; q++) if (abs(sims[q].x - x * 16) < 26 && abs(sims[q].y - (y * 16 - 8)) < 26) door_open = 1; }
        int through0 = y > 0 && y < GN && vwl[y - 1][x] && vwl[y][x], through1 = y > 0 && y < GN && e <= GN && vwl[y - 1][e] && vwl[y][e];   // does a wall run right across the end? then stop inside it (no poking out)
        int mx = (tl(x * 2) + tl(e * 2)) / 2, mz = tl(y * 2); int near = (int)((mx - tx0) * -sn + (mz - tz0) * cs) > 2000;
        wall_seg(tl(x * 2) - (through0 ? -6 : th + 4), tl(y * 2) - th, tl(e * 2) + (through1 ? -6 : th + 4), tl(y * 2) + th, v, 0, near ? whl : whf); x = e - 1; }
    for (int x = 0; x <= GN; x++) for (int y = 0; y < GN; y++) { int v = vwl[y][x]; if (!v) continue; int e = y + 1; if (v != 5) while (e < GN && vwl[e][x] == v) e++;
        if (v == 5) { door_open = 0; for (int q = 0; q < nsim; q++) if (abs(sims[q].x - (x * 16 - 8)) < 26 && abs(sims[q].y - y * 16) < 26) door_open = 1; }
        int through0 = x > 0 && x < GN && hwl[y][x - 1] && hwl[y][x], through1 = x > 0 && x < GN && e <= GN && hwl[e][x - 1] && hwl[e][x];
        int mx = tl(x * 2), mz = (tl(y * 2) + tl(e * 2)) / 2; int near = (int)((mx - tx0) * -sn + (mz - tz0) * cs) > 2000;
        wall_seg(tl(x * 2) - th, tl(y * 2) - (through0 ? -6 : th + 12), tl(x * 2) + th, tl(e * 2) + (through1 ? -6 : th + 12), v, 1, near ? whl : whf); y = e - 1; }
    for (int i = 0; i < nsim; i++) sim3d(i, yaw);   // sims first: they always get their polygons
    for (int i = 0; i < nobj; i++) {   // then the items, nearest the camera first in budget terms: far ones are skipped when the DS runs out of polygons
        if (mode == M_LIVE) { int ox2 = tl(objs[i].x * 2) - camx, oz2 = tl(objs[i].y * 2) - camz; if (abs(ox2) + abs(oz2) > 9000 && polys > 1800) continue; if (polys > 2900) continue; }
        obj3d(objs[i].x, objs[i].y, &ITEMS[objs[i].c][objs[i].i], objs[i].rot, 0); }
    if (mode == M_LIVE) { draw_city(); draw_far_all(yaw); }
    if (mode == M_LIVE && mark_t > 0) { ground(tl(mark_x * 2), tl(mark_y * 2), tl(mark_x * 2 + 2), tl(mark_y * 2 + 2), -60, C15(31, 31, 12)); mark_t--; }
    if (mode == M_BUY || mode == M_BUILD) {   // the cursor
        ground(tl(pcx * 2), tl(pcy * 2), tl(pcx * 2 + 2), tl(pcy * 2 + 2), -80, C15(31, 31, 31));
        if (mode == M_BUY && pick >= 0) obj3d(pcx, pcy, &ITEMS[cat][pick], prot, 1);
    }
}

// ============ the Sims-style game: LIVE (status on the top screen, the lot and its icons on the bottom, touch to play) ============
typedef struct { const char *lab; u8 need; u8 amt; u8 dur; } Act;
static const Act ACT[7][4] = {
    { {"Sit", 2, 25, 90}, {"Relax", 2, 35, 120}, {"Nap", 2, 60, 150}, {"Sit", 2, 20, 90} },
    { {"Sit", 1, 10, 60}, {"Look", 1, 5, 40}, {"Look", 1, 5, 40}, {"Cook", 0, 35, 120} },
    { {"Watch TV", 1, 60, 150}, {"Play", 1, 50, 150}, {"Listen", 1, 35, 120}, {"Eat", 0, 70, 120} },
    { {"Look", 1, 3, 30}, {"Look", 1, 3, 30}, {"Water", 1, 10, 60}, {"Admire", 1, 15, 60} },
    { {"Sleep", 2, 90, 240}, {"Bathe", 5, 90, 180}, {"Use", 4, 95, 100}, {"Wash", 5, 30, 80} },
    { {"Sleep", 2, 80, 200}, {"Play", 1, 60, 120}, {"Read", 1, 40, 120}, {"Rock", 2, 40, 150} },
    { {"Look", 1, 3, 30}, {"Look", 1, 3, 30}, {"Look", 1, 3, 30}, {"Smell", 1, 10, 60} } };
static const char *const NEEDN[6] = { "Hunger", "Fun", "Energy", "Social", "Bladder", "Hygiene" };
static int menu_open, popup, popup_i, speed = 1, yaw_t = 4096, live_yaw = 4096, live_dist = 12000, camx, camz, cas_tab, cas_done, cas_stage, cas_page;
static const u16 BG_L = C15(18, 25, 31), PAN = C15(8, 16, 28), PANB = C15(2, 7, 20), PANH = C15(5, 12, 26), TXT = C15(31, 31, 31);

static void panel(int x, int y, int w, int h, const char *title) {
    rect(x + 1, y, w - 2, h, PANB); rect(x, y + 1, w, h - 2, PANB); rect(x + 2, y + 2, w - 4, h - 4, PAN);
    if (title) { rect(x + 2, y + 2, w - 4, 16, PANH); text(x + 10, y + 5, 2, TXT, title); }
}
static void backdrop(void) {
    rect(0, 0, 256, 192, BG_L);
    for (int y = 0; y < 192; y += 16) for (int x = (y / 16 & 1) * 16; x < 256; x += 32) rect(x + 6, y + 6, 4, 4, C15(22, 28, 31));
}
int isq(int v); u16 lit(u16 c, int k);
static void ellf(int cx, int cy, int rx, int ry, u16 c) {   // a filled oval
    if (rx < 1 || ry < 1) return;
    for (int y = -ry; y <= ry; y++) { int hw = isq(rx * rx * (ry * ry - y * y) / (ry * ry)); rect(cx - hw, cy + y, 2 * hw + 1, 1, c); }
}
static void face(int x, int y, int sz, const Sim *m) {   // a head and shoulders portrait: flat colours, small crisp details
    int u = sz / 16; if (u < 1) u = 1; int cx = x + sz / 2;
    u16 sk = skin_of(m), hc = hair_of(m), sh = shirt_of(m), skd = shade(sk, 6);
    rect(x, y, sz, sz, C15(22, 22, 24));
    rect(x + u, y + 13 * u, 14 * u, sz - 13 * u, sh); rect(x + u, y + 13 * u, 14 * u, u, shade(sh, 6));          // shoulders
    rect(cx - 2 * u, y + 11 * u, 4 * u, 3 * u, skd);                                                            // neck
    int hw = (m->fc == 1 ? 5 : 4) * u, ht = (m->fc == 2 ? 10 : 9) * u, hx = cx - hw, hy = y + 2 * u + (m->fc == 2 ? 0 : u);
    int st = m->hair;
    if (st == 8) rect(cx - 7 * u, hy - 2 * u, 14 * u, 9 * u, shade(hc, 7));                                     // afro, behind
    if (st == 3 || st == 1 || st == 11) rect(hx - u, hy + u, 2 * hw + 2 * u, ht - u * (st == 1 ? 3 : 0) + (st == 3 ? 4 * u : 0), shade(hc, 7));
    if (st == 9) rect(cx - u, hy + 2 * u, 2 * u, 9 * u, shade(hc, 7));
    rect(hx, hy, 2 * hw, ht, sk); rect(hx, hy + ht - u, 2 * hw, u, skd); rect(hx - u, hy + 3 * u, u, 2 * u, skd); rect(hx + 2 * hw, hy + 3 * u, u, 2 * u, skd);   // head and ears
    if (m->hair != 5) {
        if (st == 7) { rect(cx - u, hy - 2 * u, 2 * u, 4 * u, hc); rect(hx, hy, 2 * hw, u, shade(hc, 8)); }
        else if (st == 6) rect(hx, hy, 2 * hw, u + u / 2, hc);
        else { rect(hx - (st == 8 ? u : 0), hy - u, 2 * hw + (st == 8 ? 2 * u : 0), 3 * u, hc); rect(hx, hy + 2 * u, u, 3 * u, hc); rect(hx + 2 * hw - u, hy + 2 * u, u, 3 * u, hc); }
        if (st == 2) for (int k = 0; k < 5; k++) rect(hx + k * (2 * hw - u) / 4, hy - 3 * u, u, 2 * u, hc);
        if (st == 4) { for (int k = 0; k < 5; k++) rect(hx + k * (2 * hw - u) / 4, hy - 2 * u, u, 2 * u, shade(hc, 7)); rect(hx - u, hy + 3 * u, u, 6 * u, hc); rect(hx + 2 * hw, hy + 3 * u, u, 6 * u, hc); }
        if (st == 10) rect(hx, hy + 2 * u, 2 * hw, u, hc);
        if (st == 11) { rect(hx - 3 * u, hy + 3 * u, 3 * u, 7 * u, hc); rect(hx + 2 * hw, hy + 3 * u, 3 * u, 7 * u, hc); }
        if (st == 3) { rect(hx - u, hy + 3 * u, 2 * u, 9 * u, hc); rect(hx + 2 * hw - u, hy + 3 * u, 2 * u, 9 * u, hc); } }
    int ey = hy + (m->fc == 2 ? 4 : 3) * u + u + (m->pm[3] - 50) * u / 40; int er = m->fc == 3 ? 2 : 1;
    rect(cx - 3 * u, ey, 2 * u, u * er, C15(31, 31, 31)); rect(cx + u, ey, 2 * u, u * er, C15(31, 31, 31));
    rect(cx - 2 * u, ey, u, u * er, C15(5, 3, 2)); rect(cx + u + u / 2, ey, u, u * er, C15(5, 3, 2));
    if (m->fc == 4) { rect(cx - 3 * u, ey, 2 * u, u / 2 + 1, skd); rect(cx + u, ey, 2 * u, u / 2 + 1, skd); }
    rect(cx - 3 * u, ey - u, 2 * u, 1, shade(hc, 6)); rect(cx + u, ey - u, 2 * u, 1, shade(hc, 6));
    rect(cx - u / 2, ey + 2 * u, u, 2 * u, skd);
    if (m->fc == 5) for (int k = 0; k < 3; k++) { px(cx - 3 * u + k * u, ey + 3 * u, shade(sk, 5)); px(cx + u + k * u, ey + 3 * u, shade(sk, 5)); }
    int my = ey + 5 * u;
    if (m->beard == 1) rect(hx, my - u, 2 * hw, hy + ht - my + u, lit(sk, 11));
    if (m->beard >= 4) rect(hx, my - 2 * u, 2 * hw, hy + ht - my + 2 * u + (m->beard == 5 ? 3 * u : 0), shade(hc, 8));
    if (m->beard == 2 || m->beard == 3) rect(cx - 3 * u, my - u, 6 * u, u, shade(hc, 8));
    if (m->beard == 3) rect(cx - u, my + u, 2 * u, 2 * u, shade(hc, 8));
    { int mw = 3 * u * (50 + m->pm[0]) / 100; if (mw < 2) mw = 2; rect(cx - mw / 2, my + (50 - m->pm[1]) * u / 40, mw, u, C15(22, 7, 8)); }
}
static int mood(const Sim *m) { int s = 0; for (int n = 0; n < 6; n++) s += m->need[n]; return s / 6; }
static u16 need_col(int v) { return v > 55 ? C15(8, 26, 8) : v > 28 ? C15(28, 24, 6) : C15(30, 8, 6); }
static const char *DAYN[7] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
static const char *relname(int r) { return r < 3 ? "Acquaintance" : r < 6 ? "Friend" : "Best Friend"; }
static void draw_top_status(void) {   // the top screen while you live: the sim's mood, the time and money, and the needs (or the bio of another sim)
    scr = scr_sub; backdrop(); const Sim *m = &sims[cur_sim];
    char t[48];
    if (cur_sim == ctl) {
        face(8, 8, 56, m);
        panel(70, 6, 180, 60, sname[ctl]);
        text(78, 28, 1, TXT, "Mood"); rect(112, 26, 70, 9, C15(24, 28, 31)); rect(113, 27, 68 * mood(m) / 100, 7, need_col(mood(m)));
        int mn = minutes % 1440, h12 = (mn / 60) % 12 ? (mn / 60) % 12 : 12;
        sprintf(t, "%s %d:%02d %s", DAYN[(minutes / 1440) % 7], h12, mn % 60, mn / 60 >= 12 ? "PM" : "AM"); text(78, 42, 1, TXT, t); sprintf(t, "%s %d", STGN[stage_of(m->age)], m->age); text(190, 28, 1, C15(31, 31, 20), t);
        sprintf(t, "Funds %d", funds); text(78, 54, 1, C15(31, 31, 20), t);
        panel(6, 72, 244, 114, "Needs");
        static const int order[6] = { 0, 1, 2, 3, 4, 5 };
        for (int k = 0; k < 6; k++) { int n = order[k], cx = 14 + (k & 1) * 120, cy = 94 + (k >> 1) * 30;
            rect(cx, cy, 14, 14, C15(18, 24, 31)); text(cx + 4, cy + 4, 1, PANB, "-"); rect(cx + 3, cy + 3, 8, 8, need_col(m->need[n]));
            text(cx + 18, cy, 1, TXT, NEEDN[n]); rect(cx + 18, cy + 7, 84, 8, C15(2, 6, 14)); rect(cx + 19, cy + 8, 82 * m->need[n] / 100, 6, need_col(m->need[n])); }
    } else {
        char tt[40]; sprintf(tt, "%s - %s %d", sname[cur_sim], STGN[stage_of(m->age)], m->age); panel(6, 6, 244, 180, tt);
        face(16, 30, 64, m);
        for (int k = 0; k < 4; k++) { rect(94 + k * 38, 30, 32, 32, C15(18, 24, 31)); text(106 + k * 38, 40, 2, PANB, "?"); }
        panel(94, 70, 148, 60, 0);
        panel(94, 136, 148, 40, 0); text(102, 150, 1, TXT, "-"); text(110, 150, 1, TXT, relname(m->rel));
        sprintf(t, "%d", mood(m)); text(16, 100, 1, TXT, "MOOD"); rect(16, 110, 64, 8, C15(24, 28, 31)); rect(17, 111, 62 * mood(m) / 100, 6, need_col(mood(m)));
    }
}
static void hud_clear(void) { u32 *q = (u32 *)hud; for (int i = 0; i < 256 * 192 / 2; i++) q[i] = 0; }
enum { S_CHAT, S_PLAY, S_HUG, S_JOKE, S_TEASE, S_ARGUE, S_FLIRT, S_KISS, S_BABY, S_FEED, S_TEACH, S_TUCK, S_ADVICE, S_INVITE, S_N };
static const char *const SOCN[S_N] = { "Chat", "Play", "Hug", "Joke", "Tease", "Argue", "Flirt", "Kiss", "Try for Baby", "Feed", "Teach", "Tuck In", "Advice", "Invite Home" };
static const signed char SREL[S_N] = { 1, 1, 1, 1, -1, -2, 1, 2, 1, 0, 1, 0, 1, 0 };
static const u8 SSOC[S_N] = { 25, 10, 20, 10, 0, 0, 15, 25, 10, 0, 5, 0, 15, 0 }, SFUN[S_N] = { 0, 30, 0, 20, 10, 0, 10, 0, 30, 0, 15, 0, 0, 0 };
static int rom_ok(int sa, int sb) { return (sa == 3 && sb == 3) || (sa >= 4 && sb >= 4); }   // romance: teens with teens, adults and elders with each other; never across the line
static int soc_ok(int id, int sa, int sb, int rel, int cit) {
    switch (id) {
        case S_CHAT: return sa >= 1 && sb >= 1; case S_PLAY: return sa >= 1 && (sa <= 2 || sb <= 2); case S_HUG: return sa >= 1; case S_JOKE: return sa >= 2 && sb >= 2;
        case S_TEASE: return sa >= 2 && sb >= 2; case S_ARGUE: return sa >= 3 && sb >= 2; case S_FLIRT: return sa >= 3 && sb >= 3 && rom_ok(sa, sb);
        case S_KISS: return sa >= 3 && sb >= 3 && rom_ok(sa, sb) && rel >= 4; case S_BABY: return !cit && sa == 4 && sb == 4 && rel >= 6;
        case S_FEED: return !cit && sa >= 2 && sb <= 1; case S_TEACH: return sa >= 3 && sb >= 1 && sb <= 2; case S_TUCK: return !cit && sa >= 2 && sb <= 1;
        case S_ADVICE: return sa >= 2 && sb == 5; case S_INVITE: return cit && sa >= 3 && sb >= 3 && rel >= 3; }
    return 0;
}
static int soc_pick[8];
static int soc_list(int tgt) { int sa = stage_of(sims[ctl].age), sb, rel, cit = tgt <= -2, n = 0;
    if (cit) { const Cit *c = &cits[-2 - tgt]; sb = c->stage; rel = crel(c->id)->rel; } else { sb = stage_of(sims[tgt].age); rel = sims[tgt].rel; }
    for (int id = 0; id < S_N && n < 8; id++) if (soc_ok(id, sa, sb, rel, cit)) soc_pick[n++] = id; return n; }
static void draw_hud(void) {
    scr = hud; hud_clear();
    rect(2, 2, 26, 26, PANB); rect(3, 3, 24, 24, C15(14, 22, 28)); rect(8, 12, 14, 10, C15(26, 14, 10)); rect(6, 8, 18, 5, C15(24, 6, 6)); rect(13, 16, 4, 6, C15(8, 8, 10));   // MAP: a little house
    for (int i = 0; i < nsim; i++) { int x = 132 + i * 21; rect(x, 2, 20, 27, i == ctl ? C15(31, 31, 12) : PANB); rect(x + 1, 3, 18, 25, C15(14, 22, 28)); face(x + 2, 4, 16, &sims[i]); rect(x + 1, 24, 18, 4, C15(2, 6, 14)); rect(x + 1, 24, 18 * mood(&sims[i]) / 100, 4, need_col(mood(&sims[i]))); }   // the household
    rect(0, 158, 34, 34, PANB); face(2, 160, 30, &sims[ctl]);
    rect(36, 168, 24, 22, PANB); rect(37, 169, 22, 20, PAN); text(42, 176, 1, TXT, speed == 1 ? ">" : speed == 2 ? ">>" : ">>>");
    rect(62, 168, 30, 22, PANB); rect(63, 169, 28, 20, cam_on ? C15(8, 26, 8) : PAN); text(66, 176, 1, TXT, "Cam");
    if (cam_on) { const char *cb[6] = { cam_pan ? "Pan" : "Turn", "In", "Out", cam_free ? "Free" : "Follow", "Reset", "Done" }; for (int k = 0; k < 6; k++) { rect(216, 36 + k * 22, 38, 20, PANB); rect(217, 37 + k * 22, 36, 18, k == 0 && cam_pan ? C15(8, 26, 8) : PANH); text(220, 43 + k * 22, 1, TXT, cb[k]); } text(70, 40, 1, TXT, "Drag to look round"); }
    rect(104, 178, 48, 14, PANB); rect(105, 179, 46, 13, PAN); for (int i = 0; i < 3; i++) rect(118 + i * 8, 184, 4, 4, TXT);
    if (menu_open) { const char *mn[4] = { "Buy", "Build", "Map", "Menu" }; for (int i = 0; i < 4; i++) { rect(40 + i * 46, 150, 44, 24, PANB); rect(41 + i * 46, 151, 42, 22, PANH); text(46 + i * 46, 158, 1, TXT, mn[i]); } }
    if (popup == 1) {
        const Obj *o = &objs[popup_i]; const char *nm = ITEMS[o->c][o->i].name, *lab = ACT[o->c][o->i].lab;
        rect(56, 100, 144, 56, PANB); rect(57, 101, 142, 54, PAN); text(64, 106, 2, TXT, nm);
        rect(64, 126, 62, 24, C15(8, 20, 8)); text(70, 135, 1, TXT, lab); rect(132, 126, 62, 24, C15(24, 8, 8)); text(138, 135, 1, TXT, "Cancel");
    } else if (popup >= 2) {
        char nm[40]; int tg = popup == 2 ? popup_i : -2 - popup_i, rel; if (popup == 2) { sprintf(nm, "%s - %s %d", sname[popup_i], STGN[stage_of(sims[popup_i].age)], sims[popup_i].age); rel = sims[popup_i].rel; }
        else { char n2[24]; cit_name(&cits[popup_i], n2); sprintf(nm, "%s - %s", n2, STGN[cits[popup_i].stage]); rel = crel(cits[popup_i].id)->rel; }
        int n = soc_list(tg);
        rect(20, 56, 216, 124, PANB); rect(21, 57, 214, 122, PAN); rect(21, 57, 214, 14, PANH); rect(20, 56, 2, 2, 0); rect(234, 56, 2, 2, 0); text(26, 61, 1, TXT, nm); rect(212, 60, 22, 14, C15(24, 8, 8)); text(219, 64, 1, TXT, "X");
        sprintf(nm, "%s", relname(rel)); text(26, 73, 1, C15(24, 28, 31), nm);
        for (int k = 0; k < n; k++) { int x = 26 + (k & 1) * 104, y = 82 + (k >> 1) * 22; rect(x, y, 100, 20, PANB); rect(x + 1, y + 1, 98, 18, soc_pick[k] == S_INVITE || soc_pick[k] == S_BABY ? C15(8, 20, 8) : PANH); text(x + 6, y + 6, 1, TXT, SOCN[soc_pick[k]]); }
        if (!n) text(26, 100, 1, TXT, "Nothing to do together"); }
    if (msg_t > 0) { rect(70, 40, 116, 14, PANB); text(76, 45, 1, TXT, msg); }
}
// ---- picking: a stylus tap on the 3D lot -> the cell under it (the view is a camera we know, so we undo it) ----
static int pick_cell(int sx, int sy, int *cx, int *cy) {
    float c = cosLerp(live_yaw) / 4096.0f, s = sinLerp(live_yaw) / 4096.0f, cp = cosLerp(pitch) / 4096.0f, sp = sinLerp(pitch) / 4096.0f;
    float nx = (sx - 128) / 128.0f, ny = (96 - sy) / 96.0f, dx = nx * 0.3640f * 1.3333f, dy = ny * 0.3640f, dz = -1.0f;
    float ox = 0, oy = 0, oz = (float)cam_deff;
    float ay = dy * cp + dz * sp, az = -dy * sp + dz * cp, by = oy * cp + oz * sp, bz = -oy * sp + oz * cp;
    float ddx = dx * c - az * s, ddz = dx * s + az * c, oox = ox * c - bz * s + camx, ooz = ox * s + bz * c + camz;
    if (ay > -0.0001f && ay < 0.0001f) return 0;
    float t = -by / ay; float hx = oox + t * ddx, hz = ooz + t * ddz;
    *cx = (int)((hx / T + GN) / 2.0f + 200) - 200; *cy = (int)((hz / T + GN) / 2.0f + 200) - 200;
    return 1;
}

// ---- interactions: what each life stage may do with whom (and how often) ----
static int cl100(int v) { return v < 0 ? 0 : v > 100 ? 100 : v; }
static void new_sim(int age, u8 skin, u8 hair, u8 hcol, u8 shirt, const char *nm, int x, int y) {
    if (nsim >= SIM_MAX) return; Sim *m = &sims[nsim]; memset(m, 0, sizeof *m); m->x = m->tx = m->wx = x; m->y = m->ty = m->wy = y; m->act = -1; m->soc = -1; m->age = age; m->skin = skin; m->hair = hair; m->hcol = hcol; m->shirt = shirt; m->fc = (u8)(nsim % 6);
    for (int n = 0; n < 6; n++) m->need[n] = 70; for (int q = 0; q < PMN; q++) m->pm[q] = 50; strncpy(sname[nsim], nm, 19); sname[nsim][19] = 0; nsim++; ui_ver++;
}
static void apply_soc(int ai, int tgt, int id) {
    Sim *a = &sims[ai]; int cit = tgt <= -2, sb, rel; Sim *o = 0; CRel *cr = 0; Cit *ct = 0;
    if (cit) { ct = &cits[-2 - tgt]; cr = crel(ct->id); sb = ct->stage; rel = cr->rel; if (cr->talks >= 4) { if (ai == ctl) say("Too busy to talk"); return; } cr->talks++; ct->stop = 100; }
    else { o = &sims[tgt]; sb = stage_of(o->age); rel = o->rel; if (ilim[ai][tgt] >= 6) { if (ai == ctl) say("Not in the mood"); return; } ilim[ai][tgt]++; ilim[tgt][ai]++; }
    if (!soc_ok(id, stage_of(a->age), sb, rel, cit)) return;
    int d = SREL[id]; a->need[3] = cl100(a->need[3] + SSOC[id]); a->need[1] = cl100(a->need[1] + SFUN[id]);
    if (o) { o->need[3] = cl100(o->need[3] + SSOC[id]); o->need[1] = cl100(o->need[1] + SFUN[id]); }
    if (id == S_FEED) o->need[0] = cl100(o->need[0] + 60); if (id == S_TUCK) o->need[2] = cl100(o->need[2] + 60);
    if (id == S_BABY) { if (nsim >= SIM_MAX) { if (ai == ctl) say("Household is full"); return; } if (pop_per + pop_born >= DS_POP_MAX) { if (ai == ctl) say("City is full (2000)"); return; }
        int bx2 = a->x + 16, by2 = a->y; for (int k = 0; k < nobj; k++) if (objs[k].c == 5 && objs[k].i == 0) { bx2 = objs[k].x * 16; by2 = objs[k].y * 16; break; }
        static const char *const BN[8] = { "Bo", "Remy", "Pip", "Juno", "Tate", "Wren", "Otis", "Fern" }; char nm[20]; sprintf(nm, "%s %s", BN[(minutes / 7 + nsim) % 8], strrchr(sname[ai], ' ') ? strrchr(sname[ai], ' ') + 1 : "B");
        new_sim(0, (minutes & 1) ? a->skin : o->skin, (u8)((minutes + nsim) % 12), (minutes & 2) ? a->hcol : o->hcol, (u8)(minutes % 6), nm, bx2, by2); pop_born++; if (ai == ctl) say("A baby is born!"); }
    if (id == S_INVITE) { if (nsim >= SIM_MAX) { if (ai == ctl) say("Household is full"); return; } char nm[20]; cit_name(ct, nm);
        new_sim(sb == 3 ? 15 : sb == 4 ? 28 : 65, ct->skin, (u8)((ct->id >> 3) % 12), ct->hair, ct->shirt, nm, a->x, a->y); cr->taken = 1; ct->on = 0; if (ai == ctl) say("Moved in!"); return; }
    if (cit) { int r = (int)cr->rel + d; cr->rel = (u8)(r < 0 ? 0 : r > 10 ? 10 : r); } else { int r = o->rel + d; o->rel = r < 0 ? 0 : r > 10 ? 10 : r; r = a->rel + d; a->rel = r < 0 ? 0 : r > 10 ? 10 : r; }
    if (ai == ctl && id != S_BABY) say(SOCN[id]);
}
static void start_soc(int ai, int tgt, int id) {
    Sim *m = &sims[ai]; int tx, ty;
    if (tgt <= -2) { const Cit *c = &cits[-2 - tgt]; tx = fdiv(c->x + 8, 16) * 16; ty = fdiv(c->y + 8, 16) * 16; } else { tx = sims[tgt].x; ty = sims[tgt].y; }
    m->tx = tx; m->ty = ty; m->act = 3 + 100; m->tgt = tgt; m->soc = id; m->act_amt = 50; m->face = 120; m->act_left = 0;
}
static void day_roll(void) {
    memset(ilim, 0, sizeof ilim); for (int k = 0; k < 64; k++) crt[k].talks = 0;
    for (int i = 0; i < nsim; i++) { Sim *m = &sims[i]; int s0 = stage_of(m->age); if (m->age < 99) m->age++; int s1 = stage_of(m->age); if (s0 != s1) { static char bf[40]; sprintf(bf, "%s is now a %s", sname[i], STGN[s1]); say(bf); } }
}
static void start_act(Sim *m, int obj, int sim) {   // walk to it, then do it
    if (sim >= 0) { start_soc((int)(m - sims), sim, S_CHAT); }
    else { const Obj *o = &objs[obj]; if (!((ITMASK[o->c * 4 + o->i] >> stage_of(m->age)) & 1)) { say(stage_of(m->age) < 3 ? "Too young for that" : "Not for your age"); return; } const Act *a = &ACT[o->c][o->i]; m->tx = o->x * 16; m->ty = o->y * 16; m->act = a->need; m->act_amt = a->amt; m->face = a->dur; m->act_obj = obj; }
}
static int obj_solid(int cx, int cy) { if (cx < 0 || cy < 0 || cx >= GN || cy >= GN) return far_solid(cx, cy); for (int i = 0; i < nobj; i++) if (objs[i].x == cx && objs[i].y == cy && ITEMS[objs[i].c][objs[i].i].kind != 1) return 1; return 0; }
static int edge_open(int ax, int ay, int bx2, int by) {   // can a sim step between two neighbouring cells? walls block, doors do not
    int ina = ax >= 0 && ay >= 0 && ax < GN && ay < GN, inb = bx2 >= 0 && by >= 0 && bx2 < GN && by < GN;
    if (!ina || !inb) { if (ina == inb) return 1; return !(by != ay && (ay == GN - 1 || by == GN - 1) && bx2 != door_cx && ax != door_cx) ? 1 : 0; }   // out of the plot: open, but the back fence has one gap, at the path
    int w;
    if (bx2 > ax) w = vwl[ay][ax + 1]; else if (bx2 < ax) w = vwl[ay][ax]; else if (by > ay) w = hwl[ay + 1][ax]; else w = hwl[ay][ax];
    return w == 0 || w == 5;
}
static int next_cell(int sx, int sy, int gx, int gy, int *nx, int *ny) {   // breadth-first search from the goal over the whole 3x3 patch; returns the neighbour of (sx,sy) that is one step closer
    #define WO SLC
    #define WN (SLC * 3)
    static short dd[WN][WN]; static u16 qq[WN * WN]; static const int DX[4] = { 1, -1, 0, 0 }, DY[4] = { 0, 0, 1, -1 };
    #define CL(v) ((v) < -WO ? -WO : (v) >= WN - WO ? WN - WO - 1 : (v))
    gx = CL(gx); gy = CL(gy); sx = CL(sx); sy = CL(sy);
    memset(dd, -1, sizeof dd); int h = 0, t = 0; qq[t++] = (u16)((gy + WO) * WN + gx + WO); dd[gy + WO][gx + WO] = 0;
    while (h < t) { int cx = (int)(qq[h] % WN) - WO, cy = (int)(qq[h] / WN) - WO; h++; for (int k = 0; k < 4; k++) { int ax = cx + DX[k], ay = cy + DY[k];
        if (ax < -WO || ay < -WO || ax >= WN - WO || ay >= WN - WO || dd[ay + WO][ax + WO] >= 0 || !edge_open(cx, cy, ax, ay)) continue;
        if (obj_solid(ax, ay) && !(ax == sx && ay == sy)) continue; dd[ay + WO][ax + WO] = (short)(dd[cy + WO][cx + WO] + 1); qq[t++] = (u16)((ay + WO) * WN + ax + WO); } }
    if (dd[sy + WO][sx + WO] < 0) return 0;
    for (int k = 0; k < 4; k++) { int ax = sx + DX[k], ay = sy + DY[k]; if (ax < -WO || ay < -WO || ax >= WN - WO || ay >= WN - WO) continue; if (dd[ay + WO][ax + WO] >= 0 && dd[ay + WO][ax + WO] < dd[sy + WO][sx + WO] && edge_open(sx, sy, ax, ay)) { *nx = ax; *ny = ay; return 1; } }
    return 0;
    #undef CL
}
static void sims_step(int frames) {
    for (int i = 0; i < nsim; i++) { Sim *m = &sims[i];
        if (m->act_left > 0) {
            for (int n = 0; n < 6; n++) if (n == (m->act >= 100 ? m->act - 100 : m->act)) { m->acc[n] += m->act_amt; while (m->acc[n] >= m->face) { m->acc[n] -= m->face; if (m->need[n] < 100) m->need[n]++; } }
            m->act_left--; if (m->act_left == 0) { m->act = -1; if (i == ctl) say("Done"); }
        } else if (m->x != m->tx || m->y != m->ty) {
            if ((m->x & 15) == 0 && (m->y & 15) == 0 && m->wx == m->x && m->wy == m->y) {   // on a cell centre: plan the next cell round the walls
                int nx, ny; if (next_cell(fdiv(m->x + 8, 16), fdiv(m->y + 8, 16), fdiv(m->tx + 8, 16), fdiv(m->ty + 8, 16), &nx, &ny)) { m->wx = nx * 16; m->wy = ny * 16; }
                else { m->tx = m->x; m->ty = m->y; m->act = -1; if (i == ctl) say("Cannot get there"); continue; } }
            if (m->x < m->wx) { m->x++; m->dirn = 8192; } else if (m->x > m->wx) { m->x--; m->dirn = 24576; } else if (m->y < m->wy) { m->y++; m->dirn = 0; } else if (m->y > m->wy) { m->y--; m->dirn = 16384; }
            if (m->x == m->tx && m->y == m->ty && m->act >= 0) {
                m->act_left = m->face;
                if (m->act >= 100) { if (m->tgt >= 0) { Sim *o = &sims[m->tgt]; o->act_left = m->face; o->act = 3 + 100; o->act_amt = 0; } m->act = 3; apply_soc(i, m->tgt, m->soc); }
            }
        } else if (i != ctl && stage_of(m->age) > 0 && ((frames + i * 37) & 127) == 0) { m->tx = (1 + (frames * 7 + i * 5) % 11) * 16; m->ty = (1 + (frames * 3 + i * 9) % 12) * 16; m->act = -1; }
        if (((frames + i * 9) % 70) == 0) for (int n = 0; n < 6; n++) if (m->need[n] > 4 && n != 3) m->need[n]--;
        if (((frames + i * 9) % 140) == 0 && m->need[3] > 4) m->need[3]--;
    }
    if ((frames % 8) == 0) { minutes++; if (minutes % 1440 == 0) day_roll(); }
}
// ---- smooth shaded 3D: ellipsoids with a colour on every vertex, so a head is round and lit instead of a box ----
static int gox, goy, goz, gcs = 4096, gsn, gres_r = 8, gres_s = 12;   // the model's place and facing (yaw), and how finely to cut a ball
int isq(int v) { if (v <= 0) return 0; int r = v, q = (r + 1) / 2; while (q < r) { r = q; q = (r + v / r) / 2; } return r; }
u16 lit(u16 c, int k) {   // k/16 of the colour (above 16 is brighter)
    int r = (c & 31) * k / 16, g = ((c >> 5) & 31) * k / 16, b = ((c >> 10) & 31) * k / 16;
    return (u16)(RGB15(r > 31 ? 31 : r, g > 31 ? 31 : g, b > 31 ? 31 : b) | BIT(15));
}
static inline void gv(int x, int u, int z) { glVertex3v16(gox + ((x * gcs + z * gsn) >> 12), goy + u, goz + ((z * gcs - x * gsn) >> 12)); }
static int shade_n(int nx, int ny, int nz) {   // light from the front, top and left: 0..16+
    int k = (nx * -1434 + ny * 2253 + nz * 3072) >> 12;   // -4096..4096
    return 8 + (k > 0 ? k * 9 / 4096 : k * 5 / 4096);
}
// an ellipsoid (centre cx,cu,cz; radii; phi p0..p1 from -8192 (bottom) to 8192 (top); around a0..a1: 0 is +x, 8192 is the front +z); taper narrows the lower half
static void ell3(int cx, int cu, int cz, int rx, int ru, int rz, u16 col, int p0, int p1, int a0, int a1, int taper) {
    int R = gres_r, S = gres_s;
    for (int i = 0; i < R; i++) for (int j = 0; j < S; j++) {
        int pp[2] = { p0 + (p1 - p0) * i / R, p0 + (p1 - p0) * (i + 1) / R }, aa[2] = { a0 + (a1 - a0) * j / S, a0 + (a1 - a0) * (j + 1) / S };
        int vx[4], vu[4], vz[4], vk[4];
        for (int k = 0; k < 4; k++) {
            int p = pp[(k == 2 || k == 3) ? 1 : 0], a = aa[(k == 1 || k == 2) ? 1 : 0];
            int cph = cosLerp(p), sph = sinLerp(p), ca = cosLerp(a), sa = sinLerp(a);
            int f = 4096 - (sph < 0 ? taper * (-sph) / 4096 : 0);
            int nx = (cph * ca) >> 12, nz = (cph * sa) >> 12;
            vx[k] = cx + (((rx * nx) >> 12) * f >> 12); vz[k] = cz + (((rz * nz) >> 12) * f >> 12); vu[k] = cu + ((ru * sph) >> 12); vk[k] = shade_n(nx, sph, nz);
        }
        for (int k = 0; k < 4; k++) { glColor(lit(col, vk[k] < 2 ? 2 : vk[k])); gv(vx[k], vu[k], vz[k]); }
        polys += 2;
    }
}
static void disc(int cx, int cu, int cz, int rx, int ru, u16 col, int k) {   // a flat oval facing front (+z), drawn as a fan of quads
    for (int i = 0; i < 6; i++) {
        int a0 = i * 32768 / 6, a1 = (i + 1) * 32768 / 6;
        glColor(lit(col, k)); gv(cx, cu, cz);
        gv(cx + ((rx * cosLerp(a0)) >> 12), cu + ((ru * sinLerp(a0)) >> 12), cz);
        gv(cx + ((rx * cosLerp(a1)) >> 12), cu + ((ru * sinLerp(a1)) >> 12), cz);
        gv(cx + ((rx * cosLerp(a1)) >> 12), cu + ((ru * sinLerp(a1)) >> 12), cz); polys++;
    }
}
static int head_z(int cu, int ru, int rz, int rx, int x, int u) {   // the head's front surface at (x, u)
    int a = x * 1024 / rx, b = (u - cu) * 1024 / ru, v = 1024 * 1024 - a * a - b * b; if (v < 0) v = 0; return rz * isq(v) / 1024;
}
static void hair_do(const Sim *m, int cu, int rx, int ru, int rz, u16 hc) {   // the hair, by style
    int st = m->hair; if (st == 5) return;
    ell3(0, cu, 0, rx * 107 / 100, ru * 107 / 100, rz * 107 / 100, hc, 3200, 8192, 0, 32768, 0);               // the cap
    if (st == 0) ell3(0, cu, 0, rx * 108 / 100, ru * 108 / 100, rz * 108 / 100, hc, -800, 3300, 13400, 35900, 0);
    if (st == 1) ell3(0, cu, 0, rx * 112 / 100, ru * 108 / 100, rz * 112 / 100, hc, -5200, 3300, 12400, 36900, 0);
    if (st == 2) for (int k = 0; k < 9; k++) {   // spikes: cones round the crown
        int a = k * 32768 / 9, p = 5200 + (k & 1) * 1400, ca = cosLerp(a), sa = sinLerp(a), cp = cosLerp(p), sp = sinLerp(p);
        int bx = rx * cp / 4096 * ca / 4096, bz = rz * cp / 4096 * sa / 4096, bu = cu + ru * sp / 4096, tx = bx * 13 / 10, tz = bz * 13 / 10, tu = bu + 520;
        for (int q = 0; q < 4; q++) { int a0 = q * 8192, a1 = (q + 1) * 8192; int x0 = bx + 130 * cosLerp(a0) / 4096, z0 = bz + 130 * sinLerp(a0) / 4096, x1 = bx + 130 * cosLerp(a1) / 4096, z1 = bz + 130 * sinLerp(a1) / 4096;
            glColor(lit(hc, 10 + q * 2)); gv(x0, bu, z0); gv(x1, bu, z1); gv(tx, tu, tz); gv(tx, tu, tz); polys++; } }
    if (st == 3) {   // long: a curtain behind and down to the shoulders
        ell3(0, cu, 0, rx * 112 / 100, ru * 108 / 100, rz * 112 / 100, hc, -3800, 3300, 11400, 37900, 0);
        for (int j = 0; j < 12; j++) { int a0 = 13400 + j * (35900 - 13400) / 12, a1 = 13400 + (j + 1) * (35900 - 13400) / 12;
            int rr = rx * 112 / 100, rz2 = rz * 112 / 100, lo = cu - ru * 100 / 100 - 600;
            int x0 = rr * cosLerp(a0) / 4096, z0 = rz2 * sinLerp(a0) / 4096, x1 = rr * cosLerp(a1) / 4096, z1 = rz2 * sinLerp(a1) / 4096;
            glColor(lit(hc, 7 + (j & 1))); gv(x0, cu - ru * 30 / 100, z0); gv(x1, cu - ru * 30 / 100, z1); gv(x1 * 12 / 10, lo, z1 * 12 / 10); gv(x0 * 12 / 10, lo, z0 * 12 / 10); polys += 2; } }
    if (st == 4) {   // braids: strings of beads hanging round the head
        int sr = gres_r, ss = gres_s; gres_r = 4; gres_s = 6;
        for (int k = 0; k < 9; k++) { int a = 11000 + k * (40000 - 11000) / 9, ca = cosLerp(a), sa = sinLerp(a);
            for (int b = 0; b < 6; b++) { int bx = rx * 106 / 100 * ca / 4096, bz = rz * 106 / 100 * sa / 4096; ell3(bx * (100 + b * 3) / 100, cu - ru * 15 / 100 - b * 190, bz * (100 + b * 3) / 100, 95 - b * 7, 120, 95 - b * 7, lit(hc, 11 - b / 2), -8192, 8192, 0, 32768, 0); } }
        gres_r = sr; gres_s = ss; }
}
static void draw_person_model(const Sim *m, int sc, int hat) {   // the sim, as a rounded model, at the current place and facing. sc: size in 1/16 of the portrait size
    (void)hat; u16 sk = skin_of(m), hc = hair_of(m), sh = shirt_of(m);
    int rx = 760, ru = 900, rz = 800, cu = 1250; (void)sc;
    ell3(0, -250, 0, 1300, 560, 640, sh, -8192, 8192, 0, 32768, 0);          // shoulders and chest
    ell3(0, 120, 0, 300, 480, 300, lit(sk, 13), -8192, 8192, 0, 32768, 0);    // neck
    ell3(0, cu, 0, rx, ru, rz, sk, -8192, 8192, 0, 32768, 760);              // head
    ell3(-rx * 97 / 100, cu - 20, -60, 90, 200, 130, lit(sk, 13), -8192, 8192, 0, 32768, 0); ell3(rx * 97 / 100, cu - 20, -60, 90, 200, 130, lit(sk, 13), -8192, 8192, 0, 32768, 0);   // ears
    hair_do(m, cu, rx, ru, rz, hc);
    int zf, ey = cu + 150;
    for (int sd = -1; sd <= 1; sd += 2) {   // eyes
        int ex = sd * 290; zf = head_z(cu, ru, rz, rx, ex, ey) + 14;
        disc(ex, ey, zf, 175, 120, C15(31, 31, 31), 15); disc(ex, ey - 6, zf + 4, 92, 92, m->hcol == 5 ? C15(8, 14, 28) : C15(9, 6, 3), 13); disc(ex, ey - 6, zf + 8, 46, 46, C15(1, 1, 2), 16);
        disc(ex - 30, ey + 28, zf + 12, 22, 22, C15(31, 31, 31), 16);
        disc(ex, ey + 128, zf + 6, 190, 30, lit(hc, 9), 14); disc(ex, ey + 300, zf - 2, 210, 36, lit(hc, 11), 14); }   // lid line and brow
    zf = head_z(cu, ru, rz, rx, 0, cu - 280) + 30; ell3(0, cu - 260, zf - 20, 100, 170, 120, lit(sk, 14), -8192, 8192, 8192 - 9000, 8192 + 9000, 0);   // nose
    zf = head_z(cu, ru, rz, rx, 0, cu - 520) + 10;
    disc(0, cu - 560, zf, 200, 62, C15(22, 7, 8), 14); disc(0, cu - 500, zf + 2, 205, 46, C15(26, 10, 10), 15); disc(0, cu - 528, zf + 4, 170, 10, C15(6, 1, 2), 12);   // lips and the line between
}
// ---- a refined blocky look: boxes with bevelled tops, a light from the front-top and a gradient on every face (still chunky, just richer) ----
static void gq(int x0, int u0, int z0, int x1, int u1, int z1, int x2, int u2, int z2, int x3, int u3, int z3, u16 c, int k0, int k1) {   // k0 at the first two corners, k1 at the last two
    glColor(lit(c, k0)); glVertex3v16(x0, u0, z0); glVertex3v16(x1, u1, z1); glColor(lit(c, k1)); glVertex3v16(x2, u2, z2); glVertex3v16(x3, u3, z3); polys += 2;
}
// a box from (x0,u0,z0) to (x1,u1,z1) (u is up); ti insets the top face (a bevel/taper), bi the bottom
static void gbox(int x0, int u0, int z0, int x1, int u1, int z1, u16 c, int ti, int bi) {
    int bx0 = x0 + bi, bx1 = x1 - bi, bz0 = z0 + bi, bz1 = z1 - bi, tx0 = x0 + ti, tx1 = x1 - ti, tz0 = z0 + ti, tz1 = z1 - ti;
    gq(bx0, u0, bz1, bx1, u0, bz1, tx1, u1, tz1, tx0, u1, tz1, c, 11, 14);   // front
    gq(bx1, u0, bz0, bx0, u0, bz0, tx0, u1, tz0, tx1, u1, tz0, c, 7, 9);     // back
    gq(bx1, u0, bz1, bx1, u0, bz0, tx1, u1, tz0, tx1, u1, tz1, c, 7, 10);    // right
    gq(bx0, u0, bz0, bx0, u0, bz1, tx0, u1, tz1, tx0, u1, tz0, c, 9, 12);    // left (the light side)
    gq(tx0, u1, tz1, tx1, u1, tz1, tx1, u1, tz0, tx0, u1, tz0, c, 16, 16);   // top
}
static void flat(int x0, int u0, int x1, int u1, int z, u16 c, int k) { gq(x0, u0, z, x1, u0, z, x1, u1, z, x0, u1, z, c, k, k); }   // a quad on the front
// flat-coloured chunky models (u is up): bx a box, fq a quad on the front
static void bx(int x0, int u0, int z0, int x1, int u1, int z1, u16 c) { block(x0, z0, x1, z1, -u0, -u1, c, shade(c, 10)); }
static void fq(int x0, int u0, int x1, int u1, int z, u16 c) { qd(x0, -u0, z, x1, -u0, z, x1, -u1, z, x0, -u1, z, c); }
static void hair3d(int st, u16 hc, int hw, int ht) {   // the head is x +-hw, z +-hw, u 0..ht
    int hz = hw, top = ht;
    if (st == 5) return;
    if (st == 7) { bx(-100, top - 30, -hz - 40, 100, top + 650, hz + 40, hc); bx(-hw - 30, top - 80, -hz - 30, hw + 30, top + 10, hz + 30, shade(hc, 6)); return; }
    if (st == 6) { bx(-hw - 30, top - 120, -hz - 30, hw + 30, top + 60, hz + 30, hc); return; }
    if (st == 8) { bx(-hw - 260, top - 500, -hz - 240, hw + 260, top + 650, hz + 220, hc); bx(-hw - 120, top + 650, -hz - 120, hw + 120, top + 800, hz + 100, hc); return; }
    bx(-hw - 60, top - 80, -hz - 60, hw + 60, top + 380, hz + 50, hc);                              // the cap
    bx(-hw - 60, top - 600, -hz - 60, hw + 60, top - 70, -hz + 260, hc);                            // the back
    bx(-hw - 70, top - 560, -hz, -hw + 10, top - 70, hz - 500, hc); bx(hw - 10, top - 560, -hz, hw + 70, top - 70, hz - 500, hc);   // sideburns
    if (st == 1) { bx(-hw - 80, top - 1000, -hz - 70, hw + 80, top - 590, hz - 300, hc); }
    if (st == 3) { bx(-hw - 70, -400, -hz - 70, hw + 70, top - 580, -hz + 280, hc); bx(-hw - 100, -300, -300, -hw, top - 600, hz - 300, hc); bx(hw, -300, -300, hw + 100, top - 600, hz - 300, hc); }
    if (st == 2) for (int k = 0; k < 7; k++) for (int q = 0; q < 2; q++) bx(-hw + 30 + k * (2 * hw - 200) / 6, top + 370, -hz + 100 + q * (2 * hz - 300), -hw + 130 + k * (2 * hw - 200) / 6, top + 760 - ((k + q) & 1) * 170, -hz + 220 + q * (2 * hz - 300), hc);
    if (st == 4) { for (int k = 0; k < 5; k++) bx(-hw + 20 + k * (2 * hw - 140) / 4, top + 370, -hz + 40, -hw + 130 + k * (2 * hw - 140) / 4, top + 700, hz - 40, shade(hc, 7));
                   for (int sd = -1; sd <= 1; sd += 2) for (int p = 0; p < 3; p++) bx(sd * (hw + 60) - 40 + (sd > 0 ? 0 : 0), top - 650 - p * 330, 120, sd * (hw + 60) + 40, top - 330 - p * 330, 220, lit(hc, 12 - p)); }
    if (st == 9) { bx(-90, top - 300, -hz - 200, 90, top + 100, -hz, hc); bx(-80, top - 1200, -hz - 280, 80, top - 250, -hz - 100, hc); }
    if (st == 10) bx(-hw - 60, top - 330, hz - 120, hw + 60, top + 200, hz + 90, hc);
    if (st == 11) for (int sd = -1; sd <= 1; sd += 2) { bx(sd * (hw + 360) - 140, top - 400, -140, sd * (hw + 360) + 140, top + 20, 140, hc); bx(sd * (hw + 360) - 90, top - 1100, -90, sd * (hw + 360) + 90, top - 390, 90, hc); }
}
static void beard3d(int bd, u16 hc, u16 sk, int hw, int hz) {
    if (bd == 1) fq(-hw + 60, 60, hw - 60, 560, hz + 4, lit(sk, 10));
    if (bd >= 4) { bx(-hw - 20, bd == 5 ? -650 : -60, -hz + 40, hw + 20, 520, hz + 70, hc); }
    if (bd == 2 || bd == 3) fq(-260, 540, 260, 650, hz + 8, hc);
    if (bd == 3) bx(-170, 20, hz - 80, 170, 480, hz + 70, hc);
}
static int sq_q, fq4[SIM_MAX], sb_sc = 100;
static void sb(int ox, int oz, int lx0, int lu0, int lz0, int lx1, int lu1, int lz1, u16 c) {   // a part in the sim's own frame (z = front), turned to face where it walks
    int ax, az, bx2, bz; lx0 = lx0 * sb_sc / 100; lx1 = lx1 * sb_sc / 100; lu0 = lu0 * sb_sc / 100; lu1 = lu1 * sb_sc / 100; lz0 = lz0 * sb_sc / 100; lz1 = lz1 * sb_sc / 100;
    switch (sq_q & 3) { case 1: ax = lz0; az = -lx0; bx2 = lz1; bz = -lx1; break; case 2: ax = -lx0; az = -lz0; bx2 = -lx1; bz = -lz1; break; case 3: ax = -lz0; az = lx0; bx2 = -lz1; bz = lx1; break; default: ax = lx0; az = lz0; bx2 = lx1; bz = lz1; }
    bx(ox + (ax < bx2 ? ax : bx2), lu0, oz + (az < bz ? az : bz), ox + (ax < bx2 ? bx2 : ax), lu1, oz + (az < bz ? bz : az), c);
}
static void sim3d(int i, int yaw) {
    const Sim *m = &sims[i]; (void)yaw; int P[PMN]; eff_pm(m->pm, P); int stg = stage_of(m->age); sb_sc = STSC[stg]; int lod = i != ctl && (abs(m->x * 2 * T / 16 - GN * T + T - camx) + abs(m->y * 2 * T / 16 - GN * T + T - camz) > 2400); if (stg < 3) { P[8] += (3 - stg) * 24; P[9] += (3 - stg) * 24; }
    int mv = m->tx != m->x || m->ty != m->y;
    if (mv) { int dx = (m->wx != m->x || m->wy != m->y) ? m->wx - m->x : m->tx - m->x, dy = (m->wx != m->x || m->wy != m->y) ? m->wy - m->y : m->ty - m->y; fq4[i] = (dx * dx >= dy * dy) ? (dx > 0 ? 1 : 3) : (dy > 0 ? 0 : 2); }
    sq_q = fq4[i];
    int x = m->x * 2 * T / 16 - GN * T + T, z = m->y * 2 * T / 16 - GN * T + T;
    u16 sk = skin_of(m), hc = stg == 5 ? C15(25, 25, 26) : hair_of(m), sh = shirt_of(m), pants = stg < 2 ? C15(24, 24, 28) : stg == 2 ? C15(8, 12, 22) : C15(6, 7, 12), skd = shade(sk, 7);
    int hw = 100 + (P[8] - 50) / 2, ht = 200 + (P[9] - 50) / 2, lh = 320 + (P[22] - 50) * 2 + (P[26] - 50) * 3, base = 630 + (P[15] - 50) * 2 + (P[22] - 50) * 2 + (P[26] - 50) * 5,
        sw = 135 + (P[14] - 50), at = 50 + (P[21] - 50) / 2, lt = 90 + (P[23] - 50) / 2, bz = 85 + (P[24] - 50) * 2, al = 250 + (P[20] - 50) * 2, hb = base + 30;
    int sw_l = mv ? sinLerp(gframes * 1100 + i * 700) * 70 / 4096 : 0;
    // legs, shoes, belt
    for (int sd = -1; sd <= 1; sd += 2) { int lx0 = sd > 0 ? 8 : -8 - lt, lx1 = sd > 0 ? 8 + lt : -8, o = sd * sw_l;
        sb(x, z, lx0, 0, -60 + o, lx1, 55, 110 + o, C15(5, 4, 4)); sb(x, z, lx0, 55, -50 + o, lx1, lh, 60 + o, pants); }
    sb(x, z, -sw - 4, lh - 10, -89, sw + 4, lh + 40, bz + 4, C15(6, 4, 2)); sb(x, z, -18, lh - 6, bz + 4, 18, lh + 34, bz + 9, C15(30, 26, 8));
    // torso with a collar, neck, shoulders
    sb(x, z, -sw, lh + 40, -85, sw, base, bz, sh); sb(x, z, -sw - 6, base - 70, -90, sw + 6, base, bz + 6, shade(sh, 9));
    sb(x, z, -42, base - 55, bz + 2, 42, base + 2, bz + 8, skd); sb(x, z, -40, base, -40, 40, base + 40, 40, skd);
    // arms: sleeve, forearm, hand; they swing against the legs
    for (int sd = -1; sd <= 1; sd += 2) { int ax0 = sd > 0 ? sw : -sw - at, ax1 = sd > 0 ? sw + at : -sw, o = -sd * sw_l;
        sb(x, z, ax0, base - al * 45 / 100, -at + o, ax1, base - 10, at + o, sh); sb(x, z, ax0 + 4, base - al, -at + 4 + o, ax1 - 4, base - al * 45 / 100, at - 4 + o, sk); sb(x, z, ax0 + 2, base - al - 45, -at + 2 + o, ax1 - 2, base - al, at + 4 + o, sk); }
    // head
    sb(x, z, -hw, hb, -hw, hw, hb + ht, hw, sk); sb(x, z, -hw - 16, hb + ht * 35 / 100, -22, -hw, hb + ht * 65 / 100, 40, skd); sb(x, z, hw, hb + ht * 35 / 100, -22, hw + 16, hb + ht * 65 / 100, 40, skd);
    { int st = m->hair, hu = hb + ht;
      if (st != 5) { sb(x, z, -hw - 22, hu - 70, -hw - 22, hw + 22, hu + 55, hw + 22, hc); sb(x, z, -hw - 22, hu - 150, -hw - 22, hw + 22, hu - 60, -20, hc);
        if (st != 6 && st != 7 && st != 10) sb(x, z, -hw - 8, hu - 62, hw - 12, hw + 8, hu - 20, hw + 26, shade(hc, 9));
        if (st == 3 || st == 1 || st == 11 || st == 9) sb(x, z, -hw - 22, hu - 330, -hw - 22, hw + 22, hu - 150, -20, hc);
        if (st == 8) sb(x, z, -hw - 85, hu - 100, -hw - 85, hw + 85, hu + 180, hw + 60, hc);
        if (st == 2) for (int k = -1; k <= 1; k++) sb(x, z, k * 66 - 24, hu + 55, -24, k * 66 + 24, hu + 165, 24, hc);
        if (st == 7) sb(x, z, -24, hu + 55, -hw, 24, hu + 200, hw, hc);
        if (st == 4) { sb(x, z, -hw - 28, hu - 300, -hw - 28, -hw + 10, hu - 40, hw, hc); sb(x, z, hw - 10, hu - 300, -hw - 28, hw + 28, hu - 40, hw, hc); } } }
    if (!lod) { int ex = 40 + (P[2] - 50) / 2, ey = hb + ht * 55 / 100, es = 28 + (P[16] - 50) / 4, f = hw;
      for (int sd = -1; sd <= 1; sd += 2) { int c0 = sd * ex;
        sb(x, z, c0 - es - 4, ey - 24, f, c0 + es + 4, ey + 26, f + 6, C15(31, 31, 31)); sb(x, z, c0 - 14, ey - 22, f + 4, c0 + 14, ey + 24, f + 10, C15(7, 5, 3)); sb(x, z, c0 - 6, ey - 12, f + 8, c0 + 6, ey + 12, f + 12, C15(1, 1, 2));
        sb(x, z, c0 - es - 6, ey + 34 + (P[7] - 50) / 3, f, c0 + es + 6, ey + 48 + (P[7] - 50) / 3, f + 7, shade(hc, 7)); }
      sb(x, z, -16 - (P[4] - 50) / 4, ey - 70, f, 16 + (P[4] - 50) / 4, ey - 6, f + 28 + (P[5] - 50) / 4, shade(sk, 8));
      int mw = 36 + (P[0] - 50) / 2, my = ey - 100 + (50 - P[1]) / 3; sb(x, z, -mw, my, f, mw, my + 16 + (P[17] - 50) / 6, f + 6, C15(21, 7, 8)); sb(x, z, -mw + 6, my + 15, f, mw - 6, my + 22, f + 5, C15(26, 12, 12));
      if (m->beard) sb(x, z, -hw + 8, hb, f - 4, hw - 8, ey - 60, f + 8, m->beard == 1 ? lit(sk, 10) : hc); }
    if (i == ctl) { int py = (hb + ht + 190) * sb_sc / 100; sb_sc = 100; sq_q = 0; bx(x - 40, py, z - 40, x + 40, py + 120, z + 40, C15(6, 30, 6)); bx(x - 30, py - 90, z - 30, x + 30, py, z + 30, C15(5, 22, 5)); }
}
// ---- Create-a-Sim: the head on the top screen, the choices on the bottom ----
static int cas_body;   // 1 when the Body sliders are showing: the camera pulls back to show the whole figure
static void draw_head(int yaw) {
    (void)yaw; const Sim *m = &sims[0]; u16 sk = skin_of(m), hc = hair_of(m), sh = shirt_of(m); int p[PMN]; eff_pm(m->pm, p);
    int hw = (m->fc == 1 ? 780 : 700) + (p[8] - 50) * 4, ht = (m->fc == 2 ? 1700 : 1500) + (p[9] - 50) * 6, hz = 700;
    int nl = (p[18] - 50) * 6, nw = 300 + (p[19] - 50) * 4, sw = 1150 + (p[14] - 50) * 8, th = 2200 + (p[15] - 50) * 20 + (p[26] - 50) * 12, bz = 450 + (p[24] - 50) * 6;
    int at = 350 + (p[21] - 50) * 4, al = 2200 + (p[20] - 50) * 20 + (p[26] - 50) * 8, hp = sw * 8 / 10 + (p[25] - 50) * 6, lh = 2800 + (p[22] - 50) * 25 + (p[26] - 50) * 20, lt = 300 + (p[23] - 50) * 4;
    int yt = -1000 - nl, yb = yt - th; u16 pants = C15(6, 7, 12);
    bx(-sw, yt, -520, sw, -300 - nl, 520, sh);                                    // shoulders
    bx(-sw * 8 / 10, yb, -450, sw * 8 / 10, yt, bz, sh);                          // torso (belly pushes the front out)
    bx(-sw - at, yt - al, -at, -sw, -300 - nl, at, sh); bx(sw, yt - al, -at, sw + at, -300 - nl, at, sh);   // arms
    bx(-sw - at, yt - al - 300, -at, -sw, yt - al, at, sk); bx(sw, yt - al - 300, -at, sw + at, yt - al, at, sk);   // hands
    bx(-hp, yb - 400, -430, hp, yb, bz, pants);                                   // hips
    bx(-hp, yb - 400 - lh, -lt, -40, yb - 400, lt, pants); bx(40, yb - 400 - lh, -lt, hp, yb - 400, lt, pants);   // legs
    bx(-hp, yb - 560 - lh, -lt, -40, yb - 400 - lh, lt + 250, C15(4, 4, 6)); bx(40, yb - 560 - lh, -lt, hp, yb - 400 - lh, lt + 250, C15(4, 4, 6));   // shoes
    bx(-nw, -330 - nl, -nw, nw, 20, nw, shade(sk, 7));                            // neck
    int hwj = hw + (p[10] - 50) * 3, ch = (p[11] - 50) * 6;
    bx(-hw, 0, -hz, hw, ht, hz, sk);                                              // head
    if (hwj > hw) bx(-hwj, 0, -hz * 9 / 10, hwj, ht * 3 / 10, hz * 9 / 10, sk);   // wide jaw
    if (ch > 0) bx(-hwj * 8 / 10, -ch, -hz * 8 / 10, hwj * 8 / 10, 40, hz * 8 / 10, sk);   // long chin
    { int es = 100 + (p[12] - 50) * 2, eh = ht * 4 / 10 + (p[13] - 50) * 4, ez = ht * 15 / 100 + (p[12] - 50) * 3;
      bx(-hw - es, eh - ez, -90, -hw, eh + ez, 160, shade(sk, 7)); bx(hw, eh - ez, -90, hw + es, eh + ez, 160, shade(sk, 7)); }   // ears
    int eu = ht * 6 / 10 + (p[3] - 50) * 5, er = (m->fc == 3 ? 100 : 70) + (p[16] - 50), ew = 150 + (p[16] - 50) * 2, zb = m->beard >= 4 ? 80 : 0;   // eyes
    for (int sd = -1; sd <= 1; sd += 2) { int c = sd * (280 + (p[2] - 50) * 3);
        fq(c - ew, eu - er, c + ew, eu + er, hz + 2, C15(31, 31, 31)); fq(c - 55, eu - er, c + 55, eu + er, hz + 4, C15(9, 6, 3)); fq(c - 28, eu - er + 15, c + 28, eu + er - 15, hz + 6, C15(2, 1, 2));
        fq(c - 40, eu + er - 45, c - 8, eu + er - 12, hz + 8, C15(31, 31, 31));
        if (m->fc == 4) fq(c - ew, eu, c + ew, eu + er, hz + 8, shade(sk, 7));                                     // sleepy lids
        { int ub = eu + er + 90 + (p[7] - 50) * 3, tl2 = (p[6] - 50) * 2, xo = c + sd * 170, xi = c - sd * 170; qd(xi, -(ub - tl2), hz + 3, xo, -(ub + tl2), hz + 3, xo, -(ub + 70 + tl2), hz + 3, xi, -(ub + 70 - tl2), hz + 3, shade(hc, 7)); }   // brow (tilt and height)
        if (m->fc == 5) for (int k = 0; k < 5; k++) fq(c - 130 + k * 55, eu - 250 - (k & 1) * 50, c - 100 + k * 55, eu - 220 - (k & 1) * 50, hz + 3, shade(sk, 5)); }
    { int nw2 = 90 + (p[4] - 50) * 2, np = 110 + (p[5] - 50) * 3; bx(-nw2, eu - 380, hz - 20, nw2, eu - 60, hz + np, shade(sk, 8)); }   // nose
    beard3d(m->beard, shade(hc, 9), sk, hw, hz);
    { int mw = 200 + (p[0] - 50) * 3, mv = ht * 2 / 10 + (p[1] - 50) * 5, lp = 70 + (p[17] - 50);   // lips
      fq(-mw, mv, mw, mv + lp, hz + 3 + zb, C15(21, 7, 8)); fq(-mw + 20, mv - lp * 7 / 10, mw - 20, mv, hz + 3 + zb, C15(26, 11, 11)); fq(-mw, mv + 10, mw, mv + 25, hz + 5 + zb, C15(5, 1, 2)); }
    hair3d(m->hair, hc, hw, ht);
}
static const int CAS_N[9] = { 12, 6, 6, 6, 6, 6, 0, 0, 0 };
static int cas_feat, cas_tgt, cas_rgb[3], cas_drag, cas_sp, cas_sg;
static const struct { const char *n; u8 i; } SLF[16] = { {"Mouth W",0},{"Mouth Y",1},{"Lips",17},{"Eye gap",2},{"Eye Y",3},{"Eye size",16},{"Nose W",4},{"Nose L",5},{"Brow tilt",6},{"Brow Y",7},{"Head W",8},{"Head H",9},{"Jaw W",10},{"Chin",11},{"Ear size",12},{"Ear Y",13} },
  SLB[11] = { {"Shldrs",14},{"Torso",15},{"Neck L",18},{"Neck W",19},{"Arm L",20},{"Arm W",21},{"Leg L",22},{"Leg W",23},{"Belly",24},{"Hips",25},{"Height",26} };
static int sl_n(void) { return cas_sg ? 11 : 16; }
static int sl_idx(int k) { int j = cas_sp * 6 + k; return j >= sl_n() ? -1 : (cas_sg ? SLB[j].i : SLF[j].i); }
static const char *sl_name(int k) { int j = cas_sp * 6 + k; return cas_sg ? SLB[j].n : SLF[j].n; }
static const char *const FEATN[8] = { "Mouth", "Eyes", "Nose", "Brows", "Head", "Jaw", "Ears", "Body" };
static const char *const FEATX[8] = { "Width", "Spacing", "Width", "Tilt", "Width", "Width", "Size", "Shoulders" }, *const FEATY[8] = { "Height", "Height", "Length", "Height", "Height", "Chin", "Height", "Torso" };
#define PADX 70
#define PADY 52
#define PADW 112
#define PADH 92
static void cas_load_rgb(void) { const Sim *m = &sims[0]; u16 c = cas_tgt == 0 ? skin_of(m) : cas_tgt == 1 ? hair_of(m) : shirt_of(m); cas_rgb[0] = c & 31; cas_rgb[1] = (c >> 5) & 31; cas_rgb[2] = (c >> 10) & 31; }
static void draw_cas(void) {   // the bottom screen: icon tabs down the left, then the choices, a pad to drag a feature on, or colour sliders; PLAY
    scr = scr_sub; backdrop();
    const char *tn[9] = { "Hair", "Color", "Face", "Beard", "Skin", "Shirt", "Pad", "Paint", "More" };
    panel(0, 4, 56, 184, 0);
    for (int i = 0; i < 9; i++) { int on = cas_tab == i; rect(5, 7 + i * 20, 48, 18, on ? C15(31, 31, 31) : C15(2, 5, 14)); rect(6, 8 + i * 20, 46, 16, on ? C15(8, 26, 8) : C15(4, 10, 24)); rect(6, 8 + i * 20, 46, 1, on ? C15(18, 31, 18) : C15(9, 16, 30)); rect(9, 12 + i * 20, 6, 8, i < 6 ? C15(31 - i * 3, 12 + i * 3, 10 + i * 4) : C15(31, 28, 8)); text(18, 12 + i * 20, 1, TXT, tn[i]); }
    static const char *ttl[9] = { "Hair Styles", "Hair Color", "Face", "Facial Hair", "Skin Tone", "Shirt", "Edit Face", "Paint", "More" };
    panel(62, 4, 190, 142, ttl[cas_tab]);
    Sim tmp = sims[0];
    if (cas_tab == 6) {   // drag a dot on the pad: it moves two things at once (like the Edit Mouth screen)
        for (int i = 0; i < 8; i++) { int fx = 66 + (i & 3) * 46, fy = 21 + (i >> 2) * 14; rect(fx, fy, 44, 12, cas_feat == i ? C15(10, 30, 10) : C15(4, 10, 24)); text(fx + 22 - tw(1, FEATN[i]) / 2, fy + 2, 1, TXT, FEATN[i]); }
        rect(PADX - 2, PADY - 2, PADW + 4, PADH + 4, PANB); rect(PADX, PADY, PADW, PADH, C15(30, 31, 31)); rect(PADX + PADW / 2, PADY, 1, PADH, C15(22, 26, 30)); rect(PADX, PADY + PADH / 2, PADW, 1, C15(22, 26, 30));
        int a = cas_feat * 2, dx = PADX + tmp.pm[a] * PADW / 100, dy = PADY + (100 - tmp.pm[a + 1]) * PADH / 100;
        rect(dx - 5, dy - 5, 11, 11, C15(3, 8, 24)); rect(dx - 3, dy - 3, 7, 7, C15(10, 18, 31));
        text(190, 56, 1, TXT, "Drag the"); text(190, 66, 1, TXT, "dot"); text(190, 82, 1, C15(22, 28, 31), FEATX[cas_feat]); text(190, 92, 1, C15(22, 28, 31), "left-right"); text(190, 106, 1, C15(22, 28, 31), FEATY[cas_feat]); text(190, 116, 1, C15(22, 28, 31), "up-down");
        rect(190, 128, 56, 14, C15(24, 10, 10)); text(200, 131, 1, TXT, "Reset");
    } else if (cas_tab == 7) {   // paint: pick what to colour, then drag three sliders
        const char *tg[3] = { "Skin", "Hair", "Shirt" };
        for (int i = 0; i < 3; i++) { rect(66 + i * 50, 24, 48, 16, cas_tgt == i ? C15(10, 30, 10) : C15(4, 10, 24)); text(72 + i * 50, 28, 1, TXT, tg[i]); }
        u16 col[3] = { C15(31, 8, 8), C15(8, 28, 8), C15(10, 14, 31) }; const char *cn[3] = { "R", "G", "B" };
        for (int k = 0; k < 3; k++) { int y = 52 + k * 24; text(68, y + 2, 1, TXT, cn[k]); rect(80, y, 128, 14, PANB); rect(82, y + 2, 124, 10, C15(2, 5, 12)); rect(82, y + 2, cas_rgb[k] * 124 / 31, 10, col[k]); rect(80 + cas_rgb[k] * 124 / 31, y - 2, 5, 18, C15(31, 31, 31)); }
        rect(214, 52, 34, 34, PANB); rect(216, 54, 30, 30, C15(cas_rgb[0], cas_rgb[1], cas_rgb[2]) | BIT(15)); rect(214, 96, 34, 14, C15(24, 10, 10)); text(218, 99, 1, TXT, "Reset");
        text(68, 128, 1, C15(22, 28, 31), "Drag a bar for any colour");
    } else if (cas_tab == 8) {   // every slider: Face or Body, six per page
        for (int i = 0; i < 2; i++) { rect(66 + i * 44, 22, 42, 13, cas_sg == i ? C15(10, 30, 10) : C15(4, 10, 24)); text(70 + i * 44, 25, 1, TXT, i ? "Body" : "Face"); }
        rect(156, 22, 44, 13, C15(24, 10, 10)); text(162, 25, 1, TXT, "Reset");
        rect(204, 22, 44, 13, cas_mult > 1 ? C15(31, 14, 4) : C15(14, 8, 24)); { char cb[8] = "Max x1"; cb[5] = (char)('0' + cas_mult); text(208, 25, 1, TXT, cb); }
        rect(206, 6, 20, 12, C15(8, 26, 8)); text(212, 9, 1, TXT, "<"); rect(228, 6, 20, 12, C15(8, 26, 8)); text(234, 9, 1, TXT, ">");
        for (int k = 0; k < 6; k++) { int q = sl_idx(k); if (q < 0) continue; int y = 40 + k * 17, v = tmp.pm[q];
            text(66, y + 3, 1, TXT, sl_name(k)); rect(124, y, 100, 13, PANB); rect(125, y + 1, 98, 11, C15(2, 5, 12)); rect(125, y + 1, v * 98 / 100, 11, C15(6, 18, 28)); rect(125 + 49, y + 1, 1, 11, C15(10, 14, 22));
            rect(121 + v * 98 / 100, y - 2, 6, 17, C15(31, 31, 31)); rect(122 + v * 98 / 100, y - 1, 4, 15, C15(10, 26, 10));
            char nb[8]; int nn = v - 50; nb[0] = nn < 0 ? '-' : '+'; nn = nn < 0 ? -nn : nn; nb[1] = (char)('0' + nn / 10); nb[2] = (char)('0' + nn % 10); nb[3] = 0; text(228, y + 3, 1, C15(22, 28, 31), nb); }
    } else {
        if (CAS_N[cas_tab] > 6) { rect(206, 6, 20, 12, C15(8, 26, 8)); text(212, 9, 1, TXT, "<"); rect(228, 6, 20, 12, C15(8, 26, 8)); text(234, 9, 1, TXT, ">"); }
        for (int k = 0; k < 6; k++) { int idx = k + (CAS_N[cas_tab] > 6 ? cas_page * 6 : 0); if (idx >= CAS_N[cas_tab]) continue;
            int x = 68 + (k % 3) * 60, y = 24 + (k / 3) * 58;
            int on = (cas_tab == 0 ? tmp.hair : cas_tab == 1 ? tmp.hcol : cas_tab == 2 ? tmp.fc : cas_tab == 3 ? tmp.beard : cas_tab == 4 ? tmp.skin : tmp.shirt) == idx;
            rect(x, y, 54, 54, on ? C15(10, 28, 8) : C15(28, 31, 31)); rect(x + 2, y + 2, 50, 50, C15(20, 26, 31));
            Sim v = tmp; if (cas_tab == 0) v.hair = (u8)idx; else if (cas_tab == 1) { v.hcol = (u8)idx; v.cc[1] = 0; } else if (cas_tab == 2) v.fc = (u8)idx; else if (cas_tab == 3) v.beard = (u8)idx; else if (cas_tab == 4) { v.skin = (u8)idx; v.cc[0] = 0; } else { v.shirt = (u8)idx; v.cc[2] = 0; }
            face(x + 9, y + 2, 36, &v);
            const char *nm = cas_tab == 0 ? HAIRN[idx] : cas_tab == 2 ? FACEN[idx] : cas_tab == 3 ? BEARDN[idx] : 0;
            if (nm) text(x + 27 - tw(1, nm) / 2, y + 42, 1, PANB, nm); } }
    rect(62, 152, 190, 34, PANB); rect(64, 154, 186, 30, C15(8, 22, 8)); text(128, 164, 2, TXT, "Play");
}
static void cas_apply_rgb(void) { sims[0].cc[cas_tgt] = C15(cas_rgb[0], cas_rgb[1], cas_rgb[2]) | BIT(15); }
static void cas_drag_at(int tx, int ty) {   // a stylus drag on the pad or a slider
    Sim *m = &sims[0];
    if (cas_tab == 6 && cas_drag == 1) { int a = cas_feat * 2, vx = (tx - PADX) * 100 / PADW, vy = 100 - (ty - PADY) * 100 / PADH; m->pm[a] = (u8)(vx < 0 ? 0 : vx > 100 ? 100 : vx); m->pm[a + 1] = (u8)(vy < 0 ? 0 : vy > 100 ? 100 : vy); }
    if (cas_tab == 8 && cas_drag >= 20) { int q = sl_idx(cas_drag - 20), v = (tx - 125) * 100 / 98; if (q >= 0) m->pm[q] = (u8)(v < 0 ? 0 : v > 100 ? 100 : v); }
    if (cas_tab == 7 && cas_drag >= 10 && cas_drag < 20) { int k = cas_drag - 10, v = (tx - 82) * 31 / 124; cas_rgb[k] = v < 0 ? 0 : v > 31 ? 31 : v; cas_apply_rgb(); }
}
static void cas_touch(int tx, int ty) {
    for (int i = 0; i < 9; i++) if (hit(5, 7 + i * 20, 48, 18, tx, ty)) { cas_tab = i; cas_page = 0; cas_sp = 0; if (i == 7) cas_load_rgb(); }
    if (CAS_N[cas_tab] > 6) { if (hit(206, 6, 20, 12, tx, ty)) cas_page = 0; if (hit(228, 6, 20, 12, tx, ty)) cas_page = 1; }
    if (cas_tab == 8) {
        if (hit(206, 6, 20, 12, tx, ty) && cas_sp > 0) cas_sp--; if (hit(228, 6, 20, 12, tx, ty) && (cas_sp + 1) * 6 < sl_n()) cas_sp++;
        for (int i = 0; i < 2; i++) if (hit(66 + i * 44, 22, 42, 13, tx, ty)) { cas_sg = i; cas_sp = 0; }
        if (hit(204, 22, 44, 13, tx, ty)) cas_mult = cas_mult % 3 + 1;
        if (hit(156, 22, 44, 13, tx, ty)) for (int j = 0; j < sl_n(); j++) sims[0].pm[cas_sg ? SLB[j].i : SLF[j].i] = 50;
        cas_drag = 0; for (int k = 0; k < 6; k++) if (sl_idx(k) >= 0 && hit(118, 38 + k * 17, 112, 17, tx, ty)) { cas_drag = 20 + k; cas_drag_at(tx, ty); }
    }
    cas_drag = 0;
    if (cas_tab == 6) {
        for (int i = 0; i < 8; i++) if (hit(66 + (i & 3) * 46, 21 + (i >> 2) * 14, 44, 12, tx, ty)) cas_feat = i;
        if (hit(PADX - 6, PADY - 6, PADW + 12, PADH + 12, tx, ty)) { cas_drag = 1; cas_drag_at(tx, ty); }
        if (hit(190, 128, 56, 14, tx, ty)) { sims[0].pm[cas_feat * 2] = 50; sims[0].pm[cas_feat * 2 + 1] = 50; }
    } else if (cas_tab == 7) {
        for (int i = 0; i < 3; i++) if (hit(66 + i * 50, 24, 48, 16, tx, ty)) { cas_tgt = i; cas_load_rgb(); }
        for (int k = 0; k < 3; k++) if (hit(76, 48 + k * 24, 140, 22, tx, ty)) { cas_drag = 10 + k; cas_drag_at(tx, ty); }
        if (hit(214, 96, 34, 14, tx, ty)) { sims[0].cc[cas_tgt] = 0; cas_load_rgb(); }
    } else for (int k = 0; k < 6; k++) if (hit(68 + (k % 3) * 60, 24 + (k / 3) * 58, 54, 54, tx, ty)) { int idx = k + (CAS_N[cas_tab] > 6 ? cas_page * 6 : 0); if (idx >= CAS_N[cas_tab]) continue; Sim *m = &sims[0];
        if (cas_tab == 0) m->hair = (u8)idx; else if (cas_tab == 1) { m->hcol = (u8)idx; m->cc[1] = 0; } else if (cas_tab == 2) m->fc = (u8)idx; else if (cas_tab == 3) m->beard = (u8)idx; else if (cas_tab == 4) { m->skin = (u8)idx; m->cc[0] = 0; } else { m->shirt = (u8)idx; m->cc[2] = 0; } }
    if (hit(62, 152, 190, 34, tx, ty)) cas_done = 1;
}
static int slot_sel;
static void house_icon(int x, int y, int sz, u16 c) {
    for (int i = 0; i < sz / 2; i++) rect(x + sz / 2 - i - 1, y + i, 2 * i + 2, 1, c);
    rect(x + sz / 8, y + sz / 2, sz * 3 / 4, sz / 2 - sz / 8, c); rect(x + sz * 2 / 5, y + sz * 3 / 4 - 1, sz / 5, sz / 4, C15(10, 22, 8));
}
static void plumb_icon(int x, int y, int sz, u16 c) {
    int cx = x + sz / 2, cy = y + sz / 2;
    for (int i = -sz / 3; i <= sz / 3; i++) { int w = (sz / 3 - (i < 0 ? -i : i)) * 2 / 3 + 1; rect(cx - w, cy + i, 2 * w + 1, 1, c); }
    for (int k = 0; k < 8; k++) { int dx = k < 4 ? (k & 1 ? 1 : -1) * (sz / 2 - 2) : 0, dy = k >= 4 ? ((k & 1) ? 1 : -1) * (sz / 2 - 2) : 0; rect(cx + dx - 1, cy + dy - 1, 3, 3, c); }
    rect(cx - sz / 2 + 2, cy, sz - 4, 1, c); rect(cx, cy - sz / 2 + 2, 1, sz - 4, c);
}
static const char *const OPN[14] = { "Anti-alias", "Outlines", "Shadows", "Lighting", "Item detail", "Ground check", "Frame rate", "Brightness", "Sky", "Camera", "Map spin", "Game speed", "Turbo CPU", "Reset all" };
static int *const OPV[14] = { &o_aa, &o_out, &o_shd, &o_lit, &o_det, &o_chk, &o_fps30, &o_bri, &o_sky, &o_cam, &o_spin, &o_spd, &o_turbo, 0 };
static const u8 OPM[14] = { 2, 2, 2, 2, 2, 2, 2, 9, 4, 3, 4, 3, 2, 1 };
static const char *opt_val(int i, char *b) {
    int v = OPV[i] ? *OPV[i] : 0;
    switch (i) { case 6: return v ? "30 fps" : "60 fps"; case 8: { static const char *n[4] = { "Day", "Sunset", "Night", "Mint" }; return n[v]; }
        case 9: { static const char *n[3] = { "Close", "Normal", "Far" }; return n[v]; } case 10: { static const char *n[4] = { "Off", "Slow", "Normal", "Fast" }; return n[v]; }
        case 11: { static const char *n[3] = { "1x", "2x", "3x" }; return n[v]; } case 7: b[0] = v < 4 ? '-' : '+'; b[1] = (char)('0' + (v < 4 ? 4 - v : v - 4)); b[2] = 0; return b;
        case 13: return "tap"; case 4: return v ? "Detailed" : "Simple"; default: return v ? "On" : "Off"; }
}
static void apply_opts(void) {
    if (o_aa) glEnable(GL_ANTIALIAS); else glDisable(GL_ANTIALIAS);
    if (o_out) { glEnable(GL_OUTLINE); glSetOutlineColor(0, RGB15(2, 2, 5)); } else glDisable(GL_OUTLINE);
    setBrightness(3, (o_bri - 4) * 3);
    { static const u8 sk[4][3] = { { 14, 20, 31 }, { 31, 18, 12 }, { 3, 4, 10 }, { 14, 28, 22 } }; glClearColor(sk[o_sky][0], sk[o_sky][1], sk[o_sky][2], 31); }
    if (isDSiMode()) setCpuClock(o_turbo ? true : false);
    speed = o_spd + 1; o_dirty = 0;
}
static void draw_opt(void) {   // the Options screen (bottom): tap a row to change it (right half forwards, left half back)
    scr = scr_sub; backdrop(); panel(6, 4, 244, 156, o_page ? "Options 2/2" : "Options 1/2");
    for (int k = 0; k < 7; k++) { int i = o_page * 7 + k; if (i >= 14) break; int y = 24 + k * 19; char b[4];
        rect(10, y, 236, 17, C15(5, 12, 26)); rect(10, y, 236, 1, C15(10, 18, 30)); text(16, y + 5, 1, TXT, OPN[i]);
        rect(150, y + 2, 92, 13, i == 13 ? C15(24, 10, 10) : OPV[i] && *OPV[i] ? C15(8, 26, 8) : C15(10, 14, 22)); { const char *v = opt_val(i, b); text(196 - tw(1, v) / 2, y + 5, 1, TXT, v); } }
    chip(10, 166, 60, 22, o_page ? "< Page" : "Page >", 0); chip(186, 166, 60, 22, "Back", 0);
}
static void opt_touch(int tx, int ty) {
    if (hit(10, 166, 60, 22, tx, ty)) { o_page ^= 1; return; }
    if (hit(186, 166, 60, 22, tx, ty)) { opt_on = 0; return; }
    for (int k = 0; k < 7; k++) { int i = o_page * 7 + k; if (i >= 14 || !hit(10, 24 + k * 19, 236, 17, tx, ty)) continue;
        if (i == 13) { o_aa = 1; o_out = 0; o_shd = 1; o_det = 1; o_fps30 = 0; o_bri = 4; o_sky = 0; o_chk = 1; o_spin = 2; o_turbo = 0; o_cam = 1; o_spd = 0; o_lit = 1; }
        else *OPV[i] = tx < 150 ? (*OPV[i] + OPM[i] - 1) % OPM[i] : (*OPV[i] + 1) % OPM[i];
        o_dirty = 1; }
}
static void draw_title(void) {
    scr = hud; backdrop(); panel(8, 8, 240, 176, "Household Occupants");
    const Sim *m = &sims[0]; rect(94, 34, 68, 68, C15(31, 31, 31)); face(98, 38, 60, m);
    const char *nm = sname[0]; char t[40]; sprintf(t, "%s", "Ashley"); text(128 - tw(1, t) / 2, 108, 1, TXT, t); text(128 - tw(1, "Newbie") / 2, 119, 1, TXT, "Newbie");
    house_icon(34, 140, 20, C15(30, 18, 12)); sprintf(t, "%d", funds); text(30, 164, 1, TXT, t);
    rect(112, 140, 20, 18, C15(31, 31, 31)); rect(114, 142, 16, 3, C15(28, 8, 8)); text(106, 164, 1, TXT, "1 day");
    rect(194, 140, 20, 20, C15(31, 31, 31)); rect(196, 142, 16, 16, C15(22, 26, 31)); rect(204, 144, 1, 7, C15(2, 4, 12)); rect(204, 151, 5, 1, C15(2, 4, 12));
    { int mn = minutes % 1440, h12 = (mn / 60) % 12 ? (mn / 60) % 12 : 12; sprintf(t, "%02d:%02d %s", h12, mn % 60, mn / 60 >= 12 ? "pm" : "am"); text(186, 164, 1, TXT, t); } (void)nm;
    if (opt_on) { draw_opt(); return; }
    scr = scr_sub; rect(0, 0, 256, 192, C15(3, 3, 4));
    rect(16, 40, 224, 118, PANB); rect(18, 42, 220, 114, C15(8, 16, 28)); rect(18, 42, 220, 16, PANH); text(26, 45, 2, TXT, "Start/Load profile");
    for (int i = 0; i < 3; i++) { int x = 26 + i * 70, y = 66; rect(x - 2, y - 2, 68, 60, slot_sel == i ? C15(31, 31, 31) : C15(2, 4, 12)); rect(x, y, 64, 56, i == 0 ? C15(10, 24, 8) : C15(6, 12, 26));
        if (i == 0) house_icon(x + 12, y + 8, 40, C15(31, 31, 31)); else plumb_icon(x + 8, y + 4, 48, C15(28, 31, 31)); }
    chip(98, 130, 28, 22, "<", 0); chip(130, 130, 28, 22, "Opt", 0); chip(162, 130, 28, 22, "Del", 0); chip(194, 130, 36, 22, "OK", 0);
    if (msg_t > 0) text(24, 166, 1, C15(31, 20, 12), msg);
}
static void new_household(void) {
    lot_seed = (u32)minutes * 7919u + 17; lot_reset(); lot_bx = -1; lot_by = -1; Sim *m = &sims[0]; m->skin = (u8)(minutes % 6); m->hair = (u8)(minutes % 12); m->hcol = (u8)(minutes % 6); m->shirt = (u8)(minutes % 6); m->beard = 0; m->fc = 0;
    for (int q = 0; q < PMN; q++) m->pm[q] = 50; m->cc[0] = m->cc[1] = m->cc[2] = 0;
}
static void title_touch(int tx, int ty) {
    if (opt_on) { opt_touch(tx, ty); return; }
    for (int i = 0; i < 3; i++) if (hit(26 + i * 70, 66, 64, 56, tx, ty)) slot_sel = i;
    if (hit(98, 130, 28, 22, tx, ty)) slot_sel = 0;
    if (hit(130, 130, 28, 22, tx, ty)) opt_on = 1;
    if (hit(162, 130, 28, 22, tx, ty)) { new_household(); say("Cleared"); slot_sel = 0; }
    if (hit(194, 130, 36, 22, tx, ty)) { if (slot_sel == 0) set_mode(M_MAP); else { new_household(); cas_tab = 0; cas_page = 0; slot_sel = 0; set_mode(M_CAS); } }
}
static void set_mode(int m) {
    mode = m;
    if (m == M_LIVE) { lcdMainOnBottom(); } else { lcdMainOnTop(); scr = hud; hud_clear(); }
}
static void live_touch(int tx, int ty) {
    if (popup) {
        if (popup == 1) { if (hit(64, 126, 62, 24, tx, ty)) { start_act(&sims[ctl], popup_i, -1); popup = 0; } else if (hit(132, 126, 62, 24, tx, ty)) popup = 0; return; }
        { int tg = popup == 2 ? popup_i : -2 - popup_i, n = soc_list(tg); if (hit(212, 60, 22, 14, tx, ty)) popup = 0;
          else for (int k = 0; k < n; k++) if (hit(26 + (k & 1) * 104, 82 + (k >> 1) * 22, 100, 20, tx, ty)) { start_soc(ctl, tg, soc_pick[k]); popup = 0; break; } }
        return; }
    if (menu_open) { menu_open = 0;
        if (hit(40, 150, 44, 24, tx, ty)) { set_mode(M_BUY); pick = -1; } else if (hit(86, 150, 44, 24, tx, ty)) { set_mode(M_BUILD); } else if (hit(132, 150, 44, 24, tx, ty)) set_mode(M_MAP); else if (hit(178, 150, 44, 24, tx, ty)) set_mode(M_TITLE);
        return; }
    if (hit(2, 2, 26, 26, tx, ty)) { set_mode(M_MAP); return; }
    if (hit(104, 178, 48, 14, tx, ty)) { menu_open = 1; return; }
    if (hit(62, 168, 30, 22, tx, ty)) { cam_on ^= 1; return; }
    if (hit(36, 168, 24, 22, tx, ty)) { speed = speed == 1 ? 2 : speed == 2 ? 3 : 1; return; }
    if (hit(0, 158, 34, 34, tx, ty)) { cur_sim = ctl; return; }
    for (int i = 0; i < nsim; i++) if (hit(132 + i * 21, 2, 20, 27, tx, ty)) { cur_sim = i; if (stage_of(sims[i].age) > 0) ctl = i; return; }
    int cx, cy; if (!pick_cell(tx, ty, &cx, &cy)) return;
    mark_x = cx; mark_y = cy; mark_t = 40;
    for (int i = 0; i < nsim; i++) { int sx = fdiv(sims[i].x + 8, 16), sy = fdiv(sims[i].y + 8, 16); if (sx == cx && sy == cy) { cur_sim = i; if (i != ctl) { popup = 2; popup_i = i; } return; } }
    for (int k = 0; k < NCIT; k++) { const Cit *c = &cits[k]; if (!cit_vis(c)) continue; int sx = fdiv(c->x + 8, 16), sy = fdiv(c->y + 8, 16); if (sx == cx && sy == cy) { popup = 3; popup_i = k; return; } }
    int o = obj_at(cx, cy); if (o >= 0) { popup = 1; popup_i = o; return; }
    if (cx >= -SLC && cy >= -SLC && cx < SLC * 2 && cy < SLC * 2) { cur_sim = ctl; sims[ctl].tx = cx * 16; sims[ctl].ty = cy * 16; sims[ctl].act = -1; sims[ctl].act_left = 0; }
}

int main(void) {
    powerOn(POWER_ALL_2D | POWER_3D_CORE | POWER_MATRIX); irqInit(); irqEnable(IRQ_VBLANK);
    videoSetMode(MODE_5_3D | DISPLAY_BG3_ACTIVE);
    videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    vramSetBankA(VRAM_A_TEXTURE); vramSetBankB(VRAM_B_TEXTURE); vramSetBankC(VRAM_C_SUB_BG); vramSetBankD(VRAM_D_MAIN_BG_0x06000000);
    int bg = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0); scr_sub = scr = bgGetGfxPtr(bg);
    int bgm = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0); hud = bgGetGfxPtr(bgm); bgSetPriority(bgm, 0);
    glInit(); glEnable(GL_ANTIALIAS); glClearColor(14, 20, 31, 31); glClearPolyID(63); glClearDepth(0x7FFF); glViewport(0, 0, 255, 191); REG_BG0CNT = BG_PRIORITY(1);
    bm_dials_default(); bm_new(seed); pop_build();
    cur_set = bm_capital(); town_pick(cur_set);
    lot_reset(); sims[0].skin = 2; sims[0].hair = 4; sims[0].hcol = 0; sims[0].shirt = 0; mode = M_TITLE; lcdMainOnTop(); hud_clear();
    int yaw = 0, dist = 13000, frames = 0, dirty = 1; unsigned lastsig = 0; int lastmode = -1;
    while (1) {
        scanKeys(); int held = keysHeld(), down = keysDown(); touchPosition tp = {0}; if (held & KEY_TOUCH) touchRead(&tp);
        const Settle *s = bm_set(cur_set); msg_t = msg_t > 0 ? msg_t - 1 : 0;
        unsigned sig = 0; int forced = (mode != lastmode); lastmode = mode;
        if (mode == M_TITLE) {
            if (down & KEY_TOUCH) title_touch(tp.px, tp.py);
            if (mode == M_TITLE && (forced || (down & KEY_TOUCH) || msg_t > 0)) draw_title();
            yaw = 0; pitch = 0; dist = 6000;
        } else if (mode == M_CAS) {
            if (down & KEY_TOUCH) cas_touch(tp.px, tp.py); else if ((held & KEY_TOUCH) && cas_drag) cas_drag_at(tp.px, tp.py);
            if (!(held & KEY_TOUCH)) cas_drag = 0;
            if (forced || (down & KEY_TOUCH) || cas_drag) draw_cas();
            if (cas_done) { cas_done = 0; set_mode(M_MAP); dirty = 1; }
            yaw = sinLerp(frames * 70) / 2; pitch = 0; dist = (cas_tab == 8 && cas_sg) || (cas_tab == 6 && cas_feat == 7) ? 19000 + (cas_mult - 1) * 6000 : 4700;
        } else if (mode == M_MAP) {
            if (down & (KEY_L | KEY_R)) { cur_set = (cur_set + ((down & KEY_R) ? 1 : bm_nset() - 1)) % bm_nset(); town_pick(cur_set); dirty = 1; }
            if (down & KEY_START) { seed = seed * 1103515245u + 12345 + frames; bm_new((u32)seed); pop_build(); lsv_n = 0; lsv_next = 0; nb_ok = 0; lot_bx = -1; cur_set = bm_capital(); town_pick(cur_set); dirty = 1; }
            if (down & KEY_LEFT) { sel_x--; dirty = 1; } if (down & KEY_RIGHT) { sel_x++; dirty = 1; }
            if (down & KEY_UP) { sel_y--; dirty = 1; } if (down & KEY_DOWN) { sel_y++; dirty = 1; }
            if (held & KEY_X) { if (dist > 3000) dist -= 60; } if (held & KEY_Y) { if (dist < 16000) dist += 60; }
            int enter = down & KEY_A, inmap = tp.py < MH * 8 && tp.px < MW * 8;
            int nx = s->x - MW / 2 + tp.px / 8, ny = s->y - MH / 2 + tp.py / 8;
            if (down & KEY_TOUCH) {
                if (tp.py >= MH * 8) { map_chip(tp.px, tp.py); dirty = 1; }
                else if (inmap) {
                    if (ne_on) { sel_x = nx; sel_y = ny; ne_cell(nx, ny); ne_last_x = nx; ne_last_y = ny; dirty = 1; }
                    else if (nx == sel_x && ny == sel_y) enter = 1; else { sel_x = nx; sel_y = ny; dirty = 1; } }
            } else if ((held & KEY_TOUCH) && inmap) {
                if (ne_on) { if (nx != ne_last_x || ny != ne_last_y) { sel_x = nx; sel_y = ny; ne_cell(nx, ny); ne_last_x = nx; ne_last_y = ny; dirty = 1; } }
                else if (nx != sel_x || ny != sel_y) { sel_x = nx; sel_y = ny; dirty = 1; } }
            if (!(held & KEY_TOUCH)) ne_last_x = -1;
            if (enter && !ne_on) { BInfo b; bm_block(sel_x, sel_y, &b); if (b.terrain == BT_SEA) say("Cannot build at sea"); else { if (lot_bx != sel_x || lot_by != sel_y) { Sim keep = sims[0]; lot_save(); lot_seed = (u32)(sel_x * 73856093 ^ sel_y * 19349663); lot_reset(); sims[0].skin = keep.skin; sims[0].hair = keep.hair; sims[0].hcol = keep.hcol; sims[0].shirt = keep.shirt; sims[0].beard = keep.beard; sims[0].fc = keep.fc; memcpy(sims[0].pm, keep.pm, PMN); memcpy(sims[0].cc, keep.cc, 6); lot_enter(sel_x, sel_y); city_spawn(); } else if (!nb_ok) { nb_load(); city_spawn(); } nb_ok = 1; cur_sim = 0; set_mode(M_LIVE); forced = 1; lastsig = ~lastsig; } }
            if (forced) dirty = 1;
            if (dirty) { scr = scr_sub; draw_map(); dirty = 0; }
            if (msg_t > 0) text(4, 150, 1, C15(31, 20, 12), msg);
            yaw += (o_spin == 0 ? 0 : o_spin == 1 ? 40 : o_spin == 2 ? 80 : 170); pitch = 3200;
        } else if (mode == M_LIVE) {
            if (cam_on) {   // stylus camera: drag to orbit/tilt (or pan), buttons for zoom, follow, reset, done
                if (down & KEY_TOUCH) { cam_drag = 0; if (hit(216, 36, 38, 20, tp.px, tp.py)) { cam_pan ^= 1; forced = 1; } else if (hit(216, 58, 38, 20, tp.px, tp.py)) cam_drag = 1;
                    else if (hit(216, 80, 38, 20, tp.px, tp.py)) cam_drag = 2; else if (hit(216, 102, 38, 20, tp.px, tp.py)) { cam_free ^= 1; forced = 1; }
                    else if (hit(216, 124, 38, 20, tp.px, tp.py)) { live_yaw = yaw_t = 4096; live_pitch = 4300; live_dist = 12000; cam_free = 0; forced = 1; }
                    else if (hit(216, 146, 38, 20, tp.px, tp.py)) { cam_on = 0; forced = 1; } else cam_drag = 3; cam_lx = tp.px; cam_ly = tp.py; }
                else if (held & KEY_TOUCH) { int ddx = tp.px - cam_lx, ddy = tp.py - cam_ly; cam_lx = tp.px; cam_ly = tp.py;
                    if (cam_drag == 1) { if (live_dist > 3500) live_dist -= 70; } else if (cam_drag == 2) { if (live_dist < 16000) live_dist += 70; }
                    else if (cam_drag == 3) { if (cam_pan) { int c = cosLerp(live_yaw), sn = sinLerp(live_yaw); cam_free = 1; camx += (-ddx * c + ddy * sn) * cam_deff / 4096 / 160; camz += (-ddx * sn - ddy * c) * cam_deff / 4096 / 160; }
                        else { live_yaw += ddx * 60; yaw_t = live_yaw; live_pitch += ddy * 30; if (live_pitch < 1800) live_pitch = 1800; if (live_pitch > 7600) live_pitch = 7600; } } }
            } else if (down & KEY_TOUCH) live_touch(tp.px, tp.py);
            if (down & KEY_L) yaw_t -= 4096; if (down & KEY_R) yaw_t += 4096;
            if (held & KEY_LEFT) { yaw_t -= 120; live_yaw = yaw_t; } if (held & KEY_RIGHT) { yaw_t += 120; live_yaw = yaw_t; }
            if (held & KEY_UP) { if (live_pitch < 7600) live_pitch += 50; } if (held & KEY_DOWN) { if (live_pitch > 1800) live_pitch -= 50; }
            if (down & KEY_SELECT) { cam_free ^= 1; if (!cam_free) { live_yaw = yaw_t = 4096; live_pitch = 4300; } }
            if (held & KEY_X) { if (live_dist > 3500) live_dist -= 80; } if (held & KEY_Y) { if (live_dist < 16000) live_dist += 80; }
            live_yaw += (yaw_t - live_yaw) / 4; if (yaw_t - live_yaw > -4 && yaw_t - live_yaw < 4) live_yaw = yaw_t;
            for (int k = 0; k < speed; k++) { sims_step(frames * speed + k); city_step(frames * speed + k); } lot_cross();
            int tx = sims[cur_sim].x * 2 * T / 16 - GN * T + T, tz = sims[cur_sim].y * 2 * T / 16 - GN * T + T; if (!cam_free) { camx += (tx - camx) / 8; camz += (tz - camz) / 8; }
            yaw = live_yaw; dist = live_dist * (o_cam == 0 ? 85 : o_cam == 1 ? 100 : 125) / 100; cam_deff = dist; pitch = live_pitch;
            sig = (unsigned)(cam_on * 3 + cam_pan * 5 + cam_free * 7 + cur_sim * 7 + minutes * 13 + popup * 17 + popup_i * 19 + menu_open * 23 + speed * 29 + msg_t * 31 + funds);
            for (int n = 0; n < 6; n++) sig = sig * 31 + (unsigned)sims[cur_sim].need[n]; sig = sig * 31 + (unsigned)sims[cur_sim].rel;
            if (mode == M_LIVE && (sig != lastsig || forced)) { draw_top_status(); draw_hud(); lastsig = sig; }
        } else {   // BUY / BUILD: the live lot on the top screen, the catalogue or plan on the bottom (drag to place, paint, draw walls, move)
            if (down & KEY_TOUCH) ui_down(tp.px, tp.py); else if (held & KEY_TOUCH) ui_hold(tp.px, tp.py);
            if (keysUp() & KEY_TOUCH) ui_up();
            if (mode == M_BUY && pick >= 0) {
                if (down & KEY_LEFT) pcx = pcx > 0 ? pcx - 1 : 0; if (down & KEY_RIGHT) pcx = pcx < GN - 1 ? pcx + 1 : GN - 1;
                if (down & KEY_UP) pcy = pcy > 0 ? pcy - 1 : 0; if (down & KEY_DOWN) pcy = pcy < GN - 1 ? pcy + 1 : GN - 1;
                if (down & KEY_R) prot = (prot + 1) & 3;
                if (down & KEY_A) place_here();
                if (down & KEY_B) pick = -1;
            } else if (mode == M_BUILD) {
                if (down & KEY_LEFT) pcx = pcx > 0 ? pcx - 1 : 0; if (down & KEY_RIGHT) pcx = pcx < GN - 1 ? pcx + 1 : GN - 1;
                if (down & KEY_UP) pcy = pcy > 0 ? pcy - 1 : 0; if (down & KEY_DOWN) pcy = pcy < GN - 1 ? pcy + 1 : GN - 1;
                if ((down & KEY_A) && tool == 0) { flr[pcy][pcx] = (u8)pal; ui_ver++; }
            }
            if (held & KEY_X) { if (bb_dist > 4500) bb_dist -= 120; } if (held & KEY_Y) { if (bb_dist < 24000) bb_dist += 120; }
            if (down & KEY_L) bb_yaw -= 4096; if (down & KEY_R) bb_yaw += 4096;
            if (held & KEY_TOUCH) { int px = tp.px, py = tp.py; for (int k = 0; k < 7; k++) { int cx = 164 + (k < 4 ? k * 22 : (k - 4) * 29), cy = k < 4 ? 164 : 178; if (hit(cx, cy, k < 4 ? 20 : 27, 12, px, py)) {
                if (k == 0) bb_yaw -= 90; else if (k == 1) bb_yaw += 90; else if (k == 2) { if (bb_pitch < 7200) bb_pitch += 50; } else if (k == 3) { if (bb_pitch > 2000) bb_pitch -= 50; }
                else if (k == 4) { if (bb_dist > 4500) bb_dist -= 120; } else if (k == 5) { if (bb_dist < 24000) bb_dist += 120; } else { bb_yaw = 3000; bb_pitch = 3900; bb_dist = 12500; } } } }
            sims_step(frames);
            if (mode != M_MAP) {
                sig = (unsigned)(mode * 7 + cat * 13 + pick * 17 + pcx * 31 + pcy * 37 + prot * 41 + tool * 43 + pal * 47 + funds * 59 + nobj * 61 + (msg_t > 0) * 71 + msg_t * 73 + forced + ui_ver * 79 + drag * 83);
                if (sig != lastsig || (down & KEY_TOUCH)) { scr = scr_sub; backdrop(); draw_ui(); lastsig = sig; } }
            yaw = bb_yaw; pitch = bb_pitch; dist = bb_dist * (o_cam == 0 ? 85 : o_cam == 1 ? 100 : 125) / 100;
        }
        if (o_dirty) apply_opts();
        polys = 0;
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspectivef32(degreesToAngle(40), 256 * 4096 / 192, 4096 * 3 / 4, 4096 * 12);
        glMatrixMode(GL_POSITION); glLoadIdentity();
        glTranslatef32(0, 0, -dist); glRotateXi(pitch); glRotateYi(yaw);
        if (mode == M_LIVE) glTranslatef32(-camx, 0, -camz); else if (mode == M_CAS) glTranslatef32(0, -650, 0);
        lit_on = o_lit && mode != M_TITLE && mode != M_MAP;
        glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(1) | (lit_on ? (POLY_FORMAT_LIGHT0 | POLY_FORMAT_LIGHT1) : 0));
        if (lit_on) { glMaterialf(GL_SPECULAR, RGB15(0, 0, 0)); glMaterialf(GL_EMISSION, RGB15(0, 0, 0));
            glLight(0, RGB15(27, 25, 21), floattov10(0.45f), floattov10(-0.55f), floattov10(-0.70f)); glLight(1, RGB15(7, 10, 17), floattov10(-0.7f), floattov10(0.1f), floattov10(0.7f)); }
        glBegin(GL_QUADS);
        if (mode == M_CAS) draw_head(yaw);
        else if (mode == M_TITLE) {}
        else if (mode == M_MAP) { BInfo b; bm_block(sel_x, sel_y, &b); draw_lot(&b, yaw); }
        else draw_build(yaw);
        glEnd();
        gframes = frames; glFlush(0); swiWaitForVBlank(); if (o_fps30) swiWaitForVBlank(); frames++;
    }
    return 0;
}
