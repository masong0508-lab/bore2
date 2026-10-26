// BORE 2 host test: the platform-free world generator running on a normal computer (step 1 of the DS/3DS port).
// Build: see ports/host/Makefile. It proves bigmap.c (the world) needs nothing from the PS1 and prints a map.
#include <stdio.h>
#include "bigmap.h"
int bsprintf(char *o, const char *f, ...) { (void)o; (void)f; return 0; }
int bsnprintf(char *o, unsigned cap, const char *f, ...) { (void)o; (void)cap; (void)f; return 0; }
void boot(const char *s) { (void)s; }
void loading(const char *w, int p) { (void)w; (void)p; }
int main(int argc, char **argv) {
    unsigned seed = argc > 1 ? (unsigned)atoi(argv[1]) : 1234;
    bm_dials_default(); bm_new(seed);
    printf("seed %u: %d settlements\n", seed, bm_nset());
    for (int i = 0; i < bm_nset(); i++) { const Settle *s = bm_set(i); printf("  %-18s kind %d at block %d,%d radius %d\n", s->name, s->kind, s->x, s->y, s->r); }
    for (int by = 0; by < BM_N; by += 16) { for (int bx = 0; bx < BM_N; bx += 8) { BInfo b; bm_block(bx, by, &b); putchar(b.terrain == BT_SEA ? '~' : b.set != 255 ? '#' : b.terrain == BT_FOREST ? 'f' : b.terrain == BT_ROCKY ? '^' : '.'); } putchar('\n'); }
    return 0;
}
