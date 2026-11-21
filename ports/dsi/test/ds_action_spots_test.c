/* DS/DSi playability: furniture interaction spots and exterior collision.
 * cc -std=c99 -Wall -Wextra -Werror ports/dsi/test/ds_action_spots_test.c -o /tmp/ds_action_spots_test && /tmp/ds_action_spots_test */
#include <assert.h>
#include <stdio.h>
#include "../source/ds_action_spots.h"
static void spot(int ox, int oy, int w, int h, int x, int y,
                 int px, int py) {
    int fx = -99, fy = -99;
    assert(ds_action_neighbor(ox, oy, w, h, x, y, &fx, &fy));
    assert(fx == px && fy == py);
    assert(ds_action_reached(ox, oy, w, h, x, y));
}
int main(void) {
    int fx, fy;
    spot(4, 4, 2, 2, 3, 4, 4, 4);
    spot(4, 4, 2, 2, 6, 5, 5, 5);
    spot(4, 4, 2, 2, 4, 3, 4, 4);
    spot(4, 4, 2, 2, 5, 6, 5, 5);
    assert(ds_action_reached(4, 4, 2, 2, 5, 4));
    assert(!ds_action_neighbor(4, 4, 2, 2, 3, 3, &fx, &fy));
    assert(!ds_action_reached(4, 4, 2, 2, 3, 3));
    assert(!ds_action_neighbor(4, 4, 2, 2, 4, 4, &fx, &fy));
    assert(!ds_action_neighbor(4, 4, 0, 2, 3, 4, &fx, &fy));
    assert(!ds_action_reached(4, 4, 0, 2, 4, 4));
    /* Doors are walkable; solid walls and fences are not. */
    unsigned char hwall[14 * 13] = {0}, vwall[13 * 14] = {0};
    for (int x = 0; x < 13; ++x) {
        hwall[x] = 1; hwall[13 * 13 + x] = 1;
    }
    for (int y = 0; y < 13; ++y) {
        vwall[y * 14] = 1; vwall[y * 14 + 13] = 1;
    }
    assert(!ds_plot_boundary_open(hwall, vwall, 13, 0, 4, -1, 4));
    assert(!ds_plot_boundary_open(hwall, vwall, 13, -1, 4, 0, 4));
    assert(!ds_plot_boundary_open(hwall, vwall, 13, 12, 4, 13, 4));
    assert(!ds_plot_boundary_open(hwall, vwall, 13, 5, 0, 5, -1));
    assert(!ds_plot_boundary_open(hwall, vwall, 13, 5, 12, 5, 13));
    vwall[4 * 14] = 5;  /* west door */
    assert(ds_plot_boundary_open(hwall, vwall, 13, 0, 4, -1, 4));
    vwall[4 * 14] = 6;  /* fence */
    assert(!ds_plot_boundary_open(hwall, vwall, 13, 0, 4, -1, 4));
    hwall[5] = 5;
    assert(ds_plot_boundary_open(hwall, vwall, 13, 5, -1, 5, 0));
    hwall[5] = 0;
    assert(ds_plot_boundary_open(hwall, vwall, 13, 5, 0, 5, -1));
    assert(ds_plot_boundary_open(hwall, vwall, 13, 1, 1, 2, 1));
    assert(ds_plot_boundary_open(hwall, vwall, 13, -2, 1, -1, 1));
    /* A social command must not trigger at the original location after
     * the other person walks off. One-cell proximity is permitted. */
    assert(ds_social_close(16, 16, 16, 16));
    assert(ds_social_close(16, 16, 32, 32));
    assert(ds_social_close(-16, 0, 0, 0));
    assert(!ds_social_close(16, 16, 48, 16));
    assert(!ds_social_close(16, 16, 16, 48));
    puts("ds_action_spots_test: PASS");
    return 0;
}
