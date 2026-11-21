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
#endif
