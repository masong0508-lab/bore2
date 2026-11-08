// realm.c - KINGDOM MODE in 3D: see realm.h.
// THE LAND: a cell is 96 world units (a Bore, 384 tall, stands four cells high: a giant), cell (x, y) at world (x * 96, -y * 96).
// Only the cells round the giant are drawn (a square 23 across, fogged at its edge), each as one floor quad, with trees, rocks,
// crystals, roads, bridges and the buildings stood on top as boxes and roofs. The Nubbins are little two-box folk: the carriers
// on the roads (coloured by what they carry), the residents round their homes, a worker at every staffed workplace.
#include "realm.h"
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "voxel.h"
#include "audio.h"
#include "save.h"
#include "font.h"
#include "crowd.h"
#include "atlas.h"
#include "citizens.h"
#include "realm/kingdom.h"
#include "realm/kingsim.h"
#include "realm/kinglog.h"
#include "realm/kinghand.h"

#define CELL 96
#define VIEW 9
static KRealm R; static KBld *Bl; static u8 *cells;   // (the buildings and   // (the land, up to 255 x 255: in the city crowd's arena, which sleeps while you are in the realm)
extern KSim realm_S; extern int realm_blob_len, realm_mis, realm_live;   // (realm_res.c: kept while the overlay is not loaded)
#define S realm_S
#define blob_len realm_blob_len
#define mis realm_mis
#define live realm_live
static KHand hand;
static void nub_make(void);
// the realm as saved bytes, kept with the life while you are not in it: they live in the simulation's
// distance-map cache (S.dist and S.queue), which is only a cache while the realm runs and is free when it does not (the GBA did the same)
#define blob ((u8 *)S.dist)
#define BLOB_CAP ((int)(sizeof S.dist + sizeof S.queue))
int dbg_realm[4];
static int mis_paid_all;                 // the realm is running; the mission you are on
static int cdist = 1250, walkT;   // the camera's distance (L2 R2 zoom), how long the giant has walked (a run after a while)
static int gx, gz, gyaw, gmove, gspd, gphase, cyaw, cyaw_t, spd, fr, msgT, panel, menu, msel, pick, psel, found_sel;
static const char *msg; static char msgB[64];

// ---- the missions (kingmis.h): ten to the crown, then the royal years ----
enum { QK_KBLD, QK_KROAD, QK_KPOP, QK_KBT, QK_KGOOD, QK_KDELIV, QK_KWORK, QK_KBRIDGE, QK_KCASTLE };
typedef struct { u8 k, a; u16 n; const char *nm; } Tq;
typedef struct { const char *nm, *story; u8 pay; Tq t[3]; } Mis;
#define TQ(k,a,n,nm) {k,a,n,nm}
static const Mis miss[] = {
  { "BIG FEET, SMALL FIELD", "YOU WAKE UP ENORMOUS IN A MUDDY FIELD. A VERY SMALL CROWD IS LOOKING UP AT YOU.", 20, { TQ(QK_KBLD,0,3,"RAISE 3 BUILDINGS"), TQ(QK_KROAD,0,12,"LAY 12 ROAD CELLS"), TQ(QK_KPOP,0,8,"HAVE 8 NUBBINS") } },
  { "TIMBER!", "THE LITTLE FOLK NEED WOOD, AND YOU ARE A WALKING FOREST MANAGEMENT PLAN.", 25, { TQ(QK_KBT,KB_WOODCUT,1,"BUILD A WOODCUTTER"), TQ(QK_KBT,KB_SAWMILL,1,"BUILD A SAWMILL"), TQ(QK_KGOOD,G_PLANKS,10,"HAVE 10 PLANKS") } },
  { "A ROCK AND A HARD PLACE", "THE FOLK ARE BUILDING A VERY SMALL CASTLE IN THEIR HEADS. IT NEEDS ROCKS.", 30, { TQ(QK_KBT,KB_QUARRY,1,"BUILD A QUARRY"), TQ(QK_KGOOD,G_STONE,20,"HAVE 20 STONE"), TQ(QK_KBT,KB_MASON,1,"BUILD A MASON") } },
  { "FLUFF", "THE FOLK HAVE FOUND WOOL AND WANT EVERYTHING SOFTER, INCLUDING YOU.", 35, { TQ(QK_KBT,KB_SHEEP,1,"BUILD A SHEEP PEN"), TQ(QK_KGOOD,G_WOOL,10,"HAVE 10 WOOL"), TQ(QK_KBT,KB_WEAVER,1,"BUILD A WEAVER") } },
  { "SHINY THINGS", "SOMETHING GLITTERS IN THE ROCKS AND THE FOLK WOULD LIKE IT POLISHED, PLEASE.", 40, { TQ(QK_KBT,KB_CRYMINE,1,"BUILD A CRYSTAL MINE"), TQ(QK_KBT,KB_CUTTER,1,"BUILD A CUTTER"), TQ(QK_KGOOD,G_POLISH,4,"HAVE 4 POLISHED") } },
  { "RULES OF THE ROAD", "ONE NUBBIN HAS BEEN SEEN WALKING IN CIRCLES. ROADS, PLEASE.", 45, { TQ(QK_KROAD,0,24,"LAY 24 ROAD CELLS"), TQ(QK_KDELIV,0,20,"DELIVER 20 LOADS"), TQ(QK_KBLD,0,8,"RAISE 8 BUILDINGS") } },
  { "TOO MANY MOUTHS", "THE REALM IS BOOMING. EVERYONE IS HUNGRY AND NOBODY HAS A BED.", 50, { TQ(QK_KPOP,0,15,"HAVE 15 NUBBINS"), TQ(QK_KWORK,0,6,"PUT 6 TO WORK"), TQ(QK_KBT,KB_HOUSE,1,"BUILD A HOUSE") } },
  { "THE FAR BANK", "THE FOLK KEEP STARING AT THE FAR BANK. BUILD A BRIDGE: IT COSTS CUT STONE.", 55, { TQ(QK_KBRIDGE,0,4,"LAY 4 BRIDGE CELLS"), TQ(QK_KBT,KB_STORE,2,"BUILD 2 STOREHOUSES"), TQ(QK_KBLD,0,16,"RAISE 16 BUILDINGS") } },
  { "FOUNDATIONS", "THE FOLK HAVE DRAWN A CASTLE IN THE DIRT. IT IS A VERY GOOD DRAWING.", 60, { TQ(QK_KCASTLE,0,1,"LAY THE FOUNDATION"), TQ(QK_KCASTLE,0,2,"RAISE THE WALLS"), TQ(QK_KPOP,0,10,"HAVE 10 NUBBINS") } },
  { "A CROWN, PROPERLY", "ONE OF THEM HAS MADE A CROWN OUT OF A BOTTLE CAP. TIME FOR A REAL ONE.", 100, { TQ(QK_KCASTLE,0,3,"RAISE THE TOWERS"), TQ(QK_KCASTLE,0,4,"CROWN A MONARCH"), TQ(QK_KPOP,0,25,"RULE 25 NUBBINS") } },
  { "ROYAL TASTES", "THE MONARCH HAS OPINIONS, MOSTLY ABOUT DRINKS.", 60, { TQ(QK_KBT,KB_WELL,1,"BUILD A WELL"), TQ(QK_KBT,KB_TAVERN,1,"BUILD A TAVERN"), TQ(QK_KPOP,0,40,"HAVE 40 NUBBINS") } },
  { "THE LONG WAY ROUND", "EVERY NUBBIN HAS A COUSIN ON THE FAR BANK. EVERY COUSIN IS VISITING.", 65, { TQ(QK_KROAD,0,60,"LAY 60 ROAD CELLS"), TQ(QK_KBRIDGE,0,12,"LAY 12 BRIDGE CELLS"), TQ(QK_KDELIV,0,100,"DELIVER 100 LOADS") } },
  { "A PROPER TOWN", "SOMEBODY PUT UP A SIGN. THE SIGN HAS A TYPO.", 70, { TQ(QK_KBLD,0,30,"RAISE 30 BUILDINGS"), TQ(QK_KWORK,0,10,"PUT 10 TO WORK"), TQ(QK_KPOP,0,80,"HAVE 80 NUBBINS") } },
  { "THE FULL LARDER", "THE MONARCH HAS INSPECTED THE STOCKPILE AND IS MAKING A FACE.", 75, { TQ(QK_KGOOD,G_PLANKS,60,"HAVE 60 PLANKS"), TQ(QK_KGOOD,G_CUT,40,"HAVE 40 CUT STONE"), TQ(QK_KGOOD,G_CLOTH,20,"HAVE 20 CLOTH") } },
  { "STANDING ROOM ONLY", "THE TAVERN IS FULL. SO IS THE SQUARE. SO, SOMEHOW, IS THE CASTLE.", 80, { TQ(QK_KBT,KB_TAVERN,2,"BUILD 2 TAVERNS"), TQ(QK_KBT,KB_HOUSE,12,"BUILD 12 HOUSES"), TQ(QK_KPOP,0,150,"HAVE 150 NUBBINS") } },
  { "A REALM THAT NEVER ENDS", "YOU CAME FOR A FIELD. NOW YOU HAVE A KINGDOM.", 100, { TQ(QK_KBLD,0,50,"RAISE 50 BUILDINGS"), TQ(QK_KPOP,0,300,"RULE 300 NUBBINS"), TQ(QK_KWORK,0,12,"PUT 12 TO WORK") } } };
