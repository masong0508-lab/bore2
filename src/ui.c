// ui: see ui.h
#include "ui.h"
#include "gfx.h"
#include "atlas.h"

// ---- the icons: 12 x 12, '#' ink, '+' a softer shade, '.' clear. Drawn white and tinted when shown ----
static const char *const icons[IC_N][12] = {
    { "............", "...#........", "...##.......", "...###......", "...####.....", "...#####....", "...######...", "...#####....", "...####.....", "...###......", "...##.......", "...#........" },   // play
    { ".....##.....", "....####....", "...######...", "..###++###..", ".###++++###.", "###++++++###", ".###++++###.", "..###++###..", "...######...", "....####....", ".....##.....", "............" },   // plumbob
    { "............", "##.....#....", "####.####...", "#..###..#...", "#+..#+..##..", "#+..#+..##..", "#+..#+..##..", "#+..#+..##..", "####.####...", ".##.#.##....", "....#.......", "............" },   // book
    { "............", "............", "...######...", "..#..#...#..", ".#...#....#.", "############", "############", "#..####..###", ".##....##...", ".##....##...", "............", "............" },   // car
    { "....####....", "...######...", "..###..###..", "..##....##..", "..##....##..", "..###..###..", "...######...", "....####....", ".....##.....", ".....##.....", "....#..#....", "...######..." },   // pin
    { "..##....##..", ".####..####.", ".####..####.", "..##....##..", "............", ".####..####.", "######.#####", "######.#####", "######.#####", "............", "............", "............" },   // people
    { ".....##.....", ".....##.....", "....####....", "############", ".##########.", "..########..", "...######...", "..###..###..", "..##....##..", ".##......##.", "............", "............" },   // star
    { "..##........", ".####.......", ".####....#..", "..##....###.", ".........#..", ".####.......", "######......", "######......", "######......", "............", "............", "............" },   // add a person
    { "...####.....", "..######....", ".###+####...", ".##+#####...", ".########...", "..######....", "...####.....", "....##......", ".....#......", "....#.......", ".....#......", "....#......." },   // balloon
    { ".#######....", ".########...", ".#######....", "....##......", "....##......", "....##......", "....##......", "....##......", "....##......", "....##......", "............", "............" },   // hammer
    { "......####..", "......#..#..", "......#..##.", "......#.....", "......#.....", "......#.....", "...####.....", "..#####.....", "..####......", "............", "............", "............" },   // note
    { "....#..#....", "..#.####.#..", "...######...", ".####..####.", "..##....##..", ".##......##.", ".##......##.", "..##....##..", ".####..####.", "...######...", "..#.####.#..", "....#..#...." },   // gear
    { "...######...", "..##....##..", "........##..", ".......##...", ".....##.....", ".....##.....", "............", ".....##.....", ".....##.....", "............", "............", "............" },   // ?
    { ".##.....##..", ".###...###..", "..###.###...", "...#####....", ".....#......", ".....#......", ".....#......", ".#########..", "..#######...", "............", "............", "............" },   // sprout
    { "..########..", "..#++++++#..", "..#++++++#..", "..#++++++#..", "..#+++++##..", "..#++++++#..", "..#++++++#..", "..#++++++#..", "..########..", "............", "............", "............" },   // door
    { "............", "....####....", "..##....##..", ".#...##...#.", "#...####...#", "#...####...#", ".#...##...#.", "..##....##..", "....####....", "............", "............", "............" },   // eye
    { "....####....", "..########..", ".####+#####.", ".###+######.", "..########..", "....####....", ".....##.....", ".....##.....", "....####....", "............", "............", "............" },   // tree
    { ".#...#...#..", "..#..#..#...", "....###.....", "#..#####..#.", ".#########..", "#..#####..#.", "....###.....", "..#..#..#...", ".#...#...#..", "............", "............", "............" },   // sun
    { "......###...", ".....###....", "....###.....", "...#######..", ".....###....", "....###.....", "...###......", "..##........", "............", "............", "............", "............" },   // bolt
    { "...###......", ".##########.", "##########.#", "###....###.#", "##..##..####", "##..##..####", "###....#####", "############", "............", "............", "............", "............" },   // camera
    { ".#.#.#..##..", ".#.#.#.###..", ".#####.###..", "..###..###..", "...#...###..", "...#....#...", "...#....#...", "...#....#...", "...#....#...", "............", "............", "............" },   // food
    { ".....#......", "....###.....", "....###.....", "...#####....", "..##+####...", "..#+#####...", "..#######...", "...#####....", "....###.....", "............", "............", "............" },   // drop
    { ".######.....", ".....##.....", "....##......", "...##.......", "..##..####..", ".######..#..", "........#...", ".......####.", "............", "............", "............", "............" },   // zz
    { "....##......", "...#..#.....", "...#..#..##.", "....##..#..#", ".##......##.", "#..#........", "#..#...##...", ".##...#..#..", ".......##...", "............", "............", "............" },   // bubbles
    { "............", "..########..", "..########..", "#.########.#", "############", "############", "#..........#", "##........##", "............", "............", "............", "............" },   // sofa
    { "...######...", "..#......#..", ".#..#..#..#.", ".#........#.", ".#.#....#.#.", ".#..####..#.", "..#......#..", "...######...", "............", "............", "............", "............" },   // smile
    { ".##########.", "############", "##.#.#.#..##", "############", ".##########.", "..##........", ".##.........", "............", "............", "............", "............", "............" },   // chat
    { ".....##.....", "....####....", "...######...", "..########..", ".##########.", "..########..", "..##.##.##..", "..##.##.##..", "..##....##..", "..##....##..", "............", "............" },   // house
    { "............", "..###..###..", "..###..###..", "..###..###..", "..###..###..", "..###..###..", "..###..###..", "..###..###..", "..###..###..", "............", "............", "............" },   // pause
    { "............", "#.....#.....", "##....##....", "###...###...", "####..####..", "#####.#####.", "####..####..", "###...###...", "##....##....", "#.....#.....", "............", "............" },   // fast
    { "....####....", "...#....#...", "..#..##..#..", "..#.#.....#.", "...#.##....", "....##.#...", "..#.....#..", "..#..##..#..", "...#....#...", "....####....", "............", "............" },   // money (S)
    { "............", "............", "..........##", ".........###", "........###.", "##.....###..", "###...###...", ".###.###....", "..#####.....", "...###......", "....#.......", "............" },   // tick
    { "....####....", "...##..##...", "..##....##..", "..##....##..", ".##########.", ".##########.", ".####..####.", ".####..####.", ".####..####.", ".##########.", ".##########.", "............" },   // lock
    { "............", ".###....###.", "#####..#####", "############", "############", "############", ".##########.", "..########..", "...######...", "....####....", ".....##.....", "............" },   // heart
    { "............", "....####....", "...#....#...", "############", "############", "############", "#####..#####", "############", "############", "############", "............", "............" },   // briefcase
    { "............", "#####.######", "#####.######", "............", "##.#####.###", "##.#####.###", "............", "#####.######", "#####.######", "............", "##.#####.###", "##.#####.###" },   // bricks
    { "#####.#####.", "#####.#####.", "#####.#####.", "#####.#####.", "#####.#####.", "............", "#####.#####.", "#####.#####.", "#####.#####.", "#####.#####.", "#####.#####.", "............" },   // floor tiles
    { "#########...", "##########..", "#########.#.", "..........#.", "......#####.", "......#.....", "......#.....", ".....###....", ".....###....", ".....###....", ".....###....", "............" },   // paint roller
    { ".##########.", "############", "##..####..##", "##..####..##", "############", "#####..#####", "#####..#####", "############", "##..####..##", "##..####..##", "############", ".##########." },   // dice
    { "...##..##...", ".####..####.", "############", "############", "###.####.###", "...######...", "...######...", "...######...", "...######...", "...######...", "...######...", "............" },   // shirt
    { "............", "............", "..##....##..", "..###..###..", "...######...", "....####....", "....####....", "...######...", "..###..###..", "..##....##..", "............", "............" },   // the pad's cross
    { "............", "............", "....####....", "...##..##...", "..##....##..", "..#......#..", "..#......#..", "..##....##..", "...##..##...", "....####....", "............", "............" },   // circle
    { "............", ".....##.....", "....####....", "....#..#....", "...##..##...", "...#....#...", "..##....##..", "..#......#..", ".##########.", ".##########.", "............", "............" },   // triangle
    { "............", "............", "..########..", "..########..", "..##....##..", "..##....##..", "..##....##..", "..##....##..", "..########..", "..########..", "............", "............" },   // square
    { ".....##.....", "....####....", ".....##.....", "....####....", "...######...", "...#++++#...", "...######...", "...#++++#...", "...######...", "...#++++#...", "...######...", "............" },   // a baby's bottle
    { "...+..+..+..", "...#..#..#..", "...#..#..#..", ".##########.", ".#+#+#+#+##.", ".##########.", ".##########.", ".#++++++++#.", ".##########.", "############", "............", "............" },   // a birthday cake
};
static TexRef ic_tex, disc_tex; static int ui_ok;
void ui_init(void) {
    if (ui_ok) return;
    u16 *px = atlas_scratch();                                    // (the scratch is free between uploads)
    memset(px, 0, 120 * 60 * 2);
    for (int k = 0; k < IC_N; k++) {
        int ox = (k % 10) * 12, oy = (k / 10) * 12;
        for (int y = 0; y < 12; y++) { const char *row = icons[k][y];
            for (int x = 0; x < 12 && row[x]; x++) {
                u16 c = row[x] == '#' ? RGB(31, 31, 31) : row[x] == '+' ? RGB(22, 23, 25) : 0;
                px[(oy + y) * 120 + ox + x] = c;
            } }
    }
    ic_tex = atlas_alloc_page(2, 120, 60); atlas_upload(&ic_tex, px);
    u16 *dp = atlas_scratch();
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {   // a disc with a little shading toward its foot
        int dx = x * 2 - 15, dy = y * 2 - 15, d2 = dx * dx + dy * dy;
        dp[y * 16 + x] = d2 <= 15 * 15 + 8 ? (u16)(y > 11 ? RGB(28, 28, 29) : RGB(31, 31, 31)) : 0;
    }
    disc_tex = atlas_alloc_page(2, 16, 16); atlas_upload(&disc_tex, dp);
    ui_ok = 1;
}

