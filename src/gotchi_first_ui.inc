// gotchi_first_ui.inc - the BOREAGOTCHI! first run, drawn (included by gotchi_ui.c): the title, the shop card, the unboxing (a box that drops, shakes,
// loses its bow and ribbon, pops its lid in a burst of light and confetti, and lets a toy rise and spin out), the egg card, and the hatching (the egg
// rocks, cracks, glows, bursts; the baby hops out of the shell). Art: gotchi_first_art.h. Times are 60 a second (iT, hT). Everything of importance
// sits 8 pixels in from the screen's edge.
static int f_ease(int p, int n) { p = CLAMP(p, 0, n); int q = 256 - p * 256 / n; return 256 - q * q / 256; }   // 0..256, quick then slow
static int f_wave(int t, int per, int amp) { return isin((t * 4096 / per) & 4095) * amp >> 12; }
static void f_toy(int x, int y, int sx16, int sy, int front, int lcd, int etilt) {   // the toy (x, y its corner); the egg shows in its LCD
    int a = fsx, b = fsy; fsx = sx16; fsy = sy;
    if (front) { SPR(TOY_FRONT, x, y, 0, 0); if (lcd) { fsq = 1; f_spr(egg_make(2, egg_pat(cb(), ck()), 0), FS_EGGS_W, FS_EGGS_H, x + (7 * sx16 >> 4), y + 10 * sy, etilt, 0); fsq = 0; } } else SPR(TOY_BACK, x, y, 0, 0);   // (the LCD is 12 x 11 art pixels: the egg 8 x 9 sits in the middle of it)
    fsx = a; fsy = b;
}
static void f_rays(int cx, int cy, int n, int r0, int r1, int rot, int cr, int cg, int cb, int sz) {   // beams of light from a point (thin lines, three wide; sz 0: all but every other one)
    for (int k = 0; k < n; k++) { if (!sz && (k & 1)) continue; int a = (rot + k * 4096 / n) & 4095, c = icos(a), s = isin(a), x0 = cx + (c * r0 >> 12), y0 = cy + (s * r0 >> 12), x1 = cx + (c * r1 >> 12), y1 = cy + (s * r1 >> 12), px = abs(c) < abs(s) ? 1 : 0;
        gfx_line(x0, y0, x1, y1, cr, cg, cb); gfx_line(x0 + px, y0 + 1 - px, x1 + px, y1 + 1 - px, cr * 2 / 3, cg * 2 / 3, cb * 2 / 3); gfx_line(x0 - px, y0 - 1 + px, x1 - px, y1 - 1 + px, cr * 2 / 3, cg * 2 / 3, cb * 2 / 3); }
}
static void f_ring(int cx, int cy, int r, int n, int sz, int cr, int cg, int cb) { f_add(); for (int k = 0; k < n; k++) { int a = k * 4096 / n; f_rect(cx + (icos(a) * r >> 12) - sz / 2, cy + (isin(a) * r >> 12) - sz / 2, sz, sz, cr, cg, cb, 1); } }
static void f_confetti(int ox, int oy, int u, int n) {   // bits of paper thrown up, they tumble and drift down
    static const u8 cc[8][3] = { { 255, 100, 160 }, { 255, 210, 70 }, { 80, 210, 255 }, { 130, 230, 90 }, { 255, 255, 255 }, { 180, 130, 255 }, { 255, 150, 60 }, { 240, 70, 80 } };
    for (int i = 0; i < n; i++) { u32 h = hsh(i * 31 + 7); int uu = u - (int)((h >> 14) % 8); if (uu < 0) continue;
        int vx = (int)(h % 97) - 48, vy = -(40 + (int)((h >> 8) % 28)), x = ox + vx * uu / 16 + f_wave(uu + i * 5, 40, 6), y = oy + vy * uu / 16 + uu * uu / 16;
        if (y > 234 || x < 2 || x > 316) continue; int sh = ((uu >> 2) + i) % 3; const u8 *c = cc[i & 7];
        gfx_rect(x, y, sh == 0 ? 3 : sh == 1 ? 2 : 4, sh == 0 ? 3 : sh == 1 ? 4 : 2, c[0], c[1], c[2], 0); }
}
static void toy_pic(int x, int y, int egg_on) {   // a little toy on a card (the first day's welcome): the shell, its LCD, the egg
    f_setup(owner_color(tt()), ck()->col, 1); f_toy(x + 20, y + 8, 32, 2, 1, egg_on, 0);
}
static void f_prompt(const char *spec) { ui_hintbar(207, spec); }

