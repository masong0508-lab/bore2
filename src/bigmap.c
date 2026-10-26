// bigmap: see bigmap.h
#include "bigmap.h"
int bm_packed, bm_gen = 3; char bm_cap_name[18];
int bm_land = BL_MAINLAND, bm_sea = 2, bm_clim = 2, bm_hills = 2, bm_ncity = 2, bm_ntown = 6, bm_nvil = 12, bm_capsz = 2;
void bm_dials_default(void) { bm_land = BL_MAINLAND; bm_sea = 2; bm_clim = 2; bm_hills = 2; bm_ncity = 2; bm_ntown = 6; bm_nvil = 12; bm_capsz = 2; }
static int packed(void) { return bm_packed || bm_gen < 3; }   // (PACKED towns: the option, or a world from before it)

static u32 wseed = 0x5A1B0E2Au;
#define NSET_MAX 62
static Settle sets[NSET_MAX]; static int nset;
static char ren[NSET_MAX][18];                     // towns the player renamed ("": as made); kept across bm_new, so a save can set them first
typedef struct { u16 c, a, b; u8 vert; } Seg;    // a highway along block edges: row (or column) c, from a to b
#define NSEG 140
static Seg segs[NSEG]; static int nseg;

u32 bm_hash(int a, int b, u32 s) {
    u32 h = (u32)a * 374761393u + (u32)b * 668265263u + s * 2246822519u + 0x9E3779B9u;
    h = (h ^ (h >> 13)) * 1274126177u; h ^= h >> 16;
    return h;
}
u32 bm_seed(void) { return wseed; }
const char *bm_state_name(void) { return "SAN BOREAS"; }

// ---- the land: value noise (a random height at every lattice point, smoothly blended between) ----
static int vn(int x, int y, int sh, u32 salt) {
    int ix = x >> sh, iy = y >> sh, m = (1 << sh) - 1;
    int fx = ((x & m) << 8) >> sh, fy = ((y & m) << 8) >> sh;
    fx = (fx * fx * (768 - 2 * fx)) >> 16; fy = (fy * fy * (768 - 2 * fy)) >> 16;
    int a = (int)(bm_hash(ix, iy, salt) & 255), b = (int)(bm_hash(ix + 1, iy, salt) & 255);
    int c = (int)(bm_hash(ix, iy + 1, salt) & 255), d = (int)(bm_hash(ix + 1, iy + 1, salt) & 255);
    int ab = a + (((b - a) * fx) >> 8), cd = c + (((d - c) * fx) >> 8);
    return ab + (((cd - ab) * fy) >> 8);
}
static int elev(int bx, int by) {
    u32 s = wseed;
    int dx = ABS(bx - BM_N / 2), dy = ABS(by - BM_N / 2), d = MAX(dx, dy), dc = (dx + dy) / 2;
    int e;
    if (bm_land == BL_ISLANDS) {                              // ISLANDS: smaller hills of land, more sea between, one big enough for the capital
        e = (vn(bx, by, 7, s) * 3 + vn(bx, by, 6, s + 1) * 6 + vn(bx, by, 5, s + 2) * 4 + vn(bx, by, 4, s + 3) * 2) / 15 - 20;
        if (d > 440) e -= (d - 440) * 170 / 72;
        if (dc < 80) e += (80 - dc) * 2 / 3;
    } else {
        e = (vn(bx, by, 7, s) * 8 + vn(bx, by, 6, s + 1) * 4 + vn(bx, by, 5, s + 2) * 2 + vn(bx, by, 4, s + 3)) / 15;
        if (bm_land == BL_BIGISLE) {                          // ONE BIG ISLAND: the sea all round, closer in
            int r = (d + dc) / 2; if (r > 250) e -= (r - 250) * 170 / 110;
            if (dc < 256) e += (256 - dc) / 6;
        } else {
            int edge = bm_land == BL_CONTINENT ? 490 : 420;   // (CONTINENT: land nearly to the map's edge)
            if (d > edge) e -= (d - edge) * 170 / (BM_N / 2 - edge);   // the land falls into the sea toward the edge of the map
            if (dc < 256) e += (256 - dc) / 8;                // and rises a little toward the middle
            if (bm_land == BL_CONTINENT) e += 6;
            if (bm_land == BL_LAKES) { int l = vn(bx, by, 5, s + 7); if (l > 170 && dc > 40) e -= (l - 170) * 2; }   // LAKES: inland water all over
        }
    }
    return e - (bm_sea - 2) * 8;                              // SEA: the water higher or lower
}
int bm_terrain(int bx, int by) {
    if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N) return BT_SEA;
    int e = elev(bx, by);
    if (e < 104) return BT_SEA;
    if (e < 110) return BT_BEACH;
    static const u8 rock[5] = { 240, 206, 186, 172, 158 };   // HILLS: FLAT .. MOUNTAINS
    if (e > rock[bm_hills % 5]) return BT_ROCKY;
    u32 s = wseed + 11;
    int m = (vn(bx, by, 6, s) * 2 + vn(bx, by, 4, s + 1)) / 3 + (bm_clim - 2) * 30;   // CLIMATE: DESERT .. JUNGLE
    if (m < 96) return BT_DESERT;
    if (m > 162) return BT_FOREST;
    return BT_GRASS;
}

