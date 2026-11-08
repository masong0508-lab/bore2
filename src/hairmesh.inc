// hairmesh.inc - the hair, drawn as a few shaped pieces instead of blocks of the grid (included by voxel.c).
// A style is a short list of pieces (shared tables, nothing per person). A piece is a box that may be narrower at the top and/or the
// bottom (a bevel: stacked pieces make a stepped dome), in QUARTER UNITS of the head: the skull is x -32..32, z -32..32 (+z the face),
// y 0 (the crown of the skull) .. 64 (the chin), y down. The pieces ride on the head's joint, so they follow every turn and nod.
// The grid still holds the hair as blocks (look_port.inc): the crowd's low model and the Live model read those; voxel_prepare drops the
// hair cells here (the head's own cells go back to skin) and this draws instead. The colours: three tones (base, a light one for the
// sheen, a dark one for the layer underneath) from the hair colour, lit with a vertical gradient (darker underneath) and textured
// with strands (a bright band under the crown), points where a cut hangs, bangs with gaps, curls.
typedef struct { s8 x0, x1, y0, y1, z0, z1; u8 in; u8 tex; u16 fl; } HP;   // in: top inset (high nibble), bottom inset (low); tex: 0 strands 1 curls 2 bangs
#define HM   1     // also its mirror image across x
#define HC   2     // crown: not drawn under a hat
#define HNT  4     // no top face
#define HNF  8     // no front face
#define HNB  16    // no back face
#define HNS  32    // no side faces
#define HDK  64    // the dark tone (the layer underneath, the nape)
#define HLT  128   // the light tone (a streak, a sweep)
#define HBH  256   // hangs behind the torso: its back is kept clear of the body
#define HTP  512   // a hanging end: its faces end in points (see-through gaps)
#define HBO  1024  // the underside shows too
#define HPP(x0, x1, y0, y1, z0, z1, ti, bi, fl, tx) { x0, x1, y0, y1, z0, z1, (u8)(((ti) << 4) | (bi)), tx, fl }

// shared shapes
#define RING(yb)      HPP(-35, 35, -3, yb, -35, 35, 4, 0, 0, 0)                  // all round the head from the crown down to the hairline
#define CROWN(h, ti)  HPP(-31, 31, -3 - (h), -3, -31, 31, ti, 0, HC, 0)          // the dome on the ring
#define NAPE(y1)      HPP(-33, 33, 10, y1, -37, -12, 0, 6, HNF | HDK, 0)           // the back of the neck
#define BURN(y1)      HPP(35, 37, 6, y1, 4, 28, 0, 4, HM | HNT | HNB, 0)    // sideburns (and the other side)
#define EARS(y1, z0, z1) HPP(34, 37, -3, y1, z0, z1, 0, 3, HM | HNT | HNF | HNB, 0) // hair over the ears

static const HP hp_crop[] = { RING(10), CROWN(11, 7), NAPE(26), BURN(24) };
static const HP hp_buzz[] = { HPP(-34, 34, -3, 11, -34, 34, 0, 0, HDK, 0), HPP(-31, 31, -8, -3, -31, 31, 6, 0, HC | HDK, 0), HPP(-33, 33, 10, 22, -35, -12, 0, 6, HNF | HDK, 0) };
static const HP hp_bowl[] = { CROWN(13, 8), RING(12), HPP(-34, 34, 6, 16, 32, 39, 0, 0, 0, 2), EARS(30, -34, 22), HPP(-37, 37, 8, 32, -39, -32, 0, 4, HNF, 0) };
static const HP hp_long[] = { RING(10), CROWN(12, 7), HPP(-35, 35, 8, 18, 30, 37, 0, 0, 0, 2),
    HPP(34, 38, 6, 70, -34, 14, 0, 4, HM | HNT | HNF | HNB, 0), HPP(-38, 38, 10, 70, -42, -34, 0, 5, HNF, 0), HPP(-38, 38, 70, 104, -48, -41, 0, 7, HNF | HBH | HTP, 0) };
static const HP hp_spiky[] = { RING(10), HPP(-9, 9, -40, -3, -9, 9, 6, 0, HC, 0), HPP(-26, -10, -34, -3, 8, 24, 6, 0, HC | HM, 0), HPP(-26, -10, -34, -3, -24, -8, 6, 0, HC | HM, 0),
    HPP(-34, -24, -14, 4, -8, 8, 0, 3, HM | HNT, 0), NAPE(24) };
