// far: see far.h
#include "far.h"
#include "gfx.h"
#include "world.h"
#include "bigmap.h"
#include "atlas.h"

#ifndef FAR_VIEW0
#define FAR_VIEW0 3
#endif
int far_view = FAR_VIEW0, far_trees = 2, far_detail = 2, far_haze = 1, far_prims;
int far_hb[3], far_np[3], far_nclip, far_ncone, far_nquad, far_hb_win, far_hb_win0;   // (test readouts: land, buildings, trees)
static const short view_m[6] = { 0, 250, 500, 1000, 1500, 2000 };
int far_metres(void) { return view_m[CLAMP(far_view, 0, 5)]; }
#define M2U(m) ((m) * 213)                  // metres to world units (a tile, 256 units, is 1.2 m)
#define ZN 4                                // the near plane, in 1/16 units (64 world units)
#define GB 3                                // the guard band: the view's edges pushed out 3x (screen coordinates stay inside +-1023)

// ================================================================== the ordering table: the ground first, then back to front
static inline int far_slot(int zw) {        // zw: depth in world units
    if (zw < 8192) { int i = zw >> OT_SHIFT; return CLAMP(i, 1, OT_NEAR - 1); }
    int i = OT_NEAR + ((zw - 8192) >> 10); return MIN(i, OT_FAR_LAST);
}
static inline u32 *pk(int words) { return (u32 *)gfx_alloc((words + 1) * 4); }
static inline void lk(u32 *p, int words, int slot) { u32 *o = gfx_ot() + slot; p[0] = ((u32)words << 24) | (*o & 0xffffff); *o = (u32)p & 0xffffff; far_prims++; }
#define FXY(x, y) (((u32)(u16)(s16)(y) << 16) | (u16)(s16)(x))

// ================================================================== the far world's own scale
static VECTOR feye;                         // the eye (local units)
static int fwx0, fwy0;                      // the window's corner in world tiles
typedef struct { short sx, sy; int z; int vx, vy, vz; } FV;   // screen; depth (world units); view space (1/16 units)
static void far_gte(void) {                 // the camera's turn without its move: points go in as (point - eye) / 16
    MATRIX m = cam.view; m.t[0] = m.t[1] = m.t[2] = 0;
    GTE_FENCE(); gte_SetRotMatrix(&m); gte_SetTransMatrix(&m);
}
static inline short sc16(int d) { d >>= 4; return (short)CLAMP(d, -32000, 32000); }
static void fxf(int x, int y, int z, FV *o) {
    SVECTOR v SVEC_ALIGN; v.vx = sc16(x - feye.vx); v.vy = sc16(y - feye.vy); v.vz = sc16(z - feye.vz); v.pad = 0;
    GTE_FENCE(); gte_ldv0(&v); gte_rtps();
    u32 sxy; int sz; VECTOR mac;
    gte_stsxy(&sxy); gte_stsz(&sz); gte_stlvnl(&mac);
    o->sx = (short)(sxy & 0xffff); o->sy = (short)(sxy >> 16); o->z = sz << 4;
    o->vx = mac.vx; o->vy = mac.vy; o->vz = mac.vz;
}
static inline int fv_in(const FV *v) { return v->vz >= ZN && ABS(v->vx) <= GB * v->vz && ABS(v->vy) <= GB * v->vz; }

// ---- the haze: colours fade into the horizon's with distance ----
static int hz_r, hz_g, hz_b, hz_d0, hz_span, hz_max, hz_inv;
static inline int haze_f(int zw) {          // 0 .. hz_max (of 256)
    if (zw <= hz_d0 || !hz_max) return 0;
    int f = ((zw - hz_d0) >> 4) * hz_inv >> 12; return MIN(f, hz_max);
}
static inline u32 hazed(int r, int g, int b, int f) {
    r += ((hz_r - r) * f) >> 8; g += ((hz_g - g) * f) >> 8; b += ((hz_b - b) * f) >> 8;
    return (u32)CLAMP(r, 0, 255) | ((u32)CLAMP(g, 0, 255) << 8) | ((u32)CLAMP(b, 0, 255) << 16);
}

// ================================================================== clipping (big ground pieces near the eye)
typedef struct { int x, y, z, r, g, b; } CV;
static inline int pdist(const CV *v, int k) {
    switch (k) { case 0: return v->z - ZN; case 1: return v->x + GB * v->z; case 2: return GB * v->z - v->x; case 3: return v->y + GB * v->z; default: return GB * v->z - v->y; }
}
static int clip1(const CV *in, int n, CV *out, int k) {
    int m = 0;
    for (int i = 0; i < n; i++) {
        const CV *a = &in[i], *b = &in[i + 1 == n ? 0 : i + 1];
        int da = pdist(a, k), db = pdist(b, k);
        if (da >= 0) out[m++] = *a;
        if ((da >= 0) != (db >= 0)) {
            int t = (da << 12) / (da - db);
            CV *c = &out[m++];
            c->x = a->x + (((b->x - a->x) * t) >> 12); c->y = a->y + (((b->y - a->y) * t) >> 12); c->z = a->z + (((b->z - a->z) * t) >> 12);
            c->r = a->r + (((b->r - a->r) * t) >> 12); c->g = a->g + (((b->g - a->g) * t) >> 12); c->b = a->b + (((b->b - a->b) * t) >> 12);
        }
    }
    return m;
}
// a gouraud polygon of n (3 or 4: a quad in a b d c order... given as a ring) view-space vertices, clipped and drawn as a fan
static void emit_clipped(const FV *v, const u32 *col, int n, int slot) {
    CV a[12], b[12]; int m = n;
    for (int i = 0; i < n; i++) { a[i].x = v[i].vx; a[i].y = v[i].vy; a[i].z = v[i].vz; a[i].r = col[i] & 255; a[i].g = (col[i] >> 8) & 255; a[i].b = (col[i] >> 16) & 255; }
    for (int k = 0; k < 5 && m >= 3; k++) { m = clip1(a, m, b, k); memcpy(a, b, sizeof(CV) * m); }
    if (m < 3) return;
    s16 sx[12], sy[12];
    for (int i = 0; i < m; i++) { sx[i] = (s16)(CENTER_X + 230 * a[i].x / a[i].z); sy[i] = (s16)(CENTER_Y + 230 * a[i].y / a[i].z); }
    for (int i = 1; i + 1 < m; i++) {
        u32 *p = pk(6); if (!p) return;
        p[1] = 0x30000000u | (u32)a[0].r | ((u32)a[0].g << 8) | ((u32)a[0].b << 16); p[2] = FXY(sx[0], sy[0]);
        p[3] = (u32)a[i].r | ((u32)a[i].g << 8) | ((u32)a[i].b << 16); p[4] = FXY(sx[i], sy[i]);
        p[5] = (u32)a[i + 1].r | ((u32)a[i + 1].g << 8) | ((u32)a[i + 1].b << 16); p[6] = FXY(sx[i + 1], sy[i + 1]);
        lk(p, 6, slot);
    }
}
// a flat piece of ground (x0..x1, z0..z1 local units, z0 > z1), its corners hazed by their distance
static void ground_quad(int x0, int x1, int z0, int z1, int r, int g, int b, int slot, const int *hh) {   // hh: the ground's height at the four corners (the hills), or 0
    FV v[4]; int h0 = hh ? hh[0] : 0, h1 = hh ? hh[1] : 0, h2 = hh ? hh[2] : 0, h3 = hh ? hh[3] : 0;
    {   // the four corners (a ring), three at a time; good enough as they come unless one is near the eye or far off the screen
        short X0 = sc16(x0 - feye.vx), X1 = sc16(x1 - feye.vx), Z0 = sc16(z0 - feye.vz), Z1 = sc16(z1 - feye.vz), Y = sc16(-feye.vy);
        SVECTOR sv[4] SVEC_ALIGN = { { X0, (short)(Y - (h0 >> 4)), Z0, 0 }, { X1, (short)(Y - (h1 >> 4)), Z0, 0 }, { X1, (short)(Y - (h2 >> 4)), Z1, 0 }, { X0, (short)(Y - (h3 >> 4)), Z1, 0 } };
        XV o[4]; gfx_xform(sv, o, 4);
        int good = 1;
        for (int i = 0; i < 4; i++) { if (o[i].z < ZN * 2 || o[i].x < CENTER_X - 680 || o[i].x > CENTER_X + 680 || o[i].y < CENTER_Y - 680 || o[i].y > CENTER_Y + 680) good = 0;
                                      v[i].sx = o[i].x; v[i].sy = o[i].y; v[i].z = o[i].z << 4; }
        if (!good) { fxf(x0, -h0, z0, &v[0]); fxf(x1, -h1, z0, &v[1]); fxf(x1, -h2, z1, &v[2]); fxf(x0, -h3, z1, &v[3]); }
        else for (int i = 0; i < 4; i++) { v[i].vz = ZN * 8; v[i].vx = v[i].vy = 0; }   // (in: no clipping)
    }
    int in = 0, behind = 0;
    for (int i = 0; i < 4; i++) { in += fv_in(&v[i]); behind += v[i].vz < ZN; }
    if (behind == 4) return;
    int sl = hh ? CLAMP(((h1 + h2 - h0 - h3) + (h3 + h2 - h0 - h1)) * 5 / ((x1 - x0 > 0 ? x1 - x0 : x0 - x1) / 8 + 1) , -70, 70) : 0;   // (the slope's light, from the north-west: the nearer to the sun a slope faces, the brighter)
    u32 c[4]; for (int i = 0; i < 4; i++) c[i] = hazed(CLAMP(r + (r * sl >> 8), 0, 255), CLAMP(g + (g * sl >> 8), 0, 255), CLAMP(b + (b * sl >> 8), 0, 255), haze_f(v[i].z));
    far_nquad++;
    if (in < 4) { far_nclip++; emit_clipped(v, c, 4, slot); return; }
    int ox = (v[0].sx < 0) & (v[1].sx < 0) & (v[2].sx < 0) & (v[3].sx < 0);
    ox |= (v[0].sx >= SCREEN_W) & (v[1].sx >= SCREEN_W) & (v[2].sx >= SCREEN_W) & (v[3].sx >= SCREEN_W);
    ox |= (v[0].sy < 0) & (v[1].sy < 0) & (v[2].sy < 0) & (v[3].sy < 0);
    ox |= (v[0].sy >= SCREEN_H) & (v[1].sy >= SCREEN_H) & (v[2].sy >= SCREEN_H) & (v[3].sy >= SCREEN_H);
    if (ox) return;
    u32 *p = pk(8); if (!p) return;                              // G4: corners 0 1 3 2 (the GPU's order: a b c d = TL TR BL BR)
    p[1] = 0x38000000u | c[0]; p[2] = FXY(v[0].sx, v[0].sy); p[3] = c[1]; p[4] = FXY(v[1].sx, v[1].sy);
    p[5] = c[3]; p[6] = FXY(v[3].sx, v[3].sy); p[7] = c[2]; p[8] = FXY(v[2].sx, v[2].sy);
    lk(p, 8, slot);
}

