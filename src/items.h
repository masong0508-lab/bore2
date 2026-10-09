// items: BORE's room items (textured boxes) in 3D, keyed by their map letter.
#pragma once
#include "common.h"

void items_init(void);                                   // uploads every material to the atlas
// draw the item letter ch standing on tile (mx,my). rot = facing 0..3 (0 = south, towards the camera at the default view),
// along = 1 when a rail / ledge / bench runs along x. t = the frame counter (lava lamp, aquarium, fire animate)
void items_draw(char ch, int mx, int my, int rot, int along, int t);
int  items_blocking(char ch);                            // 1: a wall of a thing you cannot walk through
int  items_is_item(char ch);                             // 1: a placeable item letter
const char *items_name(char ch);
int  items_height(char ch);                              // the top of the item in world units (for standing on crates etc.)
int  items_rotates(char ch);                             // 1: it faces away from the wall
int  items_along(char ch);                               // 1: rails, ledges, benches, barriers link up along a line
int  items_count(void);
char items_at(int i);                                    // the i-th placeable letter (the build menu order)
int  items_cost(char ch);
