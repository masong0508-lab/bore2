// sim: the people on the lot and the life-sim loop (needs, the clock, money, the job), BORE's rules.
#pragma once
#include "common.h"
#include "look.h"
#include "voxel.h"

#define MAX_SIMS 8
#define NEED_N 8
enum { N_FOOD, N_WC, N_REST, N_CLEAN, N_COMFY, N_FUN, N_SOCIAL, N_ROOM };
enum { ACT_NONE, ACT_EAT, ACT_WC, ACT_SLEEP, ACT_SHOWER, ACT_SIT, ACT_FUN, ACT_TALK, ACT_WORK, ACT_WALK, ACT_COFFEE, ACT_READ, ACT_RUN, ACT_PHONE, ACT_TRAVEL, ACT_STAIRS, ACT_SWIM, ACT_CARE,
       ACT_GATHER };   // (GATHER: the island's wood, stone, leaves, food, and its secrets: act_var says which, island.h G_)

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
#define SKL(raw) (((raw) / 400) > 10 ? 10 : ((raw) / 400))   // a skill counter -> its level 0..10
    int  skill_logic, skill_char, skill_creat, skill_mech, skill_farm;
    u8   tod_walk, tod_talk, tod_potty, tod_story;   // a toddler's lessons: walking, talking, the potty, stories (0..100 each)
    u8   plant, plant_d, plant_t, plant_sk, plant_hc;   // FARM LIFE: 0 a person, 1 a living STALK (cannot move or talk, fades), 2 PLANTBORE; days as a stalk, spoken to today, the old skin and hair   // LOGIC (reading, study, lectures, the stars), CHARISMA (chats), CREATIVITY (art, music), MECHANICAL (rockets, repairs)
    int  skill_band[5];     // SUPERSTAR: voice, guitar/bass, keys, drums/percussion, dancing (counters like the others: 400 a level, 0..10)
    int  dead;
    u8   rage, rage_ko;      // after you in a fight (citizens.c); knocked down by you
    u8   grump;                 // the housemate who will NOT have a thing floating over their head
    // wants & fears (wants.c)
    struct { u8 kind, arg; u16 pts, age; } want[4], fear[2];
    int  asp_pts;               // the aspiration meter 0..12000
    s8   rel[MAX_SIMS];         // the daily relationship with each housemate, -100..100 (friend at 50)
    u8   act_var;               // which pie-menu option started the action (0 the plain one)
    u8   lvl;                   // the storey it stands on (0 the ground .. 4)
    struct { u8 id; s8 val; u16 t; } ml[6];   // MOODLETS (Sims 3): id, mood points, game minutes left
    // the living walk (sim.c follow_path): a person does not ride rails from tile to tile
    u8   dest_x, dest_y;    // where the walk is really going (a path is cut to 64 waypoints, so it is planned again from here)
    u8   repath;            // plans used up on this walk
    u8   wseed;             // this walk's dice: the pace, the lane, the wobble
    u8   pause_t;           // steps left of standing still to look about
    u8   stall;             // steps spent held up (a person, a wall)
    u8   hurry;             // 0 stroll, 1 brisk, 2 desperate
    u8   mind;              // 1: this walk has already been reconsidered once (a stroll is dropped for a need at most once)
    u16  look_yaw;          // what it faces while it pauses
    s16  spd;               // walk speed now, in 1/100 units a step (it ramps up and eases off)
    // growing up: the life stages, the family tree, school
    u8   uid;               // who it is in the family tree (1..255)
    u8   par[2], partner;   // the parents' and the partner's uids (0: none)
    u8   age_days;          // days lived in this life stage
    u8   bday;              // 1: today is their birthday (a cake makes it a party)
    u8   away;              // 1: at school (off the lot until 3 PM)
    u8   grade;             // school: 0..100 (85 an A)
    u8   homework;          // 1: today's homework is done
    u8   errand;            // (the AI) what it is walking over to do: 1..5 look after a baby
} Sim;

