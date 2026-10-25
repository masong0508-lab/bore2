// arcade.c - the cabinets' games and the prize counter: see arcade.h. The little games are played on a cabinet's screen (a 240 x 160
// field, as on the GBA, drawn half again as big inside a bezel), at 30 ticks a second.
#include "arcade.h"
#include "fight.h"
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "wants.h"

#define gName arcade_gname
static const char *const gHow1[AG_NGAME] = { "", "LEFT RIGHT MOVE THE PADDLE", "LEFT RIGHT MOVE THE BASKET", "THE D-PAD PICKS A HOLE", "X STOPS THE METER" };
static const char *const gHow2[AG_NGAME] = { "", "X SERVES. CLEAR THE WALL", "CATCH SNACKS, DODGE ROCKS", "X WHACKS THE BORE", "FIVE SWINGS: RING THE BELL" };
#define gPrice arcade_gprice

static u32 arng = 0x9E3779B9u;
static int r8(void) { arng = arng * 1103515245u + 12345u; return (int)((arng >> 16) & 255); }
static int game, phase, acc, pts, tix, t, lives, quit;   // phase 0 the rules card, 1 playing, 2 game over
// the field: 240 x 160 drawn at 4/3 inside the bezel
#define FX(x) (16 + (x) * 4 / 3)
#define FY(y) (22 + (y) * 4 / 3)
static void R(int x, int y, int w, int h, int r, int g, int b) { gfx_rect(FX(x), FY(y), MAX(1, w * 4 / 3), MAX(1, h * 4 / 3), r, g, b, 0); }
static void DSC(int cx, int cy, int r, u32 c) { ui_disc(FX(cx), FY(cy), r * 8 / 3, c); }

// ---- BORE BREAKER ----
static u8 br[4][10]; static int left, px, bx, by, vx, vy, held_ball;
// ---- SNACK CATCH ----
static s16 ix[6], iy[6]; static u8 ik[6], on[6]; static int bkx;
// ---- WHACK-A-BORE ----
static int sel, mole, mt, gap, hit;
// ---- HIGH STRIKER ----
static int tr, pos, dir, stopT;

static void start(void) {
    pts = 0; tix = 0; t = 0; lives = 3; quit = 0;
    switch (game) {
    case AG_BREAKER: for (int r = 0; r < 4; r++) for (int c = 0; c < 10; c++) br[r][c] = 1; left = 40; px = 102; held_ball = 1; break;
    case AG_CATCH: memset(on, 0, sizeof on); bkx = 106; break;
    case AG_WHACK: sel = 4; mole = -1; gap = 10; hit = 0; break;
    case AG_STRIKER: tr = 0; pos = 0; dir = 1; stopT = 0; break;
    }
}
static int tick(int hd, int pr) {   // one 30 Hz tick of the game; 0 when it is over
    if (pr & PAD_START) return 0;
    switch (game) {
    case AG_BREAKER:
        if (hd & PAD_LEFT) px -= 5; if (hd & PAD_RIGHT) px += 5; px = CLAMP(px, 20, 184);
        if (held_ball) { bx = px + 16; by = 138; if (pr & PAD_CROSS) { held_ball = 0; vx = (r8() & 1) ? 2 : -2; vy = -3; } }
        else {
            bx += vx; by += vy;
            if (bx < 20) { bx = 20; vx = -vx; } if (bx > 216) { bx = 216; vx = -vx; } if (by < 13) { by = 13; vy = -vy; }
            if (vy > 0 && by >= 140 && by <= 146 && bx + 4 >= px && bx <= px + 36) { vy = -3; vx = CLAMP((bx + 2 - (px + 18)) / 5, -3, 3); if (!vx) vx = (r8() & 1) ? 1 : -1; by = 138; }
            else if (by >= 160) { lives--; held_ball = 1; audio_voice(VOICE_NAH); }
            { int c = (bx + 2 - 20) / 20, r = (by + 2 - 24) / 9; if (bx + 2 >= 20 && by + 2 >= 24 && r < 4 && c < 10 && br[r][c]) { br[r][c] = 0; left--; pts++; vy = -vy; } }
        }
        if (lives <= 0 || left <= 0) { tix = pts / 2 + (left == 0 ? 8 : 0); return 0; }
        return 1;
    case AG_CATCH:
        if (hd & PAD_LEFT) bkx -= 6; if (hd & PAD_RIGHT) bkx += 6; bkx = CLAMP(bkx, 4, 208);
        if (t % 12 == 0) for (int i = 0; i < 6; i++) if (!on[i]) { on[i] = 1; ix[i] = (s16)(8 + r8() % 216); iy[i] = 14; ik[i] = (u8)((r8() % 4 == 0) ? 2 : (r8() & 1)); break; }
        for (int i = 0; i < 6; i++) if (on[i]) {
            iy[i] = (s16)(iy[i] + 3 + t / 200);
            if (iy[i] >= 134 && iy[i] <= 146 && ix[i] + 8 >= bkx && ix[i] <= bkx + 28) { on[i] = 0; if (ik[i] == 2) { lives--; audio_voice(VOICE_GROAN); } else pts++; }
            else if (iy[i] >= 160) on[i] = 0; }
        if (++t >= 600 || lives <= 0) { tix = pts / 2; return 0; }
        return 1;
    case AG_WHACK:
        if ((pr & PAD_LEFT) && sel % 3 > 0) sel--; if ((pr & PAD_RIGHT) && sel % 3 < 2) sel++;
        if ((pr & PAD_UP) && sel > 2) sel -= 3; if ((pr & PAD_DOWN) && sel < 6) sel += 3;
        if (mole < 0) { if (--gap <= 0) { mole = r8() % 9; mt = 26 - MIN(12, pts); } }
        else if (--mt <= 0) { mole = -1; gap = 6 + (r8() & 7); }
        if (hit > 0) hit--;
        if (pr & PAD_CROSS) { if (mole == sel) { pts++; mole = -1; gap = 5 + (r8() & 7); hit = 6; } }
        if (++t >= 900) { tix = pts * 2 / 3; return 0; }
        return 1;
    case AG_STRIKER:
        if (stopT) { if (--stopT == 0) { if (++tr >= 5) { tix = pts / 45; return 0; } pos = 0; dir = 1; } return 1; }
        if (pr & PAD_CROSS) { pts += pos; stopT = 30; if (pos >= 95) audio_voice(VOICE_YEHA); return 1; }
        pos += dir * (4 + tr * 2); if (pos >= 100) { pos = 100; dir = -1; } if (pos <= 0) { pos = 0; dir = 1; }
        return 1;
    }
    return 0;
}

