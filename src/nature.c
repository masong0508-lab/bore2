// nature: see nature.h. The textures are made here at boot (grain, ripples, fronds, leaves, bark, needles); the plants are built
// each frame from a few numbers hashed from where they stand, so no two palms lean the same way.
#include "nature.h"
#include "gfx.h"
#include "items.h"
#include "world.h"

TexRef nat_tex_ground, nat_tex_grass, nat_tex_water;
static TexRef tFrond, tLeaf, tBush, tBark, tNeedle, tFlame;

static u32 nh(u32 a, u32 b) {   // a hash of two numbers
    u32 h = a * 0x9E3779B1u + b * 0x85EBCA77u + 0x165667B1u;
    h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
    return h;
}

// ================================================================== the textures
static int c31(int v) { return v < 1 ? 1 : v > 31 ? 31 : v; }
static u16 grey5(int v) { v = c31(v); return (u16)(v | (v << 5) | (v << 10)); }
static u16 col5(int r, int g, int b) { return (u16)(c31(r) | (c31(g) << 5) | (c31(b) << 10)); }
static int tnoise(int x, int y, int cell, u32 seed, int per) {   // 0..255: smooth noise over a square of `per` texels that wraps round (cell divides it)
    int n = per / cell, gx = x / cell, gy = y / cell, fx = x % cell, fy = y % cell;
    u32 x0 = (u32)(gx % n), x1 = (u32)((gx + 1) % n), y0 = (u32)(gy % n), y1 = (u32)((gy + 1) % n);
    int a = (int)(nh(x0 + seed, y0) & 255), b = (int)(nh(x1 + seed, y0) & 255), c = (int)(nh(x0 + seed, y1) & 255), d = (int)(nh(x1 + seed, y1) & 255);
    int sx = (fx << 8) / cell, sy = (fy << 8) / cell;
    sx = (sx * sx * (768 - 2 * sx)) >> 16; sy = (sy * sy * (768 - 2 * sy)) >> 16;   // (smoothstep)
    int top = a + (((b - a) * sx) >> 8), bot = c + (((d - c) * sx) >> 8);
    return top + (((bot - top) * sy) >> 8);
}
static u16 *px;   // (atlas_scratch: free between uploads, and an upload copies it onto itself)
static void leaves(int S, int n, int rad, int lmin, int g0, u32 seed, int half) {   // a round mass of small leaves on an S x S square (lit from the top left), its edge ragged
    int mid = S * 8, SH = half ? S / 2 : S;                                    // (half: kept S x S/2, a row for every two; drawn square again)
    for (int i = 0; i < n; i++) {
        u32 r = nh((u32)i, seed);
        int a = (int)(r & 4095), d16 = isqrt((int)((r >> 12) & 255) * rad * rad);   // (even over the disc; 1/16 texels)
        int cx = mid + ((d16 * icos(a)) >> 12), cy = mid + (((d16 * isin(a)) >> 12) * 7 >> 3);
        int lr16 = (lmin + (int)((r >> 20) & 1)) * 16, la = (int)((r >> 21) & 4095);
        int lit = 128 - (((cx + cy) * 32 / S >> 4) - 32) * 4;                    // the sun's side lighter
        int g = g0 + (lit >> 5) + (int)((r >> 25) & 3), rr = 3 + (lit >> 6) + (int)((r >> 27) & 1), b = 2 + (lit >> 7);
        for (int y = -5; y <= 5; y++) for (int x = -5; x <= 5; x++) {
            int X = (cx >> 4) + x, Y = (cy >> 4) + y; if (X < 0 || Y < 0 || X >= S || Y >= S) continue;
            if (half && (Y & 1)) continue;
            int al = (x * icos(la) + y * isin(la)) >> 8, ac = (-x * isin(la) + y * icos(la)) >> 8;   // (1/16 texels, along and across the leaf)
            int e = (al * al * 256) / (lr16 * lr16) + (ac * ac * 1024) / (lr16 * lr16);
            if (e > 256) continue;
            int edge = e > 150;
            px[(half ? Y >> 1 : Y) * S + X] = (u16)(col5(rr - edge, g - edge * 3, b) | 0x8000);   // (STP set: it can be drawn see-through)
        }
    }
    (void)SH;
}
// The textures go in the spare strip of VRAM under the two screens (rows 240..255; the people's sprites start at 256),
// leaving the atlas to the rest of the game.
static int strip_x;
static TexRef strip_add(int w, int h, const u16 *p) {
    TexRef t; t.tpage = (u16)getTPage(2, 0, 0, 0); t.u = (u8)strip_x; t.v = 240; t.w = (u8)w; t.h = (u8)h; t.cap = (u8)w;
    atlas_vram_load(strip_x, 240, w, h, p); strip_x += w;
    return t;
}
void nat_init(void) {
    px = atlas_scratch();
    // the ground's grain (sand, rock, earth): grey round 16, so the corners' colours show through it
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        int n = tnoise(x, y, 8, 101, 16) + tnoise(x, y, 4, 202, 16) / 2 - 192;
        int v = 16 + n / 80;
        u32 r = nh((u32)(x * 31 + y), 303) & 255;
        if (r < 5) v -= 2; else if (r < 26) v += 2; else if (r < 96) v += (r & 1) ? 1 : 0;
        px[y * 16 + x] = grey5(v);
    }
    nat_tex_ground = strip_add(16, 16, px);
    // the grass: the same, with blades
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) px[y * 16 + x] = grey5(16 + (tnoise(x, y, 8, 404, 16) - 128) / 64 - ((nh((u32)(x * 17 + y * 5), 505) & 3) == 0));
    for (int i = 0; i < 52; i++) {
        u32 r = nh((u32)i, 606); int bx = (int)(r & 15), by = (int)((r >> 5) & 15), len = 2 + (int)((r >> 10) & 1);
        for (int k = 0; k <= len; k++) px[((by - k) & 15) * 16 + bx] = grey5(k == 0 ? 12 : k == len ? 20 : 17 + (int)((r >> 12) & 1));
        if ((r >> 13) & 1) px[((by - len) & 15) * 16 + ((bx + 1) & 15)] = grey5(19);
    }
    nat_tex_grass = strip_add(16, 16, px);
    // the sea: ripples that repeat every 16 texels (twice across, so they can slide)
    for (int y = 0; y < 16; y++) for (int x = 0; x < 32; x++) {
        int s = isin(((x + 2 * y) * 256) & 4095) * 2 + isin(((3 * x - y) * 256 + 1000) & 4095) * 2 + isin(((x - 3 * y) * 256 + 2500) & 4095) + isin(((2 * x + 3 * y) * 256 + 700) & 4095);
        int v = 16 + s / 9000; if (s > 4096 * 5) v += 1;   // (gentle: the swell and the light on it do the rest)
        px[y * 32 + x] = grey5(v);
    }
    nat_tex_water = strip_add(32, 16, px);
    // a palm frond, lying along the texture (u 0 its base, u 31 its tip): a pale midrib, leaflets sweeping toward the tip
    for (int v = 0; v < 32; v++) {
        int hw = (120 * isin(((v + 3) * 2048 / 36) & 4095)) >> 12;
        for (int u = 0; u < 16; u++) {
            int d = ABS(u * 16 + 8 - 128); u16 c = 0;
            if (d <= 12 && v < 30) c = col5(14 + v / 8, 16 + v / 10, 6);
            else if (d <= hw) {
                int ph = (v * 16 - d * 7 / 10 + 512) % 40;
                if (ph < 27 && !(d > hw - 20 && (nh((u32)u, (u32)v + 707) & 1))) {
                    int edge = ph < 4 || ph > 22;
                    c = col5(4 + v / 7 - edge, 12 + v / 6 - edge * 3 + (int)(nh((u32)(u * 7 + v), 808) & 1), 2 + v / 12);
                }
            }
            px[512 + u * 32 + v] = c ? (u16)(c | 0x8000) : 0;   // (turned: along the frond is across the texture)
        }
    }
    tFrond = strip_add(32, 16, px + 512);
    // a jungle tree's leaves (32 across, kept at half height), and a bush's with its berries
    memset(px, 0, 32 * 16 * 2); leaves(32, 95, 13, 3, 10, 909, 1); tLeaf = strip_add(32, 16, px);
    memset(px, 0, 16 * 16 * 2); leaves(16, 40, 6, 1, 8, 1010, 0);
    for (int i = 0; i < 6; i++) {
        u32 r = nh((u32)i, 1111); int a = (int)(r & 4095), d = 1 + (int)((r >> 12) % 5u);
        int X = 8 + ((d * icos(a)) >> 12), Y = 7 + ((d * isin(a)) >> 12) * 3 / 4;
        if (X < 0 || Y < 0 || X > 15 || Y > 15) continue;
        px[Y * 16 + X] = (u16)(col5(24, 3, 4) | 0x8000);
    }
    tBush = strip_add(16, 16, px);
    // bark: rings (a palm's), with fibres
    for (int y = 0; y < 16; y++) for (int x = 0; x < 8; x++) {
        int ring = y & 3, f = (nh((u32)x, (u32)y + 1000) & 1) ? 1 : -1;
        px[y * 8 + x] = ring == 0 ? col5(10, 7, 5) : ring == 1 ? col5(18 + f, 14 + f, 9) : col5(15 + f, 11, 7);
    }
    tBark = strip_add(8, 16, px);
    // pine needles
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        u16 c = col5(3, 9, 5);
        if (((x + y * 2) % 5) == 0) c = col5(6, 14, 7);
        if (nh((u32)x, (u32)y + 2000) % 7u == 0) c = col5(2, 6, 3);
        px[y * 16 + x] = c;
    }
    tNeedle = strip_add(16, 16, px);
    // a flame (tip at v 0): a white-hot core, yellow, orange, red at its edge (it is drawn adding its light)
    for (int v = 0; v < 16; v++) {
        int t = (v << 8) / 15, hw = (120 * isin((isqrt(t * 256) * 2048 / 256) & 4095)) >> 12;   // (narrow tip, round foot)
        for (int u = 0; u < 16; u++) {
            int d = ABS(u * 16 + 8 - 128); u16 c = 0;
            if (hw > 0 && d < hw) {
                int heat = ((256 - (d << 8) / hw) * (100 + (t * 156 >> 8))) >> 8;
                c = heat > 170 ? col5(31, 28, 16) : heat > 110 ? col5(30, 20, 5) : heat > 55 ? col5(24, 10, 2) : col5(12, 3, 1);
            }
            px[v * 16 + u] = c ? (u16)(c | 0x8000) : 0;
        }
    }
    tFlame = strip_add(16, 16, px);
}

