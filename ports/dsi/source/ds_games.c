// ds_games.c - the PS1's mini-games on the DS (included by main.c, after the lot, the sims and the 2D helpers it uses).
// D48: BOREAGOTCHI (src/gotchi.c, gotchi_ui.c): a toy with eight creatures.
// D47: THE UNDERWORLD (src/heist_ui.c): the heist.
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
typedef struct { s8 ok, idx, n, kind, r; u8 c; s16 h; } SkCell;   // (D51) what a cell is, worked out once (the physics asked for it ten times a step)
static SkCell sk_cc[GN][GN]; static int sk_cc_key = -1;
static int sk_deck_c(int c);
static const SkCell *sk_cc_get(int mx, int my) {
    int key = nobj * 977 + lot_bx * 31 + lot_by * 7 + zone * 3 + (occ_ok ? 0 : 1); if (key != sk_cc_key) { sk_cc_key = key; memset(sk_cc, 0, sizeof sk_cc); }
    SkCell *e = &sk_cc[my][mx]; if (e->ok) return e;
    int c = sk_ch(mx, my); e->c = (u8)c; e->ok = 1; e->kind = 0; e->idx = e->n = e->r = 0; e->h = 0;
    if (is_ramp_c(c)) { int kind = ramp_kind(c), r = sk_face(mx, my), idx, n; ramp_run(mx, my, kind, r, &idx, &n); e->kind = (s8)kind; e->r = (s8)r; e->idx = (s8)idx; e->n = (s8)n; }
    else if (sk_deck_c(c)) e->h = (s16)sk_ih(c);
    return e; }
static int ramp_at(int mx, int my, int lx, int lv) { if (mx < 0 || my < 0 || mx >= GN || my >= GN) return 0; const SkCell *e = sk_cc_get(mx, my); if (!is_ramp_c(e->c)) return 0;
    int B = CLAMP(ramp_localB(e->r, CLAMP(lx, 0, 255), CLAMP(lv, 0, 255)), 0, 64); return ramp_prof(e->kind, e->n, e->n * 64 - (e->idx * 64 + B)); }
static int ramp_lip(int mx, int my, int *dir) { int c = sk_ch(mx, my); if (!is_ramp_c(c)) return 0;
    int kind = ramp_kind(c), r = sk_face(mx, my), idx, n; ramp_run(mx, my, kind, r, &idx, &n); *dir = r; return idx == 0 ? (kind == 2 ? 2 : 1) : 0; }
