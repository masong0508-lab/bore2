// look.c - the BORE person: the GBA game's look -> voxel builder, lifted as-is, wrapped for many people and the PS1.
#include "look.h"
#include "tex.h"
#include "voxel.h"

// ---- shims for the GBA globals the ported code reads ----
static u8 look[LK_N];
static u8 sUnlock = 0;                    // the Konami code: BIG HEAD and the debug bodies
static inline int mcDbl(void) { return 0; }   // MASTER CONTROLLER cheats: off
static inline int mcBig(void) { return 0; }
static int slideEffS(int v);
static u8 xo_dummy;

#include "look_port.inc"
#include "face_more.inc"

static int slideEffS(int v) { return slidePos(v) - 4; }

// ---- names the creator shows ----
static const char *const shapeNm[NSHAPE]={"AVERAGE","BROAD","BIG HEAD","STUBBY","SLIM","ATHLETIC","TALL","CHUBBY","PEAR","LANKY","STOCKY","HUNCHED","POTBELLY","MUSCLE","PETITE","BARREL","DIGITIGRADE","V-SHAPE","CURVY","RUNNER","SOFT","POWER","LONG LEGS","PUDGY","TODDLER","SPROUT","SPORTY","BELL","STURDY","SPIDER"};
static const char *const fearNm[5]={"NONE","CAT","FOX","BUNNY","BEAR"};
static const char *const muzNm[4]={"NONE","SNOUT","MUZZLE","BEAK"};
static const char *const ftailNm[4]={"NONE","FOX","CAT","BUNNY"};
#define NEAR_ALL 9
static const char *const earNm[NEAR_ALL]={"NONE","SMALL","BIG","ELF","LONG ELF","TINY","STICK-OUT","DROOPY","HUGE"};
static const char *const hairNm[NHAIR]={"CROP","BOWL","LONG","BALD","SPIKY","AFRO","FLAT TOP","SIDE TAIL","BUN","BOB","PONYTAIL","PIGTAILS","MOHAWK","PIXIE","CURLS"};
static const char *const hatNm[6]={"NONE","CAP","BEANIE","BAND","FEZ","HELMET"};
static const char *const hatColNm[6]={"AS THE TOP","AS THE BOTTOM","WHITE","BLACK","RED","GOLD"};
static const char *const beardNm[3]={"NONE","BEARD","LONG BEARD"};
static const char *const topStyNm[5]={"TEE","LONG SLEEVE","TANK","HOODIE","BARE"};
static const char *const botStyNm[4]={"PANTS","SHORTS","SKIRT","BARE"};
static const char *const shoeNm[6]={"AS THE BOTTOM","WHITE","BLACK","RED","GOLD","AS THE TOP"};
static const char *const stageNm2[AG_N]={"BABY","CHILD","TEEN","ADULT","ELDER"};
static const u8 lkCnt[LK_N]={NSHAPE,NSKIN,NEYE,NMOUTH,3,NHAIR,NSW,NSW,NSW,9,9,9,3,3,3,6,6,3,4,3,6, 6,6,5,4,NSW, 9,9,9,9,9,9,9, 4,3,7,6, 9,9,9,9,9,9,9,9,9, 5,4,4, 9,9,9, 9,9,9,9,9,9,9, 9,9,9,9,9,9,9, 9,9,9,7, 9,9,9, 9};
static int lk_cnt(int id) {   // the GBA counts, with the bigger FACE tab's
    switch (id) {
        case LK_EYES: return NEYE_ALL; case LK_MOUTH: return NMOUTH_ALL; case LK_EARS: return NEAR_ALL; case LK_BROW: return NBROW_ALL;
        case LK_NOSE: return NNOSE_ALL; case LK_CHEEK: return NCHEEK_ALL; case LK_GLASS: return NGLASS_ALL;
        default: return lkCnt[id];
    }
}
_Static_assert(LK_X_END <= LK_COUNT, "LK_COUNT in look.h must hold every look");
#include "wear_names.inc"
// OUTFITS: ready-made looks from the wardrobe (head, face, neck, back, hand, waist, ears, wrists, shoulder, feet; colour 1 and 2; the top's pattern), by name
typedef struct { const char *nm; const char *w[10]; u8 ca, cb, pat; } Theme;
static const Theme themes[] = {
    { "PICK ONE", { 0 }, 0, 0, 0 },
    { "NOTHING ON", { 0 }, 0, 0, 0 },
    { "PIRATE", { "PIRATE HAT", "EYE PATCH", 0, 0, "SWORD", "SASH", "BIG HOOPS", 0, "PARROT", "COWBOY BOOTS" }, 0, 13, 1 },
    { "ASTRONAUT", { "ASTRONAUT HELMET", 0, 0, "JETPACK", 0, "UTILITY BELT", 0, "SMART WATCH", 0, "ROCKET BOOTS" }, 11, 7, 0 },
    { "CHEF", { "CHEF HAT", "MUSTACHE", 0, 0, "SPATULA", "APRON", 0, 0, 0, 0 }, 11, 0, 4 },
    { "SKATER", { "BACKWARDS CAP", 0, 0, "SKATEBOARD", 0, 0, "STUDS", "SWEATBANDS", 0, "HIGH TOPS" }, 0, 13, 0 },
    { "ROCKER", { 0, "FUNKY SHADES", "SPIKED COLLAR", "GUITAR", 0, "CHAIN BELT", "GAUGES", "SPIKED CUFFS", 0, 0 }, 13, 0, 0 },
    { "WIZARD", { "WIZARD HAT", 0, 0, "CAPE", "MAGIC WAND", 0, 0, 0, "OWL", 0 }, 9, 2, 9 },
    { "KNIGHT", { "KNIGHT HELMET", 0, 0, "SHIELD", "SWORD", "BELT", 0, "LEATHER STRAP", 0, 0 }, 0, 15, 0 },
    { "COWBOY", { "COWBOY HAT", 0, "BANDANA", 0, 0, "LASSO", 0, "LEATHER STRAP", 0, "COWBOY BOOTS" }, 14, 0, 5 },
    { "DIVER", { 0, "SCUBA MASK", 0, "SCUBA TANK", 0, 0, 0, "SMART WATCH", 0, "FLIPPERS" }, 2, 13, 0 },
    { "FAIRY", { "FLOWER CROWN", 0, 0, "FAIRY WINGS", "MAGIC WAND", "TUTU", "PEARL DROPS", "CHARM BRACELET", 0, 0 }, 10, 6, 0 },
    { "CLOWN", { "JESTER HAT", "CLOWN NOSE", "RUFF COLLAR", 0, "BALLOON", 0, 0, 0, 0, "CLOWN SHOES" }, 0, 2, 6 },
    { "SUPERHERO", { 0, "SUPERHERO MASK", 0, "CAPE", 0, "CHAMPION BELT", 0, "SWEATBANDS", 0, "RAIN BOOTS" }, 0, 7, 0 },
    { "DETECTIVE", { "FEDORA", "PIPE", "TIE", 0, "MAGNIFIER", 0, 0, "GOLD WATCH", 0, 0 }, 14, 13, 3 },
    { "SAILOR", { "SAILOR CAP", 0, "BANDANA", 0, "FISHING ROD", 0, 0, 0, "PARROT", 0 }, 7, 0, 1 },
    { "ATHLETE", { "SWEATBAND", 0, "WHISTLE", 0, "BASKETBALL", 0, 0, "SWEATBANDS", 0, "HIGH TOPS" }, 0, 11, 2 },
    { "VIKING", { "VIKING HELMET", "FULL BEARD", 0, "SHIELD", "HAMMER", "BELT", 0, "LEATHER STRAP", 0, 0 }, 14, 0, 5 },
    { "BEACH DAY", { "STRAW HAT", "AVIATORS", "FLOWER LEI", "SURFBOARD", "SMOOTHIE", "GRASS SKIRT", 0, "FRIENDSHIP BRACELET", 0, "SANDALS" }, 5, 1, 0 },
    { "SNOW DAY", { "POM BEANIE", "SKI GOGGLES", "LONG SCARF", "SNOWBOARD", "COFFEE", 0, 0, 0, 0, "SKI BOOTS" }, 0, 11, 5 },
    { "PARTY ANIMAL", { "PARTY HAT", "FUNKY SHADES", "FEATHER BOA", "BALLOON BUNCH", "COTTON CANDY", 0, "STAR EARRINGS", "BANGLES", 0, 0 }, 10, 2, 9 },
    { "SPOOKY", { "WITCH HAT", 0, 0, "BAT WINGS", "LANTERN", 0, 0, 0, "GHOST", 0 }, 9, 1, 0 },
    { "SANTA", { "SANTA HAT", "FULL BEARD", 0, 0, 0, "BIG BUCKLE", 0, 0, 0, "COWBOY BOOTS" }, 0, 13, 0 },
    { "EXPLORER", { "SAFARI HAT", 0, "CAMERA", "BACKPACK", "MAP", "POUCH", 0, "WATCH", "PARROT", 0 }, 14, 4, 0 },
    { "ROYALTY", { "CROWN", "MONOCLE", "PEARLS", "CAPE", "TROPHY", 0, "PEARL DROPS", "GOLD WATCH", 0, 0 }, 9, 15, 8 },
    { "GAMER", { "HEADPHONES", "VR VISOR", 0, "LAPTOP BAG", "GAME PAD", 0, 0, "SMART WATCH", "ROBOT BUDDY", 0 }, 13, 6, 0 },
    { "ROCK STAR", { 0, "STAR SHADES", "GOLD CHAIN", 0, "MICROPHONE", "CHAIN BELT", "BIG HOOPS", "BANGLES", 0, "HIGH TOPS" }, 13, 15, 14 },
    { "GARDENER", { "STRAW HAT", 0, 0, 0, "WATERING CAN", "TOOL BELT", 0, "GARDEN GLOVES", 0, "RAIN BOOTS" }, 4, 2, 5 },
    { "BUILDER", { "HARD HAT", 0, 0, 0, "HAMMER", "TOOL BELT", 0, 0, 0, "COWBOY BOOTS" }, 1, 14, 5 },
    { "DOCTOR", { 0, "FACE MASK", "STETHOSCOPE", 0, 0, 0, 0, "WATCH", 0, 0 }, 11, 5, 0 },
    { "POLICE", { "POLICE CAP", "AVIATORS", "WHISTLE", 0, "FLASHLIGHT", "UTILITY BELT", 0, "WATCH", 0, 0 }, 7, 13, 0 },
    { "SCHOLAR", { "GRAD CAP", "NERD GLASSES", "TIE", "BACKPACK", "BOOK", 0, 0, "WATCH", 0, 0 }, 8, 0, 5 },
    { "HIPSTER", { "BERET", "CAT-EYE GLASSES", "LONG SCARF", 0, "COFFEE", 0, "EAR CUFF", "FRIENDSHIP BRACELET", 0, 0 }, 23, 22, 15 },
    { "SAMURAI", { "HEAD WRAP", 0, 0, "KATANA", 0, "KARATE BELT", 0, 0, 0, "SANDALS" }, 0, 13, 0 },
    { "DJ", { "BACKWARDS CAP", "FUNKY SHADES", "NECK HEADPHONES", 0, "MEGAPHONE", 0, "STUDS", "BANGLES", 0, "HIGH TOPS" }, 26, 13, 7 },
    { "BAKER", { "CHEF HAT", 0, 0, 0, "DONUT", "APRON", 0, 0, 0, 0 }, 10, 3, 20 },
    { "CAMPER", { "BUCKET HAT", 0, 0, "BEDROLL", "LANTERN", "POUCH", 0, "WATCH", 0, "RAIN BOOTS" }, 4, 14, 15 },
    { "MAGICIAN", { "TOP HAT", "MASQUERADE MASK", "BOW TIE", "CAPE", "MAGIC WAND", 0, 0, 0, 0, 0 }, 13, 0, 0 },
};
#define NTHEME ((int)(sizeof themes / sizeof themes[0]))
static int wear_find(const char *const *names, int n, const char *want) { if (!want) return 0; for (int i = 0; i < n; i++) if (!strcmp(names[i], want)) return i; return 0; }
void look_theme(Bore *b, int t) {   // a ready-made outfit: every place on the body set at once (an empty place takes off what was there)
    if (t <= 0 || t >= NTHEME) return;
    const Theme *th = &themes[t];
    static const u8 ids[10] = { LK_X_HEAD, LK_X_FACE, LK_X_NECK, LK_X_BACK, LK_X_HAND, LK_X_WAIST, LK_X_EARS, LK_X_WRIST, LK_X_SHOULDER, LK_X_FEET };
    const char *const *nm[10] = { xHeadNm, xFaceNm, xNeckNm, xBackNm, xHandNm, xWaistNm, xEarsNm, xWristNm, xShoulderNm, xFeetNm };
    const int cnt[10] = { NX_HEAD, NX_FACE, NX_NECK, NX_BACK, NX_HAND, NX_WAIST, NX_EARS, NX_WRIST, NX_SHOULDER, NX_FEET };
    for (int i = 0; i < 10; i++) b->look[ids[i]] = (u8)wear_find(nm[i], cnt[i], th->w[i]);
    if (t >= 2) { b->look[LK_X_COLA] = th->ca % NX_COL; b->look[LK_X_COLB] = th->cb % NX_COL; b->look[LK_X_TOPPAT] = th->pat % NX_PAT; }
}

