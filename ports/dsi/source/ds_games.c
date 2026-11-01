// ds_games.c - the PS1's mini-games on the DS (included by main.c, after the lot, the sims and the 2D helpers it uses).
// D44: SKATE (src/skate.c): the board in Live, the PS1's physics, tricks, combos and score.

// ---- small maths in the PS1's units (angles 0..4095 a turn, sines x4096) ----
static inline int isin(int a) { return sinLerp((s16)((a & 4095) << 3)); }
static inline int icos(int a) { return cosLerp((s16)((a & 4095) << 3)); }
static int isq(int n) { if (n <= 0) return 0; int x = n, y = (x + 1) >> 1; while (y < x) { x = y; y = (x + n / x) >> 1; } return x; }
#define ABS(v) ((v) < 0 ? -(v) : (v))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(v, a, b) ((v) < (a) ? (a) : (v) > (b) ? (b) : (v))
static u32 g_rng = 0x2545F491u;
static int g_rnd(int n) { g_rng = g_rng * 1103515245u + 12345u; return (int)((g_rng >> 16) % (u32)n); }
// the PS1's voices as the DS's little sounds
enum { V_YEHA = 6, V_YAHOO = 5, V_NAH = 2, V_GROAN = 2, V_SHRIEK = 2, V_LAUGH = 3, V_FIN = 1, V_AGREE = 0, V_CRY = 2, V_WIN = 5 };
static void voice(int v) { sfx(v); }

// ==== SKATE ====
// The rider moves in the PS1's world units (a tile 256: one DS cell; x east, z = -south, as skate.c), 60 steps a second; the DS sim follows it.
#define FX     16
#define GRAV   8
#define VPUSH  (24 * FX)
#define VMAX   (40 * FX)
#define ACCEL  5
#define OLLIE0 (9 * FX)
enum { KS_GROUND, KS_AIR, KS_GRIND, KS_MANUAL, KS_BAIL };
enum { TK_OLLIE, TK_KICK, TK_HEEL, TK_IMPOSS, TK_SHOVE, TK_NOSEGRAB, TK_TAILGRAB, TK_INDY, TK_MELON, TK_STALE, TK_5050, TK_NOSEG, TK_50, TK_CROOK, TK_BOARDS, TK_MANUAL, TK_SPIN, TK_N };
static const char *const tkNm[TK_N] = { "OLLIE", "KICKFLIP", "HEELFLIP", "IMPOSSIBLE", "POP SHOVE-IT", "NOSEGRAB", "TAILGRAB", "INDY", "MELON", "STALEFISH",
                                        "50-50", "NOSEGRIND", "5-0", "CROOKED", "BOARDSLIDE", "MANUAL", "SPIN" };
static const u16 tkPts[TK_N] = { 25, 100, 100, 150, 100, 150, 150, 125, 125, 200, 100, 150, 150, 175, 175, 50, 0 };
enum { SKB_X = 1, SKB_SQ = 2, SKB_O = 4, SKB_TRI = 8, SKB_L1 = 16, SKB_R1 = 32 };
static struct {
    int st, v, dir, fakie, y, vy, cosk, fx, fz, vh, vert, vdir, lat, airt, airmax, spin, flip, fliptk, flipn, flipt, grab, grabt;
    int held, pr, crouch, charge, bal, balv, rax, rc, rdir, rtop, rtk, rtx, rty, rcop, bailt, pusht, pushing, carve, landsq, popsq;
    long pool; int mult, ntr, landt, popt, popok; char popa[40], popb[24]; u8 tk[4], tkn[4];
} k;
static struct { int x, z, yaw; } skr;    // the rider (PS1 units)
static int sk_lx, sk_ly, sk_on_t, sk_tricks;   // (the DS sim's last position from the rider; the controls card's time; tricks landed)
static void sk_pop(const char *a, const char *b, int ok) { strncpy(k.popa, a, 39); k.popa[39] = 0; strncpy(k.popb, b ? b : "", 23); k.popb[23] = 0; k.popt = 100; k.popok = ok; }
static int sk_yaw_of(int x, int y) {   // the PS1's atan_ (0 south, 1024 east)
    if (!x && !y) return 0;
    int ax = ABS(x), ay = ABS(y), a = ax >= ay ? (ay * 512) / (ax + (ay >> 1)) : 1024 - (ax * 512) / (ay + (ax >> 1));
    if (x < 0) a = 2048 - a; if (y < 0) a = -a; return a & 4095;
}
static int yaw_of(int dx, int dz) { return sk_yaw_of(dx, -dz) & 4095; }
static int adiff(int a, int b) { return ((a - b + 2048) & 4095) - 2048; }
// ---- what is on a cell, as the PS1's world letters ----
static int sk_cell(int mx, int my) { LV = 0; int o = obj_at(mx, my); return o; }
static int sk_ch(int mx, int my) { int o = sk_cell(mx, my); return o < 0 ? '.' : ID2CH[OBJ_ID(&objs[o])] ? ID2CH[OBJ_ID(&objs[o])] : '#'; }
static int sk_face(int mx, int my) { int o = sk_cell(mx, my); return o < 0 ? 0 : objs[o].rot & 3; }
static int sk_ih(int c) {   // items_height: the top of its model (PS1 units)
    int id = PS1ID[c & 255] - 1; if (id < 0) return 0; const Item *it = &ITEMS[id]; int top = 0;
    for (int q = 0; q < it->nbox; q++) if (IBOX[it->box0 + q].z1 > top) top = IBOX[it->box0 + q].z1;
    return top ? top * 24 : it->h * 40; }
