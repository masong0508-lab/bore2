#ifndef BEARDV
#define BEARDV 0   /* test builds: -DBEARDV=n gives Kit that beard */
#endif
// BORE 2 for the DSi, step D1: the neighbourhood.
// Bottom screen: the 2D neighbourhood map. Tap a lot with the stylus (or use the D-pad) to pick it.
// Top screen: that lot in 3D. L/R change town, A/B zoom, START re-rolls the world.
// Fixed point only; the world is bigmap.c, shared with the PS1 build (nothing of it is stored).
#define PMN 28
static int o_rep = 1, o_hint = 1, o_gtime, o_roof, o_aa = 1, o_out, o_shd = 1, o_det = 1, o_fps30, o_bri = 4, o_sky, o_chk = 1, o_spin = 2, o_turbo, o_cam = 1, o_spd, o_page, opt_on, o_dirty = 1, o_lit = 1, o_sty, o_pm, o_fog, o_smax, o_ocol, o_dd = 1, o_fogd = 1, o_trf = 3, o_vol = 7, o_mus = 6, o_sfx = 8, o_trk, o_ckl = 1, o_chm = 1, o_pan = 1, o_auto = 1, o_c24, o_tab, lit_on, lit_sgn = -1, o_msrc, o_song, o_need = 2, o_day = 1, o_msgt = 1, o_tmus = 1, o_gmus = 1, o_shuf = 1, o_look = 1, o_detl, prof_full, fast_cpu, o_tex = 1, o_rt, rt_mon = -1, rt_md, rt_jump = 1;   // (o_rt: Options > Game > Real time; the real month (0 Jan) and day; 1: put the clock straight to the real one, nothing played through)

// ---- the options: one table (name, the choices, the value, the tab it lives on, its default). Saved in table order, so new ones are only ever added at the end.
typedef struct { const char *name, *vals; int *v; unsigned char tab, def; } Opt;
static const Opt OPT[] = {
    { "Anti-alias", "Off|On", &o_aa, 0, 1 }, { "Outlines", "Off|On", &o_out, 0, 0 }, { "Shadows", "Off|On", &o_shd, 0, 1 }, { "Lighting", "Off|On", &o_lit, 0, 1 },
    { "Item detail", "Simple|Detailed", &o_det, 0, 1 }, { "Ground check", "Off|On", &o_chk, 0, 1 }, { "Frame rate", "Steady 30|Up to 60", &o_fps30, 3, 0 }, { "Brightness", "-4|-3|-2|-1|+0|+1|+2|+3|+4", &o_bri, 0, 4 },
    { "Sky", "Day|Sunset|Night|Mint|Auto", &o_sky, 0, 0 }, { "Camera", "Close|Normal|Far", &o_cam, 2, 1 }, { "Map spin", "Off|Slow|Normal|Fast", &o_spin, 2, 2 }, { "Game speed", "1x|2x|3x", &o_spd, 2, 0 },
    { "Turbo CPU", "Auto|On|Off", &o_turbo, 3, 0 }, { "Sim style", "Classic|Sims 3|Chunky", &o_sty, 0, 0 }, { "Poly meter", "Off|On", &o_pm, 0, 0 }, { "Fog", "Off|On", &o_fog, 0, 0 },
    { "Save space", "Safe|Max", &o_smax, 2, 0 },
    { "Outline color", "Dark|Black|White|Pink", &o_ocol, 0, 0 }, { "Draw distance", "Near|Normal|Far", &o_dd, 0, 1 }, { "Fog range", "Close|Normal|Far", &o_fogd, 0, 1 }, { "Traffic", "Off|Few|Some|All", &o_trf, 0, 3 },
    { "Master volume", "Off|1|2|3|4|5|6|7|8|9|10", &o_vol, 1, 7 }, { "Music volume", "Off|1|2|3|4|5|6|7|8|9|10", &o_mus, 1, 6 }, { "Effects volume", "Off|1|2|3|4|5|6|7|8|9|10", &o_sfx, 1, 8 },
    { "Music track", "Auto|Daydream|Evening|Bounce|Night", &o_trk, 1, 0 }, { "Tap clicks", "Off|On", &o_ckl, 1, 1 }, { "Chimes", "Off|On", &o_chm, 1, 1 }, { "Stereo", "Center|Spread", &o_pan, 1, 1 },
    { "Autosave", "Off|On", &o_auto, 2, 1 }, { "Clock", "12 hour|24 hour", &o_c24, 2, 0 },
    { "Reset graphics", "tap", 0, 3, 0 }, { "Reset audio", "tap", 0, 3, 1 }, { "Reset all", "tap", 0, 3, 255 },
    { "Music source", "Songs|Synth", &o_msrc, 1, 0 }, { "Song", "Auto|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52", &o_song, 1, 0 },
    { "Roofs", "Cutaway|Always|Hide", &o_roof, 0, 0 },
    // (D33, from the PS1 OPTIONS: these are saved in their own block after the others, so old saves load with the defaults)
    { "Needs speed", "Never fall|Slow|Normal|Fast", &o_need, 2, 2 }, { "Day length", "Short|Normal|Long|Epic", &o_day, 2, 1 }, { "Message time", "Short|Normal|Long", &o_msgt, 2, 1 },
    { "Title music", "Off|On", &o_tmus, 1, 1 }, { "Game music", "Off|On", &o_gmus, 1, 1 }, { "Shuffle songs", "In order|At random", &o_shuf, 1, 1 },
    { "Reset game", "tap", 0, 3, 2 },
    // (D40) the 3D look (Classic: as before D40; Rich: a sky, haze in the distance, highlights; Toon: Rich with outlines round every thing) and the detail (Auto: DSi when the CPU is the DSi's fast one)
    { "3D look", "Classic|Rich|Toon", &o_look, 0, 1 }, { "Detail", "Auto|DS|DSi", &o_detl, 3, 0 },
    { "Textures", "Off|On", &o_tex, 0, 1 },   // (D43: the PS1's patterns on the ground, floors, walls and roofs)
    { "Real time", "Off|On", &o_rt, 2, 0 },
    { "Boreagotchi time", "Game clock|LIVE|Off", &o_gtime, 2, 0 },   // (D53: how the pets live: on the game clock, LIVE (real time while you hold the toy too), or off)   // (D44: the clock is the DS's own: time of day, weekday, season by the month; sleep and work stay quick)
    // (D70) the new Options screen: hold-to-repeat speed, the button hints bar, and the page you left it on (the last one is saved so Options opens where you were)
    { "Hold repeat", "Slow|Normal|Fast", &o_rep, 255, 1 }, { "Hints bar", "Off|On", &o_hint, 255, 1 }, { "Options page", "0|1|2|3|4|5|6|7", &o_tab, 255, 0 },
};
#define OPT_OLD 36   // options saved in the save's first 40 bytes (the rest go in the 'OX' block)
static void open_opts(void); static void opt_enter(void);
#define NOPT ((int)(sizeof OPT / sizeof OPT[0]))
static int opt_cnt(int i) { int n = 1; for (const char *p = OPT[i].vals; *p; p++) if (*p == '|') n++; return n; }
static const char *opt_tok(int i, int n, char *b) { const char *p = OPT[i].vals; while (n-- > 0) { while (*p && *p != '|') p++; if (*p) p++; } int k = 0; while (*p && *p != '|' && k < 15) b[k++] = *p++; b[k] = 0; return b; }
#include <nds.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "bigmap.h"

int bsprintf(char *o, const char *f, ...) { va_list a; va_start(a, f); int r = vsprintf(o, f, a); va_end(a); return r; }
int bsnprintf(char *o, unsigned cap, const char *f, ...) { va_list a; va_start(a, f); int r = vsnprintf(o, cap, f, a); va_end(a); return r; }
void boot(const char *s) { (void)s; }
void loading(const char *w, int p) { (void)w; (void)p; }
// what bigmap.c (shared with the PS1) asks of the PS1 side: no 8 MB heap here, no world.c height cache, debug counters
int ram8 = 0; void *xalloc(unsigned n) { (void)n; return 0; } int dbg_miss[4]; void world_gh_reset(void) {}

// ---------- the bottom screen: a 256 x 256 16-bit bitmap ----------
static u16 *scr, *scr_sub, *hud;
#define C15(r, g, b) (RGB15(r, g, b) | BIT(15))
static u32 dr_mask;   // (D70) the bands of 16 rows drawn into since it was last cleared: bit k is rows 16k..16k+15; draw_hud() copies and clears only those
static inline void dr_mark(int y, int h) { int a = y >> 4, b = (y + h - 1) >> 4; if (a < 0) a = 0; if (b > 11) b = 11; if (b >= a) dr_mask |= ((2u << b) - 1) & ~((1u << a) - 1); }
static void px(int x, int y, u16 c) { if ((unsigned)x < 256 && (unsigned)y < 192) { scr[y * 256 + x] = c; dr_mask |= 1u << (y >> 4); } }
static void rect(int x, int y, int w, int h, u16 c) {
    if (x < 0) { w += x; x = 0; } if (y < 0) { h += y; y = 0; } if (x + w > 256) w = 256 - x; if (y + h > 192) h = 192 - y; if (w <= 0 || h <= 0) return; dr_mark(y, h);
    u32 cc = (u32)c | ((u32)c << 16);
    for (int j = 0; j < h; j++) { u16 *p = scr + (y + j) * 256 + x; int i = 0, n = w; if (x & 1) { p[0] = c; i = 1; n--; } u32 *q = (u32 *)(p + i); for (int k = 0; k < n / 2; k++) q[k] = cc; if (n & 1) p[i + n - 1] = c; }
}
#include "ds_fonts.h"   // (D65) tools/dsbuild/mk_dsfonts.py: BODY, HEAD, SMALL, NUM, TITLE
// ---- text (D65): five fonts; the s argument of text() / tw() / text_c() ... picks one. Same roles as the PS1's docs/FONTS.md: HEAD bold, BODY clean, SMALL labels, NUM money and clock, TITLE the fun big face.
enum { FS_BODY = 1, FS_HEAD = 2, FS_SMALL = 3, FS_NUM = 4, FS_TITLE = 5 };
enum { AL_C = 0, AL_L = -1, AL_R = 1 };
static int t_tight;   // (D35) 1: the letters one pixel closer (long names on the shop's cards)
static int fnt_sel = FS_BODY;
static void font_select(int f) { fnt_sel = (f >= 1 && f <= 5) ? f : FS_BODY; }   // the font-select API: text_u() and text_cu() use the selected font
static const DsFont *fnt(int s) { if (s < 1) s = 1; if (s > 5) s = 5; return &DSFONT[s - 1]; }
static int th(int s) { return fnt(s)->fcap; }   // the cap height: what looks like "the height" of a line
static int tw(int s, const char *t) {   // the width of a string in pixels, from the pen to the last ink
    const DsFont *f = fnt(s); int w = 0, lastgap = 0;
    for (; *t; t++) { int c = (unsigned char)*t; if (c < 32 || c > 126) continue; c -= 32; w += f->adv[c] - t_tight; lastgap = f->adv[c] - (f->xoff[c] + f->w[c]); }
    return w - (lastgap > 0 ? lastgap - t_tight : 0);
}
// y of text(): the middle row of the capitals is y + 1 in every font, so a call can swap fonts and stay in place
static void text(int x, int y, int s, u16 c, const char *t) {   // a soft shadow under it (a dark outline for TITLE)
    const DsFont *f = fnt(s); int dark = ((c & 31) + ((c >> 5) & 31) + ((c >> 10) & 31)) > 45; u16 sh = C15(0, 2, 8);
    int row0 = y + 1 - f->fcap / 2 + f->fcap - 1 - (f->fb - 1);
    if (s != FS_TITLE && x >= 1 && y >= 3 + f->fcap && row0 >= 1 && row0 + f->fh + 2 < 192) { int ww = tw(s, t) + f->fh; if (x + ww + 3 < 256) {   // (D70) all inside the screen: no bounds test on each pixel
        u16 *base = scr; int pen = x; dr_mark(row0, f->fh + 2);
        for (const char *p = t; *p; p++) { int ch = (unsigned char)*p; if (ch < 32 || ch > 126) continue; ch -= 32;
            const u16 *rw = f->rows + ch * f->fh; int w = f->w[ch], gx = pen + f->xoff[ch];
            for (int r = 0; r < f->fh; r++) { unsigned v = rw[r]; if (!v) continue; u16 *d = base + (row0 + r) * 256 + gx;
                if (dark) { unsigned m = v; while (m) { int q = __builtin_clz(m) - 16; m &= ~(0x8000u >> q); d[q + 257] = sh; } }
                { unsigned m = v; while (m) { int q = __builtin_clz(m) - 16; m &= ~(0x8000u >> q); d[q] = c; } } }
            pen += f->adv[ch] - t_tight; }
        return; } }
    for (int pass = (s == FS_TITLE ? 0 : 1); pass < 2; pass++) { int pen = x;
        for (const char *p = t; *p; p++) { int ch = (unsigned char)*p; if (ch < 32 || ch > 126) continue; ch -= 32;
            const u16 *rw = f->rows + ch * f->fh; int w = f->w[ch], gx = pen + f->xoff[ch];
            for (int r = 0; r < f->fh; r++) { unsigned v = rw[r]; if (!v) continue; int yy = row0 + r;
                for (int q = 0; q < w; q++) if (v >> (15 - q) & 1) {
                    if (pass == 0) { px(gx + q - 1, yy, sh); px(gx + q + 1, yy, sh); px(gx + q, yy - 1, sh); px(gx + q, yy + 1, sh); }
                    else { if (dark && s != FS_TITLE) px(gx + q + 1, yy + 1, sh); px(gx + q, yy, c); } } }
            pen += f->adv[ch] - t_tight; } }
}
// ---- measure, then centre (D65): one place does the sums, so a label sits in the middle of its box whatever it says ----
static const u8 FS_SMALLER[6] = { 0, 3, 1, 0, 3, 2 };   // the next smaller font: HEAD -> BODY -> SMALL; NUM -> SMALL; TITLE -> HEAD
static int fit_font(int s, int w, const char *t) {   // the biggest font from s down that fits w (0 if none)
    while (s) { if (tw(s, t) <= w) return s; s = FS_SMALLER[s]; } return 0; }
static void text_c(int cx, int cy, int s, u16 c, const char *t) { text(cx - tw(s, t) / 2, cy - 1, s, c, t); }   // centred on (cx, cy): cy is the middle row of the capitals
static void text_r(int rx, int cy, int s, u16 c, const char *t) { text(rx - tw(s, t), cy - 1, s, c, t); }       // its right edge at rx
static void text_u(int x, int y, u16 c, const char *t) { text(x, y, fnt_sel, c, t); }
static void text_cu(int cx, int cy, u16 c, const char *t) { text_c(cx, cy, fnt_sel, c, t); }
static int box_mid(int y, int h, int s) { return y + (h - th(s) + 1) / 2 + (th(s) - 1) / 2; }   // the middle row of the capitals for a line centred in a box y..y+h
static void text_box(int x, int y, int w, int h, int s, u16 c, int al, const char *t) {   // t in the box: measured, the font stepped down if it is too wide, cut with a dot as the last resort, then centred both ways (al AL_L / AL_R: 3 px in from that side)
    int pad = al == AL_C ? 1 : 3, f = fit_font(s, w - 2 * pad, t); char b[48];
    if (!f) { f = s; while (FS_SMALLER[f]) f = FS_SMALLER[f]; int n = (int)strlen(t); if (n > 44) n = 44; memcpy(b, t, (size_t)n); b[n] = 0;
        while (n > 1 && tw(f, b) > w - 2 * pad) { n--; b[n] = 0; if (n > 2) b[n - 1] = '.'; } t = b; }
    int tx = al == AL_L ? x + pad : al == AL_R ? x + w - pad - tw(f, t) : x + (w - tw(f, t)) / 2;
    text(tx, box_mid(y, h, f) - 1, f, c, t);
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
// ---------- PROPERTY: the lots you own (kept for good, edited freely) and the rest, which are for sale ----------
// Your first home is free. Any other lot can be bought on the Map (Buy) for a price that depends on what stands there; Buy and Build only work on lots you own.
// A lot you sell goes back to the market (70 % of its price back, what you built there is gone). The lot you are standing on, and your last lot, cannot be sold.
#define OWN_MAX 64
typedef struct { s16 bx, by; } OwnL;
static OwnL own[OWN_MAX]; static int nown;
static int funds, lot_bx = -1, lot_by = -1, home_bx = -1, home_by = -1;   // lot_bx/by: the lot you are on now; home_bx/by: the one you live in (always one of yours)
static int is_home(int bx, int by) { return home_bx == bx && home_by == by; }
static int lot_owned(int bx, int by) { for (int i = 0; i < nown; i++) if (own[i].bx == bx && own[i].by == by) return 1; return 0; }
static int lot_price(const BInfo *b) {   // 0: not for sale (the sea, a river)
    if (b->terrain == BT_SEA || b->lot == LOT_RIVER || b->lot == LOT_SPACE || b->lot == LOT_OBS) return 0;
    int p;
    switch (b->lot) {
        case LOT_WILD: p = 600; break; case LOT_EMPTY: p = 1200; break; case LOT_FARM: p = 4000; break; case LOT_PARK: p = 5000; break;
        case LOT_COTTAGE: p = 3500; break; case LOT_HOUSE: p = 6000; break; case LOT_TOWN: p = 9000; break; case LOT_TWO: p = 11000; break; case LOT_APART: p = 14000; break;
        case LOT_SQUARE: case LOT_SKATE: p = 8000; break; case LOT_CAFE: case LOT_CONV: case LOT_GAS: p = 12000; break; case LOT_DINER: case LOT_BAR: case LOT_SALON: p = 16000; break;
        case LOT_SHOP: case LOT_GYM: case LOT_POOL: case LOT_ARCADE: case LOT_INN: p = 18000; break; case LOT_SCHOOL: case LOT_OFFICE: case LOT_CLINIC: case LOT_LIBRARY: p = 22000; break;
        case LOT_HOTEL: case LOT_CINEMA: case LOT_SPA: case LOT_GALLERY: case LOT_RESTAURANT: p = 30000; break; case LOT_TOWER: p = 45000; break; default: p = 10000; }
    if (b->terrain == BT_BEACH) p = p * 6 / 5; else if (b->terrain == BT_FOREST || b->terrain == BT_ROCKY) p = p * 9 / 10;
    return (p + (int)(b->h & 15) * 50) / 4 / 50 * 50;
}
static int lot_sell_price(const BInfo *b) { return lot_price(b) * 7 / 10; }
static void own_add(int bx, int by) { if (!lot_owned(bx, by) && nown < OWN_MAX) { own[nown].bx = (s16)bx; own[nown].by = (s16)by; nown++; } }
static void own_remove(int bx, int by) { for (int i = 0; i < nown; i++) if (own[i].bx == bx && own[i].by == by) { own[i] = own[--nown]; return; } }
static int hit(int x, int y, int w, int h, int tx, int ty);
static void prop_trade(void);
static void say(const char *m);
static int map_enter, ne_on, ne_tool, ne_lot_i, ne_g_i, ne_road = 1, ne_last_x = -1, ne_last_y;
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
    rect(x, y, w, h, on ? C15(31, 31, 20) : C15(2, 4, 12)); rect(x + 1, y + 1, w - 2, h - 2, on ? C15(10, 24, 10) : C15(6, 12, 26)); text_box(x, y, w, h, h > 17 ? FS_HEAD : FS_BODY, C15(31, 31, 31), AL_C, t);   // (D65) measured and centred in the chip, whatever it says
}
static void map_chip(int px_, int py_) {   // a tap on the strip under the map
    if (!ne_on) {
        if (hit(229, 178, 25, 14, px_, py_)) ne_on = 1; else if (hit(202, 178, 25, 14, px_, py_)) open_opts();
        else if (hit(56, 178, 38, 14, px_, py_)) map_enter = 1;
        else if (hit(96, 178, 62, 14, px_, py_)) prop_trade();
        else if (hit(2, 178, 52, 14, px_, py_) && lot_owned(sel_x, sel_y) && !is_home(sel_x, sel_y)) { BInfo b; bm_block(sel_x, sel_y, &b); if (b.terrain == BT_SEA) say("Cannot live at sea"); else { home_bx = sel_x; home_by = sel_y; say("Welcome home! Your new place"); map_enter = 1; } }   // (the old home stays yours, as a property)
        return; }
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
        if (lot_owned(bx, by)) { u16 g = is_home(bx, by) ? C15(31, 26, 4) : C15(8, 28, 31);   /* gold: your home; light blue: a property you own */ rect(i * 8, j * 8, 8, 1, g); rect(i * 8, j * 8 + 7, 8, 1, g); rect(i * 8, j * 8, 1, 8, g); rect(i * 8 + 7, j * 8, 1, 8, g); }
        if (bx == sel_x && by == sel_y) { rect(i * 8, j * 8, 8, 1, 31 | 31 << 5 | 31 << 10 | BIT(15)); rect(i * 8, j * 8 + 7, 8, 1, 0xFFFF); rect(i * 8, j * 8, 1, 8, 0xFFFF); rect(i * 8 + 7, j * 8, 1, 8, 0xFFFF); }
    }
    rect(0, 160, 256, 32, C15(3, 3, 5));
    BInfo b; bm_block(sel_x, sel_y, &b);
    char t[48];
    if (!ne_on) {
        int own_ = lot_owned(sel_x, sel_y), price = lot_price(&b), fm, pp = pop_at(sel_x, sel_y, &fm), home = is_home(sel_x, sel_y), mv = own_ && !home;
        sprintf(t, "%.15s", bm_lot_name(b.lot)); text_box(2, 159, 146, 11, FS_HEAD, C15(31, 31, 22), AL_L, t);   // (D65) row A: the lot and what it is; row B (small): the town, who lives there, where; row C: the buttons
        if (own_) { int tw_ = home ? 44 : 64; rect(150, 160, tw_, 10, home ? C15(31, 24, 2) : C15(8, 26, 31)); text_box(150, 160, tw_, 10, FS_BODY, C15(8, 4, 0), AL_C, home ? "HOME" : "PROPERTY"); }
        else if (price && pp) text_box(150, 159, 104, 11, FS_BODY, C15(20, 22, 28), AL_L, "Family home");   // (someone lives there: not for sale)
        else if (price) { sprintf(t, "FOR SALE  %d", price); text_box(150, 159, 104, 11, FS_BODY, C15(31, 24, 10), AL_L, t); }
        else text_box(150, 159, 104, 11, FS_BODY, C15(20, 22, 28), AL_L, "Not for sale");
        if (pp && !own_) sprintf(t, "%.14s   %d bores   %d,%d", s->name, pp, sel_x, sel_y); else sprintf(t, "%.14s   %d,%d   pop %d/%d", s->name, sel_x, sel_y, pop_per + pop_born, DS_POP_MAX); (void)fm; (void)mv;
        text_box(2, 170, 252, 9, FS_SMALL, C15(24, 27, 31), AL_L, t);
        if (mv) chip(2, 179, 52, 12, "Move in", 0);
        chip(56, 179, 38, 12, "Visit", 0);
        if (own_) { char sb[24]; sprintf(sb, "Sell %d", lot_sell_price(&b)); chip(96, 179, 62, 12, sb, 0); }
        else if (price && !pp) { char bb[24]; sprintf(bb, "Buy %d", price); chip(96, 179, 62, 12, bb, funds >= price); }
        chip(202, 179, 25, 12, "Opt", 0); chip(229, 179, 25, 12, "Edit", 0);
    } else {
        for (int i = 0; i < 7; i++) chip(2 + i * 36, 162, 34, 13, NETN[i], ne_tool == i);
        const char *v = ne_tool == 0 ? bm_lot_name(NELOT[ne_lot_i]) : ne_tool == 1 ? NEGN[ne_g_i] : ne_tool == 2 ? (ne_road ? "Add road" : "Remove road") : 0;
        if (v) chip(2, 177, 110, 13, v, 0); else text_box(2, 177, 200, 13, FS_BODY, C15(24, 28, 31), AL_L, "Drag across the map");
        if (v) text_box(114, 177, 94, 13, FS_SMALL, C15(24, 28, 31), AL_L, "tap = next");
        chip(212, 177, 40, 13, "Done", 0);
    }
}

// ---------- the top screen: one lot in 3D ----------
#define T 330                  // a tile in 4.12 fixed point (1.0 = 4096), so a 13-tile lot is about a unit wide
#define LOTN 13
static int polys, gy_off, polys_last, fps_now, ui_n; static volatile int ui_fps, st_rec;   // (gy_off: the rise of a storey being drawn, D36)
static inline int tl(int t) { return (t - LOTN) * T; }   // tile coordinate -> centred model coordinate
// ---- (D39) GX DISPLAY LISTS: the lot's 3D is written into RAM as the geometry engine's own packed commands and sent to it by DMA.
// The still part (ground, floors, walls, things, streets, the lots round) is recorded only when something in it changes; what moves is recorded every frame.
// Saves the ARM9 nearly all of its 3D work; vertices that share two coordinates with the one before go as one word (VTX_XY / XZ / YZ).
#define GXS_MAX 49152
#define GXD_MAX 24576
#define GXW_MAX 16384
static u32 gx_wlb[2][GXW_MAX + 64] __attribute__((aligned(32)));   // (D41) the world round the lot
#define GXB_MAX 16384
static u32 gx_blb[2][GXB_MAX + 64] __attribute__((aligned(32)));   // (D41) the lot's build (lawn, floors, walls, roof)
static u32 gx_slb[2][GXS_MAX + 64] __attribute__((aligned(32))), gx_dlb[2][GXD_MAX + 64] __attribute__((aligned(32)));   // (two of each: one is being sent while the next is written)
// A FRAME is a run of pieces (the moving list cut where a sim's cached model goes in, the still list, the swap), sent one after another by DMA (channel 0)
// from the vblank interrupt: the 3D goes out on a steady beat whatever the game loop is doing.
typedef struct { const u32 *p; int n; } GSeg;
typedef struct { GSeg s[48]; int ns; } GFrm;
static GFrm gfr[2]; static volatile int gf_ready = -1, gf_fly = -1, gf_busy, gf_on, gf_shown; static int gf_ss;   // (gf_ss: where the piece being written began)

#ifdef DSTEST
volatile int test_scene
#ifdef AUTOSC
    = AUTOSC
#endif
    ; volatile int prof_ms[12]; static u32 prof_t[12], prof_last;   // (test builds only: where the ARM9's time goes, ms a second: steps, 2D and logic, moving list, still signature and list, DMA, the rest)
static inline u32 prof_now(void) { u16 lo = TIMER_DATA(0); u16 hi = TIMER_DATA(1); if (TIMER_DATA(0) < lo) hi = TIMER_DATA(1); return ((u32)hi << 16) | lo; }
#define PROF(k) do { u32 _n = prof_now(); prof_t[k] += _n - prof_last; prof_last = _n; } while (0)
u32 dbg_pt[3]; volatile int dbg_ui[4], dbg_mb[8], dbg_log[48][6], dbg_logn;   // dbg_log: passes of 3+ vblanks: vblank, steps, the HUD and the status draw in ticks   // (D70) ticks (33.5 MHz) of the last draw_hud pieces, draw_top_status, draw_ui; and the micro-bench of rect, text_box, face, chip
#define DBGT(k, ...) do { u32 _a = prof_now(); __VA_ARGS__; dbg_ui[k] = (int)(prof_now() - _a); } while (0)
#else
#define PROF(k) do {} while (0)
#define DBGT(k, ...) do { __VA_ARGS__; } while (0)
#endif
static int dpass;          // (D39) 0: draw everything (straight to the GPU), 1: the still part only (recorded once), 2: what moves only (every frame)
static u32 gx_base, gx_spec; static int gx_far = 12; static u32 *gxb; static int gxn, gxh, gxk = 4, gx_cap, gx_full, gx_lx, gx_ly, gx_lz, gx_lv; static u32 gx_mat, gx_nrm, gx_col, gx_attr;
static inline void gx_id(u32 id) { if (gxk >= 4) { gxh = gxn++; gxb[gxh] = 0; gxk = 0; } gxb[gxh] |= id << (gxk++ * 8); }
static inline void gx_c1(u32 id, u32 v) { gx_id(id); gxb[gxn++] = v; }
ITCM_CODE static void gx_c0(u32 id) { gxk = 4; gx_id(id); gxb[gxn++] = 0; gxk = 4; }   // (a command with no parameters gets a packed word of its own and a dummy zero, as GBATEK asks)
static int tx_want, gx_txp, imm_txp = -2;
static void gx_begin(u32 *b, int cap) { gxb = b; gxn = 0; gxk = 4; gx_cap = cap - 32; gx_full = 0; gx_lv = 0; gx_mat = gx_nrm = gx_col = ~0u; tx_want = -1; gx_txp = -2; }
static int gx_end(void) { gxb = 0; return gxn; }
static void gx_fmt(u32 attr) {   // the polygon attributes (takes effect at the next BEGIN): recorded, or straight to the GPU
    gx_attr = attr; if (gxb) { if (gxn > gx_cap) return; gx_c1(0x29, attr); gx_c1(0x40, GL_QUADS); gx_lv = 0; } else { glEnd(); glPolyFmt(attr); glBegin(GL_QUADS); } }
static void gx_pid(int id) {   // (D40) a polygon id for what follows (outlines are drawn where the id changes): only when outlines are on, each id costs a BEGIN
    if (gxb && (o_out || o_look == 2) && gxn < gx_cap) { gx_c1(0x29, (gx_base & ~0x3F000000u) | ((u32)id << 24)); gx_c1(0x40, GL_QUADS); gx_lv = 0; } }
static void gx_run(const u32 *b, int n) {   // send a list to the geometry FIFO by DMA (channel 0) and wait for it
    if (n <= 0) return; DC_FlushRange(b, n * 4); while (DMA_CR(0) & DMA_BUSY) {}
    DMA_SRC(0) = (u32)b; DMA_DEST(0) = 0x4000400; DMA_CR(0) = DMA_FIFO | n; while (DMA_CR(0) & DMA_BUSY) {} }
static GFrm *gf_rec;   // the frame whose moving list is being written (0: none)
static void gf_add(const u32 *p, int n) { if (gf_rec && n > 0 && gf_rec->ns < 48) { gf_rec->s[gf_rec->ns].p = p; gf_rec->s[gf_rec->ns].n = n; gf_rec->ns++; } }
static void gf_cut(void) { if (gf_rec) { gf_add(gxb + gf_ss, gxn - gf_ss); gf_ss = gxn; gxk = 4; } }   // end the piece written so far (the next command starts a fresh packed word)
static void gf_kick(u32 vb) {   // (from the vblank interrupt) the next frame, if one is ready and it is its turn
    if (!gf_on || gf_busy || gf_ready < 0 || (!o_fps30 && (vb & 1))) return;
    gf_fly = gf_ready; gf_ready = -1; gf_shown++; gf_busy = 1;   // (the pieces one after another, waiting on each: a DMA-end interrupt chain hung the sound's FIFO in melonDS)
    for (int k = 0; k < gfr[gf_fly].ns; k++) { const GSeg *g = &gfr[gf_fly].s[k]; DMA_SRC(0) = (u32)g->p; DMA_DEST(0) = 0x4000400; DMA_CR(0) = DMA_FIFO | (u32)g->n; while (DMA_CR(0) & DMA_BUSY) {} }
    gf_busy = 0; }
static void gf_stop(void) { gf_on = 0; while (gf_busy) {} gf_ready = -1; }   // (before the CPU writes to the GPU itself: the title, Create-a-Sim and the Map)
// (D43) TEXTURES: the PS1's ground, floor, wall and roof patterns (tools/dsbuild/mk_dstex.py -> ds_tex.h), 8x8 16-bit textures in VRAM bank A, one a cell, laid
// on each face by its plane (the ground by x and z, a wall by its run and its height). They hold the pattern's light and shade; the colour is the material's.
// Far off (the far ring, the horizon) nothing is textured: the plain colour is the pattern's average, a mip level of one texel, and the haze is over it.
#include "ds_tex.h"
static int far_lod;
static inline void gx_tex(int ax, int x, int y, int z) {   // the texture coordinate before a vertex (ax: 0 the ground, 1 a face along z, 2 a face along x)
    int sc = ax == 0 ? x : ax == 1 ? z : x, tc = ax == 0 ? z : -y; gx_c1(0x22, (u32)(((sc * 199) >> 10) & 0xFFFF) | ((u32)((tc * 199) >> 10) << 16)); }
static inline int gx_txon(void) {   // the texture for the next quad (0: none); emits the change
    int w = far_lod || !o_tex ? -1 : tx_want; if (w != gx_txp) { gx_c1(0x2A, w < 0 ? 0 : (u32)(w * 16) | (1u << 16) | (1u << 17) | (7u << 26)); gx_txp = w; } return w >= 0; }
static inline void gx_v(int x, int y, int z) {   // one vertex: the shortest command that sets it
    u32 xy = (u32)(x & 0xFFFF) | ((u32)y << 16);
    if (gx_lv) { if (z == gx_lz) { gx_c1(0x25, xy); goto done; } if (y == gx_ly) { gx_c1(0x26, (u32)(x & 0xFFFF) | ((u32)z << 16)); goto done; } if (x == gx_lx) { gx_c1(0x27, (u32)(y & 0xFFFF) | ((u32)z << 16)); goto done; } }
    gx_id(0x23); gxb[gxn++] = xy; gxb[gxn++] = (u32)(z & 0xFFFF);
done: gx_lx = x; gx_ly = y; gx_lz = z; gx_lv = 1; }
static void gx_cam(int dist, int pitch, int yaw, int cx, int cy, int cz) {   // (D39) the projection, the camera, the lights and the material into the list (the CPU must not write to the GPU while a frame is being sent)
    int nr = dist >= 10240 ? 4096 * 3 / 4 : dist * 3 / 10 < 700 ? 700 : dist * 3 / 10, ymax = mulf32(nr, tanLerp(degreesToAngle(40) >> 1)), xmax = mulf32(ymax, 256 * 4096 / 192), fr = 4096 * gx_far;   // (D55: zoomed in, the near plane stays in front of the sim: the camera was nearer than 0.75 and the faces were cut into shards)   // (D40: the far plane goes with the view: 14 to 22 units)
    gx_c1(0x10, 0); gx_c0(0x15); gx_id(0x18); u32 *o = &gxb[gxn]; memset(o, 0, 64);
    o[0] = (u32)divf32(2 * nr, 2 * xmax); o[5] = (u32)divf32(2 * nr, 2 * ymax); o[10] = (u32)-divf32(fr + nr, fr - nr); o[11] = (u32)-4096; o[14] = (u32)-divf32(2 * mulf32(fr, nr), fr - nr); gxn += 16;
    gx_c1(0x10, 2); gx_c0(0x15); gx_c1(0x10, 1);
    gx_id(0x1C); gxb[gxn++] = 0; gxb[gxn++] = 0; gxb[gxn++] = (u32)-dist;
    int c = cosLerp(pitch), sn = sinLerp(pitch); gx_id(0x1A); o = &gxb[gxn]; o[0] = 4096; o[1] = 0; o[2] = 0; o[3] = 0; o[4] = (u32)c; o[5] = (u32)sn; o[6] = 0; o[7] = (u32)-sn; o[8] = (u32)c; gxn += 9;
    c = cosLerp(yaw); sn = sinLerp(yaw); gx_id(0x1A); o = &gxb[gxn]; o[0] = (u32)c; o[1] = 0; o[2] = (u32)-sn; o[3] = 0; o[4] = 4096; o[5] = 0; o[6] = (u32)sn; o[7] = 0; o[8] = (u32)c; gxn += 9;
    gx_id(0x1C); gxb[gxn++] = (u32)-cx; gxb[gxn++] = (u32)-cy; gxb[gxn++] = (u32)-cz;
    if (lit_on) { gx_c1(0x31, gx_spec);
        gx_c1(0x32, (0u << 30) | ((u32)(-358 & 0x3FF) << 20) | ((u32)(-281 & 0x3FF) << 10) | (u32)(230 & 0x3FF)); gx_c1(0x33, (0u << 30) | RGB15(27, 25, 21));
        gx_c1(0x32, (1u << 30) | ((u32)(358 & 0x3FF) << 20) | ((u32)(51 & 0x3FF) << 10) | (u32)(-358 & 0x3FF)); gx_c1(0x33, (1u << 30) | RGB15(7, 10, 17)); }
    gx_c1(0x29, gx_base); gx_c1(0x40, GL_QUADS); gx_lv = 0;
}
static void gx_push(int x, int y, int z) { if (gxb) { gx_c0(0x11); gx_id(0x1C); gxb[gxn++] = (u32)x; gxb[gxn++] = (u32)y; gxb[gxn++] = (u32)z; gx_lv = 0; } else { glPushMatrix(); glTranslatef32(x, y, z); } }   // (a far lot drawn round its own centre: no 16-bit overflow)
static void gx_pop(void) { if (gxb) { gx_c1(0x12, 1); gx_lv = 0; } else glPopMatrix(1); }
ITCM_CODE static void qd_n(int x0, int y0, int z0, int x1, int y1, int z1, int x2, int y2, int z2, int x3, int y3, int z3, u16 c, u32 nm) {   // (D39) recording, the normal known (a box face)
    if (gxn > gx_cap) { gx_full = 1; return; }
    if (lit_on) { u32 m = (u32)(c | BIT(15)) | ((u32)RGB15(((c & 31) * 11) >> 4, (((c >> 5) & 31) * 11) >> 4, (((c >> 10) & 31) * 11) >> 4) << 16);
        if (m != gx_mat || nm != gx_nrm) { if (m != gx_mat) gx_c1(0x30, m); gx_c1(0x21, nm); gx_mat = m; gx_nrm = nm; } }
    else if (c != gx_col) { gx_c1(0x20, c); gx_col = c; }
    y0 += gy_off; y1 += gy_off; y2 += gy_off; y3 += gy_off;
    if (gx_txon()) { int ax = nm == NORMAL_PACK(0, 511, 0) ? 0 : (nm == NORMAL_PACK(511, 0, 0) || nm == NORMAL_PACK(-511, 0, 0)) ? 1 : 2;
        gx_tex(ax, x0, -y0, z0); gx_v(x0, -y0, z0); gx_tex(ax, x1, -y1, z1); gx_v(x1, -y1, z1); gx_tex(ax, x2, -y2, z2); gx_v(x2, -y2, z2); gx_tex(ax, x3, -y3, z3); gx_v(x3, -y3, z3); polys += 2; return; }
    gx_v(x0, -y0, z0); gx_v(x1, -y1, z1); gx_v(x2, -y2, z2); gx_v(x3, -y3, z3); polys += 2;
}
#define NRM_NX NORMAL_PACK(-511, 0, 0)
#define NRM_PX NORMAL_PACK(511, 0, 0)
#define NRM_NZ NORMAL_PACK(0, 0, -511)
#define NRM_PZ NORMAL_PACK(0, 0, 511)
#define NRM_UP NORMAL_PACK(0, 511, 0)
ITCM_CODE static void qd(int x0, int y0, int z0, int x1, int y1, int z1, int x2, int y2, int z2, int x3, int y3, int z3, u16 c) {
    if (gxb) {   // (D39) recording
        if (gxn > gx_cap) { gx_full = 1; return; }
        if (lit_on) { u32 m = (u32)(c | BIT(15)) | ((u32)RGB15(((c & 31) * 11) >> 4, (((c >> 5) & 31) * 11) >> 4, (((c >> 10) & 31) * 11) >> 4) << 16), nm = 0;
            s64 ux = x1 - x0, uy = y0 - y1, uz = z1 - z0, vx = x3 - x0, vy = y0 - y3, vz = z3 - z0, inx = uy * vz - uz * vy, iny = uz * vx - ux * vz, inz = ux * vy - uy * vx; int s = lit_sgn * 511;
            if (!inx && !iny) nm = NORMAL_PACK(0, 0, inz > 0 ? s : -s); else if (!inx && !inz) nm = NORMAL_PACK(0, iny > 0 ? s : -s, 0); else if (!iny && !inz) nm = NORMAL_PACK(inx > 0 ? s : -s, 0, 0);
            else { float nx = (float)inx, ny = (float)iny, nz = (float)inz, l = nx * nx + ny * ny + nz * nz; if (l > 0) { float k = 511.0f * lit_sgn / __builtin_sqrtf(l); nm = NORMAL_PACK((int)(nx * k), (int)(ny * k), (int)(nz * k)); } }
            if (m != gx_mat || nm != gx_nrm) { if (m != gx_mat) gx_c1(0x30, m); gx_c1(0x21, nm); gx_mat = m; gx_nrm = nm; } }
        else if (c != gx_col) { gx_c1(0x20, c); gx_col = c; }
        y0 += gy_off; y1 += gy_off; y2 += gy_off; y3 += gy_off;
        if (gx_txon()) { int ax = 0; { s64 ux = x1 - x0, uy = y1 - y0, uz = z1 - z0, vx = x3 - x0, vy = y3 - y0, vz = z3 - z0, nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
                if (nx < 0) nx = -nx; if (ny < 0) ny = -ny; if (nz < 0) nz = -nz; ax = ny >= nx && ny >= nz ? 0 : nx >= nz ? 1 : 2; }
            gx_tex(ax, x0, -y0, z0); gx_v(x0, -y0, z0); gx_tex(ax, x1, -y1, z1); gx_v(x1, -y1, z1); gx_tex(ax, x2, -y2, z2); gx_v(x2, -y2, z2); gx_tex(ax, x3, -y3, z3); gx_v(x3, -y3, z3); polys += 2; return; }
        gx_v(x0, -y0, z0); gx_v(x1, -y1, z1); gx_v(x2, -y2, z2); gx_v(x3, -y3, z3); polys += 2; return; }
    if (lit_on) {   // lit: set the material from the colour, then the normal (which computes the lit colour); no glColor after it
        glMaterialf(GL_DIFFUSE, (u16)(c | BIT(15))); glMaterialf(GL_AMBIENT, (u16)(RGB15(((c & 31) * 11) >> 4, (((c >> 5) & 31) * 11) >> 4, (((c >> 10) & 31) * 11) >> 4)));
        s64 ux = x1 - x0, uy = y0 - y1, uz = z1 - z0, vx = x3 - x0, vy = y0 - y3, vz = z3 - z0, inx = uy * vz - uz * vy, iny = uz * vx - ux * vz, inz = ux * vy - uy * vx;
        int s = lit_sgn * 511;   // (D35) a box face lies along an axis: its normal is known without floats or a square root (the ARM9 has no FPU; this was most of a frame)
        if (!inx && !iny) { if (inz) glNormal(NORMAL_PACK(0, 0, inz > 0 ? s : -s)); }
        else if (!inx && !inz) glNormal(NORMAL_PACK(0, iny > 0 ? s : -s, 0));
        else if (!iny && !inz) glNormal(NORMAL_PACK(inx > 0 ? s : -s, 0, 0));
        else { float nx = (float)inx, ny = (float)iny, nz = (float)inz, l = nx * nx + ny * ny + nz * nz;
            if (l > 0) { float k = 511.0f * lit_sgn / __builtin_sqrtf(l); glNormal(NORMAL_PACK((int)(nx * k), (int)(ny * k), (int)(nz * k))); } }
    } else glColor(c);
    y0 += gy_off; y1 += gy_off; y2 += gy_off; y3 += gy_off;
    { int w = far_lod || !o_tex ? -1 : tx_want;   // (D52) the direct path (the Map's lot picture, the title) takes the patterns too
      if (w != imm_txp) { GFX_TEX_FORMAT = w < 0 ? 0 : (u32)(w * 16) | (1u << 16) | (1u << 17) | (7u << 26); imm_txp = w; }
      if (w >= 0) { int ax; { s64 ux = x1 - x0, uy = y1 - y0, uz = z1 - z0, vx = x3 - x0, vy = y3 - y0, vz = z3 - z0, nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
                if (nx < 0) nx = -nx; if (ny < 0) ny = -ny; if (nz < 0) nz = -nz; ax = ny >= nx && ny >= nz ? 0 : nx >= nz ? 1 : 2; }
            #define ITX(x, y, z) { int sc_ = ax == 0 ? (x) : ax == 1 ? (z) : (x), tc_ = ax == 0 ? (z) : -(y); GFX_TEX_COORD = (u32)(((sc_ * 199) >> 10) & 0xFFFF) | ((u32)((tc_ * 199) >> 10) << 16); }
            ITX(x0, -y0, z0) glVertex3v16(x0, -y0, z0); ITX(x1, -y1, z1) glVertex3v16(x1, -y1, z1); ITX(x2, -y2, z2) glVertex3v16(x2, -y2, z2); ITX(x3, -y3, z3) glVertex3v16(x3, -y3, z3); polys += 2; return;
            #undef ITX
      } }
    glVertex3v16(x0, -y0, z0); glVertex3v16(x1, -y1, z1); glVertex3v16(x2, -y2, z2); glVertex3v16(x3, -y3, z3); polys += 2;
}
ITCM_CODE static u16 shade(u16 c, int k) {   // k/8 of the colour
    int r = ((c & 31) * k) >> 3, g = (((c >> 5) & 31) * k) >> 3, b = (((c >> 10) & 31) * k) >> 3;   // (k is never negative: >> 3 is / 8)
    return (u16)(RGB15(r > 31 ? 31 : r, g > 31 ? 31 : g, b > 31 ? 31 : b) | BIT(15));
}
ITCM_CODE static void ground(int x0, int z0, int x1, int z1, int y, u16 c) { if (x0 > x1) { int t = x0; x0 = x1; x1 = t; } if (z0 > z1) { int t = z0; z0 = z1; z1 = t; } qd(x0, y, z0, x1, y, z0, x1, y, z1, x0, y, z1, c); }
ITCM_CODE static void block(int x0, int z0, int x1, int z1, int y0, int y1, u16 wall, u16 roof) {   // a box (y up is negative in this view: we flip below)
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; } if (z0 > z1) { int t = z0; z0 = z1; z1 = t; } if (y1 > y0) { int t = y0; y0 = y1; y1 = t; }   // (D39: always wound the same way, so the GPU can skip the faces turned away)
    if (gxb) { u16 sw = shade(wall, 6);   // (D39) recording: the five normals are known
        qd_n(x0, y0, z0, x1, y0, z0, x1, y1, z0, x0, y1, z0, wall, NRM_NZ); qd_n(x1, y0, z0, x1, y0, z1, x1, y1, z1, x1, y1, z0, sw, NRM_PX);
        qd_n(x1, y0, z1, x0, y0, z1, x0, y1, z1, x1, y1, z1, wall, NRM_PZ); qd_n(x0, y0, z1, x0, y0, z0, x0, y1, z0, x0, y1, z1, sw, NRM_NX);
        qd_n(x0, y1, z0, x1, y1, z0, x1, y1, z1, x0, y1, z1, roof, NRM_UP); return; }
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
    qd(x0, y, z0, x0, y, z1, x0, y + ridge, zm, x0, y + ridge, zm, shade(roof, 5));   // (D36: the gable ends; they were drawn with y the wrong way up, under the ground)
    qd(x1, y, z0, x1, y + ridge, zm, x1, y, z1, x1, y + ridge, zm, shade(roof, 5));
}
static void tree(int x, int z, int s) {   // a trunk, three tiers of leaves, a bright top
    block(x - s / 8, z - s / 8, x + s / 8, z + s / 8, 0, -s * 5 / 10, C15(13, 8, 4), C15(13, 8, 4));
    block(x - s / 2, z - s / 2, x + s / 2, z + s / 2, -s * 4 / 10, -s * 7 / 10, C15(4, 15, 5), C15(6, 19, 6));
    block(x - s * 36 / 100, z - s * 36 / 100, x + s * 36 / 100, z + s * 36 / 100, -s * 7 / 10, -s * 95 / 100, C15(5, 17, 5), C15(8, 22, 7));
}
static void person(int x, int z, u16 c, int yaw) {   // a person: one quad that faces the camera
    int s = T, hx = (s * sinLerp(yaw)) >> 13, hz = (s * cosLerp(yaw)) >> 13;
    qd(x - hx, 0, z - hz, x + hx, 0, z + hz, x + hx, -s * 2, z + hz, x - hx, -s * 2, z - hz, c);
    qd(x + hx, 0, z + hz, x - hx, 0, z - hz, x - hx, -s * 2, z - hz, x + hx, -s * 2, z + hz, c);   // (D39: and its back, for the faces the GPU skips)
}
static int fm_season(void);
static u16 seas(u16 c) {   // (D50) the grass as the season makes it, as the PS1's world.c skin_c: spring greener, autumn toward brown, winter toward snow (summer as it is)
    int s = fm_season(); if (s == 1) return c;
    int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31, tr, tg, tb, f; if (s == 0) { tr = 9; tg = 27; tb = 8; f = 8; } else if (s == 2) { tr = 24; tg = 15; tb = 6; f = 11; } else { tr = 29; tg = 30; tb = 31; f = 15; }
    r += (tr - r) * f / 16; g += (tg - g) * f / 16; b += (tb - b) * f / 16; return (u16)(r | g << 5 | b << 10 | 0x8000); }
static u32 hsh(u32 a); static void roof_shape(int X0, int Z0, int X1, int Z1, int ry, int sty, int rise, u16 rc, u16 wc); static const u16 ROOFC[6];
static void draw_lot(const BInfo *b, int yaw) {
    u32 h = b->h; int k = b->lot;
    u16 grass = b->terrain == BT_DESERT ? C15(24, 20, 10) : b->terrain == BT_ROCKY ? C15(13, 13, 13) : b->terrain == BT_BEACH ? C15(26, 24, 14) : C15(11, 20, 8);
    if (b->terrain == BT_SEA) { grass = C15(5, 12, 24); } else if (b->terrain == BT_GRASS || b->terrain == BT_FOREST) grass = seas(grass);
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
    { u32 rh = hsh(h ^ 0x500Fu); int sty = st >= 3 || k > LOT_APART ? 1 : 2 + ((rh >> 2) & 1); static const s16 PIT[3] = { 110, 170, 250 };   // (D42: the map's picture has the lot's real roof)
      roof_shape(X0 - T / 3, Z0 - T / 3, X1 + T / 3, Z1 + T / 3, Y, sty, PIT[1 + ((rh >> 8) & 1)], ROOFC[(rh >> 4) % 6], wall); }
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
#include "ds_items.h"   // (D35) the catalogue: the DS's 52 items (ids 0-51, saved as c * 4 + i), then the PS1's others, in the PS1's ten tabs (tools/dsbuild/mk_dsitems.py)
#define OBJ_ID(o) ((o)->c * 4 + (o)->i)
#define IT_OF(o) (&ITEMS[OBJ_ID(o) < NITEM ? OBJ_ID(o) : 0])
static const u16 FLOORC[5] = { C15(22, 16, 9), C15(26, 26, 24), C15(10, 16, 24), C15(14, 22, 12), C15(8, 8, 10) };
static const u16 WALLC[6] = { C15(30, 28, 22), C15(28, 20, 16), C15(20, 24, 30), C15(30, 30, 30), C15(13, 8, 5), C15(28, 24, 14) };   // 5 = a door, 6 = a fence
static int mark_x = -1, mark_y, mark_t;
static void set_mode(int m);
static int opt_ret = -1;   // where Back in Options returns to (-1 = the title)
static int cas_mult = 1;
static void eff_pm(const u8 *pm, int *p) { for (int q = 0; q < PMN; q++) { int v = 50 + (pm[q] - 50) * cas_mult; p[q] = v; } }
static u32 lot_seed = 1;
static int camx, camz, door_cx, door_cy;
static int gframes, cam_on, cam_pan, cam_free, live_pitch = 4300, cam_deff = 9000, bb_yaw = 3000, bb_pitch = 3900, bb_dist = 12500, cam_lx, cam_ly, cam_drag;
static void sim3d(int i, int yaw);
static int has_board, on_board, board_lift; static long skate_score, skate_best; static const int *em_R; static int sk_x3, sk_z3, sk_cr, sk_Rm[9];   // (D44) the skateboard: owned, ridden, the rider's height (PS1 units); em_R: a free turn for the rider's model
static void skate_draw_board(int ground); static void skate_top(void); static unsigned skate_sig(void); static void skate_on(void), skate_off(void); static void skate_frame(int held, int down, int steps);
static int mg_ui, mg_ver, arcade_tix, fight_wins, fight_champ, fight_best; static void fight_3d(void), fight_cam(int *, int *, int *, int *, int *, int *); static unsigned mg_sig(void), mg_bsig(void); static void mg_top(void), mg_bottom(void), mg_frame(int held, int down, int steps, int tx, int ty); static void ag_open(int g), hs_open(void), gm_open(void), gt_minute(void); static int arcade_game_at(int x, int y), in_arcade(void), ag_price(int g); static const char *ag_title(int g);
#define MG_SAVE 256   // (D45 on) the most the mini-games' save blocks take, after the farm's
static int mg_pack(u8 *o); static int mg_unpack(const u8 *p, int left); static void mg_reset(void);
typedef struct { u8 x, y, c, i, rot; } Obj;
static Obj objs[400]; static int nobj, funds = 20000;
static void obj_fp(const Obj *o, int *w, int *h) { const Item *it = IT_OF(o); if (o->rot & 1) { *w = it->fl; *h = it->fw; } else { *w = it->fw; *h = it->fl; } }   // (D35) the cells it covers: a two-tile bed is 1 x 2, turned 2 x 1
// ---- (D36) WALKABLE STOREYS: up to UPN floors over the ground, each with its own floor (0: open air), walls and things; stairs join them (as the PS1's)
#define UPN 3                                  // upper storeys (the PS1 has 5): the ground and three more
#define WHF (T * 21 / 5)                       // a storey's height (the walls')
#define OBJ_LV(o) (((o)->rot >> 2) & 3)        // a thing's storey rides in its turn byte (bits 2-3), so every save's objects are still 5 bytes and old ones are on the ground
static u8 uflr[UPN][GN][GN], uhwl[UPN][GN + 1][GN], uvwl[UPN][GN][GN + 1];
static int LV, ed_lv, cam_y;                   // the storey being worked on (walking, taps, the plan), the one Buy and Build edit, the camera's rise
static void ups_clear(void); static int obj_solid(int cx, int cy);
static u16 occ[UPN + 1][GN][GN]; static int occ_ok; static u32 occ_sum;   // what stands on each cell of each storey: its object + 1 (bit 15: it blocks walking), rebuilt when the objects change
static int ds_oob;   // (test readout: things a generator put outside the plot, taken off)
static void occ_build(void) { for (int i = 0; i < nobj; ) { if (objs[i].x >= GN || objs[i].y >= GN) { objs[i] = objs[--nobj]; ds_oob++; } else i++; }   /* (nothing stands off the 13 x 13 plot, where the street and its pavement are) */
    memset(occ, 0, sizeof occ); for (int i = 0; i < nobj; i++) { int w, h, l = OBJ_LV(&objs[i]); if (l > UPN) continue; obj_fp(&objs[i], &w, &h); u16 v = (u16)((i + 1) | (IT_OF(&objs[i])->kind != 1 ? 0x8000 : 0)); for (int y = objs[i].y; y < objs[i].y + h && y < GN; y++) for (int x = objs[i].x; x < objs[i].x + w && x < GN; x++) occ[l][y][x] = v; } occ_ok = 1; }
static void occ_check(void) { u32 sm = (u32)nobj * 2654435761u; const u8 *q = (const u8 *)objs; for (int i = 0; i < nobj * 5; i++) sm = sm * 31 + q[i]; if (sm != occ_sum) { occ_sum = sm; occ_ok = 0; } }   // (once a frame, so nothing that moves an object is missed)
static u8 flr[GN][GN], hwl[GN + 1][GN], vwl[GN][GN + 1];     // floors (0 none, else 1..5), walls on the cell edges (0 none, else 1..4)
static u8 (*FLv(int L))[GN] { return L ? uflr[L - 1] : flr; }      // a storey's floor, walls along x, walls along z
static u8 (*HWv(int L))[GN] { return L ? uhwl[L - 1] : hwl; }
static u8 (*VWv(int L))[GN + 1] { return L ? uvwl[L - 1] : vwl; }
static int mode = M_MAP, cat, pick = -1, pcx = 6, pcy = 6, prot, tool, pal, cam = 3000, pitch = 3200, msg_t;
static const char *msg = "";
typedef struct { int x, y, tx, ty, wx, wy, need[6], acc[6], act, act_left, act_amt, act_obj, face, rel, dirn, age, soc, tgt; u8 skin, hair, hcol, shirt, beard, fc, pm[PMN]; u16 cc[3]; } Sim;   // x, y in 1/16 cells; needs 0..100: hunger, fun, energy, social, bladder, hygiene
#define SIM_MAX 6
static Sim sims[SIM_MAX]; static int cur_sim, nsim = 3, ctl, minutes = 11 * 60 + 40;
static u8 fid[SIM_MAX], fpar[SIM_MAX][2], fpt[SIM_MAX], fid_next = 1;   // (D62) who is whose: a stable id per sim, its two parents' ids, its partner's id (0: none). Its own save block ('FY'); old saves have none
static int fam_id(int i) { if (!fid[i]) { fid[i] = fid_next++; if (!fid_next) fid_next = 1; } return fid[i]; }
// ---- (D67) what Create-a-Sim adds to a Sim, kept beside it (the Sim struct is saved whole, so it stays as it was): a hat, gender, trousers, three traits, a lifetime wish. Its own save block 'CX'; old saves have none (all zero) ----
typedef struct { u8 hat, gen, wish, wdone, pants, tr[3]; } Cx;   // hat 0-5; gen 0 girl 1 boy; wish 0 none, 1-5; wdone 1 when done; pants 0-5; traits 0 none, 1-12
static Cx csx[SIM_MAX];
static const char *const HATN[6] = { "None", "Cap", "Beanie", "Top hat", "Party", "Crown" };
static const char *const PANTN[6] = { "Navy", "Black", "Tan", "Grey", "Red", "Green" };
static const u16 PANTC[6] = { C15(6, 7, 12), C15(4, 4, 5), C15(20, 17, 10), C15(14, 14, 16), C15(22, 7, 7), C15(8, 18, 10) };
static const char *const TRN[12] = { "Neat", "Slob", "Foodie", "Light eater", "Night owl", "Sleepyhead", "Friendly", "Loner", "Playful", "Easygoing", "Iron bladder", "Tiny bladder" };
static const char *const TRD[12] = { "Stays clean for longer", "Gets grubby fast", "Gets hungry fast", "Hunger comes slowly", "Needs less sleep", "Needs lots of sleep", "Wants to chat a lot", "Happy on their own", "Gets bored fast", "Is never bored", "Can wait to go", "Has to go often" };
static const u8 TRE[12][2] = { {5, 0}, {5, 1}, {0, 1}, {0, 0}, {2, 0}, {2, 1}, {3, 1}, {3, 0}, {1, 1}, {1, 0}, {4, 0}, {4, 1} };   // the need a trait touches (hunger fun energy social bladder hygiene = 0-5) and 0 slower, 1 faster
static const char *const WSN[5] = { "Swimming in Cash", "Climb the Ladder", "Landlord", "Big Family", "Fight Champion" };
static const char *const WSD[5] = { "Have $20,000.", "Get promoted twice.", "Own 3 lots.", "Have 5 people at home.", "Win the Bortua ladder." };
static u8 slv[SIM_MAX];   // (D36) the storey each sim is on
static char sname[SIM_MAX][20] = { "Ashley Newbie", "Ronald Bacon", "Kit Farlow" };
static const char *const STGN[6] = { "Baby", "Toddler", "Child", "Teen", "Adult", "Elder" };   // the life stages: 0-2, 3-5, 6-12, 13-17, 18-59, 60+ years (a year is a day on the clock)
// ============ LIFE (D31): careers, wants and fears, the dog, ghosts, roofs and storeys ============
#define JOBN 6
enum { WN_NONE, WN_EAT, WN_SLEEP, WN_SHOWER, WN_FUN, WN_CHAT, WN_FRIEND, WN_JOB, WN_WORK, WN_PAID, WN_PROMO, WN_CASH, WN_BUY, WN_VISIT, WN_BABY, WN_PET, WN_SONG, WN_N };
enum { FR_NONE, FR_PASS, FR_WET, FR_BAD, FR_DEMOTE, FR_GHOST, FR_N };
typedef struct { u8 job, lvl, prog, strike, fear, worked, wk[3]; s16 wa[3]; u16 pts, shifts; } Life;
typedef struct { s16 bx, by; int x, y, tx, ty; u8 skin, hair, shirt, on; } Ghost;
#define GH_MAX 4
static Life life[SIM_MAX]; static Ghost gh[GH_MAX]; static int ngh, gh_next;
static int life_ver, info_pg, jobui, juke, juke_pg, dog_have, roof_st, up_st;
static struct { int x, y, tx, ty, wx, wy, hun, fx, fz; } dog;
static void life_ev(int i, int ev, int arg), ghost_add(const Sim *m), dog_reset(void), dog_draw(void), ghost_draw(void), day_roll(void); static int cl100(int v);
static int stage_of(int a) { return a < 3 ? 0 : a < 6 ? 1 : a < 13 ? 2 : a < 18 ? 3 : a < 60 ? 4 : 5; }
static const int STSC[6] = { 36, 50, 68, 88, 100, 95 };   // how tall each stage is, in percent
static u8 ilim[SIM_MAX][SIM_MAX];   // how many times two household members have talked today
static const u16 SKINC[6] = { C15(30, 24, 20), C15(27, 20, 15), C15(22, 15, 10), C15(16, 10, 6), C15(11, 7, 4), C15(29, 22, 17) };
static const u16 HAIRC[6] = { C15(3, 3, 4), C15(14, 8, 4), C15(26, 20, 8), C15(24, 8, 4), C15(24, 24, 24), C15(10, 14, 28) };
static const u16 SHIRTC[6] = { C15(26, 26, 26), C15(24, 8, 8), C15(8, 14, 26), C15(8, 22, 10), C15(28, 22, 8), C15(16, 8, 22) };
static inline u16 skin_of(const Sim *m) { return m->cc[0] ? m->cc[0] : SKINC[m->skin]; }
static inline u16 hair_of(const Sim *m) { return m->cc[1] ? m->cc[1] : HAIRC[m->hcol]; }
static inline u16 shirt_of(const Sim *m) { return m->cc[2] ? m->cc[2] : SHIRTC[m->shirt]; }
#define NHAIRD 26   // (D62) the PS1's 26 hair styles: the DS's old 12 keep their numbers (old saves), the 14 others follow
static const char *const HAIRN[NHAIRD] = { "Short", "Bob", "Spikes", "Long", "Braids", "Bald", "Buzz", "Mohawk", "Afro", "Ponytail", "Fringe", "Pigtails",
    "Flattop", "Sidetail", "Bun", "Pixie", "Curls", "Sidepart", "Slicked", "Wavy", "Friar", "Thinning", "Messy", "Topknot", "Curtains", "Mullet" };
static const u8 DS2PS[NHAIRD] = { 0, 9, 4, 2, 18, 3, 15, 12, 5, 10, 1, 11, 6, 7, 8, 13, 14, 16, 17, 19, 20, 21, 22, 23, 24, 25 };   // DS number -> the PS1's style (hstyles order)
static const u8 DS2OLD[NHAIRD] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 6, 0, 0, 0, 8, 0, 0, 3, 0, 6, 2, 0, 10, 3 };   // the flat 2D portraits draw a new style as the nearest old one
#include "ds_hair.h"
static inline int hair2d(int h) { return DS2OLD[h % NHAIRD]; }
static const char *const BEARDN[6] = { "None", "Stubble", "Mustache", "Goatee", "Beard", "Long" };
static const char *const FACEN[6] = { "Classic", "Round", "Long", "Big Eyes", "Sleepy", "Freckles" };
static int st_days, st_born, st_died;   // lifetime stats

// ---- sound: the ARM7 plays the DS's square-wave voices; the game sends notes to them. Four voices: lead, bass, pad (the music) and one for effects.
static int vid[4], snd_on, snd_dirty = 1, song = -2, sstep, stick, sfx_n = -1, sfx_i, sfx_t, sfx_sh;
static const u16 MB[12] = { 1635, 1732, 1835, 1945, 2060, 2183, 2312, 2450, 2596, 2750, 2914, 3087 };
static int mfreq(int m) { return MB[m % 12] * (1 << (m / 12 - 1)) / 100; }
static const u8 SONGL[4][32] = {
    { 72, 0, 76, 0, 79, 0, 76, 0, 74, 0, 77, 0, 81, 0, 77, 0, 72, 0, 76, 79, 84, 0, 79, 0, 77, 0, 76, 74, 72, 0, 0, 0 },
    { 69, 0, 72, 0, 76, 0, 72, 0, 71, 0, 74, 0, 77, 0, 74, 0, 69, 0, 72, 76, 81, 0, 76, 0, 74, 0, 72, 71, 69, 0, 0, 0 },
    { 79, 83, 86, 83, 79, 83, 86, 0, 81, 84, 88, 84, 81, 84, 88, 0, 79, 83, 86, 91, 88, 86, 83, 79, 74, 78, 81, 78, 74, 0, 0, 0 },
    { 0, 0, 67, 0, 0, 0, 71, 0, 0, 0, 74, 0, 0, 0, 71, 0, 0, 0, 64, 0, 0, 0, 67, 0, 0, 0, 71, 0, 0, 0, 0, 0 } };
static const u8 SONGB[4][8] = { { 48, 55, 53, 55, 48, 57, 53, 55 }, { 45, 40, 43, 40, 45, 41, 43, 40 }, { 43, 50, 48, 50, 43, 52, 50, 47 }, { 40, 40, 43, 38, 40, 40, 45, 43 } };
static const u8 SONGF[4] = { 12, 16, 9, 22 };   // frames per step
static const u8 SFXN[10][6] = { { 84, 0 }, { 72, 79, 84, 0 }, { 48, 45, 0 }, { 72, 76, 79, 84, 0 }, { 79, 84, 88, 0 }, { 72, 76, 79, 84, 88, 0 }, { 91, 96, 0 }, { 86, 0 }, { 79, 88, 0 }, { 84, 74, 0 } };   // (D70) 7 move, 8 confirm, 9 back
static const u8 SFXD[10][6] = { { 2 }, { 4, 4, 9 }, { 6, 12 }, { 4, 4, 4, 12 }, { 8, 8, 16 }, { 4, 4, 4, 4, 14 }, { 2, 3 }, { 1 }, { 2, 5 }, { 2, 5 } };
static void snd_init(void) {
    soundEnable(); static const u8 dc[4] = { DutyCycle_25, DutyCycle_50, DutyCycle_12, DutyCycle_50 };
    for (int i = 0; i < 4; i++) vid[i] = soundPlayPSG((DutyCycle)dc[i], 8 * 440, 0, 64); snd_on = 1;
}
static void vset(int v, int m, int vol) { if (!snd_on || vid[v] < 0) return; if (!m || vol <= 0) { soundSetVolume(vid[v], 0); return; } soundSetFreq(vid[v], (u16)(8 * mfreq(m))); soundSetVolume(vid[v], (u8)(vol > 127 ? 127 : vol)); }
static int mvol(int base) { return base * o_vol * o_mus / 100; }
static void sfx(int n) {
    if (!snd_on || !o_sfx || !o_vol || ((n == 0 || n >= 7) && !o_ckl) || (n >= 3 && n <= 5 && !o_chm)) return;
    if (sfx_n >= 0 && (n == 0 || n == 7)) return; sfx_n = n; sfx_i = 0; sfx_t = 0; sfx_sh = 0; }
static void sfx_p(int n, int sh) { sfx(n); if (sfx_n == n) sfx_sh = sh; }   // (D64) a pet's voice: the same jingle a few semitones up or down
static void snd_apply(void) { snd_dirty = 1; }
static int mus_ok = -1, mus_cur = -1, mus_n; static void mus_init(void), mus_stop(void), mus_tick(void); static int mus_start(int);
static void snd_tick(void) {
    if (!snd_on) return;
    if (snd_dirty) { snd_dirty = 0; static const u8 pn[2][4] = { { 64, 64, 64, 64 }, { 36, 92, 64, 64 } }; for (int i = 0; i < 4; i++) if (vid[i] >= 0) soundSetPan(vid[i], pn[o_pan][i]); song = -2; }
    int title = mode == M_TITLE || mode == M_CAS, quiet = title ? !o_tmus : (!o_gmus && !(o_msrc == 0 ? o_song : o_trk));   // (Title music / Game music off: quiet, unless you picked a song or a track yourself)
    int want = -1; if (o_mus && o_vol && !quiet) { if (o_trk) want = o_trk - 1; else { int h = (minutes / 60) % 24; want = mode == M_TITLE || mode == M_CAS ? 0 : mode == M_BUY || mode == M_BUILD ? 2 : h >= 22 || h < 6 ? 3 : h >= 18 ? 1 : 0; } }
    { int songs_on = o_msrc == 0 && o_mus && o_vol && !quiet && mus_ok != 0;
      if (mus_ok < 0) mus_init();
      if (songs_on && mus_ok == 1) {
          int pick = o_song ? o_song - 1 : (mode == M_TITLE || mode == M_CAS) ? 0 : 0;
          if (o_song == 0 && mode != M_TITLE && mode != M_CAS && (mus_cur < 0 || mus_cur == 0) && mus_n > 1) { static u32 rs = 12345; static int last; rs = rs * 1664525u + 1013904223u + (u32)minutes;   // (Shuffle songs: at random, or in order)
              pick = o_shuf ? 1 + (int)((rs >> 8) % (u32)(mus_n - 1)) : last % (mus_n - 1) + 1; last = pick; }
          if (mus_cur != pick && !(o_song == 0 && mode != M_TITLE && mode != M_CAS && mus_cur > 0)) mus_start(pick);
          else if (mus_cur < 0) mus_start(pick);
          if (mus_cur >= 0) mus_tick();
          want = -1; if (song >= 0) { vset(0, 0, 0); vset(1, 0, 0); vset(2, 0, 0); song = -1; }
      } else if (mus_cur >= 0) mus_stop(); }
    if (want != song) { song = want; sstep = 0; stick = 0; vset(0, 0, 0); vset(1, 0, 0); vset(2, 0, 0); }
    if (song >= 0) { int f = SONGF[song];
        if (stick == 0) { int nl = SONGL[song][sstep]; vset(0, nl, mvol(60)); if (!(sstep & 3)) { int nb = SONGB[song][(sstep >> 2) & 7]; vset(1, nb, mvol(55)); vset(2, nb + 12 + 7, mvol(22)); } }
        else if (stick == f * 2 / 3) vset(0, SONGL[song][sstep], mvol(34)); else if (stick == f - 1 && SONGL[song][sstep]) vset(0, 0, 0);
        if (++stick >= f) { stick = 0; sstep = (sstep + 1) & 31; } }
    if (sfx_n >= 0) { int m = SFXN[sfx_n][sfx_i]; if (m) { m += sfx_sh; if (m < 12) m = 12; if (m > 108) m = 108; } if (!SFXN[sfx_n][sfx_i]) { vset(3, 0, 0); sfx_n = -1; } else { if (sfx_t == 0) vset(3, m, o_vol * o_sfx * 90 / 100); if (++sfx_t >= SFXD[sfx_n][sfx_i]) { sfx_i++; sfx_t = 0; } } }
}

// ---- THE SONGS: the PS1 game's jukebox on the DS, with no streaming. tools/dsmusic turns every song into a note list plus its instruments' samples
// (IMA-ADPCM), and tools/dsbuild/addmusic.py appends it to the ROM. The song you hear is read off the cartridge into RAM, and each note is one of the
// sound chip's sample channels. If the cartridge cannot be read (a loader that only gives us the game's code), the synth tunes play instead.
#define MUS_BUFSZ 0x4A000
static u8 mus_buf[MUS_BUFSZ] __attribute__((aligned(32)));
static u8 mus_dir[8192] __attribute__((aligned(32)));
static int mus_ent, mus_laps, m_hd, m_cnt, m_ch[16], mus_next = -1, mus_play;
static u32 mus_base; static const u8 *mp; static u32 m_loop, m_smp, m_tbl;
static volatile const u32 MUS_LOC[4] __attribute__((used, aligned(4))) = { 0x41444D42u, 0, 0, 0x2E4D4442u };   // 'BMDA', the songs' ROM address (patched in), the size, '.BDM'
static int cart_blk(u32 addr, u32 *dst) {   // 512 bytes of the cartridge at addr (a multiple of 512)
    u32 rc = *(vu32 *)0x027FFE60; if (!rc) rc = 0x00586000;
    sysSetCartOwner(true); REG_AUXSPICNTH = CARD_CR1_ENABLE;
    cardParamCommand(CARD_CMD_DATA_READ, addr, (rc & 0x00FFFFFF) | CARD_ACTIVATE | CARD_nRESET | CARD_BLK_SIZE(1), dst, 128);
    return 1;
}
static void cart_read(u32 addr, void *dst, u32 len) { u32 *d = (u32 *)dst; for (u32 o = 0; o < len; o += 512) cart_blk(addr + o, d + o / 4); }
static u32 rd32(const u8 *p) { return p[0] | p[1] << 8 | p[2] << 16 | (u32)p[3] << 24; }
static void mus_init(void) {
    mus_ok = 0; static u32 blk[128]; const volatile u32 *loc = MUS_LOC; if (loc[0] != 0x41444D42u || !loc[1] || loc[3] != 0x2E4D4442u) return;   // (addmusic.py wrote where the songs start)
    mus_base = loc[1]; cart_blk(mus_base, blk); if (memcmp(blk, "BDM1", 4)) return;
    cart_read(mus_base, mus_dir, 4096); 
    mus_n = (int)rd32(mus_dir + 4); mus_ent = (int)rd32(mus_dir + 8); if (mus_n < 1 || mus_n > 64 || mus_ent != 96 || 16 + mus_n * mus_ent > (int)sizeof mus_dir) return;
    if (16 + mus_n * mus_ent > 4096) cart_read(mus_base + 4096, mus_dir + 4096, 4096);
    mus_ok = 1;
}
static const u8 *mus_e(int n) { return mus_dir + 16 + n * mus_ent; }
static const char *mus_name(int n) { return n >= 0 && n < mus_n ? (const char *)mus_e(n) + 40 : ""; }
static void mus_stop(void) { for (int i = 0; i < 16; i++) { if (m_ch[i] >= 0) soundKill(m_ch[i]); m_ch[i] = -1; } mus_cur = -1; }
static int mus_start(int n) {
    mus_stop(); if (mus_ok != 1 || n < 0 || n >= mus_n) return 0;
    const u8 *e = mus_e(n); u32 len = rd32(e + 4); if (len > MUS_BUFSZ) return 0;
    cart_read(mus_base + rd32(e), mus_buf, len); DC_FlushRange(mus_buf, len);
    m_tbl = rd32(e + 8); m_smp = rd32(e + 12); m_loop = rd32(e + 20); mp = mus_buf; m_hd = 0; m_cnt = 0; mus_laps = 0; mus_cur = n; return 1;
}
static void mus_tick(void) {   // once a frame: play the notes that are due
    int vs = o_vol * o_mus;
    for (int guard = 0; guard < 400; guard++) {
        if (!m_hd) { int d = *mp++; if (d == 255) { m_cnt = 254; m_hd = 2; } else { m_cnt = d; m_hd = 1; } }
        if (m_cnt > 0) { m_cnt--; if (m_cnt) return; }
        if (m_hd == 2) { m_hd = 0; continue; }
        m_hd = 0; int ev = *mp++;
        if (ev == 0xFF) { mus_laps++; if (o_song == 0 && mus_cur != 0) { mus_cur = -1; return; } mp = mus_buf + m_loop; continue; }
        if (ev == 0xFE) continue;
        if (ev & 0x80) { int sl = ev & 15; if (m_ch[sl] >= 0) { soundKill(m_ch[sl]); m_ch[sl] = -1; } continue; }
        int sl = ev & 15, kill = ev & 0x40, in = mp[0], hz = mp[1] | mp[2] << 8, vol = mp[3], pan = mp[4]; mp += 5;
        if (kill && m_ch[sl] >= 0) { soundKill(m_ch[sl]); m_ch[sl] = -1; }
        int v = vol * vs / 72; if (v > 127) v = 127; if (v < 1) { m_ch[sl] = -1; continue; }
        const u8 *ti = mus_buf + m_tbl + in * 8; u32 so = rd32(ti), sz = rd32(ti + 4);
        m_ch[sl] = sz ? soundPlaySample(mus_buf + m_smp + so, SoundFormat_ADPCM, sz, (u16)hz, (u8)v, o_pan ? (u8)pan : 64, false, 0) : -1;
    }
}
static const char *clk(char *b, int mn) { int h = (mn / 60) % 24, m = mn % 60, h12 = h % 12 ? h % 12 : 12; if (o_c24) sprintf(b, "%02d:%02d", h, m); else sprintf(b, "%d:%02d %s", h12, m, h >= 12 ? "pm" : "am"); return b; }
static void say(const char *m) { static const u8 MT[3] = { 45, 90, 150 }; msg = m; msg_t = MT[o_msgt % 3];   // (Options > Game > Message time)
   
    if (strstr(m, "Bought") || strstr(m, "Sold")) sfx(1); else if (strstr(m, "saved") || strstr(m, "loaded")) sfx(3); else if (strstr(m, "is now a")) sfx(5);
    else if (strstr(m, "Cannot") || strstr(m, "full") || strstr(m, "Not ") || strstr(m, "Too ") || strstr(m, "failed") || strstr(m, "No ") || strstr(m, "damaged")) sfx(2); }
static int obj_at(int x, int y) { if (x < 0 || y < 0 || x >= GN || y >= GN) return -1; if (!occ_ok) occ_build(); return (occ[LV][y][x] & 0x7FFF) - 1; }   // the object covering that cell (of the storey LV)
static int spawn_x[3], spawn_y[3];

// ---------- the open street world (D16): the lot you live on is the middle of a 3x3 patch of lot slots; walk off it and the next lot is there ----------
#define SLC 18                         // a slot is 18 cells (36 tiles): the lot (13 cells), a pavement, a 3-cell road, a pavement
static int nb_ok; static BInfo nb[3][3]; static u8 nb_pop[3][3], nb_fam[3][3];
static int fdiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }
enum { Z_CITY, Z_BASE, Z_ISLAND, Z_TOWN, Z_N };
static int zone; static void zone_block(int bx, int by, BInfo *b); static void gen_zone(int bx, int by);   // (D37) the zone you are in, and its lots
#define RING_MAX 3
static BInfo nbf[RING_MAX * 2 + 1][RING_MAX * 2 + 1];   // (D40) the lots round, out to three slots each way (the ring past the 3x3 is only drawn)
static void nb_load(void) {
    for (int dy = -RING_MAX; dy <= RING_MAX; dy++) for (int dx = -RING_MAX; dx <= RING_MAX; dx++) { int x = lot_bx + dx, y = lot_by + dy; BInfo *b = &nbf[dy + RING_MAX][dx + RING_MAX];
        if (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) continue;
        if (lot_bx >= 2000) zone_block(x, y, b); else if (x < 0 || y < 0 || x >= BM_N || y >= BM_N) { memset(b, 0, sizeof(BInfo)); b->terrain = BT_SEA; } else bm_block(x, y, b); }
    for (int dy = 0; dy < 3; dy++) for (int dx = 0; dx < 3; dx++) { int x = lot_bx + dx - 1, y = lot_by + dy - 1;
    if (x >= 2000) { zone_block(x, y, &nb[dy][dx]); nb_pop[dy][dx] = 0; nb_fam[dy][dx] = 0; continue; }
    if (x < 0 || y < 0 || x >= BM_N || y >= BM_N) { memset(&nb[dy][dx], 0, sizeof(BInfo)); nb[dy][dx].terrain = BT_SEA; nb_pop[dy][dx] = 0; nb_fam[dy][dx] = 0; }
    else { bm_block(x, y, &nb[dy][dx]); int f; nb_pop[dy][dx] = (u8)pop_at(x, y, &f); nb_fam[dy][dx] = (u8)f; } }
    for (int dy = 0; dy < 3; dy++) for (int dx = 0; dx < 3; dx++) nbf[dy + RING_MAX - 1][dx + RING_MAX - 1] = nb[dy][dx]; }
static void city_spawn(void); static void city_shift(int dx, int dy);
static int far_house(const BInfo *b) { return b->terrain != BT_SEA && b->lot >= LOT_HOUSE && b->lot != LOT_PARK && b->lot != LOT_SQUARE && b->lot != LOT_SKATE && b->lot != LOT_EMPTY && b->lot != LOT_RIVER; }
static void far_foot(const BInfo *b, int *x0, int *z0, int *x1, int *z1) { int w = 10 + (int)(b->h & 3) * 2, d = 8 + (int)((b->h >> 2) & 3) * 2, t0 = 4 + (24 - w) / 2 / 2 * 2 - 4; *x0 = t0 / 2; *x1 = (t0 + w + 1) / 2; *z0 = 2; *z1 = (4 + d + 1) / 2; }
static int far_solid(int wx, int wy) {   // a cell outside the plot: open street and grass, except the sea and the buildings of the lots round it
    int sx = fdiv(wx, SLC), sy = fdiv(wy, SLC); if (sx == 0 && sy == 0) return 0; if (sx < -1 || sx > 1 || sy < -1 || sy > 1) return 1;
    const BInfo *b = &nb[sy + 1][sx + 1]; if (b->terrain == BT_SEA) return 1; if (!far_house(b)) return 0;
    int lx = wx - sx * SLC, ly = wy - sy * SLC, x0, z0, x1, z1; far_foot(b, &x0, &z0, &x1, &z1); return lx >= x0 && lx < x1 && ly >= z0 && ly < z1;
}
static int z_put(int x, int y, int ch, int rot);
static int gen_two;   // (D36) the lot is a TWO-STOREY: the house gets its loft
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
    if (gen_two) {   // the loft: a floor over the back rooms, walls round it, stairs up from the living room (they rise to the back), a bed, a lamp and a rug up there
        for (int yy = y0; yy < ym; yy++) for (int xx = x0; xx < x1; xx++) uflr[0][yy][MC(xx)] = 2;
        for (int xx = x0; xx < x1; xx++) { uhwl[0][y0][MC(xx)] = ow; uhwl[0][ym][MC(xx)] = ow; } for (int yy = y0; yy < ym; yy++) { uvwl[0][yy][ME(x0)] = ow; uvwl[0][yy][ME(x1)] = ow; }
        uhwl[0][ym][MC(x0 + 5)] = 0;                                         // (the top of the stairs: open)
        PUT(x0 + 5, ym, ID_STAIRS >> 2, ID_STAIRS & 3, 0);                  // the stairs (their landing: the loft's cell behind them)
        PUT(x0 + 1, y0, 4, 0, 4); PUT(x0, y0, 3, 0, 4); PUT(x0 + 3, y0 + 1, 3, 1, 4); PUT(x1 - 2, y0, 2, 1, 4);   // bed, lamp, rug, computer (rot | storey 1 << 2)
        roof_st = 2; }
    {   // (D68) the yard, as the PS1's yard(): the path from the front door out through a gate in the fence, planters either side, trees in a pair, a lamp, the mailbox and a bin
        for (int y = y1; y <= 12; y++) flr[y][MC(xd)] = 2;
        for (int x = x0 + 1; x < x1; x++) if (x != xd) hwl[12][MC(x)] = 6;
        z_put(MC(xd - 2), y1, 'Z', 0); z_put(MC(xd + 2), y1, 'Z', 0); z_put(MC(xd + 1), y1, 'l', 0);
        z_put(MC(x0 + 1), y1 + 1, '3', 0); z_put(MC(x1 - 2), y1 + 1, '3', 0); z_put(MC(xd - 4), y1, 'p', 0); z_put(MC(xd + 4), y1, 'p', 0);
        z_put(MC(xd + 2), y1 + 2, (char)0xE0, 0); z_put(MC(xd - 2), y1 + 2, 'Y', 0); }
    pcx = MC(xd); pcy = y1 + 1; door_cx = MC(xd); door_cy = y1; { static const int sp[3] = { 1, 4, 5 }; for (int k = 0; k < 3; k++) { spawn_x[k] = MC(x0 + sp[k]); spawn_y[k] = y1 - 3; } }
    #undef PUT
    #undef FL
}
// ---- FARM LIFE (D38): the PS1's farm (sim.c: farm_midnight, the crop bed / hen coop / cow / farm stand choices) and its ledger (farm_ui.c). One farm for the household, as on the PS1 ----
static u8 fm_n[4], fm_g[4], fm_dry[4], fm_wet, fm_hens, fm_cows, fm_fed, fm_fert, fm_fz, fm_pest, fm_up, fm_wc, fm_ordk, fm_ordq, fm_ordt, fm_fair, fm_cdone, fm_cday = 255;
static u16 fm_stock[4], fm_egg, fm_milk, fm_pe, fm_pm, fm_built, fm_last, fsk[SIM_MAX];   // (fm_built / fm_last / fm_cdone / fm_cday: the ledger's pk_data 16-21; fsk: each Bore's farming skill, the PS1's skill_farm)
static int farm_ui, farm_tab; static volatile u32 vbl_n;   // (vbl_n: the hens peck by the frame)
static int fm_skl(int i) { int l = fsk[i] / 400; return l > 10 ? 10 : l; }   // (the PS1's SKL: a skill counter to its level 0..10)
static int fm_season(void) { return o_rt && rt_mon >= 0 ? (rt_mon + 10) / 3 % 4 : (st_days / 10) & 3; }   // (the PS1's seasons: ten days each, spring first; Real time: by the month, March spring .. December winter)
static const char *const SEASN[4] = { "Spring", "Summer", "Autumn", "Winter" };
static void farm_reset(void) { memset(fm_n, 0, 4); memset(fm_g, 0, 4); memset(fm_dry, 0, 4); fm_wet = fm_hens = fm_cows = fm_fed = fm_fert = fm_fz = fm_pest = fm_up = fm_wc = fm_ordk = fm_ordq = fm_ordt = fm_fair = fm_cdone = 0; fm_cday = 255;
    memset(fm_stock, 0, sizeof fm_stock); fm_egg = fm_milk = fm_pe = fm_pm = fm_built = fm_last = 0; memset(fsk, 0, sizeof fsk); farm_ui = 0; }
static int z_put(int x, int y, int ch, int rot); static void z_room(int x0, int y0, int x1, int y1, int fl, int wc);
static void gen_public(int k, u32 h) {   // lots that are not homes: parks, squares, farms and wild land outdoors; shops, diners, gyms, schools, clinics... as one open hall
    #define PUTP(xx, yy, cc, ii, rr) do { if (nobj < 400) { objs[nobj].x = (u8)(xx); objs[nobj].y = (u8)(yy); objs[nobj].c = (cc); objs[nobj].i = (ii); objs[nobj].rot = (u8)(rr); nobj++; occ_ok = 0; } } while (0)
    int outdoor = k == LOT_WILD || k == LOT_PARK || k == LOT_SQUARE || k == LOT_SKATE || k == LOT_EMPTY || k == LOT_RIVER || k == LOT_FARM;
    door_cx = 6; pcx = 6; pcy = 11;
    if (outdoor) {
        door_cy = 12;
        if (k == LOT_SQUARE || k == LOT_SKATE) for (int y = 2; y < 11; y++) for (int x = 2; x < 11; x++) flr[y][x] = 2;
        if (k == LOT_SKATE) {   // (D44) the PS1's SKATE PLAZA (world.c city_skate): quarter pipes round three sides, a funbox with a kicker either side, a rail, a ledge, a manual pad, a launch ramp
            for (int y = 0; y < GN; y++) for (int x = 0; x < GN; x++) flr[y][x] = 2;
            for (int x = 1; x <= 10; x++) { z_put(x, 0, '5', 0); z_put(x, 1, '5', 0); z_put(x, 12, '5', 2); z_put(x, 11, '5', 2); }
            for (int y = 2; y <= 10; y++) { z_put(12, y, '5', 3); z_put(11, y, '5', 3); }
            for (int y = 5; y <= 6; y++) { z_put(4, y, '1', 3); for (int x = 5; x <= 7; x++) z_put(x, y, 'X', 0); z_put(8, y, '1', 1); }
            for (int x = 3; x <= 8; x++) { z_put(x, 3, '=', 0); z_put(x, 9, 'L', 0); }
            for (int y = 5; y <= 6; y++) { z_put(1, y, 'M', 0); z_put(2, y, 'M', 0); }
            z_put(1, 8, '9', 3); z_put(2, 8, '9', 3); z_put(12, 0, 'Y', 0); z_put(12, 12, 'Y', 0); z_put(11, 0, 'Y', 0); z_put(0, 1, 'B', 0);
            door_cx = 0; door_cy = 6; for (int q = 0; q < 3; q++) { spawn_x[q] = 0; spawn_y[q] = 5 + q; } return; }
        if (k == LOT_PARK || k == LOT_SQUARE) {   // (D68) a designed park, the PS1's five designs (src/lots.inc lot_park), mirrored half the time: the walk up from the gate, benches on the path side, trees in pairs, beds in rows, a bin and the flag at the gate
            int pm = (int)(h & 1), v = (int)((h >> 2) % 5u), tp = (int)((h >> 6) & 1);
            #define PKX(xx) (pm ? 12 - (xx) : (xx))
            #define PK(xx, yy, cc) z_put(PKX(xx), (yy), (cc), 0)
            #define PKF(xa, ya, xb, yb) for (int yy = (ya); yy <= (yb); yy++) for (int xx = (xa); xx <= (xb); xx++) flr[yy][PKX(xx)] = 2
            PKF(6, 0, 6, 12);
            if (v == 0) { PKF(0, 6, 12, 6); PKF(4, 4, 8, 8); PK(6, 6, '}'); PK(4, 4, 'N'); PK(8, 4, 'N'); PK(4, 8, 'N'); PK(8, 8, 'N');
                for (int q = 0; q < 4; q++) { int sx = (q & 1) ? 10 : 2, sy = (q & 2) ? 10 : 2; PK(sx, sy, 'p'); PK(sx - 1, sy, 'Z'); PK(sx + 1, sy, 'Z'); }
                PK(1, 5, '3'); PK(11, 5, '3'); PK(1, 7, '3'); PK(11, 7, '3'); }
            else if (v == 1) { PKF(0, 6, 12, 6); PK(3, 3, '}'); PK(1, 7, 'N'); PK(4, 7, 'N'); PK(6, 4, 'N'); PK(8, 1, '3'); PK(10, 1, '3'); PK(9, 2, '3'); PK(11, 3, '3'); PK(8, 4, '3'); PK(0, 1, '3'); PK(0, 3, '3');
                PKF(8, 8, 10, 10); PK(9, 9, 'K'); PK(11, 9, 'Y'); PK(11, 11, '3'); PK(1, 11, '3'); PK(2, 9, '3'); PK(1, 8, '3'); }
            else if (v == 2) { PKF(1, 2, 5, 9); PKF(7, 6, 11, 6); PK(3, 2, '_'); PK(3, 9, '_'); PK(7, 4, 'N'); PK(7, 8, 'N');
                for (int y = 1; y <= 11; y += 2) PK(11, y, '3'); PK(8, 1, '3'); PK(9, 1, '3'); PK(9, 9, 'K'); PK(10, 10, 'Y'); }
            else if (v == 3) { for (int r = 0; r < 3; r++) { int y = 2 + r * 3; for (int x = 1; x <= 4; x++) { PK(x, y, 'p'); PK(12 - x, y, 'p'); } }
                PK(6, 1, (char)0xE4); PK(5, 11, 'N'); PK(7, 11, 'N'); PK(0, 0, '3'); PK(12, 0, '3'); PK(0, 12, '3'); PK(12, 12, '3'); PK(0, 6, 'Z'); PK(12, 6, 'Z'); }
            else { PKF(0, 6, 12, 6); PK(5, 5, 'N'); PK(7, 5, 'N'); PK(5, 7, 'N'); PK(7, 7, 'N');
                PK(1, 1, '3'); PK(11, 1, '3'); PK(3, 2, '3'); PK(9, 2, '3'); PK(1, 11, '3'); PK(11, 11, '3'); PK(2, 9, '3'); PK(10, 9, '3'); PK(9, 4, 'K'); PK(3, 4, 'K'); if (tp) PK(6, 2, '}'); }
            PK(5, 12, 'Y');
            #undef PKX
            #undef PK
            #undef PKF
        }
        for (int i = 0; i < 14 && k != LOT_PARK && k != LOT_SQUARE; i++) { int x = (int)((h >> (i * 2)) & 15) % 13, y = (int)((h >> (i * 2 + 1)) % 11) % 13; if (y > 10 || obj_at(x, y) >= 0) continue;
            int pick = (int)((h >> (i + 3)) % 5);
            if (k == LOT_PARK || k == LOT_SQUARE) { if (pick == 0) PUTP(x, y, 0, 3, abs(x - 6) > abs(y - 6) ? 1 : 0);   // (D42, as the PS1: a bench lies along the walk by it, not at random)
                else if (pick == 1) PUTP(x, y, 6, 1, 0); else if (pick == 2) PUTP(x, y, 6, 3, 0); else if (pick == 3) PUTP(x, y, 6, 2, 0); else PUTP(x, y, 3, 2, 0); }
            else if (k == LOT_FARM) continue; else PUTP(x, y, pick & 1 ? 3 : 6, pick & 1 ? 2 : 2, 0); }
        if (k == LOT_SQUARE) PUTP(6, 6, 3, 3, 0);
        if (k == LOT_FARM) {   // (D38) the PS1's lot_farm: rows of crop beds, a barn with the hen coops and the cows, a fence along the front with a gate
            for (int y = 2; y <= 10; y += 2) for (int x = 1; x <= 6; x += 2) z_put(x, y, 0xDB, 0);
            z_room(8, 1, 13, 7, 1, 2); hwl[7][10] = 5; z_put(9, 2, 0xDC, 0); z_put(9, 4, 0xDC, 0); z_put(11, 2, 0xDD, 0); z_put(11, 4, 0xDD, 0); z_put(10, 6, 0xDE, 0);
            for (int x = 0; x < GN; x++) if (x != 7) hwl[12][x] = 6; }
        for (int q = 0; q < 3; q++) { spawn_x[q] = 5 + q; spawn_y[q] = 12; } return; }
    if (k == LOT_ARCADE) {   // (D46) the PS1's lot_arcade (world.c): a row of cabinets along the back, two down the side, the prize counter by the door
        int ax0 = 1, ay0 = 2, ax1 = 11, ay1 = 9, aw = 1 + (int)(h & 3);
        z_room(ax0, ay0, ax1, ay1, 2 + (int)(h >> 4) % 3, aw); hwl[ay1][6] = 5; door_cx = 6; door_cy = ay1;
        for (int x = ax0 + 1; x <= ax1 - 1; x++) z_put(x, ay0 + 1, '!', 0);
        z_put(ax0 + 1, ay0 + 3, '!', 0); z_put(ax0 + 1, ay0 + 4, '!', 0); z_put(ax1 - 1, ay0 + 3, 'j', 0); z_put(ax1 - 1, ay0 + 5, 'V', 0);
        z_put(ax0 + 4, ay0 + 4, '?', 0); z_put(ax0 + 1, ay1 - 1, 'C', 0); z_put(ax1 - 2, ay1 - 1, 'y', 0); z_put(ax1 - 1, ay1 - 1, 'c', 0); z_put(ax0 + 6, ay0 + 4, 'U', 0);
        for (int q = 0; q < 3; q++) { spawn_x[q] = 5 + q; spawn_y[q] = ay1 - 2; } return; }
    int x0 = 2, x1 = 11, y0 = 1, y1 = 9, ow = 1 + (int)(h & 3);
    for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) flr[y][x] = (u8)(2 + (int)(h >> 4) % 3);
    for (int x = x0; x < x1; x++) { hwl[y0][x] = (u8)ow; hwl[y1][x] = (u8)ow; } for (int y = y0; y < y1; y++) { vwl[y][x0] = (u8)ow; vwl[y][x1] = (u8)ow; }
    hwl[y1][6] = 5; door_cy = y1;
    int food = k == LOT_DINER || k == LOT_CAFE || k == LOT_RESTAURANT || k == LOT_BAR || k == LOT_INN || k == LOT_HOTEL, learn = k == LOT_SCHOOL || k == LOT_LIBRARY || k == LOT_OFFICE || k == LOT_TOWER || k == LOT_GALLERY, care = k == LOT_CLINIC || k == LOT_SPA || k == LOT_POOL;
    for (int x = x0 + 1; x < x1 - 1; x += 2) { if (food) PUTP(x, y0, 1, 3, 0); }   // counters / stacks along the back
    if (learn) { for (int x = x0 + 1; x < x1 - 1; x++) PUTP(x, y0, 5, 2, 0); for (int y = y0 + 3; y < y1 - 1; y += 2) for (int x = x0 + 1; x < x1 - 1; x += 3) { PUTP(x, y, 1, 1, 0); PUTP(x, y + 1, 0, 0, 0); } }
    else if (food) { for (int y = y0 + 3; y < y1 - 1; y += 2) for (int x = x0 + 1; x < x1 - 1; x += 3) { PUTP(x, y, 1, 0, 0); PUTP(x + 1, y, 0, 0, 3); } PUTP(x1 - 1, y0, 2, 3, 0); }
    else if (care) { for (int x = x0 + 1; x < x1 - 1; x += 2) PUTP(x, y0, 4, 0, 0); PUTP(x1 - 1, y0 + 2, 4, 3, 0); PUTP(x0 + 1, y0 + 4, 3, 2, 0); }
    else if (k == LOT_SHOP) {   // (D68) a shop as the PS1's lot_shop(): shelves along the back wall by what it sells, rows of stands either side of the aisle from the door (each joined to a side wall), the till and plants by the door
        static const char *const back[5] = { "F7", "i`", "vv[R", "SiCb", "$(" }; static const char rows[5] = { '7', 'i', 'y', 'i', 'A' };
        int kind = (int)(h % 5u), lay = (int)((h >> 5) & 1); const char *bk = back[kind]; int bn = (int)strlen(bk);
        for (int x = x0; x < x1; x++) z_put(x, y0, bk[(x - x0) % bn], 0);
        for (int side = 0; side < 2; side++) { int a = side ? 8 : x0, b = side ? x1 - 1 : 4; for (int x = a; x <= b; x++) { z_put(x, y0 + 2, rows[kind], 0); if (!lay) z_put(x, y0 + 4, rows[kind], 0); } }
        z_put(x1 - 1, y1 - 1, 'y', 0); z_put(x1 - 2, y1 - 1, 'y', 0); z_put(x1 - 3, y1 - 1, 'I', 0); z_put(x0, y1 - 1, 'p', 0); z_put(x0 + 1, y1 - 1, 'l', 0); }
    else { for (int x = x0 + 1; x < x1 - 1; x += 2) { PUTP(x, y0, 1, 3, 0); } for (int x = x0 + 2; x < x1 - 1; x += 3) PUTP(x, y0 + 3, 3, 1, 0); PUTP(x0 + 1, y0 + 4, 2, 0, 1); PUTP(x1 - 1, y0 + 4, 2, 3, 3); PUTP(x0 + 1, y1 - 1, 3, 2, 0); }
    if (k == LOT_SHOP && ((h >> 5) & 3) < 2) z_put(x1 - 2, y1 - 2, 0xBE, 0);   // (D53) an electronics shop sells the Boreagotchi: its display dock (tap it)
    for (int q = 0; q < 3; q++) { spawn_x[q] = 5 + q; spawn_y[q] = y1 - 2; }
    #undef PUTP
}
static void place_sims(void) { memset(slv, 0, sizeof slv); for (int i = 0; i < nsim; i++) { Sim *m = &sims[i]; int cx = i < 3 ? spawn_x[i] : door_cx + (i % 3) - 1, cy = i < 3 ? spawn_y[i] : door_cy + 1 + i / 3; if (cy > GN - 1) cy = GN - 1; m->x = m->tx = m->wx = cx * 16; m->y = m->ty = m->wy = cy * 16; m->act = -1; m->act_left = 0; } }
static void lot_reset(void) {
    memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); ups_clear(); memset(slv, 0, sizeof slv); ed_lv = 0; nobj = 0; occ_ok = 0; pick = -1; mode = M_LIVE;   /* (funds are the household's: set by new_household, not reset on every lot you walk into) */ cat = 0; tool = 0; pal = 1;
    gen_house();
#ifdef DS_LOT   /* test only: the shot rig shows lot kind DS_LOT (0 the starter house) with the seed DS_SEED */
    if (DS_LOT) { memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; gen_public(DS_LOT, 0x9E3779B9u * (u32)(DS_SEED)); }
#endif
    nsim = 3; ctl = 0; cur_sim = 0; memset(ilim, 0, sizeof ilim); strcpy(sname[0], "Ashley Newbie"); strcpy(sname[1], "Ronald Bacon"); strcpy(sname[2], "Kit Farlow");
    for (int i = 0; i < 3; i++) { Sim *m = &sims[i]; m->x = spawn_x[i] * 16; m->y = spawn_y[i] * 16; m->tx = m->x; m->ty = m->y; m->wx = m->x; m->wy = m->y; m->act = -1; m->act_left = 0; m->age = i == 0 ? 24 : i == 1 ? 31 : 9; m->soc = -1; m->tgt = 0;
        for (int n = 0; n < 6; n++) { m->need[n] = 90 - (n * 17 + i * 11) % 60; m->acc[n] = 0; } m->rel = 0; for (int q = 0; q < PMN; q++) m->pm[q] = 50; m->cc[0] = m->cc[1] = m->cc[2] = 0; if (i) { m->skin = (u8)(i * 2); m->hair = (u8)(i); m->hcol = (u8)(i * 2); m->shirt = (u8)(i + 2); m->hair = (u8)(6 + i * 2); m->beard = (u8)(i == 1 ? 3 : i == 2 ? BEARDV : 0); m->fc = (u8)i; } }
}

typedef struct { int bx, by, nobj, pcx, pcy, dcx, dcy; u8 flr[GN][GN], hwl[GN + 1][GN], vwl[GN][GN + 1]; Obj objs[400]; u8 roof, up, hasup; u8 ufl[UPN][GN][GN], uhw[UPN][GN + 1][GN], uvw[UPN][GN][GN + 1]; } LotSave;
static int top_lv(void); static int lv_box(int L, int *x0, int *y0, int *x1, int *y1);
static void ups_clear(void) { memset(uflr, 0, sizeof uflr); memset(uhwl, 0, sizeof uhwl); memset(uvwl, 0, sizeof uvwl); }
static void shell_to_floors(int up) {   // (D36) a lot saved before D36: its upper storeys were a shell over the house; now real floors with walls round them (and stairs to buy)
    int x0, y0, x1, y1; if (!lv_box(0, &x0, &y0, &x1, &y1)) return;
    for (int L = 1; L <= up && L <= UPN; L++) { for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) uflr[L - 1][y][x] = 2;
        for (int x = x0; x < x1; x++) { uhwl[L - 1][y0][x] = 1; uhwl[L - 1][y1][x] = 1; } for (int y = y0; y < y1; y++) { uvwl[L - 1][y][x0] = 1; uvwl[L - 1][y][x1] = 1; } }
}
#define LSV_N 48   // lots remembered (and saved)
static LotSave lsv[LSV_N]; static int lsv_n, lsv_next;
static void lot_save(void) { if (lot_bx < 0) return; int k = -1; for (int i = 0; i < lsv_n; i++) if (lsv[i].bx == lot_bx && lsv[i].by == lot_by) k = i; if (k < 0) { if (lsv_n < LSV_N) k = lsv_n++; else { for (int t = 0; t < LSV_N && lot_owned(lsv[lsv_next].bx, lsv[lsv_next].by); t++) lsv_next = (lsv_next + 1) % LSV_N; k = lsv_next; lsv_next = (lsv_next + 1) % LSV_N; } }
    LotSave *q = &lsv[k]; q->bx = lot_bx; q->by = lot_by; q->nobj = nobj; q->pcx = pcx; q->pcy = pcy; q->dcx = door_cx; q->dcy = door_cy; q->roof = (u8)roof_st; up_st = top_lv(); q->up = (u8)up_st; q->hasup = 1; memcpy(q->flr, flr, sizeof flr); memcpy(q->hwl, hwl, sizeof hwl); memcpy(q->vwl, vwl, sizeof vwl); memcpy(q->objs, objs, sizeof objs);
    memcpy(q->ufl, uflr, sizeof uflr); memcpy(q->uhw, uhwl, sizeof uhwl); memcpy(q->uvw, uvwl, sizeof uvwl); }
static void prop_trade(void) {   // Buy or Sell on the Map
    BInfo b; bm_block(sel_x, sel_y, &b);
    if (lot_owned(sel_x, sel_y)) {
        if (is_home(sel_x, sel_y)) say("Your home: move out first");
        else if (lot_bx == sel_x && lot_by == sel_y) say("You are on it: leave first");
        else if (nown < 2) say("Keep at least one lot");
        else { funds += lot_sell_price(&b); own_remove(sel_x, sel_y); for (int i = 0; i < lsv_n; i++) if (lsv[i].bx == sel_x && lsv[i].by == sel_y) { lsv[i] = lsv[--lsv_n]; if (lsv_next >= lsv_n) lsv_next = 0; break; } say("Sold! Cha-ching!"); }
    } else { int pr = lot_price(&b); if (!pr) say("Not for sale"); else if (pop_at(sel_x, sel_y, 0)) say("A family lives here"); else if (funds < pr) say("Your wallet says no!"); else if (nown >= OWN_MAX) say("Too many lots"); else { funds -= pr; own_add(sel_x, sel_y); say("Yours now! Home sweet lot"); } }
}
static void lot_enter(int bx, int by) {   // the lot you walk into: what you built there last time, or a new home from its own seed
    if (lot_bx >= 0 && (lot_bx != bx || lot_by != by)) life_ev(ctl, WN_VISIT, 0);
    lot_bx = bx; lot_by = by; lot_seed = (u32)(bx * 73856093 ^ by * 19349663); pick = -1; memset(slv, 0, sizeof slv); ed_lv = 0; int keep = LV; LV = 0;
    for (int i = 0; i < lsv_n; i++) if (lsv[i].bx == bx && lsv[i].by == by) { LotSave *q = &lsv[i]; nobj = q->nobj; pcx = q->pcx; pcy = q->pcy; door_cx = q->dcx; door_cy = q->dcy; roof_st = q->roof; up_st = q->up; memcpy(flr, q->flr, sizeof flr); memcpy(hwl, q->hwl, sizeof hwl); memcpy(vwl, q->vwl, sizeof vwl); memcpy(objs, q->objs, sizeof objs); occ_ok = 0;
        if (q->hasup) { memcpy(uflr, q->ufl, sizeof uflr); memcpy(uhwl, q->uhw, sizeof uhwl); memcpy(uvwl, q->uvw, sizeof uvwl); } else { ups_clear(); shell_to_floors(up_st); }
        nb_load(); LV = keep; return; }
    memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); ups_clear(); nobj = 0; occ_ok = 0; roof_st = up_st = 0;
    if (bx >= 2000) gen_zone(bx, by); else { BInfo bi; bm_block(bx, by, &bi);
#ifdef DS_LOT
 if (DS_LOT) { bi.lot = DS_LOT; bi.h = 0x9E3779B9u * (u32)(DS_SEED); }
#endif
 if (bi.lot >= LOT_HOUSE && bi.lot <= LOT_APART) { gen_two = bi.lot == LOT_TWO; gen_house(); gen_two = 0; } else gen_public(bi.lot, bi.h); } nb_load(); LV = keep;
}
static void lot_cross(void) {   // the sim you are playing left the lot's slot: the next lot becomes the middle one and everything slides back by one slot
    const Sim *m = &sims[ctl]; int dx = fdiv(m->x + 8, 16 * SLC), dy = fdiv(m->y + 8, 16 * SLC); if (!dx && !dy) return;
    int nx = lot_bx + dx, ny = lot_by + dy; if (zone) { BInfo zb_; zone_block(nx, ny, &zb_); if (zb_.terrain == BT_SEA) return; } else if (nx < 0 || ny < 0 || nx >= BM_N || ny >= BM_N) return;
    lot_save(); lot_enter(nx, ny);
    for (int i = 0; i < nsim; i++) { Sim *s2 = &sims[i]; int ax = dx * SLC * 16, ay = dy * SLC * 16; s2->x -= ax; s2->y -= ay; s2->tx -= ax; s2->ty -= ay; s2->wx -= ax; s2->wy -= ay; }
    { int ax = dx * SLC * 16, ay = dy * SLC * 16; dog.x -= ax; dog.y -= ay; dog.tx -= ax; dog.ty -= ay; dog.wx -= ax; dog.wy -= ay; }
    city_shift(dx, dy); camx -= dx * 36 * T; camz -= dy * 36 * T; mark_x -= dx * SLC; mark_y -= dy * SLC; }
static void bar(int x, int y, int w, int v, u16 c) { rect(x, y, w, 6, C15(4, 4, 6)); rect(x + 1, y + 1, (w - 2) * v / 100, 4, c); }
static void button(int x, int y, int w, int h, const char *t, u16 c, int on) {
    rect(x, y, w, h, on ? C15(31, 31, 20) : C15(2, 2, 4)); rect(x + 1, y + 1, w - 2, h - 2, on ? c : shade(c, 4));
    text_box(x, y, w, h, FS_BODY, on ? C15(2, 2, 4) : C15(31, 31, 31), AL_C, t);   // (D65) centred (it was 4 px from the left)
}
static int hit(int x, int y, int w, int h, int tx, int ty) { return tx >= x && tx < x + w && ty >= y && ty < y + h; }
static void plumbob(int x, int y) {   // (D65) the little green diamond: it marks a toast, a thought and a want
    static const u8 PB[5][5] = { { 0, 0, 2, 0, 0 }, { 0, 2, 1, 1, 0 }, { 2, 1, 1, 1, 3 }, { 0, 3, 1, 3, 0 }, { 0, 0, 3, 0, 0 } };
    static const u16 PC[4] = { 0, C15(10, 30, 10), C15(22, 31, 20), C15(3, 17, 6) };
    for (int j = 0; j < 5; j++) for (int i = 0; i < 5; i++) if (PB[j][i]) px(x + i, y + j, PC[PB[j][i]]);
}
static void toast(int cx, int y, const char *m) {   // (D65) a notification toast: a dark rim, a warm fill, a plumbob, the words centred in what is left; it keeps inside the screen
    int s = fit_font(FS_BODY, 226, m); if (!s) s = FS_SMALL; int w = tw(s, m) + 24; if (w > 252) w = 252; int x = cx - w / 2; if (x < 2) x = 2; if (x + w > 254) x = 254 - w;
    rect(x, y, w, 13, C15(2, 2, 4)); rect(x + 1, y + 1, w - 2, 11, C15(26, 10, 6)); rect(x + 1, y + 1, w - 2, 1, C15(30, 17, 11)); plumbob(x + 4, y + 4);
    text_box(x + 11, y, w - 12, 13, s, C15(31, 31, 31), AL_C, m);
}
static void text_fit(int x, int y, int s, int w, u16 c, const char *t) {   // (D35, D65) t in w pixels: the next smaller font first (BODY -> SMALL), then the letters closer, then cut short with a dot
    { int f = fit_font(s, w, t); if (f) { text(x, y, f, c, t); return; } while (FS_SMALLER[s]) s = FS_SMALLER[s]; }
    if (tw(s, t) <= w) { text(x, y, s, c, t); return; }
    t_tight = 1; if (tw(s, t) <= w) { text(x, y, s, c, t); t_tight = 0; return; }
    char b[40]; int n = (int)strlen(t); if (n > 38) n = 38; memcpy(b, t, (size_t)n); b[n] = 0;
    while (n > 1) { b[n - 1] = '.'; b[n] = 0; if (tw(s, b) <= w) break; n--; }
    text(x, y, s, c, b); t_tight = 0;
}
// ---- the UI kit (D70): one set of parts for every menu, so they look and answer alike ----
// Touch targets are at least 24 px; the back button is always at the bottom right (BK_*); the hints bar says which buttons do what; B = touch BACK.
#define BK_X 184
#define BK_Y 162
#define BK_W 64
#define BK_H 24
enum { UI_MOVE = 7, UI_OK = 8, UI_BACK = 9 };   // sfx numbers: a soft tick for moving, two notes up for a choice, two notes down for back
static void ui_snd(int n) { sfx(n); }
static void ui_back_btn(void) { rect(BK_X, BK_Y, BK_W, BK_H, C15(2, 2, 4)); rect(BK_X + 1, BK_Y + 1, BK_W - 2, BK_H - 2, C15(24, 8, 8)); rect(BK_X + 1, BK_Y + 1, BK_W - 2, 1, C15(30, 16, 14)); text_box(BK_X, BK_Y, BK_W, BK_H, FS_HEAD, C15(31, 31, 31), AL_C, "Back"); }
static int ui_back_hit(int tx, int ty) { return tx >= BK_X - 4 && tx < BK_X + BK_W + 4 && ty >= BK_Y - 4 && ty < BK_Y + BK_H + 4; }
static void ui_button(int x, int y, int w, int h, const char *t, int style) {   // style 0 blue, 1 green (on), 2 red, 3 grey (cannot)
    static const u16 UBF[4] = { C15(6, 12, 26), C15(8, 22, 8), C15(24, 8, 8), C15(10, 12, 15) }, UBT[4] = { C15(14, 20, 31), C15(18, 29, 14), C15(30, 16, 14), C15(15, 17, 20) };
    rect(x, y, w, h, C15(2, 2, 4)); rect(x + 1, y + 1, w - 2, h - 2, UBF[style]); rect(x + 1, y + 1, w - 2, 1, UBT[style]);
    text_box(x, y, w, h, h > 17 ? FS_HEAD : FS_BODY, style == 3 ? C15(22, 24, 27) : C15(31, 31, 31), AL_C, t);
}
static void ui_focus(int x, int y, int w, int h) { u16 c = C15(31, 31, 8); rect(x - 1, y - 1, w + 2, 2, c); rect(x - 1, y + h - 1, w + 2, 2, c); rect(x - 1, y - 1, 2, h + 2, c); rect(x + w - 1, y - 1, 2, h + 2, c); }   // the yellow ring round the chosen thing (d-pad)
// the hints bar: spec "KEY:Label|KEY:Label": a little key cap and a word; it is skipped when Options > Controls > Hints bar is Off
static void ui_hints(int y, const char *spec) {
    if (!o_hint) return;
    rect(0, y, 256, 13, C15(2, 3, 8)); rect(0, y, 256, 1, C15(10, 14, 22));
    char key[8][8], lab[8][16]; int n = 0; const char *p = spec;
    while (*p && n < 8) { int k = 0, l = 0; while (*p && *p != ':' && k < 7) key[n][k++] = *p++; key[n][k] = 0; if (*p == ':') p++; while (*p && *p != '|' && l < 15) lab[n][l++] = *p++; lab[n][l] = 0; if (*p == '|') p++; n++; }
    int tot = 0; for (int i = 0; i < n; i++) tot += tw(FS_SMALL, key[i]) + 7 + 3 + tw(FS_SMALL, lab[i]) + 8; tot -= 8;
    int x = (256 - tot) / 2; if (x < 4) x = 4;
    for (int i = 0; i < n; i++) { int kw = tw(FS_SMALL, key[i]) + 7; rect(x, y + 2, kw, 9, C15(24, 27, 31)); rect(x + 1, y + 3, kw - 2, 7, C15(14, 18, 26)); text_box(x, y + 2, kw, 9, FS_SMALL, C15(31, 31, 31), AL_C, key[i]);
        x += kw + 3; text_box(x - 1, y, tw(FS_SMALL, lab[i]) + 8, 13, FS_SMALL, C15(25, 28, 31), AL_L, lab[i]); x += tw(FS_SMALL, lab[i]) + 8; }
}
// wrapped text: breaks at spaces so every line fits w; returns the lines drawn (at most maxl); the line height is lh
static int ui_wrap(int x, int y, int w, int lh, int maxl, int s, u16 c, const char *t) {
    int n = 0; char ln[64];
    while (*t && n < maxl) { int len = 0, best = 0; while (t[len] && len < 62) { ln[len] = t[len]; ln[len + 1] = 0; if (t[len] == ' ') best = len; if (tw(s, ln) > w) break; len++; }
        if (!t[len]) best = len; else { if (best <= 0) best = len ? len : 1; }
        memcpy(ln, t, (size_t)best); ln[best] = 0; text(x, y + n * lh, s, c, ln); n++; t += best; while (*t == ' ') t++; }
    return n;
}
// held keys that act now: the first press at once, then a pause, then repeats that speed up (every 8, 4, 2, 1 vblanks); o_rep scales it. d = the four d-pad keys
static u32 rep_next[4], rep_cnt[4];
static int ui_rep(int held, int down, u32 now) {
    static const u32 K[4] = { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT }; int out = 0; int sc = o_rep == 0 ? 3 : o_rep == 1 ? 2 : 1;   // Slow, Normal, Fast: the pause and the step in vblanks / 2
    for (int i = 0; i < 4; i++) {
        if (down & (int)K[i]) { out |= (int)K[i]; rep_cnt[i] = 0; rep_next[i] = now + (u32)(8 * sc + 4); }
        else if (held & (int)K[i]) { if ((int)(now - rep_next[i]) >= 0) { out |= (int)K[i]; rep_cnt[i]++; int iv = rep_cnt[i] < 4 ? 8 : rep_cnt[i] < 10 ? 4 : rep_cnt[i] < 24 ? 2 : 1; iv = (iv * sc + 1) / 2; if (iv < 1) iv = 1; rep_next[i] = now + (u32)iv; } } }
    return out;
}
// ---- the shop (D35): the PS1's ten tabs, six cards a page, cheapest first, as the PS1 lists them ----
static u8 tab_id[NTAB][64]; static int tab_n[NTAB], bpage;
static void tabs_init(void) {
    for (int t = 0; t < NTAB; t++) tab_n[t] = 0;
    for (int i = 0; i < NITEM; i++) { int t = ITEMS[i].tab; if (t >= NTAB || tab_n[t] >= 64) continue; int k = tab_n[t]++;
        while (k > 0 && ITEMS[tab_id[t][k - 1]].price > ITEMS[i].price) { tab_id[t][k] = tab_id[t][k - 1]; k--; } tab_id[t][k] = (u8)i; }
}
static int tab_pages(void) { int p = (tab_n[cat] + 5) / 6; return p < 1 ? 1 : p; }
// a little picture of an item for the shop: its own boxes (the 3D model's), seen from the front and a little above
#define PVMAX 48
typedef struct { s16 x0, z0, x1, z1, h0, h1; u16 c, t; } PBox;
static PBox pv[PVMAX]; static int npv, pv_cap;
static void obj3d(int x, int y, const Item *it, int rot, int ghost);
static void draw_mini(int x, int y, int w, int h, int id) {
    const Item *it = &ITEMS[id]; pv_cap = 1; npv = 0; obj3d(0, 0, it, 0, 0); pv_cap = 0; if (!npv) return;
    int L = it->fl * 80, u0 = 1 << 30, u1 = -(1 << 30), v0 = 1 << 30, v1 = -(1 << 30), ord_[PVMAX];
    for (int k = 0; k < npv; k++) { const PBox *b = &pv[k];   // (across and depth as the 3D lot draws them, 68 a unit; depth at half; height 82 a unit)
        int xa = b->x0 * 68 / 10, xb = b->x1 * 68 / 10, df = (L - b->z1) * 34 / 10, db = (L - b->z0) * 34 / 10, ha = b->h0 * 82 / 10, hb = b->h1 * 82 / 10;
        if (xa + df < u0) u0 = xa + df; if (xb + db > u1) u1 = xb + db; if (ha + df < v0) v0 = ha + df; if (hb + db > v1) v1 = hb + db;
        int j = k; while (j > 0 && (pv[ord_[j - 1]].z1 > b->z1 || (pv[ord_[j - 1]].z1 == b->z1 && pv[ord_[j - 1]].h0 > b->h0))) { ord_[j] = ord_[j - 1]; j--; } ord_[j] = k; }   // back to front
    int du = u1 - u0 < 1 ? 1 : u1 - u0, dv = v1 - v0 < 1 ? 1 : v1 - v0, sc = w * 256 / du;
    if (h * 256 / dv < sc) sc = h * 256 / dv; if (sc > h * 256 / 700) sc = h * 256 / 700;   // (a phone stays small; a bed fills the frame)
    int ox = x + (w - du * sc / 256) / 2 - u0 * sc / 256, oy = y + h - (h - dv * sc / 256) / 2 + v0 * sc / 256;
    for (int q = 0; q < npv; q++) { const PBox *b = &pv[ord_[q]];
        int df = (L - b->z1) * 34 / 10, db = (L - b->z0) * 34 / 10, l = ox + (b->x0 * 68 / 10 + df) * sc / 256, r = ox + (b->x1 * 68 / 10 + df) * sc / 256;
        int bt = oy - (b->h0 * 82 / 10 + df) * sc / 256, tp = oy - (b->h1 * 82 / 10 + df) * sc / 256, dpx = (db - df) * sc / 256;
        if (r <= l) r = l + 1; if (bt <= tp) bt = tp + 1; u16 dk = shade(b->c, 6);
        for (int t = 0; t < dpx; t++) rect(r + t, tp - t, 1, bt - tp, dk);       // the right side
        for (int t = 0; t < dpx; t++) rect(l + t + 1, tp - t - 1, r - l, 1, b->t);  // the top
        rect(l, tp, r - l, bt - tp, b->c); }                                     // the front
}
static void age_txt(char *t, int mk) { int lo = 0, hi = 5; while (lo < 5 && !((mk >> lo) & 1)) lo++; while (hi > 0 && !((mk >> hi) & 1)) hi--; if (lo == 0 && hi == 5) strcpy(t, "Anyone"); else sprintf(t, "%.3s-%.3s", STGN[lo], STGN[hi]); }
#define PX 2                  // the plan view: 13 cells of 12 pixels
#define PY 22
static int stairs_below(int L, int x, int y) { if (!L) return -1; int keep = LV; LV = L - 1; int o = obj_at(x, y); LV = keep; return o >= 0 && OBJ_ID(&objs[o]) == ID_STAIRS ? o : -1; }   // (D36) the flight that rises into this cell from the storey below
static void plan(int ghost) {
    u8 (*fl)[GN] = FLv(LV); u8 (*hw)[GN] = HWv(LV); u8 (*vw)[GN + 1] = VWv(LV);
    for (int y = 0; y < GN; y++) for (int x = 0; x < GN; x++) {
        rect(PX + x * 12, PY + y * 12, 12, 12, fl[y][x] ? FLOORC[fl[y][x] - 1] : LV ? (((x + y) & 1) ? C15(4, 6, 10) : C15(5, 7, 12)) : (((x + y) & 1) ? C15(9, 18, 7) : C15(11, 20, 8)));
        if (LV && !fl[y][x] && stairs_below(LV, x, y) >= 0) { rect(PX + x * 12 + 2, PY + y * 12 + 2, 8, 8, C15(24, 18, 11)); rect(PX + x * 12 + 4, PY + y * 12 + 4, 4, 4, C15(4, 6, 10)); }   // the stairwell
        rect(PX + x * 12, PY + y * 12, 12, 1, C15(0, 0, 0) | 0); }
    for (int y = 0; y <= GN; y++) for (int x = 0; x < GN; x++) if (hw[y][x]) rect(PX + x * 12, PY + y * 12 - 1, 13, 3, WALLC[hw[y][x] - 1]);
    for (int y = 0; y < GN; y++) for (int x = 0; x <= GN; x++) if (vw[y][x]) rect(PX + x * 12 - 1, PY + y * 12, 3, 13, WALLC[vw[y][x] - 1]);
    for (int i = 0; i < nobj; i++) { if (OBJ_LV(&objs[i]) != LV) continue; int w, h; obj_fp(&objs[i], &w, &h); rect(PX + objs[i].x * 12 + 2, PY + objs[i].y * 12 + 2, w * 12 - 4, h * 12 - 4, IT_OF(&objs[i])->col); }
    if (ghost) { int w = 1, h = 1; if (mode == M_BUY && pick >= 0) { const Item *it = &ITEMS[pick]; w = prot & 1 ? it->fl : it->fw; h = prot & 1 ? it->fw : it->fl; }
        int X = PX + pcx * 12, Y = PY + pcy * 12, gw = w * 12, gh = h * 12; rect(X, Y, gw, 1, 0xFFFF); rect(X, Y + gh - 1, gw, 1, 0xFFFF); rect(X, Y, 1, gh, 0xFFFF); rect(X + gw - 1, Y, 1, gh, 0xFFFF); }
}
static int drag, drag_card, dsx, dsy, drag_item, wall_val, move_obj = -1, ui_ver, last_tx, last_ty;
static const u16 BG_L = C15(18, 25, 31), PAN = C15(8, 16, 28), PANB = C15(2, 7, 20), PANH = C15(5, 12, 26), TXT = C15(31, 31, 31);
static void panel(int x, int y, int w, int h, const char *title); static void backdrop(void); static void backdrop_y(int y0, int y1);
static void cat_icon(int x, int y, int i, u16 c) {   // a 12x12 pictogram for each shop tab
    switch (i) {
        case 0: rect(x + 2, y, 3, 9, c); rect(x + 2, y + 5, 9, 3, c); rect(x + 2, y + 8, 2, 4, c); rect(x + 9, y + 8, 2, 4, c); break;   // a chair
        case 1: rect(x, y + 3, 12, 3, c); rect(x + 1, y + 6, 2, 6, c); rect(x + 9, y + 6, 2, 6, c); break;                              // a table
        case 2: rect(x, y + 1, 12, 8, c); rect(x + 1, y + 2, 10, 6, C15(4, 8, 18)); rect(x + 5, y + 9, 2, 2, c); rect(x + 3, y + 11, 6, 1, c); break;   // a screen
        case 3: rect(x + 4, y + 8, 4, 4, c); rect(x + 3, y + 1, 6, 3, C15(8, 24, 8)); rect(x + 1, y + 3, 10, 3, C15(8, 24, 8)); rect(x + 5, y + 5, 2, 3, C15(8, 24, 8)); break;   // a plant
        case 7: rect(x + 1, y + 3, 10, 8, c); rect(x + 2, y + 1, 3, 2, c); rect(x + 7, y + 1, 3, 2, c); rect(x + 3, y + 6, 6, 3, C15(4, 4, 6)); break;   // a stove
        case 14: rect(x, y + 6, 12, 3, c); rect(x, y + 5, 2, 1, c); rect(x + 10, y + 5, 2, 1, c); rect(x + 2, y + 9, 2, 2, C15(4, 4, 6)); rect(x + 8, y + 9, 2, 2, C15(4, 4, 6)); break;   // a skateboard
        case 15: rect(x + 3, y, 6, 12, c); rect(x + 2, y + 2, 8, 9, c); rect(x + 4, y + 3, 4, 3, C15(10, 22, 12)); rect(x + 4, y + 8, 1, 1, C15(4, 8, 18)); rect(x + 7, y + 8, 1, 1, C15(4, 8, 18)); break;   // the toy egg
        case 16: rect(x + 3, y, 6, 7, C15(31, 10, 14)); rect(x + 2, y + 1, 8, 5, C15(31, 10, 14)); rect(x + 4, y + 1, 2, 2, C15(31, 24, 26)); rect(x + 5, y + 7, 1, 5, c); break;   // a balloon
        case 17: rect(x + 5, y, 2, 12, C15(31, 26, 6)); rect(x, y + 4, 12, 2, C15(31, 26, 6)); rect(x + 2, y + 2, 8, 7, C15(31, 26, 6)); rect(x + 3, y + 9, 2, 3, C15(31, 26, 6)); rect(x + 7, y + 9, 2, 3, C15(31, 26, 6)); break;   // a star
        default: rect(x + 5, y + 7, 2, 5, C15(14, 8, 4)); rect(x + 1, y, 10, 4, C15(6, 20, 6)); rect(x + 2, y + 3, 8, 5, C15(8, 24, 8)); break;   // a tree
    }
}
static const u8 TABICON[NTAB] = { 0, 1, 7, 2, 3, 13, 14, 15, 16, 17 };
static void coin(int x, int y) { rect(x + 1, y, 5, 7, C15(31, 25, 4)); rect(x, y + 1, 7, 5, C15(31, 25, 4)); rect(x + 2, y + 1, 3, 5, C15(31, 30, 12)); }
static void buy_card_pos(int k, int *x, int *y) { *x = 60 + (k & 1) * 98; *y = 33 + (k >> 1) * 44; }
static void draw_ui(void) {
    backdrop();
    const char *tn[4] = { "Map", "Live", "Buy", "Build" };
    rect(0, 0, 256, 18, C15(2, 4, 12));
    for (int i = 0; i < 4; i++) chip(i * 52 + 1, 1, 50, 15, tn[i], mode == i);
    char t[40];
    if (mode == M_BUY || mode == M_BUILD) { sprintf(t, "Floor %d", ed_lv + 1); chip(210, 1, 45, 15, t, ed_lv > 0); }   // (D36) the storey Buy and Build work on
    rect(0, 172, 160, 20, C15(2, 4, 12)); rect(1, 173, 158, 18, C15(5, 12, 26)); coin(5, 178); sprintf(t, "%d", funds); text_box(16, 173, 68, 18, FS_NUM, C15(31, 31, 20), AL_L, t);   // (D65: money and the clock in the NUMBER font, centred in the bar)
    clk(t, minutes % 1440); text_box(84, 173, 74, 18, FS_NUM, C15(24, 28, 31), AL_R, t);
    if (msg_t > 0) toast(120, 158, msg);
    { const char *cc[7] = { "<", ">", "^", "v", "+", "-", "Rst" }; for (int k = 0; k < 7; k++) { int cx = 164 + (k < 4 ? k * 22 : (k - 4) * 29), cy = k < 4 ? 164 : 178; rect(cx, cy, k < 4 ? 20 : 27, 12, C15(2, 4, 12)); rect(cx + 1, cy + 1, k < 4 ? 18 : 25, 10, C15(8, 16, 28)); text_box(cx, cy, k < 4 ? 20 : 27, 12, FS_BODY, TXT, AL_C, cc[k]); } }
    if (mode == M_BUY && pick < 0) {
        for (int k = 0; k < NTAB; k++) { int on = cat == k, y = 20 + k * 14; rect(2, y, 56, 14, on ? C15(31, 31, 31) : C15(2, 4, 12)); rect(3, y + 1, 54, 12, on ? C15(8, 26, 8) : C15(10, 14, 24)); rect(3, y + 1, 54, 1, on ? C15(20, 31, 20) : C15(16, 20, 30));
            cat_icon(5, y + 1, TABICON[k], on ? C15(31, 31, 31) : C15(24, 28, 31)); text_box(19, y + 1, 39, 12, FS_BODY, on ? C15(2, 6, 2) : TXT, AL_L, TABS[k]); }
        int pages = tab_pages(); if (bpage >= pages) bpage = 0;
        rect(60, 19, 142, 12, C15(2, 4, 12)); text_box(61, 19, 108, 12, FS_HEAD, TXT, AL_L, TABN[cat]); sprintf(t, "%d/%d", bpage + 1, pages); text_box(150, 19, 51, 12, FS_NUM, C15(31, 31, 20), AL_R, t);
        chip(204, 19, 24, 12, "<", 0); chip(230, 19, 24, 12, ">", 0);
        for (int k = 0; k < 6; k++) { int q = bpage * 6 + k; if (q >= tab_n[cat]) break; int id = tab_id[cat][q]; const Item *it = &ITEMS[id]; int x, y; buy_card_pos(k, &x, &y);
            int can = funds >= it->price, lift = drag == 1 && drag_item == id;
            rect(x, y, 96, 42, lift ? C15(31, 31, 20) : C15(2, 4, 12)); rect(x + 1, y + 1, 94, 40, can ? C15(28, 31, 31) : C15(18, 20, 24));
            rect(x + 1, y + 1, 94, 11, can ? C15(8, 18, 31) : C15(10, 12, 18)); text_box(x + 1, y + 1, 94, 11, FS_HEAD, TXT, AL_L, it->name);
            rect(x + 3, y + 14, 32, 25, C15(20, 26, 31)); rect(x + 6, y + 34, 26, 3, C15(14, 20, 27)); draw_mini(x + 4, y + 15, 30, 22, id);   // a little stage with a shadow
            rect(x + 38, y + 14, 55, 13, can ? C15(4, 18, 6) : C15(22, 6, 6)); rect(x + 39, y + 15, 53, 11, can ? C15(8, 24, 8) : C15(28, 10, 10)); coin(x + 41, y + 17); sprintf(t, "%d", it->price); text_box(x + 50, y + 14, 42, 13, FS_NUM, TXT, AL_L, t);
            if (it->fw * it->fl > 1) { sprintf(t, "%dx%d", it->fw, it->fl); text_box(x + 36, y + 28, 22, 12, FS_SMALL, C15(8, 12, 22), AL_L, t); age_txt(t, it->mask); text_box(x + 58, y + 28, 36, 12, FS_SMALL, C15(8, 12, 22), AL_L, t); }
            else { age_txt(t, it->mask); text_box(x + 36, y + 28, 58, 12, FS_SMALL, can ? C15(8, 12, 22) : C15(22, 6, 6), AL_L, can ? t : "Too dear"); } }
    } else if (mode == M_BUY) {
        plan(1); const Item *it = &ITEMS[pick];
        rect(160, 20, 94, 140, C15(2, 4, 12)); rect(161, 21, 92, 138, C15(28, 31, 31)); rect(161, 21, 92, 14, C15(8, 18, 31)); text_box(161, 21, 92, 14, FS_HEAD, TXT, AL_L, it->name);
        rect(165, 38, 84, 42, C15(20, 26, 31)); rect(170, 72, 74, 4, C15(14, 20, 27)); draw_mini(167, 40, 80, 36, pick);
        coin(166, 83); sprintf(t, "%d", it->price); text_box(176, 80, 46, 12, FS_NUM, C15(8, 12, 22), AL_L, t); if (it->fw * it->fl > 1) { sprintf(t, "%dx%d", prot & 1 ? it->fl : it->fw, prot & 1 ? it->fw : it->fl); text_box(210, 80, 42, 12, FS_SMALL, C15(8, 12, 22), AL_R, t); }
        chip(166, 96, 82, 20, "Place", 1); chip(166, 118, 82, 20, "Turn", 0); chip(166, 140, 82, 18, "Back", 0);
    } else if (mode == M_BUILD) {
        plan(1);
        static char rl[16]; static const char *const RN[4] = { "Auto", "Flat", "Gable", "Hip" }; sprintf(rl, "Roof: %s", RN[roof_st]);
        const char *tl2[6] = { "Floor", "Wall", "Erase", "Move", "Sell", rl };
        for (int i = 0; i < 6; i++) button(164, 22 + i * 14, 88, 13, tl2[i], C15(24, 14, 8), tool == i);
        if (ed_lv) { sprintf(t, "Floor %d of %d", ed_lv + 1, UPN + 1); text_box(162, 106, 92, 12, FS_BODY, C15(8, 12, 22), AL_L, t); }
        int n = tool == 0 ? 5 : tool == 1 ? 6 : 0;
        for (int i = 0; i < n; i++) { rect(164 + i * 15, 124, 14, 15, pal == i + 1 ? 0xFFFF : C15(2, 2, 4)); rect(166 + i * 15, 126, 10, 11, tool == 0 ? FLOORC[i] : WALLC[i]); }
        text_box(162, 141, 92, 11, FS_BODY, C15(20, 24, 28), AL_L, tool == 1 ? "Drag along edges" : tool == 3 ? "Drag furniture" : tool == 4 ? "Tap furniture" : tool == 5 ? "Tap to change" : "Drag to paint");
        if (tool == 5) text_box(162, 152, 92, 11, FS_BODY, C15(20, 24, 28), AL_L, "See Options>Roofs"); else if (ed_lv) text_box(162, 152, 92, 11, FS_BODY, C15(20, 24, 28), AL_L, "Stairs: Buy>Special");
    }
}
static int lot_touch(int tx, int ty) {   // a stylus tap on the plan: the cell (or -1)
    if (tx < PX || ty < PY || tx >= PX + GN * 12 || ty >= PY + GN * 12) return -1;
    return ((ty - PY) / 12) * GN + (tx - PX) / 12;
}
static u8 *edge_at(int tx, int ty) {   // the wall slot nearest the stylus
    int c = lot_touch(tx, ty); if (c < 0) return 0; int x = c % GN, y = c / GN, fx = (tx - PX) % 12, fy = (ty - PY) % 12, d[4] = { fy, 11 - fy, fx, 11 - fx }, b = 0;
    for (int k = 1; k < 4; k++) if (d[k] < d[b]) b = k;
    u8 (*hw)[GN] = HWv(LV); u8 (*vw)[GN + 1] = VWv(LV); return b == 0 ? &hw[y][x] : b == 1 ? &hw[y + 1][x] : b == 2 ? &vw[y][x] : &vw[y][x + 1];
}
static int fp_free(int x, int y, int w, int h, int skip) {   // (D35) every cell of a footprint inside the plot, and nothing else on it
    if (x < 0 || y < 0 || x + w > GN || y + h > GN) return 0;
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) { int o = obj_at(i, j); if ((o >= 0 && o != skip) || (LV && !uflr[LV - 1][j][i])) return 0; }   // (upstairs: on the floor only)
    return 1;
}
static void place_here(void) {
    const Item *it = &ITEMS[pick];
    if (pick == 48) { if (dog_have) say("You already have a dog"); else if (funds < it->price) say("Your wallet says no!"); else { funds -= it->price; dog_have = 1; dog_reset(); life_ver++; ui_ver++; say("Welcome home, Rex! Woof!"); life_ev(ctl, WN_BUY, 0); } pick = -1; return; }   // (the puppy is the dog, not a thing)
    int w = prot & 1 ? it->fl : it->fw, h = prot & 1 ? it->fw : it->fl;
    if (pcx + w > GN || pcy + h > GN) { say("Not enough room"); return; }
    if (LV) for (int j = pcy; j < pcy + h; j++) for (int i = pcx; i < pcx + w; i++) if (!uflr[LV - 1][j][i]) { say("Build a floor there first"); return; }
    if (pick == ID_STAIRS && LV >= UPN) { say("No storey above the top one"); return; }   // (the PS1's words)
    if (!fp_free(pcx, pcy, w, h, -1)) { say("Something is there"); return; }
    if (nobj >= 400) { say("Lot is full"); return; }
    if (funds < it->price) { say("Your wallet says no!"); pick = -1; return; }
    funds -= it->price; objs[nobj].x = (u8)pcx; objs[nobj].y = (u8)pcy; objs[nobj].c = (u8)(pick >> 2); objs[nobj].i = (u8)(pick & 3); objs[nobj].rot = (u8)(prot | LV << 2); nobj++; occ_ok = 0; say("Bought! Nice choice"); ui_ver++; life_ev(ctl, WN_BUY, 0);
    if (pick == ID_STAIRS) { uflr[LV][pcy][pcx] = 0; say("Stairs: build a floor above, behind them"); }   // the cell over the flight becomes its stairwell
}
static int floor_ok(int L, int x, int y) {   // (D36) an upper floor needs something under it, or a floor beside it on its own storey (the PS1's rule); a stairwell stays open
    if (!L) return 1; if (stairs_below(L, x, y) >= 0) return 0; if (L == 1 || FLv(L - 1)[y][x]) return 1;
    u8 (*fl)[GN] = FLv(L); return (x > 0 && fl[y][x - 1]) || (x < GN - 1 && fl[y][x + 1]) || (y > 0 && fl[y - 1][x]) || (y < GN - 1 && fl[y + 1][x]);
}
static void build_apply(int c) {   // paint or erase one cell (of the storey being worked on)
    int x = c % GN, y = c / GN; pcx = x; pcy = y; u8 (*fl)[GN] = FLv(LV); u8 (*hw)[GN] = HWv(LV); u8 (*vw)[GN + 1] = VWv(LV);
    if (tool == 0 && fl[y][x] != pal && floor_ok(LV, x, y)) { fl[y][x] = (u8)pal; ui_ver++; }
    else if (tool == 2) { if (LV && obj_at(x, y) >= 0) return; if (LV) fl[y][x] = 0; else flr[y][x] = 0; hw[y][x] = 0; hw[y + 1][x] = 0; vw[y][x] = 0; vw[y][x + 1] = 0; ui_ver++; }   // (upstairs the floor itself goes: open air)
}
static void ui_down(int tx, int ty) {
    last_tx = tx; last_ty = ty; drag = 0; drag_card = 0;
    for (int i = 0; i < 4; i++) if (hit(i * 52, 0, 50, 16, tx, ty)) { set_mode(i); pick = -1; return; }
    if (hit(210, 0, 46, 16, tx, ty)) { ed_lv = (ed_lv + 1) % (UPN + 1); LV = ed_lv; ui_ver++; return; }
    if (mode == M_BUY && pick < 0) {
        for (int k = 0; k < NTAB; k++) if (hit(2, 20 + k * 14, 56, 14, tx, ty)) { cat = k; bpage = 0; ui_ver++; }
        { int pages = tab_pages(); if (hit(204, 19, 24, 12, tx, ty)) { bpage = (bpage + pages - 1) % pages; ui_ver++; } if (hit(230, 19, 24, 12, tx, ty)) { bpage = (bpage + 1) % pages; ui_ver++; } }
        for (int k = 0; k < 6; k++) { int q = bpage * 6 + k, x, y; if (q >= tab_n[cat]) break; int id = tab_id[cat][q]; buy_card_pos(k, &x, &y);
            if (hit(x, y, 96, 42, tx, ty)) { if (funds >= ITEMS[id].price) { drag = 1; drag_item = id; dsx = tx; dsy = ty; } else say("Your wallet says no!"); } }
    } else if (mode == M_BUY) {
        int c = lot_touch(tx, ty);
        if (c >= 0) { pcx = c % GN; pcy = c / GN; drag = 2; return; }
        if (hit(166, 96, 82, 20, tx, ty)) place_here();
        if (hit(166, 118, 82, 20, tx, ty)) prot = (prot + 1) & 3;
        if (hit(166, 140, 82, 18, tx, ty)) pick = -1;
    } else if (mode == M_BUILD) {
        for (int i = 0; i < 6; i++) if (hit(164, 22 + i * 14, 88, 13, tx, ty)) { tool = i; if (pal < 1) pal = 1; if (tool == 1 && pal > 6) pal = 6; if (i == 5) roof_st = (roof_st + 1) % 4; ui_ver++; }
        int n = tool == 0 ? 5 : tool == 1 ? 6 : 0;
        for (int i = 0; i < n; i++) if (hit(164 + i * 15, 124, 14, 15, tx, ty)) pal = i + 1;
        int c = lot_touch(tx, ty);
        if (c >= 0) { int x = c % GN, y = c / GN; pcx = x; pcy = y;
            if (tool == 0 || tool == 2) { drag = 3; build_apply(c); }
            else if (tool == 1) { u8 *e = edge_at(tx, ty); if (e) { wall_val = *e ? 0 : pal; *e = (u8)wall_val; drag = 4; ui_ver++; } }
            else if (tool == 3) { int o = obj_at(x, y); if (o >= 0) { if (OBJ_ID(&objs[o]) == ID_STAIRS) say("Stairs stay put"); else { drag = 5; move_obj = o; } } }   // (the PS1 cannot pick stairs up either)
            else if (tool == 4) { int o = obj_at(x, y); if (o >= 0) { int sx = objs[o].x, sy = objs[o].y, st = OBJ_ID(&objs[o]) == ID_STAIRS; funds += IT_OF(&objs[o])->price / 2; objs[o] = objs[--nobj]; occ_ok = 0; say("Sold! Cha-ching!"); ui_ver++;
                if (st && LV < UPN) { u8 (*up)[GN] = FLv(LV + 1); int f = sx > 0 ? up[sy][sx - 1] : 0; if (!f && sx < GN - 1) f = up[sy][sx + 1]; if (!f && sy > 0) f = up[sy - 1][sx]; if (!f && sy < GN - 1) f = up[sy + 1][sx]; up[sy][sx] = (u8)f; } } } }   // (selling stairs closes their stairwell, if there is floor round it)
    }
}
static void ui_hold(int tx, int ty) {
    last_tx = tx; last_ty = ty;
    if (drag == 1) { int dx = tx - dsx, dy = ty - dsy; if (dx < 0) dx = -dx; if (dy < 0) dy = -dy; if (dx + dy > 6) { pick = drag_item; prot = 0; drag = 2; drag_card = 1; ui_ver++; } }
    else if (drag == 2) { int c = lot_touch(tx, ty); if (c >= 0 && (c % GN != pcx || c / GN != pcy)) { pcx = c % GN; pcy = c / GN; } }
    else if (drag == 3) { int c = lot_touch(tx, ty); if (c >= 0) build_apply(c); }
    else if (drag == 4) { u8 *e = edge_at(tx, ty); if (e && *e != wall_val) { *e = (u8)wall_val; ui_ver++; } }
    else if (drag == 5) { int c = lot_touch(tx, ty), w, h; if (c >= 0 && move_obj >= 0) { obj_fp(&objs[move_obj], &w, &h); if ((c % GN != objs[move_obj].x || c / GN != objs[move_obj].y) && fp_free(c % GN, c / GN, w, h, move_obj)) { objs[move_obj].x = (u8)(c % GN); objs[move_obj].y = (u8)(c / GN); occ_ok = 0; pcx = c % GN; pcy = c / GN; ui_ver++; } } }
}
static void ui_up(void) {
    if (drag == 1) { pick = drag_item; prot = 0; ui_ver++; }                                   // a tap on a card: pick it, then drag on the plan
    else if (drag == 2) { if (lot_touch(last_tx, last_ty) >= 0) place_here(); else if (drag_card) { pick = -1; ui_ver++; } }
    drag = 0; drag_card = 0; move_obj = -1;
}
static int ob_base = -40;   // (D41) the height an item stands on (y up is negative)
static int ob_x, ob_z, ob_sx = 68, ob_sz = 68, ob_w = 80, ob_l = 80, ob_rot, ob_hs = 100, lod_force;   // the item being drawn: its corner, its scale across and along (a 10th of a unit), its footprint (80 a cell), its turn
static int lod_ex, lod_ez; static s32 lod_r2 = 0x7FFFFFFF;   // (D38) the eye's place on the ground and how far from it things keep their detail
static int lod_far(void) { int dx = (ob_x - lod_ex) >> 2, dz = (ob_z - lod_ez) >> 2; return dx * dx + dz * dz > lod_r2; }   // (was: far from the point looked at, so the things nearest the eye, at the foot of the screen, were the ones drawn as blocks)
static const u8 HSC[28] = { 140, 140, 140, 140, 200, 200, 200, 200, 200, 200, 180, 200, 200, 100, 170, 200, 150, 200, 170, 200, 150, 100, 200, 150, 100, 200, 90, 100 };
static int ob_tx_ok;   // (D52) 1 while a bought thing is drawn: its parts take the PS1's patterns by their colour
static int item_tx(u16 c) {   // (D52) wood grain on brown, lawn on green, carpet on bright colours, plaster on grey; white and the very dark stay plain
    int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31, mx = MAX(r, MAX(g, b)), mn = MIN(r, MIN(g, b)), sat = mx - mn;
    if (mx < 8) return -1; if (sat < 4) return mn >= 24 ? -1 : TX_STUCCO;
    if (g > r + 3 && g > b + 3) return TX_GRASS;
    if (r >= g && g >= b && r - b >= 6 && mx <= 23) return TX_WOOD_DECK;
    return TX_SOIL; }
ITCM_CODE static void ob2(int lx0, int lz0, int lx1, int lz1, int h0, int h1, u16 c, u16 top) {   // a part in cell units (0..80 a cell, front = +z), heights in tenths of a quarter-tile; sides c, top
    if (pv_cap) { if (npv < PVMAX && h1 > 0) { PBox *b = &pv[npv++]; b->x0 = (s16)lx0; b->z0 = (s16)lz0; b->x1 = (s16)lx1; b->z1 = (s16)lz1; b->h0 = (s16)(h0 * ob_hs / 100); b->h1 = (s16)(h1 * ob_hs / 100); b->c = c; b->t = top; } return; }   // (the shop's picture)
    int ax, az, bx2, bz;
    switch (ob_rot & 3) { case 1: ax = lz0; az = ob_w - lx0; bx2 = lz1; bz = ob_w - lx1; break; case 2: ax = ob_w - lx0; az = ob_l - lz0; bx2 = ob_w - lx1; bz = ob_l - lz1; break; case 3: ax = ob_l - lz0; az = lx0; bx2 = ob_l - lz1; bz = lx1; break; default: ax = lx0; az = lz0; bx2 = lx1; bz = lz1; }
    int X0 = ob_x + (ax < bx2 ? ax : bx2) * ob_sx / 10, X1 = ob_x + (ax < bx2 ? bx2 : ax) * ob_sx / 10, Z0 = ob_z + (az < bz ? az : bz) * ob_sz / 10, Z1 = ob_z + (az < bz ? bz : az) * ob_sz / 10;
    int hq = T / 4; tx_want = ob_tx_ok ? item_tx(c) : -1; block(X0, Z0, X1, Z1, ob_base - h0 * hq / 10 * ob_hs / 100, ob_base - h1 * hq / 10 * ob_hs / 100, c, top); tx_want = -1;
}
static void ob(int lx0, int lz0, int lx1, int lz1, int h0, int h1, u16 c) { ob2(lx0, lz0, lx1, lz1, h0, h1, c, shade(c, 10)); }
#define LC(r, g, b) C15(r, g, b)
static int fm_need(int k);
static void farm_extras(int ch, int x, int y, int base) {   // (D38) the PS1's farm_extras: what grows in a crop bed (its crop by where the bed is), the hens about a coop
    int lod = ch >> 8; ch &= 0xFF;
    if (ch == 0xDB) { int k = ((x >> 1) + (y >> 1)) & 3, n = fm_n[k]; if (!n) return;
        int need = fm_need(k), g = fm_g[k], ready = g >= need, h = ready ? 32 : 8 + 20 * g / need, cnt = (n + 3) / 4 + 1, dry = fm_dry[k] > 0 && !ready; if (cnt > 4) cnt = 4;
        u16 st = dry ? C15(19, 16, 6) : C15(5, 21, 6);
        if (lod) { static const u16 RC[4] = { C15(31, 26, 10), C15(30, 26, 7), C15(29, 5, 5), C15(30, 16, 4) }; ob(14, 14, 66, 66, base, base + (k == 3 ? 10 : h * 3 / 4), ready ? RC[k] : k == 0 && !dry ? C15(27, 22, 7) : st); return; }   // (far off: one block of the crop)
        for (int i = 0; i < cnt; i++) { int cx = (i & 1) ? 57 : 23, cz = (i & 2) ? 57 : 23;
            switch (k) {
            case 0: ob(cx - 12, cz - 3, cx + 12, cz + 3, base, base + h, dry ? C15(22, 18, 6) : C15(27, 22, 7)); if (ready) ob(cx - 13, cz - 4, cx + 13, cz + 4, base + h, base + h + 5, C15(31, 26, 10)); break;   // wheat: a row of stalks, golden ears when ripe
            case 1: ob(cx - 4, cz - 4, cx + 4, cz + 4, base, base + h + 8, st); if (ready) ob(cx + 4, cz - 3, cx + 10, cz + 3, base + h - 4, base + h + 6, C15(30, 26, 7)); break;   // corn: a stalk, a cob when ripe
            case 2: ob(cx - 11, cz - 11, cx + 11, cz + 11, base, base + h * 3 / 4, st); if (ready) { ob(cx - 9, cz + 8, cx - 3, cz + 13, base + h / 3, base + h / 3 + 5, C15(29, 5, 5)); ob(cx + 3, cz + 8, cx + 9, cz + 13, base + h / 2, base + h / 2 + 5, C15(29, 5, 5)); } break;   // tomatoes: a bush, red fruit
            default: ob(cx - 13, cz - 13, cx + 13, cz + 13, base, base + 4, st); if (ready) ob(cx - 9, cz - 9, cx + 9, cz + 9, base + 4, base + 4 + h / 2, C15(30, 16, 4)); else if (h > 16) ob(cx - 3, cz - 3, cx + 3, cz + 3, base + 4, base + 8, C15(15, 21, 7)); break;   // pumpkins
            } }
        if ((fm_pest >> k) & 1) for (int i = 0; i < 3; i++) ob(10 + i * 25, 30 + i * 8, 16 + i * 25, 36 + i * 8, base, base + 3, C15(5, 4, 4));   // pests
    } else if (!lod) { int ci = (y >> 1) & 1, nh = (fm_hens - ci + 1) / 2, t = (int)vbl_n; if (nh > 3) nh = 3;   // (two coops share the hens, as on the PS1)
        for (int i = 0; i < nh; i++) { int ph = (t * 3 + i * 97 + ci * 40) & 255, hx = 16 + i * 24 + (sinLerp((s16)(ph << 7)) >> 10), hz = 76 + ((i * 13) & 7), pk = ((t / 10 + i * 3) & 7) == 0 ? 2 : 0;
            ob(hx - 4, hz - 6, hx + 4, hz + 3, 0, 8 - pk, C15(30, 30, 29)); ob(hx - 2, hz + 2, hx + 2, hz + 6, 6 - pk, 10 - pk, C15(28, 6, 5)); } }
}
static u8 ID2CH[NITEM]; static void ramp3d(int x, int y, int ch, int r, int base);   // (D45)
static int ground_lift(int cx, int cy, int L) {   // (D41, as the PS1's fix: everything stands on the ground under it) how high the ground is at a cell: a floor 40, the lawn 20 (the checker's top), a pavement 24, a road 0
    if (L > 0) return 40; if (cx >= 0 && cy >= 0 && cx < GN && cy < GN) return flr[cy][cx] ? 40 : 20;
    int lx = ((cx % SLC) + SLC) % SLC, ly = ((cy % SLC) + SLC) % SLC; if ((lx >= 14 && lx <= 16) || (ly >= 14 && ly <= 16)) return 0; if (lx >= 13 || ly >= 13) return 24; return 20; }
static void obj3d_in(int x, int y, const Item *it, int rot, int ghost) {   // every item is a little model built from several boxes (coordinates 0..80 across the cell, heights in tenths of a quarter-tile)
    (void)ghost; tx_want = -1; int id = (int)(it - ITEMS); u16 c = it->col, d = shade(c, 6), l = shade(c, 10), wd = LC(16, 10, 5), dk = LC(3, 3, 4);
    { int ww = rot & 1 ? it->fl : it->fw, wl = rot & 1 ? it->fw : it->fl; ob_w = it->fw * 80; ob_l = it->fl * 80; ob_sx = (ww * 2 * T - T / 3) * 10 / (ww * 80); ob_sz = (wl * 2 * T - T / 3) * 10 / (wl * 80); }   // (a two-cell thing stretches to fill both)
    ob_x = tl(x * 2) + T / 6; ob_z = tl(y * 2) + T / 6; ob_rot = rot; ob_hs = id >= 0 && id < 28 ? HSC[id] : 100; ob_base = -ground_lift(x, y, gy_off ? 1 : 0);
    if (!pv_cap && id >= 0) { int rc = ID2CH[id]; if ((rc == '1' || rc == '5' || rc == '9') && !gy_off) { if (dpass == 2 && !ghost) return; ramp3d(x, y, rc, rot & 3, ob_base); return; } }   // (D45) a ramp: the PS1's curve (ds_games.c)
    if (it->nbox) {   // (D35) a PS1 model: its own boxes, converted by tools/dsbuild/mk_dsitems.py (PS1 units: 10 across, 7 up)
        int fch = id == PS1ID[0xDB] - 1 ? 0xDB : id == PS1ID[0xDC] - 1 ? 0xDC : 0, nb = fch == 0xDB ? 1 : it->nbox;   // (D38: a crop bed is its soil and what the farm has growing; a coop has its hens)
        if (dpass == 2 && !pv_cap && !ghost) { if (fch == 0xDC && o_det && !lod_force && !(mode == M_LIVE && lod_far())) farm_extras(fch, x, y, IBOX[it->box0].z1 * 7); return; }   // (D39: the moving pass draws only the hens)
        if (!pv_cap && (!o_det || lod_force || (mode == M_LIVE && lod_far()))) {   // far off: one box, as big as the model (D38: was the item's nominal height, so a crop bed stood like a crate)
            if (fch == 0xDB) { const IBx *b = &IBOX[it->box0]; ob(0, 0, 80, 80, 0, b->z1 * 7, b->side); farm_extras(fch | 0x100, x, y, b->z1 * 7); return; }
            int top = 0, a0 = 80, b0 = 80, a1 = 0, b1 = 0; for (int k = 0; k < nb; k++) { const IBx *b = &IBOX[it->box0 + k]; if (b->z1 * 7 > top) top = b->z1 * 7; if (b->a0 < a0) a0 = b->a0; if (b->b0 < b0) b0 = b->b0; if (b->a1 > a1) a1 = b->a1; if (b->b1 > b1) b1 = b->b1; }
            ob(a0 * 10, b0 * 10, a1 * 10, b1 * 10, 0, top, c); return; }
        if (o_shd && !pv_cap && it->kind != 1) { u16 sh = LC(6, 5, 4); ob_x -= 6; ob_z -= 4; ob(0, 0, ob_w, ob_l, -3, 0, sh); ob_x += 6; ob_z += 4; }
        for (int k = 0; k < nb; k++) { const IBx *b = &IBOX[it->box0 + k]; ob2(b->a0 * 10, b->b0 * 10, b->a1 * 10, b->b1 * 10, b->z0 * 7, b->z1 * 7, b->side, b->top); }
        if (fch && !pv_cap && !(fch == 0xDC && dpass == 1)) farm_extras(fch, x, y, IBOX[it->box0].z1 * 7);
        return; }
    if (!pv_cap && (!o_det || lod_force || (mode == M_LIVE && lod_far()))) { if (it->kind == 1) ob(0, 0, 80, 80, 0, 3, c); else ob(8, 8, 72, 72, 0, it->h * 10 + 6, c); return; }
    if (it->kind == 1) { ob(0, 0, 80, 80, 0, 2, d); ob(6, 6, 74, 74, 0, 4, c); ob(14, 14, 66, 66, 0, 5, l); return; }   // rug: border and centre
    if (o_shd && !pv_cap) { u16 sh = LC(6, 5, 4); ob_x -= 6; ob_z -= 4; ob(0, 0, 80, 80, -3, 0, sh); ob_x += 6; ob_z += 4; }   // a soft dark shadow on the floor
    switch (id) {
    case 0: for (int k = 0; k < 4; k++) ob(k % 2 ? 62 : 8, k / 2 ? 62 : 8, k % 2 ? 72 : 18, k / 2 ? 72 : 18, 0, 20, d); ob(6, 6, 74, 74, 20, 26, c); ob(6, 6, 74, 18, 26, 62, l); break;   // chair
    case 1: ob(0, 0, 80, 80, 0, 12, d); ob(8, 20, 72, 80, 12, 24, l); ob(0, 0, 80, 20, 12, 50, c); ob(0, 20, 12, 80, 12, 36, c); ob(68, 20, 80, 80, 12, 36, c); ob(24, 6, 56, 22, 24, 40, LC(28, 22, 8)); break;   // armchair with a cushion
    case 2: ob(0, 4, 80, 80, 0, 12, d); ob(4, 4, 76, 24, 12, 48, c); ob(4, 24, 40, 76, 12, 26, l); ob(40, 24, 76, 76, 12, 26, l); ob(0, 24, 10, 80, 12, 38, c); ob(70, 24, 80, 80, 12, 38, c); ob(8, 26, 24, 42, 26, 40, LC(28, 22, 8)); ob(56, 26, 72, 42, 26, 40, LC(10, 20, 26)); break;   // sofa with two pillows
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
    case 28: ob(4, 8, 76, 76, 0, 28, c); ob(8, 12, 72, 70, 28, 30, dk); ob(14, 16, 30, 32, 30, 33, LC(12, 12, 12)); ob(46, 16, 62, 32, 30, 33, LC(12, 12, 12)); ob(14, 44, 30, 60, 30, 33, LC(12, 12, 12)); ob(46, 44, 62, 60, 30, 33, LC(12, 12, 12)); ob(10, 76, 70, 79, 16, 24, LC(28, 8, 8)); break;   // stove
    case 29: ob(10, 20, 70, 66, 0, 14, c); ob(14, 64, 50, 67, 2, 12, dk); ob(54, 64, 66, 67, 4, 10, LC(8, 26, 8)); break;   // microwave
    case 30: ob(0, 8, 80, 72, 0, 28, c); ob(14, 20, 54, 60, 28, 30, LC(16, 20, 28)); ob(58, 18, 64, 26, 30, 44, LC(20, 20, 22)); ob(58, 18, 74, 24, 44, 46, LC(20, 20, 22)); break;   // kitchen sink
    case 31: ob(0, 20, 80, 56, 0, 30, wd); ob(0, 18, 80, 58, 30, 34, LC(22, 14, 8)); ob(8, 64, 22, 76, 0, 18, dk); ob(8, 64, 22, 76, 18, 22, c); ob(54, 64, 68, 76, 0, 18, dk); ob(54, 64, 68, 76, 18, 22, c); break;   // bar with stools
    case 32: ob(4, 4, 76, 76, 0, 4, c); ob(4, 4, 8, 76, 4, 64, LC(22, 28, 31)); ob(4, 72, 76, 76, 4, 64, LC(22, 28, 31)); ob(4, 4, 76, 8, 4, 64, LC(22, 28, 31)); ob(58, 8, 72, 20, 56, 62, LC(20, 20, 22)); ob(64, 8, 66, 12, 20, 56, LC(20, 20, 22)); break;   // shower cabin
    case 33: ob(14, 34, 66, 42, 0, 8, wd); ob(16, 36, 64, 40, 8, 56, c); ob(14, 34, 66, 36, 56, 60, wd); break;   // standing mirror
    case 34: ob(0, 0, 80, 80, 0, 26, wd); ob(8, 8, 72, 72, 26, 28, LC(10, 22, 30)); ob(0, 0, 80, 6, 26, 32, wd); ob(0, 74, 80, 80, 26, 32, wd); break;   // hot tub
    case 35: ob(10, 30, 70, 50, 0, 4, wd); ob(12, 36, 16, 44, 4, 50, wd); ob(64, 36, 68, 44, 4, 50, wd); ob(12, 38, 68, 42, 44, 48, wd); ob(20, 34, 60, 46, 20, 44, c); break;   // towel rack
    case 36: ob(0, 16, 80, 64, 0, 22, dk); ob(0, 8, 80, 30, 22, 42, dk); ob(6, 52, 74, 66, 22, 25, LC(31, 31, 31)); ob(10, 56, 70, 64, 25, 26, LC(2, 2, 2)); ob(26, 68, 54, 80, 0, 16, LC(8, 6, 6)); break;   // piano and stool
    case 37: ob(0, 10, 80, 70, 0, 20, wd); ob(4, 14, 76, 66, 20, 24, c); ob(40, 36, 46, 42, 24, 28, LC(31, 31, 31)); ob(24, 32, 30, 38, 24, 28, LC(28, 6, 6)); ob(52, 44, 58, 50, 24, 28, LC(28, 24, 4)); break;   // pool table
    case 38: ob(14, 14, 66, 66, 0, 70, c); ob(14, 14, 66, 30, 70, 82, c); ob(18, 58, 62, 66, 36, 60, LC(4, 16, 26)); ob(18, 52, 62, 66, 26, 32, dk); ob(26, 54, 32, 60, 32, 38, LC(28, 6, 6)); break;   // arcade cabinet
    case 39: ob(8, 30, 72, 52, 0, 6, wd); ob(14, 26, 66, 34, 6, 36, dk); ob(18, 28, 62, 33, 10, 32, LC(8, 18, 28)); ob(28, 40, 52, 56, 6, 10, LC(14, 14, 16)); break;   // a console and a screen
    case 40: ob(6, 40, 74, 76, 0, 72, wd); for (int r = 0; r < 4; r++) for (int k = 0; k < 5; k++) ob(10 + k * 12, 66, 20 + k * 12, 76, 6 + r * 16, 18 + r * 16, (r + k) & 1 ? LC(28, 8, 8) : (r + k) % 3 == 0 ? LC(8, 14, 28) : LC(10, 24, 10)); break;   // tall bookcase
    case 41: ob(36, 36, 44, 44, 0, 40, dk); ob(26, 26, 40, 40, 0, 6, dk); ob(20, 30, 60, 50, 40, 52, c); ob(54, 32, 72, 48, 46, 60, LC(10, 20, 30)); break;   // telescope
    case 42: ob(14, 40, 24, 46, 0, 60, c); ob(56, 40, 66, 46, 0, 60, c); ob(18, 34, 62, 40, 20, 58, LC(30, 30, 28)); ob(26, 30, 36, 34, 22, 26, LC(28, 8, 8)); ob(40, 30, 50, 34, 24, 28, LC(8, 14, 28)); break;   // an easel
    case 43: ob(6, 10, 74, 70, 0, 10, dk); ob(10, 14, 70, 66, 10, 14, LC(14, 14, 16)); ob(6, 6, 74, 16, 10, 52, dk); ob(24, 6, 56, 12, 52, 58, LC(20, 20, 22)); break;   // treadmill
    case 44: ob(0, 0, 80, 80, 0, 6, LC(24, 24, 22)); ob(6, 6, 74, 74, 6, 8, c); ob(66, 30, 72, 40, 6, 34, LC(26, 26, 28)); break;   // swimming pool
    case 45: ob(14, 24, 66, 56, 26, 36, dk); ob(18, 28, 62, 52, 36, 38, LC(8, 8, 8)); ob(20, 26, 26, 32, 0, 26, dk); ob(54, 26, 60, 32, 0, 26, dk); ob(20, 46, 26, 52, 0, 26, dk); ob(54, 46, 60, 52, 0, 26, dk); ob(14, 56, 66, 60, 30, 34, LC(26, 10, 6)); break;   // barbecue
    case 46: ob(6, 34, 12, 46, 0, 72, c); ob(68, 34, 74, 46, 0, 72, c); ob(6, 34, 74, 46, 66, 72, c); ob(26, 38, 28, 42, 18, 66, LC(20, 20, 22)); ob(52, 38, 54, 42, 18, 66, LC(20, 20, 22)); ob(22, 34, 58, 46, 14, 18, wd); break;   // swing set
    case 47: ob(10, 10, 70, 70, 0, 12, c); ob(14, 14, 66, 66, 12, 14, LC(8, 18, 30)); ob(36, 36, 44, 44, 14, 44, c); ob(28, 28, 52, 52, 44, 48, LC(8, 20, 30)); break;   // fountain
    case 48: ob(20, 30, 60, 50, 10, 30, c); ob(52, 30, 72, 48, 24, 42, c); ob(22, 28, 28, 34, 0, 10, d); ob(52, 28, 58, 34, 0, 10, d); ob(22, 46, 28, 52, 0, 10, d); ob(52, 46, 58, 52, 0, 10, d); break;   // puppy
    case 49: ob(8, 8, 72, 72, 0, 10, wd); ob(14, 14, 66, 66, 10, 12, c); break;   // dog bed
    case 50: ob(24, 24, 56, 56, 0, 6, LC(26, 26, 28)); ob(28, 28, 52, 52, 6, 8, LC(20, 10, 6)); break;   // dog bowl
    case 51: ob(6, 16, 74, 64, 0, 14, wd); ob(8, 18, 72, 62, 14, 50, c); ob(30, 30, 40, 40, 14, 26, LC(31, 20, 6)); ob(50, 40, 58, 48, 20, 30, LC(31, 8, 8)); ob(8, 18, 72, 62, 50, 52, wd); break;   // fish tank
    default: ob(4, 4, 76, 76, 0, it->h * 10, c); break; }
}
static void obj3d(int x, int y, const Item *it, int rot, int ghost) { ob_tx_ok = !pv_cap; obj3d_in(x, y, it, rot, ghost); ob_tx_ok = 0; }   // (D52)
static int door_open;
ITCM_CODE static void wall_seg(int x0, int z0, int x1, int z1, int v, int vert, int wh) {   // one wall piece (or a door) on a cell edge; vert: it runs along z
    u16 c = WALLC[v - 1], cap = shade(c, 10), base = shade(c, 6), frame = C15(11, 7, 4); int th = T / 16, ps = T / 6;
    if (v == 6) {   // a fence: two posts and two rails on the cell edge (gardens, yards, paddocks)
        int fh = T * 7 / 10; u16 pc = shade(c, 7), rc = c;
        if (!vert) { int zc = (z0 + z1) / 2; block(x0, zc - 14, x0 + 36, zc + 14, 0, -fh, pc, shade(pc, 10)); block(x1 - 36, zc - 14, x1, zc + 14, 0, -fh, pc, shade(pc, 10)); block(x0, zc - 7, x1, zc + 7, -fh * 30 / 100, -fh * 45 / 100, rc, shade(rc, 10)); block(x0, zc - 7, x1, zc + 7, -fh * 70 / 100, -fh * 85 / 100, rc, shade(rc, 10)); }
        else { int xc = (x0 + x1) / 2; block(xc - 14, z0, xc + 14, z0 + 36, 0, -fh, pc, shade(pc, 10)); block(xc - 14, z1 - 36, xc + 14, z1, 0, -fh, pc, shade(pc, 10)); block(xc - 7, z0, xc + 7, z1, -fh * 30 / 100, -fh * 45 / 100, rc, shade(rc, 10)); block(xc - 7, z0, xc + 7, z1, -fh * 70 / 100, -fh * 85 / 100, rc, shade(rc, 10)); }
        return; }
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
    { static const s8 WT[4] = { TX_CLAPBOARD, TX_BRICK, TX_STUCCO, TX_BEADBOARD }; tx_want = WT[(v - 1) & 3]; }   // (D43: the PS1's wall patterns, in the colour you painted)
    block(x0, z0, x1, z1, -40 - T / 7, -wh, c, cap); tx_want = -1;                                                       // the wall, with a pale cap
    (void)vert; (void)th;
}


// ---------- the living city (D18): citizens stroll the pavements, cars drive the roads, everyone follows the clock ----------
#define NCIT 24
#define NCAR 12
#define CITN (o_trf * (prof_full ? 8 : 4))                          // (D40: twice the crowd with the DSi's detail)
#define CARN (o_trf == 3 ? (prof_full ? 12 : 8) : o_trf * 3)
typedef struct { int x, y, dir, stop; u8 on, stage, skin, shirt, hair, pants; u16 id; } Cit;
typedef struct { u16 id; u8 rel, talks, taken; } CRel;
typedef struct { int x, y, dx, dy; u16 col; } Car;
static Cit cits[NCIT]; static CRel crt[64]; static Car cars[NCAR];
static const char *const FNAMES[12] = { "Mia", "Leo", "Zoe", "Max", "Ivy", "Sam", "Ana", "Kai", "Eli", "Ola", "Dex", "Una" };
static const char *const LNAMES[8] = { "Park", "Reyes", "Ford", "Nash", "Cole", "Vega", "Hart", "Lund" };
static u32 hsh(u32 a) { a ^= a >> 16; a *= 0x7feb352dU; a ^= a >> 15; a *= 0x846ca68bU; a ^= a >> 16; return a; }
static CRel *crel(u16 id) { int h = id & 63; for (int k = 0; k < 64; k++) { CRel *c = &crt[(h + k) & 63]; if (c->id == id) return c; if (!c->id) { c->id = id; return c; } } return &crt[h]; }
static void cit_name(const Cit *c, char *o) { sprintf(o, "%s %s", FNAMES[c->id % 12], LNAMES[(c->id / 12) % 8]); }
static int cit_street(int x, int y) { int lx = ((x % SLC) + SLC) % SLC, ly = ((y % SLC) + SLC) % SLC; return (lx == 13 || lx == 17 || ly == 13 || ly == 17 || (lx >= 13 && ly >= 13)) && !far_solid(x, y); }   // (D40: the pavements, and across the road only at the corners; it was the whole street, lanes too)
static int cit_vis(const Cit *c) { int hr = (minutes / 60) % 24; return c->on && hr >= 6 && hr < 23 && ((hr * 5 + c->id) % 7) < 5; }   // asleep indoors at night, out and about by day
static void city_spawn(void) {
    int sl[9], ns = 0; for (int j = 0; j < 9; j++) if (nb[j / 3][j % 3].set != 255 && nb[j / 3][j % 3].terrain != BT_SEA) sl[ns++] = j;
    for (int k = 0; k < NCIT; k++) { Cit *c = &cits[k]; c->on = 0; c->stop = 0; if (!ns) continue; int j = (k < 6 && nb[1][1].set != 255) ? 4 : sl[k % ns], sy = j / 3 - 1, sx = j % 3 - 1;
        u32 h = hsh((u32)(lot_bx + sx) * 73856093u ^ (u32)(lot_by + sy) * 19349663u ^ (u32)k * 83492791u); c->id = (u16)((h >> 8) | 1); if (crel(c->id)->taken) continue;
        int r = (int)(h % 100); c->stage = (u8)(r < 16 ? 2 : r < 30 ? 3 : r < 86 ? 4 : 5);
        int lx = (h >> 8) & 1 ? 17 : 13, ly = (int)((h >> 12) % 18);   // (D40: on a pavement, not in a lane) if ((h >> 20) & 1) { int t = lx; lx = ly; ly = t; }
        int cx = sx * SLC + lx, cy = sy * SLC + ly; if (far_solid(cx, cy)) continue;
        c->x = cx * 16; c->y = cy * 16; c->dir = (int)((h >> 24) & 3); c->skin = (u8)((h >> 5) % 6); c->shirt = (u8)((h >> 9) % 6); c->hair = (u8)((h >> 13) % 6); c->pants = (u8)((h >> 17) % 4); c->on = 1; }
    for (int k = 0; k < NCAR; k++) { Car *c = &cars[k]; if (!ns) { c->x = c->y = -9999; c->dx = c->dy = 0; continue; } u32 h = hsh((u32)k * 977u + (u32)lot_bx * 31u + (u32)lot_by * 7u); int j = (int)((k >> 1) % 3) - 1, hz = k & 1, fw = (int)((k >> 2) & 1); int at = (int)(h % (SLC * 3 * 16)) - SLC * 16;
        if (hz) { c->y = (fw ? 16 : 14) * 16 + j * SLC * 16; c->x = at; c->dx = fw ? 1 : -1; c->dy = 0; } else { c->x = (fw ? 16 : 14) * 16 + j * SLC * 16; c->y = at; c->dx = 0; c->dy = fw ? 1 : -1; }
        { static const u16 cc[6] = { C15(28, 6, 6), C15(8, 12, 28), C15(28, 26, 6), C15(10, 24, 10), C15(26, 26, 28), C15(4, 4, 6) }; c->col = cc[h % 6]; } }
}
static void city_step(int frames) {
    static const int DXs[4] = { 1, 0, -1, 0 }, DYs[4] = { 0, 1, 0, -1 };
    for (int k = 0; k < CITN; k++) { Cit *c = &cits[k]; if (!c->on) continue; if (c->stop > 0) { c->stop--; continue; } if (!cit_vis(c) || ((frames + k) & 1)) continue;
        if (!(c->x & 15) && !(c->y & 15)) { int cx = c->x / 16, cy = c->y / 16, d = c->dir; if (hsh((u32)(frames * 31 + k)) % 100 < 30) d = (d + (hsh((u32)frames + (u32)k) & 1 ? 1 : 3)) & 3;
            for (int t = 0; t < 4; t++) { int dd = (d + t) & 3; if (cit_street(cx + DXs[dd], cy + DYs[dd])) { c->dir = dd; break; } } }
        { int nx = c->x + DXs[c->dir], ny = c->y + DYs[c->dir]; if (!(c->x & 15) && !(c->y & 15) && !cit_street(c->x / 16 + DXs[c->dir], c->y / 16 + DYs[c->dir])) continue; c->x = nx; c->y = ny; } }
    for (int k = 0; k < CARN; k++) { Car *c = &cars[k]; c->x += c->dx; c->y += c->dy; int lo = -SLC * 16, hi = SLC * 2 * 16; if (c->x < lo) c->x = hi - 1; if (c->x >= hi) c->x = lo; if (c->y < lo) c->y = hi - 1; if (c->y >= hi) c->y = lo; }
}
static void city_shift(int dx, int dy) {
    for (int k = 0; k < NCIT; k++) { cits[k].x -= dx * SLC * 16; cits[k].y -= dy * SLC * 16; }
    for (int k = 0; k < NCAR; k++) { cars[k].x -= dx * SLC * 16; cars[k].y -= dy * SLC * 16; int w = SLC * 3 * 16, lo = -SLC * 16; if (cars[k].x < lo) cars[k].x += w; if (cars[k].x >= lo + w) cars[k].x -= w; if (cars[k].y < lo) cars[k].y += w; if (cars[k].y >= lo + w) cars[k].y -= w; }
}
static void draw_city(void) {
    static const u16 SK[6] = { C15(30, 24, 20), C15(27, 20, 15), C15(22, 15, 10), C15(16, 10, 6), C15(11, 7, 4), C15(29, 22, 17) };
    static const u16 SH[6] = { C15(26, 26, 26), C15(24, 8, 8), C15(8, 14, 26), C15(8, 22, 10), C15(28, 22, 8), C15(16, 8, 22) };
    static const u16 HR[6] = { C15(3, 3, 4), C15(14, 8, 4), C15(26, 20, 8), C15(24, 8, 4), C15(25, 25, 25), C15(10, 14, 28) };
    for (int k = 0; k < CITN; k++) { const Cit *c = &cits[k]; if (!cit_vis(c) || polys > 2800) continue; int sc = STSC[c->stage], x = c->x * 2 * T / 16 - GN * T + T, z = c->y * 2 * T / 16 - GN * T + T, sw = (((c->x + c->y) >> 3) & 1) ? 30 : -30;
        gy_off = -ground_lift(fdiv(c->x + 8, 16), fdiv(c->y + 8, 16), 0);   // (D41: on the pavement, not sunk in it)
        u16 pn = c->pants == 0 ? C15(6, 7, 12) : c->pants == 1 ? C15(10, 10, 14) : c->pants == 2 ? C15(14, 10, 6) : C15(8, 12, 8);
        block(x - 70 * sc / 100, z - 40 * sc / 100 + sw, x + 70 * sc / 100, z + 40 * sc / 100 + sw, 0, -300 * sc / 100, pn, pn);
        block(x - 90 * sc / 100, z - 50 * sc / 100, x + 90 * sc / 100, z + 50 * sc / 100, -300 * sc / 100, -590 * sc / 100, SH[c->shirt], SH[c->shirt]);
        block(x - 60 * sc / 100, z - 60 * sc / 100, x + 60 * sc / 100, z + 60 * sc / 100, -590 * sc / 100, -800 * sc / 100, SK[c->skin], c->stage == 5 ? C15(25, 25, 26) : HR[c->hair]); }
    gy_off = 0;
    for (int k = 0; k < CARN; k++) { const Car *c = &cars[k]; if (polys > 2950) break;   // (D49: a car with a hood, a set-back glass cabin and a roof, lamps at the right end, a dark skirt over the wheels, facing the way it drives)
        int ax = c->dx != 0, sg = (ax ? c->dx : c->dy) < 0 ? -1 : 1, x = c->x * 2 * T / 16 - GN * T + T, z = c->y * 2 * T / 16 - GN * T + T, la = 300, lb = 140;   // (along its way, across)
#define CARB(a0, b0, a1, b1, y0, y1, w, r) do { int A0 = sg * (a0), A1 = sg * (a1); if (ax) block(x + A0, z + (b0), x + A1, z + (b1), y0, y1, w, r); else block(x + (b0), z + A0, x + (b1), z + A1, y0, y1, w, r); } while (0)
        u16 col = c->col, glass = C15(13, 19, 25);
        CARB(-la + 20, -lb + 12, la - 20, lb - 12, -8, -48, C15(5, 5, 7), C15(6, 6, 8));                                 // (the skirt and the wheels)
        CARB(-la, -lb, la, lb, -40, -140, col, shade(col, 10));                                                           // (the body)
        CARB(-la + 70, -lb + 18, la - 150, lb - 18, -140, -250, glass, shade(col, 6));                                    // (the cabin: glass, a roof the colour of the car)
        CARB(la - 12, -lb + 24, la, lb - 24, -60, -100, C15(31, 29, 18), C15(31, 29, 18));                                // (the headlamps)
        CARB(-la, -lb + 24, -la + 12, lb - 24, -60, -100, C15(28, 5, 4), C15(28, 5, 4)); }
#undef CARB
}
static void far_lot(const BInfo *b, int ox, int oz, int pop, int yaw) {   // a neighbour's lot: flat ground, a plain house or a few trees (cheap: the polygon budget is the DS's limit)
    u32 h = b->h; int k = b->lot; if (far_lod) pop = 0;
    u16 grass = b->terrain == BT_DESERT ? C15(24, 20, 10) : b->terrain == BT_ROCKY ? C15(13, 13, 13) : b->terrain == BT_BEACH ? C15(26, 24, 14) : seas(C15(11, 20, 8));
    tx_want = b->terrain == BT_DESERT || b->terrain == BT_BEACH ? TX_SAND : b->terrain == BT_ROCKY ? TX_GRAVEL : TX_GRASS; ground(ox + tl(0), oz + tl(0), ox + tl(26), oz + tl(26), -20, grass); tx_want = -1;
    if (!far_house(b)) { int n = (k == LOT_PARK || k == LOT_SQUARE) ? 4 : 2; for (int i = 0; i < n; i++) tree(ox + tl(3 + (int)((h >> (i * 4)) & 15) * 3 / 2 % 20), oz + tl(3 + (int)((h >> (i * 4 + 2)) & 15) * 3 / 2 % 20), T * 2); return; }
    int w = 10 + (int)(h & 3) * 2, d = 8 + (int)((h >> 2) & 3) * 2, x0 = 4 + (24 - w) / 2 / 2 * 2 - 4, z0 = 4;
    int st = bm_storeys(b); int hh = 2 * st + (st == 1 ? 2 : 0);
    u16 walls[6] = { C15(26, 22, 16), C15(24, 14, 10), C15(22, 24, 26), C15(26, 26, 18), C15(16, 22, 14), C15(24, 20, 22) }; u16 wall = walls[(h >> 8) % 6];
    if (k >= LOT_SHOP) wall = k == LOT_TOWER || k == LOT_OFFICE || k == LOT_HOTEL ? C15(16, 20, 26) : C15(27, 12, 10);
    int X0 = ox + tl(x0), X1 = ox + tl(x0 + w), Z0 = oz + tl(z0), Z1 = oz + tl(z0 + d), Y = -(hh * T);
    tx_want = k > LOT_APART ? TX_WINDOWS : TX_CLAPBOARD; block(X0, Z0, X1, Z1, 0, Y, wall, wall); tx_want = -1;   // (D43: shops and towers show storeys of windows)
    if (!far_lod) { int wn = (X1 - X0) / 5; block(X0 + wn * 2, Z1, X0 + wn * 2 + wn * 7 / 10, Z1 + 14, 0, -T * 3 / 2, C15(12, 7, 4), C15(12, 7, 4)); block(X0 + wn, Z1, X0 + wn + wn * 7 / 10, Z1 + 14, -T, -T * 2, C15(17, 24, 31), C15(17, 24, 31)); block(X0 + wn * 3, Z1, X0 + wn * 3 + wn * 7 / 10, Z1 + 14, -T, -T * 2, C15(17, 24, 31), C15(17, 24, 31)); }
    { u32 rh = hsh(h ^ 0x500Fu); int sty = st >= 3 || k > LOT_APART ? 1 : 2 + ((rh >> 2) & 1); static const s16 PIT[3] = { 110, 170, 250 };   // (D42) hip, gable or flat, a colour and a pitch per building
      roof_shape(X0 - T / 3, Z0 - T / 3, X1 + T / 3, Z1 + T / 3, Y, sty, PIT[1 + ((rh >> 8) & 1)], ROOFC[(rh >> 4) % 6], wall); }
    for (int i = 0; i < pop && i < 3; i++) person(ox + tl(4 + i * 3), oz + tl(z0 + d + 3), C15(6 + i * 4, 10, 25 - i * 3), yaw);   // the bores who live there
}
static u16 land_col(int t) { return t == BT_BEACH || t == BT_DESERT ? C15(26, 23, 15) : t == BT_ROCKY ? C15(14, 16, 12) : t == BT_FOREST ? seas(C15(8, 17, 6)) : seas(C15(11, 20, 8)); }   // (D37; D50: by the season)
static void lamp_kerb(int x, int z, int dx, int dz) {   // (D40, as the PS1's kerb_lamp) a street lamp on the pavement at the kerb, its arm out over the street (dx, dz: towards the road)
    u16 pc = C15(9, 10, 11); block(x - 12, z - 12, x + 12, z + 12, 0, -T * 5 / 2, pc, pc);
    int ax = x + dx * T / 2, az = z + dz * T / 2;   // (the arm and the lamp in one box: lamp-coloured underneath)
    block(dx ? (x < ax ? x : ax) - 22 * (dx < 0) : x - 22, dz ? (z < az ? z : az) - 22 * (dz < 0) : z - 22, dx ? (x > ax ? x : ax) + 22 * (dx > 0) : x + 22, dz ? (z > az ? z : az) + 22 * (dz > 0) : z + 22, -T * 5 / 2 + 30, -T * 5 / 2 - 6, C15(31, 29, 18), C15(12, 12, 13));
}
static void bin_at(int x, int z) { block(x - 30, z - 30, x + 30, z + 30, 0, -110, C15(6, 14, 8), C15(4, 8, 5)); }
static void kerb_set(int ox, int oz) {   // (D40) a slot's street lamps (two a side, by the corner and half way, never within two cells of each other) and a bin in the pavement's corner square
    lamp_kerb(tl(ox + 14), tl(oz + 28) - 30, 0, 1); lamp_kerb(tl(ox + 4), tl(oz + 34) + 30, 0, -1); lamp_kerb(tl(ox + 28) - 30, tl(oz + 4), 1, 0); lamp_kerb(tl(ox + 34) + 30, tl(oz + 14), -1, 0);   // (half way on the lot's side, by the corner across the street)
    bin_at(tl(ox + 26) + 70, tl(oz + 26) + 70);
}
static void slot_street(int ox, int oz) {   // (D40) a far slot's own streets (along its front and its right side) in its local tiles
    ground(tl(ox), tl(oz + 28), tl(ox + 36), tl(oz + 34), 0, C15(7, 7, 9)); ground(tl(ox + 28), tl(oz), tl(ox + 34), tl(oz + 28), 0, C15(7, 7, 9));
    ground(tl(ox), tl(oz + 26), tl(ox + 26), tl(oz + 28), -24, C15(22, 22, 22)); ground(tl(ox), tl(oz + 34), tl(ox + 36), tl(oz + 36), -24, C15(22, 22, 22));
    ground(tl(ox + 26), tl(oz), tl(ox + 28), tl(oz + 28), -24, C15(22, 22, 22)); ground(tl(ox + 34), tl(oz), tl(ox + 36), tl(oz + 34), -24, C15(22, 22, 22));
}
static void draw_world(int yaw) {   // LIVE: all round the lot, the lots next door, the streets and the land (a 3x3 patch of slots)
    if (zone == Z_ISLAND) { ground(tl(-40), tl(-40), tl(76), tl(76), 90, C15(25, 22, 14)); return; }   // (the island: sand between the lots, the sea is the neighbours')
    tx_want = zone == Z_TOWN ? TX_SAND : TX_GRASS; ground(tl(-40), tl(-40), tl(76), tl(76), 90, zone == Z_TOWN ? C15(24, 21, 13) : seas(C15(7, 16, 6))); tx_want = TX_ASPHALT;
    for (int j = -1; j <= 1; j++) { int o = 36 * j;   // roads both ways
        ground(tl(-36), tl(28 + o), tl(72), tl(34 + o), 0, C15(7, 7, 9)); ground(tl(28 + o), tl(-36), tl(34 + o), tl(72), 0, C15(7, 7, 9)); }
    tx_want = TX_PAVING; for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) { int ox = 36 * i, oz = 36 * j;   // pavements in the stretches between the roads
        ground(tl(ox), tl(oz + 26), tl(ox + 26), tl(oz + 28), -24, C15(22, 22, 22)); ground(tl(ox), tl(oz + 34), tl(ox + 26), tl(oz + 36), -24, C15(22, 22, 22));
        ground(tl(ox + 26), tl(oz + 26), tl(ox + 28), tl(oz + 28), -24, C15(22, 22, 22)); ground(tl(ox + 34), tl(oz + 26), tl(ox + 36), tl(oz + 28), -24, C15(22, 22, 22)); ground(tl(ox + 26), tl(oz + 34), tl(ox + 28), tl(oz + 36), -24, C15(22, 22, 22)); ground(tl(ox + 34), tl(oz + 34), tl(ox + 36), tl(oz + 36), -24, C15(22, 22, 22));
        ground(tl(ox + 26), tl(oz), tl(ox + 28), tl(oz + 26), -24, C15(22, 22, 22)); ground(tl(ox + 34), tl(oz), tl(ox + 36), tl(oz + 26), -24, C15(22, 22, 22)); }
    tx_want = -1; for (int x = 0; x < 36; x += 6) { ground(tl(x), tl(31) - 14, tl(x) + T * 2, tl(31) + 14, -30, C15(28, 26, 8)); ground(tl(31) - 14, tl(x), tl(31) + 14, tl(x) + T * 2, -30, C15(28, 26, 8)); }   // lane dashes by this lot
    if (!zone) for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) if (nb[j + 1][i + 1].terrain != BT_SEA) kerb_set(36 * i, 36 * j);   // (D40, as the PS1: lamps on the kerb facing the street, a bin by the corner)
}
static int ring_r = 1;   // (D40) how many slots out the land is drawn (Options > Draw distance, one more with the DSi's detail)
static u16 horizon_col(void) { return zone == Z_ISLAND ? C15(5, 12, 24) : zone == Z_TOWN ? C15(24, 21, 13) : C15(7, 16, 6); }
static void draw_far_all(int yaw) {   // last, so the lot, the sims and the citizens always get the polygons first
    for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) { if (!i && !j) continue; const BInfo *b = &nb[j + 1][i + 1];
        if (b->terrain == BT_SEA) { ground(tl(36 * i), tl(36 * j), tl(36 * i + 36), tl(36 * j + 36), -8, C15(5, 12, 24)); continue; }
        if (polys > 2700) continue; far_lot(b, 36 * i * T, 36 * j * T, nb_pop[j + 1][i + 1], yaw); }
    // (D40) THE FAR RING: the slots past the 3x3, each drawn round its own corner (a matrix move: no 16-bit overflow): plain houses, streets, the land; then the
    // horizon, one big quad scaled up four times, under everything out to the far plane. The haze fades it all into the sky, so nothing pops in.
    far_lod = 1;
    for (int r = 2; r <= ring_r; r++) for (int j = -r; j <= r; j++) for (int i = -r; i <= r; i++) { if (abs(i) != r && abs(j) != r) continue; const BInfo *b = &nbf[j + RING_MAX][i + RING_MAX];
        gx_push(36 * i * T, 0, 36 * j * T);
        if (b->terrain == BT_SEA) ground(tl(0), tl(0), tl(36), tl(36), -8, C15(5, 12, 24));
        else { ground(tl(0), tl(0), tl(36), tl(36), 90, land_col(b->terrain)); if (!zone || zone == Z_TOWN) slot_street(0, 0); far_lot(b, 0, 0, 0, yaw); }
        gx_pop(); }
    far_lod = 0;
    if (gxb) { int X = (gx_far + 9) * 1024; if (X > 32000) X = 32000; gx_c0(0x11); gx_id(0x1B); gxb[gxn++] = 4 * 4096; gxb[gxn++] = 4096; gxb[gxn++] = 4 * 4096; gx_lv = 0; ground(-X, -X, X, X, 180, horizon_col()); gx_c1(0x12, 1); gx_lv = 0; }
}
// ============ TRAVEL (D37): the PS1's zones, each a few DS lots laid out from the PS1's maps with the PS1's own things ============
// SAN BOREAS is the big map (zone 0). THE MAN BASE (house, factory, rail park), CASTAWAY ISLAND (the camp, the jungle, the hills, the east beach and the
// islet, by raft) and STRANGE TOWN (the diner, main street, the lab, the trailer) are 3x3 patches of lots past the map's edge (blocks 2000 and on).

#define ZB 2000
static const char *const ZNAME[Z_N] = { "San Boreas", "The Man Base", "Castaway Island", "Strange Town" };
static int zbx(int z, int col) { return ZB + z * 8 + col; }
static int z_of(int bx) { return bx >= ZB ? (bx - ZB) / 8 : 0; }
// what each zone lot is: its ground and what it is (for the neighbours' look and the walls of the sea round the island)
static void zone_block(int bx, int by, BInfo *b) {
    int z = z_of(bx), c = (bx - ZB) % 8, r = by - ZB; memset(b, 0, sizeof *b); b->set = 255; b->h = (u32)(bx * 7919 + by * 104729); b->terrain = BT_SEA; b->lot = LOT_WILD;
    if (r < 0 || r > 2 || c < 0) return;
    if (z == Z_BASE && c <= 2) { b->terrain = BT_GRASS; b->lot = c == 1 && r == 1 ? LOT_HOUSE : c == 2 && r == 1 ? LOT_OFFICE : c == 1 && r == 2 ? LOT_SKATE : LOT_WILD; b->road = 3; }
    if (z == Z_TOWN && c <= 2) { b->terrain = BT_DESERT; b->lot = c == 0 && r == 1 ? LOT_DINER : c == 2 && r == 1 ? LOT_OFFICE : c == 1 && r == 2 ? LOT_HOUSE : LOT_WILD; b->road = 3; }
    if (z == Z_ISLAND) { if (c == 4 && r == 1) { b->terrain = BT_BEACH; return; }   // the islet, out in the sea
        if (c <= 2 && r <= 1) { b->terrain = c == 2 || (c == 1 && r == 1) ? BT_BEACH : r == 0 && c == 1 ? BT_ROCKY : BT_FOREST; } }
}
static void blk(int bx, int by, BInfo *b) { if (bx >= ZB) zone_block(bx, by, b); else bm_block(bx, by, b); }
static int z_put(int x, int y, int ch, int rot) {   // a PS1 thing by its letter, as the PS1's zone maps place them
    int id = PS1ID[ch & 255] - 1, keep = LV; LV = (rot >> 2) & 3; int busy = id < 0 || nobj >= 400 || x < 0 || y < 0 || x >= GN || y >= GN || obj_at(x, y) >= 0; LV = keep; if (busy) return -1;
    objs[nobj].x = (u8)x; objs[nobj].y = (u8)y; objs[nobj].c = (u8)(id >> 2); objs[nobj].i = (u8)(id & 3); objs[nobj].rot = (u8)rot; nobj++; occ_ok = 0; return nobj - 1; }
static void z_room(int x0, int y0, int x1, int y1, int fl, int wc) {   // a room: its floor, and walls round it on the cell edges
    for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) flr[y][x] = (u8)fl;
    for (int x = x0; x < x1; x++) { hwl[y0][x] = (u8)wc; hwl[y1][x] = (u8)wc; } for (int y = y0; y < y1; y++) { vwl[y][x0] = (u8)wc; vwl[y][x1] = (u8)wc; } }
static void z_scatter(u32 h, int n, const char *what, int x0, int y0, int x1, int y1) {   // nature: n things from 'what' on free cells of the box, by the lot's dice
    int len = (int)strlen(what);
    for (int k = 0; k < n * 4 && n > 0; k++) { h = hsh(h + (u32)k * 977u); int x = x0 + (int)(h % (u32)(x1 - x0)), y = y0 + (int)((h >> 8) % (u32)(y1 - y0)); if (z_put(x, y, (u8)what[(h >> 16) % (u32)len], (int)((h >> 20) & 3)) >= 0) n--; }
}
static int isl_wood, isl_stone, isl_leaves, isl_food = 3, isl_spear, isl_shells; static u16 isl_secrets;
enum { G_NONE, G_WOOD, G_STONE, G_LEAVES, G_BERRIES, G_COCONUT, G_FISH, G_DIG, G_WRECK, G_BOTTLE, G_JOURNAL, G_IDOL, G_SHELL, G_GOLD };
enum { SEC_BOTTLE, SEC_TREASURE, SEC_IDOL, SEC_WRECK, SEC_SHELLS, SEC_JOURNAL, SEC_ISLET, SEC_GOLD, SEC_N };
#define ISL_SHELLS 6
static void gen_zone(int bx, int by) {   // a zone lot, from the PS1's map of that zone (world.c world_gen, world_gen_island, world_gen_town), at a lot's size
    int z = z_of(bx), c = (bx - ZB) % 8, r = by - ZB; u32 h = (u32)(bx * 73856093 ^ by * 19349663);
    door_cx = 6; door_cy = 12; for (int q = 0; q < 3; q++) { spawn_x[q] = 5 + q; spawn_y[q] = 11; }
    if (z == Z_BASE) {
        if (c == 1 && r == 1) {   // THE MAN BASE's house: peach walls, a lino kitchen, a pink-tile bathroom; the GBA game's things; stairs up to a bedroom
            z_room(1, 1, 12, 11, 1, 1); z_room(1, 1, 5, 5, 5, 3); hwl[5][3] = 5; hwl[11][6] = 5; door_cy = 11;
            for (int y = 6; y < 11; y++) for (int x = 8; x < 12; x++) flr[y][x] = 3;
            z_put(1, 1, 'T', 0); z_put(4, 1, 'H', 0);
            z_put(11, 6, 'F', 3); z_put(11, 7, 'F', 3); z_put(6, 1, '#', 0); z_put(7, 1, '#', 0); z_put(6, 2, '#', 0); z_put(7, 2, '#', 0);
            z_put(2, 9, 'S', 0); z_put(1, 6, 'C', 1); z_put(1, 5, 'V', 0); z_put(3, 6, 'G', 0); z_put(1, 8, 'U', 1); z_put(4, 7, 'Q', 0); z_put(5, 7, 'I', 0);
            z_put(5, 9, 'R', 0); z_put(5, 10, 'A', 0); z_put(8, 8, 'v', 0); z_put(9, 10, 'h', 2); z_put(9, 9, 't', 0); z_put(10, 10, 'h', 2); z_put(8, 10, 'p', 0); z_put(11, 9, 'y', 3); z_put(11, 10, 'c', 3); z_put(4, 9, 'B', 0);
            z_put(9, 3, '|', 2);   // the stairs, rising north (to the bedroom over the crate room)
            for (int y = 1; y < 6; y++) for (int x = 6; x < 12; x++) uflr[0][y][x] = 2;
            for (int x = 6; x < 12; x++) { uhwl[0][1][x] = 4; uhwl[0][6][x] = 4; } for (int y = 1; y < 6; y++) { uvwl[0][y][6] = 4; uvwl[0][y][12] = 4; }
            uflr[0][3][9] = 0; z_put(7, 1, 'S', 4); z_put(11, 1, 'b', 4); z_put(7, 4, 'l', 4); z_put(11, 4, 'C', 4 | 3); roof_st = 2;
            for (int q = 0; q < 3; q++) { spawn_x[q] = 6 + q; spawn_y[q] = 8; } }
        else if (c == 2 && r == 1) {   // the FACTORY: a big hall, crates, a rail line, the work spot
            z_room(1, 1, 12, 12, 8, 2); hwl[12][6] = 5; vwl[6][1] = 5; door_cy = 12;
            static const u8 cr[][2] = { {2,2},{3,2},{2,3},{3,3},{9,9},{10,9},{9,10},{10,10},{10,2},{10,3} }; for (unsigned k = 0; k < sizeof cr / 2; k++) z_put(cr[k][0], cr[k][1], '#', 0);
            for (int x = 3; x < 9; x++) z_put(x, 7, '=', 1); z_put(9, 5, 'x', 0); }
        else if (c == 1 && r == 2) {   // the RAIL PARK: concrete, rails, kickers, quarter pipes, ledges, benches, barrels, the bus stop
            for (int y = 0; y < GN; y++) for (int x = 0; x < GN; x++) flr[y][x] = 5;
            for (int x = 1; x < 5; x++) { z_put(x, 2, '=', 1); z_put(x + 7, 2, '=', 1); } for (int x = 3; x < 9; x++) z_put(x, 9, 'J', 1);
            z_put(4, 5, '4', 1); z_put(8, 5, '2', 3); z_put(5, 0, '5', 0); z_put(6, 0, '5', 0); z_put(1, 6, 'L', 1); z_put(1, 7, 'L', 1); z_put(11, 6, 'N', 1); z_put(11, 7, 'N', 1);
            z_put(6, 5, 'X', 0); z_put(7, 5, 'X', 0); z_put(6, 6, '9', 2); z_put(1, 10, 'O', 0); z_put(2, 10, 'O', 0); z_put(10, 4, 'K', 0); z_put(0, 3, 'Y', 0); z_put(12, 3, 'Y', 0);
            for (int x = 5; x < 9; x++) z_put(x, 11, 'M', 0); z_put(0, 12, '+', 0); z_put(12, 12, 'k', 0); z_put(0, 0, 'Z', 0); z_put(12, 0, 'Z', 0); }
        else z_scatter(h, 6, "3pZ6", 1, 1, 12, 12);
    } else if (z == Z_TOWN) {
        if (c == 0 && r == 1) {   // the DINER: a lino floor, a counter, tables and chairs, a fridge, a toilet, the fish tank
            z_room(1, 2, 12, 10, 3, 2); hwl[10][6] = 5; hwl[10][7] = 5; door_cy = 10;
            z_put(2, 2, 'F', 0); z_put(3, 2, 'y', 0); z_put(4, 2, 'y', 0); z_put(5, 2, 'c', 0); z_put(11, 2, 'T', 0); z_put(10, 2, 'R', 0);
            z_put(3, 6, 't', 0); z_put(2, 6, 'h', 1); z_put(4, 6, 'h', 3); z_put(8, 6, 't', 0); z_put(7, 6, 'h', 1); z_put(9, 6, 'h', 3); z_put(11, 8, 'q', 0);
            z_scatter(h, 3, "u*", 1, 11, 12, 13); }
        else if (c == 2 && r == 1) {   // the LAB: a hazard floor, crates, the work spot, the DeadSet, a desk
            z_room(1, 1, 11, 10, 4, 3); hwl[10][5] = 5; door_cy = 10; door_cx = 5;
            z_put(2, 2, '#', 0); z_put(3, 2, '#', 0); z_put(9, 2, 'Q', 0); z_put(6, 5, 'x', 0); z_put(2, 7, 'd', 0); z_put(3, 7, 'h', 0); z_put(9, 8, 'n', 0); }
        else if (c == 1 && r == 2) {   // the TRAILER across the street: a tiny home
            z_room(2, 3, 10, 8, 1, 4); hwl[3][5] = 5; door_cx = 5; door_cy = 3;
            z_put(2, 7, 'S', 0); z_put(9, 7, 'T', 0); z_put(9, 4, 'H', 0); z_put(2, 4, 'C', 1); z_put(6, 7, 'v', 2); }
        else if (c == 1 && r == 1) {   // MAIN STREET: the bus stop, a bench, the phone, planters
            for (int y = 0; y < GN; y++) for (int x = 0; x < GN; x++) flr[y][x] = 5;
            z_put(6, 4, '+', 0); z_put(4, 4, 'N', 1); z_put(8, 4, 'I', 0); z_put(1, 4, 'Z', 0); z_put(11, 4, 'Z', 0); z_put(3, 9, 'a', 0); z_put(10, 9, 'k', 0);
            for (int q = 0; q < 3; q++) { spawn_x[q] = 5 + q; spawn_y[q] = 6; } }
        else z_scatter(h, 7, "uuu*", 0, 0, 13, 13);
    } else if (z == Z_ISLAND) {
        if (c == 1 && r == 1) {   // the CAMP: the fire, a tent, an outhouse, log benches; the fishing spot and the boat on the south shore
            z_put(6, 6, 's', 0); z_put(4, 5, 'z', 0); z_put(9, 4, '&', 0); z_put(5, 7, '%', 1); z_put(7, 7, '%', 1); z_put(3, 8, 'a', 0);
            z_put(7, 12, 0x8A, 0); z_put(6, 12, '@', 2); z_scatter(h, 3, "g", 0, 0, 13, 3); z_put(10, 9, 0x88, 0); z_put(1, 11, 0x88, 0);
            for (int q = 0; q < 3; q++) { spawn_x[q] = 6 + q; spawn_y[q] = 9; } }
        else if (c == 0 && r == 1) { z_put(5, 5, 0x86, 0); z_scatter(h, 18, "g3g3\x89\x89", 0, 0, 13, 13); }                                   // the JUNGLE: palms, oaks, berry bushes, the tiki idol in the thick of it
        else if (c == 1 && r == 0) { z_put(5, 6, '*', 0); z_put(6, 6, 0x85, 0); z_put(9, 3, '0', 0); z_put(2, 9, 'z', 1); z_put(3, 9, 0x8D, 0); z_scatter(h, 12, "**6", 0, 0, 13, 13); }   // the HILLS: rocks and pines, the spring, the big rock and its mound, the hermit's old camp
        else if (c == 2 && r == 1) { z_put(10, 6, 0x87, 0); z_put(4, 11, 0x84, 0); z_put(8, 10, 0x88, 0); z_put(2, 3, 0x88, 0); z_put(12, 9, 0x8A, 0); z_scatter(h, 5, "g\x89", 0, 0, 13, 13); }   // the EAST BEACH: the wreck, a bottle, shells, a fishing spot
        else if (c == 4 && r == 1) { z_put(6, 6, 0x8C, 0); z_put(4, 4, 'g', 0); z_put(9, 8, 'g', 0); z_put(8, 3, 'g', 0); z_put(3, 9, 0x88, 0); z_put(1, 6, 0x8B, 1); for (int q = 0; q < 3; q++) { spawn_x[q] = 2; spawn_y[q] = 5 + q; } }   // the ISLET: the golden idol, palms, a raft home
        else z_scatter(h, 9, "g3\x89", 0, 0, 13, 13);
    }
}
static int zone_open(int z) { int paid = 0, lvl = 0;
#ifdef DSTEST
    if (z == Z_TOWN) return 1;   // (test builds only)
#endif
    for (int i = 0; i < nsim; i++) { if (life[i].shifts) paid = 1; if (life[i].job && life[i].lvl > lvl) lvl = life[i].lvl; } return z == Z_BASE || z == Z_CITY || (z == Z_ISLAND && paid) || (z == Z_TOWN && lvl >= 2); }   // (the PS1's: the island after the first paycheck, the town after two promotions)
static int travel_ui, craft_ui;
static void lot_go(int bx, int by);
static void isl_found(int sec) {
    if ((isl_secrets >> sec) & 1) return; isl_secrets |= (u16)(1 << sec); static char t[40]; int n = 0; for (int i = 0; i < SEC_N; i++) n += (isl_secrets >> i) & 1;
    sprintf(t, "Secret found! %d of %d", n, SEC_N); say(n == SEC_N ? "Every secret of the island!" : t); sfx(5); life_ev(ctl, WN_VISIT, 0); }
static void travel_go(int z) {   // the whole household goes (the PS1's travel_go): to the zone's arrival lot, or home
    static char b[40];
    if (z == Z_CITY) { if (home_bx < 0) { say("No home yet: pick one on the Map"); return; } zone = Z_CITY; lot_go(home_bx, home_by); sprintf(b, "Welcome home"); say(b); return; }
    zone = z; lot_go(zbx(z, 1), ZB + 1); sprintf(b, "Welcome to %s", ZNAME[z]); say(b); life_ev(ctl, WN_VISIT, 0);
    if (z == Z_ISLAND) { msg_t = 160; say("Find food: berries, fish. Tap trees and rocks to gather"); }
}

// ---- storeys and the roof (D36): every storey drawn at its height, the roof over the top one ----
static int top_lv(void) {   // the highest storey with a floor or a wall on it
    for (int L = UPN; L > 0; L--) { const u8 *f = &uflr[L - 1][0][0], *h = &uhwl[L - 1][0][0], *v = &uvwl[L - 1][0][0];
        for (int k = 0; k < GN * GN; k++) if (f[k]) return L; for (int k = 0; k < (GN + 1) * GN; k++) if (h[k] || v[k]) return L; }
    return 0;
}
static int lv_box(int L, int *x0, int *y0, int *x1, int *y1) {   // the outline of a storey's walls (not fences), or of its floor
    u8 (*hw)[GN] = HWv(L); u8 (*vw)[GN + 1] = VWv(L); u8 (*fl)[GN] = FLv(L); *x0 = GN; *x1 = -1; *y0 = GN; *y1 = -1;
    for (int y = 0; y <= GN; y++) for (int x = 0; x < GN; x++) if (hw[y][x] && hw[y][x] != 6) { if (x < *x0) *x0 = x; if (x + 1 > *x1) *x1 = x + 1; if (y < *y0) *y0 = y; if (y > *y1) *y1 = y; }
    for (int y = 0; y < GN; y++) for (int x = 0; x <= GN; x++) if (vw[y][x] && vw[y][x] != 6) { if (x < *x0) *x0 = x; if (x > *x1) *x1 = x; if (y < *y0) *y0 = y; if (y + 1 > *y1) *y1 = y + 1; }
    if (L) for (int y = 0; y < GN; y++) for (int x = 0; x < GN; x++) if (fl[y][x]) { if (x < *x0) *x0 = x; if (x + 1 > *x1) *x1 = x + 1; if (y < *y0) *y0 = y; if (y + 1 > *y1) *y1 = y + 1; }
    return *x1 > *x0 && *y1 > *y0;
}
// ---- (D42) ROOFS, as the PS1's (world.c roof_region): every enclosed building gets a roof over its top storey, from its footprint: HIP (sloped all round),
// GABLE (two slopes, plaster gable ends) or FLAT (a slab with a parapet). The height goes in from the edge at the pitch and levels off three cells in, as the PS1's
// caps; eaves overhang the walls. In San Boreas each building rolls its own style, colour and pitch (the PS1's dice: towers and big blocks flat); Build > Roof
// sets one style for the lot (Auto / Flat / Gable / Hip; the save's two roof bits, 0 was "None" and is now Auto). Cutaway hides the roof of a building a Bore of
// yours is in, so you see in, and every roof over storeys you are below (as the PS1's b_see); Always shows them all; Hide none.
static int view_lv(void);
static const u16 ROOFC[6] = { C15(24, 11, 8), C15(13, 14, 16), C15(19, 13, 8), C15(11, 16, 10), C15(9, 11, 17), C15(26, 22, 16) };   // the PS1's CLAY RED, SLATE, CEDAR, MOSS GREEN, NAVY, SANDSTONE
static void roof_shape(int X0, int Z0, int X1, int Z1, int ry, int sty, int rise, u16 rc, u16 wc) {   // a roof over the box (eaves included), its foot at ry (up is negative)
    int w = X1 - X0, d = Z1 - Z0, sh = w < d ? w : d, r = sh / 2; if (r > 6 * T) r = 6 * T;
    u16 fas = shade(rc, 5);
    if (sty == 1) {   // FLAT: the slab, a parapet round it, a box on top (the PS1's towers and big blocks)
        u16 tc = shade(C15(18, 18, 19), 8), pc = shade(wc, 7); tx_want = TX_GRAVEL;
        block(X0, Z0, X1, Z1, ry + 30, ry - 40, fas, tc); tx_want = TX_STUCCO;
        block(X0, Z0, X1, Z0 + 40, ry - 40, ry - 110, pc, shade(pc, 10)); block(X0, Z1 - 40, X1, Z1, ry - 40, ry - 110, pc, shade(pc, 10));
        block(X0, Z0 + 40, X0 + 40, Z1 - 40, ry - 40, ry - 110, pc, shade(pc, 10)); block(X1 - 40, Z0 + 40, X1, Z1 - 40, ry - 40, ry - 110, pc, shade(pc, 10));
        tx_want = -1; if (w > 4 * T && d > 4 * T) { int cx = X0 + w * 2 / 3, cz = Z0 + d / 3; block(cx - T / 2, cz - T / 2, cx + T / 2, cz + T / 2, ry - 40, ry - 40 - T * 2 / 3, C15(20, 20, 21), C15(25, 25, 26)); }
        return; }
    int RH_ = r * rise / 256, rtop = ry - RH_, along_x = w >= d, rx = along_x ? (sty == 3 ? r : 0) : r, rz = along_x ? r : (sty == 3 ? r : 0);
    tx_want = -1; block(X0, Z0, X1, Z1, ry + 40, ry, fas, fas); tx_want = TX_SHINGLE_WALL;   // (the eaves' thickness; then the slopes in the PS1's shingles)
    u16 c0 = rc, c1 = shade(rc, 6), ce = rx ? shade(rc, 7) : wc, cf = rz ? rc : wc;   // (gable ends are the walls' plaster)
    qd(X0, ry, Z0, X1, ry, Z0, X1 - rx, rtop, Z0 + rz, X0 + rx, rtop, Z0 + rz, cf);
    qd(X0 + rx, rtop, Z1 - rz, X1 - rx, rtop, Z1 - rz, X1, ry, Z1, X0, ry, Z1, rz ? c1 : shade(wc, 7));
    qd(X0, ry, Z0, X0 + rx, rtop, Z0 + rz, X0 + rx, rtop, Z1 - rz, X0, ry, Z1, ce);
    qd(X1 - rx, rtop, Z0 + rz, X1, ry, Z0, X1, ry, Z1, X1 - rx, rtop, Z1 - rz, rx ? c1 : shade(wc, 6));
    if (X1 - rx > X0 + rx && Z1 - rz > Z0 + rz) ground(X0 + rx, Z0 + rz, X1 - rx, Z1 - rz, rtop, shade(rc, 9));   // (levelled off: a flat top)
    tx_want = -1; (void)c0;
}
typedef struct { s8 x0, y0, x1, y1, top; u8 sty, col, pit; } Bld;
static Bld blds[12]; static int nbld;
static void bld_scan(void) {   // the enclosed buildings of the lot (cells the outside cannot reach without crossing a wall or a shut door), each with its box and top storey
    static u8 out[GN + 2][GN + 2], id[GN][GN]; static u8 qx[(GN + 2) * (GN + 2)], qy[(GN + 2) * (GN + 2)]; memset(out, 0, sizeof out); memset(id, 0, sizeof id); nbld = 0;
    int qh = 0, qt = 0;
    for (int y = 0; y < GN + 2; y++) for (int x = 0; x < GN + 2; x++) if (!x || !y || x == GN + 1 || y == GN + 1) { out[y][x] = 1; qx[qt] = (u8)x; qy[qt++] = (u8)y; }
    while (qh < qt) { int x = qx[qh], y = qy[qh++];   // (grid with a border: cell (x-1, y-1))
        static const s8 DD4[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        for (int k = 0; k < 4; k++) { int nx = x + DD4[k][0], ny = y + DD4[k][1]; if (nx < 0 || ny < 0 || nx > GN + 1 || ny > GN + 1 || out[ny][nx]) continue;
            int cx = x - 1, cy = y - 1, wv = 0;
            if (k == 0 && cy >= 0 && cy < GN && cx + 1 >= 0 && cx + 1 <= GN) wv = vwl[cy][cx + 1]; if (k == 1 && cy >= 0 && cy < GN && cx >= 0 && cx <= GN) wv = vwl[cy][cx];
            if (k == 2 && cx >= 0 && cx < GN && cy + 1 >= 0 && cy + 1 <= GN) wv = hwl[cy + 1][cx]; if (k == 3 && cx >= 0 && cx < GN && cy >= 0 && cy <= GN) wv = hwl[cy][cx];
            if (wv && wv != 6) continue; out[ny][nx] = 1; qx[qt] = (u8)nx; qy[qt++] = (u8)ny; } }
    for (int y = 0; y < GN; y++) for (int x = 0; x < GN; x++) if (!out[y + 1][x + 1] && !id[y][x] && nbld < 12) {   // a building: every inside cell joined to it (walls inside it do not part it)
        Bld *b = &blds[nbld++]; b->x0 = b->x1 = (s8)x; b->y0 = b->y1 = (s8)y; qh = qt = 0; id[y][x] = (u8)nbld; qx[qt] = (u8)x; qy[qt++] = (u8)y;
        while (qh < qt) { int cx = qx[qh], cy = qy[qh++]; if (cx < b->x0) b->x0 = (s8)cx; if (cx > b->x1) b->x1 = (s8)cx; if (cy < b->y0) b->y0 = (s8)cy; if (cy > b->y1) b->y1 = (s8)cy;
            static const s8 DD4[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
            for (int k = 0; k < 4; k++) { int nx = cx + DD4[k][0], ny = cy + DD4[k][1]; if (nx < 0 || ny < 0 || nx >= GN || ny >= GN || out[ny + 1][nx + 1] || id[ny][nx]) continue; id[ny][nx] = (u8)nbld; qx[qt] = (u8)nx; qy[qt++] = (u8)ny; } }
        int t = 0; for (int L = 1; L <= UPN; L++) { int any = 0; for (int yy = b->y0; yy <= b->y1 && !any; yy++) for (int xx = b->x0; xx <= b->x1; xx++) if (uflr[L - 1][yy][xx]) { any = 1; break; } if (any) t = L; }
        b->top = (s8)t;
        u32 h = hsh(lot_seed * 2654435761u ^ (u32)(b->x0 * 31 + b->y0 * 977)); int w = b->x1 - b->x0 + 1, dd = b->y1 - b->y0 + 1;
        BInfo bi; int kind = LOT_HOUSE; if (lot_bx >= 0 && lot_bx < 2000) { bm_block(lot_bx, lot_by, &bi); kind = bi.lot; }
        int big = kind >= LOT_SHOP && kind != LOT_PARK && kind != LOT_FARM;
        b->sty = (u8)(roof_st ? (roof_st == 1 ? 1 : roof_st == 2 ? 2 : 3) : (t >= 2 || (w >= 9 && dd >= 8 && (h & 3) == 0) || (big && (h & 1)) ? 1 : 2 + ((h >> 2) & 1)));   // (the PS1's dice: towers and big blocks flat, else hip or gable)
        b->col = (u8)(zone == Z_ISLAND ? 2 : (h >> 4) % 6); b->pit = (u8)(1 + ((h >> 8) & 1));
        if (zone == Z_ISLAND && b->sty == 1) b->sty = 2; }   // (the island's huts: thatch-brown gables)
}
static int bld_seen(const Bld *b) {   // (Cutaway) 0: a Bore of yours is inside, or you look at a storey under its top: no roof, so you see in
    if (mode != M_LIVE) return mode == M_BUILD && tool == 5;
    if (o_roof == 2) return 0; if (o_roof == 1) return 1;
    if (view_lv() < b->top) return 0;
    for (int q = 0; q < nsim; q++) { int cx = fdiv(sims[q].x + 8, 16), cy = fdiv(sims[q].y + 8, 16); if (cx >= b->x0 && cx <= b->x1 && cy >= b->y0 && cy <= b->y1) return 0; }
    return 1;
}
static u32 roof_bits(void) { bld_scan(); u32 m = 0; for (int i = 0; i < nbld; i++) if (bld_seen(&blds[i])) m |= 1u << i; return m; }
static void roof_draw(int top) {
    (void)top; bld_scan(); static const s16 PIT[3] = { 110, 170, 250 };
    for (int i = 0; i < nbld; i++) { const Bld *b = &blds[i]; if (!bld_seen(b)) continue;
        int X0 = tl(b->x0 * 2) - T / 3, X1 = tl(b->x1 * 2 + 2) + T / 3, Z0 = tl(b->y0 * 2) - T / 3, Z1 = tl(b->y1 * 2 + 2) + T / 3, ry = -40 - WHF * (b->top + 1);
        u16 wc = WALLC[0]; { int v = hwl[b->y0][b->x0]; if (v >= 1 && v <= 4) wc = WALLC[v - 1]; }
        roof_shape(X0, Z0, X1, Z1, ry, b->sty, PIT[b->pit], ROOFC[b->col], wc); }
}
static void draw_floors(int L) {   // floors as the biggest same-type rectangles: no seams for the edge anti-aliasing to show the grass through (upstairs: slabs, so a storey has a thickness)
    static u8 dn[GN][GN]; memset(dn, 0, sizeof dn); u8 (*fl)[GN] = FLv(L);
    for (int y = 0; y < GN; y++) for (int x = 0; x < GN; x++) { int t = fl[y][x]; if (!t || dn[y][x]) continue; int w = 1, d = 1;
        while (x + w < GN && fl[y][x + w] == t && !dn[y][x + w]) w++;
        for (;;) { if (y + d >= GN) break; int ok = 1; for (int k = 0; k < w; k++) if (fl[y + d][x + k] != t || dn[y + d][x + k]) ok = 0; if (!ok) break; d++; }
        for (int j = 0; j < d; j++) for (int k = 0; k < w; k++) dn[y + j][x + k] = 1;
        { static const s8 FT[5] = { TX_WOOD_DECK, TX_POOL_TILE, TX_SOIL, TX_GRAVEL, TX_PAVING }; tx_want = FT[(t - 1) % 5]; }   // (D43: wood, tiles, carpet, rush mat, slate)
        if (!L) ground(tl(x * 2) - 4, tl(y * 2) - 4, tl((x + w) * 2) + 4, tl((y + d) * 2) + 4, -40, FLOORC[t - 1]);
        else block(tl(x * 2) - 4, tl(y * 2) - 4, tl((x + w) * 2) + 4, tl((y + d) * 2) + 4, 30, -40, C15(24, 22, 19), FLOORC[t - 1]); }
    tx_want = -1;
}
static u32 *wsig;          // (D39) set: draw_walls only mixes which walls are cut down into *wsig (the still list's signature)
static void draw_walls(int L, int yaw) {   // thin walls (a skirting board, the wall, a pale cap) and proper doors
    u8 (*hw)[GN] = HWv(L); u8 (*vw)[GN + 1] = VWv(L);
    int th = T / 16, whf = WHF, whl = T * 2, cs = cosLerp(yaw), sn = sinLerp(yaw); int tx0 = mode == M_LIVE ? camx : 0, tz0 = mode == M_LIVE ? camz : 0;
    for (int y = 0; y <= GN; y++) for (int x = 0; x < GN; x++) { int v = hw[y][x]; if (!v) continue; int e = x + 1; if (v != 5 && v != 6) while (e < GN && hw[y][e] == v) e++;   // runs of the same wall are one piece (the polygon budget)
        int mx = (tl(x * 2) + tl(e * 2)) / 2, mz = tl(y * 2); int near = (((mx - tx0) * -sn + (mz - tz0) * cs) >> 12) > 2000;
        if (wsig) { *wsig = (*wsig ^ (u32)(near + x * 2 + y * 64)) * 16777619u; x = e - 1; continue; }
        if (v == 5 ? (dpass != 0 && dpass != 2) : (dpass != 0 && dpass != 1)) { x = e - 1; continue; }   // (doors open and shut as the sims pass: they are drawn with what moves)
        if (v == 5) { door_open = 0; for (int q = 0; q < nsim; q++) if (slv[q] == L && abs(sims[q].x - x * 16) < 26 && abs(sims[q].y - (y * 16 - 8)) < 26) door_open = 1; }
        int through0 = v != 6 && y > 0 && y < GN && vw[y - 1][x] && vw[y][x], through1 = v != 6 && y > 0 && y < GN && e <= GN && vw[y - 1][e] && vw[y][e];   // does a wall run right across the end? then stop inside it (no poking out)
        wall_seg(tl(x * 2) - (through0 ? -6 : th + 4), tl(y * 2) - th, tl(e * 2) + (through1 ? -6 : th + 4), tl(y * 2) + th, v, 0, near ? whl : whf); x = e - 1; }
    for (int x = 0; x <= GN; x++) for (int y = 0; y < GN; y++) { int v = vw[y][x]; if (!v) continue; int e = y + 1; if (v != 5 && v != 6) while (e < GN && vw[e][x] == v) e++;
        int mx = tl(x * 2), mz = (tl(y * 2) + tl(e * 2)) / 2; int near = (((mx - tx0) * -sn + (mz - tz0) * cs) >> 12) > 2000;
        if (wsig) { *wsig = (*wsig ^ (u32)(near + x * 4096 + y * 128)) * 16777619u; y = e - 1; continue; }
        if (v == 5 ? (dpass != 0 && dpass != 2) : (dpass != 0 && dpass != 1)) { y = e - 1; continue; }
        if (v == 5) { door_open = 0; for (int q = 0; q < nsim; q++) if (slv[q] == L && abs(sims[q].x - (x * 16 - 8)) < 26 && abs(sims[q].y - y * 16) < 26) door_open = 1; }
        int through0 = v != 6 && x > 0 && x < GN && hw[y][x - 1] && hw[y][x], through1 = v != 6 && x > 0 && x < GN && e <= GN && hw[e][x - 1] && hw[e][x];
        wall_seg(tl(x * 2) - th, tl(y * 2) - (through0 ? -6 : th + 12), tl(x * 2) + th, tl(e * 2) + (through1 ? -6 : th + 12), v, 1, near ? whl : whf); y = e - 1; }
}
static int view_lv(void) { return mode == M_LIVE ? slv[ctl] : ed_lv; }   // the storey you are looking at: the ones above it are cut away
static u8 ID2CH[NITEM];
static int show_lv(void) {   // the storeys drawn: the ones over the one you look at are cut away; Roofs: Always shows the whole house, and so does Cutaway when nobody is in it
    int top = top_lv(), vl = view_lv(), show = vl;
    if (mode == M_LIVE && top > 0 && o_roof != 2) { int x0, y0, x1, y1, in = 0; if (lv_box(0, &x0, &y0, &x1, &y1)) for (int q = 0; q < nsim; q++) { int cx = fdiv(sims[q].x + 8, 16), cy = fdiv(sims[q].y + 8, 16); if (cx >= x0 && cx < x1 && cy >= y0 && cy < y1) in = 1; }
        if (o_roof == 1 || !in) show = top; }
    if (mode == M_BUILD && tool == 5) show = top;   // (the Roof tool shows the whole house and its roof)
    if (show > top) show = top; if (show < vl) show = vl; return show;
}
static int roof_seen(int top) {   // (D39, from roof_draw) 0: Cutaway hides the roof because someone is up there
    int x0, y0, x1, y1; if (!lv_box(top, &x0, &y0, &x1, &y1)) return 0;
    if (mode == M_LIVE && o_roof == 0) for (int q = 0; q < nsim; q++) { int cx = fdiv(sims[q].x + 8, 16), cy = fdiv(sims[q].y + 8, 16); if (slv[q] == top && cx >= x0 && cx < x1 && cy >= y0 && cy < y1) return 0; }
    return 1;
}
static int obj_far(const Obj *o) { ob_x = tl(o->x * 2) + T / 6; ob_z = tl(o->y * 2) + T / 6; return mode == M_LIVE && lod_far(); }
static void draw_build(int yaw) {   // (D39) dpass 1: the still part (recorded once), 2: what moves (every frame), 0: both
    int st = dpass == 0 || dpass == 1, mv = dpass == 0 || dpass == 2, wd = dpass == 0 || dpass == 3, it = dpass == 0 || dpass == 4;   // (D41: 1 the lot's build, 4 its things, 3 the world round it, 2 what moves)
    u16 lawn = land_col(nb[1][1].terrain);
    if (dpass == 3) goto world;
    int ltx = nb[1][1].terrain == BT_BEACH || nb[1][1].terrain == BT_DESERT || zone == Z_ISLAND || zone == Z_TOWN ? TX_SAND : nb[1][1].terrain == BT_ROCKY ? TX_GRAVEL : TX_GRASS;
    if (st) { gx_pid(1); tx_want = ltx; ground(tl(0), tl(0), tl(GN * 2), tl(GN * 2), 0, lawn);   // the lawn (sand on a beach): one quad and big darker squares (every polygon counts on the DS)
        if (o_chk) for (int z = 0; z < GN; z += 2) for (int x = 0; x < GN; x += 2) if (((x + z) / 2) & 1) { int e = x + 2 > GN ? GN : x + 2, f = z + 2 > GN ? GN : z + 2; ground(tl(x * 2), tl(z * 2), tl(e * 2), tl(f * 2), -20, shade(lawn, 7)); }
        if (!zone) ground(tl(0), tl(26) - 30, tl(GN * 2), tl(26), -20, C15(14, 24, 10));   // a low hedge line along the back of the plot (no more posts sticking out)
        tx_want = TX_PAVING; if (!zone) block(tl(door_cx * 2) + T / 2, tl(door_cy * 2 + 1), tl(door_cx * 2 + 2) - T / 2, tl(26), 0, -26, C15(19, 18, 16), C15(23, 22, 20)); tx_want = -1; }   // a path from the front door to the pavement
    if (dpass == 2) PROF(10);
    int top = top_lv(), vl = view_lv(), show = show_lv(); if (dpass == 2) PROF(9);
    for (int L = show; L >= 0; L--) {   // (from the storey you look at down: the DS draws at most 2048 polygons, and what is sent last is what it drops)
        gy_off = -WHF * L; lod_force = L < vl;   // (the things on the storeys under yours: simple boxes)
        if (st) { gx_pid(1); draw_floors(L); }
        gx_pid(2); draw_walls(L, yaw);
        if (mv) { if (dpass == 2) PROF(6); for (int i = 0; i < nsim; i++) if (slv[i] == L) { gx_pid(8 + i); sim3d(i, yaw); }   // sims first: they always get their polygons
            if (dpass == 2) PROF(7);
            for (int i = 0; i < nobj; i++) if (OBJ_LV(&objs[i]) == L && ID2CH[OBJ_ID(&objs[i])] == 0xDC) gx_pid(16 + i % 40), obj3d(objs[i].x, objs[i].y, IT_OF(&objs[i]), objs[i].rot, 0); }   // (the hens peck about: they move)
        if (it) for (int i = 0; i < nobj; i++) {   // then the items, nearest the camera first in budget terms: far ones are skipped when the DS runs out of polygons
            if (OBJ_LV(&objs[i]) != L) continue;
            if (mode == M_LIVE) { int ox2 = tl(objs[i].x * 2) - camx, oz2 = tl(objs[i].y * 2) - camz; if (!on_board && abs(ox2) + abs(oz2) > 9000 && polys > 1800) continue; }
            if (polys > 2900) continue;
            gx_pid(16 + i % 40); obj3d(objs[i].x, objs[i].y, IT_OF(&objs[i]), objs[i].rot, 0); } }
    gy_off = 0; lod_force = 0; if (dpass == 2) PROF(6);
world:
    if (wd) { gx_pid(3); if (mode == M_LIVE) draw_world(yaw); else {   // (D36: the street round the plot after the house, so the house is never what the DS's polygon list drops)
        tx_want = TX_ASPHALT; ground(tl(-14), tl(-9), tl(40), tl(-2), 0, C15(7, 7, 9)); ground(tl(-14), tl(28), tl(40), tl(35), 0, C15(7, 7, 9)); tx_want = TX_PAVING;   // two proper streets, three cells wide
        block(tl(-14), tl(-2), tl(40), tl(0), 0, -14, C15(20, 20, 20), C15(24, 24, 24)); block(tl(-14), tl(26), tl(40), tl(28), 0, -14, C15(20, 20, 20), C15(24, 24, 24));   // pavements
        tx_want = -1; for (int x = -13; x < 40; x += 4) { ground(tl(x), tl(-6) - 14, tl(x) + T * 2, tl(-6) + 14, -6, C15(28, 26, 8)); ground(tl(x), tl(31) - 14, tl(x) + T * 2, tl(31) + 14, -6, C15(28, 26, 8)); }   // lane dashes (flat: one quad each, not five)
        tx_want = TX_GRASS; ground(tl(-14), tl(-9), tl(40), tl(35), 90, seas(C15(7, 16, 6))); tx_want = -1;   // grass all round, so the lot does not float
        if (polys < 2600) for (int i = 0; i < 5; i++) { tree(tl(-6 + (i * 7) % 5 * 1), tl(2 + i * 5), T * 2); tree(tl(GN * 2 + 3 - (i * 5) % 4), tl(1 + i * 5), T * 2); }
        if (o_look && gxb) { int X = (gx_far + 9) * 1024; if (X > 32000) X = 32000; gx_c0(0x11); gx_id(0x1B); gxb[gxn++] = 4 * 4096; gxb[gxn++] = 4096; gxb[gxn++] = 4 * 4096; gx_lv = 0; ground(-X, -X, X, X, 180, C15(7, 16, 6)); gx_c1(0x12, 1); gx_lv = 0; }   // (D40: land to the haze, not a lot floating in the sky)
        } }
    if (mv) { gx_pid(6); dog_draw(); ghost_draw(); }
    if (st) { gy_off = 0; gx_pid(4); roof_draw(top); }   // (D42: each building's own roof over its own top storey)
    if (mode == M_LIVE) { if (mv) { PROF(2); gx_pid(56); draw_city(); PROF(7); } if (wd) { gx_pid(3); draw_far_all(yaw); } }
    gy_off = -WHF * vl;
    if (mv && mode == M_LIVE && mark_t > 0) { ground(tl(mark_x * 2), tl(mark_y * 2), tl(mark_x * 2 + 2), tl(mark_y * 2 + 2), -60, C15(31, 31, 12)); mark_t--; }
    if (mv && (mode == M_BUY || mode == M_BUILD)) {   // the cursor
        ground(tl(pcx * 2), tl(pcy * 2), tl(pcx * 2 + 2), tl(pcy * 2 + 2), -80, C15(31, 31, 31));
        if (mode == M_BUY && pick >= 0) obj3d(pcx, pcy, &ITEMS[pick], prot, 1);
    }
    gy_off = 0;
}
static u32 world_sig(void) {   // (D41) what the world round the lot depends on
    u32 h = 2166136261u; h = (h ^ (u32)(mode == M_LIVE)) * 16777619u; h = (h ^ (u32)zone) * 16777619u; h = (h ^ (u32)lot_bx) * 16777619u; h = (h ^ (u32)lot_by) * 16777619u; h = (h ^ (u32)nb_ok) * 16777619u;
    h = (h ^ (u32)(ring_r | gx_far << 4 | o_look << 10 | o_out << 12 | lit_on << 13 | o_det << 14 | o_chk << 15 | o_tex << 16)) * 16777619u;
    h = (h ^ (u32)fm_season()) * 16777619u; for (int j = 0; j < 3; j++) for (int i = 0; i < 3; i++) h = (h ^ nb_pop[j][i]) * 16777619u; return h; }
static u32 still_sig(int yaw, int part) {   // (D39) everything a still list depends on: when this changes, it is recorded again (D41: part 0 the build, 1 the things)
    u32 h = 2166136261u + (u32)part;
    #define HM(v) h = (h ^ (u32)(v)) * 16777619u
    HM(fm_season()); HM(mode); HM(zone); HM(lot_bx); HM(lot_by); HM(nb_ok); HM(nobj); HM(roof_st); HM(tool); HM(ed_lv); HM(view_lv()); HM(door_cx + door_cy * 32); HM(lit_on); HM(lit_sgn);
    int show = show_lv(), top = top_lv(); HM(show); HM(top); if (part == 0) { HM(roof_bits()); HM(roof_st | lot_seed << 2); }
    HM(o_det | o_chk << 1 | o_shd << 2 | o_lit << 3 | o_roof << 4 | o_dd << 6 | o_sty << 8 | o_trf << 10 | o_look << 12 | o_out << 14 | ring_r << 16 | prof_full << 20 | o_tex << 22);
    { const u32 *w = (const u32 *)flr; for (unsigned k = 0; k < sizeof flr / 4; k++) HM(w[k]); }
    if (part == 0) { const u8 *q = &hwl[0][0]; for (unsigned k = 0; k < sizeof hwl; k++) HM(q[k]); q = &vwl[0][0]; for (unsigned k = 0; k < sizeof vwl; k++) HM(q[k]); }
    if (top) { const u8 *q = &uflr[0][0][0]; for (unsigned k = 0; k < sizeof uflr; k++) if (q[k]) HM(k * 256 + q[k]); q = &uhwl[0][0][0]; for (unsigned k = 0; k < sizeof uhwl; k++) if (q[k]) HM(k * 256 + q[k]); q = &uvwl[0][0][0]; for (unsigned k = 0; k < sizeof uvwl; k++) if (q[k]) HM(k * 256 + q[k]); }
    if (part == 1) for (int i = 0; i < nobj; i++) { const Obj *o = &objs[i]; HM(o->x | o->y << 8 | o->c << 16 | (u32)o->i << 24); HM(o->rot);
        if (mode == M_LIVE) { int ox2 = tl(o->x * 2) - camx, oz2 = tl(o->y * 2) - camz; HM((!on_board && abs(ox2) + abs(oz2) > 9000) | obj_far(o) << 1); } }
    for (int k = 0; k < 4; k++) HM(fm_n[k] | fm_g[k] << 8 | fm_dry[k] << 16); HM(fm_pest);
    for (int j = 0; j < 3; j++) for (int i = 0; i < 3; i++) HM(nb_pop[j][i]);
    if (part == 0) { wsig = &h; for (int L = show; L >= 0; L--) draw_walls(L, yaw); wsig = 0; }
    #undef HM
    return h;
}

// ============ the Sims-style game: LIVE (status on the top screen, the lot and its icons on the bottom, touch to play) ============
static const char *const NEEDN[6] = { "Hunger", "Fun", "Energy", "Social", "Bladder", "Hygiene" };
static int menu_sel, menu_open, popup, popup_i, speed = 1, yaw_t = 4096, live_yaw = 4096, live_dist = 12000, camx, camz, cas_tab, cas_done, cas_stage, cas_page;
enum { CS_BASICS, CS_LOOKS, CS_HAIR, CS_OUTFIT, CS_TRAITS, CS_DONE, CS_N };   // (D67) Create-a-Sim's six steps, as The Sims 3 has them
static int cas_step, cas_sub;


static void lot_go(int bx, int by) {   // (D37) the household goes to a lot: the one it was on is kept, the new one entered, everyone at its arrival spot
    lot_save(); lot_enter(bx, by); place_sims(); city_spawn(); nb_ok = 1; popup = 0; menu_open = 0; travel_ui = craft_ui = farm_ui = 0; cam_free = 0; set_mode(M_LIVE); life_ver++; }
// ---- CASTAWAY ISLAND (D37): gathering, the secrets, the crafts and the raft, as the PS1's (sim.c gather_done, island.c) ----
// ---- FARM LIFE (D38): the farm's day and what a tap on its things offers (the PS1's words and numbers, sentence case) ----
static const char *const FMN[6] = { "Wheat", "Corn", "Tomatoes", "Pumpkins", "Eggs", "Milk" };
static const u8 FM_PR[6] = { 4, 7, 9, 16, 5, 10 };   // (the PS1's market prices)
static u32 fm_rs = 12345;
static u32 fm_rand(void) { fm_rs = hsh(fm_rs + (u32)minutes * 2654435761u + 77u); return fm_rs; }
static int fm_need(int k) { static const u8 d[4] = { 2, 3, 3, 4 }; int n = d[k] - (fm_skl(ctl) >= 5 ? 1 : 0); return n < 2 ? 2 : n; }   // nights of watered growth to be ready
static int fm_have(int k) { return k < 4 ? fm_stock[k] : k == 4 ? fm_egg : fm_milk; }
static void fm_take(int k, int n) { if (k < 4) fm_stock[k] -= (u16)n; else if (k == 4) fm_egg -= (u16)n; else fm_milk -= (u16)n; }
static int fm_fest(void) { int mn = minutes % 1440; return (o_rt && rt_mon >= 0 ? (minutes / 1440) % 7 == 5 && rt_md <= 7 : st_days % 10 == 9) && mn >= 600 && mn < 1320; }   // (Real time: the month's first Saturday)   // (the PS1's festival: the tenth day of each season, 10am to 10pm)
static void fm_sk(int i, int v) { if (i >= 0 && i < SIM_MAX) fsk[i] = (u16)(fsk[i] + v > 60000 ? 60000 : fsk[i] + v); }
static char fm_say_b[4][48]; static int fm_say_k;
static void fm_say(const char *fmt, int v) { char *b = fm_say_b[fm_say_k++ & 3]; sprintf(b, fmt, v); say(b); }   // (say keeps the pointer: a few buffers in turn)
static void farm_midnight(void) {   // the PS1's farm_midnight: the crops grow where watered, wither after three dry nights, frost in winter, pests; the buyer's order; eggs and milk from fed animals
    int se = fm_season();
    if (fm_up & 1) fm_wet = 15;   // (a sprinkler; the DS has no rain)
    for (int k = 0; k < 4; k++) { if (!fm_n[k]) continue;
        if (se == 3 && !(fm_up & 4)) { fm_n[k] = 0; fm_g[k] = 0; fm_dry[k] = 0; fm_fz &= ~(1 << k); fm_pest &= ~(1 << k); say("Frost killed a crop. A greenhouse helps"); continue; }
        if ((fm_pest >> k) & 1) { fm_pest &= ~(1 << k); fm_n[k] = (u8)((fm_n[k] + 1) / 2); say("Pests ate half a crop"); }
        if (fm_g[k] >= fm_need(k)) continue;
        if ((fm_wet >> k) & 1) { fm_g[k]++; fm_dry[k] = 0; if (fm_g[k] >= fm_need(k)) say("Crops are ready to harvest"); }
        else if (++fm_dry[k] >= 3) { fm_n[k] = 0; fm_g[k] = 0; fm_dry[k] = 0; fm_fz &= ~(1 << k); say("A crop withered. Water it!"); } }
    fm_wet = 0; fm_wc = 0; fm_fair = 0;
    { int any = 0; for (int k = 0; k < 4; k++) if (fm_n[k]) any = 1;
      if (any && !(fm_up & 2) && (int)(fm_rand() % 100) < (se == 1 ? 24 : 12)) { int k = (int)(fm_rand() % 4u); if (fm_n[k] && !((fm_pest >> k) & 1)) { static char b[40]; fm_pest |= (u8)(1 << k); sprintf(b, "Pests are at the %s! Spray", FMN[k]); say(b); } } }
    if (fm_ordq && fm_ordt && --fm_ordt == 0) { fm_ordq = 0; say("The order expired"); }
    if (!fm_ordq) { fm_ordk = (u8)(fm_rand() % 6u); fm_ordq = (u8)(5 + fm_rand() % 8u); fm_ordt = 5; }
    if (fm_hens && (fm_fed & 1)) { int v = fm_pe + fm_hens * (1 + (fm_skl(ctl) >= 4)); fm_pe = (u16)(v > 60 ? 60 : v); }
    if (fm_cows && (fm_fed & 2)) { int v = fm_pm + fm_cows * (1 + (fm_skl(ctl) >= 6)); fm_pm = (u16)(v > 40 ? 40 : v); }
    fm_fed = 0;
}
static int fm_pay(int v) { if (funds < v) { say("Your wallet says no!"); return 0; } funds -= v; return 1; }
static void fm_work(int i, int o, int t, int need, int amt) {   // the PS1's begin_action at the thing: walk over and work there a while
    Sim *m = &sims[i]; if (o < 0 || o >= nobj) return; m->tx = objs[o].x * 16; m->ty = objs[o].y * 16; m->act = need; m->act_amt = amt; m->face = t; m->act_obj = o; m->act_left = 0;
}
enum { FC_PLANT = 100, FC_HARV = 104, FC_SPRAY, FC_WATER, FC_EGGS = 110, FC_HEN, FC_FEEDH, FC_MILK, FC_COW, FC_FEEDC, FC_SELL = 120, FC_ORDER, FC_FAIR, FC_FERT, FC_UP, FC_EAT, FC_LEDGER, FC_NOORDER };
static char fm_lab[8][28];
static int farm_acts(int c, const char **lab, int *code) {   // the PS1's choices for the crop bed, the hen coop, the cow and the farm stand (0: not a farm thing)
    int n = 0;
    #define FA(k, ...) do { sprintf(fm_lab[n], __VA_ARGS__); lab[n] = fm_lab[n]; code[n] = (k); n++; } while (0)
    if (c == 0xDB) { static const char *const cn[4] = { "Wheat $2", "Corn $4", "Tomatoes $5", "Pumpkin $8" }; int any = 0, rdy = 0;
        for (int k = 0; k < 4; k++) if (fm_n[k]) { any = 1; if (fm_g[k] >= fm_need(k)) rdy = 1; }
        if (rdy) FA(FC_HARV, "Harvest"); if (fm_pest) FA(FC_SPRAY, "Spray Pests $3"); if (any) FA(FC_WATER, "Water the Crops");
        for (int k = 0; k < 4 && n < 8; k++) FA(FC_PLANT + k, "Plant %s", cn[k]); }
    else if (c == 0xDC) { if (fm_pe) FA(FC_EGGS, "Collect %d Eggs", fm_pe); if (fm_hens < 6) FA(FC_HEN, "Buy a Hen $40"); if (fm_hens && !(fm_fed & 1)) FA(FC_FEEDH, "Feed the Hens $2"); }
    else if (c == 0xDD) { if (fm_pm) FA(FC_MILK, "Milk the Cows (%d)", fm_pm); if (fm_cows < 3) FA(FC_COW, "Buy a Cow $250"); if (fm_cows && !(fm_fed & 2)) FA(FC_FEEDC, "Feed the Cows $5"); }
    else if (c == 0xDE) { static const char *const upn[4] = { "Sprinkler $400", "Scarecrow $120", "Greenhouse $900", "Tractor $1500" };
        int tot = fm_stock[0] + fm_stock[1] + fm_stock[2] + fm_stock[3] + fm_egg + fm_milk;
        if (tot) FA(FC_SELL, "Sell Produce (%d)", tot);
        if (fm_ordq) FA(fm_have(fm_ordk) >= fm_ordq ? FC_ORDER : FC_NOORDER, "Order: %d %s", fm_ordq, FMN[fm_ordk]);
        if (fm_fest() && !fm_fair && fm_stock[3]) FA(FC_FAIR, "Enter the Fair");
        FA(FC_FERT, "Fertilizer $6");
        for (int k = 0; k < 4; k++) if (!((fm_up >> k) & 1)) { FA(FC_UP, "%s", upn[k]); break; }
        if (fm_stock[1] + fm_stock[2] + fm_stock[3] + fm_egg + fm_milk >= 2 && n < 7) FA(FC_EAT, "Eat a Fresh Meal");
        if (n < 8) FA(FC_LEDGER, "Farm Ledger"); }
    else return 0;
    if (!n) FA(-1, "Look");
    #undef FA
    return n;
}
static void farm_do(int code, int i, int o) {   // the PS1's PC_F... codes
    Sim *s = &sims[i]; static char b[48];
    if (code >= FC_PLANT && code < FC_PLANT + 4) { static const u8 cost[4] = { 2, 4, 5, 8 }; int k = code - FC_PLANT;
        if (fm_n[k] >= 8) { say("That field is full"); return; }
        if (fm_n[k] && fm_g[k] >= fm_need(k)) { say("Harvest those first"); return; }
        if (!fm_pay(cost[k])) return; fm_work(i, o, 150, 1, 3);
        if (!fm_n[k]) { fm_g[k] = 0; fm_dry[k] = 0; fm_fz &= ~(1 << k); }
        if (fm_fert && !((fm_fz >> k) & 1)) { fm_fert--; fm_fz |= (u8)(1 << k); fm_n[k]++; fm_sk(i, 12); say("Fertilized: a bigger harvest"); return; }
        fm_n[k]++; fm_sk(i, 12); say("Planted. Now water it"); return; }
    switch (code) {
    case FC_WATER: { int any = 0, soaked = 1; for (int k = 0; k < 4; k++) if (fm_n[k]) { any = 1; if (!((fm_wet >> k) & 1) && fm_g[k] < fm_need(k)) soaked = 0; }
        if (!any) { say("Nothing is planted"); return; }
        for (int k = 0; k < 4; k++) if (fm_n[k] && fm_g[k] < fm_need(k)) fm_wet |= (u8)(1 << k);
        fm_work(i, o, 200, 1, 3); s->need[2] = cl100(s->need[2] - 2);
        if (soaked) { fm_wc += 2; say("Soaked already. Too much water?"); } else { fm_wc++; fm_sk(i, 10); say("Watered. They grow at night"); } return; }
    case FC_HARV: { int lv = fm_skl(i), got = 0;
        for (int k = 0; k < 4; k++) if (fm_n[k] && fm_g[k] >= fm_need(k)) { int y = fm_n[k] * (2 + lv / 3) + (int)(fm_rand() % (u32)(fm_n[k] + 1)); if ((fm_fz >> k) & 1) y = y * 3 / 2; if (fm_up & 8) y = y * 3 / 2;
            fm_fz &= ~(1 << k); fm_stock[k] = (u16)(fm_stock[k] + y > 999 ? 999 : fm_stock[k] + y); got += y; sprintf(b, "%d %s picked", y, FMN[k]); say(b); fm_n[k] = 0; fm_g[k] = 0; fm_dry[k] = 0; }
        if (!got) { say("Nothing is ready"); return; }
        fm_work(i, o, 260, 1, 4); fm_sk(i, 30 + got * 3); s->need[2] = cl100(s->need[2] - 4); if (got >= 12) life_ev(i, WN_FUN, 0); return; }
    case FC_SPRAY: if (!fm_pest) { say("No pests"); return; } if (!fm_pay(3)) return; fm_pest = 0; fm_work(i, o, 120, 1, 2); fm_sk(i, 15); say("Sprayed: the pests are gone"); return;
    case FC_HEN: if (fm_hens >= 6) return; if (!fm_pay(40)) return; fm_hens++; say("A new hen! Feed it daily"); fm_sk(i, 15); return;
    case FC_COW: if (fm_cows >= 3) return; if (!fm_pay(250)) return; fm_cows++; say("A new cow! Moo"); fm_sk(i, 25); return;
    case FC_FEEDH: if (!fm_pay(2)) return; fm_fed |= 1; fm_work(i, o, 120, 1, 3); fm_sk(i, 8); say("The hens are fed: eggs by morning"); return;
    case FC_FEEDC: if (!fm_pay(5)) return; fm_fed |= 2; fm_work(i, o, 120, 1, 3); fm_sk(i, 10); say("The cows are fed: milk by morning"); return;
    case FC_EGGS: fm_egg = (u16)(fm_egg + fm_pe > 999 ? 999 : fm_egg + fm_pe); fm_say("%d eggs collected", fm_pe); fm_pe = 0; fm_sk(i, 15); return;
    case FC_MILK: fm_milk = (u16)(fm_milk + fm_pm > 999 ? 999 : fm_milk + fm_pm); fm_say("%d milked", fm_pm); fm_pm = 0; fm_work(i, o, 200, 1, 6); fm_sk(i, 20); return;
    case FC_SELL: { int tot = 0; for (int k = 0; k < 6; k++) tot += fm_have(k) * FM_PR[k] * (k == 3 && fm_season() == 2 ? 3 : 2) / 2; if (!tot) { say("Nothing to sell"); return; }
        tot = tot * (100 + fm_skl(i) * 4) / 100; funds += tot; memset(fm_stock, 0, sizeof fm_stock); fm_egg = fm_milk = 0; fm_say("Sold the produce: +$%d", tot); sfx(3); return; }   // (the PS1 adds charisma too: the DS has no such skill)
    case FC_NOORDER: say("Not enough for the order"); return;
    case FC_ORDER: { if (!fm_ordq || fm_have(fm_ordk) < fm_ordq) { say("Not enough for the order"); return; }
        int v = fm_ordq * FM_PR[fm_ordk] * 2; fm_take(fm_ordk, fm_ordq); funds += v; fm_ordq = 0; fm_ordt = 0; fm_sk(i, 25); fm_say("Order filled: +$%d", v); sfx(3); return; }
    case FC_FAIR: { if (!fm_stock[3] || fm_fair) { say("You have already entered"); return; } fm_stock[3]--; fm_fair = 1;
        int sc = 20 + fm_skl(i) * 8 + (fm_up & 8 ? 8 : 0) + (int)(fm_rand() % 40u), pz = sc >= 85 ? 500 : sc >= 60 ? 200 : 40; funds += pz; fm_sk(i, 40);
        fm_say(sc >= 85 ? "Best in show! +$%d" : sc >= 60 ? "Second prize! +$%d" : "A ribbon for trying +$%d", pz); if (sc >= 85) { life_ev(i, WN_FUN, 0); sfx(5); } return; }
    case FC_FERT: if (fm_fert >= 9) { say("The shed is full"); return; } if (!fm_pay(6)) return; fm_fert++; say("A bag of fertilizer"); return;
    case FC_UP: { static const int uc[4] = { 400, 120, 900, 1500 }; static const char *const un[4] = { "Sprinkler: auto water", "Scarecrow: no pests", "Greenhouse: no frost", "Tractor: bigger harvests" };
        int k = 0; while (k < 4 && ((fm_up >> k) & 1)) k++; if (k >= 4) return; if (!fm_pay(uc[k])) return; fm_up |= (u8)(1 << k); say(un[k]); return; }
    case FC_EAT: { int left = 2; for (int k = 3; k >= 1 && left; k--) while (left && fm_stock[k]) { fm_stock[k]--; left--; } while (left && fm_egg) { fm_egg--; left--; } while (left && fm_milk) { fm_milk--; left--; }
        fm_work(i, o, 150, 0, 40); say("Fresh from the farm!"); return; }
    case FC_LEDGER: farm_ui = 1; farm_tab = 0; if (fm_cday != (u8)st_days) { fm_cdone = 0; fm_cday = (u8)st_days; } if (!fm_built && !fm_last) fm_last = (u16)st_days; return;
    }
}
// ---- the farm ledger (farm_ui.c, FARM LIFE): build, market, contracts ----
#define FB_N 6
static const char *const FBNM[FB_N] = { "Barn", "Silo", "Orchard", "Beehives", "Farm Stand", "Windmill" };
static const char *const FBHINT[FB_N] = { "A roof for the animals. They thrive.", "Grain keeps. A steady income.", "Apples and pears every year.", "Honey, and the crops like the bees.", "Sell at better prices, and earn.", "The farm runs itself a little." };
static const int FBCOST[FB_N] = { 1500, 3000, 5000, 2000, 4000, 8000 }, FBINC[FB_N] = { 60, 110, 180, 80, 120, 300 }, FGPR[6] = { 4, 7, 9, 16, 5, 10 };
static int fl_rate(void) { int r = 0; for (int k = 0; k < FB_N; k++) if ((fm_built >> k) & 1) r += FBINC[k]; return r * (100 + fm_skl(ctl) * 8) / 100; }
static int fl_stand(void) { return (fm_built >> 4) & 1 ? 25 : 0; }
static int fl_days(void) { int d = st_days - fm_last; return d < 0 ? 0 : d > 30 ? 30 : d; }
static void fl_settle(void) { if (fm_built && st_days > fm_last) funds += fl_rate() * fl_days(); fm_last = (u16)st_days; }
static void fl_contract(int r, int *k, int *q) { u32 h = ((u32)st_days * 2654435761u + (u32)r * 40503u + 17u) >> 8; *k = (int)(h % 6u); *q = 3 + (int)((h >> 4) % 6u) + fm_skl(ctl) / 3; }
static int fl_rows(void) { return farm_tab == 0 ? FB_N + 1 : farm_tab == 1 ? 6 : 3; }
static int fl_sel = -1;
static void fl_act(int r) {
    static char b[48]; fl_sel = r;
    if (farm_tab == 0) {
        if (r == FB_N) { int d = fl_days(); if (!fm_built) { say("Build something first."); return; } if (!d) { say("Nothing new today."); return; } int pay = fl_rate() * d; fl_settle(); sprintf(b, "%d days of the farm: +$%d", d, pay); say(b); sfx(3); return; }
        if ((fm_built >> r) & 1) { say("Already built."); return; }
        if (funds < FBCOST[r]) { say("Not enough cash."); return; }
        fl_settle(); funds -= FBCOST[r]; fm_built |= (u16)(1 << r); fm_sk(ctl, 40); sprintf(b, "Built: %s", FBNM[r]); say(b); sfx(3); return; }
    if (farm_tab == 1) { int n = fm_have(r); if (!n) { say("None to sell."); return; }
        int pay = n * FGPR[r] * (100 + fl_stand()) / 100; fm_take(r, n); funds += pay; fm_sk(ctl, n); sprintf(b, "Sold %d %s: +$%d", n, FMN[r], pay); say(b); sfx(3); return; }
    { int k, q; fl_contract(r, &k, &q); if ((fm_cdone >> r) & 1) { say("Done today."); return; }
      if (fm_have(k) < q) { sprintf(b, "Need %d %s (have %d)", q, FMN[k], fm_have(k)); say(b); return; }
      int pay = q * FGPR[k] * 18 / 10; fm_take(k, q); funds += pay; fm_cdone |= (u8)(1 << r); fm_sk(ctl, q * 2); sims[ctl].need[1] = cl100(sims[ctl].need[1] + 10); sprintf(b, "Contract filled: +$%d", pay); say(b); sfx(3); }
}
static void farm_draw(void) {
    static const char *const TN[3] = { "Build", "Market", "Contracts" }; char b[56];
    panel(6, 20, 244, 168, "Farm Life"); chip(204, 22, 40, 14, "X", 0);
    for (int k = 0; k < 3; k++) chip(12 + k * 64, 40, 60, 14, TN[k], k == farm_tab);
    sprintf(b, "$%d", funds); text_box(110, 22, 90, 16, FS_NUM, C15(16, 31, 16), AL_R, b);
    int n = fl_rows();
    for (int r = 0; r < n; r++) { int y = 58 + r * 14; u16 bg = r == fl_sel ? C15(8, 16, 28) : (r & 1) ? C15(5, 11, 24) : C15(6, 13, 26); rect(12, y, 232, 13, bg);
        if (farm_tab == 0) {
            if (r == FB_N) { sprintf(b, "Collect  %d days = $%d", fl_days(), fl_rate() * fl_days()); text_box(15, y, 220, 13, FS_BODY, C15(16, 31, 16), AL_L, b); continue; }
            int own = (fm_built >> r) & 1; text_box(15, y, 150, 13, FS_BODY, own ? C15(16, 31, 16) : TXT, AL_L, FBNM[r]);
            if (own) text_box(150, y, 94, 13, FS_BODY, C15(16, 31, 16), AL_R, "Built"); else { sprintf(b, "$%d", FBCOST[r]); text_box(150, y, 94, 13, FS_NUM, funds >= FBCOST[r] ? TXT : C15(31, 12, 10), AL_R, b); }
        } else if (farm_tab == 1) { int c = fm_have(r); sprintf(b, "%s  x%d", FMN[r], c); text_box(15, y, 130, 13, FS_BODY, c ? TXT : C15(18, 20, 24), AL_L, b);
            sprintf(b, "$%d each", FGPR[r] * (100 + fl_stand()) / 100); text_box(130, y, 114, 13, FS_BODY, C15(16, 31, 16), AL_R, b);
        } else { int k, q; fl_contract(r, &k, &q); int dn = (fm_cdone >> r) & 1; sprintf(b, "%d %s", q, FMN[k]); text_box(15, y, 130, 13, FS_BODY, dn ? C15(16, 31, 16) : fm_have(k) >= q ? C15(20, 28, 31) : TXT, AL_L, b);
            if (dn) text_box(150, y, 94, 13, FS_BODY, C15(16, 31, 16), AL_R, "Done"); else { sprintf(b, "Pays $%d", q * FGPR[k] * 18 / 10); text_box(130, y, 114, 13, FS_BODY, C15(16, 31, 16), AL_R, b); } } }
    int by = 158; rect(12, by, 232, 26, C15(4, 9, 20));
    const char *l2;   // (the PS1's red line under the hint: what the last tap did, here in place of the hint while it shows)
    if (farm_tab == 0) { sprintf(b, "Farm level %d  $%d a day", fm_skl(ctl), fl_rate()); text_box(15, by, 226, 13, FS_BODY, C15(20, 28, 31), AL_L, b); l2 = fl_sel >= 0 && fl_sel < FB_N ? FBHINT[fl_sel] : "The farm earns every day. Come back and collect."; }
    else if (farm_tab == 1) { text_box(15, by, 226, 13, FS_BODY, C15(20, 28, 31), AL_L, fl_stand() ? "The farm stand adds 25% to every sale" : "Build a farm stand for better prices"); l2 = "Tap a row: sells all of that one."; }
    else { text_box(15, by, 226, 13, FS_BODY, C15(20, 28, 31), AL_L, "New buyers every day"); l2 = "They pay about twice the market price."; }
    if (msg_t > 0 && msg) text_box(15, by + 12, 226, 13, FS_BODY, C15(31, 22, 10), AL_L, msg); else text_box(15, by + 12, 226, 13, FS_BODY, TXT, AL_L, l2);
}
static void farm_touch(int tx, int ty) {
    if (hit(204, 22, 40, 14, tx, ty)) { farm_ui = 0; return; }
    for (int k = 0; k < 3; k++) if (hit(12 + k * 64, 40, 60, 14, tx, ty)) { farm_tab = k; fl_sel = -1; return; }
    int n = fl_rows(); for (int r = 0; r < n; r++) if (hit(12, 58 + r * 14, 232, 13, tx, ty)) { fl_act(r); return; }
}
static int fm_sig(void) { int a = fm_wet + fm_hens * 3 + fm_cows * 5 + fm_fed * 7 + fm_fert * 11 + fm_pest * 13 + fm_up * 17 + fm_ordq * 19 + fm_fair * 23 + fm_pe * 29 + fm_pm * 31 + fm_built * 37 + fm_cdone * 41 + farm_tab * 43 + fl_sel * 47 + fm_egg * 53 + fm_milk * 59;
    for (int k = 0; k < 4; k++) a = a * 7 + fm_n[k] * 3 + fm_g[k] + fm_stock[k] * 5; return a; }
static u8 ID2CH[NITEM];   // an item's PS1 letter (0: the DS's own)
static int popup_n, popup_code[8]; static const char *popup_lab[8];   // (D38: up to eight, the farm stand has that many)
static int in_shop(void); static int gtoy_of_(int si); static void gt_furn(int cd);   // (D53, ds_games.c)
static int obj_acts(int o, const char **lab, int *code) {   // what a tap on a thing offers: the PS1's choices on the island, else the item's own action
    int c = ID2CH[OBJ_ID(&objs[o])], n = 0, isl = zone == Z_ISLAND;
    #define AC(l, k) do { lab[n] = (l); code[n] = (k); n++; } while (0)
    if (isl && c == 'g') { AC("A Coconut", 60 + G_COCONUT); AC("Chop Wood", 60 + G_WOOD); AC("Gather Leaves", 60 + G_LEAVES); }
    else if (isl && (c == '3' || c == '6')) { AC("Chop Wood", 60 + G_WOOD); AC("Gather Leaves", 60 + G_LEAVES); }
    else if (isl && c == '*') AC("Gather Stone", 60 + G_STONE);
    else if (c == 0x89) { AC("Pick Berries", 60 + G_BERRIES); AC("Gather Leaves", 60 + G_LEAVES); }
    else if (c == 0x8A) AC("Fish", 60 + G_FISH);
    else if (c == 0x85) AC("Dig", 60 + G_DIG); else if (c == 0x87) AC("Search It", 60 + G_WRECK); else if (c == 0x84) AC("Read It", 60 + G_BOTTLE);
    else if (c == 0x8D) AC("Read It", 60 + G_JOURNAL); else if (c == 0x86) AC("Listen", 60 + G_IDOL); else if (c == 0x88) AC("Pick It Up", 60 + G_SHELL); else if (c == 0x8C) AC("Take It", 60 + G_GOLD);
    else if (isl && c == 's') { if (isl_food > 0) AC("Cook and Eat", 80); AC("Craft", 81); }
    else if (c >= 0xDB && c <= 0xDE) return farm_acts(c, lab, code);
    else if (c == 0x8B) { AC("Sail Across", 82); AC("Not Now", 84); }
    else if (c == '@' || c == '+') AC(IT_OF(&objs[o])->lab, 83);
    else if (c == 'B') AC("Ride the Board", 85);   // (D44)
    else if (c == '!' && in_arcade()) { static char pl[28]; int g_ = arcade_game_at(objs[o].x, objs[o].y); snprintf(pl, sizeof pl, "%s $%d", ag_title(g_), ag_price(g_)); AC(pl, 86); }   // (D46: a cabinet)
    else if (c == 'y' && in_arcade()) { static char pz[28]; snprintf(pz, sizeof pz, "Prizes (%d tickets)", arcade_tix); AC(pz, 87); }   // (D46: the prize counter)
    else if (c >= 0xBE && c <= 0xC5 && c != 0xC1 && c != 0xC2) {   // (D53) the Boreagotchi things (the PS1's pie choices)
        if (c == 0xBE && in_shop() && !gtoy_of_(ctl)) AC("Boreagotchi $5000", 96);
        if (c == 0xBE) { AC("Open the Basket", 90); AC("The Toy Box", 91); } else if (c == 0xBF) { AC("The Toy Box", 91); AC("Open the Basket", 90); } else if (c == 0xC0) { AC("View the Blobdex", 92); AC("The Toy Box", 91); }
        else if (c == 0xC3) AC("Tuck Them In", 93); else if (c == 0xC4) AC("Feed Them All $5", 94); else if (c == 0xC5) AC("Lights For Them", 95); else AC(IT_OF(&objs[o])->lab, -1); }
    else AC(IT_OF(&objs[o])->lab, -1);
    #undef AC
    return n;
}
static int gather_len(int k) { switch (k) { case G_WOOD: return 150; case G_STONE: return 120; case G_LEAVES: return 75; case G_BERRIES: return 60; case G_COCONUT: return 90; case G_FISH: return isl_spear ? 120 : 210; case G_DIG: return 180; case G_WRECK: return 150; case G_SHELL: return 15; default: return 100; } }   // (the PS1's, halved as the DS's actions are)
static void isl_done(int i, int k, int o) {   // a gathering finished (the PS1's gather_done)
    Sim *m = &sims[i]; static char t[48]; int here = o >= 0 && o < nobj && objs[o].x * 16 == m->x && objs[o].y * 16 == m->y, me = i == ctl;
    #define GONE() do { if (here) { objs[o] = objs[--nobj]; occ_ok = 0; } } while (0)
    switch (k) {
        case G_WOOD: isl_wood++; if (me) say("+1 Wood"); break;
        case G_STONE: isl_stone++; if (me) say("+1 Stone"); break;
        case G_LEAVES: isl_leaves += 2; if (me) say("+2 Leaves"); break;
        case G_BERRIES: isl_food++; m->need[0] = cl100(m->need[0] + 10); if (me) say("+1 Food: berries"); break;
        case G_COCONUT: isl_food++; m->need[0] = cl100(m->need[0] + 10); if (me) say("+1 Food: a coconut"); break;
        case G_FISH: { int n = isl_spear ? 3 : 2; isl_food += n; sprintf(t, "+%d Food: fish!", n); if (me) say(t); life_ev(i, WN_FUN, 0); break; }
        case G_DIG: if (here) { GONE(); funds += 2500; say("Treasure! A chest of $2500"); isl_found(SEC_TREASURE); } break;
        case G_WRECK: if (!((isl_secrets >> SEC_WRECK) & 1)) { funds += 500; say("In the wreck: $500 and a map. It shows an islet"); isl_found(SEC_WRECK); } else say("Nothing more in the wreck"); break;
        case G_BOTTLE: GONE(); say("A note: dig by the big rock"); isl_found(SEC_BOTTLE); break;
        case G_JOURNAL: say("The hermit: a raft reaches the islet. The gold is there"); isl_found(SEC_JOURNAL); break;
        case G_IDOL: say("The idol hums. You feel lucky"); m->need[1] = cl100(m->need[1] + 20); isl_found(SEC_IDOL); break;
        case G_SHELL: if (here) { GONE(); isl_shells++; sprintf(t, "Seashell %d of %d", isl_shells, ISL_SHELLS); say(t); if (isl_shells >= ISL_SHELLS) isl_found(SEC_SHELLS); } break;
        case G_GOLD: if (here) { GONE(); funds += 10000; say("The golden idol! $10000"); isl_found(SEC_GOLD); } break;
    }
    #undef GONE
    life_ver++;
}
typedef struct { const char *nm; u8 wood, stone, leaves; const char *blurb; } Craft;
static const Craft CRAFTS[6] = { { "Tent", 3, 0, 4, "A dry place to sleep" }, { "Log Bench", 2, 0, 0, "Sit by the fire" }, { "Outhouse", 5, 0, 0, "A little privacy" },
    { "Spear", 2, 1, 0, "Fish faster, catch more" }, { "Shelter", 12, 2, 8, "A wooden hut with a bed" }, { "Raft", 10, 0, 6, "Sail out to the islet" } };
static int has_ch(int ch) { for (int i = 0; i < nobj; i++) if (ID2CH[OBJ_ID(&objs[i])] == ch) return i; return -1; }
static int isl_can(int k) { const Craft *c = &CRAFTS[k]; if (k == 3 && isl_spear) return 0; if (k == 5 && has_ch(0x8B) >= 0) return 0; return isl_wood >= c->wood && isl_stone >= c->stone && isl_leaves >= c->leaves; }
static int wild_ch(int ch) { return ch == 'g' || ch == '3' || ch == '6' || ch == 0x89; }   // (trees and bushes: cleared to build, and the wood is yours)
static int spot_near(int cx, int cy, int w, int hh, int *ox, int *oy) {   // a w x h patch near (cx, cy) with nothing on it but trees and bushes: the fewest to clear, and close
    int best = 1 << 30; LV = 0;
    for (int y0 = 0; y0 + hh <= GN; y0++) for (int x0 = 0; x0 + w <= GN; x0++) { int cost = abs(x0 + w / 2 - cx) + abs(y0 + hh / 2 - cy), ok = 1;
        for (int y = y0; y < y0 + hh && ok; y++) for (int x = x0; x < x0 + w && ok; x++) { int o = obj_at(x, y); if (o >= 0) { if (!wild_ch(ID2CH[OBJ_ID(&objs[o])])) ok = 0; else cost += 5; } if (flr[y][x]) ok = 0; }
        for (int q = 0; q < nsim && ok; q++) { int sx = fdiv(sims[q].x + 8, 16), sy = fdiv(sims[q].y + 8, 16); if (sx >= x0 && sx < x0 + w && sy >= y0 && sy < y0 + hh) ok = 0; }
        if (ok && cost < best) { best = cost; *ox = x0; *oy = y0; } }
    return best < (1 << 30);
}
static void clear_ground(int x0, int y0, int w, int hh) { int n = 0; for (int y = y0; y < y0 + hh; y++) for (int x = x0; x < x0 + w; x++) { int o = obj_at(x, y); if (o >= 0 && wild_ch(ID2CH[OBJ_ID(&objs[o])])) { objs[o] = objs[--nobj]; occ_ok = 0; n++; } }
    if (n) { static char t[40]; isl_wood += n; sprintf(t, "Cleared the ground: +%d wood", n); say(t); } }
static void isl_make(int k) {   // the PS1's isl_make: the thing goes near the camp's fire (a hut is built round its bed; the raft goes on the shore)
    if (!isl_can(k)) { say("Not enough to make that"); return; }
    int f = has_ch('s'), fx = f >= 0 ? objs[f].x : 6, fy = f >= 0 ? objs[f].y : 6, x, y, ok = 1; const Craft *c = &CRAFTS[k];
    switch (k) {
        case 0: case 1: case 2: ok = spot_near(fx, fy, 1, 1, &x, &y); if (ok) { clear_ground(x, y, 1, 1); z_put(x, y, k == 0 ? 'z' : k == 1 ? '%' : '&', 0); } break;
        case 3: isl_spear = 1; break;
        case 4: ok = spot_near(fx, fy, 5, 5, &x, &y); if (ok) { clear_ground(x, y, 5, 5); z_room(x, y, x + 5, y + 5, 1, 2); hwl[y + 5][x + 2] = 5; vwl[y + 2][x] = 0; z_put(x + 1, y + 1, 'z', 0); z_put(x + 3, y + 1, '%', 0); } break;   // (planks, wood walls, a door to the south)
        case 5: ok = 0; for (int xx = GN - 1; xx >= 0 && !ok; xx--) for (int yy = GN - 1; yy >= GN - 3 && !ok; yy--) if (obj_at(xx, yy) < 0) { z_put(xx, yy, 0x8B, 1); ok = 1; } break;   // (on the shore)
    }
    if (!ok) { say("No room near the fire"); return; }
    isl_wood -= c->wood; isl_stone -= c->stone; isl_leaves -= c->leaves; static char t[40]; sprintf(t, k == 5 ? "Made: Raft! It is on the shore" : "Made: %s!", c->nm); say(t); sfx(3); life_ver++; ui_ver++;
}
static void raft_sail(void) {   // across to the islet, or back to the camp: everyone comes (and a raft waits at each end)
    if (lot_bx == zbx(Z_ISLAND, 4)) { lot_go(zbx(Z_ISLAND, 1), ZB + 1); say("Back at the camp"); return; }
    lot_go(zbx(Z_ISLAND, 4), ZB + 1); say("Sailed to the islet"); isl_found(SEC_ISLET);
}
static void travel_draw(void) {   // where to: the PS1's rows, the locked ones greyed with what opens them
    static const char *const blurb[Z_N] = { "", "House, factory, rail park", "Survive, build, find 8 secrets", "Diner, trailer and the lab" }, *const lock[Z_N] = { "", "", "Earn your first paycheck", "Get promoted twice" };
    char t[48]; panel(6, 20, 244, 164, "Where to?"); chip(204, 22, 40, 14, "X", 0);
    for (int r = 0; r < 5; r++) { int y = 42 + r * 27, z = r == 0 ? Z_CITY : r == 4 ? -1 : r, open_ = z < 0 || zone_open(z), here = (r == 0 && zone == Z_CITY && lot_bx == home_bx && lot_by == home_by) || (r > 0 && r < 4 && zone == z);
        rect(12, y, 232, 25, open_ ? C15(5, 12, 26) : C15(10, 12, 18)); rect(12, y, 232, 1, C15(10, 18, 30));
        text_box(15, y + 1, 150, 12, FS_HEAD, open_ ? TXT : C15(20, 22, 26), AL_L, r == 0 ? "Home" : r == 4 ? "San Boreas" : ZNAME[z]);
        if (r == 4) { sprintf(t, "%d towns: the Map, a taxi anywhere", bm_nset()); text_box(15, y + 12, 226, 12, FS_BODY, C15(22, 26, 31), AL_L, t); }
        else text_box(15, y + 12, 226, 12, FS_BODY, here ? C15(16, 31, 16) : open_ ? C15(22, 26, 31) : C15(31, 14, 12), AL_L, here ? "You are here" : r == 0 ? "Your house in San Boreas" : open_ ? blurb[z] : lock[z]); }
}
static void travel_touch(int tx, int ty) {
    if (hit(204, 22, 40, 14, tx, ty)) { travel_ui = 0; return; }
    for (int r = 0; r < 5; r++) if (hit(12, 42 + r * 27, 232, 25, tx, ty)) { int z = r == 0 ? Z_CITY : r;
        if (r == 4) { travel_ui = 0; set_mode(M_MAP); return; }
        if (!zone_open(z)) { say(z == Z_ISLAND ? "Earn your first paycheck" : "Get promoted twice"); return; }
        travel_ui = 0; if (z == zone && z != Z_CITY) { say("You are here"); return; } travel_go(z); return; }
}
static void craft_draw(void) {
    char t[56]; panel(6, 20, 244, 164, "Craft"); chip(204, 22, 40, 14, "X", 0);
    sprintf(t, "Wood %d Stone %d Leaves %d Food %d", isl_wood, isl_stone, isl_leaves, isl_food); text_box(9, 36, 190, 14, FS_BODY, C15(31, 31, 20), AL_L, t);
    for (int k = 0; k < 6; k++) { const Craft *c = &CRAFTS[k]; int y = 52 + k * 21, can = isl_can(k);
        rect(12, y, 232, 19, can ? C15(5, 12, 26) : C15(10, 12, 18)); text_box(15, y, 80, 19, FS_HEAD, can ? TXT : C15(20, 22, 26), AL_L, c->nm);
        if (c->stone) sprintf(t, "%dW %dS %dL", c->wood, c->stone, c->leaves); else if (c->leaves) sprintf(t, "%dW %dL", c->wood, c->leaves); else sprintf(t, "%d wood", c->wood);
        text_box(95, y, 100, 10, FS_SMALL, C15(22, 26, 31), AL_L, t); text_box(95, y + 9, 100, 10, FS_SMALL, C15(18, 22, 28), AL_L, c->blurb); chip(198, y + 2, 42, 15, (k == 3 && isl_spear) ? "Have" : (k == 5 && has_ch(0x8B) >= 0) ? "Made" : "Make", can); }
}
static void craft_touch(int tx, int ty) {
    if (hit(204, 22, 40, 14, tx, ty)) { craft_ui = 0; return; }
    for (int k = 0; k < 6; k++) if (hit(198, 54 + k * 21, 42, 15, tx, ty)) { isl_make(k); return; }
}
static void panel(int x, int y, int w, int h, const char *title) {
    rect(x + 1, y, w - 2, 2, PANB); rect(x + 1, y + h - 2, w - 2, 2, PANB); rect(x, y + 1, 2, h - 2, PANB); rect(x + w - 2, y + 1, 2, h - 2, PANB); rect(x + 2, y + 2, w - 4, h - 4, PAN);   // (D70) no pixel is drawn twice: it was 3 times the memory writes
    if (title) { rect(x + 2, y + 2, w - 4, 16, PANH); text_box(x + 5, y + 2, w - 40, 16, FS_HEAD, TXT, AL_L, title); }   // (D65) centred up and down in the header
}
static void backdrop_y(int y0, int y1);
static void backdrop(void) { backdrop_y(0, 192); }
static void backdrop_y(int y0, int y1) {   // (D70) the paper with dots
    if (y1 > y0) dr_mark(y0, y1 - y0);   // (three row patterns built once, then a row is a copy of one of them)
    static u16 rw[3][256]; static int ok;
    if (!ok) { ok = 1; for (int x = 0; x < 256; x++) rw[0][x] = rw[1][x] = rw[2][x] = BG_L;
        for (int x = 0; x < 256; x += 32) for (int k = 0; k < 4; k++) { rw[1][x + 6 + k] = C15(22, 28, 31); rw[2][(x + 16 + 6 + k) & 255] = C15(22, 28, 31); } }
    for (int y = y0; y < y1; y++) { int r = y & 15; const u32 *a = (const u32 *)rw[r >= 6 && r < 10 ? 1 + ((y >> 4) & 1) : 0]; u32 *d = (u32 *)(scr + y * 256); for (int i = 0; i < 128; i++) d[i] = a[i]; }
}
int isq(int v); u16 lit(u16 c, int k);
static void ellf(int cx, int cy, int rx, int ry, u16 c) {   // a filled oval
    if (rx < 1 || ry < 1) return;
    for (int y = -ry; y <= ry; y++) { int hw = isq(rx * rx * (ry * ry - y * y) / (ry * ry)); rect(cx - hw, cy + y, 2 * hw + 1, 1, c); }
}
enum { EX_NEU, EX_HAPPY, EX_SAD, EX_ANGRY, EX_TIRED, EX_BORED, EX_SCARED, EX_LOVE, EX_SLEEP, EX_N };   // (D55) the faces' moods
static inline u16 br0(u16 hc) { return shade(hc, 6); }
#ifdef DSTEST
volatile int test_ex;   // (test builds only: 1..9 forces every face's mood (0 is the game's own), 101 draws the contact sheet of all of them)
#endif
static int expr_of(const Sim *m) {   // the face's mood from what the sim does and how its needs stand (portraits and the 3D model agree)
#ifdef DSTEST
    if (test_ex >= 1 && test_ex <= EX_N) return test_ex - 1;
#endif
    int i = (int)(m - sims); if (i < 0 || i >= nsim) return EX_NEU;
    if (m->act == 2 && m->tx == m->x && m->ty == m->y) return EX_SLEEP;
    if (m->act >= 100 && m->soc >= 0) { if (m->soc == 2 || m->soc == 6 || m->soc == 7 || m->soc == 8) return EX_LOVE; if (m->soc == 5) return EX_ANGRY; }
    int s = 0; for (int n = 0; n < 6; n++) s += m->need[n]; s /= 6;
    if (m->need[0] < 16) return EX_ANGRY; if (m->need[4] < 14) return EX_SCARED; if (m->need[2] < 22) return EX_TIRED; if (m->need[1] < 22) return EX_BORED;
    return s < 38 ? EX_SAD : s > 64 ? EX_HAPPY : EX_NEU;
}
static int face_hat = -1, face_bg = -1;   // (D67) Create-a-Sim's tiles: a hat of their own, and a mid-blue ground so white hair and black hair both show   // (D67) Create-a-Sim's tiles draw a copy of the sim with a hat of their own: -1 = the sim's own
static void face(int x, int y, int sz, const Sim *m) {   // a head and shoulders portrait: flat colours, small crisp details
    int u = sz / 16; if (u < 1) u = 1; int cx = x + sz / 2;
    u16 sk = skin_of(m), hc = hair_of(m), sh = shirt_of(m), skd = shade(sk, 6);
    rect(x, y, sz, sz, face_bg >= 0 ? (u16)face_bg : C15(22, 22, 24));
    rect(x + u, y + 13 * u, 14 * u, sz - 13 * u, sh); rect(x + u, y + 13 * u, 14 * u, u, shade(sh, 6));          // shoulders
    rect(cx - 2 * u, y + 11 * u, 4 * u, 3 * u, skd);                                                            // neck
    int hw = (m->fc == 1 ? 5 : 4) * u, ht = (m->fc == 2 ? 10 : 9) * u, hx = cx - hw, hy = y + 2 * u + (m->fc == 2 ? 0 : u);
    int st = hair2d(m->hair);
    if (st == 8) rect(cx - 7 * u, hy - 2 * u, 14 * u, 9 * u, shade(hc, 7));                                     // afro, behind
    if (st == 3 || st == 1 || st == 11) rect(hx - u, hy + u, 2 * hw + 2 * u, ht - u * (st == 1 ? 3 : 0) + (st == 3 ? 4 * u : 0), shade(hc, 7));
    if (st == 9) rect(cx - u, hy + 2 * u, 2 * u, 9 * u, shade(hc, 7));
    rect(hx, hy, 2 * hw, ht, sk); rect(hx, hy + ht - u, 2 * hw, u, skd); rect(hx - u, hy + 3 * u, u, 2 * u, skd); rect(hx + 2 * hw, hy + 3 * u, u, 2 * u, skd);   // head and ears
    if (m->hair != 5) {
        if (m->hair == 14 || m->hair == 23) rect(cx - 2 * u, hy - 3 * u, 4 * u, 3 * u, hc);   // bun, top knot
        if (st == 7) { rect(cx - u, hy - 2 * u, 2 * u, 4 * u, hc); rect(hx, hy, 2 * hw, u, shade(hc, 8)); }
        else if (st == 6) rect(hx, hy, 2 * hw, u + u / 2, hc);
        else { rect(hx - (st == 8 ? u : 0), hy - u, 2 * hw + (st == 8 ? 2 * u : 0), 3 * u, hc); rect(hx, hy + 2 * u, u, 3 * u, hc); rect(hx + 2 * hw - u, hy + 2 * u, u, 3 * u, hc); }
        if (st == 2) for (int k = 0; k < 5; k++) rect(hx + k * (2 * hw - u) / 4, hy - 3 * u, u, 2 * u, hc);
        if (st == 4) { for (int k = 0; k < 5; k++) rect(hx + k * (2 * hw - u) / 4, hy - 2 * u, u, 2 * u, shade(hc, 7)); rect(hx - u, hy + 3 * u, u, 6 * u, hc); rect(hx + 2 * hw, hy + 3 * u, u, 6 * u, hc); }
        if (st == 10) rect(hx, hy + 2 * u, 2 * hw, u, hc);
        if (st == 11) { rect(hx - 3 * u, hy + 3 * u, 3 * u, 7 * u, hc); rect(hx + 2 * hw, hy + 3 * u, 3 * u, 7 * u, hc); }
        if (st == 3) { rect(hx - u, hy + 3 * u, 2 * u, 9 * u, hc); rect(hx + 2 * hw - u, hy + 3 * u, 2 * u, 9 * u, hc); } }
    { int hat = face_hat >= 0 ? face_hat : (m >= sims && m < sims + SIM_MAX) ? csx[m - sims].hat : 0, w2 = 2 * hw;   // (D67) the hat
      switch (hat) {
        case 1: rect(hx - u, hy - 2 * u, w2 + 2 * u, 4 * u, C15(26, 6, 6)); rect(hx - u, hy + 2 * u, w2 + 4 * u, u, C15(18, 3, 4)); break;
        case 2: rect(hx - u, hy - 3 * u, w2 + 2 * u, 5 * u, C15(8, 12, 28)); rect(hx - u, hy + u, w2 + 2 * u, u, C15(5, 8, 22)); rect(cx - u, hy - 4 * u + (u > 1 ? 0 : 1), 2 * u, u, C15(31, 31, 31)); break;
        case 3: rect(hx - 2 * u, hy, w2 + 4 * u, u, C15(3, 3, 5)); rect(hx, hy - 3 * u, w2, 3 * u, C15(3, 3, 5)); rect(hx, hy - u, w2, u, C15(24, 4, 6)); break;
        case 4: rect(cx - 2 * u, hy - 2 * u, 4 * u, 2 * u, C15(31, 12, 22)); rect(cx - u, hy - 3 * u, 2 * u, u, C15(31, 26, 6)); break;
        case 5: rect(hx, hy - u, w2, 2 * u, C15(31, 25, 4)); rect(hx, hy - 2 * u, u, u, C15(31, 25, 4)); rect(cx - u / 2, hy - 2 * u, u, u, C15(31, 25, 4)); rect(hx + w2 - u, hy - 2 * u, u, u, C15(31, 25, 4)); break;
        default: break; } }
    int ex = expr_of(m), ey = hy + (m->fc == 2 ? 4 : 3) * u + u + (m->pm[3] - 50) * u / 40, er = m->fc == 3 ? 2 : 1; if (ex == EX_SCARED) er++;
    const u16 EW = C15(31, 31, 31), EP = C15(5, 3, 2), LP = C15(22, 7, 8), PK = C15(31, 11, 16); const int th = u > 1 ? u / 2 : 1;
    int bt = (m->pm[6] - 50) / 30, rise = 0; if (ex == EX_SAD || ex == EX_TIRED) bt += u; else if (ex == EX_ANGRY) bt -= u; else if (ex == EX_SCARED) { bt += u; rise = u; } else if (ex == EX_HAPPY || ex == EX_LOVE) rise = u > 1 ? u / 2 : 0;
    for (int sd = 0; sd < 2; sd++) { int x0 = sd ? cx + u : cx - 3 * u, pxx = sd ? x0 : x0 + u;   // the eyes (pupils look to the middle), the brow over each
        if (ex == EX_SLEEP) { rect(x0, ey + er * u - th, 2 * u, th, C15(5, 3, 3)); if (u > 1) { px(x0, ey + er * u - th - 1, C15(5, 3, 3)); px(x0 + 2 * u - 1, ey + er * u - th - 1, C15(5, 3, 3)); } }
        else { if (ex == EX_TIRED || ex == EX_BORED) { if (u > 1) { rect(x0, ey, 2 * u, u * er, EW); rect(x0, ey, 2 * u, (u * er + 1) / 2 + (ex == EX_TIRED ? 1 : 0), skd); } rect(pxx, ey + (u > 1 ? u * er / 2 : 0), u, u > 1 ? (u * er + 1) / 2 : 1, EP); if (ex == EX_TIRED) rect(x0, ey + u * er + 1, 2 * u, 1, shade(skd, 6)); }
            else { rect(x0, ey, 2 * u, u * er, EW); u16 pc = ex == EX_LOVE ? C15(30, 5, 10) : EP; int pw = ex == EX_SCARED && u > 1 ? u / 2 + 1 : u, ph = er > 1 && ex != EX_SCARED ? 2 * u : pw; rect(sd ? x0 : x0 + 2 * u - pw, ey + (er * u - ph) / 2, pw, ph, pc);
                if (ex == EX_LOVE && u > 1) px(sd ? x0 : x0 + 2 * u - 1, ey + (er * u - ph) / 2, EW);
                if (ex == EX_ANGRY && u > 1) rect(x0, ey, 2 * u, 1, skd); }
            if (m->fc == 4 && ex != EX_TIRED && ex != EX_BORED) rect(x0, ey, 2 * u, u / 2 + 1, skd); }
        int by = ey - u - rise - (u > 1 ? (m->pm[7] - 50) / 25 : 0), yo = by, yi = by - bt; rect(sd ? x0 + u : x0, yo, u, th, br0(hc)); rect(sd ? x0 : x0 + u, yi, u, th, br0(hc));
        if (ex == EX_LOVE || (ex == EX_HAPPY && u > 1)) rect(sd ? x0 + u : x0 - u / 2, ey + 2 * u, u + (u > 1 ? u / 2 : 0), u > 1 ? u / 2 + 1 : 1, PK); }
    rect(cx - u / 2, ey + 2 * u, u, 2 * u, skd);
    if (m->fc == 5) for (int k = 0; k < 3; k++) { px(cx - 3 * u + k * u, ey + 3 * u, shade(sk, 5)); px(cx + u + k * u, ey + 3 * u, shade(sk, 5)); }
    int my = ey + 5 * u;
    if (m->beard == 1) rect(hx, my - u, 2 * hw, hy + ht - my + u, lit(sk, 11));
    if (m->beard >= 4) rect(hx, my - 2 * u, 2 * hw, hy + ht - my + 2 * u + (m->beard == 5 ? 3 * u : 0), shade(hc, 8));
    if (m->beard == 2 || m->beard == 3) rect(cx - 3 * u, my - u, 6 * u, u, shade(hc, 8));
    if (m->beard == 3) rect(cx - u, my + u, 2 * u, 2 * u, shade(hc, 8));
    { int mw = 3 * u * (50 + m->pm[0]) / 100, my2 = my + (50 - m->pm[1]) * u / 40; if (mw < 3) mw = 3; if (ex == EX_HAPPY || ex == EX_LOVE || ex == EX_SAD) { if (mw < 4 * u) mw = 4 * u; }   // the mouth by mood
        int x0 = cx - mw / 2;
        switch (ex) {
        case EX_HAPPY: case EX_LOVE: rect(x0 + u, my2, mw - 2 * u, u, LP); rect(x0, my2 - u, u, u, LP); rect(x0 + mw - u, my2 - u, u, u, LP); if (u > 1 && mw >= 4 * u) rect(x0 + u, my2 - th, mw - 2 * u, th, EW); break;
        case EX_SAD: rect(x0 + u, my2, mw - 2 * u, u, LP); rect(x0, my2 + u, u, u, LP); rect(x0 + mw - u, my2 + u, u, u, LP); if (u > 1) px(hx + u + 1, ey + 3 * u + u / 2, C15(14, 22, 31)); break;
        case EX_ANGRY: rect(x0, my2, mw, u, C15(14, 3, 5)); rect(x0, my2 + (u > 1 ? u : 0), u, 1, C15(14, 3, 5)); rect(x0 + mw - u, my2 + (u > 1 ? u : 0), u, 1, C15(14, 3, 5)); break;
        case EX_TIRED: rect(cx - u, my2, 2 * u, 2 * u, C15(8, 1, 2)); rect(cx - u, my2, 2 * u, 1, LP); if (u > 1) rect(cx - u, my2 + 2 * u - 1, 2 * u, 1, LP); break;
        case EX_BORED: rect(x0 + u, my2, mw * 2 / 3, u, LP); break;
        case EX_SCARED: rect(cx - u, my2 - u / 2, 2 * u + (u > 1 ? 0 : 1), 2 * u, C15(8, 1, 2)); if (u > 1) { rect(cx - u, my2 - u / 2, 2 * u, 1, LP); rect(cx - u, my2 + 3 * u / 2 - 1, 2 * u, 1, LP); } break;
        case EX_SLEEP: rect(cx - u, my2, 2 * u, u, shade(LP, 6)); if (u > 1) { int zx = hx + 2 * hw + u, zy = hy - u; rect(zx, zy, 3, 1, EW); px(zx + 2, zy + 1, EW); px(zx + 1, zy + 2, EW); rect(zx, zy + 3, 3, 1, EW); } break;
        default: rect(x0 + (mw > 3 ? u / 2 : 0), my2, mw, u, LP); } }
}
static int mood(const Sim *m) { int s = 0; for (int n = 0; n < 6; n++) s += m->need[n]; return s / 6; }
static u16 need_col(int v) { return v > 55 ? C15(8, 26, 8) : v > 28 ? C15(28, 24, 6) : C15(30, 8, 6); }
static const char *DAYN[7] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
static const char *relname(int r) { return r < 3 ? "Acquaintance" : r < 6 ? "Friend" : "Best Friend"; }
// ---- LIFE: jobs, wants and fears, the jukebox screen (D31) ----
static const char *const JOBNM[JOBN] = { "Line Cook", "Shop Clerk", "Lifeguard", "Barista", "Teacher", "Office Temp" };
static const char *const JOBLV[JOBN][5] = {
    { "Dishwasher", "Prep Cook", "Line Cook", "Sous Chef", "Head Chef" }, { "Stock Boy", "Cashier", "Sales Rep", "Supervisor", "Manager" },
    { "Pool Boy", "Lifeguard", "Swim Coach", "Head Guard", "Pool Boss" }, { "Cup Washer", "Barista", "Latte Artist", "Shift Lead", "Cafe Owner" },
    { "Aide", "Tutor", "Teacher", "Dept Head", "Principal" }, { "Intern", "Temp", "Analyst", "Manager", "Director" } };
static const u8 JOBPAY[JOBN] = { 8, 7, 9, 7, 10, 12 };
static int job_pay(const Life *L) { return L->job ? JOBPAY[L->job - 1] * 10 * (L->lvl + 1) : 0; }
static const char *const WNM[WN_N] = { "", "Eat a meal", "Sleep in a bed", "Take a shower", "Have some fun", "Chat with someone", "Make a friend", "Get a job", "Go to work", "Get paid", "Get promoted", "Have $", "Buy something", "Visit another lot", "Have a baby", "Pet the dog", "Pick a song" };
static const u16 WPT[WN_N] = { 0, 300, 300, 300, 300, 400, 900, 800, 500, 500, 1500, 1200, 400, 500, 3000, 400, 300 };
static const char *const FNM[FR_N] = { "", "Pass out", "Wet myself", "A bad day at work", "Get demoted", "Meet a ghost" };
static const u16 FPT[FR_N] = { 0, 600, 500, 700, 1000, 400 };
static int want_ok(int i, int w) {   // may this sim want that, now?
    const Sim *m = &sims[i]; const Life *L = &life[i]; int st = stage_of(m->age);
    if (st == 0) return 0;
    switch (w) {
        case WN_JOB: return st >= 3 && !L->job; case WN_WORK: case WN_PAID: return st >= 3 && L->job; case WN_PROMO: return L->job && L->lvl < 4;
        case WN_CASH: case WN_BUY: return st >= 3; case WN_BABY: return st == 4 && nsim < SIM_MAX; case WN_PET: return dog_have;
        case WN_FRIEND: case WN_VISIT: return st >= 2; case WN_SONG: return mus_ok == 1;
        default: return 1; }
}
static void want_roll(int i, int k) {
    Life *L = &life[i];
    for (int t = 0; t < 24; t++) {
        int w = 1 + (int)(hsh((u32)(minutes * 31 + i * 977 + k * 131 + t * 17 + life_ver)) % (WN_N - 1)); if (!want_ok(i, w)) continue;
        int dup = 0; for (int q = 0; q < 3; q++) if (L->wk[q] == w) dup = 1; if (dup) continue;
        int arg = 0; if (w == WN_CASH) { arg = (funds / 1000 + 1) * 1000; if (arg > 30000) continue; }
        L->wk[k] = (u8)w; L->wa[k] = (s16)arg; life_ver++; return; }
}
static void fear_roll(int i) {
    Life *L = &life[i];
    for (int t = 0; t < 8; t++) { int f = 1 + (int)(hsh((u32)(minutes * 13 + i * 71 + t * 5 + life_ver)) % (FR_N - 1)); if ((f == FR_DEMOTE || f == FR_BAD) && !L->job) continue; if (f != L->fear) { L->fear = (u8)f; life_ver++; return; } }
}
static void life_gain(int i, int k) {
    Life *L = &life[i]; int w = L->wk[k], pts = WPT[w], old = L->pts / 1500; L->pts = (u16)(L->pts + pts > 60000 ? 60000 : L->pts + pts); L->wk[k] = 0; life_ver++;
    if (i == ctl) { static char b[40]; if (L->pts / 1500 > old) { int bonus = 250 * (L->pts / 1500); funds += bonus; sprintf(b, "Aspiration up! +$%d", bonus); } else sprintf(b, "Want done! +%d", pts); say(b); sfx(3); }
}
static void life_ev(int i, int ev, int arg) { (void)arg; if (i < 0 || i >= nsim) return; Life *L = &life[i]; for (int k = 0; k < 3; k++) if (L->wk[k] == ev) { life_gain(i, k); break; } }
static void life_fear(int i, int fr) {
    Life *L = &life[i]; if (i < 0 || i >= nsim || L->fear != fr) return;
    L->pts = L->pts > FPT[fr] ? (u16)(L->pts - FPT[fr]) : 0; if (i == ctl) say("A fear came true..."); fear_roll(i); life_ver++;
}
static void life_tick(void) {
    for (int i = 0; i < nsim; i++) { Life *L = &life[i]; if (stage_of(sims[i].age) == 0) continue;
        if (csx[i].wish && !csx[i].wdone) { int w = csx[i].wish - 1, ok = w == 0 ? funds >= 20000 : w == 1 ? (L->job && L->lvl >= 2) : w == 2 ? nown >= 3 : w == 3 ? nsim >= 5 : (fight_champ & 1);   // (D67) the lifetime wish picked in Create-a-Sim: done once, pays $2000
            if (ok) { csx[i].wdone = 1; funds += 2000; static char wb[40]; sprintf(wb, "Wish done! +$2000"); say(wb); life_ver++; } }
        for (int k = 0; k < 3; k++) { if (!L->wk[k]) want_roll(i, k); else if (L->wk[k] == WN_CASH && funds >= L->wa[k]) life_gain(i, k); else if (!want_ok(i, L->wk[k])) { L->wk[k] = 0; life_ver++; } }
        if (!L->fear) fear_roll(i); }
}
static void life_reset(void) { memset(life, 0, sizeof life); memset(gh, 0, sizeof gh); ngh = gh_next = 0; dog_have = 0; dog.hun = 80; life_ver++; roof_st = up_st = 0; jobui = juke = 0; info_pg = 0; }
static void job_take(int j) {
    Life *L = &life[ctl]; if (stage_of(sims[ctl].age) < 3) { say("Too young to work"); return; }
    L->job = (u8)(j + 1); L->lvl = L->prog = L->strike = 0; life_ver++; static char b[40]; sprintf(b, "Hired: %s", JOBLV[j][0]); life_ev(ctl, WN_JOB, 0); say(b);
}
static void job_work(void) {
    Life *L = &life[ctl]; static char b[44]; int hr = (minutes / 60) % 24, dw = (minutes / 1440) % 7, dn = (minutes / 1440) % 250 + 1;
    if (!L->job) { say("Get a job first: Menu > Job"); return; }
    if (dw >= 5) { say("Weekend! No shift today"); return; }
    if (hr < 6 || hr > 15) { say("Shifts start 6am to 3pm"); return; }
    if (L->worked == dn) { say("You worked today. Rest up!"); return; }
    const Sim *m = &sims[ctl]; int perf = (m->need[0] + m->need[1] + m->need[2] + m->need[5]) / 4, good = perf >= 45, pay = job_pay(L), old = minutes;
    L->worked = (u8)dn; L->shifts++; if (!o_rt) minutes += 360;   // (Real time: the shift goes by in a blink, the clock stays real)
    static const u8 NPCT[4] = { 0, 50, 100, 150 };   // (a shift tires them as the Needs speed says)
    for (int q = 0; q < nsim; q++) { static const u8 dd[6] = { 35, 20, 30, 0, 30, 20 }; for (int n = 0; n < 6; n++) { int v = sims[q].need[n] - (q == ctl ? dd[n] : 12) * NPCT[o_need & 3] / 100; sims[q].need[n] = v < 4 ? 4 : v; } }
    if (minutes / 1440 != old / 1440) day_roll();
    if (good) { funds += pay; L->prog++; L->strike = 0; sprintf(b, "Good shift! +$%d", pay); }
    else { funds += pay / 2; L->strike++; sprintf(b, "Bad day... +$%d", pay / 2); life_fear(ctl, FR_BAD); }
    life_ev(ctl, WN_WORK, 0); life_ev(ctl, WN_PAID, 0);
    if (L->prog >= 3 && L->lvl < 4) { L->lvl++; L->prog = 0; funds += 100 * L->lvl; sprintf(b, "Promoted: %s!", JOBLV[L->job - 1][L->lvl]); life_ev(ctl, WN_PROMO, 0); }
    else if (L->strike >= 2) { L->strike = 0; L->prog = 0; if (L->lvl > 0) { L->lvl--; sprintf(b, "Demoted to %s", JOBLV[L->job - 1][L->lvl]); } else { L->job = 0; strcpy(b, "You were fired"); } life_fear(ctl, FR_DEMOTE); }
    say(b); life_ver++;
}
static void job_draw(void) {
    const Life *L = &life[ctl]; char t[56]; panel(8, 20, 240, 152, "Career"); chip(204, 22, 40, 14, "X", 0);
    if (stage_of(sims[ctl].age) < 3) { text_box(14, 50, 228, 20, FS_BODY, TXT, AL_C, "Too young to work."); return; }
    if (!L->job) { text_box(12, 38, 232, 12, FS_HEAD, TXT, AL_L, "Pick a job (6am-3pm, Mon-Fri)");
        for (int k = 0; k < JOBN; k++) { sprintf(t, "%s $%d", JOBNM[k], JOBPAY[k] * 10); chip(14 + (k & 1) * 118, 56 + (k >> 1) * 36, 112, 30, t, 0); } return; }
    sprintf(t, "%s (%s)", JOBLV[L->job - 1][L->lvl], JOBNM[L->job - 1]); text_box(12, 38, 232, 12, FS_HEAD, TXT, AL_L, t);
    sprintf(t, "Level %d of 5   Pay $%d a shift", L->lvl + 1, job_pay(L)); text_box(12, 52, 232, 12, FS_BODY, TXT, AL_L, t);
    text_box(12, 67, 70, 10, FS_BODY, TXT, AL_L, "Promotion"); rect(84, 67, 100, 10, C15(2, 6, 14)); rect(85, 68, 98 * L->prog / 3, 8, C15(8, 26, 8));
    sprintf(t, "Strikes %d of 2   Shifts %d", L->strike, L->shifts); text_box(12, 82, 232, 12, FS_BODY, TXT, AL_L, t);
    chip(14, 108, 112, 28, "Go to work", 1); chip(130, 108, 112, 28, "Quit job", 0); text_box(12, 142, 232, 14, FS_BODY, C15(22, 26, 31), AL_L, "Eat and rest first for a good shift.");
}
static void job_touch(int tx, int ty) {
    if (hit(204, 22, 40, 14, tx, ty)) { jobui = 0; return; }
    Life *L = &life[ctl]; if (stage_of(sims[ctl].age) < 3) return;
    if (!L->job) { for (int k = 0; k < JOBN; k++) if (hit(14 + (k & 1) * 118, 56 + (k >> 1) * 36, 112, 30, tx, ty)) { job_take(k); jobui = 0; } return; }
    if (hit(14, 108, 112, 28, tx, ty)) { job_work(); jobui = 0; } else if (hit(130, 108, 112, 28, tx, ty)) { L->job = 0; L->lvl = L->prog = L->strike = 0; life_ver++; say("You quit. Time for a nap?"); jobui = 0; }
}
static void juke_draw(void) {
    char t[56]; panel(6, 20, 244, 164, "Jukebox"); chip(204, 22, 40, 14, "X", 0);
    if (mus_ok != 1 || mus_n < 1) { text_box(12, 54, 232, 14, FS_BODY, TXT, AL_L, "Songs are not available here."); text_box(12, 70, 232, 14, FS_BODY, TXT, AL_L, "The synth tunes are playing."); return; }
    sprintf(t, "Now: %.30s", mus_cur >= 0 ? mus_name(mus_cur) : "-"); text_box(10, 36, 190, 14, FS_BODY, C15(31, 31, 20), AL_L, t);
    int per = 6, pages = (mus_n + per - 1) / per; if (juke_pg >= pages) juke_pg = 0;
    for (int k = 0; k < per; k++) { int n = juke_pg * per + k; if (n >= mus_n) break; sprintf(t, "%d %.26s", n + 1, mus_name(n)); chip(12, 52 + k * 16, 232, 14, t, n == mus_cur); }
    chip(12, 152, 40, 24, "<", 0); chip(56, 152, 40, 24, ">", 0); chip(100, 152, 44, 24, "Next", 0); chip(148, 152, 52, 24, "Mix", 0); sprintf(t, "%d/%d", juke_pg + 1, pages); text_box(202, 152, 46, 24, FS_NUM, TXT, AL_C, t);
}
static void juke_touch(int tx, int ty) {
    if (hit(204, 22, 40, 14, tx, ty)) { juke = 0; return; }
    if (mus_ok != 1) return; int per = 6, pages = (mus_n + per - 1) / per;
    for (int k = 0; k < per; k++) { int n = juke_pg * per + k; if (n < mus_n && hit(12, 52 + k * 16, 232, 14, tx, ty)) { o_song = n + 1; life_ev(ctl, WN_SONG, 0); } }
    if (hit(12, 152, 40, 24, tx, ty)) juke_pg = (juke_pg + pages - 1) % pages;
    if (hit(56, 152, 40, 24, tx, ty)) juke_pg = (juke_pg + 1) % pages;
    if (hit(100, 152, 44, 24, tx, ty)) { int c = mus_cur < 0 ? 0 : mus_cur; o_song = (c + 1) % mus_n + 1; life_ev(ctl, WN_SONG, 0); }
    if (hit(148, 152, 52, 24, tx, ty)) o_song = 0;
}
static void life_panel(void) {   // the top screen's second page: job, aspiration, wants, fear, the dog
    const Life *L = &life[ctl]; char t[56]; panel(6, 72, 244, 114, "Life");
    if (L->job) sprintf(t, "Job: %.14s  L%d  $%d", JOBLV[L->job - 1][L->lvl], L->lvl + 1, job_pay(L)); else strcpy(t, "Job: none (menu > Job)");
    text_box(11, 88, 236, 12, FS_BODY, TXT, AL_L, t);
    sprintf(t, "Aspiration: %d stars", L->pts / 1500); text_box(11, 101, 146, 12, FS_BODY, C15(31, 31, 20), AL_L, t); rect(158, 102, 84, 8, C15(2, 6, 14)); rect(159, 103, 82 * (L->pts % 1500) / 1500, 6, C15(31, 26, 6));
    for (int k = 0; k < 3; k++) { if (!L->wk[k]) continue; if (L->wk[k] == WN_CASH) sprintf(t, "Have $%d", L->wa[k]); else strcpy(t, WNM[L->wk[k]]);
        rect(14, 118 + k * 13, 8, 8, C15(8, 22, 8)); text_box(23, 116 + k * 13, 160, 12, FS_BODY, TXT, AL_L, t); sprintf(t, "+%d", WPT[L->wk[k]]); text_box(190, 116 + k * 13, 56, 12, FS_NUM, C15(31, 31, 20), AL_R, t); }
    if (L->fear) { text_box(11, 152, 40, 12, FS_BODY, C15(31, 14, 12), AL_L, "Fear:"); text_box(48, 152, 150, 12, FS_BODY, TXT, AL_L, FNM[L->fear]); }
    if (dog_have) { sprintf(t, "Rex: %s", dog.hun > 50 ? "happy" : dog.hun > 20 ? "peckish" : "hungry!"); text_box(150, 163, 98, 12, FS_BODY, TXT, AL_L, t); }
    { const Cx *c = &csx[ctl]; if (c->wish) { sprintf(t, "Wish: %s%s", WSN[c->wish - 1], c->wdone ? " (done)" : ""); text_box(11, 163, 138, 12, FS_BODY, c->wdone ? C15(20, 31, 20) : TXT, AL_L, t); }   // (D67) what Create-a-Sim picked
      if (c->tr[0]) { char u[40]; u[0] = 0; for (int k = 0; k < 3; k++) if (c->tr[k]) { if (k) strcat(u, ", "); strcat(u, TRN[c->tr[k] - 1]); } text_box(11, 174, 236, 9, FS_SMALL, C15(24, 28, 31), AL_L, u); } }
}
static u16 offbuf[256 * 192] __attribute__((aligned(32)));   // (D37) a 2D screen is drawn here and copied over in one go: no half-drawn screen is ever shown
static int ob_owner;   // 1: offbuf holds the Live HUD (so only the bands it used need clearing); anything else that shows a whole screen sets it to 0
static void off_show(u16 *dst) { DC_FlushRange(offbuf, sizeof offbuf); dmaCopy(offbuf, dst, sizeof offbuf); scr = dst; ob_owner = 0; }
static void off_show_bands(u16 *dst, u32 mask) {   // (D70) only the bands of 16 rows in mask
    for (int b = 0; b < 12; b++) if (mask >> b & 1) { int n = b; while (n < 12 && (mask >> n & 1)) n++; u16 *src = offbuf + b * 16 * 256; u32 len = (u32)(n - b) * 16 * 256 * 2; DC_FlushRange(src, len); dmaCopy(src, dst + b * 16 * 256, len); b = n; }
    scr = dst;
}
static const char *mood_word(int v) { return v >= 85 ? "Ecstatic" : v >= 65 ? "Happy" : v >= 45 ? "Fine" : v >= 30 ? "Cranky" : v >= 15 ? "Gloomy" : "Miserable"; }   // (D65) the Sims' mood words
static void thought_bubble(const Sim *m) {   // (D65) the sim's thought, in the Needs header: a bubble with a plumbob: what it wants most (the lowest need), or that life is good
    int lo = 0; for (int k = 1; k < 6; k++) if (m->need[k] < m->need[lo]) lo = k;
    static const char *const WANT[6] = { "I'm starving!", "So bored...", "I need a nap", "Let's chat!", "Gotta go!", "I need a bath" };
    int low = m->need[lo] < 40; const char *ph = low ? WANT[lo] : mood(m) >= 75 ? "Life is good!" : "Just chilling";
    int w = tw(FS_BODY, ph) + 24, x = 246 - w; rect(66, 75, 181, 14, PANH);
    rect(x + 1, 75, w - 2, 14, 0xFFFF); rect(x, 76, w, 12, 0xFFFF); rect(x + 1, 75, w - 2, 1, C15(2, 5, 14)); rect(x + 1, 88, w - 2, 1, C15(2, 5, 14)); rect(x - 1, 77, 1, 10, C15(2, 5, 14)); rect(x + w, 77, 1, 10, C15(2, 5, 14));
    rect(x - 4, 83, 3, 3, 0xFFFF); rect(x - 7, 86, 2, 2, 0xFFFF);   // the thought's little dots
    plumbob(x + 4, 79); text_box(x + 10, 75, w - 12, 14, FS_BODY, low ? C15(20, 3, 3) : C15(2, 8, 4), AL_C, ph);
}
static void status_vals(int clear) {   // (D39) what changes by the minute on the Live top screen (mood, clock, funds, needs): redrawn on its own, straight on the screen, not the whole screen
    if ((on_board || mg_ui) && cur_sim == ctl) return;   // (D44: the skate screen is up; D46: a game)
    const Sim *m = &sims[cur_sim]; char t[48];
    rect(112, 26, 70, 9, C15(24, 28, 31)); rect(113, 27, 68 * mood(m) / 100, 7, need_col(mood(m))); text_box(112, 26, 70, 9, FS_SMALL, C15(1, 4, 10), AL_C, mood_word(mood(m)));
    if (clear) { rect(78, 41, 170, 10, PAN); rect(78, 53, 170, 10, PAN); }
    int mn = minutes % 1440, h12 = (mn / 60) % 12 ? (mn / 60) % 12 : 12;
    { char tm[12], rest[24]; const char *dn = DAYN[(minutes / 1440) % 7]; sprintf(tm, "%d:%02d", h12, mn % 60); sprintf(rest, "%s  %s", mn / 60 >= 12 ? "PM" : "AM", SEASN[fm_season()]);   // (D65: the day, the clock in the NUMBER font, then AM/PM and the season)
      int x = 79, y = box_mid(41, 10, FS_BODY) - 1; text(x, y, FS_BODY, C15(24, 28, 31), dn); x += tw(FS_BODY, dn) + 6; text(x, y, FS_NUM, TXT, tm); x += tw(FS_NUM, tm) + 5; text(x, y, FS_BODY, C15(24, 28, 31), rest); }
    text_box(76, 53, 40, 10, FS_BODY, C15(31, 31, 20), AL_L, "Funds"); sprintf(t, "%d", funds); text_box(112, 53, 64, 10, FS_NUM, C15(31, 31, 20), AL_L, t); if (o_pm) { sprintf(t, "%dfps %dq", fps_now, polys_last / 2); text_box(176, 53, 72, 10, FS_SMALL, polys_last > 2900 ? C15(31, 12, 8) : C15(24, 28, 31), AL_R, t); }
    if (info_pg == 1) return;
    if (zone != Z_ISLAND) thought_bubble(m);
    for (int k = 0; k < 6; k++) { int cx = 14 + (k & 1) * 120, cy = 94 + (k >> 1) * 30;
        rect(cx + 3, cy + 3, 8, 8, need_col(m->need[k])); rect(cx + 18, cy + 7, 84, 8, C15(2, 6, 14)); rect(cx + 19, cy + 8, 82 * m->need[k] / 100, 6, need_col(m->need[k])); }
}
static void draw_top_status(void) {   // the top screen while you live: the sim's mood, the time and money, and the needs (or the bio of another sim)
    if (mg_ui) { mg_top(); return; }   // (D46) a mini-game: its screen
    if (on_board && cur_sim == ctl) { skate_top(); return; }   // (D44) riding: the score and the combo
    scr = offbuf; backdrop(); const Sim *m = &sims[cur_sim];
    char t[48];
    if (cur_sim == ctl) {
        face(8, 8, 56, m);
        panel(70, 6, 180, 60, sname[ctl]);
        text_box(76, 26, 36, 9, FS_BODY, TXT, AL_L, "Mood"); sprintf(t, "%s %d", STGN[stage_of(m->age)], m->age); text_box(184, 26, 64, 9, FS_BODY, C15(31, 31, 20), AL_R, t);
        if (info_pg == 1) { status_vals(0); life_panel(); return; }
        panel(6, 72, 244, 114, "Needs");
        if (zone == Z_ISLAND) { int n = 0; for (int k = 0; k < SEC_N; k++) n += (isl_secrets >> k) & 1; sprintf(t, "W%d S%d L%d Food %d  %d/8", isl_wood, isl_stone, isl_leaves, isl_food, n); text_box(66, 74, 180, 14, FS_BODY, C15(31, 31, 20), AL_R, t); }   // (D37: wood, stone, leaves, food, secrets)
        static const int order[6] = { 0, 1, 2, 3, 4, 5 };
        for (int k = 0; k < 6; k++) { int n = order[k], cx = 14 + (k & 1) * 120, cy = 94 + (k >> 1) * 30;
            rect(cx, cy, 14, 14, C15(18, 24, 31)); text_box(cx + 17, cy - 4, 86, 10, FS_BODY, TXT, AL_L, NEEDN[n]); }
        status_vals(0);
    } else {
        char tt[40]; sprintf(tt, "%s - %s %d", sname[cur_sim], STGN[stage_of(m->age)], m->age); panel(6, 6, 244, 180, tt);
        face(16, 30, 64, m);
        for (int k = 0; k < 4; k++) { rect(94 + k * 38, 30, 32, 32, C15(18, 24, 31)); text_box(94 + k * 38, 30, 32, 32, FS_HEAD, PANB, AL_C, "?"); }
        panel(94, 70, 148, 60, 0);
        panel(94, 136, 148, 40, 0); sprintf(t, "- %s", relname(m->rel)); text_box(98, 136, 140, 40, FS_BODY, TXT, AL_L, t);
        sprintf(t, "%d", mood(m)); text_box(14, 96, 66, 12, FS_BODY, TXT, AL_L, "MOOD"); rect(16, 110, 64, 8, C15(24, 28, 31)); rect(17, 111, 62 * mood(m) / 100, 6, need_col(mood(m)));
    }
}
static void hud_clear(void) { u32 *q = (u32 *)hud; for (int i = 0; i < 256 * 192 / 2; i++) q[i] = 0; ob_owner = 0; }
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
static void draw_hud_i(void) {
    scr = offbuf;
    if (mg_ui) { mg_bottom(); return; }   // (D46)
    rect(2, 2, 26, 26, PANB); rect(3, 3, 24, 24, C15(14, 22, 28)); rect(8, 12, 14, 10, C15(26, 14, 10)); rect(6, 8, 18, 5, C15(24, 6, 6)); rect(13, 16, 4, 6, C15(8, 8, 10));   // MAP: a little house
    for (int i = 0; i < nsim; i++) { int x = 132 + i * 21; rect(x, 2, 20, 27, i == ctl ? C15(31, 31, 12) : PANB); rect(x + 1, 3, 18, 25, C15(14, 22, 28)); face(x + 2, 4, 16, &sims[i]); rect(x + 1, 24, 18, 4, C15(2, 6, 14)); rect(x + 1, 24, 18 * mood(&sims[i]) / 100, 4, need_col(mood(&sims[i]))); }   // the household
    rect(0, 158, 34, 34, PANB); face(2, 160, 30, &sims[ctl]);
    rect(36, 168, 24, 22, PANB); rect(37, 169, 22, 20, PAN); text_box(37, 169, 22, 20, FS_HEAD, TXT, AL_C, speed == 1 ? ">" : speed == 2 ? ">>" : ">>>");
    rect(62, 168, 30, 22, PANB); rect(63, 169, 28, 20, cam_on ? C15(8, 26, 8) : PAN); text_box(63, 169, 28, 20, FS_BODY, TXT, AL_C, "Cam");
    if (cam_on) { const char *cb[6] = { cam_pan ? "Pan" : "Turn", "In", "Out", cam_free ? "Free" : "Follow", "Reset", "Done" }; for (int k = 0; k < 6; k++) { rect(216, 36 + k * 22, 38, 20, PANB); rect(217, 37 + k * 22, 36, 18, k == 0 && cam_pan ? C15(8, 26, 8) : PANH); text_box(217, 37 + k * 22, 36, 18, FS_BODY, TXT, AL_C, cb[k]); } text_box(30, 34, 180, 14, FS_BODY, TXT, AL_C, "Drag to look round"); }
    rect(104, 178, 48, 14, PANB); rect(105, 179, 46, 13, PAN); for (int i = 0; i < 3; i++) rect(118 + i * 8, 184, 4, 4, TXT);
    if (on_board) { rect(196, 168, 58, 22, PANB); rect(197, 169, 56, 20, C15(24, 8, 8)); text_box(197, 169, 56, 20, FS_BODY, TXT, AL_C, "Get Off"); }   // (D44)
    if (menu_open) { static const char *const mn[12] = { "Buy", "Build", "Map", "Save", "Options", "Title", "Job", "Songs", "Life", "Travel", "Games", "Heist" };   // (D70) no big panel behind (it cost 8 ms to draw): the chips alone, a title strip and the hints bar
        rect(14, 40, 226, 16, PANB); rect(15, 41, 224, 14, PANH); text_box(15, 41, 224, 14, FS_HEAD, TXT, AL_C, "Menu");
        for (int i = 0; i < 12; i++) chip(14 + (i % 4) * 58, 60 + (i / 4) * 36, 54, 30, mn[i], i == 9 && zone);
        ui_focus(14 + (menu_sel % 4) * 58, 60 + (menu_sel / 4) * 36, 54, 30); ui_hints(168, "Pad:Move|A:Pick|B:Close"); }
    if (popup == 1) {
        const Item *it = IT_OF(&objs[popup_i]); const char *nm = it->name;   // (D35: wide, for the PS1's words; D37: up to three choices; D38: up to eight, in two columns)
        if (popup_n <= 3) { int top = 150 - popup_n * 24;
            rect(36, top, 184, 30 + popup_n * 24, PANB); rect(37, top + 1, 182, 28 + popup_n * 24, PAN); text_box(38, top + 1, 180, 24, FS_HEAD, TXT, AL_L, nm);
            for (int k = 0; k < popup_n; k++) { int y = top + 26 + k * 24; rect(42, y, 116, 22, C15(8, 20, 8)); text_box(42, y, 116, 22, FS_BODY, TXT, AL_C, popup_lab[k]); }
            rect(162, top + 2 + popup_n * 24, 52, 22, C15(24, 8, 8)); text_box(162, top + 2 + popup_n * 24, 52, 22, FS_BODY, TXT, AL_C, "Cancel");
        } else { int rows = (popup_n + 1) / 2, top = 156 - rows * 24;
            rect(4, top, 248, 30 + rows * 24, PANB); rect(5, top + 1, 246, 28 + rows * 24, PAN); text_box(8, top + 2, 184, 22, FS_HEAD, TXT, AL_L, nm);
            rect(196, top + 3, 50, 18, C15(24, 8, 8)); text_box(196, top + 3, 50, 18, FS_BODY, TXT, AL_C, "Cancel");
            for (int k = 0; k < popup_n; k++) { int x = 9 + (k & 1) * 121, y = top + 26 + (k >> 1) * 24; rect(x, y, 117, 22, popup_code[k] == FC_NOORDER ? C15(10, 12, 14) : C15(8, 20, 8)); text_box(x, y, 117, 22, FS_BODY, TXT, AL_C, popup_lab[k]); } }
    } else if (popup == 5) {   // (D36) at the top of a flight
        rect(36, 100, 184, 56, PANB); rect(37, 101, 182, 54, PAN); text_box(38, 101, 180, 22, FS_HEAD, TXT, AL_L, "Stairs");
        rect(42, 126, 116, 24, C15(8, 20, 8)); text_box(42, 126, 116, 24, FS_BODY, TXT, AL_C, "Go Down"); rect(162, 126, 52, 24, C15(24, 8, 8)); text_box(162, 126, 52, 24, FS_BODY, TXT, AL_C, "Cancel");
    } else if (popup == 4) {
        rect(56, 96, 144, 72, PANB); rect(57, 97, 142, 70, PAN); text_box(60, 99, 136, 16, FS_HEAD, TXT, AL_L, "Rex"); { char t2[24]; sprintf(t2, "Hunger %d", dog.hun); text_box(60, 99, 136, 16, FS_BODY, TXT, AL_R, t2); }
        chip(64, 118, 60, 20, "Pet", 0); chip(132, 118, 60, 20, "Play", 0); chip(64, 142, 60, 20, "Feed $5", 0); chip(132, 142, 60, 20, "Cancel", 0);
    } else if (popup >= 2) {
        char nm[40]; int tg = popup == 2 ? popup_i : -2 - popup_i, rel; if (popup == 2) { sprintf(nm, "%s - %s %d", sname[popup_i], STGN[stage_of(sims[popup_i].age)], sims[popup_i].age); rel = sims[popup_i].rel; }
        else { char n2[24]; cit_name(&cits[popup_i], n2); sprintf(nm, "%s - %s", n2, STGN[cits[popup_i].stage]); rel = crel(cits[popup_i].id)->rel; }
        int n = soc_list(tg);
        rect(20, 56, 216, 124, PANB); rect(21, 57, 214, 122, PAN); rect(21, 57, 214, 14, PANH); rect(20, 56, 2, 2, 0); rect(234, 56, 2, 2, 0); text_box(21, 57, 188, 14, FS_BODY, TXT, AL_L, nm); rect(213, 58, 20, 12, C15(24, 8, 8)); text_box(213, 58, 20, 12, FS_BODY, TXT, AL_C, "X");
        sprintf(nm, "%s", relname(rel)); text_box(21, 71, 214, 11, FS_SMALL, C15(24, 28, 31), AL_L, nm);
        for (int k = 0; k < n; k++) { int x = 26 + (k & 1) * 104, y = 82 + (k >> 1) * 22; rect(x, y, 100, 20, PANB); rect(x + 1, y + 1, 98, 18, soc_pick[k] == S_INVITE || soc_pick[k] == S_BABY ? C15(8, 20, 8) : PANH); text_box(x + 1, y + 1, 98, 18, FS_BODY, TXT, AL_C, SOCN[soc_pick[k]]); }
        if (!n) text_box(21, 88, 214, 24, FS_BODY, TXT, AL_C, "Nothing to do together"); }
    if (jobui) job_draw();
    if (juke) juke_draw();
    if (travel_ui) travel_draw(); else if (craft_ui) craft_draw(); else if (farm_ui) farm_draw();
    if (msg_t > 0 && !jobui && !juke && !travel_ui && !craft_ui && !farm_ui) toast(128, 40, msg);
}
static void ob_zero(u16 *p, u32 n) { DC_InvalidateRange(p, n); dmaFillWords(0, p, n); }   // (D70) clear by DMA: about three times quicker than the CPU, which is slow on main memory
static u32 hud_prev, hud_show_m = 0xFFF; static int ui_heavy;   // ui_heavy: this pass drew a screen of 2D: the 3D frame waits for the next pass, so the pass still fits in a frame (D70)
static void draw_hud(void) {   // (D70) clears and copies only the bands of rows the HUD uses (now and last time): the HUD is mostly empty, and a whole screen cost more than a frame
    int full = mg_ui || ob_owner != 1; scr = offbuf;
    u32 clr = full ? 0xFFFu : hud_prev;
    DBGT(2, for (int b = 0; b < 12; b++) if (clr >> b & 1) { int n = b; while (n < 12 && (clr >> n & 1)) n++; ob_zero(offbuf + b * 16 * 256, (u32)(n - b) * 16 * 256 * 2); b = n; });
    dr_mask = 0; DBGT(3, draw_hud_i());
    hud_show_m = full ? 0xFFFu : (hud_prev | dr_mask); hud_prev = mg_ui ? 0xFFFu : dr_mask; if (mg_ui) hud_show_m = 0xFFFu;
}
static void hud_show(void) { off_show_bands(hud, hud_show_m); ob_owner = 1; }
// ---- picking: a stylus tap on the 3D lot -> the cell under it (the view is a camera we know, so we undo it) ----
static int pick_cell_h(int sx, int sy, int hgt, int *cx, int *cy) {   // the cell where the stylus ray meets the plane hgt world units above the ground
    float c = cosLerp(live_yaw) / 4096.0f, s = sinLerp(live_yaw) / 4096.0f, cp = cosLerp(pitch) / 4096.0f, sp = sinLerp(pitch) / 4096.0f;
    float nx = (sx - 128) / 128.0f, ny = (96 - sy) / 96.0f, dx = nx * 0.3640f * 1.3333f, dy = ny * 0.3640f, dz = -1.0f;
    float ox = 0, oy = 0, oz = (float)cam_deff;
    float ay = dy * cp + dz * sp, az = -dy * sp + dz * cp, by = oy * cp + oz * sp, bz = -oy * sp + oz * cp;
    float ddx = dx * c - az * s, ddz = dx * s + az * c, oox = ox * c - bz * s + camx, ooz = ox * s + bz * c + camz;
    if (ay > -0.0001f && ay < 0.0001f) return 0;
    float t = ((float)(hgt - cam_y) - by) / ay; float hx = oox + t * ddx, hz = ooz + t * ddz;
    *cx = (int)((hx / T + GN) / 2.0f + 200) - 200; *cy = (int)((hz / T + GN) / 2.0f + 200) - 200;
    return 1;
}

static int pick_cell(int sx, int sy, int *cx, int *cy) { return pick_cell_h(sx, sy, 0, cx, cy); }
static int obj_height(int o) {   // how tall an item stands, in world units: the top of its own boxes (worked out once per item)
    static s16 hg[NITEM]; int id = OBJ_ID(&objs[o]); if (id >= NITEM) return 0;
    if (!hg[id]) { pv_cap = 1; npv = 0; obj3d(0, 0, &ITEMS[id], 0, 0); pv_cap = 0; int m = 1; for (int k = 0; k < npv; k++) if (pv[k].h1 > m) m = pv[k].h1; hg[id] = (s16)(m * 82 / 10 + 1); }
    return hg[id];
}
static int pick_obj(int sx, int sy) {   // (D35) the item under the stylus, nearest first: each item's box is projected with the 3D view's own camera (a tall or thin thing is caught by its top too)
    float c = cosLerp(live_yaw) / 4096.0f, s = sinLerp(live_yaw) / 4096.0f, cp = cosLerp(pitch) / 4096.0f, sp = sinLerp(pitch) / 4096.0f, best = 1e30f; int bi = -1;
    for (int i = 0; i < nobj; i++) { if (OBJ_LV(&objs[i]) != LV) continue; int w, h; obj_fp(&objs[i], &w, &h); float fy = 40.0f + (float)(LV * WHF - cam_y), x0 = (float)tl(objs[i].x * 2), x1 = (float)tl((objs[i].x + w) * 2), z0 = (float)tl(objs[i].y * 2), z1 = (float)tl((objs[i].y + h) * 2), y1 = fy + (float)obj_height(i);
        float u0 = 1e9f, u1 = -1e9f, v0 = 1e9f, v1 = -1e9f, zc = 0; int ok = 1;
        for (int k = 0; k < 8; k++) { float X = ((k & 1) ? x1 : x0) - camx, Y = (k & 2) ? y1 : fy, Z = ((k & 4) ? z1 : z0) - camz;
            float xa = X * c + Z * s, za = -X * s + Z * c, yb = Y * cp - za * sp, zb = Y * sp + za * cp - (float)cam_deff;
            if (zb > -100.0f) { ok = 0; break; } float u = 128.0f + xa / -zb / (0.3640f * 1.3333f) * 128.0f, v = 96.0f - yb / -zb / 0.3640f * 96.0f;
            if (u < u0) u0 = u; if (u > u1) u1 = u; if (v < v0) v0 = v; if (v > v1) v1 = v; zc += zb; }
        if (!ok || sx < u0 - 2 || sx > u1 + 2 || sy < v0 - 2 || sy > v1 + 2) continue;
        if (-zc < best) { best = -zc; bi = i; } }
    return bi;
}
// ---- interactions: what each life stage may do with whom (and how often) ----
static int cl100(int v) { return v < 0 ? 0 : v > 100 ? 100 : v; }
static void new_sim(int age, u8 skin, u8 hair, u8 hcol, u8 shirt, const char *nm, int x, int y) {
    if (nsim >= SIM_MAX) return; Sim *m = &sims[nsim]; memset(m, 0, sizeof *m); m->x = m->tx = m->wx = x; m->y = m->ty = m->wy = y; m->act = -1; m->soc = -1; m->age = age; m->skin = skin; m->hair = hair; m->hcol = hcol; m->shirt = shirt; m->fc = (u8)(nsim % 6);
    memset(&life[nsim], 0, sizeof(Life)); memset(&csx[nsim], 0, sizeof(Cx)); slv[nsim] = (u8)LV; for (int n = 0; n < 6; n++) m->need[n] = 70; for (int q = 0; q < PMN; q++) m->pm[q] = 50; strncpy(sname[nsim], nm, 19); sname[nsim][19] = 0; nsim++; ui_ver++;
}
static void apply_soc(int ai, int tgt, int id) {
    Sim *a = &sims[ai]; int cit = tgt <= -2, sb, rel; Sim *o = 0; CRel *cr = 0; Cit *ct = 0;
    if (cit) { ct = &cits[-2 - tgt]; cr = crel(ct->id); sb = ct->stage; rel = cr->rel; if (cr->talks >= 4) { if (ai == ctl) say("Busy right now. Try later"); return; } cr->talks++; ct->stop = 100; }
    else { o = &sims[tgt]; sb = stage_of(o->age); rel = o->rel; if (ilim[ai][tgt] >= 6) { if (ai == ctl) say("Not feeling chatty today"); return; } ilim[ai][tgt]++; ilim[tgt][ai]++; }
    if (!soc_ok(id, stage_of(a->age), sb, rel, cit)) return;
    int d = SREL[id]; a->need[3] = cl100(a->need[3] + SSOC[id]); a->need[1] = cl100(a->need[1] + SFUN[id]);
    if (o) { o->need[3] = cl100(o->need[3] + SSOC[id]); o->need[1] = cl100(o->need[1] + SFUN[id]); }
    if (id == S_FEED) o->need[0] = cl100(o->need[0] + 60); if (id == S_TUCK) o->need[2] = cl100(o->need[2] + 60);
    if (id == S_KISS && o && !fpt[ai] && !fpt[tgt] && ai != tgt) { int fa = fam_id(ai), fo = fam_id(tgt); fpt[ai] = (u8)fo; fpt[tgt] = (u8)fa; if (ai == ctl) say("Ooh, a new partner!"); }   // (D62) a kiss makes a pair (the family tree shows them)
    if (id == S_BABY) { if (nsim >= SIM_MAX) { if (ai == ctl) say("Household is full"); return; } if (pop_per + pop_born >= DS_POP_MAX) { if (ai == ctl) say("City is full (2000)"); return; }
        int bx2 = a->x + 16, by2 = a->y; for (int k = 0; k < nobj; k++) if (objs[k].c == 5 && objs[k].i == 0) { bx2 = objs[k].x * 16; by2 = objs[k].y * 16; break; }
        static const char *const BN[8] = { "Bo", "Remy", "Pip", "Juno", "Tate", "Wren", "Otis", "Fern" }; char nm[20]; sprintf(nm, "%s %s", BN[(minutes / 7 + nsim) % 8], strrchr(sname[ai], ' ') ? strrchr(sname[ai], ' ') + 1 : "B");
        new_sim(0, (minutes & 1) ? a->skin : o->skin, (u8)((minutes + nsim) % 12), (minutes & 2) ? a->hcol : o->hcol, (u8)(minutes % 6), nm, bx2, by2); pop_born++; st_born++; { int bi = nsim - 1, fa = fam_id(ai), fo = fam_id(tgt); fam_id(bi); fpar[bi][0] = (u8)fa; fpar[bi][1] = (u8)fo; if (!fpt[ai] && !fpt[tgt]) { fpt[ai] = (u8)fo; fpt[tgt] = (u8)fa; } } life_ev(ai, WN_BABY, 0); if (ai == ctl) say("It is a baby! Congrats!"); }
    if (id == S_INVITE) { if (nsim >= SIM_MAX) { if (ai == ctl) say("Household is full"); return; } char nm[20]; cit_name(ct, nm);
        new_sim(sb == 3 ? 15 : sb == 4 ? 28 : 65, ct->skin, (u8)((ct->id >> 3) % NHAIRD), ct->hair, ct->shirt, nm, a->x, a->y); cr->taken = 1; ct->on = 0; if (ai == ctl) say("Moved in! Say hello"); return; }
    if (cit) { int r = (int)cr->rel + d; cr->rel = (u8)(r < 0 ? 0 : r > 10 ? 10 : r); } else { int r = o->rel + d; o->rel = r < 0 ? 0 : r > 10 ? 10 : r; r = a->rel + d; a->rel = r < 0 ? 0 : r > 10 ? 10 : r; }
    { int nr = cit ? cr->rel : o->rel; life_ev(ai, WN_CHAT, 0); if (nr >= 3) life_ev(ai, WN_FRIEND, 0); }
    if (ai == ctl && id != S_BABY) say(SOCN[id]);
}
static void start_soc(int ai, int tgt, int id) {
    Sim *m = &sims[ai]; int tx, ty;
    if (tgt <= -2) { const Cit *c = &cits[-2 - tgt]; tx = fdiv(c->x + 8, 16) * 16; ty = fdiv(c->y + 8, 16) * 16; } else { tx = sims[tgt].x; ty = sims[tgt].y; }
    m->tx = tx; m->ty = ty; m->act = 3 + 100; m->tgt = tgt; m->soc = id; m->act_amt = 50; m->face = 120; m->act_left = 0;
}
static void sim_die(int k) { st_died++;   // an elder passes away: a statue marks the spot and the household closes up
    Sim *m = &sims[k]; int cx = fdiv(m->x + 8, 16), cy = fdiv(m->y + 8, 16); static char bf[40]; sprintf(bf, "%s passed away", sname[k]); say(bf); ghost_add(m);
    if (cx >= 0 && cy >= 0 && cx < GN && cy < GN && nobj < 400) { objs[nobj].x = (u8)cx; objs[nobj].y = (u8)cy; objs[nobj].c = (u8)((PS1ID[0x83] - 1) >> 2); objs[nobj].i = (u8)((PS1ID[0x83] - 1) & 3); objs[nobj].rot = (u8)(slv[k] << 2); nobj++; occ_ok = 0; }
    for (int i = k; i < nsim - 1; i++) { sims[i] = sims[i + 1]; life[i] = life[i + 1]; slv[i] = slv[i + 1]; memcpy(sname[i], sname[i + 1], 20); fid[i] = fid[i + 1]; fpar[i][0] = fpar[i + 1][0]; fpar[i][1] = fpar[i + 1][1]; fpt[i] = fpt[i + 1]; csx[i] = csx[i + 1]; } memset(&csx[nsim - 1], 0, sizeof(Cx)); nsim--; fid[nsim] = fpt[nsim] = fpar[nsim][0] = fpar[nsim][1] = 0; life_ver++;
    if (ctl == k) ctl = 0; else if (ctl > k) ctl--; if (stage_of(sims[ctl].age) == 0) for (int i = 0; i < nsim; i++) if (stage_of(sims[i].age) > 0) { ctl = i; break; }
    cur_sim = ctl; memset(ilim, 0, sizeof ilim); ui_ver++; }

// ---- saving: the cartridge's save chip. Squeezed to fit any 8 KB EEPROM: two copies (A/B, 4 KB each) so a power cut mid-save never loses the older one. The lot you are on always goes first; older lots are dropped if there is no room.
static int SAV_STRIDE = 4096;   // one copy's room: half the chip (4 KB on an 8 KB chip, 32 KB on a 64 KB chip)
#define SAV_MAGIC 0x53325242u   // "BR2S"
typedef struct { u32 magic, seq, crc, clen, rawlen; } SavHdr;
typedef struct { int seed, minutes, funds, nsim, ctl, cur_sim, cur_set, lot_bx, lot_by; u8 opt[40]; int nlots, days, born, died; } SavCore;
typedef struct { s16 bx, by, pcx, pcy, dcx, dcy; u16 nobj; } SavLot;
#define SAV_MAX 65536
static u8 sav_sh[2][SAV_MAX], sav_raw[LSV_N * 4300 + 4096], sav_buf[SAV_MAX]; static int sav_type = -2, sav_have = -1, sav_seq;
static u32 crc32b(const u8 *p, int n) { u32 c = 0xFFFFFFFFu; while (n--) { c ^= *p++; for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1))); } return ~c; }
static int rle(const u8 *p, int n, u8 *o, int mx) { int w = 0; for (int i = 0; i < n;) { if (w + 2 > mx) return -1; if (p[i]) { o[w++] = p[i++]; } else { int r = 0; while (i < n && !p[i] && r < 255) { i++; r++; } o[w++] = 0; o[w++] = (u8)r; } } return w; }
static int unrle(const u8 *p, int n, u8 *o, int mx) { int w = 0; for (int i = 0; i < n;) { if (p[i]) { if (w >= mx) return -1; o[w++] = p[i++]; } else { if (i + 1 >= n) return -1; int r = p[i + 1]; if (w + r > mx) return -1; while (r--) o[w++] = 0; i += 2; } } return w; }
static int sav_ok(void) { if (sav_type == -2) { sysSetCardOwner(true); int t = cardEepromGetType(); sav_type = (t == 2 || t == 3) ? t : -1; } return sav_type > 0; }
static int sav_valid(int k) { SavHdr h; memcpy(&h, sav_sh[k], sizeof h); return h.magic == SAV_MAGIC && h.clen > 0 && h.clen <= (u32)(k == 0 ? 2 * SAV_STRIDE : SAV_STRIDE) - sizeof h && h.rawlen <= sizeof sav_raw && crc32b(sav_sh[k] + sizeof h, h.clen) == h.crc; }
static void sav_size(void) {   // is it an 8 KB or a 64 KB chip? A 8 KB chip repeats itself at 8192, so write one byte there and see whether address 0 changes (then put both back)
    static int done; if (done) return; done = 1; SAV_STRIDE = 4096; if (sav_type == 3) { SAV_STRIDE = 32768; return; }
    u8 a, b, t; cardReadEeprom(0, &a, 1, sav_type); cardReadEeprom(8192, &b, 1, sav_type); t = (u8)(a ^ 0x5A);
    cardWriteEeprom(8192, &t, 1, sav_type); u8 r; cardReadEeprom(0, &r, 1, sav_type); cardWriteEeprom(8192, &b, 1, sav_type);
    if (r == a) { u8 c; cardReadEeprom(8192, &c, 1, sav_type); if (c == b) SAV_STRIDE = 32768; }
}
static void sav_probe(void) {   // read both copies, find the newest good one
    sav_have = -1; sav_seq = 0; if (!sav_ok()) return; sav_size();
    for (int k = 0; k < (sav_type == 3 ? 1 : 2); k++) { cardReadEeprom((u32)(k * SAV_STRIDE), sav_sh[k], (u32)(k == 0 ? 2 * SAV_STRIDE : SAV_STRIDE), sav_type); if (sav_valid(k)) { SavHdr h; memcpy(&h, sav_sh[k], sizeof h); if (sav_have < 0 || (int)h.seq > sav_seq) { sav_have = k; sav_seq = (int)h.seq; } } }
    st_days = st_born = st_died = 0;
    if (sav_have >= 0) { SavHdr h; memcpy(&h, sav_sh[sav_have], sizeof h); if (unrle(sav_sh[sav_have] + sizeof h, (int)h.clen, sav_raw, (int)sizeof sav_raw) == (int)h.rawlen && h.rawlen >= sizeof(SavCore)) { SavCore c; memcpy(&c, sav_raw, sizeof c); st_days = c.days; st_born = c.born; st_died = c.died; } }
}
typedef struct { Life life[SIM_MAX]; Ghost gh[GH_MAX]; u8 ngh, dog, hun, pad; } LifeSave;
typedef struct { u8 b[28]; u16 w[10]; u16 sk[SIM_MAX]; } FarmSave;   // (D38) the farm: 'FM' block
static void fm_pack(FarmSave *f) { memset(f, 0, sizeof *f); memcpy(f->b, fm_n, 4); memcpy(f->b + 4, fm_g, 4); memcpy(f->b + 8, fm_dry, 4);
    u8 v[15] = { fm_wet, fm_hens, fm_cows, fm_fed, fm_fert, fm_fz, fm_pest, fm_up, fm_wc, fm_ordk, fm_ordq, fm_ordt, fm_fair, fm_cdone, fm_cday }; memcpy(f->b + 12, v, 15);
    u16 w[10] = { fm_stock[0], fm_stock[1], fm_stock[2], fm_stock[3], fm_egg, fm_milk, fm_pe, fm_pm, fm_built, fm_last }; memcpy(f->w, w, sizeof w); memcpy(f->sk, fsk, sizeof fsk); }
static void fm_unpack(const FarmSave *f) { memcpy(fm_n, f->b, 4); memcpy(fm_g, f->b + 4, 4); memcpy(fm_dry, f->b + 8, 4); const u8 *v = f->b + 12;
    fm_wet = v[0]; fm_hens = v[1] > 6 ? 6 : v[1]; fm_cows = v[2] > 3 ? 3 : v[2]; fm_fed = v[3]; fm_fert = v[4]; fm_fz = v[5]; fm_pest = v[6]; fm_up = v[7]; fm_wc = v[8]; fm_ordk = v[9] % 6; fm_ordq = v[10]; fm_ordt = v[11]; fm_fair = v[12]; fm_cdone = v[13]; fm_cday = v[14];
    for (int k = 0; k < 4; k++) { if (fm_n[k] > 8) fm_n[k] = 8; fm_stock[k] = f->w[k]; } fm_egg = f->w[4]; fm_milk = f->w[5]; fm_pe = f->w[6]; fm_pm = f->w[7]; fm_built = f->w[8]; fm_last = f->w[9]; memcpy(fsk, f->sk, sizeof fsk); }
#define MGBUF_N 4096   // (D64) the mini-games' save blocks (the Boreagotchi's v48 extras need the room)
static int sav_pack(int big) {   // returns the packed size in sav_buf (header included), or 0 if even the basics do not fit
    if (lot_bx >= 0) lot_save();
    SavCore c; memset(&c, 0, sizeof c); c.seed = seed; c.minutes = minutes; c.funds = funds; c.nsim = nsim; c.ctl = ctl; c.cur_sim = cur_sim; c.cur_set = cur_set; c.lot_bx = lot_bx; c.lot_by = lot_by; c.days = st_days; c.born = st_born; c.died = st_died;
    for (int i = 0; i < NOPT && i < OPT_OLD; i++) if (OPT[i].v) c.opt[i] = (u8)*OPT[i].v;
    int n = 0; u8 *r = sav_raw; memset(r, 0, sizeof sav_raw);
    #define PUT(p, l) do { memcpy(r + n, (p), (size_t)(l)); n += (l); } while (0)
    PUT(&c, (int)sizeof c); PUT(sname, (int)sizeof sname); PUT(sims, (int)sizeof sims); PUT(crt, (int)sizeof crt);
    static u8 mgbuf[MGBUF_N]; int mgsz = mg_pack(mgbuf);   // (D48: the mini-games' blocks, as big as they are)
    int budget = (big ? 2 * SAV_STRIDE : SAV_STRIDE) - (int)sizeof(SavHdr) - (4 + nown * 4) - (4 + (int)sizeof(LifeSave)) - (4 + NOPT - OPT_OLD) - 8 - (4 + SIM_MAX) - 20 - (4 + (int)sizeof(FarmSave)) - mgsz, base = n, best = n, nl = 0, cl = rle(r, n, sav_buf + sizeof(SavHdr), budget); if (cl < 0) return 0;
    int order[LSV_N], no = 0; for (int i = 0; i < lsv_n; i++) if (lsv[i].bx == lot_bx && lsv[i].by == lot_by) order[no++] = i; for (int i = 0; i < lsv_n; i++) if (!(lsv[i].bx == lot_bx && lsv[i].by == lot_by)) order[no++] = i;
    for (int q = 0; q < no; q++) { const LotSave *L = &lsv[order[q]]; int m = n; SavLot h = { (s16)L->bx, (s16)L->by, (s16)L->pcx, (s16)L->pcy, (s16)L->dcx, (s16)L->dcy, (u16)(L->nobj | (L->roof << 12) | (L->up << 14)) };
        int nup = L->hasup ? L->up : 0; if (nup > UPN) nup = UPN; if (nup) h.nobj |= 0x800;   // (D36) its upper storeys follow
        if (m + (int)sizeof h + 533 + L->nobj * 5 + 1 + nup * 533 > (int)sizeof sav_raw) break;
        memcpy(r + m, &h, sizeof h); m += (int)sizeof h; memcpy(r + m, L->flr, sizeof L->flr); m += (int)sizeof L->flr; memcpy(r + m, L->hwl, sizeof L->hwl); m += (int)sizeof L->hwl; memcpy(r + m, L->vwl, sizeof L->vwl); m += (int)sizeof L->vwl; memcpy(r + m, L->objs, (size_t)L->nobj * 5); m += L->nobj * 5;
        if (nup) { r[m++] = (u8)nup; for (int u = 0; u < nup; u++) { memcpy(r + m, L->ufl[u], sizeof L->ufl[u]); m += (int)sizeof L->ufl[u]; memcpy(r + m, L->uhw[u], sizeof L->uhw[u]); m += (int)sizeof L->uhw[u]; memcpy(r + m, L->uvw[u], sizeof L->uvw[u]); m += (int)sizeof L->uvw[u]; } }
        ((SavCore *)r)->nlots = nl + 1; int t = rle(r, m, sav_buf + sizeof(SavHdr), budget); if (t < 0) { ((SavCore *)r)->nlots = nl; break; } n = m; nl++; best = m; cl = t; }
    #undef PUT
    (void)base; ((SavCore *)r)->nlots = nl;
    { u16 th[2] = { 0x504F, (u16)nown }; memcpy(r + best, th, 4); memcpy(r + best + 4, own, (size_t)nown * 4); best += 4 + nown * 4; }   // the lots you own, after the lots
    { LifeSave ls; memset(&ls, 0, sizeof ls); memcpy(ls.life, life, sizeof life); memcpy(ls.gh, gh, sizeof gh); ls.ngh = (u8)ngh; ls.dog = (u8)dog_have; ls.hun = (u8)dog.hun; u16 th[2] = { 0x4C46, (u16)sizeof ls }; memcpy(r + best, th, 4); memcpy(r + best + 4, &ls, sizeof ls); best += 4 + (int)sizeof ls; }   // wants, jobs, the dog, ghosts
    { u16 th[2] = { 0x584F, (u16)(NOPT - OPT_OLD) }; memcpy(r + best, th, 4); for (int i = OPT_OLD; i < NOPT; i++) r[best + 4 + i - OPT_OLD] = OPT[i].v ? (u8)*OPT[i].v : 0; best += 4 + NOPT - OPT_OLD; }   // the options added at D33
    { s16 th[4] = { 0x4D48, 4, (s16)home_bx, (s16)home_by }; memcpy(r + best, th, 8); best += 8; }   // the home you live in (D33)
    { u16 th[2] = { 0x4C53, SIM_MAX }; memcpy(r + best, th, 4); memcpy(r + best + 4, slv, SIM_MAX); best += 4 + SIM_MAX; }   // the storey each sim is on (D36)
    { s16 zs[10] = { 0x4E5A, 16, (s16)zone, (s16)isl_wood, (s16)isl_stone, (s16)isl_leaves, (s16)isl_food, (s16)isl_spear, (s16)isl_shells, (s16)isl_secrets }; memcpy(r + best, zs, 20); best += 20; }   // the zone and the island (D37)
    { u16 th[2] = { 0x4D46, (u16)sizeof(FarmSave) }; FarmSave f; fm_pack(&f); memcpy(r + best, th, 4); memcpy(r + best + 4, &f, sizeof f); best += 4 + (int)sizeof f; }   // the farm (D38)
    memcpy(r + best, mgbuf, (size_t)mgsz); best += mgsz;   // (D44 on) the mini-games: the board, the scores
    cl = rle(r, best, sav_buf + sizeof(SavHdr), budget + 4 + nown * 4 + 4 + (int)sizeof(LifeSave) + 4 + NOPT - OPT_OLD + 8 + 4 + SIM_MAX + 20 + 4 + (int)sizeof(FarmSave) + mgsz); if (cl < 0) return 0;
    SavHdr h = { SAV_MAGIC, (u32)(sav_seq + 1), crc32b(sav_buf + sizeof(SavHdr), cl), (u32)cl, (u32)best }; memcpy(sav_buf, &h, sizeof h); return (int)sizeof h + cl;
}
static int sav_write(void) {
    if (!sav_ok()) return 0;
    int big = o_smax || sav_type == 3, oldbig = 0; if (sav_have == 0) { SavHdr o; memcpy(&o, sav_sh[0], sizeof o); oldbig = o.clen > (u32)SAV_STRIDE - sizeof o; }
    int n = sav_pack(big); if (!n) return 0; int k = (big || oldbig || sav_type == 3) ? 0 : (sav_have == 0 ? 1 : 0), hs = (int)sizeof(SavHdr);   // a big copy fills the whole chip, so it (and the write after it) lives in copy A
    if (sav_type == 3) { cardEepromSectorErase(0); memset(sav_sh[0], 0xFF, (size_t)SAV_STRIDE * 2); }
    for (int o = hs; o < n; o += 64) { int l = n - o < 64 ? n - o : 64; if (memcmp(sav_buf + o, sav_sh[k] + o, (size_t)l)) cardWriteEeprom((u32)(k * SAV_STRIDE + o), sav_buf + o, (u32)l, sav_type); }
    cardWriteEeprom((u32)(k * SAV_STRIDE), sav_buf, (u32)hs, sav_type);   // the header goes last: until it lands the old copy is still the good one
    memcpy(sav_sh[k], sav_buf, (size_t)n);
    static u8 chk[SAV_MAX]; cardReadEeprom((u32)(k * SAV_STRIDE), chk, (u32)n, sav_type); if (memcmp(chk, sav_buf, (size_t)n)) { sav_probe(); return 0; }
    sav_have = k; sav_seq++; return 1;
}
static int sav_apply(void) {   // 1 = loaded
    if (sav_have < 0) return 0; SavHdr h; memcpy(&h, sav_sh[sav_have], sizeof h);
    int rl = unrle(sav_sh[sav_have] + sizeof h, h.clen, sav_raw, sizeof sav_raw); if (rl != h.rawlen || rl < (int)(sizeof(SavCore) + sizeof sname + sizeof sims + sizeof crt)) return 0;
    SavCore c; int n = 0; memcpy(&c, sav_raw, sizeof c); n = (int)sizeof c;
    if (c.nsim < 1 || c.nsim > SIM_MAX || c.ctl < 0 || c.ctl >= c.nsim || c.nlots < 0 || c.nlots > LSV_N || c.lot_bx < -1 || (c.lot_bx >= BM_N && c.lot_bx < 2000) || c.lot_bx >= 2040 || c.lot_by < -1 || (c.lot_by >= BM_N && c.lot_by < 2000) || c.lot_by >= 2040) return 0;
    static Sim ts[SIM_MAX]; static char tn[SIM_MAX][20]; static CRel tc[64]; static LotSave tl[LSV_N]; int tn_l = 0;
    memcpy(tn, sav_raw + n, sizeof tn); n += (int)sizeof tn; memcpy(ts, sav_raw + n, sizeof ts); n += (int)sizeof ts; memcpy(tc, sav_raw + n, sizeof tc); n += (int)sizeof tc;
    for (int i = 0; i < c.nlots; i++) { SavLot sl; if (n + (int)sizeof sl + 533 > rl) return 0; memcpy(&sl, sav_raw + n, sizeof sl); n += (int)sizeof sl; int rf = (sl.nobj >> 12) & 3, upv = (sl.nobj >> 14) & 3, hu = (sl.nobj >> 11) & 1; sl.nobj &= 0x7FF; if (sl.nobj > 400) return 0; LotSave *L = &tl[tn_l]; L->roof = (u8)rf; L->up = (u8)upv; L->hasup = 0; memset(L->ufl, 0, sizeof L->ufl); memset(L->uhw, 0, sizeof L->uhw); memset(L->uvw, 0, sizeof L->uvw);
        L->bx = sl.bx; L->by = sl.by; L->pcx = sl.pcx; L->pcy = sl.pcy; L->dcx = sl.dcx; L->dcy = sl.dcy; L->nobj = sl.nobj;
        memcpy(L->flr, sav_raw + n, sizeof L->flr); n += (int)sizeof L->flr; memcpy(L->hwl, sav_raw + n, sizeof L->hwl); n += (int)sizeof L->hwl; memcpy(L->vwl, sav_raw + n, sizeof L->vwl); n += (int)sizeof L->vwl;
        if (n + sl.nobj * 5 > rl) return 0; memset(L->objs, 0, sizeof L->objs); memcpy(L->objs, sav_raw + n, (size_t)sl.nobj * 5); n += sl.nobj * 5; { int k2 = 0; for (int q = 0; q < L->nobj; q++) if (L->objs[q].c * 4 + L->objs[q].i < NITEM) L->objs[k2++] = L->objs[q]; L->nobj = k2; }
        if (hu) { if (n + 1 > rl) return 0; int nu = sav_raw[n++]; if (nu > UPN || n + nu * 533 > rl) return 0; L->hasup = 1;   // (D36) the upper storeys (a save from before: none, so the old shell is turned into floors on entering)
            for (int u = 0; u < nu; u++) { memcpy(L->ufl[u], sav_raw + n, sizeof L->ufl[u]); n += (int)sizeof L->ufl[u]; memcpy(L->uhw[u], sav_raw + n, sizeof L->uhw[u]); n += (int)sizeof L->uhw[u]; memcpy(L->uvw[u], sav_raw + n, sizeof L->uvw[u]); n += (int)sizeof L->uvw[u]; } }
        tn_l++; }
    nown = 0; memset(life, 0, sizeof life); memset(gh, 0, sizeof gh); ngh = 0; dog_have = 0; dog.hun = 80; life_ver++;
    if (n + 4 <= rl) { u16 th[2]; memcpy(th, sav_raw + n, 4); if (th[0] == 0x504F && th[1] <= OWN_MAX && n + 4 + th[1] * 4 <= rl) { memcpy(own, sav_raw + n + 4, (size_t)th[1] * 4); nown = th[1]; n += 4 + th[1] * 4; } }
    if (n + 4 <= rl) { u16 th[2]; memcpy(th, sav_raw + n, 4); if (th[0] == 0x4C46 && th[1] == sizeof(LifeSave) && n + 4 + (int)sizeof(LifeSave) <= rl) { LifeSave ls; memcpy(&ls, sav_raw + n + 4, sizeof ls); memcpy(life, ls.life, sizeof life); memcpy(gh, ls.gh, sizeof gh); ngh = ls.ngh > GH_MAX ? GH_MAX : ls.ngh; dog_have = ls.dog; dog.hun = ls.hun; n += 4 + (int)sizeof(LifeSave); } }
    u8 ox[32]; int ox_have = 0, ox_n = 0; if (n + 4 <= rl) { u16 th[2]; memcpy(th, sav_raw + n, 4); if (th[0] == 0x584F && th[1] <= 32 && n + 4 + th[1] <= rl) { ox_n = th[1]; memcpy(ox, sav_raw + n + 4, (size_t)ox_n); ox_have = 1; n += 4 + ox_n; } }
    int hm_x = -1, hm_y = -1; if (n + 8 <= rl) { s16 th[4]; memcpy(th, sav_raw + n, 8); if (th[0] == 0x4D48 && th[1] == 4) { hm_x = th[2]; hm_y = th[3]; n += 8; } }
    zone = 0; isl_wood = isl_stone = isl_leaves = isl_spear = isl_shells = 0; isl_food = 3; isl_secrets = 0;
    u8 lvs[SIM_MAX]; memset(lvs, 0, sizeof lvs); if (n + 4 + SIM_MAX <= rl) { u16 th[2]; memcpy(th, sav_raw + n, 4); if (th[0] == 0x4C53 && th[1] == SIM_MAX) { memcpy(lvs, sav_raw + n + 4, SIM_MAX); n += 4 + SIM_MAX; } }   // (D36; before: everyone on the ground)
    if (n + 20 <= rl) { s16 zs[10]; memcpy(zs, sav_raw + n, 20); if (zs[0] == 0x4E5A && zs[1] == 16 && zs[2] >= 0 && zs[2] < Z_N) { zone = zs[2]; isl_wood = zs[3]; isl_stone = zs[4]; isl_leaves = zs[5]; isl_food = zs[6]; isl_spear = zs[7]; isl_shells = zs[8]; isl_secrets = (u16)zs[9]; n += 20; } }   // (D37)
    farm_reset(); if (n + 4 <= rl) { u16 th[2]; memcpy(th, sav_raw + n, 4); if (th[0] == 0x4D46 && th[1] == sizeof(FarmSave) && n + 4 + (int)sizeof(FarmSave) <= rl) { FarmSave f; memcpy(&f, sav_raw + n + 4, sizeof f); fm_unpack(&f); n += 4 + (int)sizeof f; } }   // (D38; before: no farm)
    n += mg_unpack(sav_raw + n, rl - n);   // (D44 on; before: none of it)
    seed = c.seed; bm_new((u32)seed); pop_build(); cur_set = c.cur_set >= 0 && c.cur_set < BM_N * BM_N ? c.cur_set : bm_capital();
    st_days = c.days; st_born = c.born; st_died = c.died; minutes = c.minutes; funds = c.funds; nsim = c.nsim; ctl = c.ctl; cur_sim = c.cur_sim >= 0 && c.cur_sim < nsim ? c.cur_sim : ctl;
    for (int i = 0; i < NOPT && i < OPT_OLD; i++) if (OPT[i].v && c.opt[i] < opt_cnt(i)) *OPT[i].v = c.opt[i];
    for (int i = OPT_OLD; i < NOPT; i++) if (OPT[i].v) *OPT[i].v = ox_have && i - OPT_OLD < ox_n && ox[i - OPT_OLD] < opt_cnt(i) ? ox[i - OPT_OLD] : OPT[i].def;   // (a save from before D33: the new ones at their defaults)
    o_dirty = 1; memcpy(sname, tn, sizeof sname); memcpy(sims, ts, sizeof sims); memcpy(crt, tc, sizeof crt); for (int i = 0; i < tn_l; i++) lsv[i] = tl[i]; lsv_n = tn_l; lsv_next = 0;
    for (int i = 0; i < nsim; i++) { Sim *m = &sims[i]; m->act = -1; m->act_left = 0; m->soc = -1; m->tgt = 0; m->tx = m->wx = m->x; m->ty = m->wy = m->y; }
    memset(ilim, 0, sizeof ilim); memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; pick = -1; lot_bx = -1; lot_by = -1; nb_ok = 0;
    if (!nown) { for (int i = 0; i < lsv_n; i++) own_add(lsv[i].bx, lsv[i].by); if (c.lot_bx >= 0) own_add(c.lot_bx, c.lot_by); }   // (a save from before lots could be bought: everything you had is yours)
    home_bx = hm_x; home_by = hm_y; if (!lot_owned(home_bx, home_by)) { home_bx = home_by = -1; if (c.lot_bx >= 0 && lot_owned(c.lot_bx, c.lot_by)) { home_bx = c.lot_bx; home_by = c.lot_by; } else if (nown) { home_bx = own[0].bx; home_by = own[0].by; } }   // (a save from before D33: home is where you were, or your first lot)
    if (c.lot_bx >= 0) { lot_enter(c.lot_bx, c.lot_by); sel_x = c.lot_bx < 2000 ? c.lot_bx : home_bx; sel_y = c.lot_bx < 2000 ? c.lot_by : home_by; for (int i = 0; i < SIM_MAX; i++) slv[i] = lvs[i] <= UPN ? lvs[i] : 0; }
    return 1;
}
static void gt_day(void);
static void day_roll(void) {
    st_days++; memset(ilim, 0, sizeof ilim); farm_midnight(); gt_day(); for (int k = 0; k < 64; k++) crt[k].talks = 0;
    for (int i = 0; i < nsim; i++) { Sim *m = &sims[i]; int s0 = stage_of(m->age); if (m->age < 99) m->age++; int s1 = stage_of(m->age); if (s0 != s1) { static char bf[40]; sprintf(bf, "%s is now a %s", sname[i], STGN[s1]); say(bf); } }
    for (int i = 0; i < nsim; i++) if (sims[i].age >= 80 && nsim > 1) { int alive = 0; for (int q = 0; q < nsim; q++) if (q != i && stage_of(sims[q].age) > 0) alive++; if (alive && (int)(hsh((u32)(minutes + i * 77)) % 100) < (sims[i].age - 79) * 7) { sim_die(i); break; } }
    if (o_auto && mode == M_LIVE && sav_ok() && lot_bx >= 0) { if (sav_write()) say("Saved. Sweet dreams!"); }
    sfx(4);
}
static const int SFX_[4] = { 0, 1, 0, -1 }, SFY_[4] = { 1, 0, -1, 0 };   // a flight's front (its open side) by its turn; it rises away from it
static void stairs_go(int i, int o, int down) {   // (D36) the sim at the foot (or at the landing) changes storey
    Sim *m = &sims[i]; const Obj *st = &objs[o]; int L = OBJ_LV(st), r = st->rot & 3, tx = down ? st->x + SFX_[r] : st->x - SFX_[r], ty = down ? st->y + SFY_[r] : st->y - SFY_[r], tl_ = down ? L : L + 1;
    int keep = LV; LV = tl_; int ok = tx >= 0 && ty >= 0 && tx < GN && ty < GN && !obj_solid(tx, ty); LV = keep;
    if (!ok) { if (i == ctl) say(down ? "Something is in the way down there" : "Build a floor where the stairs come out"); return; }
    slv[i] = (u8)tl_; m->x = m->tx = m->wx = tx * 16; m->y = m->ty = m->wy = ty * 16; m->act = -1; m->act_left = 0; sfx(0);
}
static void start_act(Sim *m, int obj, int sim) {   // walk to it, then do it
    if (sim >= 0) { start_soc((int)(m - sims), sim, S_CHAT); }
    else if (OBJ_ID(&objs[obj]) == ID_STAIRS) { const Obj *o = &objs[obj]; m->tx = o->x * 16; m->ty = o->y * 16; m->act = 50; m->act_amt = 0; m->face = 1; m->act_obj = obj; }   // (up: to the foot of the flight)
    else { const Obj *o = &objs[obj]; const Item *a = IT_OF(o); if (!((a->mask >> stage_of(m->age)) & 1)) { say(stage_of(m->age) < 3 ? "Too young for that" : "Not for your age"); return; } m->tx = o->x * 16; m->ty = o->y * 16; m->act = a->need; m->act_amt = a->amt; m->face = a->dur; m->act_obj = obj; }
}
static int obj_solid(int cx, int cy) { if (cx < 0 || cy < 0 || cx >= GN || cy >= GN) return LV ? 1 : far_solid(cx, cy); if (LV && !uflr[LV - 1][cy][cx]) return 1; if (!occ_ok) occ_build(); return occ[LV][cy][cx] >> 15; }   // (upstairs: only the floor there is)
static int edge_open(int ax, int ay, int bx2, int by) {   // can a sim step between two neighbouring cells? walls block, doors do not
    int ina = ax >= 0 && ay >= 0 && ax < GN && ay < GN, inb = bx2 >= 0 && by >= 0 && bx2 < GN && by < GN;
    if (LV && (!ina || !inb)) return 0;   // (upstairs: the plot is all there is)
    if (!ina || !inb) { if (ina == inb || zone) return 1; return !(by != ay && (ay == GN - 1 || by == GN - 1) && bx2 != door_cx && ax != door_cx) ? 1 : 0; }   // out of the plot: open, but the back fence has one gap, at the path
    int w; u8 (*hw)[GN] = HWv(LV); u8 (*vw)[GN + 1] = VWv(LV);
    if (bx2 > ax) w = vw[ay][ax + 1]; else if (bx2 < ax) w = vw[ay][ax]; else if (by > ay) w = hw[ay + 1][ax]; else w = hw[ay][ax];
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
// ---- the dog and the ghosts (D31) ----
static void dog_reset(void) { const Sim *m = &sims[ctl]; dog.x = dog.wx = dog.tx = fdiv(m->x + 8, 16) * 16 + 16; dog.y = dog.wy = dog.ty = fdiv(m->y + 8, 16) * 16; dog.hun = 80; dog.fx = 1; dog.fz = 0; if (obj_solid(dog.x / 16, dog.y / 16)) { dog.x = dog.wx = dog.tx = fdiv(m->x + 8, 16) * 16; } }
static void dog_step(int frames) {
    if (!dog_have) return;
    if ((frames % 500) == 0 && dog.hun > 0) { dog.hun--; if (dog.hun == 20) say("Rex is hungry! Woof!"); }
    if (dog.x == dog.tx && dog.y == dog.ty && ((frames * 7 + 3) % 90) == 0) { const Sim *m = &sims[ctl]; int cx = fdiv(m->x + 8, 16) + (int)(hsh((u32)frames) % 7) - 3, cy = fdiv(m->y + 8, 16) + (int)(hsh((u32)frames * 3 + 1) % 7) - 3; if (!obj_solid(cx, cy)) { dog.tx = cx * 16; dog.ty = cy * 16; } }
    if (dog.x != dog.tx || dog.y != dog.ty) {
        if ((dog.x & 15) == 0 && (dog.y & 15) == 0 && dog.wx == dog.x && dog.wy == dog.y) { int nx, ny; if (next_cell(fdiv(dog.x + 8, 16), fdiv(dog.y + 8, 16), fdiv(dog.tx + 8, 16), fdiv(dog.ty + 8, 16), &nx, &ny)) { dog.wx = nx * 16; dog.wy = ny * 16; } else { dog.tx = dog.x; dog.ty = dog.y; return; } }
        if (dog.x < dog.wx) { dog.x += 2; dog.fx = 1; dog.fz = 0; } else if (dog.x > dog.wx) { dog.x -= 2; dog.fx = -1; dog.fz = 0; } else if (dog.y < dog.wy) { dog.y += 2; dog.fx = 0; dog.fz = 1; } else if (dog.y > dog.wy) { dog.y -= 2; dog.fx = 0; dog.fz = -1; }
    }
}
static int ground_lift(int cx, int cy, int L);
static void dog_draw(void) {
    if (!dog_have) return; gy_off = -ground_lift(fdiv(dog.x + 8, 16), fdiv(dog.y + 8, 16), 0);
    int x = dog.x * 2 * T / 16 - GN * T + T, z = dog.y * 2 * T / 16 - GN * T + T, fx = dog.fx, fz = dog.fz; u16 c = C15(20, 13, 6), d = C15(13, 8, 3), k = C15(3, 3, 4);
    int lx = fx ? 95 : 45, lz = fx ? 45 : 95;
    for (int a = -1; a <= 1; a += 2) for (int b = -1; b <= 1; b += 2) { int px = x + (fx ? a * 70 : b * 30), pz = z + (fx ? b * 30 : a * 70); block(px - 14, pz - 14, px + 14, pz + 14, 0, -50, d, d); }
    block(x - lx, z - lz, x + lx, z + lz, -50, -120, c, shade(c, 10));
    int hx = x + fx * 110, hz = z + fz * 110; block(hx - 42, hz - 42, hx + 42, hz + 42, -90, -170, c, shade(c, 10));
    int sx = hx + fx * 46, sz = hz + fz * 46; block(sx - 18, sz - 18, sx + 18, sz + 18, -100, -135, k, k);
    block(hx - 36, hz - 36, hx - 20, hz - 20, -170, -205, d, d); block(hx + 20, hz + 20, hx + 36, hz + 36, -170, -205, d, d);
    int tx2 = x - fx * 100, tz2 = z - fz * 100; block(tx2 - 10, tz2 - 10, tx2 + 10, tz2 + 10, -110, -165, d, d); gy_off = 0;
}
static int night_now(void) { int hr = (minutes / 60) % 24; return hr >= 20 || hr < 5; }
static void ghost_add(const Sim *m) { Ghost *g = &gh[gh_next++ % GH_MAX]; if (ngh < GH_MAX) ngh++; g->bx = (s16)lot_bx; g->by = (s16)lot_by; g->x = g->tx = m->x; g->y = g->ty = m->y; g->skin = m->skin; g->hair = m->hair; g->shirt = m->shirt; g->on = 1; }
static void ghost_step(int frames) {
    if (!ngh || !night_now()) return;
    for (int k = 0; k < GH_MAX; k++) { Ghost *g = &gh[k]; if (!g->on || g->bx != lot_bx || g->by != lot_by) continue;
        if ((frames & 1) == 0) { if (g->x < g->tx) g->x++; else if (g->x > g->tx) g->x--; if (g->y < g->ty) g->y++; else if (g->y > g->ty) g->y--; }
        if (g->x == g->tx && g->y == g->ty && ((frames + k * 31) % 60) == 0) { g->tx = (int)(hsh((u32)(frames * 5 + k)) % (GN * 16)); g->ty = (int)(hsh((u32)(frames * 9 + k + 7)) % (GN * 16)); }
        if (((frames + k * 17) % 240) == 0) for (int i = 0; i < nsim; i++) if (abs(sims[i].x - g->x) < 48 && abs(sims[i].y - g->y) < 48) { sims[i].need[1] = cl100(sims[i].need[1] - 15); sims[i].need[3] = cl100(sims[i].need[3] - 5); if (i == ctl) say("Boo! A ghost!"); life_fear(i, FR_GHOST); break; } }
}
static void ghost_draw(void) {
    if (!ngh || !night_now()) return; int any = 0; for (int k = 0; k < GH_MAX; k++) if (gh[k].on && gh[k].bx == lot_bx && gh[k].by == lot_by) any = 1; if (!any) return;
    gx_fmt((gx_base & ~(u32)0x3F1F0000) | POLY_ALPHA(18) | POLY_ID(2));   // (D39: recorded with the rest)
    for (int k = 0; k < GH_MAX; k++) { const Ghost *g = &gh[k]; if (!g->on || g->bx != lot_bx || g->by != lot_by) continue;
        int x = g->x * 2 * T / 16 - GN * T + T, z = g->y * 2 * T / 16 - GN * T + T, bob = sinLerp(gframes * 300 + k * 900) * 40 / 4096; u16 c = C15(27, 29, 31), e = C15(2, 2, 5);
        block(x - 120, z - 120, x + 120, z + 120, -100 - bob, -560 - bob, c, c); block(x - 100, z - 100, x + 100, z + 100, -560 - bob, -760 - bob, c, c);
        block(x - 60, z + 100, x - 20, z + 106, -640 - bob, -690 - bob, e, e); block(x + 20, z + 100, x + 60, z + 106, -640 - bob, -690 - bob, e, e);
        block(x - 60, z - 106, x - 20, z - 100, -640 - bob, -690 - bob, e, e); block(x + 20, z - 106, x + 60, z - 100, -640 - bob, -690 - bob, e, e); }
    gx_fmt(gx_base);
}
static int arrive(int i) {   // at the thing: start doing it (0: nothing to do after all)
    Sim *m = &sims[i];
    if (m->act == 80) { if (isl_food > 0) { isl_food--; m->act = 0; m->act_amt = 70; m->face = 150; life_ver++; } else { if (i == ctl) say("Tummy rumbles... try berries or fish"); m->act = -1; return 0; } }   // (the castaway's meal: from what was gathered)
    m->act_left = m->face;
    if (m->act >= 100) { if (m->tgt >= 0) { Sim *o = &sims[m->tgt]; o->act_left = m->face; o->act = 3 + 100; o->act_amt = 0; } m->act = 3; apply_soc(i, m->tgt, m->soc); }
    return 1;
}
// ---- (D44) Real time: the DS's own clock (the ARM7 reads the clock chip into the transfer region every second: tools/dsbuild/arm7/main7.c).
// Local time as seconds since 1 Jan 1970; the game's clock is put to its weekday and minute. Minutes gone by since the last look (the DS
// was off, a save loaded) are played through, a day at most (bills, pay, birthdays at midnight); the rest is skipped.
static void rt_sync(void) {
    u32 t = *(volatile u32 *)0x02FFF010; if (t < 946684800u) return;   // (libnds' __transferRegion()->unixTime; before 2000: the clock is not set)
    u32 dn = t / 86400u; int mn = (int)(t % 86400u / 60u), y = 1970, m = 0, d = (int)dn;
    while (d >= 365 + (!(y & 3) && (y % 100 || !(y % 400)))) d -= 365 + (!(y & 3) && (y % 100 || !(y % 400))), y++;
    for (;; m++) { int ml = m == 1 ? 28 + (!(y & 3) && (y % 100 || !(y % 400))) : 31 - ((m + (m >= 7)) & 1); if (d < ml) break; d -= ml; }
    rt_mon = m; rt_md = d + 1;
    int want = (int)((dn + 3) % 7) * 1440 + mn, cur = ((minutes / 1440) % 7) * 1440 + minutes % 1440, gap = (want - cur + 10080) % 10080;   // (1 Jan 1970 was a Thursday)
    if (rt_jump) { rt_jump = 0; minutes += gap; return; }
    if (gap > 10080 - 1440) { minutes -= 10080 - gap; return; }   // (the clock went back: straight there)
    if (gap > 1440) { minutes += gap - 1440; gap = 1440; }
    while (gap-- > 0) { minutes++; gt_minute(); if (minutes % 1440 == 0) day_roll(); }
}
static void sims_step(int frames) {
    int keep_lv = LV;
    for (int i = 0; i < nsim; i++) { Sim *m = &sims[i]; LV = slv[i];   // (D36: each walks on its own storey)
        if ((m->act == 50 || m->act == 51) && !m->act_left && m->x == m->tx && m->y == m->ty) { stairs_go(i, m->act_obj, m->act == 51); continue; }   // (already at the foot, or on the landing)
        if (m->act_left > 0) {
            for (int n = 0; n < 6; n++) if (n == (m->act >= 100 ? m->act - 100 : m->act)) { m->acc[n] += m->act_amt; while (m->acc[n] >= m->face) { m->acc[n] -= m->face; if (m->need[n] < 100) m->need[n]++; } }
            m->act_left--; if (m->act_left == 0) { int an = m->act; m->act = -1; if (i == ctl) say("Done"); if (an >= 60 && an < 80) isl_done(i, an - 60, m->act_obj); if (an == 0) life_ev(i, WN_EAT, 0); else if (an == 1) life_ev(i, WN_FUN, 0); else if (an == 2 && m->act_amt >= 80) life_ev(i, WN_SLEEP, 0); else if (an == 5 && m->act_amt >= 70) life_ev(i, WN_SHOWER, 0); }
        } else if (m->x != m->tx || m->y != m->ty) {
            if ((m->x & 15) == 0 && (m->y & 15) == 0 && m->wx == m->x && m->wy == m->y) {   // on a cell centre: plan the next cell round the walls
                int nx, ny; if (next_cell(fdiv(m->x + 8, 16), fdiv(m->y + 8, 16), fdiv(m->tx + 8, 16), fdiv(m->ty + 8, 16), &nx, &ny)) { m->wx = nx * 16; m->wy = ny * 16; }
                else { m->tx = m->x; m->ty = m->y; m->act = -1; if (i == ctl) say("Cannot get there"); continue; } }
            if (m->x < m->wx) { m->x++; m->dirn = 8192; } else if (m->x > m->wx) { m->x--; m->dirn = 24576; } else if (m->y < m->wy) { m->y++; m->dirn = 0; } else if (m->y > m->wy) { m->y--; m->dirn = 16384; }
            if (m->x == m->tx && m->y == m->ty && (m->act == 50 || m->act == 51)) { stairs_go(i, m->act_obj, m->act == 51); continue; }
            if (m->x == m->tx && m->y == m->ty && m->act >= 0) { if (!arrive(i)) continue; }
        } else if (m->act >= 0 && m->act != 50 && m->act != 51) { if (!arrive(i)) continue;   // (already there)
        } else if (i != ctl && stage_of(m->age) > 0 && ((frames + i * 37) & 127) == 0) { m->tx = (1 + (frames * 7 + i * 5) % 11) * 16; m->ty = (1 + (frames * 3 + i * 9) % 12) * 16; m->act = -1; }
        { static const u8 NP[4] = { 0, 140, 70, 47 }; int np = NP[o_need & 3] * (o_rt ? 225 : 1);   // (Options > Game > Needs speed: never fall, slow, normal, fast = frames per point; Real time: a real minute takes what two of a normal day's game minutes do)
          if (np && ((frames + i * 9) % np) == 0) for (int n = 0; n < 6; n++) if (m->need[n] > 4 && n != 3) m->need[n]--;
          if (np && ((frames + i * 9) % (np * 2)) == 0 && m->need[3] > 4) m->need[3]--;
          if (np) for (int k = 0; k < 3; k++) { int t = csx[i].tr[k]; if (t < 1 || t > 12) continue; int n = TRE[t - 1][0], fast = TRE[t - 1][1], per = n == 3 ? np * 4 : np * 2;   // (D67) a trait: one need falls half as fast again, or half as fast (the social need, which falls half as often, stops falling)
              if (((frames + i * 9) % per) == 0) { if (fast) { if (m->need[n] > 4) m->need[n]--; } else if (m->need[n] < 100) m->need[n]++; } } }
        if (((frames + i * 9) % 70) == 5 && !m->act_left) {
            if (m->need[2] <= 6) { m->need[2] = 35; m->need[1] = cl100(m->need[1] - 15); if (i == ctl) say("Zzz... passed out!"); life_fear(i, FR_PASS); }
            if (m->need[4] <= 6) { m->need[4] = 60; m->need[5] = cl100(m->need[5] - 25); m->need[1] = cl100(m->need[1] - 10); if (i == ctl) say("Oh no, an accident!"); life_fear(i, FR_WET); } }
    }
    LV = 0; dog_step(frames); ghost_step(frames); LV = keep_lv; if ((frames & 63) == 0) life_tick();
    { static int was; if (o_rt && !was) rt_jump = 1; was = o_rt; }   // (just switched on: straight to the real time)
    if (o_rt) { if (rt_jump || (frames & 63) == 0) rt_sync(); }
    else { static const u8 DP[4] = { 66, 100, 150, 250 }; static int dacc; int per = 8 * DP[o_day & 3];   // (Options > Game > Day length: a game minute every 5.3, 8, 12 or 20 steps)
      dacc += 100; if (dacc >= per) { dacc -= per; if (dacc >= per) dacc = 0; minutes++; gt_minute(); if (minutes % 1440 == 0) day_roll(); } }
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
    int st = hair2d(m->hair); if (st == 5) return;
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
static void beard3d(int bd, u16 hc, u16 sk, int hw, int hz) {
    if (bd == 1) fq(-hw + 60, 60, hw - 60, 560, hz + 4, lit(sk, 10));
    if (bd >= 4) {   // (D61) a full beard in pieces: the jaw, sideburns up to the ears, a moustache over the lip, and a chin that narrows (long: three steps)
        bx(-hw - 20, -60, -hz + 40, hw + 20, 470, hz + 70, hc);
        bx(-hw - 20, 330, -hz + 40, -hw + 150, 760, hz - 160, hc); bx(hw - 150, 330, -hz + 40, hw + 20, 760, hz - 160, hc);
        fq(-300, 470, 300, 580, hz + 78, shade(hc, 5)); fq(-hw + 120, 140, hw - 120, 470, hz + 76, hc);
        if (bd == 5) { bx(-hw + 60, -420, -hz + 120, hw - 60, -60, hz + 90, hc); bx(-hw + 260, -700, -hz + 220, hw - 260, -420, hz + 100, hc); bx(-hw + 440, -900, -hz + 340, hw - 440, -700, hz + 110, hc); }
        else bx(-hw + 200, -240, -hz + 120, hw - 200, -60, hz + 90, hc);
    }
    if (bd == 2 || bd == 3) { fq(-260, 540, 260, 650, hz + 8, hc); fq(-320, 470, -200, 650, hz + 9, hc); fq(200, 470, 320, 650, hz + 9, hc); fq(-90, 500, 90, 560, hz + 10, shade(hc, 6)); }
    if (bd == 3) { bx(-170, 20, hz - 80, 170, 480, hz + 70, hc); bx(-100, -140, hz - 40, 100, 20, hz + 80, hc); }
}
static int sq_q, fq4[SIM_MAX], sb_sc = 100;
ITCM_CODE static void sb(int ox, int oz, int lx0, int lu0, int lz0, int lx1, int lu1, int lz1, u16 c) {   // a part in the sim's own frame (z = front), turned to face where it walks
    int ax, az, bx2, bz; lx0 = lx0 * sb_sc / 100; lx1 = lx1 * sb_sc / 100; lu0 = lu0 * sb_sc / 100; lu1 = lu1 * sb_sc / 100; lz0 = lz0 * sb_sc / 100; lz1 = lz1 * sb_sc / 100;
    switch (sq_q & 3) { case 1: ax = lz0; az = -lx0; bx2 = lz1; bz = -lx1; break; case 2: ax = -lx0; az = -lz0; bx2 = -lx1; bz = -lz1; break; case 3: ax = -lz0; az = lx0; bx2 = -lz1; bz = lx1; break; default: ax = lx0; az = lz0; bx2 = lx1; bz = lz1; }
    bx(ox + (ax < bx2 ? ax : bx2), lu0, oz + (az < bz ? az : bz), ox + (ax < bx2 ? bx2 : ax), lu1, oz + (az < bz ? bz : az), c);
}
// (D62) HAIR FROM THE PS1'S PIECES: each piece of a style (ds_hair.h) is a box that may narrow towards the top and the bottom, drawn as five shaded faces.
// (ox, oz, q, sc place it like sb(); hwx/hwz/ht size the head; hu is the crown's height.) Hanging ends (F_HTP) narrow to points, a bun or knot is just another piece.
static void hp_emit(const HP *p, int mir, int ox, int oz, int q, int sc, int hwx, int hwz, int hu, int ht, u16 hc) {
    int x0 = p->x0, x1 = p->x1, y0 = p->y0, y1 = p->y1, z0 = p->z0, z1 = p->z1, fl = p->fl, ti = p->in >> 4, bi = (p->in & 15) + ((fl & F_HTP) ? 4 : 0);
    if (mir) { int t = x0; x0 = -x1; x1 = -t; }
    { int lim = ((x1 - x0 < z1 - z0 ? x1 - x0 : z1 - z0) - 1) / 2; if (lim < 0) lim = 0; if (ti > lim) ti = lim; if (bi > lim) bi = lim; }
    int X[8], U[8], Z[8];
    for (int k = 0; k < 4; k++) {
        int tx = (k & 1) ? x1 - ti : x0 + ti, tz = (k & 2) ? z1 - ti : z0 + ti, bx_ = (k & 1) ? x1 - bi : x0 + bi, bz_ = (k & 2) ? z1 - bi : z0 + bi;
        int lx[2] = { tx * hwx / 32, bx_ * hwx / 32 }, lz[2] = { tz * hwz / 32, bz_ * hwz / 32 }, lu[2] = { hu - y0 * ht / 64, hu - y1 * ht / 64 };
        for (int h = 0; h < 2; h++) { int a = lx[h] * sc / 100, c = lz[h] * sc / 100, rx, rz;
            switch (q & 3) { case 1: rx = c; rz = -a; break; case 2: rx = -a; rz = -c; break; case 3: rx = -c; rz = a; break; default: rx = a; rz = c; }
            X[h * 4 + k] = ox + rx; Z[h * 4 + k] = oz + rz; U[h * 4 + k] = lu[h] * sc / 100; } }
    u16 tc = (fl & F_HDK) ? shade(hc, 6) : (fl & F_HLT) ? shade(hc, 11) : hc;
    #define HV(k) X[k], -U[k], Z[k]
    #define HQ(a, b, c, d, col) qd(HV(a), HV(b), HV(c), HV(d), col)
    if (!(fl & F_HNB)) HQ(4, 5, 1, 0, shade(tc, 7));          // back -z
    if (!(fl & F_HNS)) { HQ(5, 7, 3, 1, shade(tc, 6)); HQ(6, 4, 0, 2, shade(tc, 6)); }   // +x, -x
    if (!(fl & F_HNF)) HQ(7, 6, 2, 3, tc);                    // front +z
    if (!(fl & F_HNT)) HQ(0, 1, 3, 2, shade(tc, 10));         // top
    if (fl & F_HBO) HQ(6, 7, 5, 4, shade(tc, 5));             // underneath
    #undef HQ
    #undef HV
}
static void hair_pieces(int h, int stg, int ox, int oz, int q, int sc, int hwx, int hwz, int hu, int ht, u16 hc) {   // a whole style (stage 0: the baby's tuft)
    if (h % NHAIRD == 5) return;
    const HP *pc; int n;
    if (stg == 0) { pc = hp_baby; n = (int)(sizeof hp_baby / sizeof(HP)); hc = shade(hc, 11); } else { const HStyleD *hs = &hdstyle[DS2PS[h % NHAIRD]]; pc = hs->p; n = hs->n; }
    for (int i = 0; i < n; i++) { hp_emit(&pc[i], 0, ox, oz, q, sc, hwx, hwz, hu, ht, hc); if (pc[i].fl & F_HM) hp_emit(&pc[i], 1, ox, oz, q, sc, hwx, hwz, hu, ht, hc); }
}
// (D67) HATS (Create-a-Sim's Hair > Hat): a few boxes on the crown. mode 0 draws in a sim's own frame (sb), mode 1 on the Create-a-Sim figure (bx); hw, hz: half width and depth of the head, top: its top, ht: its height
static void hat_box(int m, int ox, int oz, int x0, int u0, int z0, int x1, int u1, int z1, u16 c) { if (m) bx(x0, u0, z0, x1, u1, z1, c); else sb(ox, oz, x0, u0, z0, x1, u1, z1, c); }
static void hat_draw(int h, int m, int ox, int oz, int hw, int hz, int top, int ht) {
    const u16 RD = C15(26, 6, 6), BL = C15(8, 12, 28), BK = C15(3, 3, 5), WH = C15(31, 31, 31), PK = C15(31, 12, 22), YL = C15(31, 26, 6), GD = C15(31, 25, 4);
    #define HB(a, b, c, d, e, f, g) hat_box(m, ox, oz, a, b, c, d, e, f, g)
    switch (h) {
    case 1: HB(-hw - hw / 14, top - ht * 22 / 100, -hz - hz / 14, hw + hw / 14, top + ht * 10 / 100, hz + hz / 14, RD); HB(-hw, top - ht * 20 / 100, hz, hw, top - ht * 13 / 100, hz + hz * 9 / 10, C15(18, 3, 4)); break;
    case 2: HB(-hw - hw / 12, top - ht * 36 / 100, -hz - hz / 12, hw + hw / 12, top + ht * 14 / 100, hz + hz / 12, BL); HB(-hw - hw / 8, top - ht * 38 / 100, -hz - hz / 8, hw + hw / 8, top - ht * 26 / 100, hz + hz / 8, C15(5, 8, 22)); HB(-hw / 6, top + ht * 14 / 100, -hz / 6, hw / 6, top + ht * 30 / 100, hz / 6, WH); break;
    case 3: HB(-hw - hw * 3 / 10, top - ht * 6 / 100, -hz - hz * 3 / 10, hw + hw * 3 / 10, top + ht * 4 / 100, hz + hz * 3 / 10, BK); HB(-hw * 8 / 10, top, -hz * 8 / 10, hw * 8 / 10, top + ht * 80 / 100, hz * 8 / 10, BK); HB(-hw * 82 / 100, top + ht * 4 / 100, -hz * 82 / 100, hw * 82 / 100, top + ht * 14 / 100, hz * 82 / 100, C15(24, 4, 6)); break;
    case 4: HB(-hw * 6 / 10, top - ht * 4 / 100, -hz * 6 / 10, hw * 6 / 10, top + ht * 22 / 100, hz * 6 / 10, PK); HB(-hw * 4 / 10, top + ht * 22 / 100, -hz * 4 / 10, hw * 4 / 10, top + ht * 42 / 100, hz * 4 / 10, YL); HB(-hw * 2 / 10, top + ht * 42 / 100, -hz * 2 / 10, hw * 2 / 10, top + ht * 62 / 100, hz * 2 / 10, PK); HB(-hw / 10, top + ht * 62 / 100, -hz / 10, hw / 10, top + ht * 72 / 100, hz / 10, WH); break;
    case 5: HB(-hw - hw / 20, top - ht * 2 / 100, -hz - hz / 20, hw + hw / 20, top + ht * 16 / 100, hz + hz / 20, GD);
        for (int s = -1; s <= 1; s++) HB(s * hw * 7 / 10 - hw / 8, top + ht * 16 / 100, -hz / 8, s * hw * 7 / 10 + hw / 8, top + ht * 32 / 100, hz / 8, GD);
        HB(-hw / 12, top + ht * 4 / 100, hz + hz / 20, hw / 12, top + ht * 12 / 100, hz + hz / 20 + hz / 14, RD); break;
    default: break; }
    #undef HB
}
// (D39) A SIM'S MODEL IS RECORDED ONCE (for its looks, its stride and its detail) round its own feet, facing +z, and each frame placed and turned by one matrix (lighting turns with it)
#define SML_N 24
#define SML_W 3200
typedef struct { u32 key, used; int n; u32 w[SML_W]; } SimL;
static SimL sml[SML_N] __attribute__((aligned(32))); static int sml_next; static volatile int sml_miss, sml_maxn, sml_full; static u32 gf_serial = 8;
static void sim_emit(const SimL *e, int x, int y, int z, int q) {   // into the list being recorded: push, turn by q quarters and move to x,y,z, the model, pop
    static const s8 RC[4][4] = { { 1, 0, 0, 1 }, { 0, -1, 1, 0 }, { -1, 0, 0, -1 }, { 0, 1, -1, 0 } };   // (m00 m02 m20 m22: as sb() turns a part)
    if (gxn + e->n + 40 > gx_cap) { gx_full = 1; return; }
    gx_c0(0x11); gx_c1(0x10, 2); gx_id(0x19); const s8 *r = RC[q & 3];
    u32 *o = &gxb[gxn]; o[0] = (u32)(r[0] * 4096); o[1] = 0; o[2] = (u32)(r[1] * 4096); o[3] = 0; o[4] = 4096; o[5] = 0; o[6] = (u32)(r[2] * 4096); o[7] = 0; o[8] = (u32)(r[3] * 4096); o[9] = (u32)x; o[10] = (u32)y; o[11] = (u32)z;
    if (em_R) for (int k = 0; k < 9; k++) o[k] = (u32)em_R[k];   // (D44: the rider on the board turns freely)
    gxn += 12;
    gx_c1(0x10, 1); if (gf_rec) { gf_cut(); gf_add(e->w, e->n); } else { gxk = 4; memcpy(&gxb[gxn], e->w, (size_t)e->n * 4); gxn += e->n; }   // (the model goes as a piece of its own: no copying)
    gxk = 4; gx_lv = 0; gx_mat = gx_nrm = gx_col = ~0u; gx_c1(0x12, 1);
}
static void sim3d(int i, int yaw) {
#ifdef HAIRALL
    sims[i].hair = (u8)HAIRALL;   // (test builds: -DHAIRALL=n gives every sim that style)
#endif
    const Sim *m = &sims[i]; (void)yaw; int P[PMN]; eff_pm(m->pm, P); int stg = stage_of(m->age); sb_sc = STSC[stg]; int lod = i != ctl && (abs(m->x * 2 * T / 16 - GN * T + T - camx) + abs(m->y * 2 * T / 16 - GN * T + T - camz) > 2400); if (stg < 3) { P[8] += (3 - stg) * 24; P[9] += (3 - stg) * 24; }
    if (o_sty == 1) { P[14] -= 8; P[15] -= 6; P[24] -= 8; P[25] -= 6; P[20] += 8; P[22] += 10; P[23] -= 4; P[16] += 14; P[9] -= 10; P[26] += 4; } else if (o_sty == 2) { P[14] += 12; P[15] += 10; P[24] += 12; P[22] -= 8; P[8] += 10; }
    int mv = m->tx != m->x || m->ty != m->y, ride = i == ctl && on_board; if (ride) mv = 0;
    if (mv) { int dx = (m->wx != m->x || m->wy != m->y) ? m->wx - m->x : m->tx - m->x, dy = (m->wx != m->x || m->wy != m->y) ? m->wy - m->y : m->ty - m->y; fq4[i] = (dx * dx >= dy * dy) ? (dx > 0 ? 1 : 3) : (dy > 0 ? 0 : 2); }
    sq_q = fq4[i];
    int x = ride ? sk_x3 : m->x * 2 * T / 16 - GN * T + T, z = ride ? sk_z3 : m->y * 2 * T / 16 - GN * T + T;
    u16 sk = skin_of(m), hc = stg == 5 ? C15(25, 25, 26) : hair_of(m), sh = shirt_of(m), pants = stg < 2 ? C15(24, 24, 28) : stg == 2 ? C15(8, 12, 22) : PANTC[csx[i].pants % 6], skd = shade(sk, 7);
    int hw = 100 + (P[8] - 50) / 2, ht = 200 + (P[9] - 50) / 2, lh = 320 + (P[22] - 50) * 2 + (P[26] - 50) * 3, base = 630 + (P[15] - 50) * 2 + (P[22] - 50) * 2 + (P[26] - 50) * 5,
        sw = 135 + (P[14] - 50), at = 50 + (P[21] - 50) / 2, lt = 90 + (P[23] - 50) / 2, bz = 85 + (P[24] - 50) * 2, al = 250 + (P[20] - 50) * 2, hb = base + 30;
    int sw_l = mv ? sinLerp(gframes * 1100 + i * 700) * 70 / 4096 : 0;
    int fx_e = lod ? 0 : expr_of(m), fx_b = lod || fx_e == EX_SLEEP ? 0 : (gframes + i * 41) % 130 < 5, fx_t = !lod && (m->act >= 100 || m->act == 3) && ((gframes >> 3) & 1);   // (D55) mood, blink, talking: part of the cached model's key
    SimL *se = 0; int cached = 0, ox = x, oz = z, oq = sq_q, ogy = gy_off; u32 *sb_ = 0; int sn_ = 0, sh_ = 0, sk_ = 0, sc_ = 0; u32 sm_ = 0, snm_ = 0, scl_ = 0;
    if (gxb && dpass == 2) {   // (D39) the cached model: the key is everything its shape and colours come from
        if (mv) sw_l = sinLerp((s16)(((gframes * 1100 + i * 700) & 32767) & ~4095)) * 70 / 4096;   // (the stride in eight steps)
        u32 key = 2166136261u; for (int q = 0; q < PMN; q++) key = (key ^ (u32)P[q]) * 16777619u;
        key = (key ^ (u32)(fx_e | fx_b << 4 | fx_t << 5 | 1 << 6)) * 16777619u; key = (key ^ (u32)(stg | lod << 3 | (u32)m->hair << 4 | (u32)m->beard << 9 | (u32)lit_on << 13 | (u32)csx[i].hat << 14)) * 16777619u; key = (key ^ ((u32)sk | (u32)hc << 16)) * 16777619u; key = (key ^ ((u32)sh | (u32)pants << 16)) * 16777619u; key = (key ^ (u32)(sw_l + 1000)) * 16777619u;
        for (int q = 0; q < SML_N; q++) if (sml[q].key == key && sml[q].n) { se = &sml[q]; cached = 1; break; }
        if (!se) { for (int t = 0; t < SML_N; t++) { se = &sml[sml_next]; sml_next = (sml_next + 1) % SML_N; if (se->used + 2 < gf_serial) break; }   // (never one the frame being sent still uses)
            se->key = key; se->n = 0; sml_miss++;
            sb_ = gxb; sn_ = gxn; sh_ = gxh; sk_ = gxk; sc_ = gx_cap; sm_ = gx_mat; snm_ = gx_nrm; scl_ = gx_col; gx_begin(se->w, SML_W); x = 0; z = 0; sq_q = 0; gy_off = 0; } }
    if (!cached) {
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
    hair_pieces(m->hair, stg, x, z, sq_q, sb_sc, hw, hw, hb + ht, ht, hc);   // (D62) the PS1's hair pieces
    if (csx[i].hat) hat_draw(csx[i].hat, 0, x, z, hw, hw, hb + ht, ht);   // (D67) the hat
    if (!lod) { int ex = 40 + (P[2] - 50) / 2, ey = hb + ht * 55 / 100, es = 28 + (P[16] - 50) / 4, f = hw, xe = fx_e, shut = xe == EX_SLEEP || fx_b;   // (D55) the face: mood, blink, talking
      const u16 WH = C15(31, 31, 31), DK = C15(8, 1, 2), LPc = C15(21, 7, 8), LPh = C15(26, 12, 12); int eh = xe == EX_SCARED ? 12 : 0, bt = (P[6] - 50) / 4, rise = 0;
      if (xe == EX_SAD || xe == EX_TIRED) bt += 14; else if (xe == EX_ANGRY) bt -= 16; else if (xe == EX_SCARED) { bt += 12; rise = 14; } else if (xe == EX_HAPPY || xe == EX_LOVE) rise = 8;
      for (int sd = -1; sd <= 1; sd += 2) { int c0 = sd * ex, xi0 = sd < 0 ? c0 : c0 - es - 6, xi1 = sd < 0 ? c0 + es + 6 : c0, xo0 = sd < 0 ? c0 - es - 6 : c0, xo1 = sd < 0 ? c0 : c0 + es + 6;   // (inner and outer halves of the brow)
        if (shut) { sb(x, z, c0 - es - 4, ey - 6, f, c0 + es + 4, ey + 8, f + 6, shade(sk, 7)); sb(x, z, c0 - es - 2, ey - 6, f + 6, c0 + es + 2, ey + 2, f + 8, C15(3, 2, 2)); }
        else { sb(x, z, c0 - es - 4, ey - 24 - eh, f, c0 + es + 4, ey + 26 + eh, f + 6, WH);
            u16 ic = xe == EX_LOVE ? C15(30, 5, 10) : m->hcol == 5 ? C15(8, 14, 28) : C15(7, 5, 3);
            sb(x, z, c0 - 14, ey - 22, f + 4, c0 + 14, ey + 24, f + 10, ic); sb(x, z, c0 - (xe == EX_SCARED ? 4 : 6), ey - (xe == EX_SCARED ? 6 : 12), f + 8, c0 + (xe == EX_SCARED ? 4 : 6), ey + (xe == EX_SCARED ? 6 : 12), f + 12, C15(1, 1, 2));
            sb(x, z, c0 - 14, ey + 6, f + 10, c0 - 6, ey + 14, f + 13, WH);   // the glint
            if (xe == EX_TIRED || xe == EX_BORED) sb(x, z, c0 - es - 5, ey + 2, f, c0 + es + 5, ey + 28, f + 15, shade(sk, 8));   // the lid, half down
            else if (xe == EX_ANGRY) { sb(x, z, xi0 - (sd < 0 ? 4 : 0), ey + 2, f, xi1 + (sd < 0 ? 0 : 4), ey + 30, f + 15, shade(sk, 8)); sb(x, z, xo0, ey + 14, f, xo1, ey + 28, f + 15, shade(sk, 8)); }
            if (xe == EX_TIRED) sb(x, z, c0 - es, ey - 38, f, c0 + es, ey - 28, f + 3, shade(sk, 6));   // a bag under the eye
            if (xe == EX_SAD) sb(x, z, c0 + sd * (es - 6), ey - 66, f, c0 + sd * (es - 6) + 8, ey - 30, f + 4, C15(14, 22, 31)); }   // a tear
        { int by = ey + 36 + (P[7] - 50) / 3 + rise; sb(x, z, xo0, by, f, xo1, by + 14, f + 7, shade(hc, 7)); sb(x, z, xi0, by + bt, f, xi1, by + bt + 14, f + 7, shade(hc, 7)); }
        if (xe == EX_LOVE) sb(x, z, c0 - es, ey - 62, f, c0 + es, ey - 40, f + 3, C15(31, 14, 17)); }   // blush
      sb(x, z, -16 - (P[4] - 50) / 4, ey - 70, f, 16 + (P[4] - 50) / 4, ey - 6, f + 28 + (P[5] - 50) / 4, shade(sk, 8));
      int mw = 36 + (P[0] - 50) / 2, my = ey - 100 + (50 - P[1]) / 3, lp = 16 + (P[17] - 50) / 6;   // the mouth by mood (talking opens it)
      if (fx_t && xe != EX_SLEEP && xe != EX_SCARED && xe != EX_TIRED) { sb(x, z, -mw * 2 / 3, my - 14, f, mw * 2 / 3, my + lp, f + 5, DK); sb(x, z, -mw * 2 / 3, my + lp - 2, f, mw * 2 / 3, my + lp + 4, f + 7, LPc); sb(x, z, -mw * 2 / 3, my - 18, f, mw * 2 / 3, my - 12, f + 7, LPc); }
      else switch (xe) {
        case EX_HAPPY: case EX_LOVE: sb(x, z, -mw, my + 8, f, mw, my + 24, f + 4, DK); sb(x, z, -mw + 10, my + 10, f + 4, mw - 10, my + 24, f + 7, WH); sb(x, z, -mw + 10, my, f, mw - 10, my + 10, f + 6, LPc); sb(x, z, -mw - 6, my + 8, f, -mw + 12, my + 26, f + 6, LPc); sb(x, z, mw - 12, my + 8, f, mw + 6, my + 26, f + 6, LPc); break;
        case EX_SAD: sb(x, z, -mw + 8, my + 4, f, mw - 8, my + 4 + lp, f + 6, LPc); sb(x, z, -mw, my - 10, f, -mw + 14, my + 12, f + 6, LPc); sb(x, z, mw - 14, my - 10, f, mw, my + 12, f + 6, LPc); break;
        case EX_ANGRY: sb(x, z, -mw, my + 2, f, mw, my + 14, f + 6, C15(14, 3, 5)); sb(x, z, -mw + 6, my + 8, f + 6, mw - 6, my + 12, f + 8, WH); sb(x, z, -mw, my - 6, f, -mw + 10, my + 4, f + 6, C15(14, 3, 5)); sb(x, z, mw - 10, my - 6, f, mw, my + 4, f + 6, C15(14, 3, 5)); break;
        case EX_TIRED: case EX_SCARED: { int ww = xe == EX_SCARED ? mw * 2 / 3 : mw / 2, hh = xe == EX_SCARED ? 22 : 26; sb(x, z, -ww, my - hh, f, ww, my + 20, f + 5, DK); sb(x, z, -ww - 6, my - hh - 4, f, ww + 6, my - hh + 4, f + 7, LPc); sb(x, z, -ww - 6, my + 16, f, ww + 6, my + 24, f + 7, LPc); sb(x, z, -ww - 6, my - hh, f, -ww, my + 20, f + 7, LPc); sb(x, z, ww, my - hh, f, ww + 6, my + 20, f + 7, LPc); break; }
        case EX_BORED: sb(x, z, -mw / 3 + 8, my, f, mw * 2 / 3 + 8, my + lp - 4, f + 6, LPc); break;
        case EX_SLEEP: sb(x, z, -mw / 2, my + 2, f, mw / 2, my + 14, f + 5, C15(17, 6, 7)); break;
        default: sb(x, z, -mw, my, f, mw, my + lp, f + 6, LPc); sb(x, z, -mw + 6, my + 15, f, mw - 6, my + 22, f + 5, LPh); }
      if (m->beard == 1) sb(x, z, -hw + 8, hb, f - 4, hw - 8, ey - 60, f + 8, lit(sk, 10));   // stubble
      else if (m->beard) {   // (D61) real beards in pieces round the mouth: moustache with drooping ends, goatee chin, full jaw with cheeks, long beard in steps
        u16 bc = shade(hc, 8); int bd = m->beard, my2 = ey - 100 + (50 - P[1]) / 3;
        sb(x, z, -mw - 18, my2 + 20, f, mw + 18, my2 + 46, f + 9, bc);   // the moustache
        sb(x, z, -mw - 26, my2 - 4, f, -mw - 12, my2 + 24, f + 8, bc); sb(x, z, mw + 12, my2 - 4, f, mw + 26, my2 + 24, f + 8, bc);   // its ends droop
        if (bd == 3) { sb(x, z, -mw / 2, hb, f - 4, mw / 2, my2 - 6, f + 10, bc); sb(x, z, -mw / 4, hb - 30, f - 2, mw / 4, hb, f + 8, bc); }   // goatee
        if (bd >= 4) {
            sb(x, z, -hw + 6, hb, f - 4, hw - 6, my2 - 12, f + 8, bc);   // the jaw under the mouth
            sb(x, z, -hw + 4, my2 - 12, f - 4, -mw - 26, ey + 10, f + 8, bc); sb(x, z, mw + 26, my2 - 12, f - 4, hw - 4, ey + 10, f + 8, bc);   // the cheeks, up to the ears
            sb(x, z, -mw - 26, my2 - 12, f, -mw + 4, my2 + 4, f + 9, bc); sb(x, z, mw - 4, my2 - 12, f, mw + 26, my2 + 4, f + 9, bc);   // round the corners of the mouth
            if (bd == 5) { sb(x, z, -hw + 30, hb - 70, f - 6, hw - 30, hb, f + 10, bc); sb(x, z, -hw + 80, hb - 140, f - 8, hw - 80, hb - 70, f + 10, bc); sb(x, z, -hw + 130, hb - 200, f - 10, hw - 130, hb - 140, f + 10, bc); } } } }
    }
    if (se) { se->used = gf_serial; if (!cached) { se->n = gx_full ? 0 : gxn; if (gx_full) sml_full++; if (se->n > sml_maxn) sml_maxn = se->n; DC_FlushRange(se->w, (u32)se->n * 4); gxb = sb_; gxn = sn_; gxh = sh_; gxk = sk_; gx_cap = sc_; gx_mat = sm_; gx_nrm = snm_; gx_col = scl_; gx_full = 0; gy_off = ogy; x = ox; z = oz; sq_q = oq; }
        if (ride) em_R = sk_Rm;   // (D44) side on to the board (skate_frame turns it)
        if (se->n) sim_emit(se, ox, -ogy + ground_lift(fdiv(m->x + 8, 16), fdiv(m->y + 8, 16), slv[i]) - (ride ? board_lift * 12 / 5 + 80 - sk_cr : 0), oz, oq); em_R = 0; polys += 120;
        if (ride) skate_draw_board(-ogy + ground_lift(fdiv(m->x + 8, 16), fdiv(m->y + 8, 16), 0)); }
    if (i == ctl) { int py = (hb + ht + 190) * sb_sc / 100; sb_sc = 100; sq_q = 0; bx(x - 40, py, z - 40, x + 40, py + 120, z + 40, C15(6, 30, 6)); bx(x - 30, py - 90, z - 30, x + 30, py, z + 30, C15(5, 22, 5)); }
}
// ---- Create-a-Sim: the head on the top screen, the choices on the bottom ----
static int cas_body, cas_ex_hold;   // 1 when the Body sliders are showing: the camera pulls back to show the whole figure
static void draw_head(int yaw) {
    (void)yaw; const Sim *m = &sims[0]; u16 sk = skin_of(m), hc = hair_of(m), sh = shirt_of(m); int p[PMN]; eff_pm(m->pm, p);
    int hw = (m->fc == 1 ? 780 : 700) + (p[8] - 50) * 4, ht = (m->fc == 2 ? 1700 : 1500) + (p[9] - 50) * 6, hz = 700;
    int nl = (p[18] - 50) * 6, nw = 300 + (p[19] - 50) * 4, sw = 1150 + (p[14] - 50) * 8, th = 2200 + (p[15] - 50) * 20 + (p[26] - 50) * 12, bz = 450 + (p[24] - 50) * 6;
    int at = 350 + (p[21] - 50) * 4, al = 2200 + (p[20] - 50) * 20 + (p[26] - 50) * 8, hp = sw * 8 / 10 + (p[25] - 50) * 6, lh = 2800 + (p[22] - 50) * 25 + (p[26] - 50) * 20, lt = 300 + (p[23] - 50) * 4;
    int yt = -1000 - nl, yb = yt - th; u16 pants = PANTC[csx[0].pants % 6];
    bx(-sw, yt, -520, sw, -300 - nl, 520, sh);                                    // shoulders
    bx(-sw * 8 / 10, yb, -450, sw * 8 / 10, yt, bz, sh);                          // torso (belly pushes the front out)
    bx(-sw - at, yt - al, -at, -sw, -300 - nl, at, sh); bx(sw, yt - al, -at, sw + at, -300 - nl, at, sh);   // arms
    bx(-sw - at, yt - al - 300, -at, -sw, yt - al, at, sk); bx(sw, yt - al - 300, -at, sw + at, yt - al, at, sk);   // hands
    bx(-hp, yb - 400, -430, hp, yb, bz, pants);                                   // hips
    bx(-hp, yb - 400 - lh, -lt, -40, yb - 400, lt, pants); bx(40, yb - 400 - lh, -lt, hp, yb - 400, lt, pants);   // legs
    bx(-hp, yb - 560 - lh, -lt, -40, yb - 400 - lh, lt + 250, C15(4, 4, 6)); bx(40, yb - 560 - lh, -lt, hp, yb - 400 - lh, lt + 250, C15(4, 4, 6));   // shoes
    { int fy = yb - 560 - lh; bx(-1700, fy - 160, -1700, 1700, fy, 1700, C15(27, 29, 31)); bx(-1900, fy - 340, -1900, 1900, fy - 160, 1900, C15(19, 23, 29)); }   // (D67) the round white stand of the 3DS's creator
    bx(-nw, -330 - nl, -nw, nw, 20, nw, shade(sk, 7));                            // neck
    int hwj = hw + (p[10] - 50) * 3, ch = (p[11] - 50) * 6;
    bx(-hw, 0, -hz, hw, ht, hz, sk);                                              // head
    if (hwj > hw) bx(-hwj, 0, -hz * 9 / 10, hwj, ht * 3 / 10, hz * 9 / 10, sk);   // wide jaw
    if (ch > 0) bx(-hwj * 8 / 10, -ch, -hz * 8 / 10, hwj * 8 / 10, 40, hz * 8 / 10, sk);   // long chin
    { int es = 100 + (p[12] - 50) * 2, eh = ht * 4 / 10 + (p[13] - 50) * 4, ez = ht * 15 / 100 + (p[12] - 50) * 3;
      bx(-hw - es, eh - ez, -90, -hw, eh + ez, 160, shade(sk, 7)); bx(hw, eh - ez, -90, hw + es, eh + ez, 160, shade(sk, 7)); }   // ears
    int xe = ((cas_step == CS_LOOKS && cas_sub > 0) || cas_ex_hold) ? EX_NEU : (int)((vbl_n / 200) % EX_N); if (xe == EX_SLEEP) xe = EX_HAPPY;   // (D55) the head shows the moods in turn while you pick, and stays neutral while you edit a feature (D67: never asleep)
#ifdef DSTEST
    if (test_ex >= 1 && test_ex <= EX_N) xe = test_ex - 1;
#endif
    int blink = xe != EX_SLEEP && (vbl_n % 170) < 7;
    int eu = ht * 6 / 10 + (p[3] - 50) * 5, er = (m->fc == 3 ? 125 : 88) + (p[16] - 50), ew = 185 + (p[16] - 50) * 2, zb = m->beard >= 4 ? 80 : 0;   // eyes
    if (xe == EX_SCARED) er += 40;
    int btl = (p[6] - 50) * 2, brise = 0; if (xe == EX_SAD || xe == EX_TIRED) btl -= 130; else if (xe == EX_ANGRY) btl += 150; else if (xe == EX_SCARED) { btl -= 110; brise = 110; } else if (xe == EX_HAPPY || xe == EX_LOVE) brise = 40;
    for (int sd = -1; sd <= 1; sd += 2) { int c = sd * (280 + (p[2] - 50) * 3), xo = c + sd * 170, xi = c - sd * 170;
        fq(c - ew, eu - er, c + ew, eu + er, hz + 2, C15(31, 31, 31)); fq(c - 70, eu - er, c + 70, eu + er, hz + 4, xe == EX_LOVE ? C15(30, 5, 10) : C15(9, 6, 3)); fq(c - 36, eu - er + 18, c + 36, eu + er - 18, hz + 6, C15(2, 1, 2));
        fq(c - 40, eu + er - 45, c - 8, eu + er - 12, hz + 8, C15(31, 31, 31));
        if (m->fc == 4 && xe != EX_TIRED) fq(c - ew, eu, c + ew, eu + er, hz + 8, shade(sk, 7));                    // sleepy lids
        if (xe == EX_TIRED || xe == EX_BORED) { fq(c - ew - 6, eu, c + ew + 6, eu + er + 6, hz + 9, shade(sk, 7)); fq(c - ew, eu - 6, c + ew, eu + 6, hz + 10, shade(sk, 5)); fq(c - ew, eu - er - 40, c + ew, eu - er - 10, hz + 3, shade(sk, 6)); }   // the lid half down, a bag under the eye
        else if (xe == EX_ANGRY) { qd(xo + sd * 6, -(eu + er + 6), hz + 9, xi, -(eu - er / 2), hz + 9, xi, -(eu + er + 6), hz + 9, xo + sd * 6, -(eu + er + 6), hz + 9, shade(sk, 7)); }   // the lid slanted, low at the inside
        if (blink || xe == EX_SLEEP) { fq(c - ew - 8, eu - er - 8, c + ew + 8, eu + er + 8, hz + 11, sk); fq(c - ew, eu - 8, c + ew, eu + 8, hz + 12, C15(3, 2, 2)); }
        if (xe == EX_SAD) fq(c + sd * 110 - 12, eu - er - 190, c + sd * 110 + 12, eu - er - 60, hz + 5, C15(14, 22, 31));   // a tear
        if (xe == EX_LOVE) fq(c - 130, eu - er - 230, c + 130, eu - er - 130, hz + 3, C15(31, 14, 17));   // the blush
        { int ub = eu + er + 90 + (p[7] - 50) * 3 + brise, tl2 = btl; qd(xi, -(ub - tl2), hz + 3, xo, -(ub + tl2), hz + 3, xo, -(ub + 70 + tl2), hz + 3, xi, -(ub + 70 - tl2), hz + 3, shade(hc, 7)); }   // brow (tilt and height)
        if (m->fc == 5) for (int k = 0; k < 5; k++) fq(c - 130 + k * 55, eu - 250 - (k & 1) * 50, c - 100 + k * 55, eu - 220 - (k & 1) * 50, hz + 3, shade(sk, 5)); }
    { int nw2 = 90 + (p[4] - 50) * 2, np = 110 + (p[5] - 50) * 3; bx(-nw2, eu - 380, hz - 20, nw2, eu - 60, hz + np, shade(sk, 8)); }   // nose
    beard3d(m->beard, shade(hc, 9), sk, hw, hz);
    { int mw = 200 + (p[0] - 50) * 3, mv = ht * 2 / 10 + (p[1] - 50) * 5, lp = 70 + (p[17] - 50), z = hz + 3 + zb; const u16 LP = C15(21, 7, 8), LH = C15(26, 11, 11), DK = C15(8, 1, 2), WH = C15(31, 31, 31);   // the mouth by mood
      switch (xe) {
      case EX_HAPPY: case EX_LOVE: fq(-mw, mv - 30, mw, mv + lp, z, DK); fq(-mw + 40, mv + lp - 80, mw - 40, mv + lp, z + 1, WH); fq(-mw, mv + lp, mw, mv + lp + 45, z + 2, LP); fq(-mw - 50, mv + 40, -mw + 50, mv + lp + 80, z + 2, LP); fq(mw - 50, mv + 40, mw + 50, mv + lp + 80, z + 2, LP); fq(-mw + 40, mv - 60, mw - 40, mv - 30, z + 2, LH); break;
      case EX_SAD: fq(-mw + 30, mv + 20, mw - 30, mv + 20 + lp, z, LP); fq(-mw - 40, mv - 70, -mw + 60, mv + 40, z, LP); fq(mw - 60, mv - 70, mw + 40, mv + 40, z, LP); break;
      case EX_ANGRY: fq(-mw, mv, mw, mv + 50, z, C15(14, 3, 5)); fq(-mw + 40, mv + 25, mw - 40, mv + 50, z + 1, WH); fq(-mw - 20, mv - 50, -mw + 50, mv + 20, z, C15(14, 3, 5)); fq(mw - 50, mv - 50, mw + 20, mv + 20, z, C15(14, 3, 5)); break;
      case EX_TIRED: case EX_SCARED: { int ww = xe == EX_SCARED ? mw * 55 / 100 : mw * 40 / 100, hh = xe == EX_SCARED ? 110 : 130; fq(-ww, mv - hh, ww, mv + hh / 2, z, DK); fq(-ww - 25, mv + hh / 2, ww + 25, mv + hh / 2 + 30, z + 1, LP); fq(-ww - 25, mv - hh - 30, ww + 25, mv - hh, z + 1, LP); fq(-ww - 25, mv - hh, -ww, mv + hh / 2, z + 1, LP); fq(ww, mv - hh, ww + 25, mv + hh / 2, z + 1, LP); break; }
      case EX_BORED: fq(-mw / 3 + 40, mv, mw * 2 / 3 + 40, mv + lp - 20, z, LP); break;
      case EX_SLEEP: fq(-mw / 2, mv + 10, mw / 2, mv + 50, z, C15(17, 6, 7)); break;
      default: fq(-mw, mv, mw, mv + lp, z, LP); fq(-mw + 20, mv - lp * 7 / 10, mw - 20, mv, z, LH); fq(-mw, mv + 10, mw, mv + 25, z + 2, C15(5, 1, 2)); } }
    hair_pieces(m->hair, 4, 0, 0, 0, 100, hw, hz, ht, ht, hc);   // (D62) the PS1's hair pieces
    if (csx[0].hat) hat_draw(csx[0].hat, 1, 0, 0, hw, hz, ht, ht);   // (D67) the hat
}
// ---- (D67) CREATE-A-SIM, in the way of The Sims 3 for the 3DS (docs/CAS_SIMS3DS_DS_NOTES.md) ----
// Bottom screen: six steps down the left (Basics, Looks, Hair, Outfit, Traits, Done), tabs inside a step, Back and Next at the foot, a welcome pop-up,
// a keyboard for the name (first, then last), a summary to confirm. Top screen: the Sim, its camera moving to the part you are working on.
static void plumb_icon(int x, int y, int sz, u16 c);
static int cas_feat, cas_tgt, cas_rgb[3], cas_drag, cas_sp, cas_sg;
static const struct { const char *n; u8 i; } SLF[16] = { {"Mouth W",0},{"Mouth Y",1},{"Lips",17},{"Eye gap",2},{"Eye Y",3},{"Eye size",16},{"Nose W",4},{"Nose L",5},{"Brow tilt",6},{"Brow Y",7},{"Head W",8},{"Head H",9},{"Jaw W",10},{"Chin",11},{"Ear size",12},{"Ear Y",13} },
  SLB[11] = { {"Shldrs",14},{"Torso",15},{"Neck L",18},{"Neck W",19},{"Arm L",20},{"Arm W",21},{"Leg L",22},{"Leg W",23},{"Belly",24},{"Hips",25},{"Height",26} };
static int sl_n(void) { return cas_sg ? 11 : 16; }
static int sl_idx(int k) { int j = cas_sp * 6 + k; return j >= sl_n() ? -1 : (cas_sg ? SLB[j].i : SLF[j].i); }
static const char *sl_name(int k) { int j = cas_sp * 6 + k; return cas_sg ? SLB[j].n : SLF[j].n; }
static const char *const FEATN[8] = { "Mouth", "Eyes", "Nose", "Brows", "Head", "Jaw", "Ears", "Body" };
static const char *const FEATX[8] = { "Width", "Spacing", "Width", "Tilt", "Width", "Width", "Size", "Shoulders" }, *const FEATY[8] = { "Height", "Height", "Length", "Height", "Height", "Chin", "Height", "Torso" };
static const char *const SHIRTN[6] = { "White", "Red", "Blue", "Green", "Yellow", "Purple" };
static const char *const CSN[CS_N] = { "Basics", "Looks", "Hair", "Outfit", "Traits", "Done" };
static const char *const NMF[12] = { "Ashley", "Juno", "Wren", "Fern", "Mabel", "Sage", "Ivy", "Nell", "Zoe", "June", "Lola", "Opal" };
static const char *const NMM[12] = { "Remy", "Pip", "Tate", "Otis", "Kit", "Ronan", "Bo", "Dash", "Rex", "Milo", "Hugo", "Finn" };
static const char *const CLN[12] = { "Newbie", "Bacon", "Farlow", "Pickle", "Marsh", "Quill", "Dunmore", "Fenn", "Hollow", "Tinker", "Vance", "Wick" };
static char cas_first[12], cas_last[12];
static int cas_pop, cas_kb, cas_kbc, cas_shift, cas_mixo, cas_note, cas_notei, cas_cd = 13500, cas_cy = -2750;   // pop-up (1 welcome, 2 leave, 3 play); keyboard (1 first, 2 last name), chained; shift; the skin mixer; the note on the top screen (1 trait, 2 wish); the camera
static u32 cas_rn = 7;
#define PADX 112
#define PADY 44
#define PADW 92
#define PADH 86
#define CPX 41
#define CPW 213
static u32 cas_rand(void) { cas_rn = cas_rn * 1664525u + 1013904223u + (u32)vbl_n; return cas_rn >> 8; }
static void cas_name(void) { if (cas_last[0]) sprintf(sname[0], "%.9s %.9s", cas_first[0] ? cas_first : "Sim", cas_last); else sprintf(sname[0], "%.9s", cas_first[0] ? cas_first : "Sim"); }
static void cas_rname(void) { strcpy(cas_first, (csx[0].gen ? NMM : NMF)[cas_rand() % 12]); strcpy(cas_last, CLN[cas_rand() % 12]); cas_name(); }
static void cas_load_rgb(void) { const Sim *m = &sims[0]; u16 c = cas_tgt == 0 ? skin_of(m) : cas_tgt == 1 ? hair_of(m) : shirt_of(m); cas_rgb[0] = c & 31; cas_rgb[1] = (c >> 5) & 31; cas_rgb[2] = (c >> 10) & 31; }
static void cas_apply_rgb(void) { sims[0].cc[cas_tgt] = C15(cas_rgb[0], cas_rgb[1], cas_rgb[2]) | BIT(15); }
static void cas_fit(int f) { static const u8 BL[3] = { 26, 50, 82 }, TO[3] = { 44, 50, 58 }, HP[3] = { 42, 50, 64 }; Sim *m = &sims[0]; m->pm[24] = BL[f]; m->pm[15] = TO[f]; m->pm[25] = HP[f]; }
static int cas_fit_of(void) { int b = sims[0].pm[24]; return b < 38 ? 0 : b >= 66 ? 2 : 1; }
static void cas_gender(int g) {   // the other kind of name goes with it (same place in the list); the shoulders follow
    int old = csx[0].gen; csx[0].gen = (u8)g; sims[0].pm[14] = (u8)(g ? 56 : 44);
    if (old != g) for (int i = 0; i < 12; i++) if (!strcmp(cas_first, (old ? NMM : NMF)[i])) { strcpy(cas_first, (g ? NMM : NMF)[i]); break; }
    cas_name(); }
static int cas_tr_at(int t) { for (int k = 0; k < 3; k++) if (csx[0].tr[k] == t) return k + 1; return 0; }
static int cas_tr_n(void) { int n = 0; for (int k = 0; k < 3; k++) n += csx[0].tr[k] != 0; return n; }
static int cas_tr_ok(int t) { return cas_tr_at(t) || (cas_tr_n() < 3 && !cas_tr_at(((t - 1) ^ 1) + 1)); }   // free slot and not the opposite of a chosen one (as the 3DS greys them)
static void cas_tr_tap(int t) {
    int a = cas_tr_at(t); u8 *tr = csx[0].tr;
    if (a) { for (int k = a - 1; k < 2; k++) tr[k] = tr[k + 1]; tr[2] = 0; } else if (cas_tr_ok(t)) tr[cas_tr_n()] = (u8)t; }
static void cas_tr_random(void) { memset(csx[0].tr, 0, 3); for (int n = 0; n < 3;) { int t = 1 + (int)(cas_rand() % 12); if (cas_tr_ok(t) && !cas_tr_at(t)) csx[0].tr[n++] = (u8)t; } }
static void cas_random(void) {   // "Create a random Sim": everything the creator has
    Sim *m = &sims[0]; csx[0].gen = (u8)(cas_rand() & 1);
    m->skin = (u8)(cas_rand() % 6); m->hair = (u8)(cas_rand() % NHAIRD); m->hcol = (u8)(cas_rand() % 6); m->shirt = (u8)(cas_rand() % 6); m->fc = (u8)(cas_rand() % 6); m->cc[0] = m->cc[1] = m->cc[2] = 0;
    m->beard = (u8)(csx[0].gen && cas_rand() % 3 == 0 ? 1 + cas_rand() % 5 : 0); csx[0].hat = (u8)(cas_rand() % 4 == 0 ? 1 + cas_rand() % 5 : 0); csx[0].pants = (u8)(cas_rand() % 6);
    for (int q = 0; q < PMN; q++) m->pm[q] = (u8)(50 + (int)(cas_rand() % 25) - 12);
    cas_fit((int)(cas_rand() % 3)); m->pm[14] = (u8)(csx[0].gen ? 56 : 44); cas_tr_random(); csx[0].wish = (u8)(1 + cas_rand() % 5); csx[0].wdone = 0; cas_rname(); cas_note = 0; }
static void cas_enter(void) {   // a new Sim: Basics, with the welcome pop-up
    memset(csx, 0, sizeof csx); cas_rn = (u32)minutes * 2654435761u + 12345u; csx[0].gen = (u8)(cas_rand() & 1); sims[0].pm[14] = (u8)(csx[0].gen ? 56 : 44); cas_rname();
    cas_step = CS_BASICS; cas_sub = 0; cas_page = 0; cas_sp = 0; cas_sg = 0; cas_feat = 0; cas_mult = 1; cas_pop = 1; cas_kb = cas_kbc = 0; cas_mixo = 0; cas_note = 0; cas_tgt = 0; cas_done = 0; cas_cd = 13500; cas_cy = -2750; }
static void cas_setsub(int s) {
    cas_sub = s; cas_page = 0; cas_sp = 0; cas_mixo = 0;
    if (cas_step == CS_HAIR && s == 1) { cas_tgt = 1; cas_load_rgb(); } if (cas_step == CS_OUTFIT && s == 2) { cas_tgt = 2; cas_load_rgb(); }
    if (cas_step == CS_TRAITS) cas_note = 0; }
static void cas_goto(int s) { if (s < 0) s = 0; if (s >= CS_N) s = CS_N - 1; cas_step = s; cas_setsub(0); cas_kb = 0; if (s == CS_DONE) cas_note = 0; }
static void cas_cam(int *d, int *y) {   // where the top camera wants to be: the head for faces and hair, the chest or the legs for clothes, the whole Sim otherwise
    int full = 0, dd = 13500, yy = -2750;
    if (cas_step == CS_LOOKS) { full = (cas_sub == 1 && cas_feat == 7) || (cas_sub == 2 && cas_sg); if (!full) { dd = 4700; yy = 650; } }
    else if (cas_step == CS_HAIR) { dd = 5400; yy = cas_sub == 3 ? 900 : 700; }
    else if (cas_step == CS_OUTFIT) { if (cas_sub == 1) { dd = 11500; yy = -4400; } else { dd = 10000; yy = -1500; } }
    else if (cas_step == CS_BASICS && cas_mixo) { dd = 4700; yy = 650; }
    *d = dd; *y = yy; }
static void cas_icon(int x, int y, int i, int on) {   // a 12x12 pictogram for each step
    u16 sk = C15(30, 22, 16), hr = C15(14, 8, 4), wh = on ? C15(31, 31, 31) : C15(24, 28, 31);
    switch (i) {
        case 0: rect(x, y + 2, 4, 8, C15(31, 24, 18)); rect(x + 4, y + 2, 4, 8, C15(24, 16, 10)); rect(x + 8, y + 2, 4, 8, C15(12, 7, 4)); break;                              // skin tones (Basics)
        case 1: rect(x + 1, y + 1, 10, 10, sk); rect(x + 3, y + 4, 2, 2, C15(2, 2, 6)); rect(x + 7, y + 4, 2, 2, C15(2, 2, 6)); rect(x + 4, y + 8, 4, 1, C15(20, 6, 6)); break;   // a face (Looks)
        case 2: rect(x + 1, y, 10, 5, hr); rect(x, y + 3, 3, 7, hr); rect(x + 9, y + 3, 3, 7, hr); rect(x + 3, y + 5, 6, 6, sk); break;                                          // hair
        case 3: rect(x, y + 1, 4, 4, C15(10, 20, 31)); rect(x + 8, y + 1, 4, 4, C15(10, 20, 31)); rect(x + 2, y + 1, 8, 10, C15(10, 20, 31)); break;                            // a shirt (Outfit)
        case 4: plumb_icon(x, y, 12, C15(14, 31, 14)); break;                                                                                                                        // the plumbob (Traits)
        default: rect(x + 1, y + 6, 3, 2, wh); rect(x + 3, y + 8, 3, 2, wh); rect(x + 5, y + 6, 3, 2, wh); rect(x + 7, y + 4, 3, 2, wh); rect(x + 9, y + 2, 3, 2, wh); break;      // a tick (Done)
    }
}
static void cbtn(int x, int y, int w, int h, const char *t, int on, int fs) {   // a button: green when it is the one chosen, grey when it can't be used
    u16 bd = on == 1 ? C15(31, 31, 31) : C15(2, 5, 14), fl = on == 1 ? C15(8, 24, 8) : on < 0 ? C15(9, 11, 16) : C15(6, 12, 26), hi = on == 1 ? C15(18, 31, 18) : on < 0 ? C15(12, 14, 19) : C15(10, 18, 31);
    rect(x, y, w, h, bd); rect(x + 1, y + 1, w - 2, h - 2, fl); rect(x + 1, y + 1, w - 2, 1, hi); text_box(x, y, w, h, fs, on < 0 ? C15(17, 19, 24) : TXT, AL_C, t); }
static void cas_field(int x, int y, int w, int h, const char *t, int on, int cur) {   // a text box (light, dark letters), a cursor bar when it is being typed in
    rect(x, y, w, h, on ? C15(31, 31, 20) : C15(2, 5, 14)); rect(x + 1, y + 1, w - 2, h - 2, C15(29, 31, 31)); text_box(x + 1, y + 1, w - 2, h - 2, FS_HEAD, C15(2, 4, 12), AL_C, t);
    if (cur) { int cxp = x + (w - tw(FS_HEAD, t)) / 2 + tw(FS_HEAD, t) + 2; if (cxp > x + w - 4) cxp = x + w - 4; rect(cxp, y + 3, 1, h - 6, C15(2, 4, 12)); } }
static void cas_tile(int x, int y, int w, int h, int on) {   // a pick: light blue, a green frame and a tick when it is the one chosen
    rect(x + 1, y + 2, w, h, C15(4, 9, 20)); rect(x, y, w, h, on ? C15(31, 31, 31) : C15(2, 5, 14)); rect(x + 1, y + 1, w - 2, h - 2, on ? C15(10, 28, 8) : C15(10, 14, 24)); rect(x + 3, y + 3, w - 6, h - 6, on ? C15(24, 31, 22) : C15(20, 26, 31));
    if (on) { rect(x + w - 15, y + 2, 12, 12, C15(31, 31, 31)); rect(x + w - 14, y + 3, 10, 10, C15(8, 22, 8)); rect(x + w - 12, y + 8, 2, 2, C15(31, 31, 31)); rect(x + w - 10, y + 9, 2, 2, C15(31, 31, 31)); rect(x + w - 8, y + 6, 2, 4, C15(31, 31, 31)); } }
static void cas_hdr(const char *t) { panel(CPX, 2, CPW, 160, 0); rect(CPX + 2, 4, CPW - 4, 16, PANH); text_box(CPX + 26, 4, CPW - 52, 16, FS_HEAD, TXT, AL_C, t); }
static void cas_pager(int pages) { if (pages < 2) return; cbtn(CPX + 3, 5, 20, 14, "<", cas_page > 0 ? 0 : -1, FS_BODY); cbtn(CPX + CPW - 23, 5, 20, 14, ">", cas_page + 1 < pages ? 0 : -1, FS_BODY); }
static void cas_subtabs(int n, const char *const *nm) { int w = (CPW - 8 - (n - 1) * 2) / n; for (int i = 0; i < n; i++) cbtn(CPX + 4 + i * (w + 2), 23, w, 15, nm[i], cas_sub == i, FS_BODY); }
static int cas_subhit(int n, int tx, int ty) { int w = (CPW - 8 - (n - 1) * 2) / n; for (int i = 0; i < n; i++) if (hit(CPX + 4 + i * (w + 2), 23, w, 15, tx, ty)) return i; return -1; }
static void cas_swatch(int x, int y, int w, int h, u16 c, int on) { rect(x, y, w, h, on ? C15(31, 31, 31) : C15(2, 5, 14)); rect(x + 1, y + 1, w - 2, h - 2, on ? C15(31, 31, 31) : C15(2, 5, 14)); rect(x + 2, y + 2, w - 4, h - 4, c); }
static void cas_pants_pic(int x, int y, int w, int h, u16 c) { rect(x, y, w, 7, shade(c, 9)); rect(x, y + 7, w / 2 - 1, h - 7, c); rect(x + w / 2 + 1, y + 7, w - w / 2 - 1, h - 7, c); }
// (the colour page: six colours of the thing, then three bars to mix any other; used for hair, the top and, from Basics, the skin)
static void cas_colpage(int ok) {
    const u16 *pal = cas_tgt == 0 ? SKINC : cas_tgt == 1 ? HAIRC : SHIRTC; const Sim *m = &sims[0]; int cur = cas_tgt == 0 ? m->skin : cas_tgt == 1 ? m->hcol : m->shirt, own = m->cc[cas_tgt] != 0;
    for (int i = 0; i < 6; i++) cas_swatch(45 + i * 34, 42, 32, 26, pal[i], !own && cur == i);
    static const u16 col[3] = { C15(31, 8, 8), C15(8, 28, 8), C15(10, 14, 31) }; static const char *const cn[3] = { "R", "G", "B" };
    for (int k = 0; k < 3; k++) { int y = 74 + k * 18; text_box(45, y, 14, 14, FS_HEAD, TXT, AL_C, cn[k]); rect(61, y, 126, 14, PANB); rect(62, y + 1, 124, 12, C15(2, 5, 12)); rect(62, y + 1, cas_rgb[k] * 124 / 31, 12, col[k]); rect(60 + cas_rgb[k] * 124 / 31, y - 2, 5, 18, C15(31, 31, 31)); }
    rect(197, 74, 54, 54, PANB); rect(199, 76, 50, 50, C15(cas_rgb[0], cas_rgb[1], cas_rgb[2]) | BIT(15));
    text_box(45, 126, 142, 10, FS_SMALL, C15(22, 28, 31), AL_L, "Drag a bar to mix a colour");
    cbtn(197, 134, 54, 20, "Reset", 0, FS_BODY); if (ok) cbtn(45, 138, 142, 20, "OK", 1, FS_HEAD); }
static int cas_coltouch(int ok, int tx, int ty) {   // 1 when the page is closed
    const Sim *m = &sims[0]; (void)m;
    for (int i = 0; i < 6; i++) if (hit(45 + i * 34, 42, 32, 26, tx, ty)) { if (cas_tgt == 0) sims[0].skin = (u8)i; else if (cas_tgt == 1) sims[0].hcol = (u8)i; else sims[0].shirt = (u8)i; sims[0].cc[cas_tgt] = 0; cas_load_rgb(); }
    for (int k = 0; k < 3; k++) if (hit(52, 72 + k * 18, 140, 18, tx, ty)) { cas_drag = 10 + k; }
    if (hit(197, 134, 54, 20, tx, ty)) { sims[0].cc[cas_tgt] = 0; cas_load_rgb(); }
    if (ok && hit(45, 138, 142, 20, tx, ty)) return 1;
    return 0; }
static void cas_drag_at(int tx, int ty) {   // a stylus drag on the pad, a slider or a colour bar
    Sim *m = &sims[0];
    if (cas_drag == 1) { int a = cas_feat * 2, vx = (tx - PADX) * 100 / PADW, vy = 100 - (ty - PADY) * 100 / PADH; m->pm[a] = (u8)(vx < 0 ? 0 : vx > 100 ? 100 : vx); m->pm[a + 1] = (u8)(vy < 0 ? 0 : vy > 100 ? 100 : vy); }
    if (cas_drag >= 20) { int q = sl_idx(cas_drag - 20), v = (tx - 106) * 100 / 96; if (q >= 0) m->pm[q] = (u8)(v < 0 ? 0 : v > 100 ? 100 : v); }
    if (cas_drag >= 10 && cas_drag < 20) { int k = cas_drag - 10, v = (tx - 62) * 31 / 124; cas_rgb[k] = v < 0 ? 0 : v > 31 ? 31 : v; cas_apply_rgb(); }
}
// ---- the six steps ----
static void cas_tiles3(int cur, const char *const *names, int n, int kind) {   // six picks, 3 x 2, each a portrait (kind 0 face / 1 beard / 2 hat / 3 shirt) or trousers (4)
    for (int k = 0; k < 6 && k < n; k++) { int x = 45 + (k % 3) * 69, y = 42 + (k / 3) * 58; cas_tile(x, y, 66, 56, cur == k);
        Sim v = sims[0]; face_hat = csx[0].hat; face_bg = C15(13, 17, 26);
        if (kind == 0) v.fc = (u8)k; else if (kind == 1) v.beard = (u8)k; else if (kind == 2) face_hat = k; else if (kind == 3) { v.shirt = (u8)k; v.cc[2] = 0; }
        if (kind == 4) cas_pants_pic(x + 18, y + 6, 30, 32, PANTC[k]); else face(x + 15, y + 3, 36, &v);
        face_hat = -1; face_bg = -1; text_box(x + 3, y + 41, 60, 13, FS_BODY, PANB, AL_C, names[k]); } }
static int cas_tiles3_hit(int tx, int ty) { for (int k = 0; k < 6; k++) if (hit(45 + (k % 3) * 69, 42 + (k / 3) * 58, 66, 56, tx, ty)) return k; return -1; }
static void cas_draw_step(void) {
    Sim *m = &sims[0];
    if (cas_step == CS_BASICS) {
        if (cas_mixo) { cas_hdr("Skin colour"); cas_colpage(1); return; }
        cas_hdr("Basics");
        cas_field(45, 24, 92, 18, cas_first, cas_kb == 1, cas_kb == 1); cas_field(139, 24, 92, 18, cas_last, cas_kb == 2, cas_kb == 2);
        cbtn(233, 24, 18, 18, "", 0, FS_BODY); rect(237, 28, 10, 10, C15(31, 31, 31)); rect(239, 30, 2, 2, C15(2, 5, 14)); rect(243, 30, 2, 2, C15(2, 5, 14)); rect(241, 33, 2, 2, C15(2, 5, 14)); rect(239, 35, 2, 2, C15(2, 5, 14)); rect(243, 35, 2, 2, C15(2, 5, 14));
        text_box(45, 44, 100, 11, FS_SMALL, C15(22, 28, 31), AL_L, "Gender"); text_box(153, 44, 98, 11, FS_SMALL, C15(22, 28, 31), AL_L, "Body");
        cbtn(45, 56, 48, 24, "Girl", csx[0].gen == 0, FS_HEAD); cbtn(95, 56, 48, 24, "Boy", csx[0].gen == 1, FS_HEAD);
        { static const char *const fn[3] = { "Slim", "Mid", "Big" }; int f = cas_fit_of(); for (int i = 0; i < 3; i++) cbtn(153 + i * 34, 56, 31, 24, fn[i], f == i, FS_BODY); }
        text_box(45, 84, 100, 11, FS_SMALL, C15(22, 28, 31), AL_L, "Skin");
        for (int i = 0; i < 6; i++) cas_swatch(45 + i * 27, 96, 25, 24, SKINC[i], !m->cc[0] && m->skin == i);
        cbtn(209, 96, 42, 24, "Mix", 0, FS_HEAD);
        cbtn(45, 128, 206, 28, "Random Sim", 0, FS_HEAD);
    } else if (cas_step == CS_LOOKS) {
        static const char *const sn[3] = { "Face", "Features", "Sliders" };
        if (cas_sub == 2) { char pg[16]; sprintf(pg, "Sliders %d/%d", cas_sp + 1, (sl_n() + 5) / 6); cas_hdr(pg); cas_pager((sl_n() + 5) / 6); } else cas_hdr("Looks");
        cas_subtabs(3, sn);
        if (cas_sub == 0) cas_tiles3(m->fc, FACEN, 6, 0);
        else if (cas_sub == 1) {
            for (int i = 0; i < 8; i++) cbtn(45, 42 + i * 14, 58, 13, FEATN[i], cas_feat == i, FS_BODY);
            rect(PADX - 2, PADY - 2, PADW + 4, PADH + 4, PANB); rect(PADX, PADY, PADW, PADH, C15(30, 31, 31)); rect(PADX + PADW / 2, PADY, 1, PADH, C15(22, 26, 30)); rect(PADX, PADY + PADH / 2, PADW, 1, C15(22, 26, 30));
            int a = cas_feat * 2, dx = PADX + m->pm[a] * PADW / 100, dy = PADY + (100 - m->pm[a + 1]) * PADH / 100;
            rect(dx - 5, dy - 5, 11, 11, C15(3, 8, 24)); rect(dx - 3, dy - 3, 7, 7, C15(10, 18, 31));
            cbtn(209, 44, 42, 20, "Reset", 0, FS_BODY);
            { char t[24]; sprintf(t, "Left-right: %s", FEATX[cas_feat]); text_box(106, 134, 146, 12, FS_SMALL, C15(22, 28, 31), AL_L, t); sprintf(t, "Up-down: %s", FEATY[cas_feat]); text_box(106, 146, 146, 12, FS_SMALL, C15(22, 28, 31), AL_L, t); }
        } else {
            for (int i = 0; i < 2; i++) cbtn(45 + i * 48, 41, 46, 15, i ? "Body" : "Face", cas_sg == i, FS_BODY);
            cbtn(141, 41, 52, 15, "Reset", 0, FS_BODY); { char cb[8] = "Max x1"; cb[5] = (char)('0' + cas_mult); cbtn(195, 41, 56, 15, cb, cas_mult > 1, FS_BODY); }
            for (int k = 0; k < 6; k++) { int q = sl_idx(k); if (q < 0) continue; int y = 60 + k * 16, v = m->pm[q];
                text_box(45, y, 58, 13, FS_BODY, TXT, AL_L, sl_name(k)); rect(105, y, 98, 13, PANB); rect(106, y + 1, 96, 11, C15(2, 5, 12)); rect(106, y + 1, v * 96 / 100, 11, C15(6, 18, 28)); rect(105 + 48, y + 1, 1, 11, C15(10, 14, 22));
                rect(102 + v * 96 / 100, y - 1, 6, 15, C15(31, 31, 31)); rect(103 + v * 96 / 100, y, 4, 13, C15(10, 26, 10));
                char nb[8]; int nn = v - 50; nb[0] = nn < 0 ? '-' : '+'; nn = nn < 0 ? -nn : nn; nb[1] = (char)('0' + nn / 10); nb[2] = (char)('0' + nn % 10); nb[3] = 0; text_box(204, y, 46, 13, FS_NUM, C15(22, 28, 31), AL_C, nb); }
        }
    } else if (cas_step == CS_HAIR) {
        static const char *const sn[4] = { "Style", "Colour", "Beard", "Hat" }; char t[24];
        if (cas_sub == 0) { sprintf(t, "Hair %d/%d", cas_page + 1, (NHAIRD + 7) / 8); cas_hdr(t); cas_pager((NHAIRD + 7) / 8); } else cas_hdr("Hair"); cas_subtabs(4, sn);
        if (cas_sub == 0) { face_hat = csx[0].hat; face_bg = C15(13, 17, 26);
            for (int k = 0; k < 8; k++) { int idx = cas_page * 8 + k; if (idx >= NHAIRD) break; int x = 45 + (k % 4) * 52, y = 42 + (k / 4) * 57; cas_tile(x, y, 50, 55, m->hair == idx); Sim v = *m; v.hair = (u8)idx; face(x + 7, y + 3, 36, &v); text_box(x + 2, y + 41, 46, 12, FS_SMALL, PANB, AL_C, HAIRN[idx]); }
            face_hat = -1; face_bg = -1; }
        else if (cas_sub == 1) cas_colpage(0);
        else if (cas_sub == 2) cas_tiles3(m->beard, BEARDN, 6, 1);
        else cas_tiles3(csx[0].hat, HATN, 6, 2);
    } else if (cas_step == CS_OUTFIT) {
        static const char *const sn[3] = { "Top", "Bottom", "Mix" }; cas_hdr("Outfit"); cas_subtabs(3, sn);
        if (cas_sub == 0) cas_tiles3(m->cc[2] ? -1 : m->shirt, SHIRTN, 6, 3); else if (cas_sub == 1) cas_tiles3(csx[0].pants, PANTN, 6, 4); else cas_colpage(0);
    } else if (cas_step == CS_TRAITS) {
        static const char *const sn[2] = { "Traits", "Life wish" }; cas_hdr("Traits"); cas_subtabs(2, sn);
        if (cas_sub == 0) {
            for (int i = 0; i < 3; i++) { int t = csx[0].tr[i]; rect(45 + i * 54, 41, 52, 22, PANB); rect(46 + i * 54, 42, 50, 20, t ? C15(8, 22, 10) : C15(10, 14, 24)); text_box(46 + i * 54, 42, 50, 20, t ? FS_BODY : FS_HEAD, t ? TXT : C15(18, 22, 28), AL_C, t ? TRN[t - 1] : "?"); }
            cbtn(208, 41, 43, 22, "Random", 0, FS_BODY);
            for (int t = 1; t <= 12; t++) { int c = (t - 1) & 1, r = (t - 1) >> 1, x = 45 + c * 104, y = 67 + r * 15, on = cas_tr_at(t) != 0, ok = cas_tr_ok(t);
                rect(x, y, 102, 14, on ? C15(8, 22, 10) : C15(8, 15, 27)); rect(x + 3, y + 2, 10, 10, on ? C15(31, 31, 31) : C15(2, 5, 14)); rect(x + 4, y + 3, 8, 8, ok ? C15(28, 31, 31) : C15(12, 14, 18));
                if (on) { rect(x + 5, y + 6, 2, 2, C15(6, 20, 6)); rect(x + 7, y + 7, 2, 2, C15(6, 20, 6)); rect(x + 9, y + 4, 2, 4, C15(6, 20, 6)); }
                text_box(x + 15, y, 86, 14, FS_BODY, ok ? TXT : C15(21, 24, 29), AL_L, TRN[t - 1]); }
        } else {
            for (int k = 0; k < 5; k++) { int y = 42 + k * 23, on = csx[0].wish == k + 1; rect(45, y, 206, 21, on ? C15(31, 31, 31) : C15(2, 5, 14)); rect(46, y + 1, 204, 19, on ? C15(8, 24, 8) : C15(8, 15, 27)); rect(50, y + 5, 11, 11, C15(2, 5, 14)); rect(51, y + 6, 9, 9, C15(28, 31, 31)); if (on) rect(53, y + 8, 5, 5, C15(8, 22, 8));
                text_box(64, y, 184, 21, FS_HEAD, TXT, AL_L, WSN[k]); }
        }
    } else {   // Done: the summary to look over
        cas_hdr("Your Sim"); text_box(45, 24, 206, 20, FS_TITLE, TXT, AL_C, sname[0]);
        rect(47, 50, 60, 60, PANB); face_hat = csx[0].hat; face_bg = C15(13, 17, 26); face(49, 52, 56, m); face_hat = face_bg = -1;
        text_box(112, 46, 138, 11, FS_SMALL, C15(22, 28, 31), AL_L, "Traits");
        for (int k = 0; k < 3; k++) { int t = csx[0].tr[k]; text_box(112, 57 + k * 13, 138, 13, FS_BODY, t ? TXT : C15(18, 22, 28), AL_L, t ? TRN[t - 1] : "-"); }
        text_box(112, 98, 138, 11, FS_SMALL, C15(22, 28, 31), AL_L, "Life wish"); text_box(112, 109, 138, 13, FS_BODY, csx[0].wish ? TXT : C15(18, 22, 28), AL_L, csx[0].wish ? WSN[csx[0].wish - 1] : "-");
        text_box(45, 114, 62, 11, FS_SMALL, C15(22, 28, 31), AL_C, HAIRN[m->hair % NHAIRD]);
        text_box(45, 132, 206, 24, FS_BODY, C15(24, 28, 31), AL_C, "Tap a tab on the left to change anything.");
    }
}
static const char KB_R[4][11] = { "1234567890", "QWERTYUIOP", "ASDFGHJKL-", "ZXCVBNM'" };
static void cas_draw_kb(void) {
    rect(0, 0, 256, 192, BG_L); panel(2, 2, 252, 188, 0); rect(4, 4, 248, 16, PANH); text_box(28, 4, 200, 16, FS_HEAD, TXT, AL_C, cas_kb == 1 ? "First name" : "Last name");
    cas_field(8, 24, 196, 20, cas_kb == 1 ? cas_first : cas_last, 1, 1); cbtn(208, 24, 40, 20, "Del", 0, FS_BODY);
    for (int r = 0; r < 4; r++) { int n = (int)strlen(KB_R[r]), x0 = 8 + (10 - n) * 12; for (int c = 0; c < n; c++) { char s[2] = { KB_R[r][c], 0 }; if (s[0] >= 'A' && s[0] <= 'Z' && !cas_shift) s[0] = (char)(s[0] + 32); cbtn(x0 + c * 24, 50 + r * 23, 22, 21, s, 0, FS_HEAD); } }
    cbtn(8, 146, 48, 22, "Shift", cas_shift, FS_BODY); cbtn(58, 146, 96, 22, "Space", 0, FS_BODY); cbtn(156, 146, 44, 22, "Cancel", 0, FS_BODY); cbtn(202, 146, 46, 22, "OK", 1, FS_HEAD);
    text_box(8, 170, 240, 14, FS_SMALL, C15(22, 28, 31), AL_C, "Tap the letters. Up to 9 each.");
}
static void cas_kb_add(char ch) { char *b = cas_kb == 1 ? cas_first : cas_last; int n = (int)strlen(b); if (n >= 9) return; if (ch >= 'A' && ch <= 'Z' && !cas_shift) ch = (char)(ch + 32); b[n] = ch; b[n + 1] = 0; cas_shift = 0; cas_name(); }
static int cas_kb_touch(int tx, int ty) {   // 1 when it should redraw
    char *b = cas_kb == 1 ? cas_first : cas_last; int n = (int)strlen(b);
    for (int r = 0; r < 4; r++) { int l = (int)strlen(KB_R[r]), x0 = 8 + (10 - l) * 12; for (int c = 0; c < l; c++) if (hit(x0 + c * 24, 50 + r * 23, 22, 21, tx, ty)) { cas_kb_add(KB_R[r][c]); return 1; } }
    if (hit(208, 24, 40, 20, tx, ty)) { if (n) { b[n - 1] = 0; if (n == 1) cas_shift = 1; cas_name(); } return 1; }
    if (hit(8, 146, 48, 22, tx, ty)) { cas_shift ^= 1; return 1; }
    if (hit(58, 146, 96, 22, tx, ty)) { if (n && n < 9 && b[n - 1] != ' ' && cas_kb == 2) { b[n] = ' '; b[n + 1] = 0; cas_shift = 1; cas_name(); } return 1; }
    if (hit(156, 146, 44, 22, tx, ty)) { cas_kb = cas_kbc = 0; return 1; }
    if (hit(202, 146, 46, 22, tx, ty)) { while (n && b[n - 1] == ' ') b[--n] = 0; if (cas_kb == 1 && cas_kbc) { cas_kb = 2; cas_shift = cas_last[0] == 0; } else { cas_kb = cas_kbc = 0; } cas_name(); return 1; }
    return 0; }
static void cas_draw_pop(void) {
    int w = 208, h = 110, x = 24, y = 41; rect(x - 2, y - 2, w + 4, h + 4, PANB); rect(x, y, w, h, C15(26, 29, 31)); rect(x, y, w, 20, PANH);
    text_box(x, y, w, 20, FS_HEAD, TXT, AL_C, cas_pop == 1 ? "Create a Sim" : cas_pop == 2 ? "Leave?" : "Ready?");
    if (cas_pop == 1) { text_box(x, y + 24, w, 14, FS_BODY, C15(2, 4, 12), AL_C, "Make your Sim in 6 steps."); text_box(x, y + 38, w, 14, FS_BODY, C15(2, 4, 12), AL_C, "Use the tabs, or Back and Next."); text_box(x, y + 52, w, 14, FS_BODY, C15(2, 4, 12), AL_C, "Last, press Play!"); cbtn(x + 64, y + 76, 80, 28, "OK", 1, FS_HEAD); }
    else if (cas_pop == 2) { text_box(x, y + 28, w, 14, FS_BODY, C15(2, 4, 12), AL_C, "Leave without making a Sim?"); cbtn(x + 12, y + 70, 86, 30, "Leave", 0, FS_HEAD); cbtn(x + 110, y + 70, 86, 30, "Stay", 1, FS_HEAD); }
    else { text_box(x, y + 28, w, 14, FS_HEAD, C15(2, 4, 12), AL_C, sname[0]); text_box(x, y + 46, w, 14, FS_BODY, C15(2, 4, 12), AL_C, "Start playing with this Sim?"); cbtn(x + 12, y + 70, 86, 30, "Play!", 1, FS_HEAD); cbtn(x + 110, y + 70, 86, 30, "Edit", 0, FS_HEAD); }
}
static int cas_pop_touch(int tx, int ty) {
    int x = 24, y = 41;
    if (cas_pop == 1) { if (hit(x + 64, y + 76, 80, 28, tx, ty)) cas_pop = 0; return 1; }
    if (hit(x + 12, y + 70, 86, 30, tx, ty)) { if (cas_pop == 2) cas_done = 2; else cas_done = 1; cas_pop = 0; return 1; }
    if (hit(x + 110, y + 70, 86, 30, tx, ty)) { cas_pop = 0; return 1; }
    return 1; }
static void draw_cas(void) {
    scr = scr_sub;
    if (cas_kb) { cas_draw_kb(); if (cas_pop) cas_draw_pop(); return; }
    backdrop();
    for (int i = 0; i < CS_N; i++) { int on = cas_step == i, y = 2 + i * 31; rect(2, y, 36, 29, on ? C15(31, 31, 31) : C15(2, 5, 14)); rect(3, y + 1, 34, 27, on ? C15(8, 26, 8) : C15(4, 10, 24)); rect(3, y + 1, 34, 1, on ? C15(18, 31, 18) : C15(9, 16, 30)); cas_icon(14, y + 3, i, on); text_box(2, y + 16, 36, 11, FS_BODY, on ? C15(2, 6, 2) : TXT, AL_C, CSN[i]); }
    cas_draw_step();
    cbtn(41, 165, 70, 24, cas_step ? "< Back" : "< Title", 0, FS_HEAD); cbtn(184, 165, 70, 24, cas_step == CS_DONE ? "Play!" : "Next >", cas_step == CS_DONE ? 1 : 0, FS_HEAD);
    { char t[12]; sprintf(t, "%d / %d", cas_step + 1, CS_N); text_box(112, 165, 70, 24, FS_NUM, TXT, AL_C, t); }
    if (cas_pop) cas_draw_pop();
}
static void cas_touch(int tx, int ty) {
    if (cas_pop) { cas_pop_touch(tx, ty); return; }
    if (cas_kb) { cas_kb_touch(tx, ty); return; }
    Sim *m = &sims[0]; cas_drag = 0;
    for (int i = 0; i < CS_N; i++) if (hit(2, 2 + i * 31, 36, 29, tx, ty)) { if (i != cas_step) cas_goto(i); return; }
    if (hit(41, 165, 70, 24, tx, ty)) { if (cas_step) cas_goto(cas_step - 1); else cas_pop = 2; return; }
    if (hit(184, 165, 70, 24, tx, ty)) { if (cas_step == CS_DONE) cas_pop = 3; else cas_goto(cas_step + 1); return; }
    if (cas_step == CS_BASICS) {
        if (cas_mixo) { if (cas_coltouch(1, tx, ty)) cas_mixo = 0; if (cas_drag) cas_drag_at(tx, ty); return; }
        if (hit(45, 24, 92, 18, tx, ty)) { cas_kb = 1; cas_kbc = 1; cas_shift = cas_first[0] == 0; }
        else if (hit(139, 24, 92, 18, tx, ty)) { cas_kb = 2; cas_kbc = 0; cas_shift = cas_last[0] == 0; }
        else if (hit(233, 24, 18, 18, tx, ty)) cas_rname();
        else if (hit(45, 56, 48, 24, tx, ty)) cas_gender(0); else if (hit(95, 56, 48, 24, tx, ty)) cas_gender(1);
        else if (hit(153, 56, 100, 24, tx, ty)) { int f = (tx - 153) / 34; if (f >= 0 && f < 3) cas_fit(f); }
        else if (hit(45, 96, 162, 24, tx, ty)) { int i = (tx - 45) / 27; if (i < 6) { m->skin = (u8)i; m->cc[0] = 0; } }
        else if (hit(209, 96, 42, 24, tx, ty)) { cas_mixo = 1; cas_tgt = 0; cas_load_rgb(); }
        else if (hit(45, 128, 206, 28, tx, ty)) cas_random();
        return; }
    if (cas_step == CS_LOOKS) {
        int s = cas_subhit(3, tx, ty); if (s >= 0) { if (s != cas_sub) cas_setsub(s); return; }
        if (cas_sub == 0) { int k = cas_tiles3_hit(tx, ty); if (k >= 0) m->fc = (u8)k; }
        else if (cas_sub == 1) {
            for (int i = 0; i < 8; i++) if (hit(45, 42 + i * 14, 58, 13, tx, ty)) cas_feat = i;
            if (hit(PADX - 6, PADY - 6, PADW + 12, PADH + 12, tx, ty)) { cas_drag = 1; cas_drag_at(tx, ty); }
            if (hit(209, 44, 42, 20, tx, ty)) { m->pm[cas_feat * 2] = 50; m->pm[cas_feat * 2 + 1] = 50; }
        } else {
            int pg = (sl_n() + 5) / 6; if (hit(CPX + 3, 5, 20, 14, tx, ty) && cas_sp > 0) cas_sp--; if (hit(CPX + CPW - 23, 5, 20, 14, tx, ty) && cas_sp + 1 < pg) cas_sp++;
            for (int i = 0; i < 2; i++) if (hit(45 + i * 48, 41, 46, 15, tx, ty)) { cas_sg = i; cas_sp = 0; }
            if (hit(195, 41, 56, 15, tx, ty)) cas_mult = cas_mult % 3 + 1;
            if (hit(141, 41, 52, 15, tx, ty)) for (int j = 0; j < sl_n(); j++) m->pm[cas_sg ? SLB[j].i : SLF[j].i] = 50;
            for (int k = 0; k < 6; k++) if (sl_idx(k) >= 0 && hit(100, 59 + k * 16, 108, 16, tx, ty)) { cas_drag = 20 + k; cas_drag_at(tx, ty); } }
        return; }
    if (cas_step == CS_HAIR) {
        int s = cas_subhit(4, tx, ty); if (s >= 0) { if (s != cas_sub) cas_setsub(s); return; }
        if (cas_sub == 0) { int pg = (NHAIRD + 7) / 8; if (hit(CPX + 3, 5, 20, 14, tx, ty) && cas_page > 0) cas_page--; if (hit(CPX + CPW - 23, 5, 20, 14, tx, ty) && cas_page + 1 < pg) cas_page++;
            for (int k = 0; k < 8; k++) if (hit(45 + (k % 4) * 52, 42 + (k / 4) * 57, 50, 55, tx, ty) && cas_page * 8 + k < NHAIRD) m->hair = (u8)(cas_page * 8 + k); }
        else if (cas_sub == 1) { cas_coltouch(0, tx, ty); if (cas_drag) cas_drag_at(tx, ty); }
        else if (cas_sub == 2) { int k = cas_tiles3_hit(tx, ty); if (k >= 0) m->beard = (u8)k; }
        else { int k = cas_tiles3_hit(tx, ty); if (k >= 0) csx[0].hat = (u8)k; }
        return; }
    if (cas_step == CS_OUTFIT) {
        int s = cas_subhit(3, tx, ty); if (s >= 0) { if (s != cas_sub) cas_setsub(s); return; }
        if (cas_sub == 0) { int k = cas_tiles3_hit(tx, ty); if (k >= 0) { m->shirt = (u8)k; m->cc[2] = 0; } } else if (cas_sub == 1) { int k = cas_tiles3_hit(tx, ty); if (k >= 0) csx[0].pants = (u8)k; }
        else { cas_coltouch(0, tx, ty); if (cas_drag) cas_drag_at(tx, ty); }
        return; }
    if (cas_step == CS_TRAITS) {
        int s = cas_subhit(2, tx, ty); if (s >= 0) { if (s != cas_sub) cas_setsub(s); return; }
        if (cas_sub == 0) {
            if (hit(208, 41, 43, 22, tx, ty)) { cas_tr_random(); cas_note = 0; }
            for (int t = 1; t <= 12; t++) if (hit(45 + ((t - 1) & 1) * 104, 67 + ((t - 1) >> 1) * 15, 102, 14, tx, ty)) { cas_tr_tap(t); cas_note = 1; cas_notei = t - 1; }
        } else for (int k = 0; k < 5; k++) if (hit(45, 42 + k * 23, 206, 21, tx, ty)) { csx[0].wish = (u8)(k + 1); csx[0].wdone = 0; cas_note = 2; cas_notei = k; }
        return; }
}
static void cas_top(void) {   // the top screen's overlay: the name, and (Traits) what the trait or the wish is
    hud_clear(); int w = tw(2, sname[0]) + 16; panel(4, 4, w, 20, 0); text_box(4, 4, w, 20, FS_HEAD, TXT, AL_C, sname[0]);   // (top left: the Sim is in the middle, whole or close)
    if (cas_step == CS_TRAITS && cas_note) {
        const char *a = cas_note == 1 ? TRN[cas_notei] : WSN[cas_notei], *b = cas_note == 1 ? TRD[cas_notei] : WSD[cas_notei];
        panel(24, 138, 208, 48, 0); rect(26, 140, 204, 16, PANH); text_box(26, 140, 204, 16, FS_HEAD, TXT, AL_C, a); text_box(26, 158, 204, 24, FS_BODY, TXT, AL_C, b); }
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
static int sky_now(void) { if (o_sky < 4 && !o_rt) return o_sky;   // (Real time: the sky follows the clock)
    int h = (minutes / 60) % 24; return h < 5 || h >= 20 ? 2 : h < 7 || h >= 18 ? 1 : 0; }
static void snd_apply(void);
// ---- (D40) THE LOOK: a sky drawn by the 2D engine behind the 3D (one colour a scanline, changed in each hblank by DMA, graded to the camera's horizon), haze that
// fades the far land into it (the GPU's fog on alpha only), highlights from a shininess table, and in Toon outlines round every thing (edge marking by polygon id).
static u16 sky_tbl[194] __attribute__((aligned(32))); static int sky_on; static u16 sky_hz, sky_zn;
static void sky_vbl(void) {   // (vblank) restart the hblank DMA (channel 1) on the main engine's backdrop colour
    DMA_CR(1) = 0; if (!sky_on) return; BG_PALETTE[0] = sky_tbl[0]; DMA_SRC(1) = (u32)(sky_tbl + 1); DMA_DEST(1) = (u32)BG_PALETTE;
    DMA_CR(1) = DMA_ENABLE | DMA_REPEAT | DMA_START_HBL | DMA_16_BIT | DMA_SRC_INC | DMA_DST_FIX | 1; }
static u16 mix15(u16 a, u16 b, int t) { int r = (a & 31) + (((b & 31) - (a & 31)) * t >> 8), g = ((a >> 5) & 31) + ((((b >> 5) & 31) - ((a >> 5) & 31)) * t >> 8), bl = ((a >> 10) & 31) + ((((b >> 10) & 31) - ((a >> 10) & 31)) * t >> 8); return (u16)RGB15(r, g, bl); }
static void sky_update(int pitch) {   // the sky for this camera: the horizon's line on the screen follows the tilt (40 degrees of view: 20 each side of the middle)
    int pd = pitch * 360 / 32768;   // degrees down
    for (int y = 0; y < 193; y++) { int e = -pd * 16 + (96 - y) * 20 * 16 / 96, t = e <= 0 ? 0 : e >= 40 * 16 ? 256 : e * 256 / (40 * 16); t = t * (512 - t) >> 8;   // (elevation in 1/16 degrees; eased)
        sky_tbl[y] = e < 0 ? mix15(sky_hz, C15(31, 31, 31), (-e > 64 ? 64 : -e) / 4) : mix15(sky_hz, sky_zn, t); }
    DC_FlushRange(sky_tbl, sizeof sky_tbl);
}
static int fog_dep(int d, int f) { int n = 3072; s64 z = ((s64)(f + n) << 12) / (f - n) - ((((s64)2 * f * n) << 12) / ((s64)(f - n) * d)); return (int)((z * 0x4000 >> 12) + 0x3FFF); }   // the 15-bit depth the fog reads at a distance d (4.12)
static void fog_setup(int d0, int d1, int alpha) {   // haze from d0 to d1 (4.12 units from the eye): the GPU's fog table over the z-buffer's depth
    int f = gx_far * 4096, a = fog_dep(d0, f), b = fog_dep(d1, f), sh = 0; while (sh < 10 && 32 * (0x400 >> (sh + 1)) >= b - a) sh++; int step = 0x400 >> sh;
    glFogShift(sh); glFogOffset(a);
    for (int q = 0; q < 32; q++) { int dep = a + q * step, lo = 0, hi = f;   // the distance at this depth (by halving), then the haze there
        if (dep >= 0x7FFF) { glFogDensity(q, 127); continue; } for (int it = 0; it < 16; it++) { int m = (lo + hi) / 2; if (fog_dep(m < 3100 ? 3100 : m, f) < dep) lo = m; else hi = m; }
        int t = (lo - d0) * 128 / (d1 - d0); if (t < 0) t = 0; if (t > 127) t = 127; glFogDensity(q, t * t / 127); }
    GFX_CONTROL = (GFX_CONTROL & ~(u16)GL_FOG_ONLY_ALPHA) | (alpha ? GL_FOG_ONLY_ALPHA : 0); glEnable(GL_FOG);
}
static int cpu_ticks;   // (D40) bus ticks of a fixed loop at start: about half on a DSi's 134 MHz CPU
ITCM_CODE static void cpu_probe(void) {   // a register-only loop in the ITCM (no memory waits): its time is the CPU clock's alone
    TIMER_CR(2) = 0; TIMER_DATA(2) = 0; TIMER_CR(2) = TIMER_ENABLE | TIMER_DIV_64; __asm__ volatile("mov r0, #65536\n1: subs r0, r0, #1\n bne 1b" ::: "r0", "cc"); cpu_ticks = TIMER_DATA(2); TIMER_CR(2) = 0;
    fast_cpu = cpu_ticks > 0 && cpu_ticks < 1536; }   // (65536 turns of about 4 cycles: 2048 ticks at 67 MHz, 1024 at 134 MHz)
static void apply_opts(void) {
    if (o_aa) glEnable(GL_ANTIALIAS); else glDisable(GL_ANTIALIAS);
    static const u8 sk[4][3] = { { 14, 20, 31 }, { 31, 18, 12 }, { 3, 4, 10 }, { 14, 28, 22 } }; int sn = sky_now();
    static const u16 SZ[4] = { C15(7, 13, 28), C15(9, 7, 19), C15(1, 2, 6), C15(8, 20, 19) }, SH[4] = { C15(21, 26, 31), C15(31, 21, 13), C15(6, 8, 16), C15(22, 30, 26) };
    sky_zn = SZ[sn]; sky_hz = SH[sn];
    { static const u16 oc[4] = { RGB15(2, 2, 5), RGB15(0, 0, 0), RGB15(31, 31, 31), RGB15(31, 12, 20) }; int on = o_out || o_look == 2; u16 c = o_out ? oc[o_ocol] : RGB15(3, 3, 6);
      if (on) { glEnable(GL_OUTLINE); for (int k = 0; k < 8; k++) glSetOutlineColor(k, c); } else glDisable(GL_OUTLINE); }
    setBrightness(3, (o_bri - 4) * 3);
    if (isDSiMode() && o_turbo != 2) setCpuClock(true); else if (isDSiMode()) setCpuClock(false);
    prof_full = o_detl == 2 || (o_detl == 0 && (fast_cpu || isDSiMode()));
    { static const u8 RR[2][3] = { { 1, 2, 2 }, { 2, 3, 3 } }; ring_r = RR[prof_full][o_dd]; gx_far = 10 + ring_r * 4; if (o_look == 0) { ring_r = 1; gx_far = 12; } }   // (Classic: the D38 view)
    int rich = o_look > 0 && mode != M_TITLE && mode != M_CAS;
    if (rich) { glClearColor(sk[sn][0], sk[sn][1], sk[sn][2], 0); REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG0 | BLEND_DST_BACKDROP; sky_on = 1; }
    else { glClearColor(sk[sn][0], sk[sn][1], sk[sn][2], 31); REG_BLDCNT = 0; sky_on = 0; }
    { static const int fn[3] = { 10, 20, 30 }, ff[3] = { 45, 60, 80 }; int F = gx_far * 4096;   // (Fog on: thick, from close; Rich and Toon: a haze at the end of the view only)
      if (o_fog) { glFogColor(sk[sn][0], sk[sn][1], sk[sn][2], rich ? 0 : 31); fog_setup(F / 100 * fn[o_fogd], F / 100 * ff[o_fogd], rich); }
      else if (rich) { glFogColor(0, 0, 0, 0); fog_setup(F / 100 * 50, F / 100 * 94, 1); } else glDisable(GL_FOG); }
    { int sp = o_look == 0 ? 0 : o_look == 1 ? 1 : 2; static u8 shin[128] __attribute__((aligned(4)));   // highlights: a soft curve (Rich), two hard bands (Toon)
      for (int q = 0; q < 128; q++) { int t = q * 2; shin[q] = (u8)(sp == 2 ? (t > 200 ? 255 : t > 150 ? 110 : 0) : (t * t / 255) * t / 255); }
      for (int q = 0; q < 32; q++) GFX_SHININESS = (u32)shin[q * 4] | (u32)shin[q * 4 + 1] << 8 | (u32)shin[q * 4 + 2] << 16 | (u32)shin[q * 4 + 3] << 24;
      gx_spec = sp == 0 ? 0 : sp == 1 ? (RGB15(6, 6, 5) | BIT(15)) : (RGB15(9, 9, 8) | BIT(15)); }
    speed = o_spd + 1; snd_apply(); o_dirty = 0;
}
// ---- (D70) OPTIONS: eight pages as touch tabs down the left of the bottom screen, big rows, the help for the chosen row on the top screen.
// Pages: GAME SCREEN SOUND CONTROLS TIME SAVE EXTRAS ABOUT. To add a row: add it to OPT above (at the end) and put its name in OPL below. Rows starting with @ are buttons.
typedef struct { const char *n, *d; } OpD;
static const OpD OPD[] = {
    { "Sim style", "How people look: Classic, Sims 3 or Chunky." }, { "Roofs", "Cutaway shows the room you are in. Always keeps roofs on. Hide takes them off." },
    { "Traffic", "How many cars drive on the streets." }, { "Needs speed", "How fast needs go down. Never fall keeps them full." }, { "Message time", "How long a message stays on the screen." },
    { "Ground check", "Checks the ground under things. Best left on." }, { "3D look", "Classic is plain. Rich adds a sky and haze. Toon adds outlines." },
    { "Brightness", "Makes both screens lighter or darker." }, { "Sky", "The colour of the sky. Auto follows the time of day." }, { "Anti-alias", "Smooths rough edges. Off is a little faster." },
    { "Outlines", "Draws a dark line round things." }, { "Outline color", "The colour of those lines." }, { "Shadows", "Soft shadows under things." }, { "Lighting", "Light and shade on things." },
    { "Textures", "Patterns on the ground, walls and roofs." }, { "Item detail", "Detailed things look nicer but need more speed." }, { "Fog", "A thick haze in the distance." },
    { "Fog range", "How near the haze starts." }, { "Draw distance", "How far you can see. Near is faster." },
    { "Master volume", "How loud everything is." }, { "Music volume", "How loud the music is." }, { "Effects volume", "How loud clicks and sounds are." },
    { "Music source", "Songs plays the real tunes. Synth plays small beeps." }, { "Music track", "Pick a synth tune, or Auto." }, { "Song", "Pick a song, or Auto." },
    { "Shuffle songs", "Play songs in order, or at random." }, { "Title music", "Music on the title screen." }, { "Game music", "Music while you play." },
    { "Tap clicks", "A click when you touch the screen." }, { "Chimes", "Little tunes for good and bad news." }, { "Stereo", "Spread puts sounds to the left and right." },
    { "Hold repeat", "How fast a held button repeats. Hold LEFT or RIGHT to change fast." }, { "Hints bar", "Shows which buttons do what." },
    { "Camera", "How close the camera sits." }, { "Map spin", "How fast the map turns." },
    { "Clock", "Show the time with 12 or 24 hours." }, { "Game speed", "How fast time passes in the game." }, { "Day length", "How long a game day lasts." },
    { "Real time", "Uses the real date and time of your DS." }, { "Boreagotchi time", "Game clock. LIVE keeps real time. Off pauses the pets." },
    { "Autosave", "Saves the game when a day ends." }, { "Save space", "Safe is the normal size. Max uses more of the save chip." },
    { "@Save now", "Writes your game to the save chip right now." }, { "@Reset all", "Puts every option back how it was. Tap twice." },
    { "Frame rate", "Steady 30 is smooth. Up to 60 is faster when it can be." }, { "Turbo CPU", "Auto uses the fast processor of a DSi." },
    { "Detail", "Auto picks the detail from your DS." }, { "Poly meter", "Shows frames a second on the screen." },
};
static const char *const OPN[8] = { "GAME", "SCREEN", "SOUND", "CONTROLS", "TIME", "SAVE", "EXTRAS", "ABOUT" };
static const char *const OPL[8] = {
    "Sim style|Roofs|Traffic|Needs speed|Message time|Ground check",
    "3D look|Brightness|Sky|Anti-alias|Outlines|Outline color|Shadows|Lighting|Textures|Item detail|Fog|Fog range|Draw distance",
    "Master volume|Music volume|Effects volume|Music source|Music track|Song|Shuffle songs|Title music|Game music|Tap clicks|Chimes|Stereo",
    "Hold repeat|Hints bar|Camera|Map spin",
    "Clock|Game speed|Day length|Real time|Boreagotchi time",
    "Autosave|Save space|@Save now|@Reset all",
    "Frame rate|Turbo CPU|Detail|Poly meter",
    "" };
#define OP_ROWS 6   // rows on view
static int op_sel, op_top, op_n, op_ix[32], op_conf, op_hdir, op_hrow, op_hcnt, op_rd = 1, title_rd;
static u32 op_hnext, op_sg[24]; static int op_full;   // op_full: 1 bottom screen from scratch, 2 top screen from scratch
static const char *op_nm[32];
static int opt_find(const char *n, int len) { for (int i = 0; i < NOPT; i++) if ((int)strlen(OPT[i].name) == len && !strncmp(OPT[i].name, n, (size_t)len)) return i; return -1; }
static void op_build(void) {
    op_n = 0; const char *p = OPL[o_tab];
    while (*p && op_n < 32) { const char *e = p; while (*e && *e != '|') e++; int len = (int)(e - p);
        if (*p == '@') { op_ix[op_n] = -1 - (p[1] == 'R'); op_nm[op_n++] = p + 1; }   // -1 Save now, -2 Reset all
        else { int i = opt_find(p, len); if (i >= 0) { op_ix[op_n] = i; op_nm[op_n++] = OPT[i].name; } }
        p = *e ? e + 1 : e; }
    if (op_sel >= op_n) op_sel = op_n ? op_n - 1 : 0;
    if (op_sel < 0) op_sel = 0;
    if (op_top > op_sel) op_top = op_sel;
    if (op_sel >= op_top + OP_ROWS) op_top = op_sel - OP_ROWS + 1;
    if (op_top < 0) op_top = 0;
}
static const char *op_rname(int q, char *b) { if (op_ix[q] >= 0) return OPT[op_ix[q]].name; const char *s = op_nm[q]; int k = 0; while (s[k] && s[k] != '|' && k < 20) { b[k] = s[k]; k++; } b[k] = 0; return b; }
static const char *op_desc(int q) {
    char b[24], key[26]; const char *nm = op_rname(q, b); if (op_ix[q] < 0) { key[0] = '@'; strcpy(key + 1, nm); nm = key; }
    for (unsigned k = 0; k < sizeof OPD / sizeof OPD[0]; k++) if (!strcmp(OPD[k].n, nm)) return OPD[k].d;
    return ""; }
static int op_clamp_type(int i) { int c = opt_cnt(i); return c >= 9 && c <= 20; }   // volumes and brightness stop at the ends; the rest wrap round
static int op_change(int i, int dir) {   // dir -1 / +1 (d-pad, arrows), 2 a tap on the row (always forward and round); returns 1 if the value moved
    int c = opt_cnt(i), v = *OPT[i].v, nv = v;
    if (dir == 2) nv = (v + 1) % c; else if (op_clamp_type(i)) { nv = v + dir; if (nv < 0 || nv >= c) return 0; } else nv = (v + dir + c) % c;
    if (nv == v) return 0;
    *OPT[i].v = nv; o_dirty = 1; return 1;
}
static void op_reset(int all) {
    if (!all) { for (int q = 0; q < op_n; q++) if (op_ix[q] >= 0 && OPT[op_ix[q]].v) *OPT[op_ix[q]].v = OPT[op_ix[q]].def; }
    else for (int i = 0; i < NOPT; i++) if (OPT[i].v && OPT[i].v != &o_smax && OPT[i].v != &o_tab) *OPT[i].v = OPT[i].def;
    o_dirty = 1; snd_apply();
}
static int op_changed(int *tot) { int n = 0, t = 0; for (int q = 0; q < op_n; q++) if (op_ix[q] >= 0 && OPT[op_ix[q]].v) { t++; if (*OPT[op_ix[q]].v != OPT[op_ix[q]].def) n++; } *tot = t; return n; }
static void opt_enter(void) { opt_on = 1; if (o_tab < 0 || o_tab > 7) o_tab = 0; op_sel = op_top = 0; op_conf = op_hdir = 0; op_build(); op_full = 3; op_rd = 1; }
static void op_back(void) { opt_on = 0; op_hdir = 0; ui_snd(UI_BACK); if (opt_ret >= 0) { int m = opt_ret; opt_ret = -1; set_mode(m); } else title_rd = 1; }
static void op_page(int k) { k = (k + 8) % 8; if (k == o_tab) return; o_tab = k; op_sel = op_top = 0; op_conf = op_hdir = 0; op_build(); memset(op_sg + 1, 0, sizeof(u32) * 9); if (op_n < OP_ROWS) op_full |= 1; op_rd = 1; ui_snd(UI_MOVE); }
static void op_select(int q) { if (q < 0 || q >= op_n) return; if (q != op_sel) { op_sel = q; ui_snd(UI_MOVE); } if (op_sel < op_top) op_top = op_sel; if (op_sel >= op_top + OP_ROWS) op_top = op_sel - OP_ROWS + 1; op_rd = 1; }
static int op_can_save(void) { return sav_ok() && opt_ret >= 0 && lot_bx >= 0; }
static void op_action(int q) {
    if (op_ix[q] == -1) { if (!sav_ok()) say("No save chip found"); else if (!op_can_save()) say("Start a game first"); else { say(sav_write() ? "Game saved" : "Save failed"); ui_snd(UI_OK); } }
    else if (op_conf > 0) { op_reset(1); op_conf = 0; say("All options reset"); ui_snd(UI_OK); }
    else { op_conf = 90; ui_snd(UI_MOVE); say("Tap again to reset all"); }
    op_rd = 1;
}
static void op_use(int q, int dir) {   // A, a tap, or an arrow on row q
    if (op_ix[q] < 0) { if (dir == 2) op_action(q); return; }
    if (op_change(op_ix[q], dir)) { ui_snd(dir == 2 ? UI_OK : UI_MOVE); if (OPT[op_ix[q]].v == &o_sfx || OPT[op_ix[q]].v == &o_vol) sfx(1); }
    op_rd = 1;
}
static int op_rep_iv(int n) { int sc = o_rep == 0 ? 3 : o_rep == 1 ? 2 : 1; int iv = n < 4 ? 8 : n < 10 ? 4 : n < 24 ? 2 : 1; iv = (iv * sc + 1) / 2; return iv < 1 ? 1 : iv; }
static void op_reset_page(void) {
    if (op_conf > 0) { op_reset(0); op_conf = 0; say("Page reset"); ui_snd(UI_OK); } else { op_conf = 90; ui_snd(UI_MOVE); say("Tap again to reset this page"); }
    op_rd = 1;
}
static void op_touch(int tx, int ty) {
    if (ui_back_hit(tx, ty)) { op_back(); return; }
    if (tx < 50) { int k = ty / 24; if (k < 8) op_page(k); return; }
    if (o_tab == 7) return;
    if (hit(52, 162, 76, 24, tx, ty)) { op_reset_page(); return; }
    if (hit(130, 162, 24, 24, tx, ty)) { int t = op_top - 3; if (t < 0) t = 0; op_top = t; if (op_sel >= op_top + OP_ROWS) op_sel = op_top + OP_ROWS - 1; ui_snd(UI_MOVE); op_rd = 1; return; }
    if (hit(156, 162, 24, 24, tx, ty)) { int t = op_top + 3, mx = op_n - OP_ROWS; if (mx < 0) mx = 0; if (t > mx) t = mx; op_top = t; if (op_sel < op_top) op_sel = op_top; ui_snd(UI_MOVE); op_rd = 1; return; }
    for (int k = 0; k < OP_ROWS; k++) { int q = op_top + k; if (q >= op_n || !hit(52, 3 + k * 26, 196, 24, tx, ty)) continue;
        op_select(q); int dir = 2; if (op_ix[q] >= 0) { if (tx < 164 && tx >= 140) dir = -1; else if (tx >= 224) dir = 1; }
        if (op_ix[q] >= 0 && dir != 2) { op_hdir = dir; op_hrow = q; op_hcnt = 0; op_hnext = vbl_n + 20; }
        op_use(q, dir); return; }
}
static void op_keys(int held, int down) {   // d-pad with hold-to-repeat, A change, B back, L R pages, SELECT reset page
    int r = ui_rep(held, down, vbl_n);
    if (down & (KEY_B | KEY_START)) { op_back(); return; }
    if (down & KEY_L) { op_page(o_tab - 1); return; }
    if (down & KEY_R) { op_page(o_tab + 1); return; }
    if (o_tab == 7) return;
    if (down & (KEY_SELECT | KEY_X)) op_reset_page();
    if (r & KEY_UP) op_select(op_sel - 1);
    if (r & KEY_DOWN) op_select(op_sel + 1);
    if (op_sel < op_n) { if (r & KEY_LEFT) op_use(op_sel, -1); if (r & KEY_RIGHT) op_use(op_sel, 1); if (down & KEY_A) op_use(op_sel, 2); }
}
static void op_tick(int held) {   // a stylus held on an arrow repeats it; the reset question times out; a toast that ends is wiped
    if (op_hdir) { if (!(held & KEY_TOUCH)) op_hdir = 0; else if ((int)(vbl_n - op_hnext) >= 0 && op_hrow < op_n && op_ix[op_hrow] >= 0) { op_hcnt++; op_hnext = vbl_n + (u32)op_rep_iv(op_hcnt); if (op_change(op_ix[op_hrow], op_hdir)) { ui_snd(UI_MOVE); op_rd = 1; } } }
    if (op_conf > 0) { op_conf--; if (!op_conf) op_rd = 1; }
    static int lm; if ((msg_t > 0) != lm) { lm = msg_t > 0; op_rd = 1; }
}
static void op_arrow(int cx, int cy, int dir, u16 c) { for (int k = 0; k < 5; k++) rect(dir < 0 ? cx - 2 + k : cx + 2 - k, cy - k, 1, 2 * k + 1, c); }   // a small triangle pointing left (dir<0) or right
// The two screens are drawn straight into video memory, one part at a time: a part is drawn again only when what it shows has changed (op_sg keeps a number for each),
// so a key press redraws one or two rows, not 192 lines. (A whole 96 KB screen in main memory plus a copy took 17 ms: more than a frame.)
// op_sg, op_full: 1 bottom screen from scratch, 2 top screen from scratch
static void op_row(int k) {
    int q = op_top + k, y = 3 + k * 26, sel = q == op_sel; char nb[24], vb[16];
    rect(52, y, 196, 24, C15(2, 4, 12)); rect(53, y + 1, 194, 22, sel ? C15(12, 22, 31) : C15(6, 12, 26)); rect(53, y + 1, 194, 1, sel ? C15(20, 28, 31) : C15(9, 16, 29));
    if (op_ix[q] >= 0) { int i = op_ix[q], v = *OPT[i].v, c = opt_cnt(i), cl = op_clamp_type(i), ch = v != OPT[i].def; u16 dim = C15(4, 7, 14), on = C15(3, 7, 16);
        text_box(54, y, 86, 24, FS_BODY, TXT, AL_L, OPT[i].name);
        rect(140, y + 2, 24, 20, cl && v == 0 ? dim : on); op_arrow(152, y + 12, -1, cl && v == 0 ? C15(10, 12, 16) : C15(28, 30, 31));
        rect(164, y + 2, 58, 20, ch ? C15(5, 17, 8) : C15(2, 5, 13)); text_box(164, y + 2, 58, 20, FS_BODY, TXT, AL_C, opt_tok(i, v, vb));
        rect(224, y + 2, 24, 20, cl && v == c - 1 ? dim : on); op_arrow(236, y + 12, 1, cl && v == c - 1 ? C15(10, 12, 16) : C15(28, 30, 31)); }
    else ui_button(54, y + 2, 192, 20, op_rname(q, nb), op_ix[q] == -1 ? (op_can_save() ? 0 : 3) : (op_conf > 0 ? 2 : 0));
    if (sel) ui_focus(52, y, 196, 24);
}
static void op_draw_sub(void) {
    scr = scr_sub;
    if (op_full & 1) { backdrop(); memset(op_sg, 0, sizeof op_sg); ui_back_btn(); op_full &= ~1; }
    if (op_sg[0] != 1u + (u32)o_tab) { op_sg[0] = 1u + (u32)o_tab; for (int k = 0; k < 8; k++) { int y = k * 24, on = k == o_tab; rect(0, y, 50, 24, on ? C15(31, 31, 31) : C15(2, 4, 12)); rect(1, y + 1, on ? 52 : 48, 22, on ? C15(8, 18, 31) : C15(6, 10, 22)); text_box(0, y, on ? 52 : 50, 24, FS_BODY, TXT, AL_C, OPN[k]); } }
    if (o_tab == 7) {
        static const char *const L[] = { "BORE 2 for Nintendo DS", "Build D70", "", "Touch and buttons both work.", "A change    B back", "L R switch pages", "Hold a key to repeat it.", "Tap an arrow and hold it too.", "SELECT resets the page." };
        if (!op_sg[1]) { op_sg[1] = 1; rect(54, 3, 194, 156, PANB); rect(56, 5, 190, 152, PAN); rect(52, 162, 128, 24, C15(18, 25, 31));
            for (int i = 0; i < 9; i++) if (L[i][0]) text(62, 12 + i * 16, i == 0 ? FS_HEAD : FS_BODY, i < 2 ? C15(31, 31, 20) : C15(26, 29, 31), L[i]); }
        return;
    }
    for (int k = 0; k < OP_ROWS; k++) { int q = op_top + k; if (q >= op_n) break; u32 sg = 0x80000000u | (u32)q << 20 | (q == op_sel) << 19 | (u32)(op_ix[q] >= 0 ? *OPT[op_ix[q]].v : op_ix[q] == -1 ? op_can_save() : op_conf > 0) << 8 | (u32)op_ix[q] & 0xFF;
        if (op_sg[1 + k] != sg) { op_sg[1 + k] = sg; op_row(k); } }
    { u32 sg = 0x80000000u | (u32)op_top << 8 | (u32)op_n; if (op_sg[8] != sg) { op_sg[8] = sg; rect(250, 3, 4, 156, C15(2, 4, 12));
        if (op_n > OP_ROWS) { int th_ = 156 * OP_ROWS / op_n; if (th_ < 10) th_ = 10; int ty_ = 3 + (156 - th_) * op_top / (op_n - OP_ROWS); rect(250, ty_, 4, th_, C15(24, 28, 31)); } } }
    { int tl = op_top > 0, bt = op_top + OP_ROWS < op_n; u32 sg = 0x80000000u | (u32)tl | (u32)bt << 1 | (u32)(op_conf > 0) << 2; if (op_sg[9] != sg) { op_sg[9] = sg;
        rect(52, 162, 128, 24, C15(18, 25, 31)); ui_button(52, 162, 76, 24, op_conf > 0 ? "Sure? Tap" : "Reset page", op_conf > 0 ? 2 : 3);
        ui_button(130, 162, 24, 24, "", tl ? 0 : 3); ui_button(156, 162, 24, 24, "", bt ? 0 : 3);
        for (int k = 0; k < 5; k++) { rect(142 - k, 170 + k, 2 * k + 1, 1, tl ? C15(31, 31, 31) : C15(18, 20, 24)); rect(168 - (4 - k), 170 + k, 2 * (4 - k) + 1, 1, bt ? C15(31, 31, 31) : C15(18, 20, 24)); } } }
}
static u32 op_psig(void) {   // what the preview shows: its own numbers
    if (o_tab == 1) return (u32)sky_now() | (u32)o_out << 3 | (u32)o_ocol << 4 | (u32)o_look << 7 | (u32)o_bri << 10 | 1u << 20;
    if (o_tab == 2) return (u32)o_vol | (u32)o_mus << 4 | (u32)o_sfx << 8 | 2u << 20;
    if (o_tab == 4) return (u32)(minutes % 1440) | (u32)(o_day & 3) << 11 | (u32)o_spd << 13 | (u32)o_c24 << 15 | 4u << 20;
    if (o_tab == 7) return 7u << 20;
    int tot, n = op_changed(&tot); return (u32)n | (u32)tot << 8 | (u32)o_tab << 16 | 1u << 24;
}
static void op_draw_top(void) {
    scr = hud; char t[48], b[24];
    if (op_full & 2) { backdrop(); memset(op_sg + 10, 0, sizeof(u32) * 12); op_full &= ~2; ui_hints(179, "A:Change|B:Back|L R:Page|SEL:Reset"); }
    { u32 sg = 0x80000000u | (u32)o_tab << 8 | (u32)op_sel << 2 | (op_n > 0); if (op_sg[10] != sg) { op_sg[10] = sg;
        rect(4, 4, 248, 22, PANB); rect(6, 6, 244, 18, PANH); sprintf(t, "OPTIONS  >  %s", OPN[o_tab]); text_box(8, 6, 190, 18, FS_HEAD, TXT, AL_L, t);
        if (o_tab != 7 && op_n > 0) { sprintf(t, "%d / %d", op_sel + 1, op_n); text_box(196, 6, 52, 18, FS_BODY, C15(24, 28, 31), AL_R, t); } } }
    { int q = op_sel; u32 sg = 0x80000000u | (u32)o_tab << 8 | (u32)q | (u32)(o_tab != 7 && q < op_n && op_ix[q] >= 0 ? *OPT[op_ix[q]].v + 1 : 0) << 16; if (op_sg[11] != sg) { op_sg[11] = sg;
        rect(4, 30, 248, 72, PANB); rect(6, 32, 244, 68, PAN);
        if (o_tab == 7) { text(12, 38, FS_HEAD, TXT, "About"); ui_wrap(12, 58, 232, 13, 3, FS_BODY, C15(25, 28, 31), "Pick a page on the left. Each page has rows. Tap a row or use the arrows to change it."); }
        else if (q < op_n) { text(12, 37, FS_HEAD, TXT, op_rname(q, b));
            if (op_ix[q] >= 0) { char vb[16], db[16]; const Opt *o = &OPT[op_ix[q]]; sprintf(t, "Now: %s", opt_tok(op_ix[q], *o->v, vb)); text(12, 52, FS_BODY, C15(31, 31, 18), t);
                if (*o->v != o->def) { sprintf(t, "Normal: %s", opt_tok(op_ix[q], o->def, db)); text_box(130, 44, 116, 14, FS_SMALL, C15(22, 27, 31), AL_R, t); } }
            ui_wrap(12, 68, 232, 13, 3, FS_BODY, C15(25, 28, 31), op_desc(q)); } } }
    { u32 sg = op_psig(); if (op_sg[12] != sg) { op_sg[12] = sg; rect(4, 106, 248, 52, PANB); rect(6, 108, 244, 48, C15(5, 11, 22));   // the preview box: what the page is doing, shown live
        if (o_tab == 1) { static const u16 SKY[4] = { C15(14, 20, 31), C15(31, 18, 12), C15(3, 4, 10), C15(14, 28, 22) }; static const char *const SKN[4] = { "Day", "Sunset", "Night", "Mint" }, *const LKN[3] = { "Classic", "Rich", "Toon" };
            int sn = sky_now(); rect(8, 110, 110, 44, SKY[sn]); rect(8, 140, 110, 14, C15(10, 22, 8));
            if (o_out || o_look == 2) { static const u16 OC[4] = { C15(2, 2, 5), C15(0, 0, 0), C15(31, 31, 31), C15(31, 12, 20) }; rect(49, 113, 36, 36, o_out ? OC[o_ocol & 3] : C15(3, 3, 6)); }
            rect(51, 115, 32, 32, C15(14, 22, 28)); face(51, 115, 32, &sims[0]);
            sprintf(t, "Sky: %s", SKN[sn & 3]); text(124, 114, FS_BODY, TXT, t); sprintf(t, "Look: %s", LKN[o_look % 3]); text(124, 128, FS_BODY, TXT, t);
            sprintf(t, "Light: %+d", o_bri - 4); text(124, 142, FS_BODY, TXT, t); }
        else if (o_tab == 2) { static const char *const nm[3] = { "Master", "Music", "Effects" }; const int *vv[3] = { &o_vol, &o_mus, &o_sfx };
            for (int k = 0; k < 3; k++) { text(12, 114 + k * 14, FS_BODY, TXT, nm[k]); for (int j = 0; j < 10; j++) rect(80 + j * 16, 112 + k * 14, 13, 10, j < *vv[k] ? (k == 0 ? C15(10, 26, 10) : k == 1 ? C15(10, 20, 31) : C15(31, 24, 8)) : C15(3, 6, 14)); } }
        else if (o_tab == 4) { static const char *const DL[4] = { "Short", "Normal", "Long", "Epic" }; clk(t, minutes % 1440); text_c(60, 132, FS_NUM, C15(31, 31, 20), t);
            sprintf(t, "Day: %s", DL[o_day & 3]); text(120, 116, FS_BODY, TXT, t); sprintf(t, "Speed: %dx", o_spd + 1); text(120, 134, FS_BODY, TXT, t); }
        else if (o_tab == 7) { text(12, 114, FS_BODY, TXT, "BORE 2 on DS"); text(12, 132, FS_BODY, C15(25, 28, 31), "Made for touch and buttons."); }
        else { int tot, n = op_changed(&tot); sprintf(t, "%d of %d changed from normal", n, tot); text(12, 114, FS_BODY, TXT, t); text(12, 132, FS_BODY, C15(25, 28, 31), n ? "Green values are changed." : "All rows are at normal."); } } }
    { u32 sg = msg_t > 0 ? (u32)(uintptr_t)msg | 1u : 0; if (op_sg[13] != sg) { op_sg[13] = sg; backdrop_y(159, 178); if (msg_t > 0) toast(128, 162, msg); } }
}
#ifdef DSTEST
volatile int dbg_op[2];   // dbg_ui: ticks of the last draw_hud, draw_top_status, draw_ui (+ off_show)   // ticks (33.5 MHz) of the last Options draw: bottom screen, top screen
static void op_draw(void) { u32 a = prof_now(); op_draw_sub(); u32 b = prof_now(); op_draw_top(); u32 c = prof_now(); dbg_op[0] = (int)(b - a); dbg_op[1] = (int)(c - b); op_rd = 0; }
#else
static void op_draw(void) { op_draw_sub(); op_draw_top(); op_rd = 0; }
#endif
#include "logo_rects.h"
static void draw_logo(int x0, int y0, int num, int den) {   // the BORE 2 logo, scaled num/den
    for (int i = 0; i < LOGO_N; i++) { int x = x0 + LOGO_R[i].x * num / den, y = y0 + LOGO_R[i].y * num / den;
        int w = x0 + (LOGO_R[i].x + LOGO_R[i].w) * num / den - x, h = y0 + (LOGO_R[i].y + LOGO_R[i].h) * num / den - y; if (w < 1) w = 1; if (h < 1) h = 1;
        rect(x, y, w, h, LOGO_P[LOGO_R[i].c] | BIT(15)); }
}
static void draw_title(void) {
    scr = hud; backdrop(); draw_logo(78, 3, 2, 3); panel(8, 64, 240, 120, "Household Occupants");
    const Sim *m = &sims[0]; rect(110, 86, 36, 36, C15(31, 31, 31)); face(112, 88, 32, m);
    const char *nm = sname[0]; char t[40]; char fnm[20], lnm[20]; { const char *sp = strchr(sname[0], ' '); if (sp) { snprintf(fnm, sizeof fnm, "%.*s", (int)(sp - sname[0]), sname[0]); snprintf(lnm, sizeof lnm, "%s", sp + 1); } else { snprintf(fnm, sizeof fnm, "%s", sname[0]); lnm[0] = 0; } }
    text_c(128, 129, FS_HEAD, TXT, fnm); text_c(128, 140, FS_BODY, C15(24, 28, 31), lnm);   // (D65) first name above last name, both centred under the face
    house_icon(34, 146, 20, C15(30, 18, 12)); sprintf(t, "%d", funds); text_c(44, 172, FS_NUM, TXT, t);
    rect(112, 146, 20, 18, C15(31, 31, 31)); rect(114, 148, 16, 3, C15(28, 8, 8)); text_c(122, 172, FS_BODY, TXT, "1 day");
    rect(194, 146, 20, 20, C15(31, 31, 31)); rect(196, 148, 16, 16, C15(22, 26, 31)); rect(204, 150, 1, 7, C15(2, 4, 12)); rect(204, 157, 5, 1, C15(2, 4, 12));
    { clk(t, minutes % 1440); text_c(204, 172, FS_NUM, TXT, t); } (void)nm;
    scr = scr_sub; rect(0, 0, 256, 192, C15(3, 3, 4));
    rect(16, 40, 224, 118, PANB); rect(18, 42, 220, 114, C15(8, 16, 28)); rect(18, 42, 220, 16, PANH); text_box(20, 42, 216, 16, FS_HEAD, TXT, AL_L, "Start/Load profile");
    for (int i = 0; i < 3; i++) { int x = 26 + i * 70, y = 66; rect(x - 2, y - 2, 68, 60, slot_sel == i ? C15(31, 31, 31) : C15(2, 4, 12)); rect(x, y, 64, 56, i == 0 ? C15(10, 24, 8) : C15(6, 12, 26));
        if (i == 0) house_icon(x + 12, y + 4, 40, C15(31, 31, 31)); else plumb_icon(x + 8, y + 0, 48, C15(28, 31, 31));
        { const char *lb = i == 0 ? (sav_have >= 0 ? "Continue" : "New game") : "New sim"; rect(x, y + 42, 64, 14, C15(2, 4, 12)); text_box(x, y + 42, 64, 14, FS_BODY, TXT, AL_C, lb); } }
    chip(98, 130, 28, 22, "<", 0); chip(130, 130, 28, 22, "Opt", 0); chip(162, 130, 28, 22, "Del", 0); chip(194, 130, 36, 22, "OK", 0);
    { static char sb[40]; if (sav_have >= 0) sprintf(sb, "Saved: day %d, %d born, %d lost", st_days, st_born, st_died); const char *st = sav_ok() ? (sav_have >= 0 ? sb : "No saved game yet") : "No save chip found"; if (msg_t <= 0) text_box(16, 158, 224, 20, FS_BODY, C15(22, 26, 31), AL_C, st); }
    ui_hints(179, "Pad:Choose|A:OK|Y:Options");
    if (msg_t > 0) toast(128, 166, msg);
}
#include "ds_games.c"   // (D44) the PS1's mini-games
static void new_household(void) {
    zone = 0; isl_wood = isl_stone = isl_leaves = isl_spear = isl_shells = 0; isl_food = 3; isl_secrets = 0; farm_reset();
    st_days = st_born = st_died = 0; nown = 0; home_bx = home_by = -1; funds = 3500; life_reset(); rt_jump = 1; mg_reset();
    lot_seed = (u32)minutes * 7919u + 17; lot_reset(); lot_bx = -1; lot_by = -1; Sim *m = &sims[0]; m->skin = (u8)(minutes % 6); m->hair = (u8)(minutes % NHAIRD); m->hcol = (u8)(minutes % 6); m->shirt = (u8)(minutes % 6); m->beard = BEARDV; m->fc = 0;
    for (int q = 0; q < PMN; q++) m->pm[q] = 50; m->cc[0] = m->cc[1] = m->cc[2] = 0; memset(csx, 0, sizeof csx);
}
static void title_touch(int tx, int ty) {
    for (int i = 0; i < 3; i++) if (hit(26 + i * 70, 66, 64, 56, tx, ty)) slot_sel = i;
    if (hit(98, 130, 28, 22, tx, ty)) slot_sel = 0;
    if (hit(130, 130, 28, 22, tx, ty)) opt_enter();
    if (hit(162, 130, 28, 22, tx, ty)) { if (slot_sel == 0 && sav_have >= 0) { u8 z[sizeof(SavHdr)]; memset(z, 0, sizeof z); for (int k = 0; k < 2; k++) { cardWriteEeprom((u32)(k * SAV_STRIDE), z, sizeof z, sav_type); memset(sav_sh[k], 0, sizeof z); } sav_have = -1; sav_seq = 0; } new_household(); say("Cleared"); slot_sel = 0; }
    if (hit(194, 130, 36, 22, tx, ty)) { if (slot_sel == 0) { if (sav_have >= 0 && sav_apply()) { say("Welcome back!"); } else if (sav_have >= 0) { say("Save is damaged"); new_household(); }
            if (lot_bx >= 2000) { city_spawn(); nb_ok = 1; set_mode(M_LIVE); } else set_mode(M_MAP); }   // (D37: saved on a trip, you are back there; the Map has no zones)
        else { new_household(); cas_enter(); slot_sel = 0; set_mode(M_CAS); } }
}
static void open_opts(void) { opt_ret = mode; set_mode(M_TITLE); opt_enter(); }
static void set_mode(int m) {
    if ((m == M_BUY || m == M_BUILD) && lot_bx >= 0 && !lot_owned(lot_bx, lot_by)) { say("Not your lot: buy it on the Map"); return; }
    if ((m == M_BUY || m == M_BUILD) && mode == M_LIVE) ed_lv = slv[ctl];   // (D36) Buy and Build start on your sim's storey
    if (m != M_LIVE && on_board) skate_off();
    if (m != M_LIVE) mg_ui = 0;
    mode = m;
    if (m == M_LIVE) { lcdMainOnBottom(); } else { lcdMainOnTop(); scr = hud; hud_clear(); }
}
static void menu_do(int k) {   // (D70) the Live menu's choice k (touch or A): one place, so both do the same
        if (k == 11) { if (stage_of(sims[ctl].age) < 3) say("Teens and up"); else hs_open(); return; }   // (D47: the underworld)
        if (k == 10) { gm_open(); return; }   // (D48: the Games menu: the skateboard (D44) and the Boreagotchi)
        if (k == 9) { travel_ui = 1; return; }
        if (k == 0) { set_mode(M_BUY); pick = -1; } else if (k == 1) set_mode(M_BUILD); else if (k == 2) set_mode(M_MAP);
        else if (k == 3) { if (!sav_ok()) say("No save chip found"); else say(sav_write() ? "Game saved" : "Save failed"); }
        else if (k == 4) open_opts();
        else if (k == 5) { if (sav_ok() && lot_bx >= 0) sav_write(); set_mode(M_TITLE); sav_probe(); }
        else if (k == 6) jobui = 1; else if (k == 7) juke = 1; else if (k == 8) { info_pg ^= 1; life_ver++; }
}
static void live_touch(int tx, int ty) {
    if (mg_ui) return;   // (D46: the mini-game reads the stylus itself)
    if (jobui) { job_touch(tx, ty); return; }
    if (travel_ui) { travel_touch(tx, ty); return; }
    if (craft_ui) { craft_touch(tx, ty); return; }
    if (farm_ui) { farm_touch(tx, ty); return; }
    if (juke) { juke_touch(tx, ty); return; }
    if (popup == 4) {
        if (hit(64, 118, 60, 20, tx, ty)) { Sim *m = &sims[ctl]; m->need[1] = cl100(m->need[1] + (dog.hun > 20 ? 25 : 8)); m->need[3] = cl100(m->need[3] + 8); say("Rex wags his tail. Good boy!"); life_ev(ctl, WN_PET, 0); life_ev(ctl, WN_FUN, 0); popup = 0; }
        else if (hit(132, 118, 60, 20, tx, ty)) { Sim *m = &sims[ctl]; m->need[1] = cl100(m->need[1] + 35); m->need[2] = cl100(m->need[2] - 8); say("Playing fetch"); life_ev(ctl, WN_PET, 0); life_ev(ctl, WN_FUN, 0); popup = 0; }
        else if (hit(64, 142, 60, 20, tx, ty)) { if (funds >= 5) { funds -= 5; dog.hun = 100; say("Rex is fed. Yum!"); } else say("Your wallet says no!"); popup = 0; }
        else if (hit(132, 142, 60, 20, tx, ty)) popup = 0;
        return; }
    if (popup) {
        if (popup == 1) { int top = 150 - popup_n * 24, rows = (popup_n + 1) / 2, top2 = 156 - rows * 24;
            for (int k = 0; k < popup_n; k++) if (popup_n <= 3 ? hit(42, top + 26 + k * 24, 116, 22, tx, ty) : hit(9 + (k & 1) * 121, top2 + 26 + (k >> 1) * 24, 117, 22, tx, ty)) { int cd = popup_code[k]; Sim *m = &sims[ctl]; const Obj *ob = &objs[popup_i]; popup = 0;
                if (cd < 0) start_act(m, popup_i, -1);
                else if (cd >= 100) farm_do(cd, ctl, popup_i);   // (D38) the farm
                else if (cd < 81) { m->tx = ob->x * 16; m->ty = ob->y * 16; m->act = cd; m->act_amt = 0; m->face = cd < 80 ? gather_len(cd - 60) : 1; m->act_obj = popup_i; m->act_left = 0; }   // (gather, or the castaway's meal: walk there, then do it)
                else if (cd == 81) craft_ui = 1; else if (cd == 82) raft_sail(); else if (cd == 83) travel_ui = 1;
                else if (cd == 86) { int g_ = arcade_game_at(ob->x, ob->y), pr_ = ag_price(g_); if (funds < pr_) say("Your wallet says no!"); else { funds -= pr_; ag_open(g_); } }   // (D46)
                else if (cd == 87) ag_open(99);
                else if (cd >= 90 && cd <= 96) gt_furn(cd);   // (D53) the Boreagotchi things
                else if (cd == 85) { objs[popup_i] = objs[--nobj]; occ_ok = 0; has_board = 1; say("A skateboard! Menu > Board rides it"); skate_on(); }   // (D44: the PS1's PICKBOARD)
                return; }
            if (popup_n <= 3 ? hit(162, top + 2 + popup_n * 24, 52, 22, tx, ty) : hit(196, top2 + 3, 50, 18, tx, ty)) popup = 0; return; }
        if (popup == 5) { if (hit(42, 126, 116, 24, tx, ty)) { Sim *m = &sims[ctl]; const Obj *st = &objs[popup_i]; int r = st->rot & 3; m->tx = (st->x - SFX_[r]) * 16; m->ty = (st->y - SFY_[r]) * 16; m->act = 51; m->act_amt = 0; m->face = 1; m->act_obj = popup_i; m->act_left = 0; popup = 0; }   // (down: to the landing)
                          else if (hit(162, 126, 52, 24, tx, ty)) popup = 0; return; }
        { int tg = popup == 2 ? popup_i : -2 - popup_i, n = soc_list(tg); if (hit(212, 60, 22, 14, tx, ty)) popup = 0;
          else for (int k = 0; k < n; k++) if (hit(26 + (k & 1) * 104, 82 + (k >> 1) * 22, 100, 20, tx, ty)) { start_soc(ctl, tg, soc_pick[k]); popup = 0; break; } }
        return; }
    if (menu_open) { menu_open = 0; int k = -1; for (int i = 0; i < 12; i++) if (hit(14 + (i % 4) * 58, 60 + (i / 4) * 36, 54, 30, tx, ty)) k = i;
        ui_snd(k >= 0 ? UI_OK : UI_BACK); menu_do(k); return; }
    if (hit(2, 2, 26, 26, tx, ty)) { set_mode(M_MAP); return; }
    if (hit(104, 178, 48, 14, tx, ty)) { menu_open = 1; return; }
    if (on_board) { if (hit(196, 168, 58, 22, tx, ty)) { skate_off(); say("Off the board"); } return; }   // (D44: riding, the lot takes no taps)
    if (hit(62, 168, 30, 22, tx, ty)) { cam_on ^= 1; return; }
    if (hit(36, 168, 24, 22, tx, ty)) { speed = speed == 1 ? 2 : speed == 2 ? 3 : 1; return; }
    if (hit(0, 158, 34, 34, tx, ty)) { cur_sim = ctl; return; }
    for (int i = 0; i < nsim; i++) if (hit(132 + i * 21, 2, 20, 27, tx, ty)) { cur_sim = i; if (stage_of(sims[i].age) > 0) ctl = i; return; }
    int vl = slv[ctl], cx, cy; LV = vl; if (!pick_cell_h(tx, ty, 40 + vl * WHF, &cx, &cy)) return;
    mark_x = cx; mark_y = cy; mark_t = 40;
    for (int i = 0; i < nsim; i++) { int sx = fdiv(sims[i].x + 8, 16), sy = fdiv(sims[i].y + 8, 16); if (slv[i] == vl && sx == cx && sy == cy) { cur_sim = i; if (i != ctl) { popup = 2; popup_i = i; } return; } }
    if (!vl && dog_have && fdiv(dog.x + 8, 16) == cx && fdiv(dog.y + 8, 16) == cy) { popup = 4; return; }
    for (int k = 0; k < CITN && !vl; k++) { const Cit *c = &cits[k]; if (!cit_vis(c)) continue; int sx = fdiv(c->x + 8, 16), sy = fdiv(c->y + 8, 16); if (sx == cx && sy == cy) { popup = 3; popup_i = k; return; } }
    int o = pick_obj(tx, ty); if (o < 0) o = obj_at(cx, cy); if (o >= 0) { popup = 1; popup_i = o; popup_n = obj_acts(o, popup_lab, popup_code); return; }
    if (vl && cx >= 0 && cy >= 0 && cx < GN && cy < GN && !uflr[vl - 1][cy][cx]) { int st = stairs_below(vl, cx, cy); if (st >= 0) { popup = 5; popup_i = st; } return; }   // (D36) the top of a flight: Go Down; open air elsewhere
    if (cx >= -SLC && cy >= -SLC && cx < SLC * 2 && cy < SLC * 2) { cur_sim = ctl; sims[ctl].tx = cx * 16; sims[ctl].ty = cy * 16; sims[ctl].act = -1; sims[ctl].act_left = 0; }
}

void systemMsgHandler(int bytes, void *user_data);
static volatile u32 lat_down; static volatile int lat_tx, lat_ty, lat_press, lat_got; static u32 lat_prev; static volatile u32 vbl_n;
#ifdef DSTEST
// (D70) responsiveness meter, test builds only (tools/dsemu/uibench.py): every fresh press seen at a vblank (ui_seen), the ones the main loop acted on (ui_proc),
// the vblanks from the press to the end of the pass that answered it (ui_lat_*), and how many vblanks each pass took (ui_stp[1..4]: 1 = a smooth 60, 2+ = a hitch)
#define UI_MASK (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_L | KEY_R | KEY_START | KEY_SELECT | KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_TOUCH)
volatile int ui_seen, ui_proc, ui_lat_sum, ui_lat_max, ui_lat_n, ui_stp[5]; static volatile u32 ui_t0; static u32 ui_cur;
#endif
static void vb_latch(void) {   // every vblank, remember fresh presses, so a quick tap is never lost while a slow frame is being drawn
    vbl_n++; sky_vbl(); gf_kick(vbl_n); u32 k = keysCurrent(), n = k & ~lat_prev; lat_prev = k;
#ifdef DSTEST
    if (n & UI_MASK) { if (!ui_t0) ui_t0 = vbl_n; ui_seen += __builtin_popcount(n & UI_MASK); }
#endif
    lat_down |= n & ~(u32)KEY_TOUCH;
    if (n & KEY_TOUCH) { lat_press = 1; lat_got = 0; }
    if ((k & KEY_TOUCH) && lat_press && !lat_got) { touchPosition t; touchRead(&t); if (t.px || t.py) { lat_tx = t.px; lat_ty = t.py; lat_got = 1; } }   // (D35: was touchReadXY(&t), which in libnds 1.8.2 takes no argument and returns the struct: t stayed garbage, so a tap shorter than one pass of the main loop landed at a random place)
}

#ifdef DS_ROAD_AUDIT
// A test build (XTRA=-DDS_ROAD_AUDIT): when the first lot goes live, every lot of every town and city is made and its things checked against the plot
// (13 x 13 cells; the street and its pavement are outside it, in draw_world). ds_chk: 0 lots, 1 things, 2 things off the plot, 3 done.
int ds_chk[4];
static void ds_road_audit(void) {
    for (int i = 0; i < bm_nset(); i++) { const Settle *st = bm_set(i); int r = st->r + 1;
        for (int by = st->y - r; by <= st->y + r; by++) for (int bx = st->x - r; bx <= st->x + r; bx++) {
            if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N) continue;
            lot_enter(bx, by); ds_chk[0]++; ds_chk[1] += nobj;
            for (int k = 0; k < nobj; k++) if (objs[k].x >= GN || objs[k].y >= GN) ds_chk[2]++; } }
    ds_chk[3] = 1;
}
#endif
int main(void) {
    irqEnable(IRQ_VBLANK); irqSet(IRQ_VBLANK, vb_latch); powerOn(POWER_ALL_2D | POWER_3D_CORE | POWER_MATRIX);
    videoSetMode(MODE_5_3D | DISPLAY_BG3_ACTIVE);
    videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    vramSetBankA(VRAM_A_TEXTURE); vramSetBankB(VRAM_B_TEXTURE); vramSetBankC(VRAM_C_SUB_BG); vramSetBankD(VRAM_D_MAIN_BG_0x06000000);
    int bg = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0); scr_sub = scr = bgGetGfxPtr(bg);
    int bgm = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0); hud = bgGetGfxPtr(bgm); bgSetPriority(bgm, 0);
    if (isDSiMode()) setCpuClock(true); cpu_probe();   // (D40) is the CPU the DSi's fast one?
    glInit(); glEnable(GL_TEXTURE_2D);
    vramSetBankA(VRAM_A_LCD); for (int k = 0; k < NDSTEX; k++) for (int i = 0; i < 64; i++) VRAM_A[k * 64 + i] = DSTEX[k][i]; vramSetBankA(VRAM_A_TEXTURE);   // (D43) the textures, 128 bytes each at the start of bank A
    glEnable(GL_ANTIALIAS); glClearColor(14, 20, 31, 31); glClearPolyID(63); glClearDepth(0x7FFF); glViewport(0, 0, 255, 191); REG_BG0CNT = BG_PRIORITY(1);
    snd_init(); tabs_init(); for (int c = 0; c < 256; c++) if (PS1ID[c]) ID2CH[PS1ID[c] - 1] = (u8)c; bm_dials_default(); bm_new(seed); pop_build();
    cur_set = bm_capital(); town_pick(cur_set);
    lot_reset(); funds = 3500; sims[0].skin = 2; sims[0].hair = 4; sims[0].hcol = 0; sims[0].shirt = 0; mode = M_TITLE; sav_probe(); lcdMainOnTop(); hud_clear();
    int yaw = 0, dist = 13000, frames = 0, dirty = 1; unsigned lastsig = 0; int lastmode = -1;
#ifdef DSTEST
    TIMER_DATA(0) = 0; TIMER_DATA(1) = 0; TIMER_CR(1) = TIMER_ENABLE | TIMER_CASCADE; TIMER_CR(0) = TIMER_ENABLE | TIMER_DIV_1;
#endif
    while (1) {
        PROF(5);
#ifdef DS_ROAD_AUDIT
        { static int done; if (!done && mode == M_LIVE) { done = 1; ds_road_audit(); } }
#endif
        int steps; { static u32 lastv; u32 v = vbl_n; steps = (int)(v - lastv); lastv = v; if (steps < 1) steps = 1; if (steps > 4) steps = 4; }   // (D39) game steps this pass: one a vblank
        ui_heavy = 0; scanKeys(); int held = keysHeld(), down = keysDown(); touchPosition tp = {0}; if (held & KEY_TOUCH) touchRead(&tp);
        { u32 ld = lat_down; lat_down = 0; down |= (int)ld; if (down & KEY_TOUCH) lat_press = 0; else if (lat_press && lat_got) { lat_press = 0; down |= KEY_TOUCH; tp.px = (u16)lat_tx; tp.py = (u16)lat_ty; } }
#ifdef DSTEST
        { static u32 pt_top, pt_mid, pt_end; extern u32 dbg_pt[3]; pt_top = dbg_pt[0]; pt_mid = dbg_pt[1]; pt_end = dbg_pt[2];
          if (steps >= 3 && dbg_logn < 48) { dbg_log[dbg_logn][0] = (int)vbl_n; dbg_log[dbg_logn][1] = steps; dbg_log[dbg_logn][2] = dbg_ui[0]; dbg_log[dbg_logn][3] = (int)(pt_mid - pt_top); dbg_log[dbg_logn][4] = (int)(pt_end - pt_mid); dbg_log[dbg_logn][5] = dbg_ui[1]; dbg_logn++; }
          dbg_pt[0] = prof_now(); }
        { int pc = __builtin_popcount((u32)down & UI_MASK); if (pc) { ui_proc += pc; ui_cur = ui_t0 ? ui_t0 : vbl_n; ui_t0 = 0; } ui_stp[steps > 4 ? 4 : steps]++; }
#endif
        { static int tb_mode = -1, tb_wait;   // (D34) a new screen takes no touch until the stylus that opened it is lifted: a tap held over the switch to the Map no longer picks the block under it
          if (mode != tb_mode) { tb_mode = mode; tb_wait = 1; }
          if (tb_wait) { if (!(held & KEY_TOUCH) || (down & KEY_TOUCH)) tb_wait = 0; else held &= ~KEY_TOUCH; } }
        const Settle *s = bm_set(cur_set); msg_t = msg_t > 0 ? msg_t - 1 : 0; occ_check();
        unsigned sig = 0; int forced = (mode != lastmode); lastmode = mode;
        if (mode == M_TITLE) {
            if (opt_on) { op_tick(held); if (down & KEY_TOUCH) op_touch(tp.px, tp.py); if (opt_on) op_keys(held, down); }   // (D70) Options: touch and buttons both
            else { if (down & KEY_TOUCH) title_touch(tp.px, tp.py);
                { int r = ui_rep(held, down, vbl_n), ns = slot_sel; if (r & (KEY_LEFT | KEY_UP)) ns = ns > 0 ? ns - 1 : 0; if (r & (KEY_RIGHT | KEY_DOWN)) ns = ns < 2 ? ns + 1 : 2;   // (D70) the title by buttons too: choose, A starts, Y opens Options
                  if (ns != slot_sel) { slot_sel = ns; ui_snd(UI_MOVE); title_rd = 1; }
                  if (down & KEY_Y) { ui_snd(UI_OK); opt_enter(); } else if (down & KEY_A) { ui_snd(UI_OK); title_touch(200, 140); } } }
            if (mode == M_TITLE && opt_on) { if (forced) op_full = 3; if (op_rd || forced) op_draw(); }
            else if (mode == M_TITLE && (forced || (down & KEY_TOUCH) || msg_t > 0 || title_rd)) { title_rd = 0; draw_title(); }
#ifdef DSTEST
            static int sh_done; if ((held & KEY_SELECT) && !sh_done) { sh_done = 1; int sv = test_ex; scr = hud; rect(0, 0, 256, 192, C15(14, 20, 28)); Sim t = sims[0];
                for (int k = 0; k < EX_N; k++) { test_ex = k + 1; face(2 + k * 28, 2, 26, &t); face(2 + k * 28, 32, 16, &t); face(2 + (k % 5) * 50, 54 + (k / 5) * 68, 48, &t); }
                test_ex = sv; }
#endif
            yaw = 0; pitch = 0; dist = 6000;
        } else if (mode == M_CAS) {
            int redraw = forced;   // (D67) Create-a-Sim: touch, the shoulder buttons for Back / Next, the camera sliding to the part being worked on
            if (down & KEY_TOUCH) { cas_touch(tp.px, tp.py); redraw = 1; } else if ((held & KEY_TOUCH) && cas_drag) { cas_drag_at(tp.px, tp.py); redraw = 1; }
            if (!(held & KEY_TOUCH)) cas_drag = 0;
            if (!cas_pop && !cas_kb) { if (down & KEY_R) { if (cas_step < CS_N - 1) cas_goto(cas_step + 1); redraw = 1; } if (down & KEY_L) { if (cas_step) cas_goto(cas_step - 1); redraw = 1; } }
            if (cas_pop == 1 && (down & (KEY_A | KEY_B))) { cas_pop = 0; redraw = 1; }
            if (redraw || cas_drag) { draw_cas(); scr = hud; cas_top(); }
            if (cas_done) { int dn = cas_done; cas_done = 0; if (dn == 2) set_mode(M_TITLE); else { cas_name(); set_mode(M_MAP); } dirty = 1; }
            { int td, ty; cas_cam(&td, &ty); if (cas_step == CS_LOOKS && td > 10000) td += (cas_mult - 1) * 3000; int d1 = td - cas_cd, d2 = ty - cas_cy; cas_cd += d1 / 5 + (d1 > 0) - (d1 < 0); cas_cy += d2 / 5 + (d2 > 0) - (d2 < 0); }
            yaw = sinLerp(frames * 70) / 2; pitch = 0; dist = cas_cd;
        } else if (mode == M_MAP) {
            if (down & (KEY_L | KEY_R)) { cur_set = (cur_set + ((down & KEY_R) ? 1 : bm_nset() - 1)) % bm_nset(); town_pick(cur_set); dirty = 1; }
            if (down & KEY_START) { seed = seed * 1103515245u + 12345 + frames; bm_new((u32)seed); pop_build(); lsv_n = 0; lsv_next = 0; nown = 0; life_reset(); nb_ok = 0; lot_bx = -1; cur_set = bm_capital(); town_pick(cur_set); dirty = 1; }
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
            if (map_enter) { enter = 1; map_enter = 0; }
            if (enter && !ne_on) { BInfo b; bm_block(sel_x, sel_y, &b); if (b.terrain == BT_SEA) say("Cannot build at sea"); else { if (!nown) own_add(sel_x, sel_y); if (home_bx < 0 && lot_owned(sel_x, sel_y)) { home_bx = sel_x; home_by = sel_y; } if (lot_bx != sel_x || lot_by != sel_y) { Sim keep = sims[0]; char kname[20]; memcpy(kname, sname[0], 20); lot_save(); zone = 0; lot_seed = (u32)(sel_x * 73856093 ^ sel_y * 19349663); lot_reset(); sims[0].skin = keep.skin; sims[0].hair = keep.hair; sims[0].hcol = keep.hcol; sims[0].shirt = keep.shirt; sims[0].beard = keep.beard; sims[0].fc = keep.fc; memcpy(sims[0].pm, keep.pm, PMN); memcpy(sims[0].cc, keep.cc, 6); memcpy(sname[0], kname, 20); lot_enter(sel_x, sel_y); place_sims(); city_spawn(); } else if (!nb_ok) { nb_load(); city_spawn(); } nb_ok = 1; cur_sim = 0; set_mode(M_LIVE); forced = 1; lastsig = ~lastsig; } }
            if (forced) dirty = 1;
            if (dirty) { scr = offbuf; draw_map(); off_show(scr_sub); dirty = 0; }
            if (msg_t > 0) toast(128, 146, msg);
            yaw += (o_spin == 0 ? 0 : o_spin == 1 ? 40 : o_spin == 2 ? 80 : 170); pitch = 3200;
        } else if (mode == M_LIVE) {
            LV = slv[ctl];
#ifdef DSTEST
#ifdef AUTO_SCENE
            { static int at_n, fired; if (!fired && ++at_n >= 10 && (held & (KEY_X | KEY_Y | KEY_L | KEY_R | KEY_SELECT))) { fired = 1; down &= ~(KEY_X | KEY_Y | KEY_L | KEY_R | KEY_SELECT); test_scene = AUTO_SCENE +((held & KEY_X) != 0) + 2 * ((held & KEY_Y) != 0) + 4 * ((held & KEY_L) != 0) + 8 * ((held & KEY_R) != 0); } }   // (the keys held then add 1 X, 2 Y, 4 L, 8 R)   // (when the emulator's poke does not reach: a build that opens that scene itself)
#endif
            if (test_scene) { int sc = test_scene; test_scene = 0;   // (test builds only: the emulator runner pokes a scene number here to measure it: 1 island, 2 farm, 3 busy street, 4 Strange Town)
                if (sc == 1) travel_go(Z_ISLAND); else if (sc == 4) travel_go(Z_TOWN);
                else if (sc == 2) { memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; gen_public(LOT_FARM, 1); occ_ok = 0; for (int q = 0; q < nsim; q++) { sims[q].x = sims[q].tx = sims[q].wx = (7 + q) * 16; sims[q].y = sims[q].ty = sims[q].wy = 11 * 16; sims[q].act = -1; } }
                else if (sc == 3) { for (int q = 0; q < nsim; q++) { sims[q].x = sims[q].tx = sims[q].wx = (door_cx + q) * 16; sims[q].y = sims[q].ty = sims[q].wy = 13 * 16; sims[q].act = -1; } live_pitch = 2200; yaw_t = live_yaw = 4096 * 2; live_dist = 14000; }
                else if (sc == 6) { live_pitch = 2600; live_dist = 4200; yaw_t = live_yaw = 4096 + 600; }   // (6: close up on the house, to see the patterns)
                else if (sc == 5) { live_pitch = 1800; live_dist = 16000; yaw_t = live_yaw = 4096 * 2 + 1500; }   // (5: the horizon)
                else if (sc >= 10 && sc < 13) { o_look = sc - 10; o_dirty = 1; }   // (10-12: Classic, Rich, Toon)
                else if (sc >= 20 && sc < 23) { o_detl = sc - 20; o_dirty = 1; }
                else if (sc >= 30 && sc < 33) { o_roof = sc - 30; o_dirty = 1; }
                else if (sc >= 60 && sc < 70) { test_ex = sc == 69 ? 0 : sc - 59; life_ver++; }   // (60-68: every face in that mood, 69: as the game decides; 100: the sheet)
                else if (sc == 71 || sc == 72) mg_test_save(sc);   // (D62: 71 writes a save with scores and family links, 72 loads it)
                else if (sc == 70) { live_pitch = 2000; live_dist = 1800; yaw_t = live_yaw = 4096 + 600; }   // (70: face height camera)
                else if ((sc >= 40 && sc < 60) || (sc >= 73 && sc < 80) || (sc >= 80 && sc < 116)) mg_test(sc); }   // (D45 on: the mini-games' test scenes, ds_games.c)   // (30-32: Roofs Cutaway, Always, Hide)   // (20-22: Detail Auto, DS, DSi)
#endif
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
            if ((down & KEY_B) && (menu_open || popup || jobui || juke || travel_ui || craft_ui || farm_ui)) { menu_open = popup = jobui = juke = travel_ui = craft_ui = farm_ui = 0; down &= ~KEY_B; ui_snd(UI_BACK); }   // (D44: that B is not the board's)
            else if ((down & KEY_START) && !jobui && !juke && !popup && !mg_ui) { menu_open ^= 1; ui_snd(menu_open ? UI_OK : UI_BACK); }            if (juke) { if (down & KEY_DOWN) juke_pg++; if (down & KEY_UP && juke_pg > 0) juke_pg--; if (down & KEY_RIGHT) { int c = mus_cur < 0 ? 0 : mus_cur; o_song = (c + 1) % (mus_n > 0 ? mus_n : 1) + 1; } }
            if (mg_ui) mg_frame(held, down, steps, tp.px, tp.py);   // (D46) a mini-game has the buttons and the stylus
            else if (on_board) { if (!menu_open && !popup && !jobui && !juke && !travel_ui && !craft_ui && !farm_ui) skate_frame(held, down, steps); }   // (D44) riding: the buttons are the board's
            else {
            if (down & KEY_L) yaw_t -= 4096; if (down & KEY_R) yaw_t += 4096;
            if (menu_open && !popup) { int r = ui_rep(held, down, vbl_n), ms = menu_sel;   // (D70) the menu by the d-pad: a 4 x 3 grid, A picks
                if (r & KEY_LEFT) ms = ms % 4 ? ms - 1 : ms + 3; if (r & KEY_RIGHT) ms = ms % 4 < 3 ? ms + 1 : ms - 3; if (r & KEY_UP) ms = ms >= 4 ? ms - 4 : ms + 8; if (r & KEY_DOWN) ms = ms < 8 ? ms + 4 : ms - 8;
                if (ms != menu_sel) { menu_sel = ms; ui_snd(UI_MOVE); } if (down & KEY_A) { menu_open = 0; ui_snd(UI_OK); menu_do(menu_sel); down &= ~KEY_A; } }
            if (!juke && !menu_open && !jobui && !popup && (held & KEY_LEFT)) { yaw_t -= 120; live_yaw = yaw_t; } if (held & KEY_RIGHT) { yaw_t += 120; live_yaw = yaw_t; }
            if (held & KEY_UP) { if (live_pitch < 7600) live_pitch += 50; } if (held & KEY_DOWN) { if (live_pitch > 1800) live_pitch -= 50; } }
            if ((down & KEY_SELECT) && !mg_ui) { cam_free ^= 1; if (!cam_free) { live_yaw = yaw_t = 4096; live_pitch = 4300; } }
#ifdef DSTEST
            if (zone == Z_ISLAND && (down & KEY_SELECT) && (held & KEY_L) && (held & KEY_R)) { cam_free ^= 1; isl_wood += 20; isl_stone += 10; isl_leaves += 20; }   // (test builds only: island stock)
            if (zone != Z_ISLAND && (down & KEY_SELECT) && (held & KEY_L) && (held & KEY_R)) { cam_free ^= 1; int bed = -1; for (int q = 0; q < nobj; q++) if (ID2CH[OBJ_ID(&objs[q])] == 0xDB) bed = q;   // (test builds only: the first press makes this lot a farm, the next ones jump to the next morning)
                if (bed < 0) { memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; gen_public(LOT_FARM, 1); funds += 5000; occ_ok = 0; for (int q = 0; q < nsim; q++) { sims[q].x = sims[q].tx = sims[q].wx = (7 + q) * 16; sims[q].y = sims[q].ty = sims[q].wy = 11 * 16; sims[q].act = -1; } }
                else { minutes += 1440 - minutes % 1440 + 360; day_roll(); } }
            if ((down & KEY_START) && (held & KEY_L) && (held & KEY_R)) { static int fk; int want = 0xDB + (fk++ & 3); menu_open = 0;   // (test builds only: the popup of the next farm thing in turn: crop bed, coop, cow, stand)
                for (int q = 0; q < nobj; q++) if (ID2CH[OBJ_ID(&objs[q])] == want) { popup = 1; popup_i = q; popup_n = obj_acts(q, popup_lab, popup_code); break; } }
#endif
#ifdef DSTEST
            { if (down & KEY_SELECT) test_ex = test_ex % EX_N + 1; if (down & KEY_START) test_ex = 0; if (held & KEY_X) { if (live_dist > 3500) live_dist -= 40; } }   // (test builds: SELECT steps through the moods, START lets the game decide, X zooms right in on the faces)
#endif
            if (!on_board && !mg_ui) { if (held & KEY_X) { if (live_dist > 3500) live_dist -= 80; } if (held & KEY_Y) { if (live_dist < 16000) live_dist += 80; } }
            live_yaw += (yaw_t - live_yaw) / 4; if (yaw_t - live_yaw > -4 && yaw_t - live_yaw < 4) live_yaw = yaw_t;
            if (!mg_ui) for (int t = 0; t < steps; t++) { for (int k = 0; k < speed; k++) { sims_step((frames + t) * speed + k); city_step((frames + t) * speed + k); } lot_cross(); } PROF(0);   // (D39: by the vblanks gone by, so the game keeps its speed whatever the frame rate)
            int tx = sims[cur_sim].x * 2 * T / 16 - GN * T + T, tz = sims[cur_sim].y * 2 * T / 16 - GN * T + T; if (!cam_free) { camx += (tx - camx) / 8; camz += (tz - camz) / 8; }
            yaw = live_yaw; dist = live_dist * (o_cam == 0 ? 85 : o_cam == 1 ? 100 : 125) / 100; cam_deff = dist; pitch = live_pitch;
            { int hd = dist * cosLerp(pitch) >> 12, r = (hd + (o_dd == 0 ? 2600 : o_dd == 1 ? 3800 : 6500)) >> 2; lod_ex = camx - (hd * sinLerp(yaw) >> 12); lod_ez = camz + (hd * cosLerp(yaw) >> 12); lod_r2 = on_board ? 0x3FFFFFFF : r * r; }   // (D51: riding the skate plaza, the lot is all drawn in full, so the lists are not recorded again as the camera follows the rider) (D38) the level of detail goes by the distance from the eye
            { static unsigned ts = 1, hs = 1; unsigned a = (unsigned)((on_board ? skate_sig() * 977 + 5 : 0) + (mg_ui ? mg_sig() * 983 + 3 : 0) + cur_sim * 7 + ctl * 3 + (minutes / 1440) * 13 + (isl_wood + isl_stone * 7 + isl_leaves * 13 + isl_food * 17 + isl_secrets * 23) * 31 * zone + o_pm * 977 + info_pg * 43 + life_ver * 53 + sims[cur_sim].age * 59 + (cur_sim != ctl ? minutes * 7 + funds : 0)); a = a * 31 + (unsigned)sims[cur_sim].rel;
              static unsigned tv; unsigned av = (unsigned)(minutes * 13 + funds + (o_pm ? polys_last / 64 + fps_now * 977 : 0) + mood(&sims[cur_sim]) * 5); for (int n = 0; n < 6; n++) av = av * 31 + (unsigned)sims[cur_sim].need[n];   // (D39: the values alone are redrawn on their own)
              unsigned b = (unsigned)(cam_on * 3 + cam_pan * 5 + cam_free * 7 + cur_sim * 11 + popup * 17 + popup_i * 19 + menu_open * 23 + speed * 29 + (msg_t > 0) * 31 + jobui * 37 + juke * 41 + travel_ui * 83 + craft_ui * 89 + farm_ui * 97 + (popup || farm_ui ? fm_sig() * 107 : 0) + (isl_wood + isl_stone * 7 + isl_leaves * 13 + isl_food * 17 + isl_shells * 19 + isl_secrets * 23 + isl_spear * 29) * 101 + nobj * 103 + juke_pg * 47 + on_board * 113 + (mg_ui ? mg_bsig() * 127 + 1 : 0) + o_song * 59 + mus_cur * 61 + nsim * 71 + dog.hun * 73 + (unsigned)(size_t)msg); for (int q = 0; q < nsim; q++) b = b * 31 + (unsigned)(mood(&sims[q]) / 4);
              if (forced) { ts = ~a; hs = ~b; } if (mode == M_LIVE) { if (a != ts) { DBGT(1, draw_top_status(); off_show(scr_sub)); ts = a; tv = av; ui_heavy = 1; } else if (av != tv) { scr = scr_sub; status_vals(1); tv = av; } if (b != hs) { DBGT(0, draw_hud(); hud_show()); ui_heavy = 1; hs = b; } } }   // (D35: not after a menu chip has just left Live, or the Live icons stayed over Buy's 3D view)
        } else {   // BUY / BUILD: the live lot on the top screen, the catalogue or plan on the bottom (drag to place, paint, draw walls, move)
            LV = ed_lv;
            if (down & KEY_TOUCH) ui_down(tp.px, tp.py); else if (held & KEY_TOUCH) ui_hold(tp.px, tp.py);
            if (keysUp() & KEY_TOUCH) ui_up();
#ifdef DSTEST
            if (mode == M_BUY && (down & KEY_SELECT)) { memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0;   // (test builds only: every item of the tab on a bare plot, two cells apart)
                for (int q = 0; q < tab_n[cat] && q < 49; q++) { int id = tab_id[cat][q]; objs[nobj].x = (u8)((q % 7) * 2); objs[nobj].y = (u8)((q / 7) * 2); objs[nobj].c = (u8)(id >> 2); objs[nobj].i = (u8)(id & 3); objs[nobj].rot = 0; nobj++; } occ_ok = 0; ui_ver++; }
#endif
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
            for (int t = 0; t < steps; t++) sims_step(frames + t);
            if (mode != M_MAP) {
                sig = (unsigned)(mode * 7 + cat * 13 + pick * 17 + bpage * 19 + pcx * 31 + pcy * 37 + prot * 41 + tool * 43 + pal * 47 + funds * 59 + nobj * 61 + (msg_t > 0) * 71 + forced + ui_ver * 79 + drag * 83 + (unsigned)(size_t)msg);
                if (sig != lastsig) { DBGT(2, scr = offbuf; draw_ui(); off_show(scr_sub)); lastsig = sig; } }
            yaw = bb_yaw; pitch = bb_pitch; dist = bb_dist * (o_cam == 0 ? 85 : o_cam == 1 ? 100 : 125) / 100;
        }
        { static int skyh = -1; int hh = (minutes / 60) % 24; if ((o_sky == 4 || o_rt) && hh != skyh) o_dirty = 1; skyh = hh; }
        if (down & KEY_TOUCH) sfx(0);
        snd_tick();
        { static int lm = -9; if (mus_cur != lm) { lm = mus_cur; if (mode == M_LIVE && mus_cur >= 0) { static char nb[48]; sprintf(nb, "Now playing: %.22s", mus_name(mus_cur)); say(nb); } } }
        PROF(1); dbg_pt[1] = prof_now();
        { static int lm = -1; if ((mode == M_TITLE || mode == M_CAS) != lm) { lm = mode == M_TITLE || mode == M_CAS; o_dirty = 1; } }
        if (o_dirty) apply_opts();
        if (sky_on) sky_update(pitch);
        // (D39) the 3D: a steady 30 frames a second (or up to 60, Options > System > Frame rate); the passes in between do the game and the 2D screens, so the touch screen answers at 60
        static u32 next3d; int fight_on = mode == M_LIVE && mg_ui == 6, is_lot = (mode == M_LIVE || mode == M_BUY || mode == M_BUILD) && !fight_on;
        { static int aa = 1; int want = !fight_on; if (want != aa) { aa = want; if (want) glEnable(GL_ANTIALIAS); else glDisable(GL_ANTIALIAS); } }   // (D59: BORTUA FIGHTER's ring has no edge blending: it showed the sky along the mat's seams)
        if (is_lot) {   // (D39) the lot: a new frame is written whenever the last one has been taken; the vblank interrupt sends it on the beat
            gf_on = 1;
            { int want = mode == M_LIVE ? slv[ctl] * WHF : (mode == M_BUY || mode == M_BUILD) ? ed_lv * WHF : 0; cam_y += (want - cam_y) / 6; if (want - cam_y > -8 && want - cam_y < 8) cam_y = want; }   // (D36) the camera rises to the storey
            if (gf_ready < 0 && !ui_heavy) {
                static u32 st_sig = 1, w_sig = 1; static int st_n, st_polys, st_k, w_n, w_polys, w_k; static u8 st_mode = 255, f_st[2], f_w[2];
                int b = gf_fly == 0 ? 1 : 0; GFrm *f = &gfr[b]; f->ns = 0; gf_serial++;
                polys = 0; lit_on = o_lit;
                gx_base = POLY_ALPHA(31) | POLY_CULL_FRONT | POLY_ID(1) | POLY_FOG | (lit_on ? (POLY_FORMAT_LIGHT0 | POLY_FORMAT_LIGHT1) : 0);
                gx_begin(gx_dlb[b], GXD_MAX); gf_rec = f; gf_ss = 0;
                gx_cam(dist, pitch, yaw, mode == M_LIVE ? camx : 0, cam_y, mode == M_LIVE ? camz : 0);
                dpass = 2; draw_build(yaw); int pd = polys; gf_cut(); gf_rec = 0; u32 *db = gxb; int dn = gxn, dh = gxh, dk = gxk; PROF(2);
                static u32 b_sig = 1; static int b_n, b_polys, b_k; static u8 f_b[2];
                int did = 0;   // (D41) at most one still list is recorded again a frame (the others next frame): a turn of the camera never costs a beat
                { u32 bg = still_sig(yaw, 0) ^ (u32)mode; if (bg != b_sig) { did = 1; int k = b_k ^ 1; if (gf_busy && f_b[gf_fly] == k) while (gf_busy) {}   // (D41) the build and the things are recorded apart: turning the camera changes one or the other, rarely both
                    polys = pd; gx_begin(gx_blb[k], GXB_MAX); dpass = 1; draw_build(yaw); b_n = gx_end(); DC_FlushRange(gx_blb[k], (u32)b_n * 4); b_polys = polys - pd; b_sig = bg; b_k = k; st_rec++; } }
                u32 sg = still_sig(yaw, 1) ^ (u32)mode;
                if ((sg != st_sig && !did) || st_mode != mode) { did = 1; int k = st_k ^ 1; if (gf_busy && f_st[gf_fly] == k) while (gf_busy) {}
                    polys = pd + b_polys; gx_begin(gx_slb[k], GXS_MAX); dpass = 4; draw_build(yaw); st_n = gx_end(); DC_FlushRange(gx_slb[k], (u32)st_n * 4); st_polys = polys - pd - b_polys; st_sig = sg; st_mode = (u8)mode; st_k = k; st_rec++; }
                { u32 wg = world_sig(); if (wg != w_sig && (!did || w_n == 0)) { int k = w_k ^ 1; if (gf_busy && f_w[gf_fly] == k) while (gf_busy) {}   // (D41) the world: only when the lot or the view changes
                    polys = 0; gx_begin(gx_wlb[k], GXW_MAX); dpass = 3; draw_build(yaw); w_n = gx_end(); DC_FlushRange(gx_wlb[k], (u32)w_n * 4); w_polys = polys; w_sig = wg; w_k = k; st_rec++; } }
                PROF(3); dpass = 0;
                gxb = db; gxn = dn; gxh = dh; gxk = dk; gf_rec = f; gf_ss = gxn;
                gf_add(gx_blb[b_k], b_n); f_b[b] = (u8)b_k; gf_add(gx_slb[st_k], st_n); f_st[b] = (u8)st_k; gf_add(gx_wlb[w_k], w_n); f_w[b] = (u8)w_k; st_polys += 0;
                gx_c1(0x50, 0); gf_cut(); gf_rec = 0; DC_FlushRange(gx_dlb[b], (u32)gxn * 4); gx_end();
                polys = pd + b_polys + st_polys + w_polys; polys_last = polys; gf_ready = b; PROF(4); }
        } else if ((int)(vbl_n - next3d) >= 0) {   // the title, Create-a-Sim and the Map: straight to the GPU, as before
        gf_stop(); u32 v3 = vbl_n; GFX_TEX_FORMAT = 0; imm_txp = -1;   // (D43: no texture left set by a list)
        polys = 0;
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); if (fight_on) gluPerspectivef32(degreesToAngle(55), 256 * 4096 / 192, 450, 4096 * 4); else gluPerspectivef32(degreesToAngle(40), 256 * 4096 / 192, 4096 * 3 / 4, 4096 * 12);
        glMatrixMode(GL_POSITION); glLoadIdentity();
        if (fight_on) { int fd, fp, fy, fx, fyy, fz; fight_cam(&fd, &fp, &fy, &fx, &fyy, &fz); glTranslatef32(0, 0, -fd); glRotateXi(fp); glRotateYi(fy); glTranslatef32(-fx, -fyy, -fz); }
        else { glTranslatef32(0, 0, -dist); glRotateXi(pitch); glRotateYi(yaw); }
        if (mode == M_CAS) glTranslatef32(0, -cas_cy, 0);
        lit_on = o_lit && mode != M_TITLE && mode != M_MAP;
        gx_base = POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(1) | (lit_on ? (POLY_FORMAT_LIGHT0 | POLY_FORMAT_LIGHT1) : 0);
        glPolyFmt(gx_base);
        if (lit_on) { glMaterialf(GL_SPECULAR, RGB15(0, 0, 0)); glMaterialf(GL_EMISSION, RGB15(0, 0, 0));
            glLight(0, RGB15(27, 25, 21), floattov10(0.45f), floattov10(-0.55f), floattov10(-0.70f)); glLight(1, RGB15(7, 10, 17), floattov10(-0.7f), floattov10(0.1f), floattov10(0.7f)); }
        glBegin(GL_QUADS);
        tx_want = -1; far_lod = 0;   // (D55: no pattern may be left over from the Map's lot: it came through as a big checker on the Create-a-Sim head)
#ifdef HAIRCYC
        if (mode == M_CAS) { sims[0].hair = (u8)((vbl_n / HAIRCYC) % NHAIRD); sims[0].hcol = 1; sims[0].skin = 0; sims[0].cc[1] = 0; sims[0].cc[0] = 0; }   // (test builds: -DHAIRCYC=n steps through every hair style, n frames each)
#endif
#ifdef DSTEST
        if (mode == M_CAS && (down & KEY_SELECT)) test_ex = test_ex % EX_N + 1;
        if (mode == M_CAS && (held & KEY_START)) {   // (test builds: hold START in Create-a-Sim to see Live's own sim model large; SELECT steps the moods)
            Sim *s0 = &sims[0]; s0->x = s0->tx = s0->wx = (GN - 1) * 8; s0->y = s0->ty = s0->wy = (GN - 1) * 8; fq4[0] = 0; s0->act = -1;
            glTranslatef32(0, 650, 0); glScalef32(4 * 4096, 4 * 4096, 4 * 4096); glTranslatef32(0, -713, 0); sim3d(0, 0); }
        else
#endif
        if (mode == M_CAS) draw_head(yaw);
        else if (fight_on) fight_3d();
        else if (mode == M_MAP) { BInfo b; bm_block(sel_x, sel_y, &b); draw_lot(&b, yaw); }
        glEnd();
        polys_last = polys; gf_shown++;
        glFlush(0);
        next3d = v3 + (o_fps30 ? 1 : 2);   // (o_fps30 is "Up to 60" since D39)
        }
        { static u32 v0; static int n0, ui0; if (vbl_n - v0 >= 60) { int dv = (int)(vbl_n - v0); fps_now = (gf_shown - n0) * 60 / dv; ui_fps = (ui_n - ui0) * 60 / dv; v0 = vbl_n; n0 = gf_shown; ui0 = ui_n;   // (D36) the frame rate (D39: 3D frames shown a second; ui_fps: passes of this loop)
#ifdef DSTEST
            for (int k = 0; k < 12; k++) { prof_ms[k] = (int)(prof_t[k] / 33514); prof_t[k] = 0; }
#endif
            if (o_pm && mode != M_LIVE && mode != M_TITLE && mode != M_CAS) { u16 *keep = scr; scr = hud; char t[32]; rect(0, 182, 92, 10, C15(2, 2, 4)); sprintf(t, "%d fps %d quads", fps_now, polys_last / 2); text_box(0, 182, 92, 10, FS_SMALL, TXT, AL_C, t); scr = keep; } } }
#ifdef DSTEST
        if (ui_cur) { int l = (int)(vbl_n - ui_cur); ui_lat_sum += l; ui_lat_n++; if (l > ui_lat_max) ui_lat_max = l; ui_cur = 0; }
#endif
        dbg_pt[2] = prof_now(); ui_n++; gframes = frames; swiWaitForVBlank(); frames += steps;
    }
    return 0;
}