// ---- the towns and cities ----
static u32 rs;
static u32 rr(void) { rs = rs * 1103515245u + 12345u; return rs >> 8; }
static const char *const nmA[16] = { "BORE", "DIP", "MOSS", "GRUM", "PEB", "WOB", "SNOOZ", "DOZ", "YAWN", "MUMBL", "FLOP", "SLUMB", "NAP", "HUMDR", "LULL", "DRON" };
static const char *const nmB[16] = { "VILLE", "TON", "FORD", "BURY", "WOOD", "FIELD", "HAVEN", "PORT", "BROOK", "DALE", "STON", "WICK", "MOUTH", "CREEK", "FALLS", "SPRINGS" };
static const char *const nmP[12] = { "PORT ", "SAN ", "LOS ", "NEW ", "OLD ", "LAKE ", "FORT ", "EAST ", "WEST ", "NORTH ", "SOUTH ", "MOUNT " };
static void make_name(char *o, int kind) {
    for (int t = 0; t < 40; t++) {
        char b[40]; b[0] = 0;
        int pre = kind != ST_VILLAGE && (rr() % 3) == 0;
        if (pre) { const char *p = nmP[rr() % 12]; strcpy(b, p); }
        strcat(b, nmA[rr() % 16]); strcat(b, nmB[rr() % 16]);
        if (strlen(b) > 17) continue;
        int dup = 0; for (int i = 0; i < nset; i++) if (!strcmp(sets[i].name, b)) dup = 1;
        if (dup) continue;
        strcpy(o, b); return;
    }
    strcpy(o, "NOWHERE");
}
static int land_ok(int x, int y, int r) {        // most of the circle is land
    static const signed char ox[9] = { 0, 7, -7, 0, 0, 5, -5, 5, -5 }, oy[9] = { 0, 0, 0, 7, -7, 5, 5, -5, -5 };
    int n = 0;
    for (int k = 0; k < 9; k++) if (bm_terrain(x + ox[k] * r / 10, y + oy[k] * r / 10) != BT_SEA) n++;
    return bm_terrain(x, y) != BT_SEA && n >= 7;
}
static int place_set(int kind, int r, int cx, int cy, int spread) {
    if (nset >= NSET_MAX) return -1;
    for (int t = 0; t < 600; t++) {
        int x = cx + (int)(rr() % (u32)(2 * spread + 1)) - spread, y = cy + (int)(rr() % (u32)(2 * spread + 1)) - spread;
        if (x < 40 || y < 40 || x > BM_N - 40 || y > BM_N - 40) continue;
        int far = 1;
        for (int i = 0; i < nset; i++) { int dx = x - sets[i].x, dy = y - sets[i].y, need = r + sets[i].r + (packed() ? 10 : 26);   // (open country between towns, not one town into the next)
            if (dx * dx + dy * dy < need * need) { far = 0; break; } }
        if (!far || !land_ok(x, y, r)) continue;
        Settle *s = &sets[nset]; s->x = (u16)x; s->y = (u16)y; s->r = (u8)r; s->kind = (u8)kind;
        if (nset == 0) strcpy(s->name, bm_cap_name[0] ? bm_cap_name : "BORE CITY"); else make_name(s->name, kind);
        return nset++;
    }
    return -1;
}
static void add_seg(int c, int a, int b, int vert) {
    if (nseg >= NSEG) return;
    Seg *s = &segs[nseg++]; s->c = (u16)c; s->a = (u16)MIN(a, b); s->b = (u16)MAX(a, b); s->vert = (u8)vert;
}
static void link(int i, int j) {                  // a highway: along i's row to j's column, then along j's column to j
    add_seg(sets[i].y, sets[i].x, sets[j].x, 0);
    add_seg(sets[j].x, sets[i].y, sets[j].y, 1);
}
static void rivers_make(void);
static int mg_cx = -1;
void bm_new(u32 seed) {
    wseed = seed ? seed : 1; rs = wseed ^ 0xA5A5A5A5u; nset = 0; nseg = 0;
    static const u8 capr[6] = { 6, 8, 11, 14, 18, 22 };
    place_set(ST_CITY, capr[bm_capsz % 6], BM_N / 2, BM_N / 2, 110);       // BORE CITY, near the middle (CAPITAL SIZE)
    if (!nset) place_set(ST_CITY, 9, BM_N / 2, BM_N / 2, 300);              // (a watery world: wherever there is room)
    if (!nset) place_set(ST_CITY, 6, BM_N / 2, BM_N / 2, 440);
    if (!nset) { Settle *c = &sets[0]; c->x = c->y = BM_N / 2; c->r = 6; c->kind = ST_CITY; strcpy(c->name, bm_cap_name[0] ? bm_cap_name : "BORE CITY"); nset = 1; }
    for (int i = 0; i < MIN(bm_ncity, BM_MAXCITY); i++) place_set(ST_CITY, 9, BM_N / 2, BM_N / 2, 360);   // (CITIES, TOWNS, VILLAGES: as many as asked, room allowing)
    for (int i = 0; i < MIN(bm_ntown, BM_MAXTOWN); i++) place_set(ST_TOWN, 5 + (int)(rr() % 2), BM_N / 2, BM_N / 2, 400);
    for (int i = 0; i < MIN(bm_nvil, BM_MAXVIL); i++) place_set(ST_VILLAGE, 3 + (int)(rr() % 2), BM_N / 2, BM_N / 2, 420);
    // the highways: the shortest network that reaches every town (Prim), and a few more for loops
    u8 in[NSET_MAX] = { 0 }; in[0] = 1;
    for (int k = 1; k < nset; k++) {
        int bi = -1, bj = -1, bd = 1 << 30;
        for (int i = 0; i < nset; i++) if (in[i]) for (int j = 0; j < nset; j++) if (!in[j]) {
            int d = ABS(sets[i].x - sets[j].x) + ABS(sets[i].y - sets[j].y);
            if (d < bd) { bd = d; bi = i; bj = j; } }
        if (bj < 0) break;
        in[bj] = 1; link(bi, bj);
    }
    for (int e = 0; e < 4 && nset > 3; e++) { int i = (int)(rr() % (u32)nset), j = (int)(rr() % (u32)nset); if (i != j) link(i, j); }
    for (int i = 0; i < nset; i++) if (ren[i][0]) strcpy(sets[i].name, ren[i]);
    rivers_make();
}
void bm_rename_clear(void) { memset(ren, 0, sizeof ren); }
void bm_rename(int i, const char *nm) { if (i < 0 || i >= NSET_MAX) return; strncpy(ren[i], nm, 17); ren[i][17] = 0; if (i < nset && ren[i][0]) strcpy(sets[i].name, ren[i]); }
const char *bm_renamed(int i) { return i >= 0 && i < NSET_MAX ? ren[i] : ""; }
int bm_nset(void) { return nset; }
int bm_nseg(void) { return nseg; }
void bm_seg(int i, int *c, int *a, int *b, int *vert) { const Seg *g = &segs[i]; *c = g->c; *a = g->a; *b = g->b; *vert = g->vert; }
const Settle *bm_set(int i) { return (i >= 0 && i < nset) ? &sets[i] : 0; }
int bm_capital(void) { return 0; }