// ---- the prize counter ----
typedef struct { const char *nm; u16 cost; s8 fun, food, social; const char *say; } Prize;
static const Prize prz[] = {
    { "CANDY", 10, 5, 15, 0, "SUGAR RUSH!" }, { "BOUNCY BALL", 25, 15, 0, 5, "BOING BOING BOING" }, { "PLUSH TEDDY", 60, 25, 0, 10, "IT HAS A TINY HAT" },
    { "MEGA TROPHY", 150, 30, 0, 30, "NUMBER ONE AT SOMETHING" }, { "GOLDEN TICKET", 400, 10, 0, 0, "TRADED IN FOR $500!" } };
#define NPRZ ((int)(sizeof prz / sizeof prz[0]))
static int psel;

void arcade_open(int g) {
    game = g; phase = 0; acc = 0; psel = 0;
    if (g == AG_FIGHTER) fight_open(1);
}
static void need_add(Sim *s, int n, int v) { s->need[n] = CLAMP(s->need[n] + (v << 10), 0, 100 << 10); }
static void finish(void) {   // a game over: the tickets, a little fun, a little sleepier
    Sim *s = &sims[player]; char b[48];
    arcade_tix = MIN(9999, arcade_tix + tix);
    need_add(s, N_FUN, 18 + tix); need_add(s, N_REST, -3);
    s->skill_fun += 6; s->skill_body += game == AG_WHACK || game == AG_STRIKER ? 6 : 0;
    snprintf(b, sizeof b, "%s: +%d TICKETS (%d IN ALL)", gName[game], tix, arcade_tix); sim_note(b);
    audio_voice(tix >= 8 ? VOICE_YAHOO : tix ? VOICE_LAUGH : VOICE_NAH);
}
int arcade_update(int hd, int pr, int steps) {
    if (game == AG_FIGHTER) {
        if (fight_update(hd, pr, steps)) return 1;
        fight_close();
        { Sim *s = &sims[player]; extern int fight_last_wins, fight_matches; if (fight_matches) { int w = fight_last_wins; tix = w * 6 + (fight_champ ? 0 : 0);
          arcade_tix = MIN(9999, arcade_tix + tix); need_add(s, N_FUN, 30 + w * 5); need_add(s, N_REST, -6); s->skill_body += 10 + w * 8;
          char b[48]; snprintf(b, sizeof b, w ? "BORTUA FIGHTER: %d WON, +%d TICKETS" : "BORTUA FIGHTER: BEATEN. NEXT TIME!", w, tix); sim_note(b); } }
        return 0;
    }
    if (game == AG_PRIZES) {
        if (pr & PAD_UP) psel = (psel + NPRZ - 1) % NPRZ;
        if (pr & PAD_DOWN) psel = (psel + 1) % NPRZ;
        if (pr & PAD_CIRCLE) return 0;
        if (pr & PAD_CROSS) {
            const Prize *p = &prz[psel];
            if (arcade_tix < p->cost) { sim_note("NOT ENOUGH TICKETS YET"); audio_voice(VOICE_NAH); return 1; }
            Sim *s = &sims[player]; arcade_tix -= p->cost;
            need_add(s, N_FUN, p->fun); need_add(s, N_FOOD, p->food); need_add(s, N_SOCIAL, p->social);
            if (psel == NPRZ - 1) cash += 500;
            { char b[48]; snprintf(b, sizeof b, "%s: %s", p->nm, p->say); sim_note(b); } audio_voice(VOICE_YAHOO);
            return 0;
        }
        return 1;
    }
    if (phase == 0) { if (pr & PAD_CROSS) { phase = 1; start(); } if (pr & PAD_CIRCLE) { cash += gPrice[game]; return 0; } return 1; }   // (backing out: the money back)
    if (phase == 2) { acc += steps; if (acc > 40 && (pr & (PAD_CROSS | PAD_CIRCLE))) { finish(); return 0; } return 1; }
    acc += steps;
    while (acc >= 2) { acc -= 2; int k = tick(hd, pr); pr = 0; if (!k) { phase = 2; acc = 0; break; } }
    return 1;
}

