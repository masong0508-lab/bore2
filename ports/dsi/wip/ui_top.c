static const char *const gdyVerb[16] = { "", "HATCHED", "GREW UP A BIT", "BECAME A GROWN-UP", "PASSED ON. REMEMBERED", "BROUGHT A GIFT", "HAD A BIRTHDAY", "HAD A TANTRUM", "WAS ILL", "IS A NEW EGG", "HAD A PLAYDATE", "IS A RARE MUTANT", "WAS LOOKED AFTER", "FELL IN LOVE", "VISITED A HOUSEMATE", "" };
static void gcell(int x, int y, int w, int h, int sel) { rect(x, y, w, h, sel ? C15(31, 31, 20) : PANB); rect(x + 1, y + 1, w - 2, h - 2, sel ? C15(24, 28, 31) : PAN); }
static void gtline(int x0, int y0, int x1, int y1) {   // a line of the tree: down from a parent, across, down to the child
    int ym = (y0 + y1) / 2, a = MIN(x0, x1), w = ABS(x1 - x0) + 1; u16 c = C15(14, 16, 24);
    rect(x0, y0, 1, ym - y0 + 1, c); rect(a, ym, w, 1, c); rect(x1, ym, 1, y1 - ym + 1, c); }
static void gtree_card(const Ln *o, int x, int y, int hl) {   // one relative: a little LCD with its name under it
    if (!o || !o->id) return; Blob bb; memset(&bb, 0, sizeof bb); bb.on = 1; bb.stage = GS_ADULT; bb.sp = o->sp; bb.hun = bb.hap = 4; g_scene(&bb, 0, 0);
    int gone = (o->nm & 128) != 0; rect(x - 1, y - 1, 46, 22, hl ? C15(31, 25, 6) : C15(9, 7, 11)); rect(x, y, 44, 20, LCD_BG); gdots_draw(x + 6, y + 2, 1, gone ? C15(14, 14, 14) : gcolInk[o->col & 7]);
    text_fit(x, y + 23, 1, 44, gone ? C15(18, 18, 18) : TXT, ggiv[o->nm & 31]); if (gone) text(x + 37, y + 1, 1, C15(31, 31, 31), "+"); }
static void gtop_tree(void) {
    Toy *t = gtt(); const Ln *me = &t->ln[gtree]; char b2[80];
    if (!me->id) { text_fit(12, 60, 1, 232, TXT, "THIS PET HAS NO FAMILY RECORD YET."); text_fit(12, 76, 1, 232, C15(20, 24, 28), "EGGS FROM A PAIR START A TREE."); return; }
    static const int gpx[4] = { 6, 54, 158, 206 }; const int mx = 30, fx = 182, ux = 106, y0 = 24, y1 = 59, y2 = 94, y3 = 129;
    const Ln *m = gln_of(t, me->mom), *f = gln_of(t, me->dad), *gp[4] = { m ? gln_of(t, m->mom) : 0, m ? gln_of(t, m->dad) : 0, f ? gln_of(t, f->mom) : 0, f ? gln_of(t, f->dad) : 0 };
    int sib[4], ns = 0, kid[5], nk = 0;
    for (int i = 0; i < GLN && ns < 4; i++) { const Ln *o = &t->ln[i]; if (o->id && o->id != me->id && ((me->mom && o->mom == me->mom) || (me->dad && o->dad == me->dad))) sib[ns++] = i; }
    for (int i = 0; i < GLN && nk < 5; i++) { const Ln *o = &t->ln[i]; if (o->id && (o->mom == me->id || o->dad == me->id)) kid[nk++] = i; }
    for (int i = 0; i < 4; i++) if (gp[i]) gtline(gpx[i] + 22, y0 + 31, (i < 2 ? mx : fx) + 22, y1 - 1);
    if (m) gtline(mx + 22, y1 + 31, ux + 22, y2 - 1); if (f) gtline(fx + 22, y1 + 31, ux + 22, y2 - 1);
    for (int q = 0; q < nk; q++) gtline(ux + 22, y2 + 31, 8 + q * 48 + 22, y3 - 1);
    for (int i = 0; i < 4; i++) gtree_card(gp[i], gpx[i], y0, 0);
    gtree_card(m, mx, y1, 0); gtree_card(f, fx, y1, 0);
    { static const int sx[4] = { 58, 154, 10, 202 }; for (int q = 0; q < ns; q++) gtree_card(&t->ln[sib[q]], sx[q], y2, 0); }
    gtree_card(me, ux, y2, 1);
    for (int q = 0; q < nk; q++) gtree_card(&t->ln[kid[q]], 8 + q * 48, y3, 0);
    sprintf(b2, "%s THE %s %s  GEN %d%s", ggiv[me->nm & 31], gcolNm[me->col & 7], gspNm[me->sp % GSP_N], gdepth(t, me->id, 0) + 1, (me->nm & 128) ? "  (PASSED ON)" : "");
    text_fit(8, 164, 1, 240, TXT, b2);
    if (t->mem) { sprintf(b2, "MEMORIAL: %d REMEMBERED. LAST: %s", t->mem, ggiv[t->memnm & 31]); text_fit(8, 175, 1, 240, C15(20, 24, 28), b2); } }
