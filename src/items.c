// items.c - BORE's textured-box items drawn as real boxes. The art is the GBA game's own (items_data.inc and the art headers).
#include "items.h"
#include "gfx.h"
#include "dog.h"
#include "atlas.h"

#include "items_data.inc"   // defines Mat, IBox, IObj and the MAT macro, straight from the GBA game
#include "items_more.inc"   // BORE 2's furniture pack (tools/make_items2.py)
#include "items_gotchi.inc"   // the BOREAGOTCHI! pack's objects (tools/make_items_gotchi.py)
#include "items_party.inc"    // the PARTY pack (tools/make_items_party.py)
#include "items_band.inc"     // the SUPERSTAR practice instruments (tools/make_items_band.py)
#include "items_home.inc"     // the garden, pool and beach things (tools/make_items_home.py)
#include "items_farm.inc"     // FARM LIFE: crop bed, coop, cow, stand (tools/make_items_farm.py)
#include "items_space.inc"    // the telescopes, rocket and console (tools/make_items_space.py)
#include "items_bed.inc"      // the beds, cribs and nursery things (tools/make_items_q.py)
#ifdef BORE_DC
#include "items_dc.inc"       // the Dreamcast furniture set (console, VMU stand, swirl rug, arcade cabinet, lamps)
#else
#define ITEMS_DC
#endif

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

// ---- SAN BOREAS: trees for the mainland, a shop shelf, a FOR SALE sign, a pool lounger ----
static const u16 pOak[6] = { RGB(9,6,3), RGB(15,10,5), RGB(5,13,4), RGB(8,18,6), RGB(12,23,8), RGB(17,27,11) };
MAT(mOakTrunk, pOak, 2, 10, "ab", "ba", "ab", "bb", "ab", "ba", "ab", "bb", "ab", "ba")
MAT(mOakLeaf, pOak, 8, 7, "..dede..", ".deefed.", "deeffeed", "edefedde", "deededed", ".cdddcd.", "..cccc..")
MAT(mOakTop, pOak, 8, 8, "..defe..", ".deffed.", "deffffed", "efffefff", "effeffef", "deffffed", ".deffed.", "..defe..")
MAT(mOakCap, pOak, 6, 3, ".deed.", "deffed", "cdeedc")
static const IBox bxOak[3] = { {3,3,5,5,0,10,{&mOakTrunk,&mOakTrunk,&mOakTrunk,&mOakTrunk,&mOakTrunk}},
                               {0,0,8,8,8,15,{&mOakLeaf,&mOakLeaf,&mOakLeaf,&mOakLeaf,&mOakTop}},
                               {1,1,7,7,15,18,{&mOakCap,&mOakCap,&mOakCap,&mOakCap,&mOakTop}} };
static const u16 pPine[6] = { RGB(8,5,3), RGB(13,9,5), RGB(3,9,5), RGB(5,13,7), RGB(7,17,9), RGB(10,21,12) };
MAT(mPineTrunk, pPine, 2, 4, "ab", "ba", "ab", "ba")
MAT(mPineA, pPine, 8, 5, "..deed..", ".deffed.", "ddeffedd", "cdeddedc", "cccccccc")
MAT(mPineB, pPine, 6, 5, "..ee..", ".deed.", "deffed", "cdeedc", "cccccc")
MAT(mPineC, pPine, 4, 4, ".ee.", "deed", "deed", "cccc")
MAT(mPineT, pPine, 4, 4, "defe", "effe", "effd", "edde")
static const IBox bxPine[4] = { {3,3,5,5,0,4,{&mPineTrunk,&mPineTrunk,&mPineTrunk,&mPineTrunk,&mPineTrunk}},
                                {0,0,8,8,4,10,{&mPineA,&mPineA,&mPineA,&mPineA,&mPineT}},
                                {1,1,7,7,10,15,{&mPineB,&mPineB,&mPineB,&mPineB,&mPineT}},
                                {2,2,6,6,15,20,{&mPineC,&mPineC,&mPineC,&mPineC,&mPineT}} };
static const u16 pShelf[8] = { RGB(20,20,22), RGB(12,12,14), RGB(30,6,5), RGB(6,18,30), RGB(30,26,4), RGB(8,24,8), RGB(30,30,30), RGB(28,14,22) };
MAT(mShF, pShelf, 8, 10, "aaaaaaaa", "acdecfdh", "abbbbbbb", "aaaaaaaa", "aefcghcd", "abbbbbbb", "aaaaaaaa", "ahcdfecg", "abbbbbbb", "aaaaaaaa")
MAT(mShS, pShelf, 1, 1, "a")
MAT(mShT, pShelf, 2, 2, "aa", "ab")
static const IBox bxShelfG[1] = { {0,2,8,6,0,10,{&mShF,&mShS,&mShF,&mShS,&mShT}} };
static const u16 pSign[4] = { RGB(22,22,24), RGB(30,30,30), RGB(28,4,4), RGB(8,8,10) };
MAT(mSignP, pSign, 1, 1, "a")
MAT(mSignF, pSign, 6, 5, "cccccc", "cbbbbc", "cddddc", "cbbbbc", "cccccc")
static const IBox bxSign[2] = { {3,3,4,4,0,8,{&mSignP,&mSignP,&mSignP,&mSignP,&mSignP}}, {1,3,7,4,6,11,{&mSignF,&mSignP,&mSignF,&mSignP,&mSignP}} };
static const u16 pLnr[4] = { RGB(24,24,26), RGB(30,30,31), RGB(6,18,28), RGB(30,22,4) };
MAT(mLnrS, pLnr, 8, 2, "aaaaaaaa", "a.a..a.a")
MAT(mLnrT, pLnr, 8, 4, "bcbcbcbc", "cbcbcbcb", "bcbcbcbc", "cbcbcbcb")
MAT(mLnrB, pLnr, 2, 3, "bc", "cb", "bc")
static const u16 pLamp[4] = { RGB(9, 9, 11), RGB(15, 15, 17), RGB(31, 29, 20), RGB(26, 24, 14) };
MAT(mLampP, pLamp, 2, 2, "ab", "ba")
MAT(mLampH, pLamp, 4, 2, "bbbb", "cddc")
MAT(mLampT, pLamp, 1, 1, "b")
static const IBox bxStreetLamp[2] = { {3,0,5,2,0,22,{&mLampP,&mLampP,&mLampP,&mLampP,&mLampP}}, {2,0,6,3,22,24,{&mLampH,&mLampH,&mLampH,&mLampH,&mLampT}} };   // (the pole at the kerb edge: local north, turned to face the street)
static const IBox bxLounger[2] = { {0,2,8,6,0,2,{&mLnrS,&mLnrS,&mLnrS,&mLnrS,&mLnrT}}, {0,2,2,6,2,5,{&mLnrB,&mLnrB,&mLnrB,&mLnrB,&mLnrT}} };

// ---- a fuel pump: a red body with a screen, a white hose holder, a yellow cap ----
static const u16 pPump[5]={RGB(27,5,5),RGB(30,30,30),RGB(5,5,7),RGB(31,26,8),RGB(8,26,10)};
MAT(mPmFront,pPump,4,6,"aaaa","acec","aaaa","abba","abba","aaaa")
MAT(mPmSide,pPump,4,6,"aaaa","aaaa","aaaa","abba","aaaa","aaaa")
MAT(mPmCap,pPump,1,1,"d")
MAT(mPmTop,pPump,1,1,"b")
static const IBox bxPump[2]={ {2,2,6,6,0,17,{&mPmFront,&mPmSide,&mPmFront,&mPmSide,&mPmTop}}, {1,1,7,7,17,20,{&mPmCap,&mPmCap,&mPmCap,&mPmCap,&mPmTop}} };

// ---- the item table: letter, name, boxes (and the along-y variant), flags ----
enum { F_ROT = 1, F_ALONG = 2, F_FLAT = 4, F_HIGH = 8, F_L2 = 16, F_W2 = 32, F_KERB = 64 };   // F_KERB: a pole at the kerb edge of a pavement tile (turned to face the street): people walk past it on the same tile   // F_L2 / F_W2: the thing is two tiles long (from its tile, away from the wall) / two wide   // F_FLAT: walk over it; F_HIGH: stand on top (crates)
// ---- the family's things: a crib (white bars round a little mattress), a birthday cake on a party table, a gravestone ----
static const u16 pCrib[5]={RGB(29,29,27),RGB(19,24,30),RGB(30,22,25),RGB(22,22,20),RGB(31,31,31)};
MAT(mCrPost,pCrib,1,1,"a")
MAT(mCrRail,pCrib,1,1,"d")
MAT(mCrBars,pCrib,6,7,"aaaaaa","a.a.a.","a.a.a.","a.a.a.","a.a.a.","a.a.a.","aaaaaa")
MAT(mCrBarsS,pCrib,4,7,"aaaa","a.a.","a.a.","a.a.","a.a.","a.a.","aaaa")
MAT(mCrMat,pCrib,6,4,"bbbbbb","bccccb","bcecbb","bbbbbb")
static const IBox bxCrib[9]={
 {0,1,1,2,0,12,{&mCrPost,&mCrPost,&mCrPost,&mCrPost,&mCrPost}}, {7,1,8,2,0,12,{&mCrPost,&mCrPost,&mCrPost,&mCrPost,&mCrPost}},
 {0,6,1,7,0,12,{&mCrPost,&mCrPost,&mCrPost,&mCrPost,&mCrPost}}, {7,6,8,7,0,12,{&mCrPost,&mCrPost,&mCrPost,&mCrPost,&mCrPost}},
 {1,2,7,6,2,4,{&mCrRail,&mCrRail,&mCrRail,&mCrRail,&mCrMat}},                                  // the mattress
 {1,1,7,2,4,11,{&mCrBars,&mCrRail,&mCrBars,&mCrRail,&mCrRail}}, {1,6,7,7,4,11,{&mCrBars,&mCrRail,&mCrBars,&mCrRail,&mCrRail}},
 {0,2,1,6,4,11,{&mCrRail,&mCrBarsS,&mCrRail,&mCrBarsS,&mCrRail}}, {7,2,8,6,4,11,{&mCrRail,&mCrBarsS,&mCrRail,&mCrBarsS,&mCrRail}} };
static const u16 pCake[7]={RGB(18,11,6),RGB(30,30,30),RGB(31,19,23),RGB(31,31,29),RGB(10,18,31),RGB(31,27,8),RGB(20,12,6)};
MAT(mCkLeg,pCake,1,1,"a")
MAT(mCkCloth,pCake,8,8,"bbbbbbbb","bbcbbcbb","bbbbbbbb","bcbbbbcb","bbbbbbbb","bbcbbcbb","bbbbbbbb","bbbbbbbb")
MAT(mCkClothS,pCake,1,1,"b")
MAT(mCkSide,pCake,4,3,"dddd","cccc","gggg")
MAT(mCkTop,pCake,4,4,"dddd","dccd","dccd","dddd")
MAT(mCkCandle,pCake,1,2,"f","e")
MAT(mCkFlame,pCake,1,1,"f")
static const IBox bxCake[8]={
 {1,1,2,2,0,6,{&mCkLeg,&mCkLeg,&mCkLeg,&mCkLeg,&mCkLeg}}, {6,1,7,2,0,6,{&mCkLeg,&mCkLeg,&mCkLeg,&mCkLeg,&mCkLeg}},
 {1,6,2,7,0,6,{&mCkLeg,&mCkLeg,&mCkLeg,&mCkLeg,&mCkLeg}}, {6,6,7,7,0,6,{&mCkLeg,&mCkLeg,&mCkLeg,&mCkLeg,&mCkLeg}},
 {0,0,8,8,6,7,{&mCkClothS,&mCkClothS,&mCkClothS,&mCkClothS,&mCkCloth}},                         // the table and its cloth
 {2,2,6,6,7,10,{&mCkSide,&mCkSide,&mCkSide,&mCkSide,&mCkTop}},                                  // the cake
 {3,3,4,4,10,12,{&mCkCandle,&mCkCandle,&mCkCandle,&mCkCandle,&mCkFlame}}, {4,4,5,5,10,12,{&mCkCandle,&mCkCandle,&mCkCandle,&mCkCandle,&mCkFlame}} };
static const u16 pGrave[4]={RGB(17,17,18),RGB(23,23,24),RGB(10,10,11),RGB(27,9,14)};
MAT(mGrBase,pGrave,1,1,"c")
MAT(mGrFace,pGrave,4,10,"abba","bbbb","baab","bbbb","baab","bbbb","bbbb","baab","bbbb","bbbb")
MAT(mGrSide,pGrave,1,1,"a")
MAT(mGrTop,pGrave,1,1,"b")
MAT(mGrFlower,pGrave,1,1,"d")
static const IBox bxGrave[3]={
 {1,3,7,6,0,2,{&mGrBase,&mGrBase,&mGrBase,&mGrBase,&mGrSide}},
 {2,4,6,5,2,12,{&mGrFace,&mGrSide,&mGrFace,&mGrSide,&mGrTop}},
 {5,5,6,6,2,3,{&mGrFlower,&mGrFlower,&mGrFlower,&mGrFlower,&mGrFlower}} };

