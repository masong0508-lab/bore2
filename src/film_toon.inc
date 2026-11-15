// film_toon.inc - the film's own people (included by cutscene.c, so it lives in CUT.OVL only; the game's Bores are untouched).
// A figure is a short list of SOFT SHAPES: lofts (rings of a given radius along an axis, 6 to 12 sides) that make an egg of a head, a barrel of a body, a tapered
// limb or a mitten of a hand. Each is drawn twice, the way a cel is inked: an ink shell (the same shape a little fatter, only its far side, set back) and then the
// shape itself in two or three FLAT tones picked by the light (one key light per shot, so a hero shot can be lit from behind with a rim). The faces are not
// textures: they are drawn polygons laid on the head's surface (eyes with a highlight, lashes, brows, a mouth that changes with the line), so they stay crisp however
// close the camera comes. Everything is body space (x sideways, y UP, z forward = the way the face looks, units of the game: a Bore is 384 tall) and goes to the GTE
// as (x, -y, z). Every bit of it is one pure function of the pose and the clock.
typedef struct { int x, y, z; } P3;
typedef struct { s16 m[3][3]; } M3;                 // columns are the axes: body = c + M * local
static inline P3 p3(int x, int y, int z) { P3 p = { x, y, z }; return p; }
static inline P3 padd(P3 a, P3 b) { return p3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline P3 psub(P3 a, P3 b) { return p3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline P3 pmul(P3 a, int k) { return p3(a.x * k >> 12, a.y * k >> 12, a.z * k >> 12); }     // k: 4096 = 1
static inline int pdot(P3 a, P3 b) { return (a.x * b.x + a.y * b.y + a.z * b.z) >> 12; }
static inline P3 pcross(P3 a, P3 b) { return p3((a.y * b.z - a.z * b.y) >> 12, (a.z * b.x - a.x * b.z) >> 12, (a.x * b.y - a.y * b.x) >> 12); }
static P3 pnorm(P3 v) { int l = isqrt(v.x * v.x + v.y * v.y + v.z * v.z); if (!l) return p3(0, 4096, 0); return p3(v.x * 4096 / l, v.y * 4096 / l, v.z * 4096 / l); }
static inline int t_sin(int a) { return isin(a & 4095); }
static inline int t_cos(int a) { return icos(a & 4095); }
static M3 m_rx(int a) { int s = t_sin(a), c = t_cos(a); M3 r = { { { 4096, 0, 0 }, { 0, c, -s }, { 0, s, c } } }; return r; }
static M3 m_ry(int a) { int s = t_sin(a), c = t_cos(a); M3 r = { { { c, 0, s }, { 0, 4096, 0 }, { -s, 0, c } } }; return r; }
static M3 m_rz(int a) { int s = t_sin(a), c = t_cos(a); M3 r = { { { c, -s, 0 }, { s, c, 0 }, { 0, 0, 4096 } } }; return r; }
static M3 m_id(void) { M3 r = { { { 4096, 0, 0 }, { 0, 4096, 0 }, { 0, 0, 4096 } } }; return r; }
static M3 m_mul(const M3 *a, const M3 *b) { M3 r; for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) r.m[i][j] = (s16)((a->m[i][0] * b->m[0][j] + a->m[i][1] * b->m[1][j] + a->m[i][2] * b->m[2][j]) >> 12); return r; }
static P3 m_app(const M3 *m, P3 v) { return p3((m->m[0][0] * v.x + m->m[0][1] * v.y + m->m[0][2] * v.z) >> 12, (m->m[1][0] * v.x + m->m[1][1] * v.y + m->m[1][2] * v.z) >> 12, (m->m[2][0] * v.x + m->m[2][1] * v.y + m->m[2][2] * v.z) >> 12); }
static M3 m_cols(P3 u, P3 v, P3 w) { M3 r = { { { (s16)u.x, (s16)v.x, (s16)w.x }, { (s16)u.y, (s16)v.y, (s16)w.y }, { (s16)u.z, (s16)v.z, (s16)w.z } } }; return r; }
static inline P3 m_col(const M3 *m, int k) { return p3(m->m[0][k], m->m[1][k], m->m[2][k]); }

// ---- the shapes: rings from the bottom (y = -1024) to the top (1024); r the radius (1024 = the instance's half width), z a forward shift of the ring (1024 = its half depth) ----
typedef struct { s16 y, r, z; } Ring;
typedef struct { u8 nr; const Ring *r; } Shape;
#define SHP(n, ...) static const Ring n##_r[] = { __VA_ARGS__ }; static const Shape n = { sizeof n##_r / sizeof n##_r[0], n##_r }
SHP(SH_HEAD,  { -1024, 0, 0 }, { -800, 500, 0 }, { -450, 880, 0 }, { -40, 1024, 0 }, { 400, 980, 0 }, { 760, 780, 0 }, { 960, 420, 0 }, { 1024, 0, 0 });   // an egg: wide at the cheeks, a soft chin
SHP(SH_HEADS, { -1024, 0, 0 }, { -800, 360, 0 }, { -450, 770, 0 }, { -40, 1010, 0 }, { 400, 990, 0 }, { 760, 790, 0 }, { 960, 430, 0 }, { 1024, 0, 0 });   // a sleeker chin (Missy)
SHP(SH_BLOB,  { -1024, 0, 0 }, { -520, 880, 0 }, { 520, 880, 0 }, { 1024, 0, 0 });                                                                          // a capsule: hands, feet, joints, ears
SHP(SH_BALL,  { -1024, 0, 0 }, { -620, 760, 0 }, { 0, 1024, 0 }, { 620, 760, 0 }, { 1024, 0, 0 });
SHP(SH_JNT,   { -1024, 0, 0 }, { 0, 1024, 0 }, { 1024, 0, 0 });                                                                                  // a joint: two cones, six sided
SHP(SH_TUBE,  { -1024, 1024, 0 }, { 1024, 1024, 0 });
SHP(SH_BARREL, { -1024, 800, 0 }, { -540, 950, 40 }, { 80, 1024, 60 }, { 640, 980, 0 }, { 1024, 640, 0 });                                                  // Chad: a big friendly barrel
SHP(SH_WAIST, { -1024, 980, 0 }, { -420, 700, 0 }, { 120, 760, 0 }, { 700, 880, 30 }, { 1024, 520, 0 });                                                  // Missy: a narrow waist, a fitted jacket
SHP(SH_BOX,   { -1024, 860, 0 }, { -300, 1000, 20 }, { 560, 1000, 0 }, { 1024, 700, 0 });                                                                  // the Intern's polo, a little boxy
SHP(SH_SKIRT, { -1024, 1024, 0 }, { 1024, 640, 0 });
SHP(SH_DISC,  { -1024, 1024, 0 }, { 1024, 1024, 0 });                                                                                                     // a tray (flat)
SHP(SH_CAP,   { -300, 1024, 0 }, { 200, 1000, 0 }, { 640, 820, 0 }, { 900, 520, 0 }, { 1024, 0, 0 });                                                     // the hair's dome: open below and round the front
SHP(SH_LOCK,  { -1024, 360, 0 }, { -300, 800, 0 }, { 500, 1024, 0 }, { 1024, 780, 0 });                                                                   // a lock of hair (a long drop)

// ---- palettes: three flat tones (shadow, base, light) and where the light begins and the shadow ends ----
typedef struct { u32 c[3]; s16 t0, t1; } Pal;     // 0xBBGGRR; the tone is 0 below t0, 2 from t1 (t1 = 5000: no light tone), else 1 (the light's dot with the normal, 4096 = 1)
#define BGR(r, g, b) ((u32)(r) | ((u32)(g) << 8) | ((u32)(b) << 16))
#define PALX(a, b, c, t0, t1) { { a, b, c }, t0, t1 }

typedef struct { const Shape *sh; P3 c; M3 m; s16 hx, hy, hz, k0, k1; u8 ns, pal; s8 bias; u8 flag; s16 a0, span; } Inst;
#define IF_NOOL 1      // no ink shell
#define IF_FACE 2      // (reserved)
#define MAXI 56
static Inst il[MAXI]; static int nil_;
static P3 g_L, g_R;                    // the key light and the rim light in body space (4096 = 1)
static int g_rim, g_ol, g_lod;   // g_lod: 1, 2 = fewer sides for a figure further away
static int g_dummy_;                // g_rim: the rim tone is on; g_ol: the ink shell's thickness, units
static u32 g_ink, g_rimc;              // the ink colour, the rim colour
static const Pal *g_pal;
static s16 cs12[13], sn12[13];
static SVECTOR vbuf[128] SVEC_ALIGN; static XV xbuf[128];

static Inst il_dummy;
static Inst *inst_add(const Shape *sh, P3 c, const M3 *m, int hx, int hy, int hz, int ns, int pal) {     // (a full list hands out a dummy, so a caller can always set a field)
    Inst *I = nil_ >= MAXI ? &il_dummy : &il[nil_++]; I->sh = sh; I->c = c; I->m = *m; I->hx = (s16)hx; I->hy = (s16)hy; I->hz = (s16)hz; I->k0 = I->k1 = 1024; I->ns = (u8)ns; I->pal = (u8)pal; I->bias = 0; I->flag = 0; I->a0 = 0; I->span = 4096;
    return I;
}
static void ax_frame(P3 v, M3 *m) {     // a frame whose y runs along v (a limb)
    P3 ref = ABS(v.x) > 2900 ? p3(0, 0, 4096) : p3(4096, 0, 0);
    P3 u = pnorm(psub(ref, pmul(v, pdot(ref, v)))), w = pcross(u, v);       // (a right-handed frame: u x v = w)
    *m = m_cols(u, v, w);
}
static Inst *add_tube(P3 a, P3 b, int r0, int r1, int pal, int ns) {
    P3 d = psub(b, a); int l = isqrt(d.x * d.x + d.y * d.y + d.z * d.z); if (l < 2) return &il_dummy;
    M3 m; ax_frame(pnorm(d), &m);
    int rm = MAX(r0, r1);
    Inst *I = inst_add(&SH_TUBE, p3((a.x + b.x) >> 1, (a.y + b.y) >> 1, (a.z + b.z) >> 1), &m, rm, l / 2, rm, ns, pal);
    I->k0 = (s16)(r0 * 1024 / rm); I->k1 = (s16)(r1 * 1024 / rm);
    return I;
}
static Inst *add_blob(const Shape *sh, P3 c, const M3 *m, int hx, int hy, int hz, int ns, int pal) { return inst_add(sh, c, m, hx, hy, hz, ns, pal); }

static void vis_inst_verts(const Inst *I, int grow, int nv, int ns, const s16 *sn, const s16 *cs, SVECTOR *out) {
    const Shape *sh = I->sh;
    int hx = I->hx + grow, hy = I->hy + grow, hz = I->hz + grow;
    P3 U = m_col(&I->m, 0), V = m_col(&I->m, 1), Wv = m_col(&I->m, 2);
    for (int i = 0; i < sh->nr; i++) {
        int y = sh->r[i].y, kk = I->k0 + (((I->k1 - I->k0) * (y + 1024)) >> 11), rr = sh->r[i].r * kk >> 10, zz = sh->r[i].z;
        int ly = y * hy >> 10, lz = zz * hz >> 10, lx = hx * rr >> 10, lw = hz * rr >> 10;
        if (grow) { if (i == 0) ly -= grow; if (i == sh->nr - 1) ly += grow; }
        int rx = I->c.x * 4 + ((V.x * ly + Wv.x * lz) >> 10), ry = I->c.y * 4 + ((V.y * ly + Wv.y * lz) >> 10), rz = I->c.z * 4 + ((V.z * ly + Wv.z * lz) >> 10);
        int ax = U.x * lx >> 10, ay = U.y * lx >> 10, az = U.z * lx >> 10, bx = Wv.x * lw >> 10, by = Wv.y * lw >> 10, bz = Wv.z * lw >> 10;
        for (int j = 0; j < nv; j++) {
            int s = sn[j], c = cs[j];
            SVECTOR *o = &out[i * nv + j];
            o->vx = (short)((rx + ((s * ax + c * bx) >> 12) + 2) >> 2); o->vy = (short)(-((ry + ((s * ay + c * by) >> 12) + 2) >> 2)); o->vz = (short)((rz + ((s * az + c * bz) >> 12) + 2) >> 2);
        }
    }
    (void)ns;
}
static inline int xcross(const XV *a, const XV *b, const XV *c) { return (b->x - a->x) * (c->y - a->y) - (c->x - a->x) * (b->y - a->y); }
static inline int xoff(const XV *a, const XV *b, const XV *c, const XV *d) { return xv_outcode(a->x, a->y) & xv_outcode(b->x, b->y) & xv_outcode(c->x, c->y) & xv_outcode(d->x, d->y); }
#define FRONT_POS 1
static void inst_draw(const Inst *I, int ol) {
    const Shape *sh = I->sh; int lod = g_lod, head = sh == &SH_HEAD || sh == &SH_HEADS;   // (a head keeps more sides: the face is drawn on its surface)
    int nr = sh->nr, ns = head ? (lod == 2 ? 8 : lod == 1 ? 10 : I->ns) : lod == 2 ? MAX(4, I->ns / 2) : lod == 1 ? MAX(5, I->ns * 3 / 4) : I->ns, open = I->span < 4096;
    if (ol && !head && ns > 6) ns = ns * 2 / 3;       // (the ink behind needs fewer sides)
    int nv = ns + open;
    s16 sn[13], cs[13], smn[12], cmn[12];
    for (int j = 0; j < nv; j++) { int a = I->a0 + I->span * j / ns; sn[j] = (s16)t_sin(a); cs[j] = (s16)t_cos(a); }
    for (int j = 0; j < ns; j++) { int a = I->a0 + (I->span * (2 * j + 1)) / (2 * ns); smn[j] = (s16)t_sin(a); cmn[j] = (s16)t_cos(a); }
    vis_inst_verts(I, ol ? g_ol : 0, nv, ns, sn, cs, vbuf);
    gfx_xform(vbuf, xbuf, nr * nv);
    const Pal *pl = &g_pal[I->pal];
    // the light in the instance's own frame
    P3 U = m_col(&I->m, 0), V = m_col(&I->m, 1), Wv = m_col(&I->m, 2);
    int Llx = pdot(U, g_L), Lly = pdot(V, g_L), Llz = pdot(Wv, g_L), Rlx = pdot(U, g_R), Rly = pdot(V, g_R), Rlz = pdot(Wv, g_R);
    s16 sj[12], sr[12];
    for (int j = 0; j < ns; j++) { sj[j] = (s16)((smn[j] * Llx + cmn[j] * Llz) >> 12); sr[j] = (s16)((smn[j] * Rlx + cmn[j] * Rlz) >> 12); }
    gfx_set_zbias(I->bias + (ol ? -2 : 0));
    for (int i = 0; i < nr - 1; i++) {
        int p0 = sh->r[i].r == 0, p1 = sh->r[i + 1].r == 0;
        int dy = (sh->r[i + 1].y - sh->r[i].y) * I->hy >> 10, dr = ((sh->r[i + 1].r - sh->r[i].r) * I->hx >> 10) * ((I->k0 + I->k1) >> 1) >> 10;
        int ln = isqrt(dy * dy + dr * dr); int nrb = ln ? dy * 4096 / ln : 4096, nyb = ln ? -dr * 4096 / ln : 0;
        int base = (nyb * Lly) >> 12, baseR = (nyb * Rly) >> 12;
        for (int j = 0; j < ns; j++) {
            int j1 = open ? j + 1 : (j + 1 == ns ? 0 : j + 1);
            const XV *a = &xbuf[i * nv + j], *b = &xbuf[i * nv + j1], *c = &xbuf[(i + 1) * nv + j], *d = &xbuf[(i + 1) * nv + j1];
            if (a->z < Z_NEAR || b->z < Z_NEAR || c->z < Z_NEAR || d->z < Z_NEAR) continue;
            int cr = p0 ? xcross(a, d, c) : xcross(a, b, c);
            if (!cr) continue;
            int front = FRONT_POS ? cr > 0 : cr < 0;
            if (ol ? front : !front) continue;
            if (xoff(a, b, c, d)) continue;
            u32 col;
            if (ol) col = g_ink;
            else {
                int dd = ((nrb * sj[j]) >> 12) + base, tone = dd >= pl->t1 ? 2 : dd >= pl->t0 ? 1 : 0;
                col = pl->c[tone];
                if (g_rim && ((nrb * sr[j]) >> 12) + baseR > 2300) col = g_rimc;
            }
            if (p0) gfx_put_f3(a, c, d, col); else if (p1) gfx_put_f3(a, b, c, col); else gfx_put_f4(a, b, c, d, col);
        }
    }
    gfx_set_zbias(0);
}