static int deck_c(int c) { return c == 'X' || c == 'M' || c == 'L' || c == 'N' || c == 'Z'; }
static int sk_deck_c(int c) { return deck_c(c); }
static int ground_at(int x, int z, int y) {   // the ground under the board (PS1 units)
    int mx = fdiv(x, 256), my = fdiv(-z, 256); if (mx < 0 || my < 0 || mx >= GN || my >= GN) return 0;
    const SkCell *e = sk_cc_get(mx, my); int c = e->c, r = ramp_at(mx, my, x - mx * 256, -z - my * 256); if (r) return r;
    if (deck_c(c)) { int h = e->h; if (y >= h - 24 || h <= 60) return h; }
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

// ==== ARCADE (src/arcade.c): the cabinets' games and the prize counter, as the PS1's. The thirteen games are the PS1's own code (ds_agames.c, built from src/ag_*.c); the field is the PS1's 240 x 160,
// drawn 1:1 on the top screen; the bottom is the stylus pad and the buttons. BORTUA FIGHTER's cabinet comes with the fight (wip-ds-fight) ====
enum { AG_FIGHTER, AG_BREAKER, AG_CATCH, AG_WHACK, AG_STRIKER, AG_BLOCKS, AG_SHMUP, AG_RACER, AG_MAZE, AG_HOOPS, AG_SKEE, AG_HOCKEY, AG_RHYTHM, AG_CLAW, AG_NGAME, AG_PRIZES = 99 };
enum { PD_L = 1, PD_R = 2, PD_U = 4, PD_D = 8, PD_X = 16, PD_O = 32, PD_ST = 64 };
#define PADB_UP 16
#define PADB_RIGHT 32
#define PADB_DOWN 64
#define PADB_LEFT 128
#define PADB_START 8
#define PADB_TRI 4096
#define PADB_CIR 8192
#define PADB_CRO 16384
#define PADB_SQU 32768
#define K8(r, g, b) C15((r) >> 3, (g) >> 3, (b) >> 3)
typedef struct { void (*start)(void); int (*tick)(int, int, int, int); void (*draw)(void); } DSGame;
extern const DSGame ds_ag_fn[AG_NGAME];
extern int ag_pts, ag_t, ag_lives, ag_combo, ag_best, ag_perf, ag_np, ag_foe, ag_shake, ag_level;
void dsag_fx_reset(void), dsag_fx_tick(void), dsag_fx_draw(void);
int dsag_tx = -1, dsag_tap = -1;
static int dsag_ox, dsag_oy;
static u16 c8(u32 c) { return C15((c & 255) >> 3, ((c >> 8) & 255) >> 3, ((c >> 16) & 255) >> 3); }
void dsag_rect(int x, int y, int w, int h, u32 c, int semi) {   // the field's own pixels (0..239, 0..159); nothing leaves the field
    if (x < 0) { w += x; x = 0; } if (y < 0) { h += y; y = 0; } if (x + w > 240) w = 240 - x; if (y + h > 160) h = 160 - y; if (w <= 0 || h <= 0) return;
    u16 cc = c8(c); x += 8 + dsag_ox; y += 16 + dsag_oy;
    if (!semi) { rect(x, y, w, h, cc); return; }
    if (x < 0) { w += x; x = 0; } if (y < 0) { h += y; y = 0; } if (x + w > 256) w = 256 - x; if (y + h > 192) h = 192 - y; if (w <= 0 || h <= 0) return;
    cc = (u16)((cc & 0x7BDE) >> 1);
    for (int j = 0; j < h; j++) { u16 *p = scr + (y + j) * 256 + x; for (int i = 0; i < w; i++) p[i] = (u16)(((p[i] & 0x7BDE) >> 1) + cc) | 0x8000; }
}
void dsag_disc(int cx, int cy, int r, u32 c) { if (cx + r < 0 || cx - r > 240 || cy + r < 0 || cy - r > 160) return; ellf(8 + dsag_ox + cx, 16 + dsag_oy + cy, r, r, c8(c)); }
void dsag_text(int x, int y, int sz, const char *s, u32 c) {   // (the PS1's button names become the DS's: X is A, O is B, TRI is X, SQ is Y; letters a pixel closer so a line fits the field)
    if (y < -8 || y > 160) return; char b[48]; int n = 0;
    for (const char *p = s; *p && n < 44;) { if ((p == s || p[-1] == ' ') && (p[1] == 0 || p[1] == ' ' || p[1] == '.' || p[1] == ',')) { if (*p == 'X') { b[n++] = 'A'; p++; continue; } if (*p == 'O') { b[n++] = 'B'; p++; continue; } }
        if ((p == s || p[-1] == ' ') && !strncmp(p, "TRI", 3) && (p[3] == 0 || p[3] == ' ')) { b[n++] = 'X'; p += 3; continue; }
        if ((p == s || p[-1] == ' ') && !strncmp(p, "SQ", 2) && (p[2] == 0 || p[2] == ' ')) { b[n++] = 'Y'; p += 2; continue; }
        b[n++] = *p++; }
    b[n] = 0; t_tight = 1; text(8 + dsag_ox + x, 16 + dsag_oy + y + 2, sz, c8(c), b); t_tight = 0; }
static int cue_gap;
void dsag_cue(int k) {
    if (cue_gap > 0 && k != 3) return; cue_gap = 6;
    switch (k) { case 0: sfx(1); break; case 1: sfx(0); break; case 2: sfx(6); break; case 3: sfx(5); break; default: sfx(2); break; }
}
static const char *const agName[AG_NGAME] = { "BORTUA FIGHTER", "BORE BREAKER", "SNACK CATCH", "WHACK-A-BORE", "HIGH STRIKER", "BORE BLOCKS", "STAR BORE", "TURBO BORE", "MUNCH MAZE", "HOOPS", "SKEE-BORE", "AIR HOCKEY", "BEAT BORE", "CLAW GRAB" };
static const char *const agTitle[AG_NGAME] = { "Bortua Fighter", "Bore Breaker", "Snack Catch", "Whack-a-Bore", "High Striker", "Bore Blocks", "Star Bore", "Turbo Bore", "Munch Maze", "Hoops", "Skee-Bore", "Air Hockey", "Beat Bore", "Claw Grab" };
static const u8 agPrice[AG_NGAME] = { 2, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 2, 1, 1 };
static const u16 agPar[AG_NGAME] = { 60, 120, 60, 40, 400, 600, 2000, 3000, 2000, 40, 260, 160, 6000, 12 };
static const u8 agCol[AG_NGAME][3] = { { 230, 60, 70 }, { 60, 200, 250 }, { 250, 170, 50 }, { 120, 220, 90 }, { 250, 120, 40 }, { 170, 90, 250 }, { 250, 80, 170 }, { 250, 220, 50 }, { 250, 230, 60 }, { 250, 130, 60 }, { 90, 220, 160 }, { 90, 160, 255 }, { 250, 90, 220 }, { 255, 120, 190 } };
static const char *const agHow1[AG_NGAME] = { "", "SLIDE THE STYLUS: THE PADDLE FOLLOWS", "SLIDE THE STYLUS: THE BASKET FOLLOWS", "TAP A HOLE, OR D-PAD AND A", "TAP A: STOP THE METER", "D-PAD MOVE. A TURNS. UP DROPS", "D-PAD FLY. A SHOOT. B BOMB", "LEFT RIGHT STEER. A GAS. B NITRO", "D-PAD TURNS. EAT EVERY DOT", "LEFT RIGHT MOVE. A STOPS THE METER", "LEFT RIGHT AIM. HOLD A, LET GO", "D-PAD MOVES YOUR MALLET", "D-PAD OR A B X Y ON THE LINE", "LEFT RIGHT MOVE. A DROPS" };
static const char *const agHow2[AG_NGAME] = { "", "BREAK THE WALL. CATCH POWER-UPS", "CATCH SNACKS. DODGE ROCKS", "GOLD BORES PAY MORE. NO BOMBS", "HIT THE SWEET SPOT. RING THE BELL", "FILL LINES. THE FLOOR RISES", "SHOOT THE FLEET. BEAT THE BOSS", "BEAT THE CLOCK. DODGE THE TRAFFIC", "BIG DOTS MAKE GHOSTS SCARED", "SINK IT. KEEP A STREAK. GET HOT", "ROLL FOR THE HIGH RINGS", "FIRST TO SEVEN", "KEEP THE GROOVE UP. BUILD A COMBO", "GRAB A PLUSH AND BRING IT HOME" };
static int arcade_game_at(int x, int y) { return 1 + (x * 3 + y * 2) % (AG_NGAME - 1); }   // (no fighter yet: 1..13)
static const char *ag_title(int g) { return agTitle[g % AG_NGAME]; }
static int ag_price(int g) { return agPrice[g % AG_NGAME]; }
#ifdef DSTEST
static int ag_testlot;
#endif
static int in_arcade(void) { BInfo b; blk(lot_bx, lot_by, &b);
#ifdef DSTEST
    if (ag_testlot) return 1;
#endif
    return b.lot == LOT_ARCADE; }
// the high-score tables ('SA' in the save): per game three of { score (17 bits), three initials (5 bits each, 1 = A) }, the first-play bits, the games played
#define SC_(e) ((int)((e) & 0x1FFFF))
static u32 ar_hs[AG_NGAME][3]; static u16 ar_pm, ar_plays;
static void ini_str(u32 e, char *o) { for (int i = 0; i < 3; i++) { int v = (int)((e >> (17 + i * 5)) & 31); o[i] = (char)(v >= 1 && v <= 26 ? 'A' + v - 1 : '-'); } o[3] = 0; }
static int hs_rank(int g, int sc) { for (int i = 0; i < 3; i++) if (sc > SC_(ar_hs[g][i])) return i; return -1; }
static void hs_put(int g, int k, int sc, const char *ini) {
    for (int i = 2; i > k; i--) ar_hs[g][i] = ar_hs[g][i - 1];
    u32 e = (u32)MIN(sc, 0x1FFFF); for (int i = 0; i < 3; i++) { int c = ini[i]; e |= (u32)((c >= 'A' && c <= 'Z' ? c - 'A' + 1 : 0) << (17 + i * 5)); } ar_hs[g][k] = e;
}
static int ag, ag_phase, ag_acc, ag_tix, ag_psel, ag_dirty, ag_rt, ag_hrank, ag_ic, ag_over; static char ag_ini[4];
static const char *ag_msg = "";
static int ag_held_id;
static void ag_start(void) {
    ag_pts = 0; ag_t = 0; ag_lives = 3; ag_combo = 0; ag_best = 0; ag_perf = 1; ag_level = 1; ag_shake = 0; ag_np = 1; ag_tix = 0; dsag_fx_reset(); dsag_ox = dsag_oy = 0;
    ds_ag_fn[ag].start();
}
typedef struct { const char *nm; u16 cost; s8 fun, food, social; int cash; const char *say; } Prize;
static const Prize prz[] = {
    { "CANDY", 10, 5, 15, 0, 0, "SUGAR RUSH!" }, { "SODA POP", 15, 8, 20, 0, 0, "FIZZY!" }, { "BOUNCY BALL", 25, 15, 0, 5, 0, "BOING BOING BOING" },
    { "SPARKLER", 40, 18, 0, 8, 0, "PRETTY LIGHTS" }, { "PLUSH TEDDY", 60, 25, 0, 10, 0, "IT HAS A TINY HAT" }, { "BOARD GAME", 90, 22, 0, 25, 0, "GAME NIGHT!" },
    { "PARTY HAT", 120, 20, 0, 30, 0, "LOOKING FESTIVE" }, { "MEGA TROPHY", 150, 30, 0, 30, 0, "NUMBER ONE!" }, { "GIANT PLUSH", 250, 40, 0, 20, 0, "BIGGER THAN YOU" },
    { "GOLDEN TICKET", 400, 10, 0, 0, 500, "TRADED IN FOR $500!" } };
#define NPRZ ((int)(sizeof prz / sizeof prz[0]))
static void need_add(Sim *s, int n, int v) { s->need[n] = cl100(s->need[n] + v); }
static void ag_open(int g) { ag = g; ag_phase = 0; ag_acc = 0; ag_psel = 0; ag_msg = ""; ag_rt = 0; dsag_tx = dsag_tap = -1; ag_held_id = 0; mg_ui = g == AG_PRIZES ? 2 : 1; mg_ver++; }
static void ag_finish(void) {   // a game over: the tickets, fun, a little sleepier
    Sim *s = &sims[ctl]; static char b[48];
    arcade_tix = MIN(9999, arcade_tix + ag_tix);
    need_add(s, 1, MIN(45, 16 + ag_tix / 2)); need_add(s, 2, -3); life_ev(ctl, WN_FUN, 0);
    snprintf(b, sizeof b, "+%d tickets (%d)", ag_tix, arcade_tix); say(b);
    voice(ag_tix >= 8 ? V_YAHOO : ag_tix ? V_LAUGH : V_NAH);
}
static int ag_tix_calc(int g, int pts) {   // the PS1's: par-based, the combo, a clean run, a first play bonus
    int t = pts * 12 / agPar[g]; if (t > 30) t = 30 + (t - 30) / 2; t += ag_best / 4; if (ag_perf && pts > agPar[g] / 2) t += 6;
    t = CLAMP(t, 0, 80); if (!(ar_pm & (1 << g))) { t += 10; ar_pm |= (u16)(1 << g); } return t;
}
static void ag_over_now(void) {
    ag_phase = 2; ag_acc = 0; ag_over = 0; ag_tix = ag_tix_calc(ag, ag_pts); ag_hrank = hs_rank(ag, ag_pts); if (ar_plays < 60000) ar_plays++;
    voice(ag_tix >= 12 ? V_YAHOO : V_LAUGH);
}
static void ag_ini_begin(void) { const char *f = sname[ctl]; for (int i = 0; i < 3; i++) { char c = f[i]; ag_ini[i] = (char)((c >= 'A' && c <= 'Z') ? c : (c >= 'a' && c <= 'z') ? c - 32 : 'A' + i); } ag_ini[3] = 0; ag_ic = 0; }
static int ag_update(int hd, int pr, int steps, int quit) {   // 1 while it runs (arcade_update)
    if (ag == AG_PRIZES) {
        if (pr & PD_U) ag_psel = (ag_psel + NPRZ - 1) % NPRZ;
        if (pr & PD_D) ag_psel = (ag_psel + 1) % NPRZ;
        if (pr & PD_O) return 0;
        if (pr & PD_X) {
            const Prize *p = &prz[ag_psel];
            if (arcade_tix < p->cost) { ag_msg = "NOT ENOUGH TICKETS YET"; voice(V_NAH); return 1; }
            Sim *s = &sims[ctl]; arcade_tix -= p->cost; static char b[48];
            need_add(s, 1, p->fun); need_add(s, 0, p->food); need_add(s, 3, p->social); funds += p->cash;
            snprintf(b, sizeof b, "%s", p->say); say(b); voice(V_YAHOO); life_ev(ctl, WN_FUN, 0);
            return 0;
        }
        return 1;
    }
    if (ag_phase == 0) { ag_rt += steps; if (pr & PD_X) { ag_phase = 1; ag_start(); } if (pr & PD_O) { funds += ag_price(ag); return 0; } return 1; }   // (backing out: the money back)
    if (ag_phase == 2) { ag_acc += steps; ag_over++; if (ag_acc > 40 && (pr & (PD_X | PD_O))) { ag_finish(); if (ag_hrank >= 0) { ag_phase = 3; ag_ini_begin(); return 1; } return 0; } return 1; }
    if (ag_phase == 3) {
        if (pr & PD_U) ag_ini[ag_ic] = (char)(ag_ini[ag_ic] == 'Z' ? 'A' : ag_ini[ag_ic] + 1);
        if (pr & PD_D) ag_ini[ag_ic] = (char)(ag_ini[ag_ic] == 'A' ? 'Z' : ag_ini[ag_ic] - 1);
        if ((pr & PD_L) && ag_ic > 0) ag_ic--; if ((pr & PD_R) && ag_ic < 2) ag_ic++;
        if (pr & PD_X) { if (ag_ic < 2) ag_ic++; else { hs_put(ag, ag_hrank, ag_pts, ag_ini); return 0; } }
        if (pr & PD_O) { if (ag_ic > 0) ag_ic--; else { hs_put(ag, ag_hrank, ag_pts, ag_ini); return 0; } }
        return 1;
    }
    if (quit) { ag_over_now(); ag_tix = 0; ag_hrank = -1; return 1; }   // (quitting: no tickets)
    ag_acc += steps;
    while (ag_acc >= 2) { ag_acc -= 2; if (cue_gap > 0) cue_gap--;
        int k = ds_ag_fn[ag].tick(hd, pr, 0, 0); pr = 0; ag_t++; dsag_fx_tick(); ag_dirty = 1; if (!k) { ag_over_now(); break; } }
    return 1;
}
// ---- the picture: the field on the top screen, the pad and the buttons on the bottom ----
static void ctext(int cx, int y, int s, u16 c, const char *t) { text(cx - tw(s, t) / 2, y, s, c, t); }
static void ar_scores(int g, int y0) {
    char b[40], n[4]; ctext(128, y0, 1, K8(255, 190, 40), "HIGH SCORES");
    for (int i = 0; i < 3; i++) { ini_str(ar_hs[g][i], n); if (SC_(ar_hs[g][i])) snprintf(b, sizeof b, "%d  %s  %d", i + 1, n, SC_(ar_hs[g][i])); else snprintf(b, sizeof b, "%d  ---  0", i + 1); ctext(128, y0 + 18 + i * 16, 1, i == 0 ? K8(255, 190, 40) : TXT, b); }
}
static void ar_top(void) {
    scr = offbuf; char b[40]; rect(0, 0, 256, 192, K8(16, 10, 34)); dsag_ox = dsag_oy = 0;
    if (mg_ui == 2) {
        panel(8, 8, 240, 160, "The Prize Counter");
        for (int i = 0; i < NPRZ; i++) { int y = 26 + i * 13, s = i == ag_psel; rect(14, y, 228, 12, s ? K8(226, 242, 254) : PANB); rect(15, y + 1, 226, 10, s ? C15(24, 28, 31) : PAN);
            text(20, y + 3, 1, s ? C15(2, 7, 20) : TXT, prz[i].nm); sprintf(b, "%d tix", prz[i].cost); text(236 - tw(1, b), y + 3, 1, arcade_tix >= prz[i].cost ? (s ? C15(2, 14, 4) : C15(14, 30, 14)) : C15(31, 12, 10), b); }
        sprintf(b, "You have %d tickets", arcade_tix); ctext(128, 158, 1, C15(24, 28, 31), b); if (ag_msg[0]) ctext(128, 176, 1, C15(31, 14, 12), ag_msg);
        return; }
    rect(6, 14, 244, 164, K8(agCol[ag][0] / 3, agCol[ag][1] / 3, agCol[ag][2] / 3 + 20));
    rect(8, 1, 240, 12, K8(60, 30, 110)); ctext(128, 4, 1, K8(agCol[ag][0], agCol[ag][1], agCol[ag][2]), agName[ag]);
    dsag_rect(0, 0, 240, 160, 0x120404u, 0);
    if (ag_phase == 0) {
        int pg = (ag_rt / 100) % 2;
        if (pg == 0) { ctext(128, 40, 2, K8(agCol[ag][0], agCol[ag][1], agCol[ag][2]), agName[ag]); ctext(128, 80, 1, TXT, agHow1[ag]); ctext(128, 96, 1, TXT, agHow2[ag]);
            sprintf(b, "PLAY $%d", ag_price(ag)); ctext(128, 128, 1, K8(255, 190, 40), b); ctext(128, 146, 1, K8(170, 210, 250), (ag_rt / 15) & 1 ? "PRESS A TO START" : ""); }
        else { ctext(128, 40, 2, TXT, agName[ag]); ar_scores(ag, 72); }
    } else if (ag_phase == 3) {
        ctext(128, 30, 2, K8(255, 190, 40), "NEW RECORD!"); sprintf(b, "SCORE %d", ag_pts); ctext(128, 56, 1, TXT, b);
        for (int i = 0; i < 3; i++) { char c[2] = { ag_ini[i], 0 }; int x = 84 + i * 28; rect(8 + x, 16 + 70, 22, 30, i == ag_ic ? K8(250, 200, 60) : K8(60, 70, 110)); rect(8 + x + 1, 16 + 71, 20, 28, K8(10, 10, 30)); text(8 + x + 5, 16 + 77, 2, TXT, c); }
        ctext(128, 128, 1, K8(170, 210, 250), "UP DOWN PICKS. A NEXT");
    } else {
        if (ag_shake > 0) { dsag_ox = (ag_shake & 1) * 3 - 1; dsag_oy = ((ag_shake >> 1) & 1) * 2; }
        ds_ag_fn[ag].draw(); dsag_fx_draw(); dsag_ox = dsag_oy = 0;
        if (ag_phase == 2) { rect(56, 68, 144, 80, K8(10, 12, 30)); rect(58, 70, 140, 76, K8(20, 22, 44)); ctext(128, 76, 2, K8(250, 210, 40), "GAME OVER");
            sprintf(b, "SCORE %d", ag_pts); ctext(128, 100, 1, TXT, b); sprintf(b, "+%d TICKETS", ag_tix); ctext(128, 116, 1, C15(14, 30, 14), b); if (ag_hrank >= 0) ctext(128, 130, 1, K8(255, 190, 40), ag_hrank == 0 ? "NEW BEST!" : "HIGH SCORE!"); }
    }
    sprintf(b, "TICKETS %d", arcade_tix); ctext(128, 181, 1, K8(200, 210, 240), b);
}
#define AQ_X 196
#define AQ_Y 164
// the bottom screen's buttons while a game runs: the strip (8,22,240,26), the D-pad, the four face buttons, Quit
static const struct { s16 x, y, w, h; u8 id; const char *t; } BTN[] = {
    { 52, 54, 44, 30, 3, "UP" }, { 8, 86, 44, 36, 1, "LEFT" }, { 96, 86, 44, 36, 2, "RIGHT" }, { 52, 124, 44, 30, 4, "DOWN" },
    { 176, 54, 44, 30, 7, "X" }, { 148, 86, 44, 36, 8, "Y" }, { 204, 86, 44, 36, 5, "A" }, { 176, 124, 44, 30, 6, "B" } };
static void ar_bottom(void) {
    scr = offbuf; char b[32]; rect(0, 0, 256, 192, K8(24, 16, 54)); rect(0, 0, 256, 16, K8(60, 30, 110)); ctext(128, 4, 1, K8(90, 230, 240), mg_ui == 2 ? "THE PRIZE COUNTER" : agName[ag]);
    if (mg_ui == 2) {
        const Prize *p = &prz[ag_psel]; chip(8, 24, 116, 44, "Up", 0); chip(132, 24, 116, 44, "Down", 0);
        rect(8, 76, 240, 70, PANB); rect(9, 77, 238, 68, PAN); ctext(128, 86, 2, C15(2, 7, 20), p->nm); sprintf(b, "%d tickets", p->cost); ctext(128, 108, 1, arcade_tix >= p->cost ? C15(2, 14, 4) : C15(31, 12, 10), b); ctext(128, 124, 1, TXT, p->say);
        chip(8, 154, 116, 32, "Swap", 1); chip(132, 154, 116, 32, "Never mind", 0); return; }
    if (ag_phase == 0) { sprintf(b, "Play $%d", ag_price(ag)); chip(14, 70, 108, 54, b, 1); chip(134, 70, 108, 54, "Money back", 0); ctext(128, 34, 1, TXT, agHow1[ag]); ctext(128, 48, 1, TXT, agHow2[ag]); return; }
    if (ag_phase == 2) { chip(30, 60, 196, 70, "Done", 1); return; }
    if (ag_phase == 3) { chip(8, 24, 116, 44, "Up", 0); chip(132, 24, 116, 44, "Down", 0); chip(8, 76, 116, 50, "Back", 0); chip(132, 76, 116, 50, ag_ic < 2 ? "Next" : "OK", 1); ctext(128, 140, 1, TXT, "A OR OK TO SAVE ON THE LAST"); return; }
    chip(AQ_X, AQ_Y, 56, 22, "Quit", 0);
    if (ag == AG_WHACK) { for (int i = 0; i < 9; i++) { int x = 8 + (i % 3) * 64, y = 22 + (i / 3) * 46; rect(x, y, 60, 42, K8(60, 40, 110)); rect(x + 2, y + 2, 56, 38, K8(30, 20, 60)); } ctext(100, 166, 1, TXT, "TAP A HOLE"); return; }
    rect(8, 22, 240, 26, K8(40, 28, 90)); rect(10, 24, 236, 22, K8(30, 20, 70)); ctext(128, 31, 1, K8(170, 190, 250), "SLIDE THE STYLUS HERE");
    for (int i = 0; i < 8; i++) chip(BTN[i].x, BTN[i].y, BTN[i].w, BTN[i].h, BTN[i].t, ag_held_id == BTN[i].id);
}
static unsigned ar_bsig(void) { return (unsigned)(mg_ui * 3 + ag_phase * 5 + ag * 7 + ag_psel * 13 + ag_ic * 19 + ag_held_id * 29 + (arcade_tix > 0) * 23); }
static void ar_frame(int held, int down, int steps, int tx, int ty) {
    int hd = 0, pr = 0, tch = held & KEY_TOUCH, td = down & KEY_TOUCH; dsag_tx = -1; ag_dirty = 0;
    if (held & KEY_LEFT) hd |= PADB_LEFT; if (held & KEY_RIGHT) hd |= PADB_RIGHT; if (held & KEY_UP) hd |= PADB_UP; if (held & KEY_DOWN) hd |= PADB_DOWN;
    if (held & KEY_A) hd |= PADB_CRO; if (held & KEY_B) hd |= PADB_CIR; if (held & KEY_X) hd |= PADB_TRI; if (held & KEY_Y) hd |= PADB_SQU;
    if (down & KEY_LEFT) pr |= PADB_LEFT; if (down & KEY_RIGHT) pr |= PADB_RIGHT; if (down & KEY_UP) pr |= PADB_UP; if (down & KEY_DOWN) pr |= PADB_DOWN;
    if (down & KEY_A) pr |= PADB_CRO; if (down & KEY_B) pr |= PADB_CIR; if (down & KEY_X) pr |= PADB_TRI; if (down & KEY_Y) pr |= PADB_SQU; if (down & KEY_START) pr |= PADB_START;
    int fl = 0;   // the menu keys (PD_*) from the same presses
    if (pr & PADB_LEFT) fl |= PD_L; if (pr & PADB_RIGHT) fl |= PD_R; if (pr & PADB_UP) fl |= PD_U; if (pr & PADB_DOWN) fl |= PD_D; if (pr & PADB_CRO) fl |= PD_X; if (pr & PADB_CIR) fl |= PD_O; if (pr & PADB_START) fl |= PD_ST;
    int id = 0;
    if (mg_ui == 2) {
        if (td) { if (hit(8, 24, 116, 44, tx, ty)) fl |= PD_U; if (hit(132, 24, 116, 44, tx, ty)) fl |= PD_D; if (hit(8, 154, 116, 32, tx, ty)) fl |= PD_X; if (hit(132, 154, 116, 32, tx, ty)) fl |= PD_O; }
    } else if (ag_phase == 0) { if (td) { if (hit(14, 70, 108, 54, tx, ty)) fl |= PD_X; if (hit(134, 70, 108, 54, tx, ty)) fl |= PD_O; } }
    else if (ag_phase == 2) { if (td) fl |= PD_X; }
    else if (ag_phase == 3) { if (td) { if (hit(8, 24, 116, 44, tx, ty)) fl |= PD_U; if (hit(132, 24, 116, 44, tx, ty)) fl |= PD_D; if (hit(8, 76, 116, 50, tx, ty)) fl |= PD_O; if (hit(132, 76, 116, 50, tx, ty)) fl |= PD_X; } }
    else {
        if (tch) {
            if (hit(AQ_X, AQ_Y, 56, 22, tx, ty)) { if (td) fl |= PD_ST; }
            else if (ag == AG_WHACK) { if (td) for (int i = 0; i < 9; i++) if (hit(8 + (i % 3) * 64, 22 + (i / 3) * 46, 60, 42, tx, ty)) dsag_tap = i; }
            else {
                if (hit(8, 22, 240, 26, tx, ty)) { dsag_tx = tx; id = 9; }
                for (int i = 0; i < 8; i++) if (hit(BTN[i].x, BTN[i].y, BTN[i].w, BTN[i].h, tx, ty)) id = BTN[i].id;
                static const int bit[10] = { 0, PADB_LEFT, PADB_RIGHT, PADB_UP, PADB_DOWN, PADB_CRO, PADB_CIR, PADB_TRI, PADB_SQU, 0 };
                hd |= bit[id]; if (id != ag_held_id) pr |= bit[id];
            }
        }
        if (id != ag_held_id) { ag_held_id = id; mg_ver++; }
    }
    int old = ag_phase, op = ag_psel, oi = ag_ic, play = ag_phase == 1 && mg_ui == 1;
    if (!(play ? ag_update(hd, pr & ~PADB_START, steps, (fl & PD_ST) != 0) : ag_update(0, fl, steps, 0))) { mg_ui = 0; mg_ver++; return; }
    if (ag_phase == 0 && ag_rt % 15 < 2) mg_ver++;
    if (ag_dirty || fl || old != ag_phase || op != ag_psel || oi != ag_ic || dsag_tx >= 0) mg_ver++;
}

// ==== THE UNDERWORLD (src/heist_ui.c): GRAND THEFT. Pick a ride, sneak through three gates (a gauge each), run for it if the alarm went; keep it or fence it. Heat cools with the days ====
enum { H_LIST, H_GATE, H_CHOICE, H_CHASE, H_END };
#define HT_N 10
static const char *const hTNM[HT_N] = { "HATCHBACK", "PICKUP TRUCK", "SPORTS CAR", "MOTORBIKE", "POLICE CRUISER", "SPEEDBOAT", "YACHT", "BIPLANE", "SEAPLANE", "JET" };
static const char *const hTWH[HT_N] = { "CAR LOT", "CAR LOT", "CAR LOT", "CAR LOT", "POLICE YARD", "HARBOUR", "HARBOUR", "AIRFIELD", "AIRFIELD", "AIRFIELD" };
static const u8 hTDF[HT_N] = { 1, 1, 3, 2, 4, 3, 5, 4, 5, 6 }, hTST[HT_N] = { 1, 1, 2, 2, 3, 2, 3, 3, 3, 5 };
static const int hTVAL[HT_N] = { 400, 700, 2500, 900, 1800, 3500, 12000, 6000, 9000, 30000 };
static const char *const hGNM[3] = { "THE FENCE", "THE GUARD", "THE LOCK" };
static const char *const hGHINT[3] = { "CLIMB WHEN THE LIGHT IS AWAY.", "SLIP PAST WHEN HE LOOKS OFF.", "PICK IT IN THE QUIET MOMENT." };
static int hs_heat, hs_day, hs_stolen, hs_logic;   // saved ('HT'): the heat, the day it was last cooled, the boats and aircraft kept (a bit each), the logic points
static int hp, hsel, hgate, halarm, htgt, hstars, hround, hmiss, hmark, hdir, hzone, hzw, htmr, hanim; static char hline[48], hline2[48];
static unsigned hrs = 9;
static int hrnd(int n) { hrs ^= hrs << 13; hrs ^= hrs >> 17; hrs ^= hrs << 5; return (int)((hrs >> 3) % (unsigned)n); }
static int hlg(void) { int l = hs_logic / 400; return l > 10 ? 10 : l; }
static void hs_open(void) {
    hp = H_LIST; hsel = 0; hline[0] = hline2[0] = 0; hanim = 0; hrs = (unsigned)(funds * 7 + st_days * 31 + 11); mg_ui = 3; mg_ver++;
    if (hs_day != (u8)st_days) { int d = (u8)st_days - hs_day; if (d < 0) d += 256; hs_heat = MAX(0, hs_heat - d * 12); hs_day = (u8)st_days; } }
static void hnew_zone(int w) { hzw = w < 8 ? 8 : w; hzone = 10 + hrnd(80 - hzw); hmark = hrnd(100); hdir = hrnd(2) ? 1 : -1; htmr = 0; }
static void hbusted(const char *why) {   // (the PS1 puts it on the record and sends you to court; here you pay a fine, are shaken, and the heat rises)
    int fine = MIN(funds, 100 * hstars * (1 + hstars / 2)); funds -= fine;
    hs_heat = MIN(100, hs_heat + 20); { Sim *s = &sims[ctl]; s->need[1] = cl100(s->need[1] - 30); s->need[3] = cl100(s->need[3] - 15); }
    say("Arrested!"); hp = H_END; strcpy(hline, why); sprintf(hline2, "FINED $%d. IT IS ON THE RECORD.", fine); voice(V_CRY);
}
static void hbegin(int t) {
    htgt = t; hgate = 0; halarm = 0; hstars = hTST[t] + (hs_heat > 50); hmiss = 0; hp = H_GATE; hline[0] = 0;
    hnew_zone(30 + hlg() * 2 - hTDF[t] * 3); voice(V_YAHOO);
}
static void hgot_away(void) {
    hs_heat = MIN(100, hs_heat + 10 + hstars * 6); hs_day = (u8)st_days; hs_logic += 30 + hTDF[htgt] * 10; hp = H_CHOICE; hsel = 0; hline[0] = 0;
    say("You got away with it!"); voice(V_WIN);
}
static void hfinish_gate(int ok) {
    if (ok) strcpy(hline, "CLEAN!"); else { halarm++; strcpy(hline, "A SLIP! SOMETHING CLANGED."); hstars = MIN(5, hstars + 1); }
    htmr = 40;
}
static int hs_update(int pr, int steps) {   // 1 while it runs (heist_update)
    hanim += steps;
    if (hp == H_LIST) {
        if (pr & PD_O) return 0;
        if (pr & PD_U) hsel = (hsel + HT_N - 1) % HT_N; if (pr & PD_D) hsel = (hsel + 1) % HT_N;
        if (pr & PD_X) { if (stage_of(sims[ctl].age) < 3) { strcpy(hline, "TOO YOUNG FOR THIS."); return 1; } hbegin(hsel); }
        return 1; }
    if (hp == H_GATE) {
        int sp = 2 + hgate + hTDF[htgt] / 2 - hlg() / 4; if (sp < 1) sp = 1;
        for (int i = 0; i < steps; i++) { hmark += hdir * sp; if (hmark > 99) { hmark = 99; hdir = -1; } if (hmark < 0) { hmark = 0; hdir = 1; } }
        if (htmr > 0) { htmr -= steps; if (htmr <= 0) {
            if (halarm >= 2) { hbusted("CAUGHT RED-HANDED!"); return 1; }
            if (++hgate >= 3) { if (halarm) { hp = H_CHASE; hround = 0; hmiss = 0; hnew_zone(36 + hlg() * 2 - hstars * 3); hline[0] = 0; strcpy(hline2, "THE ALARM IS RINGING! RUN!"); } else hgot_away(); }
            else hnew_zone(30 + hlg() * 2 - hTDF[htgt] * 3 - hgate * 2); }
            return 1; }
        if (pr & PD_X) { int d = ABS(hmark - (hzone + hzw / 2)); hfinish_gate(d <= hzw / 2 + 2); sfx(d <= hzw / 2 + 2 ? 1 : 0); }
        return 1; }
    if (hp == H_CHOICE) {
        if (pr & (PD_U | PD_D)) hsel ^= 1;
        if (pr & PD_X) {
            if (hsel == 0) { if (htgt >= 5) { hs_stolen |= 1 << htgt; sprintf(hline, "KEPT: THE %s (HOT).", hTNM[htgt]); } else { int pay = hTVAL[htgt] * (100 + hlg() * 3) / 200; funds += pay; sprintf(hline, "A CAR CAN'T BE KEPT HERE. SOLD $%d", pay); } }
            else { int pay = hTVAL[htgt] * (100 + hlg() * 3) / 100; funds += pay; sprintf(hline, "FENCED FOR $%d", pay); voice(V_WIN); }
            hp = H_END; strcpy(hline2, hs_heat > 60 ? "THE HEAT IS ON. LAY LOW." : "QUIET, FOR NOW."); life_ev(ctl, WN_CASH, 0); }
        return 1; }
    if (hp == H_CHASE) {
        int sp = 3 + hround + hstars; if (sp < 2) sp = 2;
        for (int i = 0; i < steps; i++) { hmark += hdir * sp; if (hmark > 99) { hmark = 99; hdir = -1; } if (hmark < 0) { hmark = 0; hdir = 1; } }
        if (htmr > 0) { htmr -= steps; if (htmr <= 0) { if (hmiss >= 2) hbusted("THE POLICE GOT YOU!"); else if (++hround >= hstars + 1) hgot_away(); else hnew_zone(36 + hlg() * 2 - hstars * 3 - hround); } return 1; }
        if (pr & PD_X) { int d = ABS(hmark - (hzone + hzw / 2)); if (d <= hzw / 2 + 2) strcpy(hline, "LOST THEM AT THE CORNER!"); else { hmiss++; strcpy(hline, "THEY ARE ON YOUR TAIL!"); } htmr = 40; }
        return 1; }
    if (pr & (PD_X | PD_O)) return 0;
    return 1;
}
static void hs_top(void) {
    scr = offbuf; char b[56]; rect(0, 0, 256, 192, K8(14, 14, 34));
    for (int i = 0; i < 6; i++) rect(10 + i * 40, 150 - (i * 37) % 50, 24, 60, K8(24, 24, 46));   // a skyline at night
    panel(8, 8, 240, 176, "Underworld"); int top = 38;
    if (hp == H_LIST) {
        text(16, top, 1, C15(20, 24, 28), "PICK A RIDE. ANY NIGHT."); sprintf(b, "HEAT %d", hs_heat); text(240 - tw(1, b), top, 1, hs_heat > 50 ? C15(31, 12, 10) : C15(14, 30, 14), b);
        text(16, top + 18, 2, TXT, hTNM[hsel]); sprintf(b, "%s   $%d", hTWH[hsel], hTVAL[hsel]); text(16, top + 40, 1, C15(14, 30, 14), b);
        sprintf(b, "SECURITY %d  WANTED %d  YOUR LOGIC %d", hTDF[hsel], hTST[hsel], hlg()); text(16, top + 56, 1, C15(24, 28, 31), b);
        text_fit(16, top + 74, 1, 224, C15(20, 24, 28), hsel >= 5 ? "KEPT: HOT. SKY AND SEA CAN TAKE IT OUT." : "A CAR OR BIKE: THE CHOP SHOP, FOR CASH.");
        if (hs_stolen & (1 << hsel)) text(16, top + 90, 1, C15(31, 25, 6), "YOU KEEP ONE OF THESE ALREADY.");
        text(16, top + 108, 1, C15(31, 12, 10), hline); return; }
    if (hp == H_GATE || hp == H_CHASE) {
        if (hp == H_GATE) { text(16, top, 1, TXT, hTNM[htgt]); sprintf(b, "GATE %d OF 3: %s", hgate + 1, hGNM[hgate]); text(16, top + 14, 1, C15(24, 28, 31), b); text(16, top + 28, 1, C15(20, 24, 28), hGHINT[hgate]); }
        else { text(16, top, 1, C15(31, 12, 10), "THE GETAWAY"); sprintf(b, "CORNER %d OF %d", hround + 1, hstars + 1); text(16, top + 14, 1, C15(24, 28, 31), b); text(16, top + 28, 1, C15(20, 24, 28), hline2); }
        rect(16, top + 50, 224, 16, K8(200, 206, 214)); rect(16 + hzone * 224 / 100, top + 50, hzw * 224 / 100, 16, K8(255, 235, 120)); rect(14 + hmark * 224 / 100, top + 46, 5, 24, K8(60, 70, 200));
        for (int i = 0; i < 5; i++) { rect(16 + i * 16, top + 84, 12, 12, i < hstars ? K8(236, 170, 30) : K8(110, 110, 130)); }
        text(16, top + 108, 1, TXT, hline); return; }
    if (hp == H_CHOICE) { text(16, top, 1, TXT, "YOU'RE IN. NOW WHAT?"); text(16, top + 24, 1, C15(24, 28, 31), htgt >= 5 ? "KEEP IT. IT IS HOT." : "SELL IT TO THE CHOP SHOP"); text(16, top + 42, 1, C15(24, 28, 31), "FENCE IT FOR CASH"); return; }
    text_fit(16, top + 6, 1, 224, TXT, hline); text_fit(16, top + 28, 1, 224, C15(20, 24, 28), hline2); sprintf(b, "FUNDS $%d   HEAT %d", funds, hs_heat); text(16, top + 60, 1, C15(14, 30, 14), b);
}
static void hs_bottom(void) {
    scr = offbuf; rect(0, 0, 256, 192, K8(14, 14, 34)); rect(0, 0, 256, 16, K8(30, 20, 50)); ctext(128, 4, 1, K8(200, 190, 250), "UNDERWORLD");
    if (hp == H_LIST) {
        for (int i = 0; i < HT_N; i++) { int y = 18 + i * 14, s = i == hsel; rect(8, y, 240, 13, s ? K8(226, 242, 254) : PANB); rect(9, y + 1, 238, 11, s ? C15(24, 28, 31) : PAN); char b[40]; sprintf(b, "%s", hTNM[i]); text(14, y + 6, 1, s ? C15(2, 7, 20) : TXT, b); sprintf(b, "$%d", hTVAL[i]); text(242 - tw(1, b), y + 6, 1, s ? C15(2, 14, 4) : C15(14, 30, 14), b); }
        chip(8, 162, 116, 26, "Case it", 1); chip(132, 162, 116, 26, "Leave", 0); return; }
    if (hp == H_GATE || hp == H_CHASE) { chip(8, 26, 240, 130, "NOW!", 1); return; }
    if (hp == H_CHOICE) { chip(8, 26, 240, 60, htgt >= 5 ? "Keep it (hot)" : "Sell to the chop shop", hsel == 0); chip(8, 96, 240, 60, "Fence it for cash", hsel == 1); return; }
    chip(8, 40, 240, 100, "OK", 1);
}
static void hs_frame(int held, int down, int steps, int tx, int ty) {
    (void)held; int pr = 0, td = down & KEY_TOUCH;
    if (down & KEY_UP) pr |= PD_U; if (down & KEY_DOWN) pr |= PD_D; if (down & KEY_A) pr |= PD_X; if (down & KEY_B) pr |= PD_O;
    if (td) {
        if (hp == H_LIST) { for (int i = 0; i < HT_N; i++) if (hit(8, 18 + i * 14, 240, 13, tx, ty)) hsel = i; if (hit(8, 162, 116, 26, tx, ty)) pr |= PD_X; if (hit(132, 162, 116, 26, tx, ty)) pr |= PD_O; }
        else if (hp == H_GATE || hp == H_CHASE) { if (hit(8, 26, 240, 130, tx, ty)) pr |= PD_X; }
        else if (hp == H_CHOICE) { if (hit(8, 26, 240, 60, tx, ty)) { hsel = 0; pr |= PD_X; } else if (hit(8, 96, 240, 60, tx, ty)) { hsel = 1; pr |= PD_X; } }
        else if (hit(8, 40, 240, 100, tx, ty)) pr |= PD_X; }
    int op = hp, os = hsel, ot = htmr;
    if (!hs_update(pr, steps)) { mg_ui = 0; mg_ver++; return; }
    if (pr || op != hp || os != hsel || ot != htmr || hp == H_GATE || hp == H_CHASE) mg_ver++;
}

#define MG_GT 0x5447   // 'GT': the Boreagotchi toy, its creatures and the Blobdex
// ==== BOREAGOTCHI! (src/gotchi.c, gotchi_ui.c): a toy ($5000) with up to eight creatures: eggs, babies ... grown-ups, 16 kinds, four forms each, by how they were looked after ====
#define GT_MAX 8
#define GB_N 8
#define GSP_N 16
#define GOTCHI_PRICE 5000
#define GOTCHI_EGG 150
enum { GS_EGG, GS_BABY, GS_CHILD, GS_TEEN, GS_ADULT, GS_ELDER };
enum { GF_SICK = 1, GF_SLEEP = 2, GF_DARK = 4, GF_CALL = 8, GF_FAKE = 16 };
typedef struct { u8 on, stage, form, sp, nm, hun, hap, disc, wt, poop, fl, miss, needT, bond, snk, meal, gen; u16 age; u8 pers, col, mut, id, mom, dad, mate; } Blob;   // (D54: family) pers, colour, rare mutation, its own id, its parents' ids, its mate's id
typedef struct { u8 id, sp, nm, mom, dad, col; } Ln;   // one line of a family tree: kept after a pet has passed on (nm bit 7)
#define GLN 12
typedef struct { u8 uid, own, sel, pad; u16 xp, hatched, wins, cared; Blob b[GB_N]; Ln ln[GLN]; u8 nid, lh, mem, memnm, memsp; } Toy;
static Toy gtoy[GT_MAX]; static u32 gdex[2]; static u32 grng = 0x1234567u;
static int gr(void) { grng = grng * 1103515245u + 12345u; return (int)((grng >> 16) & 255); }
static const char *const gspNm[GSP_N] = { "BLOB", "CAT", "BUNNY", "BIRD", "FROG", "DINO", "FISH", "BEAR", "GHOST", "ROBOT", "SHROOM", "SLIME", "BAT", "SNAIL", "OCTO", "ALIEN" };
static const char *const ggiv[32] = { "BLOBERT", "SNORLOCK", "MEH-CHAN", "DOZY", "SIR SIGH", "YAWNIE", "PUDDLE", "KEVIN", "NIBBLES", "GRUMP", "WOBBLE", "SPROCKET",
    "TOASTY", "MOCHI", "BEANS", "ZIGGY", "PICKLE", "DUMPLING", "FUZZ", "CLANK", "BOOP", "SQUISH", "MUNCH", "BISCUIT", "GLORP", "NOODLE", "PIP", "WAFFLES", "TWIG", "DOUGH", "ORBIT", "SNOOZE" };
static const char *const gpersNm[8] = { "CLEVER", "LAZY", "PLAYFUL", "GENTLE", "GREEDY", "BRAVE", "SHY", "GRUMPY" };
static const char *const gcolNm[8] = { "GREEN", "BLUE", "PINK", "GOLD", "MINT", "GREY", "RED", "PURPLE" };
static const u16 gcolInk[8] = { C15(3, 5, 3), C15(3, 6, 22), C15(24, 3, 12), C15(20, 13, 0), C15(2, 14, 10), C15(8, 8, 8), C15(24, 3, 3), C15(14, 3, 20) };
static const char *const gformPre[3][4] = { { "", "", "", "" }, { "ANGSTY ", "SLOUCHY ", "", "" }, { "SUPER ", "SLOB ", "CHILL ", "STAR " } };
static void gname(const Blob *b, char *out) {
    const char *pre = b->stage == GS_ELDER ? "ELDER " : b->stage == GS_ADULT ? gformPre[2][b->form & 3] : b->stage == GS_TEEN ? gformPre[1][b->form & 1] : "";
    strcpy(out, pre); strcat(out, b->stage == GS_EGG ? "EGG" : gspNm[b->sp % GSP_N]); if (b->stage == GS_BABY) strcat(out, " BABY"); }
static void gdex_see(int sp, int form) { int i = (sp % GSP_N) * 4 + (form & 3); gdex[i >> 5] |= 1u << (i & 31); }
static int gdex_has(int sp, int form) { int i = (sp % GSP_N) * 4 + (form & 3); return (gdex[i >> 5] >> (i & 31)) & 1; }
static int gdex_count(void) { int n = 0; for (int i = 0; i < GSP_N * 4; i++) n += (gdex[i >> 5] >> (i & 31)) & 1; return n; }
static void gt_reset(void) { memset(gtoy, 0, sizeof gtoy); gdex[0] = gdex[1] = 0; }
static int gsim_of(const Toy *t) { return t->uid < nsim ? t->uid : -1; }
static Toy *gtoy_of(int si) { if (si < 0 || si >= nsim) return 0; for (int i = 0; i < GT_MAX; i++) if (gtoy[i].own && gtoy[i].uid == si) return &gtoy[i]; return 0; }
static int gcount(const Toy *t) { int n = 0; for (int i = 0; i < GB_N; i++) n += t->b[i].on; return n; }
static int gneeds(const Blob *b) { return b->on && b->stage > GS_EGG && (b->hun == 0 || b->hap == 0 || b->poop || (b->fl & GF_SICK) || ((b->fl & GF_SLEEP) && !(b->fl & GF_DARK)) || (b->fl & GF_FAKE)); }
static int gcalling(const Toy *t) { int n = 0; for (int i = 0; i < GB_N; i++) n += gneeds(&t->b[i]); return n; }
static const char *gwant(const Blob *b) {
    if (b->stage == GS_EGG) return "STILL AN EGG";
    return b->hun == 0 ? "IT IS HUNGRY" : b->hap == 0 ? "IT IS BORED" : b->poop ? "IT MADE A MESS" : (b->fl & GF_SICK) ? "IT FEELS SICK" :
           ((b->fl & GF_SLEEP) && !(b->fl & GF_DARK)) ? "TURN THE LIGHT OFF" : (b->fl & GF_FAKE) ? "IT IS FIBBING. SCOLD IT" : "ALL GOOD"; }
static const u16 gxpAt[6] = { 0, 30, 100, 250, 500, 900 };
static const char *const glvNm[6] = { "NEW PARENT", "CARING", "ATTENTIVE", "BLOB WHISPERER", "BREEDER", "GOTCHI MASTER" };
static int glevel(const Toy *t) { int l = 0; for (int i = 1; i < 6; i++) if (t->xp >= gxpAt[i]) l = i; return l; }
static int gnext_xp(const Toy *t) { int l = glevel(t); return l >= 5 ? 0 : gxpAt[l + 1]; }
static void gxp(Toy *t, int n) { int l0 = glevel(t); t->xp = (u16)MIN(60000, t->xp + n); t->cared++; if (glevel(t) > l0) { static char b[40]; snprintf(b, sizeof b, "Blob care level %d!", glevel(t)); say(b); } }
// the owner's stats (the PS1's sim_gotchi_*): mood -100..100, food 0..100; fun and food given
static int gstat(int si, int what) { if (si < 0 || si >= nsim) return 0; return what == 0 ? (mood(&sims[si]) - 50) * 2 : sims[si].need[0]; }
static void gfun(int si, int n) { if (si >= 0 && si < nsim) sims[si].need[1] = cl100(sims[si].need[1] + n); }
static void gfood(int si, int n) { if (si >= 0 && si < nsim) sims[si].need[0] = cl100(sims[si].need[0] + n); }
static void gfresh(Blob *b, int sp) { memset(b, 0, sizeof *b); b->on = 1; b->sp = (u8)(sp % GSP_N); b->nm = (u8)(gr() & 31); b->hun = 4; b->hap = 4; b->wt = 5; b->pers = (u8)(gr() & 7); b->col = (u8)(gr() & 7); b->mut = (gr() & 31) == 0; }
static void gln_set(Toy *t, const Blob *b, int gone) {   // the family tree remembers it: kept up to date, and kept after it has passed on
    int k = -1; for (int i = 0; i < GLN; i++) if (t->ln[i].id == b->id) { k = i; break; } if (k < 0) k = t->lh++ % GLN;
    t->ln[k].id = b->id; t->ln[k].sp = b->sp; t->ln[k].nm = (u8)((b->nm & 31) | (gone ? 128 : 0)); t->ln[k].mom = b->mom; t->ln[k].dad = b->dad; t->ln[k].col = b->col; }
static void gnewid(Toy *t, Blob *b, int mom, int dad) { if (!++t->nid) ++t->nid; b->id = t->nid; b->mom = (u8)mom; b->dad = (u8)dad; gln_set(t, b, 0); }
static Blob *gby_id(Toy *t, int id) { if (!id) return 0; for (int i = 0; i < GB_N; i++) if (t->b[i].on && t->b[i].id == id) return &t->b[i]; return 0; }
static const Ln *gln_of(const Toy *t, int id) { if (!id) return 0; for (int i = 0; i < GLN; i++) if (t->ln[i].id == id) return &t->ln[i]; return 0; }
static int gbreed(Toy *t, Blob *x, Blob *y) {   // two grown-ups who love each other leave an egg: kind, colour, nature and a rare shine come from them
    for (int s = 0; s < GB_N; s++) if (!t->b[s].on) { Blob *e = &t->b[s]; gfresh(e, (gr() & 15) == 0 ? gr() : (gr() & 1 ? x->sp : y->sp));
        e->gen = (u8)MIN(99, MAX(x->gen, y->gen) + 1); e->pers = (gr() & 7) == 0 ? (u8)(gr() & 7) : (gr() & 1 ? x->pers : y->pers); e->col = (gr() & 7) == 0 ? (u8)(gr() & 7) : (gr() & 1 ? x->col : y->col); e->mut = (gr() & 15) == 0 || (x->mut && y->mut && (gr() & 1)); gnewid(t, e, x->id, y->id); return 1; }
    return 0; }
static int gbuy(int si) {
    if (si < 0 || si >= nsim || gtoy_of(si)) return 0;
    int k = -1; for (int i = 0; i < GT_MAX; i++) if (!gtoy[i].own || gsim_of(&gtoy[i]) < 0) { k = i; break; } if (k < 0) return 0;
    Toy *t = &gtoy[k]; memset(t, 0, sizeof *t); t->own = 1; t->uid = (u8)si; gfresh(&t->b[0], gr()); gnewid(t, &t->b[0], 0, 0); return 1; }
static int gegg(Toy *t, int slot) { if (slot < 0 || slot >= GB_N || t->b[slot].on) return 0; gfresh(&t->b[slot], gr()); gnewid(t, &t->b[slot], 0, 0); return 1; }
static int gnight(void) { int c = minutes % 1440; return c >= 20 * 60 || c < 6 * 60; }
static void gtick(Toy *t, Blob *b) {   // one game minute of one creature
    if (!b->on) return;
    if (b->age < 65000) b->age++;
    if (b->stage == GS_EGG) { if (b->age >= 20) { b->stage = GS_BABY; b->age = 0; b->hun = 3; b->hap = 3; gdex_see(b->sp, 0); t->hatched++; say("A Boreagotchi hatched!"); } return; }
    int n = gnight(), sl = (b->fl & GF_SLEEP) != 0;
    if (n && !sl && b->stage >= GS_CHILD) b->fl |= GF_SLEEP; else if (!n && sl) b->fl &= (u8)~(GF_SLEEP | GF_DARK);
    sl = (b->fl & GF_SLEEP) != 0;
    int lv = glevel(t), rate = (b->stage == GS_BABY ? 30 : b->stage == GS_CHILD ? 45 : 60) + lv * 4; if (sl) rate *= 3;
    { int rh = rate, rp = rate + 10; if (b->pers == 4) rh = rh * 4 / 5; if (b->pers == 2) rp = rp * 4 / 5; if (b->pers == 1) { rh = rh * 5 / 4; rp = rp * 5 / 4; }   // GREEDY eats more, PLAYFUL wants fun, LAZY wants little
      if (b->age % rh == 0 && b->hun) b->hun--;
      if (b->age % rp == 0 && b->hap) b->hap--; }
    if (!sl && b->meal && b->age % 90 == 45 && b->poop < 3) b->poop++;
    if (b->poop && b->age % 60 == 30 && ((gr() * 100) >> 8) < 35 - lv * 4) b->fl |= GF_SICK;
    if (!(b->fl & GF_FAKE) && !gneeds(b) && !sl && b->age % 150 == 75 && (gr() & 1)) b->fl |= GF_FAKE;
    if (gneeds(b)) { b->fl |= GF_CALL; if (++b->needT >= 15 + lv * 2) { b->needT = 0; if (b->miss < 99) b->miss++; if (b->bond) b->bond--; b->fl &= (u8)~GF_FAKE; } }
    else { b->fl &= (u8)~GF_CALL; b->needT = 0; if (!sl && b->hun >= 3 && b->hap >= 3 && b->age % 120 == 0 && b->bond < 100) b->bond++; }
    static const u16 grow[5] = { 0, 240, 720, 1440, 0 };
    if (b->stage < GS_ADULT && b->age >= grow[b->stage]) {
        b->stage++; b->age = 0;
        if (b->stage == GS_TEEN) b->form = b->miss <= 2 ? 0 : 1;
        if (b->stage == GS_ADULT) {
            b->form = (b->miss <= 1 && b->bond >= 80 && b->disc >= 50) ? 3 : (b->snk > b->meal && b->snk >= 6) ? 2 : (b->miss <= 2 && b->disc >= 50) ? 0 : 1;
            gdex_see(b->sp, b->form);
            if (b->form == 0 || b->form == 3) say(b->form == 3 ? "A rare STAR form!" : "A SUPER Bore! So proud"); else say("Your Boreagotchi grew up");
        }
        b->miss = 0; }
    if (b->stage == GS_ADULT && b->age >= 2880) { b->stage = GS_ELDER; b->age = 0; gln_set(t, b, 0); say("A Boreagotchi is an ELDER now"); }
    if (b->stage == GS_ELDER && b->age >= 1440 + b->bond * 6) { static char pm[56]; snprintf(pm, sizeof pm, "%s passed on. Always remembered", ggiv[b->nm & 31]); say(pm); gln_set(t, b, 1); t->mem++; t->memnm = b->nm; t->memsp = b->sp; b->on = 0; return; }
    if (b->hun == 0 && b->hap == 0 && (b->fl & GF_SICK) && b->miss >= 12) { b->on = 0; gln_set(t, b, 1); say("A Boreagotchi went home"); }
}
static const char *gfeed(Toy *t, Blob *b, int snack) {
    if (b->stage == GS_EGG) return "IT IS STILL AN EGG";
    if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (!snack) { if (b->hun >= 4) return "IT REFUSES. IT IS FULL"; b->hun++; if (b->wt < 99) b->wt++; if (b->meal < 250) b->meal++; if (b->bond < 100 && !(gr() & 3)) b->bond++; gxp(t, 1); return "NOM NOM. A MEAL"; }
    if (b->hap < 4) b->hap++; if (b->wt < 98) b->wt += 2; if (b->snk < 250) b->snk++; gxp(t, 1); return "CRUNCH. A SNACK. SO RELAXED"; }
static const char *glight(Toy *t, Blob *b) { (void)t; b->fl ^= GF_DARK; return (b->fl & GF_DARK) ? ((b->fl & GF_SLEEP) ? "LIGHTS OUT. GOOD NIGHT" : "LIGHTS OUT") : "LIGHTS ON"; }
static const char *gmedicine(Toy *t, Blob *b) { if (!(b->fl & GF_SICK)) return "IT IS NOT SICK. IT SPITS IT OUT"; b->fl &= (u8)~GF_SICK; if (b->bond < 100) b->bond++; gxp(t, 2); return "ALL BETTER"; }
static const char *gclean(Toy *t, Blob *b) { if (!b->poop) return "NOTHING TO CLEAN"; b->poop = 0; if (b->bond < 100) b->bond++; gxp(t, 2); return "FLUSHED"; }
static const char *gscold(Toy *t, Blob *b) {
    if (b->fl & GF_FAKE) { b->fl &= (u8)~GF_FAKE; b->disc = (u8)(b->disc + 25 > 100 ? 100 : b->disc + 25); gxp(t, 3); return "IT KNOWS BETTER NOW. DISCIPLINE UP"; }
    if (b->hap) b->hap--; return "IT DID NOTHING WRONG. IT IS SAD NOW"; }
static const char *ggame(Toy *t, Blob *b, int won) {
    if (!won) { gxp(t, 1); return "IT LOST. STILL FUN"; }
    if (b->hap < 4) b->hap++; if (b->wt > 1) b->wt--; if (b->bond < 100) b->bond += 2; gxp(t, 2); t->wins++; gfun(gsim_of(t), 6); return "IT WON! SO HAPPY"; }
static int gpower(const Blob *b) { return (b->hun + b->hap) * 3 + b->disc / 8 + b->stage * 6 + b->bond / 10 - b->miss * 2 - (b->fl & GF_SICK ? 10 : 0) + (b->form == 0 || b->form == 3 ? 4 : 0); }
static const char *gfriends(Toy *t, int a, int c, int battle) {
    Blob *x = &t->b[a], *y = &t->b[c];
    if (a == c || !x->on || !y->on || x->stage == GS_EGG || y->stage == GS_EGG) return "THEY BOTH NEED TO HAVE HATCHED";
    if ((x->fl | y->fl) & GF_SLEEP) return "SHHH. SOMEONE IS ASLEEP";
    if (battle) { int px = gpower(x) + gr() % 12, py = gpower(y) + gr() % 12; Blob *w = px >= py ? x : y, *l = px >= py ? y : x;
        if (w->bond < 97) w->bond += 3; if (l->hap) l->hap--; w->disc = (u8)MIN(100, w->disc + 6); gxp(t, 3); t->wins++;
        return w == x ? "YOUR FIRST ONE WON THE BATTLE!" : "YOUR SECOND ONE WON THE BATTLE!"; }
    if (x->hap < 4) x->hap++; if (y->hap < 4) y->hap++; x->bond = (u8)MIN(100, x->bond + 4); y->bond = (u8)MIN(100, y->bond + 4); gxp(t, 3); gfun(gsim_of(t), 8);
    if (x->stage == GS_ADULT && y->stage == GS_ADULT && x->bond >= 60 && y->bond >= 60) {   // two grown-ups who love each other: a couple, and then eggs
        if (!x->mate && !y->mate) { x->mate = y->id; y->mate = x->id; return "THEY ARE A COUPLE NOW. HEARTS EVERYWHERE"; }
        if (x->mate == y->id && !(gr() & 1) && gbreed(t, x, y)) return "THEY PLAYED... AND LEFT AN EGG!"; }
    return "A LOVELY PLAYDATE. BOTH ARE HAPPY"; }
static const char *glunch(Toy *t, Blob *b) {
    int si = gsim_of(t); if (b->stage == GS_EGG) return "IT IS STILL AN EGG"; if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (gstat(si, 1) < 12) return "YOU ARE TOO HUNGRY TO SHARE. EAT FIRST!";
    if (b->hun >= 4) return "IT IS FULL. KEEP YOUR LUNCH"; gfood(si, -15); b->hun = 4; if (b->hap < 4) b->hap++; b->bond = (u8)MIN(100, b->bond + 3); if (b->meal < 250) b->meal++; gxp(t, 2);
    return "YOU GAVE IT YOUR LUNCH. YOUR TUMMY GROWLS"; }
static const char *gnuzzle(Toy *t, Blob *b) {
    int si = gsim_of(t), h = (minutes / 60) & 255; if (b->stage == GS_EGG) return "IT IS STILL AN EGG";
    if (t->pad == (u8)h) return "IT ALREADY NUZZLED YOU THIS HOUR"; t->pad = (u8)h; gfun(si, 6); if (b->bond < 100) b->bond++; gxp(t, 1); return "ALL GOOD. IT NUZZLES YOU. AWW"; }
static int gkin(int si) { return gstat(si, 1) >= 20 && gstat(si, 0) > -40; }
static int gparent(Toy *t, const Blob *b) {   // a grown-up, awake, close parent of a baby or child
    if (b->stage > GS_CHILD) return 0; for (int k = 0; k < 2; k++) { Blob *p = gby_id(t, k ? b->dad : b->mom); if (p && p->stage >= GS_ADULT && !(p->fl & GF_SLEEP) && p->bond >= 40) { if (!(gr() & 7)) p->bond = (u8)MIN(100, p->bond + 1); return 1; } } return 0; }
static void gfamily(Toy *t, int si) {   // once an hour: mates with an egg of love, brothers and sisters playing
    for (int i = 0; i < GB_N; i++) { Blob *x = &t->b[i]; if (!x->on) continue;
        if (x->mate && !gby_id(t, x->mate)) x->mate = 0;
        if (x->stage == GS_ADULT && x->mate && x->bond >= 60) { Blob *y = gby_id(t, x->mate);
            if (y && y->stage == GS_ADULT && y->bond >= 60 && x->id < y->id && !(gr() & 7)) { int has = 0; for (int q = 0; q < GB_N; q++) if (t->b[q].on && t->b[q].stage == GS_EGG && t->b[q].mom == x->id) has = 1;
                if (!has && gbreed(t, x, y) && si == ctl) say("Love left an egg in the basket"); } }
        if (x->stage > GS_EGG && x->mom) for (int j = i + 1; j < GB_N; j++) { Blob *y = &t->b[j]; if (y->on && y->stage > GS_EGG && y->mom == x->mom && !(gr() & 3)) { if (x->hap < 4) x->hap++; if (y->hap < 4) y->hap++; if (x->bond < 99) x->bond++; if (y->bond < 99) y->bond++; } } } }
static void gt_pets(void) {   // one minute of pet life: every creature lives on
    for (int k = 0; k < GT_MAX; k++) {
        Toy *t = &gtoy[k]; if (!t->own) continue; int si = gsim_of(t); if (si < 0) continue;
        for (int i = 0; i < GB_N; i++) {
            Blob *b = &t->b[i]; if (!b->on) continue;
            gtick(t, b);
            { int food = gstat(si, 1), md = gstat(si, 0);   // a starving or miserable owner starves their pet of mood too, and a happy one lifts it
              if (b->stage > GS_EGG && !(b->fl & GF_SLEEP)) {
                  if (food < 15 && !(gr() & 15) && b->hun) { b->hun--; if (si == ctl && !(gr() & 7)) say("Your pet feels what you feel"); }
                  if (md < -40 && b->age % 25 == 0 && b->hap) b->hap--;
                  if (md > 50 && b->age % 40 == 0) { if (b->hap < 4) b->hap++; if (b->bond < 100 && !(gr() & 3)) b->bond++; } } }
            if (gneeds(b) && !(gr() & 15) && ((si != ctl && gkin(si)) || gparent(t, b))) {   // a housemate looks after their own, and parents look after their young
                if (b->hun == 0) gfeed(t, b, 0); else if (b->poop) gclean(t, b); else if (b->fl & GF_SICK) gmedicine(t, b); else if (b->fl & GF_FAKE) gscold(t, b); else if (b->hap == 0) b->hap = 2; } } }
    for (int k = 0; k < GT_MAX; k++) {   // and the other way: pets in a bad way drag their owner down, happy ones lift them
        Toy *t = &gtoy[k]; if (!t->own) continue; int si = gsim_of(t); if (si < 0 || minutes % 60) continue;
        int n = 0, glum = 0, glad = 0; for (int i = 0; i < GB_N; i++) { Blob *b = &t->b[i]; if (!b->on || b->stage == GS_EGG) continue; n++; if (b->hun == 0 && b->hap == 0) glum++; else if (b->hap >= 3 && b->hun >= 2) glad++; }
        gfamily(t, si);
        if (glum) gfun(si, -3 * glum); else if (n >= 2 && glad == n) gfun(si, 3); }
}
static void gt_minute(void) {   // every game minute (D53: Options > Game > Boreagotchi time: the game clock, LIVE, or off: they wait)
    if (o_gtime != 2) gt_pets();
    { static int beep; Toy *me = gtoy_of(ctl); if (o_gtime != 2 && me && gcalling(me) && ++beep >= 60) { beep = 0; say("Your Boreagotchi is beeping"); } }
}
static void gt_absorb(int si) {   // staring at the toy: the owner forgets to eat, sleep and talk to anyone (the PS1's sim_gotchi_absorb)
    if (si < 0 || si >= nsim) return; Sim *s = &sims[si];
    s->need[0] = MAX(0, s->need[0] - 3); s->need[2] = MAX(0, s->need[2] - 1); s->need[3] = MAX(0, s->need[3] - 2); s->need[5] = MAX(0, s->need[5] - 1);
    static int n; static const char *const fun[5] = { "Your tummy growls. The pet looks worried", "When did you last eat? The pet wonders too", "Your housemates are waving at you", "You haven't blinked in a while", "The fridge is right there. Just saying" };
    if (s->need[0] < 25 && !(++n & 3)) say(fun[(n >> 2) % 5]);
}
static void gt_live(int steps) {   // (D53) LIVE: while the handheld is open the toy runs on real time: a pet minute a second, the owner forgets to eat
    if (o_gtime != 1) return; static int a, b; a += steps; b += steps;
    while (a >= 60) { a -= 60; gt_pets(); }
    while (b >= 300) { b -= 300; gt_absorb(ctl); } }
// ---- the save: only the creatures that are there ----
static int gt_pack(u8 *o) {
    int n = 0, nt = 0; for (int k = 0; k < GT_MAX; k++) if (gtoy[k].own) nt++;
    if (!nt && !gdex[0] && !gdex[1]) return 0;
    u8 *q = o + 4; memcpy(q, gdex, 8); q += 8; *q++ = (u8)nt;
    for (int k = 0; k < GT_MAX; k++) { const Toy *t = &gtoy[k]; if (!t->own) continue; *q++ = t->uid; *q++ = t->sel; memcpy(q, &t->xp, 2); q += 2; memcpy(q, &t->hatched, 2); q += 2; memcpy(q, &t->wins, 2); q += 2; memcpy(q, &t->cared, 2); q += 2; *q++ = t->pad;
        for (int i = 0; i < GB_N; i++) { const Blob *b = &t->b[i]; *q++ = b->on; if (b->on) { memcpy(q, &b->stage, 16); q += 16; memcpy(q, &b->age, 2); q += 2; } } }
    *q++ = 'F';   // (D54) the families, after everything an older game knows
    for (int k = 0; k < GT_MAX; k++) { const Toy *t = &gtoy[k]; if (!t->own) continue; *q++ = t->nid; *q++ = t->mem; *q++ = t->memnm; *q++ = t->memsp; int nl = 0; for (int i = 0; i < GLN; i++) nl += t->ln[i].id != 0; *q++ = (u8)nl; for (int i = 0; i < GLN; i++) if (t->ln[i].id) { memcpy(q, &t->ln[i], 6); q += 6; }
        for (int i = 0; i < GB_N; i++) { const Blob *b = &t->b[i]; if (b->on) { memcpy(q, &b->pers, 7); q += 7; } } }
    n = (int)(q - o) - 4; u16 th[2] = { MG_GT, (u16)n }; memcpy(o, th, 4); return n + 4; }
static void gt_unpack(const u8 *p, int n) {
    if (n < 9) return; memcpy(gdex, p, 8); const u8 *q = p + 8, *e = p + n; int nt = *q++;
    for (int k = 0; k < nt && k < GT_MAX; k++) { Toy *t = &gtoy[k]; if (q + 11 > e) break; memset(t, 0, sizeof *t); t->own = 1; t->uid = *q++; t->sel = *q++; memcpy(&t->xp, q, 2); q += 2; memcpy(&t->hatched, q, 2); q += 2; memcpy(&t->wins, q, 2); q += 2; memcpy(&t->cared, q, 2); q += 2; t->pad = *q++;
        for (int i = 0; i < GB_N; i++) { if (q >= e) break; Blob *b = &t->b[i]; b->on = *q++; if (b->on) { if (q + 18 > e) { b->on = 0; break; } memcpy(&b->stage, q, 16); q += 16; memcpy(&b->age, q, 2); q += 2; } } }
    int fam = q < e && *q == 'F'; if (fam) q++;
    for (int k = 0; k < nt && k < GT_MAX; k++) { Toy *t = &gtoy[k];
        if (fam && q + 5 <= e) { t->nid = *q++; t->mem = *q++; t->memnm = *q++; t->memsp = *q++; int nl = *q++; for (int i = 0; i < nl && q + 6 <= e; i++) { if (i < GLN) memcpy(&t->ln[i], q, 6); q += 6; } t->lh = (u8)MIN(nl, GLN); }
        for (int i = 0; i < GB_N; i++) { Blob *b = &t->b[i]; if (!b->on) continue;
            if (fam && q + 7 <= e) { memcpy(&b->pers, q, 7); q += 7; }
            else { b->pers = (u8)(b->nm & 7); b->col = (u8)((b->nm >> 3) & 3); if (b->stage == GS_ADULT && b->age > 900) b->age = 900; if (!++t->nid) ++t->nid; b->id = t->nid; gln_set(t, b, 0); } } } }

// ---- the screens: the creature big on the top screen, the pad on the bottom ----
enum { GV_NONE, GV_BASKET, GV_HAND, GV_DEX, GV_FEED, GV_PLAYPICK, GV_LOOK, GV_COPY, GV_FRIEND, GV_STATS, GV_BOX, GV_GUIDE, GV_TREE };   // (D53: the Toy Box and the guide)
#define PD_SQ 128
#define PD_L1 256
#define PD_R1 512
#define PD_SEL 1024
#define GNP 10
static int gbx, gfrom_box, gguide, gguideback = -1, gdexback = -1;   // (D53) the Toy Box's row, whether the basket was opened from it, the guide's page and where its Back goes, where the Blobdex's Back goes
static int gtree; static int gview, gti, gbi, gisel, ganim, gmsgT, gdexpage, gsub, gmN, gmT, gmWon, gmRound, gmDir, gsidx; static u8 gseq[6]; static const char *gmsg;
static unsigned glrng;
static int grr(void) { glrng = glrng * 1103515245u + 12345u; return (int)((glrng >> 16) & 255); }
static Toy *gtt(void) { return &gtoy[gti]; }
static Blob *gcb(void) { return &gtt()->b[gbi]; }
static void gsay(const char *s) { gmsg = s; gmsgT = 150; }
static int gliving(Toy *t, int from, int dir) { for (int k = 1; k <= GB_N; k++) { int j = (from + dir * k + GB_N * 2) % GB_N; if (t->b[j].on) return j; } return from; }
// ---- the 32 x 16 LCD, and the creatures drawn on it (gotchi_ui.c, as it is) ----
static u8 gdot[16][32]; static int gminY;
static void gP(int x, int y, int v) { if (x >= 0 && x < 32 && y >= 0 && y < 16) { gdot[y][x] = (u8)v; if (v && y < gminY) gminY = y; } }
static void gRC(int x0, int y0, int x1, int y1, int v) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) gP(x, y, v); }
static void gELL(int cx, int cy, int rx, int ry, int v) { for (int y = -ry; y <= ry; y++) for (int x = -rx; x <= rx; x++) if (x * x * ry * ry + y * y * rx * rx <= rx * rx * ry * ry + rx * ry) gP(cx + x, cy + y, v); }
static void gLN(int x0, int y0, int x1, int y1, int v) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y1 - y0 : y0 - y1, sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx - dy;
    for (int n = 0; n < 40; n++) { gP(x0, y0, v); if (x0 == x1 && y0 == y1) break; int e2 = 2 * e; if (e2 > -dy) { e -= dy; x0 += sx; } if (e2 < dx) { e += dx; y0 += sy; } }
}
static void gstar(int x, int y) { gP(x, y, 1); gP(x - 1, y, 1); gP(x + 1, y, 1); gP(x, y - 1, 1); gP(x, y + 1, 1); }

