// items.c - BORE's textured-box items drawn as real boxes. The art is the GBA game's own (items_data.inc and the art headers).
#include "items.h"
#include "gfx.h"
#include "atlas.h"

#include "items_data.inc"   // defines Mat, IBox, IObj and the MAT macro, straight from the GBA game

// ---- the sound pack, the phone and the DeadSet: boxes made for BORE 2 (the GBA drew them as pixel art) ----
static const u16 pRad[5] = { RGB(6,6,8), RGB(20,20,22), RGB(28,28,30), RGB(30,24,4), RGB(12,12,14) };
MAT(mRadF, pRad, 6, 4, "aaaaaa", "abbbda", "abccba", "aaaaaa")
MAT(mRadS, pRad, 1, 1, "b")
MAT(mRadT, pRad, 6, 3, "abbbba", "abeeba", "aaaaaa")
static const IBox bxRadio[2] = { {1,2,7,5,0,4,{&mRadF,&mRadS,&mRadS,&mRadS,&mRadT}}, {3,3,5,4,4,8,{&mRadS,&mRadS,&mRadS,&mRadS,&mRadS}} };
static const u16 pSt[5] = { RGB(4,4,6), RGB(14,14,16), RGB(26,26,28), RGB(8,22,30), RGB(30,8,8) };
MAT(mStF, pSt, 4, 14, "aaaa", "abba", "acca", "abba", "aaaa", "abba", "acca", "acca", "abba", "aaaa", "adda", "aeea", "abba", "aaaa")
MAT(mStS, pSt, 1, 1, "a")
MAT(mStT, pSt, 1, 1, "b")
static const IBox bxStereo[1] = { {2,2,6,6,0,14,{&mStF,&mStS,&mStS,&mStS,&mStT}} };
static const u16 pPhn[4] = { RGB(6,6,8), RGB(28,8,8), RGB(30,30,30), RGB(16,16,18) };
MAT(mPhB, pPhn, 1, 1, "b")
MAT(mPhT, pPhn, 3, 2, "bcb", "bab")
static const IBox bxPhone[2] = { {2,3,6,5,0,6,{&mPhB,&mPhB,&mPhB,&mPhB,&mPhB}}, {3,3,5,5,6,8,{&mPhT,&mPhB,&mPhB,&mPhB,&mPhT}} };
static const u16 pDsB[6] = { RGB(6,6,8), RGB(12,12,14), RGB(28,21,16), RGB(6,20,31), RGB(22,15,11), RGB(20,20,23) };
MAT(mDsPed, pDsB, 1, 1, "b")
MAT(mDsNeck, pDsB, 1, 1, "c")
MAT(mDsHead, pDsB, 4, 4, "cccc", "dddd", "dddd", "cccc")
MAT(mDsHeadS, pDsB, 1, 1, "c")
MAT(mDsTop, pDsB, 1, 1, "e")
static const IBox bxDeadSet[3] = { {2,2,6,6,0,6,{&mDsPed,&mDsPed,&mDsPed,&mDsPed,&mDsPed}}, {3,3,5,5,6,8,{&mDsNeck,&mDsNeck,&mDsNeck,&mDsNeck,&mDsNeck}},
                                   {2,2,6,6,8,14,{&mDsHead,&mDsHeadS,&mDsHeadS,&mDsHeadS,&mDsTop}} };
static const u16 pStr[3] = { RGB(27,25,21), RGB(19,17,14), RGB(10,9,8) };
MAT(mStrT, pStr, 1, 1, "a")
MAT(mStrS, pStr, 1, 1, "b")
static const IBox bxStairsUp[4] = { {0,6,8,8,0,4,{&mStrS,&mStrS,&mStrS,&mStrS,&mStrT}}, {0,4,8,6,0,8,{&mStrS,&mStrS,&mStrS,&mStrS,&mStrT}},
                                    {0,2,8,4,0,12,{&mStrS,&mStrS,&mStrS,&mStrS,&mStrT}}, {0,0,8,2,0,16,{&mStrS,&mStrS,&mStrS,&mStrS,&mStrT}} };
