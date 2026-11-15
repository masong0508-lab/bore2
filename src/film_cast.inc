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
typedef struct { const Shape *sh; P3 c; M3 m; int hx, hy, hz; } HeadG;
static HeadG hg; static int hu_;
static void fpt(int xw, int yw, int push, SVECTOR *o) {
    int u = xw * 1024 / hu_, v = yw * 1024 / hg.hy; v = CLAMP(v, -1020, 1020);
    const Ring *r = hg.sh->r; int n = hg.sh->nr, k = 0; while (k < n - 2 && v > r[k + 1].y) k++;
    int rr = r[k].r + (r[k + 1].r - r[k].r) * (v - r[k].y) / (r[k + 1].y - r[k].y);
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
    PALX(BGR(222, 158, 20), BGR(255, 214, 60), BGR(255, 242, 150), 250, 3400),   // the yellow polo
    PALX(BGR(30, 40, 100), BGR(54, 68, 142), BGR(98, 118, 196), 250, 3600),   // navy shorts
    PALX(BGR(150, 160, 206), BGR(244, 246, 252), 0, 250, 5000),            // white sneakers
    PALX(BGR(50, 90, 190), BGR(90, 150, 255), BGR(160, 210, 255), 250, 3300), // the lanyard
    PALX(BGR(210, 214, 230), BGR(255, 255, 255), 0, 250, 5000),
    PALX(BGR(40, 40, 40), BGR(40, 40, 40), 0, 0, 5000) };
static const Body body_k[A_N] = {
    { 76, 70, 132, 12, 64, 58, 30, 78, 20, 56, 13 },        // Chad
    { 108, 100, 112, 20, 62, 58, 20, 40, 16, 40, 12 },      // Missy
    { 40, 38, 62, 4, 38, 34, 16, 34, 10, 70, 10 } };        // the Intern
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
static void head_inst(const Shape *sh, const Joints *J, int hx, int hy, int hz, int pal) {
    hg.sh = sh; hg.c = J->HC; hg.m = J->HF; hg.hx = hx; hg.hy = hy; hg.hz = hz; hu_ = hx * 1571 / 1000;
    add_blob(sh, J->HC, &J->HF, hx, hy, hz, 12, pal);
}
static P3 hf_pt(const Joints *J, int x, int y, int z) { return padd(J->HC, m_app(&J->HF, p3(x, y, z))); }
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

// ---- Chad: tall as a doorway, round as a barrel, a gold quiff, a tuxedo and a grin ----
static void build_chad(const Joints *J, const FPose *p) {
    const Body *B = &body_k[A_CHAD]; M3 id = m_id();
    for (int s = 0; s < 2; s++) {
        foot_inst(J, s, 21, 38, 14, 5, B->ank);
        leg_tubes(J, s, 28, 22, 21, 16, 4, 4);
    }
    add_blob(&SH_BALL, J->Hh, &id, 50, 26, 40, 8, 4);                                     // the hips
    add_blob(&SH_BARREL, padd(J->Hh, m_app(&J->BF, p3(0, B->torso / 2 - 4, 0))), &J->BF, 76, B->torso / 2 + 8, 56, 8, 2);    // the jacket
    add_blob(&SH_BLOB, bf_pt(J, 0, B->torso * 6 / 10, 48), &J->BF, 22, 44, 12, 6, 3);     // the shirt front
    for (int s = 0; s < 2; s++) {
        add_blob(&SH_JNT, J->S[s], &id, 24, 24, 24, 6, 2);
        limb_tubes(J, B, s, 21, 18, 18, 15, 2, 2);
        P3 d = pnorm(psub(J->Wr[s], J->E[s]));
        add_tube(psub(J->Wr[s], pm(d, 12)), J->Wr[s], 16, 16, 3, 6);                       // a white cuff
        hand_inst(J, s, &J->BF, 11, 17, 14, P_SKIN);
    }
    add_tube(J->N, hf_pt(J, 0, -22, 0), 22, 22, P_SKIN, 6);                                  // the neck
    head_inst(&SH_HEAD, J, 66, 62, 61, P_SKIN);
    for (int s = 0; s < 2; s++) add_blob(&SH_BLOB, hf_pt(J, sgn[s] * 63, -4, -2), &J->HF, 7, 13, 10, 6, P_SKIN);   // ears
    // the hair: a swept gold quiff
    Inst *I;
    I = inst_add(&SH_CAP, hf_pt(J, 0, 9, -4), &J->HF, 72, 66, 68, 12, P_HAIR); if (I) { I->a0 = 780; I->span = 2540; I->bias = 2; }   // the back and sides
    add_blob(&SH_BALL, hf_pt(J, 0, 62, 14), &J->HF, 56, 26, 52, 8, P_HAIR)->bias = 2;       // the crown
    add_blob(&SH_BALL, hf_pt(J, 0, 72, 54), &J->HF, 40, 20, 30, 8, P_HAIR)->bias = 2;       // the quiff, rolled forward over the brow
    (void)p;
}

