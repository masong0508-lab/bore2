// skysea_ui: SKY AND SEA, an overlay (SKYSEA.OVL): ten boats and six aircraft you really drive and fly, over YOUR world: the game's own SAN BOREAS, island, fields and desert. It stands alone.
//  BOATS (deep): fuel, an engine that overheats, hull damage, waves and wind (sails want a crosswind), tuning, jobs (ferry, cargo, fishing, a buoy race), a harbour to
//   refuel and repair at, and a tow home if you sink.
//  AIRCRAFT: throttle, pitch, bank, lift and stall. Land on the runway (or the sea, in the seaplane) for pay. Crash in CRASH MODE: REAL and the town pays for it: buildings
//   come down, people are hurt, the police write it down (and the pilot goes to hospital). PARACHUTE (TRIANGLE) gets you out of a plane in trouble: the plane flies on without you.
//  Stolen craft (the CRIME AND LAW pack's underworld, pk_data 40-41) can be taken out too: a landing may find the police waiting.
// pk_data: 24-25 craft owned, 26-27 earnings collected to that day, 28 jobs and landings done, 32-34 boat tuning (2 bits each), 35-36 aircraft tuning, 37 buildings brought down, 38 people hurt,
//  29 people killed, 39 bit 0: CRASH MODE REAL, bit 1: FALL DAMAGE, bit 2: DEBUG (every craft free), bits 3-4: BLAST SCALE (0 normal, 1 low, 2 extreme).
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "ovl.h"
#include "voxel.h"
#include "world.h"
#include "bigmap.h"
#include "far.h"
#include "wants.h"
#include <psxgpu.h>
#include <psxgte.h>
#include <psxpad.h>
#include <stdio.h>
#include <string.h>

#define NB 10
#define NA 8
#define NCR (NB + NA)
// name, top speed (units/frame x16), accel, turn, hull, fuel, burn, hold, price, daily income, kind (0 paddle 1 motor 2 sail 3 work 4 hover)
typedef struct { const char *nm; s16 vmax, acc, turn, hull, fuel, burn, hold; int price; s16 inc; u8 kind; } Boat;
static const Boat BT[NB] = {
    { "ROWBOAT",     40,  2, 30,  60,  60, 0,  1,   400,   20, 0 }, { "JET SKI",     330, 6, 52,  50, 100, 3,  0,  2500,  120, 1 },
    { "FISHING SKIFF",120, 3, 34,  90, 140, 1,  6,  1500,   70, 1 }, { "SPEEDBOAT",   300, 5, 42, 100, 160, 3,  2,  6000,  220, 1 },
    { "SAILBOAT",    150, 2, 26, 130, 220, 0,  2,  5000,  160, 2 }, { "TRAWLER",     110, 2, 20, 260, 400, 2, 12,  9000,  300, 3 },
    { "TUGBOAT",      90, 3, 28, 300, 380, 2, 16, 11000,  340, 3 }, { "FERRY",        95, 1, 14, 340, 520, 2, 24, 16000,  520, 3 },
    { "HOVERCRAFT",  200, 4, 60, 120, 240, 3,  4, 21000,  620, 4 }, { "SUPER YACHT", 220, 2, 18, 420, 700, 2,  8, 60000, 1700, 1 } };
// aircraft: name, top speed, stall speed, climb/pitch rate, bank rate, hull, fuel, price, income, flags (1 floats, 2 hovers, 4 balloon)
typedef struct { const char *nm; s16 vmax, stall, pitch, bank, hull, fuel; int price; s16 inc; u8 fl; } Air;
static const Air AT[NA] = {
    { "GLIDER",      110, 28, 22, 40,  60,  0,  7000,  250, 0 }, { "BIPLANE",     170, 40, 28, 46,  80, 220, 14000,  480, 0 },
    { "SEAPLANE",    190, 46, 24, 38, 110, 320, 24000,  800, 1 }, { "HELICOPTER", 150,  0, 30, 42, 120, 260, 38000, 1100, 2 },
    { "HOT AIR BALLOON",24, 0, 10, 10, 70, 200, 11000,  300, 6 }, { "JET",         400, 90, 34, 56, 150, 500, 90000, 3000, 0 },
    { "AIRLINER",    330, 70, 18, 28, 400, 900, 180000, 6000, 0 }, { "CARGO PLANE", 260, 62, 16, 26, 450, 1000, 140000, 5200, 0 } };
static const char *const CNM[NCR] = { "ROWBOAT", "JET SKI", "FISHING SKIFF", "SPEEDBOAT", "SAILBOAT", "TRAWLER", "TUGBOAT", "FERRY", "HOVERCRAFT", "SUPER YACHT", "GLIDER", "BIPLANE", "SEAPLANE", "HELICOPTER", "HOT AIR BALLOON", "JET", "AIRLINER", "CARGO PLANE" };
static const char *const CHINT[NCR] = { "OARS ONLY. FISH ALL DAY.", "FAST AND LIGHT. WAVES TOSS IT.", "SMALL HOLD. FISHES THE REEF.", "FAST. THIRSTY. RACES.", "FREE WIND. NEEDS A CROSSWIND.", "BIG HOLD. SLOW. FISHES BIG.", "PUSHES ANYTHING. TURNS ON A COIN.", "A BIG HOLD OF PASSENGERS.", "SKIMS WATER AND SAND.", "LUXURY AND SPEED.",
    "NO ENGINE. FIND THE LIFT.", "OPEN COCKPIT. FORGIVING.", "LAND ON THE WAVES.", "HOVERS. LANDS ANYWHERE.", "UP, DOWN, AND DRIFT.", "FAST. HARD TO LAND.", "FLIES PASSENGERS. LONG RUNS, WIDE TURNS.", "HAULS A HEAVY LOAD. STURDY." };
