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
    panel(6, 6, 244, 34, "Skate"); sprintf(b, "Score %ld", skate_score); text_box(9, 24, 130, 14, FS_HEAD, C15(31, 31, 20), AL_L, b); sprintf(b, "Best combo %ld", skate_best); text_box(110, 24, 136, 14, FS_BODY, C15(24, 28, 31), AL_R, b);
    panel(6, 44, 244, 62, 0);
    if (k.ntr) { int n = MIN(k.ntr, 3); b[0] = 0; for (int i = k.ntr - n; i < k.ntr; i++) { char t[24]; tk_name(i, t); if (b[0]) strcat(b, " + "); strcat(b, t); }
        text_fit(128 - MIN(tw(1, b), 232) / 2, 52, 1, 232, TXT, b); sprintf(b, "%ld X %d", k.pool, MAX(k.mult, 1)); text(128 - tw(2, b) / 2, 66, 2, C15(16, 26, 31), b); }
    else text(128 - tw(1, "Push off and find a ramp") / 2, 58, 1, C15(20, 24, 28), "Push off and find a ramp");
    if (k.st == KS_GRIND || k.st == KS_MANUAL) { int x = 78, y = 86, w = 100, p = CLAMP(k.bal, -4096, 4096) * (w / 2) / 4096;
        rect(x - 2, y - 2, w + 4, 10, C15(31, 31, 31)); rect(x, y, w, 6, C15(8, 10, 16)); rect(x + w / 2 - 12, y, 24, 6, C15(15, 26, 14)); rect(x, y, 14, 6, C15(29, 11, 9)); rect(x + w - 14, y, 14, 6, C15(29, 11, 9)); rect(x + w / 2 + p - 2, y - 2, 4, 10, C15(4, 5, 8)); }
    if (k.popt) { panel(40, 110, 176, 36, 0); text(128 - tw(2, k.popa) / 2, 116, 2, k.popok ? C15(14, 30, 14) : C15(31, 10, 8), k.popa); if (k.popb[0]) text(128 - tw(1, k.popb) / 2, 132, 1, C15(24, 28, 31), k.popb); }
    panel(6, 150, 244, 38, 0);
    if (k.st == KS_AIR) { text_box(9, 151, 238, 18, FS_BODY, TXT, AL_L, "Y flip   B grab (hold)   L R spin"); text_box(9, 169, 238, 18, FS_BODY, TXT, AL_L, "X grind   D-pad picks the trick"); }
    else { text_box(9, 151, 238, 18, FS_BODY, TXT, AL_L, "A ollie (hold: higher)   Y manual"); text_box(9, 169, 238, 18, FS_BODY, TXT, AL_L, "X grind   B off   D-pad rides"); }
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
static void dsag_fixb(const char *s, char *b) {   // (the PS1's button names become the DS's: X is A, O is B, TRI is X, SQ is Y)
    int n = 0;
    for (const char *p = s; *p && n < 44;) { if ((p == s || p[-1] == ' ') && (p[1] == 0 || p[1] == ' ' || p[1] == '.' || p[1] == ',')) { if (*p == 'X') { b[n++] = 'A'; p++; continue; } if (*p == 'O') { b[n++] = 'B'; p++; continue; } }
        if ((p == s || p[-1] == ' ') && !strncmp(p, "TRI", 3) && (p[3] == 0 || p[3] == ' ')) { b[n++] = 'X'; p += 3; continue; }
        if ((p == s || p[-1] == ' ') && !strncmp(p, "SQ", 2) && (p[2] == 0 || p[2] == ' ')) { b[n++] = 'Y'; p += 2; continue; }
        b[n++] = *p++; }
    b[n] = 0; }
static void dsag_put(int x, int y, int sz, int room, u32 c, const char *b) {   // (D65) BODY (HEAD for sz 2) if it fits the room, else the smaller font; the PS1's y is the top of the capitals
    if (y < -8 || y > 160) return; int f = fit_font(sz >= 2 ? FS_HEAD : FS_BODY, room, b); if (!f) f = FS_SMALL;
    text(8 + dsag_ox + x, 16 + dsag_oy + y + (th(f) - 1) / 2 - 1, f, c8(c), b); }
