static void button(int x, int y, int w, int h, const char *t, u16 c, int on) {
    rect(x, y, w, h, on ? C15(31, 31, 20) : C15(2, 2, 4)); rect(x + 1, y + 1, w - 2, h - 2, on ? c : shade(c, 4));
    text(x + 4, y + h / 2 - 2, 1, on ? C15(2, 2, 4) : C15(31, 31, 31), t);
}
static int hit(int x, int y, int w, int h, int tx, int ty) { return tx >= x && tx < x + w && ty >= y && ty < y + h; }
static void text_fit(int x, int y, int s, int w, u16 c, const char *t) {   // (D35) as much of t as fits in w pixels: the letters closer first, then cut short with a dot
    if (tw(s, t) <= w) { text(x, y, s, c, t); return; }
    t_tight = 1; if (tw(s, t) <= w) { text(x, y, s, c, t); t_tight = 0; return; }
    char b[40]; int n = (int)strlen(t); if (n > 38) n = 38; memcpy(b, t, (size_t)n); b[n] = 0;
    while (n > 1) { b[n - 1] = '.'; b[n] = 0; if (tw(s, b) <= w) break; n--; }
    text(x, y, s, c, b); t_tight = 0;
}
// ---- the shop (D35): the PS1's ten tabs, six cards a page, cheapest first, as the PS1 lists them ----
static u8 tab_id[NTAB][64]; static int tab_n[NTAB], bpage;
static void tabs_init(void) {
    for (int t = 0; t < NTAB; t++) tab_n[t] = 0;
    for (int i = 0; i < NITEM; i++) { int t = ITEMS[i].tab; if (t >= NTAB || tab_n[t] >= 64) continue; int k = tab_n[t]++;
        while (k > 0 && ITEMS[tab_id[t][k - 1]].price > ITEMS[i].price) { tab_id[t][k] = tab_id[t][k - 1]; k--; } tab_id[t][k] = (u8)i; }
}
static int tab_pages(void) { int p = (tab_n[cat] + 5) / 6; return p < 1 ? 1 : p; }
// a little picture of an item for the shop: its own boxes (the 3D model's), seen from the front and a little above
#define PVMAX 48
typedef struct { s16 x0, z0, x1, z1, h0, h1; u16 c, t; } PBox;
static PBox pv[PVMAX]; static int npv, pv_cap;
static void obj3d(int x, int y, const Item *it, int rot, int ghost);
static void draw_mini(int x, int y, int w, int h, int id) {
    const Item *it = &ITEMS[id]; pv_cap = 1; npv = 0; obj3d(0, 0, it, 0, 0); pv_cap = 0; if (!npv) return;
    int L = it->fl * 80, u0 = 1 << 30, u1 = -(1 << 30), v0 = 1 << 30, v1 = -(1 << 30), ord_[PVMAX];
    for (int k = 0; k < npv; k++) { const PBox *b = &pv[k];   // (across and depth as the 3D lot draws them, 68 a unit; depth at half; height 82 a unit)
        int xa = b->x0 * 68 / 10, xb = b->x1 * 68 / 10, df = (L - b->z1) * 34 / 10, db = (L - b->z0) * 34 / 10, ha = b->h0 * 82 / 10, hb = b->h1 * 82 / 10;
        if (xa + df < u0) u0 = xa + df; if (xb + db > u1) u1 = xb + db; if (ha + df < v0) v0 = ha + df; if (hb + db > v1) v1 = hb + db;
        int j = k; while (j > 0 && (pv[ord_[j - 1]].z1 > b->z1 || (pv[ord_[j - 1]].z1 == b->z1 && pv[ord_[j - 1]].h0 > b->h0))) { ord_[j] = ord_[j - 1]; j--; } ord_[j] = k; }   // back to front
    int du = u1 - u0 < 1 ? 1 : u1 - u0, dv = v1 - v0 < 1 ? 1 : v1 - v0, sc = w * 256 / du;
    if (h * 256 / dv < sc) sc = h * 256 / dv; if (sc > h * 256 / 700) sc = h * 256 / 700;   // (a phone stays small; a bed fills the frame)
    int ox = x + (w - du * sc / 256) / 2 - u0 * sc / 256, oy = y + h - (h - dv * sc / 256) / 2 + v0 * sc / 256;
    for (int q = 0; q < npv; q++) { const PBox *b = &pv[ord_[q]];
        int df = (L - b->z1) * 34 / 10, db = (L - b->z0) * 34 / 10, l = ox + (b->x0 * 68 / 10 + df) * sc / 256, r = ox + (b->x1 * 68 / 10 + df) * sc / 256;
        int bt = oy - (b->h0 * 82 / 10 + df) * sc / 256, tp = oy - (b->h1 * 82 / 10 + df) * sc / 256, D = (db - df) * sc / 256;
        if (r <= l) r = l + 1; if (bt <= tp) bt = tp + 1; u16 dk = shade(b->c, 6);
        for (int t = 0; t < D; t++) rect(r + t, tp - t, 1, bt - tp, dk);       // the right side
        for (int t = 0; t < D; t++) rect(l + t + 1, tp - t - 1, r - l, 1, b->t);  // the top
        rect(l, tp, r - l, bt - tp, b->c); }                                     // the front
}
static void age_txt(char *t, int mk) { int lo = 0, hi = 5; while (lo < 5 && !((mk >> lo) & 1)) lo++; while (hi > 0 && !((mk >> hi) & 1)) hi--; if (lo == 0 && hi == 5) strcpy(t, "Anyone"); else sprintf(t, "%.3s-%.3s", STGN[lo], STGN[hi]); }
#define PX 2                  // the plan view: 13 cells of 12 pixels
#define PY 22
static void plan(int ghost) {
    for (int y = 0; y < GN; y++) for (int x = 0; x < GN; x++) {
        rect(PX + x * 12, PY + y * 12, 12, 12, flr[y][x] ? FLOORC[flr[y][x] - 1] : (((x + y) & 1) ? C15(9, 18, 7) : C15(11, 20, 8)));
        rect(PX + x * 12, PY + y * 12, 12, 1, C15(0, 0, 0) | 0); }
    for (int y = 0; y <= GN; y++) for (int x = 0; x < GN; x++) if (hwl[y][x]) rect(PX + x * 12, PY + y * 12 - 1, 13, 3, WALLC[hwl[y][x] - 1]);
    for (int y = 0; y < GN; y++) for (int x = 0; x <= GN; x++) if (vwl[y][x]) rect(PX + x * 12 - 1, PY + y * 12, 3, 13, WALLC[vwl[y][x] - 1]);
    for (int i = 0; i < nobj; i++) { int w, h; obj_fp(&objs[i], &w, &h); rect(PX + objs[i].x * 12 + 2, PY + objs[i].y * 12 + 2, w * 12 - 4, h * 12 - 4, IT_OF(&objs[i])->col); }
    if (ghost) { int w = 1, h = 1; if (mode == M_BUY && pick >= 0) { const Item *it = &ITEMS[pick]; w = prot & 1 ? it->fl : it->fw; h = prot & 1 ? it->fw : it->fl; }
        int X = PX + pcx * 12, Y = PY + pcy * 12, W = w * 12, H = h * 12; rect(X, Y, W, 1, 0xFFFF); rect(X, Y + H - 1, W, 1, 0xFFFF); rect(X, Y, 1, H, 0xFFFF); rect(X + W - 1, Y, 1, H, 0xFFFF); }
}
static int drag, drag_card, dsx, dsy, drag_item, wall_val, move_obj = -1, ui_ver, last_tx, last_ty;
static const u16 BG_L = C15(18, 25, 31), PAN = C15(8, 16, 28), PANB = C15(2, 7, 20), PANH = C15(5, 12, 26), TXT = C15(31, 31, 31);
static void panel(int x, int y, int w, int h, const char *title); static void backdrop(void);
static void cat_icon(int x, int y, int i, u16 c) {   // a 12x12 pictogram for each shop tab
    switch (i) {
        case 0: rect(x + 2, y, 3, 9, c); rect(x + 2, y + 5, 9, 3, c); rect(x + 2, y + 8, 2, 4, c); rect(x + 9, y + 8, 2, 4, c); break;   // a chair
        case 1: rect(x, y + 3, 12, 3, c); rect(x + 1, y + 6, 2, 6, c); rect(x + 9, y + 6, 2, 6, c); break;                              // a table
        case 2: rect(x, y + 1, 12, 8, c); rect(x + 1, y + 2, 10, 6, C15(4, 8, 18)); rect(x + 5, y + 9, 2, 2, c); rect(x + 3, y + 11, 6, 1, c); break;   // a screen
        case 3: rect(x + 4, y + 8, 4, 4, c); rect(x + 3, y + 1, 6, 3, C15(8, 24, 8)); rect(x + 1, y + 3, 10, 3, C15(8, 24, 8)); rect(x + 5, y + 5, 2, 3, C15(8, 24, 8)); break;   // a plant
        case 7: rect(x + 1, y + 3, 10, 8, c); rect(x + 2, y + 1, 3, 2, c); rect(x + 7, y + 1, 3, 2, c); rect(x + 3, y + 6, 6, 3, C15(4, 4, 6)); break;   // a stove
        case 14: rect(x, y + 6, 12, 3, c); rect(x, y + 5, 2, 1, c); rect(x + 10, y + 5, 2, 1, c); rect(x + 2, y + 9, 2, 2, C15(4, 4, 6)); rect(x + 8, y + 9, 2, 2, C15(4, 4, 6)); break;   // a skateboard
        case 15: rect(x + 3, y, 6, 12, c); rect(x + 2, y + 2, 8, 9, c); rect(x + 4, y + 3, 4, 3, C15(10, 22, 12)); rect(x + 4, y + 8, 1, 1, C15(4, 8, 18)); rect(x + 7, y + 8, 1, 1, C15(4, 8, 18)); break;   // the toy egg
        case 16: rect(x + 3, y, 6, 7, C15(31, 10, 14)); rect(x + 2, y + 1, 8, 5, C15(31, 10, 14)); rect(x + 4, y + 1, 2, 2, C15(31, 24, 26)); rect(x + 5, y + 7, 1, 5, c); break;   // a balloon
        case 17: rect(x + 5, y, 2, 12, C15(31, 26, 6)); rect(x, y + 4, 12, 2, C15(31, 26, 6)); rect(x + 2, y + 2, 8, 7, C15(31, 26, 6)); rect(x + 3, y + 9, 2, 3, C15(31, 26, 6)); rect(x + 7, y + 9, 2, 3, C15(31, 26, 6)); break;   // a star
        default: rect(x + 5, y + 7, 2, 5, C15(14, 8, 4)); rect(x + 1, y, 10, 4, C15(6, 20, 6)); rect(x + 2, y + 3, 8, 5, C15(8, 24, 8)); break;   // a tree
    }
}
static const u8 TABICON[NTAB] = { 0, 1, 7, 2, 3, 13, 14, 15, 16, 17 };
static void coin(int x, int y) { rect(x + 1, y, 5, 7, C15(31, 25, 4)); rect(x, y + 1, 7, 5, C15(31, 25, 4)); rect(x + 2, y + 1, 3, 5, C15(31, 30, 12)); }
static void buy_card_pos(int k, int *x, int *y) { *x = 60 + (k & 1) * 98; *y = 33 + (k >> 1) * 44; }
static void draw_ui(void) {
    backdrop();
    const char *tn[4] = { "Map", "Live", "Buy", "Build" };
    rect(0, 0, 256, 18, C15(2, 4, 12));
    for (int i = 0; i < 4; i++) chip(i * 64 + 1, 1, 62, 15, tn[i], mode == i);
    char t[40];
    rect(0, 172, 160, 20, C15(2, 4, 12)); rect(1, 173, 158, 18, C15(5, 12, 26)); coin(5, 176); sprintf(t, "%d", funds); text(15, 177, 1, C15(31, 31, 20), t);
    clk(t, minutes % 1440); text(160 - tw(1, t) - 6, 177, 1, C15(24, 28, 31), t);
    if (msg_t > 0) { int w = tw(1, msg) + 12; rect(80 - w / 2 + 40, 160, w, 12, C15(2, 2, 4)); rect(80 - w / 2 + 41, 161, w - 2, 10, C15(26, 10, 6)); text(80 - w / 2 + 46, 163, 1, C15(31, 31, 31), msg); }
    { const char *cc[7] = { "<", ">", "^", "v", "+", "-", "Rst" }; for (int k = 0; k < 7; k++) { int cx = 164 + (k < 4 ? k * 22 : (k - 4) * 29), cy = k < 4 ? 164 : 178; rect(cx, cy, k < 4 ? 20 : 27, 12, C15(2, 4, 12)); rect(cx + 1, cy + 1, k < 4 ? 18 : 25, 10, C15(8, 16, 28)); text(cx + 4, cy + 2, 1, TXT, cc[k]); } }
    if (mode == M_BUY && pick < 0) {
        for (int k = 0; k < NTAB; k++) { int on = cat == k, y = 20 + k * 14; rect(2, y, 56, 14, on ? C15(31, 31, 31) : C15(2, 4, 12)); rect(3, y + 1, 54, 12, on ? C15(8, 26, 8) : C15(10, 14, 24)); rect(3, y + 1, 54, 1, on ? C15(20, 31, 20) : C15(16, 20, 30));
            cat_icon(5, y + 1, TABICON[k], on ? C15(31, 31, 31) : C15(24, 28, 31)); text_fit(19, y + 5, 1, 38, on ? C15(2, 6, 2) : TXT, TABS[k]); }
        int pages = tab_pages(); if (bpage >= pages) bpage = 0;
        rect(60, 19, 142, 12, C15(2, 4, 12)); text_fit(64, 22, 1, 104, TXT, TABN[cat]); sprintf(t, "%d/%d", bpage + 1, pages); text(198 - tw(1, t), 22, 1, C15(31, 31, 20), t);
        chip(204, 19, 24, 12, "<", 0); chip(230, 19, 24, 12, ">", 0);
        for (int k = 0; k < 6; k++) { int q = bpage * 6 + k; if (q >= tab_n[cat]) break; int id = tab_id[cat][q]; const Item *it = &ITEMS[id]; int x, y; buy_card_pos(k, &x, &y);
            int can = funds >= it->price, lift = drag == 1 && drag_item == id;
            rect(x, y, 96, 42, lift ? C15(31, 31, 20) : C15(2, 4, 12)); rect(x + 1, y + 1, 94, 40, can ? C15(28, 31, 31) : C15(18, 20, 24));
            rect(x + 1, y + 1, 94, 11, can ? C15(8, 18, 31) : C15(10, 12, 18)); text_fit(x + 4, y + 4, 1, 88, TXT, it->name);
            rect(x + 3, y + 14, 32, 25, C15(20, 26, 31)); rect(x + 6, y + 34, 26, 3, C15(14, 20, 27)); draw_mini(x + 4, y + 15, 30, 22, id);   // a little stage with a shadow
            rect(x + 38, y + 14, 55, 13, can ? C15(4, 18, 6) : C15(22, 6, 6)); rect(x + 39, y + 15, 53, 11, can ? C15(8, 24, 8) : C15(28, 10, 10)); coin(x + 41, y + 17); sprintf(t, "%d", it->price); text(x + 51, y + 18, 1, TXT, t);
            if (it->fw * it->fl > 1) { sprintf(t, "%dx%d", it->fw, it->fl); text(x + 38, y + 31, 1, C15(8, 12, 22), t); age_txt(t, it->mask); text_fit(x + 58, y + 31, 1, 36, C15(8, 12, 22), t); }
            else { age_txt(t, it->mask); text(x + 38, y + 31, 1, can ? C15(8, 12, 22) : C15(22, 6, 6), can ? t : "Too dear"); } }
    } else if (mode == M_BUY) {
        plan(1); const Item *it = &ITEMS[pick];
        rect(160, 20, 94, 140, C15(2, 4, 12)); rect(161, 21, 92, 138, C15(28, 31, 31)); rect(161, 21, 92, 14, C15(8, 18, 31)); text_fit(165, 25, 1, 86, TXT, it->name);
        rect(165, 38, 84, 42, C15(20, 26, 31)); rect(170, 72, 74, 4, C15(14, 20, 27)); draw_mini(167, 40, 80, 36, pick);
        coin(166, 84); sprintf(t, "%d", it->price); text(176, 85, 1, C15(8, 12, 22), t); if (it->fw * it->fl > 1) { sprintf(t, "%dx%d", prot & 1 ? it->fl : it->fw, prot & 1 ? it->fw : it->fl); text(224, 85, 1, C15(8, 12, 22), t); }
        chip(166, 96, 82, 20, "Place", 1); chip(166, 118, 82, 20, "Turn", 0); chip(166, 140, 82, 18, "Back", 0);
    } else if (mode == M_BUILD) {
        plan(1);
        static char rl[16], sl[16]; static const char *const RN[3] = { "None", "Flat", "Gable" }; sprintf(rl, "Roof: %s", RN[roof_st]); sprintf(sl, "Storeys: %d", up_st + 1);
        const char *tl2[7] = { "Floor", "Wall", "Erase", "Move", "Sell", rl, sl };
        for (int i = 0; i < 7; i++) button(164, 22 + i * 14, 88, 13, tl2[i], C15(24, 14, 8), tool == i);
        int n = tool == 0 ? 5 : tool == 1 ? 6 : 0;
        for (int i = 0; i < n; i++) { rect(164 + i * 15, 124, 14, 15, pal == i + 1 ? 0xFFFF : C15(2, 2, 4)); rect(166 + i * 15, 126, 10, 11, tool == 0 ? FLOORC[i] : WALLC[i]); }
        text(164, 144, 1, C15(20, 24, 28), tool == 1 ? "Drag along edges" : tool == 3 ? "Drag furniture" : tool == 4 ? "Tap furniture" : tool == 5 ? "Tap to change" : tool == 6 ? "Tap to add one" : "Drag to paint");
        if (tool >= 5) text(164, 154, 1, C15(20, 24, 28), "See Options>Roofs");
    }
}
static int lot_touch(int tx, int ty) {   // a stylus tap on the plan: the cell (or -1)
    if (tx < PX || ty < PY || tx >= PX + GN * 12 || ty >= PY + GN * 12) return -1;
    return ((ty - PY) / 12) * GN + (tx - PX) / 12;
}
static u8 *edge_at(int tx, int ty) {   // the wall slot nearest the stylus
    int c = lot_touch(tx, ty); if (c < 0) return 0; int x = c % GN, y = c / GN, fx = (tx - PX) % 12, fy = (ty - PY) % 12, d[4] = { fy, 11 - fy, fx, 11 - fx }, b = 0;
    for (int k = 1; k < 4; k++) if (d[k] < d[b]) b = k;
    return b == 0 ? &hwl[y][x] : b == 1 ? &hwl[y + 1][x] : b == 2 ? &vwl[y][x] : &vwl[y][x + 1];
}
static int fp_free(int x, int y, int w, int h, int skip) {   // (D35) every cell of a footprint inside the plot, and nothing else on it
    if (x < 0 || y < 0 || x + w > GN || y + h > GN) return 0;
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) { int o = obj_at(i, j); if (o >= 0 && o != skip) return 0; }
    return 1;
}
static void place_here(void) {
    const Item *it = &ITEMS[pick];
    if (pick == 48) { if (dog_have) say("You already have a dog"); else if (funds < it->price) say("Not enough funds"); else { funds -= it->price; dog_have = 1; dog_reset(); life_ver++; ui_ver++; say("Welcome home, Rex!"); life_ev(ctl, WN_BUY, 0); } pick = -1; return; }   // (the puppy is the dog, not a thing)
    int w = prot & 1 ? it->fl : it->fw, h = prot & 1 ? it->fw : it->fl;
    if (pcx + w > GN || pcy + h > GN) { say("Not enough room"); return; }
    if (!fp_free(pcx, pcy, w, h, -1)) { say("Something is there"); return; }
    if (nobj >= 400) { say("Lot is full"); return; }
    if (funds < it->price) { say("Not enough funds"); pick = -1; return; }
    funds -= it->price; objs[nobj].x = (u8)pcx; objs[nobj].y = (u8)pcy; objs[nobj].c = (u8)(pick >> 2); objs[nobj].i = (u8)(pick & 3); objs[nobj].rot = (u8)prot; nobj++; occ_ok = 0; say("Bought"); ui_ver++; life_ev(ctl, WN_BUY, 0);
}
static void build_apply(int c) {   // paint or erase one cell
    int x = c % GN, y = c / GN; pcx = x; pcy = y;
    if (tool == 0 && flr[y][x] != pal) { flr[y][x] = (u8)pal; ui_ver++; }
    else if (tool == 2) { flr[y][x] = 0; hwl[y][x] = 0; hwl[y + 1][x] = 0; vwl[y][x] = 0; vwl[y][x + 1] = 0; ui_ver++; }
}
static void ui_down(int tx, int ty) {
    last_tx = tx; last_ty = ty; drag = 0; drag_card = 0;
    for (int i = 0; i < 4; i++) if (hit(i * 64, 0, 62, 16, tx, ty)) { set_mode(i); pick = -1; return; }
    if (mode == M_BUY && pick < 0) {
        for (int k = 0; k < NTAB; k++) if (hit(2, 20 + k * 14, 56, 14, tx, ty)) { cat = k; bpage = 0; ui_ver++; }
        { int pages = tab_pages(); if (hit(204, 19, 24, 12, tx, ty)) { bpage = (bpage + pages - 1) % pages; ui_ver++; } if (hit(230, 19, 24, 12, tx, ty)) { bpage = (bpage + 1) % pages; ui_ver++; } }
        for (int k = 0; k < 6; k++) { int q = bpage * 6 + k, x, y; if (q >= tab_n[cat]) break; int id = tab_id[cat][q]; buy_card_pos(k, &x, &y);
            if (hit(x, y, 96, 42, tx, ty)) { if (funds >= ITEMS[id].price) { drag = 1; drag_item = id; dsx = tx; dsy = ty; } else say("Not enough funds"); } }
    } else if (mode == M_BUY) {
        int c = lot_touch(tx, ty);
        if (c >= 0) { pcx = c % GN; pcy = c / GN; drag = 2; return; }
        if (hit(166, 96, 82, 20, tx, ty)) place_here();
        if (hit(166, 118, 82, 20, tx, ty)) prot = (prot + 1) & 3;
        if (hit(166, 140, 82, 18, tx, ty)) pick = -1;
    } else if (mode == M_BUILD) {
        for (int i = 0; i < 7; i++) if (hit(164, 22 + i * 14, 88, 13, tx, ty)) { tool = i; if (pal < 1) pal = 1; if (tool == 1 && pal > 6) pal = 6; if (i == 5) roof_st = (roof_st + 1) % 3; if (i == 6) up_st = (up_st + 1) % 3; ui_ver++; }
        int n = tool == 0 ? 5 : tool == 1 ? 6 : 0;
        for (int i = 0; i < n; i++) if (hit(164 + i * 15, 124, 14, 15, tx, ty)) pal = i + 1;
        int c = lot_touch(tx, ty);
        if (c >= 0) { int x = c % GN, y = c / GN; pcx = x; pcy = y;
            if (tool == 0 || tool == 2) { drag = 3; build_apply(c); }
            else if (tool == 1) { u8 *e = edge_at(tx, ty); if (e) { wall_val = *e ? 0 : pal; *e = (u8)wall_val; drag = 4; ui_ver++; } }
            else if (tool == 3) { int o = obj_at(x, y); if (o >= 0) { drag = 5; move_obj = o; } }
            else if (tool == 4) { int o = obj_at(x, y); if (o >= 0) { funds += IT_OF(&objs[o])->price / 2; objs[o] = objs[--nobj]; occ_ok = 0; say("Sold"); ui_ver++; } } }
    }
}
static void ui_hold(int tx, int ty) {
    last_tx = tx; last_ty = ty;
    if (drag == 1) { int dx = tx - dsx, dy = ty - dsy; if (dx < 0) dx = -dx; if (dy < 0) dy = -dy; if (dx + dy > 6) { pick = drag_item; prot = 0; drag = 2; drag_card = 1; ui_ver++; } }
    else if (drag == 2) { int c = lot_touch(tx, ty); if (c >= 0 && (c % GN != pcx || c / GN != pcy)) { pcx = c % GN; pcy = c / GN; } }
    else if (drag == 3) { int c = lot_touch(tx, ty); if (c >= 0) build_apply(c); }
    else if (drag == 4) { u8 *e = edge_at(tx, ty); if (e && *e != wall_val) { *e = (u8)wall_val; ui_ver++; } }
    else if (drag == 5) { int c = lot_touch(tx, ty), w, h; if (c >= 0 && move_obj >= 0) { obj_fp(&objs[move_obj], &w, &h); if ((c % GN != objs[move_obj].x || c / GN != objs[move_obj].y) && fp_free(c % GN, c / GN, w, h, move_obj)) { objs[move_obj].x = (u8)(c % GN); objs[move_obj].y = (u8)(c / GN); occ_ok = 0; pcx = c % GN; pcy = c / GN; ui_ver++; } } }
}
static void ui_up(void) {
    if (drag == 1) { pick = drag_item; prot = 0; ui_ver++; }                                   // a tap on a card: pick it, then drag on the plan
    else if (drag == 2) { if (lot_touch(last_tx, last_ty) >= 0) place_here(); else if (drag_card) { pick = -1; ui_ver++; } }
    drag = 0; drag_card = 0; move_obj = -1;
}
static int ob_x, ob_z, ob_sx = 68, ob_sz = 68, ob_w = 80, ob_l = 80, ob_rot, ob_hs = 100;   // the item being drawn: its corner, its scale across and along (a 10th of a unit), its footprint (80 a cell), its turn
static const u8 HSC[28] = { 140, 140, 140, 140, 200, 200, 200, 200, 200, 200, 180, 200, 200, 100, 170, 200, 150, 200, 170, 200, 150, 100, 200, 150, 100, 200, 90, 100 };
static void ob2(int lx0, int lz0, int lx1, int lz1, int h0, int h1, u16 c, u16 top) {   // a part in cell units (0..80 a cell, front = +z), heights in tenths of a quarter-tile; sides c, top
    if (pv_cap) { if (npv < PVMAX && h1 > 0) { PBox *b = &pv[npv++]; b->x0 = (s16)lx0; b->z0 = (s16)lz0; b->x1 = (s16)lx1; b->z1 = (s16)lz1; b->h0 = (s16)(h0 * ob_hs / 100); b->h1 = (s16)(h1 * ob_hs / 100); b->c = c; b->t = top; } return; }   // (the shop's picture)
    int ax, az, bx2, bz;
    switch (ob_rot & 3) { case 1: ax = lz0; az = ob_w - lx0; bx2 = lz1; bz = ob_w - lx1; break; case 2: ax = ob_w - lx0; az = ob_l - lz0; bx2 = ob_w - lx1; bz = ob_l - lz1; break; case 3: ax = ob_l - lz0; az = lx0; bx2 = ob_l - lz1; bz = lx1; break; default: ax = lx0; az = lz0; bx2 = lx1; bz = lz1; }
    int X0 = ob_x + (ax < bx2 ? ax : bx2) * ob_sx / 10, X1 = ob_x + (ax < bx2 ? bx2 : ax) * ob_sx / 10, Z0 = ob_z + (az < bz ? az : bz) * ob_sz / 10, Z1 = ob_z + (az < bz ? bz : az) * ob_sz / 10;
    int hq = T / 4; block(X0, Z0, X1, Z1, -40 - h0 * hq / 10 * ob_hs / 100, -40 - h1 * hq / 10 * ob_hs / 100, c, top);
}
static void ob(int lx0, int lz0, int lx1, int lz1, int h0, int h1, u16 c) { ob2(lx0, lz0, lx1, lz1, h0, h1, c, shade(c, 10)); }
#define LC(r, g, b) C15(r, g, b)
static void obj3d(int x, int y, const Item *it, int rot, int ghost) {   // every item is a little model built from several boxes (coordinates 0..80 across the cell, heights in tenths of a quarter-tile)
    (void)ghost; int id = (int)(it - ITEMS); u16 c = it->col, d = shade(c, 6), l = shade(c, 10), wd = LC(16, 10, 5), dk = LC(3, 3, 4);
    { int ww = rot & 1 ? it->fl : it->fw, wl = rot & 1 ? it->fw : it->fl; ob_w = it->fw * 80; ob_l = it->fl * 80; ob_sx = (ww * 2 * T - T / 3) * 10 / (ww * 80); ob_sz = (wl * 2 * T - T / 3) * 10 / (wl * 80); }   // (a two-cell thing stretches to fill both)
    ob_x = tl(x * 2) + T / 6; ob_z = tl(y * 2) + T / 6; ob_rot = rot; ob_hs = id >= 0 && id < 28 ? HSC[id] : 100;
    if (it->nbox) {   // (D35) a PS1 model: its own boxes, converted by tools/dsbuild/mk_dsitems.py (PS1 units: 10 across, 7 up)
        if (!pv_cap && (!o_det || (mode == M_LIVE && (abs(ob_x - camx) + abs(ob_z - camz) > (o_dd == 0 ? 2600 : o_dd == 1 ? 3800 : 6500))))) { ob(8, 8, ob_w - 8, ob_l - 8, 0, it->h * 10 + 6, c); return; }
        if (o_shd && !pv_cap && it->kind != 1) { u16 sh = LC(6, 5, 4); ob_x -= 6; ob_z -= 4; ob(0, 0, ob_w, ob_l, -3, 0, sh); ob_x += 6; ob_z += 4; }
        for (int k = 0; k < it->nbox; k++) { const IBx *b = &IBOX[it->box0 + k]; ob2(b->a0 * 10, b->b0 * 10, b->a1 * 10, b->b1 * 10, b->z0 * 7, b->z1 * 7, b->side, b->top); }
        return; }
    if (!pv_cap && (!o_det || (mode == M_LIVE && (abs(ob_x - camx) + abs(ob_z - camz) > (o_dd == 0 ? 2600 : o_dd == 1 ? 3800 : 6500))))) { if (it->kind == 1) ob(0, 0, 80, 80, 0, 3, c); else ob(8, 8, 72, 72, 0, it->h * 10 + 6, c); return; }
    if (it->kind == 1) { ob(0, 0, 80, 80, 0, 2, d); ob(6, 6, 74, 74, 0, 4, c); ob(14, 14, 66, 66, 0, 5, l); return; }   // rug: border and centre
    if (o_shd && !pv_cap) { u16 sh = LC(6, 5, 4); ob_x -= 6; ob_z -= 4; ob(0, 0, 80, 80, -3, 0, sh); ob_x += 6; ob_z += 4; }   // a soft dark shadow on the floor
