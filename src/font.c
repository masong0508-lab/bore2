// font.c - BORE's own smooth proportional font (fontdata.h on the GBA), as textures: m for big menu type, l for the title, u (the medium shrunk smooth) for the HUD and menus.
#include "font.h"
#include "atlas.h"
#include "gfx.h"
#include "fonttex.inc"
#include "fonttex_ui.inc"   // (size 3: the menus' and the HUD's type, the medium font shrunk smooth)

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
static TexRef sheet[4];
static s8 idx_of[128];
// (size 0, the old small sheet, nothing draws any more: it is the ui type, so a stray 0 still prints)
static const u8 (*glyph[4])[4] = { font_u_glyph, font_m_glyph, font_l_glyph, font_u_glyph };
static const int line_h[4] = { FONT_U_LINE, FONT_M_LINE, FONT_L_LINE, FONT_U_LINE };
static const int space_w[4] = { FONT_U_SPACE, FONT_M_SPACE, FONT_L_SPACE, FONT_U_SPACE };
static const int top_rows[4] = { FONT_U_TOP, FONT_M_TOP, FONT_L_TOP, FONT_U_TOP };

static u16 grey(int v) { return v ? (u16)(0x8000 | v | (v << 5) | (v << 10)) : 0; }
void font_init(void) {
    // every inked texel marked to blend (the STP bit): so the type can be added (light on dark) or taken away (dark ink on a
    // white card) all over, not only at its soft edges (an unmarked texel is drawn as it is, whatever the blend). The sheets are
    // kept packed (coverage in nibbles, the ui type 16 greys in nibbles) and opened out here, through the atlas's scratch, once.
    static const u8 *const src[2] = { font_m_a, font_l_a }; static const int hh[2] = { FONT_M_H, FONT_L_H };
    for (int i = 0; i < 2; i++) {
        u16 *buf = atlas_scratch();
        for (int k = 0; k < FONT_SHEET_W * hh[i]; k++) {
            int a = (src[i][k >> 1] >> ((k & 1) << 2)) & 15;
            buf[k] = a >= 8 ? 0xffff : grey(a * 31 / 8);
        }
        sheet[i + 1] = atlas_alloc_page(2, FONT_SHEET_W, hh[i]); atlas_upload(&sheet[i + 1], buf);
    }
    { u16 *buf = atlas_scratch(); for (int k = 0; k < FONT_SHEET_W * FONT_U_H; k++) { int g = (font_u_a[k >> 1] >> ((k & 1) << 2)) & 15; buf[k] = grey(g * 31 / 15); }
      sheet[3] = atlas_alloc_page(2, FONT_SHEET_W, FONT_U_H); atlas_upload(&sheet[3], buf); }
    sheet[0] = sheet[3];
    for (int i = 0; i < 128; i++) idx_of[i] = -1;
    for (int i = 0; FONT_CHARS[i]; i++) { int c = (u8)FONT_CHARS[i]; if (c < 128 && idx_of[c] < 0) idx_of[c] = (s8)i; }
    // the additive blend for the soft edges: abr 1 in the tpage
    for (int i = 0; i < 4; i++) sheet[i].tpage |= 1 << 5;
}

int font_line(int size) { return line_h[CLAMP(size, 0, 3)]; }
int font_w(int size, const char *s) {
    DC_BTN(s); size = CLAMP(size, 0, 3); int w = 0;
    for (; *s; s++) { int c = (u8)*s; int i = c < 128 ? idx_of[c] : -1; w += i < 0 ? space_w[size] : glyph[size][i][3]; }
    return w;
}
int font_sub;   // 1: the glyphs are taken away from what is under them (dark ink on a white card); 0: added (light text)
void font_text(int x, int y, int size, const char *s, int r, int g, int b) {
    DC_BTN(s); size = CLAMP(size, 0, 3);
    TexRef tt = sheet[size]; const TexRef *t = &tt; int lh = line_h[size], top = top_rows[size];
    if (font_sub) tt.tpage = (u16)((tt.tpage & ~(3 << 5)) | (2 << 5));   // (subtract: the colour given is what to take away)
    y -= top;                                                 // y is the top of the capitals, like the GBA game
    for (; *s; s++) {
        int c = (u8)*s; int i = c < 128 ? idx_of[c] : -1;
        if (i < 0) { x += space_w[size]; continue; }
        const u8 *gl = glyph[size][i]; int w = gl[2];
        if (w) gfx_sprite_blend(x, y, w, lh, t->tpage, 0, t->u + gl[0], t->v + gl[1], w, lh, r >> 1, g >> 1, b >> 1);
        x += gl[3];
    }
}
void font_debug(void) { gfx_sprite(0, 60, 128, FONT_M_H, sheet[1].tpage & ~(1 << 5), 0, sheet[1].u, sheet[1].v, 128, FONT_M_H, 128, 0); gfx_sprite(140, 60, 128, FONT_U_H, sheet[3].tpage & ~(1 << 5), 0, sheet[3].u, sheet[3].v, 128, FONT_U_H, 128, 0); }
void font_textc(int cx, int y, int size, const char *s, int r, int g, int b) { font_text(cx - font_w(size, s) / 2, y, size, s, r, g, b); }
// the medium type blown up zoom times (big arcade words: "K.O.", "ROUND 1"); x is the left edge, y the capitals' top
int font_w_zoom(int zoom, const char *s) { return font_w(1, s) * zoom; }
void font_text_zoom(int x, int y, int zoom, const char *s, int r, int g, int b) {
    DC_BTN(s); const int size = 1; TexRef tt = sheet[size]; int lh = line_h[size], top = top_rows[size];
    if (font_sub) tt.tpage = (u16)((tt.tpage & ~(3 << 5)) | (2 << 5));
    y -= top * zoom;
    for (; *s; s++) {
        int c = (u8)*s; int i = c < 128 ? idx_of[c] : -1;
        if (i < 0) { x += space_w[size] * zoom; continue; }
        const u8 *gl = glyph[size][i]; int w = gl[2];
        if (w) gfx_sprite_blend(x, y, w * zoom, lh * zoom, tt.tpage, 0, tt.u + gl[0], tt.v + gl[1], w, lh, r >> 1, g >> 1, b >> 1);
        x += gl[3] * zoom;
    }
}
