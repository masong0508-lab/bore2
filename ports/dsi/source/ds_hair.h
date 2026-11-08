// ds_hair.h - the PS1 hair pieces (copied from src/hairmesh.inc by hand; the 26 styles + baby tuft + beards), drawn on the DS as plain shaded boxes by hp_emit() in main.c.
// Units: quarter units of the head: skull x -32..32, z -32..32 (+z the face), y 0 (crown) .. 64 (chin), y DOWN.
typedef struct { s8 x0, x1, y0, y1, z0, z1; u8 in; u8 tex; u16 fl; } HP;   // in: top inset (high nibble), bottom inset (low); tex: 0 strands 1 curls 2 bangs
#define F_HM   1     // also its mirror image across x
#define F_HC   2     // crown: not drawn under a hat
#define F_HNT  4     // no top face
#define F_HNF  8     // no front face
#define F_HNB  16    // no back face
#define F_HNS  32    // no side faces
#define F_HDK  64    // the dark tone (the layer underneath, the nape)
#define F_HLT  128   // the light tone (a streak, a sweep)
#define F_HBH  256   // hangs behind the torso: its back is kept clear of the body
#define F_HTP  512   // a hanging end: its faces end in points (see-through gaps)
#define F_HBO  1024  // the underside shows too
#define HPP(x0, x1, y0, y1, z0, z1, ti, bi, fl, tx) { x0, x1, y0, y1, z0, z1, (u8)(((ti) << 4) | (bi)), tx, fl }

// shared shapes
#define RING(yb)      HPP(-35, 35, -3, yb, -35, 35, 4, 0, 0, 0)                  // all round the head from the crown down to the hairline
#define CROWN(h, ti)  HPP(-31, 31, -3 - (h), -3, -31, 31, ti, 0, F_HC, 0)          // the dome on the ring
#define NAPE(y1)      HPP(-33, 33, 10, y1, -37, -12, 0, 6, F_HNF | F_HDK, 0)           // the back of the neck
#define BURN(y1)      HPP(35, 37, 6, y1, 4, 28, 0, 4, F_HM | F_HNT | F_HNB, 0)    // sideburns (and the other side)
#define EARS(y1, z0, z1) HPP(34, 37, -3, y1, z0, z1, 0, 3, F_HM | F_HNT | F_HNF | F_HNB, 0) // hair over the ears

static const HP hp_crop[] = { RING(10), CROWN(11, 7), NAPE(26), BURN(24) };
static const HP hp_buzz[] = { HPP(-34, 34, -3, 11, -34, 34, 0, 0, F_HDK, 0), HPP(-31, 31, -8, -3, -31, 31, 6, 0, F_HC | F_HDK, 0), HPP(-33, 33, 10, 22, -35, -12, 0, 6, F_HNF | F_HDK, 0) };
static const HP hp_bowl[] = { CROWN(13, 8), RING(12), HPP(-34, 34, 6, 16, 32, 39, 0, 0, 0, 2), EARS(30, -34, 22), HPP(-37, 37, 8, 32, -39, -32, 0, 4, F_HNF, 0) };
static const HP hp_long[] = { RING(10), CROWN(12, 7), HPP(-35, 35, 8, 18, 30, 37, 0, 0, 0, 2),
    HPP(34, 38, 6, 70, -34, 14, 0, 4, F_HM | F_HNT | F_HNF | F_HNB, 0), HPP(-38, 38, 10, 70, -42, -34, 0, 5, F_HNF, 0), HPP(-38, 38, 70, 104, -48, -41, 0, 7, F_HNF | F_HBH | F_HTP, 0) };
static const HP hp_spiky[] = { RING(10), HPP(-9, 9, -40, -3, -9, 9, 6, 0, F_HC, 0), HPP(-26, -10, -34, -3, 8, 24, 6, 0, F_HC | F_HM, 0), HPP(-26, -10, -34, -3, -24, -8, 6, 0, F_HC | F_HM, 0),
    HPP(-34, -24, -14, 4, -8, 8, 0, 3, F_HM | F_HNT, 0), NAPE(24) };
static const HP hp_afro[] = { HPP(-40, 40, -34, -6, -40, 38, 12, 6, F_HC, 1), HPP(-46, 46, -14, 28, -46, 10, 6, 8, F_HNF, 1), HPP(-38, 38, -6, 10, -34, 38, 0, 0, 0, 1),
    HPP(-42, 42, 26, 42, -46, -8, 0, 10, F_HNF | F_HDK, 1) };
