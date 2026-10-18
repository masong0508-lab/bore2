// story.c - STORY MISSIONS (story.h): the GBA game's ten stories (source/story.h there), retold for BORE 2's world.
// The GBA's places become SAN BOREAS's (its lounge a bar, its plaza a town square, its old town STRANGE TOWN), its prison
// a fine (RAGS TO RICHES gets set up and busted all the same), its skate letters big air off the ramps, its money a quarter.
#include "story.h"
#include "sim.h"
#include "wants.h"
#include "world.h"
#include "bigmap.h"
#include "home.h"
#include "places.h"
#include "citizens.h"
#include "audio.h"
#include "save.h"
#include "ui.h"
#include "gfx.h"
#include "look.h"

// ---- the stories ----
// a task: K_AT a kind of place in SAN BOREAS (a: LOT_), K_TOWN STRANGE TOWN, K_HOME back home, K_EV something done n times (a: the event),
// K_COMBO tricks landed one after another (n long), K_FRIEND / K_LOVE / K_STEADY / K_BFF with the story's person, K_FRIENDS n friends,
// K_MONEY n saved, K_HOUSE n Bores at home, K_GUEST someone over, K_KIDFRIEND your kid's friend, K_ROOM a room that looks great,
// K_FINE the story's bust (it takes your cash), K_HOMEOWN a home of your own, K_SKILL a skill at level n
enum { K_NONE, K_AT, K_TOWN, K_HOME, K_EV, K_COMBO, K_FRIEND, K_LOVE, K_STEADY, K_BFF, K_FRIENDS, K_MONEY, K_HOUSE, K_GUEST, K_KIDFRIEND,
       K_ROOM, K_FINE, K_HOMEOWN, K_SKILL };
typedef struct { u8 k, a; u16 n; const char *nm; } STask;
typedef struct { const char *title, *br[2]; STask t[3]; u8 kid; } SChap;   // (kid: no tasks, a baby comes home the next day)
typedef struct { const char *first, *last; u8 look[12]; u8 stage, asp, sign; s8 rel; } SPerson;
typedef struct { const char *name, *tag, *blurb[3], *end; u8 col[3]; SPerson lead, cast[2]; u8 ncast; SChap ch[5]; } SStory;