#undef TQ
#define NMIS ((int)(sizeof miss / sizeof miss[0]))
static int count_task(const Tq *q) {
    int n = 0, d = S.dim;
    switch (q->k) {
    case QK_KBLD: for (int i = 0; i < R.nB; i++) n += Bl[i].lvl ? 1 : 0; return n;
    case QK_KROAD: for (int c = 0; c < d * d; c++) n += ksRoadV(cells[c] & 7) && !(cells[c] & KC_BLD); return n;
    case QK_KBRIDGE: for (int c = 0; c < d * d; c++) n += (cells[c] & 7) == KT_BRIDGE; return n;
    case QK_KPOP: return S.pop;
    case QK_KWORK: return S.emp;
    case QK_KBT: for (int i = 0; i < R.nB; i++) n += Bl[i].type == q->a && (Bl[i].lvl || q->a == KB_CASTLE); return n;
    case QK_KGOOD: return R.goods[q->a];
    case QK_KDELIV: return (int)MIN(S.delivered, 99999u);
    case QK_KCASTLE: return R.castle;
    }
    return 0;
}
static void say(const char *s) { msg = s; msgT = 150; }
static void mission_check(void) {
    if (mis >= NMIS) return;
    const Mis *m = &miss[mis];
    for (int k = 0; k < 3; k++) if (count_task(&m->t[k]) < m->t[k].n) return;
    cash += m->pay * 100;
    snprintf(msgB, sizeof msgB, "MISSION DONE: %s  +$%d", m->nm, m->pay * 100); say(msgB);
    audio_voice(VOICE_WIN); mis++;
}

// ---- the realm in and out of its bytes ----
static void pack(void) {
    if (!live) return;
    ksPrepSave(&S); ksHoldHome(&S);
    R.gx = (u8)CLAMP(gx / CELL, 0, S.dim - 1); R.gy = (u8)CLAMP(-gz / CELL, 0, S.dim - 1);
    int n = kgEncode(blob, BLOB_CAP, &R, Bl, cells);
    if (n > 0) blob_len = n;
}
static int unpack(void) {
    if (blob_len <= 0) return 0;
    if (kgDecode(blob, blob_len, &R, Bl, cells, 0) != KGE_OK) return 0;
    kgDecode(blob, blob_len, &R, Bl, cells, 1);
    if (ksInit(&S, &R, Bl, cells) != KSE_OK) return 0;
    return 1;
}
static void giant_to(int cx, int cy) { gx = cx * CELL + CELL / 2; gz = -(cy * CELL + CELL / 2); }
static int walk_ok(int wx, int wz) {
    int d = S.dim, cx = wx / CELL, cy = -wz / CELL; if (wx < 0 || wz > 0 || cx < 1 || cy < 1 || cx >= d - 1 || cy >= d - 1) return 0;
    int c = cells[cy * d + cx]; return (c & KC_BLD) || (c & 7) != KT_WATER;
}
static void spawn(void) {   // (kinggiant.h kgSpawn) the cell nearest the middle the giant may stand on
    int d = S.dim, c = d / 2;
    if (R.gx != KG_NOPOS && R.gy != KG_NOPOS) { giant_to(R.gx, R.gy); if (walk_ok(gx, gz)) return; }
    for (int r = 0; r < d / 2; r++) for (int y = c - r; y <= c + r; y++) for (int x = c - r; x <= c + r; x++) {
        if (r && x > c - r && x < c + r && y > c - r && y < c + r) continue;
        giant_to(x, y); if (walk_ok(gx, gz)) return; }
    giant_to(c, c);
}
static void found(int tier) {   // a brand new realm (kingplay.h kpSettle): a storehouse, two hut sites, a few cells of road
    memset(&R, 0, sizeof R); R.tier = (u8)tier; R.giant = 4; R.genVer = KG_GENVER; R.gx = R.gy = KG_NOPOS;
    R.seed = (u32)VSync(-1) * 2654435761u ^ (u32)cash * 40503u; if (!R.seed) R.seed = 1; R.rng = R.seed | 1;
    kgGen(cells, R.seed, tier); ksInit(&S, &R, Bl, cells);
    int d = S.dim, c = d / 2;
    for (int y = c - 3; y < c + 5; y++) for (int x = c - 3; x < c + 5; x++) cells[y * d + x] = KT_GRASS;
    ksPlace(&S, KB_STORE, c - 1, c - 1, 1); ksPlace(&S, KB_HUT, c + 2, c - 1, 0); ksPlace(&S, KB_HUT, c - 1, c + 2, 0);
    ksRoadSet(&S, c + 1, c - 1, 1); ksRoadSet(&S, c + 1, c, 1); ksRoadSet(&S, c + 1, c + 1, 1); ksRoadSet(&S, c, c + 1, 1); ksRoadSet(&S, c - 1, c + 1, 1); ksRoadSet(&S, c + 2, c + 1, 1);
    ksGift(&S, G_WOOD, 6);   // (a founding kit: enough for the first hut)
    live = 1; mis = 0; spawn(); gyaw = 2048;
    say("THE NUBBINS LOOK UP AT YOU. GATHER WOOD: WALK TO A TREE, PRESS X");
#ifdef AUTO_TOWERS
    { int n = 0; for (int y = 4; y < d - 4 && n < 300; y += 3) for (int x = 4; x < d - 4 && n < 300; x += 3) {   // (test builds: a city of towers, ten thousand Nubbins)
        int dx = x - c, dy = y - c; if (dx * dx + dy * dy > 60 * 60 || (ABS(dx) < 6 && ABS(dy) < 6)) continue;
        for (int j = 0; j < 2; j++) for (int k = 0; k < 2; k++) { int q = (y + j) * d + x + k; if (!(cells[q] & KC_BLD)) cells[q] = KT_GRASS; }
        if (ksPlace(&S, KB_TOWER, x, y, 1) >= 0) { Bl[R.nB - 1].work = 64; n++; } if ((x & 3) == 1) { ksRoadSet(&S, x + 2, y, 1); } }
      ksCensus(&S); }
#endif
}
void realm_open(void) {
    { u32 n; cells = (u8 *)crowd_scratch(&n); Bl = (KBld *)(cells + 65536); }   // (the crowd's models are let go: they are made again when you are back)
    nub_make();
    khInit(&hand); spd = 0; panel = 0; menu = 0; pick = 0; msgT = 0; fr = 0; cyaw = cyaw_t = 0; found_sel = 1;
    if (!live) live = unpack();
    if (live) { spawn(); say("BACK IN THE REALM"); }
    audio_voice(VOICE_YEHA);
}
static int cursor_cell(int *cx, int *cy) {   // the cell in front of the giant's feet
    int fx = gx + (isin(gyaw) * (CELL * 9 / 10) >> 12), fz = gz - (icos(gyaw) * (CELL * 9 / 10) >> 12);
    *cx = fx / CELL; *cy = -fz / CELL; return *cx >= 0 && *cy >= 0 && *cx < S.dim && *cy < S.dim;
}

