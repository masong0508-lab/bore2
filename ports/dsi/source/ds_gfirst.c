// ds_gfirst.c (D69): the PS1's BOREAGOTCHI! first-run screens on the DS: the title with gold lettering and a shine, the sky of twinkling stars with a shooting star, the shop card, the
// gift box that drops, squashes, shakes, loses its ribbon and pops (confetti, sparkles, beams) while the toy rises out of it and its LCD turns on, the egg card, and the hatching (the egg
// rocks, cracks, flashes, the shell flies and the baby steps out). Art: the PS1's src/gotchi_first_art.h (see ds_gegg.h). The PS1 draws on 320 x 240; here the two screens are ONE canvas,
// 256 x 384 (the top screen over the bottom one): gvo is the canvas row at the top of the screen being drawn (0 or 192), every picture is drawn once for each screen from the same
// numbers, so a beam, a ring or a toy can fly from the box on the touch screen up to the top one. Times are 60 a second (giT, ghT), whatever the frame rate.
#ifdef DSTEST
static volatile u32 gfp[4];   // test builds: cycles (bus clock) of the sky and of the picture, per screen
#endif
static int gvo, gf_sx = 64, gf_sy = 4, gf_semi, gf_sq, gf_cx0, gf_cy0, gf_cx1 = 256, gf_cy1 = 192;   // the canvas row of this screen; the pixel's size (16ths across, pixels down); see-through; lean in whole pixels; the clip box
static u8 gfm[128][3];                                                                                // the colour of each key of the art
static const u8 gShellPal[7][3] = { { 214, 58, 120 }, { 40, 92, 214 }, { 226, 112, 24 }, { 126, 64, 200 }, { 20, 150, 140 }, { 196, 40, 44 }, { 218, 160, 10 } };   // the toy's shell colours (the PS1's PAL rows 1-7)
static u16 gf_c16(const u8 *m) { return C15(m[0] >> 3, m[1] >> 3, m[2] >> 3); }
static void gf_key(int k, int r, int g, int b) { gfm[k & 127][0] = (u8)r; gfm[k & 127][1] = (u8)g; gfm[k & 127][2] = (u8)b; }
static void gf_setup(int shell, int ink, int lcd) {   // the owner's colour for the shell, a pet colour for the egg's ink, the LCD on (1) or off (0)
    for (int i = 0; i < FS_PAL_N; i++) gf_key(FS_PAL[i][0], FS_PAL[i][1], FS_PAL[i][2], FS_PAL[i][3]);
    const u8 *sh = gShellPal[shell % 7];
    gf_key('S', MIN(255, sh[0] + 40), MIN(255, sh[1] + 40), MIN(255, sh[2] + 40)); gf_key('s', sh[0], sh[1], sh[2]); gf_key('z', sh[0] * 6 / 10, sh[1] * 6 / 10, sh[2] * 6 / 10);
    if (lcd) { gf_key('L', 172, 196, 128); gf_key('l', 150, 176, 108); } else { gf_key('L', 58, 72, 70); gf_key('l', 44, 56, 56); }
    for (const char *e = "#mdhu"; *e; e++) { u8 o[3]; gegg_rgb(ink, (int)(strchr(geggKeys, *e) - geggKeys), o); gf_key(*e, o[0], o[1], o[2]); }   // the egg's colours from the pet's (the same as on the LCD dots)
    { u8 o[3]; gegg_rgb(ink, 3, o); gf_key('~', o[0], o[1], o[2]); }   // (the egg's spots are '~' here: 'o' stays the ribbon's dark orange)
}
static void gf_clip(int x0, int y0, int x1, int y1) { gf_cx0 = x0; gf_cy0 = y0; gf_cx1 = x1; gf_cy1 = y1; }
static void gf_noclip(void) { gf_clip(0, gvo, 256, gvo + 192); }
static void gf_fill(int x, int y, int w, int h, u16 c) { rect(x, y - gvo, w, h, c); }
static void gf_round(int x, int y, int w, int h, u16 c) { gf_fill(x + 2, y, w - 4, h, c); gf_fill(x + 1, y + 1, w - 2, h - 2, c); gf_fill(x, y + 2, w, h - 4, c); }   // a box with its corners cut
// light added to what is there (5-bit numbers): saturating for the small bright things, quick and unchecked for the faint glows that only ever lie on the dark sky
static void gf_addsat(int x, int y, int w, int h, int ar, int ag, int ab) {
    y -= gvo; if (x < 0) { w += x; x = 0; } if (y < 0) { h += y; y = 0; } if (x + w > 256) w = 256 - x; if (y + h > 192) h = 192 - y; if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++) { u16 *p = scr + (y + j) * 256 + x;
        for (int i = 0; i < w; i++) { u16 v = p[i]; int r = (v & 31) + ar, g = ((v >> 5) & 31) + ag, b = ((v >> 10) & 31) + ab; p[i] = (u16)((r > 31 ? 31 : r) | ((g > 31 ? 31 : g) << 5) | ((b > 31 ? 31 : b) << 10) | 0x8000); } }
}
static void gf_addq(int x, int y, int w, int h, int ar, int ag, int ab) {
    y -= gvo; if (x < 0) { w += x; x = 0; } if (y < 0) { h += y; y = 0; } if (x + w > 256) w = 256 - x; if (y + h > 192) h = 192 - y; if (w <= 0 || h <= 0) return;
    u16 a = (u16)(ar | (ag << 5) | (ab << 10)); u32 a2 = (u32)a | ((u32)a << 16);
    for (int j = 0; j < h; j++) { u16 *p = scr + (y + j) * 256 + x; int i = 0, n = w; if (x & 1) { p[0] = (u16)(p[0] + a); i = 1; n--; } u32 *q = (u32 *)(p + i); for (int k = 0; k < n / 2; k++) q[k] += a2; if (n & 1) p[i + n - 1] = (u16)(p[i + n - 1] + a); }
}
static void gf_rect(int x, int y, int w, int h, int r, int g, int b, int semi) {   // a box in 8-bit colours inside the clip box
    int x1 = MIN(x + w, gf_cx1), y1 = MIN(y + h, gf_cy1); x = MAX(x, gf_cx0); y = MAX(y, gf_cy0); if (x1 <= x || y1 <= y) return;
    if (semi) gf_addsat(x, y, x1 - x, y1 - y, MIN(31, (r + 4) >> 3), MIN(31, (g + 4) >> 3), MIN(31, (b + 4) >> 3)); else rect(x, y - gvo, x1 - x, y1 - y, C15(r >> 3, g >> 3, b >> 3));
}
static void gf_spr(const char *const *rows, int w, int h, int x, int y, int tilt, int flip) {   // a sprite as runs of boxes: scaled in 16ths across, tilted (a wobble), flipped
    for (int j = 0; j < h; j++) {
        int yy = y + j * gf_sy; if (yy + gf_sy <= gf_cy0 || yy >= gf_cy1) continue;
        int sh = h > 1 ? tilt * (h - 1 - j) / (h - 1) : 0; const char *r = rows[j];
        if (gf_sq) { int u = gf_sx >> 4; sh = (sh + (sh < 0 ? -u / 2 : u / 2)) / u * u; }   // (the lean in whole pixels of art, so an egg's edge stays crisp)
        for (int i = 0; i < w;) {
            int c = flip ? r[w - 1 - i] : r[i]; if (c == '.') { i++; continue; }
            int k = i + 1; while (k < w && (flip ? r[w - 1 - k] : r[k]) == c) k++;
            const u8 *m = gfm[c & 127]; gf_rect(x + sh + (i * gf_sx >> 4), yy, ((k * gf_sx >> 4) - (i * gf_sx >> 4)), gf_sy, m[0], m[1], m[2], gf_semi); i = k;
        }
    }
}
#define GSPR(n, x, y, t, f) gf_spr(FS_##n, FS_##n##_W, FS_##n##_H, x, y, t, f)
static void gf_scale(int px) { gf_sx = px * 16; gf_sy = px; }
static int gwrapn(int x, int y, int w, int lines, u16 c, const char *s);
static const char *const *const gfShards[4] = { FS_SHARD0, FS_SHARD1, FS_SHARD2, FS_SHARD3 };
static const u8 gfShardW[4] = { FS_SHARD0_W, FS_SHARD1_W, FS_SHARD2_W, FS_SHARD3_W }, gfShardH[4] = { FS_SHARD0_H, FS_SHARD1_H, FS_SHARD2_H, FS_SHARD3_H };
static const char *const *gf_egg(int kind, int pat, int crack) {   // an egg sprite with its spots as '~'
    const char *const *r = gegg_make(kind, pat, crack); for (int j = 0; j < gfsEggH[kind]; j++) for (int i = 0; i < gfsEggW[kind]; i++) if (geggBuf[j][i] == 'o') geggBuf[j][i] = '~';
    return r;
}
static int gf_shell(void) { return gtt()->uid % 7; }
static void gf_toy(int x, int y, int sx16, int sy, int front, int lcd, int etilt) {   // the toy (x, y its corner); the egg shows in its LCD (the LCD is 12 x 11 art pixels: the egg 8 x 9 sits in the middle of it)
    int a = gf_sx, b = gf_sy; gf_sx = sx16; gf_sy = sy;
    if (front) { GSPR(TOY_FRONT, x, y, 0, 0); if (lcd) { gf_sq = 1; gf_spr(gf_egg(2, gegg_pat(gcb()), 0), FS_EGGS_W, FS_EGGS_H, x + (7 * sx16 >> 4), y + 10 * sy, etilt, 0); gf_sq = 0; } } else GSPR(TOY_BACK, x, y, 0, 0);
    gf_sx = a; gf_sy = b;
}
static u32 gf_h(u32 a) { a ^= a << 13; a ^= a >> 17; a ^= a << 5; return a * 2654435761u; }
static int gf_wave(int t, int per, int amp) { return isin((t * 4096 / per) & 4095) * amp >> 12; }
static int gf_ease(int p, int n) { p = CLAMP(p, 0, n); int q = 256 - p * 256 / n; return 256 - q * q / 256; }   // 0..256, quick then slow
static void gf_disc(int cx, int cy, int r, int ar, int ag, int ab, int sat) {   // a chunky circle of added light
    int st = MAX(3, r / 10);
    for (int dy = -r; dy < r; dy += st) { int a = dy + st / 2, hw = g_isq(MAX(0, r * r - a * a)); if (cy + dy + st <= gvo || cy + dy >= gvo + 192) continue; if (sat) gf_addsat(cx - hw, cy + dy, hw * 2, st, ar, ag, ab); else gf_addq(cx - hw, cy + dy, hw * 2, st, ar, ag, ab); }
}
static void gf_glow(int cx, int cy, int r, int ar, int ag, int ab) { for (int k = 0; k < 3; k++) gf_disc(cx, cy, r - k * r / 4, ar, ag, ab, 0); }   // a soft glow: three rings of faint light (5-bit numbers)
static void gf_flash(int v) { if (v > 0) { int a = MIN(31, (v + 4) >> 3); gf_addsat(0, gvo, 256, 192, a, a, a * 9 / 10); } }
static void gf_line(int x0, int y0, int x1, int y1, u16 c) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y1 - y0 : y0 - y1, sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx - dy;
    for (int n = 0; n < 400; n++) { px(x0, y0 - gvo, c); if (x0 == x1 && y0 == y1) break; int e2 = 2 * e; if (e2 > -dy) { e -= dy; x0 += sx; } if (e2 < dx) { e += dx; y0 += sy; } }
}
static void gf_rays(int cx, int cy, int n, int r0, int r1, int rot, int cr, int cg, int cb) {   // beams of light from a point (thin lines, three wide; every other one is left out)
    for (int k = 0; k < n; k += 2) { int a = (rot + k * 4096 / n) & 4095, c = icos(a), s = isin(a), x0 = cx + (c * r0 >> 12), y0 = cy + (s * r0 >> 12), x1 = cx + (c * r1 >> 12), y1 = cy + (s * r1 >> 12), p = ABS(c) < ABS(s) ? 1 : 0;
        if ((y0 < gvo - 4 && y1 < gvo - 4) || (y0 > gvo + 196 && y1 > gvo + 196)) continue;
        u16 c1 = C15(cr >> 3, cg >> 3, cb >> 3), c2 = C15((cr * 2 / 3) >> 3, (cg * 2 / 3) >> 3, (cb * 2 / 3) >> 3);
        gf_line(x0, y0, x1, y1, c1); gf_line(x0 + p, y0 + 1 - p, x1 + p, y1 + 1 - p, c2); gf_line(x0 - p, y0 - 1 + p, x1 - p, y1 - 1 + p, c2); }
}
static void gf_ring(int cx, int cy, int r, int n, int sz, int cr, int cg, int cb) { for (int k = 0; k < n; k++) { int a = k * 4096 / n; gf_rect(cx + (icos(a) * r >> 12) - sz / 2, cy + (isin(a) * r >> 12) - sz / 2, sz, sz, cr, cg, cb, 1); } }
static void gf_spark(int cx, int cy, int size) {   // a twinkle: 0 small, 1 middle, 2 big (centred)
    int sx = gf_sx, sy = gf_sy, se = gf_semi; gf_scale(2); gf_semi = 1;
    if (size == 0) gf_spr(FS_SPK_S, 3, 3, cx - 3, cy - 3, 0, 0); else if (size == 1) gf_spr(FS_SPK_M, 5, 5, cx - 5, cy - 5, 0, 0); else gf_spr(FS_SPK_L, 7, 9, cx - 7, cy - 9, 0, 0);
    gf_sx = sx; gf_sy = sy; gf_semi = se;
}
static void gf_confetti(int ox, int oy, int u, int n) {   // bits of paper thrown up, they tumble and drift down
    static const u8 cc[8][3] = { { 255, 100, 160 }, { 255, 210, 70 }, { 80, 210, 255 }, { 130, 230, 90 }, { 255, 255, 255 }, { 180, 130, 255 }, { 255, 150, 60 }, { 240, 70, 80 } };
    for (int i = 0; i < n; i++) { u32 h = gf_h((u32)(i * 31 + 7)); int uu = u - (int)((h >> 14) % 8); if (uu < 0) continue;
        int vx = (int)(h % 97) - 48, vy = -(88 + (int)((h >> 8) % 60)), x = ox + vx * uu / 12 + gf_wave(uu + i * 5, 40, 6), y = oy + vy * uu / 16 + uu * uu / 12;
        if (y > 380 || x < 2 || x > 252 || y + 4 < gvo || y > gvo + 192) continue; int sh = ((uu >> 2) + i) % 3; const u8 *c = cc[i & 7];
        rect(x, y - gvo, sh == 0 ? 3 : sh == 1 ? 2 : 4, sh == 0 ? 3 : sh == 1 ? 4 : 2, C15(c[0] >> 3, c[1] >> 3, c[2] >> 3)); }
}
// the title lettering: the 5 x 7 face at s pixels, a dark rim and shadow, a gold slope and a shine sliding across (shine: its x, -1 none)
static void gf_banner(int cx, int y, int s, const char *str, int shine) {
    static const u8 gr[7][3] = { { 255, 246, 170 }, { 255, 230, 112 }, { 255, 212, 66 }, { 255, 190, 44 }, { 246, 162, 30 }, { 226, 130, 22 }, { 196, 98, 16 } };
    if (y + 9 * s < gvo || y - s > gvo + 192) return;
    int n = (int)strlen(str), x0 = cx - (n * 6 * s - s) / 2;
    for (int pass = 0; pass < 2; pass++) for (int c = 0; c < n; c++) {
        const char *p = strchr(FS_FONT_CH, str[c] >= 'a' && str[c] <= 'z' ? str[c] - 32 : str[c]); if (!p || !*p) continue; const char *g = FS_FONT[p - FS_FONT_CH]; int gx = x0 + c * 6 * s;
        for (int j = 0; j < 7; j++) for (int i = 0; i < 5;) {
            if (g[j * 5 + i] != '#') { i++; continue; } int k = i; while (k < 5 && g[j * 5 + k] == '#') k++;
            int rx = gx + i * s, rw = (k - i) * s, ry = y + j * s;
            if (pass == 0) gf_fill(rx - s, ry - s, rw + 3 * s, 3 * s, K8(34, 10, 56));
            else { int r = gr[j][0], gg = gr[j][1], b = gr[j][2]; if (shine >= 0 && rx + rw > shine && rx < shine + 3 * s) { r = MIN(255, r + 70); gg = MIN(255, gg + 70); b = MIN(255, b + 110); } gf_fill(rx, ry, rw, s, K8(r, gg, b)); }
            i = k; }
    }
}
static int gf_banner_w(int s, const char *str) { return (int)strlen(str) * 6 * s - s; }
static void gf_tc(int cx, int y, int f, u16 c, const char *s) { text(cx - tw(f, s) / 2, y - gvo, f, c, s); }   // centred text at a canvas row
static void gf_title_case(char *s) { int up = 1; for (char *p = s; *p; p++) { if (*p == ' ') { up = 1; continue; } if (up) { up = 0; if (p[0] == 'T' && p[1] == 'H' && p[2] == 'E' && p[3] == ' ') { p[0] = 't'; p[1] = 'h'; p[2] = 'e'; p += 2; continue; } } else if (*p >= 'A' && *p <= 'Z') *p = (char)(*p + 32); } }   // BISCUIT THE RED SNAIL -> Biscuit the Red Snail

