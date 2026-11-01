// places: going out in SAN BOREAS. Which place a tile of the map window belongs to (a shop, a diner, a gym, a pool...),
// its name and its opening hours; and the careers: a job at a real diner, shop, gym, pool, inn, arcade, office or school
// of the state (or the old factory back home), each with its own shift, days, pay and six job titles.
#pragma once
#include "common.h"

enum { CAR_FACTORY, CAR_FOOD, CAR_RETAIL, CAR_FIT, CAR_SWIM, CAR_HOTEL, CAR_GAME, CAR_BIZ, CAR_TEACH,
       CAR_NIGHT, CAR_MED, CAR_FILM, CAR_BEAUTY, CAR_ART, CAR_FARM, CAR_N };   // (FARM: at a farm, not a place: job_openings finds the nearest, place -2)
typedef struct {
    const char *name;                 // "CULINARY"
    u32 lots;                         // where it is done (a bit for each LOT_ that will do)
    u8 from, to, days;                // the shift (hours) and its days (bit 0 MONDAY .. bit 6 SUNDAY)
    u16 pay0, pay_lvl;                // a full shift's pay at level 0, and how much more each level up
    const char *station;              // the things you work at there
    const char *title[6];             // level 0 .. 5
} Career;
extern const Career careers[CAR_N];

extern int  job_car;                  // the player's career
extern int  job_bx, job_by;           // the workplace's block in SAN BOREAS (the factory: -1)
extern char job_place[24];            // the workplace's name
extern int  groceries;                // meals' worth of good food in the fridge at home (a grocery shop sells them)
extern int  room_bx, room_by, room_until;   // a rented room (an inn, a hotel): its block, and until when (minutes since the life began)

typedef struct { int lot; u32 h; int bx, by; char name[24]; } PlaceInfo;
int  place_at(int tx, int ty, PlaceInfo *pi);     // the place a tile of the city window stands on (0: none, or the street)
int  place_open(int lot, int min, int day);       // open at that minute of that day?
const char *place_hours(int lot, char *out);      // "OPEN 8 AM - 10 PM", "OPEN ALL DAY", "MON - FRI 8 AM - 6 PM"
int  place_opens_at(int lot);                     // the hour it opens (for "CLOSED TILL 8 AM")
int  place_closes_at(int lot);                    // the hour it shuts (24: midnight)
const char *hour_name(int h, char *out);          // "7 AM", "12 PM"
const char *days_name(int mask, char *out);       // "MON - FRI"
int  place_shop_kind(u32 h);                      // 0 grocery, 1 clothes, 2 electronics, 3 furniture, 4 music (as world.c builds it)

int  job_on_shift(void);                          // the player's shift is on right now
int  job_shift_len(void);                         // minutes in a shift
int  job_pay(void);                               // a full shift's pay at the player's level
const char *job_title(void);                      // "LINE COOK"
int  job_here(int tx, int ty, char c);            // c at (tx, ty) is something the player works at, at their own workplace
void job_take(int car, int bx, int by, const char *place);   // a new job (the same career elsewhere keeps the level)
int  job_openings(int refbx, int refby, int *car, int *place, int n);   // jobs going near that block: careers and pop_place indices
extern int job_farm_bx, job_farm_by;               // (the farm job_openings found: its place is -2)
const char *farm_name(int bx, int by, char *out); // "SUNNY ACRES FARM"
int  life_minutes(void);                          // minutes since the life began (for the room)
