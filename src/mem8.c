// mem8: see mem8.h
#include "mem8.h"

int ram8;
static u8 *x_top, *x_end;
static struct { u8 *p; u32 n; } xr[6]; static int nxr;   // blocks given back with xrelease, used first
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
void xrelease(void *p, u32 n) {
    if (!x_top || nxr >= 6) return;
    u32 a = ((u32)p + 7) & ~7u, e = ((u32)p + n) & ~7u;
    if (e > a) { xr[nxr].p = (u8 *)a; xr[nxr].n = e - a; nxr++; }
}
void *xalloc(u32 n) {
    if (!x_top) return 0;
    n = (n + 7) & ~7u;
    for (int i = 0; i < nxr; i++) if (xr[i].n >= n) { void *q = xr[i].p; xr[i].p += n; xr[i].n -= n; memset(q, 0, n); return q; }
    if (x_top + n > x_end) return 0;
    void *p = x_top; x_top += n;
    memset(p, 0, n);
    return p;
}
u32 xfree_bytes(void) { if (!x_top) return 0; u32 r = (u32)(x_end - x_top); for (int i = 0; i < nxr; i++) r += xr[i].n; return r; }

__attribute__((optimize("no-tree-loop-distribute-patterns")))
void wmove(void *dst, const void *src, u32 n) {
    u8 *d = (u8 *)dst; const u8 *s = (const u8 *)src;
    if (d == s || !n) return;
    if (d < s || d >= s + n) {                                    // forwards
        if ((((u32)d ^ (u32)s) & 3) == 0) {
            while (n && ((u32)d & 3)) { *d++ = *s++; n--; }
            u32 *dw = (u32 *)d; const u32 *sw = (const u32 *)s;
            while (n >= 16) { u32 a = sw[0], b = sw[1], c = sw[2], e = sw[3]; dw[0] = a; dw[1] = b; dw[2] = c; dw[3] = e; dw += 4; sw += 4; n -= 16; }
            while (n >= 4) { *dw++ = *sw++; n -= 4; }
            d = (u8 *)dw; s = (const u8 *)sw;
        }
        while (n--) *d++ = *s++;
    } else {                                                      // backwards (the copy overlaps, ahead of the source)
        d += n; s += n;
        if ((((u32)d ^ (u32)s) & 3) == 0) {
            while (n && ((u32)d & 3)) { *--d = *--s; n--; }
            u32 *dw = (u32 *)d; const u32 *sw = (const u32 *)s;
            while (n >= 16) { dw -= 4; sw -= 4; u32 a = sw[0], b = sw[1], c = sw[2], e = sw[3]; dw[0] = a; dw[1] = b; dw[2] = c; dw[3] = e; n -= 16; }
            while (n >= 4) { *--dw = *--sw; n -= 4; }
            d = (u8 *)dw; s = (const u8 *)sw;
        }
        while (n--) *--d = *--s;
    }
}
// every memcpy and memmove in the game (and gcc's own, for struct copies) comes here instead (the link wraps them)
void *__wrap_memcpy(void *d, const void *s, u32 n) { wmove(d, s, n); return d; }
void *__wrap_memmove(void *d, const void *s, u32 n) { wmove(d, s, n); return d; }
