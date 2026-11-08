// The Dreamcast furniture set (BORE_DC only; included by items.c). Letters 0xF7..0xFC, bought in the catalog like any other item.
static const u16 pDcW[5] = { RGB(29,29,28), RGB(30,15,3), RGB(20,21,23), RGB(5,5,8), RGB(8,14,28) };
MAT(mDcTop, pDcW, 8, 8, "caaaaaac", "aaabbbaa", "aabaaaba", "abaabbaa", "abababaa", "abaaaaba", "aabbbbaa", "caaaaaac")
MAT(mDcFront, pDcW, 8, 3, "aaaaaaaa", "acccaddd", "aaaaaaaa")
MAT(mDcSide, pDcW, 4, 3, "aaaa", "acca", "aaaa")
MAT(mDcPad, pDcW, 4, 3, "cccc", "cbdc", "cccc")
MAT(mDcPadS, pDcW, 1, 1, "c")
static const IBox bxDcConsole[2] = { {1,1,7,6,0,3,{&mDcFront,&mDcSide,&mDcSide,&mDcSide,&mDcTop}}, {2,6,6,8,0,1,{&mDcPad,&mDcPadS,&mDcPadS,&mDcPadS,&mDcPad}} };
static const u16 pVmu[5] = { RGB(8,8,10), RGB(29,29,28), RGB(14,18,12), RGB(4,5,4), RGB(30,15,3) };
MAT(mVmuBase, pVmu, 1, 1, "a")
MAT(mVmuPost, pVmu, 1, 1, "b")
MAT(mVmuFront, pVmu, 8, 6, "bbbbbbbb", "bcccccbb", "bcdcdccb", "bcccccbb", "bbbebbbb", "bbbbbbbb")
MAT(mVmuSide, pVmu, 1, 1, "b")
static const IBox bxVmuStand[3] = { {2,2,6,6,0,1,{&mVmuBase,&mVmuBase,&mVmuBase,&mVmuBase,&mVmuBase}}, {3,3,5,5,1,4,{&mVmuPost,&mVmuPost,&mVmuPost,&mVmuPost,&mVmuPost}},
                                    {2,3,6,5,4,8,{&mVmuFront,&mVmuSide,&mVmuSide,&mVmuSide,&mVmuSide}} };
static const u16 pSwRug[4] = { RGB(29,29,28), RGB(30,15,3), RGB(8,14,28), RGB(5,5,8) };
MAT(mSwRug, pSwRug, 8, 8, "cccccccc", "cbbbbbbc", "cbaaaabc", "cbabbabc", "cbabaabc", "cbabbbbc", "cbaaaaac", "cccccccc")
static const IBox bxSwRug[1] = { {0,0,8,8,0,1,{&mSwRug,&mSwRug,&mSwRug,&mSwRug,&mSwRug}} };
static const u16 pDcArc[6] = { RGB(5,5,8), RGB(8,14,28), RGB(30,15,3), RGB(29,29,28), RGB(10,26,31), RGB(24,6,10) };
MAT(mDcArcM, pDcArc, 8, 3, "abbbbbba", "abcdcdba", "abbbbbba")
MAT(mDcArcS, pDcArc, 8, 5, "aeeeeeea", "aeeaaeea", "aeaeeaea", "aeeaaeea", "aeeeeeea")
MAT(mDcArcC, pDcArc, 8, 4, "aaaaaaaa", "abcbbfba", "abbcbbba", "aaaaaaaa")
MAT(mDcArcSide, pDcArc, 2, 4, "bb", "bb", "cc", "bb")
MAT(mDcArcTop, pDcArc, 1, 1, "a")
static const IBox bxDcArcade[4] = { {1,2,7,7,0,5,{&mDcArcC,&mDcArcSide,&mDcArcSide,&mDcArcSide,&mDcArcTop}}, {1,2,7,6,5,11,{&mDcArcS,&mDcArcSide,&mDcArcSide,&mDcArcSide,&mDcArcTop}},
                                    {1,1,7,6,11,13,{&mDcArcM,&mDcArcSide,&mDcArcSide,&mDcArcSide,&mDcArcTop}}, {1,6,7,7,5,13,{&mDcArcSide,&mDcArcSide,&mDcArcSide,&mDcArcSide,&mDcArcSide}} };
static const u16 pLampO[4] = { RGB(8,8,10), RGB(30,15,3), RGB(31,24,8), RGB(12,12,14) };
MAT(mLampPole, pLampO, 1, 1, "d")
MAT(mLampBase, pLampO, 1, 1, "a")
MAT(mLampShadeO, pLampO, 4, 4, "bcbc", "cbcb", "bcbc", "cbcb")
static const IBox bxLampO[3] = { {2,2,6,6,0,1,{&mLampBase,&mLampBase,&mLampBase,&mLampBase,&mLampBase}}, {3,3,5,5,1,11,{&mLampPole,&mLampPole,&mLampPole,&mLampPole,&mLampPole}},
                                 {2,2,6,6,11,16,{&mLampShadeO,&mLampShadeO,&mLampShadeO,&mLampShadeO,&mLampShadeO}} };
static const u16 pLampB[4] = { RGB(8,8,10), RGB(8,14,28), RGB(14,26,31), RGB(12,12,14) };
MAT(mLampShadeB, pLampB, 4, 4, "bcbc", "cbcb", "bcbc", "cbcb")
static const IBox bxLampB[3] = { {2,2,6,6,0,1,{&mLampBase,&mLampBase,&mLampBase,&mLampBase,&mLampBase}}, {3,3,5,5,1,11,{&mLampPole,&mLampPole,&mLampPole,&mLampPole,&mLampPole}},
                                 {2,2,6,6,11,16,{&mLampShadeB,&mLampShadeB,&mLampShadeB,&mLampShadeB,&mLampShadeB}} };
#define ITEMS_DC \
    { (char)0xF7, "DREAMCAST", bxDcConsole, 2, 0, 0, F_ROT, 300 },   \
    { (char)0xF8, "VMU ON A STAND", bxVmuStand, 3, 0, 0, F_ROT, 120 },   \
    { (char)0xF9, "SWIRL RUG", bxSwRug, 1, 0, 0, F_FLAT, 90 },   \
    { (char)0xFA, "DC ARCADE CABINET", bxDcArcade, 4, 0, 0, F_ROT, 1200 },   \
    { (char)0xFB, "ORANGE SWIRL LAMP", bxLampO, 3, 0, 0, 0, 110 },   \
    { (char)0xFC, "BLUE SWIRL LAMP", bxLampB, 3, 0, 0, 0, 110 },
