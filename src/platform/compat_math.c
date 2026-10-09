// compat_math.c - see compat_math.h
#include "compat_math.h"
static const int16_t qsin[1025] = {
#include "compat_sin.inc"
};
int isin(int a) {
    a &= 4095; int q = a >> 10, i = a & 1023;
    switch (q) { case 0: return qsin[i]; case 1: return qsin[1024 - i]; case 2: return -qsin[i]; default: return -qsin[1024 - i]; }
}
int icos(int a) { return isin(a + 1024); }
int SquareRoot0(int v) {
    if (v <= 0) return 0;
    unsigned x = (unsigned)v, r = 0, b = 1u << 30;
    while (b > x) b >>= 2;
    while (b) { if (x >= r + b) { x -= r + b; r = (r >> 1) + b; } else r >>= 1; b >>= 2; }
    return (int)r;
}
static int16_t fx(int v) { return (int16_t)v; }
MATRIX *RotMatrix(SVECTOR *r, MATRIX *m) {
    // Rz * Ry * Rx, the PS1 order; BORE only ever sets one angle
    int sx = isin(r->vx), cx = icos(r->vx), sy = isin(r->vy), cy = icos(r->vy), sz = isin(r->vz), cz = icos(r->vz);
    int ryx[3][3] = {
        { cy, (sy * sx) >> 12, (sy * cx) >> 12 },
        { 0, cx, -sx },
        { -sy, (cy * sx) >> 12, (cy * cx) >> 12 } };
    for (int i = 0; i < 3; i++) {
        m->m[0][i] = fx((cz * ryx[0][i] - sz * ryx[1][i]) >> 12);
        m->m[1][i] = fx((sz * ryx[0][i] + cz * ryx[1][i]) >> 12);
        m->m[2][i] = fx(ryx[2][i]);
    }
    return m;
}
MATRIX *TransMatrix(MATRIX *m, VECTOR *t) { m->t[0] = t->vx; m->t[1] = t->vy; m->t[2] = t->vz; return m; }
VECTOR *ApplyMatrixLV(MATRIX *m, VECTOR *in, VECTOR *out) {
    int x = in->vx, y = in->vy, z = in->vz;
    out->vx = (m->m[0][0] * x + m->m[0][1] * y + m->m[0][2] * z) >> 12;
    out->vy = (m->m[1][0] * x + m->m[1][1] * y + m->m[1][2] * z) >> 12;
    out->vz = (m->m[2][0] * x + m->m[2][1] * y + m->m[2][2] * z) >> 12;
    return out;
}
MATRIX *CompMatrixLV(MATRIX *a, MATRIX *b, MATRIX *out) {
    MATRIX r; VECTOR bt = { b->t[0], b->t[1], b->t[2], 0 }, at;
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++)
        r.m[i][j] = fx((a->m[i][0] * b->m[0][j] + a->m[i][1] * b->m[1][j] + a->m[i][2] * b->m[2][j]) >> 12);
    ApplyMatrixLV(a, &bt, &at);
    r.t[0] = at.vx + a->t[0]; r.t[1] = at.vy + a->t[1]; r.t[2] = at.vz + a->t[2];
    *out = r; return out;
}
