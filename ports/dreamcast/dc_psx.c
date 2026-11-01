// dc_psx.c: the parts of PSn00bSDK that BORE 2 calls, for the Dreamcast: the GTE and its maths library (in C, PS1 fixed point),
// the BIOS bits, and the PS1 register file as a plain array (the shared code writes to it; nothing reads it back as hardware).
#include <kos.h>
#include <stdint.h>
#include <string.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <psxapi.h>
#include <inline_c.h>
#include <math.h>

unsigned char dc_iobase[0x10000] __attribute__((aligned(0x10000)));
unsigned char dc_scratchpad[1024] __attribute__((aligned(32)));
DcGte dc_gte;

// ---------------------------------------------------------------- the GTE's RTPS (sf=1, lm=0), one vertex V[i]
void dc_gte_rtps(int i) {
    int32_t vx = dc_gte.v[i][0], vy = dc_gte.v[i][1], vz = dc_gte.v[i][2];   // (16-bit x 16-bit products: 32 bits each; only the sum needs 64)
    int32_t mac[3];
    for (int k = 0; k < 3; k++) {
        int64_t m = ((int64_t)dc_gte.t[k] << 12) + (int64_t)(dc_gte.r[k][0] * vx) + (int64_t)(dc_gte.r[k][1] * vy) + (int64_t)(dc_gte.r[k][2] * vz);
        mac[k] = (int32_t)(m >> 12);
    }
    dc_gte.mac[1] = mac[0]; dc_gte.mac[2] = mac[1]; dc_gte.mac[3] = mac[2];
    for (int k = 0; k < 3; k++) { int32_t v = mac[k]; dc_gte.ir[k + 1] = v < -32768 ? -32768 : v > 32767 ? 32767 : v; }
    int32_t z = mac[2]; if (z < 0) z = 0; if (z > 65535) z = 65535;
    dc_gte.sz[0] = dc_gte.sz[1]; dc_gte.sz[1] = dc_gte.sz[2]; dc_gte.sz[2] = dc_gte.sz[3]; dc_gte.sz[3] = (uint16_t)z;
    // the divide: H / SZ in 0.16, saturating at 0x1FFFF when SZ <= H/2 (what the PS1's unsigned Newton-Raphson divider does)
    uint32_t h = (uint32_t)dc_gte.h, q;
    if (z * 2 > (int32_t)h) { q = h < 32768 ? (((h << 17) / (uint32_t)z + 1) >> 1) : (uint32_t)((((uint64_t)h << 17) / (uint32_t)z + 1) >> 1); if (q > 0x1FFFF) q = 0x1FFFF; } else q = 0x1FFFF;   // (32-bit divide when it fits: the same result)
    int64_t sx = ((int64_t)dc_gte.ir[1] * q + dc_gte.ofx) >> 16, sy = ((int64_t)dc_gte.ir[2] * q + dc_gte.ofy) >> 16;
    if (sx < -1024) sx = -1024; if (sx > 1023) sx = 1023; if (sy < -1024) sy = -1024; if (sy > 1023) sy = 1023;
    dc_gte.sx[0] = dc_gte.sx[1]; dc_gte.sy[0] = dc_gte.sy[1]; dc_gte.sx[1] = dc_gte.sx[2]; dc_gte.sy[1] = dc_gte.sy[2];
    dc_gte.sx[2] = (int16_t)sx; dc_gte.sy[2] = (int16_t)sy;
}

void InitGeom(void) {
    memset(&dc_gte, 0, sizeof dc_gte);
    dc_gte.zsf3 = 0x155; dc_gte.zsf4 = 0x100; dc_gte.h = 1000;   // (PSn00bSDK's InitGeom: the average-z scales for 3 and 4 corners)
}

// ---------------------------------------------------------------- the GTE library (psxgte), in C
#define _isin_qN 10
static inline int sin_q(int qN, int x) {   // PSn00bSDK's isin (coranac's isin_S4)
    int c = x << (30 - qN), y;
    x -= 1 << qN; x <<= (31 - qN); x >>= (31 - qN); x *= x; x >>= (2 * qN - 14);
    y = 19900 - (x * 3516 >> 14); y = (1 << 12) - (x * y >> 16);
    return c >= 0 ? y : -y;
}
int isin(int x) { return sin_q(10, x); }
int icos(int x) { return sin_q(10, x + (1 << 10)); }
int hisin(int x) { return sin_q(15, x); }
int hicos(int x) { return sin_q(15, x + (1 << 15)); }

