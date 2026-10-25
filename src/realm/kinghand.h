// kinghand.h - EP6 THE LAND BOREFORE TIME, Phase 4 step d: BUILDING BY HAND. PURE C (needs kingdom.h and kingsim.h before it; no hardware).
// The cursor is the cell under the middle of the screen (the D-pad scrolls the map under it), so there is no cursor to move: the camera is the cursor.
// A "mode" is what A does there: lay or lift a ROAD, put a building SITE down (workers carry its goods and raise it), or DEMOLISH what stands there.
// NOTE (step 4c): main.c #defines W, D and H. No single capital letters as names here.
#ifndef KINGHAND_H
#define KINGHAND_H
enum { KHM_HAND, KHM_ROAD, KHM_HUT, KHM_HOUSE, KHM_WOODCUT, KHM_QUARRY, KHM_SAWMILL, KHM_MASON, KHM_STORE, KHM_SHEEP, KHM_CRYMINE, KHM_WEAVER, KHM_CUTTER, KHM_CASTLE, KHM_WELL, KHM_TAVERN, KHM_DEMO, KHM_MOVE, KHM_NUBBIN, KHM_N };   // KHM_HUT + building type = the building's mode; KHM_HAND (the first) gathers by hand and carries goods (step 4h); MOVE lifts and puts down buildings; NUBBIN lifts and puts down Nubbins
enum { KHR_NONE, KHR_ROAD, KHR_UNROAD, KHR_SITE, KHR_RAZED, KHR_GOT, KHR_NOTHING, KHR_SPOT, KHR_FULL, KHR_ARG, KHR_NOGATHER,
             KHR_TAKE, KHR_GIVE, KHR_BACK, KHR_NOLOAD, KHR_NOWANT, KHR_REFUSE, KHR_ENOUGH, KHR_BUSY,                       // step 4h, goods: load taken, delivered, put back; nothing to carry; no one asks; not wanted here; has enough; hands full
             KHR_LIFT, KHR_SET,                                                                                              // a building lifted, put down (the map changed, redraw it: khRedraw)
             KHR_HEAD, KHR_SETTLE, KHR_NOBODY, KHR_HOMEFULL, KHR_STAFFED, KHR_LASTHAND, KHR_NOTYET, KHR_NOWORK, KHR_KEEP, KHR_NOCUT, KHR_N };   // Nubbins: picked up, put down, none here, home full, already staffed, nobody left to carry, not built yet, no work for him there
// (KHR_ROAD to KHR_RAZED, KHR_LIFT and KHR_SET: the map changed, redraw it. See khRedraw.)   // what an A press did
typedef struct { int mode; } KHand;
static const char* const khName[KHM_N]={ "HAND", "ROAD", "HUT", "HOUSE", "WOODCUTTER", "QUARRY", "SAWMILL", "MASON", "STOREHOUSE", "SHEEP PEN", "MINE", "WEAVER", "CUTTER", "CASTLE", "WELL", "TAVERN", "DEMOLISH", "MOVE", "NUBBIN" };
static const char* const khMsg[KHR_N]={ "", "ROAD LAID", "ROAD LIFTED", "SITE PLACED", "DEMOLISHED", "GATHERED", "NOTHING HERE", "CANNOT BUILD HERE", "NO ROOM FOR MORE", "OUT OF THE REALM", "NOTHING TO GATHER",
    "LOAD TAKEN", "LOAD DELIVERED", "LOAD PUT BACK", "NOTHING TO CARRY HERE", "NOBODY HERE WANTS GOODS", "NOT WANTED HERE", "ALREADY HAS ENOUGH", "YOUR HANDS ARE FULL",
    "BUILDING LIFTED", "BUILDING PUT DOWN",
    "NUBBIN PICKED UP", "NUBBIN PUT DOWN", "NO NUBBIN HERE", "THAT HOME IS FULL", "ALREADY HAS ENOUGH WORKERS", "EVERY NUBBIN IS BUSY", "IT IS NOT BUILT YET", "NO JOB FOR A NUBBIN THERE", "THE CASTLE STAYS PUT", "A BRIDGE NEEDS CUT STONE" };
static inline void khInit(KHand*B){ B->mode=KHM_HAND; }
static inline void khCycle(KHand*B,int dir){ B->mode=(B->mode+(dir>0?1:KHM_N-1))%KHM_N; }
// the cell under the middle of the screen, from the camera in pixels (the screen is 240 x 160: its middle is 120, 80)
static inline int khCx(int camx){ return (camx+120)>>4; }   // (a cell is 16 pixels)
static inline int khCy(int camy){ return (camy+80)>>4; }
// the building whose 2 x 2 footprint covers cell (x,y), or -1
static inline int khBldAt(const KSim*S,int x,int y){
    for(int i=0;i<S->R->nB;i++){ const KBld*b=&S->B[i]; if(x>=b->x&&x<b->x+kgFootW(b->type)&&y>=b->y&&y<b->y+kgFootH(b->type)) return i; }
    return -1; }
