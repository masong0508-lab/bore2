// voxel.c - a BORE person's 6x4x8 block model, drawn as real 3D with the GTE.
// Cubes, sloped wedge blocks and thinner limb blocks; textured eyes and mouth on the front; legs and arms that swing as
// 3D parts. HAIR gets its own treatment: the hull is rounded (outer corners pulled in and down), strands run on
// continuously across blocks, a sheen sits under the crown, the crown is a whorl, hanging cuts end in points, bangs hang
// over the forehead, and it is lit per vertex as a rounded mass instead of a pile of bricks.
#include "voxel.h"
#include "gfx.h"
#include "atlas.h"
#include "hairtex.inc"
#include "wear_models.inc"

static TexRef tx_side, tx_crown, tx_tips, tx_fringe, tx_curl, skin_tex, cloth_tex;
static TexRef pat_tex[NPAT][2];          // the clothes patterns, darker / lighter (pattern 0 = plain cloth)
static int cur_toppat, cur_botpat;
static inline u16 wear_pal(int i) { return wearPal[i % NX_COL]; }
static u8 wear_gen;        // the patterns this person wears, as pat_tex indexes (-1 = plain)
static int quads;
int vox_t_hair, vox_t_block, vox_t_face, vox_n_hair, vox_n_block, vox_n_rad;   // (debug counters, kept for the HUD readout)
static const u8 wMask2[8] = { 10, 12, 5, 3, 8, 4, 1, 2 };   // BORE: bit k = corner (k&1 ? +x : -x, k&2 ? +z : -z) drops a full block