// ---- the sky: a deep violet gradient (dithered: the DS has 5 bits a colour), a slow glow in each screen, three layers of stars that drift and twinkle, now and then a shooting star ----
static void gf_sky(void) {
    static const u8 BAY[2][2] = { { 0, 2 }, { 3, 1 } };
    for (int y = 0; y < 192; y++) { int cy = y + gvo, r, g, b;
        if (cy < 180) { r = 26 + 4 * cy / 180; g = 12 + 4 * cy / 180; b = 62 + 12 * cy / 180; } else { int k = cy - 180; r = 30 - 25 * k / 204; g = 16 - 12 * k / 204; b = 74 - 46 * k / 204; }
        u16 p[2]; for (int xx = 0; xx < 2; xx++) { int t = BAY[y & 1][xx]; p[xx] = C15(MIN(31, (r / 2 + t) >> 2), MIN(31, (g / 2 + t) >> 2), MIN(31, (b / 2 + t) >> 2)); }
        u32 w = (u32)p[0] | ((u32)p[1] << 16), *q = (u32 *)(scr + y * 256); for (int i = 0; i < 128; i++) q[i] = w; }
    { int cx = 128 + (isin(ganim * 3 & 4095) >> 6), cy = gvo + 96 + (icos(ganim * 2 & 4095) >> 7); gf_disc(cx, cy, 88, 1, 0, 1, 0); gf_disc(cx, cy, 56, 0, 0, 1, 0); }
    static const u8 tint[3][3] = { { 214, 204, 255 }, { 255, 240, 180 }, { 170, 232, 255 } };
    for (int i = 0; i < 40; i++) {
        u32 h = gf_h((u32)(i * 77 + 5)); int lay = i < 20 ? 0 : i < 32 ? 1 : 2, sp = lay == 0 ? 14 : lay == 1 ? 9 : 6;
        int x = (int)(h % 256) - (ganim / sp) % 256; if (x < 0) x += 256; int y = (int)((h >> 9) % 384) + (ganim / (sp * 3)) % 384; if (y >= 384) y -= 384;
        if (y < gvo - 4 || y > gvo + 192) continue;
        const u8 *c = tint[i % 3]; int br = lay == 0 ? 70 : lay == 1 ? 130 : 200;
        if (lay < 2) { gf_rect(x, y, 2, 2, c[0] * br >> 8, c[1] * br >> 8, c[2] * br >> 8, 1); continue; }
        int ph = (ganim + i * 37) % 84, a = ph < 14 ? 5 - ABS(ph - 7) * 5 / 7 : 0;   // a flash every 84 ticks: the cross grows and shrinks
        gf_rect(x, y, 3, 3, c[0] * br >> 8, c[1] * br >> 8, c[2] * br >> 8, 1);
        if (a > 0) { gf_rect(x + 1 - a, y + 1, 2 * a + 1, 1, c[0] >> 1, c[1] >> 1, c[2] >> 1, 1); gf_rect(x + 1, y + 1 - a, 1, 2 * a + 1, c[0] >> 1, c[1] >> 1, c[2] >> 1, 1); }
    }
    { int c = ganim % 460; if (c < 30) { u32 h = gf_h((u32)(ganim / 460 + 99)); int hx = 150 + (int)(h % 106) - c * 9, hy = 10 + (int)((h >> 8) % 80) + c * 5;   // (a shooting star: a bright head, a fading tail)
        for (int k = 0; k < 7; k++) { int v = 220 - k * 30; if (c > 20) v = v * (30 - c) / 10; if (v > 0) gf_rect(hx + k * 7, hy - k * 3, k ? 3 : 4, k ? 2 : 4, v, v, v * 9 / 10, 1); } } }
}