static const HP hp_afro[] = { HPP(-30, 30, -56, -36, -34, 30, 11, 0, HC, 1), HPP(-44, 44, -38, 12, -50, 36, 6, 0, 0, 1), HPP(-47, 47, 12, 32, -50, 8, 0, 6, HNF, 1),
    HPP(-42, 42, 32, 48, -48, -10, 0, 12, HNF | HDK, 1) };   // a big round puff: a dome on a wide block, hanging over the ears and the nape
static const HP hp_flat[] = { HPP(-34, 34, -22, 8, -34, 34, 0, 2, HC, 0), HPP(-34, 34, 8, 24, -35, -14, 0, 4, HNF | HDK, 0), BURN(18) };
static const HP hp_sidetail[] = { RING(10), CROWN(11, 7), NAPE(24), HPP(33, 50, -2, 16, -12, 12, 4, 0, HNB | HLT, 0), HPP(35, 50, 14, 74, -11, 11, 0, 7, HNB | HTP, 0) };
static const HP hp_bun[] = { RING(10), CROWN(11, 7), NAPE(24), HPP(-17, 17, -40, -6, -38, -6, 7, 4, HC, 1), HPP(-18, 18, -12, -7, -39, -5, 0, 0, HC | HLT, 0) };
static const HP hp_bob[] = { CROWN(12, 7), RING(12), HPP(-34, 34, 6, 15, 32, 39, 0, 0, 0, 2),
    HPP(34, 38, 4, 58, -36, 24, 0, 6, HM | HNT | HTP, 0), HPP(-38, 38, 10, 58, -42, -34, 0, 7, HNF | HTP, 0) };
static const HP hp_pony[] = { RING(10), CROWN(12, 7), NAPE(22), HPP(-8, 8, -4, 12, -46, -34, 2, 2, HLT, 0), HPP(-9, 9, 10, 66, -52, -40, 0, 4, HBH | HTP, 0) };
static const HP hp_pigs[] = { RING(10), CROWN(12, 7), NAPE(22), HPP(36, 46, -2, 12, -10, 10, 3, 3, HM | HLT, 0), HPP(38, 50, 10, 60, -9, 9, 0, 5, HM | HTP, 0) };
static const HP hp_mohawk[] = { HPP(-8, 8, -3, 6, -35, 35, 0, 0, 0, 0), HPP(-6, 6, -34, -3, 17, 31, 3, 0, HC, 0), HPP(-6, 6, -40, -3, -7, 7, 3, 0, HC, 0), HPP(-6, 6, -34, -3, -31, -17, 3, 0, HC, 0),
    HPP(-8, 8, 6, 28, -37, -12, 0, 4, HNF, 0) };
static const HP hp_pixie[] = { RING(9), CROWN(10, 6), HPP(-30, 26, -2, 13, 29, 38, 3, 0, 0, 2), HPP(34, 37, 0, 26, -8, 18, 0, 3, HM | HNT | HNB, 0), HPP(-14, 14, 8, 30, -37, -12, 0, 4, HNF | HDK, 0) };
static const HP hp_curls[] = { HPP(-34, 34, -34, -14, -36, 34, 9, 0, HC, 1), HPP(-40, 40, -16, 12, -42, 37, 5, 0, 0, 1), HPP(-41, 41, 12, 36, -44, -4, 0, 8, HNF, 1), HPP(34, 43, 6, 30, -16, 18, 0, 6, HM | HNT, 1) };
static const HP hp_side[] = { RING(11), CROWN(12, 6), HPP(-31, 8, -16, -10, -22, 35, 3, 0, HC | HLT, 0), HPP(-32, 14, -4, 13, 29, 38, 3, 0, 0, 2), NAPE(26), BURN(24) };
static const HP hp_slick[] = { RING(8), CROWN(14, 8), HPP(-34, 34, 6, 38, -41, -33, 0, 6, HNF, 0), HPP(-31, 31, -20, -3, -34, -8, 4, 0, HC | HLT, 0) };
static const HP hp_braids[] = { RING(10), CROWN(11, 7), NAPE(20), HPP(32, 42, 4, 20, -6, 10, 2, 2, HM, 0), HPP(34, 42, 20, 36, -5, 9, 2, 2, HM | HLT, 0), HPP(32, 42, 36, 52, -6, 10, 2, 2, HM, 0), HPP(34, 40, 52, 62, -4, 8, 0, 4, HM | HTP, 0) };
static const HP hp_wavy[] = { RING(10), CROWN(12, 7), HPP(-35, 35, 8, 18, 30, 37, 0, 0, 0, 2),
    HPP(34, 38, 6, 28, -34, 14, 0, 0, HM | HNT | HNF | HNB, 0), HPP(34, 41, 28, 48, -32, 14, 0, 0, HM | HNT | HNF | HNB, 0), HPP(34, 38, 48, 68, -30, 12, 0, 4, HM | HNT | HNF | HNB | HTP, 0),
    HPP(-38, 38, 10, 40, -42, -34, 0, 0, HNF, 0), HPP(-41, 41, 40, 68, -45, -36, 0, 4, HNF | HBH, 0), HPP(-38, 38, 68, 98, -48, -40, 0, 7, HNF | HBH | HTP, 0) };
