/* Exercise packet output from the real clipped-polygon and billboard paths. */
#include <assert.h>
#include "../src/far.c"

u32 *projection_packets(void);
static int packet_x(u32 xy) { return (s16)(xy & 0xffff); }
static int packet_y(u32 xy) { return (s16)(xy >> 16); }

void test_far_projection(void) {
    const int lenses[] = {230, 260, 200, 320};
    for (unsigned f = 0; f < sizeof lenses / sizeof *lenses; f++) {
        int focal = lenses[f]; gfx_set_focal(focal);
        FV v[3] = {{.vx = 200, .vy = -100, .vz = 1000},
                   {.vx = 300, .vy = -100, .vz = 1000},
                   {.vx = 200, .vy = 0, .vz = 1000}};
        const u32 col[3] = {0x888888, 0x888888, 0x888888};
        u32 *p = projection_packets();
        emit_clipped(v, col, 3, 100);
        assert(gfx_pk_next == (char *)(p + 7));
        assert(packet_x(p[2]) == CENTER_X + focal / 5);
        assert(packet_y(p[2]) == CENTER_Y - focal / 10);

        feye = (VECTOR){0, 0, 0}; far_gte();
        static const u8 wall[3] = {128, 128, 128};
        p = projection_packets();
        far_dot(-256, 10000, 256, 9488, 2, wall);
        assert(gfx_pk_next == (char *)(p + 6));
        int h = packet_y(p[4]) - packet_y(p[2]);
        assert(ABS(h - focal * (2 * WALL_FULL + 40) / 9744) <= 1);
    }
    /* The top-of-screen reach also follows the active vertical field of view. */
    cam.eye = (VECTOR){0, -2000, 0}; cam.at = (VECTOR){0, -1000, 1000};
    gfx_set_focal(230); assert(far_setup()); int wide_reach = max_ground;
    gfx_set_focal(260); assert(far_setup());
    assert(max_ground > 0 && max_ground < wide_reach);
    cam.eye = (VECTOR){0, 0, 0};
    puts("far projection: clipped packets, billboard size and view reach passed");
}