// ---- the title: the lettering with a shine, a sky of stars, the kinds going by ----
static void draw_title(void) {
    sky(); int y = 24 + f_wave(anim, 110, 2), w = f_banner_w(3, "BOREAGOTCHI!"), x0 = 160 - w / 2;
    f_glow(160, y + 12, 96, 5, 4, 2);
    { int sh = x0 - 60 + (anim * 3) % 420; f_banner(160, y, 3, "BOREAGOTCHI!", sh > x0 - 10 && sh < x0 + w ? sh : -1); }
    f_spark(x0 - 10, y + 10, ((anim >> 4) & 1) * 2); f_spark(x0 + w + 10, y + 14, (((anim >> 4) + 1) & 1) * 2); f_spark(160 + f_wave(anim, 200, 90), y - 6, 0);
    tshadowc(160, 58, "THE POCKET PET THAT NEEDS YOU", 190, 200, 255);
    for (int i = 0; i < 3; i++) { Blob b; Kin k; memset(&b, 0, sizeof b); memset(&k, 0, sizeof k); b.on = 1; b.stage = GS_ADULT; b.hun = b.hap = 4; b.sp = (u8)((anim / 90 + i * 5 + 1) % 16); k.col = (u8)((anim / 90 + i) % 7 + 1);
        int x1 = 28 + i * 100, y1 = 88 + (i == 1 ? -6 : 0); lcd_tile(&b, &k, x1, y1, anim / 20 + i, 0); ui_textc(x1 + 32, y1 + 38, 1, gx_species(b.sp), UI_WHITE); }
    { Toy *m = my_toy(); char s[48]; if (m) { int c = gotchi_calling(m); if (c) sprintf(s, "%d OF YOUR PETS WANT YOU", c); else strcpy(s, "ALL QUIET. YOUR PETS ARE FINE"); chip(160, 156, s, c ? UI_RED : UI_GREEN, UI_WHITE); } }
    f_prompt("X PLAY|O LEAVE");
}