// (step 4h) pick up and put down: kinghold.h, included at the end of this file
static inline int khGoods(KSim*S,int i,int x,int y,int giant);
static inline int khHoldAct(KSim*S,int mode,int x,int y);
static inline int khRedraw(int r){ return (r>=KHR_ROAD&&r<=KHR_RAZED)||r==KHR_LIFT||r==KHR_SET; }   // did this press change what the map shows (apart from KHR_GOT, one cell)?
// ---- the giant (step 4g): its stats, and gathering by hand. The five giants of KRealm.giant: A the all-rounder, B the runner, C the lifter, D the builder, then your own Bore (average). [ASSUMED values, tune freely] ----
// step 5e: ONE table for the four stats of the roadmap's section 4.3, a point up on one, a point down on another, so none is best. Columns: stride (map pixels a frame), carry (goods in a load, kinghold.h),
// work (goods one hand-gathered tree, rock or crystal gives; a woodcutter gives 1 and takes 90 frames), build (1 plain; 2 or more: a site the giant's hands finish gives back build-1 goods, see khGoods)
//                                        stride carry work build
static const unsigned char khStatT[5][4]={ {3, 5, 2, 1},    // A the all-rounder
                                           {4, 4, 2, 1},    // B the runner: faster, carries less
                                           {2, 7, 2, 1},    // C the lifter: slower, carries more
                                           {3, 5, 1, 2},    // D the builder: gathers less, builds better
                                           {3, 5, 2, 1} };  // your own Bore: average until Phase 7 sets it from the Sim's skills
static inline int khStat(int giant,int col){ return khStatT[giant>=0&&giant<5?giant:0][col&3]; }
static inline int khStride(int giant){ return khStat(giant,0); }
static inline int khRun(int giant){ int s=khStride(giant); return s+(s+1)/2; }   // B held: 3 -> 5, 4 -> 6, 2 -> 3 (always under 8, so a frame never skips a cell)
static inline int khYield(int giant){ return khStat(giant,2); }
static inline int khBuild(int giant){ return khStat(giant,3); }
// the giant gathers what is on cell (x,y): a tree, a rock or a crystal becomes grass and its goods go to the stockpile (ksGift, so the books stay balanced)
static inline int khWork(KSim*S,int x,int y){
    int d=S->dim; if(x<0||y<0||x>=d||y>=d) return KHR_ARG;
    int c=S->cells[y*d+x]; if(c&KC_BLD) return KHR_NOGATHER; int v=c&7;
    int g=v==KT_FOREST?G_WOOD:v==KT_ROCK?G_STONE:v==KT_CRYSTAL?G_CRYSTAL:-1; if(g<0) return KHR_NOGATHER;
    S->cells[y*d+x]=KT_GRASS; ksGift(S,g,khYield(S->R->giant)); return KHR_GOT;
}
// A pressed in a mode on cell (x,y). A building's top-left corner is the cursor cell. Returns a KHR_ code; the caller redraws the map when it is not a refusal.
static inline int khAct(KHand*B,KSim*S,int x,int y){
    int d=S->dim; if(x<0||y<0||x>=d||y>=d) return KHR_ARG;
    if(B->mode==KHM_HAND){ int i=khBldAt(S,x,y); if(i>=0) return khGoods(S,i,x,y,S->R->giant); return khWork(S,x,y); }   // (step 4h: on a building the hand carries goods)
    if(B->mode==KHM_MOVE||B->mode==KHM_NUBBIN) return khHoldAct(S,B->mode,x,y);
    if(B->mode==KHM_DEMO){ int i=khBldAt(S,x,y); if(i<0) return KHR_NOTHING; if(S->B[i].type==KB_CASTLE) return KHR_KEEP; return ksDemolish(S,i)==KSE_OK?KHR_RAZED:KHR_ARG; }
    if(B->mode==KHM_ROAD){ int c=S->cells[y*d+x];
        if(c&KC_BLD) return KHR_SPOT;
        if(ksRoadV(c&7)){ return ksRoadSet(S,x,y,0)==KSE_OK?KHR_UNROAD:KHR_ARG; }
        int e=ksRoadSet(S,x,y,1); return e==KSE_OK?KHR_ROAD:e==KSE_CAP?KHR_FULL:e==KSE_ARG?KHR_ARG:e==KSE_POOR?KHR_NOCUT:KHR_SPOT; }
    { int e=ksPlace(S,B->mode-KHM_HUT,x,y,0); return e>=0?KHR_SITE:e==KSE_CAP?KHR_FULL:e==KSE_ARG?KHR_ARG:KHR_SPOT; }
}
// the cursor's look: bank 15 of the palette is bank 0 brightened, so a blinking cell shows where A will act
static inline u16 khPalCur(u16 c){ int r=c&31, g=(c>>5)&31, b=(c>>10)&31; r+=(31-r)*2/3; g+=(31-g)*2/3; b+=(31-b)*2/3; return (u16)(r|(g<<5)|(b<<10)); }

