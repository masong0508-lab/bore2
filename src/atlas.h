// atlas: 15-bit textures packed into VRAM (two 256x256 pages at x=640). Everything BORE drew as pixel art
// (item faces, wallpapers, floors, eyes and mouths) lives here and is mapped onto 3D faces.
#pragma once
#include "common.h"

typedef struct { u16 tpage; u8 u, v, w, h, cap; } TexRef;   // cap: the allocated width (a slot can hold narrower art)

void   atlas_init(void);
TexRef atlas_alloc(int w, int h);
TexRef atlas_add_spill(int w, int h, const u16 *pixels);   // like atlas_add, spilling into the strip when the pages are full
TexRef atlas_alloc_page(int page, int w, int h);   // on one page (2 = the 128-wide strip, kept for the font)
TexRef atlas_try(int w, int h);                            // a small texture wherever it fits, or none (not a failure: the caller copes)
void   atlas_upload(const TexRef *t, const u16 *pixels);   // w*h pixels, row major (0x0000 = transparent)
extern int atlas_defer;                                     // 1: small uploads wait for the frame's flip (no stall on the GPU mid-frame)
void   atlas_flush_deferred(void);                           // (gfx_flip: send them)
TexRef atlas_add(int w, int h, const u16 *pixels);
TexRef atlas_solid(u16 c15);                                // a 2x2 solid texture
extern int atlas_fails;                                     // textures that did not fit
u16   *atlas_scratch_hi(void);                                // its upper half (8192 pixels): a texture built there can be uploaded from it (the upload copies into the lower half)
u16   *atlas_scratch(void);                                   // 128x128 pixels of scratch (free between uploads: the world map is built in it)
void   atlas_vram_load(int x, int y, int w, int h, const u16 *pixels);   // pixels straight into VRAM (w*h even)
int    atlas_used(void);                                    // pixels used (for the debug readout)
TexRef atlas_add_aligned(int w, int h, int a, const u16 *pixels);   // placed at u, v multiples of a (for a repeating texture window)
