// ds_games.c - the PS1's mini-games on the DS (included by main.c, after the lot, the sims and the 2D helpers it uses).
// D46: the ARCADE (src/arcade.c): BORE BREAKER, SNACK CATCH, WHACK-A-BORE, HIGH STRIKER, the prize counter, tickets.
// D45: SKATE (src/skate.c): the board in Live, the PS1's physics, tricks, combos and score.

// ---- small maths in the PS1's units (angles 0..4095 a turn, sines x4096) ----
static inline int isin(int a) { return sinLerp((s16)((a & 4095) << 3)); }
static inline int icos(int a) { return cosLerp((s16)((a & 4095) << 3)); }
static int g_isq(int n) { if (n <= 0) return 0; int x = n, y = (x + 1) >> 1; while (y < x) { x = y; y = (x + n / x) >> 1; } return x; }
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
static int sk_yaw_of(int y, int x) {   // the PS1's atan_ (0 south, 1024 east)
    if (!x && !y) return 0;
    int ax = ABS(x), ay = ABS(y), a = ax >= ay ? (ay * 512) / (ax + (ay >> 1)) : 1024 - (ax * 512) / (ay + (ax >> 1));
    if (x < 0) a = 2048 - a; if (y < 0) a = -a; return a & 4095;
}
static int yaw_of(int dx, int dz) { return sk_yaw_of(dx, -dz) & 4095; }
static int adiff(int a, int b) { return ((a - b + 2048) & 4095) - 2048; }
// ---- what is on a cell, as the PS1's world letters ----
static int sk_cell(int mx, int my) { int keep = LV; LV = 0; int o = obj_at(mx, my); LV = keep; return o; }
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
    { int R = n * 256, x = (d * 4 * 972) >> 10; return R - g_isq(R * R - x * x); } }
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
            if (ground_at(nx, nz, y) > y + 100) continue; }   // (the PS1 lets a board climb 100 units a step)
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
    if (wasvert || behind > ahead + 10) k.v = MIN(VMAX, g_isq(vh * vh + vfall * vfall) * 15 / 16); else k.v = vh;
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
    int m = g_isq(wx * wx + wz * wz); if (m > 4096) m = 4096;
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
              if (rise > 0) { int l = g_isq(24 * 24 + rise * rise); vy = k.v * rise / l; vh = k.v * 24 / l; } }
            if (k.st == KS_MANUAL) k.st = KS_GROUND;
            take_off(vy, vh, 1);
            if (k.vert) { skr.x = px0 - ((isin(k.vdir) * 10) >> 12); skr.z = pz0 + ((icos(k.vdir) * 10) >> 12); }
            k.landt = 0; break; }
        int dh = g - y0;
        { long v2 = (long)k.v * k.v - 2L * GRAV * dh; if (v2 <= 0) { if (dh > FX) { k.dir = (k.dir + 2048) & 4095; k.fakie ^= 1; k.v = FX * 2; } else k.v = 0; } else k.v = MIN(VMAX, g_isq((int)MIN(v2, 0x3FFFFFFFL))); }
        { int run = MAX((k.v * k.cosk) >> 12, FX); int l = g_isq(run / 4 * (run / 4) + dh / 4 * (dh / 4)); k.cosk = l ? CLAMP(((run / 4) * 4096) / l, 900, 4096) : 4096; }
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
#ifdef DSTEST
    u32 t0_ = prof_now();
#endif
    for (int t = 0; t < steps && on_board; t++) { skate_step(east, -south); k.held = h; }
#ifdef DSTEST
    { extern volatile int sk_cyc; sk_cyc += (int)(prof_now() - t0_); }
#endif
    if (!on_board) return;
    if (sk_on_t > 0) sk_on_t -= steps;
    m->x = m->tx = m->wx = (skr.x - 128) / 16; m->y = m->ty = m->wy = (-skr.z - 128) / 16; sk_lx = m->x; sk_ly = m->y; m->act = -1; m->act_left = 0;
    sk_x3 = sk_wx(); sk_z3 = sk_wz(); sk_cr = skate_crouch() * 150 / 4096; rot_y(sk_Rm, (skr.yaw + 1024) & 4095);   // (the rider: where, how low, side on)
    { int d = skr.yaw; fq4[ctl] = ((d + 512) & 4095) / 1024 == 0 ? 0 : ((d + 512) & 4095) / 1024 == 1 ? 1 : ((d + 512) & 4095) / 1024 == 2 ? 2 : 3; }
    if (!cam_free && k.v / FX > 6 && k.st != KS_AIR) {   // riding: the camera swings round behind the way you roll
        int want = (16384 - k.dir * 8) & 32767, cur = yaw_t & 32767, d = ((want - cur + 16384) & 32767) - 16384; static int swing; if (abs(d) > 7000) swing = 1; if (abs(d) < 600) swing = 0;   // (a dead zone: the still lists are only recorded again while the view swings round)
        if (swing) { d = CLAMP(d, -300, 300); yaw_t += d; live_yaw = yaw_t; } }
}
static void tk_name(int i, char *out) { int t = k.tk[i & 3], n = k.tkn[i & 3]; if (t == TK_SPIN) { strcpy(out, "SPIN"); return; } sprintf(out, "%s%s", n == 2 ? "DOUBLE " : n >= 3 ? "TRIPLE " : "", tkNm[t]); }
static unsigned skate_sig(void) { int bl = (k.st == KS_GRIND || k.st == KS_MANUAL) ? (k.bal >> 9) + 100 : 0; return (unsigned)((k.popt > 0) * 3 + k.ntr * 7 + (unsigned)(k.pool >> 3) * 11 + k.mult * 13 + bl * 17 + (k.st == KS_AIR) * 19 + (unsigned)skate_score * 23); }   // (only what the top screen shows, coarsely: it is redrawn whole)
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

