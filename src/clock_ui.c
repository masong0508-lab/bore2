// clock_ui: SET CLOCK, an overlay (CLOCK.OVL). OPTIONS > GAME > REAL TIME on a PlayStation, which has no clock chip: the player sets the
// time and the date, and the vblanks keep it from there while the console is on (sim.c, sim_rt_now). After a load, when the clock has
// not been set since the console came on, it asks IS IT STILL ...? with the saved time: X keeps it, or change it first.
// O (only when switching REAL TIME on) puts it off again. It sits in the crowd's arena like the other overlays, so it costs no RAM.
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "ovl.h"
#include <psxgpu.h>
#include <psxpad.h>
#include <stdio.h>
#include <string.h>

static int ask, f, v[5]; static char ttl[32];   // 1: the load's question; the field picked; DAY-1, MONTH (0 JAN), YEAR-2000, HOUR, MINUTE
static int rdays(int y, int m, int d) { int n = d - 1; for (int k = 2000; k < y; k++) n += rt_mdays(k, 1) == 29 ? 366 : 365; for (int k = 0; k < m; k++) n += rt_mdays(y, k); return n; }
static u32 vmin(void) { return (u32)rdays(2000 + v[2], v[1], v[0] + 1) * 1440 + (u32)(v[3] * 60 + v[4]); }
static void clk_open(int a) {
    u32 m = a && rt_last ? rt_last : sim_rt_now(); if (!m) m = rt_last ? rt_last : (u32)rdays(2026, 0, 1) * 1440 + 12 * 60;
    int y, mo, d; rt_date((int)(m / 1440), &y, &mo, &d);
    v[0] = d - 1; v[1] = mo; v[2] = y - 2000; v[3] = (int)(m % 1440) / 60; v[4] = (int)(m % 60); ask = a; f = 3;
    sprintf(ttl, "IS IT STILL %d:%02d %s?", v[3] % 12 ? v[3] % 12 : 12, v[4], v[3] < 12 ? "AM" : "PM");
}
static int clk_update(int held, int pr, int steps) {
    static const u8 rn[5] = { 31, 12, 100, 24, 60 };
    int d = !!(pr & PAD_UP) - !!(pr & PAD_DOWN), ml = rt_mdays(2000 + v[2], v[1]);
    f = (f + 5 + !!(pr & PAD_RIGHT) - !!(pr & PAD_LEFT)) % 5;
    if (d) { int n = f ? rn[f] : ml; v[f] = (v[f] + d + n) % n; audio_voice(VOICE_FINISHED); }
    ml = rt_mdays(2000 + v[2], v[1]); if (v[0] >= ml) v[0] = ml - 1;
    if (pr & PAD_CROSS) { rt_base = vmin(); rt_vb0 = VSync(-1); rt_hz = GetVideoMode() == MODE_PAL ? 50 : 60; audio_voice(VOICE_AGREE); return 0; }
    if ((pr & PAD_CIRCLE) && !ask) { sim_rt_set(0); return 0; }   // (put off: main switches the option off)
    return 1;
}
static void clk_draw(void) {
    gfx_set_clear(188, 204, 222); gfx_camera_update(); ui_backdrop();
    int top = ui_window(30, 46, 260, 148, ask ? ttl : "SET THE CLOCK", IC_SUN), h = v[3]; char b[40];
    ui_wrapc(42, top + 4, 236, 3, ask ? "THE CONSOLE WAS OFF. HOW LONG, IT CANNOT TELL: KEEP THIS TIME OR CHANGE IT"
                                      : "THE PLAYSTATION HAS NO CLOCK. SET YOURS AND IT KEEPS TIME WHILE THE CONSOLE IS ON", UI_INK);
    for (int k = 0; k < 5; k++) { int x = 54 + k * 48;
        if (k == 1) { memcpy(b, "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC" + v[1] * 3, 3); b[3] = 0; } else sprintf(b, k >= 3 ? "%02d" : "%d", v[k] + (k == 0) + (k == 2) * 2000);
        if (k == f) ui_round(x - 23, top + 42, 46, 24, 6, UI_RGB(70, 176, 238), UI_BLUE2, 0);
        ui_textc(x, top + 47, 2, b, k == f ? UI_WHITE : UI_INK); }
    ui_textc(222, top + 47, 2, ":", UI_INK);
    sprintf(b, "%s  %d:%02d %s", sim_day_name((int)(vmin() / 1440) + 5), h % 12 ? h % 12 : 12, v[4], h < 12 ? "AM" : "PM"); ui_textc(160, top + 74, 1, b, UI_BLUE2);
    ui_hints(160, top + 94, ask ? "DPAD CHANGE|X YES, THAT'S IT" : "DPAD CHANGE|X SET|O CANCEL", 1);
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, clk_open, clk_update, clk_draw };
