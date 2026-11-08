// ds_trui.h (D73): the trait picker and the lifetime wish picker of Create-a-Sim, and the traits page of the Life screen (a housemate's traits).
// Bottom screen: a full-screen picker (views, categories with counts, A-Z, effect filter, Random). Top screen: the trait's words. Every button is 24 px or more high.
// Included by main.c before cas_draw_step. The words are in plain Title Case: short, no capitals walls (the names in the data are in capitals).
#ifndef DS_TRUI_H
#define DS_TRUI_H
enum { TV_ALL, TV_PICKED, TV_FOR, TV_RECENT, TV_CHEAT, TV_N };
static const char *const TVN[TV_N] = { "All", "Picked", "For you", "Recent", "Cheats" };
static int tp_open, tp_view, tp_cat, tp_kind = -1, tp_az, tp_flt, tp_sel = -1, tp_top_i, tp_n, tp_cs, tp_pop, tp_wsel = -1, tp_wcat, tp_wn, tp_wtop;
static u16 tp_list[400]; static u8 tp_wl[WISH_N + 1]; static s16 tp_sc[TR_FIRST_RWD + TR_NRWD]; static const char *tp_hint = "";
#define TP_GREY C15(21, 24, 29)
static TrState *tp_T(void) { return &dtr[0]; }
static int tp_ns(void) { return tr_nslots(trf_stage(stage_of(sims[0].age))); }
static int tp_catof(int id) { return id >= TR_FIRST_CHEAT ? tr_cat[id] : id >= TR_FIRST_EARN ? 15 : (tr_cat[id] & 15); }
static int tp_has(int id) { const TrState *t = tp_T(); for (int k = 0; k < TR_SLOTS; k++) if (t->tid[k] == id) return k + 1; return 0; }
static int tp_slot_for(int id) { const TrState *t = tp_T(); int ns = tp_ns(); for (int k = id >= TR_FIRST_CHEAT ? TR_BASE : 0; k < ns && k < TR_SLOTS; k++) if (!t->tid[k]) return k; return -1; }
static int tp_used(void) { const TrState *t = tp_T(); int n = 0; for (int k = 0; k < TR_BASE; k++) n += t->tid[k] != 0; return n; }
static char tp_wb[40];
static const char *tp_why(int id) {   // 0: it can be picked (or taken off); else the plain reason it cannot
    if (tp_has(id)) return 0; const TrState *t = tp_T();
    if (id >= TR_FIRST_EARN && id < TR_FIRST_CHEAT) return "You win this one in play";
    if (id >= TR_FIRST_CHEAT && !tr_cheat_opt) return "Turn on Cheat traits in Options";
    if (tp_slot_for(id) < 0) return id >= TR_FIRST_CHEAT ? "Cheat traits go in slots 7 to 10" : "Slots are full. Tap a picked one";
    for (int k = 0; k < TR_SLOTS; k++) if (t->tid[k] && tr_clash(id, t->tid[k])) { char n[24]; trg_title(n, trg_tname(t->tid[k]), 24); sprintf(tp_wb, "Clashes with %s", n); return tp_wb; }
    return 0;
}
static int tp_kind_ok(int id) { if (tp_kind < 0) return 1; const u8 *e = trf_effp(id); for (int k = 0; k < 3; k++) if (e[k] && tr_hook_kind[e[k]] == tp_kind) return 1; return 0; }
static int tp_cat_ok(int id) { return !tp_cat || tp_catof(id) + 1 == tp_cat; }
static void tp_for_you(void) {   // the traits that suit this Sim's name best (the same pull that gives a housemate theirs)
    char f[20], l[20]; strcpy(f, cas_first); strcpy(l, cas_last); TrPers p; trf_pers_from_name(f, l, 4, &p); int cs[15];
    for (int c = 0; c < 15; c++) { int v = 0; for (int b = 0; b < 5; b++) v += tr_pk[c][b] * ((int)p.bar[b] - 5); v += 4 * tr_pasp[p.asp % 6][c] + 3 * tr_psign[(p.sign % 12) & 3][c]; cs[c] = v; }
    for (int i = 1; i <= TR_NREG; i++) tp_sc[i] = trf_on(i) ? (s16)(4 * cs[tp_catof(i) % 15] + (int)(trf_mix(p.seed, (u32)i) % 24u)) : (s16)-9999;
    tp_n = 0; for (int r = 0; r < 60; r++) { int b = 0, bv = -9000; for (int i = 1; i <= TR_NREG; i++) if (tp_sc[i] > bv && tp_cat_ok(i) && tp_kind_ok(i)) { b = i; bv = tp_sc[i]; } if (!b) break; tp_list[tp_n++] = (u16)b; tp_sc[b] = -9999; }
}
static void tp_build(void) {
    const TrState *t = tp_T(); tp_n = 0;
    if (tp_view == TV_PICKED) { for (int k = 0; k < TR_SLOTS; k++) { int id = t->tid[k]; if (id && tp_cat_ok(id) && tp_kind_ok(id)) tp_list[tp_n++] = (u16)id; } }
    else if (tp_view == TV_RECENT) { for (int k = 0; k < 8; k++) { int id = tr_recent[k]; if (id && id < TR_FIRST_EARN && trf_on(id) && tp_cat_ok(id) && tp_kind_ok(id)) tp_list[tp_n++] = (u16)id; } }
    else if (tp_view == TV_CHEAT) { for (int i = 0; i < TR_NCHEAT; i++) { int id = tr_csorted[i]; if (trf_on(id) && tp_cat_ok(id) && tp_kind_ok(id) && tp_n < 400) tp_list[tp_n++] = (u16)id; } }
    else if (tp_view == TV_FOR) tp_for_you();
    else if (tp_cat == 16) { for (int id = TR_FIRST_EARN; id < TR_FIRST_CHEAT; id++) if (trf_on(id) && tp_kind_ok(id) && tp_n < 400) tp_list[tp_n++] = (u16)id;
        for (int a = 1; a < tp_n; a++) { int v = tp_list[a], b = a - 1; while (b >= 0 && strcmp(trg_tname(tp_list[b]), trg_tname(v)) > 0) { tp_list[b + 1] = tp_list[b]; b--; } tp_list[b + 1] = (u16)v; } }
    else for (int i = 0; i < TR_NREG; i++) { int id = tr_sorted[i]; if (trf_on(id) && tp_cat_ok(id) && tp_kind_ok(id) && tp_n < 400) tp_list[tp_n++] = (u16)id; }
    if (tp_sel >= tp_n) tp_sel = -1; if (tp_top_i > tp_n - 4) tp_top_i = tp_n - 4; if (tp_top_i < 0) tp_top_i = 0;
}
static u8 tp_chid[20]; static u16 tp_chn[20]; static int tp_nch;
static void tp_chips(void) {   // the category chips: All, then every category that has traits on the DS, with its count
    tp_nch = 0; tp_chid[0] = 0; tp_chn[0] = 0; tp_nch = 1;
    if (tp_view == TV_CHEAT) { int tot = 0; for (int g = 0; g < 10; g++) { int n = 0; for (int i = 0; i < TR_NCHEAT; i++) if (trf_on(tr_csorted[i]) && tr_cat[tr_csorted[i]] == g) n++; if (n) { tp_chid[tp_nch] = (u8)(g + 1); tp_chn[tp_nch++] = (u16)n; tot += n; } } tp_chn[0] = (u16)tot; }
    else { int tot = 0; for (int c = 0; c < 16; c++) if (trf_catn[c]) { tp_chid[tp_nch] = (u8)(c + 1); tp_chn[tp_nch++] = trf_catn[c]; if (c < 15) tot += trf_catn[c]; } tp_chn[0] = (u16)tot; }
}
static const char *tp_chname(int i) { return tp_view == TV_CHEAT ? (tp_chid[i] ? tr_cheatsub[tp_chid[i] - 1] : "All") : (tp_chid[i] ? (tp_chid[i] == 16 ? "Earned" : tr_catname[tp_chid[i] - 1]) : "All"); }
static void tp_refresh(void) { tp_chips(); int ok = 0; for (int i = 0; i < tp_nch; i++) if (tp_chid[i] == tp_cat) ok = 1; if (!ok) tp_cat = 0; tp_build(); }
static void tp_recent_add(int id) { int k = 0; while (k < 7 && tr_recent[k] != id) k++; for (; k > 0; k--) tr_recent[k] = tr_recent[k - 1]; tr_recent[0] = (u16)id; }
static void tp_commit(void) { const TrState *t = tp_T(); for (int k = TR_SLOTS - 1; k >= 0; k--) if (t->tid[k] && t->tid[k] < TR_FIRST_CHEAT) tp_recent_add(t->tid[k]); }   // (what is picked now is the next time's RECENT)
static void tp_apply(void) { trf_rebuild(tp_T()); tp_T()->f = (u8)(tp_T()->f & ~TF_PEND); }
static void tp_toggle(int id) {
    TrState *t = tp_T(); int a = tp_has(id); if (a) { t->tid[a - 1] = 0; tp_apply(); return; }
    if (tp_why(id)) return;
    if (id >= TR_FIRST_CHEAT && !tr_cheat_ok) { tp_pop = id; return; }   // the first cheat trait asks once
    int s = tp_slot_for(id); if (s < 0) return; t->tid[s] = (u16)id; tp_apply();
}
static void tp_random(void) {   // fill the six normal slots from a random personality (cheat traits are kept)
    TrState *t = tp_T(); TrPers p; for (int b = 0; b < 5; b++) p.bar[b] = (u8)(cas_rand() % 11); p.sign = (u8)(cas_rand() % 12); p.asp = (u8)(cas_rand() % 6); p.stage = 3; p.seed = cas_rand();
    for (int k = 0; k < TR_BASE; k++) t->tid[k] = 0; trf_auto_pick(&p, 0, 0, t->tid, TR_BASE); for (int k = TR_BASE; k < TR_SLOTS; k++) if (t->tid[k] && t->tid[k] < TR_FIRST_CHEAT && 0) t->tid[k] = 0; tp_apply(); tp_build();
}
static void tp_wbuild(void) { tp_wn = 0; for (int w = 1; w <= WISH_N; w++) if (trf_won(w) && (!tp_wcat || tr_wish_cat[w] + 1 == tp_wcat)) tp_wl[tp_wn++] = (u8)w; if (tp_wsel >= tp_wn) tp_wsel = -1; if (tp_wtop > tp_wn - 4) tp_wtop = tp_wn - 4; if (tp_wtop < 0) tp_wtop = 0; }
static void tp_prize(int w, char *b, int mx) {
    const WishDef *d = &tr_wish[w]; char n[24];
    switch (d->rk) { case RK_CASH: snprintf(b, mx, "Prize: $%d", d->ra); break; case RK_TRAIT: trg_title(n, trg_tname(d->ra), 24); snprintf(b, mx, "Prize: trait %s", n); break; case RK_OUTFIT: snprintf(b, mx, "Prize: a new look"); break;
        case RK_FOOD: snprintf(b, mx, "Prize: $%d for food", d->ra * 40); break; case RK_BUS: snprintf(b, mx, "Prize: $%d for fares", d->ra * 30); break; case RK_TOKENS: snprintf(b, mx, "Prize: %d tickets", d->ra); break; default: snprintf(b, mx, "Prize: a star of wants"); }
}
static void tp_open_traits(void) { tp_open = 1; tp_pop = 0; tp_az = tp_flt = 0; tp_sel = -1; tp_top_i = 0; tp_cs = 0; if (tp_view == TV_CHEAT && !tr_cheat_opt) tp_view = TV_ALL; tp_refresh(); }
static void tp_open_wish(void) { tp_open = 2; tp_pop = 0; tp_wsel = -1; tp_wtop = 0; tp_cs = 0; tp_wcat = 0; tp_wbuild(); }
// ---- drawing ----
static void tp_btn(int x, int y, int w, int h, const char *t, int on, int fs) { cbtn(x, y, w, h, t, on, fs); }
static void tp_kindname(int k, char *b, int mx) { trg_title(b, tr_kindname[k], mx); }
static void tp_draw_rows(void) {
    for (int r = 0; r < 4; r++) { int y = 78 + r * 24, i = tp_top_i + r; if (i >= tp_n) break; int id = tp_list[i], on = tp_has(id) != 0, bad = !on && tp_why(id) != 0, sel = i == tp_sel; char n[28]; trg_title(n, trg_tname(id), 28);
        rect(2, y, 224, 22, sel ? C15(31, 31, 31) : PANB); rect(3, y + 1, 222, 20, on ? C15(8, 22, 10) : bad ? C15(11, 15, 22) : C15(8, 15, 27));
        if (on) { rect(7, y + 10, 2, 2, C15(31, 31, 31)); rect(9, y + 12, 2, 2, C15(31, 31, 31)); rect(11, y + 8, 2, 5, C15(31, 31, 31)); }
        text_box(16, y + 1, 150, 20, FS_BODY, bad ? TP_GREY : TXT, AL_L, n);
        if (id >= TR_FIRST_CHEAT) { rect(168, y + 5, 48, 12, C15(26, 6, 6)); text_box(168, y + 5, 48, 12, FS_SMALL, C15(31, 31, 31), AL_C, "CHEAT"); }
        else if (sel) text_box(160, y + 1, 62, 20, FS_SMALL, on ? C15(31, 26, 20) : bad ? TP_GREY : C15(24, 31, 24), AL_R, on ? "Tap: remove" : bad ? "Can't pick" : "Tap: pick");
        else { const u8 *e = trf_effp(id); char kn[16]; tp_kindname(tr_hook_kind[e[0]], kn, 16); text_box(150, y + 1, 72, 20, FS_SMALL, TP_GREY, AL_R, kn); } }
    if (!tp_n) text_box(4, 100, 220, 40, FS_BODY, TP_GREY, AL_C, tp_view == TV_PICKED ? "Nothing picked yet." : tp_view == TV_RECENT ? "No recent picks yet." : "Nothing here. Try All.");
    cbtn(228, 78, 28, 46, "^", tp_top_i > 0 ? 0 : -1, FS_HEAD); cbtn(228, 128, 28, 46, "v", tp_top_i + 4 < tp_n ? 0 : -1, FS_HEAD);
}
static void tp_draw_az(void) {
    for (int k = 0; k < 28; k++) { int x = 2 + (k % 7) * 32, y = 78 + (k / 7) * 24; if (k >= 26) { rect(x, y, 30, 22, PANB); continue; }
        int has = 0; for (int i = 0; i < tp_n; i++) { char c = trg_tname(tp_list[i])[0]; if (c == 'A' + k) { has = 1; break; } } char s[2] = { (char)('A' + k), 0 }; cbtn(x, y, 30, 22, s, has ? 0 : -1, FS_HEAD); }
}
static void tp_draw_flt(void) {
    for (int k = 0; k < 16; k++) { int x = 2 + (k % 4) * 56, y = 78 + (k / 4) * 24; char n[16]; if (k == 0) strcpy(n, "Any"); else tp_kindname(k - 1, n, 16); cbtn(x, y, 54, 22, n, (k == 0 ? tp_kind < 0 : tp_kind == k - 1) ? 1 : 0, FS_BODY); }
}
static void tp_draw_chips(int y, int sel_cat, int n, int (*idf)(int), const char *(*nf)(int), int (*cn)(int)) {
    cbtn(0, y, 24, 24, "<", tp_cs > 0 ? 0 : -1, FS_HEAD); cbtn(232, y, 24, 24, ">", tp_cs + 3 < n ? 0 : -1, FS_HEAD);
    for (int k = 0; k < 3; k++) { int i = tp_cs + k; if (i >= n) break; char t[24]; sprintf(t, "%s %d", nf(i), cn(i)); cbtn(26 + k * 69, y, 67, 24, t, idf(i) == sel_cat ? 1 : 0, FS_BODY); }
}
static int tpc_id(int i) { return tp_chid[i]; }
static const char *tpc_nm(int i) { static char b[16]; trg_title(b, tp_chname(i), 16); return b; }
static int tpc_n(int i) { return tp_chn[i]; }
static int tpw_ids[20], tpw_cn[20], tpw_n;
static void tpw_chips(void) { tpw_n = 1; tpw_ids[0] = 0; tpw_cn[0] = 0; for (int c = 0; c < 15; c++) { int n = 0; for (int w = 1; w <= WISH_N; w++) if (trf_won(w) && tr_wish_cat[w] == c) n++; if (n) { tpw_ids[tpw_n] = c + 1; tpw_cn[tpw_n++] = n; tpw_cn[0] += n; } } }
static int tpw_id(int i) { return tpw_ids[i]; }
static const char *tpw_nm(int i) { static char b[16]; trg_title(b, i ? tr_catname[tpw_ids[i] - 1] : "All", 16); return b; }
static int tpw_c(int i) { return tpw_cn[i]; }
static void tp_draw(void) {
    rect(0, 0, 256, 192, BG_L); char t[40];
    if (tp_open == 2) {
        tpw_chips(); tp_btn(2, 0, 42, 24, "Back", 0, FS_BODY); text_box(48, 0, 206, 24, FS_HEAD, TXT, AL_L, "Pick a life wish"); text_box(2, 26, 250, 24, FS_BODY, TP_GREY, AL_L, "One wish at a time. Done wishes unlock slots.");
        tp_draw_chips(52, tp_wcat, tpw_n, tpw_id, tpw_nm, tpw_c);
        for (int r = 0; r < 4; r++) { int y = 78 + r * 24, i = tp_wtop + r; if (i >= tp_wn) break; int w = tp_wl[i], on = tp_T()->wish == w, dn = tp_T()->wd[w >> 3] >> (w & 7) & 1, sel = i == tp_wsel; char n[28]; trg_title(n, tr_wish_name[w], 28);
            rect(2, y, 224, 22, sel ? C15(31, 31, 31) : PANB); rect(3, y + 1, 222, 20, on ? C15(8, 22, 10) : dn ? C15(11, 15, 22) : C15(8, 15, 27)); text_box(8, y + 1, 160, 20, FS_BODY, dn ? TP_GREY : TXT, AL_L, n);
            text_box(150, y + 1, 72, 20, FS_SMALL, on ? C15(31, 26, 20) : sel ? C15(24, 31, 24) : TP_GREY, AL_R, on ? "Your wish" : dn ? "Done" : sel ? "Tap: pick" : ""); }
        cbtn(228, 78, 28, 46, "^", tp_wtop > 0 ? 0 : -1, FS_HEAD); cbtn(228, 128, 28, 46, "v", tp_wtop + 4 < tp_wn ? 0 : -1, FS_HEAD);
        text_box(2, 176, 252, 14, FS_SMALL, TP_GREY, AL_C, "A: pick   B: back   L R: group"); return; }
    sprintf(t, "%d / %d", tp_used(), TR_BASE); tp_btn(2, 0, 42, 24, "Back", 0, FS_BODY); text_box(46, 0, 34, 24, FS_NUM, TXT, AL_C, t);
    tp_btn(82, 0, 48, 24, "Random", 0, FS_BODY); tp_btn(132, 0, 40, 24, "Clear", 0, FS_BODY); tp_btn(174, 0, 38, 24, "A-Z", tp_az, FS_BODY); tp_btn(214, 0, 40, 24, "Filter", tp_flt || tp_kind >= 0, FS_BODY);
    { int nv = tr_cheat_opt ? 5 : 4, w = 256 / nv; for (int v = 0; v < nv; v++) { int vv = v; cbtn(v * w + 1, 26, w - 2, 24, TVN[vv], tp_view == vv ? 1 : 0, FS_BODY); } }
    tp_draw_chips(52, tp_cat, tp_nch, tpc_id, tpc_nm, tpc_n);
    if (tp_az) tp_draw_az(); else if (tp_flt) tp_draw_flt(); else tp_draw_rows();
    if (tp_kind >= 0) { char k[16]; tp_kindname(tp_kind, k, 16); sprintf(t, "Showing: %s. Filter > Any to clear", k); tp_hint = t; }
    else if (tp_view == TV_CHEAT) tp_hint = "Cheat traits: no medals or high scores";
    else if (tr_unl < TR_NUNL) { static char hb[48]; char u[24]; trg_title(u, tr_unl_text[tr_unl], 24); sprintf(hb, "Next slot: %s", u); tp_hint = hb; }
    else tp_hint = "All 10 slots are open";
    text_box(2, 176, 252, 14, FS_SMALL, TP_GREY, AL_C, tp_hint);
    if (tp_pop) { rect(20, 40, 216, 112, PANB); rect(22, 42, 212, 108, C15(26, 29, 31)); rect(22, 42, 212, 20, C15(26, 6, 6)); text_box(22, 42, 212, 20, FS_HEAD, C15(31, 31, 31), AL_C, "Cheat traits");
        text_box(26, 66, 204, 14, FS_BODY, C15(2, 4, 12), AL_C, "They are very strong."); text_box(26, 80, 204, 14, FS_BODY, C15(2, 4, 12), AL_C, "No medals, high scores or awards"); text_box(26, 94, 204, 14, FS_BODY, C15(2, 4, 12), AL_C, "while one is on. Use it?");
        cbtn(30, 116, 90, 30, "Yes", 1, FS_HEAD); cbtn(134, 116, 90, 30, "No", 0, FS_HEAD); }
}
// ---- touch ----
static void tp_jump(char c) { for (int i = 0; i < tp_n; i++) if (trg_tname(tp_list[i])[0] >= c) { tp_top_i = i; if (tp_top_i > tp_n - 4) tp_top_i = tp_n - 4; if (tp_top_i < 0) tp_top_i = 0; return; } }
static void tp_chip_touch(int tx, int ty, int n, int *cs, int *cur, int (*idf)(int), void (*chg)(void)) {
    if (hit(0, 52, 24, 24, tx, ty) && *cs > 0) { (*cs)--; return; } if (hit(232, 52, 24, 24, tx, ty) && *cs + 3 < n) { (*cs)++; return; }
    for (int k = 0; k < 3; k++) if (*cs + k < n && hit(26 + k * 69, 52, 67, 24, tx, ty)) { *cur = idf(*cs + k); chg(); }
}
static void tp_chg_cat(void) { tp_top_i = 0; tp_sel = -1; tp_build(); }
static void tp_chg_wcat(void) { tp_wtop = 0; tp_wsel = -1; tp_wbuild(); }
static void tp_pick_wish(int w) { TrState *t = tp_T(); t->wish = (u8)w; t->wbase = 0; }
static int tp_touch(int tx, int ty) {   // 1: the picker is closed now
    if (tp_open == 2) {
        if (hit(2, 0, 42, 24, tx, ty)) { tp_open = 0; return 1; }
        tp_chip_touch(tx, ty, tpw_n, &tp_cs, &tp_wcat, tpw_id, tp_chg_wcat);
        if (hit(228, 78, 28, 46, tx, ty) && tp_wtop > 0) tp_wtop = tp_wtop > 4 ? tp_wtop - 4 : 0; if (hit(228, 128, 28, 46, tx, ty) && tp_wtop + 4 < tp_wn) tp_wtop += 4;
        for (int r = 0; r < 4; r++) if (tp_wtop + r < tp_wn && hit(2, 78 + r * 24, 224, 22, tx, ty)) { int i = tp_wtop + r; if (i == tp_wsel) { tp_pick_wish(tp_wl[i]); } else tp_wsel = i; }
        return 0; }
    if (tp_pop) { if (hit(30, 116, 90, 30, tx, ty)) { int id = tp_pop; tp_pop = 0; tr_cheat_ok = 1; tp_toggle(id); } else if (hit(134, 116, 90, 30, tx, ty)) tp_pop = 0; return 0; }
    if (hit(2, 0, 42, 24, tx, ty)) { tp_open = 0; return 1; }
    if (hit(82, 0, 48, 24, tx, ty)) { tp_random(); tp_build(); return 0; }
    if (hit(132, 0, 40, 24, tx, ty)) { TrState *t = tp_T(); for (int k = 0; k < TR_SLOTS; k++) t->tid[k] = 0; tp_apply(); tp_build(); return 0; }
    if (hit(174, 0, 38, 24, tx, ty)) { tp_az ^= 1; tp_flt = 0; return 0; }
    if (hit(214, 0, 40, 24, tx, ty)) { tp_flt ^= 1; tp_az = 0; return 0; }
    { int nv = tr_cheat_opt ? 5 : 4, w = 256 / nv; for (int v = 0; v < nv; v++) if (hit(v * w + 1, 26, w - 2, 24, tx, ty)) { if (tp_view != v) { tp_view = v; tp_cat = 0; tp_cs = 0; tp_top_i = 0; tp_sel = -1; tp_az = tp_flt = 0; tp_refresh(); } return 0; } }
    tp_chip_touch(tx, ty, tp_nch, &tp_cs, &tp_cat, tpc_id, tp_chg_cat);
    if (tp_az) { for (int k = 0; k < 26; k++) if (hit(2 + (k % 7) * 32, 78 + (k / 7) * 24, 30, 22, tx, ty)) { tp_jump((char)('A' + k)); tp_az = 0; } return 0; }
    if (tp_flt) { for (int k = 0; k < 16; k++) if (hit(2 + (k % 4) * 56, 78 + (k / 4) * 24, 54, 22, tx, ty)) { tp_kind = k - 1; tp_flt = 0; tp_top_i = 0; tp_sel = -1; tp_build(); } return 0; }
    if (hit(228, 78, 28, 46, tx, ty) && tp_top_i > 0) tp_top_i = tp_top_i > 4 ? tp_top_i - 4 : 0; if (hit(228, 128, 28, 46, tx, ty) && tp_top_i + 4 < tp_n) tp_top_i += 4;
    for (int r = 0; r < 4; r++) if (tp_top_i + r < tp_n && hit(2, 78 + r * 24, 224, 22, tx, ty)) { int i = tp_top_i + r; if (i == tp_sel) { int id = tp_list[i]; tp_toggle(id); if (tp_view == TV_PICKED) tp_build(); } else tp_sel = i; }
    return 0;
}
static void tp_keys(u32 down) {   // the pad does the same: Up Down, A, B, L R
    if (tp_open == 2) { if ((down & KEY_DOWN) && tp_wsel + 1 < tp_wn) { tp_wsel++; if (tp_wsel >= tp_wtop + 4) tp_wtop = tp_wsel - 3; } if ((down & KEY_UP) && tp_wsel > 0) { tp_wsel--; if (tp_wsel < tp_wtop) tp_wtop = tp_wsel; }
        if ((down & KEY_A) && tp_wsel >= 0) tp_pick_wish(tp_wl[tp_wsel]); if (down & KEY_B) tp_open = 0;
        if ((down & KEY_R) && tp_wcat < 15) { int i = 0; for (; i < tpw_n; i++) if (tpw_ids[i] == tp_wcat) break; if (i + 1 < tpw_n) { tp_wcat = tpw_ids[i + 1]; tp_chg_wcat(); } } if ((down & KEY_L) && tp_wcat) { int i = 0; for (; i < tpw_n; i++) if (tpw_ids[i] == tp_wcat) break; if (i > 0) { tp_wcat = tpw_ids[i - 1]; tp_chg_wcat(); } } return; }
    if (tp_pop) { if (down & KEY_A) { int id = tp_pop; tp_pop = 0; tr_cheat_ok = 1; tp_toggle(id); } if (down & KEY_B) tp_pop = 0; return; }
    if ((down & KEY_DOWN) && tp_sel + 1 < tp_n) { tp_sel++; if (tp_sel >= tp_top_i + 4) tp_top_i = tp_sel - 3; } if ((down & KEY_UP) && tp_sel > 0) { tp_sel--; if (tp_sel < tp_top_i) tp_top_i = tp_sel; }
    if ((down & KEY_UP) && tp_sel < 0 && tp_n) tp_sel = 0; if ((down & KEY_A) && tp_sel >= 0 && tp_sel < tp_n) { tp_toggle(tp_list[tp_sel]); if (tp_view == TV_PICKED) tp_build(); }
    if (down & KEY_B) { if (tp_az || tp_flt) tp_az = tp_flt = 0; else tp_open = 0; }
    if ((down & KEY_R) || (down & KEY_L)) { int i = 0; for (; i < tp_nch; i++) if (tp_chid[i] == tp_cat) break; i += (down & KEY_R) ? 1 : -1; if (i >= 0 && i < tp_nch) { tp_cat = tp_chid[i]; if (i >= tp_cs + 3) tp_cs = i - 2; if (i < tp_cs) tp_cs = i; tp_chg_cat(); } }
}
// ---- the slots page (Create-a-Sim > Traits > Traits) and the wish page ----
static const char *const TP_UNLS[TR_NUNL] = { "1 wish done", "3 wishes done", "Job level 5", "10 wishes done" };
static void tp_slot_name(int k, char *b, int mx, int *state) {   // state 0 open and empty, 1 picked, 2 locked, 3 a cheat slot with cheats off
    const TrState *t = tp_T(); int ns = tp_ns(); *state = 0; b[0] = 0;
    if (k >= ns) { *state = 2; strncpy(b, TP_UNLS[k - TR_BASE < TR_NUNL ? k - TR_BASE : 0], (size_t)mx - 1); b[mx - 1] = 0; return; }
    if (t->tid[k]) { *state = 1; trg_title(b, trg_tname(t->tid[k]), mx); return; }
    if (k >= TR_BASE && !tr_cheat_opt) { *state = 0; strcpy(b, "Open slot"); return; } strcpy(b, "Pick a trait");
}
static void tp_slots_draw(void) {
    for (int k = 0; k < TR_SLOTS; k++) { int x = 45 + (k >= 5 ? 105 : 0), y = 41 + (k % 5) * 24; char n[28]; int st; tp_slot_name(k, n, 28, &st);
        int cheat = st == 1 && tp_T()->tid[k] >= TR_FIRST_CHEAT;
        rect(x, y, 103, 22, PANB); rect(x + 1, y + 1, 101, 20, st == 1 ? (cheat ? C15(24, 8, 8) : C15(8, 22, 10)) : st == 2 ? C15(10, 12, 18) : C15(8, 15, 27));
        char nn[4]; sprintf(nn, "%d", k + 1); text_box(x + 3, y + 1, 14, 20, FS_NUM, st == 2 ? TP_GREY : TXT, AL_C, nn);
        if (st == 2) { text_box(x + 18, y, 84, 11, FS_SMALL, TP_GREY, AL_L, "Locked"); text_box(x + 18, y + 10, 84, 11, FS_SMALL, TP_GREY, AL_L, n); }
        else text_box(x + 18, y + 1, 84, 20, tw(FS_BODY, n) > 82 ? FS_SMALL : FS_BODY, st == 1 ? TXT : TP_GREY, AL_L, n); }
}
static int tp_slots_touch(int tx, int ty) { for (int k = 0; k < TR_SLOTS; k++) { int x = 45 + (k >= 5 ? 105 : 0), y = 41 + (k % 5) * 24; if (hit(x, y, 103, 22, tx, ty)) { if (k >= tp_ns()) { say(TP_UNLS[k - TR_BASE < TR_NUNL ? k - TR_BASE : 0]); return 1; } tp_open_traits(); tp_sel = -1; return 1; } } return 0; }
static void tp_wishpage_draw(void) {
    const TrState *t = tp_T(); int w = t->wish; char n[28], pz[40];
    if (!w) { text_box(45, 50, 206, 30, FS_HEAD, TP_GREY, AL_C, "No wish yet"); }
    else { trg_title(n, tr_wish_name[w], 28); text_box(45, 42, 206, 22, FS_HEAD, TXT, AL_C, n); char d[48]; trg_title(d, tr_wish_desc[w], 48); text_box(45, 66, 206, 14, FS_BODY, TXT, AL_C, d); tp_prize(w, pz, 40); text_box(45, 82, 206, 14, FS_BODY, C15(24, 31, 20), AL_C, pz); }
    { int nd = trg_wdone_n(); char s[40]; sprintf(s, "Wishes done: %d of %d", nd, trf_won(1) ? 46 : 0); text_box(45, 100, 206, 12, FS_SMALL, TP_GREY, AL_C, s); }
    cbtn(45, 116, 206, 28, w ? "Change wish" : "Pick a wish", 1, FS_HEAD);
}
static int tp_wishpage_touch(int tx, int ty) { if (hit(45, 116, 206, 28, tx, ty)) { tp_open_wish(); return 1; } return 0; }
// ---- the top screen: the slots and the words of the picked thing ----
static void tp_effects(int id, int x, int y, int w) {   // up to three effect lines, good in green, bad in red
    const u8 *e = trf_effp(id); for (int k = 0; k < 3; k++) { if (!e[k]) break; int tier = (s8)e[3 + k]; char b[64], l[72]; trg_fxtext(b, e[k], tier, 60); int good = (tier > 0) == (tr_hook_good[e[k]] != 0); sprintf(l, "%c %s", tier > 0 ? '+' : '-', b);
        text_box(x, y + k * 13, w, 13, tw(FS_BODY, l) > w ? FS_SMALL : FS_BODY, good ? C15(2, 14, 3) : C15(22, 2, 2), AL_L, l); }
}
static void tp_top(void) {   // hud is the screen: called after the name plate
    const TrState *t = tp_T(); int ns = tp_ns();
    for (int k = 0; k < TR_SLOTS; k++) { int x = k < 5 ? 2 : 194, y = 30 + (k % 5) * 17; char n[24]; int st; tp_slot_name(k, n, 24, &st);
        rect(x, y, 60, 15, PANB); rect(x + 1, y + 1, 58, 13, st == 1 ? (t->tid[k] >= TR_FIRST_CHEAT ? C15(24, 8, 8) : C15(8, 22, 10)) : C15(12, 14, 20));
        if (st == 2) strcpy(n, k - TR_BASE < TR_NUNL ? TP_UNLS[k - TR_BASE] : "Locked"); else if (st == 0) strcpy(n, "Open"); text_box(x + 1, y, 58, 15, FS_SMALL, st == 1 ? TXT : TP_GREY, AL_C, n); }
    (void)ns; int id = 0; if (tp_open == 1 && tp_sel >= 0 && tp_sel < tp_n) id = tp_list[tp_sel];
    if (tp_open == 2 && tp_wsel >= 0 && tp_wsel < tp_wn) { int w = tp_wl[tp_wsel]; char n[28], d[48], pz[40]; trg_title(n, tr_wish_name[w], 28); trg_title(d, tr_wish_desc[w], 48); tp_prize(w, pz, 40);
        panel(4, 112, 248, 76, 0); rect(6, 114, 244, 17, PANH); text_box(6, 114, 244, 17, FS_HEAD, TXT, AL_C, n); rect(6, 133, 244, 30, C15(27, 30, 31)); text_box(10, 134, 236, 14, FS_BODY, C15(2, 4, 12), AL_L, d); text_box(10, 148, 236, 14, FS_BODY, C15(2, 14, 3), AL_L, pz);
        { int p = 0, need = 0; if (tr_wish_prog && tp_T()->wish == w) p = tr_wish_prog(tp_T(), &need); rect(8, 170, 150, 10, C15(2, 6, 14)); if (need) rect(9, 171, 148 * p / need, 8, C15(8, 26, 8)); text_box(162, 168, 86, 14, FS_SMALL, C15(27, 30, 31), AL_L, need ? "Progress" : "Not started"); } return; }
    if (id) { char n[28], c[24]; trg_title(n, trg_tname(id), 28); trg_title(c, id >= TR_FIRST_CHEAT ? tr_cheatsub[tr_cat[id]] : tr_catlong[tp_catof(id)], 24); int ch = id >= TR_FIRST_CHEAT;
        panel(4, 110, 248, 78, 0); rect(6, 112, 244, 17, ch ? C15(26, 8, 8) : PANH); text_box(6, 112, 244, 17, FS_HEAD, ch ? C15(31, 31, 31) : TXT, AL_C, n);
        rect(6, 131, 244, 41, C15(27, 30, 31)); tp_effects(id, 10, 132, 236); const char *why = tp_why(id);
        text_box(8, 174, 176, 12, FS_SMALL, why || ch ? C15(31, 22, 20) : C15(24, 31, 26), AL_L, why ? why : ch ? "Cheat: no medals or high scores" : tp_has(id) ? "Picked. Tap it again to remove." : "Tap it again to pick it."); text_box(186, 174, 62, 12, FS_SMALL, C15(22, 28, 31), AL_R, c); }
    else if (tp_open == 0 && cas_sub == 0) { panel(4, 130, 248, 56, 0); text_box(8, 134, 240, 14, FS_BODY, TXT, AL_C, "Tap a slot to pick traits."); text_box(8, 150, 240, 14, FS_SMALL, TP_GREY, AL_C, "Slots 7 to 10 open as you play."); { char u[40]; if (tr_unl < TR_NUNL) { sprintf(u, "Next: %s", TP_UNLS[tr_unl]); text_box(8, 166, 240, 14, FS_SMALL, C15(31, 26, 20), AL_C, u); } } }
}
// ---- the Life screen's traits page: a housemate's ten slots (the top screen, info page 2) ----
static void tp_begin(void) { memset(dtr[0].tid, 0, sizeof dtr[0].tid); dtr[0].wish = 0; dtr[0].wbase = 0; trf_rebuild(&dtr[0]); tp_open = 0; tp_view = TV_ALL; tp_cat = 0; tp_kind = -1; tp_az = tp_flt = 0; tp_sel = -1; tp_pop = 0; tp_cs = 0; }
static void tp_random_wish(void) { int n = 0, pick = 0; for (int w = 1; w <= WISH_N; w++) if (trf_won(w)) { n++; if (cas_rand() % (u32)n == 0) pick = w; } tp_pick_wish(pick); }
static void trg_page(int i) {
    const TrState *t = &dtr[i]; int ns = tr_nslots(trf_stage(stage_of(sims[i].age))); char b[56];
    panel(6, 72, 244, 114, "Traits");
    for (int k = 0; k < TR_SLOTS; k++) { int x = 10 + (k >= 5 ? 122 : 0), y = 87 + (k % 5) * 14; const char *tx = ""; char n[26]; int col = TXT, st = 0;
        if (k >= ns) { st = 2; tx = TP_UNLS[k - TR_BASE < TR_NUNL ? k - TR_BASE : 0]; col = TP_GREY; } else if (t->tid[k]) { trg_title(n, trg_tname(t->tid[k]), 26); tx = n; if (t->tid[k] >= TR_FIRST_CHEAT) col = C15(28, 8, 8); } else { tx = "-"; col = TP_GREY; }
        sprintf(b, "%d", k + 1); text_box(x, y, 10, 12, FS_SMALL, TP_GREY, AL_R, b); text_box(x + 13, y, 106, 12, tw(FS_BODY, tx) > 104 ? FS_SMALL : FS_BODY, col, AL_L, tx); (void)st; }
    TrPers p; trg_pers(i, &p); int a = 0, c = 1; for (int q = 1; q < 5; q++) if (p.bar[q] > p.bar[a]) a = q; for (int q = 0; q < 5; q++) if (q != a && p.bar[q] >= p.bar[c == a ? 0 : c] ) c = q; (void)c;
    sprintf(b, "%s. Most %s", TRG_SIGN[p.sign % 12], TRG_BAR[a]); text_box(10, 158, 236, 12, FS_SMALL, C15(24, 28, 31), AL_L, b);
    if (t->wish) { int need = 0, pr = tr_wish_prog(t, &need); char n[28]; trg_title(n, tr_wish_name[t->wish], 28); sprintf(b, "Wish: %s", n); text_box(10, 170, 120, 12, FS_SMALL, C15(31, 31, 20), AL_L, b); rect(134, 172, 108, 8, C15(2, 6, 14)); if (need) rect(135, 173, 106 * pr / need, 6, C15(8, 26, 8)); }
    else if (t->f & TF_AUTO) text_box(10, 170, 160, 12, FS_SMALL, TP_GREY, AL_L, "No wish right now."); else text_box(10, 170, 160, 12, FS_SMALL, TP_GREY, AL_L, "No wish picked.");
    text_box(150, 170, 98, 12, FS_SMALL, TP_GREY, AL_R, "Pad: < > housemate");
}
// ---- (D76) the Life screen's wish page (info page 3): a housemate's wish, its prize and progress, and the wishes they have done ----
static void trg_wishpage(int i) {
    const TrState *t = &dtr[i]; char b[64], n[28]; int done = 0, tot = 0; panel(6, 72, 244, 114, "Wishes");
    for (int w = 1; w <= WISH_N; w++) if (trf_won(w)) { tot++; if ((t->wd[w >> 3] >> (w & 7)) & 1) done++; }
    if (t->wish) { int need = 0, pr = tr_wish_prog(t, &need); char d[48], pz[40]; trg_title(n, tr_wish_name[t->wish], 28); trg_title(d, tr_wish_desc[t->wish], 48); tp_prize(t->wish, pz, 40);
        text_box(10, 86, 236, 14, FS_HEAD, C15(31, 31, 20), AL_L, n); text_box(10, 101, 236, 12, FS_BODY, TXT, AL_L, d); text_box(10, 113, 236, 12, FS_BODY, C15(24, 31, 26), AL_L, pz);
        rect(10, 129, 150, 9, C15(2, 6, 14)); if (need) rect(11, 130, 148 * pr / need, 7, C15(8, 26, 8)); text_box(164, 127, 84, 12, FS_SMALL, TP_GREY, AL_L, need ? "Progress" : "Not started"); }
    else text_box(10, 90, 236, 14, FS_BODY, TP_GREY, AL_L, (t->f & TF_AUTO) ? "No wish right now." : "No wish picked.");
    sprintf(b, "Done: %d of %d", done, tot); text_box(10, 143, 120, 12, FS_SMALL, C15(24, 28, 31), AL_L, b);
    { int shown = 0; for (int w = WISH_N; w >= 1 && shown < 2; w--) if (trf_won(w) && ((t->wd[w >> 3] >> (w & 7)) & 1)) { trg_title(n, tr_wish_name[w], 28); text_box(10, 156 + shown * 12, 236, 12, FS_SMALL, TP_GREY, AL_L, n); shown++; } }
    text_box(150, 143, 98, 12, FS_SMALL, TP_GREY, AL_R, "Pad: < > housemate");
}
#endif