static const HP hp_flat[] = { HPP(-34, 34, -22, 8, -34, 34, 0, 2, F_HC, 0), HPP(-34, 34, 8, 24, -35, -14, 0, 4, F_HNF | F_HDK, 0), BURN(18) };
static const HP hp_sidetail[] = { RING(10), CROWN(11, 7), NAPE(24), HPP(34, 46, 2, 14, -10, 8, 3, 0, F_HNB, 0), HPP(36, 46, 12, 62, -9, 7, 0, 5, F_HNB | F_HTP, 0) };
static const HP hp_bun[] = { RING(10), CROWN(11, 7), NAPE(24), HPP(-14, 14, -32, -6, -34, -8, 5, 3, F_HC, 1), HPP(-15, 15, -10, -6, -35, -7, 0, 0, F_HC | F_HLT, 0) };
static const HP hp_bob[] = { CROWN(12, 7), RING(12), HPP(-34, 34, 6, 15, 32, 39, 0, 0, 0, 2),
    HPP(34, 38, 4, 58, -36, 24, 0, 6, F_HM | F_HNT | F_HTP, 0), HPP(-38, 38, 10, 58, -42, -34, 0, 7, F_HNF | F_HTP, 0) };
static const HP hp_pony[] = { RING(10), CROWN(12, 7), NAPE(22), HPP(-8, 8, -4, 12, -46, -34, 2, 2, F_HLT, 0), HPP(-9, 9, 10, 66, -52, -40, 0, 4, F_HBH | F_HTP, 0) };
static const HP hp_pigs[] = { RING(10), CROWN(12, 7), NAPE(22), HPP(36, 46, -2, 12, -10, 10, 3, 3, F_HM | F_HLT, 0), HPP(38, 50, 10, 60, -9, 9, 0, 5, F_HM | F_HTP, 0) };
static const HP hp_mohawk[] = { HPP(-8, 8, -3, 6, -35, 35, 0, 0, 0, 0), HPP(-6, 6, -34, -3, 17, 31, 3, 0, F_HC, 0), HPP(-6, 6, -40, -3, -7, 7, 3, 0, F_HC, 0), HPP(-6, 6, -34, -3, -31, -17, 3, 0, F_HC, 0),
    HPP(-8, 8, 6, 28, -37, -12, 0, 4, F_HNF, 0) };
static const HP hp_pixie[] = { RING(9), CROWN(10, 6), HPP(-30, 26, -2, 13, 29, 38, 3, 0, 0, 2), HPP(34, 37, 0, 26, -8, 18, 0, 3, F_HM | F_HNT | F_HNB, 0), HPP(-14, 14, 8, 30, -37, -12, 0, 4, F_HNF | F_HDK, 0) };
static const HP hp_curls[] = { HPP(-36, 36, -22, 4, -36, 36, 8, 4, F_HC, 1), HPP(-40, 40, -4, 14, -40, 30, 0, 6, 0, 1), HPP(-38, 38, 12, 34, -42, -8, 0, 6, F_HNF, 1), HPP(34, 42, 4, 28, -14, 16, 0, 5, F_HM | F_HNT, 1) };
static const HP hp_side[] = { RING(11), CROWN(12, 6), HPP(-31, 8, -16, -10, -22, 35, 3, 0, F_HC | F_HLT, 0), HPP(-32, 14, -4, 13, 29, 38, 3, 0, 0, 2), NAPE(26), BURN(24) };
static const HP hp_slick[] = { RING(8), CROWN(14, 8), HPP(-34, 34, 6, 38, -41, -33, 0, 6, F_HNF, 0), HPP(-31, 31, -20, -3, -34, -8, 4, 0, F_HC | F_HLT, 0) };
static const HP hp_braids[] = { RING(10), CROWN(11, 7), NAPE(20), HPP(32, 42, 4, 20, -6, 10, 2, 2, F_HM, 0), HPP(34, 42, 20, 36, -5, 9, 2, 2, F_HM | F_HLT, 0), HPP(32, 42, 36, 52, -6, 10, 2, 2, F_HM, 0), HPP(34, 40, 52, 62, -4, 8, 0, 4, F_HM | F_HTP, 0) };
static const HP hp_wavy[] = { RING(10), CROWN(12, 7), HPP(-35, 35, 8, 18, 30, 37, 0, 0, 0, 2),
    HPP(34, 38, 6, 28, -34, 14, 0, 0, F_HM | F_HNT | F_HNF | F_HNB, 0), HPP(34, 41, 28, 48, -32, 14, 0, 0, F_HM | F_HNT | F_HNF | F_HNB, 0), HPP(34, 38, 48, 68, -30, 12, 0, 4, F_HM | F_HNT | F_HNF | F_HNB | F_HTP, 0),
    HPP(-38, 38, 10, 40, -42, -34, 0, 0, F_HNF, 0), HPP(-41, 41, 40, 68, -45, -36, 0, 4, F_HNF | F_HBH, 0), HPP(-38, 38, 68, 98, -48, -40, 0, 7, F_HNF | F_HBH | F_HTP, 0) };