const char *const look_tab_name[NTAB] = { "BODY", "FACE", "HAIR", "CLOTHES", "WEAR", "PARTS", "ASPIRE" };
static const LookRow rowBody[] = {
    {"AGE", LK_AGE, ROW_AGE}, {"SHAPE", LK_SHAPE, ROW_PICK}, {"SKIN", LK_SKIN, ROW_COLOUR}, {"SKIN TONE", LK_TONE, ROW_SLIDER},
    {"HEIGHT", LK_HEIGHT, ROW_SLIDER}, {"WEIGHT", LK_WEIGHT, ROW_SLIDER}, {"TORSO", LK_TORSO, ROW_SLIDER}, {"ARMS", LK_ARMS, ROW_SLIDER},
    {"STANCE", LK_STANCE, ROW_SLIDER}, {"BUTT", LK_BUTT, ROW_SLIDER},
    {"NECK", LK_NECK, ROW_SLIDER}, {"NECK WIDTH", LK_NECKW, ROW_SLIDER}, {"SHOULDERS", LK_SHOULW, ROW_SLIDER}, {"CHEST", LK_CHESTW, ROW_SLIDER},
    {"BELLY", LK_BELLYW, ROW_SLIDER}, {"WAIST", LK_WAISTW, ROW_SLIDER}, {"HIPS", LK_HIPW, ROW_SLIDER}, {"LEGS", LK_LEGW, ROW_SLIDER},
    {"THIGHS", LK_THIGHW, ROW_SLIDER}, {"CALVES", LK_CALFW, ROW_SLIDER}, {"FEET", LK_FOOTSZ, ROW_SLIDER}, {"ARM WIDTH", LK_ARMW, ROW_SLIDER},
    {"UPPER ARMS", LK_UARMW, ROW_SLIDER}, {"FOREARMS", LK_FARMW, ROW_SLIDER}, {"HANDS", LK_HANDSZ, ROW_SLIDER} };
