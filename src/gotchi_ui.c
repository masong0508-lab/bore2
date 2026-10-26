// gotchi_ui.c - the BOREAGOTCHI! expansion pack's screens: the TOY BOX (whose toy), the EGG BASKET (up to eight creatures), the HANDHELD (the LCD
// and its ten icons), two little games, and the BLOBDEX. It is the overlay GOTCHI.OVL: a file on the disc of its own (ovl.h), so the pack can be
// patched or left out without touching the game. The creatures are not drawn from pictures: each of the 16 kinds is built from shapes on the
// 32 x 16 LCD, at four sizes and four grown-up forms, so there are 16 x 4 different grown-ups (and a BLOBDEX to fill).
#include "gotchi.h"
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "atlas.h"
#include "crowd.h"
#include "ovl.h"
#include <psxgpu.h>

enum { V_BOX, V_BASKET, V_HAND, V_DEX, V_FEED, V_PLAYPICK, V_LOOK, V_COPY, V_FRIEND, V_STATS, V_GUIDE };
static int view, from_box, ti, bi, isel, anim, msgT, dexpage, bxsel, sub, mg, mgT, mgN, mgWon, mgRound, mgDir;
static const char *msg; static int dexback, guidepage, guideback;
static u8 seq[6], sidx;
static u32 lrng;
static int rr(void) { lrng = lrng * 1103515245u + 12345u; return (int)((lrng >> 16) & 255); }

// ---- the 32 x 16 LCD, and the creatures drawn on it ----
static u8 dot[16][32]; static int minY;
static void P(int x, int y, int v) { if (x >= 0 && x < 32 && y >= 0 && y < 16) { dot[y][x] = (u8)v; if (v && y < minY) minY = y; } }
static void RC(int x0, int y0, int x1, int y1, int v) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) P(x, y, v); }
static void ELL(int cx, int cy, int rx, int ry, int v) { for (int y = -ry; y <= ry; y++) for (int x = -rx; x <= rx; x++) if (x * x * ry * ry + y * y * rx * rx <= rx * rx * ry * ry + rx * ry) P(cx + x, cy + y, v); }
static void LN(int x0, int y0, int x1, int y1, int v) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y1 - y0 : y0 - y1, sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx - dy;
    for (int n = 0; n < 40; n++) { P(x0, y0, v); if (x0 == x1 && y0 == y1) break; int e2 = 2 * e; if (e2 > -dy) { e -= dy; x0 += sx; } if (e2 < dx) { e += dx; y0 += sy; } }
}
static void star(int x, int y) { P(x, y, 1); P(x - 1, y, 1); P(x + 1, y, 1); P(x, y - 1, 1); P(x, y + 1, 1); }

