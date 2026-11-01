// the street traffic: cars hop from road tile to road tile in their own lane, turn at crossings, queue behind each other and stop for you;
// and the vehicles you leave parked stay where you left them. Positions are kept in whole-world tiles, so the map window sliding along changes nothing.
#include "common.h"
#include "world.h"
#include "sim.h"
#include "gfx.h"
#include "items.h"
#include "save.h"
#include "traffic.h"
#include "bigmap.h"

#define NCAR 120
#define BLK BM_BLK
#define LA (BM_LOT0 / 2 - 1)   // the street is five wide: two lanes of two each way and the line between. Cars hop along the tile nearest the kerb of a lane and are drawn half a tile over, mid-lane
#define LB (BM_LOT0 / 2 + 1)
int traffic_level = 2, traffic_cheat;
typedef struct { s32 tx, ty; s8 d; u8 type, col, spd; s16 p; u16 wait, wph; s16 yaw; u8 on; } Car;   // tile (whole world), heading, progress along the tile 0..TILE
static Car car[NCAR];
typedef struct { s32 x, y; s16 yaw; u8 on; } Park;   // (x, y: whole-world units; y grows south)
static Park park[VEH_N];
static u32 rs = 12345;
static int rnd(int n) { rs = rs * 1664525u + 1013904223u; return (int)((rs >> 12) % (u32)n); }
static const s8 DX[4] = { 0, 1, 0, -1 }, DY[4] = { 1, 0, -1, 0 };   // heading 0 is down the map (yaw 0), 1 right, 2 up, 3 left
static const u8 tbl[5] = { 0, 3, 6, 11, 18 }, cheat_tbl[5] = { 0, 10, 30, 60, 120 };
static int bx_(void) { return world_wbx * BLK; }
static int by_(void) { return world_wby * BLK; }
static int road(int tx, int ty) { if (tx < 0 || ty < 0 || tx >= MW || ty >= MH) return 0; int f = floorMap[ty][tx]; return FL_IS_ROAD(f); }
static int lane_ok(int tx, int ty, int d) {   // drive on the right: east along the south lane, west the north, south the west lane, north the east
    switch (d) { case 1: return ty % BLK == LB; case 3: return ty % BLK == LA; case 0: return tx % BLK == LA; default: return tx % BLK == LB; }
}
static int target(void) {
#ifdef TRAF_TEST
    traffic_level = 3; traffic_cheat = TRAF_TEST;
#endif
 int n = tbl[traffic_level % 5] + cheat_tbl[traffic_cheat % 5]; return n > NCAR ? NCAR : n; }
