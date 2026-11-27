// trafsim: the moving traffic (src/traffic.c) run headless on a normal computer, and audited.
//   make -C ports/host/traffic            builds ./trafsim (the game's traffic.c itself, with a floor made as src/world.c makes it)
//   ./trafsim SEED GEN [LEVEL] [STEPS] [WINDOWS] [-v]   runs WINDOWS windows (each a town, or a highway, of that world) of STEPS vblanks each
//                                                      and counts every way the traffic can look wrong (see the list printed at the end)
// The floor: the street strips of every block of the 4 x 4 window, as gen_block0 paints them (asphalt, the line, the edges, the crosswalks, a
// river's bridge); nothing else is needed to know where a car may be. People: a few walkers who stroll the pavements and cross anywhere.
// The audit looks at the pose the game draws (traf_dbg_get): where each car is, which way it faces, how fast it really moves.
#include <stdio.h>
#include <assert.h>
#include "traffic.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "common.h"
#include "bigmap.h"
#include "world.h"
#include "sim.h"
#undef sprintf
#undef snprintf
int bsprintf(char *o, const char *f, ...) { va_list a; va_start(a, f); int n = vsprintf(o, f, a); va_end(a); return n; }
int bsnprintf(char *o, unsigned cap, const char *f, ...) { va_list a; va_start(a, f); int n = vsnprintf(o, cap, f, a); va_end(a); return n; }
void boot(const char *s) { (void)s; }
void loading(const char *w, int p) { (void)w; (void)p; }
int ram8 = 0, dbg_miss[4];
void *xalloc(size_t n) { return calloc(1, n); }
void world_gh_reset(void) {}

// ---- what traffic.c needs of the rest of the game ----
u8 floorMap[MH][MW];
int world_zone = ZONE_CITY, world_wbx = -1, world_wby = -1;
Sim sims[MAX_SIMS]; int nsims = 1, player = 0, veh_on = 0;
int sim_tile_x(const Sim *s) { return s->x / TILE; }
int sim_tile_y(const Sim *s) { return (-s->z) / TILE; }
int veh_colour(int t) { return t % 6; }
void sv_u8(int v) { (void)v; } void sv_u16(int v) { (void)v; } void sv_u32(unsigned v) { (void)v; }
int ld_u8(void) { return 0; } int ld_u16(void) { return 0; } unsigned ld_u32(void) { return 0; }
void items_draw_vehicle(int x, int z, int yaw, int y, int type, int ci, int wph) { (void)x; (void)z; (void)yaw; (void)y; (void)type; (void)ci; (void)wph; }
// the walking people the traffic must stop for (the townsfolk's own list in the game: citizens_walkers)
#define NPED 10
static int ped_x[NPED], ped_y[NPED], ped_dx[NPED], ped_dy[NPED], ped_n;   // (window units, 1/256 tile)
int citizens_walkers(s16 *xy, int max, int cx, int cz) { int n = 0; for (int i = 1; i < ped_n && n < max; i++) { xy[2 * n] = (s16)ped_x[i]; xy[2 * n + 1] = (s16)(-ped_y[i]); n++; } return n; }
#ifndef TRAF_SRC
#define TRAF_SRC "../../../src/traffic.c"
#endif
#include TRAF_SRC

