// sim.c - the household and the life: BORE's needs, clock, bills and job, with free 3D movement and a path finder for the others.
#include "sim.h"
#include "band.h"
#include "wants.h"
#include "arcade.h"
#include "fight.h"
#include "realm.h"
#include "world.h"
#include "items.h"
#include "audio.h"
#include "gfx.h"
#include "places.h"
#include "bigmap.h"
#include "crowd.h"
#include "traffic.h"
#include "gotchi.h"
#include "pop.h"
#include "home.h"
#include "island.h"
#include "save.h"
#include "story.h"
#include "citizens.h"
#include "skate.h"

Sim  sims[MAX_SIMS];
int  sim_want_party;
int  nsims, player;
int  clock_min = 8 * 60, clock_day = 0;
long cash = 15000;
int  job_level = 0, job_worked = 0, job_days_good = 0, job_strikes = 0;
int  sim_speed = 1;
int  bills_due = 800;
int  job_none;   // no job yet (a new life starts so, as on the GBA): no shifts, no pay, and no bills until you are hired
char last_note[32]; int last_note_t;
int sim_need_pct = 100, sim_day_pct = 100, sim_msg_t = 180;   // OPTIONS > GAME: how fast the needs fall, how long a day lasts, how long a message stays (percent, percent, steps)
int  travel_asked;
int  story_paid, story_talks;
u8   story_visit[4];
int  day_count;                 // days since this life began
// growing up: how long each life stage lasts (days; by the AGING option: off, short, normal, long), the coming baby, the cake, the graves
int  aging = 2, old_age = 1, sim_cheer, sim_story_over, baby_due = -1;
static u8 baby_par[2], baby_kind;   // the coming baby's parents' uids; 1 an adopted baby, 2 an adopted child (0: born to them)
static u8 next_uid = 1;
static int cake_slices;             // slices left on the birthday cake (0: there is none)
static struct { u8 x, y, city; char name[NAME_LEN]; } graves[6]; static int ngraves;   // (city: x, y on the home's block, not the window)
static const u8 stLen[4][AG_N] = { { 0, 0, 0, 0, 0 }, { 2, 3, 3, 9, 5 }, { 4, 6, 6, 18, 9 }, { 8, 12, 12, 36, 18 } };   // (a baby: the first half infant, the second a toddler)
int sim_stage_len(int stage) { return stLen[aging & 3][stage % AG_N]; }
enum { CARE_FEED = 1, CARE_CHANGE, CARE_PLAY, CARE_CUDDLE, CARE_BED, CARE_STORY, CARE_BATH, CARE_WALK, CARE_TALK, CARE_POTTY, CARE_COMFORT, CARE_HWHELP, CARE_FEELINGS, CARE_END };
static void bake_done(Sim *s); static void cake_eaten(Sim *s); static void homework_done(Sim *s); static void care_done(Sim *s);
static void baby_to_bed(Sim *b); static int baby_need(const Sim *b);
static int school_age(const Sim *k); static void care_start(Sim *s, int baby, int kind);
static void gather_done(Sim *s); static void raft_sail(void);
int  life_minutes(void) { return day_count * 1440 + clock_min; }

// BORE tuning (sims.h), per logic step in 1/1024 points
#define STEPS_MIN 15
#define RATE_FOOD 6
#define RATE_WC 5
#define RATE_REST 7
#define RATE_CLEAN 5
#define RATE_COMFY 6
#define RATE_FUN 5
#define RATE_SOCIAL 3
#define GAIN_SLEEP 175
#define GAIN_WASH 400
#define GAIN_SIT 300
#define GAIN_EAT 500
#define GAIN_WC 700
#define GAIN_FUN 250
#define GAIN_TALK 300
#define NIGHT_FROM 1320
#define NIGHT_TO 360
#define BILLS 800
#define SIM_RADIUS 44
#define WALK_SPEED 9      // units per step at 100%
static const u8 stSpd[5] = { 50, 80, 95, 100, 70 };
static const char *const dayNm[7] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
static const char *const needNm[NEED_N] = { "FOOD", "WC", "REST", "CLEAN", "COMFY", "FUN", "SOCIAL", "ROOM" };
const char *sim_day_name(int d) { return dayNm[d % 7]; }
int sim_outdoors(const Sim *s) { int mx = s->x / TILE, my = (-s->z) / TILE; return s->x >= 0 && s->z <= 0 && mx < MW && my < MH && floorMap[my][mx] >= FL_GRASS; }
int sim_weather(void) {
    u32 h = (u32)day_count * 2654435761u; h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
    int se = sim_season(), roll = (int)(h % 100), start = 5 + (int)((h >> 8) % 12), len = 3 + (int)((h >> 16) % 6), hr = clock_min / 60;
    int w = se == 3 ? (roll < 60 ? 3 : 0) : se == 1 ? (roll < 12 ? 1 : roll < 20 ? 2 : 0) : (roll < 45 ? 1 : (se == 0 && roll < 50) ? 2 : 0);
    return (w && hr >= start && hr < start + len) ? w : 0;
}
int sim_season_lock, sim_seasons_on = 1;   // the editor / CREATE A WORLD: 0 the year turns, 1..4 held on spring .. winter; the SEASONS pack off: summer all year
// REAL TIME (OPTIONS > GAME): the clock is your own. Real time is counted in minutes since 1 JAN 2000 (local time; 0: not known).
// The Dreamcast and the DS read their clock chips; the PlayStation has none, so the player sets it (main.c, SET CLOCK) and the
// vblanks count on from there while the console is on.
int sim_rt, rt_on, rt_hz = 60, rt_vb0; u32 rt_last, rt_base;   // the option; it is working now; the PS1's vblank rate, the vblank and the minute it was set at; the real minute the clock was last put to (saved)
s8 rt_mon = -1; u8 rt_mday; static int rt_dayn = -1;  // the real date: the month (0 JAN), the day
static int rt_leap(int y) { return !(y & 3) && (y % 100 || !(y % 400)); }
int rt_mdays(int y, int m) { return m == 1 ? 28 + rt_leap(y) : 31 - ((m + (m >= 7)) & 1); }
COLD void rt_date(int dn, int *y, int *m, int *d) { int yy = 2000, mm = 0; while (dn >= 365 + rt_leap(yy)) dn -= 365 + rt_leap(yy++); while (dn >= rt_mdays(yy, mm)) dn -= rt_mdays(yy, mm++); *y = yy; *m = mm; *d = dn + 1; }
u32 sim_rt_now(void) {
#ifdef BORE_DC
    { extern u32 dc_rtc_min(void); u32 m = dc_rtc_min(); if (m) return m; }   // (the Dreamcast's own clock)
#endif
    return rt_base ? rt_base + (u32)(VSync(-1) - rt_vb0) / (u32)(rt_hz * 60) : 0;
}
static void rt_put(u32 m) {   // the clock to that real minute (the weekday and the date with it)
    int dn = (int)(m / 1440); clock_min = (int)(m % 1440); clock_day = (dn + 5) % 7;   // (1 JAN 2000 was a SATURDAY)
    if (dn != rt_dayn) { int y, mo, d; rt_dayn = dn; rt_date(dn, &y, &mo, &d); rt_mon = (s8)mo; rt_mday = (u8)d; }
}
void sim_rt_set(int on) { if (on && !sim_rt) rt_last = 0; sim_rt = on; if (!on) rt_on = 0; }   // (switched on: straight to the real time, nothing caught up)
int sim_season(void) { return sim_season_lock ? sim_season_lock - 1 : !sim_seasons_on ? 1 : rt_on && rt_mon >= 0 ? (rt_mon + 10) / 3 % 4 : (day_count / 10) & 3; }   // (REAL TIME: by the month, MARCH spring .. DECEMBER winter)
const char *sim_season_name(int s) { static const char *const n[4] = { "SPRING", "SUMMER", "AUTUMN", "WINTER" }; return n[s & 3]; }
static int fest_day(void) { return rt_on && rt_mon >= 0 ? clock_day == 5 && rt_mday <= 7 : day_count % 10 == 9; }   // (REAL TIME: the month's first SATURDAY)
int sim_festival(void) { return fest_day() && clock_min >= 600 && clock_min < 1320; }
const char *sim_festival_name(int s) { static const char *const n[4] = { "FLOWER FAIR", "SUMMER CARNIVAL", "HARVEST FEAST", "WINTER LIGHTS" }; return n[s & 3]; }
const char *sim_need_name(int n) { return needNm[n]; }
int sim_need_val(const Sim *s, int n) { return s->need[n] >> 10; }
int sim_tile_x(const Sim *s) { return s->x / TILE; }
int sim_tile_y(const Sim *s) { return (-s->z) / TILE; }
int sim_awake(const Sim *s) { return s->action != ACT_SLEEP; }
char note_q[4][32]; int note_qn;   // the notes the feed has not shown yet (main takes them every frame)
void sim_note(const char *s) {
    strncpy(last_note, s, 31); last_note[31] = 0; last_note_t = sim_msg_t;
    if (note_qn == 4) { memmove(note_q[0], note_q[1], 3 * 32); note_qn = 3; }
    strcpy(note_q[note_qn++], last_note);
}
static void sim_say(Sim *s, const char *t) { strncpy(s->note, t, 23); s->note[23] = 0; s->note_t = sim_msg_t * 5 / 6; }

// ---- integer atan2 (4096 = a turn) ----
// ================================================================== MOODLETS (Sims 3) and TRAITS
enum { ML_NONE, ML_GOOD_MEAL, ML_SNACKED, ML_RESTED, ML_FRESH, ML_GOOD_CHAT, ML_FRIEND, ML_PROMOTED, ML_PAYDAY, ML_EMBARRASSED,
       ML_PASSED_OUT, ML_WET, ML_REJECTED, ML_TEASED, ML_HUGGED, ML_FUN_TIME, ML_GOT_A_LAUGH, ML_FLIRTY, ML_WORKOUT, ML_CAFFEINE,
       ML_HUNGRY, ML_TIRED, ML_STINKY, ML_BURSTING, ML_LONELY, ML_BORED, ML_SINGING, ML_PIZZA,
       ML_ATE_OUT, ML_SWAM, ML_OUTFIT, ML_RETAIL, ML_NEW_GAME, ML_NEW_JOB, ML_COACHED, ML_HOTEL,
       ML_FINE_DINING, ML_FILM, ML_CUT, ML_NIGHT_OUT, ML_ROUND, ML_PAMPERED, ML_INSPIRED, ML_HEALTHY, ML_TREAT,
       ML_BIRTHDAY, ML_NEW_BABY, ML_LOVE, ML_PROUD, ML_GRIEF, ML_MEMORIES, ML_CAKE, ML_GRADES, ML_HOMEWORK, ML_SCHOOL, ML_BLOB_BUDDY, ML_BLOB_HATCH, ML_BLOB_PROUD, ML_BLOB_WORRY, ML_BLOB_MISS, ML_BLOB_HOOKED, ML_BLOB_SHARED, ML_PARTY_OK, ML_PARTY_GREAT, ML_PARTY_LEGEND, ML_DJ_HIT, ML_EYES, ML_ABDUCT, ML_TANTRUM, ML_MILESTONE, ML_BLOOM, ML_DORMANT, ML_SUNBURN, ML_TAN, ML_HARVEST, ML_HERD, ML_CROPLOST, ML_MARKET, ML_N };
static const struct { const char *nm; s8 val; u16 mins; } mlDef[ML_N] = {
    { "", 0, 0 }, { "GOOD MEAL", 12, 240 }, { "SNACKED", 4, 90 }, { "WELL RESTED", 12, 300 }, { "FRESH", 8, 180 }, { "GOOD CHAT", 10, 180 },
    { "MADE A FRIEND", 20, 480 }, { "PROMOTED", 25, 720 }, { "PAYDAY", 15, 360 }, { "EMBARRASSED", -12, 120 },
    { "PASSED OUT", -15, 240 }, { "WET MYSELF", -25, 180 }, { "TURNED DOWN", -10, 180 }, { "TEASED", -15, 180 }, { "HUGGED", 12, 180 },
    { "FUN TIME", 8, 120 }, { "GOT A LAUGH", 10, 120 }, { "FLIRTY", 14, 180 }, { "WORKED OUT", 10, 180 }, { "CAFFEINATED", 6, 90 },
    { "HUNGRY", -12, 0 }, { "TIRED", -10, 0 }, { "STINKY", -15, 0 }, { "BURSTING", -15, 0 }, { "LONELY", -12, 0 }, { "BORED", -8, 0 },
    { "SANG IT OUT", 9, 120 }, { "PIZZA NIGHT", 14, 240 },
    { "ATE OUT", 14, 300 }, { "GREAT SWIM", 12, 180 }, { "NEW OUTFIT", 14, 600 }, { "RETAIL THERAPY", 8, 180 }, { "NEW GAME", 10, 300 },
    { "NEW JOB", 15, 600 }, { "PUMPED UP", 12, 180 }, { "HOTEL LUXURY", 10, 300 },
    { "FINE DINING", 20, 360 }, { "GREAT FILM", 14, 300 }, { "FRESH CUT", 14, 720 }, { "NIGHT OUT", 12, 240 }, { "LIFE OF THE PARTY", 20, 240 },
    { "PAMPERED", 15, 360 }, { "INSPIRED", 10, 240 }, { "HEALTHY", 8, 1440 }, { "SWEET TREAT", 8, 120 },
    { "GREAT BIRTHDAY", 20, 720 }, { "NEW BABY", 20, 1440 }, { "IN LOVE", 15, 720 }, { "PROUD PARENT", 8, 240 }, { "GRIEF", -15, 1440 },
    { "FOND MEMORIES", 8, 240 }, { "BIRTHDAY CAKE", 8, 180 }, { "GOOD GRADES", 10, 720 }, { "HOMEWORK DONE", 6, 240 }, { "LONG DAY AT SCHOOL", -5, 180 },
    { "BLOB BUDDY", 10, 240 }, { "IT HATCHED!", 16, 480 }, { "PROUD BLOB PARENT", 14, 720 }, { "WORRIED ABOUT MY BLOB", -8, 120 }, { "MISSING MY BLOB", -15, 720 }, { "HOOKED ON THE TOY", -8, 240 }, { "SHARED MY LUNCH", 8, 240 }, { "NICE PARTY", 10, 480 }, { "GREAT PARTY", 18, 720 }, { "LEGENDARY PARTY", 30, 1440 }, { "CROWD PLEASER", 12, 360 }, { "EYES BURNED", -25, 720 }, { "ABDUCTED!", -8, 600 },
    { "TODDLER TANTRUM", -10, 180 }, { "A BIG MILESTONE", 14, 720 }, { "IN BLOOM", 12, 1440 }, { "DORMANT", -8, 1440 }, { "SUNBURNT", -12, 720 }, { "SUN KISSED", 8, 480 },
    { "GOOD HARVEST", 14, 480 }, { "HAPPY ANIMALS", 10, 360 }, { "LOST A CROP", -10, 480 }, { "SOLD AT MARKET", 12, 300 },   // (FARM LIFE)
};
const char *sim_moodlet_name(int id) { return mlDef[id % ML_N].nm; }
static int has_trait(const Sim *s, int t, int hi) { int v = s->bore.tr[t]; return hi ? v >= 8 : v <= 2; }
int sim_traits(const Sim *s, const char *out[3]) {
    static const char *const hiNm[TR_N] = { "NEAT FREAK", "SOCIAL BUTTERFLY", "ATHLETIC", "GOOFBALL", "GOOD" };
    static const char *const loNm[TR_N] = { "SLOB", "LONER", "COUCH POTATO", "SERIOUS", "MEAN" };
    int n = 0;
    for (int t = 0; t < TR_N && n < 3; t++) { if (has_trait(s, t, 1)) out[n++] = hiNm[t]; else if (has_trait(s, t, 0)) out[n++] = loNm[t]; }
    return n;
}
void sim_gotchi_mood(int i, int kind) { static const u8 m[7] = { ML_BLOB_BUDDY, ML_BLOB_HATCH, ML_BLOB_PROUD, ML_BLOB_WORRY, ML_BLOB_MISS, ML_BLOB_HOOKED, ML_BLOB_SHARED }; if (i >= 0 && i < nsims && kind >= 0 && kind < 7) sim_moodlet(&sims[i], m[kind]); }
int sim_gotchi_stat(int i, int what) { if (i < 0 || i >= nsims) return 0; return what == 0 ? sims[i].mood : what == 1 ? sims[i].need[N_FOOD] >> 10 : sims[i].need[N_REST] >> 10; }
void sim_gotchi_food(int i, int amt) { if (i >= 0 && i < nsims) sims[i].need[N_FOOD] = CLAMP(sims[i].need[N_FOOD] + (amt << 10), 0, 100 << 10); }
void sim_gotchi_absorb(int i) {   // staring at the toy: the owner forgets to eat, sleep and talk to anyone
    if (i < 0 || i >= nsims) return; Sim *s = &sims[i];
    s->need[N_FOOD] = MAX(0, s->need[N_FOOD] - (3 << 10)); s->need[N_REST] = MAX(0, s->need[N_REST] - (1 << 10)); s->need[N_SOCIAL] = MAX(0, s->need[N_SOCIAL] - (2 << 10)); s->need[N_CLEAN] = MAX(0, s->need[N_CLEAN] - (1 << 10));
    static int n; static const char *const fun[5] = { "YOUR TUMMY GROWLS. THE PET LOOKS WORRIED", "WHEN DID YOU LAST EAT? THE PET WONDERS TOO", "YOUR HOUSEMATES ARE WAVING AT YOU", "YOU HAVEN'T BLINKED IN A WHILE", "THE FRIDGE IS RIGHT THERE. JUST SAYING" };
    if ((s->need[N_FOOD] >> 10) < 25) { sim_moodlet(s, ML_BLOB_HOOKED); if (!(++n & 3)) sim_note(fun[(n >> 2) % 5]); }
}
static void crime_midnight(void);
static void farm_midnight(void);
int sim_want_space, space_done; u8 space_tech[3]; int sim_want_resort; u8 pk_data[48];
int dj_xp, dj_heat, sim_want_dj, sim_want_super, sim_want_bus, sim_want_dog, bus_pass, sim_want_prison, rap_sheet;   // sim_want_prison: a sentence in days (HARD TIME); rap_sheet: past arrests   // bus_pass: the pass is good while day_count < bus_pass
int pet_kind, pet_col, pet_hun = 100, pet_x, pet_z, pet_yaw, pet_ph, pet_zone = -1;   // the pet (0 none, 1 dog, 2 cat): where it is, how hungry
void sim_dj_hit(int tip) { cash += tip; if (nsims) sim_moodlet(&sims[player], ML_DJ_HIT); }
void sim_party_over(int score, int guests) {   // the last guest leaves: how it went, for everyone who lives here
    for (int i = 0; i < nsims; i++) { Sim *s = &sims[i]; if (s->away) continue;
        if (score >= 1) sim_moodlet(s, score >= 8 ? ML_PARTY_LEGEND : score >= 4 ? ML_PARTY_GREAT : ML_PARTY_OK);
        s->need[N_SOCIAL] = MIN(s->need[N_SOCIAL] + ((10 + MIN(guests, 30) + score * 3) << 10), 100 << 10);
        s->need[N_FUN] = MIN(s->need[N_FUN] + ((10 + score * 4) << 10), 100 << 10); }
}
void sim_gotchi_fun(int i, int amt) { if (i >= 0 && i < nsims) sims[i].need[N_FUN] = MIN(sims[i].need[N_FUN] + (amt << 10), 100 << 10); }
void sim_moodlet(Sim *s, int id) {
    int slot = -1;
    for (int i = 0; i < 6; i++) if (s->ml[i].id == id) slot = i;
    if (slot < 0) for (int i = 0; i < 6; i++) if (!s->ml[i].id) { slot = i; break; }
    if (slot < 0) { int lo = 0; for (int i = 1; i < 6; i++) if (s->ml[i].t < s->ml[lo].t) lo = i; slot = lo; }   // full: the one nearest to running out goes
    int v = mlDef[id].val;
    if (id == ML_FRESH && has_trait(s, TR_NEAT, 1)) v += 6;                   // NEAT FREAK: clean feels great
    if (id == ML_GOOD_CHAT && has_trait(s, TR_OUT, 1)) v += 6;                // SOCIAL BUTTERFLY
    if (id == ML_WORKOUT && has_trait(s, TR_ACT, 1)) v += 8;                  // ATHLETIC
    if (id == ML_GOT_A_LAUGH && has_trait(s, TR_PLAY, 1)) v += 8;             // GOOFBALL
    s->ml[slot].id = (u8)id; s->ml[slot].val = (s8)v; s->ml[slot].t = mlDef[id].mins ? mlDef[id].mins : 1;
}
static void moodlets_minute(Sim *s) {   // the timed ones run down; the need ones come and go with the need
    for (int i = 0; i < 6; i++) if (s->ml[i].id && mlDef[s->ml[i].id].mins) { if (s->ml[i].t) s->ml[i].t--; if (!s->ml[i].t) s->ml[i].id = 0; }
    static const struct { u8 need, ml, lim; } nm[6] = { { N_FOOD, ML_HUNGRY, 25 }, { N_REST, ML_TIRED, 20 }, { N_CLEAN, ML_STINKY, 22 }, { N_WC, ML_BURSTING, 20 }, { N_SOCIAL, ML_LONELY, 22 }, { N_FUN, ML_BORED, 20 } };
    for (int k = 0; k < 6; k++) {
        int lim = nm[k].lim;
        if (nm[k].ml == ML_STINKY && has_trait(s, TR_NEAT, 0)) lim = -1;       // SLOB: doesn't notice
        if (nm[k].ml == ML_LONELY && has_trait(s, TR_OUT, 0)) lim = -1;        // LONER: fine, thanks
        if (nm[k].ml == ML_LONELY && has_trait(s, TR_OUT, 1)) lim += 15;       // SOCIAL BUTTERFLY: lonely sooner
        int low = (s->need[nm[k].need] >> 10) < lim, at = -1;
        for (int i = 0; i < 6; i++) if (s->ml[i].id == nm[k].ml) at = i;
        if (low && at < 0) sim_moodlet(s, nm[k].ml);
        if (!low && at >= 0) s->ml[at].id = 0;
    }
}
static int moodlet_sum(const Sim *s) { int m = 0; for (int i = 0; i < 6; i++) if (s->ml[i].id) m += s->ml[i].val; return m; }

static int iatan2(int y, int x) {
    if (x == 0 && y == 0) return 0;
    int ax = ABS(x), ay = ABS(y);
    int a; // 0..512 for the first octant
    if (ax >= ay) a = (ay * 512) / (ax + (ay >> 1)); else a = 1024 - (ax * 512) / (ay + (ax >> 1));
    if (x < 0) a = 2048 - a;
    if (y < 0) a = -a;
    return a & 4095;
}
static int yaw_towards(int dx, int dz) { return iatan2(dx, -dz) & 4095; }   // yaw 0 faces south (-z): rotation x' = x cos + z sin
static void turn_to(Sim *s, int target, int rate) {
    int d = (target - s->yaw) & 4095; if (d > 2048) d -= 4096;
    if (d > rate) d = rate; if (d < -rate) d = -rate;
    s->yaw = (s->yaw + d) & 4095;
}

void sim_init(void) {
    gotchi_reset(); dj_xp = 0; dj_heat = 0; band_reset(); realm_reset();   // (a new life: no realm yet; a loaded one brings its own)
    pet_kind = 0; pet_zone = -1; pet_hun = 100; bus_pass = 0; rap_sheet = 0; space_done = 0; memset(space_tech, 0, 3); memset(pk_data, 0, sizeof pk_data); col_lvl = col_major = col_on = col_cls = col_gpa = col_study = 0; memset(col_g, 0, 3); astro_xp = 0; art_n = novel = 0; art_q = royalty = 0; fm_fert = fm_fz = fm_pest = fm_up = fm_wc = fm_ordk = fm_ordq = fm_ordt = fm_fair = 0; memset(fm_n, 0, 4); memset(fm_g, 0, 4); memset(fm_dry, 0, 4); memset(fm_stock, 0, sizeof fm_stock); fm_wet = fm_hens = fm_cows = fm_fed = 0; fm_egg = fm_milk = fm_pe = fm_pm = 0; fm_hh = fm_ch = 70; fm_pet = 0; fm_buyers = 0; fm_earned = 0; for (int t = 0; t < VEH_N; t++) veh_cond[t] = 100; memset(crime_n, 0, sizeof crime_n); crime_evid = crime_def = 0; dog_default(&pet_dog); nsims = 0; player = 0; clock_min = 8 * 60; clock_day = 0; day_count = 0; rt_last = 0; cash = 15000; last_note[0] = 0; note_qn = 0;
    job_level = 0; job_car = CAR_FACTORY; job_take(CAR_FACTORY, -1, -1, "THE FACTORY"); job_none = 1;
    groceries = 10; room_bx = room_by = -1; room_until = 0; phone_tier = 1; handheld = 0;
#ifdef AUTO_PHONE_TIER
    phone_tier = AUTO_PHONE_TIER; handheld = AUTO_PHONE_TIER == 2;   // (test builds: start with that phone)
#endif
    baby_due = -1; next_uid = 1; cake_slices = 0; ngraves = 0; sim_story_over = 0; sim_cheer = 0;
}

void sim_add(const Bore *b, int mx, int my, int ai) {
    if (nsims >= MAX_SIMS) return;
    Sim *s = &sims[nsims++];
    memset(s, 0, sizeof *s);
    s->bore = *b;
    s->x = mx * TILE + TILE / 2; s->z = -(my * TILE + TILE / 2);
    s->yaw = 0; s->ai = ai; s->talk_to = -1; s->think = -1; s->grump = 0; s->lvl = 0;
    for (int i = 0; i < MAX_SIMS; i++) s->rel[i] = 0;
    for (int i = 0; i < 6; i++) s->ml[i].id = 0;
    s->act_var = 0;
    wants_roll_all(s);
    for (int i = 0; i < NEED_N; i++) s->need[i] = (70 + (int)(look_rand() % 25)) << 10;
    s->need[N_ROOM] = 50 << 10;
    s->mood = 20;
    s->uid = next_uid++; if (!next_uid) next_uid = 1;
    s->grade = 60;
}
static int playable(int i) { return i >= 0 && i < nsims && sims[i].bore.stage != AG_BABY && !sims[i].away; }
u8 sim_hold;   // SWITCH SIM: a bit per housemate you have told to stay put while you play someone else (no free will)
int sim_playable(int i) { return playable(i); }
void sim_set_player(int n) {   // take over anyone in the household (not a baby, not someone at school)
    if (!playable(n) || n == player) return;
    sims[player].ai = 1; sims[player].path_n = 0; sims[player].path_i = 0;
    player = n; sims[player].ai = 0; sims[player].path_n = 0; sims[player].path_i = 0; sim_hold &= (u8)~(1u << n);
    if (sims[player].grump) sim_note("NO MARKER. MOSS SAID NO.");
}
void sim_switch_player(void) {
    if (nsims < 2) return;
    int n = player;
    for (int k = 1; k < nsims; k++) { int i = (player + k) % nsims; if (playable(i)) { n = i; break; } }   // (not a baby, not someone at school)
    if (n == player) return;
    sims[player].ai = 1;
    player = n;
    sims[player].ai = 0;
    sims[player].path_n = 0;
    if (sims[player].grump) sim_note("NO MARKER. MOSS SAID NO.");
}

// ---- walking with collision (the lot's tiles) ----
int phone_tier = 1, handheld;
int sim_want_arcade;   // an arcade cabinet (game + 1) or the prize counter: main opens it
int sim_want_jobs;   // the computer's JOB BOARD was picked: main opens JOBS with every opening   // (a new life: an old flip phone in the pocket; better ones from an electronics shop)
// ---- VEHICLES ----
int veh_own, veh_on, veh_sel, veh_fuel[VEH_N], veh_wph; static int veh_odo, veh_lowfuel, veh_wacc; u8 veh_cond[VEH_N] = { 100, 100, 100, 100, 100, 100 };   // (condition: wears with use, fixed by a mechanic or by you)
static const struct { const char *nm; short price, speed, tank, seat; } VT[VEH_N] = {   // tank: tiles of range (0: pedals); seat: how high the driver sits
    { "BIKE", 150, 3, 0, 0 }, { "SCOOTER", 600, 4, 500, 0 }, { "SEDAN", 3200, 7, 1500, 30 }, { "VAN", 4800, 6, 1800, 30 }, { "SPORTS CAR", 9500, 10, 1000, 24 }, { "PICKUP", 5600, 6, 2000, 30 } };
int veh_seat(int t) { return VT[t % VEH_N].seat * 7 / 4; }
int veh_colour(int t) { return (t * 5 + 2 + (int)((world_seed() >> 8) & 7)) % 6; }
int has_board, on_board, board_lift;   // the skateboard: owned, ridden, how high the rider is (skate.c rides it)
static int ramp_pass;   // set while the rider's own step is tried: a board can roll onto a ramp
int dbg_air_max, dbg_tricks;
static int skate_deck(char c) { return c == 'X' || c == 'M' || c == 'L' || c == 'N' || c == 'Z'; }
static int is_ramp(char c) { return c == '1' || c == '2' || c == '4' || c == '5' || c == '9'; }
static int ramp_g(int px, int pz, char c, int mx, int my) { (void)c; return items_ramp_at(mx, my, px - mx * TILE, (-pz) - my * TILE); }
static int footprint_free(int x, int z) {
    for (int k = 0; k < 4; k++) {
        int px = x + ((k & 1) ? SIM_RADIUS : -SIM_RADIUS), pz = z + ((k & 2) ? SIM_RADIUS : -SIM_RADIUS);
        int mx = px / TILE, my = (-pz) / TILE;
        if (px < 0 || pz > 0) return 0;
        if (ramp_pass && is_ramp(world_at(mx, my)) && ramp_g(px, pz, world_at(mx, my), mx, my) <= board_lift + 240) continue;   // (a steep transition rises fast ahead of the board)   // (from the low side, or in the air above it)
        if (ramp_pass && skate_deck(world_at(mx, my)) && items_height(world_at(mx, my)) <= board_lift + 24) continue;   // (up on a funbox, a ledge, a manual pad)
        if (!world_walkable(mx, my)) {
            char ic = world_at(mx, my);   // a thing in the way blocks only where it really stands, not the whole tile (a fridge leaves a margin, a fence a gap)
            if (!items_is_item(ic) || !items_blocking(ic)) return 0;
            int h = items_hit(ic, world_item_facing(mx, my), items_along(ic) ? world_along(mx, my, ic) : 0, x - mx * TILE, -z - my * TILE, SIM_RADIUS);
            if (h != 0) return 0;
        }
    }
    return 1;
}
int sim_climb_gear;   // 1: the gear to climb steep ground (nothing gives it yet: the expansion packs will)
static int steep_up(const Sim *s, int dx, int dz) {   // the hills: a rise of more than about 1 in 2.3 ahead is a cliff to a walker (a rider or a climber with gear goes up steeper)
    if (world_zone != ZONE_CITY || s->lvl || (dx | dz) == 0) return 0;
    int m = MAX(ABS(dx), ABS(dz)), n = MIN(ABS(dx), ABS(dz)), sx = dx * 64 / m, sz = dz * 64 / m;
    int rise = world_gh(s->x + sx, s->z + sz) - world_gh(s->x - sx, s->z - sz); if (rise <= 0) return 0;
    int run = 128 + n * 53 / m, lim = (sim_climb_gear || (s == &sims[player] && (veh_on || on_board))) ? 100 : 43;
    return rise * 100 > run * lim;
}
static int walk(Sim *s, int dx, int dz) {   // returns 1 if it moved at all
    int moved = 0; ramp_pass = on_board && s == &sims[player];
    if (steep_up(s, dx, dz)) { static int nt; if (s == &sims[player] && !(nt++ & 63)) sim_note("TOO STEEP TO CLIMB"); dx = dz = 0; }
    if (dx && footprint_free(s->x + dx, s->z)) { s->x += dx; moved = 1; }
    if (dz && footprint_free(s->x, s->z + dz)) { s->z += dz; moved = 1; }
    ramp_pass = 0;
    return moved;
}
static int speed_of(const Sim *s) {
    int sp = WALK_SPEED * stSpd[s->bore.stage % 5] / 100;
    if ((s->need[N_REST] >> 10) < 20) sp = sp * 80 / 100;
    return MAX(sp, 3);
}

// ---- path finding: BFS over the tiles, then pulled straight (a person cuts corners, they do not walk a staircase) ----
extern u16 bfs_q[];                 // (atlas.c's upload buffer, lent: also to the roof rebuild in world.c, never at the same time)
s16 bfs_par[MH][MW] __attribute__((aligned(8)));   // (also voxel.c's scratch while it shapes a person, and lent to the floor drawing and the roofs)
static u32 walk_tick;          // steps since the game began: the wobble and the dice run off it
#define LOS_R 60               // the clearance a straight line needs (a little more than the body)
#define LOS_STEP 96
#define LOS_WIN 12             // how many tiles ahead a shortcut may reach
static int footprint_r(int x, int z, int r) {
    for (int k = 0; k < 4; k++) {
        int px = x + ((k & 1) ? r : -r), pz = z + ((k & 2) ? r : -r);
        if (px < 0 || pz > 0 || !world_walkable(px / TILE, (-pz) / TILE)) return 0;
    }
    return 1;
}
static int los(int x0, int z0, int x1, int z1) {   // can a body walk the straight line?
    int dx = x1 - x0, dz = z1 - z0, d = MAX(ABS(dx), ABS(dz)), n = d / LOS_STEP + 1;
    for (int i = 1; i <= n; i++) if (!footprint_r(x0 + dx * i / n, z0 + dz * i / n, LOS_R)) return 0;
    return 1;
}
int sim_find_path(Sim *s, int tx, int ty) {
    int sx = sim_tile_x(s), sy = sim_tile_y(s);
    s->dest_x = (u8)tx; s->dest_y = (u8)ty;
    if (sx == tx && sy == ty) { s->path_n = 0; return 1; }
    if (sx < 0 || sy < 0 || sx >= MW || sy >= MH) return 0;
    memset(bfs_par, 0xff, sizeof bfs_par);   // (-1: not reached)
    int qh = 0, qt = 0;
    bfs_q[qt++] = (u16)(sy * MW + sx); bfs_par[sy][sx] = (s16)(sy * MW + sx);
    static const signed char dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
    int found = 0;
    // (a tile that cannot be walked is marked -2 the first time it is asked about: it used to be asked again from each of its sides,
    //  about a tenth of a long search's time in a city window; the same tiles, in the same order, come out)
    while (qh < qt && !found) {
        int c = bfs_q[qh++], cx = c % MW, cy = c / MW;
        for (int d = 0; d < 4; d++) {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || ny < 0 || nx >= MW || ny >= MH || bfs_par[ny][nx] != -1) continue;
            if (nx == tx && ny == ty) { bfs_par[ny][nx] = (s16)c; found = 1; break; }
            if (!world_walkable(nx, ny)) { bfs_par[ny][nx] = -2; continue; }
            bfs_par[ny][nx] = (s16)c;
            bfs_q[qt++] = (u16)(ny * MW + nx);
        }
    }
    if (!found) return 0;
    // walk the parents back, then reverse into the path
    u8 *rx = (u8 *)bfs_q, *ry = rx + 640; int n = 0;   // (in the queue's room: it is not needed any more)
    int c = ty * MW + tx;
    while (c != sy * MW + sx) { if (n >= 640) return 0; rx[n] = (u8)(c % MW); ry[n] = (u8)(c / MW); n++; c = bfs_par[c / MW][c % MW]; }
    // pull it straight: from where the body really is, reach for the furthest tile in sight, and again from there
    int ax = s->x, az = s->z, i = 0, m = 0;
    while (i < n && m < 64) {
        int j = MIN(n - 1, i + LOS_WIN - 1);
        for (; j > i; j--) { int k = n - 1 - j; if (los(ax, az, rx[k] * TILE + TILE / 2, -(ry[k] * TILE + TILE / 2))) break; }
        int k = n - 1 - j;
        s->path_x[m] = rx[k]; s->path_y[m] = ry[k]; m++;
        ax = rx[k] * TILE + TILE / 2; az = -(ry[k] * TILE + TILE / 2); i = j + 1;
    }
    s->path_n = (u8)m; s->path_i = 0;
    return 1;
}
static int start_walk(Sim *s, int tx, int ty, int hurry) {   // begin a walk: a path, and this walk's own character
    if (!sim_find_path(s, tx, ty)) return 0;
    s->repath = 0; s->stall = 0; s->pause_t = 0; s->mind = 0; s->hurry = (u8)hurry;
    s->wseed = (u8)(look_rand() & 255);
    if (s->spd < 250) s->spd = 250;
    return 1;
}
static int on_item(int act);
static int approach_tile(const Sim *s, int mx, int my, int *tx, int *ty);
static int mulq(int a, int b) { return (a * b) / 4096; }   // symmetric: a small push to the left goes as far as one to the right

