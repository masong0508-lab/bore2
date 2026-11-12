// lotsim: the lot generators of src/world.c (houses, yards, shops, parks ...) run on a normal computer, for design work and the lot audit.
//   make -C ports/host/lotsim            builds ./lotsim (new generators, src/lots.inc) and ./lotsim_old (the generators as they were before the lots round)
//   ./lotsim dump SEED KIND INDEX        one lot as text (floors as colours, items as letters) and the audit's verdict on it
//   ./lotsim png SEED OUT.png [KIND]     a sheet of lots
//   ./lotsim rooms SEED BX BY            one lot, every storey, as data for rooms.py (python3 rooms.py OUT.png: a sheet of homes drawn top-down, to judge rooms by eye)
//   ./lotsim find SEED KIND              where the blocks of a kind are
//   ./lotsim audit SEED [N]              N blocks of every kind (default all of a whole world's towns), the audit's totals
// The environment it gives the generators is the one world.c does (the three map arrays, the storeys, the homes), without the 3D.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "bigmap.h"
#include "world.h"
#include "tex.h"
#define NWX 89
#define NWPNEW 6

#include <stdarg.h>
#undef sprintf
#undef snprintf
int bsprintf(char *o, const char *f, ...) { va_list a; va_start(a, f); int n = vsprintf(o, f, a); va_end(a); return n; }
int bsnprintf(char *o, unsigned cap, const char *f, ...) { va_list a; va_start(a, f); int n = vsnprintf(o, cap, f, a); va_end(a); return n; }
void boot(const char *s) { (void)s; }
void loading(const char *w, int p) { (void)w; (void)p; }
int ram8 = 0, dbg_miss[4];
void *xalloc(size_t n) { return calloc(1, n); }
void world_gh_reset(void) {}
char lifeMap[MH][MW + 1]; u8 floorMap[MH][MW], wallMap[MH][MW];
int world_zone = ZONE_CITY, roof_dirty, ao_dirty, world_wbx, world_wby;
typedef struct { char map[MH][MW + 1]; u8 fl[MH][MW]; u8 wp[MH][MW]; } Level;
static Level lvs[NUP]; static int lvon[NUP];
static Level *lv_need(int k) { if (k < 0 || k >= NUP) return 0; if (!lvon[k]) { lvon[k] = 1; for (int y = 0; y < MH; y++) { memset(lvs[k].map[y], ' ', MW); lvs[k].map[y][MW] = 0; } memset(lvs[k].fl, 0, sizeof lvs[k].fl); memset(lvs[k].wp, 0, sizeof lvs[k].wp); } return &lvs[k]; }
static Level *lv_get(int k) { return k >= 0 && k < NUP && lvon[k] ? &lvs[k] : 0; }
static Home homes[ZONE_N][MAXHOMES]; static int nhomes[ZONE_N];
static int blk_bx = 0xFFFF, blk_by = 0xFFFF, blk_unit;
static int reg_home(int x0, int y0, int x1, int y1, int dx, int dy) {
    int z = world_zone % ZONE_N, u = blk_unit++;
    if (nhomes[z] >= MAXHOMES) return u;
    Home *h = &homes[z][nhomes[z]++]; h->x0 = (u8)x0; h->y0 = (u8)y0; h->x1 = (u8)x1; h->y1 = (u8)y1; h->dx = (u8)dx; h->dy = (u8)dy;
    h->bx = (u16)blk_bx; h->by = (u16)blk_by; h->unit = (u8)u;
    return u;
}
static u32 wsalt;
#include "items_tab.h"
static int itf(int c) { for (unsigned i = 0; i < sizeof itab / sizeof itab[0]; i++) if (itab[i].c == (unsigned char)c) return itab[i].f; return -1; }
int items_is_item(char c) { return itf(c) >= 0; }
int items_blocking(char c) { int f = itf(c); return f >= 0 && !(f & (4 | 64)); }
int items_rotates(char c) { int f = itf(c); return f >= 0 && (f & 1); }
int items_foot(char c, int *w, int *l) { int f = itf(c); *w = *l = 1; if (f < 0) return 0; if (f & 32) *w = 2; if (f & 16) *l = 2; return *w * *l > 1; }
const char *items_name(char c) { for (unsigned i = 0; i < sizeof itab / sizeof itab[0]; i++) if (itab[i].c == (unsigned char)c) return itab[i].n; return "?"; }
static int is_wall(char c) { return c == 'W' || c == 'w' || c == 'E' || c == 'e' || c == 'f'; }

