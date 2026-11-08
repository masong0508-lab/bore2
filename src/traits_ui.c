// traits_ui: the trait and lifetime wish pickers of CREATE A BORE (inside CREATE.OVL, docs/TRAITS.md). It owns the rows of the PERSONALITY step.
// TRAITS tab: 600 traits in 15 categories (a strip of icons, LEFT / RIGHT), an ALL list, the PICKED ones, FOR YOU (they go with what is picked), RECENT,
// and the CHEATS (OPTIONS must have them on). SQUARE changes the list, TRIANGLE turns on a helper (jump by letter, or only one kind of effect).
// A trait that works against one already picked is greyed and says why. WISH tab: the lifetime wishes by category.
// update / draw / commit are called from create_ui.c through cas_pick.h. The data is the big table of traits_data.h (TR_BIG).
#define TR_BIG
#include "sim.h"
#include "look.h"
#include "gfx.h"
#include "ui.h"
#include "audio.h"
#include "cas_pick.h"
#include <psxpad.h>
#include <stdio.h>
#include <string.h>

extern Bore cas_edit;
#define E cas_edit

enum { V_CAT, V_ALL, V_PICK, V_SUGG, V_RECENT, V_CHEAT, V_N };
static const char *const vName[V_N] = { "BY CATEGORY", "ALL", "PICKED", "FOR YOU", "RECENT", "CHEATS" };
static const u8 vIcon[V_N] = { IC_TILES, IC_PEOPLE, IC_TICK, IC_STAR, IC_PLAY, IC_BOLT };
enum { H_NONE, H_LETTER, H_FILTER, H_N };
#define NCATS 15                 // the categories you can pick from (EARNED traits are won, not picked)
#define NROW 6
#define RH 12
static const u8 catIcon[TR_NCAT] = { TR_ICONS };

static u16 lst[TR_NREG + 2];     // the list on show (a trait id; 0: the RANDOM row)
static int nl, sel, top, view, tcat, ccat = -1, helper, filt = -1, wcat = -1, wsel, wtop, nw, hold, hdir, msgt;
static u8 wl[WISH_N + 1];
static const char *msg;