static void g_creature(const Blob *b, int fr, int d, int ox) {
    static const u8 RX[5] = { 0, 3, 4, 5, 6 }, RY[5] = { 0, 3, 4, 4, 5 };
    int st = b->stage > 4 ? 4 : b->stage, sp = b->sp % GSP_N, rx = RX[st], ry = RY[st], bob = fr & 1, by = 14 - bob, cx = 16 + ox, cy, top, ex, eyY, big = st >= GS_TEEN;
    switch (sp) {   // the body, and what makes the kind
    case 0: cy = by - ry; gELL(cx, cy, rx, ry, 1); gP(cx - rx / 2, by + 1, 1); gP(cx + rx / 2, by + 1, 1); break;
    case 1: cy = by - ry; gELL(cx, cy, rx, ry, 1); top = cy - ry; gP(cx - rx + 1, top - 1, 1); gP(cx + rx - 1, top - 1, 1); if (st > 1) { gP(cx - rx + 1, top - 2, 1); gP(cx + rx - 1, top - 2, 1); }
        gLN(cx - d * rx, cy + 1, cx - d * (rx + 2), cy - 2 + bob, 1); gP(cx - 1, by + 1, 1); gP(cx + 1, by + 1, 1); break;
    case 2: ry--; cy = by - ry; gELL(cx, cy, rx, ry, 1); top = cy - ry; { int h = 2 + st, w = st >= GS_TEEN ? 1 : 0; gRC(cx - rx / 2 - 1 - w, top - h, cx - rx / 2 - 1, top - 1, 1); gRC(cx + rx / 2 + 1, top - h + bob, cx + rx / 2 + 1 + w, top - 1, 1); }
        gP(cx - d * (rx + 1), cy + ry - 1, 1); gP(cx - 1, by + 1, 1); gP(cx + 1, by + 1, 1); break;
    case 3: cy = by - ry; gELL(cx, cy, rx, ry, 1); top = cy - ry; gP(cx + d * (rx + 1), cy, 1); if (st > 1) gP(cx + d * (rx + 2), cy, 1); gP(cx, top - 1, 1); gP(cx + 1, top - 2, 1);
        gLN(cx - d, cy, cx - d * (rx - 1), cy + ry - 2, 0); gP(cx - 1, by + 1, 1); gP(cx + 1, by + 1, 1); break;
    case 4: ry--; cy = by - ry; gELL(cx, cy, rx + 1, ry, 1); top = cy - ry; gELL(cx - rx / 2 - 1, top, 1, 1, 1); gELL(cx + rx / 2 + 1, top, 1, 1, 1);
        gRC(cx - rx + 1, cy + 1, cx + rx - 1, cy + 1, 0); if (bob) gP(cx + d * rx, cy + 1, 1); gP(cx - rx, by + 1, 1); gP(cx + rx, by + 1, 1); break;
    case 5: cy = by - ry; gELL(cx, cy, rx, ry, 1); top = cy - ry; for (int i = 0; i <= rx / 2; i++) gP(cx - d * i * 2, top - 1, 1);
        gLN(cx - d * rx, cy + ry - 1, cx - d * (rx + 3), cy + ry - 3 + bob, 1); gP(cx + d * (rx + 1), cy - 1, 1); gP(cx + d * rx, cy + 2, 1); gP(cx - 1, by + 1, 1); gP(cx + 1, by + 1, 1); break;
    case 6: ry--; cy = by - ry - 1 + bob; gELL(cx, cy, rx + 1, ry, 1); top = cy - ry; gLN(cx - d * (rx + 1), cy, cx - d * (rx + 4), cy - 2, 1); gLN(cx - d * (rx + 1), cy, cx - d * (rx + 4), cy + 2, 1); gRC(cx - d * (rx + 3), cy - 1, cx - d * (rx + 3), cy + 1, 1);
        gP(cx, top - 1, 1); gP(cx - d, top - 1, 1); gP(cx + d * (rx + 1), cy + 1, 0); break;
    case 7: cy = by - ry; gELL(cx, cy, rx, ry, 1); top = cy - ry; gELL(cx - rx + 1, top, 1, 1, 1); gELL(cx + rx - 1, top, 1, 1, 1); gP(cx - rx - 1, cy + 2, 1); gP(cx + rx + 1, cy + 2, 1); gP(cx - 1, by + 1, 1); gP(cx + 1, by + 1, 1); break;
    case 8: cy = by - ry; gELL(cx, cy - 1, rx, ry - 1, 1); gRC(cx - rx, cy - 1, cx + rx, by - 1, 1); for (int x = -rx; x <= rx; x++) if (((x + fr) & 1) == 0) gP(cx + x, by, 1); gP(cx - rx - 1, cy + bob, 1); gP(cx + rx + 1, cy + 1 - bob, 1); break;
    case 9: cy = by - ry; gRC(cx - rx, cy - ry, cx + rx, by - 1, 1); gP(cx - rx, cy - ry, 0); gP(cx + rx, cy - ry, 0); top = cy - ry; gLN(cx, top - 1, cx, top - 2, 1); gP(cx + bob, top - 3, 1);
        gRC(cx - rx + 1, by, cx - 1, by + 1, 1); gRC(cx + 1, by, cx + rx - 1, by + 1, 1); gRC(cx - rx - 1, cy - 1, cx - rx - 1, cy + 2, 1); gRC(cx + rx + 1, cy - 1, cx + rx + 1, cy + 2, 1); break;
    case 10: { int capry = ry / 2 + 1; cy = by - ry / 2; gRC(cx - rx / 2, by - ry, cx + rx / 2, by, 1); gELL(cx, by - ry, rx, capry, 1); gP(cx - rx / 2, by - ry - 1, 0); gP(cx + rx / 2, by - ry - 1, 0); if (st > 1) gP(cx, by - ry - capry + 1, 0); gRC(cx - rx / 2 - 1, by + 1 - 1, cx - rx / 2 - 1, by, 1); break; }
    case 11: cy = by - ry / 2; gELL(cx, by, rx + 1 + bob, ry, 1); for (int y = by + 1; y < 16; y++) for (int x = 0; x < 32; x++) gdot[y][x] = 0; gP(cx - rx, by + 1, 1); if (fr & 2) gP(cx + rx - 1, by + 1, 1); break;
    case 12: cy = by - ry; gELL(cx, cy, rx - 1, ry - 1, 1); top = cy - ry + 1; gP(cx - rx + 2, top - 1, 1); gP(cx + rx - 2, top - 1, 1);
        for (int i = 1; i <= 4; i++) { int w = bob ? -2 : 1; gRC(cx - rx + 1 - i, cy - 2 + (bob ? 0 : 2) + i / 2 + w / 2, cx - rx + 1 - i, cy + 1 + i / 3, 1); gRC(cx + rx - 1 + i, cy - 2 + (bob ? 0 : 2) + i / 2 + w / 2, cx + rx - 1 + i, cy + 1 + i / 3, 1); } break;
    case 13: cy = by - ry; gRC(cx - rx, by - 1, cx + rx, by, 1); gELL(cx + d * rx, by - 2, 1, 1, 1); gELL(cx - d, by - ry + 1, ry - 1 > 1 ? ry - 1 : 2, ry - 1 > 1 ? ry - 1 : 2, 1); gP(cx - d, by - ry + 1, 0); gP(cx - d + 1, by - ry + 1, 0); gP(cx - d + 1, by - ry + 2, 0);
        gLN(cx + d * rx, by - 3, cx + d * rx, by - 5, 1); gLN(cx + d * (rx - 1), by - 3, cx + d * (rx - 1), by - 5 + bob, 1); cy = by - 2; break;
    case 14: ry--; cy = by - ry - 1; gELL(cx, cy, rx, ry, 1); for (int i = 0; i < 4; i++) { int x = cx - rx + 1 + i * (2 * rx - 2) / 3; for (int y = cy + ry; y <= by + 1; y++) gP(x + (((fr + i + y) & 2) ? 1 : 0), y, 1); } break;
    default: cy = by - ry; gELL(cx, cy - 1, rx, ry - 1, 1); gRC(cx - 1, cy + ry - 2, cx + 1, by, 1); top = cy - ry; gLN(cx - 2, top + 1, cx - 4, top - 2, 1); gLN(cx + 2, top + 1, cx + 4, top - 2 + bob, 1); gP(cx - 4, top - 3, 1); gP(cx + 4, top - 3 + bob, 1); break;
    }
    // face
    ex = rx / 2 > 0 ? rx / 2 : 1; eyY = cy - ry / 3; if (sp == 4 || sp == 6 || sp == 11) eyY = cy - 1; if (sp == 10) eyY = by - ry + 1; if (sp == 13) eyY = by - 4; if (sp == 12 || sp == 15) eyY = cy - 1;
    int e1 = cx + d - ex, e2 = cx + d + ex; if (sp == 6) { e1 = e2 = cx + d * (rx - 1); } if (sp == 13) { e1 = e2 = cx + d * rx; }
    int chill = st == GS_ADULT && b->form == 2, sleepy = (b->fl & GF_SLEEP) != 0;
    if (sp == 9) { gP(e1, eyY, 0); gP(e2, eyY, 0); if (big) { gP(e1 + 1, eyY, 0); gP(e2 + 1, eyY, 0); } }
    else if (sleepy || chill) { gP(e1, eyY, 0); gP(e2, eyY, 0); if (e1 != e2 && big) { gP(e1 - 1, eyY, 0); gP(e2 + 1, eyY, 0); } }
    else { gP(e1, eyY, 0); gP(e2, eyY, 0); if (big || sp == 15) { gP(e1, eyY + 1, 0); gP(e2, eyY + 1, 0); } if (sp == 15) { gP(e1 - 1, eyY, 0); gP(e2 + 1, eyY, 0); } }
    if (sp != 4 && sp != 6) {
        int my = cy + ry / 2 + (sp == 10 ? 1 : 0); if (sp == 13) my = by - 2;
        if (b->hun == 0 || b->hap == 0 || (b->fl & GF_SICK)) { gP(cx + d, my, 0); gP(cx + d - 1, my + 1, 0); gP(cx + d + 1, my + 1, 0); }
        else if (b->hap >= 3 && big) { gP(cx + d - 1, my, 0); gP(cx + d, my + 1, 0); gP(cx + d + 1, my, 0); }
        else gP(cx + d, my, 0);
    }
    if (b->stage == GS_ELDER) { gLN(cx + d * (rx + 2), cy, cx + d * (rx + 2), by + 1, 1); gP(cx + d * (rx + 1), cy, 1); gRC(cx - 1, cy + ry / 2 + 2, cx + 1, cy + ry / 2 + 2, 1); }   // a stick and a little beard
    if (b->mut && (fr & 2)) { gstar(cx - rx - 4, cy - ry - 1); }   // a rare shine
    // the form: what looking after it made of it
    if (st == GS_TEEN) { if (b->form == 0) { gP(e1 - 1, eyY - 1, 0); gP(e2 + 1, eyY - 1, 0); } else gP(cx + rx + 1, cy - ry + 1, 1); }
    if (st == GS_ADULT) switch (b->form) {
        case 0: gRC(cx - 2, gminY - 1, cx + 2, gminY - 1, 1); gP(cx - 2, gminY - 2, 1); gP(cx, gminY - 2, 1); gP(cx + 2, gminY - 2, 1); if (fr & 2) gstar(cx + rx + 3, cy - ry); break;
        case 1: gP(cx - rx - 2 + ((fr >> 1) & 3), gminY - 1, 1); gP(cx + rx + 2 - ((fr >> 1) & 3), gminY, 1); gP(e1, eyY + 2, 0); gP(e2, eyY + 2, 0); break;
        case 2: gP(cx + rx + 2, cy - 2, 1); gP(cx + rx + 3, cy - 3, 1); gP(cx + rx + 3, cy - 1, 1); if (fr & 2) gP(cx + rx + 4, cy - 4, 1); break;
        default: gstar(cx, gminY - 3); gstar(cx - rx - 3, cy); if (fr & 2) gstar(cx + rx + 3, cy - ry + 1); break;
    }
}
static void g_egg(const Blob *b, int fr) {
    int bob = fr & 1, cy = 10 - bob; gELL(16, cy, 4, 5, 1);
    switch (b->sp & 3) { case 0: gP(14, cy - 2, 0); gP(18, cy, 0); gP(15, cy + 2, 0); gP(17, cy - 3, 0); break; case 1: gRC(13, cy - 1, 19, cy - 1, 0); gRC(13, cy + 2, 19, cy + 2, 0); break;
        case 2: for (int x = 12; x < 21; x++) gP(x, cy + ((x & 1) ? 0 : 1), 0); break; default: gELL(14, cy - 1, 1, 1, 0); gELL(18, cy + 2, 1, 1, 0); break; }
    if (b->age >= 15) { gLN(16, cy - 5, 14, cy - 2, 0); gLN(14, cy - 2, 17, cy, 0); gLN(17, cy, 15, cy + 3, 0); }
}
static void g_scene(const Blob *b, int fr, int look) {
    for (int y = 0; y < 16; y++) for (int x = 0; x < 32; x++) gdot[y][x] = 0;
    gminY = 16;
    if (!b->on) return;
    int d = look ? look : ((fr >> 1) & 1 ? -1 : 1), ox = look ? 0 : (((fr >> 1) % 3) - 1) * 2;
    if (b->stage == GS_EGG) g_egg(b, fr); else g_creature(b, fr, d, ox);
    static const u8 pp[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 1, 0 }, { 1, 1, 1, 1 } };
    for (int i = 0; i < b->poop && i < 3; i++) { int px = 28 - (i & 1) * 4, py = 12 - (i >> 1) * 4; for (int r = 0; r < 4; r++) for (int q = 0; q < 4; q++) if (pp[r][q]) gP(px + q - 1, py + r, 1); }
    if (b->fl & GF_SICK) { static const char *sk[5] = { ".###.", "#.#.#", "#####", ".###.", ".#.#." }; for (int r = 0; r < 5; r++) for (int q = 0; q < 5; q++) if (sk[r][q] == '#') gP(1 + q, 1 + r, 1); }
    if (b->fl & GF_SLEEP) { static const char *z[5] = { "###.", "..#.", ".#..", "###.", "...." }; for (int r = 0; r < 5; r++) for (int q = 0; q < 4; q++) if (z[r][q] == '#') gP(24 + q + ((fr >> 2) & 1), 1 + r, 1); }
}

