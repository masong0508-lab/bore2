/* Host guardrail for DS floor overlap, storeys and adjoining roof pieces.
 * cc -std=c99 -Wall -Wextra -Werror ports/dsi/test/ds_render_geom_test.c -o /tmp/ds_render_geom_test && /tmp/ds_render_geom_test */
#include <assert.h>
#include <stdio.h>
#include "../source/ds_render_geom.h"
/* The real, generated DS catalogue: check the *actual* stair steps and
 * the missing fixed-facing PS1 kicker aliases, not hand-picked fixtures. */
typedef unsigned char u8;
typedef unsigned short u16;
#define C15(r,g,b) ((u16)((r) | ((g) << 5) | ((b) << 10)))
#include "../source/ds_items.h"
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
    /* The renderer counts two triangles per quad; reserve room for the
     * world in the object pass, and degrade details before full capacity. */
    assert(!ds_item_over_budget(3000, 0));
    assert(!ds_item_over_budget(3350, 0));
    assert(ds_item_over_budget(3361, 0));
    assert(ds_item_over_budget(3310, 1));
    assert(ds_world_detail_budget(3600));
    assert(!ds_world_detail_budget(3601));
    assert(ds_world_far_budget(3710));
    assert(!ds_world_far_budget(3711));
    /* The preview must not invent streets in gen 7/8 low-density towns:
     * bits 0/1 are the real strips, bit 2 only the planned corner.
     * Old-world previews retain the historic both-sides treatment. */
    for (unsigned road = 0; road < 256; ++road) {
        assert(ds_preview_road_mask(road, 1) == (road & 7u));
        assert(ds_preview_road_mask(road, 0) == 3u);
    }
    assert(ds_preview_road_mask(0, 1) == 0);
    assert(ds_preview_road_mask(1, 1) == 1);
    assert(ds_preview_road_mask(2, 1) == 2);
    assert(ds_preview_road_mask(4, 1) == 4);
    /* Real L garages have a *separate* half-open wing rectangle in the
     * lot picture and distant city: the body's shared column is excluded
     * so the roof doesn't punch through the house on its common wall. */
    { int x0, z0, x1, z1;
      assert(ds_l_garage_rect(5, 8, 14, 6, 9, 7, 13, &x0, &z0, &x1, &z1));
      assert(x0 == 1 && x1 == 3 && z0 == 2 && z1 == 9);
      assert(ds_l_garage_rect(5, 8, 14, 13, 17, 7, 13, &x0, &z0, &x1, &z1));
      assert(x0 == 10 && x1 == 13 && z0 == 2 && z1 == 9);
      assert(!ds_l_garage_rect(5, 8, 14, 9, 12, 7, 13, &x0, &z0, &x1, &z1));
      assert(!ds_l_garage_rect(5, 8, 14, 13, 12, 7, 13, &x0, &z0, &x1, &z1));
    }
    /* Roofs: Hide is not the same as deleting entire upper floors. An
     * outdoor player sees all storeys under Cutaway, Always and Hide.
     * Indoors, Hide and Cutaway still expose the room being played. */
    for (int roof = 0; roof < 3; ++roof)
        assert(ds_visible_storey_top(3, 0, 1, roof, 0, 0) == 3);
    assert(ds_visible_storey_top(3, 0, 1, 0, 1, 0) == 0);
    assert(ds_visible_storey_top(3, 0, 1, 1, 1, 0) == 3);
    assert(ds_visible_storey_top(3, 0, 1, 2, 1, 0) == 0);
    assert(ds_visible_storey_top(3, 1, 0, 0, 0, 0) == 1);
    assert(ds_visible_storey_top(3, 1, 0, 0, 0, 1) == 3);
    assert(ds_visible_storey_top(0, 0, 1, 2, 0, 0) == 0);
    /* Mixed household positions must not trigger camera-wide cutaway.
     * Another Bore is indoors; the *controlled* one is outside between
     * the wings. Roof and upper floors stay visible until control moves
     * to the person inside. */
    { unsigned char ch[8 * 9] = {0}, cv[8 * 9] = {0};
      int px[3] = {2, 5, 0}, py[3] = {2, 2, 0}, n = 8;
      room(ch, cv, n, 1, 1, 4, 4);
      assert(ds_cell_enclosed(ch, cv, n, px[0], py[0]));
      assert(!ds_cell_enclosed(ch, cv, n, px[1], py[1]));
      for (int camera = 0; camera < 2; ++camera) {
          int inside = ds_cell_enclosed(ch, cv, n, px[camera], py[camera]);
          assert(ds_camera_roof_visible(0, inside) == (camera == 1));
          assert(ds_visible_storey_top(2, 0, 1, 0, inside, 0) ==
                 (camera == 0 ? 0 : 2));
          assert(ds_camera_roof_visible(1, inside) == 1);
          assert(ds_camera_roof_visible(2, inside) == 0);
          assert(ds_visible_storey_top(2, 0, 1, 1, inside, 0) == 2);
      }
      /* Camera outside with someone indoors never hides their building. */
      assert(ds_camera_roof_visible(0,
             ds_cell_enclosed(ch, cv, n, px[1], py[1])) == 1);
    }
    /* PS1's '2' and '4' are fixed-facing kickers, not separate DS items.
     * Zone generation must keep them rideable by aliasing to the real
     * Kicker item while preserving the actor's upper-storey bits. */
    assert(PS1ID['1'] != 0 && PS1ID['2'] == 0 && PS1ID['4'] == 0);
    for (int q = 0; q < 4; ++q) {
        int r = q | 4;
        assert(ds_skate_ramp_alias('2', &r) == '1');
        assert(r == (4 | 1));
        r = q | 8;
        assert(ds_skate_ramp_alias('4', &r) == '1');
        assert(r == (8 | 3));
        r = q;
        assert(ds_skate_ramp_alias('1', &r) == '1' && r == q);
        assert(ds_skate_ramp_alias('5', &r) == '5' && r == q);
        assert(ds_skate_ramp_alias('9', &r) == '9' && r == q);
    }
    assert(ds_ramp_curve_segments(0) == 3);
    assert(ds_ramp_curve_segments(1) == 4);
    assert(ds_ramp_curve_segments(2) == 5);
    for (int k = 0; k < 3; ++k) {
        int seg = ds_ramp_curve_segments(k);
        assert(seg > 2 && seg <= 6);  /* cached normals: [6] */
        assert(seg + 1 <= 8);         /* cached heights: [8] */
    }
    /* The low-LOD staircase combines two real treads per box, not all
     * eight into a featureless wall. The top meets exactly the upper
     * storey's wall/floor elevation on DS (T=330, WHF=T*21/5). */
    { const Item *st = &ITEMS[ID_STAIRS];
      assert(st->nbox == 8 && st->fw == 1 && st->fl == 1);
      assert(TABN[6][0] == 'S' && TABS[6][0] == 'S');
      int previous = 0, low_count = 0;
      for (int k = 0; k < st->nbox; k += 2) {
          const IBx *a = &IBOX[st->box0 + k];
          const IBx *b = &IBOX[st->box0 + k + 1];
          int top = a->z1 > b->z1 ? a->z1 : b->z1;
          int z0 = a->b0 < b->b0 ? a->b0 : b->b0;
          int z1 = a->b1 > b->b1 ? a->b1 : b->b1;
          assert(a->a0 == 0 && b->a1 == 8);
          assert(z1 - z0 == 2 && z0 >= 0 && z1 <= 8);
          assert(top > previous && top == 6 * (low_count + 1));
          previous = top; ++low_count;
      }
      assert(low_count == 4 && previous == 24);
      assert(previous * 7 * 330 / 40 == 330 * 21 / 5);
    }
    puts("ds_render_geom_test: PASS");
    return 0;
}
