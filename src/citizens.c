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

enum { CM_IDLE, CM_WALK, CM_CHAT, CM_CHASE, CM_KO, CM_GONE, CM_SLEEP, CM_GOHOME, CM_LEAVE, CM_CHORE, CM_VISIT, CM_PARTY };
enum { PE_FRIENDLY, PE_GRUFF, PE_FLIRTY, PE_SHY };
enum { PO_TALK, PO_JOKE, PO_HUG, PO_FLIRT, PO_GIFT, PO_PROPOSE, PO_FIGHT, PO_SORRY, PO_FAMILY };
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
    u16  st;                        // steps spent held up
    u8   wx, wy, gx, gy, wv;        // the next tile of a routed walk, the goal tile it was routed to, whether it is current
    u8   path[8], plen, pi;         // the route itself, 2 bits a step (worked out once per goal, not at every tile)
    u8   chore, chain, chx, chy, ix, iy, vh;   // a chore at home: the item letter, the one that follows it, where they stand, where the item is; a visit: the door tile and the household
    u8   zone0;                     // a party guest from another zone: where to go back to (255: none)
    u16  pid, mplace;               // SAN BOREAS: who they are in pop.c (NOPID: one of the island's or the town's own), and where they were brought to life (NOPID: home)
    u8   ax, ay, vanish;            // ... the place they hang about (a tile), and 1 when they are wanted elsewhere
} Cit;
typedef struct { u8 zone, home, n, fam; u8 mem[6]; u16 pfam; } Hh;   // a household: who lives in which of the zone's homes, under which surname (pfam: its pop.c family, or NOPID)

static Cit cits[NCIT];
static Hh hh[NHH]; static int nhh; static u8 formed[ZONE_N];
// (a citizen is built in crowd.c's unpacking Bore: it is only ever needed between one unpack and the drawing that follows)
static void party_leave_done(Cit *c);
static int party_on, party_left, party_cx, party_cy, party_in, party_spx, party_spz, party_n, party_next, party_t, home_guests;
#define PARTY_MAX 60
static u8 party_g[PARTY_MAX], party_sx[PARTY_MAX], party_sy[PARTY_MAX];
static int tick, pl_hp = 100, pl_cd, cur = -1, fight_on, moved_flag;
static u32 crng = 0x9E3779B9u;
static char pie_title[NAME_LEN + 2];
static u8 pie_code[8]; static int pie_c = -1;

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
static const char *chat_line(const Cit *c) {
    if (indoors_at(c->x, c->z) && !(crnd() & 1) && c->pers != PE_GRUFF) return L_HOME[rn(NL(L_HOME))];   // in their own house
    if (c->stage <= AG_TEEN) return L_KID[rn(NL(L_KID))];
    if (c->pers == PE_GRUFF) return L_GRUFF[rn(NL(L_GRUFF))];
    if (c->stage == AG_ELDER && (crnd() & 1)) return L_ELD[rn(NL(L_ELD))];
    return L_FRIEND[rn(NL(L_FRIEND))];
}

// ---- bodies ----------------------------------------------------------------------------------------
static void cit_bore(Bore *b, const Cit *c) {   // the same person, every time, from the seed (b may already own its face textures)
    memset(b->look, 0, sizeof b->look);
    b->stage = c->stage;
    u32 old = look_seed(c->seed * 2654435761u + 12345u);
    look_random(b);
    look_seed(old);
    if (c->pid != NOPID) { pop_first(c->pid, b->first); pop_surname(pop_per[c->pid].fam, b->last); return; }   // SAN BOREAS: their own names
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
    c->built = 1;
    #undef scratch
}

