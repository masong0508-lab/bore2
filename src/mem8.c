// mem8: see mem8.h
#include "mem8.h"

int ram8;
static u8 *x_top, *x_end;
static volatile u32 probe;

void mem8_init(void) {
    // With 2 MB the RAM shows up four times over in the first 8 MB of the address space: a word written 2 MB above
    // another lands on top of it. With 8 MB it is a different word.
    volatile u32 *lo = &probe, *hi = (volatile u32 *)((u32)&probe + 0x200000u);
    u32 keep = *lo;
    *lo = 0x12345678u; *hi = 0x9ABCDEF0u;
    ram8 = *lo == 0x12345678u;
    *lo = keep;
    if (ram8) { x_top = (u8 *)0x80200000u; x_end = (u8 *)0x80800000u; }   // (the stack stays at the top of the first 2 MB)
    else x_top = x_end = 0;
}
void *xalloc(u32 n) {
    if (!x_top) return 0;
    n = (n + 7) & ~7u;
    if (x_top + n > x_end) return 0;
    void *p = x_top; x_top += n;
    memset(p, 0, n);
    return p;
}
u32 xfree_bytes(void) { return x_top ? (u32)(x_end - x_top) : 0; }
