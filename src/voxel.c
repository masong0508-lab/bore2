// voxel.c - a BORE person's 6x4x8 block model, drawn as real 3D with the GTE.
// Cubes, sloped wedge blocks and thinner limb blocks; textured eyes and mouth on the front; legs and arms that swing as
// 3D parts. HAIR gets its own treatment: the hull is rounded (outer corners pulled in and down), strands run on
// continuously across blocks, a sheen sits under the crown, the crown is a whorl, hanging cuts end in points, bangs hang
// over the forehead, and it is lit per vertex as a rounded mass instead of a pile of bricks.
#include "voxel.h"
#include "gfx.h"
#include "atlas.h"
#include "hairtex.inc"

static TexRef tx_side, tx_crown, tx_tips, tx_fringe, tx_curl, skin_tex, cloth_tex;
static int quads;
int vox_t_hair, vox_t_block, vox_t_face, vox_n_hair, vox_n_block;   // (debug counters, kept for the HUD readout)
static const u8 wMask2[8] = { 10, 12, 5, 3, 8, 4, 1, 2 };   // BORE: bit k = corner (k&1 ? +x : -x, k&2 ? +z : -z) drops a full block

enum { G_BODY, G_LLEG, G_RLEG, G_LARM, G_RARM, G_N };
static MATRIX gm[G_N];
static const Bore *cur;
static int walk_sin;
static int hair_cx, hair_cy, hair_cz, hair_curly, hair_style;
static int hx_, hy_, hz_, hs_;
static int cam_lx, cam_ly, cam_lz, cam_cull;
static int body_corner_light(int gx, int gz, int nx, int ny, int nz);
static u8 hl_gen = 1;   // the eye in the body's space; cull axis faces that face away (body group only)
// faces that cannot face the camera are dropped before the GTE sees them (the hide bits: 0 -z, 1 +z, 2 -x, 3 +x, 4 top, 5 bottom)
static inline int cull_bits(int x0, int x1, int y0, int y1, int z0, int z1) {
    if (!cam_cull) return 0;
    int h = 0;
    if (cam_lz > z0 - 6) h |= 1;  if (cam_lz < z1 + 6) h |= 2;
    if (cam_lx > x0 - 6) h |= 4;  if (cam_lx < x1 + 6) h |= 8;
    if (cam_ly > y0 - 6) h |= 16; if (cam_ly < y1 + 6) h |= 32;
    return h;
}

int voxel_quads(void) { return quads; }

// ---- textures ----
static u16 tbuf[16 * 16];
static u32 tseed = 99;
static int tr(void) { tseed = tseed * 1103515245u + 12345u; return (int)((tseed >> 10) & 255); }
void voxel_init(void) {
    tx_side   = atlas_add(16, 32, hair_side);
    tx_crown  = atlas_add(16, 16, hair_crown);
    tx_tips   = atlas_add(16, 16, hair_tips);
    tx_fringe = atlas_add(16, 16, hair_fringe);
    tx_curl   = atlas_add(16, 16, hair_curl);
    // skin: an even tone with the faintest grain
    for (int i = 0; i < 256; i++) { int v = (124 + (tr() % 9) - 4) >> 3; tbuf[i] = RGB(v, v, v); }
    skin_tex = atlas_add(16, 16, tbuf);
    // cloth: a weave
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { int v = 118 + (((x + y) & 1) ? 10 : -6) + (tr() % 7) - 3; v >>= 3; tbuf[y * 16 + x] = RGB(v, v, v); }
    cloth_tex = atlas_add(16, 16, tbuf);
}

// ---- colour helpers ----
static inline u32 pack(int r, int g, int b) { return (u32)CLAMP(r, 0, 255) | ((u32)CLAMP(g, 0, 255) << 8) | ((u32)CLAMP(b, 0, 255) << 16); }
static inline u32 lit(u16 c15, int light) { return pack((R8(c15) * light) >> 7, (G8(c15) * light) >> 7, (B8(c15) * light) >> 7); }
static inline u32 lit_tex(u16 c15, int light) {   // textured faces: the textures sit around 48% grey, so lift the modulation a little
    light = light * 9 / 8;
    return pack((R8(c15) * light) >> 7, (G8(c15) * light) >> 7, (B8(c15) * light) >> 7);
}

