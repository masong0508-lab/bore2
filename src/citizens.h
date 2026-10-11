// citizens: a hundred civilian Bores who live in the island, the town and the city.
// Cheap crowd people (the LOD model) walk the streets; the few nearest you are promoted to the full model.
// You can talk, joke, hug, flirt, give gifts, propose (they move in as a housemate) and fight them.
#pragma once
#include "common.h"

#define NCIT 128                  // 0..9 the island, 10..49 the town, 50.. SAN BOREAS (taken by whoever of its people is near)

void citizens_init(void);                 // a new life: roll the hundred (bodies are built as each zone is first seen)
void citizens_enter(int zone);            // after arriving in a zone: put its people out on the streets
void citizens_update(int steps);          // once a frame from the life loop
void citizens_draw(void);                 // inside draw_sims
void citizens_draw_bubbles(void);         // speech over heads
void citizens_hud(void);                  // who is in front of you, and your health in a fight

int         citizens_punch(void);         // X during a fight: throw a punch (1 if it was used)
const char *citizens_hint(void);          // what X would do ("" if no citizen is near)
int         citizens_pie_open(char opt[8][16]);   // the pie menu for the citizen in front (0 = none)
const char *citizens_pie_title(void);
void        citizens_pie_do(int i);
int         citizens_party(void);         // throw a party round you: guests come (returns how many; 0 none, -1 one is on)
int         citizens_party_on(void);
// SAN BOREAS: its people come and go as the map window moves (pop.c decides who should be here)
int  citizens_take(int pid, int at, int place);  // bring person pid to life: at home (place -1) or at a place (its index); the slot, or -1
void citizens_release(int slot);                 // they are wanted elsewhere: gone as soon as nobody is looking
int  citizens_place_of(int slot);                // where a living one was brought to life: the place, or -1 for home
void citizens_shift(int dtx, int dty);           // the window moved by that many tiles: everyone with it