// ==== ARCADE (src/arcade.c): the cabinets' games and the prize counter, as the PS1's. The field is the PS1's 240 x 160, drawn 1:1 on the top screen; the bottom is the touch pad ====
enum { AG_FIGHTER, AG_BREAKER, AG_CATCH, AG_WHACK, AG_STRIKER, AG_NGAME, AG_PRIZES = 99 };
enum { PD_L = 1, PD_R = 2, PD_U = 4, PD_D = 8, PD_X = 16, PD_O = 32, PD_ST = 64 };
#define K8(r, g, b) C15((r) >> 3, (g) >> 3, (b) >> 3)
#define FIGHT_READY 0   // (the fight cabinet comes with the fight)
static const char *const agName[AG_NGAME] = { "BORTUA FIGHTER", "BORE BREAKER", "SNACK CATCH", "WHACK-A-BORE", "HIGH STRIKER" };
static const u8 agPrice[AG_NGAME] = { 2, 2, 1, 1, 1 };
static const char *const agHow1[AG_NGAME] = { "", "MOVE THE PADDLE: D-PAD OR STYLUS", "MOVE THE BASKET: D-PAD OR STYLUS", "TAP A HOLE, OR D-PAD AND A", "TAP STOP, OR PRESS A" };
static const char *const agHow2[AG_NGAME] = { "", "SERVE WITH A. CLEAR THE WALL", "CATCH SNACKS, DODGE ROCKS", "WHACK THE BORE WHEN IT POPS UP", "FIVE SWINGS: RING THE BELL" };
static int arcade_game_at(int x, int y) { int g = (x * 3 + y * 2) % AG_NGAME; if (g == AG_FIGHTER && !FIGHT_READY) g = 1 + (x + y) % 4; return g; }
#ifdef DSTEST
static int ag_testlot;
#endif
static int in_arcade(void) { BInfo b; blk(lot_bx, lot_by, &b);
#ifdef DSTEST
    if (ag_testlot) return 1;
#endif
    return b.lot == LOT_ARCADE; }
