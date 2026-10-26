// prison_ui: HARD TIME, an overlay (PRISON.OVL). You are arrested (a fight with the police, a shoplift gone wrong) and serve your sentence a day at a
// time: three slots a day (morning, afternoon, evening), each an activity. Five numbers matter: HEALTH, RESPECT (the yard's), HEAT (the guards' eyes on you),
// BEHAVIOUR (for parole) and the ESCAPE plan. Good behaviour earns parole; a plan, respect and nerve can get you out the hard way.
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "ovl.h"
#include <psxgpu.h>
#include <psxpad.h>
#include <stdio.h>
#include <string.h>

enum { A_WORK, A_JOB, A_LIB, A_YARD, A_TRADE, A_DIG, A_FIGHT, A_REST, A_LETTER, A_PAROLE, A_ESCAPE, A_N };
static const char *const ANM[A_N] = { "WORK OUT", "PRISON JOB", "THE LIBRARY", "YARD TIME", "TRADE CONTRABAND", "DIG THE TUNNEL", "PICK A FIGHT", "REST IN THE CELL", "WRITE HOME", "ASK FOR PAROLE", "ESCAPE TONIGHT!" };
static const char *const AHINT[A_N] = { "BODY SKILL UP. RESPECT UP.", "A FEW DOLLARS. GUARDS LIKE IT.", "FUN SKILL UP. QUIET AND GOOD.", "RESPECT, MAYBE TROUBLE.", "BIG MONEY. THE GUARDS WATCH.", "THE PLAN GROWS. SO DOES HEAT.",
    "RESPECT UP. HEALTH AND BEHAVIOUR DOWN.", "HEALTH BACK. HEAT COOLS.", "GOOD BEHAVIOUR. A KIND WORD HOME.", "NEEDS GOOD BEHAVIOUR.", "ONLY WHEN THE PLAN IS DONE." };
