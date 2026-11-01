// dc_prof.c: a sampling profiler for test builds (make PROF=1): timer 1 interrupts 2000 times a second and notes where the CPU
// was; between two frames the busiest places go into the log (test/profsym.py turns the addresses into function names).
#include <kos.h>
#include <arch/timer.h>
#include <arch/irq.h>
#include <string.h>
#ifdef DC_PROF
extern void dc_logf(const char *f, ...);
#define NS 65536
static uint32_t smp[NS]; static volatile int ns, on;
static void prof_irq(irq_t src, irq_context_t *cx, void *d) { (void)src; (void)d; timer_clear(TMU1); if (on && ns < NS) smp[ns++] = cx->pc; }
static int cmp(const void *a, const void *b) { uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b; return x < y ? -1 : x > y; }
void dc_prof_frame(int frame) {
    if (frame == DC_PROF) { irq_set_handler(EXC_TMU1_TUNI1, prof_irq, 0); timer_prime(TMU1, 2000, 1); timer_start(TMU1); on = 1; }
    if (frame == DC_PROF + 600 && on) {
        on = 0; timer_stop(TMU1);
        int n = ns;
        for (int i = 0; i < n; i++) smp[i] &= ~31u;      // (32-byte buckets)
        qsort(smp, n, 4, cmp);
        static uint32_t top_a[240]; static int top_c[240]; int nt = 0;
        for (int i = 0; i < n;) { int j = i; while (j < n && smp[j] == smp[i]) j++;
            int c = j - i, k = nt < 240 ? nt++ : 239;
            if (k == 239 && c <= top_c[239]) { i = j; continue; }
            top_a[k] = smp[i]; top_c[k] = c;
            while (k > 0 && top_c[k] > top_c[k - 1]) { uint32_t ta = top_a[k]; int tc = top_c[k]; top_a[k] = top_a[k - 1]; top_c[k] = top_c[k - 1]; top_a[k - 1] = ta; top_c[k - 1] = tc; k--; }
            i = j; }
        dc_logf("PROF %d samples\n", n);
        for (int i = 0; i < nt; i++) dc_logf("P %08lx %d\n", (unsigned long)top_a[i], top_c[i]);
    }
}
#else
void dc_prof_frame(int frame) { (void)frame; }
#endif
