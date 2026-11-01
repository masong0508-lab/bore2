// farm_ui: FARM LIFE's FARM BOOK, an overlay (FARM.OVL): the phone's FARM LEDGER, or FARM BOOK at the farm stand. It shows the farm the base
// game runs (sim.c: the crops through the seasons, the hens and cows and how happy they are, what you have picked) in big icons and short
// words, says what to do next, and adds
//   SELL:   sell what the farm has made, one kind or all of it, at the stand's prices.
//   BUILD:  a barn, a silo, an orchard, beehives, a farm stand and a windmill. They earn every day; collect the earnings here.
//   ORDERS: three buyers a day want a certain amount of one thing, and pay well over the market.
// pk_data 16-17: what is built, 18-19: earnings collected to that day, 20: contracts done today (bits), 21: the day they are for.
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "ovl.h"
#include "wants.h"
#include "places.h"
#include <psxgpu.h>
#include <psxpad.h>
#include <stdio.h>
#include <string.h>

enum { T_FARM, T_ANIMALS, T_SELL, T_BUILD, T_ORDERS, T_N };
#define NB 6
static const char *const BNM[NB] = { "BARN", "SILO", "ORCHARD", "BEEHIVES", "FARM STAND", "WINDMILL" };
static const char *const BHINT[NB] = { "A ROOF FOR THE ANIMALS. THEY THRIVE.", "GRAIN KEEPS. A STEADY INCOME.", "APPLES AND PEARS EVERY YEAR.", "HONEY, AND THE CROPS LIKE THE BEES.", "SELL AT BETTER PRICES, AND EARN.", "THE FARM RUNS ITSELF A LITTLE." };
static const int BCOST[NB] = { 1500, 3000, 5000, 2000, 4000, 8000 }, BINC[NB] = { 60, 110, 180, 80, 120, 300 };
static const u32 CCOL[4] = { UI_RGB(214, 170, 50), UI_RGB(236, 196, 40), UI_RGB(222, 70, 56), UI_RGB(240, 130, 30) };   // wheat, corn, tomato, pumpkin
static int tab, sel, anim; static char line[48];
static int built(void) { return pk_data[16] | pk_data[17] << 8; }
static int last(void) { return pk_data[18] | pk_data[19] << 8; }
static int lvl(void) { return wants_skill_level(sims[player].skill_farm); }
static int rate(void) { int r = 0; for (int i = 0; i < NB; i++) if (built() >> i & 1) r += BINC[i]; return r * (100 + lvl() * 8) / 100; }
static int stand(void) { return built() >> 4 & 1 ? 25 : 0; }
static void take(int k, int n) { if (k < 4) fm_stock[k] -= (u16)n; else if (k == 4) fm_egg -= (u16)n; else fm_milk -= (u16)n; }
static int price(int k) { return fmPr[k] * (100 + stand()) / 100; }
static int csel(int i, int *k, int *q) { u32 h = ((u32)day_count * 2654435761u + (u32)i * 40503u + 17u) >> 8; *k = (int)(h % 6u); *q = 3 + (int)((h >> 4) % 6u) + lvl() / 3; return 0; }
void farm_open(int a) { tab = a % T_N; sel = 0; anim = 0; line[0] = 0; if (pk_data[21] != (u8)day_count) { pk_data[20] = 0; pk_data[21] = (u8)day_count; }
    if (!built() && !last()) { pk_data[18] = (u8)day_count; pk_data[19] = (u8)(day_count >> 8); } }