// ================================================================== drawing
static const short sn256[256] = { 0, 101, 201, 301, 401, 501, 601, 700, 799, 897, 995, 1092, 1189, 1285, 1380, 1474, 1567, 1660, 1751, 1842, 1931, 2019, 2106, 2191, 2276, 2359, 2440, 2520, 2598, 2675, 2751, 2824, 2896, 2967, 3035, 3102, 3166, 3229, 3290, 3349, 3406, 3461, 3513, 3564, 3612, 3659, 3703, 3745, 3784, 3822, 3857, 3889, 3920, 3948, 3973, 3996, 4017, 4036, 4052, 4065, 4076, 4085, 4091, 4095, 4096, 4095, 4091, 4085, 4076, 4065, 4052, 4036, 4017, 3996, 3973, 3948, 3920, 3889, 3857, 3822, 3784, 3745, 3703, 3659, 3612, 3564, 3513, 3461, 3406, 3349, 3290, 3229, 3166, 3102, 3035, 2967, 2896, 2824, 2751, 2675, 2598, 2520, 2440, 2359, 2276, 2191, 2106, 2019, 1931, 1842, 1751, 1660, 1567, 1474, 1380, 1285, 1189, 1092, 995, 897, 799, 700, 601, 501, 401, 301, 201, 101, 0, -101, -201, -301, -401, -501, -601, -700, -799, -897, -995, -1092, -1189, -1285, -1380, -1474, -1567, -1660, -1751, -1842, -1931, -2019, -2106, -2191, -2276, -2359, -2440, -2520, -2598, -2675, -2751, -2824, -2896, -2967, -3035, -3102, -3166, -3229, -3290, -3349, -3406, -3461, -3513, -3564, -3612, -3659, -3703, -3745, -3784, -3822, -3857, -3889, -3920, -3948, -3973, -3996, -4017, -4036, -4052, -4065, -4076, -4085, -4091, -4095, -4096, -4095, -4091, -4085, -4076, -4065, -4052, -4036, -4017, -3996, -3973, -3948, -3920, -3889, -3857, -3822, -3784, -3745, -3703, -3659, -3612, -3564, -3513, -3461, -3406, -3349, -3290, -3229, -3166, -3102, -3035, -2967, -2896, -2824, -2751, -2675, -2598, -2520, -2440, -2359, -2276, -2191, -2106, -2019, -1931, -1842, -1751, -1660, -1567, -1474, -1380, -1285, -1189, -1092, -995, -897, -799, -700, -601, -501, -401, -301, -201, -101 };
static inline int tsin(int a) { return sn256[(a >> 4) & 255]; }   // (a table: the plants are built every frame)
static inline int tcos(int a) { return sn256[((a >> 4) + 64) & 255]; }
#define XYW(v) (*(const u32 *)&(v)->x)
__attribute__((noinline)) static void put_gt4s(const XV *a, const XV *b, const XV *c, const XV *d, u32 c0, u32 c1, u32 c2, u32 c3, u32 tp, int u0, int v0, int u1, int v1, int semi) {
    u32 *q = xv_alloc(13); if (!q) return;
    int otz = (a->z + b->z + c->z + d->z) >> 4;
    if (gfx_fog_on) { c0 = gfx_fog_apply(c0, otz); c1 = gfx_fog_apply(c1, otz); c2 = gfx_fog_apply(c2, otz); c3 = gfx_fog_apply(c3, otz); }
    u32 top = (u32)v0 << 8, bot = (u32)v1 << 8;
    q[1] = c0 | (semi ? 0x3e000000u : 0x3c000000u); q[2] = XYW(a); q[3] = (u32)u0 | top;
    q[4] = c1; q[5] = XYW(b); q[6] = ((u32)u1 | top) | tp;
    q[7] = c2; q[8] = XYW(c); q[9] = (u32)u0 | bot;
    q[10] = c3; q[11] = XYW(d); q[12] = (u32)u1 | bot;
    xv_link(q, 12, otz);
}
__attribute__((noinline)) static void put_gt3s(const XV *a, const XV *b, const XV *c, u32 c0, u32 c1, u32 c2, u32 tp, u32 uva, u32 uvb, u32 uvc, int semi) {
    u32 *q = xv_alloc(10); if (!q) return;
    int otz = ((a->z + b->z + c->z) * 0x155) >> 12;
    if (gfx_fog_on) { c0 = gfx_fog_apply(c0, otz); c1 = gfx_fog_apply(c1, otz); c2 = gfx_fog_apply(c2, otz); }
    q[1] = c0 | (semi ? 0x36000000u : 0x34000000u); q[2] = XYW(a); q[3] = uva;
    q[4] = c1; q[5] = XYW(b); q[6] = uvb | tp;
    q[7] = c2; q[8] = XYW(c); q[9] = uvc;
    xv_link(q, 9, otz);
}
__attribute__((noinline)) static void put_gt4uv(const XV *a, const XV *b, const XV *c, const XV *d, u32 c0, u32 c1, u32 c2, u32 c3, u32 tp, u32 uva, u32 uvb, u32 uvc, u32 uvd, int semi) {   // (a texture coordinate for each corner)
    u32 *q = xv_alloc(13); if (!q) return;
    int otz = (a->z + b->z + c->z + d->z) >> 4;
    if (gfx_fog_on) { c0 = gfx_fog_apply(c0, otz); c1 = gfx_fog_apply(c1, otz); c2 = gfx_fog_apply(c2, otz); c3 = gfx_fog_apply(c3, otz); }
    q[1] = c0 | (semi ? 0x3e000000u : 0x3c000000u); q[2] = XYW(a); q[3] = uva;
    q[4] = c1; q[5] = XYW(b); q[6] = uvb | tp;
    q[7] = c2; q[8] = XYW(c); q[9] = uvc;
    q[10] = c3; q[11] = XYW(d); q[12] = uvd;
    xv_link(q, 12, otz);
}
#undef XYW
#define UV(u, v) ((u32)(u) | ((u32)(v) << 8))