// ---- one plain face ----
// kind: 0 flat colour, 2 skin, 3 cloth
static void face(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, u16 col, int nx, int ny, int nz, int kind) {
    int L = gfx_light(nx, ny, nz);
    if (kind == 2 || kind == 3) {
        const TexRef *t = kind == 2 ? &skin_tex : &cloth_tex;
        u32 cc = lit_tex(col, L);
        quads += gfx_quad_gt(a, b, c, d, t->tpage, 0, t->u, t->v, t->u + 15, t->v, t->u, t->v + 15, t->u + 15, t->v + 15, cc, cc, cc, cc, 1);
    } else {
        u32 cc = lit(col, L);
        quads += gfx_quad_f(a, b, c, d, cc & 255, (cc >> 8) & 255, (cc >> 16) & 255, 1);
    }
}
// the lit colours of each colour slot, by light (0..255 in steps of 4), worked out once per person per frame
static u32 slot_lut[9][64];
static u8  slot_lut_ok[9];
static const Bore *lut_bore;
static void slot_lut_fill(int slot, u16 col, int textured) {
    for (int i = 0; i < 64; i++) { int L = i * 4 + 2; slot_lut[slot][i] = textured ? lit_tex(col, L) : lit(col, L); }
    slot_lut_ok[slot] = 1;
}
static inline u32 slot_col(int slot, int L) { L = CLAMP(L, 0, 255); return slot_lut[slot][L >> 2]; }
static void face4(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, int slot, int L0, int L1, int L2, int L3, int kind) {
    if (kind == 2 || kind == 3) {
        const TexRef *t = kind == 2 ? &skin_tex : &cloth_tex;
        quads += gfx_quad_gt(a, b, c, d, t->tpage, 0, t->u, t->v, t->u + 15, t->v, t->u, t->v + 15, t->u + 15, t->v + 15,
                             slot_col(slot, L0), slot_col(slot, L1), slot_col(slot, L2), slot_col(slot, L3), 1);
    } else {
        quads += gfx_quad_g(a, b, c, d, slot_col(slot, L0), slot_col(slot, L1), slot_col(slot, L2), slot_col(slot, L3), 1);
    }
}
static void tri(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, u16 col, int nx, int ny, int nz) {
    int L = gfx_light(nx, ny, nz);
    u32 cc = lit(col, L);
    quads += gfx_tri_f(a, b, c, cc & 255, (cc >> 8) & 255, (cc >> 16) & 255, 1);
}

static inline int solid_at(const Bore *b, int x, int y, int z) {
    if (x < 0 || x >= W || y < 0 || y >= H || z < 0 || z >= D) return 0;
    return b->vox[y][z][x];
}
static inline int cube_at(const Bore *b, int x, int y, int z) {   // a full cube hides the face next to it
    int v = solid_at(b, x, y, z);
    return v && (v >> 4) == 0;
}
static inline int hair_at(const Bore *b, int x, int y, int z) { int v = solid_at(b, x, y, z); return v && (v & 15) == 5; }

static int kind_of(int slot) { return slot == 1 ? 2 : (slot == 6 || slot == 7) ? 3 : 0; }

