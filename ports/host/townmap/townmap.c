// townmap: a town's plan from above and a world's numbers, from src/bigmap.c alone (the town-plan check of docs/HOUSING_DESIGN.md).
//   make -C ports/host/townmap                 builds ./townmap (and ./townmap_old: bigmap.c as it was before the plan, for `make check`)
//   ./townmap pic SEED GEN WHICH OUT.ppm       one settlement at 4 pixels a tile. WHICH: city (the capital), town, village (the first of each), or a settlement's number
//   ./townmap stats SEED GEN [packed]          the counts for a whole world: lots, homes, places against the game's caps, streets, green, the plan's checks
//   ./townmap sum SEED GEN [packed]            a checksum of every block of every town and a grid of the country (older worlds must keep theirs: `make check`)
//   python3 mkpics.py                          docs/plan7_city.png, plan7_town.png, plan7_village.png: world version 6 on the left, 7 on the right
// The picture is drawn from bm_block() and bm_lot_plan() only, as the game's own views are to be: streets and the pavements beside them, every
// lot in the colour of what it is, each home's body and wing as its roof, the door (yellow) and its path, the driveway. A building that is not a
// home is drawn at the footprint its generator in src/lots.inc gives it. Version 6 homes are drawn as src/lots.inc builds them (old_home below).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "common.h"
#include "bigmap.h"
#undef sprintf
#undef snprintf
int bsprintf(char *o, const char *f, ...) { va_list a; va_start(a, f); int n = vsprintf(o, f, a); va_end(a); return n; }
int bsnprintf(char *o, unsigned cap, const char *f, ...) { va_list a; va_start(a, f); int n = vsnprintf(o, cap, f, a); va_end(a); return n; }
void boot(const char *s) { (void)s; }
void loading(const char *w, int p) { (void)w; (void)p; }
int ram8 = 0, dbg_miss[4];
void *xalloc(size_t n) { return calloc(1, n); }
void world_gh_reset(void) {}
#ifdef TOWNMAP_OLD   /* bigmap.c from before the plan: nothing to ask */
enum { NK_NONE }; enum { LF_NONE, LF_SOUTH, LF_NORTH }; enum { HT_NONE, HT_COMPACT, HT_RANCH, HT_L, HT_BOX, HT_TWO, HT_TERRACE, HT_MANSION }; enum { RF_SIDE, RF_FRONT, RF_HIP };
typedef struct { u8 face, type, x0, y0, x1, y1, wx0, wy0, wx1, wy1, dx, dy, d2x, drv, yard, mir, roof, pal, wall, rcol; } LotPlan;
static int bm_nb(int bx, int by) { (void)bx; (void)by; return NK_NONE; }
static int bm_lot_face(int bx, int by) { (void)bx; (void)by; return LF_SOUTH; }
static void bm_lot_plan(int bx, int by, const BInfo *b, LotPlan *p) { (void)bx; (void)by; (void)b; memset(p, 0, sizeof *p); p->face = LF_SOUTH; }
#endif

#define B BM_BLK
#define L0 BM_LOT0
static int gen;

// ---- the game's caps (src/pop.c, src/pop.h, src/far.c) ----
#define CAP_PLACE 1800      /* PLACE_MAX */
#define CAP_FAM2  999       /* NFAM_2MB: households on a plain machine */
#define CAP_FAM8  1234      /* NFAM */
#define CAP_FAR   2400      /* FB_MAX: buildings the far view keeps */

// ---- what a block is, as the game paints it ----
static int roads(const BInfo *b) {   // its streets (world.c blk_roads: a big place's inner streets are gone)
    int rd = b->road & 7;
    if (b->lot != LOT_RIVER && (b->road & BR_MEGA)) { int p = BR_PART(b->road); if (p & 1) rd &= ~2; if (p & 2) rd &= ~1; }
    return rd;
}
static int in_town(const BInfo *b) { return b->set != 255 && b->terrain != BT_SEA; }
static int is_home(int l) { return l == LOT_HOUSE || l == LOT_COTTAGE || l == LOT_TOWN || l == LOT_TWO; }
static int is_open(int l) { return l == LOT_WILD || l == LOT_PARK || l == LOT_SQUARE || l == LOT_SKATE || l == LOT_FARM || l == LOT_RIVER; }   // nothing there needs a street door (a farm is its fields)
enum { CAT_WILD, CAT_HOME, CAT_FLATS, CAT_SHOP, CAT_WORK, CAT_CIVIC, CAT_PARK, CAT_PLAZA, CAT_EMPTY, CAT_FARM, CAT_WATER, CAT_N };
static int cat_of(int l) {
    switch (l) {
        case LOT_HOUSE: case LOT_COTTAGE: case LOT_TOWN: case LOT_TWO: return CAT_HOME;
        case LOT_APART: return CAT_FLATS;
        case LOT_OFFICE: case LOT_TOWER: case LOT_HOTEL: case LOT_INN: return CAT_WORK;
        case LOT_SCHOOL: case LOT_LIBRARY: case LOT_CLINIC: case LOT_POOL: case LOT_GYM: case LOT_GALLERY: case LOT_SPACE: case LOT_OBS: return CAT_CIVIC;
        case LOT_PARK: return CAT_PARK;
        case LOT_SQUARE: case LOT_SKATE: return CAT_PLAZA;
        case LOT_EMPTY: return CAT_EMPTY;
        case LOT_FARM: return CAT_FARM;
        case LOT_RIVER: return CAT_WATER;
        case LOT_WILD: return CAT_WILD;
        default: return CAT_SHOP;
    }
}
// each lot's building in lot tiles (x0, y0, x1, y1), facing south, from its generator in src/lots.inc (0 0 0 0: none)
static const u8 foot[LOT_N][4] = {
    [LOT_SHOP] = { 1, 1, 11, 9 }, [LOT_DINER] = { 1, 2, 11, 9 }, [LOT_GYM] = { 1, 2, 11, 9 }, [LOT_OFFICE] = { 1, 2, 11, 9 }, [LOT_INN] = { 0, 1, 12, 11 },
    [LOT_SCHOOL] = { 0, 1, 12, 8 }, [LOT_ARCADE] = { 1, 2, 11, 9 }, [LOT_TOWER] = { 1, 1, 11, 10 }, [LOT_HOTEL] = { 0, 0, 12, 12 }, [LOT_APART] = { 0, 1, 12, 10 },
    [LOT_CAFE] = { 1, 2, 11, 9 }, [LOT_BAR] = { 1, 2, 11, 9 }, [LOT_CINEMA] = { 0, 0, 12, 11 }, [LOT_SALON] = { 1, 2, 11, 9 }, [LOT_RESTAURANT] = { 0, 1, 12, 10 },
    [LOT_CLINIC] = { 1, 1, 11, 10 }, [LOT_LIBRARY] = { 0, 1, 12, 10 }, [LOT_SPA] = { 1, 1, 11, 10 }, [LOT_GALLERY] = { 1, 2, 11, 9 }, [LOT_GAS] = { 1, 1, 11, 5 },
    [LOT_CONV] = { 2, 3, 10, 9 }, [LOT_FARM] = { 8, 1, 12, 6 }, [LOT_SPACE] = { 1, 1, 11, 6 }, [LOT_OBS] = { 1, 2, 11, 9 } };
