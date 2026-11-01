// dc_life.c: SAN BOREAS drawn through the PowerVR. The world (what is where) is bigmap.c's, unchanged.
#include <kos.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "dc_gfx.h"
#include "dc_life.h"
#include "bigmap.h"

#define BLK ((float)(BM_BLK * 256))      // a block in world units (18 tiles of 256, the PS1's TILE)
#define RAD 6                            // blocks drawn each way from the player
#define WIN (2 * RAD + 1)
#define HSTEP 14.0f                      // world units per elevation point (bm_height)

static BInfo win[WIN][WIN]; static int win_bx = -9999, win_by = -9999;
static int hgt[WIN + 2][WIN + 2];        // block heights with a ring round the window (for the corners)
static float px, pz, yaw, cam_yaw, zoom = 1.0f;   // the player (world units), the way they look, the camera
static const unsigned world_seed = 0x5EED1234u;

void life_init(void) {
    bm_dials_default();
    bm_new(world_seed);
}

static void window_for(int bx, int by) {
    if (bx == win_bx && by == win_by) return;
    win_bx = bx; win_by = by;
    for (int j = 0; j < WIN; j++) for (int i = 0; i < WIN; i++) bm_block(bx - RAD + i, by - RAD + j, &win[j][i]);
    for (int j = 0; j < WIN + 2; j++) for (int i = 0; i < WIN + 2; i++) hgt[j][i] = bm_height(bx - RAD - 1 + i, by - RAD - 1 + j);
}

void life_enter(void) {
    const Settle *c = bm_set(bm_capital());
    px = (c->x + 0.5f) * BLK; pz = -(c->y + 0.5f) * BLK; yaw = 0; cam_yaw = yaw;
    win_bx = win_by = -9999;
}

static const uint32_t terr_col[6] = { 0x2a5fa8, 0xe3d49a, 0x4f9a46, 0x2c6a34, 0xd1b36a, 0x8c8a86 };
static const uint32_t house_col[8] = { 0xe8d8b0, 0xd9a07a, 0xb8cfe0, 0xe0c0c8, 0xc9dca8, 0xf0e4a0, 0xd0b8e0, 0xe8e8e8 };

static uint32_t lot_colour(const BInfo *b) {
    switch (b->lot) {
        case LOT_HOUSE: case LOT_COTTAGE: case LOT_TOWN: case LOT_TWO: return house_col[(b->h >> 4) & 7];
        case LOT_APART: return 0xc88a6a;
        case LOT_SHOP: return 0xe06a5a; case LOT_DINER: return 0xe8a030; case LOT_GYM: return 0x6a78d8; case LOT_OFFICE: return 0x8aa8c0;
        case LOT_INN: return 0xb07850; case LOT_POOL: return 0x50c0e0; case LOT_SCHOOL: return 0xc8b060; case LOT_ARCADE: return 0xc050c0;
        case LOT_TOWER: return 0x7090b0; case LOT_HOTEL: return 0xb0a0d0;
        case LOT_PARK: return 0x58b058; case LOT_SQUARE: return 0xc8c0b0; case LOT_SKATE: return 0xa8a8a8;
        case LOT_FARM: return 0xc0a050; case LOT_RIVER: return 0x3a78c0;
        default: return 0xd8d0c0;
    }
}

static void flat(float x0, float z0, float x1, float z1, float y, uint32_t c) {
    V3 q[4] = { { x0, y, z0 }, { x1, y, z0 }, { x1, y, z1 }, { x0, y, z1 } };
    dc_poly(q, 4, c);
}
static void tree(float x, float y, float z, int big) {
    dc_box((V3){ x, y, z }, 40, 40, 260, 0, 1, 0x6a4a2a);
    float s = big ? 230 : 170;
    dc_box((V3){ x, y - 220, z }, s, s, big ? 520 : 380, 0, 1, 0x2f7a3a);
}

static float corner_h[WIN][WIN][4];      // the four corner heights of a block (world units, y up)
static float ground_at(float x, float z) {   // the ground height under a world point (inside the window)
    int bx = (int)floorf(x / BLK), by = (int)floorf(-z / BLK);
    int i = bx - (win_bx - RAD), j = by - (win_by - RAD);
    if (i < 0 || j < 0 || i >= WIN || j >= WIN) return 0;
    float u = x / BLK - bx, v = (-z / BLK) - by;   // 0..1 across the block (v grows to the south)
    const float *h = corner_h[j][i];               // A (0,0) B (1,0) C (1,1) D (0,1)
    return (h[0] * (1 - u) + h[1] * u) * (1 - v) + (h[3] * (1 - u) + h[2] * u) * v;
}

