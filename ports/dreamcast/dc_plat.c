// dc_plat.c: the Dreamcast's stand-ins for the PS1 platform files that talk to hardware directly (pad.c, mcard.c, mem8.c,
// audio.c, ovl.c). Same functions and meanings, from KOS: the maple pad, plain RAM, the overlays linked in.
#include <kos.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <string.h>
#include "common.h"
#include "pad.h"
#include "mcard.h"
#include "mem8.h"
#include "ovl.h"
#include "audio.h"
#include "crowd.h"
#include "citizens.h"

KOS_INIT_FLAGS(INIT_DEFAULT);

// ---------------------------------------------------------------- the pad: a Dreamcast controller read as a PS1 digital pad
// PS1 bits (active low): 0 SELECT 3 START 4 UP 5 RIGHT 6 DOWN 7 LEFT 8 L2 9 R2 10 L1 11 R1 12 TRIANGLE 13 CIRCLE 14 CROSS 15 SQUARE.
// A = CROSS, B = CIRCLE, X = SQUARE, Y = TRIANGLE, the triggers = L1 / R1; pulled all the way with the other one = L2 / R2;
// SELECT = START with the triggers both held. The analog stick reads as the PS1's left stick.
static uint16_t dc_btn_port(int port, int *ax, int *ay) {
    maple_device_t *c = port == 0 ? maple_enum_type(0, MAPLE_FUNC_CONTROLLER) : maple_enum_dev(port, 0);
    *ax = *ay = 128;
    if (!c || !(c->info.functions & MAPLE_FUNC_CONTROLLER)) return 0xffff;
    cont_state_t *s = (cont_state_t *)maple_dev_status(c);
    if (!s) return 0xffff;
    uint16_t d = 0; uint32_t b = s->buttons;
    int lt = s->ltrig > 100, rt = s->rtrig > 100;
    if (b & CONT_START) d |= (lt && rt) ? 1 : 8;
    if (b & CONT_DPAD_UP) d |= 1 << 4; if (b & CONT_DPAD_RIGHT) d |= 1 << 5; if (b & CONT_DPAD_DOWN) d |= 1 << 6; if (b & CONT_DPAD_LEFT) d |= 1 << 7;
    if (lt && rt && !(b & CONT_START)) d |= (1 << 8) | (1 << 9); else { if (lt) d |= 1 << 10; if (rt) d |= 1 << 11; }
    if (b & CONT_Y) d |= 1 << 12; if (b & CONT_B) d |= 1 << 13; if (b & CONT_A) d |= 1 << 14; if (b & CONT_X) d |= 1 << 15;
    if (b & CONT_Z) d |= 1 << 8; if (b & CONT_C) d |= 1 << 9; if (b & CONT_D) d |= 1;
    *ax = s->joyx + 128; *ay = s->joyy + 128;
    return (uint16_t)~d;
}
static uint16_t dc_btn(int *ax, int *ay) { return dc_btn_port(0, ax, ay); }
// controllers B, C, D: held (active high, the game's own order) and newly pressed, for the fight's second player; 0 when nothing is plugged in
int dc_pad_n(int port, int *held, int *pressed) {
    static uint16_t prev[4] = { 0, 0, 0, 0 }; int ax, ay; maple_device_t *c = maple_enum_dev(port, 0);
    if (!c || !(c->info.functions & MAPLE_FUNC_CONTROLLER)) { prev[port] = 0; return 0; }
    uint16_t h = (uint16_t)~dc_btn_port(port, &ax, &ay); *held = h; *pressed = h & ~prev[port]; prev[port] = h; return 1;
}
int dc_pad2(int *held, int *pressed) { return dc_pad_n(1, held, pressed); }
static uint16_t latch = 0xffff; static int poll_on;
void pad_init(void) {}
void pad_poll_on(int on) { poll_on = on; }
int pad_read(int port, u8 *out) {
    if (port != 0) { memset(out, 0xff, 8); out[0] = 0xff; return 0; }
    int ax, ay; uint16_t b = dc_btn(&ax, &ay);
    out[0] = 0; out[1] = 0x41; out[2] = (u8)b; out[3] = (u8)(b >> 8);
    out[4] = out[5] = out[6] = out[7] = 128;
    return 1;
}
void pad_poll(void) { if (!poll_on) return; int ax, ay; latch &= dc_btn(&ax, &ay); }
unsigned short pad_latch_take(void) { uint16_t l = latch; latch = 0xffff; return l; }
void sio_select(int port) { (void)port; }
int sio_xfer(int byte, int want_ack, int wait) { (void)byte; (void)want_ack; (void)wait; return -1; }
void sio_deselect(void) {}

