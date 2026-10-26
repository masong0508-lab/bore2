// skysea_ui: SKY AND SEA, an overlay (SKYSEA.OVL): ten boats and six aircraft you really drive and fly, over an archipelago with a town on it. It stands alone.
//  BOATS (deep): fuel, an engine that overheats, hull damage, waves and wind (sails want a crosswind), tuning, jobs (ferry, cargo, fishing, a buoy race), a harbour to
//   refuel and repair at, and a tow home if you sink.
//  AIRCRAFT: throttle, pitch, bank, lift and stall. Land on the runway (or the sea, in the seaplane) for pay. Crash in CRASH MODE: REAL and the town pays for it: buildings
//   come down, people are hurt, the police write it down (and the pilot goes to hospital). PARACHUTE (TRIANGLE) gets you out of a plane in trouble: the plane flies on without you.
//  Stolen craft (the CRIME AND LAW pack's underworld, pk_data 40-41) can be taken out too: a landing may find the police waiting.
// pk_data: 24-25 craft owned, 26-27 earnings collected to that day, 28 jobs and landings done, 32-34 boat tuning (2 bits each), 35-36 aircraft tuning, 37 buildings brought down, 38 people hurt,
//  39 bit 0: CRASH MODE REAL.
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "ovl.h"
#include "wants.h"
#include <psxgpu.h>
#include <psxgte.h>
#include <psxpad.h>
#include <stdio.h>
#include <string.h>

#define NB 10
#define NA 6
#define NCR (NB + NA)
enum { M_MENU, M_BOAT, M_AIR, M_CHUTE, M_END };
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
    { "HOT AIR BALLOON",24, 0, 10, 10, 70, 200, 11000,  300, 6 }, { "JET",         400, 90, 34, 56, 150, 500, 90000, 3000, 0 } };
static const char *const CNM[NCR] = { "ROWBOAT", "JET SKI", "FISHING SKIFF", "SPEEDBOAT", "SAILBOAT", "TRAWLER", "TUGBOAT", "FERRY", "HOVERCRAFT", "SUPER YACHT", "GLIDER", "BIPLANE", "SEAPLANE", "HELICOPTER", "HOT AIR BALLOON", "JET" };
static const char *const CHINT[NCR] = { "OARS ONLY. FISH ALL DAY.", "FAST AND LIGHT. WAVES TOSS IT.", "SMALL HOLD. FISHES THE REEF.", "FAST. THIRSTY. RACES.", "FREE WIND. NEEDS A CROSSWIND.", "BIG HOLD. SLOW. FISHES BIG.", "PUSHES ANYTHING. TURNS ON A COIN.", "A BIG HOLD OF PASSENGERS.", "SKIMS WATER AND SAND.", "LUXURY AND SPEED.",
    "NO ENGINE. FIND THE LIFT.", "OPEN COCKPIT. FORGIVING.", "LAND ON THE WAVES.", "HOVERS. LANDS ANYWHERE.", "UP, DOWN, AND DRIFT.", "FAST. HARD TO LAND." };
// the world
#define CELL 320
#define RAD 7
#define NIS 5
static const s16 IS[NIS][5] = { { 0, 0, 3600, 260, 0 }, { 7600, 0, 2000, 60, 55 }, { -6500, 4200, 2600, 1500, 0 }, { 2500, -6200, 1200, 130, 0 }, { -3000, -6500, 1500, 300, 0 } };
static const s16 DK[4][2] = { { 0, -4300 }, { 7600, -2500 }, { -6500, 1300 }, { -3000, -4500 } };   // the harbours
#define RW_X0 6900
#define RW_X1 8300
#define RW_Z 130
static u32 dead[8];   // the town's buildings that are down (bit per lattice cell)
static int mode, tab, sel, anim; static char line[56], line2[56];
static unsigned rs = 5;
static int rnd(int n) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (int)((rs >> 3) % (unsigned)n); }
static u32 hs(int x, int y) { u32 h = (u32)x * 73856093u ^ (u32)y * 19349663u; h ^= h >> 13; h *= 0x5bd1e995u; return h ^ (h >> 15); }
static int isq(int v) { int r = 0, b = 1 << 28; if (v <= 0) return 0; while (b > v) b >>= 2; while (b) { if (v >= r + b) { v -= r + b; r = (r >> 1) + b; } else r >>= 1; b >>= 2; } return r; }
static int vn(int x, int z, int sc) { int gx = x / sc, gz = z / sc, fx = x % sc, fz = z % sc; if (x < 0) { gx--; fx += sc; } if (z < 0) { gz--; fz += sc; }
    int a = hs(gx, gz) & 255, b = hs(gx + 1, gz) & 255, c = hs(gx, gz + 1) & 255, d = hs(gx + 1, gz + 1) & 255; int t = a + (b - a) * fx / sc, u = c + (d - c) * fx / sc; return t + (u - t) * fz / sc; }
static int gh(int x, int z) {   // the ground: 0 and under is the sea
    int best = -20;
    for (int i = 0; i < NIS; i++) { int dx = x - IS[i][0], dz = z - IS[i][1], r = IS[i][2]; if (ABS(dx) > r || ABS(dz) > r) continue; int d = isq(dx * dx + dz * dz); if (d >= r) continue;
        int t = (r - d) * 4096 / r, h; if (IS[i][4]) { h = t > 1600 ? IS[i][4] : t * IS[i][4] / 1600; if (h > 0) h += 0; } else { int sh = (t * t) >> 12; h = IS[i][3] * sh >> 12; h += (vn(x, z, 380) - 128) * IS[i][3] / 900; }
        h -= 14; if (h > best) best = h; }
    return best;
}
static int on_strip(int x, int z) { return x > RW_X0 && x < RW_X1 && ABS(z) < RW_Z; }
// the town: a lattice of lots on the big island
#define TS 360
static int bld(int i, int j, int *x, int *z, int *h, int *pop) {   // lot (i, j), -7..7: its place, height and people; 0 if there is nothing there
    if (i < -7 || i > 7 || j < -7 || j > 7) return 0; u32 k = hs(i + 31, j + 17); *x = i * TS; *z = j * TS;
    if ((int)(k % 100) > 58 || gh(*x, *z) < 30 || (ABS(i) < 1 && ABS(j) < 1 && 0)) return 0; *h = 100 + (int)((k >> 8) % 260); *pop = (int)((k >> 16) % 4); return 1; }
