// crowd: many people at the full model's quality.
// Every built person keeps a compact copy of their prepared model (about 2 KB: the cells with their corners as byte offsets
// from the grid) in the RAM the voice clips used before they went to the sound chip. From that copy a person is drawn
//  - LIVE: unpacked and drawn in 3D, every frame (the nearest few), or
//  - as a SPRITE: the same full model rendered into a cell of spare VRAM now and then and shown as one quad between renders.
// The renders go round the crowd as the frame's time allows, the ones that changed most (turned, moved, animating) first.
#pragma once
#include "common.h"
#include "look.h"
#include "voxel.h"

#define CROWD_MAX 128             // people ids (the citizens are 0..NCIT-1)

void crowd_init(void);            // after audio_init (takes over the voice blob's RAM)
void crowd_reset(void);           // forget every stored model and sprite (a new life)
void crowd_faces(int id, Bore *b);    // give person id a face texture slot: call BEFORE building them into b (look_build bakes into it)
int  crowd_store(int id, const Bore *b);   // keep a compact copy of b's prepared model for id (0: no room)
int  crowd_has(int id);
void crowd_forget(int id);
const Bore *crowd_unpack(int id); // the full model again (a shared scratch Bore: valid until the next call)

// drawing, once a frame per visible person, nearest first
void crowd_frame(void);           // start of the citizens' drawing: budgets and the frame stamp
int  crowd_draw_live(int id, const VECTOR *pos, int yaw, const Gait *g);   // 0 if the model is not stored
// the sprite: draws it if valid, and (re)renders it first when asked and time allows; 0 if nothing could be drawn
int  crowd_sprite(int id, const VECTOR *pos, int yaw, const Gait *g, int render);
int  crowd_sprite_score(int id, const VECTOR *pos, int yaw, const Gait *g);   // how badly the sprite wants a re-render (0: fine)
void crowd_drop_sprite(int id);
int  crowd_models(void);          // (HUD) models stored, sprites rendered this frame
extern int crowd_renders, crowd_bytes;
