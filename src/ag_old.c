// ag_old.c - the four games BORE brought over from the GBA, made better: BORE BREAKER, SNACK CATCH, WHACK-A-BORE, HIGH STRIKER.
// Each is start / tick (30 Hz) / draw on a 240 x 160 field (arc_i.h). They score into ag_pts; arcade.c turns it into tickets.
#include "arc_i.h"

static int isq(int v) { int r = 0; while ((r + 1) * (r + 1) <= v) r++; return r; }
void hearts(int n, int max) { for (int i = 0; i < max; i++) { int x = 236 - (i + 1) * 9; R(x, 3, 7, 6, i < n ? 250 : 50, i < n ? 70 : 40, i < n ? 110 : 60); R(x + 1, 2, 2, 2, i < n ? 250 : 50, i < n ? 70 : 40, i < n ? 110 : 60); R(x + 4, 2, 2, 2, i < n ? 250 : 50, i < n ? 70 : 40, i < n ? 110 : 60); } }

// ================= BORE BREAKER =================
static u8 br[5][10]; static int bpx, bpw, bx, by, bvx, bvy, bheld, bleft, bwide, bslow, bbroken, brow0, bcol0;
static struct { s16 x, y; u8 k, on; } bpu[4];
static void bwall(void) {
    int lv = ag_level; bleft = 0;
    for (int r = 0; r < 5; r++) for (int c = 0; c < 10; c++) {
        int on = 1; if (lv == 2 && (r + c) % 5 == 0) on = 0; if (lv >= 3 && (c == 4 || c == 5) && r > 1) on = 0;
        br[r][c] = (u8)(on ? ((r == 0 || (lv >= 2 && r == 1) || (lv >= 3 && r < 3)) ? 2 : 1) : 0); bleft += on;
    }
}
static void bsetv(int off) {   // the ball leaves the paddle: off -64..64 across it
    int sp = 22 + ag_level * 4 + bbroken / 12; sp = MIN(sp, 50); if (bslow > 0) sp = sp * 3 / 4;
    bvx = sp * CLAMP(off, -56, 56) / 70; if (!bvx) bvx = (ar8() & 1) ? 4 : -4; bvy = -isq(sp * sp - bvx * bvx);
}
void brk_start(void) { ag_level = 1; bbroken = 0; bwall(); bpw = 36; bpx = 102; bheld = 1; bwide = bslow = 0; memset(bpu, 0, sizeof bpu); ag_lives = 3; }
int brk_tick(int hd, int pr, int h2, int p2) {
    if (hd & PAD_LEFT) bpx -= 6; if (hd & PAD_RIGHT) bpx += 6;
    bpw = bwide > 0 ? 58 : 36; if (bwide > 0) bwide--; if (bslow > 0) bslow--; bpx = CLAMP(bpx, 20, 220 - bpw);
    for (int i = 0; i < 4; i++) if (bpu[i].on) {
        bpu[i].y += 2;
        if (bpu[i].y >= 138 && bpu[i].y <= 148 && bpu[i].x + 12 >= bpx && bpu[i].x <= bpx + bpw) {
            bpu[i].on = 0; static const char *const nm[4] = { "WIDE!", "SLOW!", "LIFE!", "+50" }; pop(bpu[i].x + 6, 130, nm[bpu[i].k], UI_AMBER); cue(CUE_GREAT);
            if (bpu[i].k == 0) bwide = 360; else if (bpu[i].k == 1) bslow = 300; else if (bpu[i].k == 2) ag_lives = MIN(5, ag_lives + 1); else ag_pts += 50;
        } else if (bpu[i].y > 160) bpu[i].on = 0;
    }
    if (bheld) { bx = (bpx + bpw / 2) * 8; by = 136 * 8; if (pr & PAD_CROSS) { bheld = 0; bsetv(0); } return 1; }
    int n = MAX(1, (ABS(bvx) + ABS(bvy)) / 8 + 1);
    for (int s = 0; s < n; s++) {
        bx += bvx / n; by += bvy / n; int x = bx / 8, y = by / 8;
        if (x < 22) { bx = 22 * 8; bvx = ABS(bvx); } if (x > 218) { bx = 218 * 8; bvx = -ABS(bvx); } if (y < 14) { by = 14 * 8; bvy = ABS(bvy); }
        if (bvy > 0 && y >= 138 && y <= 146 && x + 2 >= bpx && x - 2 <= bpx + bpw) {
            bsetv((x - (bpx + bpw / 2)) * 64 / (bpw / 2 + 2)); by = 137 * 8; if (ag_combo >= 6) pop(x, 128, "NICE", UI_WHITE); ag_combo = 0; part(x, 140, 0, -20, 6, UI_RGB(150, 230, 250));
        } else if (y >= 164) {
            ag_lives--; ag_perf = 0; ag_combo = 0; bheld = 1; bwide = 0; cue(CUE_BAD); ag_shake = 8; if (ag_lives <= 0) return 0; return 1;
        }
        int c = (x - 20) / 20, r = (y - 22) / 9;
        if (x >= 20 && y >= 22 && r < 5 && c < 10 && br[r][c]) {
            if (c != bcol0 && r == brow0) bvx = -bvx; else bvy = -bvy;
            if (--br[r][c] == 0) {
                bleft--; bbroken++; ag_combo++; ag_best = MAX(ag_best, ag_combo); ag_pts += 2 + ag_combo / 4;
                { static const u8 rc[5][3] = { { 250, 60, 70 }, { 250, 150, 40 }, { 240, 230, 50 }, { 70, 220, 90 }, { 60, 170, 250 } }; burst(20 + c * 20 + 9, 22 + r * 9 + 4, 5, UI_RGB(rc[r][0], rc[r][1], rc[r][2])); }
                if (ar8() < 44) for (int i = 0; i < 4; i++) if (!bpu[i].on) { bpu[i].on = 1; bpu[i].x = (s16)(20 + c * 20 + 4); bpu[i].y = (s16)(22 + r * 9); bpu[i].k = (u8)(ar8() % 4); if (bpu[i].k == 2 && ag_lives >= 3) bpu[i].k = 3; break; }
                if (ag_combo == 8 || ag_combo == 16) { pop(x, y, ag_combo == 8 ? "COMBO 8!" : "COMBO 16!", UI_AMBER); cue(CUE_GREAT); }
            } else { part(x, y, 0, 0, 3, UI_WHITE); cue(CUE_GOOD); }
            if (!bleft) { ag_pts += 20 * ag_level; ag_level++; pop(120, 80, "WALL CLEARED!", UI_AMBER); cue(CUE_WIN); if (ag_level > 3) return 0; bwall(); bheld = 1; memset(bpu, 0, sizeof bpu); return 1; }
        }
        bcol0 = c; brow0 = r;
    }
    return 1;
}
void brk_draw(void) {
    static const u8 rc[5][3] = { { 250, 60, 70 }, { 250, 150, 40 }, { 240, 230, 50 }, { 70, 220, 90 }, { 60, 170, 250 } };
    NUM(4, 2, "SCORE", ag_pts, UI_WHITE); hearts(ag_lives, 5); { char s[16]; snprintf(s, sizeof s, "WALL %d", ag_level); TXC(120, 2, s, UI_AMBER); }
    for (int r = 0; r < 5; r++) for (int c = 0; c < 10; c++) if (br[r][c]) { int x = 20 + c * 20, y = 22 + r * 9, h = br[r][c] == 2;
        R(x, y, 19, 8, rc[r][0] / (h ? 1 : 2) + 20, rc[r][1] / (h ? 1 : 2) + 20, rc[r][2] / (h ? 1 : 2) + 20); R(x, y, 19, 2, 255, 255, 255); R(x, y + 7, 19, 1, 0, 0, 0); if (h) R(x + 1, y + 3, 17, 1, 255, 255, 255); }
    for (int i = 0; i < 4; i++) if (bpu[i].on) { static const u8 pc[4][3] = { { 60, 160, 255 }, { 80, 230, 120 }, { 255, 80, 130 }, { 255, 220, 60 } }; static const char *const L = "WS+$";
        R(bpu[i].x, bpu[i].y, 12, 8, pc[bpu[i].k][0], pc[bpu[i].k][1], pc[bpu[i].k][2]); char s[2] = { L[bpu[i].k], 0 }; TX(bpu[i].x + 3, bpu[i].y - 1, s, UI_WHITE); }
    R(bpx, 144, bpw, 6, 30, 200, 235); R(bpx, 144, bpw, 2, 255, 255, 255); R(bpx + 2, 150, bpw - 4, 1, 20, 90, 120);
    if (bwide > 0 && bwide < 60 && (bwide & 4)) R(bpx, 144, bpw, 6, 255, 255, 255);
    int x = bx / 8, y = by / 8; RA(x - 4, y - 4, 10, 10, UI_RGB(90, 130, 200)); R(x - 2, y - 2, 5, 5, 255, 255, 255);
    if (bslow > 0) TXC(200, 150, "SLOW", UI_RGB(80, 230, 120));
    if (ag_combo >= 3) { char s[16]; snprintf(s, sizeof s, "COMBO %d", ag_combo); TXC(120, 118, s, UI_AMBER); }
    if (bheld) TXC(120, 118, "X SERVES", UI_RGB(170, 210, 250));
}