static int ptx(void) { return sim_tile_x(&sims[player]); }
static int pty(void) { return sim_tile_y(&sims[player]); }
static int taken(int ax, int ay, const Car *me) {   // a tile someone is on or heading for (or the player, or a parked vehicle)
    for (int i = 0; i < NCAR; i++) { const Car *c = &car[i]; if (!c->on || c == me) continue;
        if (c->tx == ax && c->ty == ay) return 1; if (c->p && c->tx + DX[(int)c->d] == ax && c->ty + DY[(int)c->d] == ay) return 1; }
    { int px = ptx() + bx_(), py = pty() + by_(); if (ABS(px - ax) + ABS(py - ay) <= 1) return 1; }
    for (int i = 0; i < nsims; i++) { const Sim *s = &sims[i]; if (i == player || s->away || s->lvl) continue; if (ABS(s->x / TILE + bx_() - ax) + ABS((-s->z) / TILE + by_() - ay) == 0) return 1; }   // (people on foot)
    for (int t = 0; t < VEH_N; t++) if (park[t].on && ABS(park[t].x / TILE - ax) + ABS(park[t].y / TILE - ay) == 0) return 1;
    return 0;
}
static int honk_t;
void traffic_honk(void) { honk_t = 60; }   // cars near you give way for a moment
void traffic_clear(void) { for (int i = 0; i < NCAR; i++) car[i].on = 0; }
static void spawn(void) {
    int px = ptx(), py = pty();
    for (int k = 0; k < 14; k++) {
        int tx = px + rnd(61) - 30, ty = py + rnd(61) - 30;
        if (!road(tx, ty)) continue;
        int dd = ABS(tx - px) + ABS(ty - py); if (dd < 7) continue;
        int d = rnd(4); if (!lane_ok(tx, ty, d) || !road(tx + DX[d], ty + DY[d])) continue;
        if (taken(tx + bx_(), ty + by_(), 0)) continue;
        for (int i = 0; i < NCAR; i++) if (!car[i].on) {
            Car *c = &car[i]; c->on = 1; c->tx = tx + bx_(); c->ty = ty + by_(); c->d = d; c->p = 0; c->wait = 0; c->wph = 0; c->yaw = d * 1024;
            static const u8 ty_[8] = { 2, 2, 2, 3, 4, 5, 5, 1 }; c->type = ty_[rnd(8)]; if (c->type == 1) c->type = 2; c->col = rnd(6); c->spd = 9 + rnd(8); return; }
        return;
    }
}
static void step_car(Car *c) {
    int lx = c->tx - bx_(), ly = c->ty - by_();
    if (lx < 1 || ly < 1 || lx >= MW - 1 || ly >= MH - 1) { c->on = 0; return; }
    int dd = ABS(lx - ptx()) + ABS(ly - pty());
    if (dd > 46) { c->on = 0; return; }
    int want = c->d * 1024, df = ((want - c->yaw + 2048) & 4095) - 2048; c->yaw = (c->yaw + (df > 140 ? 140 : df < -140 ? -140 : df)) & 4095;   // (the turn eases round)
    if (c->p == 0 && honk_t > 0 && dd < 9) return;   // (a horn: they hold still a moment)
    if (c->p == 0) {   // at a tile centre: pick the way
        if (c->wait == 0) {
            int ok[3], n = 0, w[3] = { 6, 2, 2 }, ds[3] = { c->d, (c->d + 3) & 3, (c->d + 1) & 3 };
            for (int k = 0; k < 3; k++) { int d = ds[k]; ok[k] = lane_ok(lx, ly, d) && road(lx + DX[d], ly + DY[d]); if (ok[k]) n += w[k]; }
            if (!n) { c->on = 0; return; }   // (a dead end: gone round the back)
            int r = rnd(n), k = 0; while (r >= (ok[k] ? w[k] : 0)) { r -= ok[k] ? w[k] : 0; k++; }
            c->d = ds[k];
        }
        { int blk = 0; for (int k = 1; k <= 3 && !blk; k++) blk = taken(c->tx + DX[(int)c->d] * k, c->ty + DY[(int)c->d] * k, c);   // (a car is three tiles long: room ahead for all of it)
          if (blk) { if (++c->wait > 240) c->on = 0; return; } }
        c->wait = 0; c->p = 1;
    }
    c->p += c->spd; c->wph = (c->wph + c->spd * 18) & 4095;
    if (c->p >= TILE) { c->p = 0; c->tx += DX[(int)c->d]; c->ty += DY[(int)c->d]; }
}
void traffic_update(int steps) {
    if (world_zone != ZONE_CITY || world_wbx < 0) return;
    int want = target(), n = 0; if (honk_t > 0) honk_t -= steps;
    for (int i = 0; i < NCAR; i++) if (car[i].on) n++;
    for (int s = 0; s < steps; s++) {
        for (int i = 0; i < NCAR; i++) if (car[i].on) step_car(&car[i]);
        if (n < want) { spawn(); spawn(); n++; }
    }
    for (int i = want; i < NCAR && n > want; i++) if (car[i].on) { car[i].on = 0; n--; }   // (the dial turned down: the cars fade away)
}
static void put(const Car *c) {
    static const s8 LX[4] = { -1, 0, 1, 0 }, LY[4] = { 0, 1, 0, -1 };   // (half a tile over, into the middle of the two-tile lane)
    int x = (c->tx - bx_()) * TILE + TILE / 2 + DX[(int)c->d] * c->p + LX[(int)c->d] * (TILE / 2), y = (c->ty - by_()) * TILE + TILE / 2 + DY[(int)c->d] * c->p + LY[(int)c->d] * (TILE / 2);
    items_draw_vehicle(x, -y, c->yaw, 0, c->type, c->col, c->wph);
}
#ifdef VEH_SHOT
int veh_shot_f;
#ifndef VEH_SHOT_YAW
#define VEH_SHOT_YAW 3072
#endif
extern void items_draw_bus(int x, int z, int yaw, int y);
#endif
void traffic_draw(void) {
    if (world_zone != ZONE_CITY || world_wbx < 0) return;
#ifdef VEH_SHOT
    if (veh_shot_f) { const Sim *s = &sims[player]; int fx = isin(2600), fz = icos(2600), rx = fz, rz = -fx;   // (test builds: three cars a row, two rows, the bus, laid out across the default view)
        for (int t = 0; t < 6; t++) { int a = t < 3 ? 250 : 950, b = ((t % 3) - 1) * 560;
            items_draw_vehicle(s->x + ((fx * a + rx * b) >> 12), s->z + ((fz * a + rz * b) >> 12), VEH_SHOT_YAW, 0, t, veh_colour(t), veh_shot_f * 40); }
        items_draw_bus(s->x + ((fx * 1750 + rx * 300) >> 12), s->z + ((fz * 1750 + rz * 300) >> 12), VEH_SHOT_YAW, 0); }
#endif
    int px = ptx(), py = pty(), shown = 0;
    for (int i = 0; i < NCAR && shown < 14; i++) { const Car *c = &car[i]; if (!c->on) continue;
        if (ABS(c->tx - bx_() - px) + ABS(c->ty - by_() - py) > 17) continue; put(c); shown++; }
    for (int t = 0; t < VEH_N; t++) { const Park *p = &park[t]; if (!p->on) continue;
        int x = p->x - bx_() * TILE, y = p->y - by_() * TILE;
        if (ABS(x / TILE - px) + ABS(y / TILE - py) > 18) continue;
        items_draw_vehicle(x, -y, p->yaw, 0, t, veh_colour(t), 0); }
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
