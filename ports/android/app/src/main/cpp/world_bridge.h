// Native BORE 2 map bootstrap; not a replacement for the life simulator.
#pragma once
#include <stdint.h>
uint32_t bore_android_init(uint32_t seed);
int bore_android_window(int cx,int cy,int radius,uint32_t *out,int count);
int bore_android_describe(int x,int y,int weekday,char *out,unsigned cap);

// Stateless preview session: validates against the actual selected world lot.
// Result fields: status,id,fee,energy,fun,social,food,rest,skill,gain,minutes.
#define BORE_ANDROID_ACTIVITY_FIELDS 11
int bore_android_can_visit(int x,int y);
int bore_android_activity(int x,int y,int day,int slot,int minute,int funds,int rest,int *out);
const char *bore_android_activity_text(int id,int field);