// ---- Missy: tall and sleek; purple; a long brown fall of hair with a fringe; boots; one hip always out ----
static void build_missy(const Joints *J, const FPose *p) {
    const Body *B = &body_k[A_MISSY]; M3 id = m_id();
    for (int s = 0; s < 2; s++) {
        foot_inst(J, s, 13, 31, 12, 3, B->ank);
        leg_tubes(J, s, 17, 14, 14, 11, P_SKIN, 3);
        P3 d = pnorm(psub(J->A[s], J->K[s]));
        add_tube(padd(J->K[s], pm(d, -4)), padd(J->K[s], pm(d, 14)), 17, 17, 4, 6);         // the gold band of the boot top
    }
    add_blob(&SH_BALL, J->Hh, &id, 36, 22, 30, 8, 3);
    add_blob(&SH_SKIRT, padd(J->Hh, p3(0, -18, 0)), &id, 52, 30, 46, 8, 3);                     // the short skirt (wider at the hem: ring 0 is the hem)
    add_blob(&SH_WAIST, padd(J->Hh, m_app(&J->BF, p3(0, B->torso / 2 - 2, 0))), &J->BF, 40, B->torso / 2 + 6, 28, 8, 2);
    add_tube(bf_pt(J, 0, 6, 0), bf_pt(J, 0, 14, 0), 34, 34, 4, 8);                            // the gold belt
    for (int s = 0; s < 2; s++) {
        add_blob(&SH_JNT, J->S[s], &id, 15, 15, 15, 6, 2);
        limb_tubes(J, B, s, 13, 12, 11, 8, 2, 3);
        hand_inst(J, s, &J->BF, 7, 13, 10, P_SKIN);
    }
    add_tube(J->N, hf_pt(J, 0, -24, 0), 12, 12, P_SKIN, 6);
    head_inst(&SH_HEADS, J, 46, 50, 45, P_SKIN);
    for (int s = 0; s < 2; s++) add_blob(&SH_BLOB, hf_pt(J, sgn[s] * 44, -4, -2), &J->HF, 5, 10, 8, 6, P_SKIN);
    // the hair
    Inst *I;
    I = inst_add(&SH_CAP, hf_pt(J, 0, 8, -4), &J->HF, 50, 52, 50, 12, P_HAIR); if (I) { I->a0 = 780; I->span = 2540; I->bias = 2; }
    add_blob(&SH_BALL, hf_pt(J, 0, 46, 14), &J->HF, 38, 22, 38, 8, P_HAIR)->bias = 2;
    { P3 a = hf_pt(J, 0, 30, -34), b = hf_pt(J, 0, -92, -40); M3 m; ax_frame(pnorm(psub(b, a)), &m);
      I = inst_add(&SH_LOCK, padd(a, pm(psub(b, a), 2048)), &m, 36, 66, 11, 8, P_HAIR); if (I) I->bias = 1; }   // the long fall down the back
    for (int s = 0; s < 2; s++) { P3 a = hf_pt(J, sgn[s] * 47, 20, 8), b = hf_pt(J, sgn[s] * 49, -62, 10); M3 m; ax_frame(pnorm(psub(b, a)), &m);
      I = inst_add(&SH_LOCK, padd(a, pm(psub(b, a), 2048)), &m, 8, 42, 8, 6, P_HAIR); if (I) I->bias = 2; }   // the locks that frame her face
    (void)p;
}

