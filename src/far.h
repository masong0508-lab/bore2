// far: SAN BOREAS past the loaded map, out to the horizon. The land in big flat cells, every town's buildings as boxes (with
// rows of windows nearer in), the woods and the trees beyond the map as tree cards, all hazed into the sky with distance.
// The GTE's coordinates are 16-bit (+-32767 units, about 150 m), so the far world goes in on its own scale: (point - eye) / 16,
// a kilometre in 13,300 units. A perspective picture does not care about scale; only the depths come back 1/16 the size.
#pragma once
#include "common.h"

extern int far_view;     // how far you can see: 0 just the map round you, 1 250 m, 2 500 m, 3 1 km, 4 1.5 km, 5 2 km
extern int far_trees;    // trees past the near detail: 0 none, 1 to 100 m, 2 to 200 m, 3 to 350 m
extern int far_detail;   // buildings past the map: 0 the tall ones only, 1 every building, 2 every building with its windows
extern int far_haze;     // 0 clear air, 1 light haze, 2 thick haze
extern int far_prims;    // (last frame's far polygons)
int  far_metres(void);
void far_new_world(void);                  // a new world (after bm_new): every town's buildings, worked out once
void far_reset(void);                      // after a jump (arriving, a taxi): the land round you, worked out again
int  far_begin(u32 horizon);               // a frame's far view: set up (0: off). The sky's colour at the horizon, for the haze
void far_box_world(int x0, int z0, int x1, int z1, int sto, u16 wall15, int pitched, u16 roof15);   // a map building past the detail
void far_draw(int rx0, int ry0, int rx1, int ry1);   // the land, the towns and the trees (the detail box is left to the near world)
void far_sky(u32 top, u32 horizon);        // the sky, its haze sitting on the real horizon