static int far_bld(int l) { return (l >= LOT_HOUSE && l <= LOT_HOTEL) || (l >= LOT_FARM && l <= LOT_CONV); }   // (far.c has_bld)

// the homes of a world from before the plan, as src/lots.inc builds them (city_house, city_cottage, city_town, city_two): the "before" of the pictures
static void old_home(const BInfo *b, LotPlan *p) {
    unsigned h = b->h; int lx = L0, ly = L0;
    memset(p, 0, sizeof *p); p->face = LF_SOUTH;
    if (b->lot == LOT_HOUSE) {
        int grand = ((h >> 24) & 3) == 0, sz = (int)((h >> 26) & 15);
        int w = grand ? (sz == 0 ? 13 : sz < 5 ? 11 : sz < 9 ? 12 : 13) : 9 + (int)((h >> 3) % 4), hh = grand ? (sz == 0 ? 12 : sz < 5 ? 10 : 11) : 8 + (int)((h >> 5) % 3);
        int hx = lx + (13 - w) / 2, hy = grand ? ly : ly + 1 + (int)((h >> 7) % 2);
        if (!grand && hy + hh - 1 > ly + 10) hy = ly + 11 - hh;
        p->type = grand ? HT_MANSION : HT_BOX; p->x0 = (u8)hx; p->y0 = (u8)hy; p->x1 = (u8)(hx + w - 1); p->y1 = (u8)(hy + hh - 1);
        p->dx = (u8)(grand ? hx + w / 2 + (int)((h >> 20) % 3u) - 1 : hx + 2 + (int)((h >> 20) % (unsigned)(w - 4))); p->dy = p->y1;
    } else if (b->lot == LOT_COTTAGE) {
        int w = 8 + (int)((h >> 4) & 1), hh = 7 + (int)((h >> 5) & 1), hx = lx + 2 + (int)((h >> 6) & 1), hy = ly + 2 + (int)((h >> 7) & 1);
        if ((h >> 8) & 1) hy = ly + 11 - hh;
        p->type = HT_COMPACT; p->x0 = (u8)hx; p->y0 = (u8)hy; p->x1 = (u8)(hx + w - 1); p->y1 = (u8)(hy + hh - 1); p->dx = (u8)(hx + w / 2); p->dy = p->y1;
    } else if (b->lot == LOT_TOWN) {
        p->type = HT_TERRACE; p->x0 = (u8)lx; p->y0 = (u8)(ly + 1); p->x1 = (u8)(lx + 6); p->y1 = (u8)(ly + 9);
        p->wx0 = (u8)(lx + 6); p->wy0 = p->y0; p->wx1 = (u8)(lx + 12); p->wy1 = p->y1; p->dx = (u8)(lx + 3); p->d2x = (u8)(lx + 9); p->dy = p->y1;
    } else if (b->lot == LOT_TWO) {
        p->type = HT_TWO; p->x0 = (u8)(lx + 2); p->y0 = (u8)(ly + 1); p->x1 = (u8)(lx + 10); p->y1 = (u8)(ly + 8); p->dx = (u8)(lx + 6); p->dy = p->y1;
    }
    p->yard = (u8)(B - 1 - p->y1); p->roof = (u8)((h >> 9) % 3); p->pal = 255;   // (pal 255: the colours are the block's own dice, as the far view's)
}
static void plan_of(int bx, int by, const BInfo *b, LotPlan *p) { if (gen >= 7) bm_lot_plan(bx, by, b, p); else if (is_home(b->lot)) old_home(b, p); else { memset(p, 0, sizeof *p); p->face = LF_SOUTH; } }
static int face_of(int bx, int by, const BInfo *b) {   // which street the lot's door is on. Version 6 and before: always the south side, and LF_NONE here says no street is there
    if (gen >= 7) return bm_lot_face(bx, by);
    if (b->lot != LOT_RIVER && (b->road & BR_MEGA) && !(BR_PART(b->road) & 2)) return LF_SOUTH;   // (a big place's court)
    BInfo t; bm_block(bx, by + 1, &t); return (roads(&t) & 1) ? LF_SOUTH : LF_NONE;
}

