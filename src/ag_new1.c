// ag_new1.c - four new cabinets: BORE BLOCKS (falling blocks), STAR BORE (shoot-em-up, two ships), TURBO BORE (road racer), MUNCH MAZE (maze chase).
// Each is start / tick (30 Hz) / draw on the 240 x 160 field (arc_i.h) and scores into ag_pts.
#include "arc_i.h"

// ================= BORE BLOCKS =================
#define BR 17
static u8 bd[BR][10]; static u16 bm; static int bxp, byp, bnx, bk, bnk, bfall, block, bgar, blines, bdas, bhigh;
static const char *const PS[7] = { "....XXXX........", ".XX..XX.........", ".X..XXX.........", ".XX.XX..........", "XX...XX.........", "X...XXX.........", "..X.XXX........." };
static const u8 pcol[7][3] = { { 60, 220, 250 }, { 250, 220, 40 }, { 170, 90, 250 }, { 80, 220, 90 }, { 250, 70, 70 }, { 70, 110, 250 }, { 250, 150, 40 } };
static u16 pmask(int k) { u16 m = 0; for (int i = 0; i < 16; i++) if (PS[k][i] == 'X') m |= (u16)(1 << i); return m; }
static u16 prot(u16 m) { u16 n = 0; for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++) if (m >> (y * 4 + x) & 1) n |= (u16)(1 << (x * 4 + 3 - y)); return n; }
static int bok(u16 m, int x, int y) {
    for (int i = 0; i < 16; i++) if (m >> i & 1) { int cx = x + (i & 3), cy = y + (i >> 2); if (cx < 0 || cx > 9 || cy >= BR) return 0; if (cy >= 0 && bd[cy][cx]) return 0; }
    return 1;
}
static int bspawn(void) { bk = bnk; bnk = ar8() % 7; bm = pmask(bk); bxp = 3; byp = -1; bfall = 0; block = 0; return bok(bm, bxp, byp); }
void blk_start(void) { memset(bd, 0, sizeof bd); bgar = 0; blines = 0; bdas = 0; bhigh = 0; bnk = ar8() % 7; ag_level = 1; bspawn(); }
static void bgarbage(void) {
    for (int x = 0; x < 10; x++) if (bd[0][x]) { bhigh = 99; }
    memmove(bd[0], bd[1], (BR - 1) * 10); int h = ar8() % 10; for (int x = 0; x < 10; x++) bd[BR - 1][x] = (u8)(x == h ? 0 : 8);
    if (!bok(bm, bxp, byp)) byp--; ag_shake = 4; ag_combo = 0; pop(120, 60, "RISING!", UI_RED); cue(CUE_BAD);
}
int blk_tick(int hd, int pr, int h2, int p2) {
    if (bhigh == 99) return 0;
    ag_level = 1 + blines / 8;
    if (pr & PAD_LEFT) { if (bok(bm, bxp - 1, byp)) bxp--; bdas = 0; } if (pr & PAD_RIGHT) { if (bok(bm, bxp + 1, byp)) bxp++; bdas = 0; }
    if (hd & (PAD_LEFT | PAD_RIGHT)) { if (++bdas > 8 && !(bdas & 1)) { int d = (hd & PAD_LEFT) ? -1 : 1; if (bok(bm, bxp + d, byp)) bxp += d; } } else bdas = 0;
    if (pr & (PAD_CROSS | PAD_CIRCLE)) { u16 r = prot(bm); if (pr & PAD_CIRCLE) r = prot(prot(r)); if (bk != 1) for (int k = 0; k < 3; k++) { static const s8 kk[3] = { 0, 1, -1 }; if (bok(r, bxp + kk[k], byp)) { bm = r; bxp += kk[k]; cue(CUE_GOOD); break; } } }
    int hard = (pr & PAD_UP) != 0;
    if (hard) { int n = 0; while (bok(bm, bxp, byp + 1)) { byp++; n++; } ag_pts += n; block = 99; ag_shake = 3; }
    int iv = MAX(2, 22 - ag_level * 2); if (hd & PAD_DOWN) iv = 2;
    if (++bfall >= iv && !hard) { bfall = 0; if (bok(bm, bxp, byp + 1)) { byp++; if (hd & PAD_DOWN) ag_pts++; block = 0; } else block += 2; }
    if (!bok(bm, bxp, byp + 1) && (++block >= 12 || hard)) {   // it locks
        for (int i = 0; i < 16; i++) if (bm >> i & 1) { int cx = bxp + (i & 3), cy = byp + (i >> 2); if (cy < 0) return 0; bd[cy][cx] = (u8)(bk + 1); }
        int n = 0; for (int y = 0; y < BR; y++) { int f = 1; for (int x = 0; x < 10; x++) if (!bd[y][x]) f = 0;
            if (f) { n++; for (int x = 0; x < 10; x++) burst(75 + x * 8 + 4, 16 + y * 8 + 4, 1, UI_RGB(pcol[bd[y][x] > 7 ? 0 : bd[y][x] - 1][0], pcol[bd[y][x] > 7 ? 0 : bd[y][x] - 1][1], pcol[bd[y][x] > 7 ? 0 : bd[y][x] - 1][2])); memmove(bd[1], bd[0], y * 10); memset(bd[0], 0, 10); } }
        if (n) { static const u8 sc[5] = { 0, 10, 30, 60, 100 }; ag_combo++; ag_best = MAX(ag_best, ag_combo); blines += n; int v = sc[n] * ag_level + ag_combo * 5; ag_pts += v; char s[12]; snprintf(s, sizeof s, n == 4 ? "BORIS! +%d" : "+%d", v); pop(120, 70, s, n == 4 ? UI_AMBER : UI_WHITE); cue(n == 4 ? CUE_WIN : n >= 2 ? CUE_GREAT : CUE_GOOD); if (n == 4) ag_shake = 8; }
        else { ag_combo = 0; cue(CUE_GOOD); }
        for (int x = 0; x < 10; x++) if (bd[3][x]) { ag_perf = 0; break; }
        if (!bspawn()) return 0;
    }
    if (++bgar >= MAX(240, 560 - ag_level * 40)) { bgar = 0; bgarbage(); }
    return ag_t < 5400;
}
static void bcell(int x, int y, int k) {
    int c = k > 7 ? 7 : k - 1; static const u8 gc[3] = { 110, 110, 125 };
    if (k > 7) { R(x, y, 8, 8, gc[0], gc[1], gc[2]); R(x, y, 8, 1, 170, 170, 185); R(x, y + 7, 8, 1, 60, 60, 75); return; }
    R(x, y, 8, 8, pcol[c][0], pcol[c][1], pcol[c][2]); R(x, y, 8, 1, 255, 255, 255); R(x, y, 1, 8, 255, 255, 255); R(x, y + 7, 8, 1, pcol[c][0] / 3, pcol[c][1] / 3, pcol[c][2] / 3); R(x + 7, y, 1, 8, pcol[c][0] / 3, pcol[c][1] / 3, pcol[c][2] / 3);
}
void blk_draw(void) {
    R(73, 14, 84, 139, 8, 8, 24); R(72, 14, 1, 139, 90, 90, 170); R(157, 14, 1, 139, 90, 90, 170); R(72, 153, 86, 2, 90, 90, 170);
    for (int y = 0; y < BR; y++) for (int x = 0; x < 10; x++) { if (bd[y][x]) bcell(75 + x * 8, 16 + y * 8, bd[y][x]); else if (((x + y) & 1) == 0) R(75 + x * 8, 16 + y * 8, 8, 8, 12, 12, 32); }
    { int gy = byp; while (bok(bm, bxp, gy + 1)) gy++; for (int i = 0; i < 16; i++) if (bm >> i & 1) { int cy = gy + (i >> 2); if (cy >= 0) RA(75 + (bxp + (i & 3)) * 8, 16 + cy * 8, 8, 8, UI_RGB(90, 90, 130)); } }
    for (int i = 0; i < 16; i++) if (bm >> i & 1) { int cy = byp + (i >> 2); if (cy >= 0) bcell(75 + (bxp + (i & 3)) * 8, 16 + cy * 8, bk + 1); }
    NUM(4, 16, "SCORE", ag_pts, UI_WHITE); NUM(4, 30, "LINES", blines, UI_RGB(170, 210, 250)); NUM(4, 44, "LEVEL", ag_level, UI_AMBER);
    if (ag_combo >= 2) { char s[16]; snprintf(s, sizeof s, "COMBO %d", ag_combo); TX(4, 60, s, UI_AMBER); }
    TX(168, 16, "NEXT", UI_RGB(170, 170, 210)); { u16 m = pmask(bnk); for (int i = 0; i < 16; i++) if (m >> i & 1) bcell(172 + (i & 3) * 8, 30 + (i >> 2) * 8, bnk + 1); }
    { int w = bgar * 60 / MAX(240, 560 - ag_level * 40); TX(168, 80, "FLOOR RISES", UI_RGB(170, 170, 210)); R(168, 92, 60, 4, 40, 40, 70); R(168, 92, w, 4, w > 45 ? 250 : 220, w > 45 ? 70 : 180, 60); }
    TX(168, 108, "UP DROP", UI_RGB(120, 120, 160)); TX(168, 118, "X O TURN", UI_RGB(120, 120, 160));
    { int left = (5400 - ag_t) / 30; char s[12]; snprintf(s, sizeof s, "%d:%02d", left / 60, left % 60); TXC(120, 2, s, UI_RGB(170, 210, 250)); }
}