static int bit(int i, int j) { return (i + 7) * 15 + (j + 7); }
static int is_dead(int i, int j) { int b = bit(i, j); return dead[b >> 5] >> (b & 31) & 1; }
// ----- the craft you own -----
static int owned(void) { return pk_data[24] | pk_data[25] << 8; }
static int stol(int c) { int s = pk_data[40] | pk_data[41] << 8; return (c == 3 && (s >> 5 & 1)) || (c == 9 && (s >> 6 & 1)) || (c == 11 && (s >> 7 & 1)) || (c == 12 && (s >> 8 & 1)) || (c == 15 && (s >> 9 & 1)); }
static int has(int c) { return (owned() >> c & 1) || stol(c); }
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
static int cur;                        // the craft
static int px, py, pz, yaw, pitch, roll, spd, vy, thr, fuel, hull, temp, hold, fish, jt, jtype, jx, jz, jpay, jn, jfish, fcast, wantd, hot, landed, crashed, ctr, sunk, sx, sz, svx, svz;   // (py is height; spd in 1/16 units; roll/pitch/yaw in 4096ths)
static int cyaw, cvy, canopy;
static int windd;                      // the wind's direction
static int chute_on, cx, cy_, cz_, cdrop;
static int ax, ay, az, ayaw, aspd, apitch, aroll;   // the unmanned plane after the pilot bails out
static void say(const char *s) { strncpy(line, s, 55); line[55] = 0; ctr = 200; }
static void spawn_boat(int c) {
    cur = c; int d = rnd(1); (void)d; px = DK[0][0]; pz = DK[0][1]; while (gh(px, pz) > -8) pz -= 100; yaw = 0; roll = pitch = 0; spd = 0; thr = 0; py = 0;
    int tl = tune(c); fuel = BT[c].fuel * (100 + tl * 30) / 100; hull = BT[c].hull * (100 + tl * 25) / 100; temp = 0; hold = 0; fish = 0; jtype = 0; jt = 0; crashed = sunk = 0; mode = M_BOAT; line[0] = 0; ctr = 0; hot = stol(c);
}
static void spawn_air(int c) {
    int a = c - NB; cur = c; px = RW_X0 + 160; pz = 0; py = gh(px, pz) + 6; yaw = 1024; pitch = roll = 0; spd = 0; thr = 0; vy = 0; fuel = AT[a].fuel * (100 + tune(c) * 25) / 100; hull = AT[a].hull * (100 + tune(c) * 25) / 100;
    if (AT[a].fl & 4) fuel = 400 + tune(c) * 100; if (a == 0) { fuel = 9999; px = RW_X0 + 400; py += 360; spd = 70 * 16; thr = 0; }   // (the glider is towed up: it starts in the air)
    landed = py <= gh(px, pz) + 8; crashed = 0; mode = M_AIR; line[0] = 0; ctr = 0; hot = stol(c); chute_on = 0;
}
static void hurt(int n) { hull -= n; if (hull <= 0 && !crashed) { crashed = 1; } }
static void blow(int x, int z, int rad) {   // a crash in the town: what is near comes down
    int nb = 0, hurtn = 0;
    for (int i = -7; i <= 7; i++) for (int j = -7; j <= 7; j++) { int bx, bz, bh, pop; if (!bld(i, j, &bx, &bz, &bh, &pop) || is_dead(i, j)) continue;
        int dx = bx - x, dz = bz - z; if (dx * dx + dz * dz > rad * rad) continue;
        int b = bit(i, j); dead[b >> 5] |= 1u << (b & 31); nb++; hurtn += pop + (pop ? rnd(2) : 0); }
    if (nb) { pk_data[37] = (u8)MIN(255, pk_data[37] + nb); pk_data[38] = (u8)MIN(255, pk_data[38] + hurtn);
        for (int i = 0; i < 1 + nb / 3; i++) sim_crime(CR_ASSAULT, 20 + nb * 3); if (hurtn) sim_crime(CR_ASSAULT, 40); cash -= 300 * nb; if (cash < 0) cash = 0;
        sprintf(line2, "%d BUILDINGS DOWN, %d PEOPLE HURT", nb, hurtn); sims[player].mood = MAX(-100, sims[player].mood - 20 - nb * 4); }
    else strcpy(line2, "YOU HIT THE GROUND. NOBODY WAS HURT");
}
static void end_trip(const char *a, const char *b) { strcpy(line, a); if (b) strcpy(line2, b); mode = M_END; ctr = 0; clock_min = MIN(1439, clock_min + 45); audio_voice(crashed || sunk ? VOICE_CRY : VOICE_WIN); }
static void police_check(void) { if (hot && rnd(100) < 25 + pk_data[42] / 2) { sim_crime(CR_THEFT, 20); sim_arrest(1); strcat(line2, "  THE POLICE WERE WAITING!"); } }
static void crash_plane(int x, int z, int unmanned) {
    int a = cur - NB; (void)a; crashed = 1;
    if (real()) { blow(x, z, 520); if (!unmanned) { cash = MAX(0, cash - 2000); sims[player].mood = MAX(-100, sims[player].mood - 40); clock_min = MIN(1439, clock_min + 180);
        end_trip("THE PLANE CRASHED!", line2[0] ? line2 : "YOU WENT TO HOSPITAL. $2000 IN BILLS."); strcat(line2, "  YOU WENT TO HOSPITAL. -$2000"); } else { sprintf(line, "THE EMPTY PLANE CRASHED!"); ctr = 300; mode = M_END; } }
    else { cash = MAX(0, cash - 500); end_trip("THE PLANE CRASHED!", "ARCADE MODE: REPAIRS COST $500."); }
    audio_voice(VOICE_CRY);
}
static void bail(int jump) {   // out of the plane
    if (!(mode == M_AIR)) return; int a = cur - NB; int alt = py - MAX(0, gh(px, pz));
    if ((AT[a].fl & 4) && !jump) { say("A BALLOON LOWERS ITSELF. NO NEED."); return; }
    ax = px; ay = py; az = pz; ayaw = yaw; aspd = spd; apitch = pitch; aroll = roll; cx = px; cy_ = py; cz_ = pz; cdrop = 0; mode = M_CHUTE; chute_on = !jump && alt > 120; cvy = jump ? 14 : 0; if (jump) { aspd = 0; }
    say(jump ? "YOU JUMP! R1 OPENS THE CHUTE." : chute_on ? "CHUTE OPEN! R1 CLOSES IT." : "TOO LOW FOR A CHUTE!"); audio_voice(VOICE_YAHOO);
}
// ----- physics -----
static void boat_step(int hd) {
    const Boat *b = &BT[cur]; int tl = tune(cur), vm = b->vmax * (100 + tl * 10) / 100;
    if (hd & PAD_CROSS) thr = MIN(100, thr + 2); if (hd & PAD_SQUARE) thr = MAX(-30, thr - 3);
    if (!(hd & (PAD_CROSS | PAD_SQUARE)) && b->kind == 0) thr = thr * 31 / 32;
    int sea = (isin((anim * 7) & 4095) >> 9) + 1;   // the swell, 0..2
    int hf = 100 - (hull < b->hull / 3 ? 25 : 0) - (temp > 100 ? 60 : 0);
    int tv = vm * thr / 100 * hf / 100; if (fuel <= 0 && b->kind != 0 && b->kind != 2) tv = 0;
    if (b->kind == 2) { int rel = ABS(isin((yaw - windd) & 4095)); tv = tv * 0 + vm * rel / 4096 * (60 + MAX(0, thr)) / 100; if (fuel > 0 && thr > 60) tv += vm / 5; }
    if (b->kind == 0 || b->kind == 1 && b->hull < 80) tv = tv * (100 - sea * 8) / 100;
    if (spd < tv) spd = MIN(tv, spd + b->acc); else spd = MAX(tv, spd - b->acc * 2);
    if (hd & PAD_LEFT) yaw = (yaw - (b->turn * (40 + MIN(spd, 160) / 2) / 100 + (b->kind == 4 || cur == 6 ? 8 : 0))) & 4095;
    if (hd & PAD_RIGHT) yaw = (yaw + (b->turn * (40 + MIN(spd, 160) / 2) / 100 + (b->kind == 4 || cur == 6 ? 8 : 0))) & 4095;
    roll = ((hd & PAD_RIGHT ? 1 : hd & PAD_LEFT ? -1 : 0) * spd * 2 + (isin((anim * 19) & 4095) >> 8) * (sea + 1)) / 3;
    int mx = isin(yaw) * spd >> 16, mz = icos(yaw) * spd >> 16, nx = px + mx, nz = pz + mz, g = gh(nx, nz);
    if (g > 0 && b->kind != 4) { int imp = ABS(spd) / 12; if (imp > 2) { hurt(imp); say("AGROUND!"); audio_voice(VOICE_NAH); } spd = -spd / 4; if (hull <= 0) { sunk = 1; } }
    else if (g > 0) { spd = spd * 15 / 16; px = nx; pz = nz; }
    else { px = nx; pz = nz; }
    if (b->kind != 4 && gh(px, pz) > 0) { px -= mx * 2; pz -= mz * 2; }
    { int bx, bz, bh, pop; int i = (px + TS / 2) / TS - (px < -TS / 2), j = (pz + TS / 2) / TS - (pz < -TS / 2); if (b->kind == 4 && bld(i, j, &bx, &bz, &bh, &pop) && !is_dead(i, j) && ABS(px - bx) < 60 && ABS(pz - bz) < 60) { hurt(30); spd = -spd / 2; say("YOU HIT A BUILDING!"); } }
    fuel -= (b->burn * ABS(thr) / 100 + (b->burn && thr > 0)) * (spd > 0 ? 1 : 0) / 6 > 0 && !(anim & 15) ? 1 : 0; if (fuel < 0) fuel = 0;
    if (b->kind != 0 && b->kind != 2) { if (thr > 85 && !(anim & 7)) temp = MIN(130, temp + 1); else if (!(anim & 15)) temp = MAX(0, temp - 1); if (temp >= 120) { hurt(1); } }
    if (hull <= 0 && !sunk) sunk = 1;
    // jobs
    if (jtype == 1 && jt > 0) { jt--; int dx = px - jx, dz = pz - jz; if (dx * dx + dz * dz < 380 * 380 && ABS(spd) < 70) { cash += jpay + jt / 30; pk_data[28]++; sims[player].skill_mech += 25; sprintf(line, "DELIVERED! +$%d", jpay + jt / 30); ctr = 240; jtype = 0; audio_voice(VOICE_WIN); } else if (!jt) { say("TOO LATE. THE JOB IS GONE."); jtype = 0; } }
    if (jtype == 2) { int dx = px - jx, dz = pz - jz; if (dx * dx + dz * dz < 600 * 600 && ABS(spd) < 40) { if (++fcast >= 150) { fcast = 0; if (fish < b->hold) { fish++; say("A BITE! YOU LAND A FISH."); audio_voice(VOICE_AGREE); } else say("THE HOLD IS FULL."); } } else fcast = 0; }
    if (jtype == 3 && jt > 0) { jt--; int dx = px - jx, dz = pz - jz; if (dx * dx + dz * dz < 300 * 300) { if (++jn >= 5) { cash += jpay + jt / 20; pk_data[28]++; sprintf(line, "RACE WON! +$%d", jpay + jt / 20); ctr = 240; jtype = 0; audio_voice(VOICE_WIN); } else { int a = (jn * 819 + 600) & 4095; jx = isin(a) * 2600 >> 12; jz = icos(a) * 2600 >> 12; while (gh(jx, jz) > -5) { jx = jx * 9 / 8; jz = jz * 9 / 8; } say("BUOY! KEEP GOING."); } } } else if (jtype == 3 && !jt) { say("RACE OVER. TOO SLOW."); jtype = 0; } }