// ---- the unboxing ----
static void draw_unbox(int t) {
    sky(); int pop = t >= UB_POP, u = t - UB_POP; f_setup(owner_color(tt()), ck()->col, t >= 126); f_noclip(); fsemi = 0;
    int gy = 180, pw = 64, ph = 4, yo = 0, dx = 0, hop = 0;
    if (t < 14) yo = -150 + t * t * 150 / 196; else if (t < 18) { pw = 78; ph = 3; } else if (t < 30) yo = -(isin((t - 18) * 2048 / 12) * 10 >> 12);
    if (t >= 30 && t < UB_POP) { int a = MIN(5, 1 + (t - 30) / 10); dx = ((t >> 1) & 1) ? a : -a; hop = t % 14 < 3 ? 3 : 0; }
    int bw = 30 * pw >> 4, bx = 160 - bw / 2 + dx, top = gy + yo - 17 * ph, lidy = top - 6 * ph;
    { int gl = pop ? 3 + (u < 40 ? 4 : 0) : t >= 40 ? (t - 40) / 9 : 0; if (gl) f_glow(160, 96, pop ? 84 : 56, gl * 3, gl * 2, gl); }
    if (pop && u < 44) { int r0 = 20 + u * 4; f_rays(160, 100, 14, r0, r0 + 66 * (44 - u) / 44, u * 8, 255, 232, 120, 1); }
    f_scale(1); fsx = pw; fsy = ph;
    SPR(BOX_MOUTH, bx, top - 4 * ph, 0, 0);
    if (pop && u > 2) {   // the toy rises out of the box, turns right round and settles
        int p = u - 2, e = f_ease(p, 44), c = icos(e * 16), w16 = 48 * abs(c) / 4096 + 6, ty = 92 - 50 * e / 256 + (e == 256 ? f_wave(anim, 90, 3) : 0), tw = 22 * w16 >> 4;
        if (e < 256) f_glow(160, ty + 40, 44, 6, 5, 2);
        f_toy(160 - tw / 2, ty, w16, 3, c >= 0, t >= 126, ((anim >> 5) & 3) == 0 ? f_wave(anim, 14, 3) : 0); f_scale(1); fsx = pw; fsy = ph; }
    SPR(BOX_BODY, bx, top, 0, 0);
    { int rw = t < 72 ? pw : t < 82 ? pw * (82 - t) / 10 : 0; if (rw > 0) { int x = 160 - (6 * rw >> 4) / 2 + dx; int a = fsx; fsx = rw; SPR(RIB_BODY, x, top, 0, 0); if (!pop) SPR(RIB_LID, x, lidy - hop, 0, 0); fsx = a; } }
    { int ly2 = top + 11 * ph, lw2 = 90 * pw >> 6; ui_round(160 - lw2 / 2 + dx, ly2, lw2, 5 * ph - 3, 4, UI_RGB(255, 250, 240), UI_RGB(232, 218, 214), 0); gfx_text_raw(160 - gfx_text_w(1, "BOREAGOTCHI!") / 2 + dx, ly2 + (5 * ph - 3 - 7) / 2, 1, "BOREAGOTCHI!", 120, 28, 60); }
    if (t >= 40 && !pop) { f_add(); int v = MIN(200, 40 + (t - 40) * 3) + (((t >> 1) & 1) ? 20 : 0); f_rect(bx - 3, top - 1 - hop, bw + 6, 3 + hop, v, v * 3 / 4, v / 5, 1); }
    if (!pop) {
        int lx = 160 - (32 * pw >> 4) / 2 + dx; SPR(BOX_LID, lx, lidy - hop, 0, 0);
        if (t < 70) { int w16 = pw + (t >= 56 ? f_wave(t, 8, 9) : 0), bwid = 22 * w16 >> 4; fsx = w16; SPR(BOW, 160 - bwid / 2 + dx, lidy - 8 * ph - hop, 0, 0); fsx = pw; }
    } else {   // the lid flies off to the right, the bow tumbles off to the left, the ribbon's ends fall
        int lt = u + 0; int lx = 160 - 64 + lt * 4, ly = lidy - lt * 3 + lt * lt / 16 + (lt * lt * 0), tl = MIN(30, lt) - 6; fsx = 64; fsy = 4; SPR(BOX_LID, lx, ly, tl, 0);
    }
    if (t >= 70) {
        int bt = t - 70; int bx0 = 160 - 44, by0 = 64; fsy = 4;
        if (bt < 70) { int w16 = 16 + abs(icos(bt * 300)) * 48 / 4096; fsx = w16; SPR(BOW, bx0 - bt * 4 + (88 - (22 * w16 >> 4)) / 2, by0 - bt * 16 / 10 + bt * bt / 16, 0, (bt / 3) & 1); }
        fsx = 64;
        if (bt < 60) { SPR(TAIL_L, 160 - 36 - bt * 2, by0 + 20 + bt * bt / 10, 0, 0); SPR(TAIL_R, 160 + 32 + bt * 2, by0 + 20 + bt * bt / 12, 0, 0); }
    }
    // the light, the confetti, the twinkles
    if (t >= 36 && !pop) for (int i = 0; i < 6; i++) { int ph2 = (t * 2 + i * 23) % 40; if (ph2 < 30) f_spark(bx + 10 + (int)(hsh(i * 13 + t / 20) % (bw - 20)), top - 6 * ph - ph2, ph2 < 18 ? 1 : 0); }
    if (pop && u < 30) { f_ring(160, 100, u * 9, 16, 4, 255 - u * 8, 240 - u * 8, 160 - u * 5); }
    if (pop) f_confetti(160, 104, u, 40);
    if (t >= 86 && t < 92) f_flash((92 - t) * 16);
    if (t >= UB_DONE) { for (int i = 0; i < 4; i++) { int a = (anim * 5 + i * 1024) & 4095; f_spark(160 + (icos(a) * 74 >> 12), 90 + (isin(a) * 56 >> 12), (i + (anim >> 5)) & 1); } }
    // the words
    if (t >= 24 && t < 64) tshadowc(160, 26, "SOMETHING IS INSIDE...", 255, 255, 255);
    else if (t >= 64 && t < 92) { int k = t - 64; f_banner(160, 17 + (k < 6 ? (6 - k) * 2 : 0), 3, "OPEN IT!", -1); }
    else if (t >= 92) { int k = t - 92, w = f_banner_w(3, "TA-DA!"); f_banner(160, 17 + (k < 6 ? (6 - k) * 2 : 0), 3, "TA-DA!", k > 20 ? 160 - w / 2 - 30 + (anim * 4) % 260 : -1); f_spark(160 - w / 2 - 14, 28, (anim >> 4) & 1); f_spark(160 + w / 2 + 14, 28, ((anim >> 4) + 1) & 1); }
    f_prompt("X NEXT|START SKIP");
}