// ================================================================== the land round you: 64 x 64 cells of 4 x 4 blocks (77 m)
#define FC 64
#define FCB 4                                // blocks a cell
#define CELL_T (FCB * BM_BLK)               // tiles a cell (64)
static u8 fcell[FC][FC];                     // BT_ (0..5), +8 in a town; 0xFF: not worked out yet
static int fc_x0 = -100000, fc_y0;          // the world cell at [0][0]
static int fc_unknown;                       // cells still to work out
static int far_cell_type(int cx, int cy) {
    int bx = cx * FCB + FCB / 2, by = cy * FCB + FCB / 2;
    int t = bm_terrain(bx, by);
    if (t != BT_SEA && bm_set_at(bx, by) != 255) t |= 8;
    return t;
}
static int pbx, pby;                         // the world block you are on
static void cells_follow(int budget) {       // keep the cells centred on you; work out a few a frame (the nearest first)
    int want_x = pbx / FCB - FC / 2, want_y = pby / FCB - FC / 2;
    int dx = want_x - fc_x0, dy = want_y - fc_y0;
    if (dx || dy) {
        if (ABS(dx) >= FC / 2 || ABS(dy) >= FC / 2) memset(fcell, 0xFF, sizeof fcell);
        else {
            static u8 tmp[FC][FC];
            for (int y = 0; y < FC; y++) for (int x = 0; x < FC; x++) {
                int sx = x + dx, sy = y + dy;
                tmp[y][x] = (sx < 0 || sy < 0 || sx >= FC || sy >= FC) ? 0xFF : fcell[sy][sx];
            }
            memcpy(fcell, tmp, sizeof fcell);
        }
        fc_x0 = want_x; fc_y0 = want_y;
        fc_unknown = 0; for (int y = 0; y < FC; y++) for (int x = 0; x < FC; x++) fc_unknown += fcell[y][x] == 0xFF;
    }
    if (!fc_unknown) return;
    for (int r = 0; r <= FC / 2 && budget > 0; r++)             // rings out from the middle
        for (int y = FC / 2 - r; y <= FC / 2 + r && y < FC && budget > 0; y++) {
            int step = (y == FC / 2 - r || y == FC / 2 + r) ? 1 : 2 * r;
            for (int x = FC / 2 - r; x <= FC / 2 + r && x < FC && budget > 0; x += step ? step : 1) {
                if (y < 0 || x < 0) continue;
                if (fcell[y][x] != 0xFF) continue;
                fcell[y][x] = (u8)far_cell_type(fc_x0 + x, fc_y0 + y); budget--; fc_unknown--;
            }
        }
}
void far_reset(void) {
    if (world_wbx < 0) return;
    pbx = world_wbx + 2; pby = world_wby + 2;
    fc_x0 = -100000; cells_follow(FC * FC);
}
int far_season, far_wet;   // the season (0..3) and the weather's wet (1): main sets them
static void season_rgb(int kind, int *r, int *g, int *b) {   // kind: 0 grass, 1 forest, 2 hard ground (paving, road, rock, sand, soil), 3 water, 4 a town
    int tr = *r, tg = *g, tb = *b, f = 0;
    if (far_season == 3) { tr = 232; tg = 236; tb = 246; f = kind == 3 ? 8 : kind == 4 ? 7 : kind == 2 ? 13 : 15; }
    else if (far_season == 0 && kind <= 1) { tr = 84; tg = 214; tb = 66; f = kind ? 4 : 8; }
    else if (far_season == 2 && kind <= 1) { tr = 200; tg = 112; tb = 40; f = kind ? 10 : 11; }
    *r += (tr - *r) * f / 16; *g += (tg - *g) * f / 16; *b += (tb - *b) * f / 16;
    if (far_wet == 1 && kind != 3) { *r = *r * 11 / 16; *g = *g * 12 / 16; *b = *b * 13 / 16; }
}
static const u8 tcol[6][3] = { { 46, 92, 150 }, { 212, 198, 150 }, { 96, 148, 72 }, { 50, 104, 56 }, { 206, 178, 120 }, { 136, 130, 120 } };
static void cell_rgb(int t, int *r, int *g, int *b) {
    if (t == 0xFF) t = BT_GRASS;
    if (t & 8) { *r = 142; *g = 138; *b = 128; season_rgb(4, r, g, b); return; }       // a town: roofs and streets
    t &= 7; if (t > 5) t = BT_GRASS;
    *r = tcol[t][0]; *g = tcol[t][1]; *b = tcol[t][2];
    season_rgb(t == BT_SEA ? 3 : t == BT_FOREST ? 1 : t == BT_GRASS ? 0 : 2, r, g, b);
}