static int walk_pace(const Sim *s) {   // percent of the stage's walking speed, for this person on this walk
    int p = 88 + (int)s->bore.tr[TR_ACT] * 3 + s->mood / 20 + (int)(s->wseed & 15) - 8;
    if (s->hurry == 1) p += 15; else if (s->hurry == 2) p += 30;
    return CLAMP(p, 70, 150);
}
// other people on the way: bend the heading round them (everyone keeps to their right, so two people head-on pass each other) and ease off
static int steer_round(const Sim *s, int *want, int *ease) {
    int me = (int)(s - sims), blocked = 0;
    int fx = isin(s->yaw), fz = -icos(s->yaw), rx = -icos(s->yaw), rz = -isin(s->yaw);
    for (int i = 0; i < nsims; i++) {
        const Sim *o = &sims[i];
        if (i == me || o->lvl != s->lvl || o->dead || on_item(o->action) || o->action == ACT_SLEEP) continue;
        int dx = o->x - s->x, dz = o->z - s->z;
        if (ABS(dx) > 400 || ABS(dz) > 400) continue;
        if (s->talk_to == i && ABS(dx) + ABS(dz) < TILE * 2) continue;           // going to talk to them: no dodging
        int ahead = (dx * fx + dz * fz) >> 12, side = (dx * rx + dz * rz) >> 12;
        if (ahead <= 0 || ahead > 400 || ABS(side) > 2 * SIM_RADIUS + 30) continue;
        int near = 400 - ahead;                                                   // 0 far .. 400 touching
        int bend = near;                                                          // up to ~35 degrees
        if (side > 12) *want += bend; else *want -= bend;                        // they are on my right: swing left; else swing right
        if (ahead < 220) { int e = 30 + ahead * 70 / 220; if (e < *ease) *ease = e; }
        if (ahead < 120 && ABS(side) < 90) blocked = 1;
    }
    return blocked;
}
static int target_taken(const Sim *s) {   // the thing it was heading for is being used by someone else
    if (s->act_x < 0 || s->talk_to >= 0) return 0;
    for (int i = 0; i < nsims; i++) {
        const Sim *o = &sims[i];
        if (o != s && o->action != ACT_NONE && o->action != ACT_WALK && o->act_x == s->act_x && o->act_y == s->act_y) return 1;
    }
    return 0;
}
static void glance(Sim *s) {   // stop and look: at someone near, or just away over a shoulder
    int me = (int)(s - sims), best = -1, bd = 6 * TILE;
    for (int i = 0; i < nsims; i++) {
        if (i == me || sims[i].lvl != s->lvl) continue;
        int d = ABS(sims[i].x - s->x) + ABS(sims[i].z - s->z);
        if (d < bd) { bd = d; best = i; }
    }
    if (best >= 0 && (look_rand() & 3)) s->look_yaw = (u16)yaw_towards(sims[best].x - s->x, sims[best].z - s->z);
    else s->look_yaw = (u16)((s->yaw + ((look_rand() & 1) ? 1 : -1) * (500 + (int)(look_rand() % 1000))) & 4095);
    s->pause_t = (u8)(45 + look_rand() % 110);
}
static int follow_path(Sim *s) {   // 1 while walking, 0 when there (or when the walk is given up)
    int me = (int)(s - sims);
    // a path is cut at 64 waypoints, and the world changes under it: plan again from here while the destination is still far
    if (s->path_i >= s->path_n) {
        int fdx = s->dest_x * TILE + TILE / 2 - s->x, fdz = -(s->dest_y * TILE + TILE / 2) - s->z;
        if (s->repath < 8 && (ABS(fdx) > TILE || ABS(fdz) > TILE)) {
            s->repath++;
            if (!sim_find_path(s, s->dest_x, s->dest_y) || s->path_n == 0) return 0;
        } else return 0;
    }
    // changes of mind: the thing it wanted is taken, or a stroll gets interrupted by something that cannot wait
    if (!((walk_tick + me) & 15)) {
        if (target_taken(s)) { if (!(look_rand() & 3)) sim_say(s, "HMPH"); s->path_n = 0; s->wander_t = 0; return 0; }
        if (s->act_x < 0 && s->talk_to < 0 && !s->mind) for (int n = 0; n < N_SOCIAL; n++) if ((s->need[n] >> 10) < 22) { s->mind = 1; s->path_n = 0; s->wander_t = 0; return 0; }
        if (s->talk_to >= 0 && s->talk_to < nsims && !((walk_tick >> 4) & 1)) {   // chasing someone who has moved on: head where they are now
            const Sim *o = &sims[s->talk_to]; int ox = sim_tile_x(o), oy = sim_tile_y(o);
            if ((ABS(ox - s->act_x) > 2 || ABS(oy - s->act_y) > 2) && ABS(o->x - s->x) + ABS(o->z - s->z) < TILE * 10) {
                int ax, ay; if (approach_tile(s, ox, oy, &ax, &ay) && sim_find_path(s, ax, ay)) { s->act_x = ox; s->act_y = oy; }
            }
        }
    }
    // standing still to look about
    if (s->pause_t) {
        s->pause_t--; s->spd = 0;
        turn_to(s, s->look_yaw, 70);
        s->gait.amp = s->gait.amp > 300 ? s->gait.amp - 300 : 0;
        return 1;
    }
    int last = s->path_i == s->path_n - 1;
    if (!s->hurry && !last && s->path_n - s->path_i >= 2 && s->stall == 0 && !(look_rand() % (s->act_x < 0 ? 700 : 1500))) { glance(s); return 1; }
    // where to: the waypoint, nudged a little off the tile's middle so no two people walk the same line (not the last one: that is where the thing is)
    int tx = s->path_x[s->path_i] * TILE + TILE / 2, tz = -(s->path_y[s->path_i] * TILE + TILE / 2);
    if (!last) {
        u32 h = ((u32)s->wseed * 0x9E3779B1u) ^ ((u32)(s->path_i + 1) * 0x85EBCA6Bu) ^ (u32)me * 0x27D4EB2Fu; h ^= h >> 13; h *= 0x5BD1E995u; h ^= h >> 15;
        int jx = (int)(h & 63) - 32, jz = (int)((h >> 6) & 63) - 32;
        if (footprint_free(tx + jx, tz + jz)) { tx += jx; tz += jz; }
    }
    int dx = tx - s->x, dz = tz - s->z;
    int dist = MAX(ABS(dx), ABS(dz));
    // arrived at this waypoint? Middle ones are passed on a curve (80 units), the last one is reached
    if (last ? dist <= 6 : dist <= 80) {
        if (last) { s->x = tx; s->z = tz; }
        s->path_i++; if (s->path_i >= s->path_n) { s->path_i = s->path_n; return s->repath < 8 && (ABS(s->dest_x * TILE + TILE / 2 - s->x) > TILE || ABS(-(s->dest_y * TILE + TILE / 2) - s->z) > TILE); }
        return 1;
    }
    // steering: turn first, a lazy meander on a stroll, round other people
    int want = yaw_towards(dx, dz);
    if (!s->hurry) want += isin(((int)walk_tick * 7 + s->wseed * 29) & 4095) >> 6;      // +-64: an unhurried walk is never ruler-straight
    int ease = 100, blocked = steer_round(s, &want, &ease);
    want &= 4095;
    int diff = ((want - s->yaw + 2048) & 4095) - 2048;
    int align = 4096 - ABS(diff) * 2; if (align < 600) align = 600;                    // how far round it still has to swing: the wider, the slower it takes it
    turn_to(s, want, 90 + (ABS(diff) >> 4));                                             // a body swings round in an arc, it does not snap
    // speed: this person's pace, ramped up from a standstill, eased into corners, round people and the last tile
    int tgt = speed_of(s) * walk_pace(s);
    tgt = (tgt * align) >> 12;
    tgt = tgt * ease / 100;
    if (last && dist < 256) tgt = tgt * (40 + dist * 60 / 256) / 100;
    if (tgt < 150) tgt = 150;
    if (s->spd < tgt) s->spd = (s16)MIN(tgt, s->spd + 60); else s->spd = (s16)MAX(tgt, s->spd - 120);
    int sp = s->spd / 100 + ((int)(walk_tick & 7) < (s->spd % 100) * 8 / 100 ? 1 : 0);   // the hundredths come out right over eight steps
    if (last && sp > dist) sp = dist;
    int moved = sp > 0 ? walk(s, mulq(isin(s->yaw), sp), -mulq(icos(s->yaw), sp)) : 0;
    if (sp > 0 && !moved && !footprint_free(s->x, s->z)) { s->x += mulq(isin(s->yaw), sp); s->z -= mulq(icos(s->yaw), sp); moved = 1; }   // stuck inside something: step out, as before
    // held up? wait a moment, then plan again, then give up on this walk
    if (!moved || blocked) { if (s->stall < 250) s->stall += blocked ? 1 : 2; } else if (s->stall) s->stall--;
    if (s->stall >= 90) {
        s->stall = 0; s->wseed = (u8)(look_rand() & 255);
        if (++s->repath >= 6) { s->path_n = 0; s->spd = 0; return 0; }
        if (!sim_find_path(s, s->dest_x, s->dest_y)) { s->path_n = 0; s->spd = 0; return 0; }
        if (!(look_rand() & 1)) sim_say(s, "EXCUSE ME");
    }
    s->gait.run = 0; s->gait.amp = CLAMP(1800 + s->spd * 3 / 2, 1800, 4096);                                // slow strolls swing less
    s->gait.phase = (s->gait.phase + sp * 16 + 20) & 4095;                                  // the legs keep up with the ground
    return 1;
}

// ---- actions ----
static int action_for(char c) {
    switch (c) {
        case 'F': case 's': return ACT_EAT; case 'T': case '&': return ACT_WC; case 'S': case 'z': return ACT_SLEEP; case 'H': case '0': return ACT_SHOWER;
        case 'C': case 'U': case 'h': case 'N': case '%': case '~': return ACT_SIT;
        case '@': case '+': return ACT_TRAVEL;
        case '|': return ACT_STAIRS;
        // the furniture pack
        case '!': case '$': case '(': case ')': case '>': case '[': case 'j': case 'n': case '{': case '}': case '`': return ACT_FUN;
        case ',': case '-': return ACT_SHOWER;
        case '/': case ':': return ACT_EAT;
        case ';': return ACT_SIT;
        case '<': case CH_BED4: case CH_BEDF: case CH_BEDT: case CH_BEDL: return ACT_SLEEP;
        case '?': return ACT_READ;
        case ']': case '_': return ACT_RUN;
        case 'v': case 'R': case 'A': case 'Q': case 'q': case 'G': case 'V': return ACT_FUN;
        case 'b': case 'd': return ACT_READ; case 'c': return ACT_COFFEE; case 'm': return ACT_RUN; case 'I': return ACT_PHONE; case 'x': return ACT_WORK;
        case (char)0x90: case (char)0x91: case (char)0x92: case (char)0xA8: case (char)0xBC: return ACT_SIT;   // (the second furniture pack)
        case (char)0x93: case (char)0x94: return ACT_SLEEP;
        case (char)0x9A: case (char)0x9E: return ACT_EAT;
        case (char)0x9F: return ACT_COFFEE;
        case (char)0xA9: case (char)0xB8: return ACT_RUN;
        case (char)0xC1: case (char)0xC2: case (char)0xC7: case (char)0xCA: case (char)0xCB: case (char)0xCC: case (char)0xCD: case (char)0xCE: return ACT_FUN; case (char)0xC8: return ACT_EAT; case (char)0xC9: return ACT_COFFEE;   // (the BOREAGOTCHI! pack: a plush and a ball)
        case (char)0xA0: case (char)0xA1: case (char)0xA2: case (char)0xA3: case (char)0xA4: case (char)0xA5: case (char)0xA6: case (char)0xA7: case (char)0xB9: case (char)0xBA: return ACT_FUN;
        case CH_CAKE: return ACT_EAT; case CH_GRAVE: return ACT_READ;
        case 'g': case '3': case '6': case '*': return world_zone == ZONE_ISLAND ? ACT_GATHER : ACT_NONE;   // (on the island: wood, coconuts, stone)
        case CH_BUSH: case CH_FISH: case CH_BOTTLE: case CH_DIG: case CH_IDOL: case CH_WRECK: case CH_SHELL: case CH_GOLD: case CH_JOURNAL: return ACT_GATHER;
        default: return ACT_NONE;
    }
}
static int on_item(int act) { return act == ACT_WC || act == ACT_SLEEP || act == ACT_SHOWER || act == ACT_SIT || act == ACT_RUN || act == ACT_WORK; }
static int stands_on(int act, int mx, int my) { return on_item(act) && !(act == ACT_WORK && items_blocking(world_at(mx, my))); }   // (work at a counter: in front of it)
static int gather_kind(char c) {   // what using it gets you, unless a pie choice says otherwise
    switch (c) {
        case 'g': return G_COCONUT; case '3': case '6': return G_WOOD; case '*': return G_STONE; case CH_BUSH: return G_BERRIES; case CH_FISH: return G_FISH;
        case CH_DIG: return G_DIG; case CH_WRECK: return G_WRECK; case CH_BOTTLE: return G_BOTTLE; case CH_JOURNAL: return G_JOURNAL; case CH_IDOL: return G_IDOL;
        case CH_SHELL: return G_SHELL; case CH_GOLD: return G_GOLD;
    }
    return G_NONE;
}
static int gather_len(int k) {
    switch (k) { case G_WOOD: return 300; case G_STONE: return 240; case G_LEAVES: return 150; case G_BERRIES: return 120; case G_COCONUT: return 180;
                 case G_FISH: return isl_spear ? 240 : 420; case G_DIG: return 360; case G_WRECK: return 300; case G_SHELL: return 30; default: return 200; }
}
static const char *const gatherHint[] = { "", "CHOP WOOD", "GATHER STONE", "GATHER LEAVES", "PICK BERRIES", "A COCONUT", "FISH", "DIG", "SEARCH IT", "READ IT", "READ IT", "LISTEN", "PICK IT UP", "TAKE IT" };
static const char *act_hint(int act, char c) {
    if (act == ACT_GATHER) return gatherHint[gather_kind(c) % 14];
    if (c == CH_RAFT) return "SAIL";
    if (c == CH_CAKE) return "CAKE";
    if (c == CH_GRAVE) return "REMEMBER";
    switch (act) {
        case ACT_TRAVEL: return c == '@' ? "SAIL" : "TAKE THE BUS";
        case ACT_STAIRS: return "STAIRS";
        default: break;
    }
    switch (c) {   // the furniture pack's own words
        case (char)0xC7: return "DANCE UNDER THE LIGHTS"; case (char)0xC8: return "GRAB A BITE"; case (char)0xC9: return "SIP THE PUNCH"; case (char)0xCA: return "DANCE TO THE BASS"; case (char)0xCB: return "POP A BALLOON"; case (char)0xCC: return "SPIN SOME TRACKS"; case (char)0xCD: return "SING KARAOKE"; case (char)0xCE: return "POSE FOR PHOTOS"; case (char)0xC1: return "CUDDLE THE PLUSH"; case (char)0xC2: return "PLAY WITH THE BALL";
        case '!': return "PLAY ARCADE"; case '$': return "PLAY PIANO"; case '(': return "PLAY GUITAR"; case ')': return "PAINT";
        case '>': return "SOAK"; case '[': return "USE COMPUTER"; case 'j': return "PICK A SONG"; case 'n': return "PLAY"; case '{': return "PLAY HOUSE";
        case '}': return "MAKE A WISH"; case '`': return "PRACTICE SPEECH"; case ',': return "TAKE A BATH"; case '-': return "WASH HANDS";
        case '/': return "COOK"; case ':': return "MICROWAVE"; case ';': return "SIT"; case '<': return "SLEEP"; case '?': return "PLAY CHESS"; case '~': return "SUNBATHE";
        case ']': return "LIFT WEIGHTS"; case '_': return "SHOOT HOOPS";
        case (char)0xA0: return "PLAY DRUMS"; case (char)0xA1: return "SHOOT POOL"; case (char)0xA2: return "STARGAZE"; case (char)0xA3: return "PLAY SYNTH";
        case (char)0xA4: return "PLAY FOOSBALL"; case (char)0xA5: return "THROW DARTS"; case (char)0xA6: return "PULL THE LEVER"; case (char)0xA7: return "SPIN A RECORD";
        case (char)0xA8: return "RELAX"; case (char)0xA9: return "PUNCH THE BAG"; case (char)0xB8: return "BOUNCE"; case (char)0xB9: return "SWING";
        case (char)0xBA: return "PLAY IN SAND"; case (char)0xBC: return "WARM UP"; case (char)0x9A: return "SNACK"; case (char)0x9E: return "GRILL"; case (char)0x9F: return "A DRINK";
    }
    switch (act) {
        case ACT_EAT: return c == 's' ? "COOK AND EAT" : "EAT"; case ACT_WC: return c == '&' ? "USE OUTHOUSE" : "USE TOILET"; case ACT_SLEEP: return "SLEEP"; case ACT_SHOWER: return "SHOWER";
        case ACT_SIT: return c == 'h' ? "SIT" : "LOUNGE"; case ACT_FUN: return c == 'v' ? "WATCH TV" : c == 'R' ? "TUNE RADIO" : c == 'A' ? "CRANK IT" : c == 'Q' ? "JACK IN" : c == 'q' ? "FEED THE FISH" : "CHILL";
        case ACT_READ: return "READ"; case ACT_COFFEE: return "A CUP"; case ACT_RUN: return "A RUN"; case ACT_PHONE: return "CALL"; case ACT_WORK: return "WORK";
        default: return "";
    }
}
static int act_len(int act) {
    switch (act) { case ACT_EAT: return 200; case ACT_WC: return 160; case ACT_SHOWER: return 260; case ACT_FUN: return 420; case ACT_READ: return 420;
                   case ACT_COFFEE: return 90; case ACT_RUN: return 360; case ACT_PHONE: return 300; case ACT_TALK: return 240; case ACT_SWIM: return 600; case ACT_CARE: return 150; default: return 9999; }
}
static void begin_action(Sim *s, int act, int mx, int my) {
    if (act == ACT_TALK && (int)(s - sims) == player) story_talks++;
    if ((int)(s - sims) == player && on_board) skate_off(s);
    s->action = act; s->act_t = act_len(act); s->act_x = mx; s->act_y = my; s->act_var = 0;
    s->path_n = 0;
    if (stands_on(act, mx, my)) { s->x = mx * TILE + TILE / 2; s->z = -(my * TILE + TILE / 2); s->yaw = (world_item_facing(mx, my) * 1024) & 4095; }
    else { s->yaw = yaw_towards(mx * TILE + TILE / 2 - s->x, -(my * TILE + TILE / 2) - s->z); }
    s->gait.sit = (act == ACT_SIT || act == ACT_WC);
    s->gait.sleep = (act == ACT_SLEEP);
    s->gait.amp = 0;
    if (act == ACT_SLEEP) audio_voice(VOICE_SNORE);
    if (act == ACT_EAT && world_zone == ZONE_ISLAND && world_at(mx, my) != CH_CAKE) {   // the castaway's meal: from what they have gathered (wherever it is cooked)
        if (isl_food <= 0) { s->action = ACT_NONE; sim_say(s, "NOTHING TO EAT"); if ((int)(s - sims) == player) sim_note("NO FOOD: PICK BERRIES, FISH"); return; }
        isl_food--;
    }
    if (act == ACT_EAT) audio_voice(VOICE_YEHA);
    if (act == ACT_GATHER) { s->act_var = (u8)gather_kind(world_at(mx, my)); s->act_t = gather_len(s->act_var); }
}
static void chat_done(Sim *s, Sim *o) {   // a finished chat: the relationship grows (more between compatible people), the wants notice
    int me = (int)(s - sims), him = (int)(o - sims), bonus = 0;
    for (int t = 0; t < TR_N; t++) if (ABS((int)s->bore.tr[t] - (int)o->bore.tr[t]) <= 2) bonus++;
    s->skill_char += 6;
    int was = s->rel[him], now = was + 8 + bonus + wants_skill_level(s->skill_char) / 2; if (now > 100) now = 100;
    s->rel[him] = (s8)now; if (o->rel[me] < now) o->rel[me] = (s8)now;
    wants_event(s, W_CHAT, 0); wants_event(s, W_CHAT_WITH, him); sim_moodlet(s, ML_GOOD_CHAT);
    if (was < 50 && now >= 50) { wants_event(s, W_FRIEND, him); if (me == player) { sim_note("YOU MADE A FRIEND"); } sim_moodlet(s, ML_FRIEND); sim_moodlet(o, ML_FRIEND); }
}
static void end_action(Sim *s) {
    switch (s->action) {   // what just finished: the wants are watching
        case ACT_EAT: if (s->act_var == 8) { bake_done(s); break; }   // (act_var: 0 a snack, 1 cooked with good food, 2 eaten out, 3 cooked plain, 4 a treat, 5 fine dining,
                      if ((s->act_var == 1 || s->act_var == 3) && (int)(look_rand() % 100) < SKL(s->skill_creat) * 9) { sim_moodlet(s, ML_INSPIRED); s->skill_creat += 20; if ((int)(s - sims) == player) sim_note("A CREATIVE DISH!"); }   // (CREATIVITY in the kitchen)
                      wants_event(s, W_EAT, 0);                   //  8 baking the birthday cake, 9 a slice of it)
                      if (s->act_var == 9) { sim_moodlet(s, ML_CAKE); cake_eaten(s); break; }
                      sim_moodlet(s, s->act_var == 5 ? ML_FINE_DINING : s->act_var == 2 ? ML_ATE_OUT : s->act_var == 1 ? ML_GOOD_MEAL : s->act_var == 4 ? ML_TREAT : ML_SNACKED);
                      if (s->act_var == 2 || s->act_var == 5) wants_event(s, W_DINER, 0); break;
        case ACT_SLEEP: if (s->act_var == 2) { sim_moodlet(s, ML_PAMPERED); break; }   // (a massage)
                        wants_event(s, W_SLEEP, 0); if (s->act_t <= 0 && s->act_var == 0) { wants_reroll(s); sim_moodlet(s, ML_RESTED); }   // a full sleep: new wants on waking
                        { PlaceInfo pi; if (place_at(s->act_x, s->act_y, &pi) && (pi.lot == LOT_INN || pi.lot == LOT_HOTEL)) sim_moodlet(s, ML_HOTEL); } break;
        case ACT_SHOWER: wants_event(s, W_SHOWER, 0); sim_moodlet(s, ML_FRESH); if (s->act_var == 1) sim_moodlet(s, ML_SINGING); break;
        case ACT_FUN: wants_event(s, W_FUN, 0);
                      if (s == &sims[player]) { char c_ = world_at(s->act_x, s->act_y); if (c_ == 'v') story_event(E_TV, 0); else if (c_ == 'R' || c_ == 'A' || c_ == 'j') story_event(E_RADIO, 0); }   // (act_var: 4 browsing a shop, 5 a drink at a bar, 7 looking at art)
                      sim_moodlet(s, s->act_var == 4 ? ML_RETAIL : s->act_var == 5 ? ML_NIGHT_OUT : s->act_var == 7 ? ML_INSPIRED : ML_FUN_TIME);
                      if (s->act_var == 4) wants_event(s, W_SHOP, 0); if (s->act_var == 5) wants_event(s, W_NIGHT, 0); break;
        case ACT_SIT: if (s->act_var == 6) { sim_moodlet(s, ML_FILM); wants_event(s, W_FILM, 0); wants_event(s, W_FUN, 0); } break;   // (a film)
        case ACT_READ: if (s->act_var == 3) { homework_done(s); break; } if (s->act_var == 5) { sim_moodlet(s, ML_MEMORIES); break; }   // (homework; at a grave)
                       wants_event(s, W_READ, 0); break;
        case ACT_CARE: care_done(s); break;
        case ACT_GATHER: gather_done(s); break;
        case ACT_COFFEE: wants_event(s, W_COFFEE, 0); sim_moodlet(s, ML_CAFFEINE); break;
        case ACT_RUN: wants_event(s, W_RUN, 0); sim_moodlet(s, s->act_var == 2 ? ML_COACHED : ML_WORKOUT);
                      { PlaceInfo pi; if (place_at(s->act_x, s->act_y, &pi) && pi.lot == LOT_GYM) wants_event(s, W_GYM, 0); } break;
        case ACT_SWIM: wants_event(s, W_SWIM, 0); sim_moodlet(s, ML_SWAM); break;
        case ACT_PHONE: wants_event(s, W_PHONE, 0); if (s->act_var == 1) { sim_moodlet(s, ML_PIZZA); if (s == &sims[player]) story_event(E_PIZZA, 0); } break;
        case ACT_WORK: wants_event(s, W_WORK, 0); break;
        case ACT_TALK: if (s->talk_to >= 0 && s->talk_to < nsims) chat_done(s, &sims[s->talk_to]); break;
    }
    if (s->action == ACT_TALK && s->talk_to >= 0 && s->talk_to < nsims) {
        Sim *o = &sims[s->talk_to];
        if (o->action == ACT_TALK && o->talk_to == (int)(s - sims)) { o->action = ACT_NONE; o->talk_to = -1; o->gait.wave = 0; }
    }
    if (s->action == ACT_SWIM) {   // out of the water: onto the nearest dry tile
        int wx = s->act_x, wy = s->act_y, done = 0;
        for (int r = 1; r <= 3 && !done; r++) for (int dy = -r; dy <= r && !done; dy++) for (int dx = -r; dx <= r && !done; dx++) {
            int x = wx + dx, y = wy + dy;
            if (x < 1 || y < 1 || x >= MW - 1 || y >= MH - 1 || floorMap[y][x] == FL_WATER || !world_walkable(x, y)) continue;
            s->x = x * TILE + TILE / 2; s->z = -(y * TILE + TILE / 2); done = 1;
        }
    }
    else if (stands_on(s->action, s->act_x, s->act_y)) {   // step off the item onto open floor
        int mx = s->act_x, my = s->act_y, d = world_item_facing(mx, my);
        static const signed char dx[4] = { 0, 1, 0, -1 }, dy[4] = { 1, 0, -1, 0 };
        if (world_walkable(mx + dx[d], my + dy[d])) { s->x = (mx + dx[d]) * TILE + TILE / 2; s->z = -((my + dy[d]) * TILE + TILE / 2); }
    }
    s->action = ACT_NONE; s->act_t = 0; s->talk_to = -1;
    s->gait.sit = 0; s->gait.sleep = 0; s->gait.wave = 0;
}

static int is_night(void) { return clock_min >= NIGHT_FROM || clock_min < NIGHT_TO; }
static int is_work(void) { return job_on_shift(); }

static void run_action(Sim *s) {
    int me = (int)(s - sims);
    switch (s->action) {
        case ACT_EAT: if (s->act_var == 8) { if (!(s->act_t & 7)) s->skill_cook++; break; }   // (baking: no eating yet)
                      s->need[N_FOOD] += (s->act_var == 2 || s->act_var == 5) ? GAIN_EAT * 2 : (s->act_var == 1 || s->act_var == 3) ? GAIN_EAT * 3 / 2 : GAIN_EAT;
                      if (s->act_var == 1 && !(s->act_t & 7)) s->skill_cook++; if (s->act_var == 4) s->need[N_FUN] += 60; break;
        case ACT_WC: s->need[N_WC] += GAIN_WC; break;
        case ACT_SLEEP: if (s->act_var == 2) { s->need[N_COMFY] += GAIN_SIT; s->need[N_FUN] += GAIN_FUN / 2; s->need[N_REST] += 60; break; }   // (a massage)
                        s->need[N_REST] += is_night() ? GAIN_SLEEP * 5 / 4 : GAIN_SLEEP; s->need[N_COMFY] += 40;
                        if ((s->need[N_REST] >> 10) >= 100 && (!is_night() || clock_min < NIGHT_FROM || (rt_on && me == player))) { s->act_t = 0; sim_say(s, "RESTED"); } break;   // (REAL TIME: you wake when rested; the others sleep the real night through)
        case ACT_SHOWER: s->need[N_CLEAN] += GAIN_WASH; if (s->act_var == 1) s->need[N_FUN] += 90; break;
        case ACT_SIT: s->need[N_COMFY] += GAIN_SIT; if (s->act_var == 6) { s->need[N_FUN] += GAIN_FUN; break; }   // (a film: to the end)
                      if ((s->need[N_COMFY] >> 10) >= 100) s->act_t = 0; break;
        case ACT_FUN: s->need[N_FUN] += GAIN_FUN; s->need[N_COMFY] += 30;
                      if (s->act_var == 5) { s->need[N_SOCIAL] += GAIN_TALK / 2; if (!(s->act_t & 63)) s->gait.wave = 20; }   // (a drink at the bar)
                      if (s->act_var == 7 && !(s->act_t & 7)) { s->skill_fun++; s->skill_creat++; }
                      if (s->act_var >= 8 && s->act_var < 8 + 4) { if (!(s->act_t & 7)) { int k = s->act_var - 8; s->skill_band[k] = MIN(4400, s->skill_band[k] + 4); s->skill_creat += 2; } s->need[N_FUN] += GAIN_FUN / 2; if (!(s->act_t & 31)) s->gait.wave = 20; } break;
        case ACT_READ: s->need[N_FUN] += s->act_var == 1 ? GAIN_FUN / 4 : GAIN_FUN / 2; s->skill_fun += s->act_var == 1 ? 3 : 1; s->skill_logic += s->act_var == 1 ? 3 : 1; break;
        case ACT_COFFEE: s->need[N_REST] += s->act_var == 1 ? 320 : 180; s->need[N_WC] -= s->act_var == 1 ? 110 : 60; break;
        case ACT_RUN: s->need[N_FUN] += 120; s->need[N_CLEAN] -= s->act_var ? 160 : 90; s->skill_body += s->act_var == 2 ? 3 : s->act_var == 1 ? 2 : 1; s->gait.amp = 4096; s->gait.run = 1; s->gait.phase = (s->gait.phase + 200) & 4095; break;
        case ACT_GATHER: if (!(s->act_t & 31)) s->gait.wave = 20;   // busy hands; chopping and hauling tire you
                         if (s->act_var == G_WOOD || s->act_var == G_STONE || s->act_var == G_DIG) s->need[N_REST] -= 40;
                         if (s->act_var == G_FISH) s->need[N_FUN] += 30; break;
        case ACT_CARE: if (s->talk_to < 0 || s->talk_to >= nsims) { s->act_t = 0; break; }   // looking after the baby: busy hands
                       if (!(s->act_t & 31)) s->gait.wave = 20; s->yaw = yaw_towards(sims[s->talk_to].x - s->x, sims[s->talk_to].z - s->z); break;
        case ACT_SWIM: {   // floating about on the water, turning lazily
            int t = s->act_t;
            s->need[N_FUN] += GAIN_FUN; s->need[N_CLEAN] += 120; s->need[N_COMFY] += 20; if (!(t & 7)) s->skill_body++;
            s->x = s->act_x * TILE + TILE / 2 + ((isin((t * 9) & 4095) * 70) >> 12); s->z = -(s->act_y * TILE + TILE / 2) + ((icos((t * 5) & 4095) * 50) >> 12);
            s->yaw = (t * 5) & 4095; s->gait.sleep = 1; break; }
        case ACT_PHONE: if (s->act_var == 1) s->need[N_FOOD] += 300; else if (s->act_var == 5) s->need[N_FUN] += (handheld & 2) ? 90 : 70; else s->need[N_SOCIAL] += GAIN_TALK; if (s->act_var != 5) s->gait.wave = 2; break;
        case ACT_TALK: s->need[N_SOCIAL] += GAIN_TALK; s->need[N_FUN] += 40; if ((s->act_t & 63) == 0) s->gait.wave = 40; break;
        case ACT_WORK: if (is_work()) { s->need[N_FUN] -= 20; if (!(s->act_t & 63)) s->gait.wave = 24; if (job_car == CAR_FARM && !(s->act_t & 15)) s->skill_farm++; } else { s->act_t = 0; } break;   // (the minutes count in clock_tick)
        default: break;
    }
    for (int i = 0; i < NEED_N; i++) s->need[i] = CLAMP(s->need[i], 0, 100 << 10);
    if (s->act_t > 0) s->act_t--;
    if (s->act_t <= 0) end_action(s);
}

// ---- the lowest need and where to fix it ----
static char need_item(int n) {
    switch (n) { case N_FOOD: return 'F'; case N_WC: return 'T'; case N_REST: return 'S'; case N_CLEAN: return 'H'; case N_COMFY: return 'C'; case N_FUN: return 'v'; default: return 0; }
}
static int nearest_item(const Sim *s, char ch, int *ox, int *oy) {
    int bx = -1, by = -1, bd = 1 << 30, sx = sim_tile_x(s), sy = sim_tile_y(s);
    for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) {
        char c = lifeMap[y][x];
        int ok = (c == ch) || (ch == 'C' && (c == 'U' || c == 'h' || c == '%')) || (ch == 'v' && (c == 'R' || c == 'A' || c == 'b' || c == 'q' || c == 's'))
              || (ch == 'F' && c == 's') || (ch == 'T' && c == '&') || (ch == 'S' && (c == 'z' || c == CH_BED4 || c == CH_BEDF || c == CH_BEDT || c == CH_BEDL)) || (ch == CH_CRIB && (c == CH_BASS || c == CH_CRAD)) || (ch == 'H' && c == '0');
        if (!ok) continue;
        // taken by someone else?
        int busy = 0; for (int i = 0; i < nsims; i++) if (&sims[i] != s && sims[i].action != ACT_NONE && sims[i].act_x == x && sims[i].act_y == y) busy = 1;
        if (busy) continue;
        int d = (x - sx) * (x - sx) + (y - sy) * (y - sy);
        if (d < bd) { bd = d; bx = x; by = y; }
    }
    if (bx < 0) return 0;
    *ox = bx; *oy = by; return 1;
}
static int approach_tile(const Sim *s, int mx, int my, int *tx, int *ty) {   // the open tile to stand on to use (mx,my)
    int d = world_item_facing(mx, my);
    static const signed char dx[4] = { 0, 1, 0, -1 }, dy[4] = { 1, 0, -1, 0 };
    for (int k = 0; k < 4; k++) { int dd = (d + k) & 3; if (world_walkable(mx + dx[dd], my + dy[dd])) { *tx = mx + dx[dd]; *ty = my + dy[dd]; return 1; } }
    (void)s; return 0;
}

// the running gag: one housemate vehemently refuses any floating marker. They have a speech about it.
static const char *const grumpNm[8] = { "NO PLUMBOBS. EVER.", "I SAID NO PLUMBOBS", "NOT OVER MY HEAD", "I CAN HEAR IT SPIN",
                                        "KEEP THE GEM AWAY", "AND NO ARROWS EITHER", "IT DRIPS SMUGNESS", "A DIAMOND? OVER ME?" };