// ---- a block ----
static int set_at(int bx, int by) {
    for (int i = 0; i < nset; i++) { int dx = bx - sets[i].x, dy = by - sets[i].y, r = sets[i].r; if (dx * dx + dy * dy <= r * r) return i; }
    return 255;
}
int bm_set_at(int bx, int by) { return set_at(bx, by); }
static int on_road(int bx, int by, int vert) {
    for (int i = 0; i < nseg; i++) { const Seg *s = &segs[i]; if (s->vert != vert) continue;
        if (vert ? (s->c == bx && by >= s->a && by <= s->b) : (s->c == by && bx >= s->a && bx <= s->b)) return 1; }
    return 0;
}
static int pick(u32 r, const u8 *tab) {          // tab: lot, weight, ... 0,0 (weights sum to 100)
    int v = (int)(r % 100u);
    for (; tab[1]; tab += 2) { if (v < tab[1]) return tab[0]; v -= tab[1]; }
    return LOT_EMPTY;
}
static const u8 mixCore[] = { LOT_TOWER, 10, LOT_OFFICE, 4, LOT_SHOP, 9, LOT_DINER, 5, LOT_HOTEL, 4, LOT_INN, 2, LOT_GYM, 3, LOT_ARCADE, 2, LOT_POOL, 2, LOT_PARK, 7,
                              LOT_SQUARE, 3, LOT_SKATE, 2, LOT_EMPTY, 2, LOT_APART, 13, LOT_TOWN, 6, LOT_TWO, 4,
                              LOT_CAFE, 4, LOT_BAR, 4, LOT_CINEMA, 2, LOT_SALON, 2, LOT_RESTAURANT, 3, LOT_CLINIC, 2, LOT_LIBRARY, 1, LOT_SPA, 2, LOT_GALLERY, 2, 0, 0 };