// ================= SNACK CATCH =================
static struct { s16 x, y; u8 k, on, sw; } cit[8]; static int ckx, cstar, cmag, csq, cmiss;
void cat_start(void) { memset(cit, 0, sizeof cit); ckx = 106; cstar = 0; csq = 0; ag_lives = 3; cmiss = 0; }
int cat_tick(int hd, int pr, int h2, int p2) {
    int t = ag_t, sp = 5 + ag_level; if (hd & PAD_LEFT) ckx -= sp; if (hd & PAD_RIGHT) ckx += sp; ckx = CLAMP(ckx, 4, 208);
    ag_level = 1 + t / 360; if (cstar > 0) cstar--; if (csq > 0) csq--;
    int every = MAX(5, 14 - t / 120);
    if (t % every == 0) for (int i = 0; i < 8; i++) if (!cit[i].on) { int r = ar8(); cit[i].on = 1; cit[i].x = (s16)(8 + ar8() % 216); cit[i].y = 14; cit[i].sw = (u8)(ar8() & 7);
        cit[i].k = (u8)(r < 20 ? 4 : r < 30 ? 5 : r < 90 ? 3 : r < 130 ? 2 : r < 190 ? 1 : 0); break; }
    for (int i = 0; i < 8; i++) if (cit[i].on) {
        cit[i].y += 2 + ag_level + (cit[i].k == 3 ? 1 : 0);
        if (ag_level >= 3 && (cit[i].k < 3)) cit[i].x += (s16)(((cit[i].y / 6 + cit[i].sw) & 3) < 2 ? 1 : -1);
        int bw = cstar > 0 ? 40 : 28;
        if (cit[i].y >= 134 && cit[i].y <= 148 && cit[i].x + 8 >= ckx - (bw - 28) / 2 && cit[i].x <= ckx + bw - (bw - 28) / 2) {
            int k = cit[i].k; cit[i].on = 0;
            if (k == 3) { ag_lives--; ag_perf = 0; ag_combo = 0; cmiss++; ag_shake = 8; csq = 8; cue(CUE_BAD); burst(cit[i].x + 4, 140, 6, UI_RGB(150, 150, 160)); if (ag_lives <= 0) return 0; }
            else if (k == 4) { cstar = 270; pop(cit[i].x, 126, "DOUBLE!", UI_AMBER); cue(CUE_GREAT); }
            else if (k == 5) { ag_lives = MIN(5, ag_lives + 1); pop(cit[i].x, 126, "LIFE!", UI_RGB(255, 100, 140)); cue(CUE_GREAT); }
            else { int m = 1 + MIN(4, ag_combo / 5), v = (k + 1) * m * (cstar > 0 ? 2 : 1); ag_combo++; ag_best = MAX(ag_best, ag_combo); ag_pts += v; csq = 5;
                   char s[8]; snprintf(s, sizeof s, "+%d", v); pop(cit[i].x + 4, 128, s, UI_WHITE); burst(cit[i].x + 4, 140, 3, UI_RGB(250, 210, 60)); if (ag_combo % 10 == 0) cue(CUE_GOOD); }
        } else if (cit[i].y >= 160) { if (cit[i].k < 3) { if (ag_combo >= 5) pop(cit[i].x, 150, "MISS", UI_RED); ag_combo = 0; ag_perf = 0; } cit[i].on = 0; }
    }
    return ag_t < 1350;
}
void cat_draw(void) {
    NUM(4, 2, "SNACKS", ag_pts, UI_WHITE); hearts(ag_lives, 5);
    { int w = (1350 - ag_t) * 100 / 1350; R(70, 4, 100, 4, 40, 40, 70); R(70, 4, w, 4, w < 20 ? 250 : 90, w < 20 ? 70 : 220, w < 20 ? 70 : 250); }
    if (ag_combo >= 5) { char s[16]; snprintf(s, sizeof s, "x%d", 1 + MIN(4, ag_combo / 5)); TXC(120, 14, s, UI_AMBER); }
    for (int i = 0; i < 8; i++) if (cit[i].on) { int x = cit[i].x, y = cit[i].y;
        switch (cit[i].k) {
        case 0: DSC(x + 4, y + 4, 4, UI_RGB(240, 70, 80)); R(x + 4, y - 1, 1, 2, 80, 160, 60); break;                    // an apple
        case 1: DSC(x + 4, y + 4, 4, UI_RGB(220, 170, 90)); R(x + 2, y + 2, 1, 1, 90, 50, 20); R(x + 5, y + 4, 1, 1, 90, 50, 20); break;   // a cookie
        case 2: R(x, y + 1, 9, 7, 250, 190, 70); R(x + 2, y + 2, 2, 2, 220, 50, 50); R(x + 5, y + 4, 2, 2, 220, 50, 50); break;       // pizza
        case 3: R(x, y, 9, 8, 110, 110, 125); R(x + 1, y + 1, 4, 2, 160, 160, 175); break;                               // a rock
        case 4: R(x + 3, y, 3, 9, 255, 230, 70); R(x, y + 3, 9, 3, 255, 230, 70); R(x + 1, y + 1, 7, 7, 255, 245, 140); break;          // a star
        case 5: R(x, y + 1, 9, 6, 250, 80, 130); R(x + 1, y, 3, 2, 250, 80, 130); R(x + 5, y, 3, 2, 250, 80, 130); break;        // a heart
        } }
    { int bw = cstar > 0 ? 40 : 28, x = ckx - (bw - 28) / 2, sq = csq > 0 ? 2 : 0;
      R(x, 142 + sq, bw, 8 - sq, 170, 100, 30); R(x, 142 + sq, bw, 2, 250, 176, 64); R(x + 2, 148, bw - 4, 2, 110, 60, 20);
      R(x + 4, 136 + sq, 2, 6, 120, 70, 20); R(x + bw - 6, 136 + sq, 2, 6, 120, 70, 20);
      if (cstar > 0) { DSC(x + bw / 2, 139, 3, UI_RGB(255, 240, 120)); } }
    if (cstar > 0) TXC(120, 150, "DOUBLE POINTS", UI_AMBER);
}

