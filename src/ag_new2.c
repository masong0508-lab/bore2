// ag_new2.c - five more cabinets: HOOPS (basketball), SKEE-BORE (skee-ball), AIR HOCKEY (a duel, two pads), BEAT BORE (rhythm) and CLAW GRAB.
#include "arc_i.h"

static int isq2(int v) { int r = 0; while ((r + 1) * (r + 1) <= v) r++; return r; }

// ================= HOOPS =================
static int hxp, hhx, hmet, hdr, hst, htm, hbx, hby, hbx0, hty, hmk, hstreak, hshots, hres, hpow, hfire, hz;
void hoo_start(void) { hxp = 120; hhx = 120; hmet = 0; hdr = 1; hst = 0; htm = 0; hstreak = 0; hshots = 0; hres = 0; hfire = 0; ag_level = 1; }
static int hideal(void) { return 22 + ABS(hxp - hhx) * 60 / 100; }
static int hhalf(void) { return MAX(5, 14 - ag_level * 2 - ABS(hxp - hhx) / 20); }
int hoo_tick(int hd, int pr, int h2, int p2) {
    int t = ag_t; ag_level = 1 + t / 360;
    if (t > 240) { int a = MIN(60, (t - 240) / 12), ph = (t * 3) % 240; hhx = 120 + (ph < 120 ? ph - 60 : 180 - ph) * a / 60; }
    if (hst == 0) {
        if (hd & PAD_LEFT) hxp -= 3; if (hd & PAD_RIGHT) hxp += 3; hxp = CLAMP(hxp, 24, 216);
        hmet += hdr * (3 + ag_level); if (hmet >= 100) { hmet = 100; hdr = -1; } if (hmet <= 0) { hmet = 0; hdr = 1; }
        if (pr & PAD_CROSS) {
            int err = ABS(hmet - hideal()), half = hhalf(); hpow = hmet; hst = 1; htm = 0; hbx0 = hxp; hshots++;
            if (err <= MAX(2, half / 3)) hres = 3; else if (err <= half) hres = 2; else if (err <= half * 2 && (ar8() & 1)) hres = 1; else hres = 0;
            hty = 44; hz = hres >= 2 ? 0 : (hmet > hideal() ? 22 : -22); if (hres == 1) hz = 0;
        }
    } else if (hst == 1) {
        htm++; int T = 24; hbx = hbx0 + (hhx + (hres >= 1 ? 0 : hz) - hbx0) * htm / T; hby = 124 + (hty - 124) * htm / T - (htm * (T - htm)) / 3;
        if (htm >= T) {
            hst = 2; htm = 0;
            if (hres >= 2) {
                int far = ABS(hbx0 - hhx) > 70, v = far ? 3 : 2; hstreak++; ag_combo = hstreak; ag_best = MAX(ag_best, hstreak); if (hres == 3) v++; if (hstreak >= 3) { v *= 2; hfire = 1; }
                ag_pts += v; char s[12]; snprintf(s, sizeof s, hres == 3 ? "SWISH +%d" : "+%d", v); pop(hhx, 60, s, hfire ? UI_AMBER : UI_WHITE); cue(hres == 3 ? CUE_WIN : CUE_GOOD); burst(hhx, 48, hres == 3 ? 10 : 4, UI_RGB(255, 210, 80));
                if (hstreak == 3) { pop(hhx, 78, "ON FIRE!", UI_RED); cue(CUE_GREAT); }
            } else { hstreak = 0; ag_combo = 0; hfire = 0; ag_perf = 0; pop(hhx, 60, hres == 1 ? "RIM OUT" : "MISS", UI_RED); cue(CUE_BAD); }
        }
    } else { if (++htm > 14) { hst = 0; hmet = 0; hdr = 1; } }
    return t < 1350;
}
void hoo_draw(void) {
    R(0, 14, 240, 146, 46, 30, 66); R(0, 112, 240, 48, 190, 130, 70); for (int i = 0; i < 12; i++) R(i * 20, 112, 1, 48, 160, 105, 55); R(0, 112, 240, 2, 250, 220, 160);
    R(8, 150, 224, 2, 250, 250, 250);
    NUM(4, 2, "POINTS", ag_pts, UI_WHITE); { char s[16]; snprintf(s, sizeof s, "TIME %d", MAX(0, 1350 - ag_t) / 30); TXC(120, 2, s, UI_RGB(170, 210, 250)); } if (hstreak >= 2) { char s[16]; snprintf(s, sizeof s, "STREAK %d", hstreak); TX(176, 2, s, UI_AMBER); }
    R(hhx - 20, 24, 40, 28, 230, 230, 240); R(hhx - 18, 26, 36, 24, 190, 195, 220); R(hhx - 8, 38, 16, 11, 230, 60, 60); R(hhx - 6, 40, 12, 7, 190, 195, 220); R(hhx - 14, 52, 28, 3, 250, 120, 40);
    for (int i = 0; i < 5; i++) R(hhx - 12 + i * 6, 55, 1, 10, 235, 235, 235); R(hhx - 12, 65, 24, 1, 235, 235, 235);
    { int x = hxp, y = 150; R(x - 4, y - 14, 3, 14, 70, 70, 130); R(x + 1, y - 14, 3, 14, 70, 70, 130); R(x - 6, y - 28, 12, 14, hfire ? 255 : 230, hfire ? 120 : 70, hfire ? 40 : 70); R(x - 5, y - 38, 10, 10, 230, 175, 130); R(x - 5, y - 40, 10, 4, 60, 40, 30);
      if (hst == 0) R(x - 3, y - 36, 2, 2, 10, 10, 10); R(x - 8, y - 26 - (hst == 0 ? 6 : 12), 3, 8, 230, 175, 130); R(x + 5, y - 26 - (hst == 0 ? 6 : 12), 3, 8, 230, 175, 130); }
    if (hst == 0) DSC(hxp, 114, 5, UI_RGB(240, 120, 40)); else if (hst == 1) { DSC(hbx, hby, 5, UI_RGB(240, 120, 40)); R(hbx - 4, hby, 8, 1, 120, 50, 10); if (hfire) { DSC(hbx - 3, hby + 3, 4, UI_RGB(255, 200, 50)); } } else if (hres >= 2) DSC(hhx, 62 + htm * 3, 5, UI_RGB(240, 120, 40)); else DSC(hhx + (hz ? hz : (htm * 3)), 56 + htm * 4, 5, UI_RGB(240, 120, 40));
    // the power meter, with the zone to hit
    R(222, 22, 12, 100, 20, 20, 44); { int id = hideal(), hf = hhalf(); R(222, 120 - id - hf, 12, hf * 2, 70, 200, 90); R(222, 120 - id - MAX(2, hf / 3), 12, MAX(2, hf / 3) * 2, 255, 230, 70); R(220, 120 - hmet, 16, 3, 255, 255, 255); }
    TX(204, 126, "POWER", UI_RGB(170, 210, 250)); if (hst == 0) TXC(110, 100, "X SHOOTS. AIM THE GREEN", UI_RGB(170, 210, 250));
}

