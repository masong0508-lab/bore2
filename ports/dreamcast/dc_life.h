// dc_life: SAN BOREAS in 3D on the Dreamcast, from the game's own world code (bigmap.c) and people code (look.c, pop.c).
#pragma once
void life_init(void);                      // once: the world from its seed, the people
void life_title_scene(int t);              // the title's backdrop (a turning Bore)
void life_enter(void);                     // stepping into the life
void life_frame(int t, int pad, int pressed);   // one frame of the life: update and draw
