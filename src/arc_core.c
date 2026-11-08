// arc_core.c - what both ARCADE overlays share: the table of games, the sparks, the ticket formula, the high-score tables and medals, and a
// cabinet's flow (rules page and attract screen, play, game over, initials). The floor is arcade.c; the games are ag_*.c.
#include "arc_i.h"

// ---- what each game is ----
const GInfo gi[AG_NGAME] = {
    { "BORTUA FIGHTER", "FGT", "D-PAD MOVE. X PUNCH. O KICK", "BEAT EIGHT FIGHTERS AND THE BOSS", "EMERGENCY (ORIGINAL)", 2, 60, 0, 230, 60, 70 },
    { "BORE BREAKER", "BRK", "LEFT RIGHT MOVE. X SERVES", "BREAK THE WALL. CATCH POWER-UPS", "AMIGA MUSIC", 1, 120, 0, 60, 200, 250 },
    { "SNACK CATCH", "CAT", "LEFT RIGHT MOVE THE BASKET", "CATCH SNACKS. DODGE ROCKS", "SUNMAN SUNRISE", 1, 60, 0, 250, 170, 50 },
    { "WHACK-A-BORE", "WAK", "D-PAD PICKS A HOLE. X WHACKS", "GOLD BORES PAY MORE. NO BOMBS", "MI CORA ZONE", 1, 40, 0, 120, 220, 90 },
    { "HIGH STRIKER", "STR", "X STOPS THE METER", "HIT THE SWEET SPOT. RING THE BELL", "HOT DAMN", 1, 400, 0, 250, 120, 40 },
    { "BORE BLOCKS", "BLK", "D-PAD MOVE. X TURN. UP DROPS", "FILL LINES. THE FLOOR RISES", "WHISTLER MAN", 1, 600, 0, 170, 90, 250 },
    { "STAR BORE", "SHM", "D-PAD FLY. X SHOOT. O BOMB", "SHOOT THE FLEET. BEAT THE BOSS", "AIM AND SHOOT", 2, 2000, 0, 250, 80, 170 },
    { "TURBO BORE", "RAC", "LEFT RIGHT STEER. X GAS. O NITRO", "BEAT THE CLOCK. DODGE THE TRAFFIC", "EMERGENCY ON THE DANCE FLOOR", 2, 3000, 0, 250, 220, 50 },
    { "MUNCH MAZE", "MZE", "D-PAD TURNS. EAT EVERY DOT", "BIG DOTS MAKE GHOSTS SCARED", "THE CYNICAL SYNDICATION", 1, 2000, 0, 250, 230, 60 },
    { "HOOPS", "HOO", "LEFT RIGHT MOVE. X STOPS THE METER", "SINK IT. KEEP A STREAK. GET HOT", "GOTTCHO BARRACHO", 1, 40, 0, 250, 130, 60 },
    { "SKEE-BORE", "SKE", "LEFT RIGHT AIM. HOLD X, LET GO", "ROLL FOR THE HIGH RINGS", "THE MAGIC ACT", 1, 260, 0, 90, 220, 160 },
    { "AIR HOCKEY", "HOC", "D-PAD MOVES YOUR MALLET", "FIRST TO SEVEN. TWO PADS FOR TWO", "WORTHLESS CLOUDS", 2, 160, 1, 90, 160, 255 },
    { "BEAT BORE", "RHY", "D-PAD OR X O TRI SQ ON THE LINE", "KEEP THE GROOVE UP. BUILD A COMBO", "HOT DAMN (ORIGINAL)", 1, 6000, 0, 250, 90, 220 },
    { "CLAW GRAB", "CLW", "LEFT RIGHT MOVE. X DROPS", "GRAB A PLUSH AND BRING IT HOME", "NURSERY TIME", 1, 12, 0, 255, 120, 190 },
};
const u8 logic_game[AG_NGAME] = { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1 };   // the thinking games (LOGIC); the others are reflexes