// ---- the egg card: the toy on a glow, what to do ----
static void draw_eggcard(int t) {
    sky(); f_setup(owner_color(tt()), ck()->col, 1); f_noclip(); fsemi = 0; (void)t;
    int ty = 34 + f_wave(anim, 120, 3);
    f_glow(88, ty + 70, 96, 7, 5, 2);
    f_toy(32, ty, 80, 5, 1, 1, ((anim >> 5) & 3) == 0 ? f_wave(anim, 14, 4) : 0);
    f_spark(30, ty + 14, (anim >> 4) & 1); f_spark(142, ty + 50, ((anim >> 4) + 1) & 1); f_spark(40, ty + 128, ((anim >> 5) & 1)); f_spark(130, ty + 120, ((anim >> 5) + 1) & 1);
    ui_round(150, 34, 148, 140, 10, UI_RGB(255, 214, 70), UI_RGB(236, 160, 28), 0); ui_round(152, 36, 144, 136, 8, UI_RGB(40, 18, 64), UI_RGB(20, 8, 38), 0);
    tshadowc(224, 46, "IT CAME WITH", 255, 235, 140); tshadowc(224, 58, "AN EGG!", 255, 235, 140);
    static const char *const ln[4] = { "KEEP IT WARM.", "IT HATCHES SOON.", "THEN FEED IT AND PLAY.", "I WILL SHOW YOU HOW." };
    for (int i = 0, y = 82; i < 4; i++) { f_rect(162, y + 2, 4, 4, 255, 214, 70, 0); y += ui_wrap(172, y, 120, 2, ln[i], UI_WHITE) * 10 + 8; }
    f_prompt("X START CARING");
}

