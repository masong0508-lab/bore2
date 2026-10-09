// compat_math.h - the few PS1 SDK maths helpers BORE 2 uses, in plain C, so the same game code builds on the DS.
// Same conventions as the PS1 GTE: 4096 = 1.0, angles 4096 = a full turn, y is down.
#pragma once
#include <stdint.h>
typedef struct { int16_t vx, vy, vz, pad; } SVECTOR;
typedef struct { int32_t vx, vy, vz, pad; } VECTOR;
typedef struct { int16_t m[3][3]; int32_t t[3]; } MATRIX;
#define setVector(v, x, y, z) ((v)->vx = (x), (v)->vy = (y), (v)->vz = (z))

int isin(int a);                 // sin(a) * 4096, a in 0..4095
int icos(int a);
int SquareRoot0(int v);          // integer square root
MATRIX *RotMatrix(SVECTOR *r, MATRIX *m);              // rotation about X, Y and Z (BORE only uses one axis at a time)
MATRIX *TransMatrix(MATRIX *m, VECTOR *t);
VECTOR *ApplyMatrixLV(MATRIX *m, VECTOR *in, VECTOR *out);   // out = m.rot * in  (no translation)
MATRIX *CompMatrixLV(MATRIX *a, MATRIX *b, MATRIX *out);     // out = a * b, translation: a.rot * b.t + a.t