static u32 a_rng = 0x9E3779B9u;
static int r8(void) { a_rng = a_rng * 1103515245u + 12345u; return (int)((a_rng >> 16) & 255); }
static int ag, ag_phase, ag_acc, ag_pts, ag_tix, ag_t, ag_lives, ag_psel, ag_tx = -1, ag_tap = -1, ag_dirty;
static const char *ag_msg = "";
static u8 ab_br[4][10]; static int ab_left, ab_px, ab_bx, ab_by, ab_vx, ab_vy, ab_held;   // BORE BREAKER
static s16 ac_ix[6], ac_iy[6]; static u8 ac_ik[6], ac_on[6]; static int ac_bk;               // SNACK CATCH
static int aw_sel, aw_mole, aw_mt, aw_gap, aw_hit;                                           // WHACK-A-BORE
static int as_tr, as_pos, as_dir, as_stop;                                                   // HIGH STRIKER
static void ag_start(void) {
    ag_pts = 0; ag_tix = 0; ag_t = 0; ag_lives = 3;
    switch (ag) {
    case AG_BREAKER: for (int r = 0; r < 4; r++) for (int c = 0; c < 10; c++) ab_br[r][c] = 1; ab_left = 40; ab_px = 102; ab_held = 1; break;
    case AG_CATCH: memset(ac_on, 0, sizeof ac_on); ac_bk = 106; break;
    case AG_WHACK: aw_sel = 4; aw_mole = -1; aw_gap = 10; aw_hit = 0; break;
    case AG_STRIKER: as_tr = 0; as_pos = 0; as_dir = 1; as_stop = 0; break;
    }
}
static int ag_tick(int hd, int pr) {   // one 30 Hz tick of the game; 0 when it is over (arcade.c tick)
    if (pr & PD_ST) return 0;
    switch (ag) {
    case AG_BREAKER:
        if (hd & PD_L) ab_px -= 5; if (hd & PD_R) ab_px += 5; if (ag_tx >= 0) ab_px = ag_tx * 240 / 256 - 18; ab_px = CLAMP(ab_px, 20, 184);
        if (ab_held) { ab_bx = ab_px + 16; ab_by = 138; if (pr & PD_X) { ab_held = 0; ab_vx = (r8() & 1) ? 2 : -2; ab_vy = -3; } }
        else {
            ab_bx += ab_vx; ab_by += ab_vy;
            if (ab_bx < 20) { ab_bx = 20; ab_vx = -ab_vx; } if (ab_bx > 216) { ab_bx = 216; ab_vx = -ab_vx; } if (ab_by < 13) { ab_by = 13; ab_vy = -ab_vy; }
            if (ab_vy > 0 && ab_by >= 140 && ab_by <= 146 && ab_bx + 4 >= ab_px && ab_bx <= ab_px + 36) { ab_vy = -3; ab_vx = CLAMP((ab_bx + 2 - (ab_px + 18)) / 5, -3, 3); if (!ab_vx) ab_vx = (r8() & 1) ? 1 : -1; ab_by = 138; sfx(0); }
            else if (ab_by >= 160) { ag_lives--; ab_held = 1; voice(V_NAH); }
            { int c = (ab_bx + 2 - 20) / 20, r = (ab_by + 2 - 24) / 9; if (ab_bx + 2 >= 20 && ab_by + 2 >= 24 && r < 4 && c < 10 && ab_br[r][c]) { ab_br[r][c] = 0; ab_left--; ag_pts++; ab_vy = -ab_vy; sfx(1); } }
        }
        if (ag_lives <= 0 || ab_left <= 0) { ag_tix = ag_pts / 2 + (ab_left == 0 ? 8 : 0); return 0; }
        return 1;
    case AG_CATCH:
        if (hd & PD_L) ac_bk -= 6; if (hd & PD_R) ac_bk += 6; if (ag_tx >= 0) ac_bk = ag_tx * 240 / 256 - 14; ac_bk = CLAMP(ac_bk, 4, 208);
        if (ag_t % 12 == 0) for (int i = 0; i < 6; i++) if (!ac_on[i]) { ac_on[i] = 1; ac_ix[i] = (s16)(8 + r8() % 216); ac_iy[i] = 14; ac_ik[i] = (u8)((r8() % 4 == 0) ? 2 : (r8() & 1)); break; }
        for (int i = 0; i < 6; i++) if (ac_on[i]) {
            ac_iy[i] = (s16)(ac_iy[i] + 3 + ag_t / 200);
            if (ac_iy[i] >= 134 && ac_iy[i] <= 146 && ac_ix[i] + 8 >= ac_bk && ac_ix[i] <= ac_bk + 28) { ac_on[i] = 0; if (ac_ik[i] == 2) { ag_lives--; voice(V_GROAN); } else { ag_pts++; sfx(1); } }
            else if (ac_iy[i] >= 160) ac_on[i] = 0; }
        if (++ag_t >= 600 || ag_lives <= 0) { ag_tix = ag_pts / 2; return 0; }
        return 1;
    case AG_WHACK:
        if ((pr & PD_L) && aw_sel % 3 > 0) aw_sel--; if ((pr & PD_R) && aw_sel % 3 < 2) aw_sel++;
        if ((pr & PD_U) && aw_sel > 2) aw_sel -= 3; if ((pr & PD_D) && aw_sel < 6) aw_sel += 3;
        if (ag_tap >= 0) { aw_sel = ag_tap; pr |= PD_X; ag_tap = -1; }
        if (aw_mole < 0) { if (--aw_gap <= 0) { aw_mole = r8() % 9; aw_mt = 26 - MIN(12, ag_pts); } }
        else if (--aw_mt <= 0) { aw_mole = -1; aw_gap = 6 + (r8() & 7); }
        if (aw_hit > 0) aw_hit--;
        if (pr & PD_X) { if (aw_mole == aw_sel) { ag_pts++; aw_mole = -1; aw_gap = 5 + (r8() & 7); aw_hit = 6; sfx(1); } }
        if (++ag_t >= 900) { ag_tix = ag_pts * 2 / 3; return 0; }
        return 1;
    case AG_STRIKER:
        if (as_stop) { if (--as_stop == 0) { if (++as_tr >= 5) { ag_tix = ag_pts / 45; return 0; } as_pos = 0; as_dir = 1; } return 1; }
        if (pr & PD_X) { ag_pts += as_pos; as_stop = 30; if (as_pos >= 95) voice(V_YEHA); else sfx(0); return 1; }
        as_pos += as_dir * (4 + as_tr * 2); if (as_pos >= 100) { as_pos = 100; as_dir = -1; } if (as_pos <= 0) { as_pos = 0; as_dir = 1; }
        return 1;
    }
    return 0;
}
typedef struct { const char *nm; u16 cost; s8 fun, food, social; const char *say; } Prize;
static const Prize prz[] = {
    { "CANDY", 10, 5, 15, 0, "SUGAR RUSH!" }, { "BOUNCY BALL", 25, 15, 0, 5, "BOING BOING BOING" }, { "PLUSH TEDDY", 60, 25, 0, 10, "IT HAS A TINY HAT" },
    { "MEGA TROPHY", 150, 30, 0, 30, "NUMBER ONE!" }, { "GOLDEN TICKET", 400, 10, 0, 0, "TRADED IN FOR $500!" } };