// (from the middle ring out, room to breathe: parks, and blocks left green, wider apart the further out)
static const u8 mixMid[]  = { LOT_HOUSE, 17, LOT_TWO, 10, LOT_TOWN, 9, LOT_COTTAGE, 9, LOT_APART, 6, LOT_SHOP, 5, LOT_DINER, 3, LOT_OFFICE, 3, LOT_GYM, 2, LOT_POOL, 1,
                              LOT_SCHOOL, 1, LOT_PARK, 10, LOT_EMPTY, 6, LOT_ARCADE, 1, LOT_INN, 2, LOT_TOWER, 1, LOT_WILD, 4,
                              LOT_CAFE, 3, LOT_BAR, 2, LOT_SALON, 1, LOT_RESTAURANT, 1, LOT_CLINIC, 1, LOT_CINEMA, 1, LOT_LIBRARY, 1, 0, 0 };
static const u8 mixOut[]  = { LOT_HOUSE, 18, LOT_COTTAGE, 18, LOT_TWO, 6, LOT_EMPTY, 16, LOT_PARK, 8, LOT_FARM, 10, LOT_WILD, 14, LOT_SHOP, 3, LOT_DINER, 2, LOT_INN, 1, LOT_CAFE, 2, LOT_BAR, 2, 0, 0 };
// PACKED (the TOWNS option, and every world made before the roomy towns): a home or a shop on nearly every block
static const u8 mixMidP[] = { LOT_HOUSE, 18, LOT_TWO, 12, LOT_TOWN, 12, LOT_COTTAGE, 9, LOT_APART, 8, LOT_SHOP, 5, LOT_DINER, 3, LOT_OFFICE, 3, LOT_GYM, 2, LOT_POOL, 1,
                              LOT_SCHOOL, 1, LOT_PARK, 6, LOT_EMPTY, 6, LOT_ARCADE, 1, LOT_INN, 2, LOT_TOWER, 1,
                              LOT_CAFE, 3, LOT_BAR, 2, LOT_SALON, 1, LOT_RESTAURANT, 1, LOT_CLINIC, 1, LOT_CINEMA, 1, LOT_LIBRARY, 1, 0, 0 };
