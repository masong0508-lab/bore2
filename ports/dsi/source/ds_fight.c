// ds_fight.c (D54): BORTUA FIGHTER on the DS. The rules are the PS1's src/fight.c line for line (ds_fight_logic.h, made by tools/dsbuild/mk_dsfight.py);
// this file is the DS's own picture and shell: the ring and the sea in 3D on the touch screen, the fighters as the DS's voxel Bores with every joint posed
// (shoulders, elbows, hips, knees, the upper body's twist and lean, the head's nod), the life bars and menus on the top screen, the buttons.
// Included by ds_games.c (it is the arcade's fifth cabinet).
typedef struct { s16 arm[2], aroll[2], elbow[2], leg[2], lroll[2], knee[2], lean, twist, nod, tilt, drop, pad; } VPose;   // (the PS1's voxel.h VPose)
static inline int isqrt(int v) { return g_isq(v); }
#define PAD_UP 1
#define PAD_DOWN 2
#define PAD_LEFT 4
#define PAD_RIGHT 8
#define PAD_CROSS 16
#define PAD_CIRCLE 32
#define PAD_TRIANGLE 64
#define PAD_SQUARE 128
#define PAD_L1 256
#define PAD_R1 512
#define PAD_START 1024
#define PAD_SELECT 2048
typedef struct { u16 sk, hc, sh, pants; u8 hair, beard; int P[PMN]; } FLook;   // how a fighter looks: the DS Bore's colours, hair and body sliders
static FLook flk[2];
#define FB_L 1   // (2 players on one DS: a player's buttons as flags)
#define FB_R 2
#define FB_D 4
#define FB_J 8
#define FB_A1 16
#define FB_A2 32
static int opp_i = -1, fg_h1, fg_p1, fg_h2, fg_p2;   // who plays the other side (a Bore of the household; -1: the CPU's ladder), and the two players' buttons this frame
#define fight_2p (opp_i >= 0)
static void opp_step(int dir);
static void build_them(int st);
#include "ds_fight_logic.h"
static void flook_of(FLook *o, int si) { const Sim *m = &sims[si]; eff_pm(m->pm, o->P); o->sk = skin_of(m); o->hc = hair_of(m); o->sh = shirt_of(m); o->pants = C15(6, 7, 12); o->hair = m->hair; o->beard = m->beard; }
static void build_them(int st) {   // the opponent: a Bore of the household in a 2 player match, else the PS1's ladder, each a Bore from their own dice, dressed in their colours; BORAL is chrome all over
    if (fight_2p) { flook_of(&flk[1], opp_i); return; }
    const Who *w = &ladder[st]; FLook *o = &flk[1]; unsigned h = w->seed * 2654435761u + 7;
    for (int q = 0; q < PMN; q++) { h = h * 1103515245u + 12345u; o->P[q] = 50 + (int)((h >> 16) % 31) - 15; }
    o->sk = SKINC[(h >> 8) % 6]; o->hair = (u8)((w->seed * 5) % NHAIRD); o->beard = (u8)(w->seed % 3 == 0 ? 1 : 0);
    o->sh = w->top; o->pants = w->bot; o->hc = w->hair;
    if (st == 8) { o->sk = C15(24, 25, 28); o->hc = C15(19, 20, 24); o->sh = C15(26, 26, 30); o->pants = C15(15, 16, 20); o->hair = 5; o->beard = 0; }
}
static int fg_on(void) { return mg_ui == 6; }
// ---- the world, in the DS's units: a Bore is about 900 tall, a cell is 660; the ground is drawn twice as far as height (a voxel Bore is twice as broad as a person) ----
#define FXG(v) ((int)(v))
#define FYG(v) ((int)(v) / 2)
static void fight_cam(int *dist, int *pitch, int *yaw, int *lx, int *ly, int *lz) {
    int lookh = cam_mode == CAM_HIGH ? 500 : 950, dh = FYG(cheight - lookh), hd = FXG(cdist), sx = shake ? ((rnd8() & 7) - 4) * shake * 6 : 0, sy = shake ? ((rnd8() & 7) - 4) * shake * 6 : 0;
    *dist = bsqrt(hd * hd + dh * dh); if (*dist < 400) *dist = 400;
    *pitch = bwrap(bang(dh, hd)) * 128; *yaw = cyaw * 128;
    *lx = FXG(cax) + sx; *ly = FYG(lookh) + sy; *lz = -FXG(caz);   // (the DS's y is down)
}
static void fqf(int x0, int z0, int x1, int z1, int x2, int z2, int x3, int z3, int y, u16 c) { qd(x0, y, z0, x1, y, z1, x2, y, z2, x3, y, z3, c); }   // a flat quad at the same depth
static void ring_draw3(void) {
    const int R = FXG(BZ_R), R2 = FXG(BZ_R + 170), Rin = FXG(BZ_R * 55 / 100);
    for (int a = 0; a < 32; a++) { int a0 = a * 128, a1 = a0 + 128; qd(0, 24, 0, isin(a1) * R2 >> 12, 24, icos(a1) * R2 >> 12, isin(a0) * R2 >> 12, 24, icos(a0) * R2 >> 12, 0, 24, 0, C15(5, 7, 18)); }   // (under the mat: the seams between its pieces show this, not the sky)
    for (int a = 0; a < 32; a++) {   // the mat (a painted circle in the middle), the gold rim, in sectors
        int a0 = a * 128, a1 = a0 + 128, s0 = isin(a0), c0 = icos(a0), s1 = isin(a1), c1 = icos(a1), rs[4] = { 0, Rin, R, R2 };
        for (int r = 0; r < 3; r++) {
            u16 c = r == 0 ? ((a & 1) ? C15(8, 11, 24) : C15(7, 10, 23)) : r == 1 ? ((a & 1) ? C15(5, 7, 18) : C15(4, 7, 17)) : ((a & 1) ? C15(28, 22, 7) : C15(25, 19, 5));
            fqf(s0 * rs[r] >> 12, c0 * rs[r] >> 12, s1 * rs[r] >> 12, c1 * rs[r] >> 12, s1 * rs[r + 1] >> 12, c1 * rs[r + 1] >> 12, s0 * rs[r + 1] >> 12, c0 * rs[r + 1] >> 12, 0, c);
        }
        int dn = FYG(1700);   // the side of the platform, down to the sea
        qd(s0 * R2 >> 12, 0, c0 * R2 >> 12, s1 * R2 >> 12, 0, c1 * R2 >> 12, s1 * R2 >> 12, dn, c1 * R2 >> 12, s0 * R2 >> 12, dn, c0 * R2 >> 12, a & 1 ? C15(15, 10, 4) : C15(13, 9, 3));
    }
    for (int gz = -4; gz < 4; gz++) for (int gx = -4; gx < 4; gx++) {   // the sea: a grid of swells, darker away from the sunset
        int x0 = gx * 1200, z0 = gz * 1200; if (ABS(x0 + 600) < 1500 && ABS(z0 + 600) < 1500) continue;
        int k = (isin((fframe * 40 + gx * 900 + gz * 1300) & 4095) >> 10) + 4; u16 c0 = C15(2 + k / 2, 5 + k / 2, 13 + k), c1 = C15(3 + k / 2, 6 + k / 2, 15 + k);
        ground(x0, z0, x0 + 1200, z0 + 1200, FYG(1700), ((gx + gz) & 1) ? c0 : c1);
    }
    for (int k = 0; k < 4; k++) {   // four torches round the edge, flickering
        int a = k * 1024 + 512, x = isin(a) * (R2 + 40) >> 12, z = icos(a) * (R2 + 40) >> 12, fl = isin((fframe * 300 + k * 1000) & 4095) >> 11;
        bx(x - 14, 0, z - 14, x + 14, 300, z + 14, C15(8, 5, 3)); bx(x - 26, 300, z - 26, x + 26, 380 + fl * 2, z + 26, fl > 0 ? C15(31, 20, 4) : C15(31, 15, 3)); bx(x - 12, 380 + fl * 2, z - 12, x + 12, 440 + fl * 4, z + 12, C15(31, 28, 10));
    }
}
static void jrot(int px, int pu, int pz, int swing, int roll) {   // turn about a joint (4096 to the turn): the limb hangs from it
    glTranslatef32(px, pu, pz); if (roll) glRotateZi(roll * 8); if (swing) glRotateXi(swing * 8); glTranslatef32(-px, -pu, -pz); }