#define NPRZ ((int)(sizeof prz / sizeof prz[0]))
static void need_add(Sim *s, int n, int v) { s->need[n] = cl100(s->need[n] + v); }
static void ag_open(int g) { ag = g; ag_phase = 0; ag_acc = 0; ag_psel = 0; ag_msg = ""; ag_tx = ag_tap = -1; mg_ui = g == AG_PRIZES ? 2 : 1; mg_ver++; }
static void ag_finish(void) {   // a game over: the tickets, a little fun, a little sleepier
    Sim *s = &sims[ctl]; static char b[48];
    arcade_tix = MIN(9999, arcade_tix + ag_tix);
    need_add(s, 1, 18 + ag_tix); need_add(s, 2, -3); life_ev(ctl, WN_FUN, 0);
    snprintf(b, sizeof b, "+%d tickets (%d)", ag_tix, arcade_tix); say(b);
    voice(ag_tix >= 8 ? V_YAHOO : ag_tix ? V_LAUGH : V_NAH);
}
static int ag_update(int hd, int pr, int steps) {   // 1 while it runs (arcade_update)
    if (ag == AG_PRIZES) {
        if (pr & PD_U) ag_psel = (ag_psel + NPRZ - 1) % NPRZ;
        if (pr & PD_D) ag_psel = (ag_psel + 1) % NPRZ;
        if (pr & PD_O) return 0;
        if (pr & PD_X) {
            const Prize *p = &prz[ag_psel];
            if (arcade_tix < p->cost) { ag_msg = "NOT ENOUGH TICKETS YET"; voice(V_NAH); return 1; }
            Sim *s = &sims[ctl]; arcade_tix -= p->cost; static char b[48];
            need_add(s, 1, p->fun); need_add(s, 0, p->food); need_add(s, 3, p->social);
            if (ag_psel == NPRZ - 1) funds += 500;
            snprintf(b, sizeof b, "%s", p->say); say(b); voice(V_YAHOO); life_ev(ctl, WN_FUN, 0);
            return 0;
        }
        return 1;
    }
    if (ag_phase == 0) { if (pr & PD_X) { ag_phase = 1; ag_start(); } if (pr & PD_O) { funds += agPrice[ag]; return 0; } return 1; }   // (backing out: the money back)
    if (ag_phase == 2) { ag_acc += steps; if (ag_acc > 40 && (pr & (PD_X | PD_O))) { ag_finish(); return 0; } return 1; }
    ag_acc += steps;
    while (ag_acc >= 2) { ag_acc -= 2; int k = ag_tick(hd, pr); pr = 0; ag_dirty = 1; if (!k) { ag_phase = 2; ag_acc = 0; break; } }
    return 1;
}
// ---- the picture: the field on the top screen, the pad and the buttons on the bottom ----
#define AX(x) (8 + (x))
#define AY(y) (16 + (y))
static void aR(int x, int y, int w, int h, u16 c) { rect(AX(x), AY(y), w, h, c); }
static void aD(int cx, int cy, int r, u16 c) { ellf(AX(cx), AY(cy), r, r, c); }
static void ctext(int cx, int y, int s, u16 c, const char *t) { text(cx - tw(s, t) / 2, y, s, c, t); }
static void ar_top(void) {
    scr = offbuf; char b[40]; rect(0, 0, 256, 192, K8(16, 10, 34));
    if (mg_ui == 2) {
        panel(8, 8, 240, 160, "The Prize Counter");
        for (int i = 0; i < NPRZ; i++) { int y = 32 + i * 24, s = i == ag_psel; rect(14, y - 2, 228, 22, s ? K8(226, 242, 254) : PANB); rect(15, y - 1, 226, 20, s ? C15(24, 28, 31) : PAN);
            text(22, y + 5, 1, s ? C15(2, 7, 20) : TXT, prz[i].nm); sprintf(b, "%d tix", prz[i].cost); text(236 - tw(1, b), y + 5, 1, arcade_tix >= prz[i].cost ? C15(14, 30, 14) : C15(31, 12, 10), b); }
        sprintf(b, "You have %d tickets", arcade_tix); ctext(128, 156, 1, C15(24, 28, 31), b); if (ag_msg[0]) ctext(128, 180, 1, C15(31, 14, 12), ag_msg);
        return; }
    rect(8, 1, 240, 13, K8(60, 30, 110)); ctext(128, 4, 1, K8(90, 230, 240), agName[ag]);
    aR(0, 0, 240, 160, K8(4, 4, 18));
    if (ag_phase == 0) { ctext(128, 50, 2, K8(255, 210, 50), agName[ag]); ctext(128, 100, 1, TXT, agHow1[ag]); ctext(128, 116, 1, TXT, agHow2[ag]); }
    else {
        char s[24];
        #define HUD2(a, n, bb, m) do { snprintf(s, sizeof s, "%s %d", a, n); text(AX(4), AY(2) + 2, 1, TXT, s); snprintf(s, sizeof s, "%s %d", bb, m); text(AX(236) - tw(1, s), AY(2) + 2, 1, K8(255, 190, 40), s); } while (0)
        switch (ag) {
        case AG_BREAKER: { static const u8 rc[4][3] = { { 250, 50, 50 }, { 250, 146, 32 }, { 226, 226, 32 }, { 50, 210, 80 } };
            HUD2("SCORE", ag_pts, "LIVES", ag_lives);
            for (int r = 0; r < 4; r++) for (int c = 0; c < 10; c++) if (ab_br[r][c]) aR(20 + c * 20 + 1, 24 + r * 9 + 1, 18, 7, K8(rc[r][0], rc[r][1], rc[r][2]));
            aR(ab_px, 144, 36, 5, K8(32, 226, 240)); aR(ab_px, 144, 36, 1, K8(255, 255, 255)); aR(ab_bx, ab_by, 4, 4, K8(255, 255, 255));
            if (ab_held && ag_phase == 1) ctext(AX(120), AY(110), 1, K8(170, 210, 250), "A SERVES");
            break; }
        case AG_CATCH:
            HUD2("SNACKS", ag_pts, "LIVES", ag_lives); snprintf(s, sizeof s, "TIME %d", MAX(0, 600 - ag_t) / 30); ctext(AX(120), AY(2) + 2, 1, K8(170, 210, 250), s);
            for (int i = 0; i < 6; i++) if (ac_on[i]) { if (ac_ik[i] == 2) aR(ac_ix[i], ac_iy[i], 8, 8, K8(110, 110, 120)); else aD(ac_ix[i] + 4, ac_iy[i] + 4, 4, ac_ik[i] ? K8(250, 210, 32) : K8(250, 80, 160)); }
            aR(ac_bk, 142, 28, 8, K8(192, 112, 32)); aR(ac_bk, 142, 28, 2, K8(250, 176, 64));
            break;
        case AG_WHACK:
            HUD2("HITS", ag_pts, "TIME", MAX(0, 900 - ag_t) / 30);
            for (int i = 0; i < 9; i++) { int cx = 60 + (i % 3) * 60, cy = 44 + (i / 3) * 38;
                aD(cx, cy + 6, 16, K8(8, 8, 16)); aD(cx, cy + 4, 15, K8(40, 24, 16));
                if (i == aw_mole) { aD(cx, cy - 4, 11, K8(160, 96, 48)); aR(cx - 5, cy - 7, 3, 3, K8(255, 255, 255)); aR(cx + 3, cy - 7, 3, 3, K8(255, 255, 255)); aR(cx - 3, cy + 1, 7, 2, K8(8, 8, 16)); }
                if (i == aw_sel) { aR(cx - 19, cy - 17, 38, 1, K8(250, 210, 40)); aR(cx - 19, cy + 19, 38, 1, K8(250, 210, 40)); aR(cx - 19, cy - 17, 1, 37, K8(250, 210, 40)); aR(cx + 18, cy - 17, 1, 37, K8(250, 210, 40)); } }
            if (aw_hit > 0) ctext(AX(120), AY(140), 2, K8(250, 210, 40), "BONK!");
            break;
        case AG_STRIKER:
            HUD2("POINTS", ag_pts, "SWING", MIN(as_tr + 1, 5));
            aR(106, 22, 28, 120, K8(48, 48, 80)); aR(108, 24, 24, 116, K8(16, 16, 32));
            { int h = as_pos * 116 / 100; aR(108, 140 - h, 24, h, as_pos >= 95 ? K8(250, 210, 32) : as_pos >= 60 ? K8(250, 112, 32) : K8(48, 192, 80)); }
            aD(120, 17, 5, K8(250, 210, 40));
            if (as_stop) ctext(AX(120), AY(146), 2, TXT, as_pos >= 95 ? "DING!" : as_pos >= 60 ? "NOT BAD" : "WEAK!");
            break;
        }
        #undef HUD2
        if (ag_phase == 2) { rect(48, 50, 160, 76, K8(10, 12, 30)); rect(50, 52, 156, 72, K8(20, 22, 44)); ctext(128, 60, 2, K8(250, 210, 40), "GAME OVER");
            sprintf(b, "SCORE %d", ag_pts); ctext(128, 88, 1, TXT, b); sprintf(b, "+%d TICKETS", ag_tix); ctext(128, 104, 1, C15(14, 30, 14), b); }
    }
    sprintf(b, "TICKETS %d", arcade_tix); ctext(128, 182, 1, K8(200, 210, 240), b);
}
#define AQ_X 196
#define AQ_Y 170
static void ar_bottom(void) {
    scr = offbuf; char b[32]; rect(0, 0, 256, 192, K8(24, 16, 54)); rect(0, 0, 256, 16, K8(60, 30, 110)); ctext(128, 4, 1, K8(90, 230, 240), mg_ui == 2 ? "THE PRIZE COUNTER" : agName[ag]);
    if (mg_ui == 2) {
        for (int i = 0; i < NPRZ; i++) { int y = 22 + i * 27, s = i == ag_psel; rect(8, y, 240, 25, s ? K8(250, 210, 40) : PANB); rect(9, y + 1, 238, 23, s ? C15(24, 28, 31) : PAN); text(16, y + 9, 1, s ? C15(2, 7, 20) : TXT, prz[i].nm); sprintf(b, "%d tix", prz[i].cost); text(240 - tw(1, b), y + 9, 1, arcade_tix >= prz[i].cost ? (s ? C15(2, 14, 4) : C15(14, 30, 14)) : C15(31, 12, 10), b); }
        chip(8, 162, 116, 26, "Swap", 1); chip(132, 162, 116, 26, "Never mind", 0); return; }
    if (ag_phase == 0) { sprintf(b, "Play $%d", agPrice[ag]); chip(14, 70, 108, 54, b, 1); chip(134, 70, 108, 54, "Money back", 0); ctext(128, 34, 1, TXT, agHow1[ag]); ctext(128, 48, 1, TXT, agHow2[ag]); return; }
    if (ag_phase == 2) { chip(30, 60, 196, 70, "Done", 1); return; }
    chip(AQ_X, AQ_Y, 56, 18, "Quit", 0);
    switch (ag) {
    case AG_BREAKER: case AG_CATCH:
        rect(8, 22, 240, 100, K8(40, 28, 90)); rect(10, 24, 236, 96, K8(30, 20, 70)); ctext(128, 66, 1, K8(170, 190, 250), "Slide the stylus here");
        if (ag == AG_BREAKER) chip(8, 130, 150, 40, "Serve (A)", ab_held); break;
    case AG_WHACK:
        for (int i = 0; i < 9; i++) { int x = 8 + (i % 3) * 80, y = 22 + (i / 3) * 46; rect(x, y, 76, 42, i == aw_sel ? K8(250, 210, 40) : K8(60, 40, 110)); rect(x + 2, y + 2, 72, 38, i == aw_sel ? K8(90, 60, 30) : K8(30, 20, 60)); }
        break;
    case AG_STRIKER: chip(8, 24, 240, 130, "STOP (A)", 1); break;
    }
}
static unsigned mg_sig(void) { return (unsigned)(mg_ver * 131 + mg_ui * 7); }
static unsigned mg_bsig(void) { return (unsigned)(mg_ui * 3 + ag_phase * 5 + ag * 7 + (mg_ui == 1 && ag == AG_WHACK ? aw_sel * 11 : 0) + ag_psel * 13 + ab_held * 17 + (arcade_tix > 0) * 23); }
static void mg_top(void) { ar_top(); }
static void mg_bottom(void) { ar_bottom(); }
static void mg_frame(int held, int down, int steps, int tx, int ty) {
    int hd = 0, pr = 0, tch = held & KEY_TOUCH, td = down & KEY_TOUCH; ag_tx = -1; ag_dirty = 0;
    if (held & KEY_LEFT) hd |= PD_L; if (held & KEY_RIGHT) hd |= PD_R; if (held & KEY_UP) hd |= PD_U; if (held & KEY_DOWN) hd |= PD_D; if (held & KEY_A) hd |= PD_X;
    if (down & KEY_LEFT) pr |= PD_L; if (down & KEY_RIGHT) pr |= PD_R; if (down & KEY_UP) pr |= PD_U; if (down & KEY_DOWN) pr |= PD_D; if (down & KEY_A) pr |= PD_X; if (down & KEY_B) pr |= PD_O; if (down & KEY_START) pr |= PD_ST;
    if (mg_ui == 2) {
        if (td) { for (int i = 0; i < NPRZ; i++) if (hit(8, 22 + i * 27, 240, 25, tx, ty)) ag_psel = i; if (hit(8, 162, 116, 26, tx, ty)) pr |= PD_X; if (hit(132, 162, 116, 26, tx, ty)) pr |= PD_O; }
    } else if (ag_phase == 0) { if (td) { if (hit(14, 70, 108, 54, tx, ty)) pr |= PD_X; if (hit(134, 70, 108, 54, tx, ty)) pr |= PD_O; } }
    else if (ag_phase == 2) { if (td) pr |= PD_X; }
    else {
        if (td && hit(AQ_X, AQ_Y, 56, 18, tx, ty)) pr |= PD_ST;
        if (ag == AG_BREAKER || ag == AG_CATCH) { if (tch && hit(8, 22, 240, 100, tx, ty)) ag_tx = tx; if (ag == AG_BREAKER && td && hit(8, 130, 150, 40, tx, ty)) pr |= PD_X; }
        else if (ag == AG_WHACK) { if (td) for (int i = 0; i < 9; i++) if (hit(8 + (i % 3) * 80, 22 + (i / 3) * 46, 76, 42, tx, ty)) ag_tap = i; }
        else if (ag == AG_STRIKER) { if (td && hit(8, 24, 240, 130, tx, ty)) pr |= PD_X; }
    }
    int old = ag_phase, op = ag_psel;
    if (!ag_update(hd, pr, steps)) { mg_ui = 0; mg_ver++; return; }
    if (ag_dirty || pr || old != ag_phase || op != ag_psel || ag_tx >= 0) mg_ver++;
}