#define AT(l, nm)    { K_AT, l, 0, nm }
#define EV(e, n, nm) { K_EV, e, n, nm }
#define KD(k, n, nm) { k, 0, n, nm }
static const SStory stories[STY_N] = {
  { 0 },
  { "ROOMMATES", "ROMANCE", { "JAD MOVES INTO A FLATSHARE", "WITH BRANNET, A STRANGER.", "THE TOWN DOES THE REST." }, "A FAMILY OF YOUR OWN", { 236, 96, 140 },
    { "JAD", "PARKER", { 5,3,2,1,1,0,2,3,2, 0,0,0 }, AG_ADULT, AS_PLEAS, 6, 0 },
    { { "BRANNET", "DUNMORE", { 4,2,3,1,2,2,1,5,0, 0,0,0 }, AG_ADULT, AS_KNOW, 8, 10 } }, 1,
    { { "THE NEW ROOMMATE", { "BRANNET KEEPS TO HERSELF.", "FOOD AND A CHAT BREAK THE ICE." }, { EV(E_PIZZA, 1, "ORDER A PIZZA"), KD(K_FRIEND, 0, "BECOME FRIENDS WITH BRANNET") } },
      { "SKATE DATE", { "SHE SAYS SHE SKATES BETTER.", "PROVE IT AT THE SKATE PLAZA." }, { AT(LOT_SKATE, "GO TO A SKATE PLAZA"), KD(K_COMBO, 3, "LAND A 3 TRICK COMBO") } },
      { "SOMETHING MORE", { "BACK HOME IT FEELS DIFFERENT.", "MAKE HER LAUGH. WIN HER HEART." }, { EV(E_JOKE, 1, "TELL A JOKE"), KD(K_LOVE, 0, "FALL IN LOVE WITH BRANNET") } },
      { "THE BAR", { "A FIRST REAL DATE, A DRINK", "IN TOWN. THEN MAKE IT OFFICIAL." }, { AT(LOT_BAR, "GO TO A BAR"), KD(K_STEADY, 0, "GO STEADY WITH BRANNET") } },
      { "A CHILD COMES HOME", { "JAD AND BRANNET ARE READY.", "A BABY COMES HOME TOMORROW." }, { { 0 } }, 1 } } },
  { "NEWLYWEDS", "ROMANCE AND FAMILY", { "NICK AND ELI, JUST MARRIED.", "BILLS, DATE NIGHTS AND", "A FAMILY TO START." }, "HAPPY EVER AFTER", { 226, 168, 40 },
    { "NICK", "KOWALSKI", { 5,2,1,1,1,0,3,3,2, 0,0,0 }, AG_ADULT, AS_HOME, 1, 0 },
    { { "ELI", 0, { 4,4,2,1,2,2,5,6,0, 0,0,0 }, AG_ADULT, AS_FORTUNE, 6, 75 } }, 1,
    { { "BILLS BILLS BILLS", { "THE HONEYMOON IS OVER. EARN", "A PAYCHECK, MAKE THE PLACE NICE." }, { EV(W_PAID, 1, "EARN A PAYCHECK"), KD(K_ROOM, 0, "MAKE A ROOM LOOK GREAT") } },
      { "DATE NIGHT", { "ELI MISSES GOING OUT. A TAXI", "TO A BAR, AND A DRINK FOR TWO." }, { AT(LOT_BAR, "GO TO A BAR"), EV(W_NIGHT, 1, "HAVE A DRINK") } },
      { "A CHILD COMES HOME", { "THE NURSERY IS READY.", "SOMEONE SMALL IS ON THE WAY." }, { { 0 } }, 1 },
      { "FAMILY DAY", { "A DAY AT THE PARK, THEN BE", "THERE FOR YOUR LITTLE ONE." }, { AT(LOT_PARK, "GO TO A PARK"), KD(K_KIDFRIEND, 0, "BECOME YOUR KID'S FRIEND") } },
      { "MOVING UP", { "A BIGGER FAMILY NEEDS A", "BIGGER PAYCHECK." }, { EV(W_PROMO, 1, "GET A PROMOTION"), KD(K_MONEY, 25000, "SAVE $25000") } } } },
  { "SINGLE PARENT", "FAMILY", { "MARA AND HER GIRL JUNIE,", "ON THEIR OWN. MAKE THE", "MONEY WORK, LET PEOPLE IN." }, "YOU MADE IT WORK", { 240, 136, 56 },
    { "MARA", "BRENNAN", { 4,1,2,2,1,1,4,5,1, 0,0,0 }, AG_ADULT, AS_HOME, 2, 0 },
    { { "JUNIE", 0, { 0,1,1,2,2,1,4,5,5, 0,0,0 }, AG_CHILD, AS_GROW, 2, 40 } }, 1,
    { { "JUST US TWO", { "JUNIE MISSES THE OLD HOUSE.", "TV TOGETHER, THEN WORK." }, { EV(E_TV, 1, "WATCH TV"), EV(W_PAID, 1, "EARN A PAYCHECK") } },
      { "FRESH AIR", { "A DAY AT THE PARK. YOU BOTH", "COULD USE A FRIEND." }, { AT(LOT_PARK, "GO TO A PARK"), KD(K_FRIENDS, 1, "MAKE A FRIEND") } },
      { "MONEY IS TIGHT", { "READ UP AT HOME AND ASK", "THE BOSS FOR MORE." }, { EV(W_READ, 1, "READ A BOOK"), EV(W_PROMO, 1, "GET A PROMOTION") } },
      { "OPEN DOOR", { "JUNIE WANTS COMPANY.", "HAVE SOMEONE OVER." }, { KD(K_GUEST, 0, "HAVE A NEIGHBOR OVER"), KD(K_KIDFRIEND, 0, "BECOME YOUR KID'S FRIEND") } },
      { "ONE MORE STEP", { "ONE MORE STEP UP THE LADDER,", "AND A NEST EGG FOR JUNIE." }, { EV(W_PROMO, 1, "GET ANOTHER PROMOTION"), KD(K_MONEY, 8000, "SAVE $8000") } } } },
  { "SKATE LIFE", "SKILL AND CAREER", { "DEX AND A BOARD. FROM THE", "STREET TO THE SKATE PLAZA", "TO A PRO CONTRACT." }, "A TRUE SKATER", { 40, 170, 190 },
    { "DEX", "VALDEZ", { 6,1,6,2,1,2,6,1,3, 0,0,0 }, AG_ADULT, AS_POP, 4, 0 }, { { 0 } }, 0,
    { { "THE STREET", { "A BOARD AND AN OPEN ROAD.", "O GETS ON IT. FIND SOME RAMPS." }, { EV(W_SKATE, 1, "RIDE YOUR BOARD"), AT(LOT_SKATE, "GO TO A SKATE PLAZA"), EV(E_TRICK, 10, "LAND 10 TRICKS") } },
      { "THE SKATE PLAZA", { "THE LOCALS ARE WATCHING.", "SHOW THEM WHAT YOU GOT." }, { KD(K_COMBO, 3, "LAND A 3 TRICK COMBO"), KD(K_FRIENDS, 1, "MAKE A FRIEND"), EV(E_TRICK, 25, "LAND 25 TRICKS") } },
      { "BIG AIR", { "THE BIG RAMP IS CALLING.", "GO HIGHER THAN EVER." }, { EV(E_BIGAIR, 1, "LAND A BIG AIR"), KD(K_COMBO, 4, "LAND A 4 TRICK COMBO") } },
      { "GOING PRO", { "A SPONSOR WANTS A STEADY", "WORKER. GET A JOB, MOVE UP." }, { EV(W_PAID, 1, "EARN A PAYCHECK"), EV(W_PROMO, 1, "GET A PROMOTION") } },
      { "THE WHOLE TOWN WATCHING", { "THE WHOLE TOWN KNOWS YOUR", "NAME. PROVE IT ONE LAST TIME." }, { EV(E_TRICK, 60, "LAND 60 TRICKS"), KD(K_COMBO, 6, "LAND A 6 TRICK COMBO") } } } },
  { "HOUSEFULL", "FRIENDSHIP AND HOME", { "BRAM SHARES WITH IVAN. FIX", "UP THE PLACE, FILL IT WITH", "FRIENDS FROM STRANGE TOWN." }, "A HOUSE FULL OF LIFE", { 96, 170, 70 },
    { "BRAM", "OKAFOR", { 3,2,3,1,1,3,1,2,4, 0,0,0 }, AG_ADULT, AS_PLEAS, 9, 0 },
    { { "IVAN", "BELOV", { 6,0,4,2,1,3,0,2,6, 0,0,0 }, AG_ADULT, AS_KNOW, 5, 20 } }, 1,
    { { "HOUSE RULES", { "IVAN HOGS THE TV AND EATS", "YOUR FOOD. MAKE PEACE." }, { EV(E_TV, 1, "WATCH TV"), KD(K_FRIEND, 0, "BECOME FRIENDS WITH IVAN") } },
      { "MAKE IT NICE", { "THE PLACE IS A MESS. BUILD", "AND BUY TILL IT FEELS HOME." }, { EV(W_BUY, 1, "BUY SOMETHING NEW"), KD(K_ROOM, 0, "MAKE A ROOM LOOK GREAT") } },
      { "OPEN HOUSE", { "THROW THE DOORS OPEN.", "HAVE THE NEIGHBORS OVER." }, { KD(K_GUEST, 0, "HAVE A NEIGHBOR OVER"), KD(K_FRIENDS, 3, "MAKE 3 FRIENDS") } },
      { "THE STRANGE TOWN CROWD", { "THE FUN PEOPLE HANG OUT IN", "STRANGE TOWN. BRING ONE HOME." }, { { K_TOWN, 0, 0, "GO TO STRANGE TOWN" }, KD(K_HOUSE, 3, "FILL THE HOUSE WITH 3 BORES") } },
      { "FOUR WALLS ONE FRIDGE", { "A FULL HOUSE.", "KEEP THE BILLS PAID." }, { EV(W_PROMO, 1, "GET A PROMOTION"), KD(K_MONEY, 15000, "SAVE $15000") } } } },
  { "BEST FRIENDS", "FRIENDSHIP", { "SAM, KIKI AND RUSS. NIGHTS", "AT THE BAR, THE TOWN SQUARE", "AND ONE TRUE BEST FRIEND." }, "FRIENDS FOR LIFE", { 150, 100, 220 },
    { "SAM", "LINDQVIST", { 1,4,3,1,2,4,5,5,2, 0,0,0 }, AG_ADULT, AS_KNOW, 8, 0 },
    { { "KIKI", "TANAKA", { 1,3,5,3,2,6,2,5,1, 0,0,0 }, AG_ADULT, AS_POP, 7, 20 }, { "RUSS", "OKONKWO", { 3,6,0,0,1,1,3,1,7, 0,0,0 }, AG_ADULT, AS_HOME, 4, 20 } }, 2,
    { { "TWO NEW FACES", { "KIKI AND RUSS JUST MOVED IN.", "BREAK THE ICE WITH A JOKE." }, { EV(E_JOKE, 1, "TELL A JOKE"), KD(K_FRIEND, 0, "BECOME FRIENDS WITH KIKI") } },
      { "NIGHT OUT", { "KIKI KNOWS A PLACE: A BAR", "ACROSS TOWN. FIRST ROUND?" }, { AT(LOT_BAR, "GO TO A BAR"), EV(W_NIGHT, 1, "HAVE A DRINK"), KD(K_FRIENDS, 2, "MAKE 2 FRIENDS") } },
      { "RIDE OR DIE", { "KIKI HAD A BAD DAY.", "BE THE ONE SHE CALLS." }, { EV(E_HUG, 1, "GIVE A HUG"), KD(K_BFF, 0, "BE KIKI'S BEST FRIEND") } },
      { "THE BIG HANGOUT", { "EVERYONE MEETS AT THE TOWN", "SQUARE, THEN BACK TO YOURS." }, { AT(LOT_SQUARE, "GO TO A TOWN SQUARE"), KD(K_GUEST, 0, "HAVE A NEIGHBOR OVER") } },
      { "DREAMS AND SAVINGS", { "DREAMS COME TRUE WITH", "FRIENDS AROUND." }, { EV(E_WANT, 5, "FULFIL 5 WANTS"), KD(K_MONEY, 5000, "SAVE $5000") } } } },
  { "RAGS TO RICHES", "MONEY AND CAREER", { "PENNY HAS NOTHING. A SHADY", "DEAL, A BUST, THEN THE", "STRAIGHT ROAD UP." }, "RICH AT LAST", { 60, 150, 90 },
    { "PENNY", "QUINN", { 0,2,1,2,2,1,3,2,5, 0,0,0 }, AG_ADULT, AS_FORTUNE, 9, 0 }, { { 0 } }, 0,
    { { "NOTHING BUT A BED", { "NO HOME OF HER OWN, $1000.", "GET TO WORK." }, { EV(W_PAID, 1, "EARN A PAYCHECK"), KD(K_MONEY, 3000, "SAVE $3000") } },
      { "EASY MONEY", { "A GUY AT A BAR HAS A JOB", "FOR YOU. NO QUESTIONS." }, { AT(LOT_BAR, "GO TO A BAR"), EV(W_NIGHT, 1, "HAVE A DRINK WITH HIM") } },
      { "BUSTED", { "IT WAS A SETUP. THE COPS", "TAKE YOUR CASH. START OVER." }, { KD(K_FINE, 0, "GET BUSTED"), EV(W_GYM, 1, "WORK OUT AT A GYM"), EV(W_PAID, 1, "EARN A PAYCHECK") } },
      { "CLEAN SLATE", { "ON PROBATION. KEEP YOUR HEAD", "DOWN AND YOUR NOSE CLEAN." }, { EV(W_READ, 1, "READ A BOOK"), KD(K_MONEY, 6000, "SAVE $6000") } },
      { "GOING STRAIGHT", { "A REAL JOB AND A REAL HOME.", "THIS TIME IT PAYS." }, { EV(W_PROMO, 1, "GET A PROMOTION"), KD(K_HOMEOWN, 0, "BUY A HOME OF YOUR OWN") } } } },
  { "CAREER CLIMBER", "CAREER AND SKILL", { "WES HAS A DESK AND A PLAN.", "THE BAR, A STRANGE TOWN", "DEAL, THE CORNER OFFICE." }, "TOP OF THE LADDER", { 50, 110, 200 },
    { "WES", "HARLOW", { 5,6,2,0,1,2,0,1,1, 0,0,0 }, AG_ADULT, AS_FORTUNE, 0, 0 }, { { 0 } }, 0,
    { { "FIRST DAY", { "A DESK, A PHONE, A FUTURE.", "WORK HARD AND STUDY UP." }, { EV(W_PAID, 1, "EARN A PAYCHECK"), EV(W_READ, 1, "READ A BOOK") } },
      { "NETWORKING", { "EVERYONE WHO MATTERS GOES", "TO THE BAR. MAKE A CONTACT." }, { AT(LOT_BAR, "GO TO A BAR"), KD(K_FRIENDS, 1, "MAKE A FRIEND") } },
      { "THE FIRST STEP", { "THE BOSS NOTICED YOU.", "NOW EARN IT." }, { EV(W_PROMO, 1, "GET A PROMOTION"), KD(K_MONEY, 6000, "SAVE $6000") } },
      { "THE BIG DEAL", { "A CLIENT WANTS TO MEET IN", "STRANGE TOWN. DON'T BE LATE." }, { { K_TOWN, 0, 0, "GO TO STRANGE TOWN" }, EV(E_WANT, 5, "FULFIL 5 WANTS") } },
      { "CORNER OFFICE", { "ONE MORE PROMOTION AND", "YOU ARE THE BOSS." }, { EV(W_PROMO, 1, "GET ANOTHER PROMOTION"), KD(K_SKILL, 3, "A SKILL AT LEVEL 3") } } } },
  { "NEW IN TOWN", "FRIENDSHIP AND TOWN", { "ROSA JUST MOVED IN WITH", "GWEN. SEE EVERY CORNER OF", "TOWN AND MAKE IT HOME." }, "YOU BELONG HERE", { 60, 160, 230 },
    { "ROSA", "MORENO", { 4,5,0,1,2,5,2,6,3, 0,0,0 }, AG_ADULT, AS_POP, 3, 0 },
    { { "GWEN", "ABERNATHY", { 4,2,1,1,2,0,6,7,1, 0,0,0 }, AG_ADULT, AS_PLEAS, 11, 20 } }, 1,
    { { "UNPACKING", { "BOXES EVERYWHERE AND A", "HOUSEMATE YOU DON'T KNOW." }, { EV(E_PIZZA, 1, "ORDER A PIZZA"), KD(K_FRIEND, 0, "BECOME FRIENDS WITH GWEN") } },
      { "SEE THE SIGHTS", { "TAKE A TAXI ROUND TOWN: THE", "TOWN SQUARE, THEN THE PARK." }, { AT(LOT_SQUARE, "GO TO A TOWN SQUARE"), AT(LOT_PARK, "GO TO A PARK") } },
      { "THE LOCALS", { "STRANGE TOWN IS WHERE THE", "STORIES ARE. MEET SOMEONE." }, { { K_TOWN, 0, 0, "GO TO STRANGE TOWN" }, KD(K_FRIENDS, 2, "MAKE 2 FRIENDS") } },
      { "OPEN HOUSE", { "HAVE THE NEW FRIENDS OVER.", "MAYBE ONE OF THEM STAYS." }, { KD(K_GUEST, 0, "HAVE A NEIGHBOR OVER"), KD(K_HOUSE, 3, "FILL THE HOUSE WITH 3 BORES") } },
      { "THE SKATE PLAZA CROWD", { "THE SKATERS WANT TO MEET", "YOU. LIVE A LITTLE." }, { AT(LOT_SKATE, "GO TO A SKATE PLAZA"), EV(E_WANT, 8, "FULFIL 8 WANTS") } } } },
  { "SECOND CHANCE", "ROMANCE AND REPAIR", { "LEO AND VAL FELL OUT. THE", "OLD CREW, THE TOWN SQUARE,", "ONE LAST CHANCE TO FIX IT." }, "WORTH FIXING", { 210, 70, 70 },
    { "LEO", "ASHBY", { 5,0,2,3,1,0,1,3,1, 0,0,0 }, AG_ADULT, AS_HOME, 5, 0 },
    { { "VAL", 0, { 5,3,2,3,1,5,0,6,4, 0,0,0 }, AG_ADULT, AS_POP, 10, -20 } }, 1,
    { { "THE COLD SHOULDER", { "VAL WON'T LOOK AT YOU.", "START SMALL: A JOKE, A CHAT." }, { EV(E_JOKE, 1, "TELL A JOKE"), KD(K_FRIEND, 0, "BECOME FRIENDS AGAIN") } },
      { "OLD HABITS", { "THE OLD CREW HANGS AT THE", "TOWN SQUARE. TELL THEM: DONE." }, { AT(LOT_SQUARE, "GO TO A TOWN SQUARE"), EV(E_FIGHT, 1, "STAND UP TO SOMEONE") } },
      { "FALLING AGAIN", { "VAL HEARD YOU WALKED AWAY.", "SHE IS SOFTENING." }, { EV(E_HUG, 1, "GIVE A HUG"), KD(K_LOVE, 0, "FALL BACK IN LOVE") } },
      { "PROVE IT", { "MAKE IT REAL THIS TIME AND", "TAKE HER SOMEWHERE NICE." }, { KD(K_STEADY, 0, "GO STEADY AGAIN"), AT(LOT_PARK, "GO TO A PARK") } },
      { "A HOME WORTH IT", { "A HOME WORTH COMING BACK TO.", "SAVE UP, MAKE IT NICE." }, { KD(K_MONEY, 8000, "SAVE $8000"), KD(K_ROOM, 0, "MAKE A ROOM LOOK GREAT") } } } },
  { "TV SHOW & TELL", "FAME AND RECOVERY", { "A FALLEN TV JUDGE WINS BACK", "HER FANS, AND SHAKES THE", "URGE FOR ONE MORE MARTINI." }, "HERE TODAY", { 120, 124, 140 },
    { "MISSY", "TELLER", { 4,1,0,4,1,9,0,4,0, 0,0,0 }, AG_ADULT, AS_POP, 3, 0 }, { { 0 } }, 0,
    { { "DANCING WITH THE BARS" }, { "PULL YOURSELF A SWEATER" }, { "THE WINNER TAKES IT ALL..." }, { "...THE LOSER HAS TO FALL" }, { "WHILE SHE'S DANCING WITH THE STARS" } } },
};
#undef AT
#undef EV
#undef KD
// a little picture per story, 9 x 8 (the GBA's own: a heart, a ring, a parent and kid, a board, a house, two friends, a coin, a ladder, a pin, a cracked heart, a TV)
static const char *const pic[STY_N][8] = { { 0 },
  { ".XX...XX.", "XXXX.XXXX", "XXXXXXXXX", "XXXXXXXXX", ".XXXXXXX.", "..XXXXX..", "...XXX...", "....X...." },
  { "....X....", "...XXX...", "....X....", "..XXXXX..", ".X.....X.", ".X.....X.", ".X.....X.", "..XXXXX.." },
  { ".XX......", ".XX......", "XXXX.....", "XXXX.XX..", ".XX..XX..", ".XX.XXXX.", ".XX..XX..", "XXXX.X.X." },
  { ".........", ".........", "XXXXXXXXX", ".XXXXXXX.", ".........", ".XX...XX.", ".XX...XX.", "........." },
  { "....X....", "...XXX...", "..XXXXX..", ".XXXXXXX.", "XXXXXXXXX", "XX.XXX.XX", "XX.XXX.XX", "XXXXXXXXX" },
  { ".XX...XX.", ".XX...XX.", "XXXX.XXXX", "XXXX.XXXX", ".XX...XX.", ".XX...XX.", ".X.X.X.X.", ".X.X.X.X." },
  { "..XXXXX..", ".XXXXXXX.", "XXXX.XXXX", "XXX...XXX", "XXXX.XXXX", "XXXXXXXXX", ".XXXXXXX.", "..XXXXX.." },
  { "......XX.", "......XX.", "....XXXX.", "....XX...", "..XXXX...", "..XX.....", "XXXX.....", "XXXX....." },
  { "XXXXXXX..", "XXXXXXXX.", "XXXXXXX..", "....X....", "....X....", "....X....", "....X....", "...XXX..." },
  { ".XX...XX.", "XXXX.XXXX", "XXXXX.XXX", "XXXX.XXXX", ".XXX.XXX.", "..XX.XX..", "...XXX...", "....X...." },
  { "X.......X", ".X.....X.", "XXXXXXXXX", "X.......X", "X...X...X", "X.......X", "XXXXXXXXX", ".X.....X." } };