static void creature(const Blob *b, int fr, int d, int ox) {
    static const u8 RX[5] = { 0, 3, 4, 5, 6 }, RY[5] = { 0, 3, 4, 4, 5 };
    int st = b->stage, sp = b->sp % GSP_N, rx = RX[st], ry = RY[st], bob = fr & 1, by = 14 - bob, cx = 16 + ox, cy, top, ex, eyY, big = st >= GS_TEEN;
    switch (sp) {   // the body, and what makes the kind
    case 0: cy = by - ry; ELL(cx, cy, rx, ry, 1); P(cx - rx / 2, by + 1, 1); P(cx + rx / 2, by + 1, 1); break;
    case 1: cy = by - ry; ELL(cx, cy, rx, ry, 1); top = cy - ry; P(cx - rx + 1, top - 1, 1); P(cx + rx - 1, top - 1, 1); if (st > 1) { P(cx - rx + 1, top - 2, 1); P(cx + rx - 1, top - 2, 1); }
        LN(cx - d * rx, cy + 1, cx - d * (rx + 2), cy - 2 + bob, 1); P(cx - 1, by + 1, 1); P(cx + 1, by + 1, 1); break;
    case 2: ry--; cy = by - ry; ELL(cx, cy, rx, ry, 1); top = cy - ry; { int h = 2 + st, w = st >= GS_TEEN ? 1 : 0; RC(cx - rx / 2 - 1 - w, top - h, cx - rx / 2 - 1, top - 1, 1); RC(cx + rx / 2 + 1, top - h + bob, cx + rx / 2 + 1 + w, top - 1, 1); }
        P(cx - d * (rx + 1), cy + ry - 1, 1); P(cx - 1, by + 1, 1); P(cx + 1, by + 1, 1); break;
    case 3: cy = by - ry; ELL(cx, cy, rx, ry, 1); top = cy - ry; P(cx + d * (rx + 1), cy, 1); if (st > 1) P(cx + d * (rx + 2), cy, 1); P(cx, top - 1, 1); P(cx + 1, top - 2, 1);
        LN(cx - d, cy, cx - d * (rx - 1), cy + ry - 2, 0); P(cx - 1, by + 1, 1); P(cx + 1, by + 1, 1); break;
    case 4: ry--; cy = by - ry; ELL(cx, cy, rx + 1, ry, 1); top = cy - ry; ELL(cx - rx / 2 - 1, top, 1, 1, 1); ELL(cx + rx / 2 + 1, top, 1, 1, 1);
        RC(cx - rx + 1, cy + 1, cx + rx - 1, cy + 1, 0); if (bob) P(cx + d * rx, cy + 1, 1); P(cx - rx, by + 1, 1); P(cx + rx, by + 1, 1); break;
    case 5: cy = by - ry; ELL(cx, cy, rx, ry, 1); top = cy - ry; for (int i = 0; i <= rx / 2; i++) P(cx - d * i * 2, top - 1, 1);
        LN(cx - d * rx, cy + ry - 1, cx - d * (rx + 3), cy + ry - 3 + bob, 1); P(cx + d * (rx + 1), cy - 1, 1); P(cx + d * rx, cy + 2, 1); P(cx - 1, by + 1, 1); P(cx + 1, by + 1, 1); break;
    case 6: ry--; cy = by - ry - 1 + bob; ELL(cx, cy, rx + 1, ry, 1); top = cy - ry; LN(cx - d * (rx + 1), cy, cx - d * (rx + 4), cy - 2, 1); LN(cx - d * (rx + 1), cy, cx - d * (rx + 4), cy + 2, 1); RC(cx - d * (rx + 3), cy - 1, cx - d * (rx + 3), cy + 1, 1);
        P(cx, top - 1, 1); P(cx - d, top - 1, 1); P(cx + d * (rx + 1), cy + 1, 0); break;
    case 7: cy = by - ry; ELL(cx, cy, rx, ry, 1); top = cy - ry; ELL(cx - rx + 1, top, 1, 1, 1); ELL(cx + rx - 1, top, 1, 1, 1); P(cx - rx - 1, cy + 2, 1); P(cx + rx + 1, cy + 2, 1); P(cx - 1, by + 1, 1); P(cx + 1, by + 1, 1); break;
    case 8: cy = by - ry; ELL(cx, cy - 1, rx, ry - 1, 1); RC(cx - rx, cy - 1, cx + rx, by - 1, 1); for (int x = -rx; x <= rx; x++) if (((x + fr) & 1) == 0) P(cx + x, by, 1); P(cx - rx - 1, cy + bob, 1); P(cx + rx + 1, cy + 1 - bob, 1); break;
    case 9: cy = by - ry; RC(cx - rx, cy - ry, cx + rx, by - 1, 1); P(cx - rx, cy - ry, 0); P(cx + rx, cy - ry, 0); top = cy - ry; LN(cx, top - 1, cx, top - 2, 1); P(cx + bob, top - 3, 1);
        RC(cx - rx + 1, by, cx - 1, by + 1, 1); RC(cx + 1, by, cx + rx - 1, by + 1, 1); RC(cx - rx - 1, cy - 1, cx - rx - 1, cy + 2, 1); RC(cx + rx + 1, cy - 1, cx + rx + 1, cy + 2, 1); break;
    case 10: { int capry = ry / 2 + 1; cy = by - ry / 2; RC(cx - rx / 2, by - ry, cx + rx / 2, by, 1); ELL(cx, by - ry, rx, capry, 1); P(cx - rx / 2, by - ry - 1, 0); P(cx + rx / 2, by - ry - 1, 0); if (st > 1) P(cx, by - ry - capry + 1, 0); RC(cx - rx / 2 - 1, by + 1 - 1, cx - rx / 2 - 1, by, 1); break; }
    case 11: cy = by - ry / 2; ELL(cx, by, rx + 1 + bob, ry, 1); for (int y = by + 1; y < 16; y++) for (int x = 0; x < 32; x++) dot[y][x] = 0; P(cx - rx, by + 1, 1); if (fr & 2) P(cx + rx - 1, by + 1, 1); break;
    case 12: cy = by - ry; ELL(cx, cy, rx - 1, ry - 1, 1); top = cy - ry + 1; P(cx - rx + 2, top - 1, 1); P(cx + rx - 2, top - 1, 1);
        for (int i = 1; i <= 4; i++) { int w = bob ? -2 : 1; RC(cx - rx + 1 - i, cy - 2 + (bob ? 0 : 2) + i / 2 + w / 2, cx - rx + 1 - i, cy + 1 + i / 3, 1); RC(cx + rx - 1 + i, cy - 2 + (bob ? 0 : 2) + i / 2 + w / 2, cx + rx - 1 + i, cy + 1 + i / 3, 1); } break;
    case 13: cy = by - ry; RC(cx - rx, by - 1, cx + rx, by, 1); ELL(cx + d * rx, by - 2, 1, 1, 1); ELL(cx - d, by - ry + 1, ry - 1 > 1 ? ry - 1 : 2, ry - 1 > 1 ? ry - 1 : 2, 1); P(cx - d, by - ry + 1, 0); P(cx - d + 1, by - ry + 1, 0); P(cx - d + 1, by - ry + 2, 0);
        LN(cx + d * rx, by - 3, cx + d * rx, by - 5, 1); LN(cx + d * (rx - 1), by - 3, cx + d * (rx - 1), by - 5 + bob, 1); cy = by - 2; break;
    case 14: ry--; cy = by - ry - 1; ELL(cx, cy, rx, ry, 1); for (int i = 0; i < 4; i++) { int x = cx - rx + 1 + i * (2 * rx - 2) / 3; for (int y = cy + ry; y <= by + 1; y++) P(x + (((fr + i + y) & 2) ? 1 : 0), y, 1); } break;
    default: cy = by - ry; ELL(cx, cy - 1, rx, ry - 1, 1); RC(cx - 1, cy + ry - 2, cx + 1, by, 1); top = cy - ry; LN(cx - 2, top + 1, cx - 4, top - 2, 1); LN(cx + 2, top + 1, cx + 4, top - 2 + bob, 1); P(cx - 4, top - 3, 1); P(cx + 4, top - 3 + bob, 1); break;
    }
    // face
    ex = rx / 2 > 0 ? rx / 2 : 1; eyY = cy - ry / 3; if (sp == 4 || sp == 6 || sp == 11) eyY = cy - 1; if (sp == 10) eyY = by - ry + 1; if (sp == 13) eyY = by - 4; if (sp == 12 || sp == 15) eyY = cy - 1;
    int e1 = cx + d - ex, e2 = cx + d + ex; if (sp == 6) { e1 = e2 = cx + d * (rx - 1); } if (sp == 13) { e1 = e2 = cx + d * rx; }
    int chill = st == GS_ADULT && b->form == 2, sleepy = (b->fl & GF_SLEEP) != 0;
    if (sp == 9) { P(e1, eyY, 0); P(e2, eyY, 0); if (big) { P(e1 + 1, eyY, 0); P(e2 + 1, eyY, 0); } }
    else if (sleepy || chill) { P(e1, eyY, 0); P(e2, eyY, 0); if (e1 != e2 && big) { P(e1 - 1, eyY, 0); P(e2 + 1, eyY, 0); } }
    else { P(e1, eyY, 0); P(e2, eyY, 0); if (big || sp == 15) { P(e1, eyY + 1, 0); P(e2, eyY + 1, 0); } if (sp == 15) { P(e1 - 1, eyY, 0); P(e2 + 1, eyY, 0); } }
    if (sp != 4 && sp != 6) {
        int my = cy + ry / 2 + (sp == 10 ? 1 : 0); if (sp == 13) my = by - 2;
        if (b->hun == 0 || b->hap == 0 || (b->fl & GF_SICK)) { P(cx + d, my, 0); P(cx + d - 1, my + 1, 0); P(cx + d + 1, my + 1, 0); }
        else if (b->hap >= 3 && big) { P(cx + d - 1, my, 0); P(cx + d, my + 1, 0); P(cx + d + 1, my, 0); }
        else P(cx + d, my, 0);
    }
    // the form: what looking after it made of it
    if (st == GS_TEEN) { if (b->form == 0) { P(e1 - 1, eyY - 1, 0); P(e2 + 1, eyY - 1, 0); } else P(cx + rx + 1, cy - ry + 1, 1); }
    if (st == GS_ADULT) switch (b->form) {
        case 0: RC(cx - 2, minY - 1, cx + 2, minY - 1, 1); P(cx - 2, minY - 2, 1); P(cx, minY - 2, 1); P(cx + 2, minY - 2, 1); if (fr & 2) star(cx + rx + 3, cy - ry); break;
        case 1: P(cx - rx - 2 + ((fr >> 1) & 3), minY - 1, 1); P(cx + rx + 2 - ((fr >> 1) & 3), minY, 1); P(e1, eyY + 2, 0); P(e2, eyY + 2, 0); break;
        case 2: P(cx + rx + 2, cy - 2, 1); P(cx + rx + 3, cy - 3, 1); P(cx + rx + 3, cy - 1, 1); if (fr & 2) P(cx + rx + 4, cy - 4, 1); break;
        default: star(cx, minY - 3); star(cx - rx - 3, cy); if (fr & 2) star(cx + rx + 3, cy - ry + 1); break;
    }
}
static void egg(const Blob *b, int fr) {
    int bob = fr & 1, cy = 10 - bob; ELL(16, cy, 4, 5, 1);
    switch (b->sp & 3) { case 0: P(14, cy - 2, 0); P(18, cy, 0); P(15, cy + 2, 0); P(17, cy - 3, 0); break; case 1: RC(13, cy - 1, 19, cy - 1, 0); RC(13, cy + 2, 19, cy + 2, 0); break;
        case 2: for (int x = 12; x < 21; x++) P(x, cy + ((x & 1) ? 0 : 1), 0); break; default: ELL(14, cy - 1, 1, 1, 0); ELL(18, cy + 2, 1, 1, 0); break; }
    if (b->age >= 15) { LN(16, cy - 5, 14, cy - 2, 0); LN(14, cy - 2, 17, cy, 0); LN(17, cy, 15, cy + 3, 0); }
}
static void scene(const Blob *b, int fr, int look) {
    for (int y = 0; y < 16; y++) for (int x = 0; x < 32; x++) dot[y][x] = 0;
    minY = 16;
    if (!b->on) return;
    int d = look ? look : ((fr >> 1) & 1 ? -1 : 1), ox = look ? 0 : (((fr >> 1) % 3) - 1) * 2;
    if (b->stage == GS_EGG) egg(b, fr); else creature(b, fr, d, ox);
    static const u8 pp[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 1, 0 }, { 1, 1, 1, 1 } };
    for (int i = 0; i < b->poop && i < 3; i++) { int px = 28 - (i & 1) * 4, py = 12 - (i >> 1) * 4; for (int r = 0; r < 4; r++) for (int q = 0; q < 4; q++) if (pp[r][q]) P(px + q - 1, py + r, 1); }
    if (b->fl & GF_SICK) { static const char *sk[5] = { ".###.", "#.#.#", "#####", ".###.", ".#.#." }; for (int r = 0; r < 5; r++) for (int q = 0; q < 5; q++) if (sk[r][q] == '#') P(1 + q, 1 + r, 1); }
    if (b->fl & GF_SLEEP) { static const char *z[5] = { "###.", "..#.", ".#..", "###.", "...." }; for (int r = 0; r < 5; r++) for (int q = 0; q < 4; q++) if (z[r][q] == '#') P(24 + q + ((fr >> 2) & 1), 1 + r, 1); }
}
// the dots onto the screen: at a small size runs of them are joined into one box
static void dots_draw(int x, int y, int s, int r, int g, int bl) {
    for (int j = 0; j < 16; j++) {
        if (s >= 3) { for (int i = 0; i < 32; i++) if (dot[j][i]) gfx_rect(x + i * s, y + j * s, s - 1, s - 1, r, g, bl, 0); }
        else for (int i = 0; i < 32;) { if (!dot[j][i]) { i++; continue; } int k = i; while (k < 32 && dot[j][k]) k++; gfx_rect(x + i * s, y + j * s, (k - i) * s, s, r, g, bl, 0); i = k; }
    }
}
static const u32 shell[6][3] = { { 236, 110, 170 }, { 60, 190, 190 }, { 240, 150, 50 }, { 150, 100, 220 }, { 130, 200, 70 }, { 80, 130, 230 } };

