// font.c - BORE's own smooth proportional font (fontdata.h on the GBA), as textures: s for dense HUD rows, m for menus, l for the title.
#include "font.h"
#include "atlas.h"
#include "gfx.h"
#include "fonttex.inc"
#include "fonttex_ui.inc"   // (size 3: the menus' and the HUD's type, the medium font shrunk smooth)

static TexRef sheet[4];
static s8 idx_of[128];
static const u8 (*glyph[4])[4] = { font_s_glyph, font_m_glyph, font_l_glyph, font_u_glyph };
static const int line_h[4] = { FONT_S_LINE, FONT_M_LINE, FONT_L_LINE, FONT_U_LINE };
static const int space_w[4] = { FONT_S_SPACE, FONT_M_SPACE, FONT_L_SPACE, FONT_U_SPACE };
static const int top_rows[4] = { FONT_S_TOP, FONT_M_TOP, FONT_L_TOP, FONT_U_TOP };

void font_init(void) {
    // every inked texel marked to blend (the STP bit): so the type can be added (light on dark) or taken away (dark ink on a
    // white card) all over, not only at its soft edges (an unmarked texel is drawn as it is, whatever the blend)
    static const u16 *const src[3] = { font_s_px, font_m_px, font_l_px }; static const int hh[3] = { FONT_S_H, FONT_M_H, FONT_L_H };
    for (int i = 0; i < 3; i++) {
        u16 *buf = atlas_scratch();
        for (int k = 0; k < FONT_SHEET_W * hh[i]; k++) buf[k] = src[i][k] ? (u16)(src[i][k] | 0x8000) : 0;
        sheet[i] = atlas_alloc_page(2, FONT_SHEET_W, hh[i]); atlas_upload(&sheet[i], buf);
    }
    sheet[3] = atlas_alloc_page(2, FONT_SHEET_W, FONT_U_H); atlas_upload(&sheet[3], font_u_px);
    for (int i = 0; i < 128; i++) idx_of[i] = -1;
    for (int i = 0; FONT_CHARS[i]; i++) { int c = (u8)FONT_CHARS[i]; if (c < 128 && idx_of[c] < 0) idx_of[c] = (s8)i; }
    // the additive blend for the soft edges: abr 1 in the tpage
    for (int i = 0; i < 4; i++) sheet[i].tpage |= 1 << 5;
}

int font_line(int size) { return line_h[CLAMP(size, 0, 3)]; }
int font_w(int size, const char *s) {
    size = CLAMP(size, 0, 3); int w = 0;
    for (; *s; s++) { int c = (u8)*s; int i = c < 128 ? idx_of[c] : -1; w += i < 0 ? space_w[size] : glyph[size][i][3]; }
    return w;
}
int font_sub;   // 1: the glyphs are taken away from what is under them (dark ink on a white card); 0: added (light text)
void font_text(int x, int y, int size, const char *s, int r, int g, int b) {
    size = CLAMP(size, 0, 3);
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
void font_debug(void) { gfx_sprite(0, 60, 128, FONT_M_H, sheet[1].tpage & ~(1 << 5), 0, sheet[1].u, sheet[1].v, 128, FONT_M_H, 128, 0); gfx_sprite(140, 60, 128, FONT_S_H, sheet[0].tpage & ~(1 << 5), 0, sheet[0].u, sheet[0].v, 128, FONT_S_H, 128, 0); }
void font_textc(int cx, int y, int size, const char *s, int r, int g, int b) { font_text(cx - font_w(size, s) / 2, y, size, s, r, g, b); }
