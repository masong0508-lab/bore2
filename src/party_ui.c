// party_ui.c - the PARTY pack's guest list. It is the overlay PARTY.OVL (ovl.h): a file on the disc of its own, so the pack can be patched
// or left out without touching the game. Like The Sims: ask the people you know, ask a whole household at once, or throw the doors open.
// Who is asked lives in the game (citizens.c, pg_mem[].on); this screen only changes it, then sets party_go and the game starts the party.
#include "citizens.h"
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "ovl.h"
#include <psxgpu.h>
#include <stdio.h>
#include <string.h>

#define PARTY_CAP 60
enum { L_LIST, L_MEM };
static int lv, sel, top, msel, mtop, grp, anim, msgT, score;
static const char *msg;
static const char *const ratingNm[4] = { "A QUIET NIGHT", "A NICE PARTY", "A GREAT PARTY", "A LEGENDARY PARTY" };
static const char *const tipNm[4] = {
    "NO PARTY THINGS NEAR. ADD SOME IN BUILD MODE.",
    "A GOOD START. MORE THINGS, LONGER PARTIES.",
    "NOW IT FEELS LIKE A PARTY. GUESTS WILL REMEMBER.",
    "THE ONE THEY TALK ABOUT. A HOUSE MOOD BOOST." };
enum { R_START, R_OPEN, R_ALL, R_CLEAR, R_N };

static void say(const char *s) { msg = s; msgT = 150; }
static int invited(void) { int n = 0; for (int i = 0; i < pg_nm; i++) n += pg_mem[i].on; return n; }
static int rows(void) { return R_N + pg_ng; }
static int grp_on(int g) { int n = 0; const PGroup *k = &pg_grp[g]; for (int j = 0; j < k->n; j++) n += pg_mem[k->first + j].on; return n; }
static void grp_set(int g, int v) {
    const PGroup *k = &pg_grp[g]; int room = PARTY_CAP - invited();
    for (int j = 0; j < k->n; j++) { PGuest *m = &pg_mem[k->first + j]; if (m->on == v) continue; if (v && room <= 0) { say("THE FLOOR IS FULL: 60 GUESTS IS THE MOST"); return; } m->on = (u8)v; room += v ? -1 : 1; }
}
static int rating(void) { return score >= 8 ? 3 : score >= 4 ? 2 : score >= 1 ? 1 : 0; }