// ---- the icons ----
static const char *const iconNm[10] = { "FEED", "LIGHT", "PLAY", "MEDICINE", "CLEAN", "STATS", "SCOLD", "ATTENTION", "FRIENDS", "BLOBDEX" };
static const char *const icon[10][7] = {
    { "...#.....", "..#.##...", ".#####...", "#######..", "#######..", ".#####...", "........." },
    { "...###...", "..#...#..", "..#...#..", "...#.#...", "...###...", "...###...", "........." },
    { ".#######.", "#..#....#", "#.###.#.#", "#..#..#.#", "#.......#", ".#######.", "........." },
    { "...###...", "...#.#...", ".###.###.", ".#.....#.", ".###.###.", "...#.#...", "...###..." },
    { "..#.#.#..", "...#.#...", "..#####..", "...###...", "...###...", "..#####..", "........." },
    { "#........", "#..#.....", "#..#..#..", "#.##..#..", "#.##.##..", "#########", "........." },
    { ".#####...", ".#...#...", ".#.#.#...", ".#####...", "...#.....", "...###...", "........." },
    { "...#.....", "..###....", "..###....", ".#####...", ".#####...", "#######..", "...#....." },
    { ".##...##.", "#..#.#..#", ".##...##.", "..#...#..", ".###.###.", "#..#.#..#", "........." },
    { "..#####..", ".#.....#.", "#.##.##.#", "#.#...#.#", "#.##.##.#", ".#.....#.", "..#####.." } };
static void icon_draw(int i, int x, int y, int on, int lit) {
    u32 c = on ? 0xFFFFFF : lit ? 0x28CEF8 : 0x8A4CA8;
    if (on) gfx_rect(x - 3, y - 3, 24, 20, 255, 255, 255, 0);
    gfx_rect(x - (on ? 2 : 3), y - (on ? 2 : 3), on ? 22 : 24, on ? 18 : 20, 60, 20, 70, 0);
    for (int r = 0; r < 7; r++) for (int q = 0; q < 9; q++) if (icon[i][r][q] == '#') gfx_rect(x + q * 2, y + r * 2, 2, 2, c & 255, (c >> 8) & 255, (c >> 16) & 255, 0);
}

// ---- helpers ----
static void lt(int x, int y, const char *s) { gfx_text_raw(x, y, 1, s, 28, 44, 24); }
static void ltc(int cx, int y, const char *s) { gfx_text_raw(cx - gfx_text_w(1, s) / 2, y, 1, s, 28, 44, 24); }
static Toy *tt(void) { return &gtoy[ti]; }
static Blob *cb(void) { return &tt()->b[bi]; }
static int owner_color(const Toy *t) { return t->uid % 6; }
static void say(const char *s) { msg = s; msgT = 150; }
static int living(Toy *t, int from, int dir) { for (int k = 1; k <= GB_N; k++) { int j = (from + dir * k + GB_N * 2) % GB_N; if (t->b[j].on) return j; } return from; }
static void draw_title(int x, int y, const char *s) { ui_text(x, y, 1, s, UI_INK); }

// the toy box lists the family's toys
static int box_list(int *idx) { int n = 0; for (int i = 0; i < nsims && n < GT_MAX; i++) idx[n++] = i; return n; }
static void open_basket(int simi) { Toy *t = gotchi_toy(simi); if (!t) { say("NO BOREAGOTCHI YET. ELECTRONICS SHOPS SELL THEM"); return; } ti = (int)(t - gtoy); view = V_BASKET; bi = t->sel < GB_N ? t->sel : 0; msgT = 0; }
static const u8 *art; static int art_page = -1;
static void guide_open(int back) {
    u32 n; u8 *base = (u8 *)crowd_scratch(&n) + OVL_AT; int room = (int)n - OVL_AT - 53248;
    art = 0; art_page = -1; if (room >= 8 * 72 * 54 * 2 && audio_cd_load("GOTCHI.ART", base + 53248, room)) art = base + 53248;
    guidepage = 0; guideback = back; view = V_GUIDE;
}
void gotchi_open(int arg) {
    lrng = 0xB0BE5u + (u32)clock_min * 7; anim = 0; msgT = 0; msg = 0; dexpage = 0; bxsel = player; isel = 0; from_box = 0; sub = 0;
    if (arg == 3) { guide_open(-1); return; }
    if (arg == 2) { view = V_DEX; dexback = -1; return; }
    if (arg == 1 && gotchi_toy(player)) { open_basket(player); return; }
    view = V_BOX; from_box = 1;
}

static void feed_menu(int pr) {
    if (pr & PAD_RIGHT) sub = (sub + 1) % 3; if (pr & PAD_LEFT) sub = (sub + 2) % 3;
    if (pr & PAD_CIRCLE) { view = V_HAND; return; }
    if (pr & PAD_CROSS) { say(sub == 2 ? gotchi_lunch(tt(), cb()) : gotchi_feed(tt(), cb(), sub)); view = V_HAND; audio_voice(VOICE_YAHOO); }
}
static void start_look(void) { view = V_LOOK; mgRound = 0; mgWon = 0; mgT = 0; mgN = 0; mgDir = 0; }
static void start_copy(void) { view = V_COPY; mgRound = 0; mgWon = 0; mgN = 2; for (int i = 0; i < 6; i++) seq[i] = (u8)(rr() & 3); sidx = 0; mgT = -60; }
static void finish_game(int won) { view = V_HAND; say(gotchi_game(tt(), cb(), won)); audio_voice(won ? VOICE_YAHOO : VOICE_NAH); }

static void use_icon(void) {
    Blob *b = cb(); Toy *t = tt(); int sl = (b->fl & GF_SLEEP) != 0;
    if (b->stage == GS_EGG && isel != 5 && isel != 9) { say("IT IS STILL AN EGG. GIVE IT A MINUTE"); return; }
    if (sl && isel != 1 && isel != 5 && isel != 3 && isel != 9) { say("SHHH. IT IS ASLEEP"); return; }
    switch (isel) {
    case 0: sub = 0; view = V_FEED; break;
    case 1: say(gotchi_light(t, b)); break;
    case 2: sub = 0; view = V_PLAYPICK; break;
    case 3: say(gotchi_medicine(t, b)); break;
    case 4: say(gotchi_clean(t, b)); break;
    case 5: sub = 0; view = V_STATS; break;
    case 6: say(gotchi_scold(t, b)); break;
    case 7: say(gotchi_needs(b) ? gotchi_want(b) : gotchi_nuzzle(t, b)); break;
    case 8: { int n = 0; for (int i = 0; i < GB_N; i++) if (i != bi && t->b[i].on && t->b[i].stage > GS_EGG) n++; if (!n) { say("YOU NEED A SECOND ONE, HATCHED, TO PLAY WITH"); return; }
              sub = living(t, bi, 1); view = V_FRIEND; break; }
    default: dexpage = 0; view = V_DEX; dexback = V_HAND; break;
    }
}