static int is_ramp_c(int c) { return c == '1' || c == '5' || c == '9'; }
static int ramp_kind(int c) { return c == '5' ? 2 : c == '9' ? 1 : 0; }   // 0 kicker, 1 launch, 2 quarter pipe
static const s8 lipDx[4] = { 0, -1, 0, 1 }, lipDy[4] = { -1, 0, 1, 0 };
static int same_ramp(int x, int y, int kind, int r) { int d = sk_ch(x, y); return is_ramp_c(d) && ramp_kind(d) == kind && sk_face(x, y) == r; }
static void ramp_run(int mx, int my, int kind, int r, int *idx, int *n) { int a = 0, b = 0;
    while (a < 2 && same_ramp(mx + lipDx[r] * (a + 1), my + lipDy[r] * (a + 1), kind, r)) a++;
    while (a + b < 2 && same_ramp(mx - lipDx[r] * (b + 1), my - lipDy[r] * (b + 1), kind, r)) b++;
    *idx = a; *n = a + b + 1; }
static int ramp_prof(int kind, int n, int d) { int L = n * 64; d = CLAMP(d, 0, L); int t = d * 4096 / L, t2 = (t * t) >> 12;
    if (kind == 0) { int HT = n == 1 ? 110 : n == 2 ? 170 : 230; return (HT * (((t * 1434) >> 12) + ((t2 * 2662) >> 12))) >> 12; }
    if (kind == 1) { int HT = MIN(150 * n, 420); return (HT * t2) >> 12; }
    { int R = n * 256, x = (d * 4 * 972) >> 10; return R - isq(R * R - x * x); } }
static int ramp_localB(int r, int lx, int lv) { switch (r & 3) { case 0: return lv >> 2; case 1: return lx >> 2; case 2: return 64 - (lv >> 2); default: return 64 - (lx >> 2); } }
static int ramp_at(int mx, int my, int lx, int lv) { int c = sk_ch(mx, my); if (!is_ramp_c(c)) return 0;
    int kind = ramp_kind(c), r = sk_face(mx, my), idx, n; ramp_run(mx, my, kind, r, &idx, &n);
    int B = CLAMP(ramp_localB(r, CLAMP(lx, 0, 255), CLAMP(lv, 0, 255)), 0, 64); return ramp_prof(kind, n, n * 64 - (idx * 64 + B)); }
static int ramp_lip(int mx, int my, int *dir) { int c = sk_ch(mx, my); if (!is_ramp_c(c)) return 0;
    int kind = ramp_kind(c), r = sk_face(mx, my), idx, n; ramp_run(mx, my, kind, r, &idx, &n); *dir = r; return idx == 0 ? (kind == 2 ? 2 : 1) : 0; }
static int deck_c(int c) { return c == 'X' || c == 'M' || c == 'L' || c == 'N' || c == 'Z'; }
static int ground_at(int x, int z, int y) {   // the ground under the board (PS1 units)
    int mx = fdiv(x, 256), my = fdiv(-z, 256); if (mx < 0 || my < 0 || mx >= GN || my >= GN) return 0;
    int c = sk_ch(mx, my), r = ramp_at(mx, my, x - mx * 256, -z - my * 256); if (r) return r;
    if (deck_c(c)) { int h = sk_ih(c); if (y >= h - 24 || h <= 60) return h; }
    return 0; }
static int grind_c(int c) { return c == '=' || c == 'L' || c == 'N' || c == 'J' || c == 'Z'; }
static int grind_top(int mx, int my, int *coping) { if (mx < 0 || my < 0 || mx >= GN || my >= GN) { *coping = 0; return 0; }
    int c = sk_ch(mx, my); *coping = 0; if (grind_c(c)) return sk_ih(c);
    int d; if (ramp_lip(mx, my, &d) == 2) { *coping = 1 + d; return ramp_at(mx, my, d == 3 ? 255 : 0, d == 2 ? 255 : 0) + 6; }
    return 0; }
static int sk_ride_c(int c) { return is_ramp_c(c) || deck_c(c) || c == '.'; }
static void sk_walk(int dx, int dz) {   // sim_walk for the board: walls and solid things stop it, a ramp or a deck you can ride onto does not (nor anything not much higher than the board)
    int y = k.y / FX;
    for (int ax = 0; ax < 2; ax++) { int nx = skr.x + (ax == 0 ? dx : 0), nz = skr.z + (ax == 1 ? dz : 0);
        int cx0 = fdiv(skr.x, 256), cy0 = fdiv(-skr.z, 256), cx1 = fdiv(nx, 256), cy1 = fdiv(-nz, 256);
        if (cx1 != cx0 || cy1 != cy0) {
            if (cx1 < -SLC || cy1 < -SLC || cx1 >= SLC * 2 || cy1 >= SLC * 2) continue;
            if (!edge_open(cx0, cy0, cx1, cy1)) continue;
            int o = sk_cell(cx1, cy1); LV = 0;
            if (o >= 0 || obj_solid(cx1, cy1)) { int c = o >= 0 ? sk_ch(cx1, cy1) : '#'; if (!sk_ride_c(c) && (o < 0 || sk_ih(c) > y + 30)) continue; }
            if (ground_at(nx, nz, y) > y + 40) continue; }
        skr.x = nx; skr.z = nz; }
}
// ---- the combo ----
static void add_trick(int t, int pts) {
    if (k.ntr && k.tk[(k.ntr - 1) & 3] == t && t != TK_SPIN && (t >= TK_KICK && t <= TK_SHOVE)) k.tkn[(k.ntr - 1) & 3]++;
    else { k.tk[k.ntr & 3] = (u8)t; k.tkn[k.ntr & 3] = 1; k.ntr++; if (t != TK_OLLIE) k.mult++; }
    k.pool += pts; }
