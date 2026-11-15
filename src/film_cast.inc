// film_cast.inc - the three people of "TV SHOW & TELL": CHAD (the host: a big friendly barrel of a man), MISSY TELLER (the judge: tall, sleek, cool) and the INTERN (small, round, all eyes).
// Their skeleton (fk), how they are built from soft shapes for the pose they are in (build_*), and their faces (face_draw: polygons on the head, never a texture).
// Pose angles are 4096 to a turn; every number says what it does:
//   arm: swing of the upper arm forward (+) or back (-), 1024 = level, 2048 = straight up.   aroll: out to the side (+ for both arms), 1024 = level with the shoulder.
//   elbow: how far the forearm folds up and forward (+, 0 = straight; an elbow never bends back).   leg, lroll: the same for the thigh;  knee: the shin folds back (+).
//   lean: the body bows forward (+).  twist: the upper body turns on the hips.  nod: the head tips down (+).  tilt: the head leans to one side.  drop: the hips sink (a squat), air: the whole body hops up.
typedef struct { s16 arm[2], aroll[2], elbow[2], leg[2], lroll[2], knee[2], lean, twist, nod, tilt, drop, air; } FPose;
typedef struct { s16 thigh, shin, torso, neck, upA, foA, hw, sw, shy, headCY, ank; } Body;
typedef struct { P3 Hh, N, S[2], E[2], Wr[2], Hp[2], K[2], A[2], HC; M3 BF, HF; } Joints;
static const int sgn[2] = { 1, -1 };       // the two sides: 0 is the +x side, 1 the -x side
static inline P3 pm(P3 v, int len) { return p3(v.x * len >> 12, v.y * len >> 12, v.z * len >> 12); }   // a unit vector (4096) of this length

static P3 limb_dir(int p, int r, int s) {   // the way a limb hangs from its joint: swung forward by p, out to the side by r (s: +1 or -1, the side)
    int cp = t_cos(p), sp = t_sin(p), th = s > 0 ? r : -r;
    return p3(cp * t_sin(th) >> 12, -(cp * t_cos(th) >> 12), sp);
}
static P3 bend(P3 d, int e, int refz) {     // fold a limb at its joint, toward the front (refz 1) or the back (-1)
    P3 ref = p3(0, 0, 4096 * refz), t = psub(ref, pmul(d, pdot(ref, d)));
    if (ABS(t.x) + ABS(t.y) + ABS(t.z) < 1200) { ref = p3(0, 4096, 0); t = psub(ref, pmul(d, pdot(ref, d))); }
    return padd(pmul(d, t_cos(e)), pmul(pnorm(t), t_sin(e)));
}
static void fk(const Body *B, const FPose *p, Joints *J) {
    P3 Hh = p3(0, B->thigh + B->shin - p->drop, 0);
    M3 a = m_rx(p->lean), b = m_ry(p->twist), BF = m_mul(&a, &b);
    J->Hh = Hh; J->BF = BF;
    J->N = padd(Hh, m_app(&BF, p3(0, B->torso, 0)));
    for (int s = 0; s < 2; s++) {
        int g = sgn[s];
        J->Hp[s] = padd(Hh, p3(g * B->hw, 0, 0));
        P3 dl = limb_dir(p->leg[s], p->lroll[s], g), ds = bend(dl, p->knee[s], -1);
        J->K[s] = padd(J->Hp[s], pm(dl, B->thigh));
        J->A[s] = padd(J->K[s], pm(ds, B->shin));
        J->S[s] = padd(Hh, m_app(&BF, p3(g * B->sw, B->torso - B->shy, 0)));
        P3 da = limb_dir(p->arm[s], p->aroll[s], g), df = bend(da, p->elbow[s], 1);
        J->E[s] = padd(J->S[s], m_app(&BF, pm(da, B->upA)));
        J->Wr[s] = padd(J->E[s], m_app(&BF, pm(df, B->foA)));
    }
    M3 c = m_rx(p->nod), d = m_rz(p->tilt), e = m_mul(&BF, &c); J->HF = m_mul(&e, &d);
    P3 nt = padd(J->N, m_app(&BF, p3(0, B->neck, 0)));
    J->HC = padd(nt, m_app(&J->HF, p3(0, B->headCY, 0)));
    int lo = MIN(J->A[0].y, J->A[1].y) - B->ank, sh = -lo + p->air;       // plant the lower foot on the floor
    J->Hh.y += sh; J->N.y += sh; J->HC.y += sh;
    for (int s = 0; s < 2; s++) { J->S[s].y += sh; J->E[s].y += sh; J->Wr[s].y += sh; J->Hp[s].y += sh; J->K[s].y += sh; J->A[s].y += sh; }
}