// ---- the state (saved) ----
int story_id, story_ch;
static u8 st_done, st_cnt[3];     // the chapter's tasks ticked (bits), and the counts of the ones done more than once
static u8 st_part, st_kid;        // the story's person and its child (uids, 0: none)
static u8 st_rom;                 // how far you have fallen for st_part (0..100)
static u8 st_guest;               // someone came over this chapter
static u8 st_kidwait;             // a baby is on the way for the chapter (1)
static u8 st_card;                // a card to show: 1 a chapter begins, 2 one is done
static u8 st_combo;               // the best combo landed this chapter
static int st_sec;                // (once a second)

const char *story_name(int id) { return id > 0 && id < STY_N ? stories[id].name : ""; }
static const SChap *chap(void) { return story_id && story_ch < 5 ? &stories[story_id].ch[story_ch] : 0; }
static int ntask(const SChap *c) { int n = 0; if (c) while (n < 3 && c->t[n].k) n++; return n; }
static int reward(int ch) { return 500 + ch * 250; }   // $500, $750 ... $1500
static int sim_of(int uid) { if (!uid) return -1; for (int i = 0; i < nsims; i++) if (sims[i].uid == uid) return i; return -1; }
static int rel_with(int uid) { int i = sim_of(uid); return i < 0 || i == player ? -100 : sims[player].rel[i]; }

