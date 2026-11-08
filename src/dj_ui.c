// dj_ui.c - the DJ booth: DJ.OVL, an overlay of its own on the disc (ovl.h). Eight of the songs, cut to excerpts, on two tracker decks
// (audio.c dj_*). You cue a track, mix it in and play the tricks; or let the booth run a set and ask it for requests. Nothing here shows
// your skill, but it is behind every move: a beginner's tempo is a little off, their cue lands late, their fades come in steps and the odd
// mix is a train wreck; as the hidden skill builds, all of it tightens up, and the better tricks (echo, kills, brake, drop, scratch) open.
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "ovl.h"
#include "citizens.h"
#include <psxgpu.h>
#include <psxgte.h>
#include <stdio.h>
#include <string.h>

extern int dj_xp, dj_heat;
enum { T_ECHO = 120, T_KILL = 250, T_BRAKE = 400, T_DROP = 550, T_SCRATCH = 700 };   // the skill each trick opens at
static int mixOld, anim, sel, view, rsel, msgT, fader, mixing, mixBar0, mixRow, mixLen, mixStep, autoOn, wreck, killg, brakeS, brakeT, dropS, dropT0, echoT, played, xpT, heatT;
static const char *msg;
static int reqTrack[3], reqT[3], reqName[3], reqN, reqGap, askQ[3], askN, lastBars[2], act0[2], pulse[2];
static u32 rng;
static const char *const askers[12] = { "MISSY", "SUNMAN", "NOVA", "PIXEL", "JUNO", "BARRY", "COLA", "WHISTLER", "BOMB", "CLOUD", "MARS", "DIXIE" };
static const u16 killMask[5] = { 0, 0x000F, 0x00F0, 0x0F00, 0xF000 };
static const char *const killNm[5] = { "KILL", "KICK OUT", "BASS OUT", "MIDS OUT", "TOPS OUT" };
static int rr(int n) { rng = rng * 1103515245u + 12345u; return (int)((rng >> 16) % (u32)n); }
static void say(const char *s) { msg = s; msgT = 170; }
static int skill(void) { return dj_xp < 0 ? 0 : dj_xp > 1000 ? 1000 : dj_xp; }
static int rough(int s, int top) { int r = 1000 - s; return top * r * r / 1000000; }   // top x (1 - skill) squared: how much a beginner's hand misses by
static void xp_add(int n) {
    int before = skill(); dj_xp = MIN(1000, dj_xp + n);
    static const struct { int at; const char *s; } nt[5] = { { T_ECHO, "NEW TRICK: ECHO. HOLD L2 TO WASH THE SOUND" }, { T_KILL, "NEW TRICK: KILLS. SQUARE DROPS OUT PART OF A TRACK" }, { T_BRAKE, "NEW TRICK: THE BRAKE. L1 SLOWS THE RECORD TO A STOP" }, { T_DROP, "NEW TRICK: THE DROP. R1 CUTS IT, THEN SLAMS IT BACK" }, { T_SCRATCH, "NEW TRICK: SCRATCHING. HOLD R2 AND WIGGLE LEFT AND RIGHT" } };
    for (int i = 0; i < 5; i++) if (before < nt[i].at && dj_xp >= nt[i].at) say(nt[i].s);
}
static int master(void) { int a = dj_state(0) == 3 && dj_pos(0, 6), b = dj_state(1) == 3 && dj_pos(1, 6); if (a && b) return dj_gain(1) > dj_gain(0) ? 1 : 0; return b ? 1 : a ? 0 : -1; }
static int free_deck(void) { int m = master(); if (m < 0) return dj_state(0) == 1 || dj_state(0) == 2 ? 1 : 0; return m ^ 1; }

static const char *tipText(void) {
    static const char *const t[8] = {
        "X QUEUES A TRACK ON THE SPARE DECK",
        "TRIANGLE MIXES IT IN. LEFT AND RIGHT FADE",
        "START: THE BOOTH PLAYS. SELECT: A REQUEST",
        "BEGINNERS DRIFT OFF THE BEAT. PRACTICE HELPS",
        "GUESTS SHOUT REQUESTS. PLAY ONE FOR A TIP",
        "A SMOOTH FADE KEEPS THE SET HEAT UP",
        "NEW TRICKS OPEN UP AS YOU GET BETTER",
        "A GREAT SET LIFTS A WHOLE PARTY" };
    return t[(anim / 600) & 7];
}