// ---- the cues: a sound on a tick (the PS1 plays a voice clip at a pitch; here a jingle shifted by so many semitones; 255 is the pet's own voice) ----
typedef struct { u8 t, v; s8 sh; } GFCue;
static const GFCue gUbCue[] = { { 14, 2, -12 }, { 34, 2, -9 }, { 42, 2, -7 }, { 50, 2, -5 }, { 56, 2, -3 }, { 62, 2, -2 }, { 67, 2, 0 }, { 70, 5, 6 }, { 86, 5, 12 }, { 88, 6, 0 }, { 96, 5, 5 }, { 102, 5, 9 }, { 108, 5, 12 }, { 116, 5, 12 }, { 126, 5, 12 }, { 134, 5, 12 } };
static const GFCue gHCue[] = { { 12, 2, -13 }, { 18, 2, -13 }, { 34, 2, -10 }, { 40, 2, -10 }, { 46, 2, -10 }, { 52, 2, 0 }, { 58, 2, 3 }, { 64, 2, 4 }, { 72, 2, 5 }, { 78, 2, 7 }, { 84, 5, 12 }, { 88, 5, 12 }, { 90, 6, 0 }, { 98, 5, 9 }, { 104, 5, 12 }, { 110, 5, 12 }, { 112, 255, 0 } };
static void gf_cues(const GFCue *c, int n, int t0, int t1, const Blob *b) { for (int i = 0; i < n; i++) if (c[i].t > t0 && c[i].t <= t1) { if (c[i].v == 255) gvoicep(V_YEHA, b); else sfx_p(c[i].v, c[i].sh); } }
#define GUB_POP 86      // the unboxing: the tick the lid pops
#define GUB_DONE 130    // ... the toy has settled (Next jumps here)
#define GUB_END 330     // ... and the egg card comes by itself

