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

static int slideEffS(int v) { return slidePos(v) - 4; }

// ---- names the creator shows ----
static const char *const shapeNm[NSHAPE]={"AVERAGE","BROAD","BIG HEAD","STUBBY","SLIM","ATHLETIC","TALL","CHUBBY","PEAR","LANKY","STOCKY","HUNCHED","POTBELLY","MUSCLE","PETITE","BARREL","DIGITIGRADE","V-SHAPE","CURVY","RUNNER","SOFT","POWER","LONG LEGS","PUDGY","TODDLER","SPROUT","SPORTY","BELL","STURDY","SPIDER"};
static const char *const eyeNm[NEYE]={"SLEEPY","ROUND","HAPPY","WIDE","ANGRY","CUTE","CAT","DOT","LASHES"};
static const char *const mouthNm[NMOUTH]={"FLAT","SMILE","OH","GRIN","SMIRK","FROWN","TONGUE","FANGS","KITTY"};
static const char *const browNm[6]={"NONE","THIN","THICK","ANGRY","WORRIED","UNIBROW"};
static const char *const noseNm[6]={"NONE","BUTTON","POINTY","WIDE","PIG","ANIMAL"};
static const char *const fearNm[5]={"NONE","CAT","FOX","BUNNY","BEAR"};
static const char *const muzNm[4]={"NONE","SNOUT","MUZZLE","BEAK"};
static const char *const ftailNm[4]={"NONE","FOX","CAT","BUNNY"};
static const char *const cheekNm[5]={"NONE","BLUSH","FRECKLES","WHISKERS","SCAR"};
static const char *const glassNm[4]={"NONE","ROUND","SQUARE","SHADES"};
static const char *const earNm[3]={"NONE","SMALL","BIG"};
static const char *const hairNm[NHAIR]={"CROP","BOWL","LONG","BALD","SPIKY","AFRO","FLAT TOP","SIDE TAIL","BUN","BOB","PONYTAIL","PIGTAILS","MOHAWK","PIXIE","CURLS"};
static const char *const hatNm[6]={"NONE","CAP","BEANIE","BAND","FEZ","HELMET"};
static const char *const hatColNm[6]={"AS THE TOP","AS THE BOTTOM","WHITE","BLACK","RED","GOLD"};
static const char *const beardNm[3]={"NONE","BEARD","LONG BEARD"};
static const char *const topStyNm[5]={"TEE","LONG SLEEVE","TANK","HOODIE","BARE"};
static const char *const botStyNm[4]={"PANTS","SHORTS","SKIRT","BARE"};
static const char *const shoeNm[6]={"AS THE BOTTOM","WHITE","BLACK","RED","GOLD","AS THE TOP"};
static const char *const stageNm2[AG_N]={"BABY","CHILD","TEEN","ADULT","ELDER"};
static const u8 lkCnt[LK_N]={NSHAPE,NSKIN,NEYE,NMOUTH,3,NHAIR,NSW,NSW,NSW,9,9,9,3,3,3,6,6,3,4,3,6, 6,6,5,4,NSW, 9,9,9,9,9,9,9, 4,3,7,6, 9,9,9,9,9,9,9,9,9, 5,4,4, 9,9,9, 9,9,9,9,9,9,9, 9,9,9,9,9,9,9, 9,9,9,7, 9,9,9, 9};
_Static_assert(LK_X_END <= LK_COUNT, "LK_COUNT in look.h must hold every look");
#include "wear_names.inc"

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
    {"EARS", LK_EARS, ROW_PICK}, {"EYE SHADE", LK_EYETONE, ROW_SLIDER}, {"JAW", LK_JAWW, ROW_SLIDER}, {"EAR LIFT", LK_EARLF, ROW_SLIDER}, {"EAR WIDTH", LK_EARWID, ROW_SLIDER} };
static const LookRow rowHair[] = {
    {"STYLE", LK_HSTYLE, ROW_PICK}, {"COLOUR", LK_HCOL, ROW_COLOUR}, {"HAIR TONE", LK_HTONE, ROW_SLIDER}, {"BEARD", LK_BEARD, ROW_PICK},
    {"HAT", LK_HAT, ROW_PICK}, {"HAT COLOUR", LK_HATCOL, ROW_PICK} };