const char *grump_line(void) { static int k; return grumpNm[k++ & 7]; }
static int outdoors(const Sim *s) { int x = sim_tile_x(s), y = sim_tile_y(s); return s->lvl == 0 && x >= 0 && y >= 0 && x < MW && y < MH && floorMap[y][x] >= FL_GRASS && floorMap[y][x] != FL_WATER && floorMap[y][x] <= FL_SAND; }
// a stroll: not a dart to a random square. Out of doors it goes further, keeps roughly the way it was facing, and likes a path;
// indoors it potters. Eight ideas, the one it likes best wins.
static int pick_stroll(const Sim *s, int *ox, int *oy) {
    int sx = sim_tile_x(s), sy = sim_tile_y(s), out = outdoors(s), best = -1000, bx = -1, by = -1;
    for (int t = 0; t < 8; t++) {
        int tx, ty;
        if (look_rand() % 3) {   // on from where it was heading
            int dist = 3 + (int)(look_rand() % (out ? 14 : 5));
            tx = sx + (isin(s->yaw) * dist) / 4096 + (int)(look_rand() % 5) - 2;
            ty = sy + (icos(s->yaw) * dist) / 4096 + (int)(look_rand() % 5) - 2;
        } else {                 // or a change of mind
            int r = out ? 12 : 5;
            tx = sx + (int)(look_rand() % (2 * r + 1)) - r; ty = sy + (int)(look_rand() % (2 * r + 1)) - r;
        }
        if (tx <= 0 || ty <= 0 || tx >= MW - 1 || ty >= MH - 1 || !world_walkable(tx, ty) || (tx == sx && ty == sy)) continue;
        int sc = (int)(look_rand() & 3);
        if (s->lvl == 0) { u8 f = floorMap[ty][tx]; if (f == FL_PAVING) sc += out ? 4 : 0; else if (f == FL_GRASS) sc += out ? 2 : 0; }
        sc -= (ABS(tx - sx) + ABS(ty - sy)) / 8;
        if (sc > best) { best = sc; bx = tx; by = ty; }
    }
    if (bx < 0) return 0;
    *ox = bx; *oy = by; return 1;
}
static int think_tok;   // (one of the household makes a real decision a step: two who come free at once are a 60th of a second apart, not one frame's spike)
static void ai_think(Sim *s) {
    if (s->action != ACT_NONE || s->path_i < s->path_n) return;
    if (!s->wander_t) { if (think_tok) return; think_tok = 1; }
    if (s->bore.stage == AG_BABY) {   // a baby: asleep in its crib at night (or when tired), otherwise sat on the floor playing
        if (is_night() || (s->need[N_REST] >> 10) < 35) baby_to_bed(s);
        else { begin_action(s, ACT_SIT, sim_tile_x(s), sim_tile_y(s)); s->act_t = 240; }
        return;
    }
    int me = (int)(s - sims);
    if ((sim_hold >> me) & 1) return;   // (told to stay put: they wait where they are)
    if (s->bore.stage >= AG_TEEN && (s->need[N_FOOD] >> 10) > 15 && (s->need[N_WC] >> 10) > 15) {   // a baby crying, nobody seeing to it: go
        for (int i = 0; i < nsims; i++) {
            const Sim *b = &sims[i]; int k = baby_need(b); if (!k) continue;
            int taken = 0; for (int j = 0; j < nsims; j++) if (j != me && sims[j].talk_to == i && (sims[j].errand || sims[j].action == ACT_CARE)) taken = 1;
            if (taken) continue;
            int ax, ay; if (approach_tile(s, sim_tile_x(b), sim_tile_y(b), &ax, &ay) && start_walk(s, ax, ay, 1)) { s->action = ACT_WALK; s->act_x = -1; s->talk_to = i; s->errand = (u8)k; s->act_t = 9999; return; }
        }
    }
    if (school_age(s) && !s->homework && clock_day < 5 && clock_min >= 900 && clock_min < 1260 && !(look_rand() & 3)) {   // after school: homework at a desk or a shelf
        int ix, iy, tx, ty;
        if ((nearest_item(s, 'd', &ix, &iy) || nearest_item(s, 'b', &ix, &iy)) && approach_tile(s, ix, iy, &tx, &ty) && start_walk(s, tx, ty, 0)) {
            s->act_x = ix; s->act_y = iy; s->action = ACT_WALK; s->act_t = 9999; s->errand = 9; return; }
    }
    if (s->wander_t > 0) { s->wander_t--; return; }
    // the lowest need
    int worst = -1, wv = 101;
    for (int n = 0; n < N_SOCIAL; n++) { int v = s->need[n] >> 10; if (v < wv) { wv = v; worst = n; } }
    if (wv < 45 && worst == N_FOOD && world_zone == ZONE_ISLAND) {   // a castaway's meal: at a fire, if there is food in and a fire to get to; else off to forage
        int ix, iy, tx, ty;
        if (isl_food > 0 && nearest_item(s, 'F', &ix, &iy) && approach_tile(s, ix, iy, &tx, &ty) && start_walk(s, tx, ty, wv < 15 ? 2 : 1)) {
            s->act_x = ix; s->act_y = iy; s->action = ACT_WALK; s->act_t = 9999; return; }
        static const char wild[2] = { (char)CH_BUSH, 'g' };   // (berries, else a coconut: whichever it can get to)
        for (int k = 0; k < 2; k++) if (nearest_item(s, wild[k], &ix, &iy) && approach_tile(s, ix, iy, &tx, &ty) && start_walk(s, tx, ty, wv < 15 ? 2 : 1)) {
            s->act_x = ix; s->act_y = iy; s->action = ACT_WALK; s->act_t = 9999; return; }
    }
    if (wv < 45) {
        if (worst == N_REST && !is_night() && wv > 15) { worst = -1; }
        char ch = worst >= 0 ? need_item(worst) : 0;
        int ix, iy, tx, ty;
        if (ch && nearest_item(s, ch, &ix, &iy) && approach_tile(s, ix, iy, &tx, &ty) && start_walk(s, tx, ty, wv < 15 ? 2 : wv < 30 ? 1 : 0)) {
            s->act_x = ix; s->act_y = iy; s->action = ACT_WALK; s->act_t = 9999; return;
        }
    }
    // social: find the player or another sim
    if ((s->need[N_SOCIAL] >> 10) < 40 && nsims > 1) {
        int best = -1, bd = 1 << 30;
        for (int i = 0; i < nsims; i++) { if (i == me || sims[i].action != ACT_NONE) continue; int d = ABS(sims[i].x - s->x) + ABS(sims[i].z - s->z); if (d < bd) { bd = d; best = i; } }
        if (best >= 0 && bd < TILE * 10) {
            int tx = sim_tile_x(&sims[best]), ty = sim_tile_y(&sims[best]);
            int ax, ay; if (approach_tile(s, tx, ty, &ax, &ay) && start_walk(s, ax, ay, 0)) { s->talk_to = best; s->action = ACT_WALK; s->act_x = tx; s->act_y = ty; s->act_t = 9999; return; }
        }
    }
    // a want for somewhere to be: a seat, or something to do, if one is near and it is not already content
    if (!(look_rand() & 3)) {
        char ch = (s->need[N_COMFY] >> 10) < 85 ? 'C' : (s->need[N_FUN] >> 10) < 85 ? 'v' : 0;
        int ix, iy, tx, ty;
        if (ch && nearest_item(s, ch, &ix, &iy) && ABS(ix - sim_tile_x(s)) + ABS(iy - sim_tile_y(s)) <= 14 && approach_tile(s, ix, iy, &tx, &ty) && start_walk(s, tx, ty, 0)) {
            s->act_x = ix; s->act_y = iy; s->action = ACT_WALK; s->act_t = 9999; return;
        }
    }
    // wander
    { int tx, ty; if (pick_stroll(s, &tx, &ty) && start_walk(s, tx, ty, 0)) { s->action = ACT_WALK; s->act_x = -1; s->act_t = 9999; if (wv < 45) s->mind = 1; } }   // (already tried to fix the need and could not: no point dropping the stroll for it)
    if (s->grump && !(look_rand() % 3)) sim_say(s, grump_line());
    // then it stands about a while (shorter out in the open, and for the lively)
    s->wander_t = 50 + (int)(look_rand() % (outdoors(s) ? 220 : 340)) - (int)s->bore.tr[TR_ACT] * 6;
    if (s->wander_t < 20) s->wander_t = 20;
}

static void arrive(Sim *s) {   // an AI sim reached the end of its path
    s->action = ACT_NONE;
    if (s->errand) {   // what it came over to do
        int k = s->errand; s->errand = 0;
        if (k == 9) { if (s->act_x >= 0) { begin_action(s, ACT_READ, s->act_x, s->act_y); s->act_var = 3; s->act_t = 240; } return; }   // homework
        int bi = s->talk_to;
        if (bi >= 0 && bi < nsims && ABS(sims[bi].x - s->x) < TILE * 2 && ABS(sims[bi].z - s->z) < TILE * 2) { care_start(s, bi, k); return; }
        s->talk_to = -1; return;
    }
    if (s->talk_to >= 0) {
        Sim *o = &sims[s->talk_to];
        if (o->action == ACT_NONE && ABS(o->x - s->x) < TILE * 2 && ABS(o->z - s->z) < TILE * 2) {
            int me = (int)(s - sims);
            begin_action(s, ACT_TALK, sim_tile_x(o), sim_tile_y(o));
            begin_action(o, ACT_TALK, sim_tile_x(s), sim_tile_y(s));
            o->talk_to = me; s->talk_to = me == o->talk_to ? s->talk_to : s->talk_to;
            o->path_n = 0;
            audio_voice(VOICE_AGREE);
            return;
        }
        s->talk_to = -1;
        return;
    }
    s->spd = 0; s->pause_t = 0;
    if (s->act_x >= 0 && ABS(s->x - (s->act_x * TILE + TILE / 2)) < TILE * 2 && ABS(s->z + (s->act_y * TILE + TILE / 2)) < TILE * 2) {   // only if it really got there
        char c = world_at(s->act_x, s->act_y); int act = action_for(c);
        if (act != ACT_NONE) begin_action(s, act, s->act_x, s->act_y);
    }
}

static void tick_needs(Sim *s) {
    int sleeping = s->action == ACT_SLEEP, np = rt_on && walk_tick % 120 ? 0 : sim_need_pct;   // (REAL TIME: a real minute takes what two of a normal day's game minutes do)
    #define NR(r) ((r) * np / 100)   // (OPTIONS > GAME > NEEDS: off, slow, normal, fast)
    s->need[N_FOOD] -= sleeping ? NR(RATE_FOOD / 3) : NR(RATE_FOOD);
    s->need[N_WC] -= sleeping ? NR(RATE_WC / 3) : NR(RATE_WC);
    if (!sleeping) s->need[N_REST] -= is_night() ? NR(RATE_REST * 3 / 2) : NR(RATE_REST);
    s->need[N_CLEAN] -= NR(RATE_CLEAN);
    if (s->action != ACT_SIT && s->action != ACT_SLEEP) s->need[N_COMFY] -= NR(RATE_COMFY);
    if (s->action != ACT_FUN && s->action != ACT_READ) s->need[N_FUN] -= NR(RATE_FUN);
    if (s->action != ACT_TALK && s->action != ACT_PHONE) s->need[N_SOCIAL] -= NR(RATE_SOCIAL);
    #undef NR
    int target;   // (the room's score is a look at the 25 tiles round the sim, and it only moves the need a 256th a step: looked at every 8th step is plenty)
    { static u8 rs_age[16]; static s16 rs_val[16]; int si = (int)(s - sims) & 15; if (!(rs_age[si]++ & 7)) rs_val[si] = (s16)world_room_score(sim_tile_x(s), sim_tile_y(s)); target = rs_val[si] << 10; }
    s->need[N_ROOM] += (target - s->need[N_ROOM]) / 256;
    for (int i = 0; i < NEED_N; i++) s->need[i] = CLAMP(s->need[i], 0, 100 << 10);
    // mood: the needs feed HAPPY
    int sum = 0; for (int i = 0; i < NEED_N; i++) sum += s->need[i] >> 10;
    int m = (sum / NEED_N - 50) * 2;
    m = m / 2 + moodlet_sum(s);                                           // the needs AND the moodlets (Sims 3)
    m += (s->asp_pts - 6000) / 150;                                        // the aspiration meter colours the mood
    if (wants_desperate(s)) m -= 40;
    if (wants_level(s) == ASP_PLATINUM) m = 100;                            // PLATINUM: nothing can bring it down
    m = CLAMP(m, -100, 100);
    s->mood += (m - s->mood) / 64;
    // the thought bubble: the most urgent need you can do something about (not COMFY or ROOM: those are where you are, not what you need)
    int worst = -1, wv = 101;
    for (int n = 0; n < NEED_N; n++) { if (n == N_COMFY || n == N_ROOM) continue; int v = s->need[n] >> 10; if (v < wv) { wv = v; worst = n; } }
    s->think = wv < 20 ? worst : -1;
    if (s->think >= 0 && (int)(s - sims) == player && (s->think_t++ % 900) == 0) {
        audio_voice(s->think == N_FOOD ? VOICE_HUNGRY : s->think == N_WC ? VOICE_PEE : VOICE_NEED);
    }
    if (s->bore.stage == AG_BABY) {   // a baby: it cannot see to itself; it cries, and the grown-ups come
        int me = (int)(s - sims);
        if ((s->need[N_WC] >> 10) == 0) { if (s->tod_potty >= 100 && sim_is_toddler(s)) { s->need[N_WC] = 100 << 10; sim_say(s, "POTTY!"); } else { s->need[N_WC] = 100 << 10; s->need[N_CLEAN] = 0; } }
        if (sim_is_toddler(s) && s->action != ACT_SLEEP && s->mood < -10 && ((walk_tick + me * 131) % 1200) == 0) {   // a toddler tantrum: the whole house hears of it
            sim_say(s, "NO NO NO!"); audio_voice(VOICE_CRY); sim_moodlet(s, ML_TANTRUM);
            for (int j = 0; j < nsims; j++) if (j != me && sims[j].bore.stage >= AG_TEEN && !sims[j].away && sims[j].lvl == s->lvl) sim_moodlet(&sims[j], ML_TANTRUM);
            if (me == player || sims[player].lvl == s->lvl) sim_note("A TODDLER TANTRUM! COMFORT IT"); }
        if ((s->need[N_REST] >> 10) == 0 && s->action != ACT_SLEEP) baby_to_bed(s);
        if (baby_need(s) && s->action != ACT_SLEEP && ((walk_tick + me * 97) % 700) == 0) { sim_say(s, "WAAAH!"); audio_voice(VOICE_CRY); }
        return;
    }
    // passing out
    if ((s->need[N_REST] >> 10) == 0 && s->action != ACT_SLEEP) { s->need[N_REST] = 25 << 10; s->mood -= 30; sim_say(s, "PASSED OUT"); audio_voice(VOICE_GROAN); wants_fear(s, F_PASSOUT); sim_moodlet(s, ML_PASSED_OUT); }
    if ((s->need[N_WC] >> 10) == 0) { s->need[N_WC] = 100 << 10; s->need[N_CLEAN] = 0; s->mood -= 40; sim_say(s, "OH NO"); audio_voice(VOICE_PEE); wants_fear(s, F_PEE); sim_moodlet(s, ML_WET); sim_moodlet(s, ML_EMBARRASSED); }
    if (wants_desperate(s) && (s->think_t % 700) == 1 && s->action == ACT_NONE) {   // rock bottom: acting out
        static const char *const act[4] = { "KICKS THE BIN", "JUMPS ON THE SOFA", "SULKS LOUDLY", "WONT LEARN A THING" };
        sim_say(s, act[look_rand() & 3]);
    }
}

// ================================================================== CASTAWAY ISLAND: gathering, the secrets, the raft (island.h)
static void gather_done(Sim *s) {
    char t[32], *cell = &lifeMap[s->act_y][s->act_x];
    if (s->act_x < 0 || s->act_y < 0 || s->act_x >= MW || s->act_y >= MH) return;
    switch (s->act_var) {
        case G_WOOD: isl_wood++; sim_say(s, "+1 WOOD"); break;
        case G_STONE: isl_stone++; sim_say(s, "+1 STONE"); break;
        case G_LEAVES: isl_leaves += 2; sim_say(s, "+2 LEAVES"); break;
        case G_BERRIES: isl_food++; s->need[N_FOOD] += 10 << 10; sim_say(s, "+1 FOOD: BERRIES"); break;   // (and a few eaten as they are picked)
        case G_COCONUT: isl_food++; s->need[N_FOOD] += 10 << 10; sim_say(s, "+1 FOOD: A COCONUT"); break;
        case G_FISH: { int n = isl_spear ? 3 : 2; isl_food += n; sprintf(t, "+%d FOOD: FISH!", n); sim_say(s, t); wants_event(s, W_FUN, 0); break; }
        case G_DIG: if (*cell == CH_DIG) { *cell = '.'; world_touched(); cash += 2500; sim_note("TREASURE! A CHEST OF $2500"); isl_found(SEC_TREASURE); } break;
        case G_WRECK: if (!((isl_secrets >> SEC_WRECK) & 1)) { cash += 500; sim_note("IN THE WRECK: $500 AND A MAP"); sim_note("THE MAP SHOWS AN ISLET"); }
                      else sim_note("NOTHING MORE IN THE WRECK");
                      isl_found(SEC_WRECK); break;
        case G_BOTTLE: if (*cell == CH_BOTTLE) { *cell = '.'; world_touched(); } sim_note("A NOTE: DIG BY THE BIG ROCK"); isl_found(SEC_BOTTLE); break;
        case G_JOURNAL: sim_note("THE HERMIT: A RAFT REACHES"); sim_note("THE ISLET. THE GOLD IS THERE"); isl_found(SEC_JOURNAL); break;
        case G_IDOL: sim_note("THE IDOL HUMS. YOU FEEL LUCKY"); sim_moodlet(s, ML_INSPIRED); isl_found(SEC_IDOL); break;
        case G_SHELL: if (*cell == CH_SHELL) { *cell = '.'; world_touched(); isl_shells++; sprintf(t, "SEASHELL %d OF %d", isl_shells, ISL_SHELLS); sim_note(t);
                      if (isl_shells >= ISL_SHELLS) isl_found(SEC_SHELLS); } break;
        case G_GOLD: if (*cell == CH_GOLD) { *cell = '.'; world_touched(); cash += 10000; sim_note("THE GOLDEN IDOL! $10000"); isl_found(SEC_GOLD); } break;
    }
    for (int i = 0; i < NEED_N; i++) s->need[i] = CLAMP(s->need[i], 0, 100 << 10);
}
static int land_near(int x0, int y0, int r0, int *ox, int *oy) {   // dry ground r0 or more from (x0, y0) with nobody on it
    for (int r = r0; r <= r0 + 3; r++) for (int dy = -r; dy <= r; dy++) for (int dx = -r; dx <= r; dx++) {
        if (ABS(dx) != r && ABS(dy) != r) continue;
        int x = x0 + dx, y = y0 + dy;
        if (x < 1 || y < 1 || x >= MW - 1 || y >= MH - 1 || lifeMap[y][x] != '.' || floorMap[y][x] == FL_WATER) continue;
        int busy = 0; for (int i = 0; i < nsims; i++) if (!sims[i].away && sim_tile_x(&sims[i]) == x && sim_tile_y(&sims[i]) == y) busy = 1;
        if (!busy) { *ox = x; *oy = y; return 1; }
    }
    return 0;
}
static void raft_sail(void) {   // across to the islet, or back to the island: everyone comes (and a raft waits at each end)
    const Sim *p = &sims[player]; int px = sim_tile_x(p), py = sim_tile_y(p);
    int out = (px - isl_dock_x) * (px - isl_dock_x) + (py - isl_dock_y) * (py - isl_dock_y) < (px - isl_islet_x) * (px - isl_islet_x) + (py - isl_islet_y) * (py - isl_islet_y);   // (by the island's raft: out)
    int tx = out ? isl_islet_x : isl_dock_x, ty = out ? isl_islet_y : isl_dock_y;
    if (out && lifeMap[ty][tx] == '.') { lifeMap[ty][tx] = CH_RAFT; world_touched(); }
    for (int i = 0; i < nsims; i++) {
        Sim *o = &sims[i]; if (o->away) continue;
        if (o->action != ACT_NONE) end_action(o);
        o->action = ACT_NONE; o->path_n = 0; o->path_i = 0; o->lvl = 0; o->talk_to = -1; o->errand = 0;
        int x, y; if (!land_near(tx, ty, i == player ? 1 : 2, &x, &y)) { x = tx; y = ty; }   // (you by the raft, the rest a step back from it)
        o->x = x * TILE + TILE / 2; o->z = -(y * TILE + TILE / 2);
    }
    sim_note(out ? "YOU SAIL OUT TO THE ISLET" : "BACK ON THE ISLAND");
    if (out) isl_found(SEC_ISLET);
    audio_voice(VOICE_YAHOO);
}