// ---------------------------------------------------------------- memory: the Dreamcast has 16 MB, so it is the "8 MB" PS1 build
int ram8 = 1;
static u8 *x_pool, *x_top, *x_end;
void mem8_init(void) { u32 n = 6u << 20; x_pool = (u8 *)memalign(32, n); x_top = x_pool; x_end = x_pool + n; }
void *xalloc(u32 n) { n = (n + 7) & ~7u; if (!x_top || x_top + n > x_end) return 0; void *p = x_top; x_top += n; memset(p, 0, n); return p; }
u32 xfree_bytes(void) { return x_top ? (u32)(x_end - x_top) : 0; }
void xrelease(void *p, u32 n) { (void)p; (void)n; }
void wmove(void *dst, const void *src, u32 n) { memmove(dst, src, n); }

// ---------------------------------------------------------------- the overlays: all linked in; "loading" one hands back its table
extern const OvlHead ovl_head_kingdom, ovl_head_arcade, ovl_head_gotchi, ovl_head_party, ovl_head_dj, ovl_head_super, ovl_head_dog,
    ovl_head_prison, ovl_head_space, ovl_head_paradise, ovl_head_farm, ovl_head_skysea, ovl_head_cut, ovl_head_arcade2, ovl_head_clock, ovl_head_pkinfo;
static const struct { const char *file, *name; const OvlHead *h; } ovls[] = {
    { "KINGDOM.OVL", "THE LAND BOREFORE TIME", &ovl_head_kingdom }, { "ARCADE.OVL", "BORTUA FIGHTER", &ovl_head_arcade },
    { "GOTCHI.OVL", "BOREAGOTCHI!", &ovl_head_gotchi }, { "PARTY.OVL", "FAMILY PARTY", &ovl_head_party }, { "DJ.OVL", "DJ BOOTH", &ovl_head_dj },
    { "SUPER.OVL", "SUPERSTAR", &ovl_head_super }, { "DOG.OVL", "CREATE A DOG", &ovl_head_dog }, { "PRISON.OVL", "CRIME AND LAW", &ovl_head_prison },
    { "SPACE.OVL", "SPACE AGE", &ovl_head_space }, { "PARADISE.OVL", "ISLAND PARADISE", &ovl_head_paradise }, { "FARM.OVL", "FARM LIFE", &ovl_head_farm },
    { "SKYSEA.OVL", "SKY AND SEA", &ovl_head_skysea }, { "CUT.OVL", "CUTSCENE", &ovl_head_cut },
    { "ARCADE2.OVL", "BORTUA FIGHTER", &ovl_head_arcade2 }, { "CLOCK.OVL", "SET CLOCK", &ovl_head_clock }, { "PKINFO.OVL", "PACKS", &ovl_head_pkinfo },
};
// ARCADE.OVL holds the fighter's AGFn, ARCADE2.OVL the other games'; arc_core.c is one copy here and reads the merged table
#include "arc_i.h"
#undef ag_fn
extern const AGFn ag_fn_fight[AG_NGAME], ag_fn_games[AG_NGAME];
AGFn ag_fn_merged[AG_NGAME];
static void ag_merge(void) { for (int i = 0; i < AG_NGAME; i++) ag_fn_merged[i] = i ? ag_fn_games[i] : ag_fn_fight[0]; }
const char *ovl_pack_name(const char *file) { for (unsigned i = 0; i < sizeof ovls / sizeof ovls[0]; i++) if (!strcmp(ovls[i].file, file)) return ovls[i].name; return file; }
int ovl_installed(const char *file) { for (unsigned i = 0; i < sizeof ovls / sizeof ovls[0]; i++) if (!strcmp(ovls[i].file, file)) return 1; return !strcmp(file, "GOTCHI.ART"); }
const OvlHead *ovl_load(const char *file) {
    for (unsigned i = 0; i < sizeof ovls / sizeof ovls[0]; i++) if (!strcmp(ovls[i].file, file)) {
        u32 n; u8 *base = (u8 *)crowd_scratch(&n) + OVL_AT; crowd_reset(); memset(base, 0, (int)n - OVL_AT);   // (the arena is the overlay's, as on the PS1)
        if (ovls[i].h == &ovl_head_arcade || ovls[i].h == &ovl_head_arcade2) ag_merge();
        return ovls[i].h;
    }
    return 0;
}
void ovl_leave(void) { crowd_reset(); citizens_rebuild(); }


// ---------------------------------------------------------------- REAL TIME (OPTIONS > GAME, sim.c): the Dreamcast's own clock
// The clock chip holds local time (set in the Dreamcast's own menu), read as minutes since 1 JAN 2000; 0 when it was never set.
// Read twice a second at most (a read goes over the G2 bus).
#include <kos/rtc.h>
u32 dc_rtc_min(void) {
    static u32 m; static int vb = -1000; extern int VSync(int); int now = VSync(-1);
    if (now - vb >= 30 || now < vb) { vb = now; time_t t = rtc_unix_secs(); m = t > 946684800 ? (u32)((t - 946684800) / 60) : 0; }
    return m;
}

GPU_VideoMode GetVideoMode(void) { return MODE_NTSC; }