// ---- a plain block (skin, clothes, hats, parts) ----
// c = its 8 corners (top 4 then bottom 4, index k = (x?1:0)|(z?2:0)); m = wedge mask; hide = bit per face (0 -z, 1 +z, 2 -x, 3 +x, 4 top, 5 bottom)
// gx/gz = the grid cell for the rounded light (-1: light the faces flat)
static int Lf6[6];             // the light on the six axis faces of the current group: -z +z -x +x top bottom
static u8 Lrad[D + 1][W + 1];  // the radial (rounded body) light at each x,z grid point, this person, this frame
static void block(const SVECTOR *c, int m, int hide, int slot, u16 col, int kind, int gx, int gz) {
    const SVECTOR *t = c, *bt = c + 4;
    hide |= cull_bits(t[0].vx + 4, t[3].vx - 4, t[0].vy, bt[0].vy, t[0].vz + 4, t[3].vz - 4) & (m ? 0x0f : 0x3f);
    if ((hide & 0x3f) == 0x3f) return;
    int Lr[4];
    for (int k = 0; k < 4; k++) Lr[k] = (gx >= 0) ? Lrad[gz + (k >> 1)][gx + (k & 1)] : -1;
    #define VL(k, Lf) (Lr[k] >= 0 ? ((Lr[k] + (Lf)) >> 1) : (Lf))
    #define FACE4(a, b, c_, d, ka, kb, kc, kd, Lf) face4(a, b, c_, d, slot, VL(ka, Lf), VL(kb, Lf), VL(kc, Lf), VL(kd, Lf), kind);
    if (!(hide & 16)) {
        int Lf = Lf6[4];
        if (m) {
            int lx = ((m & 10) == 10) ? -ONE / 2 : ((m & 5) == 5) ? ONE / 2 : 0;
            int lz = ((m & 12) == 12) ? -ONE / 2 : ((m & 3) == 3) ? ONE / 2 : 0;
            int n1 = ((m & 10) == 10 || (m & 5) == 5 || (m & 12) == 12 || (m & 3) == 3);
            if (!n1) { lx = (m & 10) ? ONE / 3 : -ONE / 3; lz = (m & 12) ? ONE / 3 : -ONE / 3; }
            Lf = gfx_light(lx, -ONE * 3 / 4, lz);
        }
        FACE4(&t[2], &t[3], &t[0], &t[1], 2, 3, 0, 1, Lf)
    }
    #define SIDE(bit, TLk, TRk, Lf, nx, ny, nz) \
        if (!(hide & (bit))) { int lt_ = (m >> (TLk)) & 1, lr_ = (m >> (TRk)) & 1; \
            if (!(lt_ && lr_)) { \
                if (!lt_ && !lr_) FACE4(&t[TLk], &t[TRk], &bt[TLk], &bt[TRk], TLk, TRk, TLk, TRk, Lf) \
                else if (lt_) tri(&t[TRk], &bt[TRk], &bt[TLk], col, nx, ny, nz); \
                else tri(&t[TLk], &bt[TRk], &bt[TLk], col, nx, ny, nz); } }
    SIDE(2, 3, 2, Lf6[1], 0, 0, ONE)       // front +z: TL=(x1,z1)=3 TR=(x0,z1)=2
    SIDE(1, 0, 1, Lf6[0], 0, 0, -ONE)      // back -z
    SIDE(4, 2, 0, Lf6[2], -ONE, 0, 0)      // -x: TL=(x0,z1)=2 TR=(x0,z0)=0
    SIDE(8, 1, 3, Lf6[3], ONE, 0, 0)       // +x: TL=(x1,z0)=1 TR=(x1,z1)=3
    #undef SIDE
    #undef FACE4
    #undef VL
    if (!(hide & 32)) { u32 cc = slot_col(slot, Lf6[5]); quads += gfx_quad_f(&bt[3], &bt[2], &bt[1], &bt[0], cc & 255, (cc >> 8) & 255, (cc >> 16) & 255, 1); }
}
// a loose box (ears, horns, a tail): its corners made on the spot
static void box(int x0, int x1, int y0, int y1, int z0, int z1, int m, int slot, u16 col, int kind) {
    SVECTOR c[8];
    for (int k = 0; k < 4; k++) {
        int X = (k & 1) ? x1 : x0, Z = (k & 2) ? z1 : z0, low = (m >> k) & 1;
        setVector(&c[k], X, low ? y1 : y0, Z); setVector(&c[4 + k], X, y1, Z);
    }
    block(c, m, 0, slot, col, kind, -1, -1);
}
static void lights_for_group(void) {   // the six axis face lights under the current light yaw
    Lf6[0] = gfx_light(0, 0, -ONE); Lf6[1] = gfx_light(0, 0, ONE); Lf6[2] = gfx_light(-ONE, 0, 0); Lf6[3] = gfx_light(ONE, 0, 0);
    Lf6[4] = gfx_light(0, -ONE, 0); Lf6[5] = gfx_light(0, ONE, 0);
}
static void radial_table(void) {       // the rounded body light at every x,z grid point
    for (int gz = 0; gz <= D; gz++) for (int gx = 0; gx <= W; gx++) {
        int vx = gx * VOX - 3 * VOX, vz = gz * VOX - 2 * VOX;
        int len = SquareRoot0(vx * vx + vz * vz); if (len < 8) len = 8;
        Lrad[gz][gx] = (u8)CLAMP(gfx_light((vx << 12) / len, -600, (vz << 12) / len), 0, 255);
    }
}

// ================================================================== HAIR
#define HK   7      // how far an exposed hair corner is pulled in (units)
#define HK2  5      // how far it is pulled down per missing neighbour

// the displacement of the TOP corners of layer y at grid point (X, Z): every exposed corner of the hull is pulled in, so the
// body reads as a rounded shape instead of a pile of bricks; hair corners also drop where the cut is exposed from above.
// Shared corners are worked out from the same neighbourhood by every block that touches them: no cracks.
#define RK   5      // how far an exposed corner is pulled in (units)
#define HK2  5      // how far a hair corner is pulled down per missing neighbour
static void corner_disp(const Bore *b, int X, int y, int Z, int *dx, int *dz, int *drop) {
    int solid = 0, nonhair = 0, px = 0, mx = 0, pz = 0, mz = 0, above = 0;
    for (int k = 0; k < 4; k++) {
        int cx = (k & 1) ? X : X - 1, cz = (k & 2) ? Z : Z - 1;
        int v = solid_at(b, cx, y, cz);
        if (v && (v >> 4) != 1 && (v >> 4) != 2 && (v >> 4) != 3) {   // (the thin limb blocks stand apart: they do not shape the hull)
            solid++;
            if ((v & 15) != 5) nonhair++;
            if (k & 1) px++; else mx++;
            if (k & 2) pz++; else mz++;
            if (hair_at(b, cx, y + 1, cz)) above++;
        }
    }
    *dx = *dz = *drop = 0;
    if (solid == 4 || solid == 0) return;
    if (px && !mx) *dx = RK; else if (mx && !px) *dx = -RK;
    if (pz && !mz) *dz = RK; else if (mz && !pz) *dz = -RK;
    if (!nonhair && !above) *drop = (solid == 1) ? 3 * HK2 : (solid == 2) ? 2 * HK2 : 0;
}

// per-vertex light of the rounded hair mass: the normal points away from the hair centre. Cached per grid point per draw.
static u8 hl_cache[H + 2][D + 2][W + 2];
static u8 hl_stamp[H + 2][D + 2][W + 2];