// ---- the floor ----
static int blk_roads_(int bx, int by) { BInfo nb; bm_block(bx, by, &nb); int mg = nb.lot != LOT_RIVER && (nb.road & BR_MEGA), mpart = BR_PART(nb.road), rd = nb.road & 7; if (mg) { if (mpart & 1) rd &= ~2; if (mpart & 2) rd &= ~1; } return rd; }
static void gbox(int x0, int y0, int x1, int y1, int f) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if (x >= 0 && y >= 0 && x < MW && y < MH) floorMap[y][x] = (u8)f; }
static int river_br_(const BInfo *b) { return bm_gen < 7 || ((b->road & BR_MEGA) ? (b->road & 1) : (b->road & 2)); }
#define WB 4
static void make_floor(void) {
    memset(floorMap, FL_GRASS, sizeof floorMap);
    for (int wy = 0; wy < WB; wy++) for (int wx = 0; wx < WB; wx++) {
        int bx = world_wbx + wx, by = world_wby + wy, ox = wx * BM_BLK, oy = wy * BM_BLK; BInfo b; bm_block(bx, by, &b);
        int g7 = bm_gen >= 7 && b.set != 255 && b.terrain != BT_SEA;
        int mg = b.lot != LOT_RIVER && (b.road & BR_MEGA), mpart = BR_PART(b.road), rd = b.road & 7; if (mg) { if (mpart & 1) rd &= ~2; if (mpart & 2) rd &= ~1; }
        const int SW = BM_LOT0, SC = SW / 2;
        if (g7 && (b.road & 4)) gbox(ox, oy, ox + SW - 1, oy + SW - 1, FL_ASPHALT);
        if (rd & 1) { gbox(ox, oy, ox + BM_BLK - 1, oy + SW - 1, FL_ASPHALT); gbox(ox + SW, oy + SC, ox + BM_BLK - 1, oy + SC, FL_LINE_H); }
        if (rd & 2) { gbox(ox, oy, ox + SW - 1, oy + BM_BLK - 1, FL_ASPHALT); gbox(ox + SC, oy + SW, ox + SC, oy + BM_BLK - 1, FL_LINE_V); }
        if ((rd & 3) == 1) gbox(ox, oy + SC, ox + SW - 1, oy + SC, FL_LINE_H);
        if ((rd & 3) == 2) gbox(ox + SC, oy, ox + SC, oy + SW - 1, FL_LINE_V);
        int east = (rd & 1) ? (blk_roads_(bx + 1, by) & 2) : 0, south = (rd & 2) ? (blk_roads_(bx, by + 1) & 1) : 0;
        if (rd & 1) { int xs = (rd & 2) ? 6 : 0, xe = east ? BM_BLK - 2 : BM_BLK - 1;
            gbox(ox + xs, oy, ox + xe, oy, FL_EDGE_N); gbox(ox + xs, oy + SW - 1, ox + xe, oy + SW - 1, FL_EDGE_S);
            if (rd & 2) gbox(ox + 5, oy, ox + 5, oy + SW - 1, FL_ZEB_A);
            if (east) gbox(ox + BM_BLK - 1, oy, ox + BM_BLK - 1, oy + SW - 1, FL_ZEB_A); }
        if (rd & 2) { int ys = (rd & 1) ? 6 : 0, ye = south ? BM_BLK - 2 : BM_BLK - 1;
            gbox(ox, oy + ys, ox, oy + ye, FL_EDGE_W); gbox(ox + SW - 1, oy + ys, ox + SW - 1, oy + ye, FL_EDGE_E);
            if (rd & 1) gbox(ox, oy + 5, ox + SW - 1, oy + 5, FL_ZEB_B);
            if (south) gbox(ox, oy + BM_BLK - 1, ox + SW - 1, oy + BM_BLK - 1, FL_ZEB_B); }
        if (b.set != 255 && b.terrain != BT_SEA) {
            if (!g7 || (rd & 1)) gbox(ox + BM_LOT0, oy + BM_LOT0, ox + BM_BLK - 1, oy + BM_LOT0, FL_PAVING);
            if (!g7 || (rd & 2)) gbox(ox + BM_LOT0, oy + BM_LOT0, ox + BM_LOT0, oy + BM_BLK - 1, FL_PAVING); }
        if (b.lot == LOT_RIVER) {   // the river's water and walks, and the bridge over them (lot_river)
            int vert = b.road & BR_MEGA, br = river_br_(&b); const int SH = BM_LOT0 - 3, NB = BM_BLK - SH;
            for (int d = 3; d < NB; d++) { int fl = d <= 4 ? FL_PAVING : d <= 11 ? FL_WATER : d <= 13 ? FL_PAVING : FL_GRASS;
                for (int e = (br ? 3 : -SH); e < NB; e++) { int x = vert ? ox + d + SH : ox + e + SH, y = vert ? oy + e + SH : oy + d + SH; floorMap[y][x] = (u8)fl; } }
            if (br) for (int e = -SH; e < 3; e++) for (int d = 5; d <= 11; d++) { int x = vert ? ox + d + SH : ox + e + SH, y = vert ? oy + e + SH : oy + d + SH; floorMap[y][x] = FL_ASPHALT; }
        }
        if (g7) { if (blk_roads_(bx, by + 1) & 1) for (int x = ox + BM_LOT0; x < ox + BM_BLK; x++) floorMap[oy + BM_BLK - 1][x] = FL_PAVING;
                  if (blk_roads_(bx + 1, by) & 2) for (int y = oy + BM_LOT0; y < oy + BM_BLK; y++) floorMap[y][ox + BM_BLK - 1] = FL_PAVING; }
    }
}
static int isroad(int tx, int ty);
static int junction_square(int tx, int ty);
static int isroad(int tx, int ty) { return tx >= 0 && ty >= 0 && tx < MW && ty < MH && FL_IS_ROAD(floorMap[ty][tx]); }
static int walkable(int tx, int ty) { if (tx < 20 || ty < 20 || tx >= 52 || ty >= 52) return 0; int f = floorMap[ty][tx]; return f == FL_PAVING || FL_IS_ROAD(f); }