static void draw_intro(void) {
    if (sub == 1) { draw_unbox(iT); return; }
    if (sub == 2) { draw_eggcard(iT); return; }
    sky();
    int top = ui_window(14, 8, 292, 224, "ELECTRONICS SHOP", IC_CASE);
    ui_round(24, top + 4, 104, 140, 8, UI_RGB(255, 214, 70), UI_RGB(240, 170, 30), 0);
    f_setup(owner_color(tt()), ck()->col, 1); f_noclip(); f_glow(76, top + 52, 40, 5, 3, 0); f_toy(43, top + 10, 48, 3, 1, 1, 0);
    ui_textc(76, top + 100, 1, "BOREAGOTCHI!", UI_INK); ui_textc(76, top + 112, 1, "20 KINDS INSIDE", UI_INK); ui_badge(40, top + 126, "$5000  PAID", UI_GREEN, UI_WHITE);
    static const char *const bl[4] = { "TWENTY KINDS TO COLLECT. FOUR ARE RARE.", "FEED, PLAY, CLEAN AND TUCK IT IN.", "BOND, LAY EGGS, RAISE A FAMILY.", "ONE TOY PER BORE. EIGHT PETS EACH." };
    for (int i = 0; i < 4; i++) { int y = top + 6 + i * 32; ui_round(136, y, 160, 29, 6, UI_WHITE, UI_PANEL, 0); ui_icon_s(141, y + 9, IC_TICK, 11, UI_GREEN); ui_wrap(157, y + 4, 136, 3, bl[i], UI_INK); }
    ui_wrapc(30, top + 150, 260, 2, "THANK YOU! YOUR TOY CAME WITH ITS FIRST EGG.", UI_SOFT);
    ui_hints(160, 210, "X UNBOX IT|START SKIP", 1);
}

