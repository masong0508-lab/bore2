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
static void room(unsigned char *hw, unsigned char *vw, int n,
                 int x0, int y0, int x1, int y1) {
    for (int x = x0; x < x1; ++x) {
        hw[y0 * n + x] = 1;
        hw[y1 * n + x] = 1;
    }
    for (int y = y0; y < y1; ++y) {
        vw[y * (n + 1) + x0] = 1;
        vw[y * (n + 1) + x1] = 1;
    }
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
    /* Solid floors are 40 units high. Plain lawn and disconnected streets
     * are 0, not a fictitious 20/24-unit invisible platform. */
    unsigned char plot[13 * 13] = {0};
    assert(ds_plot_ground_lift(plot, 13, 0, 0, 1) == 0);
    assert(ds_plot_ground_lift(plot, 13, 2, 0, 1) == 20);
    assert(ds_plot_ground_lift(plot, 13, 3, 1, 1) == 20);
    assert(ds_plot_ground_lift(plot, 13, 2, 0, 0) == 0);
    plot[1 * 13 + 3] = 1;
    assert(ds_plot_ground_lift(plot, 13, 3, 1, 1) == 40);
    assert(ds_plot_ground_lift(plot, 13, 2, 0, 1) == 0);
    assert(ds_street_ground_lift(13, 4, 0, 1, 0, 1) == 24);
    assert(ds_street_ground_lift(13, 4, 0, 0, 0, 1) == 0);
    assert(ds_street_ground_lift(5, 13, 1, 0, 0, 1) == 24);
    assert(ds_street_ground_lift(5, 13, 0, 0, 0, 1) == 0);
    assert(ds_street_ground_lift(14, 14, 1, 1, 3, 1) == 0);
    assert(ds_street_ground_lift(13, 14, 1, 1, 3, 1) == 24);
    assert(ds_street_ground_lift(14, 14, 1, 1, 4, 1) == 0);
    assert(ds_street_ground_lift(14, 13, 1, 1, 4, 1) == 24);
    assert(ds_street_ground_lift(8, 8, 1, 1, 3, 1) == 0);
    assert(ds_street_ground_lift(8, 8, 1, 1, 3, 0) == 0);
    /* The bounding box of two wings includes open air. The cutaway test
     * must use actual wall containment, not a rectangle around both. */
    unsigned char hw[9 * 8] = {0}, vw[8 * 9] = {0};
    room(hw, vw, 8, 1, 1, 4, 4);
    room(hw, vw, 8, 4, 4, 7, 7);
    assert(ds_cell_enclosed(hw, vw, 8, 2, 2));
    assert(ds_cell_enclosed(hw, vw, 8, 5, 5));
    assert(!ds_cell_enclosed(hw, vw, 8, 5, 2));
    assert(!ds_cell_enclosed(hw, vw, 8, 0, 0));
    assert(!ds_cell_enclosed(hw, vw, 8, -1, 0));
    /* Doors (5) count as a roofed entry; a fence (6) does not. */
    hw[1 * 8 + 2] = 5;
    assert(ds_cell_enclosed(hw, vw, 8, 2, 2));
    hw[1 * 8 + 2] = 6;
    assert(!ds_cell_enclosed(hw, vw, 8, 2, 2));
    hw[1 * 8 + 2] = 1;
    /* A missing end wall opens the entire room to outdoors. */
    vw[2 * 9 + 1] = 0;
    assert(!ds_cell_enclosed(hw, vw, 8, 2, 2));
    puts("ds_render_geom_test: PASS");
    return 0;
}
