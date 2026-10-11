// nature: CASTAWAY ISLAND drawn as a place, not as blocks. world.c colours the island's ground corner by corner (sand into meadow
// into jungle, wet sand and foam at the waterline, shade under the trees, a sea from the shallows' turquoise to deep blue) over
// the fine grain textures made here; and the island's plants are built as plants: palms with ringed, curving trunks and drooping
// fronds, jungle trees with ragged leafy crowns, pines in tiers of needles, berry bushes and mossy boulders. Leaves are cut-out
// textures (the PlayStation leaves a texel of 0 undrawn), so their edges are leaves, not lines.
#pragma once
#include "common.h"
#include "atlas.h"

extern TexRef nat_tex_ground, nat_tex_grass, nat_tex_water;   // 32x32 grey grain (16 texels a tile), coloured by the ground's corners
void nat_init(void);                                          // the textures (once, at boot)
int  nat_plant_char(char c);                                  // a plant nature.c draws (on the island)
void nat_plant(char c, int mx, int my, int t, int fade);      // fade: see-through (it stands between the camera and you)