static const LookRow rowFace[] = {
    {"EYES", LK_EYES, ROW_PICK}, {"EYE COLOUR", LK_EYECOL, ROW_COLOUR}, {"EYE SIZE", LK_EYESZ, ROW_SLIDER}, {"EYE SPACING", LK_EYESP, ROW_SLIDER},
    {"EYE HEIGHT", LK_EYEHT, ROW_SLIDER}, {"BROWS", LK_BROW, ROW_PICK}, {"GLASSES", LK_GLASS, ROW_PICK}, {"MOUTH", LK_MOUTH, ROW_PICK},
    {"MOUTH WIDTH", LK_MOUTHW, ROW_SLIDER}, {"MOUTH HEIGHT", LK_MOUTHHT, ROW_SLIDER}, {"NOSE", LK_NOSE, ROW_PICK}, {"CHEEKS", LK_CHEEK, ROW_PICK},
    {"EARS", LK_EARS, ROW_PICK}, {"MAKE-UP", LK_X_MAKEUP, ROW_PICK}, {"LIPS", LK_X_LIPS, ROW_PICK}, {"MORE MARKS", LK_X_MARKS, ROW_PICK}, {"EYE SHADE", LK_EYETONE, ROW_SLIDER}, {"JAW", LK_JAWW, ROW_SLIDER}, {"EAR LIFT", LK_EARLF, ROW_SLIDER}, {"EAR WIDTH", LK_EARWID, ROW_SLIDER} };
