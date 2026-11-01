// skate.c - the skateboard (skate.h). Positions are the sim's own (world units: a tile 256, a person 384 tall, about 4.7 mm a unit);
// speed and height here are kept in 1/16 units (FX) so slow things still move smoothly. 60 steps a second.
#include "skate.h"
#include "world.h"
#include "items.h"
#include "audio.h"
#include "story.h"
#include "wants.h"
#include "ui.h"
#include "gfx.h"
#include "bigmap.h"

#define FX     16
#define GRAV   8                 // gravity: 0.5 units a step a step (about 1.4 g: a game's snappy air)
#define VPUSH  (24 * FX)         // pushing tops out here (about 6.5 m/s)
#define VMAX   (40 * FX)         // and nothing goes faster (rolling down a big quarter pipe)
#define ACCEL  5                 // a push's gain a step
#define OLLIE0 (9 * FX)          // an ollie's pop (a held X adds to it)
enum { ST_GROUND, ST_AIR, ST_GRIND, ST_MANUAL, ST_BAIL };
enum { TK_OLLIE, TK_KICK, TK_HEEL, TK_IMPOSS, TK_SHOVE, TK_NOSEGRAB, TK_TAILGRAB, TK_INDY, TK_MELON, TK_STALE, TK_5050, TK_NOSEG, TK_50, TK_CROOK, TK_BOARDS, TK_MANUAL, TK_SPIN, TK_N };
static const char *const tkNm[TK_N] = { "OLLIE", "KICKFLIP", "HEELFLIP", "IMPOSSIBLE", "POP SHOVE-IT", "NOSEGRAB", "TAILGRAB", "INDY", "MELON", "STALEFISH",
                                        "50-50", "NOSEGRIND", "5-0", "CROOKED", "BOARDSLIDE", "MANUAL", "SPIN" };
static const u16 tkPts[TK_N] = { 25, 100, 100, 150, 100, 150, 150, 125, 125, 200, 100, 150, 150, 175, 175, 50, 0 };

long skate_score, skate_best;
int skate_dbgv[12];
#ifdef AUTO_SKATE
short skate_trace[800]; int skate_ev[512], skate_nev;
#endif
#ifdef AUTO_SKATE
#define EV(a, b, c, d) do { if (skate_nev < 128) { int *e_ = &skate_ev[skate_nev++ * 4]; e_[0] = (a); e_[1] = (b); e_[2] = (c); e_[3] = (d); } } while (0)
#else
#define EV(a, b, c, d) do { } while (0)
#endif   // (test builds) bails, the highest air (units), grind steps, manual steps, tricks banked, flips, the best air time, vert airs
static struct {
    int st, v, dir, fakie, y, vy, cosk;        // state; speed and travel (yaw); riding backwards; height and climb (FX); the slope's cos (/4096)
    int fx, fz;                                // the position's fractions (FX)
    int vh, vert, vdir, lat;                   // in the air: the speed over the ground; a quarter pipe's vert air (no travel uphill) and its way
    int airt, airmax, spin, flip, fliptk, flipn, flipt, grab, grabt;   // air time, a spin (yaw turned), the board's flip (its trick, how many, steps in), a grab
    int held, pr, crouch, charge;              // the buttons; X held down (the ollie's wind-up)
    int bal, balv;                             // the balance of a grind or a manual (-4096..4096)
    int rax, rc, rdir, rtop, rtk, rtx, rty, rcop;    // a grind: along x (1) or y (0), the line across, which way along, the top's height, the trick, the tile
    int bailt, pusht, pushing, carve, landsq, popsq;   // (the look: a push's kick, a carve's lean, a landing's squat, an ollie's spring)
    long pool; int mult, ntr, landt, popt, popok; char popa[40], popb[24];
    u8 tk[4]; u8 tkn[4];                       // the last tricks of the combo (for the line on screen), and how many times each (a double kickflip)
} k;
static int isq(int n) { if (n <= 0) return 0; int x = n, y = (x + 1) >> 1; while (y < x) { x = y; y = (x + n / x) >> 1; } return x; }
static int atan_(int y, int x) {   // the game's yaw from a direction (0 south / -z ... as sim.c)
    if (!x && !y) return 0;
    int ax = ABS(x), ay = ABS(y), a = ax >= ay ? (ay * 512) / (ax + (ay >> 1)) : 1024 - (ax * 512) / (ay + (ax >> 1));
    if (x < 0) a = 2048 - a; if (y < 0) a = -a; return a & 4095;
}
static int yaw_of(int dx, int dz) { return atan_(dx, -dz) & 4095; }
static int adiff(int a, int b) { return ((a - b + 2048) & 4095) - 2048; }   // a - b, -2048..2047
static int rnd(int n) { return (int)(look_rand() % (u32)n); }

