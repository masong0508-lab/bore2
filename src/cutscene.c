// cutscene.c - the CUTSCENE ENGINE (CUT.OVL) and the first film in it: MISSY TELLER's opening, "TV SHOW & TELL".
// A film is DATA: camera keys (a Catmull-Rom path through them, with hard CUTS between shots), actor walks (smooth, with a real
// stride from the distance covered), pose tracks (every pose is a full-body VPose, blended, so nobody ever pops), a prop with
// its own path, subtitles that type themselves in, fades, letterbox and a few voice cues. The film plays on a clock of 60 Hz
// steps and is drawn from that clock alone, so a slow frame costs picture and never timing or smoothness.
// Units are the fight's: x and z on the ground (z the fight's, the world's -z), y up, a Bore 384 tall, yaw 0..255 (0 looks along +z).
#include "ovl.h"
#include "gfx.h"
#include "ui.h"
#include "voxel.h"
#include "look.h"
#include "audio.h"
#include "font.h"
#include "crowd.h"

static inline int fsin(int a) { return isin((a & 255) << 4) >> 4; }
static inline int fcos(int a) { return icos((a & 255) << 4) >> 4; }
static inline int bwrap(int a) { return ((a + 128) & 255) - 128; }
static int bang(int dx, int dz) {
    if (!dx && !dz) return 0;
    int best = 0, bd = -0x7fffffff;
    for (int a = 0; a < 256; a += 4) { int d = fsin(a) * dx + fcos(a) * dz; if (d > bd) { bd = d; best = a; } }
    for (int a = best - 3; a <= best + 3; a++) { int d = fsin(a) * dx + fcos(a) * dz; if (d > bd) { bd = d; best = a; } }
    return best & 255;
}
static inline int smooth(int u) { return (u * u * (3 * 1024 - 2 * u)) >> 20; }   // u 0..1024 -> 0..1024
#define T(d) ((d) * 6)                    // tenths of a second -> 60 Hz steps

// ---- the poses (VPose: arm, aroll, elbow, leg, lroll, knee [left, right], lean, twist, nod, tilt, drop) ----
#define P(a0,a1,r0,r1,e0,e1, l0,l1,lr0,lr1,k0,k1, ln,tw,nd,tl) { {a0,a1},{r0,r1},{e0,e1},{l0,l1},{lr0,lr1},{k0,k1},ln,tw,nd,tl,0,0 }
enum { Q_HANG, Q_TALK, Q_OPEN, Q_WAVE, Q_PRES, Q_POINT, Q_HIP, Q_CLAP, Q_CHEER, Q_RECOIL, Q_SERVE, Q_PUSH, Q_DANA, Q_DANB, Q_TEMPT, Q_BOW, Q_WIN, Q_N };
static const VPose pq[Q_N] = {
 /* HANG  */ P(-60,-60,-140,140,-90,-90,      -40,40,-70,70,30,30,        0,0,0,0),
 /* TALK  */ P(-60,-900,-140,-200,-90,-1000,  -40,40,-70,70,30,30,        20,60,50,0),
 /* OPEN  */ P(-760,-760,-640,640,-560,-560,  -40,40,-70,70,30,30,        40,0,-30,0),
 /* WAVE  */ P(-60,-2050,-140,420,-90,-420,   -40,40,-70,70,30,30,        0,0,0,60),
 /* PRES  */ P(-1150,-250,-760,-140,-250,-120, -240,160,-70,70,80,160,    60,-120,40,0),
 /* POINT */ P(-60,-1500,-140,-60,-90,-40,    -40,40,-70,70,30,30,        30,130,40,0),
 /* HIP   */ P(-50,-60,-520,140,-1500,-90,    -60,60,-70,70,30,30,        -20,0,0,90),
 /* CLAP  */ P(-1300,-1300,-120,120,-1400,-1400, -40,40,-70,70,30,30,     20,0,40,0),
 /* CHEER */ P(-2300,-2300,-340,340,-100,-100, -300,200,-110,110,200,160,  -30,0,-60,0),
 /* RECOIL*/ P(-700,-700,-300,300,-700,-700,  -500,300,-90,90,260,60,     -260,0,-90,0),
 /* SERVE */ P(-60,-1050,-140,-300,-90,-200,  -40,40,-70,70,30,30,        50,40,60,0),
 /* PUSH  */ P(-60,-1250,-140,-100,-90,-150,  -240,160,-70,70,80,160,     220,-160,60,0),
 /* DANA  */ P(-2050,-250,-500,300,-80,-700,  -500,300,-90,90,260,120,    80,-300,-40,110),
 /* DANB  */ P(-250,-2050,-300,500,-700,-80,  300,-500,-90,90,120,260,    80,300,-40,-110),
 /* TEMPT */ P(-60,-60,-140,140,-90,-90,      -40,40,-70,70,30,30,        230,0,260,0),
 /* BOW   */ P(-60,-60,-140,140,-90,-90,      -40,40,-70,70,30,30,        520,0,200,0),
 /* WIN   */ P(-320,-2300,-320,220,-1200,-100, -100,100,-110,110,60,60,   -60,0,-170,0) };
#undef P
static void plerp(VPose *d, const VPose *a, const VPose *b, int k) { const s16 *aa = (const s16 *)a, *bb = (const s16 *)b; s16 *dd = (s16 *)d; for (int i = 0; i < (int)(sizeof(VPose) / 2); i++) dd[i] = (s16)(aa[i] + (((bb[i] - aa[i]) * k) >> 10)); }