static const u16 pFlag[4] = { RGB(26,26,28), RGB(5,19,28), RGB(31,17,3), RGB(31,31,31) };
MAT(mPole, pFlag, 1, 1, "a")
MAT(mFlagC, pFlag, 4, 3, "bbbb", "dddd", "bbbb")
MAT(mFlagS, pFlag, 4, 3, "cccc", "aaaa", "cccc")
static const IBox bxFlagC[2] = { {3,3,4,4,0,18,{&mPole,&mPole,&mPole,&mPole,&mPole}}, {4,3,8,4,14,17,{&mFlagC,&mFlagC,&mFlagC,&mFlagC,&mFlagC}} };
static const IBox bxFlagS[2] = { {3,3,4,4,0,18,{&mPole,&mPole,&mPole,&mPole,&mPole}}, {4,3,8,4,14,17,{&mFlagS,&mFlagS,&mFlagS,&mFlagS,&mFlagS}} };
static const u16 pWork[2] = { RGB(31,24,4), RGB(11,7,1) };
MAT(mWorkT, pWork, 4, 4, "abba", "bbbb", "bbbb", "abba")
MAT(mWorkS, pWork, 1, 1, "b")
static const IBox bxWork[1] = { {1,1,7,7,0,1,{&mWorkS,&mWorkS,&mWorkS,&mWorkS,&mWorkT}} };
static const u16 pMat[2] = { RGB(13,8,5), RGB(20,13,8) };
MAT(mMatT, pMat, 2, 2, "ab", "ba")
static const IBox bxSpawn[1] = { {2,2,6,6,0,1,{&mMatT,&mMatT,&mMatT,&mMatT,&mMatT}} };


// ---- BORE 2's outdoor pack: the island (Castaway) and the desert town ----
static const u16 pPalm[5] = { RGB(10,7,4), RGB(16,11,6), RGB(6,16,5), RGB(9,22,7), RGB(13,27,10) };
MAT(mPalmTrunk, pPalm, 2, 14, "ab", "ba", "ab", "ba", "ab", "ba", "ab", "ba", "ab", "ba", "ab", "ba", "ab", "ba")
MAT(mPalmTop, pPalm, 8, 8, "..cdec..", ".cdeedc.", "cdeddedc", "deddddde", "deddddde", "cdeddedc", ".cdeedc.", "..cdec..")
MAT(mPalmSide, pPalm, 8, 3, "cdeeeedc", ".cdddddc", "..c..c..")
MAT(mPalmSide2, pPalm, 6, 2, "cdeedc", ".cddc.")
static const IBox bxPalm[3] = { {3,3,5,5,0,14,{&mPalmTrunk,&mPalmTrunk,&mPalmTrunk,&mPalmTrunk,&mPalmTrunk}},
                                {0,0,8,8,14,17,{&mPalmSide,&mPalmSide,&mPalmSide,&mPalmSide,&mPalmTop}},
                                {1,1,7,7,17,19,{&mPalmSide2,&mPalmSide2,&mPalmSide2,&mPalmSide2,&mPalmTop}} };
static const u16 pCact[4] = { RGB(5,12,5), RGB(9,19,8), RGB(13,24,11), RGB(28,27,22) };
MAT(mCactS, pCact, 3, 10, "aba", "bcb", "aba", "bcb", "aba", "bcb", "aba", "bcb", "aba", "bcb")
MAT(mCactT, pCact, 3, 3, "aba", "bdb", "aba")
MAT(mCactArm, pCact, 2, 5, "ab", "bc", "ab", "bc", "ab")
static const IBox bxCactus[3] = { {3,3,5,5,0,10,{&mCactS,&mCactS,&mCactS,&mCactS,&mCactT}}, {1,3,3,5,5,6,{&mCactArm,&mCactArm,&mCactArm,&mCactArm,&mCactT}},
                                  {1,3,2,5,5,9,{&mCactArm,&mCactArm,&mCactArm,&mCactArm,&mCactT}} };