// ---- the people ----
static unsigned prs = 99;
static int prnd(int n) { prs = prs * 1664525u + 1013904223u; return (int)((prs >> 12) % (unsigned)n); }
static void peds_init(int px, int py) {   // 0 is the player, standing at (px, py); the rest wander
    ped_n = NPED;
    for (int i = 0; i < NPED; i++) { int tx = px, ty = py; for (int k = 0; k < 60; k++) { int x = px + prnd(25) - 12, y = py + prnd(25) - 12; if (walkable(x, y)) { tx = x; ty = y; break; } }
        ped_x[i] = tx * TILE + TILE / 2; ped_y[i] = ty * TILE + TILE / 2; ped_dx[i] = ped_dy[i] = 0; }
    ped_x[0] = px * TILE + TILE / 2; ped_y[0] = py * TILE + TILE / 2;
}
static int ped_wait[NPED];
static void peds_step(int steps_moving_player) {
    for (int i = (steps_moving_player ? 0 : 1); i < ped_n; i++) {
        if (ped_wait[i] > 0) { ped_wait[i]--; continue; }
        if ((ped_x[i] % TILE) == TILE / 2 && (ped_y[i] % TILE) == TILE / 2) {   // on a tile centre: pick a way (mostly on), or stop a while to look about
            int tx = ped_x[i] / TILE, ty = ped_y[i] / TILE, ok[4], n = 0; static const int DXs[4] = { 0, 1, 0, -1 }, DYs[4] = { 1, 0, -1, 0 };
            for (int d = 0; d < 4; d++) { ok[d] = walkable(tx + DXs[d], ty + DYs[d]); n += ok[d]; }
            if (!n) continue;
            int cur = -1; for (int d = 0; d < 4; d++) if (ped_dx[i] == DXs[d] && ped_dy[i] == DYs[d]) cur = d;
            int d; if (cur >= 0 && ok[cur] && prnd(100) < 75) d = cur; else { do d = prnd(4); while (!ok[d]); }
            if (prnd(100) < 5) { ped_wait[i] = 20 + prnd(100); ped_dx[i] = ped_dy[i] = 0; continue; }
            ped_dx[i] = DXs[d]; ped_dy[i] = DYs[d];
        }
        ped_x[i] += ped_dx[i] * 4; ped_y[i] += ped_dy[i] * 4;   // (a person walks about 0.7 tile a second)
    }
}
static void sims_sync(void) { sims[0].x = ped_x[0]; sims[0].z = -ped_y[0]; sims[0].away = 0; sims[0].lvl = 0; nsims = 1; }

// ---- the audit ----
enum { V_JUMP, V_TURNSNAP, V_SLIDE, V_OFFROAD, V_LANE, V_OVERLAP, V_STUCK10, V_STUCK30, V_PEDHIT, V_POPIN, V_POPOUT, V_UTURN_NOTEND, V_INBOX, V_SPEEDSTREET, V_BACKWARD, V_N };
static const char *vname[V_N] = { "teleport (moved more than 0.14 tile in one vblank)", "sudden turn (more than 5 degrees in one vblank)", "sideways slide (faces off its way by 25 degrees)",
    "off the road (a corner of the car outside the street)", "off its lane (more than 0.3 tile from the lane line, or the wrong side)", "cars overlapping (vblanks with any pair overlapping)",
    "stopped 10 s or more (cars)", "stopped 30 s or more (cars)", "a person inside a car", "car appears in view (within 17 tiles)", "car vanishes in view (within 17 tiles)",
    "u-turn away from a dead end", "stopped inside a junction square 6 s or more (cars)", "above the street's speed", "moves backward" };
