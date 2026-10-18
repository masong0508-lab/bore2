// pop: the people of SAN BOREAS. 874 households, every one of them generated for the save and kept track of: who lives
// where (some homes stay empty), who they are (singles, an old man on his own, couples, roommates, young families, big
// families, single parents, three generations under one roof), what they do (jobs at the shops, diners, offices, inns,
// pools, schools, arcades and gyms of their town; school for the kids), and where each of them is right now (asleep, at home,
// at work, at school, out at the park, the pool, the diner...), worked out from the clock.
// The ones whose whereabouts fall inside the map window around you are brought to life as citizens (citizens.c).
#pragma once
#include "common.h"

#define NFAM    874
#define PER_MAX 2600
enum { HK_SINGLE, HK_OLDTIMER, HK_COUPLE, HK_RETIRED, HK_ROOMMATES, HK_YOUNGFAM, HK_BIGFAM, HK_SINGLEPARENT, HK_THREEGEN, HK_N };
enum { AT_SLEEP, AT_HOME, AT_WORK, AT_SCHOOL, AT_OUT };

typedef struct { u16 bx, by; u8 unit, n; u16 first; u16 sur; u8 set, kind; } Fam;
typedef struct { u32 seed; u16 fam, job; u8 stage, trait, flags, slot; s8 rel, rom; u8 hp, shift; } Per;
typedef struct { u16 bx, by; u8 lot, set; } Place;
#define PF_HOMEBODY 1
#define PF_GONE     2                 // moved in with you, or otherwise out of the world (hp 0: passed away)
#define PF_DAY      4                 // flags bits 2..7: days lived in this life stage (growing up, like your own household)

extern Fam pop_fam[NFAM];
extern Per pop_per[PER_MAX];
extern int pop_nper, pop_nplace;
extern Place pop_place[];

void pop_new(u32 seed, void *scratch, u32 scratch_bytes);   // a new life: everyone, from the seed (after bm_new)
int  pop_family_at(int bx, int by, int unit);               // who lives in that home (-1: nobody)
void pop_family_kinds(int f, u8 *adults, u8 *kids, u8 *teens, u8 *elders, u8 *couple);
int  pop_where(int p, int *place);                          // AT_*; place: the place index (work, school, out), else -1
int  pop_want(int p);                                       // 0 out, 1 home awake, 2 asleep (what citizens.c's day uses)
s8   pop_offset(int p);                                     // minutes early or late (no household moves as one)
const char *pop_first(int p, char *out);                    // their first name
const char *pop_surname(int f, char *out);
const char *pop_kind_name(int k);                           // "OLD TIMER", "ROOMMATES", ...
const char *pop_trait_name(int t);
const char *pop_job_name(int p, char *out);                 // "COOK AT THE DIPPER DINER", "RETIRED", "STUDENT"
const char *pop_doing(int p, char *out);                    // "ASLEEP AT HOME", "AT WORK: BOREMART", "AT ELM PARK"
int  pop_stage_of(int p);
void pop_tick(void);                                        // once a frame in SAN BOREAS: bring people in the window to life
int  pop_count_where(int at);                               // (directory) how many people are asleep / home / at work ...
extern int pop_aging;                                       // (options) the rest of SAN BOREAS grows older too
void pop_midnight(void);                                    // a day older all over SAN BOREAS: birthdays, babies, the old passing, new families
int  pop_alive(int f);                                      // how many of a household still live there
void pop_save(void);                                        // the memory card: what changed since the world began (save.h)
void pop_load(void);                                        // (after pop_new from the same seed)