// ---- update ----
int gotchi_update(int hd, int pr, int steps) {
    gotchi_live(steps); anim += steps; if (msgT > 0) { msgT -= steps; if (msgT <= 0) { msgT = 0; msg = 0; } }
    switch (view) {
    case V_BOX: {
        int idx[GT_MAX], n = box_list(idx); if (!n) return 0;
        if (pr & PAD_UP) bxsel = (bxsel + n - 1) % n; if (pr & PAD_DOWN) bxsel = (bxsel + 1) % n;
        if (pr & PAD_TRIANGLE) { view = V_DEX; dexpage = 0; dexback = V_BOX; }
        if (pr & PAD_SELECT) guide_open(V_BOX);
        if (pr & PAD_CIRCLE) return 0;
        if (pr & PAD_CROSS) { open_basket(bxsel); if (view == V_BASKET) from_box = 1; }
        break; }
    case V_BASKET: {
        Toy *t = tt();
        if (pr & PAD_LEFT) bi = (bi + GB_N - 1) % GB_N; if (pr & PAD_RIGHT) bi = (bi + 1) % GB_N;
        if (pr & PAD_UP) bi = (bi + GB_N - 4) % GB_N; if (pr & PAD_DOWN) bi = (bi + 4) % GB_N;
        t->sel = (u8)bi;
        if (pr & PAD_TRIANGLE) { view = V_DEX; dexpage = 0; dexback = V_BASKET; }
        if (pr & PAD_SELECT) guide_open(V_BASKET);
        if (pr & PAD_CIRCLE) { if (from_box) { view = V_BOX; from_box = 1; } else return 0; }
        if (pr & PAD_CROSS) {
            if (gotchi_sim_of(t) != player) { say("THAT IS NOT YOUR TOY. YOU CAN ONLY LOOK"); break; }
            if (!t->b[bi].on) { if (cash < GOTCHI_EGG) { say("A NEW EGG COSTS $150"); audio_voice(VOICE_NAH); } else { cash -= GOTCHI_EGG; gotchi_egg(t, bi); say("A NEW EGG! KEEP IT CLOSE"); audio_voice(VOICE_YAHOO); } }
            else { view = V_HAND; isel = 0; msgT = 0; }
        }
        break; }
    case V_HAND: {
        Toy *t = tt();
        if (pr & PAD_RIGHT) isel = (isel + 1) % 10; if (pr & PAD_LEFT) isel = (isel + 9) % 10;
        if (pr & (PAD_UP | PAD_DOWN)) isel = (isel + 5) % 10;
        if (pr & PAD_R1) bi = living(t, bi, 1); if (pr & PAD_L1) bi = living(t, bi, -1);
        t->sel = (u8)bi;
        if (pr & PAD_CIRCLE) { view = V_BASKET; msgT = 0; }
        if (pr & PAD_CROSS) use_icon();
        break; }
    case V_FEED: feed_menu(pr); break;
    case V_PLAYPICK:
        if (pr & (PAD_LEFT | PAD_RIGHT)) sub ^= 1;
        if (pr & PAD_CIRCLE) view = V_HAND;
        if (pr & PAD_CROSS) { if (sub == 0) start_look(); else start_copy(); }
        break;
    case V_LOOK:   // WHICH WAY? it looks left or right: guess first. 5 rounds, 3 to win
        if (mgN == 0) { if (pr & PAD_LEFT) { mgDir = -1; mgN = 1; mgT = 0; } if (pr & PAD_RIGHT) { mgDir = 1; mgN = 1; mgT = 0; } if (pr & PAD_CIRCLE) view = V_HAND;
            if (mgN) { sidx = (u8)((rr() & 1) ? 1 : 0); } }
        else { mgT += steps; if (mgT > 50) { int way = sidx ? 1 : -1; if (way == mgDir) mgWon++; mgRound++; mgN = 0; if (mgRound >= 5) finish_game(mgWon >= 3); } }
        break;
    case V_COPY: {   // COPY ME: watch the blob, then do the same moves (the D-pad). Three rounds, two to win
        if (pr & PAD_CIRCLE) { view = V_HAND; break; }
        mgT += steps;
        if (mgT < 0) break;
        if (mgT < mgN * 40) break;                       // still showing the moves
        if (mgT == mgN * 40 || sidx == 0) { }
        int key = (pr & PAD_UP) ? 0 : (pr & PAD_DOWN) ? 1 : (pr & PAD_LEFT) ? 2 : (pr & PAD_RIGHT) ? 3 : -1;
        if (key >= 0) { if (key == seq[sidx]) { sidx++; audio_voice(VOICE_YEHA); if (sidx >= mgN) { mgWon++; mgRound++; sidx = 0; mgN++; mgT = -50; if (mgRound >= 3) finish_game(1); } }
                        else { mgRound++; sidx = 0; mgT = -50; audio_voice(VOICE_NAH); if (mgRound - mgWon >= 2) finish_game(0); else if (mgRound >= 3) finish_game(mgWon >= 2); } }
        break; }
    case V_FRIEND: {
        Toy *t = tt();
        if (pr & PAD_RIGHT) sub = living(t, sub, 1); if (pr & PAD_LEFT) sub = living(t, sub, -1);
        while (sub == bi) sub = living(t, sub, 1);
        if (pr & PAD_CIRCLE) view = V_HAND;
        if (pr & (PAD_CROSS | PAD_SQUARE)) { const char *m = gotchi_friends(t, bi, sub, (pr & PAD_SQUARE) != 0); view = V_HAND; say(m); audio_voice(VOICE_YAHOO); }
        break; }
    case V_STATS:
        if (pr & (PAD_CROSS | PAD_RIGHT | PAD_DOWN)) { if (++sub >= 2) view = V_HAND; }
        if (pr & (PAD_CIRCLE | PAD_LEFT)) { if (sub > 0) sub--; else view = V_HAND; }
        break;
    case V_GUIDE:
        if (pr & (PAD_R1 | PAD_RIGHT | PAD_CROSS)) guidepage = (guidepage + 1) % 9; if (pr & (PAD_L1 | PAD_LEFT)) guidepage = (guidepage + 8) % 9;
        if (pr & PAD_CIRCLE) { if (guideback < 0) return 0; view = guideback; }
        break;
    case V_DEX:
        if (pr & (PAD_R1 | PAD_RIGHT)) dexpage = (dexpage + 1) & 3; if (pr & (PAD_L1 | PAD_LEFT)) dexpage = (dexpage + 3) & 3;
        if (pr & PAD_CIRCLE) { if (dexback < 0) return 0; view = dexback; }
        break;
    }
    return 1;
}