// ==== the mini-games' saves: blocks after the farm's, each { magic, size, data }; a save from before has none (all at their start) ====
#define MG_AR 0x5241   // 'AR': the arcade's tickets, the fight record
#define MG_SK 0x4B53   // 'SK': the board and the skate scores
static void mg_reset(void) { has_board = on_board = 0; board_lift = 0; skate_score = skate_best = 0; arcade_tix = fight_wins = fight_champ = fight_best = 0; mg_ui = 0; }
static int mg_pack(u8 *o) {
    int n = 0; u16 th[2]; s32 v[3];
    th[0] = MG_SK; th[1] = 12; v[0] = has_board; v[1] = (s32)skate_score; v[2] = (s32)skate_best; memcpy(o + n, th, 4); memcpy(o + n + 4, v, 12); n += 16;
    th[0] = MG_AR; th[1] = 16; { s32 w[4] = { arcade_tix, fight_wins, fight_champ, fight_best }; memcpy(o + n, th, 4); memcpy(o + n + 4, w, 16); n += 20; }
    return n; }
static int mg_unpack(const u8 *p, int left) {
    mg_reset(); int n = 0;
    while (n + 4 <= left) { u16 th[2]; memcpy(th, p + n, 4); if (n + 4 + th[1] > left) break;
        if (th[0] == MG_SK && th[1] == 12) { s32 v[3]; memcpy(v, p + n + 4, 12); has_board = v[0] & 1; skate_score = v[1] < 0 ? 0 : v[1]; skate_best = v[2] < 0 ? 0 : v[2]; }
        else if (th[0] == MG_AR && th[1] == 16) { s32 w[4]; memcpy(w, p + n + 4, 16); arcade_tix = CLAMP(w[0], 0, 9999); fight_wins = MAX(0, w[1]); fight_champ = w[2] & 1; fight_best = MAX(0, w[3]); }
        else break;
        n += 4 + th[1]; }
    return n; }
