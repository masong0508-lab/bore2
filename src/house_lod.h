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
#endif