void dsag_text(int x, int y, int sz, const char *s, u32 c) { char b[48]; dsag_fixb(s, b); dsag_put(x, y, sz, 240 - (x < 0 ? 0 : x), c, b); }
void dsag_textc(int cx, int y, int sz, const char *s, u32 c) {   // centred on cx (measured) and kept inside the field
    char b[48]; dsag_fixb(s, b); int f = fit_font(sz >= 2 ? FS_HEAD : FS_BODY, 238, b); if (!f) f = FS_SMALL; int w = tw(f, b), x = cx - w / 2; if (x < 1) x = 1; if (x + w > 239) x = 239 - w;
    if (y < -8 || y > 160) return; text(8 + dsag_ox + x, 16 + dsag_oy + y + (th(f) - 1) / 2 - 1, f, c8(c), b); }
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
static int arcade_game_at(int x, int y) { return (x * 3 + y * 2) % AG_NGAME; }   // (all fourteen, BORTUA FIGHTER the last cabinet of the rotation)
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
static void fight_open(int cr);
static void ag_open(int g) { if (g == AG_FIGHTER) { ag = g; mg_ui = 6; mg_ver++; fight_open(1); return; } ag = g; ag_phase = 0; ag_acc = 0; ag_psel = 0; ag_msg = ""; ag_rt = 0; dsag_tx = dsag_tap = -1; ag_held_id = 0; mg_ui = g == AG_PRIZES ? 2 : 1; mg_ver++; }
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
static void ctext(int cx, int y, int s, u16 c, const char *t) { int f = fit_font(s, 244, t); if (!f) f = FS_SMALL; text(cx - tw(f, t) / 2, y, f, c, t); }   // (D65) centred by measure, the font stepped down if it is too wide
static void ar_scores(int g, int y0) {
    char b[40], n[4]; ctext(128, y0, 1, K8(255, 190, 40), "HIGH SCORES");
    for (int i = 0; i < 3; i++) { ini_str(ar_hs[g][i], n); if (SC_(ar_hs[g][i])) snprintf(b, sizeof b, "%d  %s  %d", i + 1, n, SC_(ar_hs[g][i])); else snprintf(b, sizeof b, "%d  ---  0", i + 1); ctext(128, y0 + 18 + i * 16, 1, i == 0 ? K8(255, 190, 40) : TXT, b); }
}
static void ar_top(void) {
    scr = offbuf; char b[40]; rect(0, 0, 256, 192, K8(16, 10, 34)); dsag_ox = dsag_oy = 0;
    if (mg_ui == 2) {
        panel(8, 8, 240, 160, "The Prize Counter");
        for (int i = 0; i < NPRZ; i++) { int y = 26 + i * 13, s = i == ag_psel; rect(14, y, 228, 12, s ? K8(226, 242, 254) : PANB); rect(15, y + 1, 226, 10, s ? C15(24, 28, 31) : PAN);
            text_box(16, y + 1, 150, 10, FS_BODY, s ? C15(2, 7, 20) : TXT, AL_L, prz[i].nm); sprintf(b, "%d tix", prz[i].cost); text_box(150, y + 1, 90, 10, FS_BODY, arcade_tix >= prz[i].cost ? (s ? C15(2, 14, 4) : C15(14, 30, 14)) : C15(31, 12, 10), AL_R, b); }
        sprintf(b, "You have %d tickets", arcade_tix); ctext(128, 158, 1, C15(24, 28, 31), b); if (ag_msg[0]) ctext(128, 176, 1, C15(31, 14, 12), ag_msg);
        return; }
    rect(6, 14, 244, 164, K8(agCol[ag][0] / 3, agCol[ag][1] / 3, agCol[ag][2] / 3 + 20));
    rect(8, 1, 240, 12, K8(60, 30, 110)); text_box(8, 1, 240, 12, FS_HEAD, K8(agCol[ag][0], agCol[ag][1], agCol[ag][2]), AL_C, agName[ag]);
    dsag_rect(0, 0, 240, 160, 0x120404u, 0);
    if (ag_phase == 0) {
        int pg = (ag_rt / 100) % 2;
        if (pg == 0) { ctext(128, 40, 5, K8(agCol[ag][0], agCol[ag][1], agCol[ag][2]), agName[ag]); ctext(128, 80, 1, TXT, agHow1[ag]); ctext(128, 96, 1, TXT, agHow2[ag]);
            sprintf(b, "PLAY $%d", ag_price(ag)); ctext(128, 128, 1, K8(255, 190, 40), b); ctext(128, 146, 1, K8(170, 210, 250), "PRESS A TO START"); }   // (D65: it blinked on and off twice a second; it stays put now)
        else { ctext(128, 40, 5, TXT, agName[ag]); ar_scores(ag, 72); }
    } else if (ag_phase == 3) {
        ctext(128, 30, 5, K8(255, 190, 40), "NEW RECORD!"); sprintf(b, "SCORE %d", ag_pts); ctext(128, 56, 1, TXT, b);
        for (int i = 0; i < 3; i++) { char c[2] = { ag_ini[i], 0 }; int x = 84 + i * 28; rect(8 + x, 16 + 70, 22, 30, i == ag_ic ? K8(250, 200, 60) : K8(60, 70, 110)); rect(8 + x + 1, 16 + 71, 20, 28, K8(10, 10, 30)); text_box(8 + x, 16 + 70, 22, 30, FS_TITLE, TXT, AL_C, c); }
        ctext(128, 128, 1, K8(170, 210, 250), "UP DOWN PICKS. A NEXT");
    } else {
        if (ag_shake > 0) { dsag_ox = (ag_shake & 1) * 3 - 1; dsag_oy = ((ag_shake >> 1) & 1) * 2; }
        ds_ag_fn[ag].draw(); dsag_fx_draw(); dsag_ox = dsag_oy = 0;
        if (ag_phase == 2) { rect(56, 68, 144, 80, K8(10, 12, 30)); rect(58, 70, 140, 76, K8(20, 22, 44)); ctext(128, 76, 5, K8(250, 210, 40), "GAME OVER");
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
    scr = offbuf; char b[32]; rect(0, 0, 256, 192, K8(24, 16, 54)); rect(0, 0, 256, 16, K8(60, 30, 110)); text_box(0, 0, 256, 16, FS_HEAD, K8(90, 230, 240), AL_C, mg_ui == 2 ? "THE PRIZE COUNTER" : agName[ag]);
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
#include "ds_fight.c"   // (D59) BORTUA FIGHTER
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
        text_fit(16, top, 1, 226, C15(20, 24, 28), "PICK A RIDE. ANY NIGHT."); sprintf(b, "HEAT %d", hs_heat); text(242 - tw(1, b), top, 1, hs_heat > 50 ? C15(31, 12, 10) : C15(14, 30, 14), b);
        text(16, top + 18, 2, TXT, hTNM[hsel]); sprintf(b, "%s   $%d", hTWH[hsel], hTVAL[hsel]); text_fit(16, top + 40, 1, 226, C15(14, 30, 14), b);
        sprintf(b, "SECURITY %d  WANTED %d  YOUR LOGIC %d", hTDF[hsel], hTST[hsel], hlg()); text_fit(16, top + 56, 1, 226, C15(24, 28, 31), b);
        text_fit(16, top + 74, 1, 224, C15(20, 24, 28), hsel >= 5 ? "KEPT: HOT. SKY AND SEA CAN TAKE IT OUT." : "A CAR OR BIKE: THE CHOP SHOP, FOR CASH.");
        if (hs_stolen & (1 << hsel)) text_fit(16, top + 90, 1, 226, C15(31, 25, 6), "YOU KEEP ONE OF THESE ALREADY.");
        text_fit(16, top + 108, 1, 226, C15(31, 12, 10), hline); return; }
    if (hp == H_GATE || hp == H_CHASE) {
        if (hp == H_GATE) { text_fit(16, top, 1, 226, TXT, hTNM[htgt]); sprintf(b, "GATE %d OF 3: %s", hgate + 1, hGNM[hgate]); text_fit(16, top + 14, 1, 226, C15(24, 28, 31), b); text_fit(16, top + 28, 1, 226, C15(20, 24, 28), hGHINT[hgate]); }
        else { text_fit(16, top, 1, 226, C15(31, 12, 10), "THE GETAWAY"); sprintf(b, "CORNER %d OF %d", hround + 1, hstars + 1); text_fit(16, top + 14, 1, 226, C15(24, 28, 31), b); text_fit(16, top + 28, 1, 226, C15(20, 24, 28), hline2); }
        rect(16, top + 50, 224, 16, K8(200, 206, 214)); rect(16 + hzone * 224 / 100, top + 50, hzw * 224 / 100, 16, K8(255, 235, 120)); rect(14 + hmark * 224 / 100, top + 46, 5, 24, K8(60, 70, 200));
        for (int i = 0; i < 5; i++) { rect(16 + i * 16, top + 84, 12, 12, i < hstars ? K8(236, 170, 30) : K8(110, 110, 130)); }
        text_fit(16, top + 108, 1, 226, TXT, hline); return; }
    if (hp == H_CHOICE) { text_fit(16, top, 1, 226, TXT, "YOU'RE IN. NOW WHAT?"); text_fit(16, top + 24, 1, 226, C15(24, 28, 31), htgt >= 5 ? "KEEP IT. IT IS HOT." : "SELL IT TO THE CHOP SHOP"); text_fit(16, top + 42, 1, 226, C15(24, 28, 31), "FENCE IT FOR CASH"); return; }
    text_fit(16, top + 6, 1, 224, TXT, hline); text_fit(16, top + 28, 1, 224, C15(20, 24, 28), hline2); sprintf(b, "FUNDS $%d   HEAT %d", funds, hs_heat); text_fit(16, top + 60, 1, 226, C15(14, 30, 14), b);
}
static void hs_bottom(void) {
    scr = offbuf; rect(0, 0, 256, 192, K8(14, 14, 34)); rect(0, 0, 256, 16, K8(30, 20, 50)); text_box(0, 0, 256, 16, FS_HEAD, K8(200, 190, 250), AL_C, "UNDERWORLD");
    if (hp == H_LIST) {
        for (int i = 0; i < HT_N; i++) { int y = 18 + i * 14, s = i == hsel; rect(8, y, 240, 13, s ? K8(226, 242, 254) : PANB); rect(9, y + 1, 238, 11, s ? C15(24, 28, 31) : PAN); char b[40]; sprintf(b, "%s", hTNM[i]); text_box(10, y + 1, 150, 11, FS_BODY, s ? C15(2, 7, 20) : TXT, AL_L, b); sprintf(b, "$%d", hTVAL[i]); text_box(150, y + 1, 94, 11, FS_NUM, s ? C15(2, 14, 4) : C15(14, 30, 14), AL_R, b); }
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
typedef struct { u8 on, stage, form, sp, nm, hun, hap, disc, wt, poop, fl, miss, needT, bond, snk, meal, gen; u16 age; u8 pers, col, mut, id, mom, dad, mate, acc, taste; } Blob;   // (D54: family) pers, colour, rare mutation, its own id, its parents' ids, its mate's id
typedef struct { u8 id, sp, nm, mom, dad, col; } Ln;   // one line of a family tree: kept after a pet has passed on (nm bit 7)
#define GLN 24
typedef struct { u8 nrg, hyg, wish, sk[3], bd, lone, bff, fnd, bday; } Xt;   // (D64) the v48 needs and depth: energy and cleanness 0..100; a wish; skills SPEED BRAINS CHARM; birthdays; hours alone; best friend (slot + 1) and how close
typedef struct { u8 uid, own, sel, pad; u16 xp, hatched, wins, cared, plays; Blob b[GB_N]; Ln ln[GLN]; u8 nid, lh, mem, memnm, memsp, room, deco, accs; Xt x[GB_N]; u8 tal[GLN]; u8 gp[6], streak, best, gflag, ghist; u16 gday, gwon; } Toy;   // (D55) plays, the home's walls and things, the hats owned; (D64) gp today's care, the streak, the last seven days
typedef struct { u16 day; u8 kind, nm; } Dy;   // (D55) a line of the pet diary: kind is the verb (bits 0-4) and, for a game or a skill, which one (bits 5-7)
#define GD_N 32
#define NACH 20
#define NACC 8
#define NFOOD 12
static int gmood_of(const Blob *b, const Xt *x);
enum { GD_HATCH = 1, GD_GROW, GD_ADULT, GD_PASS, GD_GIFT, GD_BDAY, GD_TANTRUM, GD_ILL, GD_EGG, GD_PLAY, GD_MUT, GD_CARED, GD_BOND, GD_VISIT, GD_AWARD, GD_DAY, GD_WISH, GD_TRAIN, GD_BEST, GD_FRIEND, GD_MISS, GD_EVENT, GD_STREAK };
enum { MD_PLAIN, MD_HAPPY, MD_DANCE, MD_SLEEP, MD_SICK, MD_HUNGRY, MD_BORED, MD_ANGRY, MD_TIRED, MD_DIRTY };
static Toy gtoy[GT_MAX]; static Dy gdy[GD_N]; static u8 gdn, gbest[8]; static u32 gdex[2], gach; static u32 grng = 0x1234567u; static int gmd, gdanceT, gactT, gactK;
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
// (D62) the PS1's title screen, the hatching scene, the day summary and the first-day tips
static int gafter, gtitleT, ghT, ghpend, gsum_c, gsum_p, gsum_h, gsum_pend, gc0, gp0, gh0;
static void gt_reset(void) { ghpend = gsum_pend = gc0 = gp0 = gh0 = 0; memset(gtoy, 0, sizeof gtoy); gdex[0] = gdex[1] = 0; gach = 0; gdn = 0; memset(gdy, 0, sizeof gdy); memset(gbest, 0, sizeof gbest); }
static void gdiary(int kind, int nm) { Dy *d = &gdy[gdn++ % GD_N]; d->day = (u16)(minutes / 60); d->kind = (u8)kind; d->nm = (u8)nm; }
static int gsim_of(const Toy *t) { return t->uid < nsim ? t->uid : -1; }
static Toy *gtoy_of(int si) { if (si < 0 || si >= nsim) return 0; for (int i = 0; i < GT_MAX; i++) if (gtoy[i].own && gtoy[i].uid == si) return &gtoy[i]; return 0; }
static void gt_day(void) {   // midnight: what the owner did today (cares, games, hatchings), shown the next time the toy is opened
    Toy *t = ctl >= 0 && ctl < nsim ? gtoy_of(ctl) : 0; if (!t) return;
    gsum_c = (int)t->cared - gc0; gsum_p = (int)t->plays + (int)t->wins - gp0; gsum_h = (int)t->hatched - gh0; if (gsum_c < 0) gsum_c = 0; if (gsum_p < 0) gsum_p = 0; if (gsum_h < 0) gsum_h = 0;
    gc0 = t->cared; gp0 = t->plays + t->wins; gh0 = t->hatched; gsum_pend = 1; }
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
static void gfresh(Blob *b, int sp) { memset(b, 0, sizeof *b); b->on = 1; b->sp = (u8)(sp % GSP_N); b->nm = (u8)(gr() & 31); b->hun = 4; b->hap = 4; b->wt = 5; b->pers = (u8)(gr() & 7); b->col = (u8)(gr() & 7); b->mut = (gr() & 31) == 0; { int l = gr() % 12, d = gr() % 11; b->taste = (u8)(l << 4 | (d >= l ? d + 1 : d)); } }
static void gln_set(Toy *t, const Blob *b, int gone) {   // the family tree remembers it: kept up to date, and kept after it has passed on
    int k = -1; for (int i = 0; i < GLN; i++) if (t->ln[i].id == b->id) { k = i; break; } if (k < 0) { for (int i = 0; i < GLN && k < 0; i++) if (!t->ln[i].id) k = i;
        for (int i = 0; i < GLN && k < 0; i++) if (t->ln[i].nm & 128) { int par = 0; for (int j = 0; j < GLN; j++) if (t->ln[j].id && (t->ln[j].mom == t->ln[i].id || t->ln[j].dad == t->ln[i].id)) par = 1; if (!par) k = i; }   // (D55) the oldest memorial nobody descends from goes first
        if (k < 0) k = t->lh++ % GLN; }
    t->ln[k].id = b->id; t->ln[k].sp = b->sp; t->ln[k].nm = (u8)((b->nm & 31) | (gone ? 128 : 0)); t->ln[k].mom = b->mom; t->ln[k].dad = b->dad; t->ln[k].col = b->col; }
static void gnewid(Toy *t, Blob *b, int mom, int dad) { if (!++t->nid) ++t->nid; b->id = t->nid; b->mom = (u8)mom; b->dad = (u8)dad; gln_set(t, b, 0); }
static Blob *gby_id(Toy *t, int id) { if (!id) return 0; for (int i = 0; i < GB_N; i++) if (t->b[i].on && t->b[i].id == id) return &t->b[i]; return 0; }
static const Ln *gln_of(const Toy *t, int id) { if (!id) return 0; for (int i = 0; i < GLN; i++) if (t->ln[i].id == id) return &t->ln[i]; return 0; }
static int gpar_of(const Toy *t, const Ln *a, int c) { for (int i = 0; i < 2; i++) { int p = i ? a->dad : a->mom; if (!p) continue; if (p == c) return 1; const Ln *l = gln_of(t, p); if (l && (l->mom == c || l->dad == c)) return 1; } return 0; }
static int grel(const Toy *t, int a, int c) {   // (D55) the same pet, a parent or grandparent, a brother or sister: relatives never pair up
    const Ln *x = gln_of(t, a), *y = gln_of(t, c); if (a == c) return 1; if (!x || !y) return 0; if (gpar_of(t, x, c) || gpar_of(t, y, a)) return 1;
    return (x->mom && (x->mom == y->mom || x->mom == y->dad)) || (x->dad && (x->dad == y->dad || x->dad == y->mom)); }
static int gimport(Toy *h, const Toy *src, const Blob *p) {   // a parent from another family gets a line in this family's tree
    if (src == h) return p->id; Blob tmp = *p; if (!++h->nid) ++h->nid; tmp.id = h->nid; tmp.mom = tmp.dad = 0; gln_set(h, &tmp, 0); return tmp.id; }
static int gbreed(Toy *t, const Blob *x, const Blob *y, int mid, int did) {   // two grown-ups who love each other leave an egg: kind, colour, nature and a rare shine come from them
    for (int s = 0; s < GB_N; s++) if (!t->b[s].on) { Blob *e = &t->b[s]; gfresh(e, (gr() & 15) == 0 ? gr() : (gr() & 1 ? x->sp : y->sp));
        e->gen = (u8)MIN(99, MAX(x->gen, y->gen) + 1); e->pers = (gr() & 7) == 0 ? (u8)(gr() & 7) : (gr() & 1 ? x->pers : y->pers); e->col = (gr() & 7) == 0 ? (u8)(gr() & 7) : (gr() & 1 ? x->col : y->col); e->mut = (gr() & 15) == 0 || (x->mut && y->mut && (gr() & 1)); gnewid(t, e, mid, did); gdiary(GD_EGG, e->nm); return 1; }
    return 0; }
static int gbuy(int si) {
    if (si < 0 || si >= nsim || gtoy_of(si)) return 0;
    int k = -1; for (int i = 0; i < GT_MAX; i++) if (!gtoy[i].own || gsim_of(&gtoy[i]) < 0) { k = i; break; } if (k < 0) return 0;
    Toy *t = &gtoy[k]; memset(t, 0, sizeof *t); t->own = 1; t->uid = (u8)si; t->room = (u8)(gr() % 6); gfresh(&t->b[0], gr()); gnewid(t, &t->b[0], 0, 0); return 1; }
static int gegg(Toy *t, int slot) { if (slot < 0 || slot >= GB_N || t->b[slot].on) return 0; gfresh(&t->b[slot], gr()); gnewid(t, &t->b[slot], 0, 0); return 1; }
// ==== (D64) Boreagotchi v48 on the DS (the PS1's gotchi.c, gotchi_act.c): energy and cleanness, moods, wishes, goals, skills, surprises ====
enum { GP_CLEVER, GP_LAZY, GP_PLAYFUL, GP_GENTLE, GP_GREEDY, GP_BRAVE, GP_SHY, GP_GRUMPY };   // the DS's own order of natures
static void gx_award(int bit);
static void gup(u8 *v, int max, int n) { int x = *v + n; *v = (u8)(x > max ? max : x < 0 ? 0 : x); }
static void gxinit(Toy *t, int s) { Xt *x = &t->x[s]; memset(x, 0, sizeof *x); x->nrg = 90; x->hyg = 100; x->bday = (u8)((minutes / 60) % 168); }
static int gslot(const Toy *t, const Blob *b) { return (int)(b - t->b); }
static void gtouch(Toy *t, const Blob *b) { t->x[gslot(t, b)].lone = 0; }
static int gnight(void) { int c = minutes % 1440; return c >= 20 * 60 || c < 6 * 60; }
static int ghas(int i) { return (gach >> (i < 16 ? i : i + 5)) & 1; }   // awards 0-15 are bits 0-15, the newer four are 21-24 (bits 16-20 are the first-day script)
static int gfxk; static char gpmsg[56]; static int gpmsgOn, ggrade;   // an effect for the screen (1 wish 2 goal 3 all goals 4 new best 5 surprise); a line waiting to be shown; the last grade 0 D .. 4 S
static const char *gpending(void) { if (!gpmsgOn) return 0; gpmsgOn = 0; return gpmsg; }
static int gfxs(void) { int f = gfxk; gfxk = 0; return f; }
static int gcash(Toy *t, int n) { if (gsim_of(t) == ctl) { funds += n; return 1; } return 0; }
// ---- today's goals: three of six kinds, the same for every pet of the toy; all three keep the streak going ----
static const u8 gTarget[6] = { 3, 2, 1, 2, 3, 1 };
static const char *const gNm[6] = { "FEED 3 MEALS", "PLAY 2 GAMES", "GIVE A BATH", "TRAIN TWICE", "CUDDLE 3 TIMES", "GRANT A WISH" };
static int ggoal_type(const Toy *t, int i) { return ((int)t->gday % 6 + i * 2) % 6; }
static int ggoal_have(const Toy *t, int i) { return MIN(t->gp[ggoal_type(t, i)], gTarget[ggoal_type(t, i)]); }
static int ggoals_done(const Toy *t) { int n = 0; for (int i = 0; i < 3; i++) n += ggoal_have(t, i) >= gTarget[ggoal_type(t, i)]; return n; }
static void ggoals_roll(Toy *t) {   // a new day: the counts start again; a missed day ends the streak
    int dc = minutes / 1440; if (t->gday == (u16)dc && (t->gflag & 128)) return; int dd = dc - (int)t->gday; if (dd < 0) dd = 0;
    if (t->gflag & 128) { if (dd > 7) t->ghist = 0; else for (int k = 0; k < dd; k++) t->ghist = (u8)((t->ghist << 1 | (k == 0 && t->gwon == t->gday + 1)) & 127); if (!(dd == 1 && t->gwon == t->gday + 1)) t->streak = 0; }
    if (dd || !(t->gflag & 128)) { memset(t->gp, 0, 6); t->gflag = 128; }
    t->gday = (u16)dc; }
static void gpadd(Toy *t, int kind) {
    ggoals_roll(t); if (t->gp[kind] < 250) t->gp[kind]++;
    for (int i = 0; i < 3; i++) if (ggoal_type(t, i) == kind && t->gp[kind] == gTarget[kind] && !(t->gflag & (2 << i))) { t->gflag |= (u8)(2 << i); gcash(t, 10); gxp(t, 3); gfxk = 2; strcpy(gpmsg, "GOAL DONE! +$10"); gpmsgOn = 1; }
    if (!(t->gflag & 1) && ggoals_done(t) == 3) {
        t->gflag |= 1; t->gwon = (u16)(minutes / 1440 + 1); if (t->streak < 250) t->streak++; if (t->streak > t->best) t->best = t->streak;
        int g = 20 + 10 * MIN(t->streak, 10); gcash(t, g); gxp(t, 5); gfxk = 3; snprintf(gpmsg, sizeof gpmsg, "ALL GOALS! STREAK %d. +$%d", t->streak, g); gpmsgOn = 1;
        if (t->streak >= 3) gx_award(16); if (t->streak >= 7) gx_award(17); for (int e = 0; e < GB_N; e++) if (t->b[e].on && t->streak % 3 == 0) { gdiary(GD_STREAK, t->b[e].nm); break; } } }
// ---- wishes: each pet wants one thing at a time, by its nature; giving it is worth far more than a plain care ----
static const char *const gwNm[7] = { "", "A MEAL", "A GAME", "A BATH", "TRAINING", "A CUDDLE", "A NAP" };
static int gwish_hit(Toy *t, int s, int kind) {
    Xt *x = &t->x[s]; Blob *b = &t->b[s]; if ((x->wish & 7) != kind) return 0;
    x->wish = 0; gup(&b->bond, 100, 3); gup(&b->hap, 4, 1); gxp(t, 3); gpadd(t, 5); gdiary(GD_WISH, b->nm); gfxk = 1; gfun(gsim_of(t), 6); return 1; }
// ---- moodlets: what is going on inside, in a word or two ----
static int gmood(const Toy *t, int s) {
    const Blob *b = &t->b[s]; const Xt *x = &t->x[s];
    int m = (b->hun * 625 + b->hap * 750 + x->nrg * 20 + x->hyg * 15 + b->bond * 10) / 100; if (b->fl & GF_SICK) m -= 20; if (x->lone >= 12) m -= 8;
    return m < 0 ? 0 : m > 100 ? 100 : m; }
static const char *gmood_word(int m) { return m >= 80 ? "GREAT" : m >= 60 ? "GOOD" : m >= 40 ? "OKAY" : m >= 20 ? "CRANKY" : "MISERABLE"; }
static int gmoodlets(const Toy *t, int s, const char **o, u8 *good) {   // up to three, the bad ones first
    const Blob *b = &t->b[s]; const Xt *x = &t->x[s]; int n = 0, sl = (b->fl & GF_SLEEP) != 0;
#define ML(txt, g) do { if (n < 3) { o[n] = txt; good[n++] = (u8)(g); } } while (0)
    if (b->stage == GS_EGG) return 0;
    if (b->fl & GF_SICK) ML("SICK", 0); if (b->hun == 0) ML("STARVING", 0); else if (b->hun == 1) ML("HUNGRY", 0);
    if (!sl && x->nrg < 25) ML("TIRED", 0); if (x->hyg < 30) ML("SMELLY", 0); if (x->lone >= (b->pers == GP_PLAYFUL ? 6 : b->pers == GP_SHY ? 20 : 12)) ML("LONELY", 0); if (b->hap <= 1) ML("BORED", 0); if (b->wt >= 25) ML("STUFFED", 0);
    if (b->bond >= 70) ML("LOVED", 1); if (x->nrg >= 80 && !sl) ML("RESTED", 1); if (x->hyg >= 85) ML("SQUEAKY CLEAN", 1); if (b->hap >= 4) ML("JOLLY", 1); if (b->hun >= 4) ML("FULL", 1); if (b->disc >= 70) ML("PROUD", 1); if (x->bff && x->fnd >= 10) ML("BEST FRIEND", 1);
#undef ML
    return n; }
static const char *const gskNm[3] = { "SPEED", "BRAINS", "CHARM" };
static const char *const gtalNm[4] = { "NO TALENT", "SPEEDY", "SMARTY", "CHARMER" };
static const char *const gpersNote[8] = { "LEARNS FAST. EASY TO TRAIN", "SLEEPS IN. NEVER BORED", "BORED FAST. LOVES GAMES", "RARELY SICK. BONDS SLOWLY", "ALWAYS HUNGRY. LOVES FOOD", "LOVES A BATTLE. FUSSES", "SLOW TO TRUST. LOYAL", "FIBS A LOT. GRUMBLES" };
static const char *const gstageNote[6] = { "A COZY EGG. HATCHES SOON", "NEEDS FOOD OFTEN. NEVER TIRES", "LOVES GAMES. TIRES QUICKLY", "MOODY AND CHEEKY", "STEADY. FAMILIES START HERE", "WISE BUT TIRES EASILY" };
static const char *const gstageNm[6] = { "EGG", "BABY", "CHILD", "TEEN", "ADULT", "ELDER" };
static const char *const gwhyNm[3] = { "WIDE CATCH, LONG JUMP", "SLOW BLOCKS, MANNERS", "WIDE BEAT, BIG PRIZES" };
// ---- games: a grade, skills, prizes, best scores; training is the same game with more to gain and more to spend ----
#define NGAME 8
static const char *const ggameNm[8] = { "WHICH WAY", "COPY ME", "CATCH", "HIGH LOW", "BEAT", "HOP", "STACK", "WHACK" };
static const u8 ggameSk[8] = { 2, 1, 0, 1, 2, 0, 1, 0 };   // the skill each game trains: 0 speed 1 brains 2 charm
static const char *const ggameHow[8] = { "GUESS WHERE IT LOOKS. LUCK", "REPEAT ITS MOVES. BRAINS", "CATCH TREATS, DODGE MESS. SPEED", "HIGHER OR LOWER? LUCK", "PRESS ON THE BEAT. CHARM", "JUMP THE ROCKS. SPEED", "DROP BLOCKS INTO A TOWER. BRAINS", "WHACK THE BUG, SPARE THE MESS. SPEED" };
static const char *gplay_check(Toy *t, int slot, int train) {   // 0 when it can play (or train); else why not
    Blob *b = &t->b[slot]; Xt *x = &t->x[slot]; int m = gmood(t, slot);
    if (b->stage == GS_EGG) return "IT IS STILL AN EGG"; if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (x->nrg < (train ? 30 : 10)) return train ? "TOO TIRED TO TRAIN. LET IT REST" : "TOO TIRED TO PLAY. LIGHTS OUT?";
    if (train && b->hun < 2) return "TOO HUNGRY TO TRAIN. FEED IT FIRST";
    if (train && b->stage < GS_CHILD) return "BABIES ARE TOO SMALL TO TRAIN";
    if (m < 20) return "TOO MISERABLE. CARE FOR IT FIRST";
    if (train && m < 40) return "NOT IN THE MOOD TO TRAIN";
    if (train && b->pers == GP_LAZY && !(gr() & 3)) return "THE LAZY ONE SAYS NO. TRY AGAIN";
    return 0; }

static const char *gresult(Toy *t, Blob *b, int game, int pct, int train) {
    int slot = gslot(t, b); Xt *x = &t->x[slot]; static char rmsg[56];
    int g = pct >= 90 ? 4 : pct >= 70 ? 3 : pct >= 50 ? 2 : pct >= 30 ? 1 : 0, sk = ggameSk[game & 7], m = gmood(t, slot), si = gsim_of(t), pr = b->pers;
    static const char *const gn[5] = { "D. TRY AGAIN", "C. NOT BAD", "B. NICE", "A! GREAT", "S! AMAZING" };
    t->plays++; ggrade = g; gtouch(t, b);
    int cost = train ? 12 : 6; if (b->wt >= 20) cost += 3; if (pr == GP_LAZY) cost += 2; x->nrg = (u8)(x->nrg > cost ? x->nrg - cost : 0); gup(&x->hyg, 100, train ? -8 : -4);
    if (train) { if (b->hun > 1) b->hun--; int gain = 1 + g * 2; if (pr == GP_CLEVER) gain++; if (pr == GP_LAZY && gain > 1) gain--; gain = gain * (50 + m) / 100 + (g > 0); if (gain < 1) gain = 1; gup(&x->sk[sk], 100, gain);
        gup(&b->disc, 100, 2 + g * 3 + (pr == GP_CLEVER ? 3 : 0) + x->sk[1] / 20); if (g < 2) gup(&b->hap, 4, -1); if (g >= 2) gup(&b->bond, 100, 1); gxp(t, 2 + g); gpadd(t, 3); gwish_hit(t, slot, 4); gdiary(GD_TRAIN | (sk + 1) << 5, b->nm); }
    else {
        gup(&b->hap, 4, (g >= 2) + (g >= 4) + (pr == GP_PLAYFUL && g >= 1)); if (g >= 2 && b->wt > 1) b->wt--; gup(&b->bond, 100, (g >= 1) + (g >= 3) * 2 + (pr == GP_PLAYFUL ? 1 : 0) + (m >= 80));
        gup(&x->sk[sk], 100, g >= 2 ? 1 : 0); gxp(t, 1 + g); if (g >= 2) t->wins++; if (g >= 1) { gpadd(t, 1); gwish_hit(t, slot, 2); }
        gfun(si, 2 + g * 2); }
    if (train) snprintf(rmsg, sizeof rmsg, "%sGRADE %s", g < 3 ? "TRAINED. " : "TRAINED! ", gn[g]);
    else if (g >= 3) { int c = (g == 4 ? 10 : 5) + x->sk[2] / 20; gcash(t, c); snprintf(rmsg, sizeof rmsg, "GRADE %s +$%d", gn[g], c); } else snprintf(rmsg, sizeof rmsg, "GRADE %s", gn[g]);
    if (g == 4) gx_award(19);
    if (x->sk[0] >= 60 || x->sk[1] >= 60 || x->sk[2] >= 60) gx_award(18);
    if (game >= 0 && game < 8 && pct > gbest[game]) { if (gbest[game]) { strcat(rmsg, ". NEW BEST!"); gfxk = 4; } if (gbest[game] || pct >= 50) gdiary(GD_BEST | game << 5, b->nm); gbest[game] = (u8)MIN(100, pct); }
    return rmsg; }
// ---- best friends: two pets of one toy grow closer ----
static void gfriend2(Toy *t, int i, int j, int n) {
    Xt *a = &t->x[i], *c = &t->x[j];
    if (a->bff != j + 1) { if (a->fnd < 8 || !a->bff) { a->bff = (u8)(j + 1); a->fnd = 1; } } else if (a->fnd < 100) a->fnd = (u8)MIN(100, a->fnd + n);
    if (c->bff != i + 1) { if (c->fnd < 8 || !c->bff) { c->bff = (u8)(i + 1); c->fnd = 1; } } else if (c->fnd < 100) c->fnd = (u8)MIN(100, c->fnd + n);
    if (a->bff == j + 1 && c->bff == i + 1 && a->fnd >= 10 && a->fnd - n < 10) gdiary(GD_FRIEND, t->b[i].nm); }
static void ggrieve(Toy *t) {   // a pet passed on: the family grieves, a best friend most of all
    for (int i = 0; i < GB_N; i++) { Blob *o = &t->b[i]; if (!o->on || o->stage == GS_EGG) continue; if (o->hap > 1) o->hap--; Xt *x = &t->x[i]; if (x->bff && !t->b[x->bff - 1].on) { x->bff = 0; x->fnd = 0; o->hap = 0; gdiary(GD_MISS, o->nm); } } }
// ---- bath, surprises ----
static const char *gbath(Toy *t, Blob *b) {
    int s = gslot(t, b); Xt *x = &t->x[s]; int pr = b->pers;
    if (b->stage == GS_EGG) return "IT IS STILL AN EGG"; if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (x->hyg >= 90) return "IT IS ALREADY SQUEAKY CLEAN"; if (x->nrg < 5) return "TOO TIRED FOR A BATH";
    x->hyg = 100; b->poop = b->poop > 1 ? b->poop - 1 : 0; gup(&x->nrg, 100, -3); gup(&b->bond, 100, 1); gxp(t, 2); gtouch(t, b); gpadd(t, 2);
    if (gwish_hit(t, s, 3)) return "ITS WISH! SPLASH! SO CLEAN";
    if (pr == GP_GRUMPY || pr == GP_LAZY) { gup(&b->hap, 4, -1); return "SPLASH! GRUMBLES BUT IS CLEAN"; }
    if (pr == GP_GENTLE || pr == GP_CLEVER) { gup(&b->hap, 4, 1); return "SPLASH! IT LOVES BATH TIME"; }
    return "SPLASH! SQUEAKY CLEAN"; }
static int gev_roll(Toy *t, int slot) {   // 0 nothing; 1 a fly, 2 a coin, 3 a shooting star, 4 a storm, 5 a visitor
    Blob *b = &t->b[slot]; if (!b->on || b->stage < GS_BABY || (b->fl & GF_SLEEP) || gneeds(b)) return 0;
    int r = gr() % 10; if (b->pers == GP_GRUMPY && r < 2) return 1; if (b->pers == GP_GENTLE && r == 9) return 5;
    return r < 3 ? 1 : r < 5 ? 2 : r < 6 ? 3 : r < 8 ? 4 : 5; }
static const char *gev_do(Toy *t, int slot, int kind, int ok) {   // the reaction (ok: pressed in time)
    Blob *b = &t->b[slot]; Xt *x = &t->x[slot]; static char em[56]; int ch = x->sk[2] / 12;
    gdiary(GD_EVENT, b->nm); gfxk = ok ? 5 : 0;
    switch (kind) {
    case 1: if (ok) { gup(&b->bond, 100, 2); gup(&b->hap, 4, 1); return "SWAT! IT LAUGHS AT YOU"; } gup(&x->hyg, 100, -6); return "THE FLY LANDED ON IT. YUCK";
    case 2: if (ok) { int c = 5 + gr() % 10 + ch; gcash(t, c); snprintf(em, sizeof em, "FOUND A COIN! +$%d", c); return em; } return "THE COIN ROLLED AWAY";
    case 3: if (ok) { if (!(x->wish & 7)) x->wish = (u8)(1 + gr() % 6); gup(&b->hap, 4, 1); gup(&x->nrg, 100, 15); gxp(t, 2); return "YOU WISHED ON A STAR. IT FEELS HOPEFUL"; } return "THE STAR FADED";
    case 4: if (ok) { gup(&b->bond, 100, 3); gxp(t, 2); gtouch(t, b); return "THUNDER! YOU HELD IT CLOSE. SO BRAVE"; } gup(&b->hap, 4, -1); return "THUNDER! IT HID, SCARED";
    default: if (ok) { int c = 8 + gr() % 12 + ch; gcash(t, c); gup(&b->hap, 4, 1); snprintf(em, sizeof em, "A VISITOR GAVE A GIFT. +$%d", c);
                       if (!(gr() % 25)) { for (int e = 0; e < GB_N; e++) if (!t->b[e].on) { gegg(t, e); return "A VISITOR LEFT A MYSTERY EGG!"; } } return em; } return "THE VISITOR WAVED AND WENT";
    } }
static int gurgent(const Toy *t, int s) {   // what a pet needs most: 0 nothing, 1 medicine, 2 clean, 4 scold a fib, 5 food, 6 lights out, 7 a game, 8 a bath, 9 a nap
    const Blob *b = &t->b[s]; const Xt *x = &t->x[s]; if (!b->on || b->stage == GS_EGG) return 0;
    if (b->fl & GF_SICK) return 1; if (b->poop) return 2; if ((b->fl & GF_FAKE) && !(b->fl & GF_SLEEP)) return 4;
    if ((b->fl & GF_SLEEP) && !(b->fl & GF_DARK)) return 6; if (b->fl & GF_SLEEP) return 0; if (b->hun < 3) return 5; if (b->hap < 2) return 7;
    if (x->nrg < 20 && b->stage >= GS_CHILD) return 9; if (x->hyg < 30) return 8; return 0; }
// ---- one game minute of one creature ----
static void ghatch(Toy *t, Blob *b) { b->stage = GS_BABY; b->age = 0; b->hun = 3; b->hap = 3; gdex_see(b->sp, 0); t->hatched++; if (gsim_of(t) == ctl) ghpend = gslot(t, b) + 1; gdiary(GD_HATCH, b->nm); say("A Boreagotchi hatched!"); }
static void gtick(Toy *t, Blob *b) {   // one game minute of one creature (D64: the v48 rules: energy, cleanness, wishes, loneliness)
    if (!b->on) return;
    if (b->age < 65000) b->age++;
    if (b->stage == GS_EGG) { if (b->age >= 20) ghatch(t, b); return; }
    Xt *x = &t->x[gslot(t, b)]; int n = gnight(), sl = (b->fl & GF_SLEEP) != 0, p = b->pers, st = b->stage;
    if (!sl && st >= GS_CHILD && (n || x->nrg < 8)) b->fl |= GF_SLEEP;                                // night, or worn out: a nap by day
    else if (sl && !n && x->nrg >= (p == GP_LAZY ? 90 : 60)) b->fl &= (u8)~(GF_SLEEP | GF_DARK);    // rested (the lazy sleep in)
    sl = (b->fl & GF_SLEEP) != 0;
    int lv = glevel(t);
    {   static const u8 nP[6] = { 0, 16, 9, 11, 12, 8 }, hP[6] = { 0, 14, 18, 20, 26, 20 }, pw[8] = { 4, 6, 2, 3, 1, 4, 5, 2 };
        if (sl) { if (x->nrg < 100 && b->hun && ((b->fl & GF_DARK) || (b->age & 1))) x->nrg++; if ((x->wish & 7) == 6) { x->wish = 0; if (b->bond < 100) b->bond++; } }
        else { if (x->nrg && st > GS_BABY && (b->age % nP[st] == 0 || (!b->hun && b->age % 6 == 0))) x->nrg--; if (x->hyg && b->age % hP[st] == 0) x->hyg--; }   // (a baby never tires)
        if (b->poop && x->hyg && b->age % 6 == 0) x->hyg--;
        if (b->age % 60 == 0 && x->lone < 255) x->lone++;
        if (x->lone >= (p == GP_PLAYFUL ? 6 : p == GP_SHY ? 20 : 12) && !sl) { if (b->age % 45 == 0 && b->hap) b->hap--; if (b->age % 180 == 0 && b->bond) b->bond--; }   // lonely
        if (x->hyg < 25 && b->age % 120 == 0 && b->bond) b->bond--;
        if (st == GS_TEEN && b->age % 360 == 180 && b->disc) b->disc--;                                // teens push back
        if (!(x->wish & 7)) { if (!sl && b->age % 80 == 40 && !(gr() & 3)) { int w = (gr() & 1) ? pw[p & 7] : 1 + gr() % 6; if (st == GS_BABY && w != 1) w = 5; x->wish = (u8)w; } }
        else if (b->age % 60 == 0) { x->wish += 8; if (x->wish >= 80) { x->wish = 0; if (b->bond) b->bond--; } }   // a wish unmet for ten hours is forgotten
    }
    int rate = (b->stage == GS_BABY ? 30 : b->stage == GS_CHILD ? 45 : 60) + lv * 4; if (p == GP_GREEDY) rate = rate * 3 / 4; if (sl) rate *= 3;
    int hrate = rate + 10; if (p == GP_PLAYFUL) hrate -= 12; if (p == GP_LAZY) hrate += 15; if (x->nrg < 20 || x->hyg < 20) hrate = hrate * 2 / 3;   // tired or grubby: grumpier
    if (b->age % rate == 0 && b->hun) b->hun--;
    if (b->age % hrate == 0 && b->hap) b->hap--;
    if (!sl && b->meal && b->age % 90 == 45 && b->poop < 3) b->poop++;
    if ((b->poop || x->hyg < 20) && b->age % 60 == 30 && ((gr() * 100) >> 8) < (b->poop ? 35 : 18) - lv * 4 - (p == GP_GENTLE ? 12 : 0)) b->fl |= GF_SICK;
    if (!(b->fl & GF_FAKE) && !gneeds(b) && !sl && b->age % 150 == 75 && ((gr() & 1) || (p == GP_GRUMPY && (gr() & 1)))) b->fl |= GF_FAKE;
    if (gneeds(b)) { b->fl |= GF_CALL; if (++b->needT >= 15 + lv * 2) { b->needT = 0; if (b->miss < 99) b->miss++; if (b->bond) b->bond--; b->fl &= (u8)~GF_FAKE; } }
    else { b->fl &= (u8)~GF_CALL; b->needT = 0; if (!sl && b->hun >= 3 && b->hap >= 3 && b->age % (p == GP_GENTLE ? 90 : 120) == 0 && b->bond < 100) b->bond++; }
    static const u16 grow[5] = { 0, 240, 720, 1440, 0 };
    if (b->stage < GS_ADULT && b->age >= grow[b->stage]) {
        b->stage++; b->age = 0;
        if (b->stage == GS_TEEN) b->form = b->miss <= 2 ? 0 : 1;
        if (b->stage == GS_ADULT) {
            b->form = (b->miss <= 1 && b->bond >= 80 && b->disc >= 50) ? 3 : (b->snk > b->meal && b->snk >= 6) ? 2 : (b->miss <= 2 && b->disc >= 50) ? 0 : 1;
            gdex_see(b->sp, b->form); gdiary(GD_ADULT | b->form << 5, b->nm);
            { int s2 = x->sk[0] >= x->sk[1] ? (x->sk[0] >= x->sk[2] ? 0 : 2) : (x->sk[1] >= x->sk[2] ? 1 : 2); for (int q = 0; q < GLN; q++) if (t->ln[q].id == b->id) t->tal[q] = (u8)(x->sk[s2] > 9 ? (s2 + 1) | (x->sk[s2] / 4) << 2 : 0); }   // what it was best at stays in the family
            if (b->form == 0 || b->form == 3) say(b->form == 3 ? "A rare STAR form!" : "A SUPER Bore! So proud"); else say("Your Boreagotchi grew up");
        } else gdiary(GD_GROW | b->stage << 5, b->nm);
        b->miss = 0; }
    if (b->stage == GS_ADULT && b->age >= 2880) { b->stage = GS_ELDER; b->age = 0; gln_set(t, b, 0); gdiary(GD_GROW | GS_ELDER << 5, b->nm); say("A Boreagotchi is an ELDER now"); }
    if (b->stage == GS_ELDER && b->age >= 1440 + b->bond * 6 + (b->form == 3 ? 720 : 0) + lv * 40) { static char pm[56]; snprintf(pm, sizeof pm, "%s passed on. Always remembered", ggiv[b->nm & 31]); say(pm); gdiary(GD_PASS, b->nm); gln_set(t, b, 1); t->mem++; t->memnm = b->nm; t->memsp = b->sp; b->on = 0; ggrieve(t); return; }
    if (b->hun == 0 && b->hap == 0 && (b->fl & GF_SICK) && b->miss >= 12) { b->on = 0; gdiary(GD_PASS, b->nm); gln_set(t, b, 1); ggrieve(t); say("A Boreagotchi went home"); }
}
static const char *const gfoodNm[NFOOD] = { "RICE BALL", "MISO SOUP", "NOODLES", "GRILLED FISH", "VEG STEW", "BERRY PIE", "COOKIE", "CANDY", "FRUIT", "ICE CREAM", "POPCORN", "PUDDING" };
static const u8 gfoodCost[NFOOD] = { 0, 0, 0, 5, 5, 10, 0, 5, 0, 10, 5, 10 };
static const char *gfeed(Toy *t, Blob *b, int food, int pay) {   // (D55) twelve foods, a favourite and a dislike each; 0..5 meals, 6..11 snacks (D64: food gives energy; a wish; a goal)
    int snack = food >= 6, s = gslot(t, b); Xt *x = &t->x[s];
    if (b->stage == GS_EGG) return "IT IS STILL AN EGG";
    if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (!snack && b->hun >= 4) return "IT REFUSES. IT IS FULL";
    if (snack && b->hap >= 4 && b->hun >= 4) return "IT IS FULL AND HAPPY";
    if (pay && funds < gfoodCost[food]) return "NOT ENOUGH CASH FOR THAT";
    if (pay) funds -= gfoodCost[food];
    if ((b->taste & 15) == food && b->pers != GP_GREEDY) { if (b->hap) b->hap--; return "YUCK! IT HATES THAT. IT SPITS IT OUT"; }
    int love = (b->taste >> 4) == food;
    if (!snack) { b->hun++; if (b->wt < 99) b->wt++; if (b->meal < 250) b->meal++; if (!(gr() & 3) && b->bond < 100) b->bond++; }
    else { if (b->hap < 4) b->hap++; if (b->wt < 98) b->wt += 2; if (b->snk < 250) b->snk++; }
    if (love || b->pers == GP_GREEDY) { if (b->hap < 4) b->hap++; b->bond = (u8)MIN(100, b->bond + 2); }
    gxp(t, love ? 2 : 1); gtouch(t, b); gup(&x->nrg, 100, snack ? 6 : 4);
    if (pay) { if (!snack) gpadd(t, 0); if (gwish_hit(t, s, 1)) return "ITS WISH! IT LOVES YOU"; }
    return love ? "ITS FAVOURITE! HAPPY WIGGLES" : snack ? "CRUNCH. A SNACK. SO RELAXED" : "NOM NOM. A MEAL"; }
static const char *gquick_feed(Toy *t, Blob *b) {   // one-press care: its favourite if it is free or cheap, else the first free food it does not hate; a snack when it is full but sad
    int snack = b->hun >= 4 && b->hap < 4, base = snack ? 6 : 0, f = -1, fav = b->taste >> 4;
    if (fav >= base && fav < base + 6 && gfoodCost[fav] <= 5 && gfoodCost[fav] <= funds) f = fav;
    for (int i = 0; i < 6 && f < 0; i++) if (gfoodCost[base + i] == 0 && ((b->taste & 15) != base + i || b->pers == GP_GREEDY)) f = base + i;
    if (f < 0) f = base;
    return gfeed(t, b, f, 1); }
static const char *glight(Toy *t, Blob *b) {   // lights out helps it sleep; by day, with a tired pet, it is a nap (it wakes when rested)
    int s = gslot(t, b); Xt *x = &t->x[s]; b->fl ^= GF_DARK; gtouch(t, b);
    if ((b->fl & GF_DARK) && !(b->fl & GF_SLEEP) && b->stage >= GS_CHILD && x->nrg < 70) { b->fl |= GF_SLEEP; gwish_hit(t, s, 6); return "LIGHTS OUT. IT TAKES A NAP"; }
    if (!(b->fl & GF_DARK) && (b->fl & GF_SLEEP) && x->nrg < 60 && !gnight()) { b->fl &= (u8)~GF_SLEEP; return "LIGHTS ON. IT WAKES UP, YAWNING"; }
    return (b->fl & GF_DARK) ? ((b->fl & GF_SLEEP) ? "LIGHTS OUT. GOOD NIGHT" : "LIGHTS OUT") : "LIGHTS ON"; }
static const char *gmedicine(Toy *t, Blob *b) { if (!(b->fl & GF_SICK)) return "IT IS NOT SICK. IT SPITS IT OUT"; b->fl &= (u8)~GF_SICK; if (b->bond < 100) b->bond++; gxp(t, 2); gtouch(t, b); return "ALL BETTER"; }
static const char *gclean(Toy *t, Blob *b) { if (!b->poop) return "NOTHING TO CLEAN"; b->poop = 0; if (b->bond < 100) b->bond++; gup(&t->x[gslot(t, b)].hyg, 100, 10); gxp(t, 2); gtouch(t, b); return "FLUSHED"; }
static const char *gscold(Toy *t, Blob *b) {
    int cl = b->pers == GP_CLEVER ? 10 : 0;
    if (b->fl & GF_FAKE) { b->fl &= (u8)~GF_FAKE; gtouch(t, b); gup(&b->disc, 100, 25 + cl); gxp(t, 3); return "IT KNOWS BETTER NOW. DISCIPLINE UP"; }
    if (b->hap) b->hap--; return "IT DID NOTHING WRONG. IT IS SAD NOW"; }
static int gpower(const Blob *b) { return (b->hun + b->hap) * 3 + b->disc / 8 + b->stage * 6 + b->bond / 10 - b->miss * 2 - (b->fl & GF_SICK ? 10 : 0) + (b->form == 0 || b->form == 3 ? 4 : 0); }
static void gx_award(int bit);
static const char *gmeet(Toy *t, int a, Toy *u, int c, int battle) {   // (D55) a playdate or battle, in the family or with a housemate's pet
    Blob *x = &t->b[a], *y = &u->b[c];
    if (x == y || !x->on || !y->on || x->stage == GS_EGG || y->stage == GS_EGG) return "THEY BOTH NEED TO HAVE HATCHED";
    if ((x->fl | y->fl) & GF_SLEEP) return "SHHH. SOMEONE IS ASLEEP";
    if (battle) { int px = gpower(x) + gr() % 12 + (x->pers == 5) * 5, py = gpower(y) + gr() % 12 + (y->pers == 5) * 5; Blob *w = px >= py ? x : y, *l = px >= py ? y : x;
        if (w->bond < 97) w->bond += 3; if (l->hap) l->hap--; w->disc = (u8)MIN(100, w->disc + 6); gxp(t, 3); t->wins++; gdiary(GD_PLAY, x->nm);
        return w == x ? "YOUR PET WON THE BATTLE!" : "THE OTHER PET WON THE BATTLE!"; }
    int rel = t == u ? grel(t, x->id, y->id) : 0, sh = (x->pers == 6) + (y->pers == 6), gain = MAX(1, 4 + rel - sh);
    if (x->hap < 4) x->hap++; if (y->hap < 4) y->hap++; x->bond = (u8)MIN(100, x->bond + gain); y->bond = (u8)MIN(100, y->bond + gain); t->plays++; if (u != t) u->plays++; gxp(t, 3); gdiary(GD_PLAY, x->nm);
    gfun(gsim_of(t), 8); if (u != t) gfun(gsim_of(u), 6);
    if (rel) return "FAMILY PLAYS TOGETHER. A LOVELY GAME";
    if (x->stage >= GS_ADULT && y->stage >= GS_ADULT && x->bond >= 60 && y->bond >= 60) {   // two grown-ups who love each other: a couple, and then eggs
        if (t == u && !x->mate && !y->mate) { x->mate = y->id; y->mate = x->id; gdiary(GD_BOND, x->nm); return "THEY FELL IN LOVE! A NEW PAIR"; }
        if ((t != u || x->mate == y->id) && (gr() & 1)) { Toy *h = t; int s = -1; for (int e = 0; e < GB_N && s < 0; e++) if (!t->b[e].on) s = e;
            if (s < 0 && u != t) { h = u; for (int e = 0; e < GB_N && s < 0; e++) if (!u->b[e].on) s = e; }
            if (s >= 0) { int mid = gimport(h, t, x), did = gimport(h, u, y); gbreed(h, x, y, mid, did); if (t != u) gx_award(14); return t != u ? "TWO FAMILIES MET... AND LEFT AN EGG!" : "THEY PLAYED... AND LEFT AN EGG!"; }
            return "A LOVELY PLAYDATE. NO ROOM FOR AN EGG"; } }
    return "A LOVELY PLAYDATE. BOTH ARE HAPPY"; }
static const char *glunch(Toy *t, Blob *b) {
    int si = gsim_of(t); if (b->stage == GS_EGG) return "IT IS STILL AN EGG"; if (b->fl & GF_SLEEP) return "SHHH. IT IS ASLEEP";
    if (gstat(si, 1) < 12) return "YOU ARE TOO HUNGRY TO SHARE. EAT FIRST!";
    if (b->hun >= 4) return "IT IS FULL. KEEP YOUR LUNCH"; gfood(si, -15); b->hun = 4; if (b->hap < 4) b->hap++; b->bond = (u8)MIN(100, b->bond + 3); if (b->meal < 250) b->meal++; gxp(t, 2); gtouch(t, b); gpadd(t, 0);
    return "YOU GAVE IT YOUR LUNCH. YOUR TUMMY GROWLS"; }
static const char *gnuzzle(Toy *t, Blob *b) {
    int si = gsim_of(t), h = (minutes / 60) & 255; if (b->stage == GS_EGG) return "IT IS STILL AN EGG"; if (gneeds(b)) return gwant(b);
    if (t->pad == (u8)h) return "IT ALREADY NUZZLED YOU THIS HOUR"; t->pad = (u8)h; gfun(si, 6); if (b->bond < 100) b->bond++; gxp(t, 1); gtouch(t, b); gpadd(t, 4);
    if (gwish_hit(t, gslot(t, b), 5)) return "ITS WISH! IT NUZZLES YOU"; return "ALL GOOD. IT NUZZLES YOU. AWW"; }
static int gkin(int si) { return gstat(si, 1) >= 20 && gstat(si, 0) > -40; }
static int gparent(Toy *t, const Blob *b) {   // a grown-up, awake, close parent of a baby or child
    if (b->stage > GS_CHILD) return 0; for (int k = 0; k < 2; k++) { Blob *p = gby_id(t, k ? b->dad : b->mom); if (p && p->stage >= GS_ADULT && !(p->fl & GF_SLEEP) && p->bond >= 40) { if (!(gr() & 7)) p->bond = (u8)MIN(100, p->bond + 1); return 1; } } return 0; }
static void gfamily(Toy *t, int si) {   // once an hour: mates with an egg of love, brothers and sisters playing
    for (int i = 0; i < GB_N; i++) { Blob *x = &t->b[i]; if (!x->on) continue;
        if (x->mate && !gby_id(t, x->mate)) x->mate = 0;
        if (x->stage == GS_ADULT && x->mate && x->bond >= 60) { Blob *y = gby_id(t, x->mate);
            if (y && y->stage == GS_ADULT && y->bond >= 60 && x->id < y->id && !(gr() & 7)) { int has = 0; for (int q = 0; q < GB_N; q++) if (t->b[q].on && t->b[q].stage == GS_EGG && t->b[q].mom == x->id) has = 1;
                if (!has && gbreed(t, x, y, x->id, y->id) && si == ctl) say("Love left an egg in the basket"); } }
        if (x->stage >= GS_CHILD && !(x->fl & GF_SLEEP) && x->bond >= 50 && x->hap >= 3 && si >= 0 && !(gr() % 14)) { int g = 20 + gr() % 40; if (si == ctl) { static char gb[48]; funds += g; snprintf(gb, sizeof gb, "Your Boreagotchi brought a gift: $%d", g); say(gb); } gfun(si, 4); gdiary(GD_GIFT, x->nm); }   // (D55) a gift for its owner
        if (x->stage > GS_EGG && x->mom) for (int j = i + 1; j < GB_N; j++) { Blob *y = &t->b[j]; if (y->on && y->stage > GS_EGG && y->mom == x->mom && !(gr() & 3)) { if (x->hap < 4) x->hap++; if (y->hap < 4) y->hap++; if (x->bond < 99) x->bond++; if (y->bond < 99) y->bond++; } } } }
// ---- (D55) accessories, awards (the PS1's sixteen), playdates between families ----
static const char *const gaccNm[NACC] = { "NOTHING", "BOW", "TOP HAT", "SHADES", "SCARF", "FLOWER", "CROWN", "PARTY HAT" };
static const u8 gaccCost[NACC] = { 0, 20, 40, 40, 30, 20, 100, 30 };
static const char *gdress(Toy *t, Blob *b, int acc) {
    if (b->stage == GS_EGG) return "AN EGG HAS NO HEAD YET";
    if (acc && !(t->accs & (1 << acc))) { if (funds < gaccCost[acc]) return "NOT ENOUGH CASH"; funds -= gaccCost[acc]; t->accs |= (u8)(1 << acc); }
    b->acc = (u8)acc; if (b->hap < 4) b->hap++; gxp(t, 1); return acc ? "LOOKING GOOD!" : "ALL NATURAL"; }
static const char *const gachNm[NACH] = { "FIRST HATCH", "ALL GROWN UP", "STAR QUALITY", "COLLECTOR", "FIRST FAMILY", "GRANDPARENT", "FULL HOUSE", "BEST FRIENDS", "SHINY", "REMEMBERED", "PLAYMATES", "DYNASTY", "GOOD CARER", "BLOBDEX STAR", "TWO FAMILIES", "GOTCHI MASTER",
    "3 DAY STREAK", "WEEK STREAK", "SKILLED", "TOP SCORE" };
static const char *const gachHow[NACH] = { "HATCH AN EGG", "RAISE A GROWN-UP", "RAISE A STAR FORM", "24 FORMS IN THE BLOBDEX", "A PAIR LAYS AN EGG", "A GRANDCHILD IS BORN", "FIVE PETS IN ONE TOY", "A PET WITH FULL HEARTS",
    "A GOLD OR SHINY PET", "A PET PASSES ON", "10 PLAYDATES", "FIVE GENERATIONS", "CARE SCORE OF 500", "40 FORMS IN THE BLOBDEX", "AN EGG FROM TWO FAMILIES", "REACH CARE LEVEL 5", "ALL GOALS 3 DAYS RUNNING", "ALL GOALS 7 DAYS RUNNING", "A SKILL OF 60", "GRADE S IN A GAME" };
static int gdepth(const Toy *t, int id, int n) { const Ln *l = gln_of(t, id); if (!l || n > 6) return 0; int a = l->mom ? 1 + gdepth(t, l->mom, n + 1) : 0, b = l->dad ? 1 + gdepth(t, l->dad, n + 1) : 0; return MAX(a, b); }   // generations above it
static int gscore(void) {
    int s = 0, n = 0; for (int k = 0; k < GT_MAX; k++) if (gtoy[k].own) { const Toy *t = &gtoy[k]; s += t->xp / 2 + t->hatched * 5 + t->wins * 2 + t->plays; for (int i = 0; i < GLN; i++) if (t->ln[i].id) s += gdepth(t, t->ln[i].id, 0) * 8 + ((t->ln[i].nm & 128) ? 5 : 0); }
    for (int i = 0; i < NACH; i++) n += ghas(i); return s + n * 25 + gdex_count() * 3; }
static void gx_award(int bit) {
    if (ghas(bit)) return; gach |= 1u << (bit < 16 ? bit : bit + 5); Toy *t = gtoy_of(ctl); if (!t) return; int g = 50 + bit * 10; funds += g; { static char b[48]; snprintf(b, sizeof b, "Boreagotchi award! +$%d", g); say(b); } gfun(ctl, 6); gdiary(GD_AWARD, bit); gfxk = 4; }
static void gx_check(void) {
    int sum = 0, big = 0, ad = 0, star = 0, any = 0, deep = 0, kids = 0, gone = 0;
    for (int k = 0; k < GT_MAX; k++) if (gtoy[k].own) { const Toy *t = &gtoy[k]; any = 1; sum += t->plays; if (t->hatched) gx_award(0); if (glevel(t) >= 5) gx_award(15);
        int n = 0; for (int i = 0; i < GB_N; i++) { const Blob *b = &t->b[i]; if (!b->on) continue; n++; if (b->stage >= GS_ADULT) { ad = 1; if (b->form == 3) star = 1; } if (b->gen >= 4) big = 1; if (b->mut || b->col == 3) gx_award(8); if (b->bond >= 100) gx_award(7); }
        if (n >= 5) gx_award(6);
        for (int i = 0; i < GLN; i++) if (t->ln[i].id) { if (t->ln[i].mom) kids = 1; if (gdepth(t, t->ln[i].id, 0) >= 2) deep = 1; if (t->ln[i].nm & 128) gone = 1; } }
    if (!any) return;
    if (ad) gx_award(1); if (star) gx_award(2); if (gdex_count() >= 24) gx_award(3); if (kids) gx_award(4); if (deep) gx_award(5); if (gone) gx_award(9); if (sum >= 10) gx_award(10); if (big) gx_award(11); if (gscore() >= 500) gx_award(12); if (gdex_count() >= 40) gx_award(13); }
static int gawake(const Blob *b, int minstage) { return b->on && b->stage >= minstage && !(b->fl & GF_SLEEP); }
static void gvisits(void) {   // housemates' pets visit each other; grown-ups who love each other leave an egg for the two families
    int a = gr() % GT_MAX, c = gr() % GT_MAX; Toy *x = &gtoy[a], *y = &gtoy[c]; if (a == c || !x->own || !y->own) return;
    int i = gr() % GB_N, j = gr() % GB_N; Blob *p = &x->b[i], *q = &y->b[j]; if (!gawake(p, GS_CHILD) || !gawake(q, GS_CHILD) || p->stage == GS_ELDER || q->stage == GS_ELDER) return;
    if (p->hap < 4) p->hap++; if (q->hap < 4) q->hap++; if (p->bond < 100) p->bond++; if (q->bond < 100) q->bond++; x->plays++; y->plays++; gdiary(GD_VISIT, p->nm);
    int sa = gsim_of(x), sc = gsim_of(y); gfun(sa, 3); gfun(sc, 3); if (sa == ctl || sc == ctl) say("Your Boreagotchi had a playdate with a housemate's");
    if (p->stage >= GS_ADULT && q->stage >= GS_ADULT && p->bond >= 60 && q->bond >= 60 && !(gr() & 3)) { Toy *h = sa == ctl ? x : y; for (int e = 0; e < GB_N; e++) if (!h->b[e].on) { int mid = gimport(h, x, p), did = gimport(h, y, q); gbreed(h, p, q, mid, did); say("Two families met and left an egg!"); gx_award(14); break; } } }
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
                if (b->hun == 0) gfeed(t, b, (b->taste >> 4) % 6, 0); else if (b->poop) gclean(t, b); else if (b->fl & GF_SICK) gmedicine(t, b); else if (b->fl & GF_FAKE) gscold(t, b); else if (b->hap == 0) b->hap = 2; } } }
    for (int k = 0; k < GT_MAX; k++) {   // and the other way: pets in a bad way drag their owner down, happy ones lift them
        Toy *t = &gtoy[k]; if (!t->own) continue; int si = gsim_of(t); if (si < 0 || minutes % 60) continue;
        int n = 0, glum = 0, glad = 0; for (int i = 0; i < GB_N; i++) { Blob *b = &t->b[i]; if (!b->on || b->stage == GS_EGG) continue; n++; if (b->hun == 0 && b->hap == 0) glum++; else if (b->hap >= 3 && b->hun >= 2) glad++; }
        gfamily(t, si);
        if (glum) gfun(si, -3 * glum); else if (n >= 2 && glad == n) gfun(si, 3); }
    if (minutes % 60 == 0) { if (minutes % 120 == 0) gvisits(); gx_check(); }
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
static int gt_pack(u8 *o, int cap) {
    int n = 0, nt = 0; for (int k = 0; k < GT_MAX; k++) if (gtoy[k].own) nt++;
    if (!nt && !gdex[0] && !gdex[1]) return 0;
    u8 *q = o + 4; memcpy(q, gdex, 8); q += 8; *q++ = (u8)nt;
    for (int k = 0; k < GT_MAX; k++) { const Toy *t = &gtoy[k]; if (!t->own) continue; *q++ = t->uid; *q++ = t->sel; memcpy(q, &t->xp, 2); q += 2; memcpy(q, &t->hatched, 2); q += 2; memcpy(q, &t->wins, 2); q += 2; memcpy(q, &t->cared, 2); q += 2; *q++ = t->pad;
        for (int i = 0; i < GB_N; i++) { const Blob *b = &t->b[i]; *q++ = b->on; if (b->on) { memcpy(q, &b->stage, 16); q += 16; memcpy(q, &b->age, 2); q += 2; } } }
    *q++ = 'F';   // (D54) the families, after everything an older game knows
    for (int k = 0; k < GT_MAX; k++) { const Toy *t = &gtoy[k]; if (!t->own) continue; *q++ = t->nid; *q++ = t->mem; *q++ = t->memnm; *q++ = t->memsp; int nl = 0; for (int i = 0; i < GLN; i++) nl += t->ln[i].id != 0; *q++ = (u8)nl; for (int i = 0; i < GLN; i++) if (t->ln[i].id) { memcpy(q, &t->ln[i], 6); q += 6; }
        for (int i = 0; i < GB_N; i++) { const Blob *b = &t->b[i]; if (b->on) { memcpy(q, &b->pers, 7); q += 7; } } }
    *q++ = 'H';   // (D55) the awards, the diary (its last 16 lines, as the old game reads it), each home's walls and things and hats, and each pet's hat and taste
    memcpy(q, &gach, 4); q += 4; *q++ = gdn; { Dy hw[16]; memset(hw, 0, sizeof hw); for (int i = 1; i <= 16; i++) hw[(gdn - i) & 15] = gdy[(gdn - i) & (GD_N - 1)]; memcpy(q, hw, sizeof hw); q += sizeof hw; }
    for (int k = 0; k < GT_MAX; k++) { const Toy *t = &gtoy[k]; if (!t->own) continue; *q++ = t->room; *q++ = t->deco; *q++ = t->accs; memcpy(q, &t->plays, 2); q += 2;
        for (int i = 0; i < GB_N; i++) { const Blob *b = &t->b[i]; if (b->on) { *q++ = b->acc; *q++ = b->taste; } } }
    {   // (D64) 'X': the v48 extras, last, so a save from before still loads (and one from now loads in the old game, which stops before it)
        int sz = 2 + 8 + (int)sizeof gdy; for (int k = 0; k < GT_MAX; k++) if (gtoy[k].own) { sz += 14 + GLN; for (int i = 0; i < GB_N; i++) sz += gtoy[k].b[i].on * (int)sizeof(Xt); }
        if ((int)(q - o) + sz <= cap - 4) {
            *q++ = 'X'; *q++ = 1; memcpy(q, gbest, 8); q += 8; memcpy(q, gdy, sizeof gdy); q += sizeof gdy;
            for (int k = 0; k < GT_MAX; k++) { const Toy *t = &gtoy[k]; if (!t->own) continue; memcpy(q, t->gp, 6); q += 6; *q++ = t->streak; *q++ = t->best; *q++ = t->gflag; *q++ = t->ghist; memcpy(q, &t->gday, 2); q += 2; memcpy(q, &t->gwon, 2); q += 2; memcpy(q, t->tal, GLN); q += GLN;
                for (int i = 0; i < GB_N; i++) if (t->b[i].on) { memcpy(q, &t->x[i], sizeof(Xt)); q += sizeof(Xt); } } } }
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
            else { b->pers = (u8)(b->nm & 7); b->col = (u8)((b->nm >> 3) & 3); if (b->stage == GS_ADULT && b->age > 900) b->age = 900; if (!++t->nid) ++t->nid; b->id = t->nid; gln_set(t, b, 0); } } }
    int hv = q + 70 <= e && *q == 'H';
    if (hv) { q++; memcpy(&gach, q, 4); q += 4; gdn = *q++; { Dy hw[16]; memcpy(hw, q, sizeof hw); q += sizeof hw; memset(gdy, 0, sizeof gdy); for (int i = 1; i <= 16 && i <= gdn; i++) gdy[(gdn - i) & (GD_N - 1)] = hw[(gdn - i) & 15]; } }
    for (int k = 0; k < nt && k < GT_MAX; k++) { Toy *t = &gtoy[k]; t->room = (u8)(k % 6);
        if (hv && q + 5 <= e) { t->room = *q++; t->deco = *q++; t->accs = *q++; memcpy(&t->plays, q, 2); q += 2; }
        for (int i = 0; i < GB_N; i++) { Blob *b = &t->b[i]; if (!b->on) continue; if (hv && q + 2 <= e) { b->acc = *q++; b->taste = *q++; } else { int l = (b->nm + i) % 12; b->taste = (u8)(l << 4 | (l + 1 + b->nm % 10) % 12); } } }
    for (int k = 0; k < nt && k < GT_MAX; k++) for (int i = 0; i < GB_N; i++) if (gtoy[k].b[i].on) gxinit(&gtoy[k], i);   // the v48 needs start full; 'X' (if there) overwrites them
    int xv = hv && q + 2 + 8 + (int)sizeof gdy <= e && q[0] == 'X' && q[1] == 1;
    if (xv) { q += 2; memcpy(gbest, q, 8); q += 8; memcpy(gdy, q, sizeof gdy); q += sizeof gdy;
        for (int k = 0; k < nt && k < GT_MAX; k++) { Toy *t = &gtoy[k]; int nb = 0; for (int i = 0; i < GB_N; i++) nb += t->b[i].on; if (q + 14 + GLN + nb * (int)sizeof(Xt) > e) break;
            memcpy(t->gp, q, 6); q += 6; t->streak = *q++; t->best = *q++; t->gflag = *q++; t->ghist = *q++; memcpy(&t->gday, q, 2); q += 2; memcpy(&t->gwon, q, 2); q += 2; memcpy(t->tal, q, GLN); q += GLN;
            for (int i = 0; i < GB_N; i++) if (t->b[i].on) { memcpy(&t->x[i], q, sizeof(Xt)); q += sizeof(Xt); if (t->x[i].nrg > 100) t->x[i].nrg = 100; if (t->x[i].hyg > 100) t->x[i].hyg = 100; } } } }

