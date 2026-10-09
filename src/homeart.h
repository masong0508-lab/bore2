// homeart.h - the HOME PACK furniture art: TV, bookshelf, coffee maker, aquarium, treadmill. HOST-ONLY, like simart.h (included from itembake.h).
// Same box + texture format. TV, bookshelf and treadmill face like the fridge (4 turns); the coffee maker and the aquarium are one sprite.

// ---- TV: a wooden stand, a boxy set with a blue screen and rabbit-ear aerials ----
static const u16 pTv[6]={RGB(10,6,3),RGB(19,12,6),RGB(6,6,9),RGB(11,12,15),RGB(8,17,29),RGB(22,28,31)};
MAT(mTvWood,pTv,1,1,"b")
MAT(mTvWoodF,pTv,6,4,"aaaaaa","bbbbbb","bbbbbb","aaaaaa")
MAT(mTvBody,pTv,1,1,"c")
MAT(mTvScr,pTv,6,10,"cccccc","cdddec","cdeeec","cdeeec","cdeeec","cdddec","cdddec","cdddec","cccccc","cacccc")
MAT(mTvTop,pTv,6,5,"cccccc","cdddcc","cccccc","cccccc","cccccc")
MAT(mTvAer,pTv,1,1,"d")
static const IBox bxTv[5]={
 {1,2,7,7,0,4,{&mTvWoodF,&mTvWood,&mTvWood,&mTvWood,&mTvWood}},
 {1,2,7,7,4,14,{&mTvScr,&mTvBody,&mTvBody,&mTvBody,&mTvTop}},
 {3,3,4,4,14,20,{&mTvAer,&mTvAer,&mTvAer,&mTvAer,&mTvAer}},
 {5,3,6,4,14,19,{&mTvAer,&mTvAer,&mTvAer,&mTvAer,&mTvAer}},
 {2,7,6,8,5,6,{&mTvAer,&mTvAer,&mTvAer,&mTvAer,&mTvAer}} };

// ---- bookshelf: a tall wooden case, four shelves of coloured books ----
static const u16 pBk[8]={RGB(9,5,2),RGB(19,12,6),RGB(27,9,8),RGB(9,16,27),RGB(10,22,10),RGB(29,25,9),RGB(22,10,24),RGB(28,28,26)};
MAT(mBkWood,pBk,1,1,"b")
MAT(mBkFront,pBk,6,18,"aaaaaa","cdecfg","cdecfg","cdfcfg","cdfcfg","aaaaaa","edcgfd","edcgfd","edfgfd","edfgfd","aaaaaa","dcfedc","dcfedc","dcfegc","dcfegc","aaaaaa","bbbbbb","aaaaaa")
MAT(mBkTop,pBk,6,4,"aaaaaa","abbbba","abbbba","aaaaaa")
static const IBox bxShelf[1]={{1,3,7,7,0,18,{&mBkFront,&mBkWood,&mBkWood,&mBkWood,&mBkTop}}};

// ---- coffee maker: dark base, tall back with a red light, glass jug ----
static const u16 pCf[6]={RGB(5,5,8),RGB(12,12,15),RGB(19,19,22),RGB(28,6,5),RGB(22,12,4),RGB(31,26,14)};
MAT(mCfBase,pCf,1,1,"a")
MAT(mCfBack,pCf,4,10,"bbbb","bcbb","bbbb","bbdb","bbbb","bbbb","bbbb","bbbb","bbbb","aaaa")
MAT(mCfSide,pCf,1,1,"b")
MAT(mCfTop,pCf,4,4,"aaaa","abba","abba","aaaa")
MAT(mCfJug,pCf,3,6,"eee","ede","eee","eee","eee","aaa")
MAT(mCfJugS,pCf,1,1,"e")
MAT(mCfJugT,pCf,3,2,"fff","fef")
static const IBox bxCoffee[4]={
 {2,2,6,6,0,2,{&mCfBase,&mCfBase,&mCfBase,&mCfBase,&mCfBase}},
 {2,4,6,6,2,12,{&mCfBack,&mCfSide,&mCfSide,&mCfSide,&mCfTop}},
 {2,2,6,5,10,12,{&mCfBase,&mCfSide,&mCfBase,&mCfSide,&mCfTop}},
 {3,2,5,4,2,8,{&mCfJug,&mCfJugS,&mCfJugS,&mCfJugS,&mCfJugT}} };

// ---- aquarium: a wooden stand, a tank of blue water with an orange fish, weed and gravel, a dark lid ----
static const u16 pAq[8]={RGB(9,5,2),RGB(19,12,6),RGB(6,15,26),RGB(10,22,30),RGB(31,15,4),RGB(8,22,9),RGB(26,22,14),RGB(5,5,8)};
MAT(mAqStand,pAq,1,1,"b")
MAT(mAqStandF,pAq,6,6,"aaaaaa","bbbbbb","bbbbbb","bbbbbb","bbbbbb","aaaaaa")
MAT(mAqFront,pAq,6,8,"cddccc","cccccc","ccecec","ceeecf","cccecc","cfcccf","cfcfcf","gggggg")
MAT(mAqSide,pAq,1,1,"c")
MAT(mAqLid,pAq,6,6,"hhhhhh","hdddch","hcccch","hcccch","hdddch","hhhhhh")
static const IBox bxAqua[3]={
 {1,2,7,7,0,6,{&mAqStandF,&mAqStand,&mAqStand,&mAqStand,&mAqStand}},
 {1,2,7,7,6,14,{&mAqFront,&mAqSide,&mAqSide,&mAqSide,&mAqLid}},
 {1,2,7,7,14,15,{&mAqStand,&mAqStand,&mAqStand,&mAqStand,&mAqLid}} };

// ---- treadmill: a flat belt, two rails, an upright with the display at the back (the front of the item faces the open floor) ----
static const u16 pTm[6]={RGB(5,5,8),RGB(13,13,16),RGB(22,22,25),RGB(28,6,5),RGB(8,22,10),RGB(26,28,30)};
MAT(mTmBelt,pTm,8,8,"aaaaaaaa","abbbbbba","aabbbbaa","abbbbbba","aabbbbaa","abbbbbba","aabbbbaa","aaaaaaaa")
MAT(mTmSide,pTm,1,1,"b")
MAT(mTmRail,pTm,1,1,"c")
MAT(mTmDisp,pTm,6,4,"aaaaaa","aeeeea","aeceea","aaaaaa")
MAT(mTmBack,pTm,1,1,"a")
static const IBox bxTread[5]={
 {1,0,7,8,0,3,{&mTmSide,&mTmSide,&mTmSide,&mTmSide,&mTmBelt}},
 {1,0,2,2,3,10,{&mTmRail,&mTmRail,&mTmRail,&mTmRail,&mTmRail}},
 {6,0,7,2,3,10,{&mTmRail,&mTmRail,&mTmRail,&mTmRail,&mTmRail}},
 {1,0,7,2,10,12,{&mTmDisp,&mTmBack,&mTmBack,&mTmBack,&mTmBack}},
 {1,6,7,8,0,1,{&mTmBack,&mTmBack,&mTmBack,&mTmBack,&mTmBack}} };
