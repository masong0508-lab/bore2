typedef struct { s16 x, y, w, h; u16 key; u8 on; const char *lab; } GBtn;   // a button of the touch screen: where, what key it presses
#define GBN(a) ((int)(sizeof(a) / sizeof((a)[0])))
static const GBtn gb_box[] = { { 4, 136, 60, 22, PD_X, 1, "Open" }, { 66, 136, 60, 22, PD_SQ, 0, "Dex" }, { 128, 136, 60, 22, PD_R1, 0, "Awards" }, { 190, 136, 62, 22, PD_ST, 0, "Shop" },
    { 4, 160, 60, 24, PD_SEL, 0, "Help" }, { 66, 160, 60, 24, PD_L1, 0, "Time" }, { 128, 160, 124, 24, PD_O, 0, "Back" } };
static const GBtn gb_guide[] = { { 8, 150, 70, 34, PD_L, 0, "< Page" }, { 84, 150, 88, 34, PD_O, 1, "Back" }, { 178, 150, 70, 34, PD_R, 0, "Page >" } };
static const GBtn gb_dex[] = { { 8, 30, 70, 40, PD_L1, 0, "< Page" }, { 178, 30, 70, 40, PD_R1, 0, "Page >" }, { 78, 100, 100, 40, PD_O, 1, "Back" } };
static const GBtn gb_tree[] = { { 8, 40, 70, 60, PD_L, 0, "< Prev" }, { 178, 40, 70, 60, PD_R, 0, "Next >" }, { 78, 110, 100, 50, PD_O, 1, "Back" } };
static const GBtn gb_basket[] = { { 8, 154, 42, 30, PD_SQ, 0, "Dex" }, { 52, 154, 54, 30, PD_R1, 0, "Family" }, { 108, 154, 44, 30, PD_L1, 0, "Home" }, { 154, 154, 44, 30, PD_SEL, 0, "Help" }, { 200, 154, 48, 30, PD_O, 0, "Back" } };
static const GBtn gb_hand[] = { { 8, 138, 60, 20, PD_L1, 0, "< Pet" }, { 72, 138, 114, 20, PD_X, 1, "Use" }, { 190, 138, 58, 20, PD_R1, 0, "Pet >" }, { 8, 164, 100, 22, PD_O, 0, "Basket" } };
static const GBtn gb_feed[] = { { 6, 134, 78, 48, PD_L1, 0, "Lunch" }, { 88, 134, 80, 48, PD_X, 1, "Give" }, { 172, 134, 76, 48, PD_O, 0, "Back" } };
static const GBtn gb_play[] = { { 8, 150, 108, 30, PD_X, 1, "Play" }, { 140, 150, 108, 30, PD_O, 0, "Back" } };
static const GBtn gb_look[] = { { 8, 30, 114, 90, PD_L, 0, "< Left" }, { 134, 30, 114, 90, PD_R, 0, "Right >" }, { 140, 150, 108, 30, PD_O, 0, "Quit" } };
static const GBtn gb_copy[] = { { 88, 24, 80, 44, PD_U, 0, "Up" }, { 88, 100, 80, 44, PD_D, 0, "Down" }, { 8, 62, 76, 44, PD_L, 0, "Left" }, { 172, 62, 76, 44, PD_R, 0, "Right" }, { 140, 154, 108, 28, PD_O, 0, "Quit" } };
static const GBtn gb_catch[] = { { 8, 24, 112, 100, PD_L, 0, "< Left" }, { 136, 24, 112, 100, PD_R, 0, "Right >" }, { 140, 150, 108, 30, PD_O, 0, "Quit" } };
static const GBtn gb_hilo[] = { { 8, 30, 112, 90, PD_U, 1, "Higher" }, { 136, 30, 112, 90, PD_D, 0, "Lower" }, { 140, 150, 108, 30, PD_O, 0, "Quit" } };
static const GBtn gb_friend[] = { { 8, 30, 60, 50, PD_L, 0, "<" }, { 188, 30, 60, 50, PD_R, 0, ">" }, { 8, 100, 112, 44, PD_X, 1, "Playdate" }, { 136, 100, 112, 44, PD_SQ, 0, "Battle" }, { 140, 154, 108, 28, PD_O, 0, "Back" } };
static const GBtn gb_stats[] = { { 8, 60, 114, 60, PD_X, 1, "Next" }, { 134, 60, 114, 60, PD_O, 0, "Back" } };
static const GBtn gb_dress[] = { { 8, 30, 60, 60, PD_L, 0, "< Prev" }, { 188, 30, 60, 60, PD_R, 0, "Next >" }, { 72, 30, 112, 60, PD_X, 1, "Wear / Buy" }, { 72, 110, 112, 50, PD_O, 0, "Back" } };
static const GBtn gb_home[] = { { 8, 112, 112, 36, PD_SQ, 0, "Walls" }, { 136, 112, 112, 36, PD_O, 1, "Back" } };
static const GBtn gb_award[] = { { 8, 40, 70, 60, PD_L, 0, "< Prev" }, { 178, 40, 70, 60, PD_R, 0, "Next >" }, { 78, 110, 100, 50, PD_O, 1, "Back" } };
static const GBtn gb_diary[] = { { 58, 70, 140, 60, PD_O, 1, "Back" } };
static const GBtn gb_more[] = { { 8, 146, 240, 34, PD_O, 0, "Back" } };
static const GBtn gb_shop[] = { { 8, 150, 240, 32, PD_O, 1, "Back" } };
static const GBtn gb_none[] = { { 14, 150, 228, 30, PD_O, 0, "Back" } };
static const GBtn *gbtns(int *n) {
#define GBT(v, a) case v: *n = GBN(a); return a;
    switch (gview) { GBT(GV_BOX, gb_box) GBT(GV_GUIDE, gb_guide) GBT(GV_DEX, gb_dex) GBT(GV_TREE, gb_tree) GBT(GV_BASKET, gb_basket) GBT(GV_HAND, gb_hand) GBT(GV_FEED, gb_feed) GBT(GV_PLAYPICK, gb_play) GBT(GV_LOOK, gb_look)
        GBT(GV_COPY, gb_copy) GBT(GV_CATCH, gb_catch) GBT(GV_HILO, gb_hilo) GBT(GV_FRIEND, gb_friend) GBT(GV_STATS, gb_stats) GBT(GV_DRESS, gb_dress) GBT(GV_HOME, gb_home) GBT(GV_AWARD, gb_award) GBT(GV_DIARY, gb_diary) GBT(GV_MORE, gb_more) GBT(GV_SHOP, gb_shop) GBT(GV_NONE, gb_none) }
#undef GBT
    *n = 0; return 0; }