// ================================================================== GROWING UP: birthdays, babies, school, old age, the family tree
static Sim *by_uid(int u) { if (!u) return 0; for (int i = 0; i < nsims; i++) if (sims[i].uid == u) return &sims[i]; return 0; }
const char *sim_relation(int i, int j) {   // what j is to i
    const Sim *a = &sims[i], *b = &sims[j];
    if (a->partner && a->partner == b->uid) return "PARTNER";
    if (b->uid && (a->par[0] == b->uid || a->par[1] == b->uid)) return "PARENT";
    if (a->uid && (b->par[0] == a->uid || b->par[1] == a->uid)) return "CHILD";
    for (int k = 0; k < 2; k++) if (a->par[k] && (a->par[k] == b->par[0] || a->par[k] == b->par[1])) return "SIBLING";
    for (int k = 0; k < 2; k++) { const Sim *p = by_uid(a->par[k]); if (p && b->uid && (p->par[0] == b->uid || p->par[1] == b->uid)) return "GRANDPARENT"; }
    for (int k = 0; k < 2; k++) { const Sim *p = by_uid(b->par[k]); if (p && a->uid && (p->par[0] == a->uid || p->par[1] == a->uid)) return "GRANDCHILD"; }
    return 0;
}
const char *sim_grade(int g) { return g >= 85 ? "A" : g >= 70 ? "B" : g >= 55 ? "C" : g >= 40 ? "D" : "F"; }
int sim_grave_count(void) { return ngraves; }
int sim_grave_spot(int i, int *mx, int *my) {   // grave i in the map as it is now (0: not here)
    if (i < 0 || i >= ngraves) return 0;
    if (graves[i].city) { int wx, wy; if (world_zone != ZONE_CITY || !world_own_block(&wx, &wy)) return 0; *mx = graves[i].x + wx * BM_BLK; *my = graves[i].y + wy * BM_BLK; return 1; }
    if (world_zone != ZONE_HOME) return 0;
    *mx = graves[i].x; *my = graves[i].y; return 1;
}
#ifdef AUTO_GHOST
int sim_debug_grave(void) {   // (test builds) a gravestone two tiles from you, on your own lot
    int wx = 0, wy = 0; if (home_city && !world_own_block(&wx, &wy)) return 0; if (ngraves >= 6) return 1;
    Sim *s = &sims[player]; graves[ngraves].x = (u8)(sim_tile_x(s) + 2 - wx * BM_BLK); graves[ngraves].y = (u8)(sim_tile_y(s) - wy * BM_BLK); graves[ngraves].city = (u8)home_city;
    strcpy(graves[ngraves].name, "GRANDPA"); ngraves++; return 1;
}
#endif
void sim_grave_named(int i, char *out) { strcpy(out, i >= 0 && i < ngraves ? graves[i].name : "A FRIEND"); }
void sim_say_to(Sim *s, const char *t) { sim_say(s, t); }
int sim_grave_name(int mx, int my, char *out) {
    int city = world_zone == ZONE_CITY, wx, wy;
    if (city) { if (!world_own_block(&wx, &wy)) { strcpy(out, "A FRIEND"); return 0; } mx -= wx * BM_BLK; my -= wy * BM_BLK; }
    for (int i = 0; i < ngraves; i++) if (graves[i].city == city && graves[i].x == mx && graves[i].y == my) { strcpy(out, graves[i].name); return 1; }
    strcpy(out, "A FRIEND"); return 0;
}
static int baby_need(const Sim *b) {   // what a baby is crying for (0: nothing)
    if (b->bore.stage != AG_BABY || b->away) return 0;
    if ((b->need[N_FOOD] >> 10) < 40) return CARE_FEED;
    if ((b->need[N_WC] >> 10) < 40 || (b->need[N_CLEAN] >> 10) < 35) return CARE_CHANGE;
    if ((b->need[N_SOCIAL] >> 10) < 35) return CARE_CUDDLE;
    if ((b->need[N_FUN] >> 10) < 35) return CARE_PLAY;
    return 0;
}
static void baby_to_bed(Sim *b) {   // into the nearest free crib (or down on the floor, if there is none)
    int cx, cy;
    if (b->action != ACT_NONE) end_action(b);
    if (nearest_item(b, CH_CRIB, &cx, &cy)) begin_action(b, ACT_SLEEP, cx, cy);
    else begin_action(b, ACT_SLEEP, sim_tile_x(b), sim_tile_y(b));
}
static void care_start(Sim *s, int baby, int kind) {
    Sim *b = &sims[baby];
    begin_action(s, ACT_CARE, sim_tile_x(b), sim_tile_y(b));
    s->act_var = (u8)kind; s->talk_to = baby;
    if (kind != CARE_BED && b->action == ACT_SLEEP) end_action(b);   // (a baby fed is a baby woken)
}
int sim_is_toddler(const Sim *b) { int L = sim_stage_len(AG_BABY); return b->bore.stage == AG_BABY && L >= 2 && b->age_days * 2 >= L; }
static void milestone(Sim *b, const char *what) {   // a first: everyone who loves the little one hears of it
    char t[48]; snprintf(t, sizeof t, "%s: %s!", b->bore.first, what); sim_note(t); sim_moodlet(b, ML_MILESTONE); sim_cheer = (int)(b - sims) + 1; audio_voice(VOICE_YAHOO);
    for (int j = 0; j < nsims; j++) if (b->par[0] == sims[j].uid || b->par[1] == sims[j].uid) sim_moodlet(&sims[j], ML_MILESTONE);
}
static void teach(Sim *b, u8 *skill, const char *what) { int was = *skill; *skill = (u8)MIN(100, *skill + 25); if (was < 100 && *skill >= 100) milestone(b, what); }
static void care_done(Sim *s) {
    int bi = s->talk_to; if (bi < 0 || bi >= nsims) return;
    Sim *b = &sims[bi]; int k = s->act_var;
    if (b->bore.stage != AG_BABY && k < CARE_STORY) return;
    switch (k) {
        case CARE_FEED: b->need[N_FOOD] += 60 << 10; sim_say(b, b->plant == 2 ? "GLUG GLUG" : "NOM NOM"); break;
        case CARE_CHANGE: b->need[N_WC] = 100 << 10; b->need[N_CLEAN] += 70 << 10; sim_say(b, "AHH"); break;
        case CARE_PLAY: b->need[N_FUN] += 50 << 10; s->need[N_FUN] += 10 << 10; sim_say(b, "HEE HEE!"); break;
        case CARE_CUDDLE: b->need[N_SOCIAL] += 50 << 10; b->need[N_COMFY] += 20 << 10; s->need[N_SOCIAL] += 15 << 10; sim_say(b, "GOO GOO"); break;
        case CARE_BED: baby_to_bed(b); b->need[N_REST] += 20 << 10; break;
        case CARE_STORY: b->need[N_SOCIAL] += 35 << 10; b->need[N_FUN] += 15 << 10; b->need[N_REST] += 10 << 10; b->skill_logic += 25; b->tod_story = (u8)MIN(100, b->tod_story + 15); b->tod_talk = (u8)MIN(100, b->tod_talk + 6); s->skill_creat += 10; sim_say(b, "AGAIN! AGAIN!"); break;
        case CARE_BATH: b->need[N_CLEAN] = 100 << 10; b->need[N_FUN] += 15 << 10; b->need[N_COMFY] += 15 << 10; sim_say(b, "SPLASH!"); break;
        case CARE_WALK: teach(b, &b->tod_walk, "FIRST STEPS"); s->skill_body += 10; sim_say(b, "WOBBLE..."); break;
        case CARE_TALK: { static const char *const w[4] = { "MA MA", "DA DA", "BALL!", "NO!" }; teach(b, &b->tod_talk, "FIRST WORDS"); s->skill_char += 8; sim_say(b, w[look_rand() % 4u]); break; }
        case CARE_POTTY: teach(b, &b->tod_potty, "POTTY TRAINED"); b->need[N_WC] = 100 << 10; s->skill_mech += 0; sim_say(b, "POTTY!"); break;
        case CARE_COMFORT: b->mood = MIN(100, b->mood + 30); b->need[N_SOCIAL] += 30 << 10; b->need[N_COMFY] += 20 << 10; sim_say(b, "SNIFF..."); break;
        case CARE_HWHELP: if (b->homework) { sim_note("THE HOMEWORK IS ALREADY DONE"); break; } homework_done(b); b->grade = (u8)MIN(100, b->grade + 6); s->skill_logic += 15; sim_say(b, "THANKS!"); break;
        case CARE_FEELINGS: b->mood = MIN(100, b->mood + 25); b->need[N_SOCIAL] += 35 << 10; s->skill_char += 20; { int me = (int)(s - sims); b->rel[me] = (s8)MIN(100, b->rel[me] + 6); } sim_say(b, "I FEEL BETTER"); break;
    }
    for (int i = 0; i < NEED_N; i++) { b->need[i] = CLAMP(b->need[i], 0, 100 << 10); s->need[i] = CLAMP(s->need[i], 0, 100 << 10); }
    if (s->uid && (b->par[0] == s->uid || b->par[1] == s->uid)) sim_moodlet(s, ML_PROUD);
    { int me = (int)(s - sims); s->rel[bi] = (s8)MIN(100, s->rel[bi] + 6); b->rel[me] = (s8)MIN(100, b->rel[me] + 6); }   // (looking after a little one: it knows you)
    if (s == &sims[player]) story_event(E_CARE, b->uid);
}
static void homework_done(Sim *s) {
    if (s->homework) return;
    s->homework = 1; s->grade = (u8)MIN(100, s->grade + 8); s->skill_logic += 30;
    sim_moodlet(s, ML_HOMEWORK); wants_event(s, W_HOMEWORK, 0);
    if ((int)(s - sims) == player) sim_note(s->grade >= 85 ? "HOMEWORK DONE: STRAIGHT A'S!" : "HOMEWORK DONE");
}
static int free_floor_near(int lvl, int x0, int y0, int *ox, int *oy) {   // an empty floor tile beside (x0, y0) on that storey, nobody on it
    static const signed char dx[8] = { 1, -1, 0, 0, 1, -1, 1, -1 }, dy[8] = { 0, 0, 1, -1, 1, 1, -1, -1 };
    for (int k = 0; k < 8; k++) {
        int x = x0 + dx[k], y = y0 + dy[k];
        if (x < 1 || y < 1 || x >= MW - 1 || y >= MH - 1 || *world_cell(lvl, x, y) != '.') continue;
        int busy = 0; for (int i = 0; i < nsims; i++) if (sim_tile_x(&sims[i]) == x && sim_tile_y(&sims[i]) == y && sims[i].lvl == lvl) busy = 1;
        if (!busy) { *ox = x; *oy = y; return 1; }
    }
    return 0;
}
static void bake_done(Sim *s) {   // the cake comes out: on its party table beside the cook
    int x, y;
    if (!free_floor_near(s->lvl, sim_tile_x(s), sim_tile_y(s), &x, &y)) { sim_note("NO ROOM HERE FOR THE CAKE TABLE"); return; }
    *world_cell(s->lvl, x, y) = CH_CAKE; world_touched(); cake_slices = 5;
    sim_note("CAKE READY: BLOW THE CANDLES!"); audio_voice(VOICE_FINISHED);
}
static void cake_eaten(Sim *s) {
    if (--cake_slices > 0) return;
    char *c = world_cell(s->lvl, s->act_x, s->act_y);
    if (*c == CH_CAKE) { *c = '.'; world_touched(); }
    cake_slices = 0;
}
static int bday_one(void) { for (int i = 0; i < nsims; i++) if (sims[i].bday && !sims[i].away) return i; return -1; }   // who has a birthday today (and is here)
static void age_up(int i, int party) {   // a birthday: the next life stage, a new body for it, the note, the confetti
    Sim *s = &sims[i]; Bore *b = &s->bore;
    s->bday = 0; s->age_days = 0;
    if (b->stage >= AG_ELDER) return;
    b->stage++;
    look_fix(b);
    if (b->stage == AG_CHILD) { int avg = (s->tod_walk + s->tod_talk + s->tod_potty) / 3, st = s->tod_story;   // (the toddler years pay off)
        s->grade = (u8)MIN(100, s->grade + avg / 8 + st / 12); s->skill_body += s->tod_walk * 2; s->skill_char += s->tod_talk * 2; s->skill_logic += st * 3; s->skill_fun += avg;
        if (avg >= 70) { sim_note("A WELL-TAUGHT TODDLER: A HEAD START AT SCHOOL"); sim_moodlet(s, ML_PROUD); } else if (avg < 30) sim_note("THE TODDLER YEARS WENT BY. SCHOOL WILL BE HARD");
        s->tod_walk = s->tod_talk = s->tod_potty = s->tod_story = 0; }
    if (b->stage == AG_CHILD) b->asp = AS_GROW;                                       // a child's aspiration: GROW UP
    if (b->stage == AG_ADULT && b->asp == AS_GROW) b->asp = (u8)(look_rand() % AS_PICK);   // grown up: an aspiration of their own
    look_build(b); crowd_forget(CROWD_MAX + i);
    if (s->action == ACT_SLEEP || s->action == ACT_SIT) end_action(s);
    wants_reroll(s);
    char t[48]; const char *st = look_stage_name(b->stage);
    sprintf(t, "%s IS NOW %s %s!", b->first, (st[0] == 'A' || st[0] == 'E') ? "AN" : "A", st); sim_note(t);
    sim_moodlet(s, party ? ML_BIRTHDAY : ML_FUN_TIME);
    if (party) for (int j = 0; j < nsims; j++) if (j != i && !sims[j].away) sim_moodlet(&sims[j], ML_FUN_TIME);
    sim_cheer = i + 1;
    audio_voice(VOICE_YAHOO);
}
static void candles(Sim *s) {   // the birthday Bore blows out the candles: beside you, at the cake
    int bi = bday_one(); if (bi < 0) return;
    Sim *b = &sims[bi];
    if (b != s) { if (b->action != ACT_NONE) end_action(b); b->path_n = 0; b->x = s->x + ((isin((s->yaw + 1024) & 4095) * 70) >> 12); b->z = s->z - ((icos((s->yaw + 1024) & 4095) * 70) >> 12); b->lvl = s->lvl; b->yaw = s->yaw; }
    sim_say(b, "WHOOOOSH!");
    age_up(bi, 1);
}
char sim_end_name[NAME_LEN];
static void remove_sim(int i) {   // gone from the household: everyone after moves up a place
    strcpy(sim_end_name, sims[i].bore.last);
    look_release(&sims[i].bore);                                              // (its face textures: for the next baby)
    for (int j = i; j < nsims - 1; j++) sims[j] = sims[j + 1];
    nsims--;
    for (int j = 0; j < nsims; j++) {
        Sim *o = &sims[j];
        for (int k = i; k < MAX_SIMS - 1; k++) o->rel[k] = o->rel[k + 1];
        o->rel[MAX_SIMS - 1] = 0;
        if (o->talk_to == i) { o->talk_to = -1; o->errand = 0; if (o->action == ACT_TALK || o->action == ACT_CARE || o->action == ACT_WALK) { o->action = ACT_NONE; o->path_n = 0; o->gait.wave = 0; } }
        else if (o->talk_to > i) o->talk_to--;
    }
    if (player == i || player >= nsims) {
        player = 0; for (int j = 0; j < nsims; j++) if (playable(j)) { player = j; break; }
        if (nsims) { sims[player].ai = 0; char t[40]; snprintf(t, sizeof t, "NOW PLAYING AS %s", sims[player].bore.first); sim_note(t); }
    } else if (player > i) player--;
    for (int j = i; j <= nsims; j++) crowd_forget(CROWD_MAX + j);   // (their sprites moved slots)
    int any = 0; for (int j = 0; j < nsims; j++) if (playable(j)) any = 1;
    if (!any) sim_story_over = 1;
}
int sim_move_out(int i, char *where) {   // household member i goes to live in a home of their own in SAN BOREAS (1: gone; where: their address)
    if (i < 0 || i >= nsims || i == player) return 0;
    Sim *o = &sims[i]; int st = o->bore.stage; if (st != AG_ADULT && st != AG_ELDER) return 0;
    int nbx = home_city ? home_bx : bm_set(bm_capital())->x, nby = home_city ? home_by : bm_set(bm_capital())->y;
    int pid = pop_move_out(&o->bore, nbx, nby, home_city ? home_bx : -1, home_city ? home_by : -1, MAX(50, o->rel[player]));
    if (pid < 0) return 0;
    const Fam *fm = &pop_fam[pop_fam_of(pid)]; home_address(fm->bx, fm->by, where);
    for (int j = 0; j < nsims; j++) if (sims[j].partner == o->uid) sims[j].partner = 0;
    remove_sim(i);
    return 1;
}
static int grave_spot(int *ox, int *oy) {   // out in the yard: grass, nothing on it, near the front door (in SAN BOREAS, on the family's own lot)
    int sx, sy, x0 = 1, y0 = 1, x1 = MW - 2, y1 = MH - 2;
    if (home_city) { int wx, wy; if (!world_own_door(&sx, &sy) || !world_own_block(&wx, &wy)) return 0;
                     x0 = wx * BM_BLK + BM_LOT0; y0 = wy * BM_BLK + BM_LOT0; x1 = wx * BM_BLK + BM_BLK - 1; y1 = wy * BM_BLK + BM_BLK - 1; }
    else { world_find('P', &sx, &sy); if (sx < 0) { sx = sim_tile_x(&sims[player]); sy = sim_tile_y(&sims[player]); } }
    for (int r = 2; r < 14; r++) for (int dy = -r; dy <= r; dy++) for (int dx = -r; dx <= r; dx++) {
        if (ABS(dx) != r && ABS(dy) != r) continue;
        int x = sx + dx, y = sy + dy;
        if (x < x0 || y < y0 || x > x1 || y > y1 || lifeMap[y][x] != '.' || floorMap[y][x] < FL_GRASS || floorMap[y][x] == FL_WATER || floorMap[y][x] > FL_SAND) continue;
        *ox = x; *oy = y; return 1;
    }
    return 0;
}
static void pass_on(int i) {   // the end of a long life: gently
    Sim *s = &sims[i]; char t[48];
    sprintf(t, "%s HAS PASSED AWAY", s->bore.first); sim_note(t);
    int gx, gy;
    if (home_is_here() && ngraves < 6 && grave_spot(&gx, &gy)) {   // (at home: in the yard)
        int wx = 0, wy = 0, city = home_city; if (city) world_own_block(&wx, &wy);
        lifeMap[gy][gx] = CH_GRAVE; graves[ngraves].x = (u8)(gx - wx * BM_BLK); graves[ngraves].y = (u8)(gy - wy * BM_BLK); graves[ngraves].city = (u8)city;
        strcpy(graves[ngraves].name, s->bore.first); ngraves++; world_touched();
        sim_note("A STONE STANDS IN THE YARD"); }
    for (int j = 0; j < nsims; j++) if (j != i) { sim_moodlet(&sims[j], ML_GRIEF); wants_fear(&sims[j], F_DEATH); }
    audio_voice(VOICE_CRY);
    remove_sim(i);
}
static void genes(Bore *b, const Bore *m, const Bore *f) {   // a little of each parent: the skin, the hair and eye colours, the shape, the ears, the parts
    static const u8 ids[] = { LK_SKIN, LK_HCOL, LK_EYECOL, LK_EYES, LK_EARS, LK_NOSE, LK_BROW, LK_SHAPE, LK_TAIL, LK_HORNS, LK_ANTENNA, LK_PATTERN, LK_PATCOL };
    for (unsigned k = 0; k < sizeof ids; k++) {
        const Bore *src = (m && f) ? ((look_rand() & 1) ? m : f) : m ? m : f; if (!src) return;
        int v = src->look[ids[k]]; if (look_allowed(b, ids[k], v)) b->look[ids[k]] = (u8)v;
    }
}
static void born(void) {   // the baby comes home (or the adopted child)
    baby_due = -1;
    if (nsims >= MAX_SIMS) { sim_note("NO ROOM: 8 BORES IS THE MOST"); return; }
    Sim *ma = by_uid(baby_par[0]), *pa = by_uid(baby_par[1]);
    Bore b; look_default(&b); b.stage = baby_kind == 2 ? AG_CHILD : AG_BABY; look_random(&b);
    if (!baby_kind || baby_kind >= 3) genes(&b, ma ? &ma->bore : 0, pa ? &pa->bore : 0);
    int seedling = baby_kind == 5 || (ma && ma->plant == 2) || (pa && pa->plant == 2);   // PLANTBORE: a seedling, from a plant parent
    if (seedling) { b.look[LK_SKIN] = 4; b.look[LK_HCOL] = 5; }
    if (baby_kind >= 3 && baby_kind < 5) {   // an alien's: 3 a normal baby, a little more special; 4 out of this world
        static const u8 alk[6] = { LK_ANTENNA, LK_HORNS, LK_TAIL, LK_PATTERN, LK_EYECOL, LK_SKIN };
        static const char *const an[8] = { "ZORP", "XEEN", "BLIP", "NEBULA", "QUASAR", "ORBIT", "COMET", "VEGA" };
        int n = baby_kind == 4 ? 6 : 1, from = baby_kind == 4 ? 0 : 4;
        for (int k = 0; k < n; k++) { int id = baby_kind == 4 ? alk[k] : alk[from + (int)(look_rand() % 2u)], c = look_count(id), v = c > 1 ? 1 + (int)(look_rand() % (u32)(c - 1)) : 0; if (look_allowed(&b, id, v)) b.look[id] = (u8)v; }
        for (int k = 0; k < (baby_kind == 4 ? 3 : 1); k++) b.tr[look_rand() % TR_N] = (u8)(baby_kind == 4 ? 10 : 8);
        if (baby_kind == 4) strcpy(b.first, an[look_rand() % 8u]);
    }
    strcpy(b.last, (ma ? ma : pa ? pa : &sims[player])->bore.last);
    b.asp = AS_GROW; look_fix(&b); look_build(&b);
    const Sim *nr = ma ? ma : pa ? pa : &sims[player];
    int tx = sim_tile_x(nr), ty = sim_tile_y(nr);
    { int x, y; if (free_floor_near(nr->lvl, tx, ty, &x, &y)) { tx = x; ty = y; } }
    int keep = world_lvl; world_lvl = nr->lvl;
    sim_add(&b, tx, ty, 1);
    world_lvl = keep;
    Sim *n = &sims[nsims - 1]; n->par[0] = baby_par[0]; n->par[1] = baby_par[1]; n->lvl = nr->lvl;
    if (seedling) { n->plant = 2; n->skill_farm += 400; n->plant_sk = 2; n->plant_hc = 2; n->need[N_FOOD] = 100 << 10; }
    if (baby_kind >= 3 && baby_kind < 5) { n->skill_fun += baby_kind == 4 ? 800 : 300; n->mood = MIN(100, n->mood + 20); }
    if (b.stage == AG_BABY && home_is_here() && n->lvl == 0) {   // a crib comes with a baby (when the house has none)
        int have = 0, cx, cy;
        for (int y = 0; y < MH && !have; y++) for (int x = 0; x < MW; x++) if (lifeMap[y][x] == CH_CRIB || lifeMap[y][x] == CH_BASS || lifeMap[y][x] == CH_CRAD) { have = 1; break; }
        if (!have && free_floor_near(0, tx, ty, &cx, &cy)) { lifeMap[cy][cx] = CH_CRIB; world_touched(); sim_note("A CRIB CAME WITH THE BABY"); }
    }
    char t[48];
    if (seedling) snprintf(t, sizeof t, "A SEEDLING! MEET %s", b.first); else if (baby_kind >= 3) snprintf(t, sizeof t, baby_kind == 4 ? "A STRANGE ONE! MEET %s" : "A SPECIAL BABY! MEET %s", b.first); else if (baby_kind) snprintf(t, sizeof t, "WELCOME HOME, %s!", b.first); else snprintf(t, sizeof t, "THE BABY IS HERE! MEET %s", b.first);
    sim_note(t);
    for (int j = 0; j < nsims - 1; j++) if (sims[j].uid && (sims[j].uid == baby_par[0] || sims[j].uid == baby_par[1])) { sim_moodlet(&sims[j], ML_NEW_BABY); wants_event(&sims[j], W_BABY, 0); }
    sim_cheer = -nsims;                                                   // (negative: a newcomer, not a birthday)
    audio_voice(VOICE_YAHOO);
}
static void start_family(Sim *s, Sim *o) {   // two grown-ups who get on: a baby on the way
    int who = (int)(o - sims), me = (int)(s - sims);
    int chance = 30 + s->rel[who] + o->mood / 4 + SKL(s->skill_char) * 2;
    if ((int)(look_rand() % 100) >= chance) { sim_say(o, "NOT YET..."); s->rel[who] -= 3; wants_fear(s, F_REJECT); audio_voice(VOICE_DISAGREE); return; }
    s->partner = o->uid; o->partner = s->uid;
    baby_par[0] = s->uid; baby_par[1] = o->uid; baby_kind = 0;
    int days = aging == 1 ? 1 : aging == 3 ? 3 : 2;
    baby_due = life_minutes() + days * 1440;
    sim_say(o, "A BABY? YES!"); sim_moodlet(s, ML_LOVE); sim_moodlet(o, ML_LOVE);
    char t[40]; sprintf(t, "A BABY IS ON THE WAY! IN %d DAY%s", days, days > 1 ? "S" : ""); sim_note(t);
    audio_voice(VOICE_FLIRT);
    if (o->action != ACT_NONE && o->action != ACT_WALK) end_action(o);
    begin_action(s, ACT_TALK, sim_tile_x(o), sim_tile_y(o)); begin_action(o, ACT_TALK, sim_tile_x(s), sim_tile_y(s));
    s->talk_to = who; o->talk_to = me; o->path_n = 0; o->gait.wave = 40;
}
static void go_steady(Sim *s, Sim *o) {   // two who are sweet on each other make it official (the family tree: partners)
    int who = (int)(o - sims), me = (int)(s - sims);
    if ((int)(look_rand() % 100) >= 40 + s->rel[who] / 2 + o->mood / 4 + SKL(s->skill_char) * 2) { sim_say(o, "LET'S NOT RUSH IT"); s->rel[who] -= 3; wants_fear(s, F_REJECT); audio_voice(VOICE_DISAGREE); return; }
    s->partner = o->uid; o->partner = s->uid;
    sim_say(o, "YES! US, OFFICIALLY"); sim_moodlet(s, ML_LOVE); sim_moodlet(o, ML_LOVE); audio_voice(VOICE_FLIRT);
    char t[40]; snprintf(t, sizeof t, "GOING STEADY WITH %s", o->bore.first); sim_note(t);
    if (me == player) story_event(E_STEADY, o->uid);
    if (o->action != ACT_NONE && o->action != ACT_WALK) end_action(o);
    begin_action(s, ACT_TALK, sim_tile_x(o), sim_tile_y(o)); begin_action(o, ACT_TALK, sim_tile_x(s), sim_tile_y(s));
    s->talk_to = who; o->talk_to = me; o->path_n = 0; o->gait.wave = 40;
}
void sim_baby_coming(int p0, int p1, int minutes) { baby_par[0] = (u8)p0; baby_par[1] = (u8)p1; baby_kind = 0; baby_due = life_minutes() + minutes; sim_note("A BABY COMES HOME TOMORROW"); }
static void adopt(Sim *s) {
    if (baby_due >= 0) { sim_note("A BABY IS ALREADY ON THE WAY"); return; }
    if (nsims >= MAX_SIMS) { sim_note("THE HOUSE IS FULL (8 BORES)"); return; }
    if (cash < 1000) { sim_note("ADOPTING IS $1000"); audio_voice(VOICE_NAH); return; }
    cash -= 1000;
    baby_kind = (u8)(1 + (look_rand() & 1)); baby_par[0] = s->uid; baby_par[1] = s->partner; baby_due = life_minutes() + 1440;
    sim_note(baby_kind == 1 ? "ADOPTING A BABY: HOME TOMORROW" : "ADOPTING A CHILD: HOME TOMORROW");
    audio_voice(VOICE_YAHOO);
}
// ---- the phone in your pocket (main.c's PHONE): a call where you stand, a pizza, adopting, playing as someone else ----
int sim_cell(int what) {   // 0 a chat with a friend, 1 a pizza ($40), 2 adopt ($1000, a grown-up), 3 a text, 4 a photo, 5 a game on the handheld. 1: done
    Sim *s = &sims[player];
    if (s->bore.stage == AG_BABY) return 0;
    if (what == 2) { if (s->bore.stage != AG_ADULT && s->bore.stage != AG_ELDER) { sim_note("ONLY A GROWN-UP CAN ADOPT"); return 0; } adopt(s); return 1; }
    if (s->action != ACT_NONE && s->action != ACT_WALK) end_action(s);
    if (what == 1) { if (cash < 40) { sim_note("PIZZA IS $40"); audio_voice(VOICE_NAH); return 0; } cash -= 40; sim_note("PIZZA: $40. IT IS ON ITS WAY"); }
    if (what == 4) {   // a photo: no standing about, a smile and a click
        static const char *const pic[4] = { "SNAP! A SELFIE FOR THE ALBUM", "SNAP! THAT ONE IS A KEEPER", "SNAP! EYES SHUT. AGAIN?", "SNAP! LOOKING GOOD" };
        sim_note(pic[(clock_min + day_count) & 3]); s->need[N_FUN] = MIN(s->need[N_FUN] + (8 << 10), 100 << 10); s->gait.wave = 2; audio_voice(VOICE_YAHOO); return 1;
    }
    s->path_n = 0; s->action = ACT_NONE;
    begin_action(s, ACT_PHONE, sim_tile_x(s), sim_tile_y(s)); s->act_var = (u8)what;
    if (!what) sim_note("CALLING A FRIEND");
    if (what == 3) { static const char *const tx[4] = { "TEXTING A FRIEND: \"SUP?\"", "TEXTING A FRIEND: \"MISS YOU!\"", "TEXTING A FRIEND: \"LOL\"", "TEXTING A FRIEND: \"COFFEE?\"" };
                     sim_note(tx[(clock_min + day_count) & 3]); s->act_t = 120; }
    if (what == 5) { sim_note(handheld & 2 ? "ON THE BOREBOY ADVANCE: ONE MORE LEVEL" : handheld ? "ON THE BOREBOY: ONE MORE LEVEL" : "A QUICK GAME ON THE PHONE"); s->act_t = 360; }
    return 1;
}
int sim_play_as(int i) {   // play as that one of the family (not a baby, not away). 1: done
    if (i == player || i < 0 || i >= nsims || sims[i].bore.stage == AG_BABY || sims[i].away) return 0;
    sims[player].ai = 1; player = i; sims[player].ai = 0; sims[player].path_n = 0;
    return 1;
}
static int school_age(const Sim *k) { return k->bore.stage == AG_CHILD || k->bore.stage == AG_TEEN; }
static void school_go(void) {   // 8 AM, MONDAY TO FRIDAY: the kids and the teens are off (the one you play hands over, if someone can)
    int went = 0;
    for (int i = 0; i < nsims; i++) {
        Sim *k = &sims[i]; if (!school_age(k) || k->away) continue;
        if (i == player) {
            int n = -1; for (int j = 0; j < nsims; j++) if (j != i && playable(j) && !school_age(&sims[j])) { n = j; break; }
            if (n < 0) { k->grade = (u8)MAX(0, k->grade - 12); sim_note("SKIPPED SCHOOL TODAY"); continue; }
            sims[player].ai = 1; player = n; sims[player].ai = 0; sims[player].path_n = 0;
        }
        if (k->action != ACT_NONE) end_action(k);
        k->path_n = 0; k->away = 1; k->homework = 0; k->errand = 0; k->talk_to = -1; went++;
    }
    if (went) sim_note(went > 1 ? "THE KIDS ARE OFF TO SCHOOL" : "OFF TO SCHOOL");
}
static void school_back(void) {   // 3 PM: home again, by the one you play (wherever the family is)
    const Sim *p = &sims[player]; int n = 0;
    for (int i = 0; i < nsims; i++) {
        Sim *k = &sims[i]; if (!k->away) continue;
        k->away = 0; k->lvl = p->lvl;
        k->x = p->x + (n % 2 ? 90 : -90); k->z = p->z - 60 * (n / 2) - 60; n++;
        k->grade = (u8)MIN(100, k->grade + 6);
        k->need[N_FUN] = MAX(0, k->need[N_FUN] - (20 << 10)); k->need[N_SOCIAL] = MIN(100 << 10, k->need[N_SOCIAL] + (15 << 10));
        sim_moodlet(k, ML_SCHOOL);
    }
    if (n) sim_note("HOME FROM SCHOOL: HOMEWORK!");
}
const char *sim_life_text(const Sim *s, int full) {   // INFANT, TODDLER, CHILD.. with the days into it; a plant sim's greying in place of old age
    static char b[40]; int g = s->bore.stage, L = sim_stage_len(g); const char *st = look_stage_name(g);
    if (g == AG_BABY) st = sim_is_toddler(s) ? "TODDLER" : "INFANT";
    if (!full) { snprintf(b, sizeof b, "%s", st); return b; }
    if (s->plant == 2 && g == AG_ADULT) { int T = L + sim_stage_len(AG_ELDER); snprintf(b, sizeof b, "PLANT ADULT, GREY %d OF 4", T ? MIN(3, s->age_days * 4 / T) + 1 : 1); return b; }
    if (L && g < AG_ELDER) snprintf(b, sizeof b, "%s, DAY %d OF %d", st, MIN(s->age_days + 1, L), L); else snprintf(b, sizeof b, "%s", st);
    return b;
}
static void plant_grey(Sim *s, int note) {   // a plant sim ages by greying: green, yellowed, grey-green, grey (the same four stages, no elder body)
    int T = sim_stage_len(AG_ADULT) + sim_stage_len(AG_ELDER); if (T < 1) return;
    int g = MIN(3, s->age_days * 4 / T); static const u8 sk[4] = { 4, 20, 22, 23 }, hc[4] = { 5, 5, 4, 4 };
    if (s->bore.look[LK_SKIN] == sk[g] && s->bore.look[LK_HCOL] == hc[g]) return;
    s->bore.look[LK_SKIN] = sk[g]; s->bore.look[LK_HCOL] = hc[g]; look_build(&s->bore); crowd_forget(CROWD_MAX + (int)(s - sims));
    if (note) { char t[48]; snprintf(t, sizeof t, g == 3 ? "%s IS GOING GREY AND WILTING" : "%s IS GREYING", s->bore.first); sim_note(t); }
}
static void plant_adult_tick(Sim *s, int i) {
    int T = sim_stage_len(AG_ADULT) + sim_stage_len(AG_ELDER);
    plant_grey(s, 1);
    if (T && s->age_days >= T) { if (old_age && s->age_days > T) pass_on(i); else if (s->age_days == T) { char t[48]; snprintf(t, sizeof t, "%s IS WILTING. A SEEDLING WOULD CARRY ON", s->bore.first); sim_note(t); } }
}
static void midnight_ages(void) {   // a day older: birthdays come round, and at the very end of a long life, goodbye
    for (int i = 0; i < nsims; i++) if (sims[i].plant == 2) { int se = sim_season(); if (se == 0) sim_moodlet(&sims[i], ML_BLOOM); else if (se == 3) { sim_moodlet(&sims[i], ML_DORMANT); sims[i].need[N_REST] = MAX(0, sims[i].need[N_REST] - (25 << 10)); } }   // (plants bloom in spring, doze in winter)
    if (aging) for (int i = nsims - 1; i >= 0; i--) {   // (backwards: someone may leave)
        Sim *s = &sims[i]; int L = sim_stage_len(s->bore.stage);
        if (s->bday) { age_up(i, 0); continue; }                                    // nobody baked: they grow up quietly overnight
        if (s->age_days < 255) s->age_days++;
        if (sim_is_toddler(s)) { s->tod_walk = (u8)MIN(100, s->tod_walk + 4); s->tod_talk = (u8)MIN(100, s->tod_talk + 4); s->tod_potty = (u8)MIN(100, s->tod_potty + 3); }   // (a little learned on its own)
        if (s->plant == 2 && s->bore.stage == AG_ADULT) { plant_adult_tick(s, i); continue; }   // (a plant sim never grows old: it greys)
        if (s->plant == 2 && s->bore.stage == AG_ELDER && s->age_days == 1) { /* (a stalk turned plant late in life: stays as it is) */ }
        if (!L || s->age_days < L) continue;
        if (s->bore.stage == AG_ELDER) {
            char t[48];
            if (old_age && s->age_days > L) pass_on(i);
            else if (s->age_days == L) { snprintf(t, sizeof t, "%s IS GETTING VERY OLD", s->bore.first); sim_note(t); }
            continue;
        }
        s->bday = 1; char t[48]; snprintf(t, sizeof t, "IT'S %s'S BIRTHDAY!", s->bore.first); sim_note(t); sim_note("BAKE A CAKE AT THE FRIDGE");
    }
    if (clock_day == 0 || clock_day >= 6 || day_count <= 1) return;   // (the day just gone was a school day: TUESDAY .. SATURDAY now; not moving day)
    for (int i = 0; i < nsims; i++) { Sim *k = &sims[i];
        if (!school_age(k)) continue;
        if (!k->homework) k->grade = (u8)MAX(0, k->grade - 6);
        if (k->grade >= 85) sim_moodlet(k, ML_GRADES); }
}

static void clock_minute(void);
COLD static void rt_follow(u32 now) {   // REAL TIME: the clock to the real one, the minutes gone by played through (at most a day of them)
    if (now == rt_last) { if (clock_min != (int)(now % 1440)) rt_put(now); return; }   // (something moved the clock: back it goes)
    if (!rt_last || now < rt_last) { rt_put(now); rt_last = now; job_worked = 0; return; }   // (just switched on, a new life, or the clock set back: straight there)
    u32 d = now - rt_last; if (d > 1440) { rt_last = now - 1440; d = 1440; }   // (away more than a day: the last day's hours go by, the rest is skipped)
    rt_put(rt_last); rt_last = now;
    while (d--) clock_minute();
    rt_put(now);
}
static void clock_tick(void) {
    static int acc = 0, wcap = -1;
    if (sim_rt) { u32 now = sim_rt_now(); rt_on = now != 0;
        if (rt_on) { rt_follow(now);
            if (sims[player].action == ACT_WORK && is_work()) {   // at work: the shift's work done in a blink (a minute a step), paid when the shift ends
                const Career *cr = &careers[job_car]; if (wcap < 0) wcap = MIN((cr->to - cr->from) * 60, job_worked + cr->to * 60 - clock_min);
                if (job_worked < wcap) job_worked++; else { end_action(&sims[player]); sim_note("SHIFT DONE. PAID WHEN IT ENDS"); }
            } else wcap = -1;
            return; } }
    int spm = STEPS_MIN * sim_day_pct / 100;   // (OPTIONS > GAME > DAY LENGTH)
    if ((sims[player].action == ACT_SLEEP && sims[player].act_var != 2) || sims[player].action == ACT_WORK) spm = 1;   // asleep or at work: the hours fly (not on a massage table)
    if (sim_speed == 2) spm /= 3; if (sim_speed == 3) spm = 1;
    if (spm < 1) spm = 1;
    if (++acc < spm) return;
    acc = 0;
    clock_minute();
}
COLD static void clock_minute(void) {
    clock_min++;
    if (!(clock_min % 60)) pop_hour();   // (the lives of the ones who know you, off the map: an hour at a time)
    if (!(clock_min % 30) && clock_min >= 8 * 60 && clock_min < 19 * 60 && world_zone == ZONE_CITY && fm_total()) {   // FARM LIFE: by a farm stand with food to sell, the townsfolk come to buy
        int px = sim_tile_x(&sims[player]), py = sim_tile_y(&sims[player]), y0 = MAX(0, py - 10), y1 = MIN(MH - 1, py + 10), x0 = MAX(0, px - 10), x1 = MIN(MW - 1, px + 10);
        for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if ((u8)lifeMap[y][x] == 0xDE) { citizens_send_buyer(x, y); y = y1; break; } }
    for (int i = 0; i < nsims; i++) { wants_minute(&sims[i]); moodlets_minute(&sims[i]); }
    gotchi_minute();
    if (clock_min == 16 * 60) for (int i = 0; i < nsims; i++) for (int j = 0; j < nsims; j++) { int r = sims[i].rel[j]; sims[i].rel[j] = (s8)(r > 0 ? r - 2 : r < 0 ? r + 2 : 0); }   // 4 PM: the day's warmth fades a little
    if (sims[player].action == ACT_WORK && is_work()) job_worked++;       // minutes worked this shift
    if (clock_day < 5 && day_count > 0) {   // school: the bus at 8, home at 3 (not on the day you move in)
        if (clock_min == 360) {   // 6 AM: the morning paper (borrowed from BORE on the GBA): the headline, and three jobs in the classifieds
            static const char *const news[12] = { "SKATE PLAZA PACKED ALL WEEKEND", "LOCAL BORE LANDS A KICKFLIP", "PIZZA PRICES UP AGAIN", "MYSTERY LIGHTS OVER THE POOL",
                "COUNCIL BANS FUN. FUN OBJECTS", "RAMP BUILDER WINS AWARD", "LOST CAT FOUND IN A TREE", "NEW PHONE OUT. QUEUES ROUND THE BLOCK",
                "NEW CAFE HAS A LONG LINE", "HOT WEEK AHEAD, SAYS EXPERT", "SLOW NEWS DAY. NOTHING HAPPENS", "NEIGHBOURS COMPLAIN ABOUT NOISE" };
            char b[48]; snprintf(b, sizeof b, "THE PAPER: %s", news[day_count % 12]); sim_note(b); sim_note("3 NEW JOBS IN THE CLASSIFIEDS: PHONE, JOBS");
        }
        if (clock_min == 465) { for (int i = 0; i < nsims; i++) if (school_age(&sims[i])) { sim_note("THE SCHOOL BUS COMES AT 8"); break; } }
        if (clock_min == 480) school_go();
        if (clock_min == 900) school_back();
    }
    if (baby_due >= 0 && life_minutes() >= baby_due) born();
    {   // the shift: a reminder an hour before, the start, and the pay at the end (by the minutes worked)
        const Career *cr = &careers[job_car]; int on = !job_none && ((cr->days >> clock_day) & 1), from = cr->from * 60, to = cr->to * 60;
        char b[40], h[8];
        if (on && clock_min == from - 60) { sprintf(b, "WORK AT %s: PHONE, JOBS", hour_name(cr->from, h)); sim_note(b); }
        if (on && clock_min == from) { snprintf(b, sizeof b, "WORK NOW: %s", job_place); sim_note(b); job_worked = 0; }
        if (on && clock_min == to) {
            int len = to - from, w = MIN(job_worked, len), full = job_pay(), pay = 0;
            if (w >= len * 55 / 100) { pay = full * w / len; if (w >= len * 95 / 100) pay += full / 5; job_days_good++; pay = pay * (100 + SKL(sims[player].skill_char) * 2) / 100; }   // (a charmer gets the better deal)
            else if (w >= len / 4) pay = full * w / len;
            else job_strikes++;
            if (pay) { cash += pay; story_paid++; sprintf(b, "PAID $%d", pay); sim_note(b); wants_event(&sims[player], W_PAID, 0); sim_moodlet(&sims[player], ML_PAYDAY);
                       if (story_paid == 1) sim_note("THE ISLAND FERRY IS OPEN NOW!"); audio_voice(VOICE_YAHOO); }
            else { sim_note("NO PAY TODAY, SORRY"); wants_fear(&sims[player], F_BADDAY); }
            if (job_days_good >= (SKL(sims[player].skill_char) >= 5 ? 2 : 3) && job_level < 5) { job_level++; job_days_good = 0; job_strikes = 0; snprintf(b, sizeof b, "PROMOTED: %s!", job_title()); sim_note(b);
                                                       wants_event(&sims[player], W_PROMO, 0); sim_moodlet(&sims[player], ML_PROMOTED); }
            if (job_strikes >= 3 && job_level > 0) { job_level--; job_strikes = 0; sim_note("OUCH. YOU GOT DEMOTED"); wants_fear(&sims[player], F_DEMOTED); }
        }
    }
    if (clock_min >= 1440) {
        clock_min = 0; clock_day = (clock_day + 1) % 7; day_count++;
        if (rt_on) rt_put((u32)(rt_dayn + 1) * 1440);   // (REAL TIME: tomorrow's date; a season comes on the 1st of MARCH, JUNE, SEPTEMBER, DECEMBER)
        if (rt_on ? rt_mday == 1 && rt_mon % 3 == 2 : day_count % 10 == 0) { char sb_[40]; sprintf(sb_, "%s HAS COME!", sim_season_name(sim_season())); sim_note(sb_); }
        else if (fest_day()) { char sb_[40]; sprintf(sb_, "TODAY: THE %s! 10 AM", sim_festival_name(sim_season())); sim_note(sb_); }
        midnight_ages(); pop_midnight(); crime_midnight(); farm_midnight();
        if (royalty) { char rb_[32]; cash += royalty; sprintf(rb_, "ROYALTIES: +$%d", royalty); sim_note(rb_); }                                    // (your household, and everyone else)
        if (job_none) sim_note(day_count & 1 ? "NO JOB, NO BILLS. THE PAPER HAS JOBS AT 6" : "STILL LOOKING FOR WORK? PHONE, JOBS");
        else cash -= BILLS;
        if (job_none) { }
        else if (cash < 0) { cash = 0; sim_note("BROKE! THE BILLS WENT UNPAID"); sims[player].mood -= 30; audio_voice(VOICE_CRY); wants_fear(&sims[player], F_BROKE); }
        else sim_note("BILLS PAID: 800");
    }
}

u8 art_n, novel; u16 art_q, royalty;
// FARM LIFE: the crops (planted, days grown, thirsty or watered, days dry), the hens and cows, what you have picked and what waits at the coop and the stalls
u8 fm_fert, fm_fz, fm_pest, fm_up, fm_wc, fm_ordk, fm_ordq, fm_ordt, fm_fair;   // fertilizer in the shed, which crops have it, pests, upgrades (1 sprinkler 2 scarecrow 4 greenhouse 8 tractor), water given today, the order board, the fair
u8 fm_n[4], fm_g[4], fm_wet, fm_dry[4], fm_hens, fm_cows, fm_fed; u16 fm_stock[4], fm_egg, fm_milk, fm_pe, fm_pm;
u8 fm_hh = 70, fm_ch = 70, fm_pet; u16 fm_buyers; int fm_earned;   // (save version 43: see sim.h)
const char *const fmGood[6] = { "WHEAT", "CORN", "TOMATOES", "PUMPKINS", "EGGS", "MILK" }; const u8 fmPr[6] = { 4, 7, 9, 16, 5, 10 };
#define fmNm fmGood
int fm_need(int k) { static const u8 d[4] = { 2, 3, 3, 4 }; int n = d[k] - (SKL(sims[player].skill_farm) >= 5 ? 1 : 0); return n < 2 ? 2 : n; }
int fm_in_season(int k) { static const u8 ls[4] = { 5, 2, 3, 6 }; int se = sim_season(); return se < 3 && ((ls[k & 3] >> se) & 1); }   // WHEAT spring and autumn, CORN summer, TOMATOES spring and summer, PUMPKINS summer and autumn
int fm_have(int k) { return k < 4 ? fm_stock[k] : k == 4 ? fm_egg : fm_milk; }
static void fm_take(int k, int n) { if (k < 4) fm_stock[k] -= (u16)n; else if (k == 4) fm_egg -= (u16)n; else fm_milk -= (u16)n; }
int fm_total(void) { int t = 0; for (int k = 0; k < 6; k++) t += fm_have(k); return t; }
int farm_going(void) { return (fm_n[0] | fm_n[1] | fm_n[2] | fm_n[3] | fm_hens | fm_cows) || (job_car == CAR_FARM && !job_none) || fm_total(); }
static void fm_sold(Sim *s, int v) { fm_earned += v; wants_event(s, W_FARMSELL, 0); if (v >= 40) sim_moodlet(s, ML_MARKET); }
const char *farm_customer(int *sold) {   // a townsperson at the farm stand buys 1 to 4 of something there is, at the stand's price (half as much again as the market)
    static const char *const ln[6] = { "FRESH WHEAT!", "SWEET CORN!", "RED TOMATOES!", "A BIG PUMPKIN!", "FRESH EGGS!", "FRESH MILK!" };
    int k0 = (int)(look_rand() % 6u), k = -1; *sold = 0;
    for (int i = 0; i < 6 && k < 0; i++) if (fm_have((k0 + i) % 6)) k = (k0 + i) % 6;
    if (k < 0) return "SOLD OUT? OH WELL";
    int n = MIN(fm_have(k), 1 + (int)(look_rand() % 4u)), v = n * fmPr[k] * 3 / 2; char b[40];
    fm_take(k, n); cash += v; fm_buyers++; *sold = v; snprintf(b, sizeof b, "SOLD %d %s AT THE STAND: +$%d", n, fmGood[k], v); sim_note(b); fm_sold(&sims[player], v);
    return ln[k];
}
void farm_save(void) { sv_u8(fm_hh); sv_u8(fm_ch); sv_u8(fm_pet); sv_u16(fm_buyers); sv_u32((u32)fm_earned); }
void farm_load(void) { int h = ld_u8(), c = ld_u8(); fm_hh = (u8)MIN(100, h); fm_ch = (u8)MIN(100, c); fm_pet = (u8)ld_u8();   // (MIN reads its arguments twice: read first)
    fm_buyers = (u16)ld_u16(); fm_earned = (int)ld_u32(); }
