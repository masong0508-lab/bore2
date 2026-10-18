// skate: riding the skateboard, Tony Hawk style. Push, carve, ollie, flip and grab in the air, spin with L1 R1, grind rails, ledges
// and the coping, manual between tricks, ride quarter pipes up into vert air and back down, and bail when you land it wrong.
// Every trick goes into the combo (points times the number of tricks); land clean and it banks.
#pragma once
#include "sim.h"

enum { SKB_X = 1, SKB_SQ = 2, SKB_O = 4, SKB_TRI = 8, SKB_L1 = 16, SKB_R1 = 32 };

void skate_on(Sim *s);                 // stepping on the board
void skate_off(Sim *s);                // stepping off it
void skate_reset(void);
void skate_input(int held, int pressed);   // the pad, once a frame, before the steps (SKB_ bits)
void skate_step(Sim *s, int wx, int wz);   // one logic step; (wx, wz) the stick in world space, -4096..4096 each
int  skate_dir(void);                  // the way the board rolls
int  skate_speed(void);                // units a step
int  skate_air(void);                  // in the air (L1 R1 spin, not the camera)
int  skate_bailing(void);              // lying on the ground after a bail
int  skate_body_yaw(int yaw);          // which way the rider's body faces (side on to the board)
void skate_pose(Gait *g);              // the rider's pose (crouch, arms out, a grab, balance)
void skate_board(int *roll, int *pitch, int *spin);   // the board's own turn: a flip about its length or width, a shove-it about the up axis
int  skate_board_up(void);             // how far a grab pulls the board up
void skate_hud(void);                  // the combo, the balance meter, the score
const char *skate_hint(void);          // what X does now ("OLLIE")
extern long skate_score, skate_best;  // points banked this session, and the best combo