static void combo_bank(void) {
    k.landt = 0; if (!k.ntr) return;
    long tot = k.pool * MAX(k.mult, 1);
    if (k.mult >= 1 || k.pool > 25) {
        skate_score += tot; if (tot > skate_best) skate_best = tot;
        char a[40], b[24]; sprintf(a, "+%ld", tot); sprintf(b, k.mult > 1 ? "%d TRICK COMBO" : "LANDED", k.mult); sk_pop(a, b, 1);
        sk_tricks += k.mult;
        if (tot >= 2000) voice(V_YAHOO);
        if (k.mult >= 3) life_ev(ctl, WN_FUN, 0);   // (the PS1: a skate want; the DS's nearest is a fun one)
    }
    k.pool = 0; k.mult = 0; k.ntr = 0; k.airmax = 0; }
static void bail(const char *why) {
    k.st = KS_BAIL; k.bailt = 80; k.v = 0; k.vy = 0; k.flip = 0; k.grab = 0; k.crouch = 0;
    k.y = ground_at(skr.x, skr.z, 0) * FX; k.pool = 0; k.mult = 0; k.ntr = 0; k.landt = 0;
    sk_pop("BAIL!", why, 0); voice(V_SHRIEK); }
static void skate_reset(void) { memset(&k, 0, sizeof k); k.cosk = 4096; }
static void skate_sync_in(void) { const Sim *m = &sims[ctl]; skr.x = m->x * 16 + 128; skr.z = -(m->y * 16 + 128); sk_lx = m->x; sk_ly = m->y; }
static void skate_on(void) {
    Sim *m = &sims[ctl]; if (slv[ctl]) { say("Ride it downstairs"); return; }
    skate_reset(); skate_sync_in(); sk_on_t = 300; skr.yaw = (fq4[ctl] == 0 ? 0 : fq4[ctl] == 1 ? 1024 : fq4[ctl] == 2 ? 2048 : 3072); k.dir = skr.yaw; k.y = ground_at(skr.x, skr.z, 0) * FX; board_lift = k.y / FX;
    on_board = 1; m->act = -1; m->act_left = 0; m->tx = m->x; m->ty = m->y; say("On the board"); life_ev(ctl, WN_FUN, 0); }
static void skate_off(void) { combo_bank(); skate_reset(); on_board = 0; board_lift = 0; Sim *m = &sims[ctl]; m->x = m->tx = m->wx = fdiv(m->x + 8, 16) * 16; m->y = m->ty = m->wy = fdiv(m->y + 8, 16) * 16; }
static int uphill_of(int r) { static const u16 u[4] = { 2048, 3072, 0, 1024 }; return u[r & 3]; }
static int move_h(int dir, int v) {
    k.fx += (isin(dir) * v) >> 12; k.fz -= (icos(dir) * v) >> 12;
    int dx = k.fx / FX, dz = k.fz / FX; k.fx -= dx * FX; k.fz -= dz * FX;
    if (!dx && !dz) return 1;
    board_lift = k.y / FX;
    int x0 = skr.x, z0 = skr.z; sk_walk(dx, dz);
    return skr.x != x0 || skr.z != z0 || (ABS(dx) + ABS(dz) < 2); }