// ---- the text layer (step 4e): two lines of text over the map, as the tiles of a second background (BG1) ----
// What is under the cursor, as text, and the one-line status; both fit 38 characters (the strip is 240 pixels). No libc: the two helpers below write numbers and words.
static inline char* khCat(char*d,const char*s){ while(*s) *d++=*s++; *d=0; return d; }
static inline char* khNum(char*d,long v){ char t[12]; int n=0; if(v<0){ *d++='-'; v=-v; } do{ t[n++]=(char)('0'+v%10); v/=10; }while(v&&n<11); while(n) *d++=t[--n]; *d=0; return d; }
static const char* const khGood[8]={ "WOOD", "STONE", "WOOL", "CRYSTAL", "PLANKS", "CUT STONE", "CLOTH", "POLISHED" };   // (moved up in step 5b: the site text names the castle's goods)
static const char* const khTerrain[8]={ "GRASS", "FOREST", "ROCK", "DEEP WATER", "CRYSTAL", "MEADOW", "ROAD", "BRIDGE" };
static inline void khInfo(const KSim*S,int x,int y,char*out){   // out: at least 48 bytes
    int d=S->dim; char*e=out; *e=0;
    if(x<0||y<0||x>=d||y>=d){ khCat(out,"THE EDGE OF THE WORLD"); return; }
    int i=khBldAt(S,x,y);
    if(i>=0){ const KBld*b=&S->B[i]; const KsType*T=ksT(b->type); e=khCat(e,khName[KHM_HUT+b->type]);
        if(!b->lvl){ int cg=0,cn=0; ksCost(S,b,&cg,&cn);
            if(T->kind==KK_CASTLE){ e=khCat(e,"  STAGE "); e=khNum(e,S->R->castle+1); e=khCat(e," OF 4  "); e=khCat(e,khGood[cg<8?cg:0]); e=khCat(e," "); }   // "CASTLE  STAGE 2 OF 4  CUT STONE 3 OF 8"
            else e=khCat(e,"  SITE  GOODS ");
            e=khNum(e,ksIn(b)); e=khCat(e," OF "); khNum(e,cn); }
        else if(T->kind==KK_CASTLE){ khCat(e,"  CROWNED"); }
        else if(T->kind==KK_HOME){ e=khCat(e,"  NUBBINS "); e=khNum(e,b->work); e=khCat(e," OF "); khNum(e,T->house); }
        else if(T->kind==KK_STORE){ khCat(e,"  STOCKPILE"); }
        else if(T->kind==KK_COMFORT){ khCat(e,"  SPEEDS WORK NEARBY"); }   // (step 5g)
        else { e=khCat(e,b->work?"  WORKING":"  NO WORKER"); if(S->cmf[i]) khCat(e,S->cmf[i]>1?"  MUCH FASTER":"  FASTER"); }
        return; }
    { int v=S->cells[y*d+x]&7; khCat(out,khTerrain[v<8?v:0]); }
}
static inline long khDay(u32 day){ return day>99999u?99999L:(long)day; }   // (step 5f) the day for the screen: capped, and never negative where a long is 32 bits
static inline void khStatus(char*out,int mode,long day,int pop,int spd){   // "ROAD   DAY 12  NUBBINS 7  X4"
    char*e=khCat(out,khName[mode]); e=khCat(e,"   DAY "); e=khNum(e,day>99999?99999:day); e=khCat(e,"  NUBBINS "); e=khNum(e,pop); e=khCat(e,"  X"); khNum(e,spd==0?1:spd==1?4:16);
}
// the status line while the giant carries something: no speed, so it still fits ("STOREHOUSE  A STOREHOUSE  DAY 99999"); khHeld (kinghold.h) says what it holds
static inline void khStatus2(char*out,int mode,long day,int pop,int spd,const char*held){
    if(!held||!held[0]){ khStatus(out,mode,day,pop,spd); return; }
    char*e=khCat(out,khName[mode]); e=khCat(e,"  "); e=khCat(e,held); e=khCat(e,"  DAY "); khNum(e,day>99999?99999:day);
}
// A strip of the screen (240 x 16 pixels of the game's 15 bit bitmap, row stride 240) as 60 tiles of 4 bits a pixel, 16 half words a tile, tile (tx,ty) at index ty*30+tx.
// The text was drawn in white on black, anti-aliased: a pixel that is mostly white becomes colour 1 (the white letter; gold, 3, on the first line), a half-lit one a softer 4 (5), a black one touching a letter above, below or beside becomes colour 2
// (its dark outline, so it reads on any ground), the rest stays 0 (see through). A tile row is 8 pixels, low nibble first.
#define KH_TILES 60
static inline int khLum(const u16*fb,int x,int y){ if(x<0||y<0||x>=240||y>=16) return 0; u16 px=fb[y*240+x]; return (px&31)+((px>>5)&31)+((px>>10)&31); }   // 0..93 (no table: nothing here may take RAM)
static inline int khOn(const u16*fb,int x,int y){ return khLum(fb,x,y)>=62; }   // the body of a stroke: two thirds white or more
static inline void khRowLum(const u16*fb,int y,u8*d){ if(y<0||y>15){ for(int x=0;x<240;x++) d[x]=0; return; } const u16*p=fb+y*240; for(int x=0;x<240;x++){ u16 c=p[x]; d[x]=(u8)((c&31)+((c>>5)&31)+((c>>10)&31)); } }
static inline void khStrip(const u16*fb,u16 shadow,u16*out){   // (the font is anti-aliased and small: a stroke edge keeps a softer tone, 4 and 5, so the letters do not thin out)
    (void)shadow; u8 ring[3][240]; u8*L0=ring[0], *L1=ring[1], *L2=ring[2];   // the rows above, at and below the one being made: each row's light is worked out once (this ran about 25 reads a pixel and took two frames)
    for(int i=0;i<KH_TILES*16;i++) out[i]=0;
    khRowLum(fb,-1,L0); khRowLum(fb,0,L1); khRowLum(fb,1,L2);
    for(int y=0;y<16;y++){ int ty=y>>3, r=y&7;
        for(int x=0;x<240;x++){ int l=L1[x]; unsigned n;
            if(l>=62) n=ty==0?3u:1u;   // (the first line gold, the second white: palette colours 3 and 1 of the text bank)
            else if(l>=24) n=ty==0?5u:4u;   // (the edge of a stroke: a softer gold or white)
            else n=((x>0&&L1[x-1]>=62)||(x<239&&L1[x+1]>=62)||L0[x]>=62||L2[x]>=62)?2u:0u;   // (the dark outline, so it reads on any ground)
            if(n) out[(ty*30+(x>>3))*16+r*2+((x&7)>>2)]|=(u16)(n<<((x&3)*4)); }
        u8*t=L0; L0=L1; L1=L2; L2=t; khRowLum(fb,y+2,L2); }
}