static inline int cr(u32 c) { return c & 255; }
static inline int cg(u32 c) { return (c >> 8) & 255; }
static inline int cb(u32 c) { return (c >> 16) & 255; }
static u32 mix(u32 a, u32 b, int t, int n) {   // a .. b at t of n
    if (n <= 0) return a;
    return UI_RGB(cr(a) + (cr(b) - cr(a)) * t / n, cg(a) + (cg(b) - cg(a)) * t / n, cb(a) + (cb(b) - cb(a)) * t / n);
}
// size 1 is the interface's own smooth type (font size 3: the medium font shrunk, not the old 5-pixel one); 2 the medium
static inline int fsz(int size) { return size; }   // (gfx maps 1 to the smooth type itself)
int  ui_tw(int size, const char *s) { return gfx_text_w(fsz(size), s); }
void ui_text(int x, int y, int size, const char *s, u32 c) { gfx_text_raw(x, y, fsz(size), s, cr(c), cg(c), cb(c)); }
void ui_textc(int cx, int y, int size, const char *s, u32 c) { gfx_text_raw(cx - ui_tw(size, s) / 2, y, fsz(size), s, cr(c), cg(c), cb(c)); }

static const u8 rin4[4] = { 2, 1, 0, 0 }, rin6[6] = { 4, 2, 1, 1, 0, 0 };
void ui_round(int x, int y, int w, int h, int r, u32 top, u32 bot, int semi) {
    const u8 *in = r >= 6 ? rin6 : rin4; int rr = r >= 6 ? 6 : r >= 4 ? 4 : 0;
    if (!rr || h < 2 * rr + 1 || w < 2 * rr + 1) { gfx_grad(x, y, w, h, top, top, bot, bot, semi); return; }
    for (int i = 0; i < rr; i++) {
        if (!in[i] && i + 1 < rr && !in[i + 1]) {   // the rest of the corner rows are full width: one box down to the gradient
            u32 c0 = mix(top, bot, i, h);
            gfx_grad(x, y + i, w, h - 2 * i, c0, c0, mix(top, bot, h - i, h), mix(top, bot, h - i, h), semi);
            break;
        }
        u32 ct = mix(top, bot, i, h), cbm = mix(top, bot, h - 1 - i, h);
        gfx_rect(x + in[i], y + i, w - 2 * in[i], 1, cr(ct), cg(ct), cb(ct), semi);
        gfx_rect(x + in[i], y + h - 1 - i, w - 2 * in[i], 1, cr(cbm), cg(cbm), cb(cbm), semi);
        if (i == rr - 1) { u32 c0 = mix(top, bot, rr, h), c1 = mix(top, bot, h - rr, h); gfx_grad(x, y + rr, w, h - 2 * rr, c0, c0, c1, c1, semi); }
    }
}
void ui_shadow(int x, int y, int w, int h, int r) {
    gfx_ui_blend(2);                                              // subtract: the scene darkens under it
    ui_round(x + 2, y + 3, w, h, r, UI_RGB(46, 46, 52), UI_RGB(46, 46, 52), 1);
    ui_round(x + 1, y + 1, w, h, r, UI_RGB(30, 30, 34), UI_RGB(30, 30, 34), 1);
    gfx_ui_blend(1);
}
void ui_card(int x, int y, int w, int h) {
    ui_shadow(x, y, w, h, 6);
    ui_round(x, y, w, h, 6, UI_LINE, UI_LINE, 0);                 // the edge
    ui_round(x + 1, y + 1, w - 2, h - 2, 6, UI_CARD, UI_CARD2, 0);
}
// a white texel times the colour / 128: so the colour is halved going in, and white comes out as the colour itself
static inline int t8(int v) { return v >> 1; }
void ui_icon_s(int x, int y, int icon, int size, u32 c) {
    if (!ui_ok) ui_init();
    if (icon < 0 || icon >= IC_N) return;
    int u = ic_tex.u + (icon % 10) * 12, v = ic_tex.v + (icon / 10) * 12;
    gfx_sprite_c(x, y, size, size, ic_tex.tpage, u, v, 12, 12, t8(cr(c)) + 1, t8(cg(c)) + 1, t8(cb(c)) + 1);
}
void ui_icon(int x, int y, int icon, u32 c) { ui_icon_s(x, y, icon, 12, c); }
void ui_disc(int cx, int cy, int d, u32 c) {
    if (!ui_ok) ui_init();
    gfx_sprite_c(cx - d / 2, cy - d / 2, d, d, disc_tex.tpage, disc_tex.u, disc_tex.v, 16, 16, t8(cr(c)) + 1, t8(cg(c)) + 1, t8(cb(c)) + 1);
}
void ui_icon_disc(int cx, int cy, int d, int icon, u32 disc, u32 ink) {
    ui_disc(cx, cy, d, disc);
    if (icon >= 0) ui_icon(cx - 6, cy - 6, icon, ink);
}
int ui_window(int x, int y, int w, int h, const char *title, int icon) {
    ui_card(x, y, w, h);
    // the header: a blue band across the top, round at its upper corners
    ui_round(x + 1, y + 1, w - 2, 18, 6, UI_RGB(70, 176, 238), UI_BLUE2, 0);
    gfx_rect(x + 1, y + 13, w - 2, 6, cr(UI_BLUE2), cg(UI_BLUE2), cb(UI_BLUE2), 0);   // (square below: it meets the card)
    gfx_rect(x + 1, y + 19, w - 2, 1, 255, 255, 255, 0);
    int tx = x + 8;
    if (icon >= 0) { ui_icon_disc(x + 12, y + 10, 15, icon, UI_WHITE, UI_BLUE); tx = x + 24; }
    if (title) { gfx_text_raw(tx + 1, y + 6, 2, title, 16, 70, 130); gfx_text_raw(tx, y + 5, 2, title, 255, 255, 255); }
    return y + 24;
}
void ui_row(int x, int y, int w, int h, const char *label, int icon, int sel) {
    if (sel) {
        ui_round(x, y, w, h, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
        gfx_rect(x, y + 2, 2, h - 4, cr(UI_BLUE), cg(UI_BLUE), cb(UI_BLUE), 0);
    }
    int tx = x + 6;
    if (icon >= 0) { ui_icon(x + 5, y + (h - 12) / 2, icon, sel ? UI_BLUE : UI_SOFT); tx = x + 21; }
    ui_text(tx, y + (h - 7) / 2, 1, label, sel ? UI_BLUE2 : UI_INK);
}
void ui_button(int x, int y, int w, int h, const char *label, int icon, int sel) {
    if (sel) ui_round(x - 1, y - 1, w + 2, h + 2, 6, UI_RGB(120, 200, 245), UI_BLUE, 0);
    ui_round(x, y, w, h, 6, sel ? UI_RGB(70, 176, 238) : UI_WHITE, sel ? UI_BLUE2 : UI_RGB(232, 238, 246), 0);
    int tx = x + 8;
    if (icon >= 0) { ui_icon_disc(x + h / 2 + 2, y + h / 2, h - 4, icon, sel ? UI_WHITE : UI_BLUE, sel ? UI_BLUE : UI_WHITE); tx = x + h + 4; }
    ui_text(tx, y + (h - 7) / 2, 1, label, sel ? UI_WHITE : UI_INK);
}
void ui_bar(int x, int y, int w, int h, int pct, u32 c) {
    pct = CLAMP(pct, 0, 100);
    ui_round(x, y, w, h, h >= 9 ? 4 : 0, UI_RGB(210, 218, 228), UI_RGB(226, 232, 240), 0);
    int fw = (w - 2) * pct / 100;
    if (fw > 0) ui_round(x + 1, y + 1, fw, h - 2, h >= 11 ? 4 : 0, mix(c, UI_WHITE, 1, 3), c, 0);
}
void ui_toggle(int x, int y, int on, int sel) {
    u32 c = on ? UI_BLUE : UI_RGB(186, 194, 206);
    if (sel) ui_round(x - 1, y - 1, 24, 13, 4, UI_RGB(120, 200, 245), UI_RGB(120, 200, 245), 0);
    ui_round(x, y, 22, 11, 4, c, c, 0);
    ui_disc(on ? x + 16 : x + 5, y + 5, 9, UI_WHITE);
}
void ui_slider(int x, int y, int w, int v, int n, int sel) {
    if (n < 2) n = 2;
    int kx = x + 4 + (w - 8) * v / (n - 1);
    gfx_rect(x + 2, y + 3, w - 4, 3, 196, 204, 216, 0);
    gfx_rect(x + 2, y + 3, kx - x - 2, 3, cr(UI_BLUE), cg(UI_BLUE), cb(UI_BLUE), 0);
    for (int i = 0; i < n; i++) { int tx = x + 4 + (w - 8) * i / (n - 1); gfx_rect(tx, y + 2, 1, 5, i <= v ? 26 : 170, i <= v ? 112 : 178, i <= v ? 190 : 190, 0); }
    if (sel) ui_disc(kx, y + 4, 13, UI_RGB(120, 200, 245));
    ui_disc(kx, y + 4, 10, UI_BLUE2);
    ui_disc(kx, y + 4, 8, UI_WHITE);
}
void ui_badge(int x, int y, const char *s, u32 bg, u32 ink) {
    int w = ui_tw(1, s) + 10;
    ui_round(x, y, w, 12, 4, mix(bg, UI_WHITE, 1, 4), bg, 0);
    ui_text(x + 5, y + 2, 1, s, ink);
}
static int wrap_do(int x, int y, int w, int lines, const char *s, u32 c, int draw) {   // the words, broken onto lines that fit w; the lines used
    char line[96]; int n = 0;
    while (*s && n < lines) {
        int len = 0, fit = 0;
        while (s[len] && len < 95) {   // the longest run of whole words that fits
            int e = len; while (s[e] && s[e] != ' ') e++;
            memcpy(line, s, e); line[e] = 0;
            if (ui_tw(1, line) > w && fit) break;
            fit = e; len = e; if (s[len] == ' ') len++; else break;
        }
        if (!fit) fit = len;
        if (draw) { memcpy(line, s, fit); line[fit] = 0; ui_text(x, y + n * 10, 1, line, c); }
        s += fit; while (*s == ' ') s++;
        n++;
    }
    return n;
}
int ui_wrap(int x, int y, int w, int lines, const char *s, u32 c) { return wrap_do(x, y, w, lines, s, c, 1); }
int ui_wrap_n(int w, const char *s) { return wrap_do(0, 0, w, 9, s, 0, 0); }
void ui_dim(void) {
    gfx_ui_blend(2);
    gfx_rect(0, 0, SCREEN_W, SCREEN_H, 70, 70, 76, 1);             // the scene, a good deal darker
    gfx_ui_blend(1);
}

void ui_backdrop(void) {
    gfx_grad(0, 0, SCREEN_W, SCREEN_H, UI_RGB(210, 224, 240), UI_RGB(210, 224, 240), UI_RGB(170, 190, 214), UI_RGB(170, 190, 214), 0);
}
int ui_badge_i(int x, int y, int icon, const char *s, u32 bg, u32 ink) {
    int w = ui_tw(1, s) + 23;
    ui_round(x, y, w, 12, 4, mix(bg, UI_WHITE, 1, 4), bg, 0);
    ui_icon(x + 4, y, icon, ink);
    ui_text(x + 18, y + 2, 1, s, ink);
    return w;
}
// ---- the pad's buttons, the way the console shows them: the four shapes in their colours on a dark round button, the rest as
// dark pills with their names. A hint line is "X PLACE|O BACK": each part is its buttons, then what they do ----
static const char *const keyNm[] = { "X", "O", "TRI", "SQ", "L1", "R1", "L2", "R2", "L3", "R3", "START", "SEL", "DPAD", "RS" };
static const char *const keyShow[] = { "", "", "", "", "L1", "R1", "L2", "R2", "L3", "R3", "START", "SELECT", "D-PAD", "R-STICK" };
#define NKEY ((int)(sizeof keyNm / sizeof *keyNm))
static int key_of(const char *w, int n) {
    for (int i = 0; i < NKEY; i++) if ((int)strlen(keyNm[i]) == n && !memcmp(keyNm[i], w, n)) return i;
    return -1;
}
static int key_w(int k) { return k < 4 ? 13 : ui_tw(1, keyShow[k]) + 6; }
static int key_draw(int x, int y, int k) {
    static const u32 kc[4] = { UI_RGB(130, 180, 250), UI_RGB(255, 112, 112), UI_RGB(80, 222, 172), UI_RGB(250, 136, 226) };
    if (k < 4) { ui_disc(x + 6, y + 6, 13, UI_RGB(52, 60, 76)); ui_icon(x, y, IC_BX + k, kc[k]); return 13; }
    int w = key_w(k);
    ui_round(x, y + 1, w, 11, 4, UI_RGB(76, 86, 104), UI_RGB(46, 54, 70), 0);
    ui_text(x + 3, y + 3, 1, keyShow[k], UI_WHITE);
    return w;
}
int ui_key(int x, int y, const char *k) { int i = key_of(k, strlen(k)); return i < 0 ? 0 : key_draw(x, y, i); }
static int hints_do(int x, int y, const char *s, int draw) {
    int x0 = x; char w[32];
    while (*s) {
        for (;;) {                                     // the buttons
            const char *e = s; while (*e && *e != ' ' && *e != '|') e++;
            int k = key_of(s, e - s);
            if (k < 0) break;
            x += (draw ? key_draw(x, y, k) : key_w(k)) + 2;
            s = e; while (*s == ' ') s++;
        }
        const char *e = s; while (*e && *e != '|') e++;   // what they do
        int n = MIN((int)(e - s), 31); memcpy(w, s, n); w[n] = 0;
        if (n) { if (draw) ui_text(x + 1, y + 3, 1, w, UI_INK); x += ui_tw(1, w) + 1; }
        s = *e ? e + 1 : e;
        if (*s) x += 10;
    }
    return x - x0;
}
int ui_hints(int x, int y, const char *spec, int align) {
    int w = hints_do(0, 0, spec, 0);
    hints_do(align == 1 ? x - w / 2 : align == 2 ? x - w : x, y, spec, 1);
    return w;
}
void ui_hintbar(int y, const char *spec) {
    int w = hints_do(0, 0, spec, 0) + 14;
    ui_card(160 - w / 2, y, w, 17);
    hints_do(160 - w / 2 + 7, y + 2, spec, 1);
}
int ui_window_s(int x, int y, int w, int h, const char *title) {
    ui_card(x, y, w, h);
    ui_round(x + 1, y + 1, w - 2, 15, 6, UI_RGB(70, 176, 238), UI_BLUE2, 0);
    gfx_rect(x + 1, y + 10, w - 2, 6, cr(UI_BLUE2), cg(UI_BLUE2), cb(UI_BLUE2), 0);
    gfx_rect(x + 1, y + 16, w - 2, 1, 255, 255, 255, 0);
    if (title) ui_textc(x + w / 2, y + 5, 1, title, UI_WHITE);
    return y + 20;
}
void ui_pips(int x, int y, int v, int n, u32 c) {
    for (int k = 0; k < n; k++) {
        if (k < v) gfx_grad(x + k * 12, y, 10, 7, mix(c, UI_WHITE, 1, 3), mix(c, UI_WHITE, 1, 3), c, c, 0);
        else gfx_rect(x + k * 12, y, 10, 7, 212, 220, 230, 0);
    }
}
void ui_meter(int x, int y, int w, int m) {
    ui_round(x, y, w, 7, 0, UI_RGB(210, 218, 228), UI_RGB(226, 232, 240), 0);
    int half = (w - 2) / 2, mid = x + 1 + half, len = half * MIN(ABS(m), 100) / 100;
    if (len > 0) {
        u32 c = m >= 0 ? UI_GREEN : UI_RED, l = mix(c, UI_WHITE, 1, 3);
        gfx_grad(m >= 0 ? mid : mid - len, y + 1, len, 5, l, l, c, c, 0);
    }
    gfx_rect(mid, y - 1, 1, 9, 120, 132, 150, 0);
}
void ui_section(int x, int y, int w, const char *t) {
    ui_text(x, y, 1, t, UI_BLUE2);
    int tw = ui_tw(1, t) + 5;
    if (w > tw) gfx_rect(x + tw, y + 3, w - tw, 1, cr(UI_LINE), cg(UI_LINE), cb(UI_LINE), 0);
}
void ui_bubble(int cx, int y, const char *s, u32 c) {
    int w = ui_tw(1, s) + 10, x = cx - w / 2;
    ui_round(x - 1, y - 13, w + 2, 14, 4, UI_RGB(150, 162, 180), UI_RGB(150, 162, 180), 0);   // a thin edge, so it reads on a white wall
    ui_round(x, y - 12, w, 12, 4, UI_WHITE, UI_RGB(234, 239, 246), 0);
    gfx_rect(cx - 3, y, 6, 1, 150, 162, 180, 0); gfx_rect(cx - 2, y, 4, 1, 234, 239, 246, 0);
    gfx_rect(cx - 2, y + 1, 4, 1, 150, 162, 180, 0); gfx_rect(cx - 1, y + 1, 2, 1, 234, 239, 246, 0);
    gfx_rect(cx - 1, y + 2, 2, 1, 150, 162, 180, 0);
    ui_text(x + 5, y - 10, 1, s, c);
}