static int near_dock(void) { for (int i = 0; i < 4; i++) { int dx = px - DK[i][0], dz = pz - DK[i][1]; if (dx * dx + dz * dz < 420 * 420) return i; } return -1; }
static void dock_act(void) {   // X at a harbour, nearly stopped
    int d = near_dock(); if (d < 0 || ABS(spd) > 60) return; const Boat *b = &BT[cur];
    if (fish) { int pay = fish * (30 + b->hold * 4); cash += pay; sprintf(line, "SOLD %d FISH: +$%d", fish, pay); fish = 0; ctr = 220; audio_voice(VOICE_WIN); return; }
    int cost = (b->fuel * (100 + tune(cur) * 30) / 100 - fuel) / 4 + (b->hull * (100 + tune(cur) * 25) / 100 - hull) * 2; if (cost > 0) { if (cash < cost) { say("NOT ENOUGH CASH TO REFUEL AND REPAIR."); return; } cash -= cost; fuel = b->fuel * (100 + tune(cur) * 30) / 100; hull = b->hull * (100 + tune(cur) * 25) / 100; temp = 0; sprintf(line, "FUELLED AND MENDED: -$%d", cost); ctr = 200; return; }
    if (jtype) { say("YOU ALREADY HAVE A JOB."); return; }
    if (b->kind == 0 || cur == 2 || cur == 5) { jtype = 2; jx = DK[d][0] + 900 * (d & 1 ? -1 : 1); jz = DK[d][1] + 300; while (gh(jx, jz) > -5) { jx += (jx > 0 ? 100 : -100); } say("FISH THE REEF. STOP IN THE RING."); return; }
    if (cur == 1 || cur == 3 || cur == 8) { jtype = 3; jn = 0; jt = 60 * 90; jpay = 400 + cur * 60; jx = isin(600) * 2600 >> 12; jz = icos(600) * 2600 >> 12; while (gh(jx, jz) > -5) { jx = jx * 9 / 8; jz = jz * 9 / 8; } say("BUOY RACE: FIVE BUOYS. GO!"); return; }
    int to = (d + 1 + rnd(3)) & 3; jtype = 1; jx = DK[to][0]; jz = DK[to][1]; int dx = jx - DK[d][0], dz = jz - DK[d][1]; int dist = isq(dx * dx + dz * dz); jt = dist * 16 / MAX(30, b->vmax) * 14 / 10 + 120; jpay = dist / 14 * (2 + b->hold / 4); say("A JOB: DELIVER IT. WATCH THE CLOCK.");
}
static void air_step(int hd) {
    const Air *a = &AT[cur - NB]; int tl = tune(cur); int vm = a->vmax * (100 + tl * 8) / 100 * 16, st = a->stall * 16; int alt = py - MAX(0, gh(px, pz));
    int bal = a->fl & 4, hel = a->fl & 2;
    if (a->fl & 4) {   // the balloon: burner for up, vent for down, drifts with the wind
        if (hd & PAD_CROSS) vy = MIN(40, vy + 2); else if (hd & PAD_SQUARE) vy = MAX(-40, vy - 2); else vy = vy * 15 / 16; py += vy / 8; if (py < gh(px, pz) + 4) { py = gh(px, pz) + 4; if (vy < -26) { hurt(40); say("A HARD LANDING."); } vy = 0; }
        px += isin(windd) >> 10; pz += icos(windd) >> 10; if (hd & PAD_UP) pz += 3; if (hd & PAD_DOWN) pz -= 3; if (hd & PAD_RIGHT) px += 3; if (hd & PAD_LEFT) px -= 3; if (!(anim & 63) && vy > 0) fuel = MAX(0, fuel - 1); if (!fuel && vy > 0) vy = 0; return; }
    if (hd & PAD_CROSS) thr = MIN(100, thr + 2); if (hd & PAD_SQUARE) thr = MAX(0, thr - 2);
    if (a->fl & 2) {   // the helicopter: thr is the lift
        int up = (thr - 45) * 7 / 100; vy = vy * 7 / 8 + up; spd = spd * 15 / 16 + ((hd & PAD_DOWN) ? 5 : (hd & PAD_UP) ? -3 : 0) * 2; pitch = (hd & PAD_DOWN) ? -240 : (hd & PAD_UP) ? 240 : pitch * 7 / 8;
        if (hd & PAD_LEFT) { yaw = (yaw - a->bank / 4) & 4095; } if (hd & PAD_RIGHT) { yaw = (yaw + a->bank / 4) & 4095; } roll = (hd & PAD_RIGHT ? 200 : hd & PAD_LEFT ? -200 : 0);
        if (fuel > 0 && !(anim & 31)) fuel--; if (!fuel) vy -= 2;
    } else {
        int sinp = isin(pitch & 4095), cosp = icos(pitch & 4095);
        if (hd & PAD_DOWN) pitch = MIN(1100, pitch + a->pitch); else if (hd & PAD_UP) pitch = MAX(-1100, pitch - a->pitch); else if (!hel) pitch = pitch * 31 / 32;   // (DOWN pulls the nose up)
        if (hd & PAD_LEFT) roll = MAX(-900, roll - a->bank); else if (hd & PAD_RIGHT) roll = MIN(900, roll + a->bank); else roll = roll * 15 / 16;
        int thrust = (cur == NB ? 0 : fuel > 0 ? thr * 8 / 100 : 0); spd += thrust - spd / 60 * (spd > vm ? 3 : 1) / 1 - sinp * 3 / 4096 * (1 + (cur == NB ? 0 : 0));
        if (cur != NB && spd < 0) spd = 0; if (!thrust && cur != NB) spd -= 1;
        if (spd < 0) spd = 0; if (spd > vm * 12 / 10) spd = vm * 12 / 10;
        int lift = spd > st ? 0 : (st - spd) * 18 / st;   // below the stall speed the wing lets go
        vy = (spd * sinp >> 12) / 16 - lift - (spd < st / 2 ? 4 : 1) * (alt > 6 || spd > st ? 0 : 0);
        if (spd < st) pitch = MAX(-700, pitch - 6);
        yaw = (yaw + (isin(roll & 4095) * spd >> 14) / 8) & 4095; if (!(anim & 31) && thr > 10 && fuel > 0) fuel--; if (fuel <= 0 && cur != NB) { thr = 0; }
        if (cur == NB && alt > 40) { vy -= 1; }   // (the glider sinks, and rises on the town's warm air)
        if (cur == NB) { int th = vn(px, pz, 700); if (th > 170 && alt > 20) vy += 4; }
    }
    int fv = spd * icos(pitch & 4095) >> 12; px += (isin(yaw) * fv >> 12) / 16; pz += (icos(yaw) * fv >> 12) / 16; py += vy / 4 * (hel ? 1 : 1) / (hel ? 3 : 1);
    int g = MAX(0, gh(px, pz)), over_sea = gh(px, pz) <= 0;
    { int i = (px + TS / 2) / TS - (px < -TS / 2), j = (pz + TS / 2) / TS - (pz < -TS / 2), bx, bz, bh, pop; if (bld(i, j, &bx, &bz, &bh, &pop) && !is_dead(i, j) && ABS(px - bx) < 70 && ABS(pz - bz) < 70 && py < g + bh) { crash_plane(px, pz, 0); return; } }
    if (py <= g + 4 && !crashed) {   // the ground
        int slope = ABS(gh(px + 60, pz) - gh(px - 60, pz)) + ABS(gh(px, pz + 60) - gh(px, pz - 60)), vs = -vy, strip = on_strip(px, pz), floats = (a->fl & 1) && over_sea, ok = (spd < st * 3 / 2 || hel) && vs < (hel ? 18 : 14) && ABS(roll) < 260 && ABS(pitch) < 360 && (strip || floats || ((hel || cur == NB || (vs < 9 && spd < st)) && slope < 70));
        if (ok) { py = g + 4; if (over_sea) py = 4; vy = 0; roll = 0; pitch = 0; spd = spd * 14 / 15; if (!landed) { landed = 1; int pay = 150 + (14 - MIN(14, vs)) * 25 + (strip ? 150 : 0); if (spd < 4 * 16) { } cash += pay; pk_data[28]++; sims[player].skill_mech += 25; sprintf(line, "SAFE LANDING! +$%d", pay); ctr = 240; audio_voice(VOICE_WIN); } }
        else { int dm = vs * 4 + spd / 12 + slope / 4 + ABS(roll) / 40;   // a crash landing: how bad it is decides
            if (dm < hull && dm < 90 && !over_sea) { hurt(dm); py = g + 4; vy = 0; spd = spd / 3; roll = pitch = 0; landed = 1; sprintf(line, "CRASH LANDING! HULL -%d", dm); ctr = 240; audio_voice(VOICE_NAH); }
            else crash_plane(px, pz, 0); }
    } else landed = 0;
    if (landed && spd < 16 && thr < 10 && cur != NB) { /* parked */ }
}
static int hd_r1;
static void chute_step(int hd) {
    int alt = cy_ - MAX(0, gh(cx, cz_));
    if (hd_r1 && cy_ - MAX(0, gh(cx, cz_)) > 40) { chute_on = !chute_on; }
    if (chute_on) { cvy = -5; if (hd & PAD_LEFT) cx -= 3; if (hd & PAD_RIGHT) cx += 3; if (hd & PAD_UP) cz_ += 3; if (hd & PAD_DOWN) cz_ -= 3; cx += isin(windd) >> 11; cz_ += icos(windd) >> 11; } else { cvy = MAX(cvy - 2, -60); if (hd & PAD_LEFT) cx -= 2; if (hd & PAD_RIGHT) cx += 2; if (hd & PAD_UP) cz_ += 2; if (hd & PAD_DOWN) cz_ -= 2; }
    cy_ += cvy / 3; cdrop++;
    // the empty plane flies on
    { const Air *a = &AT[cur - NB]; (void)a; ax += (isin(ayaw) * aspd >> 12) / 16 * icos(apitch & 4095) >> 12; az += (icos(ayaw) * aspd >> 12) / 16 * icos(apitch & 4095) >> 12; ay += (aspd * isin(apitch & 4095) >> 12) / 64 - 3; apitch = apitch - 2; }
    if (!crashed) { int g = MAX(0, gh(ax, az)); int i = (ax + TS / 2) / TS - (ax < -TS / 2), j = (az + TS / 2) / TS - (az < -TS / 2), bx, bz, bh, pop; if (ay <= g + 4 || (bld(i, j, &bx, &bz, &bh, &pop) && !is_dead(i, j) && ay < g + bh && ABS(ax - bx) < 70 && ABS(az - bz) < 70)) { crashed = 1; if (real()) { blow(ax, az, 520); } else strcpy(line2, "THE EMPTY PLANE CRASHED. NOBODY HURT."); } }
    if (cy_ <= MAX(0, gh(cx, cz_)) + 4) {
        int sea = gh(cx, cz_) <= 0; mode = M_END; sunk = 0;
        if (!chute_on) { strcpy(line, sea ? "SPLASH! YOU SWIM TO SHORE." : "NO CHUTE."); if (sea || !(pk_data[39] & 2)) { strcpy(line2, "FALL DAMAGE IS OFF OR THE WATER CAUGHT YOU."); } else if (real()) { cash = MAX(0, cash - 2000); clock_min = MIN(1439, clock_min + 240); sims[player].mood = MAX(-100, sims[player].mood - 50); strcpy(line2, "YOU SURVIVED, JUST. HOSPITAL: -$2000."); } else { cash = MAX(0, cash - 300); strcpy(line2, "ARCADE MODE: A BROKEN LEG. -$300."); } crashed = 1; audio_voice(VOICE_CRY); }
        else { strcpy(line, sea ? "SPLASH! A BOAT FISHES YOU OUT." : "A SOFT LANDING. THE PARACHUTE WORKED."); if (!crashed) strcpy(line2, "THE PLANE LANDED ITSELF SOMEHOW."); if (crashed) { sprintf(line2, "THE PLANE CRASHED. %s", pk_data[37] ? "THE TOWN IS NOT HAPPY." : "NO ONE ELSE WAS HURT."); sim_crime(CR_ASSAULT, 10); } cash = MAX(0, cash - 400); audio_voice(VOICE_WIN); }
        clock_min = MIN(1439, clock_min + 60); crashed = 0; return; }
    (void)alt;
}
// ----- menus -----
static int nrows(void) { return NCR + 3; }
void skysea_open(int a) { (void)a; mode = M_MENU; tab = 0; sel = 0; anim = 0; line[0] = line2[0] = 0; rs = (unsigned)(cash * 5 + day_count * 13 + 3); windd = (day_count * 531) & 4095; ctr = 0;
    if (!owned() && !last()) { pk_data[26] = (u8)day_count; pk_data[27] = (u8)(day_count >> 8); } }
