// ds_agames.c - the PS1's arcade games on the DS. The thirteen cabinets' code (src/ag_old.c, ag_new1.c, ag_new2.c) is the PS1's own, built against
// ds_arc_i.h; the DS draws through dsag_* (ds_games.c). This file holds what the games share on the PS1 in arc_core.c: the random numbers, the sparks
// and the floating scores, and the table of games. BORTUA FIGHTER is not here (its DS port is ds_fight.c).
#define BORE_DS 1
#include "ds_arc_i.h"

int ag_pts, ag_t, ag_lives, ag_combo, ag_best, ag_perf, ag_np, ag_foe, ag_shake, ag_level;
static u32 arng = 0x9E3779B9u;
int ar8(void) { arng = arng * 1103515245u + 12345u; return (int)((arng >> 16) & 255); }
void dsag_num(int x, int y, const char *l, int v, u32 c) { char s[24]; snprintf(s, sizeof s, "%s %d", l, v); TX(x, y, s, c); }
static struct { s16 x, y, vx, vy; u8 life; u32 c; } pt[40];
void part(int x, int y, int vx, int vy, int life, u32 c) {
    for (int i = 0; i < 40; i++) if (!pt[i].life) { pt[i].x = (s16)(x * 16); pt[i].y = (s16)(y * 16); pt[i].vx = (s16)vx; pt[i].vy = (s16)vy; pt[i].life = (u8)life; pt[i].c = c; return; }
}
void burst(int x, int y, int n, u32 c) { for (int i = 0; i < n; i++) part(x, y, (ar8() - 128) / 5, (ar8() - 160) / 5, 10 + ar8() % 10, c); }
static struct { s16 x, y; u8 t; u32 c; char s[10]; } pp[6];
void pop(int x, int y, const char *s, u32 c) {
    int k = 0; for (int i = 0; i < 6; i++) if (!pp[i].t) { k = i; break; }
    pp[k].x = (s16)x; pp[k].y = (s16)y; pp[k].t = 26; pp[k].c = c; strncpy(pp[k].s, s, 9); pp[k].s[9] = 0;
}
void dsag_fx_reset(void) { memset(pt, 0, sizeof pt); memset(pp, 0, sizeof pp); }
void dsag_fx_tick(void) {
    for (int i = 0; i < 40; i++) if (pt[i].life) { pt[i].life--; pt[i].x += pt[i].vx; pt[i].y += pt[i].vy; pt[i].vy += 2; }
    for (int i = 0; i < 6; i++) if (pp[i].t) { pp[i].t--; if (pp[i].t & 1) pp[i].y--; }
    if (ag_shake > 0) ag_shake--;
}
void dsag_fx_draw(void) {
    for (int i = 0; i < 40; i++) if (pt[i].life) RC(pt[i].x >> 4, pt[i].y >> 4, 2, 2, pt[i].c);
    for (int i = 0; i < 6; i++) if (pp[i].t) TXC(pp[i].x, pp[i].y, pp[i].s, pp[i].t > 6 ? pp[i].c : UI_RGB(120, 120, 140));
}

#include "../../../src/ag_old.c"
#include "../../../src/ag_new1.c"
#include "../../../src/ag_new2.c"

typedef struct { void (*start)(void); int (*tick)(int, int, int, int); void (*draw)(void); } DSGame;
const DSGame ds_ag_fn[AG_NGAME] = { { 0, 0, 0 }, { brk_start, brk_tick, brk_draw }, { cat_start, cat_tick, cat_draw }, { wak_start, wak_tick, wak_draw },
    { str_start, str_tick, str_draw }, { blk_start, blk_tick, blk_draw }, { shm_start, shm_tick, shm_draw }, { rac_start, rac_tick, rac_draw },
    { mze_start, mze_tick, mze_draw }, { hoo_start, hoo_tick, hoo_draw }, { ski_start, ski_tick, ski_draw }, { hoc_start, hoc_tick, hoc_draw },
    { rhy_start, rhy_tick, rhy_draw }, { claw_start, claw_tick, claw_draw } };
