// pack_ui: THE PACK PAGES, an overlay (PKINFO.OVL, part of the base game: it is not a pack). Two ways in:
//   X on a card of the EXPANSIONS shelf: that pack's page. A pack on the disc shows what it holds, how to open it in a life and its five GOALS; a pack
//   that is not on the disc says NOT ADDED and which .ppf puts it on; a pack still to come says COMING SOON, what it will add, and what is already kept for
//   it (a disc slot, eight bytes of the save, item codes), so that nothing it needs will break a save or another pack when it arrives.
//   R1 on the phone's GOALS screen (in a life): PACK GOALS, a list of the twelve packs with their goals. X on one opens its page, and there X claims
//   every goal that is done: a cash reward each, and PACK MASTER when all five are claimed. What is claimed is kept in pk_ext (sim.h), so the save has it.
// The goals read what the packs already keep (sim.h, pk_data, the band, the farm ...), so they tick by themselves as the pack is played.
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "ovl.h"
#include "band.h"
#include "dog.h"
#include <psxgpu.h>
#include <psxpad.h>
#include <stdio.h>
#include <string.h>

extern int realm_mis, arcade_tix, fight_wins, fight_champ;   // (kept by the game: the realm's mission, the arcade)
extern u32 gach;                                              // (the Boreagotchi achievements, a bit each)
extern int dj_xp;