static void act(void) {
    line[0] = 0;
    if (sel == NCR) { int d = MIN(30, MAX(0, day_count - last())); if (!owned()) { strcpy(line, "BUY A CRAFT FIRST."); return; } if (!d) { strcpy(line, "NOTHING NEW TODAY."); return; } int pay = rate() * d; settle(); sprintf(line, "%d DAYS OF GUESTS: +$%d", d, pay); audio_voice(VOICE_WIN); return; }
    if (sel == NCR + 2) { pk_data[39] ^= 2; strcpy(line, pk_data[39] & 2 ? "FALL DAMAGE: ON" : "FALL DAMAGE: OFF"); return; }
    if (sel == NCR + 1) { pk_data[39] ^= 1; strcpy(line, real() ? "CRASH MODE: REAL. THE TOWN PAYS." : "CRASH MODE: ARCADE. BOUNCE BACK."); return; }
    if (has(sel)) { if (sel < NB) spawn_boat(sel); else spawn_air(sel); return; }
    if (cash < price(sel)) { strcpy(line, "NOT ENOUGH CASH."); audio_voice(VOICE_NAH); return; }
    settle(); cash -= price(sel); pk_data[24 + (sel >> 3)] |= (u8)(1 << (sel & 7)); sprintf(line, "BOUGHT: %s. X TO TAKE IT OUT.", CNM[sel]); audio_voice(VOICE_FINISHED);
}
int skysea_update(int hd, int pr, int steps) {
    anim += steps;
    if (mode == M_MENU) {
        if (pr & PAD_CIRCLE) return 0;
        int n = nrows(); if (pr & PAD_UP) sel = (sel + n - 1) % n; if (pr & PAD_DOWN) sel = (sel + 1) % n;
        if (pr & PAD_CROSS) act();
        if ((pr & PAD_SQUARE) && sel < NCR && (owned() >> sel & 1)) { int t = tune(sel), c = price(sel) / 4 * (t + 1); if (t >= 3) strcpy(line, "FULLY TUNED."); else if (cash < c) strcpy(line, "NOT ENOUGH CASH TO TUNE."); else { cash -= c; set_tune(sel, t + 1); sprintf(line, "TUNED UP: LEVEL %d", t + 1); audio_voice(VOICE_FINISHED); } }
        return 1; }
    if (mode == M_END) { if (pr & (PAD_CROSS | PAD_CIRCLE)) { police_check(); mode = M_MENU; } return 1; }
    hd_r1 = pr & PAD_R1;
    for (int s = 0; s < steps && s < 4; s++) {
        anim++;
        if (mode == M_BOAT) { boat_step(hd); if (sunk) { cash = MAX(0, cash - 600); end_trip("THE BOAT SANK!", "A TUG TOWS YOU HOME. -$600."); return 1; } }
        else if (mode == M_AIR) { air_step(hd); if (mode != M_AIR) return 1; }
        else if (mode == M_CHUTE) { chute_step(hd); if (mode != M_CHUTE) return 1; }
    }
    if (ctr > 0) ctr -= steps;
    if (mode == M_BOAT) { if (pr & PAD_TRIANGLE) dock_act(); if (pr & PAD_CIRCLE) { if (near_dock() >= 0 && ABS(spd) < 60) { hot ? police_check() : (void)0; mode = M_MENU; } else say("PARK AT A HARBOUR TO GET OFF."); } }
    if (mode == M_AIR) { if (pr & PAD_TRIANGLE) bail(0); if (pr & PAD_R1) bail(1); if (pr & PAD_CIRCLE) { if (landed && spd < 30) { police_check(); mode = M_MENU; } else say("LAND FIRST. OR PRESS TRIANGLE TO BAIL OUT."); } }
    return 1;
}
// ----- the picture -----
static u32 shade(u32 c, int k) { int r = MIN(255, (c & 255) * k >> 8), g = MIN(255, ((c >> 8) & 255) * k >> 8), b = MIN(255, ((c >> 16) & 255) * k >> 8); return UI_RGB(r, g, b); }
static void box(int x0, int y0, int z0, int x1, int y1, int z1, u32 c) {
    SVECTOR t0 = { (short)x0, (short)y0, (short)z0 }, t1 = { (short)x1, (short)y0, (short)z0 }, t2 = { (short)x0, (short)y0, (short)z1 }, t3 = { (short)x1, (short)y0, (short)z1 };
    SVECTOR b0 = { (short)x0, (short)y1, (short)z0 }, b1 = { (short)x1, (short)y1, (short)z0 }, b2 = { (short)x0, (short)y1, (short)z1 }, b3 = { (short)x1, (short)y1, (short)z1 };
    u32 top = shade(c, 300), s1 = shade(c, 220), s2 = shade(c, 170);
    gfx_quad_g(&t2, &t3, &t0, &t1, top, top, top, top, 0); gfx_quad_g(&t0, &t1, &b0, &b1, s1, s1, s1, s1, 0); gfx_quad_g(&t3, &t2, &b3, &b2, s1, s1, s1, s1, 0);
    gfx_quad_g(&t2, &t0, &b2, &b0, s2, s2, s2, s2, 0); gfx_quad_g(&t1, &t3, &b1, &b3, s2, s2, s2, s2, 0);
}
static u32 gcol(int h, int x, int z, int wave) {   // the ground's colour by height
    if (h <= 0) { int w = 150 + (isin((anim * 9 + x + z * 3) & 4095) >> 8) + wave; return UI_RGB(30, 90 + w / 4, 160 + w / 5); }
    if (h < 14) return UI_RGB(226, 206, 140); if (h < 220) return UI_RGB(70 + h / 4, 150 - h / 8, 66); if (h < 700) return UI_RGB(120, 112, 100); return UI_RGB(236, 240, 246);
}
static void world_draw(int fx, int fy, int fz, int fyaw) {
    int gx0 = fx / CELL - RAD, gz0 = fz / CELL - RAD; if (fx < 0) gx0--; if (fz < 0) gz0--;
    for (int j = 0; j < 2 * RAD + 1; j++) for (int i = 0; i < 2 * RAD + 1; i++) {
        int x0 = (gx0 + i) * CELL, z0 = (gz0 + j) * CELL, h00 = gh(x0, z0), h10 = gh(x0 + CELL, z0), h01 = gh(x0, z0 + CELL), h11 = gh(x0 + CELL, z0 + CELL);
        int y00 = -MAX(0, h00), y10 = -MAX(0, h10), y01 = -MAX(0, h01), y11 = -MAX(0, h11);
        SVECTOR a = { (short)x0, (short)y00, (short)z0 }, b = { (short)(x0 + CELL), (short)y10, (short)z0 }, c = { (short)x0, (short)y01, (short)(z0 + CELL) }, d = { (short)(x0 + CELL), (short)y11, (short)(z0 + CELL) };
        int strip = on_strip(x0 + CELL / 2, z0 + CELL / 2);
        u32 ca = strip ? UI_RGB(80, 80, 88) : gcol(h00, x0, z0, 0), cb = strip ? UI_RGB(90, 90, 98) : gcol(h10, x0 + CELL, z0, 4), cc = strip ? UI_RGB(80, 80, 88) : gcol(h01, x0, z0 + CELL, 8), cd = strip ? UI_RGB(90, 90, 98) : gcol(h11, x0 + CELL, z0 + CELL, 12);
        gfx_quad_g(&a, &b, &c, &d, ca, cb, cc, cd, 0);
    }
    int i0 = fx / TS - 4, j0 = fz / TS - 4;   // the town's buildings in range
    for (int j = j0; j <= j0 + 8; j++) for (int i = i0; i <= i0 + 8; i++) { int bx, bz, bh, pop; if (!bld(i, j, &bx, &bz, &bh, &pop)) continue; int g = MAX(0, gh(bx, bz));
        if (is_dead(i, j)) { box(bx - 50, -g - 14, bz - 50, bx + 50, -g, bz + 50, UI_RGB(60, 50, 46)); continue; }
        box(bx - 55, -g - bh, bz - 55, bx + 55, -g, bz + 55, UI_RGB(150 + (i * 37 & 63), 140 + (j * 53 & 63), 130 + ((i + j) * 29 & 63))); }
    (void)fy; (void)fyaw;
    for (int k = 0; k < 4; k++) { int dx = DK[k][0] - fx, dz = DK[k][1] - fz; if (ABS(dx) < 2800 && ABS(dz) < 2800) { box(DK[k][0] - 40, -30, DK[k][1] - 160, DK[k][0] + 40, -6, DK[k][1] + 160, UI_RGB(150, 110, 70)); box(DK[k][0] - 6, -120, DK[k][1] - 6, DK[k][0] + 6, -30, DK[k][1] + 6, UI_RGB(240, 200, 60)); } }
    if (jtype && mode == M_BOAT) box(jx - 40, -150, jz - 40, jx + 40, -10, jz + 40, jtype == 2 ? UI_RGB(255, 220, 80) : UI_RGB(255, 80, 60));   // the job's marker
}
static void craft_draw(int c, int x, int y, int z, int yw, int pt, int rl, int on_fire) {
    MATRIX m; SVECTOR r = { (short)pt, (short)yw, (short)rl }; RotMatrix(&r, &m); m.t[0] = x; m.t[1] = -y; m.t[2] = z; gfx_set_object_m(&m);
    if (c < NB) {   // a hull, a cabin and a bow
        u32 hc = c == 9 ? UI_RGB(250, 250, 255) : c == 5 ? UI_RGB(70, 110, 170) : c == 3 ? UI_RGB(230, 60, 50) : UI_RGB(220, 200, 150);
        int L = 26 + c * 8, BW = 12 + c * 2; box(-BW, -12, -L, BW, 6, L, hc); box(-BW * 2 / 3, -28, -L / 3, BW * 2 / 3, -12, L / 3, UI_RGB(240, 240, 245));
        if (c == 4) box(-1, -90, -2, 1, -12, 2, UI_RGB(120, 80, 50)), box(-2, -86, 2, 2, -30, 40, UI_RGB(250, 250, 250));
        if (c == 8) box(-BW - 8, -6, -L - 6, BW + 8, 0, L + 6, UI_RGB(60, 60, 70));
    } else {
        int a = c - NB; u32 fc = a == 5 ? UI_RGB(160, 170, 190) : a == 1 ? UI_RGB(220, 60, 50) : a == 0 ? UI_RGB(240, 240, 250) : a == 4 ? UI_RGB(230, 180, 50) : UI_RGB(240, 220, 90);
        if (a == 4) { box(-20, -10, -20, 20, 0, 20, UI_RGB(150, 100, 60)); box(-44, -120, -44, 44, -34, 44, fc); return; }
        box(-8, -10, -34, 8, 6, 34, fc); box(-52, -3, -8, 52, 1, 8, a == 5 ? UI_RGB(120, 130, 150) : UI_RGB(240, 240, 240)); box(-1, -22, 24, 1, -4, 36, UI_RGB(200, 60, 50)); box(-16, -3, 26, 16, 1, 34, UI_RGB(240, 240, 240));
        if (a == 3) box(-50, -18, -1, 50, -16, 1, UI_RGB(60, 60, 60));   // the rotor
    }
    (void)on_fire;
}
static void hud_bar(int x, int y, int w, int v, int vmax, u32 c) { ui_round(x, y, w, 7, 3, UI_RGB(40, 44, 60), UI_RGB(24, 26, 40), 0); if (v > 0) ui_round(x, y, MAX(4, w * MIN(v, vmax) / MAX(1, vmax)), 7, 3, c, c, 0); }
void skysea_draw(void) {
    char b[64];
    if (mode == M_BOAT || mode == M_AIR || mode == M_CHUTE) {
        gfx_set_clear(150, 200, 240); gfx_sky(0xf0b878u, 0xf8e8d0u); gfx_set_ambient(120, 100);
        int fx = mode == M_CHUTE ? cx : px, fy = mode == M_CHUTE ? cy_ : py, fz = mode == M_CHUTE ? cz_ : pz, dist = mode == M_BOAT ? 360 : 520;
        int vy_ = mode == M_BOAT ? 150 : 90, fy2 = mode == M_CHUTE ? 100 : 0;
        cam.at.vx = fx; cam.at.vy = -fy - 20; cam.at.vz = fz;
        cam.eye.vx = fx - ((isin(yaw) * dist) >> 12); cam.eye.vz = fz - ((icos(yaw) * dist) >> 12); cam.eye.vy = -fy - vy_ - fy2;
        if (mode == M_CHUTE) { cam.eye.vx = fx - 300; cam.eye.vz = fz - 300; }
        gfx_camera_update(); gfx_set_fog(1800, 3200); gfx_set_identity();
        world_draw(fx, fy, fz, yaw);
        if (mode == M_BOAT) craft_draw(cur, px, py, pz, yaw, 0, roll, 0);
        else if (mode == M_AIR) craft_draw(cur, px, py, pz, yaw, pitch, roll, 0);
        else { if (!crashed) craft_draw(cur, ax, ay, az, ayaw, apitch, aroll, 0); gfx_set_identity(); box(cx - 14, -cy_ - 6, cz_ - 14, cx + 14, -cy_ + 20, cz_ + 14, UI_RGB(240, 200, 60)); if (chute_on) box(cx - 46, -cy_ - 80, cz_ - 46, cx + 46, -cy_ - 56, cz_ + 46, UI_RGB(230, 60, 80)); }
        gfx_set_fog(0, 0);
        // the HUD
        ui_round(4, 4, 150, 44, 6, UI_RGB(14, 18, 34), UI_RGB(30, 36, 60), 0);
        if (mode == M_BOAT) { sprintf(b, "%s  %d KNOTS", CNM[cur], ABS(spd) / 5); ui_text(10, 8, 1, b, UI_WHITE); hud_bar(10, 20, 60, fuel, BT[cur].fuel * 2, UI_RGB(240, 200, 60)); hud_bar(10, 31, 60, hull, BT[cur].hull * 2, UI_RGB(80, 200, 100)); ui_text(76, 19, 1, "FUEL", UI_SOFT); ui_text(76, 30, 1, "HULL", UI_SOFT); hud_bar(108, 20, 40, temp, 130, temp > 100 ? UI_RGB(240, 70, 50) : UI_RGB(240, 150, 60)); ui_text(114, 30, 1, "HEAT", UI_SOFT); hud_bar(10, 41, 138, thr + 30, 130, UI_RGB(80, 160, 240)); }
        else if (mode == M_AIR) { sprintf(b, "%s  %d  ALT %d", CNM[cur], spd / 5, MAX(0, py - MAX(0, gh(px, pz)))); ui_text(10, 8, 1, b, UI_WHITE); hud_bar(10, 20, 60, fuel, AT[cur - NB].fuel * 2 + 1, UI_RGB(240, 200, 60)); hud_bar(10, 31, 60, hull, AT[cur - NB].hull * 2, UI_RGB(80, 200, 100)); ui_text(76, 19, 1, "FUEL", UI_SOFT); ui_text(76, 30, 1, "HULL", UI_SOFT); hud_bar(10, 41, 138, thr, 100, UI_RGB(80, 160, 240)); if (spd < AT[cur - NB].stall * 16 && AT[cur - NB].stall) ui_text(100, 8, 1, "STALL!", UI_BADT); }
        else { ui_text(10, 8, 1, chute_on ? "PARACHUTE" : "FALLING!", UI_WHITE); sprintf(b, "ALT %d", MAX(0, cy_ - MAX(0, gh(cx, cz_)))); ui_text(10, 20, 1, b, UI_WHITE); }
        sprintf(b, "$%d", (int)cash); ui_text(316 - ui_tw(1, b), 6, 1, b, UI_GOODT);
        if (hot) ui_text(316 - ui_tw(1, "HOT CRAFT"), 18, 1, "HOT CRAFT", UI_BADT);
        if (mode == M_BOAT && jtype) { int dx = jx - px, dz = jz - pz; sprintf(b, jtype == 2 ? "FISH %d/%d  %s" : "JOB  %dM  %ds", jtype == 2 ? fish : isq(dx * dx + dz * dz) / 10, jtype == 2 ? BT[cur].hold : jt / 60, jtype == 2 ? (fcast ? "..." : "STOP IN THE RING") : ""); ui_text(10, 52, 1, b, UI_WHITE); }
        if (mode == M_BOAT && near_dock() >= 0 && ABS(spd) < 60) ui_text(160, 200, 1, "TRIANGLE: HARBOUR  O: GET OFF", UI_WHITE);
        if (ctr > 0) ui_textc(160, 180, 1, line, UI_WHITE);
        ui_hints(160, 226, mode == M_BOAT ? "X GAS|SQ BRAKE|PAD STEER" : mode == M_AIR ? "X THRUST|SQ LESS|TRI CHUTE|R1 JUMP" : "PAD STEER", 1);
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
        if (i == NCR + 2) { ui_text(20, y + 1, 1, pk_data[39] & 2 ? "FALL DAMAGE: ON" : "FALL DAMAGE: OFF", on ? UI_BLUE2 : UI_INK); continue; }
        if (i == NCR + 1) { ui_text(20, y + 1, 1, real() ? "CRASH MODE: REAL (THE TOWN PAYS)" : "CRASH MODE: ARCADE (BOUNCE BACK)", on ? UI_BLUE2 : UI_INK); continue; }
        int h = has(i); sprintf(b, "%s%s", CNM[i], stol(i) ? " (HOT)" : ""); ui_text(20, y + 1, 1, b, h ? (on ? UI_BLUE2 : UI_GOODT) : on ? UI_BLUE2 : UI_INK);
        if (h) { sprintf(b, "DRIVE T%d", tune(i)); ui_text(300 - ui_tw(1, b), y + 1, 1, b, UI_GOODT); } else { sprintf(b, "$%d", price(i)); ui_text(300 - ui_tw(1, b), y + 1, 1, b, cash >= price(i) ? UI_INK : UI_BADT); }
    }
    int by = top + 124; ui_round(14, by, 292, 62, 5, UI_PANEL, UI_LINE, 0);
    if (sel < NCR) { sprintf(b, "%s  %s", CNM[sel], sel < NB ? "BOAT" : "AIRCRAFT"); ui_text(20, by + 4, 1, b, UI_BLUE2); ui_wrap(20, by + 16, 280, 2, CHINT[sel], UI_INK); sprintf(b, "EARNS $%d A DAY  SQ TUNES UP", inc(sel)); ui_text(20, by + 40, 1, b, UI_SOFT); }
    else { sprintf(b, "%d CRAFT  $%d A DAY", __builtin_popcount(owned()), rate()); ui_text(20, by + 4, 1, b, UI_BLUE2); sprintf(b, "JOBS AND LANDINGS %d  BUILDINGS DOWN %d  HURT %d", pk_data[28], pk_data[37], pk_data[38]); ui_text(20, by + 18, 1, b, UI_INK); }
    ui_text(20, by + 50, 1, line, UI_RED);
    ui_hints(160, 218, "X PICK|O LEAVE", 1);
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, skysea_open, skysea_update, skysea_draw };
