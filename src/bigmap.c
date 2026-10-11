// bigmap: see bigmap.h
#include "bigmap.h"

static u32 wseed = 0x5A1B0E2Au;
#define NSET_MAX 24
static Settle sets[NSET_MAX]; static int nset;
typedef struct { u16 c, a, b; u8 vert; } Seg;    // a highway along block edges: row (or column) c, from a to b
#define NSEG 72
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
    int e = (vn(bx, by, 7, s) * 8 + vn(bx, by, 6, s + 1) * 4 + vn(bx, by, 5, s + 2) * 2 + vn(bx, by, 4, s + 3)) / 15;
    int dx = ABS(bx - BM_N / 2), dy = ABS(by - BM_N / 2), d = MAX(dx, dy);
    if (d > 420) e -= (d - 420) * 170 / 92;                  // the land falls into the sea toward the edge of the map
    int dc = (dx + dy) / 2; if (dc < 256) e += (256 - dc) / 8;   // and rises a little toward the middle
    return e;
}
int bm_terrain(int bx, int by) {
    if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N) return BT_SEA;
    int e = elev(bx, by);
    if (e < 104) return BT_SEA;
    if (e < 110) return BT_BEACH;
    if (e > 186) return BT_ROCKY;
    u32 s = wseed + 11;
    int m = (vn(bx, by, 6, s) * 2 + vn(bx, by, 4, s + 1)) / 3;
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
        for (int i = 0; i < nset; i++) { int dx = x - sets[i].x, dy = y - sets[i].y, need = r + sets[i].r + 10;
            if (dx * dx + dy * dy < need * need) { far = 0; break; } }
        if (!far || !land_ok(x, y, r)) continue;
        Settle *s = &sets[nset]; s->x = (u16)x; s->y = (u16)y; s->r = (u8)r; s->kind = (u8)kind;
        if (nset == 0) strcpy(s->name, "BORE CITY"); else make_name(s->name, kind);
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
void bm_new(u32 seed) {
    wseed = seed ? seed : 1; rs = wseed ^ 0xA5A5A5A5u; nset = 0; nseg = 0;
    place_set(ST_CITY, 11, BM_N / 2, BM_N / 2, 110);                        // BORE CITY, near the middle
    for (int i = 0; i < 2; i++) place_set(ST_CITY, 9, BM_N / 2, BM_N / 2, 360);
    for (int i = 0; i < 6; i++) place_set(ST_TOWN, 5 + (int)(rr() % 2), BM_N / 2, BM_N / 2, 400);
    for (int i = 0; i < 12; i++) place_set(ST_VILLAGE, 3 + (int)(rr() % 2), BM_N / 2, BM_N / 2, 420);
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
}
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
static const u8 mixCore[] = { LOT_TOWER, 12, LOT_OFFICE, 4, LOT_SHOP, 12, LOT_DINER, 7, LOT_HOTEL, 4, LOT_INN, 2, LOT_GYM, 4, LOT_ARCADE, 3, LOT_POOL, 2, LOT_PARK, 6,
                              LOT_SQUARE, 2, LOT_SKATE, 2, LOT_EMPTY, 4, LOT_APART, 22, LOT_TOWN, 8, LOT_TWO, 6, 0, 0 };
static const u8 mixMid[]  = { LOT_HOUSE, 20, LOT_TWO, 13, LOT_TOWN, 13, LOT_COTTAGE, 10, LOT_APART, 8, LOT_SHOP, 6, LOT_DINER, 3, LOT_OFFICE, 3, LOT_GYM, 2, LOT_POOL, 1,
                              LOT_SCHOOL, 1, LOT_PARK, 7, LOT_EMPTY, 9, LOT_ARCADE, 1, LOT_INN, 2, LOT_TOWER, 1, 0, 0 };
