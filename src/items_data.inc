// items_data.inc - BORE item art: textured boxes, straight from the GBA game (tools/sync_items.sh regenerates it).
// BORE room items as true 2:1 isometric sprite art (crate, rail, fridge, toilet, door mat, skateboard, spawn marker).
// Each item is a few textured boxes, rendered ONCE on the PC into small sprites that live in ROM (itemrom.h). They cost NO RAM at all.
// Local frame of an item: a = across, b = front (+b faces the viewer at r=0),
// 8 units per tile, 1 unit = 1 px wide, z in px. Textures are authored as seen from OUTSIDE the face, left to right, top to bottom.
// Letters a..h index the material palette, '.' = see-through.
typedef struct { const char*const*rows; u8 w,h; const u16*pal; } Mat;
typedef struct { u8 a0,b0,a1,b1,z0,z1; const Mat*m[5]; } IBox;   // faces: 0 front(+b) 1 +a 2 back 3 -a 4 top
typedef struct { const IBox*b; u8 n; } IObj;
#define MAT(nm,pal,w,h,...) static const char*const nm##_r[]={__VA_ARGS__}; static const Mat nm={nm##_r,w,h,pal};

// ---- crate (2 stacked wooden crates) ----
static const u16 pCr[4]={RGB(8,4,2),RGB(24,16,7),RGB(17,10,4),RGB(29,21,10)};
#define CR8 "aaaaaaaa","acbbbbca","abcbbcba","abbccbba","abbccbba","abcbbcba","acbbbbca","aaaaaaaa"
MAT(mCrS,pCr,8,16,CR8,CR8)
MAT(mCrT,pCr,8,8,"aaaaaaaa","adddddda","abbbbbba","acccccca","abbbbbba","abbbbbba","acccccca","aaaaaaaa")
static const IBox bxCrate[1]={{0,0,8,8,0,16,{&mCrS,&mCrS,&mCrS,&mCrS,&mCrT}}};

// ---- fridge (front gets the doors and handles) ----
static const u16 pFr[5]={RGB(17,18,21),RGB(31,31,31),RGB(23,24,27),RGB(7,8,12),RGB(28,6,6)};
MAT(mFrF,pFr,6,16,"aaaaaa","abbbba","abbbba","abbdda","abbdda","abbbba","acccca","abbbba","abbdda","abedda","abbdda","abbdda","abbbba","abbbba","abbbba","aaaaaa")
MAT(mFrS,pFr,6,16,"aaaaaa","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","abbbba","aaaaaa")
MAT(mFrT,pFr,6,6,"aaaaaa","abbbba","abbbba","abbbba","abbbba","aaaaaa")
static const IBox bxFridge[1]={{1,1,7,7,0,16,{&mFrF,&mFrS,&mFrS,&mFrS,&mFrT}}};

// ---- toilet: pedestal, bowl, tank at the back ----
static const u16 pTo[5]={RGB(17,18,22),RGB(31,31,31),RGB(23,24,26),RGB(9,17,27),RGB(12,13,16)};
MAT(mToBase,pTo,1,1,"c")
MAT(mToBowl,pTo,4,3,"bbbb","bbbb","cccc")
MAT(mToBowlT,pTo,4,4,"bbbb","bddb","bddb","bbbb")
MAT(mToTankF,pTo,4,5,"bbbb","bbbb","bbbb","bbbb","cccc")
MAT(mToTankS,pTo,2,5,"bb","bb","bb","bb","cc")
MAT(mToTankT,pTo,4,2,"bbeb","bbbb")
static const IBox bxToilet[3]={
 {3,4,5,6,0,2,{&mToBase,&mToBase,&mToBase,&mToBase,&mToBase}},
 {2,3,6,7,2,5,{&mToBowl,&mToBowl,&mToBowl,&mToBowl,&mToBowlT}},
 {2,1,6,3,3,8,{&mToTankF,&mToTankS,&mToTankF,&mToTankS,&mToTankT}} };

// ---- grind rail, kicker ramp, quarter pipe, ledge, bench: generated art (tools/make_skate_items.py) ----
#include "skateart.h"
// ---- bed, shower, sofa: the life-sim furniture ----
#include "simart.h"
// ---- home pack: TV, bookshelf, coffee maker, aquarium, treadmill ----
#include "homeart.h"
// ---- living and decor pack: rug, table, chair, desk, floor lamp, houseplant, dresser, fireplace, kitchen counter ----
#include "decorart.h"

// ---- door mat / threshold ----
static const u16 pDr[4]={RGB(8,5,3),RGB(18,11,6),RGB(24,16,8),RGB(28,20,10)};
MAT(mDrSide,pDr,1,1,"a")
MAT(mDrTop,pDr,8,8,"aaaaaaaa","abbbbbba","abccccba","abcddcba","abcddcba","abccccba","abbbbbba","aaaaaaaa")
static const IBox bxDoor[1]={{0,0,8,8,0,1,{&mDrSide,&mDrSide,&mDrSide,&mDrSide,&mDrTop}}};

// ---- skateboard pickup ----
static const u16 pBd[3]={RGB(6,6,9),RGB(30,13,5),RGB(31,30,26)};
MAT(mBdEdge,pBd,1,1,"b")
MAT(mBdTop,pBd,4,6,"bbbb","baab","baab","baab","baab","bbbb")
MAT(mBdWh,pBd,1,1,"c")
static const IBox bxBoard[5]={
 {2,2,3,3,2,3,{&mBdWh,&mBdWh,&mBdWh,&mBdWh,&mBdWh}}, {5,2,6,3,2,3,{&mBdWh,&mBdWh,&mBdWh,&mBdWh,&mBdWh}},
 {2,5,3,6,2,3,{&mBdWh,&mBdWh,&mBdWh,&mBdWh,&mBdWh}}, {5,5,6,6,2,3,{&mBdWh,&mBdWh,&mBdWh,&mBdWh,&mBdWh}},
 {2,1,6,7,3,4,{&mBdEdge,&mBdEdge,&mBdEdge,&mBdEdge,&mBdTop}} };