// the light, once a frame: on the ground (straight up), and on an upright face turned each of 16 ways
static int Lup, Ltab[16], Lfr = -1;
static void light_frame(int t) {
    if (Lfr == t) return;
    Lfr = t; gfx_set_light_yaw(0);
    Lup = gfx_light(0, -ONE, 0);
    for (int k = 0; k < 16; k++) Ltab[k] = gfx_light(tcos(k * 256), 0, tsin(k * 256));
}
static inline int Lside(int a) { return Ltab[((a + 128) >> 8) & 15]; }
__attribute__((noinline)) static u32 lit(int L) { return gfx_grade_tex((u32)CLAMP(L, 16, 255) * 0x010101u); }
__attribute__((noinline)) static u32 litc(int L, int r, int g, int b) {   // a tint (128 = as painted) in that light
    int R = CLAMP((r * L) >> 7, 0, 255), G = CLAMP((g * L) >> 7, 0, 255), B = CLAMP((b * L) >> 7, 0, 255);
    return gfx_grade_tex((u32)R | ((u32)G << 8) | ((u32)B << 16));
}
static SVECTOR sv[112] SVEC_ALIGN; static XV xv[112];   // (one plant's corners)

static void shadow(int x, int z, int r) {   // a soft round shadow on the ground
    SVECTOR A, B, C, Dd;
    setVector(&A, x - r, -2, z + r); setVector(&B, x + r, -2, z + r); setVector(&C, x - r, -2, z - r); setVector(&Dd, x + r, -2, z - r);
    gfx_shadow_quad(&A, &B, &C, &Dd);
}
// a trunk: rings of `sides` corners from the foot up, each ring (k of n) leaning further (bx, bz) the higher it is; returns the top ring's first corner
static int trunk(int base, int cx, int cz, int h, int r0, int r1, int bx, int bz, int n, int sides, int twist) {
    for (int k = 0; k <= n; k++) {
        int f = (k << 12) / n, off = (f * f) >> 12, px_ = cx + ((bx * off) >> 12), pz = cz + ((bz * off) >> 12), y = -(h * k) / n;
        int r = r0 + ((r1 - r0) * k) / n; if (k == 0) r += r0 / 4;   // (the foot flares)
        for (int s = 0; s < sides; s++) { int a = s * 4096 / sides + k * twist; setVector(&sv[base + k * sides + s], px_ + ((r * tcos(a)) >> 12), y, pz + ((r * tsin(a)) >> 12)); }
    }
    return base + n * sides;
}
static void trunk_faces(int base, int n, int sides, int twist, int shade, int semi) {
    u32 tp = (u32)tBark.tpage << 16; int u0 = tBark.u, v0 = tBark.v;
    for (int s = 0; s < sides; s++) {
        int s1 = (s + 1) % sides, a0 = s * 4096 / sides, a1 = a0 + 4096 / sides;
        for (int k = 0; k < n; k++) {
            const XV *TL = &xv[base + (k + 1) * sides + s], *TR = &xv[base + (k + 1) * sides + s1], *BL = &xv[base + k * sides + s], *BR = &xv[base + k * sides + s1];
            if (!xv_vis4(TL, TR, BL, BR, 1)) continue;
            int tw = (k * twist) & 4095, La = (Lside(a0 + tw) * shade) >> 7, Lb = (Lside(a1 + tw) * shade) >> 7;
            int Lta = La + (k + 1) * 4, Ltb = Lb + (k + 1) * 4;   // (lighter up the trunk, out of the undergrowth's shade)
            put_gt4s(TL, TR, BL, BR, lit(Lta), lit(Ltb), lit(La), lit(Lb), tp, u0, v0, u0 + 7, v0 + 15, semi);
        }
    }
}

