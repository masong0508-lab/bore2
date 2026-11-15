// cutscene.c - the CUTSCENE ENGINE (CUT.OVL) and the first film in it: MISSY TELLER's opening, "TV SHOW & TELL".
// A film is DATA: camera keys (a Catmull-Rom path through them, with hard CUTS between shots), actor walks (smooth, with a real
// stride from the distance covered), pose tracks (every pose is a full-body VPose, blended, so nobody ever pops), a prop with
// its own path, subtitles that type themselves in, fades, letterbox and a few voice cues. The film plays on a clock of 60 Hz
// steps and is drawn from that clock alone, so a slow frame costs picture and never timing or smoothness.
// Units are the fight's: x and z on the ground (z the fight's, the world's -z), y up, a Bore 384 tall, yaw 0..255 (0 looks along +z).
#include "ovl.h"
#include "gfx.h"
#include "ui.h"
#include "audio.h"
#include "font.h"

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
static int film_Lv[3] = { -1840, -2460, -2600 }, film_Rv[3] = { 2200, -1200, 3200 };   // the key light and the rim light, in the camera's space (x right, y down, z away)
#include "film_toon.inc"
#include "film_cast.inc"

// ---- the poses (FPose: see film_cast.inc for what every number does) ----
#define P(a0,a1,r0,r1,e0,e1, l0,l1,lr0,lr1,k0,k1, ln,tw,nd,tl,dr,ai) { {a0,a1},{r0,r1},{e0,e1},{l0,l1},{lr0,lr1},{k0,k1},ln,tw,nd,tl,dr,ai }
enum { Q_HANG, Q_TALK, Q_OPEN, Q_WAVE, Q_PRES, Q_POINT, Q_HIP, Q_CLAP, Q_CHEER, Q_RECOIL, Q_SERVE, Q_PUSH, Q_DANA, Q_DANB, Q_TEMPT, Q_BOW, Q_WIN, Q_N };
static const FPose pq[Q_N] = {
 /* HANG   */ P(  60,  60,  120, 120,  140, 140,    0,   0,  50,  50,  30,  30,    0,   0,    0,   0,  0,  0),
 /* TALK   */ P(  80, 520,  120, 200,  160, 980,    0,  40,  50,  60,  30,  70,   20,  60,   40,   0,  0,  0),
 /* OPEN   */ P( 160, 160,  880, 880,  260, 260,    0,   0,  90,  90,  20,  20,  -40,   0,  -40,   0,  0,  0),
 /* WAVE   */ P(  60, 600,  120, 760,  140,1250,    0,   0,  50,  50,  30,  30,    0,   0,    0, 110,  0,  0),
 /* PRES   */ P( 950, 300,  420, 140,  200,1500,    0,  60,  60,  60,  40,  90,   40, -90,   30,   0,  0,  0),
 /* POINT  */ P( 100,1250,  600, 220,  1500,120,    0,   0,  50,  50,  30,  30,   30, 130,   30,   0,  0,  0),
 /* HIP    */ P(-120,-120,  520, 520, 1250,1250,    0,   0,  60,  60,  30,  30,    0,   0,    0,  80,  0,  0),
 /* CLAP   */ P( 600, 600, -330,-330,  520, 520,    0,   0,  40,  40,  40,  40,   20,   0,   40,   0,  0,  0),
 /* CHEER  */ P(2150,2150,  300, 300,  150, 150,  -60,  60, 140, 140, 180, 180,  -50,   0, -120,   0, 10, 24),
 /* RECOIL */ P( 450, 450,  720, 720,  800, 800, -120, 120,  90,  90, 260, 140, -240,   0,  -60,   0, 30,  0),
 /* SERVE  */ P( 600, 600,   30, 30,   780, 780,    0,   0,  40,  40,  30,  30,   40,   0,   90,   0,  0,  0),
 /* PUSH   */ P(  80,600,  300, 260,  300, 140,    0,   0,  60,  60,  30,  30,   90,-130,   60,   0,  0,  0),
 /* DANA   */ P(2000, 500,  250, 620,  200, 800,  180, -90, 120,  60, 150, 200,   40, 260,  -40, 110, 14,  0),
 /* DANB   */ P( 500,2000,  620, 250,  800, 200,  -90, 180,  60, 120, 200, 150,   40,-260,  -40,-110, 14,  0),
 /* TEMPT  */ P( 120, 200,  200, 560,  200,1300,    0,  60,  60,  60,  30,  60,  200,   0,  150,  80,  0,  0),
 /* BOW    */ P( 120, 120,  160, 160,  200, 200,    0,   0,  50,  50,  30,  30,  620,   0,  200,   0,  0,  0),
 /* WIN    */ P( 260,2300,  520, 330, 1300, 140,   40, -40, 140, 140,  60,  60,  -70,   0, -170,   0, 0, 0) };
#undef P
static void plerp(FPose *d, const FPose *a, const FPose *b, int k) { const s16 *aa = (const s16 *)a, *bb = (const s16 *)b; s16 *dd = (s16 *)d; for (int i = 0; i < (int)(sizeof(FPose) / 2); i++) dd[i] = (s16)(aa[i] + (((bb[i] - aa[i]) * k) >> 10)); }

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
    CK_(0,    -1500, 1050, -2500,   100, 200, 0, 0),   CK_(25, -500, 600, -2150, 0, 210, 0, 0),   CK_(50, 300, 420, -1800, 0, 200, 0, 0),
    // 2. Chad bounds in from the left; the camera tracks him across
    CK_(50,   -600, 250, -1250,  -600, 230, 0, 1),    CK_(70, -520, 250, -1200, -650, 230, 0, 0),   CK_(105, -150, 245, -600, -100, 235, 0, 0),
    // 3. the audience goes wild: a low sweep along the front row
    CK_(105,  -950, 120, -820,  -1600, 190, -250, 1),  CK_(130, -950, 130, -300, -1600, 220, 250, 0),  CK_(150, -900, 140, 100, -1600, 230, 400, 0),
    // 4. Chad, mid-shot, easing in
    CK_(150,  260, 270, -760,   -20, 265, 0, 1),       CK_(199, 90, 275, -640, -30, 270, 0, 0),
    // 5. Chad, an extreme close-up on the build to the name
    CK_(200,  150, 325, -470,   -100, 322, -60, 1),    CK_(215, 80, 325, -430, -100, 325, -60, 0),
    // 6. the reveal of Missy from low and in front (after the red flash cut on her name)
    CK_(219,  430, 130, -620,   975, 300, -150, 1),    CK_(232, 520, 150, -540, 975, 290, -150, 0),  CK_(243, 590, 200, -450, 975, 285, -150, 0),
    // 7. two-shot, wide, a slow dolly
    CK_(244,  520, 330, -1550,  500, 220, 0, 1),       CK_(285, 640, 300, -1400, 560, 225, 0, 0),
    // 8. the intern comes on with the tray
    CK_(285,  100, 260, -1250,  600, 190, -500, 1),    CK_(335, 230, 250, -1100, 740, 190, -300, 0),
    // 9. close on Missy: the tray, the glass, the face
    CK_(335,  790, 360, -640,   975, 335, -150, 1),    CK_(365, 800, 360, -600, 975, 335, -150, 0),  CK_(385, 815, 355, -560, 975, 335, -150, 0),
    // 10. the glass, close: then the camera falls with it
    CK_(385,  830, 240, -380,   850, 195, -215, 1),    CK_(395, 825, 235, -360, 850, 195, -215, 0),  CK_(405, 800, 200, -420, 800, 90, -215, 0),  CK_(420, 760, 150, -480, 790, 50, -215, 0),
    // 11. everyone reacts: a wide, then the swing round to Missy
    CK_(420,  250, 360, -1600,  500, 230, 0, 1),       CK_(465, 430, 300, -1450, 600, 235, 0, 0),
    // 12. Missy, low and heroic against the sun: a sweeping orbit across her front, audience-left round to audience-right (after the flash cut)
    CK_(473,  610, 100, -540,   975, 300, -150, 1),    CK_(505, 470, 110, -250, 975, 305, -150, 0),  CK_(545, 640, 125, 250, 975, 300, -150, 0),
    // 13. the pull-back to the whole stage for the title
    CK_(545,  200, 520, -2100,  400, 230, 0, 1),       CK_(600, 80, 600, -2500, 400, 230, 0, 0) };