static void fighter3(int i) {
    Fz *f = &F[i]; const FLook *k = &flk[i]; VPose p; pose_of(f, &p); const int *P = k->P;
    int hw = 100 + (P[8] - 50) / 2, ht = 200 + (P[9] - 50) / 2, lh = 320 + (P[22] - 50) * 2 + (P[26] - 50) * 3, base = 630 + (P[15] - 50) * 2 + (P[22] - 50) * 2 + (P[26] - 50) * 5,
        sw = 135 + (P[14] - 50), at = 50 + (P[21] - 50) / 2, lt = 90 + (P[23] - 50) / 2, bz = 85 + (P[24] - 50) * 2, al = 250 + (P[20] - 50) * 2, hb = base + 30, uk = lh * 45 / 100;
    u16 sk = k->sk, hc = k->hc, sh = k->sh, pants = k->pants, skd = shade(sk, 7);
    int lying = f->st == BS_KO || f->st == BS_THROWN, lie = lying ? (f->st == BS_KO ? MAX(f->lie, 600) : 2400) : 0;
    sb_sc = 100; sq_q = 0;
    if (!f->fall && f->y < 1800) { int sx = FXG(f->x), sz = -FXG(f->z), r = 150 - FYG(f->y) / 6; ground(sx - r, sz - r, sx + r, sz + r, -4, C15(2, 3, 9)); }   // (the shadow on the mat)
    glPushMatrix(); glTranslatef32(FXG(f->x), FYG(f->y), -FXG(f->z)); glRotateYi(16384 - (((f->yaw + f->spin) & 255) << 7));
    if (lying) { glTranslatef32(0, 95 * lie / 4096, 0); glRotateXi(-(lie * 1024 / 4096) * 8); }
    else { int lo = 0, thigh = lh - uk, shin = uk;   // the hips sink as the knees bend, so the lower foot stays on the ground
        for (int s = 0; s < 2; s++) { int lg = p.leg[s], kn = p.knee[s], lr = p.lroll[s], fy = ((thigh * icos(lg) >> 12) + (shin * icos(lg + kn) >> 12)) * icos(lr) >> 12; lo = MAX(lo, fy); }
        glTranslatef32(0, lo - lh, 0); if (p.lean) glRotateXi(p.lean * 8); }
    for (int sd = -1; sd <= 1; sd += 2) { int s = sd > 0, lx0 = sd > 0 ? 8 : -8 - lt, lx1 = sd > 0 ? 8 + lt : -8, hx = (lx0 + lx1) / 2;   // the legs: thigh from the hip, shin and shoe from the knee
        glPushMatrix(); jrot(hx, lh, 0, p.leg[s], p.lroll[s]);
        sb(0, 0, lx0, uk, -50, lx1, lh, 60, pants);
        glPushMatrix(); jrot(hx, uk, 0, p.knee[s], 0); sb(0, 0, lx0, 0, -60, lx1, 55, 110, C15(5, 4, 4)); sb(0, 0, lx0, 55, -50, lx1, uk, 60, pants); glPopMatrix(1);
        glPopMatrix(1); }
    sb(0, 0, -sw - 4, lh - 10, -89, sw + 4, lh + 40, bz + 4, C15(6, 4, 2)); sb(0, 0, -18, lh - 6, bz + 4, 18, lh + 34, bz + 9, C15(30, 26, 8));
    glPushMatrix(); if (p.twist) { glRotateYi(-p.twist * 8); }   // the upper body turns on the hips
    sb(0, 0, -sw, lh + 40, -85, sw, base, bz, sh); sb(0, 0, -sw - 6, base - 70, -90, sw + 6, base, bz + 6, shade(sh, 9)); sb(0, 0, -42, base - 55, bz + 2, 42, base + 2, bz + 8, skd);
    for (int sd = -1; sd <= 1; sd += 2) { int s = sd > 0, ax0 = sd > 0 ? sw : -sw - at, ax1 = sd > 0 ? sw + at : -sw, xc = (ax0 + ax1) / 2, el = base - al * 45 / 100;   // the arms: sleeve from the shoulder, forearm and hand from the elbow
        glPushMatrix(); jrot(xc, base - 10, 0, p.arm[s], p.aroll[s]);
        sb(0, 0, ax0, el, -at, ax1, base - 10, at, sh);
        glPushMatrix(); jrot(xc, el, 0, p.elbow[s], 0); sb(0, 0, ax0 + 4, base - al, -at + 4, ax1 - 4, el, at - 4, sk); sb(0, 0, ax0 + 2, base - al - 45, -at + 2, ax1 - 2, base - al, at + 4, sk); glPopMatrix(1);
        glPopMatrix(1); }
    glPushMatrix(); glTranslatef32(0, base, 0); if (p.tilt) glRotateZi(p.tilt * 8); if (p.nod) glRotateXi(p.nod * 8); glTranslatef32(0, -base, 0);   // the neck: the head nods and tilts
    sb(0, 0, -40, base, -40, 40, base + 40, 40, skd);
    sb(0, 0, -hw, hb, -hw, hw, hb + ht, hw, sk); sb(0, 0, -hw - 16, hb + ht * 35 / 100, -22, -hw, hb + ht * 65 / 100, 40, skd); sb(0, 0, hw, hb + ht * 35 / 100, -22, hw + 16, hb + ht * 65 / 100, 40, skd);
    hair_pieces(k->hair, 4, 0, 0, sq_q, sb_sc, hw, hw, hb + ht, ht, hc);   // (D62) the PS1's hair pieces
    { int ex = 40 + (P[2] - 50) / 2, ey = hb + ht * 55 / 100, es = 28 + (P[16] - 50) / 4, ff = hw;   // the face
      for (int sd = -1; sd <= 1; sd += 2) { int c0 = sd * ex;
        sb(0, 0, c0 - es - 4, ey - 24, ff, c0 + es + 4, ey + 26, ff + 6, C15(31, 31, 31)); sb(0, 0, c0 - 14, ey - 22, ff + 4, c0 + 14, ey + 24, ff + 10, C15(7, 5, 3)); sb(0, 0, c0 - 6, ey - 12, ff + 8, c0 + 6, ey + 12, ff + 12, C15(1, 1, 2));
        sb(0, 0, c0 - es - 6, ey + 34 + (P[7] - 50) / 3, ff, c0 + es + 6, ey + 48 + (P[7] - 50) / 3, ff + 7, shade(hc, 7)); }
      sb(0, 0, -16 - (P[4] - 50) / 4, ey - 70, ff, 16 + (P[4] - 50) / 4, ey - 6, ff + 28 + (P[5] - 50) / 4, shade(sk, 8));
      { int mw = 36 + (P[0] - 50) / 2, my = ey - 100 + (50 - P[1]) / 3; int open = f->st == BS_HIT || f->st == BS_ATK || f->st == BS_KO || f->st == BS_THROWN;
        sb(0, 0, -mw, my, ff, mw, my + (open ? 40 : 16) + (P[17] - 50) / 6, ff + 6, open ? C15(12, 2, 3) : C15(21, 7, 8)); if (!open) sb(0, 0, -mw + 6, my + 15, ff, mw - 6, my + 22, ff + 5, C15(26, 12, 12)); }
      if (k->beard) sb(0, 0, -hw + 8, hb, ff - 4, hw - 8, ey - 60, ff + 8, hc); }
    glPopMatrix(1);   // the neck
    glPopMatrix(1);   // the upper body
    glPopMatrix(1);   // the fighter
    if (f->flash > 0 && (f->flash & 1)) { /* a blow's flash: the body flickers white */ }
}
static void sparks3(void) {
    for (int k = 0; k < 4; k++) { if (spark[k].t <= 0) continue;
        int x = FXG(spark[k].wx), y = FYG(spark[k].wy), z = -FXG(spark[k].wz), r = (14 - spark[k].t) * 9 + 40; u16 c = spark[k].blk ? C15(10, 22, 31) : C15(31, 26, 6), c2 = spark[k].blk ? C15(24, 29, 31) : C15(31, 31, 24);
        bx(x - r, y - r / 3, z - r, x + r, y + r / 3, z + r, c); bx(x - r / 3, y - r, z - r / 3, x + r / 3, y + r, z + r / 3, c2); bx(x - r, y - r / 5, z - r / 5, x + r, y + r / 5, z + r / 5, c2);
    }
}
static void fight_3d(void) {   // inside the lot's direct-to-GPU pass (glBegin GL_QUADS is on)
    flook_of(&flk[0], you_i); if (fight_2p) flook_of(&flk[1], opp_i); ring_draw3();
    if (fs == FS_CHAMP) { F[0].x = 0; F[0].z = 0; }
    fighter3(0); if (fs != FS_CHAMP) fighter3(1); sparks3();
}
// ---- the screens: the top has the life bars and the words, the touch screen is the ring (its words and buttons are drawn over it) ----
static void fg_bar(int x, int w, int hp, int trail, int right) {   // a life bar: the red trail, the yellow life
    rect(x - 1, 21, w + 2, 10, C15(2, 2, 5)); int tw_ = w * trail / 100, hw_ = w * hp / 100;
    if (right) { rect(x + w - tw_, 22, tw_, 8, C15(27, 5, 4)); rect(x + w - hw_, 22, hw_, 8, C15(31, 27, 5)); } else { rect(x, 22, tw_, 8, C15(27, 5, 4)); rect(x, 22, hw_, 8, C15(31, 27, 5)); }
}
static void fg_big(int y, int s, u16 c, const char *t) {   // (D65) the big words in the TITLE face (HEAD for 1), fitted and centred, with a drop shadow
    int f = fit_font(s >= 2 ? FS_TITLE : FS_HEAD, 244, t); if (!f) f = FS_SMALL; int x = 128 - tw(f, t) / 2; text(x + 2, y + 2, f, C15(4, 4, 6), t); text(x, y, f, c, t); }