static const char *nameof(int id) { return tr_names + tr_nameoff[id]; }
static int maxpick(void) { return E.stage >= AG_ADULT ? 5 : 3; }
static int npick(void) { int n = 0; for (int i = 0; i < TR_PICK; i++) n += tr_ed_pick[i] != 0; return n; }
static int picked(int id) { for (int i = 0; i < TR_PICK; i++) if (tr_ed_pick[i] == id) return 1; return 0; }
static int clash(int a, int b) {   // two traits that cannot be had together: the same main effect, or one helps what the other hurts
    const u8 *x = tr_eff[a], *y = tr_eff[b];
    if (x[0] == y[0]) return 1;
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) if (x[i] && x[i] == y[j] && ((s8)x[3 + i] > 0) != ((s8)y[3 + j] > 0)) return 1;
    return 0;
}
static int clash_with(int id) {   // the id of a picked trait it clashes with (0: none)
    for (int i = 0; i < TR_PICK; i++) if (tr_ed_pick[i] && tr_ed_pick[i] != id && clash(id, tr_ed_pick[i])) return tr_ed_pick[i];
    return 0;
}
static int kind_ok(int id) { return filt < 0 || tr_hook_kind[tr_eff[id][0]] == filt; }
static void build(void) {
    nl = 0;
    if (view == V_CAT || view == V_ALL) { if (filt < 0) lst[nl++] = 0; for (int i = 0; i < TR_NREG; i++) { int id = tr_sorted[i]; if ((view == V_ALL || (tr_cat[id] & 15) == tcat) && kind_ok(id)) lst[nl++] = (u16)id; } }
    else if (view == V_PICK) { for (int i = 0; i < TR_PICK; i++) if (tr_ed_pick[i]) lst[nl++] = tr_ed_pick[i]; }
    else if (view == V_RECENT) { for (int i = 0; i < 8; i++) if (tr_recent[i] && (tr_recent[i] < TR_FIRST_CHEAT || tr_cheat_opt) && kind_ok(tr_recent[i])) lst[nl++] = tr_recent[i]; }
    else if (view == V_SUGG) {
        int n = npick(), cats = 0; for (int i = 0; i < TR_PICK; i++) if (tr_ed_pick[i] && tr_ed_pick[i] <= TR_NREG) cats |= 1 << (tr_cat[tr_ed_pick[i]] & 15);
        for (int i = 0; i < TR_NREG && nl < 120; i++) { int id = tr_sorted[i]; if (picked(id) || clash_with(id) || !kind_ok(id)) continue; if (n && !(cats >> (tr_cat[id] & 15) & 1)) continue; lst[nl++] = (u16)id; }
    } else if (tr_cheat_opt) { for (int i = 0; i < TR_NCHEAT; i++) { int id = tr_csorted[i]; if ((ccat < 0 || tr_cat[id] == ccat) && kind_ok(id)) lst[nl++] = (u16)id; } }
    if (sel >= nl) sel = nl ? nl - 1 : 0;
    if (top > sel) top = sel;
    if (top < sel - NROW + 1) top = sel - NROW + 1;
    if (top < 0) top = 0;
}
static void say(const char *s) { msg = s; msgt = 90; }
static void add_recent(int id) { int k = 7; for (int i = 0; i < 8; i++) if (tr_recent[i] == id) { k = i; break; } for (int i = k; i > 0; i--) tr_recent[i] = tr_recent[i - 1]; tr_recent[0] = (u16)id; }
static void remove_pick(int id) { int j = 0; for (int i = 0; i < TR_PICK; i++) if (tr_ed_pick[i] && tr_ed_pick[i] != id) tr_ed_pick[j++] = tr_ed_pick[i]; while (j < TR_PICK) tr_ed_pick[j++] = 0; }
static int add_pick(int id) {
    if (picked(id)) return 0;
    if (npick() >= maxpick()) { say("ALL SLOTS FULL. TAKE ONE OFF FIRST."); audio_voice(VOICE_NAH); return 0; }
    if (clash_with(id)) { say("THAT ONE CLASHES WITH ANOTHER PICK."); audio_voice(VOICE_NAH); return 0; }
    for (int i = 0; i < TR_PICK; i++) if (!tr_ed_pick[i]) { tr_ed_pick[i] = (u16)id; break; }
    add_recent(id); audio_voice(VOICE_AGREE); return 1;
}
static void roll(void) {   // RANDOM: fills the free slots with traits that go together; never a cheat
    int want = maxpick() == 5 ? 3 : 3;
    for (int k = 0; k < 200 && npick() < want; k++) { int id = 1 + (int)(look_rand() % TR_NREG); if (!picked(id) && !clash_with(id)) { for (int i = 0; i < TR_PICK; i++) if (!tr_ed_pick[i]) { tr_ed_pick[i] = (u16)id; break; } } }
    audio_voice(VOICE_LAUGH);
}
static void use(int id) {
    if (!id) { roll(); return; }
    if (picked(id)) { remove_pick(id); audio_voice(VOICE_AGREE); } else add_pick(id);
}
static void set_view(int v) { view = v; sel = top = 0; helper = H_NONE; build(); }

// ---- wishes ----
static void wbuild(void) { nw = 0; for (int w = 1; w <= WISH_N; w++) if (wcat < 0 || tr_wish_cat[w] == wcat) wl[nw++] = (u8)w; if (wsel >= nw) wsel = nw ? nw - 1 : 0; if (wtop > wsel) wtop = wsel; if (wtop < wsel - NROW + 1) wtop = wsel - NROW + 1; if (wtop < 0) wtop = 0; }

