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
#if defined(BORE_DC) || defined(BORE_LENS_LEGACY)
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
#if defined(BORE_DC) || defined(BORE_LENS_LEGACY)
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
    int a=0,b=0,x=0,y=0;
    for(int i=0;i<40;i++){
        x=bore_camera_follow_axis(x,&a,1000,2,18);
        y=bore_camera_follow_axis(y,&b,-1000,2,18);
        assert(a==-b&&x+y>=-1&&x+y<=0);
    }
    int before=a;bore_camera_rebase_axis(x,&a,-18*256);
    assert(a==before-18*256*256);
    x-=18*256;assert(bore_camera_floor256(a)==x);
    assert(bore_camera_follow_axis(77,&a,77,1,18)==77);
    assert(bore_camera_floor256(-1)==-1&&bore_camera_floor256(-256)==-1);
    puts("camera_lens_test: PASS (projection, parallax, focal scale)");
    return 0;
}