static void fg_hud(void) {
    fg_bar(6, 108, F[0].hp, F[0].trail, 0); fg_bar(142, 108, F[1].hp, F[1].trail, 1);
    text_fit(6, 8, 2, 108, TXT, sname[you_i]); { const char *on = fight_2p ? sname[opp_i] : ladder[stage].nm; int f = fit_font(FS_HEAD, 108, on); if (!f) f = FS_SMALL; text(250 - tw(f, on), 8, f, TXT, on); }
    for (int k = 0; k < 2; k++) { rect(8 + k * 12, 34, 8, 8, k < F[0].wins ? C15(31, 22, 3) : C15(8, 8, 12)); rect(240 - k * 12, 34, 8, 8, k < F[1].wins ? C15(31, 22, 3) : C15(8, 8, 12)); }
    char t[8]; snprintf(t, sizeof t, "%d", secs); rect(112, 4, 32, 30, C15(3, 3, 8)); text_box(112, 4, 32, 30, FS_TITLE, secs <= 5 ? C15(31, 10, 8) : C15(31, 27, 5), AL_C, t);
    { char s[24]; snprintf(s, sizeof s, fight_2p ? "TWO PLAYERS" : stage == 8 ? "FINAL STAGE" : "STAGE %d OF 9", stage + 1); text(128 - tw(1, s) / 2, 46, 1, C15(24, 26, 31), s); }
}
static void fight_top(void) {
    scr = offbuf; rect(0, 0, 256, 192, K8(26, 16, 54));

    switch (fs) {
    case FS_PICK: {
        panel(4, 4, 248, 184, "BORTUA FIGHTER"); static const char *const lab[4] = { "FIGHTER", "STYLE", "CAMERA", "OPPONENT" };
        for (int r = 0; r < 4; r++) { int y = 24 + r * 27, sel = r == pick_row; rect(12, y, 232, 25, sel ? C15(31, 31, 20) : PANB); rect(13, y + 1, 230, 23, sel ? C15(24, 28, 31) : PAN);
            text_box(13, y + 1, 200, 10, FS_SMALL, sel ? C15(2, 7, 20) : C15(20, 24, 28), AL_L, lab[r]); static char ob[40]; if (r == 3) snprintf(ob, sizeof ob, fight_2p ? "PLAYER 2: %s" : "THE CPU LADDER", fight_2p ? sname[opp_i] : "");
            const char *v = r == 0 ? sname[you_i] : r == 1 ? styleNm[style] : r == 2 ? camNm[fight_2p ? CAM_SIDE : cam_mode] : ob; text_box(13, y + 11, 190, 13, FS_HEAD, sel ? C15(2, 7, 20) : TXT, AL_L, v);
            if (sel) { text_box(204, y + 11, 18, 13, FS_HEAD, C15(4, 8, 24), AL_C, "<"); text_box(224, y + 11, 18, 13, FS_HEAD, C15(4, 8, 24), AL_C, ">"); } }
        text_fit(12, 136, 1, 232, C15(31, 25, 6), fight_2p ? "ONE DS, TWO PLAYERS. BEST OF THREE" : styleTx[style]); { char b[40]; snprintf(b, sizeof b, "POWER %d   BEST STAGE %d", F[0].pw, fight_best); if (!fight_2p) text(12, 152, 1, TXT, b); }
        text(12, 170, 1, C15(20, 24, 28), "A FIGHT   B WALK AWAY"); break; }
    case FS_VS: { panel(4, 4, 248, 184, fight_2p ? "TWO PLAYERS" : stage == 8 ? "FINAL STAGE" : "NEXT OPPONENT"); char b[24]; snprintf(b, sizeof b, "STAGE %d OF 9", stage + 1); if (!fight_2p) ctext(128, 28, 1, C15(31, 25, 6), b);
        ctext(128, 56, 5, C15(31, 10, 8), "VS"); text_fit(12, 92, 1, 232, TXT, sname[you_i]); ctext(128, 112, 1, C15(20, 24, 28), "against"); fg_big(128, 1, C15(31, 31, 22), fight_2p ? sname[opp_i] : ladder[stage].nm);
        if (fight_2p) { text(12, 142, 1, C15(24, 26, 31), "P1: D-PAD  L PUNCH  R KICK"); text(12, 154, 1, C15(24, 26, 31), "P2: Y A X B  TOUCH PUNCH KICK"); text(12, 166, 1, C15(24, 26, 31), "BACK: GUARD  L+R OR SELECT: THROW"); }
        else { char q[40]; snprintf(q, sizeof q, "\"%s\"", ladder[stage].intro); text_fit(12, 150, 1, 232, C15(24, 26, 31), q); } text(12, 178, 1, C15(20, 24, 28), "A FIGHT   B WALK AWAY"); break; }
    case FS_RESULT: { int won = F[0].wins >= 2; fg_hud(); fg_big(80, 2, won || fight_2p ? C15(31, 27, 8) : C15(25, 10, 8), fight_2p ? (won ? "PLAYER 1 WINS" : "PLAYER 2 WINS") : won ? "YOU WIN!" : "YOU LOSE");
        text(12, 130, 1, TXT, fight_2p ? "A: REMATCH   B: WALK AWAY" : won ? (stage == 8 ? "A: SEE WHAT YOU WON   B: WALK AWAY" : "A: NEXT STAGE   B: WALK AWAY") : funds >= 2 ? "A: CONTINUE $2   B: WALK AWAY" : "NO MONEY LEFT TO PLAY ON   B: WALK AWAY"); break; }
    case FS_CHAMP: ctext(128, 40, 2, C15(31, 26, 6), "BORTUA"); ctext(128, 66, 2, C15(31, 26, 6), "CHAMPION"); ctext(128, 110, 1, TXT, "NOBODY IS BORED OF YOU NOW"); ctext(128, 140, 1, C15(20, 24, 28), "A: WALK AWAY A LEGEND"); break;
    case FS_PAUSE: fg_hud(); fg_big(70, 2, C15(31, 31, 31), "PAUSED"); ctext(128, 116, 1, TXT, "START: FIGHT ON   SELECT: QUIT"); break;
    default: fg_hud();
        if (fs == FS_INTRO && ft < 100) fg_big(80, 2, C15(31, 28, 14), banner ? banner : "");
        if (fs == FS_FIGHT && callT && call) fg_big(80, 2, C15(31, 29, 17), call);
        if (fs == FS_END) { if (ft > 30) fg_big(70, 2, C15(31, 11, 8), why ? why : ""); if (ft > 90) fg_big(112, 1, TXT, winner < 0 ? "A DRAW" : fight_2p ? (winner == 0 ? "PLAYER 1 TAKES THE ROUND" : "PLAYER 2 TAKES THE ROUND") : winner == 0 ? "YOU WIN THE ROUND" : "THEY TAKE THE ROUND"); }
        if (fight_2p) { text(8, 164, 1, C15(20, 24, 28), "P1 D-PAD L PUNCH R KICK  UP JUMP"); text(8, 176, 1, C15(20, 24, 28), "P2 Y A X B  TOUCH PUNCH KICK"); }
        else { text(8, 164, 1, C15(20, 24, 28), "A PUNCH  B KICK  X JUMP  Y GUARD"); text(8, 176, 1, C15(20, 24, 28), "SELECT CAMERA  START PAUSE"); } break;
    }
}
static void fight_bottom(void) {   // over the 3D ring (the screen starts clear): the words and the buttons
    switch (fs) {
    case FS_PICK: case FS_VS: chip(150, 168, 50, 20, "Fight", 1); chip(204, 168, 48, 20, "Leave", 0); break;
    case FS_RESULT: chip(150, 168, 50, 20, "Next", 1); chip(204, 168, 48, 20, "Leave", 0); break;
    case FS_CHAMP: chip(204, 168, 48, 20, "Leave", 1); break;
    case FS_PAUSE: chip(150, 168, 50, 20, "On", 1); chip(204, 168, 48, 20, "Quit", 0); break;
    case FS_FIGHT: if (fight_2p) { chip(200, 4, 52, 18, "Pause", 0); chip(142, 144, 56, 42, "Punch", 0); chip(200, 144, 52, 42, "Kick", 0); } else chip(204, 168, 48, 20, "Pause", 0); if (callT && call) fg_big(70, 2, C15(31, 29, 17), call); break;
    case FS_INTRO: if (ft < 100) fg_big(70, 2, C15(31, 28, 14), banner ? banner : ""); break;
    case FS_END: if (ft > 30) fg_big(60, 2, C15(31, 11, 8), why ? why : ""); break;
    default: break; }
}
static unsigned fight_bsig(void) { return (unsigned)(fs * 7 + (fight_2p ? 211 : 0) + (callT > 0) * 13 + (call ? (int)(((u32)(size_t)call >> 2) & 1023) * 5 : 0) + (fs == FS_END ? (ft > 30) + (ft > 90) * 2 : 0) + (fs == FS_INTRO ? ft < 100 : 0) + (fs == FS_RESULT ? (funds >= 2) * 3 : 0)); }
static void fight_finish(void) {   // away from the cabinet: the tickets for the matches won (the PS1's arcade_update)
    if (fight_matches && fight_2p) { Sim *s = &sims[ctl]; need_add(s, 1, 30); need_add(s, 2, -6); say("Bortua Fighter: a friendly match"); life_ev(ctl, WN_FUN, 0); }
    else if (fight_matches) { Sim *s = &sims[ctl]; int w = fight_last_wins, tix = w * 6; static char b[48];
        arcade_tix = MIN(9999, arcade_tix + tix); need_add(s, 1, 30 + w * 5); need_add(s, 2, -6);
        snprintf(b, sizeof b, w ? "Bortua Fighter: %d won, +%d tickets" : "Bortua Fighter: beaten. Next time!", w, tix); say(b); life_ev(ctl, WN_FUN, 0); }
    mg_ui = 0; mg_ver++; }