// ---- the screens: the creature big on the top screen, the pad on the bottom ----
enum { GV_NONE, GV_BASKET, GV_HAND, GV_DEX, GV_FEED, GV_PLAYPICK, GV_LOOK, GV_COPY, GV_FRIEND, GV_STATS, GV_BOX, GV_GUIDE, GV_TREE, GV_CATCH, GV_HILO, GV_DRESS, GV_HOME, GV_AWARD, GV_DIARY, GV_MORE, GV_SHOP, GV_TITLE, GV_HATCH, GV_DAY, GV_INTRO, GV_TRAIN, GV_GOALS, GV_BEAT, GV_HOP, GV_STACK, GV_WHACK };   // (D53: the Toy Box and the guide) (D64: the unboxing, training, today's goals, three new games)
#define PD_SQ 128
#define PD_L1 256
#define PD_R1 512
#define PD_SEL 1024
#define PD_TR 2048
#define PD_ICON 4096
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
    int hat_top = gminY;
    // face
    ex = rx / 2 > 0 ? rx / 2 : 1; eyY = cy - ry / 3; if (sp == 4 || sp == 6 || sp == 11) eyY = cy - 1; if (sp == 10) eyY = by - ry + 1; if (sp == 13) eyY = by - 4; if (sp == 12 || sp == 15) eyY = cy - 1;
    int e1 = cx + d - ex, e2 = cx + d + ex; if (sp == 6) { e1 = e2 = cx + d * (rx - 1); } if (sp == 13) { e1 = e2 = cx + d * rx; }
    int chill = st == GS_ADULT && b->form == 2, sleepy = (b->fl & GF_SLEEP) != 0;
    if (sp == 9) { gP(e1, eyY, 0); gP(e2, eyY, 0); if (big) { gP(e1 + 1, eyY, 0); gP(e2 + 1, eyY, 0); } }
    else if (sleepy || chill) { gP(e1, eyY, 0); gP(e2, eyY, 0); if (e1 != e2 && big) { gP(e1 - 1, eyY, 0); gP(e2 + 1, eyY, 0); } }
    else { gP(e1, eyY, 0); gP(e2, eyY, 0); if (big || sp == 15) { gP(e1, eyY + 1, 0); gP(e2, eyY + 1, 0); } if (sp == 15) { gP(e1 - 1, eyY, 0); gP(e2 + 1, eyY, 0); } }
    if (sp != 4 && sp != 6) {
        int my = cy + ry / 2 + (sp == 10 ? 1 : 0); if (sp == 13) my = by - 2;
        if (b->hun == 0 || b->hap == 0 || (b->fl & GF_SICK) || gmd == MD_SICK || gmd == MD_HUNGRY || gmd == MD_BORED || gmd == MD_TIRED || gmd == MD_DIRTY || gmd == MD_ANGRY) { gP(cx + d, my, 0); gP(cx + d - 1, my + 1, 0); gP(cx + d + 1, my + 1, 0); }
        else if ((b->hap >= 3 && big) || gmd == MD_HAPPY || gmd == MD_DANCE) { gP(cx + d - 1, my, 0); gP(cx + d, my + 1, 0); gP(cx + d + 1, my, 0); }
        else gP(cx + d, my, 0);
    }
    if (gmd == MD_ANGRY) { gP(e1, eyY - 2, 0); gP(e1 - 1, eyY - 1, 0); gP(e2, eyY - 2, 0); gP(e2 + 1, eyY - 1, 0); }   // (D64) cross brows
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
    // (D60) the hats, as the PS1 draws them: dots 2 red, 3 blue, 4 gold on top of the body
    { int top = hat_top;
    switch (b->acc) {
        case 1: gP(cx - 1, top - 1, 2); gP(cx + 1, top - 1, 2); gP(cx, top - 1, 4); gP(cx - 2, top - 2, 2); gP(cx + 2, top - 2, 2); break;
        case 2: gRC(cx - 2, top - 1, cx + 2, top - 1, 1); gRC(cx - 1, top - 3, cx + 1, top - 2, 1); gRC(cx - 1, top - 2, cx + 1, top - 2, 2); break;
        case 3: gRC(e1 - 1, eyY, e2 + 1, eyY, 1); gP(e1, eyY, 1); gP(e2, eyY, 1); break;
        case 4: gRC(cx - rx + 1, cy + ry - 1, cx + rx - 1, cy + ry - 1, 2); gP(cx + rx - 1, cy + ry, 2); gP(cx + rx - 1, cy + ry + 1, 2); break;
        case 5: gP(cx + rx - 1, top - 1, 2); gP(cx + rx, top - 1, 2); gP(cx + rx - 1, top - 2, 2); gP(cx + rx - 2, top - 1, 2); gP(cx + rx - 1, top - 1, 4); break;
        case 6: gP(cx - 2, top - 1, 4); gP(cx - 2, top - 2, 4); gP(cx, top - 1, 4); gP(cx, top - 3, 4); gP(cx, top - 2, 4); gP(cx + 2, top - 1, 4); gP(cx + 2, top - 2, 4); gRC(cx - 2, top, cx + 2, top, 4); break;
        case 7: gP(cx, top - 4, 2); gRC(cx - 1, top - 3, cx + 1, top - 3, 3); gRC(cx - 2, top - 2, cx + 2, top - 2, 2); gRC(cx - 2, top - 1, cx + 2, top - 1, 3); break;
    } }
}
#include "ds_gegg.h"   // (D69) the PS1's redrawn eggs
static void gegg_dots(const char *const *r, int w, int h, int x, int y, int tilt) {   // a sprite onto the dots; its top leans by tilt dots, the foot stays
    for (int j = 0; j < h; j++) { int sh = tilt * (h - 1 - j) / (h - 1); for (int i = 0; i < w; i++) if (r[j][i] != '.') gP(x + i + sh, y + j, 16 + (int)(strchr(geggKeys, r[j][i]) - geggKeys)); }
}
static void g_egg(const Blob *b, int fr) {   // (D69) centred on the LCD (cols 11 to 20, rows 2 to 14), a slow wobble now and then, faster once it cracks (age 15, 17, 19)
    static const s8 wb[8] = { 1, 2, 1, 0, -1, -2, -1, 0 };
    int cr = b->age >= 19 ? 3 : b->age >= 17 ? 2 : b->age >= 15 ? 1 : 0, per = cr ? 12 : 24, ph = ((fr % per) + per) % per, tilt = ph >= per - 8 ? wb[ph - (per - 8)] : 0;
    geggcol = b->col & 7; gegg_dots(gegg_make(1, gegg_pat(b), cr), FS_EGGL_W, FS_EGGL_H, 11, 2, tilt);
    if (b->mut) { int x = (fr & 2) ? 26 : 5, y = (fr & 2) ? 4 : 6; gP(x, y, 23); gP(x - 1, y, 23); gP(x + 1, y, 23); gP(x, y - 1, 23); gP(x, y + 1, 23); }   // (the rare one twinkles)
}
static void g_scene(const Blob *b, int fr, int look) {
    for (int y = 0; y < 16; y++) for (int x = 0; x < 32; x++) gdot[y][x] = 0;
    gminY = 16;
    if (!b->on) return;
    int d = look ? look : ((fr >> 1) & 1 ? -1 : 1), ox = look ? 0 : (((fr >> 1) % 3) - 1) * 2; if (gmd == MD_DANCE && !look) ox = (fr & 2) ? 3 : -3;
    if (b->stage == GS_EGG) g_egg(b, fr); else g_creature(b, fr, d, ox);
    static const u8 pp[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 1, 0 }, { 1, 1, 1, 1 } };
    for (int i = 0; i < b->poop && i < 3; i++) { int px = 28 - (i & 1) * 4, py = 12 - (i >> 1) * 4; for (int r = 0; r < 4; r++) for (int q = 0; q < 4; q++) if (pp[r][q]) gP(px + q - 1, py + r, 1); }
    if (b->fl & GF_SICK) { static const char *sk[5] = { ".###.", "#.#.#", "#####", ".###.", ".#.#." }; for (int r = 0; r < 5; r++) for (int q = 0; q < 5; q++) if (sk[r][q] == '#') gP(1 + q, 1 + r, 1); }
    if (gmd == MD_DIRTY) { gP(3, 3 + (fr & 1), 1); gP(29, 6 - (fr & 1), 1); gP(5, 6, 1); }   // (D64) flies round a smelly pet
    if (b->fl & GF_SLEEP) { static const char *z[5] = { "###.", "..#.", ".#..", "###.", "...." }; for (int r = 0; r < 5; r++) for (int q = 0; q < 4; q++) if (z[r][q] == '#') gP(24 + q + ((fr >> 2) & 1), 1 + r, 1); }
}

static const char *const giconNm[15] = { "FEED", "PLAY", "LIGHT", "MEDS", "CLEAN", "SCOLD", "PET", "STATS", "TREE", "VISIT", "DRESS", "MORE", "BATH", "TRAIN", "TODAY" };   // (D64) indexed by what guse_icon() does; giconMap says where each sits on the toy
static const char *const gicon[15][7] = {
    { "...#.....", "..#.##...", ".#####...", "#######..", "#######..", ".#####...", "........." },   // feed
    { ".#######.", "#..#....#", "#.###.#.#", "#..#..#.#", "#.......#", ".#######.", "........." },   // play
    { "...###...", "..#...#..", "..#...#..", "...#.#...", "...###...", "...###...", "........." },   // light
    { "...###...", "...#.#...", ".###.###.", ".#.....#.", ".###.###.", "...#.#...", "...###..." },   // meds
    { "..#.#.#..", "...#.#...", "..#####..", "...###...", "...###...", "..#####..", "........." },   // clean
    { ".#####...", ".#...#...", ".#.#.#...", ".#####...", "...#.....", "...###...", "........." },   // scold
    { "..##.##..", ".#######.", ".#######.", "..#####..", "...###...", "....#....", "........." },   // pet (a heart)
    { "#........", "#..#.....", "#..#..#..", "#.##..#..", "#.##.##..", "#########", "........." },   // stats
    { ".##...##.", "#..#.#..#", ".##...##.", "..#...#..", ".###.###.", "#..#.#..#", "........." },   // family
    { "....#....", "....##...", "#######..", "########.", "#######..", "....##...", "....#...." },   // visit (an arrow)
    { "..#####..", "..#...#..", "..#####..", "..#####..", "#########", ".#######.", "........." },   // dress (a hat)
    { "........." , "#########", "........." , "#########", "........." , "#########", "........." },   // more
    { "..#...#..", ".#.#.#.#.", "..#...#..", "#########", "#.......#", ".#######.", "........." },   // bath: a tub with bubbles
    { "##.....##", "##.###.##", "#########", "##.###.##", "##.....##", "........." , "........." },   // train: a weight
    { "#.#####..", "#.......#", "#.#####.#", "#.......#", "#.#####..", "#.......#", "..#####.." } };   // today: a list

static int gdots_plain;   // 1: one ink only (a silhouette, a memorial)
static void gdots_draw(int x, int y, int s, u16 c) {   // the LCD's dots onto the screen: runs of one ink joined into one box (D60: 2 red, 3 blue, 4 gold for the hats)
    static const u16 acc[5] = { 0, 0, C15(27, 6, 8), C15(8, 16, 27), C15(27, 19, 1) };
    for (int j = 0; j < 16; j++) for (int i = 0; i < 32;) { int v = gdot[j][i]; if (!v) { i++; continue; } int k = i; while (k < 32 && gdot[j][k] == v) k++;
        if (v >= 16) { rect(x + i * s, y + j * s, (k - i) * s, s, gegg_c(geggcol, v - 16)); i = k; continue; }   // (D69) an egg: its own colours, square dots, no gap
        rect(x + i * s, y + j * s, (k - i) * s - (s > 2), s - (s > 2), v > 1 && v < 5 && !gdots_plain ? acc[v] : c); i = k; } }
#define LCD_BG C15(21, 24, 16)
#define LCD_INK C15(3, 5, 3)
static void gt_open_basket(int si) { Toy *t = gtoy_of(si); if (!t) { gsay(si == ctl ? "NO BOREAGOTCHI YET. A SHOP SELLS THEM" : "THEY HAVE NO TOY YET"); return; } gti = (int)(t - gtoy); gview = GV_BASKET; gbi = t->sel < GB_N ? t->sel : 0; gmsgT = 0; gmsg = 0; }
// ==== (D64) the PS1's v48 screens on the DS: the LCD at (48,6), 160 x 94 dots; meters, chips and messages under it; 14 icons on the touch screen ====
#define LX 48
#define LY 6
#define PX(x) ((x) - 32)   // the PS1's LCD box (80,18) lands at (48,6): its games and pages keep their own numbers
#define PY(y) ((y) - 12)
#define GICN 14
#define GACH_TUT 0x70000u        // gach bits 16-18: how far the first-day script went; bit 19 done or skipped; bit 20 the unboxing was seen
#define GACH_TUTOFF 0x80000u
#define GACH_INTRO 0x100000u
static int gtrainMode, gtrAlt, gevK, gevT, gevCool, gevHit, gbT, gtutT, gtutN, gwishSeen, gtutBase, glastGame, giT, gisub;   // training round; surprise kind, clock, cool-down, visitor kind; reaction clock; first-day script; last game; unbox clock
typedef struct { s16 x, y; s8 vx, vy; u8 life, kind; } GPar;   // hearts, stars, bubbles, coins, confetti that rise and fall over the LCD (1/8 pixels)
static GPar gpar[28];
static void gemit(int kind, int x, int y, int n) {
    for (int i = 0; i < n; i++) { GPar *p = 0; for (int j = 0; j < 28; j++) if (!gpar[j].life) { p = &gpar[j]; break; } if (!p) return;
        p->x = (s16)(x * 8 + (grr() % 24 - 12) * 2); p->y = (s16)(y * 8); p->kind = (u8)kind; p->life = (u8)(40 + grr() % 36);
        p->vx = (s8)(grr() % 9 - 4); p->vy = (s8)(kind == 1 || kind == 3 ? -(6 + grr() % 8) : -(3 + grr() % 4)); if (kind == 4) { p->vx = (s8)(grr() % 21 - 10); p->vy = (s8)(-(8 + grr() % 10)); } } }
static int gpar_live(void) { for (int j = 0; j < 28; j++) if (gpar[j].life) return 1; return 0; }
static void gpar_step(int steps) {
    for (int j = 0; j < 28; j++) { GPar *p = &gpar[j]; if (!p->life) continue; for (int n = 0; n < steps && p->life; n++) { p->x += p->vx; p->y += p->vy; p->life--;
        if (p->kind == 1 || p->kind == 3 || p->kind == 4) { if (p->life & 1) p->vy++; } else if (p->kind == 0) p->vx = (s8)((p->life & 16) ? 2 : -2); else if (p->kind == 2) p->vx = (s8)((p->life & 8) ? 1 : -1); } } }
static void gbm(const char *const *r, int h, int x, int y, int s, u16 c) { for (int j = 0; j < h; j++) for (int i = 0; r[j][i]; i++) if (r[j][i] == '#') rect(x + i * s, y + j * s, s, s, c); }
static void gheart(int x, int y, int s, u16 c) { static const char *const h[4] = { ".#.#.", "#####", ".###.", "..#.." }; gbm(h, 4, x, y, s, c); }
static void gstar5(int x, int y, int s, u16 c) { static const char *const h[5] = { "..#..", "..#..", "#####", ".###.", ".#.#." }; gbm(h, 5, x, y, s, c); }
static void gdisc(int cx, int cy, int d, u16 c) { int r = d / 2; for (int y = -r; y <= r; y++) { int hw = 0; while ((hw + 1) * (hw + 1) + y * y <= r * r) hw++; rect(cx - hw, cy + y, 2 * hw + 1, 1, c); } }
static void gpar_draw(void) {   // inside the LCD only
    for (int j = 0; j < 28; j++) { const GPar *p = &gpar[j]; if (!p->life) continue; int x = p->x / 8, y = p->y / 8; if (x < LX || x > LX + 150 || y < LY || y > LY + 84) continue;
        if (p->kind == 0) gheart(x, y, 2, C15(30, 8, 11)); else if (p->kind == 2) { gdisc(x + 3, y + 3, 7 + (p->life & 3), C15(15, 23, 31)); rect(x + 1, y + 1, 2, 2, C15(31, 31, 31)); }
        else if (p->kind == 3) gdisc(x + 3, y + 3, 8, C15(31, 25, 5)); else if (p->kind == 4) rect(x, y, 4, 4, C15(10 + (j * 5) % 20, 10 + (j * 9) % 20, 14 + (j * 3) % 17)); else gstar5(x, y, 2, C15(31, 27, 8)); } }
static void gvoicep(int v, const Blob *b) { static const s8 tr[6] = { 7, 0, 3, 5, 0, -5 }; sfx_p(v, tr[b->stage % 6] + (b->sp % 5) - 2); }   // a pet's voice: higher when young, lower when old, a little by kind
static void gt_open_a(int arg) {   // (D53) 0 the Toy Box, 1 your basket, 2 the Blobdex, 3 the guide
    glrng = 0xB0BE5u + (u32)minutes * 7; ganim = 0; gmsgT = 0; gmsg = 0; gdexpage = 0; gisel = 0; gsub = 0; mg_ui = 4; mg_ver++; gti = 0; gbi = 0; gbx = ctl < GT_MAX ? ctl : 0; gfrom_box = 0; gdexback = -1;
    gdanceT = gactT = 0; gevK = 0; gevCool = 700; gtrainMode = 0; gtutN = -1; memset(gpar, 0, sizeof gpar); gx_check();
    { Toy *g = gtoy_of(ctl); if (g) ggoals_roll(g); }
    if (arg == 3) { gview = GV_GUIDE; gguide = 0; gguideback = -1; return; }
    if (arg == 2) { gview = GV_DEX; gdexback = -1; return; }
    Toy *m = gtoy_of(ctl);
    if (m && !(gach & GACH_TUTOFF) && m->cared >= 10) gach |= GACH_TUTOFF | GACH_INTRO;   // (a toy from before the tutorial: no lessons)
    if (arg == 1 && m && !(gach & GACH_INTRO) && gcount(m) == 1 && m->b[0].on && m->b[0].stage == GS_EGG && m->cared == 0) {   // a toy fresh from the shop: card, unboxing, the egg
        gti = (int)(m - gtoy); gbi = 0; gisub = 0; giT = 0; gview = GV_INTRO; return; }
    if (arg == 1 && m) { gt_open_basket(ctl); gafter = gview; gtitleT = 0; gview = GV_TITLE; return; }
    { int any = 0; for (int k = 0; k < GT_MAX; k++) any |= gtoy[k].own; if (!any) { gview = GV_NONE; return; } }
    gview = GV_BOX; gfrom_box = 1; gafter = GV_BOX; gtitleT = 0; gview = GV_TITLE; }
static void gt_open(void) { gt_open_a(0); }
static int gback = GV_BASKET, gtreeback = GV_BASKET, gfr_t[GT_MAX * GB_N], gfr_s[GT_MAX * GB_N], gfr_n;   // (D55) where Back goes from the awards, home and shop, the tree's Back; the visit list (every hatched pet but this one: this toy's, then the housemates')
static int gcx, gcT, gcY[3], gcX[3], gcK[3], gcScore, gcBad, gcN, ghiNum, ghiGuess, ghiNext, ghd;   // CATCH and HIGH LOW; the pad's held keys
static const char *const groomNm[6] = { "SUNNY ROOM", "SEA ROOM", "BERRY ROOM", "MINT ROOM", "NIGHT ROOM", "GOLD ROOM" };
static const char *const gdecoNm[6] = { "WINDOW", "PLANT", "LAMP", "RUG", "PICTURES", "TOY BOX" };
static void gfr_build(void) {
    gfr_n = 0; for (int pass = 0; pass < 2; pass++) for (int k = 0; k < GT_MAX; k++) { Toy *u = &gtoy[k]; if (!u->own || gsim_of(u) < 0 || (pass == 0) != (k == gti)) continue;
        for (int i = 0; i < GB_N; i++) if (u->b[i].on && u->b[i].stage > GS_EGG && (k != gti || i != gbi)) { gfr_t[gfr_n] = k; gfr_s[gfr_n++] = i; } } }