static const LookRow rowHair[] = {
    {"STYLE", LK_HSTYLE, ROW_PICK}, {"COLOUR", LK_HCOL, ROW_COLOUR}, {"HAIR TONE", LK_HTONE, ROW_SLIDER}, {"BEARD", LK_BEARD, ROW_PICK},
    {"HAT", LK_HAT, ROW_PICK}, {"HAT COLOUR", LK_HATCOL, ROW_PICK} };
static const LookRow rowClothes[] = {
    {"TOP", LK_TOP, ROW_COLOUR}, {"TOP TONE", LK_TTONE, ROW_SLIDER}, {"TOP STYLE", LK_TOPSTY, ROW_PICK}, {"BOTTOM", LK_BOT, ROW_COLOUR},
    {"BOTTOM TONE", LK_BTONE, ROW_SLIDER}, {"BOTTOM STYLE", LK_BOTSTY, ROW_PICK}, {"SHOES", LK_SHOE, ROW_PICK},
    {"TOP PATTERN", LK_X_TOPPAT, ROW_PICK}, {"BOTTOM PATTERN", LK_X_BOTPAT, ROW_PICK}, {"PATTERN SHADE", LK_X_PATTONE, ROW_PICK} };
static const LookRow rowWear[] = {
    {"OUTFIT", LK_X_THEME, ROW_PICK}, {"HEAD", LK_X_HEAD, ROW_PICK}, {"FACE", LK_X_FACE, ROW_PICK}, {"NECK", LK_X_NECK, ROW_PICK}, {"BACK", LK_X_BACK, ROW_PICK},
    {"HOLDING", LK_X_HAND, ROW_PICK}, {"WAIST", LK_X_WAIST, ROW_PICK}, {"EARS", LK_X_EARS, ROW_PICK}, {"WRISTS", LK_X_WRIST, ROW_PICK},
    {"SHOULDER", LK_X_SHOULDER, ROW_PICK}, {"FEET", LK_X_FEET, ROW_PICK}, {"COLOUR 1", LK_X_COLA, ROW_COLOUR}, {"COLOUR 2", LK_X_COLB, ROW_COLOUR} };
static const LookRow rowParts[] = {
    {"TAIL", LK_TAIL, ROW_PICK}, {"HORNS", LK_HORNS, ROW_PICK}, {"BACK", LK_BACK, ROW_PICK}, {"HANDS", LK_CLAWS, ROW_PICK},
    {"ANTENNAE", LK_ANTENNA, ROW_PICK}, {"PATTERN", LK_PATTERN, ROW_PICK}, {"PAINT", LK_PATCOL, ROW_PICK},
    {"ANIMAL EARS", LK_FEARS, ROW_PICK}, {"MUZZLE", LK_MUZZLE, ROW_PICK}, {"FUR TAIL", LK_FTAIL, ROW_PICK},
    {"TAIL LENGTH", LK_TAILLEN, ROW_SLIDER}, {"HORN SIZE", LK_HORNSZ, ROW_SLIDER}, {"WING SIZE", LK_WINGSZ, ROW_SLIDER},
    {"ANTENNA LENGTH", LK_ANTLEN, ROW_SLIDER}, {"ANTENNA SPREAD", LK_ANTSPR, ROW_SLIDER} };
static const LookRow rowAspire[] = {
    {"ASPIRATION", LK_ASP, ROW_PERSONA}, {"LIFETIME", LK_LTW, ROW_PERSONA},
    {"NEAT", LK_TRAIT0 + 0, ROW_PERSONA}, {"OUTGOING", LK_TRAIT0 + 1, ROW_PERSONA}, {"ACTIVE", LK_TRAIT0 + 2, ROW_PERSONA},
    {"PLAYFUL", LK_TRAIT0 + 3, ROW_PERSONA}, {"NICE", LK_TRAIT0 + 4, ROW_PERSONA} };
int look_tab_rows(int tab, const LookRow **rows) {
    switch (tab) {
        case 0: *rows = rowBody; return sizeof(rowBody) / sizeof(LookRow);
        case 1: *rows = rowFace; return sizeof(rowFace) / sizeof(LookRow);
        case 2: *rows = rowHair; return sizeof(rowHair) / sizeof(LookRow);
        case 3: *rows = rowClothes; return sizeof(rowClothes) / sizeof(LookRow);
        case 4: *rows = rowWear; return sizeof(rowWear) / sizeof(LookRow);
        case 5: *rows = rowParts; return sizeof(rowParts) / sizeof(LookRow);
        default: *rows = rowAspire; return sizeof(rowAspire) / sizeof(LookRow);
    }
}