#ifdef DSTEST
static void mg_test(int sc) {
    if (sc == 42) { ag_testlot = 1; memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; gen_public(LOT_ARCADE, 0x5A3C91u); occ_ok = 0; funds += 5000; for (int q = 0; q < nsim; q++) { sims[q].x = sims[q].tx = sims[q].wx = (5 + q) * 16; sims[q].y = sims[q].ty = sims[q].wy = 7 * 16; sims[q].act = -1; } }
    if (sc >= 43 && sc <= 47) { arcade_tix = 37; funds += 5000; ag_open(sc == 47 ? AG_PRIZES : sc - 42); }   // (test builds) 40: this lot becomes the skate plaza, your Bore on the board at its west side
    if (sc == 40 || sc == 41) { memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; gen_public(LOT_SKATE, 1); occ_ok = 0; funds += 5000;
        for (int q = 0; q < nsim; q++) { sims[q].x = sims[q].tx = sims[q].wx = 0; sims[q].y = sims[q].ty = sims[q].wy = (5 + q) * 16; sims[q].act = -1; }
        sims[ctl].x = sims[ctl].tx = sims[ctl].wx = (sc == 41 ? 3 : 1) * 16; sims[ctl].y = sims[ctl].ty = sims[ctl].wy = (sc == 41 ? 3 : 7) * 16; fq4[ctl] = 1; has_board = 1; skate_on(); }
}
#endif
// ---- the ramps drawn as the PS1 draws them (items.c ramp_draw): the run's own curve, sides only at its ends; a few quads instead of the model's stacked boxes ----
static u32 qd_nrm(const int (*p)[3]) {   // a quad's normal in integers, as qd works it out (qd's floats and square root cost a frame on the ARM9)
    int ux = p[1][0] - p[0][0], uy = p[0][1] - p[1][1], uz = p[1][2] - p[0][2], vx = p[3][0] - p[0][0], vy = p[0][1] - p[3][1], vz = p[3][2] - p[0][2];
    s64 a = (s64)uy * vz - (s64)uz * vy, b = (s64)uz * vx - (s64)ux * vz, d = (s64)ux * vy - (s64)uy * vx;
    while (a > 20000 || a < -20000 || b > 20000 || b < -20000 || d > 20000 || d < -20000) { a /= 4; b /= 4; d /= 4; }
    int l = g_isq((int)(a * a + b * b + d * d)), k = lit_sgn * 511; if (!l) return NORMAL_PACK(0, 511, 0);
    return NORMAL_PACK((int)(a * k / l), (int)(b * k / l), (int)(d * k / l)) | 0x80000000u; }   // (bit 31: worked out; the GPU ignores it)
