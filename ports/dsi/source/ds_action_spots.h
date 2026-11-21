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
/* Moving across a plot border must respect a solid perimeter wall even
 * on a procedural neighborhood. Windows/fences block walking; a door (5)
 * is passable. The planner examines all four boundary orientations. */
static int ds_plot_boundary_open(const unsigned char *hw,
                                 const unsigned char *vw, int n,
                                 int ax, int ay, int bx, int by)
{
    int ina = ax >= 0 && ay >= 0 && ax < n && ay < n;
    int inb = bx >= 0 && by >= 0 && bx < n && by < n;
    if (ina == inb) return 1;
    int x = ina ? ax : bx, y = ina ? ay : by, wall = 0;
    if (ax < 0 || bx < 0) wall = vw[y * (n + 1)];
    else if (ax >= n || bx >= n) wall = vw[y * (n + 1) + n];
    else if (ay < 0 || by < 0) wall = hw[x];
    else if (ay >= n || by >= n) wall = hw[n * n + x];
    return wall == 0 || wall == 5;
}
#endif