static void pass_on(int i);
static void farm_midnight(void) {
    int se = sim_season(), wx = sim_weather(), pb = 0, lost = 0, gh = fm_up & 4;
    for (int i = 0; i < nsims; i++) if (sims[i].plant == 2) pb = 1;
    if ((fm_up & 1) || wx == 1) fm_wet = 15;                       // (a sprinkler, or the rain)
    for (int k = 0; k < 4; k++) { if (!fm_n[k]) continue;
        if (se == 3 && !gh) { fm_n[k] = 0; fm_g[k] = 0; fm_dry[k] = 0; fm_fz &= ~(1 << k); fm_pest &= ~(1 << k); lost = 1; sim_note("FROST KILLED A CROP. A GREENHOUSE HELPS"); continue; }   // (winter: they wilt)
        if ((fm_pest >> k) & 1) { fm_pest &= ~(1 << k); fm_n[k] = (u8)((fm_n[k] + 1) / 2); lost = 1; sim_note("PESTS ATE HALF A CROP"); }
        if (fm_g[k] >= fm_need(k)) continue;
        if ((fm_wet >> k) & 1) { fm_dry[k] = 0; if (!gh && !fm_in_season(k) && (day_count & 1)) continue;   // (out of its season: it grows every other day)
                                 fm_g[k]++; if (pb && fm_g[k] < fm_need(k)) fm_g[k]++; if (fm_g[k] >= fm_need(k)) sim_note("CROPS ARE READY TO HARVEST"); }
        else if (++fm_dry[k] >= 3) { fm_n[k] = 0; fm_g[k] = 0; fm_dry[k] = 0; fm_fz &= ~(1 << k); lost = 1; sim_note("A CROP WITHERED. WATER IT!"); } }
    fm_wet = 0; fm_wc = 0; fm_fair = 0;
    { int any = fm_n[0] | fm_n[1] | fm_n[2] | fm_n[3];
      if (any && !(fm_up & 2) && (int)(look_rand() % 100) < (se == 1 ? 24 : 12)) { int k = (int)(look_rand() % 4u); if (fm_n[k] && !((fm_pest >> k) & 1)) { fm_pest |= 1 << k; char b_[40]; snprintf(b_, sizeof b_, "PESTS ARE AT THE %s! SPRAY", fmNm[k]); sim_note(b_); } }
      if (any && se == 2 && day_count % 10 == 9 && !gh) sim_note("FROST TOMORROW! PICK YOUR CROPS"); }   // (the last day of autumn)
    if (lost) sim_moodlet(&sims[player], ML_CROPLOST);
    if (fm_ordq && fm_ordt && --fm_ordt == 0) { fm_ordq = 0; sim_note("THE ORDER EXPIRED"); }
    if (!fm_ordq) { fm_ordk = (u8)(look_rand() % 6u); fm_ordq = (u8)(5 + look_rand() % 8u); fm_ordt = 5; }
    for (int a = 0; a < 2; a++) {   // the animals (0 hens, 1 cows): fed and petted every day they are happy (a happy one lays or gives twice as much); unfed or sad, nothing
        int n = a ? fm_cows : fm_hens, fed = (fm_fed >> a) & 1; u8 *hp = a ? &fm_ch : &fm_hh; u16 *pp = a ? &fm_pm : &fm_pe; if (!n) continue;
        *hp = (u8)CLAMP(*hp + (fed ? 8 : -25) + ((fm_pet >> a) & 1 ? 12 : 0), 0, 100);
        if (!fed || *hp < 30) { char b_[40]; snprintf(b_, sizeof b_, fed ? "THE %s ARE SAD: NO %s. PET THEM" : "THE %s WERE HUNGRY: NO %s", a ? "COWS" : "HENS", fmGood[4 + a]); sim_note(b_); }
        else *pp = (u16)MIN(60, *pp + n * (1 + (SKL(sims[player].skill_farm) >= 4 + 2 * a) + (*hp >= 80)));
    }
    fm_fed = 0; fm_pet = 0;
    for (int i = nsims - 1; i >= 0; i--) if (sims[i].plant == 1) { Sim *s = &sims[i]; s->plant_t = 0; if (++s->plant_d >= 3) pass_on(i); else sim_note("THE STALK FADES. TALK TO IT!"); }   // (a stalk: three days, unless someone helps)
}
u8 col_lvl, col_major, col_on, col_cls, col_gpa, col_study, col_g[3]; u16 astro_xp;   // COLLEGE: degrees (0 none .. 3 doctor), the major, enrolled, lectures, GPA, study points, the GPA of each degree; the stargazing xp
static const char *const majNm[3] = { "SCIENCE", "ARTS", "BUSINESS" }, *const lvlNm[3] = { "BACHELOR", "MASTER", "DOCTOR" };
static const int tuition[3] = { 400, 900, 1600 };
const char *col_major_name(int m) { return majNm[m % 3]; }
int space_cert(void) {   // what the space program lets you do: school marks or a degree open the door; a science degree (and the marks to match) climbs the ladder
    const Sim *p = &sims[player];
    if (col_lvl >= 1 && col_major == 0) { if (col_lvl >= 3 && col_g[2] >= 85) return 4; if (col_lvl >= 2 && col_g[1] >= 80) return 3; if (col_g[0] >= 75) return 2; }
    return (col_lvl >= 1 || p->grade >= 90) ? 1 : 0;
}
const char *space_cert_name(int c) { static const char *const n[5] = { "NOT CERTIFIED", "SPACE TRAINEE", "ENGINEER", "MISSION SPECIALIST", "COMMANDER" }; return n[c % 5]; }
u8 crime_n[CR_N]; int crime_evid, crime_def, arrest_how, sim_want_end, crime_skip;
const char *const crime_nm[CR_N] = { "ASSAULT", "THEFT", "RESISTING ARREST", "JAILBREAK" }; const u8 crime_sev[CR_N] = { 8, 6, 10, 30 };
int crime_total(void) { int t = 0; for (int i = 0; i < CR_N; i++) t += crime_n[i]; return t; }
int crime_points(void) { int t = 0; for (int i = 0; i < CR_N; i++) t += crime_n[i] * crime_sev[i]; return t; }
void sim_crime(int k, int ev) { if (crime_n[k] < 9) crime_n[k]++; crime_evid = MIN(100, crime_evid + ev); }
void sim_arrest(int how) { arrest_how = how; sim_want_prison = 1; }   // (how: 0 caught fighting, 1 gave up, 2 a warrant, 3 a tip-off)
static void crime_midnight(void) {   // the detectives: what you got away with can catch up; and a name on the record draws the wrong attention
    if (crime_skip) return; if (crime_evid > 0) crime_evid -= 2;
#ifdef BORE_DC
    if (crime_total() && (int)(look_rand() % 100) < crime_evid / 2) { sim_note("POLICE AT THE DOOR: A WARRANT!"); { extern void dc_rumble(int, int); dc_rumble(5, 20); } sim_arrest(2); return; }
#else
    if (crime_total() && (int)(look_rand() % 100) < crime_evid / 2) { sim_note("POLICE AT THE DOOR: A WARRANT!"); sim_arrest(2); return; }
#endif
    if (rap_sheet >= 1 && !crime_total() && (int)(look_rand() % 100) < rap_sheet * 2) { sim_note("A TIP-OFF! THE POLICE ARE HERE"); sim_arrest(3); }
}
void sim_skip_days(int n) { crime_skip = 1; for (int i = 0; i < n; i++) { clock_min = 1440; clock_tick(); } clock_min = 8 * 60; crime_skip = 0; }   // (HARD TIME: the days served go by)
static void pet_tick(void) {   // the pet follows you about, gets hungry, and cheers you up while it is fed
    if (!pet_kind || !nsims) return;
    Sim *p = &sims[player]; int dx = p->x - pet_x, dz = p->z - pet_z;
    if (pet_zone != world_zone || dx * dx + dz * dz > 9 * TILE * 9 * TILE) { pet_zone = world_zone; pet_x = p->x - TILE; pet_z = p->z - TILE / 2; dx = p->x - pet_x; dz = p->z - pet_z; }
    int d = ABS(dx) + ABS(dz); world_lvl = p->lvl;
    if (d > TILE * 2 + TILE / 2) {
        int sp = pet_kind == 1 ? 14 - pet_dog.tr[1] / 2 : 9, nx = pet_x + (dx > 0 ? sp : dx < 0 ? -sp : 0), nz = pet_z + (dz > 0 ? sp : dz < 0 ? -sp : 0);
        if (world_walkable(nx / TILE, (-pet_z) / TILE)) pet_x = nx; if (world_walkable(pet_x / TILE, (-nz) / TILE)) pet_z = nz;
        pet_yaw = yaw_towards(dx, dz); pet_ph += 260;
    } else if (!(walk_tick & 63)) pet_yaw = (pet_yaw + (int)(look_rand() & 255) - 128) & 4095;
    if (!(walk_tick % (pet_kind == 1 ? 1500 - pet_dog.tr[3] * 90 : 1200)) && pet_hun > 0) { pet_hun--; if (pet_hun == 20) sim_note("YOUR PET IS HUNGRY"); }
    if (pet_hun > 20 && d < TILE * 4 && !(walk_tick & 127)) { int pl = pet_kind == 1 ? pet_dog.tr[0] : 5, lo = pet_kind == 1 ? pet_dog.tr[2] : 5; p->need[N_FUN] = MIN(p->need[N_FUN] + 300 + pl * 60, 100 << 10); p->need[N_SOCIAL] = MIN(p->need[N_SOCIAL] + 150 + lo * 30, 100 << 10); }   // (a playful dog cheers you up, a loyal one keeps you company)
}
void sim_step(void) {
    walk_tick++; think_tok = 0;
    clock_tick(); pet_tick();
    { static int lastwx; int wx = sim_weather(); if (wx != lastwx) { lastwx = wx; sim_note(wx == 1 ? "IT IS RAINING. TAKE COVER!" : wx == 2 ? "HAIL! GET INSIDE!" : wx == 3 ? "HEAVY SNOW IS FALLING" : "THE WEATHER HAS CLEARED"); } }
    for (int i = 0; i < nsims; i++) {
        Sim *s = &sims[i];
        if (s->away) continue;                                        // (at school: back at 3)
        world_lvl = s->lvl;
        tick_needs(s);
        if (s->plant == 2) {   // PLANTBORE: sun and rain are lunch (and the stomach is slow)
            s->need[N_FOOD] = MIN(100 << 10, s->need[N_FOOD] + RATE_FOOD * 2 / 3);
            if (sim_outdoors(s) && (sim_weather() == 1 || (!is_night() && sim_weather() == 0))) s->need[N_FOOD] = MIN(100 << 10, s->need[N_FOOD] + RATE_FOOD * 3); }
        if (s->plant == 1) {   // a living STALK: rooted, mute, and the loneliness is the end of it
            s->path_n = 0; s->spd = 0; if (s->action != ACT_NONE) end_action(s); s->talk_to = -1;
            s->need[N_SOCIAL] = MAX(0, s->need[N_SOCIAL] - 30); s->need[N_FUN] = MAX(0, s->need[N_FUN] - 20);
            if (!(walk_tick & 15)) s->mood = MAX(-100, s->mood - 1);
            continue; }
        if (!(walk_tick & 3) && sim_festival() && world_zone == ZONE_CITY && sim_outdoors(s)) {   // the festival: music, stalls and a crowd
            s->need[N_FUN] = MIN(s->need[N_FUN] + GAIN_FUN / 2, 100 << 10); s->need[N_SOCIAL] = MIN(s->need[N_SOCIAL] + GAIN_TALK / 3, 100 << 10); }
        if (!(walk_tick & 7) && world_zone != ZONE_ISLAND && sim_outdoors(s)) { int wx = sim_weather(); if (wx) s->need[N_COMFY] = MAX(0, s->need[N_COMFY] - (wx == 2 ? 120 : 60)); }   // (out in the weather: soaked, pelted, frozen)
        if (s->note_t > 0) s->note_t--;
        if (i != player && (s->rage || s->gait.sleep == 1 && s->action == ACT_NONE && s->rage_ko)) continue;   // (after you, or knocked down: citizens.c moves them)
        if (s->gait.wave > 0 && s->action != ACT_TALK && s->action != ACT_PHONE) s->gait.wave--;
        if (s->action == ACT_WALK) { if (!follow_path(s)) arrive(s); }
        else if (s->action != ACT_NONE) run_action(s);
        else { if (s->ai) ai_think(s); }
        if (s->action != ACT_WALK && s->action != ACT_RUN && s->action != ACT_NONE) s->gait.amp = 0;
        if (s->action == ACT_NONE && s->gait.amp > 0 && i != player) s->gait.amp = 0;
    }
    world_lvl = 0;
    if (last_note_t > 0) last_note_t--;
}

int sim_walk(Sim *s, int dx, int dz) { return walk(s, dx, dz); }   // (skate.c: a step with the board's way over ramps and decks)
static void veh_dismount(Sim *s, const char *why) { if (veh_on) traffic_park(veh_on - 1, s->x, s->z, s->yaw); veh_on = 0; s->gait.sit = 0; s->gait.amp = 0; s->gait.run = 0; if (why) sim_note(why); }
static int veh_back;   // backing up (down on the pad): no turning round, half speed
static void veh_drive(Sim *s, int dx, int dz) {   // a step in a vehicle: it turns like a car (slowly) and goes at its speed, burning fuel by the distance
    world_lvl = s->lvl;
    int t = veh_on - 1; s->path_n = 0; s->action = ACT_NONE; s->gait.sit = VT[t].seat > 0; s->gait.amp = 0; s->gait.run = 0;
    if (dx || dz) {
        int want = yaw_towards(dx, dz), diff = ((want - s->yaw + 2048) & 4095) - 2048;
        if (veh_back) diff = 0; else turn_to(s, want, 150 + (ABS(diff) >> 4));
        int mag = SquareRoot0(dx * dx + dz * dz); if (mag > 4096) mag = 4096;
        int align = 4096 - ABS(diff) * 2; if (align < 0) align = 0;
        int mult = VT[t].speed; if (VT[t].tank && veh_fuel[t] <= 0) mult = 1; else if (veh_cond[t] <= 0) mult = 1; else if (veh_cond[t] < 25) mult = MAX(1, mult / 2);
        int sp = (((speed_of(s) * mult * mag) >> 12) * align) >> 12; if (veh_back) sp /= 2;
        int x0 = s->x, z0 = s->z;
        if (sp) walk(s, (isin(s->yaw) * (veh_back ? -sp : sp)) >> 12, -((icos(s->yaw) * (veh_back ? -sp : sp)) >> 12));
        int moved = ABS(s->x - x0) + ABS(s->z - z0);
        veh_wph = (veh_wph + moved * 18) & 4095;
        veh_odo += moved;
        while (veh_odo >= 64) { veh_odo -= 64;
            if (VT[t].tank && veh_fuel[t] > 0 && (veh_cond[t] < 40 || (int)(look_rand() % 100) >= SKL(s->skill_mech) * 3)) veh_fuel[t]--;   // (a tinkerer sips the fuel)
            if (++veh_wacc >= 24 + SKL(s->skill_mech) * 6) { veh_wacc = 0; if (veh_cond[t] > 0) { veh_cond[t]--;
                if (veh_cond[t] == 24) { sim_note("THE ENGINE RATTLES. FIX IT SOON"); audio_voice(VOICE_GROAN); }
                else if (veh_cond[t] == 0) { sim_note("BREAKDOWN! FIX IT AT A PUMP"); audio_voice(VOICE_GROAN); } } } }
        if (VT[t].tank) {
            int pct = veh_fuel[t] * 100 / (VT[t].tank * 4);
            if (veh_fuel[t] <= 0 && veh_lowfuel < 2) { veh_lowfuel = 2; sim_note("OUT OF FUEL! FIND A GAS STATION"); audio_voice(VOICE_GROAN); }
            else if (pct < 12 && !veh_lowfuel) { veh_lowfuel = 1; sim_note("FUEL IS LOW"); }
            else if (pct >= 12 && veh_fuel[t] > 0) veh_lowfuel = 0;
        }
        { int tx = sim_tile_x(s), ty = sim_tile_y(s);                                   // a vehicle stays outdoors: into a doorway and you are off it
#ifdef AUTO_VEH
          if (0)
#else
          if (world_zone != ZONE_CITY || (tx >= 0 && ty >= 0 && tx < MW && ty < MH && floorMap[ty][tx] < FL_GRASS))
#endif
              veh_dismount(s, "PARKED OUTSIDE"); }
    }
    world_lvl = 0;
}
const char *veh_hud(void) {
    static char b[32]; if (!veh_on) return "";
    int t = veh_on - 1;
    if (VT[t].tank) snprintf(b, sizeof b, "%s  FUEL %d%% CAR %d%%", VT[t].nm, veh_fuel[t] * 100 / (VT[t].tank * 4), veh_cond[t]); else snprintf(b, sizeof b, "%s", VT[t].nm);
    return b;
}
static int veh_mount(Sim *s, int t) {   // 1: you are on it
    if (world_zone != ZONE_CITY) { sim_note("NOWHERE TO DRIVE HERE"); return 0; }
#ifndef AUTO_VEH
    { int tx = sim_tile_x(s), ty = sim_tile_y(s); if (s->lvl || tx < 0 || ty < 0 || tx >= MW || ty >= MH || floorMap[ty][tx] < FL_GRASS) { sim_note("OUTSIDE ONLY: NO DRIVING INDOORS"); return 0; } }
#endif
    if (!traffic_park_near(t, s->x, s->z) && traffic_parked(t)) { char b[40]; snprintf(b, sizeof b, "YOUR %s IS PARKED ELSEWHERE", VT[t].nm); sim_note(b); return 0; }
    veh_on = t + 1; veh_sel = t; veh_lowfuel = 0; s->gait.sit = VT[t].seat > 0; s->gait.amp = 0;
    { char b[40]; snprintf(b, sizeof b, "ON THE %s. O TO GET OFF", VT[t].nm); sim_note(b); }
    audio_voice(VOICE_YEHA); return 1;
}
static int ride_from(Sim *s, int st) {   // st: 0 on foot, 1 the board, 2.. a vehicle: the next thing to ride after it (1: on something)
    for (int k = st + 1; k <= 1 + VEH_N; k++) {
        if (k == 1 && has_board) { on_board = 1; skate_on(s); wants_event(s, W_SKATE, 0); sim_note("ON THE BOARD"); return 1; }
        if (k >= 2 && ((veh_own >> (k - 2)) & 1) && veh_mount(s, k - 2)) return 1;
    }
    return 0;
}
int sim_ride_after_board(void) { return ride_from(&sims[player], 1); }
void sim_player_move(int dx, int dz, int run) {
    Sim *s = &sims[player];
    if (s->plant == 1) return;                                           // (a stalk stays put)
    world_lvl = s->lvl;
    if (s->action != ACT_NONE && s->action != ACT_WALK) {
        if (!dx && !dz) return;                                              // standing still: whatever it is doing goes on
        if (s->action == ACT_WORK || s->action == ACT_SLEEP) return;         // (asleep, at work: the stick does not get you up; X or O does)
        end_action(s);                                                       // walking off: it stops
    }
    s->path_n = 0; s->action = ACT_NONE;
#ifdef AUTO_VEH
    { static int n_, ty_; n_++; if (veh_on && n_ > 90) { veh_dismount(s, 0); ty_ = (ty_ + 1) % VEH_N; n_ = 0; }
      if (!veh_on && world_zone == ZONE_CITY && n_ % 10 == 0) { veh_own = 63; for (int t = 0; t < VEH_N; t++) veh_fuel[t] = VT[t].tank * 4; veh_mount(s, ty_); } }   // (test builds: a new vehicle every so often)
#endif
    if (on_board) { skate_step(s, dx, dz); world_lvl = 0; return; }   // riding (skate.c)
    if (veh_on) { veh_drive(s, dx, dz); return; }
    if (!dx && !dz) {
        s->gait.amp = s->gait.amp > 300 ? s->gait.amp - 300 : 0; s->gait.run = 0;
        world_lvl = 0; return; }
    // REAL 3D WALKING: the Bore turns toward the push first and only ever moves the way it FACES -
    // far from the push direction it pivots on the spot, so it never slides sideways like a 2D sprite.
    int want = yaw_towards(dx, dz);
    int diff = ((want - s->yaw + 2048) & 4095) - 2048;                       // how far round it still has to turn
    turn_to(s, want, 260 + (ABS(diff) >> 3));                                // the wider the turn, the faster it swings
    int mag = SquareRoot0(dx * dx + dz * dz); if (mag > 4096) mag = 4096;    // stick tilt = walk speed
    int align = 4096 - ABS(diff) * 2; if (align < 0) align = 0;              // 4096 facing it .. 0 facing away
    int sp0 = speed_of(s); if (run) sp0 = sp0 * 2;
    int sp = ((sp0 * mag) >> 12) * align >> 12;
    if (sp) walk(s, (isin(s->yaw) * sp) >> 12, -((icos(s->yaw) * sp) >> 12));
    s->gait.amp = MAX(1600, (mag * align) >> 12);                            // pivoting: small steps; under way: the full swing
    s->gait.phase = (s->gait.phase + (((run ? 230 : 150) * MAX(mag, 2048)) >> 12)) & 4095;
    s->gait.run = run && mag > 2400 && align > 2048;                         // (a run only when really going: a nudge is a walk)
    world_lvl = 0;
}

void sim_player_tank(int fwd, int turn, int run) {   // first person: left and right turn on the spot, up walks the way it faces, down steps back
    Sim *s = &sims[player];
    world_lvl = s->lvl;
    if (s->action != ACT_NONE && s->action != ACT_WALK) {
        if (!fwd && !turn) { world_lvl = 0; return; }
        if (s->action == ACT_WORK || s->action == ACT_SLEEP) { world_lvl = 0; return; }
        end_action(s);
    }
    s->path_n = 0; s->action = ACT_NONE;
    if (veh_on) { int w = (s->yaw + turn * 7) & 4095; if (fwd < 0) { veh_back = 1; veh_drive(s, (isin(s->yaw) * -fwd) >> 12, -((icos(s->yaw) * -fwd) >> 12)); veh_back = 0; s->yaw = (s->yaw + turn / 2) & 4095; return; } veh_drive(s, fwd > 0 ? (isin(w) * fwd) >> 12 : turn ? (isin(w) * 900) >> 12 : 0, fwd > 0 ? -((icos(w) * fwd) >> 12) : turn ? -((icos(w) * 900) >> 12) : 0); return; }   // (first person: up drives, left and right steer)
    if (on_board) { int w = (s->yaw + turn * 6) & 4095; skate_step(s, fwd > 0 ? (isin(w) * fwd) >> 12 : turn ? (isin(w) * 1200) >> 12 : 0, fwd > 0 ? -((icos(w) * fwd) >> 12) : turn ? -((icos(w) * 1200) >> 12) : 0); world_lvl = 0; return; }   // (first person on a board: up pushes, left and right carve)
    s->yaw = (s->yaw + turn) & 4095;
    if (!fwd) { s->gait.amp = turn ? 1600 : (s->gait.amp > 300 ? s->gait.amp - 300 : 0); if (turn) s->gait.phase = (s->gait.phase + 90) & 4095; world_lvl = 0; return; }
    int sp = speed_of(s); if (run && fwd > 0) sp = sp * 2;
    sp = (sp * fwd) >> 12; if (fwd < 0) sp = sp * 2 / 3;                // (backing up: slower)
    walk(s, (isin(s->yaw) * sp) >> 12, -((icos(s->yaw) * sp) >> 12));
    s->gait.amp = 4096; s->gait.phase = (s->gait.phase + (fwd < 0 ? -130 : run ? 230 : 150)) & 4095; s->gait.run = run && fwd > 0;
    world_lvl = 0;
}
static int front_tile(const Sim *s, int *mx, int *my) {
    int fx = (isin(s->yaw) * (TILE * 3 / 4)) >> 12, fz = -((icos(s->yaw) * (TILE * 3 / 4)) >> 12);
    int px = s->x + fx, pz = s->z + fz;
    *mx = px / TILE; *my = (-pz) / TILE;
    return px >= 0 && pz <= 0;
}
static int usable_at(int tx, int ty, char c) {   // something X does something with
    if (action_for(c) != ACT_NONE) return 1;
    if ((u8)c >= 0xBE && (u8)c <= 0xEA && (u8)c != 0xDF && (u8)c != 0xE1) return 1;      // (the BOREAGOTCHI! pack's things)
    if (c == '8') return world_zone == ZONE_CITY;      // a FOR SALE sign
    if (c == CH_RAFT) return 1;
    if (c == 'B') return !has_board;                   // a skateboard on the floor: pick it up
    if (!c || !strchr("y7d`it", c)) return 0;         // a counter, a shelf, a desk, a mirror, a rack, a table: at a place, to buy at or to work at
    if (job_here(tx, ty, c)) return 1;
    PlaceInfo pi; if (!place_at(tx, ty, &pi)) return 0;
    if (c == '7' || c == '`' || c == 'i') return pi.lot == LOT_SHOP;
    if (c == 't') return pi.lot == LOT_RESTAURANT;
    return c == 'y';                                   // (someone else's desk: nothing for a visitor)
}
static int target_of(const Sim *s, int *mx, int *my, int *who) {
    *who = -1;
    int me = (int)(s - sims), beside = -1, ahead = -1, best = 1 << 30, ux = isin(s->yaw), uz = -icos(s->yaw);
    for (int i = 0; i < nsims; i++) {   // (after whoever or whatever is right in front of you: a person you are facing, then one just beside you)
        if (i == me || sims[i].away || sims[i].action == ACT_SLEEP) continue;
        int dx = sims[i].x - s->x, dz = sims[i].z - s->z;
        if (ABS(dx) >= TILE * 3 / 2 || ABS(dz) >= TILE * 3 / 2) continue;
        int dot = (dx * ux + dz * uz) >> 12;
        if (dot > TILE / 4 && dot * dot * 2 > dx * dx + dz * dz) { if (dot < best) { best = dot; ahead = i; } }   // (within 45 degrees of straight ahead: the nearest)
        else if (beside < 0) beside = i;
    }
    int fx, fy, ft = front_tile(s, &fx, &fy);
    if (ft) for (int i = 0; i < nsims; i++) if (i != me && !sims[i].away && sims[i].action != ACT_SLEEP && sim_tile_x(&sims[i]) == fx && sim_tile_y(&sims[i]) == fy) { *who = i; return 1; }   // someone right in front of you (on a sofa, in a crib)
    if (ft && usable_at(fx, fy, world_at(fx, fy))) { *mx = fx; *my = fy; return 1; }   // then the thing right in front of you
    if (ahead >= 0) { *who = ahead; return 1; }
    if (beside >= 0) { *who = beside; return 1; }
    int cx = sim_tile_x(s), cy = sim_tile_y(s);
    static const signed char dx[5] = { 0, 0, 1, 0, -1 }, dy[5] = { 0, 1, 0, -1, 0 };
    for (int k = 0; k < 5; k++) { int tx = cx + dx[k], ty = cy + dy[k]; if (usable_at(tx, ty, world_at(tx, ty))) { *mx = tx; *my = ty; return 1; } }
    if (s->lvl > 0) for (int k = 0; k < 5; k++) { int tx = cx + dx[k], ty = cy + dy[k];   // the top of a flight of stairs (open air, the stairs below)
        if (world_at_l(s->lvl, tx, ty) == ' ' && world_at_l(s->lvl - 1, tx, ty) == '|') { *mx = tx; *my = ty; return 1; } }
    return 0;
}
static int staff_only(int lot, char c) {   // the kitchens, the office's desks, the clinic's rooms, the projection: not for customers
    if (!c) return 0;
    switch (lot) {
        case LOT_DINER: return c == 'F' || c == '/';
        case LOT_CAFE: case LOT_BAR: return c == 'F' || c == '-';
        case LOT_RESTAURANT: return c == 'F' || c == '/' || c == '-';
        case LOT_CINEMA: return c == 'v' || c == 'A';
        case LOT_CLINIC: return c == 'S' || c == 'd' || c == '[' || c == '-';
        case LOT_OFFICE: case LOT_TOWER: case LOT_SCHOOL: return strchr("dx[bI", c) != 0;
    }
    return 0;
}
static const char *sim_hint_(void) {
    const Sim *s = &sims[player];
    if (s->action != ACT_NONE && s->action != ACT_WALK) return "GET UP";
    int mx, my, who;
    if (!target_of(s, &mx, &my, &who)) return "";
    if (who >= 0) return "TALK";
    char c = world_at(mx, my);
    int mine = job_here(mx, my, c);
    if (mine && is_work()) return "WORK";
    if (c == 'x') return mine ? "NOT WORK TIME" : "NOT YOUR JOB";
    if (c == '8') return "FOR SALE";
    if (c == 'B') return "PICK UP";
    PlaceInfo pi;
    if (place_at(mx, my, &pi)) {
        if (!place_open(pi.lot, clock_min, clock_day)) return "CLOSED";
        if (staff_only(pi.lot, c)) return "SORRY, STAFF ONLY BACK THERE";
        if (c == 'y' || c == 'I') switch (pi.lot) {
            case LOT_DINER: case LOT_CAFE: return "ORDER"; case LOT_SHOP: return "SHOP"; case LOT_INN: case LOT_HOTEL: return "CHECK IN";
            case LOT_BAR: return "A DRINK"; case LOT_CINEMA: return "POPCORN"; case LOT_SALON: return "A HAIRCUT"; case LOT_RESTAURANT: return "DINE";
            case LOT_CLINIC: return "A CHECKUP"; case LOT_GALLERY: return "BUY ART"; case LOT_GAS: case LOT_CONV: return "BUY"; default: return "SORRY, STAFF ONLY BACK THERE"; }
        if (c == ';' && pi.lot == LOT_CINEMA) return "A FILM";
        if (c == 't' && pi.lot == LOT_RESTAURANT) return "DINE";
        if (c == 'S' && pi.lot == LOT_SPA) return "A MASSAGE";
        if (c == '>' && pi.lot == LOT_SPA) return "SOAK";
        if (c == ')' && pi.lot == LOT_GALLERY) return "ADMIRE";
        if (c == '\x82' && pi.lot == LOT_GAS) return "FUEL UP";
        if (c == '!' && pi.lot == LOT_ARCADE) { static char hb_[28]; snprintf(hb_, sizeof hb_, "PLAY %s", arcade_game_name(arcade_game_at(mx, my))); return hb_; }
        if (c == '7' || c == '`' || c == 'i') return pi.lot == LOT_SHOP ? "SHOP" : "";
        if (c == 'd') return "SORRY, STAFF ONLY BACK THERE";
        if (c == '~' && pi.lot == LOT_POOL) return "SWIM";
    }
    return act_hint(action_for(c), c);
}
// ================================================================== THE PIE MENU (Sims 1 & 2)
// X opens it on what is in front: a person gets the social moves, a thing gets its ways of being used.
enum { SO_TALK, SO_JOKE, SO_HUG, SO_COMPLIMENT, SO_FLIRT, SO_TEASE, SO_N };
static const char *const soNm[SO_N] = { "TALK", "JOKE", "HUG", "COMPLIMENT", "FLIRT", "TEASE" };
static int pie_who, pie_mx, pie_my, pie_n; static u8 pie_code[8];   // what is open: a person (pie_who >= 0) or a tile
static int pie_place; static PlaceInfo pie_pi;                         // the place that tile is at (pie_place 0: none)
enum { PC_WORK = 40, PC_MEAL, PC_SNACK, PC_COFFEE, PC_BUY0, PC_BUY4 = PC_BUY0 + 4, PC_TRAINER, PC_SWIM, PC_PLAY, PC_ROOM, PC_BROWSE,   // (the codes past the things' own)
       PC_PARTY, PC_DJ, PC_LATTE, PC_CAKE, PC_DRINK, PC_ROUND, PC_FILM, PC_POPCORN, PC_HAIR, PC_LOOK, PC_DINE, PC_CHECKUP, PC_MASSAGE, PC_SOAK, PC_ADMIRE, PC_PAINTING, PC_BOARD, PC_PICKBOARD, PC_PHONE1, PC_PHONE2, PC_PHONE3, PC_HAND1, PC_HAND2, PC_JOBBOARD, PC_GIG, PC_WEB, PC_COOKNET, PC_CHATNET, PC_CLASS, PC_BAG, PC_BELT, PC_PRIZES, PC_FUEL, PC_CSNACK, PC_CDRINK, PC_GOT_OPEN, PC_GOT_BOX, PC_GOT_DEX, PC_GOT_BUY, PC_GOT_FEED, PC_GOT_LIGHT, PC_GOT_TUCK, PC_PRAC0, PC_PRAC_END = PC_PRAC0 + 4, PC_BUS, PC_PASS1, PC_PASS7, PC_PASS30, PC_DOG, PC_CAT, PC_PETFEED, PC_PETPLAY, PC_STEAL, PC_SPACE, PC_RESORT, PC_CAMP, PC_FARMBOOK, PC_SKYSEA, PC_HEIST, PC_ENR0, PC_ENR_END = PC_ENR0 + 3, PC_LECT, PC_STUDY, PC_REC, PC_SCOPE0, PC_SCOPE_END = PC_SCOPE0 + 4, PC_PAINT, PC_SELLART, PC_WRITE, PC_DIYFIX, PC_MECHFIX, PC_FPLANT0, PC_FPLANT_END = PC_FPLANT0 + 3, PC_FWATER, PC_FHARV, PC_FHEN, PC_FCOW, PC_FFEEDH, PC_FFEEDC, PC_FEGGS, PC_FMILK, PC_FSELL, PC_FEAT, PC_FSPRAY, PC_FFERT, PC_FORDER, PC_FUP, PC_FFAIR, PC_FDRINK, PC_FTALK, PC_FCURE, PC_FROOT, PC_FSPROUT, PC_MAIL, PC_SUN, PC_DIVE, PC_LAPS, PC_GRILL, PC_BIRDS, PC_SMELL, PC_BUYPROP, PC_FPET, PC_FBRUSH, PC_FSHOP, PC_VEH0, PC_VEH_END = PC_VEH0 + VEH_N };