// ================================================================== every town's buildings (worked out once a world)
typedef struct { u16 bx, by; u8 lot, sto, col, pad; } FB;
#define FB_MAX 2400
static FB fb[FB_MAX]; static int nfb;
static u16 fb_lo[64], fb_hi[64];             // each town's run in fb
#define FBC_MAX 900
static u16 fbc_cx[FBC_MAX], fbc_cy[FBC_MAX], fbc_lo[FBC_MAX], fbc_hi[FBC_MAX]; static int nfbc;   // the runs by cell (4x4 blocks)
static u16 fbs_lo[64], fbs_hi[64];           // each town's cells
static const u8 has_bld[LOT_N] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 };
// each lot's building, block-relative tiles (x0, y0, x1, y1) and roof (0 flat, 1 pitched)
static const u8 foot[LOT_N][5] = {
    { 0, 0, 0, 0, 0 },   { 5, 5, 13, 12, 1 }, { 5, 5, 11, 11, 1 }, { 3, 5, 15, 12, 1 }, { 5, 4, 13, 12, 1 }, { 4, 4, 15, 15, 0 },
    { 4, 4, 15, 13, 0 }, { 4, 5, 14, 12, 0 }, { 4, 4, 15, 14, 0 }, { 4, 4, 15, 14, 0 }, { 4, 4, 14, 14, 1 }, { 4, 4, 9, 8, 0 },
    { 3, 4, 15, 15, 0 }, { 4, 4, 14, 13, 0 }, { 4, 4, 14, 14, 0 }, { 3, 3, 15, 15, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0 },   { 0, 0, 0, 0, 0 },   { 6, 6, 11, 10, 1 },
    { 4, 5, 14, 12, 0 }, { 4, 5, 14, 12, 0 }, { 3, 3, 15, 14, 0 }, { 4, 5, 14, 12, 0 }, { 3, 4, 15, 13, 0 }, { 4, 4, 14, 13, 0 },   // cafe bar cinema salon restaurant clinic
    { 3, 4, 15, 13, 0 }, { 4, 4, 14, 13, 0 }, { 4, 5, 14, 12, 0 },
    { 4, 4, 14, 9, 0 }, { 4, 5, 14, 11, 0 }, { 0, 0, 0, 0, 0 } };                                                                 // library spa gallery
static const u8 wallc[10][3] = { { 226, 214, 190 }, { 176, 96, 76 }, { 236, 234, 226 }, { 214, 190, 150 }, { 160, 160, 164 },
                                 { 180, 200, 214 }, { 186, 206, 170 }, { 226, 180, 180 }, { 120, 140, 160 }, { 150, 120, 96 } };
static const u8 roofc[4][3] = { { 150, 60, 50 }, { 70, 80, 110 }, { 90, 70, 60 }, { 60, 110, 70 } };
void far_new_world(void) {
    nfb = 0; nfbc = 0;
    int ns = bm_nset();
    for (int i = 0; i < ns && i < 64; i++) {
        const Settle *s = bm_set(i);
        fb_lo[i] = (u16)nfb;
        for (int by = s->y - s->r; by <= s->y + s->r; by++) for (int bx = s->x - s->r; bx <= s->x + s->r; bx++) {
            int dx = bx - s->x, dy = by - s->y; if (dx * dx + dy * dy > s->r * s->r) continue;
            BInfo b; bm_block(bx, by, &b);
            if (b.set != i || b.lot >= LOT_N || !has_bld[b.lot] || nfb >= FB_MAX) continue;
            FB *f = &fb[nfb++];
            f->bx = (u16)bx; f->by = (u16)by; f->lot = b.lot; f->sto = (u8)bm_storeys(&b);
            f->col = (u8)(b.lot == LOT_TOWER ? 8 : (b.h >> 20) % 8u);
            f->pad = (u8)((b.h >> 9) & 3);
        }
        fb_hi[i] = (u16)nfb;
        // in cell order (a cell's buildings together: the view takes or leaves a whole cell at once)
        for (int a = fb_lo[i] + 1; a < nfb; a++) {
            FB t = fb[a]; u32 kt = ((u32)(t.by >> 2) << 10) | (t.bx >> 2); int b = a - 1;
            while (b >= fb_lo[i] && (((u32)(fb[b].by >> 2) << 10) | (fb[b].bx >> 2)) > kt) { fb[b + 1] = fb[b]; b--; }
            fb[b + 1] = t;
        }
        fbs_lo[i] = (u16)nfbc;
        for (int a = fb_lo[i]; a < nfb; ) {
            int cx = fb[a].bx >> 2, cy = fb[a].by >> 2, b = a;
            while (b < nfb && (fb[b].bx >> 2) == cx && (fb[b].by >> 2) == cy) b++;
            if (nfbc < FBC_MAX) { fbc_cx[nfbc] = (u16)cx; fbc_cy[nfbc] = (u16)cy; fbc_lo[nfbc] = (u16)a; fbc_hi[nfbc] = (u16)b; nfbc++; }
            a = b;
        }
        fbs_hi[i] = (u16)nfbc;
    }
}

// ================================================================== textures: a wall of windows (repeats), tree cards
static TexRef win_tex, tree_tex[4]; static int tex_ok;
static u32 win_e2;                           // the texture window that repeats win_tex every 16 texels
static void far_tex(void) {
    static u16 px[32 * 32];
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {   // two windows across, two storeys up: a frame, the glass
        int cx = x & 7, cy = y & 7; u16 c = RGB(31, 31, 31);
        if (cx >= 2 && cx <= 5 && cy >= 2 && cy <= 6) c = (cx == 2 || cy == 2) ? RGB(7, 9, 12) : RGB(10, 13, 17);
        if (cy == 7) c = RGB(27, 27, 27);
        px[y * 16 + x] = c;
    }
    win_tex = atlas_add_aligned(16, 16, 16, px);
    win_e2 = 0xE2000000u | 0x1E | (0x1E << 5) | (((win_tex.u >> 3) & 0x1F) << 10) | (((win_tex.v >> 3) & 0x1F) << 15);
    for (int k = 0; k < 4; k++) {            // oak, pine, palm, cactus: 16 x 32, the trunk at the bottom middle
        memset(px, 0, sizeof px);
        for (int y = 0; y < 32; y++) for (int x = 0; x < 16; x++) {
            u16 c = 0; int dx = x * 2 - 15, n = (x * 7 + y * 13) & 3;
            if (k == 0) {   // oak: a round crown on a trunk
                int ey = y - 11, e = dx * dx * 100 / (15 * 15) + ey * ey * 100 / (11 * 11);
                if (e < 100) { int s = 14 + (11 - ey) / 3 + n - (e > 70 ? 3 : 0); c = RGB(s / 3, s, s / 4); }
                else if (y >= 20 && ABS(dx) <= 2) c = RGB(12, 8, 4);
            } else if (k == 1) {   // pine: tiers narrowing up
                int tier = (y - 2) % 7, w = (y - 2) * 7 / 26 + 1 + tier / 2;
                if (y >= 2 && y < 28 && ABS(dx) <= w * 2) { int s = 12 + n + (ABS(dx) < w ? 3 : 0); c = RGB(s / 4, s, s / 2); }
                else if (y >= 28 && ABS(dx) <= 1) c = RGB(11, 7, 4);
            } else if (k == 2) {   // palm: a thin trunk, fronds at the top
                int tx = 8 + (31 - y) / 10;
                if (y >= 8 && ABS(x - tx) <= 1) c = RGB(17, 12, 7);
                if (y < 10) { int fy = ABS(dx) / 3; if (y >= 2 + fy && y <= 5 + fy) c = RGB(6, 20 + n, 6); }
            } else {   // cactus: a column with arms
                if (y >= 6 && ABS(dx) <= 3) c = RGB(8, 18 + n, 8);
                if (y >= 12 && y <= 20 && (dx == -9 || dx == -7 || dx == 7 || dx == 9)) c = RGB(8, 17, 8);
                if (y == 20 && ABS(dx) <= 9) c = RGB(8, 17, 8);
            }
            px[y * 16 + x] = c;
        }
        tree_tex[k] = atlas_add(16, 32, px);
    }
    tex_ok = 1;
}

