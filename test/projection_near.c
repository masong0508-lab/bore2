/* Include the real static near-plane projector; unused world code is discarded. */
#include <assert.h>
#include "../src/world.c"

void test_near_projection(void) {
    const int lenses[] = {230, 260, 200, 320};
    const int depths[] = {56, 80, 100, 115, 129, 130, 131, 160, 200, 500,
                          1024, 2599, 2600, 8192, 16000};
    int checked = 0;
    for (unsigned f = 0; f < sizeof lenses / sizeof *lenses; f++) {
        gfx_set_focal(lenses[f]);
        for (unsigned d = 0; d < sizeof depths / sizeof *depths; d++) {
            for (int x = -128; x <= 128; x += 32) {
                for (int y = -128; y <= 128; y += 32) {
                    NV n = {.x = x, .y = y, .z = depths[d]};
                    SVECTOR v = {(short)x, (short)y, (short)depths[d], 0};
                    int sx, sy, z;
                    nc_proj(&n);
                    assert(gfx_project(&v, &sx, &sy, &z));
                    /* The C GTE rounds its reciprocal through float; allow
                     * one pixel there, never the old lens's 6+ pixel seam. */
                    assert(ABS(sx - n.sx) <= 1 && ABS(sy - n.sy) <= 1);
                    checked++;
                }
            }
        }
    }
    gfx_set_focal(260);
    NV seam = {.x = 100, .y = -100, .z = 500};
    nc_proj(&seam);
    assert(seam.sx == 212 && seam.sy == 67);  /* old fixed-230 path: 206,73 */
    NV behind = {.z = NC_Z - 1, .sx = 1234, .sy = 5678};
    nc_proj(&behind);
    assert(behind.sx == 1234 && behind.sy == 5678);
    printf("near projection: %d shared-corner checks passed\n", checked);
}
