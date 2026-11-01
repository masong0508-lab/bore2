// citizens: a hundred civilian Bores who live in the island, the town and the city.
// Cheap crowd people (the LOD model) walk the streets; the few nearest you are promoted to the full model.
// You can talk, joke, hug, flirt, give gifts, propose (they move in as a housemate) and fight them.
#pragma once
#include "common.h"

#define NCIT 128                  // 0..9 the island, 10..49 the town, 50.. SAN BOREAS (taken by whoever of its people is near)

void citizens_init(void);                 // a new life: roll the hundred (bodies are built as each zone is first seen)
void citizens_enter(int zone);            // after arriving in a zone: put its people out on the streets
void citizens_rebuild(void);              // every model made again (the crowd arena was lent to KINGDOM MODE)
void citizens_update(int steps);          // once a frame from the life loop
void citizens_draw(void);                 // inside draw_sims
void citizens_draw_bubbles(void);         // speech over heads
void citizens_hud(void);                  // who is in front of you, and your health in a fight

int         citizens_punch(void);         // X during a fight: throw a punch (1 if it was used)
const char *citizens_hint(void);          // what X would do ("" if no citizen is near)
int         citizens_pie_open(char opt[8][24]);   // the pie menu for the citizen in front (0 = none)
const char *citizens_pie_title(void);
int         citizens_pie_do(int i);              // 1: it opened a sub-menu (ask citizens_pie_more for its choices)
void        citizens_door_marks(void);           // (doors) every person the crowd has out, for the doors to open for
int         citizens_pie_more(char opt[8][24]);
#define PG_GROUPS 48
#define PG_MEMS   160
typedef struct { u16 fam, first; u8 n; } PGroup;     // a household on the guest list (fam: pop.c's, or 0x8000 | the town's own)
typedef struct { u16 ref; u8 on, known; } PGuest;     // one person (ref: pop.c's pid, or 0x8000 | a citizen's slot)
extern PGroup pg_grp[PG_GROUPS]; extern PGuest pg_mem[PG_MEMS]; extern int pg_ng, pg_nm, party_open, party_go, party_score_now;
void citizens_known(void);                 // (the party pack) build the guest list: every household with someone you know
void citizens_names(u16 ref, char *out);   // a person's first name
void citizens_gname(int g, char *out);     // a household's name
int  citizens_party_score_here(void);      // the party things round you (0..24)
int         citizens_party(void);         // throw a party round you: guests come (returns how many; 0 none, -1 one is on)
int         citizens_party_on(void);
int         citizens_friends(void);       // (stories) how many people out there are your friends
// SAN BOREAS: its people come and go as the map window moves (pop.c decides who should be here)
int  citizens_take(int pid, int at, int place);  // bring person pid to life: at home (place -1) or at a place (its index); the slot, or -1
void citizens_release(int slot);                 // they are wanted elsewhere: gone as soon as nobody is looking
int  citizens_place_of(int slot);                // where a living one was brought to life: the place, or -1 for home
int  citizens_send_buyer(int tx, int ty);       // FARM LIFE: someone near the farm stand at that tile comes to buy (1: sent)
void citizens_shift(int dtx, int dty);           // the window moved by that many tiles: everyone with it