static const char *const SLOT[3] = { "MORNING", "AFTERNOON", "EVENING" };
static int sentence, day, slot, sel, health, respect, heat, behav, plan, earned, solitary, anim, over, outcome, logn, tdays;
static char logl[4][40]; static const char *msg; static int msgT;
static int menu[A_N], mn;
static unsigned rs = 12345;
static int rnd(int n) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (int)((rs >> 3) % (unsigned)n); }
static void say(const char *s) { for (int i = 3; i > 0; i--) strcpy(logl[i], logl[i - 1]); strncpy(logl[0], s, 39); logl[0][39] = 0; }
static int clampv(int v) { return v < 0 ? 0 : v > 100 ? 100 : v; }
static void build_menu(void) {
    mn = 0; for (int a = 0; a < A_PAROLE; a++) menu[mn++] = a;
    if (behav >= 70 && sentence - day <= sentence * 2 / 3) menu[mn++] = A_PAROLE;
    if (plan >= 100) menu[mn++] = A_ESCAPE;
    if (sel >= mn) sel = mn - 1;
}
void prison_open(int days) {
    sentence = days < 1 ? 1 : days > 30 ? 30 : days; day = 0; slot = 0; sel = 0; health = 100; respect = 10 + rap_sheet * 5; heat = 10; behav = 55; plan = 0; earned = 0; solitary = 0; anim = 0; over = 0; outcome = 0;
    msg = 0; msgT = 0; memset(logl, 0, sizeof logl); rs = (unsigned)(sentence * 7919 + cash + 1); tdays = 0;
    char b[40]; sprintf(b, "SENTENCE: %d DAYS", sentence); say(b); say("THE DOOR SLAMS. WELCOME TO HARD TIME.");
    build_menu();
}
static void finish(int how) {   // 0 served, 1 parole, 2 escaped, 3 hospital
    over = 1; outcome = how; cash += earned; char b[48];
    sim_skip_days(how == 1 ? day : how == 2 ? day : sentence);
    if (how == 2) rap_sheet = MIN(9, rap_sheet + 2); else rap_sheet = MIN(9, rap_sheet + 1);
    sims[player].mood = how == 0 ? 20 : how == 1 ? 40 : how == 2 ? -10 : -20;
    sprintf(b, how == 0 ? "RELEASED AFTER %d DAYS" : how == 1 ? "PAROLED AFTER %d DAYS" : how == 2 ? "YOU ESCAPED! %d DAYS IN" : "PATCHED UP AFTER %d DAYS", how == 0 ? sentence : day); sim_note(b);
    audio_voice(how == 2 ? VOICE_YAHOO : VOICE_FINISHED);
}
static void next_slot(void) {
    if (++slot < 3) return;
    slot = 0; day++;   // the night: heat cools; a shakedown if it is high
    heat = clampv(heat - 10);
    if (heat > 70 && rnd(100) < 60) { say("SHAKEDOWN! CONTRABAND TAKEN."); earned = earned * 2 / 3; behav = clampv(behav - 10); heat = 40; plan = plan * 3 / 4; }
    if (day >= sentence) { finish(0); return; }
    if (health <= 0) { finish(3); return; }
}
static void act(int a) {
    char b[40];
    if (solitary > 0) { solitary--; say("SOLITARY. YOU STARE AT THE WALL."); health = clampv(health + 5); next_slot(); return; }
    switch (a) {
    case A_WORK: sims[player].skill_body += 40; respect = clampv(respect + 3); health = clampv(health - 3); say("YOU PUMP IRON. THE YARD NOTICES."); break;
    case A_JOB: earned += 25; behav = clampv(behav + 5); heat = clampv(heat - 3); say("A SHIFT IN THE LAUNDRY: +$25."); break;
    case A_LIB: sims[player].skill_fun += 30; behav = clampv(behav + 3); say("QUIET HOURS IN THE LIBRARY."); break;
    case A_YARD: respect = clampv(respect + 4); if (rnd(100) < 25) { health = clampv(health - 12); behav = clampv(behav - 6); say("A SHOVE IN THE YARD. YOU HOLD YOUR OWN."); } else say("YOU WALK THE YARD. NODS AND SMIRKS."); break;
    case A_TRADE: { int pr = 40 + respect / 2; earned += pr; heat = clampv(heat + 14); sprintf(b, "A QUIET DEAL: +$%d.", pr); say(b);
                    if (heat > 60 && rnd(100) < 40) { say("CAUGHT! +1 DAY AND LOST PRIVILEGES."); behav = clampv(behav - 15); sentence = MIN(40, sentence + 1); solitary = 1; } } break;
    case A_DIG: plan = clampv(plan + 12 + respect / 20); heat = clampv(heat + 9); say("A SPOON, A WALL, A POSTER. THE HOLE GROWS."); if (heat > 75 && rnd(100) < 30) { say("A GUARD FINDS THE HOLE! PLAN SET BACK."); plan /= 2; behav = clampv(behav - 10); } break;
    case A_FIGHT: respect = clampv(respect + 10); health = clampv(health - 15); behav = clampv(behav - 10); heat = clampv(heat + 10); say("YOU PICK A FIGHT AND WIN. JUST."); if (rnd(100) < 35) { say("THE GUARDS DRAG YOU TO SOLITARY."); solitary = 1; } break;
    case A_REST: health = clampv(health + 22); heat = clampv(heat - 8); say("YOU REST. THE CELL IS QUIET."); break;
    case A_LETTER: behav = clampv(behav + 2); sims[player].mood = MIN(100, sims[player].mood + 10); say("A LETTER HOME. THEY WILL WAIT FOR YOU."); break;
    case A_PAROLE: if (rnd(100) < behav - 25) { finish(1); return; } behav = clampv(behav - 5); say("THE BOARD SAYS NOT YET."); break;
    case A_ESCAPE: { int ch = 35 + respect / 4 - heat / 4; if (rnd(100) < ch) { finish(2); return; } say("CAUGHT AT THE FENCE! +7 DAYS."); sentence = MIN(40, sentence + 7); solitary = 2; plan = 30; behav = clampv(behav - 25); } break;
    }
    if (health <= 0) { finish(3); return; }
    next_slot();
}
int prison_update(int hd, int pr, int steps) {
    anim += steps; if (msgT > 0) { msgT -= steps; if (msgT <= 0) msg = 0; }
    if (over) { return !(pr & (PAD_CROSS | PAD_CIRCLE | PAD_START)); }
    build_menu();
    if (pr & PAD_UP) sel = (sel + mn - 1) % mn; if (pr & PAD_DOWN) sel = (sel + 1) % mn;
    if (pr & PAD_CROSS) { act(menu[sel]); build_menu(); }
    return 1;
}
static void bar(int x, int y, const char *nm, int v, u32 c) { ui_text(x, y, 1, nm, UI_INK); ui_bar(x + 66, y + 2, 64, 6, v, c); }
void prison_draw(void) {
    // the cell: a grey wall, the Bore in orange, and bars
    gfx_set_clear(10, 10, 14); gfx_set_ambient(80, 90); gfx_sky(0x403c38u, 0x686460u);
    { int bx = 0, bz = -(6 * TILE), yaw = 2048, pitch = 150, dist = 620; int fx = bx - 120, fy = -110;
      cam.at.vx = fx; cam.at.vy = fy; cam.at.vz = bz;
      cam.eye.vx = fx - ((isin(yaw) * ((dist * icos(pitch)) >> 12)) >> 12); cam.eye.vz = bz - ((icos(yaw) * ((dist * icos(pitch)) >> 12)) >> 12); cam.eye.vy = fy - ((dist * isin(pitch)) >> 12);
      gfx_camera_update(); gfx_set_fog(0, 0);
      VECTOR p; setVector(&p, bx, 0, bz); Gait g; memset(&g, 0, sizeof g); g.clock = anim; g.mood = solitary ? -80 : respect > 50 ? 20 : -30; g.sit = solitary ? 1 : 0; g.seed = 3;
      voxel_draw(&sims[player].bore, &p, 2048 + 200, &g); gfx_shadow(&p, 70); }
    for (int i = 0; i < 6; i++) ui_round(12 + i * 22, 6, 6, 160, 2, UI_RGB(120, 124, 132), UI_RGB(60, 62, 70), 0);   // the bars
    ui_round(6, 4, 140, 6, 2, UI_RGB(120, 124, 132), UI_RGB(60, 62, 70), 0); ui_round(6, 160, 140, 6, 2, UI_RGB(120, 124, 132), UI_RGB(60, 62, 70), 0);
    char b[48];
    ui_round(6, 172, 150, 42, 6, UI_RGB(30, 30, 36), UI_RGB(14, 14, 18), 0);
    ui_wrap(11, 175, 140, 3, logl[0], UI_WHITE);
    int top = ui_window(160, 4, 156, 232, "HARD TIME", IC_LOCK);
    sprintf(b, "DAY %d OF %d", MIN(day + 1, sentence), sentence); ui_text(166, top, 1, b, UI_INK); ui_text(300 - ui_tw(1, SLOT[slot]), top, 1, SLOT[slot], UI_BLUE2);
    bar(166, top + 12, "HEALTH", health, health > 50 ? UI_GREEN : health > 25 ? UI_AMBER : UI_RED);
    bar(166, top + 23, "RESPECT", respect, UI_BLUE); bar(166, top + 34, "HEAT", heat, heat > 70 ? UI_RED : UI_AMBER);
    bar(166, top + 45, "BEHAVIOUR", behav, UI_GREEN); bar(166, top + 56, "ESCAPE", plan, UI_RGB(170, 90, 210));
    sprintf(b, "EARNED $%d   RECORD %d", earned, rap_sheet); ui_text(166, top + 68, 1, b, UI_GOODT);
    if (over) {
        ui_wrap(166, top + 90, 140, 6, outcome == 0 ? "YOUR TIME IS UP. THE GATE OPENS. YOU CARRY OUT WHAT YOU EARNED AND LEARNED." : outcome == 1 ? "THE BOARD SAYS YES. YOU WALK OUT EARLY, WITH A RECORD." :
                outcome == 2 ? "OVER THE FENCE AND GONE! THE RECORD GROWS. KEEP YOUR HEAD DOWN." : "THE INFIRMARY PATCHED YOU UP. YOU ARE FREE, BATTERED.", UI_SOFT);
        ui_hints(152, 222, "X LEAVE", 2); return;
    }
    for (int i = 0; i < mn; i++) { int y = top + 82 + i * 11; int on = i == sel;
        if (on) { ui_round(164, y - 1, 148, 11, 4, UI_RGB(226, 242, 254), UI_HILITE, 0); }
        ui_text(170, y, 1, ANM[menu[i]], on ? UI_BLUE2 : UI_INK); }
    ui_wrap(11, 202, 140, 1, AHINT[menu[sel]], UI_SOFT);
    ui_hints(152, 222, "UP DOWN PICK|X DO IT", 2);
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, prison_open, prison_update, prison_draw };