extern Sim  sims[MAX_SIMS];
extern int  nsims, player;
extern int  clock_min, clock_day;    // game time: minutes 0..1439, MON..SUN
// REAL TIME (OPTIONS > GAME): the clock follows the real one. Minutes since 1 JAN 2000, local time (0: not known: the PS1 until SET CLOCK)
extern int sim_rt, rt_on, rt_hz, rt_vb0; extern u32 rt_last, rt_base;
u32  sim_rt_now(void); void sim_rt_set(int on);
int  rt_mdays(int y, int m); void rt_date(int dn, int *y, int *m, int *d);   // (month 0 JAN; days since 1 JAN 2000)
extern long cash;
extern int  job_level, job_worked, job_days_good, job_strikes;
extern int  phone_tier, handheld;
extern int  sim_want_jobs, job_none, sim_want_arcade;
int  dojo_session(void);                      // the dojo class on now (0..2), -1 none
enum { BS_VOICE, BS_GTR, BS_KEYS, BS_DRUM, BS_DANCE, BS_N };
extern const char *const band_skill_name[BS_N];
int  dojo_belt(const Sim *s);                // WHITE 0 .. BLACK 5 (the body skill)
int  sim_grave_count(void);
int  sim_grave_spot(int i, int *mx, int *my);   // a departed Bore's gravestone, where it is in the map now (0: not in this place)
void sim_grave_named(int i, char *out);
void sim_say_to(Sim *s, const char *t);      // a word over their head
int  sim_move_out(int i, char *where);   // a grown-up of the household moves to a home of their own in town (1: gone)   // the phone you own (0 none, 1 FLIP, 2 SMARTPHONE, 3 BORE PHONE MAX) and the handhelds (1 BOREBOY, 2 BOREBOY ADVANCE)
// VEHICLES: bought at gas stations, ridden outdoors (O cycles the board and what you own), fuelled at the pumps
enum { VEH_BIKE, VEH_SCOOTER, VEH_SEDAN, VEH_VAN, VEH_SPORTS, VEH_PICKUP, VEH_N };
extern unsigned char fm_fert, fm_fz, fm_pest, fm_up, fm_wc, fm_ordk, fm_ordq, fm_ordt, fm_fair, fm_n[4], fm_g[4], fm_wet, fm_dry[4], fm_hens, fm_cows, fm_fed, veh_cond[VEH_N], art_n, novel; extern unsigned short fm_stock[4], fm_egg, fm_milk, fm_pe, fm_pm; extern unsigned short art_q, royalty;
// FARM LIFE, deeper (save version 43): how happy the hens and the cows are (0..100: fed and petted every day, they lay and give more; sad, nothing),
// petted today (1 hens, 2 cows), what the farm has sold (dollars, all its life) and to how many townsfolk at the stand
extern unsigned char fm_hh, fm_ch, fm_pet; extern unsigned short fm_buyers; extern int fm_earned;
extern const char *const fmGood[6]; extern const unsigned char fmPr[6];   // WHEAT CORN TOMATOES PUMPKINS EGGS MILK, and each one's price
int  fm_need(int k);                       // days a crop needs to grow
int  fm_in_season(int k);                  // 1: that crop loves this season (it grows every day; out of its season every other day; winter: frost)
int  fm_have(int k);                       // how many of farm good k there are to sell
int  fm_total(void);                       // all of them
int  farm_going(void);                     // 1: the family farms (something planted, an animal, or the job)
const char *farm_customer(int *sold);      // (citizens.c) a townsperson at the farm stand buys a few of something: what they say
void farm_save(void); void farm_load(void);
extern int  veh_own, veh_on, veh_sel, veh_fuel[VEH_N], veh_wph;   // owned (a bit each), ridden (0 none, else type + 1), the last one, tanks (quarter tiles), the wheels' turn
int  veh_seat(int t); int veh_colour(int t); const char *veh_hud(void);   // how high the driver sits; the paint; "SEDAN  FUEL 73%" for the screen
int  sim_ride_after_board(void);     // stepped off the board: on to a vehicle you own (1) or onto your feet (0)
extern int  has_board, on_board, board_lift;   // (board_lift: how high above the floor the rider is, on a ramp or in the air)
//     // a skateboard owned / being ridden (O toggles)
extern int  sim_speed;               // 1 normal, 2 fast, 3 ultra
extern int  bills_due;
extern char last_note[32]; extern int last_note_t;
extern int  travel_asked;    // the player used a boat or bus stop: main opens the travel menu
extern int  aging, old_age;  // (options) growing older: 0 off, 1 short lives, 2 normal, 3 long; old age: 1 elders pass on in the end
extern int  sim_cheer;       // a birthday just happened: that Bore's index + 1 (minus that: a baby or a child came home); main draws the confetti, then clears it
extern char sim_end_name[NAME_LEN];   // the family's surname, when its story is over
extern int  sim_story_over;  // the last of the family is gone
extern int  baby_due;        // life minutes when a baby comes home (-1: none)

void sim_init(void);
void sim_add(const Bore *b, int mx, int my, int ai);
void sim_step(void);                       // one logic step (60 per second)
void sim_player_move(int dx, int dz, int run);   // world-space push from the stick, -4096..4096 each
const char *sim_life_text(const Sim *s, int full);   // the life stage in words (toddler, infant, plant greying), with the days into it when full
int  sim_is_toddler(const Sim *b);
int  sim_pie_open(char opt[8][24]);       // X: the pie menu for what is in front (0 = nothing there)
void sim_pie_do(int i);                    // the option picked
const char *sim_pie_title(void);           // whose / what menu it is
int  sim_traits(const Sim *s, const char *out[3]);   // Sims 3 traits read off the personality: how many (up to 3)
const char *sim_moodlet_name(int id);
void sim_gotchi_mood(int simi, int kind);   // (gotchi.c) a moodlet from the BOREAGOTCHI toy
extern u8 col_lvl, col_major, col_on, col_cls, col_gpa, col_study, col_g[3]; extern u16 astro_xp; const char *col_major_name(int m); int space_cert(void); const char *space_cert_name(int c);
extern int sim_want_resort;   // a pack's screen wanted: pack*4 + arg + 1 (0 PARADISE, 1 FARM, 2 SKYSEA)
 extern u8 pk_data[48];   // ISLAND PARADISE (and room for packs to come): a few saved bytes a pack keeps for itself
