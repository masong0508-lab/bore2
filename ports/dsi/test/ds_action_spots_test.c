/* DS/DSi playability: furniture interaction spots and exterior collision.
 * cc -std=c99 -Wall -Wextra -Werror ports/dsi/test/ds_action_spots_test.c -o /tmp/ds_action_spots_test && /tmp/ds_action_spots_test */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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
    /* Stair approaches/landings must be reversible, for every facing,
     * without stepping onto a non-existent floor or off the lot edge. */
    for (int rot = 0; rot < 4; rot++) {
        int ax, ay, bx, by, lv, rx, ry, sx, sy, downstairs;
        assert(ds_stairs_points(6, 6, rot, 0, 3, 0, 13,
                                &ax, &ay, &bx, &by, &lv));
        assert(lv == 1);
        assert((ax == 6) != (ay == 6));
        assert((bx == 6) != (by == 6));
        assert(abs(ax - 6) + abs(ay - 6) == 1);
        assert(abs(bx - 6) + abs(by - 6) == 1);
        assert(ds_stairs_points(6, 6, rot, 0, 3, 1, 13,
                                &rx, &ry, &sx, &sy, &downstairs));
        assert(downstairs == 0);
        assert(rx == bx && ry == by);
        assert(sx == ax && sy == ay);
        assert(ds_stairs_points(6, 6, rot, 2, 3, 0, 13,
                                &ax, &ay, &bx, &by, &lv));
        assert(lv == 3);
    }
    { int ax, ay, bx, by, lv;
      assert(!ds_stairs_points(0, 0, 0, 0, 3, 0, 13,
                               &ax, &ay, &bx, &by, &lv));
      assert(!ds_stairs_points(6, 6, 0, 3, 3, 0, 13,
                               &ax, &ay, &bx, &by, &lv));
      assert(!ds_stairs_points(6, 6, 0, -1, 3, 1, 13,
                               &ax, &ay, &bx, &by, &lv));
      assert(!ds_stairs_points(6, 6, 4, 0, 3, 1, 13,
                               &ax, &ay, &bx, &by, &lv));
      assert(!ds_stairs_points(6, 6, 0, 0, 3, 1, 2,
                               &ax, &ay, &bx, &by, &lv));
    }
    { int acc = 90, need = 40;
      ds_action_progress(&acc, &need, 100, 15);
      assert(acc == 5 && need == 41);
      ds_action_progress(&acc, &need, 100, 94);
      assert(acc == 99 && need == 41);
      ds_action_progress(&acc, &need, 100, 1);
      assert(acc == 0 && need == 42);
    }
    { int acc = 2147483647, need = 95;
      ds_action_progress(&acc, &need, 1, 10);
      assert(acc == 0 && need == 100);
      ds_action_progress(&acc, &need, 0, 10);
      assert(acc == 0 && need == 100);
    }
    { int acc = -100, need = -3;
      ds_action_progress(&acc, &need, 10, -2);
      assert(acc == 0 && need == 0);
      ds_action_progress(&acc, &need, 10, 11);
      assert(acc == 1 && need == 1);
    }
    { int acc = 2147483647, need = 0;
      ds_action_progress(&acc, &need, 2147483647, 2147483647);
      assert(acc == 0 && need == 2);
    }
    /* Swap-delete must not silently change an in-flight interaction
     * into a different item. The last item shifts into the freed slot. */
    assert(ds_action_reindex_after_delete(0, 0, 4) == -1);
    assert(ds_action_reindex_after_delete(4, 0, 4) == 0);
    assert(ds_action_reindex_after_delete(3, 0, 4) == 3);
    assert(ds_action_reindex_after_delete(4, 4, 4) == -1);
    assert(ds_action_reindex_after_delete(0, 4, 4) == 0);
    assert(ds_action_reindex_after_delete(-1, 0, 4) == -1);
    assert(ds_action_reindex_after_delete(9, 0, 4) == -1);
    assert(ds_action_reindex_after_delete(0, -1, 4) == -1);
    puts("ds_action_spots_test: PASS");
    return 0;
}