enum { K_BASE, K_PACK, K_SOON };
typedef struct { const char *nm; u8 kind, pk, ic; const char *what, *how, *slot, *item; } Card;
// one per card of the shelf (main.c: moreNm), in the shelf's order. pk: the pack's number (0-11 are on the disc; 12 and up are kept for packs to come)
static const Card C[26] = {
    { "ISLAND CREATOR", K_BASE, 255, IC_SUN, "", "", "", "" },
    { "STORY MISSIONS", K_BASE, 255, IC_BOOK, "", "", "", "" },
    { "BORE-OUT MAN!", K_SOON, 18, IC_STAR, "THE SPACE-HOP ARCADE GAME FROM THE GBA. A DESK JOCKEY BECOMES AN ASTRONAUT: A NEW CABINET AT THE ARCADE, WITH TICKETS AND A TABLE OF HIGH SCORES.", "", "BOREOUT.OVL", "NONE" },
    { "BOREZIES!", K_SOON, 19, IC_PEOPLE, "THE TOWN GOES UNDEAD. BOARD UP THE WINDOWS, SHARE THE CANNED BEANS, KEEP THE FAMILY ALIVE UNTIL MORNING. A SCENARIO FOR A LIFE.", "", "BOREZIES.OVL", "NONE" },
    { "WORLD WAR BORE", K_SOON, 20, IC_HAMMER, "THE EASTERN ARMY CUT DOWN YOUR HEDGE. RALLY THE STREET, BUILD THE LINE AND HOLD IT: THE NEIGHBOURS FIGHT BESIDE YOU.", "", "WWBORE.OVL", "NONE" },
    { "BORTUA FIGHTER", K_PACK, 1, IC_BX, "THE ARCADE'S FIGHTING GAME, AND FOUR MORE CABINETS WITH A PRIZE COUNTER. WIN MATCHES, BEAT BORAL, COLLECT TICKETS FOR PRIZES.", "WALK INTO THE ARCADE. X AT A CABINET.", "ARCADE.OVL", "" },
    { "BOREAGOTCHI!", K_PACK, 2, IC_SMILE, "RAISE EIGHT PIXEL PETS ON EACH TOY: EGGS, FOOD, GAMES, FAMILIES, HATS AND A DEX. THE GUIDE IS ON THE SHELF.", "PAUSE, BOREAGOTCHI.", "GOTCHI.OVL", "" },
    { "BOREPILOGUE", K_SOON, 21, IC_HEART, "GROWN-UPS ONLY: RETIRE, WRITE YOUR WILL AND LEAVE A LEGACY. THE NEXT FAMILY INHERITS THE HOME, THE NAME AND A LETTER.", "", "BOREPILG.OVL", "NONE" },
    { "THE LAND BOREFORE TIME", K_PACK, 0, IC_TREE, "WAKE A GIANT IN A MUDDY FIELD AMONG THE NUBBINS. GATHER, LAY ROADS, RAISE HUTS AND WORKSHOPS AND CROWN A CASTLE. SIXTEEN MISSIONS PAY INTO YOUR LIFE.", "PAUSE, KINGDOM MODE.", "KINGDOM.OVL", "" },
    { "SEASONS", K_BASE, 255, IC_SUN, "", "", "", "" },
    { "ISLAND PARADISE", K_PACK, 9, IC_TREE, "A PRIVATE RESORT: BUILD TWELVE LUXURIES THAT PAY EVERY DAY, MOVE IT TO AN ISLE OUT AT SEA, AND JOIN THE CASTAWAY CLUB.", "PHONE, BOOK A RESORT.", "PARADISE.OVL", "" },
    { "SUPERSTAR", K_PACK, 3, IC_NOTE, "FORM A BAND OF UP TO FIFTEEN, GIVE EACH A ROLE, PICK FROM 25 SONGS, REHEARSE AND PLAY GIGS. GROW FAME AND FANS, BUY A TOUR BUS.", "PAUSE, SUPERSTAR BAND.", "SUPER.OVL", "" },
    { "CRIME AND LAW", K_PACK, 4, IC_EYE, "BREAK THE LAW AND FACE THE COURT: LAWYERS, A PLEA, A TRIAL, THEN HARD TIME DAY BY DAY. PAROLE, ESCAPE, OR RUN THE UNDERWORLD HEISTS.", "HEISTS: PHONE, UNDERWORLD.", "PRISON.OVL", "" },
    { "FARM LIFE", K_PACK, 10, IC_SPROUT, "CROPS THROUGH THE SEASONS, HENS AND COWS, A BARN, SILO, ORCHARD, BEES AND A FARM STAND, AND THREE BUYERS A DAY WITH ORDERS.", "PHONE, FARM LEDGER, OR THE FARM STAND.", "FARM.OVL", "" },
    { "SKY AND SEA", K_PACK, 11, IC_BOLT, "TEN BOATS AND SIX AIRCRAFT YOU REALLY DRIVE AND FLY: FUEL, WAVES, TUNING, JOBS AND RACES, A HARBOUR, AND CRASHES THE TOWN PAYS FOR.", "PHONE, HARBOUR AND SKY.", "SKYSEA.OVL", "" },
    { "SPEED CITY", K_SOON, 14, IC_CAR, "RACE THE STREETS: GARAGES, TUNING, LAP TIMES AND A CITY GRAND PRIX WITH THE TOWNSFOLK ON THE KERB.", "", "SPEEDCTY.OVL", "F9 FA" },
    { "BIG SCREEN", K_SOON, 15, IC_CAMERA, "MOVIE SETS, PREMIERES AND A RED CARPET OF YOUR OWN. CAST YOUR FAMILY, SHOOT THE FILM, WATCH THE TOWN QUEUE.", "", "BIGSCREN.OVL", "FB" },
    { "YOUR IDEA", K_SOON, 24, IC_STAR, "SOMETHING NEW. THE LAST SLOT IS KEPT FOR THE PACK PLAYERS ASK FOR MOST.", "", "NONE YET", "NONE" },
    { "MAGIC AND MYTH", K_SOON, 16, IC_STAR, "A WAND, A SPELLBOOK, A DRAGON. THE CORNER SHOP BECOMES A TOWER AND THE TOWN LEARNS TO LOOK UP.", "", "MAGICMYT.OVL", "FC FD" },
    { "FAMILY FUN", K_PACK, 5, IC_PARTY, "THROW PARTIES AT HOME: ASK PEOPLE YOU KNOW, A WHOLE HOUSEHOLD, OR THE WHOLE STREET. PARTY THINGS, GAMES AND A BIG CAKE.", "PHONE, PARTY.", "PARTY.OVL", "" },
    { "WILD WEST", K_SOON, 17, IC_HAMMER, "A DUSTY TOWN WITH A SALOON, A SHERIFF'S STAR AND A SHOWDOWN AT NOON.", "", "WILDWEST.OVL", "FE" },
    { "SPACE AGE", K_PACK, 6, IC_BOLT, "A SPACE PROGRAM: UPGRADE HULL, ENGINE AND GUIDANCE, THEN FLY SIX MISSIONS FROM ORBIT TO DEEP SPACE. STUDY HARD FIRST.", "THE COMPUTER: SPACE PROGRAM.", "SPACE.OVL", "" },
    { "CREATE A DOG", K_PACK, 7, IC_HEART, "MAKE YOUR OWN DOG: BREED, SIZE, EARS, TAIL, COLOURS AND FIVE TRAITS. IT FOLLOWS YOU ABOUT AND NEEDS FEEDING. THE PET SHOP SELLS ONE.", "PAUSE, CREATE A DOG.", "DOG.OVL", "" },
    { "DJ BOOTH", K_PACK, 8, IC_NOTE, "TWO DECKS AND EIGHT TRACKS: CUE, MIX, ECHO, KILL, BRAKE, DROP AND SCRATCH. NEW TRICKS OPEN AS YOUR HIDDEN SKILL GROWS.", "THE DJ BOOTH, AT HOME AND AT CLUBS.", "DJ.OVL", "" },
    { "REAL WORLD", K_SOON, 12, IC_PIN, "REAL WEATHER AND REAL HOLIDAYS ON TOP OF THE REAL TIME CLOCK (OPTIONS, GAME, REAL TIME: ALREADY IN). THE DATE ON THE CLOCK BRINGS ITS HOLIDAY.", "", "REALWRLD.OVL", "F7" },
    { "FAMILY TREE 2", K_SOON, 13, IC_BABY, "GENERATIONS AND INHERITANCE FOR THE PEOPLE, NOT ONLY PETS: A FAMILY TREE, TRAITS PASSED DOWN, GRANDPARENTS AND MEMORIALS.", "", "FAMTREE2.OVL", "F8" },
};
static const u8 CARD_OF[12] = { 8, 5, 6, 11, 12, 19, 21, 22, 23, 10, 13, 14 };   // the card of each of the twelve packs on the disc
static const char *const PFILE[12] = { "KINGDOM.OVL", "ARCADE.OVL", "GOTCHI.OVL", "SUPER.OVL", "PRISON.OVL", "PARTY.OVL", "SPACE.OVL", "DOG.OVL", "DJ.OVL", "PARADISE.OVL", "FARM.OVL", "SKYSEA.OVL" };
static const char *const PPPF[12] = { "KINGDOM", "ARCADE", "GOTCHI", "SUPER", "PRISON", "PARTY", "SPACE", "DOG", "DJ", "PARADISE", "FARM", "SKYSEA" };