// ---- the island (Castaway): a bottle with a message, the treasure's mound, a tiki idol, a wreck, shells, berry bushes, a fishing post, a raft, the golden idol, the hermit's journal ----
static const u16 pBot[5]={RGB(4,13,7),RGB(8,22,12),RGB(29,27,19),RGB(18,12,6),RGB(14,28,18)};
MAT(mBotS,pBot,4,2,"bebb","aaaa")
MAT(mBotT,pBot,4,2,"bccb","beeb")
MAT(mCork,pBot,1,1,"d")
static const IBox bxBottle[2]={ {2,3,6,5,0,2,{&mBotS,&mBotS,&mBotS,&mBotS,&mBotT}}, {6,3,7,5,0,2,{&mCork,&mCork,&mCork,&mCork,&mCork}} };
static const u16 pDig[3]={RGB(27,23,13),RGB(22,18,10),RGB(12,6,3)};
MAT(mDigT,pDig,6,6,"caaaac","acaaca","aaccaa","aaccaa","acaaca","caaaac")
MAT(mDigS,pDig,1,1,"b")
static const IBox bxDig[2]={ {1,1,7,7,0,1,{&mDigS,&mDigS,&mDigS,&mDigS,&mDigT}}, {2,2,6,6,1,2,{&mDigS,&mDigS,&mDigS,&mDigS,&mDigT}} };
static const u16 pIdol[5]={RGB(9,9,8),RGB(15,15,13),RGB(21,21,18),RGB(7,15,5),RGB(31,24,6)};
MAT(mIdolF,pIdol,6,10,"bbbbbb","bccccb","bebbeb","bbbbbb","bbccbb","bbccbb","bbbbbb","baaaab","bacaab","bbbbbb")
MAT(mIdolS,pIdol,2,2,"bd","db")
MAT(mIdolT,pIdol,2,2,"dd","db")
static const IBox bxIdol[2]={ {2,2,6,6,0,3,{&mIdolS,&mIdolS,&mIdolS,&mIdolS,&mIdolT}}, {1,2,7,6,3,14,{&mIdolF,&mIdolS,&mIdolS,&mIdolS,&mIdolT}} };
static const u16 pWreck[4]={RGB(8,5,3),RGB(15,10,5),RGB(21,14,8),RGB(8,14,6)};
MAT(mWreckS,pWreck,8,4,"bcbcbcbc","abababab","a.a.ab.a","aaaaaaaa")
MAT(mWreckT,pWreck,8,6,"abbbbbba","b.cbbc.b","bccbbccb","b.cbbd.b","bbdbbcbb","abbbbbba")
MAT(mWMast,pWreck,1,1,"a")
static const IBox bxWreck[2]={ {0,1,8,7,0,4,{&mWreckS,&mWreckS,&mWreckS,&mWreckS,&mWreckT}}, {3,3,4,4,4,12,{&mWMast,&mWMast,&mWMast,&mWMast,&mWMast}} };
static const u16 pShell[3]={RGB(31,24,24),RGB(28,15,18),RGB(31,30,28)};
MAT(mShellT,pShell,2,2,"cb","ba")
MAT(mShellS,pShell,1,1,"b")
static const IBox bxShell[1]={ {3,3,5,5,0,1,{&mShellS,&mShellS,&mShellS,&mShellS,&mShellT}} };
static const u16 pBush[4]={RGB(5,12,4),RGB(8,18,6),RGB(12,24,9),RGB(28,4,6)};
MAT(mBushS,pBush,6,5,"bcbdbc","cbcbcd","bdcbcb","cbcbdb","abaaba")
MAT(mBushT,pBush,6,6,"bcbcdb","cdcbcb","bcbcbd","dbcbcb","bcdbcb","cbcbdc")
static const IBox bxBush[1]={ {1,1,7,7,0,5,{&mBushS,&mBushS,&mBushS,&mBushS,&mBushT}} };
static const u16 pNet[3]={RGB(10,7,4),RGB(18,12,6),RGB(26,24,18)};
MAT(mPost,pNet,1,1,"b")
MAT(mNet,pNet,2,4,"c.",".c","c.",".c")
static const IBox bxFishing[3]={ {3,3,4,4,0,10,{&mPost,&mPost,&mPost,&mPost,&mPost}}, {3,3,7,4,9,10,{&mPost,&mPost,&mPost,&mPost,&mPost}},
                                 {6,3,7,4,3,9,{&mNet,&mNet,&mNet,&mNet,&mNet}} };
static const u16 pRaft[4]={RGB(10,7,4),RGB(17,12,6),RGB(22,16,8),RGB(28,28,26)};
MAT(mRaftT,pRaft,8,6,"cccccccc","bbbbbbbb","aaaaaaaa","cccccccc","bbbbbbbb","aaaaaaaa")
MAT(mRaftS,pRaft,8,2,"bcbcbcbc","abababab")
MAT(mRaftMast,pRaft,1,1,"a")
MAT(mRaftSail,pRaft,3,6,"d..","dd.","ddd","ddd","dd.","d..")
static const IBox bxRaft[3]={ {0,1,8,7,0,2,{&mRaftS,&mRaftS,&mRaftS,&mRaftS,&mRaftT}}, {3,3,4,4,2,11,{&mRaftMast,&mRaftMast,&mRaftMast,&mRaftMast,&mRaftMast}},
                              {4,3,7,4,4,11,{&mRaftSail,&mRaftSail,&mRaftSail,&mRaftSail,&mRaftSail}} };
static const u16 pGold[5]={RGB(12,12,12),RGB(18,18,17),RGB(31,26,6),RGB(26,18,2),RGB(31,31,20)};
MAT(mPlinth,pGold,1,1,"b")
MAT(mPlinthT,pGold,1,1,"a")
MAT(mGoldF,pGold,2,6,"cc","ed","cc","dd","cc","dc")
MAT(mGoldT,pGold,2,2,"ec","cd")
static const IBox bxGold[2]={ {2,2,6,6,0,4,{&mPlinth,&mPlinth,&mPlinth,&mPlinth,&mPlinthT}}, {3,3,5,5,4,11,{&mGoldF,&mGoldF,&mGoldF,&mGoldF,&mGoldT}} };
static const u16 pJour[5]={RGB(10,7,4),RGB(16,11,6),RGB(22,16,9),RGB(24,6,6),RGB(28,26,20)};
MAT(mStumpS,pJour,1,1,"b")
MAT(mStumpT,pJour,4,4,"bccb","cbbc","cbbc","bccb")
MAT(mBookT,pJour,2,2,"dd","de")
MAT(mBookS,pJour,1,1,"e")
static const IBox bxJournal[2]={ {2,2,6,6,0,3,{&mStumpS,&mStumpS,&mStumpS,&mStumpS,&mStumpT}}, {3,3,5,5,3,4,{&mBookS,&mBookS,&mBookS,&mBookS,&mBookT}} };

typedef struct { char ch; const char *name; const IBox *b; u8 n; const IBox *bv; u8 nv; u8 flags; u16 cost; } ItemDef;
static ItemDef defs[] = {   // (not const: items_init sets some of them to life size)
    { '#', "CRATE", bxCrate, 1, 0, 0, F_HIGH, 40 },
    { 'F', "FRIDGE", bxFridge, 1, 0, 0, F_ROT, 600 },
    { 'T', "TOILET", bxToilet, 3, 0, 0, F_ROT, 300 },
    { '=', "RAIL", bxRailU, 3, bxRailV, 3, F_ALONG, 80 },
    { 'L', "LEDGE", bxLedgeU, 1, bxLedgeV, 1, F_ALONG, 60 },
    { 'N', "BENCH", bxH12, 6, bxHV12, 6, F_ALONG, 120 },
    { 'J', "JERSEY", bxJerseyU, 3, bxJerseyV, 3, F_ALONG, 60 },
    { 'X', "FUNBOX", bxFunbox, 1, 0, 0, F_HIGH, 200 },
    { 'O', "BARREL", bxBarrel, 1, 0, 0, 0, 30 },
    { 'Y', "TRASH CAN", bxTrashCan, 2, 0, 0, 0, 30 },
    { '\x82', "FUEL PUMP", bxPump, 2, 0, 0, 0, 60 },
    { 'Z', "PLANTER", bxPlanter, 1, 0, 0, 0, 90 },
    { 'K', "PICNIC", bxPicnicTable, 7, 0, 0, 0, 250 },
    { 'M', "MANUAL PAD", bxManualPad, 1, 0, 0, F_FLAT, 120 },
    { '1', "KICKER", bxKicker, 8, 0, 0, F_ROT, 300 },
    { '5', "Q PIPE", bxQuarterPipe, 8, 0, 0, F_ROT, 500 },
    { '9', "LAUNCH", bxLaunch, 8, 0, 0, F_ROT, 400 },
    { 'S', "BED", bxB0, 12, 0, 0, F_ROT, 700 },
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
    { 'B', "SKATEBOARD", bxBoard, 5, 0, 0, F_FLAT, 120 },
    { '|', "STAIRS", bxBoard, 1, 0, 0, F_ROT, 400 },
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
    { '3', "OAK TREE", bxOak, 3, 0, 0, 0, 60 },
    { '6', "PINE TREE", bxPine, 4, 0, 0, 0, 60 },
    { '7', "SHOP SHELF", bxShelfG, 1, 0, 0, F_ROT, 200 },
    { '8', "FOR SALE SIGN", bxSign, 2, 0, 0, 0, 10 },
    { '~', "POOL LOUNGER", bxLounger, 2, 0, 0, F_ROT, 90 },
    { '"', "STREET LAMP", bxStreetLamp, 2, 0, 0, F_ROT | F_KERB, 120 },
    { CH_CRIB, "CRIB", bxCrib, 9, 0, 0, F_ROT, 250 },
    { CH_CAKE, "BIRTHDAY CAKE", bxCake, 8, 0, 0, 0, 40 },
    { CH_GRAVE, "GRAVESTONE", bxGrave, 3, 0, 0, F_ROT, 0 },
    { CH_BOTTLE, "MESSAGE IN A BOTTLE", bxBottle, 2, 0, 0, F_FLAT, 0 },
    { CH_DIG, "SANDY MOUND", bxDig, 2, 0, 0, F_FLAT, 0 },
    { CH_IDOL, "TIKI IDOL", bxIdol, 2, 0, 0, 0, 0 },
    { CH_WRECK, "SHIPWRECK", bxWreck, 2, 0, 0, 0, 0 },
    { CH_SHELL, "SEASHELL", bxShell, 1, 0, 0, F_FLAT, 0 },
    { CH_BUSH, "BERRY BUSH", bxBush, 1, 0, 0, 0, 30 },
    { CH_FISH, "FISHING SPOT", bxFishing, 3, 0, 0, 0, 0 },
    { CH_RAFT, "RAFT", bxRaft, 3, 0, 0, 0, 0 },
    { CH_GOLD, "GOLDEN IDOL", bxGold, 2, 0, 0, 0, 0 },
    { CH_JOURNAL, "OLD JOURNAL", bxJournal, 2, 0, 0, 0, 0 },
    ITEMS_MORE
    ITEMS_GOTCHI
    ITEMS_PARTY
    ITEMS_BAND
    ITEMS_SPACE
    ITEMS_FARM
    ITEMS_HOME
    ITEMS_BED
    ITEMS_DC
};
#define NDEFS ((int)(sizeof(defs) / sizeof(defs[0])))
_Static_assert(sizeof(defs) / sizeof(defs[0]) < 255, "def_ix holds a def + 1 in a byte");
static u8 def_ix[256], def_ix_ok;                             // a letter's def + 1 (0: not a thing), filled the first time it is asked
static const ItemDef *def_of(char ch) {
    if (!def_ix_ok) { def_ix_ok = 1;                                       // (once: the walkers and the drawing ask this thousands of times a frame)
        for (int i = NDEFS - 1; i >= 0; i--) def_ix[(u8)defs[i].ch] = (u8)(i + 1);
        def_ix['2'] = def_ix['4'] = def_ix['1']; def_ix[0] = 0;              // a kicker turned (the skate park spells its turn in the letter;
    }                                                         // '3' '6' '7' '8' and '~' are the oak, the pine, the shop shelf, the sign, the lounger)
    int k = def_ix[(u8)ch];
    return k ? &defs[k - 1] : 0;
}
int items_count(void) { return NDEFS; }
char items_at(int i) { return defs[i].ch; }
int items_cost(char ch) { const ItemDef *d = def_of(ch); return d ? d->cost : 0; }
int items_is_item(char ch) { return def_of(ch) != 0; }
int items_blocking(char ch) { const ItemDef *d = def_of(ch); return d && !(d->flags & (F_FLAT | F_KERB)); }
int items_rotates(char ch) { const ItemDef *d = def_of(ch); return d && (d->flags & F_ROT); }
int items_foot(char ch, int *w, int *l) {   // the tiles a thing covers (w across, l along its facing); 1 when it covers more than its own tile
    const ItemDef *d = def_of(ch); *w = *l = 1; if (!d) return 0;
    if (d->flags & F_W2) *w = 2; if (d->flags & F_L2) *l = 2;
    return *w * *l > 1;
}
int items_along(char ch) { const ItemDef *d = def_of(ch); return d && (d->flags & F_ALONG); }
const char *items_name(char ch) { const ItemDef *d = def_of(ch); return d ? d->name : "?"; }
int items_height(char ch) {
    const ItemDef *d = def_of(ch); if (!d) return 0;
    int top = 0; for (int i = 0; i < d->n; i++) if (d->b[i].z1 > top) top = d->b[i].z1;
    return top * IZ;
}

// ---- the material cache: every Mat becomes one texture in the atlas ----
#define MAXMAT 900
static const Mat *mat_key[MAXMAT];
static TexRef mat_tex[MAXMAT];
static int nmat;
static TexRef mat_ref(const Mat *m) {
    for (int i = 0; i < nmat; i++) if (mat_key[i] == m) return mat_tex[i];
    int w = m->w, h = m->h; u16 *mbuf = atlas_scratch_hi();   // (up to 32 x 32)
    if (w > 32) w = 32; if (h > 32) h = 32;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        char ch = m->rows[y][x];
        u16 c = (ch == '.') ? 0 : m->pal[ch - 'a'];
        if (ch != '.' && c == 0) c = 0x8000;
        mbuf[y * w + x] = c;
    }
    TexRef t = atlas_add_spill(w, h, mbuf);
    if (nmat < MAXMAT) { mat_key[nmat] = m; mat_tex[nmat] = t; nmat++; }
    return t;
}
// every box face's texture, resolved once: box_ref[box][face] is its material + 1 in mat_tex (0: no face there)
#define MAXBOX 1000
static u16 box_ref[MAXBOX][5];
static s16 box_base[NDEFS][2];     // first box index of each def's main / along-y boxes
static int nbox;
static const TexRef no_tex = { 0, 0, 0, 0, 0 };
static inline const TexRef *box_tex(const u16 *ri, int f) { return ri[f] ? &mat_tex[ri[f] - 1] : &no_tex; }
static void cache_boxes(const IBox *b, int n, s16 *base) {
    *base = (s16)nbox;
    for (int i = 0; i < n && nbox < MAXBOX; i++, nbox++) for (int f = 0; f < 5; f++) {
        box_ref[nbox][f] = 0;
        if (!b[i].m[f]) continue;
        mat_ref(b[i].m[f]);
        for (int k = 0; k < nmat; k++) if (mat_key[k] == b[i].m[f]) { box_ref[nbox][f] = (u16)(k + 1); break; }
    }
}
// LIFE SIZE: the GBA's art was drawn to read on a tiny screen, so some things stood far too tall or too short beside a person (1.8 m,
// 16 px of item height: 1 px = 11 cm). These are set to their real heights; the art is stretched or squashed to fit.
static const struct { char ch; u8 px; } lifeSize[] = {
    { '=', 4 }, { 'L', 4 }, { 'N', 8 }, { 'J', 7 }, { 'X', 4 }, { 'O', 8 }, { 'Y', 8 }, { 'Z', 5 }, { 'K', 7 }, { 'M', 2 },   // the skate park: a rail 45 cm, a funbox 45 cm, a manual pad 20 cm
    { 'h', 9 }, { 'v', 12 }, { 'V', 6 }, { 'H', 18 },                                                                       // a chair 1 m, a TV on its stand 1.35 m, a lava lamp, a shower 2 m
    { '+', 22 }, { '"', 40 }, { 'k', 28 }, { 'a', 28 }, { 'g', 40 }, { '3', 36 }, { '6', 40 }, { 'u', 14 }, { 'z', 11 },
    { '%', 4 }, { '}', 20 }, { '>', 8 }, { '_', 28 }, { (char)0xB5, 11 }, { (char)0xE0, 11 }, { (char)0xB8, 8 }, { (char)0xB9, 24 } };   // a park bench 90 cm (it stood 45), a hot tub 90, a hoop 3 m, a mailbox 1.2 m, a trampoline 90, a swing set 2.6 m   // a bus stop 2.5 m, a street lamp 4.5 m, the flags, the trees 4 to 4.5 m