enum { G_BODY, G_LLEG, G_RLEG, G_LARM, G_RARM, G_HEAD, G_N };   // G_HEAD: the head and its hair, on its own neck joint (tilts, nods)
static MATRIX gm[G_N];
static const Bore *cur;
static int sld(const Bore *b, int id);
static void head_pt(const Bore *b, SVECTOR *p);
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
    // the patterns: the same weave, with the pattern's pixels a shade darker or lighter
    for (int p = 1; p < NPAT; p++) for (int t = 0; t < 2; t++) {
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
            int v = 118 + (((x + y) & 1) ? 10 : -6) + (tr() % 7) - 3;
            if (patMask[p][y] >> x & 1) v = t ? v * 13 / 8 : v * 9 / 16;
            v = CLAMP(v, 0, 255) >> 3; tbuf[y * 16 + x] = RGB(v, v, v);
        }
        pat_tex[p][t] = atlas_add(16, 16, tbuf);
    }
   
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
static u32 slot_lut[24][64];               // 1..8 the person's colour slots, 9..22 the accessory colours (wear_col)
static u8  slot_lut_ok[24];
static const Bore *lut_bore;
// filled lazily: a slot's shade is worked out (lit, then graded for the fast path) the first time a face asks for it this draw
static u8 lut_stamp[24][64], lut_gen = 1;
static u16 lut_c15[24]; static u8 lut_tx[24];
static void slot_lut_fill(int slot, u16 col, int textured) { lut_c15[slot] = col; lut_tx[slot] = (u8)textured; memset(lut_stamp[slot], 0, 64); slot_lut_ok[slot] = 1; }
static void lut_next_person(void) { if (++lut_gen == 0) { memset(lut_stamp, 0, sizeof lut_stamp); lut_gen = 1; } }
static u32 slot_col_make(int slot, int i) {
    int L = i * 4 + 2; u16 col = lut_c15[slot];
    u32 c = lut_tx[slot] ? gfx_grade_tex(lit_tex(col, L)) : gfx_grade_flat(lit(col, L));
    slot_lut[slot][i] = c; lut_stamp[slot][i] = lut_gen;
    return c;
}
static inline u32 slot_col(int slot, int L) { int i = CLAMP(L, 0, 255) >> 2; return lut_stamp[slot][i] == lut_gen ? slot_lut[slot][i] : slot_col_make(slot, i); }
static void face4(const XV *a, const XV *b, const XV *c, const XV *d, int slot, int L0, int L1, int L2, int L3, int kind) {

    if (kind >= 2) {
        const TexRef *t = kind == 2 ? &skin_tex : kind == 3 ? &cloth_tex : &pat_tex[0][0] + (kind - 4);   // 4.. = a pattern (pat_tex[p][tone])
        quads += gfx_emit_gt4(a, b, c, d, slot_col(slot, L0), slot_col(slot, L1), slot_col(slot, L2), slot_col(slot, L3),
                              t->tpage, 0, t->u, t->v, t->u + 15, t->v + 15, 1);
    } else {
        quads += gfx_emit_g4(a, b, c, d, slot_col(slot, L0), slot_col(slot, L1), slot_col(slot, L2), slot_col(slot, L3), 1);
    }
}
static void tri(const XV *a, const XV *b, const XV *c, u16 col, int nx, int ny, int nz) {
    int L = gfx_light(nx, ny, nz);
    quads += gfx_emit_f3(a, b, c, gfx_grade_flat(lit(col, L)), 1);
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

static int kind_of(int slot) {   // 0 flat, 2 skin, 3 cloth, 4+ a patterned cloth (index into pat_tex as a flat array)
    if (slot == 1) return 2;
    if (slot == 6) return cur_toppat >= 0 ? 4 + cur_toppat : 3;
    if (slot == 7) return cur_botpat >= 0 ? 4 + cur_botpat : 3;
    return 0;
}

// ---- a plain block (skin, clothes, hats, parts) ----
// c = its 8 corners (top 4 then bottom 4, index k = (x?1:0)|(z?2:0)); m = wedge mask; hide = bit per face (0 -z, 1 +z, 2 -x, 3 +x, 4 top, 5 bottom)
// gx/gz = the grid cell for the rounded light (-1: light the faces flat, -2: a limb, round about its own axis)
static int Ldiag[4];           // a limb's rounded light: each vertical edge lit along its diagonal, so arms and legs shade like cylinders
static int Lf6[6];             // the light on the six axis faces of the current group: -z +z -x +x top bottom
static u8 Lrad[D + 1][W + 1];  // the radial (rounded body) light at each x,z grid point, this person, this frame
static void block(const SVECTOR *c, int m, int hide, int slot, u16 col, int kind, int gx, int gz) {
    hide |= cull_bits(c[0].vx + 4, c[3].vx - 4, c[0].vy, c[4].vy, c[0].vz + 4, c[3].vz - 4) & (m ? 0x0f : 0x3f);
    if ((hide & 0x3f) == 0x3f) return;
    XV xv[8]; gfx_xform(c, xv, 8);                               // the 8 corners once; the faces share them
    const XV *t = xv, *bt = xv + 4;
    int Lr[4];
    for (int k = 0; k < 4; k++) Lr[k] = (gx >= 0) ? Lrad[gz + (k >> 1)][gx + (k & 1)] : (gx == -2) ? Ldiag[k] : -1;
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
    if (!(hide & 32)) quads += gfx_emit_f4(&bt[3], &bt[2], &bt[1], &bt[0], slot_col(slot, Lf6[5]), 1);
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
    for (int k = 0; k < 4; k++) Ldiag[k] = CLAMP(gfx_light((k & 1) ? 2896 : -2896, -400, (k & 2) ? 2896 : -2896), 0, 255);
}
static short radN[D + 1][W + 1][2]; static u8 radN_ok;   // the normals never change: worked out once
static void radial_table(void) {       // the rounded body light at every x,z grid point
    if (!radN_ok) {
        for (int gz = 0; gz <= D; gz++) for (int gx = 0; gx <= W; gx++) {
            int vx = gx * VOX - 3 * VOX, vz = gz * VOX - 2 * VOX;
            int len = SquareRoot0(vx * vx + vz * vz); if (len < 8) len = 8;
            radN[gz][gx][0] = (short)((vx << 12) / len); radN[gz][gx][1] = (short)((vz << 12) / len);
        }
        radN_ok = 1;
    }
    for (int gz = 0; gz <= D; gz++) for (int gx = 0; gx <= W; gx++)
        Lrad[gz][gx] = (u8)CLAMP(gfx_light(radN[gz][gx][0], -600, radN[gz][gx][1]), 0, 255);
}

// ================================================================== HAIR
#define HK   7      // how far an exposed hair corner is pulled in (units)
#define HK2  5      // how far it is pulled down per missing neighbour

// the displacement of the TOP corners of layer y at grid point (X, Z): every exposed corner of the hull is pulled in, so the
// body reads as a rounded shape instead of a pile of bricks; hair corners also drop where the cut is exposed from above.
// Shared corners are worked out from the same neighbourhood by every block that touches them: no cracks.
#define RK   8      // how far an exposed corner is pulled in (units)
#define TK   7      // how far an exposed TOP corner of skin / clothes is pulled down per missing neighbour (a soft, pillowy top: heads, shoulders)
#define HK2  5      // how far a hair corner is pulled down per missing neighbour
static int is_hull(int v) { return v && (v >> 4) != 1 && (v >> 4) != 2 && (v >> 4) != 3; }
// the top rounding of a grid point on layer y: lower it where the hull's top edge is exposed (nothing solid above any of its 4 cells)
static int top_round(const Bore *b, int X, int y, int Z) {
    int s = 0;
    for (int k = 0; k < 4; k++) {
        int cx = (k & 1) ? X : X - 1, cz = (k & 2) ? Z : Z - 1;
        if (is_hull(solid_at(b, cx, y + 1, cz))) return 0;
        if (is_hull(solid_at(b, cx, y, cz))) s++;
    }
    return s == 1 ? 2 * TK : s == 2 ? TK : 0;
}
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
static void hfacex(const XV *a, const XV *b, const XV *c, const XV *d, int la, int lb, int lc, int ld, const TexRef *t,   // (the fast path: corners already transformed)
                   int u0, int v0, int u1, int v1, u16 col, int Lf, int var) {
    u32 c0 = gfx_grade_tex(hair_col(col, (la * 3 + Lf) >> 2, var)), c1 = gfx_grade_tex(hair_col(col, (lb * 3 + Lf) >> 2, var));
    u32 c2 = gfx_grade_tex(hair_col(col, (lc * 3 + Lf) >> 2, var - 6)), c3 = gfx_grade_tex(hair_col(col, (ld * 3 + Lf) >> 2, var - 6));
    quads += gfx_emit_gt4(a, b, c, d, c0, c1, c2, c3, t->tpage, 0, t->u + u0, t->v + v0, t->u + u1, t->v + v1, 1);
}

// a hair block at grid cell (x,y,z); c = its 8 corners from the cache (the rounded hull and the drops folded in)
static void hair_block(const Bore *b, int x, int y, int z, int m, int hide, u16 col, const SVECTOR *c) {
    int x0 = (x - 3) * VOX, y0 = -(y + 1) * VOX, z0 = (z - 2) * VOX;
    hide |= cull_bits(x0 + 8, x0 + VOX - 8, y0 + 16, y0 + VOX - 8, z0 + 8, z0 + VOX - 8) & 0x2f;   // (rounded: a margin; the top always tilts a little)
    if ((hide & 0x3f) == 0x3f) return;
    XV xv[8]; gfx_xform(c, xv, 8);
    const XV *t = xv, *bt = xv + 4;
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
        hfacex(&t[2], &t[3], &t[0], &t[1], lt[2], lt[3], lt[0], lt[1], tt, u0, v0, u1, v1, col, (lx | lz) ? gfx_light(lx, -ONE, lz) : Lf6[4], var);
    }
    if (!(hide & 2)) hfacex(&t[3], &t[2], &bt[3], &bt[2], lt[3], lt[2], lb[3], lb[2], side, 0, sv0, 15, sv1, col, Lf6[1], var);      // front +z
    if (!(hide & 1)) hfacex(&t[0], &t[1], &bt[0], &bt[1], lt[0], lt[1], lb[0], lb[1], side, 0, sv0, 15, sv1, col, Lf6[0], var);     // back -z
    if (!(hide & 4)) hfacex(&t[2], &t[0], &bt[2], &bt[0], lt[2], lt[0], lb[2], lb[0], side, 0, sv0, 15, sv1, col, Lf6[2], var);     // -x
    if (!(hide & 8)) hfacex(&t[1], &t[3], &bt[1], &bt[3], lt[1], lt[3], lb[1], lb[3], side, 0, sv0, 15, sv1, col, Lf6[3], var);      // +x
    if (!(hide & 32)) quads += gfx_emit_f4(&bt[3], &bt[2], &bt[1], &bt[0], gfx_grade_flat(lit(col, Lf6[5] - 20)), 1);
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
        head_pt(b, &a); head_pt(b, &bb); head_pt(b, &c); head_pt(b, &d);
        { int L = hair_corner_light(hx_ + hs_, hy_ + hh, hz_ + hd); hface(&a, &bb, &c, &d, L, L, L - 10, L - 10, &tx_fringe, 0, 0, 15, 15, col, 0, 0, ONE, 0); }
    }
    if (temples) {
        int Z0 = (hz_ - 2) * VOX + 2, Z1 = Z0 + hd * VOX - 4, Y1 = top + VOX * 14 / 16;
        for (int side = 0; side < 2; side++) {
            int X = side ? (hx_ + hw - 3) * VOX + 4 : (hx_ - 3) * VOX - 4;
            SVECTOR a, bb, c, d;
            if (side) { setVector(&a, X, top - 2, Z0); setVector(&bb, X, top - 2, Z1); setVector(&c, X + 2, Y1, Z0); setVector(&d, X + 2, Y1, Z1); }
            else      { setVector(&a, X, top - 2, Z1); setVector(&bb, X, top - 2, Z0); setVector(&c, X - 2, Y1, Z1); setVector(&d, X - 2, Y1, Z0); }
            head_pt(b, &a); head_pt(b, &bb); head_pt(b, &c); head_pt(b, &d);
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

// ---- the head's sculpt: everything drawn on or around the head goes through this (the blocks did at build time) ----
static void head_xf(const Bore *b, int *x, int *y, int *z) {
    int span = MAX(1, b->h_cy0 - b->h_top);                              // the head's height (y is down: the chin's y is the bigger)
    int t = CLAMP(((b->h_cy0 - *y) << 12) / span, 0, 4096);             // 0 the chin .. 4096 the crown
    int jx = 4096 - ((b->h_jaw * MAX(0, 2458 - t)) >> 12);              // the jaw: narrower over the bottom 60%
    int k = b->h_k;
    *x = b->h_cx + (((((*x - b->h_cx) * k) >> 12) * jx) >> 12);
    *z = b->h_cz + (((*z - b->h_cz) * k) >> 12);
    *y = b->h_cy0 + (((*y - b->h_cy0) * k) >> 12) - b->h_lift;
}
static void head_pt(const Bore *b, SVECTOR *p) { int x = p->vx, y = p->vy, z = p->vz; head_xf(b, &x, &y, &z); p->vx = (s16)x; p->vy = (s16)y; p->vz = (s16)z; }
static void head_box_xf(const Bore *b, int *x0, int *x1, int *y0, int *y1, int *z0, int *z1) {   // a box near the head: its centre moves, its size scales
    int cx = (*x0 + *x1) / 2, cy = (*y0 + *y1) / 2, cz = (*z0 + *z1) / 2, hw = ((*x1 - *x0) * b->h_k) >> 13, hh = ((*y1 - *y0) * b->h_k) >> 13, hd = ((*z1 - *z0) * b->h_k) >> 13;
    head_xf(b, &cx, &cy, &cz);
    *x0 = cx - hw; *x1 = cx + hw; *y0 = cy - hh; *y1 = cy + hh; *z0 = cz - hd; *z1 = cz + hd;
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
        head_pt(b, &a); head_pt(b, &bb); head_pt(b, &c); head_pt(b, &d);
        int u0 = t->u, u1 = t->u + t->w - 1;
        if (fl) { int tmp = u0; u0 = u1; u1 = tmp; }
        int L = gfx_light(0, 0, ONE);
        quads += gfx_quad_ft(&a, &bb, &c, &d, t->tpage, 0, u0, t->v, u1, t->v, u0, t->v + 7, u1, t->v + 7, CLAMP(L, 70, 140), 1, 0);
    }
}

// ---- the sprite parts BORE draws over the blocks: ears, horns, a tail, wings ----
static void extra_parts(const Bore *b, int hx, int hy, int hz, int hs) {
    int hw = 2 * hs;
    gfx_set_object_m(&gm[G_HEAD]);                               // ears and horns ride on the head
    int ears = b->look[LK_EARS];
    if (ears) {
        int s = ears == 1 ? 14 : 22, yc = -(hy + 2 * hs) * VOX + VOX * 5 / 4;
        int zc = (hz - 2) * VOX + hs * VOX;
        for (int side = 0; side < 2; side++) {
            int xa = side ? (hx + hw - 3) * VOX : (hx - 3) * VOX;
            int x0 = side ? xa : xa - s, x1 = side ? xa + s : xa, y0 = yc - s, y1 = yc + s, z0 = zc - s / 2, z1 = zc + s / 2;
            head_box_xf(b, &x0, &x1, &y0, &y1, &z0, &z1);
            box(x0, x1, y0, y1, z0, z1, 0, 1, b->col[1], 2);
        }
    }
    int horns = b->look[LK_HORNS];
    if (horns) {
        int hk = 4096 + sld(b, LK_HORNSZ) * 450;                        // HORN SIZE
        int s = ((horns == 1 ? 12 : 18) * hk) >> 12, h = ((horns == 1 ? 16 : 40) * hk) >> 12;
        int yt = -(hy + 2 * hs) * VOX;
        int zc = (hz - 2) * VOX + hs * VOX;
        for (int side = 0; side < 2; side++) {
            int xc = side ? (hx + hw - 3) * VOX - s : (hx - 3) * VOX + s;
            int x0 = xc - s, x1 = xc + s, y0 = yt - h, y1 = yt, z0 = zc - s, z1 = zc + s;
            head_box_xf(b, &x0, &x1, &y0, &y1, &z0, &z1);
            box(x0, x1, y0, y1, z0, z1, side ? 5 : 10, 8, b->col[8], 0);
            if (horns == 2) { int hh2 = ((h + 24) * b->h_k) >> 12; box(x0 + 4, x1 - 4, y0 - (hh2 - ((h * b->h_k) >> 12)), y0, z0 + 4, z1 - 4, side ? 5 : 10, 8, b->col[8], 0); }
        }
    }
    gfx_set_object_m(&gm[G_BODY]);
    int tail = b->look[LK_TAIL], ftail = b->look[LK_FTAIL];
    if (tail || ftail) {
        int n = CLAMP((tail == 2 ? 5 : 3) + sld(b, LK_TAILLEN) / 2 + (sld(b, LK_TAILLEN) > 0), 2, 8), s = ftail ? 14 : 9;   // TAIL LENGTH
        u16 col = ftail ? b->col[5] : b->col[1];
        int yc = b->w_hipy - 10, zc = -2 * VOX - s;
        for (int i = 0; i < n; i++) {
            int sw = (walk_sin * (i + 1)) >> 14;
            { int sl = (ftail == 1 && i == n - 1) ? 2 : (ftail ? 5 : 1); box(-s + sw, s + sw, yc - s, yc + s, zc - s, zc + s, 0, sl, b->col[sl], ftail ? 0 : 2); }
            zc -= s * 2 - 2; yc -= (tail == 2 || ftail == 2) ? 8 : -6;
        }
    }
    int back = b->look[LK_BACK];
    if (back == 2) {
        int yt = b->w_shy, flap = isin(walk_sin >> 1) >> 6, wk = 4096 + sld(b, LK_WINGSZ) * 500;   // WING SIZE
        for (int side = 0; side < 2; side++) {
            int sx = side ? 1 : -1;
            SVECTOR a, bb, c, d;
            setVector(&a, sx * 30, yt - 20, -2 * VOX - 4); setVector(&bb, sx * (30 + ((110 * wk) >> 12)), yt - ((90 * wk) >> 12) + flap, -2 * VOX - 30);
            setVector(&c, sx * 30, yt + 40, -2 * VOX - 4); setVector(&d, sx * (30 + ((120 * wk) >> 12)), yt + 10 + flap, -2 * VOX - 30);
            u32 cc = lit(b->col[5], gfx_light(0, 0, -ONE));
            quads += gfx_quad_f(&a, &bb, &c, &d, cc & 255, (cc >> 8) & 255, (cc >> 16) & 255, 0);
        }
    }
    if (back == 1) {   // SPIKES: a ridge of plates down the spine, the middle ones tallest
        int n = b->torso * 2 + 1, y0 = b->w_shy + 10, step = (b->w_hipy - b->w_shy - 10) / MAX(1, n - 1);
        for (int i = 0; i < n; i++) {
            int mid = n / 2 - ABS(i - n / 2), h = 14 + mid * 8, y = y0 + i * step, z = b->w_tbz;
            box(-5, 5, y - 8, y + 8, z - h, z, i & 1 ? 5 : 10, 8, b->col[8], 0);
        }
    }
    int ant = b->look[LK_ANTENNA];
    gfx_set_object_m(&gm[G_HEAD]);
    if (ant) {   // ANTENNAE: two stalks from the top of the head, a ball on each (the bigger kind: longer, bigger balls)
        int len = (ant == 1 ? 46 : 70) + sld(b, LK_ANTLEN) * 9, spr = hs * VOX / 2 + sld(b, LK_ANTSPR) * 6;
        int yt = b->w_heady, zc = (hz - 2) * VOX + hs * VOX, xc = (hx + hs - 3) * VOX, r = ant == 1 ? 8 : 11;
        for (int side = -1; side <= 1; side += 2) {
            int x = xc + side * spr, lean = side * len / 5, sway = walk_sin >> 10;
            box(x - 3, x + 3, yt - len / 2, yt, zc - 3, zc + 3, 0, 3, b->col[3], 0);
            box(x + lean / 2 - 3 + sway, x + lean / 2 + 3 + sway, yt - len, yt - len / 2, zc - 3, zc + 3, 0, 3, b->col[3], 0);
            int bx = x + lean / 2 + sway;
            box(bx - r, bx + r, yt - len - 2 * r, yt - len, zc - r, zc + r, 0, ant == 1 ? 4 : 8, b->col[ant == 1 ? 4 : 8], 0);
        }
    }
}


// ================================================================== WEAR (the creator's accessories: wear_models.inc)
static u8 wear_lut_gen[24];
static int wear_slot(const Bore *b, int c) {   // colour code -> a colour slot whose lit shades are worked out (once per person per frame)
    int sl = 9 + c;
    if (wear_lut_gen[sl] != wear_gen) {
        u16 col = c == 0 ? wear_pal(b->look[LK_X_COLA]) : c == 1 ? wear_pal(b->look[LK_X_COLB]) : c == 10 ? b->col[5] : c == 11 ? b->col[1] : wearFixed[c];
        slot_lut_fill(sl, col, 0); wear_lut_gen[sl] = wear_gen;
    }
    return sl;
}
static void wear_part(const Bore *b, const WearModel *m, int ax, int ay, int az, int u, int zdir) {
    for (int i = 0; i < m->n; i++) {
        const s8 *q = m->b[i];
        int x0 = ax + q[0] * u, x1 = ax + q[1] * u, y0 = ay - q[3] * u, y1 = ay - q[2] * u, z0, z1;
        if (zdir > 0) { z0 = az + q[4] * u; z1 = az + q[5] * u; } else { z0 = az - q[5] * u; z1 = az - q[4] * u; }
        int sl = wear_slot(b, q[6]);
        box(x0, x1, y0, y1, z0, z1, 0, sl, 0, 0);
    }
}
static void wear_draw(const Bore *b) {
    int hu = MAX(3, b->w_headh / 8), bu = VOX / 8;            // head units (the head is 16 wide), body units
    int v;
    gfx_set_object_m(&gm[G_HEAD]);
    if ((v = b->look[LK_X_HEAD] % NX_HEAD)) wear_part(b, &wearHead[v], b->w_headx, b->w_heady + VOX / 4, b->w_headz, hu, 1);   // (sunk a little into the hair)
    if ((v = b->look[LK_X_FACE] % NX_FACE)) { gfx_set_zbias(4); wear_part(b, &wearFace[v], b->w_headx, b->w_facey, b->w_facez, hu, 1); gfx_set_zbias(0); }
    gfx_set_object_m(&gm[G_BODY]);
    if ((v = b->look[LK_X_NECK] % NX_NECK)) wear_part(b, &wearNeck[v], b->w_tx, b->w_shy, b->w_tfz, bu, 1);
    if ((v = b->look[LK_X_BACK] % NX_BACK)) wear_part(b, &wearBack[v], b->w_tx, b->w_shy, b->w_tbz, bu, -1);
    if ((v = b->look[LK_X_WAIST] % NX_WAIST)) wear_part(b, &wearWaist[v], b->w_tx, b->w_hipy, b->w_tfz, bu, 1);
    if ((v = b->look[LK_X_HAND] % NX_HAND)) { gfx_set_object_m(&gm[G_RARM]); wear_part(b, &wearHand[v], b->w_hx, b->w_hy, b->w_hz, bu, 1); gfx_set_object_m(&gm[G_BODY]); }
}

static void joint_matrix(MATRIX *out, const MATRIX *obj, int pitch, int roll, int pivot_x, int pivot_y, int pivot_z) {   // a joint that swings (pitch, about x) and spreads (roll, about z)
    MATRIX r; SVECTOR rot = { (short)pitch, 0, (short)roll, 0 }; VECTOR p, rp, t;
    RotMatrix(&rot, &r);
    setVector(&p, pivot_x, pivot_y, pivot_z);
    ApplyMatrixLV(&r, &p, &rp);
    setVector(&t, p.vx - rp.vx, p.vy - rp.vy, p.vz - rp.vz);
    TransMatrix(&r, &t);
    CompMatrixLV((MATRIX *)obj, &r, out);
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

// ================================================================== BODY SHAPE
// The voxel grid gives every body the same straight-sided blocks. Here the corners are bent once, when the person is
// built: the torso tapers from the shoulders to a narrower waist, arms and legs taper towards the wrists and ankles,
// hands are a touch smaller and feet reach forward. Then the creator's body sliders (shoulders, chest, belly, waist, hips,
// legs, thighs, calves, feet, arm length and widths, hands, weight) push and pull the same corners.
static int sld(const Bore *b, int id) { return (b->look[id] + 4) % 9 - 4; }   // a slider: -4..4
static void head_box(const Bore *b, int *hx, int *hy, int *hz, int *hs) {
    *hs = b->headsz; *hx = b->bx0 + (b->bxw - 2 * *hs) / 2; *hy = b->legs + b->torso; *hz = (b->bxd - 2 * *hs) / 2;
    if (b->look[LK_SHAPE] == SH_HUNCH && *hz + 2 * *hs < b->bxd) (*hz)++;
}
static inline int scl(int v, int c, int k) { return c + (((v - c) * k) >> 12); }
static inline int lerp12(int a, int b, int t) { return a + (((b - a) * t) >> 12); }   // t 0..4096

static void shape_body(Bore *b) {
    int hx, hy, hz, hs; head_box(b, &hx, &hy, &hz, &hs);
    int baby = b->stage == AG_BABY, kid = b->stage <= AG_CHILD;
    int base = baby ? 0 : kid ? 1 : 2;                   // how strongly the adult shaping applies (babies stay round and simple)
    // ---- the proportions: longer legs, a smaller head with a jaw, lifted onto a neck ----
    int ks = 4096 + 160 * base;                                              // the legs stretch (everything above rides up with them)
    int hipY0 = -b->legs * VOX, hipY = (hipY0 * ks) >> 12, d = hipY - hipY0;   // d: how far up the hips moved (negative = up)
    int shY = -hy * VOX + d, legH = MAX(1, -hipY), torH = MAX(1, b->torso * VOX);
    b->h_cx = (s16)((hx + hs - 3) * VOX); b->h_cz = (s16)((hz + hs - 2) * VOX);
    b->h_cy0 = (s16)shY; b->h_top = (s16)(-(hy + 2 * hs) * VOX + d);
    b->h_k = (s16)CLAMP(4096 - 240 * base + sld(b, LK_HEADSZ) * 120, 3000, 5000);         // adults 88%, kids 94%, babies as built
    b->h_jaw = (s16)CLAMP(220 * base - sld(b, LK_JAWW) * 110, -600, 1200);                // the chin narrower than the brow
    b->h_lift = (s16)MAX(0, 5 * base + sld(b, LK_NECK) * 3);                               // the neck's height
    b->h_neckw = (s16)MAX(10, VOX * 11 / 20 + sld(b, LK_NECKW) * 3);
    if (baby) { b->h_lift = 0; b->h_jaw = 0; }
    for (int i = 0; i < b->ncell; i++) {                  // the stretch: legs taller, the rest lifted
        int g = b->cell[i].grp; SVECTOR *cv = b->cell[i].c;
        for (int k = 0; k < 8; k++) {
            if (g == G_LLEG || g == G_RLEG) cv[k].vy = (s16)((cv[k].vy * ks) >> 12);
            else cv[k].vy = (s16)(cv[k].vy + d);
        }
    }
    int mnx[G_N], mxx[G_N], mnz[G_N], mxz[G_N], mxy[G_N];
    for (int g = 0; g < G_N; g++) { mnx[g] = mnz[g] = 32767; mxx[g] = mxz[g] = mxy[g] = -32767; }
    for (int i = 0; i < b->ncell; i++) {
        int g = b->cell[i].grp;
        if (g == G_BODY && (b->cell[i].y >= hy || (b->cell[i].flags & 1))) continue;   // the torso alone (no head, no hair)
        for (int k = 0; k < 8; k++) { const SVECTOR *c = &b->cell[i].c[k];
            mnx[g] = MIN(mnx[g], c->vx); mxx[g] = MAX(mxx[g], c->vx); mnz[g] = MIN(mnz[g], c->vz); mxz[g] = MAX(mxz[g], c->vz); mxy[g] = MAX(mxy[g], c->vy); }
    }
    int tcx = (mnx[G_BODY] + mxx[G_BODY]) / 2, tcz = (mnz[G_BODY] + mxz[G_BODY]) / 2, thw = (mxx[G_BODY] - mnx[G_BODY]) / 2;
    int wgt = sld(b, LK_WEIGHT), tor = sld(b, LK_TORSO), shw = sld(b, LK_SHOULW), chs = sld(b, LK_CHESTW), bel = sld(b, LK_BELLYW);
    int wai = sld(b, LK_WAISTW), hip = sld(b, LK_HIPW), lgw = sld(b, LK_LEGW), thi = sld(b, LK_THIGHW), clf = sld(b, LK_CALFW), ft = sld(b, LK_FOOTSZ);
    int arl = sld(b, LK_ARMS), arw = sld(b, LK_ARMW), uaw = sld(b, LK_UARMW), faw = sld(b, LK_FARMW), hnd = sld(b, LK_HANDSZ);
    // the torso's width at a height t (0 the hips .. 4096 the shoulders), x 4096
    #define TORSO_KX(t) ({ int t_ = (t), mid_ = 4096 - ABS(2 * t_ - 4096); \
        int bs_ = t_ < 2048 ? lerp12(4096 - 100 * base, 4096 - 230 * base, t_ * 2) : lerp12(4096 - 230 * base, 4096 + 40 * base, (t_ - 2048) * 2); \
        bs_ + wgt * 200 + tor * 160 + ((shw * 220 * t_) >> 12) + ((hip * 220 * (4096 - t_)) >> 12) + ((wai * 200 * mid_) >> 12); })
    int kx_sh = TORSO_KX(4096), kx_hip = TORSO_KX(0);
    int arm_shift = (thw * (kx_sh - 4096)) >> 12, leg_shift = (thw * (kx_hip - 4096)) >> 13;
    int acx[G_N], acz[G_N];
    for (int g = 0; g < G_N; g++) { acx[g] = (mnx[g] + mxx[g]) / 2; acz[g] = (mnz[g] + mxz[g]) / 2; }
    for (int i = 0; i < b->ncell; i++) {
        int g = b->cell[i].grp, sh = b->cell[i].v >> 4;
        SVECTOR *cv = b->cell[i].c;
        if (g == G_HEAD) { for (int k = 0; k < 8; k++) head_pt(b, &cv[k]); continue; }
        if (g == G_BODY) {
            if (b->cell[i].y >= hy || (b->cell[i].flags & 1)) {                // the head, the hair, a hat, a beard: the head's sculpt
                for (int k = 0; k < 8; k++) head_pt(b, &cv[k]);
                continue;
            }
            for (int k = 0; k < 8; k++) {
                int t = CLAMP(((hipY - cv[k].vy) << 12) / torH, 0, 4096), mid = 4096 - ABS(2 * t - 4096);
                int kx = TORSO_KX(t);
                int kzf = 4096 + 60 * base + wgt * 220 + ((chs * 260 * t) >> 12) + ((bel * 300 * mid) >> 12) + ((t * 120 * base) >> 12);   // the front: a chest
                int kzb = 4096 + wgt * 160 - ((t * 80 * base) >> 12);
                cv[k].vx = scl(cv[k].vx, tcx, MAX(kx, 2000));
                cv[k].vz = cv[k].vz > tcz ? scl(cv[k].vz, tcz, MAX(kzf, 2000)) : scl(cv[k].vz, tcz, MAX(kzb, 2000));
            }
        } else if (g == G_LARM || g == G_RARM) {
            int side = acx[g] > tcx ? 1 : -1, armLen = MAX(1, mxy[g] - shY);
            for (int k = 0; k < 8; k++) {
                int u = CLAMP(((cv[k].vy - shY) << 12) / armLen, 0, 4096);      // 0 the shoulder .. 4096 the hand
                int kw = 4096 - ((u * 170 * base) >> 12) + wgt * 170 + arw * 240 + (((4096 - u) * uaw * 280) >> 12) + ((u * faw * 280) >> 12);
                if (sh == 2) kw = 4096 - 150 * base + hnd * 380 + wgt * 80;     // the hand
                cv[k].vx = scl(cv[k].vx, acx[g], MAX(kw, 1800)) + side * arm_shift;
                cv[k].vz = scl(cv[k].vz, acz[g], MAX(kw, 1800));
                cv[k].vy = shY + (((cv[k].vy - shY) * (4096 + arl * 260)) >> 12);   // arm length, from the shoulder
            }
        } else {                                                                 // a leg
            int side = acx[g] > tcx ? 1 : -1;
            for (int k = 0; k < 8; k++) {
                int u = CLAMP(((cv[k].vy - hipY) << 12) / legH, 0, 4096);      // 0 the hip .. 4096 the sole
                int kw = 4096 - ((u * 200 * base) >> 12) + wgt * 190 + lgw * 240 + (((4096 - u) * thi * 300) >> 12) + ((u * clf * 300) >> 12);
                cv[k].vx = scl(cv[k].vx, acx[g], MAX(kw, 1800)) + side * leg_shift;
                cv[k].vz = scl(cv[k].vz, acz[g], MAX(kw, 1800));
                if (b->cell[i].y == 0 && cv[k].vz > acz[g]) cv[k].vz += 5 * base + ft * 4;   // the foot reaches forward
            }
        }
    }
    #undef TORSO_KX
    // ---- where the WEAR tab's accessories attach ----
    int top = 32767, tfz = -32767, tbz = 32767, hand_y = -32767, hx0 = 32767, hx1 = -32767, hz0 = 32767, hz1 = -32767;
    for (int i = 0; i < b->ncell; i++) {
        int g = b->cell[i].grp; const SVECTOR *cv = b->cell[i].c;
        for (int k = 0; k < 8; k++) {
            if (g == G_HEAD) top = MIN(top, cv[k].vy);
            if (g == G_BODY && b->cell[i].y < hy && !(b->cell[i].flags & 1)) { tfz = MAX(tfz, cv[k].vz); tbz = MIN(tbz, cv[k].vz); }
            if (g == G_RARM) hand_y = MAX(hand_y, cv[k].vy);
        }
    }
    for (int i = 0; i < b->ncell; i++) if (b->cell[i].grp == G_RARM) for (int k = 4; k < 8; k++) {
        const SVECTOR *cv = &b->cell[i].c[k];
        if (cv->vy >= hand_y - 2) { hx0 = MIN(hx0, cv->vx); hx1 = MAX(hx1, cv->vx); hz0 = MIN(hz0, cv->vz); hz1 = MAX(hz1, cv->vz); }
    }
    if (top == 32767) top = b->h_top;
    if (tfz == -32767) { tfz = VOX; tbz = -VOX; }
    b->w_headx = b->h_cx; b->w_heady = (s16)top; b->w_headz = b->h_cz; b->w_headh = (s16)((hs * VOX * b->h_k) >> 12);
    { int fx = b->h_cx, fy = -(hy + hs) * VOX - hs * VOX / 3 + d, fz = (hz + 2 * hs - 2) * VOX; head_xf(b, &fx, &fy, &fz); b->w_facey = (s16)fy; b->w_facez = (s16)fz; }
    b->w_tx = (s16)tcx; b->w_shy = (s16)shY; b->w_hipy = (s16)hipY; b->w_tfz = (s16)tfz; b->w_tbz = (s16)tbz;
    { int lx0 = 32767, lx1 = -32767, rx0 = 32767, rx1 = -32767, gx0 = 32767, gx1 = -32767, kx0 = 32767, kx1 = -32767;
      for (int i = 0; i < b->ncell; i++) for (int k = 0; k < 8; k++) { int g = b->cell[i].grp, x = b->cell[i].c[k].vx;
          if (g == G_LARM) { lx0 = MIN(lx0, x); lx1 = MAX(lx1, x); } if (g == G_RARM) { rx0 = MIN(rx0, x); rx1 = MAX(rx1, x); }
          if (g == G_LLEG) { gx0 = MIN(gx0, x); gx1 = MAX(gx1, x); } if (g == G_RLEG) { kx0 = MIN(kx0, x); kx1 = MAX(kx1, x); } }
      b->w_lax = (s16)(lx0 <= lx1 ? (lx0 + lx1) / 2 : -VOX * 2); b->w_rax = (s16)(rx0 <= rx1 ? (rx0 + rx1) / 2 : VOX * 2);
      b->w_lgx = (s16)(gx0 <= gx1 ? (gx0 + gx1) / 2 : -VOX / 2); b->w_rgx = (s16)(kx0 <= kx1 ? (kx0 + kx1) / 2 : VOX / 2); }
    if (hx0 <= hx1) { b->w_hx = (s16)((hx0 + hx1) / 2); b->w_hy = (s16)hand_y; b->w_hz = (s16)((hz0 + hz1) / 2); }
    else { b->w_hx = (s16)(2 * VOX); b->w_hy = (s16)hipY; b->w_hz = 0; }
}

