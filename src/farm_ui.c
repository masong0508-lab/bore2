// farm_ui: FARM LIFE's ranch ledger, an overlay (FARM.OVL). Stands on its own: it works on the farm the base game already has
// (the crops, hens and cows, what you have picked), and adds
//   BUILD:     a barn, a silo, an orchard, beehives, a farm stand and a windmill. They earn every day; collect the earnings here.
//   MARKET:    sell what the farm has made, in one go, at the farm stand's better prices.
//   CONTRACTS: three buyers a day want a certain amount of one thing, and pay well over the market.
// pk_data 16-17: what is built, 18-19: earnings collected to that day, 20: contracts done today (bits), 21: the day they are for.
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "ovl.h"
#include "wants.h"
#include <psxgpu.h>
#include <psxpad.h>
#include <stdio.h>
#include <string.h>

enum { T_BUILD, T_MARKET, T_CONTRACT, T_N };
#define NB 6
static const char *const BNM[NB] = { "BARN", "SILO", "ORCHARD", "BEEHIVES", "FARM STAND", "WINDMILL" };
static const char *const BHINT[NB] = { "A ROOF FOR THE ANIMALS. THEY THRIVE.", "GRAIN KEEPS. A STEADY INCOME.", "APPLES AND PEARS EVERY YEAR.", "HONEY, AND THE CROPS LIKE THE BEES.", "SELL AT BETTER PRICES, AND EARN.", "THE FARM RUNS ITSELF A LITTLE." };
static const int BCOST[NB] = { 1500, 3000, 5000, 2000, 4000, 8000 }, BINC[NB] = { 60, 110, 180, 80, 120, 300 };
static const char *const GNM[6] = { "WHEAT", "CORN", "TOMATOES", "PUMPKINS", "EGGS", "MILK" };
static const int GPR[6] = { 4, 7, 9, 16, 5, 10 };
static int tab, sel, anim; static char line[48];
static int built(void) { return pk_data[16] | pk_data[17] << 8; }
static int last(void) { return pk_data[18] | pk_data[19] << 8; }
static int lvl(void) { return wants_skill_level(sims[player].skill_farm); }
static int rate(void) { int r = 0; for (int i = 0; i < NB; i++) if (built() >> i & 1) r += BINC[i]; return r * (100 + lvl() * 8) / 100; }
static int stand(void) { return built() >> 4 & 1 ? 25 : 0; }
static int have_n(int k) { return k < 4 ? fm_stock[k] : k == 4 ? fm_egg : fm_milk; }
static void take(int k, int n) { if (k < 4) fm_stock[k] -= (u16)n; else if (k == 4) fm_egg -= (u16)n; else fm_milk -= (u16)n; }
static int csel(int i, int *k, int *q) { u32 h = ((u32)day_count * 2654435761u + (u32)i * 40503u + 17u) >> 8; *k = (int)(h % 6u); *q = 3 + (int)((h >> 4) % 6u) + lvl() / 3; return 0; }
void farm_open(int a) { (void)a; tab = T_BUILD; sel = 0; anim = 0; line[0] = 0; if (pk_data[21] != (u8)day_count) { pk_data[20] = 0; pk_data[21] = (u8)day_count; }
    if (!built() && !last()) { pk_data[18] = (u8)day_count; pk_data[19] = (u8)(day_count >> 8); } }