void dj_open(int arg) {
    rng = 0x9E3779B1u ^ (u32)dj_xp * 977u; anim = 0; sel = 0; view = 0; rsel = 0; msgT = 0; msg = 0; mixing = 0; autoOn = 0; wreck = 0; killg = 0; brakeS = 0; dropS = 0; echoT = 0;
    reqN = 0; reqGap = 600; askN = 0; played = 0; xpT = 0; heatT = 0; fader = dj_state(1) == 3 ? (dj_state(0) == 3 ? dj_gain(1) * 256 / (dj_gain(0) + dj_gain(1) + 1) : 256) : 0;
    if (!dj_active()) { dj_begin(); if (!dj_active()) say("NO DISC AUDIO TO MIX WITH"); }
    for (int i = 0; i < 2; i++) { lastBars[i] = dj_pos(i, 4); act0[i] = dj_pos(i, 5); pulse[i] = 0; }
    if (!dj_heat) dj_heat = 20;
}
static void leave_(void) {   // out of the booth: a fade in the middle is finished, the music plays on
    if (mixing) { int m = master(); if (m >= 0 && dj_state(m ^ 1) == 3) { dj_fader(m ^ 1, 256); dj_stop(m); } mixing = 0; }
    dj_echo(0); for (int d = 0; d < 2; d++) if (dj_state(d) == 3) { dj_pitch(d, 256); if (!dropS) dj_mute(d, 0); }
    if (brakeS) for (int d = 0; d < 2; d++) if (dj_state(d) == 3) dj_set_tempo(d, 256);
}
static int cut_bars(int t) {   // how much of the song you get: a beginner's cuts are short (half a minute), a master can ride one past two minutes
    int base = dj_bars(t), s = skill(), f = 40 + s * 260 / 1000;   // 0.4x .. 3x of the usual minute
    f += rr(31) - 15; if (f < 35) f = 35;
    return base * f / 100;
}
static void cue_to(int t) { int d = free_deck(); if (dj_state(d) == 3) { say("BOTH DECKS ARE BUSY"); return; } if (dj_cue(d, t, cut_bars(t))) { sel = t; } else say("THE DECKS ARE STILL LOADING"); }
static void start_mix(void) {
    int d = free_deck(), s = skill();
    if (master() < 0) d = dj_state(0) == 2 ? 0 : 1;
    if (dj_state(d) != 2) { say(dj_state(d) == 1 ? "STILL LOADING THE TRACK..." : "QUEUE A TRACK FIRST (X)"); return; }
    int m = master();
    if (m < 0) { dj_fader(d, 256); dj_launch(d, 0); fader = d ? 256 : 0; say("THE FIRST TRACK IS ON"); return; }
    int e = (rr(2) ? 1 : -1) * rough(s, 34) * 256 / 100, rows = dj_pos(m, 2);   // a beginner's tempo can be a third out; their cue lands rows late
    int late = rough(s, rows * 2 / 5) * (1 + rr(2));
    wreck = rr(1000) < rough(s, 380);
    dj_sync(d, e); dj_launch(d, late); dj_fader(d, 0);
    mixing = 1; mixOld = m; mixBar0 = dj_pos(m, 4); mixRow = 0; mixLen = s > 700 ? 2 : s > 350 ? 3 : 2 + rr(3); mixStep = 3 + s / 80;
    fader = m ? 256 : 0;   // (the fader sits on the deck that is playing; it travels to the new one)
    say(late ? "IT'S IN... A BIT LATE" : "ON THE NEXT BAR");
}
static void mix_done(int oldd, int q) {   // the old deck goes; how it went decides the heat and the skill
    dj_stop(oldd); mixing = 0; played++;
    if (q >= 70) { dj_heat = MIN(100, dj_heat + 12); xp_add(14); say("SMOOTH. THE FLOOR LOVES IT"); }
    else if (q >= 40) { dj_heat = MIN(100, dj_heat + 5); xp_add(9); say("DECENT MIX"); }
    else { dj_heat = MAX(0, dj_heat - 14); xp_add(5); say("ROUGH ONE. THE CROWD WINCED"); }
}
static void fade_gains(int toB) {   // the fader to two volumes; a smooth curve for a good hand, a straight line for a bad one
    int x = fader; if (x < 0) x = 0; if (x > 256) x = 256;
    int a, b; if (skill() > 300) { a = 256 - x * x / 256; b = 256 - (256 - x) * (256 - x) / 256; } else { a = 256 - x; b = x; }
    if (wreck && mixing) { a = 256; b = 256; }   // (a train wreck: both decks flat out)
    dj_fader(0, a); dj_fader(1, b);
}

