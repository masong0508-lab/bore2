// helpers for mk_dsmusic.py: decode the PS1 sound chip's ADPCM, encode the DS's IMA-ADPCM
#include <stdint.h>
#include <string.h>
static const int f0[5] = { 0, 60, 115, 98, 122 }, f1[5] = { 0, 0, -52, -55, -60 };
// decode from d until an end-flag block (or n bytes); returns samples written
int spu_decode(const uint8_t *d, int n, int16_t *out) {
    int o = 0, s1 = 0, s2 = 0;
    for (int b = 0; b + 16 <= n; b += 16) {
        int sh = d[b] & 15, fl = d[b] >> 4, flags = d[b + 1]; if (fl > 4) fl = 4; if (sh > 12) sh = 9;
        for (int i = 0; i < 28; i++) {
            int nib = (d[b + 2 + i / 2] >> ((i & 1) * 4)) & 15;
            int s = (int16_t)(nib << 12) >> sh;
            s += (s1 * f0[fl] + s2 * f1[fl] + 32) >> 6;
            if (s > 32767) s = 32767; if (s < -32768) s = -32768;
            out[o++] = (int16_t)s; s2 = s1; s1 = s;
        }
        if (flags & 1) break;
    }
    return o;
}
static const int8_t idxt[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };
static const int16_t stept[89] = { 7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767 };
// DS IMA-ADPCM: a word (the first sample, the step index), then nibbles, low first. Returns bytes (a multiple of 4).
static int ima_run(const int16_t *in, int n, uint8_t *out, int idx0, double *err) {
    int pred = n ? in[0] : 0, idx = idx0; double e = 0;
    uint32_t hd = (uint16_t)pred | ((uint32_t)idx << 16); if (out) memcpy(out, &hd, 4);
    int o = 4, half = 0; uint8_t cur = 0;
    for (int i = 0; i < n; i++) {
        int step = stept[idx], diff = in[i] - pred, code = 0;
        if (diff < 0) { code = 8; diff = -diff; }
        int d = step >> 3;
        if (diff >= step) { code |= 4; diff -= step; d += step; }
        if (diff >= step >> 1) { code |= 2; diff -= step >> 1; d += step >> 1; }
        if (diff >= step >> 2) { code |= 1; d += step >> 2; }
        pred += (code & 8) ? -d : d; if (pred > 32767) pred = 32767; if (pred < -32768) pred = -32768;
        e += (double)(in[i] - pred) * (in[i] - pred);
        idx += idxt[code]; if (idx < 0) idx = 0; if (idx > 88) idx = 88;
        if (out) { if (!half) { cur = code; half = 1; } else { out[o++] = cur | (code << 4); half = 0; } }
    }
    if (out) { if (half) out[o++] = cur; while (o & 3) out[o++] = 0; }
    *err = e; return o;
}
int ima_encode(const int16_t *in, int n, uint8_t *out) {
    int best = 0; double be = 1e300, e;
    for (int i0 = 0; i0 < 89; i0 += 4) { ima_run(in, n, 0, i0, &e); if (e < be) { be = e; best = i0; } }
    return ima_run(in, n, out, best, &e);
}