static const u16 pFire[6] = { RGB(8,5,3), RGB(14,9,5), RGB(31,20,4), RGB(31,10,2), RGB(31,28,10), RGB(10,10,10) };
MAT(mLogs, pFire, 6, 2, "ababab", "bababa")
MAT(mLogT, pFire, 6, 6, "ffabff", "fbabaf", "ababab", "bababa", "fababf", "ffbaff")
MAT(mFlame, pFire, 4, 6, ".e..", ".ec.", "ecec", "dcce", "dccd", "cddc")
MAT(mFlameT, pFire, 2, 2, "ec", "ce")
static const IBox bxCampfire[2] = { {1,1,7,7,0,2,{&mLogs,&mLogs,&mLogs,&mLogs,&mLogT}}, {2,2,6,6,2,8,{&mFlame,&mFlame,&mFlame,&mFlame,&mFlameT}} };
static const u16 pTent[4] = { RGB(10,8,6), RGB(24,17,8), RGB(18,12,6), RGB(28,22,12) };
MAT(mTentF, pTent, 8, 7, "...aa...", "..abba..", ".abbbba.", "abbccbba", "abbccbba", "abbccbba", "aaaaaaaa")
MAT(mTentS, pTent, 8, 7, "bbbbbbbb", "bdbbbbdb", "bbbbbbbb", "bbdbbdbb", "bbbbbbbb", "bdbbbbdb", "aaaaaaaa")
MAT(mTentT, pTent, 1, 1, "b")
static const IBox bxTent[1] = { {0,0,8,8,0,7,{&mTentF,&mTentS,&mTentS,&mTentS,&mTentT}} };
static const u16 pRock[4] = { RGB(10,10,11), RGB(16,16,17), RGB(21,20,20), RGB(13,13,14) };
MAT(mRockS, pRock, 5, 4, "abbca", "bccbb", "abbcb", "aabba")
MAT(mRockT, pRock, 5, 5, "abbba", "bcccb", "bcccb", "bcbcb", "abbba")
static const IBox bxRock[2] = { {1,2,6,6,0,4,{&mRockS,&mRockS,&mRockS,&mRockS,&mRockT}}, {2,3,5,5,4,6,{&mRockS,&mRockS,&mRockS,&mRockS,&mRockT}} };
static const u16 pOut[4] = { RGB(9,6,3), RGB(18,12,6), RGB(14,9,5), RGB(26,22,8) };
MAT(mOutF, pOut, 6, 14, "aaaaaa", "abbbba", "abcdba", "abbbba", "abbbba", "abbbba", "abbbba", "abbbba", "abbbba", "abbbba", "abbbba", "abbbba", "abbbba", "aaaaaa")
MAT(mOutS, pOut, 6, 14, "aaaaaa", "abbbba", "acccca", "abbbba", "acccca", "abbbba", "acccca", "abbbba", "acccca", "abbbba", "acccca", "abbbba", "acccca", "aaaaaa")
MAT(mOutT, pOut, 1, 1, "a")
static const IBox bxOuthouse[1] = { {1,1,7,7,0,14,{&mOutF,&mOutS,&mOutS,&mOutS,&mOutT}} };
static const u16 pSpr[4] = { RGB(12,12,13), RGB(8,18,28), RGB(14,24,31), RGB(20,20,21) };
MAT(mSprRim, pSpr, 1, 1, "d")
MAT(mSprT, pSpr, 6, 6, "dddddd", "dbbbbd", "dbccbd", "dbccbd", "dbbbbd", "dddddd")
static const IBox bxSpring[1] = { {1,1,7,7,0,3,{&mSprRim,&mSprRim,&mSprRim,&mSprRim,&mSprT}} };
static const u16 pBoat[4] = { RGB(12,7,3), RGB(22,14,7), RGB(28,28,30), RGB(6,4,2) };
MAT(mBoatS, pBoat, 8, 3, "bbbbbbbb", "abbbbbba", "aaaaaaaa")
MAT(mBoatT, pBoat, 8, 4, "abbbbbba", "bbaabbab", "bbaabbab", "abbbbbba")
MAT(mMast, pBoat, 1, 1, "d")
MAT(mSail, pBoat, 4, 8, "c...", "cc..", "ccc.", "cccc", "cccc", "ccc.", "cc..", "c...")
static const IBox bxBoat[3] = { {0,2,8,6,0,3,{&mBoatS,&mBoatS,&mBoatS,&mBoatS,&mBoatT}}, {3,4,4,5,3,12,{&mMast,&mMast,&mMast,&mMast,&mMast}}, {4,4,8,5,5,13,{&mSail,&mSail,&mSail,&mSail,&mSail}} };
static const u16 pBus[4] = { RGB(8,8,10), RGB(28,24,4), RGB(26,26,28), RGB(4,10,24) };
MAT(mBusPole, pBus, 1, 1, "a")
MAT(mBusSign, pBus, 6, 4, "bbbbbb", "bddddb", "bddddb", "bbbbbb")
MAT(mBusBench, pBus, 8, 2, "cccccccc", "a.a..a.a")
static const IBox bxBusStop[2] = { {3,1,4,2,0,16,{&mBusPole,&mBusPole,&mBusPole,&mBusPole,&mBusPole}}, {1,1,7,2,12,16,{&mBusSign,&mBusSign,&mBusSign,&mBusSign,&mBusSign}} };
static const u16 pLog[3] = { RGB(10,7,4), RGB(17,12,6), RGB(22,16,8) };
MAT(mLogS, pLog, 8, 3, "bcbcbcbc", "abababab", "aaaaaaaa")
MAT(mLogTop, pLog, 8, 3, "cbcbcbcb", "bcbcbcbc", "cbcbcbcb")
static const IBox bxLogBench[1] = { {0,2,8,5,0,3,{&mLogS,&mLogS,&mLogS,&mLogS,&mLogTop}} };