extern int sim_want_space, space_done; extern u8 space_tech[3];   // the space program: parts (hull, engine, guidance), missions done
extern int sim_want_prison, rap_sheet; void sim_skip_days(int n);   // HARD TIME
enum { CR_ASSAULT, CR_THEFT, CR_RESIST, CR_JAILBREAK, CR_N };   // the crimes on your record, waiting for a court
extern u8 crime_n[CR_N]; extern const char *const crime_nm[CR_N]; extern const u8 crime_sev[CR_N]; extern int crime_evid, crime_def, arrest_how, sim_want_end;
int crime_total(void); int crime_points(void); void sim_crime(int kind, int evid); void sim_arrest(int how);
extern int sim_want_dog;   // 1: open the dog maker
extern int sim_want_bus, bus_pass, pet_kind, pet_col, pet_hun, pet_x, pet_z, pet_yaw, pet_ph, pet_zone;   // the bus pass (good while day_count < bus_pass) and the pet
extern int sim_want_super, sim_want_party, sim_want_dj, dj_xp, dj_heat;   // (the DJ booth) its hidden skill, the set's heat
void sim_dj_hit(int tip);
extern int sim_want_party_unused;   // (the PARTY pack) 1: open the guest list
void sim_party_over(int score, int guests);   // (the party pack) how it went
void sim_gotchi_fun(int simi, int amt);
int  sim_gotchi_stat(int simi, int what);     // 0 mood, 1 food 0..100, 2 rest
void sim_gotchi_food(int simi, int amt);
void sim_gotchi_absorb(int simi);             // the toy has you: needs drain while you stare at it
void sim_moodlet(Sim *s, int id);          // give a moodlet (re-giving refreshes it)
void sim_proud(Sim *s);                    // the PROUD moodlet (the wants meter reaching platinum)
int  sim_player_interact(void);            // X: use the thing in front, talk to the person in front. Returns 1 if something started
void sim_player_cancel(void);              // B: get up
void sim_player_cancel_sim(Sim *s);
const char *sim_hint(void);                // what X would do right now
void sim_note(const char *s);
int  sim_tile_x(const Sim *s); int sim_tile_y(const Sim *s);
const char *sim_day_name(int d);
int sim_weather(void);                     // 0 clear 1 rain 2 hail 3 snow: a few hours some days, by the season
int sim_outdoors(const Sim *s);            // standing out of doors
extern int sim_season_lock, sim_seasons_on;
int sim_season(void);                      // 0 SPRING 1 SUMMER 2 AUTUMN 3 WINTER: ten days each
const char *sim_season_name(int s);
int sim_festival(void);                    // 1 on a season's last day, from 10 AM to 10 PM: the town festival
const char *sim_festival_name(int s);
const char *sim_need_name(int n);
int  sim_need_val(const Sim *s, int n);    // 0..100
int  sim_awake(const Sim *s);
void sim_switch_player(void);
void sim_set_player(int i);   // SWITCH SIM: take over anyone playable
int  sim_playable(int i);
extern u8 sim_hold;           // bits: housemates told to wait (no free will)
void sim_player_tank(int fwd, int turn, int run);   // first person: walk (4096 forward, negative back) and turn by turn
int  sim_cell(int what);               // the phone in your pocket: 0 call a friend, 1 order a pizza, 2 adopt. 1: done
void sim_baby_coming(int p0, int p1, int minutes);   // (stories) a baby for those two (uids) comes home in that many minutes
int  sim_play_as(int i);               // play as that one of the family. 1: done
int  sim_find_path(Sim *s, int tx, int ty);
int  sim_walk(Sim *s, int dx, int dz);       // a step with collisions (on a board: over ramps and decks)
void sim_shift(int dtx, int dty);           // SAN BOREAS: the map window moved by that many tiles
void sim_take_job(int car, int bx, int by, const char *place);   // the player takes a job (places.h careers)
int  sim_front_service(void);               // a counter, a till, a machine or your work right in front: X goes to it before the people near it
int  sim_stage_len(int stage);               // days in a life stage (0: forever)
const char *sim_relation(int i, int j);      // what j is to i: "PARTNER", "PARENT", "CHILD", "SIBLING", "GRANDPARENT"... (0: nothing)
const char *sim_grade(int g);                // "A" .. "F"
int  sim_grave_name(int mx, int my, char *out);   // the name on the gravestone there
extern int day_count;                       // days since this life began
void sim_save(void);                        // the memory card: the family, the clock, the money, the job (save.h)
void sim_load(const Bore *blank);          // (after sim_init)

// the story: what this life has done so far (the travel zones unlock from these)
extern int story_paid;         // paychecks earned
extern int story_talks;        // chats the player started
extern u8  story_visit[4];     // zones set foot in