// ---- the face: polygons laid on the head's surface. Sizes and places are in world units from the middle of the face (x to one side, y up from the head's centre) ----
typedef struct { const Shape *sh; P3 c; M3 m; int hx, hy, hz, iu, iv, ird[10]; } HeadG;
static HeadG hg; static int hu_;
static void fpt(int xw, int yw, int push, SVECTOR *o) {
    int u = (xw * hg.iu) >> 16, v = (yw * hg.iv) >> 16; v = CLAMP(v, -1020, 1020);     // (reciprocals worked out once per head: no divisions per corner)
    const Ring *r = hg.sh->r; int n = hg.sh->nr, k = 0; while (k < n - 2 && v > r[k + 1].y) k++;
    int rr = r[k].r + (((r[k + 1].r - r[k].r) * (v - r[k].y) * hg.ird[k]) >> 16);
    int su = t_sin(u), cu = t_cos(u);
    int x = ((hg.hx * rr >> 10) + push) * su >> 12, z = ((hg.hz * rr >> 10) + push) * cu >> 12, y = hg.hy * v >> 10;
    P3 q = padd(hg.c, m_app(&hg.m, p3(x, y, z)));
    o->vx = (short)q.x; o->vy = (short)-q.y; o->vz = (short)q.z; o->pad = 0;
}
static SVECTOR fv[40] SVEC_ALIGN;
static int ft_n, f_push = 3;
static void f_begin(void) { ft_n = 0; }
static void f_pt(int xw, int yw) { if (ft_n < 40) fpt(xw, yw, f_push, &fv[ft_n++]); }
static void f_fan(int first, int n, u32 col, int bias) {          // the points first .. first+n-1 are a convex polygon, counter-clockwise (x to the right, y up): a fan of triangles
    if (n < 3) return;
    gfx_xform(&fv[first], &xbuf[0], n);
    gfx_set_zbias(bias > 3 ? (bias - 3) * 2 : bias);   // layers 4.. on distinct buckets (a bias of 2 is one bucket)
    for (int k = 1; k + 1 < n; k++) {
        const XV *a = &xbuf[0], *b = &xbuf[k], *c = &xbuf[k + 1];
        if (a->z < Z_NEAR || b->z < Z_NEAR || c->z < Z_NEAR) continue;
        int cr = xcross(a, b, c); if (FRONT_POS ? cr <= 0 : cr >= 0) continue;
        if ((xv_outcode(a->x, a->y) & xv_outcode(b->x, b->y) & xv_outcode(c->x, c->y)) != 0) continue;
        gfx_put_f3(a, b, c, col);
    }
    gfx_set_zbias(0); ft_n = first;
}
static void f_ell(int cx, int cy, int a, int b, int n, u32 col, int bias) {   // an ellipse: half width a, half height b
    int f0 = ft_n;
    for (int k = 0; k < n; k++) { int t = k * 4096 / n; f_pt(cx + (a * t_cos(t) >> 12), cy + (b * t_sin(t) >> 12)); }
    f_fan(f0, n, col, bias);
}
static void f_half(int cx, int cy, int a, int b, int t0, int t1, int n, u32 col, int bias) {   // a piece of an ellipse from angle t0 to t1 (4096 = round) closed by its chord
    int f0 = ft_n;
    for (int k = 0; k <= n; k++) { int t = t0 + (t1 - t0) * k / n; f_pt(cx + (a * t_cos(t) >> 12), cy + (b * t_sin(t) >> 12)); }
    f_fan(f0, n + 1, col, bias);
}
static void f_quad(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, u32 col, int bias) {   // four corners, counter-clockwise
    int f0 = ft_n; f_pt(x0, y0); f_pt(x1, y1); f_pt(x2, y2); f_pt(x3, y3); f_fan(f0, 4, col, bias);
}
static void f_tri(int x0, int y0, int x1, int y1, int x2, int y2, u32 col, int bias) { int f0 = ft_n; f_pt(x0, y0); f_pt(x1, y1); f_pt(x2, y2); f_fan(f0, 3, col, bias); }
static void f_line(int x0, int y0, int x1, int y1, int th, u32 col, int bias) {   // a stroke th world units thick, from (x0,y0) to (x1,y1)
    if (x1 < x0 || (x1 == x0 && y1 < y0)) { int t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
    int dx = x1 - x0, dy = y1 - y0, l = isqrt(dx * dx + dy * dy); if (!l) return;
    int nx = -dy * th / (2 * l), ny = dx * th / (2 * l);
    f_quad(x0 - nx, y0 - ny, x1 - nx, y1 - ny, x1 + nx, y1 + ny, x0 + nx, y0 + ny, col, bias);
}
static void f_taper(int x0, int y0, int x1, int y1, int th0, int th1, u32 col, int bias) {   // a stroke thick at one end, a point at the other (a brow, a lash, a curl)
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; t = th0; th0 = th1; th1 = t; }
    int dx = x1 - x0, dy = y1 - y0, l = isqrt(dx * dx + dy * dy); if (!l) return;
    int nx = -dy / l, ny = dx / l;
    f_quad(x0 - nx * th0 / 2, y0 - ny * th0 / 2, x1 - nx * th1 / 2, y1 - ny * th1 / 2, x1 + nx * th1 / 2, y1 + ny * th1 / 2, x0 + nx * th0 / 2, y0 + ny * th0 / 2, col, bias);
}

// ---- who they are ----
enum { A_CHAD, A_MISSY, A_INTERN, A_N };
enum { M_CLOSED, M_SMALL, M_WIDE, M_O, M_GRIN, M_SMIRK, M_N };       // the mouth shapes (a spoken line flaps between closed, small and wide)
enum { E_OPEN, E_BLINK, E_HAPPY, E_N };
typedef struct { u8 mouth, eyes, brow, blush; s8 gx, gy; u8 sweat; } Expr;   // brow: 0 level, 1 raised, 2 cross, 3 worried
enum { P_SKIN, P_HAIR, P_A, P_B, P_C, P_D, P_E, P_F, P_N };
static const Pal pal_chad[P_N] = {
    PALX(BGR(226, 152, 126), BGR(253, 204, 168), 0, -500, 5000),            // skin
    PALX(BGR(150, 88, 24), BGR(214, 148, 50), BGR(255, 214, 112), -300, 3300),   // hair (gold)
    PALX(BGR(20, 20, 50), BGR(40, 42, 84), BGR(92, 96, 160), 250, 3500),   // the jacket
    PALX(BGR(190, 196, 232), BGR(250, 250, 255), 0, 250, 5000),            // the shirt
    PALX(BGR(14, 14, 38), BGR(28, 30, 62), BGR(66, 70, 120), 250, 3800),   // trousers
    PALX(BGR(14, 10, 22), BGR(36, 28, 44), BGR(120, 110, 140), 250, 3000), // shoes
    PALX(BGR(150, 14, 38), BGR(228, 36, 68), BGR(255, 120, 140), 250, 3600), // bow tie
    PALX(BGR(170, 110, 20), BGR(255, 196, 50), BGR(255, 242, 160), 250, 3000) };   // gold