static int nrows(void) { return tab == T_BUILD ? NB + 1 : tab == T_SELL ? 7 : tab == T_ORDERS ? 3 : 0; }
static void settle(void) { if (built() && day_count > last()) cash += rate() * MIN(30, day_count - last()); pk_data[18] = (u8)day_count; pk_data[19] = (u8)(day_count >> 8); }
static void sold(int pay) { fm_earned += pay; wants_event(&sims[player], W_FARMSELL, 0); }
static void act(void) {
    line[0] = 0;
    if (tab == T_BUILD) {
        if (sel == NB) { int d = MIN(30, MAX(0, day_count - last())); if (!built()) { strcpy(line, "BUILD SOMETHING FIRST."); return; } if (!d) { strcpy(line, "NOTHING NEW TODAY."); return; }
            int pay = rate() * d; settle(); sprintf(line, "%d DAYS OF THE FARM: +$%d", d, pay); audio_voice(VOICE_WIN); return; }
        if (built() >> sel & 1) { strcpy(line, "ALREADY BUILT."); return; }
        if (cash < BCOST[sel]) { strcpy(line, "NOT ENOUGH CASH."); audio_voice(VOICE_NAH); return; }
        settle(); cash -= BCOST[sel]; pk_data[16 + (sel >> 3)] |= (u8)(1 << (sel & 7)); sims[player].skill_farm += 40; sprintf(line, "BUILT: %s", BNM[sel]); audio_voice(VOICE_FINISHED); return; }
    if (tab == T_SELL) {   // one kind, or (the last row) all of it
        int pay = 0, n = 0;
        for (int k = 0; k < 6; k++) if (k == sel || sel == 6) { int c = fm_have(k); if (c) { pay += c * price(k); n += c; take(k, c); } }
        if (!n) { strcpy(line, "NONE TO SELL."); audio_voice(VOICE_NAH); return; }
        cash += pay; sims[player].skill_farm += n; sold(pay); if (sel == 6) sprintf(line, "SOLD IT ALL: +$%d", pay); else sprintf(line, "SOLD %d %s: +$%d", n, fmGood[sel], pay); audio_voice(VOICE_WIN); return; }
    if (tab == T_ORDERS) { int k, q; csel(sel, &k, &q); if (pk_data[20] >> sel & 1) { strcpy(line, "DONE TODAY."); return; }
      if (fm_have(k) < q) { sprintf(line, "NEED %d %s (HAVE %d)", q, fmGood[k], fm_have(k)); audio_voice(VOICE_NAH); return; }
      int pay = q * fmPr[k] * 18 / 10; take(k, q); cash += pay; sold(pay); pk_data[20] |= (u8)(1 << sel); sims[player].skill_farm += q * 2; sims[player].mood = MIN(100, sims[player].mood + 10); sprintf(line, "ORDER FILLED: +$%d", pay); audio_voice(VOICE_FINISHED); }
}
int farm_update(int hd, int pr, int steps) {
    (void)hd; anim += steps; if (pr & PAD_CIRCLE) return 0;
    if (pr & (PAD_R1 | PAD_RIGHT)) { tab = (tab + 1) % T_N; sel = 0; line[0] = 0; }
    if (pr & (PAD_L1 | PAD_LEFT)) { tab = (tab + T_N - 1) % T_N; sel = 0; line[0] = 0; }
    int n = nrows(); if (n) { if (pr & PAD_UP) sel = (sel + n - 1) % n; if (pr & PAD_DOWN) sel = (sel + 1) % n; if (pr & PAD_CROSS) act(); }
    return 1;
}
static const char *todo(void) {   // the one thing to do next, in plain words
    int any = 0, rdy = 0, dry = 0;
    for (int k = 0; k < 4; k++) if (fm_n[k]) { any = 1; if (fm_g[k] >= fm_need(k)) rdy = 1; else if (!((fm_wet >> k) & 1) && !(fm_up & 1)) dry = 1; }
    if (fm_pest) return "SPRAY THE BUGS ON YOUR CROPS";
    if (rdy) return "PICK YOUR CROPS: THEY ARE READY";
    if (any && sim_season() == 2 && day_count % 10 == 9 && !(fm_up & 4)) return "FROST TONIGHT! PICK WHAT YOU CAN";
    if (dry) return "WATER YOUR CROPS";
    if (fm_hens && !(fm_fed & 1)) return "FEED THE HENS (THE COOP)";
    if (fm_cows && !(fm_fed & 2)) return "FEED THE COWS (THE STALLS)";
    if (fm_pe) return "COLLECT THE EGGS";
    if (fm_pm) return "MILK THE COWS";
    if (fm_hens && !(fm_pet & 1)) return "PET THE HENS";
    if (fm_cows && !(fm_pet & 2)) return "BRUSH THE COWS";
    if (fm_total()) return "SELL YOUR FOOD (SELL, OR THE STAND)";
    if (!any) return sim_season() == 3 && !(fm_up & 4) ? "WINTER: REST. PLANT IN SPRING" : "PLANT SOME SEEDS IN A CROP BED";
    return "ALL DONE TODAY! THEY GROW AT NIGHT";
}
static void pill(int x, int y, const char *s, u32 bg) { ui_badge(x, y, s, bg, UI_WHITE); }
static void tick_or_x(int x, int y, int ok) { ui_icon_disc(x, y, 13, ok ? IC_TICK : IC_BX, ok ? UI_GREEN : UI_RED, UI_WHITE); }
void farm_draw(void) {
    char b[56]; static const char *const TN[T_N] = { "FARM", "ANIMALS", "SELL", "BUILD", "ORDERS" }; static const u8 TI[T_N] = { IC_SPROUT, IC_HEART, IC_MONEY, IC_HAMMER, IC_CASE };
    gfx_set_clear(150, 210, 250);
    ui_round(0, 0, 320, 240, 0, UI_RGB(170, 220, 255), UI_RGB(120, 190, 90), 0);
    int top = ui_window(8, 6, 304, 228, "FARM LIFE", IC_SPROUT);
    { int x = 13; for (int i = 0; i < T_N; i++) { int on = i == tab; x += ui_badge_i(x, top, TI[i], TN[i], on ? UI_BLUE : UI_PANEL, on ? UI_WHITE : UI_SOFT) + 3; } }
    sprintf(b, "$%d", (int)cash); ui_badge(296 - ui_tw(1, b), 9, b, UI_WHITE, UI_GOODT);   // (the money, up in the header: the page tabs fill the row)
    int y0 = top + 20, se = sim_season();
    if (tab == T_FARM) {   // the crops: a row each, with how far it has grown and what it needs
        sprintf(b, "%s  DAY %d OF 10", sim_season_name(se), day_count % 10 + 1); ui_badge_i(14, y0, se == 3 ? IC_DROP : IC_SUN, b, se == 3 ? UI_BLUE2 : UI_AMBER, UI_WHITE);
        if (fm_up & 4) pill(200, y0, "GREENHOUSE", UI_GREEN);
        for (int k = 0; k < 4; k++) {
            int y = y0 + 18 + k * 24, n = fm_n[k], need = fm_need(k), g = MIN(fm_g[k], need), good = fm_in_season(k) || (fm_up & 4);
            ui_round(14, y - 2, 292, 22, 5, UI_WHITE, UI_PANEL, 0);
            ui_icon_disc(26, y + 9, 18, IC_SPROUT, n ? CCOL[k] : UI_RGB(180, 186, 196), UI_WHITE);
            ui_text(40, y + 1, 1, fmGood[k], UI_INK);
            if (n) { sprintf(b, "%d PLANTS", n); ui_text(40, y + 11, 1, b, UI_SOFT); ui_bar(112, y + 6, 80, 7, g * 100 / need, CCOL[k]);
                const char *st = (fm_pest >> k) & 1 ? "BUGS!" : g >= need ? "READY" : ((fm_wet >> k) & 1) || (fm_up & 1) ? "GROWING" : "WATER ME";
                pill(198, y + 3, st, (fm_pest >> k) & 1 ? UI_RED : g >= need ? UI_GREEN : ((fm_wet >> k) & 1) || (fm_up & 1) ? UI_SOFT : UI_BLUE); }
            else { ui_text(112, y + 6, 1, se == 3 && !(fm_up & 4) ? "TOO COLD TO PLANT" : "EMPTY BED", UI_SOFT); }
            if (se < 3 || (fm_up & 4)) { ui_icon(266, y + 3, good ? IC_SUN : IC_DROP, good ? UI_GOLDT : UI_SOFT); ui_text(280, y + 1, 1, good ? "GOOD" : "SLOW", good ? UI_GOLDT : UI_SOFT); ui_text(280, y + 11, 1, "NOW", good ? UI_GOLDT : UI_SOFT); }
        }
        int by = y0 + 18 + 4 * 24 + 2; ui_round(14, by, 292, 50, 5, UI_RGB(255, 248, 220), UI_RGB(244, 226, 170), 0);
        ui_icon_disc(28, by + 14, 20, IC_STAR, UI_AMBER, UI_WHITE); ui_text(42, by + 4, 1, "TO DO", UI_GOLDT); ui_wrap(42, by + 15, 258, 2, todo(), UI_INK);
        if (job_car == CAR_FARM && !job_none) { sprintf(b, "YOUR JOB: %s, LEVEL %d", job_title(), job_level); ui_text(42, by + 38, 1, b, UI_BLUE2); }
        else ui_text(42, by + 38, 1, "A FARM JOB: PHONE, JOBS", UI_SOFT);
    } else if (tab == T_ANIMALS) {   // the hens and the cows: how many, how happy, fed, petted, what waits to be collected
        for (int a = 0; a < 2; a++) {
            int y = y0 + a * 66, n = a ? fm_cows : fm_hens, hp = a ? fm_ch : fm_hh, w = a ? fm_pm : fm_pe;
            ui_round(14, y, 292, 60, 6, UI_WHITE, UI_PANEL, 0);
            ui_icon_disc(34, y + 22, 30, a ? IC_FOOD : IC_SMILE, n ? (a ? UI_RGB(120, 96, 80) : UI_RGB(236, 140, 60)) : UI_RGB(180, 186, 196), UI_WHITE);
            sprintf(b, a ? "COWS  %d OF 3" : "HENS  %d OF 6", n); ui_text(56, y + 4, 2, b, UI_INK);
            if (!n) { ui_text(56, y + 26, 1, a ? "BUY A COW AT THE STALLS: $250" : "BUY A HEN AT THE COOP: $40", UI_SOFT); continue; }
            ui_icon(56, y + 24, IC_HEART, hp >= 60 ? UI_RED : UI_SOFT); ui_bar(72, y + 26, 90, 8, hp, hp >= 80 ? UI_GREEN : hp >= 30 ? UI_AMBER : UI_RED);
            ui_text(168, y + 25, 1, hp >= 80 ? "VERY HAPPY" : hp >= 30 ? "OK" : "SAD", UI_INK);
            tick_or_x(62, y + 47, (fm_fed >> a) & 1); ui_text(72, y + 43, 1, "FED", UI_INK);
            tick_or_x(116, y + 47, (fm_pet >> a) & 1); ui_text(126, y + 43, 1, a ? "BRUSHED" : "PETTED", UI_INK);
            sprintf(b, a ? "MILK WAITING: %d" : "EGGS WAITING: %d", w); ui_text(196, y + 43, 1, b, w ? UI_GOODT : UI_SOFT);
        }
        ui_wrap(16, y0 + 136, 288, 3, "FEED THEM AND PET THEM EVERY DAY. HAPPY ANIMALS GIVE TWICE AS MUCH. HUNGRY OR SAD ONES GIVE NOTHING.", UI_INK);
    } else {
        int n = nrows();
        for (int i = 0; i < n; i++) {
            int y = y0 + i * 15, on = i == sel; if (on) ui_round(14, y - 2, 292, 15, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
            if (tab == T_BUILD) {
                if (i == NB) { int d = MIN(30, MAX(0, day_count - last())); sprintf(b, "COLLECT  %d DAYS = $%d", d, rate() * d); ui_text(20, y + 1, 1, b, on ? UI_BLUE2 : UI_GOODT); continue; }
                int own = built() >> i & 1; ui_text(20, y + 1, 1, BNM[i], own ? UI_GOODT : on ? UI_BLUE2 : UI_INK);
                if (own) ui_text(268, y + 1, 1, "BUILT", UI_GOODT); else { sprintf(b, "$%d", BCOST[i]); ui_text(300 - ui_tw(1, b), y + 1, 1, b, cash >= BCOST[i] ? UI_INK : UI_BADT); }
            } else if (tab == T_SELL) {
                if (i == 6) { int t = 0; for (int k = 0; k < 6; k++) t += fm_have(k) * price(k); sprintf(b, "SELL IT ALL: $%d", t); ui_text(20, y + 1, 1, b, t ? (on ? UI_BLUE2 : UI_GOODT) : UI_SOFT); continue; }
                int c = fm_have(i); sprintf(b, "%s  x%d", fmGood[i], c); ui_text(20, y + 1, 1, b, c ? (on ? UI_BLUE2 : UI_INK) : UI_SOFT);
                sprintf(b, "$%d EACH", price(i)); ui_text(300 - ui_tw(1, b), y + 1, 1, b, UI_GOODT);
            } else { int k, q; csel(i, &k, &q); int dn = pk_data[20] >> i & 1; sprintf(b, "%d %s", q, fmGood[k]); ui_text(20, y + 1, 1, b, dn ? UI_GOODT : fm_have(k) >= q ? UI_BLUE2 : on ? UI_INK : UI_SOFT);
                if (dn) ui_text(268, y + 1, 1, "DONE", UI_GOODT); else { sprintf(b, "PAYS $%d", q * fmPr[k] * 18 / 10); ui_text(300 - ui_tw(1, b), y + 1, 1, b, UI_GOODT); } }
        }
        int by = top + 128; ui_round(14, by, 292, 56, 5, UI_PANEL, UI_LINE, 0);
        if (tab == T_BUILD) { sprintf(b, "FARM LEVEL %d  $%d A DAY", lvl(), rate()); ui_text(20, by + 4, 1, b, UI_BLUE2); ui_wrap(20, by + 17, 280, 2, sel < NB ? BHINT[sel] : "THE FARM EARNS EVERY DAY. COME BACK AND COLLECT.", UI_INK); }
        else if (tab == T_SELL) { ui_text(20, by + 4, 1, stand() ? "THE FARM STAND ADDS 25% TO EVERY SALE" : "BUILD A FARM STAND FOR BETTER PRICES", UI_BLUE2);
            sprintf(b, "THE FARM HAS MADE $%d. TOWNSFOLK BOUGHT %d TIMES.", fm_earned, fm_buyers); ui_wrap(20, by + 17, 280, 2, b, UI_INK); }
        else { ui_text(20, by + 4, 1, "NEW BUYERS EVERY DAY", UI_BLUE2); ui_wrap(20, by + 17, 280, 2, "THEY PAY ABOUT TWICE THE MARKET PRICE. X FILLS ONE.", UI_INK); }
        ui_text(20, by + 42, 1, line, UI_RED);
    }
    ui_hints(160, 218, nrows() ? "X PICK|L1 R1 PAGES|O LEAVE" : "L1 R1 PAGES|O LEAVE", 1);
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, farm_open, farm_update, farm_draw };