void story_reset(void) { story_id = 0; story_ch = 0; st_done = 0; st_cnt[0] = st_cnt[1] = st_cnt[2] = 0; st_part = st_kid = 0; st_rom = 0; st_guest = 0; st_kidwait = 0; st_card = 0; st_combo = 0; }

// ---- a new life in a story: who you are and who you live with ----
static void make_bore(Bore *b, const SPerson *p, const char *last) {
    look_default(b);
    for (int i = 0; i < 12; i++) b->look[i] = p->look[i];
    b->stage = p->stage; b->asp = p->stage < AG_TEEN ? AS_GROW : p->asp; look_set_sign(b, p->sign);
    strncpy(b->first, p->first, NAME_LEN - 1); b->first[NAME_LEN - 1] = 0;
    strncpy(b->last, p->last ? p->last : last, NAME_LEN - 1); b->last[NAME_LEN - 1] = 0;
    look_fix(b); look_build(b);
}
void story_household(int id, int px, int py) {   // (sim_init has run: nobody is home yet)
    story_reset(); story_id = id;
    const SStory *s = &stories[id]; Bore b;
    make_bore(&b, &s->lead, s->lead.last); sim_add(&b, px, py, 0); look_own_face(&sims[0].bore);
    for (int k = 0; k < s->ncast; k++) {
        make_bore(&b, &s->cast[k], s->lead.last); sim_add(&b, px + 2 - k * 3, py + 1 + k, 1);
        Sim *me = &sims[0], *o = &sims[nsims - 1]; int j = nsims - 1;
        me->rel[j] = o->rel[0] = s->cast[k].rel;
        o->age_days = 3;
        if (k == 0) {
            if (o->bore.stage < AG_TEEN) { st_kid = o->uid; o->par[0] = me->uid; }   // SINGLE PARENT: your girl
            else st_part = o->uid;
        }
        if (k == 1) { Sim *a = &sims[1]; a->rel[j] = o->rel[1] = 20; }   // (BEST FRIENDS: the two of them know each other a little)
    }
    sims[0].age_days = 3;
    if (id == STY_WED) { Sim *a = &sims[0], *o = &sims[1]; a->partner = o->uid; o->partner = a->uid; st_rom = 80; }   // just married
    if (id == STY_SKATE || id == STY_ROOM) has_board = 1;   // a board of your own (O gets on it)
}
static void chapter_begin(void) {
    st_done = 0; st_cnt[0] = st_cnt[1] = st_cnt[2] = 0; st_guest = 0; st_combo = 0; st_kidwait = 0; st_card = 1;
    const SChap *c = chap();
    if (c && c->kid) {   // a baby comes home tomorrow (the story's two, or you alone)
        if (baby_due < 0) { if (nsims < MAX_SIMS) sim_baby_coming(sims[player].uid, st_part, 1440); else sim_note("THE HOUSE IS FULL: NO ROOM FOR A BABY"); }
        st_kidwait = 1;
    }
}
void story_begin(void) { if (story_id) { story_ch = 0; chapter_begin(); } }

