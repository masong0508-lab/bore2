// the street traffic: cars drive the lanes of the streets along paths of straight pieces and arcs (a right turn, a left turn, a turn round at a
// dead end), always facing the way they go. They keep their lane, slow for turns and for the street they are on, queue behind each other, stop at
// a junction for whoever has the right of way and for people on foot, and never enter a junction they cannot leave. A car is a gate (the point
// on its lane line where the piece it is on begins), the piece, and how far along it is; the map window sliding along changes nothing: gates are
// whole-world tiles. The vehicles you leave parked stay where you left them.
#include "common.h"
#include "world.h"
#include "sim.h"
#include "gfx.h"
#include "items.h"
#include "save.h"
#include "traffic.h"
#include "bigmap.h"
#include "citizens.h"

#define NCAR 18
#define BLK BM_BLK
#define LNA (BM_LOT0 / 2 - 1)   // the street is five wide: a lane of two tiles each way and the line between. Cars drive on the lane lines (tile edges 1 and 4 of the strip), the middle of each lane
#define LNB (BM_LOT0 - 1)
#define HL 270                  // half a car's length (1/256 tile)
int traffic_level = 2, traffic_cheat;
// gx, gy: the gate; p: along the piece (1/8 of 1/256 tile); jx, jy: the junction square ahead or just left (its top-left tile; -1: a turn round); d: heading (0 south, 1 east, 2 north, 3 west);
// mv: the piece (0 straight, 1 right turn, 2 left turn, 3 turn round); v, vt: speed and the speed it wants (1/8 of 1/256 tile a vblank); nm: the piece it takes at the junction mouth (+4: it has the
// right of way); jg: gates to that mouth (0: none ahead); lim: the street's speed (0: no car here)
typedef struct { s16 gx, gy, p, jx, jy; u16 wph, wait; u8 d, mv, v, vt, type, col, nm, jg, lim; } Car;
static Car car[NCAR];
#ifdef TRAF_DBG
static u8 traf_g[NCAR];   // (the audit: how many cars each slot has had)
#endif
typedef struct { s32 x, y; s16 yaw; u8 on; } Park;   // (x, y: whole-world units; y grows south)
static Park park[VEH_N];
static u32 rs = 12345;
static int rnd(int n) { rs = rs * 1664525u + 1013904223u; return (int)((rs >> 12) % (u32)n); }
static const s8 DX[4] = { 0, 1, 0, -1 }, DY[4] = { 1, 0, -1, 0 };
enum { M_STR, M_RIGHT, M_LEFT, M_UT };
static const s16 ALEN[4] = { TILE, 402, 1608, 1206 }, AR[4] = { 0, 256, 1024, 384 };   // a piece's length, and an arc's radius (1/256 tile): a right turn is a quarter circle of one tile, a left one of four, a turn round a half circle of one and a half
static const int AK[4] = { 0, 1024 * 65536 / 402, 1024 * 65536 / 1608, 2048 * 65536 / 1206 };   // the turn of the car's yaw for each unit along the arc (1/65536 of yaw)
static const u8 TV[4] = { 255, 48, 64, 48 };   // the speed in each piece
#define V_ST 88     // a street, an avenue and a highway (1/8 of 1/256 tile a vblank: 11, 15 and 20)
#define V_AV 120
#define V_HW 160
static const u8 VT[8] = { 2, 2, 2, 3, 4, 5, 5, 2 };   // (the kinds of vehicle: mostly cars, some vans and a bus)
static const u8 tbl[5] = { 0, 3, 6, 11, 18 }, cheat_tbl[5] = { 0, 10, 30, 60, 120 };
#define R_IN 22     // cars appear 22 to 30 tiles from you (past the 17 they are drawn from) and go 34 away
#define R_OUT 34
static int bx_(void) { return world_wbx * BLK; }
static int by_(void) { return world_wby * BLK; }
static int road(int tx, int ty) { tx -= bx_(); ty -= by_(); if (tx < 0 || ty < 0 || tx >= MW || ty >= MH) return 0; return FL_IS_ROAD(floorMap[ty][tx]); }
static int cell_ok(int gx, int gy, int d) {   // is the lane there road, both its tiles: the piece from this gate on heading d
    int ax = gx - (d == 3), ay = gy - (d == 2);
    return (d & 1) ? road(ax, ay - 1) && road(ax, ay) : road(ax - 1, ay) && road(ax, ay);
}
static int target(void) {
#ifdef TRAF_OFF
    return 0;   // (test builds: no moving traffic, for shots of the lots)
#endif
#ifdef TRAF_TEST
    traffic_level = 3; traffic_cheat = TRAF_TEST;
#endif
 int n = tbl[traffic_level] + cheat_tbl[traffic_cheat];   // (both 0 to 4: main.c keeps them there)
  return n > NCAR ? NCAR : n; }