// ---- a block as tiles ----
enum { K_COUNTRY, K_SEA, K_ROAD, K_PAVE, K_BLDG, K_HARD, K_WATER, K_GREEN, K_N };   // what a tile is, for the counts (K_HARD: a path, a driveway, a paved court; K_GREEN: lawn, park, field, woods, an empty lot)
typedef struct { int kind, x0, y0, x1, y1; unsigned col; } Mark;    // drawn over the tiles at pixel size: 0 an outline, 1 a ridge (x1: RF_), 2 a door, 3 a dot
typedef struct { u8 k[B][B]; unsigned c[B][B]; Mark m[24]; int nm; } Ras;
#define RGB3(r, g, b) ((unsigned)(r) << 16 | (unsigned)(g) << 8 | (unsigned)(b))
static const unsigned terr_col[6] = { RGB3(74, 124, 184), RGB3(224, 208, 152), RGB3(134, 176, 102), RGB3(92, 140, 80), RGB3(216, 192, 134), RGB3(152, 152, 142) };
enum { COL_ROAD = RGB3(74, 74, 80), COL_AVE = RGB3(232, 200, 84), COL_LINE = RGB3(188, 188, 188), COL_PAVE = RGB3(202, 200, 190), COL_LAWN = RGB3(164, 206, 122),
       COL_SAND = RGB3(226, 210, 156), COL_PATH = RGB3(214, 206, 188), COL_DRIVE = RGB3(150, 150, 152), COL_DOOR = RGB3(255, 236, 40), COL_WATER = RGB3(92, 156, 214),
       COL_PARK = RGB3(104, 180, 92), COL_TREE = RGB3(52, 120, 60), COL_WILD = RGB3(112, 158, 92), COL_SOIL = RGB3(178, 150, 96), COL_CROP = RGB3(150, 186, 84),
       COL_EMPTY = RGB3(206, 190, 150), COL_PLAZA = RGB3(218, 212, 200), COL_SHOPLOT = RGB3(236, 220, 198), COL_WORKLOT = RGB3(214, 216, 224), COL_CIVLOT = RGB3(206, 222, 238),
       COL_SHOP = RGB3(206, 98, 78), COL_WORK = RGB3(104, 122, 156), COL_CIVIC = RGB3(78, 130, 204), COL_FLATS = RGB3(150, 104, 134), COL_BARN = RGB3(150, 70, 56), COL_DECK = RGB3(196, 226, 232) };
// a neighbourhood's palette: two roof colours (what you see from above) and two wall colours (the outline)
static const unsigned pal_roof[8][2] = { { RGB3(184, 88, 62), RGB3(132, 96, 70) }, { RGB3(96, 112, 140), RGB3(150, 84, 70) }, { RGB3(158, 62, 56), RGB3(120, 108, 96) }, { RGB3(88, 128, 96), RGB3(140, 104, 76) },
                                         { RGB3(204, 126, 86), RGB3(100, 116, 138) }, { RGB3(126, 80, 62), RGB3(84, 120, 110) }, { RGB3(170, 102, 62), RGB3(110, 110, 118) }, { RGB3(112, 134, 156), RGB3(170, 110, 80) } };
static const unsigned pal_wall[8][2] = { { RGB3(236, 226, 200), RGB3(240, 240, 236) }, { RGB3(214, 190, 150), RGB3(236, 234, 226) }, { RGB3(240, 240, 236), RGB3(226, 214, 190) }, { RGB3(226, 214, 190), RGB3(186, 206, 170) },
                                         { RGB3(240, 240, 236), RGB3(214, 190, 150) }, { RGB3(236, 226, 200), RGB3(180, 200, 214) }, { RGB3(226, 214, 190), RGB3(236, 234, 226) }, { RGB3(240, 240, 236), RGB3(226, 180, 180) } };
