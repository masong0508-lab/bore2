// space_ui: the space program, an overlay (SPACE.OVL), opened from the computer. Upgrade a rocket (hull, engine, guidance), pick a mission, and fly the burns:
// press X when the marker is in the bright zone. A rough burn can still be survived by a tough hull. Missions pay once and a little less after.
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "wants.h"
#include "ovl.h"
#include <psxgpu.h>
#include <psxpad.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>

enum { S_MAIN, S_MISS, S_FLY, S_RES };
#define NM 6
static const char *const MNM[NM] = { "UP TO ORBIT", "THE MOON", "SPACE STATION", "MARS", "THE ASTEROIDS", "DEEP SPACE" };
static const int MNEED[NM] = { 0, 3, 5, 8, 10, 12 }, MFUEL[NM] = { 250, 900, 2200, 6000, 9000, 15000 }, MPAY[NM] = { 700, 2800, 6500, 22000, 38000, 80000 }, MSTG[NM] = { 3, 4, 5, 6, 7, 8 };
static const int MCERT[NM] = { 1, 2, 2, 3, 3, 4 };   // the certification each mission needs (sim.h: space_cert)
static const char *const TNM[3] = { "HULL", "ENGINE", "GUIDANCE" };
static const char *const THINT[3] = { "TOUGHER: SURVIVES ROUGH BURNS.", "CHEAPER FUEL. FASTER CLIMB.", "A WIDER BRIGHT ZONE." };
static const int UCOST[5] = { 400, 1200, 3000, 8000, 0 };
static int st, sel, msel, stage, mark, dir, zone, zw, tmr, res, anim, burns, rough, boom, fuelc;
static char line[44], line2[44];
static unsigned rs = 7;
static int rnd(int n) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (int)((rs >> 3) % (unsigned)n); }
static int tsum(void) { return space_tech[0] + space_tech[1] + space_tech[2]; }
void space_open(int a) { (void)a; st = S_MAIN; sel = 0; msel = 0; anim = 0; line[0] = line2[0] = 0; rs = (unsigned)(cash * 3 + day_count * 17 + 5); }
static void new_zone(void) { zw = 12 + space_tech[2] * 5 - msel + wants_skill_level(sims[player].skill_mech) / 2; if (zw < 8) zw = 8; zone = 10 + rnd(80 - zw); mark = rnd(100); dir = rnd(2) ? 1 : -1; tmr = 0; }
static void start_fly(void) {
    fuelc = MFUEL[msel] * (100 - space_tech[1] * 9) / 100; if (cash < fuelc) { strcpy(line, "NOT ENOUGH CASH FOR FUEL."); return; }
    if (space_cert() < MCERT[msel]) { snprintf(line, sizeof line, "NEEDS: %s", space_cert_name(MCERT[msel])); return; }
    if (tsum() < MNEED[msel]) { strcpy(line, "THE ROCKET IS NOT READY."); return; }
    cash -= fuelc; sims[player].skill_mech += 30 + msel * 10; sims[player].skill_logic += 20 + msel * 10; stage = 0; burns = 0; rough = 0; boom = 0; line[0] = 0; line2[0] = 0; st = S_FLY; new_zone(); audio_voice(VOICE_YAHOO);
}
static void finish_fly(void) {
    st = S_RES; int first = !(space_done & (1 << msel)); res = !boom;
    if (res) { int pay = first ? MPAY[msel] : MPAY[msel] / 3, flawless = !rough && burns >= MSTG[msel]; if (flawless) pay += pay / 4; cash += pay; space_done |= 1 << msel; astro_xp = (u16)MIN(60000, astro_xp + 10 + msel * 8);
        sprintf(line, first ? "MISSION COMPLETE! +$%d" : "ANOTHER GOOD FLIGHT +$%d", pay); strcpy(line2, flawless ? "A FLAWLESS FLIGHT: PAID A QUARTER MORE." : first ? "THE WORLD WATCHES. FIRST TIME IS FAMOUS." : "THE CREW WAVES. THE PAY IS SMALLER.");
        sims[player].mood = MIN(100, sims[player].mood + (first ? 80 : 30)); audio_voice(VOICE_FINISHED); }
    else { int lose = space_tech[0] > 0 ? 0 : 1; if (rnd(100) < 50) { int t = rnd(3); if (space_tech[t] > 0) space_tech[t]--; }
        strcpy(line, "THE ROCKET IS LOST. SOMETHING WAS SAVED."); strcpy(line2, lose ? "TIME TO REBUILD." : "A PART SURVIVED THE FALL."); (void)lose;
        sims[player].mood = MAX(-100, sims[player].mood - 30); audio_voice(VOICE_CRY); }
    clock_min = MIN(1439, clock_min + 90);
}
int space_update(int hd, int pr, int steps) {
    anim += steps; (void)hd;
    if (st == S_MAIN) { int n = 5; if (pr & PAD_UP) sel = (sel + n - 1) % n; if (pr & PAD_DOWN) sel = (sel + 1) % n; if (pr & PAD_CIRCLE) return 0;
        if (pr & PAD_CROSS) { line[0] = 0;
            if (sel == 0) { st = S_MISS; msel = 0; }
            else if (sel == 4) return 0;
            else { int t = sel - 1, lv = space_tech[t]; if (lv >= 4) strcpy(line, "ALREADY THE BEST."); else if (cash < UCOST[lv]) strcpy(line, "NOT ENOUGH CASH."); else { cash -= UCOST[lv]; space_tech[t]++; sims[player].skill_mech += 120; sprintf(line, "%s UPGRADED TO %d!", TNM[t], space_tech[t]); audio_voice(VOICE_AGREE); } } }
        return 1; }
    if (st == S_MISS) { if (pr & PAD_UP) msel = (msel + NM - 1) % NM; if (pr & PAD_DOWN) msel = (msel + 1) % NM; if (pr & PAD_CIRCLE) st = S_MAIN; if (pr & PAD_CROSS) start_fly(); return 1; }
    if (st == S_FLY) {
        int sp = 2 + stage / 2 + msel / 2 - space_tech[1] / 2; if (sp < 1) sp = 1; for (int i = 0; i < steps; i++) { mark += dir * sp; if (mark > 99) { mark = 99; dir = -1; } if (mark < 0) { mark = 0; dir = 1; } }
        if (tmr > 0) { tmr -= steps; if (tmr <= 0) { if (++stage >= MSTG[msel] || boom) finish_fly(); else new_zone(); } return 1; }
        if (pr & PAD_CROSS) { int d = ABS(mark - (zone + zw / 2));
            if (d <= zw / 2) { strcpy(line, "PERFECT BURN!"); burns++; }
            else if (d <= zw) { rough++; if (rnd(100) < space_tech[0] * 16 + 40 + wants_skill_level(sims[player].skill_mech) * 2) strcpy(line, "ROUGH BURN. THE HULL HOLDS."); else { strcpy(line, "ROUGH BURN! THE ROCKET BREAKS UP!"); boom = 1; } }
            else { if (rnd(100) < space_tech[0] * 14 + 12 + wants_skill_level(sims[player].skill_mech)) { strcpy(line, "A BAD BURN. THE HULL SURVIVES!"); rough++; } else { strcpy(line, "A BAD BURN! BOOM!"); boom = 1; } }
            tmr = 50; }
        return 1; }
    if (st == S_RES) { if (pr & (PAD_CROSS | PAD_CIRCLE)) st = S_MAIN; return 1; }
    return 1;
}
static void panel(void) {
    char b[48]; int top = ui_window(148, 8, 164, 224, "SPACE AGE", IC_STAR);
    sprintf(b, "CASH $%d", (int)cash); ui_text(156, top, 1, b, UI_GOODT);
    for (int t = 0; t < 3; t++) { ui_text(156, top + 14 + t * 11, 1, TNM[t], UI_INK); for (int i = 0; i < 4; i++) ui_round(220 + i * 18, top + 14 + t * 11, 15, 7, 2, i < space_tech[t] ? UI_RGB(60, 170, 230) : UI_RGB(200, 206, 214), i < space_tech[t] ? UI_RGB(30, 110, 190) : UI_RGB(170, 176, 186), 0); }
    ui_text(156, top + 48, 1, space_cert_name(space_cert()), UI_BLUE2);
    int done = 0; for (int i = 0; i < NM; i++) done += (space_done >> i) & 1; sprintf(b, "MISSIONS DONE %d OF %d", done, NM); ui_text(156, top + 59, 1, b, UI_SOFT);
}
void space_draw(void) {
    gfx_set_clear(2, 2, 10); ui_round(0, 0, 320, 240, 0, UI_RGB(4, 4, 22), UI_RGB(14, 10, 40), 0);
    for (int i = 0; i < 40; i++) { int x = (i * 53 + 11) % 146 + 2, y = (i * 97 + (st == S_FLY ? anim * (1 + i % 3) : anim / 3 * (1 + i % 2))) % 236, c = 120 + (i * 37) % 130; ui_round(x, y, 2, 2, 1, UI_RGB(c, c, c), UI_RGB(c, c, c), 0); }
    char b[48];
    if (st != S_FLY) { int rx = 70, ry = 70; ui_round(rx - 8, ry, 16, 60, 6, UI_RGB(235, 235, 245), UI_RGB(160, 165, 190), 0); ui_round(rx - 4, ry - 12, 8, 16, 4, UI_RGB(230, 60, 60), UI_RGB(160, 30, 30), 0);
        ui_round(rx - 16, ry + 40, 8, 22, 3, UI_RGB(60, 120, 220), UI_RGB(30, 70, 150), 0); ui_round(rx + 8, ry + 40, 8, 22, 3, UI_RGB(60, 120, 220), UI_RGB(30, 70, 150), 0);
        ui_round(rx - 4, ry + 14, 8, 8, 4, UI_RGB(120, 190, 250), UI_RGB(60, 120, 200), 0); }
    if (st == S_MAIN) { panel(); int top = 8 + 24 + 76; static const char *const mm[5] = { "MISSIONS", "UPGRADE HULL", "UPGRADE ENGINE", "UPGRADE GUIDANCE", "LEAVE" };
        for (int i = 0; i < 5; i++) { int y = top + i * 14; if (i == sel) ui_round(154, y - 2, 156, 13, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
            if (i >= 1 && i <= 3) { int lv = space_tech[i - 1]; sprintf(b, lv >= 4 ? "%s  MAX" : "%s  $%d", mm[i] + 8, UCOST[lv]); ui_text(160, y, 1, b, i == sel ? UI_BLUE2 : UI_INK); }
            else ui_text(160, y, 1, mm[i], i == sel ? UI_BLUE2 : UI_INK); }
        if (sel >= 1 && sel <= 3) ui_wrap(156, top + 76, 150, 2, THINT[sel - 1], UI_SOFT);
        ui_text(156, top + 96, 1, line, UI_RED); ui_hints(310, 218, "X PICK|O LEAVE", 2); return; }
    if (st == S_MISS) { panel(); int top = 8 + 24 + 76;
        for (int i = 0; i < NM; i++) { int y = top + i * 12; int lock = tsum() < MNEED[i] || space_cert() < MCERT[i]; if (i == msel) ui_round(154, y - 2, 156, 12, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
            ui_text(160, y, 1, MNM[i], lock ? UI_SOFT : i == msel ? UI_BLUE2 : UI_INK); if (space_done >> i & 1) ui_text(300, y, 1, "*", UI_GOODT); }
        sprintf(b, "FUEL $%d  PAYS $%d", MFUEL[msel] * (100 - space_tech[1] * 9) / 100, MPAY[msel]); ui_text(156, top + 76, 1, b, UI_INK);
        if (line[0]) ui_text(156, top + 88, 1, line, UI_RED);
        else if (space_cert() < MCERT[msel]) { sprintf(b, "NEEDS: %s", space_cert_name(MCERT[msel])); ui_text(156, top + 88, 1, b, UI_RED); }
        else if (tsum() < MNEED[msel]) { sprintf(b, "NEEDS PARTS TOTAL %d (YOU HAVE %d)", MNEED[msel], tsum()); ui_text(156, top + 88, 1, b, UI_RED); } ui_hints(310, 218, "X LAUNCH|O BACK", 2); return; }
    if (st == S_FLY) {   // the rocket climbs; the gauge sweeps
        int ry = 140 - stage * 10 + (tmr > 0 ? (anim & 2) : 0); ui_round(70 - 8, ry, 16, 50, 6, UI_RGB(235, 235, 245), UI_RGB(160, 165, 190), 0); ui_round(66, ry - 12, 8, 14, 4, UI_RGB(230, 60, 60), UI_RGB(160, 30, 30), 0);
        if (boom && tmr > 0) { ui_round(40, ry - 10, 60, 60, 30, UI_RGB(255, 220, 80), UI_RGB(255, 80, 20), 0); }
        else ui_round(66, ry + 50, 8, 18 + (anim & 7), 4, UI_RGB(255, 230, 120), UI_RGB(255, 100, 30), 0);
        int top = ui_window(148, 8, 164, 224, MNM[msel], IC_STAR); sprintf(b, "BURN %d OF %d", stage + 1, MSTG[msel]); ui_text(156, top, 1, b, UI_INK);
        ui_round(156, top + 20, 150, 14, 4, UI_RGB(200, 206, 214), UI_RGB(170, 176, 186), 0);
        ui_round(156 + zone * 150 / 100, top + 20, zw * 150 / 100, 14, 4, UI_RGB(255, 235, 120), UI_RGB(240, 190, 40), 0); ui_round(154 + mark * 150 / 100, top + 16, 5, 22, 2, UI_RGB(60, 70, 200), UI_RGB(30, 40, 140), 0);
        ui_wrap(156, top + 48, 150, 3, line, boom ? UI_RED : UI_INK); sprintf(b, "PERFECT %d  ROUGH %d", burns, rough); ui_text(156, top + 80, 1, b, UI_SOFT); ui_hints(310, 218, "X BURN", 2); return; }
    { int top = ui_window(148, 8, 164, 224, res ? "LANDED" : "LOST", IC_STAR); ui_wrap(156, top + 4, 150, 3, line, res ? UI_GREEN : UI_RED); ui_wrap(156, top + 40, 150, 3, line2, UI_SOFT); sprintf(b, "CASH $%d", (int)cash); ui_text(156, top + 80, 1, b, UI_GOODT); ui_hints(310, 218, "X BACK", 2); }
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, space_open, space_update, space_draw };
