// persona: a town person's personality, made from their id (seed) alone. Plain C, no game headers: the game, the DS port and the host test compile this one file.
// The same seed always gives the same five bars, star sign, aspiration and life want: in town, on their card, and when they move in (the Bore gets exactly these).
// Nothing is stored: it is a few hashes, so no citizen / person record grows.
#pragma once
#ifdef TR_HOST
#include <stdint.h>
typedef uint8_t u8; typedef int8_t s8; typedef uint32_t u32;
#else
#include "common.h"
#endif
typedef struct { u8 bar[5]; u8 sign, asp, ltw; } Persona;   // bars: NEAT OUTGOING ACTIVE PLAYFUL NICE (0..10, the five add up to 25); sign 0..11; asp 0..4 (5: GROW UP, a child's); ltw 0..3
extern const u8 pers_sign_tr[12][5];                         // each star sign's bars
int  persona_sign(const u8 bar[5]);                          // the star sign whose bars are nearest
void persona_of(u32 seed, int stage, Persona *p);            // stage < 2 (BABY, CHILD): the aspiration is GROW UP

typedef struct { u8 age, pers; s8 off; u32 first, last; } TownDna;   // age 0..3 (CHILD, TEEN, ADULT, ELDER), pers 0..3, minutes off the clock, the two name dice
u32  persona_mix(u32 a, u32 b);
u32  persona_town_seed(u32 wseed, u32 id);                   // a crowd townie's seed: only the save's world seed and their id (slot) go in. Never zero
void persona_town_dna(u32 seed, TownDna *d);                 // the rest of who they are, from that seed alone