static long viol[V_N], viol_total[V_N];
#define MAXC 64
typedef struct { int on, x, y, yaw, type, v; int px, py, pyaw, still; int stopped_flag, inbox_t, hist_y, hist_n; int yawacc, yawt; int al, sx, sy, syaw, rot, ov, gen, ut; } AC;
static AC ac[MAXC];
static long cars_seen, car_steps, steps_run, in_view_steps; static int max_cars_view;
static int g_curwin; static int g_verbose, g_tc = -1, g_t0, g_t1, g_win = -1;
static double mean_speed_sum; static long mean_speed_n;
static void obb(int x, int y, int yaw, double *cx, double *cy) {   // the car's four corners (1/256 tiles): 1.05 tiles long, 0.45 wide, round the pose
    const double OHL = 270, OHW = 112; double s = sin(yaw * 2 * M_PI / 4096), c = cos(yaw * 2 * M_PI / 4096);   // (forward is (s, c) in x, y)
    double fx = s, fy = c, rx = c, ry = -s;
    for (int k = 0; k < 4; k++) { double a = (k & 1) ? OHL : -OHL, b = (k & 2) ? OHW : -OHW; cx[k] = x + fx * a + rx * b; cy[k] = y + fy * a + ry * b; }
}
static int sat(const double *ax, const double *ay, const double *bx, const double *by) {   // do two rectangles overlap
    for (int pass = 0; pass < 2; pass++) {
        const double *px = pass ? bx : ax, *py = pass ? by : ay;
        for (int e = 0; e < 2; e++) { double nx = py[(e ? 2 : 1)] - py[0], ny = -(px[(e ? 2 : 1)] - px[0]);   // an edge's normal
            double amin = 1e30, amax = -1e30, bmin = 1e30, bmax = -1e30;
            for (int k = 0; k < 4; k++) { double d = ax[k] * nx + ay[k] * ny; if (d < amin) amin = d; if (d > amax) amax = d; d = bx[k] * nx + by[k] * ny; if (d < bmin) bmin = d; if (d > bmax) bmax = d; }
            if (amax <= bmin || bmax <= amin) return 0; }
    }
    return 1;
}
extern int traf_dbg_gen(int) __attribute__((weak));
static void audit_step(int step, int ptx, int pty) {
    static double cx[MAXC][4], cy[MAXC][4]; int act[MAXC], na = 0;
    int inview = 0;
    for (int i = 0; i < NCAR; i++) {
        int x, y, yaw, type, v, on = traf_dbg_get(i, &x, &y, &yaw, &type, &v);
        AC *a = &ac[i];
        int tx = (x - bx_() * TILE) / TILE, ty = (y - by_() * TILE) / TILE, dist = on ? ABS(tx - ptx) + ABS(ty - pty) : 0;
        if (g_verbose && on && (step & 7) == 0) { char b_[200]; traf_dbg_info(i, b_); int ut = strstr(b_, "mv3") != 0; if (ut && !a->ut) printf("  UTURN abs tile %d,%d (block %d,%d tile %d,%d) window %d step %d\n", tx + bx_(), ty + by_(), (tx + bx_()) / 18, (ty + by_()) / 18, (tx + bx_()) % 18, (ty + by_()) % 18, g_curwin, step); a->ut = ut; }
        if (on && a->on && traf_dbg_gen && traf_dbg_gen(i) != a->gen) { if (dist <= 17) viol[V_POPIN]++; cars_seen++; a->px = x; a->py = y; a->pyaw = yaw; a->still = 0; a->inbox_t = 0; a->al = 1; a->rot = 0; a->gen = traf_dbg_gen(i); }
        if (on && !a->on) { if (traf_dbg_gen) a->gen = traf_dbg_gen(i); cars_seen++; if (step > 0 && dist <= 17) { viol[V_POPIN]++; if (g_verbose) printf("  popin car %d at tile %d,%d dist %d step %d\n", i, tx, ty, dist, step); }
            a->px = x; a->py = y; a->pyaw = yaw; a->still = 0; a->inbox_t = 0; a->yawacc = 0; a->yawt = 0; }
        if (!on && a->on) { extern int traf_why; int dd = ABS((a->x - bx_() * TILE) / TILE - ptx) + ABS((a->y - by_() * TILE) / TILE - pty); if (dd <= 17) { viol[V_POPOUT]++; if (g_verbose) printf("  popout car %d at dist %d step %d why %d car tile %d,%d player %d,%d\n", i, dd, step, traf_why, (a->x - bx_() * TILE) / TILE, (a->y - by_() * TILE) / TILE, ptx, pty); } }
        if (!on) { a->on = 0; continue; }
        car_steps++; if (dist <= 17) inview++;
        if (i == g_tc && g_win == g_curwin && step >= g_t0 && step <= g_t1) { extern int traf_dbg_info(int, char *); char b[200]; traf_dbg_info(i, b); { extern int traf_gw[]; printf("  trace %d step %d gw%d [%s] x,y %.2f,%.2f yaw %d\n", i, step, traf_gw[i], b, (x - bx_() * TILE) / 256.0, (y - by_() * TILE) / 256.0, yaw); }
            for (int k = 0; k < ped_n; k++) { double dx = ped_x[k] - (x - bx_() * TILE), dy = ped_y[k] - (y - by_() * TILE); if (fabs(dx) + fabs(dy) < 2800) printf("       ped %d at %.2f,%.2f\n", k, ped_x[k] / 256.0, ped_y[k] / 256.0); }
            for (int k = 0; k < NCAR; k++) { int x2, y2, yw2, t2, v2; if (k != i && traf_dbg_get(k, &x2, &y2, &yw2, &t2, &v2)) { double dx = x2 - x, dy = y2 - y; if (fabs(dx) + fabs(dy) < 2800) { char b2[200]; traf_dbg_info(k, b2); printf("       car %d at %.2f,%.2f yaw %d [%s]\n", k, (x2 - bx_() * TILE) / 256.0, (y2 - by_() * TILE) / 256.0, yw2, b2); } } } }
        int ddx = x - a->px, ddy = y - a->py, dd2 = ddx * ddx + ddy * ddy; double sp = sqrt((double)dd2);
        if (a->on) {
            if (sp > 1500) { if (dist <= 17) viol[V_POPIN]++; cars_seen++; } else if (sp > 36) { viol[V_JUMP]++; if (g_verbose) printf("  jump %.0f car %d step %d at %d,%d\n", sp, i, step, tx, ty); }
            int dy = ((yaw - a->pyaw + 2048) & 4095) - 2048; if (ABS(dy) > 58) { viol[V_TURNSNAP]++; if (g_verbose) printf("  turnsnap %d car %d step %d at %d,%d\n", dy, i, step, tx, ty); }
            if (sp > 2.0) { double fs = sin(yaw * 2 * M_PI / 4096), fc = cos(yaw * 2 * M_PI / 4096); double dot = (ddx * fs + ddy * fc) / sp;
                if (dot < 0.906) { viol[dot < 0 ? V_BACKWARD : V_SLIDE]++; if (g_verbose) printf("  slide dot %.2f car %d step %d at %d,%d\n", dot, i, step, tx, ty); } }
            int near_ped = 0; for (int k = 0; k < ped_n; k++) if (ABS(ped_x[k] - (x - bx_() * TILE)) + ABS(ped_y[k] - (y - by_() * TILE)) < 1500) near_ped = 1;
            if (sp < 0.5 && near_ped) a->still = 0;
            else if (sp < 0.5) { a->still++; if (a->still == 600 && g_verbose) { extern int traf_dbg_info(int, char *); char b[200]; traf_dbg_info(i, b); printf("  STUCK w%d car %d step %d [%s] at %.2f,%.2f yaw %d  player %.1f,%.1f\n", g_curwin, i, step, b, (x - bx_() * TILE) / 256.0, (y - by_() * TILE) / 256.0, yaw, ped_x[0] / 256.0, ped_y[0] / 256.0); } } else { if (a->still >= 1800) viol[V_STUCK30]++; else if (a->still >= 600) viol[V_STUCK10]++; a->still = 0; }
            mean_speed_sum += sp; mean_speed_n++;
        }
        // off the road: the four corners
        obb(x, y, yaw, cx[i], cy[i]);
        int bad = 0; for (int k = 0; k < 4; k++) { int ttx = (int)floor(cx[i][k] / TILE) - bx_(), tty = (int)floor(cy[i][k] / TILE) - by_(); if (!isroad(ttx, tty)) bad = 1; }
        if (bad && dist < 60) { viol[V_OFFROAD]++; if (g_verbose && viol[V_OFFROAD] < 20) printf("  offroad car %d step %d tile %d,%d yaw %d\n", i, step, tx, ty, yaw); }
        // on its lane: when it faces along a street it must be on that direction's lane line
        { int q = yaw & 4095, dyy = ((yaw - a->pyaw + 2048) & 4095) - 2048; if (ABS(dyy) <= 1 && ((q & 1023) < 24 || (q & 1023) > 1000)) { int d = ((yaw + 512) >> 10) & 3, ax_ = d & 1 ? ty : tx; (void)ax_;
            double lx = x / (double)TILE, ly = y / (double)TILE; int mod; double err;
            if (d == 1 || d == 3) { mod = ((int)floor(ly) % 18 + 18) % 18; double fy = ly - floor(ly); double pos = mod + fy; double want = d == 1 ? 4.0 : 1.0; err = pos - want; if (d == 1 && mod >= 5) err = 0; if (d == 3 && mod >= 5) err = 0; }
            else { int m2 = ((int)floor(lx) % 18 + 18) % 18; double fx = lx - floor(lx); double pos = m2 + fx; double want = d == 0 ? 1.0 : 4.0; err = pos - want; if (m2 >= 5) err = 0; }
            // (a lane line exists only where the street is: the strip's own rows 0..4)
            if (fabs(err) > 0.3 && (d == 1 || d == 3 ? (((int)floor(ly) % 18 + 18) % 18) < 5 : (((int)floor(lx) % 18 + 18) % 18) < 5)) { viol[V_LANE]++; if (g_verbose && viol[V_LANE] < 20) printf("  lane err %.2f car %d step %d tile %d,%d yaw %d\n", err, i, step, tx, ty, yaw); } } }
        // stopped inside a junction square (all 25 tiles of a block's corner are road)
        if (junction_square(tx, ty) && sp < 0.5) { if (++a->inbox_t == 360) { viol[V_INBOX]++; if (g_verbose) printf("  stopped in junction car %d step %d at %d,%d\n", i, step, tx, ty); } } else a->inbox_t = 0;
        // turning: from leaving a street's axis to coming back to one; a half turn (a u-turn) must start where the street ends
        { int q = (yaw + 64) & 4095, aligned = (q & 1023) < 128;
          if (a->on) { if (!aligned && a->al) { a->sx = a->px; a->sy = a->py; a->syaw = a->pyaw; a->rot = 0; }
            if (!aligned) { int dy2 = ((yaw - a->pyaw + 2048) & 4095) - 2048; a->rot += dy2; }
            if (aligned && !a->al && ABS(a->rot) > 1800) { int d0 = ((a->syaw + 512) >> 10) & 3; static const int DXs[4] = { 0, 1, 0, -1 }, DYs[4] = { 1, 0, -1, 0 };
                int fx = (a->sx - bx_() * TILE) / TILE + DXs[d0] * 3, fy = (a->sy - by_() * TILE) / TILE + DYs[d0] * 3;
                if (isroad(fx, fy)) { viol[V_UTURN_NOTEND]++; if (g_verbose) printf("  u-turn with road ahead: car %d step %d from %d,%d heading %d\n", i, step, (a->sx - bx_() * TILE) / TILE, (a->sy - by_() * TILE) / TILE, d0); } }
            if (aligned) a->rot = 0; }
          a->al = aligned; }
        a->x = x; a->y = y; a->yaw = yaw; a->type = type; a->v = v; a->px = x; a->py = y; a->pyaw = yaw; a->on = 1; act[na++] = i;
    }
    if (inview > max_cars_view) max_cars_view = inview; in_view_steps += inview;
    // overlaps, and people inside a car
    int ov = 0; for (int p = 0; p < na; p++) for (int q = p + 1; q < na; q++) { int i = act[p], j = act[q]; if (ABS(ac[i].x - ac[j].x) > 700 || ABS(ac[i].y - ac[j].y) > 700) continue;
        if (sat(cx[i], cy[i], cx[j], cy[j])) { ov = 1; if (g_verbose && !ac[i].ov && viol[V_OVERLAP] < 400) { extern int traf_dbg_info(int, char *); char b1[200], b2[200]; traf_dbg_info(i, b1); traf_dbg_info(j, b2); printf("  NEW overlap w%d %d [%s] x,y %d,%d / %d [%s] x,y %d,%d step %d\n", g_curwin, i, b1, ac[i].x - bx_() * TILE, ac[i].y - by_() * TILE, j, b2, ac[j].x - bx_() * TILE, ac[j].y - by_() * TILE, step); } if (0) printf("  overlap cars %d,%d step %d at %d,%d  yaw %d %d  v %d %d\n", i, j, step, (ac[i].x - bx_() * TILE) / TILE, (ac[i].y - by_() * TILE) / TILE, ac[i].yaw, ac[j].yaw, ac[i].v, ac[j].v); } }
    if (ov) viol[V_OVERLAP]++; for (int p = 0; p < na; p++) ac[act[p]].ov = 0; for (int p = 0; p < na; p++) for (int q = p + 1; q < na; q++) { int i = act[p], j = act[q]; if (ABS(ac[i].x - ac[j].x) > 700 || ABS(ac[i].y - ac[j].y) > 700) continue; if (sat(cx[i], cy[i], cx[j], cy[j])) ac[i].ov = ac[j].ov = 1; }
    for (int p = 0; p < na; p++) { int i = act[p]; for (int k = 0; k < ped_n; k++) { double px_ = ped_x[k] + bx_() * TILE * 0 + 0.0, py_ = ped_y[k]; px_ += 0;
            double ax = ped_x[k] + bx_() * TILE, ay = ped_y[k] + by_() * TILE;   // (the people are in window units: the cars in the world's)
            double ex[4] = { ax - 4, ax + 4, ax - 4, ax + 4 }, ey[4] = { ay - 4, ay - 4, ay + 4, ay + 4 }; (void)px_; (void)py_;
            // order the little square's corners as a rectangle for sat(): 0 1 3 2
            double *sx = ex, *sy = ey;
            double cxx[4], cyy[4]; for (int c = 0; c < 4; c++) { cxx[c] = cx[i][c]; cyy[c] = cy[i][c]; }
            // car corners are in (k&1: front/back, k&2: left/right) order: 0 1 3 2 makes a ring
            double *rx = cxx, *ry = cyy;
            double fwd = (ax - ac[i].x) * sin(ac[i].yaw * 2 * M_PI / 4096) + (ay - ac[i].y) * cos(ac[i].yaw * 2 * M_PI / 4096);
            if (ac[i].v > 0 && fwd > -60 && sat(rx, ry, sx, sy)) { viol[V_PEDHIT]++; if (g_verbose && viol[V_PEDHIT] < 20) printf("  ped %d in car %d step %d  ped %.0f,%.0f car %d,%d yaw %d v %d\n", k, i, step, ax - bx_()*TILE, ay - by_()*TILE, ac[i].x - bx_()*TILE, ac[i].y - by_()*TILE, ac[i].yaw, ac[i].v); } } }
}
static int junction_square(int tx, int ty) {   // is this tile one of a junction's 5 x 5 (a block's corner), all of it road (window coordinates)
    int bx0 = tx - (((tx % 18) + 18) % 18), by0 = ty - (((ty % 18) + 18) % 18);
    for (int y = 0; y < 5; y++) for (int x = 0; x < 5; x++) if (!isroad(bx0 + x, by0 + y)) return 0;
    return tx - bx0 < 5 && ty - by0 < 5;
}