static int ptx(void) { return sim_tile_x(&sims[player]); }
static int pty(void) { return sim_tile_y(&sims[player]); }
static int honk_t;
void traffic_honk(void) { honk_t = 60; }   // cars near you give way for a moment
void traffic_clear(void) { for (int i = 0; i < NCAR; i++) car[i].lim = 0; }

// ---- where a car is: a pure function of its piece (window units, 1/256 tile) ----
static void posxy(int X, int Y, int d, int mv, int p, int *ox, int *oy, int *oyaw) {
    if (!mv) { *ox = X + DX[d] * p; *oy = Y + DY[d] * p; *oyaw = d * 1024; return; }
    int r = AR[mv], s = mv == M_RIGHT ? -1 : 1, len = ALEN[mv], q = p < len ? p : len, a = (d * 1024 + s * ((q * AK[mv]) >> 16)) & 4095, c0 = (d * 1024 + s * 1024) & 4095, c1 = (a + s * 1024) & 4095;
    *ox = X + ((r * (isin(c0) - isin(c1))) >> 12); *oy = Y + ((r * (icos(c0) - icos(c1))) >> 12); *oyaw = a;   // (the circle's centre is a radius to the side, the car a radius off it)
    if (p > len) { *ox += ((p - len) * isin(a)) >> 12; *oy += ((p - len) * icos(a)) >> 12; }   // (past the end of an arc: straight on)
}
static void pose(const Car *c, int p, int *x, int *y, int *yaw) { posxy((c->gx - bx_()) * TILE, (c->gy - by_()) * TILE, c->d, c->mv, p, x, y, yaw); }
static void ahead(const Car *c, int s, int *x, int *y, int *yaw) {   // where the car will be s on, by the way it means to go (before the junction mouth: on; after it: the piece it will take there)
    int p = (c->p >> 3) + s, d = c->d, g = 0, mv = c->mv;
    if (c->jg && p >= c->jg * TILE) { g = c->jg; p -= g * TILE; mv = c->nm & 3; }
    posxy((c->gx + DX[d] * g - bx_()) * TILE, (c->gy + DY[d] * g - by_()) * TILE, d, mv, p, x, y, yaw);
}