// ================================================================== the camera's reach
static int fdx, fdz, fh;                      // the view's direction on the ground (unit, 4096) and its length
static int max_ground, max_bld, max_tree;     // how far the ground, buildings and trees can show (world units)
static int cone_c, cone_s;                    // the cone the view takes in on the ground: its half-angle's cos and sin (4096)
static int Ls, Ln, Lw, Le, Ltop, Lroof;        // the sun on each side of a box (once a frame)
// is the ground point (x, z) (local units, radius rad) inside the view's cone, and nearer than dmax?
static inline int in_cone(int x, int z, int rad, int dmax) {
    far_ncone++;
    int dx = x - feye.vx, dz = z - feye.vz;
    if (ABS(dx) > dmax + rad || ABS(dz) > dmax + rad) return 0;
    int fwd = (dx * fdx + dz * fdz) >> 12;                     // along the view
    if (fwd < -rad) return 0;
    int lat = (dx * fdz - dz * fdx) >> 12; lat = ABS(lat);     // across it
    return lat * cone_c <= (fwd + rad) * cone_s + rad * 4096;
}
static int far_setup(void) {
    feye = cam.eye;
    int ax = cam.at.vx - cam.eye.vx, ay = cam.at.vy - cam.eye.vy, az = cam.at.vz - cam.eye.vz;
    fh = isqrt(ax * ax + az * az); if (fh < 1) fh = 1;
    fdx = (ax << 12) / fh; fdz = (az << 12) / fh;
    int far = M2U(far_metres());
    // how far the ground shows: the top edge of the screen looks down at (pitch - 27.6 degrees); past where it meets the ground,
    // nothing on the ground can show (tall things a little further)
    int h = -feye.vy; if (h < 64) h = 64;
    int tp = (ay << 12) / fh;                                   // tan(pitch) (4096; positive looking down)
    int tv = 2137;                                              // tan(half the vertical view) = 120/230
    int num = tp - tv, den = 4096 + ((tp * tv) >> 12);
    if (num > 40) {                                             // the horizon is off the top of the screen
        int ta = (num << 12) / den;                             // tan(the top edge's angle down)
        max_ground = (int)(((long long)h << 12) / ta);
        max_bld = (int)(((long long)(h + 3200) << 12) / ta);
        max_ground = MIN(max_ground, far); max_bld = MIN(max_bld, far);
        max_tree = (int)(((long long)(h + 640) << 12) / ta);       // (a tree card stands about 600 high)
    } else { max_ground = far; max_bld = far; max_tree = far; }
    static const int tree_m[4] = { 0, 100, 200, 350 };
    max_tree = MIN(MIN(M2U(tree_m[CLAMP(far_trees, 0, 3)]), max_bld), max_tree);
    // the cone on the ground: wide enough for the screen's corners when looking down (a generous 50 degrees each side)
    cone_c = 2633; cone_s = 3138;
    Ls = CLAMP(gfx_light(0, 0, -ONE), 40, 220); Ln = CLAMP(gfx_light(0, 0, ONE), 40, 220); Lw = CLAMP(gfx_light(-ONE, 0, 0), 40, 220);
    Le = CLAMP(gfx_light(ONE, 0, 0), 40, 220); Ltop = CLAMP(gfx_light(0, -ONE, 0), 60, 220); Lroof = CLAMP(gfx_light(0, -ONE * 3 / 4, -ONE / 2), 50, 230);
    // the haze
    static const int hz_m[3] = { 0, 170, 235 };
    hz_max = hz_m[CLAMP(far_haze, 0, 2)]; hz_d0 = M2U(60); hz_span = MAX(far - hz_d0, M2U(100));
    hz_inv = (256 << 16) / (hz_span >> 4) >> 4;                 // (f = (z - d0) * 256 / span, without a division a point)
    return max_ground > 0 || max_bld > 0;
}

// ================================================================== the land
// The land is laid in rectangles: each row's runs of one colour, and a run the same as the one above it carries that one on down
// (a field, a wood, the sea: one piece each, not one a row).
typedef struct { short x0, x1, y0, y1; u32 key; } Rect;
#define NRECT 360
static Rect rects[NRECT]; static int nrect;
static u16 open_[48], nopen, cur_[48], ncur;
static void rows_begin(void) { nrect = 0; nopen = 0; }
static void row_begin(void) { ncur = 0; }
static void row_run(int y, int x0, int x1, u32 key) {
    for (int i = 0; i < nopen; i++) { Rect *r = &rects[open_[i]]; if (r->x0 == x0 && r->x1 == x1 && r->key == key) { r->y1 = (short)(y + 1); if (ncur < 48) cur_[ncur++] = open_[i]; return; } }
    if (nrect >= NRECT) return;
    Rect *r = &rects[nrect]; r->x0 = (short)x0; r->x1 = (short)x1; r->y0 = (short)y; r->y1 = (short)(y + 1); r->key = key;
    if (ncur < 48) cur_[ncur++] = (u16)nrect;
    nrect++;
}
static void row_end(void) { memcpy(open_, cur_, ncur * sizeof cur_[0]); nopen = ncur; }
static u8 wvis[MH / 4][MW / 4];                  // window cells the view takes in (the land pass; the trees use it too)
static u32 floor_key0(int fl);
static u32 floor_key(int fl) {
    u32 k = floor_key0(fl); if (fl < FL_GRASS || (!far_season && !far_wet)) return k;
    int r = k & 255, g = (k >> 8) & 255, b = (k >> 16) & 255;   // (the colour is r, g, b in the key as set below: R low, B high)
    season_rgb(fl == FL_GRASS ? 0 : fl == FL_WATER ? 3 : 2, &r, &g, &b);
    return (u32)r | ((u32)g << 8) | ((u32)b << 16);
}
static u32 floor_key0(int fl) {
    switch (fl) {
        case FL_GRASS: return 0x488400 | 96;  case FL_PAVING: return 0x9EA800 | 172; case FL_SOIL: return 0x3E5C00 | 120;
        case FL_SAND: return 0x96C600 | 212;  case FL_WATER: return 0x965C00 | 46;   case FL_ROCK: return 0x788200 | 136;
        case FL_ASPHALT: case FL_LINE_H: case FL_LINE_V: case FL_ZEB_A: case FL_ZEB_B: case FL_EDGE_N: case FL_EDGE_S: case FL_EDGE_W: case FL_EDGE_E: return 0x4E4A00 | 74;
        default: return 0x728600 | 150;       // indoors (under a roof)
    }
}
static void draw_land(int rx0, int ry0, int rx1, int ry1) {
    const int hills = world_zone == ZONE_CITY && world_wbx >= 0;
    far_hb_win0 = gfx_hb();
    // 1. the window round you, past the detail: its own floor, in cells of 4 x 4 tiles
    rows_begin();
    for (int cy = 0; cy < MH / 4; cy++) {
        row_begin();
        int run0 = -1; u32 rk = 0;
        for (int cx = 0; cx <= MW / 4; cx++) {
            int ok = 0; u32 k = 0;
            if (cx < MW / 4) {
                int tx = cx * 4, ty = cy * 4;
                int inside = tx >= rx0 && tx + 3 <= rx1 && ty >= ry0 && ty + 3 <= ry1;
                int wcx = tx * TILE + 2 * TILE, wcz = -(ty * TILE + 2 * TILE);
                int vis = in_cone(wcx, wcz, 3 * TILE, MAX(max_ground, max_tree)); wvis[cy][cx] = (u8)vis;
                int near_ = MAX(ABS(wcx - feye.vx), ABS(wcz - feye.vz)) <= max_ground + 3 * TILE;
                if (!inside && vis && near_) { ok = 1; k = floor_key(floorMap[ty + 2][tx + 2]); }
            }
            if (run0 >= 0 && (!ok || k != rk)) { row_run(cy, run0, cx, rk); run0 = -1; }
            if (ok && run0 < 0) { run0 = cx; rk = k; }
        }
        row_end();
    }
    for (int i = 0; i < nrect; i++) { const Rect *r = &rects[i];
        if (hills) { for (int cy = r->y0; cy < r->y1; cy++) for (int cx = r->x0; cx < r->x1; cx++) {   // (hills: a quad a cell, each corner at the height of the land there)
                int hh[4] = { world_gh_c(cx * 4, cy * 4), world_gh_c(cx * 4 + 4, cy * 4), world_gh_c(cx * 4 + 4, cy * 4 + 4), world_gh_c(cx * 4, cy * 4 + 4) };
                ground_quad(cx * 4 * TILE, (cx + 1) * 4 * TILE, -(cy * 4 * TILE), -((cy + 1) * 4 * TILE), r->key & 255, (r->key >> 16) & 255, (r->key >> 8) & 255, OT_GROUND_WIN, hh); } }
        else
        ground_quad(r->x0 * 4 * TILE, r->x1 * 4 * TILE, -(r->y0 * 4 * TILE), -(r->y1 * 4 * TILE), r->key & 255, (r->key >> 16) & 255, (r->key >> 8) & 255, OT_GROUND_WIN, 0); }
    far_hb_win = gfx_hb();
    // 2. past the window: the land's cells (77 m)
    int reach = max_ground / (CELL_T * TILE) + 1;
    int pcx = pbx / FCB, pcy = pby / FCB;
    static u8 vis2[FC / 2 + 2][FC / 2 + 2];                      // 2x2 groups of cells the view might take in
    int gx0 = (pcx - reach) >> 1, gy0 = (pcy - reach) >> 1, gn = MIN(FC / 2 + 1, reach + 2);
    for (int gy = 0; gy <= gn; gy++) for (int gx = 0; gx <= gn; gx++) {
        int cx = (gx0 + gx) * 2, cy = (gy0 + gy) * 2;
        int x0 = (cx * CELL_T - fwx0) * TILE, z0 = -((cy * CELL_T - fwy0) * TILE);
        vis2[gy][gx] = (u8)in_cone(x0 + CELL_T * TILE, z0 - CELL_T * TILE, CELL_T * TILE * 3 / 2, max_ground);
    }
    // in 2x2 groups (154 m: the window's own floor covers the first 50 m round you): each the colour most of its four cells are
    rows_begin();
    for (int gy = 0; gy <= gn; gy++) {
        row_begin();
        int run0 = -1, rt = -1;
        for (int gx = 0; gx <= gn + 1; gx++) {
            int ok = 0, t = -1;
            if (gx <= gn && vis2[gy][gx]) {
                int ix = (gx0 + gx) * 2 - fc_x0, iy = (gy0 + gy) * 2 - fc_y0;
                if (ix >= 0 && iy >= 0 && ix + 1 < FC && iy + 1 < FC) {
                    int a = fcell[iy][ix], b = fcell[iy][ix + 1], c = fcell[iy + 1][ix], d = fcell[iy + 1][ix + 1];
                    t = (a == b || a == c || a == d) ? a : (b == c || b == d) ? b : c == d ? c : a;
                    ok = 1;
                }
            }
            if (run0 >= 0 && (!ok || t != rt)) { row_run(gy, run0, gx, (u32)rt); run0 = -1; }
            if (ok && run0 < 0) { run0 = gx; rt = t; }
        }
        row_end();
    }
    for (int i = 0; i < nrect; i++) { const Rect *r = &rects[i]; int R, G, B; cell_rgb((int)r->key, &R, &G, &B);
        int x0 = ((gx0 + r->x0) * 2 * CELL_T - fwx0) * TILE, x1 = ((gx0 + r->x1) * 2 * CELL_T - fwx0) * TILE;
        int z0 = -(((gy0 + r->y0) * 2 * CELL_T - fwy0) * TILE), z1 = -(((gy0 + r->y1) * 2 * CELL_T - fwy0) * TILE);
        if (hills) { int gs = 2 * CELL_T * TILE, nx = (x1 - x0) / gs, nz = (z0 - z1) / gs;   // (hills: a quad a group of cells)
            for (int j = 0; j < nz; j++) for (int i = 0; i < nx; i++) {
                int ax = x0 + i * gs, bz = z0 - j * gs;
                int hh[4] = { world_gh(ax, bz), world_gh(ax + gs, bz), world_gh(ax + gs, bz - gs), world_gh(ax, bz - gs) };
                ground_quad(ax, ax + gs, bz, bz - gs, R, G, B, OT_GROUND_FAR, hh); } }
        else
        ground_quad(x0, x1, z0, z1, R, G, B, OT_GROUND_FAR, 0); }
}