static const char *const giconNm[10] = { "FEED", "LIGHT", "PLAY", "MEDICINE", "CLEAN", "STATS", "SCOLD", "ATTENTION", "FRIENDS", "BLOBDEX" };
static const char *const gicon[10][7] = {
    { "...#.....", "..#.##...", ".#####...", "#######..", "#######..", ".#####...", "........." },
    { "...###...", "..#...#..", "..#...#..", "...#.#...", "...###...", "...###...", "........." },
    { ".#######.", "#..#....#", "#.###.#.#", "#..#..#.#", "#.......#", ".#######.", "........." },
    { "...###...", "...#.#...", ".###.###.", ".#.....#.", ".###.###.", "...#.#...", "...###..." },
    { "..#.#.#..", "...#.#...", "..#####..", "...###...", "...###...", "..#####..", "........." },
    { "#........", "#..#.....", "#..#..#..", "#.##..#..", "#.##.##..", "#########", "........." },
    { ".#####...", ".#...#...", ".#.#.#...", ".#####...", "...#.....", "...###...", "........." },
    { "...#.....", "..###....", "..###....", ".#####...", ".#####...", "#######..", "...#....." },
    { ".##...##.", "#..#.#..#", ".##...##.", "..#...#..", ".###.###.", "#..#.#..#", "........." },
    { "..#####..", ".#.....#.", "#.##.##.#", "#.#...#.#", "#.##.##.#", ".#.....#.", "..#####.." } };