// ---- the Intern: a head and a half of body; yellow polo, navy shorts, a bowl of brown hair with one stubborn curl ----
static void build_intern(const Joints *J, const FPose *p) {
    const Body *B = &body_k[A_INTERN]; M3 id = m_id();
    for (int s = 0; s < 2; s++) {
        foot_inst(J, s, 15, 28, 11, 4, B->ank);
        leg_tubes(J, s, 14, 12, 10, 9, P_SKIN, P_SKIN);
        add_tube(J->Hp[s], padd(J->Hp[s], pm(pnorm(psub(J->K[s], J->Hp[s])), 30)), 16, 15, 3, 6);   // the shorts' legs
    }
    add_blob(&SH_BALL, J->Hh, &id, 30, 20, 28, 8, 3);
    add_blob(&SH_BOX, padd(J->Hh, m_app(&J->BF, p3(0, B->torso / 2 + 2, 0))), &J->BF, 38, B->torso / 2 + 6, 31, 8, 2);
    add_tube(bf_pt(J, 0, B->torso - 6, 8), bf_pt(J, 0, 12, 34), 3, 3, 5, 6);                  // the lanyard
    add_blob(&SH_BLOB, bf_pt(J, 0, 22, 34), &J->BF, 9, 11, 3, 6, 6);                           // the badge
    for (int s = 0; s < 2; s++) {
        add_blob(&SH_JNT, J->S[s], &id, 14, 14, 14, 6, 2);
        add_tube(J->S[s], padd(J->S[s], pm(pnorm(psub(J->E[s], J->S[s])), 20)), 14, 13, 2, 6);    // the sleeve
        add_tube(padd(J->S[s], pm(pnorm(psub(J->E[s], J->S[s])), 18)), J->E[s], 10, 9, P_SKIN, 6);
        add_tube(J->E[s], J->Wr[s], 9, 8, P_SKIN, 6);
        hand_inst(J, s, &J->BF, 9, 13, 11, P_SKIN);
    }
    head_inst(&SH_HEAD, J, 76, 72, 70, P_SKIN);
    for (int s = 0; s < 2; s++) add_blob(&SH_BLOB, hf_pt(J, sgn[s] * 73, -6, -2), &J->HF, 7, 12, 9, 6, P_SKIN);
    Inst *I;
    I = inst_add(&SH_CAP, hf_pt(J, 0, 6, -4), &J->HF, 82, 76, 78, 12, P_HAIR); if (I) { I->a0 = 700, I->span = 2700; I->bias = 2; }
    add_blob(&SH_BALL, hf_pt(J, 0, 62, 12), &J->HF, 66, 26, 62, 8, P_HAIR)->bias = 2;
    { P3 a = hf_pt(J, 0, 80, 0), b = hf_pt(J, 5, 100, 8); add_tube(a, b, 4, 2, P_HAIR, 5)->bias = 2; }
    (void)p;
}

// ---- faces ----
typedef struct { s16 ex, ey, ea, eb, my, mw, ny; u8 style; u32 iris, iris2, brow, cheek; } FaceK;
static const FaceK face_k[A_N] = {
    { 25, 5, 12, 13, -30, 27, -10, 0, BGR(110, 64, 30), BGR(206, 130, 56), BGR(120, 70, 22), BGR(255, 150, 130) },     // Chad
    { 20, 2, 11, 9, -29, 15, -12, 1, BGR(120, 60, 220), BGR(220, 150, 255), BGR(70, 30, 40), BGR(255, 150, 170) },     // Missy
    { 27, -3, 16, 18, -40, 17, -22, 2, BGR(54, 110, 250), BGR(150, 220, 255), BGR(90, 56, 40), BGR(255, 130, 140) } };  // the Intern
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
static void face_draw(int who, const Expr *e, int blink) {
    const FaceK *K = &face_k[who]; f_begin();
    f_push = (who == A_CHAD ? 3 : who == A_MISSY ? 2 : 3) + g_lod * 2;
    for (int s = 0; s < 2; s++) {
        int sx = sgn[s];
        if (e->blush || who == A_INTERN) f_ell(sx * (K->ex + 5), K->ey - K->eb - 7, 10 + (who == A_INTERN) * 3, 5 + (who == A_INTERN) * 2, 8, K->cheek, 4);
        face_eye(who, sx, e, blink);
        // the brows
        int x0 = sx * (K->ex - K->ea * 9 / 10), x1 = sx * (K->ex + K->ea * 11 / 10), by = K->ey + K->eb + (who == A_INTERN ? 9 : 7);
        int dy0 = 0, dy1 = 0;
        if (e->brow == 1) { dy0 = dy1 = 4; } else if (e->brow == 2) { dy0 = -4; dy1 = 3; } else if (e->brow == 3) { dy0 = 5; dy1 = -1; }
        if (who == A_MISSY && e->brow == 0) { dy0 = -1; dy1 = 3; }
        f_taper(x0, by + dy0, x1, by + dy1, who == A_MISSY ? 2 : 4, who == A_MISSY ? 1 : 3, K->brow, 8);
    }
    f_line(-1, K->ny + 1, 2, K->ny - 1, 2, BGR(190, 110, 100), 6);        // the nose: a small stroke
    face_mouth(who, e->mouth);
    if (e->sweat) { int x = hg.hx * 52 / 100, y = hg.hy * 55 / 100; f_tri(x - 6, y, x + 6, y, x, y + 14, BGR(120, 200, 255), 8); f_ell(x, y, 6, 6, 8, BGR(120, 200, 255), 8); }
}

