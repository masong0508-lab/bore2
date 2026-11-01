// arcade.c - ARCADE.OVL: the FLOOR you walk about (rows of cabinets, a token machine, a score kiosk, the prize counter, the wall, townsfolk who
// play and cheer, and the people who know you, who challenge you), the prize counter, the tokens, the scores and medals. The games are in
// ARCADE2.OVL (arc2.c, ag_*.c): walking up to a cabinet and pressing X sends you there and a game's end brings you back (arcade_res.c).
// BORTUA FIGHTER (fight.c) is in this overlay. The cabinet's flow (rules, play, initials) is arc_core.c.
#include "arc_i.h"

static int mode, game, ft, quit, pe, on_core;
static u32 arng_;  
static int fx, fy, fdir, fanim;      // you: feet, facing, walk phase
// ---- the floor ----
enum { M_FLOOR, M_CORE, M_PRIZE, M_BOARD, M_TOKEN, M_CHAL };
static int acc, pmsg_t; static const char *pmsg;
#define CABW 32
static void cab_xy(int i, int *x, int *y) { *x = 8 + (i % 7) * 40; *y = i < 7 ? 52 : 122; }
static int near_cab(void) {
    int row = fy >= 100 && fy <= 124 ? 0 : fy >= 166 && fy <= 190 ? 1 : -1; if (row < 0 || fx > 282) return -1;
    if (row == 1 && fy >= 182 && (fx < 58 || (fx > 128 && fx < 192) || fx > 246)) return -1;   // (the kiosks are in front of the bottom row)
    int c = (fx - 4) / 40; if (c < 0 || c > 6 || (fx - 8) % 40 > 34 + 4) return -1;
    return row * 7 + c;
}
static int near_kiosk(void) {   // 0 tokens, 1 scores, 2 prizes
    if (fy < 182) return -1;
    if (fx < 58) return 0; if (fx > 128 && fx < 192) return 1; if (fx > 246) return 2; return -1;
}
// the townsfolk: they walk to a cabinet, play, cheer or groan, and walk on. One of them knows you and comes to challenge you.
typedef struct { s16 x, y; u8 st, t, cab, who, anim, mood, role; s8 sim; u16 c[4]; } Walk;
static Walk wk[6];
static const char *const tname[16] = { "ANN", "BOB", "CY", "DEE", "ED", "FAY", "GUS", "HAL", "IVY", "JO", "KAI", "LEE", "MAX", "NIA", "OTTO", "PIA" };
static const char *const bub_good[4] = { "WOO!", "NICE!", "YES!", "WOW!" }, *const bub_bad[3] = { "AW", "NOOO", "OOPS" };
static struct { int on, w, game, target, stake, foe_skill; char nm[12]; } ch;   // the challenge
static int ch_t, ch_cool;
static const u16 skin_c[4] = { RGB(31, 25, 20), RGB(26, 19, 14), RGB(19, 13, 9), RGB(12, 8, 6) }, hair_c[5] = { RGB(4, 3, 2), RGB(18, 10, 4), RGB(28, 22, 8), RGB(24, 6, 4), RGB(24, 24, 26) },
    top_c[8] = { RGB(31, 6, 6), RGB(6, 18, 31), RGB(8, 26, 10), RGB(31, 24, 4), RGB(24, 8, 28), RGB(31, 14, 22), RGB(4, 24, 24), RGB(30, 30, 30) };