static int run_window(int wbx, int wby, int steps, int level, unsigned seed) {
    world_wbx = wbx; world_wby = wby; make_floor();
    traffic_clear(); traffic_level = level; traffic_cheat = 0;
    // the player stands on a road tile near the middle (where a settlement is, a crossing)
    int px = 36, py = 36; for (int r = 0; r < 30; r++) { int found = 0; for (int dy = -r; dy <= r && !found; dy++) for (int dx = -r; dx <= r && !found; dx++) if (ABS(dx) == r || ABS(dy) == r) if (walkable(px + dx, py + dy)) { px += dx; py += dy; found = 1; } if (found) break; }
    prs = seed * 7919u + 13; peds_init(px, py); sims_sync();
    for (int i = 0; i < MAXC; i++) ac[i].on = 0;
    int pstep = 0;
    for (int s = 0; s < steps; s++) {
        peds_step(1); sims_sync();
        traffic_update(1);
        audit_step(s, ped_x[0] / TILE, ped_y[0] / TILE); steps_run++;
        (void)pstep;
    }
    for (int i = 0; i < NCAR; i++) if (ac[i].on) { if (ac[i].still >= 1800) viol[V_STUCK30]++; else if (ac[i].still >= 600) viol[V_STUCK10]++; }
    return 0;
}
static void windows_for(int n, int *wb) {   // the first n windows of this world: its towns, then its highways (block coords of the window's corner)
    int k = 0;
    for (int i = 0; i < bm_nset() && k < n; i++) { const Settle *s = bm_set(i); wb[2 * k] = CLAMP(s->x - 2, 0, BM_N - WB); wb[2 * k + 1] = CLAMP(s->y - 2, 0, BM_N - WB); k++;
        if (k < n && s->r > 4) { wb[2 * k] = CLAMP(s->x + 3 - 2, 0, BM_N - WB); wb[2 * k + 1] = CLAMP(s->y + 1 - 2, 0, BM_N - WB); k++; } }
    for (int i = 0; i < bm_nseg() && k < n; i++) { int c, a, b, v; bm_seg(i, &c, &a, &b, &v); int m = (a + b) / 2; wb[2 * k] = CLAMP((v ? c : m) - 2, 0, BM_N - WB); wb[2 * k + 1] = CLAMP((v ? m : c) - 2, 0, BM_N - WB); k++; }
    for (; k < n; k++) { wb[2 * k] = wb[2 * (k % (k ? k : 1))]; wb[2 * k + 1] = wb[2 * (k % (k ? k : 1)) + 1]; }
}
extern unsigned traf_dbg_jmask(int, int);
int main(int argc, char **argv) {
#ifndef TRAF_SRC
    if(argc==2&&!strcmp(argv[1],"--parking-test")){
        int x,z,yaw;
        world_wbx=100;world_wby=200;world_zone=ZONE_CITY;
        traffic_clear();traffic_park(VEH_SEDAN,20*TILE,-20*TILE,1024);
        assert(traffic_parked(VEH_SEDAN));
        assert(traffic_park_pose_near(VEH_SEDAN,21*TILE,-20*TILE,&x,&z,&yaw));
        assert(x==20*TILE&&z==-20*TILE&&yaw==1024);
        assert(!traffic_park_pose_near(VEH_SEDAN,25*TILE,-20*TILE,&x,&z,&yaw));
        assert(traffic_vehicle_blocked(VEH_VAN,20*TILE,-20*TILE,1024));
        assert(!traffic_vehicle_blocked(VEH_SEDAN,20*TILE,-20*TILE,1024));
        /* Local coordinates change with the window; absolute parking does not. */
        world_wbx++;
        assert(traffic_park_pose_near(VEH_SEDAN,(20-BM_BLK)*TILE,-20*TILE,&x,&z,&yaw));
        assert(x==(20-BM_BLK)*TILE&&z==-20*TILE&&yaw==1024);
        assert(traffic_vehicle_blocked(VEH_VAN,x,z,yaw));
        traffic_unpark(VEH_SEDAN);
        assert(!traffic_vehicle_blocked(VEH_VAN,x,z,yaw));
        puts("actual traffic parking and window rebase: PASS");return 0;
    }
#endif
    if (getenv("JM")) { for (int d = 0; d < 4; d++) { for (int m = 0; m < 3; m++) printf("0x%07xu, ", traf_dbg_jmask(d, m)); printf("\n"); } return 0; }
    if (argc < 3) { fprintf(stderr, "usage: trafsim SEED GEN [LEVEL] [STEPS] [WINDOWS] [-v]\n"); return 1; }
    unsigned seed = (unsigned)strtoul(argv[1], 0, 0); int gen = atoi(argv[2]), level = argc > 3 ? atoi(argv[3]) : 2, steps = argc > 4 ? atoi(argv[4]) : 6000, nw = argc > 5 ? atoi(argv[5]) : 6;
    for (int i = 1; i < argc; i++) { if (!strcmp(argv[i], "-v")) g_verbose = 1; if (!strcmp(argv[i], "-c") && i + 4 < argc) { g_win = atoi(argv[i + 1]); g_tc = atoi(argv[i + 2]); g_t0 = atoi(argv[i + 3]); g_t1 = atoi(argv[i + 4]); } }
    bm_dials_default(); bm_gen = gen; bm_new(seed);
    int wb[64]; if (nw > 30) nw = 30; windows_for(nw, wb);
    for (int w = 0; w < nw; w++) {
        memset(viol, 0, sizeof viol); rs = 12345 + seed * 31 + w; cars_seen = 0;
        g_curwin = w; run_window(wb[2 * w], wb[2 * w + 1], steps, level, seed + w);
        if (g_verbose) { printf("window %d (%d,%d):", w, wb[2 * w], wb[2 * w + 1]); for (int k = 0; k < V_N; k++) printf(" %ld", viol[k]); printf("  cars seen %ld\n", cars_seen); }
        for (int k = 0; k < V_N; k++) viol_total[k] += viol[k];
    }
    printf("trafsim seed %u world version %d level %d: %d windows x %d vblanks, %ld car-vblanks, mean speed %.1f units/vblank, most cars in view %d, mean in view %.2f\n", seed, gen, level, nw, steps, car_steps, mean_speed_n ? mean_speed_sum / mean_speed_n : 0.0, max_cars_view, steps_run ? (double)in_view_steps / steps_run : 0.0);
    long all = 0; for (int k = 0; k < V_N; k++) { if (k == V_SPEEDSTREET) continue; printf("  %-72s %ld\n", vname[k], viol_total[k]); if (k != V_POPIN && k != V_POPOUT) all += viol_total[k]; else all += viol_total[k]; }
    printf("  TOTAL %ld\n", all);
    return 0;
}