// ---- homes and households -----------------------------------------------------------------------------
static const Home *home_of(const Cit *c) { return c->hh == NONE ? 0 : world_home(c->zone, hh[c->hh].home); }
static void door_geo(const Home *hm, int *x, int *zf, int *zd, int *zi) {   // the door: where you stand outside it, in it, and just inside
    *x = hm->dx * TILE + TILE / 2;
    *zf = -((hm->dy + 1) * TILE + TILE / 2); *zd = -(hm->dy * TILE + TILE / 2); *zi = -((hm->dy - 1) * TILE + TILE / 2);
}
static void inside_spot(const Home *hm, int *x, int *z) {   // somewhere free on the floor
    for (int t = 0; t < 40; t++) {
        int tx = hm->x0 + rn(hm->x1 - hm->x0 + 1), ty = hm->y0 + rn(hm->y1 - hm->y0 + 1);
        if (ty >= hm->dy - 1 && tx == hm->dx) continue;       // keep the lane inside the door open
        if (world_at(tx, ty) == '.' && world_walkable(tx, ty)) { *x = tx * TILE + TILE / 2; *z = -(ty * TILE + TILE / 2); return; }
    }
    int a, b, c2; door_geo(hm, x, &a, &b, &c2); *z = c2;
}
static void sleep_spot(const Cit *c, const Home *hm, int *x, int *z) {   // a bed each (a double bed for a couple, two to a bunk bed); anyone left lies down on the floor
    int k = 0, want_ = c->mem, couple = 0;
    if (c->pid != NOPID) { u8 a, kd, t, e; pop_family_kinds(pop_per[c->pid].fam, &a, &kd, &t, &e, (u8 *)&couple); couple &= 1; if (couple && want_ > 0) want_--; }
    for (int y = hm->y0; y <= hm->y1; y++) for (int xx = hm->x0; xx <= hm->x1; xx++) {
        char ch = world_at(xx, y); if (ch != 'S' && ch != '<') continue;
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
static void form_households(int zone) {   // the first time a zone is seen: its people sort themselves into the homes the generator built
    if (zone == ZONE_CITY || formed[zone]) return;   // (SAN BOREAS's households come from pop.c)
    formed[zone] = 1;
    int nh = world_home_count(zone), idx[NCIT], n = 0;
    for (int i = 0; i < NCIT; i++) if (cits[i].zone == zone) idx[n++] = i;
    if (!nh || !n) return;
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
void citizens_init(void) {
    crng ^= (u32)look_rand() * 2246822519u;
    if (!crng) crng = 0x9E3779B9u;
    memset(cits, 0, sizeof cits);
    for (int i = 0; i < NCIT; i++) {
        Cit *c = &cits[i];
        c->seed = crnd();
        c->zone = i < 10 ? ZONE_ISLAND : i < 50 ? ZONE_TOWN : 255;          // 10 on the island, 40 in town; the rest wait for SAN BOREAS's people
        c->pid = NOPID; c->mplace = NOPID; c->vanish = 0;
        int r = rn(100);
        c->stage = r < 8 ? AG_CHILD : r < 20 ? AG_TEEN : r < 78 ? AG_ADULT : AG_ELDER;   // (until a household is formed round them)
        c->pers = (u8)rn(4);
        c->hh = NONE; c->off = (s8)(rn(91) - 45); c->sub = 0; c->st = 0;
        c->hp = 100; c->mode = CM_IDLE; c->slot = 0; c->built = 0; c->zone0 = 255;
        strcpy(c->first, "..."); c->last[0] = 0;
        if (i >= MAIN0) c->mode = CM_GONE;
    }
    memset(hh, 0, sizeof hh); nhh = 0; memset(formed, 0, sizeof formed);
    for (int k = 0; k < NHH; k++) { hh[k].pfam = NOPID; hh[k].zone = 255; }
    crowd_reset();
    pl_hp = 100; pl_cd = 0; cur = -1; fight_on = 0; pie_c = -1;
}

static void place_street(Cit *c, int px, int pz, int cx, int cz, int radius) {   // somewhere out on the streets near (cx,cz) tiles
    for (int tries = 0; tries < 300; tries++) {
        int mx = cx + rn(2 * radius + 1) - radius, my = cz + rn(2 * radius + 1) - radius;
        if (mx < 2 || my < 2 || mx >= MW - 2 || my >= MH - 2) continue;
        int x = mx * TILE + TILE / 2, z = -(my * TILE + TILE / 2);
        if (!open_at(x, z)) continue;
        if (ABS(x - px) + ABS(z - pz) < 4 * TILE) continue;
        c->x = (s16)x; c->z = (s16)z; return;
    }
}
static void place_initial(Cit *c, int px, int pz) {   // where they are right now, by the clock
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
static int fam_hh(int f) {                        // the household entry for a pop.c family (made when the first of them turns up)
    for (int k = 0; k < NHH; k++) if (hh[k].pfam == f && hh[k].zone == ZONE_CITY) return k;
    const Fam *fm = &pop_fam[f];
    int home = world_home_find(ZONE_CITY, fm->bx, fm->by, fm->unit); if (home < 0) return -1;
    for (int k = NHH - 1; k >= nhh; k--) if (hh[k].zone == 255) {
        Hh *g = &hh[k]; g->zone = ZONE_CITY; g->home = (u8)home; g->n = 0; g->fam = 0; g->pfam = (u16)f; return k; }
    return -1;
}
static void hh_drop(Cit *c) {                     // out of their household's list (the entry goes when the last one does)
    if (c->hh == NONE) return;
    Hh *g = &hh[c->hh]; int ci = (int)(c - cits);
    for (int k = 0; k < g->n; k++) if (g->mem[k] == ci) { g->mem[k] = g->mem[g->n - 1]; g->n--; break; }
    if (g->pfam != NOPID && !g->n) { g->zone = 255; g->pfam = NOPID; }
    c->hh = NONE;
}
static void main_free(Cit *c) {                   // back into pop.c: what they think of you goes with them
    int i = (int)(c - cits);
    if (c->pid != NOPID) { Per *q = &pop_per[c->pid]; q->rel = c->rel; q->rom = c->rom; if (!(q->flags & PF_GONE) || q->hp) q->hp = c->hp; q->slot = 255; }   // (hp 0 and gone: passed away)
    hh_drop(c);
    if (cur == i) cur = -1;
    crowd_forget(i);
    c->pid = NOPID; c->mplace = NOPID; c->vanish = 0; c->zone = 255; c->mode = CM_GONE; c->built = 0; c->say_t = 0; c->zone0 = 255;
}
static int place_tile(const Place *pl, int worker, int *tx, int *ty, int *indoor) {   // somewhere to stand at a place in the window
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
int citizens_take(int pid, int at, int place) {
    int slot = -1;
    for (int i = MAIN0; i < NCIT; i++) if (cits[i].zone == 255 && cits[i].pid == NOPID) { slot = i; break; }
    if (slot < 0) { dbg_tf[0]++; return -1; }
    Per *q = &pop_per[pid]; Cit *c = &cits[slot];
    int px = sims[player].x, pz = sims[player].z;
    memset(&c->gait, 0, sizeof c->gait);
    c->seed = q->seed; c->stage = q->stage; c->rel = q->rel; c->rom = q->rom; c->hp = q->hp ? q->hp : 100;
    c->pers = q->trait == 1 ? PE_GRUFF : q->trait == 6 ? PE_FLIRTY : (q->trait == 8 || q->trait == 13) ? PE_SHY : PE_FRIENDLY;
    c->homebody = (q->flags & PF_HOMEBODY) ? 1 : 0; c->off = pop_offset(pid);
    c->zone = ZONE_CITY; c->zone0 = 255; c->mode = CM_IDLE; c->t = (u16)(10 + rn(150)); c->sub = 0; c->st = 0; c->wv = 0; c->say_t = 0; c->cd = 0; c->slot = 0;
    c->pid = (u16)pid; c->vanish = 0; c->hh = NONE; c->mem = 0; c->built = 0;
    crowd_forget(slot);
    if (place < 0) {                               // at home: in their household, and wherever the clock puts them in it
        int k = fam_hh(q->fam); if (k < 0) { dbg_tf[1]++; c->zone = 255; c->pid = NOPID; c->mode = CM_GONE; return -1; }
        Hh *g = &hh[k]; if (g->n >= 6) { dbg_tf[2]++; c->zone = 255; c->pid = NOPID; c->mode = CM_GONE; return -1; }
        c->hh = (u8)k; c->mem = (u8)(pid - pop_fam[q->fam].first); g->mem[g->n++] = (u8)slot;
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
void citizens_shift(int dtx, int dty) {
    for (int i = 0; i < NCIT; i++) {
        Cit *c = &cits[i];
        if (c->zone != ZONE_CITY || c->mode == CM_GONE) continue;
        int x = c->x - dtx * TILE, z = c->z + dty * TILE;
        int ax = c->ax - dtx, ay = c->ay - dty;
        if (x < TILE || -z < TILE || x >= (MW - 1) * TILE || -z >= (MH - 1) * TILE) { if (c->pid != NOPID) { main_free(c); continue; } x = CLAMP(x, TILE, (MW - 2) * TILE); z = CLAMP(z, -(MH - 2) * TILE, -TILE); }
        c->x = (s16)x; c->z = (s16)z; c->ax = (u8)CLAMP(ax, 0, MW - 1); c->ay = (u8)CLAMP(ay, 0, MH - 1);
        c->wv = 0;                                 // routes and errands are worked out again
        if (c->mode == CM_CHORE || c->mode == CM_VISIT || c->mode == CM_GOHOME || c->mode == CM_LEAVE) { c->mode = CM_IDLE; c->t = 20; c->gait.sit = 0; }
        if (c->mode == CM_SLEEP && c->sub == 0) c->sub = 0;
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
static void vanish_tick(void) {                    // the ones wanted elsewhere go, once nobody can see them
    for (int i = MAIN0; i < NCIT; i++) {
        Cit *c = &cits[i];
        if (!c->vanish || c->zone != ZONE_CITY || c->mode == CM_PARTY || c->mode == CM_CHAT || c->mode == CM_CHASE || i == pie_c) continue;
        int dx = c->x - cam.at.vx, dz = c->z - cam.at.vz;
        if (!gfx_visible(c->x, -110, c->z, 160) || dx * dx + dz * dz > 16 * TILE * 16 * TILE) main_free(c);
    }
}

void citizens_enter(int zone) {
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
        c->x = 0; c->z = 0;
        place_initial(c, px, pz);
    }
    fight_on = 0;
}

// ---- who is in front of you ---------------------------------------------------------------------------
static int find_target(void) {
    const Sim *p = &sims[player]; int best = -1, bd = 1 << 30;
    for (int i = 0; i < NCIT; i++) {
        const Cit *c = &cits[i];
        if (c->zone != world_zone || c->mode == CM_GONE || c->mode == CM_KO || !c->built) continue;
        int dx = c->x - p->x, dz = c->z - p->z;
        if (ABS(dx) > REACH || ABS(dz) > REACH) continue;
        int d = dx * dx + dz * dz;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
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
    if (hostile_in_reach() >= 0) return "PUNCH";
    if (cur < 0) return "";
    return cits[cur].mode == CM_SLEEP ? "WAKE" : "TALK";
}

// ---- the pie menu -------------------------------------------------------------------------------------
const char *citizens_pie_title(void) { return pie_title; }
int citizens_pie_open(char opt[8][16]) {
    int i = find_target(); if (i < 0) return 0;
    Cit *c = &cits[i]; pie_c = i;
    strcpy(pie_title, c->first);
    int n = 0;
    #define OPT(txt, code) { strcpy(opt[n], txt); pie_code[n++] = (code); }
    if (c->mode == CM_CHASE) { OPT("SORRY!", PO_SORRY) return n; }
    if (c->mode == CM_SLEEP) { OPT("WAKE THEM UP", PO_TALK) return n; }
    OPT("TALK", PO_TALK) OPT("JOKE", PO_JOKE)
    if (c->hh != NONE) OPT("FAMILY", PO_FAMILY)
    if (c->rel >= 25) OPT("HUG", PO_HUG)
    if (adult(c) && pl_adult() && c->rel >= 30) OPT("FLIRT", PO_FLIRT)
    OPT("GIVE $20 GIFT", PO_GIFT)
    if (adult(c) && pl_adult() && c->rom >= 60 && c->rel >= 60) OPT("PROPOSE $150", PO_PROPOSE)
    if (adult(c) && pl_adult()) OPT("FIGHT", PO_FIGHT)
    #undef OPT
    return n;
}

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

static void marry(Cit *c) {   // they move in: the civilian becomes a housemate with the full model
    Bore *wedp = crowd_temp_bore();
    #define wed (*wedp)
    memset(&wed, 0, sizeof wed);
    cit_bore(&wed, c);        // fresh face textures (the pool's are shared), the same person from the same seed
    const Sim *p = &sims[player];
    int tx = p->x / TILE, ty = (-p->z) / TILE;
    sim_add(&wed, tx, ty, 1);
    Sim *w = &sims[nsims - 1];
    w->rel[player] = 100; sims[player].rel[nsims - 1] = 100;
    w->partner = sims[player].uid; sims[player].partner = w->uid;   // (the family tree: partners)
    w->x = p->x + 70; w->z = p->z;
    c->mode = CM_GONE; c->say_t = 0;
    if (c->pid != NOPID) { pop_per[c->pid].flags |= PF_GONE; main_free(c); }
    char b[32]; sprintf(b, "MARRIED %s!", w->bore.first); sim_note(b);
    audio_voice(VOICE_WIN);
    #undef wed
}

void citizens_pie_do(int i) {
    if (pie_c < 0 || i < 0) return;
    Cit *c = &cits[pie_c];
    if (c->mode == CM_GONE || c->mode == CM_KO) return;
    Sim *p = &sims[player];
    if (c->mode == CM_SLEEP) {   // you woke them up
        c->mode = CM_IDLE; c->t = 60; c->gait.sleep = 0; c->sub = 0; rel_add(c, -6);
        say(c, "HUH? WHAT?"); sim_moodlet(p, CML_REJECTED); audio_voice(VOICE_GROAN); return;
    }
    switch (pie_code[i]) {
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
        if (nsims >= MAX_SIMS) { sim_note("NO ROOM AT HOME"); audio_voice(VOICE_NAH); break; }
        if (cash < RING_COST) { sim_note("A RING IS $150"); audio_voice(VOICE_NAH); break; }
        {   int ch = 55 + c->rel / 4 + c->rom / 4 + (c->pers == PE_FLIRTY ? 10 : c->pers == PE_SHY ? -15 : 0);
            if (rn(100) < ch) { cash -= RING_COST; marry(c); }
            else { rom_add(c, -25); rel_add(c, -10); say(c, "I'M NOT READY..."); sim_moodlet(p, CML_REJECTED); audio_voice(VOICE_CRY); } }
        break;
    case PO_FIGHT:
        c->mode = CM_CHASE; c->hp = (u8)(c->stage == AG_ELDER ? 35 : 55 + rn(35)); c->cd = 30; c->t = 0;
        rel_add(c, -10); rom_add(c, -20); say(c, "BRING IT!"); fight_on = 1; audio_voice(VOICE_ANGRY); story_event(E_FIGHT, 0); break;
    case PO_SORRY:
        if (rn(100) < 55) { c->mode = CM_IDLE; c->t = 60; say(c, "...FINE."); }
        else { say(c, "TOO LATE!"); audio_voice(VOICE_DISAGREE); }
        break;
    }
}

int citizens_friends(void) {   // (stories) the people out there who are your friends (50 and up): the ones about now, and the rest of SAN BOREAS
    int n = 0;
    for (int i = 0; i < NCIT; i++) { const Cit *c = &cits[i]; if (c->rel < 50) continue;
        if (c->pid != NOPID ? pop_per[c->pid].slot == i : (i < MAIN0 && c->mode != CM_GONE)) n++; }   // (a city one alive now: its live feelings, not the saved ones)
    for (int p = 0; p < pop_nper; p++) { const Per *q = &pop_per[p]; if (q->rel >= 50 && q->slot == 255 && !(q->flags & PF_GONE)) n++; }
    return n;
}
// ---- fighting ---------------------------------------------------------------------------------------------
int citizens_punch(void) {
    int i = hostile_in_reach(); if (i < 0) return 0;
    Cit *c = &cits[i]; Sim *p = &sims[player];
    if (pl_cd > 0) return 1;
    pl_cd = 18;
    p->gait.wave = 14;
    int dx = c->x - p->x, dz = c->z - p->z;
    p->yaw = yaw_to(dx, dz);
    int dmg = 9 + rn(8);
    c->hp = (u8)MAX((int)c->hp - dmg, 0);
    c->gait.squash = -12;
    { int d = MAX(ABS(dx) + ABS(dz), 1), kx = dx * 40 / d, kz = dz * 40 / d, in = indoors_at(c->x, c->z);   // a shove back
      if (body_free(c->x + kx, c->z + kz, in)) { c->x += (s16)kx; c->z += (s16)kz; } }
    audio_voice(VOICE_SHRIEK);
    if (c->hp == 0) {
        c->mode = CM_KO; c->t = 900; c->gait.sleep = 1; c->gait.amp = 0;
        int loot = 5 + rn(21); cash += loot;
        char b[32]; sprintf(b, "KO! +$%d", loot); sim_note(b);
        rel_add(c, -30); say(c, "OOF..."); audio_voice(VOICE_WIN);
        if (hostile_in_reach() < 0) fight_on = 0;
    } else say(c, rn(2) ? "OW!" : "MY NOSE!");
    return 1;
}

// ---- walking: round things, through a door, to a bed ------------------------------------------------------
#ifdef CIT_TEST
int cit_snaps, cit_go; int cit_by[12][4];
#endif
static int go(Cit *c, int tx, int tz, int spd, int in) {   // walk towards a point, round things; 1 when there. A body held up too long is simply put there.
    int dx = tx - c->x, dz = tz - c->z;
    if (ABS(dx) + ABS(dz) < 40) return 1;
    turn(c, yaw_to(dx, dz), 170);
    int mx = (isin(c->yaw) * spd) >> 12, mz = -((icos(c->yaw) * spd) >> 12);
    if (body_free(c->x + mx, c->z + mz, in)) { c->x += (s16)mx; c->z += (s16)mz; c->st = 0; moved_flag = 1; }
    else if (mx && body_free(c->x + mx * 2, c->z, in)) { c->x += (s16)(mx * 2); c->st = 0; moved_flag = 1; }
    else if (mz && body_free(c->x, c->z + mz * 2, in)) { c->z += (s16)(mz * 2); c->st = 0; moved_flag = 1; }
    else if (++c->st > 150) {
#ifdef CIT_TEST
        cit_snaps++; cit_by[c->mode][c->sub & 3]++;
#endif
        c->x = (s16)tx; c->z = (s16)tz; c->st = 0; return 1; }
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
            if (!tile_ok(a, b, in) && !(a == sx && b == sy)) continue;
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
    int nx, ny;
    if (!route_next(gx, gy, sx, sy, in, &nx, &ny)) return 0;              // (fills bdist from the goal)
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
static int nav(Cit *c, int gtx, int gty, int spd, int in) {   // walk to a tile round the furniture (or the neighbours' houses); 1 when there
    int sx = c->x / TILE, sy = (-c->z) / TILE;
    if (sx == gtx && sy == gty) return go(c, gtx * TILE + TILE / 2, -(gty * TILE + TILE / 2), spd, in);
    int fresh = !c->wv || c->gx != gtx || c->gy != gty;
    if (fresh || (sx == c->wx && sy == c->wy)) {
        int ok = 0;
        if (!fresh && sx == c->wx && sy == c->wy) ok = route_step(c, sx, sy);           // on to the next tile of the route
        if (!ok && route_plan(c, gtx, gty, sx, sy, in)) ok = route_step(c, sx, sy);     // (new goal, or the route ran out: search again)
        if (!ok) { c->wv = 0; return go(c, gtx * TILE + TILE / 2, -(gty * TILE + TILE / 2), spd, in); }   // no route found: the straight line
    }
    go(c, c->wx * TILE + TILE / 2, -(c->wy * TILE + TILE / 2), spd, in);
    return 0;
}
static int scripted(Cit *c, int tx, int tz, int spd) {   // straight to a point, through a doorway, ignoring everything
    int dx = tx - c->x, dz = tz - c->z, d = ABS(dx) + ABS(dz);
    if (d <= spd * 2) { c->x = (s16)tx; c->z = (s16)tz; return 1; }
    c->yaw = (u16)yaw_to(dx, dz);
    c->x += (s16)(dx * spd / d); c->z += (s16)(dz * spd / d); moved_flag = 1;
    return 0;
}


// ---- a day at home: showers, breakfast, the sofa, the TV; and the odd visit next door ----------------------
static int sit_item(char ch) { return ch == 'T' || ch == 'C' || ch == 'h' || ch == 'U' || ch == '%' || ch == ';' || ch == (char)0x90 || ch == (char)0x91 || ch == (char)0x92 || ch == (char)0xA8; }
static int free_floor(const Home *hm, int x, int y) {   // a tile of the home's floor a body can stand on, off the lane inside the door
    if (x < hm->x0 || x > hm->x1 || y < hm->y0 || y > hm->y1) return 0;
    if (x == hm->dx && y >= hm->dy - 1) return 0;
    return world_walkable(x, y);
}
static int find_item(const Home *hm, const char *letters, int *ix, int *iy, int *sx, int *sy) {   // one of those things in the home, and the tile to stand on to use it
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
static int start_chore(Cit *c, const Home *hm, const char *letters, char chain) {
    int ix, iy, sx, sy;
    if (!find_item(hm, letters, &ix, &iy, &sx, &sy)) return 0;
    c->chore = (u8)world_at(ix, iy); c->chain = (u8)chain; c->ix = (u8)ix; c->iy = (u8)iy; c->chx = (u8)sx; c->chy = (u8)sy;
    c->mode = CM_CHORE; c->sub = 0; c->st = 0; c->wv = 0;
    return 1;
}
static int pick_chore(Cit *c, const Home *hm) {   // what a person does with a free hour at home: it depends on the time of day
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
static int start_visit(Cit *c) {   // a neighbour in the same zone, a short walk away: go and knock
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
    c->mode = CM_VISIT; c->sub = 0; c->st = 0; c->wv = 0;
    return 1;
}

static void decide(Cit *c) {   // the clock says where they should be: out, in, or in bed
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
static int party_spots(void) {             // the dance floor: free tiles round the host, nearest first (a breadth-first fill)
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
static void party_entry(void) {            // where guests come in: inside the nearest door for a party indoors, else down the street
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
int citizens_party(void) {
    if (party_on) return -1;
    const Sim *p = &sims[player];
    party_cx = p->x / TILE; party_cy = (-p->z) / TILE; party_in = indoors_at(p->x, p->z);
    int spots = party_spots();
    if (spots < 4) return 0;
    party_entry();
    party_n = 0;                            // the guests: people of this zone first (awake or not: it's a party), then the other zones'
    for (int pass = 0; pass < 2; pass++) for (int i = 0; i < NCIT && party_n < MIN(spots, PARTY_MAX); i++) {
        Cit *c = &cits[i];
        if (c->mode == CM_GONE || c->mode == CM_KO || c->mode == CM_PARTY) continue;
        if ((pass == 0) != (c->zone == world_zone)) continue;
        if (c->stage == AG_CHILD && clock_min >= 1260) continue;            // (no small kids after 9)
        party_g[party_n++] = (u8)i;
    }
    for (int k = party_n - 1; k > 0; k--) { int j = rn(k + 1); u8 t = party_g[k]; party_g[k] = party_g[j]; party_g[j] = t; }
    party_next = 0; party_t = 0; party_on = 1; party_left = PARTY_MIN;
    return party_n;
}
int citizens_party_on(void) { return party_on; }
static void party_arrive(Cit *c, int k) {  // a guest walks in
    if (c->zone != world_zone) { c->zone0 = c->zone; c->zone = (u8)world_zone; if (world_zone == ZONE_HOME) home_guests++; }
    c->x = (s16)(party_spx + (int)rn(61) - 30); c->z = (s16)(party_spz + (int)rn(61) - 30);
    c->mode = CM_PARTY; c->sub = 0; c->st = 0; c->wv = 0; c->gait.sleep = 0; c->gait.sit = 0;
    c->chx = party_sx[k % PARTY_MAX]; c->chy = party_sy[k % PARTY_MAX];
    c->t = (u16)(600 + rn(600));
}
static void party_leave_done(Cit *c) {     // home time
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
static void party_minute(void) {           // once a game minute: the clock runs out
    if (!party_on) return;
    if (--party_left > 0) return;
    party_on = 0;                          // over: everyone heads off
    for (int i = 0; i < NCIT; i++) { Cit *c = &cits[i]; if (c->mode == CM_PARTY) { c->sub = 2; c->wv = 0; c->st = 0; } }
}
static void party_step(Cit *c, int i) {
    int in = indoors_at(c->x, c->z);
    if (c->sub == 0) {                     // on the way to their spot
        if (nav(c, c->chx, c->chy, 5, party_in)) { c->sub = 1; c->yaw = (u16)yaw_to(party_cx * TILE + TILE / 2 - c->x, -(party_cy * TILE + TILE / 2) - c->z); }
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
    if (go(c, party_spx, party_spz, 5, in)) party_leave_done(c);
    else if (++c->st > 300) party_leave_done(c);
}

// ---- the crowd: walking, talking, brawling ----------------------------------------------------------------
static void step_one(Cit *c, int i) {
    Sim *p = &sims[player];
    int dx = p->x - c->x, dz = p->z - c->z;
    int d2 = dx * dx + dz * dz;
    const Home *hm = home_of(c);
    int in = indoors_at(c->x, c->z);
    moved_flag = 0;
    if (c->say_t) c->say_t--;
    if (c->cd) c->cd--;
    c->gait.talk = c->mode == CM_CHAT;
    c->gait.mood = c->rel / 2;
    if (!(((tick >> 3) + i) & 15)) decide(c);   // look at the clock every so often
    switch (c->mode) {
    case CM_IDLE:
        if (c->t) { c->t--; break; }
        if (hm && in && want(c) == 1 && rn(100) < 70 && pick_chore(c, hm)) break;                    // a free hour at home: something to do
        if (hm && !in && c->stage >= AG_TEEN && clock_min >= 540 && clock_min < 1260 && !rn(14) && start_visit(c)) break;   // or a knock on a neighbour's door
        c->wyaw = (u16)rn(4096); c->t = (u16)(80 + rn(220)); c->mode = CM_WALK;
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
        if (!c->t) { c->mode = CM_IDLE; c->t = (u16)(30 + rn(200)); break; }
        c->t--;
        turn(c, c->wyaw, 90);
        int spd = in ? 3 : c->stage == AG_CHILD ? 6 : c->stage == AG_ELDER ? 3 : 5 + (c->pers & 1);
        int mx = (isin(c->yaw) * spd) >> 12, mz = -((icos(c->yaw) * spd) >> 12);
        if (d2 < 120 * 120 && (mx * dx + mz * dz) > 0) { c->mode = CM_IDLE; c->t = 40; break; }   // do not walk into you
        if (body_free(c->x + mx, c->z + mz, in)) { c->x += (s16)mx; c->z += (s16)mz; moved_flag = 1; }
        else { c->wyaw = (u16)((c->yaw + 1024 + rn(2048)) & 4095); if (!rn(3)) { c->mode = CM_IDLE; c->t = 20; } }
        break; }
    case CM_CHAT:
        if (c->t) c->t--;
        if (!c->t || d2 > 3 * TILE * 3 * TILE) { c->mode = CM_IDLE; c->t = 30; break; }
        turn(c, yaw_to(dx, dz), 200);
        break;
    case CM_CHASE: {
        if (d2 > 7 * TILE * 7 * TILE) { c->mode = CM_IDLE; c->t = 60; say(c, "FORGET IT"); break; }
        turn(c, yaw_to(dx, dz), 180);
        if (d2 > 90 * 90) {
            int spd = in ? 4 : 7;
            int mx = (isin(c->yaw) * spd) >> 12, mz = -((icos(c->yaw) * spd) >> 12);
            if (body_free(c->x + mx, c->z + mz, in)) { c->x += (s16)mx; c->z += (s16)mz; moved_flag = 1; }
            else if (body_free(c->x + mx, c->z, in)) { c->x += (s16)mx; moved_flag = 1; }
            else if (body_free(c->x, c->z + mz, in)) { c->z += (s16)mz; moved_flag = 1; }
        } else if (!c->cd) {
            int dmg = (c->stage == AG_ELDER ? 2 : 3) + rn(5);
            pl_hp -= dmg; c->cd = 45; c->gait.wave = 14; p->gait.squash = -10;
            audio_voice(VOICE_GROAN);
            if (pl_hp <= 0) {
                int lost = MIN((int)cash, 60); cash -= lost; pl_hp = 60;
                char b[32]; sprintf(b, "KNOCKED OUT. LOST $%d", lost); sim_note(b);
                for (int k = 0; k < NCIT; k++) if (cits[k].mode == CM_CHASE) { cits[k].mode = CM_IDLE; cits[k].t = 120; }
                fight_on = 0; say(c, "STAY DOWN."); audio_voice(VOICE_CRY);
            }
        }
        break; }
    case CM_KO:
        c->gait.sleep = 1;
        if (c->t) c->t--;
        if (!c->t) { c->mode = CM_IDLE; c->hp = 25; c->gait.sleep = 0; c->t = 60; say(c, "...OW."); }
        break;
    case CM_GOHOME: {   // to the front of their door, in through it, and a step inside
        if (!hm) { c->mode = CM_IDLE; break; }
        int gx, zf, zd, zi; door_geo(hm, &gx, &zf, &zd, &zi);
        int spd = c->stage == AG_ELDER ? 3 : 5;
        if (c->sub == 0) { if (nav(c, hm->dx, hm->dy + 1, spd, 0)) c->sub = 1; }
        else if (c->sub == 1) { if (scripted(c, gx, zd, 3)) c->sub = 2; }
        else if (scripted(c, gx, zi, 3)) { c->mode = CM_IDLE; c->t = (u16)(20 + rn(100)); }
        break; }
    case CM_LEAVE: {    // across the floor to the door, out through it, and off
        if (!hm) { c->mode = CM_IDLE; break; }
        int gx, zf, zd, zi; door_geo(hm, &gx, &zf, &zd, &zi);
        if (c->sub == 0) { if (nav(c, hm->dx, hm->dy - 1, 3, 1)) c->sub = 1; }
        else if (c->sub == 1) { if (scripted(c, gx, zd, 3)) c->sub = 2; }
        else if (scripted(c, gx, zf, 3)) { c->mode = CM_IDLE; c->t = (u16)rn(40); }
        break; }
    case CM_CHORE: {
        if (!hm) { c->mode = CM_IDLE; break; }
        if (c->sub == 0) {
            if (nav(c, c->chx, c->chy, 3, 1)) {
                c->sub = 1; c->t = (u16)(chore_len((char)c->chore) / 2 + rn(chore_len((char)c->chore)));
                c->x = (s16)(c->chx * TILE + TILE / 2); c->z = (s16)(-(c->chy * TILE + TILE / 2));
                if (c->chx != c->ix || c->chy != c->iy) c->yaw = (u16)yaw_to((c->ix - c->chx) * TILE, -(c->iy - c->chy) * TILE);   // face the thing
                c->gait.sit = sit_item((char)c->chore) && c->chx == c->ix && c->chy == c->iy;
                if (d2 < 5 * TILE * 5 * TILE && !c->say_t) say(c, chore_say((char)c->chore));
            }
        } else if (c->t) {
            c->t--;
            if (c->chore == 'F' || c->chore == '/' || c->chore == '-') c->gait.wave = (c->t & 31) < 6 ? 8 : c->gait.wave;   // a bit of busy hands
        } else {
            c->gait.sit = 0;
            if (c->chain) { char nx[2] = { (char)c->chain, 0 }; if (start_chore(c, hm, nx, 0)) break; }
            c->chain = 0; c->mode = CM_IDLE; c->t = (u16)(30 + rn(150));
        }
        break; }
    case CM_VISIT: {   // out to a neighbour's door, a knock, a wait, and off again
        if (c->sub == 0) { if (nav(c, c->ix, c->iy, c->stage == AG_ELDER ? 3 : 4, 0)) {
            c->sub = 1; c->t = (u16)(200 + rn(200)); c->yaw = 0;
            if (d2 < 7 * TILE * 7 * TILE) say(c, "KNOCK KNOCK");
            const Hh *g = &hh[c->vh];
            for (int k = 0; k < g->n; k++) { Cit *o = &cits[g->mem[k]]; if (o->mode == CM_IDLE || (o->mode == CM_CHORE && o->sub == 1)) { if (!o->say_t && indoors_at(o->x, o->z)) say(o, "COMING!"); break; } }
        } }
        else if (c->t) c->t--;
        else { c->mode = CM_IDLE; c->t = (u16)(60 + rn(200)); if (!rn(2)) say(c, "OH WELL, LATER"); }
        break; }
    case CM_PARTY: party_step(c, i); break;
    case CM_SLEEP:
        if (!hm) { c->mode = CM_IDLE; break; }
        if (c->sub == 0) {
            int sx, sz; sleep_spot(c, hm, &sx, &sz);
            if (nav(c, sx / TILE, (-sz) / TILE, 3, 1)) { c->sub = 1; c->x = (s16)sx; c->z = (s16)sz; c->yaw = 1024; }
        } else { c->gait.sleep = 1; c->gait.amp = 0; }
        if (c->sub == 1 && !c->gait.sleep) c->gait.sleep = 1;
        break;
    }
    if (c->mode == CM_IDLE && c->say_t == 0 && d2 < 2 * TILE * 2 * TILE && c->rel > 10 && !rn(900)) say(c, chat_line(c));   // a friend says hi in passing
    // the walk cycle
    if (moved_flag) { c->gait.phase = (c->gait.phase + 130) & 4095; c->gait.amp = MIN(c->gait.amp + 400, 4096); }
    else c->gait.amp = MAX(c->gait.amp - 500, 0);
    if (c->mode != CM_SLEEP && c->mode != CM_KO) c->gait.sleep = 0;
    if (c->mode != CM_CHORE) c->gait.sit = 0;
    if (c->gait.wave > 0) c->gait.wave--;
    if (c->gait.squash < 0) c->gait.squash++;
    c->gait.seed = i; c->gait.clock = tick;
}

void citizens_update(int steps) {
    if (world_zone == ZONE_HOME && !home_guests && !party_on) { cur = -1; fight_on = 0; return; }
    if (world_zone == ZONE_CITY) { pop_tick(); vanish_tick(); }
    int built = 0;
    {   // bodies for those who have none yet: the nearest the camera first
        int best = -1, bd = 1 << 30;
        for (int i = 0; i < NCIT; i++) {
            const Cit *c = &cits[i];
            if (c->zone != world_zone || c->built || c->mode == CM_GONE) continue;
            int dx = c->x - cam.at.vx, dz = c->z - cam.at.vz, d = dx * dx + dz * dz;
            if (d < bd) { bd = d; best = i; }
        }
        if (best >= 0) { atlas_defer = 1; build_lod(&cits[best]); atlas_defer = 0; built++; }   // (its face goes to VRAM at the flip)
    }
    if (!built) {   // someone near the camera whose stored model was let go (the store was full): build them again
        int best = -1, bd = DRAW_DIST * DRAW_DIST;
        for (int i = 0; i < NCIT; i++) {
            const Cit *c = &cits[i];
            if (c->zone != world_zone || !c->built || c->mode == CM_GONE || crowd_has(i)) continue;
            int dx = c->x - cam.at.vx, dz = c->z - cam.at.vz, d = dx * dx + dz * dz;
            if (d < bd) { bd = d; best = i; }
        }
        if (best >= 0) { atlas_defer = 1; build_lod(&cits[best]); atlas_defer = 0; }
    }
    for (int k = 0; k < steps; k++) {
        tick++;
        party_tick();
        { static int last_min = -1; if (clock_min != last_min) { if (last_min >= 0) party_minute(); last_min = clock_min; } }
        if (pl_cd > 0) pl_cd--;
        const Sim *me = &sims[player];
        for (int i = 0; i < NCIT; i++) {
            Cit *c = &cits[i];
            if (c->zone != world_zone || c->mode == CM_GONE) continue;
            if ((tick + i) & 1) {   // far off (past the full detail, out of the way of anything you do): every other step is enough
                int dx = (me->x - c->x) >> 4, dz = (me->z - c->z) >> 4;
                if (dx * dx + dz * dz > (14 * TILE / 16) * (14 * TILE / 16) && c->mode != CM_CHASE && c->mode != CM_PARTY && c->mode != CM_CHAT) continue;
            }
            step_one(c, i);
        }
        if (!fight_on && pl_hp < 100 && !(tick & 63)) pl_hp++;   // you heal about a point a second
    }
    cur = find_target();
    fight_on = 0; for (int i = 0; i < NCIT; i++) if (cits[i].zone == world_zone && cits[i].mode == CM_CHASE) fight_on = 1;
}

// ---- drawing ----------------------------------------------------------------------------------------------
int cit_n_full, cit_n_lod, cit_n_spr, cit_n_vis; static int cit_end_hb = -1;
void citizens_draw(void) {
    cit_n_full = cit_n_lod = cit_n_spr = 0;
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
    static u8 tier[MAXDRAW]; static VECTOR pp[MAXDRAW];
    static s16 need[MAXDRAW];
    for (int k = 0; k < n; k++) {
        Cit *c = &cits[order[k]];
        int vy = 0;
        if (c->mode == CM_KO) vy = -24;
        else if (c->mode == CM_SLEEP && c->sub == 1) vy = c->mem == 0 ? -items_height('S') + 40 : -24;   // in the bed, or on the floor
        else if (c->mode == CM_CHORE && c->sub == 1 && c->gait.sit) vy = -items_height((char)c->chore) / 2 + 10;   // on the sofa, on the loo
        setVector(&pp[k], c->x, vy, c->z);
        int h = crowd_need_h(order[k], &pp[k], &c->gait);
        if (!h) { int sx, sy, z = gfx_proj(c->x, vy - 200, c->z, &sx, &sy); h = z ? (300 * 230) / z : 999; }   // (not measured yet: a guess)
        need[k] = (s16)h; tier[k] = 1;
    }
    // live: the biggest on the screen (too big for a sprite cell at 1:1, or near enough that their sprite would go stale fast),
    // the very biggest first, while the frame has time
    static u8 was_live[NCIT];                                         // (a little give, so no one flips between the two)
    for (int live = 0; live < MIN(cit_live_max, LIVE_MAX); live++) {
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
    // the sprite renders: the most out of date first, as many as fit in the time the frame has left over (it ends on a
    // vblank either way: what is left before that is free), anyone with no sprite at all (two a frame) and now and then one
    // even when there is no time, so nobody stays out of date for long
    static int post = 100, skipped;                                   // the frame's time after the people (from the last frame)
    if (cit_end_hb >= 0 && gfx_last_cpu > cit_end_hb) post = gfx_last_cpu - cit_end_hb;
    int now = gfx_hb(), end = now + post, target = end <= 515 ? 522 : end <= 777 ? 785 : 1045;
    int budget = (target - 30 - end) / MAX(20, crowd_render_hb), fresh = 0;   // (a margin: a frame that runs over waits a whole extra vblank)
    if (cit_sprite_rate == 0) budget = budget > 1 ? budget / 2 : budget; else if (cit_sprite_rate == 2) budget++;
    if (budget <= 0 && ++skipped >= 6) budget = 1;
    if (budget > 0) skipped = 0;
    for (int r = 0; r < 12; r++) {
        int best = -1, bs = 4;
        for (int k = 0; k < n; k++) if (tier[k] == 1 && score[k] > bs) { bs = score[k]; best = k; }
        if (best < 0) break;
        if (bs >= 100000) { if (++fresh > 2) break; }
        else if (budget-- <= 0) break;
        Cit *c = &cits[order[best]];
        PROF_BEGIN(); crowd_sprite(order[best], &pp[best], c->yaw, &c->gait, 1); PROF_END(6);
        tier[best] = 2;
    }
    for (int k = 0; k < n; k++) {
        Cit *c = &cits[order[k]];
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
        ui_bubble(sx, sy, c->say, UI_INK);
    }
}

static const char *stage_nm(int s) { return s == AG_CHILD ? "KID" : s == AG_TEEN ? "TEEN" : s == AG_ELDER ? "ELDER" : "ADULT"; }
static void bar(int x, int y, int w, int v, int r, int g, int b) {
    gfx_rect(x, y, w, 4, 30, 30, 36, 0);
    gfx_rect(x + 1, y + 1, (w - 2) * CLAMP(v, 0, 100) / 100, 2, r, g, b, 0);
}
int citizens_card_on(void) { return !(world_zone == ZONE_HOME && !home_guests) && cur >= 0; }   // (main: the notes go under it)
void citizens_hud(void) {
    if (world_zone == ZONE_HOME && !home_guests) return;
    const int X = 4, Y = 37;                                       // (on the left, under the household: the notes go under it)
    if (cur >= 0) {
        const Cit *c = &cits[cur];
        char t[40]; sprintf(t, "%s %s", c->first, c->last);   // a white card (The Sims 4's): who they are, how they are to you
        ui_card(X, Y, 136, 45);                                    // (four lines: the name, who they are, how they are to you, what they do)
        ui_icon_disc(X + 12, Y + 11, 16, IC_CHAT, UI_BLUE, UI_WHITE);
        ui_text(X + 24, Y + 3, 1, t, UI_INK);
        if (c->pid != NOPID) { sprintf(t, "%s, %s", stage_nm(c->stage), pop_trait_name(pop_per[c->pid].trait)); if (ui_tw(1, t) > 108) strcpy(t, pop_trait_name(pop_per[c->pid].trait)); }
        else sprintf(t, "%s", stage_nm(c->stage));
        ui_text(X + 24, Y + 12, 1, t, UI_SOFT);
        ui_text(X + 6, Y + 22, 1, rel_name(c->rel), UI_RGB(40, 150, 50));
        ui_bar(X + 70, Y + 22, 60, 4, c->rel > 0 ? c->rel : 0, UI_GREEN);
        ui_bar(X + 70, Y + 27, 60, 4, c->rom, UI_RGB(236, 100, 160));
        { char jb[40]; const char *d = (c->pid != NOPID && c->mplace != NOPID && c->mode == CM_IDLE) ? pop_job_name(c->pid, jb) : activity(c);
          if (d != jb) { strncpy(jb, d, 39); jb[39] = 0; }
          if (ui_tw(1, jb) > 126) { char *k = strchr(jb, ','); if (k) *k = 0; }
          ui_text(X + 6, Y + 34, 1, jb, UI_BLUE2); }
    }
    if (fight_on || pl_hp < 100) { int hy = cur >= 0 ? Y + 47 : Y; ui_card(X, hy, 70, 11); ui_bar(X + 4, hy + 2, 62, 7, pl_hp, pl_hp > 50 ? UI_GREEN : pl_hp > 25 ? UI_RGB(236, 176, 40) : UI_RGB(228, 84, 70)); }
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