// ---- a palm: a ringed trunk that curves as it climbs, a crown of fronds that rise, arch and hang ----
static void palm(int cx, int cz, u32 h, int semi) {
    int ht = 540 + (int)(h & 127), bend = 70 + (int)((h >> 7) & 63), ang = (int)((h >> 13) & 4095);
    int bx = (tcos(ang) * bend) >> 12, bz = (tsin(ang) * bend) >> 12;
    int far_ = 0, z;   // (further off, fewer rings, sides and fronds)
    { SVECTOR p; setVector(&p, cx, -ht / 2, cz); XV o; gfx_xform(&p, &o, 1); z = o.z; if (z < Z_NEAR - 600) return; far_ = z > 3400 ? 2 : z > 2400; }
    int NS = far_ == 2 ? 3 : far_ ? 4 : 5, PS = far_ ? 4 : 5;
    trunk(0, cx, cz, ht, 30, 18, bx, bz, NS, PS, 0);
    int ccx = cx + bx, ccz = cz + bz, ccy = -ht, base = (NS + 1) * PS;
    int nf = far_ == 2 ? 5 : far_ ? 7 : 8;
    static const short pf[4] = { 0, 1400, 2900, 4096 }, wf[4] = { 16, 74, 62, 6 };
    for (int i = 0; i < nf; i++) {   // each frond: its midline out from the crown, and its width at four points
        u32 hi = nh(h, (u32)i);
        int a = i * 4096 / nf + (int)(hi & 255) - 128, L = 290 + (int)((hi >> 8) & 63);
        int dx = tcos(a), dz = tsin(a), droop = 150 + (int)((hi >> 16) & 127);
        int dy[4] = { 0, -46, 6, droop };
        for (int k = 0; k < 4; k++) {
            int d = (L * pf[k]) >> 12, ox = ccx + ((dx * d) >> 12), oz = ccz + ((dz * d) >> 12), oy = ccy + dy[k], w = wf[k];
            setVector(&sv[base + i * 8 + k * 2], ox - ((dz * w) >> 12), oy, oz + ((dx * w) >> 12));
            setVector(&sv[base + i * 8 + k * 2 + 1], ox + ((dz * w) >> 12), oy, oz - ((dx * w) >> 12));
        }
    }
    gfx_xform(sv, xv, base + nf * 8);
    trunk_faces(0, NS, PS, 0, 116, semi);
    u32 tp = (u32)tFrond.tpage << 16; int u0 = tFrond.u, v0 = tFrond.v;
    static const u8 vv[4] = { 0, 10, 21, 31 };
    u32 fc[2][4];   // (two sets of frond colours, every other frond: base, middle, middle, tip; one a little older and yellower)
    for (int q = 0; q < 2; q++) {
        int Lf = (Lup * (104 + (int)((h >> (24 - q * 8)) & 15))) >> 7, yel = (int)((h >> (28 - q * 9)) & 7);
        fc[q][0] = litc(Lf * 78 / 100, 128 + yel * 3, 128, 120); fc[q][1] = fc[q][2] = litc(Lf, 128 + yel * 4, 128, 118); fc[q][3] = litc(Lf * 112 / 100, 128 + yel * 5, 128 + yel, 116);
    }
    for (int i = 0; i < nf; i++) {
        const u32 *c4 = fc[i & 1];
        for (int k = 0; k < 3; k++) {
            const XV *A = &xv[base + i * 8 + k * 2], *B = &xv[base + i * 8 + k * 2 + 1], *C = &xv[base + i * 8 + k * 2 + 2], *Dd = &xv[base + i * 8 + k * 2 + 3];
            if (!xv_vis4(A, B, C, Dd, 0)) continue;
            put_gt4uv(A, B, C, Dd, c4[k], c4[k], c4[k + 1], c4[k + 1], tp, UV(u0 + vv[k], v0), UV(u0 + vv[k], v0 + 15), UV(u0 + vv[k + 1], v0), UV(u0 + vv[k + 1], v0 + 15), semi);
        }
    }
    if (!far_) {   // coconuts under the crown
        u32 tp2 = (u32)tBark.tpage << 16; int n = 2 + (int)((h >> 20) & 1);
        for (int i = 0; i < n; i++) {
            int a = (int)((h >> 22) & 4095) + i * 1400, ox = ccx + ((tcos(a) * 30) >> 12), oz = ccz + ((tsin(a) * 30) >> 12), oy = ccy + 34;
            SVECTOR c4[4] SVEC_ALIGN; XV o4[4];
            setVector(&c4[0], ox - 16, oy - 16, oz); setVector(&c4[1], ox + 16, oy - 16, oz); setVector(&c4[2], ox - 16, oy + 16, oz); setVector(&c4[3], ox + 16, oy + 16, oz);
            gfx_xform(c4, o4, 4);
            if (xv_vis4(&o4[0], &o4[1], &o4[2], &o4[3], 0)) put_gt4s(&o4[0], &o4[1], &o4[2], &o4[3], litc(Lup, 120, 96, 70), litc(Lup, 110, 86, 60), litc(Lup, 70, 52, 40), litc(Lup, 60, 44, 34), tp2, tBark.u, tBark.v, tBark.u + 3, tBark.v + 3, semi);
        }
    }
    if (z < 4200) { shadow(ccx, ccz, 210); if (!far_) shadow(cx, cz, 70); }
}

// ---- a jungle tree: a stout trunk, a crown of leafy clumps (each a ring of cut-out leaves, flat on top and crossed upright) ----
static void clump(int base, int cx, int cy, int cz, int s, int yaw) {   // 12 corners: the flat one, then two upright
    int ux = (tcos(yaw) * s) >> 12, uz = (tsin(yaw) * s) >> 12;
    setVector(&sv[base + 0], cx - ux + uz, cy, cz - uz - ux); setVector(&sv[base + 1], cx + ux + uz, cy, cz + uz - ux);
    setVector(&sv[base + 2], cx - ux - uz, cy, cz - uz + ux); setVector(&sv[base + 3], cx + ux - uz, cy, cz + uz + ux);
    int top = cy - s * 3 / 4, bot = cy + s * 5 / 8;
    setVector(&sv[base + 4], cx - ux, top, cz - uz); setVector(&sv[base + 5], cx + ux, top, cz + uz);
    setVector(&sv[base + 6], cx - ux, bot, cz - uz); setVector(&sv[base + 7], cx + ux, bot, cz + uz);
    setVector(&sv[base + 8], cx + uz, top, cz - ux); setVector(&sv[base + 9], cx - uz, top, cz + ux);
    setVector(&sv[base + 10], cx + uz, bot, cz - ux); setVector(&sv[base + 11], cx - uz, bot, cz + ux);
}
static void clump_faces(int base, const TexRef *t, u32 c, u32 ct, u32 cb, int semi) {   // (the colours: on top, the upright ones' tops and feet)
    u32 tp = (u32)t->tpage << 16; int u0 = t->u, v0 = t->v, u1 = u0 + t->w - 1, v1 = v0 + t->h - 1;
    const XV *v = &xv[base];
    if (xv_vis4(&v[0], &v[1], &v[2], &v[3], 0)) put_gt4s(&v[0], &v[1], &v[2], &v[3], c, c, c, c, tp, u0, v0, u1, v1, semi);
    if (xv_vis4(&v[4], &v[5], &v[6], &v[7], 0)) put_gt4s(&v[4], &v[5], &v[6], &v[7], ct, ct, cb, cb, tp, u0, v0, u1, v1, semi);
    if (xv_vis4(&v[8], &v[9], &v[10], &v[11], 0)) put_gt4s(&v[8], &v[9], &v[10], &v[11], ct, ct, cb, cb, tp, u0, v0, u1, v1, semi);
}
static void jungle_tree(int cx, int cz, u32 h, int semi) {
    int Ht = 230 + (int)(h & 63), lean = 24 + (int)((h >> 6) & 31), ang = (int)((h >> 11) & 4095);
    int bx = (tcos(ang) * lean) >> 12, bz = (tsin(ang) * lean) >> 12;
    int far_ = 0, z;
    { SVECTOR p; setVector(&p, cx, -Ht, cz); XV o; gfx_xform(&p, &o, 1); z = o.z; if (z < Z_NEAR - 700) return; far_ = z > 2600; }
    trunk(0, cx, cz, Ht, 38, 24, bx, bz, 2, 5, 300);
    int tx = cx + bx, tz = cz + bz, base = 3 * 5, nc = z > 3800 ? 3 : far_ ? 4 : 6;
    int r = (int)((h >> 23) & 15), tr = 128 + r - 6, tg = 128 + (int)((h >> 27) & 7), tb = 122 - r / 2;   // (each tree its own green)
    clump(base, tx, -Ht - 150, tz, 122, (int)(h >> 3) & 4095);
    for (int i = 1; i < nc; i++) {
        u32 hi = nh(h, (u32)i + 40);
        int a = (i - 1) * 4096 / (nc - 1) + (int)(hi & 511) - 256, d = 92 + (int)((hi >> 9) & 31);
        clump(base + i * 12, tx + ((tcos(a) * d) >> 12), -Ht - 50 + (int)((hi >> 14) & 63) - 30, tz + ((tsin(a) * d) >> 12), 104 + (int)((hi >> 20) & 31), (int)(hi >> 4) & 4095);
    }
    gfx_xform(sv, xv, base + nc * 12);
    trunk_faces(0, 2, 5, 300, 100, semi);
    u32 cTop = litc(Lup, tr, tg, tb), cRing = litc((Lup * 112) >> 7, tr, tg, tb), cUp = litc((Lup * 104) >> 7, tr, tg, tb), cLow = litc((Lup * 64) >> 7, tr, tg, tb);
    for (int i = nc - 1; i >= 0; i--) clump_faces(base + i * 12, &tLeaf, i ? cRing : cTop, cUp, cLow, semi);   // (the low ones darker: the crown shades its own underside)
    if (z < 4200) shadow(tx, tz, 230);
}