// ================= STAR BORE =================
static struct { s16 x, y; s8 vx, vy; u8 k, hp, t, on; } se[10];
static struct { s16 x, y; s8 vx, vy; u8 on; } sbu[18], pbu[14];
static struct { s16 x, y; u8 k, on; } spu[3];
static int sx[2], sy[2], sfire[2], spow, sshield, sbomb, sinv, sboss, sbx, sby, sbhp, swave, sflash;
void shm_start(void) {
    memset(se, 0, sizeof se); memset(sbu, 0, sizeof sbu); memset(pbu, 0, sizeof pbu); memset(spu, 0, sizeof spu);
    sx[0] = 24; sy[0] = ag_np == 2 ? 60 : 80; sx[1] = 24; sy[1] = 110; spow = 1; sshield = 0; sbomb = 2; sinv = 60; sboss = 0; sbhp = 0; swave = 0; sfire[0] = sfire[1] = 0; ag_lives = 3;
}
static void sspawn(int k, int x, int y, int vx, int vy, int hp) { for (int i = 0; i < 10; i++) if (!se[i].on) { se[i].on = 1; se[i].k = (u8)k; se[i].x = (s16)x; se[i].y = (s16)y; se[i].vx = (s8)vx; se[i].vy = (s8)vy; se[i].hp = (u8)hp; se[i].t = 0; return; } }
static void sebul(int x, int y, int tx, int ty, int sp) {
    for (int i = 0; i < 18; i++) if (!sbu[i].on) { int dx = tx - x, dy = ty - y, d = MAX(1, ABS(dx) + ABS(dy)); sbu[i].on = 1; sbu[i].x = (s16)x; sbu[i].y = (s16)y; sbu[i].vx = (s8)(dx * sp * 4 / d); sbu[i].vy = (s8)(dy * sp * 4 / d); return; }
}
static void sscore(int k, int x, int y) {
    int v = k == 3 ? 60 : k == 2 ? 25 : k == 1 ? 20 : 10; ag_combo++; ag_best = MAX(ag_best, ag_combo); v *= 1 + MIN(3, ag_combo / 8); ag_pts += v;
    char s[8]; snprintf(s, sizeof s, "+%d", v); pop(x, y - 6, s, UI_WHITE); burst(x, y, k == 3 ? 14 : 6, UI_RGB(255, 190, 60)); cue(k >= 2 ? CUE_GREAT : CUE_GOOD);
    if (ar8() < (k == 3 ? 200 : 26)) for (int i = 0; i < 3; i++) if (!spu[i].on) { spu[i].on = 1; spu[i].x = (s16)x; spu[i].y = (s16)y; spu[i].k = (u8)(ar8() % 3); break; }
}
static void shurt(void) { if (sinv > 0) return; if (sshield) { sshield = 0; sinv = 40; pop(60, 70, "SHIELD GONE", UI_RGB(120, 200, 255)); return; } ag_lives--; ag_perf = 0; ag_combo = 0; spow = MAX(1, spow - 1); sinv = 70; ag_shake = 10; cue(CUE_BAD); for (int s = 0; s < 2; s++) burst(sx[s], sy[s], 10, UI_RGB(255, 120, 60)); }
int shm_tick(int hd, int pr, int h2, int p2) {
    int t = ag_t; ag_level = 1 + t / 500; if (sinv > 0) sinv--; if (sflash > 0) sflash--;
    for (int s = 0; s < (ag_np == 2 ? 2 : 1); s++) {
        int h = s ? h2 : hd, p = s ? p2 : pr, sp = 3; if (h & PAD_LEFT) sx[s] -= sp; if (h & PAD_RIGHT) sx[s] += sp; if (h & PAD_UP) sy[s] -= sp; if (h & PAD_DOWN) sy[s] += sp; sx[s] = CLAMP(sx[s], 8, 200); sy[s] = CLAMP(sy[s], 22, 150);
        if (sfire[s] > 0) sfire[s]--;
        if ((h & PAD_CROSS) && sfire[s] == 0) { sfire[s] = 5; int n = spow; for (int j = 0; j < n; j++) for (int i = 0; i < 14; i++) if (!pbu[i].on) { pbu[i].on = 1; pbu[i].x = (s16)(sx[s] + 8); pbu[i].y = (s16)(sy[s] + 3); pbu[i].vx = 9; pbu[i].vy = (s8)(n == 1 ? 0 : (j - (n - 1) / 2) * 2); break; } if (!(t & 3)) cue(CUE_GOOD); }
        if ((p & PAD_CIRCLE) && sbomb > 0) { sbomb--; sflash = 8; ag_shake = 8; memset(sbu, 0, sizeof sbu); for (int i = 0; i < 10; i++) if (se[i].on) { se[i].hp = (u8)MAX(0, se[i].hp - 6); if (!se[i].hp) { sscore(se[i].k, se[i].x + 6, se[i].y + 4); se[i].on = 0; } } if (sboss) sbhp -= 12; cue(CUE_WIN); }
    }
    // the waves
    if (!sboss) {
        int ph = t % 90;
        if (ph == 0 && t < 1700) { swave++; int w = swave % 5, lv = ag_level;
            if (w == 0 || w == 3) for (int i = 0; i < 4 + lv / 2; i++) sspawn(0, 244 + i * 18, 40 + (i & 1) * 50 + ar8() % 40, -(2 + lv / 2), 0, 1);
            else if (w == 1) for (int i = 0; i < 4; i++) sspawn(1, 244 + i * 14, 24 + i * 30, -3, 0, 1);
            else if (w == 2) { sspawn(2, 244, 40 + ar8() % 80, -1, 0, 3); sspawn(2, 280, 40 + ar8() % 80, -1, 0, 3); }
            else sspawn(3, 244, 60 + ar8() % 40, -1, 0, 8 + lv * 2); }
        if (t == 1700) { sboss = 1; sbx = 250; sby = 70; sbhp = 90 + ag_np * 30; pop(120, 60, "WARNING!", UI_RED); cue(CUE_LOSE); }
    } else {
        if (sbx > 180) sbx -= 2; else { sby = 70 + ((t % 120) < 60 ? (t % 60) : 60 - (t % 60)) - 30; if (t % 24 == 0) for (int a = -2; a <= 2; a++) sebul(sbx, sby + 8, sx[0] - 60, sy[0] + a * 22 + 4, 2 + ag_level / 3); if (t % 70 == 0) sebul(sbx, sby + 8, sx[ar8() & (ag_np - 1)], sy[0], 3); }
        if (sbhp <= 0) { ag_pts += 500; burst(sbx, sby + 8, 30, UI_RGB(255, 200, 60)); pop(120, 70, "BOSS DOWN! +500", UI_AMBER); cue(CUE_WIN); ag_shake = 16; sboss = 0; return 0; }
    }
    for (int i = 0; i < 10; i++) if (se[i].on) {
        se[i].t++; se[i].x += se[i].vx; if (se[i].k == 0) se[i].y = (s16)(se[i].y + ((se[i].t & 31) < 16 ? 1 : -1) * 2); else if (se[i].k == 1) { se[i].y += (s16)(se[i].t > 24 ? 2 : 0); if (se[i].t > 24) se[i].x -= 1; }
        if ((se[i].k == 2 || se[i].k == 3) && se[i].x < 200 && se[i].t % MAX(16, 44 - ag_level * 4) == 0) sebul(se[i].x, se[i].y + 4, sx[0], sy[0] + 4, 2 + ag_level / 3);
        if (se[i].k == 3 && se[i].x < 190) se[i].vx = 0;
        if (se[i].k == 0 && se[i].t % 40 == 20 && ag_level >= 2) sebul(se[i].x, se[i].y + 4, sx[0], sy[0] + 4, 2);
        if (se[i].x < -30 || se[i].y > 170 || se[i].y < 8) { se[i].on = 0; continue; }
        for (int s = 0; s < (ag_np == 2 ? 2 : 1); s++) if (ABS(se[i].x + 6 - sx[s] - 8) < 12 && ABS(se[i].y + 4 - sy[s] - 4) < 9) { shurt(); if (se[i].k < 2) se[i].on = 0; if (ag_lives <= 0) return 0; }
    }
    for (int i = 0; i < 14; i++) if (pbu[i].on) { pbu[i].x += pbu[i].vx; pbu[i].y += pbu[i].vy; if (pbu[i].x > 244) pbu[i].on = 0;
        for (int j = 0; j < 10; j++) if (se[j].on && pbu[i].on && pbu[i].x > se[j].x - 2 && pbu[i].x < se[j].x + (se[j].k == 3 ? 24 : 12) && pbu[i].y > se[j].y - 2 && pbu[i].y < se[j].y + (se[j].k == 3 ? 16 : 9)) { pbu[i].on = 0; if (--se[j].hp == 0) { sscore(se[j].k, se[j].x + 6, se[j].y + 4); se[j].on = 0; } else part(pbu[i].x, pbu[i].y, 0, 0, 3, UI_WHITE); }
        if (sboss && pbu[i].on && pbu[i].x > sbx && pbu[i].y > sby - 6 && pbu[i].y < sby + 26) { pbu[i].on = 0; sbhp--; part(pbu[i].x, pbu[i].y, 0, 0, 3, UI_RGB(255, 200, 80)); if (sbhp % 10 == 0) cue(CUE_GREAT); } }
    for (int i = 0; i < 18; i++) if (sbu[i].on) { sbu[i].x += sbu[i].vx / 4; sbu[i].y += sbu[i].vy / 4; if (sbu[i].x < -4 || sbu[i].y < 10 || sbu[i].y > 164) sbu[i].on = 0;
        for (int s = 0; s < (ag_np == 2 ? 2 : 1); s++) if (sbu[i].on && ABS(sbu[i].x - sx[s] - 8) < 6 && ABS(sbu[i].y - sy[s] - 4) < 5) { sbu[i].on = 0; shurt(); if (ag_lives <= 0) return 0; } }
    for (int i = 0; i < 3; i++) if (spu[i].on) { spu[i].x -= 1; for (int s = 0; s < (ag_np == 2 ? 2 : 1); s++) if (ABS(spu[i].x - sx[s] - 8) < 12 && ABS(spu[i].y - sy[s] - 4) < 10) { spu[i].on = 0; cue(CUE_GREAT);
        if (spu[i].k == 0) { spow = MIN(3, spow + 1); pop(sx[s], sy[s] - 8, "POWER UP", UI_AMBER); } else if (spu[i].k == 1) { sshield = 1; pop(sx[s], sy[s] - 8, "SHIELD", UI_RGB(120, 200, 255)); } else { sbomb = MIN(4, sbomb + 1); pop(sx[s], sy[s] - 8, "BOMB", UI_RED); } } if (spu[i].x < -10) spu[i].on = 0; }
    return t < 2700;
}
static void ship(int x, int y, int c) {
    R(x, y + 2, 14, 5, c ? 250 : 70, c ? 150 : 200, c ? 60 : 250); R(x + 3, y, 8, 3, 230, 230, 255); R(x + 8, y + 3, 6, 2, 255, 255, 255); R(x - 2, y + 1, 3, 2, 255, 160, 40 + (ag_t & 3) * 40); R(x + 2, y + 6, 6, 3, 40, 80, 140);
    if (sshield) DSC(x + 7, y + 4, 10, UI_RGB(60, 120, 200));
}
void shm_draw(void) {
    for (int i = 0; i < 24; i++) { int sp = 1 + (i & 3); R(((i * 53 + 240 * 8 - ag_t * sp) % 240 + 240) % 240, 18 + (i * 37) % 138, sp > 2 ? 2 : 1, 1, 150 + sp * 25, 150 + sp * 25, 255); }
    NUM(4, 2, "SCORE", ag_pts, UI_WHITE); hearts(ag_lives, 5); { char s[10]; snprintf(s, sizeof s, "BOMB %d", sbomb); TXC(120, 2, s, UI_RGB(255, 130, 90)); }
    for (int i = 0; i < 3; i++) if (spu[i].on) { static const u8 pc[3][3] = { { 255, 220, 60 }, { 90, 180, 255 }, { 255, 90, 90 } }; R(spu[i].x, spu[i].y, 10, 10, pc[spu[i].k][0], pc[spu[i].k][1], pc[spu[i].k][2]); char s[2] = { "PSB"[spu[i].k], 0 }; TX(spu[i].x + 2, spu[i].y, s, UI_RGB(10, 10, 30)); }
    for (int i = 0; i < 10; i++) if (se[i].on) { int x = se[i].x, y = se[i].y;
        switch (se[i].k) {
        case 0: R(x, y + 2, 12, 5, 250, 80, 150); R(x + 3, y, 6, 3, 255, 170, 210); R(x + 1, y + 5, 3, 3, 150, 40, 90); R(x + 8, y + 5, 3, 3, 150, 40, 90); break;
        case 1: R(x, y + 3, 12, 4, 120, 230, 90); R(x + 4, y, 4, 4, 190, 255, 160); R(x + 2, y + 6, 8, 2, 60, 140, 50); break;
        case 2: DSC(x + 6, y + 5, 6, UI_RGB(200, 150, 60)); R(x + 3, y + 3, 6, 4, 110, 70, 30); R(x - 3, y + 4, 6, 2, 250, 210, 80); break;
        default: R(x, y + 2, 24, 12, 180, 60, 60); R(x + 4, y, 16, 4, 240, 110, 90); R(x + 2, y + 8, 6, 6, 255, 220, 80); R(x + 16, y + 8, 6, 6, 255, 220, 80); R(x + 9, y + 5, 6, 4, 30, 20, 40); } }
    if (sboss) { R(sbx, sby, 40, 28, 90, 50, 130); R(sbx + 4, sby - 4, 32, 6, 160, 100, 220); R(sbx + 6, sby + 8, 10, 8, 255, 230, 70); R(sbx + 24, sby + 8, 10, 8, 255, 230, 70); R(sbx + 14, sby + 20, 12, 4, 20, 10, 30);
      R(60, 150, 120, 4, 40, 20, 40); R(60, 150, MAX(0, sbhp) * 120 / (90 + ag_np * 30), 4, 250, 60, 90); TXC(120, 142, "BOSS", UI_RGB(255, 140, 160)); }
    for (int i = 0; i < 14; i++) if (pbu[i].on) { R(pbu[i].x, pbu[i].y, 6, 2, 255, 255, 120); }
    for (int i = 0; i < 18; i++) if (sbu[i].on) { R(sbu[i].x - 2, sbu[i].y - 2, 4, 4, 255, 90, 60); R(sbu[i].x - 1, sbu[i].y - 1, 2, 2, 255, 230, 200); }
    for (int s = 0; s < (ag_np == 2 ? 2 : 1); s++) if (!(sinv > 0 && (ag_t & 2))) ship(sx[s], sy[s], s);
    if (sflash > 0) RA(0, 14, 240, 146, UI_RGB(200, 200, 255));
    if (ag_combo >= 8) { char s[16]; snprintf(s, sizeof s, "CHAIN %d", ag_combo); TXC(120, 150, s, UI_AMBER); }
    if (spow > 1) { char s[8]; snprintf(s, sizeof s, "POW %d", spow); TX(4, 150, s, UI_AMBER); }
}

