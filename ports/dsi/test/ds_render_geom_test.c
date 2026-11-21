/* Host guardrail for DS floor overlap, storeys and adjoining roof pieces.
 * cc -std=c99 -Wall -Wextra -Werror ports/dsi/test/ds_render_geom_test.c -o /tmp/ds_render_geom_test && /tmp/ds_render_geom_test */
#include <assert.h>
#include <stdio.h>
#include "../source/ds_render_geom.h"
static void floor_rect(int x, int y, int w, int h, int *v) {
    ds_floor_bounds(x, y, w, h, 330, 13, &v[0], &v[1], &v[2], &v[3]);
    assert(v[0] < v[2] && v[1] < v[3]);
}
static int overlaps(int *a, int *b) {
    return a[0] < b[2] && b[0] < a[2] && a[1] < b[3] && b[1] < a[3];
}
int main(void) {
    int living[4], kitchen[4], hall[4];
    floor_rect(1, 1, 5, 5, living);
    floor_rect(6, 1, 5, 5, kitchen);
    floor_rect(1, 6, 10, 3, hall);
    assert(!overlaps(living, kitchen));
    assert(!overlaps(living, hall));
    assert(!overlaps(kitchen, hall));
    assert(living[2] == kitchen[0]);
    assert(living[3] == hall[1] && kitchen[3] == hall[1]);
    /* Every tile in a 13x13 patterned room belongs to one and only one
     * coplanar surface. Test all splits, including edge/corner placements. */
    for (int split = 1; split < 13; ++split) {
        int west[4], east[4], north[4], south[4];
        floor_rect(0, 0, split, 13, west);
        floor_rect(split, 0, 13 - split, 13, east);
        floor_rect(0, 0, 13, split, north);
        floor_rect(0, split, 13, 13 - split, south);
        assert(west[2] == east[0] && !overlaps(west,east));
        assert(north[3] == south[1] && !overlaps(north,south));
    }
    /* L-shaped house: the adjoining garage cannot receive overlapping
     * eaves across its shared edge, but exposed exterior edges still can. */
    unsigned char roof[6 * 6] = {0};
    for (int y = 1; y < 4; ++y)
        for (int x = 1; x < 3; ++x) roof[y * 6 + x] = 1;
    roof[3 * 6 + 3] = roof[4 * 6 + 3] = 1;
    assert(!ds_roof_eave_clear(roof, 6, 1, 1, 2, 3, DS_ROOF_EAST));
    assert(ds_roof_eave_clear(roof, 6, 1, 1, 2, 3, DS_ROOF_WEST));
    assert(ds_roof_eave_clear(roof, 6, 1, 1, 2, 3, DS_ROOF_NORTH));
    assert(!ds_roof_eave_clear(roof, 6, 3, 3, 1, 2, DS_ROOF_WEST));
    assert(ds_roof_eave_clear(roof, 6, 3, 3, 1, 2, DS_ROOF_EAST));
    puts("ds_render_geom_test: PASS");
    return 0;
}
