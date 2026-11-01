// ds_fight_logic.h: made by tools/dsbuild/mk_dsfight.py from the PS1's src/fight.c (do not edit): BORTUA FIGHTER's rules, line for line.
// The ring is in millimetres, 60 Hz steps, angles 0..255. The drawing and the DS's screens are in ds_fight.c.
static int fframe;   // (the fight's own clock, for the sea and the torches)
static int fight_last_wins, fight_matches;   // (since the coin went in)   // (last: matches won since the coin went in)

// ---- small maths (angles 0..255, sines x256) ----
static inline int fsin(int a) { return isin((a & 255) << 4) >> 4; }
static inline int fcos(int a) { return icos((a & 255) << 4) >> 4; }
static inline int bwrap(int a) { return ((a + 128) & 255) - 128; }
static u32 frng = 0x2545F491u;
static int rnd8(void) { frng = frng * 1103515245u + 12345u; return (int)((frng >> 16) & 255); }
static int bang(int dx, int dz) {   // the angle whose (sin, cos) points along (dx, dz)
    if (!dx && !dz) return 0;
    int a = dx >= 0 ? (dz >= 0 ? 0 : 64) : (dz < 0 ? 128 : 192);
    for (int s = 32; s >= 1; s >>= 1) { int t = a + s; if (fsin(t) * dz - fcos(t) * dx <= 0) a = t; }
    return a & 255;
}
static int bsqrt(int v) { return v <= 0 ? 0 : isqrt(v); }

// ---- the moves ----
#define BZ_R 2600          // the ring's radius (mm)
#define BZ_SEP 700         // the closest two standing fighters get
#define BZ_BODY 260        // how far round a fighter's middle a blow still touches
enum { BS_IDLE, BS_WALK, BS_GUARD, BS_ATK, BS_HIT, BS_BLOCK, BS_JUMP, BS_KO, BS_WIN, BS_THROWN };
enum { FM_JAB, FM_KICK, FM_PALM, FM_SPIN, FM_THROW, FM_JKICK, FM_LOW, FM_N };
typedef struct { const char *nm; u8 start, act, rec, dmg, stun, kb, lvl; u16 reach; u8 arc; } Mv;   // wind-up, hit and recovery frames; damage, stun, knock-back; level (0 high 1 mid 2 low 3 throw); reach (mm); arc (+-, of 256)
static const Mv mvs[FM_N] = {
    { "JAB", 3, 3, 8, 6, 14, 2, 0, 950, 40 }, { "KICK", 6, 4, 12, 9, 18, 4, 1, 1200, 40 }, { "BORE PALM", 9, 4, 16, 15, 26, 10, 1, 1000, 40 },
    { "SPIN KICK", 11, 5, 18, 14, 26, 12, 0, 1250, 70 }, { "THROW", 3, 2, 22, 18, 40, 14, 3, 700, 50 }, { "JUMP KICK", 4, 8, 6, 8, 18, 4, 0, 1050, 45 },
    { "SWEEP", 7, 4, 14, 8, 22, 3, 2, 1150, 45 } };
#define EV_BLOCK 1
#define EV_HIT 2
#define EV_BIG 4
#define EV_KO 8
#define EV_THROW 16
#define EV_SWING 32
#define EV_LAND 64
typedef struct {
    s32 x, z; int y, vy; s32 kx, kz, jx, jz;
    int yaw, spin, hp, trail, trailT, st, t, stun, mv, wins, cnt, flash, combo, mx, mz, arm, jatk, fall, done, crouch;
    int aiT, aiDo, aiSide, ev, hx, hy, hz;
    int pw, sp, d;
    int lie;          // 0..4096: the K.O.'s lie-down (eased)
} Fz;
typedef struct { int mx, mz; u8 a, b, pa, pb, pj, guard, down; } FIn;
static Fz F[2];
static const char *call; static int callT; static char callB[16];
static int freeSt(const Fz *f) { return f->st == BS_IDLE || f->st == BS_WALK || f->st == BS_GUARD; }