// ---- the film's data ----
typedef struct { u16 t; s16 ex, ey, ez, ax, ay, az; u8 cut; } CK;           // a camera key: eye and aim; cut: arrive by a hard cut
typedef struct { u16 t; s16 x, z; u8 yaw; } WK;                              // an actor's mark
typedef struct { u16 t; u8 q, bl; } QK;                                      // a pose, and the steps it takes to get there
typedef struct { u16 t0, t1; u8 who; const char *tx; } Sub;                  // who: 0 none, 1 CHAD, 2 MISSY, 3 INTERN
typedef struct { u16 t; u8 v; } Cue;                                         // a voice
typedef struct { u16 t; s16 x, y, z; u8 mode; } GK;                          // the glass: mode 0 smooth, 1 falls (x and z straight, y under gravity)

#define CK_(t,ex,ey,ez,ax,ay,az,c) { T(t), ex, ey, ez, ax, ay, az, c }
static const CK cam_k[] = {
    // 1. the studio wakes up: a wide crane drifting in
    CK_(0,    -1500, 1050, -2500,   100, 200, 0, 0),   CK_(25, -500, 700, -2150, 0, 210, 0, 0),   CK_(50, 300, 420, -1800, 0, 200, 0, 0),
    // 2. Chad bounds in from the left; the camera tracks him across
    CK_(50,   -700, 250, -1250,  -1000, 230, 0, 1),    CK_(70, -520, 250, -1200, -650, 230, 0, 0),   CK_(105, -150, 245, -1000, -100, 235, 0, 0),
    // 3. the audience goes wild: a low sweep along the front row
    CK_(105,  -950, 120, -820,  -1700, 190, -250, 1),  CK_(130, -950, 130, -300, -1700, 220, 250, 0),  CK_(150, -900, 140, 100, -1700, 230, 400, 0),
    // 4. Chad, mid-shot, easing in
    CK_(150,  260, 250, -760,   -20, 235, 0, 1),       CK_(190, 90, 245, -640, -30, 235, 0, 0),
    // 5. the reveal of Missy from low and in front
    CK_(190,  430, 230, -620,   975, 270, -150, 1),    CK_(215, 520, 240, -520, 975, 275, -150, 0),  CK_(240, 590, 245, -450, 975, 280, -150, 0),
    // 6. two-shot, wide, a slow dolly
    CK_(240,  520, 330, -1550,  500, 220, 0, 1),       CK_(285, 640, 300, -1400, 560, 225, 0, 0),
    // 7. the intern comes on with the tray
    CK_(285,  100, 260, -1250,  600, 190, -500, 1),    CK_(335, 230, 250, -1100, 740, 190, -300, 0),
    // 8. close on Missy: the tray, the glass, the face
    CK_(335,  790, 360, -640,   975, 335, -150, 1),    CK_(365, 800, 360, -600, 975, 335, -150, 0),  CK_(385, 815, 355, -560, 975, 335, -150, 0),
    // 9. the glass, close: then the camera falls with it
    CK_(385,  830, 240, -380,   850, 195, -215, 1),    CK_(395, 825, 235, -360, 850, 195, -215, 0),  CK_(405, 800, 200, -420, 800, 90, -215, 0),  CK_(420, 760, 150, -480, 790, 50, -215, 0),
    // 10. everyone reacts: a wide, then the swing round to Missy
    CK_(420,  250, 360, -1700,  500, 230, 0, 1),       CK_(465, 430, 300, -1450, 700, 235, 0, 0),
    // 11. Missy, low and heroic, dancing
    CK_(465,  540, 300, -620,   975, 270, -150, 1),    CK_(505, 620, 290, -560, 975, 275, -150, 0),  CK_(545, 700, 280, -600, 975, 285, -150, 0),
    // 12. the pull-back to the whole stage for the title
    CK_(545,  200, 520, -2100,  400, 230, 0, 1),       CK_(600, 80, 700, -2500, 400, 230, 0, 0) };

#define WK_(t,x,z,y) { T(t), x, z, y }
static const WK chad_w[] = { WK_(0,-1500,-60,64), WK_(50,-1500,-60,64), WK_(76,-130,-60,64), WK_(88,-120,-60,128), WK_(190,-120,-60,128), WK_(200,-100,-60,170), WK_(345,-100,-60,170), WK_(355,-100,-60,128) };
static const WK missy_w[] = { WK_(0,975,-150,192), WK_(280,975,-150,192), WK_(292,975,-150,196), WK_(340,975,-150,184), WK_(385,975,-150,176), WK_(430,975,-150,176), WK_(470,975,-150,192) };
static const WK intern_w[] = { WK_(0,1500,-1100,225), WK_(285,1500,-1100,225), WK_(318,1100,-700,225), WK_(336,740,-300,36), WK_(350,740,-300,36), WK_(372,1000,-760,2), WK_(520,1000,-760,2) };

#define QK_(t,q,b) { T(t), q, b }
static const QK chad_q[] = { QK_(0,Q_HANG,1), QK_(50,Q_HANG,1), QK_(88,Q_PRES,18), QK_(110,Q_OPEN,20), QK_(140,Q_CHEER,18), QK_(150,Q_TALK,24), QK_(172,Q_PRES,20), QK_(200,Q_POINT,16),
                             QK_(225,Q_CLAP,14), QK_(250,Q_TALK,20), QK_(305,Q_HANG,24), QK_(420,Q_RECOIL,10), QK_(436,Q_OPEN,22), QK_(455,Q_CHEER,12), QK_(500,Q_DANA,12), QK_(510,Q_DANB,12), QK_(520,Q_DANA,12),
                             QK_(530,Q_DANB,12), QK_(540,Q_WIN,14) };