static int hair_corner_light(int gx, int gy, int gz) {   // grid point (gx 0..W, gy 0..H, gz 0..D)
    if (hl_stamp[gy][gz][gx] == hl_gen) return hl_cache[gy][gz][gx];
    int px = gx * VOX - 3 * VOX, py = -gy * VOX, pz = gz * VOX - 2 * VOX;
    int vx = px - hair_cx, vy = py - hair_cy, vz = pz - hair_cz;
    int len = SquareRoot0(vx * vx + vy * vy + vz * vz); if (len < 8) len = 8;
    int Ls = gfx_light((vx << 12) / len, (vy << 12) / len, (vz << 12) / len);
    Ls = CLAMP(Ls, 0, 255);
    hl_cache[gy][gz][gx] = (u8)Ls; hl_stamp[gy][gz][gx] = hl_gen;
    return Ls;
}
// the body's rounded light: the normal leans away from the body's vertical axis (cached per x,z grid point per draw)
static u8 bl_cache[D + 2][W + 2], bl_stamp[D + 2][W + 2];
static int body_corner_light(int gx, int gz, int nx, int ny, int nz) {   // nx..nz here carry the FACE light already (see VL)
    int Lr;
    if (bl_stamp[gz][gx] == hl_gen) Lr = bl_cache[gz][gx];
    else {
        int vx = gx * VOX - 3 * VOX, vz = gz * VOX - 2 * VOX;
        int len = SquareRoot0(vx * vx + vz * vz); if (len < 8) len = 8;
        Lr = CLAMP(gfx_light((vx << 12) / len, -600, (vz << 12) / len), 0, 255);
        bl_cache[gz][gx] = (u8)Lr; bl_stamp[gz][gx] = hl_gen;
    }
    (void)ny; (void)nz;
    return (Lr + nx) >> 1;
}
static u32 hair_col(u16 col, int L, int var) {   // hair is lit a little more generously: the darkest tones would otherwise go black
    L = L * 5 / 4 + 10 + var;
    return pack((R8(col) * L) >> 7, (G8(col) * L) >> 7, (B8(col) * L) >> 7);
}

// one hair face: corners (TL, TR, BL, BR) seen from outside with their sphere lights, texture t with its uv box, face normal n, tone var
static void hface(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, int la, int lb, int lc, int ld, const TexRef *t,
                  int u0, int v0, int u1, int v1, u16 col, int nx, int ny, int nz, int var) {
    int Lf = gfx_light(nx, ny, nz);                 // mostly the sphere, a little of the face itself (keeps a hint of the block edges)
    u32 c0 = hair_col(col, (la * 3 + Lf) >> 2, var), c1 = hair_col(col, (lb * 3 + Lf) >> 2, var);
    u32 c2 = hair_col(col, (lc * 3 + Lf) >> 2, var - 6), c3 = hair_col(col, (ld * 3 + Lf) >> 2, var - 6);
    quads += gfx_quad_gt(a, b, c, d, t->tpage, 0, t->u + u0, t->v + v0, t->u + u1, t->v + v0, t->u + u0, t->v + v1, t->u + u1, t->v + v1, c0, c1, c2, c3, 1);
}

