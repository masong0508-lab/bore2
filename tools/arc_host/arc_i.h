// arc_i.h (host test): the arcade games' API on a PC, so a game's logic and picture can be run and shot in a second (see arcsim.c)
#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
typedef uint8_t u8; typedef int8_t s8; typedef int16_t s16; typedef uint16_t u16; typedef uint32_t u32;
#define CLAMP(v,lo,hi) ((v)<(lo)?(lo):(v)>(hi)?(hi):(v))
#define ABS(v) ((v)<0?-(v):(v))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
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
#define FX(x) (10 + (x) * 5 / 4)
#define FY(y) (20 + (y) * 5 / 4)
void RC(int x, int y, int w, int h, u32 c); void RA(int x, int y, int w, int h, u32 c); void DSC(int cx, int cy, int r, u32 c);
void TX(int x, int y, const char *s, u32 c); void TXC(int cx, int y, const char *s, u32 c); void TXB(int cx, int y, const char *s, u32 c);
void NUM(int x, int y, const char *label, int v, u32 c);
int ar8(void);
enum { CUE_GOOD, CUE_BAD, CUE_GREAT, CUE_WIN, CUE_LOSE };
void cue(int k); void part(int x, int y, int vx, int vy, int life, u32 c); void burst(int x, int y, int n, u32 c); void pop(int x, int y, const char *s, u32 c);
extern int ag_pts, ag_t, ag_lives, ag_combo, ag_best, ag_perf, ag_np, ag_foe, ag_shake, ag_level;
#define R(x, y, w, h, r, g, b) RC(x, y, w, h, UI_RGB(r, g, b))
void hearts(int n, int max);
#define GAME(n) void n##_start(void); int n##_tick(int, int, int, int); void n##_draw(void);
GAME(brk) GAME(cat) GAME(wak) GAME(str) GAME(blk) GAME(shm) GAME(rac) GAME(mze) GAME(hoo) GAME(ski) GAME(hoc) GAME(rhy) GAME(claw)
