// gotchi_first.inc - the BOREAGOTCHI! screens' sky and the first run's pixel-art helpers (included by gotchi_ui.c; its art is gotchi_first_art.h,
// made by tools/make_gotchi_first.py). A sprite is drawn as runs of boxes: scaled in 16ths across and whole pixels down, tilted (a row-by-row slide:
// a wobble), flipped, clipped to a box (the LCD), in the colours of the keys: the toy's shell, the LCD and the egg's ink are set by f_setup.
static u8 fm[96][3];   // the colour of each key (a character from # to z)
static int fsx = 64, fsy = 4, fsemi, fsq, fcx0, fcy0, fcx1 = 320, fcy1 = 240;   // the pixel's size (16ths across, pixels down), see-through, the clip box
static void f_key(int k, int r, int g, int b) { u8 *m = fm[(k & 127) - 32]; m[0] = (u8)r; m[1] = (u8)g; m[2] = (u8)b; }
static void f_eggcols(int ink) { for (const char *e = "#mdhouxX"; *e; e++) { u8 o[3]; egg_rgb(ink % 8, (int)(strchr(eggKeys, *e) - eggKeys), o); f_key(*e, o[0], o[1], o[2]); } }   // the egg's colours from the pet's (the same as on the LCD dots)
static void f_setup(int shell, int ink, int lcd) {   // the owner's colour for the shell, a pet colour for the egg's ink, the LCD on (1) or off (0)
    for (int i = 0; i < FS_PAL_N; i++) f_key(FS_PAL[i][0], FS_PAL[i][1], FS_PAL[i][2], FS_PAL[i][3]);
    const u8 *sh = PAL[1 + shell % 7];
    f_key('S', MIN(255, sh[0] + 40), MIN(255, sh[1] + 40), MIN(255, sh[2] + 40)); f_key('s', sh[0], sh[1], sh[2]); f_key('z', sh[0] * 6 / 10, sh[1] * 6 / 10, sh[2] * 6 / 10);
    if (lcd) { f_key('L', 172, 196, 128); f_key('l', 150, 176, 108); } else { f_key('L', 58, 72, 70); f_key('l', 44, 56, 56); }
    f_eggcols(ink);
}
static void f_clip(int x0, int y0, int x1, int y1) { fcx0 = x0; fcy0 = y0; fcx1 = x1; fcy1 = y1; }
static void f_noclip(void) { f_clip(0, 0, 320, 240); }
static void f_rect(int x, int y, int w, int h, int r, int g, int b, int semi) {
    int x1 = MIN(x + w, fcx1), y1 = MIN(y + h, fcy1); x = MAX(x, fcx0); y = MAX(y, fcy0); if (x1 <= x || y1 <= y) return; gfx_rect(x, y, x1 - x, y1 - y, r, g, b, semi);
}
static void f_spr(const char *const *rows, int w, int h, int x, int y, int tilt, int flip) {
    for (int j = 0; j < h; j++) {
        int yy = y + j * fsy; if (yy + fsy <= fcy0 || yy >= fcy1) continue;
        int sh = h > 1 ? tilt * (h - 1 - j) / (h - 1) : 0; const char *r = rows[j];
        if (fsq) { int u = fsx >> 4; sh = (sh + (sh < 0 ? -u / 2 : u / 2)) / u * u; }   // (fsq: the lean in whole pixels of art, so an egg's edge stays crisp)
        for (int i = 0; i < w;) {
            int c = flip ? r[w - 1 - i] : r[i]; if (c == '.') { i++; continue; }
            int k = i + 1; while (k < w && (flip ? r[w - 1 - k] : r[k]) == c) k++;
            const u8 *m = fm[(c & 127) - 32]; f_rect(x + sh + (i * fsx >> 4), yy, ((k * fsx >> 4) - (i * fsx >> 4)), fsy, m[0], m[1], m[2], fsemi); i = k;
        }
    }
}
#define SPR(n, x, y, t, f) f_spr(FS_##n, FS_##n##_W, FS_##n##_H, x, y, t, f)
static void f_scale(int px) { fsx = px * 16; fsy = px; }
static const char *const *const fsShard[4] = { FS_SHARD0, FS_SHARD1, FS_SHARD2, FS_SHARD3 };
static const u8 fsShardW[4] = { FS_SHARD0_W, FS_SHARD1_W, FS_SHARD2_W, FS_SHARD3_W }, fsShardH[4] = { FS_SHARD0_H, FS_SHARD1_H, FS_SHARD2_H, FS_SHARD3_H };
static void f_add(void) { gfx_ui_blend(1); }   // see-through boxes add light (what is under them can have changed it: say so before each lot)
static void f_spark(int cx, int cy, int size) {   // a twinkle: 0 small, 1 middle, 2 big (centred)
    int sx = fsx, sy = fsy, se = fsemi; f_scale(2); fsemi = 1; f_add();
    if (size == 0) f_spr(FS_SPK_S, 3, 3, cx - 3, cy - 3, 0, 0); else if (size == 1) f_spr(FS_SPK_M, 5, 5, cx - 5, cy - 5, 0, 0); else f_spr(FS_SPK_L, 7, 9, cx - 7, cy - 9, 0, 0);
    fsx = sx; fsy = sy; fsemi = se;
}
static void f_glyph(int kind, int x, int y, int ink, int pat) {   // a little egg for the diary and the tree, a dot a pixel: 0 an egg (pattern pat), 1 a mystery egg, 2 a hatching egg (9 pixels tall at most)
    f_eggcols(ink); f_key('g', 255, 208, 60); fsx = 16; fsy = 1; fsq = 0;   // (the next sprite sets its own size)
    f_spr(egg_make(2, kind == 1 ? FS_EGG_MYS : pat, kind == 2 ? 3 : 0), FS_EGGS_W, FS_EGGS_H, x, y, 0, 0);
}
static void f_disc(int cx, int cy, int r, int cr, int cg, int cb) {   // a chunky circle, added to what is under it
    f_add(); int st = MAX(3, r / 10);
    for (int dy = -r; dy < r; dy += st) { int a = dy + st / 2, hw = isqrt(MAX(0, r * r - a * a)); f_rect(cx - hw, cy + dy, hw * 2, st, cr, cg, cb, 1); }
}
static void f_glow(int cx, int cy, int r, int cr, int cg, int cb) { for (int k = 0; k < 3; k++) f_disc(cx, cy, r - k * r / 4, cr, cg, cb); }
static void f_flash(int v) { if (v > 0) f_add(), gfx_rect(0, 0, 320, 240, MIN(255, v), MIN(255, v), MIN(255, v * 9 / 10), 1); }
// the title lettering: the 5 x 7 face at s pixels, a dark rim and shadow, a gold slope and a shine sliding across (shine: its x, -1 none)
static void f_banner(int cx, int y, int s, const char *str, int shine) {
    static const u8 gr[7][3] = { { 255, 246, 170 }, { 255, 230, 112 }, { 255, 212, 66 }, { 255, 190, 44 }, { 246, 162, 30 }, { 226, 130, 22 }, { 196, 98, 16 } };
    int n = (int)strlen(str), x0 = cx - (n * 6 * s - s) / 2;
    for (int pass = 0; pass < 2; pass++) for (int c = 0; c < n; c++) {
        const char *p = strchr(FS_FONT_CH, str[c] >= 'a' && str[c] <= 'z' ? str[c] - 32 : str[c]); if (!p || !*p) continue; const char *g = FS_FONT[p - FS_FONT_CH]; int gx = x0 + c * 6 * s;
        for (int j = 0; j < 7; j++) for (int i = 0; i < 5;) {
            if (g[j * 5 + i] != '#') { i++; continue; } int k = i; while (k < 5 && g[j * 5 + k] == '#') k++;
            int rx = gx + i * s, rw = (k - i) * s, ry = y + j * s;
            if (pass == 0) gfx_rect(rx - s, ry - s, rw + 3 * s, 3 * s, 34, 10, 56, 0);
            else { int r = gr[j][0], gg = gr[j][1], b = gr[j][2]; if (shine >= 0 && rx + rw > shine && rx < shine + 3 * s) { r = MIN(255, r + 70); gg = MIN(255, gg + 70); b = MIN(255, b + 110); } gfx_rect(rx, ry, rw, s, r, gg, b, 0); }
            i = k; }
    }
}
static int f_banner_w(int s, const char *str) { return (int)strlen(str) * 6 * s - s; }

