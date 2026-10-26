// band: SUPERSTAR's band (the pack's saved state lives in the game; its screens are the overlay SUPER.OVL, super_ui.c).
// Up to 15 members, each with up to two roles (a member can switch roles, or play two parts, at a cost). The five band skills are on every Bore
// (sim.h skill_band): VOICE, GUITAR AND BASS, KEYBOARDS, DRUMS, DANCING. How well a part of a song is played comes from the skill of whoever plays it.
#pragma once
#include "common.h"
#include "sim.h"

#define BAND_MAX 15
#define BAND_SET 12
enum { RL_NONE, RL_FRONT, RL_LEADV, RL_BACKV, RL_RAPPER, RL_LEADG, RL_RHYG, RL_BASS, RL_KEYS, RL_SYNTH, RL_STRINGS, RL_DRUMS, RL_PERC, RL_DJ,
       RL_DANCER, RL_CREW, RL_HYPE, RL_LEADER, RL_ROADIE, RL_MANAGER, RL_N };
typedef struct { u8 kind; u16 ref; u8 role[2]; u8 sk[5]; } BandMem;   // kind: 0 empty, 1 a Bore of the family (ref: its uid), 2 anyone else (ref: pop's pid, or 0x8000 | a citizen's slot)
typedef struct {
    char name[17]; u8 genre, logo, tone, made;      // made: the band exists
    u16 fame, fans; u8 gigs, pad;
    BandMem m[BAND_MAX];
    u8 set[BAND_SET];                              // the setlist: indexes into the 25 band songs (255 empty)
} Band;
extern Band band;

const char *band_role_name(int r);
const char *band_role_blurb(int r);
int  band_role_skill(int r);                       // the skill a role plays (BS_*), or -1 for the crew
void band_reset(void);
int  band_count(void);
int  band_add(int kind, int ref);                  // -1 if full or already in
void band_remove(int i);
void band_member_name(int i, char *out);
int  band_level(int i, int skill);                 // 0..10
void band_train(int i, int skill, int amt);        // amt in family points (400 a level); others learn a sixteenth of it
int  band_quality(int skill);                      // the part of one skill as the whole band plays it, 0..256 (a missing player is a backing track)
int  band_dance(void);                             // the stage show, 0..256
int  band_leader(void);                            // a member with the LEADER role (or -1)
void band_save(void);
void band_load(int ver);
