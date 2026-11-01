static int gback = GV_BASKET, gtreeback = GV_BASKET, gfr_t[GT_MAX * GB_N], gfr_s[GT_MAX * GB_N], gfr_n;   // (D55) where Back goes from the awards, home and shop, the tree's Back; the visit list (every hatched pet but this one: this toy's, then the housemates')
static int gcx, gcT, gcY[3], gcX[3], gcK[3], gcScore, gcBad, gcN, ghiNum, ghiGuess, ghiNext, ghd;   // CATCH and HIGH LOW; the pad's held keys
static const char *const groomNm[6] = { "SUNNY ROOM", "SEA ROOM", "BERRY ROOM", "MINT ROOM", "NIGHT ROOM", "GOLD ROOM" };
static const char *const gdecoNm[6] = { "WINDOW", "PLANT", "LAMP", "RUG", "PICTURES", "TOY BOX" };
static void gfr_build(void) {
    gfr_n = 0; for (int pass = 0; pass < 2; pass++) for (int k = 0; k < GT_MAX; k++) { Toy *u = &gtoy[k]; if (!u->own || gsim_of(u) < 0 || (pass == 0) != (k == gti)) continue;
        for (int i = 0; i < GB_N; i++) if (u->b[i].on && u->b[i].stage > GS_EGG && (k != gti || i != gbi)) { gfr_t[gfr_n] = k; gfr_s[gfr_n++] = i; } } }
static void gtree_sel(Toy *t, const Blob *b) { gtree = 0; if (b->on) for (int j = 0; j < GLN; j++) if (t->ln[j].id == b->id) gtree = j; }
static void gstart_catch(void) { gview = GV_CATCH; gcx = 0; gcScore = gcBad = gcN = gcT = 0; for (int i = 0; i < 3; i++) { gcY[i] = -i * 7; gcX[i] = 4 + grr() % 24; gcK[i] = grr() % 5 == 0; } gmT = -40; }
static void gstart_hilo(void) { gview = GV_HILO; gmRound = 0; gmWon = 0; ghiNum = 1 + grr() % 9; ghiGuess = 0; ghiNext = 0; gmT = 0; }
static void ghome_toggle(Toy *t) { if (gsim_of(t) != ctl) { gsay("THAT IS NOT YOUR HOME. YOU CAN ONLY LOOK"); return; } t->deco ^= (u8)(1 << gsub); gsay((t->deco >> gsub) & 1 ? "PUT IT OUT" : "PUT IT AWAY"); }
static void ghome_walls(Toy *t) { if (gsim_of(t) != ctl) { gsay("THAT IS NOT YOUR HOME. YOU CAN ONLY LOOK"); return; } t->room = (u8)((t->room + 1) % 6); gsay("NEW WALLS!"); }
static void gmore_go(int k) {   // the More page: 0 family home, 1 diary, 2 awards, 3 blobdex, 4 help, 5 shop
    gback = GV_MORE; switch (k) { case 0: gsub = 0; gview = GV_HOME; break; case 1: gview = GV_DIARY; break; case 2: gsub = 0; gview = GV_AWARD; break;
        case 3: gdexpage = 0; gview = GV_DEX; gdexback = GV_MORE; break; case 4: gview = GV_GUIDE; gguide = 0; gguideback = GV_MORE; break; default: gsub = 0; gview = GV_SHOP; } }
