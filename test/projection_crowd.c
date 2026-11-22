/* Cached sprites must resize with a lens change even before their next render. */
#include <assert.h>
#include "../src/crowd.c"

void test_crowd_projection(void) {
    assert(sizeof(Spr) == 44);  /* new lens metadata fits the former padding */
    Spr s = {.z = 4600, .focal = 230, .meas = 1,
             .mx0 = -20, .my0 = -100, .mx1 = 20, .my1 = 0};
    gfx_set_focal(230); assert(sprite_depth(&s) == 4600);
    gfx_set_focal(260); assert(sprite_depth(&s) == 5200);
    VECTOR pos = {0, 0, 4600}; Gait gait = {0}; int b[4];
    assert(want_box(&s, &pos, &gait, 4600, 160, 120, b));
    assert(b[1] <= -113 && b[2] >= 22);  /* original 100px/20px bounds grow */
    s.z = 5200; s.focal = 260;
    gfx_set_focal(230); assert(sprite_depth(&s) == 4600);
    /* Maximum GTE depth and supported focal ratio still fit the later 20.12 scale. */
    s.z = 65535; s.focal = 200; gfx_set_focal(320);
    assert(sprite_depth(&s) == 104856);
    assert((sprite_depth(&s) << 12) > 0);
    gfx_set_focal(230);
    puts("crowd projection: cached lens changes and unchanged sprite storage passed");
}