static unsigned gt_bsig(void) { Toy *t = gtt(); return (unsigned)(gtree * 59 + gbx * 41 + gguide * 43 + o_gtime * 47 + gview * 31 + gbi * 7 + gisel * 11 + gsub * 13 + gdexpage * 17 + gmN * 19 + (gview == GV_COPY ? gmT >= 0 && gmT < gmN * 40 : 0) + (funds >= GOTCHI_PRICE) * 23 + (t->b[gbi].on) * 29
    + t->deco * 61 + t->room * 67 + t->accs * 71 + (int)(gach & 0xFFFF) * 73 + funds / 10 * 79); }
static void gt_bottom(void) {
    scr = offbuf; char b[40]; rect(0, 0, 256, 192, K8(22, 12, 52)); rect(0, 0, 256, 16, K8(60, 30, 110)); ctext(128, 4, 1, K8(90, 230, 240), "BOREAGOTCHI");
    int nb; const GBtn *bt = gbtns(&nb); Toy *t = gtt();
    if (gview == GV_NONE) { chip(14, 60, 228, 50, funds >= GOTCHI_PRICE ? "Buy a toy $5000" : "Toy $5000 (not enough)", funds >= GOTCHI_PRICE); }
    else if (gview == GV_BOX) { int n = MIN(nsim, GT_MAX);
        for (int i = 0; i < n; i++) { Toy *bx = gtoy_of(i); int y = 20 + i * 14, sl = i == gbx; rect(8, y, 240, 13, sl ? C15(31, 31, 20) : PANB); rect(9, y + 1, 238, 11, sl ? C15(24, 28, 31) : PAN);
            text_fit(14, y + 3, 1, 90, sl ? C15(2, 7, 20) : TXT, sname[i]);
            if (bx) { for (int q = 0; q < GB_N; q++) { const Blob *p = &bx->b[q]; rect(108 + q * 10, y + 3, 8, 7, !p->on ? C15(9, 7, 11) : gneeds(p) ? C15(29, 6, 6) : p->stage == GS_EGG ? C15(28, 20, 4) : C15(6, 24, 8)); }
                if (gcalling(bx)) { sprintf(b, "%d!", gcalling(bx)); text(194, y + 3, 1, C15(29, 6, 6), b); } else text(194, y + 3, 1, C15(8, 22, 10), "ok"); sprintf(b, "L%d", glevel(bx)); text(224, y + 3, 1, sl ? C15(2, 7, 20) : C15(20, 24, 28), b); }
            else text(108, y + 3, 1, sl ? C15(8, 14, 24) : C15(14, 18, 24), i == ctl ? "no toy: $5000" : "no toy"); } }
    else if (gview == GV_BASKET) {
        for (int i = 0; i < GB_N; i++) { const Blob *c = &t->b[i]; int x = 8 + (i & 3) * 61, y = 20 + (i >> 2) * 66, s = i == gbi;
            rect(x, y, 59, 63, s ? C15(31, 31, 20) : PANB); rect(x + 1, y + 1, 57, 61, s ? C15(24, 28, 31) : PAN);
            if (c->on) { g_scene(c, ganim / 20 + i, 0); rect(x + 2, y + 3, 55, 28, LCD_BG); gdots_draw(x + 2 + 12, y + 3 + 6, 1, gcolInk[c->col & 7]); if (gneeds(c)) { rect(x + 46, y + 3, 11, 11, C15(29, 6, 6)); text(x + 50, y + 5, 1, TXT, "!"); }
                text_fit(x + 3, y + 36, 1, 54, s ? C15(2, 7, 20) : TXT, ggiv[c->nm & 31]); gname(c, b); text_fit(x + 3, y + 47, 1, 54, s ? C15(8, 14, 24) : C15(20, 24, 28), b); for (int h = 0; h < 4; h++) rect(x + 5 + h * 12, y + 57, 9, 3, h < (c->bond + 12) / 25 ? C15(31, 8, 8) : C15(9, 7, 11)); }
            else { text(x + 13, y + 20, 1, s ? C15(2, 7, 20) : C15(14, 18, 24), s ? "NEW EGG" : "EMPTY"); text(x + 17, y + 36, 1, C15(31, 25, 6), "$150"); } } }
    else if (gview == GV_HAND) {
        for (int i = 0; i < 12; i++) { int x = 6 + (i % 6) * 41, y = 22 + (i / 6) * 56, s = i == gisel, lit = i == 6 && (gcb()->fl & GF_CALL);
            rect(x, y, 39, 52, s ? C15(31, 31, 31) : PANB); rect(x + 1, y + 1, 37, 50, s ? C15(26, 29, 31) : lit ? C15(31, 22, 6) : C15(11, 4, 14));
            u16 ic = s ? C15(16, 8, 4) : lit ? C15(4, 8, 14) : C15(24, 16, 30); for (int r = 0; r < 7; r++) for (int q = 0; q < 9; q++) if (gicon[i][r][q] == '#') rect(x + 6 + q * 3, y + 7 + r * 3, 3, 3, ic);
            t_tight = 1; text_fit(x + 19 - MIN(tw(1, giconNm[i]), 37) / 2, y + 40, 1, 37, s ? C15(2, 7, 20) : TXT, giconNm[i]); t_tight = 0; } }
    else if (gview == GV_FEED) { Blob *c = gcb();
        for (int i = 0; i < NFOOD; i++) { int x = 6 + (i % 4) * 61, y = 20 + (i / 4) * 37, s = i == gsub; gcell(x, y, 59, 35, s);
            t_tight = 1; text_fit(x + 3, y + 4, 1, 53, s ? C15(2, 7, 20) : TXT, gfoodNm[i]); t_tight = 0;
            text(x + 3, y + 15, 1, s ? C15(8, 14, 24) : C15(16, 20, 26), i < 6 ? "meal" : "snack");
            if (gfoodCost[i]) { sprintf(b, "$%d", gfoodCost[i]); text(x + 3, y + 25, 1, s ? C15(20, 12, 0) : C15(31, 25, 6), b); } else text(x + 3, y + 25, 1, s ? C15(8, 14, 24) : C15(18, 22, 28), "free");
            if ((c->taste >> 4) == i) { rect(x + 42, y + 22, 6, 3, C15(31, 6, 6)); rect(x + 50, y + 22, 6, 3, C15(31, 6, 6)); rect(x + 42, y + 25, 14, 3, C15(31, 6, 6)); rect(x + 45, y + 28, 8, 2, C15(31, 6, 6)); }
            else if ((c->taste & 15) == i) text(x + 33, y + 25, 1, C15(31, 10, 10), "YUCK"); } }
    else if (gview == GV_PLAYPICK) { static const char *const gn[4] = { "Which way?", "Copy me", "Catch", "High low" }, *const gd[4] = { "Guess where it looks. 5 rounds.", "Repeat its moves. 3 rounds.", "Slide for the treats. Dodge the mess.", "Higher or lower? 5 rounds." };
        for (int i = 0; i < 4; i++) chip(8 + (i & 1) * 122, 22 + (i >> 1) * 50, 118, 46, gn[i], i == gsub); text_fit(14, 126, 1, 228, TXT, gd[gsub]); }
    else if (gview == GV_HOME) { for (int i = 0; i < 6; i++) { int x = 8 + (i % 3) * 82, y = 24 + (i / 3) * 40; sprintf(b, "%s %s", gdecoNm[i], (t->deco >> i) & 1 ? "ON" : "OFF"); chip(x, y, 78, 36, b, i == gsub); }
        text_fit(14, 156, 1, 228, C15(20, 24, 28), gsim_of(t) == ctl ? "Tap a thing to put it out or away." : "You can only look at their home."); }
    else if (gview == GV_MORE) { static const char *const mn[6] = { "Family home", "Pet diary", "Awards", "Blobdex", "Help pages", "Shop" }; for (int i = 0; i < 6; i++) chip(8 + (i & 1) * 124, 22 + (i >> 1) * 40, 116, 36, mn[i], i == gsub); }
    else if (gview == GV_SHOP) { Toy *mt = gtoy_of(ctl);
        for (int i = 0; i < 8; i++) { int x = 6 + (i % 4) * 61, y = 22 + (i / 4) * 62, s = i == gsub; gcell(x, y, 59, 58, s);
            t_tight = 1; text_fit(x + 3, y + 6, 1, 53, s ? C15(2, 7, 20) : TXT, i < 7 ? gaccNm[i + 1] : "NEW EGG"); t_tight = 0;
            if (i < 7) { if (mt && (mt->accs & (1 << (i + 1)))) text(x + 3, y + 30, 1, s ? C15(0, 16, 4) : C15(20, 31, 20), "OWNED"); else { sprintf(b, "$%d", gaccCost[i + 1]); text(x + 3, y + 30, 1, s ? C15(20, 12, 0) : C15(31, 25, 6), b); } }
            else text(x + 3, y + 30, 1, s ? C15(20, 12, 0) : C15(31, 25, 6), "$150"); } }
    else if (gview == GV_AWARD || gview == GV_DIARY || gview == GV_TREE) { }
    for (int i = 0; i < nb; i++) { const char *lab = bt[i].lab; int on = bt[i].on;
        if (gview == GV_BOX && i == 0 && !gtoy_of(gbx) && gbx == ctl) lab = "Buy";
        if (gview == GV_BOX && i == 5) lab = o_gtime == 0 ? "T:Game" : o_gtime == 1 ? "T:LIVE" : "T:Off";
        if (gview == GV_LOOK && i < 2) on = gmN == 0 && gmDir == (i ? 1 : -1);
        if (gview == GV_FEED && i == 0) on = gsub == 12;
        if (gview == GV_NONE) continue;
        chip(bt[i].x, bt[i].y, bt[i].w, bt[i].h, lab, on); }
    if (gview == GV_HAND && gmsg) { text_fit(12, 118 + 8, 1, 232, C15(31, 25, 6), gmsg); }
}
static void gt_frame(int held, int down, int steps, int tx, int ty) {
    int pr = 0, td = down & KEY_TOUCH;
    if (down & KEY_LEFT) pr |= PD_L; if (down & KEY_RIGHT) pr |= PD_R; if (down & KEY_UP) pr |= PD_U; if (down & KEY_DOWN) pr |= PD_D; if (down & KEY_A) pr |= PD_X; if (down & KEY_B) pr |= PD_O; if (down & KEY_Y) pr |= PD_SQ; if (down & KEY_L) pr |= PD_L1; if (down & KEY_R) pr |= PD_R1;
    if (down & KEY_SELECT) pr |= PD_SEL; if (down & KEY_START) pr |= PD_ST;
    ghd = 0; if (held & KEY_LEFT) ghd |= PD_L; if (held & KEY_RIGHT) ghd |= PD_R; if ((held & KEY_TOUCH) && gview == GV_CATCH && ty >= 20 && ty < 140) ghd |= tx < 128 ? PD_L : PD_R;
    if (td) {
        int nb; const GBtn *bt = gbtns(&nb); for (int i = 0; i < nb; i++) if (hit(bt[i].x, bt[i].y, bt[i].w, bt[i].h, tx, ty)) pr |= bt[i].key;
        if (gview == GV_CATCH) pr &= ~(PD_L | PD_R);
        if (gview == GV_NONE && hit(14, 60, 228, 50, tx, ty)) pr |= PD_X;
        if (gview == GV_BOX) { int n = MIN(nsim, GT_MAX); for (int i = 0; i < n; i++) if (hit(8, 20 + i * 14, 240, 13, tx, ty)) { if (gbx == i) pr |= PD_X; gbx = i; } }
        else if (gview == GV_BASKET) { for (int i = 0; i < GB_N; i++) if (hit(8 + (i & 3) * 61, 20 + (i >> 2) * 66, 59, 63, tx, ty)) { if (gbi == i) pr |= PD_X; gbi = i; } }
        else if (gview == GV_HAND) { for (int i = 0; i < 12; i++) if (hit(6 + (i % 6) * 41, 22 + (i / 6) * 56, 39, 52, tx, ty)) { gisel = i; pr |= PD_X; } }
        else if (gview == GV_FEED) { for (int i = 0; i < NFOOD; i++) if (hit(6 + (i % 4) * 61, 20 + (i / 4) * 37, 59, 35, tx, ty)) { if (gsub == i) pr |= PD_X; gsub = i; } }
        else if (gview == GV_PLAYPICK) { for (int i = 0; i < 4; i++) if (hit(8 + (i & 1) * 122, 22 + (i >> 1) * 50, 118, 46, tx, ty)) gsub = i; }
        else if (gview == GV_HOME) { for (int i = 0; i < 6; i++) if (hit(8 + (i % 3) * 82, 24 + (i / 3) * 40, 78, 36, tx, ty)) { gsub = i; pr |= PD_X; } }
        else if (gview == GV_MORE) { for (int i = 0; i < 6; i++) if (hit(8 + (i & 1) * 124, 22 + (i >> 1) * 40, 116, 36, tx, ty)) { gsub = i; pr |= PD_X; } }
        else if (gview == GV_SHOP) { for (int i = 0; i < 8; i++) if (hit(6 + (i % 4) * 61, 22 + (i / 4) * 62, 59, 58, tx, ty)) { if (gsub == i) pr |= PD_X; gsub = i; } } }
    int og = gview, ob = gbi, oi = gisel, os = gsub;
    if (!gt_update(pr, steps)) { mg_ui = 0; mg_ver++; return; }
    if (pr || og != gview || ob != gbi || oi != gisel || os != gsub || gview == GV_LOOK || gview == GV_COPY || gview == GV_CATCH || gview == GV_HILO || gview == GV_HOME || (ganim & 15) < steps) mg_ver++;
}