typedef struct { const char *t; u16 pay; } Goal;
static const Goal GL[12][5] = {
    { { "WAKE THE GIANT: MISSION 1", 300 }, { "FOUR MISSIONS DONE", 800 }, { "HALFWAY: MISSION 8", 2000 }, { "TWELVE MISSIONS DONE", 4000 }, { "THE CROWN: ALL SIXTEEN", 10000 } },
    { { "WIN 50 TICKETS", 200 }, { "WIN 250 TICKETS", 600 }, { "WIN A BORTUA FIGHTER MATCH", 300 }, { "WIN TEN MATCHES", 1500 }, { "BEAT BORAL: CHAMPION", 5000 } },
    { { "EARN AN ACHIEVEMENT", 200 }, { "THREE ACHIEVEMENTS", 500 }, { "SIX ACHIEVEMENTS", 1000 }, { "TEN ACHIEVEMENTS", 2500 }, { "SIXTEEN ACHIEVEMENTS", 5000 } },
    { { "FORM A BAND", 300 }, { "A FULL LINE-UP: FIVE MEMBERS", 500 }, { "PLAY A GIG", 500 }, { "TEN GIGS", 2000 }, { "FAME OF 500", 4000 } },
    { { "SERVE A SENTENCE", 200 }, { "EARN PAROLE", 500 }, { "BREAK OUT", 1000 }, { "PULL OFF A HEIST", 1500 }, { "KEEP A STOLEN BOAT OR PLANE", 3000 } },
    { { "THROW A PARTY", 200 }, { "FIVE PARTIES", 600 }, { "FIFTEEN PARTIES", 2000 }, { "A GREAT PARTY: SCORE 4", 500 }, { "A LEGENDARY PARTY: SCORE 8", 2500 } },
    { { "FLY A MISSION", 500 }, { "THREE MISSIONS DONE", 1500 }, { "ALL SIX MISSIONS DONE", 5000 }, { "A PERFECT ROCKET: ALL PARTS", 3000 }, { "REACH COMMANDER", 6000 } },
    { { "HAVE A DOG", 200 }, { "NAME YOUR DOG", 200 }, { "A WELL FED DOG", 300 }, { "", 0 }, { "", 0 } },
    { { "FIRST MIX", 150 }, { "A STEADY HAND: SKILL 100", 300 }, { "CROWD PLEASER: SKILL 350", 800 }, { "RESIDENT DJ: SKILL 650", 1500 }, { "MASTER OF THE DECKS", 4000 } },
    { { "BUILD A LUXURY", 300 }, { "SIX LUXURIES", 1000 }, { "ALL TWELVE LUXURIES", 5000 }, { "MOVE TO AN ISLE", 1500 }, { "A CASTAWAY CLUB GOAL", 800 } },
    { { "BUILD ON THE FARM", 300 }, { "THREE BUILDINGS", 800 }, { "ALL SIX BUILDINGS", 3000 }, { "THE FARM EARNS $1000", 1000 }, { "25 SALES AT THE STAND", 1500 } },
    { { "OWN A BOAT OR A PLANE", 300 }, { "FIRST JOB OR LANDING", 400 }, { "TEN JOBS AND LANDINGS", 1500 }, { "OWN FIVE CRAFT", 2500 }, { "OWN TEN CRAFT", 6000 } },
};
static const u8 NG[12] = { 5, 5, 5, 5, 5, 5, 5, 3, 5, 5, 5, 5 };
#define MASTER 3000
static int pc(int v) { return __builtin_popcount((unsigned)v); }
static int gdone(int pk, int g) {   // goal g of pack pk, read off what the pack keeps
    static const u8 tk[5] = { 1, 4, 8, 12, 16 };
    switch (pk) {
    case 0: return realm_mis >= tk[g];
    case 1: return g == 0 ? arcade_tix >= 50 : g == 1 ? arcade_tix >= 250 : g == 2 ? fight_wins >= 1 : g == 3 ? fight_wins >= 10 : fight_champ != 0;
    case 2: { static const u8 t[5] = { 1, 3, 6, 10, 16 }; return pc((int)gach) >= t[g]; }
    case 3: return g == 0 ? band.made : g == 1 ? band_count() >= 5 : g == 2 ? band.gigs >= 1 : g == 3 ? band.gigs >= 10 : band.fame >= 500;
    case 4: return g == 0 ? pk_ext[PKX_PRISON] >= 1 : g == 1 ? pk_ext[PKX_PRISON + 1] >= 1 : g == 2 ? pk_ext[PKX_PRISON + 2] >= 1 : g == 3 ? pk_ext[PKX_PRISON + 3] >= 1 : (pk_data[40] | pk_data[41] << 8) != 0;
    case 5: return g == 0 ? pk_ext[PKX_PARTY] >= 1 : g == 1 ? pk_ext[PKX_PARTY] >= 5 : g == 2 ? pk_ext[PKX_PARTY] >= 15 : g == 3 ? pk_ext[PKX_PARTY + 1] >= 4 : pk_ext[PKX_PARTY + 1] >= 8;
    case 6: return g == 0 ? pc(space_done) >= 1 : g == 1 ? pc(space_done) >= 3 : g == 2 ? pc(space_done) >= 6 : g == 3 ? space_tech[0] + space_tech[1] + space_tech[2] >= 12 : space_cert() >= 4;
    case 7: return g == 0 ? pet_kind == 1 : g == 1 ? pet_kind == 1 && pet_dog.name[0] : pet_kind == 1 && pet_hun >= 70;
    case 8: { static const u16 t[5] = { 1, 100, 350, 650, 1000 }; return dj_xp >= t[g]; }
    case 9: { int h = pk_data[0] | pk_data[1] << 8; return g == 0 ? pc(h) >= 1 : g == 1 ? pc(h) >= 6 : g == 2 ? pc(h) >= 12 : g == 3 ? pk_data[2] != 0 : (pk_data[5] | pk_data[6]) != 0; }
    case 10: { int b = pk_data[16] | pk_data[17] << 8; return g == 0 ? pc(b) >= 1 : g == 1 ? pc(b) >= 3 : g == 2 ? pc(b) >= 6 : g == 3 ? fm_earned >= 1000 : fm_buyers >= 25; }
    case 11: { int o = pk_data[24] | pk_data[25] << 8 | pk_data[30] << 16, j = pk_data[28]; return g == 0 ? pc(o) >= 1 : g == 1 ? j >= 1 : g == 2 ? j >= 10 : g == 3 ? pc(o) >= 5 : pc(o) >= 10; }
    }
    return 0;
}
static int gclaimed(int pk, int g) { return pk_ext[pk] >> g & 1; }
static int nready(int pk) { int n = 0; for (int g = 0; g < NG[pk]; g++) if (gdone(pk, g) && !gclaimed(pk, g)) n++; return n; }
static int ndone(int pk) { int n = 0; for (int g = 0; g < NG[pk]; g++) n += gdone(pk, g); return n; }

