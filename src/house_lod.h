// Whole-house upper-storey LOD with a small deadband at camera thresholds.
// The existing b_lod[] byte stores the state; no per-frame allocations.
// Distances are squared in tile units, as used by the renderer.
#ifndef BORE2_HOUSE_LOD_H
#define BORE2_HOUSE_LOD_H
static inline int house_lod_upper(int previous, int sim_dist_sq, int eye_dist_sq) {
    // Enter simplified upper-storey geometry farther away than we leave it.
    // This prevents a moving camera from flickering between true walls
    // and a box when either distance changes by one tile due to rounding.
    return previous ? (sim_dist_sq > 8 * 8 || eye_dist_sq > 20 * 20)
                    : (sim_dist_sq > 10 * 10 || eye_dist_sq > 24 * 24);
}
// A rectangular impostor cannot represent an L-shaped home or courtyard.
// Only a full connected rectangle may replace exact upper-storey walls.
// The roof-region flood already knows both tile count and the bounding box.
static inline int house_lod_box_safe(int tiles, int x0, int y0, int x1, int y1) {
    return tiles > 0 && x1 >= x0 && y1 >= y0 &&
           tiles == (x1 - x0 + 1) * (y1 - y0 + 1);
}
// Even a rectangular ground floor can have a recessed second storey.
// Its simplified box may be used only when ALL visible upper storeys
// retain the ground floor's outer wall ring. An enclosed central staircase
// or skylight is fine; a missing perimeter tile is NOT. Called once while
// building the existing roof metadata, never from the frame draw loop.
static inline int house_lod_shell_ring(const char *map, int stride,
                                       int x0, int y0, int x1, int y1) {
    if (!map || stride <= 0 || x0 < 0 || y0 < 0 ||
        x1 < x0 || y1 < y0 || x1 >= stride) return 0;
    for (int x = x0; x <= x1; ++x)
        if (map[y0 * stride + x] == ' ' ||
            map[y1 * stride + x] == ' ') return 0;
    for (int y = y0 + 1; y < y1; ++y)
        if (map[y * stride + x0] == ' ' ||
            map[y * stride + x1] == ' ') return 0;
    return 1;
}
#endif
