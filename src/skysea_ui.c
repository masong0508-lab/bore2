// skysea_ui: SKY AND SEA, an overlay (SKYSEA.OVL): boats and things that fly, together. It stands on its own.
//   FLEET:    eight craft to buy, from a rowboat to an airship. They carry guests and cargo every day; collect the takings here.
//   VOYAGES:  take a craft out yourself: a gauge sweeps, press X in the bright zone on each leg. Good legs pay well; a stormy one can cost you.
// pk_data 24-25: the fleet, 26-27: takings collected to that day, 28: voyages made.
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

enum { T_FLEET, T_VOY, T_N, S_RUN = 10, S_RES };
#define NV 8
#define NY 6
static const char *const VNM[NV] = { "ROWBOAT", "FISHING SKIFF", "SAILBOAT", "TRAWLER", "HOT AIR BALLOON", "GLIDER", "SEAPLANE", "AIRSHIP" };
static const char *const VHINT[NV] = { "A LAZY MORNING ON THE WATER.", "THE FISH BITE AT DAWN.", "WIND, SALT AND SUNSETS.", "NETS FULL OF CATCH.", "DRIFT OVER THE FIELDS.", "QUIET, HIGH AND FAST.", "LAND ON THE WAVES.", "A FLOATING HOTEL IN THE SKY." };
static const int VCOST[NV] = { 600, 1500, 3500, 7000, 9000, 14000, 26000, 60000 }, VINC[NV] = { 30, 70, 150, 300, 380, 560, 1000, 2400 };
static const char *const YNM[NY] = { "COAST RUN", "DEEP SEA FISHING", "SUNSET CRUISE", "CLOUD HOP", "MOUNTAIN PASS", "ROUND THE WORLD" };
static const int YNEED[NY] = { 1 << 0, 1 << 1 | 1 << 3, 1 << 2, 1 << 4, 1 << 5 | 1 << 6, 1 << 7 }, YLEG[NY] = { 3, 4, 4, 4, 5, 7 }, YFUEL[NY] = { 20, 80, 120, 150, 300, 900 }, YPAY[NY] = { 160, 420, 600, 760, 1700, 6500 };
static int tab, sel, anim, st, ysel, leg, mark, dir, zone, zw, tmr, good, hits; static char line[48], line2[48];
static unsigned rs = 5;
static int rnd(int n) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (int)((rs >> 3) % (unsigned)n); }
static int fleet(void) { return pk_data[24] | pk_data[25] << 8; }
static int last(void) { return pk_data[26] | pk_data[27] << 8; }
static int lvl(void) { return wants_skill_level(sims[player].skill_logic); }
static int rate(void) { int r = 0; for (int i = 0; i < NV; i++) if (fleet() >> i & 1) r += VINC[i]; return r; }
static void settle(void) { if (fleet() && day_count > last()) cash += rate() * MIN(30, day_count - last()); pk_data[26] = (u8)day_count; pk_data[27] = (u8)(day_count >> 8); }
static void new_zone(void) { zw = 14 + lvl() * 2 + (fleet() >> 7 & 1) * 4 - ysel; if (zw < 8) zw = 8; zone = 10 + rnd(80 - zw); mark = rnd(100); dir = rnd(2) ? 1 : -1; tmr = 0; }
void skysea_open(int a) { (void)a; tab = T_FLEET; st = T_FLEET; sel = 0; ysel = 0; anim = 0; line[0] = line2[0] = 0; rs = (unsigned)(cash * 5 + day_count * 13 + 3);
    if (!fleet() && !last()) { pk_data[26] = (u8)day_count; pk_data[27] = (u8)(day_count >> 8); } }
