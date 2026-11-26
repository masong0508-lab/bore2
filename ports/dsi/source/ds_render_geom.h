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
/* A stairwell has no floor tile, but it still belongs below its storey's
 * roof when the surrounding walls enclose it. Inferring roofs from floor
 * tiles alone cuts a hole in the roof and puts a lower roof inside the
 * stairs. Flood OUTSIDE once per upper storey, then combine enclosure with
 * the existing floor support. Open courtyards and lower garage wings stay
 * low. Called only when rebuilding the static roof display list; the
 * caller reuses its existing rectangle-visit grid for these heights. */
static void ds_roof_height_map(unsigned char *height,
                                const unsigned char *floors,
                                const unsigned char *hw,
                                const unsigned char *vw,
                                int n, int levels)
{
    if (n < 1 || n > 13 || levels < 0 || levels > 3) return;
    for (int c = 0; c < n * n; ++c) height[c] = 0;
    for (int l = 0; l < levels; ++l) {
        const unsigned char *fl = floors + l * n * n;
        const unsigned char *h = hw + l * n * (n + 1);
        const unsigned char *v = vw + l * n * (n + 1);
        unsigned char outside[13 * 13] = {0}, queue[13 * 13];
        int head = 0, tail = 0;
        for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) {
            int west = v[y * (n + 1) + x];
            int east = v[y * (n + 1) + x + 1];
            int north = h[y * n + x], south = h[(y + 1) * n + x];
            if ((x == 0 && (!west || west == 6)) ||
                (x == n - 1 && (!east || east == 6)) ||
                (y == 0 && (!north || north == 6)) ||
                (y == n - 1 && (!south || south == 6))) {
                int c = y * n + x;
                outside[c] = 1; queue[tail++] = (unsigned char)c;
            }
        }
        while (head < tail) {
            int c = queue[head++], x = c % n, y = c / n;
            for (int dir = 0; dir < 4; ++dir) {
                int nx = x + (dir == 0) - (dir == 1);
                int ny = y + (dir == 2) - (dir == 3);
                if (nx < 0 || ny < 0 || nx >= n || ny >= n) continue;
                int next = ny * n + nx;
                int wall = dir < 2 ? v[y * (n + 1) + x + (dir == 0)]
                                  : h[(y + (dir == 2)) * n + x];
                if (outside[next] || (wall && wall != 6)) continue;
                outside[next] = 1; queue[tail++] = (unsigned char)next;
            }
        }
        for (int c = 0; c < n * n; ++c)
            if (fl[c] || !outside[c]) height[c] = (unsigned char)(l + 1);
    }
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
    const int xedge = lx == 13 || lx == 17, yedge = ly == 13 || ly == 17;
    // At a two-way intersection the raised pavement exists only at the
    // four corners. The crossing arms themselves are at road height.
    if (h && v) return xedge && yedge ? 24 : 0;
    return ((h && yedge) || (v && xedge)) ? 24 : 0;
}
/* The DS rasterizer accepts at most 2048 polygons per frame. This
 * renderer counts two triangles per quad although it emits one quad;
 * 3800 counted triangles leave margin for sky/HUD and dynamic changes.
 * Houses, floors and Sims come first. Decorations and distant lots degrade
 * before they can starve the primary geometry. */
enum { DS_RENDER_FRAME_TRI_LIMIT = 3800,
       DS_RENDER_WORLD_RESERVE = 640,
       DS_RENDER_NEAR_RESERVE = 320,
       DS_RENDER_FAR_RESERVE = 90 };
static int ds_item_over_budget(int triangles, int yard)
{
    return triangles > DS_RENDER_FRAME_TRI_LIMIT -
           DS_RENDER_WORLD_RESERVE - (yard ? 80 : 0);
}
static int ds_world_detail_budget(int triangles)
{
    return triangles <= DS_RENDER_FRAME_TRI_LIMIT - DS_RENDER_NEAR_RESERVE;
}
static int ds_world_far_budget(int triangles)
{
    return triangles <= DS_RENDER_FRAME_TRI_LIMIT - DS_RENDER_FAR_RESERVE;
}
/* The neighborhood plan uses road bits 1 (top), 2 (left), 4 (corner).
 * Old worlds always drew both strips in this preview; preserve their look.
 * For planned worlds, drawing invented roads makes the lot preview disagree
 * with the playable streets and the neighborhood map. */
static unsigned ds_preview_road_mask(unsigned road, int planned)
{
    return planned ? (road & 7u) : 3u;
}