// a hair block at grid cell (x,y,z); c = its 8 corners from the cache (the rounded hull and the drops folded in)
static void hair_block(const Bore *b, int x, int y, int z, int m, int hide, u16 col, const SVECTOR *c) {
    const SVECTOR *t = c, *bt = c + 4;
    int x0 = (x - 3) * VOX, y0 = -(y + 1) * VOX, z0 = (z - 2) * VOX;
    hide |= cull_bits(x0 + 8, x0 + VOX - 8, y0 + 16, y0 + VOX - 8, z0 + 8, z0 + VOX - 8) & 0x2f;   // (rounded: a margin; the top always tilts a little)
    if ((hide & 0x3f) == 0x3f) return;
    int lt[4], lb[4];
    for (int k = 0; k < 4; k++) {
        lt[k] = hair_corner_light(x + (k & 1), y + 1, z + (k >> 1));
        lb[k] = hair_corner_light(x + (k & 1), y, z + (k >> 1));
    }
    int below_any = solid_at(b, x, y - 1, z) != 0;
    int above_hair = hair_at(b, x, y + 1, z);
    int var = ((x * 7 + y * 3 + z * 11) & 7) * 2 - 7;            // a touch of tone per block
    const TexRef *side = hair_curly ? &tx_curl : &tx_side;
    int sv0 = hair_curly ? 0 : (above_hair ? 16 : 0), sv1 = sv0 + 15;   // the top layer wears the sheen
    if (!hair_curly && !below_any && y > 0) { side = &tx_tips; sv0 = 0; sv1 = 15; }   // a hanging end: points
    // top
    if (!(hide & 16)) {
        const TexRef *tt = hair_curly ? &tx_curl : &tx_crown;
        int nw = hs_ * 2 + 2, nd = hs_ * 2 + 2;                      // the cap's columns and rows share one whorl
        int cu = CLAMP(x - (hx_ - 1), 0, nw - 1), cv = CLAMP(z - (hz_ - 1), 0, nd - 1);
        int u0 = cu * 16 / nw, u1 = (cu + 1) * 16 / nw - 1, v0 = (nd - 1 - cv) * 16 / nd, v1 = (nd - cv) * 16 / nd - 1;
        if (hair_curly) { u0 = 0; u1 = 15; v0 = 0; v1 = 15; }
        int lx = ((m & 10) == 10) ? -ONE / 2 : ((m & 5) == 5) ? ONE / 2 : 0;
        int lz = ((m & 12) == 12) ? -ONE / 2 : ((m & 3) == 3) ? ONE / 2 : 0;
        hface(&t[2], &t[3], &t[0], &t[1], lt[2], lt[3], lt[0], lt[1], tt, u0, v0, u1, v1, col, lx, -ONE, lz, var);
    }
    if (!(hide & 2)) hface(&t[3], &t[2], &bt[3], &bt[2], lt[3], lt[2], lb[3], lb[2], side, 0, sv0, 15, sv1, col, 0, 0, ONE, var);      // front +z
    if (!(hide & 1)) hface(&t[0], &t[1], &bt[0], &bt[1], lt[0], lt[1], lb[0], lb[1], side, 0, sv0, 15, sv1, col, 0, 0, -ONE, var);     // back -z
    if (!(hide & 4)) hface(&t[2], &t[0], &bt[2], &bt[0], lt[2], lt[0], lb[2], lb[0], side, 0, sv0, 15, sv1, col, -ONE, 0, 0, var);     // -x
    if (!(hide & 8)) hface(&t[1], &t[3], &bt[1], &bt[3], lt[1], lt[3], lb[1], lb[3], side, 0, sv0, 15, sv1, col, ONE, 0, 0, var);      // +x
    if (!(hide & 32)) {
        u32 cc = lit(col, gfx_light(0, ONE, 0) - 20);
        quads += gfx_quad_f(&bt[3], &bt[2], &bt[1], &bt[0], cc & 255, (cc >> 8) & 255, (cc >> 16) & 255, 1);
    }
}

// bangs over the forehead and strands down the temples, for the cuts that have them
static void hair_flaps(const Bore *b) {
    int st = hair_style;
    if (b->look[LK_HAT] == 5) return;                               // a helmet covers it all
    int bangs = (st == 1 || st == 2 || st == 7 || st == 9 || st == 11 || st == 13 || st == 14);
    int temples = (st == 2 || st == 9 || st == 11 || st == 5);
    u16 col = b->col[5];
    int hw = 2 * hs_, hh = 2 * hs_, hd = 2 * hs_;
    int top = -(hy_ + hh) * VOX;                                    // the top of the head
    if (bangs) {
        int X0 = (hx_ - 3) * VOX + 3, X1 = X0 + hw * VOX - 6, Z = (hz_ + hd - 2) * VOX + 5;
        int Y1 = top + VOX * 11 / 16;
        SVECTOR a, bb, c, d;
        setVector(&a, X1, top - 2, Z); setVector(&bb, X0, top - 2, Z); setVector(&c, X1, Y1, Z + 2); setVector(&d, X0, Y1, Z + 2);
        { int L = hair_corner_light(hx_ + hs_, hy_ + hh, hz_ + hd); hface(&a, &bb, &c, &d, L, L, L - 10, L - 10, &tx_fringe, 0, 0, 15, 15, col, 0, 0, ONE, 0); }
    }
    if (temples) {
        int Z0 = (hz_ - 2) * VOX + 2, Z1 = Z0 + hd * VOX - 4, Y1 = top + VOX * 14 / 16;
        for (int side = 0; side < 2; side++) {
            int X = side ? (hx_ + hw - 3) * VOX + 4 : (hx_ - 3) * VOX - 4;
            SVECTOR a, bb, c, d;
            if (side) { setVector(&a, X, top - 2, Z0); setVector(&bb, X, top - 2, Z1); setVector(&c, X + 2, Y1, Z0); setVector(&d, X + 2, Y1, Z1); }
            else      { setVector(&a, X, top - 2, Z1); setVector(&bb, X, top - 2, Z0); setVector(&c, X - 2, Y1, Z1); setVector(&d, X - 2, Y1, Z0); }
            { int L = hair_corner_light(side ? hx_ + hw : hx_, hy_ + hh, hz_ + hs_); hface(&a, &bb, &c, &d, L, L, L - 10, L - 10, &tx_tips, 0, 0, 15, 15, col, side ? ONE : -ONE, 0, 0, -4); }
        }
    }
}

static int group_of(u8 v, int x) {
    int sh = v >> 4;
    if (sh == 3) return x < W / 2 ? G_LLEG : G_RLEG;
    if (sh == 1 || sh == 2) return x < W / 2 ? G_LARM : G_RARM;
    return G_BODY;
}

