// dc_extra.c: things only the Dreamcast has. Called once a frame from the platform layer (dc_gpu.c, after the frame is handed to
// the PowerVR), so the PS1 game code knows nothing of them.
// - The VMU screen (48x32): the Bore you play - their face, which follows their mood - their name, the clock, and four needs
//   as bars; a little animation while a save is being written.
#include <kos.h>
#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <string.h>
#include "common.h"
#include "sim.h"

// ---------------------------------------------------------------- a 3x5 font for the VMU screen
static const uint16_t f35[40] = {   // 0-9, A-Z, ':', ' ', '-', '.' ; 15 bits, rows top to bottom, 3 bits each (left = 4)
    075557, 026227, 071747, 071717, 055711, 074717, 074757, 071111, 075757, 075717,
    025755, 065656, 034443, 065556, 074647, 074644, 034553, 055755, 072227, 011157, 055655, 044447, 057555, 065555, 025552,
    065644, 025573, 065655, 034317, 072222, 055557, 055522, 055575, 055255, 055222, 071247, 002020, 0, 000700, 000002 };
static int glyph(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return 10 + c - 'A';
    if (c >= 'a' && c <= 'z') return 10 + c - 'a';
    return c == ':' ? 36 : c == '-' ? 38 : c == '.' ? 39 : 37;
}
static uint8_t lcd[32][48];
static void px(int x, int y) { if (x >= 0 && x < 48 && y >= 0 && y < 32) lcd[y][x] = 1; }
static void text(int x, int y, const char *s) {
    for (; *s; s++, x += 4) { uint16_t g = f35[glyph(*s)]; for (int r = 0; r < 5; r++) for (int c = 0; c < 3; c++) if (g & (1 << (14 - r * 3 - c))) px(x + c, y + r); }
}
static void lcd_send(void) {
    maple_device_t *d = maple_enum_type(0, MAPLE_FUNC_LCD);
    if (!d) return;
    static uint8_t bits[192], last[192];
    memset(bits, 0, sizeof bits);
    for (int y = 0; y < 32; y++) for (int x = 0; x < 48; x++) if (lcd[y][x]) bits[y * 6 + x / 8] |= (uint8_t)(0x80 >> (x & 7));
    if (!memcmp(bits, last, sizeof bits)) return;
    memcpy(last, bits, sizeof bits);
    vmu_draw_lcd_rotated(d, bits);
}
static void face(int ox, int oy, int mood, int blink) {   // a 16x16 Bore head: a box with eyes and a mouth that follows the mood
    for (int i = 0; i < 16; i++) { px(ox + i, oy); px(ox + i, oy + 15); px(ox, oy + i); px(ox + 15, oy + i); }
    if (blink) { for (int i = 0; i < 3; i++) { px(ox + 3 + i, oy + 6); px(ox + 10 + i, oy + 6); } }
    else for (int j = 0; j < 3; j++) for (int i = 0; i < 2; i++) { px(ox + 4 + i, oy + 4 + j); px(ox + 10 + i, oy + 4 + j); }
    int m = mood > 15 ? 2 : mood < -15 ? 0 : 1;
    for (int i = 0; i < 8; i++) {
        int y = oy + 11;
        if (m == 2) y += (i == 0 || i == 7) ? -1 : (i == 1 || i == 6) ? 0 : 1;
        if (m == 0) y += (i == 0 || i == 7) ? 1 : (i == 1 || i == 6) ? 0 : -1;
        px(ox + 4 + i, y);
    }
}
static int saving, tick;
void dc_vmu_saving(int on) {
    saving = on;
    if (!on) return;
    memset(lcd, 0, sizeof lcd);
    face(2, 8, 100, 0);
    text(21, 8, "SAVE"); text(21, 16, "...");
    lcd_send();
}
static void vmu_status(void) {
    memset(lcd, 0, sizeof lcd);
    if (nsims <= 0 || player < 0 || player >= nsims) {   // the title: the logo
        text(8, 6, "BORE 2"); face(16, 14, 100, (tick / 30) % 5 == 0);
        lcd_send(); return;
    }
    const Sim *s = &sims[player];
    face(0, 0, s->mood, (tick / 30) % 6 == 0);
    char nm[8]; int k = 0; for (const char *p = s->bore.first; *p && k < 7; p++) nm[k++] = *p; nm[k] = 0;
    text(18, 0, nm);
    char clk[8]; int h = clock_min / 60, m = clock_min % 60; clk[0] = (char)('0' + h / 10); clk[1] = (char)('0' + h % 10); clk[2] = ':'; clk[3] = (char)('0' + m / 10); clk[4] = (char)('0' + m % 10); clk[5] = 0;
    text(18, 7, clk);
    static const int nd[4] = { N_FOOD, N_REST, N_FUN, N_SOCIAL }; static const char lab[4] = { 'F', 'R', 'J', 'S' };
    for (int i = 0; i < 4; i++) {   // the needs: a letter, then a bar (outlined; filled as far as the need is met)
        int y = 19 + (i & 1) * 7, x = (i >> 1) * 25;
        char l[2] = { lab[i], 0 }; text(x, y, l);
        int v = s->need[nd[i]] / 1024; if (v < 0) v = 0; if (v > 100) v = 100;
        int w = (v * 15 + 50) / 100;
        for (int j = 0; j < 17; j++) { px(x + 5 + j, y); px(x + 5 + j, y + 4); }
        for (int r = 1; r < 4; r++) { px(x + 5, y + r); px(x + 21, y + r); }
        for (int j = 0; j < w; j++) for (int r = 2; r < 3 + 0; r++) { px(x + 6 + j, y + 1); px(x + 6 + j, y + 2); px(x + 6 + j, y + 3); }
    }
    lcd_send();
}
void dc_extra_frame(void) {
    tick++;
    if (tick == 60) {   // (the log: what is plugged in)
        extern void dc_logf(const char *f, ...);
        for (int p = 0; p < 4; p++) for (int u = 0; u < 6; u++) { maple_device_t *d = maple_enum_dev(p, u); if (d && d->valid) dc_logf("maple %c%d: %08lx %s\n", 'A' + p, u, (unsigned long)d->info.functions, d->info.product_name); }
    }
#ifdef AUTO_SAVE
    { extern int dbg_sv[16], dbg_st[8], dbg_opt[2], dbg_cells[6]; extern void dc_logf(const char *f, ...);   // (test builds: the save test's readouts)
      if (tick % 200 == 0) { dc_logf("save:"); for (int i = 0; i < 16; i++) dc_logf(" %d", dbg_sv[i]); dc_logf(" | opt %x | stream", dbg_opt[0]); for (int i = 0; i < 8; i++) dc_logf(" %d", dbg_st[i]); dc_logf(" | song %d\n", dbg_cells[2]); } }
#endif
    extern int dc_opt_vmuscreen;
    if (saving || (tick % 30) || !dc_opt_vmuscreen) return;   // (twice a second is plenty for the VMU)
    vmu_status();
}

