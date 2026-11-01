#pragma GCC optimize("Os")
// gotchi_ui.c - the BOREAGOTCHI! expansion pack's screens: the TOY BOX, the EGG BASKET (up to eight creatures), the HANDHELD (a colour LCD with
// animated moods, a labelled icon menu with hints, stat bars), four little games, the BLOBDEX (a grid with silhouettes and a page for each kind),
// the FAMILY TREE and FAMILY HOME, the DIARY, the AWARDS and the HELP pages. It is the overlay GOTCHI.OVL: a file on the disc of its own (ovl.h).
// The creatures are not drawn from pictures: each of the 20 kinds is built from shapes on the 32 x 16 LCD, at five sizes, six grown-up forms, eight
// colours, with rare mutations, accessories and moods, so there are very many different ones (and a BLOBDEX to fill).
#include "gotchi_act.h"
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "atlas.h"
#include "crowd.h"
#include "ovl.h"
#include <psxgpu.h>

enum { V_BOX, V_BASKET, V_HAND, V_DEX, V_DEXD, V_FEED, V_PLAYPICK, V_LOOK, V_COPY, V_CATCH, V_HILO, V_FRIEND, V_STATS, V_GUIDE, V_TREE, V_HOME, V_DIARY, V_AWARD, V_MORE, V_DRESS };
enum { MD_NORM, MD_HAPPY, MD_DANCE, MD_SLEEP, MD_SICK, MD_HUNGRY, MD_BORED, MD_ANGRY };
static int view, from_box, ti, bi, isel, anim, msgT, dexsel, dexf, bxsel, sub, mgT, mgN, mgWon, mgRound, mgDir, danceT, treeSel, treeId, homeSel, listTop;
static const char *msg; static int dexback, guidepage, guideback, backv;
static u8 seq[6], sidx;
static u32 lrng;
static int rr(void) { lrng = lrng * 1103515245u + 12345u; return (int)((lrng >> 16) & 255); }

// ---- the 32 x 16 LCD, and the creatures drawn on it ----
// a dot is 0 (off), 1 the creature's ink, 2 red, 3 blue, 4 gold
static u8 dot[16][32]; static int minY;
static void P(int x, int y, int v) { if (x >= 0 && x < 32 && y >= 0 && y < 16) { dot[y][x] = (u8)v; if (v && y < minY) minY = y; } }
static void RC(int x0, int y0, int x1, int y1, int v) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) P(x, y, v); }
static void ELL(int cx, int cy, int rx, int ry, int v) { for (int y = -ry; y <= ry; y++) for (int x = -rx; x <= rx; x++) if (x * x * ry * ry + y * y * rx * rx <= rx * rx * ry * ry + rx * ry) P(cx + x, cy + y, v); }
static void LN(int x0, int y0, int x1, int y1, int v) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y1 - y0 : y0 - y1, sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx - dy;
    for (int n = 0; n < 40; n++) { P(x0, y0, v); if (x0 == x1 && y0 == y1) break; int e2 = 2 * e; if (e2 > -dy) { e -= dy; x0 += sx; } if (e2 < dx) { e += dx; y0 += sy; } }
}
static void star(int x, int y, int v) { P(x, y, v); P(x - 1, y, v); P(x + 1, y, v); P(x, y - 1, v); P(x, y + 1, v); }
static void blit(const char *const *r, int h, int x, int y, int v) { for (int j = 0; j < h; j++) for (int i = 0; r[j][i]; i++) if (r[j][i] == '#') P(x + i, y + j, v); }
static const char *const bHeart[4] = { ".#.#.", "#####", ".###.", "..#.." }, *const bNote[5] = { "..##", "..#.", "..#.", "##..", "##.." }, *const bDrop[4] = { ".#.", "#.#", "#.#", ".#." },
    *const bZ[5] = { "###.", "..#.", ".#..", "###.", "...." }, *const bAngry[5] = { "#...#", ".#.#.", "..#..", ".#.#.", "#...#" }, *const bBowl[5] = { "..#.#.", ".#.#..", "######", ".####.", "..##.." },
    *const bTherm[6] = { ".#.", "#.#", "#.#", "#.#", "###", ".#." }, *const bPoop[4] = { "..#.", ".###", "####", "####" };

// the creature: its kind's body, its face and mood, its form, its colour's mutations and its hat
static void creature(const Blob *b, const Kin *k, int fr, int d, int ox, int mood) {
    static const u8 RX[6] = { 0, 3, 4, 5, 6, 6 }, RY[6] = { 0, 3, 4, 4, 5, 5 };
    int st = b->stage, sp0 = b->sp % GSP_N, sp = sp0 >= 16 ? "\1\5\3\10"[sp0 - 16] : sp0, rx = RX[st], ry = RY[st], bob = fr & 1, hop = (mood == MD_DANCE || mood == MD_HAPPY) ? (fr & 2) : 0;
    int by = 14 - bob - hop, cx = 16 + ox, cy, top, ex, eyY, big = st >= GS_TEEN;
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
    case 10: { int capry = ry / 2 + 1; cy = by - ry / 2; RC(cx - rx / 2, by - ry, cx + rx / 2, by, 1); ELL(cx, by - ry, rx, capry, 1); P(cx - rx / 2, by - ry - 1, 0); P(cx + rx / 2, by - ry - 1, 0); if (st > 1) P(cx, by - ry - capry + 1, 0); RC(cx - rx / 2 - 1, by, cx - rx / 2 - 1, by, 1); break; }
    case 11: cy = by - ry / 2; ELL(cx, by, rx + 1 + bob, ry, 1); for (int y = by + 1; y < 16; y++) for (int x = 0; x < 32; x++) dot[y][x] = 0; P(cx - rx, by + 1, 1); if (fr & 2) P(cx + rx - 1, by + 1, 1); break;
    case 12: cy = by - ry; ELL(cx, cy, rx - 1, ry - 1, 1); top = cy - ry + 1; P(cx - rx + 2, top - 1, 1); P(cx + rx - 2, top - 1, 1);
        for (int i = 1; i <= 4; i++) { int w = bob ? -2 : 1; RC(cx - rx + 1 - i, cy - 2 + (bob ? 0 : 2) + i / 2 + w / 2, cx - rx + 1 - i, cy + 1 + i / 3, 1); RC(cx + rx - 1 + i, cy - 2 + (bob ? 0 : 2) + i / 2 + w / 2, cx + rx - 1 + i, cy + 1 + i / 3, 1); } break;
    case 13: cy = by - ry; RC(cx - rx, by - 1, cx + rx, by, 1); ELL(cx + d * rx, by - 2, 1, 1, 1); ELL(cx - d, by - ry + 1, ry - 1 > 1 ? ry - 1 : 2, ry - 1 > 1 ? ry - 1 : 2, 1); P(cx - d, by - ry + 1, 0); P(cx - d + 1, by - ry + 1, 0); P(cx - d + 1, by - ry + 2, 0);
        LN(cx + d * rx, by - 3, cx + d * rx, by - 5, 1); LN(cx + d * (rx - 1), by - 3, cx + d * (rx - 1), by - 5 + bob, 1); cy = by - 2; break;
    case 14: ry--; cy = by - ry - 1; ELL(cx, cy, rx, ry, 1); for (int i = 0; i < 4; i++) { int x = cx - rx + 1 + i * (2 * rx - 2) / 3; for (int y = cy + ry; y <= by + 1; y++) P(x + (((fr + i + y) & 2) ? 1 : 0), y, 1); } break;
    default: cy = by - ry; ELL(cx, cy - 1, rx, ry - 1, 1); RC(cx - 1, cy + ry - 2, cx + 1, by, 1); top = cy - ry; LN(cx - 2, top + 1, cx - 4, top - 2, 1); LN(cx + 2, top + 1, cx + 4, top - 2 + bob, 1); P(cx - 4, top - 3, 1); P(cx + 4, top - 3 + bob, 1); break;
    }
    top = cy - ry;
    switch (sp0) {   // the rare kinds wear something of their own
    case 16: LN(cx, top, cx + 1, top - 3, 4); P(cx + 1, top - 4, 4); break;
    case 17: for (int i = 1; i <= 3; i++) { P(cx - rx - i, cy - i + (fr & 1), 2); P(cx + rx + i, cy - i + (fr & 1), 2); } P(cx - d * (rx + 2), cy + 1, 2); if (fr & 1) P(cx - d * (rx + 3), cy, 4); break;
    case 18: LN(cx - d * rx, cy + 1, cx - d * (rx + 4), cy + 3 - (fr & 1) * 2, 2); LN(cx - d * rx, cy, cx - d * (rx + 3), cy - 2, 4); P(cx, top - 2, 2); P(cx - 1, top - 3 + (fr & 1), 4); break;
    case 19: ELL(cx + 4, top - 2, 2, 2, 4); ELL(cx + 5, top - 2, 2, 2, 0); P(cx - 4, top - 3, 4); break;
    }
    // face
    ex = rx / 2 > 0 ? rx / 2 : 1; eyY = cy - ry / 3; if (sp == 4 || sp == 6 || sp == 11) eyY = cy - 1; if (sp == 10) eyY = by - ry + 1; if (sp == 13) eyY = by - 4; if (sp == 12 || sp == 15) eyY = cy - 1;
    int e1 = cx + d - ex, e2 = cx + d + ex; if (sp == 6) { e1 = e2 = cx + d * (rx - 1); } if (sp == 13) { e1 = e2 = cx + d * rx; }
    int chill = st == GS_ADULT && b->form == 2, sleepy = mood == MD_SLEEP || (b->fl & GF_SLEEP);
    if (sp == 9) { P(e1, eyY, 0); P(e2, eyY, 0); if (big) { P(e1 + 1, eyY, 0); P(e2 + 1, eyY, 0); } }
    else if (sleepy || chill || mood == MD_BORED) { P(e1, eyY, 0); P(e2, eyY, 0); if (e1 != e2 && big) { P(e1 - 1, eyY, 0); P(e2 + 1, eyY, 0); } }
    else if (mood == MD_SICK) { P(e1, eyY, 0); P(e2, eyY, 0); P(e1 - 1, eyY - 1, 0); P(e2 + 1, eyY - 1, 0); P(e1 + 1, eyY + 1, 0); P(e2 - 1, eyY + 1, 0); }
    else { P(e1, eyY, 0); P(e2, eyY, 0); if (big || sp == 15) { P(e1, eyY + 1, 0); P(e2, eyY + 1, 0); } if (sp == 15) { P(e1 - 1, eyY, 0); P(e2 + 1, eyY, 0); } if (mood == MD_ANGRY) { P(e1 - 1, eyY - 1, 0); P(e2 + 1, eyY - 1, 0); } }
    if (sp != 4 && sp != 6) {
        int my = cy + ry / 2 + (sp == 10 ? 1 : 0); if (sp == 13) my = by - 2;
        if (mood == MD_HUNGRY) { P(cx + d, my, 0); P(cx + d + 1, my, 0); P(cx + d, my + 1, 0); P(cx + d + 1, my + 1, 0); }
        else if (mood == MD_SICK || mood == MD_ANGRY || b->hun == 0 || b->hap == 0) { P(cx + d, my, 0); P(cx + d - 1, my + 1, 0); P(cx + d + 1, my + 1, 0); }
        else if (mood == MD_HAPPY || mood == MD_DANCE || (b->hap >= 3 && big)) { P(cx + d - 1, my, 0); P(cx + d, my + 1, 0); P(cx + d + 1, my, 0); }
        else P(cx + d, my, 0);
    }
    // the form: what looking after it made of it
    if (st == GS_TEEN) { if (b->form == 0) { P(e1 - 1, eyY - 1, 0); P(e2 + 1, eyY - 1, 0); } else P(cx + rx + 1, cy - ry + 1, 1); }
    if (st >= GS_ADULT) switch (b->form % GFORMS) {
        case 0: RC(cx - 2, minY - 1, cx + 2, minY - 1, 4); P(cx - 2, minY - 2, 4); P(cx, minY - 2, 4); P(cx + 2, minY - 2, 4); if (fr & 2) star(cx + rx + 3, cy - ry, 4); break;
        case 1: P(cx - rx - 2 + ((fr >> 1) & 3), minY - 1, 1); P(cx + rx + 2 - ((fr >> 1) & 3), minY, 1); P(e1, eyY + 2, 0); P(e2, eyY + 2, 0); break;
        case 2: P(cx + rx + 2, cy - 2, 3); P(cx + rx + 3, cy - 3, 3); P(cx + rx + 3, cy - 1, 3); if (fr & 2) P(cx + rx + 4, cy - 4, 3); break;
        case 3: star(cx, minY - 3, 2); star(cx - rx - 3, cy, 4); if (fr & 2) star(cx + rx + 3, cy - ry + 1, 4); break;
        case 4: RC(cx - rx - 2, cy - 1, cx - rx - 1, cy, 2); RC(cx + rx + 1, cy - 1, cx + rx + 2, cy, 2); P(e1 - 1, eyY - 1, 0); P(e2 + 1, eyY - 1, 0); break;
        default: RC(cx - rx + 1, cy + ry - 1, cx + rx - 1, cy + ry - 1, 0); P(cx - rx - 1, cy + 1, 1); P(cx + rx + 1, cy + 1, 1); P(cx - rx - 1, cy + 2, 1); P(cx + rx + 1, cy + 2, 1); break;
    }
    if (st == GS_ELDER) { P(e1, eyY - 1, 0); P(e2, eyY - 1, 0); LN(cx + rx + 2, cy, cx + rx + 2, by + 1, 3); P(cx + rx + 3, cy, 3); }
    // mutations and hats
    if (k->mut & GMU_SPOTS) { P(cx - rx / 2, cy + 1, 0); P(cx + rx / 2, cy - 1, 0); P(cx + 1, cy + ry - 1, 0); }
    if (k->mut & GMU_HALO) RC(cx - 2, minY - 3, cx + 2, minY - 3, 4);
    if (k->mut & GMU_SPARKLE) { star(cx - rx - 4 + (fr & 1), cy - 1 - (fr & 3), 4); if (fr & 2) P(cx + rx + 4, cy + 1, 4); }
    switch (k->acc) {
        case 1: P(cx - 1, top - 1, 2); P(cx + 1, top - 1, 2); P(cx, top - 1, 4); P(cx - 2, top - 2, 2); P(cx + 2, top - 2, 2); break;
        case 2: RC(cx - 2, top - 3, cx + 2, top - 3, 1); RC(cx - 1, top - 5, cx + 1, top - 4, 1); RC(cx - 1, top - 4, cx + 1, top - 4, 2); break;
        case 3: RC(e1 - 1, eyY, e2 + 1, eyY, 1); P(e1, eyY, 1); P(e2, eyY, 1); break;
        case 4: RC(cx - rx + 1, cy + ry - 1, cx + rx - 1, cy + ry - 1, 2); P(cx + rx - 1, cy + ry, 2); P(cx + rx - 1, cy + ry + 1, 2); break;
        case 5: P(cx + rx - 1, top - 1, 2); P(cx + rx, top - 1, 2); P(cx + rx - 1, top - 2, 2); P(cx + rx - 2, top - 1, 2); P(cx + rx - 1, top - 1, 4); break;
        case 6: P(cx - 2, top - 1, 4); P(cx - 2, top - 2, 4); P(cx, top - 1, 4); P(cx, top - 3, 4); P(cx, top - 2, 4); P(cx + 2, top - 1, 4); P(cx + 2, top - 2, 4); RC(cx - 2, top, cx + 2, top, 4); break;
        case 7: P(cx, top - 4, 2); RC(cx - 1, top - 3, cx + 1, top - 3, 3); RC(cx - 2, top - 2, cx + 2, top - 2, 2); RC(cx - 2, top - 1, cx + 2, top - 1, 3); break;
    }
}
static void egg(const Blob *b, const Kin *k, int fr) {
    int bob = fr & 1, cy = 10 - bob; ELL(16, cy, 4, 5, 1);
    switch (b->sp & 3) { case 0: P(14, cy - 2, 0); P(18, cy, 0); P(15, cy + 2, 0); P(17, cy - 3, 0); break; case 1: RC(13, cy - 1, 19, cy - 1, 0); RC(13, cy + 2, 19, cy + 2, 0); break;
        case 2: for (int x = 12; x < 21; x++) P(x, cy + ((x & 1) ? 0 : 1), 0); break; default: ELL(14, cy - 1, 1, 1, 0); ELL(18, cy + 2, 1, 1, 0); break; }
    if (k->mut) star(24, 3 + (fr & 1), 4);
    if (b->age >= 15) { LN(16, cy - 5, 14, cy - 2, 0); LN(14, cy - 2, 17, cy, 0); LN(17, cy, 15, cy + 3, 0); }
}
static int mood_of(const Blob *b) {
    if (!b->on || b->stage == GS_EGG) return MD_NORM;
    if (danceT > 0) return MD_DANCE;
    if (b->fl & GF_SLEEP) return MD_SLEEP;
    if (b->fl & GF_SICK) return MD_SICK;
    if (b->fl & GF_TANTRUM) return MD_ANGRY;
    if (b->hun == 0) return MD_HUNGRY;
    if (b->hap <= 1) return MD_BORED;
    if (b->hap >= 3 && b->hun >= 2) return MD_HAPPY;
    return MD_NORM;
}
static int lcdCol;   // the ink: 0 the classic LCD green, 1..7 colours; 8 a dark silhouette
static void scene(const Blob *b, const Kin *k, int fr, int look, int mood) {
    memset(dot, 0, sizeof dot); minY = 16; lcdCol = b->on ? k->col : 8;
    if (!b->on) return;
    int d = look ? look : (mood == MD_BORED ? -1 : ((fr >> 1) & 1 ? -1 : 1)), ox = look ? 0 : (((fr >> 1) % 3) - 1) * 2;
    if (mood == MD_DANCE) ox = ((fr >> 1) & 1) ? 4 : -4; else if (mood == MD_ANGRY) ox = (fr & 1) * 2 - 1; else if (mood == MD_SLEEP) ox = 0;
    if (b->stage == GS_EGG) egg(b, k, fr); else creature(b, k, fr, d, ox, mood);
    for (int i = 0; i < b->poop && i < 3; i++) blit(bPoop, 4, 27 - (i & 1) * 4, 11 - (i >> 1) * 4, 1);
    switch (mood) {
    case MD_HAPPY: blit(bHeart, 4, 24, 5 - (fr & 3), 2); if (fr & 1) blit(bHeart, 4, 2, 3 + (fr & 2), 2); break;
    case MD_DANCE: blit(bNote, 5, 24 + (fr & 1), 2 + ((fr >> 1) & 1), 3); blit(bNote, 5, 3 - (fr & 1), 3 - ((fr >> 1) & 1), 2); break;
    case MD_SLEEP: blit(bZ, 5, 24 + ((fr >> 2) & 1), 2 - ((fr >> 1) & 1), 3); P(27, 8 - (fr & 1), 3); break;
    case MD_SICK: blit(bTherm, 6, 2, 2, 2); blit(bDrop, 4, 25, 3 + (fr & 1) * 2, 3); break;
    case MD_HUNGRY: blit(bBowl, 5, 22, 1 + (fr & 1), 4); break;
    case MD_BORED: P(24, 3, 1); P(26, 3, 1); P(28, 3, 1); if (fr & 2) P(22, 6, 1); break;
    case MD_ANGRY: if (fr & 2) blit(bAngry, 5, 1, 1, 2); blit(bAngry, 5, 25, 2, 2); break;
    }
}
static const u8 PAL[10][3] = { { 28, 44, 24 }, { 214, 58, 120 }, { 40, 92, 214 }, { 226, 112, 24 }, { 126, 64, 200 }, { 20, 150, 140 }, { 196, 40, 44 }, { 218, 160, 10 }, { 54, 44, 90 }, { 214, 222, 234 } };
static const u8 INKV[5][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 214, 50, 70 }, { 48, 96, 214 }, { 220, 160, 18 } };
// the dots onto the screen: runs of one ink are joined into one box, a thin gap under each row gives the LCD its lines
static void dots_draw(int x, int y, int s, int c0, int c1, int xa, int xb) {
    for (int j = c0; j < c1; j++) {
        for (int i = xa; i < xb;) {
            int v = dot[j][i]; if (!v) { i++; continue; } int k = i; while (k < xb && dot[j][k] == v) k++;
            const u8 *c = lcdCol == 8 ? PAL[8] : v == 1 ? PAL[lcdCol] : INKV[v]; gfx_rect(x + (i - xa) * s, y + (j - c0) * s, (k - i) * s, s >= 3 ? s - 1 : s, c[0], c[1], c[2], 0); i = k;
        }
    }
}
static void dots_all(int x, int y, int s) { dots_draw(x, y, s, 0, 16, 0, 32); }
static void lcd_bg(int x, int y, int w, int h, int dark) { gfx_rect(x, y, w, h, dark ? 70 : 172, dark ? 84 : 196, dark ? 62 : 128, 0); }