// ---- the title: the lettering with a shine, a sky of stars, the kinds going by ----
static void gf_lcdtile(int x, int y, int sp, int col, int fr) {   // a pet on a little rounded LCD, 64 x 32
    if (y + 40 < gvo || y - 4 > gvo + 192) return;
    gf_round(x - 3, y - 3, 70, 38, K8(52, 24, 70)); gf_round(x - 2, y - 2, 68, 36, K8(30, 12, 44)); gf_fill(x, y, 64, 32, LCD_BG);
    Blob b; memset(&b, 0, sizeof b); b.on = 1; b.stage = GS_ADULT; b.hun = b.hap = 4; b.sp = (u8)(sp % GSP_N); b.col = (u8)col; gmd = MD_PLAIN; g_scene(&b, fr, 0); gdots_draw(x, y - gvo, 2, gcolInk[col & 7]);
}
static void gf_title(void) {
    int y = 22 + gf_wave(ganim, 110, 2), w = gf_banner_w(3, "BOREAGOTCHI!"), x0 = 128 - w / 2; char s[48];
    gf_glow(128, y + 12, 90, 1, 1, 0);
    { int sh = x0 - 60 + (ganim * 3) % 420; gf_banner(128, y, 3, "BOREAGOTCHI!", sh > x0 - 10 && sh < x0 + w ? sh : -1); }
    gf_spark(x0 - 8, y + 10, ((ganim >> 4) & 1) * 2); gf_spark(x0 + w + 8, y + 14, (((ganim >> 4) + 1) & 1) * 2); gf_spark(128 + gf_wave(ganim, 200, 100), y - 8, 0);
    gf_tc(128, 60, FS_BODY, C15(23, 25, 31), "The pocket pet that needs you");
    for (int i = 0; i < 3; i++) { int sp = (ganim / 90 + i * 5 + 1) % GSP_N, col = (ganim / 90 + i) % 7 + 1, x1 = 12 + i * 84, y1 = 82 + (i == 1 ? -6 : 0);
        gf_lcdtile(x1, y1, sp, col, ganim / 20 + i); gf_tc(x1 + 32, y1 + 43, FS_BODY, TXT, gspNm[sp]); }
    { Toy *m = gtoy_of(ctl); int c = m ? gcalling(m) : 0; if (c) sprintf(s, "%d of your pets want you", c); else strcpy(s, "All quiet. Your pets are fine");
      int cw = tw(FS_BODY, s) + 16; gf_round(128 - cw / 2, 150, cw, 16, c ? C15(24, 5, 5) : C15(5, 18, 8)); gf_tc(128, 154, FS_BODY, TXT, s); }
}