MATRIX *MulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *m2) {
    MATRIX r;
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) {
        int32_t s = (int32_t)m0->m[i][0] * m1->m[0][j] + (int32_t)m0->m[i][1] * m1->m[1][j] + (int32_t)m0->m[i][2] * m1->m[2][j];
        s >>= 12; r.m[i][j] = (int16_t)(s < -32768 ? -32768 : s > 32767 ? 32767 : s);
    }
    memcpy(m2->m, r.m, sizeof r.m);
    return m2;
}
MATRIX *RotMatrix(SVECTOR *r, MATRIX *m) {
    int s0 = isin(r->vx), s1 = isin(r->vy), s2 = isin(r->vz), c0 = icos(r->vx), c1 = icos(r->vy), c2 = icos(r->vz);
    MATRIX mx = { { { ONE, 0, 0 }, { 0, (int16_t)c0, (int16_t)-s0 }, { 0, (int16_t)s0, (int16_t)c0 } } };
    MATRIX my = { { { (int16_t)c1, 0, (int16_t)s1 }, { 0, ONE, 0 }, { (int16_t)-s1, 0, (int16_t)c1 } } };
    MATRIX mz = { { { (int16_t)c2, (int16_t)-s2, 0 }, { (int16_t)s2, (int16_t)c2, 0 }, { 0, 0, ONE } } };
    MATRIX t;
    MulMatrix0(&mx, &my, &t);
    MulMatrix0(&t, &mz, m);
    return m;
}
MATRIX *TransMatrix(MATRIX *m, VECTOR *r) { m->t[0] = r->vx; m->t[1] = r->vy; m->t[2] = r->vz; return m; }
static inline int32_t sat16_(int64_t v) { return v < -32768 ? -32768 : v > 32767 ? 32767 : (int32_t)v; }
VECTOR *ApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1) {
    // as the SDK does it on the GTE: the vector goes in through IR1..3 (16 bits) and comes out of them (saturated)
    int64_t x = (int16_t)v0->vx, y = (int16_t)v0->vy, z = (int16_t)v0->vz; VECTOR o;
    o.vx = sat16_((m->m[0][0] * x + m->m[0][1] * y + m->m[0][2] * z) >> 12);
    o.vy = sat16_((m->m[1][0] * x + m->m[1][1] * y + m->m[1][2] * z) >> 12);
    o.vz = sat16_((m->m[2][0] * x + m->m[2][1] * y + m->m[2][2] * z) >> 12);
    v1->vx = o.vx; v1->vy = o.vy; v1->vz = o.vz;
    return v1;
}
static inline int32_t sat16(int64_t v) { return v < -32768 ? -32768 : v > 32767 ? 32767 : (int32_t)v; }
MATRIX *CompMatrixLV(MATRIX *m0, MATRIX *m1, MATRIX *m2) {
    // as the SDK does it on the GTE: m1's translation goes in through IR1..3 (16 bits), comes out of IR1..3 (saturated to 16 bits)
    int64_t ix = (int16_t)m1->t[0], iy = (int16_t)m1->t[1], iz = (int16_t)m1->t[2];
    int32_t t0 = sat16((((int64_t)m0->t[0] << 12) + m0->m[0][0] * ix + m0->m[0][1] * iy + m0->m[0][2] * iz) >> 12);
    int32_t t1 = sat16((((int64_t)m0->t[1] << 12) + m0->m[1][0] * ix + m0->m[1][1] * iy + m0->m[1][2] * iz) >> 12);
    int32_t t2 = sat16((((int64_t)m0->t[2] << 12) + m0->m[2][0] * ix + m0->m[2][1] * iy + m0->m[2][2] * iz) >> 12);
    MulMatrix0(m0, m1, m2);
    m2->t[0] = t0; m2->t[1] = t1; m2->t[2] = t2;
    return m2;
}
int SquareRoot0(int v) {
    if (v <= 0) return 0;
    unsigned x = (unsigned)v, r = 0, bit = 1u << 30;
    while (bit > x) bit >>= 2;
    while (bit) { if (x >= r + bit) { x -= r + bit; r = (r >> 1) + bit; } else r >>= 1; bit >>= 2; }
    return (int)r;
}
int SquareRoot12(int v) { return SquareRoot0(v) << 6; }
void VectorNormalS(VECTOR *v0, SVECTOR *v1) {
    int64_t x = v0->vx, y = v0->vy, z = v0->vz, l2 = x * x + y * y + z * z;
    if (!l2) { v1->vx = v1->vy = v1->vz = 0; return; }
    double l = sqrt((double)l2);
    v1->vx = (int16_t)(x * 4096 / l); v1->vy = (int16_t)(y * 4096 / l); v1->vz = (int16_t)(z * 4096 / l);
}
void VectorNormal(VECTOR *v0, VECTOR *v1) { SVECTOR s; VectorNormalS(v0, &s); v1->vx = s.vx; v1->vy = s.vy; v1->vz = s.vz; }

// ---------------------------------------------------------------- the BIOS bits
int EnterCriticalSection(void) { return 1; }
void ExitCriticalSection(void) {}
