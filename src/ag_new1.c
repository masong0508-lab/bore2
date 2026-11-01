#include "arc_i.h"
#define S(n) void n##_start(void) {} int n##_tick(int a, int b, int c, int d) { return 0; } void n##_draw(void) {}
S(blk) S(shm) S(rac) S(mze)