// ---- the shop card (the top screen is a card with the toy; the touch screen says thank you) ----
static void gf_shop_top(void) {
    panel(4, 4, 248, 184, "Electronics shop");
    rect(10, 24, 104, 130, K8(240, 170, 30)); rect(11, 25, 102, 128, K8(255, 214, 70)); gf_setup(gf_shell(), gcb()->col, 1); gf_noclip(); gf_toy(29, 28, 48, 3, 1, 1, 0);
    text_box(10, 112, 104, 14, FS_HEAD, K8(40, 30, 10), AL_C, "BOREAGOTCHI!"); text_box(10, 125, 104, 12, FS_BODY, K8(40, 30, 10), AL_C, "16 kinds inside"); { const char *p = "$5000 paid"; int pw = tw(FS_BODY, p) + 14; rect(62 - pw / 2, 139, pw, 13, C15(6, 20, 8)); text_box(62 - pw / 2, 139, pw, 13, FS_BODY, TXT, AL_C, p); }
    static const char *const bl[4] = { "Sixteen kinds to collect.", "Feed, play, clean and tuck it in.", "Bond, lay eggs, raise a family.", "One toy per Bore. Eight pets each." };
    for (int i = 0; i < 4; i++) { int y = 24 + i * 32; rect(120, y, 128, 29, PANB); rect(121, y + 1, 126, 27, PAN); rect(125, y + 10, 8, 8, C15(10, 26, 10)); gwrapn(138, y + 5, 106, 3, TXT, bl[i]); }
}
static void gf_shop_bottom(void) {
    gf_glow(128, 192 + 70, 70, 1, 1, 0); gf_spark(40, 192 + 44, (ganim >> 4) & 1); gf_spark(216, 192 + 64, ((ganim >> 4) + 1) & 1); gf_spark(60, 192 + 100, ((ganim >> 5) & 1)); gf_spark(200, 192 + 24, ((ganim >> 5) + 1) & 1);
    gf_tc(128, 192 + 28, FS_HEAD, C15(31, 26, 8), "Thank you!"); gf_tc(128, 192 + 48, FS_BODY, TXT, "Your toy came with"); gf_tc(128, 192 + 62, FS_BODY, TXT, "its first egg."); gf_tc(128, 192 + 92, FS_BODY, C15(23, 25, 31), "Tap Next to open the box.");
}