_Static_assert(PC_VEH_END < 256, "pie codes are bytes");
static int room_here(void) { return room_bx == pie_pi.bx && room_by == pie_pi.by && life_minutes() < room_until; }
static char pie_title[24];
const char *sim_pie_title(void) { return pie_title; }
static int sim_pie_open_(char opt[8][24]) {
    Sim *s = &sims[player];
    pie_n = 0;
    if (s->plant == 1) { sim_note("A STALK CAN'T MOVE OR TALK. SWITCH TO ANOTHER BORE"); return 0; }
    if (s->action != ACT_NONE && s->action != ACT_WALK) return 0;        // busy: X just stops (sim_player_interact)
    int mx, my, who;
    if (!target_of(s, &mx, &my, &who)) return 0;
    pie_who = who; pie_mx = mx; pie_my = my;
    if (who >= 0) {
        Sim *o = &sims[who];
        strcpy(pie_title, o->bore.first);
        #define SOPT(txt, v) { snprintf(opt[pie_n], 24, "%s", txt); pie_code[pie_n++] = (u8)(v); }
        if (o->plant == 1) { SOPT("TALK TO THE STALK", PC_FTALK) SOPT("TONIC $500", PC_FCURE) return pie_n; }
        if (o->bore.stage == AG_BABY) {   // a baby: looking after it (the little ones can play and cuddle)
            int big = s->bore.stage >= AG_TEEN, pl = o->plant == 2, td = sim_is_toddler(o);
            if (big) { SOPT(pl ? "WATER IT" : "FEED", 20 + CARE_FEED) if (!td || o->tod_potty < 100) SOPT(pl ? "REPOT IT" : "CHANGE", 20 + CARE_CHANGE) }
            SOPT("PLAY", 20 + CARE_PLAY)
            if (!td) SOPT("CUDDLE", 20 + CARE_CUDDLE)
            if (big && td) {
                if (o->mood < 0) SOPT("COMFORT IT", 20 + CARE_COMFORT)
                SOPT("READ A STORY", 20 + CARE_STORY)
                if (o->tod_walk < 100) SOPT("TEACH TO WALK", 20 + CARE_WALK)
                if (o->tod_talk < 100) SOPT("TEACH TO TALK", 20 + CARE_TALK)
                if (o->tod_potty < 100) SOPT("POTTY TRAIN", 20 + CARE_POTTY)
                if (pie_n < 7 && o->need[N_CLEAN] < (80 << 10)) SOPT("BATH TIME", 20 + CARE_BATH)
            }
            if (big && pie_n < 8) SOPT(td ? "LULLABY AND BED" : "PUT TO BED", 20 + CARE_BED)
            return pie_n;
        }
        int adults = s->bore.stage >= AG_TEEN && o->bore.stage >= AG_TEEN;
        int kidcare = s->bore.stage >= AG_ADULT && (o->bore.stage == AG_CHILD || o->bore.stage == AG_TEEN) && !o->away;
        for (int k = 0; k < SO_N; k++) {
            if (k == SO_HUG && s->rel[who] < 25) continue;                  // you hug friends
            if (k == SO_FLIRT && (!adults || s->rel[who] < 35)) continue;   // flirting needs grown-ups who get on
            if (k == SO_TEASE && s->mood > 40 && !has_trait(s, TR_NICE, 0)) continue;   // a good mood doesn't think of teasing (MEAN always does)
            SOPT(soNm[k], k)
        }
        if (s->bore.stage == AG_ADULT && o->bore.stage == AG_ADULT && s->rel[who] >= 50 && baby_due < 0 && nsims < MAX_SIMS && pie_n < 8)
            SOPT("START A FAMILY", 15)
        if (adults && s->partner != o->uid && pie_n < 8 && (story_is_partner(o->uid) ? story_loved() : s->rel[who] >= 70))
            SOPT("GO STEADY", 16)
        if (kidcare && !o->homework && school_age(o) && pie_n < 8) SOPT("HELP WITH HOMEWORK", 20 + CARE_HWHELP)
        if (kidcare && o->bore.stage == AG_TEEN && o->mood < 20 && pie_n < 8) SOPT("TALK FEELINGS", 20 + CARE_FEELINGS)
        if (kidcare && o->bore.stage == AG_CHILD && pie_n < 8) SOPT("BEDTIME STORY", 20 + CARE_STORY)
        #undef SOPT
        return pie_n;
    }
    char c = world_at(mx, my); int act = action_for(c);
    if (c == ' ') { strcpy(pie_title, "STAIRS"); strcpy(opt[0], "GO DOWN"); pie_code[0] = 1; pie_n = 1; return 1; }
    if (c == '8' && world_zone == ZONE_CITY && world_wbx >= 0) {   // a home for sale: buy it, and the family moves in
        int bx = world_wbx + mx / BM_BLK, by = world_wby + my / BM_BLK;
        if (!home_for_sale(bx, by)) { sim_note("SOLD!"); return 0; }
        home_address(bx, by, pie_title);
        sprintf(opt[0], "MOVE IN $%d", home_price(bx, by)); pie_code[0] = 36; pie_n = 1;   // (the old home is kept as a property)
        strcpy(opt[pie_n], "BUY AS PROPERTY"); pie_code[pie_n++] = PC_BUYPROP;                 // (yours, but you live where you are)
        strcpy(opt[pie_n], "WHAT IS IT?"); pie_code[pie_n++] = 37; return pie_n;
    }
    if (c == 'B') { strcpy(pie_title, "SKATEBOARD"); strcpy(opt[0], "PICK UP"); pie_code[0] = PC_PICKBOARD; pie_n = 1; return 1; }
    pie_place = place_at(mx, my, &pie_pi);
    int mine = job_here(mx, my, c);
    if (mine && !is_work()) { if (c == 'x') { char b[40], h1[8], h2[8]; sprintf(b, "WORK IS %s TO %s", hour_name(careers[job_car].from, h1), hour_name(careers[job_car].to, h2)); sim_note(b); return 0; } mine = 0; }
    if (c == 'x' && !mine) { sim_note("YOU DO NOT WORK HERE"); return 0; }
    if (pie_place && !mine) {
        if (!place_open(pie_pi.lot, clock_min, clock_day)) { char b[40], h[8]; sprintf(b, "CLOSED. OPENS AT %s", hour_name(place_opens_at(pie_pi.lot), h)); sim_note(b); return 0; }
        if (staff_only(pie_pi.lot, c)) { sim_note("SORRY, STAFF ONLY BACK THERE"); return 0; }
    }
    strcpy(pie_title, pie_place ? pie_pi.name : act_hint(act, c));
    #define OPT(txt, v) { snprintf(opt[pie_n], 24, "%s", txt); pie_code[pie_n++] = v; }
    if (mine) OPT("WORK", PC_WORK)
    if (c == 'x') act = ACT_NONE;                                          // (the marker is only for working)
    if (pie_place && (pie_pi.lot == LOT_INN || pie_pi.lot == LOT_HOTEL) && act == ACT_SLEEP && !room_here()) {   // a bed at an inn: yours once you have paid for the room
        if (!mine) { sim_note("RENT A ROOM AT THE FRONT DESK"); return 0; }
        act = ACT_NONE;
    }
    if (pie_place && (c == 'y' || c == 'I' || c == 'x')) act = ACT_NONE;   // (the till, the desk bell: what the place does, not a phone)
    if (pie_place && pie_pi.lot == LOT_ARCADE && c == '!') { char b_[28]; int g_ = arcade_game_at(mx, my); snprintf(b_, sizeof b_, "PLAY %s $%d", arcade_game_name(g_), arcade_price(g_)); OPT(b_, PC_PLAY) act = ACT_NONE; }
    if (pie_place && pie_pi.lot == LOT_ARCADE && c == 'y') { char b_[28]; snprintf(b_, sizeof b_, "PRIZES (%d TICKETS)", arcade_tix); OPT(b_, PC_PRIZES) }
    switch ((u8)c) {   // the BOREAGOTCHI! pack's objects
        case 0xBE: OPT("OPEN THE BASKET", PC_GOT_OPEN) OPT("THE TOY BOX", PC_GOT_BOX) act = ACT_NONE; break;
        case 0xBF: OPT("THE TOY BOX", PC_GOT_BOX) OPT("OPEN THE BASKET", PC_GOT_OPEN) act = ACT_NONE; break;
        case 0xC0: OPT("VIEW THE BLOBDEX", PC_GOT_DEX) OPT("THE TOY BOX", PC_GOT_BOX) act = ACT_NONE; break;
        case 0xCC: OPT("DJ THE DECKS", PC_DJ) OPT("SEND INVITES", PC_PARTY) act = ACT_NONE; break;
        case 0xC3: OPT("TUCK THEM IN", PC_GOT_TUCK) act = ACT_NONE; break;
        case 0xC4: OPT("FEED THEM ALL $5", PC_GOT_FEED) act = ACT_NONE; break;
        case 0xC5: OPT("LIGHTS FOR THEM", PC_GOT_LIGHT) act = ACT_NONE; break;
        case 0xC6: OPT("ADMIRE", PC_ADMIRE) act = ACT_NONE; break;
        case 0xCF: OPT("PRACTICE DRUMS", PC_PRAC0 + BS_DRUM) act = ACT_NONE; break;   // (SUPERSTAR: the practice instruments)
        case 0xD0: OPT("PRACTICE GUITAR", PC_PRAC0 + BS_GTR) act = ACT_NONE; break;
        case 0xD1: OPT("PRACTICE KEYS", PC_PRAC0 + BS_KEYS) act = ACT_NONE; break;
        case 0xD2: OPT("SING AND PRACTICE", PC_PRAC0 + BS_VOICE) act = ACT_NONE; break;
        case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD7: { static const char *const sn[5] = { "STARGAZE: SMALL SCOPE", "STARGAZE: HOBBY SCOPE", "STARGAZE: RESEARCH SCOPE", "STARGAZE: OBSERVATORY", "STARGAZE: RADIO ARRAY" }; OPT(sn[(u8)c - 0xD3], PC_SCOPE0 + ((u8)c - 0xD3)) act = ACT_NONE; break; }
        case 0xD8: OPT("ADMIRE THE ROCKET", PC_ADMIRE) act = ACT_NONE; break;
        case 0xD9: OPT("MISSION CONTROL", PC_SPACE) OPT("MY RECORD", PC_REC) act = ACT_NONE; break;
        case 0xDA: OPT("PAINT A PICTURE", PC_PAINT) act = ACT_NONE; break;
        case 0xDB: { static const char *const cn[4] = { "WHEAT $2", "CORN $4", "TOMATOES $5", "PUMPKIN $8" }; int any = 0, rdy = 0; char b_[24]; Sim *s = &sims[player];
            for (int k = 0; k < 4; k++) { if (fm_n[k]) { any = 1; if (fm_g[k] >= fm_need(k)) rdy = 1; } }
            if (rdy) OPT("HARVEST", PC_FHARV) if (fm_pest) OPT("SPRAY PESTS $3", PC_FSPRAY) if (any) OPT("WATER THE CROPS", PC_FWATER)
            if (s->plant == 2) { OPT("DRINK FROM THE BED", PC_FDRINK) OPT("ROOT IN THE SOIL", PC_FROOT) if (s->bore.stage == AG_ADULT && baby_due < 0 && nsims < MAX_SIMS && fm_fert >= 2) OPT("SPROUT A SEEDLING", PC_FSPROUT) }
            for (int k = 0; k < 4 && pie_n < 8; k++) { snprintf(b_, sizeof b_, "PLANT %s", cn[k]); OPT(b_, PC_FPLANT0 + k) }
            act = ACT_NONE; break; }
        case 0xDC: { char b_[24]; if (fm_pe) { snprintf(b_, sizeof b_, "COLLECT %d EGGS", fm_pe); OPT(b_, PC_FEGGS) } if (fm_hens && !(fm_fed & 1)) OPT("FEED THE HENS $2", PC_FFEEDH) if (fm_hens && !(fm_pet & 1)) OPT("PET THE HENS", PC_FPET) if (fm_hens < 6) OPT("BUY A HEN $40", PC_FHEN) act = ACT_NONE; break; }
        case 0xDD: { char b_[24]; if (fm_pm) { snprintf(b_, sizeof b_, "MILK THE COWS (%d)", fm_pm); OPT(b_, PC_FMILK) } if (fm_cows && !(fm_fed & 2)) OPT("FEED THE COWS $5", PC_FFEEDC) if (fm_cows && !(fm_pet & 2)) OPT("BRUSH THE COWS", PC_FBRUSH) if (fm_cows < 3) OPT("BUY A COW $250", PC_FCOW) act = ACT_NONE; break; }
        case 0xE0: OPT("CHECK THE MAIL", PC_MAIL) act = ACT_NONE; break;
        case 0xE2: OPT("SMELL THE FLOWERS", PC_SMELL) act = ACT_NONE; break;
        case 0xE3: OPT("ADMIRE THE GNOME", PC_ADMIRE) act = ACT_NONE; break;
        case 0xE4: OPT("WATCH THE BIRDS", PC_BIRDS) act = ACT_NONE; break;
        case 0xE5: OPT("DIVE IN", PC_DIVE) OPT("SWIM", PC_SWIM) act = ACT_NONE; break;
        case 0xE6: OPT("SWIM", PC_SWIM) OPT("SWIM LAPS", PC_LAPS) act = ACT_NONE; break;
        case 0xE7: OPT("WATCH THE POOL", PC_BIRDS) act = ACT_NONE; break;
        case 0xE8: case 0xE9: OPT("SUNBATHE", PC_SUN) act = ACT_NONE; break;
        case 0xEA: OPT("GRILL OUT $8", PC_GRILL) act = ACT_NONE; break;
        case 0xDE: { static const char *const upn[4] = { "SPRINKLER $400", "SCARECROW $120", "GREENHOUSE $900", "TRACTOR $1500" };
            char b_[28]; int tot = fm_total();
            if (tot) { snprintf(b_, sizeof b_, "SELL PRODUCE (%d)", tot); OPT(b_, PC_FSELL) }
            if (fm_ordq) { snprintf(b_, sizeof b_, "ORDER: %d %s", fm_ordq, fmGood[fm_ordk]); OPT(b_, fm_have(fm_ordk) >= fm_ordq ? PC_FORDER : 0) }
            if (sim_festival() && !fm_fair && fm_stock[3]) OPT("ENTER THE FAIR", PC_FFAIR)
            OPT("FARM BOOK", PC_FARMBOOK)
            OPT("FERTILIZER $6", PC_FFERT)
            for (int k = 0; k < 4; k++) if (!((fm_up >> k) & 1)) { OPT(upn[k], PC_FUP + 0) pie_code[pie_n - 1] = (u8)(PC_FUP); break; }
            if (tot - fm_stock[0] >= 2) OPT("EAT A FRESH MEAL", PC_FEAT)
            act = ACT_NONE; break; }
    }
    if (c == CH_CAKE) {   // the birthday cake: the candles (if there is a birthday), then slices
        if (bday_one() >= 0) OPT("BLOW CANDLES", 30)
        OPT("A SLICE", 31) act = ACT_NONE; }
    if (c == CH_GRAVE) { char nm[NAME_LEN]; sim_grave_name(mx, my, nm); strcpy(pie_title, nm); OPT("REMEMBER", 32) act = ACT_NONE; }
    if (act == ACT_GATHER) {   // the island: what you can get from it
        int k = gather_kind(c);
        strcpy(pie_title, items_name(c));
        OPT(gatherHint[k], 100 + k)
        if (c == 'g' || c == '3' || c == '6') { if (k != G_WOOD) OPT("CHOP WOOD", 100 + G_WOOD) OPT("GATHER LEAVES", 100 + G_LEAVES) }
        if (c == CH_BUSH) OPT("GATHER LEAVES", 100 + G_LEAVES)
        act = ACT_NONE;
    }
    if (c == CH_RAFT) { strcpy(pie_title, "THE RAFT"); OPT("SAIL ACROSS", 39) OPT("NOT NOW", 37) act = ACT_NONE; }
    if (c == 's' && world_zone == ZONE_ISLAND) {   // the camp's fire: a meal from what you gathered, or make something
        if (isl_food > 0) OPT("COOK AND EAT", 1)
        OPT("CRAFT", 38) OPT("CASTAWAY CLUB", PC_CAMP) act = ACT_NONE;
    }
    if (act == ACT_READ && school_age(s) && !s->homework) OPT("HOMEWORK", 33)
    if (act == ACT_EAT && c != CH_CAKE && !pie_place && s->bore.stage >= AG_TEEN && bday_one() >= 0 && !cake_slices) OPT("BIRTHDAY CAKE", 34)
    if (pie_place && ((pie_pi.lot == LOT_SPA && (c == 'S' || c == '>')) || (pie_pi.lot == LOT_GALLERY && c == ')'))) act = ACT_NONE;
    switch (act) {
        case ACT_NONE: break;
        case ACT_EAT: OPT("SNACK", 0) OPT("COOK A MEAL", 1) break;
        case ACT_SLEEP: OPT("SLEEP", 0) OPT("NAP", 1) break;
        case ACT_SHOWER: OPT("SHOWER", 0) OPT("SING IN IT", 1) break;
        case ACT_RUN: OPT("JOG", 0) OPT("SPRINT", 1) break;
        case ACT_COFFEE: OPT("A CUP", 0) OPT("ESPRESSO", 1) break;
        case ACT_READ: OPT("READ", 0) OPT("STUDY", 1) break;
        case ACT_PHONE: OPT("CALL A FRIEND", 0) OPT("ORDER PIZZA", 1) OPT("THROW A PARTY", 2) OPT("BOOK A RESORT", PC_RESORT) OPT("FARM LEDGER", PC_FARMBOOK) OPT("HARBOUR AND SKY", PC_SKYSEA) if (s->bore.stage >= AG_TEEN) OPT("UNDERWORLD", PC_HEIST) if (s->bore.stage == AG_ADULT || s->bore.stage == AG_ELDER) OPT("ADOPT $1000", 35) break;
        case ACT_TRAVEL: if (c == '+' && world_zone == ZONE_CITY) { OPT("BUS TO A TOWN", PC_BUS) OPT("TO THE COUNTRY, ISLAND", 0) OPT("DAY PASS $10", PC_PASS1) OPT("WEEK PASS $30", PC_PASS7) OPT("MONTH PASS $90", PC_PASS30) } else OPT(act_hint(act, c), 0) break;
        case ACT_STAIRS: OPT("GO UP", 0) break;
        case ACT_FUN: if (c == '[') {   // the COMPUTER (borrowed from BORE on the GBA): the job board, a paid gig, the web
                          OPT("PLAY A GAME", 0) OPT("JOB BOARD", PC_JOBBOARD)  OPT("SURF THE WEB", PC_WEB) OPT("WRITE A NOVEL", PC_WRITE)OPT("COOKING CLASS ONLINE", PC_COOKNET) OPT("CHAT ONLINE", PC_CHATNET)
                          if (s->bore.stage >= AG_TEEN) OPT("FREELANCE GIG", PC_GIG) break; }
                      if (c == 'R' || c == 'A' || c == 'j') {   // a radio: the stations
                          for (int k = 0; k < audio_station_count(); k++) OPT(audio_station_name(k), 10 + k)
                          OPT("TURN IT OFF", 9) break; }
                      OPT(act_hint(act, c), 0) break;
        default: OPT(act_hint(act, c), 0) break;
    }
    if (pet_kind && pie_n < 8 && act == ACT_EAT) OPT("FEED THE PET $3", PC_PETFEED)
    if (pet_kind && pie_n < 8 && act == ACT_FUN) OPT("PLAY WITH THE PET", PC_PETPLAY)
    if (pie_place && !mine && pie_pi.lot == LOT_SHOP && s->bore.stage >= AG_TEEN && pie_n < 8) OPT("SHOPLIFT", PC_STEAL)
    if (pie_place && !mine) switch (pie_pi.lot) {   // what the place sells
        case LOT_DINER: if (c == 'y') { OPT("A MEAL $15", PC_MEAL) OPT("A SNACK $6", PC_SNACK) OPT("A COFFEE $3", PC_COFFEE) } else if (c == 'c') OPT("A COFFEE $3", PC_COFFEE) break;
        case LOT_SHOP: { static const char *const buy[5] = { "GROCERIES $60", "AN OUTFIT $150", "A GAME $60", 0, "A RECORD $25" };
                         int k = place_shop_kind(bm_hash(pie_pi.bx, pie_pi.by, 3)); if (buy[k] && pie_n < 7) OPT(buy[k], PC_BUY0 + k)
                         if (k == 0 && fm_total() && pie_n < 7) OPT("SELL FARM FOOD", PC_FSHOP)   // (FARM LIFE: a grocery buys what the farm makes)
                         if (k == 3) {   // an electronics shop: phones and handhelds
                             if (phone_tier < 1) OPT("FLIP PHONE $60", PC_PHONE1)
                             if (phone_tier < 2) OPT("SMARTPHONE $350", PC_PHONE2)
                             if (phone_tier < 3 && pie_n < 7) OPT("BORE PHONE MAX $900", PC_PHONE3)
                             if (!(handheld & 1) && pie_n < 7) OPT("BOREBOY $120", PC_HAND1)
                             if (!(handheld & 2) && pie_n < 7) OPT("BOREBOY ADVANCE $220", PC_HAND2) }
                         if ((k == 3 || k == 2) && !gotchi_toy(player) && s->bore.stage >= AG_CHILD && pie_n < 7) OPT("BOREAGOTCHI $5000", PC_GOT_BUY)
                         if (k == 1 && !has_board && pie_n < 7) OPT("SKATEBOARD $120", PC_BOARD)
                         if (!pet_kind && s->bore.stage >= AG_CHILD && pie_n < 7) { OPT("A PUPPY $150", PC_DOG) if (pie_n < 7) OPT("A KITTEN $120", PC_CAT) }
                         if (pie_n < 8) OPT("BROWSE", PC_BROWSE) } break;
        case LOT_GYM: if ((c == ']' || c == 'm' || c == '_') && pie_n < 8) OPT("A TRAINER $20", PC_TRAINER)
                      if ((c == ']' || c == 'm' || c == '_') && s->bore.stage >= AG_CHILD) {   // THE DOJO CLASS (borrowed from BORE on the GBA)
                          if (dojo_session() >= 0 && pie_n < 8) OPT("ATTEND CLASS $10", PC_CLASS)
                          if (pie_n < 8) OPT("DRILL THE BAG", PC_BAG)
                          if (pie_n < 8) OPT("MY BELT", PC_BELT) } break;
        case LOT_SCHOOL: if (c == 'd' && (s->bore.stage == AG_ADULT || s->bore.stage == AG_ELDER) && pie_n < 7) {   // COLLEGE: for grown-ups, at a desk in any school
                             if (col_on) { OPT("ATTEND LECTURE", PC_LECT) OPT("STUDY FOR EXAMS", PC_STUDY) }
                             else if (col_lvl == 0) { OPT("ENROLL: SCIENCE $400", PC_ENR0) OPT("ENROLL: ARTS $400", PC_ENR0 + 1) OPT("ENROLL: BUSINESS $400", PC_ENR0 + 2) }
                             else if (col_lvl < 3) OPT(col_lvl == 1 ? "MASTER'S COURSE $900" : "DOCTORATE $1600", PC_ENR0 + 3)
                             OPT("MY RECORD", PC_REC) } break;
        case LOT_POOL: if ((c == '~' || c == '>') && pie_n < 8) OPT("SWIM", PC_SWIM) break;
        case LOT_INN: case LOT_HOTEL: if (c == 'y' || c == 'I') OPT(room_here() ? "MY ROOM: PAID" : "A ROOM $60", PC_ROOM) break;
        case LOT_CAFE: if (c == 'y' || c == 'c') { OPT("A LATTE $5", PC_LATTE) if (c == 'y') OPT("A CAKE $6", PC_CAKE) } break;
        case LOT_BAR: if (c == 'y') { OPT("A DRINK $8", PC_DRINK) OPT("A ROUND $40", PC_ROUND) } break;
        case LOT_CINEMA: if (c == ';' && pie_n < 8) OPT("A FILM $12", PC_FILM) else if (c == 'y' || c == ':') OPT("POPCORN $4", PC_POPCORN) break;
        case LOT_SALON: if ((c == 'y' || c == '`' || c == 'h') && pie_n < 7) { OPT("A HAIRCUT $40", PC_HAIR) OPT("A NEW LOOK $120", PC_LOOK) } break;
        case LOT_RESTAURANT: if ((c == 'y' || c == 't' || c == 'h') && pie_n < 8) OPT("FINE DINING $40", PC_DINE) break;
        case LOT_CLINIC: if (c == 'y') OPT("A CHECKUP $30", PC_CHECKUP) break;
        case LOT_SPA: if (c == 'S') OPT("A MASSAGE $50", PC_MASSAGE) else if (c == '>') OPT("SOAK $15", PC_SOAK) break;
        case LOT_GALLERY: if (c == ')') OPT("ADMIRE THE ART", PC_ADMIRE) else if (c == 'y') { OPT("A PAINTING $300", PC_PAINTING) if (art_n > 0) OPT("SELL YOUR ART", PC_SELLART) } break;
        case LOT_GAS: case LOT_CONV:
            if (c == '\x82') {                                   // a pump: fill the tank of what you drive (or of the last thing you did)
                int t = veh_on ? veh_on - 1 : veh_sel; char b_[16];
                if (((veh_own >> t) & 1) && VT[t].tank) { int q = VT[t].tank * 4 - veh_fuel[t]; if (q <= 0) OPT("TANK IS FULL", 0) else { snprintf(b_, sizeof b_, "FILL UP $%d", MAX(2, (q * 2 + 399) / 400)); OPT(b_, PC_FUEL) }
                    if (veh_cond[t] < 100) { OPT("FIX IT YOURSELF", PC_DIYFIX) snprintf(b_, sizeof b_, "MECHANIC $%d", (100 - veh_cond[t]) * VT[t].price / 250 + 5); OPT(b_, PC_MECHFIX) } }
                else OPT("WHAT IS IT?", 37)
            } else if (c == 'y' || c == 'I') {
                OPT("A SNACK $4", PC_CSNACK) OPT("A DRINK $3", PC_CDRINK)
                if (pie_pi.lot == LOT_GAS) for (int t = 0; t < VEH_N && pie_n < 7; t++) if (!((veh_own >> t) & 1)) { char b_[16]; snprintf(b_, sizeof b_, "%s $%d", t == VEH_SPORTS ? "SPORTS" : VT[t].nm, VT[t].price); OPT(b_, PC_VEH0 + t) }
            } else if (c == 'c') OPT("A COFFEE $3", PC_COFFEE)
            break;
    }
    #undef OPT
    if (!pie_n) { sim_note(pie_place ? "SORRY, STAFF ONLY BACK THERE" : "NOTHING TO DO HERE"); return 0; }
    return pie_n;
}
static void social(Sim *s, Sim *o, int move) {   // a social move: the other Bore accepts or rejects it (Sims 2: mood, the relationship, personality)
    int me = (int)(s - sims), who = (int)(o - sims);
    int chance = 55 + s->rel[who] / 2 + o->mood / 4;
    for (int t = 0; t < TR_N; t++) if (ABS((int)s->bore.tr[t] - (int)o->bore.tr[t]) <= 2) chance += 4;   // compatible
    if (move == SO_JOKE) chance += has_trait(o, TR_PLAY, 1) ? 30 : has_trait(o, TR_PLAY, 0) ? -25 : 0;
    if (move == SO_HUG) chance += has_trait(o, TR_NICE, 1) ? 15 : -5;
    if (move == SO_FLIRT) chance -= 25;
    if (move == SO_TEASE) chance = 100;                                   // it always lands. That's the problem.
    if (o->grump && move == SO_JOKE) chance = 100;                        // MOSS loves a joke (that isn't about floating things)
    int ok = (int)(look_rand() % 100) < chance;
    static const s8 gain_[SO_N] = { 6, 9, 11, 8, 13, -12 };
    if (!ok) {
        sim_say(o, move == SO_FLIRT ? "UH... NO." : move == SO_HUG ? "PERSONAL SPACE!" : "NOT FUNNY.");
        s->rel[who] -= 5; if (o->rel[me] > -100) o->rel[me] -= 3;
        wants_fear(s, F_REJECT); sim_moodlet(s, ML_REJECTED); if (move == SO_FLIRT) sim_moodlet(s, ML_EMBARRASSED);
        audio_voice(VOICE_DISAGREE); return;
    }
    int g = gain_[move]; if (has_trait(s, TR_NICE, 1) && g > 0) g += 3;
    int r = s->rel[who] + g; s->rel[who] = (s8)CLAMP(r, -100, 100);
    r = o->rel[me] + g; o->rel[me] = (s8)CLAMP(r, -100, 100);
    if (me == player) story_event(move == SO_JOKE ? E_JOKE : move == SO_HUG ? E_HUG : move == SO_FLIRT ? E_FLIRT : E_TALK, o->uid);
    switch (move) {
        case SO_TALK: sim_say(o, "OH TOTALLY"); audio_voice(VOICE_AGREE); break;
        case SO_JOKE: sim_say(o, "HAHAHA!"); audio_voice(VOICE_LAUGH); sim_moodlet(o, ML_GOT_A_LAUGH); sim_moodlet(s, ML_GOT_A_LAUGH); break;
        case SO_HUG: sim_say(o, "AWW"); audio_voice(VOICE_AGREE); sim_moodlet(o, ML_HUGGED); sim_moodlet(s, ML_HUGGED); break;
        case SO_COMPLIMENT: sim_say(o, "YOU THINK SO?"); audio_voice(VOICE_YAHOO); o->mood += 10; break;
        case SO_FLIRT: sim_say(o, "OH, STOP IT"); audio_voice(VOICE_FLIRT); sim_moodlet(o, ML_FLIRTY); sim_moodlet(s, ML_FLIRTY); break;
        case SO_TEASE: sim_say(o, "HEY!"); audio_voice(VOICE_ANGRY); sim_moodlet(o, ML_TEASED); if (has_trait(s, TR_NICE, 0)) sim_moodlet(s, ML_GOT_A_LAUGH); break;
    }
    if (o->grump && !(look_rand() & 1)) sim_say(o, grump_line());
    if (o->action != ACT_NONE && o->action != ACT_WALK) end_action(o);
    begin_action(s, ACT_TALK, sim_tile_x(o), sim_tile_y(o));
    begin_action(o, ACT_TALK, sim_tile_x(s), sim_tile_y(s));
    s->talk_to = who; o->talk_to = me; o->path_n = 0;
}
static int pay(int n) {
    { int d = SKL(sims[player].skill_char) * 2; if (d && n > 5) n = n * (100 - d) / 100; }   // (CHARISMA: a charmer haggles: up to 20% off)
    if (cash < n) { char b[32]; sprintf(b, "THAT IS $%d", n); sim_note(b); audio_voice(VOICE_NAH); return 0; }
    cash -= n; return 1;
}
static void shopped(Sim *s, const char *what, int ml) { sim_note(what); sim_moodlet(s, ml); wants_event(s, W_SHOP, 0); wants_event(s, W_BUY, 0); audio_voice(VOICE_YAHOO); }
static void new_look(Bore *b, int tab) {   // a tab of the creator rolled again: the clothes (a shop's rails), the hair (a salon's chair)
    const LookRow *rows; int n = look_tab_rows(tab, &rows);
    for (int i = 0; i < n; i++) {
        int id = rows[i].id; if (id >= LK_TRAIT0 || rows[i].kind == ROW_AGE) continue;
        int cnt = look_count(id); if (cnt < 2) continue;
        for (int t = 0; t < 8; t++) { int v = (int)(look_rand() % cnt); if (look_allowed(b, id, v)) { b->look[id] = (u8)v; break; } }
    }
    look_build(b);
}
static void swim(Sim *s) {   // into the nearest water
    int bx = -1, by = -1, bd = 1 << 30;
    for (int y = pie_my - 4; y <= pie_my + 4; y++) for (int x = pie_mx - 4; x <= pie_mx + 4; x++) {
        if (x < 0 || y < 0 || x >= MW || y >= MH || floorMap[y][x] != FL_WATER) continue;
        int d = (x - pie_mx) * (x - pie_mx) + (y - pie_my) * (y - pie_my); if (d < bd) { bd = d; bx = x; by = y; }
    }
    if (bx < 0) { sim_note("THERE IS NO WATER HERE"); return; }
    begin_action(s, ACT_SWIM, bx, by);
    s->x = bx * TILE + TILE / 2; s->z = -(by * TILE + TILE / 2); s->gait.sleep = 1;
    sim_say(s, "CANNONBALL!"); audio_voice(VOICE_YAHOO);
}
// ---- THE DOJO CLASS (borrowed from BORE on the GBA, dojo.h): at the gym, classes at 9 AM, 1 PM and 6 PM (two hours each), each
// weekday its own lesson; a class trains the body, more for a regular (days in a row). The belt is the body skill: WHITE .. BLACK,
// and a higher belt hits harder in a fight (citizens.c) ----
static const u8 dj_start[3] = { 9, 13, 18 };
int dojo_session(void) { for (int i = 0; i < 3; i++) if (clock_min >= dj_start[i] * 60 && clock_min < (dj_start[i] + 2) * 60) return i; return -1; }
const char *const band_skill_name[BS_N] = { "VOICE", "GUITAR AND BASS", "KEYBOARDS", "DRUMS", "DANCING" };
int dojo_belt(const Sim *s) { int l = wants_skill_level(s->skill_body) / 2; return l > 5 ? 5 : l; }
static const char *const dj_belt[6] = { "WHITE", "YELLOW", "GREEN", "BLUE", "BROWN", "BLACK" };
static const char *const dj_day[7] = { "STANCE AND GUARD", "THE COMBO", "KICKS", "THE FIREBALL", "THE UPPERCUT", "THE SPIN KICK", "MEDITATION" };
static int dj_last = -1, dj_streak, dj_key = -1;
static void place_do(Sim *s, int code) {
    switch (code) {
        case PC_WORK: if (job_none) { sim_note("YOU DO NOT WORK HERE (YET): PHONE, JOBS"); return; } begin_action(s, ACT_WORK, pie_mx, pie_my); { char b[40]; snprintf(b, sizeof b, "AT WORK: %s", job_title()); sim_note(b); } break;
        case PC_MEAL: if (!pay(15)) return; begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 2; s->act_t = 260; sim_say(s, "THE SPECIAL, PLEASE"); break;
        case PC_SNACK: if (!pay(6)) return; begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 0; break;
        case PC_COFFEE: if (!pay(3)) return; begin_action(s, ACT_COFFEE, pie_mx, pie_my); s->act_var = 1; break;
        case PC_BUY0: if (!pay(60)) return; groceries += 7; shopped(s, "GROCERIES: 7 GOOD MEALS AT HOME", ML_RETAIL); break;
        case PC_BUY0 + 1: if (!pay(150)) return; new_look(&s->bore, 3); crowd_forget(CROWD_MAX + player); shopped(s, "A NEW OUTFIT!", ML_OUTFIT); break;
        case PC_BUY0 + 2: if (!pay(60)) return; s->need[N_FUN] = MIN(s->need[N_FUN] + (30 << 10), 100 << 10); shopped(s, "A NEW GAME!", ML_NEW_GAME); break;
        case PC_BUY4: if (!pay(25)) return; s->need[N_FUN] = MIN(s->need[N_FUN] + (20 << 10), 100 << 10); shopped(s, "A NEW RECORD!", ML_RETAIL); break;
        case PC_BOARD: if (!pay(120)) return; has_board = 1; shopped(s, "A SKATEBOARD! O TO RIDE", ML_RETAIL); break;
        case PC_PHONE1: if (!pay(60)) return; phone_tier = 1; shopped(s, "A FLIP PHONE: CALLS, TEXTS, TAXIS", ML_RETAIL); break;
        case PC_PHONE2: if (!pay(350)) return; phone_tier = 2; shopped(s, "A SMARTPHONE! MAPS, JOBS, PHOTOS AND MORE", ML_RETAIL); break;
        case PC_PHONE3: if (!pay(900)) return; phone_tier = 3; shopped(s, "THE BORE PHONE MAX! EVERY APP THERE IS", ML_RETAIL); break;
        case PC_GOT_BUY: if (cash < GOTCHI_PRICE) { sim_note("A BOREAGOTCHI COSTS $5000"); audio_voice(VOICE_NAH); return; } if (!pay(GOTCHI_PRICE)) return;
                         gotchi_buy(player); shopped(s, "A BOREAGOTCHI! IT CAME WITH AN EGG", ML_NEW_GAME); sim_want_gotchi = 2; break;
        case PC_GOT_OPEN: if (!gotchi_toy(player)) { sim_note("YOU NEED A BOREAGOTCHI: ELECTRONICS SHOPS, $5000"); return; } sim_want_gotchi = 2; break;
        case PC_GOT_BOX: sim_want_gotchi = 1; break;
        case PC_DJ: sim_want_dj = 1; break;
        case PC_PARTY: { extern void throw_party(void); throw_party(); } break;
        case PC_GOT_DEX: sim_want_gotchi = 3; break;
        case PC_GOT_FEED: if (!gotchi_toy(player)) { sim_note("YOU NEED A BOREAGOTCHI FIRST"); return; } if (!pay(5)) return; sim_note(gotchi_house(player, 0)); break;
        case PC_GOT_LIGHT: sim_note(gotchi_toy(player) ? gotchi_house(player, 1) : "YOU NEED A BOREAGOTCHI FIRST"); break;
        case PC_GOT_TUCK: sim_note(gotchi_toy(player) ? gotchi_house(player, 2) : "YOU NEED A BOREAGOTCHI FIRST"); break;
        case PC_HAND1: if (!pay(120)) return; handheld |= 1; shopped(s, "A BOREBOY! PLAY IT FROM THE PHONE'S GAMES", ML_NEW_GAME); break;
        case PC_HAND2: if (!pay(220)) return; handheld |= 2; shopped(s, "A BOREBOY ADVANCE! THE GAMES LOOK GREAT", ML_NEW_GAME); break;
        case PC_PICKBOARD: has_board = 1; *world_cell(s->lvl, pie_mx, pie_my) = '.'; world_touched(); sim_note("A SKATEBOARD! O TO RIDE"); audio_voice(VOICE_YAHOO); break;
        case PC_BROWSE: begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_var = 4; s->act_t = 240; break;
        case PC_JOBBOARD: sim_want_jobs = 2; break;
        case PC_CLASS: {
            int ses = dojo_session(), key = day_count * 3 + ses; char b[48];
            if (ses < 0) { sim_note("CLASSES: 9 AM, 1 PM AND 6 PM"); return; }
            if (key == dj_key) { sim_note("YOU ARE IN THIS CLASS ALREADY"); return; }
            if ((s->need[N_REST] >> 10) < 20) { sim_note("TOO TIRED FOR CLASS"); audio_voice(VOICE_NAH); return; }
            if (!pay(10)) return;
            dj_streak = dj_last == day_count - 1 ? MIN(dj_streak + 1, 7) : dj_last == day_count ? dj_streak : 1; dj_last = day_count; dj_key = key;
            int was = dojo_belt(s);
            begin_action(s, ACT_RUN, pie_mx, pie_my); s->act_var = 2; s->act_t = 600;
            s->skill_body += 80 + 20 * dj_streak; s->need[N_REST] = MAX(0, s->need[N_REST] - (12 << 10));
            snprintf(b, sizeof b, "TODAY'S LESSON: %s", dj_day[clock_day % 7]); sim_note(b);
            if (dj_streak > 1) { snprintf(b, sizeof b, "A REGULAR: %d DAYS IN A ROW", dj_streak); sim_note(b); }
            if (dojo_belt(s) > was) { snprintf(b, sizeof b, "A NEW BELT: %s!", dj_belt[dojo_belt(s)]); sim_note(b); audio_voice(VOICE_WIN); }
            sim_say(s, "HAI!"); break; }
        case PC_BAG: if ((s->need[N_REST] >> 10) < 10) { sim_note("TOO TIRED"); return; }
                     begin_action(s, ACT_RUN, pie_mx, pie_my); s->act_var = 1; s->act_t = 240; s->skill_body += 15; sim_say(s, "HUP! HUP!"); break;
        case PC_BELT: { char b[48]; int bl = dojo_belt(s), raw = s->skill_body, next = (bl + 1) * 800;
                        if (bl >= 5) snprintf(b, sizeof b, "%s BELT. THE TOP!", dj_belt[bl]); else snprintf(b, sizeof b, "%s BELT: %d%% TO %s", dj_belt[bl], (raw - bl * 800) * 100 / 800, dj_belt[bl + 1]);
                        sim_note(b); { int ses = dojo_session(); const char *nx = ses >= 0 ? "A CLASS IS ON NOW" : clock_min < 9 * 60 ? "NEXT CLASS 9 AM" : clock_min < 13 * 60 ? "NEXT CLASS 1 PM" : clock_min < 18 * 60 ? "NEXT CLASS 6 PM" : "NEXT CLASS 9 AM TOMORROW"; sim_note(nx); } (void)next; break; }   // (main opens JOBS: every opening, not just the paper's three)
        case PC_WEB: case PC_COOKNET: case PC_CHATNET: case PC_GIG: {   // a few of each a day: not a free skill farm
            static u16 day_; static u8 used_[4]; int k = code - PC_GIG; if (day_ != (u16)(day_count + 1)) { day_ = (u16)(day_count + 1); memset(used_, 0, 4); }
            if (used_[k] >= (k == 0 ? 2 : 3)) { sim_note("ENOUGH OF THAT FOR TODAY"); audio_voice(VOICE_NAH); return; }
            if (k == 0 && (s->need[N_REST] >> 10) < 25) { sim_note("TOO TIRED FOR A GIG"); audio_voice(VOICE_NAH); return; }
            used_[k]++; if (k == 1) s->skill_logic += 12; if (k == 3) s->skill_char += 12; begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = k == 0 ? 420 : 240;
            if (k == 0) { int pay = 35 + (s->skill_cook + s->skill_fun + s->skill_creat + s->skill_logic) / 60; cash += pay; s->need[N_REST] = MAX(0, s->need[N_REST] - (15 << 10)); char b[32]; sprintf(b, "FREELANCE GIG: +$%d", pay); sim_note(b); audio_voice(VOICE_FINISHED); }
            else if (k == 1) sim_note((day_count + clock_min) & 3 ? "SURFED THE WEB" : "YOU FELL DOWN A WIKI HOLE");
            else if (k == 2) { s->skill_cook += 40; sim_note("A COOKING CLASS ONLINE: COOKING UP"); }
            else { s->need[N_SOCIAL] = MIN(100 << 10, s->need[N_SOCIAL] + (12 << 10)); sim_note("CHATTED ONLINE: FEELING LESS ALONE"); }
            break; }
        case PC_TRAINER: if (!pay(20)) return; begin_action(s, ACT_RUN, pie_mx, pie_my); s->act_var = 2; sim_say(s, "FEEL THE BURN"); break;
        case PC_SWIM: swim(s); break;
        case PC_PLAY: { int g_ = arcade_game_at(pie_mx, pie_my); if (!pay(arcade_price(g_))) return; sim_want_arcade = g_ + 1; break; }
        case PC_PRIZES: sim_want_arcade = AG_PRIZES + 1; break;
        case PC_FUEL: { int t = veh_on ? veh_on - 1 : veh_sel; int q = VT[t].tank * 4 - veh_fuel[t], cost = MAX(2, (q * 2 + 399) / 400);
                        if (q <= 0) { sim_note("THE TANK IS FULL"); break; }
                        if (!pay(cost)) return; veh_fuel[t] = VT[t].tank * 4; veh_lowfuel = 0; shopped(s, "FILLED UP", ML_RETAIL); } break;
        case PC_FPLANT0: case PC_FPLANT0 + 1: case PC_FPLANT0 + 2: case PC_FPLANT0 + 3: { static const u8 cost[4] = { 2, 4, 5, 8 }; int k = code - PC_FPLANT0;
            if (fm_n[k] >= 8) { sim_note("THAT FIELD IS FULL"); audio_voice(VOICE_NAH); return; }
            if (fm_n[k] && fm_g[k] >= fm_need(k)) { sim_note("HARVEST THOSE FIRST"); return; }
            if (sim_season() == 3 && !(fm_up & 4)) { sim_note("TOO COLD TO PLANT. A GREENHOUSE HELPS"); audio_voice(VOICE_NAH); return; }   // (winter: the ground is frozen)
            if (!pay(cost[k])) return; begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 150; s->act_var = 7;
            if (!fm_n[k]) { fm_g[k] = 0; fm_dry[k] = 0; fm_fz &= ~(1 << k); } if (fm_fert && !((fm_fz >> k) & 1)) { fm_fert--; fm_fz |= 1 << k; sim_note("FERTILIZED: A BIGGER HARVEST"); } fm_n[k]++; s->skill_farm += 12 * (s->plant == 2 ? 2 : 1);
            sim_note(fm_in_season(k) || (fm_up & 4) ? "PLANTED. NOW WATER IT" : "PLANTED. IT GROWS SLOW THIS SEASON"); break; }
        case PC_FWATER: { int any = 0, soaked = 1; for (int k = 0; k < 4; k++) if (fm_n[k]) { any = 1; if (!((fm_wet >> k) & 1) && fm_g[k] < fm_need(k)) soaked = 0; }
            if (!any) { sim_note("NOTHING IS PLANTED"); return; }
            for (int k = 0; k < 4; k++) if (fm_n[k] && fm_g[k] < fm_need(k)) fm_wet |= 1 << k;
            begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 200; s->act_var = 7; s->need[N_REST] = MAX(0, s->need[N_REST] - (2 << 10));
            if (soaked) { fm_wc += 2; sim_note("SOAKED ALREADY. TOO MUCH WATER?"); } else { fm_wc++; s->skill_farm += 10 * (s->plant == 2 ? 2 : 1); sim_note("WATERED. THEY GROW AT NIGHT"); }
            if (s->plant == 0 && fm_wc >= 8 && s == &sims[player]) { int r = (int)(look_rand() % 1000u);   // (EXTREMELY rare: too much watering gets into you)
                if (r < 20) { s->plant = 1; s->plant_d = 0; s->plant_t = 0; s->plant_sk = s->bore.look[LK_SKIN]; s->plant_hc = s->bore.look[LK_HCOL]; s->bore.look[LK_SKIN] = 4; s->bore.look[LK_HCOL] = 5; end_action(s); s->path_n = 0;
                    sim_note("YOU WATERED TOO MUCH... YOU ARE A STALK!"); audio_voice(VOICE_GROAN); }
                else if (r < 35) { s->plant = 2; s->plant_sk = s->bore.look[LK_SKIN]; s->plant_hc = s->bore.look[LK_HCOL]; s->bore.look[LK_SKIN] = 4; s->bore.look[LK_HCOL] = 5; sim_moodlet(s, ML_PROUD);
                    sim_note("YOU ARE PLANTBORE! THE SUN FEEDS YOU"); audio_voice(VOICE_YAHOO); } } break; }
        case PC_FHARV: { int lv = SKL(s->skill_farm), got = 0; char b_[40];
            for (int k = 0; k < 4; k++) if (fm_n[k] && fm_g[k] >= fm_need(k)) { int y = fm_n[k] * (2 + lv / 3) + (int)(look_rand() % (fm_n[k] + 1)); if ((fm_fz >> k) & 1) y = y * 3 / 2; if (fm_up & 8) y = y * 3 / 2; if (s->plant == 2) y += fm_n[k]; fm_fz &= ~(1 << k); fm_stock[k] = (u16)MIN(999, fm_stock[k] + y); got += y; snprintf(b_, sizeof b_, "%d %s PICKED", y, fmGood[k]); sim_note(b_); fm_n[k] = 0; fm_g[k] = 0; fm_dry[k] = 0; }
            if (!got) { sim_note("NOTHING IS READY"); return; }
            begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 260; s->act_var = 7; s->skill_farm += 30 + got * 3; s->need[N_REST] = MAX(0, s->need[N_REST] - (4 << 10));
            sim_moodlet(s, ML_HARVEST); if (got >= 12) sim_moodlet(s, ML_PROUD); wants_event(s, W_HARVEST, 0); break; }
        case PC_FHEN: if (fm_hens >= 6) break; if (!pay(40)) return; if (!fm_hens) fm_hh = 70; fm_hens++; sim_note("A NEW HEN! FEED IT DAILY"); s->skill_farm += 15; break;
        case PC_FCOW: if (fm_cows >= 3) break; if (!pay(250)) return; if (!fm_cows) fm_ch = 70; fm_cows++; sim_note("A NEW COW! MOO"); s->skill_farm += 25; break;
        case PC_FFEEDH: case PC_FFEEDC: { int c_ = code == PC_FFEEDC; if (!pay(c_ ? 5 : 2)) return; fm_fed |= 1 << c_; begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 120; s->act_var = 7; s->skill_farm += 8 + c_ * 2;
            sim_note(c_ ? "THE COWS ARE FED: MILK BY MORNING" : "THE HENS ARE FED: EGGS BY MORNING"); wants_event(s, W_ANIMALS, 0); break; }
        case PC_FPET: case PC_FBRUSH: { int c_ = code == PC_FBRUSH; fm_pet |= 1 << c_; begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 180; s->act_var = 7; s->skill_farm += 10;   // (once a day each: they are happier, and so are you)
            s->need[N_FUN] = MIN(100 << 10, s->need[N_FUN] + (15 << 10)); s->need[N_COMFY] = MIN(100 << 10, s->need[N_COMFY] + (8 << 10)); sim_moodlet(s, ML_HERD); wants_event(s, W_ANIMALS, 0);
            sim_note(c_ ? "THE COWS LOVE A BRUSH. MOO!" : "THE HENS CLUCK HAPPILY"); break; }
        case PC_FEGGS: case PC_FMILK: { int c_ = code == PC_FMILK, n_ = c_ ? fm_pm : fm_pe; char b_[32]; if (c_) { fm_milk = (u16)MIN(999, fm_milk + n_); fm_pm = 0; } else { fm_egg = (u16)MIN(999, fm_egg + n_); fm_pe = 0; }
            snprintf(b_, sizeof b_, c_ ? "%d MILK" : "%d EGGS", n_); strcat(b_, " COLLECTED"); sim_note(b_); s->skill_farm += 15 + c_ * 5; if (c_) { begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 200; s->act_var = 7; }
            if ((c_ ? fm_ch : fm_hh) >= 80) sim_moodlet(s, ML_HERD); wants_event(s, W_ANIMALS, 0); break; }
        case PC_FSELL: case PC_FSHOP: { int tot = 0; char b_[40];   // (the stand, or a grocery in town: the grocery pays a fifth more)
            for (int k = 0; k < 6; k++) tot += fm_have(k) * fmPr[k] * (k == 3 && sim_season() == 2 ? 3 : 2) / 2; if (!tot) { sim_note("NOTHING TO SELL"); return; }
            tot = tot * (100 + SKL(s->skill_char) * 3 + SKL(s->skill_farm) * 4 + (code == PC_FSHOP) * 20) / 100; cash += tot; memset(fm_stock, 0, sizeof fm_stock); fm_egg = fm_milk = 0; s->skill_char += 10;
            snprintf(b_, sizeof b_, "SOLD THE FARM FOOD: +$%d", tot); sim_note(b_); audio_voice(VOICE_YAHOO); fm_sold(s, tot); break; }
        case PC_FEAT: { int left = 2; for (int k = 3; k >= 1 && left; k--) while (left && fm_stock[k]) { fm_stock[k]--; left--; } while (left && fm_egg) { fm_egg--; left--; } while (left && fm_milk) { fm_milk--; left--; }
            begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 0; s->need[N_FOOD] += 40 << 10; sim_moodlet(s, ML_HEALTHY); s->skill_cook += 25; sim_note("FRESH FROM THE FARM!"); break; }
        case PC_FSPRAY: if (!fm_pest) { sim_note("NO PESTS"); return; } if (!pay(3)) return; fm_pest = 0; begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 120; s->act_var = 7; s->skill_farm += 15; s->skill_mech += 5; sim_note("SPRAYED: THE PESTS ARE GONE"); break;
        case PC_FFERT: if (fm_fert >= 9) { sim_note("THE SHED IS FULL"); return; } if (!pay(6)) return; fm_fert++; sim_note("A BAG OF FERTILIZER"); break;
        case PC_FORDER: { int have[6] = { fm_stock[0], fm_stock[1], fm_stock[2], fm_stock[3], fm_egg, fm_milk }; static const u8 pr[6] = { 4, 7, 9, 16, 5, 10 }; char b_[40];
            if (!fm_ordq || have[fm_ordk] < fm_ordq) { sim_note("NOT ENOUGH FOR THE ORDER"); return; }
            { int v = fm_ordq * pr[fm_ordk] * 2; if (fm_ordk < 4) fm_stock[fm_ordk] -= fm_ordq; else if (fm_ordk == 4) fm_egg -= fm_ordq; else fm_milk -= fm_ordq;
              v = v * (100 + SKL(s->skill_char) * 3) / 100; cash += v; fm_ordq = 0; fm_ordt = 0; s->skill_char += 12; s->skill_farm += 25; snprintf(b_, sizeof b_, "ORDER FILLED: +$%d", v); sim_note(b_); sim_moodlet(s, ML_PROUD); fm_sold(s, v); } break; }
        case PC_FUP: { static const int uc[4] = { 400, 120, 900, 1500 }; static const char *const un[4] = { "SPRINKLER: AUTO WATER", "SCARECROW: NO PESTS", "GREENHOUSE: NO FROST", "TRACTOR: BIGGER HARVESTS" };
            int k = 0; while (k < 4 && ((fm_up >> k) & 1)) k++; if (k >= 4) break; if (!pay(uc[k])) return; fm_up |= 1 << k; sim_note(un[k]); s->skill_mech += 40; audio_voice(VOICE_AGREE); break; }
        case PC_FFAIR: { char b_[40]; int sc = 20 + SKL(s->skill_farm) * 8 + (fm_up & 8 ? 8 : 0) + SKL(s->skill_creat) * 2 + (int)(look_rand() % 40u), pz;
            if (!fm_stock[3] || fm_fair) { sim_note("YOU HAVE ALREADY ENTERED"); return; } fm_stock[3]--; fm_fair = 1;
            pz = sc >= 85 ? 500 : sc >= 60 ? 200 : 40; cash += pz; s->skill_farm += 40; snprintf(b_, sizeof b_, sc >= 85 ? "BEST IN SHOW! +$%d" : sc >= 60 ? "SECOND PRIZE! +$%d" : "A RIBBON FOR TRYING +$%d", pz); sim_note(b_);
            if (sc >= 85) { sim_moodlet(s, ML_PROUD); audio_voice(VOICE_WIN); } fm_sold(s, pz); break; }
        case PC_MAIL: { static int mail_day = -1; char b_[40]; if (mail_day == day_count) { sim_note("NO NEW MAIL TODAY"); return; } mail_day = day_count; int r = (int)(look_rand() % 100u);
            if (r < 40) sim_note("JUNK MAIL. INTO THE BIN"); else if (r < 65) { sim_note("A LETTER FROM A FRIEND!"); sim_moodlet(s, ML_GOOD_CHAT); s->need[N_SOCIAL] = MIN(100 << 10, s->need[N_SOCIAL] + (20 << 10)); }
            else if (r < 85) { int v = 20 + (int)(look_rand() % 40u); cash += v; snprintf(b_, sizeof b_, "A CARD WITH $%d INSIDE!", v); sim_note(b_); sim_moodlet(s, ML_PAYDAY); }
            else { int v = 15 + (int)(look_rand() % 80u); cash += v; snprintf(b_, sizeof b_, "A REFUND CHECK: +$%d", v); sim_note(b_); }
            break; }
        case PC_SMELL: begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 120; s->act_var = 7; sim_moodlet(s, ML_FRESH); s->need[N_FUN] = MIN(100 << 10, s->need[N_FUN] + (10 << 10)); sim_note("LOVELY. SMELLS LIKE SPRING"); break;
        case PC_BIRDS: begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 240; s->act_var = 7; s->need[N_FUN] = MIN(100 << 10, s->need[N_FUN] + (18 << 10)); s->need[N_COMFY] = MIN(100 << 10, s->need[N_COMFY] + (8 << 10)); s->skill_logic += 6; break;
        case PC_SUN: { int hr = clock_min / 60, hot = hr >= 11 && hr <= 16 && sim_weather() == 0;
            begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 500; s->act_var = 7; s->need[N_COMFY] = MIN(100 << 10, s->need[N_COMFY] + (25 << 10)); s->need[N_FUN] = MIN(100 << 10, s->need[N_FUN] + (12 << 10));
            if (hot && (int)(look_rand() % 100u) < 22) { sim_moodlet(s, ML_SUNBURN); sim_note("OW. YOU GOT SUNBURNT"); } else if (hr >= 8 && hr <= 18) sim_moodlet(s, ML_TAN); break; }
        case PC_DIVE: { swim(s); if (s->action != ACT_SWIM) break;
            if ((int)(look_rand() % 100u) < 28 - SKL(s->skill_body) * 2) { sim_note("BELLY FLOP! OUCH"); sim_moodlet(s, ML_EMBARRASSED); s->skill_body += 10; }
            else { sim_note("WHAT A DIVE!"); sim_moodlet(s, ML_PROUD); s->need[N_FUN] = MIN(100 << 10, s->need[N_FUN] + (22 << 10)); s->skill_body += 30; } break; }
        case PC_LAPS: { swim(s); if (s->action != ACT_SWIM) break; s->skill_body += 60; s->need[N_REST] = MAX(0, s->need[N_REST] - (6 << 10)); sim_note("LAPS: A GOOD WORKOUT"); break; }
        case PC_GRILL: { if (!pay(8)) return; begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 1; s->skill_cook += 30; sim_moodlet(s, ML_GOOD_MEAL); sim_note("BURGERS ON THE GRILL!"); break; }
        case PC_FROOT: if (s->plant != 2) break; begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 600; s->act_var = 7;
            s->need[N_FOOD] = MIN(100 << 10, s->need[N_FOOD] + (50 << 10)); s->need[N_COMFY] = MIN(100 << 10, s->need[N_COMFY] + (30 << 10)); s->need[N_REST] = MIN(100 << 10, s->need[N_REST] + (25 << 10)); s->skill_farm += 25; sim_moodlet(s, ML_HEALTHY); sim_note("ROOTED IN THE SOIL: PEACE"); break;
        case PC_FSPROUT: if (s->plant != 2 || s->bore.stage != AG_ADULT || baby_due >= 0 || nsims >= MAX_SIMS || fm_fert < 2) { sim_note("IT NEEDS 2 FERTILIZER AND ROOM"); return; }
            fm_fert -= 2; baby_par[0] = s->uid; baby_par[1] = 0; baby_kind = 5; baby_due = life_minutes() + 1440 * (aging == 1 ? 1 : aging == 3 ? 3 : 2); sim_note("A SEEDLING IS SPROUTING..."); sim_moodlet(s, ML_NEW_BABY); break;
        case PC_FDRINK: if (s->plant != 2) break; begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 150; s->act_var = 7; s->need[N_FOOD] = MIN(100 << 10, s->need[N_FOOD] + (35 << 10)); s->need[N_CLEAN] = MIN(100 << 10, s->need[N_CLEAN] + (20 << 10)); sim_note("SLURP. DELICIOUS"); break;
        case PC_DIYFIX: { int t = veh_on ? veh_on - 1 : veh_sel, lv = SKL(s->skill_mech), miss = 100 - veh_cond[t], parts = miss * VT[t].price / 600 * (100 - lv * 7) / 100 + 2;
            if (miss <= 0) { sim_note("IT RUNS FINE"); break; }
            if (!pay(parts)) return; begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 300; s->act_var = 7;
            if ((int)(look_rand() % 100) < 38 - lv * 4) { sim_note("YOU MAKE IT WORSE. OOPS"); veh_cond[t] = (u8)MAX(0, veh_cond[t] - 8); s->skill_mech += 25; ; }
            else { int g = 25 + lv * 7; veh_cond[t] = (u8)MIN(100, veh_cond[t] + g); s->skill_mech += 70; s->skill_logic += 8; sim_note("FIXED IT YOURSELF!"); if (lv >= 3) sim_moodlet(s, ML_PROUD); } break; }
        case PC_MECHFIX: { int t = veh_on ? veh_on - 1 : veh_sel; if (!pay((100 - veh_cond[t]) * VT[t].price / 250 + 5)) return; veh_cond[t] = 100; s->skill_mech += 10; shopped(s, "FIXED UP", ML_RETAIL); break; }
        case PC_CSNACK: if (!pay(4)) return; begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 0; break;
        case PC_CDRINK: if (!pay(3)) return; begin_action(s, ACT_COFFEE, pie_mx, pie_my); s->act_var = 1; break;
        case PC_VEH0: case PC_VEH0 + 1: case PC_VEH0 + 2: case PC_VEH0 + 3: case PC_VEH0 + 4: case PC_VEH0 + 5: {
            int t = code - PC_VEH0; if (t < 0 || t >= VEH_N || ((veh_own >> t) & 1)) break;
            if (!pay(VT[t].price)) return; veh_own |= 1 << t; veh_fuel[t] = VT[t].tank * 4; veh_cond[t] = 100; veh_sel = t;
            { char b[40]; snprintf(b, sizeof b, "A NEW %s! O TO RIDE", VT[t].nm); shopped(s, b, ML_RETAIL); } } break;
        case PC_LATTE: if (!pay(5)) return; begin_action(s, ACT_COFFEE, pie_mx, pie_my); s->act_var = 1; break;
        case PC_CAKE: if (!pay(6)) return; begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 4; s->act_t = 150; break;
        case PC_DRINK: if (!pay(8)) return; begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_var = 5; s->act_t = 300; sim_say(s, "WHAT'S GOOD HERE?"); break;
        case PC_ROUND: if (!pay(40)) return; s->need[N_SOCIAL] = MIN(s->need[N_SOCIAL] + (40 << 10), 100 << 10); sim_moodlet(s, ML_ROUND);
                       wants_event(s, W_NIGHT, 0); sim_note("A ROUND FOR THE HOUSE!"); sim_say(s, "DRINKS ARE ON ME!"); audio_voice(VOICE_YAHOO); break;
        case PC_FILM: if (!pay(12)) return; begin_action(s, ACT_SIT, pie_mx, pie_my); s->act_var = 6; s->act_t = 480; sim_note("THE FILM IS STARTING"); break;
        case PC_POPCORN: if (!pay(4)) return; begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 4; s->act_t = 120; break;
        case PC_HAIR: case PC_LOOK: if (!pay(code == PC_HAIR ? 40 : 120)) return;
                      new_look(&s->bore, 2); if (code == PC_LOOK) new_look(&s->bore, 3); crowd_forget(CROWD_MAX + player);
                      sim_moodlet(s, ML_CUT); sim_note(code == PC_HAIR ? "A FRESH HAIRCUT!" : "A WHOLE NEW LOOK!"); audio_voice(VOICE_YAHOO); break;
        case PC_DINE: if (!pay(40)) return; begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 5; s->act_t = 300; sim_say(s, "THE CHEF'S SPECIAL");
                      if (world_at(pie_mx, pie_my) == 'h') {   // at a chair: sat at the table
                          s->x = pie_mx * TILE + TILE / 2; s->z = -(pie_my * TILE + TILE / 2); s->yaw = (world_item_facing(pie_mx, pie_my) * 1024) & 4095; s->gait.sit = 1; }
                      break;
        case PC_CHECKUP: if (!pay(30)) return; sim_moodlet(s, ML_HEALTHY); sim_note("THE DOCTOR SAYS: ALL CLEAR!"); audio_voice(VOICE_YAHOO); break;
        case PC_MASSAGE: if (!pay(50)) return; begin_action(s, ACT_SLEEP, pie_mx, pie_my); s->act_var = 2; s->act_t = 300; break;
        case PC_SOAK: if (!pay(15)) return; begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_var = 0; break;
        case PC_PRAC0: case PC_PRAC0 + 1: case PC_PRAC0 + 2: case PC_PRAC0 + 3: begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_var = (u8)(8 + (code - PC_PRAC0)); s->act_t = 300; sim_say(s, "FROM THE TOP!"); break;
        case PC_STEAL: { int v = 40 + (int)(look_rand() % 80); if ((int)(look_rand() % 100) < 45 + SKL(sims[player].skill_logic) * 3) { cash += v; char b_[40]; sprintf(b_, "SHOPLIFTED $%d WORTH. RUN!", v); sim_note(b_); sim_say(s, "NOBODY SAW"); sim_crime(CR_THEFT, 8); }   // (unseen: but a camera may have)
                       else { sim_say(s, "OH NO"); sim_note("CAUGHT SHOPLIFTING!"); sim_crime(CR_THEFT, 40); sim_arrest(1); } } break;
        case PC_PAINT: { char b_[48]; int lv = SKL(s->skill_creat);
            if (art_n >= 9) { sim_note("NO ROOM FOR MORE: SELL SOME AT A GALLERY"); audio_voice(VOICE_NAH); return; }
            if ((s->need[N_REST] >> 10) < 15) { sim_note("TOO TIRED TO PAINT"); audio_voice(VOICE_NAH); return; }
            begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_var = 7; s->act_t = 360; s->need[N_REST] = MAX(0, s->need[N_REST] - (6 << 10));
            { int q = 18 + lv * 8 + (int)(look_rand() % 26); art_n++; art_q = (u16)(art_q + q); s->skill_creat += 45;
              snprintf(b_, sizeof b_, q > 80 ? "A FINE PAINTING! QUALITY %d" : "YOU PAINT A PICTURE: QUALITY %d", q); sim_note(b_);
              if ((int)(look_rand() % 100) < 20 + lv * 6) { sim_moodlet(s, ML_INSPIRED); sim_note("YOU FEEL INSPIRED"); } } break; }
        case PC_SELLART: { char b_[48]; if (art_n <= 0) { sim_note("NO PAINTINGS TO SELL"); return; }
            int avg = art_q / art_n, price = 20 + avg * 3, lv = SKL(s->skill_creat); price += price * SKL(s->skill_char) * 3 / 100;
            if (lv >= 7 && (int)(look_rand() % 100) < 10) { price += 1500; sim_note("A COLLECTOR CALLS IT A MASTERPIECE!"); sim_moodlet(s, ML_PROUD); }
            cash += price; art_n--; art_q = (u16)(art_q > avg ? art_q - avg : 0); s->skill_char += 8; snprintf(b_, sizeof b_, "SOLD A PAINTING: +$%d", price); sim_note(b_); audio_voice(VOICE_YAHOO); break; }
        case PC_WRITE: { char b_[48]; int lv = SKL(s->skill_creat);
            if ((s->need[N_REST] >> 10) < 20) { sim_note("TOO TIRED TO WRITE"); audio_voice(VOICE_NAH); return; }
            begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 300; s->need[N_REST] = MAX(0, s->need[N_REST] - (8 << 10)); s->skill_creat += 25; s->skill_logic += 6;
            novel = (u8)MIN(100, novel + 6 + lv * 2 + SKL(s->skill_logic) / 2);
            if (novel >= 100) { int adv = 300 + lv * 200 + (int)(look_rand() % 300) + SKL(s->skill_char) * 40; cash += adv; novel = 0; royalty = (u16)MIN(300, royalty + 4 + lv * 3);
                snprintf(b_, sizeof b_, "NOVEL PUBLISHED! +$%d", adv); sim_note(b_); sim_note("AND ROYALTIES EVERY DAY"); sim_moodlet(s, ML_PROUD); audio_voice(VOICE_WIN); }
            else { snprintf(b_, sizeof b_, "THE NOVEL: %d%% WRITTEN", novel); sim_note(b_); } break; }
        case PC_RESORT: sim_want_resort = 1; break;
        case PC_CAMP: sim_want_resort = 2; break;
        case PC_HEIST: sim_want_prison = 3; break;
        case PC_FARMBOOK: sim_want_resort = 5; break;
        case PC_SKYSEA: sim_want_resort = 9; break;
        case PC_SPACE: if (space_cert() < 1) { sim_note("NOT CERTIFIED: GREAT MARKS OR A DEGREE FIRST"); audio_voice(VOICE_NAH); return; } sim_want_space = 1; break;
        case PC_ENR0: case PC_ENR0 + 1: case PC_ENR0 + 2: case PC_ENR0 + 3: { char b_[48]; int nl = col_lvl;
            if (code == PC_ENR0 + 3) { if (col_lvl < 1 || col_lvl > 2) return; } else { if (col_lvl > 0) { sim_note("YOU ALREADY HAVE A DEGREE"); return; } col_major = (u8)(code - PC_ENR0); }
            if (!pay(tuition[nl])) return; col_on = 1; col_cls = 0; col_gpa = 0; col_study = 0;
            snprintf(b_, sizeof b_, "ENROLLED: %s OF %s", lvlNm[nl], majNm[col_major]); sim_note(b_); sim_note("LECTURES: WEEKDAYS 8 AM TO 5 PM"); audio_voice(VOICE_AGREE); break; }
        case PC_STUDY: { if ((s->need[N_REST] >> 10) < 15) { sim_note("TOO TIRED TO STUDY"); audio_voice(VOICE_NAH); return; }
            begin_action(s, ACT_READ, pie_mx, pie_my); s->act_t = 300; s->need[N_REST] = MAX(0, s->need[N_REST] - (6 << 10)); col_study = (u8)MIN(40, col_study + 14); s->skill_logic += 30; sim_note("STUDIED HARD: READY FOR LECTURE"); break; }
        case PC_LECT: { char b_[48];
            if (clock_day >= 5 || clock_min < 8 * 60 || clock_min >= 17 * 60) { sim_note("LECTURES: WEEKDAYS 8 AM TO 5 PM"); audio_voice(VOICE_NAH); return; }
            if ((s->need[N_REST] >> 10) < 20) { sim_note("TOO TIRED FOR LECTURE"); audio_voice(VOICE_NAH); return; }
            begin_action(s, ACT_READ, pie_mx, pie_my); s->act_t = 480; s->need[N_REST] = MAX(0, s->need[N_REST] - (14 << 10));
            { s->skill_logic += 50; int sc = 36 + (int)(look_rand() % 26) + MIN(col_study, 36) + (s->mood > 30 ? 5 : 0) + wants_skill_level(s->skill_logic); if (sc > 100) sc = 100; col_study = col_study > 12 ? (u8)(col_study - 12) : 0;
              col_gpa = (u8)(((int)col_gpa * col_cls + sc) / (col_cls + 1)); col_cls++; snprintf(b_, sizeof b_, "LECTURE %d OF 8: GPA %d", col_cls, col_gpa); sim_note(b_); }
            if (col_cls >= 8) { col_on = 0;
                if (col_gpa >= 50) { col_g[col_lvl] = col_gpa; snprintf(b_, sizeof b_, "GRADUATED! %s OF %s", lvlNm[col_lvl], majNm[col_major]); col_lvl++; sim_note(b_); snprintf(b_, sizeof b_, "FINAL GPA %d", col_g[col_lvl - 1]); sim_note(b_); sims[player].mood = MIN(100, sims[player].mood + 60); audio_voice(VOICE_WIN); }
                else { sim_note("YOU FAILED THE COURSE. TRY AGAIN"); audio_voice(VOICE_CRY); } } break; }
        case PC_REC: { char b_[48];
            if (col_on) { snprintf(b_, sizeof b_, "%s OF %s: %d OF 8, GPA %d", lvlNm[col_lvl], majNm[col_major], col_cls, col_gpa); sim_note(b_); }
            if (col_lvl) { snprintf(b_, sizeof b_, "DEGREE: %s OF %s", lvlNm[col_lvl - 1], majNm[col_major]); sim_note(b_); }
            else if (!col_on) sim_note("NO DEGREE YET. A SCHOOL DESK ENROLLS YOU");
            snprintf(b_, sizeof b_, "SPACE: %s", space_cert_name(space_cert())); sim_note(b_); break; }
        case PC_SCOPE0: case PC_SCOPE0 + 1: case PC_SCOPE0 + 2: case PC_SCOPE0 + 3: case PC_SCOPE0 + 4: { int L = code - PC_SCOPE0 + 1, hr = clock_min / 60; char b_[48];
            static const struct { u8 L; const char *nm; u16 pay; } fd[10] = { {1,"MOON CRATERS",0}, {1,"SATURN'S RINGS",20}, {2,"A COMET",150}, {2,"A METEOR SHOWER",60}, {3,"A NEW ASTEROID",400}, {3,"A DISTANT NEBULA",300},
                                                                              {4,"A SUPERNOVA",1500}, {4,"A NEW EXOPLANET",2500}, {5,"A BLACK HOLE'S GLOW",6000}, {5,"A REPEATING SIGNAL",12000} };
            if (L >= 3 && space_cert() < L - 2) { snprintf(b_, sizeof b_, "NEEDS: %s", space_cert_name(L - 2)); sim_note(b_); audio_voice(VOICE_NAH); return; }
            int day = hr >= 6 && hr < 19;   // (daytime is allowed: slow, and the sun is in the sky)
            if (sim_weather()) { sim_note("CLOUDED OUT"); audio_voice(VOICE_NAH); return; }
            if ((s->need[N_REST] >> 10) < 15) { sim_note("TOO TIRED TO STARGAZE"); audio_voice(VOICE_NAH); return; }
            begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_t = 420; s->need[N_REST] = MAX(0, s->need[N_REST] - (8 << 10)); s->act_var = 0; s->skill_logic += day ? 8 : 20;
            if (day && (int)(look_rand() % 100) < 3 + L * 2) {   // a slip of the scope: straight into the sun
                sim_moodlet(s, ML_EYES); s->mood = MAX(-100, s->mood - 15); sim_note("OW! YOU LOOKED AT THE SUN!"); sim_note("YOUR EYES ARE BURNED"); sim_say(s, "MY EYES!"); audio_voice(VOICE_SHRIEK); break; }
            if ((int)(look_rand() % 100) >= (day ? 6 + L * 2 : 30 + L * 8) + wants_skill_level(s->skill_logic)) { astro_xp = (u16)MIN(60000, astro_xp + (day ? 1 : 2)); sim_note(day ? "THE SKY IS BRIGHT. NOT MUCH TO SEE" : "A QUIET NIGHT. BEAUTIFUL STARS"); break; }
            if (!day && L >= 1) {   // a mind that has looked up long enough: THEY look back
                static int last_abd = -99; int pct = MIN(14, wants_skill_level(s->skill_logic) * 3 / 2 + col_lvl * 2);   // (the LOGIC skill: the sharper the mind, the likelier THEY notice)
                if (pct > 0 && day_count - last_abd >= 4 && (int)(look_rand() % 100) < pct) { last_abd = day_count; int r = (int)(look_rand() % 100); sim_moodlet(s, ML_ABDUCT);
                    sim_note("A BRIGHT LIGHT FROM ABOVE!"); sim_note("YOU ARE ABDUCTED!"); clock_min = MIN(1439, clock_min + 180); astro_xp = (u16)MIN(60000, astro_xp + 40);
                    if (r < 38 && s->bore.stage == AG_ADULT && baby_due < 0 && nsims < MAX_SIMS) {   // a baby, from the stars (whoever you are)
                        baby_par[0] = s->uid; baby_par[1] = 0; baby_kind = (u8)((int)(look_rand() % 100) < 45 ? 4 : 3); int days = aging == 1 ? 1 : aging == 3 ? 3 : 2; baby_due = life_minutes() + days * 1440;
                        sim_note("THEY LEAVE YOU WITH A SURPRISE..."); sim_note("A BABY IS ON THE WAY!"); sim_say(s, "WHAT JUST HAPPENED?"); audio_voice(VOICE_SHRIEK); }
                    else if (r < 70) { int g = 300 + (int)(look_rand() % 700); cash += g; s->mood = MIN(100, s->mood + 25); snprintf(b_, sizeof b_, "A GIFT FROM ABOVE: +$%d", g); sim_note(b_); sim_say(s, "THEY WERE NICE!"); audio_voice(VOICE_YAHOO); }
                    else { s->mood = MAX(-100, s->mood - 20); s->need[N_REST] = MAX(0, s->need[N_REST] - (30 << 10)); sim_note("PROBED AND RETURNED. RUDE."); sim_say(s, "NEVER AGAIN..."); audio_voice(VOICE_CRY); }
                    break; } }
            { int ok[10], n = 0; for (int i = 0; i < 10; i++) if (fd[i].L <= (day ? 1 : L)) ok[n++] = i; int a = (int)(look_rand() % (u32)n), b2 = (int)(look_rand() % (u32)n); int k = ok[MAX(a, b2)];
              cash += fd[k].pay; astro_xp = (u16)MIN(60000, astro_xp + fd[k].L * 6); s->mood = MIN(100, s->mood + 12 + fd[k].L * 6);
              snprintf(b_, sizeof b_, fd[k].pay ? "FOUND %s! +$%d" : "YOU SEE %s", fd[k].nm, fd[k].pay); sim_note(b_); if (fd[k].L >= 4) audio_voice(VOICE_WIN); } break; }
        case PC_BUS: sim_want_bus = 1; break;
        case PC_PASS1: case PC_PASS7: case PC_PASS30: { int n = code == PC_PASS1 ? 1 : code == PC_PASS7 ? 7 : 30, pr = code == PC_PASS1 ? 10 : code == PC_PASS7 ? 30 : 90;
            if (!pay(pr)) return; bus_pass = MAX(bus_pass, day_count) + n; char b_[40]; sprintf(b_, "BUS PASS: %d DAYS LEFT", bus_pass - day_count); sim_note(b_); audio_voice(VOICE_FINISHED); } break;
        case PC_DOG: case PC_CAT: { if (!pay(code == PC_DOG ? 150 : 120)) return; pet_kind = code == PC_DOG ? 1 : 2; pet_col = (int)(look_rand() % 6); if (code == PC_DOG) dog_random(&pet_dog, look_rand());   /* (a store's puppy: a ready-made one; the CREATE A DOG screen makes your own) */ pet_hun = 100; pet_zone = -1; pet_ph = 0; sim_note(pet_kind == 1 ? "YOU GOT A PUPPY!" : "YOU GOT A KITTEN!"); audio_voice(VOICE_YAHOO); wants_event(s, W_FUN, 0); } break;
        case PC_PETFEED: if (!pay(3)) return; pet_hun = 100; sim_say(s, "GOOD PET!"); sim_note("YUM! YOUR PET IS FED"); break;
        case PC_PETPLAY: s->need[N_FUN] = MIN(s->need[N_FUN] + (12 << 10), 100 << 10); s->need[N_SOCIAL] = MIN(s->need[N_SOCIAL] + (6 << 10), 100 << 10); pet_hun = MAX(0, pet_hun - 4); pet_ph += 900; sim_say(s, "WHO'S A GOOD PET?"); break;
        case PC_ADMIRE: begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_var = 7; s->act_t = 200; break;
        case PC_PAINTING: if (!pay(300)) return; s->skill_creat += 60; shopped(s, "A PAINTING, JUST FOR YOU", ML_INSPIRED); break;
        case PC_ROOM: if (room_here()) { sim_note("YOUR ROOM IS PAID TILL NOON"); return; }
                      if (!pay(60)) return; room_bx = pie_pi.bx; room_by = pie_pi.by; room_until = (day_count + 1) * 1440 + 12 * 60;
                      sim_note("A ROOM TILL NOON TOMORROW"); audio_voice(VOICE_YAHOO); break;
    }
}
#ifdef AUTO_GROW
int sim_debug_grow(int what, int arg) {   // (test builds) hurry a family's life along
    Sim *me = &sims[player];
    if (arg < 0 || arg >= nsims) return 0;
    Sim *o = &sims[arg];
    switch (what) {
        case 1: o->age_days = (u8)MAX(0, sim_stage_len(o->bore.stage) - 1); return 1;                 // a birthday at the next midnight
        case 2: { int ax, ay; if (!approach_tile(me, sim_tile_x(o), sim_tile_y(o), &ax, &ay)) return 0;  // stand facing them
                  if (o->action != ACT_NONE && o->action != ACT_SLEEP) end_action(o); o->path_n = 0; o->action = o->action == ACT_SLEEP ? ACT_SLEEP : ACT_NONE; o->wander_t = 900;
                  me->x = ax * TILE + TILE / 2; me->z = -(ay * TILE + TILE / 2); me->lvl = o->lvl; me->action = ACT_NONE; me->path_n = 0;
                  me->yaw = yaw_towards(o->x - me->x, o->z - me->z); return 1; }
        case 3: me->rel[arg] = 80; o->rel[player] = 80; return 1;                                       // they get on
        case 4: baby_due = life_minutes(); return 1;                                                     // the baby, now
        case 5: o->need[N_FOOD] = 20 << 10; o->need[N_SOCIAL] = 30 << 10; if (o->action == ACT_SLEEP) end_action(o); return 1;   // a hungry baby
        case 6: { int x, y; if (!free_floor_near(me->lvl, sim_tile_x(o) + 1, sim_tile_y(o), &x, &y)) return 0;   // a crib beside them
                  *world_cell(me->lvl, x, y) = CH_CRIB; world_touched(); return 1; }
        case 7: o->bore.stage = AG_ELDER; look_fix(&o->bore); look_build(&o->bore); crowd_forget(CROWD_MAX + arg); o->age_days = (u8)sim_stage_len(AG_ELDER); return 1;   // at the end of a long life
        case 8: me->ai = 1; player = arg; o->ai = 0; o->path_n = 0; if (o->action == ACT_WALK) o->action = ACT_NONE; return 1;   // play them
        case 9: baby_to_bed(o); return 1;                                                              // a baby, off to sleep
    }
    return 0;
}
#endif
#if defined(AUTO_SPORT) || defined(AUTO_PLACES) || defined(AUTO_TOUR) || defined(AUTO_GROW) || defined(AUTO_HOME) || defined(AUTO_ISLE) || defined(AUTO_SCENE) || defined(AUTO_PHONE) || defined(AUTO_SAVE) || defined(AUTO_LE)
int sim_debug_goto(char c, int lot) {   // (test builds) stand the player in front of the nearest c (at a place of that kind, lot >= 0)
    Sim *s = &sims[player]; int sx = sim_tile_x(s), sy = sim_tile_y(s), best = 1 << 30, bx = -1, by = -1, tx = 0, ty = 0;
    for (int y = 1; y < MH - 1; y++) for (int x = 1; x < MW - 1; x++) {
        if (lifeMap[y][x] != c) continue;
        PlaceInfo pi; if (lot >= 0 && !(place_at(x, y, &pi) && pi.lot == lot)) continue;
        int ax, ay; if (!approach_tile(s, x, y, &ax, &ay)) continue;
        int d = (x - sx) * (x - sx) + (y - sy) * (y - sy); if (d < best) { best = d; bx = x; by = y; tx = ax; ty = ay; }
    }
    if (bx < 0) return 0;
    s->x = tx * TILE + TILE / 2; s->z = -(ty * TILE + TILE / 2); s->lvl = 0; s->action = ACT_NONE; s->path_n = 0;
    s->yaw = yaw_towards(bx * TILE + TILE / 2 - s->x, -(by * TILE + TILE / 2) - s->z);
    return 1;
}
#endif
int sim_front_service(void) {   // right in front of you: a place's counter, shelf or machine, or something you work at (before the people behind it)
    const Sim *s = &sims[player]; int fx, fy;
    world_lvl = s->lvl;
    int r = front_tile(s, &fx, &fy);
    char c = r ? world_at(fx, fy) : 0;
    r = c && (job_here(fx, fy, c) || (place_at(fx, fy, 0) && usable_at(fx, fy, c)));   // (at a place: whatever is right ahead)
    world_lvl = 0;
    return r;
}
void sim_take_job(int car, int bx, int by, const char *place) {
    if (job_none) job_level = 0;
    job_none = 0;
    job_take(car, bx, by, place);
    char b[40]; snprintf(b, sizeof b, "NEW JOB: %s", job_title()); sim_note(b);
    sim_moodlet(&sims[player], ML_NEW_JOB); audio_voice(VOICE_YAHOO);
}
static void sim_pie_do_(int i) {
    Sim *s = &sims[player];
    if (i < 0 || i >= pie_n) return;
    if (pie_who >= 0) {
        int code = pie_code[i];
        if (code > 20 && code < 20 + CARE_END) { care_start(s, pie_who, code - 20); return; }
        if (code == PC_FTALK) { Sim *o = &sims[pie_who]; if (o->plant != 1) return; if (o->plant_t) { sim_note("IT HEARS YOU. COME BACK TOMORROW"); return; }
            o->plant_t = 1; if (o->plant_d) o->plant_d--; o->mood = MIN(100, o->mood + 30); o->need[N_SOCIAL] = MIN(100 << 10, o->need[N_SOCIAL] + (40 << 10)); sim_note("THE STALK RUSTLES. IT LIVES ON"); return; }
        if (code == PC_FCURE) { Sim *o = &sims[pie_who]; if (o->plant != 1) return; if (!pay(500)) return; o->plant = 0; o->plant_d = 0; o->bore.look[LK_SKIN] = o->plant_sk; o->bore.look[LK_HCOL] = o->plant_hc; o->mood = MIN(100, o->mood + 40); sim_moodlet(o, ML_PROUD); sim_note("THE TONIC WORKED! YOU ARE YOURSELF AGAIN"); return; }
        if (code == 15) { start_family(s, &sims[pie_who]); return; }
        if (code == 16) { go_steady(s, &sims[pie_who]); return; }
        social(s, &sims[pie_who], code); return;
    }
    switch (pie_code[i]) {   // the family's own
        case 30: candles(s); return;
        case 31: begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 9; s->act_t = 120; return;
        case 32: begin_action(s, ACT_READ, pie_mx, pie_my); s->act_var = 5; s->act_t = 200; return;
        case 33: begin_action(s, ACT_READ, pie_mx, pie_my); s->act_var = 3; s->act_t = 240; return;
        case 34: begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 8; s->act_t = 300; sim_say(s, "A CAKE FOR THE BIRTHDAY!"); return;
        case 35: adopt(s); return;
        case 36: home_buy(world_wbx + pie_mx / BM_BLK, world_wby + pie_my / BM_BLK, HB_KEEP); return;   // (main moves the family in)
        case PC_BUYPROP: home_buy(world_wbx + pie_mx / BM_BLK, world_wby + pie_my / BM_BLK, HB_PROP); return;
        case 38: isl_craft_asked = 1; return;                                                  // (main opens the list of things to make)
        case 39: raft_sail(); return;
        case 37: if (world_zone != ZONE_CITY) return;
                 { BInfo b; bm_block(world_wbx + pie_mx / BM_BLK, world_wby + pie_my / BM_BLK, &b); char t[40];
                   snprintf(t, sizeof t, "FOR SALE: %s", home_kind(b.lot)); sim_note(t); sim_note(home_blurb(b.lot)); return; }
    }
    if (pie_code[i] >= 100 && pie_code[i] < 114 && action_for(world_at(pie_mx, pie_my)) == ACT_GATHER) { begin_action(s, ACT_GATHER, pie_mx, pie_my); s->act_var = (u8)(pie_code[i] - 100); s->act_t = gather_len(s->act_var); return; }   // (the island: 100 + what is gathered. Only for a thing you gather from: the codes past PC_PRAC_END are 100 and up too, and every one of them landed here)
    if (pie_code[i] >= PC_WORK) { place_do(s, pie_code[i]); return; }
    if (pie_code[i] == 1 && world_zone == ZONE_ISLAND && world_at(pie_mx, pie_my) == 's') {   // the castaway's meal
        begin_action(s, ACT_EAT, pie_mx, pie_my); if (s->action == ACT_EAT) { s->act_var = 1; s->act_t = 300; } return; }
    char c = world_at(pie_mx, pie_my); int act = action_for(c);
    if (c == ' ' || act == ACT_STAIRS) {   // the stairs: up from the bottom, down from the top
        static const signed char fx[4] = { 0, 1, 0, -1 }, fy[4] = { 1, 0, -1, 0 };
        int up = c != ' ', lo = up ? s->lvl : s->lvl - 1;
        int keep = world_lvl; world_lvl = lo; int f = world_item_facing(pie_mx, pie_my); world_lvl = keep;
        int tx = up ? pie_mx - fx[f] : pie_mx + fx[f], ty = up ? pie_my - fy[f] : pie_my + fy[f], tl = up ? lo + 1 : lo;
        world_lvl = tl; int ok = world_walkable(tx, ty); world_lvl = keep;
        if (!ok) { sim_note(up ? "BUILD A FLOOR WHERE THE STAIRS COME OUT" : "SOMETHING IS IN THE WAY DOWN THERE"); return; }
        s->lvl = (u8)tl; s->x = tx * TILE + TILE / 2; s->z = -(ty * TILE + TILE / 2); s->yaw = up ? ((f * 1024 + 2048) & 4095) : ((f * 1024) & 4095);   // (up: into the flight; down: the way it faces)
        audio_voice(VOICE_FINISHED); return;
    }
    if (pie_code[i] >= 9 && (c == 'R' || c == 'A' || c == 'j')) {   // tune in (or off), then dance to it
        int st = pie_code[i] == 9 ? -1 : pie_code[i] - 10; audio_station_play(st);
        char b[32]; sprintf(b, st < 0 ? "RADIO OFF" : "NOW ON %s", audio_station_name(st)); sim_note(b);
        if (st < 0) return;
    }
    if (act == ACT_TRAVEL) { travel_asked = 1; return; }
    if (act == ACT_PHONE && pie_code[i] == 1) { if (cash < 40) { sim_note("PIZZA IS $40"); return; } cash -= 40; }
    if (act == ACT_PHONE && pie_code[i] == 2) { extern void throw_party(void); throw_party(); return; }
    begin_action(s, act, pie_mx, pie_my);
    if (s->action != act) return;                                         // (it could not: nothing to eat, on the island)
    s->act_var = pie_code[i];
    if (act == ACT_SLEEP && s->act_var == 1) s->act_t = 300;              // a nap: an hour or so
    if (act == ACT_EAT && s->act_var == 1) {                              // cooking takes a while; it is a good meal if there is good food in
        s->act_t = 420;
        if (world_zone == ZONE_ISLAND) { } else if (groceries > 0) groceries--; else { s->act_var = 3; sim_note("NO GROCERIES, SO A PLAIN MEAL"); }
    }
}

