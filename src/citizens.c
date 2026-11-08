// citizens: see citizens.h
#include "citizens.h"
#include "story.h"
#include "ui.h"
#include "sim.h"
#include "world.h"
#include "voxel.h"
#include "look.h"
#include "gfx.h"
#include "audio.h"
#include "items.h"
#include "crowd.h"
#include "pop.h"
#include "bigmap.h"
#include "persona.h"
#include "save.h"

#define LIVE_MAX  6                 // civilians drawn live in 3D at once (the nearest); everyone else is a sprite of the same model
int cit_live_max = LIVE_MAX, cit_maxdraw = 64, cit_sprite_rate = 1;   // (the options menu: live ones, the crowd's size, sprite refresh 0 slow 1 normal 2 fast)
#define MAXDRAW   64                // civilians drawn at all, nearest first (a party fills a screen)
#define DRAW_DIST (14 * TILE)
#define FULL_DIST (5 * TILE)
#define FULL_EYE  2900                // ... and closer than this to the camera (the default view looks from ~2160 away)
#define REACH     (TILE * 3 / 2)    // how close counts as "in front of you"
#define PUNCH_R   150
#define CR        40                // body radius outside
#define CRI       28                // ... and indoors, where the furniture is tight
#define RING_COST 150
#define GIFT_COST 20
#define BUILD_PER_FRAME 1
#define NHH       120
#define MAIN0     50                // the first of SAN BOREAS's slots
#define NOPID     0xFFFF
#define NONE      255

enum { CM_IDLE, CM_WALK, CM_CHAT, CM_CHASE, CM_KO, CM_GONE, CM_SLEEP, CM_GOHOME, CM_LEAVE, CM_CHORE, CM_VISIT, CM_PARTY, CM_STOP, CM_AWAY, CM_GREET, CM_BUY };   // (STOP: a peacemaker, come to break the fight up; GREET: someone who knows you, come over to say hello; BUY: a customer for the farm stand)
enum { PE_FRIENDLY, PE_GRUFF, PE_FLIRTY, PE_SHY };
enum { PO_TALK, PO_JOKE, PO_HUG, PO_FLIRT, PO_GIFT, PO_PROPOSE, PO_FIGHT, PO_SORRY, PO_FAMILY, PO_MOVEIN,
       PO_SUB_CHAT, PO_SUB_SOC, PO_SUB_ROM, PO_SUB_MEAN, PO_BACK,   // the pie's sub-menus
       PO_COMPL, PO_GOSSIP, PO_JOB, PO_ADVICE, PO_HIGH5, PO_DANCE, PO_FLOWERS, PO_DRINK, PO_KISS, PO_DATE, PO_INSULT, PO_PRANK, PO_APOL };
// the moodlet ids in sim.c's list
enum { CML_GOOD_CHAT = 5, CML_EMBARRASSED = 9, CML_REJECTED = 12, CML_HUGGED = 14, CML_LAUGH = 16, CML_FLIRTY = 17 };

typedef struct {
    u32  seed;                      // the dice that make this person (look_random from this seed, always the same one)
    char first[NAME_LEN], last[NAME_LEN];
    LodBore lod;                    // the crowd model, built once
    Gait gait;
    s16  x, z;                      // world position (z is negative south, like the sims)
    u16  yaw, wyaw, t, say_t;
    const char *say;
    u8   zone, stage, mode, built, hp, pers, slot, cd;
    s8   rel, rom;                  // what they think of you, and the romance
    u8   hh, mem, sub, homebody;    // their household, their place in it, the step of a door/bed routine, stays in by day
    s8   off;                       // minutes early or late: no household moves as one
    u8   owe;                       // the scheduler: steps lived but not yet thought through (in what was padding)
    u16  st;                        // steps spent held up
    u8   wx, wy, gx, gy, wv;        // the next tile of a routed walk, the goal tile it was routed to, whether it is current (2: no way found from tile wx,wy)
    u8   path[8], plen, pi;         // the route itself, 2 bits a step (worked out once per goal, not at every tile)
    u8   chore, chain, chx, chy, ix, iy, vh;   // a chore at home: the item letter, the one that follows it, where they stand, where the item is; a visit: the door tile and the household
    u8   zone0;                     // a party guest from another zone: where to go back to (255: none)
    u8   flg;                       // F_*: met you, said hello, known some other way, put off, far; the tier (in what was padding)
    u16  pid, mplace;               // SAN BOREAS: who they are in pop.c (NOPID: one of the island's or the town's own), and where they were brought to life (NOPID: home)
    u8   ax, ay, vanish;            // ... the place they hang about (a tile), and 1 when they are wanted elsewhere
    u8   cop;                       // 1: a police officer, called to a fight (in uniform)
} Cit;
#define F_MET  1                    // has met you: any interaction at all, for good
#define F_HI   2                    // came over to say hello, this time out
#define F_KN   4                    // known to you some other way (pop.c: a moved-out Bore, a neighbour, a coworker)
#define F_LATE 8                    // put off by the clock last frame
#define F_FAR  16                   // a far stranger, past 26 tiles
#define F_TSH  5                    // the tier this frame, bits 5-6
typedef struct { u8 zone, home, n, fam; u8 mem[6]; u16 pfam; } Hh;   // a household: who lives in which of the zone's homes, under which surname (pfam: its pop.c family, or NOPID)

static Cit cits[NCIT];
static Hh hh[NHH]; static int nhh; static u8 formed[ZONE_N];
// (a citizen is built in crowd.c's unpacking Bore: it is only ever needed between one unpack and the drawing that follows)
static void party_leave_done(Cit *c);
static void meet(Cit *c); static int cit_known(const Cit *c);
static void crowd_react(int start); static void heat_add(int v); static int fam_in_reach(void); static int fam_punch(void); static void fam_calm(void); static void fam_step(void); static int heat;
extern int aggro_pct, rage_pct, aggro_who, rage_who, peace_on;
static int party_on, party_left, party_cx, party_cy, party_in, party_spx, party_spz, party_n, party_next, party_t, home_guests;
#define PARTY_MAX 60
static u8 party_g[PARTY_MAX], party_sx[PARTY_MAX], party_sy[PARTY_MAX];
static int tick, pl_hp = 100, pl_cd, cur = -1, fight_on, moved_flag;
#ifdef AUTO_CARD
int dbg_cardname;
#endif
static u32 crng = 0x9E3779B9u;
static u8 moved_mask[(50 + 7) / 8]; static u8 moved_got;   // the crowd townies (slots 0..49) who moved in with you: the save keeps the list (version 53), so they are not out in town as well
void citizens_save(void) { sv_n(moved_mask, sizeof moved_mask); }
void citizens_load(void) { ld_n(moved_mask, sizeof moved_mask); moved_got = 1; }
static char pie_title[NAME_LEN + 2];
static u8 pie_code[8]; static int pie_c = -1;

#ifdef CIT_PROBE   // (test builds: what the townsfolk do over a run, to compare one scheduler with another: tools/perf/citprobe.py)
int dbg_cc[16];                       // counters (names in citprobe.py)
u16 dbg_cpr[24][24]; int dbg_cpr_n;   // a snapshot of the crowd every 512 steps
int dbg_cm[16][2], dbg_pm[16][2];     // steps lived and ground walked, by mode; route searches by mode, and the ones that found no way
int dbg_rf[8][8], dbg_rfn;            // the last route searches that found no way: who, from, to
int dbg_wk[4];                        // walks begun, ended by the time, ended held up, steps held up
#define CP(k, v) (dbg_cc[k] += (v))
#define WK(k) (dbg_wk[k]++)
#else
#define CP(k, v) ((void)0)
#define WK(k) ((void)0)
#endif
#ifdef AUTO_LE
extern int dbg_greet[6], dbg_buy[6];
#define GL(k) (dbg_greet[k]++)
#define GB(k) (dbg_buy[k]++)
#define IFLE(x) (x)
#else
#define GL(k) ((void)0)
#define GB(k) ((void)0)
#define IFLE(x) ((void)0)
#endif
static u32 crnd(void) { crng ^= crng << 13; crng ^= crng >> 17; crng ^= crng << 5; return crng; }
static int  rn(int n) { return (int)((crnd() >> 8) % (u32)n); }

// ---- small helpers -------------------------------------------------------------------------------
static int atan2_i(int y, int x) {      // 0..4095 round the circle, 0 along +x, counter-clockwise
    if (!x && !y) return 0;
    int ax = ABS(x), ay = ABS(y), a;
    int hi = MAX(ax, ay), lo = MIN(ax, ay);
    int r = (lo << 12) / hi;
    a = (r * (512 + ((178 * (4096 - r)) >> 12))) >> 12;
    if (ay > ax) a = 1024 - a;
    if (x < 0) a = 2048 - a;
    if (y < 0) a = 4096 - a;
    return a & 4095;
}
static int yaw_to(int dx, int dz) { return atan2_i(dx, -dz); }   // yaw 0 faces -z, as the sims do
static void turn(Cit *c, int target, int rate) {
    int d = ((target - c->yaw + 6144) & 4095) - 2048;
    c->yaw = (u16)((c->yaw + CLAMP(d, -rate, rate)) & 4095);
}
static int indoors_at(int x, int z) {   // inside a building: the floor under it is one of the room floors
    int mx = x / TILE, my = (-z) / TILE;
    if (x < 0 || z > 0 || mx >= MW || my >= MH) return 0;
    return floorMap[my][mx] < FL_GRASS;
}
static int body_free(int x, int z, int in) {   // can a body stand here? in: inside a room (and stay inside), else out on the streets (not water, not in things)
    int r = in ? CRI : CR;
    for (int k = 0; k < 4; k++) {
        int px = x + ((k & 1) ? r : -r), pz = z + ((k & 2) ? r : -r);
        if (px < 0 || pz > 0) return 0;
        int mx = px / TILE, my = (-pz) / TILE;
        if (mx >= MW || my >= MH) return 0;
        int f = floorMap[my][mx];
        if (in ? f >= FL_GRASS : (f < FL_GRASS || f == FL_WATER)) return 0;
        if (!world_walkable(mx, my)) return 0;
    }
    return 1;
}
static int open_at(int x, int z) { return body_free(x, z, 0); }
static void say(Cit *c, const char *t) { c->say = t; c->say_t = 200; }
static int adult(const Cit *c) { return c->stage >= AG_ADULT; }
static int pl_adult(void) { return sims[player].bore.stage >= AG_ADULT; }

static const char *const L_FRIEND[] = { "NICE DAY FOR IT", "HAVE YOU SEEN MY DOG", "THE NOODLES ARE GOOD", "I LOVE THIS PLACE", "YOU LOOK LIKE A BORE", "HEARD A RUMOUR", "I NEED A NAP", "BUSES ARE LATE AGAIN", "MY FEET HURT", "WANT A MINT?", "THE DRUMS WERE LOUD", "LIFE IS GOOD, MAN" };
static const char *const L_GRUFF[]   = { "WHAT.", "GO AWAY", "I'M BUSY BEING MAD", "NOT NOW", "UGH. YOU.", "DON'T TALK TO ME", "WHAT DO YOU WANT" };
static const char *const L_KID[]     = { "I LIKE FROGS", "TAG! YOU'RE IT", "ARE WE THERE YET", "MY MUM SAYS HI", "I CAN BURP A SONG", "LOOK AT MY STICK" };
static const char *const L_ELD[]     = { "BACK IN MY DAY", "THESE KNEES...", "KIDS TODAY, HONESTLY", "SPEAK UP, DEAR", "I REMEMBER THE SEA", "HAVE A BISCUIT" };
static const char *const L_HOME[]    = { "MAKE YOURSELF AT HOME", "MIND THE RUG", "WE LIVE HERE, YOU KNOW", "THE KETTLE'S ON", "NICE OF YOU TO DROP BY" };
#define NL(a) ((int)(sizeof(a) / sizeof((a)[0])))
COLD static const char *chat_line(const Cit *c) {
    if (indoors_at(c->x, c->z) && !(crnd() & 1) && c->pers != PE_GRUFF) return L_HOME[rn(NL(L_HOME))];   // in their own house
    if (c->stage <= AG_TEEN) return L_KID[rn(NL(L_KID))];
    if (c->pers == PE_GRUFF) return L_GRUFF[rn(NL(L_GRUFF))];
    if (c->stage == AG_ELDER && (crnd() & 1)) return L_ELD[rn(NL(L_ELD))];
    return L_FRIEND[rn(NL(L_FRIEND))];
}

// ---- bodies ----------------------------------------------------------------------------------------
static void cit_persona(Bore *b, u32 seed) {   // their personality, from their seed alone (persona.c): the same in town, on their card and the day they move in
    Persona p; persona_of(seed, b->stage, &p);
    for (int i = 0; i < TR_N; i++) b->tr[i] = p.bar[i];
    b->asp = p.asp; b->ltw = p.ltw;
}
COLD static void cit_bore0(Bore *b, const Cit *c);
COLD static void cit_bore(Bore *b, const Cit *c) { cit_bore0(b, c); cit_persona(b, c->seed); }   // the same person, every time, from the seed (b may already own its face textures)
COLD static void cit_bore0(Bore *b, const Cit *c) {
    memset(b->look, 0, sizeof b->look);
    b->stage = c->stage;
    if (c->pid != NOPID) { const u8 *lk = pop_custom_look(c->pid, b->first, b->last);   // one of yours who moved out: their own looks and names
        if (lk) { memcpy(b->look, lk, LK_COUNT); b->stage = c->stage; look_fix(b); look_build(b); return; } }
    u32 old = look_seed(c->seed * 2654435761u + 12345u);
    look_random(b);
    look_seed(old);
    if (c->cop) { static int pt; if (!pt) pt = look_theme_named("POLICE"); look_theme(b, pt); b->col[6] = RGB(4, 8, 20); b->col[7] = RGB(3, 5, 14); }   // (the uniform)
    if (c->pid != NOPID) { pop_first(c->pid, b->first); pop_surname(pop_fam_of(c->pid), b->last); return; }   // SAN BOREAS: their own names
    { TownDna d; persona_town_dna(c->seed, &d); look_first_gen(d.first, b->first); look_last_gen(d.last, b->last); }   // a crowd townie's name: from their seed alone (the same name in every boot, every save of this world)
    if (c->hh != NONE) { strncpy(b->last, look_last_name(hh[c->hh].fam), NAME_LEN - 1); b->last[NAME_LEN - 1] = 0; }   // a household shares its surname
}
static void build_lod(Cit *c) {
    Bore *sp = crowd_temp_bore();
    #define scratch (*sp)
    memset(sp, 0, sizeof *sp);
    int id = (int)(c - cits);
    crowd_faces(id, &scratch);                                         // the person's own face slot: look_build bakes the eyes and mouth into it
    cit_bore(&scratch, c);
    crowd_store(id, &scratch);                                         // and the full model, kept compact
    strncpy(c->first, scratch.first, NAME_LEN - 1); c->first[NAME_LEN - 1] = 0;
    strncpy(c->last, scratch.last, NAME_LEN - 1); c->last[NAME_LEN - 1] = 0;
    voxel_lod_build(&scratch, &c->lod);
    c->built = 1; CP(9, 1);
    #undef scratch
}