static int nrows(void) { return tab == T_FLEET ? NV + 1 : NY; }
static void start_run(void) {
    if (!(fleet() & YNEED[ysel])) { strcpy(line, "YOU NEED THE RIGHT CRAFT."); audio_voice(VOICE_NAH); return; }
    if (cash < YFUEL[ysel]) { strcpy(line, "NOT ENOUGH CASH FOR FUEL."); audio_voice(VOICE_NAH); return; }
    cash -= YFUEL[ysel]; leg = 0; hits = 0; good = 0; line[0] = line2[0] = 0; st = S_RUN; new_zone(); audio_voice(VOICE_YAHOO);
}
static void finish_run(void) {
    int pay = YPAY[ysel] * (hits * 2 + (YLEG[ysel] - hits)) / (YLEG[ysel] * 2); pk_data[28]++;
    if (hits * 2 < YLEG[ysel]) { pay = YPAY[ysel] / 5; strcpy(line, "A ROUGH TRIP. LITTLE TO SHOW."); strcpy(line2, "THE SEA (OR SKY) IS MOODY."); sims[player].mood = MAX(-100, sims[player].mood - 10); }
    else { strcpy(line, hits == YLEG[ysel] ? "A PERFECT VOYAGE!" : "A GOOD VOYAGE!"); strcpy(line2, "THE GUESTS TIP WELL."); sims[player].mood = MIN(100, sims[player].mood + 25); audio_voice(VOICE_WIN); }
    cash += pay; sims[player].skill_logic += 30; sims[player].skill_fun += 20; { char t[32]; snprintf(t, sizeof t, " +$%d", pay); strncat(line2, t, sizeof line2 - strlen(line2) - 1); }
    clock_min = MIN(1439, clock_min + 60 + YLEG[ysel] * 10); st = S_RES;
}
static void act(void) {
    line[0] = 0;
    if (tab == T_FLEET) {
        if (sel == NV) { int d = MIN(30, MAX(0, day_count - last())); if (!fleet()) { strcpy(line, "BUY A CRAFT FIRST."); return; } if (!d) { strcpy(line, "NOTHING NEW TODAY."); return; }
            int pay = rate() * d; settle(); sprintf(line, "%d DAYS OF GUESTS: +$%d", d, pay); audio_voice(VOICE_WIN); return; }
        if (fleet() >> sel & 1) { strcpy(line, "YOU OWN IT."); return; }
        if (cash < VCOST[sel]) { strcpy(line, "NOT ENOUGH CASH."); audio_voice(VOICE_NAH); return; }
        settle(); cash -= VCOST[sel]; pk_data[24 + (sel >> 3)] |= (u8)(1 << (sel & 7)); sprintf(line, "BOUGHT: %s", VNM[sel]); audio_voice(VOICE_FINISHED); return; }
    ysel = sel; start_run();
}
int skysea_update(int hd, int pr, int steps) {
    anim += steps; (void)hd;
    if (st == S_RUN) {
        int sp = 2 + leg + ysel / 2 - lvl() / 4; if (sp < 1) sp = 1;
        for (int i = 0; i < steps; i++) { mark += dir * sp; if (mark > 99) { mark = 99; dir = -1; } if (mark < 0) { mark = 0; dir = 1; } }
        if (tmr > 0) { tmr -= steps; if (tmr <= 0) { if (++leg >= YLEG[ysel]) finish_run(); else new_zone(); } return 1; }
        if (pr & PAD_CROSS) { int d = ABS(mark - (zone + zw / 2));
            if (d <= zw / 2) { strcpy(line, "SMOOTH!"); hits++; } else if (d <= zw) strcpy(line, "A BUMPY LEG."); else strcpy(line, "STORM! HOLD ON!");
            tmr = 40; }
        return 1; }
    if (st == S_RES) { if (pr & (PAD_CROSS | PAD_CIRCLE)) { st = T_FLEET; line[0] = 0; } return 1; }
    if (pr & PAD_CIRCLE) return 0;
    if (pr & (PAD_R1 | PAD_RIGHT | PAD_L1 | PAD_LEFT)) { tab ^= 1; sel = 0; line[0] = 0; }
    int n = nrows(); if (pr & PAD_UP) sel = (sel + n - 1) % n; if (pr & PAD_DOWN) sel = (sel + 1) % n;
    if (pr & PAD_CROSS) act(); return 1;
}
void skysea_draw(void) {
    char b[56]; static const char *const TN[2] = { "FLEET", "VOYAGES" };
    gfx_set_clear(110, 190, 240);
    ui_round(0, 0, 320, 240, 0, UI_RGB(140, 210, 255), UI_RGB(60, 150, 210), 0);
    ui_round(0, 150, 320, 90, 0, UI_RGB(30, 130, 190), UI_RGB(20, 90, 150), 0);
    for (int i = 0; i < 4; i++) ui_round((i * 91 + anim / 2) % 340 - 20, 20 + i * 14, 50, 14, 7, UI_RGB(255, 255, 255), UI_RGB(225, 235, 245), 0);   // clouds drift
    if (st == S_RUN || st == S_RES) {
        int top = ui_window(40, 20, 240, 190, YNM[ysel], IC_PIN);
        if (st == S_RUN) { sprintf(b, "LEG %d OF %d", leg + 1, YLEG[ysel]); ui_text(50, top, 1, b, UI_INK);
            ui_round(50, top + 22, 220, 16, 4, UI_RGB(200, 206, 214), UI_RGB(170, 176, 186), 0);
            ui_round(50 + zone * 220 / 100, top + 22, zw * 220 / 100, 16, 4, UI_RGB(255, 235, 120), UI_RGB(240, 190, 40), 0); ui_round(48 + mark * 220 / 100, top + 18, 5, 24, 2, UI_RGB(60, 70, 200), UI_RGB(30, 40, 140), 0);
            { int bx = 60 + leg * 190 / YLEG[ysel], by = top + 80 + (anim >> 3 & 3); ui_round(bx, by, 34, 10, 4, UI_RGB(240, 240, 250), UI_RGB(150, 160, 190), 0); ui_round(bx + 14, by - 14, 4, 14, 1, UI_RGB(120, 80, 40), UI_RGB(90, 60, 30), 0); }
            ui_wrap(50, top + 46, 220, 2, line, UI_INK); sprintf(b, "SMOOTH %d", hits); ui_text(50, top + 120, 1, b, UI_SOFT); ui_hints(160, 190, "X STEER", 1); }
        else { ui_wrap(50, top + 6, 220, 2, line, UI_GREEN); ui_wrap(50, top + 40, 220, 3, line2, UI_SOFT); sprintf(b, "CASH $%d", (int)cash); ui_text(50, top + 100, 1, b, UI_GOODT); ui_hints(160, 190, "X OK", 1); }
        return;
    }
    int top = ui_window(8, 6, 304, 228, "SKY AND SEA", IC_CAR);
    { int x = 14; for (int i = 0; i < 2; i++) { int w = ui_tw(1, TN[i]) + 14, on = i == tab; ui_round(x, top, w, 14, 4, on ? UI_BLUE : UI_PANEL, on ? UI_BLUE2 : UI_PANEL, 0); ui_text(x + 7, top + 3, 1, TN[i], on ? UI_WHITE : UI_SOFT); x += w + 4; } }
    sprintf(b, "$%d", (int)cash); ui_text(300 - ui_tw(1, b), top + 3, 1, b, UI_GOODT);
    int n = nrows(), first = CLAMP(sel - 3, 0, MAX(0, n - 7)), y0 = top + 20;
    for (int k = 0; k < 7 && first + k < n; k++) {
        int i = first + k, y = y0 + k * 15, on = i == sel; if (on) ui_round(14, y - 2, 292, 15, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
        if (tab == T_FLEET) {
            if (i == NV) { int d = MIN(30, MAX(0, day_count - last())); sprintf(b, "COLLECT  %d DAYS = $%d", d, rate() * d); ui_text(20, y + 1, 1, b, on ? UI_BLUE2 : UI_GOODT); continue; }
            int own = fleet() >> i & 1; ui_text(20, y + 1, 1, VNM[i], own ? UI_GOODT : on ? UI_BLUE2 : UI_INK);
            if (own) ui_text(268, y + 1, 1, "OWNED", UI_GOODT); else { sprintf(b, "$%d", VCOST[i]); ui_text(300 - ui_tw(1, b), y + 1, 1, b, cash >= VCOST[i] ? UI_INK : UI_BADT); }
        } else { int ok = (fleet() & YNEED[i]) != 0; ui_text(20, y + 1, 1, YNM[i], ok ? (on ? UI_BLUE2 : UI_INK) : UI_SOFT); sprintf(b, "PAYS $%d", YPAY[i]); ui_text(300 - ui_tw(1, b), y + 1, 1, b, ok ? UI_GOODT : UI_SOFT); }
    }
    int by = top + 128; ui_round(14, by, 292, 56, 5, UI_PANEL, UI_LINE, 0);
    if (tab == T_FLEET) { sprintf(b, "%d CRAFT  $%d A DAY", __builtin_popcount(fleet()), rate()); ui_text(20, by + 4, 1, b, UI_BLUE2); ui_wrap(20, by + 17, 280, 2, sel < NV ? VHINT[sel] : "YOUR CRAFT EARN EVERY DAY. COME BACK AND COLLECT.", UI_INK); }
    else { sprintf(b, "%d LEGS  FUEL $%d  VOYAGES %d", YLEG[sel], YFUEL[sel], pk_data[28]); ui_text(20, by + 4, 1, b, UI_BLUE2); ui_wrap(20, by + 17, 280, 2, (fleet() & YNEED[sel]) ? "PRESS X ON THE BRIGHT ZONE EACH LEG." : "BUY THE RIGHT CRAFT FIRST (BOAT OR AIRCRAFT).", UI_INK); }
    ui_text(20, by + 42, 1, line, UI_RED);
    ui_hints(160, 218, "X PICK|L1 R1 TABS|O LEAVE", 1);
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, skysea_open, skysea_update, skysea_draw };