void skate_dbg_speed(int v) { k.v = v * FX; }
static int dbg_freeze;
void skate_dbg_pose(int n) {   // (test builds) hold a pose still to look at it: 0 riding, 1 wound up, 2 tucked in the air, 3 indy, 4 nosegrab, 5 manual, 6 grind, 7 a push, 8 a kickflip, 9 a heelflip half way, -1 let go
    dbg_freeze = n >= 0; if (n < 0) return;
    k.st = ST_GROUND; k.crouch = 0; k.charge = 0; k.grab = 0; k.flip = 0; k.bal = 0; k.pusht = 0; k.popsq = 0; k.landsq = 0; k.carve = 0;
    switch (n) {
        case 1: k.crouch = 1; k.charge = 30; break;
        case 2: k.st = ST_AIR; break;
        case 3: k.st = ST_AIR; k.grab = TK_INDY; break;
        case 4: k.st = ST_AIR; k.grab = TK_NOSEGRAB; break;
        case 5: k.st = ST_MANUAL; break;
        case 6: k.st = ST_GRIND; k.rtk = TK_5050; break;
        case 7: k.pusht = 11; break;
        case 8: k.st = ST_AIR; k.flip = 1; k.fliptk = TK_KICK; k.flipn = 1; k.flipt = 5; break;
        case 9: k.st = ST_AIR; k.flip = 1; k.fliptk = TK_IMPOSS; k.flipn = 1; k.flipt = 8; break;
    }
}
void skate_reset(void) { int sc = 0; (void)sc; long s = skate_score, b = skate_best; memset(&k, 0, sizeof k); skate_score = s; skate_best = b; k.cosk = 4096; }
int skate_dir(void) { return k.dir; }
int skate_speed(void) { return k.st == ST_AIR ? k.vh / FX : k.v / FX; }
int skate_air(void) { return on_board && k.st == ST_AIR; }
int skate_bailing(void) { return on_board && k.st == ST_BAIL; }

// ---- the ground under the board: the floor, a ramp's surface, the top of a deck you are up on ----
static int deck_c(char c) { return c == 'X' || c == 'M' || c == 'L' || c == 'N' || c == 'Z'; }
static int ground_at(int x, int z, int y) {   // world units (y: the rider's height now, world units)
    if (x < 0 || z > 0) return 0;
    int mx = x / TILE, my = (-z) / TILE; char c = world_at(mx, my);
    int r = items_ramp_at(mx, my, x - mx * TILE, -z - my * TILE); if (r) return r;
    if (deck_c(c)) { int h = items_height(c); if (y >= h - 24 || h <= 60) return h; }   // (a low one, a manual pad: you roll up onto it)
    return 0;
}
static int grind_c(char c) { return c == '=' || c == 'L' || c == 'N' || c == 'J' || c == 'Z'; }
static int grind_top(int mx, int my, int *coping) {   // what can be ground on this tile: its top's height (0: nothing); a quarter pipe's coping too
    char c = world_at(mx, my); *coping = 0;
    if (grind_c(c)) return items_height(c);
    int d; if (items_ramp_lip(mx, my, &d) == 2) { *coping = 1 + d; return items_ramp_at(mx, my, d == 3 ? 255 : 0, d == 2 ? 255 : 0) + 6; }
    return 0;
}

// ---- the combo ----
static void pop(const char *a, const char *b, int ok) { strncpy(k.popa, a, 39); k.popa[39] = 0; strncpy(k.popb, b ? b : "", 23); k.popb[23] = 0; k.popt = 100; k.popok = ok; }
static void add_trick(int t, int pts) {
    if (k.ntr && k.tk[(k.ntr - 1) & 3] == t && t != TK_SPIN && (t >= TK_KICK && t <= TK_SHOVE)) { k.tkn[(k.ntr - 1) & 3]++; }   // (a double: the same flip again)
    else { k.tk[k.ntr & 3] = (u8)t; k.tkn[k.ntr & 3] = 1; k.ntr++; if (t != TK_OLLIE) k.mult++; }
    k.pool += pts;
}
static void combo_bank(Sim *s) {
    k.landt = 0;
    if (!k.ntr) return;
    long tot = k.pool * MAX(k.mult, 1);
    if (k.mult >= 1 || k.pool > 25) {
        skate_score += tot; if (tot > skate_best) skate_best = tot;
        char a[40], b[24]; sprintf(a, "+%ld", tot); sprintf(b, k.mult > 1 ? "%d TRICK COMBO" : "LANDED", k.mult); pop(a, b, 1);
        skate_dbgv[4] += k.mult;
        if (k.mult >= 1) { for (int i = 1; i <= k.mult; i++) story_event(E_TRICK, i); if (k.airmax >= 50) story_event(E_BIGAIR, 0); }
        if (tot >= 2000) audio_voice(VOICE_YAHOO);
        if (k.mult >= 3) wants_event(s, W_SKATE, 0);
    }
    k.pool = 0; k.mult = 0; k.ntr = 0; k.airmax = 0;
}
static void bail(Sim *s, const char *why) {
    EV(9, why[0], why[1], k.v / FX);
    k.st = ST_BAIL; k.bailt = 80; k.v = 0; k.vy = 0; k.flip = 0; k.grab = 0; k.crouch = 0;
    k.y = ground_at(s->x, s->z, 0) * FX; k.pool = 0; k.mult = 0; k.ntr = 0; k.landt = 0;
    s->gait.sleep = 1; pop("BAIL!", why, 0); audio_voice(VOICE_SHRIEK); skate_dbgv[0]++; skate_dbgv[8] = why[0] + why[1] * 256 + why[5] * 65536;
}