static void person_init(Walk *w, int who) {
    w->who = (u8)who; w->c[0] = skin_c[ar8() & 3]; w->c[1] = hair_c[ar8() % 5]; w->c[2] = top_c[ar8() & 7]; w->c[3] = RGB(6 + ar8() % 8, 6 + ar8() % 8, 14 + ar8() % 14);
    w->sim = -1; w->role = 3;
}
static void walkers_init(void) {
    memset(wk, 0, sizeof wk);
    for (int i = 0; i < 6; i++) {
        Walk *w = &wk[i]; person_init(w, (i * 5 + ar8()) & 15);
        w->x = (s16)(30 + ar8() % 240); w->y = (s16)(i & 1 ? 112 : 178); w->st = 0; w->t = (u8)(ar8() % 40); w->cab = (u8)(ar8() % AG_NGAME);
    }
    ch.on = 0; ch_t = 150; ch_cool = 0;
}
static void aim(Walk *w) {   // where they go to play: in front of a cabinet
    int x, y; cab_xy(w->cab, &x, &y); w->t = 0;
}
static int wp(Walk *w, int *tx, int *ty, int gx, int gy) {   // the next waypoint: along an aisle, round by the open end, along the other
    int a = w->y < 140 ? 0 : 1, b = gy < 140 ? 0 : 1;
    if (a != b) { if (ABS(w->x - 298) > 3) { *tx = 298; *ty = w->y; } else { *tx = 298; *ty = gy; } } else { *tx = gx; *ty = gy; }
    return 1;
}
static int walk_to(Walk *w, int gx, int gy, int sp) {   // 1 once there
    int tx, ty; wp(w, &tx, &ty, gx, gy);
    int dx = tx - w->x, dy = ty - w->y;
    if (dx) w->x = (s16)(w->x + CLAMP(dx, -sp, sp)); else if (dy) w->y = (s16)(w->y + CLAMP(dy, -sp, sp));
    w->anim++;
    return ABS(gx - w->x) < 2 && ABS(gy - w->y) < 2;
}
static void cabfront(int c, int *x, int *y, int slot) { int cx, cy; cab_xy(c, &cx, &cy); *x = cx + 16 + (slot ? -9 + slot * 6 : 0); *y = c < 7 ? 108 : 176; }
static void sim_chal(void) {   // pick who comes: someone in the house, or a name from town
    ch.on = 0; int pick = -1;
    for (int k = 0; k < nsims; k++) { int j = (ar8() + k) % nsims; if (j != player && !sims[j].away && sims[j].bore.stage != AG_BABY) { pick = j; break; } }
    Walk *w = &wk[0]; for (int i = 0; i < 6; i++) if (wk[i].st != 6) { w = &wk[i]; break; }
    int role = 3; char nm[12];
    if (pick >= 0 && (ar8() & 3)) {
        Sim *p = &sims[player], *o = &sims[pick];
        int fam = o->uid == p->partner || o->par[0] == p->uid || o->par[1] == p->uid || p->par[0] == o->uid || p->par[1] == o->uid;
        role = fam ? 2 : p->rel[pick] <= -20 ? 1 : 0;
        strncpy(nm, o->bore.first, 11); nm[11] = 0;
        for (int k = 0; k < 4; k++) w->c[k] = o->bore.col[k == 0 ? 1 : k == 1 ? 5 : k == 2 ? 6 : 7];
        w->sim = (s8)pick;
    } else { role = (ar8() & 3) == 0 ? 1 : (ar8() & 1) ? 0 : 3; strncpy(nm, tname[ar8() & 15], 11); nm[11] = 0; w->sim = -1; }
    w->role = (u8)role; w->x = 304; w->y = 112; w->st = 6; w->t = 0; w->mood = 0;
    strcpy(ch.nm, nm); ch.w = (int)(w - wk); ch.on = 1;
    int g = (int)(((w->sim >= 0 ? sims[w->sim].uid : ar8()) * 7 + role * 3 + ar8() % 3) % AG_NGAME);
    if (g == AG_FIGHTER || g == AG_CLAW) g = AG_BREAKER + (g + role) % 4;
    ch.game = g; ch.stake = role == 2 ? 5 : role == 1 ? 12 : 8; ch.foe_skill = 25 + ar8() % 55 + (role == 1 ? 15 : 0);
    ch.target = gi[g].par * (50 + ch.foe_skill) / 100;
    ch_t = 600;
}
static void walkers_tick(void) {
    int cheer_all = 0;
    if (!ch.on && --ch_t <= 0) { if (ch_cool > 0) ch_cool--; else sim_chal(); ch_t = 300; }
    for (int i = 0; i < 6; i++) {
        Walk *w = &wk[i]; w->t++;
        switch (w->st) {
        case 0: if (w->t > 20 + ar8() % 30) { w->cab = (u8)(ar8() % AG_NGAME); w->st = 1; w->t = 0; } break;     // looking round
        case 1: { int gx, gy; cabfront(w->cab, &gx, &gy, (i % 3) - 1); if (walk_to(w, gx, gy, 1 + (i & 1))) { w->st = 2; w->t = 0; w->anim = 0; } break; }   // walking to a cabinet
        case 2: if (w->t > 110 + ar8() % 100) { w->mood = (u8)((ar8() % 3) ? 1 : 2); w->st = 3; w->t = 0; if (w->mood == 1 && (ar8() & 1)) cheer_all = 1; } break;   // playing
        case 3: if (w->t > 40) { w->st = 0; w->t = 0; } break;                                                // cheering or groaning
        case 6: { if (walk_to(w, fx - 18, fy, 2)) { w->st = 7; w->t = 0; } if (w->t > 900) { w->st = 8; } break; }   // the challenger: comes to you
        case 7: if (w->t > 700) { w->st = 8; } break;                                                          // waiting for your answer
        case 8: if (walk_to(w, 304, 112, 2) || w->t > 200) { w->st = 0; w->t = 0; w->x = (s16)(60 + ar8() % 180); w->sim = -1; w->role = 3; ch.on = 0; ch_cool = 1; ch_t = 600; } break;
        }
    }
    if (cheer_all) for (int i = 0; i < 6; i++) if (wk[i].st == 0 || wk[i].st == 2) { if (ar8() & 1) { wk[i].mood = 1; wk[i].st = 3; wk[i].t = 20; } }
}
static void person(int x, int y, const u16 *c, int anim, int moving, int up, int dir, int jump) {
    int sw = moving ? ((anim >> 2) & 1) * 2 - 1 : 0; y -= jump;
    KA(x - 6, y - 1, 12, 3, UI_RGB(0, 0, 0));
    gfx_rect_c(x - 3, y - 6, 3, 6 + sw, c[3], 0); gfx_rect_c(x, y - 6, 3, 6 - sw, c[3], 0);          // legs
    gfx_rect_c(x - 4, y - 14, 8, 8, c[2], 0);                                                             // top
    gfx_rect_c(x - 6, y - 14 + (up ? -4 : sw), 2, 6, c[2], 0); gfx_rect_c(x + 4, y - 14 + (up ? -4 : -sw), 2, 6, c[2], 0);   // arms
    gfx_rect_c(x - 4, y - 21, 8, 7, c[0], 0); gfx_rect_c(x - 4, y - 22, 8, 3, c[1], 0);                   // head, hair
    K(x - 2 + dir, y - 18, 1, 2, UI_RGB(10, 10, 20)); K(x + 1 + dir, y - 18, 1, 2, UI_RGB(10, 10, 20));
}
static void me_cols(u16 *c) { Bore *b = &sims[player].bore; c[0] = b->col[1]; c[1] = b->col[5]; c[2] = b->col[6]; c[3] = b->col[7]; }

