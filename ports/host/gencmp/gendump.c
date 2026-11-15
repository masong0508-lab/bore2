// gendump: the whole of one world, from src/bigmap.c alone, as hashes (and, with DUMP=1, as text for diff). Built against any commit's src/
// by test/gen_compare.sh, to prove worlds of older versions come out byte for byte the same after a change.
//   gendump SEED GEN [packed]
// Prints one line per part of the world: the settlements and highways, every block (terrain, lot, set, road, dice, homes, storeys, height, hill),
// every lot's plan (neighbourhood, face, house type, body, wing, door, drive, yard, mirror, roof, palette, wall, roof colour), its footprint,
// the street names of every home and the neighbourhood and place names. Only functions that exist in every version since 2 are used.
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
static unsigned long long HS[8]; static int dump;
static void mix(int k, unsigned long long v) { HS[k] = (HS[k] ^ v) * 1099511628211ull; }
static void mixs(int k, const char *s) { for (; *s; s++) mix(k, (unsigned char)*s); mix(k, 255); }
int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: gendump SEED GEN [packed]\n"); return 1; }
    unsigned seed = (unsigned)strtoul(argv[1], 0, 0); int gen = atoi(argv[2]); dump = getenv("DUMP") != 0;
    for (int i = 0; i < 8; i++) HS[i] = 14695981039346656037ull;
    bm_dials_default(); bm_gen = gen; bm_packed = argc > 3 && !strcmp(argv[3], "packed");
    bm_new(seed);
    int ns = bm_nset(); mix(0, ns);
    for (int i = 0; i < ns; i++) { const Settle *s = bm_set(i); mix(0, s->x); mix(0, s->y); mix(0, s->r); mix(0, s->kind); mixs(0, s->name); if (dump) printf("set %d %d,%d r%d k%d %s\n", i, s->x, s->y, s->r, s->kind, s->name); }
    for (int i = 0; i < bm_nseg(); i++) { int c, a, b, v; bm_seg(i, &c, &a, &b, &v); mix(0, c); mix(0, a); mix(0, b); mix(0, v); if (dump) printf("seg %d %d %d %d\n", c, a, b, v); }
    long nplan = 0, nhome = 0;
    for (int by = 0; by < BM_N; by++) for (int bx = 0; bx < BM_N; bx++) {
        BInfo b; bm_block(bx, by, &b);
        mix(1, b.terrain); mix(1, b.lot); mix(1, b.set); mix(1, b.road); mix(1, b.h); mix(1, bm_units(&b)); mix(1, bm_storeys(&b)); mix(1, bm_set_at(bx, by));
        mix(2, bm_height(bx, by)); mix(2, bm_hill(bx, by));
        if (dump) printf("blk %d,%d t%d lot%d set%d road%d h%08x u%d s%d\n", bx, by, b.terrain, b.lot, b.set, b.road, b.h, bm_units(&b), bm_storeys(&b));
        if (b.set == 255) continue;
        mix(3, bm_nb(bx, by)); mix(3, bm_lot_face(bx, by));
        LotPlan p; memset(&p, 0, sizeof p); bm_lot_plan(bx, by, &b, &p); nplan++;
        mix(3, p.face); mix(3, p.type); mix(3, p.x0); mix(3, p.y0); mix(3, p.x1); mix(3, p.y1); mix(3, p.wx0); mix(3, p.wy0); mix(3, p.wx1); mix(3, p.wy1);
        mix(3, p.dx); mix(3, p.dy); mix(3, p.d2x); mix(3, p.drv); mix(3, p.yard); mix(3, p.mir); mix(3, p.roof); mix(3, p.pal); mix(3, p.wall); mix(3, p.rcol);   // (roofs, palette, walls)
        if (dump) printf("plan %d,%d nb%d face%d type%d body %d,%d..%d,%d wing %d,%d..%d,%d door %d,%d d2 %d drv %d yard %d mir %d roof %d pal %d wall %d rc %d\n", bx, by, bm_nb(bx, by), p.face, p.type, p.x0, p.y0, p.x1, p.y1, p.wx0, p.wy0, p.wx1, p.wy1, p.dx, p.dy, p.d2x, p.drv, p.yard, p.mir, p.roof, p.pal, p.wall, p.rcol);
        u8 f[5] = { 0 }; LotPlan pp; memset(&pp, 0, sizeof pp); int fo = bm_lot_foot(bx, by, &b, f, &pp);
        mix(4, fo); for (int i = 0; i < 5; i++) mix(4, f[i]); mix(4, pp.face); mix(4, pp.type);
        LotPlan hp; memset(&hp, 0, sizeof hp); int ho = bm_lot_home(bx, by, &b, &hp); mix(4, ho); mix(4, hp.type); mix(4, hp.x0); mix(4, hp.x1); mix(4, hp.roof);
        if (b.terrain != BT_SEA) {
            char nm[64] = ""; if (bm_nb_name(bx, by, nm)) mixs(5, nm);
            for (int u = 0; u < bm_units(&b) && u < 12; u++) { char st[64] = ""; bm_street(b.set, bx, by, u, st); mixs(5, st); nhome++; if (dump) printf("street %d,%d u%d %s\n", bx, by, u, st); }
            if (bm_is_place(b.lot)) { char pn[64] = ""; bm_place_name(b.lot, b.h, pn); mixs(5, pn); }
        }
    }
    printf("seed %08x gen %d%s: sets %d plans %ld homes %ld\n", seed, gen, bm_packed ? " packed" : "", ns, nplan, nhome);
    const char *nm[6] = { "settle+highways", "blocks", "height+hill", "lot plans+roofs", "footprints", "streets+names" };
    for (int i = 0; i < 6; i++) printf("  %-16s %016llx\n", nm[i], HS[i]);
    return 0;
}