// ================= SKEE-BORE =================
static struct { s16 x, y; u8 r; s8 v; u8 kind; } SH[11];
static const s16 SHT[11][4] = { { 120, 26, 8, 50 }, { 80, 38, 8, 40 }, { 160, 38, 8, 40 }, { 38, 24, 7, 100 }, { 202, 24, 7, 100 }, { 100, 58, 9, 30 }, { 140, 58, 9, 30 }, { 120, 74, 10, 20 }, { 62, 84, 11, 10 }, { 178, 84, 11, 10 }, { 120, 94, 11, 10 } };
static int kx, kpow, kdir, kst, ktm, kbx, kby, kbx0, klx, kly, kballs, kpts, khot, kres;
void ski_start(void) { kx = 120; kpow = 0; kdir = 1; kst = 0; ktm = 0; kballs = 9; khot = 0; kres = 0; }
int ski_tick(int hd, int pr, int h2, int p2) {
    if (kst == 0) {
        if (hd & PAD_LEFT) kx -= 3; if (hd & PAD_RIGHT) kx += 3; kx = CLAMP(kx, 20, 220);
        if (hd & PAD_CROSS) { kpow += kdir * 4; if (kpow >= 100) { kpow = 100; kdir = -1; } if (kpow <= 0) { kpow = 0; kdir = 1; } }
        else if (kpow > 0) {   // let go: it rolls
            kst = 1; ktm = 0; kbx0 = kx; kballs--; int n = ar8() % 9 - 4; kly = 106 - kpow * 78 / 100 + n / 2; klx = kx + (ar8() % 7 - 3) + (kpow > 90 ? ar8() % 11 - 5 : 0); kpow = 0; kdir = 1; }
    } else if (kst == 1) {
        ktm++; int T = 22; kbx = kbx0 + (klx - kbx0) * ktm / T; kby = 150 + (kly - 150) * ktm / T - (ktm > 8 ? (ktm - 8) * (T - ktm) / 3 : 0);
        if (ktm >= T) {
            int best = -1, bd = 999; for (int i = 0; i < 11; i++) { int dx = klx - SHT[i][0], dy = kly - SHT[i][1], d = isq2(dx * dx + dy * dy) - SHT[i][2]; if (d < bd) { bd = d; best = i; } }
            int v = 0; if (best >= 0) { int p = SHT[best][3]; if (bd <= 2) v = p; else if (bd <= 9) v = p / 2; }
            kres = v; kst = 2; ktm = 0; ag_pts += v;
            if (v >= 30) { ag_combo++; ag_best = MAX(ag_best, ag_combo); } else { ag_combo = 0; ag_perf = 0; }
            if (v >= 40) { khot++; if (khot == 3) { ag_pts += 50; pop(120, 100, "HOT STREAK +50", UI_AMBER); cue(CUE_WIN); khot = 0; } } else khot = 0;
            char s[14]; snprintf(s, sizeof s, v ? "+%d" : "MISS", v); pop(CLAMP(klx, 20, 220), MAX(18, kly - 10), s, v >= 40 ? UI_AMBER : v ? UI_WHITE : UI_RED); cue(v >= 100 ? CUE_WIN : v >= 30 ? CUE_GREAT : v ? CUE_GOOD : CUE_BAD); if (v) burst(klx, kly, v >= 40 ? 10 : 4, UI_RGB(120, 240, 200));
        }
    } else { if (++ktm > 20) { if (kballs <= 0) return 0; kst = 0; } }
    return 1;
}
void ski_draw(void) {
    R(0, 14, 240, 146, 20, 70, 70); R(0, 14, 240, 100, 12, 50, 60);
    for (int i = 0; i < 11; i++) { int x = SHT[i][0], y = SHT[i][1], r = SHT[i][2]; DSC(x, y, r, UI_RGB(14, 20, 30)); DSC(x, y, r - 2, UI_RGB(i == 0 ? 250 : i < 3 ? 250 : i < 5 ? 250 : 90, i == 0 ? 200 : i < 3 ? 120 : i < 5 ? 60 : 200, i < 5 ? 60 : 160)); char s[5]; snprintf(s, sizeof s, "%d", SHT[i][3]); TXC(x, y - 3, s, UI_RGB(10, 10, 30)); }
    R(0, 106, 240, 10, 150, 100 + 20, 60); R(0, 106, 240, 2, 250, 200, 120); for (int i = 0; i < 6; i++) R(i * 42, 118, 30, 42, 160 - (i & 1) * 18, 105, 55);
    NUM(4, 2, "SCORE", ag_pts, UI_WHITE); { char s[16]; snprintf(s, sizeof s, "BALLS %d", kballs); TXC(120, 2, s, UI_AMBER); }
    R(kx - 6, 120, 12, 5, 250, 220, 60); if (kst == 0) { DSC(kx, 135, 5, UI_RGB(240, 130, 50)); if (kpow > 0 || (ag_t & 8)) { R(kx - 1, 114 - kpow / 2, 2, 12 + kpow / 2, 255, 255, 255); } }
    if (kst == 1) { DSC(kbx, kby, 5 - ktm / 8, UI_RGB(240, 130, 50)); } if (kst == 2 && kres == 0) DSC(klx + ktm / 2, kly + ktm * 3, 4, UI_RGB(240, 130, 50));
    R(10, 150, 60, 6, 20, 20, 44); R(10, 150, kpow * 60 / 100, 6, kpow > 85 ? 255 : 80, kpow > 85 ? 200 : 220, 80); TX(76, 149, "POWER", UI_RGB(170, 210, 250));
    if (kst == 0) TXC(150, 150, "HOLD X, LET GO", UI_RGB(170, 210, 250));
}

