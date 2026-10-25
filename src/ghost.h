// ghost: the ghosts of the family's departed, at their graves (ghost.c)
#pragma once
#include "common.h"
extern int ghost_mode;            // OPTIONS > LIFE > GHOSTS: 0 OFF, 1 ON (at night), 2 HAUNTED (always)
void ghost_init(void);            // the ghost's two frames into the texture atlas (at start-up, while it has room)
void ghost_tick(int steps, int t);   // the scares (the life loop)
void ghost_draw(int cam_yaw, int t); // after the world