static void guse_icon(void) {   // the twelve icons: feed, play, light, meds, clean, scold / pet, stats, family, visit, dress, more
    Blob *b = gcb(); Toy *t = gtt(); int sl = (b->fl & GF_SLEEP) != 0;
    static const u8 okegg[12] = { 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1 }, oksleep[12] = { 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1 };
    if (b->stage == GS_EGG && !okegg[gisel]) { gsay("IT IS STILL AN EGG. GIVE IT A MINUTE"); return; }
    if (sl && !oksleep[gisel]) { gsay("SHHH. IT IS ASLEEP"); return; }
    switch (gisel) {
    case 0: gsub = 0; gview = GV_FEED; break;
    case 1: gsub = 0; gview = GV_PLAYPICK; break;
    case 2: gsay(glight(t, b)); break;
    case 3: gsay(gmedicine(t, b)); break;
    case 4: gsay(gclean(t, b)); break;
    case 5: gsay(gscold(t, b)); break;
    case 6: gsay(gneeds(b) ? gwant(b) : gnuzzle(t, b)); break;
    case 7: gsub = 0; gview = GV_STATS; break;
    case 8: gtree_sel(t, b); gtreeback = GV_HAND; gview = GV_TREE; break;
    case 9: gfr_build(); if (!gfr_n) { gsay("YOU NEED ANOTHER PET, HATCHED, TO PLAY WITH"); return; } gsub = 0; gview = GV_FRIEND; break;
    case 10: gsub = b->acc; gview = GV_DRESS; break;
    default: gsub = 0; gview = GV_MORE; break;
    }
}
static int gt_update(int pr, int steps) {   // 1 while it runs (gotchi_update)
    gt_live(steps); ganim += steps; if (gmsgT > 0) { gmsgT -= steps; if (gmsgT <= 0) { gmsgT = 0; gmsg = 0; } }
    switch (gview) {
    case GV_BOX: { int n = MIN(nsim, GT_MAX); if (n < 1) return 0;
        if (pr & (PD_U | PD_L)) gbx = (gbx + n - 1) % n; if (pr & (PD_D | PD_R)) gbx = (gbx + 1) % n; if (gbx >= n) gbx = 0;
        if (pr & PD_SQ) { gview = GV_DEX; gdexpage = 0; gdexback = GV_BOX; }
        if (pr & PD_SEL) { gview = GV_GUIDE; gguide = 0; gguideback = GV_BOX; }
        if (pr & PD_R1) { gview = GV_AWARD; gsub = 0; gback = GV_BOX; }
        if (pr & PD_ST) { gview = GV_SHOP; gsub = 0; gback = GV_BOX; }
        if (pr & PD_O) return 0;
        if (pr & PD_X) { if (!gtoy_of(gbx) && gbx == ctl) { if (funds < GOTCHI_PRICE) { gsay("A TOY COSTS $5000"); voice(V_NAH); } else if (gbuy(ctl)) { funds -= GOTCHI_PRICE; say("A Boreagotchi and its first egg!"); gt_open_basket(ctl); gfrom_box = 1; voice(V_YAHOO); } }
                          else { gt_open_basket(gbx); if (gview == GV_BASKET) gfrom_box = 1; } }
        if (pr & PD_L1) o_gtime = (o_gtime + 1) % 3;   // the clock: the game's, LIVE, off
        break; }
    case GV_GUIDE:
        if (pr & (PD_R1 | PD_R | PD_X)) gguide = (gguide + 1) % GNP; if (pr & (PD_L1 | PD_L)) gguide = (gguide + GNP - 1) % GNP;
        if (pr & PD_O) { if (gguideback < 0) return 0; gview = gguideback; }
        break;
    case GV_NONE: if (pr & PD_O) return 0; if (pr & PD_X) { if (funds < GOTCHI_PRICE) { gsay("A TOY COSTS $5000"); voice(V_NAH); } else if (gbuy(ctl)) { funds -= GOTCHI_PRICE; say("A Boreagotchi and its first egg!"); gt_open(); voice(V_YAHOO); } } break;
    case GV_BASKET: { Toy *t = gtt();
        if (pr & PD_L) gbi = (gbi + GB_N - 1) % GB_N; if (pr & PD_R) gbi = (gbi + 1) % GB_N; if (pr & PD_U) gbi = (gbi + GB_N - 4) % GB_N; if (pr & PD_D) gbi = (gbi + 4) % GB_N;
        t->sel = (u8)gbi;
        if (pr & PD_SQ) { gview = GV_DEX; gdexpage = 0; gdexback = GV_BASKET; }
        if (pr & PD_SEL) { gview = GV_GUIDE; gguide = 0; gguideback = GV_BASKET; }
        if (pr & PD_R1) { gtree_sel(t, &t->b[gbi]); gtreeback = GV_BASKET; gview = GV_TREE; }
        if (pr & PD_L1) { gsub = 0; gback = GV_BASKET; gview = GV_HOME; }
        if (pr & PD_O) { if (gfrom_box) gview = GV_BOX; else return 0; }
        if (pr & PD_X) { if (gsim_of(t) != ctl) { gsay("THAT IS NOT YOUR TOY. YOU CAN ONLY LOOK"); break; } if (!t->b[gbi].on) { if (funds < GOTCHI_EGG) { gsay("A NEW EGG COSTS $150"); voice(V_NAH); } else { funds -= GOTCHI_EGG; gegg(t, gbi); gsay("A NEW EGG! KEEP IT CLOSE"); voice(V_YAHOO); } } else { gview = GV_HAND; gisel = 0; gmsgT = 0; gmsg = 0; } }
        break; }
    case GV_HAND: { Toy *t = gtt();
        if (pr & PD_R) gisel = (gisel + 1) % 12; if (pr & PD_L) gisel = (gisel + 11) % 12; if (pr & (PD_U | PD_D)) gisel = (gisel + 6) % 12;
        if (pr & PD_R1) gbi = gliving(t, gbi, 1); if (pr & PD_L1) gbi = gliving(t, gbi, -1);
        t->sel = (u8)gbi;
        if (pr & PD_O) { gview = GV_BASKET; gmsgT = 0; gmsg = 0; }
        if (pr & PD_X) guse_icon();
        break; }
    case GV_FEED:
        if (pr & PD_R) gsub = (gsub + 1) % 13; if (pr & PD_L) gsub = (gsub + 12) % 13; if (pr & PD_D) gsub = (gsub + 4) % 13; if (pr & PD_U) gsub = (gsub + 9) % 13; if (pr & PD_L1) gsub = 12;
        if (pr & PD_O) { gview = GV_HAND; return 1; }
        if (pr & PD_X) { const char *m = gsub == 12 ? glunch(gtt(), gcb()) : gfeed(gtt(), gcb(), gsub, 1); gsay(m); gview = GV_HAND; voice(m[0] == 'Y' && m[1] == 'U' ? V_NAH : V_YAHOO); }
        break;
    case GV_PLAYPICK:
        if (pr & (PD_L | PD_U)) gsub = (gsub + 3) % 4; if (pr & (PD_R | PD_D)) gsub = (gsub + 1) % 4; if (pr & PD_O) gview = GV_HAND;
        if (pr & PD_X) { if (gsub == 0) { gview = GV_LOOK; gmRound = 0; gmWon = 0; gmT = 0; gmN = 0; gmDir = 0; } else if (gsub == 1) { gview = GV_COPY; gmRound = 0; gmWon = 0; gmN = 2; for (int i = 0; i < 6; i++) gseq[i] = (u8)(grr() & 3); gsidx = 0; gmT = -60; } else if (gsub == 2) gstart_catch(); else gstart_hilo(); }
        break;
    case GV_LOOK:   // WHICH WAY? it looks left or right: guess first. 5 rounds, 3 to win
        if (gmN == 0) { if (pr & PD_L) { gmDir = -1; gmN = 1; gmT = 0; } if (pr & PD_R) { gmDir = 1; gmN = 1; gmT = 0; } if (pr & PD_O) gview = GV_HAND; if (gmN) gsidx = (grr() & 1) ? 1 : 0; }
        else { gmT += steps; if (gmT > 50) { int way = gsidx ? 1 : -1; if (way == gmDir) gmWon++; gmRound++; gmN = 0; if (gmRound >= 5) gfinish_game(gmWon >= 3); } }
        break;
    case GV_COPY: {   // COPY ME: watch the blob, then do the same moves (the D-pad). Three rounds, two to win
        if (pr & PD_O) { gview = GV_HAND; break; }
        gmT += steps; if (gmT < 0) break; if (gmT < gmN * 40) break;
        int key = (pr & PD_U) ? 0 : (pr & PD_D) ? 1 : (pr & PD_L) ? 2 : (pr & PD_R) ? 3 : -1;
        if (key >= 0) { if (key == gseq[gsidx]) { gsidx++; voice(V_YEHA); if (gsidx >= gmN) { gmWon++; gmRound++; gsidx = 0; gmN++; gmT = -50; if (gmRound >= 3) gfinish_game(1); } }
                        else { gmRound++; gsidx = 0; gmT = -50; voice(V_NAH); if (gmRound - gmWon >= 2) gfinish_game(0); else if (gmRound >= 3) gfinish_game(gmWon >= 2); } }
        break; }
    case GV_CATCH: {   // CATCH: slide under the falling treats, dodge the mess. 14 treats, 8 to win
        if (pr & PD_O) { gview = GV_HAND; break; }
        gmT += steps; if (gmT < 0) break;
        if (ghd & PD_L) gcx = MAX(-12, gcx - steps); if (ghd & PD_R) gcx = MIN(12, gcx + steps);
        gcT += steps; while (gcT >= 7) { gcT -= 7;
            for (int i = 0; i < 3; i++) { gcY[i]++; if (gcY[i] == 12) { int dx = gcX[i] - (16 + gcx); if (dx >= -4 && dx <= 4) { if (gcK[i]) { gcBad++; voice(V_NAH); } else { gcScore++; voice(V_YEHA); } gcY[i] = 99; } }
                if (gcY[i] >= 15) { gcN++; gcY[i] = -(grr() % 6); gcX[i] = 4 + grr() % 24; gcK[i] = grr() % 5 == 0; } } }
        if (gcN >= 14) gfinish_game(gcScore >= 8 && gcBad < 3);
        break; }
    case GV_HILO:
        if (pr & PD_O) { gview = GV_HAND; break; }
        if (ghiNext) { gmT += steps; if (gmT > 55) { ghiNext = 0; ghiNum = 1 + grr() % 9; gmRound++; if (gmRound >= 5) gfinish_game(gmWon >= 3); } }
        else if (pr & (PD_U | PD_D)) { int nx = 1 + grr() % 9; if (nx == ghiNum) nx = nx % 9 + 1; ghiGuess = (pr & PD_U) ? 1 : -1; ghiNext = nx; gmT = 0; if ((nx > ghiNum) == (ghiGuess > 0)) { gmWon++; voice(V_YEHA); } else voice(V_NAH); }
        break;
    case GV_FRIEND: { gfr_build(); if (!gfr_n) { gview = GV_HAND; break; } if (gsub >= gfr_n) gsub = 0;
        if (pr & (PD_R | PD_D)) gsub = (gsub + 1) % gfr_n; if (pr & (PD_L | PD_U)) gsub = (gsub + gfr_n - 1) % gfr_n;
        if (pr & PD_O) gview = GV_HAND;
        if (pr & (PD_X | PD_SQ)) { const char *m = gmeet(gtt(), gbi, &gtoy[gfr_t[gsub]], gfr_s[gsub], (pr & PD_SQ) != 0); gview = GV_HAND; gsay(m); voice(V_YAHOO); }
        break; }
    case GV_STATS:
        if (pr & (PD_X | PD_R | PD_D)) { if (++gsub >= 2) gview = GV_HAND; }
        if (pr & (PD_O | PD_L)) { if (gsub > 0) gsub--; else gview = GV_HAND; }
        break;
    case GV_TREE: { Toy *t = gtt(); int d = (pr & (PD_R | PD_R1)) ? 1 : (pr & (PD_L | PD_L1)) ? -1 : 0;
        if (d) for (int k = 1; k <= GLN; k++) { int j = (gtree + d * k + GLN * 2) % GLN; if (t->ln[j].id) { gtree = j; break; } }
        if (pr & PD_O) gview = gtreeback;
        break; }
    case GV_DEX:
        if (pr & (PD_R1 | PD_R)) gdexpage = (gdexpage + 1) & 3; if (pr & (PD_L1 | PD_L)) gdexpage = (gdexpage + 3) & 3;
        if (pr & PD_O) { if (gdexback < 0) return 0; gview = gdexback; }
        break;
    case GV_DRESS:
        if (pr & PD_R) gsub = (gsub + 1) % NACC; if (pr & PD_L) gsub = (gsub + NACC - 1) % NACC;
        if (pr & PD_O) gview = GV_HAND;
        if (pr & PD_X) { const char *m = gdress(gtt(), gcb(), gsub); gsay(m); voice(m[0] == 'N' || m[0] == 'A' ? V_NAH : V_YAHOO); }
        break;
    case GV_MORE:
        if (pr & PD_R) gsub = (gsub + 1) % 6; if (pr & PD_L) gsub = (gsub + 5) % 6; if (pr & PD_D) gsub = (gsub + 2) % 6; if (pr & PD_U) gsub = (gsub + 4) % 6;
        if (pr & PD_O) gview = GV_HAND;
        if (pr & PD_X) gmore_go(gsub);
        break;
    case GV_HOME: { Toy *t = gtt();
        if (pr & PD_R) gsub = (gsub + 1) % 6; if (pr & PD_L) gsub = (gsub + 5) % 6; if (pr & (PD_U | PD_D)) gsub = (gsub + 3) % 6;
        if (pr & PD_X) ghome_toggle(t); if (pr & PD_SQ) ghome_walls(t);
        if (pr & PD_O) gview = gback;
        break; }
    case GV_DIARY: if (pr & PD_O) gview = gback; break;
    case GV_AWARD:
        if (pr & PD_R) gsub = (gsub + 1) % NACH; if (pr & PD_L) gsub = (gsub + NACH - 1) % NACH; if (pr & PD_D) gsub = (gsub + 2) % NACH; if (pr & PD_U) gsub = (gsub + NACH - 2) % NACH;
        if (pr & PD_O) gview = gback;
        break;
    case GV_SHOP: { Toy *t = gtoy_of(ctl);
        if (pr & PD_R) gsub = (gsub + 1) % 8; if (pr & PD_L) gsub = (gsub + 7) % 8; if (pr & (PD_U | PD_D)) gsub = (gsub + 4) % 8;
        if (pr & PD_O) { gview = gback; break; }
        if (pr & PD_X) { if (!t) { gsay("YOU NEED A BOREAGOTCHI FIRST"); break; }
            if (gsub < 7) { int a = gsub + 1; if (t->accs & (1 << a)) gsay("YOU OWN THAT ALREADY"); else if (funds < gaccCost[a]) { gsay("NOT ENOUGH CASH"); voice(V_NAH); } else { funds -= gaccCost[a]; t->accs |= (u8)(1 << a); gsay("BOUGHT! WEAR IT FROM DRESS"); voice(V_YAHOO); } }
            else { int s = -1; for (int i = 0; i < GB_N; i++) if (!t->b[i].on) { s = i; break; } if (s < 0) gsay("THE BASKET IS FULL"); else if (funds < GOTCHI_EGG) { gsay("A NEW EGG COSTS $150"); voice(V_NAH); } else { funds -= GOTCHI_EGG; gegg(t, s); gsay("A NEW EGG! KEEP IT CLOSE"); voice(V_YAHOO); } } }
        break; }
    }
    return 1;
}
