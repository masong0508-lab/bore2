#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "seqdec.h"
typedef uint32_t u32; typedef uint16_t u16; typedef uint8_t u8;
static u8 *load(const char *f, long *n) { FILE *F = fopen(f, "rb"); fseek(F, 0, 2); *n = ftell(F); rewind(F); u8 *b = malloc(*n); fread(b, 1, *n, F); fclose(F); return b; }
int main(int c, char **v) {
    long na, nb; u8 *A = load(v[1], &na), *B = load(v[2], &nb);
    int n = ((u32 *)A)[1], bad = 0; long total = 0;
    static SeqDec d;
    for (int i = 0; i < n; i++) {
        u32 *ia = (u32 *)(A + 8 + i * 16), *ib = (u32 *)(B + 8 + i * 16);
        if (!ia[1]) continue;
        u8 *a = A + ia[0] * 2048, *b = B + ib[0] * 2048;
        u32 *ha = (u32 *)a, *hb = (u32 *)b;
        u32 nord = ha[1], loop = ha[2];
        const u8 *ord = a + ha[11]; const u16 *rows = (const u16 *)(a + ha[12]); const u32 *po = (const u32 *)(a + ha[13]); const u8 *ev = a + ha[18];
        const u32 *x = (const u32 *)(b + hb[19]);
        if (hb[0] != 0x32515342) { printf("song %d not packed\n", i); bad++; continue; }
        if (loop) seqdec_start(&d, b + hb[18], x[0]); else seqdec_start(&d, b + x[1], x[2]);
        int o = 0, r = 0, laps = 0;
        while (laps < 2) {
            const u8 *e = ev + po[ord[o]]; int rr = r, cnt;
            for (;;) { int h = *e++; if (h & 0x80) { h &= 0x7f; if (rr < h) { cnt = 0; break; } rr -= h; } else if (rr-- == 0) { cnt = h; break; } else e += 3 * h; }
            int m = seqdec_row(&d);
            if (m != cnt) { printf("song %d ord %d row %d: %d vs %d notes\n", i, o, r, cnt, m); bad++; break; }
            for (int k = 0; k < 3 * cnt; k++) if (seqdec_byte(&d) != e[k]) { printf("song %d ord %d row %d: note bytes differ\n", i, o, r); bad++; laps = 9; break; }
            total += cnt;
            if (++r >= rows[ord[o]]) { r = 0; if (++o >= (int)nord) { o = loop; laps++; seqdec_start(&d, b + x[1], x[2]); } else if (o == (int)loop) seqdec_start(&d, b + x[1], x[2]); }
        }
    }
    printf("%s: %ld notes compared over two laps of every song\n", bad ? "MISMATCH" : "all songs match", total);
    return bad != 0;
}