static const HP hp_horse[] = { HPP(-12, 12, -3, 8, 30, 36, 2, 0, 0, 2), HPP(-36, 36, -2, 34, -39, -10, 0, 6, HNF, 0), HPP(33, 39, -2, 30, -36, 12, 0, 4, HM | HNT | HNB, 0) };
static const HP hp_thin[] = { HPP(-26, 26, -3, 0, -26, 26, 3, 0, HC, 2), HPP(-34, 34, 2, 30, -37, -12, 0, 5, HNF, 0), HPP(34, 37, 2, 28, -30, 10, 0, 3, HM | HNT | HNB, 0), HPP(-22, 4, -5, -3, -12, 28, 1, 0, HC | HLT, 0) };   // (a thin mat on the crown and one comb-over, the sides kept)
static const HP hp_messy[] = { RING(11), CROWN(12, 5), HPP(-20, -6, -22, -10, -4, 16, 3, 0, HC, 0), HPP(8, 26, -20, -8, -14, 8, 3, 0, HC, 0), HPP(30, 42, -8, 4, -6, 10, 2, 0, HM | HNB, 0), HPP(-30, 22, -2, 14, 29, 39, 3, 0, 0, 2), NAPE(24) };
static const HP hp_knot[] = { RING(10), CROWN(10, 6), NAPE(22), BURN(20), HPP(-12, 12, -30, -12, -12, 12, 4, 3, HC, 1), HPP(-10, 10, -14, -10, -10, 10, 0, 0, HC | HLT, 0) };   // (a close cut with a round knot on top, a tie round its foot)
static const HP hp_curt[] = { CROWN(12, 7), HPP(-35, -2, -3, 16, 31, 38, 0, 3, 0, 2), HPP(2, 35, -3, 16, 31, 38, 0, 3, 0, 2), HPP(-35, 35, -3, 8, -35, 33, 0, 0, 0, 0), EARS(32, -34, 24), HPP(-37, 37, 8, 34, -39, -32, 0, 5, HNF, 0) };
static const HP hp_mullet[] = { RING(10), CROWN(12, 7), HPP(34, 37, 4, 22, -20, 18, 0, 3, HM | HNT | HNF | HNB, 0), HPP(-37, 37, 10, 70, -44, -34, 0, 6, HNF | HTP | HBH, 0) };
static const HP hp_baby[] = { HPP(-12, 12, -9, -3, -12, 12, 4, 0, HC, 0), HPP(-5, 5, -15, -9, 2, 12, 2, 0, HC, 0) };

// beards (the jaw, the cheeks; the long one hangs on the chest): pieces too, drawn whatever the hair
#define BEARD_JAW  HPP(-33, 33, 43, 70, 22, 36, 2, 5, HDK, 0)                              // the jaw and chin, stepped in at its upper edge
#define BEARD_STASH HPP(-18, 18, 35, 43, 30, 38, 0, 2, HDK, 0)                              // the moustache
#define BEARD_CHEEK HPP(34, 37, 24, 62, -4, 30, 5, 3, HM | HNT | HNF | HNB | HDK, 0)         // the sideburns running down into the jaw
static const HP hp_beard[] = { BEARD_JAW, BEARD_STASH, BEARD_CHEEK };
static const HP hp_lbeard[] = { BEARD_JAW, BEARD_STASH, BEARD_CHEEK, HPP(-26, 26, 66, 96, 20, 39, 0, 9, HTP | HDK, 0) };

