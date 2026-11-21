#ifndef BORE2_DS_MAPVIEW_H
#define BORE2_DS_MAPVIEW_H
/* Pure viewport logic. The cursor always stays in view; pan only when it
 * enters a three-block edge gutter. No heap, GPU state, or frame allocations. */
static void ds_mapview_follow(int *left, int *top, int sx, int sy,
                              int width, int height, int world, int reset)
{
    if (width <= 0 || height <= 0 || world <= 0) return;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx >= world) sx = world - 1;
    if (sy >= world) sy = world - 1;
    int mx = width > 8 ? 3 : 0, my = height > 8 ? 3 : 0;
    if (reset || *left < 0 || *top < 0) {
        *left = sx - width / 2;
        *top = sy - height / 2;
    } else {
        if (sx < *left + mx) *left = sx - mx;
        else if (sx >= *left + width - mx) *left = sx - (width - mx - 1);
        if (sy < *top + my) *top = sy - my;
        else if (sy >= *top + height - my) *top = sy - (height - my - 1);
    }
    int max_x = world > width ? world - width : 0;
    int max_y = world > height ? world - height : 0;
    if (*left < 0) *left = 0;
    if (*top < 0) *top = 0;
    if (*left > max_x) *left = max_x;
    if (*top > max_y) *top = max_y;
}
#endif
