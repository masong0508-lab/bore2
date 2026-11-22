// Regression for a moving camera oscillating across the former hard LOD edge.
#include "../src/house_lod.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    int state = 0;
    assert(!house_lod_upper(0, 9 * 9, 22 * 22)); // old threshold is in deadband
    for (int i = 0; i < 40; ++i)
        state = house_lod_upper(state, (i & 1) ? 9 * 9 : 10 * 10,
                               (i & 1) ? 22 * 22 : 23 * 23);
    assert(!state);
    state = house_lod_upper(state, 11 * 11, 22 * 22);
    assert(state);
    for (int i = 0; i < 40; ++i)
        state = house_lod_upper(state, (i & 1) ? 9 * 9 : 10 * 10,
                               (i & 1) ? 22 * 22 : 23 * 23);
    assert(state);
    assert(!house_lod_upper(state, 8 * 8, 20 * 20));
    assert(house_lod_upper(0, 8 * 8, 25 * 25));
    assert(house_lod_upper(1, 7 * 7, 21 * 21));
    assert(!house_lod_upper(1, 7 * 7, 20 * 20));
    puts("House upper-storey LOD: hysteresis and camera movement PASS");
    return 0;
}