// ---- the icons ----
#define NICON 12
static const char *const iconNm[NICON] = { "FEED", "PLAY", "LIGHT", "MEDS", "CLEAN", "SCOLD", "PET", "STATS", "FAMILY", "VISIT", "DRESS", "MORE" };
static const char *const iconHint[NICON] = { "FEED MEALS, SNACKS OR YOUR OWN LUNCH. IT HAS FAVOURITES", "FOUR GAMES TO CHEER IT UP AND TRAIN IT", "LIGHTS OUT HELPS IT SLEEP. ON IN THE DAY",
    "CURES THE SICK. NOT FOR A HEALTHY PET", "FLUSH THE MESS BEFORE IT MAKES THEM ILL", "ONLY FOR FIBS AND TANTRUMS", "CUDDLE IT. CALMS A TANTRUM. A NUZZLE CHEERS YOU", "NEEDS, PERSONALITY, TASTES, AGE",
    "THE FAMILY TREE: PARENTS, KIDS, GRANDPARENTS", "PLAYDATES AND BATTLES WITH ANY PET, EVEN A HOUSEMATE'S", "HATS AND BOWS FOR YOUR PET", "HOME, DIARY, AWARDS, BLOBDEX AND HELP" };
static const char *const icon[NICON][7] = {
    { "...#.....", "..#.##...", ".#####...", "#######..", "#######..", ".#####...", "........." },
    { ".#######.", "#..#....#", "#.###.#.#", "#..#..#.#", "#.......#", ".#######.", "........." },
    { "...###...", "..#...#..", "..#...#..", "...#.#...", "...###...", "...###...", "........." },
    { "...###...", "...#.#...", ".###.###.", ".#.....#.", ".###.###.", "...#.#...", "...###..." },
    { "..#.#.#..", "...#.#...", "..#####..", "...###...", "...###...", "..#####..", "........." },
    { ".#####...", ".#...#...", ".#.#.#...", ".#####...", "...#.....", "...###...", "........." },
    { "...#.....", "..###....", "..###....", ".#####...", ".#####...", "#######..", "...#....." },
    { "#........", "#..#.....", "#..#..#..", "#.##..#..", "#.##.##..", "#########", "........." },
    { "..#...#..", ".###.###.", "..#...#..", "...#.#...", "..#####..", ".#######.", "........." },
    { ".##...##.", "#..#.#..#", ".##...##.", "..#...#..", ".###.###.", "#..#.#..#", "........." },
    { "..####...", ".######..", "..####...", "#########", ".#######.", "........." , "........." },
    { "..#####..", ".#.....#.", "#.##.##.#", "#.#...#.#", "#.##.##.#", ".#.....#.", "..#####.." } };
static void icon_bits(int i, int x, int y, int sc, u32 c) { for (int r = 0; r < 7; r++) for (int q = 0; q < 9; q++) if (icon[i][r][q] == '#') gfx_rect(x + q * sc, y + r * sc, sc, sc, c & 255, (c >> 8) & 255, (c >> 16) & 255, 0); }

// ---- helpers ----
static void lt(int x, int y, const char *s) { gfx_text_raw(x, y, 1, s, 28, 44, 24); }
static void ltc(int cx, int y, const char *s) { gfx_text_raw(cx - gfx_text_w(1, s) / 2, y, 1, s, 28, 44, 24); }
static Toy *tt(void) { return &gtoy[ti]; }
static Blob *cb(void) { return &tt()->b[bi]; }
static Kin *ck(void) { return &tt()->k[bi]; }
static int mine(void) { return gotchi_sim_of(tt()) == player; }
static int owner_color(const Toy *t) { return t->uid % 6; }
static void say(const char *s) { msg = s; msgT = 150; }
static int living(Toy *t, int from, int dir) { for (int k = 1; k <= GB_N; k++) { int j = (from + dir * k + GB_N * 2) % GB_N; if (t->b[j].on) return j; } return from; }
static void cue(int v) { audio_voice(v); }

static int box_list(int *idx) { int n = 0; for (int i = 0; i < nsims && n < GT_MAX; i++) idx[n++] = i; return n; }
static void open_basket(int simi) { Toy *t = gotchi_toy(simi); if (!t) { say("NO BOREAGOTCHI YET. ELECTRONICS SHOPS SELL THEM"); return; } ti = (int)(t - gtoy); view = V_BASKET; bi = t->sel < GB_N ? t->sel : 0; msgT = 0; }
static const u8 *art; static int art_page = -1;
static u8 *art_buf;
static void guide_open(int back) {   // (the pictures are read a page at a time through the atlas's upload scratch, the upper half)
    art = 0; art_page = -1; art_buf = 0; if (ovl_installed("GOTCHI.OVL")) { art_buf = (u8 *)atlas_scratch_hi(); art = art_buf; }
    guidepage = 0; guideback = back; view = V_GUIDE;
}
static void goto_dex(int back) { dexback = back; dexsel = 0; view = V_DEX; }
void gotchi_open(int arg) {
    lrng = 0xB0BE5u + (u32)clock_min * 7; anim = 0; msgT = 0; msg = 0; dexsel = 0; bxsel = player; isel = 0; from_box = 0; sub = 0; danceT = 0; treeId = 0; gx_check();
    if (arg == 3) { guide_open(-1); return; }
    if (arg == 2) { goto_dex(-1); return; }
    if (arg == 1 && gotchi_toy(player)) { open_basket(player); return; }
    view = V_BOX; from_box = 1;
}

// ---- the games ----
static int cx_, cScore, cBad, cN, cY[3], cX[3], cK[3], cT;
static int hiNum, hiNext, hiGuess;
static void start_look(void) { view = V_LOOK; mgRound = 0; mgWon = 0; mgT = 0; mgN = 0; mgDir = 0; }
static void start_copy(void) { view = V_COPY; mgRound = 0; mgWon = 0; mgN = 2; for (int i = 0; i < 6; i++) seq[i] = (u8)(rr() & 3); sidx = 0; mgT = -60; }
static void start_catch(void) { view = V_CATCH; cx_ = 0; cScore = cBad = cN = cT = 0; for (int i = 0; i < 3; i++) { cY[i] = -i * 7; cX[i] = 4 + rr() % 24; cK[i] = rr() % 5 == 0; } mgT = -40; }
static void start_hilo(void) { view = V_HILO; mgRound = 0; mgWon = 0; hiNum = 1 + rr() % 9; hiGuess = 0; hiNext = 0; mgT = 0; }
static void finish_game(int won, int g) { view = V_HAND; say(gx_game_end(tt(), bi, won, g)); cue(won ? VOICE_WIN : VOICE_NAH); if (won) danceT = 150; }