static IBox lifeBox[192]; static int nlifeBox;
static void life_size(void) {
    for (unsigned k = 0; k < sizeof lifeSize / sizeof lifeSize[0]; k++)
        for (int i = 0; i < NDEFS; i++) {
            if (defs[i].ch != lifeSize[k].ch) continue;
            for (int pass = 0; pass < 2; pass++) {
                const IBox *src = pass ? defs[i].bv : defs[i].b; int nb = pass ? defs[i].nv : defs[i].n;
                if (!src || nlifeBox + nb > (int)(sizeof lifeBox / sizeof lifeBox[0])) continue;
                int top = 1; for (int j = 0; j < nb; j++) top = MAX(top, src[j].z1);
                IBox *dst = &lifeBox[nlifeBox]; nlifeBox += nb;
                for (int j = 0; j < nb; j++) { dst[j] = src[j]; int z0 = (src[j].z0 * lifeSize[k].px + top / 2) / top, z1 = (src[j].z1 * lifeSize[k].px + top / 2) / top;
                    if (z1 <= z0) z1 = z0 + 1; dst[j].z0 = (u8)z0; dst[j].z1 = (u8)z1; }
                if (pass) defs[i].bv = dst; else defs[i].b = dst;
            }
        }
}
COLD void items_init(void) {
    nmat = 0; nbox = 0;
    if (!nlifeBox) life_size();
    for (int i = 0; i < NDEFS; i++) {
        cache_boxes(defs[i].b, defs[i].n, &box_base[i][0]); if (defs[i].bv) cache_boxes(defs[i].bv, defs[i].nv, &box_base[i][1]); else box_base[i][1] = -1;
    }
}

// ---- drawing ----
static void rotPt(int r, int a, int b, int *u, int *v) {
    switch (r & 3) { case 0: *u = a; *v = b; break; case 1: *u = b; *v = 8 - a; break; case 2: *u = 8 - a; *v = 8 - b; break; default: *u = 8 - b; *v = a; }
}
static void unrot(int r, int u, int v, int *a, int *b) {   // world (u,v) -> local (a,b)
    switch (r & 3) { case 0: *a = u; *b = v; break; case 1: *a = 8 - v; *b = u; break; case 2: *a = 8 - u; *b = 8 - v; break; default: *a = v; *b = 8 - u; }
}
static int shade_of(int nx, int ny, int nz) { int L = gfx_light(nx, ny, nz); return CLAMP(L, 40, 200); }
// the six axis directions' shades and their graded colours, worked out once a frame (items only ever face along the axes)
static int sh6_fr = -1, sh6[6]; static u32 sh6_top[6], sh6_bot[6], sh6_flat[6];
static void shade6_update(int t) {
    if (t == sh6_fr) return;
    sh6_fr = t;
    static const short n6[6][3] = { { 0, 0, -ONE }, { ONE, 0, 0 }, { 0, 0, ONE }, { -ONE, 0, 0 }, { 0, -ONE, 0 }, { 0, ONE, 0 } };   // S E N W top bottom
    for (int k = 0; k < 6; k++) { sh6[k] = shade_of(n6[k][0], n6[k][1], n6[k][2]); sh6_flat[k] = gfx_grade_tex((u32)sh6[k] * 0x010101u); }
}

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

int world_nc_need(const XV *a, const XV *b, const XV *c, const XV *d);
void world_nc_quad(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, const u32 *col, int tex, u16 tpage, const u32 *uv, int cull);
static void tex_facex(const XV *a, const XV *b, const XV *c, const XV *d, const SVECTOR *sa, const SVECTOR *sb, const SVECTOR *sc, const SVECTOR *sd, const TexRef *tp, int dir) {   // tex_face on the fast path (dir: S E N W)
    const TexRef *t = tp;
    int s = sh6[dir], st = s + (s >> 4), sb_ = (s * box_ao) >> 8;
    u32 ct = gfx_grade_tex((u32)st * 0x010101u), cb = gfx_grade_tex((u32)sb_ * 0x010101u);
    if (world_nc_need(a, b, c, d)) {   // (close to the eye: cut at the near plane and split, so a pole or a sign holds its shape)
        const u32 cc[4] = { ct, ct, cb, cb };
        const u32 uv[4] = { (u32)t->u | ((u32)t->v << 8), (u32)(t->u + t->w - 1) | ((u32)t->v << 8), (u32)t->u | ((u32)(t->v + t->h - 1) << 8), (u32)(t->u + t->w - 1) | ((u32)(t->v + t->h - 1) << 8) };
        world_nc_quad(sa, sb, sc, sd, cc, 1, t->tpage, uv, 1); return;
    }
    if (!xv_vis4(a, b, c, d, 1)) return;
    gfx_put_gt4(a, b, c, d, ct, ct, cb, cb, (u32)t->tpage << 16, 0, t->u, t->v, t->u + t->w - 1, t->v + t->h - 1);
}
static void draw_box(const IBox *bx, const u16 *ref, int r, int ox, int oz, int t) {
    int ua, va, ub, vb; rotPt(r, bx->a0, bx->b0, &ua, &va); rotPt(r, bx->a1, bx->b1, &ub, &vb);
    int u0 = MIN(ua, ub), u1 = MAX(ua, ub), v0 = MIN(va, vb), v1 = MAX(va, vb);
    if (u1 == u0 || v1 == v0) return;
    int x0 = ox + u0 * IUNIT, x1 = ox + u1 * IUNIT;
    int zN = -(oz + v0 * IUNIT), zS = -(oz + v1 * IUNIT);          // north (max world z) and south
    int y0 = -bx->z1 * IZ, y1 = -bx->z0 * IZ;
    if (bx->z1 == bx->z0) y0 = y1 - 2;
    (void)t;
    { int h = bx->z1 - bx->z0; box_ao = bx->z0 == 0 ? (h > 8 ? 176 : 150) : 214; }   // on the floor: a dark foot (stronger on low things); higher up: a soft one
    // world faces: 0 south (+v), 1 east (+u), 2 north (-v), 3 west (-u) show local face (wf - r)
    const TexRef *mS = box_tex(ref, (0 - r) & 3), *mE = box_tex(ref, (1 - r) & 3), *mN = box_tex(ref, (2 - r) & 3), *mW = box_tex(ref, (3 - r) & 3);
    // only the sides that face the eye (the rest could never show)
    int fS = cam.eye.vz < zS && mS->tpage, fE = cam.eye.vx > x1 && mE->tpage, fN = cam.eye.vz > zN && mN->tpage, fW = cam.eye.vx < x0 && mW->tpage;
    int fT = ref[4] && cam.eye.vy < y0;
    if (!(fS | fE | fN | fW | fT)) return;
    // the 8 corners, transformed once (k: bit 0 = x1, bit 1 = south, bit 2 = bottom)
    SVECTOR cv[8]; XV v[8];
    for (int k = 0; k < 8; k++) setVector(&cv[k], (k & 1) ? x1 : x0, (k & 4) ? y1 : y0, (k & 2) ? zS : zN);
    gfx_xform(cv, v, 8);
    if (fS) tex_facex(&v[2], &v[3], &v[6], &v[7], &cv[2], &cv[3], &cv[6], &cv[7], mS, 0);
    if (fE) tex_facex(&v[3], &v[1], &v[7], &v[5], &cv[3], &cv[1], &cv[7], &cv[5], mE, 1);
    if (fN) tex_facex(&v[1], &v[0], &v[5], &v[4], &cv[1], &cv[0], &cv[5], &cv[4], mN, 2);
    if (fW) tex_facex(&v[0], &v[2], &v[4], &v[6], &cv[0], &cv[2], &cv[4], &cv[6], mW, 3);
    // top: TL=(x0,zN) TR=(x1,zN) BL=(x0,zS) BR=(x1,zS); the texture turns with the item
    if (fT) {
        TexRef tt = *box_tex(ref, 4); const SVECTOR *cv_ = cv;
        int topnc = world_nc_need(&v[0], &v[1], &v[2], &v[3]);
        if (!topnc && !xv_vis4(&v[0], &v[1], &v[2], &v[3], 1)) return;
        int uv[4][2]; int cu[4] = { u0, u1, u0, u1 }, cvv[4] = { v0, v0, v1, v1 };
        int aw = bx->a1 - bx->a0, bh = bx->b1 - bx->b0; if (aw < 1) aw = 1; if (bh < 1) bh = 1;
        static const int rcp[9] = { 0, 65536, 32768, 21846, 16384, 13108, 10923, 9363, 8192 };   // 65536 / n (boxes are 1..8 units)
        int ra = aw <= 8 ? rcp[aw] : 65536 / aw, rb = bh <= 8 ? rcp[bh] : 65536 / bh;
        for (int k = 0; k < 4; k++) {
            int la, lb; unrot(r, cu[k], cvv[k], &la, &lb);
            int fu = ((la - bx->a0) * (tt.w - 1) * ra) >> 16, fv = ((lb - bx->b0) * (tt.h - 1) * rb) >> 16;
            uv[k][0] = tt.u + CLAMP(fu, 0, tt.w - 1); uv[k][1] = tt.v + CLAMP(fv, 0, tt.h - 1);
        }
        if (topnc) { const u32 cc[4] = { sh6_flat[4], sh6_flat[4], sh6_flat[4], sh6_flat[4] }; const u32 uvp[4] = { (u32)uv[0][0] | ((u32)uv[0][1] << 8), (u32)uv[1][0] | ((u32)uv[1][1] << 8), (u32)uv[2][0] | ((u32)uv[2][1] << 8), (u32)uv[3][0] | ((u32)uv[3][1] << 8) };
            world_nc_quad(&cv_[0], &cv_[1], &cv_[2], &cv_[3], cc, 1, tt.tpage, uvp, 1); return; }
        gfx_emit_ft4(&v[0], &v[1], &v[2], &v[3], sh6_flat[4], tt.tpage, 0,
                     uv[0][0], uv[0][1], uv[1][0], uv[1][1], uv[2][0], uv[2][1], uv[3][0], uv[3][1], 1, 0);
    }
}

int getenv_noshadow;