static const Pal pal_missy[P_N] = {
    PALX(BGR(234, 170, 164), BGR(255, 226, 204), 0, -500, 5000),
    PALX(BGR(56, 24, 40), BGR(112, 54, 54), BGR(190, 110, 96), 200, 2500), // brown hair with a sheen
    PALX(BGR(92, 34, 150), BGR(152, 62, 208), BGR(204, 128, 240), 250, 3300), // the purple jacket
    PALX(BGR(34, 14, 58), BGR(62, 26, 98), BGR(104, 56, 152), 250, 3500),  // dark violet: skirt, boots, gloves
    PALX(BGR(170, 110, 20), BGR(255, 196, 50), BGR(255, 242, 160), 250, 3000), // gold
    PALX(BGR(190, 196, 232), BGR(250, 250, 255), 0, 250, 5000),
    PALX(BGR(180, 30, 90), BGR(255, 70, 150), BGR(255, 150, 200), 250, 3600),
    PALX(BGR(40, 40, 40), BGR(40, 40, 40), 0, 0, 5000) };
static const Pal pal_intern[P_N] = {
    PALX(BGR(228, 160, 130), BGR(252, 212, 178), 0, -500, 5000),
    PALX(BGR(60, 34, 30), BGR(112, 68, 46), BGR(176, 120, 84), 200, 2900),
    PALX(BGR(196, 70, 18), BGR(250, 118, 34), BGR(255, 180, 100), 250, 3400),   // the bright orange polo
    PALX(BGR(34, 38, 64), BGR(58, 66, 100), BGR(104, 114, 160), 250, 3600),   // slim dark trousers
    PALX(BGR(150, 160, 206), BGR(244, 246, 252), 0, 250, 5000),            // white sneakers
    PALX(BGR(50, 90, 190), BGR(90, 150, 255), BGR(160, 210, 255), 250, 3300), // the lanyard
    PALX(BGR(210, 214, 230), BGR(255, 255, 255), 0, 250, 5000),
    PALX(BGR(40, 40, 40), BGR(40, 40, 40), 0, 0, 5000) };
static const Body body_k[A_N] = {
    { 80, 74, 146, 10, 66, 60, 30, 62, 22, 42, 13 },        // Chad (384 tall, the head a sixth of it)
    { 112, 104, 118, 12, 62, 58, 20, 42, 16, 40, 12 },      // Missy (taller, slimmer, the head a seventh)
    { 72, 68, 126, 10, 58, 52, 22, 56, 18, 38, 11 } };      // the Intern (a lean young man, 340 tall)
static const u32 INK = BGR(36, 14, 34), WHITE = BGR(255, 255, 255);

static void foot_inst(const Joints *J, int s, int hx, int hy, int hz, int pal, int ank) {
    P3 ds = pnorm(psub(J->A[s], J->K[s])), up = pmul(ds, -4096), z0 = p3(0, 0, 4096);
    P3 f = pnorm(psub(z0, pmul(ds, pdot(z0, ds)))); P3 u = pcross(f, up); M3 m = m_cols(u, f, up);
    P3 c = padd(padd(J->A[s], pm(f, hy * 2 / 5)), pm(up, hz - ank));
    add_blob(&SH_BLOB, c, &m, hx, hy, hz, 6, pal);
}
static void hand_inst(const Joints *J, int s, const M3 *bf, int hx, int hy, int hz, int pal) {
    P3 d = pnorm(psub(J->Wr[s], J->E[s])); M3 m; ax_frame(d, &m);
    add_blob(&SH_BLOB, padd(J->Wr[s], pm(d, hy * 3 / 4)), &m, hx, hy, hz, 6, pal); (void)bf;
}
static int g_hk = 4096;     // the head's size against the size its face and hair were drawn at (4096 = 1): the face is laid on the head by angle, so it shrinks with it
#define HS(v) ((v) * g_hk >> 12)
static P3 hf_pt(const Joints *J, int x, int y, int z) { return padd(J->HC, m_app(&J->HF, p3(HS(x), HS(y), HS(z)))); }
static void head_inst(const Shape *sh, const Joints *J, int hx, int hy, int hz, int pal, int k) {
    g_hk = k;
    hg.sh = sh; hg.c = J->HC; hg.m = J->HF; hg.hx = HS(hx); hg.hy = HS(hy); hg.hz = HS(hz); hu_ = hx * 1571 / 1000;
    hg.iu = 1024 * 65536 / hu_; hg.iv = 1024 * 65536 / hy; for (int j = 0; j + 1 < sh->nr && j < 10; j++) hg.ird[j] = 65536 / (sh->r[j + 1].y - sh->r[j].y);
    add_blob(sh, J->HC, &J->HF, HS(hx), HS(hy), HS(hz), 12, pal);
}
static P3 bf_pt(const Joints *J, int x, int y, int z) { return padd(J->Hh, m_app(&J->BF, p3(x, y, z))); }
static void limb_tubes(const Joints *J, const Body *B, int s, int rA0, int rA1, int rF0, int rF1, int palU, int palF) {
    add_tube(J->S[s], J->E[s], rA0, rA1, palU, 6); add_tube(J->E[s], J->Wr[s], rF0, rF1, palF, 6);
    M3 id = m_id();
    add_blob(&SH_JNT, J->E[s], &id, rA1 + 1, rA1 + 1, rA1 + 1, 6, palU); (void)B;
}
static void leg_tubes(const Joints *J, int s, int rT0, int rT1, int rS0, int rS1, int palT, int palS) {
    add_tube(J->Hp[s], J->K[s], rT0, rT1, palT, 6); add_tube(J->K[s], J->A[s], rS0, rS1, palS, 6);
    M3 id = m_id();
    add_blob(&SH_JNT, J->K[s], &id, rT1 + 1, rT1 + 1, rT1 + 1, 6, palT);
}