// ---- the item table: letter, name, boxes (and the along-y variant), flags ----
enum { F_ROT = 1, F_ALONG = 2, F_FLAT = 4, F_HIGH = 8 };   // F_FLAT: walk over it; F_HIGH: stand on top (crates)
typedef struct { char ch; const char *name; const IBox *b; u8 n; const IBox *bv; u8 nv; u8 flags; u16 cost; } ItemDef;
static const ItemDef defs[] = {
    { '#', "CRATE", bxCrate, 1, 0, 0, F_HIGH, 40 },
    { 'F', "FRIDGE", bxFridge, 1, 0, 0, F_ROT, 600 },
    { 'T', "TOILET", bxToilet, 3, 0, 0, F_ROT, 300 },
    { '=', "RAIL", bxRailU, 3, bxRailV, 3, F_ALONG, 80 },
    { 'L', "LEDGE", bxLedgeU, 1, bxLedgeV, 1, F_ALONG, 60 },
    { 'N', "BENCH", bxBenchU, 3, bxBenchV, 3, F_ALONG, 120 },
    { 'J', "JERSEY", bxJerseyU, 3, bxJerseyV, 3, F_ALONG, 60 },
    { 'X', "FUNBOX", bxFunbox, 1, 0, 0, F_HIGH, 200 },
    { 'O', "BARREL", bxBarrel, 1, 0, 0, 0, 30 },
    { 'Y', "TRASH CAN", bxTrashCan, 2, 0, 0, 0, 30 },
    { 'Z', "PLANTER", bxPlanter, 1, 0, 0, 0, 90 },
    { 'K', "PICNIC", bxPicnicTable, 7, 0, 0, 0, 250 },
    { 'M', "MANUAL PAD", bxManualPad, 1, 0, 0, F_FLAT, 120 },
    { '1', "KICKER", bxKicker, 8, 0, 0, F_ROT, 300 },
    { '5', "Q PIPE", bxQuarterPipe, 8, 0, 0, F_ROT, 500 },
    { '9', "LAUNCH", bxLaunch, 8, 0, 0, F_ROT, 400 },
    { 'S', "BED", bxBed, 4, 0, 0, F_ROT, 700 },
    { 'H', "SHOWER", bxShower, 4, 0, 0, F_ROT, 450 },
    { 'C', "SOFA", bxSofa, 4, 0, 0, F_ROT, 500 },
    { 'D', "DOOR", bxDoor, 1, 0, 0, F_FLAT, 100 },
    { 'G', "WATER PIPE", bxPipe, 4, 0, 0, 0, 90 },
    { 'V', "LAVA LAMP", bxLava, 3, 0, 0, 0, 60 },
    { 'U', "BEANBAG", bxBeanbag, 2, 0, 0, F_ROT, 150 },
    { 'Q', "DEADSET VYBE", bxDeadSet, 3, 0, 0, F_ROT, 5000 },
    { 'I', "PHONE", bxPhone, 2, 0, 0, 0, 120 },
    { 'R', "RADIO", bxRadio, 2, 0, 0, F_ROT, 200 },
    { 'A', "SOUND SYSTEM", bxStereo, 1, 0, 0, F_ROT, 900 },
    { 'v', "TV", bxTv, 5, 0, 0, F_ROT, 800 },
    { 'b', "BOOKSHELF", bxShelf, 1, 0, 0, F_ROT, 350 },
    { 'c', "COFFEE MAKER", bxCoffee, 4, 0, 0, 0, 90 },
    { 'q', "AQUARIUM", bxAqua, 3, 0, 0, 0, 400 },
    { 'm', "TREADMILL", bxTread, 5, 0, 0, F_ROT, 700 },
    { 'r', "RUG", bxDRug, 1, 0, 0, F_FLAT, 80 },
    { 't', "DINING TABLE", bxDTable, 5, 0, 0, 0, 300 },
    { 'h', "CHAIR", bxDChair, 6, 0, 0, F_ROT, 90 },
    { 'd', "DESK", bxDDesk, 6, 0, 0, F_ROT, 350 },
    { 'l', "FLOOR LAMP", bxDLamp, 3, 0, 0, 0, 110 },
    { 'p', "HOUSEPLANT", bxDPlant, 3, 0, 0, 0, 70 },
    { 'i', "DRESSER", bxDDresser, 1, 0, 0, F_ROT, 400 },
    { 'o', "FIREPLACE", bxDFire, 3, 0, 0, F_ROT, 900 },
    { 'y', "COUNTER", bxDCounter, 1, 0, 0, F_ROT, 250 },
    { '^', "STAIRS UP", bxStairsUp, 4, 0, 0, F_ROT, 500 },
    { 'a', "COMMUNITY FLAG", bxFlagC, 2, 0, 0, F_FLAT, 0 },
    { 'k', "SKATE FLAG", bxFlagS, 2, 0, 0, F_FLAT, 0 },
    { 'x', "WORK MARKER", bxWork, 1, 0, 0, F_FLAT, 0 },
    { 'P', "SPAWN", bxSpawn, 1, 0, 0, F_FLAT, 0 },
    { 'B', "BOARD", bxBoard, 5, 0, 0, F_FLAT, 0 },
    { 'g', "PALM TREE", bxPalm, 3, 0, 0, 0, 150 },
    { 'u', "CACTUS", bxCactus, 3, 0, 0, 0, 40 },
    { 's', "CAMPFIRE", bxCampfire, 2, 0, 0, 0, 80 },
    { 'z', "TENT", bxTent, 1, 0, 0, F_ROT, 200 },
    { '*', "ROCK", bxRock, 2, 0, 0, 0, 20 },
    { '&', "OUTHOUSE", bxOuthouse, 1, 0, 0, F_ROT, 150 },
    { '0', "SPRING", bxSpring, 1, 0, 0, 0, 120 },
    { '@', "BOAT", bxBoat, 3, 0, 0, F_ROT, 0 },
    { '+', "BUS STOP", bxBusStop, 2, 0, 0, F_ROT, 0 },
    { '%', "LOG BENCH", bxLogBench, 1, 0, 0, F_ROT, 30 },
};
#define NDEFS ((int)(sizeof(defs) / sizeof(defs[0])))
static const ItemDef *def_of(char ch) {
    if (ch >= '1' && ch <= '4') ch = '1';
    if (ch >= '5' && ch <= '8') ch = '5';
    if (ch == '~') ch = '^';
    for (int i = 0; i < NDEFS; i++) if (defs[i].ch == ch) return &defs[i];
    return 0;
}
int items_count(void) { return NDEFS; }
char items_at(int i) { return defs[i].ch; }
int items_cost(char ch) { const ItemDef *d = def_of(ch); return d ? d->cost : 0; }
int items_is_item(char ch) { return def_of(ch) != 0; }
int items_blocking(char ch) { const ItemDef *d = def_of(ch); return d && !(d->flags & F_FLAT); }
int items_rotates(char ch) { const ItemDef *d = def_of(ch); return d && (d->flags & F_ROT); }
int items_along(char ch) { const ItemDef *d = def_of(ch); return d && (d->flags & F_ALONG); }
const char *items_name(char ch) { const ItemDef *d = def_of(ch); return d ? d->name : "?"; }
int items_height(char ch) {
    const ItemDef *d = def_of(ch); if (!d) return 0;
    int top = 0; for (int i = 0; i < d->n; i++) if (d->b[i].z1 > top) top = d->b[i].z1;
    return top * IZ;
}