static const u8 mixOutP[] = { LOT_HOUSE, 20, LOT_COTTAGE, 22, LOT_TWO, 8, LOT_EMPTY, 20, LOT_PARK, 6, LOT_FARM, 12, LOT_SHOP, 4, LOT_DINER, 2, LOT_INN, 2, LOT_CAFE, 2, LOT_BAR, 2, 0, 0 };
static const u8 mixVilP[] = { LOT_HOUSE, 22, LOT_COTTAGE, 28, LOT_EMPTY, 21, LOT_FARM, 16, LOT_PARK, 4, LOT_SHOP, 3, LOT_DINER, 3, LOT_BAR, 3, 0, 0 };
static const u8 mixCoreP[] = { LOT_TOWER, 10, LOT_OFFICE, 4, LOT_SHOP, 9, LOT_DINER, 5, LOT_HOTEL, 4, LOT_INN, 2, LOT_GYM, 3, LOT_ARCADE, 2, LOT_POOL, 2, LOT_PARK, 5,
                              LOT_SQUARE, 2, LOT_SKATE, 2, LOT_EMPTY, 2, LOT_APART, 16, LOT_TOWN, 6, LOT_TWO, 4,
                              LOT_CAFE, 4, LOT_BAR, 4, LOT_CINEMA, 2, LOT_SALON, 2, LOT_RESTAURANT, 3, LOT_CLINIC, 2, LOT_LIBRARY, 1, LOT_SPA, 2, LOT_GALLERY, 2, 0, 0 };
