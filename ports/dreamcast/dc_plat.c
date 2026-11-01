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
static uint16_t dc_btn(int *ax, int *ay) {
    maple_device_t *c = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    *ax = *ay = 128;
    if (!c) return 0xffff;
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

// ---------------------------------------------------------------- the memory card: none yet (the VMU comes next)
int mcard_last_err = MC_NOCARD, mcard_dir_keep;
int mcard_read(int sector, u8 *out) { (void)sector; (void)out; return 0; }
int mcard_write(int sector, const u8 *in) { (void)sector; (void)in; return 0; }
int mcard_present(void) { mcard_last_err = MC_NOCARD; return 0; }
int mcard_rbegin(const char *name) { (void)name; mcard_last_err = MC_NOCARD; return 0; }
void mcard_rget(void *out, int n) { memset(out, 0, n); }
int mcard_rok(void) { return 0; }
int mcard_wbegin(const char *name, int blocks) { (void)name; (void)blocks; mcard_last_err = MC_NOCARD; return 0; }
void mcard_wput(const void *in, int n) { (void)in; (void)n; }
int mcard_wpatch(int sector, const u8 *sec) { (void)sector; (void)sec; return 0; }
int mcard_delete(const char *name) { (void)name; return 0; }
int mcard_wend(void) { return 0; }
void mcard_title_sector(u8 *sec, const char *title, int blocks, const u16 *pal) { (void)sec; (void)title; (void)blocks; (void)pal; }

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
    ovl_head_prison, ovl_head_space, ovl_head_paradise, ovl_head_farm, ovl_head_skysea, ovl_head_cut;
static const struct { const char *file, *name; const OvlHead *h; } ovls[] = {
    { "KINGDOM.OVL", "THE LAND BOREFORE TIME", &ovl_head_kingdom }, { "ARCADE.OVL", "BORTUA FIGHTER", &ovl_head_arcade },
    { "GOTCHI.OVL", "BOREAGOTCHI!", &ovl_head_gotchi }, { "PARTY.OVL", "FAMILY PARTY", &ovl_head_party }, { "DJ.OVL", "DJ BOOTH", &ovl_head_dj },
    { "SUPER.OVL", "SUPERSTAR", &ovl_head_super }, { "DOG.OVL", "CREATE A DOG", &ovl_head_dog }, { "PRISON.OVL", "CRIME AND LAW", &ovl_head_prison },
    { "SPACE.OVL", "SPACE AGE", &ovl_head_space }, { "PARADISE.OVL", "ISLAND PARADISE", &ovl_head_paradise }, { "FARM.OVL", "FARM LIFE", &ovl_head_farm },
    { "SKYSEA.OVL", "SKY AND SEA", &ovl_head_skysea }, { "CUT.OVL", "CUTSCENE", &ovl_head_cut },
};
const char *ovl_pack_name(const char *file) { for (unsigned i = 0; i < sizeof ovls / sizeof ovls[0]; i++) if (!strcmp(ovls[i].file, file)) return ovls[i].name; return file; }
int ovl_installed(const char *file) { for (unsigned i = 0; i < sizeof ovls / sizeof ovls[0]; i++) if (!strcmp(ovls[i].file, file)) return 1; return !strcmp(file, "GOTCHI.ART"); }
const OvlHead *ovl_load(const char *file) {
    for (unsigned i = 0; i < sizeof ovls / sizeof ovls[0]; i++) if (!strcmp(ovls[i].file, file)) {
        u32 n; u8 *base = (u8 *)crowd_scratch(&n) + OVL_AT; crowd_reset(); memset(base, 0, (int)n - OVL_AT);   // (the arena is the overlay's, as on the PS1)
        return ovls[i].h;
    }
    return 0;
}
void ovl_leave(void) { crowd_reset(); citizens_rebuild(); }

// ---------------------------------------------------------------- sound: silent for now (the AICA comes later); the jukebox's list is the real one
#include "songs.inc"
volatile int band_n[6], band_miss[6];
static int juke_cur = -1, juke_shuf;
void audio_init(void) {}
int audio_cd_part(const char *name, u32 sector, int count, void *dst) { (void)name; (void)sector; (void)count; (void)dst; return 0; }
int audio_cd_load(const char *name, void *dst, int room) { (void)name; (void)dst; (void)room; return 0; }
int audio_cd_peek(const char *name, void *dst) { (void)name; (void)dst; return 0; }
void audio_voice(int v) { (void)v; }
void audio_volumes(int a, int b, int c) { (void)a; (void)b; (void)c; }
void audio_music_level(int l) { (void)l; }
void audio_update(void) {}
int audio_song_count(void) { return NSONGS; }
int audio_song_find(const char *name) { for (int i = 0; i < NSONGS; i++) if (!strcmp(songs[i].name, name)) return i; return -1; }
void audio_band_set(const unsigned char *r, const int *q) { (void)r; (void)q; }
void audio_band_quality(int role, int q) { (void)role; (void)q; }
const char *audio_song_name(int i) { return i >= 0 && i < NSONGS ? songs[i].name : ""; }
const char *audio_song_artist(int i) { return i >= 0 && i < NSONGS ? songs[i].artist : ""; }
int audio_song_len(int i) { return i >= 0 && i < NSONGS ? songs[i].seconds : 0; }
void audio_jukebox_play(int i) { juke_cur = i; }
void audio_jukebox_stop(void) { juke_cur = -1; }
void audio_jukebox_next(void) { if (juke_cur >= 0) juke_cur = (juke_cur + 1) % NSONGS; }
int audio_jukebox_current(void) { return juke_cur; }
int audio_jukebox_elapsed(void) { return 0; }
void audio_jukebox_toggle_shuffle(void) { juke_shuf ^= 1; }
int audio_jukebox_shuffle(void) { return juke_shuf; }
int audio_cd_ok(void) { return 1; }
int audio_station_count(void) { return 0; }
const char *audio_station_name(int s) { (void)s; return ""; }
int audio_station(void) { return -1; }
void audio_station_play(int s) { (void)s; }
int dj_count(void) { return 0; }
const char *dj_track_name(int t) { (void)t; return ""; }
const char *dj_track_real(int t) { (void)t; return ""; }
int dj_active(void) { return 0; }
void dj_begin(void) {}
int dj_state(int d) { (void)d; return 0; }
int dj_track(int d) { (void)d; return -1; }
int dj_cue(int d, int t, int bars) { (void)d; (void)t; (void)bars; return 0; }
int dj_bars(int t) { (void)t; return 0; }
void dj_fader(int d, int g) { (void)d; (void)g; }
int dj_gain(int d) { (void)d; return 0; }
void dj_set_tempo(int d, int t) { (void)d; (void)t; }
int dj_tempo(int d) { (void)d; return 0; }
int dj_order_ms(int d) { (void)d; return 0; }
int dj_order_ms0(int d) { (void)d; return 0; }
void dj_sync(int d, int err) { (void)d; (void)err; }
void dj_launch(int d, int cue_row) { (void)d; (void)cue_row; }
int dj_pending(int d) { (void)d; return 0; }
void dj_stop(int d) { (void)d; }
void dj_mute(int d, int mask) { (void)d; (void)mask; }
int dj_mutemask(int d) { (void)d; return 0; }
void dj_pitch(int d, int pm) { (void)d; (void)pm; }
void dj_echo(int on) { (void)on; }
int dj_pos(int d, int what) { (void)d; (void)what; return 0; }