// ---- update ----
static const u8 pickModes[] = { KHM_HAND, KHM_ROAD, KHM_HUT, KHM_HOUSE, KHM_WOODCUT, KHM_QUARRY, KHM_SAWMILL, KHM_MASON, KHM_STORE, KHM_SHEEP, KHM_CRYMINE, KHM_WEAVER, KHM_CUTTER, KHM_WELL, KHM_TAVERN, KHM_ROW, KHM_TOWER, KHM_CASTLE, KHM_MOVE, KHM_NUBBIN, KHM_DEMO };
#define NPICK ((int)sizeof pickModes)
int realm_update(int held, int pressed, int steps) {
    fr += steps; if (msgT) msgT = MAX(0, msgT - steps);
    if (!live) {   // FOUND A REALM: pick its size
        if (pressed & PAD_UP) found_sel = (found_sel + 3) % 4;
        if (pressed & PAD_DOWN) found_sel = (found_sel + 1) % 4;
        if (pressed & PAD_CROSS) { found(found_sel); audio_voice(VOICE_YAHOO); }
        if (pressed & PAD_CIRCLE) { crowd_reset(); citizens_rebuild(); return 0; }
        return 1;
    }
    if (menu) {   // START: the realm's menu
        if (pressed & PAD_UP) msel = (msel + 2) % 3;
        if (pressed & PAD_DOWN) msel = (msel + 1) % 3;
        if (pressed & PAD_CIRCLE) menu = 0;
        if (pressed & PAD_CROSS) {
            if (msel == 0) menu = 0;
            else if (msel == 1) { spd = (spd + 1) % 3; }
            else { pack(); live = 0; crowd_reset(); citizens_rebuild(); sim_note("THE REALM WAITS FOR YOU (SAVE YOUR LIFE TO KEEP IT)"); return 0; }
        }
        return 1;
    }
    if (pick) {   // SQUARE: what X does
        if (pressed & PAD_UP) psel = (psel + NPICK - 1) % NPICK;
        if (pressed & PAD_DOWN) psel = (psel + 1) % NPICK;
        if (pressed & PAD_LEFT) psel = MAX(0, psel - 6);
        if (pressed & PAD_RIGHT) psel = MIN(NPICK - 1, psel + 6);
        if (pressed & (PAD_CROSS | PAD_SQUARE)) { hand.mode = pickModes[psel]; pick = 0; audio_voice(VOICE_AGREE); }
        if (pressed & PAD_CIRCLE) pick = 0;
        return 1;
    }
    if (pressed & PAD_START) { menu = 1; msel = 0; return 1; }
    if (pressed & PAD_SQUARE) { pick = 1; for (int i = 0; i < NPICK; i++) if (pickModes[i] == hand.mode) psel = i; return 1; }
    if (pressed & PAD_TRIANGLE) panel = (panel + 1) % 3;
    if (pressed & PAD_SELECT) { spd = (spd + 1) % 3; snprintf(msgB, sizeof msgB, "TIME RUNS X%d", spd == 0 ? 1 : spd == 1 ? 4 : 16); say(msgB); }
    if (held & PAD_L1) cyaw_t -= 40 * steps;   // turn (hold)
    if (held & PAD_R1) cyaw_t += 40 * steps;
    { int dd = cyaw_t - cyaw; cyaw += CLAMP(dd, -64 * steps, 64 * steps); }
    if (held & PAD_L2) cdist = MAX(650, cdist - 24 * steps);    // zoom in
    if (held & PAD_R2) cdist = MIN(2400, cdist + 24 * steps);   // zoom out
    // the giant walks (the pad as the camera sees it)
    int ux = (held & PAD_RIGHT ? 1 : 0) - (held & PAD_LEFT ? 1 : 0), uy = (held & PAD_UP ? 1 : 0) - (held & PAD_DOWN ? 1 : 0);
    gmove = ux || uy; if (!gmove) walkT = 0;
    if (gmove) {
        static const s16 dirY[3][3] = { { 3584, 3072, 2560 }, { 0, 0, 2048 }, { 512, 1024, 1536 } };   // [ux + 1][uy + 1]: up 0, right 1024, down 2048, left 3072
        int want = (dirY[ux + 1][1 - uy] + cyaw) & 4095, dy = ((want - gyaw + 2048) & 4095) - 2048;
        gyaw = (gyaw + CLAMP(dy, -300 * steps, 300 * steps)) & 4095;
        walkT += steps; int sp = (held & PAD_CIRCLE) ? 36 : 18; gspd = sp;   // (O held: a run)
        for (int k = 0; k < steps; k++) {
            int mx = isin(want) * sp >> 12, mz = -(icos(want) * sp >> 12);
            if (walk_ok(gx + mx, gz + mz)) { gx += mx; gz += mz; } else if (walk_ok(gx + mx, gz)) gx += mx; else if (walk_ok(gx, gz + mz)) gz += mz;
            gphase = (gphase + (sp > 20 ? 230 : 150)) & 4095; }
    }
    if (pressed & PAD_CROSS) {
        int cx, cy; if (cursor_cell(&cx, &cy)) {
            int bx = cx, by = cy;
            if (hand.mode >= KHM_HUT && hand.mode <= KHM_TOWER) { bx = cx - 1 + ((isin(gyaw) > 0) ? 1 : 0); by = cy - 1 + ((icos(gyaw) < 0) ? 1 : 0); }   // (a 2 x 2 site: the corner nearest the giant)
            int r = khAct(&hand, &S, bx, by);
            say(khMsg[r < KHR_N ? r : 0]);
            audio_voice(r == KHR_SITE || r == KHR_GIVE || r == KHR_GOT || r == KHR_ROAD ? VOICE_AGREE : r == KHR_SPOT || r == KHR_REFUSE || r == KHR_NOGATHER || r == KHR_NOTHING ? VOICE_NAH : VOICE_FINISHED);
        }
    }
    static const u8 rate[3] = { 1, 4, 16 };
    for (int k = 0; k < steps * rate[spd]; k++) ksStep(&S);
    if (S.ev) { static const char *const cst[5] = { "", "THE CASTLE'S FOUNDATION IS LAID!", "THE CASTLE'S WALLS ARE UP!", "THE TOWERS RISE!", "LONG LIVE THE MONARCH: THE REALM IS CROWNED!" }; say(cst[MIN(S.ev, 4)]); S.ev = 0; audio_voice(VOICE_YAHOO); }
    if (!(fr & 31)) mission_check();
    { int h = 0; for (int i = 0; i < R.nB; i++) h += ksT(Bl[i].type)->kind == KK_HOME; dbg_realm[0] = R.nB; dbg_realm[1] = h; dbg_realm[2] = S.pop; dbg_realm[3] = ksLedger(&S); }
    return 1;
}

// ---- the picture ----
static u32 hsh(int x, int y) { u32 h = (u32)x * 73856093u ^ (u32)y * 19349663u; h ^= h >> 13; h *= 0x5bd1e995u; return h ^ (h >> 15); }
#define HZ_R 200
#define HZ_G 224
#define HZ_B 240
static int hzf;
static int haze_of(int wx, int wz);
static u32 hz(u32 c, int f) { if (f <= 0) return c; int r = c & 255, g = (c >> 8) & 255, b = (c >> 16) & 255; return UI_RGB(r + ((HZ_R - r) * f >> 8), g + ((HZ_G - g) * f >> 8), b + ((HZ_B - b) * f >> 8)); }
static void quad(int x0, int z0, int x1, int z1, int y, u32 c0, u32 c1) {   // a flat quad on the ground (x, z in world units), each corner hazed by its distance
    SVECTOR a = { (short)x0, (short)y, (short)z0 }, b = { (short)x1, (short)y, (short)z0 }, c = { (short)x0, (short)y, (short)z1 }, d = { (short)x1, (short)y, (short)z1 };
    gfx_quad_g(&a, &b, &c, &d, hz(c0, haze_of(x0, z0)), hz(c1, haze_of(x1, z0)), hz(c1, haze_of(x0, z1)), hz(c0, haze_of(x1, z1)), 0);
}
// THE HAZE: things further from the giant fade toward the sky's colour at the horizon, so the land runs off into the distance
// instead of stopping at the edge of what is drawn in full (hzf: 0 clear .. 256 all haze, set per thing by haze_at)
static u32 shade(u32 c, int k) {
    int r = MIN(255, (c & 255) * k >> 8), g = MIN(255, ((c >> 8) & 255) * k >> 8), b = MIN(255, ((c >> 16) & 255) * k >> 8);
    if (hzf) { r += (HZ_R - r) * hzf >> 8; g += (HZ_G - g) * hzf >> 8; b += (HZ_B - b) * hzf >> 8; }
    return UI_RGB(r, g, b); }
