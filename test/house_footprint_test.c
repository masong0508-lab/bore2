// A real planned house footprint is inclusive; the far exterior is half-open.
// The town renderer's body and roof must meet the same wall corners as the
// near renderer when crossing the streaming/window and view-distance edges.
#include "../src/house_footprint.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    enum { BM_BLK_TEST = 16, TILE_TEST = 256 };
    for (int bx = 0; bx < 32; ++bx) for (int origin = 0; origin < 32 * BM_BLK_TEST; origin += 16)
      for (int x0 = 0; x0 < BM_BLK_TEST; ++x0)
        for (int x1 = x0; x1 < BM_BLK_TEST; ++x1) {
            // Near b_x1 is the last occupied wall tile, so near far_box
            // is (b_x1 + 1) * TILE, not b_x1 * TILE.
            int min = (bx * BM_BLK_TEST + x0 - origin) * TILE_TEST;
            int max = house_foot_end_tile(bx, x1, origin, BM_BLK_TEST) * TILE_TEST;
            assert(max - min == (x1 - x0 + 1) * TILE_TEST);
            assert(max > min);
            if (x1 == BM_BLK_TEST - 1) {
                int next_min = ((bx + 1) * BM_BLK_TEST - origin) * TILE_TEST;
                assert(max == next_min); // no gap or overlap at block seams
            }
        }
    // Sample home, 9 wall tiles wide: the old far path drew only eight.
    assert(house_foot_end_tile(5, 10, 5 * BM_BLK_TEST, BM_BLK_TEST) == 11);
    assert(house_foot_end_tile(5, 3, 4 * BM_BLK_TEST, BM_BLK_TEST) == 20);
    puts("Generated near/far footprint: inclusive walls and block edges PASS");
    return 0;
}
