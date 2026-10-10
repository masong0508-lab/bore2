// fmt: a small, quick sprintf for the HUD (the C library's vsnprintf was ~6% of a city frame). %d %i %u %x %X %s %c %%, with
// '-' / '0' flags, a width and 'l'. Everything that says sprintf in BORE 2 lands here (common.h).
#include <stdarg.h>
static int fmt_v(char *o, unsigned cap, const char *f, va_list ap) {
    unsigned n = 0;
    #define PUT(ch) do { if (n + 1 < cap) o[n] = (ch); n++; } while (0)
    while (*f) {
        char ch = *f++;
        if (ch != '%') { PUT(ch); continue; }
        int left = 0, zero = 0, width = 0;
        for (;; f++) { if (*f == '-') left = 1; else if (*f == '0') zero = 1; else break; }
        while (*f >= '0' && *f <= '9') width = width * 10 + (*f++ - '0');
        while (*f == 'l' || *f == 'h') f++;
        char sp = *f ? *f++ : 0, tmp[12]; const char *s = tmp; int len = 0, neg = 0;
        switch (sp) {
        case 'd': case 'i': { long v = va_arg(ap, long); unsigned long u = v < 0 ? (neg = 1, 0ul - (unsigned long)v) : (unsigned long)v;
                              do { tmp[11 - len++] = (char)('0' + u % 10); u /= 10; } while (u); s = tmp + 12 - len; break; }
        case 'u': { unsigned long u = va_arg(ap, unsigned long); do { tmp[11 - len++] = (char)('0' + u % 10); u /= 10; } while (u); s = tmp + 12 - len; break; }
        case 'x': case 'X': { unsigned long u = va_arg(ap, unsigned long); const char *dg = sp == 'x' ? "0123456789abcdef" : "0123456789ABCDEF";
                              do { tmp[11 - len++] = dg[u & 15]; u >>= 4; } while (u); s = tmp + 12 - len; break; }
        case 's': s = va_arg(ap, const char *); if (!s) s = "(null)"; while (s[len]) len++; zero = 0; break;
        case 'c': tmp[0] = (char)va_arg(ap, int); len = 1; zero = 0; break;
        case '%': tmp[0] = '%'; len = 1; break;
        default: continue;
        }
        int pad = width - len - neg;
        if (!left && !zero) while (pad-- > 0) PUT(' ');
        if (neg) PUT('-');
        if (!left && zero) while (pad-- > 0) PUT('0');
        for (int i = 0; i < len; i++) PUT(s[i]);
        if (left) while (pad-- > 0) PUT(' ');
    }
    if (cap) o[n < cap ? n : cap - 1] = 0;
    return (int)n;
    #undef PUT
}
int bsprintf(char *o, const char *f, ...) { va_list ap; va_start(ap, f); int n = fmt_v(o, 0x7fffffff, f, ap); va_end(ap); return n; }
int bsnprintf(char *o, unsigned cap, const char *f, ...) { va_list ap; va_start(ap, f); int n = fmt_v(o, cap, f, ap); va_end(ap); return n; }