void party_open_ui(int arg) {
    citizens_known(); lv = L_LIST; sel = R_START; top = msel = mtop = 0; grp = 0; anim = 0; msgT = 0; msg = 0; party_go = 0; party_open = 0;
    score = citizens_party_score_here();
}
int party_update(int hd, int pr, int steps) {
    anim += steps; if (msgT > 0) { msgT -= steps; if (msgT <= 0) { msgT = 0; msg = 0; } }
    if (lv == L_LIST) {
        int n = rows();
        if (pr & PAD_UP) sel = (sel + n - 1) % n; if (pr & PAD_DOWN) sel = (sel + 1) % n;
        if (sel < top) top = sel; if (sel > top + 5) top = sel - 5;
        if (pr & PAD_CIRCLE) return 0;
        if (sel >= R_N && (pr & PAD_RIGHT)) { grp = sel - R_N; lv = L_MEM; msel = 0; mtop = 0; }
        if (pr & PAD_CROSS) {
            if (sel == R_START) { if (!invited() && !party_open) say("ASK SOMEONE FIRST, OR OPEN THE HOUSE TO EVERYONE"); else if (cash < (party_open ? 200 : 100)) { say("YOU NEED MORE CASH FOR THAT"); audio_voice(VOICE_NAH); } else { party_go = 1; return 0; } }
            else if (sel == R_OPEN) { party_open ^= 1; say(party_open ? "OPEN HOUSE: STRANGERS FILL THE FLOOR. $200" : "ONLY THE PEOPLE YOU ASKED. $100"); }
            else if (sel == R_ALL) { int room = PARTY_CAP; for (int i = 0; i < pg_nm; i++) { pg_mem[i].on = (u8)(pg_mem[i].known && room > 0); room -= pg_mem[i].on; } say(pg_nm ? "ASKED EVERYONE YOU KNOW" : "YOU DON'T KNOW ANYONE YET. TRY AN OPEN HOUSE"); }
            else if (sel == R_CLEAR) { for (int i = 0; i < pg_nm; i++) pg_mem[i].on = 0; say("THE LIST IS CLEAR"); }
            else { int g = sel - R_N; grp_set(g, grp_on(g) == pg_grp[g].n ? 0 : 1); }
        }
    } else {
        const PGroup *k = &pg_grp[grp];
        if (pr & PAD_UP) msel = (msel + k->n - 1) % k->n; if (pr & PAD_DOWN) msel = (msel + 1) % k->n;
        if (msel < mtop) mtop = msel; if (msel > mtop + 4) mtop = msel - 4;
        if (pr & (PAD_CIRCLE | PAD_LEFT)) lv = L_LIST;
        if (pr & PAD_CROSS) { PGuest *m = &pg_mem[k->first + msel]; if (!m->on && invited() >= PARTY_CAP) say("THE FLOOR IS FULL: 60 GUESTS IS THE MOST"); else m->on ^= 1; }
        if (pr & PAD_TRIANGLE) { int all = grp_on(grp) == k->n; grp_set(grp, all ? 0 : 1); }
    }
    return 1;
}
static void row_special(int y, int i, int on) {
    static const char *const nm[R_N] = { "START THE PARTY", "OPEN HOUSE", "ASK EVERYONE I KNOW", "CLEAR THE LIST" };
    static const int ic[R_N] = { IC_PARTY, IC_DOOR, IC_PEOPLE, IC_TICK };
    char b[32]; strcpy(b, nm[i]);
    ui_row(20, y, 184, 17, b, ic[i], on);
    if (i == R_OPEN) ui_toggle(176, y + 3, party_open, on);
}
void party_draw(void) {
    ui_backdrop();
    int t = ui_window(14, 8, 292, 224, lv == L_LIST ? "THE GUEST LIST" : "WHO IS COMING", IC_PARTY);
    char b[48];
    sprintf(b, "%d / %d ASKED", invited(), PARTY_CAP); ui_badge_r(298, 12, b, UI_WHITE, UI_BLUE2);
    // the right: what kind of night it will be
    ui_round(214, t + 2, 88, 150, 7, UI_WHITE, UI_PANEL, 0);
    ui_text_box(214, t + 5, 88, 12, TX_CENTER | TX_MID, 1, "THE NIGHT", UI_SOFT);
    int r = rating(); ui_wrapc(216, t + 19, 84, 2, ratingNm[r], r >= 2 ? UI_GOODT : UI_INK);
    for (int i = 0; i < 3; i++) ui_icon(240 + i * 14, t + 44, IC_STAR, i < r ? UI_AMBER : UI_LINE);
    sprintf(b, "%d THINGS ABOUT", score); ui_text_box(214, t + 60, 88, 12, TX_CENTER | TX_MID, 1, b, UI_SOFT);
    ui_bar(222, t + 74, 72, 6, MIN(100, score * 8), UI_GREEN);
    ui_wrapc(216, t + 86, 84, 6, tipNm[r], UI_SOFT);
    sprintf(b, "$%d", party_open ? 200 : 100); ui_badge(258 - (ui_tw(1, b) + 10) / 2, t + 136, b, UI_GREEN, UI_WHITE);
    if (lv == L_LIST) {
        int n = rows();
        for (int k = 0; k < 7 && top + k < n; k++) {
            int i = top + k, y = t + 2 + k * 21;
            if (i < R_N) { row_special(y, i, i == sel); continue; }
            int g = i - R_N, on = grp_on(g), tot = pg_grp[g].n; char nm[40]; citizens_gname(g, nm);
            ui_item(20, y, 184, 18, i == sel);
            ui_disc(31, y + 9, 9, on == tot ? UI_GREEN : on ? UI_AMBER : UI_LINE);
            if (on == tot) ui_icon(25, y + 3, IC_TICK, UI_WHITE);
            sprintf(b, "%d/%d", on, tot); int bw = ui_badge_r(200, y + 3, b, on ? UI_BLUE : UI_LINE, on ? UI_WHITE : UI_SOFT);
            ui_fit_box(42, y, 200 - bw - 46, 18, TX_LEFT | TX_MID, nm, UI_INK);
        }
        ui_scrollbar(208, t + 2, 146, top, 7, n);
        if (!pg_ng) ui_wrap(24, t + 110, 180, 4, "NOBODY ON THE LIST YET. MEET PEOPLE IN TOWN, OR OPEN THE HOUSE.", UI_SOFT);
        ui_hints(160, 214, "X PICK|RIGHT WHO'S IN IT|O CANCEL", 1);
    } else {
        const PGroup *k = &pg_grp[grp]; char nm[40]; citizens_gname(grp, nm); ui_text_box(22, t - 1, 180, 13, TX_LEFT | TX_MID, 1, nm, UI_BLUE2);
        for (int j = 0; j < 5 && mtop + j < k->n; j++) {
            int i = mtop + j, y = t + 14 + j * 24; const PGuest *m = &pg_mem[k->first + i]; char fn[24]; citizens_names(m->ref, fn);
            ui_item(20, y, 188, 21, i == msel);
            ui_icon(27, y + 4, IC_SMILE, m->on ? UI_GREEN : UI_LINE);
            ui_fit_box(45, y, 70, 21, TX_LEFT | TX_MID, fn, UI_INK);
            ui_badge(122, y + 5, m->known ? "FRIEND" : "STRANGER", m->known ? UI_BLUE : UI_LINE, m->known ? UI_WHITE : UI_SOFT);
            ui_toggle(180, y + 5, m->on, i == msel);
        }
        ui_hints(160, 214, "X ASK|TRI WHOLE HOUSEHOLD|O BACK", 1);
    }
    if (msg) ui_toast(190, msg);
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, party_open_ui, party_update, party_draw };