// ---- Chad: a big friendly man, broad in the shoulder; head about a sixth of his height; a gold quiff with a side part, a tuxedo and a grin ----
static void tuft(const Joints *J, int x, int y, int z, int dx, int dy, int dz, int len, int w, int pal, int bias) {   // a lock of hair: from (x,y,z) on the head (authoring units) out along (dx,dy,dz)
    P3 a = hf_pt(J, x, y, z), d = pnorm(p3(dx, dy, dz)), b = padd(a, pm(d, HS(len))); M3 m; ax_frame(d, &m);
    Inst *I = inst_add(&SH_LOCK, p3((a.x + b.x) >> 1, (a.y + b.y) >> 1, (a.z + b.z) >> 1), &m, HS(w), HS(len) / 2, HS(w), 6, pal); I->bias = (s8)bias;
}
static int plen(P3 a, P3 b) { return isqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y) + (b.z - a.z) * (b.z - a.z)); }
static void build_chad(const Joints *J, const FPose *p) {
    const Body *B = &body_k[A_CHAD]; M3 id = m_id();
    for (int s = 0; s < 2; s++) {
        foot_inst(J, s, 21, 40, 14, 5, B->ank);
        leg_tubes(J, s, 27, 22, 21, 16, 4, 4);
    }
    add_blob(&SH_BALL, J->Hh, &id, 52, 28, 40, 8, 4);                                     // the hips
    add_blob(&SH_BARREL, padd(J->Hh, m_app(&J->BF, p3(0, B->torso / 2 - 4, 0))), &J->BF, 78, B->torso / 2 + 8, 54, 8, 2);    // the jacket: chest and shoulders
    add_blob(&SH_BLOB, bf_pt(J, 0, B->torso * 6 / 10, 52), &J->BF, 20, 44, 10, 6, 3);     // the shirt front
    for (int s = 0; s < 2; s++) {
        add_blob(&SH_JNT, J->S[s], &id, 22, 22, 22, 6, 2);
        limb_tubes(J, B, s, 22, 19, 18, 15, 2, 2);
        P3 d = pnorm(psub(J->Wr[s], J->E[s]));
        add_tube(psub(J->Wr[s], pm(d, 12)), J->Wr[s], 16, 16, 3, 6);                       // a white cuff
        hand_inst(J, s, &J->BF, 11, 17, 14, P_SKIN);
    }
    head_inst(&SH_HEAD, J, 66, 62, 61, P_SKIN, 2550);                                       // (the head's size: 0.54 of the face's drawing size)
    add_tube(psub(J->N, p3(0, 6, 0)), hf_pt(J, 0, -40, 0), 17, 16, P_SKIN, 8);                // a real neck
    add_blob(&SH_BLOB, hf_pt(J, 0, -6, 66), &J->HF, HS(7), HS(11), HS(8), 6, P_SKIN);        // the nose
    for (int s = 0; s < 2; s++) add_blob(&SH_BLOB, hf_pt(J, sgn[s] * 66, -4, -2), &J->HF, HS(8), HS(15), HS(11), 6, P_SKIN);   // ears
    // the hair: a swept gold quiff with a side part, wrapped round the skull
    Inst *I;
    I = inst_add(&SH_CAP, hf_pt(J, 0, 8, -4), &J->HF, HS(74), HS(70), HS(70), 12, P_HAIR); if (I) { I->a0 = 700; I->span = 2700; I->bias = 2; }   // back and sides, down to the nape
    add_blob(&SH_BALL, hf_pt(J, 0, 58, 10), &J->HF, HS(60), HS(30), HS(56), 8, P_HAIR)->bias = 2;       // the crown
    add_blob(&SH_BALL, hf_pt(J, -14, 74, 52), &J->HF, HS(46), HS(24), HS(34), 8, P_HAIR)->bias = 3;     // the quiff, swept up and over to one side
    tuft(J, 20, 70, 40, 40, 40, 90, 34, 18, P_HAIR, 3);
    (void)p;
}

