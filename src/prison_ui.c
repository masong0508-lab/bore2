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

enum { A_WORK, A_JOB, A_LIB, A_YARD, A_TRADE, A_DIG, A_FIGHT, A_REST, A_LETTER, A_BRIBE, A_APPEALC, A_PAROLE, A_ESCAPE, A_N,
       R_APPEAL, R_CLEM, R_MEAL, R_DIG, R_LETTER, R_ESC, R_N };   // (R_*: death row)
enum { P_ARREST, P_LAWYER, P_PLEA, P_TRIAL, P_VERDICT, P_CELL, P_ROW };
static const char *const ANM[A_N] = { "WORK OUT", "PRISON JOB", "THE LIBRARY", "YARD TIME", "TRADE CONTRABAND", "DIG THE TUNNEL", "PICK A FIGHT", "REST IN THE CELL", "WRITE HOME", "BRIBE GUARD  $300", "APPEAL  $1500", "ASK FOR PAROLE", "ESCAPE TONIGHT!" };
static const char *const AHINT[A_N] = { "BODY SKILL UP. RESPECT UP.", "A FEW DOLLARS. GUARDS LIKE IT.", "FUN SKILL UP. QUIET AND GOOD.", "RESPECT, MAYBE TROUBLE.", "BIG MONEY. THE GUARDS WATCH.", "THE PLAN GROWS. SO DOES HEAT.",
    "RESPECT UP. HEALTH AND BEHAVIOUR DOWN.", "HEALTH BACK. HEAT COOLS.", "GOOD BEHAVIOUR. A KIND WORD HOME.", "HEAT DOWN. MAYBE. RISKY.", "A NEW LAWYER. A CHANCE AT A RETRIAL.", "NEEDS GOOD BEHAVIOUR.", "ONLY WHEN THE PLAN IS DONE." };