static void freset(Fz *f, s32 x, s32 z, int yaw) {
    int pw = f->pw, sp = f->sp, d = f->d, w = f->wins; memset(f, 0, sizeof *f); f->pw = pw; f->sp = sp; f->d = d; f->wins = w;
    f->x = x; f->z = z; f->yaw = yaw & 255; f->hp = f->trail = 100;
}
static void numcall(const char *suffix, int n) { snprintf(callB, sizeof callB, "%d%s", n, suffix); call = callB; callT = 40; }
static int land(Fz *a, Fz *o, int mv) {   // a blow's active frames: 1 = it connected (a hit or a block)
    const Mv *m = &mvs[mv]; int dx = (int)(o->x - a->x), dz = (int)(o->z - a->z), d = bsqrt(dx * dx + dz * dz), ang = bang(dx, dz);
    if (d > m->reach + BZ_BODY) return 0;
    if (bwrap(ang - a->yaw) > m->arc || bwrap(ang - a->yaw) < -m->arc) return 0;   // (a sidestep beats a straight blow)
    if (o->st == BS_KO || o->st == BS_THROWN) return 0;
    if (m->lvl == 3 && (o->y > 120 || o->st == BS_JUMP)) return 0;                // (nobody throws a fighter in the air)
    if (m->lvl == 0 && o->y > 600 && mv != FM_JKICK) return 0;                     // (high blows sail under a high jump)
    if (m->lvl == 0 && o->crouch && mv != FM_JKICK) return 0;                      // (and over a crouch)
    if (m->lvl == 2 && o->y > 200) return 0;                                       // (a sweep under a jump)
    static const s16 hy[4] = { 1500, 1150, 350, 1150 };
    a->hx = (int)o->x; a->hz = (int)o->z; a->hy = o->y + hy[m->lvl] - (o->crouch ? 450 : 0);
    int guard = (o->st == BS_GUARD || o->st == BS_BLOCK) && m->lvl != 3 && (m->lvl != 2 || o->crouch) && (m->lvl == 2 || !o->crouch || m->lvl == 1);
    if (guard) { int back = bwrap((ang + 128) - o->yaw); if (back > 70 || back < -70) guard = 0; }   // (a guard covers the front)
    int S = fsin(ang), C = fcos(ang);
    if (guard) { o->st = BS_BLOCK; o->t = 8; o->kx = S * m->kb * 5 >> 8; o->kz = C * m->kb * 5 >> 8; a->ev |= EV_BLOCK; return 1; }
    int dmg = m->dmg * a->pw / 100;
    if (o->st == BS_ATK && o->t < mvs[o->mv].start) { dmg = dmg * 3 / 2; call = "COUNTER!"; callT = 40; }
    if (o->st == BS_HIT) { o->combo++; numcall(" HITS", o->combo + 1); } else o->combo = 0;
    o->hp = o->hp > dmg ? o->hp - dmg : 0;
#ifdef BORE_DC
    { extern void dc_rumble(int, int); dc_rumble(dmg > 20 ? 7 : dmg > 10 ? 5 : 3, 6); }   // (Dreamcast: the Jump Pack feels the blow)
#endif
    o->trailT = 24; o->flash = 4; o->crouch = 0;
    o->st = m->lvl == 3 ? BS_THROWN : BS_HIT; o->t = o->stun = m->stun; o->kx = S * m->kb * 13 >> 8; o->kz = C * m->kb * 13 >> 8; o->jatk = 0;
    a->ev |= EV_HIT; if (m->dmg >= 14) a->ev |= EV_BIG;
    if (m->lvl == 3) { call = "THROW!"; callT = 40; a->ev |= EV_THROW; o->yaw = (a->yaw + 128) & 255; }
    if (mv == FM_PALM || mv == FM_SPIN || mv == FM_LOW) { call = m->nm; callT = 45; }
    if (o->hp == 0) { o->st = BS_KO; o->t = 0; o->kx = S * (m->kb + 6) * 13 >> 8; o->kz = C * (m->kb + 6) * 13 >> 8; a->ev |= EV_BIG | EV_KO; }
    return 1;
}
static void fstep(Fz *f, Fz *o, const FIn *in) {
    int dx = (int)(o->x - f->x), dz = (int)(o->z - f->z), dist = bsqrt(dx * dx + dz * dz), ang = bang(dx, dz), S = fsin(ang), C = fcos(ang);
    f->cnt++; if (f->flash > 0) f->flash--;
    if (f->trailT > 0) f->trailT--; else if (f->trail > f->hp) f->trail--;
    if (f->trail < f->hp) f->trail = f->hp;
    int turn = 0;
    if (f->st == BS_KO) { f->t++; f->lie = MIN(4096, f->lie + 300); }
    else if (f->st == BS_WIN) { f->t++; turn = 4; }
    else if (f->st == BS_HIT || f->st == BS_BLOCK || f->st == BS_THROWN) { if (--f->t <= 0) { f->st = f->y > 0 ? BS_JUMP : BS_IDLE; f->combo = 0; f->jatk = 0; } }
    else if (f->st == BS_ATK) { const Mv *m = &mvs[f->mv]; f->t++;
        if (f->t <= m->start) turn = 3;
        if (f->t == 1) f->ev |= EV_SWING;
        if (f->t > m->start && f->t <= m->start + m->act && !f->done) f->done = land(f, o, f->mv);
        if (f->mv == FM_SPIN) { int span = m->start + m->act; f->spin = f->t < span ? f->t * 256 / span : 0; }
        if (f->t >= m->start + m->act + m->rec) { f->st = BS_IDLE; f->spin = 0; if (f->mv == FM_LOW) f->crouch = in->down; } }
    else if (f->st == BS_JUMP) { turn = 3;
        if (!f->jatk && (in->pa || in->pb)) { f->jatk = 1; f->done = 0; f->mv = FM_JKICK; f->t = 0; f->arm = in->pb ? 1 : 0; f->ev |= EV_SWING; }
        if (f->jatk) { f->t++; if (f->t > mvs[FM_JKICK].start && f->t <= mvs[FM_JKICK].start + mvs[FM_JKICK].act && !f->done) f->done = land(f, o, FM_JKICK); } }
    else {   // free to act
        turn = 7; int fdot = (in->mx * S + in->mz * C) >> 8, fwd = fdot > 100, back = fdot < -100, mv = in->mx || in->mz;
        f->mx = in->mx; f->mz = in->mz; f->crouch = in->down && f->y == 0;
        int ab = (in->pa && in->b) || (in->pb && in->a);
        if (ab && dist <= mvs[FM_THROW].reach + BZ_BODY) { f->st = BS_ATK; f->mv = FM_THROW; f->t = 0; f->jatk = 0; f->done = 0; f->crouch = 0; }
        else if (in->pb && f->crouch) { f->st = BS_ATK; f->mv = FM_LOW; f->t = 0; f->done = 0; }
        else if (in->pa) { f->st = BS_ATK; f->mv = fwd ? FM_PALM : FM_JAB; f->t = 0; f->done = 0; f->arm = fwd ? 0 : (f->arm ^ 1); f->crouch = 0; }
        else if (in->pb) { f->st = BS_ATK; f->mv = back ? FM_SPIN : FM_KICK; f->t = 0; f->done = 0; f->arm ^= 1; f->crouch = 0; }
        else if (in->pj && f->y == 0) { f->st = BS_JUMP; f->vy = 62; f->jatk = 0; f->crouch = 0; f->jx = in->mx * f->sp / 128; f->jz = in->mz * f->sp / 128; }
        else if (in->guard) f->st = BS_GUARD;
        else if (mv && !f->crouch) f->st = BS_WALK; else f->st = BS_IDLE;
        if (f->st == BS_WALK) { int spd = f->sp * 2; if (back) spd = spd * 4 / 5; f->x += in->mx * spd >> 8; f->z += in->mz * spd >> 8; }
    }
    if (turn) { int dy = bwrap(ang - f->yaw); dy = CLAMP(dy, -turn, turn); f->yaw = (f->yaw + dy) & 255; }
    if (f->st == BS_JUMP) { f->y += f->vy; f->vy -= 3; f->x += f->jx; f->z += f->jz; if (f->y <= 0) { f->y = 0; f->vy = 0; f->jx = f->jz = 0; f->st = BS_IDLE; f->jatk = 0; f->ev |= EV_LAND; } }
    else if (f->st == BS_THROWN) { int p = f->stun > 0 ? (f->stun - f->t) * 256 / f->stun : 256, q = p < 128 ? p : 256 - p; f->y = q * 5; f->x += f->kx; f->z += f->kz; f->kx = f->kx * 14 / 16; f->kz = f->kz * 14 / 16; if (f->t <= 1) { f->y = 0; f->ev |= EV_LAND; } }
    else if (f->kx || f->kz) { f->x += f->kx; f->z += f->kz; f->kx = f->kx * 13 / 16; f->kz = f->kz * 13 / 16; if (ABS(f->kx) < 4) f->kx = 0; if (ABS(f->kz) < 4) f->kz = 0; }
    { int rr = bsqrt((int)(f->x / 4) * (int)(f->x / 4) + (int)(f->z / 4) * (int)(f->z / 4)) * 4;   // the edge of the ring
      if (!f->fall) {
          if (freeSt(f) || f->st == BS_ATK || f->st == BS_JUMP) { if (rr > BZ_R - 120) { f->x = f->x * (BZ_R - 120) / rr; f->z = f->z * (BZ_R - 120) / rr; } }   // (on your feet you never step off)
          else if (rr > BZ_R + 120 && f->y <= 0) { f->fall = 1; f->st = BS_KO; f->t = 0; f->vy = 0; f->kx /= 2; f->kz /= 2; } }               // (a blow carried you over: down you go)
      else { f->y += f->vy; f->vy -= 4; f->x += f->kx; f->z += f->kz; f->kx = f->kx * 13 / 16; f->kz = f->kz * 13 / 16; if (f->y < -2200) f->y = -2200; } }
    if (dist < BZ_SEP && f->st != BS_KO && o->st != BS_KO && f->y < 300 && o->y < 300) { int push = (BZ_SEP - dist) / 2 + 1; f->x -= S * push >> 8; f->z -= C * push >> 8; o->x += S * push >> 8; o->z += C * push >> 8; }
}
static void orbit_vec(const Fz *f, const Fz *o, int uy, int ux, int *mx, int *mz) {
    int a = bang((int)(o->x - f->x), (int)(o->z - f->z)), S = fsin(a), C = fcos(a);
    *mx = uy * S + ux * C; *mz = uy * C - ux * S; if (uy && ux) { *mx = *mx * 181 >> 8; *mz = *mz * 181 >> 8; }
}
static void ai(Fz *f, const Fz *o, FIn *in) {   // approach, circle, poke, guard what it sees coming, now and then a special
    memset(in, 0, sizeof *in);
    if (o->st == BS_KO || o->st == BS_WIN || f->st == BS_KO) return;
    int dx = (int)(o->x - f->x), dz = (int)(o->z - f->z), d = bsqrt(dx * dx + dz * dz), lv = f->d, uy = 0, ux = 0;
    if (o->st == BS_ATK && o->t <= mvs[o->mv].start && d <= mvs[o->mv].reach + BZ_BODY + 300 && f->aiDo != 2 && f->aiDo != 11) {
        int r = rnd8() * 100 >> 8;
        if (r < 14 + lv * 8) { f->aiDo = o->mv == FM_LOW ? 12 : 2; f->aiT = 10 + (rnd8() & 7); }
        else if (r < 26 + lv * 8) { f->aiDo = 11; f->aiSide = (rnd8() & 1) ? 1 : -1; f->aiT = 9 + (rnd8() & 5); } }
    if (f->aiT > 0) f->aiT--;
    else { int r = rnd8() * 100 >> 8; f->aiT = (18 - lv) + (rnd8() & 15); f->aiSide = (rnd8() & 1) ? 1 : -1;
        if (d > 2300) f->aiDo = r < 80 ? 1 : (lv >= 2 ? 6 : 1);
        else if (d > 1500) f->aiDo = r < 42 ? 1 : r < 58 ? 3 : (r < 68 && lv >= 2) ? 4 : r < 84 ? 8 : 0;
        else f->aiDo = r < 26 ? 7 : r < 42 ? 3 : r < 52 ? 8 : (r < 60 && lv >= 1) ? 9 : (r < 66 && lv >= 3) ? 5 : (r < 72 && lv >= 2) ? 13 : r < 80 ? 0 : r < 92 ? 10 : 2; }
    switch (f->aiDo) {
        case 1: uy = 1; break;
        case 2: in->guard = 1; break;
        case 3: in->b = in->pb = 1; f->aiDo = 0; break;
        case 4: uy = 1; in->a = in->pa = 1; f->aiDo = 0; break;
        case 5: uy = -1; in->b = in->pb = 1; f->aiDo = 0; break;
        case 6: uy = 1; in->pj = 1; f->aiDo = 0; break;
        case 7: in->a = in->pa = 1; f->aiDo = 0; break;
        case 8: ux = f->aiSide; if (d > 1300) uy = 1; break;
        case 9: in->a = in->b = 1; in->pa = 1; f->aiDo = 0; break;
        case 10: uy = -1; break;
        case 11: ux = f->aiSide; break;
        case 12: in->guard = 1; in->down = 1; break;                             // a low guard
        case 13: in->down = 1; in->b = in->pb = f->crouch; if (f->crouch) f->aiDo = 0; break;   // crouch, then sweep
    }
    orbit_vec(f, o, uy, ux, &in->mx, &in->mz);
}