static void take_off(int vy, int vh, int lipped) {
    k.st = KS_AIR; k.vy = vy; k.vh = vh; k.airt = 0; k.spin = 0; k.flip = 0; k.flipn = 0; k.grab = 0; k.vert = 0; k.lat = 0;
    int mx = fdiv(skr.x, 256), my = fdiv(-skr.z, 256), d, lip = ramp_lip(mx, my, &d);
    if (!lip) { int mx2 = fdiv(skr.x - ((isin(k.dir) * 80) >> 12), 256), my2 = fdiv(-(skr.z + ((icos(k.dir) * 80) >> 12)), 256); lip = ramp_lip(mx2, my2, &d); }
    if (lipped && lip == 2 && vy > 4 * FX) { int u = uphill_of(d), a = adiff(k.dir, u); if (ABS(a) < 1300) { k.vert = 1; k.vdir = u; k.lat = (vh * isin(a)) >> 12; } }
}
static int dir_pick(int rel) { if (rel == -9999) return 0; int a = ABS(rel); return a < 700 ? 1 : a > 1350 ? 2 : rel > 0 ? 3 : 4; }
static void sk_land(int g) {
    int a = adiff(skr.yaw, k.dir);
    k.y = g * FX;
    if (k.flip && k.flipt < (k.fliptk == TK_IMPOSS ? 22 : 18) * k.flipn - 3) { bail("LAND THE FLIP"); return; }
    if (ABS(a) < 760) k.fakie = 0; else if (ABS(a) > 2048 - 760) k.fakie = 1; else { bail("LAND STRAIGHT"); return; }
    if (k.airt > k.airmax) k.airmax = k.airt;
    if (k.spin) { int n = (ABS(k.spin) + 512) / 2048; if (n) add_trick(TK_SPIN, n * 100); }
    int ahead = ground_at(skr.x + ((isin(k.dir) * 40) >> 12), skr.z - ((icos(k.dir) * 40) >> 12), g), behind = ground_at(skr.x - ((isin(k.dir) * 40) >> 12), skr.z + ((icos(k.dir) * 40) >> 12), g);
    int vfall = -k.vy, vh = k.vert ? ABS(k.lat) : k.vh, wasvert = k.vert;
    if (k.vert) { k.dir = (k.dir + 2048) & 4095; k.fakie ^= 1; int t = ahead; ahead = behind; behind = t; }
    if (wasvert || behind > ahead + 10) k.v = MIN(VMAX, isq(vh * vh + vfall * vfall) * 15 / 16); else k.v = vh;
    k.st = KS_GROUND; k.vy = 0; k.vert = 0; k.grab = 0; k.flip = 0;
    skr.yaw = (k.dir + (k.fakie ? 2048 : 0)) & 4095;
    k.landt = 26; k.landsq = 14;
    if (k.airt > 20 && !k.ntr) add_trick(TK_OLLIE, 25);
}
static int sk_stick_rel;
static void grind_end(void) { if (!k.rcop) return; int u = uphill_of(k.rcop - 1); skr.x -= (isin(u) * 30) >> 12; skr.z += (icos(u) * 30) >> 12; k.dir = (u + 2048) & 4095; k.fakie = ABS(adiff(skr.yaw, k.dir)) > 1024; }
static int rail_try(int from_ground) {
    int bx = skr.x, bz = skr.z, cop = 0, best = -1, bmx = 0, bmy = 0, btop = 0;
    for (int i = 0; i < 3 && best < 0; i++) {
        int px = bx + ((isin(k.dir) * (i * 90)) >> 12), pz = bz - ((icos(k.dir) * (i * 90)) >> 12), mx = fdiv(px, 256), my = fdiv(-pz, 256), c;
        int top = grind_top(mx, my, &c); if (!top) continue;
        int y = k.y / FX; if (y < top - (from_ground ? 160 : 70) || y > top + 260) continue;
        best = i; bmx = mx; bmy = my; btop = top; cop = c; }
    if (best < 0) return 0;
    int ax;
    if (cop) ax = (cop - 1) == 0 || (cop - 1) == 2;
    else { int c_, nx = grind_top(bmx - 1, bmy, &c_) || grind_top(bmx + 1, bmy, &c_), ny = grind_top(bmx, bmy - 1, &c_) || grind_top(bmx, bmy + 1, &c_);
           ax = nx && !ny ? 1 : ny && !nx ? 0 : ABS(isin(k.dir)) > ABS(icos(k.dir)); }
    k.rax = ax; k.rtx = bmx; k.rty = bmy; k.rtop = btop * FX; k.rcop = cop;
    if (cop) { int d = cop - 1; k.rc = ax ? -(bmy * 256 + (d == 0 ? 0 : 256)) : bmx * 256 + (d == 3 ? 256 : 0); }
    else k.rc = ax ? -(bmy * 256 + 128) : bmx * 256 + 128;
    int along = ax ? isin(k.dir) : -icos(k.dir); k.rdir = along >= 0 ? 1 : -1;
    int railyaw = ax ? (k.rdir > 0 ? 1024 : 3072) : (k.rdir > 0 ? 0 : 2048);
    int sp = MAX((k.st == KS_AIR ? k.vh : k.v) * ABS(along) >> 12, 7 * FX);
    int body = adiff(skr.yaw, railyaw), side = ABS(body) > 600 && ABS(body) < 1450;
    int pick = dir_pick(sk_stick_rel);
    k.rtk = side ? TK_BOARDS : pick == 1 ? TK_NOSEG : pick == 2 ? TK_50 : pick >= 3 ? TK_CROOK : TK_5050;
    k.st = KS_GRIND; k.v = sp; k.dir = railyaw; k.y = k.rtop; k.vy = 0; k.bal = (g_rnd(600) - 300); k.balv = 0; k.flip = 0; k.grab = 0;
    if (side) skr.yaw = (railyaw + (body > 0 ? 1024 : 3072)) & 4095; else { k.fakie = ABS(body) > 1024; skr.yaw = (railyaw + (k.fakie ? 2048 : 0)) & 4095; }
    if (ax) { skr.z = k.rc; if (fdiv(skr.x, 256) != bmx) skr.x = bmx * 256 + (k.rdir > 0 ? 6 : 250); }
    else { skr.x = k.rc; if (fdiv(-skr.z, 256) != bmy) skr.z = -(bmy * 256 + (k.rdir > 0 ? 6 : 250)); }
    add_trick(k.rtk, tkPts[k.rtk]); k.landt = 0; voice(V_FIN);
    return 1; }