/* Return the *exposed* half-open cell rectangle of an L-house garage wing,
 * excluding the shared column of the main house. Both map preview and
 * distant city render this exact rectangle; upper-floor/roof alignment does
 * not depend on camera scale or lot facing. Coordinates are block tiles. */
static int ds_l_garage_rect(int lot0, int body0, int body1,
                            int wing0, int wing1, int wing_y0, int wing_y1,
                            int *x0, int *z0, int *x1, int *z1)
{
    if (wing1 < wing0 || wing_y1 < wing_y0) return 0;
    int a = wing0 - lot0, b = wing1 - lot0 + 1;
    int c = body0 - lot0, d = body1 - lot0 + 1;
    if (a < c) b = c;
    else if (b > d) a = d;
    else return 0;
    *x0 = a; *x1 = b;
    *z0 = wing_y0 - lot0; *z1 = wing_y1 - lot0 + 1;
    return a < b && *z0 < *z1;
}

/* In Cutaway mode, only the player/camera's *own* building loses
 * its roof. A different member of the household entering another room
 * must not make the viewed building vanish. Always/Hide override this. */
static int ds_camera_roof_visible(int roof_mode, int camera_inside)
{
    return roof_mode == 1 || (roof_mode == 0 && !camera_inside);
}

/* A ramp model is also its skating collision surface. When Build mode
 * moves/turns an item without changing the number of objects, the
 * occupancy grid's generation changes; the cached skate surface must
 * follow it or the rider may interact with an invisible old obstacle. */
static int ds_skate_cache_changed(int key, int previous_key,
                                  unsigned epoch, unsigned previous_epoch)
{
    return key != previous_key || epoch != previous_epoch;
}

/* Preserve PS1's three fixed-facings for kicker aliases. They aren't
 * separate catalogue items on DS, so the zone loader turns them into the
 * real Kicker item with the correct orientation. The regular Kicker still
 * honours all four Buy/Build rotation choices. Keep upper-storey bits. */
static int ds_skate_ramp_alias(int ch, int *rotation)
{
    if (ch == '2' || ch == '4') {
        *rotation = (*rotation & ~3) | (ch == '2' ? 1 : 3);
        return '1';
    }
    return ch;
}

/* Actual PS1 ramps have curved surfaces; two DS chords per kicker are too
 * faceted and the board can visually float between physics sample points.
 * These counts fit the DS cached height/normal buffers (max six chords). */
static int ds_ramp_curve_segments(int kind)
{
    return kind == 2 ? 5 : kind == 1 ? 4 : 3;
}

/* An on-screen touch projects a 3D ray onto the viewed storey's
 * horizontal surface. Only intersections IN FRONT of the camera are
 * actionable. When the top of the view looks over the horizon, the
 * previous code accepted t<0 and could select a cell behind the user.
 * Reject nearly parallel rays and hits beyond the selectable scene.
 * This is used only on touch; never in the per-frame render loop. */
static int ds_touch_plane_hit(float origin_y, float ray_y,
                              float surface_y, float *distance)
{
    if (ray_y > -0.0001f && ray_y < 0.0001f) return 0;
    float t = (surface_y - origin_y) / ray_y;
    if (!(t > 0.0f) || t > 64000.0f) return 0;
    *distance = t;
    return 1;
}

/* Roof Hide hides the roof mesh, *not the upper storeys of an outdoor
 * house*. For an indoor Sim, both Hide and Cutaway continue to reveal the
 * active floor; Always retains every floor. This decision must remain
 * identical in static-list caching and in the actual draw pass. */
static int ds_visible_storey_top(int top, int viewed, int live,
                                  int roof_mode, int camera_inside, int roof_tool)
{
    int show = viewed;
    if (live && top > 0 && (roof_mode == 1 || !camera_inside)) show = top;
    if (roof_tool) show = top;
    if (show > top) show = top;
    if (show < viewed) show = viewed;
    return show;
}



// Map preview ground: same 2x2 checker scale as live; no checker on sea.
static inline int ds_preview_ground_step(int n,int checker,int water){
    return checker&&!water?2:n;
}
static inline int ds_preview_ground_end(int start,int step,int n){
    return start+step<n?start+step:n;
}


// The map preview rotates each frame, but its selected lot plan does not.
typedef struct {int x,y,gen,edit,valid;unsigned seed;} DsPreviewKey;
static inline int ds_preview_key_equal(DsPreviewKey a,DsPreviewKey b){
    return a.valid&&b.valid&&a.x==b.x&&a.y==b.y&&a.gen==b.gen&&a.edit==b.edit&&a.seed==b.seed;
}
#endif