// ================= AIR HOCKEY =================
static int mx_[2], my_[2], mvx_[2], mvy_[2], qx, qy, qvx, qvy, hs_[2], hfz, hlv, cdx, cdy, cdt;
static void hserve(int to) { qx = 120 * 16; qy = 87 * 16; qvx = to ? 40 : -40; qvy = (ar8() - 128) / 8; }
void hoc_start(void) { mx_[0] = 28; my_[0] = 87; mx_[1] = 212; my_[1] = 87; hs_[0] = hs_[1] = 0; hfz = 0; hlv = 1; cdx = 212; cdy = 87; cdt = 0; hserve(ar8() & 1); }
static void hmove(int s, int h) {
    int ox = mx_[s], oy = my_[s], sp = 5; if (h & PAD_LEFT) mx_[s] -= sp; if (h & PAD_RIGHT) mx_[s] += sp; if (h & PAD_UP) my_[s] -= sp; if (h & PAD_DOWN) my_[s] += sp;
    mx_[s] = CLAMP(mx_[s], s ? 128 : 14, s ? 226 : 112); my_[s] = CLAMP(my_[s], 28, 148); mvx_[s] = mx_[s] - ox; mvy_[s] = my_[s] - oy;
}
int hoc_tick(int hd, int pr, int h2, int p2) {
    ag_level = hlv = CLAMP(1 + hs_[0] / 2 - (hs_[1] > hs_[0]), 1, 4);
    if (hfz > 0) { if (--hfz == 0) hserve(hs_[0] + hs_[1] & 1); return 1; }
    hmove(0, hd);
    if (ag_np == 2) hmove(1, h2);
    else {   // the CPU
        int ox = mx_[1], oy = my_[1], sp = 2 + hlv; if (--cdt <= 0) { cdt = 24; cdy = (ar8() % 31 - 15) * (5 - hlv) / 3; }
        int px = qx / 16, py = qy / 16, gx = 214, gy = 87;
        if (px > 120 && qvx > -20) { gx = px + 14 < 226 ? px + 12 : 226; gy = py + cdy / 2; if (px - mx_[1] > -4 && ABS(py - my_[1]) < 10) gx = px - 14; }
        else if (px > 120) { gx = 200; gy = py + cdy; } else { gx = 212; gy = 87 + (py - 87) / 3; }
        mx_[1] += CLAMP(gx - mx_[1], -sp, sp); my_[1] += CLAMP(gy - my_[1], -sp, sp); mx_[1] = CLAMP(mx_[1], 128, 226); my_[1] = CLAMP(my_[1], 28, 148); mvx_[1] = mx_[1] - ox; mvy_[1] = my_[1] - oy;
    }
    qx += qvx; qy += qvy; qvx -= qvx / 128; qvy -= qvy / 128;
    int px = qx / 16, py = qy / 16;
    if (py < 26) { qy = 26 * 16; qvy = ABS(qvy); cue(CUE_GOOD); } if (py > 150) { qy = 150 * 16; qvy = -ABS(qvy); }
    int ing = py > 62 && py < 112;
    if (!ing) { if (px < 12) { qx = 12 * 16; qvx = ABS(qvx); } if (px > 228) { qx = 228 * 16; qvx = -ABS(qvx); } }
    for (int s = 0; s < 2; s++) { int dx = px - mx_[s], dy = py - my_[s], d2 = dx * dx + dy * dy;
        if (d2 < 15 * 15 && d2 > 0) { int d = MAX(1, isq2(d2)); int sp = isq2(mvx_[s] * mvx_[s] + mvy_[s] * mvy_[s]); int pw = MAX(6, sp * 2 + 3) + hlv * (s ? 0 : 0); qvx = dx * pw * 16 / d; qvy = dy * pw * 16 / d; qx = (mx_[s] + dx * 15 / d) * 16; qy = (my_[s] + dy * 15 / d) * 16; cue(CUE_GOOD); part(px, py, 0, -8, 5, UI_WHITE);
            if (s == 0 && sp > 6) { ag_combo++; ag_best = MAX(ag_best, ag_combo); } } }
    { int vs = ABS(qvx) + ABS(qvy); if (vs > 220) { qvx = qvx * 220 / vs; qvy = qvy * 220 / vs; } }
    px = qx / 16;
    if (px < 3 || px > 237) { int w = px > 120 ? 0 : 1; hs_[w]++; hfz = 45; ag_shake = 6; burst(w ? 4 : 236, 87, 12, w ? UI_RGB(255, 100, 90) : UI_RGB(120, 160, 255)); qvx = qvy = 0; qx = 120 * 16; qy = 87 * 16;
        if (w == 0) { ag_pts += 20 + ag_combo * 2; ag_combo = 0; pop(180, 80, "GOAL!", UI_AMBER); cue(CUE_WIN); } else { ag_perf = 0; ag_combo = 0; pop(60, 80, hs_[1] == 6 ? "MATCH POINT" : "THEY SCORE", UI_RED); cue(CUE_BAD); } }
    if (hs_[0] >= 7 || hs_[1] >= 7 || ag_t >= 2700) { if (hs_[0] > hs_[1]) { ag_pts += 60 + (7 - hs_[1]) * 5; pop(120, 80, "YOU WIN!", UI_AMBER); cue(CUE_WIN); } return 0; }
    return 1;
}
void hoc_draw(void) {
    R(4, 18, 232, 138, 40, 70, 150); R(6, 20, 228, 134, 70, 130, 210); R(119, 20, 2, 134, 240, 240, 255); DSC(120, 87, 18, UI_RGB(110, 170, 235)); DSC(120, 87, 16, UI_RGB(70, 130, 210)); R(0, 62, 6, 50, 10, 10, 30); R(234, 62, 6, 50, 10, 10, 30); R(4, 62, 3, 50, 250, 80, 80); R(233, 62, 3, 50, 250, 80, 80);
    NUM(4, 2, "YOU", hs_[0], UI_RGB(255, 140, 120)); { char s[12]; snprintf(s, sizeof s, "%s %d", ag_np == 2 ? "P2" : "CPU", hs_[1]); TX(190, 2, s, UI_RGB(130, 170, 255)); } NUM(100, 2, "SCORE", ag_pts, UI_WHITE);
    for (int s = 0; s < 2; s++) { int x = mx_[s], y = my_[s]; DSC(x, y, 10, s ? UI_RGB(40, 60, 190) : UI_RGB(190, 50, 50)); DSC(x, y, 7, s ? UI_RGB(90, 120, 250) : UI_RGB(250, 100, 90)); DSC(x, y, 3, UI_RGB(255, 255, 255)); }
    DSC(qx / 16, qy / 16, 5, UI_RGB(20, 20, 30)); DSC(qx / 16, qy / 16, 3, UI_RGB(240, 240, 80));
    if (hfz > 0) TXC(120, 60, hs_[0] > hs_[1] ? "NICE!" : "KEEP GOING", UI_AMBER);
}

