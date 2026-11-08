// paradise_ui: ISLAND PARADISE, an overlay (PARADISE.OVL). It stands on its own: it needs nothing but the base game.
//   RESORT:  twelve luxuries to build, from sun loungers to a royal suite. They earn money every day, and the earnings wait to be collected here.
//   ISLES:   cut-off islets out at sea that may be turned into a Paradise of their own (each world rolls its own: not every world has them all).
//            Move the resort to one and it earns more, and its perk keeps you happy.
//   CASTAWAY CLUB: goals for the island survival mode (secrets, shells, the raft, stores...). Each is paid once per island life, and
//            the club's rank takes money off the resort's prices.
// Everything it keeps is in pk_data (sim.h), which the save carries.
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "ovl.h"
#include "world.h"
#include "island.h"
#include "items.h"
#include <psxgpu.h>
#include <psxpad.h>
#include <stdio.h>
#include <string.h>

enum { T_RESORT, T_ISLES, T_CLUB, T_N };
#define NF 12
#define NI 6
#define NC 12
static const char *const FNM[NF] = { "SUN LOUNGERS", "TIKI BAR", "PRIVATE BEACH", "INFINITY POOL", "SPA PAVILION", "LAGOON FALLS", "OVERWATER VILLAS", "YACHT MARINA", "SEAPLANE DOCK", "STAR DECK", "ROYAL SUITE", "GRAND BALLROOM" };
static const char *const FHINT[NF] = { "COMFY CHAIRS IN THE SUN.", "COCONUTS, UMBRELLAS, A BARMAN.", "SOFT SAND, NO CROWDS.", "THE EDGE MEETS THE SEA.", "HOT STONES AND QUIET.", "A WATERFALL INTO A LAGOON.", "SLEEP OVER THE WAVES.", "BIG BOATS, BIGGER TIPS.", "GUESTS FLY RIGHT IN.", "TELESCOPES AND A NIGHT SKY.", "FOR KINGS AND POP STARS.", "GALAS UNDER CHANDELIERS." };
static const int FCOST[NF] = { 800, 2500, 4000, 6500, 9000, 14000, 20000, 32000, 45000, 60000, 85000, 120000 };
static const int FINC[NF] = { 40, 120, 180, 260, 350, 520, 800, 1100, 1500, 1900, 2600, 3500 };
static const char *const INM[NI] = { "SUNSET COVE", "PEARL LAGOON", "VOLCANO KEY", "CORAL ATOLL", "MOONLIT ISLE", "GOLDEN SHORES" };
static const char *const IHINT[NI] = { "WARM EVENINGS. A LITTLE MORE INCOME.", "CALM WATER. GUESTS FEEL BETTER.", "FIERY VIEWS. BIG INCOME.", "A REEF TO DIVE. GUESTS RETURN OFTEN.", "NIGHT PARTIES. HAPPY HOSTS.", "THE RICHEST GUESTS OF ALL." };
static const int IPCT[NI] = { 120, 110, 160, 130, 125, 175 }, IMOOD[NI] = { 5, 20, 0, 8, 25, 10 }, ICOST[NI] = { 3000, 4500, 9000, 6000, 7500, 12000 };
static const char *const CNM[NC] = { "FIRST SECRET", "HALF THE SECRETS", "EVERY SECRET", "SHELL COLLECTOR", "WELL STOCKED", "PLENTY TO EAT", "SPEAR FISHER", "RAFT BUILDER", "A WEEK ASHORE", "TREASURE HUNTER", "THE GOLDEN IDOL", "THE ISLET" };
static const char *const CHINT[NC] = { "FIND ONE SECRET.", "FIND FOUR SECRETS.", "FIND ALL EIGHT SECRETS.", "PICK UP ALL SIX SEASHELLS.", "HOLD 20 WOOD, 10 STONE, 10 LEAVES.", "HOLD 10 FOOD.", "MAKE A SPEAR.", "MAKE THE RAFT.", "BE ON THE ISLAND ON DAY 7.", "DIG UP THE TREASURE.", "FIND THE GOLDEN IDOL.", "SAIL TO THE ISLET." };
static const int CPAY[NC] = { 300, 800, 2500, 400, 400, 300, 200, 500, 600, 500, 1000, 400 };
static const char *const RANK[5] = { "LANDLUBBER", "DRIFTER", "FORAGER", "SURVIVOR", "ISLAND LEGEND" };
// pk_data: 0-1 resort luxuries, 2 isle (0: none), 3-4 earnings collected up to day, 5-6 club goals paid, 7 seed set, 8-11 the world's seed, 12-13 isles seen
static int tab, sel, anim; static char line[48];
static u32 seed(void) { return (u32)pk_data[8] | (u32)pk_data[9] << 8 | (u32)pk_data[10] << 16 | (u32)pk_data[11] << 24; }
static int have(void) { return pk_data[0] | pk_data[1] << 8; }
static int paid(void) { return pk_data[5] | pk_data[6] << 8; }
static int popc(int v) { int n = 0; while (v) { n += v & 1; v >>= 1; } return n; }
static int last(void) { return pk_data[3] | pk_data[4] << 8; }
static int isle_ok(int i) { return (int)(((seed() >> (i * 3)) * 2654435761u + (u32)i * 97u) >> 20) % 100 < 58 || i == 0; }   // (a roll per world: sunset cove always, the rest by chance)
static int stars(void) { return 1 + popc(have()) / 3; }
static int rate(void) { int r = 0; for (int i = 0; i < NF; i++) if (have() >> i & 1) r += FINC[i]; int pct = pk_data[2] ? IPCT[pk_data[2] - 1] : 100; return r * pct / 100; }
static int cr_ok(int i) {
    switch (i) {
        case 0: return isl_secret_count() >= 1; case 1: return isl_secret_count() >= 4; case 2: return isl_secret_count() >= SEC_N;
        case 3: return isl_shells >= ISL_SHELLS; case 4: return isl_wood >= 20 && isl_stone >= 10 && isl_leaves >= 10; case 5: return isl_food >= 10;
        case 6: return isl_spear != 0; case 7: return world_count(CH_RAFT) > 0; case 8: return world_zone == ZONE_ISLAND && day_count >= 7;
        case 9: return isl_secrets >> SEC_TREASURE & 1; case 10: return isl_secrets >> SEC_GOLD & 1; default: return isl_secrets >> SEC_ISLET & 1;
    }
}
static int disc(int c) { int n = popc(paid()); return c * (100 - MIN(30, n * 3)) / 100; }   // (the club's rank: up to 30% off the resort)
void paradise_open(int a) {
    tab = a == 1 ? T_CLUB : T_RESORT; sel = 0; anim = 0; line[0] = 0;
    if (!pk_data[7]) { u32 s = (u32)day_count * 2654435761u + (u32)cash * 40503u + 0x5EEDu; for (int i = 0; i < 4; i++) pk_data[8 + i] = (u8)(s >> (8 * i)); pk_data[7] = 1; pk_data[3] = (u8)day_count; pk_data[4] = (u8)(day_count >> 8); }
}
static int nrows(void) { return tab == T_RESORT ? NF + 1 : tab == T_ISLES ? NI : NC; }
static void act(void) {
    line[0] = 0;
    if (tab == T_RESORT) {
        if (sel == NF) {   // the earnings since the last visit
            int days = MIN(30, day_count - last()); if (days < 0) days = 0;
            if (!have()) { strcpy(line, "BUILD SOMETHING LOVELY FIRST."); return; }
            if (!days) { strcpy(line, "NO GUESTS HAVE PAID YET TODAY."); return; }
            int pay = rate() * days; cash += pay; pk_data[3] = (u8)day_count; pk_data[4] = (u8)(day_count >> 8);
            sims[player].mood = MIN(100, sims[player].mood + 10 + (pk_data[2] ? IMOOD[pk_data[2] - 1] : 0));
            sprintf(line, "%d DAYS OF GUESTS: +$%d", days, pay); audio_voice(VOICE_WIN); return; }
        if (have() >> sel & 1) { strcpy(line, "ALREADY BUILT."); return; }
        int c = disc(FCOST[sel]); if (cash < c) { strcpy(line, "NOT ENOUGH CASH."); audio_voice(VOICE_NAH); return; }
        if (have() && day_count > last()) { int days = MIN(30, day_count - last()), pay = rate() * days; cash += pay; }   // (what was earned so far is paid before the rate changes)
        pk_data[3] = (u8)day_count; pk_data[4] = (u8)(day_count >> 8);
        cash -= c; pk_data[sel >> 3] |= (u8)(1 << (sel & 7)); sprintf(line, "BUILT: %s", FNM[sel]); audio_voice(VOICE_FINISHED); return;
    }
    if (tab == T_ISLES) {
        if (!isle_ok(sel)) { strcpy(line, "NOT IN THIS WORLD."); return; }
        if (pk_data[2] == sel + 1) { strcpy(line, "YOUR RESORT IS HERE."); return; }
        if (cash < ICOST[sel]) { strcpy(line, "NOT ENOUGH CASH."); audio_voice(VOICE_NAH); return; }
        if (have() && day_count > last()) { int days = MIN(30, day_count - last()); cash += rate() * days; pk_data[3] = (u8)day_count; pk_data[4] = (u8)(day_count >> 8); }
        cash -= ICOST[sel]; pk_data[2] = (u8)(sel + 1); pk_data[12] |= (u8)(1 << sel); sprintf(line, "THE RESORT MOVES TO %s!", INM[sel]); audio_voice(VOICE_YAHOO); return;
    }
    if (paid() >> sel & 1) { strcpy(line, "PAID ALREADY."); return; }
    if (!cr_ok(sel)) { strcpy(line, "NOT YET. KEEP SURVIVING."); audio_voice(VOICE_NAH); return; }
    { int p = paid() | 1 << sel; pk_data[5] = (u8)p; pk_data[6] = (u8)(p >> 8); }
    cash += CPAY[sel]; sims[player].mood = MIN(100, sims[player].mood + 15); sprintf(line, "GOAL! +$%d", CPAY[sel]); audio_voice(VOICE_WIN);
}
int paradise_update(int hd, int pr, int steps) {
    (void)hd; anim += steps;
    if (pr & PAD_CIRCLE) return 0;
    if (pr & (PAD_R1 | PAD_RIGHT)) { tab = (tab + 1) % T_N; sel = 0; line[0] = 0; }
    if (pr & (PAD_L1 | PAD_LEFT)) { tab = (tab + T_N - 1) % T_N; sel = 0; line[0] = 0; }
    int n = nrows();
    if (pr & PAD_UP) sel = (sel + n - 1) % n; if (pr & PAD_DOWN) sel = (sel + 1) % n;
    if (pr & PAD_CROSS) act();
    return 1;
}
void paradise_draw(void) {
    char b[56]; static const char *const TN[T_N] = { "RESORT", "ISLES", "CASTAWAY CLUB" };
    gfx_set_clear(40, 150, 200);   // a sunset sky over a lagoon
    ui_round(0, 0, 320, 240, 0, UI_RGB(255, 190, 120), UI_RGB(70, 190, 210), 0);
    ui_round(0, 150, 320, 90, 0, UI_RGB(30, 150, 200), UI_RGB(20, 110, 170), 0);
    ui_round(252, 30, 40, 40, 20, UI_RGB(255, 235, 150), UI_RGB(255, 200, 90), 0);
    ui_round(210, 138, 90, 24, 12, UI_RGB(240, 214, 150), UI_RGB(210, 180, 120), 0); ui_round(246, 100, 5, 42, 2, UI_RGB(120, 80, 40), UI_RGB(90, 60, 30), 0); ui_round(228, 92, 40, 12, 6, UI_RGB(50, 160, 70), UI_RGB(30, 120, 50), 0);
    int top = ui_window(8, 6, 304, 228, "ISLAND PARADISE", IC_SUN);
    { int x = 14; for (int i = 0; i < T_N; i++) { int w = ui_tw(1, TN[i]) + 14, on = i == tab; ui_round(x, top, w, 14, 4, on ? UI_BLUE : UI_PANEL, on ? UI_BLUE2 : UI_PANEL, 0); ui_text_box(x, top, w, 14, TX_CENTER | TX_MID, 1, TN[i], on ? UI_WHITE : UI_SOFT); x += w + 4; } }
    sprintf(b, "$%d", (int)cash); ui_text_box(220, top, 80, 14, TX_RIGHT | TX_MID, 1, b, UI_GOODT);
    int n = nrows(), first = CLAMP(sel - 3, 0, MAX(0, n - 7)), y0 = top + 20;
    for (int k = 0; k < 7 && first + k < n; k++) {
        int i = first + k, y = y0 + k * 15, on = i == sel;
        if (on) ui_round(14, y - 2, 292, 15, 4, UI_RGB(226, 242, 254), UI_HILITE, 0);
        if (tab == T_RESORT) {
            if (i == NF) { int days = MIN(30, MAX(0, day_count - last())); sprintf(b, "COLLECT EARNINGS  %d DAYS = $%d", days, rate() * days); ui_text_box(20, y - 2, 240, 15, TX_LEFT | TX_MID, 1, b, on ? UI_BLUE2 : UI_GOODT); continue; }
            int own = have() >> i & 1; ui_text_box(20, y - 2, 240, 15, TX_LEFT | TX_MID, 1, FNM[i], own ? UI_GOODT : on ? UI_BLUE2 : UI_INK);
            if (own) ui_text_box(200, y - 2, 100, 15, TX_RIGHT | TX_MID, 1, "BUILT", UI_GOODT); else { sprintf(b, "$%d", disc(FCOST[i])); ui_text_box(200, y - 2, 100, 15, TX_RIGHT | TX_MID, 1, b, cash >= disc(FCOST[i]) ? UI_INK : UI_BADT); }
        } else if (tab == T_ISLES) {
            int ok = isle_ok(i); ui_text_box(20, y - 2, 240, 15, TX_LEFT | TX_MID, 1, ok ? INM[i] : "???", pk_data[2] == i + 1 ? UI_GOODT : !ok ? UI_SOFT : on ? UI_BLUE2 : UI_INK);
            if (!ok) ui_text_box(200, y - 2, 100, 15, TX_RIGHT | TX_MID, 1, "NO ISLE", UI_SOFT); else if (pk_data[2] == i + 1) ui_text_box(200, y - 2, 100, 15, TX_RIGHT | TX_MID, 1, "HOME", UI_GOODT); else { sprintf(b, "$%d", ICOST[i]); ui_text_box(200, y - 2, 100, 15, TX_RIGHT | TX_MID, 1, b, cash >= ICOST[i] ? UI_INK : UI_BADT); }
        } else {
            int dn = paid() >> i & 1, rd = cr_ok(i); ui_text_box(20, y - 2, 240, 15, TX_LEFT | TX_MID, 1, CNM[i], dn ? UI_GOODT : rd ? UI_BLUE2 : on ? UI_INK : UI_SOFT);
            if (dn) ui_text_box(200, y - 2, 100, 15, TX_RIGHT | TX_MID, 1, "PAID", UI_GOODT); else { sprintf(b, rd ? "CLAIM $%d" : "$%d", CPAY[i]); ui_text_box(200, y - 2, 100, 15, TX_RIGHT | TX_MID, 1, b, rd ? UI_GOODT : UI_SOFT); }
        }
    }
    int by = top + 128; ui_round(14, by, 292, 56, 5, UI_PANEL, UI_LINE, 0);
    if (tab == T_RESORT) { sprintf(b, stars() == 1 ? "%d STAR  $%d A DAY%s" : "%d STARS  $%d A DAY%s", stars(), rate(), pk_data[2] ? "  AT" : ""); ui_text_box(20, by + 2, 280, 12, TX_LEFT | TX_MID, 1, b, UI_BLUE2); if (pk_data[2]) ui_text_box(20 + ui_tw(1, b) + 4, by + 2, 120, 12, TX_LEFT | TX_MID, 1, INM[pk_data[2] - 1], UI_GOODT);
        ui_wrap(20, by + 17, 280, 2, sel < NF ? FHINT[sel] : "THE GUESTS PAY EVERY DAY. COME BACK AND COLLECT.", UI_INK); }
    else if (tab == T_ISLES) { ui_text_box(20, by + 2, 280, 12, TX_LEFT | TX_MID, 1, "SMALL ISLES CUT OFF FROM THE WORLD", UI_BLUE2); ui_wrap(20, by + 17, 280, 2, isle_ok(sel) ? IHINT[sel] : "THIS WORLD HAS NO SUCH ISLE.", UI_INK); }
    else { int n2 = popc(paid()); sprintf(b, "RANK: %s  (%d OF %d)", RANK[MIN(4, n2 / 3)], n2, NC); ui_text_box(20, by + 2, 280, 12, TX_LEFT | TX_MID, 1, b, UI_BLUE2); ui_wrap(20, by + 17, 280, 2, CHINT[sel], UI_INK); }
    ui_text_box(20, by + 40, 280, 12, TX_LEFT | TX_MID, 1, line, UI_RED);
    ui_hints(160, 218, "X PICK|L1 R1 TABS|O LEAVE", 1);
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, paradise_open, paradise_update, paradise_draw };