// ---- is a task done? ----
static int friends(void) {
    int n = citizens_friends();
    for (int i = 0; i < nsims; i++) if (i != player && sims[player].rel[i] >= 50) n++;
    return n;
}
static int place_here(int lot) {
    if (world_zone != ZONE_CITY || world_wbx < 0) return 0;
    BInfo b; bm_block(world_wbx + sim_tile_x(&sims[player]) / BM_BLK, world_wby + sim_tile_y(&sims[player]) / BM_BLK, &b);
    return b.lot == lot;
}
static int skill_top(void) { const Sim *s = &sims[player]; int a = wants_skill_level(s->skill_cook), b = wants_skill_level(s->skill_body), c = wants_skill_level(s->skill_fun); return MAX(a, MAX(b, c)); }
static int task_now(const STask *t, int i) {   // a task that is a state, not an event
    switch (t->k) {
        case K_AT: return place_here(t->a);
        case K_TOWN: return world_zone == ZONE_TOWN;
        case K_HOME: return home_is_here();
        case K_EV: return st_cnt[i] >= t->n;
        case K_COMBO: return st_combo >= t->n;
        case K_FRIEND: return rel_with(st_part) >= 50;
        case K_LOVE: return story_loved();
        case K_STEADY: { int j = sim_of(st_part); return j >= 0 && sims[player].partner == st_part; }
        case K_BFF: return rel_with(st_part) >= 90;
        case K_FRIENDS: return friends() >= t->n;
        case K_MONEY: return cash >= (long)t->n;
        case K_HOUSE: return nsims >= t->n;
        case K_GUEST: return st_guest;
        case K_KIDFRIEND: return rel_with(st_kid) >= 50;
        case K_ROOM: return sim_need_val(&sims[player], N_ROOM) >= 80;
        case K_HOMEOWN: return home_city;
        case K_SKILL: return skill_top() >= t->n;
    }
    return 0;
}
static void task_done(int i, const STask *t) {
    st_done |= (u8)(1 << i);
    char b[40]; snprintf(b, sizeof b, "DONE: %s", t->nm); sim_note(b); audio_voice(VOICE_FINISHED);
}
static void chapter_done(void) {
    int pay = reward(story_ch); cash += pay;
    char b[40]; snprintf(b, sizeof b, "CHAPTER %d DONE  +$%d", story_ch + 1, pay); sim_note(b);
    audio_voice(VOICE_WIN);
    story_ch++;
    if (story_ch < 5) chapter_begin(); else st_done = 0;
    st_card = 2;   // the CHAPTER COMPLETE card (then the new chapter's own: story_card_shown)
}
void story_event(int ev, int arg) {
    if (!story_id || story_ch >= 5) return;
    if (ev == E_FLIRT && arg == st_part) st_rom = (u8)MIN(100, st_rom + 18);   // falling for the story's person: flirts, hugs, chats
    if (ev == E_HUG && arg == st_part) st_rom = (u8)MIN(100, st_rom + 6);
    if (ev == E_TALK && arg == st_part) st_rom = (u8)MIN(100, st_rom + 2);
    if ((ev == W_PARTY || ev == E_CITCHAT) && home_is_here()) st_guest = 1;   // someone over: a party at yours, or a chat on your own lot
    if (ev == E_TRICK && arg > st_combo) st_combo = (u8)MIN(arg, 255);
    const SChap *c = chap(); int n = ntask(c);
    for (int i = 0; i < n; i++) {
        const STask *t = &c->t[i];
        if (st_done >> i & 1 || t->k != K_EV || t->a != ev) continue;
        if (st_cnt[i] < 255) st_cnt[i]++;
        if (st_cnt[i] >= t->n) task_done(i, t);
    }
}
void story_tick(int steps) {
    if (!story_id || story_ch >= 5) return;
    st_sec += steps; if (st_sec < 30) return; st_sec = 0;
#ifdef STORY_SKIP
    { static int sk; if (++sk >= 24) { sk = 0; chapter_done(); return; } }   // (test builds: a chapter every 12 seconds)
#endif
    const SChap *c = chap(); int n = ntask(c);
    if (c->kid) {   // a baby on the way: the chapter ends when it is home
        if (st_kidwait && baby_due < 0) {
            int k = -1; for (int i = 0; i < nsims; i++) if (sims[i].bore.stage == AG_BABY || sims[i].bore.stage == AG_CHILD) if (k < 0 || sims[i].uid > sims[k].uid) k = i;
            if (k >= 0) st_kid = sims[k].uid;
            st_kidwait = 0; chapter_done();
        }
        return;
    }
    for (int i = 0; i < n; i++) {
        const STask *t = &c->t[i];
        if (st_done >> i & 1) continue;
        if (t->k == K_FINE) {   // RAGS TO RICHES: the setup. The cops take most of what you have
            long fine = cash - cash / 4; cash -= fine;
            char b[40]; snprintf(b, sizeof b, "BUSTED! THE COPS TOOK $%ld", fine); sim_note(b); audio_voice(VOICE_CRY);
            task_done(i, t); continue;
        }
        if (t->k != K_EV && task_now(t, i)) task_done(i, t);
    }
    if (n && st_done == (1 << n) - 1) chapter_done();
}
int story_card(void) { return st_card; }
void story_card_shown(void) { st_card = st_card == 2 && story_ch < 5 ? 1 : 0; }   // (CHAPTER COMPLETE, then the new chapter's own card)
int story_place(int *lot) {
    const SChap *c = chap(); int n = ntask(c);
    for (int i = 0; i < n; i++) {
        if (st_done >> i & 1) continue;
        if (c->t[i].k == K_AT) { *lot = c->t[i].a; return 1; }
        if (c->t[i].k == K_TOWN) return 2;
        if (c->t[i].k == K_HOME && !home_is_here()) return 3;
    }
    return 0;
}
int story_town_open(void) { int l; return story_place(&l) == 2; }
int story_is_partner(int uid) { return story_id && uid && uid == st_part; }
int story_loved(void) { return st_part && st_rom >= 60 && rel_with(st_part) >= 50; }

