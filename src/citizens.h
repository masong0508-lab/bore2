// citizens: a hundred civilian Bores who live in the island, the town and the city.
// Cheap crowd people (the LOD model) walk the streets; the few nearest you are promoted to the full model.
// You can talk, joke, hug, flirt, give gifts, propose (they move in as a housemate) and fight them.
#pragma once
#include "common.h"

#define NCIT 100

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