// ---- drawing the floor ----
static void thumb(int g, int x, int y) {   // what a cabinet's little screen shows (22 x 16)
    int t = ft;
    K(x, y, 22, 16, UI_RGB(6, 6, 14));
    switch (g) {
    case AG_FIGHTER: K(x + 3, y + 5, 4, 9, UI_RGB(230, 70, 70)); K(x + 15, y + 5, 4, 9, UI_RGB(70, 130, 240)); if ((t >> 3) & 1) K(x + 7, y + 7, 3, 2, UI_RGB(255, 255, 255)); break;
    case AG_BREAKER: for (int r = 0; r < 3; r++) for (int c = 0; c < 5; c++) K(x + 1 + c * 4, y + 1 + r * 3, 3, 2, UI_RGB(r == 0 ? 250 : r == 1 ? 250 : 60, r == 0 ? 60 : r == 1 ? 200 : 220, r == 2 ? 80 : 50)); K(x + 3 + (t / 3) % 14, y + 13, 5, 1, UI_RGB(90, 230, 250)); K(x + 5 + (t * 2) % 12, y + 8 + (t / 2) % 5, 1, 1, UI_RGB(255, 255, 255)); break;
    case AG_CATCH: for (int i = 0; i < 3; i++) K(x + 3 + i * 7, y + (t + i * 5) % 12, 2, 2, UI_RGB(250, 200, 40)); K(x + 5 + (t / 4) % 9, y + 13, 7, 2, UI_RGB(190, 110, 30)); break;
    case AG_WHACK: for (int i = 0; i < 3; i++) for (int j = 0; j < 2; j++) { K(x + 2 + i * 7, y + 3 + j * 7, 5, 3, UI_RGB(40, 24, 16)); if (((t >> 3) + i + j) % 5 == 0) K(x + 3 + i * 7, y + 1 + j * 7, 3, 3, UI_RGB(170, 100, 50)); } break;
    case AG_STRIKER: K(x + 9, y + 1, 4, 14, UI_RGB(50, 50, 80)); K(x + 9, y + 15 - (t % 28) / 2, 4, (t % 28) / 2, UI_RGB(250, 160, 40)); K(x + 8, y, 6, 2, UI_RGB(250, 220, 40)); break;
    case AG_BLOCKS: K(x + 6, y + 1, 10, 14, UI_RGB(14, 14, 30)); K(x + 7, y + 11, 3, 3, UI_RGB(170, 90, 250)); K(x + 10, y + 12, 3, 2, UI_RGB(80, 220, 240)); K(x + 13, y + 11, 2, 3, UI_RGB(250, 220, 40)); K(x + 8 + (t / 8) % 3 * 2, y + 1 + (t / 3) % 9, 4, 2, UI_RGB(250, 90, 90)); break;
    case AG_SHMUP: for (int i = 0; i < 4; i++) K(x + (i * 7 + t / 3) % 22, y + (i * 5 + t) % 16, 1, 1, UI_RGB(255, 255, 255)); K(x + 10, y + 11, 3, 4, UI_RGB(90, 230, 250)); K(x + 11, y + 6 - (t % 6), 1, 3, UI_RGB(255, 240, 80)); K(x + 4 + (t / 5) % 12, y + 2, 4, 2, UI_RGB(250, 70, 130)); break;
    case AG_RACER: K(x, y + 6, 22, 10, UI_RGB(30, 120, 40)); K(x + 5, y + 6, 12, 10, UI_RGB(70, 70, 80)); for (int i = 0; i < 3; i++) gfx_rect(x + 10, y + 7 + ((t * 2 + i * 4) % 10), 2, 2, 240, 240, 240, 0); K(x + 9, y + 12, 4, 3, UI_RGB(250, 60, 60)); K(x, y, 22, 6, UI_RGB(90, 150, 240)); break;
    case AG_MAZE: K(x + 1, y + 1, 20, 1, UI_RGB(40, 40, 220)); K(x + 1, y + 14, 20, 1, UI_RGB(40, 40, 220)); K(x + 8, y + 5, 6, 1, UI_RGB(40, 40, 220)); for (int i = 0; i < 5; i++) K(x + 3 + i * 4, y + 8, 1, 1, UI_RGB(250, 220, 160)); K(x + (t / 2) % 18 + 1, y + 7, 3, 3, UI_RGB(250, 230, 40)); K(x + 19 - (t / 3) % 16, y + 3, 3, 3, UI_RGB(250, 70, 70)); break;
    case AG_HOOPS: K(x + 14, y + 2, 6, 1, UI_RGB(250, 120, 40)); K(x + 20, y, 1, 4, UI_RGB(240, 240, 240)); gfx_rect(x + 3 + (t % 16) / 2, y + 12 - (t % 16 < 8 ? (t % 16) : 16 - (t % 16)), 2, 2, 250, 140, 60, 0); K(x + 2, y + 14, 18, 2, UI_RGB(150, 100, 60)); break;
    case AG_SKEE: for (int i = 1; i < 4; i++) K(x + 11 - i * 3, y + 9 - i * 2, i * 6, 1, UI_RGB(90, 230, 170)); K(x + 9, y + 2, 4, 2, UI_RGB(250, 220, 40)); K(x + 10, y + 15 - (t % 12), 2, 2, UI_RGB(250, 130, 60)); break;
    case AG_HOCKEY: K(x + 11, y, 1, 16, UI_RGB(70, 100, 200)); K(x + 3, y + 5 + (t / 5) % 6, 3, 3, UI_RGB(240, 60, 60)); K(x + 16, y + 5 + (t / 4) % 6, 3, 3, UI_RGB(60, 100, 240)); gfx_rect(x + 10 + (t / 2) % 4 * 0 + ((t >> 2) & 1) * 3, y + 7, 2, 2, 240, 240, 240, 0); break;
    case AG_RHYTHM: for (int i = 0; i < 4; i++) { K(x + 2 + i * 5, y, 4, 16, UI_RGB(16, 12, 34)); K(x + 2 + i * 5, y + (t * 2 + i * 5) % 14, 4, 2, UI_RGB(i == 0 ? 250 : i == 1 ? 90 : i == 2 ? 80 : 250, i == 0 ? 60 : i == 1 ? 230 : i == 2 ? 240 : 220, i == 3 ? 40 : 90)); } K(x, y + 13, 22, 1, UI_RGB(255, 255, 255)); break;
    case AG_CLAW: K(x + 2 + (t / 3) % 14, y, 1, 5 + (t % 20 > 10 ? 4 : 0), UI_RGB(200, 200, 210)); K(x + 1 + (t / 3) % 14, y + 5 + (t % 20 > 10 ? 4 : 0), 3, 2, UI_RGB(200, 200, 210)); for (int i = 0; i < 4; i++) K(x + 2 + i * 5, y + 11, 4, 4, UI_RGB(i & 1 ? 250 : 240, i & 1 ? 120 : 200, i & 1 ? 190 : 60)); break;
    }
}
static void cab_draw(int g, int x, int y, int lit) {
    const GInfo *a = &gi[g]; int r = a->r, gg = a->g, b = a->b;
    KA(x - 3, y + 44, 38, 5, UI_RGB(r / 3, gg / 3, b / 3));                                          // the glow on the carpet
    KA(x + 2, y + 46, 30, 3, UI_RGB(0, 0, 0));
    K(x, y + 6, CABW, 40, UI_RGB(r / 5 + 14, gg / 5 + 12, b / 5 + 18));                              // body
    K(x, y + 6, 3, 40, UI_RGB(r / 8, gg / 8, b / 8 + 6)); K(x + 29, y + 6, 3, 40, UI_RGB(r / 8, gg / 8, b / 8 + 6));
    K(x, y, CABW, 8, UI_RGB(r, gg, b)); KA(x, y, CABW, 2, UI_RGB(255, 255, 255));                    // marquee
    ui_textc(x + 16, y + 1, 1, a->tag, UI_RGB(255, 255, 255));
    K(x + 3, y + 9, 26, 20, UI_RGB(4, 4, 8)); thumb(g, x + 5, y + 11);                               // screen
    K(x + 3, y + 9, 26, 1, UI_RGB(r / 2, gg / 2, b / 2));
    K(x + 1, y + 30, 30, 6, UI_RGB(r / 2 + 40, gg / 2 + 40, b / 2 + 40));                            // control deck
    K(x + 7, y + 28, 2, 4, UI_RGB(200, 200, 210)); ui_disc(x + 8, y + 28, 5, UI_RGB(r, gg, b));
    K(x + 17, y + 32, 3, 3, UI_RGB(250, 60, 60)); K(x + 22, y + 32, 3, 3, UI_RGB(250, 220, 50));
    K(x + 11, y + 39, 10, 5, UI_RGB(24, 24, 30)); if ((ft >> 4) & 1) K(x + 14, y + 41, 4, 1, UI_RGB(255, 220, 60));
    if (lit) { K(x - 1, y - 1, CABW + 2, 1, UI_RGB(255, 255, 255)); K(x - 1, y + 46, CABW + 2, 1, UI_RGB(255, 255, 255)); K(x - 1, y - 1, 1, 48, UI_RGB(255, 255, 255)); K(x + CABW, y - 1, 1, 48, UI_RGB(255, 255, 255)); }
}
static void kiosk_token(int x, int y) {
    K(x, y, 48, 34, UI_RGB(200, 150, 20)); K(x + 2, y + 2, 44, 30, UI_RGB(240, 190, 40)); K(x, y, 48, 6, UI_RGB(250, 220, 60));
    ui_textc(x + 24, y, 1, "TOKENS", UI_RGB(120, 70, 0)); K(x + 6, y + 10, 22, 12, UI_RGB(30, 20, 10));
    for (int i = 0; i < 3; i++) ui_disc(x + 11 + i * 7, y + 16, 6, UI_RGB(250, 210, 50)); K(x + 32, y + 12, 10, 4, UI_RGB(20, 20, 20)); K(x + 34, y + 22, 8, 8, UI_RGB(60, 40, 10));
}
static void kiosk_score(int x, int y) {
    K(x, y, 56, 34, UI_RGB(40, 60, 140)); K(x + 2, y + 2, 52, 30, UI_RGB(70, 100, 200)); K(x, y, 56, 6, UI_RGB(90, 130, 230));
    ui_textc(x + 28, y, 1, "SCORES", UI_WHITE); K(x + 5, y + 9, 46, 21, UI_RGB(10, 14, 40));
    for (int i = 0; i < 3; i++) gfx_rect(x + 8, y + 12 + i * 6, 14 + ((ft / 6 + i * 5) % 6) * 4, 3, 250, 220 - i * 40, 60 + i * 50, 0);
}
static void kiosk_prize(int x, int y) {
    K(x, y - 6, 66, 40, UI_RGB(150, 40, 120)); K(x + 2, y - 4, 62, 8, UI_RGB(240, 90, 190)); ui_textc(x + 33, y - 5, 1, "PRIZES", UI_WHITE);
    K(x + 3, y + 5, 60, 14, UI_RGB(40, 14, 40));   // the shelf with toys
    for (int i = 0; i < 6; i++) { int c = i * 37; gfx_rect(x + 5 + i * 10, y + 8 + ((i & 1) ? 0 : 2), 7, 8, 100 + c % 150, 80 + (c * 3) % 170, 90 + (c * 7) % 160, 0); K(x + 6 + i * 10, y + 9, 1, 1, UI_RGB(255, 255, 255)); }
    K(x, y + 19, 66, 15, UI_RGB(120, 30, 100)); K(x, y + 19, 66, 3, UI_RGB(250, 120, 210));
}
static void floor_draw(void) {
    gfx_set_clear(16, 8, 30);
    K(0, 0, 320, 240, UI_RGB(24, 12, 42));                                         // the carpet
    for (int y = 50; y < 240; y += 8) K(0, y, 320, 1, UI_RGB(32, 16, 56));
    for (int x = 0; x < 320; x += 20) for (int y = 53; y < 240; y += 24) gfx_rect(x + ((y / 24) & 1) * 10, y, 3, 3, 56, 30, 96, 0);
    K(0, 0, 320, 48, UI_RGB(30, 16, 52));                                         // the wall
    for (int y = 6; y < 48; y += 6) K(0, y, 320, 1, UI_RGB(22, 10, 40));
    int fl = (ft >> 5) & 1;
    K(0, 46, 320, 2, UI_RGB(240, 60, 200)); KA(0, 48, 320, 3, UI_RGB(120, 20, 100));                   // the neon rail
    ui_textc(160, 3, 2, "BORE ARCADE", fl ? UI_RGB(255, 120, 230) : UI_RGB(120, 250, 255));
    // the wall's leaderboard: one game at a time, its three best
    { int g = (ft / 120) % AG_NGAME; K(70, 20, 180, 24, UI_RGB(8, 8, 24)); K(70, 20, 180, 1, UI_RGB(90, 230, 250)); K(70, 43, 180, 1, UI_RGB(250, 90, 220));
      ui_text(74, 22, 1, gi[g].nm, UI_RGB(gi[g].r, gi[g].g, gi[g].b));
      for (int i = 0; i < 3; i++) { char s[24], n[4]; ini_str(arc.hs[g][i], n); if (SC(arc.hs[g][i])) snprintf(s, sizeof s, "%d %s %d", i + 1, n, SC(arc.hs[g][i])); else snprintf(s, sizeof s, "%d --- ---", i + 1); ui_text(74 + i * 60, 33, 1, s, i == 0 ? UI_AMBER : UI_WHITE); } }
    K(8, 20, 54, 24, UI_RGB(8, 8, 24)); ui_textc(35, 22, 1, "TOKENS", UI_AMBER); { char s[12]; snprintf(s, sizeof s, "%d", arc.tok); ui_textc(35, 32, 1, s, UI_WHITE); }
    K(258, 20, 54, 24, UI_RGB(8, 8, 24)); ui_textc(285, 22, 1, "TICKETS", UI_GREEN); { char s[12]; snprintf(s, sizeof s, "%d", arcade_tix); ui_textc(285, 32, 1, s, UI_WHITE); }
    int nc = near_cab();
    for (int i = 0; i < AG_NGAME; i++) { int x, y; cab_xy(i, &x, &y); cab_draw(i, x, y, i == nc); }
    kiosk_token(8, 190); kiosk_score(132, 190); kiosk_prize(248, 192);
    KA(0, 124, 320, 1, UI_RGB(240, 60, 200)); KA(0, 188, 320, 1, UI_RGB(90, 230, 250));
    // people, back to front
    Walk *ord[7]; int n = 0; for (int i = 0; i < 6; i++) ord[n++] = &wk[i];
    for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) if (ord[j]->y < ord[i]->y) { Walk *t = ord[i]; ord[i] = ord[j]; ord[j] = t; }
    u16 mc[4]; me_cols(mc); int mey = fy, drawn = 0;
    for (int i = 0; i <= n; i++) {
        if (!drawn && (i == n || mey <= ord[i]->y)) {
            person(fx, fy, mc, fanim, fanim > 0 && (mode == M_FLOOR), 0, fdir ? 1 : -1, 0); drawn = 1;
            ui_icon_s(fx - 6, fy - 36, IC_ARROW, 12, UI_RGB(255, 230, 80));
        }
        if (i == n) break;
        Walk *w = ord[i]; int mv = w->st == 1 || w->st == 6 || w->st == 8, jump = 0, up = 0;
        if (w->st == 3) { if (w->mood == 1) { jump = ABS(((w->t * 3) % 12) - 6) > 3 ? 0 : 3 + (w->t & 3); up = 1; } }
        if (w->st == 2) { up = (w->t >> 2) & 1; }
        int dir = 0; { int cx, cy; cab_xy(w->cab, &cx, &cy); dir = w->st == 2 ? 0 : (w->x < 160 ? 1 : -1); (void)cy; }
        person(w->x, w->y, w->c, w->anim, mv, up, dir, jump);
        if (w->st == 3 && w->t < 36) ui_bubble(w->x, w->y - 28, w->mood == 1 ? bub_good[w->who & 3] : bub_bad[w->who % 3], UI_RGB(255, 255, 255));
        if (w->st == 6 || w->st == 7) { ui_bubble(w->x, w->y - 28, w->role == 2 ? "PLAY ME!" : w->role == 1 ? "BET YOU CAN'T" : "CHALLENGE!", UI_RGB(255, 235, 150)); }
    }
    core_fx_draw();
}