// ---- the unboxing: a box that drops from the top screen onto the touch screen, shakes, loses its bow and ribbon, pops its lid in a burst of light and confetti, and lets the toy rise up to the top screen ----
static void gf_unbox(int t) {
    const int GY = 332, CX = 128, MY = 262;   // the box's floor and middle (canvas rows), where the light bursts from
    int pop = t >= GUB_POP, u = t - GUB_POP; gf_setup(gf_shell(), gcb()->col, t >= 126); gf_noclip(); gf_semi = 0;
    int pw = 64, ph = 4, yo = 0, dx = 0, hop = 0;
    if (t < 14) yo = -342 + t * t * 342 / 196; else if (t < 18) { pw = 78; ph = 3; } else if (t < 30) yo = -(isin((t - 18) * 2048 / 12) * 10 >> 12);
    if (t >= 30 && t < GUB_POP) { int a = MIN(5, 1 + (t - 30) / 10); dx = ((t >> 1) & 1) ? a : -a; hop = t % 14 < 3 ? 3 : 0; }
    int bw = 30 * pw >> 4, bx = CX - bw / 2 + dx, top = GY + yo - 17 * ph, lidy = top - 6 * ph;
    { int gl = pop ? 3 + (u < 40 ? 4 : 0) : t >= 40 ? (t - 40) / 9 : 0; if (gl) gf_glow(CX, 192, pop ? 96 : 64, (gl * 3 + 4) >> 3, (gl * 2 + 4) >> 3, (gl + 4) >> 3); }
    if (pop && u < 44) { int r0 = 24 + u * 5; gf_rays(CX, MY, 14, r0, r0 + 120 * (44 - u) / 44, u * 8, 255, 232, 120); }
    gf_scale(1); gf_sx = pw; gf_sy = ph;
    GSPR(BOX_MOUTH, bx, top - 4 * ph, 0, 0);
    if (pop && u > 2) {   // the toy rises out of the box, turns right round and settles on the top screen
        int p = u - 2, e = gf_ease(p, 44), c = icos(e * 16), w16 = 48 * ABS(c) / 4096 + 6, y0 = top - 20, ty = y0 - (y0 - 62) * e / 256 + (e == 256 ? gf_wave(ganim, 90, 3) : 0), tw_ = 22 * w16 >> 4;
        if (e < 256) gf_glow(CX, ty + 40, 44, 1, 1, 0);
        gf_toy(CX - tw_ / 2, ty, w16, 3, c >= 0, t >= 126, ((ganim >> 5) & 3) == 0 ? gf_wave(ganim, 14, 3) : 0); gf_scale(1); gf_sx = pw; gf_sy = ph; }
    GSPR(BOX_BODY, bx, top, 0, 0);
    { int rw = t < 72 ? pw : t < 82 ? pw * (82 - t) / 10 : 0; if (rw > 0) { int x = CX - (6 * rw >> 4) / 2 + dx; int a = gf_sx; gf_sx = rw; GSPR(RIB_BODY, x, top, 0, 0); if (!pop) GSPR(RIB_LID, x, lidy - hop, 0, 0); gf_sx = a; } }
    { int ly2 = top + 11 * ph, lw2 = 90 * pw >> 6, lh2 = 5 * ph - 3; gf_round(CX - lw2 / 2 + dx, ly2, lw2, lh2, C15(31, 31, 29)); gf_fill(CX - lw2 / 2 + dx + 2, ly2 + lh2 - 2, lw2 - 4, 2, C15(29, 27, 26)); text_box(CX - lw2 / 2 + dx, ly2 - gvo, lw2, lh2, FS_SMALL, K8(120, 28, 60), AL_C, "BOREAGOTCHI!"); }
    if (t >= 40 && !pop) { int v = MIN(200, 40 + (t - 40) * 3) + (((t >> 1) & 1) ? 20 : 0); gf_rect(bx - 3, top - 1 - hop, bw + 6, 3 + hop, v, v * 3 / 4, v / 5, 1); }
    if (!pop) {
        int lx = CX - (32 * pw >> 4) / 2 + dx; GSPR(BOX_LID, lx, lidy - hop, 0, 0);
        if (t < 70) { int w16 = pw + (t >= 56 ? gf_wave(t, 8, 9) : 0), bwid = 22 * w16 >> 4; gf_sx = w16; GSPR(BOW, CX - bwid / 2 + dx, lidy - 8 * ph - hop, 0, 0); gf_sx = pw; }
    } else {   // the lid flies off to the right, up over the top screen; the bow tumbles off to the left, the ribbon's ends fall
        int lt = u, lx = CX - 64 + lt * 4, ly = lidy - lt * 7 + lt * lt / 8, tl = MIN(30, lt) - 6; gf_sx = 64; gf_sy = 4; GSPR(BOX_LID, lx, ly, tl, 0);
    }
    if (t >= 70) {
        int bt = t - 70, bx0 = CX - 44, by0 = 208; gf_sy = 4;
        if (bt < 70) { int w16 = 16 + ABS(icos(bt * 300)) * 48 / 4096; gf_sx = w16; GSPR(BOW, bx0 - bt * 3 + (88 - (22 * w16 >> 4)) / 2, by0 - bt * 3 + bt * bt / 12, 0, (bt / 3) & 1); }
        gf_sx = 64;
        if (bt < 70) { GSPR(TAIL_L, CX - 36 - bt * 2, by0 + 20 + bt * bt / 10, 0, 0); GSPR(TAIL_R, CX + 32 + bt * 2, by0 + 20 + bt * bt / 12, 0, 0); }
    }
    // the light, the confetti, the twinkles
    if (t >= 36 && !pop) for (int i = 0; i < 6; i++) { int ph2 = (t * 2 + i * 23) % 40; if (ph2 < 30) gf_spark(bx + 10 + (int)(gf_h((u32)(i * 13 + t / 20)) % (u32)(bw - 20)), top - 6 * ph - ph2 * 3, ph2 < 18 ? 1 : 0); }
    if (pop && u < 30) gf_ring(CX, MY, u * 11, 16, 4, 255 - u * 8, 240 - u * 8, 160 - u * 5);
    if (pop) gf_confetti(CX, MY, u, 40);
    if (t >= GUB_POP && t < GUB_POP + 6) gf_flash((GUB_POP + 6 - t) * 16);
    if (t >= GUB_DONE) for (int i = 0; i < 4; i++) { int a = (ganim * 5 + i * 1024) & 4095; gf_spark(CX + (icos(a) * 76 >> 12), 104 + (isin(a) * 58 >> 12), (i + (ganim >> 5)) & 1); }
    // the words (on the top screen)
    if (t >= 24 && t < 64) gf_tc(CX, 26, FS_BODY, TXT, "Something is inside...");
    else if (t >= 64 && t < 92) { int k = t - 64; gf_banner(CX, 17 + (k < 6 ? (6 - k) * 2 : 0), 3, "OPEN IT!", -1); }
    else if (t >= 92) { int k = t - 92, w = gf_banner_w(3, "TA-DA!"); gf_banner(CX, 17 + (k < 6 ? (6 - k) * 2 : 0), 3, "TA-DA!", k > 20 ? CX - w / 2 - 30 + (ganim * 4) % 260 : -1); gf_spark(CX - w / 2 - 14, 28, (ganim >> 4) & 1); gf_spark(CX + w / 2 + 14, 28, ((ganim >> 4) + 1) & 1); }
}

