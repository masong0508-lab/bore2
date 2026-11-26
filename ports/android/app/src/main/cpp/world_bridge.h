// Native BORE 2 map bootstrap; not a replacement for the life simulator.
#pragma once
#include <stdint.h>
uint32_t bore_android_init(uint32_t seed);
int bore_android_window(int cx,int cy,int radius,uint32_t *out,int count);
int bore_android_describe(int x,int y,int weekday,char *out,unsigned cap);