// ================= TURBO BORE =================
static int rbonus, rpx, rsp, rdist, rclock, rcur, rnitro, rcheck, rnos, rcrash, rcp_t;
static struct { s16 z; s8 lane; u8 sp, on, col, passed; } rcar[6];
static const s8 rseg[8] = { 0, 4, -4, 6, 0, -6, 3, -3 };
void rac_start(void) { memset(rcar, 0, sizeof rcar); rpx = 0; rsp = 0; rdist = 0; rclock = 1500; rcur = 0; rnitro = 3; rcheck = 1; rnos = 0; rcrash = 0; rcp_t = 0; rbonus = 0; ag_level = 1; }
static void RS(int x, int y, int w, int h, int r, int g, int b) { if (x < 0) { w += x; x = 0; } if (x + w > 240) w = 240 - x; if (w > 0) R(x, y, w, h, r, g, b); }
static int rhalf(int r) { return 4 + r * 19 / 10; }
static int rcen(int r, int px) { int c = rcur * (52 - r) * (52 - r) / 1500 / 9; return 120 - px * rhalf(r) / 110 + c; }
int rac_tick(int hd, int pr, int h2, int p2) {
    int mx = rnos > 0 ? 140 : 100; ag_level = 1 + rdist / 9000;
    if (rcrash > 0) { rcrash--; rsp = rsp * 9 / 10; } else if (hd & PAD_CROSS) rsp = MIN(mx, rsp + 2); else rsp = MAX(0, rsp - 1);
    if ((hd & PAD_SQUARE) || (hd & PAD_TRIANGLE)) rsp = MAX(0, rsp - 3);
    if ((pr & PAD_CIRCLE) && rnitro > 0 && rnos <= 0) { rnitro--; rnos = 60; rsp = MIN(140, rsp + 30); cue(CUE_WIN); ag_shake = 6; }
    if (rnos > 0) rnos--;
    int seg = (rdist / 1800) % 8, tg = rseg[seg] * 4; rcur += (tg - rcur) / 12;
    int st = 2 + rsp / 25; if (hd & PAD_LEFT) rpx -= st; if (hd & PAD_RIGHT) rpx += st; rpx -= rcur * rsp / 2000;
    rpx = CLAMP(rpx, -130, 130);
    if (ABS(rpx) > 104) { if (rsp > 40) rsp -= 3; if (!(ag_t & 7)) part(120 + (rpx > 0 ? 24 : -24), 150, 0, -10, 6, UI_RGB(120, 160, 70)); ag_perf = 0; }
    rdist += rsp / 8; ag_pts = rdist / 10 + rbonus;
    rclock--;
    if (rdist >= rcheck * 9000) { rcheck++; rclock += 600; rcp_t = 45; pop(120, 50, "CHECKPOINT +20S", UI_AMBER); cue(CUE_WIN); rbonus += 100; }
    if (rcp_t > 0) rcp_t--;
    if (rclock <= 0) return 0;
    int every = MAX(26, 60 - ag_level * 8);
    if (ag_t % every == 0) for (int i = 0; i < 6; i++) if (!rcar[i].on) { rcar[i].on = 1; rcar[i].z = 520; rcar[i].lane = (s8)((ar8() % 3) - 1); rcar[i].sp = (u8)(20 + ar8() % 40 + ag_level * 3); rcar[i].col = (u8)(ar8() % 4); rcar[i].passed = 0; break; }
    for (int i = 0; i < 6; i++) if (rcar[i].on) {
        rcar[i].z = (s16)(rcar[i].z - (rsp - rcar[i].sp) / 6);
        if (rcar[i].z > 560) { rcar[i].on = 0; continue; }
        int wx = rcar[i].lane * 60;
        if (rcar[i].z < 6 && rcar[i].z > -6 && ABS(wx - rpx) < 34 && rcrash == 0) { rcrash = 30; rsp = rsp / 3; ag_combo = 0; ag_perf = 0; ag_shake = 10; cue(CUE_BAD); burst(120, 140, 10, UI_RGB(255, 150, 60)); rcar[i].on = 0; rclock -= 60; }
        else if (rcar[i].z < -6) { if (!rcar[i].passed && ABS(wx - rpx) < 70 && ABS(wx - rpx) >= 34 && rsp > 40) { ag_combo++; ag_best = MAX(ag_best, ag_combo); int v = 20 * MIN(5, ag_combo); rbonus += v; char s[12]; snprintf(s, sizeof s, "CLOSE! +%d", v); pop(120, 100, s, UI_WHITE); cue(CUE_GOOD); } rcar[i].on = 0; }
    }
    return 1;
}
void rac_draw(void) {
    int hz = 58; static const u8 gr[2][3] = { { 40, 150, 50 }, { 30, 125, 42 } }, rd[2][3] = { { 90, 90, 100 }, { 80, 80, 92 } };
    R(0, 14, 240, hz - 14, 80, 150, 240); R(0, 40, 240, 18, 140, 190, 250); DSC(170, 40, 11, UI_RGB(255, 230, 120));
    for (int i = 0; i < 6; i++) R(i * 44 - (rdist / 40) % 44, hz - 12 - (i * 7) % 11, 36, 12 + (i * 7) % 11, 60, 70, 140);
    for (int r = 1; r <= 52; r++) {
        int y = hz + (r - 1) * 2, ph = ((rdist + 52 * 90 / r * 6 / 4) / 360) & 1, cx = rcen(r, rpx), hw = rhalf(r);
        R(0, y, 240, 3, gr[ph][0], gr[ph][1], gr[ph][2]);
        { int rr = ph ? 250 : 240, gg = ph ? 60 : 240; RS(cx - hw - hw / 8, y, hw / 8 + 1, 3, rr, gg, gg); RS(cx + hw, y, hw / 8 + 1, 3, rr, gg, gg); }
        RS(cx - hw, y, hw * 2, 3, rd[ph][0], rd[ph][1], rd[ph][2]);
        if (ph) { RS(cx - hw / 3, y, MAX(1, hw / 28), 3, 230, 230, 230); RS(cx + hw / 3, y, MAX(1, hw / 28), 3, 230, 230, 230); }
    }
    for (int pass = 0; pass < 6; pass++) { int best = -1, bz = -9999; for (int k = 0; k < 6; k++) if (rcar[k].on && rcar[k].z > bz && rcar[k].passed != 2) { best = k; bz = rcar[k].z; } if (best < 0) break; int k = best; rcar[k].passed = 2;
        int zz = MAX(0, rcar[k].z), r = 52 * 60 / (zz + 60); if (r < 3) continue; int w = 4 + r * 6 / 10, h = 3 + r * 4 / 10, x = rcen(r, rpx) + rcar[k].lane * 60 * rhalf(r) / 110, y = hz + r * 2 - h;
        static const u8 cc[4][3] = { { 240, 70, 70 }, { 240, 210, 60 }, { 90, 230, 120 }, { 200, 100, 240 } };
        R(x - w / 2, y, w, h, cc[rcar[k].col][0], cc[rcar[k].col][1], cc[rcar[k].col][2]); R(x - w / 2 + w / 6, y - h / 3, w * 2 / 3, h / 3 + 1, 200, 230, 255); R(x - w / 2, y + h - h / 4, w / 5 + 1, h / 4, 20, 20, 20); R(x + w / 2 - w / 5, y + h - h / 4, w / 5 + 1, h / 4, 20, 20, 20); R(x - w / 3, y + h / 2, w / 4 + 1, 1 + h / 6, 255, 255, 120); }
    for (int k = 0; k < 6; k++) if (rcar[k].passed == 2) rcar[k].passed = 0;
    { int sh = rcrash > 0 ? (rcrash & 2) * 2 - 2 : 0, x = 120 + sh; R(x - 16, 138, 32, 14, 250, 190, 40); R(x - 12, 132, 24, 8, 255, 230, 120); R(x - 10, 134, 20, 4, 180, 220, 250); R(x - 18, 148, 7, 8, 20, 20, 24); R(x + 11, 148, 7, 8, 20, 20, 24); R(x - 14, 140, 6, 2, 250, 60, 60); R(x + 8, 140, 6, 2, 250, 60, 60);
      if (rnos > 0) { R(x - 8, 154, 6, 6 + (ag_t & 3) * 2, 255, 200, 60); R(x + 2, 154, 6, 6 + (ag_t & 3) * 2, 255, 120, 40); } }
    { char s[20]; snprintf(s, sizeof s, "TIME %d", MAX(0, rclock) / 30); TXC(120, 2, s, rclock < 300 && (ag_t & 4) ? UI_RED : UI_RGB(170, 210, 250)); NUM(4, 2, "SCORE", ag_pts, UI_WHITE); snprintf(s, sizeof s, "N%d %d", rnitro, rsp); TX(190, 2, s, UI_AMBER); }
    { int w = rsp * 40 / 140; R(4, 150, 42, 6, 20, 20, 40); R(5, 151, w, 4, rsp > 100 ? 255 : 90, rsp > 100 ? 120 : 230, 60); }
    if (rcp_t > 0) TXB(120, 60, "GO GO GO", UI_AMBER);
}