// ---- THE SKATEBOARD, modelled (the GBA's was a brick on four cubes): a thin 7-ply deck with a kicked nose and tail,
// griptape on top, the graphic underneath, two silver trucks and four urethane wheels ----
static void skate_model(void);
static int sk_k = 16;   // scale /16 (13: a real board, 0.85 m long)
static void sk_quad(SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d, int r, int g, int bl, int nx, int ny, int nz) {
    if (sk_k != 16) { SVECTOR *q[4] = { a, b, c, d }; for (int i = 0; i < 4; i++) { q[i]->vx = (s16)(q[i]->vx * sk_k >> 4); q[i]->vy = (s16)(q[i]->vy * sk_k >> 4); q[i]->vz = (s16)(q[i]->vz * sk_k >> 4); } }
    int L = gfx_light(nx, ny, nz);
    gfx_quad_f(a, b, c, d, MIN(255, (r * L) >> 7), MIN(255, (g * L) >> 7), MIN(255, (bl * L) >> 7), 1);
}
static __attribute__((noinline)) void sk_face(const XV *A, const XV *B, const XV *C, const XV *DQ, const s16 *n, int r, int g, int b) {   // (sk_box's fast path: one face, if it shows)
    if (!xv_vis4(A, B, C, DQ, 1)) return;
    int L = gfx_light(n[0], n[1], n[2]);
    gfx_put_f4(A, B, C, DQ, gfx_grade_flat((u32)MIN(255, (r * L) >> 7) | ((u32)MIN(255, (g * L) >> 7) << 8) | ((u32)MIN(255, (b * L) >> 7) << 16)));
    gfx_prim_count++;
}
static void sk_box(int x0, int x1, int y0, int y1, int z0, int z1, int r, int g, int b) {   // y0 top (smaller), z0 < z1
    if (!gfx_bb_on) {   // the fast path: the 8 corners transformed once (not 4 for each of the 5 faces), each face tested and coloured only if it shows
        static const u8 fc[5][4] = { { 0, 1, 2, 3 }, { 1, 5, 3, 7 }, { 5, 4, 7, 6 }, { 4, 0, 6, 2 }, { 4, 5, 0, 1 } };   // (the faces below, in their order; k: bit 0 x1, bit 1 bottom, bit 2 z1)
        static const s16 fn[5][3] = { { 0, 0, -ONE }, { ONE, 0, 0 }, { 0, 0, ONE }, { -ONE, 0, 0 }, { 0, -ONE, 0 } };
        const s16 X[2] = { (s16)(x0 * sk_k >> 4), (s16)(x1 * sk_k >> 4) }, Y[2] = { (s16)(y0 * sk_k >> 4), (s16)(y1 * sk_k >> 4) }, Z[2] = { (s16)(z0 * sk_k >> 4), (s16)(z1 * sk_k >> 4) };
        SVECTOR cv[8] SVEC_ALIGN; XV v[8];
        for (int k = 0; k < 8; k++) setVector(&cv[k], X[k & 1], Y[(k >> 1) & 1], Z[k >> 2]);
        gfx_xform(cv, v, 8);
        for (int f = 0; f < 5; f++) sk_face(&v[fc[f][0]], &v[fc[f][1]], &v[fc[f][2]], &v[fc[f][3]], fn[f], r, g, b);
        return;
    }
    SVECTOR A, B, C, DD;
    setVector(&A, x0, y0, z0); setVector(&B, x1, y0, z0); setVector(&C, x0, y1, z0); setVector(&DD, x1, y1, z0); sk_quad(&A, &B, &C, &DD, r, g, b, 0, 0, -ONE);
    setVector(&A, x1, y0, z0); setVector(&B, x1, y0, z1); setVector(&C, x1, y1, z0); setVector(&DD, x1, y1, z1); sk_quad(&A, &B, &C, &DD, r, g, b, ONE, 0, 0);
    setVector(&A, x1, y0, z1); setVector(&B, x0, y0, z1); setVector(&C, x1, y1, z1); setVector(&DD, x0, y1, z1); sk_quad(&A, &B, &C, &DD, r, g, b, 0, 0, ONE);
    setVector(&A, x0, y0, z1); setVector(&B, x0, y0, z0); setVector(&C, x0, y1, z1); setVector(&DD, x0, y1, z0); sk_quad(&A, &B, &C, &DD, r, g, b, -ONE, 0, 0);
    setVector(&A, x0, y0, z1); setVector(&B, x1, y0, z1); setVector(&C, x0, y0, z0); setVector(&DD, x1, y0, z0); sk_quad(&A, &B, &C, &DD, r, g, b, 0, -ONE, 0);
}
static void sk_slab(int w, int z0, int ya, int z1, int yb, int T) {   // one deck section from z0 (top at ya) to z1 (top at yb)
    SVECTOR A, B, C, DD; int x0 = -w, x1 = w;
    setVector(&A, x0, yb, z1); setVector(&B, x1, yb, z1); setVector(&C, x0, ya, z0); setVector(&DD, x1, ya, z0); sk_quad(&A, &B, &C, &DD, 46, 46, 54, 0, -ONE, 0);        // griptape
    setVector(&A, x0, ya + T, z0); setVector(&B, x1, ya + T, z0); setVector(&C, x0, yb + T, z1); setVector(&DD, x1, yb + T, z1); sk_quad(&A, &B, &C, &DD, 240, 104, 40, 0, ONE, 0);   // the graphic
    setVector(&A, x1, ya, z0); setVector(&B, x1, yb, z1); setVector(&C, x1, ya + T, z0); setVector(&DD, x1, yb + T, z1); sk_quad(&A, &B, &C, &DD, 222, 184, 124, ONE, 0, 0);    // the ply edge
    setVector(&A, x0, yb, z1); setVector(&B, x0, ya, z0); setVector(&C, x0, yb + T, z1); setVector(&DD, x0, ya + T, z0); sk_quad(&A, &B, &C, &DD, 222, 184, 124, -ONE, 0, 0);
}
static void skate_item(int mx, int my, int r) {
    VECTOR p; setVector(&p, mx * TILE + TILE / 2, 0, -(my * TILE + TILE / 2));
    gfx_set_object(&p, (r & 1) * 1024);
    { SVECTOR A, B, C, DD; setVector(&A, -40, -2, 100); setVector(&B, 40, -2, 100); setVector(&C, -40, -2, -100); setVector(&DD, 40, -2, -100); gfx_shadow_quad(&A, &B, &C, &DD); }
    sk_k = 13; skate_model(); sk_k = 16;
    gfx_set_identity();
}
void items_draw_board(int x, int z, int yaw, int y, int roll, int pitch, int spin) {   // the ridden board, turned about its middle by a flip (roll: about its length; pitch: across; spin: a shove-it)
    if (!roll && !pitch) { VECTOR p; setVector(&p, x, y, z); gfx_set_object(&p, (yaw + spin) & 4095); sk_k = 13; skate_model(); sk_k = 16; gfx_set_identity(); return; }
    MATRIX my_, mf, m; SVECTOR ry = { 0, (short)((yaw + spin) & 4095), 0, 0 }, rf = { (short)pitch, 0, (short)roll, 0 };
    RotMatrix(&ry, &my_); RotMatrix(&rf, &mf); MulMatrix0(&my_, &mf, &m);
    {   // (the turn is about the board's middle, 14 units up, not its wheels: t = pos + c - R c, c = (0, -14, 0))
      int ox_ = (m.m[0][1] * -14) >> 12, oy_ = (m.m[1][1] * -14) >> 12, oz_ = (m.m[2][1] * -14) >> 12;
      m.t[0] = x - ox_; m.t[1] = y - 14 - oy_; m.t[2] = z - oz_; }
    gfx_set_object_m(&m); gfx_set_light_yaw((yaw + spin) & 4095);
    sk_k = 13; skate_model(); sk_k = 16;
    gfx_set_identity();
}
// ---- VEHICLES: toy-scale models in the game's voxel/box style (the driver sits at the origin, the model's front is +z, up is -y; the world scale is 28/16) ----
// A car is a table of boxes in ten tones (body, a darker lower tone, an accent for the roof and stripes, glass, trim, lamps ...), a sloped
// cabin (windscreen, side windows, rear window and roof) and four wheels with a rim and spokes that turn with the road. A far car (veh_lod)
// keeps the same footprint and height in a handful of boxes. Under a driver the car is open (no roof) so the driver shows.
// The game's yaw faces south at 0 and a vehicle walks (sin, -cos) like a Bore: the models are turned by 2048 - yaw (voxel.c's MODEL_ANGLE).
static const u8 vcol[6][3] = { { 200, 48, 48 }, { 56, 96, 190 }, { 232, 196, 60 }, { 70, 150, 90 }, { 236, 236, 232 }, { 40, 40, 48 } };
int items_veh_open;                                       // set by the caller while the vehicle under the driver is drawn: no roof
static int veh_lod, veh_cs;                               // 1: far, simple model; which side the camera is on (-1 the model's -x side, 1 its +x, 0 above)
static u8 vtone[10][3];
enum { K_BODY, K_LOW, K_ACC, K_GLASS, K_DARK, K_CHROME, K_HEAD, K_TAIL, K_WHITE, K_SEAT };
enum { F_MIR = 1, F_OPEN = 2, F_NEAR = 4, F_FAR = 8 };    // mirror across x; only with the top open; only up close; only far off
typedef struct { s16 x0, x1, y0, y1, z0, z1; u8 k, f; } VB;
#define VBX(x0, x1, y0, y1, z0, z1, k, f) { x0, x1, y0, y1, z0, z1, k, f }
static void veh_tones(int ci) {
    const u8 *c = vcol[ci % 6]; int i;
    for (i = 0; i < 3; i++) { vtone[K_BODY][i] = c[i]; vtone[K_LOW][i] = c[i] * 7 / 10; vtone[K_ACC][i] = (ci & 1) ? 232 - i * 4 : 46 + i * 3; }
    static const u8 fix[7][3] = { { 130, 176, 214 }, { 38, 38, 44 }, { 196, 202, 214 }, { 255, 238, 160 }, { 224, 40, 40 }, { 236, 238, 244 }, { 54, 50, 62 } };
    for (i = 0; i < 7; i++) for (int j = 0; j < 3; j++) vtone[K_GLASS + i][j] = fix[i][j];
}
static void veh_box(int k, int x0, int x1, int y0, int y1, int z0, int z1) { sk_box(x0, x1, y0, y1, z0, z1, vtone[k][0], vtone[k][1], vtone[k][2]); }
static void veh_rows(const VB *v, int n) {
    for (; n > 0; n--, v++) {
        int f = v->f;
        if (((f & F_OPEN) && !items_veh_open) || ((f & F_NEAR) && veh_lod) || ((f & F_FAR) && !veh_lod)) continue;
        veh_box(v->k, v->x0, v->x1, v->y0, v->y1, v->z0, v->z1);
        if (f & F_MIR) veh_box(v->k, -v->x1, -v->x0, v->y0, v->y1, v->z0, v->z1);
    }
}
static void sk_q(int k, int nx, int ny, int nz, int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, int dx, int dy, int dz) {   // a free quad (corners TL TR BL BR as seen from outside)
    SVECTOR A, B, C, DD; setVector(&A, ax, ay, az); setVector(&B, bx, by, bz); setVector(&C, cx, cy, cz); setVector(&DD, dx, dy, dz);
    sk_quad(&A, &B, &C, &DD, vtone[k][0], vtone[k][1], vtone[k][2], nx, ny, nz);
}
// a strip on the plane x (facing out, x < 0 the other way), from (za, ya) to (zb, yb), w across each side of the line: a spoke, the side of a tube
static void veh_strip(int k, int x, int za, int ya, int zb, int yb, int w) {
    int dz = zb - za, dy = yb - ya, l = SquareRoot0(dz * dz + dy * dy); if (l < 1) l = 1;
    int pz = -dy * w / l, py = dz * w / l;     // (the line turned a quarter)
    if (x > 0) sk_q(k, ONE, 0, 0, x, ya - py, za - pz, x, yb - py, zb - pz, x, ya + py, za + pz, x, yb + py, zb + pz);
    else       sk_q(k, -ONE, 0, 0, x, yb - py, zb - pz, x, ya - py, za - pz, x, yb + py, zb + pz, x, ya + py, za + pz);
}
// a thin tube from (za, ya) to (zb, yb) (a bike's frame): both sides and the top
COLD static void veh_tube(int k, int za, int ya, int zb, int yb, int w) {
    if (zb < za) { int t = za; za = zb; zb = t; t = ya; ya = yb; yb = t; }   // (rear end first)
    veh_strip(k, w, za, ya, zb, yb, w + 1); veh_strip(k, -w, za, ya, zb, yb, w + 1);
    sk_q(k, 0, -ONE, 0, -w, yb - w - 1, zb, w, yb - w - 1, zb, -w, ya - w - 1, za, w, ya - w - 1, za);
}
// a wheel: a tyre (two boxes make an octagon), then, on the side that shows, a rim, spokes and a hub cap that turn (wph: 4096 a turn at the reference radius 32)
COLD static void veh_wheel(int x, int z, int R, int wph, int tw) {
    int s = x < 0 ? -1 : 1, ang = (wph * 32 / R) & 4095, c = R * 3 / 10;
    if (veh_lod) { sk_box(x - tw, x + tw, -2 * R, 0, z - R, z + R, 30, 30, 34); return; }
    sk_box(x - tw, x + tw, -2 * R, 0, z - R + c, z + R - c, 30, 30, 34); sk_box(x - tw, x + tw, -2 * R + c, -c, z - R, z + R, 30, 30, 34);
    if (veh_cs && veh_cs != s) return;           // (the side that does not show: the tyre alone)
    int xo = x + s * tw, r2 = R * 6 / 10;
    sk_box(s > 0 ? xo : xo - 2, s > 0 ? xo + 2 : xo, -R - r2, -R + r2, z - r2, z + r2, 190, 196, 208);
    for (int k = 0; k < 2; k++) { int a = (ang + k * 1024) & 4095, dy = (isin(a) * (r2 - 1)) >> 12, dz = (icos(a) * (r2 - 1)) >> 12;
        veh_strip(K_DARK, xo + s * 3, z - dz, -R - dy, z + dz, -R + dy, 3); }
    sk_box(s > 0 ? xo : xo - 5, s > 0 ? xo + 5 : xo, -R - 4, -R + 4, z - 4, z + 4, 236, 236, 230);
}
static void veh_wheels(int R, int x, int zr, int zf, int wph, int tw) {
    veh_wheel(-x, zr, R, wph, tw); veh_wheel(x, zr, R, wph, tw); veh_wheel(-x, zf, R, wph, tw); veh_wheel(x, zf, R, wph, tw);
}
// the cabin: its base (ybase) runs from zr0 to zf0, its top (ytop) from zr1 to zf1 (a windscreen and a rear window that slope), half a width hw; the roof is the accent tone
COLD static void veh_cabin(int hw, int ybase, int ytop, int zr0, int zr1, int zf1, int zf0) {
    int nz = 2400, ny = -3000;
    if (veh_lod) { veh_box(K_GLASS, -hw, hw, ytop + 7, ybase, (zr0 + zr1) / 2, (zf0 + zf1) / 2); veh_box(K_ACC, -hw, hw, ytop, ytop + 7, zr1 - 6, zf1 + 4); return; }   // (far: glass and a roof, one box each)
    sk_q(K_GLASS, 0, ny, nz, hw, ytop, zf1, -hw, ytop, zf1, hw, ybase, zf0, -hw, ybase, zf0);                  // the windscreen
    if (items_veh_open) {   // open: a low frame at the windscreen, no roof; the driver shows
        veh_box(K_BODY, -hw, -hw + 6, ytop, ybase, zf1 - 2, zf0 + 2); veh_box(K_BODY, hw - 6, hw, ytop, ybase, zf1 - 2, zf0 + 2);
        return; }
    sk_q(K_GLASS, 0, ny, -nz, -hw, ytop, zr1, hw, ytop, zr1, -hw, ybase, zr0, hw, ybase, zr0);                // the rear window
    sk_q(K_GLASS, ONE, 0, 0, hw, ytop, zr1, hw, ytop, zf1, hw, ybase, zr0, hw, ybase, zf0);                   // the side windows
    sk_q(K_GLASS, -ONE, 0, 0, -hw, ytop, zf1, -hw, ytop, zr1, -hw, ybase, zf0, -hw, ybase, zr0);
    { int zm = (zr1 + zf1) / 2 - 6;                                                                            // the middle pillar
      veh_box(K_BODY, hw - 1, hw + 1, ytop, ybase, zm - 4, zm + 4); veh_box(K_BODY, -hw - 1, -hw + 1, ytop, ybase, zm - 4, zm + 4); }
    veh_box(K_ACC, -hw, hw, ytop - 7, ytop, zr1 - 6, zf1 + 4);                                                // the roof
}
// body tables: z runs from the tail (-) to the nose (+)
static const VB B_SEDAN[] = {
    VBX(-104, 104, -34, -14, -226, 234, K_DARK, 0),                                       // the underside between the wheels
    VBX(-106, 106, -64, -34, -232, 240, K_BODY, 0),                                       // the lower body
    VBX(-107, 107, -40, -30, -190, 206, K_LOW, F_NEAR),                                   // the sills
    VBX(-99, 99, -72, -64, 82, 238, K_BODY, F_NEAR),                                      // the bonnet
    VBX(-99, 99, -70, -64, -230, -90, K_BODY, F_NEAR),                                    // the boot
    VBX(-100, 100, -54, -38, 238, 249, K_CHROME, F_NEAR), VBX(-100, 100, -54, -38, -246, -234, K_CHROME, F_NEAR),   // the bumpers
    VBX(-48, 48, -62, -50, 238, 244, K_DARK, F_NEAR),                                     // the grille
    VBX(58, 98, -64, -50, 240, 246, K_HEAD, F_NEAR | F_MIR), VBX(60, 100, -62, -48, -242, -234, K_TAIL, F_NEAR | F_MIR),
    VBX(106, 108, -62, -34, -34, -30, K_DARK, F_NEAR | F_MIR), VBX(106, 108, -62, -34, 66, 70, K_DARK, F_NEAR | F_MIR),   // the door seams
    VBX(104, 116, -96, -86, 62, 72, K_DARK, F_NEAR | F_MIR),                              // the mirrors
    VBX(-70, 70, -86, -64, -60, 22, K_SEAT, F_OPEN | F_NEAR), VBX(-70, 70, -116, -86, -60, -44, K_SEAT, F_OPEN | F_NEAR),   // the seat
    VBX(-70, 70, -86, -64, -150, -90, K_SEAT, F_OPEN | F_NEAR), VBX(-70, 70, -112, -86, -150, -134, K_SEAT, F_OPEN | F_NEAR),
    VBX(-92, 92, -88, -66, 40, 66, K_DARK, F_OPEN | F_NEAR),
};
static const VB B_SPORT[] = {
    VBX(-106, 106, -28, -12, -204, 236, K_DARK, 0),
    VBX(-108, 108, -58, -26, -206, 244, K_BODY, 0),                                       // low and wide
    VBX(-109, 109, -36, -26, -170, 200, K_LOW, F_NEAR),
    VBX(-94, 94, -64, -58, 90, 238, K_BODY, F_NEAR),                                      // a low bonnet
    VBX(-94, 94, -64, -58, -200, -110, K_BODY, F_NEAR),
    VBX(-14, 14, -66, -64, -200, 238, K_ACC, F_NEAR),                                     // a racing stripe
    VBX(-100, 100, -42, -26, 244, 255, K_DARK, F_NEAR), VBX(-100, 100, -44, -28, -216, -206, K_DARK, F_NEAR),
    VBX(-60, 60, -54, -42, 244, 250, K_DARK, F_NEAR),
    VBX(52, 102, -56, -46, 244, 250, K_HEAD, F_NEAR | F_MIR), VBX(50, 104, -52, -42, -216, -208, K_TAIL, F_NEAR | F_MIR),
    VBX(-90, 90, -96, -90, -210, -170, K_ACC, F_NEAR), VBX(-72, -64, -90, -58, -206, -190, K_DARK, F_NEAR), VBX(64, 72, -90, -58, -206, -190, K_DARK, F_NEAR),   // the wing
    VBX(108, 118, -82, -74, 80, 90, K_DARK, F_NEAR | F_MIR),
    VBX(-62, 62, -72, -58, -60, 14, K_SEAT, F_OPEN | F_NEAR), VBX(-62, 62, -96, -72, -60, -44, K_SEAT, F_OPEN | F_NEAR),
    VBX(-84, 84, -78, -60, 32, 56, K_DARK, F_OPEN | F_NEAR),
};
static const VB B_VAN[] = {
    VBX(-106, 106, -36, -14, -300, 176, K_DARK, 0),
    VBX(-108, 108, -90, -34, -304, 176, K_BODY, 0),                                       // the chassis and the cab's lower half
    VBX(-109, 109, -44, -34, -270, 150, K_LOW, F_NEAR),
    VBX(-104, 104, -246, -90, -304, -44, K_WHITE, 0),                                     // the box, white
    VBX(-105, 105, -170, -146, -300, -48, K_ACC, F_NEAR),                                 // a stripe along the box
    VBX(-98, 98, -252, -246, -296, -48, K_LOW, F_NEAR),                                   // the roof
    VBX(-2, 2, -240, -92, -306, -304, K_DARK, F_NEAR), VBX(-60, 60, -150, -128, -307, -304, K_ACC, F_NEAR),     // the doors' seam, a sign
    VBX(-100, 100, -84, -66, 176, 188, K_DARK, F_NEAR),                                   // the front bumper
    VBX(-48, 48, -88, -70, 176, 182, K_DARK, F_NEAR), VBX(56, 98, -88, -74, 178, 184, K_HEAD, F_NEAR | F_MIR),
    VBX(60, 100, -86, -66, -312, -304, K_TAIL, F_NEAR | F_MIR), VBX(-100, 100, -72, -58, -318, -304, K_DARK, F_NEAR),
    VBX(106, 120, -140, -124, 76, 86, K_DARK, F_NEAR | F_MIR),
    VBX(-70, 70, -100, -84, -40, 26, K_SEAT, F_OPEN | F_NEAR), VBX(-70, 70, -134, -100, -40, -26, K_SEAT, F_OPEN | F_NEAR),
    VBX(-86, 86, -102, -88, 50, 86, K_DARK, F_OPEN | F_NEAR),
};
static const VB B_PICK[] = {
    VBX(-106, 106, -36, -14, -278, 224, K_DARK, 0),
    VBX(-108, 108, -86, -34, -280, 232, K_BODY, 0),
    VBX(-109, 109, -46, -34, -250, 200, K_LOW, F_NEAR),
    VBX(-98, 98, -94, -86, -274, -50, K_DARK, F_NEAR),                                    // the bed's floor
    VBX(98, 108, -118, -86, -280, -50, K_BODY, F_NEAR | F_MIR), VBX(-108, 108, -118, -86, -284, -276, K_BODY, F_NEAR), VBX(-108, 108, -118, -86, -56, -48, K_BODY, F_NEAR),   // the bed's sides, tailgate and bulkhead
    VBX(-109, 109, -124, -118, -284, -48, K_ACC, F_NEAR),                                 // the rail
    VBX(76, 98, -104, -94, -190, -140, K_LOW, F_NEAR | F_MIR),                            // the wheel arches inside
    VBX(-104, 104, -78, -62, 232, 242, K_CHROME, F_NEAR), VBX(-104, 104, -78, -62, -292, -284, K_CHROME, F_NEAR),
    VBX(-48, 48, -84, -70, 232, 238, K_DARK, F_NEAR), VBX(62, 100, -86, -72, 234, 240, K_HEAD, F_NEAR | F_MIR), VBX(64, 102, -82, -68, -292, -284, K_TAIL, F_NEAR | F_MIR),
    VBX(108, 120, -128, -112, 80, 92, K_DARK, F_NEAR | F_MIR),
    VBX(-70, 70, -100, -86, -40, 28, K_SEAT, F_OPEN | F_NEAR), VBX(-70, 70, -132, -100, -40, -28, K_SEAT, F_OPEN | F_NEAR),
    VBX(-92, 92, -104, -88, 50, 84, K_DARK, F_OPEN | F_NEAR),
    VBX(-100, 100, -118, -94, -272, -52, K_DARK, F_FAR),                                  // (far: the bed seen as one dark block)
};
// the cabins and the wheels: hw, ybase, ytop, zr0, zr1, zf1, zf0, then the wheel radius, x, rear axle, front axle, tyre half width
static const s16 VCAB[4][12] = {
    { 100, -72, -128, -170, -122, 38, 82, 32, 101, -150, 150, 14 },      // sedan
    { 100, -64, -102, -124, -74, 14, 90, 28, 104, -140, 132, 15 },       // sports
    { 104, -90, -148, -62, -48, 42, 84, 34, 102, -150, 112, 14 },        // van
    { 104, -86, -142, -66, -50, 44, 86, 36, 104, -160, 124, 15 },        // pickup
};
COLD static void veh_car(int t, int ci, int wph) {
    static const VB *const tab[4] = { B_SEDAN, B_SPORT, B_VAN, B_PICK };
    static const u8 nrow[4] = { sizeof B_SEDAN / sizeof(VB), sizeof B_SPORT / sizeof(VB), sizeof B_VAN / sizeof(VB), sizeof B_PICK / sizeof(VB) };
    int m = t == 2 ? 0 : t == 4 ? 1 : t == 3 ? 2 : 3; const s16 *c = VCAB[m];
    veh_tones(ci);
    veh_rows(tab[m], nrow[m]);
    veh_cabin(c[0], c[1], c[2], c[3], c[4], c[5], c[6]);
    veh_wheels(c[7], c[8], c[9], c[10], wph, c[11]);
}
static const VB B_BIKE[] = {
    VBX(-6, 6, -108, -100, -44, -10, K_DARK, 0),                                          // the saddle
    VBX(-30, 30, -118, -112, 52, 62, K_DARK, 0), VBX(26, 34, -122, -112, 50, 64, K_WHITE, F_MIR),   // the bars and grips
    VBX(-9, 9, -30, -26, -24, -16, K_CHROME, 0), VBX(-3, 3, -30, -26, -24, 20, K_CHROME, 0),        // the chain ring and crank
    VBX(-14, -4, -22, -18, 10, 24, K_DARK, 0), VBX(4, 14, -22, -18, -26, -12, K_DARK, 0),           // the pedals
};
static const s8 BIKE_TUBE[7][5] = { { -78, -34, -8, -22, 3 }, { -78, -34, -30, -100, 3 }, { -8, -22, -30, -100, 3 }, { -8, -22, 62, -96, 3 }, { -30, -92, 62, -100, 3 }, { 62, -104, 78, -34, 3 }, { 58, -108, 66, -92, 4 } };
COLD static void veh_bike(int ci, int wph) {   // two spoked wheels, a real frame, a saddle, handlebars, pedals
    veh_tones(ci);
    const int R = 34, c = 10;
    for (int e = 0; e < 2; e++) { int z = e ? 78 : -78, ang = (wph * 32 / R) & 4095;
        for (int k = 0; k < 8; k++) {   // the tyre: a ring of eight boxes (top, bottom, front, back and the four corners)
            int y0, y1, z0, z1;
            switch (k) { case 0: y0 = -2 * R; y1 = y0 + 5; z0 = z - R + c; z1 = z + R - c; break; case 1: y0 = -5; y1 = 0; z0 = z - R + c; z1 = z + R - c; break;
                         case 2: y0 = -2 * R + c; y1 = -c; z0 = z - R; z1 = z0 + 5; break; case 3: y0 = -2 * R + c; y1 = -c; z1 = z + R; z0 = z1 - 5; break;
                         default: y0 = (k & 1) ? -5 - c : -2 * R + 5; y1 = y0 + c; z0 = (k & 2) ? z + R - 5 - c : z - R + 5; z1 = z0 + c; }
            if (veh_lod && k > 3) continue;
            sk_box(-3, 3, y0, y1, z0, z1, 30, 30, 34); }
        if (!veh_lod) for (int s = -1; s <= 1; s += 2) { if (veh_cs && veh_cs != s) continue;
            for (int k = 0; k < 3; k++) { int a = (ang + k * 683) & 4095, dy = (isin(a) * (R - 6)) >> 12, dz = (icos(a) * (R - 6)) >> 12; veh_strip(K_CHROME, s * 4, z - dz, -R - dy, z + dz, -R + dy, 1); } }
        veh_box(K_CHROME, -6, 6, -R - 5, -R + 5, z - 5, z + 5); }
    for (int i = 0; i < 7; i++) { const s8 *t = BIKE_TUBE[i]; veh_tube(i == 5 ? K_ACC : K_BODY, t[0], t[1], t[2], t[3], t[4]); }
    veh_rows(B_BIKE, sizeof B_BIKE / sizeof(VB));
}
static const VB B_SCOOT[] = {
    VBX(-26, 26, -34, -22, -104, 62, K_BODY, 0), VBX(-22, 22, -42, -34, -100, -40, K_DARK, 0), VBX(-22, 22, -50, -42, -96, -44, K_ACC, 0),   // the deck, the seat's base, the seat
    VBX(-24, 24, -48, -28, -112, -102, K_BODY, 0), VBX(-18, 18, -38, -22, 62, 104, K_BODY, 0), VBX(-5, 5, -118, -34, 66, 74, K_CHROME, 0),   // the rear mudguard, the front body, the stem
    VBX(-38, 38, -126, -118, 62, 78, K_DARK, 0), VBX(36, 44, -128, -116, 60, 80, K_WHITE, F_MIR), VBX(-10, 10, -112, -100, 74, 84, K_HEAD, 0), VBX(-12, 12, -34, -28, -112, -104, K_TAIL, 0),
};
COLD static void veh_scooter(int ci, int wph) {
    veh_tones(ci); veh_wheel(0, -86, 22, wph, 8); veh_wheel(0, 90, 22, wph, 8); veh_rows(B_SCOOT, sizeof B_SCOOT / sizeof(VB));
}
#ifdef VEH_SLOPE
#define VGH(x, z) (((x) - cam.at.vx) / 5 + ((z) - cam.at.vz) / 9)   /* (test builds: a made-up hill under the vehicles, to see them tip) */
#else
#define VGH(x, z) gfx_lift_fn((x), (z))
#endif
COLD static int veh_pitch_roll(int x, int z, int ang, int half, int hw, int *pitch, int *roll) {   // the slope under a vehicle: its nose and tail, its two sides (ang: the model's turn)
    int s = isin(ang), c = icos(ang), fx = (s * half) >> 12, fz = (c * half) >> 12, rx = (c * hw) >> 12, rz = -((s * hw) >> 12);   // (the front is (sin, cos), the +x side (cos, -sin))
    int hf = VGH(x + fx, z + fz), hr = VGH(x - fx, z - fz), hp = VGH(x + rx, z + rz), hn = VGH(x - rx, z - rz);
    *pitch = CLAMP((hf - hr) * 652 / (2 * half), -300, 300); *roll = CLAMP(-(hp - hn) * 652 / (2 * hw), -250, 250);
    return *pitch || *roll;
}
int dbg_vdraw, dbg_vq, dbg_vpitch;
COLD void items_draw_vehicle(int x, int z, int yaw, int y, int type, int ci, int wph) {
    dbg_vdraw++; dbg_vq = type * 1000 + ci;
    int up = gfx_lift_fn ? VGH(x, z) : gfx_lift;   // (the same ground gfx_set_object stands it on)
    if (!gfx_bb_on && !gfx_visible(x, y - up - 220, z, 660)) return;   // (off the screen altogether: a car is up to 4.2 m long and 2.5 m high at its scale: a ball of 660 round its middle holds it)
    int ang = (2048 - yaw) & 4095;
    { int ex = cam.eye.vx - x, ez = cam.eye.vz - z, c = icos(ang), s = isin(ang);   // the camera in the model's frame: which side's wheels show, and how far off it is
      int ox = (ex * c - ez * s) >> 12; veh_cs = ox > 120 ? 1 : ox < -120 ? -1 : 0;
      int ey = (cam.eye.vy + y - up) >> 4; ex >>= 4; ez >>= 4; veh_lod = ex * ex + ez * ez + ey * ey > (2800 >> 4) * (2800 >> 4) && !items_veh_open; }
    int pi = 0, ro = 0;
    if (type >= 2 && gfx_lift_fn && !veh_lod && veh_pitch_roll(x, z, ang, 260, 190, &pi, &ro)) {   // on a slope: tipped to follow it, about the middle
        MATRIX my_, mf, m; SVECTOR ry = { 0, (short)ang, 0, 0 }, rf = { (short)pi, 0, (short)ro, 0 };
        RotMatrix(&ry, &my_); RotMatrix(&rf, &mf); MulMatrix0(&my_, &mf, &m);
        m.t[0] = x; m.t[1] = y - up; m.t[2] = z; sk_k = 28; gfx_set_object_m(&m); gfx_set_light_yaw(ang); dbg_vpitch = pi * 1000 + ro;
    } else { VECTOR p; setVector(&p, x, y, z); gfx_set_object(&p, ang); sk_k = 28; }
    switch (type) { case 0: veh_bike(ci, wph); break; case 1: veh_scooter(ci, wph); break; default: veh_car(type, ci, wph); }
    sk_k = 16; gfx_set_identity();
}
static void skate_model(void) {
    const int BW = 34, Lf = 78, L = 112, T = 6, top = -30;
    sk_slab(BW, -Lf, top, Lf, top, T);                                     // the flat middle
    sk_slab(BW - 4, Lf, top, L, top - 15, T);                              // the nose kicks up
    sk_slab(BW - 4, -L, top - 15, -Lf, top, T);                            // and the tail
    { SVECTOR A, B, C, DD;                                                 // the rounded ends
      setVector(&A, BW - 4, top - 15, L); setVector(&B, -(BW - 4), top - 15, L); setVector(&C, BW - 4, top - 15 + T, L); setVector(&DD, -(BW - 4), top - 15 + T, L); sk_quad(&A, &B, &C, &DD, 222, 184, 124, 0, 0, ONE);
      setVector(&A, -(BW - 4), top - 15, -L); setVector(&B, BW - 4, top - 15, -L); setVector(&C, -(BW - 4), top - 15 + T, -L); setVector(&DD, BW - 4, top - 15 + T, -L); sk_quad(&A, &B, &C, &DD, 222, 184, 124, 0, 0, -ONE); }
    for (int e = -1; e <= 1; e += 2) {                                    // the trucks and wheels
        int zc = e * (Lf - 22);
        sk_box(-14, 14, top + T, top + T + 5, zc - 10, zc + 10, 150, 156, 168);     // baseplate
        sk_box(-32, 32, top + T + 5, top + T + 11, zc - 5, zc + 5, 190, 196, 208);  // hanger
        sk_box(-44, -32, -17, 0, zc - 9, zc + 9, 242, 232, 204);                     // wheels
        sk_box(32, 44, -17, 0, zc - 9, zc + 9, 242, 232, 204);
    }
}