// ================= BEAT BORE =================
static struct { s16 y; u8 l, on; } rn[24];
static int rgr, rfev, rmult, rlast, rflash[4], rjt, rrun, rlane;
#define RHIT 132
#ifdef HOSTBOT
int rhy_bot(void) { int m = 0; static const int KM[4] = { 128, 64, 16, 32 }; for (int i = 0; i < 24; i++) if (rn[i].on && ABS(rn[i].y - RHIT) <= 2) m |= KM[rn[i].l]; return m; }
#endif
void rhy_start(void) { memset(rn, 0, sizeof rn); rgr = 60; rfev = 0; rjt = 0; rrun = 0; rlane = 1; ag_level = 1; memset(rflash, 0, sizeof rflash); }
static void rspawn(int l) { for (int i = 0; i < 24; i++) if (!rn[i].on) { rn[i].on = 1; rn[i].y = 14; rn[i].l = (u8)l; return; } }
int rhy_tick(int hd, int pr, int h2, int p2) {
    int t = ag_t; ag_level = 1 + t / 450; int beat = MAX(5, 9 - ag_level / 2);
    if (t % beat == 0 && t < 1700 && ar8() < 150 + ag_level * 20) {
        int r = ar8() % 10; if (r < 4) { int dl = (ar8() & 1) ? 1 : -1; rlane = CLAMP(rlane + dl, 0, 3); } else if (r < 6) rlane = ar8() & 3; rspawn(rlane);
        if (ag_level >= 2 && ar8() < 40 + ag_level * 15) rspawn((rlane + 1 + ar8() % 3) & 3);
    }
    int sp = 2 + (ag_level > 3);
    for (int i = 0; i < 24; i++) if (rn[i].on) { rn[i].y = (s16)(rn[i].y + sp); if (rn[i].y > RHIT + 16) { rn[i].on = 0; ag_combo = 0; ag_perf = 0; rgr -= 7; rflash[rn[i].l] = -6; cue(CUE_BAD); pop(48 + rn[i].l * 40 + 18, RHIT - 10, "MISS", UI_RED); } }
    static const int KM[4] = { PAD_LEFT | PAD_SQUARE, PAD_DOWN | PAD_CROSS, PAD_UP | PAD_TRIANGLE, PAD_RIGHT | PAD_CIRCLE };
    for (int l = 0; l < 4; l++) {
        if (rflash[l] > 0) rflash[l]--; if (rflash[l] < 0) rflash[l]++;
        if (pr & KM[l]) { rflash[l] = 6; int b = -1, bd = 99; for (int i = 0; i < 24; i++) if (rn[i].on && rn[i].l == l) { int d = ABS(rn[i].y - RHIT); if (d < bd) { bd = d; b = i; } }
            if (b >= 0 && bd <= 16) { rn[b].on = 0; int g = bd <= 5 ? 2 : bd <= 10 ? 1 : 0; ag_combo++; ag_best = MAX(ag_best, ag_combo); int m = 1 + MIN(3, ag_combo / 10); if (rfev > 0) m *= 2; int v = (g == 2 ? 10 : g == 1 ? 6 : 3) * m; ag_pts += v; rgr = MIN(100, rgr + (g == 2 ? 2 : g == 1 ? 1 : 0));
                pop(48 + l * 40 + 18, RHIT - 18, g == 2 ? "PERFECT" : g == 1 ? "GREAT" : "OK", g == 2 ? UI_AMBER : UI_WHITE); burst(48 + l * 40 + 18, RHIT, g == 2 ? 8 : 3, UI_RGB(l == 0 ? 250 : l == 1 ? 90 : l == 2 ? 80 : 250, l == 0 ? 60 : l == 1 ? 230 : l == 2 ? 240 : 220, l == 3 ? 40 : 90)); cue(g == 2 ? CUE_GREAT : CUE_GOOD);
                if (ag_combo % 16 == 0) { rfev = 150; pop(120, 60, "FEVER!", UI_RED); cue(CUE_WIN); } }
            else if (b < 0 || bd > 16) { if (ag_combo > 3) pop(48 + l * 40 + 18, RHIT - 18, "OOPS", UI_RED); ag_combo = 0; rgr -= 1; } }
    }
    if (rfev > 0) rfev--;
    if (rgr <= 0) { pop(120, 70, "GROOVE LOST", UI_RED); return 0; }
    return t < 1800;
}
void rhy_draw(void) {
    int on = (ag_t / MAX(5, 9 - ag_level / 2)) & 1; R(0, 14, 240, 146, on ? 24 : 18, 12, on ? 50 : 40);
    NUM(4, 2, "SCORE", ag_pts, UI_WHITE); { R(150, 3, 60, 5, 40, 30, 60); R(150, 3, MAX(0, rgr) * 60 / 100, 5, rgr < 25 ? 250 : 90, rgr < 25 ? 70 : 230, 90); TX(100, 2, "GROOVE", UI_RGB(170, 210, 250)); }
    for (int l = 0; l < 4; l++) { int x = 48 + l * 40; static const u8 lc[4][3] = { { 250, 60, 90 }, { 90, 230, 120 }, { 80, 240, 250 }, { 250, 220, 40 } };
        R(x, 14, 36, 146, 10 + lc[l][0] / 14, 8 + lc[l][1] / 14, 20 + lc[l][2] / 14); R(x, 14, 1, 146, lc[l][0] / 3, lc[l][1] / 3, lc[l][2] / 3);
        if (rflash[l] > 0) R(x, RHIT - 6, 36, 40, lc[l][0] / 3, lc[l][1] / 3, lc[l][2] / 3);
        R(x + 2, RHIT - 4, 32, 8, rflash[l] > 0 ? 255 : lc[l][0], rflash[l] > 0 ? 255 : lc[l][1], rflash[l] > 0 ? 255 : lc[l][2]);
        static const char *const lb = "<VA>"; static const char *const lb2 = "<v^>"; char s[2] = { lb2[l], 0 }; (void)lb; TXC(x + 18, RHIT + 12, s, UI_RGB(200, 200, 230)); }
    R(184, 14, 1, 146, 70, 70, 130);
    for (int i = 0; i < 24; i++) if (rn[i].on) { int l = rn[i].l, x = 48 + l * 40; static const u8 lc[4][3] = { { 250, 60, 90 }, { 90, 230, 120 }, { 80, 240, 250 }, { 250, 220, 40 } };
        R(x + 3, rn[i].y - 3, 30, 8, lc[l][0], lc[l][1], lc[l][2]); R(x + 3, rn[i].y - 3, 30, 2, 255, 255, 255); R(x + 3, rn[i].y + 4, 30, 1, 40, 30, 50); }
    if (ag_combo >= 4) { char s[16]; snprintf(s, sizeof s, "%d", ag_combo); TXB(214, 60, s, UI_AMBER); TXC(214, 82, "COMBO", UI_WHITE); }
    if (rfev > 0) { TXC(214, 100, "FEVER x2", UI_RED); }
    { int w = (1800 - ag_t) * 36 / 1800; R(6, 20, 4, 36, 40, 30, 60); R(6, 56 - w, 4, w, 120, 200, 250); }
}