static void requests_tick(int steps) {
    for (int i = 0; i < reqN; i++) reqT[i] -= steps;
    for (int i = 0; i < reqN; ) { if (reqT[i] <= 0) { for (int j = i; j < reqN - 1; j++) { reqTrack[j] = reqTrack[j + 1]; reqT[j] = reqT[j + 1]; reqName[j] = reqName[j + 1]; } reqN--; say("A REQUEST GOT AWAY"); dj_heat = MAX(0, dj_heat - 4); } else i++; }
    int m = master();
    if (m >= 0) {
        for (int i = 0; i < reqN; i++) if (dj_track(m) == reqTrack[i] && dj_gain(m) >= 170) {
            int tip = 12 + skill() / 25; extern void sim_dj_hit(int tip); sim_dj_hit(tip);
            char b[48]; sprintf(b, "%s LOVES IT! $%d TIP", askers[reqName[i]], tip); say(b); xp_add(25); dj_heat = MIN(100, dj_heat + 10);
            for (int j = i; j < reqN - 1; j++) { reqTrack[j] = reqTrack[j + 1]; reqT[j] = reqT[j + 1]; reqName[j] = reqName[j + 1]; } reqN--; break; }
        if (reqN < 2 && (reqGap -= steps) <= 0) { reqGap = 1100 + rr(900); int t = rr(8), tries = 0; while ((t == dj_track(m) || t == dj_track(m ^ 1)) && tries++ < 8) t = rr(8); reqTrack[reqN] = t; reqT[reqN] = 3200; reqName[reqN] = rr(12); reqN++; }
    }
}
static void auto_dj(void) {   // the booth's own hand: a set that keeps going, asked-for tracks first
    int m = master(); if (m < 0) { if (dj_state(0) == 0 && dj_state(1) == 0) { int t = askN ? askQ[0] : rr(8); if (askN) { for (int j = 0; j < askN - 1; j++) askQ[j] = askQ[j + 1]; askN--; } cue_to(t); } else if (dj_state(0) == 2 || dj_state(1) == 2) start_mix(); return; }
    if (mixing) return;
    int o = m ^ 1;
    if (dj_state(o) == 0 && dj_pos(m, 4) >= 3) { int t = askN ? askQ[0] : rr(8), tries = 0; if (askN) { for (int j = 0; j < askN - 1; j++) askQ[j] = askQ[j + 1]; askN--; } else while (t == dj_track(m) && tries++ < 8) t = rr(8); cue_to(t); }
    else if (dj_state(o) == 2 && dj_pos(m, 4) >= 6 && dj_pos(m, 1) < 4) { int keep = dj_xp; dj_xp = MAX(dj_xp, 650); start_mix(); dj_xp = keep; }
}