static const QK missy_q[] = { QK_(0,Q_HANG,1), QK_(205,Q_HIP,22), QK_(228,Q_WAVE,14), QK_(250,Q_HIP,16), QK_(278,Q_TALK,16), QK_(300,Q_HIP,24), QK_(345,Q_TEMPT,40), QK_(370,Q_HANG,40), QK_(380,Q_TEMPT,20),
                              QK_(392,Q_PUSH,8), QK_(404,Q_PUSH,40), QK_(425,Q_RECOIL,6), QK_(440,Q_POINT,14), QK_(458,Q_HIP,16), QK_(476,Q_DANA,10), QK_(488,Q_DANB,10), QK_(500,Q_DANA,10), QK_(512,Q_DANB,10),
                              QK_(524,Q_DANA,10), QK_(536,Q_WIN,12), QK_(560,Q_BOW,30) };
static const QK intern_q[] = { QK_(0,Q_HANG,1), QK_(296,Q_SERVE,12), QK_(336,Q_SERVE,12), QK_(350,Q_HANG,20), QK_(420,Q_RECOIL,6), QK_(440,Q_CLAP,14), QK_(500,Q_CHEER,14) };

static const GK glass_k[] = { { T(285), 300, 0, -1500, 0 }, { T(336), 786, 188, -237, 0 }, { T(350), 850, 192, -215, 0 }, { T(392), 850, 192, -215, 0 }, { T(403), 815, 192, -215, 0 }, { T(409), 788, 192, -215, 0 }, { T(424), 765, 0, -215, 1 } };
#define GLASS_X 765
#define GLASS_Z (-215)
#define GLASS_APPEAR T(336)
#define GLASS_BREAK  T(424)

static const Sub subs[] = {
    { T(70),  T(105), 1, "GOOD EVENING, BORETOWN!" },
    { T(108), T(148), 1, "WELCOME BACK TO DANCING WITH THE BARS!" },
    { T(152), T(188), 1, "OUR JUDGE NEEDS NO INTRODUCTION..." },
    { T(192), T(214), 1, "...BUT SHE GETS ONE ANYWAY:" },
    { T(216), T(238), 1, "MISSY TELLER!" },
    { T(244), T(272), 2, "HELLO, DARLINGS." },
    { T(276), T(300), 2, "TRY NOT TO BORE ME TONIGHT." },
    { T(302), T(334), 2, "SOMEBODY GET ME A DRINK." },
    { T(342), T(372), 3, "YOUR USUAL, MISS TELLER." },
    { T(374), T(392), 2, "..." },
    { T(396), T(418), 2, "NOT TONIGHT, DARLING." },
    { T(436), T(466), 1, "SHE IS BACK, FOLKS!" },
    { T(470), T(520), 2, "WATCH ME DANCE." } };
static const Cue cues[] = { { T(88), VOICE_YEHA }, { T(112), VOICE_LAUGH }, { T(150), VOICE_YAHOO }, { T(228), VOICE_FLIRT }, { T(350), VOICE_NEED }, { T(424), VOICE_SHRIEK }, { T(440), VOICE_YEHA }, { T(476), VOICE_WIN } };
static const struct { u16 t; u8 lvl; } cheers[] = { { T(0), 40 }, { T(100), 255 }, { T(160), 110 }, { T(228), 255 }, { T(262), 120 }, { T(424), 40 }, { T(432), 255 }, { T(560), 255 } };
#define FILM_END T(600)

// ---- the people ----
enum { A_CHAD, A_MISSY, A_INTERN, A_N };
typedef struct { Bore *b; int x, z, yaw, lx, lz, amp, phase; VPose pose; } Act;
static Act act[A_N];
static int ft, cue_i, lights, skipped, done_t, gal;   // gal: the pose gallery (a test: AUTO_CUT=9999)

#define BORE_AT(i) ((Bore *)((((u32)crowd_scratch(&(u32){0}) + 7) & ~7u) + (i) * ((sizeof(Bore) + 7) & ~7u)))
static void build_cast(void) {
    static const u8 missy_look[12] = { 4,1,0,4,1,9,0,4,0, 0,0,0 };
    for (int i = 0; i < A_N; i++) act[i].b = BORE_AT(i);
    Bore *m = act[A_MISSY].b; look_default(m);
    for (int i = 0; i < 12; i++) m->look[i] = missy_look[i];
    m->stage = AG_ADULT; m->asp = AS_POP; look_set_sign(m, 3); strcpy(m->first, "MISSY"); strcpy(m->last, "TELLER"); look_fix(m); look_build(m);
    m->col[6] = RGB(18, 4, 22); m->col[7] = RGB(18, 4, 22); voxel_prepare(m);
    static const u16 seeds[2] = { 311, 1701 };
    for (int k = 0; k < 2; k++) {
        Bore *b = act[k == 0 ? A_CHAD : A_INTERN].b;
        u32 old = look_seed(seeds[k] * 7919u + 1u); look_default(b); look_random(b); look_seed(old);
        b->stage = AG_ADULT; strcpy(b->first, k == 0 ? "CHAD" : "INTERN"); b->last[0] = 0; look_fix(b); look_build(b);
        if (k == 0) { b->col[6] = RGB(3, 3, 5); b->col[7] = RGB(3, 3, 5); b->col[5] = RGB(22, 16, 6); }
        else { b->col[6] = RGB(28, 26, 6); b->col[7] = RGB(6, 8, 20); }
        voxel_prepare(b);
    }
}