// ---- the material cache: every Mat becomes one texture in the atlas ----
#define MAXMAT 640
static const Mat *mat_key[MAXMAT];
static TexRef mat_tex[MAXMAT];
static int nmat;
static u16 mbuf[32 * 32];
static TexRef mat_ref(const Mat *m) {
    for (int i = 0; i < nmat; i++) if (mat_key[i] == m) return mat_tex[i];
    int w = m->w, h = m->h;
    if (w > 32) w = 32; if (h > 32) h = 32;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        char ch = m->rows[y][x];
        u16 c = (ch == '.') ? 0 : m->pal[ch - 'a'];
        if (ch != '.' && c == 0) c = 0x8000;
        mbuf[y * w + x] = c;
    }
    TexRef t = atlas_add(w, h, mbuf);
    if (nmat < MAXMAT) { mat_key[nmat] = m; mat_tex[nmat] = t; nmat++; }
    return t;
}
// every box face's texture, resolved once: ref_of(def, variant, box, face)
#define MAXBOX 440
static TexRef box_ref[MAXBOX][5];
static int box_base[NDEFS][2];     // first box index of each def's main / along-y boxes
static int nbox;
static void cache_boxes(const IBox *b, int n, int *base) {
    *base = nbox;
    for (int i = 0; i < n && nbox < MAXBOX; i++, nbox++) for (int f = 0; f < 5; f++) {
        TexRef none = { 0, 0, 0, 0, 0 };
        box_ref[nbox][f] = b[i].m[f] ? mat_ref(b[i].m[f]) : none;
    }
}
void items_init(void) {
    nmat = 0; nbox = 0;
    for (int i = 0; i < NDEFS; i++) { cache_boxes(defs[i].b, defs[i].n, &box_base[i][0]); if (defs[i].bv) cache_boxes(defs[i].bv, defs[i].nv, &box_base[i][1]); else box_base[i][1] = -1; }
}