// ---- the sky: a deep violet gradient, a slow glow, three layers of stars that drift and twinkle, now and then a shooting star ----
static void sky(void) {
    gfx_grad(0, 0, 320, 120, UI_RGB(26, 12, 62), UI_RGB(26, 12, 62), UI_RGB(30, 16, 74), UI_RGB(30, 16, 74), 0);
    gfx_grad(0, 120, 320, 120, UI_RGB(30, 16, 74), UI_RGB(30, 16, 74), UI_RGB(5, 4, 28), UI_RGB(5, 4, 28), 0);
    f_add();
    { int cx = 160 + (isin(anim * 3 & 4095) >> 6), cy = 110 + (icos(anim * 2 & 4095) >> 7); f_disc(cx, cy, 88, 7, 4, 12); f_disc(cx, cy, 56, 6, 4, 10); }
    static const u8 tint[3][3] = { { 214, 204, 255 }, { 255, 240, 180 }, { 170, 232, 255 } };
    for (int i = 0; i < 32; i++) {
        u32 h = hsh(i * 77 + 5); int lay = i < 16 ? 0 : i < 26 ? 1 : 2, sp = lay == 0 ? 14 : lay == 1 ? 9 : 6;
        int x = (int)(h % 320) - (anim / sp) % 320; if (x < 0) x += 320; int y = (int)((h >> 9) % 240) + (anim / (sp * 3)) % 240; if (y >= 240) y -= 240;
        const u8 *c = tint[i % 3]; int br = lay == 0 ? 70 : lay == 1 ? 130 : 200;
        if (lay < 2) { gfx_rect(x, y, 2, 2, c[0] * br >> 8, c[1] * br >> 8, c[2] * br >> 8, 1); continue; }
        int ph = (anim + i * 37) % 84, a = ph < 14 ? 5 - abs(ph - 7) * 5 / 7 : 0;   // a flash every 84 ticks: the cross grows and shrinks
        gfx_rect(x, y, 3, 3, c[0] * br >> 8, c[1] * br >> 8, c[2] * br >> 8, 1);
        if (a > 0) { gfx_rect(x + 1 - a, y + 1, 2 * a + 1, 1, c[0] >> 1, c[1] >> 1, c[2] >> 1, 1); gfx_rect(x + 1, y + 1 - a, 1, 2 * a + 1, c[0] >> 1, c[1] >> 1, c[2] >> 1, 1); }
    }
    { int c = anim % 460; if (c < 26) { u32 h = hsh(anim / 460 + 99); int hx = 180 + (int)(h % 130) - c * 9, hy = 14 + (int)((h >> 8) % 60) + c * 4;   // (a shooting star: a bright head, a fading tail)
        for (int k = 0; k < 7; k++) { int v = 220 - k * 30; if (c > 16) v = v * (26 - c) / 10; if (v > 0) gfx_rect(hx + k * 7, hy - k * 3, k ? 3 : 4, k ? 2 : 4, v, v, v * 9 / 10, 1); } } }
}