// ---- draw ----
#define IW 72
#define IH 54
#define NIMG 8
static const u32 C_INK = UI_RGB(28, 44, 24);
static u32 hsh(u32 a) { a ^= a << 13; a ^= a >> 17; a ^= a << 5; return a * 2654435761u; }
static void tshadow(int x, int y, const char *s, int r, int g, int b) { gfx_text_raw(x + 1, y + 1, 1, s, 20, 8, 30); gfx_text_raw(x, y, 1, s, r, g, b); }
static void tshadowc(int cx, int y, const char *s, int r, int g, int b) { tshadow(cx - gfx_text_w(1, s) / 2, y, s, r, g, b); }
static void chip(int cx, int y, const char *s, u32 bg, u32 ink) { int w = ui_tw(1, s) + 10; ui_badge(cx - w / 2, y, s, bg, ink); }
static void formchip(const Blob *b, int cx, int y) {
    static const u32 fc[4] = { UI_RGB(236, 176, 40), UI_RGB(150, 112, 70), UI_RGB(60, 190, 210), UI_RGB(220, 70, 180) };
    static const char *const fn[4] = { "SUPER", "SLOB", "CHILL", "STAR" };
    if (b->stage == GS_EGG) chip(cx, y, "EGG", UI_AMBER, UI_INK);
    else if (b->stage == GS_BABY) chip(cx, y, "BABY", UI_GREEN, UI_WHITE);
    else if (b->stage == GS_CHILD) chip(cx, y, "CHILD", UI_BLUE, UI_WHITE);
    else if (b->stage == GS_TEEN) chip(cx, y, b->form ? "SLOUCHY" : "ANGSTY", b->form ? UI_RGB(150, 112, 70) : UI_BLUE2, UI_WHITE);
    else chip(cx, y, fn[b->form & 3], fc[b->form & 3], (b->form & 3) == 0 ? UI_INK : UI_WHITE);
}
static void lcd_tile(const Blob *b, int x, int y, int fr, int look) {   // a pet on a little rounded LCD, 64 x 32
    ui_round(x - 2, y - 2, 68, 36, 5, UI_RGB(52, 24, 70), UI_RGB(30, 12, 44), 0);
    scene(b, fr, look); gfx_rect(x, y, 64, 32, 172, 196, 128, 0); dots_draw(x, y, 2, 30, 46, 26);
}
static void sky(void) {
    for (int y = 0; y < 240; y += 8) { int g = y * 20 / 240; gfx_rect(0, y, 320, 8, 22 - g, 12 - g / 2, 52 - g, 0); }
    for (int i = 0; i < 28; i++) { u32 h = hsh(i * 77 + 5); int x = (int)(h % 320), y = (int)((h >> 9) % 240), tw = ((anim >> 3) + i) & 7; gfx_rect(x, y, tw == 0 ? 3 : 2, tw == 0 ? 3 : 2, 200, 190, 255, 1); }
}
static void draw_box(void) {
    sky();
    int top = ui_window(14, 8, 292, 224, "TOY BOX", IC_STAR);
    int idx[GT_MAX], n = box_list(idx); int y = top + 3;
    for (int k = 0; k < n; k++) {
        int si = idx[k]; Toy *t = gotchi_toy(si); int sel = k == bxsel;
        ui_round(20, y, 280, 28, 7, sel ? UI_HILITE : UI_WHITE, sel ? UI_HILITE : UI_PANEL, 0);
        if (sel) ui_round(19, y - 1, 282, 30, 8, UI_BLUE, UI_BLUE, 1);
        if (t) { const u32 *sh = shell[owner_color(t)]; ui_disc(36, y + 14, 22, UI_RGB(sh[0], sh[1], sh[2])); ui_icon(30, y + 8, IC_SMILE, UI_WHITE); }
        else { ui_disc(36, y + 14, 22, UI_LINE); ui_icon(30, y + 8, IC_LOCK, UI_WHITE); }
        ui_text(54, y + 4, 1, sims[si].bore.first, UI_INK);
        if (t) {
            char b[48]; sprintf(b, "LV %d  %s", gotchi_level(t), gotchi_level_name(gotchi_level(t))); ui_text(54, y + 16, 1, b, UI_SOFT);
            for (int i = 0; i < GB_N; i++) { const Blob *p = &t->b[i]; u32 c = !p->on ? UI_LINE : gotchi_needs(p) ? UI_RED : p->stage == GS_EGG ? UI_AMBER : UI_GREEN; ui_disc(190 + i * 11, y + 9, 8, c); }
            if (gotchi_calling(t)) { sprintf(b, "%d CALLING", gotchi_calling(t)); ui_badge(190, y + 15, b, UI_RED, UI_WHITE); } else ui_badge(190, y + 15, "ALL GOOD", UI_GREEN, UI_WHITE);
        } else ui_text(54, y + 16, 1, "NO TOY YET: $5000 AT ELECTRONICS SHOPS", UI_SOFT);
        y += 31;
    }
    { char b[56]; sprintf(b, "BLOBDEX %d OF %d", gotchi_dex_count(), GSP_N * 4); ui_bar(60, 194, 200, 7, gotchi_dex_count() * 100 / (GSP_N * 4), UI_AMBER); ui_textc(160, 184, 1, b, UI_GOLDT); }
    if (msg) ui_textc(160, 172, 1, msg, UI_BADT);
    ui_hints(160, 212, "X OPEN|TRI BLOBDEX|SEL GUIDE|O BACK", 1);
}
static void draw_basket(void) {
    sky(); Toy *t = tt();
    char title[40]; int si = gotchi_sim_of(t); sprintf(title, "%s'S BASKET", si >= 0 ? sims[si].bore.first : "THE");
    int top = ui_window(14, 8, 292, 224, title, IC_STAR);
    for (int i = 0; i < GB_N; i++) {
        Blob *b = &t->b[i]; int x = 20 + (i & 3) * 70, y = top + 3 + (i >> 2) * 80, sel = i == bi;
        ui_round(x, y, 66, 76, 7, sel ? UI_HILITE : UI_WHITE, sel ? UI_HILITE : UI_PANEL, 0);
        if (sel) ui_round(x - 1, y - 1, 68, 78, 8, UI_BLUE, UI_BLUE, 1);
        if (b->on) {
            lcd_tile(b, x + 1, y + 3, anim / 20 + i, 0);
            if (gotchi_needs(b)) { ui_disc(x + 59, y + 8, 13, UI_RED); ui_textc(x + 59, y + 3, 1, "!", UI_WHITE); }
            ui_textfit(x + 4, y + 40, 58, gotchi_given(b->nm), UI_INK); formchip(b, x + 33, y + 51);
            for (int h = 0; h < 4; h++) ui_icon_s(x + 8 + h * 13, y + 63, IC_HEART, 10, h < (b->bond + 12) / 25 ? UI_RED : UI_LINE);
        } else { ui_icon_s(x + 21, y + 14, IC_ADD, 24, UI_LINE); ui_textc(x + 33, y + 46, 1, sel ? "NEW EGG" : "EMPTY", sel ? UI_BLUE2 : UI_SOFT); ui_textc(x + 33, y + 58, 1, "$150", UI_GOLDT); }
    }
    { int y = top + 166; char b[40]; sprintf(b, "LV %d  %s", gotchi_level(t), gotchi_level_name(gotchi_level(t))); ui_badge(22, y, b, UI_BLUE2, UI_WHITE); sprintf(b, "%d HATCHED", t->hatched); ui_badge(150, y, b, UI_GREEN, UI_WHITE); sprintf(b, "%d WINS", t->wins); ui_badge(232, y, b, UI_AMBER, UI_INK); }
    if (msg) ui_textc(160, 178, 1, msg, UI_BADT);
    ui_hints(160, 212, "X OPEN|TRI BLOBDEX|SEL GUIDE|O BACK", 1);
}
// ---- the handheld ----
static void egg_rows(int cy, int rx, int ry, int r, int g, int b, int grad) {
    for (int y = cy - ry; y < cy + ry; y += 2) { int dy = y + 1 - cy; long h2 = (long)rx * rx - (long)dy * dy * rx * rx / ((long)ry * ry); if (h2 <= 0) continue; int h = 1; while ((long)(h + 1) * (h + 1) <= h2) h++;
        int k = grad ? (y - (cy - ry)) / 14 : 0; gfx_rect(160 - h, y, h * 2, 2, MAX(0, r - k * 3), MAX(0, g - k * 2), MAX(0, b - k * 3), 0); }
}
static int egg_half(int y) { int dy = y - 109; long h2 = 100L * 100 - (long)dy * dy * 100 * 100 / (104L * 104); int h = 0; while ((long)(h + 1) * (h + 1) <= h2) h++; return h2 > 0 ? h : 0; }
static void draw_egg_body(int c) {
    const u32 *sh = shell[c];
    ui_disc(160, 6, 30, UI_RGB(190, 192, 205)); ui_disc(160, 6, 14, UI_RGB(20, 10, 40));   // the keyring
    gfx_rect(70, 214, 180, 5, 0, 0, 10, 1); gfx_rect(86, 219, 148, 3, 0, 0, 10, 1);          // its shadow
    egg_rows(109, 104, 105, (int)sh[0] * 45 / 100, (int)sh[1] * 45 / 100, (int)sh[2] * 45 / 100, 0);   // the rim
    egg_rows(109, 100, 102, (int)sh[0], (int)sh[1], (int)sh[2], 1);
    for (int i = 0; i < 26; i++) { u32 h = hsh(i * 131 + 9); int y = 20 + (int)(h % 180), hf = egg_half(y) - 8; if (hf < 4) continue; int x = 160 - hf + (int)((h >> 8) % (2 * hf)); gfx_rect(x, y, 2, 2, 255, 255, 255, 1); }
    for (int y = 26; y < 92; y += 2) { int hf = egg_half(y); gfx_rect(160 - hf + 8 + (y - 26) / 9, y, 3, 2, 255, 255, 255, 1); }      // a gloss down the left
    gfx_rect(160 - egg_half(24) + 24, 24, 12, 3, 255, 255, 255, 1);
}
static void lit_icon(int i, int x, int y, int on, int lit) {
    u32 top = on ? UI_RGB(255, 255, 255) : lit ? UI_RGB(255, 200, 70) : UI_RGB(86, 36, 104), bot = on ? UI_RGB(190, 222, 255) : lit ? UI_RGB(230, 140, 20) : UI_RGB(50, 18, 66);
    ui_round(x - 4, y - 4, 26, 22, 5, top, bot, 0);
    u32 c = on ? 0x6A3C2E : lit ? 0x1C3C70 : 0xC890F0;
    for (int r = 0; r < 7; r++) for (int q = 0; q < 9; q++) if (icon[i][r][q] == '#') gfx_rect(x + q * 2, y + r * 2, 2, 2, c & 255, (c >> 8) & 255, (c >> 16) & 255, 0);
}
static void pips(int x, int y, int v, int n, int r, int g, int b) { for (int i = 0; i < n; i++) { if (i < v) gfx_rect(x + i * 7, y, 5, 5, r, g, b, 0); else gfx_rect(x + i * 7, y, 5, 5, 70, 56, 86, 0); } }
static void draw_hand(int lcdMode) {
    Toy *t = tt(); Blob *b = cb(); int dark = (b->fl & GF_SLEEP) && (b->fl & GF_DARK);
    sky(); draw_egg_body(owner_color(t));
    // the screen: a dark bezel, the LCD, and the little lights along its lip
    ui_round(87, 42, 146, 84, 8, UI_RGB(54, 26, 74), UI_RGB(26, 10, 40), 0);
    gfx_rect(95, 49, 130, 66, 18, 8, 30, 0);
    gfx_rect(96, 50, 128, 64, dark ? 70 : 172, dark ? 84 : 196, dark ? 62 : 128, 0);
    if (lcdMode == 0) { scene(b, anim / 20, 0); dots_draw(96, 50, 4, 28, 44, 24); if (gotchi_time == 1) { ui_badge(98, 52, "LIVE", UI_RED, UI_WHITE); } else if (gotchi_time == 2) ui_badge(98, 52, "PAUSED", UI_SOFT, UI_WHITE); }
    pips(98, 118, b->hun, 4, 255, 170, 40); pips(206, 118, b->hap, 4, 255, 100, 170); for (int i = 0; i < 4; i++) gfx_rect(147 + i * 7, 118, 5, 5, i < (b->bond + 12) / 25 ? 255 : 70, i < (b->bond + 12) / 25 ? 60 : 56, i < (b->bond + 12) / 25 ? 70 : 86, 0);
    if (lcdMode == 0) { char nm[40], b2[64]; gotchi_name(b, nm); sprintf(b2, "%s THE %s", gotchi_given(b->nm), nm); int w = gfx_text_w(1, b2) + 16; ui_round(160 - w / 2, 128, w, 13, 6, UI_RGB(60, 24, 80), UI_RGB(34, 12, 50), 0); tshadowc(160, 131, b2, 255, 235, 140); }
    for (int i = 0; i < 10; i++) lit_icon(i, 95 + (i % 5) * 28, i < 5 ? 20 : 146, i == isel && lcdMode == 0, i == 7 && (b->fl & GF_CALL));
    { const char *nm = iconNm[isel]; int w = ui_tw(1, nm) + 14; ui_round(160 - w / 2, 167, w, 12, 6, UI_RGB(255, 255, 255), UI_RGB(206, 222, 250), 0); ui_textc(160, 169, 1, nm, UI_INK); }
    ui_disc(138, 196, 17, UI_RGB(255, 214, 70)); ui_disc(160, 203, 17, UI_RGB(255, 214, 70)); ui_disc(182, 196, 17, UI_RGB(255, 214, 70));
    ui_disc(137, 194, 6, UI_RGB(255, 245, 190)); ui_disc(159, 201, 6, UI_RGB(255, 245, 190)); ui_disc(181, 194, 6, UI_RGB(255, 245, 190));
    if (msg) { ui_round(22, 215, 276, 20, 8, UI_RGB(24, 10, 40), UI_RGB(12, 4, 24), 0); ui_wrapc(30, 217, 260, 2, msg, UI_WHITE); }
    if (lcdMode == 0) { ui_hints(6, 176, "X USE", 0); ui_hints(6, 190, "L1 R1 PET", 0); ui_hints(314, 176, "O BASKET", 2); }
}
static void lcd_panel(void) { gfx_rect(96, 50, 128, 64, 172, 196, 128, 0); }
static void draw_stats(void) {
    draw_hand(1); lcd_panel(); Toy *t = tt(); Blob *b = cb(); char s[56];
    if (sub == 0) {
        lt(100, 53, "HUNGER"); lt(100, 82, "HAPPY"); lt(172, 53, "BOND");
        for (int i = 0; i < 4; i++) { ui_disc(108 + i * 16, 72, 11, i < b->hun ? UI_RGB(28, 44, 24) : UI_RGB(130, 160, 100)); ui_disc(108 + i * 16, 101, 11, i < b->hap ? UI_RGB(28, 44, 24) : UI_RGB(130, 160, 100)); ui_disc(180 + i * 11, 72, 8, i < (b->bond + 12) / 25 ? UI_RGB(28, 44, 24) : UI_RGB(130, 160, 100)); }
        sprintf(s, "%d GRAMS", b->wt); lt(172, 86, s);
    } else {
        char nm[32]; gotchi_name(b, nm); lt(100, 53, nm);
        sprintf(s, "AGE %d HOURS", b->age / 60); lt(100, 64, s); sprintf(s, "DISCIPLINE %d%%", b->disc); lt(100, 75, s);
        sprintf(s, "MISTAKES %d  GEN %d", b->miss, b->gen + 1); lt(100, 86, s); sprintf(s, "YOU: %s", gotchi_level_name(gotchi_level(t))); lt(100, 97, s);
        int nx = gotchi_next_xp(t); if (nx) ui_bar(100, 108, 120, 4, t->xp * 100 / nx, UI_RGB(28, 44, 24));
    }
    ui_hints(6, 176, "X MORE", 0); ui_hints(314, 176, "O BACK", 2);
}
static void tri_btn(int x, int w, const char *l, int on) { ui_round(x, 66, w, 22, 5, on ? UI_BLUE : UI_CARD2, on ? UI_BLUE2 : UI_CARD2, 0); ui_textc(x + w / 2, 73, 1, l, on ? UI_WHITE : UI_INK); }
static void draw_feed(void) { draw_hand(1); lcd_panel();
    ltc(160, 53, "WHAT TO GIVE IT?"); tri_btn(98, 38, "MEAL", sub == 0); tri_btn(138, 40, "SNACK", sub == 1); tri_btn(180, 42, "LUNCH", sub == 2);
    { static const char *const bl[3] = { "A MEAL FILLS IT UP", "A SNACK CHEERS IT UP", "YOUR OWN LUNCH: YOU GET HUNGRIER" }; ui_wrapc(100, 94, 120, 2, bl[sub], UI_INK); }
    ui_hints(6, 176, "X GIVE", 0); ui_hints(314, 176, "O BACK", 2); }