static const u8 mixVil[]  = { LOT_HOUSE, 18, LOT_COTTAGE, 22, LOT_EMPTY, 18, LOT_FARM, 16, LOT_PARK, 5, LOT_WILD, 12, LOT_SHOP, 3, LOT_DINER, 3, LOT_BAR, 3, 0, 0 };
static int landmark(const Settle *s, int dx, int dy) {   // what every town has at its heart
    if (dx == 0 && dy == 0) return LOT_SQUARE;
    if (s->kind == ST_VILLAGE) {
        if (dx == 1 && dy == 0) return LOT_SHOP; if (dx == -1 && dy == 0) return LOT_DINER; if (dx == 0 && dy == 1) return LOT_INN; if (dx == 0 && dy == -1) return LOT_BAR;
        return -1;
    }
    static const signed char lm[][3] = { {1,0,LOT_OFFICE}, {-1,0,LOT_SHOP}, {0,1,LOT_DINER}, {0,-1,LOT_INN}, {1,1,LOT_POOL}, {-1,-1,LOT_SCHOOL},
                                         {1,-1,LOT_GYM}, {-1,1,LOT_PARK}, {2,0,LOT_ARCADE}, {2,-1,LOT_CAFE}, {-2,1,LOT_BAR},   // (a town: these eleven)
                                         {-2,0,LOT_SKATE}, {0,2,LOT_TOWER}, {0,-2,LOT_SHOP}, {2,2,LOT_HOTEL}, {-2,-2,LOT_POOL}, {3,0,LOT_SCHOOL}, {-3,0,LOT_TOWER},
                                         {1,2,LOT_RESTAURANT}, {-1,-2,LOT_CLINIC}, {2,1,LOT_CINEMA}, {-2,-1,LOT_SALON}, {1,-2,LOT_LIBRARY}, {-1,2,LOT_SPA},
                                         {3,1,LOT_GALLERY} };
    int n = s->kind == ST_CITY ? 25 : 11;
    for (int i = 0; i < n; i++) if (lm[i][0] == dx && lm[i][1] == dy) return lm[i][2];
    return -1;
}
static void bm_block0(int bx, int by, BInfo *b) {
    b->terrain = (u8)bm_terrain(bx, by);
    b->h = bm_hash(bx, by, wseed ^ 0xB10Cu);
    b->set = 255; b->road = 0; b->lot = LOT_WILD;
    if (b->terrain == BT_SEA) {
        if (on_road(bx, by, 0)) b->road |= 1;      // a causeway over the water
        if (on_road(bx, by, 1)) b->road |= 2;
        return;
    }
    int si = set_at(bx, by);
    if (si != 255) {
        const Settle *s = &sets[si];
        b->set = (u8)si; b->road = 3;
        int dx = bx - s->x, dy = by - s->y, q = (dx * dx + dy * dy) * 256 / (s->r * s->r);
        int lm = landmark(s, dx, dy);
        if (lm >= 0) { b->lot = (u8)lm; return; }
        int pk = packed();
        const u8 *mix = s->kind == ST_VILLAGE ? (pk ? mixVilP : mixVil) : (s->kind == ST_CITY && q < 64) ? (pk ? mixCoreP : mixCore) : q < 170 ? (pk ? mixMidP : mixMid) : (pk ? mixOutP : mixOut);
        b->lot = (u8)pick(b->h >> 4, mix);
        if (b->terrain == BT_BEACH && b->lot == LOT_FARM) b->lot = LOT_PARK;
        return;
    }
    if (on_road(bx, by, 0)) b->road |= 1;
    if (on_road(bx, by, 1)) b->road |= 2;
    if (b->terrain == BT_GRASS && (b->h % 100u) < 3) b->lot = LOT_FARM;
}
// ---- rivers and mega places ----
static u8 rvOn[NSET_MAX], rvV[NSET_MAX]; static s8 rvOff[NSET_MAX];
static void rivers_make(void) {   // some towns have a river through them: along a row (or a column) of blocks a little off the centre
    mg_cx = -1;
    for (int i = 0; i < NSET_MAX; i++) rvOn[i] = 0;
    for (int i = 0; i < nset; i++) {
        const Settle *s = &sets[i]; u32 h = bm_hash(i, 77, wseed ^ 0x71E5u);
        int ch = s->kind == ST_CITY ? 62 : s->kind == ST_TOWN ? 38 : 0;
        if ((int)(h % 100u) >= ch || s->r < 4) continue;
        int mag = 2 + (int)((h >> 9) % (u32)MIN(3, s->r - 3));
        rvOn[i] = 1; rvV[i] = (u8)((h >> 8) & 1); rvOff[i] = (s8)(((h >> 12) & 1) ? -mag : mag);
    }
}
static int river_at(int bx, int by, int si) {   // 0: no, 1: runs west-east, 2: north-south (the river belongs to town si and runs a little way out of it)
#ifdef RIVER_TEST
    if (by % 5 == 0) return 1; if (bx % 5 == 2) return 2;   // (test builds: rivers everywhere)
#endif
    for (int i = 0; i < nset; i++) if (rvOn[i] && (si == 255 || si == i)) {
        int dx = bx - sets[i].x, dy = by - sets[i].y, r = sets[i].r + 3;
        if (rvV[i]) { if (dx == rvOff[i] && ABS(dy) <= r) return 2; } else if (dy == rvOff[i] && ABS(dx) <= r) return 1;
    }
    return 0;
}
static const u8 mallMix[]  = { LOT_SHOP, 28, LOT_CAFE, 14, LOT_RESTAURANT, 10, LOT_SALON, 10, LOT_CINEMA, 12, LOT_ARCADE, 10, LOT_GYM, 6, LOT_GALLERY, 5, LOT_SPA, 5, 0, 0 };
static const u8 plazaMix[] = { LOT_TOWER, 34, LOT_OFFICE, 20, LOT_HOTEL, 16, LOT_RESTAURANT, 10, LOT_CAFE, 8, LOT_BAR, 6, LOT_GALLERY, 6, 0, 0 };
static int mega_kind(int gx, int gy, int si) {   // -1: not a group; else MK_ (a group is the 2x2 blocks from an even block)
    static int cgx = -1, cgy = -1, csi = -1, ck;
    if (mg_cx < 0) { cgx = -1; mg_cx = 0; }                // (a new world: forget the last answer)
    if (gx == cgx && gy == cgy && si == csi) return ck;
    cgx = gx; cgy = gy; csi = si; ck = -1;
    const Settle *s = &sets[si]; if (s->kind == ST_VILLAGE) return ck;
    u32 h = bm_hash(gx, gy, wseed ^ 0x3E6Au);
    if ((int)(h % 100u) >= (s->kind == ST_CITY ? 17 : 10)) return ck;
    for (int k = 0; k < 4; k++) {
        int x = gx + (k & 1), y = gy + (k >> 1); BInfo t; bm_block0(x, y, &t);
        if (t.set != si || t.terrain == BT_SEA || landmark(s, x - s->x, y - s->y) >= 0 || river_at(x, y, si)) return ck;
    }
    int k = (int)((h >> 8) % 100u);
    ck = s->kind == ST_CITY ? (k < 34 ? MK_PARK : k < 64 ? MK_MALL : MK_PLAZA) : (k < 50 ? MK_PARK : MK_MALL);
    return ck;
}
void bm_block(int bx, int by, BInfo *b) {
    bm_block0(bx, by, b);
    if (b->terrain == BT_SEA || bm_gen < 4) return;   // (rivers and mega places: worlds made from this version on)
    int rv = river_at(bx, by, b->set);
    if (rv) { b->lot = LOT_RIVER; if (rv == 2) b->road |= BR_MEGA; else b->road &= (u8)~BR_MEGA; return; }
    if (b->set == 255 || b->lot == LOT_WILD) return;
    int k = mega_kind(bx & ~1, by & ~1, b->set);
    if (k < 0) return;
    int part = (bx & 1) | ((by & 1) << 1);
    b->road = (u8)((b->road & 7) | BR_MEGA | (part << 4) | (k << 6));
    b->lot = (u8)(k == MK_PARK ? LOT_PARK : pick(b->h >> 4, k == MK_MALL ? mallMix : plazaMix));
    if (k == MK_MALL && part == 0) b->lot = LOT_SHOP;
}
int bm_storeys(const BInfo *b) {
    if (b->lot == LOT_TWO) return 2;
    if (b->lot == LOT_APART) return b->set != 255 && sets[b->set].kind == ST_CITY && (b->h >> 27) < 20 ? 3 + (int)((b->h >> 12) % 3u) : 2 + (int)((b->h >> 12) & 1);
    if (b->lot == LOT_TOWER) return 3 + (int)((b->h >> 12) % 3u);
    if (b->lot == LOT_HOTEL) return 3 + (int)((b->h >> 12) & 1);
    return 1;
}
int bm_units(const BInfo *b) {
    switch (b->lot) { case LOT_TOWN: return 2; case LOT_HOUSE: case LOT_COTTAGE: case LOT_TWO: return 1; case LOT_APART: return 2 * bm_storeys(b); default: return 0; }
}
int bm_is_place(int lot) { return (lot >= LOT_SHOP && lot <= LOT_SKATE) || (lot >= LOT_CAFE && lot <= LOT_GALLERY); }
static const char *const lotNm[LOT_N] = { "OPEN COUNTRY", "HOUSE", "COTTAGE", "TOWNHOUSES", "TWO-STOREY", "FLATS", "SHOP", "DINER", "GYM", "OFFICE", "INN",
                                          "POOL", "SCHOOL", "ARCADE", "OFFICE TOWER", "HOTEL", "PARK", "TOWN SQUARE", "SKATE PLAZA", "EMPTY LOT", "FARM",
                                          "CAFE", "BAR", "CINEMA", "SALON", "RESTAURANT", "CLINIC", "LIBRARY", "SPA", "GALLERY", "RIVER" };