static void box(int x0, int y0, int z0, int x1, int y1, int z1, u32 c) {   // y0 the top (up is -y), y1 the bottom; the faces that face the camera
    SVECTOR t0 = { (short)x0, (short)y0, (short)z0 }, t1 = { (short)x1, (short)y0, (short)z0 }, t2 = { (short)x0, (short)y0, (short)z1 }, t3 = { (short)x1, (short)y0, (short)z1 };
    SVECTOR b0 = { (short)x0, (short)y1, (short)z0 }, b1 = { (short)x1, (short)y1, (short)z0 }, b2 = { (short)x0, (short)y1, (short)z1 }, b3 = { (short)x1, (short)y1, (short)z1 };
    u32 top = shade(c, 300), s1 = shade(c, 220), s2 = shade(c, 170);
    gfx_quad_g(&t2, &t3, &t0, &t1, top, top, top, top, 1);
    gfx_quad_g(&t0, &t1, &b0, &b1, s1, s1, s1, s1, 1);   // the -z face
    gfx_quad_g(&t3, &t2, &b3, &b2, s1, s1, s1, s1, 1);   // +z
    gfx_quad_g(&t2, &t0, &b2, &b0, s2, s2, s2, s2, 1);   // -x
    gfx_quad_g(&t1, &t3, &b1, &b3, s2, s2, s2, s2, 1);   // +x
}
static void roof(int x0, int z0, int x1, int z1, int y, int h, u32 c) {   // a pyramid roof
    SVECTOR a = { (short)x0, (short)y, (short)z0 }, b = { (short)x1, (short)y, (short)z0 }, cc = { (short)x0, (short)y, (short)z1 }, d = { (short)x1, (short)y, (short)z1 };
    SVECTOR t = { (short)((x0 + x1) / 2), (short)(y - h), (short)((z0 + z1) / 2) };
    u32 l = shade(c, 260), m = shade(c, 200), k = shade(c, 150);
    gfx_quad_g(&t, &t, &a, &b, l, l, l, l, 0); (void)m;
    gfx_tri_f(&t, &d, &cc, (int)(m & 255), (int)((m >> 8) & 255), (int)((m >> 16) & 255), 0);
    gfx_tri_f(&t, &cc, &a, (int)(k & 255), (int)((k >> 8) & 255), (int)((k >> 16) & 255), 0);
    gfx_tri_f(&t, &b, &d, (int)(k & 255), (int)((k >> 8) & 255), (int)((k >> 16) & 255), 0);
}
// ---- the models (near the giant in full; further off simpler) ----
static void wedge(int x0, int x1, int y, int z0, int z1, int ridge, int h, u32 c) {   // a gable roof along x: two slopes and two ends
    int zm = (z0 + z1) / 2; (void)ridge;
    SVECTOR a = { (short)x0, (short)y, (short)z0 }, b = { (short)x1, (short)y, (short)z0 }, cc = { (short)x0, (short)y, (short)z1 }, d = { (short)x1, (short)y, (short)z1 };
    SVECTOR r0 = { (short)x0, (short)(y - h), (short)zm }, r1 = { (short)x1, (short)(y - h), (short)zm };
    u32 l = shade(c, 270), m = shade(c, 190), k = shade(c, 150);
    gfx_quad_g(&r0, &r1, &a, &b, l, l, l, l, 0);   // the south slope
    gfx_quad_g(&r1, &r0, &d, &cc, m, m, m, m, 0);  // the north slope
    gfx_tri_f(&r0, &a, &cc, (int)(k & 255), (int)((k >> 8) & 255), (int)((k >> 16) & 255), 0);
    gfx_tri_f(&r1, &d, &b, (int)(k & 255), (int)((k >> 8) & 255), (int)((k >> 16) & 255), 0);
}
static void window(int x, int y, int z, int lit) { box(x - 7, y - 9, z - 2, x + 7, y + 9, z, lit ? UI_RGB(250, 220, 120) : UI_RGB(90, 130, 170)); }
static void tree(int wx, int wz, u32 h, int near) {   // a pine (tiers of cones) or a broadleaf (a round crown of boxes), by the cell's dice
    int s = 26 + (int)(h & 15), ht = 120 + (int)((h >> 4) & 63), pine = (h >> 9) & 1;
    u32 g = (h & 0x100) ? UI_RGB(46, 128, 52) : UI_RGB(32, 108, 62);
    if (!near) { roof(wx - s, wz - s, wx + s, wz + s, -20, ht * 3 / 4, g); return; }
    box(wx - 6, -46, wz - 6, wx + 6, 0, wz + 6, UI_RGB(112, 72, 38));
    if (pine) { u32 pg = shade(g, 210);
        roof(wx - s - 6, wz - s - 6, wx + s + 6, wz + s + 6, -30, ht / 2, pg);
        roof(wx - s, wz - s, wx + s, wz + s, -30 - ht / 4, ht / 2, shade(pg, 270));
        roof(wx - s * 3 / 5, wz - s * 3 / 5, wx + s * 3 / 5, wz + s * 3 / 5, -30 - ht / 2, ht / 2, shade(pg, 320)); }
    else { int c = 40 + (int)((h >> 12) & 63);
        box(wx - s, -c - s * 3 / 2, wz - s, wx + s, -c, wz + s, g);
        box(wx - s * 3 / 4 + 10, -c - s * 2 - 8, wz - s * 3 / 4, wx + s * 3 / 4 + 10, -c - s, wz + s * 3 / 4, shade(g, 280));
        box(wx - s - 12, -c - s, wz - s / 2, wx - s + 8, -c - 8, wz + s / 2, shade(g, 220));
        if (h & 0x40) box(wx + s - 6, -c - s, wz - 3, wx + s, -c - s + 6, wz + 3, UI_RGB(220, 40, 40)); }   // (an apple)
}
static void rock(int wx, int wz, u32 h, int near, int crys) {   // a boulder of two or three chunks, a crystal cluster on some
    int s = 24 + (int)(h & 15), t = 28 + (int)((h >> 4) & 31), ox = (int)((h >> 8) & 15) - 8;
    u32 g = (h & 0x200) ? UI_RGB(138, 136, 132) : UI_RGB(124, 128, 136);
    box(wx - s + ox, -t, wz - s, wx + s + ox, 0, wz + s, g);
    if (!near) return;
    box(wx - s * 2 / 3 + ox + 10, -t - s / 2, wz - s / 2, wx + s / 3 + ox + 10, -t + 2, wz + s / 2, shade(g, 280));
    box(wx + s + ox - 4, -t / 2, wz - s / 2 + 6, wx + s + ox + s / 2, 0, wz + s / 2 + 6, shade(g, 200));
    if (crys) for (int k = 0; k < 3; k++) { int cx = wx + (k - 1) * 14 + ox, cz = wz + (k & 1) * 10, ch = 70 + k * 24;
        SVECTOR a = { (short)(cx - 9), (short)-t, (short)cz }, b = { (short)(cx + 9), (short)-t, (short)cz }, c2 = { (short)cx, (short)-t, (short)(cz + 9) }, tp = { (short)(cx + 2), (short)(-t - ch), (short)(cz + 3) };
        int sh = (isin((fr * 60 + k * 1300) & 4095) >> 7);
        gfx_tri_f(&a, &b, &tp, 110 + sh, 230, 255, 0); gfx_tri_f(&b, &c2, &tp, 80, 190 + sh, 245, 0); gfx_tri_f(&c2, &a, &tp, 150, 240, 255, 0); }
}
// the Nubbins as SPRITES: a little 8 x 14 figure in four shirts, two steps of a walk (64 x 14), sorted with the 3D at its feet's depth
static TexRef nub_tex; static int nub_ok, nub_drawn;
static void nub_make(void) {
    if (nub_ok) return; nub_ok = 1;
    nub_tex = atlas_try(64, 14); if (!nub_tex.w) return;
    static u16 px[64 * 14]; memset(px, 0, sizeof px);
    static const u16 shirt[4] = { RGB(28, 8, 8), RGB(6, 14, 28), RGB(8, 22, 8), RGB(28, 22, 6) };
    static const char *const art[14] = { "..hhhh..", ".hhhhhh.", ".hssssh.", ".sksksk.", ".ssssss.", "..ssss..", ".cccccc.", "scccccs.", "scccccs.", ".cccccc.", ".pp..pp.", ".pp..pp.", ".pp..pp.", ".bb..bb." };
    for (int f = 0; f < 8; f++) for (int y = 0; y < 14; y++) for (int x = 0; x < 8; x++) {
        char c = art[y][x]; if (y >= 10 && (f & 1)) { int l = x < 4; if (y >= 12) c = (l ? (x == 0 || x == 1) : 0) ? '.' : c; if (y == 10 + 1 && l) c = x == 2 ? 'p' : c; }   // (the other step: the left leg forward)
        u16 v = c == 'h' ? RGB(6, 4, 2) : c == 's' ? RGB(30, 23, 17) : c == 'k' ? RGB(2, 2, 3) : c == 'c' ? shirt[f >> 1] : c == 'p' ? RGB(8, 8, 14) : c == 'b' ? RGB(5, 3, 2) : 0;
        px[y * 64 + f * 8 + x] = v ? (u16)(v | 0x8000) : 0; }
    atlas_upload(&nub_tex, px);
}
static void nub_sprite(int wx, int wz, int kind, int walk) {   // one Nubbin standing at (wx, wz)
    if (!nub_tex.w || nub_drawn >= 520) return;
    int sx, sy, z = gfx_proj(wx, 0, wz, &sx, &sy); if (z <= 0) return;
    int h = 38 * 230 / z; if (h < 3) return; int w = h * 8 / 14 + 1; if (gfx_wide) w = w * 3 / 4 + 1;
    int f = ((kind & 3) << 1) | (walk & 1), L = 128;
    gfx_ot_sprite(sx - w / 2, sy - h, w, h, nub_tex.tpage, nub_tex.u + f * 8, nub_tex.v, 8, 14, z, L, L, L);
    nub_drawn++;
}
static void nubbin(int wx, int wz, u32 col, int bob) { (void)col; nub_sprite(wx, wz, (int)(col >> 3) & 3, (bob >> 11) & 1); }
static const u32 goodCol[8] = { UI_RGB(150, 100, 50), UI_RGB(150, 150, 156), UI_RGB(245, 245, 240), UI_RGB(120, 230, 250), UI_RGB(214, 170, 100), UI_RGB(196, 196, 206), UI_RGB(200, 60, 80), UI_RGB(170, 250, 255) };
static void house_body(int x0, int z0, int x1, int z1, int wh, u32 wall, u32 rf, int timber, int lit) {   // walls, a gable roof, a door, windows, a chimney
    box(x0, -wh, z1, x1, 0, z0, wall);
    if (timber) { u32 tc = UI_RGB(96, 60, 32); box(x0 - 1, -wh, z0 - 1, x0 + 5, 0, z0 + 1, tc); box(x1 - 5, -wh, z0 - 1, x1 + 1, 0, z0 + 1, tc); box(x0, -wh / 2 - 3, z0 - 1, x1, -wh / 2 + 3, z0 + 1, tc); }
    wedge(x0 - 8, x1 + 8, -wh, z0 + 10, z1 - 10, 0, wh * 2 / 3 + 20, rf);
    int cx = (x0 + x1) / 2;
    box(cx - 11, -40, z0 - 3, cx + 11, 0, z0, UI_RGB(80, 50, 28));   // the door
    window(x0 + 24, -wh / 2 - 12, z0, lit); window(x1 - 24, -wh / 2 - 12, z0, lit);
    box(x1 - 34, -wh - wh * 2 / 3 - 10, (z0 + z1) / 2 - 8, x1 - 20, -wh - 10, (z0 + z1) / 2 + 8, UI_RGB(150, 90, 70));   // the chimney
}
static void building(const KBld *b, int i) {
    int x0 = b->x * CELL + 8, z0 = -(b->y * CELL + 8), x1 = (b->x + 2) * CELL - 8, z1 = -((b->y + 2) * CELL - 8), cx = (x0 + x1) / 2, cz = (z0 + z1) / 2;
    // (z0 is the south edge, the higher z: the doors face south)
    { int t = x0; (void)t; } int zs = z0 - 10, zn = z1 + 10;
    const KsType *T = ksT(b->type);
    int lit = (int)((fr / 600 + i) & 1);
    if (b->type == KB_CASTLE) {   // the castle by its stage: a slab, the walls, the towers, the keep and its crown
        int st = R.castle; u32 sc = UI_RGB(190, 186, 176), sd = UI_RGB(160, 156, 148);
        box(x0, -12, z1, x1, 0, z0, sd);
        if (st >= 2) { box(x0, -110, z1, x0 + 20, -12, z0, sc); box(x1 - 20, -110, z1, x1, -12, z0, sc); box(x0, -110, z0 - 20, cx - 22, -12, z0, sc); box(cx + 22, -110, z0 - 20, x1, -12, z0, sc); box(x0, -110, z1, x1, -12, z1 + 20, sc);
                       box(cx - 22, -110, z0 - 20, cx + 22, -70, z0, sc); box(cx - 18, -66, z0 - 2, cx + 18, -12, z0, UI_RGB(70, 46, 26));   // the gate
                       for (int k = 0; k < 6; k++) { int bx = x0 + 10 + k * (x1 - x0 - 20) / 5; box(bx - 6, -126, z0 - 20, bx + 6, -110, z0, sc); box(bx - 6, -126, z1, bx + 6, -110, z1 + 20, sc); } }   // battlements
        if (st >= 3) for (int k = 0; k < 4; k++) { int tx = k & 1 ? x1 - 22 : x0 + 22, tz = k & 2 ? z1 + 22 : z0 - 22; box(tx - 22, -190, tz - 22, tx + 22, -12, tz + 22, shade(sc, 240)); roof(tx - 28, tz - 28, tx + 28, tz + 28, -190, 70, UI_RGB(60, 70, 170)); window(tx, -150, tz + 22, 1); }
        if (st >= 4) { box(cx - 36, -250, cz - 36, cx + 36, -12, cz + 36, UI_RGB(224, 218, 204)); roof(cx - 42, cz - 42, cx + 42, cz + 42, -250, 100, UI_RGB(200, 40, 50)); window(cx, -200, cz + 36, 1);
                       int fl = (fr * 40) & 4095; box(cx - 2, -420, cz - 2, cx + 2, -350, cz + 2, UI_RGB(110, 70, 36)); box(cx + 2, -420 + (isin(fl) >> 10), cz - 1, cx + 40, -396 + (isin(fl) >> 10), cz + 1, UI_RGB(230, 190, 40)); }
        if (st < 4) { int fl = (fr * 40) & 4095; box(cx - 2, -60, cz - 2, cx + 2, 0, cz + 2, UI_RGB(110, 70, 36)); box(cx + 2, -60 + (isin(fl) >> 10), cz - 1, cx + 30, -44 + (isin(fl) >> 10), cz + 1, UI_RGB(220, 40, 50)); }
        int g = 0, n = 0; ksCost(&S, b, &g, &n); if (st < 4) { int k = ksIn(b); for (int j = 0; j < k && j < 8; j++) box(x1 - 30 - (j & 3) * 14, -12 - (j >> 2) * 12 - 12, z1 + 8, x1 - 18 - (j & 3) * 14, -12 - (j >> 2) * 12, z1 + 20, goodCol[g & 7]); }
        return;
    }
    if (!b->lvl) {   // a site: scaffold poles at the corners, rails between, the delivered goods in a pile, a Nubbin with a plan
        u32 pc = UI_RGB(200, 170, 110);
        for (int k = 0; k < 4; k++) { int px = k & 1 ? x1 : x0, pz = k & 2 ? z1 : z0; box(px - 4, -70, pz - 4, px + 4, 0, pz + 4, pc); }
        box(x0, -64, z0 - 3, x1, -58, z0, pc); box(x0, -64, z1, x1, -58, z1 + 3, pc);
        quad(x0, z1, x1, z0, -1, UI_RGB(150, 120, 80), UI_RGB(140, 110, 72));
        int g = 0, n = 0; ksCost(&S, b, &g, &n); int k = ksIn(b);
        for (int j = 0; j < k; j++) box(cx - 30 + (j % 4) * 16, -14 - (j / 4) * 14, cz - 6, cx - 18 + (j % 4) * 16, -(j / 4) * 14, cz + 6, goodCol[g & 7]);
        nub_sprite(cx + 50, zs - 6, 3, (fr >> 4) & 1);
        return;
    }
    switch (b->type) {
    case KB_HUT: house_body(x0 + 24, zs - 14, x1 - 24, zn + 14, 54, UI_RGB(186, 146, 96), UI_RGB(206, 176, 76), 0, lit); break;   // (a thatched roof)
    case KB_HOUSE: house_body(x0 + 10, zs, x1 - 10, zn, 96, UI_RGB(234, 224, 200), UI_RGB(176, 64, 52), 1, lit); break;
    case KB_ROW: for (int k = 0; k < 3; k++) { int a = x0 + k * (x1 - x0) / 3, bb = a + (x1 - x0) / 3 - 4; static const u32 rc[3] = { UI_RGB(200, 120, 90), UI_RGB(220, 200, 160), UI_RGB(160, 170, 190) };
                   house_body(a + 2, zs, bb, zn, 110, rc[k], UI_RGB(90, 80, 90), 0, lit ^ k); } break;
    case KB_TOWER: { int th = 380; box(x0 + 14, -th, zn, x1 - 14, 0, zs, UI_RGB(206, 200, 188));
                   for (int f = 0; f < 6; f++) { int y = -40 - f * 58; for (int k = 0; k < 4; k++) window(x0 + 36 + k * 36, y, zs, ((f + k + i + fr / 300) % 3) == 0); box(x0 + 12, y - 30, zs - 2, x1 - 12, y - 26, zs + 2, UI_RGB(170, 160, 150)); }
                   roof(x0 + 6, zn - 8, x1 - 6, zs + 8, -th, 80, UI_RGB(60, 90, 160)); box(cx - 14, -44, zs - 3, cx + 14, 0, zs, UI_RGB(80, 50, 28)); } break;
    case KB_STORE: house_body(x0 + 8, zs, x1 - 8, zn, 100, UI_RGB(196, 160, 110), UI_RGB(60, 90, 160), 1, 0);
                   for (int k = 0; k < 4; k++) box(x0 - 10 + k * 26, -24 - (k & 1) * 22, zs - 30, x0 + 12 + k * 26, -(k & 1) * 22, zs - 8, UI_RGB(170, 120, 60)); break;   // crates at the door
    case KB_WOODCUT: house_body(x0 + 20, zs, cx + 10, zn, 70, UI_RGB(150, 104, 60), UI_RGB(90, 120, 60), 0, lit);
                   for (int k = 0; k < 3; k++) box(cx + 30, -16 - k * 16, zn + 10 + k * 4, x1, -k * 16, zn + 26 + k * 4, UI_RGB(140, 90, 50));   // the log pile
                   box(cx + 40, -24, zs - 30, cx + 64, 0, zs - 6, UI_RGB(150, 110, 70)); box(cx + 50, -50, zs - 20, cx + 54, -24, zs - 16, UI_RGB(170, 170, 180)); break;   // a stump and its axe
    case KB_SAWMILL: house_body(x0 + 10, zs, x1 - 10, zn, 80, UI_RGB(176, 126, 76), UI_RGB(120, 80, 50), 1, lit);
                   { int a = (fr * 200) & 4095; for (int k = 0; k < 4; k++) { int an = a + k * 1024; box(x1 - 4, -60 + (icos(an) * 34 >> 12) - 6, cz + (isin(an) * 34 >> 12) - 6, x1 + 4, -60 + (icos(an) * 34 >> 12) + 6, cz + (isin(an) * 34 >> 12) + 6, UI_RGB(110, 80, 50)); } } break;   // the water wheel
    case KB_QUARRY: quad(x0 + 10, zn, x1 - 10, zs, -1, UI_RGB(120, 116, 110), UI_RGB(100, 96, 92));
                   for (int k = 0; k < 3; k++) box(x0 + 20 + k * 40, -20 - k * 8, zn + 20, x0 + 50 + k * 40, 0, zn + 50, UI_RGB(150, 150, 154));
                   box(cx - 3, -110, cz - 3, cx + 3, 0, cz + 3, UI_RGB(110, 70, 36)); box(cx - 40, -110, cz - 3, cx + 3, -104, cz + 3, UI_RGB(110, 70, 36)); box(cx - 42, -104, cz - 2, cx - 38, -50, cz + 2, UI_RGB(60, 60, 60)); break;   // a crane
    case KB_MASON: house_body(x0 + 14, zs, x1 - 14, zn, 76, UI_RGB(170, 168, 160), UI_RGB(100, 100, 120), 0, lit); box(x0 - 6, -20, zs - 30, x0 + 30, 0, zs - 8, UI_RGB(196, 196, 206)); break;
    case KB_SHEEP: { u32 fc = UI_RGB(170, 128, 76);
        for (int k = 0; k < 5; k++) { int px = x0 + k * (x1 - x0) / 4; box(px - 3, -34, zs - 3, px + 3, 0, zs + 3, fc); box(px - 3, -34, zn - 3, px + 3, 0, zn + 3, fc); }
        box(x0, -28, zs - 2, x1, -24, zs + 2, fc); box(x0, -28, zn - 2, x1, -24, zn + 2, fc); box(x0 - 2, -28, zn, x0 + 2, -24, zs, fc); box(x1 - 2, -28, zn, x1 + 2, -24, zs, fc);
        for (int k = 0; k < 4; k++) { int sx = x0 + 34 + k * 38 + (isin((fr * 10 + k * 1400) & 4095) >> 9), sz = cz + ((k & 1) ? 26 : -26);
            box(sx - 15, -26, sz - 10, sx + 15, -8, sz + 10, UI_RGB(248, 248, 242)); box(sx + 13, -28, sz - 6, sx + 23, -16, sz + 6, UI_RGB(40, 40, 40)); box(sx - 10, -8, sz - 6, sx - 6, 0, sz + 6, UI_RGB(40, 40, 40)); box(sx + 6, -8, sz - 6, sx + 10, 0, sz + 6, UI_RGB(40, 40, 40)); } } break;
    case KB_CRYMINE: { box(x0 + 20, -60, zn, x1 - 20, 0, cz, UI_RGB(110, 110, 118)); box(cx - 24, -50, cz - 4, cx + 24, 0, cz, UI_RGB(20, 18, 24));   // the mine mouth
        box(cx - 28, -56, cz - 6, cx - 22, 0, cz, UI_RGB(110, 70, 36)); box(cx + 22, -56, cz - 6, cx + 28, 0, cz, UI_RGB(110, 70, 36)); box(cx - 28, -60, cz - 6, cx + 28, -52, cz, UI_RGB(110, 70, 36));
        box(cx - 6, -3, cz, cx + 6, 0, zs, UI_RGB(70, 60, 50)); box(cx - 14, -22, zs - 40, cx + 14, -4, zs - 20, UI_RGB(90, 80, 70)); box(cx - 10, -30, zs - 36, cx + 10, -22, zs - 24, UI_RGB(120, 230, 250)); } break;   // rails and a cart of crystal
    case KB_WEAVER: house_body(x0 + 12, zs, x1 - 12, zn, 80, UI_RGB(214, 186, 206), UI_RGB(160, 70, 140), 1, lit);
                   for (int k = 0; k < 3; k++) box(x0 + 4 + k * 22, -70, zs - 24, x0 + 20 + k * 22, -20, zs - 22, k == 1 ? UI_RGB(200, 60, 80) : UI_RGB(80, 120, 200)); break;   // cloth on a line
    case KB_CUTTER: house_body(x0 + 12, zs, x1 - 12, zn, 80, UI_RGB(160, 206, 224), UI_RGB(70, 150, 190), 0, lit); { SVECTOR a = { (short)(x1 - 40), -110, (short)cz }, c2 = { (short)(x1 - 20), -110, (short)cz }, t = { (short)(x1 - 30), -150, (short)cz }; gfx_tri_f(&a, &c2, &t, 170, 250, 255, 0); } break;
    case KB_WELL: { box(cx - 30, -28, cz - 30, cx + 30, 0, cz + 30, UI_RGB(150, 150, 160)); quad(cx - 22, cz + 22, cx + 22, cz - 22, -29, UI_RGB(40, 90, 200), UI_RGB(60, 120, 220));
        box(cx - 30, -96, cz - 3, cx - 24, -28, cz + 3, UI_RGB(110, 70, 36)); box(cx + 24, -96, cz - 3, cx + 30, -28, cz + 3, UI_RGB(110, 70, 36)); box(cx - 30, -80, cz - 2, cx + 30, -76, cz + 2, UI_RGB(110, 70, 36));
        box(cx - 6, -70, cz - 6, cx + 6, -58, cz + 6, UI_RGB(140, 100, 60)); wedge(cx - 40, cx + 40, -96, cz + 34, cz - 34, 0, 34, UI_RGB(150, 60, 40)); } break;
    case KB_TAVERN: house_body(x0 + 8, zs, x1 - 8, zn, 100, UI_RGB(170, 110, 70), UI_RGB(130, 46, 36), 1, 1);
                   box(cx + 30, -84, zs - 30, cx + 34, -60, zs - 4, UI_RGB(60, 40, 20)); box(cx + 20, -60, zs - 34, cx + 44, -40, zs - 30, UI_RGB(230, 190, 40));   // its sign
                   box(x0 + 10, -24, zs - 40, x0 + 40, -20, zs - 20, UI_RGB(140, 90, 50)); box(x0 + 22, -20, zs - 34, x0 + 28, 0, zs - 26, UI_RGB(110, 70, 36)); break;   // a table outside
    }
    // the folk: residents round a home (a crowd at the big ones), a worker at a staffed workplace
    if (T->kind == KK_HOME) { int n = b->work < 6 ? b->work : 6 + (b->work - 6) / 5;
        for (int k = 0; k < n; k++) { int a = (fr * (5 + (k & 3)) + k * 977 + i * 700) & 4095, r = 120 + (k % 3) * 26; nub_sprite(cx + (isin(a) * r >> 12), cz + (icos(a) * r >> 12), (k + i) & 3, (fr / 8 + k) & 1); } }
    if ((T->kind == KK_GATHER || T->kind == KK_PROC) && b->work) nub_sprite(cx + 44, zs - 30, i & 3, (fr / 10) & 1);
    if ((T->kind == KK_GATHER || T->kind == KK_PROC) && ksOut(b)) for (int j = 0; j < ksOut(b) && j < 6; j++) box(x1 + 4, -12 - j * 10, cz - 6, x1 + 18, -2 - j * 10, cz + 6, goodCol[T->outG & 7]);
}
static int haze_of(int wx, int wz) {   // 0 near the giant .. 236 far off (a faint silhouette stays)
    int dx = (wx - gx) >> 4, dz = (wz - gz) >> 4, d = isqrt(dx * dx + dz * dz) << 4;
    return CLAMP((d - 4 * CELL) * 256 / (16 * CELL), 0, 236);
}
#define FAR 27
static void far_draw(int ccx, int ccy) {   // past the full detail: the land in blocks of 3 x 3 cells, its woods, rocks and roofs as low hazy shapes, then the sea
    int d = S.dim;
    gfx_floor_pass = 1;
    for (int by = -FAR; by < FAR; by += 3) for (int bx = -FAR; bx < FAR; bx += 3) {
        if (bx >= -VIEW - 2 && bx + 3 <= VIEW + 2 && by >= -VIEW - 2 && by + 3 <= VIEW + 2) continue;   // (drawn in full)
        int x = ccx + bx, y = ccy + by, wx = x * CELL, wz = -(y * CELL);
        if (bx * bx + by * by > FAR * FAR) continue;
        u32 col; int v = KT_WATER, c = 0;
        if (x >= 0 && y >= 0 && x + 1 < d && y + 1 < d) { c = cells[(y + 1) * d + x + 1]; v = c & 7; }
        col = v == KT_WATER ? UI_RGB(40, 100, 196) : v == KT_FOREST ? UI_RGB(52, 120, 54) : v == KT_ROCK || v == KT_CRYSTAL ? UI_RGB(120, 126, 116) : v == KT_MEADOW ? UI_RGB(120, 184, 70) : v == KT_ROAD ? UI_RGB(190, 160, 116) : UI_RGB(86, 158, 64);
        if (c & KC_BLD) col = UI_RGB(160, 150, 120);
        quad(wx, wz, wx + 3 * CELL, wz - 3 * CELL, v == KT_WATER ? 14 : 0, col, shade(col, 236));
    }
    gfx_floor_pass = 0;
    for (int by = -FAR; by < FAR; by += 3) for (int bx = -FAR; bx < FAR; bx += 3) {   // the silhouettes
        if (bx >= -VIEW - 2 && bx + 3 <= VIEW + 2 && by >= -VIEW - 2 && by + 3 <= VIEW + 2) continue;
        if (bx * bx + by * by > FAR * FAR) continue;
        int x = ccx + bx, y = ccy + by; if (x < 0 || y < 0 || x + 1 >= d || y + 1 >= d) continue;
        int c = cells[(y + 1) * d + x + 1], v = c & 7, wx = x * CELL + CELL * 3 / 2, wz = -(y * CELL + CELL * 3 / 2);
        hzf = haze_of(wx, wz);
        if (c & KC_BLD) { int t = -1; for (int i = 0; i < R.nB; i++) if (x + 1 >= Bl[i].x && x + 1 < Bl[i].x + 2 && y + 1 >= Bl[i].y && y + 1 < Bl[i].y + 2) { t = Bl[i].type; break; }
            int h = t == KB_TOWER ? 380 : t == KB_CASTLE ? 260 : 100; box(wx - 80, -h, wz - 80, wx + 80, 0, wz + 80, UI_RGB(206, 196, 180)); roof(wx - 88, wz - 88, wx + 88, wz + 88, -h, 60, UI_RGB(70, 90, 160)); }
        else if (v == KT_FOREST) roof(wx - 130, wz - 130, wx + 130, wz + 130, 0, 150, UI_RGB(40, 110, 50));
        else if (v == KT_ROCK || v == KT_CRYSTAL) box(wx - 90, -60, wz - 90, wx + 90, 0, wz + 90, UI_RGB(130, 130, 134));
    }
    hzf = 0;
}
static void land_draw(void) {
    int d = S.dim, ccx = gx / CELL, ccy = -gz / CELL;
    nub_drawn = 0;
    gfx_set_identity(); gfx_set_reserve(RESERVE_PEOPLE);   // (the land may not take the giant's and the HUD's share)
    int x0 = MAX(0, ccx - VIEW), x1 = MIN(d - 1, ccx + VIEW), y0 = MAX(0, ccy - VIEW), y1 = MIN(d - 1, ccy + VIEW);
    gfx_floor_pass = 1;
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        int c = cells[y * d + x], v = c & 7; u32 h = hsh(x, y);
        int wx = x * CELL, wz = -(y * CELL), k = 236 + (int)(h & 31);
        u32 col;
        switch (v) {
            case KT_WATER: { int w = isin((fr * 50 + x * 700 + y * 1100) & 4095) >> 8; col = UI_RGB(30 + w, 90 + w, 190 + w); break; }
            case KT_MEADOW: col = UI_RGB(126, 190, 72); break;
            case KT_ROAD: col = UI_RGB(200, 166, 116); break;
            case KT_BRIDGE: col = (x + y) & 1 ? UI_RGB(150, 100, 56) : UI_RGB(130, 86, 48); break;
            case KT_ROCK: case KT_CRYSTAL: col = UI_RGB(118, 132, 96); break;
            case KT_FOREST: col = UI_RGB(70, 140, 56); break;
            default: col = UI_RGB(88, 162, 66);
        }
        if (c & KC_BLD) col = UI_RGB(150, 156, 112);
        if (v != KT_WATER) {   // a shore: the grass beside water goes sandy
            int sea = (x > 0 && (cells[y * d + x - 1] & 7) == KT_WATER) || (x < d - 1 && (cells[y * d + x + 1] & 7) == KT_WATER) || (y > 0 && (cells[(y - 1) * d + x] & 7) == KT_WATER) || (y < d - 1 && (cells[(y + 1) * d + x] & 7) == KT_WATER);
            if (sea && v != KT_ROAD && v != KT_BRIDGE && !(c & KC_BLD)) col = UI_RGB(226, 206, 150); }
        quad(wx, wz, wx + CELL, wz - CELL, v == KT_WATER ? 14 : 0, shade(col, k), shade(col, k - 18));
    }
    gfx_floor_pass = 0;
    for (int i = 0; i < R.nB; i++) { const KBld *b = &Bl[i]; if (b->x + 2 < x0 || b->x > x1 || b->y + 2 < y0 || b->y > y1) continue; hzf = haze_of((b->x + 1) * CELL, -(b->y + 1) * CELL); building(b, i); }
    hzf = 0;
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        int c = cells[y * d + x], v = c & 7; if (c & KC_BLD) continue;
        int wx = x * CELL + CELL / 2, wz = -(y * CELL + CELL / 2); u32 h = hsh(x, y);
        int ox = (int)(h % 31) - 15, oz = (int)((h >> 8) % 31) - 15, nr = ABS(x - ccx) <= 4 && ABS(y - ccy) <= 4;
        hzf = haze_of(wx, wz);
        if (v == KT_FOREST) { tree(wx + ox, wz + oz, h, nr); if (nr && (h & 0x10000)) tree(wx - ox - 10, wz - oz + 14, h >> 3, 1); }
        else if (v == KT_ROCK || v == KT_CRYSTAL) rock(wx, wz, h, nr, v == KT_CRYSTAL);
        else if (v == KT_MEADOW && nr && (h & 3) == 0) { box(wx + ox - 3, -8, wz + oz - 3, wx + ox + 3, 0, wz + oz + 3, (h & 4) ? UI_RGB(250, 230, 60) : UI_RGB(250, 120, 160)); box(wx - ox - 3, -6, wz - oz - 3, wx - ox + 3, 0, wz - oz + 3, UI_RGB(250, 250, 250)); }
        else if (v == KT_GRASS && nr && (h & 7) == 0) box(wx + ox - 8, -10, wz + oz - 8, wx + ox + 8, 0, wz + oz + 8, UI_RGB(60, 140, 50));   // a bush
        else if (v == KT_BRIDGE) { box(wx - CELL / 2, -14, wz - CELL / 2, wx - CELL / 2 + 6, 0, wz + CELL / 2, UI_RGB(110, 70, 36)); box(wx + CELL / 2 - 6, -14, wz - CELL / 2, wx + CELL / 2, 0, wz + CELL / 2, UI_RGB(110, 70, 36)); }
        else if (v == KT_ROAD && S.pop > 40) {   // the streets: the bigger the realm, the busier (up to 4 a cell; ten thousand Nubbins make a crowd)
            int n = S.pop >= 4000 ? 4 : S.pop >= 1000 ? 3 : S.pop >= 200 ? 2 : (h & 1);
            for (int k = 0; k < n; k++) { int a = (fr * 3 + (int)(h >> (k * 4)) * 64) & 4095, px = wx + (isin(a) * 36 >> 12) + (k - 2) * 8, pz = wz + (icos(a + k * 900) * 36 >> 12); nub_sprite(px, pz, (int)(h >> (k + 3)) & 3, (fr / 7 + k) & 1); } }
    }
    for (int w = 0; w < KS_MAXW; w++) { const KsWk *k = &S.wk[w]; if (!k->st || k->cell == 0xFFFF) continue;   // the carriers on the roads, each with its good on its head
        int x = k->cell % d, y = k->cell / d; if (x < x0 || x > x1 || y < y0 || y > y1) continue;
        int wx = x * CELL + CELL / 2 + (int)(w % 5) * 6 - 12, wz = -(y * CELL + CELL / 2) + (int)(w % 3) * 8 - 8;
        nub_sprite(wx, wz, w & 3, (fr / 6 + w) & 1);
        box(wx - 6, -50, wz - 6, wx + 6, -38, wz + 6, goodCol[k->good & 7]); }
    hzf = 0;
    far_draw(ccx, ccy);
    gfx_set_reserve(0);
}
static void cursor_draw(void) {
    int cx, cy; if (!cursor_cell(&cx, &cy)) return;
    int bx = cx, by = cy, w = 1;
    if (hand.mode >= KHM_HUT && hand.mode <= KHM_TOWER) { bx = cx - 1 + ((isin(gyaw) > 0) ? 1 : 0); by = cy - 1 + ((icos(gyaw) < 0) ? 1 : 0); w = 2; }
    int ok = w == 2 ? ksSpotOk(&S, hand.mode - KHM_HUT, bx, by) : 1, p = 140 + (isin((fr * 120) & 4095) >> 6);
    gfx_set_identity();
    u32 c = ok ? UI_RGB(p / 2, p, p / 2) : UI_RGB(p, p / 4, p / 4);
    int x0 = bx * CELL, z0 = -(by * CELL), x1 = x0 + w * CELL, z1 = z0 - w * CELL;
    box(x0, -4, z1, x1, -1, z1 + 6, c); box(x0, -4, z0 - 6, x1, -1, z0, c); box(x0, -4, z1, x0 + 6, -1, z0, c); box(x1 - 6, -4, z1, x1, -1, z0, c);
}
static void panel_goods(int y) {
    ui_round(8, y, 304, 30, 8, UI_CARD, UI_CARD2, 0);
    for (int g = 0; g < 8; g++) { int x = 12 + (g & 3) * 73, yy = y + 3 + (g >> 2) * 13; char b[24]; snprintf(b, sizeof b, "%s %d", khGood[g], R.goods[g]);
        ui_disc(x + 4, yy + 5, 7, goodCol[g]); ui_textfit(x + 11, yy, 60, b, UI_INK); }
}
void realm_draw(void) {
    gfx_set_clear(150, 196, 236); gfx_sky(0xecc496u, 0xf0e0c8u);
    gfx_set_ambient(100, 96);
    if (!live) {   // FOUND A REALM
        gfx_set_fog(0, 0);
        ui_backdrop();
        int top = ui_window(40, 20, 240, 200, "FOUND A REALM", IC_STAR);
        static const char *const nm[4] = { "HAMLET  (48 x 48)", "REALM  (96 x 96)", "KINGDOM  (128 x 128)", "EMPIRE  (255 x 255)" };
        static const char *const tx[4] = { "A LITTLE ISLAND. QUICK TO FILL", "ROOM FOR A TOWN AND A CASTLE", "A BIG LAND. A LONG REIGN", "ROOM FOR TEN THOUSAND NUBBINS" };
        ui_wrapc(50, top - 2, 220, 3, "YOU WILL WAKE UP A GIANT ON AN ISLAND. THE LITTLE FOLK, THE NUBBINS, NEED YOU. HOW BIG A LAND?", UI_SOFT);
        for (int i = 0; i < 4; i++) { int y = top + 40 + i * 34, s = i == found_sel;
            if (s) ui_round(48, y - 2, 224, 32, 6, UI_RGB(226, 242, 254), UI_HILITE, 0);
            ui_text(58, y + 2, 1, nm[i], s ? UI_BLUE2 : UI_INK); ui_text(58, y + 15, 1, tx[i], UI_SOFT); }
        ui_hintbar(214, "DPAD PICK|X FOUND IT|O NOT NOW");
        return;
    }
    // the camera: behind and above the giant, turned with L1 R1
    int fx = gx, fz = gz, dist = cdist, pitch = 420 + (cdist - 650) * 300 / 1750, yaw = (2048 - cyaw) & 4095;   // (closer is lower, further looks down)
    cam.at.vx = fx; cam.at.vy = -170; cam.at.vz = fz;
    cam.eye.vx = fx - ((isin(yaw) * ((dist * icos(pitch)) >> 12)) >> 12);
    cam.eye.vz = fz - ((icos(yaw) * ((dist * icos(pitch)) >> 12)) >> 12);
    cam.eye.vy = -170 - ((dist * isin(pitch)) >> 12);
    gfx_camera_update();
    gfx_set_fog(0, 0);   // (the realm's own haze instead: see shade)
    land_draw();
    cursor_draw();
    {   // the giant: your own Bore
        Gait g; memset(&g, 0, sizeof g); g.clock = fr; g.mood = 60; g.phase = gphase; g.amp = gmove ? 4096 : 0; g.run = gmove && (gspd > 20);
        if (S.hd.k != KHD_NONE) { static VPose carry; carry.arm[0] = carry.arm[1] = -1300; carry.elbow[0] = carry.elbow[1] = -500; carry.aroll[0] = -100; carry.aroll[1] = 100;
            carry.leg[0] = (s16)(gmove ? (isin(gphase) >> 4) : 0); carry.leg[1] = (s16)-carry.leg[0]; carry.knee[0] = carry.knee[1] = 120; g.pose = &carry; }
        VECTOR p = { gx, 0, gz }; voxel_draw(&sims[player].bore, &p, gyaw, &g); gfx_shadow(&p, 90);
        if (S.hd.k == KHD_GOODS) { gfx_set_identity(); int hx = gx + (isin(gyaw) * 70 >> 12), hz = gz - (icos(gyaw) * 70 >> 12); for (int j = 0; j < S.hd.n && j < 6; j++) box(hx - 14, -330 - j * 14, hz - 14, hx + 14, -316 - j * 14, hz + 14, goodCol[S.hd.g & 7]); }
        if (S.hd.k == KHD_BLD) { gfx_set_identity(); int hx = gx + (isin(gyaw) * 70 >> 12), hz = gz - (icos(gyaw) * 70 >> 12); box(hx - 50, -400, hz - 50, hx + 50, -320, hz + 50, UI_RGB(196, 160, 110)); }
    }
    gfx_set_fog(0, 0);
    // the HUD
    { char b[64]; ui_round(8, 8, 304, 18, 7, UI_RGB(30, 40, 70), UI_RGB(18, 24, 48), 0);
      snprintf(b, sizeof b, "DAY %ld   NUBBINS %d   WORKING %d   TIME X%d", khDay(R.day), S.pop, S.emp, spd == 0 ? 1 : spd == 1 ? 4 : 16); ui_text(16, 11, 1, b, UI_WHITE);
      char c[64]; khCost(c, &S, hand.mode); ui_round(8, 28, 304, 16, 6, UI_CARD, UI_CARD2, 0); ui_textfit(14, 30, 292, c, UI_BLUE2); }
    { int cx, cy; char b[64]; if (cursor_cell(&cx, &cy)) { khInfo(&S, cx, cy, b); ui_round(8, 196, 304, 16, 6, UI_CARD, UI_CARD2, 0); ui_textfit(14, 198, 292, b, UI_INK); } }
    { char h[40]; khHeld(&S, h); if (h[0]) { char b[56]; snprintf(b, sizeof b, "CARRYING: %s", h); ui_badge(8, 48, b, UI_AMBER, UI_INK); } }
    if (panel == 1) panel_goods(156);
    if (panel == 2 && mis < NMIS) {   // the mission
        const Mis *m = &miss[mis]; int top = ui_window(30, 46, 260, 140, m->nm, IC_STAR);
        ui_wrap(40, top, 240, 3, m->story, UI_SOFT);
        for (int k = 0; k < 3; k++) { int have = count_task(&m->t[k]), y = top + 46 + k * 20; char b[40]; snprintf(b, sizeof b, "%d/%d", MIN(have, m->t[k].n), m->t[k].n);
            ui_text(40, y, 1, m->t[k].nm, have >= m->t[k].n ? UI_GOODT : UI_INK); ui_text(276 - ui_tw(1, b), y, 1, b, have >= m->t[k].n ? UI_GOODT : UI_SOFT); }
        { char b[32]; snprintf(b, sizeof b, "PAYS $%d", m->pay * 100); ui_text(40, top + 108, 1, b, UI_GOLDT); }
    }
    if (panel == 2 && mis >= NMIS) { int top = ui_window(30, 60, 260, 60, "ALL MISSIONS DONE", IC_STAR); ui_wrap(40, top, 240, 2, "THE REALM IS YOURS TO KEEP. IT NEVER ENDS.", UI_SOFT); }
    if (msgT && msg) { int w = MIN(300, ui_tw(1, msg) + 20), two = ui_tw(1, msg) > 280; ui_round(160 - w / 2, two ? 164 : 174, w, two ? 30 : 18, 8, UI_RGB(30, 40, 70), UI_RGB(18, 24, 48), 0); ui_wrapc(160 - w / 2 + 8, two ? 167 : 177, w - 16, 2, msg, UI_WHITE); }
    if (!pick && !menu) ui_hintbar(216, "X DO|O RUN|SQ BUILD|L1 R1 TURN|L2 R2 ZOOM");
    if (pick) {
        int top = ui_window(16, 20, 288, 196, "WHAT X DOES", IC_HAMMER);
        for (int i = 0; i < NPICK; i++) { int col = i / 7, row = i % 7, x = 24 + col * 94, y = top + row * 20, s = i == psel;
            if (s) ui_round(x - 2, y - 2, 92, 18, 5, UI_RGB(226, 242, 254), UI_HILITE, 0);
            ui_textfit(x + 2, y, 86, khName[pickModes[i]], s ? UI_BLUE2 : UI_INK); }
        { char c[64]; khCost(c, &S, pickModes[psel]); ui_round(20, top + 146, 280, 18, 6, UI_PANEL, UI_PANEL, 0); ui_textfit(26, top + 148, 268, c, UI_INK); }
        ui_hintbar(220, "DPAD PICK|X CHOOSE|O BACK");
    }
    if (menu) {
        ui_dim(); int top = ui_window(80, 60, 160, 110, "THE REALM", IC_STAR);
        const char *it[3] = { "BACK TO IT", spd == 0 ? "TIME: NORMAL" : spd == 1 ? "TIME: FAST" : "TIME: FASTEST", "LEAVE THE REALM" };
        for (int i = 0; i < 3; i++) ui_button(88, top + i * 22, 144, 18, it[i], i == 2 ? IC_DOOR : IC_PLAY, i == msel);
    }
}

// the overlay's head: what BORE2.EXE calls (ovl.h)
#include "ovl.h"
static void open_(int a) { (void)a; realm_open(); }
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, open_, realm_update, realm_draw };