static int on_t;   // (the controls are shown for a few seconds after you step on)
void skate_on(Sim *s) { skate_reset(); on_t = 300; k.dir = s->yaw; k.y = ground_at(s->x, s->z, 0) * FX; board_lift = k.y / FX; }
void skate_off(Sim *s) { combo_bank(s); s->gait.sleep = 0; skate_reset(); on_board = 0; board_lift = 0; }
void skate_input(int held, int pressed) { k.held = held; k.pr |= pressed; }

// ---- moving over the ground: the step, the walls, the slopes (what climbing costs and dropping gives) ----
static int move_h(Sim *s, int dir, int v) {   // v (FX) along dir over the ground: 1 moved, 0 blocked
    k.fx += (isin(dir) * v) >> 12; k.fz -= (icos(dir) * v) >> 12;
    int dx = k.fx / FX, dz = k.fz / FX; k.fx -= dx * FX; k.fz -= dz * FX;
    if (!dx && !dz) return 1;
    board_lift = k.y / FX;
    int x0 = s->x, z0 = s->z; sim_walk(s, dx, dz);
    return s->x != x0 || s->z != z0 || (ABS(dx) + ABS(dz) < 2);
}
static int uphill_of(int r) { static const u16 u[4] = { 2048, 3072, 0, 1024 }; return u[r & 3]; }
static void take_off(Sim *s, int vy, int vh, int lipped) {   // into the air (off a lip, an edge, an ollie)
    EV(1, lipped, vy / FX, vh / FX);
    k.st = ST_AIR; k.vy = vy; k.vh = vh; k.airt = 0; k.spin = 0; k.flip = 0; k.flipn = 0; k.grab = 0; k.vert = 0; k.lat = 0;
    int mx = s->x / TILE, my = (-s->z) / TILE, d, lip = items_ramp_lip(mx, my, &d);
    if (!lip) { int mx2 = (s->x - ((isin(k.dir) * 80) >> 12)) / TILE, my2 = (-(s->z + ((icos(k.dir) * 80) >> 12))) / TILE; lip = items_ramp_lip(mx2, my2, &d); }
    if (lipped) { skate_dbgv[9] = lip * 10 + d; skate_dbgv[10] = vy / FX; skate_dbgv[11] = vh / FX; }
    if (lipped && lip == 2 && vy > 4 * FX) {   // off a quarter pipe: vert air (the rider goes up and comes back down into it; only the sideways part carries)
        int u = uphill_of(d), a = adiff(k.dir, u);
        EV(2, d, adiff(k.dir, uphill_of(d)), s->x);
        if (ABS(a) < 1300) { skate_dbgv[7]++; k.vert = 1; k.vdir = u; k.lat = (vh * isin(a)) >> 12; }
    }
}
static int stick_rel(int want, int m, int base) { return m < 900 ? -9999 : adiff(want, base); }   // the stick against a way (-9999: let go)
static int dir_pick(int rel) { if (rel == -9999) return 0; int a = ABS(rel); return a < 700 ? 1 : a > 1350 ? 2 : rel > 0 ? 3 : 4; }   // 0 none, 1 forward, 2 back, 3 left, 4 right