// ---- the egg card: the toy on a glow on the top screen, what to do on the touch screen ----
static void gf_eggcard(void) {
    gf_setup(gf_shell(), gcb()->col, 1); gf_noclip(); gf_semi = 0;
    int ty = 24 + gf_wave(ganim, 120, 3);
    gf_glow(128, ty + 70, 100, 2, 1, 0);
    gf_toy(73, ty, 80, 5, 1, 1, ((ganim >> 5) & 3) == 0 ? gf_wave(ganim, 14, 4) : 0);
    gf_spark(34, ty + 20, (ganim >> 4) & 1); gf_spark(224, ty + 52, ((ganim >> 4) + 1) & 1); gf_spark(44, ty + 126, ((ganim >> 5) & 1)); gf_spark(214, ty + 112, ((ganim >> 5) + 1) & 1);
    gf_round(8, 192 + 8, 240, 128, K8(255, 214, 70)); gf_round(10, 192 + 10, 236, 124, K8(40, 18, 64));
    gf_tc(128, 192 + 24, FS_HEAD, K8(255, 235, 140), "It came with an egg!");
    static const char *const ln[4] = { "Keep it warm.", "It hatches soon.", "Then feed it and play.", "I will show you how." };
    for (int i = 0; i < 4; i++) { gf_fill(16, 192 + 52 + i * 20, 4, 4, K8(255, 214, 70)); gf_fill(236, 192 + 52 + i * 20, 4, 4, K8(255, 214, 70)); gf_tc(128, 192 + 54 + i * 20, FS_BODY, TXT, ln[i]); }
}

