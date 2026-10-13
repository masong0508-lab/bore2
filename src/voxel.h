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
    int sleep;      // 1: lying down (or heading that way)
    int lie;        // 0..4096: how far a lie-down (or a get-up) has got; main.c eases it toward 4096 while sleep is 1, back to 0 when it is not
    int bed;        // lying on: 1 the bed (a pillow under the head, a blanket over), 2 another bed, 0 the floor, a crib or the water
    int wave;       // frames left of a wave / talk gesture
    int squash;     // -16..16 (reaction pops)
    // the body language (main.c fills these): the pose is worked out in voxel_draw from them
    int mood;       // -100..100: happy Bores bounce, sad ones droop
    int talk;       // 1: in a conversation (gestures, nods)
    int seed;       // a per-person offset so a room of Bores never breathes in step
    int clock;      // frames, for the idle cycle
} Gait;

// The crowd model: the same person at half the voxel resolution (4 x 2 x 4 cells instead of 6 x 4 x 8), made from the full Bore
// build and keeping its colours, hair, clothes and walk. About 50 bytes, so a whole city of them can be kept; a citizen is only
// given a full Bore (and the full model) when they are close enough to meet. Roughly 25 quads instead of 150+.
typedef struct {
    u8  c[4][2][4];   // [row from the feet][z back..front][x left arm, left half, right half, right arm]: 0 empty, else colour slot 1..8
    u16 col[9];       // the person's colour slots (the same as Bore.col)
    u8  legrows;      // coarse rows that are legs
    u8  headrow;      // first coarse row of the head
} LodBore;
void voxel_lod_build(const Bore *b, LodBore *m);
void voxel_draw_lod(const LodBore *m, const VECTOR *pos, int yaw, const Gait *g);

void voxel_init(void);
void voxel_prepare(Bore *b);   // after buildLook: fills the drawing cache in the Bore
void voxel_draw(const Bore *b, const VECTOR *pos, int yaw, const Gait *g);
void voxel_draw_covers(const Bore *b, const VECTOR *pos, int yaw, const Gait *g);   // the blanket pulled over someone lying in a bed (after voxel_draw)
int  voxel_lie_ease(int lie);                                                         // lie 0..4096 as a smooth 0..4096 (slow, quick, slow)
int  voxel_quads(void);     // quads drawn by the last call (debug)