// ---- what the tasks say, and how far along ----
static void task_line(const STask *t, int i, int done, char *out, int cap) {
    if (done) { snprintf(out, cap, "%s", t->nm); return; }
    int v = -1, of = t->n;
    switch (t->k) {
        case K_EV: if (t->n > 1) v = st_cnt[i]; break;
        case K_FRIENDS: if (t->n > 1) v = friends(); break;
        case K_MONEY: snprintf(out, cap, "%s  $%ld", t->nm, MIN(cash, (long)t->n)); return;
        case K_HOUSE: v = nsims; break;
        case K_FRIEND: case K_BFF: case K_KIDFRIEND: { int r = rel_with(t->k == K_KIDFRIEND ? st_kid : st_part); snprintf(out, cap, "%s  %d/%d", t->nm, MAX(r, -100), t->k == K_BFF ? 90 : 50); return; }
        case K_LOVE: snprintf(out, cap, "%s  %d%%", t->nm, MIN(st_rom * 100 / 60, 100)); return;
        case K_COMBO: if (st_combo > 1) { v = st_combo; } break;
        case K_SKILL: v = skill_top(); break;
    }
    if (v >= 0) snprintf(out, cap, "%s  %d/%d", t->nm, MIN(v, of), of); else snprintf(out, cap, "%s", t->nm);
}
const char *story_hud(char *out) {
    const SChap *c = chap(); int n = ntask(c);
    if (!c) return 0;
    if (c->kid) { sprintf(out, "CH%d  A BABY COMES HOME", story_ch + 1); return out; }
    for (int i = 0; i < n; i++) if (!(st_done >> i & 1)) { char l[40]; task_line(&c->t[i], i, 0, l, sizeof l); snprintf(out, 44, "CH%d  %s", story_ch + 1, l); return out; }
    return 0;
}

// ---- the save ----
void story_save(void) {
    sv_u8(story_id); sv_u8(story_ch); sv_u8(st_done); sv_n(st_cnt, 3); sv_u8(st_part); sv_u8(st_kid); sv_u8(st_rom); sv_u8(st_guest); sv_u8(st_kidwait); sv_u8(st_combo);
}
void story_load(void) {
    story_reset();
    story_id = ld_u8(); story_ch = ld_u8(); st_done = (u8)ld_u8(); ld_n(st_cnt, 3); st_part = (u8)ld_u8(); st_kid = (u8)ld_u8(); st_rom = (u8)ld_u8(); st_guest = (u8)ld_u8(); st_kidwait = (u8)ld_u8(); st_combo = (u8)ld_u8();
    if (story_id >= STY_N) story_reset();
    if (story_ch > 5) story_ch = 5;
}