// ================================================================== the buildings past the map
static inline u32 lit(const u8 *c, int L, int f) { return hazed((c[0] * L) >> 7, (c[1] * L) >> 7, (c[2] * L) >> 7, f); }
static void face_f4(const XV *a, const XV *b, const XV *c, const XV *d, u32 col, int slot) {   // a b c d = TL TR BL BR
    u32 *p = pk(5); if (!p) return;
    p[1] = 0x28000000u | col; p[2] = FXY(a->x, a->y); p[3] = FXY(b->x, b->y); p[4] = FXY(c->x, c->y); p[5] = FXY(d->x, d->y);
    lk(p, 5, slot);
}
static void face_win(const XV *a, const XV *b, const XV *c, const XV *d, u32 col, int uw, int vh, int slot) {   // windows, repeated
    u32 *p = pk(11); if (!p) return;
    uw = MIN(uw, 255); vh = MIN(vh, 255);
    u32 tp = (u32)win_tex.tpage << 16;
    p[1] = win_e2;
    p[2] = 0x2C000000u | col; p[3] = FXY(a->x, a->y); p[4] = 0;                       p[5] = FXY(b->x, b->y); p[6] = (u32)uw | tp;
    p[7] = FXY(c->x, c->y);    p[8] = (u32)vh << 8;                                   p[9] = FXY(d->x, d->y); p[10] = (u32)uw | ((u32)vh << 8);
    p[11] = 0xE2000000u;
    lk(p, 11, slot);
}
static inline int xv_good(const XV *v) { return v->z >= ZN && v->x > -1000 && v->x < 1320 && v->y > -1000 && v->y < 1240; }
// a building as a box: x0 < x1, z0 > z1 (north, south), sto storeys; its walls' colour, a pitched roof (or flat) and its colour
static int far_lift;   // the hills: how high the ground is under the building being drawn
static void far_card(int x0, int z0, int x1, int z1, int top, const u8 *wc) {   // far off: one face, the one most turned to you
    int ex = feye.vx, ey = feye.vy + far_lift, ez = feye.vz, cx = (x0 + x1) / 2, cz = (z0 + z1) / 2;
    int ax, az, bx, bz, L;
    if (ABS(ex - cx) * (z0 - z1) > ABS(ez - cz) * (x1 - x0)) {   // an east or west face
        if (ex > cx) { ax = x1; az = z1; bx = x1; bz = z0; L = Le; } else { ax = x0; az = z0; bx = x0; bz = z1; L = Lw; }
    } else {
        if (ez > cz) { ax = x1; az = z0; bx = x0; bz = z0; L = Ln; } else { ax = x0; az = z1; bx = x1; bz = z1; L = Ls; }
    }
    SVECTOR v[4] SVEC_ALIGN = { { sc16(ax - ex), sc16(top - ey), sc16(az - ez), 0 }, { sc16(bx - ex), sc16(top - ey), sc16(bz - ez), 0 },
                                { sc16(ax - ex), sc16(-ey), sc16(az - ez), 0 }, { sc16(bx - ex), sc16(-ey), sc16(bz - ez), 0 } };
    XV o[4]; gfx_xform(v, o, 4);
    for (int i = 0; i < 4; i++) if (!xv_good(&o[i])) return;
    int zw = ((o[0].z + o[3].z) >> 1) << 4;
    face_f4(&o[0], &o[1], &o[2], &o[3], lit(wc, L, haze_f(zw)), far_slot(zw));
}
static void far_box(int x0, int z0, int x1, int z1, int sto, const u8 *wc, int pitched, const u8 *rc, int windows) {
    int top = -(sto * WALL_FULL + (pitched ? 0 : 40));
    int ex = feye.vx, ey = feye.vy + far_lift, ez = feye.vz;
    { int dx = ABS((x0 + x1) / 2 - ex), dz = ABS((z0 + z1) / 2 - ez);
      if (MAX(dx, dz) > M2U(150)) { far_card(x0, z0, x1, z1, top, wc); return; } }   // (far off: one face does)
    short X0 = sc16(x0 - ex), X1 = sc16(x1 - ex), Z0 = sc16(z0 - ez), Z1 = sc16(z1 - ez), Y0 = sc16(-ey), Y1 = sc16(top - ey);
    SVECTOR v[8] SVEC_ALIGN = { { X0, Y0, Z0, 0 }, { X1, Y0, Z0, 0 }, { X0, Y0, Z1, 0 }, { X1, Y0, Z1, 0 },     // the foot: NW NE SW SE
                                { X0, Y1, Z0, 0 }, { X1, Y1, Z0, 0 }, { X0, Y1, Z1, 0 }, { X1, Y1, Z1, 0 } };   // the top
    XV o[8]; gfx_xform(v, o, 8);
    for (int i = 0; i < 8; i++) if (!xv_good(&o[i])) return;
    int zmid = (o[0].z + o[3].z + o[4].z + o[7].z) >> 2, zw = zmid << 4;
    int slot = far_slot(zw), hf = haze_f(zw);
    int use_win = windows && far_detail >= 2 && tex_ok && 230 * sto * WALL_FULL > zw * 20;   // (windows when it stands 20 px or more)
    int widx = (x1 - x0) / TILE, widz = (z0 - z1) / TILE;
    // the walls that face the eye (at most two), then the roof
    #define WALL(A, B, C, D, L, w) { u32 col = lit(wc, L, hf); \
        if (use_win) face_win(&o[A], &o[B], &o[C], &o[D], col, (w) * 4, sto * 8, slot); else face_f4(&o[A], &o[B], &o[C], &o[D], col, slot); }
    if (ez < z1) WALL(6, 7, 2, 3, Ls, widx)                     // south: SW SE (top), SW SE (foot)
    if (ez > z0) WALL(5, 4, 1, 0, Ln, widx)                     // north: seen from the north, NE is on the left
    if (ex < x0) WALL(4, 6, 0, 2, Lw, widz)                     // west: NW SW
    if (ex > x1) WALL(7, 5, 3, 1, Le, widz)                     // east: SE NE
    #undef WALL
    if (ey >= top) return;                                       // (looking up at it: no roof to see)
    if (pitched) {                                               // four slopes up to a point
        SVECTOR ap SVEC_ALIGN; ap.vx = sc16((x0 + x1) / 2 - ex); ap.vz = sc16((z0 + z1) / 2 - ez); ap.vy = sc16(top - MIN(x1 - x0, z0 - z1) / 3 - ey); ap.pad = 0;
        XV A; gfx_xform(&ap, &A, 1); if (!xv_good(&A)) return;
        const XV *ring[4] = { &o[4], &o[5], &o[7], &o[6] };     // NW NE SE SW
        static const u8 sh[4] = { 6, 7, 8, 7 };
        for (int k = 0; k < 4; k++) {
            const XV *p0 = ring[k], *p1 = ring[(k + 1) & 3];
            if ((p1->x - p0->x) * (A.y - p0->y) - (A.x - p0->x) * (p1->y - p0->y) <= 0) continue;   // facing away
            u32 col = lit(rc, Lroof * sh[k] / 7, hf);
            u32 *p = pk(4); if (!p) return;
            p[1] = 0x20000000u | col; p[2] = FXY(p0->x, p0->y); p[3] = FXY(p1->x, p1->y); p[4] = FXY(A.x, A.y);
            lk(p, 4, slot);
        }
    } else {
        static const u8 flat[3] = { 120, 118, 114 };
        face_f4(&o[4], &o[5], &o[6], &o[7], lit(flat, Ltop, hf), slot);
    }
}
static void far_dot(int x0, int z0, int x1, int z1, int sto, const u8 *wc) {   // very far off: the whole building as one flat rectangle (one point transformed, one primitive)
    int cx = (x0 + x1) / 2, cz = (z0 + z1) / 2; FV b; fxf(cx, -far_lift, cz, &b);
    if (b.vz < ZN * 4 || !fv_in(&b)) return;
    int zw = b.z; if (zw < 64) return;
    int s16_ = (230 << 14) / zw;                                       // pixels a unit (1/16384)
    int wp = (MAX(x1 - x0, z0 - z1) * s16_) >> 14, hp = ((sto * WALL_FULL + 40) * s16_) >> 14;
    if (wp < 1) wp = 1; if (hp < 1) hp = 1;
    int xa = b.sx - wp / 2, xb = xa + wp, ya = b.sy - hp, yb = b.sy;
    if (xb < 0 || xa >= SCREEN_W || yb < 0 || ya >= SCREEN_H) return;
    u32 col = lit(wc, Ls, haze_f(zw));
    u32 *p = pk(5); if (!p) return;
    p[1] = 0x28000000u | col; p[2] = FXY(xa, ya); p[3] = FXY(xb, ya); p[4] = FXY(xa, yb); p[5] = FXY(xb, yb);
    lk(p, 5, far_slot(zw));
}
static void far_building(const FB *f, int x0, int z0, int x1, int z1) {
    int pitched = foot[f->lot][4] && f->sto <= 2;
    far_lift = world_gh((x0 + x1) / 2, (z0 + z1) / 2);
    far_box(x0, z0, x1, z1, f->sto, wallc[f->col % 10], pitched, roofc[f->pad & 3], f->lot != LOT_HOUSE && f->lot != LOT_COTTAGE);
    far_lift = 0;
}
// the map's own buildings past the detail (world.c's skyline), drawn the same way
void far_box_world(int x0, int z0, int x1, int z1, int sto, u16 wall15, int pitched, u16 roof15) {
    u8 wc[3] = { (u8)R8(wall15), (u8)G8(wall15), (u8)B8(wall15) }, rc[3] = { (u8)R8(roof15), (u8)G8(roof15), (u8)B8(roof15) };
    far_lift = world_gh((x0 + x1) / 2, (z0 + z1) / 2);
    far_box(x0, z0, x1, z1, sto, wc, pitched && sto <= 2, rc, sto >= 2);
    far_lift = 0;
}
static void draw_buildings(void) {
    int ns = bm_nset(); if (ns > 64) ns = 64;
    int far2 = max_bld;
    for (int i = 0; i < ns; i++) {
        const Settle *s = bm_set(i);
        int sx = ((s->x * BM_BLK + BM_BLK / 2) - fwx0) * TILE, sz = -(((s->y * BM_BLK + BM_BLK / 2) - fwy0) * TILE), rad = (s->r + 1) * BM_BLK * TILE;
        if (!in_cone(sx, sz, rad, far2)) continue;
        for (int c = fbs_lo[i]; c < fbs_hi[i]; c++) {
          int ccx = ((fbc_cx[c] * 4 + 2) * BM_BLK - fwx0) * TILE, ccz = -(((fbc_cy[c] * 4 + 2) * BM_BLK - fwy0) * TILE);
          if (!in_cone(ccx, ccz, CELL_T * TILE * 3 / 4, far2)) continue;
          for (int k = fbc_lo[c]; k < fbc_hi[c]; k++) {
            const FB *f = &fb[k];
            if (f->bx >= world_wbx && f->bx < world_wbx + MW / BM_BLK && f->by >= world_wby && f->by < world_wby + MH / BM_BLK) continue;   // (the map draws its own)
            const u8 *ft = foot[f->lot];
            int tx0 = f->bx * BM_BLK + ft[0] - fwx0, ty0 = f->by * BM_BLK + ft[1] - fwy0, tx1 = f->bx * BM_BLK + ft[2] - fwx0, ty1 = f->by * BM_BLK + ft[3] - fwy0;
            int x0 = tx0 * TILE, x1 = tx1 * TILE, z0 = -ty0 * TILE, z1 = -ty1 * TILE;
            int cx = (x0 + x1) / 2, cz = (z0 + z1) / 2;
            int dx = ABS(cx - feye.vx), dz = ABS(cz - feye.vz), d = MAX(dx, dz) + MIN(dx, dz) / 2;
            if (d > far2) continue;
            int hgt = f->sto * WALL_FULL;
            if (230 * hgt < d * 2) continue;                            // under two pixels tall: lost in the haze
            if (far_detail == 0 && f->sto < 3 && d > M2U(120)) continue;
            if (d > M2U(300)) { far_lift = world_gh(cx, cz); far_dot(x0, z0, x1, z1, f->sto, wallc[f->col % 10]); far_lift = 0; }   // (far: one rectangle each)
            else far_building(f, x0, z0, x1, z1);
          }
        }
    }
}