static void use_icon(void) {
    Blob *b = cb(); Toy *t = tt(); int sl = (b->fl & GF_SLEEP) != 0;
    if (b->stage == GS_EGG && isel != 7 && isel != 8 && isel != 11) { say("IT IS STILL AN EGG. GIVE IT A MINUTE"); return; }
    if (sl && isel != 2 && isel != 7 && isel != 3 && isel != 8 && isel != 11 && isel != 10) { say("SHHH. IT IS ASLEEP"); return; }
    switch (isel) {
    case 0: sub = 0; listTop = 0; view = V_FEED; break;
    case 1: sub = 0; view = V_PLAYPICK; break;
    case 2: say(gx_light(t, bi)); break;
    case 3: say(gx_medicine(t, bi)); if (!(b->fl & GF_SICK)) cue(VOICE_FINISHED); break;
    case 4: say(gx_clean(t, bi)); break;
    case 5: say(gx_scold(t, bi)); break;
    case 6: say(gx_pet(t, bi)); cue(VOICE_YEHA); break;
    case 7: sub = 0; view = V_STATS; break;
    case 8: treeId = ck()->id; treeSel = 0; view = V_TREE; break;
    case 9: { int n = 0; for (int k = 0; k < GT_MAX; k++) if (gtoy[k].own) for (int i = 0; i < GB_N; i++) if ((k != ti || i != bi) && gtoy[k].b[i].on && gtoy[k].b[i].stage > GS_EGG) n++;
              if (!n) { say("YOU NEED ANOTHER PET, HATCHED, TO PLAY WITH"); return; } sub = 0; view = V_FRIEND; break; }
    case 10: sub = ck()->acc; view = V_DRESS; break;
    default: sub = 0; view = V_MORE; break;
    }
}
// the friends list: every hatched pet but this one, this toy's first, then the housemates'
static int fr_t[GT_MAX * GB_N], fr_s[GT_MAX * GB_N], fr_n;
static void fr_build(void) {
    fr_n = 0; for (int pass = 0; pass < 2; pass++) for (int k = 0; k < GT_MAX; k++) { if (!gtoy[k].own || (pass == 0) != (k == ti)) continue;
        for (int i = 0; i < GB_N; i++) if (gtoy[k].b[i].on && gtoy[k].b[i].stage > GS_EGG && (k != ti || i != bi)) { fr_t[fr_n] = k; fr_s[fr_n++] = i; } }
}

// ---- update ----
int gotchi_update(int hd, int pr, int steps) {
    int v0 = view; gotchi_live(steps); anim += steps; if (danceT > 0) danceT -= steps; if (msgT > 0) { msgT -= steps; if (msgT <= 0) { msgT = 0; msg = 0; } }
    switch (view) {
    case V_BOX: {
        int idx[GT_MAX], n = box_list(idx); if (!n) return 0;
        if (pr & PAD_UP) bxsel = (bxsel + n - 1) % n; if (pr & PAD_DOWN) bxsel = (bxsel + 1) % n;
        if (pr & PAD_TRIANGLE) goto_dex(V_BOX);
        if (pr & PAD_SQUARE) { view = V_AWARD; sub = 0; backv = V_BOX; }
        if (pr & PAD_SELECT) guide_open(V_BOX);
        if (pr & PAD_CIRCLE) return 0;
        if (pr & PAD_CROSS) { open_basket(bxsel); if (view == V_BASKET) from_box = 1; }
        break; }
    case V_BASKET: {
        Toy *t = tt();
        if (pr & PAD_LEFT) bi = (bi + GB_N - 1) % GB_N; if (pr & PAD_RIGHT) bi = (bi + 1) % GB_N;
        if (pr & PAD_UP) bi = (bi + GB_N - 4) % GB_N; if (pr & PAD_DOWN) bi = (bi + 4) % GB_N;
        t->sel = (u8)bi;
        if (pr & PAD_TRIANGLE) goto_dex(V_BASKET);
        if (pr & PAD_SQUARE) { view = V_HOME; homeSel = 0; backv = V_BASKET; }
        if (pr & PAD_SELECT) guide_open(V_BASKET);
        if (pr & PAD_CIRCLE) { if (from_box) { view = V_BOX; from_box = 1; } else return 0; }
        if (pr & PAD_CROSS) {
            if (!mine()) { say("THAT IS NOT YOUR TOY. YOU CAN ONLY LOOK"); break; }
            if (!t->b[bi].on) { if (cash < GOTCHI_EGG) { say("A NEW EGG COSTS $150"); cue(VOICE_NAH); } else { cash -= GOTCHI_EGG; gotchi_egg(t, bi); say("A NEW EGG! KEEP IT CLOSE"); cue(VOICE_YAHOO); } }
            else { view = V_HAND; isel = 0; msgT = 0; }
        }
        break; }
    case V_HAND: {
        Toy *t = tt();
        if (pr & PAD_RIGHT) isel = (isel + 1) % NICON; if (pr & PAD_LEFT) isel = (isel + NICON - 1) % NICON;
        if (pr & (PAD_UP | PAD_DOWN)) isel = (isel + 6) % NICON;
        if (pr & PAD_R1) bi = living(t, bi, 1); if (pr & PAD_L1) bi = living(t, bi, -1);
        t->sel = (u8)bi;
        if (pr & PAD_CIRCLE) { view = V_BASKET; msgT = 0; }
        if (pr & PAD_CROSS) use_icon();
        { static int lm; int m = mood_of(cb()); if (m != lm) { if (m == MD_HUNGRY) cue(VOICE_HUNGRY); else if (m == MD_SICK) cue(VOICE_NEED); else if (m == MD_ANGRY) cue(VOICE_ANGRY); lm = m; } }
        break; }
    case V_FEED: {
        if (pr & PAD_RIGHT) { sub = (sub + 1) % 3; listTop = 0; } if (pr & PAD_LEFT) { sub = (sub + 2) % 3; listTop = 0; }
        if (sub < 2) { if (pr & PAD_DOWN) listTop = (listTop + 1) % 6; if (pr & PAD_UP) listTop = (listTop + 5) % 6; }
        if (pr & PAD_CIRCLE) { view = V_HAND; return 1; }
        if (pr & PAD_CROSS) { const char *m = sub == 2 ? gx_lunch(tt(), bi) : gx_feed(tt(), bi, sub * 6 + listTop); say(m); view = V_HAND;
            cue(m[0] == 'Y' && m[1] == 'U' && m[2] == 'C' ? VOICE_NAH : VOICE_YAHOO); }
        break; }
    case V_PLAYPICK:
        if (pr & PAD_RIGHT) sub = (sub + 1) % NGAME; if (pr & PAD_LEFT) sub = (sub + NGAME - 1) % NGAME; if (pr & PAD_UP) sub = (sub + 2) % NGAME; if (pr & PAD_DOWN) sub = (sub + 2) % NGAME;
        if (pr & PAD_CIRCLE) view = V_HAND;
        if (pr & PAD_CROSS) { if (sub == 0) start_look(); else if (sub == 1) start_copy(); else if (sub == 2) start_catch(); else start_hilo(); }
        break;
    case V_LOOK:   // WHICH WAY? it looks left or right: guess first. 5 rounds, 3 to win
        if (mgN == 0) { if (pr & PAD_LEFT) { mgDir = -1; mgN = 1; mgT = 0; } if (pr & PAD_RIGHT) { mgDir = 1; mgN = 1; mgT = 0; } if (pr & PAD_CIRCLE) view = V_HAND;
            if (mgN) { sidx = (u8)((rr() & 1) ? 1 : 0); } }
        else { mgT += steps; if (mgT > 50) { int way = sidx ? 1 : -1; if (way == mgDir) { mgWon++; cue(VOICE_YEHA); } mgRound++; mgN = 0; if (mgRound >= 5) finish_game(mgWon >= 3, 0); } }
        break;
    case V_COPY: {   // COPY ME: watch the blob, then do the same moves (the D-pad). Three rounds, two to win
        if (pr & PAD_CIRCLE) { view = V_HAND; break; }
        mgT += steps;
        if (mgT < 0) break;
        if (mgT < mgN * 40) break;
        int key = (pr & PAD_UP) ? 0 : (pr & PAD_DOWN) ? 1 : (pr & PAD_LEFT) ? 2 : (pr & PAD_RIGHT) ? 3 : -1;
        if (key >= 0) { if (key == seq[sidx]) { sidx++; cue(VOICE_YEHA); if (sidx >= mgN) { mgWon++; mgRound++; sidx = 0; mgN++; mgT = -50; if (mgRound >= 3) finish_game(1, 1); } }
                        else { mgRound++; sidx = 0; mgT = -50; cue(VOICE_NAH); if (mgRound - mgWon >= 2) finish_game(0, 1); else if (mgRound >= 3) finish_game(mgWon >= 2, 1); } }
        break; }
    case V_CATCH: {   // CATCH: slide under the falling treats, dodge the mess. 14 treats, 8 to win
        if (pr & PAD_CIRCLE) { view = V_HAND; break; }
        mgT += steps; if (mgT < 0) break;
        if (hd & PAD_LEFT) cx_ = MAX(-12, cx_ - steps); if (hd & PAD_RIGHT) cx_ = MIN(12, cx_ + steps);
        cT += steps; while (cT >= 7) { cT -= 7;
            for (int i = 0; i < 3; i++) { cY[i]++; if (cY[i] >= 12 && cY[i] <= 13) { int dx = cX[i] - (16 + cx_ / 2 * 2 / 2 * 0 + cx_); if (dx >= -4 && dx <= 4 && cY[i] == 12) { if (cK[i]) { cBad++; cue(VOICE_NAH); } else { cScore++; cue(VOICE_YEHA); } cY[i] = 99; } }
                if (cY[i] >= 15) { cN++; cY[i] = -(rr() % 6); cX[i] = 4 + rr() % 24; cK[i] = rr() % 5 == 0; } } }
        if (cN >= 14) finish_game(cScore >= 8 && cBad < 3, 2);
        break; }
    case V_HILO: {
        if (pr & PAD_CIRCLE) { view = V_HAND; break; }
        if (hiNext) { mgT += steps; if (mgT > 55) { hiNext = 0; hiNum = 1 + rr() % 9; mgRound++; if (mgRound >= 5) finish_game(mgWon >= 3, 3); } }
        else if (pr & (PAD_UP | PAD_DOWN)) { int nx = 1 + rr() % 9; if (nx == hiNum) nx = nx % 9 + 1; hiGuess = (pr & PAD_UP) ? 1 : -1; hiNext = nx; mgT = 0; if ((nx > hiNum) == (hiGuess > 0)) { mgWon++; cue(VOICE_YEHA); } else cue(VOICE_NAH); }
        break; }
    case V_FRIEND: {
        fr_build(); if (!fr_n) { view = V_HAND; break; } if (sub >= fr_n) sub = 0;
        if (pr & PAD_RIGHT) sub = (sub + 1) % fr_n; if (pr & PAD_LEFT) sub = (sub + fr_n - 1) % fr_n;
        if (pr & PAD_DOWN) { int k = fr_t[sub]; while (fr_t[sub] == k) sub = (sub + 1) % fr_n; } if (pr & PAD_UP) { int k = fr_t[sub]; while (fr_t[sub] == k) sub = (sub + fr_n - 1) % fr_n; }
        if (pr & PAD_CIRCLE) view = V_HAND;
        if (pr & (PAD_CROSS | PAD_SQUARE)) { const char *m = gx_meet(tt(), bi, &gtoy[fr_t[sub]], fr_s[sub], (pr & PAD_SQUARE) != 0); view = V_HAND; say(m); cue(VOICE_LAUGH); danceT = 100; }
        break; }
    case V_STATS:
        if (pr & (PAD_CROSS | PAD_RIGHT | PAD_DOWN)) { if (++sub >= 3) view = V_HAND; }
        if (pr & (PAD_CIRCLE | PAD_LEFT | PAD_UP)) { if (sub > 0) sub--; else view = V_HAND; }
        break;
    case V_DRESS:
        if (pr & PAD_RIGHT) sub = (sub + 1) % NACC; if (pr & PAD_LEFT) sub = (sub + NACC - 1) % NACC;
        if (pr & PAD_CIRCLE) view = V_HAND;
        if (pr & PAD_CROSS) { const char *m = gx_dress(tt(), bi, sub); say(m); if (m[0] == 'L') { cue(VOICE_YAHOO); danceT = 80; } else if (m[0] == 'N') cue(VOICE_NAH); }
        break;
    case V_MORE:
        if (pr & PAD_DOWN) sub = (sub + 1) % 5; if (pr & PAD_UP) sub = (sub + 4) % 5; if (pr & PAD_CIRCLE) view = V_HAND;
        if (pr & PAD_CROSS) { backv = V_MORE; if (sub == 0) { view = V_HOME; homeSel = 0; } else if (sub == 1) view = V_DIARY; else if (sub == 2) { view = V_AWARD; sub = 0; } else if (sub == 3) goto_dex(V_MORE); else guide_open(V_MORE); }
        break;
    case V_GUIDE:
        if (pr & (PAD_R1 | PAD_RIGHT | PAD_CROSS)) guidepage = (guidepage + 1) % 14; if (pr & (PAD_L1 | PAD_LEFT)) guidepage = (guidepage + 13) % 14;
        if (pr & PAD_CIRCLE) { if (guideback < 0) return 0; view = guideback; }
        break;
    case V_DEX:
        if (pr & PAD_RIGHT) dexsel = (dexsel + 1) % GSP_N; if (pr & PAD_LEFT) dexsel = (dexsel + GSP_N - 1) % GSP_N; if (pr & PAD_DOWN) dexsel = (dexsel + 5) % GSP_N; if (pr & PAD_UP) dexsel = (dexsel + GSP_N - 5) % GSP_N;
        if (pr & PAD_CROSS) { dexf = 0; view = V_DEXD; }
        if (pr & PAD_CIRCLE) { if (dexback < 0) return 0; view = dexback; }
        break;
    case V_DEXD:
        if (pr & (PAD_RIGHT | PAD_R1)) dexf = (dexf + 1) % GFORMS; if (pr & (PAD_LEFT | PAD_L1)) dexf = (dexf + GFORMS - 1) % GFORMS; if (pr & PAD_DOWN) dexf = (dexf + 3) % GFORMS; if (pr & PAD_UP) dexf = (dexf + 3) % GFORMS;
        if (pr & PAD_CIRCLE) view = V_DEX;
        break;
    case V_TREE: case V_DIARY: case V_AWARD: case V_HOME: break;
    }
    if (view != v0) pr = 0;
    if (view == V_TREE) { extern void tree_input(int pr); tree_input(pr); }
    else if (view == V_AWARD) { if (pr & PAD_DOWN) sub = (sub + 1) % NACH; if (pr & PAD_UP) sub = (sub + NACH - 1) % NACH; if (pr & PAD_CIRCLE) view = backv; }
    else if (view == V_DIARY) { if (pr & PAD_CIRCLE) view = V_MORE; }
    else if (view == V_HOME) {
        Toy *t = tt();
        if (pr & PAD_RIGHT) homeSel = (homeSel + 1) % 6; if (pr & PAD_LEFT) homeSel = (homeSel + 5) % 6;
        if (pr & PAD_SQUARE) t->room = (u8)((t->room + 1) % 6);
        if ((pr & PAD_CROSS) && mine()) { t->deco ^= (u8)(1 << homeSel); cue(VOICE_YEHA); }
        if (pr & PAD_CIRCLE) view = backv == V_MORE ? V_MORE : V_BASKET;
    }
    return 1;
}