static void gdots_draw(int x, int y, int s, u16 c) {   // the LCD's dots onto the screen: runs of them joined into one box
    for (int j = 0; j < 16; j++) for (int i = 0; i < 32;) { if (!gdot[j][i]) { i++; continue; } int k = i; while (k < 32 && gdot[j][k]) k++; rect(x + i * s, y + j * s, (k - i) * s - (s > 2), s - (s > 2), c); i = k; } }
#define LCD_BG C15(21, 24, 16)
#define LCD_INK C15(3, 5, 3)
static void gt_open_basket(int si) { Toy *t = gtoy_of(si); if (!t) { gsay(si == ctl ? "NO BOREAGOTCHI YET. A SHOP SELLS THEM" : "THEY HAVE NO TOY YET"); return; } gti = (int)(t - gtoy); gview = GV_BASKET; gbi = t->sel < GB_N ? t->sel : 0; gmsgT = 0; gmsg = 0; }
static void gt_open_a(int arg) {   // (D53) 0 the Toy Box, 1 your basket, 2 the Blobdex, 3 the guide
    glrng = 0xB0BE5u + (u32)minutes * 7; ganim = 0; gmsgT = 0; gmsg = 0; gdexpage = 0; gisel = 0; gsub = 0; mg_ui = 4; mg_ver++; gti = 0; gbi = 0; gbx = ctl < GT_MAX ? ctl : 0; gfrom_box = 0; gdexback = -1;
    if (arg == 3) { gview = GV_GUIDE; gguide = 0; gguideback = -1; return; }
    if (arg == 2) { gview = GV_DEX; gdexback = -1; return; }
    if (arg == 1 && gtoy_of(ctl)) { gt_open_basket(ctl); return; }
    { int any = 0; for (int k = 0; k < GT_MAX; k++) any |= gtoy[k].own; if (!any) { gview = GV_NONE; return; } }
    gview = GV_BOX; gfrom_box = 1; }