static const HP hp_horse[] = { HPP(-14, 14, -3, 9, 30, 36, 2, 0, 0, 2), HPP(-34, 34, 4, 34, -37, -12, 0, 5, F_HNF, 0), HPP(34, 37, 4, 30, -34, 10, 0, 3, F_HM | F_HNT | F_HNB, 0) };
static const HP hp_thin[] = { HPP(-27, 27, -6, -3, -27, 27, 3, 0, F_HC | F_HLT, 0), HPP(-34, 34, 4, 30, -37, -12, 0, 5, F_HNF, 0), HPP(34, 37, 4, 28, -30, 10, 0, 3, F_HM | F_HNT | F_HNB, 0), HPP(-24, 6, -5, -2, -14, 30, 1, 0, F_HC | F_HLT, 0) };
static const HP hp_messy[] = { RING(11), CROWN(12, 5), HPP(-20, -6, -22, -10, -4, 16, 3, 0, F_HC, 0), HPP(8, 26, -20, -8, -14, 8, 3, 0, F_HC, 0), HPP(30, 42, -8, 4, -6, 10, 2, 0, F_HM | F_HNB, 0), HPP(-30, 22, -2, 14, 29, 39, 3, 0, 0, 2), NAPE(24) };
static const HP hp_knot[] = { HPP(-34, 34, -3, 8, -34, 34, 0, 0, 0, 0), NAPE(22), HPP(-12, 12, -36, -3, -12, 12, 4, 0, F_HC, 1), HPP(-14, 14, -14, -10, -14, 14, 0, 0, F_HC | F_HLT, 0) };
static const HP hp_curt[] = { CROWN(12, 7), HPP(-35, -2, -3, 16, 31, 38, 0, 3, 0, 2), HPP(2, 35, -3, 16, 31, 38, 0, 3, 0, 2), HPP(-35, 35, -3, 8, -35, 33, 0, 0, 0, 0), EARS(32, -34, 24), HPP(-37, 37, 8, 34, -39, -32, 0, 5, F_HNF, 0) };
static const HP hp_mullet[] = { RING(10), CROWN(12, 7), HPP(34, 37, 4, 22, -20, 18, 0, 3, F_HM | F_HNT | F_HNF | F_HNB, 0), HPP(-37, 37, 10, 70, -44, -34, 0, 6, F_HNF | F_HTP | F_HBH, 0) };
static const HP hp_baby[] = { HPP(-12, 12, -9, -3, -12, 12, 4, 0, F_HC, 0), HPP(-5, 5, -15, -9, 2, 12, 2, 0, F_HC, 0) };

// beards (the jaw, the cheeks; the long one hangs on the chest): pieces too, drawn whatever the hair
static const HP hp_beard[] = { HPP(-34, 34, 34, 68, 24, 36, 0, 4, F_HDK, 0), HPP(34, 36, 20, 62, -4, 30, 0, 3, F_HM | F_HNT | F_HNF | F_HNB | F_HDK, 0) };
static const HP hp_lbeard[] = { HPP(-34, 34, 34, 68, 24, 36, 0, 4, F_HDK, 0), HPP(34, 36, 20, 62, -4, 30, 0, 3, F_HM | F_HNT | F_HNF | F_HNB | F_HDK, 0), HPP(-26, 26, 66, 94, 20, 39, 0, 9, F_HTP | F_HDK, 0) };
typedef struct { const HP *p; u8 n; } HStyleD;
#define HSD(a) { a, (u8)(sizeof(a) / sizeof(HP)) }
static const HStyleD hdstyle[26] = {   // the PS1's order (look.c hairNm); 3 is BALD (never asked for)
    HSD(hp_crop), HSD(hp_bowl), HSD(hp_long), { 0, 0 }, HSD(hp_spiky), HSD(hp_afro), HSD(hp_flat), HSD(hp_sidetail), HSD(hp_bun), HSD(hp_bob), HSD(hp_pony), HSD(hp_pigs), HSD(hp_mohawk), HSD(hp_pixie), HSD(hp_curls),
    HSD(hp_buzz), HSD(hp_side), HSD(hp_slick), HSD(hp_braids), HSD(hp_wavy), HSD(hp_horse), HSD(hp_thin), HSD(hp_messy), HSD(hp_knot), HSD(hp_curt), HSD(hp_mullet) };
