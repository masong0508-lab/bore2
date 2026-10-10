// citizens: see citizens.h
#include "citizens.h"
#include "sim.h"
#include "world.h"
#include "voxel.h"
#include "look.h"
#include "gfx.h"
#include "audio.h"

#define NPOOL     4                 // civilians drawn with the full face-and-hair model at once
#define MAXDRAW   24                // civilians drawn at all, nearest first
#define DRAW_DIST (14 * TILE)
#define FULL_DIST (5 * TILE)
#define REACH     (TILE * 3 / 2)    // how close counts as "in front of you"
#define PUNCH_R   150
#define CR        40                // body radius for walking
#define RING_COST 150
#define GIFT_COST 20
#define BUILD_PER_FRAME 2

enum { CM_IDLE, CM_WALK, CM_CHAT, CM_CHASE, CM_KO, CM_GONE };
enum { PE_FRIENDLY, PE_GRUFF, PE_FLIRTY, PE_SHY };
enum { PO_TALK, PO_JOKE, PO_HUG, PO_FLIRT, PO_GIFT, PO_PROPOSE, PO_FIGHT, PO_SORRY };
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
} Cit;

static Cit cits[NCIT];
static Bore pool[NPOOL]; static int pool_owner[NPOOL]; static int pool_ready;
static Bore scratch; static int scratch_ready;
static int tick, pl_hp = 100, pl_cd, cur = -1, fight_on;
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
static int open_at(int x, int z) {      // can a body stand here? (streets, grass, sand: not water, not indoors, not in things)
    for (int k = 0; k < 4; k++) {
        int px = x + ((k & 1) ? CR : -CR), pz = z + ((k & 2) ? CR : -CR);
        if (px < 0 || pz > 0) return 0;
        int mx = px / TILE, my = (-pz) / TILE;
        if (mx >= MW || my >= MH) return 0;
        int f = floorMap[my][mx];
        if (f < FL_GRASS || f == FL_WATER) return 0;
        if (!world_walkable(mx, my)) return 0;
    }
    return 1;
}
static void say(Cit *c, const char *t) { c->say = t; c->say_t = 200; }
static int adult(const Cit *c) { return c->stage >= AG_ADULT; }
static int pl_adult(void) { return sims[player].bore.stage >= AG_ADULT; }

static const char *const L_FRIEND[] = { "NICE DAY FOR IT", "HAVE YOU SEEN MY DOG", "THE NOODLES ARE GOOD", "I LOVE THIS PLACE", "YOU LOOK LIKE A BORE", "HEARD A RUMOUR", "I NEED A NAP", "BUSES ARE LATE AGAIN", "MY FEET HURT", "WANT A MINT?", "THE DRUMS WERE LOUD", "LIFE IS GOOD, MAN" };
static const char *const L_GRUFF[]   = { "WHAT.", "GO AWAY", "I'M BUSY BEING MAD", "NOT NOW", "UGH. YOU.", "DON'T TALK TO ME", "WHAT DO YOU WANT" };
static const char *const L_KID[]     = { "I LIKE FROGS", "TAG! YOU'RE IT", "ARE WE THERE YET", "MY MUM SAYS HI", "I CAN BURP A SONG", "LOOK AT MY STICK" };
static const char *const L_ELD[]     = { "BACK IN MY DAY", "THESE KNEES...", "KIDS TODAY, HONESTLY", "SPEAK UP, DEAR", "I REMEMBER THE SEA", "HAVE A BISCUIT" };
#define NL(a) ((int)(sizeof(a) / sizeof((a)[0])))
static const char *chat_line(const Cit *c) {
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
}
static void build_lod(Cit *c) {
    if (!scratch_ready) { look_default(&scratch); scratch_ready = 1; }   // one scratch person owns the two face textures; every build reuses them
    cit_bore(&scratch, c);
    strncpy(c->first, scratch.first, NAME_LEN - 1); c->first[NAME_LEN - 1] = 0;
    strncpy(c->last, scratch.last, NAME_LEN - 1); c->last[NAME_LEN - 1] = 0;
    voxel_lod_build(&scratch, &c->lod);
    c->built = 1;
}