// ---- Missy: tall and sleek; purple; long brown hair with a side parting and a fringe in layered locks; boots; one hip always out ----
static void build_missy(const Joints *J, const FPose *p) {
    const Body *B = &body_k[A_MISSY]; M3 id = m_id();
    for (int s = 0; s < 2; s++) {
        foot_inst(J, s, 14, 33, 12, 3, B->ank);
        leg_tubes(J, s, 19, 15, 15, 12, P_SKIN, 3);
        P3 d = pnorm(psub(J->A[s], J->K[s]));
        add_tube(padd(J->K[s], pm(d, -4)), padd(J->K[s], pm(d, 16)), 18, 18, 4, 6);         // the gold band of the boot top
    }
    add_blob(&SH_BALL, J->Hh, &id, 38, 24, 32, 8, 3);
    add_blob(&SH_SKIRT, padd(J->Hh, p3(0, -18, 0)), &id, 56, 32, 48, 8, 3);                     // the short skirt (wider at the hem: ring 0 is the hem)
    add_blob(&SH_WAIST, padd(J->Hh, m_app(&J->BF, p3(0, B->torso / 2 - 2, 0))), &J->BF, 44, B->torso / 2 + 6, 30, 8, 2);
    add_tube(bf_pt(J, 0, 6, 0), bf_pt(J, 0, 14, 0), 36, 36, 4, 8);                            // the gold belt
    for (int s = 0; s < 2; s++) {
        add_blob(&SH_JNT, J->S[s], &id, 16, 16, 16, 6, 2);
        limb_tubes(J, B, s, 14, 12, 12, 10, 2, 3);
        hand_inst(J, s, &J->BF, 8, 14, 11, P_SKIN);
    }
    head_inst(&SH_HEADS, J, 46, 50, 45, P_SKIN, 2800);
    add_tube(psub(J->N, p3(0, 6, 0)), hf_pt(J, 0, -44, 0), 13, 12, P_SKIN, 8);                // a slim neck, a real one
    add_blob(&SH_BLOB, hf_pt(J, 0, -6, 45), &J->HF, HS(5), HS(9), HS(7), 6, P_SKIN);          // the nose
    for (int s = 0; s < 2; s++) add_blob(&SH_BLOB, hf_pt(J, sgn[s] * 45, -4, -2), &J->HF, HS(6), HS(11), HS(8), 6, P_SKIN);
    // the hair: wraps the skull, back volume, a long fall, locks that frame the face
    Inst *I;
    I = inst_add(&SH_CAP, hf_pt(J, 0, 6, -4), &J->HF, HS(52), HS(56), HS(52), 12, P_HAIR); if (I) { I->a0 = 700; I->span = 2700; I->bias = 2; }
    add_blob(&SH_BALL, hf_pt(J, 0, 38, -8), &J->HF, HS(46), HS(36), HS(44), 8, P_HAIR)->bias = 2;       // volume at the back of the crown
    { P3 a = hf_pt(J, 0, 30, -30), b = hf_pt(J, 0, -150, -44); M3 m; ax_frame(pnorm(psub(b, a)), &m);
      I = inst_add(&SH_LOCK, padd(a, pm(psub(b, a), 2048)), &m, HS(46), plen(a, b) / 2, HS(14), 8, P_HAIR); if (I) I->bias = 1; }   // the long fall down the back
    for (int s = 0; s < 2; s++) {
        P3 a = hf_pt(J, sgn[s] * 38, 30, -10), b = hf_pt(J, sgn[s] * 42, -110, 6); M3 m; ax_frame(pnorm(psub(b, a)), &m);
        I = inst_add(&SH_LOCK, padd(a, pm(psub(b, a), 2048)), &m, HS(13), plen(a, b) / 2, HS(11), 6, P_HAIR); if (I) I->bias = 2;   // a lock over each shoulder
    }
    (void)p;
}

// ---- the Intern: a lean young man in his twenties; messy short hair, a headset, a bright orange polo with the sleeves rolled up, a lanyard, slim dark trousers ----
static void build_intern(const Joints *J, const FPose *p) {
    const Body *B = &body_k[A_INTERN]; M3 id = m_id();
    for (int s = 0; s < 2; s++) {
        foot_inst(J, s, 15, 32, 11, 4, B->ank);
        leg_tubes(J, s, 17, 13, 12, 10, 3, 3);
    }
    add_blob(&SH_BALL, J->Hh, &id, 34, 22, 28, 8, 3);
    add_blob(&SH_BOX, padd(J->Hh, m_app(&J->BF, p3(0, B->torso / 2 + 2, 0))), &J->BF, 56, B->torso / 2 + 6, 30, 8, 2);    // the polo
    add_tube(bf_pt(J, 0, B->torso - 6, 12), bf_pt(J, 0, 18, 36), 2, 2, 5, 6);                  // the lanyard
    add_blob(&SH_BLOB, bf_pt(J, 0, 26, 36), &J->BF, 10, 12, 3, 6, 6);                           // the badge
    for (int s = 0; s < 2; s++) {
        add_blob(&SH_JNT, J->S[s], &id, 17, 17, 17, 6, 2);
        P3 d = pnorm(psub(J->E[s], J->S[s])), q = padd(J->S[s], pm(d, 44));
        add_tube(J->S[s], q, 16, 14, 2, 6);                                                    // the sleeve, rolled up above the elbow
        add_tube(padd(q, pm(d, -3)), padd(q, pm(d, 6)), 15, 15, 6, 6);                          // the roll: a lighter band
        add_tube(q, J->E[s], 12, 11, P_SKIN, 6);
        add_tube(J->E[s], J->Wr[s], 10, 8, P_SKIN, 6);
        add_blob(&SH_JNT, J->E[s], &id, 11, 11, 11, 6, P_SKIN);
        hand_inst(J, s, &J->BF, 8, 14, 11, P_SKIN);
    }
    head_inst(&SH_HEADL, J, 66, 62, 61, P_SKIN, 2100);
    add_tube(psub(J->N, p3(0, 6, 0)), hf_pt(J, 0, -42, 0), 14, 13, P_SKIN, 8);                // the neck
    add_blob(&SH_BLOB, hf_pt(J, 0, -6, 66), &J->HF, HS(7), HS(11), HS(9), 6, P_SKIN);         // the nose
    for (int s = 0; s < 2; s++) add_blob(&SH_BLOB, hf_pt(J, sgn[s] * 66, -4, -2), &J->HF, HS(8), HS(15), HS(11), 6, P_SKIN);
    // the headset: cups on the ears, a band over the head, a mic by the mouth
    for (int s = 0; s < 2; s++) add_blob(&SH_BLOB, hf_pt(J, sgn[s] * 72, -2, -2), &J->HF, HS(6), HS(11), HS(10), 6, P_F);
    add_tube(hf_pt(J, 72, 10, -2), hf_pt(J, 56, 74, 0), 2, 2, P_F, 5); add_tube(hf_pt(J, 56, 74, 0), hf_pt(J, 0, 88, -2), 2, 2, P_F, 5);
    add_tube(hf_pt(J, 0, 88, -2), hf_pt(J, -56, 74, 0), 2, 2, P_F, 5); add_tube(hf_pt(J, -56, 74, 0), hf_pt(J, -72, 10, -2), 2, 2, P_F, 5);
    add_tube(hf_pt(J, 74, -16, 8), hf_pt(J, 30, -52, 58), 2, 2, P_F, 5); add_blob(&SH_BLOB, hf_pt(J, 30, -52, 60), &J->HF, 3, 4, 3, 5, P_F);
    // messy short hair: a close cap and tufts going every way
    Inst *I;
    I = inst_add(&SH_CAP, hf_pt(J, 0, 8, -4), &J->HF, HS(72), HS(68), HS(68), 12, P_HAIR); if (I) { I->a0 = 700; I->span = 2700; I->bias = 2; }
    add_blob(&SH_BALL, hf_pt(J, 0, 56, 8), &J->HF, HS(58), HS(28), HS(54), 8, P_HAIR)->bias = 2;
    tuft(J, -30, 62, 14, -50, 40, 30, 26, 14, P_HAIR, 3); tuft(J, 8, 72, 20, 10, 50, 60, 26, 15, P_HAIR, 3); tuft(J, 34, 60, 10, 60, 40, 20, 24, 13, P_HAIR, 3);
    (void)p;
}