#include "prelude.inc"
#ifdef LOTSIM_OLD
#include "old_gen.inc"
#else
#include "lots.inc"
#endif

// ---- a block, as gen_block0 makes it (without the streets) ----
static int cur_bx, cur_by;
static void synth_occ(unsigned h) {   // a household for each unit, from the block's dice (what pop_family_at gives in the game)
    for (int u = 0; u < 12; u++) {
        Occ *o = &occ_blk[u]; memset(o, 0, sizeof *o); unsigned r = city_h((int)h + u * 13, u + 77);
        if (getenv("OCC")) { int a = 1, k = 0, t = 0, e = 0, c = 0, b = 0; sscanf(getenv("OCC"), "%d,%d,%d,%d,%d,%d", &a, &k, &t, &e, &c, &b);   // (the picture tool picks the household: adults,kids,teens,elders,couple,babies)
            o->adults = (u8)a; o->kids = (u8)k; o->teens = (u8)t; o->elders = (u8)e; o->couple = (u8)c; o->babies = (u8)b; continue; }
        if ((r & 15) == 0) { o->vacant = 1; continue; }
        o->adults = 1 + ((r >> 4) & 1); o->couple = o->adults == 2 && ((r >> 5) & 3) != 0; o->kids = ((r >> 7) & 3) == 0 ? 2 : ((r >> 7) & 3) == 1 ? 1 : 0; o->teens = ((r >> 9) & 7) == 0; o->elders = ((r >> 12) & 7) == 0; o->babies = ((r >> 15) & 7) == 0;
        if (!o->kids && !o->teens && !o->babies && !o->elders && o->adults == 1) o->couple = 0;
    }
    occ_on = 1;
}
void lot_make(int lot, int lx, int ly, unsigned h, int st) {
    switch (lot) {
        case LOT_HOUSE:   city_house(lx, ly, h); break;
        case LOT_COTTAGE: city_cottage(lx, ly, h); break;
        case LOT_TOWN:    city_town(lx, ly, h); break;
        case LOT_TWO:     city_two(lx, ly, h); break;
        case LOT_SHOP:    lot_shop(lx, ly, h); break;
        case LOT_DINER:   city_diner(lx, ly, h); break;
        case LOT_GYM:     city_gym(lx, ly, h); break;
        case LOT_OFFICE:  city_office(lx, ly, h); break;
        case LOT_INN:     lot_inn(lx, ly, h); break;
        case LOT_POOL:    lot_pool(lx, ly, h); break;
        case LOT_SCHOOL:  lot_school(lx, ly, h); break;
        case LOT_ARCADE:  lot_arcade(lx, ly, h); break;
        case LOT_PARK:    lot_park(lx, ly, h); break;
        case LOT_SQUARE:  city_square(lx, ly); break;
        case LOT_SKATE:   city_skate(lx, ly); break;
        case LOT_EMPTY:   lot_empty(lx, ly, h); break;
        case LOT_FARM:    lot_farm(lx, ly, h); break;
        case LOT_APART:   lot_apart(lx, ly, h, st); break;
        case LOT_TOWER:   lot_tower(lx, ly, h, st); break;
        case LOT_HOTEL:   lot_hotel(lx, ly, h, st); break;
        case LOT_CAFE:    lot_cafe(lx, ly, h); break;
        case LOT_BAR:     lot_bar(lx, ly, h); break;
        case LOT_CINEMA:  lot_cinema(lx, ly, h); break;
        case LOT_SALON:   lot_salon(lx, ly, h); break;
        case LOT_RESTAURANT: lot_restaurant(lx, ly, h); break;
        case LOT_CLINIC:  lot_clinic(lx, ly, h); break;
        case LOT_LIBRARY: lot_library(lx, ly, h); break;
        case LOT_SPA:     lot_spa(lx, ly, h); break;
        case LOT_GALLERY: lot_gallery(lx, ly, h); break;
        case LOT_GAS:     lot_gas(lx, ly, h); break;
        case LOT_CONV:    lot_conv(lx, ly, h); break;
        case LOT_SPACE:   lot_space(lx, ly, h); break;
        case LOT_OBS:     lot_obs(lx, ly, h); break;
        default: break;
    }
}
static int make_block(int bx, int by, BInfo *bo) {   // block (bx,by) of the world into the map's block (1,1): its lot at (BM_BLK + BM_LOT0, ...)
    BInfo b; bm_block(bx, by, &b); if (bo) *bo = b;
    for (int y = 0; y < MH; y++) { for (int x = 0; x < MW; x++) { lifeMap[y][x] = '.'; floorMap[y][x] = FL_GRASS; wallMap[y][x] = 0; } lifeMap[y][MW] = 0; }
    for (int k = 0; k < NUP; k++) lvon[k] = 0;
    nhomes[ZONE_CITY] = 0; blk_bx = bx; blk_by = by; blk_unit = 0; cur_bx = bx; cur_by = by;
    TREE = (b.terrain == BT_FOREST || b.terrain == BT_ROCKY) ? '6' : b.terrain == BT_DESERT ? 'u' : b.terrain == BT_BEACH ? 'g' : '3';
    synth_occ(b.h);
    int ox = BM_BLK, oy = BM_BLK, lx = ox + BM_LOT0, ly = oy + BM_LOT0;
    int st = bm_storeys(&b);
    lot_make(b.lot, lx, ly, b.h, st);
    occ_on = 0;
    return b.lot;
}