// ---- homes and households -----------------------------------------------------------------------------
static const Home *home_of(const Cit *c) { return c->hh == NONE ? 0 : world_home(c->zone, hh[c->hh].home); }
static void door_geo(const Home *hm, int *x, int *zf, int *zd, int *zi) {   // the door: where you stand outside it, in it, and just inside
    *x = hm->dx * TILE + TILE / 2;
    *zf = -((hm->dy + 1) * TILE + TILE / 2); *zd = -(hm->dy * TILE + TILE / 2); *zi = -((hm->dy - 1) * TILE + TILE / 2);
}
COLD static void inside_spot(const Home *hm, int *x, int *z) {   // somewhere free on the floor
    for (int t = 0; t < 40; t++) {
        int tx = hm->x0 + rn(hm->x1 - hm->x0 + 1), ty = hm->y0 + rn(hm->y1 - hm->y0 + 1);
        if (ty >= hm->dy - 1 && tx == hm->dx) continue;       // keep the lane inside the door open
        if (world_at(tx, ty) == '.' && world_walkable(tx, ty)) { *x = tx * TILE + TILE / 2; *z = -(ty * TILE + TILE / 2); return; }
    }
    int a, b, c2; door_geo(hm, x, &a, &b, &c2); *z = c2;
}
COLD static void sleep_spot(const Cit *c, const Home *hm, int *x, int *z) {   // a bed each (a double bed for a couple, two to a bunk bed); anyone left lies down on the floor
    int k = 0, want_ = c->mem, couple = 0;
    if (c->pid != NOPID) { u8 a, kd, t, e; pop_family_kinds(pop_fam_of(c->pid), &a, &kd, &t, &e, (u8 *)&couple); couple &= 1; if (couple && want_ > 0) want_--; }
    for (int y = hm->y0; y <= hm->y1; y++) for (int xx = hm->x0; xx <= hm->x1; xx++) {
        char ch = world_at(xx, y); if (ch != 'S' && ch != '<' && ch != CH_BED4 && ch != CH_BEDF && ch != CH_BEDT && ch != CH_BEDL) continue;
        int cap = ch == '<' ? 2 : 1;
        if (want_ < k + cap) { *x = xx * TILE + TILE / 2; *z = -(y * TILE + TILE / 2); return; }
        k += cap;
    }
    int need = want_ - k; k = 0;
    for (int y = hm->y0; y <= hm->y1 && y < hm->dy - 1; y++) for (int xx = hm->x0; xx <= hm->x1; xx++)
        if (world_at(xx, y) == '.' && world_walkable(xx, y)) { if (k++ == need) { *x = xx * TILE + TILE / 2; *z = -(y * TILE + TILE / 2); return; } }
    inside_spot(hm, x, z);
}
static void roles(int size, u8 *st) {   // who lives together
    int r = rn(10);
    if (size == 1) { st[0] = r < 3 ? AG_ELDER : AG_ADULT; return; }
    if (size == 2) {
        if (r < 5) st[0] = st[1] = AG_ADULT;                                      // a couple, or flatmates
        else if (r < 7) st[0] = st[1] = AG_ELDER;                                 // a retired couple
        else if (r < 9) { st[0] = AG_ADULT; st[1] = rn(2) ? AG_CHILD : AG_TEEN; }  // a parent and a kid
        else { st[0] = AG_ELDER; st[1] = AG_ADULT; }                              // a grown-up child looking after a parent
        return;
    }
    st[0] = st[1] = AG_ADULT; int k = 2;
    if (size >= 5) st[k++] = AG_ELDER;                                            // a grandparent moved in
    while (k < size) st[k++] = rn(3) ? (rn(2) ? AG_CHILD : AG_TEEN) : AG_ADULT;   // the kids (and a lodger now and then)
    if (size == 3 && r < 3) st[2] = AG_ADULT;                                     // three flatmates
}
COLD static void form_households(int zone) {   // the first time a zone is seen: its people sort themselves into the homes the generator built
    if (zone == ZONE_CITY || formed[zone]) return;   // (SAN BOREAS's households come from pop.c)
    formed[zone] = 1;
    u32 keep_rng = crng; crng = persona_mix(world_seed() ^ 0x48554F53u, (u32)zone + 1u); if (!crng) crng = 1;   // who lives with whom: from the world seed and the zone alone (the same every boot); the dice are put back at the end
    int nh = world_home_count(zone), idx[NCIT], n = 0;
    for (int i = 0; i < NCIT; i++) if (cits[i].zone == zone) idx[n++] = i;
    if (!nh || !n) { crng = keep_rng; return; }
    int order[MAXHOMES];
    for (int i = 0; i < nh; i++) order[i] = i;
    for (int i = nh - 1; i > 0; i--) { int j = rn(i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    int used = 0;
    for (int hi = 0; hi < nh && used < n && nhh < NHH; hi++) {
        int left = n - used, hl = nh - hi, avg = (left + hl - 1) / hl;
        int size = hl == 1 ? left : CLAMP(avg + rn(3) - 1, 1, 6);
        size = MIN(MIN(size, left), 6);
        Hh *hg = &hh[nhh]; hg->zone = (u8)zone; hg->home = (u8)order[hi]; hg->n = (u8)size; hg->fam = (u8)rn(16);
        u8 st[6]; roles(size, st);
        for (int j = 0; j < size; j++) {
            Cit *c = &cits[idx[used++]];
            c->hh = (u8)nhh; c->mem = (u8)j; c->stage = st[j]; hg->mem[j] = (u8)(c - cits);
            c->homebody = (u8)(rn(100) < (c->stage == AG_ELDER ? 60 : c->stage == AG_ADULT ? 25 : c->stage == AG_TEEN ? 10 : 0));
        }
        nhh++;
    }
    crng = keep_rng;
}

// ---- the day: asleep, at home, or out ------------------------------------------------------------------
static int want(const Cit *c) {   // 0 out and about, 1 at home awake, 2 asleep in bed
    if (c->pid != NOPID) return pop_want(c->pid);
    int m = clock_min + c->off; if (m < 0) m += 1440; if (m >= 1440) m -= 1440;
    if (m >= 1350 || m < 400) return 2;
    if (c->homebody) return 1;
    int we = clock_day >= 5, a, b;
    switch (c->stage) {
        case AG_ELDER: a = 540; b = 990; break;
        case AG_CHILD: a = we ? 570 : 870; b = 1140; break;     // school, then out to play
        case AG_TEEN:  a = we ? 570 : 840; b = 1200; break;
        default:       a = we ? 660 : 450; b = we ? 1200 : 1080; break;   // work
    }
    return (m >= a && m < b) ? 0 : 1;
}

// ---- the hundred ------------------------------------------------------------------------------------
COLD void citizens_init(void) {
    crng ^= (u32)look_rand() * 2246822519u;   // (what the crowd does from here is dice; who they are is not: see persona_town_seed)
    if (!crng) crng = 0x9E3779B9u;
    u32 wseed = world_seed();
    if (!moved_got) memset(moved_mask, 0, sizeof moved_mask);   // (a new life: nobody has moved in yet; a loaded one brought its list)
    memset(cits, 0, sizeof cits);
    for (int i = 0; i < NCIT; i++) {
        Cit *c = &cits[i];
        TownDna dna; c->seed = persona_town_seed(wseed, (u32)i); persona_town_dna(c->seed, &dna);   // (a pure function of the world seed and the slot)
        c->zone = i < 10 ? ZONE_ISLAND : i < 50 ? ZONE_TOWN : 255;          // 10 on the island, 40 in town; the rest wait for SAN BOREAS's people
        c->pid = NOPID; c->mplace = NOPID; c->vanish = 0;
        c->stage = dna.age == 0 ? AG_CHILD : dna.age == 1 ? AG_TEEN : dna.age == 2 ? AG_ADULT : AG_ELDER;   // (until a household is formed round them)
        c->pers = dna.pers;
        c->hh = NONE; c->off = dna.off; c->sub = 0; c->st = 0;
        c->hp = 100; c->mode = CM_IDLE; c->slot = 0; c->built = 0; c->zone0 = 255; c->owe = 0; c->flg = 0;
        strcpy(c->first, "..."); c->last[0] = 0;
        if (i >= MAIN0) c->mode = CM_GONE;
        if (i < MAIN0 && (moved_mask[i >> 3] >> (i & 7) & 1)) c->mode = CM_GONE;   // moved in with you: they are in your house, not out in town too
    }
    moved_got = 0;
    memset(hh, 0, sizeof hh); nhh = 0; memset(formed, 0, sizeof formed);
    for (int k = 0; k < NHH; k++) { hh[k].pfam = NOPID; hh[k].zone = 255; }
    crowd_reset();
    pl_hp = 100; pl_cd = 0; cur = -1; fight_on = 0; pie_c = -1;
#ifdef AUTO_RAGE
    rage_pct = 100; rage_who = 0; aggro_pct = 100;   // (test builds: everyone goes for you)
#endif
}

COLD static void place_street(Cit *c, int px, int pz, int cx, int cz, int radius) {   // somewhere out on the streets near (cx,cz) tiles
    for (int tries = 0; tries < 300; tries++) {
        int mx = cx + rn(2 * radius + 1) - radius, my = cz + rn(2 * radius + 1) - radius;
        if (mx < 2 || my < 2 || mx >= MW - 2 || my >= MH - 2) continue;
        int x = mx * TILE + TILE / 2, z = -(my * TILE + TILE / 2);
        if (!open_at(x, z)) continue;
        if (ABS(x - px) + ABS(z - pz) < 4 * TILE) continue;
        c->x = (s16)x; c->z = (s16)z; return;
    }
}
COLD static void place_initial(Cit *c, int px, int pz) {   // where they are right now, by the clock
    const Home *hm = home_of(c);
    c->yaw = (u16)rn(4096); c->mode = CM_IDLE; c->t = (u16)(10 + rn(200)); c->say_t = 0; c->sub = 0; c->st = 0; c->gait.sleep = 0;
    if (!hm) { place_street(c, px, pz, MW / 2, MH / 2, MW / 2 - 3); return; }
    int w = want(c), x, z;
    if (w == 0) place_street(c, px, pz, hm->dx, hm->dy + 3, 8);
    else if (w == 1) { inside_spot(hm, &x, &z); c->x = (s16)x; c->z = (s16)z; }
    else { sleep_spot(c, hm, &x, &z); c->x = (s16)x; c->z = (s16)z; c->mode = CM_SLEEP; c->sub = 1; c->gait.sleep = 1; c->yaw = 1024; }
    if (w == 0 && !c->x) { int a, b, d, e; door_geo(hm, &a, &b, &d, &e); c->x = (s16)a; c->z = (s16)b; }
}

// ---- SAN BOREAS: people brought to life where pop.c says they are -------------------------------------------
COLD static int fam_hh(int f) {                        // the household entry for a pop.c family (made when the first of them turns up)
    for (int k = 0; k < NHH; k++) if (hh[k].pfam == f && hh[k].zone == ZONE_CITY) return k;
    const Fam *fm = &pop_fam[f];
    int home = world_home_find(ZONE_CITY, fm->bx, fm->by, fm->unit); if (home < 0) return -1;
    for (int k = NHH - 1; k >= nhh; k--) if (hh[k].zone == 255) {
        Hh *g = &hh[k]; g->zone = ZONE_CITY; g->home = (u8)home; g->n = 0; g->fam = 0; g->pfam = (u16)f; return k; }
    return -1;
}
COLD static void hh_drop(Cit *c) {                     // out of their household's list (the entry goes when the last one does)
    if (c->hh == NONE) return;
    Hh *g = &hh[c->hh]; int ci = (int)(c - cits);
    for (int k = 0; k < g->n; k++) if (g->mem[k] == ci) { g->mem[k] = g->mem[g->n - 1]; g->n--; break; }
    if (g->pfam != NOPID && !g->n) { g->zone = 255; g->pfam = NOPID; }
    c->hh = NONE;
}
COLD static void main_free(Cit *c) {                   // back into pop.c: what they think of you goes with them
    int i = (int)(c - cits);
    if (c->pid != NOPID) { Per *q = &pop_per[c->pid]; q->rel = c->rel; q->rom = c->rom; if (!(q->flags & PF_GONE) || q->hp) q->hp = MIN(c->hp, 127); q->slot = 255;   // (hp 0 and gone: passed away)
        if (c->flg & F_MET) q->met = 1; if (q->met || q->rel || q->rom) pop_know(c->pid); }   // (who knows you is remembered, and kept on pop.c's list of them)
    hh_drop(c);
    if (cur == i) cur = -1;
    crowd_forget(i);
    c->pid = NOPID; c->mplace = NOPID; c->vanish = 0; c->zone = 255; c->mode = CM_GONE; c->built = 0; c->say_t = 0; c->zone0 = 255; c->owe = 0; c->flg = 0;
}
COLD static int place_tile(const Place *pl, int worker, int *tx, int *ty, int *indoor) {   // somewhere to stand at a place in the window
    int ox = (pl->bx - world_wbx) * BM_BLK + BM_LOT0, oy = (pl->by - world_wby) * BM_BLK + BM_LOT0;
    int want_in = pl->lot != LOT_PARK && pl->lot != LOT_SQUARE && pl->lot != LOT_SKATE && pl->lot != LOT_POOL;
    for (int pass = 0; pass < 3; pass++) for (int t = 0; t < 40; t++) {
        int x = ox + rn(13), y = oy + rn(13);
        if (x < 1 || y < 1 || x >= MW - 1 || y >= MH - 1 || !world_walkable(x, y) || floorMap[y][x] == FL_WATER) continue;
        int in = floorMap[y][x] < FL_GRASS;
        if (pass < 2 && in != want_in) continue;
        if (pass == 0 && worker) {                // at work: by a counter, a desk, a till, the work marker
            static const signed char dx4[4] = { 1, -1, 0, 0 }, dy4[4] = { 0, 0, 1, -1 }; int ok = 0;
            for (int k = 0; k < 4; k++) { char ch = world_at(x + dx4[k], y + dy4[k]); if (ch && strchr("yd[xIF7!]m~`j)", ch)) ok = 1; }
            if (!ok) continue;
        }
        *tx = x; *ty = y; *indoor = in; return 1;
    }
    return 0;
}
int dbg_tf[6];
COLD int citizens_take(int pid, int at, int place) {
    int slot = -1;
    for (int i = MAIN0; i < NCIT; i++) if (cits[i].zone == 255 && cits[i].pid == NOPID) { slot = i; break; }
    int kn = pop_knows_you(pid);
    if (slot < 0 && kn) {                          // every body taken, and this one knows you: a stranger out of sight makes room (back into pop.c, as they are)
        int bd = -1;
        for (int i = MAIN0; i < NCIT; i++) { const Cit *o = &cits[i];
            if (o->zone != ZONE_CITY || o->pid == NOPID || cit_known(o) || i == cur || i == pie_c || o->zone0 != 255) continue;
            if (o->mode != CM_IDLE && o->mode != CM_WALK && o->mode != CM_AWAY && o->mode != CM_SLEEP && o->mode != CM_CHORE) continue;
            if (gfx_visible(o->x, -110, o->z, 160)) continue;
            int dx = (o->x - cam.at.vx) >> 4, dz = (o->z - cam.at.vz) >> 4, d = dx * dx + dz * dz;
            if (d > bd) { bd = d; slot = i; } }
        if (slot >= 0) { main_free(&cits[slot]); CP(14, 1); }
    }
    if (slot < 0) { dbg_tf[0]++; return -1; }
    Per *q = &pop_per[pid]; Cit *c = &cits[slot];
    int px = sims[player].x, pz = sims[player].z;
    memset(&c->gait, 0, sizeof c->gait);
    c->seed = q->seed; c->stage = q->stage; c->rel = q->rel; c->rom = q->rom; c->hp = q->hp ? q->hp : 100;
    c->pers = q->trait == 1 ? PE_GRUFF : q->trait == 6 ? PE_FLIRTY : (q->trait == 8 || q->trait == 13) ? PE_SHY : PE_FRIENDLY;
    c->homebody = (q->flags & PF_HOMEBODY) ? 1 : 0; c->off = pop_offset(pid);
    c->zone = ZONE_CITY; c->zone0 = 255; c->mode = CM_IDLE; c->t = (u16)(10 + rn(150)); c->sub = 0; c->st = 0; c->wv = 0; c->say_t = 0; c->cd = 0; c->slot = 0;
    c->pid = (u16)pid; c->vanish = 0; c->hh = NONE; c->mem = 0; c->built = 0;
    c->flg = (u8)((q->met ? F_MET : 0) | (kn ? F_KN : 0)); c->owe = 0; CP(13, kn);
    crowd_forget(slot);
    if (place < 0) {                               // at home: in their household, and wherever the clock puts them in it
        int k = fam_hh(pop_fam_of(pid)); if (k < 0) { dbg_tf[1]++; c->zone = 255; c->pid = NOPID; c->mode = CM_GONE; return -1; }
        Hh *g = &hh[k]; if (g->n >= 6) { dbg_tf[2]++; c->zone = 255; c->pid = NOPID; c->mode = CM_GONE; return -1; }
        c->hh = (u8)k; c->mem = (u8)(pid - pop_fam[pop_fam_of(pid)].first); g->mem[g->n++] = (u8)slot;
        c->mplace = NOPID; c->x = 0; c->z = 0;
        place_initial(c, px, pz);
        if (!c->x && !c->z) { dbg_tf[3]++; main_free(c); return -1; }
    } else {                                       // out: at work, at school, or out having fun
        int tx, ty, in;
        if (!place_tile(&pop_place[place], at == AT_WORK, &tx, &ty, &in)) { dbg_tf[4]++; c->zone = 255; c->pid = NOPID; c->mode = CM_GONE; return -1; }
        c->x = (s16)(tx * TILE + TILE / 2); c->z = (s16)(-(ty * TILE + TILE / 2)); c->yaw = (u16)rn(4096);
        c->ax = (u8)tx; c->ay = (u8)ty; c->mplace = (u16)place;
        if (at == AT_WORK) c->t = (u16)(200 + rn(400));
    }
    q->slot = (u8)slot;
    return slot;
}
void citizens_release(int slot) { if (slot >= MAIN0 && slot < NCIT && cits[slot].pid != NOPID) cits[slot].vanish = 1; }
int citizens_place_of(int slot) { return (slot >= MAIN0 && slot < NCIT && cits[slot].mplace != NOPID) ? cits[slot].mplace : -1; }
COLD void citizens_shift(int dtx, int dty) {
    for (int i = 0; i < NCIT; i++) {
        Cit *c = &cits[i];
        if (c->zone != ZONE_CITY || c->mode == CM_GONE) continue;
        int x = c->x - dtx * TILE, z = c->z + dty * TILE;
        int ax = c->ax - dtx, ay = c->ay - dty;
        if (x < TILE || -z < TILE || x >= (MW - 1) * TILE || -z >= (MH - 1) * TILE) { if (c->pid != NOPID) { main_free(c); continue; } x = CLAMP(x, TILE, (MW - 2) * TILE); z = CLAMP(z, -(MH - 2) * TILE, -TILE); }
        c->x = (s16)x; c->z = (s16)z; c->ax = (u8)CLAMP(ax, 0, MW - 1); c->ay = (u8)CLAMP(ay, 0, MH - 1);
        c->wv = 0;                                 // routes and errands are worked out again
        if (c->mode == CM_CHORE || c->mode == CM_VISIT || c->mode == CM_GOHOME || c->mode == CM_LEAVE) { c->mode = CM_IDLE; c->t = 20; c->gait.sit = 0; }
        if (c->mode == CM_SLEEP && c->sub == 3) c->sub = 0;      // (the way to bed: the bed is found again)
        if (c->mode == CM_GREET || c->mode == CM_BUY) { c->mode = CM_IDLE; c->t = 20; }   // (a customer's stand has moved with the map: the next one comes)
    }
    for (int k = 0; k < NHH; k++) {               // the households' homes have new numbers
        Hh *g = &hh[k]; if (g->zone != ZONE_CITY || g->pfam == NOPID) continue;
        const Fam *fm = &pop_fam[g->pfam];
        int home = world_home_find(ZONE_CITY, fm->bx, fm->by, fm->unit);
        if (home >= 0) { g->home = (u8)home; continue; }
        for (int j = g->n - 1; j >= 0; j--) main_free(&cits[g->mem[j]]);   // their home left the window
    }
    if (party_on && world_zone == ZONE_CITY) {      // a party moves with the map, or ends if it fell off it
        party_cx -= dtx; party_cy -= dty; party_spx -= dtx * TILE; party_spz += dty * TILE;
        for (int k = 0; k < PARTY_MAX; k++) { party_sx[k] = (u8)(party_sx[k] - dtx); party_sy[k] = (u8)(party_sy[k] - dty); }
        if (party_cx < 2 || party_cy < 2 || party_cx >= MW - 2 || party_cy >= MH - 2) party_left = 1;
    }
}
COLD static void vanish_tick(const u8 *lv, int nlv) {   // the ones wanted elsewhere go, once nobody can see them (lv: the live list)
    for (int j = 0; j < nlv; j++) {
        int i = lv[j]; if (i < MAIN0) continue;
        Cit *c = &cits[i];
        if (!c->vanish || c->zone != ZONE_CITY || c->mode == CM_PARTY || c->mode == CM_CHAT || c->mode == CM_CHASE || i == pie_c) continue;
        int dx = c->x - cam.at.vx, dz = c->z - cam.at.vz;
        if (!gfx_visible(c->x, -110, c->z, 160) || dx * dx + dz * dz > 16 * TILE * 16 * TILE) { main_free(c); continue; }
        if ((c->mode == CM_IDLE || c->mode == CM_WALK) && !indoors_at(c->x, c->z)) {   // seen, and wanted elsewhere: they walk off down the street rather than blink out
            const Sim *p = &sims[player]; int px = p->x - c->x, pz = p->z - c->z, d = ABS(px) + ABS(pz) + 1;
            int tx = c->x / TILE - px * 24 / d * 1, ty = (-c->z) / TILE + pz * 24 / d * 1;   // (away from you, a long way)
            c->ax = (u8)CLAMP(tx, 2, MW - 3); c->ay = (u8)CLAMP(ty, 2, MH - 3); c->mode = CM_AWAY; c->t = 1200; c->wv = 0;
        }
    }
}

COLD void citizens_enter(int zone) {
    int px = sims[player].x, pz = sims[player].z;
    if (party_on || home_guests) {   // travelling ends a party: the guests go back where they came from
        party_on = 0;
        for (int i = 0; i < NCIT; i++) if (cits[i].mode == CM_PARTY || cits[i].zone0 != 255) party_leave_done(&cits[i]);
        home_guests = 0;
    }
    form_households(zone);
    for (int i = MAIN0; i < NCIT; i++) if (cits[i].pid != NOPID) main_free(&cits[i]);   // SAN BOREAS's people are brought in again by pop.c, where they are now
    for (int i = 0; i < NCIT; i++) {
        Cit *c = &cits[i];
        c->slot = 0;
        if (c->zone != zone || c->mode == CM_GONE) continue;
        if (c->hp < 30) c->hp = 30;
        c->flg &= F_MET | F_KN; c->owe = 0;   // (a new visit: they may come over to say hello again)
        c->x = 0; c->z = 0;
        place_initial(c, px, pz);
    }
    fight_on = 0;
}

// ---- who is in front of you ---------------------------------------------------------------------------
COLD static int find_target_l(const u8 *lv, int nlv) {   // (lv: the frame's live list, or 0 for every slot)
    const Sim *p = &sims[player]; int best = -1, bd = 1 << 30;
    for (int j = 0; j < (lv ? nlv : NCIT); j++) {
        int i = lv ? lv[j] : j; const Cit *c = &cits[i];
        if (c->zone != world_zone || c->mode == CM_GONE || c->mode == CM_KO || !c->built) continue;
        int dx = c->x - p->x, dz = c->z - p->z;
        if (ABS(dx) > REACH || ABS(dz) > REACH) continue;
        int d = dx * dx + dz * dz;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
static int find_target(void) { return find_target_l(0, 0); }
static int hostile_in_reach(void) {
    const Sim *p = &sims[player]; int best = -1, bd = 1 << 30;
    for (int i = 0; i < NCIT; i++) {
        const Cit *c = &cits[i];
        if (c->zone != world_zone || c->mode != CM_CHASE) continue;
        int dx = c->x - p->x, dz = c->z - p->z;
        if (ABS(dx) > PUNCH_R || ABS(dz) > PUNCH_R) continue;
        int d = dx * dx + dz * dz;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
static const char *rel_name(int r) { return r < -40 ? "ENEMY" : r < -10 ? "UNFRIENDLY" : r < 20 ? "STRANGER" : r < 50 ? "PAL" : r < 75 ? "FRIEND" : "CLOSE"; }
const char *citizens_hint(void) {
    if (hostile_in_reach() >= 0 || fam_in_reach() >= 0) return "PUNCH";
    if (cur < 0) return "";
    return cits[cur].mode == CM_SLEEP ? "WAKE" : "TALK";
}

// ---- the pie menu -------------------------------------------------------------------------------------
const char *citizens_pie_title(void) { return pie_title; }
static int pie_lvl;   // 0 the top; 1 CHAT, 2 FRIENDLY, 3 ROMANCE, 4 MEAN (each ends in BACK)
COLD static int pie_build(const Cit *c, char opt[8][24]) {
    int n = 0, ad = adult(c) && pl_adult();
    #define OPT(txt, code) { strcpy(opt[n], txt); pie_code[n++] = (code); }
    if (c->mode == CM_CHASE) { OPT("SORRY!", PO_SORRY) return n; }
    if (c->mode == CM_SLEEP) { OPT("WAKE THEM UP", PO_TALK) return n; }
    switch (pie_lvl) {
    case 0:
        OPT("TALK", PO_TALK) OPT("CHAT...", PO_SUB_CHAT) OPT("FRIENDLY...", PO_SUB_SOC)
        if (ad) OPT("ROMANCE...", PO_SUB_ROM)
        OPT("MEAN...", PO_SUB_MEAN)
        if (c->hh != NONE) OPT("FAMILY", PO_FAMILY)
        break;
    case 1:
        OPT("JOKE", PO_JOKE) OPT("ASK ABOUT JOB", PO_JOB) OPT("GOSSIP", PO_GOSSIP) OPT("ASK ADVICE", PO_ADVICE) OPT("COMPLIMENT", PO_COMPL)
        if (c->hh != NONE) OPT("FAMILY", PO_FAMILY)
        OPT("BACK", PO_BACK)
        break;
    case 2:
        if (c->rel >= 25) OPT("HUG", PO_HUG)
        OPT("HIGH FIVE", PO_HIGH5) OPT("DANCE", PO_DANCE) OPT("BUY DRINK $10", PO_DRINK) OPT("GIVE $20 GIFT", PO_GIFT)
        if (c->rel < 20) OPT("SAY SORRY", PO_APOL)
        if (ad && c->rel >= 60 && c->rom < 60) OPT("ASK TO MOVE IN", PO_MOVEIN)   // (a good friend: borrowed from BORE on the GBA)
        OPT("BACK", PO_BACK)
        break;
    case 3:
        if (c->rel >= 30) OPT("FLIRT", PO_FLIRT)
        OPT("FLOWERS $40", PO_FLOWERS)
        if (c->rel >= 40 && c->rom >= 30) OPT("GO ON A DATE $60", PO_DATE)
        if (c->rel >= 40 && c->rom >= 40) OPT("KISS", PO_KISS)
        if (c->rom >= 60 && c->rel >= 60) OPT("PROPOSE $150", PO_PROPOSE)
        OPT("BACK", PO_BACK)
        break;
    default:
        OPT("INSULT", PO_INSULT) OPT("PRANK", PO_PRANK)
        if (ad) OPT("FIGHT", PO_FIGHT)
        OPT("BACK", PO_BACK)
        break;
    }
    #undef OPT
    return n;
}
COLD int citizens_pie_open(char opt[8][24]) {
    int i = find_target(); if (i < 0) return 0;
    Cit *c = &cits[i]; pie_c = i; pie_lvl = 0; meet(c);
    strcpy(pie_title, c->first);
    return pie_build(c, opt);
}
void citizens_door_marks(void) { for (int i = 0; i < NCIT; i++) { const Cit *c = &cits[i]; if (c->zone != world_zone || c->mode == CM_GONE || c->mode == CM_KO) continue; world_door_mark(c->x / TILE, (-c->z) / TILE); } }
COLD int citizens_pie_more(char opt[8][24]) { return pie_c < 0 ? 0 : pie_build(&cits[pie_c], opt); }   // a sub-menu: the pie again, with the new choices

static int accept(const Cit *c, int base) {   // the dice: their mood towards you, their nature
    int ch = base + c->rel / 2 + sims[player].mood / 4;
    if (c->pers == PE_FRIENDLY) ch += 12; else if (c->pers == PE_GRUFF) ch -= 18;
    return rn(100) < ch;
}
static void rel_add(Cit *c, int d) { c->rel = (s8)CLAMP(c->rel + d, -100, 100); }
static void rom_add(Cit *c, int d) { c->rom = (s8)CLAMP(c->rom + d, 0, 100); }
static void chat_begin(Cit *c) {
    c->mode = CM_CHAT; c->t = 240;
    c->gait.wave = 40;
    c->yaw = (u16)yaw_to(sims[player].x - c->x, sims[player].z - c->z);
}

COLD static void marry_as(Cit *c, int wedding) {   // they move in: the civilian becomes a housemate with the full model (a wedding: as your partner)
    Bore *wedp = crowd_temp_bore();
    #define wed (*wedp)
    memset(&wed, 0, sizeof wed);
    cit_bore(&wed, c);        // fresh face textures (the pool's are shared), the same person from the same seed
    const Sim *p = &sims[player];
    int tx = p->x / TILE, ty = (-p->z) / TILE;
    sim_add(&wed, tx, ty, 1);
    Sim *w = &sims[nsims - 1];
    w->rel[player] = wedding ? 100 : (s8)MAX(60, c->rel); sims[player].rel[nsims - 1] = w->rel[player];
    if (wedding) { w->partner = sims[player].uid; sims[player].partner = w->uid; }   // (the family tree: partners)
    w->x = p->x + 70; w->z = p->z;
    c->mode = CM_GONE; c->say_t = 0;
    { int ci = (int)(c - cits); if (c->pid == NOPID && ci < MAIN0) moved_mask[ci >> 3] |= (u8)(1u << (ci & 7)); }
    if (c->pid != NOPID) { pop_per[c->pid].flags |= PF_GONE; main_free(c); }
    char b[40]; if (wedding) sprintf(b, "MARRIED %s!", w->bore.first); else sprintf(b, "%s MOVED IN WITH YOU!", w->bore.first); sim_note(b);
    audio_voice(wedding ? VOICE_WIN : VOICE_MOVE_YES);
    #undef wed
}
static void marry(Cit *c) { marry_as(c, 1); }

static const char *const L_GOSS[] = { "THE MAYOR NAPS AT WORK", "SOMEONE'S DOG ATE A SHOE", "THE DINER CHANGED ITS SOUP", "I HEARD A FIGHT DOWNTOWN", "THEY SAY THE PARK IS HAUNTED", "A NEW SHOP IS OPENING", "THE BUS DRIVER QUIT AGAIN" };
static const char *const L_ADV[] = { "DRINK MORE WATER", "SLEEP. JUST SLEEP.", "BE NICE TO PEOPLE", "SAVE SOME CASH", "CALL YOUR FRIENDS", "EAT A REAL MEAL", "GO FOR A WALK" };
COLD int citizens_pie_do(int i) {
    if (pie_c < 0 || i < 0) return 0;
    Cit *c = &cits[pie_c];
    if (c->mode == CM_GONE || c->mode == CM_KO) return 0;
    Sim *p = &sims[player];
    if (c->mode == CM_SLEEP) {   // you woke them up
        c->mode = CM_IDLE; c->t = 60; c->gait.sleep = 0; c->sub = 0; rel_add(c, -6);
        say(c, "HUH? WHAT?"); sim_moodlet(p, CML_REJECTED); audio_voice(VOICE_GROAN); return 0;
    }
    switch (pie_code[i]) {
    case PO_SUB_CHAT: pie_lvl = 1; return 1;
    case PO_SUB_SOC:  pie_lvl = 2; return 1;
    case PO_SUB_ROM:  pie_lvl = 3; return 1;
    case PO_SUB_MEAN: pie_lvl = 4; return 1;
    case PO_BACK:     pie_lvl = 0; return 1;
    case PO_COMPL:
        if (accept(c, 50)) { rel_add(c, 6); say(c, "AWW, YOU'RE SWEET"); chat_begin(c); sim_moodlet(p, CML_GOOD_CHAT); audio_voice(VOICE_AGREE); }
        else { rel_add(c, -1); say(c, "OKAY...?"); audio_voice(VOICE_DISAGREE); }
        break;
    case PO_GOSSIP:
        story_talks++;
        if (accept(c, 40)) { rel_add(c, 4); say(c, L_GOSS[rn(NL(L_GOSS))]); chat_begin(c); story_event(E_CITCHAT, 0); audio_voice(VOICE_AGREE); }
        else { say(c, "NOT MY BUSINESS"); audio_voice(VOICE_DISAGREE); }
        break;
    case PO_ADVICE:
        if (accept(c, 40)) { rel_add(c, 3); say(c, L_ADV[rn(NL(L_ADV))]); chat_begin(c); sim_moodlet(p, CML_GOOD_CHAT); audio_voice(VOICE_AGREE); }
        else { say(c, "ASK SOMEONE ELSE"); audio_voice(VOICE_DISAGREE); }
        break;
    case PO_JOB: {
        static char jb[40];
        if (c->pid != NOPID && accept(c, 50)) { pop_job_name(c->pid, jb); say(c, jb); rel_add(c, 3); chat_begin(c); audio_voice(VOICE_AGREE); }
        else { say(c, c->pid != NOPID ? "WHY DO YOU ASK" : "JUST GETTING BY"); audio_voice(VOICE_DISAGREE); }
        break; }
    case PO_HIGH5:
        if (accept(c, 55)) { rel_add(c, 5); say(c, "HIGH FIVE!"); chat_begin(c); sim_moodlet(p, CML_LAUGH); audio_voice(VOICE_YEHA); }
        else { rel_add(c, -2); say(c, "NAH"); audio_voice(VOICE_DISAGREE); }
        break;
    case PO_DANCE:
        if (accept(c, 40)) { rel_add(c, 7); say(c, "WOO! LET'S GO!"); chat_begin(c); c->gait.wave = 90; sim_moodlet(p, CML_LAUGH); audio_voice(VOICE_YAHOO); }
        else { rel_add(c, -3); say(c, "I DON'T DANCE"); sim_moodlet(p, CML_EMBARRASSED); audio_voice(VOICE_DISAGREE); }
        break;
    case PO_DRINK:
        if (cash < 10) { sim_note("A DRINK IS $10"); audio_voice(VOICE_NAH); break; }
        cash -= 10; rel_add(c, 6); say(c, "CHEERS!"); chat_begin(c); sim_moodlet(p, CML_GOOD_CHAT); audio_voice(VOICE_YAHOO); break;
    case PO_APOL:
        if (accept(c, 30)) { rel_add(c, 12); say(c, "ALL RIGHT. WE'RE OKAY"); chat_begin(c); audio_voice(VOICE_AGREE); }
        else { say(c, "NOT GOOD ENOUGH"); audio_voice(VOICE_DISAGREE); }
        break;
    case PO_FLOWERS:
        if (cash < 40) { sim_note("FLOWERS ARE $40"); audio_voice(VOICE_NAH); break; }
        cash -= 40; rel_add(c, 6); rom_add(c, c->pers == PE_SHY ? 5 : 8); say(c, "FLOWERS! FOR ME?"); chat_begin(c); sim_moodlet(p, CML_FLIRTY); audio_voice(VOICE_FLIRT); break;
    case PO_DATE:
        if (cash < 60) { sim_note("A DATE IS $60"); audio_voice(VOICE_NAH); break; }
        if (accept(c, 25 + c->rom / 3)) { cash -= 60; rom_add(c, 14); rel_add(c, 8); say(c, "THAT WAS LOVELY"); chat_begin(c); sim_moodlet(p, CML_FLIRTY); audio_voice(VOICE_FLIRT); }
        else { rom_add(c, -4); say(c, "MAYBE LATER"); sim_moodlet(p, CML_REJECTED); audio_voice(VOICE_DISAGREE); }
        break;
    case PO_KISS:
        if (accept(c, 10 + c->rom / 2)) { rom_add(c, 10); rel_add(c, 5); say(c, "MMM..."); chat_begin(c); sim_moodlet(p, CML_FLIRTY); audio_voice(VOICE_FLIRT); }
        else { rom_add(c, -8); rel_add(c, -8); say(c, "WHOA! NO!"); sim_moodlet(p, CML_EMBARRASSED); audio_voice(VOICE_DISAGREE); }
        break;
    case PO_INSULT:
        rel_add(c, -12); say(c, c->pers == PE_GRUFF ? "SAY THAT AGAIN" : "WOW. RUDE."); audio_voice(VOICE_DISAGREE); sim_moodlet(p, CML_REJECTED);
        if (c->pers == PE_GRUFF && rn(100) < 30) { c->mode = CM_CHASE; c->hp = (u8)(55 + rn(35)); c->cd = 30; c->t = 0; fight_on = 1; heat_add(8); crowd_react(1); audio_voice(VOICE_ANGRY); story_event(E_FIGHT, 0); }
        break;
    case PO_PRANK:
        if (rn(100) < 45) { rel_add(c, 4); say(c, "HA! YOU GOT ME"); chat_begin(c); sim_moodlet(p, CML_LAUGH); audio_voice(VOICE_LAUGH); }
        else { rel_add(c, -9); say(c, "NOT FUNNY!"); sim_moodlet(p, CML_REJECTED); audio_voice(VOICE_ANGRY); }
        break;
    case PO_TALK:
        story_talks++;
        if (c->pers == PE_GRUFF && c->rel < 10 && rn(100) < 45) { say(c, "WHAT."); rel_add(c, -2); audio_voice(VOICE_DISAGREE); break; }
        rel_add(c, c->pers == PE_FRIENDLY ? 8 : 5); say(c, chat_line(c)); chat_begin(c); story_event(E_CITCHAT, 0);
        sim_moodlet(p, CML_GOOD_CHAT); audio_voice(VOICE_AGREE); break;
    case PO_FAMILY: {   // who do they live with?
        static char fam[32];
        const Hh *g = c->hh == NONE ? 0 : &hh[c->hh];
        if (!g || !accept(c, 55)) { rel_add(c, -2); say(c, "NONE OF YOUR BUSINESS"); audio_voice(VOICE_DISAGREE); break; }
        rel_add(c, 3);
        if (g->n <= 1) strcpy(fam, "I LIVE ALONE");
        else {
            const Cit *o = &cits[g->mem[(c->mem + 1) % g->n]];
            const char *nm = o->first[0] == '.' ? "SOMEONE" : o->first;
            if (g->n > 2) snprintf(fam, sizeof fam, "I LIVE WITH %s +%d", nm, g->n - 2);
            else snprintf(fam, sizeof fam, "I LIVE WITH %s", nm);
        }
        say(c, fam); chat_begin(c); audio_voice(VOICE_AGREE);
        break; }
    case PO_JOKE:
        story_talks++;
        if (accept(c, 45)) { rel_add(c, 9); say(c, "HAHAHA!"); chat_begin(c); story_event(E_JOKE, 0); story_event(E_CITCHAT, 0); sim_moodlet(p, CML_LAUGH); audio_voice(VOICE_LAUGH); }
        else { rel_add(c, -4); say(c, "NOT FUNNY."); sim_moodlet(p, CML_REJECTED); audio_voice(VOICE_DISAGREE); }
        break;
    case PO_HUG:
        if (accept(c, 40)) { rel_add(c, 8); rom_add(c, 3); say(c, "AWW"); chat_begin(c); story_event(E_HUG, 0); story_event(E_CITCHAT, 0); sim_moodlet(p, CML_HUGGED); audio_voice(VOICE_AGREE); }
        else { rel_add(c, -6); say(c, "PERSONAL SPACE!"); sim_moodlet(p, CML_REJECTED); audio_voice(VOICE_DISAGREE); }
        break;
    case PO_FLIRT: {
        int bonus = c->pers == PE_FLIRTY ? 25 : c->pers == PE_SHY ? -15 : 0;
        if (accept(c, 20 + bonus)) { rom_add(c, 12); rel_add(c, 4); say(c, "OH, STOP IT"); chat_begin(c); sim_moodlet(p, CML_FLIRTY); audio_voice(VOICE_FLIRT); }
        else { rom_add(c, -5); rel_add(c, -6); say(c, "UH... NO."); sim_moodlet(p, CML_EMBARRASSED); audio_voice(VOICE_DISAGREE); }
        break; }
    case PO_GIFT:
        if (cash < GIFT_COST) { sim_note("A GIFT IS $20"); audio_voice(VOICE_NAH); break; }
        cash -= GIFT_COST; rel_add(c, 10); say(c, "FOR ME? THANKS!"); chat_begin(c); audio_voice(VOICE_YAHOO); break;
    case PO_PROPOSE:
        if (nsims >= MAX_SIMS) { sim_note("THE HOUSE IS TOO FULL FOR ONE MORE"); audio_voice(VOICE_NAH); break; }
        if (cash < RING_COST) { sim_note("A RING IS $150"); audio_voice(VOICE_NAH); break; }
        {   int ch = 55 + c->rel / 4 + c->rom / 4 + (c->pers == PE_FLIRTY ? 10 : c->pers == PE_SHY ? -15 : 0);
            if (rn(100) < ch) { cash -= RING_COST; marry(c); }
            else { rom_add(c, -25); rel_add(c, -10); say(c, "I'M NOT READY..."); sim_moodlet(p, CML_REJECTED); audio_voice(VOICE_CRY); } }
        break;
    case PO_FIGHT:
        c->mode = CM_CHASE; c->hp = (u8)(c->stage == AG_ELDER ? 35 : 55 + rn(35)); c->cd = 30; c->t = 0;
        rel_add(c, -10); rom_add(c, -20); say(c, "BRING IT!"); fight_on = 1; heat_add(12); crowd_react(1); audio_voice(VOICE_ANGRY); story_event(E_FIGHT, 0); break;
    case PO_MOVEIN:   // a friend moves in with you: they say yes when they like you enough (and there is room)
        audio_voice(VOICE_MOVE_ASK);
        if (nsims >= MAX_SIMS) { say(c, "YOUR PLACE IS FULL!"); sim_note("THE HOUSE IS TOO FULL FOR ONE MORE"); break; }
        if (rn(100) < 30 + c->rel / 2 + (c->pers == PE_FRIENDLY ? 15 : c->pers == PE_GRUFF ? -20 : 0)) { say(c, "I'D LOVE TO!"); marry_as(c, 0); }
        else { rel_add(c, -4); say(c, "I LIKE MY OWN PLACE..."); sim_moodlet(p, CML_REJECTED); audio_voice(VOICE_MOVE_NO); }
        break;
    case PO_SORRY:
        if (rn(100) < 55) { c->mode = CM_IDLE; c->t = 60; say(c, "...FINE."); }
        else { say(c, "TOO LATE!"); audio_voice(VOICE_DISAGREE); }
        break;
    }
    return 0;
}

COLD int citizens_friends(void) {   // (stories) the people out there who are your friends (50 and up): the ones about now, and the rest of SAN BOREAS
    int n = 0;
    for (int i = 0; i < NCIT; i++) { const Cit *c = &cits[i]; if (c->rel < 50) continue;
        if (c->pid != NOPID ? pop_per[c->pid].slot == i : (i < MAIN0 && c->mode != CM_GONE)) n++; }   // (a city one alive now: its live feelings, not the saved ones)
    for (int p = 0; p < pop_nper; p++) { const Per *q = &pop_per[p]; if (q->rel >= 50 && q->slot == 255 && !(q->flags & PF_GONE)) n++; }
    return n;
}
// ---- fighting ---------------------------------------------------------------------------------------------
// THE HEAT of a fight (0..100) rises with every blow and every K.O. and cools when it is over. Each half second the people who can
// see it make up their minds: some JOIN IN (by AGGRESSION, the cheat slider: at 100% everyone in sight piles on the moment a fight
// starts), some come to STOP YOU (the peacemakers: hit someone in front of them and they may turn on you), and a hot fight calls
// the POLICE. RAGE (the other slider) needs no fight at all: at 100% anyone who sees you goes for you. WHO says who may join in
// or lunge: civilians, the police, your own family, or a mix.
int dbg_rage[4];
int aggro_pct = 40, rage_pct, aggro_who = 7, rage_who = 1, peace_on = 1;   // (main.c's options set these)
static void heat_add(int v) { heat = MIN(100, heat + v); }
static const u8 whoBits[6] = { 7, 1, 2, 4, 3, 6 };   // EVERYONE, CIVILIANS, POLICE, FAMILY, CIVILIANS + POLICE, FAMILY + POLICE
static int fam_hp[MAX_SIMS], fam_cd[MAX_SIMS], fam_ko[MAX_SIMS];
static void fam_calm(void) { for (int i = 0; i < nsims; i++) sims[i].rage = 0; }
static int fam_in_reach(void) {
    const Sim *p = &sims[player];
    for (int i = 0; i < nsims; i++) { const Sim *s = &sims[i]; if (i == player || !s->rage || s->lvl != p->lvl) continue;
        if (ABS(s->x - p->x) <= PUNCH_R && ABS(s->z - p->z) <= PUNCH_R) return i; }
    return -1;
}
COLD static void fam_go(int i) {   // one of the family turns on you
    Sim *s = &sims[i]; if (s->rage || fam_ko[i] || s->bore.stage < AG_TEEN || s->away || s->lvl != sims[player].lvl) return;
    s->rage = 1; fam_hp[i] = 70; fam_cd[i] = 40; s->action = ACT_NONE; s->path_n = 0;
    static const char *const fl[4] = { "YOU'RE GROUNDED!", "NOT IN THIS FAMILY!", "I'VE HAD IT WITH YOU!", "COME HERE!" };
    sim_say_to(s, fl[rn(4)]); audio_voice(VOICE_ANGRY);
}
COLD static int fam_punch(void) {
    int i = fam_in_reach(); if (i < 0) return 0;
    Sim *p = &sims[player], *s = &sims[i];
    if (pl_cd > 0) return 1;
    pl_cd = 18; p->gait.wave = 14; p->yaw = yaw_to(s->x - p->x, s->z - p->z);
    fam_hp[i] -= tr_scale(p->tr, E_FIGHT_DMG, 9 + rn(8) + (dojo_belt(p) + tr_add(p->tr, E_FIGHT_BELT)) * 2); s->gait.squash = -12; heat_add(5); audio_voice(VOICE_SHRIEK);
    if (fam_hp[i] <= 0) { s->rage = 0; s->rage_ko = 1; fam_ko[i] = 600; s->gait.sleep = 1; s->gait.amp = 0; sim_say_to(s, "I'M TELLING MOM..."); audio_voice(VOICE_CRY); }
    else sim_say_to(s, rn(2) ? "OW!" : "HOW COULD YOU!");
    return 1;
}
COLD static void fam_step(void) {   // the family members who are after you (sim.c leaves them be while they are)
    Sim *p = &sims[player];
    for (int i = 0; i < nsims; i++) {
        Sim *s = &sims[i]; if (i == player) continue;
        if (fam_ko[i]) { if (--fam_ko[i] == 0) { s->gait.sleep = 0; s->rage_ko = 0; sim_say_to(s, "...FINE. I FORGIVE YOU."); } else s->gait.sleep = 1; continue; }
        if (!s->rage) continue;
        int dx = p->x - s->x, dz = p->z - s->z, d2 = dx * dx + dz * dz, in = indoors_at(s->x, s->z);
        if (d2 > 9 * TILE * 9 * TILE || s->lvl != p->lvl) { s->rage = 0; sim_say_to(s, "HMPH."); continue; }
        s->yaw = yaw_to(dx, dz);
        if (d2 > 95 * 95) { int mx = (isin(s->yaw) * 6) >> 12, mz = -((icos(s->yaw) * 6) >> 12);
            if (body_free(s->x + mx, s->z + mz, in)) { s->x += mx; s->z += mz; } else if (body_free(s->x + mx, s->z, in)) s->x += mx; else if (body_free(s->x, s->z + mz, in)) s->z += mz;
            s->gait.amp = 4096; s->gait.run = 1; s->gait.phase = (s->gait.phase + 220) & 4095; }
        else { s->gait.amp = 0; if (fam_cd[i] > 0) fam_cd[i]--; else {
            pl_hp -= tr_scale(p->tr, E_FIGHT_DEF, 3 + rn(5)); fam_cd[i] = 45; s->gait.wave = 14; p->gait.squash = -10; audio_voice(VOICE_GROAN);
            if (pl_hp <= 0) { pl_hp = 60; fam_calm(); heat = 0; for (int k = 0; k < NCIT; k++) if (cits[k].mode == CM_CHASE || cits[k].mode == CM_STOP) { cits[k].mode = CM_IDLE; cits[k].t = 120; }
                sim_note("KNOCKED OUT BY FAMILY. AWKWARD."); sim_say_to(s, "AND STAY OUT OF MY ROOM!"); audio_voice(VOICE_CRY); } } }
    }
}
COLD static void join_in(Cit *c) {
    static const char *const jl[5] = { "FIGHT! FIGHT!", "GET THEM!", "LEAVE THEM ALONE!", "MY TURN!", "YOU WANT SOME TOO?" };
    c->mode = CM_CHASE; c->hp = (u8)(c->cop ? 120 : c->stage == AG_ELDER ? 35 : 55 + rn(35)); c->cd = 20 + rn(30); c->t = 0;
    say(c, c->cop ? (rn(2) ? "POLICE! FREEZE!" : "DROP IT, BORE!") : jl[rn(5)]); rel_add(c, -8); meet(c);
}
COLD static void call_cop(void) {   // the police: an officer arrives from up the street
    Sim *p = &sims[player]; int best = -1, bd = 0;
    for (int i = 0; i < NCIT; i++) { Cit *c = &cits[i];
        if (c->zone != world_zone || c->cop || !adult(c) || (c->mode != CM_IDLE && c->mode != CM_WALK)) continue;
        int d = ABS(c->x - p->x) + ABS(c->z - p->z); if (d > bd && d < 14 * TILE) { bd = d; best = i; } }
    if (best < 0) return;
    Cit *c = &cits[best]; c->cop = 1; c->built = 0;   // (dressed again: the uniform)
    join_in(c); sim_note("SOMEONE CALLED THE POLICE!"); audio_voice(VOICE_SHRIEK);
}
COLD static void crowd_react(int start) {   // the people who can see you make up their minds (start: a fight has just begun)
    Sim *p = &sims[player];
    if (crime_total() && crime_evid >= 30 && rn(1000) < crime_evid / 3 + (int)crime_total()) call_cop();   // (a witness phones in what you did)
    if (!fight_on && !start) { if (heat > 0) heat--; }
    int ab = whoBits[aggro_who % 6], rb = whoBits[rage_who % 6], cops = 0;
    for (int i = 0; i < NCIT; i++) if (cits[i].zone == world_zone && cits[i].cop && cits[i].mode == CM_CHASE) cops++;
    for (int i = 0; i < NCIT; i++) {
        Cit *c = &cits[i];
        if (c->zone != world_zone || c->stage < AG_TEEN || (c->mode != CM_IDLE && c->mode != CM_WALK && c->mode != CM_CHAT && c->mode != CM_CHORE && c->mode != CM_PARTY && c->mode != CM_VISIT)) continue;
        int dx = c->x - p->x, dz = c->z - p->z, d2 = dx * dx + dz * dz;
        if (d2 > 8 * TILE * 8 * TILE) continue;
        int kind = c->cop ? 2 : 1;
        if (rage_pct && (rb & kind) && d2 < 6 * TILE * 6 * TILE && rn(100) < (rage_pct >= 100 ? 100 : rage_pct / 3)) { join_in(c); if (!c->cop) say(c, "YOU! YES, YOU!"); heat_add(4); continue; }
        if (!fight_on && !start) continue;
        int ch = aggro_pct >= 100 ? 100 : heat * aggro_pct / 150;
        if ((ab & kind) && rn(100) < ch) { join_in(c); heat_add(3); continue; }
        if (peace_on && !c->cop && heat < 80 && rn(100) < 6 + (start ? 10 : 0)) { c->mode = CM_STOP; c->t = 400; say(c, "HEY! STOP THAT!"); meet(c); }
    }
    if (fight_on || start) {   // a hot fight brings the police; a cool one may be talked down
        if ((ab & 2) && heat >= 35 && cops < 1 + heat / 35 && rn(100) < 20 + aggro_pct / 2) call_cop();
        if (ab & 4) for (int i = 0; i < nsims; i++) if (i != player && (aggro_pct >= 100 || rn(100) < heat * aggro_pct / 250)) fam_go(i);
        if (heat < 40) { int peace = 0; for (int i = 0; i < NCIT; i++) if (cits[i].zone == world_zone && cits[i].mode == CM_STOP) peace++;
            if (peace && rn(100) < 10 * peace) for (int i = 0; i < NCIT; i++) { Cit *c = &cits[i]; if (c->zone == world_zone && c->mode == CM_CHASE && !c->cop) { c->mode = CM_IDLE; c->t = 90; say(c, "FINE. WHATEVER."); break; } } }
    }
    if (rage_pct && (rb & 2) && cops == 0 && rn(1000) < rage_pct) call_cop();
    if (rage_pct && (rb & 4)) for (int i = 0; i < nsims; i++) if (i != player && rn(100) < (rage_pct >= 100 ? 100 : rage_pct / 6)) fam_go(i);
}
COLD int citizens_punch(void) {
    int i = hostile_in_reach(); if (i < 0) return fam_punch();
    Cit *c = &cits[i]; Sim *p = &sims[player]; meet(c);
    if (pl_cd > 0) return 1;
    pl_cd = 18;
    p->gait.wave = 14;
    int dx = c->x - p->x, dz = c->z - p->z;
    p->yaw = yaw_to(dx, dz);
    int dmg = tr_scale(p->tr, E_FIGHT_DMG, 9 + rn(8) + (dojo_belt(p) + tr_add(p->tr, E_FIGHT_BELT)) * 2);   // (the dojo belt: a black belt hits a good deal harder)
    c->hp = (u8)MAX((int)c->hp - dmg, 0);
    c->gait.squash = -12;
    { int d = MAX(ABS(dx) + ABS(dz), 1), kx = dx * 40 / d, kz = dz * 40 / d, in = indoors_at(c->x, c->z);   // a shove back
      if (body_free(c->x + kx, c->z + kz, in)) { c->x += (s16)kx; c->z += (s16)kz; } }
    audio_voice(VOICE_SHRIEK);
    if (c->mode == CM_CHASE && !c->cop) { crime_def++; if (crime_def > 9) crime_def = 9; } else { sim_crime(c->cop ? CR_RESIST : CR_ASSAULT, 12); }   // (hitting someone who is hitting you is self-defence)
    heat_add(6);
    for (int k = 0; k < NCIT; k++) { Cit *q = &cits[k]; if (q->mode != CM_STOP || q->zone != world_zone) continue;   // (you hit someone right in front of a peacemaker)
        int ex = q->x - p->x, ez = q->z - p->z; if (ex * ex + ez * ez < 3 * TILE * 3 * TILE && rn(100) < 30 + heat / 2) { q->mode = CM_CHASE; q->hp = (u8)(55 + rn(35)); q->cd = 30; say(q, "THAT'S IT!"); heat_add(10); } }
    if (c->hp == 0) {
        c->mode = CM_KO; c->t = 900; c->gait.sleep = 1; c->gait.amp = 0;
        int loot = tr_scale(p->tr, E_LOOT, 5 + rn(21)); cash += loot;
        char b[32]; sprintf(b, "KO! +$%d", loot); sim_note(b);
        rel_add(c, -30); say(c, "OOF..."); audio_voice(VOICE_WIN); heat_add(15);
        if (hostile_in_reach() < 0) fight_on = 0;
    } else say(c, rn(2) ? "OW!" : "MY NOSE!");
    return 1;
}

// ---- walking: round things, through a door, to a bed ------------------------------------------------------
#ifdef CIT_TEST
int cit_snaps, cit_go; int cit_by[12][4];
#endif
// THE SCHEDULER lets one call live several steps (n) at once (someone far off, thought about every few frames): a walk
// covers the ground of n steps in one move where the way is clear (never past the point it walks to), timers count down by n.
// g_used says how many steps the call took; what is left over is lived on the next call, so nothing is lost, only grouped.
static int g_used;
static int stride_free(const Cit *c, int mx, int mz, int k, int in) {   // a stride of k steps: clear at its end and every 24 units along it (no corner a single step could not get past)
    int len = (ABS(mx) + ABS(mz)) * k, every = MAX(1, k * 24 / MAX(len, 1));
    for (int j = every; j < k; j += every) if (!body_free(c->x + mx * j, c->z + mz * j, in)) return 0;
    return body_free(c->x + mx * k, c->z + mz * k, in);
}
static int go(Cit *c, int tx, int tz, int spd, int in, int n) {   // walk towards a point, round things; 1 when there. A body held up too long is simply put there.
    int dx = tx - c->x, dz = tz - c->z, d = ABS(dx) + ABS(dz);
    g_used = 1;
    if (d < 40) return 1;
    int k = n > 1 ? MIN(n, MAX(1, (d * 7 / 10 - 28) / spd)) : 1;   // (steps that stay short of the point, however it lies)
    turn(c, yaw_to(dx, dz), 170 * k);
    int mx = (isin(c->yaw) * spd) >> 12, mz = -((icos(c->yaw) * spd) >> 12);
    if (k > 1 && stride_free(c, mx, mz, k, in)) { c->x += (s16)(mx * k); c->z += (s16)(mz * k); c->st = 0; moved_flag = 1; g_used = k; return 0; }
    if (body_free(c->x + mx, c->z + mz, in)) { c->x += (s16)mx; c->z += (s16)mz; c->st = 0; moved_flag = 1; }
    else if (mx && body_free(c->x + mx * 2, c->z, in)) { c->x += (s16)(mx * 2); c->st = 0; moved_flag = 1; }
    else if (mz && body_free(c->x, c->z + mz * 2, in)) { c->z += (s16)(mz * 2); c->st = 0; moved_flag = 1; }
    else if (++c->st > 150) {
#ifdef CIT_TEST
        cit_snaps++; cit_by[c->mode][c->sub & 3]++;
#endif
        c->x = (s16)tx; c->z = (s16)tz; c->st = 0; CP(6, 1); return 1; }
    return 0;
}
#define BW 12                                    // routes are worked out inside a window of 25x25 tiles round the goal
static u8 bdist[2 * BW + 1][2 * BW + 1]; static u16 bq[(2 * BW + 1) * (2 * BW + 1)];
static int tile_ok(int x, int y, int in) {
    if (x < 1 || y < 1 || x >= MW - 1 || y >= MH - 1) return 0;
    int f = floorMap[y][x];
    if (in ? f >= FL_GRASS : (f < FL_GRASS || f == FL_WATER)) return 0;
    return world_walkable(x, y);
}
static int route_next(int gx, int gy, int sx, int sy, int in, int *nx, int *ny) {   // breadth-first from the goal; the neighbour of (sx,sy) one step nearer
    const int N = 2 * BW + 1, ox = gx - BW, oy = gy - BW;
    static const signed char dx4[4] = { 1, -1, 0, 0 }, dy4[4] = { 0, 0, 1, -1 };
    if (sx < ox || sy < oy || sx >= ox + N || sy >= oy + N) return 0;
    memset(bdist, 255, sizeof bdist);
    int h = 0, t = 0; bdist[gy - oy][gx - ox] = 0; bq[t++] = (u16)((gy - oy) * N + (gx - ox));
    while (h < t) {
        int q = bq[h++], x = q % N + ox, y = q / N + oy;
        if (x == sx && y == sy) break;
        for (int k = 0; k < 4; k++) {
            int a = x + dx4[k], b = y + dy4[k];
            if (a < ox || b < oy || a >= ox + N || b >= oy + N || bdist[b - oy][a - ox] != 255) continue;
            if (!(a == sx && b == sy) && !tile_ok(a, b, in)) { bdist[b - oy][a - ox] = 254; continue; }   // (blocked: asked once, not from each side; 254 is never walked back to)
            bdist[b - oy][a - ox] = (u8)(bdist[y - oy][x - ox] + 1); bq[t++] = (u16)((b - oy) * N + (a - ox));
        }
    }
    int here = bdist[sy - oy][sx - ox]; if (here == 255 || here == 0) return 0;
    for (int k = 0; k < 4; k++) {
        int a = sx + dx4[k], b = sy + dy4[k];
        if (a < ox || b < oy || a >= ox + N || b >= oy + N) continue;
        if (bdist[b - oy][a - ox] == here - 1) { *nx = a; *ny = b; return 1; }
    }
    return 0;
}
static int route_plan(Cit *c, int gx, int gy, int sx, int sy, int in) {   // one search, then the whole route (up to 32 steps) kept
    int nx, ny; CP(2, 1);
#ifdef CIT_PROBE
    dbg_pm[c->mode & 15][0]++;
#endif
    if (!route_next(gx, gy, sx, sy, in, &nx, &ny)) {
#ifdef CIT_PROBE
        { int *f = dbg_rf[dbg_rfn++ & 7]; f[0] = (int)(c - cits); f[1] = sx; f[2] = sy; f[3] = gx; f[4] = gy; f[5] = in; f[6] = floorMap[sy][sx] | world_walkable(sx, sy) << 8 | (c->x & 255) << 16; f[7] = c->sub | c->owe << 8 | (c->flg >> F_TSH & 3) << 16 | ((-c->z) & 255) << 24; }
        dbg_pm[c->mode & 15][1]++;
#endif
        return 0; }              // (fills bdist from the goal)
    const int N = 2 * BW + 1, ox = gx - BW, oy = gy - BW;
    static const signed char dx4[4] = { 1, -1, 0, 0 }, dy4[4] = { 0, 0, 1, -1 };
    int x = sx, y = sy, n = 0; memset(c->path, 0, sizeof c->path);
    while (n < 32 && !(x == gx && y == gy)) {
        int here = bdist[y - oy][x - ox], k;
        for (k = 0; k < 4; k++) { int a = x + dx4[k], b = y + dy4[k];
            if (a >= ox && b >= oy && a < ox + N && b < oy + N && bdist[b - oy][a - ox] == here - 1) break; }
        if (k == 4) break;
        c->path[n >> 2] |= (u8)(k << ((n & 3) * 2)); n++; x += dx4[k]; y += dy4[k];
    }
    if (!n) return 0;
    c->plen = (u8)n; c->pi = 0; c->gx = (u8)gx; c->gy = (u8)gy; c->wv = 1;
    return 1;
}
static int route_step(Cit *c, int sx, int sy) {   // the next waypoint along the kept route (0: it has run out)
    static const signed char dx4[4] = { 1, -1, 0, 0 }, dy4[4] = { 0, 0, 1, -1 };
    if (c->pi >= c->plen) return 0;
    int k = (c->path[c->pi >> 2] >> ((c->pi & 3) * 2)) & 3; c->pi++;
    c->wx = (u8)(sx + dx4[k]); c->wy = (u8)(sy + dy4[k]);
    return 1;
}
// BIG DECISIONS are shared out over frames: a route search (a 25x25 breadth-first fill) or a look round the home for
// something to do costs a token, and a frame has CIT_THINK of them (the last one kept for those who know you, or are
// busy with you). Someone who finds none waits where they are until the next frame's.
#define CIT_THINK 4
static int think_tok, think_pri;
static int think(void) { if (think_tok > (think_pri ? 0 : 1)) { think_tok--; return 1; } CP(12, 1); return 0; }
COLD static int nav(Cit *c, int gtx, int gty, int spd, int in, int n) {   // walk to a tile round the furniture (or the neighbours' houses); 1 when there
    int sx = c->x / TILE, sy = (-c->z) / TILE;
    if (sx == gtx && sy == gty) return go(c, gtx * TILE + TILE / 2, -(gty * TILE + TILE / 2), spd, in, n);
    if (c->wv == 2 && c->gx == gtx && c->gy == gty && c->wx == sx && c->wy == sy)   // (searched from this very tile for this goal and found no way: the straight line again, without the search)
        return go(c, gtx * TILE + TILE / 2, -(gty * TILE + TILE / 2), spd, in, n);
    int fresh = c->wv != 1 || c->gx != gtx || c->gy != gty;
    if (fresh || (sx == c->wx && sy == c->wy)) {
        int ok = 0;
        if (!fresh && sx == c->wx && sy == c->wy) ok = route_step(c, sx, sy);           // on to the next tile of the route
        if (!ok) { if (!think()) { g_used = n; return 0; }                              // (no search left this frame: they stand and think)
                   if (route_plan(c, gtx, gty, sx, sy, in)) ok = route_step(c, sx, sy); }   // (new goal, or the route ran out: search again)
        if (!ok) { c->wv = 2; c->gx = (u8)gtx; c->gy = (u8)gty; c->wx = (u8)sx; c->wy = (u8)sy; return go(c, gtx * TILE + TILE / 2, -(gty * TILE + TILE / 2), spd, in, n); }   // no route found: the straight line (and no search again until they are on another tile)
    }
    go(c, c->wx * TILE + TILE / 2, -(c->wy * TILE + TILE / 2), spd, in, n);
    return 0;
}
static int scripted(Cit *c, int tx, int tz, int spd, int n) {   // straight to a point, through a doorway, ignoring everything
    int dx = tx - c->x, dz = tz - c->z, d = ABS(dx) + ABS(dz);
    g_used = 1;
    if (d <= spd * 2) { c->x = (s16)tx; c->z = (s16)tz; return 1; }
    int k = MIN(n, MAX(1, d / spd - 2));
    c->yaw = (u16)yaw_to(dx, dz);
    c->x += (s16)(dx * spd * k / d); c->z += (s16)(dz * spd * k / d); moved_flag = 1; g_used = k;
    return 0;
}


// ---- a day at home: showers, breakfast, the sofa, the TV; and the odd visit next door ----------------------
static int sit_item(char ch) { return ch == 'T' || ch == 'C' || ch == 'h' || ch == 'U' || ch == '%' || ch == ';' || ch == (char)0x90 || ch == (char)0x91 || ch == (char)0x92 || ch == (char)0xA8; }
static int free_floor(const Home *hm, int x, int y) {   // a tile of the home's floor a body can stand on, off the lane inside the door
    if (x < hm->x0 || x > hm->x1 || y < hm->y0 || y > hm->y1) return 0;
    if (x == hm->dx && y >= hm->dy - 1) return 0;
    return world_walkable(x, y);
}
COLD static int find_item(const Home *hm, const char *letters, int *ix, int *iy, int *sx, int *sy) {   // one of those things in the home, and the tile to stand on to use it
    int fx[16], fy[16], n = 0;
    for (int y = hm->y0; y <= hm->y1; y++) for (int x = hm->x0; x <= hm->x1 && n < 16; x++) {
        char ch = world_at(x, y);
        if (ch && strchr(letters, ch)) { fx[n] = x; fy[n] = y; n++; }
    }
    for (int t = 0; t < n; t++) {
        int k = (rn(n) + t) % n, x = fx[k], y = fy[k];
        static const signed char ox[4] = { 0, -1, 1, 0 }, oy[4] = { 1, 0, 0, -1 };   // in front of it first
        if (free_floor(hm, x, y)) { *ix = *sx = x; *iy = *sy = y; return 1; }          // flat things (toilet, shower, sofa) are stood on
        for (int d = 0; d < 4; d++) if (free_floor(hm, x + ox[d], y + oy[d])) { *ix = x; *iy = y; *sx = x + ox[d]; *sy = y + oy[d]; return 1; }
    }
    return 0;
}
COLD static int start_chore(Cit *c, const Home *hm, const char *letters, char chain) {
    int ix, iy, sx, sy;
    if (!find_item(hm, letters, &ix, &iy, &sx, &sy)) return 0;
    c->chore = (u8)world_at(ix, iy); c->chain = (u8)chain; c->ix = (u8)ix; c->iy = (u8)iy; c->chx = (u8)sx; c->chy = (u8)sy;
    c->mode = CM_CHORE; c->sub = 0; c->st = 0; c->wv = 0; CP(3, 1);
    return 1;
}
COLD static int pick_chore(Cit *c, const Home *hm) {   // what a person does with a free hour at home: it depends on the time of day
    int m = clock_min + c->off; if (m < 0) m += 1440; if (m >= 1440) m -= 1440;
    int r = rn(100); const char *L;
    if (m < 600)        L = r < 30 ? "H," : r < 65 ? "F/" : r < 80 ? "T" : "-";
    else if (m >= 1020) L = r < 30 ? "F/" : r < 62 ? "v" : r < 78 ? "ChU%;" : r < 88 ? "T" : r < 96 ? "H," : "[";
    else                L = r < 22 ? "F/" : r < 42 ? "ChU%;" : r < 62 ? "v" : r < 74 ? "T" : r < 84 ? "[" : "-";
    char chain = (L[0] == 'F' && m >= 1020 && !rn(2)) ? 'v' : 0;      // supper, then the telly
    return start_chore(c, hm, L, chain);
}
static int chore_len(char ch) {
    switch (ch) { case 'H': case ',': return 260; case 'F': case '/': return 180; case 'T': return 110; case '-': return 70;
                  case 'v': return 520; case '[': return 380; default: return 420; }
}
static const char *chore_say(char ch) {
    switch (ch) { case 'H': case ',': return "SHOWER TIME"; case 'F': case '/': return "SNACK TIME"; case 'T': return "BE RIGHT BACK";
                  case '-': return "WASH UP"; case 'v': return "WHAT'S ON?"; case '[': return "CHECKING MAIL"; default: return "FEET UP"; }
}
static const char *activity(const Cit *c) {   // what the HUD says they are up to
    switch (c->mode) {
    case CM_SLEEP: return "ASLEEP";
    case CM_GOHOME: return "HEADING HOME";
    case CM_LEAVE: return "HEADING OUT";
    case CM_AWAY: return "WALKING OFF";
    case CM_GREET: return "COMING OVER";
    case CM_BUY: return "OFF TO THE FARM STAND";
    case CM_VISIT: return "VISITING";
    case CM_CHAT: return "CHATTING";
    case CM_CHASE: return "ANGRY";
    case CM_CHORE:
        if (c->sub == 0) return "BUSY AT HOME";
        switch (c->chore) { case 'H': case ',': return "IN THE SHOWER"; case 'F': case '/': return "HAVING A SNACK"; case 'T': return "IN THE LOO";
                            case '-': return "WASHING UP"; case 'v': return "WATCHING TV"; case '[': return "ON THE COMPUTER"; default: return "RELAXING"; }
    default: return indoors_at(c->x, c->z) ? "AT HOME" : "OUT AND ABOUT";
    }
}
COLD static int start_visit(Cit *c) {   // a neighbour in the same zone, a short walk away: go and knock
    const Home *me = home_of(c); if (!me) return 0;
    int cand[NHH], n = 0;
    for (int h = 0; h < NHH; h++) {
        if (hh[h].zone != c->zone || h == c->hh) continue;
        const Home *o = world_home(hh[h].zone, hh[h].home); if (!o) continue;
        if (ABS((int)o->dx - (int)me->dx) + ABS((int)o->dy - (int)me->dy) > 10) continue;
        cand[n++] = h;
    }
    if (!n) return 0;
    int h = cand[rn(n)]; const Home *o = world_home(hh[h].zone, hh[h].home);
    c->vh = (u8)h; c->ix = o->dx; c->iy = (u8)(o->dy + 1);
    c->mode = CM_VISIT; c->sub = 0; c->st = 0; c->wv = 0; CP(4, 1);
    return 1;
}

COLD static void decide(Cit *c) {   // the clock says where they should be: out, in, or in bed
    if (c->hh == NONE || c->mode == CM_PARTY) return;
    int w = want(c), in = indoors_at(c->x, c->z);
    if (c->mode == CM_IDLE || c->mode == CM_WALK) {
        if (w == 0 && in) { c->mode = CM_LEAVE; c->sub = 0; c->st = 0; }
        else if (w != 0 && !in) { c->mode = CM_GOHOME; c->sub = 0; c->st = 0; }
        else if (w == 2 && in) { c->mode = CM_SLEEP; c->sub = 0; c->st = 0; }
    } else if (c->mode == CM_SLEEP && w != 2) {
        c->mode = CM_IDLE; c->t = (u16)rn(120); c->gait.sleep = 0; c->sub = 0;
        const Home *hm = home_of(c);
        if (hm && in) start_chore(c, hm, "H,", 'F');                    // up, a shower, then breakfast
    } else if (c->mode == CM_CHORE) {
        if (w == 2) { c->gait.sit = 0; c->mode = CM_SLEEP; c->sub = 0; c->st = 0; }
        else if (w == 0 && c->sub == 0 && !c->chain && in) { c->mode = CM_LEAVE; c->sub = 0; c->st = 0; }   // (a chore already begun is finished first)
    } else if (c->mode == CM_VISIT) {
        if (w != 0 && !in) { c->mode = CM_GOHOME; c->sub = 0; c->st = 0; }
    }
}


// ---- PARTIES: up to 60 guests, dancing round the host ----------------------------------------------------------------
// The host (you) calls; guests arrive one after another at the door or down the street, take a spot round you, dance, chat,
// and leave when the party is over (three hours). At home the guests come over from the other zones and go back after.
#define PARTY_MIN 180                       // how long a party lasts, in game minutes
static const char *const L_PARTY[] = { "WOOO!", "GREAT PARTY!", "WHO PICKED THIS SONG?", "DANCE OFF!", "I LOVE THIS ONE", "MORE SNACKS?", "BEST NIGHT EVER", "SHOW US A MOVE", "ONE MORE SONG!", "MY FEET!" };
static int party_tile_ok(int x, int y) {
    if (x < 1 || y < 1 || x >= MW - 1 || y >= MH - 1) return 0;
    if (!world_walkable(x, y)) return 0;
    return (floorMap[y][x] < FL_GRASS) == party_in && floorMap[y][x] != FL_WATER;
}
COLD static int party_spots(void) {             // the dance floor: free tiles round the host, nearest first (a breadth-first fill)
    const int N = 2 * BW + 1, ox = party_cx - BW, oy = party_cy - BW;
    static const signed char dx4[4] = { 1, -1, 0, 0 }, dy4[4] = { 0, 0, 1, -1 };
    memset(bdist, 255, sizeof bdist);
    int h = 0, t = 0, n = 0; bdist[BW][BW] = 0; bq[t++] = (u16)(BW * N + BW);
    while (h < t && n < PARTY_MAX) {
        int q = bq[h++], x = q % N + ox, y = q / N + oy;
        if (bdist[y - oy][x - ox] >= 2 && ((x - party_cx) & 1) == 0 && ((y - party_cy) & 1) == 0) { party_sx[n] = (u8)x; party_sy[n] = (u8)y; n++; }   // a tile apart each way: room to dance
        for (int k = 0; k < 4; k++) {
            int a = x + dx4[k], b = y + dy4[k];
            if (a < ox || b < oy || a >= ox + N || b >= oy + N || bdist[b - oy][a - ox] != 255 || !party_tile_ok(a, b)) continue;
            bdist[b - oy][a - ox] = (u8)(bdist[y - oy][x - ox] + 1); bq[t++] = (u16)((b - oy) * N + (a - ox));
        }
    }
    return n;
}
COLD static void party_entry(void) {            // where guests come in: inside the nearest door for a party indoors, else down the street
    party_spx = party_cx * TILE + TILE / 2; party_spz = -(party_cy * TILE + TILE / 2);
    int best = 1 << 30;
    for (int y = MAX(1, party_cy - 12); y <= MIN(MH - 2, party_cy + 12); y++) for (int x = MAX(1, party_cx - 12); x <= MIN(MW - 2, party_cx + 12); x++) {
        if (party_in) {
            if (world_at(x, y) != 'D') continue;
            static const signed char dx4[4] = { 1, -1, 0, 0 }, dy4[4] = { 0, 0, 1, -1 };
            for (int k = 0; k < 4; k++) { int a = x + dx4[k], b = y + dy4[k];
                if (!party_tile_ok(a, b)) continue;
                int d = (a - party_cx) * (a - party_cx) + (b - party_cy) * (b - party_cy);
                if (d < best) { best = d; party_spx = a * TILE + TILE / 2; party_spz = -(b * TILE + TILE / 2); } }
        } else {
            int d = (x - party_cx) * (x - party_cx) + (y - party_cy) * (y - party_cy);
            if (d < 36 || d > 100 || !party_tile_ok(x, y)) continue;     // 6 to 10 tiles off: they walk in
            int s = d + (int)rn(40); if (s < best) { best = s; party_spx = x * TILE + TILE / 2; party_spz = -(y * TILE + TILE / 2); }
        }
    }
}
// ---- THE GUEST LIST: who you know, by household (the screen is PARTY.OVL; the lists live here) ----
PGroup pg_grp[PG_GROUPS]; PGuest pg_mem[PG_MEMS]; int pg_ng, pg_nm, party_open, party_go, party_score_now;
static int party_ginv[PARTY_MAX];          // which of this party's guests were asked by name (the ones whose opinion of you moves)
static int party_nguests, party_score, party_ninv;
static int pg_known_c(const Cit *c) { return c->rel != 0 || c->rom != 0; }
static int pg_known_p(int p) { const Per *q = &pop_per[p]; if (q->slot != 255 && q->slot < NCIT) return pg_known_c(&cits[q->slot]); return q->rel != 0 || q->rom != 0; }
COLD void citizens_names(u16 ref, char *out) {       // "FIRST LAST"
    if (ref & 0x8000) { strcpy(out, cits[ref & 0x7FFF].first); return; }
    pop_first(ref, out);
}
COLD void citizens_gname(int g, char *out) {          // a household's name
    const PGroup *k = &pg_grp[g];
    if (k->fam & 0x8000) { const Cit *c = &cits[pg_mem[k->first].ref & 0x7FFF]; sprintf(out, "%s HOME", c->last[0] ? c->last : c->first); }
    else { char b[NAME_LEN + 2]; pop_surname(k->fam, b); sprintf(out, "%s HOUSEHOLD", b); }
}
COLD void citizens_known(void) {                      // build the lists: every household with someone you know in it
    pg_ng = pg_nm = 0;
    for (int f = 0; f < pop_famcap && pg_ng < PG_GROUPS; f++) {
        const Fam *fm = &pop_fam[f]; int any = 0, live = 0;
        for (int j = 0; j < fm->n; j++) { int p = fm->first + j; if (pop_per[p].flags & PF_GONE) continue; live++; if (pg_known_p(p)) any = 1; }
        if (!any || pg_nm + live > PG_MEMS) continue;
        PGroup *g = &pg_grp[pg_ng++]; g->fam = (u16)f; g->first = (u16)pg_nm; g->n = 0;
        for (int j = 0; j < fm->n; j++) { int p = fm->first + j; if (pop_per[p].flags & PF_GONE) continue; PGuest *m = &pg_mem[pg_nm++]; m->ref = (u16)p; m->on = 0; m->known = (u8)pg_known_p(p); g->n++; }
    }
    for (int h = 0; h < nhh && pg_ng < PG_GROUPS; h++) {   // the island's and the town's own people, by household
        const Hh *hp = &hh[h]; if (hp->zone == 255 || hp->pfam != NOPID) continue;
        int any = 0, live = 0; for (int j = 0; j < hp->n; j++) { const Cit *c = &cits[hp->mem[j]]; if (c->mode == CM_GONE || c->pid != NOPID) continue; live++; if (pg_known_c(c)) any = 1; }
        if (!any || pg_nm + live > PG_MEMS) continue;
        PGroup *g = &pg_grp[pg_ng++]; g->fam = (u16)(0x8000 | h); g->first = (u16)pg_nm; g->n = 0;
        for (int j = 0; j < hp->n; j++) { const Cit *c = &cits[hp->mem[j]]; if (c->mode == CM_GONE || c->pid != NOPID) continue; PGuest *m = &pg_mem[pg_nm++]; m->ref = (u16)(0x8000 | hp->mem[j]); m->on = 0; m->known = (u8)pg_known_c(c); g->n++; }
    }
}
COLD static int party_item_score(void) {                  // the party things round the host: each kind counts up to three
    int cnt[8] = { 0 };
    for (int y = MAX(1, party_cy - 12); y <= MIN(MH - 2, party_cy + 12); y++) for (int x = MAX(1, party_cx - 12); x <= MIN(MW - 2, party_cx + 12); x++) {
        int ch = (u8)world_at(x, y); if (ch >= 0xC7 && ch <= 0xCE && cnt[ch - 0xC7] < 3) cnt[ch - 0xC7]++; }
    int sc = 0; for (int k = 0; k < 8; k++) sc += cnt[k]; return sc;
}
int citizens_party_score_here(void) { const Sim *p = &sims[player]; party_cx = p->x / TILE; party_cy = (-p->z) / TILE; return party_item_score(); }
COLD static int slot_for(u16 ref) {                      // a guest's body: their own live one, or one made for the night (SAN BOREAS's people)
    if (ref & 0x8000) { int i = ref & 0x7FFF; Cit *c = &cits[i]; return (c->mode == CM_GONE || c->mode == CM_KO || c->mode == CM_PARTY || c->zone == 255) ? -1 : i; }
    Per *q = &pop_per[ref]; if (q->flags & PF_GONE) return -1;
    if (q->slot != 255 && q->slot < NCIT) { Cit *c = &cits[q->slot]; return c->mode == CM_PARTY ? -1 : q->slot; }
    int slot = -1; for (int i = MAIN0; i < NCIT; i++) if (cits[i].zone == 255 && cits[i].pid == NOPID) { slot = i; break; }
    if (slot < 0) return -1;
    Cit *c = &cits[slot]; memset(&c->gait, 0, sizeof c->gait);
    c->seed = q->seed; c->stage = q->stage; c->rel = q->rel; c->rom = q->rom; c->hp = q->hp ? q->hp : 100;
    c->pers = q->trait == 1 ? PE_GRUFF : q->trait == 6 ? PE_FLIRTY : (q->trait == 8 || q->trait == 13) ? PE_SHY : PE_FRIENDLY;
    c->homebody = 0; c->off = 0; c->zone = ZONE_CITY; c->zone0 = 255; c->mode = CM_IDLE; c->t = 10; c->sub = 0; c->st = 0; c->wv = 0; c->say_t = 0; c->cd = 0; c->slot = 0;
    c->pid = ref; c->vanish = 0; c->hh = NONE; c->mem = 0; c->built = 0; c->mplace = NOPID; c->x = 0; c->z = 0;
    c->flg = (u8)((q->met ? F_MET : 0) | (pop_knows_you(ref) ? F_KN : 0)); c->owe = 0;
    crowd_forget(slot); q->slot = (u8)slot; return slot;
}
COLD int citizens_party(void) {      // the list is pg_mem[].on (and party_open: strangers fill the rest)
    if (party_on) return -1;
    const Sim *p = &sims[player];
    party_cx = p->x / TILE; party_cy = (-p->z) / TILE; party_in = indoors_at(p->x, p->z);
    int spots = party_spots();
    if (spots < 4) return 0;
    party_entry();
    party_n = 0; party_ninv = 0; int cap = MIN(spots, PARTY_MAX);
    for (int k = 0; k < pg_nm && party_n < cap; k++) {          // the ones you asked, by name
        if (!pg_mem[k].on) continue;
        int sl = slot_for(pg_mem[k].ref); if (sl < 0) continue;
        int dup = 0; for (int j = 0; j < party_n; j++) if (party_g[j] == sl) dup = 1; if (dup) continue;
        if (cits[sl].stage == AG_CHILD && clock_min >= 1260) continue;
        party_ginv[party_n] = 1; party_g[party_n++] = (u8)sl; party_ninv++;
    }
    if (party_open) for (int pass = 0; pass < 2; pass++) for (int i = 0; i < NCIT && party_n < cap; i++) {   // an open house: whoever is about
        Cit *c = &cits[i];
        if (c->mode == CM_GONE || c->mode == CM_KO || c->mode == CM_PARTY) continue;
        if ((pass == 0) != (c->zone == world_zone)) continue;
        if (c->stage == AG_CHILD && clock_min >= 1260) continue;
        int dup = 0; for (int j = 0; j < party_n; j++) if (party_g[j] == i) dup = 1; if (dup) continue;
        party_ginv[party_n] = 0; party_g[party_n++] = (u8)i;
    }
    if (!party_n) return 0;
    for (int k = party_n - 1; k > 0; k--) { int j = rn(k + 1); u8 t = party_g[k]; party_g[k] = party_g[j]; party_g[j] = t; int ti = party_ginv[k]; party_ginv[k] = party_ginv[j]; party_ginv[j] = ti; }
    party_score = party_item_score(); party_score_now = party_score; party_nguests = party_n;
    party_next = 0; party_t = 0; party_on = 1; party_left = PARTY_MIN + MIN(party_score, 12) * 20;
    return party_n;
}
int citizens_party_on(void) { return party_on; }
COLD static void party_arrive(Cit *c, int k) {  // a guest walks in
    if (c->zone != world_zone) { c->zone0 = c->zone; c->zone = (u8)world_zone; if (world_zone == ZONE_HOME) home_guests++; }
    c->x = (s16)(party_spx + (int)rn(61) - 30); c->z = (s16)(party_spz + (int)rn(61) - 30);
    c->mode = CM_PARTY; c->sub = 0; c->st = 0; c->wv = 0; c->gait.sleep = 0; c->gait.sit = 0; meet(c);
    c->chx = party_sx[k % PARTY_MAX]; c->chy = party_sy[k % PARTY_MAX];
    c->t = (u16)(600 + rn(600));
}
COLD static void party_leave_done(Cit *c) {     // home time
    if (c->pid != NOPID) { if (c->zone0 != 255 && c->zone == ZONE_HOME && home_guests > 0) home_guests--; c->zone0 = 255; main_free(c); return; }   // (one of SAN BOREAS's, made for the night: back into pop.c)
    c->mode = CM_IDLE; c->t = (u16)rn(60); c->sub = 0; c->wv = 0;
    if (c->zone0 != 255) { if (c->zone == ZONE_HOME && home_guests > 0) home_guests--; c->zone = c->zone0; c->zone0 = 255; c->x = 0; c->z = 0; crowd_drop_sprite((int)(c - cits)); }
}
static void party_tick(void) {             // once a step: guests arrive one after another
    if (!party_on) return;
    if (party_next < party_n && ++party_t >= 9) {
        party_t = 0; Cit *c = &cits[party_g[party_next]]; party_arrive(c, party_next); party_next++;
        if (party_next == 1) say(c, "WE BROUGHT SNACKS!");
    }
}
COLD static void party_minute(void) {           // once a game minute: the clock runs out
    if (!party_on) return;
    if (--party_left > 0) return;
    party_on = 0;                          // over: everyone heads off
    if (dj_active()) party_score += dj_heat / 25;   // (a set in the booth that kept the floor hot: up to four more)
    { int gifts = 0, friends = 0;
      for (int k = 0; k < party_n; k++) { Cit *c = &cits[party_g[k]]; if (c->mode != CM_PARTY) continue;
          if (party_ginv[k]) { c->rel = (s8)MIN(100, c->rel + 3 + party_score / 2); friends++; if (party_score >= 4 && c->rom > 0) c->rom = (s8)MIN(100, c->rom + 2); }
          else c->rel = (s8)MIN(100, c->rel + 1);
          gifts += 1 + party_score / 3; }
      if (party_score >= 2) { gifts = MIN(gifts, 400); cash += gifts; }
      sim_party_over(party_score, party_n);
      { char b[48]; sprintf(b, party_score >= 8 ? "LEGENDARY PARTY! $%d IN GIFTS" : party_score >= 4 ? "GREAT PARTY! $%d IN GIFTS" : party_score >= 2 ? "NICE PARTY. $%d IN GIFTS" : "A QUIET ONE. THEY HAD FUN%.0d", gifts); sim_note(b); } (void)friends; }
    for (int i = 0; i < NCIT; i++) { Cit *c = &cits[i]; if (c->mode == CM_PARTY) { c->sub = 2; c->wv = 0; c->st = 0; } }
}
COLD static void party_step(Cit *c, int i) {   // (party guests are thought about every step: one step at a time)
    int in = indoors_at(c->x, c->z);
    if (c->sub == 0) {                     // on the way to their spot
        if (nav(c, c->chx, c->chy, 5, party_in, 1)) { c->sub = 1; c->yaw = (u16)yaw_to(party_cx * TILE + TILE / 2 - c->x, -(party_cy * TILE + TILE / 2) - c->z); }
        else if (++c->st > 400) { c->x = (s16)(c->chx * TILE + TILE / 2); c->z = (s16)(-(c->chy * TILE + TILE / 2)); c->sub = 1; }
        return;
    }
    if (c->sub == 1) {                     // dancing: steps in place, a sway, a wave, the odd shout
        moved_flag = 1;
        int sway = isin((tick * 24 + i * 300) & 4095) >> 4;
        int face = yaw_to(party_cx * TILE + TILE / 2 - c->x, -(party_cy * TILE + TILE / 2) - c->z);
        c->yaw = (u16)((face + sway) & 4095);
        if (!((tick + i) & 63) && !rn(3)) c->gait.wave = 24;
        if (!c->say_t && !rn(1400)) say(c, L_PARTY[rn(NL(L_PARTY))]);
        return;
    }
    // sub 2: the party's over - out the way they came, then off home
    if (go(c, party_spx, party_spz, 5, in, 1)) party_leave_done(c);
    else if (++c->st > 300) party_leave_done(c);
}

// ---- the crowd: walking, talking, brawling ----------------------------------------------------------------
static int count_down(u16 *t, int n) { int k = MIN((int)*t, n); *t = (u16)(*t - k); return MAX(k, 1); }   // (a timer: the steps it took)
static int decide_due(int i, int n) {   // the clock is looked at for 8 steps in every 128 (((tick >> 3) + i) & 15 == 0): did one of the n steps just lived fall there?
    return n >= 128 || (int)((tick + 8 * i) & 127) <= n + 6;   // (the phase of this step in that cycle; the window is its first 8)
}
static void greet_maybe(Cit *c, int d2, int in);
COLD static void fight_one(Cit *c, int dx, int dz, int d2, int in) {   // (one step) after you, or come to break the fight up
    Sim *p = &sims[player];
    if (c->mode == CM_CHASE) {
        if (d2 > (c->cop ? 12 : 7) * TILE * (c->cop ? 12 : 7) * TILE) { c->mode = CM_IDLE; c->t = 60; say(c, "FORGET IT"); return; }
        turn(c, yaw_to(dx, dz), 180);
        if (d2 > 90 * 90) {
            int spd = in ? 4 : 7;
            int mx = (isin(c->yaw) * spd) >> 12, mz = -((icos(c->yaw) * spd) >> 12);
            if (body_free(c->x + mx, c->z + mz, in)) { c->x += (s16)mx; c->z += (s16)mz; moved_flag = 1; }
            else if (body_free(c->x + mx, c->z, in)) { c->x += (s16)mx; moved_flag = 1; }
            else if (body_free(c->x, c->z + mz, in)) { c->z += (s16)mz; moved_flag = 1; }
        } else if (c->cop && !c->cd && rn(100) < (heat < 50 ? 60 : 30)) {   // the police try to cuff you first; the cooler you are, the likelier
            if (rn(100) < (int)(SKL(sims[player].skill_char)) * 5 && crime_total() < 3) { sim_note("YOU TALK YOUR WAY OUT OF IT"); say(c, "...JUST A WARNING."); fam_calm(); fight_on = 0; heat = heat / 2; c->mode = CM_IDLE; c->t = 200; for (int k = 0; k < NCIT; k++) if (cits[k].mode == CM_CHASE || cits[k].mode == CM_STOP) { cits[k].mode = CM_IDLE; cits[k].t = 120; } return; }   // (CHARISMA)
            int how_ = crime_total() && heat < 50 ? 1 : 0; sim_note("UNDER ARREST!"); say(c, "YOU ARE UNDER ARREST."); fam_calm(); fight_on = 0; heat = 0; c->mode = CM_IDLE; c->t = 120;
            for (int k = 0; k < NCIT; k++) if (cits[k].mode == CM_CHASE || cits[k].mode == CM_STOP) { cits[k].mode = CM_IDLE; cits[k].t = 120; }
            sim_arrest(how_); audio_voice(VOICE_CRY);
        } else if (!c->cd) {
            int dmg = (c->stage == AG_ELDER ? 2 : 3) + rn(5) + (c->cop ? 3 : 0);
            pl_hp -= tr_scale(p->tr, E_FIGHT_DEF, dmg); c->cd = 45; c->gait.wave = 14; p->gait.squash = -10;
            audio_voice(VOICE_GROAN);
            if (pl_hp <= 0) {
                int lost = MIN((int)cash, 60); cash -= lost; pl_hp = 60;
                char b[32]; sprintf(b, "KNOCKED OUT. LOST $%d", lost); sim_note(b);
                for (int k = 0; k < NCIT; k++) if (cits[k].mode == CM_CHASE || cits[k].mode == CM_STOP) { cits[k].mode = CM_IDLE; cits[k].t = 120; }
                fam_calm(); heat = 0;
                if (c->cop) { sim_note("ARRESTED!"); sim_crime(CR_RESIST, 15); sim_arrest(0); }
                fight_on = 0; say(c, c->cop ? "YOU HAVE THE RIGHT TO STAY BORED." : "STAY DOWN."); audio_voice(VOICE_CRY);
            }
        }
        return; }
    {   // a peacemaker: up close, hands out, talking you down
        if (c->t) c->t--;
        if (!c->t || !fight_on) { c->mode = CM_IDLE; c->t = 60; say(c, fight_on ? "I GIVE UP ON YOU" : "THANK GOODNESS"); return; }
        turn(c, yaw_to(dx, dz), 180);
        if (d2 > 140 * 140) { int spd = in ? 4 : 6, mx = (isin(c->yaw) * spd) >> 12, mz = -((icos(c->yaw) * spd) >> 12);
            if (body_free(c->x + mx, c->z + mz, in)) { c->x += (s16)mx; c->z += (s16)mz; moved_flag = 1; } }
        else { c->gait.wave = 10; if (!c->say_t) { static const char *const st[4] = { "STOP IT!", "BREAK IT UP!", "THAT'S ENOUGH!", "CALM DOWN!" }; say(c, st[rn(4)]); } }
        }
}
COLD static int step_rest(Cit *c, int i, int n, int dx, int dz, int d2, const Home *hm, int in) {   // (step_core's other modes: built small)
    Sim *p = &sims[player]; int used = 1;
    switch (c->mode) {
    case CM_AWAY:   // wanted elsewhere: off along the streets to the edge, and gone once out of sight
        if (!c->t || nav(c, c->ax, c->ay, in ? 3 : c->stage == AG_ELDER ? 3 : 5, in, MIN(n, (int)c->t))) { c->mode = CM_IDLE; c->t = 30; }
        else { used = g_used; c->t = (u16)(c->t - MIN((int)c->t, used)); }
        break;
    case CM_GREET:  // someone who knows you, come over to say hello (round the benches, lamps and fences, then straight up to you; given up if held up)
        if (!c->t || c->st > 90 || d2 > 12 * TILE * 12 * TILE) { GL(!c->t ? 2 : c->st > 90 ? 3 : 4); c->mode = CM_IDLE; c->t = 60; break; }
        if (d2 < 150 * 150) {
            static const char *const gl[6] = { "HEY! IT'S YOU!", "THERE YOU ARE!", "FANCY SEEING YOU HERE", "HI! HOW ARE YOU?", "I WAS JUST THINKING OF YOU", "LONG TIME NO SEE!" };
            const char *ln = c->rel < 0 ? (c->pers == PE_GRUFF ? "YOU AGAIN." : "OH. HELLO.") : c->rom >= 40 ? "THERE YOU ARE, YOU" : gl[rn(6)];
            say(c, ln); c->gait.wave = 40; c->yaw = (u16)yaw_to(dx, dz); c->mode = CM_IDLE; c->t = 150;
            if (c->rel >= 0) rel_add(c, 1); if (c->pid != NOPID) pop_seen(c->pid);
            CP(10, 1); GL(1); break;
        }
        { int spd = in ? 4 : c->stage == AG_ELDER ? 4 : 7, m = MIN(n, (int)c->t);   // (a hurry: you might be off again)
          if (d2 < 2 * TILE * 2 * TILE) go(c, p->x, p->z, spd, in, m); else nav(c, p->x / TILE, (-p->z) / TILE, spd, in, m);
          used = g_used; c->t = (u16)(c->t - MIN((int)c->t, used)); }   // (the steps walked count against its time, which never ran down before: a greeter who could not reach you kept on)
        break;
    case CM_BUY:    // a customer for the farm stand (citizens_send_buyer): over to it, buys a few of something (sim.c), a word, and off again; they know you now
        if (!c->t || c->st > 90) { GB(3); c->mode = CM_IDLE; c->t = 60; break; }
        if (ABS(c->x / TILE - c->ix) + ABS((-c->z) / TILE - c->iy) <= 1) { int v; GB(2); say(c, farm_customer(&v)); c->gait.wave = 40; c->mode = CM_IDLE; c->t = 240; meet(c); if (v) rel_add(c, 2); break; }
        nav(c, c->ix, c->iy, 5, in, MIN(n, (int)c->t)); used = g_used; c->t = (u16)(c->t - MIN((int)c->t, used));
        break;
    case CM_CHAT:
        if (c->t) c->t--;
        if (!c->t || d2 > 3 * TILE * 3 * TILE) { c->mode = CM_IDLE; c->t = 30; break; }
        turn(c, yaw_to(dx, dz), 200);
        break;
    case CM_CHASE: case CM_STOP: fight_one(c, dx, dz, d2, in); break;
    case CM_KO:
        c->gait.sleep = 1;
        if (c->t) used = count_down(&c->t, n);
        if (!c->t) { c->mode = CM_IDLE; c->hp = 25; c->gait.sleep = 0; c->t = 60; say(c, "...OW."); }
        break;
    case CM_GOHOME: {   // to the front of their door, in through it, and a step inside
        if (!hm) { c->mode = CM_IDLE; break; }
        int gx, zf, zd, zi; door_geo(hm, &gx, &zf, &zd, &zi);
        int spd = c->stage == AG_ELDER ? 3 : 5;
        if (c->sub == 0) { if (nav(c, hm->dx, hm->dy + 1, spd, 0, n)) c->sub = 1; }
        else if (c->sub == 1) { if (scripted(c, gx, zd, 3, n)) c->sub = 2; }
        else if (scripted(c, gx, zi, 3, n)) { c->mode = CM_IDLE; c->t = (u16)(20 + rn(100)); }
        used = g_used; break; }
    case CM_LEAVE: {    // across the floor to the door, out through it, and off
        if (!hm) { c->mode = CM_IDLE; break; }
        int gx, zf, zd, zi; door_geo(hm, &gx, &zf, &zd, &zi);
        if (c->sub == 0) { if (nav(c, hm->dx, hm->dy - 1, 3, 1, n)) c->sub = 1; }
        else if (c->sub == 1) { if (scripted(c, gx, zd, 3, n)) c->sub = 2; }
        else if (scripted(c, gx, zf, 3, n)) { c->mode = CM_IDLE; c->t = (u16)rn(40); }
        used = g_used; break; }
    case CM_CHORE: {
        if (!hm) { c->mode = CM_IDLE; break; }
        if (c->sub == 0) {
            if (nav(c, c->chx, c->chy, 3, 1, n)) {
                c->sub = 1; c->t = (u16)(chore_len((char)c->chore) / 2 + rn(chore_len((char)c->chore)));
                c->x = (s16)(c->chx * TILE + TILE / 2); c->z = (s16)(-(c->chy * TILE + TILE / 2));
                if (c->chx != c->ix || c->chy != c->iy) c->yaw = (u16)yaw_to((c->ix - c->chx) * TILE, -(c->iy - c->chy) * TILE);   // face the thing
                c->gait.sit = sit_item((char)c->chore) && c->chx == c->ix && c->chy == c->iy;
                if (d2 < 5 * TILE * 5 * TILE && !c->say_t) say(c, chore_say((char)c->chore));
            }
            used = g_used;
        } else if (c->t) {
            used = count_down(&c->t, n);
            if (c->chore == 'F' || c->chore == '/' || c->chore == '-') c->gait.wave = (c->t & 31) < 6 ? 8 : c->gait.wave;   // a bit of busy hands
        } else {
            c->gait.sit = 0;
            if (c->chain) { char nx[2] = { (char)c->chain, 0 }; if (start_chore(c, hm, nx, 0)) break; }
            c->chain = 0; c->mode = CM_IDLE; c->t = (u16)(30 + rn(150));
        }
        break; }
    case CM_VISIT: {   // out to a neighbour's door, a knock, a wait, and off again
        if (c->sub == 0) { if (nav(c, c->ix, c->iy, c->stage == AG_ELDER ? 3 : 4, 0, n)) {
            c->sub = 1; c->t = (u16)(200 + rn(200)); c->yaw = 0;
#ifdef BORE_DC
            if (d2 < 7 * TILE * 7 * TILE) { say(c, "KNOCK KNOCK"); { extern void dc_rumble(int, int); dc_rumble(3, 4); } }   // (the Jump Pack knocks too)
#else
            if (d2 < 7 * TILE * 7 * TILE) say(c, "KNOCK KNOCK");
#endif
            const Hh *g = &hh[c->vh];
            for (int k = 0; k < g->n; k++) { Cit *o = &cits[g->mem[k]]; if (o->mode == CM_IDLE || (o->mode == CM_CHORE && o->sub == 1)) { if (!o->say_t && indoors_at(o->x, o->z)) say(o, "COMING!"); break; } }
        } used = g_used; }
        else if (c->t) used = count_down(&c->t, n);
        else { c->mode = CM_IDLE; c->t = (u16)(60 + rn(200)); if (!rn(2)) say(c, "OH WELL, LATER"); }
        break; }
    case CM_PARTY: party_step(c, i); break;
    case CM_SLEEP:   // (sub 0: the bed is looked for, once; 3: on the way to it; 1: asleep)
        if (!hm) { c->mode = CM_IDLE; break; }
        if (c->sub == 0) {
            if (!think()) { used = n; break; }
            int sx, sz; sleep_spot(c, hm, &sx, &sz); c->ix = (u8)(sx / TILE); c->iy = (u8)((-sz) / TILE); c->sub = 3;
        }
        if (c->sub == 3) {
            if (nav(c, c->ix, c->iy, 3, 1, n)) { c->sub = 1; c->x = (s16)(c->ix * TILE + TILE / 2); c->z = (s16)(-(c->iy * TILE + TILE / 2)); c->yaw = 1024; }
            used = g_used;
        } else if (c->sub == 1) { c->gait.sleep = 1; c->gait.amp = 0; used = n; }   // (a night's sleep: all the steps there are)
        break;
    }
    return used;
}
COLD static int step_core(Cit *c, int i, int n) {   // live up to n steps of this person's life in one go; returns the steps it took (1..n)
    Sim *p = &sims[player];
    int dx = p->x - c->x, dz = p->z - c->z;
    int d2 = dx * dx + dz * dz;
    const Home *hm = home_of(c);
    int in = indoors_at(c->x, c->z), used = 1, say0 = c->say_t, cd0 = c->cd;
    moved_flag = 0; g_used = 1;
    c->gait.talk = c->mode == CM_CHAT;
    c->gait.mood = c->rel / 2;
    if (decide_due(i, n)) { int m0_ = c->mode; decide(c); CP(5, c->mode != m0_); (void)m0_; }   // look at the clock every so often
    switch (c->mode) {
    case CM_IDLE:
        if (c->t) { used = count_down(&c->t, n); break; }
        if (hm && in && want(c) == 1 && rn(100) < 70) { if (!think()) { used = n; break; } if (pick_chore(c, hm)) break; }   // a free hour at home: something to do
        if (hm && !in && c->stage >= AG_TEEN && clock_min >= 540 && clock_min < 1260 && !rn(14)) { if (!think()) { used = n; break; } if (start_visit(c)) break; }   // or a knock on a neighbour's door
        greet_maybe(c, d2, in); if (c->mode == CM_GREET) break;                                   // or, knowing you, over to say hello
        c->wyaw = (u16)rn(4096); c->t = (u16)(80 + rn(220)); c->mode = CM_WALK; WK(0);
        if (hm && !in) {   // out on the streets: drift back towards home when too far from it
            int a, zf, b, e; door_geo(hm, &a, &zf, &b, &e);
            int hx = a - c->x, hz = zf - c->z;
            if (hx * hx + hz * hz > 9 * TILE * 9 * TILE) c->wyaw = (u16)((yaw_to(hx, hz) + rn(512) - 256) & 4095);
        }
        if (!hm && c->mplace != NOPID) {   // at work, at school, out somewhere: they keep about the place
            int hx = c->ax * TILE + TILE / 2 - c->x, hz = -(c->ay * TILE + TILE / 2) - c->z;
            if (hx * hx + hz * hz > 4 * TILE * 4 * TILE) c->wyaw = (u16)((yaw_to(hx, hz) + rn(400) - 200) & 4095);
            else if (!rn(3)) { c->mode = CM_IDLE; c->t = (u16)(60 + rn(240)); break; }   // mostly they stay put (behind the counter, at the desk)
        }
        break;
    case CM_WALK: {
        if (!c->t) { c->mode = CM_IDLE; c->t = (u16)(30 + rn(200)); WK(1); break; }
        int k = ABS(((c->wyaw - c->yaw + 6144) & 4095) - 2048) > 90 ? 1 : MIN(n, (int)c->t);   // (still turning: a step at a time, as ever; facing the way: strides)
        turn(c, c->wyaw, 90 * k);
        int spd = in ? 3 : c->stage == AG_CHILD ? 6 : c->stage == AG_ELDER ? 3 : 5 + (c->pers & 1);
        int mx = (isin(c->yaw) * spd) >> 12, mz = -((icos(c->yaw) * spd) >> 12);
        if (d2 < 120 * 120 && (mx * dx + mz * dz) > 0) { c->mode = CM_IDLE; c->t = 40; break; }   // do not walk into you
        if (k > 1 && stride_free(c, mx, mz, k, in)) { c->x += (s16)(mx * k); c->z += (s16)(mz * k); moved_flag = 1; c->t = (u16)(c->t - k); used = k; break; }   // (several steps of clear street at once)
        c->t--;
        if (body_free(c->x + mx, c->z + mz, in)) { c->x += (s16)mx; c->z += (s16)mz; moved_flag = 1; }
        else { c->wyaw = (u16)((c->yaw + 1024 + rn(2048)) & 4095); WK(3); if (!rn(3)) { c->mode = CM_IDLE; c->t = 20; WK(2); } }
        break; }
    default: used = step_rest(c, i, n, dx, dz, d2, hm, in); break;
    }
    if (used > n) used = n;
    if (c->mode == CM_IDLE && c->say_t == 0 && d2 < 2 * TILE * 2 * TILE && c->rel > 10 && rn(900) < used) say(c, chat_line(c));   // a friend says hi in passing
    if (c->say_t && c->say_t == say0) c->say_t = (u16)MAX(0, say0 - used);   // (the bubble and the cool-down run out by the steps lived; a new line starts its own)
    if (c->cd && c->cd == cd0) c->cd = (u8)MAX(0, cd0 - used);
    // the walk cycle
    c->gait.run = moved_flag && (c->mode == CM_CHASE || c->mode == CM_STOP);
    if (moved_flag) { c->gait.phase = (c->gait.phase + (c->gait.run ? 200 : 130) * used) & 4095; c->gait.amp = MIN(c->gait.amp + 400 * used, 4096); }
    else c->gait.amp = MAX(c->gait.amp - 500 * used, 0);
    if (c->mode != CM_SLEEP && c->mode != CM_KO) c->gait.sleep = 0;
    if (c->mode != CM_CHORE) c->gait.sit = 0;
    if (c->gait.wave > 0) c->gait.wave = MAX(0, c->gait.wave - used);
    if (c->gait.squash < 0) c->gait.squash = MIN(0, c->gait.squash + used);
    c->gait.seed = i; c->gait.clock = tick;
    return used;
}
COLD static void step_one(Cit *c, int i, int n) {   // n steps of their life: a few big strides through it (what is left goes to the next call)
    int x0_ = c->x, z0_ = c->z, left = n;
    for (int it = 0; it < 8 && left > 0 && c->mode != CM_GONE; it++) {
#ifdef CIT_PROBE
        int m_ = c->mode & 15, x1_ = c->x, z1_ = c->z;
#endif
        int u = step_core(c, i, left); left -= u; CP(0, 1);
#ifdef CIT_PROBE
        dbg_cm[m_][0] += u; dbg_cm[m_][1] += ABS(c->x - x1_) + ABS(c->z - z1_);
#endif
    }
    c->owe = (u8)MIN(left, 255);
    CP(1, n - left); CP(7, ABS(c->x - x0_) + ABS(c->z - z0_)); (void)x0_; (void)z0_;
}

#ifdef CIT_PROBE
static void cit_probe(void) {   // a snapshot of the crowd every 512 steps
    if (dbg_cpr_n >= 24) return;
    u16 *r = dbg_cpr[dbg_cpr_n++]; memset(r, 0, 48);
    r[0] = (u16)clock_min; r[23] = (u16)(clock_day | indoors_at(sims[player].x, sims[player].z) << 8);   // (and whether you are indoors)
    for (int i = 0; i < NCIT; i++) { const Cit *c = &cits[i]; if (c->zone != world_zone || c->mode == CM_GONE) continue;
        r[1]++; if (c->mode < 14) r[2 + c->mode]++; if (indoors_at(c->x, c->z)) r[16]++; if (c->mode == CM_SLEEP && c->sub == 1) r[17]++; if (c->rel > 0) r[18] += (u16)c->rel;
        if (c->rel || c->rom) r[22]++; }
    r[19] = (u16)dbg_cc[3]; r[20] = (u16)dbg_cc[5]; r[21] = (u16)(dbg_cc[7] >> 8);
}
#endif
// ---- THE SCHEDULER: 5 townsfolk or 500, the same time a frame, and the ones who know you first ------------------------------
// Each frame every live townsperson is put in one of four tiers:
//   0 BUSY WITH YOU: chatting, fighting you, making peace, at your party, coming over to you, the one in front of you, or
//     within 3 tiles: thought about every logic step, as always.
//   1 KNOW YOU (or you them): met you (any interaction at all, for good), a friend, a rival or an ex (what they think of you,
//     the romance), one of yours who moved out, a neighbour on your block, a coworker at your job: every step too, wherever
//     they are in the map window.
//   2 STRANGERS NEAR OR IN SIGHT (10 tiles, or on the screen to 16): once a frame, living all the frame's steps (1..3) in one.
//   3 STRANGERS FAR: every 4th frame (past 26 tiles every 8th), living all the steps since their last turn.
// A call that lives several steps is the same life in bigger strides (step_core: timers run out by the steps, walks cover the
// ground of them in one move, the clock is looked at if its moment fell in them): nobody's routine slows down or loses state.
// The work is counted in thinking calls against CIT_BUDGET a frame (a new body built this frame takes CIT_BUILD_COST of it),
// handed out tier by tier: a tier that does not fit what is left is spaced out round-robin (every 2nd frame, 4th, ...) with
// each living all the steps it missed, never thinned out. A clock stops the strangers' turns in a frame that has already run
// CIT_HB_CAP hblanks (a body was built): they go first in the next one. Route searches and looking round a home are shared out
// over frames too (CIT_THINK, above).
#define CIT_BUDGET     64            // thinking calls a frame (about half an hblank each): a crowd of 60 who all know you is still thought about every frame
#define CIT_BUILD_COST 32
#define CIT_HB_CAP     80            // (a frame's townsfolk: about 30 hb, so only a frame with a new body in it gets here)
#define OWE_MAX        64            // (more steps than that owed: a stranger starved for seconds; the rest are let go)
#if defined(AUTO_CITY) || defined(CIT_PROBE)
int dbg_ctier[8];                    // (test readouts, summed over frames: the four tiers' sizes, calls, frames a tier was spaced out, strangers put off by the clock)
#define CT(k, v) (dbg_ctier[k] += (v))
#else
#define CT(k, v) ((void)0)
#endif
static int cit_known(const Cit *c) { return (c->flg & (F_MET | F_KN)) || c->rel || c->rom; }
static void meet(Cit *c) {           // any interaction at all: they know you now, for good
    if (c->flg & F_MET) return;
    c->flg |= F_MET; if (c->pid != NOPID) { pop_per[c->pid].met = 1; pop_know(c->pid); }
}
COLD static void greet_maybe(Cit *c, int d2, int in) {   // (at a loose end) someone who knows you, out and seeing you out too, comes over to say hello: once each time they are out
    if (!cit_known(c) || (c->flg & F_HI) || in || c->rel <= -40 || fight_on || c->stage < AG_CHILD) return;
    if (d2 > 8 * TILE * 8 * TILE || d2 < 2 * TILE * 2 * TILE || indoors_at(sims[player].x, sims[player].z) || rn(3)) return;
    c->flg |= F_HI; c->mode = CM_GREET; c->t = 420; c->st = 0; c->wv = 0; GL(0);
}
static int ceil_div(int a, int b) { return (a + b - 1) / b; }
// A body costs about 190 hb (the voxel model's shaping is most of it): a quarter of a 20 fps frame, so a frame that built one used to
// miss its vblank. Now only someone who would be drawn this very frame and has no body is built at once, here; everyone else who
// wants one (coming into range, off the screen, or whose stored model was let go and is drawn plain meanwhile) waits for a frame
// with the time to spare (citizens_draw builds them there, before the sprite renders, which get what is left). If no such frame
// comes for a second and a half, the nearest is built here anyway.
static int cit_pend = -1, cit_pend_wait;   // the body to build in spare time (-1 none); frames it has waited
int cit_build_hb = 200;                    // what a body costs (the highest lately, easing down: a spare frame must have this much)
COLD static int would_draw(const Cit *c, int pin) {   // citizens_draw would draw them now (as far as the update can tell: last frame's camera)
    int dx = c->x - cam.at.vx, dz = c->z - cam.at.vz, d = dx * dx + dz * dz;
    if (d > DRAW_DIST * DRAW_DIST) return 0;
    if (!pin && d > 3 * TILE * 3 * TILE && indoors_at(c->x, c->z)) return 0;
    return gfx_visible(c->x, -110, c->z, 150);
}
COLD static __attribute__((noinline)) void cit_build_now(Cit *c) {
    int t0 = VSync(1);
    atlas_defer = 1; build_lod(c); atlas_defer = 0;   // (its face goes to VRAM at the flip)
    int t = VSync(1) - t0; if (t > 0 && t < 600) cit_build_hb = t > cit_build_hb ? t : cit_build_hb - ((cit_build_hb - t) >> 4);
}
COLD static void cit_build(const u8 *lv, int nlv, int *built) {   // bodies for those who have none yet: the nearest the camera first (in SAN BOREAS, only those near enough to be seen soon)
    int pin = indoors_at(sims[player].x, sims[player].z);
    int best = -1, bd = world_zone == ZONE_CITY ? (DRAW_DIST + 4 * TILE) * (DRAW_DIST + 4 * TILE) : 1 << 30, now = -1, nd = 1 << 30;
    for (int j = 0; j < nlv; j++) {
        int i = lv[j]; const Cit *c = &cits[i];
        if (c->zone != world_zone || c->built || c->mode == CM_GONE) continue;
        int dx = c->x - cam.at.vx, dz = c->z - cam.at.vz, d = dx * dx + dz * dz;
        if (d < bd) { bd = d; best = i; }
        if (d < nd && would_draw(c, pin)) { nd = d; now = i; }   // (on the screen with nothing to draw: it cannot wait)
    }
    if (best < 0) {
        bd = DRAW_DIST * DRAW_DIST;   // someone near the camera whose stored model was let go (the store was full): build them again
        for (int j = 0; j < nlv; j++) {
            int i = lv[j]; const Cit *c = &cits[i];
            if (c->zone != world_zone || !c->built || c->mode == CM_GONE) continue;
            int dx = c->x - cam.at.vx, dz = c->z - cam.at.vz, d = dx * dx + dz * dz;
            if (d < bd && !crowd_has(i)) { bd = d; best = i; }   // (the near ones only are asked about)
        }
    }
    cit_pend = best;
    if (best < 0) { cit_pend_wait = 0; return; }
    if (now < 0 && ++cit_pend_wait <= 30) return;   // (it can wait for a frame with time to spare)
    cit_build_now(&cits[now >= 0 ? now : best]); (*built)++; cit_pend = -1; cit_pend_wait = 0;
}
COLD static __attribute__((noinline)) int cit_spare_build(int left) {   // (citizens_draw) a body in the frame's spare time, if one is wanted and fits in left hb: the time it took
    if (cit_pend < 0) return 0;
    Cit *c = &cits[cit_pend];
    if (c->zone != world_zone || c->mode == CM_GONE || (c->built && crowd_has(cit_pend))) { cit_pend = -1; return 0; }
    if (left < cit_build_hb + 8) return 0;
    int t0 = VSync(1); cit_build_now(c); cit_pend = -1; cit_pend_wait = 0;
    return VSync(1) - t0;
}
COLD void citizens_update(int steps) {   // (built small: the work is in step_one)
    if (world_zone == ZONE_HOME && !home_guests && !party_on) { cur = -1; fight_on = 0; return; }
    int h0 = VSync(1);
    static int cfr; cfr++;
    if (world_zone == ZONE_CITY) pop_tick();
    static u8 lv[NCIT], lst[NCIT]; int nlv = 0;   // who is here: one look at every slot a frame, and everything after works from this list
    for (int i = 0; i < NCIT; i++) if (cits[i].zone == world_zone && cits[i].mode != CM_GONE) lv[nlv++] = (u8)i;
    if (world_zone == ZONE_CITY) vanish_tick(lv, nlv);
    int built = 0; cit_build(lv, nlv, &built);
    // the tiers (lst: the busy with you from the front, the ones who know you from the back; everyone's tier is kept in flg)
    int nt[4] = { 0, 0, 0, 0 }, nfar2 = 0;
    const Sim *me = &sims[player];
    for (int j = 0; j < nlv; j++) {
        int i = lv[j]; Cit *c = &cits[i];
        if (c->zone != world_zone || c->mode == CM_GONE) continue;
        int dx = (me->x - c->x) >> 4, dz = (me->z - c->z) >> 4, d2 = dx * dx + dz * dz, t;   // (in sixteenths of a tile)
        #define T16(n) ((n) * TILE / 16) * ((n) * TILE / 16)
#ifdef CIT_FLAT
        if (1) t = 0; else   // (test builds: everyone every step, one at a time: what the strides are checked against)
#endif
        if (c->mode == CM_CHAT || c->mode == CM_CHASE || c->mode == CM_STOP || c->mode == CM_PARTY || c->mode == CM_GREET || i == cur || i == pie_c || d2 < T16(3)) t = 0;
        else if (cit_known(c)) t = 1;
        else if (d2 < T16(10) || (d2 < T16(16) && gfx_visible(c->x, -110, c->z, 160))) t = 2;
        else { t = 3; if (d2 > T16(26)) nfar2++; }
        c->flg = (u8)((c->flg & (F_MET | F_HI | F_KN | F_LATE)) | (t == 3 && d2 > T16(26) ? F_FAR : 0) | t << F_TSH);
        if (c->pid != NOPID && t <= 1 && d2 < T16(6) && !((cfr + i) & 31)) pop_seen(c->pid);   // (someone who knows you, and saw you today)
        #undef T16
        if (t == 0) lst[nt[0]] = (u8)i; else if (t == 1) lst[NCIT - 1 - nt[1]] = (u8)i;
        nt[t]++;
    }
    // the budget, tier by tier: every step, or once every fp frames (round-robin, all the steps since)
    int bud = CIT_BUDGET - (built ? CIT_BUILD_COST : 0) - nt[0] * steps;
    int fpA = 0;                                                         // (0: every step)
    if (nt[1] * steps > bud) { fpA = 1; while (fpA < 8 && ceil_div(nt[1], fpA) > bud) fpA <<= 1; CT(6, 1); }
    bud -= fpA ? ceil_div(nt[1], fpA) : nt[1] * steps;
    int fpB = 1; while (fpB < 8 && ceil_div(nt[2], fpB) > bud) fpB <<= 1;
    bud -= ceil_div(nt[2], fpB);
    int mC = 1; while (mC < 4 && ceil_div(nt[3] - nfar2, 4 * mC) + ceil_div(nfar2, 8 * mC) > bud) mC <<= 1;
    if (fpB > 1 || mC > 1) CT(6, 1);
    for (int t = 0; t < 4; t++) CT(t, nt[t]);
    int calls0 = 0;
#ifdef CIT_PROBE
    calls0 = dbg_cc[0];
#endif
    think_tok = CIT_THINK;
    for (int k = 0; k < steps; k++) {
        tick++;
        party_tick();
        { static int last_min = -1; if (clock_min != last_min) { if (last_min >= 0) party_minute(); last_min = clock_min; } }
        if (pl_cd > 0) pl_cd--;
        think_pri = 1;
        for (int j = 0, e = nt[0] + (fpA ? 0 : nt[1]); j < e; j++) {   // busy with you, and (when they fit) the ones who know you: a step at a time
            int i = j < nt[0] ? lst[j] : lst[NCIT - 1 - (j - nt[0])]; Cit *c = &cits[i];
            if (c->zone != world_zone || c->mode == CM_GONE) continue;
            if (c->owe < 255) c->owe++;
            step_one(c, i, c->owe); CT(4, 1);
        }
#ifdef CIT_PROBE
        if (!(tick & 511)) cit_probe();
#endif
#ifdef CIT_STAND
        if (!(tick % 600) && tick > 1200) {   // (test builds: now and then three people out on the street come near you, having met you once: do they come over?)
            const Sim *p = &sims[player]; int put = 0;
            for (int j = 0; j < nlv && put < 3; j++) { Cit *c = &cits[lv[j]];
                if (c->zone != world_zone || indoors_at(c->x, c->z) || (c->mode != CM_IDLE && c->mode != CM_WALK)) continue;
                place_street(c, p->x, p->z, p->x / TILE, (-p->z) / TILE, 6); c->mode = CM_IDLE; c->t = 0; c->flg &= (u8)~F_HI; meet(c); c->wv = 0; put++; } }
#endif
        if (!fight_on && pl_hp < 100 && !(tick & 63)) pl_hp = MIN(100, pl_hp + 1 + tr_pct(sims[player].tr, E_FIGHT_REGEN) / 100);   // you heal about a point a second
        fam_step();
        if (!(tick & 31)) crowd_react(0);
    }
    // the rest: their turn comes round every fp frames (or straight away, if the clock put them off last frame); the list is
    // started at a different place each frame, so the clock never puts off the same people twice running
    for (int j0 = nlv ? (cfr * 7) % nlv : 0, j = 0; j < nlv; j++) {
        int i = lv[(j0 + j) % nlv]; Cit *c = &cits[i]; int t = c->flg >> F_TSH & 3;
        if (c->zone != world_zone || c->mode == CM_GONE || t < (fpA ? 1 : 2)) continue;
        { int o = c->owe + steps; if (o > OWE_MAX) { CP(15, o - OWE_MAX); o = OWE_MAX; } c->owe = (u8)o; }   // (the steps of this frame, owed to everyone not yet stepped)
        int late = c->flg & F_LATE;
        int fp = t == 1 ? fpA : t == 2 ? fpB : ((c->flg & F_FAR) ? 8 : 4) * mC;
        if (!late && ((cfr + i) & (fp - 1)) && c->owe < OWE_MAX) continue;  // not their frame
        think_pri = t == 1;
        if (t >= 2 && c->owe < OWE_MAX / 2 && VSync(1) - h0 > CIT_HB_CAP) { c->flg |= F_LATE; CT(7, 1); continue; }   // (the frame has run long: next frame; but not for someone already half a second behind)
        c->flg &= (u8)~F_LATE;
        step_one(c, i, c->owe); CT(4, 1);
    }
#ifdef CIT_PROBE
    { int cl = dbg_cc[0] - calls0; if (cl > dbg_cc[11]) dbg_cc[11] = cl; }
#endif
    (void)calls0;
    cur = find_target_l(lv, nlv);   // (a party guest who came in from another zone this frame is on the list from the next)
#ifdef AUTO_CARD   // (test builds: the person nearest to you is the one the card shows; AUTO_CARD & 2: at frame AUTO_CARD_AT they move in with you (the shots of the same person in town and in the household))
    { static int n_; n_++; if (cur < 0) { int bd = 1 << 30; for (int j = 0; j < nlv; j++) { const Cit *c = &cits[lv[j]]; int dx = (c->x - sims[player].x) >> 4, dz = (c->z - sims[player].z) >> 4, d = dx * dx + dz * dz; if (c->zone == world_zone && c->mode != CM_GONE && d < bd) { bd = d; cur = lv[j]; } } }
      if ((AUTO_CARD & 2) && n_ >= AUTO_CARD_AT && !dbg_cardname && cur >= 0 && nsims < MAX_SIMS) { char tb[80]; sprintf(tb, "%s %s", cits[cur].first, cits[cur].last); dbg_cardname = tb[0]; marry_as(&cits[cur], 0); cur = -1; } }
#endif
    fight_on = 0; { int ch = 0, co = 0; for (int j = 0; j < nlv; j++) { const Cit *c = &cits[lv[j]]; if (c->zone == world_zone && c->mode == CM_CHASE) { fight_on = 1; ch++; co += c->cop; } } dbg_rage[0] = ch; dbg_rage[1] = co; }
    for (int i = 0; i < nsims; i++) if (sims[i].rage) fight_on = 1;
    dbg_rage[2] = pl_hp; dbg_rage[3] = heat;
}

// ---- drawing ----------------------------------------------------------------------------------------------
int cit_live_cap = 99, cit_fresh_max = 2, cit_stale_n = 6;   // (main's speed helper, when frames run slow: fewer people in full 3D, fewer first sprites a frame, a stale sprite forced less often)
int cit_spare_hb;                    // this frame's work done only because it fitted the spare time before the vblank (main: the frame's need is the rest)
#define CIT_FLIP_HB 12               // (the flip's own work before it waits for the vblank, and a little give)
int cit_n_full, cit_n_lod, cit_n_spr, cit_n_vis; static int cit_end_hb = -1;
void citizens_draw(void) {
    cit_n_full = cit_n_lod = cit_n_spr = 0; cit_spare_hb = 0;
    if (world_zone == ZONE_HOME && !home_guests) { cit_end_hb = -1; return; }
    int order[MAXDRAW], od[MAXDRAW], n = 0;
    int pin = indoors_at(sims[player].x, sims[player].z);
    for (int i = 0; i < NCIT; i++) {   // the nearest MAXDRAW to where the camera looks, nearest first
        const Cit *c = &cits[i];
        if (c->zone != world_zone || c->mode == CM_GONE) continue;
        if (!c->built) continue;
        int dx = c->x - cam.at.vx, dz = c->z - cam.at.vz, d = dx * dx + dz * dz;
        if (d > DRAW_DIST * DRAW_DIST) continue;
        if (!pin && d > 3 * TILE * 3 * TILE && indoors_at(c->x, c->z)) continue;   // behind walls: not worth the polygons
        if (!gfx_visible(c->x, -110, c->z, 150)) continue;                          // off the screen: nothing to draw
        int cap = MIN(cit_maxdraw, MAXDRAW);
        int pos = n < cap ? n++ : cap;
        if (pos == cap) { if (d >= od[cap - 1]) continue; pos = cap - 1; }
        while (pos > 0 && od[pos - 1] > d) { order[pos] = order[pos - 1]; od[pos] = od[pos - 1]; pos--; }
        order[pos] = i; od[pos] = d;
    }
    // the nearest few (by the eye) are drawn live in 3D; the rest are sprites of the very same model, re-rendered round the
    // crowd as the frame's time allows (the ones that turned, moved or are busy first)
    crowd_frame(); cit_n_vis = n;
    static u8 tier[MAXDRAW]; static VECTOR pp[MAXDRAW]; const int mcap = crowd_capacity();
    static s16 need[MAXDRAW];
    for (int k = 0; k < n; k++) {
        Cit *c = &cits[order[k]];
        int vy = 0;
        if (c->mode == CM_KO) vy = -24;
        else if (c->mode == CM_SLEEP && c->sub == 1) vy = c->mem == 0 ? -items_height('S') + 40 : -24;   // in the bed, or on the floor
        else if (c->mode == CM_CHORE && c->sub == 1 && c->gait.sit) vy = -items_height((char)c->chore) / 2 + 10;   // on the sofa, on the loo
        setVector(&pp[k], c->x, vy - world_gh(c->x, c->z), c->z);   // (on the hills: up the slope they stand on)
        if (k >= mcap && !(k < mcap + 2 && crowd_has(order[k]))) { need[k] = 0; tier[k] = 3; continue; }   // (more people than models fit: the nearest keep theirs, the far ones are drawn plain)
        int h = crowd_need_h(order[k], &pp[k], &c->gait);
        if (!h) { int sx, sy, z = gfx_proj(c->x, vy - 200, c->z, &sx, &sy); h = z ? (300 * 230) / z : 999; }   // (not measured yet: a guess)
        need[k] = (s16)h; tier[k] = 1;
    }
    // live: the biggest on the screen (too big for a sprite cell at 1:1, or near enough that their sprite would go stale fast),
    // the very biggest first, while the frame has time
    static u8 was_live[NCIT];                                         // (a little give, so no one flips between the two)
    for (int live = 0; live < MIN(MIN(cit_live_max, cit_live_cap), LIVE_MAX); live++) {
        int best = -1;
        for (int k = 0; k < n; k++) if (tier[k] == 1 && need[k] > (was_live[order[k]] ? 100 : 112) && crowd_has(order[k]) && (best < 0 || need[k] > need[best])) best = k;
        if (best < 0 || (live >= 2 && gfx_hb() + live * 36 > 340)) break;
        tier[best] = 0;
    }
    for (int k = 0; k < n; k++) was_live[order[k]] = tier[k] == 0;
    for (int k = 0; k < n; k++) if (tier[k] == 0) {
        Cit *c = &cits[order[k]];
        PROF_BEGIN(); crowd_draw_live(order[k], &pp[k], c->yaw, &c->gait); PROF_END(5); cit_n_full++;
    }
    static int score[MAXDRAW];                                        // how out of date each sprite is (worked out once a frame)
    for (int k = 0; k < n; k++) score[k] = tier[k] == 1 ? crowd_sprite_score(order[k], &pp[k], cits[order[k]].yaw, &cits[order[k]].gait) : 0;
    // THE FRAME'S SPARE TIME. It ends on a vblank either way, so the time between its own work and that vblank is free. What is
    // still to do after this point (the sprites and shadows below, the speech bubbles, the HUD, the flip) is taken from the last
    // frame (the most of it lately); the vblank is the one the frame's own work reaches, never sooner than the FRAME RATE option
    // lets it. The spare time goes to a body someone is waiting for, then to the most out of date sprites, each one only if it
    // still fits by the clock (a count worked out up front, as it was, let a frame run past its vblank now and then, and the work
    // after this point was not counted at all). Someone with no sprite yet (drawn plain meanwhile) is rendered when it fits, two
    // a frame at most, and at least every 3rd frame; one stale sprite at least every 6th, so nobody stays out of date for long.
    static int rest = 160, rest_lo = 100, skipped, fskip;           // (the time after this point: the most of it lately, and the least)
    if (cit_end_hb >= 0 && gfx_last_cpu > cit_end_hb) { int r_ = gfx_last_cpu - cit_end_hb; rest = r_ > rest ? r_ : rest - ((rest - r_) >> 3); rest_lo = r_ < rest_lo ? r_ : rest_lo + ((r_ - rest_lo) >> 3); }
    int now = gfx_hb(), end = now + rest;
    // the vblank: the first one the frame's own work does not clearly run past, from the least it could still need (filling a
    // frame on to the next vblank when it would have made this one costs a whole vblank; guessing too early only leaves time unused)
    int vb = (now + rest_lo + CIT_FLIP_HB - 36 + 262) / 263; if (vb < gfx_min_vsyncs) vb = gfx_min_vsyncs;   // (24 hb past it: clearly)
    int target = vb * 2625 / 10 - CIT_FLIP_HB;
    if (cit_sprite_rate == 0) target = end + (target - end) / 2;     // (SPRITES: SLOW: half the spare time)
    { extern int world_stream_want, world_stream_hb; if (world_stream_want) target -= world_stream_hb + 16; }   // (the map window's move waiting for spare time comes first)
    cit_spare_hb = cit_spare_build(target - end);                     // a body someone is waiting for
    int est = MAX(20, crowd_render_hb) + 4, fresh = 0, nr = 0, over = 0, waiting = 0;
    for (int r = 0; r < 12; r++) {
        int best = -1, bs = 4;
        for (int k = 0; k < n; k++) if (tier[k] == 1 && score[k] > bs) { bs = score[k]; best = k; }
        if (best < 0) break;
        int first = bs >= 100000, t0 = gfx_hb(), fits = t0 + rest + est <= target;
        if (first) { waiting = 1; if (fresh >= cit_fresh_max) break; }
        if (!fits) {   // (no time for it: only the one a frame that keeps everyone from going stale, or FAST's one more)
            if (over || !(cit_sprite_rate == 2 || (first ? fskip >= 3 : skipped >= cit_stale_n))) break;
            over = 1;
        }
        Cit *c = &cits[order[best]];
        PROF_BEGIN(); crowd_sprite(order[best], &pp[best], c->yaw, &c->gait, 1); PROF_END(6);
        tier[best] = 2;
        if (first) fresh++; else nr++;
        if (fits) cit_spare_hb += gfx_hb() - t0;
#ifdef AUTO_CITY
        { extern int dbg_rn[3]; dbg_rn[fits ? 0 : first ? 1 : 2]++; }   // (test readout: renders in spare time; forced: first sprites, stale ones)
#endif
    }
    if (nr) skipped = 0; else skipped++;
    if (fresh || !waiting) fskip = 0; else fskip++;
    for (int k = 0; k < n; k++) {
        Cit *c = &cits[order[k]];
        if (tier[k] == 3) { PROF_BEGIN(); voxel_draw_lod(&c->lod, &pp[k], c->yaw, &c->gait); cit_n_lod++; PROF_END(6); }
        if (tier[k] == 1) {
            PROF_BEGIN();
            if (!crowd_sprite(order[k], &pp[k], c->yaw, &c->gait, 0)) { voxel_draw_lod(&c->lod, &pp[k], c->yaw, &c->gait); cit_n_lod++; } else cit_n_spr++;   // (no model yet)
            PROF_END(6);
        }
        if (c->mode != CM_KO && c->mode != CM_SLEEP) { VECTOR sp; setVector(&sp, c->x, 0, c->z); gfx_shadow(&sp, 70); }
    }
    cit_end_hb = gfx_hb();
}

void citizens_draw_bubbles(void) {
    if (world_zone == ZONE_HOME && !home_guests) return;
    for (int i = 0; i < NCIT; i++) {
        const Cit *c = &cits[i];
        if (c->zone != world_zone || c->mode == CM_GONE || !c->say_t || !c->say) continue;
        int dx = c->x - cam.at.vx, dz = c->z - cam.at.vz;
        if (dx * dx + dz * dz > DRAW_DIST * DRAW_DIST) continue;
        VECTOR p; setVector(&p, c->x, 0, c->z);
        gfx_set_object(&p, 0);
        SVECTOR top; setVector(&top, 0, -H * VOX - 40, 0);
        int sx, sy, sz;
        if (!gfx_project(&top, &sx, &sy, &sz)) continue;
        ui_bubble(gfx_ui_pre(sx), sy, c->say, UI_INK);
    }
}

static const char *stage_nm(int s) { return s == AG_CHILD ? "KID" : s == AG_TEEN ? "TEEN" : s == AG_ELDER ? "ELDER" : "ADULT"; }
static void bar(int x, int y, int w, int v, int r, int g, int b) {
    gfx_rect(x, y, w, 4, 30, 30, 36, 0);
    gfx_rect(x + 1, y + 1, (w - 2) * CLAMP(v, 0, 100) / 100, 2, r, g, b, 0);
}
int citizens_card_on(void) { return !(world_zone == ZONE_HOME && !home_guests) && cur >= 0; }   // (main: the notes go under it)
void citizens_persona_text(u32 seed, int stage, char *l1, char *l2) {   // "ARIES, FORTUNE" and "OUTGOING 9, NEAT 2": the person's personality for a card (from the seed, nothing stored)
    Persona p; persona_of(seed, stage, &p);
    int i1 = 0, i2 = -1;
    for (int k = 1; k < TR_N; k++) if (ABS(p.bar[k] - 5) > ABS(p.bar[i1] - 5)) i1 = k;
    for (int k = 0; k < TR_N; k++) if (k != i1 && (i2 < 0 || ABS(p.bar[k] - 5) > ABS(p.bar[i2] - 5))) i2 = k;
    sprintf(l1, "%s, %s", look_sign_name(p.sign), look_asp_name(p.asp));
    sprintf(l2, "%s %d, %s %d", look_trait_name(i1), p.bar[i1], look_trait_name(i2), p.bar[i2]);
}
COLD void citizens_hud(void) {
    if (world_zone == ZONE_HOME && !home_guests) return;
    const int X = 8, Y = 37;                                       // (on the left, under the household: the notes go under it)
    if (cur >= 0) {
        const Cit *c = &cits[cur];
        char t[40], l1[40], l2[40]; sprintf(t, "%s %s", c->first, c->last);   // a white card (The Sims 4's): who they are, how they are to you
        citizens_persona_text(c->seed, c->stage, l1, l2);
        ui_card(X, Y, 150, 67);                                    // (the name, their personality: sign and wish, the two strongest bars, no traits yet; how they are to you, what they do)
        ui_icon_disc(X + 12, Y + 11, 16, IC_CHAT, UI_BLUE, UI_WHITE);
        ui_text(X + 24, Y + 3, 1, t, UI_INK);
        sprintf(t, "%s, %s", stage_nm(c->stage), l1); if (ui_tw(1, t) > 122) strcpy(t, l1);
        ui_text(X + 24, Y + 12, 1, t, UI_SOFT);
        ui_text(X + 6, Y + 22, 1, l2, UI_INK);
        ui_text(X + 6, Y + 31, 1, "NO TRAITS YET: THEY COME", UI_RGB(150, 100, 30));
        ui_text(X + 6, Y + 39, 1, "WHEN THEY MOVE IN", UI_RGB(150, 100, 30));
        ui_text(X + 6, Y + 48, 1, rel_name(c->rel), UI_RGB(40, 150, 50));
        ui_bar(X + 70, Y + 48, 74, 3, c->rel > 0 ? c->rel : 0, UI_GREEN);
        ui_bar(X + 70, Y + 52, 74, 3, c->rom, UI_RGB(236, 100, 160));
        { char jb[40]; const char *d = (c->pid != NOPID && c->mplace != NOPID && c->mode == CM_IDLE) ? pop_job_name(c->pid, jb) : activity(c);
          if (d != jb) { strncpy(jb, d, 39); jb[39] = 0; }
          if (ui_tw(1, jb) > 138) { char *k = strchr(jb, ','); if (k) *k = 0; }
          ui_text(X + 6, Y + 57, 1, jb, UI_BLUE2); }
    }
    if (fight_on || pl_hp < 100) { int hy = cur >= 0 ? Y + 69 : Y; ui_card(X, hy, 70, 11); ui_bar(X + 4, hy + 2, 62, 7, pl_hp, pl_hp > 50 ? UI_GREEN : pl_hp > 25 ? UI_RGB(236, 176, 40) : UI_RGB(228, 84, 70)); }
    if (fight_on) ui_hintbar(146, "X PUNCH");
}

#ifdef CIT_TEST
void citizens_debug(int zone, int *in, int *out, int *asleep, int *transit, int *homeless, int *bad) {
    *in = *out = *asleep = *transit = *homeless = *bad = 0;
    for (int i = 0; i < NCIT; i++) { Cit *c = &cits[i]; if (c->zone != zone) continue;
        if (c->hh == NONE) { (*homeless)++; }
        int ind = indoors_at(c->x, c->z);
        if (c->mode == CM_SLEEP) (*asleep)++; else if (c->mode == CM_GOHOME || c->mode == CM_LEAVE) (*transit)++; else if (ind) (*in)++; else (*out)++;
        const Home *hm = home_of(c);
        if (hm && ind) { int mx = c->x / TILE, my = (-c->z) / TILE; if (!(mx >= hm->x0 && mx <= hm->x1 && my >= hm->y0 && my <= hm->y1) && !(mx == hm->dx && my == hm->dy)) (*bad)++; }
        if (!ind && !body_free(c->x, c->z, 0) && c->mode != CM_GOHOME && c->mode != CM_LEAVE) (*bad)++;
    }
}
#endif
void citizens_rebuild(void) { for (int i = 0; i < NCIT; i++) cits[i].built = 0; }   // (the crowd's arena was lent out: every model is made again, nearest first)
COLD int citizens_send_buyer(int tx, int ty) {   // FARM LIFE: someone out and about near the farm stand at tile (tx, ty) comes over to buy (the ones who know you first); 1: sent
    int best = -1, bs = 1 << 30; GB(0);
    for (int i = 0; i < NCIT; i++) { const Cit *c = &cits[i];
        if (c->zone != world_zone || (c->mode != CM_IDLE && c->mode != CM_WALK) || c->stage < AG_TEEN || i == pie_c || indoors_at(c->x, c->z)) continue;
        int dx = c->x / TILE - tx, dy = (-c->z) / TILE - ty, d = dx * dx + dy * dy; if (d > 24 * 24) continue;
        if (cit_known(c)) d >>= 2;
        if (d < bs) { bs = d; best = i; } }
    if (best < 0) return 0;
    GB(1); IFLE(dbg_buy[4] = best); Cit *c = &cits[best]; c->mode = CM_BUY; c->ix = (u8)tx; c->iy = (u8)ty; c->t = 900; c->st = 0; c->wv = 0; return 1;
}
#ifdef AUTO_LE
int dbg_greet[6], dbg_buy[6];   // (test builds) farm stand customers: asked for, sent, got there, gave up. And people who know you: set off to say hello, got to you; gave up: the time, held up, you too far
int citizens_dbg_walk_up(int slot, int want, int far) {   // (test builds) walk up to that one (slot -1: the nearest grown-up out of doors and awake who likes you less
    // than want): in front of them (far 0), or on the pavement far tiles off (passing by, on a walk round town). Their slot, or -1
    Sim *p = &sims[player]; int best = slot >= 0 && slot < NCIT && cits[slot].zone == world_zone && cits[slot].mode != CM_GONE ? slot : -1, bd = 1 << 30;
    for (int i = 0; i < NCIT && slot < 0; i++) { const Cit *c = &cits[i];
        if (c->zone != world_zone || c->mode == CM_GONE || c->mode == CM_KO || c->mode == CM_SLEEP || c->mode == CM_CHAT || c->rel >= want || c->stage < AG_TEEN || indoors_at(c->x, c->z)) continue;
        int dx = c->x - p->x, dz = c->z - p->z, d = dx * dx + dz * dz; if (d < bd) { bd = d; best = i; } }
    if (best < 0) return -1;
    Cit *c = &cits[best];
    if (!far) { p->x = c->x - TILE * 2 / 3; p->z = c->z; p->yaw = 1024; c->mode = CM_IDLE; c->t = 300; return best; }
    if (indoors_at(c->x, c->z)) return -1;   // (on the pavement, far tiles off or as near that as there is: where a walk round town goes)
    int cx = c->x / TILE, cy = (-c->z) / TILE, ux = -1, uy = -1, bs = 99;
    for (int y = cy - far; y <= cy + far; y++) for (int x = cx - far; x <= cx + far; x++) {
        if (x < 1 || y < 1 || x >= MW - 1 || y >= MH - 1 || floorMap[y][x] != FL_PAVING || !world_walkable(x, y)) continue;
        int d = MAX(ABS(x - cx), ABS(y - cy)); if (d >= 3 && far - d < bs) { bs = far - d; ux = x; uy = y; } }
    if (ux < 0) return -1;
    p->x = ux * TILE + TILE / 2; p->z = -(uy * TILE + TILE / 2); return best;
}
int citizens_dbg_rel(int slot) { return slot >= 0 && slot < NCIT ? cits[slot].rel : -999; }
int citizens_dbg_pid(int slot) { return slot >= 0 && slot < NCIT ? cits[slot].pid : -1; }
int citizens_dbg_state(int slot) { if (slot < 0 || slot >= NCIT) return -1; const Cit *c = &cits[slot]; int dx = (c->x - sims[player].x) / TILE, dz = (c->z - sims[player].z) / TILE;
    return c->mode | MIN(99, ABS(dx) + ABS(dz)) << 4 | (c->flg & 7) << 11 | indoors_at(c->x, c->z) << 14 | indoors_at(sims[player].x, sims[player].z) << 15 | MIN(c->t, 4095) << 16; }
#endif