static const unsigned old_wall[8] = { RGB3(226, 214, 190), RGB3(176, 96, 76), RGB3(236, 234, 226), RGB3(214, 190, 150), RGB3(160, 160, 164), RGB3(180, 200, 214), RGB3(186, 206, 170), RGB3(226, 180, 180) };   // (far.c wallc, roofc)
static const unsigned old_roof[4] = { RGB3(150, 60, 50), RGB3(70, 80, 110), RGB3(90, 70, 60), RGB3(60, 110, 70) };
static void put(Ras *r, int x, int y, int k, unsigned c) { if (x >= 0 && y >= 0 && x < B && y < B) { r->k[y][x] = (u8)k; r->c[y][x] = c; } }
static void box(Ras *r, int x0, int y0, int x1, int y1, int k, unsigned c) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) put(r, x, y, k, c); }
static void mark(Ras *r, int kind, int x0, int y0, int x1, int y1, unsigned c) { if (r->nm < 24) { Mark *m = &r->m[r->nm++]; m->kind = kind; m->x0 = x0; m->y0 = y0; m->x1 = x1; m->y1 = y1; m->col = c; } }
static unsigned shade(unsigned c, int pct) { int rr = (int)(c >> 16) * pct / 100, g = (int)((c >> 8) & 255) * pct / 100, bb = (int)(c & 255) * pct / 100; return RGB3(MIN(rr, 255), MIN(g, 255), MIN(bb, 255)); }
static void building(Ras *r, int x0, int y0, int x1, int y1, unsigned roof, unsigned wall, int form) {
    box(r, x0, y0, x1, y1, K_BLDG, roof); mark(r, 0, x0, y0, x1, y1, wall); if (form >= 0) mark(r, 1, x0, y0, x1, y1, (unsigned)form);
}
static void ras_block(int bx, int by, Ras *r) {
    BInfo b, bs, be; bm_block(bx, by, &b); bm_block(bx, by + 1, &bs); bm_block(bx + 1, by, &be);
    int town = in_town(&b), rd = roads(&b), rds = roads(&bs), rde = roads(&be);
    unsigned ground = terr_col[b.terrain % 6];
    if (town) ground = (b.terrain == BT_DESERT || b.terrain == BT_BEACH) ? COL_SAND : COL_LAWN;
    r->nm = 0;
    box(r, 0, 0, B - 1, B - 1, b.terrain == BT_SEA ? K_SEA : town ? K_GREEN : K_COUNTRY, ground);
    int ave_t = 0, ave_l = 0;                                                 // (version 7: an avenue's line is yellow)
    if (gen >= 7 && town) { const Settle *s = bm_set(b.set); ave_t = !((by - s->y) & 3); ave_l = !((bx - s->x) & 3); }
    if (rd & 1) { box(r, 0, 0, B - 1, L0 - 1, K_ROAD, COL_ROAD); for (int x = (rd & 2) ? L0 : 0; x < B; x++) if (ave_t || (x & 1)) put(r, x, L0 / 2, K_ROAD, ave_t ? COL_AVE : COL_LINE); }
    if (rd & 2) { box(r, 0, 0, L0 - 1, B - 1, K_ROAD, COL_ROAD); for (int y = (rd & 1) ? L0 : 0; y < B; y++) if (ave_l || (y & 1)) put(r, L0 / 2, y, K_ROAD, ave_l ? COL_AVE : COL_LINE); }
    if ((rd & 7) == 4) box(r, 0, 0, L0 - 1, L0 - 1, K_ROAD, COL_ROAD);          // (only the corner is paved)
    if (!town) return;
    int mg = b.lot != LOT_RIVER && (b.road & BR_MEGA), part = BR_PART(b.road), mk = BR_KIND(b.road);
    // the pavements: version 6 and before a town block always has its row and column (world.c); the plan has one on both sides of every street and nowhere else
    if (gen < 7 || (rd & 1)) box(r, L0, L0, B - 1, L0, K_PAVE, COL_PAVE);
    if (gen < 7 || (rd & 2)) box(r, L0, L0, L0, B - 1, K_PAVE, COL_PAVE);
    if (gen >= 7 && (rds & 1) && in_town(&bs)) box(r, L0, B - 1, B - 1, B - 1, K_PAVE, COL_PAVE);
    if (gen >= 7 && (rde & 2) && in_town(&be)) box(r, B - 1, L0, B - 1, B - 1, K_PAVE, COL_PAVE);
    if (mg && part) {                                                          // a big place's inner strips: its court (world.c mega_zone)
        unsigned fl = mk == MK_PARK ? COL_PARK : COL_PLAZA; int k = mk == MK_PARK ? K_GREEN : K_HARD;
        if (part & 1) { box(r, 0, (part & 2) ? 0 : L0, L0 - 1, B - 1, k, fl); if (mk == MK_PARK) box(r, L0 / 2, (part & 2) ? 0 : L0, L0 / 2, B - 1, K_HARD, COL_PATH); }
        if (part & 2) { box(r, (part & 1) ? 0 : L0, 0, B - 1, L0 - 1, k, fl); if (mk == MK_PARK) box(r, (part & 1) ? 0 : L0, L0 / 2, B - 1, L0 / 2, K_HARD, COL_PATH); }
        if (part == 3) { if (mk == MK_PARK) box(r, 0, 0, L0 - 1, L0 - 1, K_WATER, COL_WATER); else box(r, 1, 1, 3, 3, K_WATER, COL_WATER); }
    }
    LotPlan p; plan_of(bx, by, &b, &p);
    int face = gen >= 7 ? p.face : LF_SOUTH, north = face == LF_NORTH, l = b.lot, cat = cat_of(l);
    #define LY(y) (north ? L0 + 12 - (y) : L0 + (y))                           /* a lot row of a south-facing generator, turned for the facing */
    if (l == LOT_RIVER) {                                                      // (world.c lot_river: the water with a walk on each bank; the street along the edge crosses it on a bridge)
        int vert = b.road & BR_MEGA;
        for (int d = 5; d < B; d++) for (int e = 0; e < B; e++) {
            unsigned c = d <= 6 ? COL_PAVE : d <= 13 ? COL_WATER : d <= 15 ? COL_PAVE : ground; int k = d <= 6 ? K_PAVE : d <= 13 ? K_WATER : d <= 15 ? K_PAVE : K_GREEN;
            if (e < L0 && d >= 7 && d <= 13 && (gen < 7 || (b.road & (vert ? 1 : 2)))) { c = COL_ROAD; k = K_ROAD; }   // the bridge (the plan: only where a street crosses)
            else if (e < L0 && !(d >= 7 && d <= 13)) continue;
            if (vert) put(r, d, e, k, c); else put(r, e, d, k, c);
        }
        return;
    }
    switch (cat) {
        case CAT_PARK: box(r, L0 + (gen < 7), L0 + (gen < 7), B - 1, B - 1, K_GREEN, COL_PARK); box(r, L0 + 6, L0 + 1, L0 + 6, B - 1, K_HARD, COL_PATH);
            for (int y = L0 + 1; y < B; y++) for (int x = L0 + 1; x < B; x++) if (bm_hash(bx * B + x, by * B + y, 77) % 9u == 0 && x != L0 + 6) put(r, x, y, K_GREEN, COL_TREE);
            break;
        case CAT_WILD: for (int y = (rd & 1) ? L0 + 1 : 0; y < B; y++) for (int x = (rd & 2) ? L0 + 1 : 0; x < B; x++) { if (r->k[y][x] != K_GREEN) continue; put(r, x, y, K_GREEN, bm_hash(bx * B + x, by * B + y, 78) % 5u == 0 ? COL_TREE : COL_WILD); }
            break;
        case CAT_PLAZA: box(r, L0, L0, B - 1, B - 1, K_HARD, COL_PLAZA); if (l == LOT_SQUARE) box(r, L0 + 5, L0 + 5, L0 + 7, L0 + 7, K_WATER, COL_WATER); else box(r, L0 + 4, L0 + 5, L0 + 8, L0 + 6, K_HARD, shade(COL_PLAZA, 80));
            break;
        case CAT_EMPTY: box(r, L0 + 1, L0 + 1, B - 2, B - 2, K_GREEN, COL_EMPTY); mark(r, 3, L0 + 6, LY(11), 0, 0, RGB3(200, 40, 40));
            break;
        case CAT_FARM: box(r, L0 + 1, L0 + 1, B - 1, B - 1, K_GREEN, COL_CROP); for (int y = 1; y <= 11; y += 2) box(r, L0 + 1, LY(y), L0 + 6, LY(y), K_GREEN, COL_SOIL);
            building(r, L0 + 8, MIN(LY(1), LY(6)), L0 + 12, MAX(LY(1), LY(6)), COL_BARN, shade(COL_BARN, 60), RF_FRONT); mark(r, 2, L0 + 10, LY(6), 0, 0, COL_DOOR);
            break;
        default: break;
    }
    if (cat == CAT_HOME && p.type) {
        unsigned roof, wall;
        if (p.pal == 255) { roof = old_roof[(b.h >> 9) & 3]; wall = old_wall[(b.h >> 20) % 8u]; } else { roof = pal_roof[p.pal & 7][p.rcol & 1]; wall = pal_wall[p.pal & 7][p.wall & 1]; }
        if (p.type == HT_TWO || p.type == HT_MANSION) roof = shade(roof, 86);
        int n = face == LF_NORTH;
        if (p.drv) for (int y = n ? L0 : B - 1; n ? y <= p.y1 : y >= p.y0; y += n ? 1 : -1) {   // the driveway: from the street edge to the first wall in its way, or to the body's back line
            if (r->k[y][p.drv] == K_BLDG || r->k[y][p.drv + 1] == K_BLDG) break;
            int w0 = p.wx1 && y >= p.wy0 && y <= p.wy1 && p.drv + 1 >= p.wx0 && p.drv <= p.wx1, b0 = y >= p.y0 && y <= p.y1 && p.drv + 1 >= p.x0 && p.drv <= p.x1;
            if (w0 || b0) break;
            put(r, p.drv, y, K_HARD, COL_DRIVE); put(r, p.drv + 1, y, K_HARD, COL_DRIVE); }
        for (int y = n ? L0 : B - 1; y != p.dy; y += n ? 1 : -1) { put(r, p.dx, y, K_HARD, COL_PATH); if (p.d2x) put(r, p.d2x, y, K_HARD, COL_PATH); }   // the path from each door to the street
        if (p.wx1) building(r, p.wx0, p.wy0, p.wx1, p.wy1, p.type == HT_TERRACE ? roof : shade(roof, 112), wall, p.type == HT_TERRACE ? p.roof : -1);
        building(r, p.x0, p.y0, p.x1, p.y1, roof, wall, p.roof);
        mark(r, 2, p.dx, p.dy, n, 0, COL_DOOR); if (p.d2x) mark(r, 2, p.d2x, p.dy, n, 0, COL_DOOR);
        if (p.type == HT_TWO || p.type == HT_MANSION) mark(r, 0, p.x0 + 1, p.y0 + 1, p.x1 - 1, p.y1 - 1, shade(roof, 70));   // (an upstairs: a second line inside)
    } else if (foot[l][2] && cat != CAT_FARM) {
        const u8 *f = foot[l]; unsigned lotc = cat == CAT_SHOP ? COL_SHOPLOT : cat == CAT_WORK ? COL_WORKLOT : cat == CAT_CIVIC ? COL_CIVLOT : COL_LAWN;
        unsigned roof = cat == CAT_SHOP ? COL_SHOP : cat == CAT_WORK ? COL_WORK : cat == CAT_CIVIC ? COL_CIVIC : COL_FLATS;
        if (l == LOT_TOWER || l == LOT_HOTEL) roof = shade(roof, 80);
        if (cat != CAT_FLATS) box(r, L0 + 1, L0 + 1, B - 1 - (gen >= 7), B - 1 - (gen >= 7), K_HARD, lotc);
        if (l == LOT_GAS) box(r, L0, MIN(LY(6), LY(12)), B - 1, MAX(LY(6), LY(12)), K_HARD, COL_DRIVE);                // the forecourt
        int y0 = MIN(LY(f[1]), LY(f[3])), y1 = MAX(LY(f[1]), LY(f[3])), dy = north ? y0 : y1, dx = L0 + (f[0] + f[2]) / 2;
        if (l == LOT_APART) { for (int y = north ? L0 : B - 1; y != dy; y += north ? 1 : -1) { put(r, L0 + 2, y, K_HARD, COL_PATH); put(r, L0 + 5, y, K_HARD, COL_PATH); put(r, L0 + 10, y, K_HARD, COL_PATH); } }
        else for (int y = north ? L0 : B - 1; y != dy; y += north ? 1 : -1) put(r, dx, y, K_HARD, COL_PATH);
        building(r, L0 + f[0], y0, L0 + f[2], y1, roof, shade(roof, 60), -1);
        mark(r, 2, dx, dy, north, 0, COL_DOOR);
        if (l == LOT_TOWER || l == LOT_HOTEL || l == LOT_APART) mark(r, 0, L0 + f[0] + 1, y0 + 1, L0 + f[2] - 1, y1 - 1, shade(roof, 70));
    } else if (l == LOT_POOL) {                                                // the community pool: a fenced deck, the water
        box(r, L0, L0, B - 1, B - 1, K_HARD, COL_DECK); mark(r, 0, L0, L0, B - 1, B - 1, COL_CIVIC); box(r, L0 + 3, MIN(LY(3), LY(7)), L0 + 9, MAX(LY(3), LY(7)), K_WATER, COL_WATER);
        mark(r, 2, L0 + 6, LY(12), north, 0, COL_DOOR);
    }
    #undef LY
}