static void draw_block(int i, int j) {
    const BInfo *b = &win[j][i];
    int bx = win_bx - RAD + i, by = win_by - RAD + j;
    float xa = bx * BLK, xb = xa + BLK, zn = -by * BLK, zs = zn - BLK;       // west/east edges; north (z larger) and south edges
    const float *h = corner_h[j][i];
    uint32_t c = terr_col[b->terrain % 6];
    float hm = (h[0] + h[1] + h[2] + h[3]) / 4;
    if (b->terrain == BT_SEA) { flat(xa, zs, xb, zn, 0, c); return; }
    // the ground: two shaded triangles' worth, as one quad with a colour per corner (lit by the slope)
    V3 g[4] = { { xa, -h[0], zn }, { xb, -h[1], zn }, { xb, -h[2], zs }, { xa, -h[3], zs } };
    uint32_t cc[4];
    for (int k = 0; k < 4; k++) { int lit = 100 + (int)((h[(k + 1) & 3] - h[k]) / 40.0f); if (lit < 70) lit = 70; if (lit > 125) lit = 125; cc[k] = dc_shade(c, lit); }
    dc_poly_g(g, cc, 4);
    float y = -hm - 5;
    if (b->road & 1) flat(xa, zn - 3 * 256, xb, zn, y, 0x4a4a52);
    if (b->road & 2) flat(xa, zs, xa + 3 * 256, zn, y, 0x4a4a52);
    if ((b->road & 3) == 3) flat(xa, zn - 3 * 256, xa + 3 * 256, zn, y - 1, 0x55555d);
    if (b->road & 1) for (int k = 0; k < 8; k++) flat(xa + k * 576 + 120, zn - 138, xa + k * 576 + 380, zn - 118, y - 1, 0xe8d860);   // lane dashes
    // what stands on the lot
    float lx0 = xa + 5 * 256, lx1 = xa + 17 * 256, lz1 = zn - 5 * 256, lz0 = zn - 17 * 256, cx = (lx0 + lx1) / 2, cz = (lz0 + lz1) / 2;
    int st = bm_storeys(b);
    uint32_t lc = lot_colour(b);
    switch (b->lot) {
        case LOT_WILD: case LOT_EMPTY: break;
        case LOT_PARK: case LOT_SQUARE: case LOT_SKATE: case LOT_FARM:
            flat(lx0, lz0, lx1, lz1, y, lc);
            if (b->lot == LOT_PARK) for (int k = 0; k < 5; k++) {uint32_t hh = bm_hash(bx * 7 + k, by * 3 + k * 5, 99); tree(lx0 + 300 + (hh & 1023) * 2.0f, y, lz0 + 300 + ((hh >> 10) & 1023) * 2.0f, 1); }
            break;
        case LOT_RIVER: flat(xa, zs, xb, zn, y + 2, lc); break;
        default: {
            flat(lx0, lz0, lx1, lz1, y, 0x9a9a8e);   // the pad
            float hh = st * 520.0f, hx = (b->lot == LOT_TOWER || b->lot == LOT_HOTEL || b->lot == LOT_OFFICE) ? 1100 : 1300;
            if (b->lot >= LOT_HOUSE && b->lot <= LOT_TOWN) hx = 1000;
            dc_box((V3){ cx, y, cz }, hx, hx, hh, 0, 1, lc);
            if (b->lot >= LOT_HOUSE && b->lot <= LOT_APART && b->lot != LOT_APART) {   // a pitched roof on the homes
                V3 r0 = { cx - hx - 80, y - hh, cz - hx - 80 }, r1 = { cx + hx + 80, y - hh, cz - hx - 80 }, r2 = { cx + hx + 80, y - hh, cz + hx + 80 }, r3 = { cx - hx - 80, y - hh, cz + hx + 80 };
                V3 tp = { cx, y - hh - 520, cz };
                V3 t0[3] = { r0, r1, tp }, t1[3] = { r1, r2, tp }, t2[3] = { r2, r3, tp }, t3[3] = { r3, r0, tp };
                dc_poly(t0, 3, 0x9a4a38); dc_poly(t1, 3, 0x823c2e); dc_poly(t2, 3, 0xa85a46); dc_poly(t3, 3, 0x70342a);
            }
        }
    }
    if (b->terrain == BT_FOREST && b->lot == LOT_WILD) for (int k = 0; k < 7; k++) {uint32_t hh = bm_hash(bx * 13 + k, by * 17 + k * 3, 7); tree(xa + 300 + (hh & 4095) % 4000, y + 5, zs + 300 + ((hh >> 12) & 4095) % 4000, 1); }
}

