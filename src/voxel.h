// voxel: draws a BORE person's 6x4x8 block model as real 3D geometry with the GTE.
// Cubes, sloped wedge blocks, thinner limb blocks, textured eyes and mouth, hair with strands and a sheen,
// legs and arms that swing as 3D parts.
#pragma once
#include "common.h"
#include "look.h"

typedef struct {
    int phase;      // walk cycle 0..4095
    int amp;        // swing amount 0..4096
    int sit;        // 1: sitting (legs forward)
    int sleep;      // 1: lying down
    int wave;       // frames left of a wave / talk gesture
    int squash;     // -16..16 (reaction pops)
} Gait;

void voxel_init(void);
void voxel_prepare(Bore *b);   // after buildLook: fills the drawing cache in the Bore
void voxel_draw(const Bore *b, const VECTOR *pos, int yaw, const Gait *g);
int  voxel_quads(void);     // quads drawn by the last call (debug)