// ---- hatching: the egg rocks, cracks and glows, bursts into pieces and light; the baby hops out of the shell ----
static const struct { u8 s, e, amp; } gHRock[5] = { { 8, 20, 6 }, { 30, 44, 8 }, { 52, 60, 12 }, { 62, 70, 14 }, { 72, 84, 16 } };
static void gf_hatch(void) {
    Blob *b = gcb(); int h = ghT; const u8 *sh = gShellPal[gf_shell() % 7]; gf_setup(gf_shell(), b->col, 1); gf_semi = 0; gf_noclip();
    const int lx = 24, ly = 20, lw = 208, lh = 108, ex0 = lx + (lw - FS_EGGB_W * 5) / 2;
    if (gvo == 0) {   // the toy (top screen): its shell, the dark frame, the LCD with its lines, three gold buttons
        gf_glow(128, 82, 118, 1, 1, 2);
        for (int y = 4; y < 160; y += 2) { int k = (y - 4) * 256 / 156, r = MIN(255, sh[0] + 40) * (256 - k) / 256 + sh[0] * 6 / 10 * k / 256, g = MIN(255, sh[1] + 40) * (256 - k) / 256 + sh[1] * 6 / 10 * k / 256, bl = MIN(255, sh[2] + 40) * (256 - k) / 256 + sh[2] * 6 / 10 * k / 256;
            int ins = y < 14 ? 14 - y : y > 148 ? y - 148 : 0; gf_fill(8 + ins, y, 240 - 2 * ins, 2, K8(r, g, bl)); }
        gf_round(16, 12, 224, 124, K8(54, 26, 74)); gf_fill(20, 14, 216, 3, C15(10, 8, 12));
        for (int i = 0; i < 3; i++) { int cx = 100 + i * 28; gf_round(cx - 6, 142, 13, 13, K8(255, 205, 60)); gf_fill(cx - 3, 144, 3, 3, K8(255, 246, 170)); }
        for (int y = ly; y < ly + lh; y += 2) { int k = (y - ly) * 8 / lh; gf_fill(lx, y, lw, 2, K8(182 - k * 3, 206 - k * 3, 138 - k * 2)); }
        for (int y = ly + 5; y < ly + lh; y += 6) gf_fill(lx, y, lw, 1, K8(156, 182, 116));
    }
    gf_clip(lx, ly, lx + lw, ly + lh);
    int rock = 0, hopy = 0; for (int i = 0; i < 5; i++) if (h >= gHRock[i].s && h < gHRock[i].e) rock = gf_wave(h - gHRock[i].s, (gHRock[i].e - gHRock[i].s) / 3, gHRock[i].amp);
    if (h >= 62 && h < 84 && (h % 8) < 3) hopy = -(h % 8 + 1) * 2;
    int stage = h >= 72 ? 3 : h >= 52 ? 2 : h >= 34 ? 1 : 0, ey = ly + (lh - (FS_EGGB_H - 2) * 5 - 10) / 2 + hopy, pat = gegg_pat(b);   // (5 px a dot, the LCD's own scale: 14 x 17 and its shadow, in the middle)
    if (h >= 52 && h < 88) { int v = MIN(14, (h - 52) / 3 + 2); gf_rect(lx, ly, lw, lh, v, v, v * 9 / 10, 1); }   // (the LCD brightens as the light inside grows)
    if (h < 88) { const char *const *er = gf_egg(0, pat, stage); gf_sx = 80; gf_sy = 5; gf_sq = 1;
        gf_spr(er + FS_EGGB_H - 2, FS_EGGB_W, 2, ex0, ey - hopy + (FS_EGGB_H - 2) * 5, 0, 0); gf_spr(er, FS_EGGB_W, FS_EGGB_H - 2, ex0, ey, rock, 0); gf_sq = 0; }   // (the shadow stays on the ground when it hops)
    if (h >= 84 && h < 96) { int v = h < 88 ? (h - 84) * 28 : 110 - (h - 88) * 14; gf_rect(lx, ly, lw, lh, MIN(255, v), MIN(255, v), MIN(255, v * 9 / 10), 1); }
    if (h >= 88) {   // the baby hops out of the shell; the shell's bits and the light fly
        int u = h - 88, hy = u < 22 ? -(isin(u * 2048 / 22) * 16 >> 12) : 0;
        gf_sx = 80; gf_sy = 5; gf_spr(FS_EGGCUP, FS_EGGCUP_W, FS_EGGCUP_H, ex0, ey - hopy + (FS_EGGB_H - FS_EGGCUP_H) * 5, 0, 0);
        if (h >= 92 && gvo == 0) { gmd = h > 112 ? MD_HAPPY : MD_PLAIN; g_scene(b, (h - 90) / 8, 0); gmd = MD_PLAIN; gdots_draw(lx + 8, ly - 4 + hy, 6, gcolInk[b->col & 7]); }   // (feet on the egg's floor)
        if (u < 36) gf_ring(128, 74, u * 5, 16, 4, 255 - u * 6, 250 - u * 6, 170 - u * 4);
        for (int i = 0; i < 12; i++) { u32 q = gf_h((u32)(i * 17 + 3)); int vx = (int)(q % 61) - 30, vy = -(30 + (int)((q >> 8) % 36)), x = 122 + vx * u / 4, y = 74 + vy * u / 6 + u * u / 14; gf_sx = 80; gf_sy = 5; gf_spr(gfShards[i & 3], gfShardW[i & 3], gfShardH[i & 3], x, y, 0, ((u >> 2) + i) & 1); }
        for (int i = 0; i < 6; i++) { int ph = (u * 2 + i * 31) % 50; if (u > 6 && ph < 36) gf_spark(lx + 22 + (int)(gf_h((u32)(i * 29 + 1)) % (u32)(lw - 44)), ly + 16 + (int)(gf_h((u32)(i * 19 + 5)) % 60), ph < 20 ? 1 : 0); }
    }
    gf_noclip();
    if (h >= 84 && h < 96) { int v = h < 88 ? (h - 84) * 28 : 110 - (h - 88) * 14; gf_flash(v / 2); }   // (a flash that spills over both screens)
    // the words (on the touch screen)
    { char s[56], nm[40]; const int by = 192 + 22;
      if (h < 52) gf_tc(128, by + 10, FS_BODY, TXT, "Something is moving inside...");
      else if (h < 88) gf_banner(128, by + ((h >> 2) & 1), 2, "IT IS CRACKING!", -1);
      else if (h < 112) { int k2 = h - 88; gf_banner(128, by - 4 + (k2 < 6 ? (6 - k2) * 2 : 0), 3, "POP!", -1); }
      else { int k2 = h - 112; gf_banner(128, by + (k2 < 6 ? (6 - k2) : 0), 2, "IT HATCHED!", k2 > 12 ? 70 + (ganim * 4) % 200 : -1); gname(b, nm); sprintf(s, "%s THE %s", ggiv[b->nm & 31], nm); gf_title_case(s); gf_tc(128, by + 36, FS_HEAD, TXT, s);
          if (b->mut) gf_tc(128, by + 54, FS_BODY, K8(255, 130, 200), "A rare one!"); else { sprintf(s, "%s. Gen %d", gcolNm[b->col & 7], b->gen + 1); gf_title_case(s); gf_tc(128, by + 54, FS_BODY, C15(23, 25, 31), s); } }
    }
}

// ---- which screen draws what ----
static int gf_first(void) { return gview == GV_TITLE || gview == GV_INTRO || gview == GV_HATCH; }
static void gf_screen(int bottom) {   // the first-run view on the top screen (0) or the touch screen (1): the sky, then the picture for that screen
    gvo = bottom ? 192 : 0; gf_semi = 0; gf_sq = 0; gf_noclip();
#ifdef DSTEST
    u32 c0 = TIMER_DATA(0) | ((u32)TIMER_DATA(1) << 16); gf_sky(); gfp[bottom] = (TIMER_DATA(0) | ((u32)TIMER_DATA(1) << 16)) - c0; c0 = TIMER_DATA(0) | ((u32)TIMER_DATA(1) << 16);
#else
    gf_sky();
#endif
    if (gview == GV_TITLE) gf_title();
    else if (gview == GV_HATCH) gf_hatch();
    else if (gisub == 1) gf_unbox(giT);
    else if (gisub == 2) gf_eggcard();
    else if (bottom) gf_shop_bottom(); else gf_shop_top();
#ifdef DSTEST
    gfp[2 + bottom] = (TIMER_DATA(0) | ((u32)TIMER_DATA(1) << 16)) - c0;
#endif
    gvo = 0; gf_clip(0, 0, 256, 192);
}