// ---- the prize counter ----
typedef struct { const char *nm, *say; u16 cost; u8 cat, kind, bit; } Prize;   // kind: 1 snack, 2 toy, 3 mood, 4 outfit, 5 pet, 6 egg, 7 cash, 8 home;  bit: the owned flag (0 none: you can buy it again)
static const char *const pcat[4] = { "TOYS", "HOME", "STYLE", "PETS AND EGGS" };
static const Prize prz[] = {
    { "CANDY", "SUGAR RUSH!", 8, 0, 1, 0 }, { "BOUNCY BALL", "BOING BOING BOING", 20, 0, 2, 0 }, { "YO-YO", "WALK THE DOG", 30, 0, 2, 0 }, { "PLUSH TEDDY", "IT HAS A TINY HAT", 60, 0, 3, 1 },
    { "ROBOT TOY", "BEEP BOOP. SMART TOO", 120, 0, 3, 2 }, { "MEGA TROPHY", "NUMBER ONE AT SOMETHING", 200, 0, 3, 3 },
    { "LAVA LAMP", "GLOOP GLOOP. SENT HOME", 90, 1, 8, 4 }, { "BEAN BAG", "SO SOFT. SENT HOME", 160, 1, 8, 5 }, { "NEON SIGN", "LOOKS GREAT AT HOME", 220, 1, 8, 6 }, { "HOME ARCADE CABINET", "SENT HOME. FREE PLAY!", 450, 1, 8, 7 },
    { "ROCK STAR OUTFIT", "YOU ROCK", 250, 2, 4, 8 }, { "SAMURAI OUTFIT", "HAI!", 300, 2, 4, 9 }, { "SUPERHERO OUTFIT", "UP, UP AND AWAY", 350, 2, 4, 10 }, { "ROYAL OUTFIT", "ALL BOW TO YOU", 400, 2, 4, 11 },
    { "BOREAGOTCHI EGG", "IT IS WARM", 140, 3, 6, 0 }, { "KITTEN", "MEOW. IT IS YOURS", 350, 3, 5, 12 }, { "PUPPY", "WOOF! IT IS YOURS", 450, 3, 5, 13 },
    { "GOLDEN TICKET", "TRADED IN FOR $120!", 500, 3, 7, 0 } };
