// spuenc: raw signed 16-bit mono PCM on stdin -> PlayStation SPU ADPCM (28 samples per 16-byte block, best of the 5 filters and 13 shifts
// per block, closed loop against the decoder).
//   spuenc out.adpcm            a one-shot: the last block is a silent one flagged end (the voice is muted there)
//   spuenc out.adpcm --loop     a seamless loop: the input is the loop body (padded with its own start to a whole number of 28-sample blocks);
//                               block 0 is flagged "loop start", the last block "end + repeat"; the body is encoded twice and the first pass thrown
//                               away, so the decoder's history at the jump is the loop's own end and the jump makes no click
//   spuenc out.adpcm --stream   plain blocks, no flags and no end block (tools/make_film_mix.py cuts it into chunks and flags those)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
static const int F0[5] = { 0, 60, 115, 98, 122 }, F1[5] = { 0, 0, -52, -55, -60 };
static int clamp16(int v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : v; }
static void encode(const int16_t *pcm, size_t n, int *ph1, int *ph2, FILE *o) {
    int h1 = *ph1, h2 = *ph2;
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
        if (o) fwrite(bb, 1, 16, o);
        h1 = bh1; h2 = bh2;
    }
    *ph1 = h1; *ph2 = h2;
}
int main(int argc, char **argv) {
    if (argc < 2) return 1;
    int loop = argc > 2 && !strcmp(argv[2], "--loop"), stream = argc > 2 && !strcmp(argv[2], "--stream");
    size_t cap = 1 << 20, n = 0; int16_t *pcm = malloc(cap * 2);
    for (;;) { if (n + 65536 > cap) { cap *= 2; pcm = realloc(pcm, cap * 2); } size_t r = fread(pcm + n, 2, 65536, stdin); if (!r) break; n += r; }
    FILE *o = fopen(argv[1], "wb"); if (!o) return 1;
    int h1 = 0, h2 = 0;
    if (loop) {
        size_t m = (n + 27) / 28 * 28;
        if (m != n) { pcm = realloc(pcm, m * 2 + 2); for (size_t i = n; i < m; i++) pcm[i] = pcm[i - n]; n = m; }
        encode(pcm, n, &h1, &h2, NULL);                       // pass one: only for the history at the end of the body
        encode(pcm, n, &h1, &h2, o);
        fflush(o); fseek(o, 1, SEEK_SET); fputc(4, o);        // loop start on the first block
        fseek(o, -15, SEEK_END); fputc(3, o);                 // end + repeat on the last
    } else {
        encode(pcm, n, &h1, &h2, o);
        if (!stream) { uint8_t last[16] = { 0, 5 }; fwrite(last, 1, 16, o); }
    }
    fclose(o); return 0;
}
