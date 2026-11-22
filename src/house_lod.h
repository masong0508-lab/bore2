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
#endif