static const u8 mixOut[]  = { LOT_HOUSE, 20, LOT_COTTAGE, 22, LOT_TWO, 8, LOT_EMPTY, 24, LOT_PARK, 6, LOT_FARM, 12, LOT_SHOP, 4, LOT_DINER, 2, LOT_INN, 2, 0, 0 };
static const u8 mixVil[]  = { LOT_HOUSE, 22, LOT_COTTAGE, 28, LOT_EMPTY, 24, LOT_FARM, 16, LOT_PARK, 4, LOT_SHOP, 3, LOT_DINER, 3, 0, 0 };
static int landmark(const Settle *s, int dx, int dy) {   // what every town has at its heart
    if (dx == 0 && dy == 0) return LOT_SQUARE;
    if (s->kind == ST_VILLAGE) {
        if (dx == 1 && dy == 0) return LOT_SHOP; if (dx == -1 && dy == 0) return LOT_DINER; if (dx == 0 && dy == 1) return LOT_INN;
        return -1;
    }
    static const signed char lm[][3] = { {1,0,LOT_OFFICE}, {-1,0,LOT_SHOP}, {0,1,LOT_DINER}, {0,-1,LOT_INN}, {1,1,LOT_POOL}, {-1,-1,LOT_SCHOOL},
                                         {1,-1,LOT_GYM}, {-1,1,LOT_PARK}, {2,0,LOT_ARCADE}, {-2,0,LOT_SKATE}, {0,2,LOT_TOWER}, {0,-2,LOT_SHOP},
                                         {2,2,LOT_HOTEL}, {-2,-2,LOT_POOL}, {3,0,LOT_SCHOOL}, {-3,0,LOT_TOWER} };
    int n = s->kind == ST_CITY ? 16 : 9;
    for (int i = 0; i < n; i++) if (lm[i][0] == dx && lm[i][1] == dy) return lm[i][2];
    return -1;
}
void bm_block(int bx, int by, BInfo *b) {
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
        const u8 *mix = s->kind == ST_VILLAGE ? mixVil : (s->kind == ST_CITY && q < 64) ? mixCore : q < 170 ? mixMid : mixOut;
        b->lot = (u8)pick(b->h >> 4, mix);
        if (b->terrain == BT_BEACH && b->lot == LOT_FARM) b->lot = LOT_PARK;
        return;
    }
    if (on_road(bx, by, 0)) b->road |= 1;
    if (on_road(bx, by, 1)) b->road |= 2;
    if (b->terrain == BT_GRASS && (b->h % 100u) < 3) b->lot = LOT_FARM;
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
int bm_is_place(int lot) { return lot >= LOT_SHOP && lot <= LOT_SKATE; }
static const char *const lotNm[LOT_N] = { "OPEN COUNTRY", "HOUSE", "COTTAGE", "TOWNHOUSES", "TWO-STOREY", "FLATS", "SHOP", "DINER", "GYM", "OFFICE", "INN",
                                          "POOL", "SCHOOL", "ARCADE", "OFFICE TOWER", "HOTEL", "PARK", "TOWN SQUARE", "SKATE PLAZA", "EMPTY LOT", "FARM" };
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
    static const char *const shop[8] = { "BOREMART", "QUICK BUY", "DIPPA'S DELI", "MOSS MARKET", "THE CORNER SHOP", "SNOOZ & SONS", "YAWN-MART", "BIG BUY" };
    static const char *const diner[6] = { "THE DIPPER DINER", "GREASY SPOON", "ROUTE 9 DINER", "MOM'S KITCHEN", "THE NOODLE BAR", "PANCAKE PALACE" };
    static const char *const gym[4] = { "IRON BORE GYM", "FLEX ZONE", "PUMP HOUSE", "SWEAT SHOP" };
    static const char *const off[6] = { "BORECORP", "DULLTECH", "GREYBOX LTD", "SNOOZE INSURANCE", "PAPER & CO", "BEIGE BANK" };
    static const char *const inn[4] = { "THE SLEEPY INN", "DOZE MOTEL", "THE YAWNING LION", "NAPTIME HOTEL" };
    static const char *const pool[3] = { "COMMUNITY POOL", "SPLASH ZONE", "THE LIDO" };
    static const char *const sch[3] = { "BORE HIGH", "DIPPER ELEMENTARY", "MOSS ACADEMY" };
    static const char *const arc[3] = { "PIXEL PALACE", "GAME ZONE", "8-BIT ALLEY" };
    static const char *const park[6] = { "ELM PARK", "MOSS GARDENS", "DIPPER GREEN", "QUIET PARK", "THE COMMONS", "NAPPER'S FIELD" };
    const char *s;
    switch (lot) {
        case LOT_SHOP: s = shop[h % 8u]; break;   case LOT_DINER: s = diner[h % 6u]; break; case LOT_GYM: s = gym[h % 4u]; break;
        case LOT_OFFICE: s = off[h % 6u]; break;  case LOT_INN: s = inn[h % 4u]; break;     case LOT_POOL: s = pool[h % 3u]; break;
        case LOT_SCHOOL: s = sch[h % 3u]; break;  case LOT_ARCADE: s = arc[h % 3u]; break;  case LOT_PARK: s = park[h % 6u]; break;
        case LOT_TOWER: s = off[(h >> 3) % 6u]; break; case LOT_HOTEL: s = inn[(h >> 2) % 4u]; break;
        default: s = bm_lot_name(lot); break;
    }
    strcpy(out, s);
    return out;
}