// ---- faces ----
typedef struct { s16 ex, ey, ea, eb, my, mw, ny; u8 style; u32 iris, iris2, brow, cheek; } FaceK;
static const FaceK face_k[A_N] = {
    { 23, 5, 10, 11, -31, 25, -10, 0, BGR(110, 64, 30), BGR(206, 130, 56), BGR(120, 70, 22), BGR(255, 180, 160) },     // Chad
    { 17, 2, 9, 8, -29, 14, -12, 1, BGR(120, 60, 220), BGR(220, 150, 255), BGR(70, 30, 40), BGR(255, 180, 190) },     // Missy
    { 23, 3, 10, 11, -31, 16, -11, 2, BGR(54, 110, 200), BGR(130, 190, 240), BGR(70, 44, 32), BGR(255, 176, 170) } };  // the Intern
static void f_ring(int cx, int cy, int a, int b, int t0, int t1, int n, int th, u32 col, int bias) {   // a stroke along an ellipse (angles 4096 to a turn), th thick, drawn outside it
    for (int k = 0; k < n; k++) {
        int ta = t0 + (t1 - t0) * k / n, tb = t0 + (t1 - t0) * (k + 1) / n;
        int ax = cx + (a * t_cos(ta) >> 12), ay = cy + (b * t_sin(ta) >> 12), bx = cx + (a * t_cos(tb) >> 12), by = cy + (b * t_sin(tb) >> 12);
        int ox = cx + ((a + th) * t_cos(ta) >> 12), oy = cy + ((b + th) * t_sin(ta) >> 12), px = cx + ((a + th) * t_cos(tb) >> 12), py = cy + ((b + th) * t_sin(tb) >> 12);
        f_quad(ax, ay, ox, oy, px, py, bx, by, col, bias);
    }
}
static void face_eye(int who, int sx, const Expr *e, int blink) {          // sx: -1 or 1, the side of the face
    const FaceK *K = &face_k[who]; int cx = sx * K->ex, cy = K->ey, a = K->ea, b = K->eb;
    int mode = e->eyes == E_HAPPY ? 2 : (blink || e->eyes == E_BLINK) ? 1 : 0;
    if (mode) {   // a shut eye: a bowed line (a happy one bows up)
        int n = 5, py = 0, px = 0;
        for (int k = 0; k <= n; k++) {
            int t = 2048 * k / n, x = cx + sx * 0 + (a * 2 / 1 * (k - n / 2) / n) , y = cy + (mode == 2 ? 1 : -1) * (b * t_sin(t) >> 13) - (mode == 2 ? 0 : b / 3);
            if (k) f_line(px, py, x, y, K->style == 1 ? 3 : 2, INK, 8);
            px = x; py = y;
        }
        if (K->style == 1) f_taper(cx + sx * a, cy - b / 3, cx + sx * (a + 7), cy + 1, 3, 0, INK, 8);
        return;
    }
    int ix = cx + sx * e->gx / 2, iy = cy + e->gy / 2;
    f_ell(cx, cy, a, b, g_lod ? 8 : 12, WHITE, 4);
    int ra = a * (K->style == 1 ? 74 : K->style == 2 ? 78 : 64) / 100, rb = b * (K->style == 2 ? 92 : 88) / 100;
    f_ell(ix, iy, ra, rb, g_lod ? 8 : 10, K->iris, 5);
    if (!g_lod) f_half(ix, iy, ra, rb, 2048, 4096, 5, K->iris2, 6);                       // the lower half of the iris, lighter
    f_ell(ix, iy, ra * 58 / 100, rb * 62 / 100, g_lod ? 6 : 8, BGR(18, 8, 30), 7);       // the pupil
    f_ell(ix - ra * 32 / 100, iy + rb * 34 / 100, ra * 34 / 100, rb * 30 / 100, g_lod ? 5 : 8, WHITE, 8);   // the highlight
    // the outline: thick over the top, thin under
    f_ring(cx, cy, a, b, 0, 2048, g_lod ? 3 : 6, K->style == 1 ? 4 : 3, INK, 7);
    f_ring(cx, cy, a, b, 2048, 4096, g_lod ? 3 : 6, 1, INK, 7);
    if (K->style == 1) {      // Missy: the wing and a second lash
        f_taper(cx + sx * a, cy + b / 2, cx + sx * (a + 9), cy + b + 3, 4, 0, INK, 8);
        f_taper(cx + sx * (a - 3), cy + b, cx + sx * (a + 3), cy + b + 6, 3, 0, INK, 8);
    }
}
static void face_mouth(int who, int shape) {
    const FaceK *K = &face_k[who]; int my = K->my, w = K->mw; const u32 TONGUE = BGR(255, 110, 130);
    switch (shape) {
    case M_CLOSED: for (int k = 0; k < 4; k++) { int x0 = -w + w * k / 2, x1 = -w + w * (k + 1) / 2, y0 = my + ABS(x0) / 6 - 2, y1 = my + ABS(x1) / 6 - 2; f_line(x0, y0 + 1, x1, y1 + 1, 2, INK, 8); } break;
    case M_SMIRK:  f_line(-w, my, 0, my - 2, 2, INK, 8); f_line(0, my - 2, w, my + w / 4, 2, INK, 8); f_taper(w, my + w / 4, w + 3, my + w / 4 + 3, 2, 0, INK, 8); break;
    case M_SMALL:  f_half(0, my + 3, w * 6 / 10, w * 5 / 10, 2048, 4096, 8, INK, 8); f_half(0, my + 3, w * 40 / 100, w * 24 / 100, 2048, 4096, 6, TONGUE, 9); f_line(-w * 6 / 10, my + 3, w * 6 / 10, my + 3, 2, INK, 8); break;
    case M_WIDE:   f_half(0, my + 5, w, w * 85 / 100, 2048, 4096, 10, INK, 8); f_half(0, my + 5, w * 62 / 100, w * 36 / 100, 2048, 4096, 8, TONGUE, 9);
                   f_quad(-w * 80 / 100, my + 5 - w * 22 / 100, w * 80 / 100, my + 5 - w * 22 / 100, w * 80 / 100, my + 5, -w * 80 / 100, my + 5, WHITE, 10); break;
    case M_GRIN:   f_half(0, my + 8, w * 118 / 100, w * 100 / 100, 2048, 4096, 10, INK, 8); f_half(0, my + 8, w * 70 / 100, w * 44 / 100, 2048, 4096, 8, TONGUE, 9);
                   f_quad(-w * 100 / 100, my + 8 - w * 26 / 100, w * 100 / 100, my + 8 - w * 26 / 100, w * 112 / 100, my + 8, -w * 112 / 100, my + 8, WHITE, 10); break;
    case M_O:      f_ell(0, my, w * 36 / 100, w * 46 / 100, 10, INK, 8); f_ell(0, my - w * 16 / 100, w * 22 / 100, w * 18 / 100, 8, TONGUE, 9); break;
    }
}
static void face_far(int who, const Expr *e) {       // far away: two dark eyes with a point of white, a mouth, nothing more
    const FaceK *K = &face_k[who]; f_begin(); f_push = 8;
    for (int s = 0; s < 2; s++) { int sx = sgn[s]; f_ell(sx * K->ex, K->ey, K->ea, K->eb, 4, BGR(30, 16, 40), 8); }
    f_line(-K->mw / 2, K->my, K->mw / 2, K->my, e->mouth >= M_SMALL && e->mouth != M_CLOSED && e->mouth != M_SMIRK ? 5 : 2, INK, 8);
}
static void face_draw(int who, const Expr *e, int blink) {
    const FaceK *K = &face_k[who]; f_begin();
    f_push = (who == A_CHAD ? 3 : who == A_MISSY ? 2 : 3) + g_lod * 2;
    for (int s = 0; s < 2; s++) {
        int sx = sgn[s];
        if (e->blush) f_ell(sx * (K->ex + 5), K->ey - K->eb - 7, 9, 4, 8, K->cheek, 4);
        face_eye(who, sx, e, blink);
        // the brows
        int x0 = sx * (K->ex - K->ea * 9 / 10), x1 = sx * (K->ex + K->ea * 11 / 10), by = K->ey + K->eb + 7;
        int dy0 = 0, dy1 = 0;
        if (e->brow == 1) { dy0 = dy1 = 4; } else if (e->brow == 2) { dy0 = -4; dy1 = 3; } else if (e->brow == 3) { dy0 = 5; dy1 = -1; }
        if (who == A_MISSY && e->brow == 0) { dy0 = -1; dy1 = 3; }
        f_taper(x0, by + dy0, x1, by + dy1, who == A_MISSY ? 2 : 4, who == A_MISSY ? 1 : 3, K->brow, 8);   // (a brow over each eye)
    }
    f_line(-1, K->ny + 1, 2, K->ny - 1, 2, BGR(190, 110, 100), 6);        // the nose: a small stroke
    face_mouth(who, e->mouth);
    if (e->sweat) { int x = hg.hx * 52 / 100, y = hg.hy * 55 / 100; f_tri(x - 6, y, x + 6, y, x, y + 14, BGR(120, 200, 255), 8); f_ell(x, y, 6, 6, 8, BGR(120, 200, 255), 8); }
}

