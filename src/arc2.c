// arc2.c - ARCADE2.OVL: the games (ag_*.c) behind a cabinet. The rules page and attract screen, play, game over and initials are arc_core.c;
// when it is over the floor (ARCADE.OVL) is loaded again, standing at this cabinet (arcade_res.c: arun).
#include "arc_i.h"
#include "ovl.h"

static void a2_open(int g) { if (!arun.prev) { arun.prev = (u8)(audio_jukebox_current() + 2); arun.cur = 0; } core_open(g); }
static int a2_update(int hd, int pr, int steps) {
    if (core_update(hd, pr, steps)) return 1;
    arun.chain = 1; arun.g = (u8)(100 + core_game); return 0;
}
const AGFn ag_fn[AG_NGAME] = { { 0, 0, 0 }, { brk_start, brk_tick, brk_draw }, { cat_start, cat_tick, cat_draw }, { wak_start, wak_tick, wak_draw },
    { str_start, str_tick, str_draw }, { blk_start, blk_tick, blk_draw }, { shm_start, shm_tick, shm_draw }, { rac_start, rac_tick, rac_draw },
    { mze_start, mze_tick, mze_draw }, { hoo_start, hoo_tick, hoo_draw }, { ski_start, ski_tick, ski_draw }, { hoc_start, hoc_tick, hoc_draw },
    { rhy_start, rhy_tick, rhy_draw }, { claw_start, claw_tick, claw_draw } };
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, a2_open, a2_update, core_draw };