static void c_open(int inlife) {
    memset(tr_ed_pick, 0, sizeof tr_ed_pick); tr_ed_wish = 0; msgt = 0; view = V_CAT; tcat = 0; ccat = -1; helper = H_NONE; filt = -1; sel = top = wsel = wtop = 0; wcat = -1;
    if (inlife && nsims) { const TrState *t = sims[player].tr; for (int i = 0; i < TR_PICK; i++) tr_ed_pick[i] = t->tid[i]; tr_ed_wish = t->wish; }
    build(); wbuild();
}
static int rep(int held, int pr, int bit_dn, int bit_up) {   // a move with the pad, repeating while it is held
    int mv = !!(pr & bit_dn) - !!(pr & bit_up);
    if (!mv) { int h = !!(held & bit_dn) - !!(held & bit_up); if (h && h == hdir) { if (++hold > 14 && !(hold & 1)) mv = h; } else hold = 0; hdir = h; } else { hdir = mv; hold = 0; }
    return mv;
}
static void c_update(int tab, int held, int pr) {
    if (msgt > 0) msgt--;
    int mv = rep(held, pr, PAD_DOWN, PAD_UP), lr = !!(pr & PAD_RIGHT) - !!(pr & PAD_LEFT);
    if (tab == 1) {   // WISH
        if (lr) { wcat += lr; if (wcat < -1) wcat = NCATS - 1; if (wcat >= NCATS) wcat = -1; wsel = wtop = 0; wbuild(); audio_voice(VOICE_AGREE); }
        if (mv && nw) { wsel = (wsel + nw + mv) % nw; wbuild(); }
        if ((pr & PAD_CROSS) && nw) { tr_ed_wish = wl[wsel]; audio_voice(VOICE_YAHOO); }
        return;
    }
    if (pr & PAD_SQUARE) { int v = view; do v = (v + 1) % V_N; while (v == V_CHEAT && 0); set_view(v); audio_voice(VOICE_AGREE); return; }
    if (pr & PAD_TRIANGLE) { helper = (helper + 1) % H_N; audio_voice(VOICE_AGREE); say(helper == H_LETTER ? "UP AND DOWN JUMP TO THE NEXT LETTER. TRI AGAIN: FILTER." : helper == H_FILTER ? "UP AND DOWN CHOOSE A KIND OF EFFECT. TRI AGAIN: OFF." : "BACK TO NORMAL."); }
    if (lr) {
        if (view == V_CAT) { tcat = (tcat + NCATS + lr) % NCATS; sel = top = 0; build(); audio_voice(VOICE_AGREE); }
        else if (view == V_CHEAT) { ccat += lr; if (ccat < -1) ccat = 9; if (ccat > 9) ccat = -1; sel = top = 0; build(); audio_voice(VOICE_AGREE); }
        else if (view == V_ALL || view == V_SUGG || view == V_RECENT) { filt += lr; if (filt < -1) filt = TR_NKIND - 1; if (filt >= TR_NKIND) filt = -1; sel = top = 0; build(); audio_voice(VOICE_AGREE); }
    }
    if (mv && nl) {
        if (helper == H_LETTER && nl > 1) {   // to the next (or last) first letter
            int c = nameof(lst[sel] ? lst[sel] : lst[1 < nl ? 1 : 0])[0], i = sel;
            for (int k = 0; k < nl; k++) { i = (i + nl + mv) % nl; if (!lst[i]) continue; if (nameof(lst[i])[0] != c) break; }
            if (mv < 0) { int c2 = nameof(lst[i])[0]; while (i > 0 && lst[i - 1] && nameof(lst[i - 1])[0] == c2) i--; }   // (back: the first of that letter)
            sel = i;
        } else if (helper == H_FILTER) { filt += mv; if (filt < -1) filt = TR_NKIND - 1; if (filt >= TR_NKIND) filt = -1; sel = top = 0; build(); return; }
        else sel = (sel + nl + mv) % nl;
        build();
    }
    if ((pr & PAD_CROSS) && nl) { use(lst[sel]); if (view == V_PICK || view == V_SUGG) build(); }
}

