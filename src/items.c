// items.c - BORE's textured-box items drawn as real boxes. The art is the GBA game's own (items_data.inc and the art headers).
#include "items.h"
#include "gfx.h"
#include "atlas.h"

#include "items_data.inc"   // defines Mat, IBox, IObj and the MAT macro, straight from the GBA game
#include "items_more.inc"   // BORE 2's furniture pack (tools/make_items2.py)

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
static const IBox bxStreetLamp[2] = { {3,3,5,5,0,22,{&mLampP,&mLampP,&mLampP,&mLampP,&mLampP}}, {2,2,6,6,22,24,{&mLampH,&mLampH,&mLampH,&mLampH,&mLampT}} };
static const IBox bxLounger[2] = { {0,2,8,6,0,2,{&mLnrS,&mLnrS,&mLnrS,&mLnrS,&mLnrT}}, {0,2,2,6,2,5,{&mLnrB,&mLnrB,&mLnrB,&mLnrB,&mLnrT}} };

// ---- a fuel pump: a red body with a screen, a white hose holder, a yellow cap ----
static const u16 pPump[5]={RGB(27,5,5),RGB(30,30,30),RGB(5,5,7),RGB(31,26,8),RGB(8,26,10)};
MAT(mPmFront,pPump,4,6,"aaaa","acec","aaaa","abba","abba","aaaa")
MAT(mPmSide,pPump,4,6,"aaaa","aaaa","aaaa","abba","aaaa","aaaa")
MAT(mPmCap,pPump,1,1,"d")
MAT(mPmTop,pPump,1,1,"b")
static const IBox bxPump[2]={ {2,2,6,6,0,17,{&mPmFront,&mPmSide,&mPmFront,&mPmSide,&mPmTop}}, {1,1,7,7,17,20,{&mPmCap,&mPmCap,&mPmCap,&mPmCap,&mPmTop}} };

// ---- the item table: letter, name, boxes (and the along-y variant), flags ----
enum { F_ROT = 1, F_ALONG = 2, F_FLAT = 4, F_HIGH = 8 };   // F_FLAT: walk over it; F_HIGH: stand on top (crates)
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
    { 'N', "BENCH", bxBenchU, 3, bxBenchV, 3, F_ALONG, 120 },
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
    { '"', "STREET LAMP", bxStreetLamp, 2, 0, 0, 0, 120 },
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
// every box face's texture, resolved once: box_ref[box][face] is its material + 1 in mat_tex (0: no face there)
#define MAXBOX 768
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
    { '=', 4 }, { 'L', 4 }, { 'N', 4 }, { 'J', 7 }, { 'X', 4 }, { 'O', 8 }, { 'Y', 8 }, { 'Z', 5 }, { 'K', 7 }, { 'M', 2 },   // the skate park: a rail 45 cm, a funbox 45 cm, a manual pad 20 cm
    { 'h', 9 }, { 'v', 12 }, { 'V', 6 }, { 'H', 18 },                                                                       // a chair 1 m, a TV on its stand 1.35 m, a lava lamp, a shower 2 m
    { '+', 22 }, { '"', 40 }, { 'k', 28 }, { 'a', 28 }, { 'g', 40 }, { '3', 36 }, { '6', 40 }, { 'u', 14 }, { 'z', 11 } };   // a bus stop 2.5 m, a street lamp 4.5 m, the flags, the trees 4 to 4.5 m
static IBox lifeBox[96]; static int nlifeBox;
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