static void land(Sim *s, int g) {
    int face = (s->yaw + (k.fakie ? 2048 : 0)) & 4095, a = adiff(s->yaw, k.dir);   // the rider's nose against the way it travels
    EV(3, g, k.vy / FX, adiff(s->yaw, k.dir));
    k.y = g * FX;
#ifdef BORE_DC
    { extern void dc_rumble(int, int); int v = ABS(k.vy / FX); if (v > 8) dc_rumble(v > 60 ? 6 : v > 30 ? 4 : 2, 8); }   // (Dreamcast: the Jump Pack feels the landing)
#endif
    if (k.flip && k.flipt < (k.fliptk == TK_IMPOSS ? 22 : 18) * k.flipn - 3) { bail(s, "LAND THE FLIP"); return; }
    if (ABS(a) < 760) k.fakie = 0; else if (ABS(a) > 2048 - 760) k.fakie = 1; else { bail(s, "LAND STRAIGHT"); return; }
    (void)face;
    if (k.airt > k.airmax) k.airmax = k.airt;
    if (k.spin) { int n = (ABS(k.spin) + 512) / 2048; if (n) add_trick(TK_SPIN, n * 100); }
    // the speed: on flat ground what was over the ground carries on; landing on a slope the fall becomes speed down it
    int ahead = ground_at(s->x + ((isin(k.dir) * 40) >> 12), s->z - ((icos(k.dir) * 40) >> 12), g), behind = ground_at(s->x - ((isin(k.dir) * 40) >> 12), s->z + ((icos(k.dir) * 40) >> 12), g);
    int vfall = -k.vy, vh = k.vert ? ABS(k.lat) : k.vh, wasvert = k.vert;
    if (k.vert) { k.dir = (k.dir + 2048) & 4095; k.fakie ^= 1; int t = ahead; ahead = behind; behind = t; }   // back down the quarter pipe the way you went up (riding fakie)
    if (wasvert || behind > ahead + 10) k.v = MIN(VMAX, isq(vh * vh + vfall * vfall) * 15 / 16); else k.v = vh;   // (into a transition: the fall carries on down it)
    k.st = ST_GROUND; k.vy = 0; k.vert = 0; k.grab = 0; k.flip = 0;
    s->yaw = (k.dir + (k.fakie ? 2048 : 0)) & 4095;
    k.landt = 26; k.landsq = 14;   // a moment to keep the combo going (a manual, an ollie, a grind); the legs take the landing
    if (k.airt > 20 && !k.ntr) add_trick(TK_OLLIE, 25);
    s->gait.squash = -10;
}

static int skate_stick_rel;   // (the stick against the way you are going this step: the grinds read it)
static void grind_end(Sim *s) {   // leaving a coping grind: back over the ramp, heading down it
    if (!k.rcop) return; int u = uphill_of(k.rcop - 1);
    s->x -= (isin(u) * 30) >> 12; s->z += (icos(u) * 30) >> 12; k.dir = (u + 2048) & 4095; k.fakie = ABS(adiff(s->yaw, k.dir)) > 1024;
}
static int rail_try(Sim *s, int from_ground) {   // TRIANGLE: a rail, a ledge or the coping under or just ahead of the board, within reach
    int bx = s->x, bz = s->z, cop = 0, best = -1, bmx = 0, bmy = 0, btop = 0;
    for (int i = 0; i < 3 && best < 0; i++) {
        int px = bx + ((isin(k.dir) * (i * 90)) >> 12), pz = bz - ((icos(k.dir) * (i * 90)) >> 12), mx = px / TILE, my = (-pz) / TILE, c;
        int top = grind_top(mx, my, &c); if (!top) continue;
        int y = k.y / FX; if (y < top - (from_ground ? 160 : 70) || y > top + 260) continue;   // (from the ground: an ollie onto it; from the air: close above it)
        best = i; bmx = mx; bmy = my; btop = top; cop = c;
    }
    if (best < 0) return 0;
    int ax;   // which way the rail runs: along its neighbours, else the way you are going
    if (cop) ax = (cop - 1) == 0 || (cop - 1) == 2;   // (the coping runs across the lip)
    else { int c_, nx = grind_top(bmx - 1, bmy, &c_) || grind_top(bmx + 1, bmy, &c_), ny = grind_top(bmx, bmy - 1, &c_) || grind_top(bmx, bmy + 1, &c_);
           ax = nx && !ny ? 1 : ny && !nx ? 0 : ABS(isin(k.dir)) > ABS(icos(k.dir)); }
    k.rax = ax; k.rtx = bmx; k.rty = bmy; k.rtop = btop * FX; k.rcop = cop;
    if (cop) { int d = cop - 1; k.rc = ax ? -(bmy * TILE + (d == 0 ? 0 : TILE)) : bmx * TILE + (d == 3 ? TILE : 0); }   // (the lip's edge)
    else k.rc = ax ? -(bmy * TILE + TILE / 2) : bmx * TILE + TILE / 2;
    int along = ax ? isin(k.dir) : -icos(k.dir); k.rdir = along >= 0 ? 1 : -1;   // (y: south is +y, -z)
    int railyaw = ax ? (k.rdir > 0 ? 1024 : 3072) : (k.rdir > 0 ? 0 : 2048);
    int sp = MAX((k.st == ST_AIR ? k.vh : k.v) * ABS(along) >> 12, 7 * FX);
    int body = adiff(s->yaw, railyaw), side = ABS(body) > 600 && ABS(body) < 1450;   // side on to the rail: a slide
    int pick = dir_pick(skate_stick_rel);
    k.rtk = side ? TK_BOARDS : pick == 1 ? TK_NOSEG : pick == 2 ? TK_50 : pick >= 3 ? TK_CROOK : TK_5050;
    EV(5, bmx, bmy, sp / FX);
    k.st = ST_GRIND; k.v = sp; k.dir = railyaw; k.y = k.rtop; k.vy = 0; k.bal = (rnd(600) - 300); k.balv = 0; k.flip = 0; k.grab = 0;
    if (side) s->yaw = (railyaw + (body > 0 ? 1024 : 3072)) & 4095; else { k.fakie = ABS(body) > 1024; s->yaw = (railyaw + (k.fakie ? 2048 : 0)) & 4095; }
    if (ax) { s->z = k.rc; if (s->x / TILE != bmx) s->x = bmx * TILE + (k.rdir > 0 ? 6 : TILE - 6); }   // (onto it, not short of it)
    else { s->x = k.rc; if ((-s->z) / TILE != bmy) s->z = -(bmy * TILE + (k.rdir > 0 ? 6 : TILE - 6)); }
    add_trick(k.rtk, tkPts[k.rtk]); k.landt = 0;
    audio_voice(VOICE_FINISHED);
    return 1;
}