// ---- drawing ----
static int fmt_eff(const u8 *e, char *b, int cap) {   // what a trait does, in words (e: hook x3, tier x3)
    int n = 0; b[0] = 0;
    for (int k = 0; k < 3; k++) {
        int h = e[k]; if (!h) continue; int m = (s8)e[3 + k], v = (m < 0 ? -m : m) * tr_unit[h];
        const char *t = m > 0 ? tr_hook_up[h] : tr_hook_dn[h];
        if (n && n < cap - 2) { b[n++] = '.'; b[n++] = ' '; }
        for (; *t && n < cap - 8; t++) { if (t[0] == '{' && t[1] == 'n' && t[2] == '}') { n += sprintf(b + n, "%d", v); t += 2; } else b[n++] = *t; }
    }
    b[n] = 0; return n;
}
static int fmt_desc(int id, char *b, int cap) { return fmt_eff(tr_eff[id], b, cap); }
static void strip(int x0, int y, int cur, int n, int red, const u8 *icons) {   // five round tabs, the one chosen big
    for (int j = -2; j <= 2; j++) {
        int i = (cur + n + j) % n, cx = x0 + 11 + (j + 2) * 24, cy = y + 8;
        if (j == 0) { ui_disc(cx, cy, 20, red ? UI_RED : UI_BLUE2); ui_disc(cx, cy, 16, red ? UI_RGB(244, 120, 104) : UI_BLUE); }
        else ui_disc(cx, cy, 15, UI_CARD2);
        if (icons) ui_icon(cx - 6, cy - 6, icons[i], j == 0 ? UI_WHITE : UI_SOFT);
        else { char b[2] = { tr_cheatsub[i][0], 0 }; ui_textc(cx, cy - 3, 1, b, j == 0 ? UI_WHITE : UI_SOFT); }
    }
}
static void pips(int x, int y) {
    int mx = maxpick();
    for (int i = 0; i < 5; i++) { int on = i < npick(), opt = i >= 3, avail = i < mx; u32 c = !avail ? UI_CARD : on ? UI_GREEN : opt ? UI_LINE : UI_CARD2;
        ui_round(x + i * 11, y, 9, 9, 3, c, c, 0); }
}
static void c_draw(int tab, int x, int y, int w, int h) {
    char b[96]; (void)h;
    if (tab == 1) {   // WISH
        strip(x, y, wcat < 0 ? 0 : wcat, NCATS, 0, catIcon);
        snprintf(b, sizeof b, "%s (%d)", wcat < 0 ? "ALL WISHES" : tr_catlong[wcat], nw); ui_textc(x + w / 2, y + 20, 1, b, UI_BLUE2);
        for (int i = wtop; i < nw && i < wtop + NROW; i++) { int ry = y + 32 + (i - wtop) * RH, s = i == wsel, id = wl[i];
            if (s) ui_round(x, ry, w, RH - 1, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
            ui_textfit(x + 4, ry + 2, w - 22, tr_wish_name[id], s ? UI_BLUE2 : UI_INK);
            if (tr_ed_wish == id) ui_icon(x + w - 15, ry, IC_TICK, UI_GREEN); }
        if (nw > NROW) { int th = NROW * RH * NROW / nw; if (th < 6) th = 6; gfx_rect(x + w + 1, y + 32, 2, NROW * RH, 200, 208, 220, 0); gfx_rect(x + w + 1, y + 32 + (NROW * RH - th) * wtop / (nw - NROW), 2, th, 38, 146, 222, 0); }
        if (nw) { int id = wl[wsel]; const WishDef *d = &tr_wish[id];
            ui_wrap(x + 2, y + 107, w - 4, 2, tr_wish_desc[id], UI_INK);
            snprintf(b, sizeof b, "GOAL %d: %s", d->thr, tr_ev_name[d->ev]); ui_textfit(x + 2, y + 128, w - 4, b, UI_SOFT);
            switch (d->rk) { case RK_CASH: snprintf(b, sizeof b, "PRIZE: $%d", d->ra); break; case RK_TRAIT: snprintf(b, sizeof b, "PRIZE: A NEW TRAIT"); break; case RK_OUTFIT: snprintf(b, sizeof b, "PRIZE: A NEW OUTFIT"); break;
                case RK_FOOD: snprintf(b, sizeof b, "PRIZE: %d MEALS", d->ra); break; case RK_BUS: snprintf(b, sizeof b, "PRIZE: BUS PASS, %d DAYS", d->ra); break; case RK_TOKENS: snprintf(b, sizeof b, "PRIZE: %d TOKENS", d->ra); break; default: snprintf(b, sizeof b, "PRIZE: %d WANT POINTS", d->ra); }
            ui_textfit(x + 2, y + 139, w - 4, b, UI_GOLDT); }
        return;
    }
    // TRAITS
    if (view == V_CAT) strip(x, y, tcat, NCATS, 0, catIcon);
    else if (view == V_CHEAT) strip(x, y, ccat < 0 ? 0 : ccat, 10, 1, 0);
    else strip(x, y, view, V_N, view == V_CHEAT, vIcon);
    { char k[24]; k[0] = 0; if (filt >= 0) snprintf(k, sizeof k, " %s", tr_kindname[filt]);
      if (view == V_CAT) snprintf(b, sizeof b, "%s%s (%d)", tr_catlong[tcat], k, nl - (filt < 0 ? 1 : 0));
      else if (view == V_CHEAT) snprintf(b, sizeof b, "%s%s (%d)", ccat < 0 ? "ALL CHEATS" : tr_cheatsub[ccat], k, nl);
      else snprintf(b, sizeof b, "%s%s (%d)", vName[view], k, nl); }
    ui_textc(x + w / 2, y + 20, 1, b, view == V_CHEAT ? UI_BADT : UI_BLUE2);
    int ly = y + 31;
    if (view == V_CHEAT && !tr_cheat_opt) {
        ui_round(x, ly, w, 70, 6, UI_PANEL, UI_CARD, 0);
        ui_wrapc(x + 6, ly + 8, w - 12, 5, "CHEAT TRAITS ARE OFF. SWITCH THEM ON IN OPTIONS, GAME PAGE. NO MEDALS OR HIGH SCORES WHILE THEY ARE ON.", UI_INK);
    } else if (!nl) {
        ui_round(x, ly, w, 40, 6, UI_PANEL, UI_CARD, 0);
        ui_wrapc(x + 6, ly + 8, w - 12, 3, view == V_PICK ? "NOTHING PICKED YET." : view == V_RECENT ? "NO TRAITS USED YET." : "NOTHING FITS.", UI_SOFT);
    }
    for (int i = top; i < nl && i < top + NROW; i++) {
        int id = lst[i], ry = ly + (i - top) * RH, s = i == sel;
        if (s) { ui_round(x, ry, w, RH - 1, 4, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(x, ry + 2, 2, RH - 5, 38, 146, 222, 0); }
        if (!id) { ui_text(x + 6, ry + 2, 1, "RANDOM TRAITS", s ? UI_BLUE2 : UI_INK); ui_icon(x + w - 15, ry, IC_DICE, s ? UI_BLUE : UI_LINE); continue; }
        int pk = picked(id), bad = !pk && clash_with(id), ch = id >= TR_FIRST_CHEAT;
        u32 c = pk ? UI_GOODT : bad ? UI_RGB(140, 150, 168) : ch ? UI_BADT : s ? UI_BLUE2 : UI_INK;
        ui_textfit(x + 6, ry + 2, w - 22, nameof(id), c);
        if (pk) ui_icon(x + w - 15, ry, IC_TICK, UI_GREEN); else if (bad) ui_icon(x + w - 15, ry, IC_LOCK, UI_LINE);
        if (ch && !pk && !bad) ui_icon(x + w - 15, ry, IC_BOLT, UI_RED);
    }
    if (nl > NROW) { int th = NROW * RH * NROW / nl; if (th < 6) th = 6; gfx_rect(x + w + 1, ly, 2, NROW * RH, 200, 208, 220, 0); gfx_rect(x + w + 1, ly + (NROW * RH - th) * top / (nl - NROW), 2, th, 38, 146, 222, 0); }
    // the slots, and what the chosen trait does
    int dy = ly + NROW * RH + 3;
    pips(x + 2, dy); snprintf(b, sizeof b, "%d OF %d", npick(), maxpick()); ui_text(x + 60, dy, 1, b, UI_SOFT);
    if (msgt > 0 && msg) { ui_wrap(x + 2, dy + 11, w - 4, 3, msg, UI_BADT); return; }
    if (nl && lst[sel]) {
        int id = lst[sel], o = clash_with(id);
        if (o && !picked(id)) { snprintf(b, sizeof b, "CLASHES WITH %s", nameof(o)); ui_wrap(x + 2, dy + 11, w - 4, 3, b, UI_BADT); }
        else { fmt_desc(id, b, sizeof b); ui_wrap(x + 2, dy + 11, w - 4, 3, b, UI_INK); }
    } else if (nl) ui_wrap(x + 2, dy + 11, w - 4, 3, "X PICKS A FEW GOOD ONES FOR YOU.", UI_SOFT);
}
static const char *c_sum(char *b) { int n = npick(); if (!n) return "NONE"; sprintf(b, "%d TRAITS", n); return b; }
static const char *c_hint(int tab) { return tab == 1 ? "X PICK|L1 R1 TAB" : "X PICK|SQ LIST|TRI A-Z"; }

// ---- keeping them ----
static void rebuild(TrState *t) {   // every effect again, from the ids
    memset(t->fx, 0, sizeof t->fx); memset(t->ch, 0, sizeof t->ch); memset(t->cm, 0, sizeof t->cm); t->f &= (u8)~(TF_CHEAT | TF_CHON);
    for (int k = 0; k < TR_SLOTS; k++) {
        int id = t->tid[k]; if (!id) continue;
        if (k < TR_PICK) tr_apply_id(t, id, tr_eff[id], (const s8 *)(tr_eff[id] + 3));
        else { int i = id >= TR_FIRST_RWD ? TR_NEARN + id - TR_FIRST_RWD : id - TR_FIRST_EARN; if (i >= 0 && i < TR_NEARN + TR_NRWD) tr_apply_id(t, id, tr_small[i], (const s8 *)(tr_small[i] + 3)); }
    }
}
static void c_commit(int inlife) {
    TrState *t;
    if (inlife && nsims) { if (sims[player].tr == &tr_null) sims[player].tr = tr_alloc(); t = sims[player].tr; }
    else { tr_stage_p = 0; t = tr_stage_p = tr_alloc(); tr_staged = 1; }
    if (t == &tr_null) return;
    int pn = !inlife || !nsims || sims[player].tr == t;   // (the names on the Life panel are the player's)
    for (int i = 0; i < TR_PICK; i++) { t->tid[i] = tr_ed_pick[i]; if (pn) { tr_pn[i][0] = 0; if (tr_ed_pick[i]) { strncpy(tr_pn[i], nameof(tr_ed_pick[i]), TR_NLEN - 1); tr_pn[i][TR_NLEN - 1] = 0; } } }
    rebuild(t);
    int w = tr_ed_wish ? tr_ed_wish : 1;
    if (w != t->wish) tr_wish_pick(t, w);
}
const CasPick tr_cas = { c_open, c_update, c_draw, c_commit, c_sum, c_hint };

// ---- the page on the Life panel's TRAITS tab (CREATE.OVL, open(2)): every trait the Bore has, what each does, the traits still to win, the wishes ----
static int pg_tab, pg_sel, pg_top, pg_n;
static u16 pg_id[TR_SLOTS];
static u8 seen_set[TR_NEARN / 8 + 1];
static const TrState *pg_t(void) { return sims[player].tr; }
static int small_i(int id) { return id >= TR_FIRST_RWD ? TR_NEARN + id - TR_FIRST_RWD : id - TR_FIRST_EARN; }
static int shown_has(int i) { return seen_set[i >> 3] >> (i & 7) & 1; }
static void seen_mark(int i) { seen_set[i >> 3] |= (u8)(1 << (i & 7)); }
void tr_page_open(void) {
    pg_tab = pg_sel = pg_top = 0; wcat = -1; wsel = wtop = 0; const TrState *t = pg_t(); pg_n = 0;
    for (int k = 0; k < TR_SLOTS; k++) if (t->tid[k]) pg_id[pg_n++] = t->tid[k];
    wbuild();
}
int tr_page_update(int held, int pr) {
    if (pr & PAD_CIRCLE) return 0;
    if (pr & (PAD_L1 | PAD_R1)) { pg_tab ^= 1; audio_voice(VOICE_AGREE); }
    int mv = rep(held, pr, PAD_DOWN, PAD_UP), lr = !!(pr & PAD_RIGHT) - !!(pr & PAD_LEFT);
    if (pg_tab == 0) { if (mv && pg_n) { pg_sel = (pg_sel + pg_n + mv) % pg_n; if (pg_top > pg_sel) pg_top = pg_sel; if (pg_top < pg_sel - 5) pg_top = pg_sel - 5; } }
    else {
        if (lr) { wcat += lr; if (wcat < -1) wcat = NCATS - 1; if (wcat >= NCATS) wcat = -1; wsel = wtop = 0; wbuild(); }
        if (mv && nw) { wsel = (wsel + nw + mv) % nw; wbuild(); }
        if ((pr & PAD_CROSS) && nw) { int w = wl[wsel]; if (tr_wdone[w >> 3] >> (w & 7) & 1) audio_voice(VOICE_NAH); else { tr_wish_pick(sims[player].tr, w); audio_voice(VOICE_YAHOO); } }
    }
    return 1;
}
static void prize(const WishDef *d, char *b, int n) {
    switch (d->rk) { case RK_CASH: snprintf(b, n, "PRIZE: $%d", d->ra); break; case RK_TRAIT: snprintf(b, n, "PRIZE: A NEW TRAIT"); break; case RK_OUTFIT: snprintf(b, n, "PRIZE: A NEW OUTFIT"); break;
        case RK_FOOD: snprintf(b, n, "PRIZE: %d MEALS", d->ra); break; case RK_BUS: snprintf(b, n, "PRIZE: BUS PASS, %d DAYS", d->ra); break; case RK_TOKENS: snprintf(b, n, "PRIZE: %d TOKENS", d->ra); break; default: snprintf(b, n, "PRIZE: %d WANT POINTS", d->ra); }
}
void tr_page_draw(void) {
    char b[96]; const TrState *t = pg_t();
    memset(seen_set, 0, sizeof seen_set);
    ui_backdrop();
    int top = ui_window(4, 4, 312, 232, "TRAITS AND WISHES", IC_STAR);
    for (int i = 0; i < 2; i++) { int on = i == pg_tab, x = 10 + i * 82; ui_round(x, top + 2, 78, 15, 6, on ? UI_RGB(70, 176, 238) : UI_PANEL, on ? UI_BLUE2 : UI_CARD2, 0); ui_textc(x + 39, top + 6, 1, i ? "WISH LIST" : "TRAITS", on ? UI_WHITE : UI_INK); }
    int y0 = top + 24;
    if (pg_tab == 0) {
        for (int i = pg_top; i < pg_n && i < pg_top + 6; i++) { int id = pg_id[i], y = y0 + (i - pg_top) * 14, s = i == pg_sel, ch = id >= TR_FIRST_CHEAT;
            if (s) ui_round(10, y, 138, 13, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
            ui_icon(12, y, id < TR_FIRST_EARN ? (ch ? IC_BOLT : IC_SMILE) : IC_STAR, ch ? UI_RED : id < TR_FIRST_EARN ? UI_BLUE : UI_AMBER);
            ui_textfit(28, y + 3, 116, nameof(id), ch ? UI_BADT : s ? UI_BLUE2 : UI_INK); }
        if (!pg_n) ui_wrapc(10, y0 + 10, 138, 4, "NO TRAITS YET. PICK THEM IN CREATE A BORE. DO THINGS MANY TIMES AND YOU WIN MORE.", UI_SOFT);
        if (pg_n) { int id = pg_id[pg_sel], ch = id >= TR_FIRST_CHEAT; ui_section(156, y0 - 2, 150, "WHAT IT DOES");
            if (id < TR_FIRST_EARN || ch) fmt_desc(id, b, sizeof b); else fmt_eff(tr_small[small_i(id)], b, sizeof b);
            ui_wrap(158, y0 + 12, 148, 5, b, UI_INK);
            if (id >= TR_FIRST_EARN && id < TR_FIRST_RWD) { const EarnDef *e = &tr_earn[id - TR_FIRST_EARN]; snprintf(b, sizeof b, "WON BY: %s, %d", tr_ev_name[e->ev], e->thr); ui_wrap(158, y0 + 62, 148, 2, b, UI_SOFT); }
            else if (id >= TR_FIRST_RWD && !ch) ui_text(158, y0 + 62, 1, "A LIFETIME WISH PRIZE", UI_GOLDT);
            else if (ch) ui_text(158, y0 + 62, 1, "CHEAT: NO MEDALS OR SCORES", UI_BADT); }
        ui_section(10, y0 + 82, 296, "NEXT TO WIN");
        for (int shown = 0; shown < 3; shown++) {   // the three earned traits nearest to being won
            int best = -1, bp = -1;
            for (int i = 0; i < TR_NEARN; i++) { if (!tr_sname(i)[0] || shown_has(i)) continue; int id = TR_FIRST_EARN + i, own = 0; for (int k = 0; k < TR_SLOTS; k++) own |= t->tid[k] == id; if (own) continue;
                int p = 100 * tr_ev_a[tr_earn[i].ev] / (tr_earn[i].thr ? tr_earn[i].thr : 1); if (p > bp) { bp = p; best = i; } }
            if (best < 0) break;
            { int y = y0 + 94 + shown * 24; const EarnDef *e = &tr_earn[best]; ui_textfit(10, y, 110, tr_sname(best), UI_INK); ui_bar(124, y + 2, 120, 7, MIN(100, bp), UI_GREEN);
              snprintf(b, sizeof b, "%d/%d", MIN(tr_ev_a[e->ev], e->thr), e->thr); ui_text(250, y, 1, b, UI_SOFT); ui_textfit(10, y + 12, 230, tr_ev_name[e->ev], UI_SOFT); }
            seen_mark(best);
        }
    } else {
        strip(10, y0 - 2, wcat < 0 ? 0 : wcat, NCATS, 0, catIcon);
        snprintf(b, sizeof b, "%s (%d)", wcat < 0 ? "ALL WISHES" : tr_catlong[wcat], nw); ui_textc(10 + 59, y0 + 18, 1, b, UI_BLUE2);
        for (int i = wtop; i < nw && i < wtop + 8; i++) { int w = wl[i], y = y0 + 30 + (i - wtop) * 14, s = i == wsel, dn = tr_wdone[w >> 3] >> (w & 7) & 1;
            if (s) ui_round(10, y, 118, 13, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
            ui_textfit(14, y + 3, 96, tr_wish_name[w], dn ? UI_GOODT : s ? UI_BLUE2 : UI_INK);
            if (dn) ui_icon(112, y, IC_TICK, UI_GREEN); else if (t->wish == w) ui_icon(112, y, IC_STAR, UI_AMBER); }
        if (nw) { int w = wl[wsel]; const WishDef *d = &tr_wish[w]; int dn = tr_wdone[w >> 3] >> (w & 7) & 1;
            ui_section(138, y0 - 2, 168, tr_wish_name[w]); ui_wrap(140, y0 + 12, 164, 3, tr_wish_desc[w], UI_INK);
            snprintf(b, sizeof b, "GOAL %d: %s", d->thr, tr_ev_name[d->ev]); ui_wrap(140, y0 + 46, 164, 2, b, UI_SOFT);
            prize(d, b, sizeof b); ui_textfit(140, y0 + 74, 164, b, UI_GOLDT);
            if (dn) ui_badge_i(140, y0 + 90, IC_TICK, "DONE", UI_GREEN, UI_WHITE);
            else { int need, pr; if (t->wish == w) { pr = tr_wish_prog(t, &need); ui_text(140, y0 + 90, 1, "YOUR WISH NOW", UI_GOLDT); } else { need = d->thr; pr = tr_ev_a[d->ev]; ui_text(140, y0 + 90, 1, "X CHOOSES THIS WISH", UI_BLUE2); }
                ui_bar(140, y0 + 104, 120, 8, MIN(100, 100 * pr / (need ? need : 1)), UI_GREEN); snprintf(b, sizeof b, "%d/%d", MIN(pr, need), need); ui_text(266, y0 + 103, 1, b, UI_INK); } }
    }
    ui_hints(160, top + 194, pg_tab ? "X CHOOSE|L1 R1 TAB|O BACK" : "UP DOWN LOOK|L1 R1 TAB|O BACK", 1);
}