// ---- the face art on the +z side ----
static void face_sprites(const Bore *b) {
    for (int i = 0; i < b->ndec; i++) {
        int ci_ = b->decc[i], x = ci_ % W, z = (ci_ / W) % D, y = ci_ / (W * D);
        u16 code = b->dec[y][z][x];
        int sz = ((code >> 8) & 3) + 1, fl = (code >> 10) & 1;
        int id = ((code & 7) | ((code >> 8) & 0x78)) - 1;
        int eye = id < NEYE_PUB;
        const TexRef *t = eye ? &b->tex_eye : &b->tex_mouth;
        int wc = eye ? 1 : 2, fw = wc * sz;
        int X0 = (x - 3) * VOX, X1 = X0 + fw * VOX;
        int Y0 = -(y + 1) * VOX, Y1 = -(y + 1 - sz) * VOX;
        int Z = (z - 2) * VOX + VOX + 3;
        int sh = 0, sv = 0, gw = 0, gh = 0;
        if (eye) {
            int e = (b->look[LK_EYESZ] + 4) % 9 - 4; gw = e * VOX / 9; gh = gw;
            sh = ((b->look[LK_EYESP] + 4) % 9 - 4) * VOX / 9; if (!fl) sh = -sh;
            sv = -((b->look[LK_EYEHT] + 4) % 9 - 4) * VOX / 9;
        } else {
            gw = ((b->look[LK_MOUTHW] + 4) % 9 - 4) * VOX * 2 / 9;
            sv = -((b->look[LK_MOUTHHT] + 4) % 9 - 4) * VOX / 9;
        }
        X0 -= gw / 2 - sh; X1 += gw / 2 + sh; Y0 -= gh / 2 - sv; Y1 += gh / 2 + sv;
        int mx = (X1 - X0) / 12, my = (Y1 - Y0) / 12;
        X0 += mx; X1 -= mx; Y0 += my; Y1 -= my;
        SVECTOR a, bb, c, d;
        setVector(&a, X1, Y0, Z); setVector(&bb, X0, Y0, Z); setVector(&c, X1, Y1, Z); setVector(&d, X0, Y1, Z);
        int u0 = t->u, u1 = t->u + t->w - 1;
        if (fl) { int tmp = u0; u0 = u1; u1 = tmp; }
        int L = gfx_light(0, 0, ONE);
        quads += gfx_quad_ft(&a, &bb, &c, &d, t->tpage, 0, u0, t->v, u1, t->v, u0, t->v + 7, u1, t->v + 7, CLAMP(L, 70, 140), 1, 0);
    }
}

// ---- the sprite parts BORE draws over the blocks: ears, horns, a tail, wings ----
static void extra_parts(const Bore *b, int hx, int hy, int hz, int hs) {
    int hw = 2 * hs;
    int ears = b->look[LK_EARS];
    if (ears) {
        int s = ears == 1 ? 14 : 22, yc = -(hy + 2 * hs) * VOX + VOX * 5 / 4;
        int zc = (hz - 2) * VOX + hs * VOX;
        for (int side = 0; side < 2; side++) {
            int xa = side ? (hx + hw - 3) * VOX : (hx - 3) * VOX;
            int x0 = side ? xa : xa - s, x1 = side ? xa + s : xa;
            box(x0, x1, yc - s, yc + s, zc - s / 2, zc + s / 2, 0, 1, b->col[1], 2);
        }
    }
    int horns = b->look[LK_HORNS];
    if (horns) {
        int s = horns == 1 ? 12 : 18, h = horns == 1 ? 16 : 40;
        int yt = -(hy + 2 * hs) * VOX;
        int zc = (hz - 2) * VOX + hs * VOX;
        for (int side = 0; side < 2; side++) {
            int xc = side ? (hx + hw - 3) * VOX - s : (hx - 3) * VOX + s;
            box(xc - s, xc + s, yt - h, yt, zc - s, zc + s, side ? 5 : 10, 8, b->col[8], 0);
            if (horns == 2) box(xc - s + 4, xc + s - 4, yt - h - 24, yt - h, zc - s + 4, zc + s - 4, side ? 5 : 10, 8, b->col[8], 0);
        }
    }
    int tail = b->look[LK_TAIL], ftail = b->look[LK_FTAIL];
    if (tail || ftail) {
        int n = tail == 2 ? 5 : 3, s = ftail ? 14 : 9;
        u16 col = ftail ? b->col[5] : b->col[1];
        int yc = -b->legs * VOX - 10, zc = -2 * VOX - s;
        for (int i = 0; i < n; i++) {
            int sw = (walk_sin * (i + 1)) >> 14;
            { int sl = (ftail == 1 && i == n - 1) ? 2 : (ftail ? 5 : 1); box(-s + sw, s + sw, yc - s, yc + s, zc - s, zc + s, 0, sl, b->col[sl], ftail ? 0 : 2); }
            zc -= s * 2 - 2; yc -= (tail == 2 || ftail == 2) ? 8 : -6;
        }
    }
    int back = b->look[LK_BACK];
    if (back == 2) {
        int yt = -(b->legs + b->torso) * VOX, flap = isin(walk_sin >> 1) >> 6;
        for (int side = 0; side < 2; side++) {
            int sx = side ? 1 : -1;
            SVECTOR a, bb, c, d;
            setVector(&a, sx * 30, yt - 20, -2 * VOX - 4); setVector(&bb, sx * (30 + 110), yt - 90 + flap, -2 * VOX - 30);
            setVector(&c, sx * 30, yt + 40, -2 * VOX - 4); setVector(&d, sx * (30 + 120), yt + 10 + flap, -2 * VOX - 30);
            u32 cc = lit(b->col[5], gfx_light(0, 0, -ONE));
            quads += gfx_quad_f(&a, &bb, &c, &d, cc & 255, (cc >> 8) & 255, (cc >> 16) & 255, 0);
        }
    }
}

