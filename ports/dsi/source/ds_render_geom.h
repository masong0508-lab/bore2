#ifndef BORE2_DS_RENDER_GEOM_H
#define BORE2_DS_RENDER_GEOM_H
/* Integer-only DS lot geometry: adjacent floors must meet exactly, never
 * overlap coplanarly. Origin is LOTN, with two tiles per build cell. */
static void ds_floor_bounds(int x, int y, int w, int d, int tile, int origin,
                            int *x0, int *z0, int *x1, int *z1)
{
    *x0 = (x * 2 - origin) * tile;
    *z0 = (y * 2 - origin) * tile;
    *x1 = ((x + w) * 2 - origin) * tile;
    *z1 = ((y + d) * 2 - origin) * tile;
}
/* No eaves where a roof section touches any other enclosed cell. This keeps
 * greedy roof rectangles from interpenetrating at joins and taller wings. */
enum { DS_ROOF_WEST, DS_ROOF_EAST, DS_ROOF_NORTH, DS_ROOF_SOUTH };
static int ds_roof_eave_clear(const unsigned char *occupied, int size,
                              int x, int y, int w, int d, int side)
{
    if (side == DS_ROOF_WEST || side == DS_ROOF_EAST) {
        int xx = side == DS_ROOF_WEST ? x - 1 : x + w;
        if (xx < 0 || xx >= size) return 1;
        for (int yy = y; yy < y + d; ++yy)
            if (occupied[yy * size + xx]) return 0;
    } else {
        int yy = side == DS_ROOF_NORTH ? y - 1 : y + d;
        if (yy < 0 || yy >= size) return 1;
        for (int xx = x; xx < x + w; ++xx)
            if (occupied[yy * size + xx]) return 0;
    }
    return 1;
}
/* True cutaway enclosure, not the bounding rectangle of the walls:
 * an L's courtyard, a detached garage gap and an unfenced yard stay OUTSIDE.
 * Flood at most 13x13 cells, all stack storage, no heap or save changes.
 * Doors enclose an interior; fences (wall code 6) do not. */
static int ds_cell_enclosed(const unsigned char *hw, const unsigned char *vw,
                            int n, int x, int y)
{
    if (n < 1 || n > 13 || x < 0 || y < 0 || x >= n || y >= n) return 0;
    unsigned char seen[13 * 13] = {0}, queue[13 * 13];
    int head = 0, tail = 0, pos = y * n + x;
    queue[tail++] = (unsigned char)pos; seen[pos] = 1;
    while (head < tail) {
        int c = queue[head++], cx = c % n, cy = c / n;
        for (int dir = 0; dir < 4; ++dir) {
            int nx = cx + (dir == 0) - (dir == 1);
            int ny = cy + (dir == 2) - (dir == 3);
            int wall = dir < 2 ? vw[cy * (n + 1) + cx + (dir == 0)]
                               : hw[(cy + (dir == 2)) * n + cx];
            if (wall && wall != 6) continue;
            if (nx < 0 || ny < 0 || nx >= n || ny >= n) return 0;
            int next = ny * n + nx;
            if (!seen[next]) { seen[next] = 1; queue[tail++] = (unsigned char)next; }
        }
    }
    return 1;
}
/* Stand on the surface that draw_build actually paints. The 2x2 checker
 * is raised only if every cell in its patch is empty; all other grass is at
 * height zero. Previously outdoor characters hovered 20 units above it. */
static int ds_plot_ground_lift(const unsigned char *fl, int n, int x, int y,
                               int checker)
{
    if (fl[y * n + x]) return 40;
    if (!checker) return 0;
    int bx = x & ~1, by = y & ~1;
    if (!(((bx + by) / 2) & 1)) return 0;
    int ex = bx + 1 < n ? bx + 1 : bx;
    int ey = by + 1 < n ? by + 1 : by;
    if (fl[by * n + bx] || fl[by * n + ex] ||
        fl[ey * n + bx] || fl[ey * n + ex]) return 0;
    return 20;
}
/* The planned grid draws streets only on selected edges. A missing road
 * cannot support an imaginary raised sidewalk or floating pedestrian. */
static int ds_street_ground_lift(int lx, int ly, int south, int east,
                                 int corner, int planned)
{
    if (lx < 13 && ly < 13) return 0;
    if (!planned) {
        if ((lx >= 14 && lx <= 16) || (ly >= 14 && ly <= 16)) return 0;
        return lx >= 13 || ly >= 13 ? 24 : 0;
    }
    if (lx <= 12) return south && (ly == 13 || ly == 17) ? 24 : 0;
    if (ly <= 12) return east && (lx == 13 || lx == 17) ? 24 : 0;
    int h = corner & 1, v = corner & 2;
    if (corner && !h && !v) h = v = 1;
    return ((h && (ly == 13 || ly == 17)) ||
            (v && (lx == 13 || lx == 17))) ? 24 : 0;
}
#endif