// ---- evaluating the data at time t ----
static int cr(int p0, int p1, int p2, int p3, int u) {   // Catmull-Rom, u 0..1024
    int A = -p0 + 3 * p1 - 3 * p2 + p3, B = 2 * p0 - 5 * p1 + 4 * p2 - p3, C = -p0 + p2, DD = 2 * p1;
    return (((((A * u >> 10) + B) * u >> 10) + C) * u >> 10) / 2 + DD / 2;
}
static void cam_at(int t, int *o) {   // o[6]: eye xyz, aim xyz
    const int n = sizeof cam_k / sizeof cam_k[0]; int i = 0;
    for (int k = 0; k < n - 1; k++) if (cam_k[k].t <= t) i = k;
    const CK *a = &cam_k[i], *b = &cam_k[MIN(i + 1, n - 1)];
    const CK *p0 = (i > 0 && !a->cut) ? &cam_k[i - 1] : a, *p3 = (i + 2 < n && !cam_k[i + 2].cut) ? &cam_k[i + 2] : b;
    int u = 0;
    if (i + 1 < n && !b->cut && b->t > a->t) u = CLAMP((t - a->t) * 1024 / (b->t - a->t), 0, 1024); else if (b->cut || i + 1 >= n) b = a;
    if (b == a) { p0 = p3 = a; }
    o[0] = cr(p0->ex, a->ex, b->ex, p3->ex, u); o[1] = cr(p0->ey, a->ey, b->ey, p3->ey, u); o[2] = cr(p0->ez, a->ez, b->ez, p3->ez, u);
    o[3] = cr(p0->ax, a->ax, b->ax, p3->ax, u); o[4] = cr(p0->ay, a->ay, b->ay, p3->ay, u); o[5] = cr(p0->az, a->az, b->az, p3->az, u);
}
static void mark_at(const WK *k, int n, int t, int *x, int *z, int *yaw) {
    int i = 0; for (int j = 0; j < n - 1; j++) if (k[j].t <= t) i = j;
    const WK *a = &k[i], *b = &k[MIN(i + 1, n - 1)];
    if (t >= b->t || b == a) { *x = b->x; *z = b->z; *yaw = b->yaw; return; }
    int u = smooth((t - a->t) * 1024 / (b->t - a->t));
    *x = a->x + ((b->x - a->x) * u >> 10); *z = a->z + ((b->z - a->z) * u >> 10);
    *yaw = (a->yaw + (bwrap(b->yaw - a->yaw) * u >> 10)) & 255;
}
static void pose_at(const QK *k, int n, int t, VPose *o) {
    int i = 0; for (int j = 0; j < n; j++) if (k[j].t <= t) i = j;
    const VPose *to = &pq[k[i].q], *from = &pq[i > 0 ? k[i - 1].q : k[i].q];
    int bl = MAX(1, (int)k[i].bl * 3), u = i > 0 ? CLAMP((t - k[i].t) * 1024 / bl, 0, 1024) : 1024;
    plerp(o, from, to, smooth(u));
}
static int cheer_at(int t) {
    int i = 0; const int n = sizeof cheers / sizeof cheers[0]; for (int j = 0; j < n; j++) if (cheers[j].t <= t) i = j;
    int dt = t - cheers[i].t, v = cheers[i].lvl;
    return v > 130 ? MAX(70, v - dt / 3) : v;
}

// ---- stepping ----
#define FADE_IN T(12)
void cut_open(int arg) {
    build_cast();
    gal = arg == 9999; ft = arg > 1 && !gal ? arg : 0; cue_i = 0; lights = 0; skipped = 0; done_t = 0;
    while (cue_i < (int)(sizeof cues / sizeof cues[0]) && cues[cue_i].t < ft) cue_i++;
    for (int i = 0; i < A_N; i++) { act[i].amp = 0; act[i].phase = 0; act[i].lx = -99999; }
    audio_voice(VOICE_NEED);
}
static void walk_pose(const Act *a, VPose *o) {   // a stride made from the phase: the hips swing, the knees fold behind, the arms counter-swing
    int s = fsin(a->phase), s2 = fsin(a->phase + 64);
    memset(o, 0, sizeof *o);
    o->leg[0] = (s16)(-s * 5); o->leg[1] = (s16)(s * 5); o->knee[0] = (s16)(MAX(0, s2) * 5); o->knee[1] = (s16)(MAX(0, -s2) * 5);
    o->arm[0] = (s16)(s * 3); o->arm[1] = (s16)(-s * 3); o->elbow[0] = o->elbow[1] = -200; o->lean = 90; o->drop = (s16)(-ABS(s2) / 4);
    o->lroll[0] = -70; o->lroll[1] = 70; o->aroll[0] = -140; o->aroll[1] = 140;
}
static void poses_now(void) {
    static const struct { const WK *w; int nw; const QK *q; int nq; } tr[A_N] = {
        { chad_w, sizeof chad_w / sizeof chad_w[0], chad_q, sizeof chad_q / sizeof chad_q[0] },
        { missy_w, sizeof missy_w / sizeof missy_w[0], missy_q, sizeof missy_q / sizeof missy_q[0] },
        { intern_w, sizeof intern_w / sizeof intern_w[0], intern_q, sizeof intern_q / sizeof intern_q[0] } };
    for (int i = 0; i < A_N; i++) {
        Act *a = &act[i]; int x, z, yaw; mark_at(tr[i].w, tr[i].nw, ft, &x, &z, &yaw);
        int d = a->lx == -99999 ? 0 : (int)(ABS(x - a->lx) + ABS(z - a->lz));
        a->lx = x; a->lz = z; a->x = x; a->z = z; a->yaw = yaw;
        a->phase = (a->phase + d * 3 / 2) & 255;
        int want = d > 0 ? CLAMP(d * 90, 0, 1024) : 0;
        a->amp += (want - a->amp) / 6; if (!want && a->amp < 16) a->amp = 0;
        VPose p; pose_at(tr[i].q, tr[i].nq, ft, &p);
        VPose w; walk_pose(a, &w); plerp(&a->pose, &p, &w, a->amp);
        a->pose.nod += (s16)(fsin(ft * 3 + i * 40) / 6);      // breathing
        a->pose.drop += (s16)(fsin(ft * 2 + i * 70) / 10);
    }
}
static int speaker_now(void) { for (unsigned i = 0; i < sizeof subs / sizeof subs[0]; i++) if (ft >= subs[i].t0 && ft < subs[i].t1) return subs[i].who; return 0; }