int dj_update(int hd, int pr, int steps) {
    anim += steps; if (msgT > 0) { msgT -= steps; if (msgT <= 0) { msgT = 0; msg = 0; } }
    if (!dj_active()) return (pr & PAD_CIRCLE) ? 0 : 1;
    int s = skill(), m = master();
    if (view == 1) {   // asking the booth for a track
        if (pr & PAD_UP) rsel = (rsel + 7) & 7; if (pr & PAD_DOWN) rsel = (rsel + 1) & 7;
        if (pr & PAD_CIRCLE) view = 0;
        if (pr & PAD_CROSS) { if (askN < 3) { askQ[askN++] = rsel; autoOn = 1; say("ASKED FOR IT. THE BOOTH WILL SLOT IT IN"); } else say("THE BOOTH HAS A FULL QUEUE"); view = 0; }
        return 1;
    }
    if (pr & PAD_CIRCLE) { leave_(); return 0; }
    if (pr & PAD_UP) sel = (sel + 7) & 7; if (pr & PAD_DOWN) sel = (sel + 1) & 7;
    if (pr & PAD_CROSS) cue_to(sel);
    if (pr & PAD_TRIANGLE) start_mix();
    if (pr & PAD_START) { autoOn ^= 1; say(autoOn ? "THE BOOTH TAKES THE DECKS" : "THE DECKS ARE YOURS"); }
    if (pr & PAD_SELECT) { view = 1; rsel = sel; }
    // the fader by hand (this takes a mix over)
    if (hd & (PAD_LEFT | PAD_RIGHT)) {
        if (!(hd & PAD_R2)) { if (mixing) { mixing = 2; wreck = 0; } fader += (hd & PAD_RIGHT ? 1 : -1) * 6 * steps; fader = fader < 0 ? 0 : fader > 256 ? 256 : fader; }
    }
    if (mixing == 1) { int o = mixOld;   // the booth carries the fade over the bars: smooth for a good hand, in steps for a bad one
        if (dj_pending(o ^ 1)) { mixRow = 0; }   // (waiting for the bar)
        else {
            if (!mixRow) { mixRow = 1; mixBar0 = dj_pos(o, 4); }                          // the new deck came in: the fade is timed from here
            int bars = dj_pos(o, 4) - mixBar0, rows = MAX(1, dj_pos(o, 2));
            int prog = (bars * rows + dj_pos(o, 1)) * 256 / (mixLen * rows); prog = prog > 256 ? 256 : prog < 0 ? 0 : prog;
            if (wreck && prog < 200) prog = prog / 3;                                      // a train wreck drags on
            prog = prog * mixStep / 256 * 256 / mixStep;                                   // (a beginner's fader moves in notches)
            fader = (o ? 256 - prog : prog);
            if (prog >= 256) { int q = 100 - rough(s, 100) - (wreck ? 60 : 0); mix_done(o, q); }
        }
    }
    if (mixing == 2) { if (fader == 0 || fader == 256) { int newd = fader ? 1 : 0; if (dj_state(newd) == 3) { int q = 78 - rough(s, 40); mix_done(newd ^ 1, q); } else mixing = 0; } }
    fade_gains(0);
    // the tricks
    if (s >= T_ECHO) { if (hd & PAD_L2) { dj_echo(1); echoT = 1; } else if (echoT) { echoT = 0; dj_echo(0); } } else if ((hd & PAD_L2) && !(anim & 127)) say("ECHO UNLOCKS WITH PRACTICE");
    if (m >= 0 && s >= T_KILL && (pr & PAD_SQUARE)) { killg = (killg + 1) % 5; dj_mute(m, killMask[killg]); say(killg ? killNm[killg] : "EVERYTHING BACK IN"); xp_add(1); }
    if (m >= 0 && s >= T_BRAKE && (pr & PAD_L1)) { brakeS = brakeS ? 2 : 1; brakeT = 0; xp_add(1); }
    if (m >= 0 && brakeS) {
        int base = 256, cur = dj_tempo(m);
        if (brakeS == 1) { cur -= 6 * steps; if (cur <= 40) { cur = 40; } dj_set_tempo(m, cur); }
        else { cur += 10 * steps; int hit = base + (rr(2) ? 1 : -1) * rough(s, 18) * 256 / 100; if (cur >= hit) { cur = hit; brakeS = 0; } dj_set_tempo(m, cur); }
    }
    if (m >= 0 && s >= T_DROP && (pr & PAD_R1) && !dropS) { dropS = 1; dropT0 = dj_pos(m, 4); dj_mute(m, 0xFFFF); xp_add(1); }
    if (dropS && m >= 0) { int need = 1 + (s < 800 ? rr(2) : 0) * (s < 800); if (dj_pos(m, 4) - dropT0 >= need && dj_pos(m, 1) < 3) { dropS = 0; dj_mute(m, killMask[killg]); dj_heat = MIN(100, dj_heat + 3); say("BACK IN!"); } }
    if (m >= 0 && s >= T_SCRATCH && (hd & PAD_R2)) { int w = (hd & PAD_RIGHT) ? 1 : (hd & PAD_LEFT) ? -1 : 0; int pm = 256 + w * (60 + (anim & 7) * 14) + (isin(anim * 160 & 4095) >> 5); dj_pitch(m, pm); pulse[m] = 6; }
    else if (m >= 0 && dj_pos(m, 6)) dj_pitch(m, 256);
    // the clock: the skill creeps up while the decks run, the heat cools
    if (m >= 0) { xpT += steps; if (xpT > 900) { xpT = 0; xp_add(1); } heatT += steps; if (heatT > 600) { heatT = 0; if (dj_heat > 5) dj_heat--; } }
    if (!mixing) requests_tick(steps); else requests_tick(steps / 2 + 1);
    if (autoOn) auto_dj();
    return 1;
}