// ---- random ----
static u32 rng_state = 0x1234567;
u32 look_rand(void) { rng_state = rng_state * 1103515245u + 12345u; return rng_state >> 8; }
static int rnd8(void) { return (int)(look_rand() & 255); }
u32 look_seed(u32 s) { u32 old = rng_state; rng_state = s; return old; }   // reseed the dice (returns the old state, so a caller can put it back)

static void wp_avg_init(void) {
    for (int i = 0; i < NWP; i++) {
        const Tex *t = &wpTex[i]; int r = 0, g = 0, b = 0;
        for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) { u16 c = t->c[t->p[y][x] - '0']; r += C_R(c); g += C_G(c); b += C_B(c); }
        wpAvg[i] = RGB(r / 64, g / 64, b / 64);
    }
}
const Tex *look_wp_tex(int i) { return &wpTex[i]; }
const Tex *look_fl_tex(int i) { return &flTex[i]; }
u16 look_wp_avg(int i) { return wpAvg[i]; }

void look_init(void) { wp_avg_init(); }

int look_is_slider(int id) { return id < LK_N && lkSlide(id); }
int look_count(int id) {
    if (id < LK_N) return lk_cnt(id);
    switch (id) {
        case LK_X_HEAD: return NX_HEAD; case LK_X_FACE: return NX_FACE; case LK_X_NECK: return NX_NECK; case LK_X_BACK: return NX_BACK;
        case LK_X_HAND: return NX_HAND; case LK_X_WAIST: return NX_WAIST; case LK_X_COLA: case LK_X_COLB: return NX_COL;
        case LK_X_THEME: return NTHEME;
        case LK_X_MAKEUP: return NMAKEUP; case LK_X_LIPS: return NLIPS; case LK_X_MARKS: return NCHEEK_ALL;
        case LK_X_EARS: return NX_EARS; case LK_X_WRIST: return NX_WRIST; case LK_X_SHOULDER: return NX_SHOULDER; case LK_X_FEET: return NX_FEET;
        case LK_X_TOPPAT: case LK_X_BOTPAT: return NX_PAT; case LK_X_PATTONE: return 2;
        default: return 0;
    }
}

int look_allowed(const Bore *b, int id, int v) {
    stage = b->stage;
    switch (id) {
        case LK_SHAPE: return shapeMask() >> v & 1;
        case LK_EARS:  return v < 3 ? stMaskEars[stage] >> v & 1 : stMaskEars[stage] >> 2 & 1;   // (the new ears: whoever may have big ones)
        case LK_HSTYLE: return stMaskHair[stage] >> v & 1;
        case LK_SKIN: return v < NSKIN;
        case LK_HCOL: case LK_TOP: case LK_BOT: return v < stSwatches[stage];
        case LK_TOPSTY: return v < 4 || stage >= AG_ADULT;
        case LK_BOTSTY: return v < 3 || stage >= AG_ADULT;
        case LK_BEARD: return v == 0 || stage >= AG_ADULT;
        default: return 1;
    }
}