int cut_update(int held, int pressed, int steps) {
    if (steps < 1) steps = 1;
    if (pressed & (PAD_START | PAD_CIRCLE | PAD_TRIANGLE)) skipped = 1;
    if (skipped) { done_t += steps; if (done_t > 18) { for (int i = 0; i < A_N; i++) look_release(act[i].b); return 0; } }
    else ft += steps;
    while (cue_i < (int)(sizeof cues / sizeof cues[0]) && cues[cue_i].t <= ft) audio_voice(cues[cue_i++].v);
    if (ft >= FILM_END + T(8)) { for (int i = 0; i < A_N; i++) look_release(act[i].b); return 0; }
    lights += (256 - lights) * MIN(steps, 8) / 40;
    poses_now();
    return 1;
}

// ---- the studio ----
static const u8 palette[6][3] = { { 255, 60, 140 }, { 255, 170, 40 }, { 80, 220, 120 }, { 60, 170, 255 }, { 170, 90, 255 }, { 255, 90, 60 } };
static inline void V3(SVECTOR *v, int x, int y, int z) { v->vx = (short)x; v->vy = (short)-y; v->vz = (short)-z; }
static u32 shade(int r, int g, int b, int k) { r = r * k >> 8; g = g * k >> 8; b = b * k >> 8; return UI_RGB(MIN(r, 255), MIN(g, 255), MIN(b, 255)); }
static void box_side(int x0, int y0, int z0, int x1, int y1, int z1, u32 c0, u32 c1) {   // a quad from (x0,y0,z0) top-left to (x1,y1,z1) bottom-right, any vertical plane
    SVECTOR a, b, c, d; V3(&a, x0, y1, z0); V3(&b, x1, y1, z1); V3(&c, x0, y0, z0); V3(&d, x1, y0, z1);
    gfx_quad_g(&a, &b, &c, &d, c0, c0, c1, c1, 0);
}
static void floor_draw(int tf) {
    static const struct { int r, g, b; } lc[3] = { { 255, 240, 200 }, { 255, 80, 190 }, { 80, 190, 255 } };
    int lx[3] = { act[A_CHAD].x, 900, fsin(tf / 3) * 7 }, lz[3] = { act[A_CHAD].z, -190, fcos(tf / 4) * 5 + 200 };
    if (tf < T(190)) { lx[1] = fsin(tf / 2 + 40) * 6; lz[1] = fcos(tf / 3) * 4 + 100; }
    gfx_floor_pass = 1;
    const int N = 400;
    for (int gz = -3; gz < 4; gz++) for (int gx = -4; gx < 4; gx++) {
        int cx[4], cz[4]; cx[0] = gx * N; cz[0] = gz * N; cx[1] = cx[0] + N; cz[1] = cz[0]; cx[2] = cx[0]; cz[2] = cz[0] + N; cx[3] = cx[1]; cz[3] = cz[2];
        SVECTOR p[4]; u32 col[4];
        for (int k = 0; k < 4; k++) {
            V3(&p[k], cx[k], 0, cz[k]);
            int r = 22 + (((gx + gz) & 1) ? 10 : 0), g = r, b = r + 22;
            for (int l = 0; l < 3; l++) { int dx = cx[k] - lx[l], dz = cz[k] - lz[l]; int d2 = dx * dx + dz * dz; int i = MAX(0, 256 - d2 / 2300) * lights >> 8;
                r += lc[l].r * i >> 9; g += lc[l].g * i >> 9; b += lc[l].b * i >> 9; }
            col[k] = UI_RGB(MIN(r, 255), MIN(g, 255), MIN(b, 255));
        }
        gfx_quad_g(&p[0], &p[1], &p[2], &p[3], col[0], col[1], col[2], col[3], 0);
    }
    gfx_floor_pass = 0;
}
static void backdrop_draw(int tf) {
    const int Z = 1100, PW = 200, NP = 18;
    for (int i = 0; i < NP; i++) {
        int x0 = -1800 + i * PW + 6, x1 = x0 + PW - 12;
        int w = (isin(((i * 230 - tf * 11) & 4095)) >> 3) + 255;     // a chase running along the wall
        int k = ((90 + (w * 165 >> 9)) * lights) >> 8; const u8 *c = palette[(i / 3 + tf / 150) % 6];
        SVECTOR a, b, cc, d; V3(&a, x0, 760, Z); V3(&b, x1, 760, Z); V3(&cc, x0, 40, Z); V3(&d, x1, 40, Z);
        gfx_quad_g(&a, &b, &cc, &d, shade(c[0], c[1], c[2], k / 3), shade(c[0], c[1], c[2], k / 3), shade(c[0], c[1], c[2], k), shade(c[0], c[1], c[2], k), 0);
    }
    SVECTOR a, b, c, d;   // the gold arch above and the black plinth below
    V3(&a, -1800, 830, Z - 10); V3(&b, 1800, 830, Z - 10); V3(&c, -1800, 760, Z - 10); V3(&d, 1800, 760, Z - 10);
    gfx_quad_f(&a, &b, &c, &d, 200 * lights >> 8, 150 * lights >> 8, 40 * lights >> 8, 0);
    V3(&a, -1800, 40, Z - 10); V3(&b, 1800, 40, Z - 10); V3(&c, -1800, 0, Z - 10); V3(&d, 1800, 0, Z - 10);
    gfx_quad_f(&a, &b, &c, &d, 14, 12, 24, 0);
    for (int i = 0; i < 24; i++) {   // marquee bulbs along the arch, chasing
        int on = ((i * 3 - tf / 5) & 7) < 3; int x = -1700 + i * 148;
        SVECTOR p0, p1, p2; V3(&p0, x - 22, 850, Z - 14); V3(&p1, x + 22, 850, Z - 14); V3(&p2, x, 800, Z - 14);
        if (on) gfx_tri_f(&p0, &p1, &p2, 255, 235, 150, 0); else gfx_tri_f(&p0, &p1, &p2, 90, 70, 40, 0);
    }
}
static void desk_draw(void) {
    const int X0 = 790, X1 = 900, Zs = 380, HT = 190;
    SVECTOR a, b, c, d;
    V3(&a, X0, HT, -Zs); V3(&b, X1, HT, -Zs); V3(&c, X0, HT, Zs); V3(&d, X1, HT, Zs);
    gfx_quad_f(&a, &b, &c, &d, 120 * lights >> 8, 70 * lights >> 8, 40 * lights >> 8, 0);
    box_side(X0, HT, -Zs, X0, 0, Zs, shade(140, 40, 50, lights), shade(60, 14, 24, lights));      // the front, with a gold band
    box_side(X0, HT - 28, -Zs, X0, HT - 40, Zs, shade(240, 190, 60, lights), shade(240, 190, 60, lights));
    box_side(X0, HT, -Zs, X1, 0, -Zs, shade(100, 30, 40, lights), shade(40, 10, 16, lights));
    box_side(X0, HT, Zs, X1, 0, Zs, shade(100, 30, 40, lights), shade(40, 10, 16, lights));
    for (int k = 0; k < 3; k++) {   // the score paddles, lying on the desk
        int z = -240 + k * 240; SVECTOR p0, p1, p2, p3; V3(&p0, 830, HT + 3, z - 40); V3(&p1, 870, HT + 3, z - 40); V3(&p2, 830, HT + 3, z + 40); V3(&p3, 870, HT + 3, z + 40);
        gfx_quad_f(&p0, &p1, &p2, &p3, 230 * lights >> 8, 230 * lights >> 8, 200 * lights >> 8, 0);
    }
}
static void crowd_draw(int tf) {
    int ch = cheer_at(tf);
    for (int row = 0; row < 3; row++) {
        int x = -1500 - row * 150, base = row * 70;
        SVECTOR a, b, c, d; V3(&a, x + 75, base, -850); V3(&b, x + 75, base, 850); V3(&c, x + 75, 0, -850); V3(&d, x + 75, 0, 850);
        gfx_quad_f(&a, &b, &c, &d, 28, 22, 40, 0);
        for (int i = 0; i < 9; i++) {
            int z = -760 + i * 190 + ((row * 53) & 63) - 30, h = (row * 5 + i * 3) & 7;
            int bob = ABS(fsin(tf / 2 + i * 31 + row * 17)) * ch >> 10, rr = 60 + (h * 23 & 127), gg = 40 + (h * 41 & 127), bb = 70 + (h * 57 & 127);
            int bw = 125, bh = 150, y0 = base + bob;
            int k = (210 * lights) >> 8;
            SVECTOR p0, p1, p2, p3;
            V3(&p0, x, y0 + bh, z - bw / 2); V3(&p1, x, y0 + bh, z + bw / 2); V3(&p2, x, y0, z - bw / 2); V3(&p3, x, y0, z + bw / 2);
            gfx_quad_f(&p0, &p1, &p2, &p3, rr * k >> 8, gg * k >> 8, bb * k >> 8, 0);
            V3(&p0, x, y0 + bh + 62, z - 34); V3(&p1, x, y0 + bh + 62, z + 34); V3(&p2, x, y0 + bh, z - 34); V3(&p3, x, y0 + bh, z + 34);
            gfx_quad_f(&p0, &p1, &p2, &p3, 230 * k >> 8, 180 * k >> 8, 140 * k >> 8, 0);
            if (ch > 90) {   // hands up
                int up = (fsin(tf + i * 40) >> 4) + 20;
                V3(&p0, x, y0 + bh + 50 + up, z - 66); V3(&p1, x, y0 + bh + 50 + up, z - 48); V3(&p2, x, y0 + bh - 10, z - 60); V3(&p3, x, y0 + bh - 10, z - 44);
                gfx_quad_f(&p0, &p1, &p2, &p3, 230 * k >> 8, 180 * k >> 8, 140 * k >> 8, 0);
                V3(&p0, x, y0 + bh + 50 + up, z + 48); V3(&p1, x, y0 + bh + 50 + up, z + 66); V3(&p2, x, y0 + bh - 10, z + 44); V3(&p3, x, y0 + bh - 10, z + 60);
                gfx_quad_f(&p0, &p1, &p2, &p3, 230 * k >> 8, 180 * k >> 8, 140 * k >> 8, 0);
            }
        }
    }
}
static int glass_at(int t, int *o) {   // o: x y z; 0 if not there
    if (t < GLASS_APPEAR || t >= GLASS_BREAK) return 0;
    const int n = sizeof glass_k / sizeof glass_k[0]; int i = 0; for (int j = 0; j < n - 1; j++) if (glass_k[j].t <= t) i = j;
    const GK *a = &glass_k[i], *b = &glass_k[i + 1];
    if (i == 0) {   // on the tray: follows the intern's hand
        const Act *n_ = &act[A_INTERN]; o[0] = n_->x + (fsin(n_->yaw) * 78 >> 8); o[2] = n_->z + (fcos(n_->yaw) * 78 >> 8); o[1] = 188; return 1;
    }
    int u = CLAMP((t - a->t) * 1024 / (b->t - a->t), 0, 1024);
    if (i == 5) {   // pushed over the edge: falls
        int uu = (u * u) >> 10; o[0] = a->x + ((b->x - a->x) * u >> 10); o[2] = a->z + ((b->z - a->z) * u >> 10); o[1] = a->y + ((b->y - a->y) * uu >> 10); return 1;
    }
    int s = smooth(u); o[0] = a->x + ((b->x - a->x) * s >> 10); o[1] = a->y + ((b->y - a->y) * s >> 10); o[2] = a->z + ((b->z - a->z) * s >> 10); return 1;
}
static void glass_draw(int t) {
    int g[3];
    if (glass_at(t, g)) {
        int x = g[0], y = g[1], z = g[2], sp = isin(t * 70) >> 9;   // (a glint)
        SVECTOR a, b, c; V3(&a, x - 18, y + 36, z); V3(&b, x + 18, y + 36, z); V3(&c, x, y + 4, z);
        gfx_tri_f(&a, &b, &c, 200, 230, 250, 0);
        V3(&a, x - 3, y + 4, z); V3(&b, x + 3, y + 4, z); V3(&c, x - 3, y - 14, z);
        gfx_tri_f(&a, &b, &c, 180, 200, 230, 0);
        SVECTOR d; V3(&a, x - 14, y + 4 - 14, z); V3(&b, x + 14, y - 14 + 4, z); V3(&c, x, y - 16, z); (void)d;
        V3(&a, x - 3, y + 30, z); V3(&b, x + 6, y + 30, z); V3(&c, x + 1, y + 20, z);
        gfx_tri_f(&a, &b, &c, 60, 150, 60, 0);   // the olive
        (void)sp;
    }
    if (t >= GLASS_BREAK && t < GLASS_BREAK + T(10)) {   // the shatter: a burst of shards, each a pure function of time
        int dt = t - GLASS_BREAK;
        for (int i = 0; i < 14; i++) {
            int vx = (((i * 97) & 31) - 16) * 2, vz = (((i * 61) & 31) - 16) * 2, vy = 60 + ((i * 37) & 31) * 3;
            int x = GLASS_X + (vx * dt >> 4), z = GLASS_Z + (vz * dt >> 4), y = (vy * dt >> 4) - (dt * dt >> 4); if (y < 0) y = 0;
            SVECTOR a, b, c; V3(&a, x - 5, y + 9, z); V3(&b, x + 5, y + 9, z); V3(&c, x, y, z);
            gfx_tri_f(&a, &b, &c, 200 + (i & 3) * 14, 230, 255, 0);
        }
    }
}
static void actor_draw(int i) {
    Act *a = &act[i];
    if (i == A_INTERN && ft < T(285)) return;
    if (i == A_CHAD && ft < T(50) && 0) return;
    VPose p = a->pose;
    int sp = speaker_now();
    if (sp == i + 1) {   // the one speaking nods and gestures
        p.nod += (s16)(fsin(ft * 7) / 3); p.arm[1] += (s16)(fsin(ft * 5 + 30) * 2); p.tilt += (s16)(fsin(ft * 3) / 4);
    }
    Gait g; memset(&g, 0, sizeof g); g.pose = &p; g.clock = ft; g.seed = i * 5;
    VECTOR pos = { a->x, 0, -a->z };
    voxel_draw(a->b, &pos, a->yaw << 4, &g);
    VECTOR sh = { pos.vx, 0, pos.vz }; gfx_shadow(&sh, 70);
}

