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

static u16 lst[TR_NREG + 12];     // the list on show (a trait id; 0: the RANDOM row)
static int nl, sel, top, view, tcat, ccat = -1, helper, filt = -1, wcat = -1, wsel, wtop, nw, hold, hdir, msgt;
static u8 wl[WISH_N + 1];
static const char *msg;

static const char *nameof(int id) { return tr_names + tr_nameoff[id]; }
static int ro;   // 1: a living Bore whose traits are assigned from the personality: shown, not edited
static int maxpick(void) { return tr_nslots(E.stage); }
static int slot_free(int id) {   // the first open slot a trait may take (a cheat: only 7..10; -1 none)
    int ns = maxpick(); for (int i = id >= TR_FIRST_CHEAT ? TR_BASE : 0; i < ns && i < TR_SLOTS; i++) if (!tr_ed_pick[i]) return i; return -1; }
static int locked_text(int k, const char **t) {   // slot k is shut: why
    if (k < TR_BASE) { *t = "WHEN OLDER"; return 1; }
    if (E.stage < TR_UNL_STAGE || k - TR_BASE < tr_unl) { *t = "WHEN A TEEN"; return 1; }
    *t = tr_unl_text[k - TR_BASE]; return 1; }
static int npick(void) { int n = 0; for (int i = 0; i < TR_SLOTS; i++) n += tr_ed_pick[i] != 0; return n; }
static int picked(int id) { for (int i = 0; i < TR_PICK; i++) if (tr_ed_pick[i] == id) return 1; return 0; }
#define clash tr_clash   // (traits_auto.c: the same rule gives a family member their traits)
static int clash_with(int id) {   // the id of a picked trait it clashes with (0: none)
    for (int i = 0; i < TR_PICK; i++) if (tr_ed_pick[i] && tr_ed_pick[i] != id && clash(id, tr_ed_pick[i])) return tr_ed_pick[i];
    return 0;
}
static int kind_ok(int id) { return filt < 0 || tr_hook_kind[tr_eff[id][0]] == filt; }
static void build(void) {
    nl = 0;
    if (view == V_CAT || view == V_ALL) { if (filt < 0) lst[nl++] = 0; for (int i = 0; i < TR_NREG; i++) { int id = tr_sorted[i]; if ((view == V_ALL || (tr_cat[id] & 15) == tcat) && kind_ok(id)) lst[nl++] = (u16)id; } }
    else if (view == V_PICK) { for (int i = 0; i < TR_SLOTS; i++) lst[nl++] = (u16)(0x8000 | i); }   // (the ten slots, the shut ones with what opens them)
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
static int fixed(int id) { return id >= TR_FIRST_EARN && id < TR_FIRST_CHEAT; }   // a won trait stays
static void remove_pick(int id) { for (int i = 0; i < TR_SLOTS; i++) if (tr_ed_pick[i] == id && !fixed(id)) tr_ed_pick[i] = 0; }
static int add_pick(int id) {
    if (picked(id)) return 0;
    if (ro) { say("THEIR TRAITS COME FROM THEIR PERSONALITY."); audio_voice(VOICE_NAH); return 0; }
    if (id >= TR_FIRST_CHEAT && !tr_cheat_opt) { say("SWITCH CHEAT TRAITS ON IN OPTIONS FIRST."); audio_voice(VOICE_NAH); return 0; }
    int sl = slot_free(id);
    if (sl < 0) { say(id >= TR_FIRST_CHEAT ? "CHEATS NEED AN OPEN EXTRA SLOT: 7 TO 10." : "ALL SLOTS FULL. TAKE ONE OFF FIRST."); audio_voice(VOICE_NAH); return 0; }
    if (clash_with(id)) { say("THAT ONE CLASHES WITH ANOTHER PICK."); audio_voice(VOICE_NAH); return 0; }
    tr_ed_pick[sl] = (u16)id; add_recent(id); audio_voice(VOICE_AGREE); return 1;
}
static void roll(void) {   // RANDOM: fills the free normal slots with traits that go together; never a cheat
    if (ro) { say("THEIR TRAITS COME FROM THEIR PERSONALITY."); audio_voice(VOICE_NAH); return; }
    for (int k = 0; k < 400; k++) { int sl = slot_free(1); if (sl < 0 || sl >= TR_BASE) break; int id = 1 + (int)(look_rand() % TR_NREG); if (!picked(id) && !clash_with(id)) tr_ed_pick[sl] = (u16)id; }
    audio_voice(VOICE_LAUGH);
}
static void use(int id) {
    if (id & 0x8000) {   // a slot row
        int k = id & 15, ns = maxpick(); const char *tx;
        if (k >= ns) { locked_text(k, &tx); say(tx); audio_voice(VOICE_NAH); }
        else if (!tr_ed_pick[k]) say("EMPTY. PICK A TRAIT FROM A LIST.");
        else if (ro) { say("THEIR TRAITS COME FROM THEIR PERSONALITY."); audio_voice(VOICE_NAH); }
        else if (fixed(tr_ed_pick[k])) { say("A WON TRAIT STAYS FOR GOOD."); audio_voice(VOICE_NAH); }
        else { tr_ed_pick[k] = 0; audio_voice(VOICE_AGREE); }
        return; }
    if (!id) { roll(); return; }
    if (picked(id)) { remove_pick(id); audio_voice(VOICE_AGREE); } else add_pick(id);
}
static void set_view(int v) { view = v; sel = top = 0; helper = H_NONE; build(); }

// ---- wishes ----
static void wbuild(void) { nw = 0; for (int w = 1; w <= WISH_N; w++) if (wcat < 0 || tr_wish_cat[w] == wcat) wl[nw++] = (u8)w; if (wsel >= nw) wsel = nw ? nw - 1 : 0; if (wtop > wsel) wtop = wsel; if (wtop < wsel - NROW + 1) wtop = wsel - NROW + 1; if (wtop < 0) wtop = 0; }

static void c_open(int inlife) {
    memset(tr_ed_pick, 0, sizeof tr_ed_pick); tr_ed_wish = 0; msgt = 0; view = V_CAT; tcat = 0; ccat = -1; helper = H_NONE; filt = -1; sel = top = wsel = wtop = 0; wcat = -1;
    ro = 0; if (inlife && nsims) { const TrState *t = sims[player].tr; for (int i = 0; i < TR_SLOTS; i++) tr_ed_pick[i] = t->tid[i]; tr_ed_wish = t->wish; ro = (t->f & TF_AUTO) != 0; }
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
    if ((pr & PAD_TRIANGLE) && view != V_PICK) { helper = (helper + 1) % H_N; audio_voice(VOICE_AGREE); say(helper == H_LETTER ? "UP AND DOWN JUMP TO THE NEXT LETTER. TRI AGAIN: FILTER." : helper == H_FILTER ? "UP AND DOWN CHOOSE A KIND OF EFFECT. TRI AGAIN: OFF." : "BACK TO NORMAL."); }
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
static void pips(int x, int y) {   // ten slots: six always, four that open; green: used, grey: open, dark: shut
    int ns = maxpick();
    for (int i = 0; i < TR_SLOTS; i++) { int on = tr_ed_pick[i] != 0, avail = i < ns; u32 c = !avail ? UI_CARD : on ? (i >= TR_BASE ? UI_GOLDT : UI_GREEN) : UI_CARD2;
        ui_round(x + i * 9 + (i >= TR_BASE ? 3 : 0), y, 8, 8, 3, c, c, 0); }
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
        if (id & 0x8000) {   // a slot row
            int k = id & 15, tid = tr_ed_pick[k]; const char *tx;
            if (tid) { snprintf(b, sizeof b, "%d %s", k + 1, nameof(tid)); ui_textfit(x + 6, ry + 2, w - 22, b, tid >= TR_FIRST_CHEAT ? UI_BADT : s ? UI_BLUE2 : UI_INK); ui_icon(x + w - 15, ry, tid >= TR_FIRST_CHEAT ? IC_BOLT : tid >= TR_FIRST_EARN ? IC_STAR : IC_TICK, tid >= TR_FIRST_CHEAT ? UI_RED : tid >= TR_FIRST_EARN ? UI_AMBER : UI_GREEN); }
            else if (k < maxpick()) { snprintf(b, sizeof b, "%d EMPTY", k + 1); ui_textfit(x + 6, ry + 2, w - 22, b, UI_SOFT); }
            else { locked_text(k, &tx); snprintf(b, sizeof b, "%d %s", k + 1, tx); ui_textfit(x + 6, ry + 2, w - 22, b, UI_RGB(140, 150, 168)); ui_icon(x + w - 15, ry, IC_LOCK, UI_LINE); }
            continue; }
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
    pips(x + 2, dy); snprintf(b, sizeof b, "%d OF %d", npick(), maxpick()); ui_text(x + 2 + 10 * 9 + 8, dy, 1, b, UI_SOFT);
    if (msgt > 0 && msg) { ui_wrap(x + 2, dy + 11, w - 4, 3, msg, UI_BADT); return; }
    if (nl && (lst[sel] & 0x8000)) {   // a slot row: what is in it, or why it is shut
        int k = lst[sel] & 15, tid = tr_ed_pick[k]; const char *tx;
        if (tid) fmt_desc(tid, b, sizeof b); else if (k < maxpick()) snprintf(b, sizeof b, "SLOT %d IS OPEN. PICK A TRAIT FROM ANY LIST.", k + 1); else { locked_text(k, &tx); snprintf(b, sizeof b, "SLOT %d IS SHUT: %s.", k + 1, tx); }
        if (tid >= TR_FIRST_EARN && tid < TR_FIRST_CHEAT) { char c[80]; fmt_eff(tr_small[tid >= TR_FIRST_RWD ? TR_NEARN + tid - TR_FIRST_RWD : tid - TR_FIRST_EARN], c, sizeof c); snprintf(b, sizeof b, "WON. %s", c); }
        ui_wrap(x + 2, dy + 11, w - 4, 3, b, UI_INK);
    } else if (nl && lst[sel]) {
        int id = lst[sel], o = clash_with(id);
        if (o && !picked(id)) { snprintf(b, sizeof b, "CLASHES WITH %s", nameof(o)); ui_wrap(x + 2, dy + 11, w - 4, 3, b, UI_BADT); }
        else { fmt_desc(id, b, sizeof b); ui_wrap(x + 2, dy + 11, w - 4, 3, b, UI_INK); }
    } else if (nl) ui_wrap(x + 2, dy + 11, w - 4, 3, "X PICKS A FEW GOOD ONES FOR YOU.", UI_SOFT);
}
static const char *c_sum(char *b) { int n = npick(); if (!n) return "NONE"; sprintf(b, "%d OF %d TRAITS", n, maxpick()); return b; }
static const char *c_hint(int tab) { return tab == 1 ? "X PICK|L1 R1 TAB" : "X PICK|SQ LIST|TRI A-Z"; }

// ---- keeping them ----
static void set_names(TrState *t) { char (*nm)[TR_NLEN] = tr_names_of(t); for (int k = 0; k < TR_SLOTS; k++) { nm[k][0] = 0; if (t->tid[k]) { strncpy(nm[k], nameof(t->tid[k]), TR_NLEN - 1); nm[k][TR_NLEN - 1] = 0; } } }
static void c_commit(int inlife) {
    TrState *t;
    if (inlife && nsims) { if (sims[player].tr == &tr_null) sims[player].tr = tr_alloc(); t = sims[player].tr; }
    else { tr_stage_p = 0; t = tr_stage_p = tr_alloc(); tr_staged = 1; }
    if (t == &tr_null) return;
    tr_stage_set(t, E.stage);
    if (ro && inlife && nsims) {   // an assigned member re-made: their personality may have changed, so the traits are assigned again
        memset(t->tid, 0, sizeof t->tid); tr_rebuild(t); set_names(t); t->f |= TF_PEND; tr_pend_flag = 1; tr_pend_cool = 0; return; }
    int ns = maxpick();
    for (int i = 0; i < TR_SLOTS; i++) { t->tid[i] = i < ns || fixed(tr_ed_pick[i]) ? tr_ed_pick[i] : 0; if (t->tid[i] >= TR_FIRST_CHEAT && i < TR_BASE) t->tid[i] = 0; }
    t->f &= (u8)~TF_AUTO;
    tr_rebuild(t); set_names(t);
    int w = tr_ed_wish ? tr_ed_wish : 1;
    if (w != t->wish) tr_wish_pick(t, w);
}
const CasPick tr_cas = { c_open, c_update, c_draw, c_commit, c_sum, c_hint };

// ---- open(3): the household's traits, assigned from the personality (traits_auto.c). Runs once and leaves. ----
static const Sim *par_of(const Sim *s, int k) { int u = s->par[k]; if (!u) return 0; for (int i = 0; i < nsims; i++) if (sims[i].uid == u && sims[i].tr != &tr_null) return &sims[i]; return 0; }
void tr_auto_run(void) {
    static u16 inh[2 * TR_SLOTS];
    for (int pass = 0; pass < 2; pass++) for (int i = 0; i < nsims; i++) {   // pass 0: those with no parent waiting to be assigned; pass 1: the rest (a child takes after its parents)
        Sim *s = &sims[i]; TrState *t = s->tr; if (t == &tr_null || !(t->f & TF_PEND)) continue;
        const Sim *pa[2] = { par_of(s, 0), par_of(s, 1) };
        if (pass == 0 && ((pa[0] && (pa[0]->tr->f & TF_PEND)) || (pa[1] && (pa[1]->tr->f & TF_PEND)))) continue;
        int added = 0, had = 0; for (int k = 0; k < TR_SLOTS; k++) had += t->tid[k] != 0;
        if (t->f & TF_AUTO) {
            TrPers p; int np = (pa[0] != 0) + (pa[1] != 0);
            for (int b = 0; b < TR_N; b++) { int v = s->bore.tr[b], sum = 0; for (int q = 0; q < 2; q++) if (pa[q]) sum += pa[q]->bore.tr[b];
                p.bar[b] = (u8)(s->bore.stage < AG_ADULT && np ? (v * 2 + sum * 2 / np) / 4 : v); if (p.bar[b] > 10) p.bar[b] = 10; }
            p.sign = (u8)look_sign_of(&s->bore); p.asp = s->bore.asp; p.stage = s->bore.stage; p.seed = tr_name_seed(s->bore.first, s->bore.last);
            int ninh = 0; for (int q = 0; q < 2; q++) if (pa[q] && s->bore.stage < AG_ADULT) for (int k = 0; k < TR_SLOTS; k++) { int id = pa[q]->tr->tid[k]; if (id && id < TR_FIRST_EARN) inh[ninh++] = (u16)id; }
            added = tr_auto_pick(&p, inh, ninh, t->tid, tr_nslots(s->bore.stage));
            tr_rebuild(t);
        }
        set_names(t); t->f &= (u8)~TF_PEND;
        if (added && (day_count || clock_min > 8 * 60 + 30)) { char b[40]; if (had) snprintf(b, sizeof b, "%s GOT %s", s->bore.first, added > 1 ? "NEW TRAITS" : "A NEW TRAIT"); else snprintf(b, sizeof b, "%s GOT %d TRAITS", s->bore.first, added); sim_note(b); }
    }
    tr_pend_flag = 0;
}

// ---- the page on the Life panel's TRAITS tab (CREATE.OVL, open(2)): every trait the Bore has, what each does, the traits still to win, the wishes ----
static int pg_tab, pg_sel, pg_top, pg_n, pg_who;   // (pg_who: whose traits: anyone in the household)
static u8 seen_set[TR_NEARN / 8 + 1];
static const TrState *pg_t(void) { return sims[pg_who < nsims ? pg_who : player].tr; }
static int small_i(int id) { return id >= TR_FIRST_RWD ? TR_NEARN + id - TR_FIRST_RWD : id - TR_FIRST_EARN; }
static int shown_has(int i) { return seen_set[i >> 3] >> (i & 7) & 1; }
static void seen_mark(int i) { seen_set[i >> 3] |= (u8)(1 << (i & 7)); }
void tr_page_open(void) {
    pg_tab = pg_sel = pg_top = 0; wcat = -1; wsel = wtop = 0; pg_who = player; pg_n = TR_SLOTS; ro = 0;
    wbuild();
}
int tr_page_update(int held, int pr) {
    if (pr & PAD_CIRCLE) return 0;
    if (pr & (PAD_L1 | PAD_R1)) { pg_tab ^= 1; audio_voice(VOICE_AGREE); if (pg_tab) pg_who = player; }
    int mv = rep(held, pr, PAD_DOWN, PAD_UP), lr = !!(pr & PAD_RIGHT) - !!(pr & PAD_LEFT);
    if (pg_tab == 0) {
        if (mv) { pg_sel = (pg_sel + pg_n + mv) % pg_n; if (pg_top > pg_sel) pg_top = pg_sel; if (pg_top < pg_sel - 5) pg_top = pg_sel - 5; }
        if (lr && nsims > 1) { pg_who = (pg_who + nsims + lr) % nsims; audio_voice(VOICE_AGREE); }   // (LEFT and RIGHT: the next housemate)
    }
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
static void personality_line(const Sim *o, char *b, int n) {   // "ARIES. OUTGOING 9, NEAT 2": the sign and the two strongest bars
    int i1 = 0, i2 = -1;
    for (int k = 1; k < TR_N; k++) if (ABS(o->bore.tr[k] - 5) > ABS(o->bore.tr[i1] - 5)) i1 = k;
    for (int k = 0; k < TR_N; k++) if (k != i1 && (i2 < 0 || ABS(o->bore.tr[k] - 5) > ABS(o->bore.tr[i2] - 5))) i2 = k;
    snprintf(b, n, "%s. %s %d, %s %d", look_sign_name(look_sign_of(&o->bore)), look_trait_name(i1), o->bore.tr[i1], look_trait_name(i2), o->bore.tr[i2]);
}
void tr_page_draw(void) {
    char b[96]; const TrState *t = pg_t(); const Sim *who = &sims[pg_who < nsims ? pg_who : player]; int au = (t->f & TF_AUTO) != 0, ns = tr_nslots(who->bore.stage);
    memset(seen_set, 0, sizeof seen_set);
    ui_backdrop();
    char title[40]; snprintf(title, sizeof title, pg_tab ? "TRAITS AND WISHES" : who == &sims[player] ? "YOUR TRAITS" : "%s'S TRAITS", who->bore.first);
    int top = ui_window(4, 4, 312, 232, title, IC_STAR);
    for (int i = 0; i < 2; i++) { int on = i == pg_tab, x = 10 + i * 82; ui_round(x, top + 2, 78, 15, 6, on ? UI_RGB(70, 176, 238) : UI_PANEL, on ? UI_BLUE2 : UI_CARD2, 0); ui_textc(x + 39, top + 6, 1, i ? "WISH LIST" : "TRAITS", on ? UI_WHITE : UI_INK); }
    if (!pg_tab && nsims > 1) ui_text(180, top + 6, 1, "LEFT RIGHT: HOUSEMATES", UI_SOFT);
    int y0 = top + 24;
    if (pg_tab == 0) {
        for (int i = pg_top; i < TR_SLOTS && i < pg_top + 6; i++) { int id = t->tid[i], y = y0 + (i - pg_top) * 14, s = i == pg_sel, ch = id >= TR_FIRST_CHEAT;
            if (s) ui_round(10, y, 138, 13, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
            if (id) { snprintf(b, sizeof b, "%d %s", i + 1, tr_names_of(t)[i][0] ? tr_names_of(t)[i] : nameof(id)); ui_icon(12, y, id < TR_FIRST_EARN ? (ch ? IC_BOLT : IC_SMILE) : IC_STAR, ch ? UI_RED : id < TR_FIRST_EARN ? UI_BLUE : UI_AMBER); ui_textfit(28, y + 3, 116, b, ch ? UI_BADT : s ? UI_BLUE2 : UI_INK); }
            else if (i < ns) { snprintf(b, sizeof b, "%d EMPTY", i + 1); ui_round(15, y + 2, 9, 9, 3, UI_CARD2, UI_CARD2, 0); ui_textfit(28, y + 3, 116, b, UI_SOFT); }
            else { const char *tx = i < TR_BASE ? "WHEN OLDER" : who->bore.stage < TR_UNL_STAGE || i - TR_BASE < tr_unl ? "WHEN A TEEN" : tr_unl_text[i - TR_BASE]; snprintf(b, sizeof b, "%d %s", i + 1, tx); ui_icon(12, y, IC_LOCK, UI_LINE); ui_textfit(28, y + 3, 116, b, UI_RGB(140, 150, 168)); } }
        { int id = t->tid[pg_sel], ch = id >= TR_FIRST_CHEAT; ui_section(156, y0 - 2, 150, id ? "WHAT IT DOES" : pg_sel < ns ? "OPEN SLOT" : "SHUT SLOT");
          if (id) { if (id < TR_FIRST_EARN || ch) fmt_desc(id, b, sizeof b); else fmt_eff(tr_small[small_i(id)], b, sizeof b); }
          else if (pg_sel < ns) snprintf(b, sizeof b, au ? "THEIR PERSONALITY FITS NO MORE TRAITS." : "PICK A TRAIT IN CREATE A BORE, OR WIN ONE.");
          else if (pg_sel >= TR_BASE && who->bore.stage >= TR_UNL_STAGE && pg_sel - TR_BASE >= tr_unl) snprintf(b, sizeof b, "OPENS FOR THE WHOLE FAMILY: %s.", tr_unl_text[pg_sel - TR_BASE]);
          else snprintf(b, sizeof b, "THIS SLOT OPENS WHEN THEY GROW UP.");
          ui_wrap(158, y0 + 12, 148, 5, b, id ? UI_INK : UI_SOFT);
          if (id >= TR_FIRST_EARN && id < TR_FIRST_RWD) { const EarnDef *e = &tr_earn[id - TR_FIRST_EARN]; snprintf(b, sizeof b, "WON BY: %s, %d", tr_ev_name[e->ev], e->thr); ui_wrap(158, y0 + 62, 148, 2, b, UI_SOFT); }
          else if (id >= TR_FIRST_RWD && !ch) ui_text(158, y0 + 62, 1, "A LIFETIME WISH PRIZE", UI_GOLDT);
          else if (ch) ui_text(158, y0 + 62, 1, "CHEAT: NO MEDALS OR SCORES", UI_BADT);
          else if (id && au) ui_text(158, y0 + 62, 1, "FROM THEIR PERSONALITY", UI_SOFT); }
        if (au) { ui_section(10, y0 + 90, 296, "THEIR PERSONALITY"); personality_line(who, b, sizeof b); ui_text(10, y0 + 104, 1, b, UI_INK);
                  ui_wrap(10, y0 + 118, 296, 3, "CHOSEN FROM THE STAR SIGN, THE FIVE BARS AND THE ASPIRATION. THE TRAITS GROW UP WITH THEM AND ARE NOT EDITED.", UI_SOFT); }
        else {
        ui_section(10, y0 + 90, 296, "NEXT TO WIN");
        for (int shown = 0; shown < 3; shown++) {   // the three earned traits nearest to being won
            int best = -1, bp = -1;
            for (int i = 0; i < TR_NEARN; i++) { if (!tr_sname(i)[0] || shown_has(i)) continue; int id = TR_FIRST_EARN + i, own = 0; for (int k = 0; k < TR_SLOTS; k++) own |= t->tid[k] == id; if (own) continue;
                int p = 100 * tr_ev_a[tr_earn[i].ev] / (tr_earn[i].thr ? tr_earn[i].thr : 1); if (p > bp) { bp = p; best = i; } }
            if (best < 0) break;
            { int y = y0 + 102 + shown * 22; const EarnDef *e = &tr_earn[best]; ui_textfit(10, y, 110, tr_sname(best), UI_INK); ui_bar(124, y + 2, 120, 7, MIN(100, bp), UI_GREEN);
              snprintf(b, sizeof b, "%d/%d", MIN(tr_ev_a[e->ev], e->thr), e->thr); ui_text(250, y, 1, b, UI_SOFT); ui_textfit(10, y + 12, 230, tr_ev_name[e->ev], UI_SOFT); }
            seen_mark(best);
        } }
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
    ui_hints(160, top + 194, pg_tab ? "X CHOOSE|L1 R1 TAB|O BACK" : "UP DOWN|LEFT RIGHT WHO|L1 R1 TAB|O BACK", 1);
}