void look_fix(Bore *b) {   // a birthday: whatever the new age cannot wear goes back to the first thing it can
    for (int id = 0; id < LK_COUNT; id++) {
        if (look_allowed(b, id, b->look[id])) continue;
        int c = look_count(id); for (int v = 0; v < c; v++) if (look_allowed(b, id, v)) { b->look[id] = (u8)v; break; }
    }
}
const char *look_option_name(int id, int v) {
    switch (id) {
        case LK_SHAPE: return shapeNm[v % NSHAPE];
        case LK_EYES: return eyeNmAll[v % NEYE_ALL];
        case LK_MOUTH: return mouthNmAll[v % NMOUTH_ALL];
        case LK_EARS: return earNm[v % NEAR_ALL];
        case LK_HSTYLE: return hairNm[v % NHAIR];
        case LK_TAIL: return tailNm[v % 3];
        case LK_HORNS: return hornNm[v % 3];
        case LK_BACK: return backNm[v % 3];
        case LK_HAT: return hatNm[v % 6];
        case LK_HATCOL: return hatColNm[v % 6];
        case LK_BEARD: return beardNm[v % 3];
        case LK_TOPSTY: return topStyNm[v % 5];
        case LK_BOTSTY: return botStyNm[v % 4];
        case LK_SHOE: return shoeNm[v % 6];
        case LK_BROW: return browNmAll[v % NBROW_ALL];
        case LK_NOSE: return noseNmAll[v % NNOSE_ALL];
        case LK_CHEEK: case LK_X_MARKS: return cheekNmAll[v % NCHEEK_ALL];
        case LK_GLASS: return glassNmAll[v % NGLASS_ALL];
        case LK_X_MAKEUP: return makeupNm[v % NMAKEUP];
        case LK_X_LIPS: return lipsNm[v % NLIPS];
        case LK_CLAWS: return clawNm[v % 4];
        case LK_ANTENNA: return antNm[v % 3];
        case LK_PATTERN: return patNm[v % 7];
        case LK_PATCOL: return patColNm[v % 6];
        case LK_FEARS: return fearNm[v % 5];
        case LK_MUZZLE: return muzNm[v % 4];
        case LK_FTAIL: return ftailNm[v % 4];
        case LK_AGE: return stageNm2[v % AG_N];
        case LK_X_HEAD: return xHeadNm[v % NX_HEAD];
        case LK_X_FACE: return xFaceNm[v % NX_FACE];
        case LK_X_NECK: return xNeckNm[v % NX_NECK];
        case LK_X_BACK: return xBackNm[v % NX_BACK];
        case LK_X_HAND: return xHandNm[v % NX_HAND];
        case LK_X_WAIST: return xWaistNm[v % NX_WAIST];
        case LK_X_THEME: return themes[v % NTHEME].nm;
        case LK_X_EARS: return xEarsNm[v % NX_EARS];
        case LK_X_WRIST: return xWristNm[v % NX_WRIST];
        case LK_X_SHOULDER: return xShoulderNm[v % NX_SHOULDER];
        case LK_X_FEET: return xFeetNm[v % NX_FEET];
        case LK_X_TOPPAT: case LK_X_BOTPAT: return xPatNm[v % NX_PAT];
        case LK_X_PATTONE: return v ? "LIGHTER" : "DARKER";
        case LK_ASP: return aspNm[v % AS_PICK];
        default: return 0;
    }
}
u16 look_option_colour(int id, int v) {
    switch (id) {
        case LK_SKIN: return skinTones[v % NSKIN];
        case LK_HCOL: return hairTones[v % NSW];
        case LK_TOP: return topTones[v % NSW];
        case LK_BOT: return botTones[v % NSW];
        case LK_EYECOL: return eyeTones[v % NSW];
        case LK_X_COLA: case LK_X_COLB: return xCol[v % NX_COL];
        default: return 0;
    }
}
const char *look_stage_name(int s) { return stageNm2[s % AG_N]; }
const char *look_shape_name(int v) { return shapeNm[v % NSHAPE]; }
const char *look_asp_name(int a) { return aspNm[a % AS_N]; }
const char *look_trait_name(int t) { return trNm[t % TR_N]; }
const char *look_sign_name(int s) { return signNm[s % 12]; }
int look_sign_of(const Bore *b) { for (int i = 0; i < TR_N; i++) pTr[i] = b->tr[i]; return signOf(); }
void look_set_sign(Bore *b, int s) { for (int i = 0; i < TR_N; i++) b->tr[i] = signTr[s % 12][i]; }

void look_default(Bore *b) {
    memset(b, 0, sizeof *b);                  // (the face texture slots too: a fresh person gets its own)
    b->stage = AG_ADULT;
    b->asp = AS_FORTUNE; b->ltw = 0; look_set_sign(b, 0);
    strcpy(b->first, "BORE"); strcpy(b->last, "TWO");
    look_build(b);
}

static int isPart(int id) { return id == LK_TAIL || id == LK_HORNS || id == LK_BACK || id == LK_CLAWS || id == LK_ANTENNA; }
static const char *const firstNm[16] = { "DIXIE", "MISSY", "GOTTCHO", "SUNMAN", "WHISTLER", "COLA", "NOVA", "PIXEL", "JUNO", "BARRY", "TREE", "MARS", "CLOUD", "BOMB", "ACE", "DOT" };
static const char *const lastNm[16] = { "BARRACHO", "DIPPER", "MIDNIGHT", "FLEXICODE", "SYNDICATE", "CONDENSED", "SPACE", "CITIZEN", "STAGED", "WINNER", "HOTDAMN", "EXCUSES", "MAGIC", "NURSERY", "CLOSER", "ZONE" };

const char *look_last_name(int i) { return lastNm[i & 15]; }   // so a household can share a surname

