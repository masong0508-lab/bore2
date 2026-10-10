// BORE 2 (PlayStation) - shared types and world constants.
// The sequel to BORE (GBA): the voxel people, the rooms and the items are the real BORE data, now in true 3D.
#pragma once
#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>
#include <inline_c.h>

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32;

// 15-bit colour, the same bit layout the GBA used (red in the low bits): the BORE palettes port unchanged.
#define RGB(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
#define C_R(c) ((c)&31)
#define C_G(c) (((c)>>5)&31)
#define C_B(c) (((c)>>10)&31)
// 15-bit colour -> 8-bit channels for a primitive
#define R8(c) ((C_R(c)<<3)|(C_R(c)>>2))
#define G8(c) ((C_G(c)<<3)|(C_G(c)>>2))
#define B8(c) ((C_B(c)<<3)|(C_B(c)>>2))

#define SCREEN_W 320
#define SCREEN_H 240
#define CENTER_X (SCREEN_W/2)
#define CENTER_Y (SCREEN_H/2)

// ---- world units (y is DOWN, like the GTE: a height h sits at y = -h) ----
#define TILE   256      // one room tile
#define VOX    48       // one creature block (8 blocks = 384 = the height of a wall's doorway... a person is 1.5 tiles tall)
#define IUNIT  32       // one item unit (BORE items are 8 units per tile)
#define IZ     24       // one item z px (a 16 px crate is 384 tall, as tall as a person)
#define WALL_FULL 512   // a wall
#define WALL_LOW  160   // a low wall / a cut-away wall

// ---- the creature build space (BORE) ----
#define W 6
#define D 4
#define H 8

// ---- the map (BORE) ----
#define MW 64   // the map: the old 40x40 lots were one house; 64x64 holds the open-world city (a 4x4 grid of 16-tile blocks)
#define MH 64

#define CLAMP(v,lo,hi) ((v)<(lo)?(lo):(v)>(hi)?(hi):(v))
#define ABS(v) ((v)<0?-(v):(v))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

static inline int fx_mul(int a, int b) { return (int)(((long long)a * b) >> 12); }   // 20.12 x 20.12