static const char *const RNM[6] = { "APPEAL  $2500", "PLEAD FOR CLEMENCY", "ORDER THE LAST MEAL", "DIG FOR YOUR LIFE", "WRITE A LAST LETTER", "RUN FOR THE FENCE!" };
static const char *const RHINT[6] = { "A LAWYER FIGHTS THE VERDICT.", "THE GOVERNOR MAY LISTEN.", "A GOOD MEAL. NOTHING MORE.", "THE PLAN GROWS. SO DOES HEAT.", "THEY WILL READ IT FOR YEARS.", "NOW OR NEVER." };
// the lawyers: the state's free one (overworked), a cheap one (sometimes late), a private one, a shark, a dream team, and, rarely, a crooked one who fixes juries
static const char *const LNM[7] = { "STATE APPOINTED  FREE", "CHEAP LAWYER  $150", "PRIVATE LAWYER  $600", "THE SHARK  $2500", "DREAM TEAM  $9000", "SLICK SAL  $4000 ?", "DEFEND YOURSELF  FREE" };
static const char *const LDESC[7] = { "FREE. OVERWORKED. MAY NOT EVEN READ YOUR FILE.", "CHEAP. SOMETIMES LATE. SOMETIMES DRUNK.", "BETTER ODDS. COSTS REAL MONEY.", "THE JURY FEARS THEM. SO DO THE DEBTS.", "A WHOLE FIRM. WINS NEARLY EVERYTHING.", "WINKS A LOT. KNOWS THE JURORS. NOT LEGAL.", "YOUR LOGIC AND CHARM IN COURT. RISKY, BUT FREE." };
static const int LQ0[7] = { 1, 2, 3, 5, 8, 6, 1 }, LCOST[7] = { 0, 150, 600, 2500, 9000, 4000, 0 };
#define LQ(i) ((i) == 6 ? 1 + (SKL(sims[player].skill_logic) + SKL(sims[player].skill_char)) / 3 : LQ0[i])   // (yourself: logic and charm)
static int nl, corrupt;
#define LID(i) ((i) < 5 ? (i) : (i) == 5 ? 6 : 5)   // (the list: five lawyers, yourself, then Sal if he turns up)
static const char *const PNM[3] = { "PLEAD NOT GUILTY", "PLEAD GUILTY (A DEAL)", "BRIBE THE JUDGE  $1500" };
static const char *const SLOT[3] = { "MORNING", "AFTERNOON", "EVENING" };
static int phase, lawyer, plea, tstep, tT, verdict, framed, glt, cpts, evid0, def0, appeals, execd, cs, hasrow, sentence0, sentence, day, slot, sel, health, respect, heat, behav, plan, earned, solitary, anim, over, outcome, logn, tdays;
static char logl[4][40]; static const char *msg; static int msgT;
static int menu[R_N], mn;
static unsigned rs = 12345;
static int rnd(int n) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (int)((rs >> 3) % (unsigned)n); }
static void say(const char *s) { for (int i = 3; i > 0; i--) strcpy(logl[i], logl[i - 1]); strncpy(logl[0], s, 39); logl[0][39] = 0; }
static int clampv(int v) { return v < 0 ? 0 : v > 100 ? 100 : v; }
static void build_menu(void) {
    mn = 0;
    if (phase == P_ROW) { menu[mn++] = R_APPEAL; menu[mn++] = R_CLEM; menu[mn++] = R_MEAL; menu[mn++] = R_DIG; menu[mn++] = R_LETTER; if (plan >= 100) menu[mn++] = R_ESC; if (sel >= mn) sel = mn - 1; return; }
    for (int a = 0; a < A_PAROLE; a++) { if (a == A_APPEALC && (appeals <= 0 || (!framed && glt && evid0 > 60))) continue; menu[mn++] = a; }
    if (behav >= 70 && sentence - day <= sentence * 2 / 3) menu[mn++] = A_PAROLE;
    if (plan >= 100) menu[mn++] = A_ESCAPE;
    if (sel >= mn) sel = mn - 1;
}
static char chg[3][40];
#ifdef AUTO_PRISON
#define FORCE_CORRUPT 1
#endif
extern void heist_open(void); extern int heist_update(int, int, int); extern void heist_draw(void); static int heisting;   // (GRAND THEFT: heist_ui.c, the underworld, opened from the phone)
void prison_open(int arg) {
    heisting = arg == 3; if (heisting) { heist_open(); return; }
    (void)arg; phase = P_ARREST; sentence = 0; day = 0; slot = 0; sel = 0; health = 100; respect = 10 + rap_sheet * 5; heat = 10; behav = 55; plan = 0; earned = 0; solitary = 0; anim = 0; over = 0; outcome = 0;
    msg = 0; msgT = 0; memset(logl, 0, sizeof logl); rs = (unsigned)(cash + 1 + crime_evid * 31 + rap_sheet * 977 + day_count * 13); tdays = 0; lawyer = 0; corrupt = rnd(100) < 16; 
#ifdef FORCE_CORRUPT
    corrupt = 1;
#endif
    nl = 6 + corrupt; plea = 0; tstep = 0; tT = 0; verdict = 0; appeals = 2; execd = 0; hasrow = 0;
    glt = crime_total() > 0; framed = !glt; cpts = crime_points(); evid0 = crime_evid; def0 = crime_def;
    if (framed) { crime_n[CR_ASSAULT] = 0; cpts = 5 + rnd(6); evid0 = 10 + rap_sheet * 6; strcpy(chg[0], "CHARGE: SUSPICIOUS BEHAVIOUR"); chg[1][0] = chg[2][0] = 0; }   // no crime on the books: a charge is found anyway
    else { int l = 0; for (int i = 0; i < CR_N && l < 3; i++) if (crime_n[i]) { sprintf(chg[l], "%s X%d", crime_nm[i], crime_n[i]); l++; } for (; l < 3; l++) chg[l][0] = 0; }
    build_menu();
}
static void finish(int how) {   // 0 served, 1 parole, 2 escaped, 3 hospital, 4 acquitted, 5 executed, 6 freed on appeal
    over = 1; outcome = how; cash += earned; char b[48]; int served = how == 0 ? sentence : day;
    if (how < 3 && pk_ext[PKX_PRISON + how] < 255) pk_ext[PKX_PRISON + how]++;   // (the PACK GOALS page: sentences served, paroles, escapes)
    sim_skip_days(how == 4 ? 1 : how == 5 ? 0 : served);
    if (how == 2) rap_sheet = MIN(9, rap_sheet + 2);
    memset(crime_n, 0, sizeof crime_n); crime_evid = 0; crime_def = 0;
    sims[player].mood = how == 0 ? 20 : how == 1 ? 40 : how == 2 ? -10 : how == 4 || how == 6 ? 60 : -20;
    sprintf(b, how == 0 ? "RELEASED AFTER %d DAYS" : how == 1 ? "PAROLED AFTER %d DAYS" : how == 2 ? "YOU ESCAPED! %d DAYS IN" : how == 3 ? "PATCHED UP AFTER %d DAYS" : how == 4 ? "NOT GUILTY! FREE (%d)" : how == 5 ? "EXECUTED (%d)" : "THE APPEAL WON! FREE AFTER %d", how == 4 || how == 5 ? 0 : served); sim_note(b);
    if (how == 5) { sim_want_end = 1; audio_voice(VOICE_CRY); } else audio_voice(how == 2 || how == 4 || how == 6 ? VOICE_YAHOO : VOICE_FINISHED);
}
static void next_slot(void) {
    if (++slot < 3) return;
    slot = 0; day++;   // the night: heat cools; a shakedown if it is high
    heat = clampv(heat - 10);
    if (phase == P_ROW) { execd--; char t[40]; if (execd > 0) { sprintf(t, "%d DAYS TO THE EXECUTION.", execd); say(t); } if (health <= 0 || execd <= 0) { finish(5); return; } return; }
    if (heat > 70 && rnd(100) < 60) { say("SHAKEDOWN! CONTRABAND TAKEN."); earned = earned * 2 / 3; behav = clampv(behav - 10); heat = 40; plan = plan * 3 / 4; }
    if (day >= sentence) { finish(0); return; }
    if (health <= 0) { finish(3); return; }
}
static void act(int a) {
    char b[40];
    if (solitary > 0) { solitary--; say("SOLITARY. YOU STARE AT THE WALL."); health = clampv(health + 5); next_slot(); return; }
    if (a >= R_APPEAL) { switch (a) {
        case R_APPEAL: if (cash < 2500) { say("YOU CANNOT AFFORD A LAWYER."); return; } cash -= 2500; if (appeals > 0) appeals--;
            if (rnd(100) < 22 + LQ(lawyer) * 9 + (framed ? 30 : 0) - evid0 / 4 + (def0 ? 8 : 0)) { say("APPEAL GRANTED! LIFE INSTEAD."); phase = P_CELL; sentence = 20; day = 0; slot = 0; sel = 0; execd = 0; build_menu(); return; } say("THE APPEAL IS DENIED."); break;
        case R_CLEM: if (rnd(100) < 3 + behav / 9 + (framed ? 12 : 0)) { say("THE GOVERNOR COMMUTES YOUR SENTENCE!"); phase = P_CELL; sentence = 25; day = 0; slot = 0; sel = 0; execd = 0; build_menu(); return; } say("THE GOVERNOR DOES NOT ANSWER."); break;
        case R_MEAL: sims[player].mood = MIN(100, sims[player].mood + 25); health = clampv(health + 10); say("A GREAT MEAL. YOU ALMOST SMILE."); break;
        case R_DIG: plan = clampv(plan + 14); heat = clampv(heat + 10); say("THE HOLE GROWS BEHIND THE POSTER."); if (heat > 70 && rnd(100) < 35) { say("A GUARD FINDS IT!"); plan /= 2; } break;
        case R_LETTER: behav = clampv(behav + 8); sims[player].mood = MIN(100, sims[player].mood + 8); say("YOU WRITE: I WAS BORED, THAT IS ALL."); break;
        case R_ESC: if (rnd(100) < 20 + respect / 4 - heat / 5) { finish(2); return; } say("CAUGHT! THE DATE MOVES UP."); execd = MAX(1, execd - 1); plan = 30; solitary = 1; break;
        } next_slot(); return; }
    switch (a) {
    case A_BRIBE: if (cash < 300) { say("YOU CANNOT AFFORD IT."); return; } cash -= 300; if (rnd(100) < 60) { heat = clampv(heat - 35); behav = clampv(behav + 3); say("THE GUARD LOOKS AWAY. HEAT DOWN."); } else { behav = clampv(behav - 12); heat = clampv(heat + 10); say("THE GUARD TAKES IT AND TELLS."); } break;
    case A_APPEALC: if (cash < 1500) { say("YOU CANNOT AFFORD IT."); return; } cash -= 1500; appeals--;
        if (rnd(100) < 20 + LQ(lawyer) * 8 + (framed ? 30 : 0) + (def0 ? 8 : 0) - evid0 / 5) { finish(6); return; } say("THE JUDGE REFUSES THE APPEAL."); break;
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

// ---- the court: charges, a lawyer, a plea, a trial, the verdict -----------------------------------------------------------------------------------
static char tl[5][40]; static const char *vtxt;
static void verdict_go(void) {   // the jury: what the other side has, what yours says, and the mood of twelve strangers
    int fix = 0, bad = 0; if (lawyer == 6) { sims[player].skill_logic += 40; sims[player].skill_char += 40; } if (lawyer == 0 && rnd(100) < 30) bad = 8; if (lawyer == 1 && rnd(100) < 25) bad = 10;   // (asleep, or late)
    if (lawyer == 5) { int r = rnd(100); if (r < 70) fix = 60; else if (r < 88) { fix = -1; } }   // (the jury fixed: or caught at it)
    int pros = glt ? 22 + evid0 / 2 + cpts / 2 : 12 + rap_sheet * 8, def = LQ(lawyer) * 9 + def0 * 9 + (arrest_how == 1 ? 8 : 0) - bad + (fix > 0 ? fix : 0), jury = rnd(45) - 22, score = pros + rap_sheet * 3 - def + jury;
    int guilty = plea == 1 || score > 18; if (fix < 0 && plea != 1) { guilty = 1; cpts += 15; }
    sentence0 = (cpts + (framed ? 8 : 0)) / 5 + rap_sheet; if (sentence0 < 1) sentence0 = 1; if (plea == 1) sentence0 = MAX(1, sentence0 * 55 / 100); if (sentence0 > 30) sentence0 = 30;
    verdict = 0;
    if (guilty) { verdict = 1; rap_sheet = MIN(9, rap_sheet + 1);
        int capital = plea != 1 && (cpts >= 40 || (rap_sheet >= 3 && cpts >= 25)) && rnd(100) < 30 + rap_sheet * 8 - LQ(lawyer) * 5 - (arrest_how == 1 ? 8 : 0);
        if (capital) verdict = 2; }
    if (fix < 0 && plea != 1) strcpy(tl[3], "JURY TAMPERING! SAL IS CAUGHT. YOU TOO.");
    else if (fix > 0 && plea != 1) strcpy(tl[3], "THE JURY SMILES AT YOUR LAWYER. ODD.");
    else if (bad && plea != 1) strcpy(tl[3], lawyer ? "YOUR LAWYER IS LATE AND SWEATING." : "YOUR LAWYER NEVER READ THE FILE.");
    if (!guilty) { strcpy(tl[4], glt ? "NOT GUILTY! LUCKY, SOME WOULD SAY." : "NOT GUILTY! THE TRUTH WON."); vtxt = "NOT GUILTY"; }
    else if (verdict == 2) { strcpy(tl[4], "GUILTY. THE JUDGE SAYS DEATH."); vtxt = "DEATH SENTENCE"; }
    else { sprintf(tl[4], "GUILTY. %d DAYS%s.", sentence0, plea == 1 ? " (A DEAL)" : framed ? " (FOR NOTHING)" : ""); vtxt = "GUILTY"; }
    if (guilty && plea != 1) { int fine = MIN((int)cash, cpts * 8); cash -= fine; }
}
static void court_next(void) {
    if (phase == P_TRIAL) {
        static const char *const wit[3] = { "A WITNESS POINTS AT YOU.", "NOBODY SAW A THING.", "A SHAKY WITNESS CHANGES THEIR STORY." };
        char b[40]; switch (tstep) {
        case 0: strcpy(tl[0], glt ? (evid0 > 55 ? wit[0] : evid0 > 25 ? wit[2] : wit[1]) : "THE POLICE REPORT SAYS YOU LOOKED GUILTY."); break;
        case 1: strcpy(tl[1], glt ? (evid0 > 55 ? "CAMERAS AND WITNESSES LINE UP." : "THE EVIDENCE IS THIN.") : "THERE IS NO REAL EVIDENCE AT ALL."); break;
        case 2: sprintf(b, def0 ? "YOUR LAWYER: SELF DEFENCE! (%d)" : "YOUR LAWYER OBJECTS A LOT.", def0); strcpy(tl[2], b); break;
        case 3: strcpy(tl[3], rap_sheet ? "THE JURY SEES YOUR RECORD." : "A CLEAN RECORD HELPS."); break;
        default: verdict_go(); phase = P_VERDICT; tstep = 0; return; }
        tstep++; tT = 0; return; }
}
static int court_update(int pr) {
    tT++;
    if (phase == P_ARREST) { if (pr & PAD_CROSS) { phase = P_LAWYER; sel = 0; } return 1; }
    if (phase == P_LAWYER || phase == P_PLEA) { int n = phase == P_LAWYER ? nl : 3;
        if (pr & PAD_UP) sel = (sel + n - 1) % n; if (pr & PAD_DOWN) sel = (sel + 1) % n;
        if (pr & PAD_CROSS) {
            if (phase == P_LAWYER) { if (cash < LCOST[LID(sel)]) { say("NOT ENOUGH CASH."); return 1; } cash -= LCOST[LID(sel)]; lawyer = LID(sel); if (lawyer == 5) say("SAL: LEAVE THE JURY TO ME."); phase = P_PLEA; sel = 0; }
            else { plea = sel;
                if (sel == 2) { if (cash < 1500) { say("NOT ENOUGH CASH."); return 1; } cash -= 1500; if (rnd(100) < 40) { tstep = 9; strcpy(tl[4], "THE JUDGE DISMISSES THE CASE!"); vtxt = "DISMISSED"; verdict = 0; phase = P_VERDICT; plea = 0; return 1; }
                    say("THE JUDGE REPORTS YOU!"); sim_crime(CR_RESIST, 0); cpts += 10; plea = 0; }
                if (plea == 1) { verdict_go(); phase = P_VERDICT; tstep = 1; } else { phase = P_TRIAL; tstep = 0; tT = 0; memset(tl, 0, sizeof tl); court_next(); } } }
        return 1; }
    if (phase == P_TRIAL) { if (tT > 70 || (pr & PAD_CROSS)) court_next(); return 1; }
    if (phase == P_VERDICT) { if (pr & PAD_CROSS) {
        if (verdict == 0) { finish(4); phase = P_CELL; over = 1; }
        else if (verdict == 2) { phase = P_ROW; execd = 7; day = 0; slot = 0; sel = 0; sentence = 99; health = 100; plan = 0; behav = 55; over = 0; heat = 10; say("DEATH ROW. SEVEN DAYS."); build_menu(); }
        else { phase = P_CELL; sentence = sentence0; day = 0; slot = 0; sel = 0; over = 0; char b[40]; sprintf(b, "SENTENCE: %d DAYS", sentence); say(b); say("THE DOOR SLAMS."); build_menu(); } }
        return 1; }
    return 1;
}
static void court_draw(void) {
    gfx_set_clear(24, 16, 10); char b[48];
    ui_round(0, 0, 320, 240, 0, UI_RGB(70, 46, 28), UI_RGB(34, 22, 14), 0);
    ui_round(100, 14, 120, 40, 6, UI_RGB(120, 80, 44), UI_RGB(70, 44, 24), 0); ui_text(160 - ui_tw(1, "THE JUDGE") / 2, 22, 1, "THE JUDGE", UI_WHITE);   // the bench
    ui_round(146, 32, 28, 10, 3, UI_RGB(230, 200, 150), UI_RGB(190, 150, 100), 0);
    for (int i = 0; i < 6; i++) ui_round(14 + i * 12, 20 + (i & 1) * 6, 9, 14, 3, UI_RGB(150, 160, 190), UI_RGB(90, 100, 130), 0);   // the jury
    ui_round(256, 22, 48, 22, 4, UI_RGB(110, 76, 42), UI_RGB(68, 44, 24), 0); ui_text(262, 30, 1, "WITNESS", UI_WHITE);
    ui_round(100, 66, 120, 14, 4, UI_RGB(40, 40, 70), UI_RGB(24, 24, 44), 0);
    int top = ui_window(30, 86, 260, 146, "THE COURT", IC_LOCK);
    if (phase == P_ARREST) {
        static const char *const how[4] = { "THE POLICE TOOK YOU DOWN.", "YOU GAVE YOURSELF UP.", "THE POLICE CAME WITH A WARRANT.", "A TIP-OFF BROUGHT THE POLICE." };
        ui_text(40, top + 2, 1, how[arrest_how & 3], UI_INK); ui_text(40, top + 16, 1, "THE CHARGES:", UI_BLUE2);
        for (int i = 0; i < 3; i++) if (chg[i][0]) ui_text(48, top + 28 + i * 11, 1, chg[i], UI_INK);
        ui_wrap(40, top + 64, 240, 4, framed ? "NOTHING IS ON YOUR RECORD. BUT THEY FOUND A CHARGE ANYWAY. CAN YOU PROVE IT?" : "YOUR CRIMES ARE ON THE BOOKS. A LAWYER AND A PLEA MAY CHANGE THE ENDING.", UI_SOFT);
        ui_hints(280, top + 108, "X TO THE LAWYER", 2); return; }
    if (phase == P_LAWYER || phase == P_PLEA) {
        ui_text(40, top + 2, 1, phase == P_LAWYER ? "CHOOSE A LAWYER" : "HOW DO YOU PLEAD?", UI_BLUE2);
        { int cnt = phase == P_LAWYER ? nl : 3, rh = phase == P_LAWYER ? 11 : 14;
          for (int i = 0; i < cnt; i++) { int y = top + 16 + i * rh; if (i == sel) ui_round(36, y - 2, 248, rh, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
            ui_text(44, y, 1, phase == P_LAWYER ? LNM[LID(i)] : PNM[i], i == sel ? UI_BLUE2 : UI_INK); } }
        ui_wrap(40, top + 86, 240, 2, phase == P_LAWYER ? LDESC[LID(sel)] :
            (sel == 0 ? "FIGHT IT. EVIDENCE, JURY AND LAWYER DECIDE." : sel == 1 ? "A SHORTER SENTENCE. NO DEATH PENALTY. NO TRIAL." : "40% IT WORKS. IF NOT, MORE TROUBLE."), UI_SOFT);
        sprintf(b, "CASH $%d", (int)cash); ui_text(280 - ui_tw(1, b), top + 2, 1, b, UI_GOODT);
        ui_text(40, top + 106, 1, logl[0], UI_RED); ui_hints(280, top + 108, "UP DOWN PICK|X DECIDE", 2); return; }
    if (phase == P_TRIAL) { ui_text(40, top + 2, 1, "THE TRIAL", UI_BLUE2); for (int i = 0; i < 5; i++) if (tl[i][0] && i < 4) ui_text(40, top + 16 + i * 12, 1, tl[i], i == tstep - 1 ? UI_INK : UI_SOFT); ui_hints(280, top + 108, "X NEXT", 2); return; }
    if (phase == P_VERDICT) {
        ui_text(160 - ui_tw(2, vtxt) / 2, top + 6, 2, vtxt, verdict == 0 ? UI_GREEN : UI_RED);
        for (int i = 0; i < 4; i++) if (tl[i][0]) ui_text(40, top + 32 + i * 10, 1, tl[i], UI_SOFT);
        ui_text(40, top + 76, 1, tl[4], UI_INK);
        ui_hints(280, top + 108, verdict == 0 ? "X WALK FREE" : "X TAKEN AWAY", 2); }
}
int prison_update(int hd, int pr, int steps) {
    if (heisting) return heist_update(hd, pr, steps);
    if (phase < P_CELL && !(phase == P_VERDICT && over)) { anim += steps; return court_update(pr); }
    anim += steps; if (msgT > 0) { msgT -= steps; if (msgT <= 0) msg = 0; }
    if (over) { return !(pr & (PAD_CROSS | PAD_CIRCLE | PAD_START)); }
    build_menu();
    if (pr & PAD_UP) sel = (sel + mn - 1) % mn; if (pr & PAD_DOWN) sel = (sel + 1) % mn;
    if (pr & PAD_CROSS) { act(menu[sel]); build_menu(); }
    return 1;
}
static void bar(int x, int y, const char *nm, int v, u32 c) { ui_text(x, y, 1, nm, UI_INK); ui_bar(x + 66, y + 2, 64, 6, v, c); }
void prison_draw(void) {
    if (heisting) { heist_draw(); return; }
    if (phase < P_CELL) { court_draw(); return; }
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
    int top = ui_window(160, 4, 156, 232, phase == P_ROW ? "DEATH ROW" : "THE CELL", IC_LOCK);
    if (phase == P_ROW) sprintf(b, "%d DAYS LEFT", execd); else if (sentence <= 0) sprintf(b, "FREE"); else sprintf(b, "DAY %d OF %d", MIN(day + 1, sentence), sentence); ui_text(166, top, 1, b, UI_INK); ui_text(300 - ui_tw(1, SLOT[slot]), top, 1, SLOT[slot], UI_BLUE2);
    bar(166, top + 12, "HEALTH", health, health > 50 ? UI_GREEN : health > 25 ? UI_AMBER : UI_RED);
    bar(166, top + 23, "RESPECT", respect, UI_BLUE); bar(166, top + 34, "HEAT", heat, heat > 70 ? UI_RED : UI_AMBER);
    bar(166, top + 45, "BEHAVIOUR", behav, UI_GREEN); bar(166, top + 56, "ESCAPE", plan, UI_RGB(170, 90, 210));
    sprintf(b, "EARNED $%d   RECORD %d", earned, rap_sheet); ui_text(166, top + 68, 1, b, UI_GOODT);
    if (over) {
        ui_wrap(166, top + 90, 140, 6, outcome == 0 ? "YOUR TIME IS UP. THE GATE OPENS. YOU CARRY OUT WHAT YOU EARNED AND LEARNED." : outcome == 1 ? "THE BOARD SAYS YES. YOU WALK OUT EARLY, WITH A RECORD." :
                outcome == 2 ? "OVER THE FENCE AND GONE! THE RECORD GROWS. KEEP YOUR HEAD DOWN." : outcome == 3 ? "THE INFIRMARY PATCHED YOU UP. YOU ARE FREE, BATTERED." : outcome == 5 ? "THE LIGHTS GO OUT. A NEW LIFE BEGINS ELSEWHERE." : outcome == 6 ? "THE APPEAL WON. THE COURT WAS WRONG. YOU WALK OUT." : "NOT GUILTY. THE COURT LETS YOU GO.", UI_SOFT);
        ui_hints(152, 218, "X LEAVE", 2); return;
    }
    for (int i = 0; i < mn; i++) { int y = top + 82 + i * 11; int on = i == sel;
        if (on) { ui_round(164, y - 1, 148, 11, 4, UI_RGB(226, 242, 254), UI_HILITE, 0); }
        ui_text(170, y, 1, menu[i] >= R_APPEAL ? RNM[menu[i] - R_APPEAL] : ANM[menu[i]], on ? UI_BLUE2 : UI_INK); }
    ui_wrap(11, 202, 140, 1, menu[sel] >= R_APPEAL ? RHINT[menu[sel] - R_APPEAL] : AHINT[menu[sel]], UI_SOFT);
    ui_hints(152, 218, "UP DOWN PICK|X DO IT", 2);
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, prison_open, prison_update, prison_draw };
