// font.c - BORE 2's text styles (docs/FONTS.md). The glyph sheets are the disc file FONTS.BIN (made by tools/make_fonts.py): one block of
// 4-bit grey sheets in the VRAM strip, a few 16-colour palettes (CLUTs) beside it, and the glyph tables here (about 3 KB): the sheets
// themselves take no main memory (the old 15-bit sheets were 44 KB of the program).
#include "font.h"
#include "atlas.h"
#include "gfx.h"
#include "audio.h"

#ifdef BORE_DC
#include <string.h>
// Dreamcast pad: the PS1 button names in the texts become the Dreamcast's (X = A, O = B, SQ / SQUARE = X, TRI / TRIANGLE = Y), whole words only
static int dc_wc(int c) { return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '\'' || c == '$' || c == '%'; }
static const char *dc_btn(const char *s, char *buf) {
    static const struct { const char *from, *to; } m[] = { { "CROSS", "A" }, { "CIRCLE", "B" }, { "SQUARE", "X" }, { "TRIANGLE", "Y" }, { "SQ", "X" }, { "TRI", "Y" }, { "X", "A" }, { "O", "B" } };
    int o = 0, hit = 0;
    for (const char *p = s; *p;) {
        int prev = p > s ? p[-1] : ' ';
        if (dc_wc(*p) && !dc_wc(prev) && prev != '.') {
            int k;
            for (k = 0; k < 8; k++) {
                int l = (int)strlen(m[k].from);
                if (strncmp(p, m[k].from, (size_t)l)) continue;
                int nx = p[l];
                if (dc_wc(nx) || nx == '.' || nx == '-' || nx == ':') continue;
                if (o + 2 >= 600) return s;
                buf[o++] = m[k].to[0]; p += l; hit = 1; break;
            }
            if (k < 8) continue;
        }
        if (o + 2 >= 600) return s;
        buf[o++] = *p++;
    }
    if (!hit) return s;
    buf[o] = 0; return buf;
}
#define DC_BTN(s) char dcb_[600]; s = dc_btn(s, dcb_)
#else
#define DC_BTN(s)
#endif

#define NG 94                                   // '!' .. '~'
typedef struct { u8 line, top, cap, space, col, fb, clut, pad; u8 g[NG][4]; s8 off[NG]; } Fnt;   // g: u, v (in the block), w, advance
static Fnt fnt[FS_N];
static int nfnt, rows_n, ready;
static u16 tp_base[2]; static u16 clut_w[3]; static int blk_v;
int font_look;
int font_sub;   // 1: the glyphs are taken away from what is under them (dark ink on a white card); 0: added (light text)

int font_ready(void) { return ready; }
void font_init(void) {
    u8 *hd = (u8 *)atlas_scratch();             // (the atlas's 32 KB scratch: free at start-up)
    if (!audio_cd_part("FONTS.BIN", 0, 2, hd)) return;
    if (memcmp(hd, "BFNT", 4)) return;
    int nf = hd[6], ncl = hd[7], R = hd[8] | hd[9] << 8;
    if (nf > FS_N || ncl > 3 || R > 250) return;
    const u8 *p = hd + 16;
    for (int i = 0; i < nf; i++) {
        Fnt *f = &fnt[i];
        f->line = p[0]; f->top = p[1]; f->cap = p[2]; f->space = p[3]; f->col = p[4]; f->fb = p[5]; f->clut = p[6]; p += 8;
        memcpy(f->g, p, NG * 4); p += NG * 4;
        memcpy(f->off, p, NG); p += NG;
    }
    u16 cl[3][16]; memcpy(cl, p, ncl * 32);
    TexRef t = atlas_alloc_page(2, 128, R + 1);
    if (!t.tpage) return;
    int bx = 896, by = ((t.tpage & 16) ? 256 : 0) + t.v;
    tp_base[0] = (u16)((t.tpage & 16) | 14 | (1 << 5)); tp_base[1] = (u16)((t.tpage & 16) | 15 | (1 << 5));   // 4-bit pages (mode 0) at x 896 and 960, the additive blend
    blk_v = t.v;
    for (int r = 0; r < R; ) {                  // the sheet rows, 128 at a time (128 texels... 256 bytes a row: 8 rows a sector)
        int n = MIN(128, R - r);
        if (!audio_cd_part("FONTS.BIN", (u32)(2 + r / 8), (n + 7) / 8, atlas_scratch())) return;
        atlas_vram_load(bx, by + r, 128, n, atlas_scratch());
        r += n;
    }
    u16 *row = atlas_scratch(); memset(row, 0, 256);
    for (int i = 0; i < ncl; i++) memcpy(row + i * 16, cl[i], 32);
    atlas_vram_load(bx, by + R, 128, 1, row);
    for (int i = 0; i < ncl; i++) clut_w[i] = (u16)(((bx + i * 16) >> 4) | ((by + R) << 6));
    nfnt = nf; rows_n = R; ready = 1;
}