// ---- hatching: the egg rocks, cracks and glows, bursts into pieces and light; the baby hops out of the shell ----
static const struct { u8 s, e, amp; } hRock[5] = { { 8, 20, 6 }, { 30, 44, 8 }, { 52, 60, 12 }, { 62, 70, 14 }, { 72, 84, 16 } };
static void draw_hatch(void) {
    sky(); Blob *b = cb(); Kin *k = ck(); int h = hT; const u8 *sh = PAL[1 + owner_color(tt()) % 7];
    f_setup(owner_color(tt()), k->col, 1); fsemi = 0; f_noclip();
    f_glow(160, 84, 120, 5, 3, 8);
    ui_round(36, 10, 248, 148, 20, UI_RGB(MIN(255, sh[0] + 40), MIN(255, sh[1] + 40), MIN(255, sh[2] + 40)), UI_RGB(sh[0] * 6 / 10, sh[1] * 6 / 10, sh[2] * 6 / 10), 0);
    ui_round(46, 18, 228, 124, 10, UI_RGB(54, 26, 74), UI_RGB(26, 10, 40), 0); f_add(); ui_round(48, 12, 200, 4, 2, UI_RGB(120, 120, 120), UI_RGB(120, 120, 120), 1);
    for (int i = 0; i < 3; i++) { ui_disc(132 + i * 28, 150, 9, UI_RGB(255, 205, 60)); ui_disc(131 + i * 28, 148, 3, UI_RGB(255, 246, 170)); }
    const int lx = 54, ly = 26, lw = 212, lh = 108;
    gfx_grad(lx, ly, lw, lh, UI_RGB(182, 206, 138), UI_RGB(182, 206, 138), UI_RGB(160, 186, 118), UI_RGB(160, 186, 118), 0);
    for (int y = ly + 5; y < ly + lh; y += 6) gfx_rect(lx, y, lw, 1, 156, 182, 116, 0);
    f_clip(lx, ly, lx + lw, ly + lh);
    int rock = 0, hopy = 0; for (int i = 0; i < 5; i++) if (h >= hRock[i].s && h < hRock[i].e) rock = f_wave(h - hRock[i].s, (hRock[i].e - hRock[i].s) / 3, hRock[i].amp);
    if (h >= 62 && h < 84 && (h % 8) < 3) hopy = -(h % 8 + 1) * 2;
    int stage = h >= 72 ? 3 : h >= 52 ? 2 : h >= 34 ? 1 : 0, ex = lx + (lw - FS_EGGB_W * 5) / 2, ey = ly + (lh - (FS_EGGB_H - 2) * 5 - 10) / 2 + hopy, pat = egg_pat(b, k);   // (5 px a dot, the LCD's own scale: 14 x 17 and its shadow, in the middle)
    if (h >= 52 && h < 88) { int v = MIN(14, (h - 52) / 3 + 2); f_add(); f_rect(lx, ly, lw, lh, v, v, v * 9 / 10, 1); }   // (the LCD brightens as the light inside grows: no halo round the egg)
    if (h < 88) { const char *const *er = egg_make(0, pat, stage); fsx = 80; fsy = 5; fsq = 1;
        f_spr(er + FS_EGGB_H - 2, FS_EGGB_W, 2, ex, ey - hopy + (FS_EGGB_H - 2) * 5, 0, 0); f_spr(er, FS_EGGB_W, FS_EGGB_H - 2, ex, ey, rock, 0); fsq = 0; }   // (the shadow stays on the ground when it hops)
    if (h >= 84 && h < 96) { f_add(); int v = h < 88 ? (h - 84) * 28 : 110 - (h - 88) * 14; f_rect(lx, ly, lw, lh, MIN(255, v), MIN(255, v), MIN(255, v * 9 / 10), 1); }
    if (h >= 88) {   // the baby hops out of the shell; the shell's bits and the light fly
        int u = h - 88; int hy = u < 22 ? -(isin(u * 2048 / 22) * 16 >> 12) : 0;
        fsx = 80; fsy = 5; f_spr(FS_EGGCUP, FS_EGGCUP_W, FS_EGGCUP_H, ex, ey - hopy + (FS_EGGB_H - FS_EGGCUP_H) * 5, 0, 0);
        if (h >= 92) { scene(b, k, (h - 90) / 8, 0, h > 112 ? MD_HAPPY : MD_NORM); dots_draw(64, 22 + hy, 6, 0, 16, 0, 32); }   // (feet on the egg's floor)
        if (u < 36) { f_ring(160, 84, u * 5, 16, 4, 255 - u * 6, 250 - u * 6, 170 - u * 4); }
        for (int i = 0; i < 12; i++) { u32 q = hsh(i * 17 + 3); int vx = (int)(q % 61) - 30, vy = -(30 + (int)((q >> 8) % 36)), x = 154 + vx * u / 4, y = 74 + vy * u / 6 + u * u / 14; fsx = 80; fsy = 5; f_spr(fsShard[i & 3], fsShardW[i & 3], fsShardH[i & 3], x, y, 0, ((u >> 2) + i) & 1); }
        for (int i = 0; i < 6; i++) { int ph = (u * 2 + i * 31) % 50; if (u > 6 && ph < 36) f_spark(lx + 22 + (int)(hsh(i * 29 + 1) % (lw - 44)), ly + 16 + (int)(hsh(i * 19 + 5) % 60), ph < 20 ? 1 : 0); }
    }
    f_noclip();
    { char s[56], nm[40];
      if (h < 52) tshadowc(160, 170, "SOMETHING IS MOVING INSIDE...", 255, 255, 255);
      else if (h < 88) { int w = f_banner_w(2, "IT IS CRACKING!"); f_banner(160, 166 + ((h >> 2) & 1), 2, "IT IS CRACKING!", -1); (void)w; }
      else if (h < 112) { int k2 = h - 88; f_banner(160, 164 + (k2 < 6 ? (6 - k2) * 2 : 0), 3, "POP!", -1); }
      else if (h >= 112) { int k2 = h - 112; f_banner(160, 164 + (k2 < 6 ? (6 - k2) : 0), 2, "IT HATCHED!", k2 > 12 ? 100 + (anim * 4) % 200 : -1); gx_name(b, nm); sprintf(s, "%s THE %s", gx_given(b->nm), nm); tshadowc(160, 184, s, 255, 255, 255);
             if (k->mut || b->sp >= 16) tshadowc(160, 195, "A RARE ONE!", 255, 130, 200); else { sprintf(s, "%s, %s. GEN %d", gx_colour(k->col), gx_pers(k->pers), b->gen + 1); tshadowc(160, 195, s, 190, 200, 255); } }
      if (h > 130) f_prompt("X SAY HELLO"); }
}