typedef struct { const HP *p; u8 n; } HStyle;
#define HS(a) { a, (u8)(sizeof(a) / sizeof(HP)) }
static const HStyle hstyles[NHAIR] = {
    HS(hp_crop), HS(hp_bowl), HS(hp_long), { 0, 0 }, HS(hp_spiky), HS(hp_afro), HS(hp_flat), HS(hp_sidetail), HS(hp_bun), HS(hp_bob), HS(hp_pony), HS(hp_pigs), HS(hp_mohawk), HS(hp_pixie), HS(hp_curls),
    HS(hp_buzz), HS(hp_side), HS(hp_slick), HS(hp_braids), HS(hp_wavy), HS(hp_horse), HS(hp_thin), HS(hp_messy), HS(hp_knot), HS(hp_curt), HS(hp_mullet) };
static const HStyle hs_baby = HS(hp_baby);

// ---- the three tones, and a lit-colour table per tone (worked out the first time a face asks, once per person per draw) ----
static u8 htone[3][3];                    // 0 the hair colour, 1 dark (the layer underneath), 2 light (the sheen)
static u32 hlut[3][32]; static u8 hlst[3][32], hlgen = 1;
static void hair_tones(u16 c) {
    int r = R8(c), g = G8(c), b = B8(c), lum = (r * 3 + g * 5 + b * 2) / 10, k = 60 + (lum < 70 ? 30 : lum < 120 ? 14 : 0);
    htone[0][0] = (u8)r; htone[0][1] = (u8)g; htone[0][2] = (u8)b;
    htone[1][0] = (u8)(r * 185 >> 8); htone[1][1] = (u8)(g * 178 >> 8); htone[1][2] = (u8)(b * 205 >> 8);          // darker, a little cooler
    htone[2][0] = (u8)MIN(255, r + ((255 - r) * (k + 10) >> 8)); htone[2][1] = (u8)MIN(255, g + ((255 - g) * k >> 8)); htone[2][2] = (u8)MIN(255, b + ((255 - b) * (k - 10) >> 8));   // lighter, a little warm
    if (++hlgen == 0) { memset(hlst, 0, sizeof hlst); hlgen = 1; }
}
static u32 hair_tone(int t, int L) {
    int i = CLAMP(L, 0, 255) >> 3;
    if (hlst[t][i] == hlgen) return hlut[t][i];
    int Lx = (i * 8 + 4) * 5 / 4 + 10;
    u32 c = gfx_grade_tex(pack((htone[t][0] * Lx) >> 7, (htone[t][1] * Lx) >> 7, (htone[t][2] * Lx) >> 7));
    hlut[t][i] = c; hlst[t][i] = hlgen;
    return c;
}

