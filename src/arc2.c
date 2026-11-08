// arc2.c - ARCADE2.OVL: the games (ag_*.c) behind a cabinet. The rules page and attract screen, play, game over and initials are arc_core.c;
// when it is over the floor (ARCADE.OVL) is loaded again, standing at this cabinet (arcade_res.c: arun).
#include "arc_i.h"
#include "ovl.h"

static void a2_open(int g) { if (!arun.prev) arun.prev = (u8)(audio_jukebox_current() + 2); arun.cur = 0; core_open(g); }   // (a file read stops the music stream: the song starts again)
#ifndef TOUR_FIRST
#define TOUR_FIRST 1
#endif
#ifdef AUTO_TOUR   // (test builds: every cabinet in turn, 300 frames each, played by a simple bot, for the screenshots)
static int tour_n;
static void tour_bot(int *hd, int *pr) {
    static int prev, dir; int t = tour_n % 300, g = TOUR_FIRST + tour_n / 300; if (g >= AG_NGAME) g = AG_NGAME - 1;
    if (t == 2) { arc.tok = 99; arun.foe = 0; core_open(g); }
    if (t % 18 == 0) dir = ar8() & 15;
    int h = 0, lr = dir & 3, ud = dir & 12;
    if (lr & 1) h |= PAD_LEFT; if (lr & 2) h |= PAD_RIGHT;
    if (g == AG_SHMUP || g == AG_MAZE || g == AG_HOCKEY || g == AG_BLOCKS || g == AG_WHACK) { if (ud & 4) h |= PAD_UP; if (ud & 8) h |= PAD_DOWN; }
    if (g == AG_SHMUP || g == AG_RACER) h |= PAD_CROSS;
    if (g == AG_BLOCKS && t % 9 == 0) h |= PAD_CROSS;
    if (g == AG_SKEE) h = (h & (PAD_LEFT | PAD_RIGHT)) | ((t / 25) & 1 ? 0 : PAD_CROSS);
    if (t % 45 == 30 && (g == AG_HOOPS || g == AG_CLAW || g == AG_WHACK || g == AG_BREAKER || g == AG_STRIKER)) h |= PAD_CROSS;
    if (g == AG_RHYTHM) { int k = t % 5; h = k == 0 ? PAD_CROSS | PAD_LEFT : k == 2 ? PAD_UP | PAD_SQUARE : k == 3 ? PAD_CIRCLE : 0; }
    if (t == 50) h = PAD_CROSS; else if (t < 50) h = 0;
    *pr = h & ~prev; prev = h; *hd = h;
}
#endif
static int a2_update(int hd, int pr, int steps) {
#ifdef AUTO_TOUR
    tour_n++; tour_bot(&hd, &pr);
#endif
    if (core_update(hd, pr, steps)) return 1;
    arun.chain = 1; arun.g = (u8)(100 + core_game); return 0;
}
const AGFn ag_fn[AG_NGAME] = { { 0, 0, 0 }, { brk_start, brk_tick, brk_draw }, { cat_start, cat_tick, cat_draw }, { wak_start, wak_tick, wak_draw },
    { str_start, str_tick, str_draw }, { blk_start, blk_tick, blk_draw }, { shm_start, shm_tick, shm_draw }, { rac_start, rac_tick, rac_draw },
    { mze_start, mze_tick, mze_draw }, { hoo_start, hoo_tick, hoo_draw }, { ski_start, ski_tick, ski_draw }, { hoc_start, hoc_tick, hoc_draw },
    { rhy_start, rhy_tick, rhy_draw }, { claw_start, claw_tick, claw_draw } };
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, a2_open, a2_update, core_draw };
