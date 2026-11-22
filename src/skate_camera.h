/* Rider chase camera: keep the next ramp/rail visible, not offscreen
 * until the board is already touching it. Speed is the skate engine's
 * public fixed-step units; distance is in world units (TILE=256).
 * The normal focus smoother applies to this target, so transitions
 * and dismounts aren't abrupt. No per-frame allocation or extra rays. */
#ifndef BORE_SKATE_CAMERA_H
#define BORE_SKATE_CAMERA_H

static inline int bore_skate_camera_lead(int speed)
{
    /* Stationary/manual balance: stay centered. Normal rolling:
     * 1/2 tile ahead at speed 22. Fast riding: cap under one tile.
     * Keep this deterministic regardless of frame pacing. */
    if (speed <= 6) return 0;
    if (speed >= 36) return 240;
    return (speed - 6) * 8;
}
#endif