// ---- the poses: every joint of the voxel Bore (4096 a full turn; see voxel.h VPose) ----
enum { PZ_IDLE, PZ_GUARD, PZ_JAB_A, PZ_JAB_B, PZ_KICK_A, PZ_KICK_B, PZ_PALM, PZ_SPIN, PZ_THROW, PZ_HIT, PZ_JUMP, PZ_WIN, PZ_CROUCH, PZ_SWEEP, PZ_N };
#define P(a0,a1,r0,r1,e0,e1, l0,l1,lr0,lr1,k0,k1, ln,tw,nd,tl) { {a0,a1},{r0,r1},{e0,e1},{l0,l1},{lr0,lr1},{k0,k1},ln,tw,nd,tl,0,0 }
static const VPose pz[PZ_N] = {
 /* IDLE   */ P(-760,-620,-170,170,-1500,-1650,  -300,260,-90,90,320,420,   70,-140,40,0),
 /* GUARD  */ P(-1150,-1150,60,-60,-1750,-1750, -300,260,-90,90,460,520,  130,-40,140,0),
 /* JAB A  */ P(-760,-420,-170,190,-1500,-1950,  -300,260,-90,90,320,420,   60,-260,40,0),
 /* JAB B  */ P(-700,-1060,-150,40,-1600,-40,    -330,280,-90,90,380,360,  170,260,20,0),
 /* KICK A */ P(-820,-760,-260,260,-1500,-1550,  0,-1100,-60,40,80,1650,   -60,-60,60,0),
 /* KICK B */ P(-520,-300,-560,560,-1100,-900,   120,-1140,-60,30,120,60,  -220,-120,80,0),
 /* PALM   */ P(-1030,-1030,130,-130,-120,-120,  -560,420,-60,60,640,180,  260,0,-20,0),
 /* SPIN   */ P(-300,-300,-950,950,-400,-400,    40,-260,-40,880,80,60,    -150,0,60,0),
 /* THROW  */ P(-880,-880,170,-170,-700,-700,    -420,300,-80,80,560,420,  220,0,40,0),
 /* HIT    */ P(-260,-260,-420,420,-800,-800,    -120,140,-90,90,320,300,  -360,0,-260,120),
 /* JUMP   */ P(-640,-640,-500,500,-900,-900,    -900,-640,-90,90,1650,1500, 40,0,40,0),
 /* WIN    */ P(-320,-2300,-320,220,-1200,-100,  0,0,-110,110,60,60,       -60,0,-170,0),
 /* CROUCH */ P(-900,-800,-120,120,-1650,-1700,  -900,-200,-120,120,1700,1500, 180,-100,120,0),
 /* SWEEP  */ P(-500,-500,-700,700,-900,-900,    -1000,-900,-120,1000,1750,120, 260,0,140,0) };