// ---- the look: the story's picture, the cards, the journal, the picker ----
static u32 scol(int id, int k) { const u8 *c = stories[id].col; return UI_RGB(c[0] * k / 8, c[1] * k / 8, c[2] * k / 8); }   // the story's colour, k/8 as bright
static u32 slight(int id) { const u8 *c = stories[id].col; return UI_RGB((c[0] + 255) / 2, (c[1] + 255) / 2, (c[2] + 255) / 2); }
static void story_tile(int id, int x, int y, int sz, int t) {   // the story's picture on a rounded tile in its colour: white blocks with a soft shadow, gently beating
    ui_round(x, y, sz, sz, sz / 6, slight(id), scol(id, 8), 0);
    int sc = (sz - 8) / 9; if (sc < 1) sc = 1;
    int beat = ((t >> 4) & 7) == 0 ? 1 : 0;
    int ox = x + (sz - sc * 9) / 2, oy = y + (sz - sc * 8) / 2 - beat;
    for (int r = 0; r < 8; r++) for (int q = 0; q < 9; q++) if (pic[id][r][q] == 'X') gfx_rect(ox + q * sc + 1, oy + r * sc + 2, sc, sc, 0, 0, 0, 1);
    for (int r = 0; r < 8; r++) for (int q = 0; q < 9; q++) if (pic[id][r][q] == 'X') {
        gfx_rect(ox + q * sc, oy + r * sc, sc, sc, 255, 255, 255, 0);
        if (sc >= 3) gfx_rect(ox + q * sc, oy + r * sc + sc - 1, sc, 1, 214, 222, 236, 0);
    }
}
static void tick_disc(int cx, int cy, int st) {   // 2 done (a green tick), 1 now (a gold ring), 0 to come
    if (st == 2) ui_icon_disc(cx, cy, 13, IC_TICK, UI_GREEN, UI_WHITE);
    else if (st == 1) { ui_disc(cx, cy, 13, UI_RGB(236, 176, 40)); ui_disc(cx, cy, 8, UI_WHITE); ui_disc(cx, cy, 4, UI_RGB(236, 176, 40)); }
    else { ui_disc(cx, cy, 13, UI_RGB(186, 196, 210)); ui_disc(cx, cy, 9, UI_WHITE); }
}
static void tasks_draw(int x, int y, int w, int dy) {   // this chapter's tasks, ticked
    const SChap *c = chap(); int n = ntask(c); char l[48];
    if (c && c->kid) { tick_disc(x + 6, y + 5, 1); ui_text(x + 16, y + 2, 1, story_ch < 5 ? "A BABY COMES HOME TOMORROW" : "", UI_INK); return; }
    for (int i = 0; i < n; i++) {
        int d = st_done >> i & 1; task_line(&c->t[i], i, d, l, sizeof l);
        ui_round(x, y + i * dy - 2, w, dy - 2, 4, d ? UI_RGB(234, 249, 234) : UI_WHITE, d ? UI_RGB(212, 238, 212) : UI_PANEL, 0);
        tick_disc(x + 8, y + i * dy + 3, d ? 2 : 0);
        ui_text(x + 18, y + i * dy + 1, 1, l, d ? UI_GOODT : UI_INK);
    }
}
static struct { s16 x, y, vx, vy; u8 c, w; } cf[36]; static int cf_on;
static void confetti(int t) {
    static const u8 pal[6][3] = { {255, 90, 110}, {255, 200, 50}, {90, 200, 255}, {120, 220, 100}, {200, 120, 255}, {255, 150, 60} };
    if (t == 0 || !cf_on) { cf_on = 1; u32 r = 0x2545F491u ^ (u32)VSync(-1);
        for (int k = 0; k < 36; k++) { r ^= r << 13; r ^= r >> 17; r ^= r << 5; cf[k].x = (s16)((r % 320) << 4); cf[k].y = (s16)(-(int)((r >> 9) % 160) << 4);
            cf[k].vx = (s16)((int)((r >> 17) % 33) - 16); cf[k].vy = (s16)(14 + (r >> 23) % 22); cf[k].c = (u8)((r >> 3) % 6); cf[k].w = (u8)(2 + (r >> 27) % 3); } }
    for (int k = 0; k < 36; k++) {
        cf[k].x += cf[k].vx; cf[k].y += cf[k].vy; if ((cf[k].y >> 4) > 240) cf[k].y -= 260 << 4;
        int x = cf[k].x >> 4, y = cf[k].y >> 4; if (y < 0 || x < 0 || x > 318) continue;
        const u8 *c = pal[cf[k].c]; int tall = ((t + k) >> 3) & 1;
        gfx_rect(x, y, tall ? 2 : cf[k].w, tall ? cf[k].w : 2, c[0], c[1], c[2], 0);
    }
}
static void band(int id, int x, int y, int w, int h) {   // a header band in the story's colour
    ui_round(x, y, w, h, 6, slight(id), scol(id, 7), 0);
    gfx_rect(x, y + h - 6, w, 6, (stories[id].col[0] * 7) >> 3, (stories[id].col[1] * 7) >> 3, (stories[id].col[2] * 7) >> 3, 0);
}
void story_card_draw(int kind, int t) {
    if (!story_id) return;
    const SStory *s = &stories[story_id]; char b[48];
    int done = kind == 2, end = story_ch >= 5, shown = done ? story_ch - 1 : story_ch;   // (a done card: the chapter just finished, and the next one under it)
    if (done) confetti(t); else cf_on = 0;
    const int CX = 22, CY = 20, CW = 276, CH = 200;
    ui_card(CX, CY, CW, CH);
    band(story_id, CX + 1, CY + 1, CW - 2, 46);
    story_tile(story_id, CX + 8, CY + 6, 36, t);
    gfx_text_raw(CX + 52, CY + 8, 2, s->name, 255, 255, 255); gfx_text_raw(CX + 52, CY + 26, 1, s->tag, 255, 255, 255);
    sprintf(b, done ? "CHAPTER %d COMPLETE" : end ? "THE END" : "CHAPTER %d OF 5", shown + 1);
    ui_badge(CX + CW - ui_tw(1, b) - 18, CY + 26, b, UI_WHITE, done ? UI_GOODT : scol(story_id, 6));
    int y = CY + 54;
    if (done) {
        ui_textc(160, y, 2, stories[story_id].ch[shown].title, UI_INK);
        sprintf(b, "+$%d", reward(shown)); ui_icon_disc(160 - ui_tw(2, b) / 2 - 10, y + 25, 15, IC_MONEY, UI_GREEN, UI_WHITE); ui_text(160 - ui_tw(2, b) / 2 + 2, y + 19, 2, b, UI_GOODT);
        y += 40;
    }
    gfx_rect(CX + 10, y, CW - 20, 1, 196, 206, 220, 0); y += 5;
    if (end) {
        ui_textc(160, y + 6, 2, s->end, scol(story_id, 6));
        ui_textc(160, y + 30, 1, "THE STORY IS DONE. YOUR LIFE GOES ON:", UI_SOFT);
        ui_textc(160, y + 42, 1, "KEEP PLAYING.", UI_SOFT);
    } else {
        const SChap *c = chap();
        sprintf(b, "%s%s", done ? "NEXT: " : "", c->title); ui_text(CX + 12, y, done ? 1 : 2, b, done ? UI_SOFT : UI_INK);
        y += done ? 12 : 18;
        ui_text(CX + 12, y, 1, c->br[0], UI_INK); ui_text(CX + 12, y + 10, 1, c->br[1], UI_INK); y += 24;
        if (!done) { ui_text(CX + 12, y, 1, c->kid ? "WHAT HAPPENS:" : "DO ALL OF THESE, IN ANY ORDER:", UI_SOFT); y += 11; }
        tasks_draw(CX + 10, y, CW - 20, 13);
    }
    int lot; int pl = !end && story_place(&lot);
    ui_hints(160, CY + CH - 14, pl ? "X OK|TRI TAXI THERE $20" : "X OK", 1);
}
void story_journal_draw(int t) {
    const SStory *s = &stories[story_id]; char b[48];
    int top = ui_window(10, 6, 300, 228, "STORY JOURNAL", IC_BOOK);
    story_tile(story_id, 18, top + 2, 28, t);
    ui_text(52, top + 2, 2, s->name, UI_INK); ui_text(52, top + 18, 1, s->tag, UI_SOFT);
    int n5 = MIN(story_ch, 5);
    sprintf(b, story_ch >= 5 ? "THE END" : "CHAPTER %d OF 5", story_ch + 1); ui_text(300 - ui_tw(1, b), top + 3, 1, b, UI_INK);
    ui_bar(214, top + 15, 86, 7, n5 * 20, scol(story_id, 8));
    int y = top + 34;
    for (int i = 0; i < 6; i++) {   // the timeline: the five chapters and THE END
        int st = i < story_ch ? 2 : i == story_ch ? 1 : 0, ry = y + i * 13;
        if (i < 5) gfx_rect(22, ry + 6, 2, 13, st == 2 ? 84 : 196, st == 2 ? 196 : 206, st == 2 ? 72 : 220, 0);
        if (st == 1) ui_round(30, ry - 2, 272, 12, 4, UI_HILITE, UI_RGB(196, 224, 246), 0);
        tick_disc(23, ry + 3, st);
        const char *nm = i < 5 ? s->ch[i].title : s->end;
        if (i == 5) { snprintf(b, sizeof b, "THE END: %s", nm); nm = b; }
        ui_text(36, ry, 1, nm, st == 2 ? UI_GOODT : st == 1 ? UI_INK : UI_SOFT);
        if (i < 5) { char r[12]; sprintf(r, "+$%d", reward(i)); ui_text(298 - ui_tw(1, r), ry, 1, r, st == 2 ? UI_GOODT : UI_SOFT); }
    }
    y += 6 * 13 + 2;
    gfx_rect(18, y, 284, 1, 196, 206, 220, 0); y += 5;
    if (story_ch >= 5) {
        ui_textc(160, y + 8, 2, s->end, scol(story_id, 6));
        int tot = 0; for (int i = 0; i < 5; i++) tot += reward(i);
        sprintf(b, "ALL FIVE CHAPTERS DONE. EARNED $%d", tot); ui_textc(160, y + 30, 1, b, UI_SOFT);
        ui_hints(160, 218, "TRI THE STORY SO FAR|O BACK", 1);
        return;
    }
    const SChap *c = chap();
    ui_text(18, y, 1, c->br[0], UI_INK); ui_text(18, y + 10, 1, c->br[1], UI_INK); y += 24;
    tasks_draw(16, y, 288, 14);
    int lot; ui_hints(160, 218, story_place(&lot) ? "X TAXI THERE $20|TRI THE STORY SO FAR|O BACK" : "TRI THE STORY SO FAR|O BACK", 1);
}
void story_pick_draw(int sel, int t) {
    const SStory *s = &stories[sel]; char b[48];
    int top = ui_window(6, 6, 308, 228, "STORY MISSIONS", IC_BOOK), lock = sel > STY_PLAYABLE;
    story_tile(sel, 14, top + 2, 64, t);
    ui_text(86, top + 2, 2, s->name, UI_INK);
    ui_badge(86, top + 20, s->tag, slight(sel), scol(sel, 5));
    for (int i = 0; i < 3; i++) ui_text(86, top + 36 + i * 10, 1, s->blurb[i], UI_INK);
    if (lock) { ui_text(14, top + 74, 1, "MISSY'S STORY IS ON ITS WAY: VOICED AND FILMED.", UI_BADT); ui_text(14, top + 84, 1, "COMING SOON.", UI_SOFT); }
    else {
        if (s->ncast == 2) snprintf(b, sizeof b, "YOU ARE %s. WITH %s AND %s", s->lead.first, s->cast[0].first, s->cast[1].first);
        else if (s->ncast) snprintf(b, sizeof b, "YOU ARE %s. WITH %s", s->lead.first, s->cast[0].first);
        else snprintf(b, sizeof b, "YOU ARE %s. ON YOUR OWN", s->lead.first);
        ui_text(14, top + 74, 1, b, UI_BLUE2);
    }
    int y = top + 90;
    gfx_rect(14, y, 292, 1, 196, 206, 220, 0); y += 4;
    ui_text(14, y, 1, "THE CHAPTERS", UI_SOFT);
    { int pay = 0; for (int i = 0; i < 5; i++) pay += reward(i); sprintf(b, "PAYS $%d IN ALL", pay); ui_text(306 - ui_tw(1, b), y, 1, b, lock ? UI_SOFT : UI_GOODT); }
    y += 12;
    for (int i = 0; i < 6; i++) {
        const char *nm = i < 5 ? s->ch[i].title : s->end;
        ui_round(14, y + i * 13 - 1, 292, 12, 4, i == 5 ? slight(sel) : UI_PANEL, i == 5 ? slight(sel) : UI_PANEL, 0);
        if (i < 5) { sprintf(b, "%d", i + 1); ui_disc(22, y + i * 13 + 4, 10, scol(sel, 7)); gfx_text_raw(20, y + i * 13 + 1, 1, b, 255, 255, 255); }
        else ui_icon(16, y + i * 13 - 1, IC_STAR, scol(sel, 5));
        if (i == 5) snprintf(b, sizeof b, "THE END: %s", nm);
        ui_text(32, y + i * 13 + 1, 1, i == 5 ? b : nm, lock ? UI_SOFT : UI_INK);
    }
    y += 6 * 13 + 4;
    for (int i = 1; i < STY_N; i++) { int x = 160 - (STY_N - 1) * 7 + (i - 1) * 14; ui_disc(x + 4, y + 3, i == sel ? 9 : 6, i == sel ? scol(i, 8) : UI_RGB(196, 206, 220)); }
    ui_hints(160, 218, lock ? "LEFT RIGHT STORY|O BACK" : "LEFT RIGHT STORY|X START|O BACK", 1);
}
