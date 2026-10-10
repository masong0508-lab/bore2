// sim.c - the household and the life: BORE's needs, clock, bills and job, with free 3D movement and a path finder for the others.
#include "sim.h"
#include "wants.h"
#include "world.h"
#include "items.h"
#include "audio.h"

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
#define WORK_FROM 540
#define WORK_TO 1020
#define PAY0 1400
#define PAY_LVL 800
#define BILLS 800
#define SIM_RADIUS 44
#define WALK_SPEED 9      // units per step at 100%
static const u8 stSpd[5] = { 50, 80, 95, 100, 70 };
static const char *const dayNm[7] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
static const char *const needNm[NEED_N] = { "FOOD", "WC", "REST", "CLEAN", "COMFY", "FUN", "SOCIAL", "ROOM" };
static const char *const jobNm[6] = { "NEWBIE", "AMATEUR", "SPONSORED", "PRO", "TEAM RIDER", "LEGEND" };
const char *sim_day_name(int d) { return dayNm[d % 7]; }
const char *sim_need_name(int n) { return needNm[n]; }
int sim_need_val(const Sim *s, int n) { return s->need[n] >> 10; }
int sim_tile_x(const Sim *s) { return s->x / TILE; }
int sim_tile_y(const Sim *s) { return (-s->z) / TILE; }
int sim_awake(const Sim *s) { return s->action != ACT_SLEEP; }
void sim_note(const char *s) { strncpy(last_note, s, 31); last_note[31] = 0; last_note_t = 180; }
static void sim_say(Sim *s, const char *t) { strncpy(s->note, t, 23); s->note[23] = 0; s->note_t = 150; }

// ---- integer atan2 (4096 = a turn) ----
// ================================================================== MOODLETS (Sims 3) and TRAITS
enum { ML_NONE, ML_GOOD_MEAL, ML_SNACKED, ML_RESTED, ML_FRESH, ML_GOOD_CHAT, ML_FRIEND, ML_PROMOTED, ML_PAYDAY, ML_EMBARRASSED,
       ML_PASSED_OUT, ML_WET, ML_REJECTED, ML_TEASED, ML_HUGGED, ML_FUN_TIME, ML_GOT_A_LAUGH, ML_FLIRTY, ML_WORKOUT, ML_CAFFEINE,
       ML_HUNGRY, ML_TIRED, ML_STINKY, ML_BURSTING, ML_LONELY, ML_BORED, ML_SINGING, ML_PIZZA, ML_N };