static void draw_playpick(void) { draw_hand(1); lcd_panel();
    ltc(160, 53, "PICK A GAME"); tri_btn(98, 58, "WHICH WAY", sub == 0); tri_btn(160, 58, "COPY ME", sub == 1);
    ui_wrapc(100, 94, 120, 2, sub ? "REPEAT ITS MOVES. 3 ROUNDS" : "GUESS WHERE IT LOOKS. 5 ROUNDS", UI_INK); ui_hints(6, 176, "X PLAY", 0); ui_hints(314, 176, "O BACK", 2); }
static void draw_look(void) {
    draw_hand(1); lcd_panel(); Blob *b = cb(); char s[40];
    scene(b, anim / 20, mgN ? (sidx ? 1 : -1) : 0); dots_draw(96, 50, 4, 28, 44, 24);
    sprintf(s, "ROUND %d  WINS %d", mgRound + 1, mgWon); tshadowc(160, 131, s, 255, 235, 140);
    msg = mgN == 0 ? "WHICH WAY WILL IT LOOK? LEFT OR RIGHT" : ((sidx ? 1 : -1) == mgDir ? "YES! IT LOOKED YOUR WAY" : "NOPE. THE OTHER WAY"); msgT = 2;
}
static void draw_copy(void) {
    draw_hand(1); lcd_panel(); Blob *b = cb(); char s[40]; static const char *nm[4] = { "UP", "DOWN", "LEFT", "RIGHT" };
    int showing = mgT >= 0 && mgT < mgN * 40, k = showing ? mgT / 40 : -1, look = 0, jump = 0;
    if (showing) { int m = seq[k]; look = m == 2 ? -1 : m == 3 ? 1 : 0; jump = m == 0; }
    scene(b, jump ? 1 : 0, look); dots_draw(96, 50 - (jump ? 6 : 0), 4, 28, 44, 24);
    sprintf(s, "ROUND %d  GOT %d", mgRound + 1, mgWon); tshadowc(160, 131, s, 255, 235, 140);
    if (mgT < 0) msg = "WATCH CLOSELY..."; else if (showing) msg = nm[seq[k]]; else { static char m2[32]; sprintf(m2, "YOUR TURN: %d OF %d", sidx + 1, mgN); msg = m2; } msgT = 2;
}
static void draw_friend(void) {
    draw_hand(1); Toy *t = tt(); Blob *c = &t->b[sub]; lcd_panel(); scene(c, anim / 20, 0); dots_draw(96, 50, 4, 28, 44, 24);
    char nm[40], s[64]; gotchi_name(c, nm); sprintf(s, "WITH %s THE %s", gotchi_given(c->nm), nm); tshadowc(160, 131, s, 255, 235, 140);
    msg = "X: PLAYDATE    SQUARE: BATTLE    <> PICK A FRIEND"; msgT = 2;
}
// ---- the blobdex ----
static void draw_dex(void) {
    sky(); char t[32]; sprintf(t, "BLOBDEX  %d OF %d", gotchi_dex_count(), GSP_N * 4);
    int top = ui_window(14, 8, 292, 224, t, IC_STAR); static const char *const fn[4] = { "SUPER", "SLOB", "CHILL", "STAR" };
    static const u32 fc[4] = { UI_RGB(236, 176, 40), UI_RGB(150, 112, 70), UI_RGB(60, 190, 210), UI_RGB(220, 70, 180) };
    for (int f = 0; f < 4; f++) chip(90 + f * 52 + 24, top + 1, fn[f], fc[f], f == 0 ? UI_INK : UI_WHITE);
    int y = top + 14;
    for (int r = 0; r < 4; r++) {
        int sp = dexpage * 4 + r;
        ui_round(20, y, 280, 35, 7, UI_WHITE, UI_PANEL, 0);
        ui_text(28, y + 13, 1, gotchi_species(sp), UI_INK);
        for (int f = 0; f < 4; f++) {
            int x = 90 + f * 52, has = gotchi_dex_has(sp, f);
            ui_round(x - 1, y + 1, 50, 34, 5, has ? fc[f] : UI_LINE, has ? fc[f] : UI_LINE, 0);
            if (has) { Blob b; memset(&b, 0, sizeof b); b.on = 1; b.stage = GS_ADULT; b.sp = (u8)sp; b.form = (u8)f; b.hun = 4; b.hap = 4; scene(&b, 0, 0); gfx_rect(x, y + 2, 48, 32, 172, 196, 128, 0); dots_draw(x - 8, y + 2, 2, 30, 46, 26); }
            else { gfx_rect(x, y + 2, 48, 32, 214, 222, 234, 0); ui_textc(x + 24, y + 14, 1, "?", UI_SOFT); }
        }
        y += 38;
    }
    { char p[40]; sprintf(p, "PAGE %d OF 4", dexpage + 1); ui_textc(160, 188, 1, p, UI_SOFT); ui_textc(160, 199, 1, "GOOD CARE: SUPER, STAR. SNACKS: CHILL.", UI_SOFT); }
    ui_hints(160, 212, "L1 R1 PAGE|O BACK", 1);
}
// ---- the guide: pictures, little live art, and plain words ----
#define NGP 9
typedef struct { const char *title; s8 img; u8 strip; const char *tip[3]; } GPage;
static const GPage gp[NGP] = {
    { "MEET THE TOY", 0, 0, { "A TINY PET ON A KEYCHAIN. IT LOVES YOU BACK.", "$5000 AT ELECTRONICS AND GAMES SHOPS.", "EVERYONE CAN OWN ONE. EIGHT PETS EACH." } },
    { "THE HANDHELD", 1, 1, { "D-PAD PICKS, X USES. L1 R1 SWAP PETS.", "BELL LIT UP? A PET NEEDS YOU. HURRY!", "HUNGRY PETS BEEP EVEN WHEN YOU'RE BUSY." } },
    { "EIGHT PETS PER TOY", 2, 2, { "THE BASKET HOLDS EIGHT. A NEW EGG IS $150.", "16 KINDS: CATS, ROBOTS, GHOSTS, ALIENS...", "HEARTS SHOW HOW CLOSE YOU TWO ARE." } },
    { "GROWING UP", 3, 3, { "EGG, BABY, CHILD, TEEN, ADULT. ~2 DAYS.", "YOUR CARE DECIDES WHAT IT BECOMES.", "GROWN-UPS FILL THE BLOBDEX. GET THEM ALL!" } },
    { "PLAY, FRIENDS AND EGGS", 4, 4, { "WIN WHICH WAY OR COPY ME TO CHEER IT UP.", "PLAYDATES CHEER BOTH. SQUARE = BATTLE.", "BEST FRIENDS MAY LEAVE A SURPRISE EGG." } },
    { "HIJINKS: YOU AND THE PET", 5, 5, { "LIVE MODE: REAL TIME. YOU MAY FORGET TO EAT!", "SAD OR STARVING? YOUR PETS FEEL IT TOO.", "SHARE YOUR LUNCH, OR GET A NUZZLE FOR FUN." } },
    { "MOODS AND CARE SKILL", 6, 6, { "PETS CHANGE YOUR MOOD: PROUD PARENT, BUDDY...", "NEGLECT THEM AND YOU WORRY. LOSE ONE: OUCH.", "CARE SKILL GROWS: LESS HUNGER, LESS SICK." } },
    { "BUILD AND BUY THINGS", 7, 7, { "NEW BUILD TAB: NINE THINGS FOR HOME.", "DOCK OPENS THE BASKET. CABINET: BLOBDEX.", "BOWL, LAMP, BED CARE FOR ALL PETS AT ONCE." } },
    { "WHAT'S NEW", 6, 8, { "NEW: EIGHT PETS, 16 KINDS, 64 FORMS, BLOBDEX.", "NEW: GAMES, PLAYDATES, BATTLES, EGGS, MOODS.", "NEW: BUILD ITEMS, SHOP BUY, TIME OPTION." } },
};
static void art_show(int i) {
    if (!art || i < 0 || i >= NIMG || art_page == i) return;
    u16 *px = atlas_scratch(); const u16 *src = (const u16 *)(art + (u32)i * IW * IH * 2);
    for (int y = 0; y < IH; y++) for (int x = 0; x < 128; x++) px[y * 128 + x] = x < IW ? src[y * IW + x] : 0;
    atlas_vram_load(0, 256, 128, 128, px); art_page = i;
}
static void strip_species(int a, int n, const int *sp, int stage) {   // n pets in a row on LCD tiles, labelled
    for (int i = 0; i < n; i++) { Blob b; memset(&b, 0, sizeof b); b.on = 1; b.stage = (u8)stage; b.sp = (u8)sp[i]; b.hun = 4; b.hap = 4; b.form = 0; int x = 22 + i * (276 / n) + (276 / n - 68) / 2; lcd_tile(&b, x, a, anim / 20 + i, 0); chip(x + 32, a + 38, gotchi_species(sp[i]), UI_BLUE2, UI_WHITE); }
}
static void guide_strip(int k, int y) {
    ui_round(20, y - 4, 280, 64, 7, UI_WHITE, UI_PANEL, 0); y += 1;
    int cyc = (anim / 120) % GSP_N;
    switch (k) {
    case 0: { static const int s[4] = { 0, 1, 4, 9 }; int sp[4]; for (int i = 0; i < 4; i++) sp[i] = (s[i] + cyc) % GSP_N; strip_species(y, 4, sp, GS_ADULT); break; }
    case 1: { static const int ic[5] = { 0, 1, 3, 4, 6 }; for (int i = 0; i < 5; i++) { int x = 42 + i * 52; ui_round(x - 4, y + 4, 26, 22, 5, UI_RGB(86, 36, 104), UI_RGB(50, 18, 66), 0);
                for (int r = 0; r < 7; r++) for (int q = 0; q < 9; q++) if (icon[ic[i]][r][q] == '#') gfx_rect(x + q * 2, y + 8 + r * 2, 2, 2, 200, 144, 240, 0); chip(x + 9, y + 33, iconNm[ic[i]], UI_BLUE2, UI_WHITE); } break; }
    case 2: { int sp[1] = { cyc }; for (int i = 0; i < 4; i++) { Blob b; memset(&b, 0, sizeof b); b.on = 1; b.sp = (u8)((cyc + i * 3) % GSP_N); b.stage = (u8)(i == 0 ? GS_EGG : GS_BABY + (i - 1) * 1); if (i == 3) b.stage = GS_ADULT; b.hun = b.hap = 4; int x = 26 + i * 69; lcd_tile(&b, x, y, anim / 20 + i, 0); formchip(&b, x + 32, y + 38); } (void)sp; break; }
    case 3: { static const u8 st[5] = { GS_EGG, GS_BABY, GS_CHILD, GS_TEEN, GS_ADULT }; static const char *const nm[5] = { "EGG", "BABY", "CHILD", "TEEN", "ADULT" };
              for (int i = 0; i < 5; i++) { Blob b; memset(&b, 0, sizeof b); b.on = 1; b.sp = (u8)cyc; b.stage = st[i]; b.hun = b.hap = 4; b.age = 16; int x = 24 + i * 55; ui_round(x - 1, y - 1, 52, 28, 4, UI_RGB(52, 24, 70), UI_RGB(30, 12, 44), 0); scene(&b, anim / 20, 0); gfx_rect(x, y, 50, 26, 172, 196, 128, 0); dots_draw(x - 7, y - 3, 2, 30, 46, 26); chip(x + 25, y + 31, nm[i], UI_GREEN, UI_WHITE); } break; }
    case 4: { Blob a, b2; memset(&a, 0, sizeof a); memset(&b2, 0, sizeof b2); a.on = b2.on = 1; a.stage = b2.stage = GS_ADULT; a.sp = (u8)cyc; b2.sp = (u8)((cyc + 5) % GSP_N); a.hun = a.hap = b2.hun = b2.hap = 4;
            lcd_tile(&a, 40, y, anim / 20, 1); lcd_tile(&b2, 212, y, anim / 20, -1); ui_icon_s(144, y + 4, IC_HEART, 24, (anim / 15) & 1 ? UI_RED : UI_BADT); ui_textc(156, y + 34, 1, "PLAYDATE", UI_INK); ui_textc(80, y + 38, 1, "OR", UI_SOFT); break; }
    case 5: { ui_icon_disc(60, y + 14, 30, IC_SMILE, UI_BLUE, UI_WHITE); ui_text(36, y + 36, 1, "YOU: TUMMY", UI_INK); { Blob b; memset(&b, 0, sizeof b); b.on = 1; b.stage = GS_ADULT; b.sp = (u8)cyc; b.hun = b.hap = 4; lcd_tile(&b, 214, y, anim / 20, -1); }
            ui_textc(150, y + 2, 1, "SHARE LUNCH", UI_GOLDT); ui_textc(150, y + 36, 1, "YOU STARVE, IT STARVES", UI_BADT); ui_icon_s(138, y + 14, IC_ARROW, 20, UI_GOLDT); break; }
    case 6: { static const char *const m[6] = { "IT HATCHED! +16", "PROUD PARENT +14", "BLOB BUDDY +10", "WORRIED -8", "HOOKED ON THE TOY -8", "MISSING MY BLOB -15" };
              for (int i = 0; i < 6; i++) { int x = 26 + (i & 1) * 138, yy = y + (i >> 1) * 18; ui_badge(x, yy, m[i], i < 3 ? UI_GREEN : UI_RED, UI_WHITE); } break; }
    case 7: { static const char *const o[9] = { "DOCK $120", "SHELF $180", "CABINET $300", "PLUSH $60", "BALL $40", "BED $90", "BOWL $40", "LAMP $70", "TROPHY $200" };
              for (int i = 0; i < 9; i++) ui_badge(24 + (i % 3) * 92, y + (i / 3) * 18, o[i], UI_BLUE2, UI_WHITE); break; }
    default: ui_textc(160, y + 2, 1, "NO MISSIONS: JUST YOU AND THE PETS.", UI_INK); ui_textc(160, y + 16, 1, "IT'S ITS OWN FILE ON THE DISC (GOTCHI.OVL):", UI_SOFT); ui_textc(160, y + 28, 1, "SO IT CAN BE PATCHED ON ITS OWN.", UI_SOFT);
        ui_badge(60, y + 42, "16 KINDS", UI_GREEN, UI_WHITE); ui_badge(130, y + 42, "64 FORMS", UI_AMBER, UI_INK); ui_badge(200, y + 42, "8 PER TOY", UI_BLUE2, UI_WHITE); break;
    }
}
static void draw_guide(void) {
    sky(); const GPage *g = &gp[guidepage];
    int top = ui_window(14, 8, 292, 224, g->title, IC_HELP);
    { char b[16]; sprintf(b, "PAGE %d OF %d", guidepage + 1, NGP); ui_badge(300 - ui_tw(1, b) - 10, 12, b, UI_WHITE, UI_BLUE2); }
    art_show(g->img);
    ui_round(19, top + 2, 108, 80, 6, UI_BLUE2, UI_BLUE2, 0);
    if (art && art_page == g->img) gfx_sprite_c(21, top + 4, 104, 76, getTPage(2, 0, 0, 256), 0, 0, IW, IH, 128, 128, 128);
    else { gfx_rect(21, top + 4, 104, 76, 172, 196, 128, 0); ui_icon_s(61, top + 30, IC_STAR, 24, UI_WHITE); }
    for (int i = 0; i < 3; i++) { int y = top + 2 + i * 27; ui_round(134, y, 168, 25, 6, UI_WHITE, UI_PANEL, 0); ui_disc(142, y + 12, 8, UI_BLUE); ui_wrap(150, y + 3, 150, 2, g->tip[i], UI_INK); }
    guide_strip(g->strip, top + 90);
    for (int k = 0; k < NGP; k++) ui_disc(160 - (NGP - 1) * 6 + k * 12, 204, 7, k == guidepage ? UI_BLUE : UI_LINE);
    ui_hints(160, 214, "L1 R1 PAGE|O BACK", 1);
}
void gotchi_draw(void) {
    switch (view) {
    case V_BOX: draw_box(); break; case V_BASKET: draw_basket(); break; case V_HAND: draw_hand(0); break; case V_FEED: draw_feed(); break;
    case V_PLAYPICK: draw_playpick(); break; case V_LOOK: draw_look(); break; case V_COPY: draw_copy(); break; case V_FRIEND: draw_friend(); break;
    case V_STATS: draw_stats(); break; case V_GUIDE: draw_guide(); break; default: draw_dex(); break;
    }
}
#include "ovl.h"
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, gotchi_open, gotchi_update, gotchi_draw };
