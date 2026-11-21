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
/* Conversations cannot finish with someone several rooms or streets away.
 * Positions are the same 1/16-cell coordinates used by Sim movement. */
static int ds_social_close(int ax, int ay, int bx, int by)
{
    int dx = ax - bx, dy = ay - by;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx <= 16 && dy <= 16;
}
/* Return the true foot and landing of a stair on all four facings.
 * Both endpoints must exist on the 13x13 plot, and the stair must have
 * a floor above. Callers still test collision at the two endpoints. */
static int ds_stairs_points(int x, int y, int rotation,
                            int lower_level, int max_upper_level,
                            int down, int size,
                            int *from_x, int *from_y,
                            int *to_x, int *to_y, int *to_level)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { 1, 0, -1, 0 };
    if (rotation < 0 || rotation > 3 || lower_level < 0 ||
        lower_level >= max_upper_level || max_upper_level < 1 ||
        size < 3 || x < 0 || y < 0 || x >= size || y >= size) return 0;
    int sign = down ? -1 : 1;
    int ax = x + sign * dx[rotation], ay = y + sign * dy[rotation];
    int bx = x - sign * dx[rotation], by = y - sign * dy[rotation];
    if (ax < 0 || ay < 0 || ax >= size || ay >= size ||
        bx < 0 || by < 0 || bx >= size || by >= size) return 0;
    *from_x = ax; *from_y = ay;
    *to_x = bx; *to_y = by;
    *to_level = down ? lower_level : lower_level + 1;
    return 1;
}
/* Convert completed action progress to need points in constant time.
 * Unlike a per-point while-loop, this cannot freeze a handheld when an
 * older or damaged save has an unexpectedly large partial accumulator.
 * The result preserves fractional progress without changing the save type. */
static void ds_action_progress(int *acc, int *need, int duration, int gain)
{
    if (duration <= 0) duration = 1;
    unsigned int old = *acc > 0 ? (unsigned int)*acc : 0u;
    unsigned int inc = gain > 0 ? (unsigned int)gain : 0u;
    /* int accumulators can each be at most INT_MAX; their unsigned sum
     * is at most UINT_MAX-1, so no wrap is possible. */
    unsigned int total = old + inc;
    unsigned int d = (unsigned int)duration;
    unsigned int points = total / d;
    *acc = (int)(total % d);
    int current = *need < 0 ? 0 : *need > 100 ? 100 : *need;
    unsigned int space = (unsigned int)(100 - current);
    if (points > space) points = space;
    *need = current + (int)points;
}
/* Object removal swaps the last catalog entry into the emptied slot.
 * Commands aimed at the removed item cancel; commands aimed at the
 * shifted last item follow its new index. -1 means no active target. */
static int ds_action_reindex_after_delete(int index, int removed, int last)
{
    if (index < 0 || last < 0 || removed < 0 ||
        removed > last || index > last) return -1;
    if (index == removed) return -1;
    return index == last ? removed : index;
}
#endif