const char *bm_lot_name(int lot) { return lotNm[lot % LOT_N]; }

static const char *const stNm[24] = { "MAIN", "ELM", "OAK", "PINE", "MAPLE", "BIRCH", "CEDAR", "ASH", "WILLOW", "LAKE", "HILL", "PARK",
                                      "MILL", "CHURCH", "BRIDGE", "RIVER", "SPRING", "MARKET", "HIGH", "STATION", "DIPPER", "MOSS", "BORE", "YAWN" };
static const char *const stSf[4] = { " ST", " AVE", " RD", " LN" };
const char *bm_street(int set, int bx, int by, int unit, char *out) {
    int sx = set < nset ? sets[set].x : 0, sy = set < nset ? sets[set].y : 0;
    int num = 100 + ((bx - sx + 64) & 127) * 4 + unit * 2;
    u32 h = bm_hash(by - sy, set, wseed ^ 0x57EEu);
    sprintf(out, "%d %s%s", num, stNm[h % 24u], stSf[(h >> 8) & 3]);
    return out;
}
const char *bm_place_name(int lot, u32 h, char *out) {
    static const char *const shop[5][4] = { { "BOREMART", "MOSS MARKET", "YAWN-MART", "DIPPA'S DELI" }, { "SNOOZ & SONS", "THREADBARE", "DRESSED TO SNOOZE", "THE CLOTHES LINE" },
        { "GAME BARN", "PLAY PIT", "QUICK BUY", "LEVEL UP" }, { "BYTE BARN", "GADGET GARAGE", "BLEEP & BLOOP", "ZAP ELECTRONICS" }, { "SPIN CITY", "THE CORNER SHOP", "GROOVE BIN", "BIG BUY" } };   // (by kind: places.c)
    static const char *const diner[6] = { "THE DIPPER DINER", "GREASY SPOON", "ROUTE 9 DINER", "MOM'S KITCHEN", "THE NOODLE BAR", "PANCAKE PALACE" };
    static const char *const gym[4] = { "IRON BORE GYM", "FLEX ZONE", "PUMP HOUSE", "SWEAT SHOP" };
    static const char *const off[6] = { "BORECORP", "DULLTECH", "GREYBOX LTD", "SNOOZE INSURANCE", "PAPER & CO", "BEIGE BANK" };
    static const char *const inn[4] = { "THE SLEEPY INN", "DOZE MOTEL", "THE YAWNING LION", "NAPTIME HOTEL" };
    static const char *const pool[3] = { "COMMUNITY POOL", "SPLASH ZONE", "THE LIDO" };
    static const char *const sch[3] = { "BORE HIGH", "DIPPER ELEMENTARY", "MOSS ACADEMY" };
    static const char *const arc[3] = { "PIXEL PALACE", "GAME ZONE", "8-BIT ALLEY" };
    static const char *const park[6] = { "ELM PARK", "MOSS GARDENS", "DIPPER GREEN", "QUIET PARK", "THE COMMONS", "NAPPER'S FIELD" };
    static const char *const cafe[6] = { "THE DAILY GRIND", "BEAN THERE", "THE DOZY BEAN", "CAFE MOSS", "PERK UP", "THE SLEEPY CUP" };
    static const char *const bar[6] = { "THE YAWNING DOG", "LAST ORDERS", "THE NIGHT OWL", "BORE'S TAVERN", "THE LOCAL", "DIPPER'S PUB" };
    static const char *const cin[4] = { "THE ROXY", "BORE CINEMA", "STARLIGHT", "THE BIJOU" };
    static const char *const sal[4] = { "SNIP SNAP", "CURL UP & DYE", "THE MANE EVENT", "SHEAR BLISS" };
    static const char *const res[6] = { "CHEZ BORE", "THE GOLDEN SPOON", "LA SNOOZA", "THE SILVER FORK", "MOSS & THYME", "DINE O'CLOCK" };
    static const char *const cli[4] = { "BORE GENERAL", "MOSS MEDICAL", "DIPPER CLINIC", "WELLNESS CENTRE" };
    static const char *const lib[3] = { "BORE LIBRARY", "THE READING ROOM", "MOSS LIBRARY" };
    static const char *const spa[4] = { "SERENITY SPA", "THE STEAM ROOM", "ZEN DEN", "BLISS SPA" };
    static const char *const gal[4] = { "THE GREY GALLERY", "MOSS MODERN", "BEIGE ART HOUSE", "DIPPER MUSEUM" };
    const char *s;
    switch (lot) {
        case LOT_SHOP: s = shop[(h >> 8) % 5u][h % 4u]; break;   case LOT_DINER: s = diner[h % 6u]; break; case LOT_GYM: s = gym[h % 4u]; break;
        case LOT_OFFICE: s = off[h % 6u]; break;  case LOT_INN: s = inn[h % 4u]; break;     case LOT_POOL: s = pool[h % 3u]; break;
        case LOT_SCHOOL: s = sch[h % 3u]; break;  case LOT_ARCADE: s = arc[h % 3u]; break;  case LOT_PARK: s = park[h % 6u]; break;
        case LOT_TOWER: s = off[(h >> 3) % 6u]; break; case LOT_HOTEL: s = inn[(h >> 2) % 4u]; break;
        case LOT_CAFE: s = cafe[h % 6u]; break;   case LOT_BAR: s = bar[h % 6u]; break;     case LOT_CINEMA: s = cin[h % 4u]; break;
        case LOT_SALON: s = sal[h % 4u]; break;   case LOT_RESTAURANT: s = res[h % 6u]; break; case LOT_CLINIC: s = cli[h % 4u]; break;
        case LOT_LIBRARY: s = lib[h % 3u]; break; case LOT_SPA: s = spa[h % 4u]; break;     case LOT_GALLERY: s = gal[h % 4u]; break;
        default: s = bm_lot_name(lot); break;
    }
    strcpy(out, s);
    return out;
}