void skate_step(Sim *s, int wx, int wz) {
    if (dbg_freeze) { s->gait.amp = 300; s->gait.sleep = 0; return; }
    int m = isq(wx * wx + wz * wz); if (m > 4096) m = 4096;
    int want = m > 900 ? yaw_of(wx, wz) : -1;
    int pr = k.pr; k.pr = 0;
    int rel = want >= 0 ? adiff(want, k.dir) : -9999;           // the stick against the way you go
    int relb = want >= 0 ? adiff(want, s->yaw) : -9999;         // ... against the way the rider faces (the flips and grabs)
    skate_stick_rel = rel;
    if (k.popt) k.popt--;
    if (k.landsq) k.landsq--; if (k.popsq) k.popsq--;
    if (k.pushing || (k.pusht % 44)) k.pusht++; else k.pusht = 0;   // a push: the back foot down, a kick, back on the board (it finishes once started)
    s->gait.amp = 300; s->gait.sleep = k.st == ST_BAIL;
    switch (k.st) {
    case ST_BAIL:
        if (--k.bailt <= 0) { k.st = ST_GROUND; s->gait.sleep = 0; k.dir = s->yaw; }
        board_lift = k.y / FX; return;
    case ST_GROUND: case ST_MANUAL: {
        // X: crouch, and pop on letting go (held longer: higher)
        if (k.held & SKB_X) { k.crouch = 1; if (k.charge < 30) k.charge++; }
        else if (k.crouch) { k.crouch = 0; int vy = OLLIE0 + k.charge * FX / 6; k.charge = 0;
            if (k.st == ST_MANUAL) { k.st = ST_GROUND; }
            add_trick(TK_OLLIE, 25); take_off(s, vy, k.v, 0); k.landt = 0; k.popsq = 10; audio_voice(VOICE_YEHA); break; }
        if ((pr & SKB_TRI) && rail_try(s, 1)) break;
        if (pr & SKB_SQ) {   // SQUARE: into a manual (or out of one)
            if (k.st == ST_MANUAL) { k.st = ST_GROUND; k.landt = 22; }
            else if (k.v > 5 * FX) { k.st = ST_MANUAL; k.bal = rnd(400) - 200; k.balv = 0; add_trick(TK_MANUAL, tkPts[TK_MANUAL]); k.landt = 0; }
        }
        if ((pr & SKB_O) && k.st == ST_GROUND) { skate_off(s); if (!sim_ride_after_board()) sim_note("OFF THE BOARD AND ON YOUR FEET"); return; }
        // steering, pushing, braking
        int rate = 84 - k.v / (FX * 1); if (rate < 34) rate = 34; if (k.st == ST_MANUAL) rate /= 2;
        k.pushing = 0;
        if (want >= 0 && ABS(rel) < 1500) { int d = CLAMP(rel, -rate, rate); k.dir = (k.dir + d) & 4095; k.carve += (d * 12 - k.carve) / 6; } else k.carve -= k.carve / 6;   // carving (the stick far behind you does not spin you round)
        if (want >= 0 && ABS(rel) >= 2300 && k.cosk > 3600) k.v -= 12;              // pulling back on the flat: the foot drags
        else if (want >= 0 && ABS(rel) < 1400 && k.st == ST_GROUND && k.v < VPUSH && k.cosk > 3600) { k.v += ACCEL * m >> 12; k.pushing = 1; }   // pushing (not on a steep slope)
        k.v -= 1 + k.v / 400;                                                       // rolling friction
        if (k.v < 0) k.v = 0;
        if (k.st == ST_MANUAL) {   // the balance: it tips more the further it is gone; the stick (forward, back) brings it round
            int along = rel == -9999 ? 0 : (m * icos(rel)) >> 12;
            k.balv = k.balv * 15 / 16 + k.bal / 300 + rnd(7) - 3 - along / 80; k.balv = CLAMP(k.balv, -200, 200); k.bal += k.balv;
            k.pool += 1; skate_dbgv[3]++;
            if (ABS(k.bal) > 4096) { bail(s, "BALANCE"); break; }
            if (k.v < 2 * FX) { k.st = ST_GROUND; k.landt = 10; }
        }
        // the move, and the ground it brings you to
        int y0 = k.y, px0 = s->x, pz0 = s->z, ok = move_h(s, k.dir, (k.v * k.cosk) >> 12);
        if (!ok) {
            EV(4, s->x, s->z, k.v / FX);
            if ((k.held & SKB_TRI) && rail_try(s, 1)) break;
            int fx_ = s->x + ((isin(k.dir) * 120) >> 12), fz_ = s->z - ((icos(k.dir) * 120) >> 12), fm = fx_ / TILE, fn = (-fz_) / TILE;
            char fc = world_at(fm, fn); int tall = !items_is_item(fc) || items_height(fc) > 150;   // (a wall, or something big)
            if (tall && k.v > 18 * FX) { bail(s, "SLAMMED"); break; }
            k.v = k.v / 4; if (!tall) { k.dir = (k.dir + 2048) & 4095; k.fakie ^= 1; k.v = MIN(k.v, 3 * FX); }   // (a bump off it, rolling back)
        }
        int g = ground_at(s->x, s->z, y0 / FX) * FX;
        int run_ = (k.v * k.cosk) >> 12;
        if (g < y0 - 8 * FX - 4 * run_) {   // off a lip, or off an edge (more of a drop than the slope down gives): into the air along the slope
            int vy = 0, vh = k.v;
            {   // the slope you leave from (measured just behind, on the ramp: the lip's own angle)
                int bx = px0 - ((isin(k.dir) * 24) >> 12), bz = pz0 + ((icos(k.dir) * 24) >> 12);
                int rise = ground_at(px0, pz0, y0 / FX) - ground_at(bx, bz, y0 / FX);   // over 24 units
                if (rise > 0) { int l = isq(24 * 24 + rise * rise); vy = k.v * rise / l; vh = k.v * 24 / l; }
            }
            if (k.st == ST_MANUAL) k.st = ST_GROUND;
            take_off(s, vy, vh, 1);
            if (k.vert) { s->x = px0 - ((isin(k.vdir) * 10) >> 12); s->z = pz0 + ((icos(k.vdir) * 10) >> 12); }   // (vert: back over the ramp, to come down into it)
            k.landt = 0; break;
        }
        int dh = g - y0;
        { long v2 = (long)k.v * k.v - 2L * GRAV * dh; if (v2 <= 0) { if (dh > FX) { EV(6, s->x, k.y / FX, 0); k.dir = (k.dir + 2048) & 4095; k.fakie ^= 1; k.v = FX * 2; } else k.v = 0; } else k.v = MIN(VMAX, isq((int)MIN(v2, 0x3FFFFFFFL))); }   // climbing costs speed, dropping gives it
        { int run = MAX((k.v * k.cosk) >> 12, FX); int l = isq(run / 4 * (run / 4) + dh / 4 * (dh / 4)); k.cosk = l ? CLAMP(((run / 4) * 4096) / l, 900, 4096) : 4096; }
        k.vy = dh;   // (the climb rate: what a lip throws you up with)
        k.y = g;
        if (k.landt && --k.landt == 0 && k.st == ST_GROUND) combo_bank(s);
        if (!k.landt && k.ntr && k.st == ST_GROUND && !(k.held & SKB_X)) combo_bank(s);
        s->yaw = (k.dir + (k.fakie ? 2048 : 0)) & 4095;
        break; }
    case ST_AIR: {
        k.airt++;
        k.y += k.vy; k.vy -= GRAV; if (k.y / FX > skate_dbgv[1]) skate_dbgv[1] = k.y / FX; if (k.airt > skate_dbgv[6]) skate_dbgv[6] = k.airt;
        if (k.vert) move_h(s, (k.vdir + 1024) & 4095, k.lat);   // vert: only along the coping
        else if (!move_h(s, k.dir, k.vh)) k.vh = 0;              // (a wall in the air: you drop down it)
        if (k.held & SKB_L1) { s->yaw = (s->yaw + 78) & 4095; k.spin += 78; }   // spinning
        if (k.held & SKB_R1) { s->yaw = (s->yaw - 78) & 4095; k.spin -= 78; }
        if (pr & SKB_SQ) {   // a flip: which one by the stick (again during one: a double)
            int p = dir_pick(relb), t = p == 1 ? TK_IMPOSS : p == 2 ? TK_SHOVE : p == 4 ? TK_HEEL : TK_KICK;
            if (k.flip && k.fliptk == t && k.flipn < 3) { k.flipn++; add_trick(t, tkPts[t] * k.flipn); }
            else if (!k.flip) { skate_dbgv[5]++; k.flip = 1; k.fliptk = t; k.flipn = 1; k.flipt = 0; add_trick(t, tkPts[t]); }
        }
        if (k.flip) { k.flipt++; int len = (k.fliptk == TK_IMPOSS ? 22 : 18) * k.flipn; if (k.flipt >= len) k.flip = 0; }
        if ((pr & SKB_O) && !k.grab && !k.flip) {   // a grab: held, it scores more
            int p = dir_pick(relb), t = p == 1 ? TK_NOSEGRAB : p == 2 ? TK_TAILGRAB : p == 3 ? TK_INDY : p == 4 ? TK_MELON : TK_STALE;
            k.grab = t; k.grabt = 0; add_trick(t, tkPts[t]);
        }
        if (k.grab) { if (k.held & SKB_O) { k.grabt++; if (k.grabt > 10) k.pool += 3; } else k.grab = 0; }
        if ((pr & SKB_TRI) || ((k.held & SKB_TRI) && k.vy < 0)) { if (rail_try(s, 0)) break; }
        int g = ground_at(s->x, s->z, k.y / FX + 30);
        if (k.vy < 0 && k.y <= g * FX) land(s, g);
        if (k.y < -100 * FX) k.y = 0;
        break; }
    case ST_GRIND: {
        int along = k.rdir * k.v;   // moving along the rail
        if (k.rax) { k.fx += along; int d = k.fx / FX; k.fx -= d * FX; s->x += d; s->z = k.rc; }
        else { k.fz -= along; int d = k.fz / FX; k.fz -= d * FX; s->z += d; s->x = k.rc; }
        k.v -= 1; if (k.v < 6 * FX) k.v = 6 * FX;
        k.pool += k.rtk == TK_BOARDS ? 2 : 1; skate_dbgv[2]++;
        {   // the balance: the stick across the rail holds it
            int lat = 0; if (want >= 0) { int a = adiff(want, k.dir); lat = (m * isin(a)) >> 12; }
            k.balv = k.balv * 15 / 16 + k.bal / 320 + rnd(7) - 3 - lat / 80; k.balv = CLAMP(k.balv, -200, 200); k.bal += k.balv;
            if (ABS(k.bal) > 4096) { bail(s, "BALANCE"); break; }
        }
        int mx = s->x / TILE, my = (-s->z) / TILE, c_;
        int off = k.rax ? (mx != k.rtx) : (my != k.rty);
        if (off) { if (grind_top(mx, my, &c_) * FX == k.rtop) { k.rtx = mx; k.rty = my; off = 0; } }   // on along the next piece of it
        if (k.held & SKB_X) k.crouch = 1;
        else if (k.crouch) { k.crouch = 0; add_trick(TK_OLLIE, 25); grind_end(s); take_off(s, OLLIE0, k.v, 0); break; }   // ollie out of it
        if (off) { grind_end(s); take_off(s, 3 * FX, k.v, 0); }   // off the end
        board_lift = k.y / FX;
        break; }
    }
    if (k.st != ST_GRIND) board_lift = k.y / FX;
#ifdef AUTO_SKATE
    { static int c_; if (++c_ >= 40) { c_ = 0; EV(7, (world_wbx * BM_BLK + s->x / TILE) % BM_BLK - BM_LOT0, (world_wby * BM_BLK + (-s->z) / TILE) % BM_BLK - BM_LOT0, k.st * 1000 + k.v / FX); } }
    { extern int tst_trace; static int n; if (tst_trace && n < 200) { short *t = &skate_trace[n++ * 4]; t[0] = (short)s->x; t[1] = (short)(k.y / FX); t[2] = (short)k.v; t[3] = (short)(k.st * 1000 + k.vy / FX); } }
#endif
}

