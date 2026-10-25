// fight: BORTUA FIGHTER, the arcade's fighting game, in real 3D. Your own Bore (or anyone in the household) against a ladder of
// eight, then the chrome boss BORAL, on a round ring at sunset over the sea. The rules, the blows and the opponents' AI come from
// BORE on the GBA (its BALLZ mode: bortua3dcore.h); the fighters are the voxel Bores themselves, posed joint by joint (elbows and
// knees and all: voxel.c VPose).
//   D-PAD  move (as the screen sees it: up steps into the ring, down out of it)   X punch   O kick   X + O close up: a throw
//   SQUARE guard   TRIANGLE jump (X or O in the air: a jump kick)   toward + X the BORE PALM   away + O the SPIN KICK
//   SELECT the camera (SIDE, CHASE, HIGH)   START pauses
//   A round is 30 seconds, best of three: a K.O., a RING OUT (knocked over the edge), or more life left at TIME.
#pragma once
#include "common.h"

void fight_open(int credits);              // into the arcade cabinet (from the life: paid already)
int  fight_update(int held, int pressed, int steps);   // 1 while it runs; 0 when you walk away from the cabinet
void fight_draw(void);
extern int fight_wins, fight_champ, fight_best;   // matches won, beat BORAL, the furthest stage reached (saved with the life)