static const struct { const char *nm; s8 val; u16 mins; } mlDef[ML_N] = {
    { "", 0, 0 }, { "GOOD MEAL", 12, 240 }, { "SNACKED", 4, 90 }, { "WELL RESTED", 12, 300 }, { "FRESH", 8, 180 }, { "GOOD CHAT", 10, 180 },
    { "MADE A FRIEND", 20, 480 }, { "PROMOTED", 25, 720 }, { "PAYDAY", 15, 360 }, { "EMBARRASSED", -12, 120 },
    { "PASSED OUT", -15, 240 }, { "WET MYSELF", -25, 180 }, { "TURNED DOWN", -10, 180 }, { "TEASED", -15, 180 }, { "HUGGED", 12, 180 },
    { "FUN TIME", 8, 120 }, { "GOT A LAUGH", 10, 120 }, { "FLIRTY", 14, 180 }, { "WORKED OUT", 10, 180 }, { "CAFFEINATED", 6, 90 },
    { "HUNGRY", -12, 0 }, { "TIRED", -10, 0 }, { "STINKY", -15, 0 }, { "BURSTING", -15, 0 }, { "LONELY", -12, 0 }, { "BORED", -8, 0 },
    { "SANG IT OUT", 9, 120 }, { "PIZZA NIGHT", 14, 240 },
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

void sim_init(void) { nsims = 0; player = 0; clock_min = 8 * 60; clock_day = 0; cash = 15000; job_level = 0; job_worked = 0; last_note[0] = 0; }

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
}
void sim_switch_player(void) {
    if (nsims < 2) return;
    sims[player].ai = 1;
    player = (player + 1) % nsims;
    sims[player].ai = 0;
    sims[player].path_n = 0;
    if (sims[player].grump) sim_note("THIS ONE REFUSES THE MARKER. NOTHING FLOATS.");
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

// ---- path finding (BFS over the tiles) ----
static u16 bfs_q[MW * MH];
static s16 bfs_par[MH][MW];
int sim_find_path(Sim *s, int tx, int ty) {
    int sx = sim_tile_x(s), sy = sim_tile_y(s);
    if (sx == tx && sy == ty) { s->path_n = 0; return 1; }
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
    u8 rx[256], ry[256]; int n = 0;
    int c = ty * MW + tx;
    while (n < 255 && c != sy * MW + sx) { rx[n] = (u8)(c % MW); ry[n] = (u8)(c / MW); n++; c = bfs_par[c / MW][c % MW]; }
    int m = MIN(n, 64);
    for (int i = 0; i < m; i++) { s->path_x[i] = rx[n - 1 - i]; s->path_y[i] = ry[n - 1 - i]; }
    s->path_n = (u8)m; s->path_i = 0;
    return 1;
}
static int follow_path(Sim *s) {   // 1 while walking, 0 when there
    if (s->path_i >= s->path_n) return 0;
    int tx = s->path_x[s->path_i] * TILE + TILE / 2, tz = -(s->path_y[s->path_i] * TILE + TILE / 2);
    int dx = tx - s->x, dz = tz - s->z;
    int sp = speed_of(s);
    {   // housemates walk the same way: turn first, quarter speed while cornering
        int want = yaw_towards(dx, dz), diff = ((want - s->yaw + 2048) & 4095) - 2048;
        turn_to(s, want, 300 + (ABS(diff) >> 3));
        int align = 4096 - ABS(diff) * 2; if (align < 1024) align = 1024;
        sp = (sp * align) >> 12; if (sp < 2) sp = 2;
    }
    if (ABS(dx) <= sp && ABS(dz) <= sp) { s->x = tx; s->z = tz; s->path_i++; if (s->path_i >= s->path_n) return 0; return 1; }
    int mx = CLAMP(dx, -sp, sp), mz = CLAMP(dz, -sp, sp);
    // the last tile may be the item itself: step straight in
    if (s->path_i == s->path_n - 1) { s->x += mx; s->z += mz; }
    else if (!walk(s, mx, mz)) { s->x += mx; s->z += mz; }
    s->gait.amp = 4096; s->gait.phase = (s->gait.phase + 140) & 4095;
    return 1;
}

// ---- actions ----
static int action_for(char c) {
    switch (c) {
        case 'F': case 's': return ACT_EAT; case 'T': case '&': return ACT_WC; case 'S': case 'z': return ACT_SLEEP; case 'H': case '0': return ACT_SHOWER;
        case 'C': case 'U': case 'h': case 'N': case '%': return ACT_SIT;
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
        case 'b': return ACT_READ; case 'c': return ACT_COFFEE; case 'm': return ACT_RUN; case 'I': return ACT_PHONE; case 'x': return ACT_WORK;
        default: return ACT_NONE;
    }
}
static int on_item(int act) { return act == ACT_WC || act == ACT_SLEEP || act == ACT_SHOWER || act == ACT_SIT || act == ACT_RUN || act == ACT_WORK; }
static const char *act_hint(int act, char c) {
    switch (act) {
        case ACT_TRAVEL: return c == '@' ? "SAIL" : "TAKE THE BUS";
        case ACT_STAIRS: return "STAIRS";
        default: break;
    }
    switch (c) {   // the furniture pack's own words
        case '!': return "PLAY ARCADE"; case '$': return "PLAY PIANO"; case '(': return "PLAY GUITAR"; case ')': return "PAINT";
        case '>': return "SOAK"; case '[': return "USE COMPUTER"; case 'j': return "PICK A SONG"; case 'n': return "PLAY"; case '{': return "PLAY HOUSE";
        case '}': return "MAKE A WISH"; case '`': return "PRACTICE SPEECH"; case ',': return "TAKE A BATH"; case '-': return "WASH HANDS";
        case '/': return "COOK"; case ':': return "MICROWAVE"; case ';': return "SIT"; case '<': return "SLEEP"; case '?': return "PLAY CHESS";
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
                   case ACT_COFFEE: return 90; case ACT_RUN: return 360; case ACT_PHONE: return 300; case ACT_TALK: return 240; default: return 9999; }
}
static void begin_action(Sim *s, int act, int mx, int my) {
    if (act == ACT_TALK && (int)(s - sims) == player) story_talks++;
    s->action = act; s->act_t = act_len(act); s->act_x = mx; s->act_y = my; s->act_var = 0;
    s->path_n = 0;
    if (on_item(act)) { s->x = mx * TILE + TILE / 2; s->z = -(my * TILE + TILE / 2); s->yaw = (world_item_facing(mx, my) * 1024) & 4095; }
    else { s->yaw = yaw_towards(mx * TILE + TILE / 2 - s->x, -(my * TILE + TILE / 2) - s->z); }
    s->gait.sit = (act == ACT_SIT || act == ACT_WC);
    s->gait.sleep = (act == ACT_SLEEP);
    s->gait.amp = 0;
    if (act == ACT_SLEEP) audio_voice(VOICE_SNORE);
    if (act == ACT_EAT) audio_voice(VOICE_YEHA);
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
        case ACT_EAT: wants_event(s, W_EAT, 0); sim_moodlet(s, s->act_var == 1 ? ML_GOOD_MEAL : ML_SNACKED); break;
        case ACT_SLEEP: wants_event(s, W_SLEEP, 0); if (s->act_t <= 0 && s->act_var == 0) { wants_reroll(s); sim_moodlet(s, ML_RESTED); } break;   // a full sleep: new wants on waking
        case ACT_SHOWER: wants_event(s, W_SHOWER, 0); sim_moodlet(s, ML_FRESH); if (s->act_var == 1) sim_moodlet(s, ML_SINGING); break;
        case ACT_FUN: wants_event(s, W_FUN, 0); sim_moodlet(s, ML_FUN_TIME); break;
        case ACT_READ: wants_event(s, W_READ, 0); break;
        case ACT_COFFEE: wants_event(s, W_COFFEE, 0); sim_moodlet(s, ML_CAFFEINE); break;
        case ACT_RUN: wants_event(s, W_RUN, 0); sim_moodlet(s, ML_WORKOUT); break;
        case ACT_PHONE: wants_event(s, W_PHONE, 0); if (s->act_var == 1) sim_moodlet(s, ML_PIZZA); break;
        case ACT_WORK: wants_event(s, W_WORK, 0); break;
        case ACT_TALK: if (s->talk_to >= 0 && s->talk_to < nsims) chat_done(s, &sims[s->talk_to]); break;
    }
    if (s->action == ACT_TALK && s->talk_to >= 0 && s->talk_to < nsims) {
        Sim *o = &sims[s->talk_to];
        if (o->action == ACT_TALK && o->talk_to == (int)(s - sims)) { o->action = ACT_NONE; o->talk_to = -1; o->gait.wave = 0; }
    }
    if (on_item(s->action)) {   // step off the item onto open floor
        int mx = s->act_x, my = s->act_y, d = world_item_facing(mx, my);
        static const signed char dx[4] = { 0, 1, 0, -1 }, dy[4] = { 1, 0, -1, 0 };
        if (world_walkable(mx + dx[d], my + dy[d])) { s->x = (mx + dx[d]) * TILE + TILE / 2; s->z = -((my + dy[d]) * TILE + TILE / 2); }
    }
    s->action = ACT_NONE; s->act_t = 0; s->talk_to = -1;
    s->gait.sit = 0; s->gait.sleep = 0; s->gait.wave = 0;
}