#undef P
static const u8 mvA[FM_N] = { PZ_JAB_A, PZ_KICK_A, PZ_PALM, PZ_SPIN, PZ_THROW, PZ_KICK_A, PZ_CROUCH }, mvB[FM_N] = { PZ_JAB_B, PZ_KICK_B, PZ_PALM, PZ_SPIN, PZ_THROW, PZ_KICK_B, PZ_SWEEP };
static void plerp(VPose *d, const VPose *a, const VPose *b, int k) { const s16 *aa = (const s16 *)a, *bb = (const s16 *)b; s16 *dd = (s16 *)d; for (int i = 0; i < (int)(sizeof(VPose) / 2); i++) dd[i] = (s16)(aa[i] + (((bb[i] - aa[i]) * k) >> 8)); }
static void pmirror(VPose *p) {   // left for right
    s16 t;
    #define SW(f) t = p->f[0]; p->f[0] = p->f[1]; p->f[1] = t;
    SW(arm) SW(elbow) SW(leg) SW(knee)
    t = p->aroll[0]; p->aroll[0] = (s16)-p->aroll[1]; p->aroll[1] = (s16)-t;
    t = p->lroll[0]; p->lroll[0] = (s16)-p->lroll[1]; p->lroll[1] = (s16)-t;
    #undef SW
    p->twist = (s16)-p->twist; p->tilt = (s16)-p->tilt;
}
static void pose_of(const Fz *f, VPose *o) {
    const VPose *idle = &pz[f->crouch ? PZ_CROUCH : PZ_IDLE];
    switch (f->st) {
    case BS_GUARD: if (f->crouch) { *o = pz[PZ_CROUCH]; o->arm[0] = o->arm[1] = -1200; o->elbow[0] = o->elbow[1] = -1800; } else *o = pz[PZ_GUARD]; break;
    case BS_BLOCK: plerp(o, &pz[f->crouch ? PZ_CROUCH : PZ_GUARD], &pz[PZ_HIT], 50); break;
    case BS_JUMP: if (f->jatk) { const Mv *m = &mvs[FM_JKICK]; int k = f->t < m->start ? f->t * 256 / m->start : 256; VPose a = pz[PZ_JUMP], b = pz[PZ_KICK_B]; if (f->arm) { pmirror(&a); pmirror(&b); } plerp(o, &a, &b, k); }
                  else { int k = CLAMP(f->vy * 2 + 128, 0, 256); plerp(o, &pz[PZ_JUMP], idle, 256 - k > 200 ? 200 : 256 - k); plerp(o, &pz[PZ_JUMP], o, 60); } break;
    case BS_ATK: { const Mv *m = &mvs[f->mv]; VPose a = pz[mvA[f->mv]], b = pz[mvB[f->mv]], id = *idle; int t = f->t, half = m->start / 2;
        if (f->arm && f->mv != FM_PALM && f->mv != FM_SPIN && f->mv != FM_THROW) { pmirror(&a); pmirror(&b); }
        if (t <= half) plerp(o, &id, &a, half ? t * 256 / half : 256);
        else if (t < m->start) plerp(o, &a, &b, (t - half) * 256 / (m->start - half));
        else if (t < m->start + m->act) *o = b;
        else plerp(o, &b, &id, (t - m->start - m->act) * 256 / m->rec);
        break; }
    case BS_HIT: { int k = f->stun - f->t; k = k < 4 ? k * 64 : 256; if (f->t < 6) k = f->t * 40; plerp(o, idle, &pz[PZ_HIT], CLAMP(k, 0, 256)); break; }
    case BS_THROWN: case BS_KO: *o = pz[PZ_HIT]; break;
    case BS_WIN: { *o = pz[PZ_WIN]; int w = isin((f->t * 80) & 4095) >> 5; o->arm[1] = (s16)(o->arm[1] + w); break; }
    case BS_WALK: { *o = *idle;
        int fm = (f->mx * fsin(f->yaw) + f->mz * fcos(f->yaw)) >> 8, sm = (f->mx * fcos(f->yaw) - f->mz * fsin(f->yaw)) >> 8;   // forward and sideways parts of the step
        int ph = (f->cnt * 9) & 255, sw = fsin(ph) * 380 >> 8, sw2 = fsin(ph + 128) * 380 >> 8;
        o->leg[0] = (s16)(o->leg[0] + (sw * fm >> 8)); o->leg[1] = (s16)(o->leg[1] + (sw2 * fm >> 8));
        o->lroll[0] = (s16)(o->lroll[0] - ABS(sw * sm >> 8) / 2); o->lroll[1] = (s16)(o->lroll[1] + ABS(sw2 * sm >> 8) / 2);
        o->knee[0] = (s16)(o->knee[0] + MAX(0, fcos(ph)) * 400 / 256); o->knee[1] = (s16)(o->knee[1] + MAX(0, fcos(ph + 128)) * 400 / 256);
        break; }
    default: *o = *idle; { int bob = fsin(f->cnt * 5) * 60 >> 8; o->knee[0] = (s16)(o->knee[0] + bob); o->knee[1] = (s16)(o->knee[1] + bob); o->elbow[1] = (s16)(o->elbow[1] - bob); }
    }
}

