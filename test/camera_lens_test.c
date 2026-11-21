#include <assert.h>
#include <stdio.h>
#include "../src/camera_lens.h"

/* Projection consistency checks for the original PS1 320px camera:
 * GTE projects screen pixels as focal * world_offset / eye_depth.
 * The new lens and farther eye must preserve the scale of the focused
 * subject while shrinking the too-large foreground/background ratio. */
static int radius1000(int focal, int eye, int offset)
{
    assert(eye > offset);
    return (focal * 1000000) / (eye - offset);
}
static void verify(void)
{
    int close_f = bore_camera_focal(0), fp_f = bore_camera_focal(1);
    assert(fp_f == BORE_CAMERA_BASE_FOCAL);
#ifdef BORE_DC
    assert(close_f == 230);
#else
    assert(close_f == 260);
    assert(close_f > BORE_CAMERA_BASE_FOCAL);
#endif
    for (int dist = 600; dist <= 7000; dist += 125) {
        int eye = bore_camera_distance(dist, close_f);
        int old_focus = radius1000(BORE_CAMERA_BASE_FOCAL, dist, 0);
        int new_focus = radius1000(close_f, eye, 0);
        int err = old_focus - new_focus;
        if (err < 0) err = -err;
        /* Half-unit rounded integer dolly distance may cause <= 1px
         * equivalent jitter at these distances. */
        assert(err < old_focus / 300 + 2);
        if (dist >= 1000) {
            int near_old = radius1000(230, dist, 200);
            int near_new = radius1000(close_f, eye, 200);
            int far_old = radius1000(230, dist, -200);
            int far_new = radius1000(close_f, eye, -200);
#ifdef BORE_DC
            assert(near_old == near_new && far_old == far_new);
#else
            /* A foreground rail should grow LESS, the rear of a
             * staircase should look larger relative to its front. */
            assert(near_new <= near_old);
            assert(far_new >= far_old);
#endif
        }
        assert(bore_camera_distance(dist, fp_f) == dist);
    }
}
int main(void)
{
    verify();
    puts("camera_lens_test: PASS (projection, parallax, focal scale)");
    return 0;
}