// ---- draw ----
#define IW 72
#define IH 54
#define NIMG 8
static u32 hsh(u32 a) { a ^= a << 13; a ^= a >> 17; a ^= a << 5; return a * 2654435761u; }
static void tshadow(int x, int y, const char *s, int r, int g, int b) { gfx_text_raw(x + 1, y + 1, 1, s, 20, 8, 30); gfx_text_raw(x, y, 1, s, r, g, b); }
static void tshadowc(int cx, int y, const char *s, int r, int g, int b) { tshadow(cx - gfx_text_w(1, s) / 2, y, s, r, g, b); }
static void chip(int cx, int y, const char *s, u32 bg, u32 ink) { int w = ui_tw(1, s) + 10; ui_badge(cx - w / 2, y, s, bg, ink); }
static const u32 fcol[GFORMS] = { UI_RGB(236, 176, 40), UI_RGB(150, 112, 70), UI_RGB(60, 190, 210), UI_RGB(220, 70, 180), UI_RGB(230, 90, 50), UI_RGB(240, 150, 170) };
static void formchip(const Blob *b, int cx, int y) {
    if (b->stage == GS_EGG) chip(cx, y, "EGG", UI_AMBER, UI_INK);
    else if (b->stage == GS_BABY) chip(cx, y, "BABY", UI_GREEN, UI_WHITE);
    else if (b->stage == GS_CHILD) chip(cx, y, "CHILD", UI_BLUE, UI_WHITE);
    else if (b->stage == GS_TEEN) chip(cx, y, b->form ? "SLOUCHY" : "ANGSTY", b->form ? UI_RGB(150, 112, 70) : UI_BLUE2, UI_WHITE);
    else if (b->stage == GS_ELDER) chip(cx, y, "ELDER", UI_RGB(130, 130, 150), UI_WHITE);
    else chip(cx, y, gx_form(b->form), fcol[b->form % GFORMS], (b->form % GFORMS) == 0 ? UI_INK : UI_WHITE);
}
static void lcd_tile(const Blob *b, const Kin *k, int x, int y, int fr, int look) {   // a pet on a little rounded LCD, 64 x 32
    ui_round(x - 2, y - 2, 68, 36, 5, UI_RGB(52, 24, 70), UI_RGB(30, 12, 44), 0);
    scene(b, k, fr, look, mood_of(b)); lcd_bg(x, y, 64, 32, 0); dots_all(x, y, 2);
}
static void mini_tile(const Blob *b, const Kin *k, int x, int y, int w, int dim) {   // a 1-pixel LCD tile for the tree: crops the dots' empty sides
    ui_round(x - 1, y - 1, w + 2, 20, 3, dim ? UI_RGB(90, 90, 110) : UI_RGB(52, 24, 70), dim ? UI_RGB(60, 60, 80) : UI_RGB(30, 12, 44), 0);
    scene(b, k, 0, 0, MD_NORM); gfx_rect(x, y, w, 18, dim ? 150 : 172, dim ? 158 : 196, dim ? 150 : 128, 0); dots_draw(x + (w - 24) / 2, y + 1, 1, 0, 16, 4, 28);
}
static void sky(void) {
    for (int y = 0; y < 240; y += 8) { int g = y * 20 / 240; gfx_rect(0, y, 320, 8, 22 - g, 12 - g / 2, 52 - g, 0); }
    for (int i = 0; i < 28; i++) { u32 h = hsh(i * 77 + 5); int x = (int)(h % 320), y = (int)((h >> 9) % 240), tw = ((anim >> 3) + i) & 7; gfx_rect(x, y, tw == 0 ? 3 : 2, tw == 0 ? 3 : 2, 200, 190, 255, 1); }
}
static void kin_blob(Blob *b, Kin *k, const Line *l) {   // a creature made from a line record (a memorial, a relative in the tree)
    memset(b, 0, sizeof *b); memset(k, 0, sizeof *k); b->on = 1; b->stage = GS_ADULT; b->sp = l->sp; b->form = (u8)((l->fc & 7) % GFORMS); b->hun = b->hap = 4; k->col = (u8)((l->fc >> 3) & 7); k->mut = l->mut; k->pers = l->pg & 7; b->nm = l->nm;
}
static void draw_box(void) {
    sky();
    int top = ui_window(14, 8, 292, 224, "TOY BOX", IC_STAR);
    int idx[GT_MAX], n = box_list(idx); int y = top + 3;
    for (int k = 0; k < n; k++) {
        int si = idx[k]; Toy *t = gotchi_toy(si); int sel = k == bxsel;
        ui_round(20, y, 280, 28, 7, sel ? UI_HILITE : UI_WHITE, sel ? UI_HILITE : UI_PANEL, 0);
        if (sel) ui_round(19, y - 1, 282, 30, 8, UI_BLUE, UI_BLUE, 1);
        if (t) { const u8 *sh = PAL[1 + owner_color(t) % 7]; ui_disc(36, y + 14, 22, UI_RGB(sh[0], sh[1], sh[2])); ui_icon(30, y + 8, IC_SMILE, UI_WHITE); }
        else { ui_disc(36, y + 14, 22, UI_LINE); ui_icon(30, y + 8, IC_LOCK, UI_WHITE); }
        ui_text(54, y + 4, 1, sims[si].bore.first, UI_INK);
        if (t) {
            char b[48]; int gens = 0; for (int i = 0; i < GB_N; i++) if (t->b[i].on && t->b[i].gen > gens) gens = t->b[i].gen;
            sprintf(b, "LV %d  %s  GEN %d", gotchi_level(t), gx_level(gotchi_level(t)), gens + 1); ui_text(54, y + 16, 1, b, UI_SOFT);
            for (int i = 0; i < GB_N; i++) { const Blob *p = &t->b[i]; u32 c = !p->on ? UI_LINE : gotchi_needs(p) ? UI_RED : p->stage == GS_EGG ? UI_AMBER : UI_GREEN; ui_disc(190 + i * 11, y + 9, 8, c); }
            if (gotchi_calling(t)) { sprintf(b, "%d CALLING", gotchi_calling(t)); ui_badge(190, y + 15, b, UI_RED, UI_WHITE); } else ui_badge(190, y + 15, "ALL GOOD", UI_GREEN, UI_WHITE);
        } else ui_text(54, y + 16, 1, "NO TOY YET: $5000 AT ELECTRONICS SHOPS", UI_SOFT);
        y += 31;
    }
    { char b[64]; sprintf(b, "BLOBDEX %d OF %d    CARE SCORE %d", gotchi_dex_count(), GSP_N * GFORMS, gx_score()); ui_bar(60, 194, 200, 7, gotchi_dex_count() * 100 / (GSP_N * GFORMS), UI_AMBER); ui_textc(160, 184, 1, b, UI_GOLDT); }
    if (msg) ui_textc(160, 172, 1, msg, UI_BADT);
    ui_hints(160, 212, "X OPEN|TRI DEX|SQ AWARDS|SEL HELP", 1);
}
static void draw_basket(void) {
    sky(); Toy *t = tt();
    char title[40]; int si = gotchi_sim_of(t); sprintf(title, "%s'S BASKET", si >= 0 ? sims[si].bore.first : "THE");
    int top = ui_window(14, 8, 292, 224, title, IC_STAR);
    for (int i = 0; i < GB_N; i++) {
        Blob *b = &t->b[i]; Kin *k = &t->k[i]; int x = 20 + (i & 3) * 70, y = top + 3 + (i >> 2) * 80, sel = i == bi;
        ui_round(x, y, 66, 76, 7, sel ? UI_HILITE : UI_WHITE, sel ? UI_HILITE : UI_PANEL, 0);
        if (sel) ui_round(x - 1, y - 1, 68, 78, 8, UI_BLUE, UI_BLUE, 1);
        if (b->on) {
            lcd_tile(b, k, x + 1, y + 3, anim / 20 + i, 0);
            if (gotchi_needs(b)) { ui_disc(x + 59, y + 8, 13, UI_RED); ui_textc(x + 59, y + 3, 1, "!", UI_WHITE); }
            if (k->mate) ui_icon_s(x + 3, y + 6, IC_HEART, 10, UI_RED);
            ui_textfit(x + 4, y + 40, 58, gx_given(b->nm), UI_INK); formchip(b, x + 33, y + 51);
            for (int h = 0; h < 4; h++) ui_icon_s(x + 8 + h * 13, y + 63, IC_HEART, 10, h < (b->bond + 12) / 25 ? UI_RED : UI_LINE);
        } else { ui_icon_s(x + 21, y + 14, IC_ADD, 24, UI_LINE); ui_textc(x + 33, y + 46, 1, sel ? "NEW EGG" : "EMPTY", sel ? UI_BLUE2 : UI_SOFT); ui_textc(x + 33, y + 58, 1, "$150", UI_GOLDT); }
    }
    { int y = top + 166; char b[40]; sprintf(b, "LV %d", gotchi_level(t)); ui_badge(22, y, b, UI_BLUE2, UI_WHITE); sprintf(b, "%d HATCHED", t->hatched); ui_badge(66, y, b, UI_GREEN, UI_WHITE); sprintf(b, "%d PLAYS", t->plays); ui_badge(150, y, b, UI_AMBER, UI_INK); sprintf(b, "SCORE %d", gx_score()); ui_badge(224, y, b, UI_RGB(150, 100, 210), UI_WHITE); }
    if (msg) ui_textc(160, 178, 1, msg, UI_BADT);
    ui_hints(160, 212, "X OPEN|SQ HOME|TRI DEX|SEL HELP", 1);
}
// ---- the handheld ----
static void body(int c) {
    const u8 *sh = PAL[1 + c % 7]; u32 top = UI_RGB(MIN(255, sh[0] + 40), MIN(255, sh[1] + 40), MIN(255, sh[2] + 40)), bot = UI_RGB(sh[0] * 6 / 10, sh[1] * 6 / 10, sh[2] * 6 / 10);
    ui_round(12, 7, 296, 226, 26, bot, bot, 0); ui_round(14, 8, 292, 222, 26, top, UI_RGB(sh[0], sh[1], sh[2]), 0);
    ui_round(24, 11, 272, 6, 3, UI_RGB(255, 255, 255), UI_RGB(255, 255, 255), 1);
    ui_disc(160, 8, 16, UI_RGB(190, 192, 205)); ui_disc(160, 8, 7, UI_RGB(20, 10, 40));   // the keyring
}
static void lit_icon(int i, int cx, int y, int on, int lit) {
    u32 top = on ? UI_RGB(255, 255, 255) : lit ? UI_RGB(255, 200, 70) : UI_RGB(86, 36, 104), bot = on ? UI_RGB(190, 222, 255) : lit ? UI_RGB(230, 140, 20) : UI_RGB(50, 18, 66);
    int hop = on ? ((anim >> 3) & 1) * -2 : 0;
    ui_round(cx - 20, y + hop, 40, 28, 6, top, bot, 0); if (on) ui_round(cx - 21, y - 1 + hop, 42, 30, 7, UI_BLUE, UI_BLUE, 1);
    icon_bits(i, cx - 9, y + 3 + hop, 2, on ? 0x6A3C2E : lit ? 0x1C3C70 : 0xC890F0);
    int tw = gfx_text_w(1, iconNm[i]); gfx_text_raw(cx - tw / 2, y + 19 + hop, 1, iconNm[i], on ? 60 : 210, on ? 40 : 190, on ? 100 : 240);
}
static const u8 iconMap[NICON] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static void bar3(int x, int y, const char *l, int v, int r, int g, int bl) {   // a labelled stat bar with its number, on the body
    char n[16]; sprintf(n, "%d", v); gfx_text_raw(x, y, 1, l, 252, 248, 255); gfx_rect(x, y + 10, 32, 6, 30, 14, 46, 0); gfx_rect(x + 1, y + 11, 30 * CLAMP(v, 0, 100) / 100, 4, r, g, bl, 0); gfx_text_raw(x + 35, y + 9, 1, n, 255, 255, 255);
}
static void draw_hand(int lcdMode) {
    Toy *t = tt(); Blob *b = cb(); Kin *k = ck(); int dark = (b->fl & GF_SLEEP) && (b->fl & GF_DARK), md = mood_of(b); char s[64];
    sky(); body(owner_color(t));
    ui_round(72, 14, 176, 102, 10, UI_RGB(54, 26, 74), UI_RGB(26, 10, 40), 0);   // the LCD, big, with a bezel
    lcd_bg(80, 18, 160, 80, dark);
    if (lcdMode == 0) { scene(b, k, anim / 20, 0, md); dots_all(80, 18, 5); if (gotchi_time == 1) ui_badge(82, 20, "LIVE", UI_RED, UI_WHITE); else if (gotchi_time == 2) ui_badge(82, 20, "PAUSED", UI_SOFT, UI_WHITE); }
    gfx_rect(80, 100, 160, 12, dark ? 70 : 150, dark ? 84 : 176, dark ? 62 : 108, 0);
    if (lcdMode == 0) { char nm[40]; gx_name(b, nm); sprintf(s, "%s THE %s", gx_given(b->nm), nm); gfx_text_raw(160 - gfx_text_w(1, s) / 2, 102, 1, s, 28, 44, 24); }
    bar3(20, 22, "HUNGER", b->hun * 25, 255, 170, 40); bar3(20, 52, "HAPPY", b->hap * 25, 255, 100, 170); bar3(20, 82, "BOND", b->bond, 255, 60, 70);   // stat bars down both sides, with numbers
    bar3(260, 22, "DISCIP", b->disc, 90, 200, 255); gfx_text_raw(260, 52, 1, "AGE", 252, 248, 255); gfx_text_raw(260, 63, 1, gx_stage(b->stage), 255, 235, 140);
    sprintf(s, "%dG", b->wt); gfx_text_raw(260, 82, 1, "WEIGHT", 252, 248, 255); gfx_text_raw(260, 93, 1, s, 255, 235, 140);
    chip(100, 119, gx_pers(k->pers), UI_RGB(120, 70, 180), UI_WHITE); formchip(b, 160, 119); if (k->mate) ui_badge(196, 119, "PAIRED", UI_RED, UI_WHITE); else if (b->gen) { sprintf(s, "GEN %d", b->gen + 1); ui_badge(204, 119, s, UI_AMBER, UI_INK); }
    for (int i = 0; i < NICON; i++) lit_icon(i, 54 + (i % 6) * 42, i < 6 ? 135 : 166, i == isel && lcdMode == 0, i == 6 && (b->fl & GF_CALL));
    ui_round(20, 197, 280, 13, 6, UI_RGB(255, 255, 255), UI_RGB(206, 222, 250), 0); ui_textfit(26, 200, 268, lcdMode == 0 ? iconHint[isel] : "", UI_INK);
    ui_round(20, 211, 280, 20, 8, UI_RGB(24, 10, 40), UI_RGB(12, 4, 24), 0);
    if (msg) ui_wrapc(26, 213, 268, 2, msg, UI_WHITE); else if (lcdMode == 0) ui_hints(160, 217, "X USE|L1 R1 PET|O BASKET", 1);
}
static void lcd_panel(void) { lcd_bg(80, 18, 160, 94, 0); }
static void draw_stats(void) {
    draw_hand(1); lcd_panel(); Toy *t = tt(); Blob *b = cb(); Kin *k = ck(); char s[64], nm[40]; gx_name(b, nm);
    if (sub == 0) {
        ltc(160, 20, "NEEDS"); static const char *const l[3] = { "HUNGER", "HAPPY", "BOND" }; int v[3] = { b->hun * 25, b->hap * 25, b->bond };
        for (int i = 0; i < 3; i++) { lt(84, 36 + i * 17, l[i]); ui_bar(130, 35 + i * 17, 80, 9, v[i], i == 0 ? UI_AMBER : i == 1 ? UI_RGB(240, 100, 160) : UI_RED); sprintf(s, "%d", v[i]); lt(216, 36 + i * 17, s); }
        sprintf(s, "DISCIPLINE %d  MISTAKES %d", b->disc, b->miss); lt(84, 90, s); sprintf(s, "WEIGHT %d G   %s", b->wt, (b->fl & GF_SICK) ? "SICK" : b->poop ? "MESSY" : "WELL"); lt(84, 101, s);
    } else if (sub == 1) {
        lt(84, 20, nm); sprintf(s, "%s %s   GEN %d", gx_colour(k->col), gx_species(b->sp), b->gen + 1); lt(84, 31, s);
        sprintf(s, "%s: %s", gx_pers(k->pers), gx_pers_note(k->pers)); ui_wrap(84, 42, 152, 2, s, UI_RGB(28, 44, 24));
        sprintf(s, "LIKES %s", gx_food(k->taste >> 4)); lt(84, 64, s); sprintf(s, "HATES %s", gx_food(k->taste & 15)); lt(84, 75, s);
        sprintf(s, "AGE %d HOURS IN THIS STAGE", b->age / 60); lt(84, 86, s); sprintf(s, "%s%s", k->mut ? "RARE MUTATION! " : "", k->acc ? gx_acc(k->acc) : "NO HAT"); lt(84, 97, s);
    } else {
        ltc(160, 20, "YOU AS A CARER"); sprintf(s, "LEVEL %d: %s", gotchi_level(t), gx_level(gotchi_level(t))); lt(84, 36, s);
        int nx = gotchi_next_xp(t); if (nx) ui_bar(84, 50, 150, 6, t->xp * 100 / nx, UI_RGB(28, 44, 24));
        sprintf(s, "CARE SCORE %d", gx_score()); lt(84, 62, s); sprintf(s, "%d HATCHED  %d WINS", t->hatched, t->wins); lt(84, 74, s); sprintf(s, "%d PLAYDATES AND GAMES", t->plays); lt(84, 86, s);
        sprintf(s, "%d AWARDS OF %d", ({ int c = 0; for (int i = 0; i < NACH; i++) c += (gach >> i) & 1; c; }), NACH); lt(84, 98, s);
    }
    { char p[16]; sprintf(p, "%d / 3", sub + 1); lt(214, 20, p); }
    ui_hints(160, 217, "X MORE|O BACK", 1);
}
static void tab(int x, int w, const char *l, int on) { ui_round(x, 22, w, 12, 5, on ? UI_BLUE : UI_CARD2, on ? UI_BLUE2 : UI_CARD2, 0); ui_textc(x + w / 2, 24, 1, l, on ? UI_WHITE : UI_INK); }
static void draw_feed(void) {
    draw_hand(1); lcd_panel(); Kin *k = ck(); char s[32];
    tab(84, 46, "MEALS", sub == 0); tab(134, 46, "SNACKS", sub == 1); tab(184, 52, "LUNCH", sub == 2);
    if (sub == 2) { ui_wrapc(86, 46, 148, 4, "SHARE YOUR OWN LUNCH. IT IS FULL RIGHT UP, BUT YOUR TUMMY GROWLS AND YOU GET HUNGRIER", UI_INK); }
    else for (int i = 0; i < 6; i++) { int f = sub * 6 + i, y = 38 + i * 12, on = i == listTop;
        if (on) gfx_rect(84, y - 1, 152, 12, 255, 255, 255, 1); lt(88, y + 1, gx_food(f));
        if ((k->taste >> 4) == f) ui_icon_s(188, y, IC_HEART, 10, UI_RED); else if ((k->taste & 15) == f) { gfx_text_raw(188, y + 1, 1, "YUCK", 200, 50, 50); }
        if (gx_food_cost(f)) { sprintf(s, "$%d", gx_food_cost(f)); gfx_text_raw(236 - gfx_text_w(1, s) - 2, y + 1, 1, s, 120, 80, 10); } else if (on) { } }
    ui_hints(160, 217, "X GIVE|LR TAB|UD PICK|O BACK", 1);
}
static void draw_playpick(void) {
    draw_hand(1); lcd_panel(); ltc(160, 22, "PICK A GAME");
    static const char *const how[NGAME] = { "GUESS WHERE IT LOOKS. 5 ROUNDS", "REPEAT ITS MOVES. 3 ROUNDS", "SLIDE TO CATCH TREATS, DODGE MESS", "WILL THE NEXT NUMBER BE HIGHER OR LOWER?" };
    for (int i = 0; i < NGAME; i++) { int x = 84 + (i & 1) * 78, y = 36 + (i >> 1) * 20; ui_round(x, y, 74, 17, 5, i == sub ? UI_BLUE : UI_CARD2, i == sub ? UI_BLUE2 : UI_CARD2, 0); ui_textc(x + 37, y + 5, 1, gx_game(i), i == sub ? UI_WHITE : UI_INK); }
    ui_wrapc(86, 82, 148, 3, how[sub], UI_INK); ui_hints(160, 217, "X PLAY|O BACK", 1);
}
static void game_scene(int look, int ox, int mood) { Blob *b = cb(); scene(b, ck(), anim / 20, look, mood); }
static void draw_look(void) {
    draw_hand(1); lcd_panel(); char s[40];
    game_scene(mgN ? (sidx ? 1 : -1) : 0, 0, MD_NORM); dots_draw(80, 18, 5, 0, 16, 0, 32);
    sprintf(s, "ROUND %d  WINS %d", mgRound + 1, mgWon); tshadowc(160, 118, s, 255, 235, 140);
    msg = mgN == 0 ? "WHICH WAY WILL IT LOOK? LEFT OR RIGHT" : ((sidx ? 1 : -1) == mgDir ? "YES! IT LOOKED YOUR WAY" : "NOPE. THE OTHER WAY"); msgT = 2;
}
static void draw_copy(void) {
    draw_hand(1); lcd_panel(); char s[40]; static const char *nm[4] = { "UP", "DOWN", "LEFT", "RIGHT" };
    int showing = mgT >= 0 && mgT < mgN * 40, k = showing ? mgT / 40 : -1, look = 0, jump = 0;
    if (showing) { int m = seq[k]; look = m == 2 ? -1 : m == 3 ? 1 : 0; jump = m == 0; }
    memset(dot, 0, sizeof dot); scene(cb(), ck(), jump ? 1 : 0, look, jump ? MD_HAPPY : MD_NORM); dots_draw(80, 18 - (jump ? 6 : 0), 5, 0, 16, 0, 32);
    sprintf(s, "ROUND %d  GOT %d", mgRound + 1, mgWon); tshadowc(160, 118, s, 255, 235, 140);
    if (mgT < 0) msg = "WATCH CLOSELY..."; else if (showing) msg = nm[seq[k]]; else { static char m2[32]; sprintf(m2, "YOUR TURN: %d OF %d", sidx + 1, mgN); msg = m2; } msgT = 2;
}
static void draw_catch(void) {
    draw_hand(1); lcd_panel(); char s[40]; Blob *b = cb();
    memset(dot, 0, sizeof dot); minY = 16; lcdCol = ck()->col; Blob g = *b; g.stage = MIN(g.stage, GS_CHILD); if (g.stage < GS_CHILD) g.stage = GS_CHILD; creature(&g, ck(), anim / 8, 1, cx_, MD_NORM);
    for (int i = 0; i < 3; i++) if (cY[i] >= 0 && cY[i] < 15) { if (cK[i]) blit(bPoop, 4, cX[i] - 1, cY[i] - 1, 1); else { star(cX[i], cY[i], 4); } }
    dots_draw(80, 18, 5, 0, 16, 0, 32);
    sprintf(s, "TREATS %d   MESS %d   %d LEFT", cScore, cBad, 14 - cN); tshadowc(160, 118, s, 255, 235, 140); msg = mgT < 0 ? "GET READY..." : "LEFT AND RIGHT: CATCH THE STARS, DODGE THE MESS"; msgT = 2;
}
static void draw_hilo(void) {
    draw_hand(1); lcd_panel(); char s[40]; Blob *b = cb();
    memset(dot, 0, sizeof dot); minY = 16; lcdCol = ck()->col; Blob g = *b; if (g.stage < GS_CHILD) g.stage = GS_CHILD; creature(&g, ck(), anim / 20, 1, -9, hiNext ? ((hiNext > hiNum) == (hiGuess > 0) ? MD_HAPPY : MD_BORED) : MD_NORM);
    dots_draw(80, 18, 5, 0, 16, 0, 32);
    sprintf(s, "%d", hiNum); gfx_text_raw(188, 30, 3, s, 28, 44, 24);
    if (hiNext) { sprintf(s, "%d", hiNext); gfx_text_raw(188, 62, 3, s, (hiNext > hiNum) == (hiGuess > 0) ? 30 : 200, (hiNext > hiNum) == (hiGuess > 0) ? 130 : 40, 40); }
    sprintf(s, "ROUND %d  WINS %d", mgRound + 1, mgWon); tshadowc(160, 118, s, 255, 235, 140); msg = hiNext ? ((hiNext > hiNum) == (hiGuess > 0) ? "RIGHT!" : "WRONG. NEXT ONE") : "UP: HIGHER    DOWN: LOWER"; msgT = 2;
}
static const char *relname(int a, int c) {
    const Line *x = gotchi_line(a), *y = gotchi_line(c); if (!x || !y) return "STRANGERS"; if (a == c) return "SAME";
    if (x->mom == c || x->dad == c) return "THEIR PARENT"; if (y->mom == a || y->dad == a) return "THEIR CHILD";
    if (gx_kin(a, c)) return (x->mom && x->mom == y->mom) || (x->dad && x->dad == y->dad) ? "SIBLINGS" : "FAMILY"; return "STRANGERS";
}
static void draw_friend(void) {
    draw_hand(1); fr_build(); if (!fr_n) return; if (sub >= fr_n) sub = 0; Toy *t = tt(); Toy *u = &gtoy[fr_t[sub]]; Blob *c = &u->b[fr_s[sub]]; Kin *kc = &u->k[fr_s[sub]];
    lcd_panel(); scene(c, kc, anim / 20, 0, mood_of(c)); dots_draw(80, 18, 4, 0, 16, 0, 32); char s[64], nm[40]; gx_name(c, nm); int os = gotchi_sim_of(u);
    sprintf(s, "%s THE %s", gx_given(c->nm), nm); tshadowc(160, 118, s, 255, 235, 140);
    sprintf(s, u == t ? "YOURS" : "%s'S", os >= 0 ? sims[os].bore.first : "?"); lt(84, 22, s); lt(84, 33, relname(ck()->id, kc->id)); sprintf(s, "%d / %d", sub + 1, fr_n); lt(84, 44, s);
    ui_icon_s(210, 22, IC_HEART, 12, c->bond >= 60 ? UI_RED : UI_LINE);
    msg = "X: PLAYDATE    SQUARE: BATTLE    <> PICK A FRIEND, UD SWAP FAMILY"; msgT = 2;
}
static void draw_dress(void) {
    draw_hand(1); lcd_panel(); Blob *b = cb(); Kin k = *ck(); k.acc = (u8)sub; char s[40];
    scene(b, &k, anim / 20, 0, danceT > 0 ? MD_DANCE : MD_HAPPY); dots_draw(80, 18, 4, 0, 16, 0, 32);
    ltc(160, 20, gx_acc(sub)); { const Toy *t = tt(); if (sub == 0 || (t->accs & (1 << sub))) s[0] = 0, strcpy(s, sub == ck()->acc ? "WEARING IT" : "OWNED"); else sprintf(s, "$%d", gx_acc_cost(sub)); lt(84, 100, s); }
    sprintf(s, "%d / %d", sub + 1, NACC); lt(210, 100, s); ui_hints(160, 217, "X WEAR|LR PICK|O BACK", 1);
}
static void draw_more(void) {
    draw_hand(1); lcd_panel(); static const char *const it[5] = { "FAMILY HOME", "PET DIARY", "AWARDS", "BLOBDEX", "HELP PAGES" };
    ltc(160, 20, "MORE"); for (int i = 0; i < 5; i++) { ui_round(90, 34 + i * 15, 140, 13, 5, i == sub ? UI_BLUE : UI_CARD2, i == sub ? UI_BLUE2 : UI_CARD2, 0); ui_textc(160, 37 + i * 15, 1, it[i], i == sub ? UI_WHITE : UI_INK); }
    ui_hints(160, 217, "X OPEN|O BACK", 1);
}
// ---- the blobdex: a grid of kinds (a dark shape until you have met it), and a page for each ----
static int dex_best(int sp, int *col) {   // the best form seen (-1: none), and a colour seen
    int f = -1; for (int i = 0; i < GFORMS; i++) if (gotchi_dex_has(sp, i)) f = i; *col = 0; for (int c = 0; c < 8; c++) if (gotchi_cdex_has(sp, c)) { *col = c; break; } return f;
}
static void dex_pet(int sp, int form, int col, int known, int x, int y, int s, int fr) {
    Blob b; Kin k; memset(&b, 0, sizeof b); memset(&k, 0, sizeof k); b.on = 1; b.stage = GS_ADULT; b.sp = (u8)sp; b.form = (u8)form; b.hun = 4; b.hap = 4; k.col = (u8)col;
    scene(&b, &k, fr, 0, MD_NORM); if (!known) lcdCol = 8; dots_draw(x, y, s, 2, 16, 4, 28);
}
static void draw_dex(void) {
    sky(); char t[40]; sprintf(t, "BLOBDEX  %d OF %d", gotchi_dex_count(), GSP_N * GFORMS);
    int top = ui_window(14, 8, 292, 224, t, IC_STAR);
    for (int i = 0; i < GSP_N; i++) {
        int x = 20 + (i % 5) * 56, y = top + 2 + (i / 5) * 42, col, f = dex_best(i, &col), sel = i == dexsel, n = 0; for (int q = 0; q < GFORMS; q++) n += gotchi_dex_has(i, q);
        ui_round(x, y, 54, 40, 6, sel ? UI_HILITE : UI_WHITE, sel ? UI_HILITE : UI_PANEL, 0); if (sel) ui_round(x - 1, y - 1, 56, 42, 7, UI_BLUE, UI_BLUE, 1);
        ui_round(x + 3, y + 2, 48, 28, 4, f >= 0 ? UI_RGB(172, 196, 128) : UI_RGB(206, 214, 232), f >= 0 ? UI_RGB(160, 184, 118) : UI_RGB(206, 214, 232), 0);
        dex_pet(i, f >= 0 ? f : 0, col, f >= 0, x + 3, y + 3, 2, anim / 20);
        if (f >= 0) { char c[8]; sprintf(c, "%d/6", n); ui_textfit(x + 4, y + 31, 48, gx_species(i), UI_INK); ui_text(x + 36, y + 4, 1, c, UI_GOLDT); } else ui_textc(x + 27, y + 31, 1, i >= 16 ? "RARE ???" : "???", UI_SOFT);
    }
    ui_hints(160, 214, "X PAGE|ARROWS PICK|O BACK", 1);
}
static void draw_dexd(void) {
    static const char *const how[GFORMS] = { "GOOD CARE AND LITTLE MISCHIEF", "LOTS OF CARE MISTAKES", "MORE SNACKS THAN MEALS", "NO MISTAKES, BIG BOND, GOOD MANNERS", "LIGHT, TRAINED, WELL PLAYED WITH", "BIG MEALS, LITTLE PLAY" };
    sky(); int col, best = dex_best(dexsel, &col); char t[48]; sprintf(t, "%s", best >= 0 ? gx_species(dexsel) : "???");
    int top = ui_window(14, 8, 292, 224, t, IC_STAR);
    int has = gotchi_dex_has(dexsel, dexf); ui_round(20, top + 2, 140, 76, 8, has ? UI_RGB(172, 196, 128) : UI_RGB(206, 214, 232), has ? UI_RGB(160, 184, 118) : UI_RGB(206, 214, 232), 0);
    dex_pet(dexsel, dexf, col, has, 24, top + 6, 4, anim / 20);
    ui_text(168, top + 4, 1, has ? gx_form(dexf) : "UNKNOWN FORM", UI_INK); ui_wrap(168, top + 16, 130, 3, has ? how[dexf] : "RAISE ONE TO FIND OUT", UI_SOFT);
    if (dexsel >= 16) ui_badge(168, top + 46, "RARE KIND", UI_RGB(150, 100, 210), UI_WHITE); else if (has) { sprintf(t, "%d OF 6 FORMS SEEN", ({ int c = 0; for (int q = 0; q < GFORMS; q++) c += gotchi_dex_has(dexsel, q); c; })); ui_text(168, top + 48, 1, t, UI_GOLDT); }
    for (int i = 0; i < GFORMS; i++) { int x = 20 + i * 47, h = gotchi_dex_has(dexsel, i); ui_round(x - 1, top + 86, 46, 26, 5, i == dexf ? UI_BLUE : UI_LINE, i == dexf ? UI_BLUE : UI_LINE, 0);
        ui_round(x, top + 87, 44, 24, 4, h ? UI_RGB(172, 196, 128) : UI_RGB(206, 214, 232), h ? UI_RGB(172, 196, 128) : UI_RGB(206, 214, 232), 0); dex_pet(dexsel, i, col, h, x - 2, top + 87, 1, 0); chip(x + 22, top + 114, h ? gx_form(i) : "???", h ? fcol[i] : UI_LINE, UI_WHITE); }
    ui_text(20, top + 134, 1, "COLOURS SEEN", UI_INK); for (int c = 0; c < 8; c++) { int sn = gotchi_cdex_has(dexsel, c); const u8 *p = PAL[c]; ui_disc(32 + c * 28, top + 156, 16, sn ? UI_RGB(p[0], p[1], p[2]) : UI_LINE); if (!sn) ui_textc(32 + c * 28, top + 152, 1, "?", UI_WHITE); }
    ui_wrap(20, top + 166, 270, 2, "FAMILIES CAN HAND DOWN COLOURS, KINDS AND RARE MUTATIONS.", UI_SOFT);
    ui_hints(160, 214, "LR FORM|O BACK", 1);
}
// ---- the family tree ----
typedef struct { u8 id, row; s16 x; } TNode;
static TNode tn[24]; static int ntn;
static void tn_add(int id, int row, int x) { if (id && ntn < 24) { for (int i = 0; i < ntn; i++) if (tn[i].id == id && tn[i].row == row) return; tn[ntn].id = (u8)id; tn[ntn].row = (u8)row; tn[ntn++].x = (s16)x; } }
static void tree_build(void) {
    ntn = 0; Line *s = gotchi_line(treeId); if (!s) { treeId = ck()->id; s = gotchi_line(treeId); } if (!s) return;
    int mom = s->mom, dad = s->dad; Line *m = gotchi_line(mom), *d = gotchi_line(dad);
    tn_add(m ? m->mom : 0, 0, 40); tn_add(m ? m->dad : 0, 0, 100); tn_add(d ? d->mom : 0, 0, 200); tn_add(d ? d->dad : 0, 0, 260);
    tn_add(mom, 1, 70); tn_add(dad, 1, 230);
    tn_add(treeId, 2, 160);
    { int sx = 60, nsb = 0; for (int i = 0; i < GL_N && nsb < 4; i++) { Line *o = &gline[i]; if (!o->id || o->id == treeId) continue; if ((s->mom && o->mom == s->mom) || (s->dad && o->dad == s->dad)) { tn_add(o->id, 2, nsb < 2 ? sx + nsb * 42 : 218 + (nsb - 2) * 42); nsb++; } } }
    { int kids[6], nk = gx_children(treeId, kids, 6); for (int i = 0; i < nk; i++) tn_add(kids[i], 3, 160 + (i * 2 - (nk - 1)) * 24); }
}
void tree_input(int pr) {
    tree_build(); if (!ntn) { if (pr & PAD_CIRCLE) view = V_HAND; return; } if (treeSel >= ntn) treeSel = 0;
    if (pr & (PAD_LEFT | PAD_RIGHT)) { int best = -1, bd = 999; for (int i = 0; i < ntn; i++) if (i != treeSel && tn[i].row == tn[treeSel].row) { int dx = tn[i].x - tn[treeSel].x; if ((pr & PAD_RIGHT) ? dx > 0 : dx < 0) { if (ABS(dx) < bd) { bd = ABS(dx); best = i; } } } if (best >= 0) treeSel = best; }
    if (pr & (PAD_UP | PAD_DOWN)) { int dr = (pr & PAD_DOWN) ? 1 : -1, best = -1, bd = 999; for (int i = 0; i < ntn; i++) if (tn[i].row == tn[treeSel].row + dr) { int dx = ABS(tn[i].x - tn[treeSel].x); if (dx < bd) { bd = dx; best = i; } } if (best >= 0) treeSel = best; }
    if (pr & PAD_CROSS) { treeId = tn[treeSel].id; treeSel = 0; tree_build(); for (int i = 0; i < ntn; i++) if (tn[i].id == treeId) treeSel = i; }
    if (pr & PAD_CIRCLE) { if (treeId != ck()->id) { treeId = ck()->id; treeSel = 0; } else view = V_HAND; }
}
static void draw_tree(void) {
    sky(); ui_accent = 2; int top = ui_window(14, 8, 292, 224, "FAMILY TREE", IC_PEOPLE); ui_accent = 0; tree_build();
    if (!ntn) { ui_wrapc(30, top + 40, 260, 3, "THIS PET HAS NO FAMILY RECORD YET. EGGS FROM A PAIR START A TREE.", UI_SOFT); ui_hints(160, 214, "O BACK", 1); return; }
    static const int ry[4] = { 0, 28, 62, 108 }; static const char *const rl[4] = { "GRANDPARENTS", "PARENTS", "", "CHILDREN" };
    for (int i = 0; i < 4; i++) if (rl[i][0]) { int has = 0; for (int q = 0; q < ntn; q++) has |= tn[q].row == i; if (has) ui_text(20, top + 2 + ry[i] + (i == 0 ? 0 : 0), 1, rl[i], UI_SOFT); }
    int sy = top + 2;
    for (int i = 0; i < ntn; i++) {   // the lines first
        if (tn[i].row == 2 && tn[i].id != treeId) continue;
        for (int j = 0; j < ntn; j++) if (tn[j].row == tn[i].row + 1 || 0) { const Line *c = gotchi_line(tn[j].id); int isp = c && (c->mom == tn[i].id || c->dad == tn[i].id); if (tn[i].row == 2 && tn[j].row != 3) continue; if (!isp) continue;
            int x0 = tn[i].x, y0 = sy + ry[tn[i].row] + (tn[i].row == 2 ? 50 : 36), x1 = tn[j].x, y1 = sy + ry[tn[j].row] + 6; if (tn[i].row == 2) y1 = sy + ry[3] + 4;
            int ym = (y0 + y1) / 2; gfx_rect(x0 - 1, y0, 2, ym - y0, 150, 160, 190, 0); gfx_rect(MIN(x0, x1) - 1, ym, ABS(x1 - x0) + 2, 2, 150, 160, 190, 0); gfx_rect(x1 - 1, ym, 2, y1 - ym, 150, 160, 190, 0); }
    }
    for (int i = 0; i < ntn; i++) {
        const Line *l = gotchi_line(tn[i].id); if (!l) continue; Blob b; Kin k; kin_blob(&b, &k, l); int sel = i == treeSel, alive = l->fc & 128, gone = (l->fc & 64) != 0, row = tn[i].row, x = tn[i].x, y = sy + ry[row] + 4;
        int self = tn[i].id == treeId; int w = self ? 68 : 36;
        if (self) { ui_round(x - 36, y - 4, 72, 52, 7, UI_HILITE, UI_HILITE, 0); }
        if (sel) ui_round(x - w / 2 - 3, y - 3, w + 6, self ? 50 : 36, 6, UI_BLUE, UI_BLUE, 0);
        if (self) { scene(&b, &k, anim / 20, 0, MD_NORM); lcd_bg(x - 32, y, 64, 32, 0); dots_all(x - 32, y, 2); if (!alive) gfx_rect(x - 32, y, 64, 32, 255, 255, 255, 1); }
        else mini_tile(&b, &k, x - 18, y, 36, gone);
        ui_textfit(x - w / 2, y + (self ? 34 : 20), w, gx_given(l->nm), gone ? UI_SOFT : UI_INK);
        if (gone) ui_icon_s(x + (self ? 24 : 10), y - 2, IC_STAR, 10, UI_AMBER);
    }
    { const Line *l = gotchi_line(tn[treeSel].id); char s[80]; int by = top + 178;
      ui_round(20, by, 280, 20, 6, UI_PANEL, UI_PANEL, 0);
      if (l) { sprintf(s, "%s THE %s %s  GEN %d  %s", gx_given(l->nm), gx_colour((l->fc >> 3) & 7), gx_species(l->sp), (l->pg >> 3) + 1, (l->fc & 64) ? "PASSED ON. REMEMBERED" : (l->fc & 128) ? gx_pers(l->pg & 7) : "UNKNOWN"); ui_textfit(26, by + 6, 270, s, UI_INK); } }
    ui_hints(160, 214, "ARROWS PICK|X CENTRE|O BACK", 1);
}
// ---- the family home: the family in its room, the memorials on the shelf, and the room yours to dress ----
static void draw_home(void) {
    static const u8 wall[6][3] = { { 255, 226, 150 }, { 150, 210, 240 }, { 245, 170, 200 }, { 170, 232, 200 }, { 60, 52, 110 }, { 240, 200, 90 } };
    Toy *t = tt(); const u8 *w = wall[t->room % 6]; int dk = t->room % 6 == 4; char s[48];
    sky(); int top = ui_window(14, 8, 292, 224, gx_room(t->room), IC_HOUSE); (void)top;
    gfx_rect(20, 32, 280, 102, w[0], w[1], w[2], 0); gfx_rect(20, 134, 280, 74, w[0] * 6 / 10, w[1] * 6 / 10, w[2] * 6 / 10, 0); gfx_rect(20, 132, 280, 3, w[0] / 2, w[1] / 2, w[2] / 2, 0);
    for (int x = 20; x < 300; x += 28) gfx_rect(x, 134, 1, 74, w[0] / 2, w[1] / 2, w[2] / 2, 1);
    u8 d = t->deco;   // the things in it
    if (d & 1) { gfx_rect(36, 42, 44, 36, 255, 255, 255, 0); gfx_rect(38, 44, 40, 32, 140, 200, 255, 0); gfx_rect(57, 44, 2, 32, 255, 255, 255, 0); gfx_rect(38, 59, 40, 2, 255, 255, 255, 0); gfx_rect(44, 66, 10, 5, 255, 255, 255, 0); }
    if (d & 2) { gfx_rect(266, 120, 12, 14, 150, 90, 50, 0); for (int i = 0; i < 4; i++) gfx_rect(262 + i * 6, 96 + (i & 1) * 6, 8, 24, 60, 170, 70, 0); }
    if (d & 4) { gfx_rect(208, 100, 3, 32, 120, 100, 80, 0); gfx_rect(200, 90, 20, 12, 255, 230, 140, 0); gfx_rect(204, 102, 12, 2, 255, 230, 140, 1); }
    if (d & 8) { ui_round(80, 176, 160, 26, 12, UI_RGB(220, 90, 110), UI_RGB(180, 60, 90), 0); ui_round(92, 180, 136, 18, 8, UI_RGB(250, 190, 100), UI_RGB(240, 160, 80), 0); }
    if (d & 16) { gfx_rect(138, 42, 44, 32, 130, 90, 50, 0); gfx_rect(141, 45, 38, 26, 250, 240, 220, 0); for (int i = 0; i < 4; i++) { gfx_rect(146 + i * 8, 52 + (i & 1) * 5, 6, 8, 230 - i * 30, 90 + i * 30, 120, 0); } }
    if (d & 32) { ui_round(26, 150, 40, 26, 4, UI_RGB(200, 130, 60), UI_RGB(160, 100, 40), 0); gfx_rect(30, 146, 8, 6, 255, 100, 100, 0); gfx_rect(44, 144, 8, 8, 100, 160, 255, 0); gfx_rect(54, 148, 8, 4, 255, 220, 80, 0); }
    // the memorial shelf: the last of those who passed on
    { int shown = 0; gfx_rect(190, 82, 100, 3, 120, 80, 50, 0); for (int i = GL_N - 1; i >= 0 && shown < 3; i--) { const Line *l = &gline[i]; if (!l->id || !(l->fc & 64)) continue; Blob b; Kin k; kin_blob(&b, &k, l);
          int x = 194 + shown * 32; scene(&b, &k, 0, 0, MD_NORM); gfx_rect(x, 62, 28, 20, 255, 250, 235, 0); gfx_rect(x + 1, 63, 26, 18, 214, 222, 214, 0); dots_draw(x - 1, 64, 1, 0, 16, 4, 28); gfx_rect(x + 11, 79, 6, 3, 250, 120, 160, 0); shown++; }
      if (!shown) gfx_text_raw(196, 70, 1, "MEMORIALS", dk ? 220 : 120, dk ? 220 : 90, dk ? 255 : 60); }
    // the family, parents at the back, the young in front
    int order[GB_N], n = 0; for (int g = 0; g < 32; g++) for (int i = 0; i < GB_N; i++) if (t->b[i].on && t->b[i].gen == g) order[n++] = i;
    for (int j = 0; j < n; j++) { int i = order[j]; Blob *b = &t->b[i]; Kin *k = &t->k[i]; int row = (j * 2 < n || n <= 2) ? 0 : 1, col = row ? j - (n + 1) / 2 : j; int cnt = row ? n / 2 : (n + 1) / 2;
        int x = 20 + (280 - cnt * 64) / 2 + col * 64 + (row ? 0 : 0), y = row ? 154 : 112; int fr = anim / 20 + i; if (b->stage == GS_EGG) y += 6;
        scene(b, k, fr, 0, mood_of(b)); dots_draw(x, y + (b->stage <= GS_CHILD ? 4 : 0), 2, 0, 16, 0, 32); if (k->mate) ui_icon_s(x + 27, y - 8, IC_HEART, 10, UI_RED); }
    if (!n) ui_textc(160, 150, 1, "THE HOME IS EMPTY", UI_SOFT);
    { static const char *const dn[6] = { "WINDOW", "PLANT", "LAMP", "RUG", "PICTURES", "TOY BOX" }; sprintf(s, "%s: %s", dn[homeSel], (t->deco >> homeSel) & 1 ? "ON" : "OFF"); ui_badge(22, 192, s, UI_BLUE2, UI_WHITE); }
    ui_hints(160, 214, mine() ? "X THING|LR PICK|SQ WALLS|O BACK" : "SQ WALLS|O BACK", 1);
}
static void draw_diary(void) {
    sky(); ui_accent = 6; int top = ui_window(14, 8, 292, 224, "PET DIARY", IC_BOOK); ui_accent = 0; int p = gotchi_diary_pos(), shown = 0;
    for (int i = 1; i <= GD_N && shown < 13; i++) { const Diary *d = &gdiary[((p - i) % GD_N + GD_N) % GD_N]; if (!d->kind || i > p) continue; char s[80]; gx_diary(d, s);
        ui_round(20, top + 2 + shown * 14, 280, 13, 4, shown & 1 ? UI_PANEL : UI_WHITE, shown & 1 ? UI_PANEL : UI_WHITE, 0); ui_textfit(25, top + 5 + shown * 14, 270, s, d->kind == GD_PASS ? UI_BADT : d->kind == GD_AWARD ? UI_GOLDT : UI_INK); shown++; }
    if (!shown) ui_wrapc(30, top + 60, 260, 3, "NOTHING WRITTEN YET. HATCH AN EGG AND LIFE GOES IN THE DIARY.", UI_SOFT);
    ui_hints(160, 214, "O BACK", 1);
}
static void draw_award(void) {
    sky(); ui_accent = 6; int top = ui_window(14, 8, 292, 224, "AWARDS", IC_STAR); ui_accent = 0; char s[48];
    for (int i = 0; i < NACH; i++) { int x = 20 + (i & 1) * 140, y = top + 2 + (i >> 1) * 19, got = (gach >> i) & 1, sel = i == sub; ui_round(x, y, 138, 17, 5, sel ? UI_HILITE : UI_WHITE, sel ? UI_HILITE : UI_PANEL, 0);
        if (sel) ui_round(x - 1, y - 1, 140, 19, 6, UI_BLUE, UI_BLUE, 1);
        ui_icon_s(x + 3, y + 3, got ? IC_TICK : IC_LOCK, 11, got ? UI_GREEN : UI_LINE); ui_textfit(x + 18, y + 5, 116, gx_ach(i), got ? UI_INK : UI_SOFT); }
    sprintf(s, "%s: %s  (+$%d)", gx_ach(sub), gx_ach_how(sub), 50 + sub * 10); ui_round(20, top + 160, 280, 14, 5, UI_AMBER, UI_AMBER, 0); ui_textfit(26, top + 163, 268, s, UI_INK);
    sprintf(s, "CARE SCORE %d", gx_score()); ui_textc(160, top + 178, 1, s, UI_GOLDT);
    ui_hints(160, 214, "UD PICK|O BACK", 1);
}
// ---- the guide: pictures, little live art, and plain words ----
#define NGP 14
typedef struct { const char *title; s8 img; u8 strip; const char *tip[3]; } GPage;
static const GPage gp[NGP] = {
    { "MEET THE TOY", 0, 0, { "A TINY PET ON A KEYCHAIN. IT LOVES YOU BACK.", "$5000 AT ELECTRONICS AND GAMES SHOPS.", "EVERYONE CAN OWN ONE. EIGHT PETS EACH." } },
    { "THE HANDHELD", 1, 1, { "D-PAD PICKS AN ICON, X USES IT. L1 R1 SWAP PETS.", "THE HINT BAR TELLS YOU WHAT EACH ICON DOES.", "BELL LIT UP? A PET NEEDS YOU. HURRY!" } },
    { "EIGHT PETS PER TOY", 2, 2, { "THE BASKET HOLDS EIGHT. A NEW EGG IS $150.", "20 KINDS: CATS, ROBOTS, GHOSTS, DRAGONS...", "HEARTS SHOW HOW CLOSE YOU TWO ARE." } },
    { "GROWING UP", 3, 3, { "EGG, BABY, CHILD, TEEN, ADULT, THEN ELDER.", "YOUR CARE DECIDES WHICH OF SIX FORMS IT BECOMES.", "GROWN-UPS FILL THE BLOBDEX. GET THEM ALL!" } },
    { "PLAY, FRIENDS AND EGGS", 4, 4, { "FOUR GAMES CHEER IT UP AND TRAIN IT.", "PLAYDATES CHEER BOTH. SQUARE = BATTLE.", "VISIT A HOUSEMATE'S PETS TOO." } },
    { "HIJINKS: YOU AND THE PET", 5, 5, { "LIVE MODE: REAL TIME. YOU MAY FORGET TO EAT!", "SAD OR STARVING? YOUR PETS FEEL IT TOO.", "SHARE YOUR LUNCH, OR GET A NUZZLE FOR FUN." } },
    { "MOODS AND CARE SKILL", 6, 6, { "PETS CHANGE YOUR MOOD: PROUD PARENT, BUDDY...", "NEGLECT THEM AND YOU WORRY. LOSE ONE: OUCH.", "CARE SKILL GROWS: LESS HUNGER, LESS SICK." } },
    { "BUILD AND BUY THINGS", 7, 7, { "NEW BUILD TAB: NINE THINGS FOR HOME.", "DOCK OPENS THE BASKET. CABINET: BLOBDEX.", "BOWL, LAMP, BED CARE FOR ALL PETS AT ONCE." } },
    { "WHAT'S NEW", 6, 8, { "NEW: FAMILIES, EGGS BY LOVE, A FAMILY TREE.", "NEW: HOMES, DIARY, AWARDS, HATS, FOODS, GAMES.", "NEW: RARE KINDS, COLOURS, ELDERS AND MEMORIALS." } },
    { "FAMILIES", -1, 9, { "TWO GROWN-UPS WHO LOVE EACH OTHER BECOME A PAIR.", "A PAIR LAYS EGGS. KIDS INHERIT KIND, COLOUR, TEMPER.", "PARENTS FEED THE YOUNG. BROTHERS AND SISTERS PLAY." } },
    { "THE FAMILY TREE", -1, 10, { "FAMILY ICON: PARENTS, GRANDPARENTS, KIDS, SIBLINGS.", "PRESS X ON A RELATIVE TO MAKE THEM THE CENTRE.", "RELATIVES NEVER PAIR UP. HOUSEMATES' PETS CAN." } },
    { "MOODS ON THE SCREEN", -1, 11, { "SLEEPY, HUNGRY, HAPPY, SICK, BORED, DANCING.", "ANGRY MEANS A TANTRUM: CUDDLE IT OR SCOLD IT.", "PERSONALITIES CHANGE HOW EACH ONE GROWS." } },
    { "FOOD, HATS AND HOME", -1, 12, { "12 FOODS. EACH PET LOVES ONE AND HATES ONE.", "HATS AND BOWS FROM THE DRESS ICON.", "DECORATE THE FAMILY HOME. MEMORIALS SIT ON THE SHELF." } },
    { "OLD AGE AND AWARDS", -1, 13, { "ELDERS SLOW DOWN. THEY PASS ON, GENTLY.", "THEIR KIDS REMEMBER THEM. THE TREE KEEPS THEM.", "16 AWARDS PAY THE HUMAN. ASK THE DIARY." } },
};
static void art_show(int i) {
    if (!art || i < 0 || i >= NIMG || art_page == i) return;
    u32 off = (u32)i * IW * IH * 2, sec = off / 2048, cnt = (off % 2048 + IW * IH * 2 + 2047) / 2048;
    if (!audio_cd_part("GOTCHI.ART", sec, (int)cnt, art_buf)) { art = 0; return; }
    u16 *px = atlas_scratch(); const u16 *src = (const u16 *)(art_buf + off % 2048);
    for (int y = 0; y < IH; y++) for (int x = 0; x < 128; x++) px[y * 128 + x] = x < IW ? src[y * IW + x] : 0;
    atlas_vram_load(0, 256, 128, 128, px); art_page = i;
}
static void gtile(int sp, int stage, int form, int col, int x, int y, int mood, int look, const char *lab) {
    Blob b; Kin k; memset(&b, 0, sizeof b); memset(&k, 0, sizeof k); b.on = 1; b.stage = (u8)stage; b.sp = (u8)sp; b.form = (u8)form; b.hun = b.hap = 4; b.age = 16; k.col = (u8)col;
    if (mood == MD_SLEEP) b.fl |= GF_SLEEP; ui_round(x - 2, y - 2, 68, 36, 5, UI_RGB(52, 24, 70), UI_RGB(30, 12, 44), 0); scene(&b, &k, anim / 20, look, mood); lcd_bg(x, y, 64, 32, 0); dots_all(x, y, 2); if (lab) chip(x + 32, y + 38, lab, UI_BLUE2, UI_WHITE);
}
static void strip_species(int a, int n, const int *sp, int stage) { for (int i = 0; i < n; i++) { int x = 22 + i * (276 / n) + (276 / n - 68) / 2; gtile(sp[i], stage, 0, (sp[i] + 1) % 8, x, a, MD_NORM, 0, gx_species(sp[i])); } }
static void guide_strip(int k, int y) {
    ui_round(20, y - 4, 280, 64, 7, UI_WHITE, UI_PANEL, 0); y += 1;
    int cyc = (anim / 120) % 16;
    switch (k) {
    case 0: { static const int s[4] = { 0, 1, 4, 9 }; int sp[4]; for (int i = 0; i < 4; i++) sp[i] = (s[i] + cyc) % 16; strip_species(y, 4, sp, GS_ADULT); break; }
    case 1: { static const int ic[5] = { 0, 2, 3, 8, 10 }; for (int i = 0; i < 5; i++) { int x = 42 + i * 52; ui_round(x - 4, y + 4, 26, 22, 5, UI_RGB(86, 36, 104), UI_RGB(50, 18, 66), 0); icon_bits(ic[i], x, y + 8, 2, 0xC890F0); chip(x + 9, y + 33, iconNm[ic[i]], UI_BLUE2, UI_WHITE); } break; }
    case 2: for (int i = 0; i < 4; i++) gtile((cyc + i * 3) % 16, i == 0 ? GS_EGG : i == 3 ? GS_ADULT : GS_BABY + i - 1, 0, i, 26 + i * 69, y, MD_NORM, 0, i == 0 ? "EGG" : i == 3 ? "GROWN-UP" : "BABY"); break;
    case 3: { static const u8 st[5] = { GS_EGG, GS_BABY, GS_CHILD, GS_TEEN, GS_ADULT }; static const char *const nm[5] = { "EGG", "BABY", "CHILD", "TEEN", "ADULT" };
              for (int i = 0; i < 5; i++) { Blob b; Kin kk; memset(&b, 0, sizeof b); memset(&kk, 0, sizeof kk); b.on = 1; b.sp = (u8)cyc; b.stage = st[i]; b.hun = b.hap = 4; b.age = 16; int x = 24 + i * 55; ui_round(x - 1, y - 1, 52, 28, 4, UI_RGB(52, 24, 70), UI_RGB(30, 12, 44), 0); scene(&b, &kk, anim / 20, 0, MD_NORM); gfx_rect(x, y, 50, 26, 172, 196, 128, 0); dots_draw(x - 7, y - 3, 2, 0, 16, 0, 32); chip(x + 25, y + 31, nm[i], UI_GREEN, UI_WHITE); } break; }
    case 4: gtile(cyc, GS_ADULT, 0, 1, 40, y, MD_NORM, 1, 0); gtile((cyc + 5) % 16, GS_ADULT, 0, 2, 212, y, MD_NORM, -1, 0); ui_icon_s(144, y + 4, IC_HEART, 24, (anim / 15) & 1 ? UI_RED : UI_BADT); ui_textc(156, y + 34, 1, "PLAYDATE", UI_INK); break;
    case 5: ui_icon_disc(60, y + 14, 30, IC_SMILE, UI_BLUE, UI_WHITE); ui_text(36, y + 36, 1, "YOU: TUMMY", UI_INK); gtile(cyc, GS_ADULT, 0, 3, 214, y, MD_NORM, -1, 0);
            ui_textc(150, y + 2, 1, "SHARE LUNCH", UI_GOLDT); ui_textc(150, y + 36, 1, "YOU STARVE, IT STARVES", UI_BADT); ui_icon_s(138, y + 14, IC_ARROW, 20, UI_GOLDT); break;
    case 6: { static const char *const m[6] = { "IT HATCHED! +16", "PROUD PARENT +14", "BLOB BUDDY +10", "WORRIED -8", "HOOKED ON THE TOY -8", "MISSING MY BLOB -15" };
              for (int i = 0; i < 6; i++) ui_badge(26 + (i & 1) * 138, y + (i >> 1) * 18, m[i], i < 3 ? UI_GREEN : UI_RED, UI_WHITE); break; }
    case 7: { static const char *const o[9] = { "DOCK $120", "SHELF $180", "CABINET $300", "PLUSH $60", "BALL $40", "BED $90", "BOWL $40", "LAMP $70", "TROPHY $200" };
              for (int i = 0; i < 9; i++) ui_badge(24 + (i % 3) * 92, y + (i / 3) * 18, o[i], UI_BLUE2, UI_WHITE); break; }
    case 8: ui_textc(160, y + 2, 1, "NO MISSIONS: JUST YOU AND THE PETS.", UI_INK); ui_textc(160, y + 16, 1, "IT'S ITS OWN FILE ON THE DISC (GOTCHI.OVL):", UI_SOFT);
        ui_badge(40, y + 36, "20 KINDS", UI_GREEN, UI_WHITE); ui_badge(112, y + 36, "FAMILIES", UI_RED, UI_WHITE); ui_badge(184, y + 36, "6 FORMS", UI_AMBER, UI_INK); ui_badge(246, y + 36, "16 AWARDS", UI_BLUE2, UI_WHITE); break;
    case 9: { gtile(cyc, GS_ADULT, 0, 1, 24, y, MD_NORM, 0, "MUM"); gtile((cyc + 3) % 16, GS_ADULT, 0, 2, 100, y, MD_NORM, 0, "DAD"); ui_icon_s(88, y - 2, IC_HEART, 12, UI_RED); gtile(cyc, GS_BABY, 0, 1, 212, y, MD_HAPPY, 0, "BABY"); ui_icon_s(176, y + 8, IC_ARROW, 20, UI_GOLDT); break; }
    case 10: { Blob b; Kin kk; for (int i = 0; i < 3; i++) { memset(&b, 0, sizeof b); memset(&kk, 0, sizeof kk); b.on = 1; b.stage = GS_ADULT; b.sp = (u8)((cyc + i) % 16); b.hun = b.hap = 4; kk.col = (u8)(i + 1); int x = 60 + i * 80; scene(&b, &kk, 0, 0, MD_NORM); gfx_rect(x, y + (i == 1 ? 24 : 0), 36, 18, 172, 196, 128, 0); dots_draw(x + 6, y + 1 + (i == 1 ? 24 : 0), 1, 0, 16, 4, 28); }
             gfx_rect(78, y + 18, 2, 6, 150, 160, 190, 0); gfx_rect(158, y + 18, 2, 6, 150, 160, 190, 0); gfx_rect(78, y + 18, 82, 2, 150, 160, 190, 0); ui_textc(160, y + 46, 1, "GRANDPARENTS - PARENTS - YOU - KIDS", UI_SOFT); break; }
    case 11: { static const int ms[4] = { MD_SLEEP, MD_HUNGRY, MD_SICK, MD_BORED }; static const char *const mn[4] = { "SLEEPY", "HUNGRY", "SICK", "BORED" };
              for (int i = 0; i < 4; i++) { Blob b; Kin kk; memset(&b, 0, sizeof b); memset(&kk, 0, sizeof kk); b.on = 1; b.stage = GS_ADULT; b.sp = (u8)cyc; b.hun = b.hap = 4; kk.col = (u8)i; if (ms[i] == MD_SLEEP) b.fl = GF_SLEEP; int x = 28 + i * 66; scene(&b, &kk, anim / 20, 0, ms[i]); gfx_rect(x, y, 56, 28, 172, 196, 128, 0); dots_draw(x - 4, y - 1, 2, 1, 16, 2, 30); ui_textc(x + 28, y + 32, 1, mn[i], UI_INK); } break; }
    case 12: { static const char *const f[6] = { "RICE BALL", "FISH $5", "PIE $10", "COOKIE", "ICE CREAM $10", "FRUIT" }; for (int i = 0; i < 6; i++) ui_badge(24 + (i % 3) * 92, y + (i / 3) * 18, f[i], i < 3 ? UI_GREEN : UI_AMBER, i < 3 ? UI_WHITE : UI_INK);
             ui_textc(160, y + 42, 1, "HEART = FAVOURITE    YUCK = HATES IT", UI_SOFT); break; }
    default: for (int i = 0; i < 3; i++) gtile((cyc + i * 4) % 16, i == 2 ? GS_ELDER : GS_ADULT, i == 1 ? 3 : 0, i, 26 + i * 90, y, MD_NORM, 0, i == 2 ? "ELDER" : i == 1 ? "STAR" : "ADULT"); break;
    }
}
static void draw_guide(void) {
    sky(); const GPage *g = &gp[guidepage];
    int top = ui_window(14, 8, 292, 224, g->title, IC_HELP);
    
    art_show(g->img);
    ui_round(19, top + 2, 108, 80, 6, UI_BLUE2, UI_BLUE2, 0);
    if (art && art_page == g->img && g->img >= 0) gfx_sprite_c(21, top + 4, 104, 76, getTPage(2, 0, 0, 256), 0, 0, IW, IH, 128, 128, 128);
    else { Blob b; Kin k; memset(&b, 0, sizeof b); memset(&k, 0, sizeof k); b.on = 1; b.stage = guidepage == 13 ? GS_ELDER : GS_ADULT; b.sp = (u8)((guidepage * 5 + anim / 150) % 16); b.hun = b.hap = 4; k.col = (u8)(guidepage % 7 + 1); k.acc = (u8)(guidepage == 12 ? 6 : 0);
        static const u8 pm[5] = { MD_HAPPY, MD_NORM, MD_DANCE, MD_HAPPY, MD_NORM }; scene(&b, &k, anim / 20, 0, pm[(guidepage - 9) % 5]); gfx_rect(21, top + 4, 104, 76, 172, 196, 128, 0); dots_draw(31, top + 19, 3, 1, 16, 2, 30); }
    for (int i = 0; i < 3; i++) { int y = top + 2 + i * 27; ui_round(134, y, 168, 25, 6, UI_WHITE, UI_PANEL, 0); ui_disc(142, y + 12, 8, UI_BLUE); ui_wrap(150, y + 3, 150, 2, g->tip[i], UI_INK); }
    guide_strip(g->strip, top + 90);
    for (int k = 0; k < NGP; k++) ui_disc(160 - (NGP - 1) * 5 + k * 10, 204, 6, k == guidepage ? UI_BLUE : UI_LINE);
    { char b[16]; sprintf(b, "%d OF %d", guidepage + 1, NGP); ui_text(24, 216, 1, b, UI_SOFT); } ui_hints(160, 214, "L1 R1 PAGE|O BACK", 1);
}
void gotchi_draw(void) {
    switch (view) {
    case V_BOX: draw_box(); break; case V_BASKET: draw_basket(); break; case V_HAND: draw_hand(0); break; case V_FEED: draw_feed(); break;
    case V_PLAYPICK: draw_playpick(); break; case V_LOOK: draw_look(); break; case V_COPY: draw_copy(); break; case V_CATCH: draw_catch(); break; case V_HILO: draw_hilo(); break;
    case V_FRIEND: draw_friend(); break; case V_STATS: draw_stats(); break; case V_GUIDE: draw_guide(); break; case V_DEX: draw_dex(); break; case V_DEXD: draw_dexd(); break;
    case V_TREE: draw_tree(); break; case V_HOME: draw_home(); break; case V_DIARY: draw_diary(); break; case V_AWARD: draw_award(); break; case V_MORE: draw_more(); break; default: draw_dress(); break;
    }
}
#include "ovl.h"
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, gotchi_open, gotchi_update, gotchi_draw };