// ---- the camera ----
enum { CAM_SIDE, CAM_CHASE, CAM_HIGH, CAM_N };
static const char *const camNm[CAM_N] = { "SIDE", "CHASE", "HIGH" };
static int cam_mode, cyaw, cdist, cheight;   // cyaw 0..255 the way the camera looks (in fight angles)
static s32 cax, caz;
static void cam_follow(int snap) {
    const Fz *p = &F[0], *o = &F[1];
    int dx = (int)(o->x - p->x), dz = (int)(o->z - p->z), d = bsqrt(dx * dx + dz * dz), L = bang(dx, dz);
    s32 ax = (p->x + o->x) / 2, az = (p->z + o->z) / 2; int ty, th, dd;
    if (cam_mode == CAM_CHASE) { ty = L; th = 1600; dd = 1500 + d / 6; ax = p->x + (dx >> 2); az = p->z + (dz >> 2); }
    else if (cam_mode == CAM_HIGH) { ty = (L - 64) & 255; th = 3600; dd = 2600 + d / 2; }
    else { ty = (L - 64) & 255; th = 1800; dd = 1300 + d * 2 / 5; }
    if (snap) { cyaw = ty; cax = ax; caz = az; cdist = dd; cheight = th; return; }
    int dy = bwrap(ty - cyaw); dy = CLAMP(dy, -5, 5); cyaw = (cyaw + dy) & 255;
    cax += (ax - cax) / 6; caz += (az - caz) / 6; cdist += (dd - cdist) / 8; cheight += (th - cheight) / 8;
}
#define MM(v) ((int)(v) * 7 / 32)   // millimetres to world units (up and down)
#define MX(v) ((int)(v) * 7 / 16)   // ... and along the ground: twice as far, as a voxel Bore is twice as broad as a person
static int shake;