// ================= CLAW GRAB =================
static struct { s16 x; u8 k, on; } cp[8];
static int clx, cly, cst, ctm, ctry, cheld, cwait, cgood, cprz;
static const u8 cv[5] = { 1, 2, 3, 4, 6 };
static void cfill(void) { for (int i = 0; i < 8; i++) { cp[i].on = 1; cp[i].x = (s16)(60 + i * 22 + (ar8() % 9)); int r = ar8(); cp[i].k = (u8)(r < 70 ? 0 : r < 130 ? 1 : r < 185 ? 2 : r < 235 ? 3 : 4); } }
void claw_start(void) { cfill(); clx = 120; cly = 28; cst = 0; ctm = 0; ctry = 6; cheld = -1; cwait = 0; cgood = 0; cprz = 0; }
static int cnear(void) { int b = -1, bd = 99; for (int i = 0; i < 8; i++) if (cp[i].on) { int d = ABS(cp[i].x + 8 - clx); if (d < bd) { bd = d; b = i; } } return bd <= 11 ? b : -1; }
int claw_tick(int hd, int pr, int h2, int p2) {
    switch (cst) {
    case 0:
        if (hd & PAD_LEFT) clx -= 3; if (hd & PAD_RIGHT) clx += 3; clx = CLAMP(clx, 52, 224); cwait++;
        if ((pr & PAD_CROSS) || cwait > 450) { cst = 1; ctm = 0; ctry--; cwait = 0; }
        break;
    case 1: cly += 4; if (cly >= 112) { cly = 112; cst = 2; ctm = 0; } break;
    case 2: ctm++; if (ctm == 12) { int b = cnear(); cheld = -1; cgood = 0;
            if (b >= 0) { int v = cv[cp[b].k], dd = ABS(cp[b].x + 8 - clx), pc = 92 - v * 9 + (dd <= 3 ? 16 : 0) + (ag_combo >= 2 ? 4 : 0); cgood = dd <= 3; if ((int)(ar8() % 100) < pc) { cheld = b; cue(CUE_GOOD); } else { pop(clx, 100, "SLIP", UI_RED); cue(CUE_BAD); } } else pop(clx, 100, "MISSED", UI_RED); }
        if (ctm >= 20) { cst = 3; ctm = 0; } break;
    case 3: cly -= 3; if (cheld >= 0 && cly < 76 && cly >= 70 && cp[cheld].on && (int)(ar8() % 100) < 6 + cv[cp[cheld].k] * 3 - (cgood ? 6 : 0)) { cheld = -1; pop(clx, 70, "OH NO", UI_RED); cue(CUE_BAD); }
        if (cly <= 28) { cly = 28; cst = 4; ctm = 0; } break;
    case 4: if (clx > 36) clx -= 4; else { cst = 5; ctm = 0; if (cheld >= 0) { int v = cv[cp[cheld].k], pts = v * 2 + (cp[cheld].k == 4 ? 4 : 0); ag_pts += pts; cp[cheld].on = 0; ag_combo++; ag_best = MAX(ag_best, ag_combo); cprz++; char s[12]; snprintf(s, sizeof s, "+%d", pts); pop(clx + 10, 110, s, UI_AMBER); cue(cp[cheld].k == 4 ? CUE_WIN : CUE_GREAT); burst(26, 120, 8, UI_RGB(255, 200, 100)); } else { ag_combo = 0; ag_perf = 0; } } break;
    case 5: if (++ctm > 20) { cheld = -1; int n = 0; for (int i = 0; i < 8; i++) n += cp[i].on; if (n < 3) cfill(); if (ctry <= 0) return 0; cst = 0; clx = 120; } break;
    }
    return 1;
}
static void plush(int x, int y, int k) {
    static const u8 pc[5][3] = { { 200, 130, 70 }, { 250, 250, 250 }, { 90, 210, 100 }, { 150, 160, 190 }, { 255, 215, 60 } };
    int r = pc[k][0], g = pc[k][1], b = pc[k][2];
    R(x, y + 6, 16, 12, r, g, b); R(x + 2, y, 12, 9, r, g, b); R(x + 2, y - 3, 3, 4, r, g, b); R(x + 11, y - 3, 3, 4, r, g, b); R(x + 4, y + 3, 2, 2, 10, 10, 20); R(x + 10, y + 3, 2, 2, 10, 10, 20); R(x + 6, y + 6, 4, 2, r / 2, g / 2, b / 2);
    if (k == 4) { R(x + 5, y - 6, 6, 3, 255, 255, 255); R(x - 1, y + 7, 2, 4, 255, 255, 220); }
    if (k == 2) { R(x + 2, y - 2, 3, 3, 255, 255, 255); R(x + 11, y - 2, 3, 3, 255, 255, 255); }
    if (k == 3) { R(x + 7, y - 4, 2, 4, 255, 80, 80); }
}
void claw_draw(void) {
    R(0, 14, 240, 146, 70, 40, 90); R(44, 18, 188, 128, 20, 24, 50); R(46, 20, 184, 124, 34, 40, 80); R(44, 142, 188, 4, 250, 120, 190);
    R(6, 100, 38, 46, 30, 20, 50); R(10, 104, 30, 40, 10, 10, 20); TXC(25, 90, "CHUTE", UI_RGB(255, 220, 120)); R(44, 22, 188, 3, 150, 150, 170);
    for (int i = 0; i < 8; i++) if (cp[i].on && !(cheld == i && cst >= 3)) plush(cp[i].x, 124, cp[i].k);
    { int x = clx; R(x - 1, 24, 2, cly - 24, 190, 190, 205); R(x - 8, 20, 16, 5, 120, 120, 150);
      int op = cst == 2 ? MAX(0, 6 - ctm / 2) : cst == 0 || cst == 1 ? 6 : 0; if (cst >= 3 && cheld < 0) op = 3;
      u32 col = cgood && cst >= 2 ? UI_RGB(90, 255, 120) : UI_RGB(230, 230, 245); R(x - 4, cly, 8, 5, 150, 150, 170); RC(x - 5 - op, cly + 4, 3, 10, col); RC(x + 3 + op, cly + 4, 3, 10, col); RC(x - 4 - op, cly + 12, 3, 3, col); RC(x + 1 + op, cly + 12, 3, 3, col);
      if (cheld >= 0 && cst >= 3 && cp[cheld].on) plush(x - 8, cly + 6, cp[cheld].k); }
    NUM(4, 2, "PLUSH", ag_pts, UI_WHITE); { char s[16]; snprintf(s, sizeof s, "TRIES %d", ctry); TXC(120, 2, s, UI_AMBER); } if (cprz) { char s[12]; snprintf(s, sizeof s, "WON %d", cprz); TX(190, 2, s, UI_GREEN); }
    if (cst == 0) { TXC(138, 150, "LEFT RIGHT. X DROPS", UI_RGB(170, 210, 250)); }
}