static void gt_open(void) { gt_open_a(0); }
static void gfinish_game(int won) { gview = GV_HAND; gsay(ggame(gtt(), gcb(), won)); voice(won ? V_YAHOO : V_NAH); }
static void guse_icon(void) {
    Blob *b = gcb(); Toy *t = gtt(); int sl = (b->fl & GF_SLEEP) != 0;
    if (b->stage == GS_EGG && gisel != 5 && gisel != 9) { gsay("IT IS STILL AN EGG. GIVE IT A MINUTE"); return; }
    if (sl && gisel != 1 && gisel != 5 && gisel != 3 && gisel != 9) { gsay("SHHH. IT IS ASLEEP"); return; }
    switch (gisel) {
    case 0: gsub = 0; gview = GV_FEED; break;
    case 1: gsay(glight(t, b)); break;
    case 2: gsub = 0; gview = GV_PLAYPICK; break;
    case 3: gsay(gmedicine(t, b)); break;
    case 4: gsay(gclean(t, b)); break;
    case 5: gsub = 0; gview = GV_STATS; break;
    case 6: gsay(gscold(t, b)); break;
    case 7: gsay(gneeds(b) ? gwant(b) : gnuzzle(t, b)); break;
    case 8: { int n = 0; for (int i = 0; i < GB_N; i++) if (i != gbi && t->b[i].on && t->b[i].stage > GS_EGG) n++; if (!n) { gsay("YOU NEED A SECOND ONE, HATCHED, TO PLAY WITH"); return; }
              gsub = gliving(t, gbi, 1); gview = GV_FRIEND; break; }
    default: gdexpage = 0; gview = GV_DEX; gdexback = GV_HAND; break;
    }
}
static int gt_update(int pr, int steps) {   // 1 while it runs (gotchi_update)
    gt_live(steps); ganim += steps; if (gmsgT > 0) { gmsgT -= steps; if (gmsgT <= 0) { gmsgT = 0; gmsg = 0; } }
    switch (gview) {
    case GV_BOX: { int n = MIN(nsim, GT_MAX); if (n < 1) return 0;
        if (pr & (PD_U | PD_L)) gbx = (gbx + n - 1) % n; if (pr & (PD_D | PD_R)) gbx = (gbx + 1) % n; if (gbx >= n) gbx = 0;
        if (pr & PD_SQ) { gview = GV_DEX; gdexpage = 0; gdexback = GV_BOX; }
        if (pr & PD_SEL) { gview = GV_GUIDE; gguide = 0; gguideback = GV_BOX; }
        if (pr & PD_O) return 0;
        if (pr & PD_X) { if (!gtoy_of(gbx) && gbx == ctl) { if (funds < GOTCHI_PRICE) { gsay("A TOY COSTS $5000"); voice(V_NAH); } else if (gbuy(ctl)) { funds -= GOTCHI_PRICE; say("A Boreagotchi and its first egg!"); gt_open_basket(ctl); gfrom_box = 1; voice(V_YAHOO); } }
                          else { gt_open_basket(gbx); if (gview == GV_BASKET) gfrom_box = 1; } }
        if (pr & PD_L1) o_gtime = (o_gtime + 1) % 3;   // the clock: the game's, LIVE, off
        break; }
    case GV_GUIDE:
        if (pr & (PD_R1 | PD_R | PD_X)) gguide = (gguide + 1) % GNP; if (pr & (PD_L1 | PD_L)) gguide = (gguide + GNP - 1) % GNP;
        if (pr & PD_O) { if (gguideback < 0) return 0; gview = gguideback; }
        break;
    case GV_NONE: if (pr & PD_O) return 0; if (pr & PD_X) { if (funds < GOTCHI_PRICE) { gsay("A TOY COSTS $5000"); voice(V_NAH); } else if (gbuy(ctl)) { funds -= GOTCHI_PRICE; say("A Boreagotchi and its first egg!"); gt_open(); voice(V_YAHOO); } } break;
    case GV_BASKET: { Toy *t = gtt();
        if (pr & PD_L) gbi = (gbi + GB_N - 1) % GB_N; if (pr & PD_R) gbi = (gbi + 1) % GB_N; if (pr & PD_U) gbi = (gbi + GB_N - 4) % GB_N; if (pr & PD_D) gbi = (gbi + 4) % GB_N;
        t->sel = (u8)gbi;
        if (pr & PD_SQ) { gview = GV_DEX; gdexpage = 0; gdexback = GV_BASKET; }
        if (pr & PD_SEL) { gview = GV_GUIDE; gguide = 0; gguideback = GV_BASKET; }
        if (pr & PD_R1) { gtree = 0; if (t->b[gbi].on) for (int j = 0; j < GLN; j++) if (t->ln[j].id == t->b[gbi].id) gtree = j; gview = GV_TREE; }
        if (pr & PD_O) { if (gfrom_box) gview = GV_BOX; else return 0; }
        if (pr & PD_X) { if (gsim_of(t) != ctl) { gsay("THAT IS NOT YOUR TOY. YOU CAN ONLY LOOK"); break; } if (!t->b[gbi].on) { if (funds < GOTCHI_EGG) { gsay("A NEW EGG COSTS $150"); voice(V_NAH); } else { funds -= GOTCHI_EGG; gegg(t, gbi); gsay("A NEW EGG! KEEP IT CLOSE"); voice(V_YAHOO); } } else { gview = GV_HAND; gisel = 0; gmsgT = 0; gmsg = 0; } }
        break; }
    case GV_HAND: { Toy *t = gtt();
        if (pr & PD_R) gisel = (gisel + 1) % 10; if (pr & PD_L) gisel = (gisel + 9) % 10; if (pr & (PD_U | PD_D)) gisel = (gisel + 5) % 10;
        if (pr & PD_R1) gbi = gliving(t, gbi, 1); if (pr & PD_L1) gbi = gliving(t, gbi, -1);
        t->sel = (u8)gbi;
        if (pr & PD_O) { gview = GV_BASKET; gmsgT = 0; gmsg = 0; }
        if (pr & PD_X) guse_icon();
        break; }
    case GV_FEED:
        if (pr & PD_R) gsub = (gsub + 1) % 3; if (pr & PD_L) gsub = (gsub + 2) % 3;
        if (pr & PD_O) { gview = GV_HAND; return 1; }
        if (pr & PD_X) { gsay(gsub == 2 ? glunch(gtt(), gcb()) : gfeed(gtt(), gcb(), gsub)); gview = GV_HAND; voice(V_YAHOO); }
        break;
    case GV_PLAYPICK:
        if (pr & (PD_L | PD_R)) gsub ^= 1; if (pr & PD_O) gview = GV_HAND;
        if (pr & PD_X) { if (gsub == 0) { gview = GV_LOOK; gmRound = 0; gmWon = 0; gmT = 0; gmN = 0; gmDir = 0; } else { gview = GV_COPY; gmRound = 0; gmWon = 0; gmN = 2; for (int i = 0; i < 6; i++) gseq[i] = (u8)(grr() & 3); gsidx = 0; gmT = -60; } }
        break;
    case GV_LOOK:   // WHICH WAY? it looks left or right: guess first. 5 rounds, 3 to win
        if (gmN == 0) { if (pr & PD_L) { gmDir = -1; gmN = 1; gmT = 0; } if (pr & PD_R) { gmDir = 1; gmN = 1; gmT = 0; } if (pr & PD_O) gview = GV_HAND; if (gmN) gsidx = (grr() & 1) ? 1 : 0; }
        else { gmT += steps; if (gmT > 50) { int way = gsidx ? 1 : -1; if (way == gmDir) gmWon++; gmRound++; gmN = 0; if (gmRound >= 5) gfinish_game(gmWon >= 3); } }
        break;
    case GV_COPY: {   // COPY ME: watch the blob, then do the same moves (the D-pad). Three rounds, two to win
        if (pr & PD_O) { gview = GV_HAND; break; }
        gmT += steps; if (gmT < 0) break; if (gmT < gmN * 40) break;
        int key = (pr & PD_U) ? 0 : (pr & PD_D) ? 1 : (pr & PD_L) ? 2 : (pr & PD_R) ? 3 : -1;
        if (key >= 0) { if (key == gseq[gsidx]) { gsidx++; voice(V_YEHA); if (gsidx >= gmN) { gmWon++; gmRound++; gsidx = 0; gmN++; gmT = -50; if (gmRound >= 3) gfinish_game(1); } }
                        else { gmRound++; gsidx = 0; gmT = -50; voice(V_NAH); if (gmRound - gmWon >= 2) gfinish_game(0); else if (gmRound >= 3) gfinish_game(gmWon >= 2); } }
        break; }
    case GV_FRIEND: { Toy *t = gtt();
        if (pr & PD_R) gsub = gliving(t, gsub, 1); if (pr & PD_L) gsub = gliving(t, gsub, -1); while (gsub == gbi) gsub = gliving(t, gsub, 1);
        if (pr & PD_O) gview = GV_HAND;
        if (pr & (PD_X | PD_SQ)) { const char *m = gfriends(t, gbi, gsub, (pr & PD_SQ) != 0); gview = GV_HAND; gsay(m); voice(V_YAHOO); }
        break; }
    case GV_STATS:
        if (pr & (PD_X | PD_R | PD_D)) { if (++gsub >= 2) gview = GV_HAND; }
        if (pr & (PD_O | PD_L)) { if (gsub > 0) gsub--; else gview = GV_HAND; }
        break;
    case GV_TREE: { Toy *t = gtt(); int d = (pr & (PD_R | PD_R1)) ? 1 : (pr & (PD_L | PD_L1)) ? -1 : 0;
        if (d) for (int k = 1; k <= GLN; k++) { int j = (gtree + d * k + GLN * 2) % GLN; if (t->ln[j].id) { gtree = j; break; } }
        if (pr & PD_O) gview = GV_BASKET;
        break; }
    case GV_DEX:
        if (pr & (PD_R1 | PD_R)) gdexpage = (gdexpage + 1) & 3; if (pr & (PD_L1 | PD_L)) gdexpage = (gdexpage + 3) & 3;
        if (pr & PD_O) { if (gdexback < 0) return 0; gview = gdexback; }
        break;
    }
    return 1;
}
// ---- drawing ----
static void gpips(int x, int y, int v, int n, u16 on) { for (int i = 0; i < n; i++) rect(x + i * 9, y, 7, 7, i < v ? on : C15(9, 7, 11)); }
static const char *const gformNm[4] = { "SUPER", "SLOB", "CHILL", "STAR" };
static void gwrap(int x, int y, int w, u16 c, const char *s) {   // two lines at most, cut at a space
    char b[72]; int n = (int)strlen(s); if (n > 70) n = 70; if (tw(1, s) <= w) { text(x, y, 1, c, s); return; }
    int cut = 0; for (int i = 0; i < n; i++) if (s[i] == ' ') { memcpy(b, s, (size_t)i); b[i] = 0; if (tw(1, b) <= w) cut = i; else break; }
    if (!cut) cut = n / 2; memcpy(b, s, (size_t)cut); b[cut] = 0; text(x, y, 1, c, b); text_fit(x, y + 10, 1, w, c, s + cut + (s[cut] == ' '));
}
typedef struct { const char *title; u8 strip; const char *tip[3]; } GPage;
static const GPage gpg[GNP] = {   // the PS1's guide (gotchi_ui.c gp[]), the same words
    { "MEET THE TOY", 0, { "A TINY PET ON A KEYCHAIN. IT LOVES YOU BACK.", "$5000 AT ELECTRONICS SHOPS.", "EVERYONE CAN OWN ONE. EIGHT PETS EACH." } },
    { "THE HANDHELD", 1, { "D-PAD PICKS, A USES. L R SWAP PETS.", "BELL LIT UP? A PET NEEDS YOU. HURRY!", "HUNGRY PETS BEEP EVEN WHEN YOU'RE BUSY." } },
    { "EIGHT PETS PER TOY", 2, { "THE BASKET HOLDS EIGHT. A NEW EGG IS $150.", "16 KINDS: CATS, ROBOTS, GHOSTS, ALIENS...", "HEARTS SHOW HOW CLOSE YOU TWO ARE." } },
    { "GROWING UP", 3, { "EGG, BABY, CHILD, TEEN, ADULT. ~2 DAYS.", "YOUR CARE DECIDES WHAT IT BECOMES.", "GROWN-UPS FILL THE BLOBDEX. GET THEM ALL!" } },
    { "PLAY, FRIENDS AND EGGS", 4, { "WIN WHICH WAY OR COPY ME TO CHEER IT UP.", "PLAYDATES CHEER BOTH. Y = BATTLE.", "BEST FRIENDS MAY LEAVE A SURPRISE EGG." } },
    { "HIJINKS: YOU AND THE PET", 5, { "LIVE MODE: REAL TIME. YOU MAY FORGET TO EAT!", "SAD OR STARVING? YOUR PETS FEEL IT TOO.", "SHARE YOUR LUNCH, OR GET A NUZZLE FOR FUN." } },
    { "MOODS AND CARE SKILL", 6, { "PETS CHANGE YOUR MOOD: PROUD PARENT, BUDDY...", "NEGLECT THEM AND YOU WORRY. LOSE ONE: OUCH.", "CARE SKILL GROWS: LESS HUNGER, LESS SICK." } },
    { "BUILD AND BUY THINGS", 7, { "THE GOTCH. TAB: NINE THINGS FOR HOME.", "DOCK OPENS THE BASKET. CABINET: BLOBDEX.", "BOWL, LAMP, BED CARE FOR ALL PETS AT ONCE." } },
    { "FAMILIES", 9, { "BEST FRIENDS BECOME A COUPLE, THEN LEAVE EGGS.", "BABIES GET COLOURS, NATURES, A RARE SHINE.", "PARENTS CARE FOR BABIES. ELDERS PASS ON." } },
    { "WHAT'S NEW", 8, { "NEW: FAMILY TREE, COUPLES, EGGS OF LOVE.", "NEW: NATURES, COLOURS, ELDERS, MEMORIAL.", "NEW: SIBLINGS PLAY, PARENTS CARE." } },
};
static void gtile(int x, int y, int sp, int stage, int look, int sc) {   // a pet on a little LCD
    Blob bb; memset(&bb, 0, sizeof bb); bb.on = 1; bb.sp = (u8)(sp % GSP_N); bb.stage = (u8)stage; bb.hun = bb.hap = 4; bb.age = (u16)(stage == GS_EGG ? 0 : 16); g_scene(&bb, ganim / 20, look);
    rect(x, y, 32 * sc + 2, 16 * sc + 2, LCD_BG); gdots_draw(x + 1, y + 1, sc, LCD_INK); }