static int sim_player_interact_(void) {
    Sim *s = &sims[player];
    if (s->action != ACT_NONE && s->action != ACT_WALK) { end_action(s); return 1; }
    int mx, my, who;
    if (!target_of(s, &mx, &my, &who)) return 0;
    if (who >= 0) {
        Sim *o = &sims[who];
        if (o->mood < -45 && !(look_rand() & 3)) { sim_say(o, "NOT NOW."); wants_fear(s, F_REJECT); s->rel[who] -= 4; audio_voice(VOICE_DISAGREE); return 1; }   // turned down
        if (o->action != ACT_NONE && o->action != ACT_WALK) end_action(o);
        begin_action(s, ACT_TALK, sim_tile_x(o), sim_tile_y(o));
        begin_action(o, ACT_TALK, sim_tile_x(s), sim_tile_y(s));
        s->talk_to = who; o->talk_to = player; o->path_n = 0;
        if (o->grump) sim_say(o, grump_line());
        audio_voice(look_rand() & 1 ? VOICE_JOKE : VOICE_AGREE);
        return 1;
    }
    char c = world_at(mx, my);
    if (c == 'x' && !(job_here(mx, my, c) && is_work())) { sim_note(job_here(mx, my, c) ? "NOT WORK TIME" : "YOU DO NOT WORK HERE"); return 0; }
    if (action_for(c) == ACT_NONE) return 0;
    if (c == 'R' || c == 'A') audio_jukebox_next();
    if (action_for(c) == ACT_TRAVEL) { travel_asked = 1; return 1; }
    begin_action(s, action_for(c), mx, my);
    return 1;
}
void sim_player_cancel(void) {
    Sim *s = &sims[player];
    if (s->action != ACT_NONE) { end_action(s); return; }
    if (veh_on) { int t = veh_on - 1; veh_dismount(s, 0); if (!ride_from(s, 2 + t)) sim_note("OFF AND ON YOUR FEET"); return; }
    if (on_board) { skate_off(s); if (!ride_from(s, 1)) sim_note("OFF THE BOARD AND ON YOUR FEET"); return; }
    if (has_board || veh_own) ride_from(s, 0);
}
void sim_player_cancel_sim(Sim *s) { if (s->action != ACT_NONE) end_action(s); }