static void tex_facex(const XV *a, const XV *b, const XV *c, const XV *d, const TexRef *tp, int dir) {   // tex_face on the fast path (dir: S E N W)
    if (!xv_vis4(a, b, c, d, 1)) return;
    const TexRef *t = tp;
    int s = sh6[dir], st = s + (s >> 4), sb = (s * box_ao) >> 8;
    u32 ct = gfx_grade_tex((u32)st * 0x010101u), cb = gfx_grade_tex((u32)sb * 0x010101u);
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
    if (fS) tex_facex(&v[2], &v[3], &v[6], &v[7], mS, 0);
    if (fE) tex_facex(&v[3], &v[1], &v[7], &v[5], mE, 1);
    if (fN) tex_facex(&v[1], &v[0], &v[5], &v[4], mN, 2);
    if (fW) tex_facex(&v[0], &v[2], &v[4], &v[6], mW, 3);
    // top: TL=(x0,zN) TR=(x1,zN) BL=(x0,zS) BR=(x1,zS); the texture turns with the item
    if (fT) {
        TexRef tt = *box_tex(ref, 4);
        if (!xv_vis4(&v[0], &v[1], &v[2], &v[3], 1)) return;
        int uv[4][2]; int cu[4] = { u0, u1, u0, u1 }, cv[4] = { v0, v0, v1, v1 };
        int aw = bx->a1 - bx->a0, bh = bx->b1 - bx->b0; if (aw < 1) aw = 1; if (bh < 1) bh = 1;
        static const int rcp[9] = { 0, 65536, 32768, 21846, 16384, 13108, 10923, 9363, 8192 };   // 65536 / n (boxes are 1..8 units)
        int ra = aw <= 8 ? rcp[aw] : 65536 / aw, rb = bh <= 8 ? rcp[bh] : 65536 / bh;
        for (int k = 0; k < 4; k++) {
            int la, lb; unrot(r, cu[k], cv[k], &la, &lb);
            int fu = ((la - bx->a0) * (tt.w - 1) * ra) >> 16, fv = ((lb - bx->b0) * (tt.h - 1) * rb) >> 16;
            uv[k][0] = tt.u + CLAMP(fu, 0, tt.w - 1); uv[k][1] = tt.v + CLAMP(fv, 0, tt.h - 1);
        }
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
static void sk_box(int x0, int x1, int y0, int y1, int z0, int z1, int r, int g, int b) {   // y0 top (smaller), z0 < z1
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
// ---- VEHICLES: toy-scale open models (the driver sits at the origin, the front is +z): a bike, a scooter, a sedan, a van, a sports car, a pickup ----
static const u8 vcol[6][3] = { { 200, 48, 48 }, { 56, 96, 190 }, { 232, 196, 60 }, { 70, 150, 90 }, { 236, 236, 232 }, { 40, 40, 48 } };
static void veh_wheel(int x, int z, int R, int wph) {   // a tyre (a box) with a bright hub mark that turns with the road
    sk_box(x - 14, x + 14, -2 * R, 0, z - R, z + R, 30, 30, 34);
    int hy = -R + ((isin(wph & 4095) * (R - 6)) >> 12), hz = z + ((icos(wph & 4095) * (R - 6)) >> 12);
    sk_box(x - (x < 0 ? 16 : 14), x + (x < 0 ? 14 : 16), hy - 4, hy + 4, hz - 4, hz + 4, 226, 226, 214);
}
static void veh_model(int t, int ci, int wph) {
    int r = vcol[ci % 6][0], g = vcol[ci % 6][1], b = vcol[ci % 6][2], dk = 36, gl[3] = { 130, 176, 214 };
    switch (t) {
    case 0:   // a bike: a frame, a seat, handlebars, two spoked wheels
        veh_wheel(0, -78, 34, wph); veh_wheel(0, 78, 34, wph);
        sk_box(-4, 4, -66, -58, -70, 74, r, g, b); sk_box(-4, 4, -66, -26, 60, 68, r, g, b); sk_box(-4, 4, -66, -30, -76, -68, r, g, b);
        sk_box(-12, 12, -76, -68, -60, -26, dk, dk, dk); sk_box(-34, 34, -100, -94, 56, 64, dk, dk, dk); sk_box(-4, 4, -100, -62, 58, 64, 150, 150, 160);
        break;
    case 1:   // a scooter: a deck, a stem, handlebars, small wheels
        veh_wheel(0, -86, 22, wph); veh_wheel(0, 90, 22, wph);
        sk_box(-26, 26, -30, -20, -100, 70, r, g, b); sk_box(-5, 5, -116, -26, 68, 76, 150, 150, 160); sk_box(-38, 38, -122, -114, 66, 80, dk, dk, dk);
        sk_box(-18, 18, -34, -30, 72, 100, 240, 240, 200);
        break;
    case 2:   // a sedan (open top): body, bonnet, boot, a windscreen, seats, lights
        veh_wheel(-114, -150, 32, wph); veh_wheel(114, -150, 32, wph); veh_wheel(-114, 150, 32, wph); veh_wheel(114, 150, 32, wph);
        sk_box(-104, 104, -64, -30, -230, 240, r, g, b); sk_box(-100, 100, -76, -64, 50, 236, r * 9 / 10, g * 9 / 10, b * 9 / 10); sk_box(-100, 100, -76, -64, -226, -90, r * 9 / 10, g * 9 / 10, b * 9 / 10);
        sk_box(-96, 96, -128, -76, 66, 76, gl[0], gl[1], gl[2]);                                      // the windscreen
        sk_box(-96, -86, -128, -76, 62, 80, r, g, b); sk_box(86, 96, -128, -76, 62, 80, r, g, b);
        sk_box(-60, 60, -84, -64, -54, 20, dk, dk, dk); sk_box(-60, 60, -112, -84, -54, -40, dk, dk, dk); sk_box(-60, 60, -84, -64, -150, -90, dk, dk, dk);
        sk_box(-90, -50, -60, -44, 236, 244, 255, 240, 160); sk_box(50, 90, -60, -44, 236, 244, 255, 240, 160); sk_box(-90, -50, -60, -44, -236, -228, 220, 40, 40); sk_box(50, 90, -60, -44, -236, -228, 220, 40, 40);
        break;
    case 3:   // a van: a short cab, a tall box behind
        veh_wheel(-114, -150, 34, wph); veh_wheel(114, -150, 34, wph); veh_wheel(-114, 110, 34, wph); veh_wheel(114, 110, 34, wph);
        sk_box(-108, 108, -70, -30, -300, 170, r, g, b); sk_box(-108, 108, -250, -70, -300, -50, r * 8 / 10 + 40, g * 8 / 10 + 40, b * 8 / 10 + 40);
        sk_box(-100, 100, -130, -70, 56, 66, gl[0], gl[1], gl[2]); sk_box(-100, -90, -130, -70, 52, 70, r, g, b); sk_box(90, 100, -130, -70, 52, 70, r, g, b);
        sk_box(-50, 50, -88, -70, -40, 30, dk, dk, dk); sk_box(-50, 50, -116, -88, -40, -26, dk, dk, dk);
        sk_box(-90, -50, -64, -48, 166, 174, 255, 240, 160); sk_box(50, 90, -64, -48, 166, 174, 255, 240, 160); sk_box(-80, 80, -170, -150, -304, -296, 230, 230, 230);
        break;
    case 4:   // a sports car: low and long, a nose, a spoiler
        veh_wheel(-110, -140, 28, wph); veh_wheel(110, -140, 28, wph); veh_wheel(-110, 130, 28, wph); veh_wheel(110, 130, 28, wph);
        sk_box(-100, 100, -50, -22, -190, 230, r, g, b); sk_box(-90, 90, -62, -50, 70, 220, r, g, b); sk_box(-80, 80, -60, -50, -180, -40, r, g, b);
        sk_box(-90, 90, -100, -50, 42, 50, gl[0], gl[1], gl[2]); sk_box(-60, 60, -76, -50, -40, 20, dk, dk, dk); sk_box(-60, 60, -92, -76, -40, -28, dk, dk, dk);
        sk_box(-84, 84, -88, -80, -200, -170, r, g, b); sk_box(-70, -62, -88, -50, -196, -184, dk, dk, dk); sk_box(62, 70, -88, -50, -196, -184, dk, dk, dk);
        sk_box(-90, -40, -50, -36, 226, 234, 255, 240, 160); sk_box(40, 90, -50, -36, 226, 234, 255, 240, 160);
        break;
    default:  // a pickup: a cab like the sedan's, an open bed behind
        veh_wheel(-116, -160, 36, wph); veh_wheel(116, -160, 36, wph); veh_wheel(-116, 120, 36, wph); veh_wheel(116, 120, 36, wph);
        sk_box(-106, 106, -70, -30, -280, 220, r, g, b); sk_box(-100, 100, -82, -70, 50, 216, r * 9 / 10, g * 9 / 10, b * 9 / 10);
        sk_box(-100, 100, -132, -82, 62, 72, gl[0], gl[1], gl[2]); sk_box(-100, -90, -132, -82, 58, 76, r, g, b); sk_box(90, 100, -132, -82, 58, 76, r, g, b);
        sk_box(-100, -92, -110, -70, -270, -50, r, g, b); sk_box(92, 100, -110, -70, -270, -50, r, g, b); sk_box(-100, 100, -110, -70, -276, -268, r, g, b);   // the bed's sides and tailgate
        sk_box(-60, 60, -92, -70, -40, 20, dk, dk, dk); sk_box(-60, 60, -118, -92, -40, -28, dk, dk, dk);
        sk_box(-96, -56, -64, -48, 214, 222, 255, 240, 160); sk_box(56, 96, -64, -48, 214, 222, 255, 240, 160);
        break;
    }
}
int dbg_vdraw, dbg_vq;
void items_draw_vehicle(int x, int z, int yaw, int y, int type, int ci, int wph) {
    dbg_vdraw++; dbg_vq = type * 1000 + ci;
    VECTOR p; setVector(&p, x, y, z); gfx_set_object(&p, yaw & 4095); sk_k = 28; veh_model(type, ci, wph); sk_k = 16; gfx_set_identity();
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
void items_draw(char ch, int mx, int my, int rot, int along, int t) {
    if (ch == 'P') return;
    shade6_update(t);
    if (ch == '|') { stairs_item(mx, my, rot); return; }
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
    for (int i = 0; i < n; i++) draw_box(&b[i], box_ref[base + i], r, ox, oz, t);
}