// ---- drawing ----
static void rotPt(int r, int a, int b, int *u, int *v) {
    switch (r & 3) { case 0: *u = a; *v = b; break; case 1: *u = b; *v = 8 - a; break; case 2: *u = 8 - a; *v = 8 - b; break; default: *u = 8 - b; *v = a; }
}
static void unrot(int r, int u, int v, int *a, int *b) {   // world (u,v) -> local (a,b)
    switch (r & 3) { case 0: *a = u; *b = v; break; case 1: *a = 8 - v; *b = u; break; case 2: *a = 8 - u; *b = 8 - v; break; default: *a = v; *b = 8 - u; }
}
static int shade_of(int nx, int ny, int nz) { int L = gfx_light(nx, ny, nz); return CLAMP(L, 40, 200); }

// a side: a top-to-bottom gradient, darker where it meets what it stands on (contact shading, like the walls). ao = the bottom's light /256
static int box_ao = 256;
static void tex_face(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, const TexRef *tp, int nx, int ny, int nz, int flip) {
    if (!tp->tpage) return;
    TexRef t = *tp;
    int u0 = t.u, u1 = t.u + t.w - 1;
    if (flip) { int tmp = u0; u0 = u1; u1 = tmp; }
    int s = shade_of(nx, ny, nz), st = s + (s >> 4), sb = (s * box_ao) >> 8;
    u32 ct = (u32)st * 0x010101, cb = (u32)sb * 0x010101;
    gfx_quad_gt(a, b, c, d, t.tpage, 0, u0, t.v, u1, t.v, u0, t.v + t.h - 1, u1, t.v + t.h - 1, ct, ct, cb, cb, 1);
}