void look_random(Bore *b) {
    stage = b->stage;
    for (int id = 0; id < LK_N; id++) {
        if (lkSlide(id)) { b->look[id] = (u8)slideVal(rnd8() % 5 + rnd8() % 5); continue; }
        int v = 0;
        for (int t = 0; t < 20; t++) {
            v = rnd8() % lk_cnt(id);
            if (id == LK_EARS && v >= 3 && (rnd8() & 3)) v = 1 + (rnd8() & 1);   // (elf ears and the like now and then)
            if (isPart(id) && (rnd8() & 1)) v = 0;
            if ((id == LK_PATTERN || id == LK_FEARS || id == LK_MUZZLE || id == LK_FTAIL) && (rnd8() % 3)) v = 0;
            if ((id == LK_HAT || id == LK_BEARD || id == LK_GLASS || id == LK_CHEEK) && (rnd8() & 1)) v = 0;
            if (id == LK_BEARD && stage < AG_ADULT) v = 0;
            if (id == LK_TOPSTY && v == 4) v = 0;
            if (id == LK_BOTSTY && v == 3) v = 0;
            if (look_allowed(b, id, v)) break;
            v = 0;
        }
        b->look[id] = (u8)v;
    }
    {   // the wardrobe: about one accessory in three, patterns now and then
        static const u8 ids[6] = { LK_X_HEAD, LK_X_FACE, LK_X_NECK, LK_X_BACK, LK_X_HAND, LK_X_WAIST };
        for (int i = 0; i < 6; i++) { int cnt = look_count(ids[i]); b->look[ids[i]] = (u8)((rnd8() % 3 == 0) ? 1 + rnd8() % (cnt - 1) : 0); }
        b->look[LK_X_COLA] = (u8)(rnd8() % NX_COL); b->look[LK_X_COLB] = (u8)(rnd8() % NX_COL);
        b->look[LK_X_TOPPAT] = (u8)((rnd8() % 3 == 0) ? 1 + rnd8() % (NX_PAT - 1) : 0);
        b->look[LK_X_BOTPAT] = (u8)((rnd8() % 4 == 0) ? 1 + rnd8() % (NX_PAT - 1) : 0);
        b->look[LK_X_PATTONE] = (u8)(rnd8() & 1);
        b->look[LK_X_MAKEUP] = (u8)((rnd8() % 5 == 0) ? 1 + rnd8() % (NMAKEUP - 1) : 0);
        b->look[LK_X_LIPS] = (u8)((rnd8() % 5 == 0) ? 1 + rnd8() % (NLIPS - 1) : 0);
        b->look[LK_X_MARKS] = (u8)((rnd8() % 6 == 0) ? 1 + rnd8() % (NCHEEK_ALL - 1) : 0);
    }
    look_set_sign(b, rnd8() % 12);
    b->asp = (u8)(rnd8() % AS_PICK); b->ltw = (u8)(rnd8() % LTN);
    strcpy(b->first, firstNm[rnd8() & 15]); strcpy(b->last, lastNm[rnd8() & 15]);
    {   // (the big wardrobe's new places: now and then earrings, a wrist thing, a shoulder pet, something on the feet)
        static const u8 ids[4] = { LK_X_EARS, LK_X_WRIST, LK_X_SHOULDER, LK_X_FEET }; static const u8 odds[4] = { 3, 4, 9, 8 };
        for (int i = 0; i < 4; i++) { int cnt = look_count(ids[i]); b->look[ids[i]] = (u8)((rnd8() % odds[i] == 0) ? 1 + rnd8() % (cnt - 1) : 0); }
    }
    look_build(b);
}

// ---- face art -> textures ----
static u16 facebuf[19 * 8];
// the face textures of the household's Bores who have gone: the next new face takes one (the atlas has no free, so nothing is lost)
static TexRef face_free[8]; static int face_nfree;
void look_release(Bore *b) {
    TexRef *t[2] = { &b->tex_eye, &b->tex_mouth };
    for (int k = 0; k < 2; k++) { if (t[k]->tpage && face_nfree < 8) face_free[face_nfree++] = *t[k]; t[k]->tpage = 0; }
}
void look_own_face(Bore *b) { b->tex_eye.tpage = 0; b->tex_mouth.tpage = 0; look_build(b); }
static TexRef face_alloc(int w) {
    for (int i = 0; i < face_nfree; i++) if (face_free[i].cap >= w) { TexRef t = face_free[i]; face_free[i] = face_free[--face_nfree]; t.w = (u8)w; return t; }
    return atlas_alloc(w, 8);
}
static const u16 lipTones[NLIPS] = { 0, RGB(24,3,5), RGB(30,12,20), RGB(31,14,10), RGB(18,3,11), RGB(16,5,22), RGB(3,3,4), RGB(28,22,6), RGB(6,10,26), RGB(24,15,12), RGB(13,7,4) };
static const u16 shadowTones[NMAKEUP] = { 0, 0, 0, 0, RGB(8,16,30), RGB(19,9,26), RGB(29,23,8), RGB(30,14,22), RGB(8,22,10), RGB(9,8,10), RGB(26,22,30) };
static void bake_face(Bore *b, int eye) {
    const char *const *art = eye ? eyeArtAll[b->sty_eye % NEYE_ALL] : mouthArtAll[b->sty_mouth % NMOUTH_ALL];
    int aw = eye ? 9 : 19;
    const Bore *lb = b;
    int br = lb->look[LK_BROW] % NBROW_ALL, gl = lb->look[LK_GLASS] % NGLASS_ALL, no = lb->look[LK_NOSE] % NNOSE_ALL, ch = lb->look[LK_CHEEK] % NCHEEK_ALL;
    int mk = lb->look[LK_X_MAKEUP] % NMAKEUP, lp = lb->look[LK_X_LIPS] % NLIPS, mr = lb->look[LK_X_MARKS] % NCHEEK_ALL;
    static const char key[] = FACE_KEYS;
    char g[8][20];
    for (int y = 0; y < 8; y++) for (int x = 0; x < aw; x++) {
        char c = art[y][x];
        if (eye) {
            if (br && y < 2) { char t = browArtAll[br][y][x]; if (t != '.') c = t; }
        } else if (c == '.') {
            if (no && y < 2) c = noseArtAll[no][y][x];
            if (c == '.' && ch && y < 4) c = cheekArtAll[ch][y][x];
            if (c == '.' && mr && y < 4) c = cheekArtAll[mr][y][x];
        } else if (lp && c == 'k') c = 'p';   // LIPS: the mouth's lines in the lip colour
        g[y][x] = c;
    }
    if (eye && mk) {   // MAKE-UP, fitted to whichever eye: shadow on the lid above it, a line along its top, lashes, a wing at the outer corner
        int top = -1, x0 = 9, x1 = -1;
        for (int y = 0; y < 8 && top < 0; y++) for (int x = 0; x < 9; x++) if (art[y][x] != '.') { top = y; if (x < x0) x0 = x; if (x > x1) x1 = x; }
        if (top >= 0) {
            int lr = top - 1;
            if (mk == 1 || mk == 2) { if (lr >= 0) for (int x = x0; x <= x1; x++) if (g[lr][x] == '.') g[lr][x] = 'k';
                                      if (mk == 2) { if (x0 > 0 && top >= 0) g[top][x0 - 1] = 'k'; if (x0 > 1 && lr >= 0) g[lr][x0 - 2] = 'k'; if (x0 > 0 && lr >= 0) g[lr][x0 - 1] = 'k'; } }
            else if (mk == 3) { for (int x = x0; x <= x1; x += 2) { if (lr >= 0 && g[lr][x] == '.') g[lr][x] = 'k'; } if (lr >= 1 && x0 > 0) g[lr - 1][x0 - 1] = 'k'; if (x0 > 0) g[top][x0 - 1] = 'k'; }
            else for (int y = MAX(0, top - 2); y < top; y++) for (int x = MAX(0, x0 - 1); x <= MIN(8, x1 + 1); x++)
                if (g[y][x] == '.') g[y][x] = (mk == 10 && ((x + y) & 1)) ? 'y' : 'e';
        }
    }
    if (eye && gl) for (int y = 0; y < 8; y++) for (int x = 0; x < 9; x++) { char t = glassArtAll[gl][y][x]; if (t != '.') g[y][x] = t; }
    u16 pal[17]; memcpy(pal, b->facepal, sizeof pal);
    if (lp) pal[10] = lipTones[lp];
    if (mk >= 4) pal[13] = shadowTones[mk];
    for (int y = 0; y < 8; y++) for (int x = 0; x < aw; x++) {
        char c = g[y][x];
        u16 px = 0;
        if (c != '.') { int k = 0; while (key[k] && key[k] != c) k++; if (!key[k]) k = 0; px = pal[k]; if (!px) px = 0x8000; }
        facebuf[y * aw + x] = px;
    }
    TexRef *t = eye ? &b->tex_eye : &b->tex_mouth;
    if (!t->tpage) *t = face_alloc(aw);
    else if (t->w != aw) { if (aw > t->cap) *t = face_alloc(aw); else t->w = (u8)aw; }   // a slot keeps its place when the new art fits (the crowd's face pool)
    atlas_upload(t, facebuf);
}