static void world_draw(void) {
    // the camera behind the player, looking over their shoulder at the town
    float fx = sinf(cam_yaw), fz = cosf(cam_yaw);
    float gy = -ground_at(px, pz);
    float dist = 3800.0f * zoom, up = 3600.0f * zoom;
    V3 at = { px, gy - 300, pz };
    V3 eye = { px - fx * dist, gy - up, pz - fz * dist };
    dc_camera(eye, at);
    // far to near is not needed (the PVR's depth buffer sorts it), but the cheap clip: skip blocks wholly behind the eye
    for (int j = 0; j < WIN; j++) for (int i = 0; i < WIN; i++) {
        float cx = (win_bx - RAD + i + 0.5f) * BLK, cz = -(win_by - RAD + j + 0.5f) * BLK;
        float dx = cx - eye.x, dz = cz - eye.z;
        if (dx * fx + dz * fz < -BLK * 0.8f) continue;
        draw_block(i, j);
    }
}

// a stand-in for a Bore until look.c's people are wired in
static void bore_stand_in(V3 p, float yw, uint32_t skin, uint32_t top, uint32_t bot) {
    float s = sinf(yw), c = cosf(yw);
    dc_box((V3){ p.x, p.y, p.z }, 90, 70, 230, s, c, bot);
    dc_box((V3){ p.x, p.y - 230, p.z }, 130, 80, 300, s, c, top);
    dc_box((V3){ p.x, p.y - 530, p.z }, 100, 100, 190, s, c, skin);
}

void life_title_scene(int t) {
    dc_camera((V3){ 0, -500, 3000 }, (V3){ 0, -250, 0 });
    bore_stand_in((V3){ 0, 0, 0 }, t * 0.03f, 0xf0c8a0, 0x4a78d0, 0x3a3a6a);
}

void life_frame(int t, int pad, int pressed) {
    (void)pressed;
    float sp = (pad & CONT_A) ? 80.0f : 40.0f;
    if (pad & CONT_DPAD_LEFT) yaw += 0.045f;
    if (pad & CONT_DPAD_RIGHT) yaw -= 0.045f;
    if (pad & CONT_DPAD_UP) { px += sinf(yaw) * sp; pz += cosf(yaw) * sp; }
    if (pad & CONT_DPAD_DOWN) { px -= sinf(yaw) * sp; pz -= cosf(yaw) * sp; }
    if (pad & CONT_X) zoom *= 0.985f;
    if (pad & CONT_Y) zoom *= 1.015f;
    if (zoom < 0.4f) zoom = 0.4f; if (zoom > 3.0f) zoom = 3.0f;
    // the camera eases round to behind the player
    float d = yaw - cam_yaw; while (d > 3.14159f) d -= 6.28318f; while (d < -3.14159f) d += 6.28318f;
    cam_yaw += d * 0.08f;
    int bx = (int)floorf(px / BLK), by = (int)floorf(-pz / BLK);
    window_for(bx, by);
    for (int j = 0; j < WIN; j++) for (int i = 0; i < WIN; i++) {
        #define HH(a, b2) ((float)hgt[(j + 1) + (b2)][(i + 1) + (a)] * HSTEP)
        corner_h[j][i][0] = (HH(-1, -1) + HH(0, -1) + HH(-1, 0) + HH(0, 0)) / 4;
        corner_h[j][i][1] = (HH(0, -1) + HH(1, -1) + HH(0, 0) + HH(1, 0)) / 4;
        corner_h[j][i][2] = (HH(0, 0) + HH(1, 0) + HH(0, 1) + HH(1, 1)) / 4;
        corner_h[j][i][3] = (HH(-1, 0) + HH(0, 0) + HH(-1, 1) + HH(0, 1)) / 4;
        #undef HH
    }
    dc_frame_begin(0x4f8fe0, 0xbfdcf0);
    world_draw();
    bore_stand_in((V3){ px, -ground_at(px, pz), pz }, yaw, 0xf0c8a0, 0xd04a3a, 0x3a3a6a);
    // the interface
    dc_rect_a(4, 4, 150, 30, 0x101830, 170);
    dc_text(10, 8, 1.0f, bm_state_name(), 0xffffff);
    int sidx = bm_set_at(bx, by);
    dc_text(10, 20, 0.75f, sidx != 255 ? bm_set(sidx)->name : "OPEN COUNTRY", 0xffe080);
    char buf[64]; sprintf(buf, "BLOCK %d,%d", bx, by);
    dc_text(10, 228, 0.75f, buf, 0xffffff);
    { pvr_stats_t st; pvr_get_stats(&st); char fb[24]; sprintf(fb, "%d FPS", (int)(st.frame_rate + 0.5f)); dc_text(270, 228, 0.75f, fb, 0xffff80); }
    dc_frame_end();
}