// ================= WHACK-A-BORE =================
static struct { s8 k; s16 t; } wh[9]; static int wsel, whit, wgap, whit_at;
void wak_start(void) { memset(wh, 0, sizeof wh); wsel = 4; whit = 0; wgap = 10; ag_lives = 3; }
int wak_tick(int hd, int pr, int h2, int p2) {
    if ((pr & PAD_LEFT) && wsel % 3 > 0) wsel--; if ((pr & PAD_RIGHT) && wsel % 3 < 2) wsel++; if ((pr & PAD_UP) && wsel > 2) wsel -= 3; if ((pr & PAD_DOWN) && wsel < 6) wsel += 3;
    int t = ag_t, up = 0; for (int i = 0; i < 9; i++) if (wh[i].k) up++;
    int maxup = t > 600 ? 3 : t > 300 ? 2 : 1;
    if (--wgap <= 0 && up < maxup) { int h = ar8() % 9, r = ar8(); if (!wh[h].k) { wh[h].k = (s8)(r < 30 ? 3 : r < 50 ? 2 : 1); wh[h].t = (s16)MAX(9, 26 - t / 60 - (wh[h].k == 2 ? 6 : 0)); wgap = MAX(3, 12 - t / 150 + (ar8() & 3)); } }
    for (int i = 0; i < 9; i++) if (wh[i].k) { if (--wh[i].t <= 0) { if (wh[i].k < 3) { ag_combo = 0; ag_perf = 0; } wh[i].k = 0; } }
    if (whit > 0) whit--;
    if (pr & PAD_CROSS) {
        whit = 5; whit_at = wsel;
        if (wh[wsel].k) { int k = wh[wsel].k; int cx = 60 + (wsel % 3) * 60, cy = 44 + (wsel / 3) * 38;
            if (k == 3) { ag_lives--; ag_combo = 0; ag_perf = 0; ag_pts = MAX(0, ag_pts - 3); pop(cx, cy - 16, "BOOM!", UI_RED); ag_shake = 8; cue(CUE_BAD); burst(cx, cy, 8, UI_RGB(255, 140, 40)); if (ag_lives <= 0) return 0; }
            else { int v = (k == 2 ? 3 : 1) * (1 + MIN(3, ag_combo / 4)); ag_combo++; ag_best = MAX(ag_best, ag_combo); ag_pts += v; char s[8]; snprintf(s, sizeof s, "+%d", v); pop(cx, cy - 16, s, k == 2 ? UI_AMBER : UI_WHITE); burst(cx, cy - 6, 5, k == 2 ? UI_RGB(255, 220, 60) : UI_RGB(170, 110, 60)); cue(k == 2 ? CUE_GREAT : CUE_GOOD); }
            wh[wsel].k = 0; wgap = MAX(2, 6 - t / 300);
        } else { ag_combo = 0; }
    }
    return ag_t < 1200;
}
void wak_draw(void) {
    NUM(4, 2, "HITS", ag_pts, UI_WHITE); hearts(ag_lives, 3); { char s[16]; snprintf(s, sizeof s, "TIME %d", MAX(0, 1200 - ag_t) / 30); TXC(120, 2, s, UI_RGB(170, 210, 250)); }
    R(0, 18, 240, 142, 30, 90, 40); for (int i = 0; i < 12; i++) R(i * 20 + (i & 1) * 4, 20 + (i * 37) % 130, 6, 2, 40, 120, 50);
    for (int i = 0; i < 9; i++) { int cx = 60 + (i % 3) * 60, cy = 44 + (i / 3) * 38;
        DSC(cx, cy + 8, 17, UI_RGB(10, 30, 14)); DSC(cx, cy + 6, 16, UI_RGB(60, 36, 20)); DSC(cx, cy + 5, 13, UI_RGB(20, 12, 8));
        if (wh[i].k) { int k = wh[i].k, lift = MIN(wh[i].t, 6) * 2; u32 c = k == 2 ? UI_RGB(255, 210, 50) : k == 3 ? UI_RGB(70, 70, 90) : UI_RGB(170, 105, 55);
            DSC(cx, cy + 6 - lift / 2, 11, c); R(cx - 6, cy - 2 - lift / 2, 3, 4, 255, 255, 255); R(cx + 3, cy - 2 - lift / 2, 3, 4, 255, 255, 255); R(cx - 5, cy - lift / 2, 2, 2, 10, 10, 20); R(cx + 4, cy - lift / 2, 2, 2, 10, 10, 20);
            if (k == 3) { R(cx - 1, cy - 12 - lift / 2, 2, 5, 200, 200, 80); if (ag_t & 2) R(cx - 2, cy - 14 - lift / 2, 4, 3, 255, 120, 40); R(cx - 4, cy + 3 - lift / 2, 8, 2, 200, 40, 40); }
            else { R(cx - 3, cy + 3 - lift / 2, 7, 2, 40, 10, 10); if (k == 2) { R(cx - 6, cy - 8 - lift / 2, 12, 3, 255, 230, 80); R(cx - 6, cy - 11 - lift / 2, 2, 3, 255, 230, 80); R(cx + 4, cy - 11 - lift / 2, 2, 3, 255, 230, 80); } } }
        if (i == wsel) { int sw = whit > 0 && whit_at == i; R(cx - 20, cy - 18, 40, 1, 250, 210, 40); R(cx - 20, cy + 20, 40, 1, 250, 210, 40); R(cx - 20, cy - 18, 1, 39, 250, 210, 40); R(cx + 19, cy - 18, 1, 39, 250, 210, 40);
            R(cx + 6, cy - 22 + (sw ? 14 : 0), 5, 18, 150, 100, 50); R(cx + 2, cy - 28 + (sw ? 14 : 0), 16, 9, 220, 70, 70); R(cx + 2, cy - 28 + (sw ? 14 : 0), 16, 2, 255, 160, 160); } }
    if (ag_combo >= 4) { char s[16]; snprintf(s, sizeof s, "COMBO %d", ag_combo); TXC(120, 150, s, UI_AMBER); }
}