// ---- junctions: the squares at a block's corner where streets meet ----
static int jfind(int gx, int gy, int d, int *jx, int *jy) {   // is this gate, heading d, the mouth of a junction square (5 x 5 tiles)?
    int mx = gx % BLK, my = gy % BLK;
    switch (d) {
        case 1: if (my != LNB || mx) return 0; *jx = gx; *jy = gy - LNB; return 1;
        case 3: if (my != LNA || mx != BM_LOT0) return 0; *jx = gx - BM_LOT0; *jy = gy - LNA; return 1;
        case 0: if (mx != LNA || my) return 0; *jx = gx - LNA; *jy = gy; return 1;
        default: if (mx != LNB || my != BM_LOT0) return 0; *jx = gx - LNB; *jy = gy - BM_LOT0; return 1;
    }
}
static int jarms(int jx, int jy) {   // 0 if the square is not all road; else 16 and a bit for each way out (heading d) that has three cells of lane
    for (int y = 0; y < BM_LOT0; y++) for (int x = 0; x < BM_LOT0; x++) if (!road(jx + x, jy + y)) return 0;
    int a = 16;
    for (int d = 0; d < 4; d++) {
        int gx = jx + (d == 1 ? BM_LOT0 : d == 3 ? 0 : d == 0 ? LNA : LNB), gy = jy + (d == 0 ? BM_LOT0 : d == 2 ? 0 : d == 1 ? LNB : LNA);
        if (cell_ok(gx, gy, d) && cell_ok(gx + DX[d], gy + DY[d], d) && cell_ok(gx + 2 * DX[d], gy + 2 * DY[d], d)) a |= 1 << d;
    }
    return a;
}
// the tiles of a junction square (5 x 5, a bit each) a car sweeps on its way through, by heading d and piece m (straight, right, left): the car's box (400 x 230) walked along the piece
static const u32 JMK[4][3] = {
    { 0x0318c63u, 0x0000063u, 0x1cf1c63u }, { 0x1ff8000u, 0x0318000u, 0x077f318u },
    { 0x18c6318u, 0x18c0000u, 0x18c71e7u }, { 0x00003ffu, 0x0000318u, 0x0319fdcu } };
static u32 jmask(int d, int m) { return m > 2 ? 0 : JMK[d][m]; }
static int street_lim(int gx, int gy, int d) {   // how fast this street is driven: a highway between towns, an avenue (every fourth street of a planned town), a street
    BInfo b; int bx = (gx - (d == 3)) / BLK, by = (gy - (d == 2)) / BLK; bm_block(bx, by, &b);
    if (b.set == 255) return V_HW;
    if (bm_gen >= 7) { const Settle *s = bm_set(b.set); return (((d & 1) ? by - s->y : bx - s->x) & 3) ? V_ST : V_AV; }
    return ((d & 1) ? by : bx) % 4 ? V_ST : V_AV;
}
static void plan(Car *c) {   // a junction up to four gates ahead: pick the way through it now (straight 6, right 2, left 2 of those with a street)
    int d = c->d;
    for (int k = 1; k <= 4; k++) {
        int gx = c->gx + DX[d] * k, gy = c->gy + DY[d] * k, jx, jy;
        if (!jfind(gx, gy, d, &jx, &jy)) continue;
        int a = jarms(jx, jy), l = (d + 1) & 3, r = (d + 3) & 3;
        if (!(a & ((1 << l) | (1 << r)))) return;   // (nothing to the side: a plain street, or a dead end)
        int w[3] = { (a >> d) & 1 ? 6 : 0, (a >> r) & 1 ? 2 : 0, (a >> l) & 1 ? 2 : 0 }, q = rnd(w[0] + w[1] + w[2]), m = 0;
        while (q >= w[m]) q -= w[m++];
        c->jx = (s16)jx; c->jy = (s16)jy; c->jg = (u8)k; c->nm = (u8)m; return;
    }
}