void voxel_prepare(Bore *b) {
    int n = 0, nd = 0;
    for (int grp = 0; grp < G_N; grp++)
        for (int y = 0; y < H; y++) for (int z = 0; z < D; z++) for (int x = 0; x < W; x++) {
            u8 v = b->vox[y][z][x];
            if (!v) continue;
            { int g_ = group_of(v, x); if (g_ == G_BODY && y >= b->legs + b->torso) g_ = G_HEAD; if (g_ != grp) continue; }
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
            int is_hair = (slot == 5 && !in && (grp == G_BODY || grp == G_HEAD));
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
                    if (!is_hair) drop = top_round(b, gX, y, gZ);
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
    shape_body(b);
    if (b->h_lift > 0 || b->h_k < 4096) {                // the head sits smaller and higher than its blocks did: the shoulders and the chin show
        int hy = b->legs + b->torso;
        for (int i = 0; i < n; i++) {
            if (b->cell[i].grp != G_BODY && b->cell[i].grp != G_HEAD) continue;
            if (b->cell[i].y == hy) b->cell[i].hide &= ~32;
            else if (b->cell[i].y == hy - 1 && !(b->cell[i].flags & 1)) b->cell[i].hide &= ~16;
        }
    }
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
        { MATRIX t_ = obj; CompMatrixLV(&t_, &r, &obj); };
    }
    int swing = (walk_sin * 300) >> 12;
    int hip = b->w_hipy, shoulder = b->w_shy;
    int legL = g->sit ? -1024 : swing, legR = g->sit ? -1024 : -swing;
    int armL = g->sit ? -200 : -swing * 3 / 4, armR = g->sit ? -200 : swing * 3 / 4;
    // ---- the body language: stylised, a little cute ----
    int t = g->clock + g->seed * 37, moving = g->amp > 2000, idle = !moving && !g->sit && !g->sleep;
    int breath = isin((t * 22) & 4095);                                   // a slow breath, ~3 s
    int happy = g->mood > 45, sad = g->mood < -35, baby = b->stage <= AG_CHILD;
    int rollL = -150, rollR = 150;                                        // a relaxed A-pose: arms off the sides (never stiff at attention)
    int legRollL = 0, legRollR = 0, headTilt = 0, headNod = 0, lean = 0;
    if (idle) {
        rollL -= breath >> 8; rollR += breath >> 8;                       // the arms rise with the breath
        armL += (breath >> 9) - 60; armR += (breath >> 9) - 60;           // and hang a touch forward
        legRollL = -45; legRollR = 45;                                    // feet a little apart
        int look = isin((t * 7) & 4095);                                  // now and then a curious head tilt
        headTilt = look > 2800 ? (look - 2800) / 6 : look < -2800 ? (look + 2800) / 6 : 0;
        if (happy) {                                                      // happy: a little hop every few seconds, arms up with it
            int ph = (t * 3) & 1023;
            if (ph < 96) { obj.t[1] -= (isin(ph * 21) * (baby ? 22 : 16)) >> 12; rollL -= 220; rollR += 220; }
        }
        if (sad) { rollL += 100; rollR -= 100; headNod = 260; lean = 120; armL += 60; armR += 60; }   // sad: arms in, head down, a slouch
    }
    if (moving) { rollL -= 60; rollR += 60; armL = armL * 5 / 4; armR = armR * 5 / 4; lean = g->amp > 3800 ? 90 : 40;      // a stride leans in
                  headNod = -40 + (ABS(walk_sin) >> 8); }                                              // and the head bobs
    if (g->talk && !g->wave) {                                            // in a conversation: one hand talks, the head nods along
        int gst = isin((t * 60) & 4095);
        armR = -700 + (gst >> 4); rollR += 120;
        headNod = (isin((t * 90) & 4095) >> 7) - 30; headTilt += 60;
    }
    if (g->wave) { int w = isin((g->wave * 400) & 4095) >> 4; armR = -2400 + w; rollR = 260; }
    if (g->sit) { rollL = -60; rollR = 60; armL = -500; armR = -500; }   // sitting: hands on the knees
    if (lean) { MATRIX r; SVECTOR rr = { (short)lean, 0, 0, 0 }; RotMatrix(&rr, &r); r.t[0] = r.t[1] = r.t[2] = 0; { MATRIX t_ = obj; CompMatrixLV(&t_, &r, &obj); } }   // the lean pivots at the feet (RotMatrix leaves the translation unset)
    gm[G_BODY] = obj;
    {   // the eye in the body's space: turn the eye offset back by the yaw
        int dx = cam.eye.vx - p.vx, dy = cam.eye.vy - p.vy, dz = cam.eye.vz - p.vz;
        int c = icos(yaw), sn = isin(yaw);
        cam_lx = (dx * c - dz * sn) >> 12; cam_lz = (dz * c + dx * sn) >> 12; cam_ly = dy;
        cam_cull = !g->sleep;
    }
    { int t0 = gfx_hb();
    joint_matrix(&gm[G_LLEG], &obj, legL, legRollL, b->w_lgx, hip, 0);
    joint_matrix(&gm[G_RLEG], &obj, legR, legRollR, b->w_rgx, hip, 0);
    joint_matrix(&gm[G_LARM], &obj, armL, rollL, b->w_lax, shoulder, 0);
    joint_matrix(&gm[G_RARM], &obj, armR, rollR, b->w_rax, shoulder, 0);
    joint_matrix(&gm[G_HEAD], &obj, headNod, headTilt, b->h_cx, b->h_cy0 - b->h_lift, b->h_cz);   // the neck
    vox_t_face += gfx_hb() - t0; }
    gfx_set_light_yaw(yaw);
    int tprep0 = gfx_hb();
    // the head and the hair mass
    head_box(b, &hx_, &hy_, &hz_, &hs_);
    hair_cx = (hx_ + hs_ - 3) * VOX; hair_cy = -(hy_ + hs_) * VOX - VOX / 3; hair_cz = (hz_ + hs_ - 2) * VOX;
    hair_style = b->look[LK_HSTYLE];
    hair_curly = (hair_style == 5 || hair_style == 14);
    if (++hl_gen == 0) { memset(hl_stamp, 0, sizeof hl_stamp); memset(bl_stamp, 0, sizeof bl_stamp); hl_gen = 1; }
    { int tp = b->look[LK_X_TOPPAT] % NPAT, bp = b->look[LK_X_BOTPAT] % NPAT, tone = b->look[LK_X_PATTONE] & 1;
      cur_toppat = tp ? tp * 2 + tone : -1; cur_botpat = bp ? bp * 2 + tone : -1; }
    wear_gen++;
    { int q0 = gfx_hb(); lut_next_person(); for (int sl = 1; sl < 9; sl++) { lut_c15[sl] = b->col[sl]; lut_tx[sl] = kind_of(sl) != 0; } vox_n_block += gfx_hb() - q0; }
    { int q0 = gfx_hb(); radial_table(); vox_n_rad += gfx_hb() - q0; }
    lights_for_group();

    vox_n_hair += gfx_hb() - tprep0;   // (reused: the per-person set-up)
    int grp = -1, cull0 = cam_cull;
    int tcells0 = gfx_hb();
    for (int i = 0; i < b->ncell; i++) {
        const u8 *c = (const u8 *)&b->cell[i];
        int x = c[0], y = c[1], z = c[2], v = c[3], hide = c[4], fl = c[6];
        if (c[5] != grp) { grp = c[5]; gfx_set_object_m(&gm[grp]); cam_cull = (grp == G_BODY || grp == G_HEAD) ? cull0 : 0; }
        int slot = v & 15, sh = v >> 4, m = (sh >= 4 && sh <= 11) ? wMask2[sh - 4] : 0;
        if (fl & 1) { hair_block(b, x, y, z, m, hide, b->col[slot], b->cell[i].c); continue; }
        block(b->cell[i].c, m, hide, slot, b->col[slot], kind_of(slot), (fl & 2) ? -2 : x, z);
    }
    gfx_set_object_m(&gm[G_BODY]);
    if (b->h_lift > 0) {   // the neck: a skin column from the shoulders up into the head
        int nw = b->h_neckw, nd = nw * 9 / 10;
        box(b->h_cx - nw, b->h_cx + nw, b->h_cy0 - b->h_lift - 3, b->h_cy0 + 3, b->h_cz - nd, b->h_cz + nd, 0, 1, b->col[1], 2);
    }
    gfx_set_object_m(&gm[G_HEAD]);
    gfx_set_zbias(6);                                            // the face art and the bangs lie on the blocks: pull them in front
    vox_t_block += gfx_hb() - tcells0;
    { int t0 = gfx_hb(); face_sprites(b);
    if (hair_style != 3) hair_flaps(b); vox_t_hair += gfx_hb() - t0; }
    gfx_set_zbias(0);
    { PROF_BEGIN(); extra_parts(b, hx_, hy_, hz_, hs_);
    wear_draw(b); PROF_END(7); }
}