// ================= MUNCH MAZE =================
#define ZW 15
#define ZH 9
static const char *const MZ[ZH] = { "###############", "#o.....#.....o#", "#.###.#.#.###.#", "#.............#", "#.##.#####.##.#", "#.............#", "#.###.#.#.###.#", "#o.....#.....o#", "###############" };
static u8 mdot[ZH][ZW]; static int mpx, mpy, mdx, mdy, mnd, mleft, mpow, mfruit, mfx, mdie, meat, mlv, mt;
static struct { s16 x, y; s8 dx, dy; u8 st, out; } mg[4];
static int mwall(int tx, int ty) { return tx < 0 || ty < 0 || tx >= ZW || ty >= ZH || MZ[ty][tx] == '#'; }
static void mreset(void) {
    mpx = 7 * 16; mpy = 5 * 16; mdx = -1; mdy = 0; mnd = -1; mpow = 0; mdie = 0; meat = 0;
    for (int i = 0; i < 4; i++) { mg[i].x = (s16)((5 + i) * 16); mg[i].y = 3 * 16; mg[i].dx = (s8)(i & 1 ? -1 : 1); mg[i].dy = 0; mg[i].st = 0; mg[i].out = (u8)(i * 90 - (mlv - 1) * 20); if (mg[i].out > 250) mg[i].out = 0; }
}
static void mfill(void) { mleft = 0; for (int y = 0; y < ZH; y++) for (int x = 0; x < ZW; x++) { char c = MZ[y][x]; mdot[y][x] = (u8)(c == '.' ? 1 : c == 'o' ? 2 : 0); if (c == '.' || c == 'o') mleft++; } mdot[5][7] = 0; mdot[3][7] = 0; mleft -= 2; mfruit = 0; }
void mze_start(void) { mlv = 1; ag_level = 1; ag_lives = 3; mt = 0; mfill(); mreset(); }
static void mgturn(int i) {   // a ghost at a tile centre picks a way
    int tx = mg[i].x / 16, ty = mg[i].y / 16, best = -1, bd2 = 99999; static const s8 DD[4][2] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };
    int gx = mpx / 16, gy = mpy / 16;
    if (i == 1) { gx += mdx * 3; gy += mdy * 3; } else if (i == 2) { gx = (ar8() & 1) ? 1 : 13; gy = (ar8() & 1) ? 1 : 7; if (ar8() & 1) { gx = mpx / 16; gy = mpy / 16; } } else if (i == 3 && ABS(gx - tx) + ABS(gy - ty) < 5) { gx = 0; gy = 8; }
    for (int d = 0; d < 4; d++) { if (DD[d][0] == -mg[i].dx && DD[d][1] == -mg[i].dy && (mg[i].dx || mg[i].dy)) continue; if (mwall(tx + DD[d][0], ty + DD[d][1])) continue;
        int sc = mg[i].st == 1 ? (int)ar8() : (tx + DD[d][0] - gx) * (tx + DD[d][0] - gx) + (ty + DD[d][1] - gy) * (ty + DD[d][1] - gy); if (sc < bd2) { bd2 = sc; best = d; } }
    if (best < 0) { mg[i].dx = (s8)-mg[i].dx; mg[i].dy = (s8)-mg[i].dy; } else { mg[i].dx = DD[best][0]; mg[i].dy = DD[best][1]; }
}
int mze_tick(int hd, int pr, int h2, int p2) {
    mt++; ag_level = mlv;
    if (mdie > 0) { if (--mdie == 0) { ag_lives--; if (ag_lives <= 0) return 0; mreset(); } return 1; }
    int nd = (hd & PAD_LEFT) ? 0 : (hd & PAD_RIGHT) ? 1 : (hd & PAD_UP) ? 2 : (hd & PAD_DOWN) ? 3 : -1; if (nd >= 0) { mnd = nd; if ((nd == 0 && mdx > 0) || (nd == 1 && mdx < 0) || (nd == 2 && mdy > 0) || (nd == 3 && mdy < 0)) { mdx = -mdx; mdy = -mdy; } }
    for (int s = 0; s < 2; s++) {   // the player moves 2 px a tick
        if (mpx % 16 == 0 && mpy % 16 == 0) {
            int tx = mpx / 16, ty = mpy / 16;
            if (mdot[ty][tx]) { int v = mdot[ty][tx]; mdot[ty][tx] = 0; mleft--; ag_pts += v == 2 ? 10 : 2; if (v == 2) { mpow = 210 - mlv * 20; meat = 0; for (int i = 0; i < 4; i++) if (mg[i].st == 0 && mg[i].out == 0) { mg[i].st = 1; mg[i].dx = (s8)-mg[i].dx; mg[i].dy = (s8)-mg[i].dy; } cue(CUE_GREAT); pop(mpx + 8, mpy + 10, "POWER", UI_AMBER); } else if (!(mleft & 3)) cue(CUE_GOOD); }
            if (mleft == 70 || mleft == 30) if (!mfruit) { mfruit = 300; mfx = 7; }
            if (mnd >= 0) { static const s8 DD[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } }; if (!mwall(tx + DD[mnd][0], ty + DD[mnd][1])) { mdx = DD[mnd][0]; mdy = DD[mnd][1]; } }
            if (mwall(tx + mdx, ty + mdy)) { mdx = mdy = 0; }
        }
        mpx += mdx * 2; mpy += mdy * 2;
    }
    if (mfruit > 0) { mfruit--; if (ABS(mpx - mfx * 16) < 10 && ABS(mpy - 5 * 16) < 10) { mfruit = 0; ag_pts += 100 * mlv; pop(mfx * 16 + 8, 80, "BONUS", UI_AMBER); cue(CUE_WIN); } }
    if (mpow > 0) { mpow--; if (mpow == 0) for (int i = 0; i < 4; i++) if (mg[i].st == 1) mg[i].st = 0; }
    for (int i = 0; i < 4; i++) {
        if (mg[i].out > 0) { mg[i].out--; continue; }
        int skip = mg[i].st == 1 ? 2 : (mlv >= 3 ? 8 : 4 - (mlv > 1)); if (mg[i].st == 2) skip = 0;
        if (skip && (mt % skip) == 0) continue;
        if (mg[i].x % 16 == 0 && mg[i].y % 16 == 0) { if (mg[i].st == 2) { int tx = mg[i].x / 16, ty = mg[i].y / 16; if (tx == 7 && ty == 3) mg[i].st = 0; } mgturn(i); }
        mg[i].x += mg[i].dx * 2; mg[i].y += mg[i].dy * 2;
        if (ABS(mg[i].x - mpx) < 11 && ABS(mg[i].y - mpy) < 11) {
            if (mg[i].st == 1) { mg[i].st = 2; meat++; int v = 100 << MIN(3, meat - 1); ag_pts += v; ag_combo++; ag_best = MAX(ag_best, ag_combo); char s[8]; snprintf(s, sizeof s, "+%d", v); pop(mg[i].x + 8, mg[i].y + 10, s, UI_WHITE); cue(CUE_GREAT); burst(mg[i].x + 8, mg[i].y + 22, 8, UI_RGB(120, 160, 255)); }
            else if (mg[i].st == 0) { mdie = 40; ag_combo = 0; ag_perf = 0; ag_shake = 8; cue(CUE_BAD); burst(mpx + 8, mpy + 22, 10, UI_RGB(250, 230, 40)); }
        }
    }
    if (mleft <= 0) { ag_pts += 200 * mlv; pop(120, 80, "MAZE CLEAR!", UI_AMBER); cue(CUE_WIN); mlv++; if (mlv > 3) return 0; mfill(); mreset(); }
    return 1;
}
void mze_draw(void) {
    R(0, 14, 240, 146, 6, 4, 24);
    for (int y = 0; y < ZH; y++) for (int x = 0; x < ZW; x++) { int X = x * 16, Y = 14 + y * 16;
        if (MZ[y][x] == '#') { R(X + 1, Y + 1, 14, 14, 30, 40, 200); R(X + 3, Y + 3, 10, 10, 8, 8, 60); if (!mwall(x, y - 1)) R(X + 1, Y + 1, 14, 2, 120, 140, 255); }
        else if (mdot[y][x] == 1) R(X + 7, Y + 7, 3, 3, 250, 220, 170); else if (mdot[y][x] == 2 && (mt & 4)) DSC(X + 8, Y + 8, 5, UI_RGB(255, 240, 200)); else if (mdot[y][x] == 2) DSC(X + 8, Y + 8, 4, UI_RGB(255, 240, 200)); }
    if (mfruit > 0 && (mfruit > 60 || (mt & 4))) { R(mfx * 16 + 4, 14 + 5 * 16 + 5, 9, 8, 240, 60, 70); R(mfx * 16 + 8, 14 + 5 * 16 + 2, 2, 4, 70, 190, 70); }
    for (int i = 0; i < 4; i++) { static const u8 gc[4][3] = { { 250, 60, 70 }, { 250, 150, 200 }, { 60, 230, 240 }, { 250, 170, 50 } }; int x = mg[i].x, y = 14 + mg[i].y; u8 r = gc[i][0], g = gc[i][1], b = gc[i][2];
        if (mg[i].st == 1) { r = (mpow < 60 && (mt & 4)) ? 255 : 40; g = (mpow < 60 && (mt & 4)) ? 255 : 60; b = (mpow < 60 && (mt & 4)) ? 255 : 230; }
        if (mg[i].st == 2) { R(x + 3, y + 5, 3, 3, 255, 255, 255); R(x + 10, y + 5, 3, 3, 255, 255, 255); continue; }
        R(x + 2, y + 2, 12, 12, r, g, b); R(x + 3, y + 1, 10, 2, r, g, b); R(x + 2, y + 14, 3, 2, r, g, b); R(x + 7, y + 14, 3, 2, r, g, b); R(x + 11, y + 14, 3, 2, r, g, b);
        if (mg[i].st == 0) { R(x + 4, y + 5, 3, 4, 255, 255, 255); R(x + 9, y + 5, 3, 4, 255, 255, 255); R(x + 5 + mg[i].dx, y + 7 + mg[i].dy, 2, 2, 20, 20, 120); R(x + 10 + mg[i].dx, y + 7 + mg[i].dy, 2, 2, 20, 20, 120); } else { R(x + 4, y + 5, 2, 2, 255, 255, 255); R(x + 10, y + 5, 2, 2, 255, 255, 255); R(x + 4, y + 11, 8, 1, 255, 255, 255); } }
    { int x = mpx, y = 14 + mpy; if (mdie > 0) { int w = MAX(1, mdie * 12 / 40); R(x + 8 - w / 2, y + 2, w, 12, 250, 230, 40); } else { int open = ((mt >> 2) & 1) && (mdx || mdy);
        DSC(x + 8, y + 8, 6, UI_RGB(250, 230, 40)); if (open) { if (mdx) R(x + (mdx > 0 ? 8 : 0), y + 6, 8, 4, 6, 4, 24); else R(x + 6, y + (mdy > 0 ? 8 : 0), 4, 8, 6, 4, 24); } R(x + 8 - (mdx < 0 ? 2 : -1), y + 3, 2, 2, 20, 20, 30); } }
    NUM(4, 2, "SCORE", ag_pts, UI_WHITE); hearts(ag_lives, 3); { char s[10]; snprintf(s, sizeof s, "MAZE %d", mlv); TXC(120, 2, s, UI_AMBER); }
}