// ---- a pine: three tiers of needles on a short trunk ----
static void pine(int cx, int cz, u32 h, int semi) {
    { SVECTOR p; setVector(&p, cx, -300, cz); XV o; gfx_xform(&p, &o, 1); if (o.z < Z_NEAR - 600) return; }
    trunk(0, cx, cz, 120, 22, 16, 0, 0, 1, 4, 0);
    int base = 2 * 4, S = 7, tip = (int)((h >> 4) & 31) - 16;
    for (int k = 0; k < 3; k++) {
        int y = -70 - 115 * k, R = 160 - 38 * k + (int)((h >> (k * 3)) & 15), hgt = 200 - 18 * k;
        for (int s = 0; s < S; s++) {
            int a = s * 4096 / S + k * 300, rr = R + (int)(nh(h, (u32)(k * 8 + s)) & 31) - 16;
            setVector(&sv[base + k * (S + 1) + s], cx + ((rr * tcos(a)) >> 12), y, cz + ((rr * tsin(a)) >> 12));
        }
        setVector(&sv[base + k * (S + 1) + S], cx + tip / 2, y - hgt, cz - tip / 3);
    }
    gfx_xform(sv, xv, base + 3 * (S + 1));
    trunk_faces(0, 1, 4, 0, 96, semi);
    u32 tp = (u32)tNeedle.tpage << 16; int u0 = tNeedle.u, v0 = tNeedle.v;
    u32 uvT = UV(u0 + 8, v0), uvL = UV(u0, v0 + 15), uvR = UV(u0 + 15, v0 + 15);
    for (int k = 0; k < 3; k++) {
        const XV *T = &xv[base + k * (S + 1) + S];
        u32 ctip = litc((Lup * 118) >> 7, 128, 132, 124);
        for (int s = 0; s < S; s++) {
            const XV *Lft = &xv[base + k * (S + 1) + s], *Rgt = &xv[base + k * (S + 1) + (s + 1) % S];
            if (!xv_vis3(T, Rgt, Lft, 1)) continue;
            int a0 = s * 4096 / S + k * 300, a1 = a0 + 4096 / S;
            int La = (Lside(a0) + Lup) * (82 + k * 6) / 256, Lb = (Lside(a1) + Lup) * (82 + k * 6) / 256;
            put_gt3s(T, Rgt, Lft, ctip, lit(Lb), lit(La), tp, uvT, uvR, uvL, semi);
        }
    }
    shadow(cx, cz, 175);
}

// ---- a berry bush: three leafy clumps, berries in them ----
static void bush(int cx, int cz, u32 h, int semi) {
    for (int i = 0; i < 3; i++) {
        u32 hi = nh(h, (u32)i + 70);
        int a = i * 1365 + (int)(hi & 511), d = 26 + (int)((hi >> 9) & 31);
        clump(i * 12, cx + ((tcos(a) * d) >> 12), -58 - (int)((hi >> 14) & 15), cz + ((tsin(a) * d) >> 12), 70 + (int)((hi >> 18) & 15), (int)(hi >> 4) & 4095);
    }
    gfx_xform(sv, xv, 36);
    u32 c0 = lit((Lup * 120) >> 7), c1 = lit((Lup * 110) >> 7), c2 = lit((Lup * 70) >> 7);
    for (int i = 0; i < 3; i++) clump_faces(i * 12, &tBush, c0, c1, c2, semi);
    shadow(cx, cz, 96);
}

// ---- a boulder: two rings of corners and a crown, every one nudged, the top often mossy ----
static void boulder(int cx, int cz, u32 h, int semi) {
    const int S = 6;
    int R0 = 96 + (int)(h & 15), R1 = 74 + (int)((h >> 4) & 15), H1 = 58 + (int)((h >> 8) & 15), H2 = 104 + (int)((h >> 12) & 31), turn = (int)((h >> 17) & 1023);
    for (int s = 0; s < S; s++) {
        u32 hs = nh(h, (u32)s);
        int a = s * 4096 / S + turn, b = a + 2048 / S, r0 = R0 + (int)(hs & 31) - 16, r1 = R1 + (int)((hs >> 5) & 31) - 16;
        setVector(&sv[s], cx + ((r0 * tcos(a)) >> 12), 0, cz + ((r0 * tsin(a)) >> 12));
        setVector(&sv[S + s], cx + ((r1 * tcos(b)) >> 12), -H1 - (int)((hs >> 10) & 15), cz + ((r1 * tsin(b)) >> 12));
    }
    setVector(&sv[2 * S], cx + (int)((h >> 20) & 31) - 16, -H2, cz + (int)((h >> 25) & 31) - 16);
    gfx_xform(sv, xv, 2 * S + 1);
    int moss = (h >> 30) & 1;
    u32 tp = (u32)nat_tex_ground.tpage << 16; int u0 = nat_tex_ground.u, v0 = nat_tex_ground.v;
    u32 uvA = UV(u0, v0 + 15), uvB = UV(u0 + 15, v0 + 15), uvC = UV(u0 + 8, v0), uvD = UV(u0 + 15, v0);
    const XV *top = &xv[2 * S];
    u32 ctop = moss ? litc(Lup, 100, 118, 84) : litc(Lup, 124, 118, 110);
    for (int s = 0; s < S; s++) {
        int s1 = (s + 1) % S, a0 = s * 4096 / S + turn, a1 = a0 + 4096 / S, am = a0 + 2048 / S;
        const XV *r0a = &xv[s], *r0b = &xv[s1], *r1a = &xv[S + s], *r1b = &xv[S + s1];
        u32 c0a = litc(Lside(a0) * 7 / 8, 118, 112, 104), c0b = litc(Lside(a1) * 7 / 8, 118, 112, 104);
        u32 c1a = moss ? litc((Lside(am) + Lup) / 2, 104, 116, 92) : litc((Lside(am) + Lup) / 2, 122, 116, 108);
        u32 c1b = moss ? litc((Lside(am + 4096 / S) + Lup) / 2, 104, 116, 92) : litc((Lside(am + 4096 / S) + Lup) / 2, 122, 116, 108);
        if (xv_vis3(r1a, r0b, r0a, 1)) put_gt3s(r1a, r0b, r0a, c1a, c0b, c0a, tp, uvC, uvB, uvA, semi);
        if (xv_vis3(r1a, r1b, r0b, 1)) put_gt3s(r1a, r1b, r0b, c1a, c1b, c0b, tp, uvC, uvD, uvB, semi);
        if (xv_vis3(top, r1b, r1a, 1)) put_gt3s(top, r1b, r1a, ctop, c1b, c1a, tp, uvC, uvB, uvA, semi);
    }
    shadow(cx, cz, 130);
}

