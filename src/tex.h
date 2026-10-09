// the 8x8 wallpaper / floor pattern record BORE uses (shared by look.c, which holds the tables, and world.c, which maps them onto walls and floors)
#pragma once
#include "common.h"
typedef struct { const char *nm; u16 c[4]; const char *p[8]; } Tex;
#define NWP 14
#define NFL 14
const Tex *look_wp_tex(int i);     // 0..NWP-1: the pattern wallpapers
const Tex *look_fl_tex(int i);     // 0..NFL-1: the floors
u16 look_wp_avg(int i);            // 0..NWP-1