static void stairs_item(int mx, int my, int r) {   // a flight of stairs filling one tile: eight steps up a whole storey, rising away from the open side
    VECTOR p; setVector(&p, mx * TILE + TILE / 2, 0, -(my * TILE + TILE / 2));
    gfx_set_object(&p, (4096 - r * 1024) & 4095);
    for (int i = 0; i < 8; i++) {
        int z0 = -TILE / 2 + i * (TILE / 8), z1 = z0 + TILE / 8, top = -(i + 1) * (WALL_FULL / 8), k = (i & 1) ? 0 : 14;
        sk_box(-104, 104, top, 0, z0, z1, 176 + k, 128 + k, 84 + k / 2);                // the step, solid to the floor
        sk_box(-104, 104, top - 4, top, z0 - 6, z1, 214, 170, 118);                       // its nosing
    }
    sk_box(-118, -104, -WALL_FULL - 30, 0, -TILE / 2, TILE / 2, 120, 84, 56);            // the side stringers
    sk_box(104, 118, -WALL_FULL - 30, 0, -TILE / 2, TILE / 2, 120, 84, 56);
    gfx_set_identity();
}


// ---- RAMPS: smooth, life-sized, and as long as you lay them ----
// A run of the same ramp laid one behind the other (same kind, same turn) is ONE ramp: two kickers in a row make a longer, taller
// kicker, three quarter pipe tiles a tall one. The height comes from the whole run's profile (the GBA's slabs only keep the footprint):
//   KICKER  a gentle curve up to a lip about 30 degrees steep: 0.5 m high on one tile, 0.8 m on two, 1.1 m on three
//   LAUNCH  steeper (about 50 degrees at the lip): 0.7 m a tile, 1.4 m on two, up to 2 m
//   Q PIPE  a quarter circle as wide as the run is long, cut near the top (72 degrees) with a steel coping: 0.8 m, 1.65 m, 2.5 m
// (A person is 1.8 m: 384 units. A tile is 1.2 m.)
extern char world_at(int x, int y); extern int world_item_facing(int x, int y);
static int is_ramp_c(char c) { return c == '1' || c == '2' || c == '4' || c == '5' || c == '9'; }
static int ramp_kind(char c) { return c == '5' ? 2 : c == '9' ? 1 : 0; }   // 0 kicker, 1 launch, 2 quarter pipe
int items_ramp_rot(char c, int facing) { return (c == '1' || c == '2' || c == '4') ? c - '1' : facing & 3; }   // which way the lip is: 0 north, 1 west, 2 south, 3 east
static const s8 lipDx[4] = { 0, -1, 0, 1 }, lipDy[4] = { -1, 0, 1, 0 };
static int same_ramp(int x, int y, int kind, int r) { char d = world_at(x, y); return is_ramp_c(d) && ramp_kind(d) == kind && items_ramp_rot(d, world_item_facing(x, y)) == r; }
static void ramp_run(int mx, int my, int kind, int r, int *idx, int *n) {   // how many tiles of the run lie between this one and the lip, and how long the run is
    int a = 0, b = 0;
    while (a < 2 && same_ramp(mx + lipDx[r] * (a + 1), my + lipDy[r] * (a + 1), kind, r)) a++;
    while (a + b < 2 && same_ramp(mx - lipDx[r] * (b + 1), my - lipDy[r] * (b + 1), kind, r)) b++;
    *idx = a; *n = a + b + 1;
}
static int isq(int n) { if (n <= 0) return 0; int x = n, y = (x + 1) >> 1; while (y < x) { x = y; y = (x + n / x) >> 1; } return x; }
static int ramp_prof(int kind, int n, int d) {   // the height (world units) d steps up from the bottom of the run (a step is 4 units: 64 a tile)
    int L = n * 64; d = CLAMP(d, 0, L);
    int t = d * 4096 / L, t2 = (t * t) >> 12;
    if (kind == 0) { int HT = n == 1 ? 110 : n == 2 ? 170 : 230; return (HT * (((t * 1434) >> 12) + ((t2 * 2662) >> 12))) >> 12; }   // 0.35 t + 0.65 t^2
    if (kind == 1) { int HT = MIN(150 * n, 420); return (HT * t2) >> 12; }
    { int R = n * 256, x = (d * 4 * 972) >> 10; return R - isq(R * R - x * x); }                                                       // (to 95% of the radius)
}
static int ramp_localB(int r, int lx, int lv) {   // 0 at the lip's edge of the tile .. 64 at the far edge
    switch (r & 3) { case 0: return lv >> 2; case 1: return lx >> 2; case 2: return 64 - (lv >> 2); default: return 64 - (lx >> 2); }
}
int items_ramp_at(int mx, int my, int lx, int lv) {   // the ramp's surface height (world units) at (lx, lv) in tile (mx, my); 0 if it is not a ramp
    char c = world_at(mx, my); if (!is_ramp_c(c)) return 0;
    int kind = ramp_kind(c), r = items_ramp_rot(c, world_item_facing(mx, my)), idx, n; ramp_run(mx, my, kind, r, &idx, &n);
    int B = CLAMP(ramp_localB(r, CLAMP(lx, 0, 255), CLAMP(lv, 0, 255)), 0, 64);
    return ramp_prof(kind, n, n * 64 - (idx * 64 + B));
}
int items_ramp_h(char ch, int facing, int lx, int lv) { (void)ch; (void)facing; (void)lx; (void)lv; return 0; }   // (old single-tile form: items_ramp_at now)
int items_ramp_lip(int mx, int my, int *dir) {   // 1 when this ramp tile holds its run's lip (and *dir: which way the lip faces, 0 north..3 east); 2 a quarter pipe's
    char c = world_at(mx, my); if (!is_ramp_c(c)) return 0;
    int kind = ramp_kind(c), r = items_ramp_rot(c, world_item_facing(mx, my)), idx, n; ramp_run(mx, my, kind, r, &idx, &n);
    *dir = r; return idx == 0 ? (kind == 2 ? 2 : 1) : 0;
}
static void rp_pt(SVECTOR *o, int r, int ox, int oz, int A, int B, int Hh) {
    int u, v;
    switch (r & 3) { case 0: u = A; v = B; break; case 1: u = B; v = 64 - A; break; case 2: u = 64 - A; v = 64 - B; break; default: u = 64 - B; v = A; }
    setVector(o, ox + u * 4, -Hh, -(oz + v * 4));
}
static int rp_light(int r, int na, int nb, int nup) {   // a normal in the ramp's frame (A across, B away from the lip, up) lit in the world's
    int nu, nv; switch (r & 3) { case 0: nu = na; nv = nb; break; case 1: nu = nb; nv = -na; break; case 2: nu = -na; nv = -nb; break; default: nu = -nb; nv = na; }
    return gfx_light(nu, -nup, -nv);
}
static u32 rp_col(const u8 *c, int L) { return (u32)MIN(255, (c[0] * L) >> 7) | ((u32)MIN(255, (c[1] * L) >> 7) << 8) | ((u32)MIN(255, (c[2] * L) >> 7) << 16); }
static void rp_quad(int r, int ox, int oz, const int p[4][3], const u8 *c, int na, int nb, int nup) {   // a flat-lit quad: corners TL TR BL BR seen from outside
    SVECTOR q[4]; for (int k = 0; k < 4; k++) rp_pt(&q[k], r, ox, oz, p[k][0], p[k][1], p[k][2]);
    int L = rp_light(r, na, nb, nup);
    gfx_quad_f(&q[0], &q[1], &q[2], &q[3], MIN(255, (c[0] * L) >> 7), MIN(255, (c[1] * L) >> 7), MIN(255, (c[2] * L) >> 7), 1);
}
static void ramp_draw(char ch, int mx, int my, int r) {
    int kind = ramp_kind(ch), idx, n; ramp_run(mx, my, kind, r, &idx, &n);
    int S = kind == 2 ? 8 : kind == 1 ? 6 : 4, ox = mx * TILE, oz = my * TILE;
    static const u8 topC[3][3] = { { 150, 108, 66 }, { 62, 104, 168 }, { 104, 106, 118 } }, sideC[3][3] = { { 128, 88, 50 }, { 46, 74, 128 }, { 150, 108, 64 } };   // (lit up to about twice this)
    const u8 *tc = topC[kind], *sc = sideC[kind];
    int hb[9], cb[9]; u32 col[9];
    for (int k = 0; k <= S; k++) {   // the heights and the light at each strip's edge (a smooth curve: the light is blended across, no bands)
        int B = k * 64 / S, d = n * 64 - (idx * 64 + B);
        hb[k] = ramp_prof(kind, n, d);
        int dh = ramp_prof(kind, n, d + 1) - ramp_prof(kind, n, d - 1);   // the rise over 8 units (2 steps)
        { int run = 8, l2 = isq(run * run + dh * dh); if (l2 < 1) l2 = 1; cb[k] = rp_light(r, 0, (dh * 4096) / l2, (run * 4096) / l2); }
        col[k] = rp_col(tc, cb[k]);
    }
    for (int k = 0; k < S; k++) {   // the riding surface
        int b0 = k * 64 / S, b1 = (k + 1) * 64 / S;
        SVECTOR q[4]; rp_pt(&q[0], r, ox, oz, 0, b0, hb[k]); rp_pt(&q[1], r, ox, oz, 64, b0, hb[k]); rp_pt(&q[2], r, ox, oz, 0, b1, hb[k + 1]); rp_pt(&q[3], r, ox, oz, 64, b1, hb[k + 1]);
        gfx_quad_g(&q[0], &q[1], &q[2], &q[3], col[k], col[k], col[k + 1], col[k + 1], 1);
    }
    // the cheeks, where the ramp ends sideways (not between two ramps side by side)
    static const s8 aDx[4][2] = { { -1, 1 }, { 0, 0 }, { 1, -1 }, { 0, 0 } }, aDy[4][2] = { { 0, 0 }, { 1, -1 }, { 0, 0 }, { -1, 1 } };   // the neighbour past A = 0 and past A = 64
    for (int e = 0; e < 2; e++) {
        if (same_ramp(mx + aDx[r][e], my + aDy[r][e], kind, r)) continue;
        int A = e ? 64 : 0;
        for (int k = 0; k < S; k++) {
            int b0 = k * 64 / S, b1 = (k + 1) * 64 / S, h0 = hb[k], h1 = hb[k + 1];
            if (h0 <= 2 && h1 <= 2) continue;
            if (e) { int q[4][3] = { { 64, b1, h1 }, { 64, b0, h0 }, { 64, b1, 0 }, { 64, b0, 0 } }; rp_quad(r, ox, oz, q, sc, 4096, 0, 0); }
            else   { int q[4][3] = { { 0, b0, h0 }, { 0, b1, h1 }, { 0, b0, 0 }, { 0, b1, 0 } }; rp_quad(r, ox, oz, q, sc, -4096, 0, 0); }
        }
        (void)A;
    }
    if (idx == 0) {   // the lip's tile: the back, and what finishes the lip
        static const u8 back[3] = { 140, 100, 62 }, steel[3] = { 214, 220, 230 };
        int HT = hb[0];
        { int q[4][3] = { { 64, 0, HT }, { 0, 0, HT }, { 64, 0, 0 }, { 0, 0, 0 } }; rp_quad(r, ox, oz, q, back, 0, -4096, 0); }
        if (kind == 2) {   // the coping: a steel pipe along the top
            int t1[4][3] = { { 0, 0, HT + 10 }, { 64, 0, HT + 10 }, { 0, 3, HT + 7 }, { 64, 3, HT + 7 } }; rp_quad(r, ox, oz, t1, steel, 0, 0, 4096);
            int t2[4][3] = { { 0, 3, HT + 7 }, { 64, 3, HT + 7 }, { 0, 3, HT - 4 }, { 64, 3, HT - 4 } }; rp_quad(r, ox, oz, t2, steel, 0, 4096, 0);
            int t3[4][3] = { { 64, 0, HT + 10 }, { 0, 0, HT + 10 }, { 64, -1, HT }, { 0, -1, HT } }; rp_quad(r, ox, oz, t3, steel, 0, -4096, 0);
        } else {           // a steel plate on the lip
            int t1[4][3] = { { 0, 0, HT + 3 }, { 64, 0, HT + 3 }, { 0, 4, hb[0] - 2 + 3 }, { 64, 4, hb[0] - 2 + 3 } }; rp_quad(r, ox, oz, t1, steel, 0, 2000, 3600);
        }
    }
}