static void gtree_sel(Toy *t, const Blob *b) { gtree = 0; if (b->on) for (int j = 0; j < GLN; j++) if (t->ln[j].id == b->id) gtree = j; }
static void gstart_catch(void) { gview = GV_CATCH; gcx = 0; gcScore = gcBad = gcN = gcT = 0; for (int i = 0; i < 3; i++) { gcY[i] = -i * 7; gcX[i] = 4 + grr() % 24; gcK[i] = grr() % 5 == 0; } gmT = -40; }
static void gstart_hilo(void) { gview = GV_HILO; gmRound = 0; gmWon = 0; ghiNum = 1 + grr() % 9; ghiGuess = 0; ghiNext = 0; gmT = 0; }
static void ghome_toggle(Toy *t) { if (gsim_of(t) != ctl) { gsay("THAT IS NOT YOUR HOME. YOU CAN ONLY LOOK"); return; } t->deco ^= (u8)(1 << gsub); gsay((t->deco >> gsub) & 1 ? "PUT IT OUT" : "PUT IT AWAY"); }
static void ghome_walls(Toy *t) { if (gsim_of(t) != ctl) { gsay("THAT IS NOT YOUR HOME. YOU CAN ONLY LOOK"); return; } t->room = (u8)((t->room + 1) % 6); gsay("NEW WALLS!"); }
static void gafter_fx(void) { int f = gfxs(); if (f == 1) { gemit(1, 128, 46, 10); voice(V_YAHOO); } else if (f == 2) { gemit(3, 128, 40, 6); voice(V_FIN); } else if (f == 3) { gemit(3, 128, 40, 10); gemit(4, 128, 40, 18); voice(V_YAHOO); } else if (f == 4) { gemit(4, 128, 40, 16); voice(V_YAHOO); } else if (f == 5) gemit(1, 128, 40, 6); }
static void gsay2(const char *m) { static char c[96]; const char *g = gpending(); if (g) { snprintf(c, sizeof c, "%s  %s", m, g); gsay(c); } else gsay(m); }
static int gskill(int i) { return gtt()->x[gbi].sk[i]; }
// ---- the first day, scripted: a card, warm the egg until it hatches, then feed, play, clean and tuck in (each step starts by making the pet need exactly that), a card to finish ----
static int gtut_pg(void) { return (int)((gach & GACH_TUT) >> 16); }
static void gtut_set(int pg) { if (pg >= 7) gach |= GACH_TUTOFF; gach = (gach & ~GACH_TUT) | (u32)pg << 16; gtutN = -1; }
static int gtut_page(void) {   // which step is up (-1: none): 0 the welcome card, 1 warm the egg, 2 feed, 3 play, 4 clean, 5 lights out, 6 the finishing card
    if ((gach & GACH_TUTOFF) || gview != GV_HAND || gsim_of(gtt()) != ctl) return -1; int pg = gtut_pg(); if (pg > 6) return -1;
    if (pg == 0 || pg == 6) return pg; return gcb()->stage == GS_EGG ? 1 : (pg == 1 ? 2 : pg); }
static void gtut_skip(void) { gtut_set(7); gsay("TUTORIAL SKIPPED. HELP IS IN MORE"); }
static void gtut_run(int pg, int pr) {   // the interactive steps
    Toy *t = gtt(); Blob *b = gcb(); Xt *x = &t->x[gbi]; if (pg != gtut_pg()) gtut_set(pg);
    if (gtutN != pg) { gtutN = pg; gtutT = 0; gtutBase = t->plays;
        if (pg == 2) { if (b->hun > 1) b->hun = 1; } else if (pg == 3) b->hap = 1; else if (pg == 4) b->poop = 1; else if (pg == 5) { x->nrg = 5; b->fl |= GF_SLEEP; b->fl &= (u8)~GF_DARK; } }
    if (pr & PD_ST) { gtut_skip(); return; }
    switch (pg) {
    case 1: if (pr & PD_X) { gtutT++; gemit(0, 128, 40, 2); sfx_p(0, 4 + gtutT * 2); if (gtutT >= 5) ghatch(t, b); } break;
    case 2: if (b->hun >= 2) { gemit(1, 128, 40, 6); gtut_set(3); } break;
    case 3: if (t->plays > gtutBase) { gemit(1, 128, 40, 6); gtut_set(4); } break;
    case 4: if (!b->poop) { gemit(2, 128, 40, 6); gtut_set(5); } break;
    case 5: if (b->fl & GF_DARK) { x->nrg = 80; b->fl &= (u8)~(GF_SLEEP | GF_DARK); gemit(1, 128, 40, 8); gtut_set(6); } break; } }
// ---- the games: BEAT, HOP, STACK and WHACK are new; their screens live on the LCD in the PS1's own numbers (PX, PY) ----
#define BN 12
static int gbAt[BN], gbRes[BN], gbTm, gbPerf, gbGood, gbMiss, gbCombo, gbJudge, gbJT;
#define HN 14
static int ghY, ghVy, ghOx[3], ghOt[3], ghSp, ghNext, ghSpawn, ghClear, ghHits, ghDone, ghHurt;   // heights and speeds are in 1/16 pixel
#define SN 9
static int gsL[SN + 1], gsW[SN + 1], gsCx, gsCw, gsDir, gsLv, gsPerf, gsOver;
#define WN 14
static int gwMax, gwN, gwSt, gwT, gwW, gwHole, gwMess, gwLast, gwPts, gwHit, gwMiss, gwRes, gwRT, gwMood, gwMT; static u8 gwH[WN];
static const u8 gwcx[4] = { 192, 192, 160, 224 }, gwcy[4] = { 38, 88, 63, 63 };
static void gstart_beat(void) { gview = GV_BEAT; gbTm = 0; gbPerf = gbGood = gbMiss = gbCombo = gbJT = 0; int a = 100; for (int i = 0; i < BN; i++) { gbAt[i] = a; gbRes[i] = 0; a += 34 + (grr() % 3) * 11; } gmT = -60; }
static void gstart_hop(void) { gview = GV_HOP; ghY = ghVy = 0; ghSp = 36; ghSpawn = 70; ghClear = ghHits = ghDone = ghHurt = ghNext = 0; for (int i = 0; i < 3; i++) ghOx[i] = -99 * 16; gmT = -60; }
static void gstart_stack(void) { gview = GV_STACK; gsL[0] = 104; gsW[0] = 52; gsLv = 1; gsCw = 52; gsCx = 84 * 16; gsDir = 1; gsPerf = gsOver = 0; gmT = -50; }
static void gstart_whack(void) { gview = GV_WHACK; gwMax = gwN = gwSt = gwRes = gwRT = gwMT = gwPts = gwHit = gwMiss = gwMess = gwHole = 0; gwT = 40; gwW = 60; gwLast = -1; memset(gwH, 0, sizeof gwH); gmT = -50; }
static void gwhack_end(int res, int pts) {   // a pop is over: 1 quick, 2 slow, 3 too late, 4 hit the mess, 5 left the mess alone, 6 wrong hole
    gwH[gwN++] = (u8)res; gwPts += pts; gwRes = res; gwRT = 40; gwSt = 0; gwT = gwN >= WN ? 50 : 18 + grr() % 22; gwMT = 26; gwMood = (res == 1 || res == 2 || res == 5) ? MD_HAPPY : MD_BORED;
    if (res <= 2) gwHit++; else if (res != 5) gwMiss++; }
static void gfinish_game(int pct, int g) {
    Toy *t = gtt(); Blob *b = gcb(); gview = GV_HAND; gsay2(gresult(t, b, g, pct, gtrainMode)); int gr_ = ggrade;
    if (gr_ >= 3) { gemit(1, 128, 46, 8); gvoicep(V_YAHOO, b); gdanceT = 150; } else if (gr_ >= 2) { gemit(0, 128, 46, 4); gvoicep(V_YEHA, b); gdanceT = 90; } else gvoicep(gr_ ? V_AGREE : V_NAH, b);
    if (gr_ == 4) gemit(4, 128, 40, 14); gafter_fx(); gtrainMode = 0; }
static int gblocked(const Blob *b, int sleepOk) {
    if (b->stage == GS_EGG) { gsay("IT IS STILL AN EGG. GIVE IT A MINUTE"); return 1; }
    if (!sleepOk && (b->fl & GF_SLEEP)) { gsay("SHHH. IT IS ASLEEP"); return 1; }
    return 0; }
static void gfxact(int kind, int vc) { gactT = 55; gactK = kind; if (vc >= 0) gvoicep(vc, gcb()); gemit(kind == 2 ? 2 : kind == 3 ? 0 : 1, 128, 50, 4); gafter_fx(); }
static void gstart_game(int g) {
    const char *why = gplay_check(gtt(), gbi, gtrainMode); if (why) { gsay(why); gvoicep(V_NAH, gcb()); gtrainMode = 0; return; }
    gsub = g; glastGame = g;
    if (g == 0) { gview = GV_LOOK; gmRound = 0; gmWon = 0; gmT = 0; gmN = 0; gmDir = 0; }
    else if (g == 1) { gview = GV_COPY; gmRound = 0; gmWon = 0; gmN = 2; for (int i = 0; i < 6; i++) gseq[i] = (u8)(grr() & 3); gsidx = 0; gmT = -60; }
    else if (g == 2) gstart_catch(); else if (g == 3) gstart_hilo(); else if (g == 4) gstart_beat(); else if (g == 5) gstart_hop(); else if (g == 6) gstart_stack(); else gstart_whack(); }
static void ga_feed(void) { Toy *t = gtt(); int c0 = t->cared; if (gblocked(gcb(), 0)) return; gsay2(gquick_feed(t, gcb())); if (t->cared != c0) gfxact(1, V_YAHOO); else gvoicep(V_NAH, gcb()); }
static void ga_clean(void) { Toy *t = gtt(); Blob *b = gcb(); int c0 = t->cared; if (gblocked(b, 1)) return; gsay2(b->poop || !(b->fl & GF_SICK) ? gclean(t, b) : gmedicine(t, b)); if (t->cared != c0) gfxact(2, V_FIN); }
static void ga_bath(void) { Toy *t = gtt(); int c0 = t->cared; if (gblocked(gcb(), 0)) return; gsay2(gbath(t, gcb())); if (t->cared != c0) { gfxact(2, V_LAUGH); gemit(2, 120, 56, 8); } else gvoicep(V_AGREE, gcb()); }
static void ga_light(void) { Blob *b = gcb(); gsay2(glight(gtt(), b)); gafter_fx(); if (b->fl & GF_DARK) sfx_p(2, 0); else gvoicep(V_AGREE, b); }
static void ga_play(void) { if (gblocked(gcb(), 0)) return; gtrainMode = 0; gstart_game((glastGame + 1) % NGAME); }
static void ga_train(int sk) { static const u8 gm[3][3] = { { 5, 2, 7 }, { 1, 6, 1 }, { 4, 0, 4 } }; if (gblocked(gcb(), 0)) return; gtrainMode = 1; gstart_game(gm[sk][gtrAlt++ % 3]); }   // a training round: the game that teaches that skill (they take turns)
static void ga_pet(void) { Toy *t = gtt(); int c0 = t->cared; gsay2(gnuzzle(t, gcb())); if (t->cared != c0) gfxact(3, V_YEHA); else gvoicep(V_AGREE, gcb()); }
static void ga_smart(void) {   // SELECT: whatever it needs most
    Blob *b = gcb(); Toy *t = gtt(); if (gblocked(b, 1)) return;
    switch (gurgent(t, gbi)) {
    case 1: { int c0 = t->cared; gsay2(gmedicine(t, b)); if (t->cared != c0) gfxact(3, V_FIN); break; }
    case 2: ga_clean(); break;
    case 0: ga_pet(); break;
    case 4: gsay2(gscold(t, b)); gvoicep(V_NAH, b); break;
    case 5: ga_feed(); break;
    case 6: case 9: ga_light(); break;
    case 8: ga_bath(); break;
    default: ga_play(); break; } }
static const u8 giconMap[GICN] = { 0, 1, 4, 12, 2, 3, 6, 13, 5, 14, 7, 8, 9, 11 };   // the order on the toy: daily care in the top row (FEED PLAY CLEAN BATH LIGHT MEDS PET), then TRAIN SCOLD TODAY STATS TREE VISIT MORE
static void gmore_go(int k) {   // the More page: 0 family home, 1 diary, 2 awards, 3 blobdex, 4 dress, 5 help, 6 shop, 7 toy box
    gback = GV_MORE; switch (k) { case 0: gsub = 0; gview = GV_HOME; break; case 1: gsub = 0; gview = GV_DIARY; break; case 2: gsub = 0; gview = GV_AWARD; break;
        case 3: gdexpage = 0; gview = GV_DEX; gdexback = GV_MORE; break; case 4: gsub = gcb()->acc; gview = GV_DRESS; break; case 5: gview = GV_GUIDE; gguide = 0; gguideback = GV_MORE; break;
        case 6: gsub = 0; gview = GV_SHOP; break; default: gfrom_box = 1; gbx = ctl < GT_MAX ? ctl : 0; gview = GV_BOX; } }
static void guse_icon(void) {   // the fourteen icons
    Blob *b = gcb(); Toy *t = gtt(); int sl = (b->fl & GF_SLEEP) != 0, a = giconMap[gisel % GICN];
    if (b->stage == GS_EGG && a != 7 && a != 8 && a != 11 && a != 14) { gsay("IT IS STILL AN EGG. GIVE IT A MINUTE"); return; }
    if (sl && a != 2 && a != 7 && a != 3 && a != 8 && a != 11 && a != 10 && a != 14) { gsay("SHHH. IT IS ASLEEP"); return; }
    switch (a) {
    case 0: gsub = 0; gview = GV_FEED; break;
    case 1: gsub = glastGame; gview = GV_PLAYPICK; break;
    case 2: ga_light(); break;
    case 3: { int c0 = t->cared; gsay2(gmedicine(t, b)); if (t->cared != c0) gfxact(3, V_FIN); else gvoicep(V_NAH, b); break; }
    case 4: { int c0 = t->cared; gsay2(gclean(t, b)); if (t->cared != c0) gfxact(2, V_FIN); break; }
    case 5: { const char *m = gscold(t, b); gsay2(m); gvoicep(m[0] == 'I' && m[1] == 'T' && m[3] == 'K' ? V_AGREE : V_CRY, b); break; }
    case 6: ga_pet(); break;
    case 7: gsub = 0; gview = GV_STATS; break;
    case 8: gtree_sel(t, b); gtreeback = GV_HAND; gview = GV_TREE; break;
    case 9: gfr_build(); if (!gfr_n) { gsay("YOU NEED ANOTHER PET, HATCHED, TO PLAY WITH"); return; } gsub = 0; gview = GV_FRIEND; break;
    case 12: ga_bath(); break;
    case 13: gsub = 0; gview = GV_TRAIN; break;
    case 14: ggoals_roll(t); gview = GV_GOALS; break;
    default: gsub = 0; gview = GV_MORE; break; } }
static void gt_next(void) {   // after the title or a scene: a hatching waiting for you, then yesterday's summary, then where you were going
    Toy *t = gtoy_of(ctl);
    if (ghpend && t) { gti = (int)(t - gtoy); gbi = ghpend - 1; ghpend = 0; ghT = 0; if (gafter == GV_BOX || gafter == GV_TITLE) gafter = GV_BASKET; gview = GV_HATCH; return; }
    if (gsum_pend) { gsum_pend = 0; gview = GV_DAY; return; }
    gview = gafter; }