// ---- a picture of one settlement ----
static unsigned char *img; static int IW, IH;
static void px(int x, int y, unsigned c) { if (x >= 0 && y >= 0 && x < IW && y < IH) { unsigned char *p = img + (y * IW + x) * 3; p[0] = (unsigned char)(c >> 16); p[1] = (unsigned char)(c >> 8); p[2] = (unsigned char)c; } }
static void pline(int x0, int y0, int x1, int y1, unsigned c) { int n = MAX(ABS(x1 - x0), ABS(y1 - y0)); for (int i = 0; i <= n; i++) px(x0 + (n ? (x1 - x0) * i / n : 0), y0 + (n ? (y1 - y0) * i / n : 0), c); }
static int which_set(const char *w) {
    if (!strcmp(w, "city")) return bm_capital();
    int kind = !strcmp(w, "town") ? ST_TOWN : !strcmp(w, "village") ? ST_VILLAGE : -1;
    if (kind < 0) return atoi(w);
    for (int i = 0; i < bm_nset(); i++) if (bm_set(i)->kind == kind) return i;
    return 0;
}
static const char *const nkName[8] = { "", "CORE", "MAIN", "CIVIC", "URBAN", "SUBURB", "ESTATE", "GREEN" };
static int do_pic(const char *which, const char *out, int S) {
    int si = which_set(which); const Settle *s = bm_set(si); if (!s) return 1;
    int R = s->r + 2, N = 2 * R + 1; IW = IH = N * B * S; img = malloc((size_t)IW * IH * 3);
    static Ras r;
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) {
        int bx = s->x - R + i, by = s->y - R + j, ox = i * B * S, oy = j * B * S;
        ras_block(bx, by, &r);
        for (int y = 0; y < B; y++) for (int x = 0; x < B; x++) for (int v = 0; v < S; v++) for (int u = 0; u < S; u++) px(ox + x * S + u, oy + y * S + v, r.c[y][x]);
        for (int k = 0; k < r.nm; k++) { const Mark *m = &r.m[k]; int X0 = ox + m->x0 * S, Y0 = oy + m->y0 * S, X1 = ox + (m->x1 + 1) * S - 1, Y1 = oy + (m->y1 + 1) * S - 1;
            if (m->kind == 0) { pline(X0, Y0, X1, Y0, m->col); pline(X0, Y1, X1, Y1, m->col); pline(X0, Y0, X0, Y1, m->col); pline(X1, Y0, X1, Y1, m->col); }
            else if (m->kind == 1) { unsigned c = shade(r.c[m->y0][m->x0], 68); int cx = (X0 + X1) / 2, cy = (Y0 + Y1) / 2, in = MIN(X1 - X0, Y1 - Y0) / 2;
                if (m->col == RF_SIDE) pline(X0 + 1, cy, X1 - 1, cy, c);                 // the ridge along the street
                else if (m->col == RF_FRONT) pline(cx, Y0 + 1, cx, Y1 - 1, c);           // turned to the street
                else { int a = X0 + in, bb = X1 - in; if (a > bb) { a = bb = cx; } pline(a, cy, bb, cy, c); pline(X0 + 1, Y0 + 1, a, cy, c); pline(X0 + 1, Y1 - 1, a, cy, c); pline(X1 - 1, Y0 + 1, bb, cy, c); pline(X1 - 1, Y1 - 1, bb, cy, c); } }
            else if (m->kind == 2) { for (int v = 0; v < S; v++) for (int u = 0; u < S; u++) px(X0 + u, Y0 + v, m->col); }
            else { for (int v = 0; v < S; v++) for (int u = 0; u < S; u++) px(X0 + u, Y0 + v, m->col); } }
    }
    FILE *f = fopen(out, "wb"); if (!f) return 1;
    fprintf(f, "P6\n%d %d\n255\n", IW, IH); fwrite(img, 3, (size_t)IW * IH, f); fclose(f);
    printf("SET %d %s kind %d r %d at %d,%d size %d\n", si, s->name, s->kind, s->r, s->x, s->y, IW);
    if (gen >= 7) for (int ny = -(R / 4) - 1; ny <= R / 4 + 1; ny++) for (int nx = -(R / 4) - 1; nx <= R / 4 + 1; nx++) {   // each neighbourhood's kind, for the labels: where (pixels) and what
        int cnt = 0, kind = 0;
        for (int j = 0; j < 4; j++) for (int i = 0; i < 4; i++) { int k = bm_nb(s->x + nx * 4 + i, s->y + ny * 4 + j); if (k) { kind = k; cnt++; } }
        if (cnt >= 5) printf("NB %d %d %s\n", (nx * 4 + 2 + R) * B * S, (ny * 4 + 2 + R) * B * S, nkName[kind]);
    }
    return 0;
}