static int nrows(void) { return tab == T_BUILD ? NB + 1 : tab == T_MARKET ? 6 : 3; }
static void settle(void) { if (built() && day_count > last()) cash += rate() * MIN(30, day_count - last()); pk_data[18] = (u8)day_count; pk_data[19] = (u8)(day_count >> 8); }
static void act(void) {
    line[0] = 0;
    if (tab == T_BUILD) {
        if (sel == NB) { int d = MIN(30, MAX(0, day_count - last())); if (!built()) { strcpy(line, "BUILD SOMETHING FIRST."); return; } if (!d) { strcpy(line, "NOTHING NEW TODAY."); return; }
            int pay = rate() * d; settle(); sprintf(line, "%d DAYS OF THE FARM: +$%d", d, pay); audio_voice(VOICE_WIN); return; }
        if (built() >> sel & 1) { strcpy(line, "ALREADY BUILT."); return; }
        if (cash < BCOST[sel]) { strcpy(line, "NOT ENOUGH CASH."); audio_voice(VOICE_NAH); return; }
        settle(); cash -= BCOST[sel]; pk_data[16 + (sel >> 3)] |= (u8)(1 << (sel & 7)); sims[player].skill_farm += 40; sprintf(line, "BUILT: %s", BNM[sel]); audio_voice(VOICE_FINISHED); return; }
    if (tab == T_MARKET) {
        int n = have_n(sel); if (!n) { strcpy(line, "NONE TO SELL."); audio_voice(VOICE_NAH); return; }
        int pay = n * GPR[sel] * (100 + stand()) / 100; take(sel, n); cash += pay; sims[player].skill_farm += n; sprintf(line, "SOLD %d %s: +$%d", n, GNM[sel], pay); audio_voice(VOICE_WIN); return; }
    { int k, q; csel(sel, &k, &q); if (pk_data[20] >> sel & 1) { strcpy(line, "DONE TODAY."); return; }
      if (have_n(k) < q) { sprintf(line, "NEED %d %s (HAVE %d)", q, GNM[k], have_n(k)); audio_voice(VOICE_NAH); return; }
      int pay = q * GPR[k] * 18 / 10; take(k, q); cash += pay; pk_data[20] |= (u8)(1 << sel); sims[player].skill_farm += q * 2; sims[player].mood = MIN(100, sims[player].mood + 10); sprintf(line, "CONTRACT FILLED: +$%d", pay); audio_voice(VOICE_FINISHED); }
}
int farm_update(int hd, int pr, int steps) {
    (void)hd; anim += steps; if (pr & PAD_CIRCLE) return 0;
    if (pr & (PAD_R1 | PAD_RIGHT)) { tab = (tab + 1) % T_N; sel = 0; line[0] = 0; }
    if (pr & (PAD_L1 | PAD_LEFT)) { tab = (tab + T_N - 1) % T_N; sel = 0; line[0] = 0; }
    int n = nrows(); if (pr & PAD_UP) sel = (sel + n - 1) % n; if (pr & PAD_DOWN) sel = (sel + 1) % n;
    if (pr & PAD_CROSS) act(); return 1;
}
void farm_draw(void) {
    char b[56]; static const char *const TN[T_N] = { "BUILD", "MARKET", "CONTRACTS" };
    gfx_set_clear(150, 210, 250);
    ui_round(0, 0, 320, 240, 0, UI_RGB(170, 220, 255), UI_RGB(120, 190, 90), 0);
    int top = ui_window(8, 6, 304, 228, "FARM LIFE", IC_SPROUT);
    { int x = 14; for (int i = 0; i < T_N; i++) { int w = ui_tw(1, TN[i]) + 14, on = i == tab; ui_round(x, top, w, 14, 4, on ? UI_BLUE : UI_PANEL, on ? UI_BLUE2 : UI_PANEL, 0); ui_text(x + 7, top + 3, 1, TN[i], on ? UI_WHITE : UI_SOFT); x += w + 4; } }
    sprintf(b, "$%d", (int)cash); ui_text(300 - ui_tw(1, b), top + 3, 1, b, UI_GOODT);
    int n = nrows(), y0 = top + 20;
    for (int i = 0; i < n; i++) {
        int y = y0 + i * 15, on = i == sel; if (on) ui_round(14, y - 2, 292, 15, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
        if (tab == T_BUILD) {
            if (i == NB) { int d = MIN(30, MAX(0, day_count - last())); sprintf(b, "COLLECT  %d DAYS = $%d", d, rate() * d); ui_text(20, y + 1, 1, b, on ? UI_BLUE2 : UI_GOODT); continue; }
            int own = built() >> i & 1; ui_text(20, y + 1, 1, BNM[i], own ? UI_GOODT : on ? UI_BLUE2 : UI_INK);
            if (own) ui_text(268, y + 1, 1, "BUILT", UI_GOODT); else { sprintf(b, "$%d", BCOST[i]); ui_text(300 - ui_tw(1, b), y + 1, 1, b, cash >= BCOST[i] ? UI_INK : UI_BADT); }
        } else if (tab == T_MARKET) {
            int c = have_n(i); sprintf(b, "%s  x%d", GNM[i], c); ui_text(20, y + 1, 1, b, c ? (on ? UI_BLUE2 : UI_INK) : UI_SOFT);
            sprintf(b, "$%d EACH", GPR[i] * (100 + stand()) / 100); ui_text(300 - ui_tw(1, b), y + 1, 1, b, UI_GOODT);
        } else { int k, q; csel(i, &k, &q); int dn = pk_data[20] >> i & 1; sprintf(b, "%d %s", q, GNM[k]); ui_text(20, y + 1, 1, b, dn ? UI_GOODT : have_n(k) >= q ? UI_BLUE2 : on ? UI_INK : UI_SOFT);
            if (dn) ui_text(268, y + 1, 1, "DONE", UI_GOODT); else { sprintf(b, "PAYS $%d", q * GPR[k] * 18 / 10); ui_text(300 - ui_tw(1, b), y + 1, 1, b, UI_GOODT); } }
    }
    int by = top + 128; ui_round(14, by, 292, 56, 5, UI_PANEL, UI_LINE, 0);
    if (tab == T_BUILD) { sprintf(b, "FARM LEVEL %d  $%d A DAY", lvl(), rate()); ui_text(20, by + 4, 1, b, UI_BLUE2); ui_wrap(20, by + 17, 280, 2, sel < NB ? BHINT[sel] : "THE FARM EARNS EVERY DAY. COME BACK AND COLLECT.", UI_INK); }
    else if (tab == T_MARKET) { ui_text(20, by + 4, 1, stand() ? "THE FARM STAND ADDS 25% TO EVERY SALE" : "BUILD A FARM STAND FOR BETTER PRICES", UI_BLUE2); ui_wrap(20, by + 17, 280, 2, "X SELLS ALL OF THAT ONE.", UI_INK); }
    else { ui_text(20, by + 4, 1, "NEW BUYERS EVERY DAY", UI_BLUE2); ui_wrap(20, by + 17, 280, 2, "THEY PAY ABOUT TWICE THE MARKET PRICE. X FILLS ONE.", UI_INK); }
    ui_text(20, by + 42, 1, line, UI_RED);
    ui_hints(160, 218, "X PICK|L1 R1 TABS|O LEAVE", 1);
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, farm_open, farm_update, farm_draw };