#define NPRZ ((int)(sizeof prz / sizeof prz[0]))
static int pcur, ptab;
static int pidx(int tab, int k) { int n = 0; for (int i = 0; i < NPRZ; i++) if (prz[i].cat == tab || (tab == 3 && prz[i].kind == 7)) { if (n == k) return i; n++; } return -1; }
static int pcount(int tab) { int n = 0; for (int i = 0; i < NPRZ; i++) if (prz[i].cat == tab || (tab == 3 && prz[i].kind == 7)) n++; return n; }
static void prize_buy(int i) {
    const Prize *p = &prz[i]; Sim *s = &sims[player];
    if (p->bit && (arc.prz & (1u << p->bit))) { pmsg = "YOU HAVE THAT ONE ALREADY"; pmsg_t = 60; audio_voice(VOICE_NAH); return; }
    if (arcade_tix < p->cost) { pmsg = "NOT ENOUGH TICKETS YET"; pmsg_t = 60; audio_voice(VOICE_NAH); return; }
    switch (p->kind) {
    case 5: if (pet_kind) { pmsg = "YOU HAVE A PET ALREADY"; pmsg_t = 60; audio_voice(VOICE_NAH); return; } break;
    case 6: { Toy *t = gotchi_toy(player); if (t) { int e = -1; for (int k = 0; k < GB_N; k++) if (!t->b[k].on) { e = k; break; } if (e < 0) { pmsg = "NO ROOM IN THE TOY"; pmsg_t = 60; audio_voice(VOICE_NAH); return; } } } break;
    }
    arcade_tix -= p->cost; if (p->bit) arc.prz |= 1u << p->bit;
    switch (p->kind) {
    case 1: need_add(s, N_FUN, 5); need_add(s, N_FOOD, 15); break;
    case 2: need_add(s, N_FUN, 15); need_add(s, N_SOCIAL, 4); s->skill_body += 40; break;
    case 3: need_add(s, N_FUN, 25); need_add(s, N_SOCIAL, 12); sim_moodlet(s, 32); if (p->bit == 2) s->skill_logic += 200; break;
    case 8: need_add(s, N_ROOM, 25); need_add(s, N_COMFY, 12); sim_moodlet(s, 32); break;
    case 4: { int t = look_theme_named(p->bit == 8 ? "ROCK STAR" : p->bit == 9 ? "SAMURAI" : p->bit == 10 ? "SUPERHERO" : "ROYALTY"); if (t) { look_theme(&s->bore, t); look_build(&s->bore); } sim_moodlet(s, 30); medal(9); break; }
    case 5: pet_kind = p->bit == 12 ? 2 : 1; pet_col = look_rand() % 6; if (pet_kind == 1) dog_random(&pet_dog, look_rand()); pet_hun = 100; pet_zone = -1; pet_ph = 0; need_add(s, N_FUN, 20); medal(10); break;
    case 6: { Toy *t = gotchi_toy(player); if (!t) gotchi_buy(player); else for (int k = 0; k < GB_N; k++) if (!t->b[k].on) { gotchi_egg(t, k); break; } need_add(s, N_FUN, 15); medal(11); break; }
    case 7: cash += 120; break;
    }
    medal(8); pmsg = p->say; pmsg_t = 90; audio_voice(VOICE_YAHOO);
    { char b[48]; snprintf(b, sizeof b, "%s: %s", p->nm, p->say); sim_note(b); }
}
static const char *const tokn[3] = { "5 TOKENS", "24 TOKENS", "65 TOKENS" }; static const u8 tokc[3] = { 5, 20, 50 }, tokg[3] = { 5, 24, 65 };
static int tsel;
// ---- opening, leaving, the flow ----
// arcade_open(arg): 0 BORTUA FIGHTER's page, AG_PRIZES the prize counter, 100 + g the floor (standing at cabinet g; 113 and up: at the door)
static void fighter_start(void) { fight_open(1); }
static int fighter_tick(int hd, int pr, int steps, int x) {
    if (fight_update(hd, pr, steps)) return 1;
    fight_close();
    { extern int fight_last_wins, fight_matches; int w = fight_last_wins; ag_pts = w * 60 + (fight_champ ? 120 : 0) + fight_best * 10; ag_best = w * 3; ag_perf = fight_matches && w >= 3; if (!fight_matches) ag_pts = 0; }
    return 0;
}
const AGFn ag_fn[AG_NGAME] = { { fighter_start, fighter_tick, fight_draw } };
static void mount_challenger(void) {   // back from a game of a challenge: the challenger is here, cheering or groaning
    Walk *w = &wk[0]; w->x = (s16)fx; w->y = (s16)fy; w->x = (s16)CLAMP(fx - 22, 12, 300); w->st = 3; w->t = 0; w->mood = (u8)(arun.res == 1 ? 2 : 1); w->role = arun.role; w->sim = arun.sim;
    if (arun.sim >= 0 && arun.sim < nsims) { Bore *b = &sims[arun.sim].bore; w->c[0] = b->col[1]; w->c[1] = b->col[5]; w->c[2] = b->col[6]; w->c[3] = b->col[7]; }
    arun.res = 0; arun.foe = 0; ch.on = 0; ch_cool = 1; ch_t = 700;
}
void arcade_open(int a) { arun.role = 77;
    acc = 0; ft = 0; quit = 0; pe = 0; tsel = 1; on_core = 0; 
    walkers_init(); pmsg = 0; pcur = 0; ptab = 0; ag_np = 1; fx = 160; fy = 112; fdir = 1; fanim = 0;
       // (the music you had: given back when you leave)
    if (!(arc.ach & 1)) { arc.tok = (u16)MIN(65000, arc.tok + 8); arc.ach |= 1; sim_note("WELCOME! 8 FREE TOKENS ARE IN YOUR POCKET"); }
    if (a == AG_PRIZES) { mode = M_PRIZE; game = AG_PRIZES; fx = 282; fy = 186; }
    else if (a == AG_FIGHTER) { game = 0; mode = M_CORE; on_core = 1; arun.foe = 0; core_open(0); }
    else if (a >= 100 && a < 100 + AG_NGAME) { int x, y; cab_xy(a - 100, &x, &y); fx = x + 16; fy = a < 107 ? 108 : 172; mode = M_FLOOR; }
    else mode = M_FLOOR;
    if (arun.res && mode == M_FLOOR) mount_challenger();
    if (mode == M_FLOOR) core_song(-1); arun.role = 78;
}
static void leave(void) {
    
    arun.prev = arun.cur = 0; arun.foe = 0; quit = 1;
}
static void go_game(int g, int foe) {   // to a cabinet: this overlay holds BORTUA FIGHTER; ARCADE2.OVL the rest
    game = g; arun.foe = (u16)foe;
    if (g == AG_FIGHTER) { mode = M_CORE; on_core = 1; core_open(0); return; }
    arun.chain = 2; arun.g = (u8)g; quit = 1;
}
static void tick_floor(int hd, int pr) {
    int dx = 0, dy = 0; if (hd & PAD_LEFT) dx = -1; if (hd & PAD_RIGHT) dx = 1; if (hd & PAD_UP) dy = -1; if (hd & PAD_DOWN) dy = 1;
    int nx = fx + dx * 3, ny = fy + dy * 2;
    int a1 = ny >= 102 && ny <= 121, a2 = ny >= 168 && ny <= 190, ow = nx >= 284 && ny >= 102 && ny <= 190, inx = nx >= 6 && nx <= 314;
    if (inx && (a1 || a2 || ow)) { fx = nx; fy = ny; }
    else if (inx && dx && ((fy >= 102 && fy <= 121) || (fy >= 168 && fy <= 190) || fx >= 284)) fx = nx;
    else if (dy && fx >= 284 && ny >= 102 && ny <= 190) fy = ny;
    fx = CLAMP(fx, 6, 314);
    if (dx) fdir = dx > 0; fanim = (dx || dy) ? fanim + 1 : 0;
    if (pr & PAD_CROSS) {
        int k = near_kiosk(), c = near_cab(), w = -1;
        for (int i = 0; i < 6; i++) if ((wk[i].st == 7 || wk[i].st == 6) && ABS(wk[i].x - fx) < 36 && ABS(wk[i].y - fy) < 14) w = i;
        if (w >= 0 && ch.on && ch.w == w) { mode = M_CHAL; wk[w].st = 7; }
        else if (k == 0) { mode = M_TOKEN; tsel = 1; pmsg_t = 0; }
        else if (k == 1) { mode = M_BOARD; pcur = 0; }
        else if (k == 2) { mode = M_PRIZE; pcur = 0; pmsg = 0; pmsg_t = 0; }
        else if (c >= 0) go_game(c, 0);
    }
    if (pr & (PAD_CIRCLE | PAD_START)) leave();
}
static void tick(int hd, int pr) {
    core_fx_tick(); ft++; walkers_tick(); if (pmsg_t > 0) pmsg_t--;
    switch (mode) {
    case M_FLOOR: tick_floor(hd, pr); break;
    case M_PRIZE: {
        int n = pcount(ptab);
        if (pr & PAD_UP) pcur = (pcur + n - 1) % n; if (pr & PAD_DOWN) pcur = (pcur + 1) % n;
        if (pr & PAD_LEFT) { ptab = (ptab + 3) % 4; pcur = 0; } if (pr & PAD_RIGHT) { ptab = (ptab + 1) % 4; pcur = 0; }
        if (pr & PAD_CROSS) { int i = pidx(ptab, pcur); if (i >= 0) prize_buy(i); }
        if (pr & (PAD_CIRCLE | PAD_START)) { if (game == AG_PRIZES) leave(); else mode = M_FLOOR; }
        break; }
    case M_BOARD:
        if (pr & PAD_LEFT) pcur = (pcur + AG_NGAME) % (AG_NGAME + 1); if (pr & PAD_RIGHT) pcur = (pcur + 1) % (AG_NGAME + 1);
        if (pr & (PAD_CIRCLE | PAD_START | PAD_CROSS)) mode = M_FLOOR;
        break;
    case M_TOKEN:
        if (pr & PAD_UP) tsel = (tsel + 2) % 3; if (pr & PAD_DOWN) tsel = (tsel + 1) % 3;
        if (pr & PAD_CROSS) { if (cash < tokc[tsel]) { pmsg = "NOT ENOUGH MONEY"; pmsg_t = 50; audio_voice(VOICE_NAH); } else { cash -= tokc[tsel]; arc.tok = (u16)MIN(65000, arc.tok + tokg[tsel]); pmsg = "CLINK CLINK CLINK!"; pmsg_t = 50; audio_voice(VOICE_AGREE); } }
        if (pr & (PAD_CIRCLE | PAD_START)) mode = M_FLOOR;
        break;
    case M_CHAL:
        if (pr & PAD_CROSS) {   // accepted: what the game needs to know travels in arun
            arun.role = wk[ch.w].role; arun.sim = wk[ch.w].sim; arun.stake = (u8)ch.stake; strncpy(arun.nm, ch.nm, 9); arun.nm[9] = 0;
            go_game(ch.game, ch.target); }
        if (pr & (PAD_CIRCLE | PAD_START)) { mode = M_FLOOR; wk[ch.w].st = 8; wk[ch.w].t = 0; ch_cool = 1; }
        break;
    }
}
int arcade_update(int hd, int pr, int steps) { arun.role = 79;
    if (on_core) {
        if (core_update(hd, pr, steps)) return 1;
        on_core = 0;   // the fight (or a challenge at it) is over: the floor again
        if (arun.res) { mode = M_FLOOR; mount_challenger(); } else mode = M_FLOOR;
        core_song(-1); fx = CLAMP(fx, 6, 314); return 1;
    }
    if (quit) return 0;
    acc += steps;
    while (acc >= 2) { acc -= 2; tick(hd, pr); pr = 0; if (quit) return 0; }
    return 1;
}