// ================= HIGH STRIKER =================
static int spos, sdir, sstop, sswing, sring, szone, sball;
void str_start(void) { spos = 0; sdir = 1; sstop = 0; sswing = 0; sring = 0; szone = 60; sball = 0; }
int str_tick(int hd, int pr, int h2, int p2) {
    if (sstop) {
        sstop--; if (sball < spos) sball = MIN(spos, sball + 6); if (sring > 0) sring--;
        if (sstop == 0) { if (++sswing >= 5) return 0; spos = 0; sdir = 1; sball = 0; szone = MAX(8, 60 - sswing * 12); }
        return 1;
    }
    if (pr & PAD_CROSS) {
        int sc = spos, bonus = 0; if (spos >= 97) { bonus = 25; sc += bonus; ag_combo++; ag_best = MAX(ag_best, ag_combo); sring = 24; cue(CUE_WIN); burst(120, 22, 12, UI_RGB(255, 230, 80)); pop(120, 40, "DING!", UI_AMBER); }
        else if (spos >= 85) { sc += 5; ag_combo++; ag_best = MAX(ag_best, ag_combo); cue(CUE_GOOD); pop(120, 60, "GREAT", UI_WHITE); } else { ag_combo = 0; ag_perf = 0; cue(CUE_BAD); pop(120, 100, spos < 40 ? "WEAK!" : "NOT BAD", UI_RGB(200, 200, 220)); }
        ag_pts += sc + (ag_combo >= 2 ? ag_combo * 5 : 0); sstop = 40; sball = 0; return 1;
    }
    spos += sdir * (3 + sswing * 2 + (ag_t / 90 > 4 ? 0 : 0)); if (spos >= 100) { spos = 100; sdir = -1; } if (spos <= 0) { spos = 0; sdir = 1; }
    return 1;
}
void str_draw(void) {
    NUM(4, 2, "POINTS", ag_pts, UI_WHITE); { char s[16]; snprintf(s, sizeof s, "SWING %d OF 5", MIN(5, sswing + 1)); TX(150, 2, s, UI_AMBER); }
    R(0, 20, 240, 140, 40, 24, 70); for (int i = 0; i < 10; i++) R(10 + i * 24, 140 + (i & 1) * 4, 14, 20, 60, 36, 100);   // the fair's stripes
    R(100, 22, 40, 124, 60, 60, 100); R(103, 24, 34, 120, 14, 14, 34);
    for (int i = 0; i < 5; i++) { static const char *const L[5] = { "EASY", "FAIR", "GOOD", "GREAT", "BELL" }; TX(142, 128 - i * 26, L[i], i == 4 ? UI_AMBER : UI_RGB(170, 170, 210)); R(138, 138 - i * 28, 3, 1, 200, 200, 230); }
    { int h = (sstop ? sball : spos) * 114 / 100; u32 c = (sstop ? spos : spos) >= 97 ? UI_RGB(255, 230, 50) : spos >= 85 ? UI_RGB(255, 140, 40) : spos >= 60 ? UI_RGB(240, 200, 50) : UI_RGB(60, 210, 90);
      RC(104, 142 - h, 32, h, c); R(104, 142 - h, 32, 2, 255, 255, 255); if (!sstop) { R(98, 142 - h, 44, 2, 255, 255, 255); } }
    R(120 - 12, 17, 24, 6, 250, 210, 50); DSC(120, 17, 7, sring > 0 && (sring & 2) ? UI_RGB(255, 255, 255) : UI_RGB(250, 190, 40));
    { int sw = sstop ? 1 : 0; R(40, 120, 8, 24, 220, 120, 80); R(38, 102, 12, 18, 220, 60, 60); R(36, 92, 16, 12, 230, 170, 120);   // the strongman and his hammer
      R(52, sw ? 124 : 100, 22, 4, 120, 80, 40); R(sw ? 72 : 66, sw ? 118 : 90, 12, 16, 150, 150, 160); }
    if (ag_combo >= 2) { char s[16]; snprintf(s, sizeof s, "STREAK %d", ag_combo); TXC(190, 150, s, UI_AMBER); }
    if (!sstop) TXC(190, 90, "X STOPS IT", UI_RGB(170, 210, 250));
}