// ---- what is in a car's way ----
typedef struct { s16 x, y, r; } Circ;   // a round patch of street that is taken (window units)
#define NOB 40
static s16 pd[20][2]; static u8 pdr[20]; static int npd;   // the people on foot (the player first): where, and how wide a berth (half, 1/256 tile)
static int body(Circ *o, int n, int x, int y, int yaw) { int sx = (isin(yaw) * 150) >> 12, sy = (icos(yaw) * 150) >> 12; o[n].x = x + sx; o[n].y = y + sy; o[n].r = 125; o[n + 1].x = x - sx; o[n + 1].y = y - sy; o[n + 1].r = 125; return n + 2; }   // (a car: a circle at each end)
static int gather(const Car *c, Circ *o) {   // the cars, the vehicles parked in the road and the people within reach of this car
    int n = 0, x0, y0, y_;
    pose(c, c->p >> 3, &x0, &y0, &y_);
    for (int j = 0; j < NCAR && n < NOB - 2; j++) { const Car *k = &car[j]; if (k == c || !k->lim || ABS(k->gx - c->gx) + ABS(k->gy - c->gy) > 13) continue;
        int x, y, yaw; pose(k, k->p >> 3, &x, &y, &yaw); n = body(o, n, x, y, yaw); }
    for (int t = 0; t < VEH_N && n < NOB - 2; t++) { const Park *p = &park[t]; if (!p->on) continue;
        int x = p->x - bx_() * TILE, y = p->y - by_() * TILE; if (ABS(x - x0) + ABS(y - y0) > 3200) continue;
        n = body(o, n, x, y, p->yaw); }
    for (int i = 0; i < npd && n < NOB; i++) if (ABS(pd[i][0] - x0) + ABS(pd[i][1] - y0) < 3200) { o[n].x = pd[i][0]; o[n].y = pd[i][1]; o[n++].r = pdr[i] * 2; }
    return n;
}
static int hit(const Circ *o, int n, int x, int y, int r) { for (int i = 0; i < n; i++) { int dx = o[i].x - x, dy = o[i].y - y, rr = o[i].r + r; if (dx * dx + dy * dy < rr * rr) return 1; } return 0; }
static const s16 PS[9] = { 0, 150, 270, 310, 390, 490, 610, 770, 1030 };   // how far ahead of its middle a car looks (the bumper is at 270), and the speed it may keep when something is there
static const u8 PV[9] = { 0, 0, 0, 0, 40, 64, 88, 112, 160 };
static int dstop(const Car *c) { return c->jg * TILE - (c->p >> 3) - ((c->nm & 3) == M_UT ? 0 : TILE + HL); }   // how far its bumper has to go to the line it stops at (a turn round: the middle of the car to where it turns)
#ifdef TRAF_DBG
int traf_gw[40];
#define GWHY(ci, w) (traf_gw[ci] = (w))
#else
#define GWHY(ci, w) ((void)0)
#endif
static int grant(const Car *c, int ci, const Circ *o, int n) {   // may it go: nobody on a crossing path has the road or has waited longer, and the way out is clear
    int m = c->nm & 3;
    if (c->jx >= 0) { u32 mk = jmask(c->d, m);
        for (int j = 0; j < NCAR; j++) { const Car *k = &car[j]; if (j == ci || !k->lim || k->jx != c->jx || k->jy != c->jy || k->d == c->d || !(k->jg || (k->nm & 4))) continue;
            if ((mk & jmask(k->d, k->nm & 3)) && ((k->nm & 4) || (k->wait > c->wait && dstop(k) < 200))) { GWHY(ci, 100 + j); return 0; } } }
    int dE = c->jg * TILE - (c->p >> 3), L = m ? ALEN[m] : BM_LOT0 * TILE;
    for (int t = 1; t <= 3; t++) { int x, y, yaw; ahead(c, dE + (t < 3 ? L * t / 3 : L + 330), &x, &y, &yaw); if (hit(o, n, x, y, 110)) { GWHY(ci, 10 + t); return 0; } }
    return 1;
}
static void ctl(Car *c, int ci) {   // how fast it wants to go: the street's speed, less for a turn, a junction it has no right to yet, whatever is ahead
    Circ o[NOB]; int n = gather(c, o), vt = c->lim;
    if (c->mv && vt > TV[c->mv]) vt = TV[c->mv];
    if (c->jg) {
        int m = c->nm & 3, dE = c->jg * TILE - (c->p >> 3);
        if (m && vt > TV[m] + (dE >> 2)) vt = TV[m] + (dE >> 2);
        if (!(c->nm & 4)) { int dS = dstop(c);
            if (dS < 440 && grant(c, ci, o, n)) c->nm |= 4;
            else { int v = dS > 24 ? (dS - 24) >> 1 : 0; if (v < vt) vt = v; } }
    }
    { int x, y, yaw; ahead(c, 0, &x, &y, &yaw); int near = hit(o, n, x, y, 1900); c->nm = (u8)((c->nm & ~8) | (near ? 0 : 8));   // (nothing within eight tiles: it looks again in two turns, not one)
      if (near) for (int k = 0; k < 9; k++) { ahead(c, PS[k], &x, &y, &yaw); if (hit(o, n, x, y, 110)) { if (PV[k] < vt) vt = PV[k]; break; } } }
    if (honk_t > 0 && !c->mv && !(c->nm & 4) && ABS(c->gx - bx_() - ptx()) + ABS(c->gy - by_() - pty()) < 9) vt = 0;   // (a horn: they hold still a moment)
    c->vt = (u8)vt;
}