// ---- the camp's things: a log (or a pole) between two points, a stone, the campfire, the tent, the log bench, the raft, the boat ----
static void prism(int base, int ax, int ay, int az, int bx, int by, int bz, int r, int sides) {   // 2 rings of corners round the line A-B
    int dx = bx - ax, dy = by - ay, dz = bz - az, dl = isqrt(dx * dx + dy * dy + dz * dz); if (dl < 1) dl = 1;
    dx = (dx << 12) / dl; dy = (dy << 12) / dl; dz = (dz << 12) / dl;
    int ux = -dz, uz = dx, ul = isqrt(ux * ux + uz * uz);
    if (ul < 64) { ux = 4096; uz = 0; } else { ux = (ux << 12) / ul; uz = (uz << 12) / ul; }
    int vx = (-dz * 0 + dy * uz) >> 12, vy = (dz * ux - dx * uz) >> 12, vz = (-dy * ux) >> 12;   // v = d x u (u level)
    for (int s = 0; s < sides; s++) {
        int a = s * 4096 / sides, c = tcos(a), sn = tsin(a);
        int ox = (((ux * c + vx * sn) >> 12) * r) >> 12, oy = (((vy * sn) >> 12) * r) >> 12, oz = (((uz * c + vz * sn) >> 12) * r) >> 12;
        setVector(&sv[base + s], ax + ox, ay + oy, az + oz); setVector(&sv[base + sides + s], bx + ox, by + oy, bz + oz);
    }
}
static void prism_faces(int base, int sides, const TexRef *t, int L, int tr, int tg, int tb, int semi) {   // (after gfx_xform)
    u32 tp = (u32)t->tpage << 16;
    for (int s = 0; s < sides; s++) {
        int s1 = (s + 1) % sides;
        const XV *A0 = &xv[base + s], *B0 = &xv[base + sides + s], *A1 = &xv[base + s1], *B1 = &xv[base + sides + s1];
        if (!xv_vis4(A0, B0, A1, B1, 1)) continue;
        int Ls = (L * (96 + ((tsin(s * 4096 / sides + 1024) + 4096) >> 7))) >> 7;   // (lighter on top)
        u32 c = litc(Ls, tr, tg, tb);
        put_gt4s(A0, B0, A1, B1, c, c, c, c, tp, t->u, t->v, t->u + 7, t->v + 15, semi);
    }
}
static void facing_axes(int rot, int *fx, int *fz) { *fx = rot == 1 ? 1 : rot == 3 ? -1 : 0; *fz = rot == 0 ? -1 : rot == 2 ? 1 : 0; }   // (0 S, 1 E, 2 N, 3 W)
static void glow(int cx, int cz, int rad, int r, int g, int b) {   // light on the ground round a fire: a fan, added to what is there
    SVECTOR p[9] SVEC_ALIGN; XV o[9];
    setVector(&p[0], cx, -3, cz);
    for (int k = 0; k < 8; k++) setVector(&p[1 + k], cx + ((rad * tcos(k * 512)) >> 12), -3, cz + ((rad * tsin(k * 512)) >> 12));
    gfx_xform(p, o, 9);
    u32 cc = (u32)r | ((u32)g << 8) | ((u32)b << 16);
    if (o[0].z < Z_NEAR) return;
    u32 *ot = gfx_ot() + OT_SHADOW;                           // (on the ground, under everything standing: the blend set first, put back after)
    #define LINK_(p, n) { (p)[0] = ((u32)(n) << 24) | (*ot & 0xffffff); *ot = (u32)(p) & 0xffffff; gfx_frame_prims++; }
    u32 *e0 = xv_alloc(2); if (!e0) return; e0[1] = 0xE1000000u | (1u << 9) | (1u << 10); LINK_(e0, 1)   // (drawn last: half and half again)
    for (int k = 0; k < 8; k++) {
        const XV *a = &o[0], *b2 = &o[1 + k], *c2 = &o[1 + ((k + 1) & 7)];
        if (!xv_vis3(a, b2, c2, 0)) continue;
        u32 *q = xv_alloc(7); if (!q) break;
        q[1] = 0x32000000u | cc; q[2] = *(const u32 *)&a->x; q[3] = 0; q[4] = *(const u32 *)&b2->x; q[5] = 0; q[6] = *(const u32 *)&c2->x;
        LINK_(q, 6)
    }
    u32 *e1 = xv_alloc(2); if (!e1) return; e1[1] = 0xE1000000u | (1u << 5) | (1u << 9) | (1u << 10); LINK_(e1, 1)   // (drawn first: the blend adds)
    #undef LINK_
}
static void campfire(int cx, int cz, u32 h, int t) {
    for (int i = 0; i < 7; i++) {   // the ring of stones
        int a = i * 4096 / 7 + (int)(h & 255), x = cx + ((80 * tcos(a)) >> 12), z = cz + ((80 * tsin(a)) >> 12);
        u32 hs = nh(h, (u32)i + 300);
        for (int k = 0; k < 4; k++) { int b = k * 1024 + (int)(hs & 511); setVector(&sv[i * 5 + k], x + ((22 * tcos(b)) >> 12), 0, z + ((22 * tsin(b)) >> 12)); }
        setVector(&sv[i * 5 + 4], x + (int)((hs >> 9) & 7) - 4, -24 - (int)((hs >> 12) & 7), z);
    }
    for (int i = 0; i < 4; i++) {   // the logs, leant together
        int a = i * 1024 + 300, x = cx + ((58 * tcos(a)) >> 12), z = cz + ((58 * tsin(a)) >> 12);
        prism(35 + i * 6, x, -6, z, cx + ((10 * tcos(a)) >> 12), -88, cz + ((10 * tsin(a)) >> 12), 13, 3);
    }
    gfx_xform(sv, xv, 35 + 24);
    u32 tp = (u32)nat_tex_ground.tpage << 16; int u0 = nat_tex_ground.u, v0 = nat_tex_ground.v;
    for (int i = 0; i < 7; i++) for (int k = 0; k < 4; k++) {
        const XV *T = &xv[i * 5 + 4], *A = &xv[i * 5 + k], *B = &xv[i * 5 + ((k + 1) & 3)];
        if (!xv_vis3(T, B, A, 1)) continue;
        u32 c1 = litc(Lup, 120, 116, 110), c2 = litc(Lside(k * 1024) * 3 / 4, 110, 106, 100);
        put_gt3s(T, B, A, c1, c2, c2, tp, UV(u0 + 4, v0), UV(u0 + 8, v0 + 8), UV(u0, v0 + 8), 0);
    }
    for (int i = 0; i < 4; i++) prism_faces(35 + i * 6, 3, &tBark, (Lup * 90) >> 7, 120, 100, 86, 0);
    // the flames: three crossed, flickering, adding their light
    int fl = 110 + ((tsin((t * 220) & 4095) + tsin((t * 370 + 1000) & 4095)) >> 8);
    u32 tpa = (u32)((tFlame.tpage & ~(3 << 5)) | (1 << 5)) << 16;
    SVECTOR f[12] SVEC_ALIGN; XV fo[12];
    for (int k = 0; k < 3; k++) {
        int a = k * 1365 + ((t * 9) & 255), w = 40 + k * 4, hx = (w * tcos(a)) >> 12, hz = (w * tsin(a)) >> 12, top = -fl - k * 12 - 20;
        setVector(&f[k * 4 + 0], cx - hx, top, cz - hz); setVector(&f[k * 4 + 1], cx + hx, top, cz + hz);
        setVector(&f[k * 4 + 2], cx - hx, -8, cz - hz); setVector(&f[k * 4 + 3], cx + hx, -8, cz + hz);
    }
    gfx_xform(f, fo, 12);
    for (int k = 0; k < 3; k++) if (xv_vis4(&fo[k * 4], &fo[k * 4 + 1], &fo[k * 4 + 2], &fo[k * 4 + 3], 0)) {
        u32 c = 0x808080u;
        put_gt4s(&fo[k * 4], &fo[k * 4 + 1], &fo[k * 4 + 2], &fo[k * 4 + 3], c, c, c, c, tpa, tFlame.u, tFlame.v, tFlame.u + 15, tFlame.v + 15, 1);
    }
    int night = CLAMP(200 - Lup, 20, 150);   // (the glow on the ground shows most at night)
    glow(cx, cz, 300, night * 3 / 4, night * 2 / 5, night / 8);
}
static void tent(int cx, int cz, int rot) {
    int fx, fz; facing_axes(rot, &fx, &fz);
    int sx = -fz, sz = fx;   // (across the tent)
    const int TW = 104, TL = 122, TH = 216;
    #define TP(i, a, y, l) setVector(&sv[i], cx + sx * (a) + fx * (l), y, cz + sz * (a) + fz * (l))
    TP(0, 0, -TH, TL); TP(1, 0, -TH, -TL); TP(2, -TW, 0, TL); TP(3, TW, 0, TL); TP(4, -TW, 0, -TL); TP(5, TW, 0, -TL);
    TP(6, 0, -TH + 30, TL + 2); TP(7, -TW + 18, -2, TL + 2); TP(8, TW - 18, -2, TL + 2);   // (the doorway, dark inside)
    #undef TP
    gfx_xform(sv, xv, 9);
    u32 tp = (u32)nat_tex_ground.tpage << 16; int u0 = nat_tex_ground.u, v0 = nat_tex_ground.v;
    int La = Lside(fx ? (fx > 0 ? 1024 : 3072) : (fz > 0 ? 0 : 2048));
    u32 ridge = litc(Lup, 150, 110, 64), skA = litc((Lup + La) / 2, 136, 98, 56), skB = litc((Lup * 3 / 4 + Lside(2048)) / 2, 120, 86, 50);
    if (xv_vis4(&xv[1], &xv[0], &xv[4], &xv[2], 0)) put_gt4s(&xv[1], &xv[0], &xv[4], &xv[2], ridge, ridge, skB, skB, tp, u0, v0, u0 + 15, v0 + 15, 0);
    if (xv_vis4(&xv[0], &xv[1], &xv[3], &xv[5], 0)) put_gt4s(&xv[0], &xv[1], &xv[3], &xv[5], ridge, ridge, skA, skA, tp, u0, v0, u0 + 15, v0 + 15, 0);
    u32 back = litc(Lup * 3 / 4, 130, 94, 54), door = litc(Lup / 3, 60, 40, 28);
    if (xv_vis3(&xv[1], &xv[4], &xv[5], 0)) put_gt3s(&xv[1], &xv[4], &xv[5], back, back, back, tp, UV(u0 + 8, v0), UV(u0, v0 + 15), UV(u0 + 15, v0 + 15), 0);
    if (xv_vis3(&xv[0], &xv[2], &xv[3], 0)) put_gt3s(&xv[0], &xv[2], &xv[3], back, back, back, tp, UV(u0 + 8, v0), UV(u0, v0 + 15), UV(u0 + 15, v0 + 15), 0);
    if (xv_vis3(&xv[6], &xv[7], &xv[8], 0)) put_gt3s(&xv[6], &xv[7], &xv[8], door, door, door, tp, UV(u0 + 8, v0), UV(u0, v0 + 15), UV(u0 + 15, v0 + 15), 0);
    shadow(cx, cz, 150);
}
static void log_bench(int cx, int cz, int rot) {
    int fx, fz; facing_axes(rot, &fx, &fz);
    int ax = -fz * 110, az = fx * 110;   // (the log lies across the way it faces)
    prism(0, cx - ax, -38, cz - az, cx + ax, -38, cz + az, 38, 6);
    gfx_xform(sv, xv, 12);
    prism_faces(0, 6, &tBark, (Lup * 100) >> 7, 132, 104, 80, 0);
    u32 tp = (u32)nat_tex_ground.tpage << 16; int u0 = nat_tex_ground.u, v0 = nat_tex_ground.v;
    for (int e = 0; e < 2; e++) {   // its sawn ends: pale rings
        u32 c = litc(Lup * 7 / 8, 170, 140, 96);
        for (int s = 1; s < 5; s++) {
            const XV *a = &xv[e * 6], *b = &xv[e * 6 + s], *c2 = &xv[e * 6 + s + 1];
            if (xv_vis3(a, b, c2, 0)) put_gt3s(a, b, c2, c, c, c, tp, UV(u0, v0), UV(u0 + 8, v0), UV(u0, v0 + 8), 0);
        }
    }
    shadow(cx, cz, 120);
}
static void raft(int cx, int cz) {
    for (int i = 0; i < 5; i++) prism(i * 8, cx - 118, -20, cz - 92 + i * 46, cx + 118, -20, cz - 92 + i * 46, 22, 4);
    gfx_xform(sv, xv, 40);
    for (int i = 0; i < 5; i++) prism_faces(i * 8, 4, &tBark, (Lup * (96 + (i & 1) * 10)) >> 7, 130, 104, 78, 0);
    SVECTOR b[8] SVEC_ALIGN; XV bo[8];   // two lashings across the logs
    for (int k = 0; k < 2; k++) { int x = cx - 70 + k * 140;
        setVector(&b[k * 4 + 0], x - 8, -44, cz + 112); setVector(&b[k * 4 + 1], x + 8, -44, cz + 112); setVector(&b[k * 4 + 2], x - 8, -44, cz - 112); setVector(&b[k * 4 + 3], x + 8, -44, cz - 112); }
    gfx_xform(b, bo, 8);
    u32 tp = (u32)nat_tex_ground.tpage << 16; u32 c = litc(Lup, 92, 70, 44);
    for (int k = 0; k < 2; k++) if (xv_vis4(&bo[k * 4], &bo[k * 4 + 1], &bo[k * 4 + 2], &bo[k * 4 + 3], 0))
        put_gt4s(&bo[k * 4], &bo[k * 4 + 1], &bo[k * 4 + 2], &bo[k * 4 + 3], c, c, c, c, tp, nat_tex_ground.u, nat_tex_ground.v, nat_tex_ground.u + 3, nat_tex_ground.v + 15, 0);
}
static void boat(int cx, int cz, int rot, int t, int wreck) {   // a little sailing boat: a pointed hull, a deck, a mast and its sail (a wreck: sunk in the sand, grey, its mast snapped)
    int fx, fz; facing_axes(rot, &fx, &fz);
    int sx = -fz, sz = fx, bob = wreck ? 34 : (tsin((t * 30) & 4095) >> 9) + 6;
    static const short hull[7][2] = { { 0, 170 }, { 64, 96 }, { 76, -10 }, { 66, -132 }, { -66, -132 }, { -76, -10 }, { -64, 96 } };   // (across, along)
    for (int i = 0; i < 7; i++) {
        int a = hull[i][0], l = hull[i][1];
        setVector(&sv[i], cx + sx * a + fx * l, -64 + bob, cz + sz * a + fz * l);                 // the gunwale
        setVector(&sv[7 + i], cx + sx * a * 2 / 5 + fx * l * 4 / 5, 20 + bob, cz + sz * a * 2 / 5 + fz * l * 4 / 5);   // the keel line
    }
    setVector(&sv[14], cx + fx * 20, -60 + bob, cz + fz * 20); setVector(&sv[15], cx + fx * 20 + (wreck ? sx * 90 : 0), (wreck ? -300 : -470) + bob, cz + fz * 20 + (wreck ? sz * 90 : 0));   // the mast
    setVector(&sv[16], cx + fx * 150, -78 + bob, cz + fz * 150); setVector(&sv[17], cx - fx * 104, -96 + bob, cz - fz * 104);  // (the sail's corners)
    gfx_xform(sv, xv, 18);
    u32 tp = (u32)nat_tex_ground.tpage << 16; int u0 = nat_tex_ground.u, v0 = nat_tex_ground.v;
    for (int i = 0; i < 7; i++) {
        int i1 = (i + 1) % 7;
        const XV *A = &xv[i], *B = &xv[i1], *C = &xv[7 + i], *Dd = &xv[7 + i1];
        if (!xv_vis4(A, B, C, Dd, 0)) continue;
        u32 c = wreck ? litc(Lside((i * 585 + rot * 1024) & 4095) * 3 / 4, 104, 92, 80) : litc(Lside((i * 585 + rot * 1024) & 4095) * 7 / 8, i & 1 ? 150 : 140, i & 1 ? 66 : 60, 48);
        put_gt4s(A, B, C, Dd, c, c, c, c, tp, u0, v0, u0 + 15, v0 + 7, 0);
    }
    u32 deck = wreck ? litc(Lup * 3 / 4, 110, 96, 80) : litc(Lup, 156, 122, 84);   // the deck: a fan from the bow
    for (int i = 1; i < 6; i++) if (xv_vis3(&xv[0], &xv[i], &xv[i + 1], 0)) put_gt3s(&xv[0], &xv[i], &xv[i + 1], deck, deck, deck, tp, UV(u0, v0), UV(u0 + 15, v0), UV(u0, v0 + 15), 0);
    { SVECTOR m[4] SVEC_ALIGN; XV mo[4]; const SVECTOR *p0 = &sv[14], *p1 = &sv[15];   // the mast (a thin upright quad, turned to the side)
      setVector(&m[0], p1->vx - sx * 6, p1->vy, p1->vz - sz * 6); setVector(&m[1], p1->vx + sx * 6, p1->vy, p1->vz + sz * 6);
      setVector(&m[2], p0->vx - sx * 6, p0->vy, p0->vz - sz * 6); setVector(&m[3], p0->vx + sx * 6, p0->vy, p0->vz + sz * 6);
      gfx_xform(m, mo, 4); u32 c = litc(Lup * 3 / 4, 110, 80, 50);
      if (xv_vis4(&mo[0], &mo[1], &mo[2], &mo[3], 0)) put_gt4s(&mo[0], &mo[1], &mo[2], &mo[3], c, c, c, c, tp, u0, v0, u0 + 3, v0 + 15, 0); }
    u32 sail = litc(Lup * 9 / 8, 150, 146, 136);
    if (!wreck && xv_vis3(&xv[15], &xv[16], &xv[17], 0)) put_gt3s(&xv[15], &xv[16], &xv[17], sail, sail, sail, tp, UV(u0 + 8, v0), UV(u0 + 15, v0 + 15), UV(u0, v0 + 15), 0);
}

