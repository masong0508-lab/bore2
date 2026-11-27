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

// Local block travel: output excludes start, includes destination; -1 no route.
#define BORE_ANDROID_ROUTE_MAX (33*33)
int bore_android_route(int x,int y,int X,int Y,uint32_t *out,int cap);
// Shift data: status, minutes, pay, energy cost, food cost, skill, gain.
#define BORE_ANDROID_SHIFT_FIELDS 7
enum { SHIFT_OK,SHIFT_CLOSED,SHIFT_TIRED,SHIFT_HUNGRY };
int bore_android_shift(int x,int y,int day,int minute,int energy,int food,int *out);

// Residential interior: lot, entry x/y, exact 18x18 footprint.
#define BORE_ANDROID_INTERIOR_FIELDS (3+18*18)
int bore_android_interior(int x,int y,int *out,int count);

#define BORE_ANDROID_WALK_FIELDS (54*54)
int bore_android_walk_mask(int cx,int cy,int *out,int count);