// ---- the hundred ------------------------------------------------------------------------------------
void citizens_init(void) {
    crng ^= (u32)look_rand() * 2246822519u;
    if (!crng) crng = 0x9E3779B9u;
    memset(cits, 0, sizeof cits);
    for (int i = 0; i < NCIT; i++) {
        Cit *c = &cits[i];
        c->seed = crnd();
        c->zone = i < 10 ? ZONE_ISLAND : i < 50 ? ZONE_TOWN : ZONE_CITY;     // 10 on the island, 40 in town, 50 in the city
        int r = rn(100);
        c->stage = r < 8 ? AG_CHILD : r < 20 ? AG_TEEN : r < 78 ? AG_ADULT : AG_ELDER;
        c->pers = (u8)rn(4);
        c->hp = 100; c->mode = CM_IDLE; c->slot = 0; c->built = 0;
        strcpy(c->first, "..."); c->last[0] = 0;
    }
    for (int k = 0; k < NPOOL; k++) pool_owner[k] = -1;
    pl_hp = 100; pl_cd = 0; cur = -1; fight_on = 0; pie_c = -1;
}

void citizens_enter(int zone) {
    int px = sims[player].x, pz = sims[player].z;
    for (int k = 0; k < NPOOL; k++) if (pool_owner[k] >= 0) { cits[pool_owner[k]].slot = 0; pool_owner[k] = -1; }
    for (int i = 0; i < NCIT; i++) {
        Cit *c = &cits[i];
        c->slot = 0; c->gait.sleep = 0;
        if (c->zone != zone || c->mode == CM_GONE) continue;
        if (c->mode == CM_CHASE || c->mode == CM_KO) { c->mode = CM_IDLE; c->hp = MAX(c->hp, 30); }
        for (int tries = 0; tries < 300; tries++) {
            int mx = 2 + rn(MW - 4), my = 2 + rn(MH - 4);
            int x = mx * TILE + TILE / 2, z = -(my * TILE + TILE / 2);
            if (!open_at(x, z)) continue;
            if (ABS(x - px) + ABS(z - pz) < 5 * TILE) continue;
            c->x = (s16)x; c->z = (s16)z; c->yaw = (u16)rn(4096);
            break;
        }
        c->mode = CM_IDLE; c->t = (u16)(10 + rn(200)); c->say_t = 0;
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
    return cur >= 0 ? "TALK" : "";
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
    OPT("TALK", PO_TALK) OPT("JOKE", PO_JOKE)
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
    static Bore wed;
    memset(&wed, 0, sizeof wed);
    cit_bore(&wed, c);        // fresh face textures (the pool's are shared), the same person from the same seed
    const Sim *p = &sims[player];
    int tx = p->x / TILE, ty = (-p->z) / TILE;
    sim_add(&wed, tx, ty, 1);
    Sim *w = &sims[nsims - 1];
    w->rel[player] = 100; sims[player].rel[nsims - 1] = 100;
    w->x = p->x + 70; w->z = p->z;
    c->mode = CM_GONE; c->say_t = 0;
    char b[32]; sprintf(b, "MARRIED %s!", w->bore.first); sim_note(b);
    audio_voice(VOICE_WIN);
}

void citizens_pie_do(int i) {
    if (pie_c < 0 || i < 0) return;
    Cit *c = &cits[pie_c];
    if (c->mode == CM_GONE || c->mode == CM_KO) return;
    Sim *p = &sims[player];
    switch (pie_code[i]) {
    case PO_TALK:
        story_talks++;
        if (c->pers == PE_GRUFF && c->rel < 10 && rn(100) < 45) { say(c, "WHAT."); rel_add(c, -2); audio_voice(VOICE_DISAGREE); break; }
        rel_add(c, c->pers == PE_FRIENDLY ? 8 : 5); say(c, chat_line(c)); chat_begin(c);
        sim_moodlet(p, CML_GOOD_CHAT); audio_voice(VOICE_AGREE); break;
    case PO_JOKE:
        story_talks++;
        if (accept(c, 45)) { rel_add(c, 9); say(c, "HAHAHA!"); chat_begin(c); sim_moodlet(p, CML_LAUGH); audio_voice(VOICE_LAUGH); }
        else { rel_add(c, -4); say(c, "NOT FUNNY."); sim_moodlet(p, CML_REJECTED); audio_voice(VOICE_DISAGREE); }
        break;
    case PO_HUG:
        if (accept(c, 40)) { rel_add(c, 8); rom_add(c, 3); say(c, "AWW"); chat_begin(c); sim_moodlet(p, CML_HUGGED); audio_voice(VOICE_AGREE); }
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
        rel_add(c, -10); rom_add(c, -20); say(c, "BRING IT!"); fight_on = 1; audio_voice(VOICE_ANGRY); break;
    case PO_SORRY:
        if (rn(100) < 55) { c->mode = CM_IDLE; c->t = 60; say(c, "...FINE."); }
        else { say(c, "TOO LATE!"); audio_voice(VOICE_DISAGREE); }
        break;
    }
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
    { int d = MAX(ABS(dx) + ABS(dz), 1), kx = dx * 40 / d, kz = dz * 40 / d;   // a shove back
      if (open_at(c->x + kx, c->z + kz)) { c->x += (s16)kx; c->z += (s16)kz; } }
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

// ---- the crowd: walking, talking, brawling ----------------------------------------------------------------
static void step_one(Cit *c, int i) {
    Sim *p = &sims[player];
    int dx = p->x - c->x, dz = p->z - c->z;
    int d2 = dx * dx + dz * dz;
    int moving = 0;
    if (c->say_t) c->say_t--;
    if (c->cd) c->cd--;
    c->gait.talk = c->mode == CM_CHAT;
    c->gait.mood = c->rel / 2;
    switch (c->mode) {
    case CM_IDLE:
        if (c->t) { c->t--; break; }
        c->wyaw = (u16)rn(4096); c->t = (u16)(80 + rn(220)); c->mode = CM_WALK;
        break;
    case CM_WALK: {
        if (!c->t) { c->mode = CM_IDLE; c->t = (u16)(30 + rn(200)); break; }
        c->t--;
        turn(c, c->wyaw, 90);
        int spd = c->stage == AG_CHILD ? 5 : c->stage == AG_ELDER ? 2 : 3 + (c->pers & 1);
        int mx = (isin(c->yaw) * spd) >> 12, mz = -((icos(c->yaw) * spd) >> 12);
        if (d2 < 120 * 120 && (mx * dx + mz * dz) > 0) { c->mode = CM_IDLE; c->t = 40; break; }   // do not walk into you
        if (open_at(c->x + mx, c->z + mz)) { c->x += (s16)mx; c->z += (s16)mz; moving = 1; }
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
            int mx = (isin(c->yaw) * 7) >> 12, mz = -((icos(c->yaw) * 7) >> 12);
            if (open_at(c->x + mx, c->z + mz)) { c->x += (s16)mx; c->z += (s16)mz; moving = 1; }
            else if (open_at(c->x + mx, c->z)) { c->x += (s16)mx; moving = 1; }
            else if (open_at(c->x, c->z + mz)) { c->z += (s16)mz; moving = 1; }
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
    }
    if (c->mode == CM_IDLE && c->say_t == 0 && d2 < 2 * TILE * 2 * TILE && c->rel > 10 && !rn(900)) say(c, chat_line(c));   // a friend says hi in passing
    // the walk cycle
    if (moving) { c->gait.phase = (c->gait.phase + 130) & 4095; c->gait.amp = MIN(c->gait.amp + 400, 4096); }
    else c->gait.amp = MAX(c->gait.amp - 500, 0);
    if (c->gait.wave > 0) c->gait.wave--;
    if (c->gait.squash < 0) c->gait.squash++;
    c->gait.seed = i; c->gait.clock = tick;
}

void citizens_update(int steps) {
    if (world_zone == ZONE_HOME) { cur = -1; fight_on = 0; return; }
    int built = 0;
    for (int i = 0; i < NCIT && built < BUILD_PER_FRAME; i++) {
        Cit *c = &cits[i];
        if (c->zone == world_zone && !c->built && c->mode != CM_GONE) { build_lod(c); built++; }
    }
    for (int k = 0; k < steps; k++) {
        tick++;
        if (pl_cd > 0) pl_cd--;
        for (int i = 0; i < NCIT; i++) {
            Cit *c = &cits[i];
            if (c->zone != world_zone || c->mode == CM_GONE) continue;
            step_one(c, i);
        }
        if (!fight_on && pl_hp < 100 && !(tick & 63)) pl_hp++;   // you heal about a point a second
    }
    cur = find_target();
    fight_on = 0; for (int i = 0; i < NCIT; i++) if (cits[i].zone == world_zone && cits[i].mode == CM_CHASE) fight_on = 1;
}

// ---- drawing ----------------------------------------------------------------------------------------------
static void pool_init(void) {
    if (pool_ready) return;
    for (int k = 0; k < NPOOL; k++) { look_default(&pool[k]); pool_owner[k] = -1; }   // each owns its two face textures for good
    pool_ready = 1;
}
void citizens_draw(void) {
    if (world_zone == ZONE_HOME) return;
    int order[MAXDRAW], od[MAXDRAW], n = 0;
    for (int i = 0; i < NCIT; i++) {   // the nearest MAXDRAW to where the camera looks, nearest first
        const Cit *c = &cits[i];
        if (c->zone != world_zone || !c->built || c->mode == CM_GONE) continue;
        int dx = c->x - cam.at.vx, dz = c->z - cam.at.vz, d = dx * dx + dz * dz;
        if (d > DRAW_DIST * DRAW_DIST) continue;
        int pos = n < MAXDRAW ? n++ : MAXDRAW;
        if (pos == MAXDRAW) { if (d >= od[MAXDRAW - 1]) continue; pos = MAXDRAW - 1; }
        while (pos > 0 && od[pos - 1] > d) { order[pos] = order[pos - 1]; od[pos] = od[pos - 1]; pos--; }
        order[pos] = i; od[pos] = d;
    }
    // who gets the full model: the nearest few within FULL_DIST; one rebuild a frame
    int want[NPOOL], nw = 0;
    for (int k = 0; k < n && nw < NPOOL; k++) if (od[k] <= FULL_DIST * FULL_DIST) want[nw++] = order[k];
    for (int k = 0; k < NPOOL; k++) {   // let go of the ones that are no longer wanted
        if (pool_owner[k] < 0) continue;
        int keep = 0; for (int j = 0; j < nw; j++) if (want[j] == pool_owner[k]) keep = 1;
        if (!keep) { cits[pool_owner[k]].slot = 0; pool_owner[k] = -1; }
    }
    for (int j = 0; j < nw; j++) {
        Cit *c = &cits[want[j]];
        if (c->slot) continue;
        pool_init();
        int k = 0; while (k < NPOOL && pool_owner[k] >= 0) k++;
        if (k == NPOOL) break;
        cit_bore(&pool[k], c);
        pool_owner[k] = want[j]; c->slot = (u8)(k + 1);
        break;
    }
    for (int k = 0; k < n; k++) {
        Cit *c = &cits[order[k]];
        VECTOR p; setVector(&p, c->x, c->mode == CM_KO ? -24 : 0, c->z);
        if (c->slot) voxel_draw(&pool[c->slot - 1], &p, c->yaw, &c->gait);
        else voxel_draw_lod(&c->lod, &p, c->yaw, &c->gait);
        if (c->mode != CM_KO) { VECTOR sp; setVector(&sp, c->x, 0, c->z); gfx_shadow(&sp, 70); }
    }
}

void citizens_draw_bubbles(void) {
    if (world_zone == ZONE_HOME) return;
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
        int w = gfx_text_w(1, c->say) + 8;
        gfx_rect(sx - w / 2, sy - 12, w, 12, 245, 245, 250, 0);
        gfx_text(sx - w / 2 + 4, sy - 10, 1, c->say, 40, 40, 40);
        gfx_rect(sx - 2, sy, 4, 3, 245, 245, 250, 0);
    }
}

static const char *stage_nm(int s) { return s == AG_CHILD ? "KID" : s == AG_TEEN ? "TEEN" : s == AG_ELDER ? "ELDER" : "ADULT"; }
static void bar(int x, int y, int w, int v, int r, int g, int b) {
    gfx_rect(x, y, w, 4, 30, 30, 36, 0);
    gfx_rect(x + 1, y + 1, (w - 2) * CLAMP(v, 0, 100) / 100, 2, r, g, b, 0);
}
void citizens_hud(void) {
    if (world_zone == ZONE_HOME) return;
    if (cur >= 0) {
        const Cit *c = &cits[cur];
        char t[32]; sprintf(t, "%s %s", c->first, c->last);
        int w = MAX(gfx_text_w(1, t) + 12, 120);
        gfx_rect(78, 4, w, 26, 13, 15, 21, 0); gfx_rect(78, 4, 2, 26, 255, 200, 40, 0);
        gfx_text(84, 7, 1, t, 235, 235, 245);
        sprintf(t, "%s %s", stage_nm(c->stage), rel_name(c->rel));
        gfx_text(84, 17, 1, t, 150, 150, 160);
        bar(84 + 70, 17, 40, c->rel > 0 ? c->rel : 0, 90, 220, 90);
        bar(84 + 70, 22, 40, c->rom, 255, 120, 170);
    }
    if (fight_on || pl_hp < 100) {
        gfx_rect(78, 32, 62, 8, 13, 15, 21, 0);
        gfx_rect(79, 33, 60 * pl_hp / 100, 6, pl_hp > 50 ? 90 : 255, pl_hp > 50 ? 220 : pl_hp > 25 ? 200 : 80, 70, 0);
    }
    if (fight_on) gfx_textc(160, 150, 1, "X PUNCHES", 255, 150, 120);
}