// ---- how it looks ----
int skate_body_yaw(int yaw) { return (yaw + 1024) & 4095; }   // side on: the chest to the right of the nose
void skate_pose(Gait *g) {
    g->skate = k.st == ST_BAIL ? 0 : 1;
    int cr = 900;                                                                   // riding: knees a little bent
    if (k.crouch) cr = 2600 + MIN(k.charge, 30) * 50;                              // winding up an ollie
    else if (k.popsq) cr = 0;                                                       // the pop: legs straight
    else if (k.landsq) cr = 900 + k.landsq * 180;                                  // a landing taken in the knees
    else if (k.st == ST_GRIND) cr = 1900;
    else if (k.st == ST_MANUAL) cr = 1400;
    else if (k.st == ST_AIR) cr = (k.flip || k.grab) ? 3400 : 2000;                // tucked in the air, more for a trick
    g->crouch = CLAMP(cr, 0, 4096);
    g->grab = k.grab ? (k.grab == TK_NOSEGRAB ? 2 : k.grab == TK_TAILGRAB ? 3 : 1) : 0;
    g->roll = (k.st == ST_MANUAL) ? -k.bal / 6 - 260 : 0;                         // a manual: leaning back over the tail, wobbling with the balance
    g->pitch = (k.st == ST_GRIND) ? k.bal / 7 : (k.st == ST_GROUND ? CLAMP(k.carve, -360, 360) : 0);   // a grind's balance; leaning into a carve
    { int ph = k.pusht % 44; g->push = ph ? isin(ph * 4096 / 44 / 2) : 0; }       // the push: the back leg swings out and back (half a sine)
    g->spread = k.st == ST_AIR && k.flip ? 1 : 0;                                   // the feet off the board for a flip
}
int skate_board_up(void) { return k.st == ST_AIR && k.grab ? 34 : 0; }   // a grab pulls the board up to the hand
void skate_board(int *roll, int *pitch, int *spin) {
    *roll = *pitch = *spin = 0;
    if (k.st == ST_MANUAL) *pitch = -300 - k.bal / 16;                  // the nose up
    if (k.st == ST_GRIND && k.rtk == TK_NOSEG) *pitch = 220; else if (k.st == ST_GRIND && k.rtk == TK_50) *pitch = -220;
    if (!k.flip) return;
    int len = (k.fliptk == TK_IMPOSS ? 22 : 18) * k.flipn, a = (k.flipt * 4096 * k.flipn) / MAX(len, 1);
    switch (k.fliptk) { case TK_KICK: *roll = a; break; case TK_HEEL: *roll = -a; break; case TK_IMPOSS: *pitch = a; break; case TK_SHOVE: *spin = a / 2; break; }
}
const char *skate_hint(void) { return k.st == ST_GRIND ? "OLLIE OFF" : k.st == ST_AIR ? "" : "OLLIE"; }
static void tk_name(int i, char *out) {
    int t = k.tk[i & 3], n = k.tkn[i & 3];
    if (t == TK_SPIN) { strcpy(out, "SPIN"); return; }
    sprintf(out, "%s%s", n == 2 ? "DOUBLE " : n >= 3 ? "TRIPLE " : "", tkNm[t]);
}
void skate_hud(void) {
    if (!on_board) return;
    char b[64];
    if (on_t > 0) { on_t--; ui_hintbar(150, k.st == ST_AIR ? "SQ FLIP|O GRAB|L1 R1 SPIN|TRI GRIND" : "X OLLIE|SQ MANUAL|TRI GRIND|O OFF"); }
    if (k.ntr) {   // the combo so far: the last tricks, and the points times the tricks
        int n = MIN(k.ntr, 3); b[0] = 0;
        for (int i = k.ntr - n; i < k.ntr; i++) { char t[24]; tk_name(i, t); if (b[0]) strcat(b, " + "); strcat(b, t); }
        int w = MAX(ui_tw(1, b), 60) + 16; ui_card(160 - w / 2, 62, w, 30);
        ui_textc(160, 65, 1, b, UI_INK);
        sprintf(b, "%ld X %d", k.pool, MAX(k.mult, 1)); ui_textc(160, 76, 2, b, UI_BLUE2);
    }
    if (k.st == ST_GRIND || k.st == ST_MANUAL) {   // the balance meter
        int x = 110, y = 96, w = 100, p = CLAMP(k.bal, -4096, 4096) * (w / 2) / 4096;
        ui_round(x - 2, y - 2, w + 4, 10, 4, UI_WHITE, UI_CARD2, 0);
        gfx_rect(x + w / 2 - 12, y, 24, 6, 120, 210, 110, 0); gfx_rect(x, y, 14, 6, 230, 90, 70, 0); gfx_rect(x + w - 14, y, 14, 6, 230, 90, 70, 0);
        gfx_rect(x + w / 2 + p - 2, y - 2, 4, 10, 30, 40, 60, 0);
    }
    if (k.popt) {   // what just happened: points banked, or a bail
        int fall = k.popt > 80 ? (k.popt - 80) : 0, w = ui_tw(2, k.popa) + 24;
        ui_card(160 - w / 2, 104 - fall, w, k.popb[0] ? 30 : 18);
        ui_textc(160, 107 - fall, 2, k.popa, k.popok ? UI_GOODT : UI_BADT);
        if (k.popb[0]) ui_textc(160, 122 - fall, 1, k.popb, UI_SOFT);
    }
    sprintf(b, "SCORE %ld", skate_score); { int w = ui_tw(1, b) + 14; ui_card(316 - w, 162, w, 14); ui_text(323 - w, 165, 1, b, UI_INK); }
}