static int mode, card, lsel, live, anim, have[12], msgT; static char line[48];   // mode: 0 the list, 1 a page; live: opened in a life (PACK GOALS), so X can claim
void pki_open(int a) {
    anim = 0; msgT = 0; line[0] = 0; live = a == 255; lsel = 0;
    for (int i = 0; i < 12; i++) have[i] = -1;   // (found out one a frame, the one on show first: each is a look at the disc)
    if (a == 255) mode = 0; else { mode = 1; card = a % 26; }
}
static int claim(int pk) {
    int pay = 0, n = 0, all = 1;
    for (int g = 0; g < NG[pk]; g++) { if (gdone(pk, g) && !gclaimed(pk, g)) { pay += GL[pk][g].pay; pk_ext[pk] |= (u8)(1 << g); n++; } if (!gclaimed(pk, g)) all = 0; }
    if (all && !(pk_ext[pk] >> 7 & 1)) { pk_ext[pk] |= 0x80; pay += MASTER; n++; sims[player].mood = MIN(100, sims[player].mood + 40); snprintf(line, sizeof line, "PACK MASTER! +$%d", pay); }
    else if (n) snprintf(line, sizeof line, "%d GOAL%s CLAIMED: +$%d", n, n > 1 ? "S" : "", pay);
    if (n) { cash += pay; sims[player].mood = MIN(100, sims[player].mood + 10); sim_note(line); audio_voice(VOICE_WIN); }
    return n;
}
int pki_update(int hd, int pr, int steps) {
    (void)hd; anim += steps; if (msgT > 0) msgT -= steps;
    { int w = mode == 1 && C[card].kind == K_PACK ? C[card].pk : -1; if (w < 0 || have[w] >= 0) { w = -1; for (int i = 0; i < 12; i++) if (have[i] < 0) { w = i; break; } } if (w >= 0) have[w] = ovl_installed(PFILE[w]); }
    if (mode == 0) {
        if (pr & PAD_CIRCLE) return 0;
        if (pr & PAD_UP) lsel = (lsel + 11) % 12; if (pr & PAD_DOWN) lsel = (lsel + 1) % 12;
        if (pr & PAD_CROSS) { mode = 1; card = CARD_OF[lsel]; line[0] = 0; }
        return 1;
    }
    const Card *c = &C[card];
    if (pr & PAD_CIRCLE) { if (live) { mode = 0; line[0] = 0; return 1; } return 0; }
    if (c->kind == K_PACK) {
        if (live) { if (pr & PAD_LEFT) { lsel = (lsel + 11) % 12; card = CARD_OF[lsel]; line[0] = 0; } if (pr & PAD_RIGHT) { lsel = (lsel + 1) % 12; card = CARD_OF[lsel]; line[0] = 0; } }
        if ((pr & PAD_CROSS) && live) {
            if (have[c->pk] <= 0) { strcpy(line, "NOT ON THIS DISC."); audio_voice(VOICE_NAH); }
            else if (!claim(c->pk)) { strcpy(line, nready(c->pk) ? "" : "NOTHING TO CLAIM YET."); audio_voice(VOICE_NAH); }
            msgT = 180;
        }
    }
    return 1;
}
static void bg(void) {
    gfx_set_clear(20, 34, 60); gfx_camera_update(); ui_backdrop();
}
static void list_draw(void) {
    char b[48]; int top = ui_window_s(6, 6, 308, 228, "PACK GOALS"), tot = 0, got = 0;
    for (int i = 0; i < 12; i++) { int y = top + 2 + i * 15, on = i == lsel, pk = i, rd = nready(pk), dn = ndone(pk);
        const Card *c = &C[CARD_OF[i]]; tot += NG[pk]; for (int g = 0; g < NG[pk]; g++) got += gclaimed(pk, g);
        if (on) ui_round(12, y - 1, 296, 14, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
        ui_icon(16, y, have[i] > 0 ? c->ic : IC_LOCK, have[i] > 0 ? UI_BLUE2 : UI_SOFT);
        ui_text(32, y + 1, 1, c->nm, have[i] <= 0 ? UI_SOFT : on ? UI_BLUE2 : UI_INK);
        if (have[i] < 0) ui_text(300 - ui_tw(1, "..."), y + 1, 1, "...", UI_SOFT);
        else if (!have[i]) ui_text(300 - ui_tw(1, "NOT ADDED"), y + 1, 1, "NOT ADDED", UI_SOFT);
        else { ui_bar(190, y + 3, 60, 6, dn * 100 / NG[pk], pk_ext[pk] >> 7 & 1 ? UI_AMBER : UI_GREEN); sprintf(b, "%d OF %d", dn, NG[pk]); ui_text(256, y + 1, 1, b, UI_SOFT);
               if (rd) ui_icon(292, y, IC_MONEY, UI_GOODT); else if (pk_ext[pk] >> 7 & 1) ui_icon(292, y, IC_STAR, UI_GOLDT); } }
    sprintf(b, "%d GOALS CLAIMED OF %d", got, tot); ui_text(14, top + 184, 1, b, UI_SOFT);
    ui_hints(160, 220, "X OPEN A PACK|UP DOWN CHOOSE|O BACK", 1);
}
static void page_draw(void) {
    const Card *c = &C[card]; char b[64]; int top = ui_window_s(6, 6, 308, 228, c->kind == K_SOON ? "COMING SOON" : "PACK PAGE");
    int pk = c->pk, ok = c->kind == K_PACK && pk < 12 && have[pk] > 0, soon = c->kind == K_SOON;
    u32 dc = ok ? UI_RGB(255, 204, 70) : soon ? UI_RGB(206, 214, 228) : UI_LINE;
    ui_round(14, top + 1, 40, 40, 20, dc, dc, 0); ui_icon_disc(34, top + 21, 34, c->ic, ok ? UI_BLUE2 : UI_SOFT, UI_WHITE);
    ui_text(62, top + 3, 2, c->nm, UI_INK);
    if (ok) ui_badge_i(62, top + 22, IC_TICK, "ON THIS DISC", UI_GOODT, UI_WHITE);
    else if (soon) ui_badge_i(62, top + 22, IC_LOCK, "COMING SOON", UI_AMBER, UI_INK);
    else if (have[pk] < 0) ui_badge_i(62, top + 22, IC_LOCK, "LOOKING AT THE DISC", UI_SOFT, UI_WHITE);
    else ui_badge_i(62, top + 22, IC_LOCK, "NOT ADDED", UI_BADT, UI_WHITE);
    int y = top + 46; y += ui_wrap(14, y, 292, 4, c->what, UI_INK) * 10 + 4;
    if (soon) {   // what is kept for it
        ui_section(14, y + 2, 292, "KEPT FOR IT");
        ui_text(14, y + 18, 1, "A SLOT ON THE DISC:", UI_SOFT); ui_text(130, y + 18, 1, c->slot, UI_INK);
        sprintf(b, "PK%d: 8 BYTES", pk); ui_text(14, y + 30, 1, "ROOM IN YOUR SAVE:", UI_SOFT); ui_text(130, y + 30, 1, b, UI_INK);
        ui_text(14, y + 42, 1, "ITEM CODES:", UI_SOFT); ui_text(130, y + 42, 1, c->item, UI_INK);
        ui_wrap(14, y + 58, 292, 3, "OLD SAVES LOAD AS THEY ARE AND NOTHING CHANGES IN THE GAME UNTIL IT COMES. WHEN IT DOES, IT IS A PATCH ON THE BASE DISC LIKE THE OTHER PACKS.", UI_SOFT);
        ui_hints(160, 220, "O BACK", 1); return; }
    if (c->how[0]) { ui_text(14, y, 1, "OPEN IT:", UI_BLUE2); ui_wrap(64, y, 242, 1, c->how, UI_INK); y += 14; }
    if (!ok) { snprintf(b, sizeof b, "PATCH BORE2-%s.PPF ON TO ADD IT", PPPF[pk]); ui_wrap(14, y + 4, 292, 3, have[pk] < 0 ? "ONE MOMENT..." : b, UI_BADT);
               ui_wrap(14, y + 34, 292, 3, "A PACK WORKS ALONE, IN ANY ORDER, ON THE BASE DISC. THE FULL DISC HAS THEM ALL.", UI_SOFT); ui_hints(160, 220, live ? "LEFT RIGHT PACKS|O BACK" : "O BACK", 1); return; }
    ui_section(14, y, 292, "PACK GOALS"); y += 14;
    for (int g = 0; g < NG[pk]; g++) { int yy = y + g * 13, d = gdone(pk, g), cl = gclaimed(pk, g);
        if (cl) ui_icon_disc(20, yy + 5, 12, IC_TICK, UI_GREEN, UI_WHITE); else if (d) ui_icon_disc(20, yy + 5, 12, IC_MONEY, UI_AMBER, UI_WHITE); else ui_disc(20, yy + 5, 10, UI_RGB(186, 196, 210));
        ui_text(32, yy, 1, GL[pk][g].t, cl ? UI_GOODT : d ? UI_INK : UI_SOFT);
        sprintf(b, "$%d", GL[pk][g].pay); ui_text(304 - ui_tw(1, b), yy, 1, b, cl ? UI_GOODT : d ? UI_GOLDT : UI_SOFT); }
    if (pk_ext[pk] >> 7 & 1) ui_badge_i(214, top + 22, IC_STAR, "PACK MASTER", UI_AMBER, UI_INK);
    if (line[0] && msgT > 0) ui_textc(160, 208, 1, line, UI_GOODT);
    else if (!live) ui_textc(160, 208, 1, "CLAIM GOALS IN A LIFE: PHONE, GOALS, R1", UI_SOFT);
    ui_hints(160, 220, live ? (nready(pk) ? "X CLAIM|LEFT RIGHT PACKS|O BACK" : "LEFT RIGHT PACKS|O BACK") : "O BACK", 1);
}
void pki_draw(void) { bg(); if (mode == 0) list_draw(); else page_draw(); }
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, pki_open, pki_update, pki_draw };