static void gtop_home(void) {
    static const u8 wall[6][3] = { { 255, 226, 150 }, { 150, 210, 240 }, { 245, 170, 200 }, { 170, 232, 200 }, { 60, 52, 110 }, { 240, 200, 90 } };
    Toy *t = gtt(); const u8 *w = wall[t->room % 6]; int dk = t->room % 6 == 4, d = t->deco; char s[40];
    panel(4, 4, 248, 184, groomNm[t->room % 6]);
    rect(8, 24, 240, 62, K8(w[0], w[1], w[2])); rect(8, 88, 240, 96, K8(w[0] * 6 / 10, w[1] * 6 / 10, w[2] * 6 / 10)); rect(8, 86, 240, 3, K8(w[0] / 2, w[1] / 2, w[2] / 2));
    for (int x = 8; x < 248; x += 24) rect(x, 88, 1, 96, K8(w[0] / 2, w[1] / 2, w[2] / 2));
    if (d & 1) { rect(20, 30, 34, 28, K8(255, 255, 255)); rect(22, 32, 30, 24, K8(140, 200, 255)); rect(36, 32, 2, 24, K8(255, 255, 255)); rect(22, 43, 30, 2, K8(255, 255, 255)); }
    if (d & 2) { rect(212, 72, 10, 14, K8(150, 90, 50)); for (int i = 0; i < 4; i++) rect(208 + i * 5, 52 + (i & 1) * 5, 6, 20, K8(60, 170, 70)); }
    if (d & 4) { rect(120, 60, 3, 28, K8(120, 100, 80)); rect(114, 50, 16, 11, K8(255, 230, 140)); }
    if (d & 8) { rect(70, 158, 120, 22, K8(220, 90, 110)); rect(80, 162, 100, 14, K8(250, 190, 100)); }
    if (d & 16) { rect(62, 30, 38, 26, K8(130, 90, 50)); rect(64, 32, 34, 22, K8(250, 240, 220)); for (int i = 0; i < 4; i++) rect(68 + i * 7, 38 + (i & 1) * 4, 5, 7, K8(230 - i * 30, 90 + i * 30, 120)); }
    if (d & 32) { rect(14, 140, 34, 22, K8(200, 130, 60)); rect(18, 136, 7, 5, K8(255, 100, 100)); rect(32, 134, 7, 7, K8(100, 160, 255)); rect(40, 138, 7, 4, K8(255, 220, 80)); }
    rect(156, 74, 90, 3, K8(120, 80, 50));   // the memorial shelf: the last of those who passed on
    { int shown = 0; for (int i = GLN - 1; i >= 0 && shown < 3; i--) { const Ln *o = &t->ln[i]; if (!o->id || !(o->nm & 128)) continue; Blob bb; memset(&bb, 0, sizeof bb); bb.on = 1; bb.stage = GS_ADULT; bb.sp = o->sp; bb.hun = bb.hap = 4; g_scene(&bb, 0, 0);
          int x = 160 + shown * 30; rect(x, 52, 26, 20, K8(255, 250, 235)); rect(x + 1, 53, 24, 18, K8(214, 222, 214)); gdots_draw(x - 3, 54, 1, C15(10, 10, 10)); rect(x + 10, 72, 6, 2, K8(250, 120, 160)); shown++; }
      if (!shown) text(166, 58, 1, dk ? C15(28, 28, 31) : C15(14, 10, 6), "MEMORIALS"); }
    int order[GB_N], n = 0; for (int g = 0; g < 100; g++) for (int i = 0; i < GB_N; i++) if (t->b[i].on && t->b[i].gen == g) order[n++] = i;   // the family: parents at the back, the young in front
    for (int j = 0; j < n; j++) { Blob *b = &t->b[order[j]]; int row = (j * 2 < n || n <= 2) ? 0 : 1, cnt = row ? n / 2 : (n <= 2 ? n : (n + 1) / 2), col = row ? j - (n + 1) / 2 : j;
        int x = 8 + (240 - cnt * 58) / 2 + col * 58 - 3, y = row ? 144 : 100; g_scene(b, ganim / 20 + j, 0); gdots_draw(x, y + (b->stage <= GS_CHILD ? 8 : 0), 2, dk ? C15(28, 28, 31) : gcolInk[b->col & 7]);
        if (b->mate) { rect(x + 44, y - 2, 6, 3, C15(31, 6, 6)); rect(x + 45, y + 1, 4, 2, C15(31, 6, 6)); } }
    if (!n) text(80, 130, 1, TXT, "THE HOME IS EMPTY");
    sprintf(s, "%s: %s", gdecoNm[gsub % 6], (d >> (gsub % 6)) & 1 ? "ON" : "OFF"); rect(10, 175, 100, 11, C15(8, 12, 24)); text(14, 177, 1, TXT, s);
    if (gmsg) { rect(112, 175, 134, 11, C15(8, 12, 24)); text_fit(116, 177, 1, 128, TXT, gmsg); } }