// ---- shared bits ----
static u32 arng = 0x9E3779B9u;
int ar8(void) { arng = arng * 1103515245u + 12345u; return (int)((arng >> 16) & 255); }
int ag_pts, ag_t, ag_lives, ag_combo, ag_best, ag_perf, ag_np, ag_foe, ag_shake, ag_level;
static int cr_(u32 c) { return c & 255; } static int cg_(u32 c) { return (c >> 8) & 255; } static int cb_(u32 c) { return (c >> 16) & 255; }
__attribute__((noinline)) void K(int x, int y, int w, int h, u32 c) { gfx_rect(x, y, w, h, cr_(c), cg_(c), cb_(c), 0); }
__attribute__((noinline)) void KA(int x, int y, int w, int h, u32 c) { gfx_rect(x, y, w, h, cr_(c), cg_(c), cb_(c), 1); }
__attribute__((noinline)) void RC(int x, int y, int w, int h, u32 c) { gfx_rect(FX(x), FY(y), MAX(1, w * 5 / 4), MAX(1, h * 5 / 4), cr_(c), cg_(c), cb_(c), 0); }
__attribute__((noinline)) void RA(int x, int y, int w, int h, u32 c) { gfx_rect(FX(x), FY(y), MAX(1, w * 5 / 4), MAX(1, h * 5 / 4), cr_(c), cg_(c), cb_(c), 1); }
void DSC(int cx, int cy, int r, u32 c) { ui_disc(FX(cx), FY(cy), r * 5 / 2, c); }
void TX(int x, int y, const char *s, u32 c) { ui_text(FX(x), FY(y), 1, s, c); }
void TXC(int cx, int y, const char *s, u32 c) { ui_textc(FX(cx), FY(y), 1, s, c); }
void TXB(int cx, int y, const char *s, u32 c) { gfx_textc(FX(cx), FY(y), 2, s, cr_(c), cg_(c), cb_(c)); }
void NUM(int x, int y, const char *l, int v, u32 c) { char s[24]; snprintf(s, sizeof s, "%s %d", l, v); TX(x, y, s, c); }
void cue(int k) {
    static const u8 v[5] = { VOICE_AGREE, VOICE_NAH, VOICE_YEHA, VOICE_WIN, VOICE_GROAN };
    audio_voice(v[k % 5]);
}
// sparks and floating numbers
static struct { s16 x, y, vx, vy; u8 life; u32 c; } pt[40];
void part(int x, int y, int vx, int vy, int life, u32 c) {
    for (int i = 0; i < 40; i++) if (!pt[i].life) { pt[i].x = (s16)(x * 16); pt[i].y = (s16)(y * 16); pt[i].vx = (s16)vx; pt[i].vy = (s16)vy; pt[i].life = (u8)life; pt[i].c = c; return; }
}
void burst(int x, int y, int n, u32 c) { for (int i = 0; i < n; i++) part(x, y, (ar8() - 128) / 5, (ar8() - 160) / 5, 10 + ar8() % 10, c); }
static struct { s16 x, y; u8 t; u32 c; char s[10]; } pp[6];
void pop(int x, int y, const char *s, u32 c) {
    int k = 0; for (int i = 0; i < 6; i++) if (!pp[i].t) { k = i; break; }
    pp[k].x = (s16)x; pp[k].y = (s16)y; pp[k].t = 26; pp[k].c = c; strncpy(pp[k].s, s, 9); pp[k].s[9] = 0;
}
void core_fx_tick(void) {
    for (int i = 0; i < 40; i++) if (pt[i].life) { pt[i].life--; pt[i].x += pt[i].vx; pt[i].y += pt[i].vy; pt[i].vy += 2; }
    for (int i = 0; i < 6; i++) if (pp[i].t) { pp[i].t--; if (pp[i].t & 1) pp[i].y--; }
    if (ag_shake > 0) ag_shake--;
}
void core_fx_draw(void) {
    for (int i = 0; i < 40; i++) if (pt[i].life) RC(pt[i].x >> 4, pt[i].y >> 4, 2, 2, pt[i].c);
    for (int i = 0; i < 6; i++) if (pp[i].t) TXC(pp[i].x, pp[i].y, pp[i].s, pp[i].t > 6 ? pp[i].c : UI_RGB(120, 120, 140));
}