// ---- chill pack: water pipe, lava lamp, beanbag ----
// water pipe: a green glass base, a tall tube with a mouthpiece flare, a metal downstem and bowl on the side
static const u16 pGl[5]={RGB(5,16,9),RGB(10,24,14),RGB(16,29,20),RGB(24,31,27),RGB(14,14,16)};
MAT(mGlBase,pGl,4,5,"bbbb","bccb","bccb","bbbb","aaaa")
MAT(mGlBaseT,pGl,4,4,"abba","bddb","bddb","abba")
MAT(mGlTube,pGl,2,14,"dc","cc","cc","cb","cb","cb","cb","cb","cb","cb","cb","cb","cb","bb")
MAT(mGlTubeT,pGl,2,2,"aa","aa")
MAT(mGlStem,pGl,1,4,"e","e","e","e")
static const IBox bxPipe[4]={
 {2,2,6,6,0,5,{&mGlBase,&mGlBase,&mGlBase,&mGlBase,&mGlBaseT}},
 {3,3,5,5,5,19,{&mGlTube,&mGlTube,&mGlTube,&mGlTube,&mGlTubeT}},
 {2,2,6,6,19,20,{&mGlBase,&mGlBase,&mGlBase,&mGlBase,&mGlBaseT}},
 {5,4,7,5,5,9,{&mGlStem,&mGlStem,&mGlStem,&mGlStem,&mGlStem}},
};
// lava lamp: a dark metal cone, a glass body of purple fluid with orange blobs, a metal cap
static const u16 pLv[5]={RGB(6,6,9),RGB(12,12,15),RGB(14,4,20),RGB(31,15,4),RGB(31,26,10)};
MAT(mLvMet,pLv,3,3,"bbb","bab","aaa")
MAT(mLvBody,pLv,3,11,"ccc","cdc","ddc","ccc","ccc","cdd","cde","ccc","dcc","ddc","ccc")
MAT(mLvTop,pLv,3,3,"bab","aaa","bab")
static const IBox bxLava[3]={
 {2,2,6,6,0,3,{&mLvMet,&mLvMet,&mLvMet,&mLvMet,&mLvTop}},
 {3,3,6,6,3,14,{&mLvBody,&mLvBody,&mLvBody,&mLvBody,&mLvTop}},
 {3,3,6,6,14,16,{&mLvMet,&mLvMet,&mLvMet,&mLvMet,&mLvTop}},
};
// beanbag: a squashy purple seat and a slouchy back (faces the open floor, like the sofa)
static const u16 pBb[4]={RGB(10,4,14),RGB(17,8,22),RGB(23,12,27),RGB(27,18,30)};
MAT(mBbS,pBb,6,5,"cccccc","ccdccc","bcccdb","bbbbbb","aaaaaa")
MAT(mBbT,pBb,6,6,"bccccb","ccddcc","cdddcc","ccddcc","cccccc","bccccb")
MAT(mBbBack,pBb,6,4,"cdcccc","cccddc","bccccb","bbbbbb")
static const IBox bxBeanbag[2]={
 {1,2,7,8,0,5,{&mBbS,&mBbS,&mBbS,&mBbS,&mBbT}},
 {1,0,7,3,0,9,{&mBbBack,&mBbS,&mBbBack,&mBbS,&mBbT}},
};

// ---- DeadSet 3Thousand VYBE: a parody VR headset on a display bust (hand-traced pixel art, not boxes; the bust faces right, mirrored for the other way) ----
static const u16 pDs[15]={RGB(3,3,4),RGB(5,5,7),RGB(13,13,15),RGB(6,20,31),RGB(28,21,16),RGB(22,15,11),RGB(13,7,3),RGB(19,11,5),RGB(6,4,3),RGB(6,18,8),RGB(4,12,5),RGB(12,12,14),RGB(20,20,23),RGB(27,15,13),RGB(31,31,31)};
static const char*const dsArt[28]={
 "......aaaaaa.........",
 ".....aiiiiiiaa.......",
 "....aibbbbbbbia......",
 "...aiiiiiiibbbia.....",
 "...aiiiiiiiibbba.....",
 "..aiiiiiiiibbbbbba...",
 "..aiifeiiibbccccbba..",
 "..aiifeeiibcbbbbdcba.",
 "..aiifeeeibcbbbobbcba",
 "..aiieeeeebbccccccbba",
 "..aiieeeeeebbbbbbbbe.",
 "...aieeeeeeeeeeeeeen.",
 "...aiheeeeeeeeeeefna.",
 "...ahhgeeeeefhhhnna..",
 "...ahhhghheehhghhha..",
 "...ahghhhhghhhhhhga..",
 "....ahhhhghhhhghha...",
 "....aghhhhhhghhga....",
 "..aaajghhghhhhgjaa...",
 ".ajjjjjghhhhhgjjjja..",
 "ajjjjjjjgggggjjjjjja.",
 "akjjjjjjjjjjjjjjjjka.",
 ".akkkjjjjjjjjjjkkka..",
 "..aaakkkkkkkkkkaa....",
 ".......alllla........",
 "....aallmmmllllaa....",
 "...almmmmmmmmmlla....",
 "....aallllllllaa....."};