// ---- cars come and go ----
#ifdef TRAF_DBG
#define KILL(c, why) ((c)->lim = 0, traf_why = (why))
extern int traf_why;
#else
#define KILL(c, why) ((c)->lim = 0)
#endif
static int off(const Car *c, int ex) {   // away: out of the map window's reach, or far from you
    int tx = c->gx - bx_(), ty = c->gy - by_();
    int dd = ABS(tx - ptx()) + ABS(ty - pty());
    return dd > R_OUT + ex || (dd > 19 && (tx < 4 || ty < 4 || tx >= 4 * BLK - 4 || ty >= 4 * BLK - 4));   // (the window is four blocks, 72 tiles, of the 80: past its edge there is no street, a car near you turns round there)
}
static void arrive(Car *c) {   // the end of a piece: the next gate, and the next piece
    int d = c->d, m = c->mv;
    c->p -= ALEN[m] << 3;
    if (m == M_STR) { c->gx += DX[d]; c->gy += DY[d]; }
    else if (m == M_UT) { int l = (d + 1) & 3; c->gx += DX[l] * 3; c->gy += DY[l] * 3; c->d = (d + 2) & 3; c->nm = 0; }
    else { int n = m == M_LEFT ? (d + 1) & 3 : (d + 3) & 3, r = m == M_LEFT ? 4 : 1; c->gx += (DX[d] + DX[n]) * r; c->gy += (DY[d] + DY[n]) * r; c->d = (u8)n; }
    c->mv = M_STR; d = c->d;
    int gx = c->gx, gy = c->gy;
    if (off(c, 0) || !cell_ok(gx, gy, d)) { KILL(c, off(c, 0) ? 1 : 2); return; }
    if (!(gx % BLK) || !(gy % BLK) || m) c->lim = (u8)street_lim(gx, gy, d);
    if ((c->nm & 4) && !c->jg && c->jx >= 0 && (gx < c->jx - 1 || gx > c->jx + 6 || gy < c->jy - 1 || gy > c->jy + 6)) c->nm = 0;   // (out of the junction: others may go)
    if (c->jg) { if (!--c->jg) { c->mv = c->nm & 3; c->nm |= 4; } return; }
    if (cell_ok(gx + 3 * DX[d], gy + 3 * DY[d], d)) {
        if (!cell_ok(gx + 4 * DX[d], gy + 4 * DY[d], d)) { c->jx = c->jy = -1; c->jg = 1; c->nm = M_UT; }   // (the street ends: turn round at the next gate, where three cells are left: the turn needs them)
        else plan(c);
    } else if (cell_ok(gx + 2 * DX[d], gy + 2 * DY[d], d)) c->mv = M_UT; else KILL(c, 3);
}
static void step_car(Car *c, int ci, int tk) {
    if (!((tk + ci) & 3)) {
        if (off(c, 0) || (c->wait > 1200 && off(c, -12))) { KILL(c, 4); return; }   // (a car held up for twenty seconds, out of sight: gone)
        if (c->jg || !(c->nm & 8) || !((tk + ci) & 4)) ctl(c, ci); }
    int v = c->v, vt = c->vt;
    v = v < vt ? MIN(v + 2, vt) : MAX(v - (v > vt + 24 ? 8 : 4), vt);
    c->v = (u8)v; c->wait = v ? 0 : c->wait + (c->wait < 60000); c->wph = (c->wph + ((v * 3) >> 1)) & 4095;   // (a wheel at the reference radius turns 4096 in 352 units)
    c->p += v;
    if (c->p >= ALEN[c->mv] << 3) arrive(c);
}
static int spawn(void) {
    int px = ptx(), py = pty(), i;
    for (i = 0; i < NCAR && car[i].lim; i++) ;
    if (i == NCAR) return 0;
    for (int k = 0; k < 6; k++) {
        int tx = px + rnd(61) - 30, ty = py + rnd(61) - 30, dd = ABS(tx - px) + ABS(ty - py), d = rnd(4), gx, gy;
        if (dd < R_IN || dd > R_IN + 8 || tx < 8 || ty < 8 || tx >= 4 * BLK - 8 || ty >= 4 * BLK - 8) continue;
        tx += bx_(); ty += by_();
        if (d & 1) { if (ty % BLK >= BM_LOT0) continue; gx = tx; gy = ty - ty % BLK + (d == 1 ? LNB : LNA); }
        else { if (tx % BLK >= BM_LOT0) continue; gx = tx - tx % BLK + (d == 0 ? LNA : LNB); gy = ty; }
        if (gx % BLK < BM_LOT0 + 2 && gy % BLK < BM_LOT0 + 2) continue;   // (not in a junction)
        int ok = 1; for (int j = -2; j < 6 && ok; j++) ok = cell_ok(gx + DX[d] * j, gy + DY[d] * j, d);
        if (!ok) continue;
        Car *c = &car[i]; memset(c, 0, sizeof *c); c->gx = (s16)gx; c->gy = (s16)gy; c->d = (u8)d; c->lim = (u8)street_lim(gx, gy, d); c->jx = c->jy = -1;
        { Circ o[NOB]; int n = gather(c, o); int x, y, yaw; ahead(c, 450, &x, &y, &yaw); if (hit(o, n, x, y, 1100)) c->lim = 0; }   // (nobody within a car and a half or so of it)
        if (!c->lim) continue;
        c->type = VT[rnd(8)]; c->col = (u8)rnd(6);
        c->vt = c->v = c->lim >> 1; plan(c);
#ifdef TRAF_DBG
        traf_g[i]++;
#endif
        return 1;
    }
    return 0;
}
static void people(void) {   // everyone on foot the cars must give way to: your household and the townsfolk
    npd = 0;
    for (int i = 0; i < nsims && npd < 8; i++) { const Sim *s = &sims[i]; if (s->away || s->lvl) continue; pd[npd][0] = s->x; pd[npd][1] = -s->z; pdr[npd++] = i == player ? 75 : 45; }
    { s16 xy[24]; int m = citizens_walkers(xy, 12, sims[player].x, -sims[player].z);
      for (int i = 0; i < m; i++) { pd[npd][0] = xy[2 * i]; pd[npd][1] = -xy[2 * i + 1]; pdr[npd++] = 45; } }
    for (int i = 0; i < npd; i++) { int tx = pd[i][0] >> 8, ty = pd[i][1] >> 8; if (tx >= 0 && ty >= 0 && tx < MW && ty < MH && floorMap[ty][tx] >= FL_ZEB_A && floorMap[ty][tx] <= FL_ZEB_B) pdr[i] = 110; }   // (on a crossing: a wide berth)
}
void traffic_update(int steps) {
    if (world_zone != ZONE_CITY || world_wbx < 0) return;
    int want = target(), n = 0; static u8 tk; if (honk_t > 0) honk_t -= steps;
    for (int i = 0; i < NCAR; i++) if (car[i].lim) n++;
    if (!(tk & 4)) people();   // (the people on foot move slowly: not every frame)
    for (int s = 0; s < steps; s++) {
        tk++;
        for (int i = 0; i < NCAR; i++) if (car[i].lim) step_car(&car[i], i, tk);
        if (n < want) n += spawn();
    }
    for (int i = 0; i < NCAR && n > want; i++) if (car[i].lim && off(&car[i], -14)) { KILL(&car[i], 5); n--; }   // (the dial turned down: the cars out of sight go)
}
static void put(const Car *c) { int x, y, yaw; pose(c, c->p >> 3, &x, &y, &yaw); items_draw_vehicle(x, -y, yaw, 0, c->type, c->col, c->wph); }
#ifdef TRAF_DBG
int traf_why; unsigned traf_dbg_jmask(int d, int m) { return jmask(d, m); }
int traf_dbg_gen(int i) { return traf_g[i]; }   // (why the last car went: the audit tells the ones that go in sight)
int traf_dbg_info(int i, char *out) { const Car *c = &car[i]; return sprintf(out, "d%d mv%d p%d jg%d nm%d jx,jy %d,%d v%d vt%d wait%d lim%d g %d,%d", c->d, c->mv, c->p >> 3, c->jg, c->nm, c->jx, c->jy, c->v, c->vt, c->wait, c->lim, c->gx - bx_(), c->gy - by_()); }
int traf_dbg_get(int i, int *x, int *y, int *yaw, int *type, int *v) { const Car *c = &car[i]; if (!c->lim) return 0; int xx, yy; pose(c, c->p >> 3, &xx, &yy, yaw); *x = xx + bx_() * TILE; *y = yy + by_() * TILE; *type = c->type; *v = c->v; return 1; }
#endif
#ifdef VEH_SHOT
int veh_shot_f;
#ifndef VEH_SHOT_YAW
#define VEH_SHOT_YAW 3072
#endif
extern void items_draw_bus(int x, int z, int yaw, int y);
#endif
// ---- parked cars (world version 7): a car stands on some homes' drives, worked out from the town plan (nothing is stored: the same lot always has the same car) ----
typedef struct { s16 x, z, yaw; u8 t, c; } PCar;
static PCar pcar[20]; static int pcar_n, pcar_key[6] = { -1 };
extern int bm_edit_stamp;
static void pcars_build(int cbx, int cby) {
    pcar_n = 0;
    for (int wy = MAX(0, cby - 1); wy <= MIN(3, cby + 1); wy++) for (int wx = MAX(0, cbx - 1); wx <= MIN(3, cbx + 1); wx++) {
        int gx = world_wbx + wx, gy = world_wby + wy; BInfo b; LotPlan q;
        bm_block(gx, gy, &b); bm_lot_plan(gx, gy, &b, &q);
        if (q.type == HT_NONE || !q.drv || pcar_n >= 20) continue;
        u32 h = bm_hash(gx, gy, 0xCA85u); if ((h & 7) >= 5) continue;                          // (five homes in eight)
        int n = q.face == LF_NORTH, y0 = n ? 22 - q.y1 : q.y0, y1 = n ? 22 - q.y0 : q.y1, wy1 = n ? 22 - q.wy0 : q.wy1;
        int garage = q.wx1 || q.type == HT_RANCH, top = q.wx1 ? wy1 + 1 : q.type == HT_RANCH ? y1 + 1 : y0;   // (as the generator's drive: to the garage's front, or beside the house)
        if (18 - top < 4) continue;
        int c = garage ? top * TILE + (TILE * 15) / 8 : 18 * TILE - (TILE * 5) / 2;               // (centre row in 1/256 tiles, seen from the south: against the garage, or by the street)
        if (n) c = 23 * TILE - c;
        PCar *p = &pcar[pcar_n++];
        p->x = (s16)((wx * BM_BLK + q.drv + 1) * TILE); p->z = (s16)-(wy * BM_BLK * TILE + c);
        p->yaw = (s16)(((h >> 8) & 3) == 0 ? (n ? 2048 : 0) : (n ? 0 : 2048));                      // (mostly nose in)
        p->t = VT[(h >> 12) & 7]; p->c = (u8)((h >> 16) % 6u);
    }
}
static void pcars_draw(void) {
    if (bm_gen < 7) return;
    int px = ptx(), py = pty(), cbx = CLAMP(px / BLK, 0, 3), cby = CLAMP(py / BLK, 0, 3);
    if (pcar_key[0] != world_wbx || pcar_key[1] != world_wby || pcar_key[2] != cbx || pcar_key[3] != cby || pcar_key[4] != bm_edit_stamp || pcar_key[5] != bm_gen) {
        pcar_key[0] = world_wbx; pcar_key[1] = world_wby; pcar_key[2] = cbx; pcar_key[3] = cby; pcar_key[4] = bm_edit_stamp; pcar_key[5] = bm_gen; pcars_build(cbx, cby); }
    int shown = 0;
    for (int i = 0; i < pcar_n && shown < 5; i++) { const PCar *p = &pcar[i];
        if (ABS(p->x / TILE - px) + ABS(-p->z / TILE - py) > 13) continue;
        items_draw_vehicle(p->x, p->z, p->yaw, 0, p->t, p->c, 0); shown++; }
}
void traffic_draw(void) {
    if (world_zone != ZONE_CITY || world_wbx < 0) return;
#ifdef VEH_SHOT
    if (veh_shot_f) { const Sim *s = &sims[player]; int fx = isin(2600), fz = icos(2600), rx = fz, rz = -fx;   // (test builds: three cars a row, two rows, the bus, laid out across the default view)
        for (int t = 0; t < 6; t++) { int a = t < 3 ? 250 : 950, b = ((t % 3) - 1) * 560;
            items_draw_vehicle(s->x + ((fx * a + rx * b) >> 12), s->z + ((fz * a + rz * b) >> 12), VEH_SHOT_YAW, 0, t, veh_colour(t), veh_shot_f * 40); }
        items_draw_bus(s->x + ((fx * 1750 + rx * 300) >> 12), s->z + ((fz * 1750 + rz * 300) >> 12), VEH_SHOT_YAW, 0); }
#endif
    int px = ptx(), py = pty(), shown = 0;
    for (int i = 0; i < NCAR && shown < 14; i++) { const Car *c = &car[i]; if (!c->lim) continue;
        if (ABS(c->gx - bx_() - px) + ABS(c->gy - by_() - py) > 18) continue; put(c); shown++; }
    for (int t = 0; t < VEH_N; t++) { const Park *p = &park[t]; if (!p->on) continue;
        int x = p->x - bx_() * TILE, y = p->y - by_() * TILE;
        if (ABS(x / TILE - px) + ABS(y / TILE - py) > 18) continue;
        items_draw_vehicle(x, -y, p->yaw, 0, t, veh_colour(t), 0); }
    pcars_draw();
}
void traffic_park(int t, int x, int z, int yaw) {
    if (t < 0 || t >= VEH_N || world_zone != ZONE_CITY || world_wbx < 0) return;
    Park *p = &park[t]; p->on = 1; p->x = x + bx_() * TILE; p->y = -z + by_() * TILE; p->yaw = yaw & 4095;
}
int traffic_parked(int t) { return t >= 0 && t < VEH_N && park[t].on; }
int traffic_park_near(int t, int x, int z) {
    if (!traffic_parked(t)) return 0;
    Park *p = &park[t]; int dx = p->x - (x + bx_() * TILE), dy = p->y - (-z + by_() * TILE);
    if (ABS(dx) > 3 * TILE || ABS(dy) > 3 * TILE) return 0;
    p->on = 0; return 1;
}
void traffic_save(void) { for (int t = 0; t < VEH_N; t++) { sv_u8(park[t].on); sv_u32((u32)park[t].x); sv_u32((u32)park[t].y); sv_u16(park[t].yaw); } }
void traffic_load(int ver) {
    traffic_clear();
    for (int t = 0; t < VEH_N; t++) { park[t].on = 0; if (ver >= 19) { park[t].on = ld_u8() & 1; park[t].x = (s32)ld_u32(); park[t].y = (s32)ld_u32(); park[t].yaw = ld_u16(); } }
}