static void draw_box(const IBox *bx, const TexRef *ref, int r, int ox, int oz, int t) {
    int ua, va, ub, vb; rotPt(r, bx->a0, bx->b0, &ua, &va); rotPt(r, bx->a1, bx->b1, &ub, &vb);
    int u0 = MIN(ua, ub), u1 = MAX(ua, ub), v0 = MIN(va, vb), v1 = MAX(va, vb);
    if (u1 == u0 || v1 == v0) return;
    int x0 = ox + u0 * IUNIT, x1 = ox + u1 * IUNIT;
    int zN = -(oz + v0 * IUNIT), zS = -(oz + v1 * IUNIT);          // north (max world z) and south
    int y0 = -bx->z1 * IZ, y1 = -bx->z0 * IZ;
    if (bx->z1 == bx->z0) y0 = y1 - 2;
    (void)t;
    { int h = bx->z1 - bx->z0; box_ao = bx->z0 == 0 ? (h > 8 ? 176 : 150) : 214; }   // on the floor: a dark foot (stronger on low things); higher up: a soft one
    SVECTOR A, B, C, Dd;
    // world faces: 0 south (+v), 1 east (+u), 2 north (-v), 3 west (-u) show local face (wf - r)
    const TexRef *mS = &ref[(0 - r) & 3], *mE = &ref[(1 - r) & 3], *mN = &ref[(2 - r) & 3], *mW = &ref[(3 - r) & 3];
    // only the sides that face the eye (the rest could never show)
    if (cam.eye.vz < zS) { setVector(&A, x0, y0, zS); setVector(&B, x1, y0, zS); setVector(&C, x0, y1, zS); setVector(&Dd, x1, y1, zS); tex_face(&A, &B, &C, &Dd, mS, 0, 0, -ONE, 0); }
    if (cam.eye.vx > x1) { setVector(&A, x1, y0, zS); setVector(&B, x1, y0, zN); setVector(&C, x1, y1, zS); setVector(&Dd, x1, y1, zN); tex_face(&A, &B, &C, &Dd, mE, ONE, 0, 0, 0); }
    if (cam.eye.vz > zN) { setVector(&A, x1, y0, zN); setVector(&B, x0, y0, zN); setVector(&C, x1, y1, zN); setVector(&Dd, x0, y1, zN); tex_face(&A, &B, &C, &Dd, mN, 0, 0, ONE, 0); }
    if (cam.eye.vx < x0) { setVector(&A, x0, y0, zN); setVector(&B, x0, y0, zS); setVector(&C, x0, y1, zN); setVector(&Dd, x0, y1, zS); tex_face(&A, &B, &C, &Dd, mW, -ONE, 0, 0, 0); }
    // top: TL=(x0,zN) TR=(x1,zN) BL=(x0,zS) BR=(x1,zS); the texture turns with the item
    if (ref[4].tpage && cam.eye.vy < y0) {
        TexRef tt = ref[4];
        int uv[4][2]; int cu[4] = { u0, u1, u0, u1 }, cv[4] = { v0, v0, v1, v1 };
        int aw = bx->a1 - bx->a0, bh = bx->b1 - bx->b0; if (aw < 1) aw = 1; if (bh < 1) bh = 1;
        for (int k = 0; k < 4; k++) {
            int la, lb; unrot(r, cu[k], cv[k], &la, &lb);
            int fu = (la - bx->a0) * (tt.w - 1) / aw, fv = (lb - bx->b0) * (tt.h - 1) / bh;
            uv[k][0] = tt.u + CLAMP(fu, 0, tt.w - 1); uv[k][1] = tt.v + CLAMP(fv, 0, tt.h - 1);
        }
        setVector(&A, x0, y0, zN); setVector(&B, x1, y0, zN); setVector(&C, x0, y0, zS); setVector(&Dd, x1, y0, zS);
        gfx_quad_ft(&A, &B, &C, &Dd, tt.tpage, 0, uv[0][0], uv[0][1], uv[1][0], uv[1][1], uv[2][0], uv[2][1], uv[3][0], uv[3][1], shade_of(0, -ONE, 0), 1, 0);
    }
}