// ---- the audit (src/lot_audit.inc) ----
#include "../../../src/lot_audit.inc"

// ---- text and picture output ----
static const char *lotname(int l) { return bm_lot_name(l); }
static void dump_reach(void);
static void dump(int lot, int bx, int by) {
    printf("block %d,%d  %s\n", bx, by, lotname(lot));
    int lx = BM_BLK + BM_LOT0, ly = BM_BLK + BM_LOT0;
    for (int y = ly - 1; y <= ly + 13; y++) {
        for (int x = lx - 1; x <= lx + 13; x++) {
            char c = lifeMap[y][x]; int f = floorMap[y][x];
            if (c == '.') putchar(f == FL_GRASS ? ',' : f == FL_PAVING ? '_' : f == FL_PATIO ? ':' : f == FL_WATER ? '~' : f == FL_SOIL ? ';' : f == FL_GRAVEL ? '\'' : f < NFL ? "0123456789abcd"[f] : '?');
            else if ((unsigned char)c >= 0x80) putchar('@' + (((unsigned char)c) & 31) / 2); else putchar(c);
        }
        putchar('\n');
    }
}

static void dump_reach(void) {
    int lx = BM_BLK + BM_LOT0, ly = BM_BLK + BM_LOT0;
    for (int y = ly - 1; y <= ly + 13; y++) { for (int x = lx - 1; x <= lx + 13; x++) putchar(au_reach[y][x] ? 'r' : lifeMap[y][x] == '.' ? 'u' : au_in[y][x] ? 'i' : '.'); putchar('\n'); }
}
// ---- the picture tool's data: one lot, every storey, every tile (what is on it, its floor, which way a turned thing faces) for rooms.py ----
static int lot_facing(char (*m)[MW + 1], u8 (*wp)[MW], int x, int y) {   // world.c's world_item_facing, on a storey's arrays
    static const signed char dx[4] = { 0, 1, 0, -1 }, dy[4] = { 1, 0, -1, 0 };
    #define AT(a, b) ((a) < 0 || (b) < 0 || (a) >= MW || (b) >= MH ? 'W' : m[b][a])
    #define OPEN(a, b) (AT(a, b) == '.' || AT(a, b) == 'D' || AT(a, b) == 'B' || AT(a, b) == 'P' || AT(a, b) == 'r')
    char c = m[y][x];
    if (!items_rotates(c)) return -1;
    if (wp[y][x] & 0x80) return wp[y][x] & 3;
    if (c == 'h' || c == ';') for (int d = 0; d < 4; d++) { char n = AT(x + dx[d], y + dy[d]); if (n == 't' || n == 'd' || n == 'y' || n == '`' || n == '[' || n == '?') return d; }
    if (c == '[' || c == 'd' || c == '`') for (int d = 0; d < 4; d++) if (AT(x + dx[d], y + dy[d]) == 'h') return d;
    for (int d = 0; d < 4; d++) if (OPEN(x + dx[d], y + dy[d]) && !OPEN(x + dx[(d + 2) & 3], y + dy[(d + 2) & 3])) return d;
    for (int d = 0; d < 4; d++) if (OPEN(x + dx[d], y + dy[d])) return d;
    return 0;
}
static void rooms_dump(int lot, int bx, int by) {
    int lx = BM_BLK + BM_LOT0, ly = BM_BLK + BM_LOT0;
    for (int l = 0; l < NUP; l++) {
        char (*m)[MW + 1] = l ? (lv_get(l - 1) ? lv_get(l - 1)->map : 0) : lifeMap; if (!m) continue;
        u8 (*fl)[MW] = l ? lv_get(l - 1)->fl : floorMap, (*wp)[MW] = l ? lv_get(l - 1)->wp : wallMap;
        int any = !l; for (int y = ly; y <= ly + 12 && !any; y++) for (int x = lx; x <= lx + 12; x++) if (m[y][x] != ' ') { any = 1; break; }
        if (!any) continue;
        printf("LEVEL %d %d %d %s\n", l, bx, by, lotname(lot));
        for (int y = ly - 1; y <= ly + 13; y++) { for (int x = lx - 1; x <= lx + 13; x++) { unsigned char c = (unsigned char)m[y][x];
            printf("%d:%d:%d:%d:%s|", c, fl[y][x], items_is_item((char)c) ? lot_facing(m, wp, x, y) : -1, items_is_item((char)c) ? (items_blocking((char)c) ? 1 : 2) : 0, items_is_item((char)c) ? items_name((char)c) : ""); }
            putchar('\n'); }
    }
    for (int u = 0; u < blk_unit && u < 12; u++) { const Occ *o = &occ_blk[u]; printf("OCC %d adults %d kids %d teens %d elders %d couple %d babies %d vacant %d\n", u, o->adults, o->kids, o->teens, o->elders, o->couple, o->babies, o->vacant); }
}
int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: lotsim dump|png|audit SEED ...\n"); return 1; }
    unsigned seed = (unsigned)strtoul(argv[2], 0, 0); bm_gen = 6; bm_dials_default(); bm_new(seed); wsalt = seed;
    if (!strcmp(argv[1], "census")) {   // lotsim census SEED: what the world's homes hold, thing by thing (so a change to the furnishing can be checked against what the sims use)
        static long cnt[256]; long homesN = 0, want[4] = { 0, 0, 0, 0 }, lack[8] = { 0 }; static unsigned char done[BM_N][BM_N];
        for (int i = 0; i < bm_nset(); i++) { const Settle *s = bm_set(i); int r = s->r + 1;
            for (int by = s->y - r; by <= s->y + r; by++) for (int bx = s->x - r; bx <= s->x + r; bx++) {
                if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N || done[by][bx]) continue; done[by][bx] = 1;
                BInfo b; bm_block(bx, by, &b); if (b.set == 255 || b.terrain == BT_SEA || b.lot < LOT_HOUSE || b.lot > LOT_APART) continue;
                make_block(bx, by, 0);
                for (int h = 0; h < nhomes[ZONE_CITY]; h++) { const Home *hm = &homes[ZONE_CITY][h]; if (occ_blk[hm->unit].vacant) continue; homesN++;
                    { Beds bd = beds_of(&occ_blk[hm->unit]); want[0] += bd.abeds + bd.tbeds; want[1] += bd.bunks; want[2] += bd.cribs; want[3] += occ_blk[hm->unit].adults + occ_blk[hm->unit].elders + occ_blk[hm->unit].kids + occ_blk[hm->unit].teens + occ_blk[hm->unit].babies; }
                    long was[256]; memcpy(was, cnt, sizeof was);
                    for (int l = 0; l < NUP; l++) { char (*m)[MW + 1] = l ? (lv_get(l - 1) ? lv_get(l - 1)->map : 0) : lifeMap; if (!m) continue;
                        if (b.lot == LOT_APART && l != hm->unit / 2) continue;       // (a flat is on its own storey)
                        for (int y = hm->y0; y <= hm->y1; y++) for (int x = hm->x0; x <= hm->x1; x++) { unsigned char c = (unsigned char)m[y][x]; if (c != '.' && c != ' ' && c != 'W' && c != 'E' && c != 'D') cnt[c]++; } }
                    #define HAS(ch) (cnt[(unsigned char)(ch)] > was[(unsigned char)(ch)])
                    { int miss[8] = { !HAS('F'), !HAS('T'), !(HAS('S') || HAS('<')), !HAS('H'), !(HAS('C') || HAS('U') || HAS('h')), !(HAS('v') || HAS('R') || HAS('A') || HAS('b') || HAS('q')), !HAS('t'), !HAS('/') };
                      for (int q = 0; q < 8; q++) lack[q] += miss[q];
                      if (getenv("LACK")) for (int q = 0; q < 8; q++) if (miss[q] && ((atoi(getenv("LACK")) >> q) & 1)) printf("lack %d %s %d %d unit %d\n", q, lotname(b.lot), bx, by, hm->unit); } } } }
        printf("homes lived in %ld\n", homesN);
        printf("# people %ld; beds wanted (beds_of): beds %ld, bunks %ld, cribs %ld\n", want[3], want[0], want[1], want[2]);
        printf("# homes without: a fridge %ld, a toilet %ld, a bed %ld, a shower %ld, a seat (sofa, beanbag, chair) %ld, fun (telly, radio, hifi, books, aquarium) %ld, a table %ld, a stove %ld\n", lack[0], lack[1], lack[2], lack[3], lack[4], lack[5], lack[6], lack[7]);
        for (int c = 0; c < 256; c++) if (cnt[c]) printf("%-18s %3d %6ld  %.2f a home\n", items_name((char)c), c, cnt[c], (double)cnt[c] / homesN);
        return 0;
    }
    if (!strcmp(argv[1], "at")) {       // lotsim at SEED BX BY: that block as text, storey by storey, each beside what the audit could walk to (r), could not (u: floor, i: indoors), and its verdict
        int bx = atoi(argv[3]), by = atoi(argv[4]); int lot = make_block(bx, by, 0); int lx = BM_BLK + BM_LOT0, ly = BM_BLK + BM_LOT0;
        lot_audit(lx, ly, lot); printf("block %d,%d  %s\n", bx, by, lotname(lot)); audit_line(stdout, "this lot", &lot_aud);
        for (int l = 0; l < NUP; l++) { char (*m)[MW + 1] = l ? (lv_get(l - 1) ? lv_get(l - 1)->map : 0) : lifeMap; if (!m) continue;
            u8 (*fl)[MW] = l ? lv_get(l - 1)->fl : floorMap, (*wp)[MW] = l ? lv_get(l - 1)->wp : wallMap;
            au_level(l, m, fl, wp, l == 0 ? 0 : l == 1 ? lifeMap : lv_get(l - 2)->map, lx, ly, lx + 12, ly + 12, lot);
            printf("storey %d\n", l);
            for (int y = ly - 1; y <= ly + 13; y++) {
                for (int x = lx - 1; x <= lx + 13; x++) { char c = m[y][x]; int f = fl[y][x];
                    if (c == '.') putchar(f == FL_GRASS ? ',' : f < NFL ? '_' : ':'); else if ((unsigned char)c >= 0x80) putchar('@' + (((unsigned char)c) & 31) / 2); else putchar(c); }
                printf("   ");
                for (int x = lx - 1; x <= lx + 13; x++) putchar(au_reach[y][x] ? 'r' : m[y][x] == '.' ? 'u' : au_in[y][x] ? 'i' : '.');
                putchar('\n'); } }
        return 0;
    }
    if (!strcmp(argv[1], "dump")) {
        int kind = argc > 3 ? atoi(argv[3]) : LOT_HOUSE, idx = argc > 4 ? atoi(argv[4]) : 0, n = 0;
        for (int by = 0; by < BM_N; by++) for (int bx = 0; bx < BM_N; bx++) {
            BInfo b; bm_block(bx, by, &b); if (b.set == 255 || b.terrain == BT_SEA || b.lot != kind) continue;
            if (n++ < idx) continue;
            int lot = make_block(bx, by, 0); lot_audit(BM_BLK + BM_LOT0, BM_BLK + BM_LOT0, lot); dump(lot, bx, by); if (getenv("REACH")) dump_reach(); audit_report(stdout, 1); return 0;
        }
        return 1;
    }
    if (!strcmp(argv[1], "find")) {     // lotsim find SEED KIND: the blocks of that kind
        int kind = argc > 3 ? atoi(argv[3]) : LOT_HOUSE;
        for (int by = 0; by < BM_N; by++) for (int bx = 0; bx < BM_N; bx++) { BInfo b; bm_block(bx, by, &b); if (b.set == 255 || b.terrain == BT_SEA || b.lot != kind) continue;
            printf("%d %d h %08x grand %d storeys %d\n", bx, by, b.h, ((b.h >> 24) & 3) == 0, bm_storeys(&b)); }
        return 0;
    }
    if (!strcmp(argv[1], "rooms")) {    // lotsim rooms SEED BX BY: the lot of that block for rooms.py (OCC=adults,kids,teens,elders,couple,babies picks who lives there)
        int bx = atoi(argv[3]), by = atoi(argv[4]); int lot = make_block(bx, by, 0); occ_on = 0;
        lot_audit(BM_BLK + BM_LOT0, BM_BLK + BM_LOT0, lot); rooms_dump(lot, bx, by); audit_line(stdout, "AUDIT", &lot_aud); return 0;
    }
    if (!strcmp(argv[1], "audit")) {   // every block of every settlement (or the first N of each kind)
        int cap = argc > 3 ? atoi(argv[3]) : 0; static unsigned char done[BM_N][BM_N]; static int per[LOT_N];
        for (int i = 0; i < bm_nset(); i++) { const Settle *s = bm_set(i); int r = s->r + 1;
            for (int by = s->y - r; by <= s->y + r; by++) for (int bx = s->x - r; bx <= s->x + r; bx++) {
                if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N || done[by][bx]) continue; done[by][bx] = 1;
                BInfo b; bm_block(bx, by, &b); if (b.set == 255 || b.terrain == BT_SEA || b.lot == LOT_WILD || b.lot == LOT_RIVER) continue;
                if (cap && per[b.lot]++ >= cap) continue;
                LotAud was = lot_aud; int lot = make_block(bx, by, 0); lot_audit(BM_BLK + BM_LOT0, BM_BLK + BM_LOT0, lot);
                if (getenv("BAD") && lot <= LOT_APART && (lot_aud.door > was.door || lot_aud.lane > was.lane || lot_aud.floating > was.floating || lot_aud.overlap > was.overlap || lot_aud.unreach_item > was.unreach_item || lot_aud.unreach_floor > was.unreach_floor))   // (BAD=1: where the homes' faults are)
                    printf("bad %s %d %d: door %ld lane %ld floating %ld unreach floor %ld item %ld\n", lotname(lot), bx, by, lot_aud.door - was.door, lot_aud.lane - was.lane, lot_aud.floating - was.floating, lot_aud.unreach_floor - was.unreach_floor, lot_aud.unreach_item - was.unreach_item); } }
        audit_report(stdout, 0); return 0;
    }
    return 1;
}