static void part_matrix(MATRIX *out, const MATRIX *obj, int ang, int pivot_y, int pivot_z) {
    MATRIX r; SVECTOR rot = { (short)ang, 0, 0, 0 }; VECTOR p, rp, t;
    RotMatrix(&rot, &r);
    setVector(&p, 0, pivot_y, pivot_z);
    ApplyMatrixLV(&r, &p, &rp);
    setVector(&t, p.vx - rp.vx, p.vy - rp.vy, p.vz - rp.vz);
    TransMatrix(&r, &t);
    CompMatrixLV((MATRIX *)obj, &r, out);
}


// ---- the drawing cache: worked out once per look (look_build calls it) ----
void voxel_prepare(Bore *b) {
    int n = 0, nd = 0;
    for (int grp = 0; grp < G_N; grp++)
        for (int y = 0; y < H; y++) for (int z = 0; z < D; z++) for (int x = 0; x < W; x++) {
            u8 v = b->vox[y][z][x];
            if (!v || group_of(v, x) != grp) continue;
            int slot = v & 15, sh = v >> 4, hide = 0, m = 0, in = 0;
            if (sh >= 4 && sh <= 11) m = wMask2[sh - 4];
            else if (sh == 1 || sh == 2 || sh == 3) in = 1;
            if (in) {
                int up = solid_at(b, x, y + 1, z), dn = solid_at(b, x, y - 1, z);
                if (up && (up >> 4) == sh) hide |= 16;
                if (dn && ((dn >> 4) == sh || (dn >> 4) == 2 || (dn >> 4) == 1)) hide |= 32;
                if (up && (up >> 4) == 0 && sh == 1) hide |= 16;
            } else {
                if (cube_at(b, x, y, z - 1)) hide |= 1;
                if (cube_at(b, x, y, z + 1)) hide |= 2;
                if (cube_at(b, x - 1, y, z)) hide |= 4;
                if (cube_at(b, x + 1, y, z)) hide |= 8;
                if (cube_at(b, x, y + 1, z) && !m) hide |= 16;
                if (cube_at(b, x, y - 1, z) || y == 0) hide |= 32;
            }
            int is_hair = (slot == 5 && !in && grp == G_BODY);
            if (is_hair && hair_at(b, x, y + 1, z)) hide |= 16;
            SVECTOR cc[8];
            int x0 = (x - 3) * VOX, x1 = x0 + VOX, y0 = -(y + 1) * VOX, y1 = -y * VOX, z0 = (z - 2) * VOX, z1 = z0 + VOX;
            int inset = (sh == 1) ? 7 : (sh == 2) ? 9 : (sh == 3) ? 4 : 0;
            if (inset) { x0 += inset; x1 -= inset; z0 += inset; z1 -= inset; }
            int below = solid_at(b, x, y - 1, z); if (below && ((below >> 4) == 1 || (below >> 4) == 2 || (below >> 4) == 3)) below = 0;
            for (int k = 0; k < 4; k++) {
                int X = (k & 1) ? x1 : x0, Z = (k & 2) ? z1 : z0, low = (m >> k) & 1;
                int dx = 0, dz = 0, drop = 0, bx = 0, bz = 0;
                if (!in) {
                    int gX = (k & 1) ? x + 1 : x, gZ = (k & 2) ? z + 1 : z, d2;
                    corner_disp(b, gX, y, gZ, &dx, &dz, &drop);
                    if (!is_hair) drop = 0;
                    if (below) corner_disp(b, gX, y - 1, gZ, &bx, &bz, &d2);
                }
                int ty = low ? (is_hair ? y0 + VOX * 7 / 10 : y1) : y0 + drop;   // a wedge corner: hair drops 70%, the rest the full block (the GBA's slopes)
                setVector(&cc[k], X + dx, ty, Z + dz);
                setVector(&cc[4 + k], X + bx, y1, Z + bz);
            }
            if (n < W * D * H) { b->cell[n].x = (u8)x; b->cell[n].y = (u8)y; b->cell[n].z = (u8)z; b->cell[n].v = v; b->cell[n].hide = (u8)hide; b->cell[n].grp = (u8)grp;
                                 b->cell[n].flags = (u8)((is_hair ? 1 : 0) | (in ? 2 : 0)); memcpy(b->cell[n].c, cc, sizeof cc); n++; }
        }
    b->ncell = (u8)n;
    for (int y = 0; y < H; y++) for (int z = 0; z < D; z++) for (int x = 0; x < W; x++) {
        u16 code = b->dec[y][z][x];
        if (!code) continue;
        if (((code >> 3) & 7) || ((code >> 6) & 3)) continue;
        if (nd < 8) b->decc[nd++] = (u8)(x + z * W + y * W * D);
    }
    b->ndec = (u8)nd;
}