static void skate_step(int wx, int wz) {   // one 60 Hz step; (wx, wz) the stick in world space, -4096..4096
    int m = isq(wx * wx + wz * wz); if (m > 4096) m = 4096;
    int want = m > 900 ? yaw_of(wx, wz) : -1;
    int pr = k.pr; k.pr = 0;
    int rel = want >= 0 ? adiff(want, k.dir) : -9999, relb = want >= 0 ? adiff(want, skr.yaw) : -9999;
    sk_stick_rel = rel;
    if (k.popt) k.popt--; if (k.landsq) k.landsq--; if (k.popsq) k.popsq--;
    if (k.pushing || (k.pusht % 44)) k.pusht++; else k.pusht = 0;
    switch (k.st) {
    case KS_BAIL: if (--k.bailt <= 0) { k.st = KS_GROUND; k.dir = skr.yaw; } board_lift = k.y / FX; return;
    case KS_GROUND: case KS_MANUAL: {
        if (k.held & SKB_X) { k.crouch = 1; if (k.charge < 30) k.charge++; }
        else if (k.crouch) { k.crouch = 0; int vy = OLLIE0 + k.charge * FX / 6; k.charge = 0; if (k.st == KS_MANUAL) k.st = KS_GROUND;
            add_trick(TK_OLLIE, 25); take_off(vy, k.v, 0); k.landt = 0; k.popsq = 10; voice(V_YEHA); break; }
        if ((pr & SKB_TRI) && rail_try(1)) break;
        if (pr & SKB_SQ) { if (k.st == KS_MANUAL) { k.st = KS_GROUND; k.landt = 22; } else if (k.v > 5 * FX) { k.st = KS_MANUAL; k.bal = g_rnd(400) - 200; k.balv = 0; add_trick(TK_MANUAL, tkPts[TK_MANUAL]); k.landt = 0; } }
        if ((pr & SKB_O) && k.st == KS_GROUND) { skate_off(); say("Off the board and on your feet"); return; }
        int rate = 84 - k.v / FX; if (rate < 34) rate = 34; if (k.st == KS_MANUAL) rate /= 2;
        k.pushing = 0;
        if (want >= 0 && ABS(rel) < 1500) { int d = CLAMP(rel, -rate, rate); k.dir = (k.dir + d) & 4095; k.carve += (d * 12 - k.carve) / 6; } else k.carve -= k.carve / 6;
        if (want >= 0 && ABS(rel) >= 2300 && k.cosk > 3600) k.v -= 12;
        else if (want >= 0 && ABS(rel) < 1400 && k.st == KS_GROUND && k.v < VPUSH && k.cosk > 3600) { k.v += ACCEL * m >> 12; k.pushing = 1; }
        k.v -= 1 + k.v / 400; if (k.v < 0) k.v = 0;
        if (k.st == KS_MANUAL) { int along = rel == -9999 ? 0 : (m * icos(rel)) >> 12;
            k.balv = k.balv * 15 / 16 + k.bal / 300 + g_rnd(7) - 3 - along / 80; k.balv = CLAMP(k.balv, -200, 200); k.bal += k.balv; k.pool += 1;
            if (ABS(k.bal) > 4096) { bail("BALANCE"); break; }
            if (k.v < 2 * FX) { k.st = KS_GROUND; k.landt = 10; } }
        int y0 = k.y, px0 = skr.x, pz0 = skr.z, ok = move_h(k.dir, (k.v * k.cosk) >> 12);
        if (!ok) {
            if ((k.held & SKB_TRI) && rail_try(1)) break;
            int fx_ = skr.x + ((isin(k.dir) * 120) >> 12), fz_ = skr.z - ((icos(k.dir) * 120) >> 12), fm = fdiv(fx_, 256), fn = fdiv(-fz_, 256);
            int fc = sk_ch(fm, fn), tall = fc == '.' || sk_ih(fc) > 150;   // (a wall, or something big)
            if (tall && k.v > 18 * FX) { bail("SLAMMED"); break; }
            k.v = k.v / 4; if (!tall) { k.dir = (k.dir + 2048) & 4095; k.fakie ^= 1; k.v = MIN(k.v, 3 * FX); } }
        int g = ground_at(skr.x, skr.z, y0 / FX) * FX, run_ = (k.v * k.cosk) >> 12;
        if (g < y0 - 8 * FX - 4 * run_) { int vy = 0, vh = k.v;
            { int bx = px0 - ((isin(k.dir) * 24) >> 12), bz = pz0 + ((icos(k.dir) * 24) >> 12), rise = ground_at(px0, pz0, y0 / FX) - ground_at(bx, bz, y0 / FX);
              if (rise > 0) { int l = isq(24 * 24 + rise * rise); vy = k.v * rise / l; vh = k.v * 24 / l; } }
            if (k.st == KS_MANUAL) k.st = KS_GROUND;
            take_off(vy, vh, 1);
            if (k.vert) { skr.x = px0 - ((isin(k.vdir) * 10) >> 12); skr.z = pz0 + ((icos(k.vdir) * 10) >> 12); }
            k.landt = 0; break; }
        int dh = g - y0;
        { long v2 = (long)k.v * k.v - 2L * GRAV * dh; if (v2 <= 0) { if (dh > FX) { k.dir = (k.dir + 2048) & 4095; k.fakie ^= 1; k.v = FX * 2; } else k.v = 0; } else k.v = MIN(VMAX, isq((int)MIN(v2, 0x3FFFFFFFL))); }
        { int run = MAX((k.v * k.cosk) >> 12, FX); int l = isq(run / 4 * (run / 4) + dh / 4 * (dh / 4)); k.cosk = l ? CLAMP(((run / 4) * 4096) / l, 900, 4096) : 4096; }
        k.vy = dh; k.y = g;
        if (k.landt && --k.landt == 0 && k.st == KS_GROUND) combo_bank();
        if (!k.landt && k.ntr && k.st == KS_GROUND && !(k.held & SKB_X)) combo_bank();
        skr.yaw = (k.dir + (k.fakie ? 2048 : 0)) & 4095;
        break; }
    case KS_AIR: {
        k.airt++; k.y += k.vy; k.vy -= GRAV;
        if (k.vert) move_h((k.vdir + 1024) & 4095, k.lat); else if (!move_h(k.dir, k.vh)) k.vh = 0;
        if (k.held & SKB_L1) { skr.yaw = (skr.yaw + 78) & 4095; k.spin += 78; }
        if (k.held & SKB_R1) { skr.yaw = (skr.yaw - 78) & 4095; k.spin -= 78; }
        if (pr & SKB_SQ) { int p = dir_pick(relb), t = p == 1 ? TK_IMPOSS : p == 2 ? TK_SHOVE : p == 4 ? TK_HEEL : TK_KICK;
            if (k.flip && k.fliptk == t && k.flipn < 3) { k.flipn++; add_trick(t, tkPts[t] * k.flipn); }
            else if (!k.flip) { k.flip = 1; k.fliptk = t; k.flipn = 1; k.flipt = 0; add_trick(t, tkPts[t]); } }
        if (k.flip) { k.flipt++; int len = (k.fliptk == TK_IMPOSS ? 22 : 18) * k.flipn; if (k.flipt >= len) k.flip = 0; }
        if ((pr & SKB_O) && !k.grab && !k.flip) { int p = dir_pick(relb), t = p == 1 ? TK_NOSEGRAB : p == 2 ? TK_TAILGRAB : p == 3 ? TK_INDY : p == 4 ? TK_MELON : TK_STALE; k.grab = t; k.grabt = 0; add_trick(t, tkPts[t]); }
        if (k.grab) { if (k.held & SKB_O) { k.grabt++; if (k.grabt > 10) k.pool += 3; } else k.grab = 0; }
        if ((pr & SKB_TRI) || ((k.held & SKB_TRI) && k.vy < 0)) { if (rail_try(0)) break; }
        { int g = ground_at(skr.x, skr.z, k.y / FX + 30); if (k.vy < 0 && k.y <= g * FX) sk_land(g); }
        if (k.y < -100 * FX) k.y = 0;
        break; }
    case KS_GRIND: {
        int along = k.rdir * k.v;
        if (k.rax) { k.fx += along; int d = k.fx / FX; k.fx -= d * FX; skr.x += d; skr.z = k.rc; } else { k.fz -= along; int d = k.fz / FX; k.fz -= d * FX; skr.z += d; skr.x = k.rc; }
        k.v -= 1; if (k.v < 6 * FX) k.v = 6 * FX;
        k.pool += k.rtk == TK_BOARDS ? 2 : 1;
        { int lat = 0; if (want >= 0) { int a = adiff(want, k.dir); lat = (m * isin(a)) >> 12; }
          k.balv = k.balv * 15 / 16 + k.bal / 320 + g_rnd(7) - 3 - lat / 80; k.balv = CLAMP(k.balv, -200, 200); k.bal += k.balv;
          if (ABS(k.bal) > 4096) { bail("BALANCE"); break; } }
        int mx = fdiv(skr.x, 256), my = fdiv(-skr.z, 256), c_, off = k.rax ? (mx != k.rtx) : (my != k.rty);
        if (off) { if (grind_top(mx, my, &c_) * FX == k.rtop) { k.rtx = mx; k.rty = my; off = 0; } }
        if (k.held & SKB_X) k.crouch = 1;
        else if (k.crouch) { k.crouch = 0; add_trick(TK_OLLIE, 25); grind_end(); take_off(OLLIE0, k.v, 0); break; }
        if (off) { grind_end(); take_off(3 * FX, k.v, 0); }
        board_lift = k.y / FX; break; }
    }
    if (k.st != KS_GRIND) board_lift = k.y / FX;
}
// ---- how it looks: the board under the rider (its flip, its grab), the rider side on, crouched by what it does ----
static int skate_crouch(void) {   // how far the knees bend (0..4096), as skate_pose
    int cr = 900; if (k.crouch) cr = 2600 + MIN(k.charge, 30) * 50; else if (k.popsq) cr = 0; else if (k.landsq) cr = 900 + k.landsq * 180;
    else if (k.st == KS_GRIND) cr = 1900; else if (k.st == KS_MANUAL) cr = 1400; else if (k.st == KS_AIR) cr = (k.flip || k.grab) ? 3400 : 2000; return CLAMP(cr, 0, 4096); }