// ---- the picture ----
static void draw_prize(void) {
    gfx_set_clear(20, 6, 30); K(0, 0, 320, 240, UI_RGB(40, 12, 56));
    K(0, 0, 320, 20, UI_RGB(150, 40, 120)); gfx_textc(160, 3, 2, "THE PRIZE COUNTER", 255, 255, 255);
    for (int t = 0; t < 4; t++) { int x = 8 + t * 78; ui_round(x, 24, 74, 16, 5, t == ptab ? UI_RGB(250, 190, 60) : UI_RGB(70, 40, 100), t == ptab ? UI_RGB(240, 140, 30) : UI_RGB(50, 24, 80), 0); ui_textc(x + 37, 28, 1, t == 3 ? "PETS EGGS" : pcat[t], t == ptab ? UI_INK : UI_WHITE); }
    int n = pcount(ptab);
    for (int k = 0; k < n; k++) { int i = pidx(ptab, k), y = 46 + k * 22, s = k == pcur, own = prz[i].bit && (arc.prz & (1u << prz[i].bit));
        ui_round(10, y, 190, 20, 5, s ? UI_RGB(226, 242, 254) : UI_RGB(60, 30, 90), s ? UI_HILITE : UI_RGB(46, 20, 70), 0);
        ui_text(18, y + 6, 1, prz[i].nm, s ? UI_BLUE2 : UI_WHITE);
        char c[16]; if (own) snprintf(c, sizeof c, "OWNED"); else snprintf(c, sizeof c, "%d TIX", prz[i].cost);
        ui_text(192 - ui_tw(1, c), y + 6, 1, c, own ? UI_RGB(150, 150, 170) : arcade_tix >= prz[i].cost ? (s ? UI_GOODT : UI_GREEN) : (s ? UI_BADT : UI_RGB(250, 130, 120))); }
    // the shop window: a toy for the one you hover
    { int i = pidx(ptab, pcur); ui_round(208, 46, 104, 120, 8, UI_RGB(30, 14, 50), UI_RGB(20, 8, 36), 0);
      if (i >= 0) { int k = i * 41; ui_disc(260, 90, 40, UI_RGB(100 + k % 150, 70 + (k * 3) % 170, 90 + (k * 7) % 160)); ui_disc(260, 90, 24, UI_RGB(180 + k % 70, 160 + (k * 3) % 90, 170 + (k * 7) % 80)); ui_icon_s(248, 78, ptab == 0 ? IC_STAR : ptab == 1 ? IC_SOFA : ptab == 2 ? IC_SHIRT : IC_HEART, 24, UI_WHITE);
          ui_wrapc(210, 124, 100, 2, prz[i].nm, UI_AMBER); ui_wrapc(210, 146, 100, 2, prz[i].say, UI_RGB(200, 200, 230)); } }
    { char b[48]; snprintf(b, sizeof b, "YOU HAVE %d TICKETS", arcade_tix); ui_textc(160, 188, 1, b, UI_WHITE); }
    if (pmsg_t > 0 && pmsg) ui_textc(160, 204, 1, pmsg, UI_AMBER);
    ui_hintbar(218, "L R PAGE|U D PICK|X SWAP|O BACK");
}
static void draw_token(void) {
    gfx_set_clear(24, 16, 4); K(0, 0, 320, 240, UI_RGB(52, 36, 10)); K(0, 0, 320, 20, UI_RGB(220, 160, 30)); gfx_textc(160, 3, 2, "THE TOKEN MACHINE", 70, 40, 0);
    ui_round(50, 34, 220, 28, 8, UI_RGB(20, 14, 6), UI_RGB(10, 8, 2), 0);
    { char s[48]; snprintf(s, sizeof s, "TOKENS %d      CASH $%ld", arc.tok, cash); ui_textc(160, 44, 1, s, UI_AMBER); }
    for (int i = 0; i < 3; i++) { int y = 76 + i * 34, s = i == tsel; ui_round(50, y, 220, 30, 8, s ? UI_RGB(255, 236, 150) : UI_RGB(90, 64, 18), s ? UI_RGB(240, 190, 50) : UI_RGB(60, 42, 8), 0);
        for (int k = 0; k < 1 + i * 2; k++) ui_disc(66 + k * 7, y + 15, 10, UI_RGB(250, 210, 50));
        ui_text(110, y + 11, 1, tokn[i], s ? UI_INK : UI_WHITE); char c[12]; snprintf(c, sizeof c, "$%d", tokc[i]); ui_text(262 - ui_tw(1, c), y + 11, 1, c, s ? (cash >= tokc[i] ? UI_GOODT : UI_BADT) : UI_WHITE); }
    ui_textc(160, 184, 1, "A TOKEN IS ABOUT A DOLLAR. MORE TOKENS, BETTER DEAL.", UI_RGB(240, 220, 160));
    if (pmsg_t > 0 && pmsg) ui_textc(160, 198, 1, pmsg, UI_WHITE);
    ui_hintbar(218, "U D PICK|X BUY|O BACK");
}
static void draw_board(void) {
    gfx_set_clear(4, 8, 24); K(0, 0, 320, 240, UI_RGB(10, 16, 44)); K(0, 0, 320, 20, UI_RGB(60, 90, 200));
    if (pcur < AG_NGAME) {
        const GInfo *a = &gi[pcur]; gfx_textc(160, 3, 2, "HIGH SCORES", 255, 255, 255);
        gfx_textc(160, 36, 2, a->nm, a->r, a->g, a->b);
        for (int i = 0; i < 3; i++) { char s[32], n[4]; ini_str(arc.hs[pcur][i], n); if (SC(arc.hs[pcur][i])) snprintf(s, sizeof s, "%d   %s   %d", i + 1, n, SC(arc.hs[pcur][i])); else snprintf(s, sizeof s, "%d   ---   ", i + 1); gfx_textc(160, 78 + i * 26, 2, s, i == 0 ? 250 : 220, i == 0 ? 210 : 220, i == 0 ? 60 : 230); }
        { int n = 0; for (int g = 0; g < AG_NGAME; g++) if (arc.pm & (1 << g)) n++; char s[40]; snprintf(s, sizeof s, "GAMES TRIED %d OF %d. PLAYED %d TIMES", n, AG_NGAME, arc.plays); ui_textc(160, 170, 1, s, UI_RGB(170, 200, 250)); }
    } else {
        gfx_textc(160, 3, 2, "MEDALS", 255, 255, 255);
        for (int i = 0; i < 20; i++) { int x = 12 + (i & 1) * 154, y = 28 + (i >> 1) * 17, on = (arc.ach >> i) & 1; ui_round(x, y, 148, 15, 4, on ? UI_RGB(250, 210, 60) : UI_RGB(30, 40, 80), on ? UI_RGB(240, 150, 30) : UI_RGB(20, 28, 60), 0); ui_text(x + 6, y + 4, 1, medal_nm[i], on ? UI_INK : UI_RGB(130, 140, 180)); }
    }
    ui_hintbar(218, "L R PAGE|O BACK");
}
static void draw_chal(void) {
    ui_round(40, 56, 240, 128, 10, UI_RGB(30, 20, 60), UI_RGB(16, 10, 36), 0);
    static const char *const rn[4] = { "YOUR FRIEND", "YOUR RIVAL", "YOUR FAMILY", "A LOCAL" };
    char s[64]; snprintf(s, sizeof s, "%s %s", rn[wk[ch.w].role & 3], ch.nm); gfx_textc(160, 64, 2, s, 255, 220, 120);
    snprintf(s, sizeof s, "CHALLENGES YOU TO %s", gi[ch.game].nm); ui_textc(160, 92, 1, s, UI_WHITE);
    snprintf(s, sizeof s, "BEAT THEIR SCORE OF %d", ch.target); ui_textc(160, 108, 1, s, UI_RGB(200, 210, 250));
    snprintf(s, sizeof s, "%d TICKETS ON IT. THE TOKEN IS ON THEM", ch.stake); ui_textc(160, 124, 1, s, UI_GREEN);
    ui_textc(160, 142, 1, wk[ch.w].role == 2 ? "FAMILY TIME. EVERYONE WINS A LITTLE" : wk[ch.w].role == 1 ? "A RIVAL. WIN AND THEY SULK" : "A FRIENDLY GAME", UI_RGB(255, 200, 220));
    ui_hintbar(166, "X ACCEPT|O NOT NOW");
}
void arcade_draw(void) { arun.role = 80;
    if (on_core) { core_draw(); return; }
    switch (mode) {
    case M_FLOOR: case M_CHAL: {
        floor_draw();
        int c = near_cab(), k = near_kiosk(); char s[56] = "";
        if (mode == M_FLOOR) {
            if (c >= 0) snprintf(s, sizeof s, "X PLAY %s", gi[c].nm); else if (k == 0) snprintf(s, sizeof s, "X BUY TOKENS"); else if (k == 1) snprintf(s, sizeof s, "X HIGH SCORES AND MEDALS"); else if (k == 2) snprintf(s, sizeof s, "X THE PRIZE COUNTER");
            for (int i = 0; i < 6; i++) if ((wk[i].st == 7 || wk[i].st == 6) && ABS(wk[i].x - fx) < 36 && ABS(wk[i].y - fy) < 14 && ch.on) snprintf(s, sizeof s, "X ANSWER %s", ch.nm);
            K(0, 224, 320, 16, UI_RGB(10, 6, 24)); ui_textc(160, 228, 1, s[0] ? s : "D-PAD WALK. O LEAVE", s[0] ? UI_AMBER : UI_RGB(170, 170, 210));
            if (pmsg_t > 0 && pmsg) ui_textc(160, 212, 1, pmsg, UI_RED);
        }
        if (mode == M_CHAL) draw_chal();
        break; }
    case M_PRIZE: draw_prize(); break;
    case M_TOKEN: draw_token(); break;
    case M_BOARD: draw_board(); break;
    }
}

// the overlay's head (ovl.h)
#include "ovl.h"
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, arcade_open, arcade_update, arcade_draw };
