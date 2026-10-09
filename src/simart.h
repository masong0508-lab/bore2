// simart.h - life-sim furniture for BORE: BED (S), SHOWER (H), SOFA (C). Same box + texture format as items.h.
// All three are faced like the fridge: away from the wall, toward open floor. Included from items.h after skateart.h.

// ---- bed: headboard at the back, blue blanket, white pillow ----
static const u16 pBed[5]={RGB(10,6,3),RGB(8,12,26),RGB(14,18,29),RGB(30,30,28),RGB(18,11,5)};
MAT(mBeWood,pBed,1,1,"e")
MAT(mBeSide,pBed,1,1,"b")
MAT(mBeBlan,pBed,6,7,"bbbbbb","cccccc","bbbbbb","bbbbbb","cccccc","bbbbbb","bbbbbb")
MAT(mBePil,pBed,4,2,"dddd","dddd")
MAT(mBePilS,pBed,1,1,"d")
static const IBox bxBed[4]={
 {0,0,8,1,0,10,{&mBeWood,&mBeWood,&mBeWood,&mBeWood,&mBeWood}},                 // headboard
 {0,1,8,8,0,3,{&mBeWood,&mBeWood,&mBeWood,&mBeWood,&mBeWood}},                  // frame
 {1,1,7,8,3,5,{&mBeSide,&mBeSide,&mBeSide,&mBeSide,&mBeBlan}},                  // mattress + blanket
 {2,1,6,3,5,7,{&mBePilS,&mBePilS,&mBePilS,&mBePilS,&mBePil}} };                 // pillow

// ---- shower: tray, tiled back wall, shower head, glass door in front (see-through inside the frame) ----
static const u16 pShw[5]={RGB(24,25,28),RGB(26,30,31),RGB(15,22,27),RGB(30,31,31),RGB(9,9,13)};
MAT(mShTray,pShw,1,1,"d")
MAT(mShFrame,pShw,1,1,"a")
MAT(mShHead,pShw,1,1,"e")
MAT(mShWall,pShw,8,16,"cccccccc","cccccccc","cccccccc","dddddddd","cccccccc","cccccccc","cccccccc","dddddddd",
                    "cccccccc","cccccccc","cccccccc","dddddddd","cccccccc","cccccccc","cccccccc","dddddddd")
MAT(mShGlass,pShw,8,16,"aaaaaaaa","a......a","a.b....a","a.b....a","a......a","a......a","a....b.a","a....b.a",
                      "a......a","a......a","a......a","a.b....a","a.b....a","a......a","a......a","aaaaaaaa")
static const IBox bxShower[4]={
 {0,0,8,8,0,2,{&mShTray,&mShTray,&mShTray,&mShTray,&mShTray}},
 {0,0,8,1,2,16,{&mShWall,&mShWall,&mShWall,&mShWall,&mShFrame}},
 {3,1,5,3,12,14,{&mShHead,&mShHead,&mShHead,&mShHead,&mShHead}},
 {0,7,8,8,2,16,{&mShGlass,&mShFrame,&mShGlass,&mShFrame,&mShFrame}} };

// ---- sofa: tall back, two arms, cushion ----
static const u16 pSofa[4]={RGB(12,7,3),RGB(25,17,8),RGB(30,23,12),RGB(19,12,5)};
MAT(mSoBack,pSofa,1,1,"b")
MAT(mSoArm,pSofa,1,1,"b")
MAT(mSoArmT,pSofa,1,1,"c")
MAT(mSoSeatF,pSofa,6,5,"bbbbbb","bccccb","bccccb","bbbbbb","dddddd")
MAT(mSoSeatT,pSofa,6,6,"bbbbbb","bccccb","bccccb","bccccb","bccccb","bbbbbb")
static const IBox bxSofa[4]={
 {0,0,8,2,0,10,{&mSoBack,&mSoBack,&mSoBack,&mSoBack,&mSoArmT}},
 {1,2,7,8,0,5,{&mSoSeatF,&mSoArm,&mSoArm,&mSoArm,&mSoSeatT}},
 {0,2,1,8,0,7,{&mSoArm,&mSoArm,&mSoArm,&mSoArm,&mSoArmT}},
 {7,2,8,8,0,7,{&mSoArm,&mSoArm,&mSoArm,&mSoArm,&mSoArmT}} };
