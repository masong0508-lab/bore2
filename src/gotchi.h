// gotchi: the BOREAGOTCHI! expansion pack's data and rules (the screens and the hands-on actions are the overlay GOTCHI.OVL, gotchi_ui.c and
// gotchi_act.c, so the pack stays a separate file on the disc). A toy costs $5000 at an electronics shop and belongs to one Bore of the
// family; each toy holds up to EIGHT creatures, 20 kinds (4 rare), growing into one of six forms by how they were looked after.
// FAMILIES: every creature has a KIN record (personality, colour, taste, mutations, accessory, mate) and a LINE record (who its mother
// and father were), so pets bond, lay eggs, raise their young, grow old and pass on, leaving a memorial and a family tree.
#pragma once
#include "common.h"

#define GT_MAX 8
#define GB_N 8
#define GSP_N 20
#define GFORMS 6
#define GL_N 36
#define GD_N 16
#define GOTCHI_PRICE 5000
#define GOTCHI_EGG 150
enum { GS_EGG, GS_BABY, GS_CHILD, GS_TEEN, GS_ADULT, GS_ELDER };
enum { GF_SICK = 1, GF_SLEEP = 2, GF_DARK = 4, GF_CALL = 8, GF_FAKE = 16, GF_TANTRUM = 32 };
enum { GM_BUDDY, GM_HATCH, GM_PROUD, GM_WORRY, GM_MISS, GM_HOOKED, GM_SHARED };   // the moods it gives its owner
enum { GP_SHY, GP_BOLD, GP_GREEDY, GP_LAZY, GP_PLAYFUL, GP_GENTLE, GP_CHEEKY, GP_CLEVER };
enum { GMU_SPARKLE = 1, GMU_SPOTS = 2, GMU_HALO = 4 };   // rare mutations, passed down
enum { GD_HATCH = 1, GD_GROW, GD_ADULT, GD_PASS, GD_GIFT, GD_BDAY, GD_TANTRUM, GD_ILL, GD_EGG, GD_PLAY, GD_MUT, GD_CARED, GD_BOND, GD_VISIT, GD_AWARD, GD_DAY };   // a diary kind is the verb (bits 0-4) and, for growing up, the new stage or form (bits 5-7)
typedef struct { u8 on, stage, form, sp, nm, hun, hap, disc, wt, poop, fl, miss, needT, bond, snk, meal, gen; u16 age; } Blob;
typedef struct { u8 pers, col, mut, taste, acc, mate, id, bday; } Kin;          // taste: liked food (high nibble), disliked (low); mate: slot + 1
typedef struct { u8 id, sp, nm, fc, pg, mom, dad, mut; } Line;                  // fc: form | col << 3 | 64 passed on | 128 alive; pg: pers | gen << 3
typedef struct { u16 day; u8 kind, id; } Diary;
typedef struct { u8 uid, own, sel, pad; u16 xp, hatched, wins, cared; u8 room, deco, accs, p2; u16 plays; Kin k[GB_N]; Blob b[GB_N]; } Toy;
extern Toy gtoy[GT_MAX];
extern Line gline[GL_N];
extern Diary gdiary[GD_N];
extern u32 gach;                           // achievements won (one bit each)
#define GACH_TUT   0x70000u                 // gach bits 16..18: how far the first-day tutorial went (0 start .. 5); bit 19: tutorial done or skipped; bit 20: the unboxing was seen
#define GACH_TUTOFF 0x80000u
#define GACH_INTRO 0x100000u
extern u8 gsum_pend, gsum_c, gsum_p, gsum_h;   // the end-of-day summary waits to be shown (1): cares, games and hatches that day
extern int sim_want_gotchi;
extern int gotchi_time;                    // 0 the game clock, 1 LIVE in hand (the toy runs on real time while you hold it), 2 off (they wait)

void  gotchi_reset(void);
void  gotchi_minute(void);                 // from the game clock: every creature lives on
void  gotchi_save(void); void gotchi_load(int ver);
Toy  *gotchi_toy(int simi);                // the toy that sim owns (0: none)
int   gotchi_sim_of(const Toy *t);         // its owner's sim index, -1 when gone
int   gotchi_buy(int simi);                // a toy and its first egg for that sim (0 when it has one already)
int   gotchi_any(void);                    // does anyone in the family own one?
int   gotchi_count(const Toy *t);          // creatures in it (eggs too)
int   gotchi_calling(const Toy *t);        // how many of them need you
int   gotchi_level(const Toy *t);          // care skill 0..5
int   gotchi_next_xp(const Toy *t);        // xp for the next level (0: the top)
int   gotchi_needs(const Blob *b);
int   gotchi_egg(Toy *t, int slot);        // a new egg in an empty slot (paid by the caller)
void  gotchi_xp(Toy *t, int n);
void  gotchi_mood(const Toy *t, int kind);
void  gotchi_diary(int kind, int id);
// families
Line *gotchi_line(int id);                 // a creature's line record (0 when forgotten)
int   gotchi_slot_of(const Toy *t, int id);   // which basket slot holds that line id (-1: none)
// the Blobdex
void  gotchi_dex_see(int sp, int form);
int   gotchi_dex_has(int sp, int form);
int   gotchi_dex_count(void);
void  gotchi_cdex_see(int sp, int col);
int   gotchi_cdex_has(int sp, int col);
void  gotchi_live(int steps);                // called while the handheld is open: LIVE time and the hijinks
const char *gotchi_house(int simi, int what);   // objects around the house: 0 feed them all, 1 lights, 2 tuck in
int   gotchi_diary_pos(void);            // how many diary lines were ever written (the newest is the one before it, round the ring)
extern u16 gt_h, gt_done;               // game hours lived (since the game started or loaded) / how many the families have been caught up on (the overlay does that)
