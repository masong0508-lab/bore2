// sim.c - the household and the life: BORE's needs, clock, bills and job, with free 3D movement and a path finder for the others.
#include "sim.h"
#include "wants.h"
#include "world.h"
#include "items.h"
#include "audio.h"
#include "gfx.h"
#include "places.h"
#include "bigmap.h"
#include "crowd.h"
#include "pop.h"
#include "home.h"
#include "island.h"

Sim  sims[MAX_SIMS];
int  nsims, player;
int  clock_min = 8 * 60, clock_day = 0;
long cash = 15000;
int  job_level = 0, job_worked = 0, job_days_good = 0, job_strikes = 0;
int  sim_speed = 1;
int  bills_due = 800;
char last_note[32]; int last_note_t;
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
static const u8 stLen[4][AG_N] = { { 0, 0, 0, 0, 0 }, { 1, 3, 3, 9, 5 }, { 2, 6, 6, 18, 9 }, { 4, 12, 12, 36, 18 } };
int sim_stage_len(int stage) { return stLen[aging & 3][stage % AG_N]; }
enum { CARE_FEED = 1, CARE_CHANGE, CARE_PLAY, CARE_CUDDLE, CARE_BED };
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
const char *sim_need_name(int n) { return needNm[n]; }
int sim_need_val(const Sim *s, int n) { return s->need[n] >> 10; }
int sim_tile_x(const Sim *s) { return s->x / TILE; }
int sim_tile_y(const Sim *s) { return (-s->z) / TILE; }
int sim_awake(const Sim *s) { return s->action != ACT_SLEEP; }
char note_q[4][32]; int note_qn;   // the notes the feed has not shown yet (main takes them every frame)
void sim_note(const char *s) {
    strncpy(last_note, s, 31); last_note[31] = 0; last_note_t = 180;
    if (note_qn == 4) { memmove(note_q[0], note_q[1], 3 * 32); note_qn = 3; }
    strcpy(note_q[note_qn++], last_note);
}
static void sim_say(Sim *s, const char *t) { strncpy(s->note, t, 23); s->note[23] = 0; s->note_t = 150; }

// ---- integer atan2 (4096 = a turn) ----
// ================================================================== MOODLETS (Sims 3) and TRAITS
enum { ML_NONE, ML_GOOD_MEAL, ML_SNACKED, ML_RESTED, ML_FRESH, ML_GOOD_CHAT, ML_FRIEND, ML_PROMOTED, ML_PAYDAY, ML_EMBARRASSED,
       ML_PASSED_OUT, ML_WET, ML_REJECTED, ML_TEASED, ML_HUGGED, ML_FUN_TIME, ML_GOT_A_LAUGH, ML_FLIRTY, ML_WORKOUT, ML_CAFFEINE,
       ML_HUNGRY, ML_TIRED, ML_STINKY, ML_BURSTING, ML_LONELY, ML_BORED, ML_SINGING, ML_PIZZA,
       ML_ATE_OUT, ML_SWAM, ML_OUTFIT, ML_RETAIL, ML_NEW_GAME, ML_NEW_JOB, ML_COACHED, ML_HOTEL,
       ML_FINE_DINING, ML_FILM, ML_CUT, ML_NIGHT_OUT, ML_ROUND, ML_PAMPERED, ML_INSPIRED, ML_HEALTHY, ML_TREAT,
       ML_BIRTHDAY, ML_NEW_BABY, ML_LOVE, ML_PROUD, ML_GRIEF, ML_MEMORIES, ML_CAKE, ML_GRADES, ML_HOMEWORK, ML_SCHOOL, ML_N };
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
    nsims = 0; player = 0; clock_min = 8 * 60; clock_day = 0; day_count = 0; cash = 15000; last_note[0] = 0; note_qn = 0;
    job_level = 0; job_car = CAR_FACTORY; job_take(CAR_FACTORY, -1, -1, "THE FACTORY");
    groceries = 10; room_bx = room_by = -1; room_until = 0;
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
static int footprint_free(int x, int z) {
    for (int k = 0; k < 4; k++) {
        int px = x + ((k & 1) ? SIM_RADIUS : -SIM_RADIUS), pz = z + ((k & 2) ? SIM_RADIUS : -SIM_RADIUS);
        int mx = px / TILE, my = (-pz) / TILE;
        if (px < 0 || pz > 0) return 0;
        if (!world_walkable(mx, my)) return 0;
    }
    return 1;
}
static int walk(Sim *s, int dx, int dz) {   // returns 1 if it moved at all
    int moved = 0;
    if (dx && footprint_free(s->x + dx, s->z)) { s->x += dx; moved = 1; }
    if (dz && footprint_free(s->x, s->z + dz)) { s->z += dz; moved = 1; }
    return moved;
}
static int speed_of(const Sim *s) {
    int sp = WALK_SPEED * stSpd[s->bore.stage % 5] / 100;
    if ((s->need[N_REST] >> 10) < 20) sp = sp * 80 / 100;
    return MAX(sp, 3);
}