// ---- one piece ----
static void hp_face(const XV *a, const XV *b, const XV *c, const XV *d, int tone, int L, int gt, int gb, const TexRef *t) {   // one face: lit lighter at the top, darker underneath
    quads += gfx_emit_gt4(a, b, c, d, hair_tone(tone, L + gt), hair_tone(tone, L + gt), hair_tone(tone, L - gb), hair_tone(tone, L - gb), t->tpage, 0, t->u, t->v, t->u + 15, t->v + 15, 1);
}
static void hair_piece(const HP *p, int mir, int cx, int cy, int cz, int hw, int zbh, int idx) {
    int x0 = p->x0, x1 = p->x1, y0 = p->y0, y1 = p->y1, z0 = p->z0, z1 = p->z1, fl = p->fl;
    int ti = p->in >> 4, bi = p->in & 15;
    if (mir) { int t = x0; x0 = -x1; x1 = -t; }
    if (fl & HBH) { if (z1 > zbh) { int d = z1 - zbh; z0 -= d; z1 -= d; } }
    SVECTOR c[8] __attribute__((aligned(8)));
    for (int k = 0; k < 4; k++) {
        int tx = (k & 1) ? x1 - ti : x0 + ti, tz = (k & 2) ? z1 - ti : z0 + ti, bx = (k & 1) ? x1 - bi : x0 + bi, bz = (k & 2) ? z1 - bi : z0 + bi;
        setVector(&c[k], cx + ((tx * hw) >> 5), cy + ((y0 * hw) >> 5), cz + ((tz * hw) >> 5));
        setVector(&c[4 + k], cx + ((bx * hw) >> 5), cy + ((y1 * hw) >> 5), cz + ((bz * hw) >> 5));
    }
    int tone = (fl & HDK) ? 1 : (fl & HLT) ? 2 : 0;
    int hgt = MAX(1, y1 - y0), tilt = CLAMP((ti - bi) * 16 / hgt, 0, 16), var = ((idx * 5) & 3) * 3 - 4;
    int Lt = Lf6[4], gt = MIN(8, hgt), gb = MIN(18, hgt + 4);
    XV xv[8]; gfx_xform(c, xv, 8);
    const XV *t = xv, *bt = xv + 4;
    const TexRef *sd = (p->tex == 1) ? &tx_curl : (p->tex == 2) ? &tx_fringe : (fl & HTP) ? &tx_tips : &tx_side;
    if (!(fl & HNT)) { const TexRef *tt = p->tex == 1 ? &tx_curl : &tx_crown; int L = Lt + var; hp_face(&t[2], &t[3], &t[0], &t[1], tone, L, 6, 6, tt); }
    #define SL(Ls) ((Ls) + ((Lt - (Ls)) * tilt >> 4) + var)
    if (!(fl & HNF)) hp_face(&t[3], &t[2], &bt[3], &bt[2], tone, SL(Lf6[1]), gt, gb, sd);      // front +z
    if (!(fl & HNB)) hp_face(&t[0], &t[1], &bt[0], &bt[1], tone, SL(Lf6[0]), gt, gb, sd);      // back -z
    if (!(fl & HNS)) { hp_face(&t[2], &t[0], &bt[2], &bt[0], tone, SL(Lf6[2]), gt, gb, sd);    // -x
                       hp_face(&t[1], &t[3], &bt[1], &bt[3], tone, SL(Lf6[3]), gt, gb, sd); }  // +x
    #undef SL
    if (fl & HBO) { u32 cc = hair_tone(tone, Lf6[5] - 10); quads += gfx_emit_g4(&bt[3], &bt[2], &bt[1], &bt[0], cc, cc, cc, cc, 1); }
}

static void hair_draw(const Bore *b) {
    int st = b->look[LK_HSTYLE] % NHAIR, hat = b->look[LK_HAT] || (b->look[LK_X_HEAD] % NX_HEAD), bd = b->look[LK_BEARD] % 3;
    const HStyle *hs = b->stage == AG_BABY ? &hs_baby : &hstyles[st];
    int cx = b->h_cx, cz = b->h_cz, cy, hw = b->w_headh;
    { int x = cx, y = b->h_top, z = cz; head_xf(b, &x, &y, &z); cy = y; }
    int zbh = ((b->w_tbz - cz) * 32) / MAX(1, hw) - 3;                // the torso's back, in head quarter units: hanging hair stays behind it
    hair_tones(b->col[5]);
    gfx_set_zbias(3);
    if (b->look[LK_HAT] != 5) for (int i = 0; i < hs->n; i++) {      // (a helmet covers it all)
        const HP *p = &hs->p[i];
        if (hat && (p->fl & HC)) continue;
        hair_piece(p, 0, cx, cy, cz, hw, zbh, i);
        if (p->fl & HM) hair_piece(p, 1, cx, cy, cz, hw, zbh, i + 3);
    }
    if (bd && b->stage > AG_CHILD) for (int i = 0; i < (bd == 2 ? 4 : 3); i++) {
        const HP *p = &(bd == 2 ? hp_lbeard : hp_beard)[i];
        hair_piece(p, 0, cx, cy, cz, hw, zbh, i);
        if (p->fl & HM) hair_piece(p, 1, cx, cy, cz, hw, zbh, i + 3);
    }
    gfx_set_zbias(0);
}