#define WK_(t,x,z,y) { T(t), x, z, y }
static const WK chad_w[] = { WK_(0,-1500,-60,64), WK_(50,-1500,-60,64), WK_(76,-130,-60,64), WK_(88,-120,-60,128), WK_(190,-120,-60,128), WK_(200,-100,-60,170), WK_(345,-100,-60,170), WK_(355,-100,-60,128) };
static const WK missy_w[] = { WK_(0,975,-150,192), WK_(280,975,-150,192), WK_(292,975,-150,196), WK_(340,975,-150,184), WK_(385,975,-150,176), WK_(430,975,-150,176), WK_(470,975,-150,192) };
static const WK intern_w[] = { WK_(0,1500,-1100,225), WK_(285,1500,-1100,225), WK_(318,1100,-600,225), WK_(336,740,-300,36), WK_(350,740,-300,36), WK_(372,600,-760,2), WK_(520,600,-760,2) };

#define QK_(t,q,b) { T(t), q, b }
static const QK chad_q[] = { QK_(0,Q_HANG,1), QK_(50,Q_HANG,1), QK_(88,Q_PRES,18), QK_(110,Q_OPEN,20), QK_(140,Q_CHEER,18), QK_(150,Q_TALK,24), QK_(172,Q_PRES,20), QK_(200,Q_POINT,16),
                             QK_(225,Q_CLAP,14), QK_(250,Q_TALK,20), QK_(305,Q_HANG,24), QK_(420,Q_RECOIL,10), QK_(436,Q_OPEN,22), QK_(455,Q_CHEER,12), QK_(500,Q_DANA,12), QK_(510,Q_DANB,12), QK_(520,Q_DANA,12),
                             QK_(530,Q_DANB,12), QK_(540,Q_WIN,14) };
static const QK missy_q[] = { QK_(0,Q_HANG,1), QK_(205,Q_HIP,22), QK_(228,Q_WAVE,14), QK_(250,Q_HIP,16), QK_(278,Q_TALK,16), QK_(300,Q_HIP,24), QK_(345,Q_TEMPT,40), QK_(370,Q_HANG,40), QK_(380,Q_TEMPT,20),
                              QK_(392,Q_PUSH,8), QK_(404,Q_PUSH,40), QK_(425,Q_RECOIL,6), QK_(440,Q_POINT,14), QK_(458,Q_HIP,16), QK_(476,Q_DANA,10), QK_(488,Q_DANB,10), QK_(500,Q_DANA,10), QK_(512,Q_DANB,10),
                              QK_(524,Q_DANA,10), QK_(536,Q_WIN,12), QK_(560,Q_BOW,30) };
static const QK intern_q[] = { QK_(0,Q_HANG,1), QK_(296,Q_SERVE,12), QK_(336,Q_SERVE,12), QK_(350,Q_HANG,20), QK_(420,Q_RECOIL,6), QK_(440,Q_CLAP,14), QK_(500,Q_CHEER,14) };

static const GK glass_k[] = { { T(285), 300, 0, -1500, 0 }, { T(336), 786, 206, -237, 0 }, { T(350), 850, 192, -215, 0 }, { T(392), 850, 192, -215, 0 }, { T(403), 815, 192, -215, 0 }, { T(409), 788, 192, -215, 0 }, { T(424), 765, 0, -215, 1 } };
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
    { T(374), T(392), 2, "HMM." },
    { T(396), T(418), 2, "NOT TONIGHT, DARLING." },
    { T(436), T(466), 1, "SHE IS BACK, FOLKS!" },
    { T(470), T(520), 2, "WATCH ME DANCE." } };
#include "film_mix.inc"      // the sound mix (tools/make_film_mix.py, docs/FILM_MIX.md): film_mix_open[] (what plays when: foley, crowd, shouts, ducks, fades), film_lines_open[] (each line's level and place)
static const struct { u16 t; u8 lvl; } cheers[] = { { T(0), 40 }, { T(100), 255 }, { T(160), 110 }, { T(228), 255 }, { T(262), 120 }, { T(424), 40 }, { T(432), 255 }, { T(560), 255 } };
#define FILM_END T(600)

// ---- the people (film_toon.inc, film_cast.inc: soft shapes drawn as cels; they do not use the game's Bores) ----
typedef struct { int x, z, yaw, lx, lz, amp, phase; FPose pose; } Act;
static Act act[A_N];
static int ft, cue_i, lights, skipped, done_t, gal, fh, last_fh;   // fh: the people's clock, held (a new drawing every HOLD steps)   // gal: the pose gallery and model sheet (tests: AUTO_CUT=9999, 9998)
static int grade_keep, pose_t;

// ---- the STORY FILMS (film_story.inc): the same studio and cast, a fixed set of camera moves, poses by beat ----
#include "film_story.inc"
static int mini, mini_id;                         // 0 the opening film; 1 a story's opening, 2 its ending (cut_open's arg 1000 + id, 2000 + id)
#define M_PL T(30)                                // the title plate over the opening crane
#define M_BEAT T(34)                              // every beat: one line, one camera move
static int mini_base(void) { return mini == 1 ? M_PL : 0; }
static int mini_total(void) { return mini == 1 ? M_PL + 4 * M_BEAT + T(12) : 4 * M_BEAT + T(40); }
static int mini_k(void) { int b = ft - mini_base(); return b < 0 ? -1 : MIN(b / M_BEAT, mini == 1 ? 3 : 4); }   // the beat now (-1: the plate; 4: the end card of an ending)
static int mini_spk_actor(int k) { static const u8 a[5] = { A_CHAD, A_INTERN, A_MISSY, A_CHAD, A_CHAD }; return a[MAX(0, MIN(k, 4))]; }
static const s16 msh[5][9] = {      // shot: eye at the start, eye at the end, the aim
    { -900, 420, -2300,  300, 380, -1900,   0, 220, 0 },       // 0 the wide crane
    {  -60, 270, -880,  -230, 285, -760,   -330, 275, 0 },     // 1 Chad, medium, easing in
    {  330, 250, -1150,  140, 240, -980,   -30, 230, -420 },   // 2 Missy
    {  520, 110, -760,   400, 125, -640,   260, 300, -60 },    // 3 the Intern
    {  120, 420, -1500,  -60, 640, -2300,   0, 220, 0 } };     // 4 the pull-back