// ---- the saved block: tables, medals ----
void ini_str(u32 e, char *o) { for (int i = 0; i < 3; i++) { int v = (e >> (17 + i * 5)) & 31; o[i] = (char)(v >= 1 && v <= 26 ? 'A' + v - 1 : '-'); } o[3] = 0; }
int hs_rank(int g, int sc) { for (int i = 0; i < 3; i++) if (sc > SC(arc.hs[g][i])) return i; return -1; }
void hs_put(int g, int k, int sc, const char *ini) {
    for (int i = 2; i > k; i--) arc.hs[g][i] = arc.hs[g][i - 1];
    u32 e = (u32)MIN(sc, 0x1FFFF); for (int i = 0; i < 3; i++) e |= (u32)((ini[i] - 'A' + 1) & 31) << (17 + i * 5);
    arc.hs[g][k] = e;
}
const char *const medal_nm[20] = { "WELCOME", "FIRST GAME", "25 GAMES", "100 GAMES", "ALL 14 GAMES", "A HIGH SCORE", "HALF THE WALL", "JACKPOT 40", "FIRST PRIZE",
    "NEW OUTFIT", "A PET", "AN EGG", "WON A CHALLENGE", "BEAT A RIVAL", "FAMILY FUN", "COMBO OF 15", "FLAWLESS", "TWO PLAYER WIN", "REFLEXES 5", "LOGIC 5" };
void medal(int b) {
    if (arc.ach & (1u << b)) return;
    arc.ach |= 1u << b; arcade_tix = MIN(9999, arcade_tix + 25);
    char s[40]; snprintf(s, sizeof s, "MEDAL: %s! +25 TICKETS", medal_nm[b]); sim_note(s); audio_voice(VOICE_YEHA);
}