// ---- path finding: BFS over the tiles, then pulled straight (a person cuts corners, they do not walk a staircase) ----
u16 bfs_q[MW * MH];                 // (also lent to the roof rebuild in world.c: never at the same time)
s16 bfs_par[MH][MW];
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
    for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) bfs_par[y][x] = -1;
    int qh = 0, qt = 0;
    bfs_q[qt++] = (u16)(sy * MW + sx); bfs_par[sy][sx] = (s16)(sy * MW + sx);
    static const signed char dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
    int found = 0;
    while (qh < qt && !found) {
        int c = bfs_q[qh++], cx = c % MW, cy = c / MW;
        for (int d = 0; d < 4; d++) {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || ny < 0 || nx >= MW || ny >= MH || bfs_par[ny][nx] >= 0) continue;
            if (!(nx == tx && ny == ty) && !world_walkable(nx, ny)) continue;
            bfs_par[ny][nx] = (s16)c;
            if (nx == tx && ny == ty) { found = 1; break; }
            bfs_q[qt++] = (u16)(ny * MW + nx);
        }
    }
    if (!found) return 0;
    // walk the parents back, then reverse into the path
    static u8 rx[640], ry[640]; int n = 0;
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
    s->gait.amp = CLAMP(1800 + s->spd * 3 / 2, 1800, 4096);                                // slow strolls swing less
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
        case '<': return ACT_SLEEP;
        case '?': return ACT_READ;
        case ']': case '_': return ACT_RUN;
        case 'v': case 'R': case 'A': case 'Q': case 'q': case 'G': case 'V': return ACT_FUN;
        case 'b': case 'd': return ACT_READ; case 'c': return ACT_COFFEE; case 'm': return ACT_RUN; case 'I': return ACT_PHONE; case 'x': return ACT_WORK;
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
        case '!': return "PLAY ARCADE"; case '$': return "PLAY PIANO"; case '(': return "PLAY GUITAR"; case ')': return "PAINT";
        case '>': return "SOAK"; case '[': return "USE COMPUTER"; case 'j': return "PICK A SONG"; case 'n': return "PLAY"; case '{': return "PLAY HOUSE";
        case '}': return "MAKE A WISH"; case '`': return "PRACTICE SPEECH"; case ',': return "TAKE A BATH"; case '-': return "WASH HANDS";
        case '/': return "COOK"; case ':': return "MICROWAVE"; case ';': return "SIT"; case '<': return "SLEEP"; case '?': return "PLAY CHESS"; case '~': return "SUNBATHE";
        case ']': return "LIFT WEIGHTS"; case '_': return "SHOOT HOOPS";
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
    int was = s->rel[him], now = was + 8 + bonus; if (now > 100) now = 100;
    s->rel[him] = (s8)now; if (o->rel[me] < now) o->rel[me] = (s8)now;
    wants_event(s, W_CHAT, 0); wants_event(s, W_CHAT_WITH, him); sim_moodlet(s, ML_GOOD_CHAT);
    if (was < 50 && now >= 50) { wants_event(s, W_FRIEND, him); if (me == player) { sim_note("YOU MADE A FRIEND"); } sim_moodlet(s, ML_FRIEND); sim_moodlet(o, ML_FRIEND); }
}
static void end_action(Sim *s) {
    switch (s->action) {   // what just finished: the wants are watching
        case ACT_EAT: if (s->act_var == 8) { bake_done(s); break; }   // (act_var: 0 a snack, 1 cooked with good food, 2 eaten out, 3 cooked plain, 4 a treat, 5 fine dining,
                      wants_event(s, W_EAT, 0);                   //  8 baking the birthday cake, 9 a slice of it)
                      if (s->act_var == 9) { sim_moodlet(s, ML_CAKE); cake_eaten(s); break; }
                      sim_moodlet(s, s->act_var == 5 ? ML_FINE_DINING : s->act_var == 2 ? ML_ATE_OUT : s->act_var == 1 ? ML_GOOD_MEAL : s->act_var == 4 ? ML_TREAT : ML_SNACKED);
                      if (s->act_var == 2 || s->act_var == 5) wants_event(s, W_DINER, 0); break;
        case ACT_SLEEP: if (s->act_var == 2) { sim_moodlet(s, ML_PAMPERED); break; }   // (a massage)
                        wants_event(s, W_SLEEP, 0); if (s->act_t <= 0 && s->act_var == 0) { wants_reroll(s); sim_moodlet(s, ML_RESTED); }   // a full sleep: new wants on waking
                        { PlaceInfo pi; if (place_at(s->act_x, s->act_y, &pi) && (pi.lot == LOT_INN || pi.lot == LOT_HOTEL)) sim_moodlet(s, ML_HOTEL); } break;
        case ACT_SHOWER: wants_event(s, W_SHOWER, 0); sim_moodlet(s, ML_FRESH); if (s->act_var == 1) sim_moodlet(s, ML_SINGING); break;
        case ACT_FUN: wants_event(s, W_FUN, 0);   // (act_var: 4 browsing a shop, 5 a drink at a bar, 7 looking at art)
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
        case ACT_PHONE: wants_event(s, W_PHONE, 0); if (s->act_var == 1) sim_moodlet(s, ML_PIZZA); break;
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
                        if ((s->need[N_REST] >> 10) >= 100 && (!is_night() || clock_min < NIGHT_FROM)) { s->act_t = 0; sim_say(s, "RESTED"); } break;
        case ACT_SHOWER: s->need[N_CLEAN] += GAIN_WASH; if (s->act_var == 1) s->need[N_FUN] += 90; break;
        case ACT_SIT: s->need[N_COMFY] += GAIN_SIT; if (s->act_var == 6) { s->need[N_FUN] += GAIN_FUN; break; }   // (a film: to the end)
                      if ((s->need[N_COMFY] >> 10) >= 100) s->act_t = 0; break;
        case ACT_FUN: s->need[N_FUN] += GAIN_FUN; s->need[N_COMFY] += 30;
                      if (s->act_var == 5) { s->need[N_SOCIAL] += GAIN_TALK / 2; if (!(s->act_t & 63)) s->gait.wave = 20; }   // (a drink at the bar)
                      if (s->act_var == 7 && !(s->act_t & 7)) s->skill_fun++; break;
        case ACT_READ: s->need[N_FUN] += s->act_var == 1 ? GAIN_FUN / 4 : GAIN_FUN / 2; s->skill_fun += s->act_var == 1 ? 3 : 1; break;
        case ACT_COFFEE: s->need[N_REST] += s->act_var == 1 ? 320 : 180; s->need[N_WC] -= s->act_var == 1 ? 110 : 60; break;
        case ACT_RUN: s->need[N_FUN] += 120; s->need[N_CLEAN] -= s->act_var ? 160 : 90; s->skill_body += s->act_var == 2 ? 3 : s->act_var == 1 ? 2 : 1; s->gait.amp = 4096; s->gait.phase = (s->gait.phase + 200) & 4095; break;
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
        case ACT_PHONE: if (s->act_var == 1) s->need[N_FOOD] += 300; else s->need[N_SOCIAL] += GAIN_TALK; s->gait.wave = 2; break;
        case ACT_TALK: s->need[N_SOCIAL] += GAIN_TALK; s->need[N_FUN] += 40; if ((s->act_t & 63) == 0) s->gait.wave = 40; break;
        case ACT_WORK: if (is_work()) { s->need[N_FUN] -= 20; if (!(s->act_t & 63)) s->gait.wave = 24; } else { s->act_t = 0; } break;   // (the minutes count in clock_tick)
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
              || (ch == 'F' && c == 's') || (ch == 'T' && c == '&') || (ch == 'S' && c == 'z') || (ch == 'H' && c == '0');
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
static void ai_think(Sim *s) {
    if (s->action != ACT_NONE || s->path_i < s->path_n) return;
    if (s->bore.stage == AG_BABY) {   // a baby: asleep in its crib at night (or when tired), otherwise sat on the floor playing
        if (is_night() || (s->need[N_REST] >> 10) < 35) baby_to_bed(s);
        else { begin_action(s, ACT_SIT, sim_tile_x(s), sim_tile_y(s)); s->act_t = 240; }
        return;
    }
    int me = (int)(s - sims);
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
    int sleeping = s->action == ACT_SLEEP;
    s->need[N_FOOD] -= sleeping ? RATE_FOOD / 3 : RATE_FOOD;
    s->need[N_WC] -= sleeping ? RATE_WC / 3 : RATE_WC;
    if (!sleeping) s->need[N_REST] -= is_night() ? RATE_REST * 3 / 2 : RATE_REST;
    s->need[N_CLEAN] -= RATE_CLEAN;
    if (s->action != ACT_SIT && s->action != ACT_SLEEP) s->need[N_COMFY] -= RATE_COMFY;
    if (s->action != ACT_FUN && s->action != ACT_READ) s->need[N_FUN] -= RATE_FUN;
    if (s->action != ACT_TALK && s->action != ACT_PHONE) s->need[N_SOCIAL] -= RATE_SOCIAL;
    int target = world_room_score(sim_tile_x(s), sim_tile_y(s)) << 10;
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
    // the thought bubble: the most urgent need
    int worst = -1, wv = 101;
    for (int n = 0; n < NEED_N; n++) { int v = s->need[n] >> 10; if (v < wv) { wv = v; worst = n; } }
    s->think = wv < 20 ? worst : -1;
    if (s->think >= 0 && (int)(s - sims) == player && (s->think_t++ % 900) == 0) {
        audio_voice(s->think == N_FOOD ? VOICE_HUNGRY : s->think == N_WC ? VOICE_PEE : VOICE_NEED);
    }
    if (s->bore.stage == AG_BABY) {   // a baby: it cannot see to itself; it cries, and the grown-ups come
        int me = (int)(s - sims);
        if ((s->need[N_WC] >> 10) == 0) { s->need[N_WC] = 100 << 10; s->need[N_CLEAN] = 0; }
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
static void care_done(Sim *s) {
    int bi = s->talk_to; if (bi < 0 || bi >= nsims) return;
    Sim *b = &sims[bi]; if (b->bore.stage != AG_BABY) return;
    switch (s->act_var) {
        case CARE_FEED: b->need[N_FOOD] += 60 << 10; sim_say(b, "NOM NOM"); break;
        case CARE_CHANGE: b->need[N_WC] = 100 << 10; b->need[N_CLEAN] += 70 << 10; sim_say(b, "AHH"); break;
        case CARE_PLAY: b->need[N_FUN] += 50 << 10; s->need[N_FUN] += 10 << 10; sim_say(b, "HEE HEE!"); break;
        case CARE_CUDDLE: b->need[N_SOCIAL] += 50 << 10; b->need[N_COMFY] += 20 << 10; s->need[N_SOCIAL] += 15 << 10; sim_say(b, "GOO GOO"); break;
        case CARE_BED: baby_to_bed(b); break;
    }
    for (int i = 0; i < NEED_N; i++) { b->need[i] = CLAMP(b->need[i], 0, 100 << 10); s->need[i] = CLAMP(s->need[i], 0, 100 << 10); }
    if (s->uid && (b->par[0] == s->uid || b->par[1] == s->uid)) sim_moodlet(s, ML_PROUD);
}
static void homework_done(Sim *s) {
    if (s->homework) return;
    s->homework = 1; s->grade = (u8)MIN(100, s->grade + 8);
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
    for (int j = 0; j < nsims; j++) if (j != i) sim_moodlet(&sims[j], ML_GRIEF);
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
    if (nsims >= MAX_SIMS) { sim_note("NO ROOM: 6 BORES IS THE MOST"); return; }
    Sim *ma = by_uid(baby_par[0]), *pa = by_uid(baby_par[1]);
    Bore b; look_default(&b); b.stage = baby_kind == 2 ? AG_CHILD : AG_BABY; look_random(&b);
    if (!baby_kind) genes(&b, ma ? &ma->bore : 0, pa ? &pa->bore : 0);
    strcpy(b.last, (ma ? ma : pa ? pa : &sims[player])->bore.last);
    b.asp = AS_GROW; look_fix(&b); look_build(&b);
    const Sim *nr = ma ? ma : pa ? pa : &sims[player];
    int tx = sim_tile_x(nr), ty = sim_tile_y(nr);
    { int x, y; if (free_floor_near(nr->lvl, tx, ty, &x, &y)) { tx = x; ty = y; } }
    int keep = world_lvl; world_lvl = nr->lvl;
    sim_add(&b, tx, ty, 1);
    world_lvl = keep;
    Sim *n = &sims[nsims - 1]; n->par[0] = baby_par[0]; n->par[1] = baby_par[1]; n->lvl = nr->lvl;
    if (b.stage == AG_BABY && home_is_here() && n->lvl == 0) {   // a crib comes with a baby (when the house has none)
        int have = 0, cx, cy;
        for (int y = 0; y < MH && !have; y++) for (int x = 0; x < MW; x++) if (lifeMap[y][x] == CH_CRIB) { have = 1; break; }
        if (!have && free_floor_near(0, tx, ty, &cx, &cy)) { lifeMap[cy][cx] = CH_CRIB; world_touched(); sim_note("A CRIB CAME WITH THE BABY"); }
    }
    char t[48];
    if (baby_kind) snprintf(t, sizeof t, "WELCOME HOME, %s!", b.first); else snprintf(t, sizeof t, "THE BABY IS HERE! MEET %s", b.first);
    sim_note(t);
    for (int j = 0; j < nsims - 1; j++) if (sims[j].uid && (sims[j].uid == baby_par[0] || sims[j].uid == baby_par[1])) { sim_moodlet(&sims[j], ML_NEW_BABY); wants_event(&sims[j], W_BABY, 0); }
    sim_cheer = -nsims;                                                   // (negative: a newcomer, not a birthday)
    audio_voice(VOICE_YAHOO);
}
static void start_family(Sim *s, Sim *o) {   // two grown-ups who get on: a baby on the way
    int who = (int)(o - sims), me = (int)(s - sims);
    int chance = 30 + s->rel[who] + o->mood / 4;
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
static void adopt(Sim *s) {
    if (baby_due >= 0) { sim_note("A BABY IS ALREADY ON THE WAY"); return; }
    if (nsims >= MAX_SIMS) { sim_note("THE HOUSE IS FULL (6 BORES)"); return; }
    if (cash < 1000) { sim_note("ADOPTING IS $1000"); audio_voice(VOICE_NAH); return; }
    cash -= 1000;
    baby_kind = (u8)(1 + (look_rand() & 1)); baby_par[0] = s->uid; baby_par[1] = s->partner; baby_due = life_minutes() + 1440;
    sim_note(baby_kind == 1 ? "ADOPTING A BABY: HOME TOMORROW" : "ADOPTING A CHILD: HOME TOMORROW");
    audio_voice(VOICE_YAHOO);
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
static void midnight_ages(void) {   // a day older: birthdays come round, and at the very end of a long life, goodbye
    if (aging) for (int i = nsims - 1; i >= 0; i--) {   // (backwards: someone may leave)
        Sim *s = &sims[i]; int L = sim_stage_len(s->bore.stage);
        if (s->bday) { age_up(i, 0); continue; }                                    // nobody baked: they grow up quietly overnight
        if (s->age_days < 255) s->age_days++;
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

static void clock_tick(void) {
    static int acc = 0;
    int spm = STEPS_MIN;
    if ((sims[player].action == ACT_SLEEP && sims[player].act_var != 2) || sims[player].action == ACT_WORK) spm = 1;   // asleep or at work: the hours fly (not on a massage table)
    if (sim_speed == 2) spm /= 3; if (sim_speed == 3) spm = 1;
    if (spm < 1) spm = 1;
    if (++acc < spm) return;
    acc = 0;
    clock_min++;
    for (int i = 0; i < nsims; i++) { wants_minute(&sims[i]); moodlets_minute(&sims[i]); }
    if (clock_min == 16 * 60) for (int i = 0; i < nsims; i++) for (int j = 0; j < nsims; j++) { int r = sims[i].rel[j]; sims[i].rel[j] = (s8)(r > 0 ? r - 2 : r < 0 ? r + 2 : 0); }   // 4 PM: the day's warmth fades a little
    if (sims[player].action == ACT_WORK && is_work()) job_worked++;       // minutes worked this shift
    if (clock_day < 5 && day_count > 0) {   // school: the bus at 8, home at 3 (not on the day you move in)
        if (clock_min == 465) { for (int i = 0; i < nsims; i++) if (school_age(&sims[i])) { sim_note("THE SCHOOL BUS COMES AT 8"); break; } }
        if (clock_min == 480) school_go();
        if (clock_min == 900) school_back();
    }
    if (baby_due >= 0 && life_minutes() >= baby_due) born();
    {   // the shift: a reminder an hour before, the start, and the pay at the end (by the minutes worked)
        const Career *cr = &careers[job_car]; int on = (cr->days >> clock_day) & 1, from = cr->from * 60, to = cr->to * 60;
        char b[40], h[8];
        if (on && clock_min == from - 60) { sprintf(b, "WORK AT %s: START, JOBS", hour_name(cr->from, h)); sim_note(b); }
        if (on && clock_min == from) { snprintf(b, sizeof b, "WORK NOW: %s", job_place); sim_note(b); job_worked = 0; }
        if (on && clock_min == to) {
            int len = to - from, w = MIN(job_worked, len), full = job_pay(), pay = 0;
            if (w >= len * 55 / 100) { pay = full * w / len; if (w >= len * 95 / 100) pay += full / 5; job_days_good++; }
            else if (w >= len / 4) pay = full * w / len;
            else job_strikes++;
            if (pay) { cash += pay; story_paid++; sprintf(b, "PAID $%d", pay); sim_note(b); wants_event(&sims[player], W_PAID, 0); sim_moodlet(&sims[player], ML_PAYDAY);
                       if (story_paid == 1) sim_note("THE ISLAND FERRY IS OPEN NOW!"); audio_voice(VOICE_YAHOO); }
            else { sim_note("NO PAY TODAY"); wants_fear(&sims[player], F_BADDAY); }
            if (job_days_good >= 3 && job_level < 5) { job_level++; job_days_good = 0; job_strikes = 0; snprintf(b, sizeof b, "PROMOTED: %s!", job_title()); sim_note(b);
                                                       wants_event(&sims[player], W_PROMO, 0); sim_moodlet(&sims[player], ML_PROMOTED); }
            if (job_strikes >= 3 && job_level > 0) { job_level--; job_strikes = 0; sim_note("DEMOTED"); wants_fear(&sims[player], F_DEMOTED); }
        }
    }
    if (clock_min >= 1440) {
        clock_min = 0; clock_day = (clock_day + 1) % 7; day_count++;
        midnight_ages(); pop_midnight();                                    // (your household, and everyone else)
        cash -= BILLS;
        if (cash < 0) { cash = 0; sim_note("BROKE! BILLS UNPAID"); sims[player].mood -= 30; audio_voice(VOICE_CRY); wants_fear(&sims[player], F_BROKE); }
        else sim_note("BILLS PAID: 800");
    }
}

void sim_step(void) {
    walk_tick++;
    clock_tick();
    for (int i = 0; i < nsims; i++) {
        Sim *s = &sims[i];
        if (s->away) continue;                                        // (at school: back at 3)
        world_lvl = s->lvl;
        tick_needs(s);
        if (s->note_t > 0) s->note_t--;
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

// ---- the player ----
void sim_player_move(int dx, int dz, int run) {
    Sim *s = &sims[player];
    world_lvl = s->lvl;
    if (s->action != ACT_NONE && s->action != ACT_WALK) {
        if (!dx && !dz) return;                                              // standing still: whatever it is doing goes on
        if (s->action == ACT_WORK || s->action == ACT_SLEEP) return;         // (asleep, at work: the stick does not get you up; X or O does)
        end_action(s);                                                       // walking off: it stops
    }
    s->path_n = 0; s->action = ACT_NONE;
    if (!dx && !dz) { s->gait.amp = s->gait.amp > 300 ? s->gait.amp - 300 : 0; return; }
    // REAL 3D WALKING: the Bore turns toward the push first and only ever moves the way it FACES -
    // far from the push direction it pivots on the spot, so it never slides sideways like a 2D sprite.
    int want = yaw_towards(dx, dz);
    int diff = ((want - s->yaw + 2048) & 4095) - 2048;                       // how far round it still has to turn
    turn_to(s, want, 260 + (ABS(diff) >> 3));                                // the wider the turn, the faster it swings
    int mag = SquareRoot0(dx * dx + dz * dz); if (mag > 4096) mag = 4096;    // stick tilt = walk speed
    int align = 4096 - ABS(diff) * 2; if (align < 0) align = 0;              // 4096 facing it .. 0 facing away
    int sp0 = speed_of(s); if (run) sp0 = sp0 * 3 / 2;
    int sp = ((sp0 * mag) >> 12) * align >> 12;
    if (sp) walk(s, (isin(s->yaw) * sp) >> 12, -((icos(s->yaw) * sp) >> 12));
    s->gait.amp = MAX(1600, (mag * align) >> 12);                            // pivoting: small steps; under way: the full swing
    s->gait.phase = (s->gait.phase + (((run ? 200 : 150) * MAX(mag, 2048)) >> 12)) & 4095;
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
    if (c == '8') return world_zone == ZONE_CITY;      // a FOR SALE sign
    if (c == CH_RAFT) return 1;
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
    PlaceInfo pi;
    if (place_at(mx, my, &pi)) {
        if (!place_open(pi.lot, clock_min, clock_day)) return "CLOSED";
        if (staff_only(pi.lot, c)) return "STAFF ONLY";
        if (c == 'y' || c == 'I') switch (pi.lot) {
            case LOT_DINER: case LOT_CAFE: return "ORDER"; case LOT_SHOP: return "SHOP"; case LOT_INN: case LOT_HOTEL: return "CHECK IN";
            case LOT_BAR: return "A DRINK"; case LOT_CINEMA: return "POPCORN"; case LOT_SALON: return "A HAIRCUT"; case LOT_RESTAURANT: return "DINE";
            case LOT_CLINIC: return "A CHECKUP"; case LOT_GALLERY: return "BUY ART"; default: return "STAFF ONLY"; }
        if (c == ';' && pi.lot == LOT_CINEMA) return "A FILM";
        if (c == 't' && pi.lot == LOT_RESTAURANT) return "DINE";
        if (c == 'S' && pi.lot == LOT_SPA) return "A MASSAGE";
        if (c == '>' && pi.lot == LOT_SPA) return "SOAK";
        if (c == ')' && pi.lot == LOT_GALLERY) return "ADMIRE";
        if (c == '!' && pi.lot == LOT_ARCADE) return "PLAY $2";
        if (c == '7' || c == '`' || c == 'i') return pi.lot == LOT_SHOP ? "SHOP" : "";
        if (c == 'd') return "STAFF ONLY";
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
       PC_LATTE, PC_CAKE, PC_DRINK, PC_ROUND, PC_FILM, PC_POPCORN, PC_HAIR, PC_LOOK, PC_DINE, PC_CHECKUP, PC_MASSAGE, PC_SOAK, PC_ADMIRE, PC_PAINTING };
static int room_here(void) { return room_bx == pie_pi.bx && room_by == pie_pi.by && life_minutes() < room_until; }
static char pie_title[24];
const char *sim_pie_title(void) { return pie_title; }
static int sim_pie_open_(char opt[8][16]) {
    Sim *s = &sims[player];
    pie_n = 0;
    if (s->action != ACT_NONE && s->action != ACT_WALK) return 0;        // busy: X just stops (sim_player_interact)
    int mx, my, who;
    if (!target_of(s, &mx, &my, &who)) return 0;
    pie_who = who; pie_mx = mx; pie_my = my;
    if (who >= 0) {
        Sim *o = &sims[who];
        strcpy(pie_title, o->bore.first);
        #define SOPT(txt, v) { strcpy(opt[pie_n], txt); pie_code[pie_n++] = (u8)(v); }
        if (o->bore.stage == AG_BABY) {   // a baby: looking after it (the little ones can play and cuddle)
            int big = s->bore.stage >= AG_TEEN;
            if (big) { SOPT("FEED", 20 + CARE_FEED) SOPT("CHANGE", 20 + CARE_CHANGE) }
            SOPT("PLAY", 20 + CARE_PLAY) SOPT("CUDDLE", 20 + CARE_CUDDLE)
            if (big) SOPT("PUT TO BED", 20 + CARE_BED)
            return pie_n;
        }
        int adults = s->bore.stage >= AG_TEEN && o->bore.stage >= AG_TEEN;
        for (int k = 0; k < SO_N; k++) {
            if (k == SO_HUG && s->rel[who] < 25) continue;                  // you hug friends
            if (k == SO_FLIRT && (!adults || s->rel[who] < 35)) continue;   // flirting needs grown-ups who get on
            if (k == SO_TEASE && s->mood > 40 && !has_trait(s, TR_NICE, 0)) continue;   // a good mood doesn't think of teasing (MEAN always does)
            SOPT(soNm[k], k)
        }
        if (s->bore.stage == AG_ADULT && o->bore.stage == AG_ADULT && s->rel[who] >= 50 && baby_due < 0 && nsims < MAX_SIMS && pie_n < 8)
            SOPT("START A FAMILY", 15)
        #undef SOPT
        return pie_n;
    }
    char c = world_at(mx, my); int act = action_for(c);
    if (c == ' ') { strcpy(pie_title, "STAIRS"); strcpy(opt[0], "GO DOWN"); pie_code[0] = 1; pie_n = 1; return 1; }
    if (c == '8' && world_zone == ZONE_CITY && world_wbx >= 0) {   // a home for sale: buy it, and the family moves in
        int bx = world_wbx + mx / BM_BLK, by = world_wby + my / BM_BLK;
        if (!home_for_sale(bx, by)) { sim_note("SOLD"); return 0; }
        home_address(bx, by, pie_title);
        sprintf(opt[0], "BUY $%d", home_price(bx, by)); pie_code[0] = 36;
        strcpy(opt[1], "WHAT IS IT?"); pie_code[1] = 37; pie_n = 2; return 2;
    }
    pie_place = place_at(mx, my, &pie_pi);
    int mine = job_here(mx, my, c);
    if (mine && !is_work()) { if (c == 'x') { char b[40], h1[8], h2[8]; sprintf(b, "WORK IS %s TO %s", hour_name(careers[job_car].from, h1), hour_name(careers[job_car].to, h2)); sim_note(b); return 0; } mine = 0; }
    if (c == 'x' && !mine) { sim_note("THIS IS NOT YOUR WORKPLACE"); return 0; }
    if (pie_place && !mine) {
        if (!place_open(pie_pi.lot, clock_min, clock_day)) { char b[40], h[8]; sprintf(b, "CLOSED. OPENS AT %s", hour_name(place_opens_at(pie_pi.lot), h)); sim_note(b); return 0; }
        if (staff_only(pie_pi.lot, c)) { sim_note("STAFF ONLY"); return 0; }
    }
    strcpy(pie_title, pie_place ? pie_pi.name : act_hint(act, c));
    #define OPT(txt, v) { strcpy(opt[pie_n], txt); pie_code[pie_n++] = v; }
    if (mine) OPT("WORK", PC_WORK)
    if (c == 'x') act = ACT_NONE;                                          // (the marker is only for working)
    if (pie_place && (pie_pi.lot == LOT_INN || pie_pi.lot == LOT_HOTEL) && act == ACT_SLEEP && !room_here()) {   // a bed at an inn: yours once you have paid for the room
        if (!mine) { sim_note("RENT A ROOM AT THE FRONT DESK"); return 0; }
        act = ACT_NONE;
    }
    if (pie_place && (c == 'y' || c == 'I' || c == 'x')) act = ACT_NONE;   // (the till, the desk bell: what the place does, not a phone)
    if (pie_place && pie_pi.lot == LOT_ARCADE && c == '!') { OPT("PLAY $2", PC_PLAY) act = ACT_NONE; }
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
        OPT("CRAFT", 38) act = ACT_NONE;
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
        case ACT_PHONE: OPT("CALL A FRIEND", 0) OPT("ORDER PIZZA", 1) OPT("THROW A PARTY", 2) if (s->bore.stage == AG_ADULT || s->bore.stage == AG_ELDER) OPT("ADOPT $1000", 35) break;
        case ACT_STAIRS: OPT("GO UP", 0) break;
        case ACT_FUN: if (c == 'R' || c == 'A' || c == 'j') {   // a radio: the stations
                          for (int k = 0; k < audio_station_count(); k++) OPT(audio_station_name(k), 10 + k)
                          OPT("TURN IT OFF", 9) break; }
                      OPT(act_hint(act, c), 0) break;
        default: OPT(act_hint(act, c), 0) break;
    }
    if (pie_place && !mine) switch (pie_pi.lot) {   // what the place sells
        case LOT_DINER: if (c == 'y') { OPT("A MEAL $15", PC_MEAL) OPT("A SNACK $6", PC_SNACK) OPT("A COFFEE $3", PC_COFFEE) } else if (c == 'c') OPT("A COFFEE $3", PC_COFFEE) break;
        case LOT_SHOP: { static const char *const buy[5] = { "GROCERIES $60", "AN OUTFIT $150", "A GAME $60", 0, "A RECORD $25" };
                         int k = place_shop_kind(pie_pi.h); if (buy[k] && pie_n < 7) OPT(buy[k], PC_BUY0 + k)
                         if (pie_n < 8) OPT("BROWSE", PC_BROWSE) } break;
        case LOT_GYM: if ((c == ']' || c == 'm' || c == '_') && pie_n < 8) OPT("A TRAINER $20", PC_TRAINER) break;
        case LOT_POOL: if ((c == '~' || c == '>') && pie_n < 8) OPT("SWIM", PC_SWIM) break;
        case LOT_INN: case LOT_HOTEL: if (c == 'y' || c == 'I') OPT(room_here() ? "MY ROOM: PAID" : "A ROOM $60", PC_ROOM) break;
        case LOT_CAFE: if (c == 'y' || c == 'c') { OPT("A LATTE $5", PC_LATTE) if (c == 'y') OPT("A CAKE $6", PC_CAKE) } break;
        case LOT_BAR: if (c == 'y') { OPT("A DRINK $8", PC_DRINK) OPT("A ROUND $40", PC_ROUND) } break;
        case LOT_CINEMA: if (c == ';' && pie_n < 8) OPT("A FILM $12", PC_FILM) else if (c == 'y' || c == ':') OPT("POPCORN $4", PC_POPCORN) break;
        case LOT_SALON: if ((c == 'y' || c == '`' || c == 'h') && pie_n < 7) { OPT("A HAIRCUT $40", PC_HAIR) OPT("A NEW LOOK $120", PC_LOOK) } break;
        case LOT_RESTAURANT: if ((c == 'y' || c == 't' || c == 'h') && pie_n < 8) OPT("FINE DINING $40", PC_DINE) break;
        case LOT_CLINIC: if (c == 'y') OPT("A CHECKUP $30", PC_CHECKUP) break;
        case LOT_SPA: if (c == 'S') OPT("A MASSAGE $50", PC_MASSAGE) else if (c == '>') OPT("SOAK $15", PC_SOAK) break;
        case LOT_GALLERY: if (c == ')') OPT("ADMIRE THE ART", PC_ADMIRE) else if (c == 'y') OPT("A PAINTING $300", PC_PAINTING) break;
    }
    #undef OPT
    if (!pie_n) { sim_note(pie_place ? "STAFF ONLY" : "NOTHING TO DO HERE"); return 0; }
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
    if (bx < 0) { sim_note("NO WATER HERE"); return; }
    begin_action(s, ACT_SWIM, bx, by);
    s->x = bx * TILE + TILE / 2; s->z = -(by * TILE + TILE / 2); s->gait.sleep = 1;
    sim_say(s, "CANNONBALL!"); audio_voice(VOICE_YAHOO);
}
static void place_do(Sim *s, int code) {
    switch (code) {
        case PC_WORK: begin_action(s, ACT_WORK, pie_mx, pie_my); { char b[40]; snprintf(b, sizeof b, "AT WORK: %s", job_title()); sim_note(b); } break;
        case PC_MEAL: if (!pay(15)) return; begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 2; s->act_t = 260; sim_say(s, "THE SPECIAL, PLEASE"); break;
        case PC_SNACK: if (!pay(6)) return; begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 0; break;
        case PC_COFFEE: if (!pay(3)) return; begin_action(s, ACT_COFFEE, pie_mx, pie_my); s->act_var = 1; break;
        case PC_BUY0: if (!pay(60)) return; groceries += 7; shopped(s, "GROCERIES: 7 GOOD MEALS AT HOME", ML_RETAIL); break;
        case PC_BUY0 + 1: if (!pay(150)) return; new_look(&s->bore, 3); crowd_forget(CROWD_MAX + player); shopped(s, "A NEW OUTFIT!", ML_OUTFIT); break;
        case PC_BUY0 + 2: if (!pay(60)) return; s->need[N_FUN] = MIN(s->need[N_FUN] + (30 << 10), 100 << 10); shopped(s, "A NEW GAME!", ML_NEW_GAME); break;
        case PC_BUY4: if (!pay(25)) return; s->need[N_FUN] = MIN(s->need[N_FUN] + (20 << 10), 100 << 10); shopped(s, "A NEW RECORD!", ML_RETAIL); break;
        case PC_BROWSE: begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_var = 4; s->act_t = 240; break;
        case PC_TRAINER: if (!pay(20)) return; begin_action(s, ACT_RUN, pie_mx, pie_my); s->act_var = 2; sim_say(s, "FEEL THE BURN"); break;
        case PC_SWIM: swim(s); break;
        case PC_PLAY: if (!pay(2)) return; begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_var = 0; break;
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
        case PC_ADMIRE: begin_action(s, ACT_FUN, pie_mx, pie_my); s->act_var = 7; s->act_t = 200; break;
        case PC_PAINTING: if (!pay(300)) return; shopped(s, "A PAINTING, JUST FOR YOU", ML_INSPIRED); break;
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
#if defined(AUTO_PLACES) || defined(AUTO_TOUR) || defined(AUTO_GROW) || defined(AUTO_HOME) || defined(AUTO_ISLE) || defined(AUTO_SCENE)
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
    job_take(car, bx, by, place);
    char b[40]; snprintf(b, sizeof b, "NEW JOB: %s", job_title()); sim_note(b);
    sim_moodlet(&sims[player], ML_NEW_JOB); audio_voice(VOICE_YAHOO);
}
static void sim_pie_do_(int i) {
    Sim *s = &sims[player];
    if (i < 0 || i >= pie_n) return;
    if (pie_who >= 0) {
        int code = pie_code[i];
        if (code > 20 && code <= 20 + CARE_BED) { care_start(s, pie_who, code - 20); return; }
        if (code == 15) { start_family(s, &sims[pie_who]); return; }
        social(s, &sims[pie_who], code); return;
    }
    switch (pie_code[i]) {   // the family's own
        case 30: candles(s); return;
        case 31: begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 9; s->act_t = 120; return;
        case 32: begin_action(s, ACT_READ, pie_mx, pie_my); s->act_var = 5; s->act_t = 200; return;
        case 33: begin_action(s, ACT_READ, pie_mx, pie_my); s->act_var = 3; s->act_t = 240; return;
        case 34: begin_action(s, ACT_EAT, pie_mx, pie_my); s->act_var = 8; s->act_t = 300; sim_say(s, "A CAKE FOR THE BIRTHDAY!"); return;
        case 35: adopt(s); return;
        case 36: home_buy(world_wbx + pie_mx / BM_BLK, world_wby + pie_my / BM_BLK); return;   // (main moves the family in)
        case 38: isl_craft_asked = 1; return;                                                  // (main opens the list of things to make)
        case 39: raft_sail(); return;
        case 37: if (world_zone != ZONE_CITY) return;
                 { BInfo b; bm_block(world_wbx + pie_mx / BM_BLK, world_wby + pie_my / BM_BLK, &b); char t[40];
                   snprintf(t, sizeof t, "FOR SALE: %s", home_kind(b.lot)); sim_note(t); sim_note(home_blurb(b.lot)); return; }
    }
    if (pie_code[i] >= 100) { begin_action(s, ACT_GATHER, pie_mx, pie_my); s->act_var = (u8)(pie_code[i] - 100); s->act_t = gather_len(s->act_var); return; }   // (the island)
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
        s->lvl = (u8)tl; s->x = tx * TILE + TILE / 2; s->z = -(ty * TILE + TILE / 2); s->yaw = up ? ((4096 - f * 1024 + 2048) & 4095) : ((4096 - f * 1024) & 4095);
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
        if (world_zone == ZONE_ISLAND) { } else if (groceries > 0) groceries--; else { s->act_var = 3; sim_note("NO GROCERIES: A PLAIN MEAL"); }
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
    if (c == 'x' && !(job_here(mx, my, c) && is_work())) { sim_note(job_here(mx, my, c) ? "NOT WORK TIME" : "THIS IS NOT YOUR WORKPLACE"); return 0; }
    if (action_for(c) == ACT_NONE) return 0;
    if (c == 'R' || c == 'A') audio_jukebox_next();
    if (action_for(c) == ACT_TRAVEL) { travel_asked = 1; return 1; }
    begin_action(s, action_for(c), mx, my);
    return 1;
}
void sim_player_cancel(void) { Sim *s = &sims[player]; if (s->action != ACT_NONE) end_action(s); }
void sim_player_cancel_sim(Sim *s) { if (s->action != ACT_NONE) end_action(s); }

const char * sim_hint(void) { world_lvl = sims[player].lvl; const char * r = sim_hint_(); world_lvl = 0; return r; }

int sim_pie_open(char opt[8][16]) { world_lvl = sims[player].lvl; int r = sim_pie_open_(opt); world_lvl = 0; return r; }

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