static void skate_board_rot(int *roll, int *pitch, int *spin) {
    *roll = *pitch = *spin = 0;
    if (k.st == KS_MANUAL) *pitch = -300 - k.bal / 16;
    if (k.st == KS_GRIND && k.rtk == TK_NOSEG) *pitch = 220; else if (k.st == KS_GRIND && k.rtk == TK_50) *pitch = -220;
    if (!k.flip) return;
    int len = (k.fliptk == TK_IMPOSS ? 22 : 18) * k.flipn, a = (k.flipt * 4096 * k.flipn) / MAX(len, 1);
    switch (k.fliptk) { case TK_KICK: *roll = a; break; case TK_HEEL: *roll = -a; break; case TK_IMPOSS: *pitch = a; break; case TK_SHOVE: *spin = a / 2; break; } }
#define SK_U(v) ((v) * 12 / 5)   // PS1 units up -> DS units (as the item models' heights)
static int sk_wx(void) { return (skr.x - 128) * 2 * T / 256 - GN * T + T; }
static int sk_wz(void) { return (-skr.z - 128) * 2 * T / 256 - GN * T + T; }
static void gx_mtx(int x, int y, int z, const int *R) {   // push, then move to (x, y, z) and turn by R (3x3, 4.12): into the list being recorded
    if (gxn + 40 > gx_cap) { gx_full = 1; return; }
    gx_c0(0x11); gx_c1(0x10, 2); gx_id(0x19); u32 *o = &gxb[gxn]; for (int q = 0; q < 9; q++) o[q] = (u32)R[q]; o[9] = (u32)x; o[10] = (u32)y; o[11] = (u32)z; gxn += 12; gx_c1(0x10, 1); gx_lv = 0; gx_mat = gx_nrm = gx_col = ~0u;
    gx_c1(0x29, gx_base); gx_c1(0x40, GL_QUADS); }