void voxel_draw(const Bore *b, const VECTOR *pos, int yaw, const Gait *g) {
    quads = 0; cur = b;
    MATRIX obj; SVECTOR rot = { 0, (short)yaw, 0, 0 }; VECTOR p = *pos;
    walk_sin = (isin(g->phase) * g->amp) >> 12;                       // -4096..4096
    int bob = g->amp ? (ABS(isin(g->phase * 2)) * 5 * g->amp) >> 24 : 0;
    p.vy -= bob;
    RotMatrix(&rot, &obj);
    TransMatrix(&obj, &p);
    if (g->sleep) {   // lying on the back
        MATRIX r; SVECTOR rr = { -1024, 0, 0, 0 }; VECTOR t = { 0, -40, -60 };
        RotMatrix(&rr, &r); TransMatrix(&r, &t);
        CompMatrixLV(&obj, &r, &obj);
    }
    int swing = (walk_sin * 300) >> 12;
    int hip = -b->legs * VOX, shoulder = -(b->legs + b->torso) * VOX;
    int legL = g->sit ? -1024 : swing, legR = g->sit ? -1024 : -swing;
    int armL = g->sit ? -200 : -swing * 3 / 4, armR = g->sit ? -200 : swing * 3 / 4;
    if (g->wave) { int w = isin((g->wave * 400) & 4095) >> 4; armR = -2400 + w; }
    gm[G_BODY] = obj;
    {   // the eye in the body's space: turn the eye offset back by the yaw
        int dx = cam.eye.vx - p.vx, dy = cam.eye.vy - p.vy, dz = cam.eye.vz - p.vz;
        int c = icos(yaw), sn = isin(yaw);
        cam_lx = (dx * c - dz * sn) >> 12; cam_lz = (dz * c + dx * sn) >> 12; cam_ly = dy;
        cam_cull = !g->sleep;
    }
    { int t0 = VSync(1);
    part_matrix(&gm[G_LLEG], &obj, legL, hip, 0);
    part_matrix(&gm[G_RLEG], &obj, legR, hip, 0);
    part_matrix(&gm[G_LARM], &obj, armL, shoulder, 0);
    part_matrix(&gm[G_RARM], &obj, armR, shoulder, 0);
    vox_t_face = VSync(1) - t0; }
    gfx_set_light_yaw(yaw);

    // the head and the hair mass
    hs_ = b->headsz; hx_ = b->bx0 + (b->bxw - 2 * hs_) / 2; hy_ = b->legs + b->torso; hz_ = (b->bxd - 2 * hs_) / 2;
    if (b->look[LK_SHAPE] == SH_HUNCH && hz_ + 2 * hs_ < b->bxd) hz_++;
    hair_cx = (hx_ + hs_ - 3) * VOX; hair_cy = -(hy_ + hs_) * VOX - VOX / 3; hair_cz = (hz_ + hs_ - 2) * VOX;
    hair_style = b->look[LK_HSTYLE];
    hair_curly = (hair_style == 5 || hair_style == 14);
    if (++hl_gen == 0) { memset(hl_stamp, 0, sizeof hl_stamp); memset(bl_stamp, 0, sizeof bl_stamp); hl_gen = 1; }
    for (int sl = 1; sl < 9; sl++) slot_lut_fill(sl, b->col[sl], kind_of(sl) != 0);
    radial_table();
    lights_for_group();

    int grp = -1;
    int tcells0 = VSync(1);
    for (int i = 0; i < b->ncell; i++) {
        const u8 *c = (const u8 *)&b->cell[i];
        int x = c[0], y = c[1], z = c[2], v = c[3], hide = c[4], fl = c[6];
        if (c[5] != grp) { grp = c[5]; gfx_set_object_m(&gm[grp]); if (grp != G_BODY) cam_cull = 0; }
        int slot = v & 15, sh = v >> 4, m = (sh >= 4 && sh <= 11) ? wMask2[sh - 4] : 0;
        if (fl & 1) { hair_block(b, x, y, z, m, hide, b->col[slot], b->cell[i].c); continue; }
        block(b->cell[i].c, m, hide, slot, b->col[slot], kind_of(slot), (fl & 2) ? -1 : x, z);
    }
    gfx_set_object_m(&gm[G_BODY]);
    gfx_set_zbias(6);                                            // the face art and the bangs lie on the blocks: pull them in front
    vox_t_block = VSync(1) - tcells0;
    { int t0 = VSync(1); face_sprites(b);
    if (hair_style != 3) hair_flaps(b); vox_t_hair = VSync(1) - t0; }
    gfx_set_zbias(0);
    extra_parts(b, hx_, hy_, hz_, hs_);
}
