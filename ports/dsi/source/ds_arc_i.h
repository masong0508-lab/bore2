// ds_arc_i.h - the PS1's arcade games (src/ag_old.c, ag_new1.c, ag_new2.c) on the DS: the same API as src/arc_i.h, drawn on the DS top screen
// through four calls that ds_games.c provides (dsag_rect, dsag_disc, dsag_text, dsag_cue). The field is the PS1's 240 x 160, 1:1, at (8, 16).
#pragma once
#include <string.h>
#include <stdio.h>
typedef unsigned char u8; typedef signed char s8; typedef short s16; typedef unsigned short u16; typedef unsigned int u32;
#ifndef CLAMP
#define CLAMP(v,lo,hi) ((v)<(lo)?(lo):(v)>(hi)?(hi):(v))
#endif
#ifndef ABS
#define ABS(v) ((v)<0?-(v):(v))
#endif
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif
#define UI_RGB(r, g, b) ((u32)(r) | ((u32)(g) << 8) | ((u32)(b) << 16))
#define UI_INK UI_RGB(22, 28, 40)
#define UI_GREEN UI_RGB(84, 196, 72)
#define UI_WHITE UI_RGB(255, 255, 255)
#define UI_RED UI_RGB(228, 84, 70)
#define UI_AMBER UI_RGB(236, 176, 40)
#define PAD_SELECT 1
#define PAD_START 8
#define PAD_UP 16
#define PAD_RIGHT 32
#define PAD_DOWN 64
#define PAD_LEFT 128
#define PAD_TRIANGLE 4096
#define PAD_CIRCLE 8192
#define PAD_CROSS 16384
#define PAD_SQUARE 32768
// the DS side (ds_games.c)
void dsag_rect(int x, int y, int w, int h, u32 c, int semi);
void dsag_disc(int cx, int cy, int r, u32 c);
void dsag_text(int x, int y, int sz, const char *s, u32 c);
void dsag_cue(int k);
extern int dsag_tx;   // where the stylus is on the pad (0..255), or -1
#define RC(x, y, w, h, c) dsag_rect(x, y, w, h, c, 0)
#define RA(x, y, w, h, c) dsag_rect(x, y, w, h, c, 1)
#define DSC(cx, cy, r, c) dsag_disc(cx, cy, r, c)
#define TX(x, y, s, c) dsag_text(x, y, 1, s, c)
#define TXC(cx, y, s, c) dsag_text((cx) - (int)strlen(s) * 3, y, 1, s, c)
#define TXB(cx, y, s, c) dsag_text((cx) - (int)strlen(s) * 6, y, 2, s, c)
#define ar8 dsag_ar8
#define cue dsag_cue
#define part dsag_part
#define burst dsag_burst
#define pop dsag_pop
#define hearts dsag_hearts
#define NUM dsag_num
int ar8(void);
void dsag_num(int x, int y, const char *label, int v, u32 c);
enum { CUE_GOOD, CUE_BAD, CUE_GREAT, CUE_WIN, CUE_LOSE };
void part(int x, int y, int vx, int vy, int life, u32 c); void burst(int x, int y, int n, u32 c); void pop(int x, int y, const char *s, u32 c);
void hearts(int n, int max);
extern int ag_pts, ag_t, ag_lives, ag_combo, ag_best, ag_perf, ag_np, ag_foe, ag_shake, ag_level;
#define R(x, y, w, h, r, g, b) RC(x, y, w, h, UI_RGB(r, g, b))
#define GAME(n) void n##_start(void); int n##_tick(int, int, int, int); void n##_draw(void);
GAME(brk) GAME(cat) GAME(wak) GAME(str) GAME(blk) GAME(shm) GAME(rac) GAME(mze) GAME(hoo) GAME(ski) GAME(hoc) GAME(rhy) GAME(claw)
enum { AG_FIGHTER, AG_BREAKER, AG_CATCH, AG_WHACK, AG_STRIKER, AG_BLOCKS, AG_SHMUP, AG_RACER, AG_MAZE, AG_HOOPS, AG_SKEE, AG_HOCKEY, AG_RHYTHM, AG_CLAW, AG_NGAME, AG_PRIZES = 99 };