typedef struct { const char *nm; u16 seed; u16 top, bot, hair; u8 pw, sp, d; const char *intro; } Who;
static const Who ladder[9] = {
    { "AKIRA YAWNKI",   11, C15(29,29,28), C15(29,29,28), C15(3,3,4),    100, 16, 0, "I TRAINED IN MY SLEEP" },
    { "PAI CHAN-MEH",   23, C15(6,12,28),  C15(6,12,28),  C15(3,3,4),     90, 18, 1, "FAST AS A NAP" },
    { "LAU SNOOZE",     37, C15(28,26,20), C15(9,9,11),   C15(28,28,28), 105, 15, 2, "THE OLD WAYS ARE THE DULL WAYS" },
    { "WOLF HAWKYAWN",  41, C15(22,4,4),   C15(4,6,14),   C15(20,12,4),  120, 13, 3, "I WRESTLE BEARS. THEY GET BORED" },
    { "JEFFRY MCSNORE", 53, C15(28,22,6),  C15(10,8,4),   C15(4,3,2),    125, 12, 4, "THE SEA TAUGHT ME TO WAIT" },
    { "KAGE-MARU-ZZZ",  67, C15(10,4,16),  C15(10,4,16),  C15(2,2,3),    100, 19, 5, "YOU WILL NOT SEE ME COMING" },
    { "SARAH BORYANT",  71, C15(28,6,6),   C15(4,4,6),    C15(31,26,10), 105, 18, 6, "I KICK FIRST. I ASK NEVER" },
    { "JACKY BORYANT",  83, C15(4,4,6),    C15(28,6,6),   C15(31,26,10), 110, 17, 7, "MY SISTER WARNED YOU" },
    { "BORAL",          97, 0, 0, 0,                                     135, 18, 9, "..." } };   // (chrome all over)
static int you_i;                   // which of the household you fight as
static const char *const styleNm[4] = { "BALANCED", "POWER", "SPEED", "TECHNIQUE" };
static const char *const styleTx[4] = { "A BIT OF EVERYTHING", "HITS HARD, MOVES SLOW", "QUICK FEET, LIGHT HANDS", "THE AI GUARDS LESS (AND SO DO YOU)" };
static int style;