// ---- the build menu (step 4f): one line a mode, with what it costs and what the stockpile holds ----
static inline void khCost(char*out,const KSim*S,int mode){   // out: at least 48 bytes. "SAWMILL  4 WOOD  HAVE 12", "ROAD  FREE", "DEMOLISH  GOODS COME BACK"
    char*e=khCat(out,khName[mode]);
    if(mode==KHM_HAND){ khCat(e,"  GATHER AND CARRY GOODS"); return; }
    if(mode==KHM_MOVE){ khCat(e,"  LIFT AND PUT DOWN  FREE"); return; }
    if(mode==KHM_NUBBIN){ khCat(e,"  MOVE A RESIDENT OR WORKER"); return; }
    if(mode==KHM_ROAD){ khCat(e,"  FREE"); return; }
    if(mode==KHM_DEMO){ khCat(e,"  GOODS COME BACK"); return; }
    int g=0, n=0; ksCostType(S,mode-KHM_HUT,0,&g,&n); if(g>7) g=0; long have=S->R->goods[g];
    if(mode-KHM_HUT==KB_CASTLE&&S->R->castle>=KG_CASTLE){ khCat(e,"  ALREADY CROWNED"); return; }
    e=khCat(e,"  "); e=khNum(e,n); e=khCat(e," "); e=khCat(e,khGood[g]); e=khCat(e,"  HAVE "); khNum(e,have>99999?99999:have);
}
#include "kinghold.h"   // Phase 4h: pick up and put down (goods, buildings, Nubbins); needs kinglog.h before this file
#endif