int items_hit(char ch, int rot, int along, int cu, int cv, int r) {
    const ItemDef *d = def_of(ch); if (!d) return -1;
    if (ch == '1' || ch == '2' || ch == '4' || ch == '5' || ch == '9' || ch == '|') return -1;
    const IBox *b = d->b; int n = d->n, rr = (d->flags & F_ROT) ? rot : 0;
    if ((d->flags & F_ALONG) && !along && d->bv) { b = d->bv; n = d->nv; }
    r -= 6;   // a little give, so a body can brush past an edge
    for (int i = 0; i < n; i++) {
        if (b[i].z0 >= 12) continue;   // (above head height: it hangs over, it does not block)
        int ua, va, ub, vb; rotPt(rr, b[i].a0, b[i].b0, &ua, &va); rotPt(rr, b[i].a1, b[i].b1, &ub, &vb);
        int u0 = MIN(ua, ub) * IUNIT, u1 = MAX(ua, ub) * IUNIT, v0 = MIN(va, vb) * IUNIT, v1 = MAX(va, vb) * IUNIT;
        if (cu + r > u0 && cu - r < u1 && cv + r > v0 && cv - r < v1) return 1;
    }
    return 0;
}

static int is_ramp_c(char c);
// ---- the pet (a dog or a cat, about knee high) and the city bus ----
static const u8 petCol[6][3] = { { 150, 100, 60 }, { 52, 48, 54 }, { 238, 234, 226 }, { 226, 140, 50 }, { 142, 142, 152 }, { 206, 172, 120 } };
void items_draw_pet(int x, int z, int yaw, int y, int kind, int ci, int phase) {
    int r = petCol[ci % 6][0], g = petCol[ci % 6][1], b = petCol[ci % 6][2], dk = 40;
    VECTOR p; setVector(&p, x, y, z); gfx_set_object(&p, yaw & 4095); sk_k = 16;
    int sw = (isin(phase & 4095) * 14) >> 12, wag = (isin((phase * 2) & 4095) * 10) >> 12;   // the legs swing; the tail wags
    if (kind == 1) {   // a dog: a long body, a head with a snout and floppy ears, four legs, a wagging tail
        sk_box(-24, 24, -74, -34, -50, 50, r, g, b);
        sk_box(-22, 22, -100, -58, 40, 84, r, g, b); sk_box(-14, 14, -84, -62, 84, 104, r * 9 / 10, g * 9 / 10, b * 9 / 10); sk_box(-6, 6, -88, -80, 100, 108, dk, dk, dk);
        sk_box(-30, -22, -104, -70, 46, 62, r * 7 / 10, g * 7 / 10, b * 7 / 10); sk_box(22, 30, -104, -70, 46, 62, r * 7 / 10, g * 7 / 10, b * 7 / 10);
        sk_box(-16, -8, -96, -90, 80, 84, 20, 20, 24); sk_box(8, 16, -96, -90, 80, 84, 20, 20, 24);
        sk_box(-20, -8, -34, 0, 24 + sw, 40 + sw, r * 8 / 10, g * 8 / 10, b * 8 / 10); sk_box(8, 20, -34, 0, 24 - sw, 40 - sw, r * 8 / 10, g * 8 / 10, b * 8 / 10);
        sk_box(-20, -8, -34, 0, -40 - sw, -24 - sw, r * 8 / 10, g * 8 / 10, b * 8 / 10); sk_box(8, 20, -34, 0, -40 + sw, -24 + sw, r * 8 / 10, g * 8 / 10, b * 8 / 10);
        sk_box(-4 + wag, 4 + wag, -92, -64, -66, -48, r, g, b);
    } else {           // a cat: smaller, slim, pointed ears, a long upright tail
        sk_box(-18, 18, -58, -30, -40, 40, r, g, b);
        sk_box(-16, 16, -82, -50, 32, 64, r, g, b); sk_box(-14, -6, -94, -82, 36, 46, r, g, b); sk_box(6, 14, -94, -82, 36, 46, r, g, b);
        sk_box(-12, -6, -72, -66, 62, 66, 140, 220, 120); sk_box(6, 12, -72, -66, 62, 66, 140, 220, 120); sk_box(-3, 3, -62, -58, 62, 68, 230, 150, 160);
        sk_box(-14, -6, -30, 0, 20 + sw, 32 + sw, r * 8 / 10, g * 8 / 10, b * 8 / 10); sk_box(6, 14, -30, 0, 20 - sw, 32 - sw, r * 8 / 10, g * 8 / 10, b * 8 / 10);
        sk_box(-14, -6, -30, 0, -32 - sw, -20 - sw, r * 8 / 10, g * 8 / 10, b * 8 / 10); sk_box(6, 14, -30, 0, -32 + sw, -20 + sw, r * 8 / 10, g * 8 / 10, b * 8 / 10);
        sk_box(-3 + wag, 3 + wag, -96, -50, -50, -42, r, g, b); sk_box(-3 + wag, 3 + wag, -100, -90, -50, -42, r * 6 / 10, g * 6 / 10, b * 6 / 10);
    }
    gfx_set_identity();
}
static const VB B_BUS[] = {   // the city bus (model units at the cars' scale): white, a stripe, a row of windows, two doors, a lit destination sign
    VBX(-118, 118, -50, -22, -336, 336, K_DARK, 0),                                       // the underside
    VBX(-122, 122, -128, -46, -340, 340, K_WHITE, 0),                                     // the lower body
    VBX(-123, 123, -108, -84, -340, 340, K_BODY, 0),                                      // the stripe, in the body colour
    VBX(-123, 123, -208, -128, -318, 300, K_GLASS, 0),                                    // the windows
    VBX(-122, 122, -244, -208, -340, 340, K_WHITE, 0),                                    // the roof's band
    VBX(-104, 104, -252, -244, -300, 300, K_LOW, F_NEAR), VBX(-60, 60, -260, -252, -170, 40, K_WHITE, F_NEAR),   // the roof and an air unit
    VBX(-116, 116, -214, -130, 340, 346, K_GLASS, 0),                                     // the windscreen
    VBX(-80, 80, -240, -218, 340, 346, K_HEAD, F_NEAR),                                   // the destination sign
    VBX(-120, 120, -62, -42, 340, 352, K_DARK, F_NEAR), VBX(-120, 120, -62, -42, -352, -340, K_DARK, F_NEAR),   // the bumpers
    VBX(80, 116, -108, -88, 344, 352, K_HEAD, F_NEAR | F_MIR), VBX(80, 116, -108, -88, -352, -344, K_TAIL, F_NEAR | F_MIR),
    VBX(-100, 100, -204, -134, -346, -340, K_GLASS, F_NEAR),                              // the back window
    VBX(122, 126, -200, -52, 222, 296, K_DARK, F_NEAR | F_MIR), VBX(122, 126, -200, -52, -86, -12, K_DARK, F_NEAR | F_MIR),   // the doors
    VBX(123, 128, -204, -198, 218, 300, K_WHITE, F_NEAR | F_MIR), VBX(123, 128, -204, -198, -90, -8, K_WHITE, F_NEAR | F_MIR),
    VBX(123, 128, -200, -52, 256, 260, K_WHITE, F_NEAR | F_MIR), VBX(123, 128, -200, -52, -52, -48, K_WHITE, F_NEAR | F_MIR),   // (the door leaves meet)
    VBX(122, 142, -204, -176, 300, 308, K_DARK, F_NEAR | F_MIR),                          // the mirrors
};
void items_draw_bus(int x, int z, int yaw, int y) {   // the city bus; yaw is the model's own turn (as stands at a stop)
    VECTOR p; setVector(&p, x, y, z); gfx_set_object(&p, yaw & 4095); sk_k = 28;
    { int ex = cam.eye.vx - x, ez = cam.eye.vz - z, c = icos(yaw), s = isin(yaw), ox = (ex * c - ez * s) >> 12; veh_cs = ox > 120 ? 1 : ox < -120 ? -1 : 0; veh_lod = 0; }
    veh_tones(1); items_veh_open = 0;
    veh_rows(B_BUS, sizeof B_BUS / sizeof(VB));
    veh_wheels(38, 112, -200, 210, 0, 16);
    sk_k = 16; gfx_set_identity();
}

