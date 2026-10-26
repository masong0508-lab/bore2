// heist_ui: GRAND THEFT, the underworld of the CRIME AND LAW pack (it lives in PRISON.OVL, opened from the phone: UNDERWORLD).
// Pick a ride from the car lot, the harbour or the airfield and case the joint: three gates to sneak through (a fence, a guard, a lock), a gauge for
// each. Then the getaway: the police want you, and you run for it. Clean away, you KEEP the thing (it is hot) or FENCE it for cash. Caught, it goes on the
// record: CR_THEFT, evidence, and the arrest that leads into HARD TIME. Every theft leaves HEAT that cools with the days.
// A stolen boat or aircraft is kept in pk_data 40-41 (a bit each); SKY AND SEA lets you fly or sail it, if that pack is on the disc too. Neither needs the other.
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

enum { H_LIST, H_GATE, H_CHOICE, H_CHASE, H_END };
#define NT 10
static const char *const TNM[NT] = { "HATCHBACK", "PICKUP TRUCK", "SPORTS CAR", "MOTORBIKE", "POLICE CRUISER", "SPEEDBOAT", "YACHT", "BIPLANE", "SEAPLANE", "JET" };
static const char *const TWH[NT] = { "CAR LOT", "CAR LOT", "CAR LOT", "CAR LOT", "POLICE YARD", "HARBOUR", "HARBOUR", "AIRFIELD", "AIRFIELD", "AIRFIELD" };
static const u8 TDF[NT] = { 1, 1, 3, 2, 4, 3, 5, 4, 5, 6 }, TST[NT] = { 1, 1, 2, 2, 3, 2, 3, 3, 3, 5 };
static const int TVAL[NT] = { 400, 700, 2500, 900, 1800, 3500, 12000, 6000, 9000, 30000 };
static const char *const GNM[3] = { "THE FENCE", "THE GUARD", "THE LOCK" };
static const char *const GHINT[3] = { "CLIMB WHEN THE LIGHT IS AWAY.", "SLIP PAST WHEN HE LOOKS OFF.", "PICK IT IN THE QUIET MOMENT." };
static int hp, hsel, gate, alarm, tgt, stars, round_, miss, mark, dir, zone, zw, tmr, anim, good; static char line[48], line2[48];
static unsigned rs = 9;
static int rnd(int n) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (int)((rs >> 3) % (unsigned)n); }
static int lg(void) { return wants_skill_level(sims[player].skill_logic); }
static int stolen(void) { return pk_data[40] | pk_data[41] << 8; }
static int heat(void) { return pk_data[42]; }
void heist_open(void) { hp = H_LIST; hsel = 0; line[0] = line2[0] = 0; anim = 0; rs = (unsigned)(cash * 7 + day_count * 31 + 11);
    if (pk_data[43] != (u8)day_count) { int d = (u8)day_count - pk_data[43]; if (d < 0) d += 256; pk_data[42] = (u8)MAX(0, heat() - d * 12); pk_data[43] = (u8)day_count; } }