static void opp_step(int dir) {   // the opponent row: the CPU, then each other Bore of the household as player 2
    int n = nsim + 1, v = opp_i + 1, k = 0; do { v = (v + dir + n) % n; k++; } while (v - 1 == you_i && k <= n); opp_i = v - 1; if (opp_i == you_i) opp_i = -1;
}
static void fight_frame(int held, int down, int steps, int tx, int ty) {
    int hd = 0, pr = 0;
    if (fight_2p && (fs == FS_FIGHT || fs == FS_INTRO || fs == FS_END)) {   // two players, one DS: P1 the D-pad (up jumps) and L R; P2 Y A X B as left right jump duck, SELECT a throw, and the touch chips for punch and kick
        int tp = (held & KEY_TOUCH) && fs == FS_FIGHT && hit(142, 144, 56, 42, tx, ty), tk = (held & KEY_TOUCH) && fs == FS_FIGHT && hit(200, 144, 52, 42, tx, ty), tpd = (down & KEY_TOUCH) && fs == FS_FIGHT && hit(142, 144, 56, 42, tx, ty), tkd = (down & KEY_TOUCH) && fs == FS_FIGHT && hit(200, 144, 52, 42, tx, ty);
        fg_h1 = fg_p1 = fg_h2 = fg_p2 = 0;
        if (held & KEY_LEFT) fg_h1 |= FB_L; if (held & KEY_RIGHT) fg_h1 |= FB_R; if (held & KEY_DOWN) fg_h1 |= FB_D; if (held & KEY_L) fg_h1 |= FB_A1; if (held & KEY_R) fg_h1 |= FB_A2;
        if (down & KEY_UP) fg_p1 |= FB_J; if (down & KEY_L) fg_p1 |= FB_A1; if (down & KEY_R) fg_p1 |= FB_A2;
        if (held & KEY_Y) fg_h2 |= FB_L; if (held & KEY_A) fg_h2 |= FB_R; if (held & KEY_B) fg_h2 |= FB_D; if (tp) fg_h2 |= FB_A1; if (tk) fg_h2 |= FB_A2; if (held & KEY_SELECT) fg_h2 |= FB_A1 | FB_A2;
        if (down & KEY_X) fg_p2 |= FB_J; if (tpd) fg_p2 |= FB_A1; if (tkd) fg_p2 |= FB_A2; if (down & KEY_SELECT) fg_p2 |= FB_A1 | FB_A2;
        if (down & KEY_START) pr |= PAD_START;
        if ((down & KEY_TOUCH) && fs == FS_FIGHT && hit(200, 4, 52, 18, tx, ty)) pr |= PAD_START;
        int og = fs; if (!fight_update(0, pr, steps)) { fight_finish(); return; } (void)og; mg_ver++; return; }
    if (held & KEY_UP) hd |= PAD_UP; if (held & KEY_DOWN) hd |= PAD_DOWN; if (held & KEY_LEFT) hd |= PAD_LEFT; if (held & KEY_RIGHT) hd |= PAD_RIGHT;
    if (held & KEY_A) hd |= PAD_CROSS; if (held & KEY_B) hd |= PAD_CIRCLE; if (held & KEY_X) hd |= PAD_TRIANGLE; if (held & KEY_Y) hd |= PAD_SQUARE; if (held & KEY_L) hd |= PAD_L1; if (held & KEY_R) hd |= PAD_R1;
    if (down & KEY_UP) pr |= PAD_UP; if (down & KEY_DOWN) pr |= PAD_DOWN; if (down & KEY_LEFT) pr |= PAD_LEFT; if (down & KEY_RIGHT) pr |= PAD_RIGHT;
    if (down & KEY_A) pr |= PAD_CROSS; if (down & KEY_B) pr |= PAD_CIRCLE; if (down & KEY_X) pr |= PAD_TRIANGLE; if (down & KEY_Y) pr |= PAD_SQUARE; if (down & KEY_L) pr |= PAD_L1; if (down & KEY_R) pr |= PAD_R1;
    if (down & KEY_START) pr |= PAD_START; if (down & KEY_SELECT) pr |= PAD_SELECT;
    if (down & KEY_TOUCH) {
        if (fs == FS_PICK || fs == FS_VS || fs == FS_RESULT || fs == FS_PAUSE) { if (hit(150, 168, 50, 20, tx, ty)) pr |= fs == FS_PAUSE ? PAD_START : PAD_CROSS; if (hit(204, 168, 48, 20, tx, ty)) pr |= fs == FS_PAUSE ? PAD_SELECT : PAD_CIRCLE; }
        else if (fs == FS_CHAMP) { if (hit(204, 168, 48, 20, tx, ty)) pr |= PAD_CROSS; }
        else if (fs == FS_FIGHT && !fight_2p) { if (hit(204, 168, 48, 20, tx, ty)) pr |= PAD_START; } }
    int og = fs; if (!fight_update(hd, pr, steps)) { fight_finish(); return; }
    if (fs != og || fs != FS_PICK || pr) mg_ver++;
    if (fs == FS_FIGHT || fs == FS_END || fs == FS_INTRO) mg_ver++;
}