static const LookRow rowClothes[] = {
    {"TOP", LK_TOP, ROW_COLOUR}, {"TOP TONE", LK_TTONE, ROW_SLIDER}, {"TOP STYLE", LK_TOPSTY, ROW_PICK}, {"BOTTOM", LK_BOT, ROW_COLOUR},
    {"BOTTOM TONE", LK_BTONE, ROW_SLIDER}, {"BOTTOM STYLE", LK_BOTSTY, ROW_PICK}, {"SHOES", LK_SHOE, ROW_PICK},
    {"TOP PATTERN", LK_X_TOPPAT, ROW_PICK}, {"BOTTOM PATTERN", LK_X_BOTPAT, ROW_PICK}, {"PATTERN SHADE", LK_X_PATTONE, ROW_PICK} };
static const LookRow rowWear[] = {
    {"HEAD", LK_X_HEAD, ROW_PICK}, {"FACE", LK_X_FACE, ROW_PICK}, {"NECK", LK_X_NECK, ROW_PICK}, {"BACK", LK_X_BACK, ROW_PICK},
    {"HOLDING", LK_X_HAND, ROW_PICK}, {"WAIST", LK_X_WAIST, ROW_PICK}, {"COLOUR 1", LK_X_COLA, ROW_COLOUR}, {"COLOUR 2", LK_X_COLB, ROW_COLOUR} };
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
    if (id < LK_N) return lkCnt[id];
    switch (id) {
        case LK_X_HEAD: return NX_HEAD; case LK_X_FACE: return NX_FACE; case LK_X_NECK: return NX_NECK; case LK_X_BACK: return NX_BACK;
        case LK_X_HAND: return NX_HAND; case LK_X_WAIST: return NX_WAIST; case LK_X_COLA: case LK_X_COLB: return NX_COL;
        case LK_X_TOPPAT: case LK_X_BOTPAT: return NX_PAT; case LK_X_PATTONE: return 2;
        default: return 0;
    }
}

int look_allowed(const Bore *b, int id, int v) {
    stage = b->stage;
    switch (id) {
        case LK_SHAPE: return shapeMask() >> v & 1;
        case LK_EARS:  return stMaskEars[stage] >> v & 1;
        case LK_HSTYLE: return stMaskHair[stage] >> v & 1;
        case LK_SKIN: return v < NSKIN;
        case LK_HCOL: case LK_TOP: case LK_BOT: return v < stSwatches[stage];
        case LK_TOPSTY: return v < 4 || stage >= AG_ADULT;
        case LK_BOTSTY: return v < 3 || stage >= AG_ADULT;
        case LK_BEARD: return v == 0 || stage >= AG_ADULT;
        default: return 1;
    }
}