// ================================================================== trees: cards facing you
#define WT_MAX 700
static u8 wt_x[WT_MAX], wt_y[WT_MAX], wt_k[WT_MAX]; static int nwt, wt_stamp = -1;
static u16 wt_lo[(MH / 4) * (MW / 4) + 1];      // each window cell's run of trees (in cell order)
static void window_trees(void) {
    if (wt_stamp == world_stamp) return;
    wt_stamp = world_stamp; nwt = 0;
    for (int cy = 0; cy < MH / 4; cy++) for (int cx = 0; cx < MW / 4; cx++) {
        wt_lo[cy * (MW / 4) + cx] = (u16)nwt;
        for (int y = cy * 4; y < cy * 4 + 4; y++) for (int x = cx * 4; x < cx * 4 + 4; x++) {
            char c = lifeMap[y][x]; int k = c == '3' ? 0 : c == '6' ? 1 : c == 'g' ? 2 : c == 'u' ? 3 : -1;
            if (k < 0 || nwt >= WT_MAX) continue;
            wt_x[nwt] = (u8)x; wt_y[nwt] = (u8)y; wt_k[nwt] = (u8)k; nwt++;
        }
    }
    wt_lo[(MH / 4) * (MW / 4)] = (u16)nwt;
}

static const short tree_w[4] = { 300, 300, 300, 150 }, tree_h[4] = { 470, 500, 470, 260 };
static int light_tree;                                         // the trees' shade (128 = as painted)
static void tree_card(int x, int z, int k, int big) {
    FV b; fxf(x, -world_gh(x, z), z, &b);
    if (b.vz < ZN * 4 || !fv_in(&b)) return;
    int zw = b.z; if (zw < 64) return;
    int s16_ = (230 << 14) / zw;                                 // pixels a unit (1/16384)
    int hp = (tree_h[k] * big * s16_) >> 16; if (hp < 2) return;
    int wp = (tree_w[k] * big * s16_) >> 16; if (wp < 1) wp = 1;
    int x0 = b.sx - wp / 2, x1 = x0 + wp, y0 = b.sy - hp, y1 = b.sy;
    if (x1 < 0 || x0 >= SCREEN_W || y1 < 0 || y0 >= SCREEN_H) return;
    int f = haze_f(zw), L = light_tree;
    int r = L + (((hz_r * 128 / 160) - L) * f >> 8), g = L + (((hz_g * 128 / 200) - L) * f >> 8), bb = L + (((hz_b * 128 / 120) - L) * f >> 8);
    r = CLAMP(r, 0, 255); g = CLAMP(g, 0, 255); bb = CLAMP(bb, 0, 255);
    const TexRef *t = &tree_tex[k];
    u32 *p = pk(9); if (!p) return;
    p[1] = 0x2C000000u | (u32)r | ((u32)g << 8) | ((u32)bb << 16);
    p[2] = FXY(x0, y0); p[3] = (u32)t->u | ((u32)t->v << 8);
    p[4] = FXY(x1, y0); p[5] = (u32)(t->u + 15) | ((u32)t->v << 8) | ((u32)t->tpage << 16);
    p[6] = FXY(x0, y1); p[7] = (u32)t->u | ((u32)(t->v + 31) << 8);
    p[8] = FXY(x1, y1); p[9] = (u32)(t->u + 15) | ((u32)(t->v + 31) << 8);
    lk(p, 9, far_slot(zw));
}
static void draw_trees(int rx0, int ry0, int rx1, int ry1) {
    if (!far_trees || !tex_ok) return;
    light_tree = CLAMP(gfx_light(0, -ONE / 2, -ONE / 2), 60, 200);
    // the map's own trees past the detail (where they really stand)
    window_trees();
    for (int c = 0; c < (MH / 4) * (MW / 4); c++) {
        if (!wvis[c / (MW / 4)][c % (MW / 4)]) continue;
        for (int i = wt_lo[c]; i < wt_lo[c + 1]; i++) {
            int x = wt_x[i], y = wt_y[i];
            if (x >= rx0 && x <= rx1 && y >= ry0 && y <= ry1) continue;   // (drawn as themselves)
            int wx = x * TILE + TILE / 2, wz = -(y * TILE + TILE / 2);
            int ddx = ABS(wx - feye.vx), ddz = ABS(wz - feye.vz); if (MAX(ddx, ddz) > max_tree) continue;
            tree_card(wx, wz, wt_k[i], 4);
        }
    }
    // past the map: woods and scattered trees, placed by each cell's dice
    int reach = max_tree / (CELL_T * TILE) + 1, pcx = pbx / FCB, pcy = pby / FCB;
    int wx0 = world_wbx * BM_BLK, wy0 = world_wby * BM_BLK;
    for (int cy = pcy - reach; cy <= pcy + reach; cy++) for (int cx = pcx - reach; cx <= pcx + reach; cx++) {
        int ix = cx - fc_x0, iy = cy - fc_y0; if (ix < 0 || iy < 0 || ix >= FC || iy >= FC) continue;
        int t = fcell[iy][ix]; if (t == 0xFF) continue;
        int n = 0, k = 0;
        if (t & 8) { n = 6; k = 0; } else switch (t) {
            case BT_FOREST: n = 26; k = 1; break; case BT_GRASS: n = 7; k = 0; break; case BT_BEACH: n = 5; k = 2; break;
            case BT_DESERT: n = 4; k = 3; break; case BT_ROCKY: n = 4; k = 1; break; default: n = 0;
        }
        if (!n) continue;
        int ctx = cx * CELL_T, cty = cy * CELL_T;
        int ccx = (ctx + CELL_T / 2 - fwx0) * TILE, ccz = -((cty + CELL_T / 2 - fwy0) * TILE);
        if (!in_cone(ccx, ccz, CELL_T * TILE * 3 / 4, max_tree)) continue;
        int overlap = ctx < wx0 + MW && ctx + CELL_T > wx0 && cty < wy0 + MH && cty + CELL_T > wy0;
        u32 seed = bm_seed() ^ 0x7E3Eu;
        for (int j = 0; j < n; j++) {
            u32 hsh = bm_hash(cx * 61 + j, cy * 37 - j, seed);
            int tx = ctx + (int)(hsh % CELL_T), ty = cty + (int)((hsh >> 6) % CELL_T);
            if (overlap && tx >= wx0 && tx < wx0 + MW && ty >= wy0 && ty < wy0 + MH) continue;   // (in the map: its own trees)
            int kk = k; if (t == BT_GRASS && (hsh >> 20) % 3u == 0) kk = 1;
            if (t == BT_FOREST && (hsh >> 21) % 4u == 0) kk = 0;
            tree_card((tx - fwx0) * TILE + TILE / 2, -((ty - fwy0) * TILE + TILE / 2), kk, 4 + (int)((hsh >> 24) & 1));
        }
    }
}