// ---- the picture ----
static void sky_(void) { gfx_grad(0, 0, 320, 240, UI_RGB(18, 12, 44), UI_RGB(18, 12, 44), UI_RGB(60, 20, 92), UI_RGB(60, 20, 92), 0); for (int i = 0; i < 28; i++) { int x = (i * 83 + anim / 3 * (1 + (i & 1))) % 320, y = (i * 47) % 150; gfx_rect(x, y, 1 + (i & 1), 1, 200, 190, 255, 0); } }
static void platter(int cx, int cy, int r, int d, int playing, u32 col) {
    ui_disc(cx, cy, r * 2 + 4, UI_RGB(10, 8, 20)); ui_disc(cx, cy, r * 2, UI_RGB(30, 26, 52));
    for (int k = 0; k < 6; k++) { int a = (anim * (playing ? 7 : 0) + k * 683 + d * 300) & 4095; int x = cx + (icos(a) * (r - 3) >> 12), y = cy + (isin(a) * (r - 3) >> 12); gfx_rect(x - 1, y - 1, 3, 3, (col & 255), (col >> 8) & 255, (col >> 16) & 255, 0); }
    ui_disc(cx, cy, r, col); ui_disc(cx, cy, 5, UI_RGB(18, 12, 40));
}
static void deck_card(int d, int x, int y) {
    static const char *const stNm[4] = { "EMPTY", "LOADING", "READY", "ON AIR" };
    int st = dj_state(d), tr = dj_track(d), live = st == 3 && dj_pos(d, 6);
    ui_round(x, y, 142, 72, 8, UI_WHITE, UI_PANEL, 0);
    u32 col = d ? UI_RGB(236, 90, 160) : UI_RGB(60, 160, 236);
    platter(x + 30, y + 30, 20, d, live, col);
    char b[24]; sprintf(b, "DECK %c", 'A' + d); ui_text(x + 62, y + 5, 1, b, d ? UI_RGB(190, 40, 110) : UI_BLUE2);
    ui_badge(x + 62, y + 16, stNm[st], st == 3 ? UI_GREEN : st == 2 ? UI_AMBER : UI_LINE, st >= 2 ? UI_WHITE : UI_SOFT);
    if (tr >= 0) ui_wrap(x + 62, y + 29, 76, 2, dj_track_name(tr), UI_INK); else ui_text(x + 62, y + 32, 1, "PICK A TRACK", UI_SOFT);
    if (st == 3) {   // where in the excerpt it is, and the bar's beats
        int n = MAX(1, dj_pos(d, 3)), o = dj_pos(d, 0); ui_bar(x + 8, y + 56, 126, 5, (o * 100 + dj_pos(d, 1) * 100 / MAX(1, dj_pos(d, 2))) / n, col);
        int rows = MAX(1, dj_pos(d, 2)), beat = dj_pos(d, 1) * 4 / rows; for (int k = 0; k < 4; k++) gfx_rect(x + 62 + k * 10, y + 50, 8, 4, k == beat ? 255 : 120, k == beat ? 220 : 120, k == beat ? 80 : 130, 0);
        { int a = dj_pos(d, 5) - act0[d]; if (a != 0) { pulse[d] = MIN(10, pulse[d] + MIN(4, ABS(a))); act0[d] = dj_pos(d, 5); } }
    }
    for (int k = 0; k < 8; k++) { int h = live ? 3 + ((pulse[d] * (k * 7 + 3) + anim * 3 + k * 5) % 9) : 2; if (live && (anim & 3) == 0 && pulse[d] > 0) {} gfx_rect(x + 8 + k * 6, y + 70 - h, 4, h, col & 255, (col >> 8) & 255, (col >> 16) & 255, 0); }
    if (pulse[d] > 0 && !(anim & 3)) pulse[d]--;
}
void dj_draw(void) {
    sky_();
    int top = ui_window_s(8, 6, 304, 206, "THE DECKS") + 1;
    int s = skill(), m = master();
    { char b[24]; sprintf(b, "SET HEAT %d", dj_heat); ui_badge(300 - ui_tw(1, b) - 10, 8, b, dj_heat > 66 ? UI_RED : dj_heat > 33 ? UI_AMBER : UI_BLUE2, UI_WHITE); }
    deck_card(0, 14, top + 1); deck_card(1, 164, top + 1);
    // the crossfader
    int fy = top + 76; ui_round(14, fy, 292, 14, 6, UI_WHITE, UI_PANEL, 0); ui_text(20, fy + 3, 1, "A", UI_BLUE2); ui_text(292, fy + 3, 1, "B", UI_RGB(190, 40, 110));
    ui_bar(34, fy + 5, 250, 4, 0, UI_LINE); gfx_rect(34 + fader * 240 / 256, fy + 1, 10, 12, 250, 250, 255, 0); gfx_rect(36 + fader * 240 / 256, fy + 3, 6, 8, mixing ? 255 : 90, mixing ? 150 : 140, mixing ? 60 : 230, 0);
    if (mixing) ui_badge(120, fy - 8, mixing == 2 ? "YOUR HAND" : "BLENDING", UI_AMBER, UI_WHITE);
    // the tracks
    int ty = fy + 17; ui_round(14, ty, 188, 62, 7, UI_WHITE, UI_PANEL, 0);
    for (int k = 0; k < 3; k++) {
        int i = (sel / 3) * 3 + k; if (i >= 8) break; int y = ty + 3 + k * 19, on = i == sel;
        ui_round(18, y, 180, 17, 5, on ? UI_HILITE : UI_WHITE, on ? UI_HILITE : UI_PANEL, 0);
        ui_textfit(24, y + 5, 134, dj_track_name(i), UI_INK);
        for (int d = 0; d < 2; d++) if (dj_track(d) == i && dj_state(d) >= 1) ui_badge(158 + d * 18, y + 3, d ? "B" : "A", d ? UI_RGB(236, 90, 160) : UI_BLUE, UI_WHITE);
    }
    // the tricks (locked ones greyed)
    static const struct { const char *n; int at; } fx[5] = { { "ECHO", T_ECHO }, { "KILLS", T_KILL }, { "BRAKE", T_BRAKE }, { "DROP", T_DROP }, { "SCRATCH", T_SCRATCH } };
    ui_round(208, ty, 98, 62, 7, UI_WHITE, UI_PANEL, 0);
    for (int i = 0; i < 5; i++) { int on = s >= fx[i].at; int hot = (i == 0 && echoT) || (i == 1 && killg) || (i == 2 && brakeS) || (i == 3 && dropS); ui_badge(214, ty + 3 + i * 11, fx[i].n, hot ? UI_RED : on ? UI_BLUE2 : UI_LINE, on ? UI_WHITE : UI_SOFT); if (!on) ui_icon(286, ty + 2 + i * 11, IC_LOCK, UI_SOFT); }
    // requests, or the hint
    int ry = ty + 66;
    if (reqN) {
        for (int i = 0; i < reqN; i++) { char b[64]; sprintf(b, "%s: \"%s\"", askers[reqName[i]], dj_track_name(reqTrack[i])); ui_round(14, ry + i * 14, 292, 13, 6, UI_RGB(255, 244, 200), UI_RGB(255, 232, 160), 0); ui_textfit(20, ry + i * 14 + 3, 240, b, UI_INK); ui_bar(264, ry + i * 14 + 4, 36, 5, reqT[i] * 100 / 3200, UI_AMBER); }
    } else ui_textfit(16, ry + 3, 288, tipText(), UI_SOFT);
    if (autoOn) ui_badge(14, top - 1, "BOOTH ON", UI_GREEN, UI_WHITE);
    if (view == 1) {   // ask for a request
        ui_dim(); int t = ui_window(40, 40, 240, 156, "ASK THE BOOTH", IC_NOTE);
        for (int k = 0; k < 8; k++) { int y = t + 2 + k * 14; ui_round(48, y, 224, 13, 5, k == rsel ? UI_HILITE : UI_WHITE, k == rsel ? UI_HILITE : UI_PANEL, 0); ui_textfit(54, y + 3, 150, dj_track_name(k), UI_INK); ui_textfit(190, y + 3, 78, dj_track_real(k), UI_SOFT); }
        ui_hints(160, 202, "X ASK|O BACK", 1);
    } else ui_hints(160, 213, "X CUE|TRI MIX|SEL ASK|O EXIT", 1);
    if (msg) { int w = ui_tw(1, msg) + 16; ui_round(160 - w / 2, 40, w, 16, 7, UI_WHITE, UI_PANEL, 0); ui_textc(160, 45, 1, msg, UI_INK); }
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, dj_open, dj_update, dj_draw };