int font_style_of(int scale) { return scale <= 1 ? (font_look == LOOK_READABLE ? FS_READ : FS_BODY) : scale == 2 ? FS_HEAD : FS_BIG; }
static inline int stl(int s) { return s < 0 || s >= nfnt ? 0 : s; }
int font_line(int style) { return fnt[stl(style)].line; }
int font_cap(int style) { return fnt[stl(style)].cap; }

// ---- case: the game's text is written in capitals; in the friendly look a few words become Title Case, longer text Sentence case ----
static int lstr(const char *list, const char *w, int n) {      // is w (n letters, upper case) a word of the space-separated list?
    for (const char *p = list; *p;) {
        const char *e = p; while (*e && *e != ' ') e++;
        if ((int)(e - p) == n && !memcmp(p, w, (size_t)n)) return 1;
        p = *e ? e + 1 : e;
    }
    return 0;
}
static const char L_SMALL[] = "A AN AND AS AT BUT BY FOR IN OF ON OR THE TO UP IF IS IT ME MY NO SO US WE BE DO GO HE OFF OUT";
static const char L_ACRO[] = "TV DJ OK PM ID XP HP PC CD DS GBA VMU FPS ATM NPC BMX UFO DIY LED USB GPS SUV VIP BBQ WC MC CPU RAM VMS HQ PS2 UD LR LRUD ABC RIP";
static const char L_PROPER[] = "BORE BORES BOREAS SAN DIXIE DIPPINN MISSY BOREBOY BOREAGOTCHI PLAYSTATION DREAMCAST SIMS MAPLE SHORES SUNNY HOLLOW PINE BAY STRANGE TOWN BYTE BARN GADGET GARAGE BLEEP BLOOP ZAP ELECTRONICS MON TUE WED THU FRI SAT SUN MONDAY TUESDAY WEDNESDAY THURSDAY FRIDAY SATURDAY SUNDAY JANUARY FEBRUARY MARCH APRIL JUNE JULY AUGUST SEPTEMBER OCTOBER NOVEMBER DECEMBER DIPPER MAN HI YO OH AH";
int font_case(const char *s, char *out, int n) {
    int len = (int)strlen(s), nw = 0, has_lower = 0, sent_p = 0;
    if (len >= n) return 0;
    for (int i = 0; i < len; i++) {
        int c = (u8)s[i];
        if (c >= 'a' && c <= 'z') { has_lower = 1; break; }
        if (c >= 'A' && c <= 'Z' && (i == 0 || !((s[i - 1] >= 'A' && s[i - 1] <= 'Z') || s[i - 1] == '\''))) nw++;
        if ((c == '.' || c == '?' || c == '!') && s[i + 1] == ' ' && i + 2 < len) sent_p = 1;
    }
    if (has_lower || !nw) return 0;
    int keyed = (s[0] == 'X' || s[0] == 'O') && s[1] == ' ';           // a pad hint ("X PLAY BORTUA FIGHTER"): a command with names in it: Title Case unless it is long
    int sentence = keyed ? (nw >= 7 || sent_p) : (nw >= 4 || sent_p);
    int cap_next = 1, last_digit = 0, changed = 0;
    for (int i = 0; i < len;) {
        int c = (u8)s[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {     // a separator
            out[i] = (char)c;
            if (((c == '.' || c == '?' || c == '!') && (s[i + 1] == ' ' || !s[i + 1]))) cap_next = 1;
            i++; continue;
        }
        int j = i, dig = 0, nlet = 0;                                   // a word: letters and digits, and apostrophes between letters
        while (j < len && (((u8)s[j] >= 'A' && (u8)s[j] <= 'Z') || ((u8)s[j] >= '0' && (u8)s[j] <= '9') || (s[j] == '\'' && j + 1 < len && s[j + 1] >= 'A' && s[j + 1] <= 'Z' && j > i))) { if (s[j] >= '0' && s[j] <= '9') dig = 1; else if (s[j] != '\'') nlet++; j++; }
        int wl = j - i, cap_first = 0;
        if (dig) { memcpy(out + i, s + i, (size_t)wl); last_digit = 1; cap_next = 0; i = j; continue; }
        char up[24]; int un = 0; for (int k = i; k < j && un < 23; k++) up[un++] = s[k]; up[un] = 0;
        int apos = 0; for (int k = 0; k < un; k++) if (up[k] == '\'') apos = k + 1;
        int keep = 0;
        if (nlet == 1 && !apos) { if (up[0] == 'A') cap_first = cap_next; else keep = 1; }          // X, O (the pad's buttons): as they are; "A" is the word
        else if (up[0] == 'I' && (un == 1 || up[1] == '\'')) { cap_first = 1; }                      // I, I'M, I'LL
        else if (nlet == 2 && !apos && ((up[0] == 'A' && up[1] == 'M') || (up[0] == 'P' && up[1] == 'M')) && last_digit) keep = 1;   // 7:45 AM
        else if (nlet <= 4 && lstr(L_ACRO, up, un)) keep = 1;
        else if (lstr(L_PROPER, up, un)) cap_first = 1;
        else if (sentence) cap_first = cap_next;
        else cap_first = cap_next || !lstr(L_SMALL, up, un);
        for (int k = i; k < j; k++) {
            int ch = (u8)s[k];
            if (keep) out[k] = (char)ch;
            else if (k == i && cap_first) out[k] = (char)ch;
            else if (ch >= 'A' && ch <= 'Z') out[k] = (char)(ch + 32);
            else out[k] = (char)ch;
        }
        if (!keep) changed = 1;
        last_digit = 0; cap_next = 0; i = j;
    }
    out[len] = 0;
    return changed;
}
// the text as it will be drawn: the friendly case (not for the digits, the big type or the as-written look)
int font_raw;   // 1: the text exactly as written (the pad's key caps: L1, START)
static const char *shape(int style, const char *s, char *buf, int n) {
    if (font_look == LOOK_CAPS || font_raw || style == FS_NUM || style == FS_BIG) return s;
    return font_case(s, buf, n) ? buf : s;
}
#define SHAPE_N 160
const char *font_shaped(const char *s, char *buf, int n) { return font_look == LOOK_CAPS || font_raw ? s : font_case(s, buf, n) ? buf : s; }   // (a whole text, before it is broken into lines: the case is decided for all of it)

static inline const u8 *glyph_of(const Fnt *f, int c) { return c > 32 && c < 127 ? f->g[c - 33] : 0; }
int font_w(int style, const char *s) {
    DC_BTN(s); style = stl(style); char cb[SHAPE_N]; s = shape(style, s, cb, SHAPE_N);
    const Fnt *f = &fnt[style]; int w = 0;
    for (; *s; s++) {
        int c = (u8)*s; const u8 *g = glyph_of(f, c);
        if (!g) { w += f->space; continue; }
        if (!g[2] && !g[3] && f->fb != 255) { const Fnt *b = &fnt[stl(f->fb)]; const u8 *h = glyph_of(b, c); w += h ? h[3] : b->space; }
        else w += g[3];
    }
    return w;
}
int font_ink_w(int style, const char *s) {                  // advance width less the space after the last glyph
    DC_BTN(s); style = stl(style); char cb[SHAPE_N]; s = shape(style, s, cb, SHAPE_N);
    const Fnt *f = &fnt[style]; int w = font_w(style, s), n = (int)strlen(s);
    while (n > 0 && s[n - 1] == ' ') { w -= f->space; n--; }
    if (n > 0) { const u8 *g = glyph_of(f, (u8)s[n - 1]); if (g && g[2]) { int tail = g[3] - (f->off[(u8)s[n - 1] - 33] + g[2]); if (tail > 0 && tail < 3) w -= tail; } }
    return w;
}
static void draw_glyph(const Fnt *f, const u8 *g, int off, int x, int y, int zoom, int r, int g_, int b, int abr_sub) {
    u16 tp = tp_base[f->col]; if (abr_sub) tp = (u16)((tp & ~(3 << 5)) | (2 << 5));
    int cl = clut_w[font_look == LOOK_READABLE ? 2 : f->clut];
    gfx_sprite_blend(x + off * zoom, y, g[2] * zoom, f->line * zoom, tp, (u16)cl, g[0], blk_v + g[1], g[2], f->line, r >> 1, g_ >> 1, b >> 1);
}
void font_text(int style, int x, int y, const char *s, int r, int g, int b) {
    if (!ready) return;
    DC_BTN(s); style = stl(style); char cb[SHAPE_N]; s = shape(style, s, cb, SHAPE_N);
    const Fnt *f = &fnt[style];
    for (; *s; s++) {
        int c = (u8)*s; const u8 *gl = glyph_of(f, c);
        if (!gl) { x += f->space; continue; }
        if (!gl[2] && !gl[3] && f->fb != 255) {        // a glyph this style does not have: from its fallback style, on the same baseline
            const Fnt *fb = &fnt[stl(f->fb)]; const u8 *h = glyph_of(fb, c);
            if (h) { if (h[2]) draw_glyph(fb, h, fb->off[c - 33], x, y + f->cap - fb->cap - fb->top, 1, r, g, b, font_sub); x += h[3]; }
            else x += fb->space;
            continue;
        }
        if (gl[2]) draw_glyph(f, gl, f->off[c - 33], x, y - f->top, 1, r, g, b, font_sub);
        x += gl[3];
    }
}
void font_textc(int style, int cx, int y, const char *s, int r, int g, int b) { font_text(style, cx - font_ink_w(style, s) / 2, y, s, r, g, b); }
void font_debug(void) { }
// the fun type blown up zoom times (big arcade words: "K.O.", "ROUND 1"); x is the left edge, y the capitals' top
int font_w_zoom(int zoom, const char *s) { return font_w(FS_FUN, s) * zoom; }
void font_text_zoom(int x, int y, int zoom, const char *s, int r, int g, int b) {
    if (!ready) return;
    DC_BTN(s); const Fnt *f = &fnt[FS_FUN];
    y -= f->top * zoom;
    for (; *s; s++) {
        int c = (u8)*s; const u8 *gl = glyph_of(f, c);
        if (!gl) { x += f->space * zoom; continue; }
        if (gl[2]) draw_glyph(f, gl, f->off[c - 33], x, y, zoom, r, g, b, font_sub);
        x += gl[3] * zoom;
    }
}