static void gintro_go(void) { gview = GV_HAND; gisel = 0; gmsgT = 0; gmsg = 0; gafter = GV_HAND; }
static int gt_update(int pr, int steps) {   // 1 while it runs (gotchi_update)
    gt_live(steps); ganim += steps; gpar_step(steps); if (gactT > 0) gactT -= steps; if (gdanceT > 0) gdanceT -= steps; if (gmsgT > 0) { gmsgT -= steps; if (gmsgT <= 0) { gmsgT = 0; gmsg = 0; } }
    if (ghpend && (gview == GV_HAND || gview == GV_BASKET) && gtoy_of(ctl) && gti == (int)(gtoy_of(ctl) - gtoy)) { gafter = gview; gt_next(); }   // it hatched while you look
#ifdef AUTO_GAME   /* test hook: the first time the handheld shows a hatched pet, play game AUTO_GAME (a training round when AUTO_TRAIN) */
    { static int once; if (!once && gview == GV_HAND && gcb()->stage > GS_EGG) { once = 1; gtrainMode = AUTO_TRAIN; gcb()->hap = 3; gtt()->x[gbi].nrg = 90; gstart_game(AUTO_GAME); } }
#endif
    switch (gview) {
    case GV_TITLE: gtitleT += steps; if (pr & PD_O) return 0; if ((pr & (PD_X | PD_ST)) || gtitleT > 160) gt_next(); break;
    case GV_INTRO:   // 0 the shop card, 1 the box, 2 the egg
        giT += steps; if (gisub == 1 && giT >= 40 && giT - steps < 40) voice(V_YAHOO); if (gisub == 1 && giT >= 75 && giT - steps < 75) voice(V_WIN);
        if (pr & (PD_ST | PD_O)) { gach |= GACH_INTRO; gintro_go(); break; }
        if (pr & PD_X) { if (gisub == 0) { gisub = 1; giT = 0; } else if (gisub == 1) { if (giT < 110) giT = 110; else { gisub = 2; giT = 0; } } else { gach |= GACH_INTRO; gintro_go(); } }
        if (gisub == 1 && giT > 220) { gisub = 2; giT = 0; }
        break;
    case GV_HATCH: { int o = ghT; ghT += steps; static const u8 cr[5] = { 12, 30, 48, 66, 78 };
        for (int i = 0; i < 5; i++) if (o < cr[i] && ghT >= cr[i]) sfx(2);   // the shell creaks
        if (o < 88 && ghT >= 88) sfx(5); if (o < 112 && ghT >= 112) sfx(1);   // the crack, then the hello
        if ((pr & PD_ST) || ((pr & PD_X) && ghT > 130)) gt_next(); break; }
    case GV_DAY: if (pr & (PD_X | PD_O | PD_ST)) gt_next(); break;
    case GV_BOX: { int n = MIN(nsim, GT_MAX); if (n < 1) return 0;
        if (pr & (PD_U | PD_L)) gbx = (gbx + n - 1) % n; if (pr & (PD_D | PD_R)) gbx = (gbx + 1) % n; if (gbx >= n) gbx = 0;
        if (pr & PD_SQ) { gview = GV_DEX; gdexpage = 0; gdexback = GV_BOX; }
        if (pr & PD_SEL) { gview = GV_GUIDE; gguide = 0; gguideback = GV_BOX; }
        if (pr & PD_R1) { gview = GV_AWARD; gsub = 0; gback = GV_BOX; }
        if (pr & PD_ST) { gview = GV_SHOP; gsub = 0; gback = GV_BOX; }
        if (pr & PD_O) { if (gtoy_of(ctl) && !gfrom_box) { gview = GV_HAND; break; } if (gfrom_box && gtoy_of(ctl) && gafter == GV_HAND) { gview = GV_HAND; break; } return 0; }
        if (pr & PD_X) { if (!gtoy_of(gbx) && gbx == ctl) { if (funds < GOTCHI_PRICE) { gsay("A TOY COSTS $5000"); voice(V_NAH); } else if (gbuy(ctl)) { funds -= GOTCHI_PRICE; say("A Boreagotchi and its first egg!"); gt_open_a(1); voice(V_YAHOO); } }
                          else { gt_open_basket(gbx); if (gview == GV_BASKET) gfrom_box = 1; } }
        if (pr & PD_L1) o_gtime = (o_gtime + 1) % 3;   // the clock: the game's, LIVE, off
        break; }
    case GV_GUIDE:
        if (pr & (PD_R1 | PD_R | PD_X)) gguide = (gguide + 1) % GNP; if (pr & (PD_L1 | PD_L)) gguide = (gguide + GNP - 1) % GNP;
        if (pr & PD_O) { if (gguideback < 0) return 0; gview = gguideback; }
        break;
    case GV_NONE: if (pr & PD_O) return 0; if (pr & PD_X) { if (funds < GOTCHI_PRICE) { gsay("A TOY COSTS $5000"); voice(V_NAH); } else if (gbuy(ctl)) { funds -= GOTCHI_PRICE; say("A Boreagotchi and its first egg!"); gt_open_a(1); voice(V_YAHOO); } } break;
    case GV_BASKET: { Toy *t = gtt();
        if (pr & PD_L) gbi = (gbi + GB_N - 1) % GB_N; if (pr & PD_R) gbi = (gbi + 1) % GB_N; if (pr & PD_U) gbi = (gbi + GB_N - 4) % GB_N; if (pr & PD_D) gbi = (gbi + 4) % GB_N;
        t->sel = (u8)gbi;
        if (pr & PD_SQ) { gview = GV_DEX; gdexpage = 0; gdexback = GV_BASKET; }
        if (pr & PD_SEL) { gview = GV_GUIDE; gguide = 0; gguideback = GV_BASKET; }
        if (pr & PD_R1) { gtree_sel(t, &t->b[gbi]); gtreeback = GV_BASKET; gview = GV_TREE; }
        if (pr & PD_L1) { gsub = 0; gback = GV_BASKET; gview = GV_HOME; }
        if (pr & PD_O) { if (gfrom_box) gview = GV_BOX; else return 0; }
        if (pr & PD_X) { if (gsim_of(t) != ctl) { gsay("THAT IS NOT YOUR TOY. YOU CAN ONLY LOOK"); break; } if (!t->b[gbi].on) { if (funds < GOTCHI_EGG) { gsay("A NEW EGG COSTS $150"); voice(V_NAH); } else { funds -= GOTCHI_EGG; gegg(t, gbi); gsay("A NEW EGG! KEEP IT CLOSE"); voice(V_YAHOO); } } else { gview = GV_HAND; gisel = 0; gmsgT = 0; gmsg = 0; } }
        break; }
    case GV_HAND: { Toy *t = gtt(); int cp = gtut_page(); Blob *bb = gcb();
        if (cp == 0 || cp == 6) {   // the cards: A goes on, B or START skips the rest
            if (pr & PD_X) { if (cp == 6) { funds += 100; gsay("WELL DONE! +$100 FIRST DAY BONUS"); gemit(3, 128, 40, 12); gemit(4, 128, 40, 14); voice(V_YAHOO); } else voice(V_AGREE); gtut_set(cp + 1); gtutN = -1; }
            else if (pr & (PD_O | PD_ST)) gtut_skip();
            break; }
        if (cp >= 1) { gtut_run(cp, pr); if (cp == 1) pr &= ~(PD_X | PD_ICON); cp = gtut_page(); if (cp == 0 || cp == 6) break; }
        if (gevK && (pr & PD_X)) {   // a surprise is up: A reacts (the fly must be in the middle)
            int ok = 1; if (gevK == 1) { int fx_ = 84 + ABS(((300 - gevT) * 2) % 300 - 150); ok = ABS(fx_ - 160) < 30; }
            gsay2(gev_do(t, gbi, gevK, ok)); gafter_fx(); if (ok) { gemit(gevK == 2 || gevK == 5 ? 3 : gevK == 4 ? 0 : 1, 128, 46, 6); gvoicep(V_YAHOO, bb); gdanceT = 60; } else gvoicep(V_NAH, bb); gevK = 0; gevCool = 1500 + grr() * 6; pr &= ~PD_X; }
        if (pr & PD_R) gisel = (gisel + 1) % GICN; if (pr & PD_L) gisel = (gisel + GICN - 1) % GICN;
        if (pr & (PD_U | PD_D)) gisel = (gisel + 7) % GICN;
        if (pr & PD_R1) { gbi = gliving(t, gbi, 1); gevK = 0; } if (pr & PD_L1) { gbi = gliving(t, gbi, -1); gevK = 0; }
        t->sel = (u8)gbi;
        if (pr & PD_O) { gview = GV_BASKET; gmsgT = 0; gmsg = 0; }
        if (pr & (PD_X | PD_ICON)) guse_icon();
        else if (pr & PD_TR) ga_feed();          // one press: the common things, with sensible choices
        else if (pr & PD_SQ) ga_play();
        else if (pr & PD_SEL) ga_smart();
        if (gview == GV_HAND) { static int lm; int m = gmood_of(gcb(), &gtt()->x[gbi]); if (m != lm) { if (m == MD_HUNGRY) gvoicep(V_NAH, gcb()); else if (m == MD_SICK) gvoicep(V_CRY, gcb()); lm = m; }
            { const Xt *x = &t->x[gbi]; int w = x->wish & 7; if (w && w != gwishSeen) gemit(1, 100, 40, 2); gwishSeen = w; }
            if (gcb()->stage > GS_EGG && cp < 0) {   // surprises: a fly, a coin, a star, thunder, a visitor
                if (gevK) { gevT -= steps; if (gevT <= 0) { gsay2(gev_do(t, gbi, gevK, 0)); gevK = 0; gevCool = 1500 + grr() * 6; gvoicep(V_NAH, bb); } }
                else if (!gmsgT && !(gcb()->fl & GF_SLEEP)) { gevCool -= steps; if (gevCool <= 0) { gevK = gev_roll(t, gbi); if (gevK) { gevT = 300; if (gevK == 4) gvoicep(V_CRY, bb); if (gevK == 5) gevHit = grr(); } else gevCool = 900; } } } }
        break; }
    case GV_FEED:
        if (pr & PD_R) gsub = (gsub + 1) % 13; if (pr & PD_L) gsub = (gsub + 12) % 13; if (pr & PD_D) gsub = (gsub + 4) % 13; if (pr & PD_U) gsub = (gsub + 9) % 13; if (pr & PD_L1) gsub = 12;
        if (pr & PD_O) { gview = GV_HAND; return 1; }
        if (pr & PD_X) { Toy *t = gtt(); int c0 = t->cared; const char *m = gsub == 12 ? glunch(t, gcb()) : gfeed(t, gcb(), gsub, 1); gsay2(m); gview = GV_HAND; gvoicep(m[0] == 'Y' && m[1] == 'U' ? V_NAH : V_YAHOO, gcb()); if (t->cared != c0) gfxact(1, -1); gafter_fx(); }
        break;
    case GV_PLAYPICK:
        if (pr & PD_R) gsub = (gsub + 1) % NGAME; if (pr & PD_L) gsub = (gsub + NGAME - 1) % NGAME; if (pr & PD_U) gsub = (gsub + NGAME - 2) % NGAME; if (pr & PD_D) gsub = (gsub + 2) % NGAME;
        if (pr & PD_O) gview = GV_HAND;
        if (pr & PD_X) { gtrainMode = 0; gstart_game(gsub); }
        break;
    case GV_TRAIN:   // pick a skill: each is trained by its own games
        if (pr & (PD_R | PD_D)) gsub = (gsub + 1) % 3; if (pr & (PD_L | PD_U)) gsub = (gsub + 2) % 3;
        if (pr & PD_O) gview = GV_HAND;
        if (pr & PD_X) ga_train(gsub);
        break;
    case GV_GOALS: if (pr & (PD_O | PD_X)) gview = GV_HAND; break;
    case GV_BEAT: {   // BEAT: press when a note is on the ring. perfect 2, good 1, twelve notes
        if (pr & PD_O) { gview = GV_HAND; gtrainMode = 0; break; }
        gmT += steps; if (gmT < 0) break;
        int win = 4 + gskill(2) / 50, win2 = 9 + gskill(2) / 40;
        for (int n = 0; n < steps; n++) { gbTm++; for (int i = 0; i < BN; i++) if (!gbRes[i] && gbTm > gbAt[i] + win2) { gbRes[i] = 3; gbMiss++; gbCombo = 0; gbJudge = 3; gbJT = 40; } }
        if (pr & (PD_X | PD_SQ | PD_TR)) {
            int hit_ = -1, bd = 99; for (int i = 0; i < BN; i++) if (!gbRes[i]) { int d = ABS(gbAt[i] - gbTm); if (d < bd) { bd = d; hit_ = i; } }
            if (hit_ >= 0 && bd <= win2 + 3) { if (bd <= win) { gbRes[hit_] = 1; gbPerf++; gbJudge = 1; gbCombo++; } else { gbRes[hit_] = 2; gbGood++; gbJudge = 2; gbCombo++; } gbJT = 40; gbT = 20; gemit(gbJudge == 1 ? 1 : 0, PX(100), PY(76), 1); sfx_p(gbJudge == 1 ? V_YEHA : V_AGREE, MIN(gbCombo, 10)); }
            else { gbJudge = 4; gbJT = 30; gbCombo = 0; sfx_p(V_NAH, 4); } }
        if (gbJT > 0) gbJT -= steps; if (gbT > 0) gbT -= steps;
        if (gbTm > gbAt[BN - 1] + win2 + 20) gfinish_game((gbPerf * 2 + gbGood) * 100 / (BN * 2), 4);
        break; }
    case GV_HOP: {   // HOP: A to jump the rocks. fourteen of them, three lives
        if (pr & PD_O) { gview = GV_HAND; gtrainMode = 0; break; }
        gmT += steps; if (gmT < 0) break;
        int g16 = 5 - gskill(0) / 40;   // gravity in 1/16: the faster pet floats a little longer
        for (int n = 0; n < steps; n++) {
            if (ghY == 0 && ghVy == 0 && (pr & (PD_X | PD_SQ | PD_TR))) { ghVy = 83; pr = 0; sfx_p(V_YAHOO, 8); }
            if (ghY > 0 || ghVy > 0) { ghY += ghVy; ghVy -= g16; if (ghY <= 0) { ghY = 0; ghVy = 0; } }
            if (ghHurt > 0) ghHurt--;
            ghSpawn--; if (ghSpawn <= 0 && ghNext < HN) { for (int i = 0; i < 3; i++) if (ghOx[i] < -90) { ghOx[i] = 250 * 16; ghOt[i] = grr() % 3; ghNext++; break; } ghSpawn = 62 + grr() % 40 - ghClear * 2; if (ghSpawn < 44) ghSpawn = 44; }
            ghSp = 34 + MIN(ghClear, 12) * 2;
            for (int i = 0; i < 3; i++) if (ghOx[i] > -90) { ghOx[i] -= ghSp; int ow = ghOt[i] == 2 ? 22 : ghOt[i] == 1 ? 10 : 12, oh = ghOt[i] == 1 ? 24 : 10;
                int px = 116 * 16 - 10 * 16, px2 = 116 * 16 + 10 * 16;   // the pet's body: 20 px wide
                if (ghOx[i] / 16 < 20) { ghOx[i] = -99 * 16; ghClear++; ghDone++; continue; }
                if (ghOx[i] < px2 && ghOx[i] + ow * 16 > px && ghY / 16 < oh && ghHurt == 0) { ghHits++; ghHurt = 50; ghDone++; ghOx[i] = -99 * 16; gbT = 30; sfx_p(V_CRY, 3); gemit(2, PX(120), PY(80), 2); } }
        }
        if (ghHits >= 3 || (ghNext >= HN && ghDone >= HN)) gfinish_game(ghClear * 100 / HN, 5);
        break; }
    case GV_STACK: {   // STACK: A drops the sliding block. nine floors; a perfect drop keeps the width
        if (pr & PD_O) { gview = GV_HAND; gtrainMode = 0; break; }
        gmT += steps; if (gmT < 0) break;
        if (gsOver) { gsOver += steps; if (gsOver > 70) gfinish_game(MIN(100, ((gsLv - 1) * 100 + gsPerf * 12) / SN), 6); break; }
        int sp16 = 26 + gsLv * 5 - gskill(1) / 10;   // 1/16 px a frame: it speeds up each floor; brains slow it a little
        for (int n = 0; n < steps; n++) { gsCx += gsDir * sp16; if (gsCx > 168 * 16 - gsCw * 16) { gsCx = 168 * 16 - gsCw * 16; gsDir = -1; } if (gsCx < 84 * 16) { gsCx = 84 * 16; gsDir = 1; } }
        if (pr & (PD_X | PD_SQ | PD_TR)) {
            int cl = gsCx / 16, pl = gsL[gsLv - 1], pw = gsW[gsLv - 1], lo = MAX(cl, pl), hi = MIN(cl + gsCw, pl + pw), ov = hi - lo;
            if (ABS(cl - pl) <= 3) { cl = pl; ov = MIN(gsCw, pw); lo = pl; gsPerf++; gemit(1, PX(cl + ov / 2), PY(100 - gsLv * 8), 3); sfx_p(V_WIN, gsLv); } else if (ov > 0) sfx_p(V_AGREE, gsLv);
            if (ov <= 0) { gsOver = 1; gsW[gsLv] = 0; sfx_p(V_NAH, 3); gbT = 40; }
            else { gsL[gsLv] = lo; gsW[gsLv] = ov; gsLv++; gsCw = ov; gsCx = (gsDir > 0 ? 84 : 168 - ov) * 16; if (gsLv > SN) { gsOver = 1; gsLv = SN + 1; gemit(4, PX(130), PY(60), 14); } }
        }
        break; }
    case GV_WHACK: {   // WHACK: D-pad toward the bug. fourteen pops; the window shrinks; from the fourth pop some are mess, which must be left alone
        if (pr & PD_O) { gview = GV_HAND; gtrainMode = 0; break; }
        gmT += steps; if (gmT < 0) break;
        int key = (pr & PD_U) ? 0 : (pr & PD_D) ? 1 : (pr & PD_L) ? 2 : (pr & PD_R) ? 3 : -1;
#ifdef AUTO_WHACK   /* test hook: a player who reacts after AUTO_WHACK frames, now and then picks the wrong hole or whacks the mess */
        if (gwSt == 1 && gwW - gwT >= AUTO_WHACK) key = (gwMess && !(gwN & 1)) ? -1 : (gwN == 5 ? (gwHole + 1) & 3 : gwHole);
#endif
        if (gwRT > 0) gwRT -= steps; if (gwMT > 0) gwMT -= steps;
        if (gwSt == 1) {
            if (key >= 0) {
                if (!gwMess && key == gwHole) { int q = gwT * 5 > gwW * 2; gwhack_end(q ? 1 : 2, q ? 2 : 1); gemit(q ? 1 : 0, PX(gwcx[key]), PY(gwcy[key]) - 10, 3); sfx_p(q ? V_YEHA : V_AGREE, 4 + gwN); }
                else { gwhack_end(gwMess ? 4 : 6, 0); gemit(2, PX(gwcx[key]), PY(gwcy[key]) - 8, 2); sfx_p(V_GROAN, 0); } }
            else { gwT -= steps; if (gwT <= 0) { if (gwMess) { gwhack_end(5, 1); sfx_p(V_AGREE, 9); } else { gwhack_end(3, 0); sfx_p(V_NAH, 0); } } } }
        else {
            if (key >= 0 && gwN < WN && gwRT < 34) { gwRes = 7; gwRT = 30; gwPts = MAX(0, gwPts - 1); gwMood = MD_BORED; gwMT = 20; sfx_p(V_NAH, 5); }
            gwT -= steps;
            if (gwT <= 0) { if (gwN >= WN) gfinish_game(gwMax ? gwPts * 100 / gwMax : 0, 7);
                else { gwHole = (gwLast + 1 + grr() % 3) & 3; gwLast = gwHole; gwMess = gwN >= 3 && grr() % 4 == 0; gwMax += gwMess ? 1 : 2; gwW = MAX(36, 64 - gwN * 2) + gskill(0) / 8; gwT = gwW; gwSt = 1; sfx_p(gwMess ? V_GROAN : V_AGREE, gwMess ? -6 : 11 + gwHole * 3); } } }
        break; }
    case GV_LOOK:   // WHICH WAY? it looks left or right: guess first. 5 rounds, 3 to win
        if (gmN == 0) { if (pr & PD_L) { gmDir = -1; gmN = 1; gmT = 0; } if (pr & PD_R) { gmDir = 1; gmN = 1; gmT = 0; } if (pr & PD_O) gview = GV_HAND; if (gmN) gsidx = (grr() & 1) ? 1 : 0; }
        else { gmT += steps; if (gmT > 50) { int way = gsidx ? 1 : -1; if (way == gmDir) { gmWon++; voice(V_YEHA); } gmRound++; gmN = 0; if (gmRound >= 5) gfinish_game(gmWon * 20, 0); } }
        break;
    case GV_COPY: {   // COPY ME: watch the blob, then do the same moves (the D-pad). Three rounds, two to win
        if (pr & PD_O) { gview = GV_HAND; gtrainMode = 0; break; }
        gmT += steps; if (gmT < 0) break; if (gmT < gmN * 40) break;
        int key = (pr & PD_U) ? 0 : (pr & PD_D) ? 1 : (pr & PD_L) ? 2 : (pr & PD_R) ? 3 : -1;
        if (key >= 0) { if (key == gseq[gsidx]) { gsidx++; voice(V_YEHA); if (gsidx >= gmN) { gmWon++; gmRound++; gsidx = 0; gmN++; gmT = -50; if (gmRound >= 3) gfinish_game(100, 1); } }
                        else { gmRound++; gsidx = 0; gmT = -50; voice(V_NAH); if (gmRound - gmWon >= 2 || gmRound >= 3) gfinish_game(gmWon * 33 + 5, 1); } }
        break; }
    case GV_CATCH: {   // CATCH: slide under the falling treats, dodge the mess. 14 treats, 12 is perfect
        if (pr & PD_O) { gview = GV_HAND; gtrainMode = 0; break; }
        gmT += steps; if (gmT < 0) break;
        if (ghd & PD_L) gcx = MAX(-12, gcx - steps); if (ghd & PD_R) gcx = MIN(12, gcx + steps);
        gcT += steps; while (gcT >= 7) { gcT -= 7;
            for (int i = 0; i < 3; i++) { gcY[i]++; if (gcY[i] == 12) { int dx = gcX[i] - (16 + gcx), cw = 4 + gskill(0) / 30; if (dx >= -cw && dx <= cw) { if (gcK[i]) { gcBad++; voice(V_NAH); } else { gcScore++; voice(V_YEHA); } gcY[i] = 99; } }
                if (gcY[i] >= 15) { gcN++; gcY[i] = -(grr() % 6); gcX[i] = 4 + grr() % 24; gcK[i] = grr() % 5 == 0; } } }
        if (gcN >= 14) gfinish_game(MAX(0, gcScore * 100 / 12 - gcBad * 10), 2);
        break; }
    case GV_HILO:
        if (pr & PD_O) { gview = GV_HAND; break; }
        if (ghiNext) { gmT += steps; if (gmT > 55) { ghiNext = 0; ghiNum = 1 + grr() % 9; gmRound++; if (gmRound >= 5) gfinish_game(gmWon * 20, 3); } }
        else if (pr & (PD_U | PD_D)) { int nx = 1 + grr() % 9; if (nx == ghiNum) nx = nx % 9 + 1; ghiGuess = (pr & PD_U) ? 1 : -1; ghiNext = nx; gmT = 0; if ((nx > ghiNum) == (ghiGuess > 0)) { gmWon++; voice(V_YEHA); } else voice(V_NAH); }
        break;
    case GV_FRIEND: { gfr_build(); if (!gfr_n) { gview = GV_HAND; break; } if (gsub >= gfr_n) gsub = 0;
        if (pr & (PD_R | PD_D)) gsub = (gsub + 1) % gfr_n; if (pr & (PD_L | PD_U)) gsub = (gsub + gfr_n - 1) % gfr_n;
        if (pr & PD_O) gview = GV_HAND;
        if (pr & (PD_X | PD_SQ)) { const char *m = gmeet(gtt(), gbi, &gtoy[gfr_t[gsub]], gfr_s[gsub], (pr & PD_SQ) != 0); gview = GV_HAND; gsay2(m); gafter_fx(); gvoicep(V_LAUGH, gcb()); gdanceT = 100; gemit(0, 128, 46, 5); }
        break; }
    case GV_STATS:
        if (pr & (PD_X | PD_R | PD_D)) { if (++gsub >= 5) gview = GV_HAND; }
        if (pr & (PD_L | PD_U)) { if (gsub > 0) gsub--; }
        if (pr & PD_O) gview = GV_HAND;
        break;
    case GV_TREE: { Toy *t = gtt(); int d = (pr & (PD_R | PD_R1)) ? 1 : (pr & (PD_L | PD_L1)) ? -1 : 0;
        if (d) for (int k = 1; k <= GLN; k++) { int j = (gtree + d * k + GLN * 2) % GLN; if (t->ln[j].id) { gtree = j; break; } }
        if (pr & PD_O) gview = gtreeback;
        break; }
    case GV_DEX:
        if (pr & (PD_R1 | PD_R)) gdexpage = (gdexpage + 1) & 3; if (pr & (PD_L1 | PD_L)) gdexpage = (gdexpage + 3) & 3;
        if (pr & PD_O) { if (gdexback < 0) return 0; gview = gdexback; }
        break;
    case GV_DRESS:
        if (pr & PD_R) gsub = (gsub + 1) % NACC; if (pr & PD_L) gsub = (gsub + NACC - 1) % NACC;
        if (pr & PD_O) gview = GV_HAND;
        if (pr & PD_X) { const char *m = gdress(gtt(), gcb(), gsub); gsay(m); if (m[0] == 'L') { voice(V_YAHOO); gdanceT = 80; } else if (m[0] == 'N') voice(V_NAH); }
        break;
    case GV_MORE:
        if (pr & PD_R) gsub = (gsub + 1) % 8; if (pr & PD_L) gsub = (gsub + 7) % 8; if (pr & PD_D) gsub = (gsub + 2) % 8; if (pr & PD_U) gsub = (gsub + 6) % 8;
        if (pr & PD_O) gview = GV_HAND;
        if (pr & PD_X) gmore_go(gsub);
        break;
    case GV_HOME: { Toy *t = gtt();
        if (pr & PD_R) gsub = (gsub + 1) % 6; if (pr & PD_L) gsub = (gsub + 5) % 6; if (pr & (PD_U | PD_D)) gsub = (gsub + 3) % 6;
        if (pr & PD_X) ghome_toggle(t); if (pr & PD_SQ) ghome_walls(t);
        if (pr & PD_O) gview = gback;
        break; }
    case GV_DIARY:
        if (pr & (PD_R | PD_D)) gsub = (gsub + 1) % 3; if (pr & (PD_L | PD_U)) gsub = (gsub + 2) % 3;
        if (pr & PD_O) gview = gback;
        break;
    case GV_AWARD:
        if (pr & PD_R) gsub = (gsub + 1) % NACH; if (pr & PD_L) gsub = (gsub + NACH - 1) % NACH; if (pr & PD_D) gsub = (gsub + 2) % NACH; if (pr & PD_U) gsub = (gsub + NACH - 2) % NACH;
        if (pr & PD_O) gview = gback;
        break;
    case GV_SHOP: { Toy *t = gtoy_of(ctl);
        if (pr & PD_R) gsub = (gsub + 1) % 8; if (pr & PD_L) gsub = (gsub + 7) % 8; if (pr & (PD_U | PD_D)) gsub = (gsub + 4) % 8;
        if (pr & PD_O) { gview = gback; break; }
        if (pr & PD_X) { if (!t) { gsay("YOU NEED A BOREAGOTCHI FIRST"); break; }
            if (gsub < 7) { int a = gsub + 1; if (t->accs & (1 << a)) gsay("YOU OWN THAT ALREADY"); else if (funds < gaccCost[a]) { gsay("NOT ENOUGH CASH"); voice(V_NAH); } else { funds -= gaccCost[a]; t->accs |= (u8)(1 << a); gsay("BOUGHT! WEAR IT FROM DRESS"); voice(V_YAHOO); } }
            else { int s = -1; for (int i = 0; i < GB_N; i++) if (!t->b[i].on) { s = i; break; } if (s < 0) gsay("THE BASKET IS FULL"); else if (funds < GOTCHI_EGG) { gsay("A NEW EGG COSTS $150"); voice(V_NAH); } else { funds -= GOTCHI_EGG; gegg(t, s); gsay("A NEW EGG! KEEP IT CLOSE"); voice(V_YAHOO); } } }
        break; }
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
    { "THE HANDHELD", 1, { "D-PAD PICKS, A USES. L R SWAP PETS. X FEEDS.", "SELECT DOES WHAT IT NEEDS MOST. TAP ICONS TOO.", "TRAIN SKILLS, DO THREE GOALS A DAY (TODAY)." } },
    { "EIGHT PETS PER TOY", 2, { "THE BASKET HOLDS EIGHT. A NEW EGG IS $150.", "16 KINDS: CATS, ROBOTS, GHOSTS, ALIENS...", "HEARTS SHOW HOW CLOSE YOU TWO ARE." } },
    { "GROWING UP", 3, { "EGG, BABY, CHILD, TEEN, ADULT. ~2 DAYS.", "YOUR CARE DECIDES WHAT IT BECOMES.", "GROWN-UPS FILL THE BLOBDEX. GET THEM ALL!" } },
    { "PLAY, FRIENDS AND EGGS", 4, { "EIGHT GAMES TRAIN AND CHEER IT UP.", "PLAYDATES CHEER BOTH. Y = BATTLE.", "BEST FRIENDS MAY LEAVE A SURPRISE EGG." } },
    { "HIJINKS: YOU AND THE PET", 5, { "LIVE MODE: REAL TIME. YOU MAY FORGET TO EAT!", "SAD OR STARVING? YOUR PETS FEEL IT TOO.", "SHARE YOUR LUNCH, OR GET A NUZZLE FOR FUN." } },
    { "MOODS AND CARE SKILL", 6, { "PETS CHANGE YOUR MOOD: PROUD PARENT, BUDDY...", "NEGLECT THEM AND YOU WORRY. LOSE ONE: OUCH.", "CARE SKILL GROWS: LESS HUNGER, LESS SICK." } },
    { "BUILD AND BUY THINGS", 7, { "THE GOTCH. TAB: NINE THINGS FOR HOME.", "DOCK OPENS THE BASKET. CABINET: BLOBDEX.", "BOWL, LAMP, BED CARE FOR ALL PETS AT ONCE." } },
    { "FAMILIES", 9, { "BEST FRIENDS BECOME A COUPLE, THEN LEAVE EGGS.", "BABIES GET COLOURS, NATURES, A RARE SHINE.", "PARENTS CARE FOR BABIES. ELDERS PASS ON." } },
    { "WHAT'S NEW", 8, { "NEW: FAMILY TREE, COUPLES, EGGS OF LOVE.", "NEW: NATURES, COLOURS, ELDERS, MEMORIAL.", "NEW: SIBLINGS PLAY, PARENTS CARE." } },
};
static void gtile(int x, int y, int sp, int stage, int look, int sc) {   // a pet on a little LCD
    Blob bb; memset(&bb, 0, sizeof bb); bb.on = 1; bb.sp = (u8)(sp % GSP_N); bb.stage = (u8)stage; bb.hun = bb.hap = 4; bb.age = (u16)(stage == GS_EGG ? 3 : 16); bb.col = (u8)(stage == GS_EGG ? 3 : 0); g_scene(&bb, ganim / 20, look);   // (D69: a guide's egg is a gold one)
    rect(x, y, 32 * sc + 2, 16 * sc + 2, LCD_BG); gdots_draw(x + 1, y + 1, sc, LCD_INK); }
static void guide_strip(int k) {   // the picture under each page, on the touch screen
    int y = 26, cyc = (ganim / 120) % GSP_N; rect(6, y - 4, 244, 112, C15(12, 8, 18)); char b[32];
    switch (k) {
    case 0: for (int i = 0; i < 3; i++) { int sp = (i * 4 + cyc) % GSP_N; gtile(14 + i * 80, y + 4, sp, GS_ADULT, 0, 2); text(14 + i * 80, y + 44, 1, TXT, gspNm[sp]); } text_fit(14, y + 70, 1, 228, C15(20, 24, 28), "Sixteen kinds. Each grows four ways."); break;
    case 1: { static const int ic[5] = { 0, 2, 3, 4, 5 }; for (int i = 0; i < 5; i++) { int x = 12 + i * 47; rect(x, y + 4, 43, 50, C15(11, 4, 14)); for (int r = 0; r < 7; r++) for (int q = 0; q < 9; q++) if (gicon[ic[i]][r][q] == '#') rect(x + 8 + q * 3, y + 10 + r * 3, 3, 3, C15(24, 16, 30)); t_tight = 1; text_fit(x + 2, y + 38, 1, 40, TXT, giconNm[ic[i]]); t_tight = 0; }
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
// ==== (D64) drawing: the pet's screen (LCD, chips, four needs, bond, manners, wish, goals), the games, the pages ====
static void gl(int x, int y, const char *s) { text_fit(PX(x), PY(y), 1, 156, LCD_INK, s); }          // text on the LCD at the PS1's own numbers
static void glc(int cx, int y, const char *s) { text(PX(cx) - tw(1, s) / 2, PY(y), 1, LCD_INK, s); }
static int gwrapn(int x, int y, int w, int lines, u16 c, const char *s) {   // words broken to fit w (at most that many lines); returns the lines used
    char ln[80]; int n = (int)strlen(s), pos = 0, l = 0;
    while (pos < n && l < lines) { int last = -1; for (int k = pos; k <= n; k++) { if (k == n || s[k] == ' ') { int len = k - pos; if (len > 78) len = 78; memcpy(ln, s + pos, (size_t)len); ln[len] = 0; if (tw(1, ln) <= w) last = k; else break; } }
        if (last < 0) { last = pos + 1; while (last < n && s[last] != ' ') last++; }
        int len = last - pos; if (len > 78) len = 78; memcpy(ln, s + pos, (size_t)len); ln[len] = 0; text_fit(x, y + l * 9, 1, w, c, ln); pos = last; while (pos < n && s[pos] == ' ') pos++; l++; }
    return l; }
static void gstrip(const char *s) { rect(LX, LY + 82, 160, 12, K8(150, 176, 108)); text(LX + 80 - MIN(tw(1, s), 156) / 2, LY + 84, 1, LCD_INK, s); }
static int gchip(int x, int y, const char *s, u16 bg, u16 ink) { int w = tw(1, s) + 8; rect(x, y, w, 11, bg); text(x + 4, y + 2, 1, ink, s); return x + w + 3; }
static void gmeter(int x, int y, int w, const char *l, int pct) {   // a word and a bar under it that goes green, amber, red (and flashes when low)
    int p = CLAMP(pct, 0, 100); u16 c = p > 50 ? C15(10, 26, 10) : p > 25 ? C15(31, 24, 5) : C15(30, 8, 8); int low = p <= 25 && ((ganim >> 4) & 1);
    text_fit(x, y, 1, w, low ? C15(31, 15, 14) : TXT, l); rect(x, y + 9, w, 6, K8(30, 14, 46)); if (p) rect(x + 1, y + 10, (w - 2) * p / 100, 4, c); }
static int gmood_of(const Blob *b, const Xt *x) {   // 0 plain, 1 happy, 2 dance, 3 sleep, 4 sick, 5 hungry, 6 bored, 7 angry, 8 tired, 9 dirty
    if (!b->on || b->stage == GS_EGG) return 0;
    if (gdanceT > 0) return 2; if (gactT > 0 && !(b->fl & GF_SLEEP)) return 1; if (b->fl & GF_SLEEP) return 3; if (b->fl & GF_SICK) return 4; if (b->hun == 0) return 5;
    if (x->nrg < 25 && b->stage >= GS_CHILD) return 8; if (x->hyg < 30) return 9; if (b->hap <= 1) return 6; if (b->hap >= 3 && b->hun >= 2) return 1; return 0; }
static void gformchip(const Blob *b, int x, int y) {
    static const u16 fc[4] = { C15(29, 22, 5), C15(18, 14, 8), C15(7, 23, 26), C15(27, 8, 22) };
    const char *s; u16 bg = C15(9, 14, 26), ink = TXT;
    if (b->stage == GS_EGG) { s = "EGG"; bg = C15(28, 20, 4); ink = C15(4, 3, 2); } else if (b->stage == GS_BABY) { s = "BABY"; bg = C15(6, 20, 8); } else if (b->stage == GS_CHILD) s = "CHILD";
    else if (b->stage == GS_TEEN) { s = b->form ? "SLOUCHY" : "ANGSTY"; bg = b->form ? C15(18, 14, 8) : C15(8, 10, 24); } else if (b->stage == GS_ELDER) { s = "ELDER"; bg = C15(16, 16, 19); } else { s = gformNm[b->form & 3]; bg = fc[b->form & 3]; if ((b->form & 3) == 0) ink = C15(4, 3, 2); }
    gchip(x, y, s, bg, ink); }
static int gform_w(const Blob *b) { const char *s = b->stage == GS_EGG ? "EGG" : b->stage == GS_BABY ? "BABY" : b->stage == GS_CHILD ? "CHILD" : b->stage == GS_TEEN ? (b->form ? "SLOUCHY" : "ANGSTY") : b->stage == GS_ELDER ? "ELDER" : gformNm[b->form & 3]; return tw(1, s) + 11; }
static void gtoy_pic(int x, int y, int egg_on) {   // a little toy on a card: body, LCD with the egg, buttons
    rect(x, y, 84, 74, K8(80, 36, 130)); rect(x + 1, y + 1, 82, 72, K8(150, 90, 210)); rect(x + 3, y + 3, 78, 6, K8(250, 252, 255));
    rect(x + 8, y + 10, 68, 36, K8(30, 12, 44)); rect(x + 10, y + 12, 64, 32, K8(172, 196, 128));
    if (egg_on) { Blob e; memset(&e, 0, sizeof e); e.on = 1; e.sp = 1; g_scene(&e, ganim / 20, 0); gdots_draw(x + 10, y + 12, 2, LCD_INK); }
    gdisc(x + 24, y + 58, 10, K8(255, 205, 60)); gdisc(x + 42, y + 62, 10, K8(255, 205, 60)); gdisc(x + 60, y + 58, 10, K8(255, 205, 60)); }
static void gcard(int pg) {   // the first day's welcome and closing cards, over the LCD
    rect(10, 4, 236, 108, K8(52, 100, 220)); rect(12, 6, 232, 104, K8(240, 246, 255)); gchip(18, 10, pg ? "NICE WORK" : "DAY ONE", C15(9, 14, 26), TXT);
    if (pg == 0) { gtoy_pic(18, 26, 1); gwrapn(110, 28, 128, 7, K8(30, 30, 60), "THIS EGG IS YOURS. WARM IT UP AND IT HATCHES. THEN FEED IT, PLAY WITH IT, CLEAN UP AND TUCK IT IN. I WILL SHOW YOU."); }
    else { gstar5(34, 34, 8, K8(255, 200, 40)); gheart(22, 84, 4, K8(226, 60, 70)); gdisc(80, 94, 16, K8(70, 200, 90)); gwrapn(110, 28, 128, 8, K8(30, 30, 60), "YOU FED IT, PLAYED, CLEANED UP AND TUCKED IT IN. 3 GOALS A DAY: SEE THE TODAY ICON. KEEP A STREAK FOR PRIZES. HERE IS $100."); } }
static void gdraw_event(void) {   // a surprise on the LCD (the PS1's pictures, its numbers)
    int e = gevT, x, y;
    switch (gevK) {
    case 1: rect(PX(130), PY(20), 2, 78, C15(15, 31, 15)); rect(PX(190), PY(20), 2, 78, C15(15, 31, 15)); x = 84 + ABS(((300 - e) * 2) % 300 - 150); y = 36 + ((e * 5) % 36);
        rect(PX(x), PY(y), 6, 5, LCD_INK); rect(PX(x) - 3, PY(y) - 2, 4, 3, C15(31, 31, 31)); rect(PX(x) + 5, PY(y) - 2, 4, 3, C15(31, 31, 31)); break;
    case 2: x = 190; y = 20 + MIN(70, (300 - e) / 3); gdisc(PX(x), PY(y), 28, K8(255, 200, 40)); gdisc(PX(x), PY(y), 16, K8(255, 236, 130)); break;
    case 3: x = 236 - (300 - e) * 3 / 4; y = 20 + (300 - e) / 4; gstar5(PX(x), PY(y), 3, K8(255, 230, 90)); for (int i = 1; i < 6; i++) rect(PX(x) + i * 6 + 6, PY(y) - i * 2 + 4, 3, 3, K8(255, 240, 160)); break;
    case 4: if ((e % 48) < 5) rect(LX, LY, 160, 80, C15(31, 31, 31)); { static const char *const bolt[7] = { "..##", ".##.", "###.", ".###", "..##.", "..#..", ".#.." }; gbm(bolt, 7, PX(190), PY(24), 4, K8(255, 226, 60)); } break;
    default: break; } }
static const char *const gcoachTx[6] = { "", "WARM THE EGG. TAP WARM 5 TIMES", "IT IS HUNGRY. TAP FEED", "IT IS BORED. TAP PLAY", "IT MADE A MESS. TAP CLEAN", "IT IS SLEEPY. TAP LIGHT" };
static const u8 gurgIcon[10] = { 99, 5, 2, 6, 8, 0, 4, 1, 3, 4 };
static const char *const gurgTxt[10] = { "", "SICK! TAP MEDS", "A MESS! TAP CLEAN", "", "IT IS FIBBING. TAP SCOLD", "HUNGRY! TAP FEED", "SLEEPY. TAP LIGHT", "BORED. TAP PLAY", "SMELLY! TAP BATH", "TIRED. TAP LIGHT FOR A NAP" };
static const char *const gdyVerb[32] = { "", "HATCHED", "GREW UP A BIT", "BECAME A GROWN-UP", "PASSED ON. REMEMBERED", "BROUGHT A GIFT", "HAD A BIRTHDAY", "HAD A TANTRUM", "WAS ILL", "IS A NEW EGG", "HAD A PLAYDATE", "IS A RARE MUTANT", "WAS LOOKED AFTER", "FELL IN LOVE", "VISITED A HOUSEMATE", "",
    "", "HAD A WISH COME TRUE", "TRAINED HARD", "BEAT ITS BEST SCORE", "MADE A BEST FRIEND", "MISSES A FRIEND", "HAD A SURPRISE", "KEPT THE DAILY GOALS", "", "", "", "", "", "", "", "" };
static void gstats_page(Toy *t, Blob *b, const Xt *x) {   // the five pages on the LCD
    char s[72], nm[40]; gname(b, nm);
    if (gsub == 0) {
        glc(160, 20, "NEEDS"); static const char *const l[6] = { "FOOD", "FUN", "ENERGY", "CLEAN", "BOND", "MANNERS" }; int v[6] = { b->hun * 25, b->hap * 25, x->nrg, x->hyg, b->bond, b->disc };
        for (int i = 0; i < 6; i++) { int y = 31 + i * 10; gl(84, y, l[i]); rect(PX(138), PY(y + 1), 66, 7, K8(110, 140, 90)); rect(PX(139), PY(y + 2), 64 * v[i] / 100, 5, v[i] > 50 ? C15(4, 18, 4) : v[i] > 25 ? C15(22, 14, 0) : C15(24, 3, 3)); sprintf(s, "%d", v[i]); gl(210, y, s); }
        int m = gmood(t, gbi); sprintf(s, "MOOD %s  %d", gmood_word(m), m); gl(84, 93, s);
        sprintf(s, "%dG  MISTAKES %d  %s", b->wt, b->miss, (b->fl & GF_SICK) ? "SICK" : b->poop ? "MESSY" : "WELL"); gl(84, 103, s);
    } else if (gsub == 1) {
        gl(84, 20, nm); sprintf(s, "%s %s   GEN %d", gcolNm[b->col & 7], gspNm[b->sp % GSP_N], b->gen + 1); gl(84, 31, s);
        sprintf(s, "%s: %s", gpersNm[b->pers & 7], gpersNote[b->pers & 7]); gwrapn(PX(84), PY(42), 152, 2, LCD_INK, s);
        sprintf(s, "LIKES %s", gfoodNm[(b->taste >> 4) % NFOOD]); gl(84, 64, s); sprintf(s, "HATES %s", gfoodNm[(b->taste & 15) % NFOOD]); gl(84, 75, s);
        sprintf(s, "%s. %s", gstageNm[b->stage % 6], gstageNote[b->stage % 6]); gwrapn(PX(84), PY(86), 152, 2, LCD_INK, s);
    } else if (gsub == 2) {
        glc(160, 20, "SKILLS");
        for (int i = 0; i < 3; i++) { int y = 31 + i * 19; gl(84, y, gskNm[i]); rect(PX(130), PY(y + 1), 78, 7, K8(110, 140, 90)); rect(PX(131), PY(y + 2), 76 * x->sk[i] / 100, 5, C15(5, 8, 22)); sprintf(s, "%d", x->sk[i]); gl(214, y, s); gl(84, y + 9, gwhyNm[i]); }
        int tl = 0; for (int q = 0; q < GLN; q++) if (t->ln[q].id == b->id) tl = t->tal[q]; sprintf(s, "TALENT: %s", gtalNm[tl & 3]); gl(84, 89, s);
        if (x->bff && t->b[x->bff - 1].on) { sprintf(s, "BEST FRIEND %s", ggiv[t->b[x->bff - 1].nm & 31]); gl(84, 100, s); } else gl(84, 100, "NO BEST FRIEND YET");
    } else if (gsub == 3) {
        sprintf(s, "MEMORIES  AGE %d", x->bd); gl(84, 20, s); int shown = 0;
        for (int i = 1; i <= GD_N && shown < 7; i++) { const Dy *d = &gdy[((gdn - i) % GD_N + GD_N) % GD_N]; if (!d->kind || i > gdn || (d->nm & 31) != (b->nm & 31) || (d->kind & 31) == GD_AWARD || (d->kind & 31) == GD_DAY) continue;
            sprintf(s, "DAY %d %02d %s", d->day / 24 + 1, d->day % 24, gdyVerb[d->kind & 31]); gl(84, 32 + shown * 10, s); shown++; }
        if (!shown) gwrapn(PX(84), PY(36), 152, 4, LCD_INK, "NOTHING YET. HATCHING, GROWING, WISHES, BIRTHDAYS AND SURPRISES ARE WRITTEN HERE.");
    } else {
        glc(160, 20, "YOU AS A CARER"); sprintf(s, "LEVEL %d: %s", glevel(t), glvNm[glevel(t)]); gl(84, 32, s);
        int nx = gnext_xp(t); if (nx) { rect(PX(84), PY(45), 150, 6, K8(130, 160, 100)); rect(PX(85), PY(46), 148 * MIN(100, t->xp * 100 / nx) / 100, 4, LCD_INK); }
        sprintf(s, "CARE SCORE %d", gscore()); gl(84, 56, s); sprintf(s, "%d HATCHED  %d WINS", t->hatched, t->wins); gl(84, 67, s); sprintf(s, "%d PLAYDATES AND GAMES", t->plays); gl(84, 78, s);
        sprintf(s, "STREAK %d DAYS  BEST %d", t->streak, t->best); gl(84, 89, s);
        { int c = 0; for (int i = 0; i < NACH; i++) c += ghas(i); sprintf(s, "%d AWARDS OF %d", c, NACH); gl(84, 100, s); } } }
static void gtop_pet(void) {   // every view that shows the pet: the LCD, what is going on inside, the four needs, bond, manners, wish and goals
    Toy *t = gtt(), *ct = t; Blob *c = gcb(); Blob pv; const char *own = 0; char b[96]; int cs = gbi, cp = gview == GV_HAND ? gtut_page() : -1;
    if (gview == GV_FRIEND) { gfr_build(); if (gfr_n) { if (gsub >= gfr_n) gsub = 0; ct = &gtoy[gfr_t[gsub]]; cs = gfr_s[gsub]; c = &ct->b[cs]; if (ct != t && gsim_of(ct) >= 0) own = sname[gsim_of(ct)]; } }
    if (gview == GV_DRESS) { pv = *c; pv.acc = (u8)gsub; c = &pv; }
    const Xt *x = &ct->x[cs];
    int dark = (c->fl & GF_SLEEP) && (c->fl & GF_DARK); u16 ink = dark ? C15(14, 18, 12) : gcolInk[c->col & 7];
    rect(LX - 8, LY - 4, 176, 102, K8(26, 10, 40)); rect(LX, LY, 160, 94, dark ? C15(8, 10, 8) : LCD_BG);
    int look = gview == GV_LOOK ? (gmN ? (gsidx ? 1 : -1) : 0) : 0, jump = 0;
    if (gview == GV_COPY) { int showing = gmT >= 0 && gmT < gmN * 40, k = showing ? gmT / 40 : -1; if (showing) { int m = gseq[k]; look = m == 2 ? -1 : m == 3 ? 1 : 0; jump = m == 0; } }
    gmd = gmood_of(c, x); if (jump || gview == GV_DRESS) gmd = gdanceT > 0 ? 2 : 1; if (gevK == 4 && gview == GV_HAND) gmd = 7; if (gview == GV_BEAT && gbT > 0) gmd = 2;
    if (gview == GV_STATS) { gstats_page(ct, c, x); }
    else if (gview == GV_TRAIN) { glc(160, 20, "TRAIN A SKILL"); static const char *const gm[3] = { "HOP, CATCH", "COPY, STACK", "BEAT, WHICH WAY" };
        for (int i = 0; i < 3; i++) { int y = 30 + i * 22, s = i == gsub; rect(PX(84), PY(y), 152, 20, s ? C15(8, 12, 28) : C15(14, 17, 10)); rect(PX(85), PY(y + 1), 150, 18, s ? C15(8, 12, 28) : C15(18, 21, 13));
            u16 ik = s ? TXT : LCD_INK; text(PX(90), PY(y + 2), 1, ik, gskNm[i]); text(PX(90), PY(y + 11), 1, s ? C15(22, 26, 31) : C15(8, 10, 8), gm[i]); rect(PX(150), PY(y + 3), 60, 6, K8(30, 14, 46)); rect(PX(150), PY(y + 4), 58 * x->sk[i] / 100, 4, C15(31, 24, 5)); sprintf(b, "%d", x->sk[i]); text(PX(216), PY(y + 2), 1, ik, b); } }
    else if (gview == GV_CATCH) { memset(gdot, 0, sizeof gdot); gminY = 16; Blob g = *c; if (g.stage < GS_CHILD) g.stage = GS_CHILD; g_creature(&g, ganim / 8, 1, gcx);
        static const u8 pp[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 1, 0 }, { 1, 1, 1, 1 } };
        for (int i = 0; i < 3; i++) if (gcY[i] >= 0 && gcY[i] < 15) { if (gcK[i]) { for (int r = 0; r < 4; r++) for (int q = 0; q < 4; q++) if (pp[r][q]) gP(gcX[i] + q - 1, gcY[i] + r - 1, 1); } else gstar(gcX[i], gcY[i]); }
        gdots_draw(LX, LY, 5, ink); }
    else if (gview == GV_HILO) { memset(gdot, 0, sizeof gdot); gminY = 16; Blob g = *c; if (g.stage < GS_CHILD) g.stage = GS_CHILD; g_creature(&g, ganim / 20, 1, -9); gdots_draw(LX, LY, 5, ink);
        sprintf(b, "%d", ghiNum); text(PX(190), PY(30), 2, LCD_INK, b);
        if (ghiNext) { int ok = (ghiNext > ghiNum) == (ghiGuess > 0); sprintf(b, "%d", ghiNext); text(PX(190), PY(62), 2, ok ? C15(3, 16, 5) : C15(24, 5, 5), b); } }
    else if (gview == GV_BEAT) { g_scene(c, ganim / 20, 0); gdots_draw(PX(112), PY(22), 3, ink);
        rect(PX(84), PY(90), 152, 2, C15(11, 14, 9)); gdisc(PX(100), PY(91), 22, K8(60, 90, 50)); gdisc(PX(100), PY(91), 18, K8(206, 228, 160)); if (gbT > 0) gdisc(PX(100), PY(91), 24 + gbT / 3, C15(31, 31, 31));
        for (int i = 0; i < BN; i++) { int xx = 100 + (gbAt[i] - gbTm) * 2; if (xx < 86 || xx > 232) continue; u16 cc = gbRes[i] == 1 ? C15(31, 26, 7) : gbRes[i] == 2 ? C15(15, 25, 15) : gbRes[i] == 3 ? C15(19, 19, 19) : C15(28, 9, 16);
            if (gbRes[i] == 1 || gbRes[i] == 2) { gstar5(PX(xx) - 6, PY(82 - (gbTm - gbAt[i]) / 2), 2, cc); continue; } gdisc(PX(xx), PY(91), 12, cc); gdisc(PX(xx), PY(91), 6, C15(31, 31, 31)); }
        if (gbJT > 0) { static const char *const jn[5] = { "", "PERFECT!", "GOOD", "MISS", "TOO EARLY" }; text(LX + 80 - tw(1, jn[gbJudge]) / 2, PY(74), 1, gbJudge == 1 ? C15(31, 26, 7) : gbJudge >= 3 ? C15(15, 15, 15) : C15(7, 26, 7), jn[gbJudge]); } }
    else if (gview == GV_HOP) { rect(PX(80), PY(96), 160, 3, C15(11, 14, 9));
        for (int i = 0; i < 3; i++) gheart(PX(84) + i * 12, PY(21), 2, i < 3 - ghHits ? C15(28, 6, 8) : C15(14, 17, 10));
        if (!(ghHurt > 0 && (ghHurt & 4))) { g_scene(c, ganim / 20, 0); gdots_draw(PX(84), PY(64) - ghY / 16, 2, ink); }
        for (int i = 0; i < 3; i++) if (ghOx[i] > -90 * 16) { int xx = ghOx[i] / 16, w = ghOt[i] == 2 ? 22 : ghOt[i] == 1 ? 10 : 12, h = ghOt[i] == 1 ? 24 : 10, x0 = MAX(80, xx), x1 = MIN(240, xx + w); if (x1 <= x0) continue;
            rect(PX(x0), PY(96 - h), x1 - x0, h, C15(12, 9, 5)); rect(PX(x0), PY(96 - h), x1 - x0, 3, C15(9, 18, 9)); } }
    else if (gview == GV_WHACK) { static const char *const rl[8] = { "", "QUICK! 2", "GOOD 1", "TOO LATE", "OOPS, MESS", "GOOD, LEFT IT", "WRONG HOLE", "TOO EARLY" };
        for (int i = 0; i < 4; i++) { gdisc(PX(gwcx[i]), PY(gwcy[i]) + 4, 26, K8(60, 90, 50)); gdisc(PX(gwcx[i]), PY(gwcy[i]) + 2, 22, K8(206, 228, 160)); }
        if (gwSt == 1) { int hx = PX(gwcx[gwHole]), hy = PY(gwcy[gwHole]); if (gwMess) { gdisc(hx, hy, 18, C15(26, 4, 4)); text(hx - 2, hy - 4, 1, C15(31, 31, 31), "!"); }
            else { gdisc(hx, hy, 14, LCD_INK); rect(hx - 8, hy - 8, 4, 3, LCD_INK); rect(hx + 5, hy - 8, 4, 3, LCD_INK); rect(hx - 3, hy - 2, 2, 2, LCD_BG); rect(hx + 2, hy - 2, 2, 2, LCD_BG); } }
        gmd = gwMT > 0 ? gwMood : MD_PLAIN; g_scene(c, ganim / 20, 0); gdots_draw(PX(84), PY(44), 2, ink);
        if (gwRT > 0 && gwRes) text(LX + 80 - tw(1, rl[gwRes]) / 2, PY(74), 1, (gwRes <= 2 || gwRes == 5) ? C15(3, 16, 5) : C15(24, 5, 5), rl[gwRes]); }
    else if (gview == GV_STACK) { static const u8 pc[8][3] = { { 240, 90, 120 }, { 250, 160, 60 }, { 240, 214, 60 }, { 110, 200, 90 }, { 70, 190, 210 }, { 90, 130, 240 }, { 160, 100, 230 }, { 230, 100, 200 } };
        rect(PX(84), PY(97), 90, 2, C15(11, 14, 9));
        for (int i = 0; i < gsLv && i <= SN; i++) { const u8 *q = pc[i % 8]; rect(PX(gsL[i]), PY(97 - (i + 1) * 7), gsW[i], 6, K8(q[0], q[1], q[2])); rect(PX(gsL[i]), PY(97 - (i + 1) * 7), gsW[i], 2, K8(MIN(255, q[0] + 40), MIN(255, q[1] + 40), MIN(255, q[2] + 40))); }
        if (gmT >= 0 && gsLv <= SN) { const u8 *q = pc[gsLv % 8]; int yy = 97 - (gsLv + 1) * 7 + (gsOver ? gsOver * 2 : 0); if (yy < 100 - 82 + 4 + 40 || 1) rect(PX(gsCx / 16), PY(yy), gsCw, 6, K8(q[0], q[1], q[2]));
            if (!gsOver) { rect(PX(gsL[gsLv - 1]), PY(yy + 6), 1, 4, C15(31, 31, 31)); rect(PX(gsL[gsLv - 1] + gsW[gsLv - 1]), PY(yy + 6), 1, 4, C15(31, 31, 31)); } }
        gmd = gsLv >= 6 ? 2 : gsOver && gsLv <= SN ? 6 : 0; g_scene(c, ganim / 20, 0); gdots_draw(PX(176), PY(66), 2, ink); }
    else { g_scene(c, ganim / 20 + (jump ? 1 : 0), look); gdots_draw(LX, LY - (jump ? 5 : 0), 5, ink);
        if (gview == GV_HAND && cp < 0 && gevK == 5) { Blob vb; memset(&vb, 0, sizeof vb); vb.on = 1; vb.stage = GS_CHILD; vb.sp = (u8)(gevHit % 16); vb.hun = vb.hap = 4; memset(gdot, 0, sizeof gdot); gminY = 16; g_creature(&vb, ganim / 20, -1, 10); gdots_draw(LX, LY, 5, ink); }
        if (gview == GV_HAND && cp < 0 && gevK) gdraw_event(); }
    gmd = 0;
    // the strip along the foot of the LCD
    if (gview == GV_TRAIN) gstrip(gwhyNm[gsub % 3]); else if (gview == GV_STATS) { }
    else if (gview == GV_DRESS) { Toy *dt = gtt(); if (gsub == 0 || (dt->accs & (1 << gsub))) sprintf(b, "%s: %s", gaccNm[gsub], gsub == gcb()->acc ? "WEARING IT" : "OWNED"); else sprintf(b, "%s: $%d", gaccNm[gsub], gaccCost[gsub]); gstrip(b); }
    else if (gview == GV_PLAYPICK) { sprintf(b, "BEST SCORE %d", gbest[gsub & 7]); gstrip(b); }
    else if (gview == GV_LOOK || gview == GV_COPY) { sprintf(b, "ROUND %d  %s %d", gmRound + 1, gview == GV_LOOK ? "WINS" : "GOT", gmWon); gstrip(b); }
    else if (gview == GV_HILO) { sprintf(b, "ROUND %d  WINS %d", gmRound + 1, gmWon); gstrip(b); }
    else if (gview == GV_CATCH) { sprintf(b, "TREATS %d  MESS %d  %d LEFT", gcScore, gcBad, 14 - gcN); gstrip(b); }
    else if (gview == GV_BEAT) { sprintf(b, "PERFECT %d  GOOD %d  COMBO %d", gbPerf, gbGood, gbCombo); gstrip(b); }
    else if (gview == GV_HOP) { sprintf(b, "ROCKS %d OF %d  LIVES %d", ghDone, HN, 3 - ghHits); gstrip(b); }
    else if (gview == GV_STACK) { sprintf(b, "FLOOR %d OF %d  PERFECT %d", MIN(gsLv, SN), SN, gsPerf); gstrip(b); }
    else if (gview == GV_WHACK) { sprintf(b, "POP %d OF %d  POINTS %d", MIN(gwN + 1, WN), WN, gwPts); gstrip(b); }
    else { char nm[40]; gname(c, nm); if (own) sprintf(b, "%s THE %s (%s)", ggiv[c->nm & 31], nm, own); else sprintf(b, "%s THE %s", ggiv[c->nm & 31], nm); gstrip(b); }
    gpar_draw();
    if (o_gtime == 1) text(LX + 126, LY + 2, 1, C15(26, 4, 4), "LIVE"); else if (o_gtime == 2) text(LX + 114, LY + 2, 1, C15(12, 16, 12), "PAUSED");
    if (cp == 0 || cp == 6) { gcard(cp); return; }
    // the chips: its form, then what is going on inside (bad ones red, good ones green)
    { int cx = 8; gformchip(c, cx, 108); cx += gform_w(c);
      if (cp >= 1 && cp <= 5) { sprintf(b, "DAY ONE. STEP %d OF 5", cp); gchip(cx, 108, b, C15(9, 14, 26), TXT); }
      else { const char *ml[3]; u8 gd[3]; int n = gview == GV_HAND || gview == GV_STATS ? gmoodlets(ct, cs, ml, gd) : 0; for (int i = 0; i < n; i++) { if (cx + tw(1, ml[i]) + 8 > 250) break; cx = gchip(cx, 108, ml[i], gd[i] ? C15(5, 20, 8) : C15(25, 5, 5), TXT); }
        if (c->mate && cx + 50 < 250) cx = gchip(cx, 108, "PAIRED", C15(27, 9, 16), TXT); else if (c->gen && cx + 44 < 250) { sprintf(b, "GEN %d", c->gen + 1); cx = gchip(cx, 108, b, C15(29, 22, 5), C15(4, 3, 2)); } } }
    // the needs: four across, then bond, manners, the wish, today's goals
    gmeter(6, 122, 56, "FOOD", c->hun * 25); gmeter(68, 122, 56, "FUN", c->hap * 25); gmeter(130, 122, 56, "ENERGY", x->nrg); gmeter(192, 122, 56, "CLEAN", x->hyg);
    gmeter(6, 140, 56, "BOND", c->bond); gmeter(68, 140, 56, "MANNERS", c->disc);
    { int w = x->wish & 7; text(130, 140, 1, TXT, "WISH"); if (w && c->stage > GS_EGG) text_fit(130, 149, 1, 58, C15(31, 28, 9), gwNm[w]); else text(130, 149, 1, C15(17, 15, 22), "NONE");
      text(192, 140, 1, TXT, "GOALS"); int dn = ggoals_done(ct); for (int i = 0; i < 3; i++) { rect(192 + i * 19, 150, 17, 7, K8(30, 14, 46)); if (i < dn) rect(193 + i * 19, 151, 15, 5, C15(31, 26, 7)); } }
    // the line along the bottom: a message, a surprise, the step, what it needs, or what the screen is for
    { const char *m = gmsg; static char m2[64]; u16 mc = TXT;
      if (gview == GV_LOOK && !m) m = gmN == 0 ? "WHICH WAY WILL IT LOOK?" : ((gsidx ? 1 : -1) == gmDir ? "YES! IT LOOKED YOUR WAY" : "NOPE. THE OTHER WAY");
      else if (gview == GV_COPY && !m) { static const char *nm[4] = { "UP", "DOWN", "LEFT", "RIGHT" }; int showing = gmT >= 0 && gmT < gmN * 40; if (gmT < 0) m = "WATCH CLOSELY..."; else if (showing) m = nm[gseq[gmT / 40]]; else { sprintf(m2, "YOUR TURN: %d OF %d", gsidx + 1, gmN); m = m2; } }
      else if (gview == GV_CATCH && !m) m = gmT < 0 ? "GET READY..." : "SLIDE: CATCH STARS, DODGE MESS";
      else if (gview == GV_HILO && !m) m = ghiNext ? ((ghiNext > ghiNum) == (ghiGuess > 0) ? "RIGHT!" : "WRONG. NEXT ONE") : "WILL THE NEXT ONE BE HIGHER OR LOWER?";
      else if (gview == GV_BEAT && !m) m = gmT < 0 ? "GET READY..." : "TAP WHEN A NOTE IS ON THE RING";
      else if (gview == GV_HOP && !m) m = gmT < 0 ? "GET READY..." : "TAP TO JUMP THE ROCKS";
      else if (gview == GV_STACK && !m) m = gmT < 0 ? "GET READY..." : gsOver ? (gsLv > SN ? "THE TOWER IS DONE!" : "IT TUMBLED!") : "TAP TO DROP THE BLOCK. LINE IT UP";
      else if (gview == GV_WHACK && !m) m = gmT < 0 ? "GET READY..." : "D-PAD TOWARD THE BUG. LEAVE THE MESS";
      else if (gview == GV_PLAYPICK && !m) { sprintf(m2, "%s. ENERGY %d", ggameHow[gsub & 7], x->nrg); m = m2; }
      else if (gview == GV_TRAIN && !m) m = "PICK A SKILL, THEN TRAIN";
      else if (gview == GV_HAND && !m) {
          if (gevK) { static const char *const et[6] = { "", "A FLY! REACT WHEN IT IS IN THE MIDDLE", "A COIN! REACT TO GRAB IT", "A SHOOTING STAR! REACT TO WISH", "THUNDER! REACT TO HOLD IT CLOSE", "A VISITOR! REACT TO SAY HELLO" }; m = et[gevK]; mc = C15(31, 26, 7); }
          else if (cp >= 1 && cp <= 5) { m = gcoachTx[cp]; mc = C15(31, 26, 7); if (cp == 1) { for (int i = 0; i < 5; i++) gdisc(226 + i * 7 - 40, 183, 6, i < gtutT ? C15(31, 14, 18) : C15(10, 8, 14)); } }
          else { int u = gurgent(t, gbi); if (u && gurgTxt[u][0]) { m = gurgTxt[u]; mc = C15(31, 20, 12); } } }
      if (m) { rect(6, 162, 244, 20, K8(24, 10, 40)); gwrapn(10, 164, 236, 2, mc, m); } } }
static void gtop_goals(void) {   // TODAY: three goals, the streak, the last seven days
    Toy *t = gtt(); char s[64]; ggoals_roll(t); panel(4, 4, 248, 184, "Today's goals");
    static const u16 gic[6] = { C15(31, 22, 8), C15(31, 14, 24), C15(14, 24, 31), C15(31, 26, 5), C15(31, 10, 10), C15(24, 20, 31) }; static const char *const gl_[6] = { "F", "G", "B", "T", "C", "W" };
    for (int i = 0; i < 3; i++) { int ty = ggoal_type(t, i), have = ggoal_have(t, i), tg = gTarget[ty], y = 24 + i * 32, done = have >= tg;
        rect(8, y, 240, 29, PANB); rect(9, y + 1, 238, 27, done ? C15(6, 18, 9) : PAN); gdisc(24, y + 14, 20, done ? C15(8, 26, 10) : gic[ty]); text(21, y + 10, 1, C15(3, 3, 6), done ? "OK" : gl_[ty]);
        text(40, y + 4, 1, TXT, gNm[ty]); rect(40, y + 17, 140, 7, K8(30, 14, 46)); rect(41, y + 18, 138 * have / tg, 5, done ? C15(10, 26, 10) : C15(9, 14, 28)); sprintf(s, "%d / %d", have, tg); text(186, y + 16, 1, C15(20, 24, 28), s);
        if (done) gchip(206, y + 4, "DONE", C15(6, 20, 8), TXT); else gchip(204, y + 4, "+$10", C15(29, 22, 5), C15(4, 3, 2)); }
    rect(8, 122, 240, 30, C15(24, 16, 4)); rect(9, 123, 238, 28, C15(8, 7, 14)); sprintf(s, "STREAK  %d DAYS", t->streak); text(16, 127, 1, C15(31, 26, 7), s); sprintf(s, "BEST  %d", t->best); text(16, 139, 1, C15(20, 24, 28), s);
    sprintf(s, "ALL THREE: +$%d", 20 + 10 * MIN(t->streak + ((t->gflag & 1) ? 0 : 1), 10)); gchip(120, 130, s, C15(29, 22, 5), C15(4, 3, 2));
    text(14, 160, 1, C15(20, 24, 28), "LAST 7 DAYS"); for (int i = 0; i < 7; i++) { int on = (t->ghist >> (6 - i)) & 1; gdisc(106 + i * 18, 164, 13, on ? C15(10, 26, 10) : C15(10, 8, 14)); if (on) text(103 + i * 18, 160, 1, TXT, "+"); }
    gdisc(230, 160, 13, (t->gflag & 1) ? C15(29, 22, 5) : C15(12, 10, 16)); text(222, 178, 1, C15(20, 24, 28), "TODAY");
    text(14, 176, 1, C15(20, 24, 28), "NEW GOALS EVERY MORNING."); }
static void gtop_intro(void) {   // first run: the shop card, the unboxing, the egg
    char s[64];
    if (gisub == 0) { panel(4, 4, 248, 184, "Electronics shop");
        rect(10, 24, 104, 130, K8(240, 170, 30)); rect(11, 25, 102, 128, K8(255, 214, 70)); gtoy_pic(20, 30, 1); ctext(62, 108, 1, K8(40, 30, 10), "BOREAGOTCHI!"); ctext(62, 120, 1, K8(40, 30, 10), "16 KINDS INSIDE"); gchip(24, 134, "$5000  PAID", C15(6, 20, 8), TXT);
        static const char *const bl[4] = { "SIXTEEN KINDS TO COLLECT.", "FEED, PLAY, CLEAN AND TUCK IT IN.", "BOND, LAY EGGS, RAISE A FAMILY.", "ONE TOY PER BORE. EIGHT PETS EACH." };
        for (int i = 0; i < 4; i++) { int y = 24 + i * 32; rect(120, y, 128, 29, PANB); rect(121, y + 1, 126, 27, PAN); rect(125, y + 10, 8, 8, C15(10, 26, 10)); gwrapn(138, y + 5, 106, 3, TXT, bl[i]); }
        gwrapn(14, 160, 232, 2, C15(22, 26, 31), "THANK YOU! YOUR TOY CAME WITH ITS FIRST EGG."); return; }
    if (gisub == 1) { int t = giT, bx = 68, by = 100, rise = CLAMP((t - 45) * 2, 0, 80); rect(0, 0, 256, 192, K8(22, 12, 52));
        ctext(128, 14, 1, t < 40 ? TXT : K8(255, 235, 140), t < 40 ? "SOMETHING IS INSIDE..." : t < 75 ? "OPEN IT!" : "TA-DA!");
        rect(bx, by - 10, 120, 80, K8(110, 70, 36)); if (t > 44) gtoy_pic(86, by + 40 - rise, t > 60);
        rect(bx, by + 18, 120, 52, K8(170, 112, 56)); rect(bx + 2, by + 20, 116, 48, K8(214, 150, 80)); rect(bx + 54, by + 18, 12, 52, K8(255, 205, 60)); ctext(bx + 60, by + 50, 1, TXT, "BOREAGOTCHI!");
        { int lx = bx - 4 + (t < 40 ? ((t >> 1) & 1) * 4 - 2 : MIN(80, (t - 40) * 2)), ly = by - 22 - (t < 40 ? 0 : MIN(40, (t - 40) * 2)); rect(lx, ly, 128, 18, K8(190, 130, 66)); rect(lx + 2, ly + 2, 124, 14, K8(230, 168, 96)); rect(lx + 58, ly, 12, 18, K8(255, 205, 60)); }
        if (t >= 75) for (int i = 0; i < 26; i++) { u32 h = (u32)(i * 31 + 7) * 2654435761u; int x = (int)(h % 256), y = (int)(((h >> 8) % 160) + (t - 75) * (2 + (int)(h >> 20) % 3)) % 192; rect(x, y, 4, 4, K8(90 + (int)(h & 127), 90 + (int)((h >> 5) & 127), 120 + (int)((h >> 11) & 127))); }
        return; }
    Blob *c = gcb(); rect(LX - 8, LY - 4, 176, 102, K8(26, 10, 40)); rect(LX, LY, 160, 94, LCD_BG); g_scene(c, ganim / 20, 0); gdots_draw(LX, LY, 5, gcolInk[c->col & 7]);
    rect(12, 108, 232, 76, K8(24, 10, 40)); ctext(128, 114, 1, K8(255, 235, 140), "IT CAME WITH AN EGG!");
    gwrapn(18, 130, 220, 5, TXT, "KEEP IT WARM. IT HATCHES IN A FEW GAME MINUTES. THEN FEED IT, PLAY WITH IT AND WATCH IT GROW. I WILL SHOW YOU HOW."); (void)s; }
static void gtop_diary(void) {   // the last 32 lines, 13 to a page
    panel(4, 4, 248, 184, "Pet diary"); int shown = 0, seen = 0, skip = gsub * 13; char s[96]; sprintf(s, "PAGE %d OF 3", gsub + 1); text(176, 7, 1, C15(20, 24, 28), s);
    for (int i = 1; i <= GD_N && shown < 13; i++) { const Dy *d = &gdy[((gdn - i) % GD_N + GD_N) % GD_N]; if (!d->kind || i > gdn) continue; if (seen++ < skip) continue; int day = d->day / 24 + 1, hr = d->day % 24;
        if (d->kind == GD_AWARD) sprintf(s, "DAY %d %02d  AWARD: %s", day, hr, gachNm[d->nm % NACH]); else if (d->kind == GD_DAY) sprintf(s, "DAY %d %02d  GOODNIGHT. %d CARES TODAY", day, hr, d->nm); else sprintf(s, "DAY %d %02d  %s %s", day, hr, ggiv[d->nm & 31], gdyVerb[d->kind & 31]);
        rect(8, 22 + shown * 12, 240, 11, shown & 1 ? C15(8, 12, 24) : C15(5, 9, 21)); text_fit(12, 24 + shown * 12, 1, 232, d->kind == GD_PASS || d->kind == GD_MISS ? C15(31, 14, 12) : d->kind == GD_AWARD ? C15(31, 25, 6) : TXT, s); shown++; }
    if (!shown) { text_fit(12, 60, 1, 232, TXT, "NOTHING WRITTEN YET."); text_fit(12, 76, 1, 232, C15(20, 24, 28), "HATCH AN EGG AND LIFE GOES IN THE DIARY."); } }
static void gtop_award(void) {
    panel(4, 4, 248, 184, "Awards"); char s[80];
    for (int i = 0; i < NACH; i++) { int x = 8 + (i & 1) * 122, y = 22 + (i >> 1) * 13, got = ghas(i), sel = i == gsub; rect(x, y, 120, 12, sel ? C15(31, 25, 6) : PANB); rect(x + 1, y + 1, 118, 10, got ? C15(8, 22, 12) : PAN);
        text(x + 4, y + 2, 1, got ? C15(31, 31, 20) : C15(14, 18, 24), got ? "+" : "-"); text_fit(x + 14, y + 2, 1, 102, got ? TXT : C15(18, 22, 28), gachNm[i]); }
    rect(8, 154, 240, 22, C15(24, 16, 4)); sprintf(s, "%s: %s", gachNm[gsub % NACH], gachHow[gsub % NACH]); gwrapn(12, 156, 232, 2, TXT, s);
    sprintf(s, "REWARD +$%d%s   CARE SCORE %d", 50 + gsub * 10, ghas(gsub) ? "  (WON)" : "", gscore()); text_fit(12, 178, 1, 232, C15(31, 25, 6), s); }
static void gcell(int x, int y, int w, int h, int sel) { rect(x, y, w, h, sel ? C15(31, 31, 20) : PANB); rect(x + 1, y + 1, w - 2, h - 2, sel ? C15(24, 28, 31) : PAN); }
static void gtline(int x0, int y0, int x1, int y1) {   // a line of the tree: down from a parent, across, down to the child
    int ym = (y0 + y1) / 2, a = MIN(x0, x1), w = ABS(x1 - x0) + 1; u16 c = C15(14, 16, 24);
    rect(x0, y0, 1, ym - y0 + 1, c); rect(a, ym, w, 1, c); rect(x1, ym, 1, y1 - ym + 1, c); }
static const Toy *gtree_t;   // (D69) the toy whose tree is drawn: a line that is still an egg is drawn as its egg
static void gtree_card(const Ln *o, int x, int y, int hl) {   // one relative: a little LCD with its name under it
    if (!o || !o->id) return; Blob bb; memset(&bb, 0, sizeof bb); bb.on = 1; bb.stage = GS_ADULT; bb.sp = o->sp; bb.hun = bb.hap = 4; bb.col = o->col & 7;
    if (gtree_t && !(o->nm & 128)) for (int i = 0; i < GB_N; i++) { const Blob *e = &gtree_t->b[i]; if (e->on && e->id == o->id && e->stage == GS_EGG) { bb.stage = GS_EGG; bb.age = 3; bb.mut = e->mut; } }
    g_scene(&bb, 0, 0);
    int gone = (o->nm & 128) != 0; rect(x - 1, y - 1, 46, 22, hl ? C15(31, 25, 6) : C15(9, 7, 11)); rect(x, y, 44, 20, LCD_BG); gdots_draw(x + 6, y + 2, 1, gone ? C15(14, 14, 14) : gcolInk[o->col & 7]);
    text_fit(x, y + 23, 1, 44, gone ? C15(18, 18, 18) : TXT, ggiv[o->nm & 31]); if (gone) text(x + 37, y + 1, 1, C15(31, 31, 31), "+"); }
static void gtree_people(const Toy *t, int y);
static void gtop_tree(void) {
    Toy *t = gtt(); const Ln *me = &t->ln[gtree]; char b2[80]; gtree_t = t;
    if (!me->id) { text_fit(12, 60, 1, 232, TXT, "THIS PET HAS NO FAMILY RECORD YET."); text_fit(12, 76, 1, 232, C15(20, 24, 28), "EGGS FROM A PAIR START A TREE."); return; }
    static const int gpx[4] = { 6, 54, 158, 206 }; const int mx = 30, fx = 182, ux = 106, y0 = 24, y1 = 59, y2 = 94, y3 = 129;
    const Ln *m = gln_of(t, me->mom), *f = gln_of(t, me->dad), *gp[4] = { m ? gln_of(t, m->mom) : 0, m ? gln_of(t, m->dad) : 0, f ? gln_of(t, f->mom) : 0, f ? gln_of(t, f->dad) : 0 };
    int sib[4], ns = 0, kid[5], nk = 0;
    for (int i = 0; i < GLN && ns < 4; i++) { const Ln *o = &t->ln[i]; if (o->id && o->id != me->id && ((me->mom && o->mom == me->mom) || (me->dad && o->dad == me->dad))) sib[ns++] = i; }
    for (int i = 0; i < GLN && nk < 5; i++) { const Ln *o = &t->ln[i]; if (o->id && (o->mom == me->id || o->dad == me->id)) kid[nk++] = i; }
    for (int i = 0; i < 4; i++) if (gp[i]) gtline(gpx[i] + 22, y0 + 31, (i < 2 ? mx : fx) + 22, y1 - 1);
    if (m) gtline(mx + 22, y1 + 31, ux + 22, y2 - 1); if (f) gtline(fx + 22, y1 + 31, ux + 22, y2 - 1);
    for (int q = 0; q < nk; q++) gtline(ux + 22, y2 + 31, 8 + q * 48 + 22, y3 - 1);
    for (int i = 0; i < 4; i++) gtree_card(gp[i], gpx[i], y0, 0);
    gtree_card(m, mx, y1, 0); gtree_card(f, fx, y1, 0);
    { static const int sx[4] = { 58, 154, 10, 202 }; for (int q = 0; q < ns; q++) gtree_card(&t->ln[sib[q]], sx[q], y2, 0); }
    gtree_card(me, ux, y2, 1);
    for (int q = 0; q < nk; q++) gtree_card(&t->ln[kid[q]], 8 + q * 48, y3, 0);
    sprintf(b2, "%s THE %s %s  GEN %d%s", ggiv[me->nm & 31], gcolNm[me->col & 7], gspNm[me->sp % GSP_N], gdepth(t, me->id, 0) + 1, (me->nm & 128) ? "  (PASSED ON)" : "");
    text_fit(8, 162, 1, 240, TXT, b2);
    gtree_people(t, 171);
    if (t->mem) { sprintf(b2, "MEMORIAL: %d REMEMBERED. LAST: %s", t->mem, ggiv[t->memnm & 31]); text_fit(8, 180, 1, 240, C15(20, 24, 28), b2); } }
static void gtop_home(void) {
    static const u8 wall[6][3] = { { 255, 226, 150 }, { 150, 210, 240 }, { 245, 170, 200 }, { 170, 232, 200 }, { 60, 52, 110 }, { 240, 200, 90 } };
    Toy *t = gtt(); const u8 *w = wall[t->room % 6]; int dk = t->room % 6 == 4, d = t->deco; char s[40];
    panel(4, 4, 248, 184, groomNm[t->room % 6]);
    rect(8, 24, 240, 62, K8(w[0], w[1], w[2])); rect(8, 88, 240, 96, K8(w[0] * 6 / 10, w[1] * 6 / 10, w[2] * 6 / 10)); rect(8, 86, 240, 3, K8(w[0] / 2, w[1] / 2, w[2] / 2));
    for (int x = 8; x < 248; x += 24) rect(x, 88, 1, 96, K8(w[0] / 2, w[1] / 2, w[2] / 2));
    if (d & 1) { rect(20, 30, 34, 28, K8(255, 255, 255)); rect(22, 32, 30, 24, K8(140, 200, 255)); rect(36, 32, 2, 24, K8(255, 255, 255)); rect(22, 43, 30, 2, K8(255, 255, 255)); }
    if (d & 2) { rect(212, 72, 10, 14, K8(150, 90, 50)); for (int i = 0; i < 4; i++) rect(208 + i * 5, 52 + (i & 1) * 5, 6, 20, K8(60, 170, 70)); }
    if (d & 4) { rect(120, 60, 3, 28, K8(120, 100, 80)); rect(114, 50, 16, 11, K8(255, 230, 140)); }
    if (d & 8) { rect(70, 158, 120, 22, K8(220, 90, 110)); rect(80, 162, 100, 14, K8(250, 190, 100)); }
    if (d & 16) { rect(62, 30, 38, 26, K8(130, 90, 50)); rect(64, 32, 34, 22, K8(250, 240, 220)); for (int i = 0; i < 4; i++) rect(68 + i * 7, 38 + (i & 1) * 4, 5, 7, K8(230 - i * 30, 90 + i * 30, 120)); }
    if (d & 32) { rect(14, 140, 34, 22, K8(200, 130, 60)); rect(18, 136, 7, 5, K8(255, 100, 100)); rect(32, 134, 7, 7, K8(100, 160, 255)); rect(40, 138, 7, 4, K8(255, 220, 80)); }
    rect(156, 74, 90, 3, K8(120, 80, 50));   // the memorial shelf: the last of those who passed on
    { int shown = 0; for (int i = GLN - 1; i >= 0 && shown < 3; i--) { const Ln *o = &t->ln[i]; if (!o->id || !(o->nm & 128)) continue; Blob bb; memset(&bb, 0, sizeof bb); bb.on = 1; bb.stage = GS_ADULT; bb.sp = o->sp; bb.hun = bb.hap = 4; g_scene(&bb, 0, 0);
          int x = 160 + shown * 30; rect(x, 52, 26, 20, K8(255, 250, 235)); rect(x + 1, 53, 24, 18, K8(214, 222, 214)); gdots_draw(x - 3, 54, 1, C15(10, 10, 10)); rect(x + 10, 72, 6, 2, K8(250, 120, 160)); shown++; }
      if (!shown) text(166, 58, 1, dk ? C15(28, 28, 31) : C15(14, 10, 6), "MEMORIALS"); }
    int order[GB_N], n = 0; for (int g = 0; g < 100; g++) for (int i = 0; i < GB_N; i++) if (t->b[i].on && t->b[i].gen == g) order[n++] = i;   // the family: parents at the back, the young in front
    for (int j = 0; j < n; j++) { Blob *b = &t->b[order[j]]; int row = (j * 2 < n || n <= 2) ? 0 : 1, cnt = row ? n / 2 : (n <= 2 ? n : (n + 1) / 2), col = row ? j - (n + 1) / 2 : j;
        int x = 8 + (240 - cnt * 58) / 2 + col * 58 - 3, y = row ? 144 : 100; g_scene(b, ganim / 20 + j, 0); gdots_draw(x, y + (b->stage <= GS_CHILD ? 8 : 0), 2, dk ? C15(28, 28, 31) : gcolInk[b->col & 7]);
        if (b->mate) { rect(x + 44, y - 2, 6, 3, C15(31, 6, 6)); rect(x + 45, y + 1, 4, 2, C15(31, 6, 6)); } }
    if (!n) text(80, 130, 1, TXT, "THE HOME IS EMPTY");
    sprintf(s, "%s: %s", gdecoNm[gsub % 6], (d >> (gsub % 6)) & 1 ? "ON" : "OFF"); rect(10, 175, 100, 11, C15(8, 12, 24)); text(14, 177, 1, TXT, s);
    if (gmsg) { rect(112, 175, 134, 11, C15(8, 12, 24)); text_fit(116, 177, 1, 128, TXT, gmsg); } }
static void gtop_shop(void) {
    panel(4, 4, 248, 184, "Boreagotchi shop"); char s[48]; Toy *t = gtoy_of(ctl);
    for (int i = 0; i < 8; i++) { int a = i + 1, y = 24 + i * 15, sel = i == gsub; rect(8, y, 240, 14, sel ? C15(31, 25, 6) : PANB); rect(9, y + 1, 238, 12, PAN);
        if (i < 7) { text(14, y + 3, 1, TXT, gaccNm[a]); if (t && (t->accs & (1 << a))) text(190, y + 3, 1, C15(20, 31, 20), "OWNED"); else { sprintf(s, "$%d", gaccCost[a]); text(190, y + 3, 1, C15(31, 25, 6), s); } }
        else { text(14, y + 3, 1, TXT, "A NEW EGG"); text(190, y + 3, 1, C15(31, 25, 6), "$150"); } }
    sprintf(s, "YOUR CASH $%d", funds); text(12, 150, 1, C15(31, 25, 6), s); text_fit(12, 162, 1, 232, C15(20, 24, 28), "A hat is yours for good. Wear it from Dress.");
    if (gmsg) text_fit(12, 174, 1, 232, C15(31, 12, 10), gmsg); }
// ---- (D62) the title and attract screen, the hatching scene, the end of the day, the first-day tips, the people on the tree ----
static void gtop_title(void) {
    char s[48]; const char *tt = "BOREAGOTCHI!"; int y = 22 + ((ganim >> 5) & 1);
    panel(4, 4, 248, 184, 0);
    ctext(130, y + 2, 2, K8(20, 8, 40), tt); ctext(128, y, 2, C15(31, 26, 8), tt);
    ctext(128, 48, 1, C15(22, 26, 31), "THE POCKET PET THAT NEEDS YOU");
    for (int i = 0; i < 3; i++) { Blob b; memset(&b, 0, sizeof b); b.on = 1; b.stage = GS_ADULT; b.hun = b.hap = 4; b.sp = (u8)((ganim / 90 + i * 5 + 1) % GSP_N); b.col = (u8)((ganim / 90 + i) % 7 + 1);
        int x0 = 14 + i * 78, y0 = 66 + (i == 1 ? -4 : 0); g_scene(&b, ganim / 20 + i, 0); rect(x0 - 2, y0 - 2, 68, 36, K8(26, 10, 40)); rect(x0, y0, 64, 32, LCD_BG); gdots_draw(x0, y0, 2, gcolInk[b.col & 7]); ctext(x0 + 32, y0 + 40, 1, TXT, gspNm[b.sp % GSP_N]); }
    { Toy *m = gtoy_of(ctl); int c = m ? gcalling(m) : 0; if (c) sprintf(s, "%d OF YOUR PETS WANT YOU", c); else strcpy(s, "ALL QUIET. YOUR PETS ARE FINE");
      rect(28, 124, 200, 16, c ? C15(24, 5, 5) : C15(5, 18, 8)); ctext(128, 128, 1, TXT, s); }
    if ((ganim >> 5) & 1) ctext(128, 154, 1, TXT, "PRESS A OR TAP PLAY");
}
static void gtop_hatch(void) {
    Blob *c = gcb(); int h = ghT; char s[72], nm[40]; u16 ink = gcolInk[c->col & 7];
    panel(4, 4, 248, 184, "Boreagotchi");
    rect(14, 24, 228, 112, K8(26, 10, 40)); rect(18, 28, 220, 104, LCD_BG);
    if (h < 90) { Blob g = *c; g.stage = GS_EGG; g.age = (u16)(h > 30 ? 15 : 3); g_scene(&g, h / 6, 0); int sh = h < 24 ? 0 : ((h / 2) % 3 - 1) * (h < 70 ? 3 : 6); gdots_draw(26 + sh, 32, 6, ink); }
    else { g_scene(c, (h - 90) / 8, 0); gdots_draw(26, 32, 6, ink); int t = h - 90 > 70 ? 70 : h - 90;   // the baby, and bits of shell flying
        if (t < 60) for (int i = 0; i < 6; i++) { int dx = (i - 3) * (t + 6) / 3 + (i & 1), yy = 10 - t / 3 + (t * t) / (60 + i * 8), px = 26 + (16 + dx) * 6, py = 32 + yy * 6; if (px > 18 && px < 228 && py > 28 && py < 124) rect(px, py, 6, 6, ink); } }
    if (h >= 84 && h < 90) rect(18, 28, 220, 104, C15(31, 31, 31));
    if (h < 52) ctext(128, 144, 1, TXT, "SOMETHING IS MOVING INSIDE...");
    else if (h < 90) ctext(128, 144, 1, C15(31, 28, 12), "IT IS CRACKING!");
    else if (h < 112) ctext(128, 144, 1, TXT, "...");
    else { ctext(128, 142, 2, C15(31, 26, 8), "IT HATCHED!"); gname(c, nm); sprintf(s, "%s THE %s", ggiv[c->nm & 31], nm); text_fit(128 - MIN(tw(1, s), 240) / 2, 162, 1, 240, TXT, s);
        sprintf(s, "%s. GEN %d", gcolNm[c->col & 7], c->gen + 1); ctext(128, 175, 1, C15(22, 26, 31), s); }
}
static void gtop_day(void) {
    Toy *t = gtoy_of(ctl); char b[64]; static const char *const lb[3] = { "CARES", "GAMES", "HATCHED" }; int v[3] = { gsum_c, gsum_p, gsum_h };
    panel(4, 4, 248, 184, "Goodnight. Your day");
    for (int i = 0; i < 3; i++) { int x = 12 + i * 80; rect(x, 26, 76, 40, PANB); rect(x + 1, 27, 74, 38, PAN); sprintf(b, "%d", v[i]); ctext(x + 38, 31, 2, C15(8, 20, 31), b); ctext(x + 38, 52, 1, C15(20, 24, 28), lb[i]); }
    int n = 0, c = 0, sl = 0; if (t) for (int i = 0; i < GB_N; i++) { const Blob *p = &t->b[i]; if (!p->on) continue; n++; c += gneeds(p) && !(p->fl & GF_SLEEP); sl += (p->fl & GF_SLEEP) != 0; }
    if (t) for (int i = 0, j = 0; i < GB_N; i++) if (t->b[i].on) { int x = 12 + j * 30; g_scene(&t->b[i], ganim / 20 + i, 0); rect(x - 1, 74, 30, 18, PANB); rect(x, 75, 28, 16, LCD_BG); gdots_draw(x - 2, 75, 1, gcolInk[t->b[i].col & 7]); if (t->b[i].fl & GF_SLEEP) text(x + 18, 76, 1, C15(22, 22, 31), "Z"); j++; }
    gwrap(14, 100, 228, TXT, !n ? "NO PETS YET." : c ? "SOME PETS STILL NEED YOU. A LITTLE CARE MAKES A HAPPIER MORNING." : gsum_c ? "A GENTLE DAY. EVERYONE IS FED, HAPPY AND READY FOR BED." : "A QUIET DAY. THEY MISSED YOU A LITTLE. SAY HELLO TOMORROW.");
    sprintf(b, "%d PETS, %d ASLEEP, %d CALLING", n, sl, c); ctext(128, 134, 1, C15(22, 26, 31), b);
    ctext(128, 150, 1, C15(20, 24, 28), "IT GOES IN THE PET DIARY.");
}
static void gtree_people(const Toy *t, int y) {   // the people of the family on the tree page: partner, kids, parents (who is whose: the 'FY' block)
    int o = gsim_of(t); if (o < 0 || !fid[o]) return; char w[100]; int k = 0, nk = 0;
    for (const char *q = sname[o]; *q && *q != ' ' && k < 12; q++) w[k++] = *q; w[k++] = ':'; w[k] = 0;
    for (int j = 0; j < nsim && nk < 5; j++) { if (j == o || !fid[j]) continue;
        const char *r = fid[j] == fpt[o] ? " PARTNER " : (fpar[j][0] == fid[o] || fpar[j][1] == fid[o]) ? " KID " : (fid[j] == fpar[o][0] || fid[j] == fpar[o][1]) ? " PARENT " : 0; if (!r) continue;
        int l = (int)strlen(w); if (l + (int)strlen(r) + 10 > 92) break; strcpy(w + l, r); l += (int)strlen(r); for (const char *q = sname[j]; *q && *q != ' ' && l < 98; q++) w[l++] = *q; w[l] = 0; nk++; }
    if (nk) text_fit(8, y, 1, 240, C15(31, 25, 6), w);
}
static void gt_top(void) {
    scr = offbuf; char b[72]; rect(0, 0, 256, 192, K8(22, 12, 52));
    for (int i = 0; i < 24; i++) { int x = (i * 97 + 13) % 256, y = (i * 61 + 7) % 192; rect(x, y, ((ganim >> 3) + i) % 7 ? 1 : 2, ((ganim >> 3) + i) % 7 ? 1 : 2, C15(24, 24, 31)); }
    if (gview == GV_HOME) { gtop_home(); return; }
    if (gview == GV_TITLE) { gtop_title(); return; } if (gview == GV_HATCH) { gtop_hatch(); return; } if (gview == GV_DAY) { gtop_day(); return; }
    if (gview == GV_INTRO) { gtop_intro(); return; } if (gview == GV_GOALS) { gtop_goals(); return; }
    if (gview == GV_NONE || gview == GV_DEX || gview == GV_TREE || gview == GV_DIARY || gview == GV_AWARD || gview == GV_SHOP || gview == GV_BOX || gview == GV_GUIDE) panel(4, 4, 248, 184, "Boreagotchi");
    if (gview == GV_NONE) { text_fit(12, 40, 1, 228, TXT, "A Boreagotchi: a pet on a keychain."); text_fit(12, 56, 1, 228, TXT, "Eight creatures, 16 kinds, 64 forms."); text_fit(12, 72, 1, 228, TXT, "It grows up as you care for it."); sprintf(b, "The toy costs $%d.", GOTCHI_PRICE); text(12, 96, 1, C15(31, 25, 6), b); if (gmsg) text(12, 150, 1, C15(31, 12, 10), gmsg); return; }
    if (gview == GV_DEX) {
        sprintf(b, "BLOBDEX  %d OF %d   PAGE %d OF 4", gdex_count(), GSP_N * 4, gdexpage + 1); text(12, 26, 1, C15(31, 25, 6), b);
        for (int r = 0; r < 4; r++) { int sp = gdexpage * 4 + r, y = 36 + r * 34; text(10, y + 12, 1, TXT, gspNm[sp]);
            for (int f = 0; f < 4; f++) { int x = 70 + f * 46; rect(x - 1, y - 1, 45, 32, f == 0 ? C15(29, 22, 5) : f == 1 ? C15(18, 14, 8) : f == 2 ? C15(7, 23, 26) : C15(27, 8, 22)); rect(x, y, 43, 30, LCD_BG);
                if (gdex_has(sp, f)) { Blob bb; memset(&bb, 0, sizeof bb); bb.on = 1; bb.stage = GS_ADULT; bb.sp = (u8)sp; bb.form = (u8)f; bb.hun = bb.hap = 4; g_scene(&bb, 0, 0); gdots_draw(x + 5, y + 7, 1, LCD_INK); } else text(x + 18, y + 12, 1, C15(12, 14, 12), "?"); } }
        text(12, 174, 1, C15(20, 24, 28), "Care: SUPER, STAR. Snacks: CHILL."); return; }
    if (gview == GV_TREE) { gtop_tree(); return; }
    if (gview == GV_DIARY) { gtop_diary(); return; }
    if (gview == GV_AWARD) { gtop_award(); return; }
    if (gview == GV_SHOP) { gtop_shop(); return; }
    if (gview == GV_BOX) { Toy *bt = gtoy_of(gbx); int n = MIN(nsim, GT_MAX); if (gbx >= n) gbx = 0; bt = gtoy_of(gbx);
        text_fit(12, 28, 1, 232, C15(31, 25, 6), sname[gbx < nsim ? gbx : 0]);
        if (bt) { sprintf(b, "LV %d  %s", glevel(bt), glvNm[glevel(bt)]); text(12, 44, 1, TXT, b); sprintf(b, "%d pets   %d calling", gcount(bt), gcalling(bt)); text(12, 58, 1, gcalling(bt) ? C15(31, 14, 12) : C15(20, 28, 20), b); sprintf(b, "%d hatched   %d wins   %d playdates", bt->hatched, bt->wins, bt->plays); text_fit(12, 72, 1, 232, C15(20, 24, 28), b); }
        else text_fit(12, 44, 1, 232, C15(20, 24, 28), gbx == ctl ? "No toy yet: $5000 at a shop." : "No toy yet.");
        text(12, 96, 1, TXT, "Pets live on:"); text_fit(12, 108, 1, 232, C15(31, 25, 6), gclock_name());
        { int na = 0; for (int i = 0; i < NACH; i++) na += (gach >> i) & 1; sprintf(b, "CARE SCORE %d   AWARDS %d OF %d", gscore(), na, NACH); text_fit(12, 124, 1, 232, C15(31, 28, 12), b); }
        sprintf(b, "BLOBDEX %d OF %d", gdex_count(), GSP_N * 4); text(12, 140, 1, C15(31, 25, 6), b); rect(12, 154, 150, 5, C15(9, 7, 11)); rect(12, 154, 150 * gdex_count() / (GSP_N * 4), 5, C15(29, 22, 5));
        if (gmsg) text_fit(12, 168, 1, 232, C15(31, 12, 10), gmsg); return; }
    if (gview == GV_GUIDE) { const GPage *g = &gpg[gguide]; text_fit(12, 24, 1, 160, C15(31, 25, 6), g->title); sprintf(b, "PAGE %d OF %d", gguide + 1, GNP); text(176, 24, 1, C15(20, 24, 28), b);
        for (int i = 0; i < 3; i++) { rect(10, 44 + i * 44, 236, 38, K8(36, 16, 70)); rect(14, 54 + i * 44, 6, 6, C15(8, 20, 31)); gwrap(26, 48 + i * 44, 214, TXT, g->tip[i]); }
        for (int k = 0; k < GNP; k++) rect(128 - (GNP - 1) * 6 + k * 12 - 3, 176, 6, 6, k == gguide ? C15(8, 20, 31) : C15(9, 7, 11)); return; }
    gtop_pet();
}
typedef struct { s16 x, y, w, h; u16 key; u8 on; const char *lab; } GBtn;   // a button of the touch screen: where, what key it presses
#define GBN(a) ((int)(sizeof(a) / sizeof((a)[0])))
static const GBtn gb_box[] = { { 4, 136, 60, 22, PD_X, 1, "Open" }, { 66, 136, 60, 22, PD_SQ, 0, "Dex" }, { 128, 136, 60, 22, PD_R1, 0, "Awards" }, { 190, 136, 62, 22, PD_ST, 0, "Shop" },
    { 4, 160, 60, 24, PD_SEL, 0, "Help" }, { 66, 160, 60, 24, PD_L1, 0, "Time" }, { 128, 160, 124, 24, PD_O, 0, "Back" } };
static const GBtn gb_guide[] = { { 8, 150, 70, 34, PD_L, 0, "< Page" }, { 84, 150, 88, 34, PD_O, 1, "Back" }, { 178, 150, 70, 34, PD_R, 0, "Page >" } };
static const GBtn gb_dex[] = { { 8, 30, 70, 40, PD_L1, 0, "< Page" }, { 178, 30, 70, 40, PD_R1, 0, "Page >" }, { 78, 100, 100, 40, PD_O, 1, "Back" } };
static const GBtn gb_tree[] = { { 8, 40, 70, 60, PD_L, 0, "< Prev" }, { 178, 40, 70, 60, PD_R, 0, "Next >" }, { 78, 110, 100, 50, PD_O, 1, "Back" } };
static const GBtn gb_basket[] = { { 8, 154, 42, 30, PD_SQ, 0, "Dex" }, { 52, 154, 54, 30, PD_R1, 0, "Family" }, { 108, 154, 44, 30, PD_L1, 0, "Home" }, { 154, 154, 44, 30, PD_SEL, 0, "Help" }, { 200, 154, 48, 30, PD_O, 0, "Back" } };
static const GBtn gb_hand[] = { { 4, 130, 48, 26, PD_L1, 0, "< Pet" }, { 56, 130, 48, 26, PD_R1, 0, "Pet >" }, { 108, 130, 74, 26, PD_TR, 0, "Quick feed" }, { 186, 130, 66, 26, PD_SEL, 0, "Do next" }, { 4, 160, 70, 24, PD_O, 0, "Basket" } };
static const GBtn gb_handev[] = { { 4, 130, 48, 26, PD_L1, 0, "< Pet" }, { 56, 130, 48, 26, PD_R1, 0, "Pet >" }, { 108, 130, 74, 26, PD_TR, 0, "Quick feed" }, { 186, 130, 66, 26, PD_SEL, 0, "Do next" }, { 4, 160, 70, 24, PD_O, 0, "Basket" }, { 80, 158, 172, 28, PD_X, 1, "REACT!" } };
static const GBtn gb_card[] = { { 14, 40, 228, 64, PD_X, 1, "Next" }, { 14, 118, 228, 44, PD_ST, 0, "Skip the lessons" } };
static const GBtn gb_warm[] = { { 14, 24, 228, 92, PD_X, 1, "WARM THE EGG" }, { 14, 130, 110, 30, PD_ST, 0, "Skip lessons" } };
static const GBtn gb_intro[] = { { 8, 150, 108, 30, PD_ST, 0, "Skip" }, { 140, 150, 108, 30, PD_X, 1, "Next" } };
static const GBtn gb_feed[] = { { 6, 134, 78, 48, PD_L1, 0, "Lunch" }, { 88, 134, 80, 48, PD_X, 1, "Give" }, { 172, 134, 76, 48, PD_O, 0, "Back" } };
static const GBtn gb_play[] = { { 8, 150, 108, 30, PD_X, 1, "Play" }, { 140, 150, 108, 30, PD_O, 0, "Back" } };
static const GBtn gb_train[] = { { 8, 150, 108, 30, PD_X, 1, "Train" }, { 140, 150, 108, 30, PD_O, 0, "Back" } };
static const GBtn gb_goals[] = { { 58, 120, 140, 50, PD_O, 1, "Back" } };
static const GBtn gb_look[] = { { 8, 30, 114, 90, PD_L, 0, "< Left" }, { 134, 30, 114, 90, PD_R, 0, "Right >" }, { 140, 150, 108, 30, PD_O, 0, "Quit" } };
static const GBtn gb_copy[] = { { 88, 24, 80, 44, PD_U, 0, "Up" }, { 88, 100, 80, 44, PD_D, 0, "Down" }, { 8, 62, 76, 44, PD_L, 0, "Left" }, { 172, 62, 76, 44, PD_R, 0, "Right" }, { 140, 154, 108, 28, PD_O, 0, "Quit" } };
static const GBtn gb_catch[] = { { 8, 24, 112, 100, PD_L, 0, "< Left" }, { 136, 24, 112, 100, PD_R, 0, "Right >" }, { 140, 150, 108, 30, PD_O, 0, "Quit" } };
static const GBtn gb_hilo[] = { { 8, 30, 112, 90, PD_U, 1, "Higher" }, { 136, 30, 112, 90, PD_D, 0, "Lower" }, { 140, 150, 108, 30, PD_O, 0, "Quit" } };
static const GBtn gb_beat[] = { { 8, 24, 240, 110, PD_X, 1, "TAP ON THE BEAT" }, { 140, 150, 108, 30, PD_O, 0, "Quit" } };
static const GBtn gb_hop[] = { { 8, 24, 240, 110, PD_X, 1, "TAP TO JUMP" }, { 140, 150, 108, 30, PD_O, 0, "Quit" } };
static const GBtn gb_stack[] = { { 8, 24, 240, 110, PD_X, 1, "TAP TO DROP" }, { 140, 150, 108, 30, PD_O, 0, "Quit" } };
static const GBtn gb_friend[] = { { 8, 30, 60, 50, PD_L, 0, "<" }, { 188, 30, 60, 50, PD_R, 0, ">" }, { 8, 100, 112, 44, PD_X, 1, "Playdate" }, { 136, 100, 112, 44, PD_SQ, 0, "Battle" }, { 140, 154, 108, 28, PD_O, 0, "Back" } };
static const GBtn gb_stats[] = { { 8, 60, 114, 60, PD_X, 1, "Next" }, { 134, 60, 114, 60, PD_O, 0, "Back" } };
static const GBtn gb_dress[] = { { 8, 30, 60, 60, PD_L, 0, "< Prev" }, { 188, 30, 60, 60, PD_R, 0, "Next >" }, { 72, 30, 112, 60, PD_X, 1, "Wear / Buy" }, { 72, 110, 112, 50, PD_O, 0, "Back" } };
static const GBtn gb_home[] = { { 8, 112, 112, 36, PD_SQ, 0, "Walls" }, { 136, 112, 112, 36, PD_O, 1, "Back" } };
static const GBtn gb_award[] = { { 8, 40, 70, 60, PD_L, 0, "< Prev" }, { 178, 40, 70, 60, PD_R, 0, "Next >" }, { 78, 110, 100, 50, PD_O, 1, "Back" } };
static const GBtn gb_diary[] = { { 8, 70, 70, 60, PD_L, 0, "< Page" }, { 178, 70, 70, 60, PD_R, 0, "Page >" }, { 78, 70, 100, 60, PD_O, 1, "Back" } };
static const GBtn gb_more[] = { { 8, 148, 240, 34, PD_O, 0, "Back" } };
static const GBtn gb_shop[] = { { 8, 150, 240, 32, PD_O, 1, "Back" } };
static const GBtn gb_title[] = { { 14, 50, 228, 56, PD_X, 1, "Play" }, { 14, 120, 228, 40, PD_O, 0, "Leave" } };
static const GBtn gb_hatch[] = { { 8, 120, 100, 40, PD_ST, 0, "Skip" }, { 148, 120, 100, 40, PD_X, 1, "Say hello" } };
static const GBtn gb_day[] = { { 14, 80, 228, 60, PD_X, 1, "Continue" } };
static const GBtn gb_none[] = { { 14, 150, 228, 30, PD_O, 0, "Back" } };
static const GBtn *gbtns(int *n) {
#define GBT(v, a) case v: *n = GBN(a); return a;
    if (gview == GV_HAND) { int cp = gtut_page(); if (cp == 0 || cp == 6) { *n = GBN(gb_card); return gb_card; } if (cp == 1) { *n = GBN(gb_warm); return gb_warm; } if (gevK) { *n = GBN(gb_handev); return gb_handev; } }
    switch (gview) { GBT(GV_BOX, gb_box) GBT(GV_GUIDE, gb_guide) GBT(GV_DEX, gb_dex) GBT(GV_TREE, gb_tree) GBT(GV_BASKET, gb_basket) GBT(GV_HAND, gb_hand) GBT(GV_FEED, gb_feed) GBT(GV_PLAYPICK, gb_play) GBT(GV_LOOK, gb_look)
        GBT(GV_COPY, gb_copy) GBT(GV_CATCH, gb_catch) GBT(GV_HILO, gb_hilo) GBT(GV_FRIEND, gb_friend) GBT(GV_STATS, gb_stats) GBT(GV_DRESS, gb_dress) GBT(GV_HOME, gb_home) GBT(GV_AWARD, gb_award) GBT(GV_DIARY, gb_diary) GBT(GV_MORE, gb_more) GBT(GV_SHOP, gb_shop) GBT(GV_NONE, gb_none) GBT(GV_TITLE, gb_title) GBT(GV_HATCH, gb_hatch) GBT(GV_DAY, gb_day)
        GBT(GV_INTRO, gb_intro) GBT(GV_TRAIN, gb_train) GBT(GV_GOALS, gb_goals) GBT(GV_BEAT, gb_beat) GBT(GV_HOP, gb_hop) GBT(GV_STACK, gb_stack) GBT(GV_WHACK, gb_copy) }
#undef GBT
    *n = 0; return 0; }
static unsigned gt_bsig(void) { Toy *t = gtt(); return (unsigned)(gtree * 59 + gbx * 41 + gguide * 43 + o_gtime * 47 + gview * 31 + gbi * 7 + gisel * 11 + gsub * 13 + gdexpage * 17 + gmN * 19 + (gview == GV_COPY ? gmT >= 0 && gmT < gmN * 40 : 0) + (funds >= GOTCHI_PRICE) * 23 + (t->b[gbi].on) * 29
    + t->deco * 61 + t->room * 67 + t->accs * 71 + (int)(gach & 0xFFFFF) * 73 + funds / 10 * 79 + (gview == GV_HAND ? gtut_page() * 83 + gurgent(t, gbi) * 89 + ((t->b[gbi].fl & GF_CALL) != 0) * 97 + ((ganim >> 5) & 1) * 101 : 0) + gisub * 103 + (gevK ? 107 : 0)); }
static void gt_bottom(void) {
    scr = offbuf; char b[40]; rect(0, 0, 256, 192, K8(22, 12, 52)); rect(0, 0, 256, 16, K8(60, 30, 110)); ctext(128, 4, 1, K8(90, 230, 240), "BOREAGOTCHI");
    int nb; const GBtn *bt = gbtns(&nb); Toy *t = gtt(); int cp = gview == GV_HAND ? gtut_page() : -1;
    if (gview == GV_NONE) { chip(14, 60, 228, 50, funds >= GOTCHI_PRICE ? "Buy a toy $5000" : "Toy $5000 (not enough)", funds >= GOTCHI_PRICE); }
    else if (gview == GV_BOX) { int n = MIN(nsim, GT_MAX);
        for (int i = 0; i < n; i++) { Toy *bx = gtoy_of(i); int y = 20 + i * 14, sl = i == gbx; rect(8, y, 240, 13, sl ? C15(31, 31, 20) : PANB); rect(9, y + 1, 238, 11, sl ? C15(24, 28, 31) : PAN);
            text_fit(14, y + 3, 1, 90, sl ? C15(2, 7, 20) : TXT, sname[i]);
            if (bx) { for (int q = 0; q < GB_N; q++) { const Blob *p = &bx->b[q]; rect(108 + q * 10, y + 3, 8, 7, !p->on ? C15(9, 7, 11) : gneeds(p) ? C15(29, 6, 6) : p->stage == GS_EGG ? C15(28, 20, 4) : C15(6, 24, 8)); }
                if (gcalling(bx)) { sprintf(b, "%d!", gcalling(bx)); text(194, y + 3, 1, C15(29, 6, 6), b); } else text(194, y + 3, 1, C15(8, 22, 10), "ok"); sprintf(b, "L%d", glevel(bx)); text(224, y + 3, 1, sl ? C15(2, 7, 20) : C15(20, 24, 28), b); }
            else text(108, y + 3, 1, sl ? C15(8, 14, 24) : C15(14, 18, 24), i == ctl ? "no toy: $5000" : "no toy"); } }
    else if (gview == GV_BASKET) {
        for (int i = 0; i < GB_N; i++) { const Blob *c = &t->b[i]; int x = 8 + (i & 3) * 61, y = 20 + (i >> 2) * 66, s = i == gbi;
            rect(x, y, 59, 63, s ? C15(31, 31, 20) : PANB); rect(x + 1, y + 1, 57, 61, s ? C15(24, 28, 31) : PAN);
            if (c->on) { g_scene(c, ganim / 20 + i, 0); rect(x + 2, y + 3, 55, 28, LCD_BG); gdots_draw(x + 2 + 12, y + 3 + 6, 1, gcolInk[c->col & 7]); if (gneeds(c)) { rect(x + 46, y + 3, 11, 11, C15(29, 6, 6)); text(x + 50, y + 5, 1, TXT, "!"); }
                t_tight = 1; text_fit(x + 1, y + 33, 1, 57, s ? C15(2, 7, 20) : TXT, ggiv[c->nm & 31]); t_tight = 0; gname(c, b); gwrap(x + 3, y + 43, 54, s ? C15(8, 14, 24) : C15(20, 24, 28), b);
                for (int h = 0; h < 4; h++) rect(x + 5 + h * 12, y + 27, 9, 3, h < (c->bond + 12) / 25 ? C15(31, 8, 8) : C15(9, 7, 11)); }
            else { text(x + 13, y + 20, 1, s ? C15(2, 7, 20) : C15(14, 18, 24), s ? "NEW EGG" : "EMPTY"); text(x + 17, y + 36, 1, C15(31, 25, 6), "$150"); } } }
    else if (gview == GV_HAND && cp == 0) { text_fit(14, 24, 1, 228, C15(22, 26, 31), "Welcome! Tap Next to read on."); }
    else if (gview == GV_HAND && cp == 6) { text_fit(14, 24, 1, 228, C15(22, 26, 31), "Well done! Tap Next to finish."); }
    else if (gview == GV_HAND && cp == 1) { text_fit(14, 170, 1, 228, C15(22, 26, 31), "Tap the big button 5 times."); }
    else if (gview == GV_HAND) { int u = gurgent(t, gbi), ui = (u > 0 && u < 10) ? gurgIcon[u] : 99;
        for (int i = 0; i < GICN; i++) { int a = giconMap[i], x = 2 + (i % 7) * 36, y = 20 + (i / 7) * 52, s = i == gisel, lit = a == 6 && (gcb()->fl & GF_CALL), hint = i == ui && ((ganim >> 5) & 1);
            rect(x, y, 34, 50, s ? C15(31, 31, 31) : hint ? C15(31, 22, 6) : PANB); rect(x + 1, y + 1, 32, 48, s ? C15(26, 29, 31) : lit ? C15(31, 22, 6) : hint ? C15(14, 8, 4) : C15(11, 4, 14));
            u16 ic = s ? C15(16, 8, 4) : lit ? C15(4, 8, 14) : C15(24, 16, 30); for (int r = 0; r < 7; r++) for (int q = 0; q < 9; q++) if (gicon[a][r][q] == '#') rect(x + 4 + q * 3, y + 6 + r * 3, 3, 3, ic);
            t_tight = 1; text(x + 17 - tw(1, giconNm[a]) / 2, y + 38, 1, s ? C15(2, 7, 20) : TXT, giconNm[a]); t_tight = 0; }
        if (!gevK) text_fit(80, 166, 1, 172, C15(22, 26, 31), giconNm[giconMap[gisel % GICN]]); }
    else if (gview == GV_FEED) { Blob *c = gcb();
        for (int i = 0; i < NFOOD; i++) { int x = 6 + (i % 4) * 61, y = 20 + (i / 4) * 37, s = i == gsub; gcell(x, y, 59, 35, s);
            gwrap(x + 3, y + 3, 53, s ? C15(2, 7, 20) : TXT, gfoodNm[i]);
            text(x + 3, y + 25, 1, s ? C15(8, 14, 24) : C15(16, 20, 26), i < 6 ? "meal" : "snack");
            if (gfoodCost[i]) { sprintf(b, "$%d", gfoodCost[i]); text(x + 56 - tw(1, b), y + 25, 1, s ? C15(20, 12, 0) : C15(31, 25, 6), b); } else text(x + 56 - tw(1, "free"), y + 25, 1, s ? C15(8, 14, 24) : C15(18, 22, 28), "free");
            if ((c->taste >> 4) == i) { rect(x + 42, y + 22, 6, 3, C15(31, 6, 6)); rect(x + 50, y + 22, 6, 3, C15(31, 6, 6)); rect(x + 42, y + 25, 14, 3, C15(31, 6, 6)); rect(x + 45, y + 28, 8, 2, C15(31, 6, 6)); }
            else if ((c->taste & 15) == i) text(x + 33, y + 25, 1, C15(31, 10, 10), "YUCK"); } }
    else if (gview == GV_PLAYPICK) {
        for (int i = 0; i < NGAME; i++) { int x = 6 + (i & 3) * 61, y = 20 + (i >> 2) * 48, s = i == gsub; gcell(x, y, 59, 45, s);
            gwrap(x + 3, y + 4, 53, s ? C15(2, 7, 20) : TXT, ggameNm[i]);
            if (gbest[i]) { static const char *const gl_[5] = { "D", "C", "B", "A", "S" }; int gg = gbest[i] >= 90 ? 4 : gbest[i] >= 70 ? 3 : gbest[i] >= 50 ? 2 : gbest[i] >= 30 ? 1 : 0; sprintf(b, "%s %d", gl_[gg], gbest[i]); text(x + 3, y + 28, 1, s ? C15(20, 12, 0) : C15(31, 25, 6), b); }
            else text(x + 3, y + 28, 1, s ? C15(8, 14, 24) : C15(16, 20, 26), "new"); }
        gwrapn(10, 120, 236, 2, TXT, ggameHow[gsub & 7]); }
    else if (gview == GV_TRAIN) { Toy *tt = gtt(); const Xt *x = &tt->x[gbi];
        for (int i = 0; i < 3; i++) { int cx = 6 + i * 83, s = i == gsub; gcell(cx, 22, 79, 66, s); text(cx + 6, 28, 1, s ? C15(2, 7, 20) : TXT, gskNm[i]); rect(cx + 6, 44, 67, 7, K8(30, 14, 46)); rect(cx + 7, 45, 65 * x->sk[i] / 100, 5, C15(31, 24, 5)); sprintf(b, "%d / 100", x->sk[i]); text(cx + 6, 56, 1, s ? C15(8, 14, 24) : C15(20, 24, 28), b);
            static const char *const gg3[3] = { "HOP CATCH WHACK", "COPY STACK", "BEAT WHICH WAY" }; gwrap(cx + 6, 68, 70, s ? C15(8, 14, 24) : C15(16, 20, 26), gg3[i]); }
        gwrapn(10, 98, 236, 2, TXT, gwhyNm[gsub % 3]); gwrapn(10, 124, 236, 2, C15(22, 26, 31), "Each round costs energy. Good grades add skill."); }
    else if (gview == GV_GOALS) { text_fit(14, 30, 1, 228, C15(22, 26, 31), "Do all three goals today."); text_fit(14, 46, 1, 228, C15(22, 26, 31), "Keep a streak for bigger prizes."); text_fit(14, 62, 1, 228, C15(22, 26, 31), "New goals every morning."); }
    else if (gview == GV_INTRO) text_fit(14, 30, 1, 228, C15(22, 26, 31), gisub == 0 ? "Your new toy! Tap Next." : gisub == 1 ? "Opening the box..." : "Tap Next. Then warm the egg.");
    else if (gview == GV_BEAT || gview == GV_HOP || gview == GV_STACK) text_fit(14, 140, 1, 124, C15(22, 26, 31), gview == GV_BEAT ? "Tap on the beat." : gview == GV_HOP ? "Tap to jump the rocks." : "Tap to drop the block.");
    else if (gview == GV_WHACK) text_fit(10, 8, 1, 236, C15(22, 26, 31), "");
    else if (gview == GV_HOME) { for (int i = 0; i < 6; i++) { int x = 8 + (i % 3) * 82, y = 24 + (i / 3) * 40; sprintf(b, "%s %s", gdecoNm[i], (t->deco >> i) & 1 ? "ON" : "OFF"); chip(x, y, 78, 36, b, i == gsub); }
        text_fit(14, 156, 1, 228, C15(20, 24, 28), gsim_of(t) == ctl ? "Tap a thing to put it out or away." : "You can only look at their home."); }
    else if (gview == GV_MORE) { static const char *const mn[8] = { "Family home", "Pet diary", "Awards", "Blobdex", "Dress up", "Help pages", "Shop", "Toy box" }; for (int i = 0; i < 8; i++) chip(8 + (i & 1) * 124, 22 + (i >> 1) * 31, 116, 28, mn[i], i == gsub); }
    else if (gview == GV_SHOP) { Toy *mt = gtoy_of(ctl);
        for (int i = 0; i < 8; i++) { int x = 6 + (i % 4) * 61, y = 22 + (i / 4) * 62, s = i == gsub; gcell(x, y, 59, 58, s);
            t_tight = 1; text_fit(x + 3, y + 6, 1, 53, s ? C15(2, 7, 20) : TXT, i < 7 ? gaccNm[i + 1] : "NEW EGG"); t_tight = 0;
            if (i < 7) { if (mt && (mt->accs & (1 << (i + 1)))) text(x + 3, y + 30, 1, s ? C15(0, 16, 4) : C15(20, 31, 20), "OWNED"); else { sprintf(b, "$%d", gaccCost[i + 1]); text(x + 3, y + 30, 1, s ? C15(20, 12, 0) : C15(31, 25, 6), b); } }
            else text(x + 3, y + 30, 1, s ? C15(20, 12, 0) : C15(31, 25, 6), "$150"); } }
    else if (gview == GV_TITLE) text_fit(14, 30, 1, 228, C15(22, 26, 31), "Tap Play, or just wait.");
    else if (gview == GV_HATCH) text_fit(14, 30, 1, 228, C15(22, 26, 31), "Something is happening in the egg.");
    else if (gview == GV_DAY) text_fit(14, 40, 1, 228, C15(22, 26, 31), "Sleep well. The pets sleep too.");
    for (int i = 0; i < nb; i++) { const char *lab = bt[i].lab; int on = bt[i].on;
        if (gview == GV_BOX && i == 0 && !gtoy_of(gbx) && gbx == ctl) lab = "Buy";
        if (gview == GV_BOX && i == 5) lab = o_gtime == 0 ? "T:Game" : o_gtime == 1 ? "T:LIVE" : "T:Off";
        if (gview == GV_LOOK && i < 2) on = gmN == 0 && gmDir == (i ? 1 : -1);
        if (gview == GV_FEED && i == 0) on = gsub == 12;
        if (gview == GV_WHACK && i == 4) lab = "Quit";
        if (gview == GV_NONE) continue;
        chip(bt[i].x, bt[i].y, bt[i].w, bt[i].h, lab, on); }
}
static void gt_frame(int held, int down, int steps, int tx, int ty) {
    int pr = 0, td = down & KEY_TOUCH;
    if (down & KEY_LEFT) pr |= PD_L; if (down & KEY_RIGHT) pr |= PD_R; if (down & KEY_UP) pr |= PD_U; if (down & KEY_DOWN) pr |= PD_D; if (down & KEY_A) pr |= PD_X; if (down & KEY_B) pr |= PD_O; if (down & KEY_Y) pr |= PD_SQ; if (down & KEY_X) pr |= PD_TR; if (down & KEY_L) pr |= PD_L1; if (down & KEY_R) pr |= PD_R1;
    if (down & KEY_SELECT) pr |= PD_SEL; if (down & KEY_START) pr |= PD_ST;
    ghd = 0; if (held & KEY_LEFT) ghd |= PD_L; if (held & KEY_RIGHT) ghd |= PD_R; if ((held & KEY_TOUCH) && gview == GV_CATCH && ty >= 20 && ty < 140) ghd |= tx < 128 ? PD_L : PD_R;
    if (td) {
        int nb; const GBtn *bt = gbtns(&nb); for (int i = 0; i < nb; i++) if (hit(bt[i].x, bt[i].y, bt[i].w, bt[i].h, tx, ty)) pr |= bt[i].key;
        if (gview == GV_CATCH) pr &= ~(PD_L | PD_R);
        if (gview == GV_NONE && hit(14, 60, 228, 50, tx, ty)) pr |= PD_X;
        if (gview == GV_BOX) { int n = MIN(nsim, GT_MAX); for (int i = 0; i < n; i++) if (hit(8, 20 + i * 14, 240, 13, tx, ty)) { if (gbx == i) pr |= PD_X; gbx = i; } }
        else if (gview == GV_BASKET) { for (int i = 0; i < GB_N; i++) if (hit(8 + (i & 3) * 61, 20 + (i >> 2) * 66, 59, 63, tx, ty)) { if (gbi == i) pr |= PD_X; gbi = i; } }
        else if (gview == GV_HAND) { int cp = gtut_page(); if (cp != 0 && cp != 1 && cp != 6) for (int i = 0; i < GICN; i++) if (hit(2 + (i % 7) * 36, 20 + (i / 7) * 52, 34, 50, tx, ty)) { gisel = i; pr |= PD_ICON; } }
        else if (gview == GV_FEED) { for (int i = 0; i < NFOOD; i++) if (hit(6 + (i % 4) * 61, 20 + (i / 4) * 37, 59, 35, tx, ty)) { if (gsub == i) pr |= PD_X; gsub = i; } }
        else if (gview == GV_PLAYPICK) { for (int i = 0; i < NGAME; i++) if (hit(6 + (i & 3) * 61, 20 + (i >> 2) * 48, 59, 45, tx, ty)) { if (gsub == i) pr |= PD_X; gsub = i; } }
        else if (gview == GV_TRAIN) { for (int i = 0; i < 3; i++) if (hit(6 + i * 83, 22, 79, 66, tx, ty)) { if (gsub == i) pr |= PD_X; gsub = i; } }
        else if (gview == GV_HOME) { for (int i = 0; i < 6; i++) if (hit(8 + (i % 3) * 82, 24 + (i / 3) * 40, 78, 36, tx, ty)) { gsub = i; pr |= PD_X; } }
        else if (gview == GV_MORE) { for (int i = 0; i < 8; i++) if (hit(8 + (i & 1) * 124, 22 + (i >> 1) * 31, 116, 28, tx, ty)) { gsub = i; pr |= PD_X; } }
        else if (gview == GV_SHOP) { for (int i = 0; i < 8; i++) if (hit(6 + (i % 4) * 61, 22 + (i / 4) * 62, 59, 58, tx, ty)) { if (gsub == i) pr |= PD_X; gsub = i; } } }
    int og = gview, ob = gbi, oi = gisel, os = gsub, oe = gevK;
    if (!gt_update(pr, steps)) { mg_ui = 0; mg_ver++; return; }
    int anim = gview == GV_LOOK || gview == GV_COPY || gview == GV_CATCH || gview == GV_HILO || gview == GV_BEAT || gview == GV_HOP || gview == GV_STACK || gview == GV_WHACK || (gview == GV_INTRO && gisub == 1) || gpar_live() || gevK || gdanceT > 0 || gactT > 0;
    if (pr || og != gview || ob != gbi || oi != gisel || os != gsub || oe != gevK || anim || (gview == GV_HOME && (ganim % 20) < steps) || (gview == GV_HATCH && !(ganim & 3)) || (ganim & 15) < steps) mg_ver++;
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
static unsigned mg_bsig(void) { return mg_ui == 6 ? fight_bsig() : mg_ui == 3 ? (unsigned)(30 + hp * 5 + hsel * 11) : mg_ui == 4 ? gt_bsig() + 77 : mg_ui == 5 ? 5 : ar_bsig(); }
static void mg_top(void) { if (mg_ui == 6) fight_top(); else if (mg_ui == 3) hs_top(); else if (mg_ui == 4) gt_top(); else if (mg_ui == 5) gm_top(); else ar_top(); }
static void mg_bottom(void) { if (mg_ui == 6) fight_bottom(); else if (mg_ui == 3) hs_bottom(); else if (mg_ui == 4) gt_bottom(); else if (mg_ui == 5) gm_bottom(); else ar_bottom(); }
static void mg_frame(int held, int down, int steps, int tx, int ty) { if (mg_ui == 6) fight_frame(held, down, steps, tx, ty); else if (mg_ui == 3) hs_frame(held, down, steps, tx, ty); else if (mg_ui == 4) gt_frame(held, down, steps, tx, ty); else if (mg_ui == 5) gm_frame(down, tx, ty); else ar_frame(held, down, steps, tx, ty); }

// ==== the mini-games' saves: blocks after the farm's, each { magic, size, data }; a save from before has none (all at their start) ====
#define MG_HT 0x5448   // 'HT': the underworld's heat, kept boats and aircraft, logic
#define MG_AR 0x5241   // 'AR': the arcade's tickets, the fight record
#define MG_AS 0x5341   // 'SA': the arcade's high-score tables, first-play bits, games played
#define MG_SK 0x4B53   // 'SK': the board and the skate scores
#define MG_FY 0x5946   // 'FY': who is whose among the sims (ids, parents, partner): the Boreagotchi tree shows them
static void mg_reset(void) { has_board = on_board = 0; board_lift = 0; skate_score = skate_best = 0; arcade_tix = fight_wins = fight_champ = fight_best = 0; memset(fid, 0, sizeof fid); memset(fpar, 0, sizeof fpar); memset(fpt, 0, sizeof fpt); fid_next = 1; memset(ar_hs, 0, sizeof ar_hs); ar_pm = ar_plays = 0; mg_ui = 0; hs_heat = hs_day = hs_stolen = hs_logic = 0; gt_reset(); }
static int mg_pack(u8 *o) {
    int n = 0; u16 th[2]; s32 v[3];
    th[0] = MG_SK; th[1] = 12; v[0] = has_board; v[1] = (s32)skate_score; v[2] = (s32)skate_best; memcpy(o + n, th, 4); memcpy(o + n + 4, v, 12); n += 16;
    th[0] = MG_AR; th[1] = 16; { s32 w[4] = { arcade_tix, fight_wins, fight_champ, fight_best }; memcpy(o + n, th, 4); memcpy(o + n + 4, w, 16); n += 20; }
    th[0] = MG_AS; th[1] = sizeof ar_hs + 4; memcpy(o + n, th, 4); memcpy(o + n + 4, ar_hs, sizeof ar_hs); memcpy(o + n + 4 + sizeof ar_hs, &ar_pm, 2); memcpy(o + n + 6 + sizeof ar_hs, &ar_plays, 2); n += 8 + (int)sizeof ar_hs;
    th[0] = MG_HT; th[1] = 16; { s32 w[4] = { hs_heat, hs_day, hs_stolen, hs_logic }; memcpy(o + n, th, 4); memcpy(o + n + 4, w, 16); n += 20; }
    th[0] = MG_FY; th[1] = SIM_MAX * 4; memcpy(o + n, th, 4); memcpy(o + n + 4, fid, SIM_MAX); memcpy(o + n + 4 + SIM_MAX, fpar, SIM_MAX * 2); memcpy(o + n + 4 + SIM_MAX * 3, fpt, SIM_MAX); n += 4 + SIM_MAX * 4;
    n += gt_pack(o + n, MGBUF_N - n);
    return n; }
static int mg_unpack(const u8 *p, int left) {
    mg_reset(); int n = 0;
    while (n + 4 <= left) { u16 th[2]; memcpy(th, p + n, 4); if (n + 4 + th[1] > left) break;
        if (th[0] == MG_SK && th[1] == 12) { s32 v[3]; memcpy(v, p + n + 4, 12); has_board = v[0] & 1; skate_score = v[1] < 0 ? 0 : v[1]; skate_best = v[2] < 0 ? 0 : v[2]; }
        else if (th[0] == MG_AR && th[1] == 16) { s32 w[4]; memcpy(w, p + n + 4, 16); arcade_tix = CLAMP(w[0], 0, 9999); fight_wins = MAX(0, w[1]); fight_champ = w[2] & 1; fight_best = MAX(0, w[3]); }
        else if (th[0] == MG_AS && th[1] == sizeof ar_hs + 4) { memcpy(ar_hs, p + n + 4, sizeof ar_hs); memcpy(&ar_pm, p + n + 4 + sizeof ar_hs, 2); memcpy(&ar_plays, p + n + 6 + sizeof ar_hs, 2); }
        else if (th[0] == MG_HT && th[1] == 16) { s32 w[4]; memcpy(w, p + n + 4, 16); hs_heat = CLAMP(w[0], 0, 100); hs_day = w[1] & 255; hs_stolen = w[2] & 0xFFFF; hs_logic = MAX(0, w[3]); }
        else if (th[0] == MG_FY && th[1] == SIM_MAX * 4) { memcpy(fid, p + n + 4, SIM_MAX); memcpy(fpar, p + n + 4 + SIM_MAX, SIM_MAX * 2); memcpy(fpt, p + n + 4 + SIM_MAX * 3, SIM_MAX); for (int i = 0; i < SIM_MAX; i++) if (fid[i] >= fid_next) fid_next = (u8)(fid[i] + 1); if (!fid_next) fid_next = 1; }
        else if (th[0] == MG_GT) gt_unpack(p + n + 4, th[1]);
        else break;
        n += 4 + th[1]; }
    return n; }
#ifdef DSTEST
static void gt_tbase(int tut) {   // (D64) a toy with a grown-up (slot 0, wishing), a baby and a child; the handheld open on the first
    funds += 8000; gt_reset(); gbuy(ctl); Toy *t = gtoy_of(ctl); static const u8 stg[3] = { GS_ADULT, GS_BABY, GS_CHILD };
    for (int i = 0; i < 3; i++) { Blob *b = &t->b[i]; gfresh(b, i * 3 + 1); b->stage = stg[i]; b->form = (u8)(i == 0 ? 3 : 0); b->age = (u16)(i * 40); b->bond = (u8)(60 - i * 15); b->hun = 3; b->hap = 3; b->col = (u8)(i + 2); gnewid(t, b, 0, 0); gdex_see(b->sp, b->form); gxinit(t, i); t->x[i].nrg = 80; t->x[i].hyg = 70; }
    t->b[0].disc = 60; t->x[0].sk[0] = 40; t->x[0].sk[1] = 22; t->x[0].sk[2] = 63; t->x[0].wish = 1 | 8; t->cared = 40; if (!tut) gach |= 0x180000u; else { gach &= ~0x1F0000u; t->cared = 0; }
    gt_open_a(1); gview = GV_HAND; gti = (int)(t - gtoy); gbi = 0; gisel = 0; gmsgT = 0; gmsg = 0; ghpend = 0; gsum_pend = 0; }
static Toy *gt_ebase(void) {   // (D69) a toy with eight eggs of different kinds and colours (the last one rare); the handheld open on the first
    funds += 8000; gt_reset(); gbuy(ctl); Toy *t = gtoy_of(ctl);
    for (int i = 0; i < GB_N; i++) { if (i) gegg(t, i); Blob *b = &t->b[i]; b->stage = GS_EGG; b->sp = (u8)i; b->col = (u8)(i % 7 + 1); b->mut = (u8)(i == 7); b->age = 3; b->hun = b->hap = 4; gxinit(t, i); }
    t->cared = 40; gach |= 0x180000u; gt_open_a(1); gview = GV_HAND; gti = (int)(t - gtoy); gbi = 0; gisel = 0; gmsgT = 0; gmsg = 0; ghpend = 0; gsum_pend = 0; return t; }
static void mg_test(int sc) {
    if (sc >= 101 && sc <= 115) {   // (D69) the redrawn eggs (build with -DAUTO_SCENE=100, hold keys): 101-106 the six patterns, 107 the rare one, 108-110 the three crack stages, 111 basket, 112 home, 113 tree, 114 day card, 115 guide (eggs)
        Toy *t = gt_ebase(); Blob *b = &t->b[0];
        if (sc <= 106) { b->sp = (u8)(sc - 101); b->col = (u8)(sc - 101 + 1); }
        if (sc == 107) { b->mut = 1; b->col = 3; }
        if (sc >= 108 && sc <= 110) { b->sp = 3; b->col = 2; b->age = (u16)(sc == 108 ? 15 : sc == 109 ? 17 : 19); }
        if (sc == 111) { gview = GV_BASKET; } if (sc == 112) { gview = GV_HOME; gsub = 1; }
        if (sc == 113) { mg_test(57); Toy *u = gtt(); for (int j = 0; j < GLN; j++) if (u->ln[j].id == u->b[6].id) gtree = j; }
        if (sc == 114) { for (int i = 2; i < GB_N; i++) { Blob *p = &t->b[i]; if (i < 5) { p->stage = (u8)(i - 1); p->age = 40; } } gsum_c = 5; gsum_p = 2; gsum_h = 1; gview = GV_DAY; }
        if (sc == 115) { gt_open_a(3); gguide = 2; } }
    if (sc >= 73 && sc <= 79) { gt_tbase(0); Toy *t = gtt(); gbest[0] = 55; gbest[2] = 95; gbest[4] = 78; gbest[7] = 30; t->gp[0] = 3; t->gp[1] = 1; t->streak = 2; t->best = 4; t->ghist = 0x35; t->gflag = 0;
        if (sc == 73) { t->b[0].hun = 1; t->b[0].poop = 1; }
        if (sc == 74) { gsub = 1; gview = GV_FEED; } if (sc == 75) { gsub = 7; gview = GV_PLAYPICK; } if (sc == 76) { gsub = 2; gview = GV_TRAIN; } if (sc == 77) { ggoals_roll(t); gview = GV_GOALS; }
        if (sc == 78) { gsub = 0; gview = GV_STATS; } if (sc == 79) { gsub = 2; gview = GV_STATS; } }
    if (sc == 98) { funds += 8000; gt_reset(); gbuy(ctl); gt_open_a(1); }   // a toy fresh from the shop: the unboxing, then the first-day lessons
    if (sc >= 90 && sc <= 99 && sc != 98) { gt_tbase(0); gbest[7] = 30; Toy *t = gtt();
        if (sc == 90) { gsub = 1; gview = GV_STATS; } if (sc == 91) { gsub = 3; gview = GV_STATS; gdiary(GD_HATCH, t->b[0].nm); gdiary(GD_WISH, t->b[0].nm); gdiary(GD_TRAIN | (1 << 5), t->b[0].nm); gdiary(GD_BEST | (7 << 5), t->b[0].nm); }
        if (sc == 92) { gtrainMode = 0; gstart_game(4); } if (sc == 93) gstart_game(5); if (sc == 94) gstart_game(6); if (sc == 95) gstart_game(7);
        if (sc == 96) { gview = GV_INTRO; gisub = 0; } if (sc == 97) { gview = GV_INTRO; gisub = 2; gcb()->stage = GS_EGG; }
        if (sc == 99) { gevK = 1; gevT = 99999; gevCool = 99999; t->b[0].poop = 1; t->x[0].hyg = 20; } }
    if (sc == 42) { ag_testlot = 1; memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; gen_public(LOT_ARCADE, 0x5A3C91u); occ_ok = 0; funds += 5000; for (int q = 0; q < nsim; q++) { sims[q].x = sims[q].tx = sims[q].wx = (5 + q) * 16; sims[q].y = sims[q].ty = sims[q].wy = 7 * 16; sims[q].act = -1; } }
    if (sc == 49) { funds += 8000; gt_reset(); gbuy(ctl); Toy *t = gtoy_of(ctl); static const u8 stg[8] = { GS_EGG, GS_BABY, GS_CHILD, GS_TEEN, GS_ADULT, GS_ADULT, GS_ADULT, GS_ADULT };
        for (int i = 0; i < 7; i++) { Blob *b = &t->b[i]; gfresh(b, i * 3 + 1); b->stage = stg[i]; b->form = (u8)(i - 4 < 0 ? i & 1 : i - 4); b->age = (u16)(i * 11); b->bond = (u8)(i * 14); b->hun = (u8)(4 - (i == 2)); b->hap = (u8)(i == 3 ? 0 : 3); if (i == 1) b->poop = 2; if (i == 5) b->fl |= GF_SICK; gdex_see(b->sp, b->form); }
        for (int f = 0; f < 4; f++) { gdex_see(0, f); gdex_see(9, f); } t->xp = 120; gt_open(); }
    if (sc == 56 || sc == 57) { funds += 8000; gt_reset(); gbuy(ctl); Toy *t = gtoy_of(ctl); Blob *bs = t->b; static const u8 stg[8] = { GS_ADULT, GS_ADULT, GS_BABY, GS_CHILD, GS_TEEN, GS_ELDER, GS_EGG, GS_ADULT };
        for (int i = 0; i < 8; i++) { Blob *b = &bs[i]; gfresh(b, i * 2 + 1); b->stage = stg[i]; b->age = (u16)(i * 30); b->bond = 80; b->pers = (u8)i; b->col = (u8)i; b->mut = i == 2; b->acc = (u8)(i % 8); gnewid(t, b, 0, 0); }
        bs[0].mate = bs[1].id; bs[1].mate = bs[0].id; for (int i = 2; i < 5; i++) { bs[i].mom = bs[0].id; bs[i].dad = bs[1].id; gln_set(t, &bs[i], 0); } bs[6].mom = bs[1].id; bs[6].dad = bs[0].id; gln_set(t, &bs[6], 0);
        { Blob gm; memset(&gm, 0, sizeof gm); gm.on = 1; gm.sp = 3; gm.nm = 7; gm.col = 3; gnewid(t, &gm, 0, 0); gln_set(t, &gm, 1); bs[0].mom = gm.id; gln_set(t, &bs[0], 0); t->mem = 1; t->memnm = 7; t->memsp = 3; }
        for (int f = 0; f < 4; f++) gdex_see(0, f);
        if (nsim >= 3) { fam_id(0); fam_id(1); fam_id(2); fpt[0] = fid[1]; fpt[1] = fid[0]; fpar[2][0] = fid[0]; fpar[2][1] = fid[1]; }   // (D62) the people: sims 0 and 1 are a pair, sim 2 their kid
        gt_open_a(1); if (gview == GV_TITLE) gview = gafter; if (sc == 57) { gtree = 0; for (int j = 0; j < GLN; j++) if (t->ln[j].id == bs[2].id) gtree = j; gview = GV_TREE; } else gbi = 2; }
    if (sc == 48) { funds += 5000; hs_open(); }
    if (sc == 50) { funds += 8000; gt_reset(); gbuy(ctl); Toy *t = gtoy_of(ctl); for (int i = 0; i < 4; i++) { Blob *b = &t->b[i]; gfresh(b, i * 3 + 2); b->stage = (u8)(i ? GS_CHILD + i - 1 : GS_BABY); b->age = (u16)(i * 20); b->col = (u8)(i + 1); b->hun = 3; b->hap = (u8)(i == 2 ? 0 : 3); gnewid(t, b, 0, 0); }   // (D62) the title, the hatching and the day summary in turn
        t->cared = 40; ghpend = 1; gsum_pend = 1; gsum_c = 7; gsum_p = 2; gsum_h = 1; gach |= 0x180000u; gt_open_a(1); }
    if (sc == 51) { gt_testshop = 1; memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; gen_public(LOT_SHOP, 0x5A3C91u); occ_ok = 0; funds += 5000; { int dk = has_ch(0xBE); if (dk >= 0) { popup = 1; popup_i = dk; popup_n = obj_acts(dk, popup_lab, popup_code); } else say("No dock"); } for (int q = 0; q < nsim; q++) { sims[q].x = sims[q].tx = sims[q].wx = (7 + q) * 16; sims[q].y = sims[q].ty = sims[q].wy = 7 * 16; sims[q].act = -1; } }
    if (sc >= 52 && sc <= 55) { funds += 8000; gt_reset(); for (int q = 0; q < nsim && q < 3; q++) { gbuy(q); Toy *t = gtoy_of(q); for (int i = 0; i < (q ? 3 : 5); i++) { gegg(t, i); Blob *b = &t->b[i]; b->stage = (u8)(1 + (i + q) % 4); b->hun = (u8)(1 + i % 4); b->hap = (u8)(i == 2 ? 0 : 3); } t->xp = (u16)(q * 80); }
        for (int f = 0; f < 4; f++) gdex_see(0, f);
        if (sc == 52) gt_open_a(0); else if (sc == 53) { gt_open_a(3); gguide = 5; } else if (sc == 54) { gt_open_a(3); gguide = 3; } else { gt_open_a(1); o_gtime = 1; } if (gview == GV_TITLE) gview = gafter; }
    if (sc >= 58 && sc <= 59) { mg_test(56); Toy *t = gtt(); gbi = 0; gsub = 1; t->deco = 5; t->mem = 2; t->room = 1; gview = sc == 59 ? GV_HOME : GV_MORE; }   // (D56) More page, family home
    if (sc >= 43 && sc <= 47) { arcade_tix = 37; funds += 5000; ag_open(sc == 47 ? AG_PRIZES : sc - 42); }
    if (sc == 89) { arcade_tix = 37; funds += 5000; ag_open(AG_FIGHTER); }
    if (sc >= 80 && sc <= 88) { arcade_tix = 37; funds += 5000; ag_open(sc - 75); }   // (test builds) 60..71: games 5..13+   // (test builds) 40: this lot becomes the skate plaza, your Bore on the board at its west side
    if (sc == 40 || sc == 41) { memset(flr, 0, sizeof flr); memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl); nobj = 0; gen_public(LOT_SKATE, 1); occ_ok = 0; funds += 5000;
        for (int q = 0; q < nsim; q++) { sims[q].x = sims[q].tx = sims[q].wx = 0; sims[q].y = sims[q].ty = sims[q].wy = (5 + q) * 16; sims[q].act = -1; }
        sims[ctl].x = sims[ctl].tx = sims[ctl].wx = (sc == 41 ? 3 : 1) * 16; sims[ctl].y = sims[ctl].ty = sims[ctl].wy = (sc == 41 ? 3 : 7) * 16; fq4[ctl] = 1; has_board = 1; skate_on(); }
}
volatile int test_sres;   // (test builds) the result of the save/load test scenes
static void mg_test_save(int sc) {   // 71: fill in the arcade scores, the fight record, the family links and write the save; 72: read it back (run again with the same BORE_SAV file)
    if (sc == 71) { mg_test(73); for (int q = 0; q < 3; q++) gdiary(GD_TRAIN | (1 << 5), gtt()->b[0].nm); arcade_tix = 77; fight_wins = 5; fight_champ = 1; fight_best = 9; ar_hs[0][0] = 1234u | (1u << 17) | (2u << 22) | (3u << 27); ar_hs[5][1] = 777u; ar_hs[AG_NGAME - 1][2] = 4242u; ar_plays = 31; ar_pm = 5;
        fam_id(0); fam_id(1); fam_id(2); fpt[0] = fid[1]; fpt[1] = fid[0]; fpar[2][0] = fid[0]; fpar[2][1] = fid[1]; test_sres = sav_write() + 100; }
    if (sc == 72) { test_sres = sav_apply() + 200; gt_open_a(1); gview = GV_BASKET; gbi = 2;
#ifdef LOAD_VIEW
        gview = LOAD_VIEW; gbi = 0; gti = (int)(gtoy_of(ctl) - gtoy); gsub = LOAD_VIEW == GV_STATS ? 2 : 7;
#endif
    }   // (D64) and look at the family that came back
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