// ---- the numbers for a whole world ----
static const signed char lm_city[][2] = { {0,0}, {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {-1,-1}, {1,-1}, {-1,1}, {2,0}, {2,-1}, {-2,1}, {-2,0}, {0,2}, {0,-2}, {2,2}, {-2,-2}, {3,0}, {-3,0}, {1,2}, {-1,-2}, {2,1}, {-2,-1}, {1,-2}, {-1,2}, {3,1} };   // (bigmap.c landmark)
static const signed char lm_space[][2] = { {0,0}, {-1,0}, {1,0}, {0,1}, {0,-1}, {1,1}, {-1,-1}, {-1,1}, {1,-1}, {2,0} };
static int do_stats(void) {
    static int lots[LOT_N], kinds[8], types[8], faces[3], tiles[K_N];
    int units = 0, places = 0, farb = 0, blocks = 0, landlocked = 0, lm_n = 0, lm_bad = 0, same_type = 0, same_look = 0, pairs = 0, bad_plan = 0, short_sets = 0;
    static Ras r; char miss[512] = "";
    for (int si = 0; si < bm_nset(); si++) {
        const Settle *s = bm_set(si); int have[4] = { 0, 0, 0, 0 };
        for (int by = s->y - s->r; by <= s->y + s->r; by++) for (int bx = s->x - s->r; bx <= s->x + s->r; bx++) {
            BInfo b; bm_block(bx, by, &b); if (b.set != si || b.terrain == BT_SEA) continue;
            blocks++; lots[b.lot % LOT_N]++; units += bm_units(&b); places += bm_is_place(b.lot); farb += far_bld(b.lot);
            if (b.lot == LOT_EMPTY) have[0]++; if (b.lot == LOT_HOUSE) have[1]++; if (b.lot == LOT_COTTAGE) have[2]++; if (b.lot == LOT_TWO) have[3]++;
            kinds[bm_nb(bx, by) & 7]++;
            int face = face_of(bx, by, &b);
            if (!is_open(b.lot)) { faces[face]++; if (face == LF_NONE) { landlocked++; if (getenv("VERBOSE")) printf("  landlocked: %s at %d,%d (%s %+d,%+d)\n", bm_lot_name(b.lot), bx, by, s->name, bx - s->x, by - s->y); } }
            ras_block(bx, by, &r); for (int y = 0; y < B; y++) for (int x = 0; x < B; x++) tiles[r.k[y][x]]++;
            if (is_home(b.lot)) {                                              // the plan's own rules
                LotPlan p, q; plan_of(bx, by, &b, &p); types[p.type & 7]++;
                int lo = (b.road & 2) ? L0 + 1 : L0, ok = p.x0 >= lo && p.x1 < B && p.y0 > L0 && p.y1 < B - 1 + (gen < 7) && p.x0 < p.x1 && p.y0 < p.y1 && p.dx > p.x0 && p.dx < (p.type == HT_TERRACE ? p.x1 + 1 : p.x1) && (p.dy == p.y0 || p.dy == p.y1);
                if (p.wx1 && !(p.wx0 >= lo && p.wx1 < B && p.wy0 > L0 && p.wy1 < B - 1 && (p.wx0 == p.x1 || p.wx1 == p.x0))) ok = 0;
                if (p.drv && (p.drv < lo || p.drv + 1 >= B || p.dx == p.drv || p.dx == p.drv + 1)) ok = 0;
                if (gen >= 7 && p.dy != (p.face == LF_NORTH ? p.y0 : p.y1)) ok = 0;
                if (gen >= 7 && !ok) { bad_plan++; if (getenv("VERBOSE")) printf("  bad plan: %s at %d,%d type %d body %d,%d-%d,%d wing %d,%d-%d,%d door %d,%d drv %d road %d\n", bm_lot_name(b.lot), bx, by, p.type, p.x0, p.y0, p.x1, p.y1, p.wx0, p.wy0, p.wx1, p.wy1, p.dx, p.dy, p.drv, b.road); }
                BInfo e; bm_block(bx + 1, by, &e);                             // the next lot along the street
                if (e.set == si && e.terrain != BT_SEA && is_home(e.lot)) { plan_of(bx + 1, by, &e, &q);
                    if (q.face == p.face) { pairs++; if (q.type == p.type) { same_type++; if (q.roof == p.roof && q.mir == p.mir) same_look++; } } }
            }
        }
        const signed char (*lm)[2] = s->kind == ST_SPACEPORT ? lm_space : lm_city; int n = s->kind == ST_SPACEPORT ? 10 : s->kind == ST_CITY ? 26 : s->kind == ST_TOWN ? 12 : 5;
        for (int k = 0; k < n; k++) { int bx = s->x + lm[k][0], by = s->y + lm[k][1]; BInfo b, t; bm_block(bx, by, &b); if (b.set != si || b.terrain == BT_SEA || b.lot == LOT_RIVER) continue;
            bm_block(bx, by + 1, &t); lm_n++; if (!(roads(&t) & 1)) { lm_bad++; if (getenv("VERBOSE")) printf("  landmark with no street to its south: %s at %+d,%+d of %s\n", bm_lot_name(b.lot), lm[k][0], lm[k][1], s->name); } }
        if (!have[0] || !have[1] || !have[2] || !have[3]) { short_sets++; if (strlen(miss) < 440) sprintf(miss + strlen(miss), " %s(%s%s%s%s)", s->name, have[0] ? "" : "E", have[1] ? "" : "H", have[2] ? "" : "C", have[3] ? "" : "T"); }
    }
    int town_tiles = blocks * B * B;
    printf("world version %d%s: %d settlements, %d town blocks\n", gen, bm_packed ? " PACKED" : "", bm_nset(), blocks);
    printf("lots:"); for (int l = 0; l < LOT_N; l++) if (lots[l]) printf(" %s %d,", bm_lot_name(l), lots[l]); printf("\n");
    int homes = lots[LOT_HOUSE] + lots[LOT_COTTAGE] + lots[LOT_TOWN] + lots[LOT_TWO] + lots[LOT_APART];
    printf("home lots %d (house %d cottage %d two-storey %d terrace %d flats %d), empty lots for sale %d\n", homes, lots[LOT_HOUSE], lots[LOT_COTTAGE], lots[LOT_TWO], lots[LOT_TOWN], lots[LOT_APART], lots[LOT_EMPTY]);
    printf("home units %d (households: %d on 2 MB, %d on 8 MB)\n", units, CAP_FAM2, CAP_FAM8);
    printf("places %d of %d%s\n", places, CAP_PLACE, places > CAP_PLACE ? "  OVER THE CAP" : "");
    printf("far-view buildings %d of %d%s\n", farb, CAP_FAR, farb > CAP_FAR ? "  OVER THE CAP" : "");
    printf("street %.1f%%  pavement %.1f%%  buildings %.1f%%  paths and courts %.1f%%  water %.1f%%  green %.1f%%\n", 100.0 * tiles[K_ROAD] / town_tiles, 100.0 * tiles[K_PAVE] / town_tiles,
           100.0 * tiles[K_BLDG] / town_tiles, 100.0 * tiles[K_HARD] / town_tiles, 100.0 * tiles[K_WATER] / town_tiles, 100.0 * tiles[K_GREEN] / town_tiles);
    printf("lots with a door: facing south %d, north %d, landlocked %d\n", faces[LF_SOUTH], faces[LF_NORTH], landlocked);
    printf("landmarks %d, without a street to the south %d\n", lm_n, lm_bad);
    printf("settlements without one of empty lot / house / cottage / two-storey: %d%s\n", short_sets, miss);
    if (gen >= 7) {
        printf("neighbourhood blocks:"); for (int k = 1; k < 8; k++) printf(" %s %d,", nkName[k], kinds[k]); printf("\n");
        printf("houses: compact %d ranch %d L %d box %d two-storey %d terrace %d mansion %d\n", types[HT_COMPACT], types[HT_RANCH], types[HT_L], types[HT_BOX], types[HT_TWO], types[HT_TERRACE], types[HT_MANSION]);
        printf("neighbours on a street: %d pairs, the same type %d, the same type and roof and side %d; plans out of their lot %d\n", pairs, same_type, same_look, bad_plan);
    } else printf("houses: cottage %d house %d mansion %d two-storey %d terrace %d; neighbours on a street: %d pairs, the same type %d\n", types[HT_COMPACT], types[HT_BOX], types[HT_MANSION], types[HT_TWO], types[HT_TERRACE], pairs, same_type);
    return 0;
}
static int do_sum(void) {   // every block of every town (and round it), and a grid of the whole map
    unsigned h = 2166136261u; int n = 0;
    #define MIX(v) (h = (h ^ (unsigned)(v)) * 16777619u)
    #define BLK(x, y) do { BInfo b; bm_block(x, y, &b); MIX(b.terrain); MIX(b.lot); MIX(b.set); MIX(b.road); MIX(b.h); MIX(bm_units(&b)); MIX(bm_storeys(&b)); n++; } while (0)
    for (int si = 0; si < bm_nset(); si++) { const Settle *s = bm_set(si); MIX(s->x); MIX(s->y); MIX(s->r); MIX(s->kind); for (const char *c = s->name; *c; c++) MIX(*c);
        for (int by = s->y - s->r - 4; by <= s->y + s->r + 4; by++) for (int bx = s->x - s->r - 4; bx <= s->x + s->r + 4; bx++) BLK(bx, by); }
    for (int by = 0; by < BM_N; by += 5) for (int bx = 0; bx < BM_N; bx += 7) BLK(bx, by);
    for (int i = 0; i < bm_nseg(); i++) { int c, a, b, v; bm_seg(i, &c, &a, &b, &v); MIX(c); MIX(a); MIX(b); MIX(v); }
    printf("%08x %d blocks\n", h, n);
    return 0;
}
static int do_list(void) {   // every block of every settlement: set, bx, by, neighbourhood kind, lot, house type/roof/pal, face (for picking shots)
    for (int si = 0; si < bm_nset(); si++) { const Settle *s = bm_set(si);
        for (int by = s->y - s->r; by <= s->y + s->r; by++) for (int bx = s->x - s->r; bx <= s->x + s->r; bx++) {
            BInfo b; bm_block(bx, by, &b); if (b.set != si || b.terrain == BT_SEA) continue;
            LotPlan p; memset(&p, 0, sizeof p); if (is_home(b.lot)) plan_of(bx, by, &b, &p);
            printf("B %d %d %d nb %d lot %s type %d roof %d pal %d face %d road %d\n", si, bx, by, bm_nb(bx, by), bm_lot_name(b.lot), p.type, p.roof, p.pal, p.face, b.road); } }
    return 0;
}
int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: townmap pic SEED GEN WHICH OUT.ppm | stats SEED GEN [packed] | sum SEED GEN [packed]\n"); return 1; }
    unsigned seed = (unsigned)strtoul(argv[2], 0, 0); gen = atoi(argv[3]);
    bm_dials_default(); bm_gen = gen;
    if (!strcmp(argv[1], "pic")) { if (argc < 6) return 1; bm_new(seed); return do_pic(argv[4], argv[5], argc > 6 ? atoi(argv[6]) : 4); }
    bm_packed = argc > 4 && !strcmp(argv[4], "packed");
    if (argc > 5) { bm_capsz = atoi(argv[5]); if (argc > 8) { bm_ncity = atoi(argv[6]); bm_ntown = atoi(argv[7]); bm_nvil = atoi(argv[8]); } }   // (the dials: capital size, cities, towns, villages)
    bm_new(seed);
    if (!strcmp(argv[1], "list")) return do_list();
    return !strcmp(argv[1], "stats") ? do_stats() : do_sum();
}