// ---- a cabinet's flow ----
enum { C_RULES, C_PLAY, C_OVER, C_INIT };
int core_game, core_ft; static int cm, acc, rules_t, over_t, tix, hrank, newbest, didwin, ini_c; static char ini[4];
void core_song(int g) { return; int s = g >= 0 ? audio_song_find(gi[g].song) : audio_song_find("HOT DAMN"); if (s >= 0 && s + 1 != arun.cur) { audio_jukebox_play(s); arun.cur = (u8)(s + 1); } }
void need_add(Sim *s, int n, int v) { s->need[n] = CLAMP(s->need[n] + (v << 10), 0, 100 << 10); }
static void begin_game(void) {   // the token is in
    ag_pts = 0; ag_t = 0; ag_lives = 3; ag_combo = 0; ag_best = 0; ag_perf = 1; ag_level = 1; ag_shake = 0; tix = 0; memset(pt, 0, sizeof pt); memset(pp, 0, sizeof pp);
    ag_np = 1; if (gi[core_game].two) { u8 b[8]; if (pad_read(1, b) && b[0] == 0 && (b[1] >> 4) >= 4) ag_np = 2; }
    ag_fn[core_game].start();
    cm = C_PLAY; acc = 0; core_song(core_game);
}
static int tix_calc(int g, int pts) {
    Sim *s = &sims[player]; int t = pts * 12 / gi[g].par;
    if (t > 30) t = 30 + (t - 30) / 2;                       // the big scores count half
    t += ag_best / 4;                                       // the combo
    if (ag_perf && pts > gi[g].par / 2) t += 6;             // no mistakes
    int lv = SKL(logic_game[g] ? s->skill_logic : s->skill_body);
    t += t * lv / 20;                                       // reflexes or logic: up to half as much again
    return CLAMP(t, 0, 80);
}
static void finish(void) {   // a game is over: the tickets, the table, the skills, the mood, the medals
    Sim *s = &sims[player]; char b[56]; int g = core_game;
    tix = tix_calc(g, ag_pts); if (!(arc.pm & (1 << g))) { tix += 10; arc.pm |= (u16)(1 << g); }
    didwin = 0; arun.res = 0;
    if (arun.foe > 0) {   // a challenge: win it for the stake and a bonus
        didwin = ag_pts > arun.foe; arun.res = (u8)(didwin ? 1 : 2);
        if (didwin) tix += arun.stake; else tix = MAX(0, tix - arun.stake);
        if (arun.sim >= 0 && arun.sim < nsims) { int j = arun.sim, d = didwin ? (arun.role == 1 ? -4 : 7) : (arun.role == 1 ? 3 : 4); sims[j].rel[player] = (s8)CLAMP(sims[j].rel[player] + d, -100, 100); s->rel[j] = (s8)CLAMP(s->rel[j] + d, -100, 100); }
        need_add(s, N_SOCIAL, didwin ? 18 : 12); sim_moodlet(s, didwin ? 16 : (arun.role == 1 ? 13 : 15));
        if (didwin) { medal(12); if (arun.role == 1) medal(13); if (arun.role == 2) medal(14); if (ag_np == 2) medal(17); }
    }
    arcade_tix = MIN(9999, arcade_tix + tix);
    need_add(s, N_FUN, MIN(45, 16 + tix / 2)); need_add(s, N_REST, -3); sim_moodlet(s, 15);
    if (logic_game[g]) s->skill_logic += 8; else s->skill_body += 6; s->skill_fun += 6;
    if (SKL(s->skill_body) >= 5) medal(18); if (SKL(s->skill_logic) >= 5) medal(19);
    hrank = hs_rank(g, ag_pts); newbest = hrank == 0;
    if (arc.plays < 60000) arc.plays++;
    medal(1); if (arc.plays >= 25) medal(2); if (arc.plays >= 100) medal(3); if (arc.pm == (1 << AG_NGAME) - 1) medal(4);
    if (tix >= 40) medal(7); if (ag_best >= 15) medal(15); if (ag_perf && ag_pts > gi[g].par) medal(16);
    snprintf(b, sizeof b, "%s: +%d TICKETS (%d IN ALL)", gi[g].nm, tix, arcade_tix); sim_note(b);
    audio_voice(didwin || tix >= 12 ? VOICE_YAHOO : tix ? VOICE_LAUGH : VOICE_NAH);
    cm = C_OVER; over_t = 0;
}
static const char *note1;
void core_open(int g) { core_game = g; cm = C_RULES; rules_t = 0; acc = 0; core_ft = 0; note1 = 0; memset(pt, 0, sizeof pt); memset(pp, 0, sizeof pp); }
static void start_play(int g, int free) {
    int c = gi[g].tok;
    if (!free && arc.tok < c) { static char nb[44]; snprintf(nb, sizeof nb, "YOU NEED %d TOKENS. TRY THE TOKEN MACHINE", c); note1 = nb; audio_voice(VOICE_NAH); ag_shake = 6; return; }
    if (!free) arc.tok = (u16)(arc.tok - c);
    note1 = 0; begin_game();
}
static void ini_begin(void) {
    const char *f = sims[player].bore.first; for (int i = 0; i < 3; i++) { char c = f[i]; ini[i] = (char)((c >= 'A' && c <= 'Z') ? c : (c >= 'a' && c <= 'z') ? c - 32 : 'A' + i); }
    ini[3] = 0; ini_c = 0;
}
static int p2_edge(int *h) {   // the second pad: held, and what was just pressed
    static int prev; u8 b[8]; int hd = 0;
    if (pad_read(1, b) && b[0] == 0 && (b[1] >> 4) >= 4) hd = ~(b[2] | (b[3] << 8)) & 0xffff;
    int pr = hd & ~prev; prev = hd; *h = hd; return pr;
}
static void hs_done(void) { hs_put(core_game, hrank, ag_pts, ini); medal(5); { int n = 0; for (int g = 0; g < AG_NGAME; g++) if (arc.hs[g][0]) n++; if (n >= 7) medal(6); } }
static int tick(int hd, int pr) {   // 0: leave for the floor
    core_fx_tick(); core_ft++;
    switch (cm) {
    case C_RULES:
        rules_t++;
        if (pr & PAD_CROSS) start_play(core_game, arun.foe > 0);
        if (pr & (PAD_CIRCLE | PAD_START)) return 0;
        break;
    case C_PLAY: {
        int h2 = 0, p2 = 0; if (ag_np == 2) p2 = p2_edge(&h2);
        ag_t++;
        if ((pr & PAD_SELECT) && ag_t > 30) { finish(); break; }   // (SELECT ends it and takes what you have)
        if (!ag_fn[core_game].tick(hd, pr, h2, p2)) finish();
        break; }
    case C_OVER:
        over_t++;
        if (over_t > 36 && (pr & PAD_CROSS)) { if (hrank >= 0) { cm = C_INIT; ini_begin(); } else { if (arun.foe > 0) return 0; cm = C_RULES; rules_t = 0; core_song(-1); } }
        break;
    case C_INIT:
        if (pr & PAD_UP) ini[ini_c] = (char)(ini[ini_c] == 'Z' ? 'A' : ini[ini_c] + 1);
        if (pr & PAD_DOWN) ini[ini_c] = (char)(ini[ini_c] == 'A' ? 'Z' : ini[ini_c] - 1);
        if ((pr & PAD_LEFT) && ini_c > 0) ini_c--; if ((pr & PAD_RIGHT) && ini_c < 2) ini_c++;
        if (((pr & PAD_CROSS) && ini_c == 2) || ((pr & PAD_CIRCLE) && ini_c == 0)) { hs_done(); if (arun.foe > 0) return 0; cm = C_RULES; rules_t = 0; core_song(-1); }
        else if (pr & PAD_CROSS) ini_c++; else if ((pr & PAD_CIRCLE) && ini_c > 0) ini_c--;
        break;
    }
    return 1;
}
int core_update(int hd, int pr, int steps) {
    if (cm == C_PLAY && core_game == AG_FIGHTER) {   // (the 3D fight keeps its own time)
        if (!ag_fn[0].tick(hd, pr, steps, 0)) finish();
        return 1;
    }
    acc += steps;
    while (acc >= 2) { acc -= 2; if (!tick(hd, pr)) return 0; pr = 0; }
    return 1;
}

