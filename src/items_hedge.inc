// items_hedge.inc - the slim yard hedge and its pier (world version 7 fronts: src/lots7.inc yard7, field7). Hand written, no generator.
// The hedge is half a tile thick (4 of 8 units) and 1.3 m tall: a dark-to-light clipped body and a lighter, narrower cap (a stepped, rounded top), small
// blossoms and berries in the leaf. A body only blocks where it stands (items_hit), so a walker can brush past its two thin margins. Runs link along x or y
// (F_ALONG, as the bench and the rail). The pier (a gate post / corner piece) is a squared post with a berry ball. Ids 0x8E and 0x8F were kept free;
// item_cat() never lists them for sale (0x82..0x8F), so they are scenery only. The DS port draws the same boxes (docs/HOUSING_DESIGN.md "Slim hedges").
// (CH_HEDGE7 and CH_PIER7 are in items.h)
static const u16 pHg[6] = { RGB(3,11,4), RGB(5,17,6), RGB(8,24,8), RGB(11,29,10), RGB(30,8,10), RGB(30,27,12) };   // dark, mid, light, highlight green; berry; blossom
MAT(mHgSide, pHg, 8, 8, "cdccdccd", "ccdccfcc", "dcccdccc", "bcbbccbc", "bbabbbeb", "abbbabbb", "aababaab", "aaaaaaaa")
MAT(mHgCap, pHg, 8, 3, "cdccdccd", "dccfcdcc", "ccdccccd")
MAT(mHgTop, pHg, 8, 8, "cdcccdcc", "ccdcccdc", "dcccdccc", "ccdccccd", "cdcccfcc", "ccccdccc", "dccdcccc", "ccdccdcc")
MAT(mHgBall, pHg, 4, 4, "cdcc", "cecd", "dcec", "ccdc")
static const IBox bxHedge7U[2] = { {0,2,8,6,0,9,{&mHgSide,&mHgSide,&mHgSide,&mHgSide,&mHgTop}}, {0,3,8,5,9,12,{&mHgCap,&mHgCap,&mHgCap,&mHgCap,&mHgTop}} };
static const IBox bxHedge7V[2] = { {2,0,6,8,0,9,{&mHgSide,&mHgSide,&mHgSide,&mHgSide,&mHgTop}}, {3,0,5,8,9,12,{&mHgCap,&mHgCap,&mHgCap,&mHgCap,&mHgTop}} };
static const IBox bxPier7[3] = { {1,1,7,7,0,8,{&mHgSide,&mHgSide,&mHgSide,&mHgSide,&mHgTop}}, {2,2,6,6,8,10,{&mHgCap,&mHgCap,&mHgCap,&mHgCap,&mHgTop}}, {3,3,5,5,10,13,{&mHgBall,&mHgBall,&mHgBall,&mHgBall,&mHgBall}} };
#define ITEMS_HEDGE \
    { CH_HEDGE7, "CLIPPED HEDGE", bxHedge7U, 2, bxHedge7V, 2, F_ALONG, 40 },   \
    { CH_PIER7, "HEDGE PIER", bxPier7, 3, 0, 0, 0, 60 },
