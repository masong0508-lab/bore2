// gfx_fast.h: the fast path, inline. A mesh's corners are transformed once (gfx_xform: three at a time on the GTE) into XVs, and
// faces are emitted from those: the back-face test on the CPU from the screen positions, the packet written as whole words, the
// colours already graded (gfx_grade_*). Inline, so a mesh loop pays no call or stack-argument cost per face.
#ifndef GFX_FAST_H
#define GFX_FAST_H
extern char *gfx_pk_next, *gfx_pk_end;
extern u32 *gfx_ot_cur;
extern int gfx_zbias, gfx_zbase, gfx_ot_max, gfx_fog_on, gfx_floor_pass, gfx_frame_prims, gfx_overflow;
u32 gfx_fog_apply(u32 c, int otz);

static inline int xv_outcode(int x, int y) { return (x < 0 ? 1 : x >= SCREEN_W ? 2 : 0) | (y < 0 ? 4 : y >= SCREEN_H ? 8 : 0); }
static inline int xv_cull(const XV *a, const XV *b, const XV *c) {   // 1 = facing away (the GTE's NCLIP, on the CPU)
    return (b->x - a->x) * (c->y - a->y) - (c->x - a->x) * (b->y - a->y) <= 0;
}
// 1 if the quad can show (in front of the near plane, facing the eye if cull, on the screen)
static inline int xv_vis4(const XV *a, const XV *b, const XV *c, const XV *d, int cull) {
    if (a->z < Z_NEAR || b->z < Z_NEAR || c->z < Z_NEAR || d->z < Z_NEAR) return 0;
    if (cull && xv_cull(a, b, c)) return 0;
    return (xv_outcode(a->x, a->y) & xv_outcode(b->x, b->y) & xv_outcode(c->x, c->y) & xv_outcode(d->x, d->y)) == 0;
}
static inline int xv_vis3(const XV *a, const XV *b, const XV *c, int cull) {
    if (a->z < Z_NEAR || b->z < Z_NEAR || c->z < Z_NEAR) return 0;
    if (cull && xv_cull(a, b, c)) return 0;
    return (xv_outcode(a->x, a->y) & xv_outcode(b->x, b->y) & xv_outcode(c->x, c->y)) == 0;
}
static inline u32 *xv_alloc(int words) {
    u32 *p = (u32 *)gfx_pk_next;
    if ((char *)(p + words) > gfx_pk_end) { gfx_overflow++; return 0; }
    gfx_pk_next = (char *)(p + words);
    return p;
}
static inline void xv_link(u32 *q, int n, int otz) {
    int i;
    if (gfx_floor_pass) i = OT_FLOOR;
    else { i = (otz >> OT_SHIFT) - gfx_zbias - gfx_zbase; if (i < 1) i = 1; else if (i > gfx_ot_max) i = gfx_ot_max; }   // (zbase, ot_max: a person rendered into its own small table)
    u32 *o = gfx_ot_cur + i;
    q[0] = ((u32)n << 24) | (*o & 0xffffff); *o = (u32)q & 0xffffff;
    gfx_frame_prims++;
}
#define XYW(v) (*(const u32 *)&(v)->x)
// the "put" functions emit without testing (call xv_vis4 first: the colours can then be worked out only for faces that show)
static inline void gfx_put_f4(const XV *a, const XV *b, const XV *c, const XV *d, u32 col) {
    u32 *q = xv_alloc(6); if (!q) return;
    int otz = (a->z + b->z + c->z + d->z) >> 4;
    if (gfx_fog_on) col = gfx_fog_apply(col, otz);
    q[1] = col | 0x28000000u; q[2] = XYW(a); q[3] = XYW(b); q[4] = XYW(c); q[5] = XYW(d);
    xv_link(q, 5, otz);
}
static inline void gfx_put_f3(const XV *a, const XV *b, const XV *c, u32 col) {
    u32 *q = xv_alloc(5); if (!q) return;
    int otz = ((a->z + b->z + c->z) * 0x155) >> 12;
    if (gfx_fog_on) col = gfx_fog_apply(col, otz);
    q[1] = col | 0x20000000u; q[2] = XYW(a); q[3] = XYW(b); q[4] = XYW(c);
    xv_link(q, 4, otz);
}
static inline void gfx_put_g4(const XV *a, const XV *b, const XV *c, const XV *d, u32 c0, u32 c1, u32 c2, u32 c3) {
    u32 *q = xv_alloc(9); if (!q) return;
    int otz = (a->z + b->z + c->z + d->z) >> 4;
    if (gfx_fog_on) { c0 = gfx_fog_apply(c0, otz); c1 = gfx_fog_apply(c1, otz); c2 = gfx_fog_apply(c2, otz); c3 = gfx_fog_apply(c3, otz); }
    q[1] = c0 | 0x38000000u; q[2] = XYW(a); q[3] = c1; q[4] = XYW(b); q[5] = c2; q[6] = XYW(c); q[7] = c3; q[8] = XYW(d);
    xv_link(q, 8, otz);
}
// uv as a box: a = (u0,v0) TL, b = (u1,v0) TR, c = (u0,v1) BL, d = (u1,v1) BR. tp = tpage << 16, cl = clut << 16
static inline void gfx_put_gt4(const XV *a, const XV *b, const XV *c, const XV *d, u32 c0, u32 c1, u32 c2, u32 c3, u32 tp, u32 cl, int u0, int v0, int u1, int v1) {
    u32 *q = xv_alloc(13); if (!q) return;
    int otz = (a->z + b->z + c->z + d->z) >> 4;
    if (gfx_fog_on) { c0 = gfx_fog_apply(c0, otz); c1 = gfx_fog_apply(c1, otz); c2 = gfx_fog_apply(c2, otz); c3 = gfx_fog_apply(c3, otz); }
    u32 top = (u32)v0 << 8, bot = (u32)v1 << 8;
    q[1] = c0 | 0x3c000000u; q[2] = XYW(a); q[3] = ((u32)u0 | top) | cl;
    q[4] = c1;               q[5] = XYW(b); q[6] = ((u32)u1 | top) | tp;
    q[7] = c2;               q[8] = XYW(c); q[9] = (u32)u0 | bot;
    q[10] = c3;              q[11] = XYW(d); q[12] = (u32)u1 | bot;
    xv_link(q, 12, otz);
}
// uv per corner, packed u | v << 8
static inline void gfx_put_ft4(const XV *a, const XV *b, const XV *c, const XV *d, u32 col, u32 tp, u32 cl, u32 uva, u32 uvb, u32 uvc, u32 uvd, int semi) {
    u32 *q = xv_alloc(10); if (!q) return;
    int otz = (a->z + b->z + c->z + d->z) >> 4;
    if (gfx_fog_on) col = gfx_fog_apply(col, otz);
    q[1] = col | (semi ? 0x2e000000u : 0x2c000000u);
    q[2] = XYW(a); q[3] = uva | cl; q[4] = XYW(b); q[5] = uvb | tp; q[6] = XYW(c); q[7] = uvc; q[8] = XYW(d); q[9] = uvd;
    xv_link(q, 9, otz);
}
#undef XYW
// the testing versions (return 1 if drawn)
static inline int gfx_emit_f4(const XV *a, const XV *b, const XV *c, const XV *d, u32 col, int cull) {
    if (!xv_vis4(a, b, c, d, cull)) return 0; gfx_put_f4(a, b, c, d, col); return 1;
}
static inline int gfx_emit_f3(const XV *a, const XV *b, const XV *c, u32 col, int cull) {
    if (!xv_vis3(a, b, c, cull)) return 0; gfx_put_f3(a, b, c, col); return 1;
}
static inline int gfx_emit_g4(const XV *a, const XV *b, const XV *c, const XV *d, u32 c0, u32 c1, u32 c2, u32 c3, int cull) {
    if (!xv_vis4(a, b, c, d, cull)) return 0; gfx_put_g4(a, b, c, d, c0, c1, c2, c3); return 1;
}
static inline int gfx_emit_gt4(const XV *a, const XV *b, const XV *c, const XV *d, u32 c0, u32 c1, u32 c2, u32 c3,
                               u16 tpage, u16 clut, int u0, int v0, int u1, int v1, int cull) {
    if (!xv_vis4(a, b, c, d, cull)) return 0; gfx_put_gt4(a, b, c, d, c0, c1, c2, c3, (u32)tpage << 16, (u32)clut << 16, u0, v0, u1, v1); return 1;
}
static inline int gfx_emit_ft4(const XV *a, const XV *b, const XV *c, const XV *d, u32 col, u16 tpage, u16 clut,
                               int ua, int va, int ub, int vb, int uc, int vc, int ud, int vd, int cull, int semi) {
    if (!xv_vis4(a, b, c, d, cull)) return 0;
    gfx_put_ft4(a, b, c, d, col, (u32)tpage << 16, (u32)clut << 16, (u32)ua | ((u32)va << 8), (u32)ub | ((u32)vb << 8), (u32)uc | ((u32)vc << 8), (u32)ud | ((u32)vd << 8), semi);
    return 1;
}
#endif