// ================================================================== CROWD MODEL (half resolution)
// The full 6x4x8 build is folded into 4x2x4 cells: the arm columns stay on their own (so the silhouette keeps its arms), the four
// inner columns pair up, depth and height pair up. A cell is solid when about a third of what it covers is; its colour is the slot
// most of it was. The cells keep their size in world units (arm columns one block wide, the rest two), so a crowd member is as tall
// and as wide as the full model and reads as the same person: same skin, hair, top, bottom, same walk.
void voxel_lod_build(const Bore *b, LodBore *m) {
    static const u8 xa[4] = { 0, 1, 3, 5 }, xb[4] = { 1, 3, 5, 6 };
    memcpy(m->col, b->col, sizeof m->col);
    m->legrows = (u8)CLAMP((b->legs + 1) / 2, 1, 3);
    m->headrow = (u8)CLAMP((b->legs + b->torso) / 2, 1, 3);
    for (int cy = 0; cy < 4; cy++) for (int cz = 0; cz < 2; cz++) for (int cx = 0; cx < 4; cx++) {
        int cnt[9] = { 0 }, n = 0, tot = 0;
        for (int y = cy * 2; y < cy * 2 + 2; y++) for (int z = cz * 2; z < cz * 2 + 2; z++) for (int x = xa[cx]; x < xb[cx]; x++) {
            int v = b->vox[y][z][x]; tot++;
            if (v) { int s = v & 15; if (s < 1) s = 1; if (s > 8) s = 8; cnt[s]++; n++; }
        }
        int best = 0, bn = 0;
        for (int s = 1; s <= 8; s++) if (cnt[s] >= bn && cnt[s]) { bn = cnt[s]; best = s; }
        m->c[cy][cz][cx] = (n * 3 >= tot && best) ? (u8)best : 0;
    }
}

