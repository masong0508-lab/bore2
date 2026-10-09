// sim: the people on the lot and the life-sim loop (needs, the clock, money, the job), BORE's rules.
#pragma once
#include "common.h"
#include "look.h"
#include "voxel.h"

#define MAX_SIMS 6
#define NEED_N 8
enum { N_FOOD, N_WC, N_REST, N_CLEAN, N_COMFY, N_FUN, N_SOCIAL, N_ROOM };
enum { ACT_NONE, ACT_EAT, ACT_WC, ACT_SLEEP, ACT_SHOWER, ACT_SIT, ACT_FUN, ACT_TALK, ACT_WORK, ACT_WALK, ACT_COFFEE, ACT_READ, ACT_RUN, ACT_PHONE, ACT_TRAVEL };

typedef struct {
    Bore bore;
    int  x, z;              // world position
    int  yaw;               // 0..4095
    Gait gait;
    int  need[NEED_N];      // 0..100 x 1024
    int  mood;              // -100..100
    int  action, act_t, act_x, act_y;   // what it does, steps left, the item tile
    int  ai;                // 1: lives its own life
    int  think;             // thought bubble: a need (0..7) or -1
    int  think_t;
    // pathing
    u8   path_n, path_i;
    u8   path_x[64], path_y[64];
    int  wander_t;
    int  talk_to;           // the sim it is talking to (-1)
    char note[24]; int note_t;   // a floating note over the head
    int  hurt_t;
    int  skill_cook, skill_body, skill_fun;
    int  dead;
    u8   grump;                 // the housemate who will NOT have a thing floating over their head
    // wants & fears (wants.c)
    struct { u8 kind, arg; u16 pts, age; } want[4], fear[2];
    int  asp_pts;               // the aspiration meter 0..12000
    s8   rel[MAX_SIMS];         // the daily relationship with each housemate, -100..100 (friend at 50)
    u8   act_var;               // which pie-menu option started the action (0 the plain one)
    struct { u8 id; s8 val; u16 t; } ml[6];   // MOODLETS (Sims 3): id, mood points, game minutes left
} Sim;

extern Sim  sims[MAX_SIMS];
extern int  nsims, player;
extern int  clock_min, clock_day;    // game time: minutes 0..1439, MON..SUN
extern long cash;
extern int  job_level, job_worked, job_days_good, job_strikes;
extern int  sim_speed;               // 1 normal, 2 fast, 3 ultra
extern int  bills_due;
extern char last_note[32]; extern int last_note_t;
extern int  travel_asked;    // the player used a boat or bus stop: main opens the travel menu

void sim_init(void);
void sim_add(const Bore *b, int mx, int my, int ai);
void sim_step(void);                       // one logic step (60 per second)
void sim_player_move(int dx, int dz, int run);   // world-space push from the stick, -4096..4096 each
int  sim_pie_open(char opt[8][16]);       // X: the pie menu for what is in front (0 = nothing there)
void sim_pie_do(int i);                    // the option picked
const char *sim_pie_title(void);           // whose / what menu it is
int  sim_traits(const Sim *s, const char *out[3]);   // Sims 3 traits read off the personality: how many (up to 3)
const char *sim_moodlet_name(int id);
void sim_moodlet(Sim *s, int id);          // give a moodlet (re-giving refreshes it)
int  sim_player_interact(void);            // X: use the thing in front, talk to the person in front. Returns 1 if something started
void sim_player_cancel(void);              // B: get up
void sim_player_cancel_sim(Sim *s);
const char *sim_hint(void);                // what X would do right now
void sim_note(const char *s);
int  sim_tile_x(const Sim *s); int sim_tile_y(const Sim *s);
const char *sim_day_name(int d);
const char *sim_need_name(int n);
int  sim_need_val(const Sim *s, int n);    // 0..100
int  sim_awake(const Sim *s);
void sim_switch_player(void);
int  sim_find_path(Sim *s, int tx, int ty);

// the story: what this life has done so far (the travel zones unlock from these)
extern int story_paid;         // paychecks earned
extern int story_talks;        // chats the player started
extern u8  story_visit[3];     // zones set foot in
