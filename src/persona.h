// persona: a town person's personality, made from their id (seed) alone. Plain C, no game headers: the game, the DS port and the host test compile this one file.
// The same seed always gives the same five bars, star sign, aspiration and life want: in town, on their card, and when they move in (the Bore gets exactly these).
// Nothing is stored: it is a few hashes, so no citizen / person record grows.
#pragma once
#ifdef TR_HOST
#include <stdint.h>
typedef uint8_t u8; typedef uint32_t u32;
#else
#include "common.h"
#endif
typedef struct { u8 bar[5]; u8 sign, asp, ltw; } Persona;   // bars: NEAT OUTGOING ACTIVE PLAYFUL NICE (0..10, the five add up to 25); sign 0..11; asp 0..4 (5: GROW UP, a child's); ltw 0..3
extern const u8 pers_sign_tr[12][5];                         // each star sign's bars
int  persona_sign(const u8 bar[5]);                          // the star sign whose bars are nearest
void persona_of(u32 seed, int stage, Persona *p);            // stage < 2 (BABY, CHILD): the aspiration is GROW UP