const char * sim_hint(void) { world_lvl = sims[player].lvl; const char * r = sim_hint_(); world_lvl = 0; return r; }

int sim_pie_open(char opt[8][24]) { world_lvl = sims[player].lvl; int r = sim_pie_open_(opt); world_lvl = 0; return r; }

void sim_pie_do(int i) { world_lvl = sims[player].lvl; sim_pie_do_(i); world_lvl = 0; }

int sim_player_interact(void) { world_lvl = sims[player].lvl; int r = sim_player_interact_(); world_lvl = 0; return r; }

// SAN BOREAS's map window moved by (dtx, dty) tiles: everyone in the household moves with it (anyone left off the
// edge of the new window is brought to the player's side, their errand dropped)
void sim_shift(int dtx, int dty) {
    const Sim *p = &sims[player];
    int px = p->x - dtx * TILE, pz = p->z + dty * TILE;
    for (int i = 0; i < nsims; i++) {
        Sim *s = &sims[i];
        s->x -= dtx * TILE; s->z += dty * TILE;
        if (s->act_x >= 0) { s->act_x -= dtx; s->act_y -= dty; }
        int bad = 0;
        for (int k = 0; k < s->path_n; k++) { int x = s->path_x[k] - dtx, y = s->path_y[k] - dty; if (x < 0 || y < 0 || x >= MW || y >= MH) bad = 1; s->path_x[k] = (u8)x; s->path_y[k] = (u8)y; }
        { int x = s->dest_x - dtx, y = s->dest_y - dty; if (x < 0 || y < 0 || x >= MW || y >= MH) bad = 1; s->dest_x = (u8)CLAMP(x, 0, MW - 1); s->dest_y = (u8)CLAMP(y, 0, MH - 1); }
        if (s->act_x >= MW || s->act_y >= MH || (s->act_x < 0 && s->act_x != -1) || s->act_y < 0) bad = 1;
        int out = s->x < TILE || -s->z < TILE || s->x >= (MW - 1) * TILE || -s->z >= (MH - 1) * TILE;
        if (bad || out) { if (s->action != ACT_NONE) end_action(s); s->path_n = 0; s->path_i = 0; s->act_x = -1; s->action = ACT_NONE; }
        if (out && i != player) { s->x = px + 80 * (i % 3) - 80; s->z = pz - 80 * (i / 3); s->lvl = 0; }
    }
}

// ---- the memory card (save.c): the family, the clock, the money, the job; the rest of a Sim starts over (standing, idle) ----
__attribute__((optimize("Os"))) void sim_save(void) {
    sv_u8(nsims); sv_u8(player);
    sv_u16(clock_min); sv_u8(clock_day); sv_u32((u32)day_count); sv_u32((u32)cash);
    sv_u8(job_level); sv_u16(job_worked); sv_u16(job_days_good); sv_u8(job_strikes); sv_u8(job_car);
    sv_u16(job_bx); sv_u16(job_by); sv_str(job_place, 23);
    sv_u16(groceries); sv_u16(room_bx); sv_u16(room_by); sv_u32((u32)room_until);
    sv_u8(has_board); sv_u16(bills_due); sv_u32((u32)baby_due); sv_n(baby_par, 2); sv_u8(baby_kind); sv_u8(next_uid); sv_u8(cake_slices);
    sv_u8(sim_story_over); sv_str(sim_end_name, NAME_LEN - 1);
    sv_u8(ngraves); for (int i = 0; i < ngraves; i++) { sv_u8(graves[i].x); sv_u8(graves[i].y); sv_u8(graves[i].city); sv_str(graves[i].name, NAME_LEN - 1); }
    sv_u16(story_paid); sv_u16(story_talks); sv_n(story_visit, 4);
    sv_u8(phone_tier); sv_u8(handheld);   // (version 10)
    sv_u8(job_none);   // (version 12)
    sv_u16((u16)arcade_tix); sv_u16((u16)fight_wins); sv_u8((u8)fight_champ); sv_u8((u8)fight_best);   // (version 15)   arcade_save();   // (version 45: the high scores, tokens, prizes, medals)
    gotchi_save(); sv_u16(dj_xp); sv_u8(dj_heat); band_save(); sv_u8(pet_kind); sv_u8(pet_col); sv_u8(pet_hun); sv_u16(bus_pass); sv_n(pet_dog.look, DL_N); sv_n(pet_dog.tr, DOG_TR); sv_str(pet_dog.name, DOG_NAME - 1); sv_u8(rap_sheet); sv_n(crime_n, CR_N); sv_u8(crime_evid); sv_n(space_tech, 3); sv_u16(space_done); sv_u8(col_lvl); sv_u8(col_major); sv_u8(col_on); sv_u8(col_cls); sv_u8(col_gpa); sv_u8(col_study); sv_n(col_g, 3); sv_u16(astro_xp); for (int t = 0; t < VEH_N; t++) sv_u8(veh_cond[t]); sv_n(fm_n, 4); sv_n(fm_g, 4); sv_n(fm_dry, 4); sv_u8(fm_wet); sv_u8(fm_hens); sv_u8(fm_cows); sv_u8(fm_fed); for (int k = 0; k < 4; k++) sv_u16(fm_stock[k]); sv_u16(fm_egg); sv_u16(fm_milk); sv_u16(fm_pe); sv_u16(fm_pm); sv_u8(fm_fert); sv_u8(fm_fz); sv_u8(fm_pest); sv_u8(fm_up); sv_u8(fm_wc); sv_u8(fm_ordk); sv_u8(fm_ordq); sv_u8(fm_ordt); sv_u8(fm_fair); sv_u8(art_n); sv_u8(novel); sv_u16(art_q); sv_u16(royalty);   // (version 21; the band: version 25)
    sv_u8(veh_own); sv_u8(veh_sel); for (int t = 0; t < VEH_N; t++) sv_u16((u16)veh_fuel[t]); traffic_save();   // (version 18: the vehicles)
    realm_save();   // (version 15: KINGDOM MODE's realm)
    sv_n(pk_data, sizeof pk_data);   // (version 39: what a pack keeps)
    for (int i = 0; i < nsims; i++) {
        const Sim *s = &sims[i]; const Bore *b = &s->bore;
        sv_n(b->look, LK_COUNT); sv_u8(b->stage); sv_u8(b->asp); sv_u8(b->ltw); sv_n(b->tr, TR_N); sv_str(b->first, NAME_LEN - 1); sv_str(b->last, NAME_LEN - 1);
        sv_u32((u32)s->x); sv_u32((u32)s->z); sv_u16(s->yaw);
        for (int k = 0; k < NEED_N; k++) sv_u32((u32)s->need[k]);
        sv_u8(s->mood + 100); sv_u8(s->ai); sv_u16(s->skill_cook); sv_u16(s->skill_body); sv_u16(s->skill_fun); sv_u8(s->dead); sv_u8(s->grump); for (int k = 0; k < BS_N; k++) sv_u16((u16)s->skill_band[k]);   // (version 24)
        sv_u16((u16)s->skill_logic); sv_u16((u16)s->skill_char); sv_u16((u16)s->skill_creat); sv_u16((u16)s->skill_mech);   // (version 32)
        sv_u16((u16)s->skill_farm);   // (version 34)
        sv_u8(s->plant); sv_u8(s->plant_d); sv_u8(s->plant_t); sv_u8(s->plant_sk); sv_u8(s->plant_hc);   // (version 35)
        sv_u8(s->tod_walk); sv_u8(s->tod_talk); sv_u8(s->tod_potty); sv_u8(s->tod_story);   // (version 36)
        for (int k = 0; k < 4; k++) { sv_u8(s->want[k].kind); sv_u8(s->want[k].arg); sv_u16(s->want[k].pts); sv_u16(s->want[k].age); }
        for (int k = 0; k < 2; k++) { sv_u8(s->fear[k].kind); sv_u8(s->fear[k].arg); sv_u16(s->fear[k].pts); sv_u16(s->fear[k].age); }
        sv_u16(s->asp_pts); sv_n(s->rel, MAX_SIMS); sv_u8(s->lvl);
        for (int k = 0; k < 6; k++) { sv_u8(s->ml[k].id); sv_u8(s->ml[k].val); sv_u16(s->ml[k].t); }
        sv_u8(s->uid); sv_n(s->par, 2); sv_u8(s->partner); sv_u8(s->age_days); sv_u8(s->bday); sv_u8(s->away); sv_u8(s->grade); sv_u8(s->homework);
        { int act = s->action; if (act == ACT_WALK || act == ACT_TRAVEL || act == ACT_STAIRS || act == ACT_TALK || act == ACT_CARE || act == ACT_RUN) act = ACT_NONE;   // (what they were doing, if it stays put: asleep, eating, at work...)
          sv_u8(act); sv_u16(MIN(s->act_t, 65535)); sv_u8(s->act_x); sv_u8(s->act_y); sv_u8(s->act_var); }
    }
}
__attribute__((optimize("Os"))) void sim_load(const Bore *blank) {   // (after sim_init; the faces are built here)
    int n = ld_u8(), pl = ld_u8();
    clock_min = ld_u16(); clock_day = ld_u8(); day_count = (int)ld_u32(); cash = (long)(s32)ld_u32();
    job_level = ld_u8(); job_worked = ld_u16(); job_days_good = ld_u16(); job_strikes = ld_u8(); job_car = ld_u8();
    job_bx = (s16)ld_u16(); job_by = (s16)ld_u16(); ld_str(job_place, 23);
    groceries = ld_u16(); room_bx = (s16)ld_u16(); room_by = (s16)ld_u16(); room_until = (int)ld_u32();
    has_board = ld_u8(); on_board = 0; board_lift = 0; skate_reset(); bills_due = ld_u16(); baby_due = (int)ld_u32(); ld_n(baby_par, 2); baby_kind = (u8)ld_u8(); next_uid = (u8)ld_u8(); cake_slices = ld_u8();
    sim_story_over = ld_u8(); ld_str(sim_end_name, NAME_LEN - 1);
    { int g = ld_u8(); ngraves = MIN(g, 6); } for (int i = 0; i < ngraves; i++) { graves[i].x = (u8)ld_u8(); graves[i].y = (u8)ld_u8(); graves[i].city = (u8)ld_u8(); ld_str(graves[i].name, NAME_LEN - 1); }
    story_paid = ld_u16(); story_talks = ld_u16(); ld_n(story_visit, 4);
    if (save_ver >= 10) { phone_tier = ld_u8() & 3; handheld = ld_u8() & 3; } else { phone_tier = 3; handheld = 0; }   // (an older life keeps every app it had)
    job_none = save_ver >= 12 ? ld_u8() & 1 : 0;   // (an older life keeps its job)
    memset(&arc, 0, sizeof arc); memset(&arun, 0, sizeof arun); if (save_ver >= 15) { arcade_tix = ld_u16(); fight_wins = ld_u16(); fight_champ = ld_u8() & 1; fight_best = ld_u8(); } else arcade_tix = fight_wins = fight_champ = fight_best = 0; if (save_ver >= 45) arcade_load();
    gotchi_load(save_ver); if (save_ver >= 23) { dj_xp = ld_u16(); dj_heat = ld_u8(); } else { dj_xp = 0; dj_heat = 0; } band_load(save_ver); if (save_ver >= 27) { pet_kind = ld_u8() & 3; pet_col = ld_u8() % 6; pet_hun = ld_u8(); bus_pass = ld_u16(); if (save_ver >= 28) { ld_n(pet_dog.look, DL_N); ld_n(pet_dog.tr, DOG_TR); ld_str(pet_dog.name, DOG_NAME - 1); } else dog_random(&pet_dog, 1); if (save_ver >= 29) { rap_sheet = ld_u8(); ld_n(crime_n, CR_N); crime_evid = ld_u8(); if (save_ver >= 30) { ld_n(space_tech, 3); space_done = ld_u16(); } else { space_done = 0; memset(space_tech, 0, 3); }
            if (save_ver >= 31) { col_lvl = ld_u8() & 3; col_major = ld_u8() % 3; col_on = ld_u8() & 1; col_cls = ld_u8(); col_gpa = ld_u8(); col_study = ld_u8(); ld_n(col_g, 3); astro_xp = ld_u16(); for (int t = 0; t < VEH_N; t++) veh_cond[t] = 100; if (save_ver >= 33) { for (int t = 0; t < VEH_N; t++) veh_cond[t] = ld_u8(); ld_n(fm_n, 4); ld_n(fm_g, 4); ld_n(fm_dry, 4); fm_wet = ld_u8(); fm_hens = ld_u8(); fm_cows = ld_u8(); fm_fed = ld_u8(); for (int k = 0; k < 4; k++) fm_stock[k] = ld_u16(); fm_egg = ld_u16(); fm_milk = ld_u16(); fm_pe = ld_u16(); fm_pm = ld_u16(); if (save_ver >= 35) { fm_fert = ld_u8(); fm_fz = ld_u8(); fm_pest = ld_u8(); fm_up = ld_u8(); fm_wc = ld_u8(); fm_ordk = ld_u8(); fm_ordq = ld_u8(); fm_ordt = ld_u8(); fm_fair = ld_u8(); } else { fm_fert = fm_fz = fm_pest = fm_up = fm_wc = fm_ordk = fm_ordq = fm_ordt = fm_fair = 0; } art_n = ld_u8(); novel = ld_u8(); art_q = ld_u16(); royalty = ld_u16(); } else { art_n = novel = 0; art_q = royalty = 0; } } else { col_lvl = col_major = col_on = col_cls = col_gpa = col_study = 0; memset(col_g, 0, 3); astro_xp = 0; } } else { rap_sheet = 0; memset(crime_n, 0, sizeof crime_n); crime_evid = 0; } } else { pet_kind = 0; pet_hun = 100; bus_pass = 0; } pet_zone = -1;
    veh_on = 0; veh_odo = 0; veh_lowfuel = 0;
    if (save_ver >= 18) { veh_own = ld_u8() & ((1 << VEH_N) - 1); veh_sel = ld_u8() % VEH_N; for (int t = 0; t < VEH_N; t++) veh_fuel[t] = ld_u16(); } else { veh_own = 0; veh_sel = 0; for (int t = 0; t < VEH_N; t++) veh_fuel[t] = 0; }
    traffic_load(save_ver);
    realm_load(save_ver);
    if (save_ver >= 39) ld_n(pk_data, sizeof pk_data); else memset(pk_data, 0, sizeof pk_data);
    for (int i = 0; i < n && i < MAX_SIMS; i++) {
        sim_add(blank, 0, 0, 1);   // (a blank Bore: the Bore is large, so it is filled in place)
        Sim *s = &sims[nsims - 1]; Bore *b = &s->bore;
        memset(b->look, 0, LK_COUNT); ld_n(b->look, save_ver >= 6 ? LK_COUNT : 120); b->stage = (u8)ld_u8(); b->asp = (u8)ld_u8(); b->ltw = (u8)ld_u8(); ld_n(b->tr, TR_N); ld_str(b->first, NAME_LEN - 1); ld_str(b->last, NAME_LEN - 1);
        b->tex_eye.tpage = 0; b->tex_mouth.tpage = 0; b->baked = 0;
        look_build(b);
        s->x = (int)ld_u32(); s->z = (int)ld_u32(); s->yaw = ld_u16();
        for (int k = 0; k < NEED_N; k++) s->need[k] = (int)ld_u32();
        s->mood = ld_u8() - 100; s->ai = ld_u8(); s->skill_cook = ld_u16(); s->skill_body = ld_u16(); s->skill_fun = ld_u16(); s->dead = ld_u8(); s->grump = (u8)ld_u8(); for (int k = 0; k < BS_N; k++) s->skill_band[k] = save_ver >= 24 ? ld_u16() : 0;
        if (save_ver >= 32) { s->skill_logic = ld_u16(); s->skill_char = ld_u16(); s->skill_creat = ld_u16(); s->skill_mech = ld_u16(); } else s->skill_logic = s->skill_char = s->skill_creat = s->skill_mech = 0;
        if (save_ver >= 34) s->skill_farm = ld_u16(); else s->skill_farm = 0;
        if (save_ver >= 35) { s->plant = ld_u8(); s->plant_d = ld_u8(); s->plant_t = ld_u8(); s->plant_sk = ld_u8(); s->plant_hc = ld_u8(); } else s->plant = s->plant_d = s->plant_t = 0;
        if (save_ver >= 36) { s->tod_walk = ld_u8(); s->tod_talk = ld_u8(); s->tod_potty = ld_u8(); s->tod_story = ld_u8(); } else s->tod_walk = s->tod_talk = s->tod_potty = s->tod_story = 0;
        for (int k = 0; k < 4; k++) { s->want[k].kind = (u8)ld_u8(); s->want[k].arg = (u8)ld_u8(); s->want[k].pts = (u16)ld_u16(); s->want[k].age = (u16)ld_u16(); }
        for (int k = 0; k < 2; k++) { s->fear[k].kind = (u8)ld_u8(); s->fear[k].arg = (u8)ld_u8(); s->fear[k].pts = (u16)ld_u16(); s->fear[k].age = (u16)ld_u16(); }
        s->asp_pts = ld_u16(); ld_n(s->rel, save_ver >= 22 ? MAX_SIMS : 6); s->lvl = (u8)ld_u8();
        for (int k = 0; k < 6; k++) { s->ml[k].id = (u8)ld_u8(); s->ml[k].val = (s8)ld_u8(); s->ml[k].t = (u16)ld_u16(); }
        s->uid = (u8)ld_u8(); ld_n(s->par, 2); s->partner = (u8)ld_u8(); s->age_days = (u8)ld_u8(); s->bday = (u8)ld_u8(); s->away = (u8)ld_u8(); s->grade = (u8)ld_u8(); s->homework = (u8)ld_u8();
        s->action = ld_u8(); s->act_t = ld_u16(); s->act_x = (s8)ld_u8(); s->act_y = (s8)ld_u8(); s->act_var = (u8)ld_u8();
        if (s->action >= ACT_GATHER + 1) s->action = ACT_NONE;
        s->gait.sit = (s->action == ACT_SIT || s->action == ACT_WC); s->gait.sleep = (s->action == ACT_SLEEP);
    }
    player = CLAMP(pl, 0, MAX(0, nsims - 1));
}
void sim_proud(Sim *s) { sim_moodlet(s, ML_PROUD); }