// ---- the match ----
enum { FS_PICK, FS_VS, FS_INTRO, FS_FIGHT, FS_END, FS_RESULT, FS_OVER, FS_CHAMP, FS_PAUSE };
static int fs, ft, stage, round_n, secs, sub, winner, credits, slowmo, hitstop, flash;
static const char *why, *banner;
static int pick_row;
static struct { int x, y, t, blk; s32 wx, wy, wz; } spark[4];
static int sp_n;
static void set_stats(void) {
    const Sim *s = &sims[you_i];
    int belt = 0, body = 0; (void)s;   // (the DS has no dojo belts or skills)
    static const s8 spw[4] = { 0, 18, -10, 0 }, ssp[4] = { 0, -2, 3, 1 };
    F[0].pw = 100 + spw[style] + belt * 4 + body; F[0].sp = 16 + ssp[style]; F[0].d = 0;
    const Who *w = &ladder[stage]; F[1].pw = w->pw; F[1].sp = w->sp; F[1].d = MIN(9, w->d + (style == 3 ? 0 : 0));
}
static void round_begin(void) {
    freset(&F[0], -900, 0, 64); freset(&F[1], 900, 0, 192);
    call = 0; callT = 0; secs = 30; sub = 0; winner = -1; why = 0; slowmo = hitstop = flash = 0; sp_n = 0;
    cam_follow(1); fs = FS_INTRO; ft = 0;
    static char rb[16]; if (round_n >= 3) banner = "FINAL ROUND"; else { snprintf(rb, sizeof rb, "ROUND %d", round_n); banner = rb; }
    voice(V_YEHA);
}
static void match_begin(void) { fight_matches++; F[0].wins = F[1].wins = 0; round_n = 1; build_them(stage); set_stats(); round_begin(); }
static void fight_open(int cr) {
    
    credits = cr; fight_last_wins = 0; fight_matches = 0; stage = 0; style = 0; you_i = ctl; pick_row = 0; cam_mode = CAM_SIDE;
    fs = FS_PICK; ft = 0;
    build_them(0); set_stats();
    freset(&F[0], -900, 0, 64); freset(&F[1], 900, 0, 192); cam_follow(1);
}
static void add_spark(const Fz *a, int blk) {
    int i = sp_n++ & 3; spark[i].wx = a->hx; spark[i].wy = a->hy; spark[i].wz = a->hz; spark[i].t = 14; spark[i].blk = blk;
}
static void events(Fz *a) {
    int e = a->ev; a->ev = 0; if (!e) return;
    if (e & EV_BLOCK) { add_spark(a, 1); hitstop = 3; }
    if (e & EV_HIT) { add_spark(a, 0); hitstop = (e & EV_BIG) ? 8 : 4; shake = (e & EV_BIG) ? 3 : 1;
                      if (e & EV_BIG) voice(a == &F[0] ? V_GROAN : V_SHRIEK); }
    if (e & EV_KO) { slowmo = 90; flash = 8; voice(V_SHRIEK); }
    if ((e & EV_SWING) && a->st == BS_ATK && (a->mv == FM_PALM || a->mv == FM_SPIN)) voice(V_YEHA);
}
static void round_over(int w, const char *wh) {
    winner = w; why = wh; fs = FS_END; ft = 0;
    if (w >= 0) { F[w].wins++; if (F[w].st != BS_KO) { F[w].st = BS_WIN; F[w].t = 0; } }
}
static void read_input(int held, int pressed, FIn *in) {
    memset(in, 0, sizeof *in);
    int ux = (held & PAD_RIGHT ? 1 : 0) - (held & PAD_LEFT ? 1 : 0), uy = (held & PAD_UP ? 1 : 0) - (held & PAD_DOWN ? 1 : 0);
    if (cam_mode == CAM_CHASE) { orbit_vec(&F[0], &F[1], uy, ux, &in->mx, &in->mz); }   // (behind you: up goes at them)
    else {   // as the screen sees it: right is right, up is into the ring
        int S = fsin(cyaw), C = fcos(cyaw);
        in->mx = uy * S + ux * C; in->mz = uy * C - ux * S; if (uy && ux) { in->mx = in->mx * 181 >> 8; in->mz = in->mz * 181 >> 8; }
        if (cam_mode == CAM_SIDE && uy < 0 && !ux) { in->mx = in->mz = 0; in->down = 1; }   // (DOWN alone in the side view: crouch)
        else if (cam_mode == CAM_SIDE && uy > 0 && !ux) { in->mx = in->mx / 2; in->mz = in->mz / 2; }   // (UP: a sidestep)
    }
    if (cam_mode != CAM_SIDE && (held & PAD_L1)) in->down = 1;
    in->a = !!(held & PAD_CROSS); in->b = !!(held & PAD_CIRCLE); in->pa = !!(pressed & PAD_CROSS); in->pb = !!(pressed & PAD_CIRCLE);
    in->pj = !!(pressed & PAD_TRIANGLE); in->guard = !!(held & (PAD_SQUARE | PAD_R1));
}
static void step_fight(int held, int pressed) {
    FIn a, b; read_input(held, pressed, &a); ai(&F[1], &F[0], &b);
    if (style == 3) { if (rnd8() < 40 && b.guard) b.guard = 0; }
    fstep(&F[0], &F[1], &a); fstep(&F[1], &F[0], &b);
    events(&F[0]); events(&F[1]);
    if (callT) callT--;
    if (++sub >= 60) { sub = 0; if (secs > 0) secs--; }
    if (F[1].st == BS_KO) round_over(0, F[1].fall ? "RING OUT" : "K.O.");
    else if (F[0].st == BS_KO) round_over(1, F[0].fall ? "RING OUT" : "K.O.");
    else if (secs == 0) round_over(F[0].hp > F[1].hp ? 0 : F[1].hp > F[0].hp ? 1 : -1, "TIME");
}
static int fight_update(int held, int pressed, int steps) {
    for (int k = 0; k < 4; k++) if (spark[k].t > 0) spark[k].t -= steps;
    if (flash) flash = MAX(0, flash - steps);
    switch (fs) {
    case FS_PICK: {   // pick the fighter, the style, the camera
        if (pressed & PAD_UP) pick_row = (pick_row + 2) % 3;
        if (pressed & PAD_DOWN) pick_row = (pick_row + 1) % 3;
        int dir = (pressed & PAD_RIGHT) ? 1 : (pressed & PAD_LEFT) ? -1 : 0;
        if (dir && pick_row == 0) { int n = 0; do { you_i = (you_i + dir + nsim) % nsim; } while (stage_of(sims[you_i].age) < 2 && ++n < nsim); }
        if (dir && pick_row == 1) style = (style + dir + 4) & 3;
        if (dir && pick_row == 2) cam_mode = (cam_mode + dir + CAM_N) % CAM_N;
        if (dir) voice(V_AGREE);
        F[0].cnt += steps; F[1].cnt += steps; ft += steps;
        if (pressed & PAD_CROSS) { fs = FS_VS; ft = 0; set_stats(); }
        if (pressed & PAD_CIRCLE) return 0;
        return 1; }
    case FS_VS: ft += steps; F[0].cnt += steps; F[1].cnt += steps;
        if (ft > 20 && (pressed & PAD_CROSS)) match_begin();
        if (pressed & PAD_CIRCLE) return 0;
        return 1;
    case FS_INTRO: ft += steps; F[0].cnt += steps; F[1].cnt += steps; cam_follow(0);
        if (ft >= 100) { fs = FS_FIGHT; ft = 0; banner = 0; call = "FIGHT!"; callT = 40; }
        return 1;
    case FS_PAUSE:
        if (pressed & PAD_START) fs = FS_FIGHT;
        if (pressed & PAD_SELECT) { fs = FS_OVER; ft = 0; }
        return 1;
    case FS_FIGHT:
        if (pressed & PAD_START) { fs = FS_PAUSE; return 1; }
        if (pressed & PAD_SELECT) cam_mode = (cam_mode + 1) % CAM_N;
        for (int s = 0; s < steps && fs == FS_FIGHT; s++) {
            if (hitstop) { hitstop--; continue; }
            step_fight(held, s == 0 ? pressed : 0);
        }
        if (shake) shake--;
        cam_follow(0); return 1;
    case FS_END:   // the blow that ended it plays out (slowly, for a K.O.), then the word
        for (int s = 0; s < steps; s++) {
            ft++;
            if (slowmo && (ft & 3)) continue;
            FIn none; memset(&none, 0, sizeof none); fstep(&F[0], &F[1], &none); fstep(&F[1], &F[0], &none); F[0].ev = F[1].ev = 0;
        }
        if (slowmo) slowmo = MAX(0, slowmo - steps);
        if (shake) shake--;
        cam_follow(0);
        if (ft >= 170) {
            if (winner == 0) voice(V_YAHOO); else if (winner == 1) voice(V_NAH);
            if (F[0].wins >= 2 || F[1].wins >= 2) { fs = FS_RESULT; ft = 0; if (F[0].wins >= 2) { fight_last_wins++; if (fight_wins < 9999) fight_wins++; if (stage + 1 > fight_best) fight_best = stage + 1; } }
            else { round_n++; round_begin(); }
        }
        return 1;
    case FS_RESULT: ft += steps; F[0].cnt += steps; F[1].cnt += steps; cam_follow(0);
        if (ft > 40 && (pressed & PAD_CROSS)) {
            if (F[0].wins >= 2) { if (stage == 8) { fight_champ = 1; fs = FS_CHAMP; ft = 0; voice(V_WIN); } else { stage++; build_them(stage); set_stats(); fs = FS_VS; ft = 0; } }
            else if (funds >= 2) { funds -= 2; match_begin(); }   // CONTINUE: another $2
            else { fs = FS_OVER; ft = 0; }
        }
        if (ft > 40 && (pressed & PAD_CIRCLE)) { fs = FS_OVER; ft = 0; }
        return 1;
    case FS_CHAMP: ft += steps; F[0].cnt += steps; F[0].st = BS_WIN; F[0].t += steps;
        if (ft > 60 && (pressed & (PAD_CROSS | PAD_CIRCLE))) { fs = FS_OVER; ft = 0; }
        return 1;
    case FS_OVER: ft += steps; if (ft > 30 || (pressed & (PAD_CROSS | PAD_CIRCLE))) { return 0; } return 1;
    }
    return 0;
}