static int is_night(void) { return clock_min >= NIGHT_FROM || clock_min < NIGHT_TO; }
static int is_work(void) { return clock_day < 5 && clock_min >= WORK_FROM && clock_min < WORK_TO; }

static void run_action(Sim *s) {
    int me = (int)(s - sims);
    switch (s->action) {
        case ACT_EAT: s->need[N_FOOD] += s->act_var == 1 ? GAIN_EAT * 3 / 2 : GAIN_EAT; if (s->act_var == 1 && !(s->act_t & 7)) s->skill_cook++; break;
        case ACT_WC: s->need[N_WC] += GAIN_WC; break;
        case ACT_SLEEP: s->need[N_REST] += is_night() ? GAIN_SLEEP * 5 / 4 : GAIN_SLEEP; s->need[N_COMFY] += 40;
                        if ((s->need[N_REST] >> 10) >= 100 && (!is_night() || clock_min < NIGHT_FROM)) { s->act_t = 0; sim_say(s, "RESTED"); } break;
        case ACT_SHOWER: s->need[N_CLEAN] += GAIN_WASH; if (s->act_var == 1) s->need[N_FUN] += 90; break;
        case ACT_SIT: s->need[N_COMFY] += GAIN_SIT; if ((s->need[N_COMFY] >> 10) >= 100) s->act_t = 0; break;
        case ACT_FUN: s->need[N_FUN] += GAIN_FUN; s->need[N_COMFY] += 30; break;
        case ACT_READ: s->need[N_FUN] += s->act_var == 1 ? GAIN_FUN / 4 : GAIN_FUN / 2; s->skill_fun += s->act_var == 1 ? 3 : 1; break;
        case ACT_COFFEE: s->need[N_REST] += s->act_var == 1 ? 320 : 180; s->need[N_WC] -= s->act_var == 1 ? 110 : 60; break;
        case ACT_RUN: s->need[N_FUN] += 120; s->need[N_CLEAN] -= s->act_var == 1 ? 160 : 90; s->skill_body += s->act_var == 1 ? 2 : 1; s->gait.amp = 4096; s->gait.phase = (s->gait.phase + 200) & 4095; break;
        case ACT_PHONE: if (s->act_var == 1) s->need[N_FOOD] += 300; else s->need[N_SOCIAL] += GAIN_TALK; s->gait.wave = 2; break;
        case ACT_TALK: s->need[N_SOCIAL] += GAIN_TALK; s->need[N_FUN] += 40; if ((s->act_t & 63) == 0) s->gait.wave = 40; break;
        case ACT_WORK: if (is_work()) { if (me == player) job_worked++; s->need[N_FUN] -= 20; } else { s->act_t = 0; } break;
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
static void ai_think(Sim *s) {
    if (s->action != ACT_NONE || s->path_i < s->path_n) return;
    if (s->wander_t > 0) { s->wander_t--; return; }
    int me = (int)(s - sims);
    // the lowest need
    int worst = -1, wv = 101;
    for (int n = 0; n < N_SOCIAL; n++) { int v = s->need[n] >> 10; if (v < wv) { wv = v; worst = n; } }
    if (wv < 45) {
        if (worst == N_REST && !is_night() && wv > 15) { worst = -1; }
        char ch = worst >= 0 ? need_item(worst) : 0;
        int ix, iy, tx, ty;
        if (ch && nearest_item(s, ch, &ix, &iy) && approach_tile(s, ix, iy, &tx, &ty) && sim_find_path(s, tx, ty)) {
            s->act_x = ix; s->act_y = iy; s->action = ACT_WALK; s->act_t = 9999; return;
        }
    }
    // social: find the player or another sim
    if ((s->need[N_SOCIAL] >> 10) < 40 && nsims > 1) {
        int best = -1, bd = 1 << 30;
        for (int i = 0; i < nsims; i++) { if (i == me || sims[i].action != ACT_NONE) continue; int d = ABS(sims[i].x - s->x) + ABS(sims[i].z - s->z); if (d < bd) { bd = d; best = i; } }
        if (best >= 0 && bd < TILE * 10) {
            int tx = sim_tile_x(&sims[best]), ty = sim_tile_y(&sims[best]);
            int ax, ay; if (approach_tile(s, tx, ty, &ax, &ay) && sim_find_path(s, ax, ay)) { s->talk_to = best; s->action = ACT_WALK; s->act_x = tx; s->act_y = ty; s->act_t = 9999; return; }
        }
    }
    // wander
    for (int t = 0; t < 8; t++) {
        int tx = sim_tile_x(s) + (int)(look_rand() % 11) - 5, ty = sim_tile_y(s) + (int)(look_rand() % 11) - 5;
        if (tx > 0 && ty > 0 && tx < MW - 1 && ty < MH - 1 && world_walkable(tx, ty) && sim_find_path(s, tx, ty)) { s->action = ACT_WALK; s->act_x = -1; s->act_t = 9999; break; }
    }
    if (s->grump && !(look_rand() % 3)) sim_say(s, grump_line());
    s->wander_t = 120 + (int)(look_rand() % 300);
}

static void arrive(Sim *s) {   // an AI sim reached the end of its path
    s->action = ACT_NONE;
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
    if (s->act_x >= 0) {
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
    // passing out
    if ((s->need[N_REST] >> 10) == 0 && s->action != ACT_SLEEP) { s->need[N_REST] = 25 << 10; s->mood -= 30; sim_say(s, "PASSED OUT"); audio_voice(VOICE_GROAN); wants_fear(s, F_PASSOUT); sim_moodlet(s, ML_PASSED_OUT); }
    if ((s->need[N_WC] >> 10) == 0) { s->need[N_WC] = 100 << 10; s->need[N_CLEAN] = 0; s->mood -= 40; sim_say(s, "OH NO"); audio_voice(VOICE_PEE); wants_fear(s, F_PEE); sim_moodlet(s, ML_WET); sim_moodlet(s, ML_EMBARRASSED); }
    if (wants_desperate(s) && (s->think_t % 700) == 1 && s->action == ACT_NONE) {   // rock bottom: acting out
        static const char *const act[4] = { "KICKS THE BIN", "JUMPS ON THE SOFA", "SULKS LOUDLY", "WONT LEARN A THING" };
        sim_say(s, act[look_rand() & 3]);
    }
}

static void clock_tick(void) {
    static int acc = 0;
    int spm = STEPS_MIN;
    if (sims[player].action == ACT_SLEEP) spm = 1;
    if (sim_speed == 2) spm /= 3; if (sim_speed == 3) spm = 1;
    if (spm < 1) spm = 1;
    if (++acc < spm) return;
    acc = 0;
    clock_min++;
    for (int i = 0; i < nsims; i++) { wants_minute(&sims[i]); moodlets_minute(&sims[i]); }
    if (clock_min == 16 * 60) for (int i = 0; i < nsims; i++) for (int j = 0; j < nsims; j++) { int r = sims[i].rel[j]; sims[i].rel[j] = (s8)(r > 0 ? r - 2 : r < 0 ? r + 2 : 0); }   // 4 PM: the day's warmth fades a little
    if (clock_min == WORK_FROM && clock_day < 5) { sim_note("WORK: 9 TO 5 AT THE MARKER"); job_worked = 0; }
    if (clock_min == WORK_TO && clock_day < 5) {
        int quota = 300, pay = 0;
        if (job_worked >= quota) { pay = PAY0 + PAY_LVL * job_level; job_days_good++; if (job_worked >= quota * 2) pay += 600; }
        else if (job_worked >= quota / 2) { pay = (PAY0 + PAY_LVL * job_level) / 2; }
        else { job_strikes++; }
        if (pay) { cash += pay; story_paid++; char b[32]; sprintf(b, "PAID %d", pay); sim_note(b); wants_event(&sims[player], W_PAID, 0); sim_moodlet(&sims[player], ML_PAYDAY);
                   if (story_paid == 1) sim_note("PAID! THE ISLAND FERRY WILL TAKE YOU NOW"); audio_voice(VOICE_YAHOO); }
        else { sim_note("NO PAY TODAY"); wants_fear(&sims[player], F_BADDAY); }
        if (job_days_good >= 3 && job_level < 5) { job_level++; job_days_good = 0; sim_note("PROMOTED!"); wants_event(&sims[player], W_PROMO, 0); sim_moodlet(&sims[player], ML_PROMOTED); }
        if (job_strikes >= 3 && job_level > 0) { job_level--; job_strikes = 0; sim_note("DEMOTED"); wants_fear(&sims[player], F_DEMOTED); }
    }
    if (clock_min >= 1440) {
        clock_min = 0; clock_day = (clock_day + 1) % 7;
        cash -= BILLS;
        if (cash < 0) { cash = 0; sim_note("BROKE! BILLS UNPAID"); sims[player].mood -= 30; audio_voice(VOICE_CRY); wants_fear(&sims[player], F_BROKE); }
        else sim_note("BILLS PAID: 800");
    }
}

void sim_step(void) {
    clock_tick();
    for (int i = 0; i < nsims; i++) {
        Sim *s = &sims[i];
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
    if (s->action != ACT_NONE && s->action != ACT_WALK) { if (s->action == ACT_WORK || s->action == ACT_SLEEP) return; end_action(s); }
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
static int target_of(const Sim *s, int *mx, int *my, int *who) {
    *who = -1;
    int me = (int)(s - sims);
    // a person in front?
    for (int i = 0; i < nsims; i++) {
        if (i == me) continue;
        int dx = sims[i].x - s->x, dz = sims[i].z - s->z;
        if (ABS(dx) < TILE * 3 / 2 && ABS(dz) < TILE * 3 / 2 && sims[i].action != ACT_SLEEP) { *who = i; return 1; }
    }
    int fx, fy;
    if (front_tile(s, &fx, &fy) && action_for(world_at(fx, fy)) != ACT_NONE) { *mx = fx; *my = fy; return 1; }
    int cx = sim_tile_x(s), cy = sim_tile_y(s);
    static const signed char dx[5] = { 0, 0, 1, 0, -1 }, dy[5] = { 0, 1, 0, -1, 0 };
    for (int k = 0; k < 5; k++) { int tx = cx + dx[k], ty = cy + dy[k]; if (action_for(world_at(tx, ty)) != ACT_NONE) { *mx = tx; *my = ty; return 1; } }
    if (s->lvl > 0) for (int k = 0; k < 5; k++) { int tx = cx + dx[k], ty = cy + dy[k];   // the top of a flight of stairs (open air, the stairs below)
        if (world_at_l(s->lvl, tx, ty) == ' ' && world_at_l(s->lvl - 1, tx, ty) == '|') { *mx = tx; *my = ty; return 1; } }
    return 0;
}
static const char *sim_hint_(void) {
    const Sim *s = &sims[player];
    if (s->action != ACT_NONE && s->action != ACT_WALK) return "GET UP";
    int mx, my, who;
    if (!target_of(s, &mx, &my, &who)) return "";
    if (who >= 0) return "TALK";
    char c = world_at(mx, my);
    if (c == 'x' && !is_work()) return "NOT WORK TIME";
    return act_hint(action_for(c), c);
}
// ================================================================== THE PIE MENU (Sims 1 & 2)
// X opens it on what is in front: a person gets the social moves, a thing gets its ways of being used.
enum { SO_TALK, SO_JOKE, SO_HUG, SO_COMPLIMENT, SO_FLIRT, SO_TEASE, SO_N };
static const char *const soNm[SO_N] = { "TALK", "JOKE", "HUG", "COMPLIMENT", "FLIRT", "TEASE" };
static int pie_who, pie_mx, pie_my, pie_n; static u8 pie_code[8];   // what is open: a person (pie_who >= 0) or a tile
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
        int adults = s->bore.stage >= AG_TEEN && o->bore.stage >= AG_TEEN;
        for (int k = 0; k < SO_N; k++) {
            if (k == SO_HUG && s->rel[who] < 25) continue;                  // you hug friends
            if (k == SO_FLIRT && (!adults || s->rel[who] < 35)) continue;   // flirting needs grown-ups who get on
            if (k == SO_TEASE && s->mood > 40 && !has_trait(s, TR_NICE, 0)) continue;   // a good mood doesn't think of teasing (MEAN always does)
            strcpy(opt[pie_n], soNm[k]); pie_code[pie_n++] = (u8)k;
        }
        return pie_n;
    }
    char c = world_at(mx, my); int act = action_for(c);
    if (c == ' ') { strcpy(pie_title, "STAIRS"); strcpy(opt[0], "GO DOWN"); pie_code[0] = 1; pie_n = 1; return 1; }
    if (c == 'x' && !is_work()) { sim_note("WORK IS 9 TO 5, MON TO FRI"); return 0; }
    strcpy(pie_title, act_hint(act, c));
    #define OPT(txt, v) { strcpy(opt[pie_n], txt); pie_code[pie_n++] = v; }
    switch (act) {
        case ACT_EAT: OPT("SNACK", 0) OPT("COOK A MEAL", 1) break;
        case ACT_SLEEP: OPT("SLEEP", 0) OPT("NAP", 1) break;
        case ACT_SHOWER: OPT("SHOWER", 0) OPT("SING IN IT", 1) break;
        case ACT_RUN: OPT("JOG", 0) OPT("SPRINT", 1) break;
        case ACT_COFFEE: OPT("A CUP", 0) OPT("ESPRESSO", 1) break;
        case ACT_READ: OPT("READ", 0) OPT("STUDY", 1) break;
        case ACT_PHONE: OPT("CALL A FRIEND", 0) OPT("ORDER PIZZA", 1) break;
        case ACT_STAIRS: OPT("GO UP", 0) break;
        case ACT_FUN: if (c == 'R' || c == 'A' || c == 'j') {   // a radio: the stations
                          for (int k = 0; k < audio_station_count(); k++) OPT(audio_station_name(k), 10 + k)
                          OPT("TURN IT OFF", 9) break; }
                      OPT(act_hint(act, c), 0) break;
        default: OPT(act_hint(act, c), 0) break;
    }
    #undef OPT
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
static void sim_pie_do_(int i) {
    Sim *s = &sims[player];
    if (i < 0 || i >= pie_n) return;
    if (pie_who >= 0) { social(s, &sims[pie_who], pie_code[i]); return; }
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
    begin_action(s, act, pie_mx, pie_my);
    s->act_var = pie_code[i];
    if (act == ACT_SLEEP && s->act_var == 1) s->act_t = 300;              // a nap: an hour or so
    if (act == ACT_EAT && s->act_var == 1) s->act_t = 420;                // cooking takes a while
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
    if (c == 'x' && !is_work()) { sim_note("WORK IS 9 TO 5, MON TO FRI"); return 0; }
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
