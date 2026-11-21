#ifndef BORE2_DS_ACTION_SPOTS_H
#define BORE2_DS_ACTION_SPOTS_H
/* A furniture interaction is performed beside its footprint, not INSIDE its
 * solid geometry. The neighbor shares an actual edge, never a diagonal.
 * These helpers are independent of the DS SDK for host regression tests. */
static int ds_action_neighbor(int ox, int oy, int w, int h,
                              int x, int y, int *fx, int *fy)
{
    if (w < 1 || h < 1) return 0;
    if (x == ox - 1 && y >= oy && y < oy + h) {
        *fx = ox; *fy = y; return 1;
    }
    if (x == ox + w && y >= oy && y < oy + h) {
        *fx = ox + w - 1; *fy = y; return 1;
    }
    if (y == oy - 1 && x >= ox && x < ox + w) {
        *fx = x; *fy = oy; return 1;
    }
    if (y == oy + h && x >= ox && x < ox + w) {
        *fx = x; *fy = oy + h - 1; return 1;
    }
    return 0;
}
/* For a gatherable object, the selected actor may be standing next to
 * any part of the original object when the timed action completes. */
static int ds_action_reached(int ox, int oy, int w, int h, int x, int y)
{
    int fx = 0, fy = 0;
    if (w < 1 || h < 1) return 0;
    return (x >= ox && x < ox + w && y >= oy && y < oy + h) ||
           ds_action_neighbor(ox, oy, w, h, x, y, &fx, &fy);
}
#endif