const char *look_option_name(int id, int v) {
    switch (id) {
        case LK_SHAPE: return shapeNm[v % NSHAPE];
        case LK_EYES: return eyeNm[v % NEYE];
        case LK_MOUTH: return mouthNm[v % NMOUTH];
        case LK_EARS: return earNm[v % 3];
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
        case LK_BROW: return browNm[v % 6];
        case LK_NOSE: return noseNm[v % 6];
        case LK_CHEEK: return cheekNm[v % 5];
        case LK_GLASS: return glassNm[v % 4];
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
static const char *const firstNm[16] = { "DIPPA", "MISSY", "GOTTCHO", "SUNMAN", "WHISTLER", "COLA", "NOVA", "PIXEL", "JUNO", "BARRY", "TREE", "MARS", "CLOUD", "BOMB", "ACE", "DOT" };
static const char *const lastNm[16] = { "BARRACHO", "DIPPER", "MIDNIGHT", "FLEXICODE", "SYNDICATE", "CONDENSED", "SPACE", "CITIZEN", "STAGED", "WINNER", "HOTDAMN", "EXCUSES", "MAGIC", "NURSERY", "CLOSER", "ZONE" };

const char *look_last_name(int i) { return lastNm[i & 15]; }   // so a household can share a surname

void look_random(Bore *b) {
    stage = b->stage;
    for (int id = 0; id < LK_N; id++) {
        if (lkSlide(id)) { b->look[id] = (u8)slideVal(rnd8() % 5 + rnd8() % 5); continue; }
        int v = 0;
        for (int t = 0; t < 20; t++) {
            v = rnd8() % lkCnt[id];
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
    }
    look_set_sign(b, rnd8() % 12);
    b->asp = (u8)(rnd8() % AS_PICK); b->ltw = (u8)(rnd8() % LTN);
    strcpy(b->first, firstNm[rnd8() & 15]); strcpy(b->last, lastNm[rnd8() & 15]);
    look_build(b);
}

// ---- face art -> textures ----
static u16 facebuf[19 * 8];
static void bake_face(Bore *b, int eye) {
    const Spr *sp = &spr[eye ? b->sty_eye : NEYE + b->sty_mouth];
    int aw = 10 * sp->wc - 1;
    int br = look[LK_BROW], gl = look[LK_GLASS], no = look[LK_NOSE], ch = look[LK_CHEEK];
    static const char key[] = "kwrsibghl";
    for (int y = 0; y < 8; y++) for (int x = 0; x < aw; x++) {
        char c = sp->art[y][x];
        if (eye) {
            if (br && y < 2) { char t = brArt[br - 1][y][x]; if (t != '.') c = t; }
            if (gl) { char t = glArt[gl - 1][y][x]; if (t != '.') c = t; }
        } else if (c == '.') {
            if (no && y < 2) c = noArt[no - 1][y][x];
            if (c == '.' && ch && y < 4) c = chArt[ch - 1][y][x];
        }
        u16 px = 0;
        if (c != '.') { int k = 0; while (key[k] && key[k] != c) k++; if (!key[k]) k = 0; px = b->facepal[k]; if (!px) px = 0x8000; }
        facebuf[y * aw + x] = px;
    }
    TexRef *t = eye ? &b->tex_eye : &b->tex_mouth;
    if (!t->tpage) *t = atlas_alloc(aw, 8);
    atlas_upload(t, facebuf);
}

void look_build(Bore *b) {
    memcpy(look, b->look, LK_N);
    stage = b->stage;
    custom = 0;
    fixLook();
    buildLook();
    setColors();
    memcpy(b->look, look, LK_N);          // fixLook may have pulled a choice into what the stage allows
    memcpy(b->vox, vox, sizeof vox);
    memcpy(b->dec, dec, sizeof dec);
    for (int i = 0; i < 9; i++) b->col[i] = base[i];
    b->sty_eye = (u8)sty[0]; b->sty_mouth = (u8)sty[1];
    { int L, T, hs; bodyPlan(&L, &T, &hs); b->legs = (u8)L; b->torso = (u8)T; b->headsz = (u8)hs; }
    b->bx0 = (u8)BX0; b->bxw = (u8)BXW; b->bxh = (u8)BXH; b->bxd = (u8)BXD;
    // the face palette (k dark, w white, r red, s lid shadow, i iris, b blush, g glasses frame, h brows, l dark lenses), as setColors builds it
    u16 sk = base[1], bl = RGB(((sk & 31) + 31) / 2, (((sk >> 5) & 31) + 8) / 2, (((sk >> 10) & 31) + 12) / 2);
    b->facepal[0] = base[3]; b->facepal[1] = base[2]; b->facepal[2] = base[4]; b->facepal[3] = shade(base[1], 11);
    b->facepal[4] = toneBy(eyeTones[look[LK_EYECOL] % NSW], slideEffS(look[LK_EYETONE])); b->facepal[5] = bl; b->facepal[6] = RGB(6, 6, 8);
    b->facepal[7] = shade(base[5], 10); b->facepal[8] = RGB(4, 5, 9);
    bake_face(b, 1);
    bake_face(b, 0);
    voxel_prepare(b);
    b->baked = 1;
}
