/* Host regression for scrolling the DS's 32 x 20 neighborhood map.
 * cc -std=c99 -Wall -Wextra -Werror ports/dsi/test/ds_mapview_test.c -o /tmp/ds_mapview_test && /tmp/ds_mapview_test */
#include <assert.h>
#include <stdio.h>
#include "../source/ds_mapview.h"
#define W 32
#define H 20
#define N 1024
static void visible(int x, int y, int l, int t) {
    assert(l >= 0 && t >= 0 && l <= N - W && t <= N - H);
    assert(x >= l && x < l + W && y >= t && y < t + H);
}
int main(void) {
    int l = -1, t = -1;
    ds_mapview_follow(&l, &t, 500, 450, W, H, N, 1);
    visible(500, 450, l, t);
    int l0 = l, t0 = t;
    for (int x = 497; x <= 502; ++x) {
        ds_mapview_follow(&l, &t, x, 450, W, H, N, 0);
        assert(l == l0 && t == t0);
    }
    for (int x = 502; x < 900; ++x) {
        ds_mapview_follow(&l, &t, x, 450, W, H, N, 0);
        visible(x, 450, l, t);
    }
    for (int y = 450; y > 20; --y) {
        ds_mapview_follow(&l, &t, 899, y, W, H, N, 0);
        visible(899, y, l, t);
    }
    const int corners[4][2] = {{0,0},{1023,0},{0,1023},{1023,1023}};
    for (int i = 0; i < 4; ++i) {
        ds_mapview_follow(&l, &t, corners[i][0], corners[i][1], W, H, N, 1);
        visible(corners[i][0], corners[i][1], l, t);
    }
    /* A distant town resets the camera; no stale town-centre origin. */
    ds_mapview_follow(&l, &t, 27, 721, W, H, N, 1);
    visible(27, 721, l, t);
    assert(l < 27 && t > 700);
    puts("ds_mapview_test: PASS");
    return 0;
}