// ---- the hair that lies on the head (fringes, side-burns), drawn over the face's base: the head's shape under it is in build_* ----
static void hair_decals(int who) {
    const Pal *pl = who == A_CHAD ? pal_chad : who == A_MISSY ? pal_missy : pal_intern; u32 hb = pl[P_HAIR].c[1], hs = pl[P_HAIR].c[0];
    f_begin(); f_push = who == A_INTERN ? 6 : who == A_MISSY ? 6 : 6;
    if (who == A_MISSY) {   // a long swept fringe
        static const s8 bx[6] = { -36, -22, -9, 5, 19, 33 }, ty[6] = { 12, 4, 9, 2, 10, 14 };
        for (int k = 0; k < 6; k++) { int x = bx[k], t = ty[k];
            f_tri(x - 11, 44, x + 4, t - 3, x + 12, 44, hs, 12);
            f_tri(x - 10, 46, x + 5, t + 1, x + 11, 46, hb, 13); }
    } else if (who == A_INTERN) {   // a blunt bowl fringe
        static const s8 bx[7] = { -57, -38, -19, 0, 19, 38, 57 };
        for (int k = 0; k < 7; k++) { int x = bx[k], t = 30 + (k & 1) * 6;
            f_tri(x - 12, 72, x, t - 2, x + 12, 72, hs, 12);
            f_tri(x - 11, 74, x + 1, t + 2, x + 11, 74, hb, 13); }
    } else {   // Chad: side-burns
        for (int s = 0; s < 2; s++) { int x = sgn[s] * 58; f_quad(x - 5, 22, x - 5, -14, x + 5, -14, x + 5, 22, hb, 13); }
    }
}

// ---- one person, drawn: fk, then the list of shapes (ink shell first, then the flat colour), the hair on the head, the face ----
static int g_sheet;       // 1: the model sheet (test): full detail at any distance
static int g_noink;       // 1: no ink shells (a crowded or far frame)
static int g_wpx = 14;    // the ink's weight, tenths of a pixel (heavier in a close-up)
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
        g_ol = CLAMP(Dz * g_wpx / 2300, 2, 8); g_lod = g_sheet ? 0 : Dz > 850 ? 2 : Dz > 500 ? 1 : 0;
    }
    for (int i = 0; i < nil_; i++) inst_draw(&il[i], 0);       // (the colour first, then the ink behind it: a full packet buffer loses the ink, never the person)
    if (!g_noink) for (int i = 0; i < nil_; i++) if (!(il[i].flag & IF_NOOL) && !(il[i].sh == &SH_JNT || (il[i].hx < 14 && il[i].hy < 14 && il[i].hz < 14) || (g_lod == 2 && il[i].hx < 20 && il[i].hy < 20 && il[i].hz < 20))) inst_draw(&il[i], 1);   // (far away, the small parts go without ink)
    hair_decals(who);
    face_draw(who, e, blink);
}