static void mini_cam(int *o) {
    int k = mini_k(), sh = mini == 1 && k < 0 ? 0 : k < 0 ? 0 : k == 0 ? 1 : k == 1 ? 3 : k == 2 ? 2 : 4;
    int t0 = k < 0 ? 0 : mini_base() + k * M_BEAT, len = k < 0 ? M_PL : k == 4 ? T(40) : M_BEAT, u = smooth(CLAMP((ft - t0) * 1024 / len, 0, 1024));
    const s16 *q = msh[sh];
    for (int i = 0; i < 3; i++) { o[i] = q[i] + ((q[3 + i] - q[i]) * u >> 10); o[3 + i] = q[6 + i]; }
}
static int mini_cheer(int t) { int k = mini_k(); (void)t; return k == 2 ? 255 : k >= 4 ? 255 : k == 3 ? 130 : 60; }
static int mini_flash_t(void) { return mini_base() + 2 * M_BEAT; }
static int mini_q(int who, int k) {   // the pose of a person in beat k (-1: the plate)
    int spk = mini_spk_actor(k < 0 ? 0 : k);
    if (k >= 4) return Q_CHEER;
    if (who == spk) return who == A_CHAD ? (k == 0 ? Q_PRES : k == 3 ? Q_OPEN : Q_TALK) : who == A_MISSY ? (k == 2 ? Q_POINT : Q_TALK) : Q_TALK;
    return who == A_MISSY ? Q_HIP : (k == 2 && who == A_CHAD ? Q_CLAP : Q_HANG);
}
static void mini_poses(void) {
    static const struct { s16 x, z; u8 yaw; } at[A_N] = { { -330, -60, 150 }, { -30, -420, 128 }, { 260, -60, 112 } };
    int k = mini_k(), t0 = k < 0 ? 0 : mini_base() + k * M_BEAT, u = smooth(CLAMP((ft - t0) * 1024 / T(5), 0, 1024));
    for (int i = 0; i < A_N; i++) {
        Act *a = &act[i]; a->x = at[i].x; a->z = at[i].z; a->yaw = at[i].yaw; a->amp = 0;
        plerp(&a->pose, &pq[mini_q(i, k - 1)], &pq[mini_q(i, k)], u);
        a->pose.nod += (s16)(fsin(fh * 3 + i * 40) / 6); a->pose.drop += (s16)(fsin(fh * 2 + i * 70) / 10);
    }
}
static void mini_expr(int who, Expr *e, int *blink) {
    int k = mini_k(), spk = k >= 0 && k < 4 && mini_spk_actor(k) == who;
    e->mouth = who == A_CHAD ? M_GRIN : who == A_MISSY ? M_SMIRK : M_SMALL; e->eyes = k >= 4 ? E_HAPPY : E_OPEN; e->brow = who == A_MISSY && k == 2 ? 2 : who == A_INTERN ? 3 : 1;
    e->blush = 1; e->sweat = who == A_INTERN && k < 4; e->gx = e->gy = 0;
    if (spk) { static const u8 flap[8] = { 2, 1, 0, 2, 1, 2, 0, 1 }; e->mouth = flap[(fh / 5 * 3 + k * 5) & 7]; }
    *blink = ((fh / 5 + who * 11) % 31) == 0;
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
static void pose_at(const QK *k, int n, int t, FPose *o) {
    int i = 0; for (int j = 0; j < n; j++) if (k[j].t <= t) i = j;
    const FPose *to = &pq[k[i].q], *from = &pq[i > 0 ? k[i - 1].q : k[i].q];
    int bl = MAX(1, (int)k[i].bl * 3), u = i > 0 ? CLAMP((t - k[i].t) * 1024 / bl, 0, 1024) : 1024;
    plerp(o, from, to, smooth(u));
}
static int cheer_at(int t) {
    if (mini) return mini_cheer(t);
    int i = 0; const int n = sizeof cheers / sizeof cheers[0]; for (int j = 0; j < n; j++) if (cheers[j].t <= t) i = j;
    int dt = t - cheers[i].t, v = cheers[i].lvl;
    return v > 130 ? MAX(70, v - dt / 3) : v;
}

// the voice-over (FILMVOX.BIN: line i belongs to subs[i]): a line starts a moment after its subtitle does, and the subtitle stays up until the
// line is spoken (never into the next subtitle)
#define NSUBS ((int)(sizeof subs / sizeof subs[0]))
#define VOX_LEAD 3
static int vox_i, vox_n, vox_end;
static int sub_t1(int i) {
    int e = subs[i].t0 + VOX_LEAD + audio_film_steps(i) + 6, nx = i + 1 < NSUBS ? subs[i + 1].t0 - 1 : 0x7fff;
    if (e > nx) e = nx; return e > subs[i].t1 ? e : subs[i].t1;
}
static int speaker_now(void) { for (int i = 0; i < NSUBS; i++) if (ft >= subs[i].t0 && ft < sub_t1(i)) return subs[i].who; return 0; }
extern int gfx_grade;
static void film_leave(void) { audio_film_end(); gfx_grade = grade_keep; }

// ---- stepping ----
#define HOLD 1                        // the people are posed for every drawing (they used to move on fives: 12 a second, and a picture that moved 2 vblanks in 3 made the cadence stutter)
#define BEAT 5                        // blinks and the flap of a mouth keep their old pace (every fifth step)
#define FADE_IN T(12)
void cut_open(int arg) {
    grade_keep = gfx_grade; gfx_grade = 0;               // (the game's warm/cool grade off: the film's colours are the ones drawn)
    mini = 0; gal = arg >= 9998 ? arg : 0; ft = arg > 1 && !gal && arg < 1000 ? arg : 0;
    if (arg >= 1000 && arg < 3000 && arg % 1000 > 0 && arg % 1000 < MF_N) { mini = arg / 1000; mini_id = arg % 1000; ft = 0; } fh = ft - ft % HOLD; last_fh = -1; pose_t = fh; cue_i = 0; lights = 0; skipped = 0; done_t = 0;
    for (int i = 0; i < A_N; i++) { act[i].amp = 0; act[i].phase = 0; act[i].lx = -99999; }
    vox_n = gal || mini ? 0 : audio_film_load("FILMVOX.BIN"); vox_end = 0;
    for (vox_i = 0; vox_i < NSUBS && subs[vox_i].t0 + VOX_LEAD < ft; vox_i++) {}
    if (!mini) { if (vox_n) audio_cut_begin(film_mix_open, (int)(sizeof film_mix_open / sizeof film_mix_open[0]), film_lines_open, (int)(sizeof film_lines_open / sizeof film_lines_open[0]), 0, ft); else audio_voice(VOICE_NEED); }   // (the score is track 0 of FILMMUS.BIN; the mini-films and the gallery keep the game voice)
}
static void walk_pose(const Act *a, FPose *o) {   // a stride made from the phase: the hips swing, the knees fold behind, the arms counter-swing
    int s = fsin(a->phase), s2 = fsin(a->phase + 64);
    memset(o, 0, sizeof *o);
    o->leg[0] = (s16)(s * 5 / 4); o->leg[1] = (s16)(-s * 5 / 4); o->knee[0] = (s16)(MAX(0, s2) * 3); o->knee[1] = (s16)(MAX(0, -s2) * 3);
    o->arm[0] = (s16)(-s * 5 / 4); o->arm[1] = (s16)(s * 5 / 4); o->elbow[0] = o->elbow[1] = 260; o->lean = 60; o->drop = (s16)(ABS(s2) / 5);
    o->lroll[0] = o->lroll[1] = 40; o->aroll[0] = o->aroll[1] = 90;
}
static void poses_now(void) {
    if (mini) { mini_poses(); return; }
    static const struct { const WK *w; int nw; const QK *q; int nq; } tr[A_N] = {
        { chad_w, sizeof chad_w / sizeof chad_w[0], chad_q, sizeof chad_q / sizeof chad_q[0] },
        { missy_w, sizeof missy_w / sizeof missy_w[0], missy_q, sizeof missy_q / sizeof missy_q[0] },
        { intern_w, sizeof intern_w / sizeof intern_w[0], intern_q, sizeof intern_q / sizeof intern_q[0] } };
    for (int i = 0; i < A_N; i++) {
        Act *a = &act[i]; int x, z, yaw; mark_at(tr[i].w, tr[i].nw, fh, &x, &z, &yaw);
        int d = a->lx == -99999 ? 0 : (int)(ABS(x - a->lx) + ABS(z - a->lz));
        int dt = CLAMP(fh - pose_t, 1, 30), dn = d * 5 / dt;           // (the distance covered in five steps, whatever the gap since the last drawing)
        a->lx = x; a->lz = z; a->x = x; a->z = z; a->yaw = yaw;
        a->phase = (a->phase + d * 3 / 2) & 255;
        int want = dn > 0 ? CLAMP(dn * 90, 0, 1024) : 0;
        a->amp += (want - a->amp) * dt / 30; if (!want && a->amp < 16) a->amp = 0;
        FPose p; pose_at(tr[i].q, tr[i].nq, fh, &p);
        FPose w; walk_pose(a, &w); plerp(&a->pose, &p, &w, a->amp);
        a->pose.nod += (s16)(fsin(fh * 3 + i * 40) / 6);      // breathing
        a->pose.drop += (s16)(fsin(fh * 2 + i * 70) / 10);
    }
}

int cut_update(int held, int pressed, int steps) {
    if (steps < 1) steps = 1;
    if ((pressed & (PAD_START | PAD_CIRCLE | PAD_TRIANGLE)) && !skipped) { skipped = 1; if (vox_n) { audio_cut_fadeout(10); vox_n = 0; } }   // (a skip: the sound goes down in 10 steps, then film_leave stops it)
    if (skipped) { done_t += steps; if (done_t > 18) { film_leave(); return 0; } }
    else ft += steps;
    if (mini) { static int mc; if (ft < 2) mc = 0; if (!mc && ft >= mini_flash_t()) { mc = 1; audio_voice(VOICE_YEHA); } if (mc == 1 && mini_k() >= 4) { mc = 2; audio_voice(VOICE_WIN); } }
    else audio_cut_tick(ft, steps);     // (the mix: cues, ducks, fades)
    if (vox_n && !skipped) {
        if (vox_end && ft >= vox_end) { audio_film_line(-1, 0); vox_end = 0; }     // (the crowd comes back up)
        while (vox_i < NSUBS && subs[vox_i].t0 + VOX_LEAD <= ft) { int i = vox_i++; if (audio_film_steps(i)) { audio_film_line(i, 150); vox_end = ft + audio_film_steps(i) + 12; } }
    }
    if (mini ? ft >= mini_total() : ft >= FILM_END + T(8)) { film_leave(); return 0; }
    lights += (256 - lights) * MIN(steps, 8) / 40;
    fh = ft - ft % HOLD;
    if (fh != last_fh) { poses_now(); pose_t = last_fh = fh; }     // (a new drawing of the people)
    return 1;
}

// ---- the studio, drawn as a bright cartoon: a clean cyan-to-white backdrop in flat bands, ring-pattern panels, light shafts, sparkles; a chibi crowd ----
static const u16 beat_t[] = { T(105), T(228), T(392), T(436), T(545) };      // the white flash cuts
static const struct { u16 a, b; } line_w[] = { { T(226), T(240) }, { T(388), T(424) }, { T(436), T(452) }, { T(496), T(512) }, { T(545), T(558) } };   // the speed lines
#define FL1 T(216)     // the two graphic flash cuts (red / black / white): "MISSY TELLER!" and "WATCH ME DANCE."
#define FL2 T(470)
#define RESERVE_PLATE (8*1024)  // (more with a title plate up: its type is a lot of pieces)
#define RESERVE_FILM (2*1024)   // kept back from the people for the type, the plate and the lens
#define FL_COVER 14    // steps the flash covers the whole picture, then 4 more steps of white clearing
static inline void V3(SVECTOR *v, int x, int y, int z) { v->vx = (short)x; v->vy = (short)-y; v->vz = (short)-z; }
static u32 shade(int r, int g, int b, int k) { r = r * k >> 8; g = g * k >> 8; b = b * k >> 8; return UI_RGB(MIN(r, 255), MIN(g, 255), MIN(b, 255)); }
static void box_side(int x0, int y0, int z0, int x1, int y1, int z1, u32 c0, u32 c1) {   // a quad from (x0,y0,z0) top-left to (x1,y1,z1) bottom-right, any vertical plane
    SVECTOR a, b, c, d; V3(&a, x0, y1, z0); V3(&b, x1, y1, z1); V3(&c, x0, y0, z0); V3(&d, x1, y0, z1);
    gfx_quad_g(&a, &b, &c, &d, c0, c0, c1, c1, 0);
}
static void wq(int x0, int y0, int x1, int y1, int Z, int r, int g, int b, int bias) {    // a flat rectangle on the back wall
    SVECTOR a, bb, c, d; V3(&a, x0, y1, Z); V3(&bb, x1, y1, Z); V3(&c, x0, y0, Z); V3(&d, x1, y0, Z);
    gfx_set_zbias(bias); gfx_quad_f(&a, &bb, &c, &d, r, g, b, 0); gfx_set_zbias(0);
}
static void floor_draw(int tf) {
    gfx_floor_pass = 1;
    const int N = 400;
    int ex = cam.eye.vx, ez = -cam.eye.vz, low = -cam.eye.vy < 330;
    for (int gz = -3; gz < 5; gz++) for (int gx = -5; gx < 5; gx++) {
        int x0 = gx * N, z0 = gz * N, ck = ((gx + gz) & 1) ? 0 : 1;
        int r = (200 + ck * 14) * lights >> 8, g = (226 + ck * 10) * lights >> 8, b = (247 + ck * 6) * lights >> 8;
        int m = (low && ABS(x0 + N / 2 - ex) < 640 && ABS(z0 + N / 2 - ez) < 640) ? 2 : 1, n = N / m;     // (a tile the camera skims over is cut in four: a quad that crosses the near plane is dropped whole, and a hole in the floor popped in for a drawing)
        for (int iz = 0; iz < m; iz++) for (int ix = 0; ix < m; ix++) {
            SVECTOR p[4]; V3(&p[0], x0 + ix * n, 0, z0 + iz * n); V3(&p[1], x0 + ix * n + n, 0, z0 + iz * n); V3(&p[2], x0 + ix * n, 0, z0 + iz * n + n); V3(&p[3], x0 + ix * n + n, 0, z0 + iz * n + n);
            gfx_quad_f(&p[0], &p[1], &p[2], &p[3], r, g, b, 0);
        }
    }
    gfx_floor_pass = 0;
    (void)tf;
}
static void wall_disc(int cx, int cy, int Z, int rad, int r, int g, int b, int bias) {   // a six-sided disc on the wall, in two quads
    int hx = rad / 2, hy = rad * 7 / 8; SVECTOR a, bb, c, d;
    gfx_set_zbias(bias);
    V3(&a, cx - hx, cy + hy, Z); V3(&bb, cx + hx, cy + hy, Z); V3(&c, cx - rad, cy, Z); V3(&d, cx + rad, cy, Z); gfx_quad_f(&a, &bb, &c, &d, r, g, b, 0);
    V3(&a, cx - rad, cy, Z); V3(&bb, cx + rad, cy, Z); V3(&c, cx - hx, cy - hy, Z); V3(&d, cx + hx, cy - hy, Z); gfx_quad_f(&a, &bb, &c, &d, r, g, b, 0);
    gfx_set_zbias(0);
}
static void spark(int x, int y, int Z, int s, int r, int g, int b) {   // a four-point star
    SVECTOR a, bb, c, d; int w = s / 4 + 1;
    V3(&a, x, y + s, Z); V3(&bb, x + w, y, Z); V3(&c, x - w, y, Z); V3(&d, x, y - s, Z); gfx_set_zbias(10); gfx_quad_f(&a, &bb, &c, &d, r, g, b, 0);
    V3(&a, x - s, y, Z); V3(&bb, x, y + w, Z); V3(&c, x, y - w, Z); V3(&d, x + s, y, Z); gfx_quad_f(&a, &bb, &c, &d, r, g, b, 0); gfx_set_zbias(0);
}
static void backdrop_draw(int tf) {
    const int Z = 1100, X0 = -2600, X1 = 2600, NB = 8;
    for (int i = 0; i < NB; i++) {   // the sky: eight flat bands from white at the floor to cyan at the top
        int y0 = i * 112, y1 = y0 + 112, u = i * 255 / (NB - 1);
        wq(X0, y0, X1, y1, Z, (230 - (230 - 100) * u / 255) * lights >> 8, (247 - (247 - 200) * u / 255) * lights >> 8, (255 - (255 - 240) * u / 255) * lights >> 8, 0);
    }
    int sc = (tf / 4) % 960;   // ring panels drifting slowly sideways: white ring, cyan, white eye
    for (int row = 0; row < 2; row++) for (int i = -6; i <= 6; i++) {
        int cx = i * 480 + (row ? 240 : 0) + sc - 480, cy = row ? 600 : 230;
        if (cx < -2500 || cx > 2500 || !gfx_visible(cx, -cy, -Z, 190)) continue;
        wall_disc(cx, cy, Z - 2, 160, 255 * lights >> 8, 255 * lights >> 8, 255 * lights >> 8, 2);
        wall_disc(cx, cy, Z - 3, 128, 120 * lights >> 8, 208 * lights >> 8, 246 * lights >> 8, 4);
    }
    for (int k = 0; k < 4; k++) {   // light shafts from the rig: a wide pale one with a narrow bright core
        int xt = -1500 + k * 600 + fsin(tf / 3 + k * 50) / 6, xb = xt + 420 + fsin(tf / 4 + k * 70) / 4, tw = 70, bw = 250;
        SVECTOR a, bb, c, d;
        V3(&a, xt - tw, 900, Z - 8); V3(&bb, xt + tw, 900, Z - 8); V3(&c, xb - bw, 0, Z - 8); V3(&d, xb + bw, 0, Z - 8); gfx_set_zbias(8);
        gfx_quad_f(&a, &bb, &c, &d, 196 * lights >> 8, 232 * lights >> 8, 254 * lights >> 8, 0);
        V3(&a, xt - tw / 3, 900, Z - 9); V3(&bb, xt + tw / 3, 900, Z - 9); V3(&c, xb - bw / 3, 0, Z - 9); V3(&d, xb + bw / 3, 0, Z - 9); gfx_set_zbias(10);
        gfx_quad_f(&a, &bb, &c, &d, 226 * lights >> 8, 246 * lights >> 8, 255 * lights >> 8, 0); gfx_set_zbias(0);
    }
    for (int k = 0; k < 12; k++) {   // sparkles twinkling
        int ph = (tf * 5 + k * 97) & 255, s = fsin(ph) > 0 ? fsin(ph) / 6 : 0; if (s < 4) continue;
        int x = -1900 + ((k * 7919) % 3800), y = 140 + ((k * 4271) % 640);
        if (gfx_visible(x, -y, -Z, 60)) spark(x, y, Z - 12, s, 255, k & 1 ? 250 : 255, k & 1 ? 190 : 255);
    }
    wq(X0, 0, X1, 26, Z - 12, 255, 255, 255, 12);    // the white skirting
}
static void desk_draw(void) {
    const int X0 = 790, X1 = 900, Zs = 380, HT = 190;
    SVECTOR a, b, c, d;
    V3(&a, X0, HT, -Zs); V3(&b, X1, HT, -Zs); V3(&c, X0, HT, Zs); V3(&d, X1, HT, Zs);
    gfx_quad_f(&a, &b, &c, &d, 255 * lights >> 8, 252 * lights >> 8, 240 * lights >> 8, 0);
    box_side(X0, HT, -Zs, X0, 0, Zs, shade(250, 250, 255, lights), shade(190, 215, 245, lights));      // the front: white, with a cyan and a yellow band
    box_side(X0, HT - 36, -Zs, X0, HT - 70, Zs, shade(110, 206, 240, lights), shade(110, 206, 240, lights));
    box_side(X0, HT - 70, -Zs, X0, HT - 84, Zs, shade(255, 242, 110, lights), shade(255, 242, 110, lights));
    box_side(X0, HT, -Zs, X1, 0, -Zs, shade(200, 225, 250, lights), shade(150, 190, 235, lights));
    box_side(X0, HT, Zs, X1, 0, Zs, shade(200, 225, 250, lights), shade(150, 190, 235, lights));
    for (int k = 0; k < 3; k++) {   // the score paddles, lying on the desk
        int z = -240 + k * 240; SVECTOR p0, p1, p2, p3; V3(&p0, 830, HT + 3, z - 40); V3(&p1, 870, HT + 3, z - 40); V3(&p2, 830, HT + 3, z + 40); V3(&p3, 870, HT + 3, z + 40);
        gfx_quad_f(&p0, &p1, &p2, &p3, 255 * lights >> 8, 120 * lights >> 8, 140 * lights >> 8, 0);
    }
}
static void pl4(int x, int z0, int y0, int z1, int y1, int z2, int y2, int z3, int y3, int r, int g, int b, int bias) {   // a flat quad in the plane x: corners (z,y): top left, top right, bottom left, bottom right
    SVECTOR a, bb, c, d; V3(&a, x, y0, z0); V3(&bb, x, y1, z1); V3(&c, x, y2, z2); V3(&d, x, y3, z3);
    gfx_set_zbias(bias); gfx_quad_f(&a, &bb, &c, &d, r, g, b, 0); gfx_set_zbias(0);
}
static void oct4(int x, int zc, int yc, int rd, int r, int g, int b, int bias) {   // a round head: an octagon in three quads
    int q = rd * 38 / 100;
    pl4(x, zc - q, yc + rd, zc + q, yc + rd, zc - q, yc - rd, zc + q, yc - rd, r, g, b, bias);
    pl4(x, zc - q, yc + rd, zc - rd, yc + q, zc - q, yc - rd, zc - rd, yc - q, r, g, b, bias);
    pl4(x, zc + rd, yc + q, zc + q, yc + rd, zc + rd, yc - q, zc + q, yc - rd, r, g, b, bias);
}
static void crowd_draw(int tf) {
    static const u8 hair[6][3] = { { 60, 36, 28 }, { 230, 90, 50 }, { 250, 214, 90 }, { 40, 40, 70 }, { 200, 90, 160 }, { 90, 60, 150 } };
    static const u8 shirt[6][3] = { { 255, 120, 150 }, { 255, 200, 60 }, { 90, 220, 140 }, { 90, 170, 255 }, { 190, 130, 255 }, { 255, 140, 70 } };
    int ch = cheer_at(tf);
    const int kk = lights;
    for (int row = 0; row < 2; row++) {
        int x = -1500 - row * 190, base = row * 90;
        { SVECTOR a, b, c, d; V3(&a, x + 90, base + 20, -900); V3(&b, x + 90, base + 20, 900); V3(&c, x + 90, 0, -900); V3(&d, x + 90, 0, 900);
          gfx_quad_f(&a, &b, &c, &d, 150 * kk >> 8, 168 * kk >> 8, 226 * kk >> 8, 0); }
        for (int i = 0; i < 9; i++) {
            int z = -780 + i * 200 + ((row * 53) & 63) - 30, h = (row * 5 + i * 3) % 6;
            if (!gfx_visible(x, -(base + 150), -z, 130)) continue;
            int bob = ABS(fsin(tf / 2 + i * 31 + row * 17)) * ch >> 10, y0 = base + bob, bw = 128, bh = 128, rd = 38, yc = y0 + bh + rd + 4;      // (grown-ups: a head a sixth of the height, shoulders, a neck)
            const u8 *sh = shirt[(i + row * 2) % 6], *hr = hair[h];
            pl4(x, z - bw / 2, y0 + bh, z + bw / 2, y0 + bh, z - bw / 2, y0 + 4, z + bw / 2, y0 + 4, sh[0] * kk >> 8, sh[1] * kk >> 8, sh[2] * kk >> 8, 4);
            if (ch > 90) {   // hands up
                int up = (fsin(tf + i * 40) >> 4) + 24;
                for (int s = 0; s < 2; s++) { int zz = z + (s ? 1 : -1) * (bw / 2 + 6);
                    pl4(x, zz - 8, y0 + bh + 60 + up, zz + 8, y0 + bh + 60 + up, zz - 8, y0 + bh - 20, zz + 8, y0 + bh - 20, sh[0] * kk >> 8, sh[1] * kk >> 8, sh[2] * kk >> 8, 6);
                    pl4(x, zz - 11, y0 + bh + 80 + up, zz + 11, y0 + bh + 80 + up, zz - 11, y0 + bh + 56 + up, zz + 11, y0 + bh + 56 + up, 252 * kk >> 8, 206 * kk >> 8, 170 * kk >> 8, 8); }
            }
            pl4(x, z - 11, yc - rd + 8, z + 11, yc - rd + 8, z - 12, y0 + bh - 4, z + 12, y0 + bh - 4, 238 * kk >> 8, 190 * kk >> 8, 160 * kk >> 8, 7);    // the neck
            oct4(x, z, yc, rd + 3, 36 * kk >> 8, 16 * kk >> 8, 40 * kk >> 8, 8);      // the head: ink, then skin
            oct4(x, z, yc, rd, 255 * kk >> 8, 214 * kk >> 8, 182 * kk >> 8, 10);
            pl4(x, z - rd * 5 / 10, yc + rd * 12 / 10, z + rd * 5 / 10, yc + rd * 12 / 10, z - rd * 105 / 100, yc + rd * 20 / 100, z + rd * 105 / 100, yc + rd * 20 / 100, hr[0] * kk >> 8, hr[1] * kk >> 8, hr[2] * kk >> 8, 12);
            for (int s = 0; s < 2; s++) { int zz = z + (s ? 1 : -1) * rd * 38 / 100;
                pl4(x, zz - 5, yc + 2 + 6, zz + 5, yc + 2 + 6, zz - 5, yc - 9, zz + 5, yc - 9, 40 * kk >> 8, 24 * kk >> 8, 70 * kk >> 8, 14);                // the big eye
                pl4(x, zz - 4, yc + 6, zz + 0, yc + 6, zz - 4, yc + 1, zz + 0, yc + 1, 255 * kk >> 8, 255 * kk >> 8, 255 * kk >> 8, 16);                    // its highlight
                int zc = z + (s ? 1 : -1) * rd * 66 / 100;
                pl4(x, zc - 7, yc - 10, zc + 7, yc - 10, zc - 7, yc - 17, zc + 7, yc - 17, 255 * kk >> 8, 176 * kk >> 8, 170 * kk >> 8, 12); }              // the cheek
            if (ch > 90) pl4(x, z - 8, yc - 17, z + 8, yc - 17, z - 6, yc - 27, z + 6, yc - 27, 150 * kk >> 8, 30 * kk >> 8, 50 * kk >> 8, 14);      // a shout
            else pl4(x, z - 7, yc - 19, z + 7, yc - 19, z - 5, yc - 22, z + 5, yc - 22, 150 * kk >> 8, 30 * kk >> 8, 50 * kk >> 8, 14);
        }
    }
}
static int glass_at(int t, int *o) {   // o: x y z; 0 if not there
    if (t < GLASS_APPEAR || t >= GLASS_BREAK) return 0;
    const int n = sizeof glass_k / sizeof glass_k[0]; int i = 0; for (int j = 0; j < n - 1; j++) if (glass_k[j].t <= t) i = j;
    const GK *a = &glass_k[i], *b = &glass_k[i + 1];
    if (i == 0) {   // on the tray: follows the intern's hand
        const Act *n_ = &act[A_INTERN]; o[0] = n_->x + (fsin(n_->yaw) * 78 >> 8); o[2] = n_->z + (fcos(n_->yaw) * 78 >> 8); o[1] = 206; return 1;
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
// what is on their faces: keyed states (a mouth at rest, the eyes, the brows, cheeks, a drop of sweat), and a mouth that flaps while a line is spoken
static const struct { u16 t; u8 who, mouth, eyes, brow, blush, sweat; } emo[] = {
    { T(0),   A_CHAD,   M_GRIN,  E_OPEN,  1, 1, 0 }, { T(420), A_CHAD,   M_O,     E_OPEN,  1, 0, 0 }, { T(436), A_CHAD,   M_GRIN,  E_HAPPY, 1, 1, 0 }, { T(470), A_CHAD,   M_GRIN,  E_OPEN, 1, 1, 0 },
    { T(0),   A_MISSY,  M_SMIRK, E_OPEN,  0, 0, 0 }, { T(374), A_MISSY,  M_CLOSED, E_OPEN, 2, 0, 0 }, { T(396), A_MISSY,  M_SMIRK, E_OPEN, 1, 0, 0 }, { T(440), A_MISSY,  M_SMIRK, E_OPEN, 0, 1, 0 }, { T(476), A_MISSY,  M_GRIN, E_HAPPY, 1, 1, 0 },
    { T(0),   A_INTERN, M_SMALL, E_OPEN,  3, 1, 1 }, { T(372), A_INTERN, M_CLOSED, E_OPEN, 3, 1, 1 }, { T(420), A_INTERN, M_O,     E_OPEN,  1, 0, 1 }, { T(440), A_INTERN, M_GRIN,  E_HAPPY, 1, 1, 0 } };
static int speaking(int who) {
    if (mini) { int k = mini_k(); return k >= 0 && k < 4 && mini_spk_actor(k) == who ? k : -1; }   // the line being spoken by this person now: its index, or -1
    for (int i = 0; i < NSUBS; i++) if (subs[i].who == who + 1) { int a = subs[i].t0 + VOX_LEAD, b = a + (vox_n ? audio_film_steps(i) : subs[i].t1 - subs[i].t0 - VOX_LEAD); if (ft >= a && ft < b) return i; }
    return -1;
}
static void expr_now(int who, Expr *e, int *blink) {
    if (mini) { mini_expr(who, e, blink); return; }
    int k = 0; for (unsigned j = 0; j < sizeof emo / sizeof emo[0]; j++) if (emo[j].who == who && emo[j].t <= ft) k = (int)j;
    e->mouth = emo[k].mouth; e->eyes = emo[k].eyes; e->brow = emo[k].brow; e->blush = emo[k].blush; e->sweat = emo[k].sweat; e->gx = e->gy = 0;
    int sp = speaking(who);
    if (sp >= 0) { static const u8 flap[8] = { 2, 1, 0, 2, 1, 2, 0, 1 }; e->mouth = flap[(fh / BEAT * 3 + sp * 5) & 7]; if (e->eyes == E_HAPPY) e->eyes = E_OPEN; }
    *blink = ((fh / BEAT + who * 11) % 31) == 0;
}
static void actor_draw(int i) {
    Act *a = &act[i];
    if (!mini && i == A_INTERN && ft < T(285)) return;
    if (!gfx_visible(a->x, -200, -a->z, 320)) return;
    FPose p = a->pose;
    if (speaking(i) >= 0) {   // the one speaking nods and gestures
        p.nod += (s16)(fsin(fh * 7) / 3); p.arm[1] += (s16)(fsin(fh * 5 + 30) / 2); p.tilt += (s16)(fsin(fh * 3) / 5);
    }
    Expr e; int blink; expr_now(i, &e, &blink);
    fig_draw(i, a->x, a->z, a->yaw, &p, &e, blink);
    VECTOR sh = { a->x, 0, -a->z }; gfx_shadow(&sh, i == A_INTERN ? 50 : 70);
}

// ---- drawing ----
static void subtitle_draw(void) {
    for (unsigned i = 0; i < sizeof subs / sizeof subs[0]; i++) {
        const Sub *s = &subs[i]; if (ft < s->t0 || ft >= sub_t1(i)) continue;
        char buf[48]; int n = (ft - s->t0) / 2 + 1, L = (int)strlen(s->tx); if (n > L) n = L; memcpy(buf, s->tx, n); buf[n] = 0;
        int fade = MIN(MIN(ft - s->t0, sub_t1(i) - ft), 8);
        static const u32 nc[4] = { UI_WHITE, UI_RGB(255, 210, 80), UI_RGB(255, 120, 200), UI_RGB(120, 220, 140) };
        static const char *const nm[4] = { "", "CHAD", "MISSY", "INTERN" };
        int y = SCREEN_H - 62; int w = ui_tw(1, s->tx) + 24;
        gfx_ui_blend(2); gfx_rect(160 - w / 2, y - 3, w, s->who ? 28 : 16, 70 * fade / 8, 70 * fade / 8, 70 * fade / 8, 1); gfx_ui_blend(1);
        if (s->who) { ui_textc(160, y, 1, nm[s->who], nc[s->who]); ui_textc(160, y + 12, 1, buf, UI_WHITE); } else ui_textc(160, y + 2, 1, buf, UI_WHITE);
    }
}
static void gallery_draw(void) {   // tests: AUTO_CUT=9999 six poses of the library at a time, front on (a page every 3 s); 9998 the model sheet: every person in front, 3/4 and side views and three mouths
    static int pg; pg++;
    gfx_set_clear(236, 230, 250); gfx_set_ambient(110, 120); gfx_set_fog(0, 0);
    FPose st = pq[Q_HANG]; Expr e = { M_CLOSED, E_OPEN, 0, 0, 0, 0, 0 };
    if (gal == 9998) {
        int cell = (pg / 8) % 18, who = cell / 6, v = cell % 6;
        int yaw = v == 0 ? 128 : v == 1 ? 96 : v == 2 ? 64 : 112; e.mouth = v < 3 ? M_CLOSED : v - 3 == 0 ? M_CLOSED : v - 3 == 1 ? M_SMALL : M_WIDE;
        if (v >= 3) e.mouth = (u8)(v - 3 == 0 ? M_CLOSED : v - 3 == 1 ? M_SMALL : M_WIDE);
        if (v < 3) e.mouth = M_SMALL; if (who == A_MISSY && v < 3) e.mouth = M_SMIRK; if (who == A_CHAD && v < 3) e.mouth = M_GRIN;
        e.brow = 0; e.blush = 1; e.sweat = who == A_INTERN && v < 3;
        Joints J; fk(&body_k[who], &st, &J);
        int hc = J.HC.y, d = v < 3 ? 600 : 250, ay = v < 3 ? 215 : hc;
        cam.eye.vx = 0; cam.eye.vy = -ay; cam.eye.vz = d; cam.at.vx = 0; cam.at.vy = -ay; cam.at.vz = 0;
        gfx_camera_update(); gfx_set_identity();
        gfx_set_reserve(RESERVE_UI); g_sheet = 1; g_wpx = v < 3 ? 14 : 22; g_rim = 0;
        fig_draw(who, 0, 0, v < 3 ? yaw : 128, &st, &e, 0);
        g_sheet = 0; gfx_set_reserve(0);
        for (int b = 0; b < 5; b++) gfx_rect(2 + b * 5, 2, 4, 4, ((cell + 1) >> b) & 1 ? 255 : 0, ((cell + 1) >> b) & 1 ? 255 : 0, ((cell + 1) >> b) & 1 ? 255 : 0, 0);
        return;
    }
    cam.eye.vx = 0; cam.eye.vy = -300; cam.eye.vz = 1900; cam.at.vx = 0; cam.at.vy = -170; cam.at.vz = 0; gfx_camera_update(); gfx_set_identity();
    gfx_set_reserve(RESERVE_UI);
    int page = (pg / 180) % 3;
    for (int k = 0; k < 6; k++) { int i = page * 6 + k; if (i >= Q_N) break;
        FPose p = pq[i];
        fig_draw(A_CHAD, (k - 3) * 270 + 135, 0, 128, &p, &e, 0);
    }
    gfx_set_reserve(0);
}
// ---- the lens: graphic type, the flash cuts, the title plates, the flare ----
extern int font_sub;   // (font.c: 1 takes the glyph away from what is under it, 0 adds it)
static void blob2(int cx, int cy, int r, int c) {   // an additive round-ish glow: a plus and a box, an octagon
    gfx_rect(cx - r, cy - r * 4 / 10, r * 2, r * 8 / 10, c, c, c, 1); gfx_rect(cx - r * 4 / 10, cy - r, r * 8 / 10, r * 2, c, c, c, 1); gfx_rect(cx - r * 75 / 100, cy - r * 75 / 100, r * 150 / 100, r * 150 / 100, c, c, c, 1);
}
static void flare_draw(int t) {   // the lens flare of a low sun: a bright core, a long streak, ghosts along the line to the middle
    int sx = 262 - t / 6, sy = 40 + t / 12; gfx_ui_blend(1);
    int a = MIN(t, 12) * 255 / 12 * (120 + (ABS(fsin(t * 3)) >> 2)) >> 8;   // (a slight shimmer)
    gfx_grad(0, sy - 1, sx, 3, 0, a / 2 | a / 2 << 8 | a / 3 << 16, 0, a / 2 | a / 2 << 8 | a / 3 << 16, 1);
    gfx_grad(sx, sy - 1, SCREEN_W - sx, 3, a / 2 | a / 2 << 8 | a / 3 << 16, 0, a / 2 | a / 2 << 8 | a / 3 << 16, 0, 1);
    blob2(sx, sy, 14, a / 2); blob2(sx, sy, 7, a);
    for (int k = 1; k <= 3; k++) { int gx = sx + (160 - sx) * k * 7 / 10, gy = sy + (120 - sy) * k * 7 / 10; blob2(gx, gy, 6 + k * 3, a / 9); }
    gfx_ui_blend(1);
}
static void plate(int in, int out, const char *l1, const char *l2, const char *sub, int y0) { pl_plate(in, out, l1, l2, sub, y0, 106, 206, 239); }
#ifdef FILM_PROF      // test builds: a log of the film's cost per drawing (hblanks of CPU in each part, packet bytes, overflow); read from a RAM dump with test/ramsym.py
u16 film_log[600][7]; u8 film_ps_log[600][5]; int film_log_n; static int fp[8];
#define PROF(i) (fp[i] = VSync(1))
#else
#define PROF(i)
#endif

static void mini_draw(void) {
    const MFilm *F = &mfilm[mini_id]; int k = mini_k(), total = mini_total();
    gfx_set_clear(150, 215, 245); gfx_set_fog(0, 0); gfx_set_ambient(96, 112);
    int f2 = ft - mini_flash_t();
    if (f2 >= 0 && f2 < 14) { pl_flash(f2, mini == 1 ? F->fo1 : F->fe1, mini == 1 ? F->fo2 : F->fe2, mini_id & 1); return; }
    int c[6]; mini_cam(c);
    int sway = 1;
    cam.eye.vx = c[0] + fsin(ft / 2) * sway / 90; cam.eye.vy = -(c[1]) + fcos(ft / 3) * sway / 90; cam.eye.vz = -c[2];
    cam.at.vx = c[3]; cam.at.vy = -c[4]; cam.at.vz = -c[5];
    gfx_camera_update(); gfx_set_identity();
    { int dx = c[0] - c[3], dy = c[1] - c[4], dz = c[2] - c[5]; int d2 = dx * dx + dy * dy + dz * dz; g_wpx = d2 < 700 * 700 ? 22 : d2 < 1200 * 1200 ? 17 : 13; }
    int hero = k == 2;
    g_rim = 1; g_rimc = hero ? BGR(255, 246, 214) : BGR(196, 236, 255);
#ifndef MX_NOFLOOR
    floor_draw(ft);
#endif
#ifndef MX_NOBACK
    backdrop_draw(ft);
#endif
#ifndef MX_NOCROWD
    crowd_draw(ft);
#endif
    g_noink = 0;
    gfx_set_reserve(RESERVE_PLATE);
#ifndef MX_NOACT
    for (int i = 0; i < A_N; i++) actor_draw(i);
#endif
    gfx_set_reserve(0);
    if (f2 >= 14 && f2 < 18) { int f = 255 - (f2 - 14) * 64; gfx_ui_blend(1); gfx_rect(0, 0, SCREEN_W, SCREEN_H, f, f, f, 1); }
    int lb = MIN(24, MIN(ft / 2, (total - ft) / 2)); if (lb < 0) lb = 0;
    gfx_rect(0, 0, SCREEN_W, lb, 0, 0, 0, 0); gfx_rect(0, SCREEN_H - lb, SCREEN_W, lb, 0, 0, 0, 0);
    if (k >= 0 && k < 4 && !(f2 >= 0 && f2 < 18)) {   // the line of the beat, typed in
        const char *tx = mini == 1 ? F->open[k] : F->end[k]; int t0 = mini_base() + k * M_BEAT + T(3), t1 = mini_base() + (k + 1) * M_BEAT - T(2);
        if (ft >= t0 && ft < t1) {
            static const char *const nm[4] = { "CHAD", "INTERN", "MISSY", "CHAD" }; static const u32 nc[4] = { UI_RGB(255, 210, 80), UI_RGB(120, 220, 140), UI_RGB(255, 120, 200), UI_RGB(255, 210, 80) };
            char buf[48]; int n = (ft - t0) / 2 + 1, L = (int)strlen(tx); if (n > L) n = L; memcpy(buf, tx, n); buf[n] = 0;
            int fade = MIN(MIN(ft - t0, t1 - ft), 8), y = SCREEN_H - 62, w = ui_tw(1, tx) + 24;
            gfx_ui_blend(2); gfx_rect(160 - w / 2, y - 3, w, 28, 70 * fade / 8, 70 * fade / 8, 70 * fade / 8, 1); gfx_ui_blend(1);
            ui_textc(160, y, 1, nm[k], nc[k]); ui_textc(160, y + 12, 1, buf, UI_WHITE);
        }
    }
    if (mini == 1 && ft > T(2) && ft < T(29)) pl_plate(ft - T(2), T(28) - ft, F->l1, F->l2, F->sub, 40, F->col[0], F->col[1], F->col[2]);
    if (mini == 2 && k >= 4) { static char both[28]; snprintf(both, sizeof both, "%s %s", F->l1, F->l2); pl_plate(ft - 4 * M_BEAT, -1, "THE END", both, "THANK YOU, BORETOWN", 52, F->col[0], F->col[1], F->col[2]); }
    int fa = ft < T(10) ? 255 - ft * 255 / T(10) : ft > total - T(6) ? MIN(255, (ft - (total - T(6))) * 255 / T(6)) : 0;
    if (skipped) fa = MAX(fa, MIN(255, done_t * 14));
    if (fa > 0) { gfx_ui_blend(2); gfx_rect(0, 0, SCREEN_W, SCREEN_H, fa, fa, fa, 1); gfx_ui_blend(1); }
    if (ft > T(30) && ft < T(90) && !skipped) ui_text(250, 8 + 24, 1, "START SKIP", UI_SOFT);
}
void cut_draw(void) {
    if (gal) { gallery_draw(); return; }
    if (mini) { mini_draw(); return; }
    PROF(0);
    int f1 = ft - FL1, f2 = ft - FL2, fl = -1, fw = 0;
    if (f1 >= 0 && f1 < FL_COVER) { fl = f1; fw = 0; } else if (f2 >= 0 && f2 < FL_COVER) { fl = f2; fw = 1; }
    gfx_set_clear(150, 215, 245); gfx_set_fog(0, 0);
    gfx_set_ambient(96, 112);
    if (fl >= 0) { pl_flash(fl, fw ? "WATCH ME" : "MISSY", fw ? "DANCE." : "TELLER!", fw); subtitle_draw(); return; }      // (a flash cut covers everything: nothing else is drawn)
    int c[6]; cam_at(ft, c);
    int sway = ft < T(335) || ft > T(425) ? 1 : 2;   // a handheld drift (a little more when it is close in)
    int hb = ft >= T(345) && ft < T(392) ? ABS(fsin(ft * 4)) / 40 : 0;   // the heartbeat in the close-up
    cam.eye.vx = c[0] + fsin(ft / 2) * sway / 90 + hb; cam.eye.vy = -(c[1]) + fcos(ft / 3) * sway / 90 + hb; cam.eye.vz = -c[2];
    cam.at.vx = c[3]; cam.at.vy = -c[4]; cam.at.vz = -c[5];
    gfx_camera_update();
    gfx_set_identity();
    { int dx = c[0] - c[3], dy = c[1] - c[4], dz = c[2] - c[5]; int d2 = dx * dx + dy * dy + dz * dz;   // the ink is heavier in a close-up
      g_wpx = d2 < 600 * 600 ? 22 : d2 < 1200 * 1200 ? 17 : 13; }
    int hero = ft >= FL2 + FL_COVER && ft < T(545);
    g_rim = 1; g_rimc = hero ? BGR(255, 246, 214) : BGR(196, 236, 255);
    floor_draw(ft); PROF(1);
    backdrop_draw(ft); desk_draw(); PROF(2); crowd_draw(ft); PROF(3);
    g_noink = ft > T(556);                                        // (the last pull-back: the people small, a plate over them, no room for ink)
    gfx_set_reserve(ft < T(46) || ft > T(556) ? RESERVE_PLATE : RESERVE_FILM);
    for (int i = 0; i < A_N; i++) actor_draw(i);
    gfx_set_reserve(0); PROF(4);
    gfx_set_identity(); glass_draw(fh);
    // the lens: speed lines on the beats, a flash, the flare, the letterbox sliding in, the fade in and out, then the words
    for (unsigned k = 0; k < sizeof line_w / sizeof line_w[0]; k++) if (ft >= line_w[k].a && ft < line_w[k].b) {
        int seed = (ft / 6) * 7919 + k * 131;                    // (new lines every sixth of a second: on sixes)
        int fade = MIN(MIN(ft - line_w[k].a, line_w[k].b - ft), 6);
        for (int i = 0; i < 26; i++) {
            u32 h = (u32)(seed + i * 2654435) * 2246822519u; h ^= h >> 15; h *= 3266489917u; h ^= h >> 13;
            int a = h & 255, r0 = 70 + ((h >> 8) & 63) + (6 - fade) * 14, r1 = r0 + 50 + ((h >> 16) & 63), tx, ty;
            int ux = fcos(a), uy = fsin(a), lum = 200 + ((h >> 24) & 55);
            for (;;) { tx = 160 + ux * r1 * 4 / 3 / 256; ty = 120 + uy * r1 / 256; if ((tx >= 1 && tx < SCREEN_W - 1 && ty >= 24 && ty < SCREEN_H - 24) || r1 <= r0 + 6) break; r1 -= 8; }
            int sx = 160 + ux * r0 * 4 / 3 / 256, sy = 120 + uy * r0 / 256;
            gfx_line(sx, sy, tx, ty, lum, lum, lum); gfx_line(sx + 1, sy, tx + 1, ty, lum, lum, lum);
        }
    }
    for (unsigned k = 0; k < sizeof beat_t / sizeof beat_t[0]; k++) if (ft >= beat_t[k] && ft < beat_t[k] + 5) {   // a flash cut on the beat
        int f = 150 - (ft - beat_t[k]) * 30; gfx_ui_blend(1); gfx_rect(0, 0, SCREEN_W, SCREEN_H, f, f, f, 1); }
    { int fa = ft - FL1 - FL_COVER, fb = ft - FL2 - FL_COVER, k = fa >= 0 && fa < 4 ? fa : fb >= 0 && fb < 4 ? fb : -1;   // the white clearing after a graphic cut
      if (k >= 0) { int f = 255 - k * 64; gfx_ui_blend(1); gfx_rect(0, 0, SCREEN_W, SCREEN_H, f, f, f, 1); } }
    if (hero) flare_draw(ft - FL2 - FL_COVER);
    int lb = MIN(24, MIN(ft / 2, (FILM_END + T(8) - ft) / 2 + 0)); if (lb < 0) lb = 0;
    gfx_rect(0, 0, SCREEN_W, lb, 0, 0, 0, 0); gfx_rect(0, SCREEN_H - lb, SCREEN_W, lb, 0, 0, 0, 0);
    if (ft >= GLASS_BREAK && ft < GLASS_BREAK + 8) { int f = 255 - (ft - GLASS_BREAK) * 30; gfx_ui_blend(1); gfx_rect(0, 0, SCREEN_W, SCREEN_H, f, f, f, 1); }
    subtitle_draw();
    if (ft > T(2) && ft < T(46)) plate(ft - T(2), T(44) - ft, "TV SHOW", "& TELL", "STARRING MISSY TELLER", 40);          // the title plate over the opening crane
    if (ft > T(560)) plate(ft - T(560), 99999, "MISSY", "TELLER", "HERE TODAY...", 52);                                         // the final card
    int fa = ft < FADE_IN ? 255 - ft * 255 / FADE_IN : ft > FILM_END ? MIN(255, (ft - FILM_END) * 255 / T(6)) : 0;
    if (skipped) fa = MAX(fa, MIN(255, done_t * 14));
    if (fa > 0) { gfx_ui_blend(2); gfx_rect(0, 0, SCREEN_W, SCREEN_H, fa, fa, fa, 1); gfx_ui_blend(1); }
    if (ft > T(30) && ft < T(60) && !skipped) ui_text(250, 8 + 24, 1, "START SKIP", UI_SOFT);
#ifdef FILM_PROF
    PROF(5);
    { extern int gfx_last_wait, gfx_overflow; extern int film_pf[4]; u16 *l = film_log[film_log_n++ % 600]; u8 *q = (u8 *)&l[2];
      l[0] = (u16)ft; l[1] = (u16)(fp[5] - fp[0]); q[0] = (u8)MIN(255, (fp[2] - fp[0]) / 4); q[1] = (u8)MIN(255, (fp[3] - fp[2]) / 4); for (int k = 0; k < 4; k++) q[2 + k] = (u8)MIN(255, film_pf[k] / 4);
      { extern int film_ps[8]; for (int k = 0; k < 5; k++) { film_ps_log[(film_log_n - 1) % 600][k] = (u8)MIN(255, film_ps[k] / 4); film_ps[k] = 0; } }
      l[5] = (u16)(gfx_bytes_used() >> 4 | (gfx_overflow ? 0x8000 : 0)); l[6] = (u16)MIN(65535, gfx_last_wait); film_pf[0] = film_pf[1] = film_pf[2] = film_pf[3] = 0; }
#endif
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, cut_open, cut_update, cut_draw };