// ---- the hair that lies on the head (fringes, side-burns), drawn over the face's base: the head's shape under it is in build_* ----
static void hair_decals(int who) {
    const Pal *pl = who == A_CHAD ? pal_chad : who == A_MISSY ? pal_missy : pal_intern; u32 hb = pl[P_HAIR].c[1], hs = pl[P_HAIR].c[0];
    f_begin(); f_push = 6;
    if (who == A_MISSY) {   // a side parting and a fringe in layered locks, swept away from the parting
        static const s8 bx[7] = { -38, -26, -14, -3, 9, 22, 35 }, ty[7] = { 8, 2, 6, 10, 12, 4, 10 };
        for (int k = 0; k < 7; k++) { int x = bx[k], t = ty[k];
            f_tri(x - 12, 46, x + 7, t - 4, x + 13, 46, hs, 12);
            f_tri(x - 10, 48, x + 8, t + 1, x + 11, 48, hb, 13); }
        f_line(-14, 60, -9, 40, 2, hs, 14);      // the parting
    } else if (who == A_INTERN) {   // a messy fringe: uneven, falling every way
        static const s8 bx[6] = { -46, -28, -10, 8, 26, 44 }, ty[6] = { 20, 30, 14, 28, 18, 26 }, lx[6] = { -6, 4, -4, 6, -5, 3 };
        for (int k = 0; k < 6; k++) { int x = bx[k], t = ty[k];
            f_tri(x - 13, 58, x + lx[k], t - 3, x + 14, 58, hs, 12);
            f_tri(x - 12, 60, x + lx[k] + 1, t + 2, x + 13, 60, hb, 13); }
    } else {   // Chad: a clean hairline across the forehead, a side part, side-burns
        static const s8 bx[5] = { -40, -20, 0, 20, 40 }, ty[5] = { 34, 38, 30, 36, 40 };
        for (int k = 0; k < 5; k++) { int x = bx[k], t = ty[k];
            f_tri(x - 12, 58, x + 5, t - 2, x + 13, 58, hs, 12);
            f_tri(x - 11, 60, x + 6, t + 3, x + 12, 60, hb, 13); }
        f_line(-26, 64, -18, 40, 2, hs, 14);     // the part
        for (int s = 0; s < 2; s++) { int x = sgn[s] * 60; f_quad(x - 4, 20, x - 4, -12, x + 4, -12, x + 4, 20, hb, 13); }
    }
}