// ================================================================== the sky
void far_sky_sea(u32 top, u32 horizon, u32 sea) {   // (sea: below the horizon, the haze deepens into it; 0: haze all the way down)
    // where the horizon crosses the screen: a point far ahead at the eye's height
    int ax = cam.at.vx - cam.eye.vx, az = cam.at.vz - cam.eye.vz, l = isqrt(ax * ax + az * az); if (l < 1) l = 1;
    far_gte();
    SVECTOR v SVEC_ALIGN; v.vx = (short)(ax * 20000 / l); v.vz = (short)(az * 20000 / l); v.vy = 0; v.pad = 0;
    GTE_FENCE(); gte_ldv0(&v); gte_rtps(); u32 sxy; gte_stsxy(&sxy);
    gfx_set_identity();
    int hy = (s16)(sxy >> 16);
    int tr = top & 255, tg = (top >> 8) & 255, tb = (top >> 16) & 255, br = horizon & 255, bg = (horizon >> 8) & 255, bb = (horizon >> 16) & 255;
    int ytop = hy - 150;                                       // the gradient: 150 px of sky from the horizon up to the deep blue
    u32 *ot = gfx_ot() + OT_SKY;
    #define LINKP(p, n) { (p)[0] = ((u32)(n) << 24) | (*ot & 0xffffff); *ot = (u32)(p) & 0xffffff; }
    if (hy > 0) {                                              // the sky itself
        int y0 = MAX(ytop, 0), y1 = MIN(hy, SCREEN_H);
        int t0 = ytop >= 0 ? 0 : (-ytop) * 256 / 150, t1 = (y1 - ytop) * 256 / 150; t1 = MIN(t1, 256);
        u32 c0 = (u32)(tr + (((br - tr) * t0) >> 8)) | ((u32)(tg + (((bg - tg) * t0) >> 8)) << 8) | ((u32)(tb + (((bb - tb) * t0) >> 8)) << 16);
        u32 c1 = (u32)(tr + (((br - tr) * t1) >> 8)) | ((u32)(tg + (((bg - tg) * t1) >> 8)) << 8) | ((u32)(tb + (((bb - tb) * t1) >> 8)) << 16);
        if (y1 > y0) { u32 *p = (u32 *)gfx_alloc(9 * 4); if (p) {
            p[1] = 0x38000000u | c0; p[2] = FXY(0, y0); p[3] = c0; p[4] = FXY(SCREEN_W, y0); p[5] = c1; p[6] = FXY(0, y1); p[7] = c1; p[8] = FXY(SCREEN_W, y1);
            LINKP(p, 8) } }
        if (ytop > 0) { u32 *p = (u32 *)gfx_alloc(6 * 4); if (p) {          // above the gradient: the deep blue
            p[1] = 0x28000000u | top; p[2] = FXY(0, 0); p[3] = FXY(SCREEN_W, 0); p[4] = FXY(0, MIN(ytop, SCREEN_H)); p[5] = FXY(SCREEN_W, MIN(ytop, SCREEN_H));
            LINKP(p, 5) } }
    }
    if (sea && hy < SCREEN_H) {   // the open sea: from the haze at the horizon, deepening over 70 px, then the deep colour
        // (linked first: the table draws the last linked first, so the bands lie over the sea's fill)
        {   // waves: bands of lighter and darker water at geometric distances (they bunch up towards the horizon), drifting with the swell
            static int swell; swell++;
            int hh = -cam.eye.vy; if (hh < 60) hh = 60; if (hh > 4000) hh = 4000;
            int sr = sea & 255, sg = (sea >> 8) & 255, sb = (sea >> 16) & 255;
            int ph = (swell * 3) & 255;                                   // 0..255 = one band step
            int yprev = SCREEN_H + 40;
            for (int k = 0; k < 21; k++) {
                int bs = 700 << (k / 3), dd = bs + bs * ((k % 3) * 170 + ((ph * 170) >> 8)) / 510;   // (three bands to each doubling of distance)
                int y = hy + (230 * hh) / dd;
                if (y >= yprev) continue;
                if (!(k & 1)) { int y0 = MAX(y, hy + 1), y1 = MIN(yprev, SCREEN_H);   // the lit bands
                    if (y1 > y0 && y0 < SCREEN_H) { int dl = 13 - k / 2; if (dl < 4) dl = 4;
                        u32 c = (u32)MIN(255, sr + dl) | ((u32)MIN(255, sg + dl + 6) << 8) | ((u32)MIN(255, sb + dl + 8) << 16);
                        u32 *q = (u32 *)gfx_alloc(6 * 4); if (q) { q[1] = 0x28000000u | c; q[2] = FXY(0, y0); q[3] = FXY(SCREEN_W, y0); q[4] = FXY(0, y1); q[5] = FXY(SCREEN_W, y1); LINKP(q, 5) } } }
                yprev = y;
            }
        }
        int yb = hy + 70, y0 = MAX(hy, 0), y1 = MIN(yb, SCREEN_H);
        if (y1 > y0) { u32 *p = (u32 *)gfx_alloc(9 * 4); if (p) {
            int t0 = (y0 - hy) * 256 / 70, t1 = (y1 - hy) * 256 / 70;
            int sr = sea & 255, sg = (sea >> 8) & 255, sb = (sea >> 16) & 255;
            u32 c0 = (u32)(br + (((sr - br) * t0) >> 8)) | ((u32)(bg + (((sg - bg) * t0) >> 8)) << 8) | ((u32)(bb + (((sb - bb) * t0) >> 8)) << 16);
            u32 c1 = (u32)(br + (((sr - br) * t1) >> 8)) | ((u32)(bg + (((sg - bg) * t1) >> 8)) << 8) | ((u32)(bb + (((sb - bb) * t1) >> 8)) << 16);
            p[1] = 0x38000000u | c0; p[2] = FXY(0, y0); p[3] = c0; p[4] = FXY(SCREEN_W, y0); p[5] = c1; p[6] = FXY(0, y1); p[7] = c1; p[8] = FXY(SCREEN_W, y1);
            LINKP(p, 8) } }
        if (yb < SCREEN_H) { u32 *p = (u32 *)gfx_alloc(6 * 4); if (p) {
            p[1] = 0x28000000u | sea; p[2] = FXY(0, MAX(yb, 0)); p[3] = FXY(SCREEN_W, MAX(yb, 0)); p[4] = FXY(0, SCREEN_H); p[5] = FXY(SCREEN_W, SCREEN_H);
            LINKP(p, 5) } }
    } else if (hy < SCREEN_H) { int y0 = MAX(hy, 0); u32 *p = (u32 *)gfx_alloc(6 * 4); if (p) {   // below the horizon (the land covers it): haze
        p[1] = 0x28000000u | horizon; p[2] = FXY(0, y0); p[3] = FXY(SCREEN_W, y0); p[4] = FXY(0, SCREEN_H); p[5] = FXY(SCREEN_W, SCREEN_H);
        LINKP(p, 5) } }
    #undef LINKP
}
void far_sky(u32 top, u32 horizon) { far_sky_sea(top, horizon, 0); }