int nat_plant_char(char c) { return c == 'g' || c == '3' || c == '6' || c == '*' || c == (char)CH_BUSH || c == 's' || c == 'z' || c == '%' || c == '@' || c == (char)CH_RAFT || c == (char)CH_WRECK; }
void nat_plant(char c, int mx, int my, int t, int fade) {
    light_frame(t);
    gfx_set_identity();
    int cx = mx * TILE + TILE / 2, cz = -(my * TILE + TILE / 2);
    u32 h = nh((u32)(mx * 131 + my), (u32)c * 7919u);
    cx += (int)((h >> 8) & 63) - 32; cz += (int)((h >> 14) & 63) - 32;   // (not on a grid)
    if (c == 's' || c == 'z' || c == '%' || c == '@' || c == (char)CH_RAFT || c == (char)CH_WRECK) {   // (the camp's things sit square on their tiles)
        int x = mx * TILE + TILE / 2, z = -(my * TILE + TILE / 2), rot = world_item_facing(mx, my);
        if (c == 's') campfire(x, z, h, t); else if (c == 'z') tent(x, z, rot); else if (c == '%') log_bench(x, z, rot);
        else if (c == '@') boat(x, z, rot, t, 0); else if (c == (char)CH_WRECK) boat(x, z, (int)(h & 3), t, 1); else raft(x, z);
        return;
    }
    switch (c) {
        case 'g': palm(cx, cz, h, fade); break;
        case '3': jungle_tree(cx, cz, h, fade); break;
        case '6': pine(cx, cz, h, fade); break;
        case '*': boulder(cx, cz, h, 0); break;
        default: bush(cx, cz, h, fade); break;
    }
}