static void gtop_diary(void) {
    panel(4, 4, 248, 184, "Pet diary"); int shown = 0;
    for (int i = 1; i <= GD_N && shown < 13; i++) { const Dy *d = &gdy[((gdn - i) % GD_N + GD_N) % GD_N]; if (!d->kind || i > gdn) continue; char s[80]; int day = d->day / 24 + 1, hr = d->day % 24;
        if (d->kind == GD_AWARD) sprintf(s, "DAY %d %02d  AWARD: %s", day, hr, gachNm[d->nm % NACH]); else sprintf(s, "DAY %d %02d  %s %s", day, hr, ggiv[d->nm & 31], gdyVerb[d->kind & 15]);
        rect(8, 22 + shown * 12, 240, 11, shown & 1 ? C15(8, 12, 24) : C15(5, 9, 21)); text_fit(12, 24 + shown * 12, 1, 232, d->kind == GD_PASS ? C15(31, 14, 12) : d->kind == GD_AWARD ? C15(31, 25, 6) : TXT, s); shown++; }
    if (!shown) { text_fit(12, 60, 1, 232, TXT, "NOTHING WRITTEN YET."); text_fit(12, 76, 1, 232, C15(20, 24, 28), "HATCH AN EGG AND LIFE GOES IN THE DIARY."); } }
static void gtop_award(void) {
    panel(4, 4, 248, 184, "Awards"); char s[72];
    for (int i = 0; i < NACH; i++) { int x = 8 + (i & 1) * 122, y = 22 + (i >> 1) * 15, got = (gach >> i) & 1, sel = i == gsub; rect(x, y, 120, 14, sel ? C15(31, 25, 6) : PANB); rect(x + 1, y + 1, 118, 12, got ? C15(8, 22, 12) : PAN);
        text(x + 4, y + 3, 1, got ? C15(31, 31, 20) : C15(14, 18, 24), got ? "+" : "-"); text_fit(x + 14, y + 3, 1, 102, got ? TXT : C15(18, 22, 28), gachNm[i]); }
    rect(8, 144, 240, 24, C15(24, 16, 4)); sprintf(s, "%s: %s", gachNm[gsub % NACH], gachHow[gsub % NACH]); text_fit(12, 148, 1, 232, TXT, s); sprintf(s, "REWARD +$%d%s", 50 + gsub * 10, (gach >> gsub) & 1 ? "  (WON)" : ""); text(12, 158, 1, C15(31, 28, 12), s);
    sprintf(s, "CARE SCORE %d", gscore()); text(12, 173, 1, C15(31, 25, 6), s); }