// ---- the CREATE-A-DOG pack: a dog from its DogLook (dog.h). Faces +z; feet at y 0, up is negative ----
static void dbx(int x0, int x1, int y0, int y1, int z0, int z1, const int *c) { sk_box(x0, x1, y0, y1, z0, z1, c[0], c[1], c[2]); }
static void dpal(int i, int *c) { dog_pal(i, &c[0], &c[1], &c[2]); }
static void dshade(const int *c, int k, int *o) { for (int i = 0; i < 3; i++) o[i] = MIN(255, c[i] * k / 16); }
void items_draw_dog(int x, int z, int yaw, int y, const DogLook *dl, int phase) {
    const u8 *l = dl->look;
    int c1[3], c2[3], cp[3], cn[3], ce[3], cl[3], ch[3], co[3], d1[3], dk[3] = { 24, 22, 26 }, wh[3] = { 244, 244, 240 }, gd[3] = { 232, 190, 60 }, pk[3] = { 236, 110, 130 };
    dpal(l[DL_C1], c1); dpal(l[DL_C2], c2); dpal(l[DL_PATC], cp); dpal(l[DL_NOSEC], cn); { static const u8 ec[8] = { 2, 1, 15, 13, 14, 11, 9, 0 }; dpal(ec[l[DL_EYEC] & 7], ce); }
    dpal(l[DL_COLLARC], cl); dpal(l[DL_HATC], ch); dpal(l[DL_OUTC], co); dshade(c1, 13, d1);
    int k = 12 + l[DL_SIZE]; VECTOR p; setVector(&p, x, y, z); gfx_set_object(&p, yaw & 4095); sk_k = k;
    int sw = (isin(phase & 4095) * 14) >> 12, wag = (isin((phase * 2) & 4095) * 8) >> 12;
    int fur = l[DL_FUR] & 3, L = 36 + l[DL_BODY] * 4, DW = 22 + (fur >= 2 ? 3 : 0) + (fur == 3 ? 3 : 0), lh = 18 + l[DL_LEGS] * 6, yb1 = -lh, yb0 = yb1 - 38;
    int hs = 15 + l[DL_HEAD] * 2, hz0 = L - 6, hz1 = hz0 + 2 * hs, hy1 = yb0 + 24, hy0 = hy1 - 2 * hs, zc = (hz0 + hz1) / 2;
    int sn = 4 + l[DL_SNOUT] * 4, ws = hs * 11 / 20, sy0 = hy0 + hs * 9 / 10, sy1 = hy1 - 3, nz = 4 + l[DL_NOSESZ], pat = l[DL_PAT] % DOG_NPAT;
    const int *bc = c1, *snc = (pat == 7) ? c2 : (pat == 8) ? cp : c1;
    // the legs (four), the feet in the second colour for socks / tuxedo
    for (int i = 0; i < 4; i++) { int xs = (i & 1) ? DW - 12 : -DW + 2, zz = (i & 2) ? -L + 4 : L - 18, sg = ((i & 1) ^ ((i >> 1) & 1)) ? sw : -sw;
        dbx(xs, xs + 10, -lh, 0, zz + sg, zz + 14 + sg, d1);
        if (pat == 4 || pat == 7) dbx(xs - 1, xs + 11, -9, 0, zz + sg - 1, zz + 15 + sg, c2); }
    dbx(-DW, DW, yb0, yb1, -L, L, bc);                                                                     // the body
    if (fur >= 2) { int cm[3]; dshade(c1, 17, cm); dbx(-DW + 4, DW - 4, yb0 - 4, yb0 + 10, L - 14, L + 6, cm); }   // a mane
    if (fur == 3) { int cm[3]; dshade(c1, 15, cm); dbx(-DW - 2, DW + 2, yb1 - 6, yb1 + 2, -L + 4, L - 8, cm); }
    dbx(-hs, hs, hy0, hy1, hz0, hz1, bc);                                                                // the head
    dbx(-ws, ws, sy0, sy1, hz1, hz1 + sn, snc);                                                          // the snout
    dbx(-nz, nz, sy0 - 1, sy0 + nz, hz1 + sn - 1, hz1 + sn + 4, cn);                                     // the nose
    // the coat's pattern
    if (pat == 1) { for (int i = -1; i <= 1; i++) { dbx(-DW + 4 + (i & 1) * 10, -DW + 14 + (i & 1) * 10, yb0 - 1, yb0 + 8, i * (L / 2) - 5, i * (L / 2) + 5, cp); dbx(DW - 12 - (i & 1) * 8, DW - 2, yb0 - 1, yb0 + 8, i * (L / 2) + 6, i * (L / 2) + 15, cp); } }
    if (pat == 2) dbx(0, hs + 1, hy0 + 3, hy0 + hs + 2, hz1 - 7, hz1 + 1, cp);
    if (pat == 3) dbx(-DW - 1, DW + 1, yb0 - 1, yb0 + 16, -L * 6 / 10, L * 3 / 10, cp);
    if (pat == 5) { dbx(-DW + 4, DW - 4, yb1 - 8, yb1 + 1, -L + 6, L - 6, c2); dbx(-12, 12, yb0 + 12, yb1 - 2, L - 4, L + 1, c2); }
    if (pat == 6) for (int i = -1; i <= 1; i++) dbx(-DW - 1, DW + 1, yb0 - 1, yb0 + 9, i * (L / 2) - 3, i * (L / 2) + 3, cp);
    if (pat == 7) dbx(-12, 12, yb0 + 8, yb1 - 2, L - 4, L + 1, c2);
    if (pat == 8) dbx(-hs + 3, hs - 3, hy0 + hs * 6 / 10, hy1 - 2, hz1 - 3, hz1 + 1, cp);
    // the ears
    { int es = 8 + l[DL_EARSZ] * 3, st = l[DL_EARS] & 7, e1[3]; dshade(c1, 12, e1);
      for (int sd = -1; sd <= 1; sd += 2) { int xa = sd < 0 ? -hs - 6 : hs, xb = sd < 0 ? -hs : hs + 6;
        switch (st) {
          case 0: dbx(xa, xb, hy0 + 2, hy0 + 2 + es * 6 / 5, zc - 6, zc + 6, e1); break;                                  // floppy
          case 1: dbx(sd < 0 ? -hs + 1 : hs - 11, sd < 0 ? -hs + 11 : hs - 1, hy0 - es * 6 / 10, hy0, zc - 4, zc + 4, e1); dbx(sd < 0 ? -hs + 3 : hs - 9, sd < 0 ? -hs + 9 : hs - 3, hy0 - es, hy0 - es * 6 / 10, zc - 3, zc + 3, e1); break;   // pointy
          case 2: dbx(sd < 0 ? -hs - 3 : hs - 7, sd < 0 ? -hs + 7 : hs + 3, hy0 - es / 2, hy0 + 4, zc - 6, zc + 6, e1); break;       // round
          case 3: dbx(xa + (sd < 0 ? 2 : 0), xb - (sd < 0 ? 0 : 2), hy0 + 2, hy0 + 2 + es * 9 / 5, zc - 4, zc + 4, e1); break;     // long
          case 4: dbx(sd < 0 ? -hs : hs - 10, sd < 0 ? -hs + 10 : hs, hy0 - es / 2, hy0, zc - 6, zc + 6, e1); dbx(sd < 0 ? -hs : hs - 10, sd < 0 ? -hs + 10 : hs, hy0 - es / 2, hy0 - es / 4, zc + 6, zc + 12, e1); break;   // folded
          case 5: dbx(sd < 0 ? -hs + 1 : hs - 13, sd < 0 ? -hs + 13 : hs - 1, hy0 - es * 14 / 10, hy0, zc - 3, zc + 3, e1); break; // bat
          case 6: dbx(sd < 0 ? -hs + 2 : hs - 8, sd < 0 ? -hs + 8 : hs - 2, hy0 - 8, hy0, zc - 3, zc + 3, e1); break;              // tiny
          default: break; } } }
    // the tail
    { int tl = 14 + l[DL_TAILSZ] * 5, st = l[DL_TAIL] % 7, yt = yb0 + 6;
      switch (st) {
        case 0: dbx(-4 + wag, 4 + wag, yt - 4, yt + 6, -L - tl / 2, -L, d1); dbx(-4 + wag, 4 + wag, yt - tl * 6 / 10, yt - 4, -L - tl * 8 / 10, -L - tl / 2, d1); break;
        case 1: dbx(-4 + wag, 4 + wag, yb0 - 6, yb0 + 8, -L - 8, -L, d1); dbx(-4 + wag, 4 + wag, yb0 - 18, yb0 - 6, -L - 12, -L - 4, d1); dbx(-4 + wag, 4 + wag, yb0 - 22, yb0 - 14, -L - 8, -L, d1); break;
        case 2: dbx(-8 + wag, 8 + wag, yt - 10, yt + 8, -L - tl * 8 / 10, -L, d1); dbx(-8 + wag, 8 + wag, yt - 10, yt + 8, -L - tl * 8 / 10 - 4, -L - tl * 8 / 10, c2); break;
        case 3: dbx(-4 + wag, 4 + wag, yb0 - 4, yb0 + 8, -L - 7, -L, d1); break;
        case 4: dbx(-2 + wag, 2 + wag, yt, yt + 4, -L - tl * 14 / 10, -L, d1); break;
        case 5: dbx(-11 + wag, 11 + wag, yb0 - 14, yb0 + 10, -L - 20, -L, d1); break;
        default: dbx(-4 + wag, 4 + wag, yb0 - tl - 6, yb0 + 6, -L - 7, -L, d1); break; } }
    // the face: eyes, brows, mouth
    { int e = 2 + l[DL_EYESZ] / 2, ey = hy0 + hs * 7 / 10, st = l[DL_EYEST] % 6, ex = hs * 5 / 10;
      for (int sd = -1; sd <= 1; sd += 2) { int cx = sd * ex, ee = st == 5 ? e * 14 / 10 : e;
        if (st == 4 || (st == 3 && sd < 0)) { dbx(cx - ee, cx + ee, ey, ey + 2, hz1, hz1 + 2, dk); continue; }   // squint, wink
        if (st == 5) dbx(cx - ee - 1, cx + ee + 1, ey - ee - 1, ey + ee + 1, hz1, hz1 + 1, wh);
        dbx(cx - ee, cx + ee, st == 1 ? ey : ey - ee, ey + ee, hz1 + 1, hz1 + 2, ce);
        if (st == 1) dbx(cx - ee, cx + ee, ey - ee, ey, hz1 + 1, hz1 + 2, c1);                                     // sleepy: a lid
        dbx(cx - ee / 2, cx + ee / 2, ey - ee / 2, ey + ee / 2, hz1 + 2, hz1 + 3, dk);
        if (st == 2) dbx(cx + ee / 3, cx + ee / 3 + 2, ey - ee, ey - ee + 3, hz1 + 3, hz1 + 4, wh);              // a sparkle
        if (l[DL_BROWS]) dbx(cx - ee + 1, cx + ee - 1, ey - ee - 6, ey - ee - 3, hz1, hz1 + 1, c2); } }
    { int mo = l[DL_MOUTH] & 3, my = sy1 - 3;
      if (mo == 0) dbx(-ws + 2, ws - 2, my, my + 2, hz1 + 1, hz1 + sn + 1, dk);
      if (mo == 1) { dbx(-ws + 2, ws - 2, my, my + 2, hz1 + 1, hz1 + sn + 1, dk); dbx(-4, 4, my + 2, my + 12, hz1 + sn - 9, hz1 + sn - 2, pk); }
      if (mo == 2) { dbx(-ws + 3, ws - 3, my - 2, my + 4, hz1 + 1, hz1 + sn + 1, dk); dbx(-4, 4, my + 3, my + 7, hz1 + sn - 8, hz1 + sn, pk); }
      if (mo == 3) { dbx(-ws + 3, ws - 3, my - 2, my + 4, hz1 + 1, hz1 + sn + 1, dk); dbx(-5, 5, my + 3, my + 18, hz1 + sn - 10, hz1 + sn - 1, pk); } }
    // the wear: collar, outfit, glasses, hat
    { int st = l[DL_COLLAR] & 7, ny = yb0 + 4;
      if (st) { dbx(-DW - 1, DW + 1, ny, ny + 12, L - 16, L - 8, st == 7 ? gd : cl);
        if (st == 2) { dbx(-12, 12, ny + 12, ny + 26, L - 16, L - 8, cl); dbx(-5, 5, ny + 26, ny + 36, L - 16, L - 8, cl); }
        if (st == 3) { dbx(-14, -2, ny + 2, ny + 14, L - 6, L, cl); dbx(2, 14, ny + 2, ny + 14, L - 6, L, cl); dbx(-3, 3, ny + 5, ny + 11, L - 7, L + 1, dk); }
        if (st == 4) dbx(-4, 4, ny + 12, ny + 22, L - 8, L - 2, gd);
        if (st == 5) for (int i = -1; i <= 1; i++) dbx(i * 14 - 2, i * 14 + 2, ny - 5, ny, L - 14, L - 10, wh);
        if (st == 6) dbx(DW + 1, DW + 5, ny, ny + 30, L - 15, L - 9, cl); } }
    { int st = l[DL_OUTFIT] % 6;
      if (st == 1) { dbx(-DW - 2, DW + 2, yb0 - 1, yb1 - 6, -L + 10, L - 8, co); dbx(-DW - 3, DW + 3, yb0 + 12, yb0 + 18, -L + 10, L - 8, wh); }
      if (st == 2) { dbx(-DW - 1, DW + 1, yb0 - 3, yb0, -L, L - 10, co); dbx(-DW - 3, -DW - 1, yb0 - 3, yb1 + 6, -L + 2, L - 12, co); dbx(DW + 1, DW + 3, yb0 - 3, yb1 + 6, -L + 2, L - 12, co); }
      if (st == 3) { dbx(-DW - 2, -DW + 2, yb0 + 2, yb1 - 6, -L + 12, L - 14, co); dbx(DW - 2, DW + 2, yb0 + 2, yb1 - 6, -L + 12, L - 14, co); dbx(-DW, DW, yb0 - 2, yb0, -L + 12, L - 14, co); }
      if (st == 4) { dbx(-12, 12, yb0 - 20, yb0, -L + 12, -L + 34, co); dbx(-12, 12, yb0 - 22, yb0 - 18, -L + 12, -L + 34, wh); }
      if (st == 5) { dbx(-DW - 2, DW + 2, yb0 - 2, yb1 - 8, -L + 12, L - 14, co); dbx(-DW - 3, DW + 3, yb0 + 10, yb0 + 16, -L + 12, L - 14, wh); } }
    { int st = l[DL_GLASS] & 3, e = 2 + l[DL_EYESZ] / 2, ey = hy0 + hs * 7 / 10, ex = hs * 5 / 10, lens[3] = { 150, 200, 235 };
      if (st == 1) for (int sd = -1; sd <= 1; sd += 2) { int cx = sd * ex; dbx(cx - e - 3, cx + e + 3, ey - e - 3, ey + e + 3, hz1 + 3, hz1 + 4, dk); dbx(cx - e - 1, cx + e + 1, ey - e - 1, ey + e + 1, hz1 + 4, hz1 + 5, lens); }
      if (st == 2) { dbx(-hs + 1, hs - 1, ey - e - 2, ey + e + 2, hz1 + 3, hz1 + 6, dk); }
      if (st == 3) { int cx = ex; dbx(cx - e - 3, cx + e + 3, ey - e - 3, ey + e + 3, hz1 + 3, hz1 + 4, gd); dbx(cx - e - 1, cx + e + 1, ey - e - 1, ey + e + 1, hz1 + 4, hz1 + 5, lens); dbx(cx + e + 3, cx + e + 5, ey, ey + 40, hz1 + 3, hz1 + 4, gd); } }
    { int st = l[DL_HAT] & 7;
      if (st == 1) { dbx(-hs, hs, hy0 - 7, hy0, hz0 + 2, hz1 - 2, ch); dbx(-hs + 3, hs - 3, hy0 - 3, hy0, hz1 - 2, hz1 + 10, ch); }
      if (st == 2) { dbx(-9, 9, hy0 - 8, hy0, zc - 9, zc + 9, ch); dbx(-6, 6, hy0 - 17, hy0 - 8, zc - 6, zc + 6, wh); dbx(-3, 3, hy0 - 26, hy0 - 17, zc - 3, zc + 3, ch); dbx(-2, 2, hy0 - 29, hy0 - 26, zc - 2, zc + 2, gd); }
      if (st == 3) { dbx(-hs + 2, hs - 2, hy0 - 8, hy0, hz0 + 2, hz1 - 2, gd); for (int i = -1; i <= 1; i++) dbx(i * (hs - 6) - 3, i * (hs - 6) + 3, hy0 - 16, hy0 - 8, zc - 3, zc + 3, gd); dbx(-3, 3, hy0 - 6, hy0 - 2, hz1 - 4, hz1 - 2, ch); }
      if (st == 4) { dbx(hs - 22, hs - 8, hy0 - 12, hy0, zc - 5, zc + 5, ch); dbx(hs - 6, hs + 8, hy0 - 12, hy0, zc - 5, zc + 5, ch); dbx(hs - 9, hs - 5, hy0 - 8, hy0 - 2, zc - 5, zc + 5, wh); }
      if (st == 5) for (int sd = -1; sd <= 1; sd += 2) { int xa = sd * (hs - 8); dbx(xa - 2, xa + 2, hy0 - 24, hy0, zc - 2, zc + 2, ch); dbx(xa - 2 + sd * 6, xa + 2 + sd * 6, hy0 - 24, hy0 - 16, zc - 2, zc + 2, ch); dbx(xa - 2 - sd * 6, xa + 2 - sd * 6, hy0 - 16, hy0 - 10, zc - 2, zc + 2, ch); }
      if (st == 6) { dbx(-hs - 4, hs + 4, hy0 - 4, hy0, hz0 - 4, hz1 + 4, dk); dbx(-hs + 4, hs - 4, hy0 - 28, hy0 - 4, hz0 + 4, hz1 - 4, dk); dbx(-hs + 4, hs - 4, hy0 - 12, hy0 - 6, hz0 + 4, hz1 - 4, ch); }
      if (st == 7) { dbx(-1, 1, hy0 - 8, hy0, zc - 1, zc + 1, c1); for (int i = 0; i < 4; i++) dbx(((i & 1) ? 4 : -8), ((i & 1) ? 8 : -4), hy0 - 14 + (i >> 1) * 4, hy0 - 8 + (i >> 1) * 4, zc - 4 + (i & 2) * 2, zc + 0 + (i & 2) * 2, ch); } }
    sk_k = 16; gfx_set_identity();
}
int bus_here(int mx, int my) { extern int clock_min; return ((clock_min + mx * 7 + my * 13) % 20) < 4; }   // a bus stands at every stop for four minutes of each twenty
// ---- FARM LIFE, alive: the crops grow in the beds (the farm's state: a stage per crop), the hens scratch about the coop ----
#include "sim.h"
static void fm_box(int x0, int x1, int y0, int y1, int z0, int z1, int r, int g, int b) { sk_box(x0, x1, y0, y1, z0, z1, r, g, b); }
static void farm_extras(char ch, int mx, int my, int t) {
    VECTOR p; setVector(&p, mx * TILE + TILE / 2, 0, -(my * TILE + TILE / 2));
    gfx_set_object(&p, 0);
    if (ch == (char)0xDB) {
        int k = ((mx >> 1) + (my >> 1)) & 3, n = fm_n[k], need = fm_need(k);
        if (n) {
            int g = fm_g[k], ready = g >= need, h = ready ? 100 : 22 + 62 * g / (need ? need : 1), cnt = MIN(4, (n + 3) / 4 + 1);
            int dry = fm_dry[k] > 0 && !ready;
            for (int i = 0; i < cnt; i++) {
                int cx = (i & 1) ? 54 : -54, cz = (i & 2) ? 54 : -54, w = 11;
                int sr = dry ? 150 : 40, sg = dry ? 130 : 170, sb = dry ? 50 : 50;
                switch (k) {
                case 0: for (int j = -1; j <= 1; j++) { fm_box(cx + j * 22 - 4, cx + j * 22 + 4, -h, -2, cz - 4, cz + 4, 214, 178, 56); if (ready) fm_box(cx + j * 22 - 6, cx + j * 22 + 6, -h - 12, -h, cz - 5, cz + 5, 240, 200, 70); } break;   // wheat
                case 1: fm_box(cx - w / 2, cx + w / 2, -h - 20, -2, cz - w / 2, cz + w / 2, sr, sg, sb); if (ready) fm_box(cx + w / 2, cx + w / 2 + 12, -h - 6, -h + 18, cz - 6, cz + 6, 245, 210, 60); break;   // corn
                case 2: fm_box(cx - 20, cx + 20, -h * 3 / 4, -2, cz - 20, cz + 20, sr, sg, sb); if (ready) { fm_box(cx - 16, cx - 6, -h * 3 / 4 - 8, -h * 3 / 4 + 4, cz - 4, cz + 8, 230, 40, 40); fm_box(cx + 8, cx + 18, -h * 3 / 4 - 8, -h * 3 / 4 + 4, cz - 8, cz + 4, 230, 40, 40); } break;   // tomatoes
                default: fm_box(cx - 24, cx + 24, -14, -2, cz - 24, cz + 24, sr, sg, sb); if (ready) fm_box(cx - 14, cx + 14, -h * 2 / 3, -14, cz - 14, cz + 14, 240, 130, 30); else if (h > 40) fm_box(cx - 6, cx + 6, -22, -14, cz - 6, cz + 6, 120, 170, 60); break;   // pumpkins
                }
            }
            if ((fm_pest >> k) & 1) for (int i = 0; i < 3; i++) fm_box(-70 + i * 60, -62 + i * 60, -10, -4, -30 + i * 20, -22 + i * 20, 40, 30, 30);   // pests
        }
    } else {   // the coop: hens, a few steps about in front
        int ci = (my >> 1) & 1, nh = MAX(0, MIN(3, (fm_hens - ci + 1) / 2));
        for (int i = 0; i < nh; i++) {
            int ph = (t * 3 + i * 97 + ci * 40) & 255, x = -50 + i * 50 + (isin((ph << 4) & 4095) >> 9), z = 108 + (i * 13 & 15);
            int peck = ((t / 10 + i * 3) & 7) == 0 ? 8 : 0;
            fm_box(x - 9, x + 9, -22 + peck / 2, -6, z - 14, z + 8, 245, 245, 240);                      // the body
            fm_box(x - 5, x + 5, -34 + peck, -22 + peck / 2, z - 18, z - 8, 245, 245, 240);              // the head
            fm_box(x - 2, x + 2, -39 + peck, -34 + peck, z - 16, z - 12, 220, 40, 40);                   // the comb
            fm_box(x - 2, x + 2, -29 + peck, -26 + peck, z - 22, z - 18, 240, 170, 40);                  // the beak
            fm_box(x - 6, x - 3, -6, 0, z - 4, z - 1, 240, 170, 40); fm_box(x + 3, x + 6, -6, 0, z - 4, z - 1, 240, 170, 40);   // the feet
        }
    }
    gfx_set_identity();
}
void items_draw(char ch, int mx, int my, int rot, int along, int t) {
    if (ch == 'P') return;
    shade6_update(t);
    if (ch == '|') { stairs_item(mx, my, rot); return; }
    if (ch == '+' && bus_here(mx, my)) {   // the bus is in: pulled up beside the stop
        int u, v; rotPt(rot, 4, -4, &u, &v); items_draw_bus(mx * TILE + u * IUNIT, -(my * TILE + v * IUNIT), (1024 + rot * 1024) & 4095, 0); }
    if (ch == 'B') { skate_item(mx, my, rot); return; }                    // the SPAWN marker: where the Bore starts, nothing to see (in 3D the GBA's pad read as a figurine base)
    const ItemDef *d = def_of(ch);
    if (!d) return;
    const IBox *b = d->b; int n = d->n; int base = box_base[d - defs][0];
    if ((d->flags & F_ALONG) && !along && d->bv) { b = d->bv; n = d->nv; base = box_base[d - defs][1]; }
    int r = (d->flags & F_ROT) ? rot : 0;
    if (ch == '1' || ch == '2' || ch == '4') r = ch - '1';
    int ox = mx * TILE, oz = my * TILE;
    if (!(d->flags & F_FLAT) && !getenv_noshadow && !is_ramp_c(ch)) {   // a contact shadow under the item (not a ramp: it would sort over the slope): the union of its boxes on the floor, a touch bigger
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
    if (ch == '1' || ch == '2' || ch == '4' || ch == '5' || ch == '9') { ramp_draw(ch, mx, my, r); return; }   // (the ramps: smooth slopes, not stacked slabs)
    if (ch == (char)0xDB) n = 1;                                              // (FARM LIFE: a bed draws only its soil; what grows in it is the farm's own state)
    else if (ch == (char)0xDD && fm_cows <= ((my >> 1) & 1)) n = 1;           // (a stall: the cow is there only when you own one for it)
    for (int i = 0; i < n; i++) draw_box(&b[i], box_ref[base + i], r, ox, oz, t);
    if (ch == (char)0xDB || ch == (char)0xDC) farm_extras(ch, mx, my, t);
}