static void guide_strip(int k) {   // the picture under each page, on the touch screen
    int y = 26, cyc = (ganim / 120) % GSP_N; rect(6, y - 4, 244, 112, C15(12, 8, 18)); char b[32];
    switch (k) {
    case 0: for (int i = 0; i < 3; i++) { int sp = (i * 4 + cyc) % GSP_N; gtile(14 + i * 80, y + 4, sp, GS_ADULT, 0, 2); text(14 + i * 80, y + 44, 1, TXT, gspNm[sp]); } text_fit(14, y + 70, 1, 228, C15(20, 24, 28), "Sixteen kinds. Each grows four ways."); break;
    case 1: { static const int ic[5] = { 0, 1, 3, 4, 6 }; for (int i = 0; i < 5; i++) { int x = 12 + i * 47; rect(x, y + 4, 43, 50, C15(11, 4, 14)); for (int r = 0; r < 7; r++) for (int q = 0; q < 9; q++) if (gicon[ic[i]][r][q] == '#') rect(x + 8 + q * 3, y + 10 + r * 3, 3, 3, C15(24, 16, 30)); t_tight = 1; text_fit(x + 2, y + 38, 1, 40, TXT, giconNm[ic[i]]); t_tight = 0; }
            text_fit(14, y + 70, 1, 228, C15(20, 24, 28), "Pick an icon, press A. The bell is yours."); break; }
    case 2: for (int i = 0; i < 3; i++) { int sp = (cyc + i * 3) % GSP_N; gtile(14 + i * 80, y + 4, sp, i == 0 ? GS_EGG : i == 1 ? GS_BABY : GS_ADULT, 0, 2); text(14 + i * 80, y + 44, 1, TXT, i == 0 ? "EGG" : i == 1 ? "BABY" : "ADULT"); }
            for (int h = 0; h < 4; h++) rect(70 + h * 30, y + 70, 24, 6, h < 3 ? C15(31, 8, 8) : C15(9, 7, 11)); break;
    case 3: { static const u8 st[5] = { GS_EGG, GS_BABY, GS_CHILD, GS_TEEN, GS_ADULT }; static const char *const nm[5] = { "EGG", "BABY", "CHILD", "TEEN", "ADULT" };
            for (int i = 0; i < 5; i++) { gtile(8 + i * 48, y + 14, cyc, st[i], 0, 1); text(8 + i * 48 + 4, y + 36, 1, TXT, nm[i]); } text_fit(14, y + 62, 1, 228, C15(20, 24, 28), "Care well: SUPER or STAR. Snacks: CHILL."); break; }
    case 4: gtile(20, y + 6, cyc, GS_ADULT, 1, 2); gtile(166, y + 6, (cyc + 5) % GSP_N, GS_ADULT, -1, 2); { int hb = (ganim / 15) & 1; rect(112, y + 14, 12, 8, hb ? C15(31, 8, 8) : C15(20, 6, 6)); rect(116, y + 22, 4, 4, hb ? C15(31, 8, 8) : C15(20, 6, 6)); }
            text(86, y + 50, 1, TXT, "PLAYDATE OR BATTLE"); break;
    case 5: text(12, y + 8, 1, C15(31, 25, 6), "YOU: TUMMY"); rect(12, y + 22, 70, 7, C15(9, 7, 11)); rect(12, y + 22, 70 * (o_gtime == 1 ? 3 : 6) / 8, 7, C15(31, 22, 8)); gtile(170, y + 6, cyc, GS_ADULT, 0, 2);
            text(96, y + 8, 1, TXT, "SHARE LUNCH"); text_fit(12, y + 50, 1, 228, C15(31, 14, 12), "You starve, it starves. It lifts you too."); break;
    case 6: { static const char *const m[6] = { "IT HATCHED! +16", "PROUD PARENT +14", "BLOB BUDDY +10", "WORRIED -8", "HOOKED ON THE TOY -8", "MISSING MY BLOB -15" };
            for (int i = 0; i < 6; i++) { int x = 8 + (i & 1) * 122, yy = y + 4 + (i >> 1) * 22; rect(x, yy, 118, 18, i < 3 ? C15(6, 18, 8) : C15(22, 6, 6)); text_fit(x + 4, yy + 5, 1, 112, TXT, m[i]); } break; }
    case 7: { static const char *const o[9] = { "DOCK $120", "SHELF $180", "CABINET $300", "PLUSH $60", "BALL $40", "BED $90", "BOWL $40", "LAMP $70", "TROPHY $200" };
            for (int i = 0; i < 9; i++) { int x = 8 + (i % 3) * 82, yy = y + 4 + (i / 3) * 22; rect(x, yy, 78, 18, C15(8, 12, 24)); text_fit(x + 4, yy + 5, 1, 72, TXT, o[i]); } break; }
    case 9: { static const u8 sp3[3] = { 1, 7, 3 }; for (int i = 0; i < 3; i++) { gtile(20 + i * 76, y + 30 + (i == 1) * 0, (cyc + sp3[i]) % GSP_N, i == 2 ? GS_BABY : GS_ADULT, 0, 2); }
            { int hb = (ganim / 15) & 1; rect(88, y + 36, 8, 6, hb ? C15(31, 8, 8) : C15(20, 6, 6)); rect(90, y + 42, 4, 3, hb ? C15(31, 8, 8) : C15(20, 6, 6)); }
            text(20, y + 6, 1, C15(31, 25, 6), "PARENT"); text(96, y + 6, 1, C15(31, 25, 6), "PARENT"); text(172, y + 6, 1, TXT, "CHILD"); text_fit(14, y + 74, 1, 228, C15(20, 24, 28), "Press Family in the basket to see the tree."); break; }
    default: text(14, y + 6, 1, TXT, "NO MISSIONS: JUST YOU AND THE PETS."); text(14, y + 20, 1, C15(20, 24, 28), "The toy box lists every housemate's toy."); sprintf(b, "TIME: %s", o_gtime == 0 ? "THE GAME CLOCK" : o_gtime == 1 ? "LIVE" : "OFF"); text(14, y + 34, 1, C15(31, 25, 6), b);
            rect(14, y + 56, 64, 16, C15(6, 18, 8)); text(20, y + 60, 1, TXT, "16 KINDS"); rect(86, y + 56, 64, 16, C15(24, 16, 4)); text(92, y + 60, 1, TXT, "64 FORMS"); rect(158, y + 56, 64, 16, C15(8, 12, 24)); text(164, y + 60, 1, TXT, "8 PER TOY"); break;
    }
}
static const char *gclock_name(void) { return o_gtime == 0 ? "the game clock" : o_gtime == 1 ? "LIVE (real time in hand)" : "off (they wait)"; }
static void gt_top(void) {
    scr = offbuf; char b[72]; rect(0, 0, 256, 192, K8(22, 12, 52));
    for (int i = 0; i < 24; i++) { int x = (i * 97 + 13) % 256, y = (i * 61 + 7) % 192; rect(x, y, ((ganim >> 3) + i) % 7 ? 1 : 2, ((ganim >> 3) + i) % 7 ? 1 : 2, C15(24, 24, 31)); }
    panel(4, 4, 248, 184, "Boreagotchi");
    if (gview == GV_NONE) { text_fit(12, 40, 1, 228, TXT, "A Boreagotchi: a pet on a keychain."); text_fit(12, 56, 1, 228, TXT, "Eight creatures, 16 kinds, 64 forms."); text_fit(12, 72, 1, 228, TXT, "It grows up as you care for it."); sprintf(b, "The toy costs $%d.", GOTCHI_PRICE); text(12, 96, 1, C15(31, 25, 6), b); if (gmsg) text(12, 150, 1, C15(31, 12, 10), gmsg); return; }
    if (gview == GV_DEX) {
        sprintf(b, "BLOBDEX  %d OF %d   PAGE %d OF 4", gdex_count(), GSP_N * 4, gdexpage + 1); text(12, 26, 1, C15(31, 25, 6), b);
        for (int r = 0; r < 4; r++) { int sp = gdexpage * 4 + r, y = 36 + r * 34; text(10, y + 12, 1, TXT, gspNm[sp]);
            for (int f = 0; f < 4; f++) { int x = 70 + f * 46; rect(x - 1, y - 1, 45, 32, f == 0 ? C15(29, 22, 5) : f == 1 ? C15(18, 14, 8) : f == 2 ? C15(7, 23, 26) : C15(27, 8, 22)); rect(x, y, 43, 30, LCD_BG);
                if (gdex_has(sp, f)) { Blob bb; memset(&bb, 0, sizeof bb); bb.on = 1; bb.stage = GS_ADULT; bb.sp = (u8)sp; bb.form = (u8)f; bb.hun = bb.hap = 4; g_scene(&bb, 0, 0); gdots_draw(x + 5, y + 7, 1, LCD_INK); } else text(x + 18, y + 12, 1, C15(12, 14, 12), "?"); } }
        text(12, 174, 1, C15(20, 24, 28), "Care: SUPER, STAR. Snacks: CHILL."); return; }
    if (gview == GV_TREE) { Toy *t = gtt(); const Ln *me = &t->ln[gtree]; char b2[80]; int ids[4][5], nn[4];
        for (int r = 0; r < 4; r++) { nn[r] = 0; for (int q = 0; q < 5; q++) ids[r][q] = 0; }
        const Ln *m = gln_of(t, me->mom), *f = gln_of(t, me->dad);
        if (m) { if (m->mom) ids[0][nn[0]++] = m->mom; if (m->dad) ids[0][nn[0]++] = m->dad; } if (f) { if (f->mom) ids[0][nn[0]++] = f->mom; if (f->dad) ids[0][nn[0]++] = f->dad; }
        if (me->mom) ids[1][nn[1]++] = me->mom; if (me->dad) ids[1][nn[1]++] = me->dad;
        ids[2][nn[2]++] = me->id; for (int i = 0; i < GLN && nn[2] < 5; i++) { const Ln *o = &t->ln[i]; if (o->id && o->id != me->id && ((me->mom && o->mom == me->mom) || (me->dad && o->dad == me->dad))) ids[2][nn[2]++] = o->id; }
        for (int i = 0; i < GLN && nn[3] < 5; i++) { const Ln *o = &t->ln[i]; if (o->id && (o->mom == me->id || o->dad == me->id)) ids[3][nn[3]++] = o->id; }
        static const char *const rl[4] = { "GRANDPARENTS", "PARENTS", "YOU AND SIBLINGS", "CHILDREN" };
        for (int r = 0; r < 4; r++) { int y = 18 + r * 37; (void)rl;
            for (int q = 0; q < nn[r]; q++) { const Ln *o = gln_of(t, ids[r][q]); int x = 8 + q * 48, hl = o == me; if (!o) continue; Blob bb; memset(&bb, 0, sizeof bb); bb.on = 1; bb.stage = GS_ADULT; bb.sp = o->sp; bb.hun = bb.hap = 4; g_scene(&bb, 0, 0);
                rect(x - 1, y + 8, 46, 20, hl ? C15(31, 25, 6) : C15(9, 7, 11)); rect(x, y + 9, 44, 18, LCD_BG); gdots_draw(x + 6, y + 10, 1, (o->nm & 128) ? C15(14, 14, 14) : gcolInk[o->col & 7]);
                text_fit(x + 1, y + 27, 1, 44, (o->nm & 128) ? C15(16, 16, 16) : TXT, ggiv[o->nm & 31]); if (o->nm & 128) text(x + 36, y + 10, 1, C15(31, 31, 31), "+"); } }
        sprintf(b2, "%s THE %s %s  GEN %d%s", ggiv[me->nm & 31], gcolNm[me->col & 7], gspNm[me->sp % GSP_N], 1, (me->nm & 128) ? "  (PASSED ON)" : ""); { Blob *lv = gby_id(t, me->id); if (lv) sprintf(b2, "%s THE %s %s  GEN %d", ggiv[me->nm & 31], gcolNm[me->col & 7], gspNm[me->sp % GSP_N], lv->gen + 1); }
        text_fit(8, 170, 1, 240, TXT, b2); if (t->mem) { sprintf(b2, "MEMORIAL: %d REMEMBERED. LAST: %s", t->mem, ggiv[t->memnm & 31]); text_fit(8, 180, 1, 240, C15(20, 24, 28), b2); } return; }
    if (gview == GV_BOX) { Toy *bt = gtoy_of(gbx); int n = MIN(nsim, GT_MAX); if (gbx >= n) gbx = 0; bt = gtoy_of(gbx);
        text_fit(12, 28, 1, 232, C15(31, 25, 6), sname[gbx < nsim ? gbx : 0]);
        if (bt) { sprintf(b, "LV %d  %s", glevel(bt), glvNm[glevel(bt)]); text(12, 44, 1, TXT, b); sprintf(b, "%d pets   %d calling", gcount(bt), gcalling(bt)); text(12, 58, 1, gcalling(bt) ? C15(31, 14, 12) : C15(20, 28, 20), b); sprintf(b, "%d hatched   %d wins", bt->hatched, bt->wins); text(12, 72, 1, C15(20, 24, 28), b); }
        else text_fit(12, 44, 1, 232, C15(20, 24, 28), gbx == ctl ? "No toy yet: $5000 at a shop." : "No toy yet.");
        text(12, 100, 1, TXT, "Pets live on:"); text_fit(12, 114, 1, 232, C15(31, 25, 6), gclock_name());
        sprintf(b, "BLOBDEX %d OF %d", gdex_count(), GSP_N * 4); text(12, 140, 1, C15(31, 25, 6), b); rect(12, 154, 150, 5, C15(9, 7, 11)); rect(12, 154, 150 * gdex_count() / (GSP_N * 4), 5, C15(29, 22, 5));
        if (gmsg) text_fit(12, 168, 1, 232, C15(31, 12, 10), gmsg); return; }
    if (gview == GV_GUIDE) { const GPage *g = &gpg[gguide]; text_fit(12, 24, 1, 160, C15(31, 25, 6), g->title); sprintf(b, "PAGE %d OF %d", gguide + 1, GNP); text(176, 24, 1, C15(20, 24, 28), b);
        for (int i = 0; i < 3; i++) { rect(10, 44 + i * 44, 236, 38, K8(36, 16, 70)); rect(14, 54 + i * 44, 6, 6, C15(8, 20, 31)); gwrap(26, 48 + i * 44, 214, TXT, g->tip[i]); }
        for (int k = 0; k < GNP; k++) rect(128 - (GNP - 1) * 6 + k * 12 - 3, 176, 6, 6, k == gguide ? C15(8, 20, 31) : C15(9, 7, 11)); return; }
    Toy *t = gtt(); Blob *c = gcb(); int dark = (c->fl & GF_SLEEP) && (c->fl & GF_DARK);
    rect(14, 24, 228, 112, K8(26, 10, 40)); rect(18, 28, 220, 104, dark ? C15(8, 10, 8) : LCD_BG);
    if (gview == GV_FRIEND) { c = &t->b[gsub]; }
    int look = gview == GV_LOOK ? (gmN ? (gsidx ? 1 : -1) : 0) : 0, jump = 0;
    if (gview == GV_COPY) { int showing = gmT >= 0 && gmT < gmN * 40, k = showing ? gmT / 40 : -1; if (showing) { int m = gseq[k]; look = m == 2 ? -1 : m == 3 ? 1 : 0; jump = m == 0; } }
    if (gview == GV_STATS) { Blob *s = c; text(26, 34, 1, LCD_INK, "");
        if (gsub == 0) { text(26, 34, 1, LCD_INK, "HUNGER"); gpips(26, 48, s->hun, 4, LCD_INK); text(26, 66, 1, LCD_INK, "HAPPY"); gpips(26, 80, s->hap, 4, LCD_INK); text(140, 34, 1, LCD_INK, "BOND"); for (int i = 0; i < 4; i++) rect(140 + i * 10, 48, 8, 8, i < (s->bond + 12) / 25 ? C15(26, 4, 4) : C15(14, 17, 10)); sprintf(b, "%d GRAMS", s->wt); text(140, 66, 1, LCD_INK, b); }
        else { gname(s, b); text(26, 34, 1, LCD_INK, b); sprintf(b, "AGE %d HOURS", s->age / 60); text(26, 48, 1, LCD_INK, b); sprintf(b, "DISCIPLINE %d%%", s->disc); text(26, 62, 1, LCD_INK, b); sprintf(b, "MISTAKES %d  GEN %d", s->miss, s->gen + 1); text(26, 76, 1, LCD_INK, b); sprintf(b, "%s  %s%s", gpersNm[s->pers & 7], gcolNm[s->col & 7], s->mut ? "  SHINY" : ""); text(26, 90, 1, LCD_INK, b); sprintf(b, "YOU: %s", glvNm[glevel(t)]); text(26, 104, 1, LCD_INK, b);
            int nx = gnext_xp(t); if (nx) { rect(26, 118, 120, 5, C15(14, 17, 10)); rect(26, 118, 120 * t->xp / nx, 5, LCD_INK); } } }
    else { g_scene(c, ganim / 20 + (jump ? 1 : 0), look); gdots_draw(26, 32 - (jump ? 6 : 0), 6, dark ? C15(14, 18, 12) : gcolInk[c->col & 7]); }
    if (o_gtime == 1) text(206, 30, 1, C15(26, 4, 4), "LIVE"); else if (o_gtime == 2) text(192, 30, 1, C15(12, 16, 12), "PAUSED");
    gname(c, b); { char b2[80]; sprintf(b2, "%s THE %s", ggiv[c->nm & 31], b); text(128 - tw(1, b2) / 2, 142, 1, TXT, b2); }
    text(20, 156, 1, C15(31, 22, 8), "HUNGER"); gpips(66, 154, c->hun, 4, C15(31, 22, 8)); text(112, 156, 1, C15(31, 14, 24), "HAPPY"); gpips(152, 154, c->hap, 4, C15(31, 14, 24));
    for (int i = 0; i < 4; i++) rect(196 + i * 11, 154, 9, 7, i < (c->bond + 12) / 25 ? C15(31, 8, 8) : C15(9, 7, 11));
    { const char *w = c->stage == GS_EGG ? "AN EGG" : gneeds(c) ? gwant(c) : "ALL GOOD"; text(20, 172, 1, gneeds(c) ? C15(31, 14, 12) : C15(20, 28, 20), w); }
    if (gview == GV_LOOK) { sprintf(b, "ROUND %d  WINS %d", gmRound + 1, gmWon); text(140, 172, 1, C15(31, 31, 18), b); }
    if (gview == GV_COPY) { sprintf(b, "ROUND %d  GOT %d", gmRound + 1, gmWon); text(140, 172, 1, C15(31, 31, 18), b); }
    if (gmsg || gview == GV_LOOK || gview == GV_COPY) { const char *m = gmsg; static char m2[40];
        if (gview == GV_LOOK && !gmsg) m = gmN == 0 ? "WHICH WAY WILL IT LOOK?" : ((gsidx ? 1 : -1) == gmDir ? "YES! IT LOOKED YOUR WAY" : "NOPE. THE OTHER WAY");
        if (gview == GV_COPY && !gmsg) { static const char *nm[4] = { "UP", "DOWN", "LEFT", "RIGHT" }; int showing = gmT >= 0 && gmT < gmN * 40; if (gmT < 0) m = "WATCH CLOSELY..."; else if (showing) m = nm[gseq[gmT / 40]]; else { sprintf(m2, "YOUR TURN: %d OF %d", gsidx + 1, gmN); m = m2; } }
        if (m) { rect(10, 124, 236, 14, K8(24, 10, 40)); text_fit(14, 128, 1, 228, TXT, m); } }
}
static unsigned gt_bsig(void) { return (unsigned)(gtree * 59 + gbx * 41 + gguide * 43 + o_gtime * 47 + gview * 31 + gbi * 7 + gisel * 11 + gsub * 13 + gdexpage * 17 + gmN * 19 + (gview == GV_COPY ? gmT >= 0 && gmT < gmN * 40 : 0) + (funds >= GOTCHI_PRICE) * 23 + (gtt()->b[gbi].on) * 29); }
static void gt_bottom(void) {
    scr = offbuf; char b[40]; rect(0, 0, 256, 192, K8(22, 12, 52)); rect(0, 0, 256, 16, K8(60, 30, 110)); ctext(128, 4, 1, K8(90, 230, 240), "BOREAGOTCHI");
    if (gview == GV_NONE) { chip(14, 60, 228, 50, funds >= GOTCHI_PRICE ? "Buy a toy $5000" : "Toy $5000 (not enough)", funds >= GOTCHI_PRICE); chip(14, 150, 228, 30, "Back", 0); return; }
    if (gview == GV_BOX) { int n = MIN(nsim, GT_MAX);
        for (int i = 0; i < n; i++) { Toy *bt = gtoy_of(i); int y = 20 + i * 16, sl = i == gbx; rect(8, y, 240, 15, sl ? C15(31, 31, 20) : PANB); rect(9, y + 1, 238, 13, sl ? C15(24, 28, 31) : PAN);
            text_fit(14, y + 4, 1, 90, sl ? C15(2, 7, 20) : TXT, sname[i]);
            if (bt) { for (int q = 0; q < GB_N; q++) { const Blob *p = &bt->b[q]; rect(108 + q * 10, y + 4, 8, 8, !p->on ? C15(9, 7, 11) : gneeds(p) ? C15(29, 6, 6) : p->stage == GS_EGG ? C15(28, 20, 4) : C15(6, 24, 8)); }
                if (gcalling(bt)) { sprintf(b, "%d!", gcalling(bt)); text(194, y + 4, 1, C15(29, 6, 6), b); } else text(194, y + 4, 1, C15(8, 22, 10), "ok"); sprintf(b, "L%d", glevel(bt)); text(224, y + 4, 1, sl ? C15(2, 7, 20) : C15(20, 24, 28), b); }
            else text(108, y + 4, 1, sl ? C15(8, 14, 24) : C15(14, 18, 24), i == ctl ? "no toy: $5000" : "no toy"); }
        chip(6, 154, 42, 30, !gtoy_of(gbx) && gbx == ctl ? "Buy" : "Open", 1); chip(50, 154, 36, 30, "Dex", 0); chip(88, 154, 52, 30, "Guide", 0);
        chip(142, 154, 56, 30, o_gtime == 0 ? "T:Game" : o_gtime == 1 ? "T:LIVE" : "T:Off", 0); chip(200, 154, 50, 30, "Back", 0); return; }
    if (gview == GV_GUIDE) { guide_strip(gpg[gguide].strip); chip(8, 150, 70, 34, "< Page", 0); chip(84, 150, 88, 34, "Back", 1); chip(178, 150, 70, 34, "Page >", 0); return; }
    Toy *t = gtt();
    if (gview == GV_TREE) { chip(8, 40, 70, 60, "< Prev", 0); chip(178, 40, 70, 60, "Next >", 0); chip(78, 110, 100, 50, "Back", 1); return; }
    if (gview == GV_BASKET || gview == GV_DEX) {
        if (gview == GV_DEX) { chip(8, 30, 70, 40, "< Page", 0); chip(178, 30, 70, 40, "Page >", 0); chip(78, 100, 100, 40, "Back", 1); return; }
        for (int i = 0; i < GB_N; i++) { const Blob *c = &t->b[i]; int x = 8 + (i & 3) * 61, y = 20 + (i >> 2) * 66, s = i == gbi;
            rect(x, y, 59, 63, s ? C15(31, 31, 20) : PANB); rect(x + 1, y + 1, 57, 61, s ? C15(24, 28, 31) : PAN);
            if (c->on) { g_scene(c, ganim / 20 + i, 0); rect(x + 2, y + 3, 55, 28, LCD_BG); gdots_draw(x + 2 + 12, y + 3 + 6, 1, gcolInk[c->col & 7]); if (gneeds(c)) { rect(x + 46, y + 3, 11, 11, C15(29, 6, 6)); text(x + 50, y + 5, 1, TXT, "!"); }
                text_fit(x + 3, y + 36, 1, 54, s ? C15(2, 7, 20) : TXT, ggiv[c->nm & 31]); gname(c, b); text_fit(x + 3, y + 47, 1, 54, s ? C15(8, 14, 24) : C15(20, 24, 28), b); for (int h = 0; h < 4; h++) rect(x + 5 + h * 12, y + 57, 9, 3, h < (c->bond + 12) / 25 ? C15(31, 8, 8) : C15(9, 7, 11)); }
            else { text(x + 13, y + 20, 1, s ? C15(2, 7, 20) : C15(14, 18, 24), s ? "NEW EGG" : "EMPTY"); text(x + 17, y + 36, 1, C15(31, 25, 6), "$150"); } }
        chip(8, 154, 48, 30, "Dex", 0); chip(58, 154, 48, 30, "Guide", 0); chip(108, 154, 62, 30, "Family", 0); sprintf(b, "LV %d", glevel(t)); chip(172, 154, 36, 30, b, 0); chip(210, 154, 38, 30, "Back", 0); return; }
    if (gview == GV_HAND || gview == GV_STATS || gview == GV_FEED || gview == GV_PLAYPICK || gview == GV_LOOK || gview == GV_COPY || gview == GV_FRIEND) {
        if (gview == GV_HAND) {
            for (int i = 0; i < 10; i++) { int x = 8 + (i % 5) * 49, y = 22 + (i / 5) * 56, s = i == gisel, lit = i == 7 && (gcb()->fl & GF_CALL);
                rect(x, y, 47, 52, s ? C15(31, 31, 31) : PANB); rect(x + 1, y + 1, 45, 50, s ? C15(26, 29, 31) : lit ? C15(31, 22, 6) : C15(11, 4, 14));
                u16 ic = s ? C15(16, 8, 4) : lit ? C15(4, 8, 14) : C15(24, 16, 30); for (int r = 0; r < 7; r++) for (int q = 0; q < 9; q++) if (gicon[i][r][q] == '#') rect(x + 8 + q * 3, y + 7 + r * 3, 3, 3, ic);
                t_tight = 1; text_fit(x + 23 - MIN(tw(1, giconNm[i]), 44) / 2, y + 40, 1, 44, s ? C15(2, 7, 20) : TXT, giconNm[i]); t_tight = 0; }
            chip(8, 138, 60, 20, "< Pet", 0); chip(190, 138, 58, 20, "Pet >", 0); chip(72, 138, 114, 20, "Use", 1); chip(8, 164, 100, 22, "Basket", 0); return; }
        if (gview == GV_FEED) { chip(8, 30, 74, 50, "Meal", gsub == 0); chip(91, 30, 74, 50, "Snack", gsub == 1); chip(174, 30, 74, 50, "Lunch", gsub == 2);
            static const char *const bl[3] = { "A meal fills it up", "A snack cheers it up", "Your own lunch: you get hungrier" }; text_fit(14, 100, 1, 228, TXT, bl[gsub]); chip(8, 150, 108, 30, "Give", 1); chip(140, 150, 108, 30, "Back", 0); return; }
        if (gview == GV_PLAYPICK) { chip(8, 30, 114, 60, "Which way?", gsub == 0); chip(134, 30, 114, 60, "Copy me", gsub == 1); text_fit(14, 108, 1, 228, TXT, gsub ? "Repeat its moves. 3 rounds." : "Guess where it looks. 5 rounds."); chip(8, 150, 108, 30, "Play", 1); chip(140, 150, 108, 30, "Back", 0); return; }
        if (gview == GV_LOOK) { chip(8, 30, 114, 90, "< Left", gmN == 0 && gmDir == -1); chip(134, 30, 114, 90, "Right >", gmN == 0 && gmDir == 1); chip(140, 150, 108, 30, "Quit", 0); return; }
        if (gview == GV_COPY) { chip(88, 24, 80, 44, "Up", 0); chip(88, 100, 80, 44, "Down", 0); chip(8, 62, 76, 44, "Left", 0); chip(172, 62, 76, 44, "Right", 0); chip(140, 154, 108, 28, "Quit", 0); return; }
        if (gview == GV_FRIEND) { chip(8, 30, 60, 50, "<", 0); chip(188, 30, 60, 50, ">", 0); chip(8, 100, 112, 44, "Playdate", 1); chip(136, 100, 112, 44, "Battle", 0); chip(140, 154, 108, 28, "Back", 0); return; }
        chip(8, 60, 114, 60, "Next", 1); chip(134, 60, 114, 60, "Back", 0); return; }
}
static void gt_frame(int held, int down, int steps, int tx, int ty) {
    (void)held; int pr = 0, td = down & KEY_TOUCH;
    if (down & KEY_LEFT) pr |= PD_L; if (down & KEY_RIGHT) pr |= PD_R; if (down & KEY_UP) pr |= PD_U; if (down & KEY_DOWN) pr |= PD_D; if (down & KEY_A) pr |= PD_X; if (down & KEY_B) pr |= PD_O; if (down & KEY_Y) pr |= PD_SQ; if (down & KEY_L) pr |= PD_L1; if (down & KEY_R) pr |= PD_R1;
    if (down & KEY_SELECT) pr |= PD_SEL;
    if (td) {
        if (gview == GV_BOX) { int n = MIN(nsim, GT_MAX); for (int i = 0; i < n; i++) if (hit(8, 20 + i * 16, 240, 15, tx, ty)) { if (gbx == i) pr |= PD_X; gbx = i; }
            if (hit(6, 154, 42, 30, tx, ty)) pr |= PD_X; if (hit(50, 154, 36, 30, tx, ty)) pr |= PD_SQ; if (hit(88, 154, 52, 30, tx, ty)) pr |= PD_SEL; if (hit(142, 154, 56, 30, tx, ty)) pr |= PD_L1; if (hit(200, 154, 50, 30, tx, ty)) pr |= PD_O; }
        else if (gview == GV_GUIDE) { if (hit(8, 150, 70, 34, tx, ty)) pr |= PD_L; if (hit(178, 150, 70, 34, tx, ty)) pr |= PD_R; if (hit(84, 150, 88, 34, tx, ty)) pr |= PD_O; }
        else if (gview == GV_NONE) { if (hit(14, 60, 228, 50, tx, ty)) pr |= PD_X; if (hit(14, 150, 228, 30, tx, ty)) pr |= PD_O; }
        else if (gview == GV_DEX) { if (hit(8, 30, 70, 40, tx, ty)) pr |= PD_L1; if (hit(178, 30, 70, 40, tx, ty)) pr |= PD_R1; if (hit(78, 100, 100, 40, tx, ty)) pr |= PD_O; }
        else if (gview == GV_BASKET) { for (int i = 0; i < GB_N; i++) if (hit(8 + (i & 3) * 61, 20 + (i >> 2) * 66, 59, 63, tx, ty)) { if (gbi == i) pr |= PD_X; gbi = i; }
            if (hit(8, 154, 48, 30, tx, ty)) pr |= PD_SQ; if (hit(58, 154, 48, 30, tx, ty)) pr |= PD_SEL; if (hit(108, 154, 62, 30, tx, ty)) pr |= PD_R1; if (hit(210, 154, 38, 30, tx, ty)) pr |= PD_O; }
        else if (gview == GV_TREE) { if (hit(8, 40, 70, 60, tx, ty)) pr |= PD_L; if (hit(178, 40, 70, 60, tx, ty)) pr |= PD_R; if (hit(78, 110, 100, 50, tx, ty)) pr |= PD_O; }
        else if (gview == GV_HAND) { for (int i = 0; i < 10; i++) if (hit(8 + (i % 5) * 49, 22 + (i / 5) * 56, 47, 52, tx, ty)) { gisel = i; pr |= PD_X; }
            if (hit(8, 138, 60, 20, tx, ty)) pr |= PD_L1; if (hit(190, 138, 58, 20, tx, ty)) pr |= PD_R1; if (hit(72, 138, 114, 20, tx, ty)) pr |= PD_X; if (hit(8, 164, 100, 22, tx, ty)) pr |= PD_O; }
        else if (gview == GV_FEED) { for (int i = 0; i < 3; i++) if (hit(8 + i * 83, 30, 74, 50, tx, ty)) gsub = i; if (hit(8, 150, 108, 30, tx, ty)) pr |= PD_X; if (hit(140, 150, 108, 30, tx, ty)) pr |= PD_O; }
        else if (gview == GV_PLAYPICK) { if (hit(8, 30, 114, 60, tx, ty)) gsub = 0; if (hit(134, 30, 114, 60, tx, ty)) gsub = 1; if (hit(8, 150, 108, 30, tx, ty)) pr |= PD_X; if (hit(140, 150, 108, 30, tx, ty)) pr |= PD_O; }
        else if (gview == GV_LOOK) { if (hit(8, 30, 114, 90, tx, ty)) pr |= PD_L; if (hit(134, 30, 114, 90, tx, ty)) pr |= PD_R; if (hit(140, 150, 108, 30, tx, ty)) pr |= PD_O; }
        else if (gview == GV_COPY) { if (hit(88, 24, 80, 44, tx, ty)) pr |= PD_U; if (hit(88, 100, 80, 44, tx, ty)) pr |= PD_D; if (hit(8, 62, 76, 44, tx, ty)) pr |= PD_L; if (hit(172, 62, 76, 44, tx, ty)) pr |= PD_R; if (hit(140, 154, 108, 28, tx, ty)) pr |= PD_O; }
        else if (gview == GV_FRIEND) { if (hit(8, 30, 60, 50, tx, ty)) pr |= PD_L; if (hit(188, 30, 60, 50, tx, ty)) pr |= PD_R; if (hit(8, 100, 112, 44, tx, ty)) pr |= PD_X; if (hit(136, 100, 112, 44, tx, ty)) pr |= PD_SQ; if (hit(140, 154, 108, 28, tx, ty)) pr |= PD_O; }
        else if (gview == GV_STATS) { if (hit(8, 60, 114, 60, tx, ty)) pr |= PD_X; if (hit(134, 60, 114, 60, tx, ty)) pr |= PD_O; } }
    int og = gview, ob = gbi, oi = gisel, os = gsub;
    if (!gt_update(pr, steps)) { mg_ui = 0; mg_ver++; return; }
    if (pr || og != gview || ob != gbi || oi != gisel || os != gsub || gview == GV_LOOK || gview == GV_COPY || (ganim & 15) < steps) mg_ver++;
}