static void new_zone(int w) { zw = w; if (zw < 8) zw = 8; zone = 10 + rnd(80 - zw); mark = rnd(100); dir = rnd(2) ? 1 : -1; tmr = 0; }
static void busted(const char *why) {
    int n = 1 + stars / 2; for (int i = 0; i < n; i++) sim_crime(CR_THEFT, 14 + stars * 4);
    pk_data[42] = (u8)MIN(100, heat() + 20); sims[player].mood = MAX(-100, sims[player].mood - 30);
    sim_note(why); sim_arrest(0); hp = H_END; strcpy(line, why); strcpy(line2, "ON THE RECORD. THE COURT IS NEXT."); audio_voice(VOICE_CRY);
}
static void begin(int t) {
    tgt = t; gate = 0; alarm = 0; stars = TST[t] + (heat() > 50); miss = 0; hp = H_GATE; line[0] = 0;
    new_zone(30 + lg() * 2 - TDF[t] * 3); audio_voice(VOICE_YAHOO);
}
static void got_away(void) {
    sim_crime(CR_THEFT, 4 + stars * 2); pk_data[42] = (u8)MIN(100, heat() + 10 + stars * 6); pk_data[43] = (u8)day_count;
    sims[player].skill_logic += 30 + TDF[tgt] * 10; hp = H_CHOICE; hsel = 0; line[0] = 0;
    sim_note("YOU GOT AWAY WITH IT!"); audio_voice(VOICE_WIN);
}
static void finish_gate(int ok) {
    if (ok) { strcpy(line, "CLEAN!"); } else { alarm++; strcpy(line, "A SLIP! SOMETHING CLANGED."); stars = MIN(5, stars + 1); }
    tmr = 40;
}
int heist_update(int hd, int pr, int steps) {
    (void)hd; anim += steps;
    if (hp == H_LIST) {
        if (pr & PAD_CIRCLE) return 0;
        if (pr & PAD_UP) hsel = (hsel + NT - 1) % NT; if (pr & PAD_DOWN) hsel = (hsel + 1) % NT;
        if (pr & PAD_CROSS) { if (sims[player].bore.stage < AG_TEEN) { strcpy(line, "TOO YOUNG FOR THIS."); return 1; } begin(hsel); }
        return 1; }
    if (hp == H_GATE) {
        int sp = 2 + gate + TDF[tgt] / 2 - lg() / 4; if (sp < 1) sp = 1;
        for (int i = 0; i < steps; i++) { mark += dir * sp; if (mark > 99) { mark = 99; dir = -1; } if (mark < 0) { mark = 0; dir = 1; } }
        if (tmr > 0) { tmr -= steps; if (tmr <= 0) {
            if (alarm >= 2) { busted("CAUGHT RED-HANDED!"); return 1; }
            if (++gate >= 3) { if (alarm) { hp = H_CHASE; round_ = 0; miss = 0; new_zone(36 + lg() * 2 - stars * 3); line[0] = 0; strcpy(line2, "THE ALARM IS RINGING! RUN!"); } else got_away(); }
            else new_zone(30 + lg() * 2 - TDF[tgt] * 3 - gate * 2); }
            return 1; }
        if (pr & PAD_CROSS) { int d = ABS(mark - (zone + zw / 2)); finish_gate(d <= zw / 2 + 2); }
        return 1; }
    if (hp == H_CHOICE) {
        if (pr & (PAD_UP | PAD_DOWN)) hsel ^= 1;
        if (pr & PAD_CROSS) {
            if (hsel == 0) { if (tgt >= 5) { pk_data[40 + (tgt >> 3)] |= (u8)(1 << (tgt & 7)); sprintf(line, "KEPT: THE %s (HOT).", TNM[tgt]); } else { int pay = TVAL[tgt] * (100 + lg() * 3) / 200; cash += pay; sprintf(line, "A CAR CAN'T BE KEPT HERE. SOLD $%d", pay); } }
            else { int pay = TVAL[tgt] * (100 + lg() * 3) / 100; cash += pay; sprintf(line, "FENCED FOR $%d", pay); audio_voice(VOICE_WIN); }
            hp = H_END; strcpy(line2, heat() > 60 ? "THE HEAT IS ON. LAY LOW." : "QUIET, FOR NOW."); }
        return 1; }
    if (hp == H_CHASE) {
        int sp = 3 + round_ + stars; if (sp < 2) sp = 2;
        for (int i = 0; i < steps; i++) { mark += dir * sp; if (mark > 99) { mark = 99; dir = -1; } if (mark < 0) { mark = 0; dir = 1; } }
        if (tmr > 0) { tmr -= steps; if (tmr <= 0) { if (miss >= 2) busted("THE POLICE GOT YOU!"); else if (++round_ >= stars + 1) got_away(); else new_zone(36 + lg() * 2 - stars * 3 - round_); } return 1; }
        if (pr & PAD_CROSS) { int d = ABS(mark - (zone + zw / 2)); if (d <= zw / 2 + 2) strcpy(line, "LOST THEM AT THE CORNER!"); else { miss++; strcpy(line, "THEY ARE ON YOUR TAIL!"); } tmr = 40; }
        return 1; }
    if (pr & (PAD_CROSS | PAD_CIRCLE)) return 0;
    return 1;
}
void heist_draw(void) {
    char b[56]; gfx_set_clear(10, 10, 30);
    ui_round(0, 0, 320, 240, 0, UI_RGB(14, 14, 34), UI_RGB(30, 20, 50), 0);
    for (int i = 0; i < 6; i++) ui_round(10 + i * 52, 150 - (i * 37) % 50, 30, 90, 0, UI_RGB(24, 24, 46), UI_RGB(18, 18, 36), 0);   // a skyline at night
    int top = ui_window(30, 10, 260, 218, "UNDERWORLD", IC_LOCK);
    if (hp == H_LIST) {
        ui_text(36, top, 1, "PICK A RIDE. ANY NIGHT.", UI_SOFT); sprintf(b, "HEAT %d", heat()); ui_text(284 - ui_tw(1, b), top, 1, b, heat() > 50 ? UI_BADT : UI_GOODT);
        int first = CLAMP(hsel - 4, 0, NT - 8);
        for (int k = 0; k < 8; k++) { int i = first + k, y = top + 14 + k * 14, on = i == hsel; if (on) ui_round(36, y - 2, 248, 14, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
            sprintf(b, "%s  %s", TNM[i], TWH[i]); ui_text(40, y, 1, b, on ? UI_BLUE2 : UI_INK); sprintf(b, "$%d", TVAL[i]); ui_text(280 - ui_tw(1, b), y, 1, b, UI_GOODT); }
        sprintf(b, "SECURITY %d  WANTED %d  YOUR LOGIC %d", TDF[hsel], TST[hsel], lg()); ui_text(36, top + 130, 1, b, UI_BLUE2);
        ui_wrap(36, top + 144, 244, 2, hsel >= 5 ? "THE THING CAN BE KEPT: HOT. SKY AND SEA CAN TAKE IT OUT." : "A CAR OR BIKE GOES TO A CHOP SHOP FOR CASH.", UI_SOFT);
        ui_text(36, top + 166, 1, line, UI_RED); ui_hints(160, 214, "X CASE IT|O LEAVE", 1); return; }
    if (hp == H_GATE || hp == H_CHASE) {
        if (hp == H_GATE) { ui_text(36, top, 1, TNM[tgt], UI_INK); sprintf(b, "GATE %d OF 3: %s", gate + 1, GNM[gate]); ui_text(36, top + 14, 1, b, UI_BLUE2); ui_wrap(36, top + 28, 244, 2, GHINT[gate], UI_SOFT); }
        else { ui_text(36, top, 1, "THE GETAWAY", UI_BADT); sprintf(b, "CORNER %d OF %d", round_ + 1, stars + 1); ui_text(36, top + 14, 1, b, UI_BLUE2); ui_wrap(36, top + 28, 244, 2, line2, UI_SOFT); }
        ui_round(40, top + 66, 236, 16, 4, UI_RGB(200, 206, 214), UI_RGB(170, 176, 186), 0);
        ui_round(40 + zone * 236 / 100, top + 66, zw * 236 / 100, 16, 4, UI_RGB(255, 235, 120), UI_RGB(240, 190, 40), 0); ui_round(38 + mark * 236 / 100, top + 62, 5, 24, 2, UI_RGB(60, 70, 200), UI_RGB(30, 40, 140), 0);
        for (int i = 0; i < 5; i++) ui_icon(40 + i * 16, top + 100, IC_STAR, i < stars ? UI_RGB(236, 170, 30) : UI_RGB(110, 110, 130));   // the wanted stars
        ui_wrap(36, top + 124, 244, 2, line, UI_INK); ui_hints(160, 214, "X NOW", 1); return; }
    if (hp == H_CHOICE) { ui_text(36, top, 1, "YOU'RE IN. NOW WHAT?", UI_INK); const char *o[2] = { tgt >= 5 ? "KEEP IT. IT IS HOT." : "SELL IT TO THE CHOP SHOP", "FENCE IT FOR CASH" };
        for (int i = 0; i < 2; i++) { int y = top + 24 + i * 18; if (i == hsel) ui_round(36, y - 2, 248, 16, 4, UI_RGB(226, 242, 254), UI_HILITE, 0); ui_text(42, y + 1, 1, o[i], i == hsel ? UI_BLUE2 : UI_INK); }
        ui_hints(160, 214, "X PICK", 1); return; }
    ui_wrap(36, top + 6, 244, 3, line, UI_INK); ui_wrap(36, top + 44, 244, 3, line2, UI_SOFT); sprintf(b, "CASH $%d   HEAT %d", (int)cash, heat()); ui_text(36, top + 100, 1, b, UI_GOODT); ui_hints(160, 214, "X OK", 1);
}
