// Stand-ins for the PS1 services the shared game code calls (boot messages, the fast printf, ...).
#include <stdio.h>
#include <stdarg.h>
int bsprintf(char *o, const char *f, ...) { va_list a; va_start(a, f); int n = vsprintf(o, f, a); va_end(a); return n; }
int bsnprintf(char *o, unsigned cap, const char *f, ...) { va_list a; va_start(a, f); int n = vsnprintf(o, cap, f, a); va_end(a); return n; }
void boot(const char *s) { printf("boot: %s\n", s); }
void loading(const char *w, int p) { (void)w; (void)p; }
int dbg_miss[4];                 // (the PS1 build's profiler counters bigmap.c bumps)
void world_gh_reset(void) {}     // (world.c's height cache: the Dreamcast layer keeps its own)