// ================================================================== the frame
static int far_on_frame;
int far_begin(u32 horizon) {
    far_prims = 0; far_on_frame = 0; far_nclip = far_ncone = far_nquad = 0;
    if (world_zone != ZONE_CITY || world_wbx < 0 || far_view <= 0) return 0;
    if (!tex_ok) far_tex();
    hz_r = horizon & 255; hz_g = (horizon >> 8) & 255; hz_b = (horizon >> 16) & 255;
    fwx0 = world_wbx * BM_BLK; fwy0 = world_wby * BM_BLK;
    { int ptx = (cam.at.vx >> 8) + fwx0, pty = (-cam.at.vz >> 8) + fwy0; pbx = ptx / BM_BLK; pby = pty / BM_BLK; }
    cells_follow(24);
    if (!far_setup()) return 0;
    far_gte(); far_on_frame = 1;
    return 1;
}
void far_draw(int rx0, int ry0, int rx1, int ry1) {
    if (!far_on_frame) return;
    far_gte();
    int t0 = gfx_hb(); draw_land(rx0, ry0, rx1, ry1); int t1 = gfx_hb(); int p1 = far_prims;
    draw_buildings(); int t2 = gfx_hb(); int p2 = far_prims;
    draw_trees(rx0, ry0, rx1, ry1); int t3 = gfx_hb();
    far_hb[0] = t1 - t0; far_hb[1] = t2 - t1; far_hb[2] = t3 - t2; far_np[0] = p1; far_np[1] = p2 - p1; far_np[2] = far_prims - p2;
    gfx_set_identity();
}
