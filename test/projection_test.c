/* Real renderer entry points, using the port's existing C GTE on the host.
 * This checks projection agreement; it is not a physical-PS1 timing test. */
#include <assert.h>
#include "../src/gfx.h"

void test_near_projection(void);
void test_far_projection(void);
void test_crowd_projection(void);

static u32 packets[4096];
static u32 ot[OT_LEN];
u32 *projection_packets(void) {
    memset(packets, 0, sizeof packets);
    memset(ot, 0, sizeof ot);
    gfx_pk_next = (char *)packets;
    gfx_pk_end = (char *)(packets + 4096);
    gfx_ot_cur = ot; gfx_ot_max = OT_NEAR - 1;
    return packets;
}

/* The test never submits a GPU list. Keep allocation and lens changes real. */
void ClearOTagR(uint32_t *ot, size_t length) { memset(ot, 0, length * sizeof *ot); }
int DrawOTag(const uint32_t *ot) { (void)ot; return 0; }

int main(void) {
    InitGeom();
    gte_SetGeomOffset(CENTER_X, CENTER_Y);
    gte_SetGeomScreen(gfx_focal);
    cam.view = (MATRIX){{{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}}, {0, 0, 0}};
    gte_SetRotMatrix(&cam.view); gte_SetTransMatrix(&cam.view);
    test_near_projection();
    test_far_projection();
    test_crowd_projection();

    /* Render-to-texture scales only the GTE, then restores the world lens. */
    for (int focal = 230; focal <= 260; focal += 30) {
        gfx_set_focal(focal);
        projection_packets(); gfx_ot_cur = gfx_ot(); gfx_ot_max = OT_NEAR - 1;
        assert(gfx_imp_begin(0, 256, 48, 64, 0, 256, 1000, 2048));
        assert(dc_gte.h == focal / 2 && gfx_focal == focal);
        gfx_imp_end();
        assert(dc_gte.h == focal && gfx_focal == focal);
    }
    gfx_set_focal(230);
    assert(gfx_visible(680, 0, 1000, 0));
    gfx_set_focal(260);
    assert(!gfx_visible(680, 0, 1000, 0));
    assert(gfx_visible(600, 0, 1000, 0));
    puts("projection_test: PASS (real near/far/crowd paths, lens changes, impostor restore)");
    return 0;
}