// ---- the picture ----
void bezel(int g) {
    const GInfo *a = &gi[g]; int r = a->r, gg = a->g, b = a->b;
    gfx_set_clear(r / 12, gg / 12, b / 12 + 4);
    K(0, 0, SCREEN_W, SCREEN_H, UI_RGB(r / 10 + 8, gg / 10 + 6, b / 10 + 14));
    K(0, 0, SCREEN_W, 17, UI_RGB(r / 2, gg / 2, b / 2)); K(0, 0, SCREEN_W, 2, UI_RGB(r, gg, b)); K(0, 15, SCREEN_W, 2, UI_RGB(r, gg, b));
    ui_textc(160, 4, 1, a->nm, UI_WHITE);
    { char s[16]; snprintf(s, sizeof s, "TOKENS %d", arc.tok); ui_text(6, 4, 1, s, UI_AMBER); snprintf(s, sizeof s, "TIX %d", arcade_tix); ui_text(314 - ui_tw(1, s), 4, 1, s, UI_GREEN); }
    K(FX(0) - 3, FY(0) - 3, 306, 206, UI_RGB(r, gg, b));   // the screen's glowing edge
}
void scan(void) { for (int y = FY(0); y < FY(160); y += 3) KA(FX(0), y, 300, 1, 0); }
static void draw_rules(void) {
    const GInfo *a = &gi[core_game]; int ph = (rules_t / 100) % 3;
    gfx_textc(160, 36, 2, a->nm, a->r, a->g, a->b);
    K(100, 62, 120, 84, UI_RGB(0, 0, 0)); K(102, 64, 116, 80, UI_RGB(8, 8, 24));
    if (ph == 0) { ui_textc(160, 80, 1, a->h1, UI_WHITE); ui_textc(160, 96, 1, a->h2, UI_RGB(200, 210, 240)); { char s[40]; snprintf(s, sizeof s, "COSTS %d TOKEN%s", a->tok, a->tok > 1 ? "S" : ""); ui_textc(160, 118, 1, s, UI_AMBER); } }
    else if (ph == 1) { ui_textc(160, 68, 1, "HIGH SCORES", UI_AMBER); for (int i = 0; i < 3; i++) { char s[28], n[4]; ini_str(arc.hs[core_game][i], n); if (SC(arc.hs[core_game][i])) snprintf(s, sizeof s, "%d  %s  %d", i + 1, n, SC(arc.hs[core_game][i])); else snprintf(s, sizeof s, "%d  ---  0", i + 1); ui_textc(160, 86 + i * 16, 1, s, i == 0 ? UI_AMBER : UI_WHITE); } }
    else { ui_textc(160, 70, 1, "TICKETS", UI_GREEN); { char s[40]; snprintf(s, sizeof s, "SCORE %d = ABOUT 12 TICKETS", a->par); ui_textc(160, 88, 1, s, UI_WHITE); ui_textc(160, 102, 1, "COMBOS AND NO MISTAKES PAY MORE", UI_RGB(200, 210, 240)); ui_textc(160, 118, 1, logic_game[core_game] ? "A THINKING GAME: LOGIC SKILL" : "A REFLEX GAME: BODY SKILL", UI_RGB(170, 210, 250)); } }
    if (arun.foe > 0) { char s[48]; snprintf(s, sizeof s, "%s CHALLENGES YOU: BEAT %d", arun.nm, arun.foe); ui_textc(160, 154, 1, s, UI_RGB(255, 200, 120)); }
    if ((rules_t >> 4) & 1) ui_textc(160, 172, 1, "INSERT TOKEN", UI_AMBER); else ui_textc(160, 172, 1, arun.foe > 0 ? "X TO PLAY. THE TOKEN IS ON THEM" : "X TO PLAY", UI_WHITE);
    if (note1) ui_textc(160, 188, 1, note1, UI_RED);
    ui_hintbar(204, "X PLAY|O THE FLOOR");
}
static void draw_over(void) {
    ui_round(60, 62, 200, 116, 8, UI_RGB(20, 22, 44), UI_RGB(10, 12, 30), 0);
    gfx_textc(160, 68, 2, "GAME OVER", 250, 210, 50);
    char s[44]; snprintf(s, sizeof s, "SCORE %d", ag_pts); ui_textc(160, 92, 1, s, UI_WHITE);
    if (newbest) ui_textc(160, 104, 1, "NEW BEST!", UI_AMBER); else if (hrank >= 0) ui_textc(160, 104, 1, "ON THE TABLE!", UI_AMBER);
    snprintf(s, sizeof s, "+%d TICKETS", tix); ui_textc(160, 118, 1, s, UI_GREEN);
    if (ag_best >= 4) { snprintf(s, sizeof s, "BEST COMBO %d", ag_best); ui_textc(160, 130, 1, s, UI_RGB(170, 210, 250)); }
    if (arun.foe > 0) { snprintf(s, sizeof s, didwin ? "YOU BEAT %s! +%d" : "%s WINS. -%d", arun.nm, arun.stake); ui_textc(160, 146, 1, s, didwin ? UI_GREEN : UI_RED); }
    if (over_t > 36) ui_hintbar(166, hrank >= 0 ? "X ENTER YOUR INITIALS" : "X CONTINUE");
}
static void draw_init(void) {
    ui_round(70, 70, 180, 100, 8, UI_RGB(20, 22, 44), UI_RGB(10, 12, 30), 0);
    ui_textc(160, 76, 1, hrank == 0 ? "THE NEW CHAMPION!" : "A HIGH SCORE!", UI_AMBER);
    { char s[24]; snprintf(s, sizeof s, "%d", ag_pts); gfx_textc(160, 90, 2, s, 255, 255, 255); }
    for (int i = 0; i < 3; i++) { char c[2] = { ini[i], 0 }; int x = 130 + i * 24; K(x, 118, 18, 22, i == ini_c ? UI_RGB(250, 200, 60) : UI_RGB(60, 70, 110)); K(x + 1, 119, 16, 20, UI_RGB(10, 10, 30)); gfx_textc(x + 9, 121, 2, c, 255, 255, 255); }
    ui_hintbar(176, "UP DOWN LETTER|X NEXT");
}
void core_draw(void) {
    if (cm == C_PLAY && core_game == AG_FIGHTER) { ag_fn[0].draw(); return; }
    bezel(core_game); R(0, 0, 240, 160, 3, 3, 14);
    if (cm == C_RULES) draw_rules();
    else {
        ag_fn[core_game].draw(); core_fx_draw();
        if (arun.foe > 0) { char s[40]; snprintf(s, sizeof s, "VS %s %d", arun.nm, arun.foe); TXC(120, 150, s, ag_pts > arun.foe ? UI_GREEN : UI_AMBER); }
        if (cm == C_OVER) draw_over(); if (cm == C_INIT) draw_init();
    }
    scan();
}