void look_build(Bore *b) {
    memcpy(look, b->look, LK_N);
    stage = b->stage;
    custom = 0;
    int eyes = look[LK_EYES] % NEYE_ALL, mouth = look[LK_MOUTH] % NMOUTH_ALL, ears = look[LK_EARS] % NEAR_ALL;   // (the GBA builder knows its own nine and three: the rest go round it)
    if (eyes >= NEYE) look[LK_EYES] = 0;
    if (mouth >= NMOUTH) look[LK_MOUTH] = 0;
    if (ears >= 3) look[LK_EARS] = 2;
    fixLook();
    buildLook();
    look[LK_EYES] = (u8)eyes; look[LK_MOUTH] = (u8)mouth;
    if (ears >= 3) look[LK_EARS] = look[LK_EARS] == 2 ? (u8)ears : look[LK_EARS];
    setColors();
    memcpy(b->look, look, LK_N);          // fixLook may have pulled a choice into what the stage allows
    memcpy(b->vox, vox, sizeof vox);
    memcpy(b->dec, dec, sizeof dec);
    for (int i = 0; i < 9; i++) b->col[i] = base[i];
    b->sty_eye = (u8)eyes; b->sty_mouth = (u8)mouth;
    { int L, T, hs; bodyPlan(&L, &T, &hs); b->legs = (u8)L; b->torso = (u8)T; b->headsz = (u8)hs; }
    b->bx0 = (u8)BX0; b->bxw = (u8)BXW; b->bxh = (u8)BXH; b->bxd = (u8)BXD;
    // the face palette (k dark, w white, r red, s lid shadow, i iris, b blush, g glasses frame, h brows, l dark lenses), as setColors builds it
    u16 sk = base[1], bl = RGB(((sk & 31) + 31) / 2, (((sk >> 5) & 31) + 8) / 2, (((sk >> 10) & 31) + 12) / 2);
    b->facepal[0] = base[3]; b->facepal[1] = base[2]; b->facepal[2] = base[4]; b->facepal[3] = shade(base[1], 11);
    b->facepal[4] = toneBy(eyeTones[look[LK_EYECOL] % NSW], slideEffS(look[LK_EYETONE])); b->facepal[5] = bl; b->facepal[6] = RGB(6, 6, 8);
    b->facepal[7] = shade(base[5], 10); b->facepal[8] = RGB(4, 5, 9);
    b->facepal[9] = RGB(30, 24, 6); b->facepal[10] = RGB(24, 3, 5); b->facepal[11] = RGB(6, 14, 30); b->facepal[12] = shade(base[1], 6);   // y gold, p lips, c paint, n a mole
    b->facepal[13] = RGB(9, 8, 10); b->facepal[14] = RGB(14, 24, 31); b->facepal[15] = RGB(22, 23, 25); b->facepal[16] = RGB(28, 22, 15);   // e shadow, t a tear, m metal, a a plaster
    bake_face(b, 1);
    bake_face(b, 0);
    voxel_prepare(b);
    b->baked = 1;
}
int look_theme_named(const char *nm) { for (int i = 1; i < NTHEME; i++) if (!strcmp(themes[i].nm, nm)) return i; return 0; }