static inline int lod_at(const LodBore *m, int x, int y, int z) {
    if (x < 0 || x > 3 || y < 0 || y > 3 || z < 0 || z > 1) return 0;
    return m->c[y][z][x];
}
// draw the cells of one body part (cx0..cx1 columns, cy0..cy1 rows) in the current object space
static void lod_cells(const LodBore *m, int cx0, int cx1, int cy0, int cy1, const u32 lc[9][6], int cull) {
    static const short xs[5] = { -3 * VOX, -2 * VOX, 0, 2 * VOX, 3 * VOX };
    for (int cy = cy0; cy <= cy1; cy++) for (int cz = 0; cz < 2; cz++) for (int cx = cx0; cx <= cx1; cx++) {
        int s = m->c[cy][cz][cx]; if (!s) continue;
        int X0 = xs[cx], X1 = xs[cx + 1], Y0 = -(cy * 2 + 2) * VOX, Y1 = -(cy * 2) * VOX, Z0 = (cz - 1) * 2 * VOX, Z1 = Z0 + 2 * VOX;
        int hb = 0;
        if (lod_at(m, cx, cy, cz - 1)) hb |= 1;  if (lod_at(m, cx, cy, cz + 1)) hb |= 2;
        if (lod_at(m, cx - 1, cy, cz)) hb |= 4;  if (lod_at(m, cx + 1, cy, cz)) hb |= 8;
        if (lod_at(m, cx, cy + 1, cz)) hb |= 16; if (cy == 0 || lod_at(m, cx, cy - 1, cz)) hb |= 32;
        if (cull) {
            if (cam_lz > Z0) hb |= 1;  if (cam_lz < Z1) hb |= 2;
            if (cam_lx > X0) hb |= 4;  if (cam_lx < X1) hb |= 8;
            if (cam_ly > Y0) hb |= 16; if (cam_ly < Y1) hb |= 32;
        }
        if ((hb & 0x3f) == 0x3f) continue;
        SVECTOR t[4], bt[4];
        for (int k = 0; k < 4; k++) {
            int X = (k & 1) ? X1 : X0, Z = (k & 2) ? Z1 : Z0;
            setVector(&t[k], X, Y0, Z); setVector(&bt[k], X, Y1, Z);
        }
        #define LQ(a, b, c_, d, f) { u32 cc = lc[s][f]; quads += gfx_quad_f(a, b, c_, d, cc & 255, (cc >> 8) & 255, (cc >> 16) & 255, 1); }
        if (!(hb & 16)) LQ(&t[2], &t[3], &t[0], &t[1], 4)
        if (!(hb & 2))  LQ(&t[3], &t[2], &bt[3], &bt[2], 1)    // front +z
        if (!(hb & 1))  LQ(&t[0], &t[1], &bt[0], &bt[1], 0)    // back -z
        if (!(hb & 4))  LQ(&t[2], &t[0], &bt[2], &bt[0], 2)    // -x
        if (!(hb & 8))  LQ(&t[1], &t[3], &bt[1], &bt[3], 3)    // +x
        if (!(hb & 32)) LQ(&bt[3], &bt[2], &bt[1], &bt[0], 5)
        #undef LQ
    }
}