static void qd_c(const int (*p)[3], u16 c, u32 *nc) {   // a quad whose normal is kept in *nc once worked out (the same slope, the same normal)
    if (!gxb) { qd(p[0][0], p[0][1], p[0][2], p[1][0], p[1][1], p[1][2], p[2][0], p[2][1], p[2][2], p[3][0], p[3][1], p[3][2], c); return; }
    if (!*nc || ((*nc >> 30) & 1) != (lit_sgn < 0)) *nc = qd_nrm(p) | (lit_sgn < 0 ? 0x40000000u : 0);
    qd_n(p[0][0], p[0][1], p[0][2], p[1][0], p[1][1], p[1][2], p[2][0], p[2][1], p[2][2], p[3][0], p[3][1], p[3][2], c, *nc & 0x3FFFFFFFu); }
static s16 rp_hc[3][3][3][8]; static u8 rp_hok; static u32 rp_nt[3][3][3][6][4], rp_ns[3][3][3][6][4][2], rp_nb[4], rp_ncop[4];
static int rp_seg(int kind) { return kind == 2 ? 4 : kind == 1 ? 3 : 2; }
volatile int dbg_noramp;
static void ramp3d(int x, int y, int ch, int r, int base) {
    if (dbg_noramp) return;
    if (!rp_hok) { rp_hok = 1; for (int kd = 0; kd < 3; kd++) for (int nn = 1; nn <= 3; nn++) for (int ix = 0; ix < nn; ix++) for (int q = 0; q <= rp_seg(kd); q++) rp_hc[kd][nn - 1][ix][q] = (s16)ramp_prof(kd, nn, nn * 64 - (ix * 64 + 64 * q / rp_seg(kd))); }
    int kind = ramp_kind(ch), idx, n; ramp_run(x, y, kind, r, &idx, &n);
    static const u16 TOPC[3] = { C15(19, 13, 8), C15(8, 13, 21), C15(13, 13, 15) }, SIDC[3] = { C15(16, 11, 6), C15(6, 9, 16), C15(19, 13, 8) };
    int S = rp_seg(kind), X0 = tl(x * 2), Z0 = tl(y * 2), RW_ = 2 * T;
    static const s8 ax[4] = { 1, 0, -1, 0 }, az[4] = { 0, -1, 0, 1 };
    int sideA = !same_ramp(x - ax[r], y - az[r], kind, r), sideB = !same_ramp(x + ax[r], y + az[r], kind, r);
    const s16 *hc = rp_hc[kind][n - 1][idx];
    #define RP(A, B, H, o) do { int u_, v_; switch (r & 3) { case 0: u_ = A; v_ = B; break; case 1: u_ = B; v_ = 64 - (A); break; case 2: u_ = 64 - (A); v_ = 64 - (B); break; default: u_ = 64 - (B); v_ = A; } o[0] = X0 + u_ * RW_ / 64; o[1] = H; o[2] = Z0 + v_ * RW_ / 64; } while (0)
    for (int s = 0; s < S; s++) { int B0 = 64 * s / S, B1 = 64 * (s + 1) / S, h0 = base - hc[s] * 12 / 5, h1 = base - hc[s + 1] * 12 / 5, p[4][3];
        RP(0, B0, h0, p[0]); RP(64, B0, h0, p[1]); RP(64, B1, h1, p[2]); RP(0, B1, h1, p[3]);
        qd_c((const int (*)[3])p, TOPC[kind], &rp_nt[kind][n - 1][idx][s][r]);
        if (sideA && (h0 < base || h1 < base)) { RP(0, B1, base, p[0]); RP(0, B0, base, p[1]); RP(0, B0, h0, p[2]); RP(0, B1, h1, p[3]); qd_c((const int (*)[3])p, SIDC[kind], &rp_ns[kind][n - 1][idx][s][r][0]); }
        if (sideB && (h0 < base || h1 < base)) { RP(64, B0, base, p[0]); RP(64, B1, base, p[1]); RP(64, B1, h1, p[2]); RP(64, B0, h0, p[3]); qd_c((const int (*)[3])p, SIDC[kind], &rp_ns[kind][n - 1][idx][s][r][1]); } }
    if (idx == 0) { int h = base - hc[0] * 12 / 5, p[4][3]; RP(0, 0, base, p[0]); RP(64, 0, base, p[1]); RP(64, 0, h, p[2]); RP(0, 0, h, p[3]); qd_c((const int (*)[3])p, shade(SIDC[kind], 6), &rp_nb[r]);
        if (kind == 2) { RP(0, -2, h - 14, p[0]); RP(64, -2, h - 14, p[1]); RP(64, 3, h - 14, p[2]); RP(0, 3, h - 14, p[3]); qd_c((const int (*)[3])p, C15(24, 24, 26), &rp_ncop[r]); } }   // (a quarter pipe's coping)
    #undef RP
}
volatile int sk_cyc;
