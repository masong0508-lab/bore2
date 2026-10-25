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
#include "realm/kingdom.h"
#include "realm/kingsim.h"
#include "realm/kinglog.h"
#include "realm/kinghand.h"

#define CELL 96
#define VIEW 11
static KRealm R; static KBld Bl[KS_MAXB]; static u8 cells[128 * 128]; static KSim S; static KHand hand;
static int blob_len;   // the realm as saved bytes, kept with the life while you are not in it: they live in the simulation's
// distance-map cache (S.dist and S.queue), which is only a cache while the realm runs and is free when it does not (the GBA did the same)
#define blob ((u8 *)S.dist)
#define BLOB_CAP ((int)(sizeof S.dist + sizeof S.queue))
static int live, mis, mis_paid_all;                 // the realm is running; the mission you are on
static int gx, gz, gyaw, gmove, gphase, cyaw, cyaw_t, spd, fr, msgT, panel, menu, msel, pick, psel, found_sel;
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
void realm_reset(void) { blob_len = 0; live = 0; mis = 0; }
void realm_save(void) { pack(); sv_u8((u8)mis); sv_u16((u16)blob_len); sv_n(blob, blob_len); }
void realm_load(int ver) {
    live = 0; mis = 0; blob_len = 0;
    if (ver < 15) return;
    mis = ld_u8(); int n = ld_u16(); if (n > BLOB_CAP) n = 0; ld_n(blob, n); blob_len = n;
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
}
void realm_open(void) {
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
static const u8 pickModes[] = { KHM_HAND, KHM_ROAD, KHM_HUT, KHM_HOUSE, KHM_WOODCUT, KHM_QUARRY, KHM_SAWMILL, KHM_MASON, KHM_STORE, KHM_SHEEP, KHM_CRYMINE, KHM_WEAVER, KHM_CUTTER, KHM_WELL, KHM_TAVERN, KHM_CASTLE, KHM_MOVE, KHM_NUBBIN, KHM_DEMO };
#define NPICK ((int)sizeof pickModes)
int realm_update(int held, int pressed, int steps) {
    fr += steps; if (msgT) msgT = MAX(0, msgT - steps);
    if (!live) {   // FOUND A REALM: pick its size
        if (pressed & PAD_UP) found_sel = (found_sel + 2) % 3;
        if (pressed & PAD_DOWN) found_sel = (found_sel + 1) % 3;
        if (pressed & PAD_CROSS) { found(found_sel); audio_voice(VOICE_YAHOO); }
        if (pressed & PAD_CIRCLE) return 0;
        return 1;
    }
    if (menu) {   // START: the realm's menu
        if (pressed & PAD_UP) msel = (msel + 2) % 3;
        if (pressed & PAD_DOWN) msel = (msel + 1) % 3;
        if (pressed & PAD_CIRCLE) menu = 0;
        if (pressed & PAD_CROSS) {
            if (msel == 0) menu = 0;
            else if (msel == 1) { spd = (spd + 1) % 3; }
            else { pack(); live = 0; sim_note("THE REALM WAITS FOR YOU (SAVE YOUR LIFE TO KEEP IT)"); return 0; }
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
    if (pressed & PAD_L1) cyaw_t -= 512;
    if (pressed & PAD_R1) cyaw_t += 512;
    { int dd = cyaw_t - cyaw; cyaw += CLAMP(dd, -64 * steps, 64 * steps); }
    // the giant walks (the pad as the camera sees it)
    int ux = (held & PAD_RIGHT ? 1 : 0) - (held & PAD_LEFT ? 1 : 0), uy = (held & PAD_UP ? 1 : 0) - (held & PAD_DOWN ? 1 : 0);
    gmove = ux || uy;
    if (gmove) {
        static const s16 dirY[3][3] = { { 3584, 3072, 2560 }, { 0, 0, 2048 }, { 512, 1024, 1536 } };   // [ux + 1][uy + 1]: up 0, right 1024, down 2048, left 3072
        int want = (dirY[ux + 1][1 - uy] + cyaw) & 4095, dy = ((want - gyaw + 2048) & 4095) - 2048;
        gyaw = (gyaw + CLAMP(dy, -300 * steps, 300 * steps)) & 4095;
        int sp = (held & (PAD_R2 | PAD_L2)) ? 30 : 18;
        for (int k = 0; k < steps; k++) {
            int mx = isin(want) * sp >> 12, mz = -(icos(want) * sp >> 12);
            if (walk_ok(gx + mx, gz + mz)) { gx += mx; gz += mz; } else if (walk_ok(gx + mx, gz)) gx += mx; else if (walk_ok(gx, gz + mz)) gz += mz;
            gphase = (gphase + 150) & 4095; }
    }
    if (pressed & PAD_CROSS) {
        int cx, cy; if (cursor_cell(&cx, &cy)) {
            int bx = cx, by = cy;
            if (hand.mode >= KHM_HUT && hand.mode <= KHM_TAVERN) { bx = cx - 1 + ((isin(gyaw) > 0) ? 1 : 0); by = cy - 1 + ((icos(gyaw) < 0) ? 1 : 0); }   // (a 2 x 2 site: the corner nearest the giant)
            int r = khAct(&hand, &S, bx, by);
            say(khMsg[r < KHR_N ? r : 0]);
            audio_voice(r == KHR_SITE || r == KHR_GIVE || r == KHR_GOT || r == KHR_ROAD ? VOICE_AGREE : r == KHR_SPOT || r == KHR_REFUSE || r == KHR_NOGATHER || r == KHR_NOTHING ? VOICE_NAH : VOICE_FINISHED);
        }
    }
    static const u8 rate[3] = { 1, 4, 16 };
    for (int k = 0; k < steps * rate[spd]; k++) ksStep(&S);
    if (S.ev) { static const char *const cst[5] = { "", "THE CASTLE'S FOUNDATION IS LAID!", "THE CASTLE'S WALLS ARE UP!", "THE TOWERS RISE!", "LONG LIVE THE MONARCH: THE REALM IS CROWNED!" }; say(cst[MIN(S.ev, 4)]); S.ev = 0; audio_voice(VOICE_YAHOO); }
    if (!(fr & 31)) mission_check();
    return 1;
}

// ---- the picture ----
static u32 hsh(int x, int y) { u32 h = (u32)x * 73856093u ^ (u32)y * 19349663u; h ^= h >> 13; h *= 0x5bd1e995u; return h ^ (h >> 15); }
static void quad(int x0, int z0, int x1, int z1, int y, u32 c0, u32 c1) {   // a flat quad on the ground (x, z in world units)
    SVECTOR a = { (short)x0, (short)y, (short)z0 }, b = { (short)x1, (short)y, (short)z0 }, c = { (short)x0, (short)y, (short)z1 }, d = { (short)x1, (short)y, (short)z1 };
    gfx_quad_g(&a, &b, &c, &d, c0, c1, c1, c0, 0);
}
static u32 shade(u32 c, int k) { int r = (c & 255) * k >> 8, g = ((c >> 8) & 255) * k >> 8, b = ((c >> 16) & 255) * k >> 8; return UI_RGB(MIN(r, 255), MIN(g, 255), MIN(b, 255)); }
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
static void tree(int wx, int wz, u32 h) {
    int s = 30 + (int)(h & 15), ht = 120 + (int)((h >> 4) & 63);
    box(wx - 7, -40, wz - 7, wx + 7, 0, wz + 7, UI_RGB(110, 70, 36));
    u32 g = (h & 0x100) ? UI_RGB(40, 120, 50) : UI_RGB(30, 104, 60);
    roof(wx - s, wz - s, wx + s, wz + s, -36, ht / 2, g);
    roof(wx - s * 2 / 3, wz - s * 2 / 3, wx + s * 2 / 3, wz + s * 2 / 3, -36 - ht / 3, ht / 2, shade(g, 290));
}
static void nubbin(int wx, int wz, u32 col, int bob) {   // a Nubbin: a body and a head
    int y = -((bob & 4095) > 2048 ? 4 : 0);
    box(wx - 6, y - 22, wz - 5, wx + 6, y, wz + 5, col);
    box(wx - 5, y - 33, wz - 5, wx + 5, y - 23, wz + 5, UI_RGB(240, 196, 150));
}
static const u32 goodCol[8] = { UI_RGB(150, 100, 50), UI_RGB(150, 150, 156), UI_RGB(245, 245, 240), UI_RGB(120, 230, 250), UI_RGB(214, 170, 100), UI_RGB(196, 196, 206), UI_RGB(200, 60, 80), UI_RGB(170, 250, 255) };
static void building(const KBld *b, int i) {
    int x0 = b->x * CELL + 8, z0 = -(b->y * CELL + 8), x1 = (b->x + 2) * CELL - 8, z1 = -((b->y + 2) * CELL - 8), cx = (x0 + x1) / 2, cz = (z0 + z1) / 2;
    const KsType *T = ksT(b->type);
    if (b->type == KB_CASTLE) {   // the castle by its stage: a slab, the walls, the towers, the crown
        int st = R.castle; u32 sc = UI_RGB(186, 184, 176);
        box(x0, -12, z1, x1, 0, z0, sc);
        if (st >= 2) { box(x0, -110, z1, x0 + 20, -12, z0, sc); box(x1 - 20, -110, z1, x1, -12, z0, sc); box(x0, -110, z0 - 20, x1, -12, z0, sc); box(x0, -110, z1, x1, -12, z1 + 20, sc); }
        if (st >= 3) { for (int k = 0; k < 4; k++) { int tx = k & 1 ? x1 - 22 : x0 + 22, tz = k & 2 ? z1 + 22 : z0 - 22; box(tx - 22, -190, tz - 22, tx + 22, -12, tz + 22, shade(sc, 240)); roof(tx - 26, tz - 26, tx + 26, tz + 26, -190, 60, UI_RGB(60, 70, 160)); } }
        if (st >= 4) { box(cx - 34, -230, cz - 34, cx + 34, -12, cz + 34, UI_RGB(220, 214, 200)); roof(cx - 40, cz - 40, cx + 40, cz + 40, -230, 90, UI_RGB(200, 40, 50)); box(cx - 3, -380, cz - 3, cx + 3, -318, cz + 3, UI_RGB(230, 190, 40)); }
        if (st < 4) { int fl = (fr * 40) & 4095; box(cx - 2, -60, cz - 2, cx + 2, 0, cz + 2, UI_RGB(110, 70, 36)); box(cx + 2, -60 + (isin(fl) >> 10), cz - 1, cx + 30, -44 + (isin(fl) >> 10), cz + 1, UI_RGB(220, 40, 50)); }
        int g = 0, n = 0; ksCost(&S, b, &g, &n); if (st < 4) { int k = ksIn(b); for (int j = 0; j < k && j < 8; j++) box(x1 - 30 - (j & 3) * 14, -12 - (j >> 2) * 12 - 12, z1 + 8, x1 - 18 - (j & 3) * 14, -12 - (j >> 2) * 12, z1 + 20, goodCol[g & 7]); }
        return;
    }
    if (!b->lvl) {   // a site: pegs at the corners, a string between them, the delivered goods in a pile
        u32 pc = UI_RGB(230, 210, 150);
        for (int k = 0; k < 4; k++) { int px = k & 1 ? x1 : x0, pz = k & 2 ? z1 : z0; box(px - 4, -40, pz - 4, px + 4, 0, pz + 4, pc); }
        quad(x0, z1, x1, z0, -1, UI_RGB(150, 120, 80), UI_RGB(140, 110, 72));
        int g = 0, n = 0; ksCost(&S, b, &g, &n); int k = ksIn(b);
        for (int j = 0; j < k; j++) box(cx - 30 + (j % 4) * 16, -14 - (j / 4) * 14, cz - 6, cx - 18 + (j % 4) * 16, -(j / 4) * 14, cz + 6, goodCol[g & 7]);
        return;
    }
    static const u32 wallC[KS_NTYPES] = { UI_RGB(190, 150, 100), UI_RGB(230, 220, 196), UI_RGB(150, 104, 60), UI_RGB(150, 150, 150), UI_RGB(176, 126, 76), UI_RGB(170, 168, 160), UI_RGB(196, 160, 110),
                                          UI_RGB(220, 210, 170), UI_RGB(110, 120, 140), UI_RGB(210, 180, 200), UI_RGB(150, 200, 220), UI_RGB(186, 184, 176), UI_RGB(150, 150, 160), UI_RGB(170, 110, 70) };
    static const u32 roofC[KS_NTYPES] = { UI_RGB(200, 170, 70), UI_RGB(170, 60, 50), UI_RGB(90, 120, 60), UI_RGB(90, 90, 100), UI_RGB(120, 80, 50), UI_RGB(100, 100, 120), UI_RGB(60, 90, 160),
                                          UI_RGB(200, 170, 70), UI_RGB(60, 60, 90), UI_RGB(160, 70, 140), UI_RGB(70, 150, 190), UI_RGB(60, 70, 160), UI_RGB(80, 120, 200), UI_RGB(150, 50, 40) };
    int wh = b->type == KB_HOUSE ? 110 : b->type == KB_STORE ? 100 : b->type == KB_WELL ? 26 : 70;
    if (b->type == KB_SHEEP) {   // a fenced meadow with sheep in it
        u32 fc = UI_RGB(160, 120, 70);
        box(x0, -26, z0 - 4, x1, -20, z0, fc); box(x0, -26, z1, x1, -20, z1 + 4, fc); box(x0, -26, z1, x0 + 4, -20, z0, fc); box(x1 - 4, -26, z1, x1, -20, z0, fc);
        for (int k = 0; k < 3; k++) { int sx = x0 + 40 + k * 44 + (isin((fr * 10 + k * 1400) & 4095) >> 9), sz = cz + (k - 1) * 30; box(sx - 14, -22, sz - 9, sx + 14, -4, sz + 9, UI_RGB(246, 246, 240)); box(sx + 12, -24, sz - 5, sx + 20, -14, sz + 5, UI_RGB(40, 40, 40)); }
    } else if (b->type == KB_WELL) {
        box(cx - 30, -26, cz - 30, cx + 30, 0, cz + 30, wallC[b->type]); quad(cx - 22, cz + 22, cx + 22, cz - 22, -27, UI_RGB(40, 90, 200), UI_RGB(60, 120, 220));
        box(cx - 30, -90, cz - 3, cx - 24, -26, cz + 3, UI_RGB(110, 70, 36)); box(cx + 24, -90, cz - 3, cx + 30, -26, cz + 3, UI_RGB(110, 70, 36)); roof(cx - 38, cz - 30, cx + 38, cz + 30, -90, 30, roofC[b->type]);
    } else {
        box(x0 + 10, -wh, z1 + 10, x1 - 10, 0, z0 - 10, wallC[b->type]);
        roof(x0, z1, x1, z0, -wh, b->type == KB_HUT ? 70 : 54, roofC[b->type]);
        box(cx - 12, -40, z0 - 12, cx + 12, -2, z0 - 9, UI_RGB(70, 46, 26));   // the door (south)
        if (b->type == KB_WOODCUT || b->type == KB_SAWMILL) for (int k = 0; k < 3; k++) box(x1 - 6, -12 - k * 12, z1 + 10 + k * 8, x1 + 26, -k * 12, z1 + 20 + k * 8, UI_RGB(140, 90, 50));
        if (b->type == KB_QUARRY || b->type == KB_MASON) box(x0 - 20, -24, z0 - 40, x0 + 10, 0, z0 - 10, UI_RGB(140, 140, 146));
        if (b->type == KB_CRYMINE || b->type == KB_CUTTER) { SVECTOR a = { (short)(x0 - 10), 0, (short)(z0 - 20) }, c2 = { (short)(x0 + 10), 0, (short)(z0 - 20) }, t = { (short)x0, -60, (short)(z0 - 24) }; gfx_tri_f(&a, &c2, &t, 120, 230, 250, 0); }
        if (b->type == KB_TAVERN) box(cx + 20, -70, z0 - 14, cx + 40, -50, z0 - 10, UI_RGB(230, 190, 40));   // its sign
    }
    // the folk: residents round a home, a worker at a staffed workplace
    if (T->kind == KK_HOME) for (int k = 0; k < b->work && k < 3; k++) { int a = (fr * 6 + k * 1365 + i * 700) & 4095; nubbin(cx + (isin(a) * 120 >> 12), cz + (icos(a) * 120 >> 12), UI_RGB(70 + k * 50, 120, 200 - k * 40), fr * 300 + k * 900); }
    if ((T->kind == KK_GATHER || T->kind == KK_PROC) && b->work) nubbin(cx + 40, z0 - 30, UI_RGB(200, 120, 60), fr * 200);
    if ((T->kind == KK_GATHER || T->kind == KK_PROC) && ksOut(b)) for (int j = 0; j < ksOut(b) && j < 6; j++) box(x1 + 4, -12 - j * 10, cz - 6, x1 + 18, -2 - j * 10, cz + 6, goodCol[T->outG & 7]);
}
static void land_draw(void) {
    int d = S.dim, ccx = gx / CELL, ccy = -gz / CELL;
    gfx_set_identity();
    int x0 = MAX(0, ccx - VIEW), x1 = MIN(d - 1, ccx + VIEW), y0 = MAX(0, ccy - VIEW), y1 = MIN(d - 1, ccy + VIEW);
    // the sea beyond the land's edge, as big quads
    gfx_floor_pass = 1;
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        int c = cells[y * d + x], v = c & 7; u32 h = hsh(x, y);
        int wx = x * CELL, wz = -(y * CELL), k = 236 + (int)(h & 31);
        u32 col;
        switch (v) {
            case KT_WATER: { int w = isin((fr * 50 + x * 700 + y * 1100) & 4095) >> 8; col = UI_RGB(30 + w, 90 + w, 190 + w); break; }
            case KT_MEADOW: col = UI_RGB(120, 186, 70); break;
            case KT_ROAD: col = UI_RGB(196, 160, 110); break;
            case KT_BRIDGE: col = (x + y) & 1 ? UI_RGB(150, 100, 56) : UI_RGB(130, 86, 48); break;
            case KT_ROCK: case KT_CRYSTAL: col = UI_RGB(120, 140, 90); break;
            default: col = UI_RGB(86, 160, 64);
        }
        if (c & KC_BLD) col = UI_RGB(150, 160, 110);
        quad(wx, wz, wx + CELL, wz - CELL, v == KT_WATER ? 10 : 0, shade(col, k), shade(col, k - 18));
    }
    gfx_floor_pass = 0;
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        int c = cells[y * d + x], v = c & 7; if (c & KC_BLD) continue;
        int wx = x * CELL + CELL / 2, wz = -(y * CELL + CELL / 2); u32 h = hsh(x, y);
        int ox = (int)(h % 31) - 15, oz = (int)((h >> 8) % 31) - 15;
        if (v == KT_FOREST) { tree(wx + ox, wz + oz, h); if (h & 0x10000) tree(wx - ox - 10, wz - oz + 14, h >> 3); }
        else if (v == KT_ROCK || v == KT_CRYSTAL) { int s = 26 + (int)(h & 15); box(wx - s + ox / 2, -(30 + (int)((h >> 4) & 31)), wz - s, wx + s + ox / 2, 0, wz + s, UI_RGB(136, 136, 140));
            if (v == KT_CRYSTAL) { SVECTOR a = { (short)(wx - 12), -20, (short)wz }, b = { (short)(wx + 12), -20, (short)wz }, t = { (short)wx, -110, (short)(wz + 6) }; gfx_tri_f(&a, &b, &t, 110, 230, 255, 0); gfx_tri_f(&b, &a, &t, 80, 190, 240, 0); } }
        else if (v == KT_MEADOW && (h & 3) == 0) { box(wx + ox - 3, -8, wz + oz - 3, wx + ox + 3, 0, wz + oz + 3, (h & 4) ? UI_RGB(250, 230, 60) : UI_RGB(250, 120, 160)); }
        else if (v == KT_BRIDGE) { box(wx - CELL / 2, -10, wz - CELL / 2, wx - CELL / 2 + 6, 0, wz + CELL / 2, UI_RGB(110, 70, 36)); box(wx + CELL / 2 - 6, -10, wz - CELL / 2, wx + CELL / 2, 0, wz + CELL / 2, UI_RGB(110, 70, 36)); }
    }
    for (int i = 0; i < R.nB; i++) { const KBld *b = &Bl[i]; if (b->x + 2 < x0 || b->x > x1 || b->y + 2 < y0 || b->y > y1) continue; building(b, i); }
    for (int w = 0; w < KS_MAXW; w++) { const KsWk *k = &S.wk[w]; if (!k->st || k->cell == 0xFFFF) continue;   // the carriers on the roads
        int x = k->cell % d, y = k->cell / d; if (x < x0 || x > x1 || y < y0 || y > y1) continue;
        int wx = x * CELL + CELL / 2 + (int)(w % 5) * 6 - 12, wz = -(y * CELL + CELL / 2) + (int)(w % 3) * 8 - 8;
        nubbin(wx, wz, goodCol[k->good & 7], fr * 400 + w * 333);
        box(wx - 6, -46, wz - 6, wx + 6, -34, wz + 6, goodCol[k->good & 7]); }   // (the good, on its head)
}
static void cursor_draw(void) {
    int cx, cy; if (!cursor_cell(&cx, &cy)) return;
    int bx = cx, by = cy, w = 1;
    if (hand.mode >= KHM_HUT && hand.mode <= KHM_TAVERN) { bx = cx - 1 + ((isin(gyaw) > 0) ? 1 : 0); by = cy - 1 + ((icos(gyaw) < 0) ? 1 : 0); w = 2; }
    int ok = w == 2 ? ksSpotOk(&S, hand.mode - KHM_HUT, bx, by) : 1, p = 140 + (isin((fr * 120) & 4095) >> 6);
    gfx_set_identity();
    u32 c = ok ? UI_RGB(p / 2, p, p / 2) : UI_RGB(p, p / 4, p / 4);
    int x0 = bx * CELL, z0 = -(by * CELL), x1 = x0 + w * CELL, z1 = z0 - w * CELL;
    box(x0, -4, z1, x1, -1, z1 + 6, c); box(x0, -4, z0 - 6, x1, -1, z0, c); box(x0, -4, z1, x0 + 6, -1, z0, c); box(x1 - 6, -4, z1, x1, -1, z0, c);
}
static void panel_goods(int y) {
    ui_round(4, y, 312, 30, 8, UI_CARD, UI_CARD2, 0);
    for (int g = 0; g < 8; g++) { int x = 10 + (g & 3) * 77, yy = y + 3 + (g >> 2) * 13; char b[24]; snprintf(b, sizeof b, "%s %d", khGood[g], R.goods[g]);
        ui_disc(x + 4, yy + 5, 7, goodCol[g]); ui_textfit(x + 11, yy, 64, b, UI_INK); }
}
void realm_draw(void) {
    gfx_set_clear(150, 196, 236); gfx_sky(0xecc496u, 0xf0e0c8u);
    gfx_set_ambient(100, 96);
    if (!live) {   // FOUND A REALM
        gfx_set_fog(0, 0);
        ui_backdrop();
        int top = ui_window(40, 20, 240, 200, "FOUND A REALM", IC_STAR);
        static const char *const nm[3] = { "HAMLET  (48 x 48)", "REALM  (96 x 96)", "KINGDOM  (128 x 128)" };
        static const char *const tx[3] = { "A LITTLE ISLAND. QUICK TO FILL", "ROOM FOR A TOWN AND A CASTLE", "A BIG LAND. A LONG REIGN" };
        ui_wrapc(160, top, 220, 3, "YOU WILL WAKE UP A GIANT ON AN ISLAND. THE LITTLE FOLK, THE NUBBINS, NEED YOU. HOW BIG A LAND?", UI_SOFT);
        for (int i = 0; i < 3; i++) { int y = top + 44 + i * 36, s = i == found_sel;
            if (s) ui_round(48, y - 2, 224, 32, 6, UI_RGB(226, 242, 254), UI_HILITE, 0);
            ui_text(58, y + 2, 1, nm[i], s ? UI_BLUE2 : UI_INK); ui_text(58, y + 15, 1, tx[i], UI_SOFT); }
        ui_hintbar(214, "DPAD PICK|X FOUND IT|O NOT NOW");
        return;
    }
    // the camera: behind and above the giant, turned with L1 R1
    int fx = gx, fz = gz, pitch = 640, dist = 1250, yaw = (2048 - cyaw) & 4095;
    cam.at.vx = fx; cam.at.vy = -170; cam.at.vz = fz;
    cam.eye.vx = fx - ((isin(yaw) * ((dist * icos(pitch)) >> 12)) >> 12);
    cam.eye.vz = fz - ((icos(yaw) * ((dist * icos(pitch)) >> 12)) >> 12);
    cam.eye.vy = -170 - ((dist * isin(pitch)) >> 12);
    gfx_camera_update();
    gfx_set_fog(1300, 2100);
    land_draw();
    cursor_draw();
    {   // the giant: your own Bore
        Gait g; memset(&g, 0, sizeof g); g.clock = fr; g.mood = 60; g.phase = gphase; g.amp = gmove ? 4096 : 0;
        if (S.hd.k != KHD_NONE) { static VPose carry; carry.arm[0] = carry.arm[1] = -1300; carry.elbow[0] = carry.elbow[1] = -500; carry.aroll[0] = -100; carry.aroll[1] = 100;
            carry.leg[0] = (s16)(gmove ? (isin(gphase) >> 4) : 0); carry.leg[1] = (s16)-carry.leg[0]; carry.knee[0] = carry.knee[1] = 120; g.pose = &carry; }
        VECTOR p = { gx, 0, gz }; voxel_draw(&sims[player].bore, &p, gyaw, &g); gfx_shadow(&p, 90);
        if (S.hd.k == KHD_GOODS) { gfx_set_identity(); int hx = gx + (isin(gyaw) * 70 >> 12), hz = gz - (icos(gyaw) * 70 >> 12); for (int j = 0; j < S.hd.n && j < 6; j++) box(hx - 14, -330 - j * 14, hz - 14, hx + 14, -316 - j * 14, hz + 14, goodCol[S.hd.g & 7]); }
        if (S.hd.k == KHD_BLD) { gfx_set_identity(); int hx = gx + (isin(gyaw) * 70 >> 12), hz = gz - (icos(gyaw) * 70 >> 12); box(hx - 50, -400, hz - 50, hx + 50, -320, hz + 50, UI_RGB(196, 160, 110)); }
    }
    gfx_set_fog(0, 0);
    // the HUD
    { char b[64]; ui_round(4, 4, 312, 18, 7, UI_RGB(30, 40, 70), UI_RGB(18, 24, 48), 0);
      snprintf(b, sizeof b, "DAY %ld   NUBBINS %d   WORKING %d   TIME X%d", khDay(R.day), S.pop, S.emp, spd == 0 ? 1 : spd == 1 ? 4 : 16); ui_text(12, 7, 1, b, UI_WHITE);
      char c[64]; khCost(c, &S, hand.mode); ui_round(4, 24, 312, 16, 6, UI_CARD, UI_CARD2, 0); ui_textfit(10, 26, 300, c, UI_BLUE2); }
    { int cx, cy; char b[64]; if (cursor_cell(&cx, &cy)) { khInfo(&S, cx, cy, b); ui_round(4, 196, 312, 16, 6, UI_CARD, UI_CARD2, 0); ui_textfit(10, 198, 300, b, UI_INK); } }
    { char h[40]; khHeld(&S, h); if (h[0]) { char b[56]; snprintf(b, sizeof b, "CARRYING: %s", h); ui_badge(8, 44, b, UI_AMBER, UI_INK); } }
    if (panel == 1) panel_goods(156);
    if (panel == 2 && mis < NMIS) {   // the mission
        const Mis *m = &miss[mis]; int top = ui_window(30, 46, 260, 140, m->nm, IC_STAR);
        ui_wrap(40, top, 240, 3, m->story, UI_SOFT);
        for (int k = 0; k < 3; k++) { int have = count_task(&m->t[k]), y = top + 46 + k * 20; char b[40]; snprintf(b, sizeof b, "%d/%d", MIN(have, m->t[k].n), m->t[k].n);
            ui_text(40, y, 1, m->t[k].nm, have >= m->t[k].n ? UI_GOODT : UI_INK); ui_text(276 - ui_tw(1, b), y, 1, b, have >= m->t[k].n ? UI_GOODT : UI_SOFT); }
        { char b[32]; snprintf(b, sizeof b, "PAYS $%d", m->pay * 100); ui_text(40, top + 108, 1, b, UI_GOLDT); }
    }
    if (panel == 2 && mis >= NMIS) { int top = ui_window(30, 60, 260, 60, "ALL MISSIONS DONE", IC_STAR); ui_wrap(40, top, 240, 2, "THE REALM IS YOURS TO KEEP. IT NEVER ENDS.", UI_SOFT); }
    if (msgT && msg) { int w = MIN(300, ui_tw(1, msg) + 20); ui_round(160 - w / 2, 172, w, 18, 8, UI_RGB(30, 40, 70), UI_RGB(18, 24, 48), 0); ui_textfit(170 - w / 2, 175, w - 20, msg, UI_WHITE); }
    if (!pick && !menu) ui_hintbar(216, "X DO IT|SQ BUILD|TRI GOODS/GOAL|L1 R1 TURN|START MENU");
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
