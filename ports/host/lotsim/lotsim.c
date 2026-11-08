// lotsim: the lot generators of src/world.c (houses, yards, shops, parks ...) run on a normal computer, for design work and the lot audit.
//   make -C ports/host/lotsim            builds ./lotsim (new generators, src/lots.inc) and ./lotsim_old (the generators as they were before the lots round)
//   ./lotsim dump SEED KIND INDEX        one lot as text (floors as colours, items as letters) and the audit's verdict on it
//   ./lotsim png SEED OUT.png [KIND]     a sheet of lots
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
    for (int y = ly - 1; y <= ly + 13; y++) { for (int x = lx - 1; x <= lx + 13; x++) putchar(au_reach[y][x] ? 'r' : au_in[y][x] ? 'i' : '.'); putchar('\n'); }
}
int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: lotsim dump|png|audit SEED ...\n"); return 1; }
    unsigned seed = (unsigned)strtoul(argv[2], 0, 0); bm_gen = 6; bm_dials_default(); bm_new(seed); wsalt = seed;
    if (!strcmp(argv[1], "dump")) {
        int kind = argc > 3 ? atoi(argv[3]) : LOT_HOUSE, idx = argc > 4 ? atoi(argv[4]) : 0, n = 0;
        for (int by = 0; by < BM_N; by++) for (int bx = 0; bx < BM_N; bx++) {
            BInfo b; bm_block(bx, by, &b); if (b.set == 255 || b.terrain == BT_SEA || b.lot != kind) continue;
            if (n++ < idx) continue;
            int lot = make_block(bx, by, 0); lot_audit(BM_BLK + BM_LOT0, BM_BLK + BM_LOT0, lot); dump(lot, bx, by); if (getenv("REACH")) dump_reach(); audit_report(stdout, 1); return 0;
        }
        return 1;
    }
    if (!strcmp(argv[1], "audit")) {   // every block of every settlement (or the first N of each kind)
        int cap = argc > 3 ? atoi(argv[3]) : 0; static unsigned char done[BM_N][BM_N]; static int per[LOT_N];
        for (int i = 0; i < bm_nset(); i++) { const Settle *s = bm_set(i); int r = s->r + 1;
            for (int by = s->y - r; by <= s->y + r; by++) for (int bx = s->x - r; bx <= s->x + r; bx++) {
                if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N || done[by][bx]) continue; done[by][bx] = 1;
                BInfo b; bm_block(bx, by, &b); if (b.set == 255 || b.terrain == BT_SEA || b.lot == LOT_WILD || b.lot == LOT_RIVER) continue;
                if (cap && per[b.lot]++ >= cap) continue;
                int lot = make_block(bx, by, 0); lot_audit(BM_BLK + BM_LOT0, BM_BLK + BM_LOT0, lot); } }
        audit_report(stdout, 0); return 0;
    }
    return 1;
}
