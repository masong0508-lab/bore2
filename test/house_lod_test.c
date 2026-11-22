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
    // Irregular footprints cannot use a full rectangular box: its walls
    // would pass through an L-house courtyard or a stair/patio opening.
    assert(house_lod_box_safe(12, 0, 0, 3, 2));
    assert(house_lod_box_safe(1, 3, 5, 3, 5));
    assert(!house_lod_box_safe(11, 0, 0, 3, 2)); // missing courtyard tile
    assert(!house_lod_box_safe(8, 1, 1, 3, 3));  // 3x3 L-shaped
    assert(!house_lod_box_safe(0, 1, 1, 3, 3));
    assert(!house_lod_box_safe(3, 6, 2, 5, 3));
    puts("House upper-storey LOD: hysteresis, movement, and courtyard-safe geometry PASS");
    return 0;
}