// ---- drawing ----
static void subtitle_draw(void) {
    for (unsigned i = 0; i < sizeof subs / sizeof subs[0]; i++) {
        const Sub *s = &subs[i]; if (ft < s->t0 || ft >= s->t1) continue;
        char buf[48]; int n = (ft - s->t0) / 2 + 1, L = (int)strlen(s->tx); if (n > L) n = L; memcpy(buf, s->tx, n); buf[n] = 0;
        int fade = MIN(MIN(ft - s->t0, s->t1 - ft), 8);
        static const u32 nc[4] = { UI_WHITE, UI_RGB(255, 210, 80), UI_RGB(255, 120, 200), UI_RGB(120, 220, 140) };
        static const char *const nm[4] = { "", "CHAD", "MISSY", "INTERN" };
        int y = SCREEN_H - 62; int w = ui_tw(1, s->tx) + 24;
        gfx_ui_blend(2); gfx_rect(160 - w / 2, y - 3, w, s->who ? 28 : 16, 70 * fade / 8, 70 * fade / 8, 70 * fade / 8, 1); gfx_ui_blend(1);
        if (s->who) { ui_textc(160, y, 1, nm[s->who], nc[s->who]); ui_textc(160, y + 12, 1, buf, UI_WHITE); } else ui_textc(160, y + 2, 1, buf, UI_WHITE);
    }
}
static void gallery_draw(void) {   // six poses of the library at a time, front on (a page every 3 s): how to check what a number does
    static int pg; pg++;
    gfx_set_clear(40, 40, 60); gfx_set_ambient(110, 120);
    cam.eye.vx = 0; cam.eye.vy = -300; cam.eye.vz = 1900; cam.at.vx = 0; cam.at.vy = -170; cam.at.vz = 0; gfx_camera_update(); gfx_set_identity();
    lights = 256; floor_draw(0);
    gfx_set_reserve(RESERVE_UI);
    int page = (pg / 180) % 3;
    for (int k = 0; k < 6; k++) { int i = page * 6 + k; if (i >= Q_N) break;
        VPose p = pq[i]; Gait g; memset(&g, 0, sizeof g); g.pose = &p;
        VECTOR pos = { (k - 3) * 270 + 135, 0, 0 };
        voxel_draw(act[A_CHAD].b, &pos, 2048, &g);
    }
    gfx_set_reserve(0);
}
void cut_draw(void) {
    if (gal) { gallery_draw(); return; }
    gfx_set_clear(10, 6, 20); gfx_set_fog(0, 0);
    gfx_sky(0x180810u, 0x401828u);
    gfx_set_ambient(96, 112);
    int c[6]; cam_at(ft, c);
    int sway = ft < T(335) || ft > T(425) ? 1 : 2;   // a handheld drift (a little more when it is close in)
    int hb = ft >= T(345) && ft < T(392) ? ABS(fsin(ft * 4)) / 40 : 0;   // the heartbeat in the close-up
    cam.eye.vx = c[0] + fsin(ft / 2) * sway / 90 + hb; cam.eye.vy = -(c[1]) + fcos(ft / 3) * sway / 90 + hb; cam.eye.vz = -c[2];
    cam.at.vx = c[3]; cam.at.vy = -c[4]; cam.at.vz = -c[5];
    gfx_camera_update();
    gfx_set_identity();
    floor_draw(ft); backdrop_draw(ft); desk_draw(); crowd_draw(ft);
    gfx_set_reserve(RESERVE_UI);
    for (int i = 0; i < A_N; i++) actor_draw(i);
    gfx_set_reserve(0);
    gfx_set_identity(); glass_draw(ft);
    // the lens: letterbox sliding in, a flash on the break, the fade in and out, then the words
    int lb = MIN(24, MIN(ft / 2, (FILM_END + T(8) - ft) / 2 + 0)); if (lb < 0) lb = 0;
    gfx_rect(0, 0, SCREEN_W, lb, 0, 0, 0, 0); gfx_rect(0, SCREEN_H - lb, SCREEN_W, lb, 0, 0, 0, 0);
    if (ft >= GLASS_BREAK && ft < GLASS_BREAK + 8) { int f = 255 - (ft - GLASS_BREAK) * 30; gfx_ui_blend(1); gfx_rect(0, 0, SCREEN_W, SCREEN_H, f, f, f, 1); }
    subtitle_draw();
    if (ft > T(2) && ft < T(46)) {   // the title card over the opening crane
        int a = MIN(MIN(ft - T(2), T(46) - ft), 30) * 255 / 30;
        const char *s = "TV SHOW & TELL"; int w = font_w_zoom(2, s);
        font_text_zoom(160 - w / 2 + 2, 90 + 2, 2, s, 0, 0, 0);
        font_text_zoom(160 - w / 2, 90, 2, s, 255 * a >> 8, 220 * a >> 8, 90 * a >> 8);
        ui_textc(160, 128, 1, "STARRING MISSY TELLER", a > 128 ? UI_WHITE : UI_SOFT);
    }
    if (ft > T(560)) { const char *s = "MISSY TELLER"; int w = font_w_zoom(2, s); int a = MIN(ft - T(560), 30) * 255 / 30; font_text_zoom(160 - w / 2, 96, 2, s, 255 * a >> 8, 120 * a >> 8, 200 * a >> 8);
                         ui_textc(160, 124, 1, "HERE TODAY...", a > 128 ? UI_WHITE : UI_SOFT); }
    int fa = ft < FADE_IN ? 255 - ft * 255 / FADE_IN : ft > FILM_END ? MIN(255, (ft - FILM_END) * 255 / T(6)) : 0;
    if (skipped) fa = MAX(fa, MIN(255, done_t * 14));
    if (fa > 0) { gfx_ui_blend(2); gfx_rect(0, 0, SCREEN_W, SCREEN_H, fa, fa, fa, 1); gfx_ui_blend(1); }
    if (ft > T(30) && ft < T(60) && !skipped) ui_text(250, 8 + 24, 1, "START SKIP", UI_SOFT);
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, cut_open, cut_update, cut_draw };