// ---- the picture: a cabinet's screen, bezel and all ----
static void bezel(const char *title) {
    gfx_set_clear(6, 4, 14);
    gfx_rect(0, 0, SCREEN_W, SCREEN_H, 16, 10, 34, 0);
    ui_round(8, 2, 304, 18, 6, UI_RGB(60, 30, 110), UI_RGB(30, 14, 64), 0);
    gfx_textc(160, 5, 1, title, 90, 230, 240);
}
static void field_back(void) { R(0, 0, 240, 160, 4, 4, 18); }
static void scan(void) { for (int y = FY(0); y < FY(160); y += 3) gfx_rect(FX(0), y, 320, 1, 0, 0, 0, 1); }
static void hud2(const char *a, int n, const char *b, int m) {
    char s[24]; snprintf(s, sizeof s, "%s %d", a, n); ui_text(FX(4), FY(2), 1, s, UI_WHITE);
    snprintf(s, sizeof s, "%s %d", b, m); ui_text(FX(236) - ui_tw(1, s), FY(2), 1, s, UI_AMBER);
}
static void tix_line(void) { char b[32]; snprintf(b, sizeof b, "TICKETS %d", arcade_tix); ui_textc(160, 222, 1, b, UI_RGB(200, 210, 240)); }
void arcade_draw(void) {
    if (game == AG_FIGHTER) { fight_draw(); return; }
    if (game == AG_PRIZES) {
        bezel("THE PRIZE COUNTER");
        int top = ui_window(40, 28, 240, 186, "SWAP YOUR TICKETS", IC_STAR);
        for (int i = 0; i < NPRZ; i++) { int y = top + i * 24, s = i == psel;
            if (s) ui_round(46, y - 2, 228, 22, 6, UI_RGB(226, 242, 254), UI_HILITE, 0);
            ui_text(54, y + 3, 1, prz[i].nm, s ? UI_BLUE2 : UI_INK);
            char c[16]; snprintf(c, sizeof c, "%d TIX", prz[i].cost); ui_text(266 - ui_tw(1, c), y + 3, 1, c, arcade_tix >= prz[i].cost ? UI_GOODT : UI_BADT); }
        { char b[40]; snprintf(b, sizeof b, "YOU HAVE %d TICKETS", arcade_tix); ui_textc(160, top + NPRZ * 24 + 6, 1, b, UI_SOFT); }
        ui_hintbar(220, "DPAD PICK|X SWAP|O NEVER MIND");
        return;
    }
    bezel(gName[game]);
    field_back();
    if (phase == 0) {
        gfx_textc(160, 70, 2, gName[game], 255, 210, 50);
        ui_textc(160, 116, 1, gHow1[game], UI_WHITE); ui_textc(160, 132, 1, gHow2[game], UI_WHITE);
        ui_hintbar(196, "X PLAY|O MONEY BACK"); scan(); tix_line(); return;
    }
    switch (game) {
    case AG_BREAKER: {
        static const u8 rc[4][3] = { { 250, 50, 50 }, { 250, 146, 32 }, { 226, 226, 32 }, { 50, 210, 80 } };
        hud2("SCORE", pts, "LIVES", lives);
        for (int r = 0; r < 4; r++) for (int c = 0; c < 10; c++) if (br[r][c]) R(20 + c * 20 + 1, 24 + r * 9 + 1, 18, 7, rc[r][0], rc[r][1], rc[r][2]);
        R(px, 144, 36, 5, 32, 226, 240); R(px, 144, 36, 1, 255, 255, 255); R(bx, by, 4, 4, 255, 255, 255);
        if (held_ball && phase == 1) ui_textc(160, FY(110), 1, "X SERVES", UI_RGB(170, 210, 250));
        break; }
    case AG_CATCH:
        hud2("SNACKS", pts, "LIVES", lives);
        { char s[16]; snprintf(s, sizeof s, "TIME %d", MAX(0, 600 - t) / 30); ui_textc(160, FY(2), 1, s, UI_RGB(170, 210, 250)); }
        for (int i = 0; i < 6; i++) if (on[i]) { if (ik[i] == 2) R(ix[i], iy[i], 8, 8, 110, 110, 120); else DSC(ix[i] + 4, iy[i] + 4, 4, ik[i] ? UI_RGB(250, 210, 32) : UI_RGB(250, 80, 160)); }
        R(bkx, 142, 28, 8, 192, 112, 32); R(bkx, 142, 28, 2, 250, 176, 64);
        break;
    case AG_WHACK:
        hud2("HITS", pts, "TIME", MAX(0, 900 - t) / 30);
        for (int i = 0; i < 9; i++) { int cx = 60 + (i % 3) * 60, cy = 44 + (i / 3) * 38;
            DSC(cx, cy + 6, 16, UI_RGB(8, 8, 16)); DSC(cx, cy + 4, 15, UI_RGB(40, 24, 16));
            if (i == mole) { DSC(cx, cy - 4, 11, UI_RGB(160, 96, 48)); R(cx - 5, cy - 7, 3, 3, 255, 255, 255); R(cx + 3, cy - 7, 3, 3, 255, 255, 255); R(cx - 3, cy + 1, 7, 2, 8, 8, 16); }
            if (i == sel) { R(cx - 19, cy - 17, 38, 1, 250, 210, 40); R(cx - 19, cy + 19, 38, 1, 250, 210, 40); R(cx - 19, cy - 17, 1, 37, 250, 210, 40); R(cx + 18, cy - 17, 1, 37, 250, 210, 40); } }
        if (hit > 0) gfx_textc(160, FY(140), 2, "BONK!", 250, 210, 40);
        break;
    case AG_STRIKER:
        hud2("POINTS", pts, "SWING", MIN(tr + 1, 5));
        R(106, 22, 28, 120, 48, 48, 80); R(108, 24, 24, 116, 16, 16, 32);
        { int h = pos * 116 / 100; if (pos >= 95) R(108, 140 - h, 24, h, 250, 210, 32); else if (pos >= 60) R(108, 140 - h, 24, h, 250, 112, 32); else R(108, 140 - h, 24, h, 48, 192, 80); }
        DSC(120, 17, 5, UI_RGB(250, 210, 40));
        if (stopT) gfx_textc(160, FY(146), 2, pos >= 95 ? "DING!" : pos >= 60 ? "NOT BAD" : "WEAK!", 255, 255, 255);
        break;
    }
    if (phase == 2) {
        ui_round(80, 80, 160, 70, 8, UI_RGB(20, 22, 44), UI_RGB(10, 12, 30), 0);
        gfx_textc(160, 88, 2, "GAME OVER", 250, 210, 40);
        char s[32]; snprintf(s, sizeof s, "SCORE %d", pts); ui_textc(160, 112, 1, s, UI_WHITE);
        snprintf(s, sizeof s, "+%d TICKETS", tix); ui_textc(160, 128, 1, s, UI_GREEN);
        if (acc > 40) ui_hintbar(196, "X DONE");
    }
    scan(); tix_line();
}

// the overlay's head (ovl.h)
#include "ovl.h"
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, arcade_open, arcade_update, arcade_draw };