static void mmul3(int *o, const int *a, const int *b) { for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) o[r * 3 + c] = (a[r * 3] * b[c] + a[r * 3 + 1] * b[3 + c] + a[r * 3 + 2] * b[6 + c]) >> 12; }
static void rot_y(int *m, int a) { int c = icos(a), s = isin(a); int r[9] = { c, 0, -s, 0, 4096, 0, s, 0, c }; memcpy(m, r, sizeof r); }   // (DS row vectors: v * M)
static void rot_x(int *m, int a) { int c = icos(a), s = isin(a); int r[9] = { 4096, 0, 0, 0, c, s, 0, -s, c }; memcpy(m, r, sizeof r); }
static void rot_z(int *m, int a) { int c = icos(a), s = isin(a); int r[9] = { c, s, 0, -s, c, 0, 0, 0, 4096 }; memcpy(m, r, sizeof r); }
static int sk_ds_yaw(int ps1yaw) { return ps1yaw & 4095; }   // a PS1 heading as a turn of a model facing +z on the DS (both: 0 south, 1024 east)
static void skate_draw_board(int ground) {   // (the moving pass) the deck, its trucks and wheels, turned and flipped
    if (!on_board || !gxb) return;
    int ro, pi, sp; skate_board_rot(&ro, &pi, &sp);
    int up = k.st == KS_AIR && k.grab ? 34 : 0, y = ground - SK_U(board_lift + up) - 34;
    int Ry[9], Rr[9], Rp[9], Rs[9], t1[9], t2[9], R[9];
    rot_y(Ry, sk_ds_yaw(k.st == KS_GRIND || k.st == KS_AIR ? skr.yaw : k.dir) + sp); rot_z(Rr, ro); rot_x(Rp, pi); rot_y(Rs, 0);
    mmul3(t1, Rr, Rp); mmul3(t2, t1, Rs); mmul3(R, t2, Ry);
    gx_mtx(sk_wx(), y, sk_wz(), R);
    bx(-80, 18, -270, 80, 42, 270, C15(30, 13, 5)); bx(-76, 42, -262, 76, 46, 262, C15(6, 6, 7));   // the deck and its grip tape
    for (int e = -1; e <= 1; e += 2) { bx(-60, 4, e * 170 - 20, 60, 18, e * 170 + 20, C15(20, 20, 22)); for (int sd = -1; sd <= 1; sd += 2) bx(sd * 60 - (sd > 0 ? 0 : 30), -24, e * 170 - 22, sd * 60 + (sd > 0 ? 30 : 0), 8, e * 170 + 22, C15(30, 30, 26)); }
    gx_c1(0x12, 1); gx_lv = 0; gx_mat = gx_nrm = gx_col = ~0u; polys += 28;
}
// ---- the controls: the DS's buttons as the PS1's, the D-pad as the stick (as the camera sees it) ----
static void skate_frame(int held, int down, int steps) {
    if (!on_board) return;
    Sim *m = &sims[ctl];
    if (m->x != sk_lx || m->y != sk_ly) skate_sync_in();   // (the patch moved under us: a walk into the next lot)
    int h = 0, p = 0;
    if (held & KEY_A) h |= SKB_X; if (held & KEY_Y) h |= SKB_SQ; if (held & KEY_B) h |= SKB_O; if (held & KEY_X) h |= SKB_TRI; if (held & KEY_L) h |= SKB_L1; if (held & KEY_R) h |= SKB_R1;
    if (down & KEY_A) p |= SKB_X; if (down & KEY_Y) p |= SKB_SQ; if (down & KEY_B) p |= SKB_O; if (down & KEY_X) p |= SKB_TRI;
    k.held = h; k.pr |= p;
    int ux = ((held & KEY_RIGHT) ? 1 : 0) - ((held & KEY_LEFT) ? 1 : 0), uy = ((held & KEY_UP) ? 1 : 0) - ((held & KEY_DOWN) ? 1 : 0);
    int th = live_yaw, c = cosLerp((s16)th), s = sinLerp((s16)th), sc = (ux && uy) ? 2896 : 4096;
    int east = (ux * c + uy * s) * sc >> 12, south = (ux * s - uy * c) * sc >> 12;   // (screen up is (sin, -cos) of the camera's turn: see gx_cam)
    for (int t = 0; t < steps && on_board; t++) { skate_step(east, -south); k.held = h; }
    if (!on_board) return;
    if (sk_on_t > 0) sk_on_t -= steps;
    m->x = m->tx = m->wx = (skr.x - 128) / 16; m->y = m->ty = m->wy = (-skr.z - 128) / 16; sk_lx = m->x; sk_ly = m->y; m->act = -1; m->act_left = 0;
    sk_x3 = sk_wx(); sk_z3 = sk_wz(); sk_cr = skate_crouch() * 150 / 4096; rot_y(sk_Rm, (skr.yaw + 1024) & 4095);   // (the rider: where, how low, side on)
    { int d = skr.yaw; fq4[ctl] = ((d + 512) & 4095) / 1024 == 0 ? 0 : ((d + 512) & 4095) / 1024 == 1 ? 1 : ((d + 512) & 4095) / 1024 == 2 ? 2 : 3; }
    if (!cam_free && k.v / FX > 6 && k.st != KS_AIR) {   // riding: the camera swings round behind the way you roll
        int want = (16384 - k.dir * 8) & 32767, cur = yaw_t & 32767, d = ((want - cur + 16384) & 32767) - 16384; d = CLAMP(d, -260, 260); yaw_t += d; live_yaw = yaw_t; }
}
static void tk_name(int i, char *out) { int t = k.tk[i & 3], n = k.tkn[i & 3]; if (t == TK_SPIN) { strcpy(out, "SPIN"); return; } sprintf(out, "%s%s", n == 2 ? "DOUBLE " : n >= 3 ? "TRIPLE " : "", tkNm[t]); }
static unsigned skate_sig(void) { return (unsigned)(k.popt * 3 + k.ntr * 7 + (unsigned)k.pool * 11 + k.mult * 13 + (k.bal >> 7) * 17 + k.st * 19 + (unsigned)skate_score * 23 + (sk_on_t > 0) * 29 + (k.st == KS_AIR) * 31); }
static void skate_top(void) {   // the top screen while you ride: the score, the combo, the balance, what just happened, the buttons
    scr = offbuf; backdrop(); char b[64];
    panel(6, 6, 244, 34, "Skate"); sprintf(b, "Score %ld", skate_score); text(14, 24, 2, C15(31, 31, 20), b); sprintf(b, "Best combo %ld", skate_best); text(246 - tw(1, b), 26, 1, C15(24, 28, 31), b);
    panel(6, 44, 244, 62, 0);
    if (k.ntr) { int n = MIN(k.ntr, 3); b[0] = 0; for (int i = k.ntr - n; i < k.ntr; i++) { char t[24]; tk_name(i, t); if (b[0]) strcat(b, " + "); strcat(b, t); }
        text_fit(128 - MIN(tw(1, b), 232) / 2, 52, 1, 232, TXT, b); sprintf(b, "%ld X %d", k.pool, MAX(k.mult, 1)); text(128 - tw(2, b) / 2, 66, 2, C15(16, 26, 31), b); }
    else text(128 - tw(1, "Push off and find a ramp") / 2, 58, 1, C15(20, 24, 28), "Push off and find a ramp");
    if (k.st == KS_GRIND || k.st == KS_MANUAL) { int x = 78, y = 86, w = 100, p = CLAMP(k.bal, -4096, 4096) * (w / 2) / 4096;
        rect(x - 2, y - 2, w + 4, 10, C15(31, 31, 31)); rect(x, y, w, 6, C15(8, 10, 16)); rect(x + w / 2 - 12, y, 24, 6, C15(15, 26, 14)); rect(x, y, 14, 6, C15(29, 11, 9)); rect(x + w - 14, y, 14, 6, C15(29, 11, 9)); rect(x + w / 2 + p - 2, y - 2, 4, 10, C15(4, 5, 8)); }
    if (k.popt) { panel(40, 110, 176, 36, 0); text(128 - tw(2, k.popa) / 2, 116, 2, k.popok ? C15(14, 30, 14) : C15(31, 10, 8), k.popa); if (k.popb[0]) text(128 - tw(1, k.popb) / 2, 132, 1, C15(24, 28, 31), k.popb); }
    panel(6, 150, 244, 38, 0);
    if (k.st == KS_AIR) { text(12, 157, 1, TXT, "Y flip   B grab (hold)   L R spin"); text(12, 171, 1, TXT, "X grind   D-pad picks the trick"); }
    else { text(12, 157, 1, TXT, "A ollie (hold: higher)   Y manual"); text(12, 171, 1, TXT, "X grind   B off   D-pad rides"); }
}