// ---- (D53) the Boreagotchi things in the house and the shop that sells the toy ----
static int gt_testshop;
static int in_shop(void) { BInfo b; blk(lot_bx, lot_by, &b);
#ifdef DSTEST
    if (gt_testshop) return 1;
#endif
    return b.lot == LOT_SHOP; }
static int gtoy_of_(int si) { return gtoy_of(si) != 0; }
static const char *gt_house(int si, int what) {   // the PS1's gotchi_house: the bowl, the lamp, the bed care for every pet at once
    Toy *t = gtoy_of(si); if (!t) return "NO BOREAGOTCHI"; int n = 0;
    for (int i = 0; i < GB_N; i++) { Blob *b = &t->b[i]; if (!b->on || b->stage == GS_EGG) continue;
        if (what == 0) { if (b->hun < 4 && !(b->fl & GF_SLEEP)) { b->hun++; if (b->wt < 99) b->wt++; if (b->meal < 250) b->meal++; n++; } }
        else if (what == 1) { b->fl ^= GF_DARK; n++; }
        else if ((b->fl & GF_SLEEP) && !(b->fl & GF_DARK)) { b->fl |= GF_DARK; if (b->bond < 100) b->bond++; n++; } }
    if (n) gxp(t, 1);
    return what == 0 ? (n ? "They all tucked in. Nom nom" : "Nobody is hungry") : what == 1 ? "Click. Lights changed" : (n ? "Tucked in. Sweet dreams" : "Nobody needs tucking in"); }
static void gt_furn(int cd) {   // 90 open the basket, 91 the Toy Box, 92 the Blobdex, 93 tuck in, 94 feed, 95 lights, 96 buy the toy (a shop's dock)
    if (cd == 96) { if (funds < GOTCHI_PRICE) { say("A Boreagotchi costs $5000"); voice(V_NAH); } else if (gtoy_of(ctl)) say("You have one already"); else if (gbuy(ctl)) { funds -= GOTCHI_PRICE; say("A Boreagotchi! It came with an egg"); voice(V_YAHOO); gt_open_a(1); } return; }
    if (cd == 91) { gt_open_a(0); return; } if (cd == 92) { gt_open_a(2); return; }
    if (cd == 90) { if (!gtoy_of(ctl)) say("No Boreagotchi yet: a shop sells them"); else gt_open_a(1); return; }
    if (!gtoy_of(ctl)) { say("You need a Boreagotchi first"); return; }
    if (cd == 94) { if (funds < 5) { say("Not enough funds"); return; } funds -= 5; say(gt_house(ctl, 0)); } else if (cd == 95) say(gt_house(ctl, 1)); else say(gt_house(ctl, 2)); life_ver++; }

// ---- the Games menu: what the Menu's Games chip opens ----
static void gm_open(void) { mg_ui = 5; mg_ver++; }
static void gm_top(void) {
    scr = offbuf; rect(0, 0, 256, 192, K8(22, 12, 52)); panel(8, 8, 240, 176, "Games");
    text_fit(18, 40, 1, 224, TXT, "Skateboard: ride it, do tricks."); text(18, 56, 1, C15(20, 24, 28), has_board ? (on_board ? "You are riding it." : "You have a board.") : "Buy one: Buy > Skate.");
    text_fit(18, 84, 1, 224, TXT, "Boreagotchi: a pet on a keychain."); char b[40]; Toy *t = gtoy_of(ctl); sprintf(b, t ? "%d creatures. Level %d." : "A toy costs $5000.", t ? gcount(t) : 0, t ? glevel(t) : 0); text(18, 100, 1, C15(20, 24, 28), b);
    text(18, 128, 1, TXT, "Arcade and prizes: in town."); text(18, 142, 1, TXT, "Underworld: Menu > Heist.");
}
static void gm_bottom(void) { scr = offbuf; rect(0, 0, 256, 192, K8(22, 12, 52)); rect(0, 0, 256, 16, K8(60, 30, 110)); ctext(128, 4, 1, K8(90, 230, 240), "GAMES");
    chip(14, 30, 228, 50, on_board ? "Get off the board" : has_board ? "Ride the board" : "Skateboard (none yet)", on_board); chip(14, 92, 228, 50, "Boreagotchi", 0); chip(14, 154, 228, 28, "Back", 0); }
static void gm_frame(int down, int tx, int ty) {
    int td = down & KEY_TOUCH; if (down & KEY_B) { mg_ui = 0; mg_ver++; return; }
    if (!td) return;
    if (hit(14, 30, 228, 50, tx, ty)) { mg_ui = 0; mg_ver++; if (on_board) skate_off(); else if (has_board) skate_on(); else say("Buy a skateboard (Buy > Skate)"); }
    else if (hit(14, 92, 228, 50, tx, ty)) gt_open();
    else if (hit(14, 154, 228, 28, tx, ty)) { mg_ui = 0; mg_ver++; } }

// ---- the shell: which mini-game has the screens ----
static unsigned mg_sig(void) { return (unsigned)(mg_ver * 131 + mg_ui * 7); }
static unsigned mg_bsig(void) { return mg_ui == 3 ? (unsigned)(30 + hp * 5 + hsel * 11) : mg_ui == 4 ? gt_bsig() + 77 : mg_ui == 5 ? 5 : ar_bsig(); }
static void mg_top(void) { if (mg_ui == 3) hs_top(); else if (mg_ui == 4) gt_top(); else if (mg_ui == 5) gm_top(); else ar_top(); }
static void mg_bottom(void) { if (mg_ui == 3) hs_bottom(); else if (mg_ui == 4) gt_bottom(); else if (mg_ui == 5) gm_bottom(); else ar_bottom(); }
static void mg_frame(int held, int down, int steps, int tx, int ty) { if (mg_ui == 3) hs_frame(held, down, steps, tx, ty); else if (mg_ui == 4) gt_frame(held, down, steps, tx, ty); else if (mg_ui == 5) gm_frame(down, tx, ty); else ar_frame(held, down, steps, tx, ty); }

// ==== the mini-games' saves: blocks after the farm's, each { magic, size, data }; a save from before has none (all at their start) ====
#define MG_HT 0x5448   // 'HT': the underworld's heat, kept boats and aircraft, logic
#define MG_AR 0x5241   // 'AR': the arcade's tickets, the fight record
#define MG_AS 0x5341   // 'SA': the arcade's high-score tables, first-play bits, games played
#define MG_SK 0x4B53   // 'SK': the board and the skate scores
static void mg_reset(void) { has_board = on_board = 0; board_lift = 0; skate_score = skate_best = 0; arcade_tix = fight_wins = fight_champ = fight_best = 0; memset(ar_hs, 0, sizeof ar_hs); ar_pm = ar_plays = 0; mg_ui = 0; hs_heat = hs_day = hs_stolen = hs_logic = 0; gt_reset(); }
static int mg_pack(u8 *o) {
    int n = 0; u16 th[2]; s32 v[3];
    th[0] = MG_SK; th[1] = 12; v[0] = has_board; v[1] = (s32)skate_score; v[2] = (s32)skate_best; memcpy(o + n, th, 4); memcpy(o + n + 4, v, 12); n += 16;
    th[0] = MG_AR; th[1] = 16; { s32 w[4] = { arcade_tix, fight_wins, fight_champ, fight_best }; memcpy(o + n, th, 4); memcpy(o + n + 4, w, 16); n += 20; }
    th[0] = MG_AS; th[1] = sizeof ar_hs + 4; memcpy(o + n, th, 4); memcpy(o + n + 4, ar_hs, sizeof ar_hs); memcpy(o + n + 4 + sizeof ar_hs, &ar_pm, 2); memcpy(o + n + 6 + sizeof ar_hs, &ar_plays, 2); n += 8 + (int)sizeof ar_hs;
    th[0] = MG_HT; th[1] = 16; { s32 w[4] = { hs_heat, hs_day, hs_stolen, hs_logic }; memcpy(o + n, th, 4); memcpy(o + n + 4, w, 16); n += 20; }
    n += gt_pack(o + n);
    return n; }
static int mg_unpack(const u8 *p, int left) {
    mg_reset(); int n = 0;
    while (n + 4 <= left) { u16 th[2]; memcpy(th, p + n, 4); if (n + 4 + th[1] > left) break;
        if (th[0] == MG_SK && th[1] == 12) { s32 v[3]; memcpy(v, p + n + 4, 12); has_board = v[0] & 1; skate_score = v[1] < 0 ? 0 : v[1]; skate_best = v[2] < 0 ? 0 : v[2]; }
        else if (th[0] == MG_AR && th[1] == 16) { s32 w[4]; memcpy(w, p + n + 4, 16); arcade_tix = CLAMP(w[0], 0, 9999); fight_wins = MAX(0, w[1]); fight_champ = w[2] & 1; fight_best = MAX(0, w[3]); }
        else if (th[0] == MG_AS && th[1] == sizeof ar_hs + 4) { memcpy(ar_hs, p + n + 4, sizeof ar_hs); memcpy(&ar_pm, p + n + 4 + sizeof ar_hs, 2); memcpy(&ar_plays, p + n + 6 + sizeof ar_hs, 2); }
        else if (th[0] == MG_HT && th[1] == 16) { s32 w[4]; memcpy(w, p + n + 4, 16); hs_heat = CLAMP(w[0], 0, 100); hs_day = w[1] & 255; hs_stolen = w[2] & 0xFFFF; hs_logic = MAX(0, w[3]); }
        else if (th[0] == MG_GT) gt_unpack(p + n + 4, th[1]);
        else break;
        n += 4 + th[1]; }
    return n; }
#ifdef DSTEST
static void mg_test(int sc) {
    if (sc == 42) { ag_testlot = 1; memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; gen_public(LOT_ARCADE, 0x5A3C91u); occ_ok = 0; funds += 5000; for (int q = 0; q < nsim; q++) { sims[q].x = sims[q].tx = sims[q].wx = (5 + q) * 16; sims[q].y = sims[q].ty = sims[q].wy = 7 * 16; sims[q].act = -1; } }
    if (sc == 49) { funds += 8000; gt_reset(); gbuy(ctl); Toy *t = gtoy_of(ctl); static const u8 stg[8] = { GS_EGG, GS_BABY, GS_CHILD, GS_TEEN, GS_ADULT, GS_ADULT, GS_ADULT, GS_ADULT };
        for (int i = 0; i < 7; i++) { Blob *b = &t->b[i]; gfresh(b, i * 3 + 1); b->stage = stg[i]; b->form = (u8)(i - 4 < 0 ? i & 1 : i - 4); b->age = (u16)(i * 11); b->bond = (u8)(i * 14); b->hun = (u8)(4 - (i == 2)); b->hap = (u8)(i == 3 ? 0 : 3); if (i == 1) b->poop = 2; if (i == 5) b->fl |= GF_SICK; gdex_see(b->sp, b->form); }
        for (int f = 0; f < 4; f++) { gdex_see(0, f); gdex_see(9, f); } t->xp = 120; gt_open(); }
    if (sc == 56 || sc == 57) { funds += 8000; gt_reset(); gbuy(ctl); Toy *t = gtoy_of(ctl); Blob *bs = t->b; static const u8 stg[8] = { GS_ADULT, GS_ADULT, GS_BABY, GS_CHILD, GS_TEEN, GS_ELDER, GS_EGG, GS_ADULT };
        for (int i = 0; i < 8; i++) { Blob *b = &bs[i]; gfresh(b, i * 2 + 1); b->stage = stg[i]; b->age = (u16)(i * 30); b->bond = 80; b->pers = (u8)i; b->col = (u8)i; b->mut = i == 2; gnewid(t, b, 0, 0); }
        bs[0].mate = bs[1].id; bs[1].mate = bs[0].id; for (int i = 2; i < 5; i++) { bs[i].mom = bs[0].id; bs[i].dad = bs[1].id; gln_set(t, &bs[i], 0); } bs[6].mom = bs[1].id; bs[6].dad = bs[0].id; gln_set(t, &bs[6], 0);
        { Blob gm; memset(&gm, 0, sizeof gm); gm.on = 1; gm.sp = 3; gm.nm = 7; gm.col = 3; gnewid(t, &gm, 0, 0); gln_set(t, &gm, 1); bs[0].mom = gm.id; gln_set(t, &bs[0], 0); t->mem = 1; t->memnm = 7; t->memsp = 3; }
        for (int f = 0; f < 4; f++) gdex_see(0, f);
        gt_open_a(1); if (sc == 57) { gtree = 0; for (int j = 0; j < GLN; j++) if (t->ln[j].id == bs[2].id) gtree = j; gview = GV_TREE; } else gbi = 2; }
    if (sc == 48) { funds += 5000; hs_open(); }
    if (sc == 51) { gt_testshop = 1; memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; gen_public(LOT_SHOP, 0x5A3C91u); occ_ok = 0; funds += 5000; { int dk = has_ch(0xBE); if (dk >= 0) { popup = 1; popup_i = dk; popup_n = obj_acts(dk, popup_lab, popup_code); } else say("No dock"); } for (int q = 0; q < nsim; q++) { sims[q].x = sims[q].tx = sims[q].wx = (7 + q) * 16; sims[q].y = sims[q].ty = sims[q].wy = 7 * 16; sims[q].act = -1; } }
    if (sc >= 52 && sc <= 55) { funds += 8000; gt_reset(); for (int q = 0; q < nsim && q < 3; q++) { gbuy(q); Toy *t = gtoy_of(q); for (int i = 0; i < (q ? 3 : 5); i++) { gegg(t, i); Blob *b = &t->b[i]; b->stage = (u8)(1 + (i + q) % 4); b->hun = (u8)(1 + i % 4); b->hap = (u8)(i == 2 ? 0 : 3); } t->xp = (u16)(q * 80); }
        for (int f = 0; f < 4; f++) gdex_see(0, f);
        if (sc == 52) gt_open_a(0); else if (sc == 53) { gt_open_a(3); gguide = 5; } else if (sc == 54) { gt_open_a(3); gguide = 3; } else { gt_open_a(1); o_gtime = 1; } }
    if (sc >= 43 && sc <= 47) { arcade_tix = 37; funds += 5000; ag_open(sc == 47 ? AG_PRIZES : sc - 42); }
    if (sc >= 80 && sc <= 88) { arcade_tix = 37; funds += 5000; ag_open(sc - 75); }   // (test builds) 60..71: games 5..13+   // (test builds) 40: this lot becomes the skate plaza, your Bore on the board at its west side
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