// ---- one person, drawn: fk, then the list of shapes (ink shell first, then the flat colour), the hair on the head, the face ----
static int g_sheet;       // 1: the model sheet (test): full detail at any distance
static int g_noink;       // 1: no ink shells (a crowded or far frame)
static int g_far;         // 1: far away (a head ten pixels wide): no small parts, no hair decals, a face of two dots and a line
static int g_wpx = 14;    // the ink's weight, tenths of a pixel (heavier in a close-up)
#ifdef FILM_PROF
int film_pf[4]; static int pf_t;
#define PF(k) do { int n_ = VSync(1); if (k) film_pf[k - 1] += n_ - pf_t; pf_t = n_; } while (0)
#else
#define PF(k)
#endif
static void fig_draw(int who, int x, int z, int yaw, const FPose *p, const Expr *e, int blink) {
    Joints J; fk(&body_k[who], p, &J);
    nil_ = 0; g_pal = who == A_CHAD ? pal_chad : who == A_MISSY ? pal_missy : pal_intern; g_ink = INK;
    if (who == A_CHAD) build_chad(&J, p); else if (who == A_MISSY) build_missy(&J, p); else build_intern(&J, p);
    VECTOR pos = { x, 0, -z }; int ym = (2048 - (yaw << 4)) & 4095;
    gfx_set_object(&pos, ym);
    {   // the lights, from the camera's side into the body's space
        MATRIX R; SVECTOR rr = { 0, (short)ym, 0, 0 }; RotMatrix(&rr, &R);
        int Lw[2][3], Lm[2][3];
        for (int k = 0; k < 2; k++) {
            const int *lv = k ? film_Rv : film_Lv;
            for (int i = 0; i < 3; i++) Lw[k][i] = (cam.view.m[0][i] * lv[0] + cam.view.m[1][i] * lv[1] + cam.view.m[2][i] * lv[2]) >> 12;
            for (int i = 0; i < 3; i++) Lm[k][i] = (R.m[0][i] * Lw[k][0] + R.m[1][i] * Lw[k][1] + R.m[2][i] * Lw[k][2]) >> 12;
        }
        g_L = p3(Lm[0][0], -Lm[0][1], Lm[0][2]); g_R = p3(Lm[1][0], -Lm[1][1], Lm[1][2]);
        int dx = x - cam.eye.vx, dy = -cam.eye.vy, dz = -z - cam.eye.vz;
        int Dz = (cam.view.m[2][0] * dx + cam.view.m[2][1] * dy + cam.view.m[2][2] * dz) >> 12; if (Dz < 60) Dz = 60;
        g_ol = CLAMP(Dz * g_wpx / 2300, 2, 8); g_lod = g_sheet ? 0 : Dz > 850 ? 2 : Dz > 500 ? 1 : 0; g_far = !g_sheet && Dz > 1250;
    }
    PF(0);
    for (int i = 0; i < nil_; i++) { inst_cull(&il[i]); if (g_far && (il[i].sh == &SH_JNT || (il[i].hx < 16 && il[i].hy < 16 && il[i].hz < 16))) il[i].flag |= IF_OFF; }
    for (int i = 0; i < nil_; i++) inst_draw(&il[i], 0);       // (the colour first, then the ink behind it: a full packet buffer loses the ink, never the person)
    if (!g_noink) for (int i = 0; i < nil_; i++) if (!(il[i].flag & IF_NOOL) && !(il[i].sh == &SH_JNT || (il[i].hx < 14 && il[i].hy < 14 && il[i].hz < 14) || (g_lod == 2 && il[i].hx < 20 && il[i].hy < 20 && il[i].hz < 20))) inst_draw(&il[i], 1);   // (far away, the small parts go without ink)
    PF(1);
    int showface = 1;      // (the face and the fringe are decals on the head's front: not worked out when the head is off the picture or turned away)
    for (int i = 0; i < nil_; i++) if (IS_HEAD(il[i].sh)) {
        if (il[i].flag & IF_OFF) showface = 0;
        else { static SVECTOR pts[2] SVEC_ALIGN; static XV q[2]; P3 fw = m_col(&il[i].m, 2);
               pts[0].vx = (short)il[i].c.x; pts[0].vy = (short)-il[i].c.y; pts[0].vz = (short)il[i].c.z;
               pts[1].vx = (short)(il[i].c.x + (fw.x * 100 >> 12)); pts[1].vy = (short)-(il[i].c.y + (fw.y * 100 >> 12)); pts[1].vz = (short)(il[i].c.z + (fw.z * 100 >> 12));
               gfx_xform(pts, q, 2); if (q[1].z - q[0].z > 45) showface = 0; }
        break;
    }
    if (showface && g_far) face_far(who, e);
    else if (showface) { hair_decals(who); PF(2); face_draw(who, e, blink); PF(3); }
}