enum { M_MENU, M_BOAT, M_AIR, M_CHUTE, M_WRECK, M_END };
static int mode, tab, sel, anim; static char line[56], line2[56];
static unsigned rs = 5;
static int rnd(int n) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (int)((rs >> 3) % (unsigned)n); }
static u32 hs(int x, int y) { u32 h = (u32)x * 73856093u ^ (u32)y * 19349663u; h ^= h >> 13; h *= 0x5bd1e995u; return h ^ (h >> 15); }
static int isq(int v) { int r = 0, b = 1 << 28; if (v <= 0) return 0; while (b > v) b >>= 2; while (b) { if (v >= r + b) { v -= r + b; r = (r >> 1) + b; } else r >>= 1; b >>= 2; } return r; }
static int saved_far;
static int dbg(void) { return pk_data[39] & 4; }
// ----- the world: whichever one you are in (SAN BOREAS, the island, the fields, the desert), flown and sailed over -----
// x and z are in units of 4 world units (a tile, 256 units, is 64 of them), over the whole world: tile = x >> 6, and for z the other way (-z >> 6).
#define SC 4
#define BLKP (BM_BLK * 256 / SC)           // one block of SAN BOREAS (1152)
#define DOCKR 3500
extern void world_open_at(int cbx, int cby);
extern u32 world_horizon; extern int world_far_range;
static int wt(int t) { return t - (world_zone == ZONE_CITY && world_wbx >= 0 ? world_wbx * BM_BLK : 0); }   // an absolute tile column -> the window's (rows: wu)
static int wu(int t) { return t - (world_zone == ZONE_CITY && world_wby >= 0 ? world_wby * BM_BLK : 0); }
static int city(void) { return world_zone == ZONE_CITY && world_wbx >= 0; }
static int org_x(void) { return city() ? world_wbx * BM_BLK * 256 : 0; }       // the window's corner, in world units
static int org_z(void) { return city() ? world_wby * BM_BLK * 256 : 0; }
static int rel_x(int x) { return x * SC - org_x(); }                           // a point of the whole world -> the window's world units (what the 3D takes)
static int rel_z(int z) { return z * SC + org_z(); }
static int nbx = -1, nby, nbt;
static int sea_blk(int bx, int by) { if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N) return 0; if (bx != nbx || by != nby) { nbx = bx; nby = by; nbt = bm_terrain(bx, by) == BT_SEA; } return nbt; }
static int is_sea(int x, int z) {
    int tx = x >> 6, ty = (-z) >> 6;
    if (city()) { if (tx < 0 || ty < 0 || tx >= BM_N * BM_BLK || ty >= BM_N * BM_BLK) return 1; return sea_blk(tx / BM_BLK, ty / BM_BLK); }
    if (tx < 0 || ty < 0 || tx >= MW || ty >= MH) return world_zone == ZONE_ISLAND;
    return world_zone == ZONE_ISLAND && floorMap[ty][tx] == FL_WATER;
}
static int gh(int x, int z) { if (is_sea(x, z)) return -20; return city() ? world_gh(rel_x(x), rel_z(z)) / SC + 1 : 1; }
static int hasb(int lot) { return (lot >= 1 && lot <= 15) || (lot >= 20 && lot <= 31); }
static int bbx = -1, bby; static BInfo binf;
static int blk(int bx, int by) { if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N) return 0; if (bx != bbx || by != bby) { bbx = bx; bby = by; bm_block(bx, by, &binf); } return 1; }
static int bldg(int x, int z, int *top) {   // a building at this point (its top, in these units)? the window's own walls, else SAN BOREAS's lots
    int tx = x >> 6, ty = (-z) >> 6; if (tx < 0 || ty < 0) return 0;
    if (wt(tx) >= 0 && wt(tx) < MW && wu(ty) >= 0 && wu(ty) < MH) { char ch = lifeMap[wu(ty)][wt(tx)]; if (ch == 'W' || ch == 'w' || ch == 'D') { *top = (ch == 'w' ? 1 : 3) * WALL_FULL / SC; return 1; } if (!city()) return 0; }
    if (!city()) return 0;
    int bx = tx / BM_BLK, by = ty / BM_BLK; if (!blk(bx, by) || !hasb(binf.lot)) return 0;
    int lx = tx - bx * BM_BLK, ly = ty - by * BM_BLK; if (lx < 5 || lx > 14 || ly < 5 || ly > 13) return 0;
    *top = (bm_storeys(&binf) * WALL_FULL + 200) / SC; return 1;
}
// the harbours: where the sea reaches a town (or just where you started)
static int hx[6], hz[6], nh;
static int sea_near(int bx0, int by0, int R, int *ox, int *oz) {   // the nearest block of sea (rings out from the block)
    for (int r = 0; r <= R; r += 2) for (int d = -r; d <= r; d += 2) { int cx[4] = { bx0 + d, bx0 + d, bx0 - r, bx0 + r }, cy[4] = { by0 - r, by0 + r, by0 + d, by0 + d };
        for (int k = 0; k < 4; k++) if (sea_blk(cx[k], cy[k])) { *ox = (cx[k] * BM_BLK + BM_BLK / 2) * 64; *oz = -((cy[k] * BM_BLK + BM_BLK / 2) * 64); return 1; } }
    return 0;
}
static int water_near(int tx0, int ty0, int *ox, int *oz) {   // the island's water: the nearest tile of it in the map, or the edge
    for (int r = 0; r < 90; r += 1) for (int d = -r; d <= r; d++) { int cx[4] = { tx0 + d, tx0 + d, tx0 - r, tx0 + r }, cy[4] = { ty0 - r, ty0 + r, ty0 + d, ty0 + d };
        for (int k = 0; k < 4; k++) if (is_sea(cx[k] * 64 + 32, -(cy[k] * 64 + 32))) { *ox = cx[k] * 64 + 32; *oz = -(cy[k] * 64 + 32); return 1; } }
    return 0;
}
static int rand_sea(int cx, int cz, int lo, int hi, int *ox, int *oz) {   // a point of sea lo..hi blocks away
    for (int t = 0; t < 60; t++) { int a = rnd(4096), d = (lo + rnd(MAX(1, hi - lo))) * 700; int x = cx + (isin(a) * d >> 12), z = cz + (icos(a) * d >> 12); if (is_sea(x, z) && is_sea(x + 400, z) && is_sea(x - 400, z + 400)) { *ox = x; *oz = z; return 1; } }
    return 0;
}
// what a crash does: the wreck burns, the blast knocks buildings down, and the people near it are hurt
typedef struct { int x, z, top, age; } Rub;
static Rub rub[28]; static int nrub, dirty_w, ndem;
typedef struct { int x, y, z, age, sev; } Boom;
static Boom boom[4];
static void kaboom(int x, int y, int z, int sev) { for (int i = 3; i > 0; i--) boom[i] = boom[i - 1]; boom[0].x = x; boom[0].y = y; boom[0].z = z; boom[0].age = 1; boom[0].sev = sev; }
static int demolish(int tx, int ty) {   // the building on that tile of the world goes: rubble and fire; the people in it
    int ppl = 0, top = 600 / SC * 2;
    int rx = wt(tx), ry = wu(ty);
    if (rx >= 0 && rx < MW && ry >= 0 && ry < MH && lifeMap[ry][rx] != '.') {   // (the window's own: its walls come down round the spot)
        int x0 = MAX(0, rx - 6), x1 = MIN(MW - 1, rx + 6), y0 = MAX(0, ry - 6), y1 = MIN(MH - 1, ry + 6), n = 0;
        for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { char ch = lifeMap[y][x]; if (ch == 'W' || ch == 'w' || ch == 'D') { lifeMap[y][x] = '.'; floorMap[y][x] = FL_ROCK; n++; } }
        if (n) { world_touched(); ppl = 1 + n / 8; }
    }
    if (city()) { int bx = tx / BM_BLK, by = ty / BM_BLK;
        if (blk(bx, by) && hasb(binf.lot)) { ppl += bm_units(&binf) * 2; top = (bm_storeys(&binf) * WALL_FULL) / SC; bm_ovr_set(bx, by, BO_LOT, LOT_EMPTY); bbx = -1; } else if (!ppl) return 0; }
    else if (!ppl) return 0;
    if (ndem >= 30) return ppl;
    ndem++; dirty_w = 1; if (nrub < 28) { rub[nrub].x = tx * 64 + 32; rub[nrub].z = -(ty * 64 + 32); rub[nrub].top = top; rub[nrub].age = 0; nrub++; }
    return ppl + rnd(3);
}
static void blast(int x, int z, int rad, int *nb, int *ppl) {   // everything built within rad of the spot comes down
    int t0x = (x - rad) >> 6, t1x = (x + rad) >> 6, t0y = (-z - rad) >> 6, t1y = (-z + rad) >> 6;
    for (int ty = t0y; ty <= t1y; ty++) for (int tx = t0x; tx <= t1x; tx++) { int dx = tx * 64 + 32 - x, dz = -(ty * 64 + 32) - z, top; if (dx * dx + dz * dz > rad * rad) continue;
        if (!bldg(tx * 64 + 32, -(ty * 64 + 32), &top)) continue; int p = demolish(tx, ty); if (p) { (*nb)++; *ppl += p; } }
}
// ----- the craft you own -----
static int owned(void) { return pk_data[24] | pk_data[25] << 8 | pk_data[30] << 16; }
static int stol(int c) { int s = pk_data[40] | pk_data[41] << 8; return (c == 3 && (s >> 5 & 1)) || (c == 9 && (s >> 6 & 1)) || (c == 11 && (s >> 7 & 1)) || (c == 12 && (s >> 8 & 1)) || (c == 15 && (s >> 9 & 1)); }
static int has(int c) { return (owned() >> c & 1) || stol(c) || dbg(); }
static int last(void) { return pk_data[26] | pk_data[27] << 8; }
static int tune(int c) { return c < NB ? (pk_data[32 + c / 4] >> (c % 4 * 2)) & 3 : (pk_data[35 + (c - NB) / 4] >> ((c - NB) % 4 * 2)) & 3; }
static void set_tune(int c, int v) { u8 *p = c < NB ? &pk_data[32 + c / 4] : &pk_data[35 + (c - NB) / 4]; int sh = (c < NB ? c : c - NB) % 4 * 2; *p = (u8)((*p & ~(3 << sh)) | (v << sh)); }
static int price(int c) { return c < NB ? BT[c].price : AT[c - NB].price; }
static int inc(int c) { return c < NB ? BT[c].inc : AT[c - NB].inc; }
static int rate(void) { int r = 0; for (int i = 0; i < NCR; i++) if (owned() >> i & 1) r += inc(i); return r; }
static int lg(void) { return wants_skill_level(sims[player].skill_mech); }
static void settle(void) { if (owned() && day_count > last()) cash += rate() * MIN(30, day_count - last()); pk_data[26] = (u8)day_count; pk_data[27] = (u8)(day_count >> 8); }
static int real(void) { return pk_data[39] & 1; }
// ----- the drive -----
static int cur, px, pz, py, yaw, pitch, roll, spd, vy, thr, fuel, hull, temp, fish, jt, jtype, jx, jz, jpay, jn, fcast, hot, landed, crashed, ctr, sunk, cvy, windd, chute_on, cx, cy_, cz_, ax, ay, az, ayaw, aspd, apitch, aroll, hd_r1, wtm, wrx, wry, wrz, nbd, nkill, nhurt;
static void say(const char *s) { strncpy(line, s, 55); line[55] = 0; ctr = 200; }
static int my_x(void) { return (sims[player].x + org_x()) / SC; }
static int my_z(void) { return (sims[player].z - org_z()) / SC; }
static int find_water(int *ox, int *oz) {
    int x = my_x(), z = my_z();
    if (city()) return sea_near((x >> 6) / BM_BLK, ((-z) >> 6) / BM_BLK, 120, ox, oz);
    if (world_zone == ZONE_ISLAND) return water_near(x >> 6, (-z) >> 6, ox, oz);
    return 0;
}
static void spawn_boat(int c) {
    int sx, sz; if (!find_water(&sx, &sz)) { say("NO WATER HERE. TRY THE CITY OR THE ISLAND."); return; }
    cur = c; px = sx; pz = sz; yaw = 0; for (int a = 0; a < 4096; a += 512) if (is_sea(px + (isin(a) * 1600 >> 12), pz + (icos(a) * 1600 >> 12))) { yaw = a; break; }
    roll = pitch = 0; spd = 0; thr = 0; py = 0;
    nh = 1; hx[0] = px; hz[0] = pz; px += isin(yaw) * 500 >> 12; pz += icos(yaw) * 500 >> 12;
    if (city()) for (int i = 0; i < bm_nset() && i < 16 && nh < 6; i++) { const Settle *s = bm_set(i); int ox, oz; if (!sea_near(s->x, s->y, 10, &ox, &oz)) continue;
        int dx = ox - px, dz = oz - pz, far_ = 0; if (ABS(dx) > 90 * BLKP || ABS(dz) > 90 * BLKP) continue; for (int k = 0; k < nh; k++) if (ABS(ox - hx[k]) < 8 * BLKP && ABS(oz - hz[k]) < 8 * BLKP) far_ = 1; if (far_) continue; hx[nh] = ox; hz[nh] = oz; nh++; }
    int tl = tune(c); fuel = BT[c].fuel * (100 + tl * 30) / 100; hull = BT[c].hull * (100 + tl * 25) / 100; temp = 0; fish = 0; jtype = 0; jt = 0; crashed = sunk = 0; mode = M_BOAT; line[0] = 0; ctr = 0; hot = stol(c);
}
static void spawn_air(int c) {
    int a = c - NB; cur = c; px = my_x(); pz = my_z(); yaw = 0; pitch = roll = 0; vy = 0; thr = 0; spd = 0; chute_on = 0;
    int g = MAX(1, gh(px, pz)), fx = AT[a].fl;
    py = g + (a == 5 ? 1700 : a == 0 ? 1800 : a == 2 || a == 1 ? 1000 : fx & 4 ? 6 : fx & 2 ? 6 : 1000);
    if (!(fx & 6)) { spd = AT[a].vmax * 16 * 7 / 10; thr = a == 0 ? 0 : 60; }
    fuel = AT[a].fuel * (100 + tune(c) * 25) / 100; hull = AT[a].hull * (100 + tune(c) * 25) / 100;
    if (fx & 4) fuel = 400 + tune(c) * 100; if (a == 0) fuel = 9999;
    landed = !!(fx & 6); crashed = 0; mode = M_AIR; line[0] = 0; ctr = 0; hot = stol(c);
}
static void hurt(int n) { hull -= n; if (hull <= 0 && !crashed) crashed = 1; }
static void end_trip(const char *a, const char *b) { strcpy(line, a); if (b) strcpy(line2, b); mode = M_END; ctr = 0; clock_min = MIN(1439, clock_min + 45); audio_voice(crashed || sunk ? VOICE_CRY : VOICE_WIN); }
static void leave_trip(void) {   // back to the menu: what came down stays down
    if (dirty_w) { if (city()) { world_open_at(world_wbx + 2, world_wby + 2); far_new_world(); far_reset(); } else world_touched(); dirty_w = 0; }
    nrub = 0; ndem = 0; memset(boom, 0, sizeof boom); mode = M_MENU;
}
static void police_check(void) { if (hot && rnd(100) < 25 + pk_data[42] / 2) { sim_crime(CR_THEFT, 20); sim_arrest(1); strcat(line2, "  THE POLICE WERE WAITING!"); } }
static int blast_scale(void) { int k = (pk_data[39] >> 3) & 3; return k == 0 ? 100 : k == 1 ? 60 : 180; }   // NORMAL, LOW, EXTREME (percent)
static int sev_now(void) {   // how bad an impact is: the faster and harder you hit, and the fuller the tanks, the worse (5..100)
    int a = cur - NB, vm = MAX(1, AT[a].vmax * 16), fl = AT[a].fuel ? fuel * 100 / (AT[a].fuel + 1) : 0, s = spd * 60 / vm + MAX(0, -vy) * 2 + MIN(100, fl) / 5 + (AT[a].fl & 4 ? 0 : 6);
    return CLAMP(s, 5, 100);
}
static void crash_at(int x, int y, int z, int unmanned, int sev) {   // a plane meets the ground or a building: the blast (sev 5..100 sets how big)
    int nb = 0, ppl = 0; sev = sev * blast_scale() / 100; sev = CLAMP(sev, 3, 160); kaboom(x, y, z, sev); audio_voice(VOICE_CRY); line2[0] = 0;
    if (real()) { blast(x, z, 350 + sev * 22, &nb, &ppl);
        int dead = ppl / 3, hurtp = ppl - dead; nbd += nb; nkill += dead; nhurt += hurtp;
        pk_data[37] = (u8)MIN(255, pk_data[37] + nb); pk_data[38] = (u8)MIN(255, pk_data[38] + hurtp); pk_data[29] = (u8)MIN(255, pk_data[29] + dead);
        if (nb) { for (int i = 0; i < 1 + nb / 2; i++) sim_crime(CR_ASSAULT, 20 + nb * 3); if (ppl) sim_crime(CR_ASSAULT, 60); if (dead) sim_crime(CR_ASSAULT, 100); cash = MAX(0, cash - 300 * nb); sims[player].mood = MAX(-100, sims[player].mood - 20 - nb * 4);
            sprintf(line2, "%d BUILDINGS DOWN. %d KILLED, %d HURT.", nb, dead, hurtp); }
        else strcpy(line2, "YOU HIT THE GROUND. NOBODY WAS HURT."); }
    else { cash = MAX(0, cash - 500); strcpy(line2, "ARCADE MODE: REPAIRS COST $500."); }
    if (!unmanned) { crashed = 1; wrx = x; wry = y; wrz = z; wtm = 0; mode = M_WRECK; strcpy(line, "THE PLANE CRASHED!");
        if (real()) { cash = MAX(0, cash - 2000); sims[player].mood = MAX(-100, sims[player].mood - 40); clock_min = MIN(1439, clock_min + 180); strcat(line2, " YOU WENT TO HOSPITAL: -$2000."); } }
}
static void bail(int jump) {   // out of the craft (a jump: no canopy; the empty craft goes on)
    if (mode != M_AIR) return; int a = cur - NB, alt = py - MAX(0, gh(px, pz));
    ax = px; ay = py; az = pz; ayaw = yaw; aspd = spd; apitch = pitch; aroll = roll; cx = px; cy_ = py; cz_ = pz; mode = M_CHUTE; chute_on = !jump && alt > 120; cvy = jump ? 14 : 0; if (jump) aspd = 0; if (AT[a].fl & 6) { aspd = 0; ayaw = yaw; }
    say(jump ? "YOU JUMP! R1 OPENS THE CHUTE." : chute_on ? "CHUTE OPEN! R1 CLOSES IT." : "TOO LOW FOR A CHUTE!"); audio_voice(VOICE_YAHOO);
}
// ----- physics -----
static void boat_step(int hd) {
    const Boat *b = &BT[cur]; int tl = tune(cur), vm = b->vmax * (100 + tl * 10) / 100;
    if (hd & PAD_CROSS) thr = MIN(100, thr + 2); if (hd & PAD_SQUARE) thr = MAX(-30, thr - 3);
    if (!(hd & (PAD_CROSS | PAD_SQUARE)) && b->kind == 0) thr = thr * 31 / 32;
    int sea = (isin((anim * 7) & 4095) >> 9) + 1;
    int hf = 100 - (hull < b->hull / 3 ? 25 : 0) - (temp > 100 ? 60 : 0);
    int tv = vm * thr / 100 * hf / 100; if (fuel <= 0 && b->kind != 0 && b->kind != 2) tv = 0;
    if (b->kind == 2) { int rel = ABS(isin((yaw - windd) & 4095)); tv = vm * rel / 4096 * (60 + MAX(0, thr)) / 100; if (fuel > 0 && thr > 60) tv += vm / 5; }
    if (b->kind == 0 || (b->kind == 1 && b->hull < 80)) tv = tv * (100 - sea * 8) / 100;
    if (spd < tv) spd = MIN(tv, spd + b->acc); else spd = MAX(tv, spd - b->acc * 2);
    int tn = b->turn * (40 + MIN(spd, 160) / 2) / 100 + (b->kind == 4 || cur == 6 ? 8 : 0);
    if (hd & PAD_LEFT) yaw = (yaw - tn) & 4095; if (hd & PAD_RIGHT) yaw = (yaw + tn) & 4095;
    roll = ((hd & PAD_RIGHT ? 1 : hd & PAD_LEFT ? -1 : 0) * spd * 2 + (isin((anim * 19) & 4095) >> 8) * (sea + 1)) / 3;
    int mx = isin(yaw) * spd >> 16, mz = icos(yaw) * spd >> 16, nx = px + mx, nz = pz + mz, top;
    if (b->kind != 4) { if (!is_sea(nx, nz) || gh(nx, nz) > 0) { int imp = ABS(spd) / 12; if (imp > 2) { hurt(imp); say("AGROUND!"); audio_voice(VOICE_NAH); } spd = -spd / 4; } else { px = nx; pz = nz; } }
    else { px = nx; pz = nz; if (bldg(px, pz, &top)) { hurt(30); spd = -spd / 2; px -= mx * 3; pz -= mz * 3; say("YOU HIT A BUILDING!"); } }
    if (hull <= 0) sunk = 1;
    if (b->burn && thr > 0 && spd > 0 && !(anim & 15)) fuel = MAX(0, fuel - 1 - b->burn / 3);
    if (b->kind != 0 && b->kind != 2) { if (thr > 85 && !(anim & 7)) temp = MIN(130, temp + 1); else if (!(anim & 15)) temp = MAX(0, temp - 1); if (temp >= 120) hurt(1); }
    if (jtype == 1 && jt > 0) { jt--; int dx = px - jx, dz = pz - jz; if (ABS(dx) < DOCKR && ABS(dz) < DOCKR && ABS(spd) < 70) { int pay = jpay + jt / 30; cash += pay; pk_data[28]++; sims[player].skill_mech += 25; sprintf(line, "DELIVERED! +$%d", pay); ctr = 240; jtype = 0; audio_voice(VOICE_WIN); } else if (!jt) { say("TOO LATE. THE JOB IS GONE."); jtype = 0; } }
    if (jtype == 2) { int dx = px - jx, dz = pz - jz; if (ABS(dx) < 2500 && ABS(dz) < 2500 && ABS(spd) < 40) { if (++fcast >= 150) { fcast = 0; if (fish < b->hold) { fish++; say("A BITE! YOU LAND A FISH."); audio_voice(VOICE_AGREE); } else say("THE HOLD IS FULL."); } } else fcast = 0; }
    if (jtype == 3 && jt > 0) { jt--; int dx = px - jx, dz = pz - jz; if (ABS(dx) < 2500 && ABS(dz) < 2500) { if (++jn >= 5) { int pay = jpay + jt / 20; cash += pay; pk_data[28]++; sprintf(line, "RACE WON! +$%d", pay); ctr = 240; jtype = 0; audio_voice(VOICE_WIN); } else { if (!rand_sea(hx[0], hz[0], 8, 18, &jx, &jz)) { jx = hx[0]; jz = hz[0]; } say("BUOY! KEEP GOING."); } } } else if (jtype == 3 && !jt) { say("RACE OVER. TOO SLOW."); jtype = 0; }
}
static int near_dock(void) { for (int i = 0; i < nh; i++) { int dx = px - hx[i], dz = pz - hz[i]; if (ABS(dx) < DOCKR && ABS(dz) < DOCKR) return i; } return -1; }
static void dock_act(void) {   // TRIANGLE at a harbour, nearly stopped
    int d = near_dock(); if (d < 0 || ABS(spd) > 60) return; const Boat *b = &BT[cur]; int fm = b->fuel * (100 + tune(cur) * 30) / 100, hm = b->hull * (100 + tune(cur) * 25) / 100;
    if (fish) { int pay = fish * (30 + b->hold * 4); cash += pay; sprintf(line, "SOLD %d FISH: +$%d", fish, pay); fish = 0; ctr = 220; audio_voice(VOICE_WIN); return; }
    int cost = (fm - fuel) / 4 + (hm - hull) * 2; if (cost > 0) { if (cash < cost && !dbg()) { say("NOT ENOUGH CASH TO REFUEL AND REPAIR."); return; } if (!dbg()) cash -= cost; fuel = fm; hull = hm; temp = 0; sprintf(line, "FUELLED AND MENDED: -$%d", dbg() ? 0 : cost); ctr = 200; return; }
    if (jtype) { say("YOU ALREADY HAVE A JOB."); return; }
    if (b->kind == 0 || cur == 2 || cur == 5) { jtype = 2; fcast = 0; if (!rand_sea(hx[d], hz[d], 5, 11, &jx, &jz)) { jx = hx[d]; jz = hz[d]; } say("FISH THE REEF. STOP IN THE RING."); return; }
    if (cur == 1 || cur == 3 || cur == 8) { jtype = 3; jn = 0; jt = 60 * 150; jpay = 400 + cur * 60; if (!rand_sea(hx[d], hz[d], 8, 18, &jx, &jz)) { jx = hx[d]; jz = hz[d]; } say("BUOY RACE: FIVE BUOYS. GO!"); return; }
    int to = -1; if (nh > 1) { to = (d + 1 + rnd(nh - 1)) % nh; jx = hx[to]; jz = hz[to]; } else if (!rand_sea(hx[d], hz[d], 25, 50, &jx, &jz)) { say("NOWHERE TO GO."); return; }
    int dx = jx - hx[d], dz = jz - hz[d], dist = isq(MIN(2000000, ABS(dx) / 4) * MIN(2000000, ABS(dx) / 4) + MIN(2000000, ABS(dz) / 4) * MIN(2000000, ABS(dz) / 4)) * 4; jtype = 1; jt = dist * 16 / MAX(30, b->vmax) * 14 / 10 + 240; jpay = dist / 120 * (2 + b->hold / 4) + 100; say("A JOB: DELIVER IT. WATCH THE CLOCK.");
}
static void air_step(int hd) {
    const Air *a = &AT[cur - NB]; int tl = tune(cur); int vm = a->vmax * (100 + tl * 8) / 100 * 16, st = a->stall * 16; int g0 = MAX(1, gh(px, pz)), alt = py - g0;
    int hel = a->fl & 2;
    if (a->fl & 4) {   // the balloon: burner up, vent down, drifts with the wind
        if (hd & PAD_CROSS) vy = MIN(40, vy + 2); else if (hd & PAD_SQUARE) vy = MAX(-40, vy - 2); else vy = vy * 15 / 16; py += vy / 4;
        px += isin(windd) >> 10; pz += icos(windd) >> 10; if (hd & PAD_UP) pz += 6; if (hd & PAD_DOWN) pz -= 6; if (hd & PAD_RIGHT) px += 6; if (hd & PAD_LEFT) px -= 6; if (!(anim & 63) && vy > 0) fuel = MAX(0, fuel - 1); if (!fuel && vy > 0) vy = 0;
        int top; if (bldg(px, pz, &top) && py < g0 + top + 4) { if (vy < -26) { crash_at(px, py, pz, 0, sev_now()); return; } py = g0 + top + 4; vy = 0; }
        if (py < g0 + 4) { py = g0 + 4; if (vy < -26) { hurt(40); say("A HARD LANDING."); } vy = 0; }
        landed = py <= g0 + 8; return; }
    if (hd & PAD_CROSS) thr = MIN(100, thr + 2); if (hd & PAD_SQUARE) thr = MAX(0, thr - 2);
    if (hel) {   // the helicopter: thr is the lift
        int up = (thr - 45) * 7 / 100; vy = vy * 7 / 8 + up; spd = spd * 15 / 16 + ((hd & PAD_DOWN) ? 10 : (hd & PAD_UP) ? -6 : 0); pitch = (hd & PAD_DOWN) ? -240 : (hd & PAD_UP) ? 240 : pitch * 7 / 8;
        if (hd & PAD_LEFT) yaw = (yaw - a->bank / 4) & 4095; if (hd & PAD_RIGHT) yaw = (yaw + a->bank / 4) & 4095; roll = (hd & PAD_RIGHT ? 200 : hd & PAD_LEFT ? -200 : 0);
        if (fuel > 0 && !(anim & 31)) fuel--; if (!fuel) vy -= 2;
    } else {
        int sinp = isin(pitch & 4095);
        if (hd & PAD_DOWN) pitch = MIN(1100, pitch + a->pitch); else if (hd & PAD_UP) pitch = MAX(-1100, pitch - a->pitch); else pitch = pitch * 31 / 32;
        if (hd & PAD_LEFT) roll = MAX(-900, roll - a->bank); else if (hd & PAD_RIGHT) roll = MIN(900, roll + a->bank); else roll = roll * 15 / 16;
        int thrust = (cur == NB ? 0 : fuel > 0 ? thr * (vm / 100) / 60 : 0); spd += thrust - spd / 60 * (spd > vm ? 3 : 1) - sinp * 3 / 4096;
        if (cur != NB && !thrust) spd -= 1; if (spd < 0) spd = 0; if (spd > vm * 12 / 10) spd = vm * 12 / 10;
        int lift = spd > st ? 0 : (st - spd) * 18 / st;
        vy = (spd * sinp >> 12) / 16 - lift; if (spd < st) pitch = MAX(-700, pitch - 6);
        yaw = (yaw + (isin(roll & 4095) * spd >> 14) / 8) & 4095; if (!(anim & 31) && thr > 10 && fuel > 0) fuel--; if (fuel <= 0 && cur != NB) thr = 0;
        if (cur == NB) { vy -= 1; int th = hs(px >> 9, pz >> 9) & 255; if (th > 170 && alt > 20) vy += 4; }
    }
    int fv = spd * icos(pitch & 4095) >> 12; if (hel) fv = spd * 3; px += (isin(yaw) * fv >> 12) / 64; pz += (icos(yaw) * fv >> 12) / 64; py += vy / 4 * (hel ? 1 : 1) / (hel ? 3 : 1);
    int g = MAX(1, gh(px, pz)), over_sea = is_sea(px, pz), top;
    if (bldg(px, pz, &top) && py < g + top) {   // a building
        int vs = -vy; if (hel && vs < 12 && ABS(spd) < 60) { py = g + top + 4; vy = 0; landed = 1; }
        else { crash_at(px, py, pz, 0, sev_now()); return; } }
    if (py <= g + 4 && !crashed) {   // the ground
        int slope = ABS(gh(px + 64, pz) - gh(px - 64, pz)) + ABS(gh(px, pz + 64) - gh(px, pz - 64)), vs = -vy, floats = (a->fl & 1) && over_sea;
        int ok = (spd < st * 3 / 2 || hel) && vs < (hel ? 18 : 14) && ABS(roll) < 260 && ABS(pitch) < 360 && (floats || ((hel || cur == NB || (vs < 9 && spd < st)) && slope < 45) || (!over_sea && slope < 25 && vs < 9));
        if (over_sea && !floats && !hel) ok = 0;
        if (ok) { py = over_sea ? 4 : g + 4; vy = 0; roll = 0; pitch = 0; spd = spd * 14 / 15; if (!landed) { landed = 1; int pay = 150 + (14 - MIN(14, vs)) * 25; cash += pay; pk_data[28]++; sims[player].skill_mech += 25; sprintf(line, "SAFE LANDING! +$%d", pay); ctr = 240; audio_voice(VOICE_WIN); } }
        else { int dm = vs * 4 + spd / 12 + slope + ABS(roll) / 40;
            if (dm < hull && dm < 90 && !over_sea) { hurt(dm); py = g + 4; vy = 0; spd = spd / 3; roll = pitch = 0; landed = 1; sprintf(line, "CRASH LANDING! HULL -%d", dm); ctr = 240; audio_voice(VOICE_NAH); }
            else { crash_at(px, py, pz, 0, sev_now()); return; } }
    } else landed = 0;
}
static void chute_step(int hd) {
    if (hd_r1 && cy_ - MAX(0, gh(cx, cz_)) > 40) chute_on = !chute_on;
    if (chute_on) { cvy = -5; cx += isin(windd) >> 11; cz_ += icos(windd) >> 11; } else cvy = MAX(cvy - 2, -60);
    int mv = chute_on ? 5 : 3; if (hd & PAD_LEFT) cx -= mv; if (hd & PAD_RIGHT) cx += mv; if (hd & PAD_UP) cz_ += mv; if (hd & PAD_DOWN) cz_ -= mv;
    cy_ += cvy / 3;
    if (!crashed) { int g = MAX(1, gh(ax, az)), top; ax += (isin(ayaw) * aspd >> 12) / 64; az += (icos(ayaw) * aspd >> 12) / 64; ay += (aspd * isin(apitch & 4095) >> 12) / 256 - 3; apitch -= 2;
        if (ay <= g + 4 || (bldg(ax, az, &top) && ay < g + top)) { crashed = 1; crash_at(ax, ay, az, 1, 35); } }
    int gg = MAX(1, gh(cx, cz_));
    if (cy_ <= gg + 4) {
        int sea = is_sea(cx, cz_); mode = M_END; sunk = 0; int plane_crashed = crashed;
        if (!chute_on && !sea) { strcpy(line, "NO CHUTE!"); if (pk_data[39] & 2) { if (real()) { cash = MAX(0, cash - 2000); clock_min = MIN(1439, clock_min + 240); sims[player].mood = MAX(-100, sims[player].mood - 50); strcpy(line2, "YOU SURVIVED, JUST. HOSPITAL: -$2000."); } else { cash = MAX(0, cash - 300); strcpy(line2, "ARCADE MODE: A BROKEN LEG. -$300."); } audio_voice(VOICE_CRY); } else { strcpy(line2, "FALL DAMAGE IS OFF. YOU DUST YOURSELF OFF."); audio_voice(VOICE_YAHOO); } }
        else { strcpy(line, sea ? "SPLASH! A BOAT FISHES YOU OUT." : "A SOFT LANDING. THE PARACHUTE WORKED."); cash = MAX(0, cash - 400); audio_voice(VOICE_WIN); }
        if (plane_crashed) { char t[40]; sprintf(t, " THE PLANE CRASHED. %d BUILDINGS DOWN.", nbd); strncat(line2, t, 54 - strlen(line2)); } else strncat(line2, " THE EMPTY PLANE LANDED ITSELF.", 54 - strlen(line2));
        clock_min = MIN(1439, clock_min + 60); crashed = 0; }
}
static int nrows(void) { return NCR + 5; }
static void act(void);
void skysea_open(int a) { (void)a; mode = M_MENU; tab = 0; sel = 0; anim = 0; line[0] = line2[0] = 0; rs = (unsigned)(cash * 5 + day_count * 13 + 3); windd = (day_count * 531) & 4095; ctr = 0; saved_far = far_view; far_view = 5; nrub = 0; ndem = 0; dirty_w = 0;
    if (!owned() && !last()) { pk_data[26] = (u8)day_count; pk_data[27] = (u8)(day_count >> 8); }
#ifdef AUTO_SKYSEL
    sel = AUTO_SKYSEL; act();
#endif
}
static void act(void) {
    line[0] = 0;
    if (sel == NCR) { int d = MIN(30, MAX(0, day_count - last())); if (!owned()) { strcpy(line, "BUY A CRAFT FIRST."); return; } if (!d) { strcpy(line, "NOTHING NEW TODAY."); return; } int pay = rate() * d; settle(); sprintf(line, "%d DAYS OF GUESTS: +$%d", d, pay); audio_voice(VOICE_WIN); return; }
    if (sel == NCR + 4) { int k = (pk_data[39] >> 3) & 3; k = k == 0 ? 2 : k == 2 ? 1 : 0; pk_data[39] = (u8)((pk_data[39] & ~(3 << 3)) | (k << 3)); strcpy(line, k == 0 ? "BLAST SCALE: NORMAL" : k == 1 ? "BLAST SCALE: LOW" : "BLAST SCALE: EXTREME"); return; }
    if (sel == NCR + 3) { pk_data[39] ^= 4; strcpy(line, dbg() ? "DEBUG ON: EVERY CRAFT IS FREE" : "DEBUG OFF"); return; }
    if (sel == NCR + 2) { pk_data[39] ^= 2; strcpy(line, pk_data[39] & 2 ? "FALL DAMAGE: ON" : "FALL DAMAGE: OFF"); return; }
    if (sel == NCR + 1) { pk_data[39] ^= 1; strcpy(line, real() ? "CRASH MODE: REAL. THE TOWN PAYS." : "CRASH MODE: ARCADE. BOUNCE BACK."); return; }
    if (has(sel)) { if (sel < NB) spawn_boat(sel); else spawn_air(sel); return; }
    if (cash < price(sel) && !dbg()) { strcpy(line, "NOT ENOUGH CASH."); audio_voice(VOICE_NAH); return; }
    settle(); if (!dbg()) cash -= price(sel); pk_data[sel < 16 ? 24 + (sel >> 3) : 30] |= (u8)(1 << (sel & 7)); sprintf(line, "BOUGHT: %s. X TO TAKE IT OUT.", CNM[sel]); audio_voice(VOICE_FINISHED);
}
static void burn(int steps) {   // the fires burn on and spread; the blasts age
    for (int i = 0; i < nrub; i++) rub[i].age += steps;
    for (int i = 0; i < 4; i++) if (boom[i].age) { boom[i].age += steps; if (boom[i].age > 200) boom[i].age = 0; }
    if (nrub && real() && ndem < 30 && !(anim & 127)) { Rub *r = &rub[rnd(nrub)]; if (r->age < 2400 && rnd(100) < 40) {
        int x = r->x + (rnd(3) - 1) * 700, z = r->z + (rnd(3) - 1) * 700, top; if (bldg(x, z, &top)) { int p = demolish(x >> 6, (-z) >> 6); if (p) { nbd++; nhurt += p; pk_data[37] = (u8)MIN(255, pk_data[37] + 1); pk_data[38] = (u8)MIN(255, pk_data[38] + p); sim_crime(CR_ASSAULT, 30); say("THE FIRE SPREADS!"); } } } }
}
int skysea_update(int hd, int pr, int steps) {
    anim += steps;
    if (mode == M_MENU) {
        if (pr & PAD_CIRCLE) { far_view = saved_far; return 0; }
        int n = nrows(); if (pr & PAD_UP) sel = (sel + n - 1) % n; if (pr & PAD_DOWN) sel = (sel + 1) % n;
        if (pr & PAD_CROSS) act();
        if ((pr & PAD_SQUARE) && sel < NCR && ((owned() >> sel & 1) || dbg())) { int t = tune(sel), c = price(sel) / 4 * (t + 1); if (dbg()) c = 0; if (t >= 3) strcpy(line, "FULLY TUNED."); else if (cash < c) strcpy(line, "NOT ENOUGH CASH TO TUNE."); else { cash -= c; set_tune(sel, t + 1); sprintf(line, "TUNED UP: LEVEL %d", t + 1); audio_voice(VOICE_FINISHED); } }
        return 1; }
    if (mode == M_END) { if (pr & (PAD_CROSS | PAD_CIRCLE)) { police_check(); leave_trip(); } return 1; }
    burn(steps);
    if (mode == M_WRECK) { wtm += steps; if (wtm > 240 || ((pr & PAD_CROSS) && wtm > 40)) { mode = M_END; clock_min = MIN(1439, clock_min + 45); } return 1; }
    hd_r1 = pr & PAD_R1;
    for (int s = 0; s < steps && s < 4; s++) {
        anim++;
        if (mode == M_BOAT) { boat_step(hd); if (sunk) { cash = MAX(0, cash - 600); end_trip("THE BOAT SANK!", "A TUG TOWS YOU HOME. -$600."); return 1; } }
        else if (mode == M_AIR) { air_step(hd); if (mode != M_AIR) return 1; }
        else if (mode == M_CHUTE) { chute_step(hd); if (mode != M_CHUTE) return 1; }
    }
    if (ctr > 0) ctr -= steps;
    if (mode == M_BOAT) { if (pr & PAD_TRIANGLE) dock_act(); if (pr & PAD_CIRCLE) { if (near_dock() >= 0 && ABS(spd) < 60) { if (hot) police_check(); leave_trip(); } else say("PARK AT A HARBOUR TO GET OFF."); } }
    if (mode == M_AIR) { if (pr & PAD_TRIANGLE) bail(0); if (pr & PAD_R1) bail(1); if (pr & PAD_CIRCLE) { if (landed && spd < 30) { police_check(); leave_trip(); } else say("LAND FIRST. OR TRIANGLE TO BAIL OUT."); } }
    return 1;
}
static u32 shade(u32 c, int k) { int r = MIN(255, (c & 255) * k >> 8), g = MIN(255, ((c >> 8) & 255) * k >> 8), b = MIN(255, ((c >> 16) & 255) * k >> 8); return UI_RGB(r, g, b); }
static void box(int x0, int y0, int z0, int x1, int y1, int z1, u32 c) {
    SVECTOR t0 = { (short)x0, (short)y0, (short)z0 }, t1 = { (short)x1, (short)y0, (short)z0 }, t2 = { (short)x0, (short)y0, (short)z1 }, t3 = { (short)x1, (short)y0, (short)z1 };
    SVECTOR b0 = { (short)x0, (short)y1, (short)z0 }, b1 = { (short)x1, (short)y1, (short)z0 }, b2 = { (short)x0, (short)y1, (short)z1 }, b3 = { (short)x1, (short)y1, (short)z1 };
    u32 top = shade(c, 300), s1 = shade(c, 220), s2 = shade(c, 170);
    gfx_quad_g(&t2, &t3, &t0, &t1, top, top, top, top, 0); gfx_quad_g(&t0, &t1, &b0, &b1, s1, s1, s1, s1, 0); gfx_quad_g(&t3, &t2, &b3, &b2, s1, s1, s1, s1, 0);
    gfx_quad_g(&t2, &t0, &b2, &b0, s2, s2, s2, s2, 0); gfx_quad_g(&t1, &t3, &b1, &b3, s2, s2, s2, s2, 0);
}
static void tbox(int x0, int y0, int z0, int x1, int y1, int z1, int f, u32 c) {   // a box that narrows toward +z: the far end at f/4096 of the width and height
    int cx = (x0 + x1) / 2, cy = (y0 + y1) / 2, hx = (x1 - x0) / 2 * f >> 12, hy = (y1 - y0) / 2 * f >> 12;
    SVECTOR a0 = { (short)x0, (short)y0, (short)z0 }, a1 = { (short)x1, (short)y0, (short)z0 }, a2 = { (short)x0, (short)y1, (short)z0 }, a3 = { (short)x1, (short)y1, (short)z0 };
    SVECTOR b0 = { (short)(cx - hx), (short)(cy - hy), (short)z1 }, b1 = { (short)(cx + hx), (short)(cy - hy), (short)z1 }, b2 = { (short)(cx - hx), (short)(cy + hy), (short)z1 }, b3 = { (short)(cx + hx), (short)(cy + hy), (short)z1 };
    u32 top = shade(c, 300), s1 = shade(c, 200), fr = shade(c, 240);
    gfx_quad_g(&a0, &a1, &b0, &b1, top, top, top, top, 0); gfx_quad_g(&a0, &b0, &a2, &b2, s1, s1, s1, s1, 0); gfx_quad_g(&a1, &a3, &b1, &b3, s1, s1, s1, s1, 0); gfx_quad_g(&b0, &b1, &b2, &b3, fr, fr, fr, fr, 0);
}
static void flat(int x0, int z0, int x1, int z1, int x2, int z2, int x3, int z3, int y, u32 c) {   // a flat panel (a wing, a blade): four corners at height y
    SVECTOR a = { (short)x0, (short)y, (short)z0 }, b = { (short)x1, (short)y, (short)z1 }, d = { (short)x2, (short)y, (short)z2 }, e = { (short)x3, (short)y, (short)z3 };
    u32 t = shade(c, 280); gfx_quad_g(&a, &b, &d, &e, t, t, t, t, 0);
}
static void blade(int cx, int y, int cz, int R, int ang, int w, u32 c) {   // a spinning blade (and its other half) seen from above
    int sx = isin(ang) * R >> 12, sz = icos(ang) * R >> 12, nx = icos(ang) * w >> 12, nz = -isin(ang) * w >> 12;
    flat(cx - sx - nx, cz - sz - nz, cx - sx + nx, cz - sz + nz, cx + sx + nx, cz + sz + nz, cx + sx - nx, cz + sz - nz, y, c);
}
#define GLASS UI_RGB(110, 160, 210)
static void hullb(int L, int HW, int hy, u32 hc, u32 dc) {   // a hull: a body to a point at the bow, a deck on top
    box(-HW, -hy, -L, HW, 8, L / 3, hc); tbox(-HW, -hy, L / 3, HW, 8, L, 700, hc); box(-HW + 3, -hy - 3, -L + 4, HW - 3, -hy, L / 3, dc); tbox(-HW + 3, -hy - 3, L / 3, HW - 3, -hy, L - 8, 600, dc);
}
static int seat[3];   // where the pilot sits (craft space), and whether standing in seat[2] (0 sits)
static void craft_draw(int c, int x, int y, int z, int yw, int pt, int rl, int on_fire) {
    MATRIX m; SVECTOR r = { (short)pt, (short)yw, (short)rl }; RotMatrix(&r, &m); for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) m.m[i][j] = (short)(m.m[i][j] * 7);   // (the models are drawn small: seven times in the world)
    m.t[0] = rel_x(x); m.t[1] = -(y * SC); m.t[2] = rel_z(z); gfx_set_object_m(&m);
    u32 WH = UI_RGB(244, 244, 248), DK = UI_RGB(50, 54, 66), RD = UI_RGB(214, 56, 50), BL = UI_RGB(50, 96, 170), YL = UI_RGB(240, 200, 60), WD = UI_RGB(160, 112, 66);
    seat[0] = 0; seat[1] = -16; seat[2] = 0; (void)on_fire;
    if (c < NB) {
        switch (c) {
        case 0: hullb(70, 22, 18, WD, UI_RGB(190, 150, 100)); box(-20, -22, -10, 20, -18, 0, WD); box(-20, -22, 25, 20, -18, 35, WD); box(-34, -24, 0, -22, -22, 4, DK); box(22, -24, 0, 34, -22, 4, DK); box(-62, -22, 2, -22, -21, 3, DK); seat[1] = -20; seat[2] = -5; break;
        case 1: hullb(78, 18, 14, YL, DK); box(-8, -26, -50, 8, -14, -10, DK); box(-14, -32, 10, 14, -28, 14, DK); box(-2, -38, 12, 2, -28, 14, DK); tbox(-10, -26, 14, 10, -16, 50, 1500, WH); seat[1] = -26; seat[2] = -30; break;
        case 2: hullb(90, 28, 22, BL, WH); box(-18, -48, 4, 18, -22, 40, WH); box(-14, -44, 40, 14, -30, 42, GLASS); box(-6, -30, -100, 6, -8, -94, DK); box(-3, -4, -112, 3, 12, -98, DK); seat[1] = -24; seat[2] = -40; break;
        case 3: hullb(110, 30, 20, RD, WH); box(-26, -24, -20, 26, -21, 30, UI_RGB(60, 60, 70)); tbox(-22, -46, 30, 22, -22, 60, 1800, GLASS); box(-24, -44, -60, 24, -24, -30, DK); box(-8, -30, -110, 8, -10, -100, DK); box(-30, -14, -108, 30, -10, 0, WH); seat[1] = -26; seat[2] = -4; break;
        case 4: hullb(110, 26, 18, WH, UI_RGB(190, 150, 100)); box(-2, -200, 0, 2, -20, 4, WD); box(-1, -190, -50, 1, -40, 8, WH); box(-1, -170, 8, 1, -44, 78, UI_RGB(255, 236, 190)); box(-2, -60, -60, 2, -56, 24, WD); box(-16, -34, -40, 16, -18, 10, UI_RGB(190, 150, 100)); seat[1] = -22; seat[2] = -70; break;
        case 5: hullb(150, 40, 26, BL, UI_RGB(120, 120, 130)); box(-26, -72, -70, 26, -26, -20, WH); box(-20, -62, -20, 20, -40, -16, GLASS); box(-3, -130, -50, 3, -72, -44, DK); box(-3, -112, 40, 3, -28, 46, DK); box(-3, -100, 40, 3, -96, 100, DK); box(-30, -30, 50, 30, -26, 120, UI_RGB(90, 100, 110)); seat[1] = -50; seat[2] = -45; break;
        case 6: hullb(110, 40, 28, DK, RD); box(-26, -90, -40, 26, -28, 30, WH); box(-22, -78, 30, 22, -50, 33, GLASS); box(-26, -100, -40, 26, -90, 30, RD); box(-8, -136, -20, 8, -100, 0, DK); box(-8, -140, -22, 8, -136, 2, RD); for (int k = 0; k < 4; k++) box(-44, -14, -80 + k * 40, -36, -4, -70 + k * 40, DK), box(36, -14, -80 + k * 40, 44, -4, -70 + k * 40, DK); seat[1] = -62; seat[2] = 0; break;
        case 7: hullb(200, 52, 30, WH, UI_RGB(180, 180, 190)); box(-44, -66, -150, 44, -33, 100, WH); box(-44, -100, -120, 44, -66, 40, UI_RGB(236, 236, 240)); for (int k = 0; k < 6; k++) { box(-46, -92, -110 + k * 24, -43, -76, -96 + k * 24, GLASS); box(43, -92, -110 + k * 24, 46, -76, -96 + k * 24, GLASS); box(-46, -58, -140 + k * 36, -43, -44, -124 + k * 36, GLASS); box(43, -58, -140 + k * 36, 46, -44, -124 + k * 36, GLASS); }
            box(-10, -140, -80, 10, -100, -60, RD); box(-30, -116, 50, 30, -100, 80, UI_RGB(236, 236, 240)); box(-24, -112, 80, 24, -102, 83, GLASS); seat[1] = -118; seat[2] = 65; seat[0] = 0; break;
        case 8: box(-52, -12, -66, 52, -2, 66, DK); tbox(-48, -32, -62, 48, -12, 62, 3200, UI_RGB(120, 130, 144)); box(-30, -50, -10, 30, -32, 36, GLASS); box(-14, -64, -52, 14, -32, -36, DK); box(-22, -88, -56, 22, -84, -52, DK); blade(0, -70, -48, 34, anim * 400, 5, DK); box(-8, -50, 36, 8, -32, 52, YL); seat[1] = -34; seat[2] = 6; break;
        default: hullb(240, 46, 24, WH, UI_RGB(150, 120, 80)); box(-46, -30, -230, 46, -24, 240, BL); box(-40, -60, -170, 40, -24, 60, WH); box(-34, -90, -130, 34, -60, 20, WH); for (int k = 0; k < 5; k++) box(-42, -52, -160 + k * 40, -39, -40, -140 + k * 40, GLASS), box(39, -52, -160 + k * 40, 42, -40, -140 + k * 40, GLASS);
            box(-30, -86, 20, 30, -60, 80, WH); box(-26, -74, 80, 26, -64, 83, GLASS); box(-2, -140, -50, 2, -90, -46, DK); box(-14, -126, -52, 14, -122, -44, DK); seat[1] = -62; seat[2] = 40; break;
        }
        return;
    }
    switch (c - NB) {
    case 0: tbox(-7, -10, -50, 7, 4, 40, 1400, WH); tbox(-6, -8, -100, 6, 2, -50, 700, WH); box(-2, -30, -104, 2, -8, -92, WH); box(-18, -3, -100, 18, -1, -92, WH);
        flat(-8, -10, -8, 6, -240, -8, -240, 10, -6, WH); flat(8, -10, 8, 6, 240, -8, 240, 10, -6, WH); box(-6, -16, 4, 6, -10, 34, GLASS); seat[1] = -12; seat[2] = 14; break;
    case 1: tbox(-9, -14, -60, 9, 6, 50, 1500, RD); tbox(-7, -12, -100, 7, 2, -60, 800, RD); box(-2, -40, -104, 2, -12, -90, RD); box(-22, -4, -100, 22, -1, -90, RD);
        flat(-8, -50, -8, 10, -120, 0, -120, 30, -50, UI_RGB(240, 220, 120)); flat(8, -50, 8, 10, 120, 0, 120, 30, -50, UI_RGB(240, 220, 120)); box(-110, -52, -4, 110, -48, 4, UI_RGB(240, 220, 120));
        flat(-8, -10, -8, 6, -120, -4, -120, 16, -10, UI_RGB(220, 60, 50)); flat(8, -10, 8, 6, 120, -4, 120, 16, -10, UI_RGB(220, 60, 50));
        box(-3, -10, 50, 3, -6, 54, DK); blade(0, -8, 56, 30, anim * 900, 4, DK); box(-14, -52, -6, 14, -44, 14, DK);
        box(-1, -50, -36, 1, -12, -34, DK); box(-40, -49, -36, 40, -47, -34, DK); seat[1] = -16; seat[2] = -6; break;
    case 2: tbox(-12, -22, -60, 12, 8, 60, 1600, WH); tbox(-10, -18, -110, 10, 4, -60, 800, WH); box(-18, -3, 8, -10, 14, 70, UI_RGB(180, 190, 200)); box(10, -3, 8, 18, 14, 70, UI_RGB(180, 190, 200));
        box(-150, -44, -10, 150, -40, 24, YL); box(-3, -36, -70, 3, -30, -50, DK); box(-20, -50, -104, 20, -20, -92, WH); box(-2, -60, -108, 2, -20, -92, RD); box(-1, -16, 54, 1, 6, 64, DK); blade(0, -8, 64, 34, anim * 1100, 4, DK); seat[1] = -26; seat[2] = -8; break;
    case 3: tbox(-9, -14, -80, 9, 4, 40, 1200, DK); box(-8, -16, 10, 8, 4, 30, DK); box(-4, -40, 10, 4, -16, 18, DK); box(-34, -16, -2, 34, -12, 4, UI_RGB(180, 180, 190)); box(-24, -3, 10, 24, 0, 16, DK);
        box(-3, -20, 50, 3, -14, 56, UI_RGB(240, 140, 60)); blade(0, -14, 58, 26, anim * 800, 4, DK); box(-1, -40, 70, 1, -36, 90, DK); box(-70, -26, 36, 70, -22, 52, YL); box(-70, -26, 36, -66, -14, 52, YL); box(66, -26, 36, 70, -14, 52, YL); box(-50, -8, -10, 50, -6, 4, UI_RGB(60, 120, 200)); box(-60, -18, -120, 60, -14, -100, YL); seat[1] = -18; seat[2] = -4; break;
    case 4: box(-14, -26, -14, 14, -4, 14, WD); for (int k = 0; k < 4; k++) { int xx = k & 1 ? 12 : -12, zz = k & 2 ? 12 : -12; box(xx - 1, -80, zz - 1, xx + 1, -26, zz + 1, DK); }
        box(-12, -10, -12, 12, -2, 12, UI_RGB(100, 70, 40)); tbox(-52, -150, -52, 52, -80, 52, 4096, RD); tbox(-44, -210, -44, 44, -150, 44, 3200, YL); tbox(-27, -250, -27, 27, -210, 27, 2400, RD); tbox(-60, -112, -60, 60, -80, 60, 4096, BL); box(-5, -84, -5, 5, -78, 5, UI_RGB(255, 140, 40)); seat[1] = -4; seat[2] = 1; break;
    case 6: tbox(-26, -40, -300, 26, 14, 220, 900, WH); tbox(-24, -36, 220, 24, 8, 330, 500, WH); box(-26, -4, -300, 26, 6, 220, UI_RGB(60, 90, 160)); box(-14, -50, 200, 14, -34, 236, GLASS);
        flat(-26, -10, -26, 40, -330, 20, -330, 130, -2, UI_RGB(200, 204, 214)); flat(26, -10, 26, 40, 330, 20, 330, 130, -2, UI_RGB(200, 204, 214));
        box(-150, -6, -20, -118, 14, 60, UI_RGB(130, 136, 150)); box(118, -6, -20, 150, 14, 60, UI_RGB(130, 136, 150)); box(-2, -150, -280, 2, -30, -220, UI_RGB(60, 90, 160)); flat(-2, -130, -2, -280, -120, -300, -120, -250, -140, WH); flat(2, -130, 2, -280, 120, -300, 120, -250, -140, WH);
        for (int k = 0; k < 8; k++) { box(-27, -30, -250 + k * 55, -26, -20, -236 + k * 55, GLASS); box(26, -30, -250 + k * 55, 27, -20, -236 + k * 55, GLASS); } seat[1] = -30; seat[2] = 210; break;
    case 7: tbox(-30, -52, -260, 30, 18, 230, 800, UI_RGB(170, 176, 150)); tbox(-26, -48, 230, 26, 6, 320, 500, UI_RGB(170, 176, 150)); box(-24, -62, 190, 24, -46, 232, GLASS); box(-28, 6, -250, 28, 20, 200, DK);
        flat(-30, -50, -30, 20, -300, 0, -300, 90, -48, UI_RGB(150, 156, 130)); flat(30, -50, 30, 20, 300, 0, 300, 90, -48, UI_RGB(150, 156, 130)); box(-110, -40, -30, -90, -12, 50, DK); box(90, -40, -30, 110, -12, 50, DK);
        box(-2, -170, -250, 2, -50, -200, UI_RGB(150, 156, 130)); flat(-2, -165, -2, -260, -110, -285, -110, -235, -166, UI_RGB(150, 156, 130)); flat(2, -165, 2, -260, 110, -285, 110, -235, -166, UI_RGB(150, 156, 130)); seat[1] = -44; seat[2] = 205; break;
    default: tbox(-9, -14, -60, 9, 8, 90, 1500, UI_RGB(170, 180, 196)); tbox(-8, -12, -120, 8, 6, -60, 900, UI_RGB(170, 180, 196)); tbox(-6, -16, 90, 6, 4, 150, 500, UI_RGB(40, 40, 50)); box(-9, -22, 20, 9, -14, 60, GLASS);
        flat(-8, -4, -8, 50, -90, -60, -90, -30, -2, UI_RGB(140, 150, 170)); flat(8, -4, 8, 50, 90, -60, 90, -30, -2, UI_RGB(140, 150, 170)); flat(-8, -118, -8, -90, -60, -130, -60, -112, -8, UI_RGB(140, 150, 170)); flat(8, -118, 8, -90, 60, -130, 60, -112, -8, UI_RGB(140, 150, 170));
        box(-1, -44, -120, 1, -12, -86, RD); box(-16, 0, 30, -8, 10, 70, DK); box(8, 0, 30, 16, 10, 70, DK); box(-8, 2, -118, -2, 14, -108, UI_RGB(255, 150, 60)); box(2, 2, -118, 8, 14, -108, UI_RGB(255, 150, 60)); seat[1] = -22; seat[2] = 40; break;
    }
    (void)0;
}
static void pilot_draw(int x, int y, int z, int yw, int pt, int stand) {   // your Bore, life size, in the seat
    Gait g; memset(&g, 0, sizeof g); g.clock = anim; g.mood = 60; g.sit = !stand; g.seed = 5;
    int sn = isin(yw & 4095), cs = icos(yw & 4095);
    VECTOR p = { rel_x(x) + (((seat[0] * cs + seat[2] * sn) >> 12) * 7), -(y * SC - seat[1] * 7 - ((seat[2] * isin(pt & 4095) >> 12) * 7)), rel_z(z) + (((-seat[0] * sn + seat[2] * cs) >> 12) * 7) };
    voxel_scale = 4096; voxel_draw(&sims[player].bore, &p, yw, &g); gfx_shadow(&p, 100);
}
static void faller_draw(void) {   // you, falling or hanging from the canopy
    static VPose up, fall; Gait g; memset(&g, 0, sizeof g); g.clock = anim; g.mood = 60; g.seed = 5;
    up.arm[0] = up.arm[1] = -3300; up.leg[0] = 80; up.leg[1] = -80;
    fall.arm[0] = fall.arm[1] = -1500; fall.aroll[0] = -1000; fall.aroll[1] = 1000; fall.lroll[0] = -250; fall.lroll[1] = 250; fall.knee[0] = fall.knee[1] = 200; fall.lean = 700;
    g.pose = chute_on ? &up : &fall; int X = rel_x(cx), Z = rel_z(cz_), Y = cy_ * SC; VECTOR p = { X, -Y, Z };
    voxel_scale = 4096; voxel_draw(&sims[player].bore, &p, yaw, &g);
    gfx_set_identity(); if (chute_on) { tbox(X - 700, -Y - 1500, Z - 700, X + 700, -Y - 1050, Z + 700, 4096, UI_RGB(230, 60, 80)); box(X - 230, -Y - 1480, Z - 700, X + 230, -Y - 1070, Z + 700, UI_RGB(250, 250, 250)); box(X - 8, -Y - 1050, Z - 8, X + 8, -Y - 420, Z + 8, UI_RGB(40, 40, 40)); }
}
static void hud_bar(int x, int y, int w, int v, int vmax, u32 c) { ui_round(x, y, w, 7, 3, UI_RGB(40, 44, 60), UI_RGB(24, 26, 40), 0); if (v > 0) ui_round(x, y, MAX(4, w * MIN(v, vmax) / MAX(1, vmax)), 7, 3, c, c, 0); }
// ----- the picture: the game's own world draw, with the craft, the pilot, fires and blasts put in -----
static void flames(int x, int y, int z, int n, int sz, int sd) {   // fire (world units, y up)
    for (int k = 0; k < n; k++) { u32 h = hs(k * 31 + sd, anim / 3 + k * 7); int ox = (int)(h & 63) * sz / 32 - sz, oz = (int)((h >> 6) & 63) * sz / 32 - sz, hh = sz / 2 + (int)((h >> 12) & 255) * sz / 160, w = sz / 5 + (k & 3) * 10;
        box(x + ox - w, -y - hh, z + oz - w, x + ox + w, -y, z + oz + w, k & 1 ? UI_RGB(255, 150, 30) : UI_RGB(255, 220, 70));
        box(x + ox - w / 2, -y - hh * 2, z + oz - w / 2, x + ox + w / 2, -y - hh, z + oz + w / 2, UI_RGB(70 + (k & 1) * 30, 60, 56)); }   // (and the smoke over it)
}
static void effects(void) {
    gfx_set_identity();
    for (int i = 0; i < nrub; i++) { Rub *r = &rub[i]; int X = rel_x(r->x), Z = rel_z(r->z); int g = MAX(0, gh(r->x, r->z)) * SC, hh = MIN(500, r->top * SC / 3);
        box(X - 380, -g - hh / 2, Z - 300, X - 60, -g, Z + 200, UI_RGB(86, 74, 66)); box(X + 40, -g - hh, Z - 340, X + 400, -g, Z + 40, UI_RGB(110, 96, 86)); box(X - 200, -g - hh / 3, Z + 100, X + 150, -g, Z + 380, UI_RGB(70, 60, 54));
        if (r->age < 3600) flames(X, g, Z, 4 + (r->top > 400), 160 + MIN(200, r->top / 2), i); }
    for (int i = 0; i < 4; i++) { Boom *b = &boom[i]; if (!b->age) continue; int X = b->x * SC - org_x(), Z = b->z * SC + org_z(), Y = b->y * SC, R = 250 + b->sev * 28, a = b->age, r = R * MIN(a, 36) / 36;
        if (a < 110) { box(X - r, -Y - r, Z - r, X + r, -Y + r / 3, Z + r, a < 60 ? UI_RGB(255, 120, 30) : UI_RGB(90, 70, 60)); if (a < 70) box(X - r / 2, -Y - r * 3 / 4, Z - r / 2, X + r / 2, -Y + r / 4, Z + r / 2, a < 30 ? UI_RGB(255, 250, 200) : UI_RGB(255, 200, 60)); }
        if (a > 20 && a < 200) { int sm = R * (a - 20) / 100; box(X - R / 3, -Y - sm, Z - R / 3, X + R / 3, -Y, Z + R / 3, UI_RGB(60, 56, 54)); }
        if (a < 60) flames(X, Y, Z, 5, R / 3, i + 9); }
}
static void markers(void) {   // the harbours' posts and the job's marker
    gfx_set_identity();
    if (mode == M_BOAT) { for (int i = 0; i < nh; i++) { int X = rel_x(hx[i]), Z = rel_z(hz[i]); if (ABS(X - rel_x(px)) < 60000 && ABS(Z - rel_z(pz)) < 60000) { box(X - 90, -2200, Z - 90, X + 90, 0, Z + 90, UI_RGB(240, 200, 60)); box(X - 200, -100, Z - 200, X + 200, 0, Z + 200, UI_RGB(150, 110, 70)); } }
        if (jtype) { int X = rel_x(jx), Z = rel_z(jz); box(X - 150, -3000, Z - 150, X + 150, 0, Z + 150, jtype == 2 ? UI_RGB(255, 220, 80) : UI_RGB(255, 80, 60)); } }
}
static void sky_light(void) {
    int t = clock_min, day = (t > 360 && t < 1200) ? 255 : (t >= 1200 && t < 1320) ? 255 - (t - 1200) * 2 : (t >= 300 && t <= 360) ? (t - 300) * 4 : 20; day = CLAMP(day, 20, 255);
    gfx_set_clear((20 * 8 * day) >> 8, (26 * 8 * day) >> 8, (31 * 8 * day) >> 8); gfx_set_ambient(54 + (day * 36 >> 8), 48 + (day * 60 >> 8));
    int tr = (70 * day) >> 8, tg = (120 * day) >> 8, tb = (220 * day) >> 8, br = (215 * day) >> 8, bg = (205 * day) >> 8, bb = (190 * day) >> 8;
    if (world_zone == ZONE_TOWN) { tr = (110 * day) >> 8; tg = (140 * day) >> 8; tb = (200 * day) >> 8; br = (240 * day) >> 8; bg = (190 * day) >> 8; bb = (120 * day) >> 8; }
    if (world_zone == ZONE_ISLAND) { tr = (40 * day) >> 8; tg = (140 * day) >> 8; tb = (230 * day) >> 8; br = (200 * day) >> 8; bg = (230 * day) >> 8; bb = (240 * day) >> 8; }
    u32 top = (u32)tr | ((u32)tg << 8) | ((u32)tb << 16), hor = (u32)br | ((u32)bg << 8) | ((u32)bb << 16);
    world_horizon = hor; world_far_range = (city() && far_view) ? 60 : 34;
    if (city() && far_view) far_sky(top, hor);
    else if (world_zone == ZONE_ISLAND) far_sky_sea(top, hor, (u32)((40 * day) >> 8) | ((u32)((112 * day) >> 8) << 8) | ((u32)((168 * day) >> 8) << 16));
    else if (world_zone == ZONE_HOME) far_sky_sea(top, hor, (u32)((64 * day) >> 8) | ((u32)((120 * day) >> 8) << 8) | ((u32)((52 * day) >> 8) << 16));
    else if (world_zone == ZONE_TOWN) far_sky_sea(top, hor, (u32)((196 * day) >> 8) | ((u32)((160 * day) >> 8) << 8) | ((u32)((108 * day) >> 8) << 16));
    else gfx_sky(top, hor);
}
static void scene(void) {
    int fx = mode == M_CHUTE ? cx : mode == M_WRECK ? wrx : px, fy = mode == M_CHUTE ? cy_ : mode == M_WRECK ? wry : py, fz = mode == M_CHUTE ? cz_ : mode == M_WRECK ? wrz : pz;
    int cyw = mode == M_WRECK ? (anim * 9) & 4095 : yaw, dist = mode == M_BOAT ? 2400 : mode == M_AIR ? (cur >= NB + 6 ? 6200 : 2900) : mode == M_CHUTE ? 2000 : 3200, up = mode == M_BOAT ? 900 : mode == M_AIR ? (cur >= NB + 6 ? 1900 : 800) : 1100;
    int X = rel_x(fx), Z = rel_z(fz), Y = fy * SC;
    cam.at.vx = X; cam.at.vy = -(Y + 200); cam.at.vz = Z;
    cam.eye.vx = X - ((isin(cyw) * dist) >> 12); cam.eye.vz = Z - ((icos(cyw) * dist) >> 12); cam.eye.vy = -(Y + up);
    { int ge = city() ? world_gh(cam.eye.vx, cam.eye.vz) : 0; if (-cam.eye.vy < ge + 350) cam.eye.vy = -(ge + 350); }
    gfx_camera_update(); sky_light(); gfx_prim_count = 0;
    { int ftx = CLAMP(X >> 8, 0, MW - 1), fty = CLAMP((-Z) >> 8, 0, MH - 1); world_draw(ftx, fty, anim, 0); }
    gfx_set_identity(); gfx_set_fog(0, 0);
    markers(); effects();
    if (mode == M_BOAT || mode == M_AIR) { craft_draw(cur, px, py, pz, yaw, mode == M_AIR ? pitch : 0, roll, 0); if (cur < NB + 6) pilot_draw(px, py, pz, yaw, mode == M_AIR ? pitch : 0, (cur < NB && (cur == 5 || cur == 6 || cur == 7 || cur == 9 || cur == 4)) || cur == NB + 4); }
    else if (mode == M_WRECK) { craft_draw(cur, wrx, MAX(0, gh(wrx, wrz)) + 4, wrz, yaw, 420, 900, 0); gfx_set_identity(); flames(rel_x(wrx), MAX(0, gh(wrx, wrz)) * SC + 60, rel_z(wrz), 6, 260, 3); }
    else { if (!crashed) craft_draw(cur, ax, ay, az, ayaw, apitch, aroll, 0); faller_draw(); }
}
void skysea_draw(void) {
    char b[64];
    if (mode >= M_BOAT && mode <= M_WRECK) {
        scene();
        // the HUD
        ui_round(8, 8, 150, 44, 6, UI_RGB(14, 18, 34), UI_RGB(30, 36, 60), 0);
        if (mode == M_BOAT) { sprintf(b, "%s  %d KNOTS", CNM[cur], ABS(spd) / 5); ui_text(14, 12, 1, b, UI_WHITE); hud_bar(14, 24, 60, fuel, BT[cur].fuel * 2, UI_RGB(240, 200, 60)); hud_bar(14, 35, 60, hull, BT[cur].hull * 2, UI_RGB(80, 200, 100)); ui_text(80, 23, 1, "FUEL", UI_SOFT); ui_text(80, 34, 1, "HULL", UI_SOFT); hud_bar(112, 24, 40, temp, 130, temp > 100 ? UI_RGB(240, 70, 50) : UI_RGB(240, 150, 60)); ui_text(118, 34, 1, "HEAT", UI_SOFT); hud_bar(14, 45, 138, thr + 30, 130, UI_RGB(80, 160, 240)); }
        else if (mode == M_AIR) { sprintf(b, "%s  %d  ALT %d", CNM[cur], spd / 5, MAX(0, py - MAX(0, gh(px, pz)))); ui_text(14, 12, 1, b, UI_WHITE); hud_bar(14, 24, 60, fuel, AT[cur - NB].fuel * 2 + 1, UI_RGB(240, 200, 60)); hud_bar(14, 35, 60, hull, AT[cur - NB].hull * 2, UI_RGB(80, 200, 100)); ui_text(80, 23, 1, "FUEL", UI_SOFT); ui_text(80, 34, 1, "HULL", UI_SOFT); hud_bar(14, 45, 138, thr, 100, UI_RGB(80, 160, 240)); if (spd < AT[cur - NB].stall * 16 && AT[cur - NB].stall) ui_text(104, 12, 1, "STALL!", UI_BADT); }
        else { ui_text(14, 12, 1, chute_on ? "PARACHUTE" : "FALLING!", UI_WHITE); sprintf(b, "ALT %d", MAX(0, cy_ - MAX(0, gh(cx, cz_)))); ui_text(14, 24, 1, b, UI_WHITE); }
        sprintf(b, "$%d", (int)cash); ui_text(312 - ui_tw(1, b), 10, 1, b, UI_GOODT);
        if (hot) ui_text(312 - ui_tw(1, "HOT CRAFT"), 22, 1, "HOT CRAFT", UI_BADT);
        if (mode == M_BOAT && jtype) { int dx = (jx - px) / 8, dz = (jz - pz) / 8; sprintf(b, jtype == 2 ? "FISH %d/%d  %s" : "JOB  %dM  %ds", jtype == 2 ? fish : isq(dx * dx + dz * dz) * 8 * SC / 213, jtype == 2 ? BT[cur].hold : jt / 60, jtype == 2 ? (fcast ? "..." : "STOP IN THE RING") : ""); ui_text(14, 56, 1, b, UI_WHITE); }
        if (mode == M_BOAT && near_dock() >= 0 && ABS(spd) < 60) ui_text(160, 200, 1, "TRIANGLE: HARBOUR  O: GET OFF", UI_WHITE);
        if (mode == M_WRECK) { ui_textc(160, 30, 1, line, UI_WHITE); ui_textc(160, 44, 1, line2, UI_WHITE); }
        if (ctr > 0) ui_textc(160, 180, 1, line, UI_WHITE);
        ui_hints(160, 218, mode == M_BOAT ? "X GAS|SQ BRAKE|PAD STEER" : mode == M_AIR ? "X THRUST|SQ LESS|TRI CHUTE|R1 JUMP" : mode == M_WRECK ? "X SKIP" : "PAD STEER", 1);
        return; }
    gfx_set_clear(110, 190, 240);
    ui_round(0, 0, 320, 240, 0, UI_RGB(140, 210, 255), UI_RGB(60, 150, 210), 0); ui_round(0, 150, 320, 90, 0, UI_RGB(30, 130, 190), UI_RGB(20, 90, 150), 0);
    for (int i = 0; i < 4; i++) ui_round((i * 91 + anim / 2) % 340 - 20, 20 + i * 14, 50, 14, 7, UI_RGB(255, 255, 255), UI_RGB(225, 235, 245), 0);
    if (mode == M_END) { int top = ui_window(30, 40, 260, 150, "TRIP OVER", IC_PIN); ui_wrap(40, top + 4, 240, 3, line, UI_INK); ui_wrap(40, top + 42, 240, 4, line2, UI_SOFT); sprintf(b, "CASH $%d", (int)cash); ui_text(40, top + 100, 1, b, UI_GOODT); ui_hints(160, 176, "X OK", 1); return; }
    int top = ui_window(8, 6, 304, 228, "SKY AND SEA", IC_CAR);
    sprintf(b, "$%d", (int)cash); ui_text(300 - ui_tw(1, b), top + 1, 1, b, UI_GOODT); ui_text(16, top + 1, 1, "BOATS AND AIRCRAFT", UI_SOFT);
    int n = nrows(), first = CLAMP(sel - 3, 0, MAX(0, n - 7)), y0 = top + 14;
    for (int k = 0; k < 7 && first + k < n; k++) {
        int i = first + k, y = y0 + k * 15, on = i == sel; if (on) ui_round(14, y - 2, 292, 15, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
        if (i == NCR) { int d = MIN(30, MAX(0, day_count - last())); sprintf(b, "COLLECT  %d DAYS = $%d", d, rate() * d); ui_text(20, y + 1, 1, b, on ? UI_BLUE2 : UI_GOODT); continue; }
        if (i == NCR + 4) { int k = (pk_data[39] >> 3) & 3; ui_text(20, y + 1, 1, k == 0 ? "BLAST SCALE: NORMAL" : k == 1 ? "BLAST SCALE: LOW" : "BLAST SCALE: EXTREME", on ? UI_BLUE2 : UI_INK); continue; }
        if (i == NCR + 3) { ui_text(20, y + 1, 1, dbg() ? "DEBUG: EVERY CRAFT FREE (ON)" : "DEBUG: EVERY CRAFT FREE (OFF)", on ? UI_BLUE2 : UI_INK); continue; }
        if (i == NCR + 2) { ui_text(20, y + 1, 1, pk_data[39] & 2 ? "FALL DAMAGE: ON" : "FALL DAMAGE: OFF", on ? UI_BLUE2 : UI_INK); continue; }
        if (i == NCR + 1) { ui_text(20, y + 1, 1, real() ? "CRASH MODE: REAL (THE TOWN PAYS)" : "CRASH MODE: ARCADE (BOUNCE BACK)", on ? UI_BLUE2 : UI_INK); continue; }
        int h = has(i); sprintf(b, "%s%s", CNM[i], stol(i) ? " (HOT)" : ""); ui_text(20, y + 1, 1, b, h ? (on ? UI_BLUE2 : UI_GOODT) : on ? UI_BLUE2 : UI_INK);
        if (h) { sprintf(b, "DRIVE T%d", tune(i)); ui_text(300 - ui_tw(1, b), y + 1, 1, b, UI_GOODT); } else { sprintf(b, "$%d", price(i)); ui_text(300 - ui_tw(1, b), y + 1, 1, b, cash >= price(i) ? UI_INK : UI_BADT); }
    }
    int by = top + 124; ui_round(14, by, 292, 62, 5, UI_PANEL, UI_LINE, 0);
    if (sel < NCR) { sprintf(b, "%s  %s", CNM[sel], sel < NB ? "BOAT" : "AIRCRAFT"); ui_text(20, by + 4, 1, b, UI_BLUE2); ui_wrap(20, by + 16, 280, 2, CHINT[sel], UI_INK); sprintf(b, "EARNS $%d A DAY  SQ TUNES UP", inc(sel)); ui_text(20, by + 40, 1, b, UI_SOFT); }
    else { sprintf(b, "%d CRAFT  $%d A DAY", __builtin_popcount(owned()), rate()); ui_text(20, by + 4, 1, b, UI_BLUE2); sprintf(b, "LANDINGS %d  DOWN %d  HURT %d  KILLED %d", pk_data[28], pk_data[37], pk_data[38], pk_data[29]); ui_text(20, by + 18, 1, b, UI_INK); }
    ui_text(20, by + 50, 1, line, UI_RED);
    ui_hints(160, 218, "X PICK|O LEAVE", 1);
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, skysea_open, skysea_update, skysea_draw };