static void gtop_shop(void) {
    panel(4, 4, 248, 184, "Boreagotchi shop"); char s[48]; Toy *t = gtoy_of(ctl);
    for (int i = 0; i < 8; i++) { int a = i + 1, y = 24 + i * 15, sel = i == gsub; rect(8, y, 240, 14, sel ? C15(31, 25, 6) : PANB); rect(9, y + 1, 238, 12, PAN);
        if (i < 7) { text(14, y + 3, 1, TXT, gaccNm[a]); if (t && (t->accs & (1 << a))) text(190, y + 3, 1, C15(20, 31, 20), "OWNED"); else { sprintf(s, "$%d", gaccCost[a]); text(190, y + 3, 1, C15(31, 25, 6), s); } }
        else { text(14, y + 3, 1, TXT, "A NEW EGG"); text(190, y + 3, 1, C15(31, 25, 6), "$150"); } }
    sprintf(s, "YOUR CASH $%d", funds); text(12, 150, 1, C15(31, 25, 6), s); text_fit(12, 162, 1, 232, C15(20, 24, 28), "A hat is yours for good. Wear it from Dress.");
    if (gmsg) text_fit(12, 174, 1, 232, C15(31, 12, 10), gmsg); }
static void gt_top(void) {
    scr = offbuf; char b[72]; rect(0, 0, 256, 192, K8(22, 12, 52));
    for (int i = 0; i < 24; i++) { int x = (i * 97 + 13) % 256, y = (i * 61 + 7) % 192; rect(x, y, ((ganim >> 3) + i) % 7 ? 1 : 2, ((ganim >> 3) + i) % 7 ? 1 : 2, C15(24, 24, 31)); }
    if (gview == GV_HOME) { gtop_home(); return; }
    panel(4, 4, 248, 184, "Boreagotchi");
    if (gview == GV_NONE) { text_fit(12, 40, 1, 228, TXT, "A Boreagotchi: a pet on a keychain."); text_fit(12, 56, 1, 228, TXT, "Eight creatures, 16 kinds, 64 forms."); text_fit(12, 72, 1, 228, TXT, "It grows up as you care for it."); sprintf(b, "The toy costs $%d.", GOTCHI_PRICE); text(12, 96, 1, C15(31, 25, 6), b); if (gmsg) text(12, 150, 1, C15(31, 12, 10), gmsg); return; }
    if (gview == GV_DEX) {
        sprintf(b, "BLOBDEX  %d OF %d   PAGE %d OF 4", gdex_count(), GSP_N * 4, gdexpage + 1); text(12, 26, 1, C15(31, 25, 6), b);
        for (int r = 0; r < 4; r++) { int sp = gdexpage * 4 + r, y = 36 + r * 34; text(10, y + 12, 1, TXT, gspNm[sp]);
            for (int f = 0; f < 4; f++) { int x = 70 + f * 46; rect(x - 1, y - 1, 45, 32, f == 0 ? C15(29, 22, 5) : f == 1 ? C15(18, 14, 8) : f == 2 ? C15(7, 23, 26) : C15(27, 8, 22)); rect(x, y, 43, 30, LCD_BG);
                if (gdex_has(sp, f)) { Blob bb; memset(&bb, 0, sizeof bb); bb.on = 1; bb.stage = GS_ADULT; bb.sp = (u8)sp; bb.form = (u8)f; bb.hun = bb.hap = 4; g_scene(&bb, 0, 0); gdots_draw(x + 5, y + 7, 1, LCD_INK); } else text(x + 18, y + 12, 1, C15(12, 14, 12), "?"); } }
        text(12, 174, 1, C15(20, 24, 28), "Care: SUPER, STAR. Snacks: CHILL."); return; }
    if (gview == GV_TREE) { gtop_tree(); return; }
    if (gview == GV_DIARY) { gtop_diary(); return; }
    if (gview == GV_AWARD) { gtop_award(); return; }
    if (gview == GV_SHOP) { gtop_shop(); return; }
    if (gview == GV_BOX) { Toy *bt = gtoy_of(gbx); int n = MIN(nsim, GT_MAX); if (gbx >= n) gbx = 0; bt = gtoy_of(gbx);
        text_fit(12, 28, 1, 232, C15(31, 25, 6), sname[gbx < nsim ? gbx : 0]);
        if (bt) { sprintf(b, "LV %d  %s", glevel(bt), glvNm[glevel(bt)]); text(12, 44, 1, TXT, b); sprintf(b, "%d pets   %d calling", gcount(bt), gcalling(bt)); text(12, 58, 1, gcalling(bt) ? C15(31, 14, 12) : C15(20, 28, 20), b); sprintf(b, "%d hatched   %d wins   %d playdates", bt->hatched, bt->wins, bt->plays); text_fit(12, 72, 1, 232, C15(20, 24, 28), b); }
        else text_fit(12, 44, 1, 232, C15(20, 24, 28), gbx == ctl ? "No toy yet: $5000 at a shop." : "No toy yet.");
        text(12, 96, 1, TXT, "Pets live on:"); text_fit(12, 108, 1, 232, C15(31, 25, 6), gclock_name());
        { int na = 0; for (int i = 0; i < NACH; i++) na += (gach >> i) & 1; sprintf(b, "CARE SCORE %d   AWARDS %d OF %d", gscore(), na, NACH); text_fit(12, 124, 1, 232, C15(31, 28, 12), b); }
        sprintf(b, "BLOBDEX %d OF %d", gdex_count(), GSP_N * 4); text(12, 140, 1, C15(31, 25, 6), b); rect(12, 154, 150, 5, C15(9, 7, 11)); rect(12, 154, 150 * gdex_count() / (GSP_N * 4), 5, C15(29, 22, 5));
        if (gmsg) text_fit(12, 168, 1, 232, C15(31, 12, 10), gmsg); return; }
    if (gview == GV_GUIDE) { const GPage *g = &gpg[gguide]; text_fit(12, 24, 1, 160, C15(31, 25, 6), g->title); sprintf(b, "PAGE %d OF %d", gguide + 1, GNP); text(176, 24, 1, C15(20, 24, 28), b);
        for (int i = 0; i < 3; i++) { rect(10, 44 + i * 44, 236, 38, K8(36, 16, 70)); rect(14, 54 + i * 44, 6, 6, C15(8, 20, 31)); gwrap(26, 48 + i * 44, 214, TXT, g->tip[i]); }
        for (int k = 0; k < GNP; k++) rect(128 - (GNP - 1) * 6 + k * 12 - 3, 176, 6, 6, k == gguide ? C15(8, 20, 31) : C15(9, 7, 11)); return; }
    Toy *t = gtt(); Blob *c = gcb(); Blob pv; const char *own = 0;
    if (gview == GV_FRIEND) { gfr_build(); if (gfr_n) { if (gsub >= gfr_n) gsub = 0; Toy *u = &gtoy[gfr_t[gsub]]; c = &u->b[gfr_s[gsub]]; if (u != t && gsim_of(u) >= 0) own = sname[gsim_of(u)]; } }
    if (gview == GV_DRESS) { pv = *c; pv.acc = (u8)gsub; c = &pv; }
    int dark = (c->fl & GF_SLEEP) && (c->fl & GF_DARK); u16 ink = dark ? C15(14, 18, 12) : gcolInk[c->col & 7];
    rect(14, 24, 228, 112, K8(26, 10, 40)); rect(18, 28, 220, 104, dark ? C15(8, 10, 8) : LCD_BG);
    int look = gview == GV_LOOK ? (gmN ? (gsidx ? 1 : -1) : 0) : 0, jump = 0;
    if (gview == GV_COPY) { int showing = gmT >= 0 && gmT < gmN * 40, k = showing ? gmT / 40 : -1; if (showing) { int m = gseq[k]; look = m == 2 ? -1 : m == 3 ? 1 : 0; jump = m == 0; } }
    if (gview == GV_STATS) { Blob *s = c; text(26, 34, 1, LCD_INK, "");
        if (gsub == 0) { text(26, 34, 1, LCD_INK, "HUNGER"); gpips(26, 48, s->hun, 4, LCD_INK); text(26, 66, 1, LCD_INK, "HAPPY"); gpips(26, 80, s->hap, 4, LCD_INK); text(140, 34, 1, LCD_INK, "BOND"); for (int i = 0; i < 4; i++) rect(140 + i * 10, 48, 8, 8, i < (s->bond + 12) / 25 ? C15(26, 4, 4) : C15(14, 17, 10)); sprintf(b, "%d GRAMS", s->wt); text(140, 66, 1, LCD_INK, b);
            sprintf(b, "LIKES %s", gfoodNm[(s->taste >> 4) % NFOOD]); text_fit(26, 98, 1, 200, LCD_INK, b); sprintf(b, "HATES %s", gfoodNm[(s->taste & 15) % NFOOD]); text_fit(26, 110, 1, 200, LCD_INK, b); }
        else { gname(s, b); text(26, 34, 1, LCD_INK, b); sprintf(b, "AGE %d HOURS", s->age / 60); text(26, 48, 1, LCD_INK, b); sprintf(b, "DISCIPLINE %d%%", s->disc); text(26, 62, 1, LCD_INK, b); sprintf(b, "MISTAKES %d  GEN %d", s->miss, s->gen + 1); text(26, 76, 1, LCD_INK, b); sprintf(b, "%s  %s%s", gpersNm[s->pers & 7], gcolNm[s->col & 7], s->mut ? "  SHINY" : ""); text(26, 90, 1, LCD_INK, b); sprintf(b, "YOU: %s  %d AWARDS", glvNm[glevel(t)], ({ int na = 0; for (int i = 0; i < NACH; i++) na += (gach >> i) & 1; na; })); text_fit(26, 104, 1, 206, LCD_INK, b);
            int nx = gnext_xp(t); if (nx) { rect(26, 118, 120, 5, C15(14, 17, 10)); rect(26, 118, 120 * t->xp / nx, 5, LCD_INK); } } }
    else if (gview == GV_CATCH) { memset(gdot, 0, sizeof gdot); gminY = 16; Blob g = *c; if (g.stage < GS_CHILD) g.stage = GS_CHILD; g_creature(&g, ganim / 8, 1, gcx);
        static const u8 pp[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 1, 0 }, { 1, 1, 1, 1 } };
        for (int i = 0; i < 3; i++) if (gcY[i] >= 0 && gcY[i] < 15) { if (gcK[i]) { for (int r = 0; r < 4; r++) for (int q = 0; q < 4; q++) if (pp[r][q]) gP(gcX[i] + q - 1, gcY[i] + r - 1, 1); } else gstar(gcX[i], gcY[i]); }
        gdots_draw(26, 32, 6, ink); }
    else if (gview == GV_HILO) { memset(gdot, 0, sizeof gdot); gminY = 16; Blob g = *c; if (g.stage < GS_CHILD) g.stage = GS_CHILD; g_creature(&g, ganim / 20, 1, -9); gdots_draw(26, 32, 6, ink);
        sprintf(b, "%d", ghiNum); rect(168, 36, 56, 28, K8(60, 90, 50)); text(190, 46, 2, TXT, b);
        if (ghiNext) { int ok = (ghiNext > ghiNum) == (ghiGuess > 0); sprintf(b, "%d", ghiNext); rect(168, 72, 56, 28, ok ? K8(40, 140, 60) : K8(170, 50, 50)); text(190, 82, 2, TXT, b); } }
    else { g_scene(c, ganim / 20 + (jump ? 1 : 0), look); gdots_draw(26, 32 - (jump ? 6 : 0), 6, ink); }
    if (o_gtime == 1) text(206, 30, 1, C15(26, 4, 4), "LIVE"); else if (o_gtime == 2) text(192, 30, 1, C15(12, 16, 12), "PAUSED");
    gname(c, b); { char b2[96]; if (own) sprintf(b2, "%s THE %s (OF %s)", ggiv[c->nm & 31], b, own); else sprintf(b2, "%s THE %s", ggiv[c->nm & 31], b); text_fit(128 - MIN(tw(1, b2), 240) / 2, 142, 1, 240, TXT, b2); }
    text(20, 156, 1, C15(31, 22, 8), "HUNGER"); gpips(66, 154, c->hun, 4, C15(31, 22, 8)); text(112, 156, 1, C15(31, 14, 24), "HAPPY"); gpips(152, 154, c->hap, 4, C15(31, 14, 24));
    for (int i = 0; i < 4; i++) rect(196 + i * 11, 154, 9, 7, i < (c->bond + 12) / 25 ? C15(31, 8, 8) : C15(9, 7, 11));
    { const char *w = c->stage == GS_EGG ? "AN EGG" : gneeds(c) ? gwant(c) : "ALL GOOD"; text(20, 172, 1, gneeds(c) ? C15(31, 14, 12) : C15(20, 28, 20), w); }
    if (gview == GV_LOOK) { sprintf(b, "ROUND %d  WINS %d", gmRound + 1, gmWon); text(140, 172, 1, C15(31, 31, 18), b); }
    if (gview == GV_COPY) { sprintf(b, "ROUND %d  GOT %d", gmRound + 1, gmWon); text(140, 172, 1, C15(31, 31, 18), b); }
    if (gview == GV_CATCH) { sprintf(b, "TREATS %d  MESS %d  %d LEFT", gcScore, gcBad, 14 - gcN); text_fit(112, 172, 1, 128, C15(31, 31, 18), b); }
    if (gview == GV_HILO) { sprintf(b, "ROUND %d  WINS %d", gmRound + 1, gmWon); text(140, 172, 1, C15(31, 31, 18), b); }
    if (gmsg || gview == GV_LOOK || gview == GV_COPY || gview == GV_CATCH || gview == GV_HILO || gview == GV_DRESS) { const char *m = gmsg; static char m2[48];
        if (gview == GV_LOOK && !gmsg) m = gmN == 0 ? "WHICH WAY WILL IT LOOK?" : ((gsidx ? 1 : -1) == gmDir ? "YES! IT LOOKED YOUR WAY" : "NOPE. THE OTHER WAY");
        if (gview == GV_COPY && !gmsg) { static const char *nm[4] = { "UP", "DOWN", "LEFT", "RIGHT" }; int showing = gmT >= 0 && gmT < gmN * 40; if (gmT < 0) m = "WATCH CLOSELY..."; else if (showing) m = nm[gseq[gmT / 40]]; else { sprintf(m2, "YOUR TURN: %d OF %d", gsidx + 1, gmN); m = m2; } }
        if (gview == GV_CATCH && !gmsg) m = gmT < 0 ? "GET READY..." : "SLIDE: CATCH THE STARS, DODGE THE MESS";
        if (gview == GV_HILO && !gmsg) m = ghiNext ? ((ghiNext > ghiNum) == (ghiGuess > 0) ? "RIGHT!" : "WRONG. NEXT ONE") : "WILL THE NEXT ONE BE HIGHER OR LOWER?";
        if (gview == GV_DRESS && !gmsg) { Toy *dt = gtt(); if (gsub == 0 || (dt->accs & (1 << gsub))) sprintf(m2, "%s: %s", gaccNm[gsub], gsub == gcb()->acc ? "WEARING IT" : "OWNED"); else sprintf(m2, "%s: $%d", gaccNm[gsub], gaccCost[gsub]); m = m2; }
        if (m) { rect(10, 124, 236, 14, K8(24, 10, 40)); text_fit(14, 128, 1, 228, TXT, m); } }
}
