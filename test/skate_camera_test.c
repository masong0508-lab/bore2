#include <assert.h>
#include <stdio.h>
#include "../src/skate_camera.h"

int main(void)
{
    /* At a standstill or during a stationary manual the rider remains in
     * the center. Faster rolling reveals more of the next obstacle. */
    for (int s = -10; s <= 6; ++s)
        assert(bore_skate_camera_lead(s) == 0);
    assert(bore_skate_camera_lead(7) == 8);
    assert(bore_skate_camera_lead(10) == 32);
    assert(bore_skate_camera_lead(22) == 128);
    assert(bore_skate_camera_lead(36) == 240);
    for (int s = 37; s < 150; ++s)
        assert(bore_skate_camera_lead(s) == 240);
    int prev = 0;
    for (int s = 0; s <= 150; ++s) {
        int d = bore_skate_camera_lead(s);
        assert(d >= prev);
        assert(d >= 0 && d < 256); /* never even a whole 256-unit tile */
        prev = d;
    }
    /* Exact deterministic values, independent of refresh cadence:
     * camera_apply's existing VBLANK smoother handles moving between
     * these targets without forcing extra per-frame camera scans. */
    assert(bore_skate_camera_lead(22) - bore_skate_camera_lead(20) == 16);
    puts("skate_camera_test: PASS (speed, clamp, monotonic framing)");
    return 0;
}