// ---------------------------------------------------------------- OPTIONS > DREAMCAST: the Dreamcast's own switches (a tab of the
// PS1 options screen, drawn with the game's own interface parts; kept apart from the PS1 options, so saves are the same on both)
#include "ui.h"
#include <psxpad.h>
extern int dc_vivid, dc_filter;
int dc_opt_rumble = 1, dc_opt_vmuscreen = 1;
extern void dc_grade_changed(void);
enum { DO_COLOUR, DO_TEX, DO_RUMBLE, DO_VMU, DO_N };
static const char *const dnm[DO_N] = { "COLOURS", "TEXTURES", "JUMP PACK", "VMU SCREEN" };
static const char *const dval[DO_N][2] = { { "CLASSIC PS1", "VIVID" }, { "SHARP", "SMOOTH" }, { "OFF", "ON" }, { "OFF", "ON" } };
static const char *const dblurb[DO_N] = { "VIVID: RICHER, BRIGHTER COLOURS. CLASSIC: THE PLAYSTATION'S OWN",
    "SMOOTH: TEXTURES FILTERED (BILINEAR). SHARP: CRISP PIXELS AS ON THE PS1", "THE JUMP PACK SHAKES FOR THE DOORBELL, THE PHONE, FIGHTS AND LANDINGS",
    "YOUR BORE'S FACE, MOOD, CLOCK AND NEEDS ON THE VMU" };
static int dsel;
static int *dvar(int i) { return i == DO_COLOUR ? &dc_vivid : i == DO_TEX ? &dc_filter : i == DO_RUMBLE ? &dc_opt_rumble : &dc_opt_vmuscreen; }
void dc_opts_update(unsigned short btn, unsigned short prev) {
#define DP(b) (!(btn & (b)) && (prev & (b)))
    if (DP(PAD_UP)) dsel = (dsel + DO_N - 1) % DO_N;
    if (DP(PAD_DOWN)) dsel = (dsel + 1) % DO_N;
    if (DP(PAD_LEFT) || DP(PAD_RIGHT) || DP(PAD_CROSS)) {
        int *v = dvar(dsel); *v = !*v;
        if (dsel == DO_COLOUR) dc_grade_changed();
    }
#undef DP
}
void dc_opts_draw(int top) {
    for (int r = 0; r < DO_N; r++) {
        int y = top + r * 26, sel = r == dsel; const char *v = dval[r][*dvar(r) & 1];
        if (sel) { ui_round(104, y, 202, 24, 6, UI_RGB(226, 242, 254), UI_HILITE, 0); }
        ui_text(110, y + 3, 1, dnm[r], sel ? UI_BLUE2 : UI_INK);
        ui_text(300 - ui_tw(1, v), y + 3, 1, v, UI_BLUE2);
        if (sel) ui_text(110, y + 13, 1, "LEFT RIGHT TO CHANGE", UI_SOFT);
    }
    ui_round(104, top + 166, 202, 32, 6, UI_RGB(234, 240, 248), UI_RGB(224, 231, 240), 0);
    ui_icon(110, top + 176, IC_HELP, UI_BLUE);
    ui_wrap(126, top + 171, 176, 2, dblurb[dsel], UI_INK);
}

// ---------------------------------------------------------------- the Jump Pack: a shake for the game's big moments (src/ calls this
// under BORE_DC only: a skate landing, a hit in the fight game, a knock at the door, the police at the door). power 1..7, a few frames.
#include <dc/maple/purupuru.h>
void dc_rumble(int power, int frames) {
    if (!dc_opt_rumble) return;
    maple_device_t *d = maple_enum_type(0, MAPLE_FUNC_PURUPURU);
    if (!d) return;
    purupuru_effect_t e; e.raw = 0;
    e.motor = 1; e.fpow = (uint32_t)(power < 1 ? 1 : power > 7 ? 7 : power); e.conv = 1; e.freq = 26; e.inc = (uint8_t)(frames < 1 ? 1 : frames > 255 ? 255 : frames);
    purupuru_rumble(d, &e);
}
