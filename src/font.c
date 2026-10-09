// font.c - BORE's own smooth proportional font (fontdata.h on the GBA), as textures: s for dense HUD rows, m for menus, l for the title.
#include "font.h"
#include "atlas.h"
#include "gfx.h"
#include "fonttex.inc"

static TexRef sheet[3];
static s8 idx_of[128];
static const u8 (*glyph[3])[4] = { font_s_glyph, font_m_glyph, font_l_glyph };
static const int line_h[3] = { FONT_S_LINE, FONT_M_LINE, FONT_L_LINE };
static const int space_w[3] = { FONT_S_SPACE, FONT_M_SPACE, FONT_L_SPACE };
static const int top_rows[3] = { FONT_S_TOP, FONT_M_TOP, FONT_L_TOP };

void font_init(void) {
    sheet[0] = atlas_alloc_page(2, FONT_SHEET_W, FONT_S_H); atlas_upload(&sheet[0], font_s_px);
    sheet[1] = atlas_alloc_page(2, FONT_SHEET_W, FONT_M_H); atlas_upload(&sheet[1], font_m_px);
    sheet[2] = atlas_alloc_page(2, FONT_SHEET_W, FONT_L_H); atlas_upload(&sheet[2], font_l_px);
    for (int i = 0; i < 128; i++) idx_of[i] = -1;
    for (int i = 0; FONT_CHARS[i]; i++) { int c = (u8)FONT_CHARS[i]; if (c < 128 && idx_of[c] < 0) idx_of[c] = (s8)i; }
    // the additive blend for the soft edges: abr 1 in the tpage
    for (int i = 0; i < 3; i++) sheet[i].tpage |= 1 << 5;
}

int font_line(int size) { return line_h[CLAMP(size, 0, 2)]; }
int font_w(int size, const char *s) {
    size = CLAMP(size, 0, 2); int w = 0;
    for (; *s; s++) { int c = (u8)*s; int i = c < 128 ? idx_of[c] : -1; w += i < 0 ? space_w[size] : glyph[size][i][3]; }
    return w;
}
void font_text(int x, int y, int size, const char *s, int r, int g, int b) {
    size = CLAMP(size, 0, 2);
    const TexRef *t = &sheet[size]; int lh = line_h[size], top = top_rows[size];
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