int getenv_noshadow;
void items_draw(char ch, int mx, int my, int rot, int along, int t) {
    const ItemDef *d = def_of(ch);
    if (!d) return;
    const IBox *b = d->b; int n = d->n; int base = box_base[d - defs][0];
    if ((d->flags & F_ALONG) && !along && d->bv) { b = d->bv; n = d->nv; base = box_base[d - defs][1]; }
    int r = (d->flags & F_ROT) ? rot : 0;
    if (ch >= '1' && ch <= '4') r = ch - '1';
    if (ch >= '5' && ch <= '8') r = ch - '5';
    if (ch == '~') r = (rot + 2) & 3;
    int ox = mx * TILE, oz = my * TILE;
    if (!(d->flags & F_FLAT) && !getenv_noshadow) {   // a contact shadow under the item: the union of its boxes on the floor, a touch bigger
        int a0 = 8, b0 = 8, a1 = 0, b1 = 0;
        for (int i = 0; i < n; i++) { if (b[i].z0 > 2) continue; a0 = MIN(a0, b[i].a0); b0 = MIN(b0, b[i].b0); a1 = MAX(a1, b[i].a1); b1 = MAX(b1, b[i].b1); }
        if (a1 > a0 && b1 > b0) {
            int ua, va, ub, vb; rotPt(r, a0, b0, &ua, &va); rotPt(r, a1, b1, &ub, &vb);
            int u0 = MIN(ua, ub), u1 = MAX(ua, ub), v0 = MIN(va, vb), v1 = MAX(va, vb);
            int x0 = ox + u0 * IUNIT - 10, x1 = ox + u1 * IUNIT + 10, zN = -(oz + v0 * IUNIT) + 10, zS = -(oz + v1 * IUNIT) - 10;
            SVECTOR A, B, C, Dd;
            x0 -= 26; x1 += 26; zN += 26; zS -= 26;                     // the soft edge reaches past the footprint
            setVector(&A, x0, -2, zN); setVector(&B, x1, -2, zN); setVector(&C, x0, -2, zS); setVector(&Dd, x1, -2, zS);
            gfx_shadow_quad(&A, &B, &C, &Dd);
        }
    }
    for (int i = 0; i < n; i++) draw_box(&b[i], box_ref[base + i], r, ox, oz, t);
}
