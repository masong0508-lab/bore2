// spuenc: raw signed 16-bit mono PCM on stdin -> PlayStation SPU ADPCM (28 samples per 16-byte block, best of the 5 filters and 13 shifts
// per block, closed loop against the decoder). A one-shot: the last block is a silent one flagged end. usage: spuenc <out.adpcm> < pcm.s16le
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
static const int F0[5] = { 0, 60, 115, 98, 122 }, F1[5] = { 0, 0, -52, -55, -60 };
static int clamp16(int v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : v; }
int main(int argc, char **argv) {
    if (argc < 2) return 1;
    size_t cap = 1 << 20, n = 0; int16_t *pcm = malloc(cap * 2);
    for (;;) { if (n + 65536 > cap) { cap *= 2; pcm = realloc(pcm, cap * 2); } size_t r = fread(pcm + n, 2, 65536, stdin); if (!r) break; n += r; }
    FILE *o = fopen(argv[1], "wb"); if (!o) return 1;
    int h1 = 0, h2 = 0;
    for (size_t p = 0; p < n; p += 28) {
        int x[28]; for (int i = 0; i < 28; i++) x[i] = p + i < n ? pcm[p + i] : 0;
        long best = -1; uint8_t bb[16]; int bh1 = 0, bh2 = 0;
        for (int f = 0; f < 5; f++) for (int sh = 0; sh <= 12; sh++) {
            uint8_t blk[16]; memset(blk, 0, 16); blk[0] = (uint8_t)(sh | (f << 4));
            int a = h1, b = h2; long err = 0;
            for (int i = 0; i < 28; i++) {
                int pred = (a * F0[f] + b * F1[f] + 32) >> 6;
                int d = x[i] - pred;
                int q = (int)(((long)d * (1 << sh) + (1 << 11)) >> 12);   // nibble = d / 2^(12-sh)
                q = q > 7 ? 7 : q < -8 ? -8 : q;
                int s = clamp16(((q << 12) >> sh) + pred);
                long e = s - x[i]; err += e * e;
                blk[2 + i / 2] |= (uint8_t)((q & 15) << (4 * (i & 1)));
                b = a; a = s;
            }
            if (best < 0 || err < best) { best = err; memcpy(bb, blk, 16); bh1 = a; bh2 = b; }
        }
        fwrite(bb, 1, 16, o); h1 = bh1; h2 = bh2;
    }
    uint8_t last[16] = { 0, 5 }; fwrite(last, 1, 16, o);
    fclose(o); return 0;
}