void voxel_draw_lod(const LodBore *m, const VECTOR *pos, int yaw, const Gait *g) {
    quads = 0;
    MATRIX obj; SVECTOR rot = { 0, (short)yaw, 0, 0 }; VECTOR p = *pos;
    int ws = (isin(g->phase) * g->amp) >> 12;
    int bob = g->amp ? (ABS(isin(g->phase * 2)) * 5 * g->amp) >> 24 : 0;
    p.vy -= bob;
    RotMatrix(&rot, &obj);
    TransMatrix(&obj, &p);
    if (g->sleep) { MATRIX r; SVECTOR rr = { -1024, 0, 0, 0 }; VECTOR t = { 0, -40, -60 }; RotMatrix(&rr, &r); TransMatrix(&r, &t); { MATRIX t_ = obj; CompMatrixLV(&t_, &r, &obj); }; }
    int swing = (ws * 300) >> 12;
    int legL = g->sit ? -1024 : swing, legR = g->sit ? -1024 : -swing;
    int armL = g->sit ? -200 : -swing * 3 / 4, armR = g->sit ? -200 : swing * 3 / 4;
    { int dx = cam.eye.vx - p.vx, dy = cam.eye.vy - p.vy, dz = cam.eye.vz - p.vz, c = icos(yaw), sn = isin(yaw);
      cam_lx = (dx * c - dz * sn) >> 12; cam_lz = (dz * c + dx * sn) >> 12; cam_ly = dy; }
    int cull = !g->sleep;
    int lr = m->legrows, hipY = -lr * 2 * VOX;
    int armTop = -1; for (int cy = 3; cy >= 0 && armTop < 0; cy--) if (m->c[cy][0][0] || m->c[cy][1][0] || m->c[cy][0][3] || m->c[cy][1][3]) armTop = cy;
    int shY = armTop < 0 ? hipY : -(armTop * 2 + 2) * VOX + 24;
    MATRIX gmL[5];   // 0 body, 1 left leg, 2 right leg, 3 left arm, 4 right arm
    gmL[0] = obj;
    joint_matrix(&gmL[1], &obj, legL, 0, 0, hipY, 0);
    joint_matrix(&gmL[2], &obj, legR, 0, 0, hipY, 0);
    joint_matrix(&gmL[3], &obj, armL, 0, 0, shY, 0);
    joint_matrix(&gmL[4], &obj, armR, 0, 0, shY, 0);
    gfx_set_object_m(&gmL[0]);
    gfx_set_light_yaw(yaw);
    u32 lc[9][6];
    { int L[6] = { gfx_light(0, 0, -ONE), gfx_light(0, 0, ONE), gfx_light(-ONE, 0, 0), gfx_light(ONE, 0, 0), gfx_light(0, -ONE, 0), gfx_light(0, ONE, 0) };
      for (int s = 1; s <= 8; s++) for (int f = 0; f < 6; f++) lc[s][f] = lit(m->col[s], L[f]); }
    lod_cells(m, 1, 2, lr, 3, lc, cull);                                   // torso and head (the body part above the legs)
    // the eyes: two dark dots on the front of the lowest skin row of the head
    for (int cy = 3; cy >= m->headrow; cy--) {
        if (m->c[cy][1][1] == 1 && m->c[cy][1][2] == 1 && (!cull || cam_lz >= 2 * VOX)) {
            int Y0 = -(cy * 2 + 2) * VOX, ey = Y0 + VOX * 2 * 45 / 100, eh = 12, fz = 2 * VOX + 2;
            SVECTOR a, b, c, d; int ex[2] = { -VOX / 2 - 5, VOX / 2 - 5 };
            for (int e = 0; e < 2; e++) {
                setVector(&a, ex[e] + 10, ey, fz); setVector(&b, ex[e], ey, fz); setVector(&c, ex[e] + 10, ey + eh, fz); setVector(&d, ex[e], ey + eh, fz);   // seen from the front +x is on the left
                quads += gfx_quad_f(&a, &b, &c, &d, 26, 22, 34, 1);
            }
            break;
        }
    }
    gfx_set_object_m(&gmL[1]); lod_cells(m, 1, 1, 0, lr - 1, lc, cull);
    gfx_set_object_m(&gmL[2]); lod_cells(m, 2, 2, 0, lr - 1, lc, cull);
    gfx_set_object_m(&gmL[3]); lod_cells(m, 0, 0, 0, 3, lc, cull);
    gfx_set_object_m(&gmL[4]); lod_cells(m, 3, 3, 0, 3, lc, cull);
    gfx_set_object_m(&gmL[0]);
}
