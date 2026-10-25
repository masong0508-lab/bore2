// kingsim.h - EP6 THE LAND BOREFORE TIME, Phase 3 step a: the realm's ECONOMY (buildings, goods, Nubbins, the land giving and growing back).
// Pure C on top of kingdom.h (needs u8 u16 u32 and kingdom.h before it): no screen, no save memory, no EWRAM, so tools/test_kingsim.c runs it on a PC.
// kinglog.h (step b) adds the carriers and the roads' distance maps; this file keeps their STATE in KSim so one struct holds the whole simulation.
//
// THE RULES (all numbers are in the tables below, none of them are in a saved realm, so they can be tuned without breaking saves):
//  - A building is a BLUEPRINT (level 0) until its cost (one good) has been carried to it, then it is built (level 1) and the cost is used up.
//  - Homes (hut 4, house 8 Nubbins) take one new Nubbin a day. The founding party is KS_PARTY Nubbins with no home, so a new realm can build.
//  - A quarter of the Nubbins (and one) stay free to carry; the rest staff the gatherers and processors (b->work = 1 staffed).
//  - A staffed gatherer fells a tree / quarries a rock in its reach every `period` frames and holds up to KS_BUF goods; a working woodcutter also plants.
//  - A staffed processor turns 2 of its input into 1 output every `period` frames. A storehouse is the way into and out of the shared stockpile
//    (R->goods): no storehouse, no stockpile (goods already in it wait there).
//  - Goods are never made or lost without the books knowing: S->made[g] is what SHOULD exist (gifts and production minus what is used up or lost);
//    ksLedger checks it against what does exist (stockpile + buildings + carriers).
//  - b->stock in a saved building: low byte = goods delivered to it (a blueprint's cost, a processor's input), high byte = goods it holds to send out.
// Needs: nothing but kingdom.h. Every function is static inline.
#define KS_MAXB    64       // buildings the simulation runs (the save format allows 250; a bigger realm than this is refused on load: KSE_CAP). 40 until the big realms: 64 holds a town of 48 houses and the crowds of THE LAND BOREFORE TIME's last missions
#ifndef KS_MAXN
#define KS_MAXN    512      // road cells (a build may change it with -DKS_MAXN=...: the road list, the queue and every distance map shrink or grow with it)
#endif
#ifndef KS_MAPS
#define KS_MAPS    12       // distance maps kept at once (kinglog.h). Each is KS_MAXN bytes. 8 starved a 30 building realm in the test; 10 was enough for 40 buildings, 12 is for 64
#endif
#define KS_MAXW    40       // carriers
#define KS_POPCAP  400      // the most a realm can hold: 50 houses of 8 (64 buildings leave room for them and the castle, the storehouses and the workplaces); the homes are the real limit; kingfx.h draws all of them (crowds) at any count
#define KS_PARTY   4        // the founding party
#define KS_BUF     8        // goods a building holds
#define KS_DAYT    600      // frames in a realm day
#define KS_TMAX    (KS_DAYT*7158278u)   // the frame clock S->t counts 0 .. KS_TMAX-1 and starts again: a whole number of days (and of KS_WALK steps), so a day never comes early or late at the wrap, and it fits a u32 (step 5f)
#define KS_GR      6        // a gatherer's reach: this many cells each way from the middle of its 2 x 2
#define KS_CR      8        // (step 5g) a finished well or tavern helps the producers whose 2 x 2 is within this many cells (each way) of its own 2 x 2
#define KS_REGROW  12       // tries a day a working woodcutter makes to plant a tree on bare grass in its reach
enum { KB_HUT, KB_HOUSE, KB_WOODCUT, KB_QUARRY, KB_SAWMILL, KB_MASON, KB_STORE, KB_SHEEP, KB_CRYMINE, KB_WEAVER, KB_CUTTER, KB_CASTLE, KB_WELL, KB_TAVERN, KS_NTYPES };   // (step 5a: the wool chain and the crystal chain; step 5g: the well and the tavern, numbers 12 and 13, after the castle so that no saved building changes its number)
enum { KK_HOME, KK_GATHER, KK_PROC, KK_STORE, KK_CASTLE, KK_COMFORT };
enum { G_WOOD, G_STONE, G_WOOL, G_CRYSTAL, G_PLANKS, G_CUT, G_CLOTH, G_POLISH };
enum { KSE_OK=0, KSE_ARG=-1, KSE_CAP=-2, KSE_SPOT=-3, KSE_ROAD=-4, KSE_POOR=-5 };   // (KSE_POOR, step 5d: not enough cut stone for a bridge)
typedef struct { u8 kind, costG, costN, staff, house, inG, outG, res, period; } KsType;
static const KsType KS_T[KS_NTYPES]={
 /* kind     costG    costN staff house inG     outG     resource   period */
 { KK_HOME,   G_WOOD,   3, 0, 4, 0,       0,       0,         0   },   // hut
 { KK_HOME,   G_PLANKS, 3, 0, 8, 0,       0,       0,         0   },   // house
 { KK_GATHER, G_WOOD,   2, 1, 0, 0,       G_WOOD,  KT_FOREST, 90  },   // woodcutter
 { KK_GATHER, G_WOOD,   3, 1, 0, 0,       G_STONE, KT_ROCK,   150 },   // quarry
 { KK_PROC,   G_WOOD,   4, 1, 0, G_WOOD,  G_PLANKS,0,         120 },   // sawmill
 { KK_PROC,   G_PLANKS, 3, 1, 0, G_STONE, G_CUT,   0,         150 },   // mason
 { KK_STORE,  G_WOOD,   3, 0, 0, 0,       0,       0,         0   },   // storehouse
 { KK_GATHER, G_WOOD,   3, 1, 0, 0,       G_WOOL,  KT_MEADOW, 120 },   // sheep pen: wool from the meadow in reach; the sheep graze, the meadow is not used up [ASSUMED]
 { KK_GATHER, G_PLANKS, 3, 1, 0, 0,       G_CRYSTAL,KT_CRYSTAL,180 },  // crystal mine: a crystal cell in reach becomes grass, like a quarried rock [ASSUMED]
 { KK_PROC,   G_PLANKS, 3, 1, 0, G_WOOL,  G_CLOTH, 0,         140 },   // weaver: 2 wool make 1 cloth
 { KK_PROC,   G_CUT,    3, 1, 0, G_CRYSTAL,G_POLISH,0,         180 },   // crystal cutter: 2 crystal make 1 polished crystal
 { KK_CASTLE, G_PLANKS, 6, 0, 0, 0,       0,       0,         0   },   // castle (step 5b): the cost here is only the first lot of stage 1; the real costs are KS_CL, by stage
 { KK_COMFORT,G_STONE,  4, 0, 0, 0,       0,       0,         0   },   // well (step 5g) [ASSUMED]: no worker, no goods in or out; every working producer in KS_CR cells makes its goods 1/8 faster
 { KK_COMFORT,G_PLANKS, 6, 0, 0, 0,       0,       0,         0   },   // tavern (step 5g) [ASSUMED]: the same, and a producer with both a well and a tavern in reach is 2/8 faster
};
static inline const KsType*ksT(int type){ return &KS_T[type<KS_NTYPES?type:0]; }

typedef struct { u16 cell; u8 st, src, dst, good, md; u16 stall; } KsWk;   // a carrier (kinglog.h): st 0 free, 1 walking with a good
enum { KHD_NONE, KHD_GOODS, KHD_BLD, KHD_NUB };   // (step 4h) what the giant carries: nothing, goods, a lifted building, a Nubbin
typedef struct { u8 k, g, n, bt, bl, bw; } KHold;   // the giant's hold, ONE thing at a time. goods: g the good, n how many. building: bt type, bl level, bw workers or residents. Nubbin: bt 1 a resident, 0 a worker. Never saved (kpSave sends it home first)
typedef struct {
    KRealm*R; KBld*B; u8*cells; int dim;
    u32 t, delivered, timeouts, aborts, mrr;
    long made[KG_GOODS];                       // the books
    int pop, emp, nWt, nStore, nN, nW, bfsSlot, bfsHead, bfsTail;
    u16 nodeCell[KS_MAXN];                     // the road cells, sorted: a road cell's number is its place in this list
    u16 timer[KS_MAXB]; u8 inRes[KS_MAXB];     // per building: the production clock; carriers on their way to it
    u8 cmf[KS_MAXB];                           // (step 5g) per building: 0..2 kinds of comfort (a well, a tavern) in reach; made by ksCensus, never saved
    u8 mapB[KS_MAPS], mapReady[KS_MAPS], mapRef[KS_MAPS]; u32 mapUse[KS_MAPS];   // kinglog.h: a distance map is for one building
    KsWk wk[KS_MAXW];
    u32 opMatch, opWalk, opBfs, opSearch, opProd;   // work counters (the PC test turns them into cycles)
    KHold hd;                                       // (step 4h) what the giant carries: goods in it are still the realm's goods, so ksHeld counts them
    u8 ev;                                          // (step 5b) a castle stage was finished: the number of stages done (1 to 4), 0 for none; the screen says it and clears it. Never saved
    u8 dist[KS_MAPS][KS_MAXN]; u16 queue[KS_MAXN];   // KEEP THESE TWO LAST (step 4a): they are only a cache, rebuilt by ksBfs, and once ksPrepSave has run they are free for kingmem.h's save scratch
} KSim;

static inline void ksInc(u32*c){ if(*c!=0xFFFFFFFFu) (*c)++; }   // (step 5f) a run counter that stops at its ceiling instead of wrapping to 0
static inline u32 ksAge(const KSim*S,u32 stamp){ return S->t>=stamp?S->t-stamp:S->t+(KS_TMAX-stamp); }   // frames since a stamp of the frame clock, across its wrap
static inline u32 ksRnd(KSim*S){ u32 x=S->R->rng?S->R->rng:2463534242u; x^=x<<13; x^=x>>17; x^=x<<5; S->R->rng=x; return x; }
static inline int ksIn(const KBld*b){ return b->stock&255; }
static inline int ksOut(const KBld*b){ return b->stock>>8; }
static inline void ksSetIn(KBld*b,int v){ b->stock=(u16)((b->stock&0xFF00)|(v&255)); }
static inline void ksSetOut(KBld*b,int v){ b->stock=(u16)((b->stock&255)|((v&255)<<8)); }
#define ksRoadV(v) ((v)==KT_ROAD||(v)==KT_BRIDGE)   // a road cell: a road on land or a bridge over water (step 5d); the carriers walk both
#define KS_BRIDGE_N 1                                // a bridge cell costs this much cut stone from the stockpile [ASSUMED]
static inline int ksIsRoad(const KSim*S,int c){ return c>=0&&c<S->dim*S->dim&&ksRoadV(S->cells[c]); }
static inline int ksNode(KSim*S,int c){   // the place of a road cell in the list, or -1 (a binary search: about 10 steps)
    int lo=0, hi=S->nN-1; while(lo<=hi){ int m=(lo+hi)>>1; S->opSearch++; int v=S->nodeCell[m]; if(v==c) return m; if(v<c) lo=m+1; else hi=m-1; } return -1;
}
static inline int ksAdj(const KSim*S,const KBld*b,int*out){   // the cells beside a 2 x 2 building (up to 8)
    int d=S->dim, x=b->x, y=b->y, n=0, px[8]={x,x+1,x,x+1,x-1,x-1,x+2,x+2}, py[8]={y-1,y-1,y+2,y+2,y,y+1,y,y+1};
    for(int i=0;i<8;i++) if(px[i]>=0&&py[i]>=0&&px[i]<d&&py[i]<d) out[n++]=py[i]*d+px[i];
    return n;
}
// put goods in the stockpile; what does not fit under the ceiling is lost and the books know
static inline void ksPool(KSim*S,int g,int n){ if(n<=0) return; long v=(long)S->R->goods[g]+n; if(v>KG_GOODMAX){ S->made[g]-=v-KG_GOODMAX; v=KG_GOODMAX; } S->R->goods[g]=(u16)v; }
static inline void ksGift(KSim*S,int g,int n){ if(n<=0) return; S->made[g]+=n; ksPool(S,g,n); }   // goods the realm is given (the founding kit, a payout)
// (step 5g) COMFORT: a finished well or tavern makes the producers near it work faster. cmf[i] counts the KINDS (a well, a tavern) in reach, 0 to 2, so two wells are no better than one.
static inline void ksComfort(KSim*S){
    KRealm*R=S->R;
    for(int i=0;i<R->nB;i++){ const KBld*b=&S->B[i]; S->cmf[i]=0; if(!b->lvl) continue; const KsType*T=ksT(b->type); if(T->kind!=KK_GATHER&&T->kind!=KK_PROC) continue;
        int has=0; for(int j=0;j<R->nB;j++){ const KBld*c=&S->B[j]; if(!c->lvl||ksT(c->type)->kind!=KK_COMFORT) continue; int dx=c->x-b->x, dy=c->y-b->y; if(dx<0) dx=-dx; if(dy<0) dy=-dy; S->opProd++;
            if(dx<=KS_CR&&dy<=KS_CR) has|=1<<(c->type-KB_WELL); }
        S->cmf[i]=(u8)((has&1)+((has>>1)&1)); }
}
// the production period of building i, with its comfort: 1/8 shorter for each kind (never under 1)
static inline unsigned ksPeriod(const KSim*S,int i){ unsigned p=ksT(S->B[i].type)->period; unsigned q=p-p*S->cmf[i]/8u; return q<1u?1u:q; }
static inline void ksCensus(KSim*S){
    KRealm*R=S->R; int pop=KS_PARTY, emp=0, st=0;
    for(int i=0;i<R->nB;i++){ const KBld*b=&S->B[i]; const KsType*T=ksT(b->type); if(!b->lvl) continue;
        if(T->kind==KK_HOME) pop+=b->work; else if(T->kind==KK_STORE) st++; else if(T->kind==KK_GATHER||T->kind==KK_PROC) emp+=b->work; }
    if(pop>KS_POPCAP) pop=KS_POPCAP;
    S->pop=pop; S->emp=emp; S->nStore=st; int w=pop-emp; S->nWt=w<0?0:w>KS_MAXW?KS_MAXW:w;
    ksComfort(S);
}
// ---- (step 5b) THE CASTLE: one building, four stages (R->castle counts the stages done: 0 none, 1 foundation, 2 walls, 3 towers, 4 crowned). Each stage is two LOTS of goods carried to the
// site like any building's cost (b->work is the lot, 0 or 1; b->stock the goods delivered to it). A lot is at most KS_BUF (8) goods. The second lot done finishes the stage; the fourth stage finishes the
// castle (lvl 1) and crowns the monarch. [ASSUMED amounts: tune here, nothing of them is in a saved realm]
static const u8 KS_CL[KG_CASTLE][2][2]={
 { {G_PLANKS,6}, {G_CUT,6}    },   // 1 foundation
 { {G_CUT,8},    {G_PLANKS,4} },   // 2 walls
 { {G_CUT,6},    {G_POLISH,4} },   // 3 towers
 { {G_POLISH,6}, {G_CLOTH,6}  },   // 4 throne room
};
static inline void ksCostType(const KSim*S,int type,int lot,int*g,int*n){   // the good and the number a new site of this type asks for (a castle: the lot of the stage it is at)
    const KsType*T=ksT(type);
    if(T->kind==KK_CASTLE){ int s=S->R->castle; if(s>KG_CASTLE-1) s=KG_CASTLE-1; *g=KS_CL[s][lot&1][0]; *n=KS_CL[s][lot&1][1]; }
    else { *g=T->costG; *n=T->costN; }
}
static inline void ksCost(const KSim*S,const KBld*b,int*g,int*n){ ksCostType(S,b->type,b->work,g,n); }
static inline void ksCastleLot(KSim*S,KBld*b){   // a lot is paid: the first moves to the second, the second finishes the stage
    KRealm*R=S->R; if(b->work==0){ b->work=1; return; }
    b->work=0; if(R->castle<KG_CASTLE) R->castle++; S->ev=R->castle;
    if(R->castle>=KG_CASTLE){ b->lvl=1; ksCensus(S); }
}
static inline long ksCarried(const KSim*S,int g){ long n=0; for(int w=0;w<KS_MAXW;w++) if(S->wk[w].st&&S->wk[w].good==g) n++; return n; }
static inline long ksHeld(const KSim*S,int g){   // everything that exists of a good
    long v=S->R->goods[g];
    for(int i=0;i<S->R->nB;i++){ const KBld*b=&S->B[i]; const KsType*T=ksT(b->type);
        if(!b->lvl){ int cg=0,cn=0; ksCost(S,b,&cg,&cn); if(cg==g) v+=ksIn(b); continue; }
        if((T->kind==KK_GATHER||T->kind==KK_PROC)&&T->outG==g) v+=ksOut(b);
        if(T->kind==KK_PROC&&T->inG==g) v+=ksIn(b); }
    if(S->hd.k==KHD_GOODS&&S->hd.g==g) v+=S->hd.n;   // (the giant's load)
    return v+ksCarried(S,g);
}
static inline int ksLedger(const KSim*S){ for(int g=0;g<KG_GOODS;g++) if(ksHeld(S,g)!=S->made[g]) return 1+g; return 0; }
// (step 4h) before a save: what the giant carries goes home. Goods go back on the pile, a resident moves into a home with room (none free: he is simply not counted, new Nubbins move in daily),
// a worker is just a free Nubbin again. A lifted BUILDING cannot be sent home: returns 1 and keeps it (the save is refused until it is put down). Returns 0 otherwise.
static inline int ksHoldHome(KSim*S){
    KHold*h=&S->hd; if(h->k==KHD_BLD) return 1;
    if(h->k==KHD_GOODS) ksPool(S,h->g,h->n);
    else if(h->k==KHD_NUB&&h->bt){ for(int i=0;i<S->R->nB;i++){ KBld*b=&S->B[i]; const KsType*T=ksT(b->type); if(b->lvl&&T->kind==KK_HOME&&b->work<T->house){ b->work++; break; } } ksCensus(S); }
    __builtin_memset(h,0,sizeof *h); return 0;
}

// ---- road cells ----
static inline void ksNodeBuild(KSim*S){ int n=S->dim*S->dim, k=0; for(int c=0;c<n;c++) if(ksRoadV(S->cells[c])&&k<KS_MAXN) S->nodeCell[k++]=(u16)c; S->nN=k; }
static inline void ksMapsDirty(KSim*S){ for(int m=0;m<KS_MAPS;m++) S->mapReady[m]=0; S->bfsSlot=-1; }
static inline void ksSnap(KSim*S,KsWk*k){   // the road cell under a carrier is gone: step to a neighbouring road cell, else the nearest one, else off the map (0xFFFF)
    int d=S->dim, c=k->cell, nb[4]={c+1,c-1,c+d,c-d};
    for(int j=0;j<4;j++) if(c!=0xFFFF&&ksIsRoad(S,nb[j])){ k->cell=(u16)nb[j]; return; }
    int best=-1, bd=1<<20, cx=c%d, cy=c/d;
    for(int n=0;n<S->nN;n++){ int x=S->nodeCell[n]%d, y=S->nodeCell[n]/d; int m=(x>cx?x-cx:cx-x)+(y>cy?y-cy:cy-y); if(m<bd){ bd=m; best=n; } }
    k->cell=best>=0?S->nodeCell[best]:0xFFFF;
}
static inline void ksDrop(KSim*S,KsWk*k,int giveBack){   // a carrier stops: its good goes back in the stockpile (or is lost at the ceiling), its claims are let go
    if(giveBack) ksPool(S,k->good,1);
    if(k->md<KS_MAPS&&S->mapRef[k->md]) S->mapRef[k->md]--;
    if(k->dst<KS_MAXB&&S->inRes[k->dst]) S->inRes[k->dst]--;
    k->st=0;
}
static inline void ksRecall(KSim*S){ for(int w=0;w<KS_MAXW;w++) if(S->wk[w].st) ksDrop(S,&S->wk[w],1);   // every carrier home with its good, every map let go (before a save, a demolition)
    for(int m=0;m<KS_MAPS;m++){ S->mapB[m]=0xFF; S->mapReady[m]=0; S->mapRef[m]=0; S->mapUse[m]=0; }
    __builtin_memset(S->inRes,0,sizeof S->inRes); S->bfsSlot=-1; }
static inline int ksRoadSet(KSim*S,int x,int y,int on){
    int d=S->dim; if(x<2||y<2||x>=d-2||y>=d-2) return KSE_ARG;
    int c=y*d+x, v=S->cells[c];
    if(on){ if(ksRoadV(v)) return KSE_OK; if(v!=KT_GRASS&&v!=KT_MEADOW&&v!=KT_WATER) return KSE_SPOT; if(S->nN>=KS_MAXN) return KSE_CAP;
        int br=v==KT_WATER; if(br&&S->R->goods[G_CUT]<KS_BRIDGE_N) return KSE_POOR;   // (a bridge: its cut stone leaves the stockpile and the books together, so ksCheck stays balanced)
        if(br){ S->R->goods[G_CUT]=(u16)(S->R->goods[G_CUT]-KS_BRIDGE_N); S->made[G_CUT]-=KS_BRIDGE_N; }
        int i=S->nN; while(i>0&&S->nodeCell[i-1]>c){ S->nodeCell[i]=S->nodeCell[i-1]; i--; } S->nodeCell[i]=(u16)c; S->nN++; S->cells[c]=(u8)(br?KT_BRIDGE:KT_ROAD); }
    else { if(!ksRoadV(v)) return KSE_OK; int i=0; while(i<S->nN&&S->nodeCell[i]!=c) i++;
        for(;i+1<S->nN;i++){ S->nodeCell[i]=S->nodeCell[i+1]; }
        S->nN--;
        S->cells[c]=(u8)kgCell(S->R->seed,S->R->tier,x,y); }       // the land underneath comes back
    ksMapsDirty(S);
    for(int w=0;w<KS_MAXW;w++) if(S->wk[w].st&&!ksIsRoad(S,S->wk[w].cell)){ ksSnap(S,&S->wk[w]); if(S->wk[w].cell==0xFFFF) ksDrop(S,&S->wk[w],1); }
    return KSE_OK;
}

// ---- buildings ----
static inline int ksSpotOk(const KSim*S,int type,int x,int y){
    int d=S->dim; if(type>=KS_NTYPES||x<2||y<2||x+2>d-2||y+2>d-2) return 0;
    for(int j=0;j<2;j++) for(int k=0;k<2;k++){ int v=S->cells[(y+j)*d+x+k]; if(v!=KT_GRASS&&v!=KT_MEADOW) return 0; }   // (a cell with bit 7, a road, trees, rock and water all fail)
    return 1;
}
static inline int ksPlace(KSim*S,int type,int x,int y,int built){   // returns the building's number or a KSE_ error
    KRealm*R=S->R; if(R->nB>=KS_MAXB) return KSE_CAP; if(!ksSpotOk(S,type,x,y)) return KSE_SPOT;
    if(type==KB_CASTLE){ if(R->castle>=KG_CASTLE) return KSE_SPOT; for(int i=0;i<R->nB;i++) if(S->B[i].type==KB_CASTLE) return KSE_SPOT; built=0; }   // (one castle to a realm, and none after the crown)
    KBld*b=&S->B[R->nB]; __builtin_memset(b,0,sizeof *b); b->type=(u8)type; b->x=(u8)x; b->y=(u8)y; b->lvl=(u8)(built?1:0);
    for(int j=0;j<2;j++) for(int k=0;k<2;k++) S->cells[(y+j)*S->dim+x+k]|=KC_BLD;
    S->timer[R->nB]=0; S->inRes[R->nB]=0; R->nB++; ksCensus(S); return R->nB-1;
}
static inline int ksDemolish(KSim*S,int i){   // what the building holds goes to the stockpile; every carrier goes home first (building numbers shift)
    KRealm*R=S->R; if(i<0||i>=R->nB) return KSE_ARG;
    ksRecall(S); KBld*b=&S->B[i]; const KsType*T=ksT(b->type);
    if(!b->lvl){ int cg=0,cn=0; ksCost(S,b,&cg,&cn); ksPool(S,cg,ksIn(b)); }
    else { if(T->kind==KK_GATHER||T->kind==KK_PROC) ksPool(S,T->outG,ksOut(b)); if(T->kind==KK_PROC) ksPool(S,T->inG,ksIn(b)); }
    for(int j=0;j<2;j++) for(int k=0;k<2;k++) S->cells[(b->y+j)*S->dim+b->x+k]&=(u8)~KC_BLD;
    for(int k=i;k+1<R->nB;k++){ S->B[k]=S->B[k+1]; S->timer[k]=S->timer[k+1]; }
    R->nB--; ksCensus(S); return KSE_OK;
}

// ---- start (a new realm, or one just loaded with kgDecode): R, B and cells are the caller's; nothing is kept but pointers ----
static inline int ksInit(KSim*S,KRealm*R,KBld*B,u8*cells){
    int d=kgDim(R->tier); if(!d) return KSE_ARG; if(R->nB>KS_MAXB) return KSE_CAP;
    for(int i=0;i<R->nB;i++){ const KBld*b=&B[i]; if(b->type>=KS_NTYPES||b->x<2||b->y<2||b->x+2>d-2||b->y+2>d-2) return KSE_ARG; }
    { int nc=0; for(int i=0;i<R->nB;i++) nc+=B[i].type==KB_CASTLE; if(nc>1) return KSE_ARG; }   // (one castle only)
    __builtin_memset(S,0,sizeof *S); S->R=R; S->B=B; S->cells=cells; S->dim=d; S->bfsSlot=-1;
    for(int m=0;m<KS_MAPS;m++) S->mapB[m]=0xFF;
    for(int c=0;c<d*d;c++){ int v=cells[c]&7; cells[c]=(u8)v; if(v>=KT_N){ cells[c]=(u8)kgCell(R->seed,R->tier,c%d,c/d); } }   // a damaged cell value is made again from the land
    for(int i=0;i<R->nB;i++) for(int j=0;j<2;j++) for(int k=0;k<2;k++){ int c=(B[i].y+j)*d+B[i].x+k;   // a building on a road or on another building: refused
        if(cells[c]&KC_BLD) return KSE_ARG;
        if(cells[c]==KT_BRIDGE) return KSE_ARG;   // (no building stands on a bridge)
        if(cells[c]==KT_ROAD) cells[c]=(u8)kgCell(R->seed,R->tier,c%d,c/d);
        cells[c]|=KC_BLD; }
    { int n=0; for(int c=0;c<d*d;c++) n+=ksRoadV(cells[c]&7)&&!(cells[c]&KC_BLD); if(n>KS_MAXN) return KSE_CAP; }
    ksNodeBuild(S);
    for(int i=0;i<R->nB;i++){ KBld*b=&B[i]; const KsType*T=ksT(b->type);   // clamp what a damaged save could hold
        if(b->lvl>1){ b->lvl=1; }
        if(T->kind==KK_CASTLE){ if(R->castle>=KG_CASTLE){ b->lvl=1; b->work=0; b->stock=0; } else { b->lvl=0; if(b->work>1) b->work=0; } }   // (a crowned realm has a finished castle, any other a site at its stage)
        if(!b->lvl){ int cg=0,cn=0; ksCost(S,b,&cg,&cn); if(ksIn(b)>cn){ ksSetIn(b,cn); } }
        if(ksIn(b)>KS_BUF){ ksSetIn(b,KS_BUF); }
        if(ksOut(b)>KS_BUF){ ksSetOut(b,KS_BUF); }
        if(T->kind==KK_HOME&&b->work>T->house){ b->work=T->house; }
        if(T->kind!=KK_HOME&&b->work>1){ b->work=1; }
        if(T->kind==KK_COMFORT){ b->work=0; }   // (a well or a tavern has no worker)
        if(b->lvl&&(T->kind==KK_STORE||T->kind==KK_HOME)){ b->stock=0; }
        if(b->lvl&&T->kind==KK_GATHER){ ksSetIn(b,0); }
        if(!b->lvl&&ksOut(b)){ ksSetOut(b,0); }
        (void)0; }
    for(int g=0;g<KG_GOODS;g++){ if(R->goods[g]>KG_GOODMAX) R->goods[g]=KG_GOODMAX; }
    ksCensus(S);
    for(int g=0;g<KG_GOODS;g++) S->made[g]=ksHeld(S,g);     // the books start from what the realm holds now
    return KSE_OK;
}

// ---- one frame of the economy: the buildings work; once a day the Nubbins move in and find jobs and the forests grow ----
static inline void ksGather(KSim*S,int i){
    KBld*b=&S->B[i]; const KsType*T=ksT(b->type); int d=S->dim, mx=b->x+1, my=b->y+1;
    for(int y=my-KS_GR;y<=my+KS_GR+1;y++) for(int x=mx-KS_GR;x<=mx+KS_GR+1;x++){
        S->opProd++; if(x<0||y<0||x>=d||y>=d) continue; int c=y*d+x; if(S->cells[c]!=T->res) continue;
        if(T->res!=KT_MEADOW) S->cells[c]=KT_GRASS; ksSetOut(b,ksOut(b)+1); S->made[T->outG]++; return; }   // (a felled tree or a quarried rock is grass; it is an edit in the save)
}
static inline void ksDay(KSim*S){
    KRealm*R=S->R; if(R->day<0xFFFFFFFFu) R->day++;
    ksCensus(S); int pop=S->pop;
    for(int i=0;i<R->nB&&pop<KS_POPCAP;i++){ KBld*b=&S->B[i]; const KsType*T=ksT(b->type); if(b->lvl&&T->kind==KK_HOME&&b->work<T->house){ b->work++; pop++; } }
    int emp=0, lim=pop-1-pop/4; if(lim<0) lim=0;
    for(int i=0;i<R->nB;i++){ const KBld*b=&S->B[i]; const KsType*T=ksT(b->type); if(b->lvl&&(T->kind==KK_GATHER||T->kind==KK_PROC)) emp+=b->work; }
    for(int kind=KK_PROC;kind>=KK_GATHER;kind--) for(int i=R->nB-1;i>=0&&emp>lim;i--){ KBld*b=&S->B[i]; const KsType*T=ksT(b->type); if(b->lvl&&T->kind==kind&&b->work){ b->work=0; emp--; } }   // fewer Nubbins than jobs: processors lose theirs first, then the last gatherers
    for(int kind=KK_GATHER;kind<=KK_PROC;kind++) for(int i=0;i<R->nB&&emp<lim;i++){ KBld*b=&S->B[i]; const KsType*T=ksT(b->type); if(b->lvl&&T->kind==kind&&!b->work){ b->work=1; emp++; } }   // the gatherers are staffed first: without wood and stone nothing else can be built
    ksCensus(S);
    for(int i=0;i<R->nB;i++){ const KBld*b=&S->B[i]; const KsType*T=ksT(b->type); int d=S->dim;   // forests grow back round a working woodcutter (a quarry runs dry)
        if(!b->lvl||T->kind!=KK_GATHER||T->res!=KT_FOREST||!b->work) continue;
        for(int k=0;k<KS_REGROW;k++){ u32 r=ksRnd(S); int cx=b->x+1+(int)(r%13u)-KS_GR, cy=b->y+1+(int)((r>>8)%13u)-KS_GR; S->opProd++;
            if(cx<2||cy<2||cx>d-3||cy>d-3){ continue; }
            int c=cy*d+cx; if(S->cells[c]!=KT_GRASS){ continue; }
            S->cells[c]=KT_FOREST; } }   // (the woodcutter plants: a forest never dies out for want of a tree to seed it)
}
static inline void ksEcon(KSim*S){
    KRealm*R=S->R;
    for(int i=0;i<R->nB;i++){ KBld*b=&S->B[i]; const KsType*T=ksT(b->type); S->opProd++;
        if(!b->lvl){ int cg=0,cn=0; ksCost(S,b,&cg,&cn); if(ksIn(b)>=cn){ S->made[cg]-=cn; ksSetIn(b,0); if(T->kind==KK_CASTLE) ksCastleLot(S,b); else { b->lvl=1; ksCensus(S); } } continue; }
        if(T->kind==KK_GATHER&&b->work&&ksOut(b)<KS_BUF){ if(++S->timer[i]>=ksPeriod(S,i)){ S->timer[i]=0; ksGather(S,i); } }
        else if(T->kind==KK_PROC&&b->work&&ksIn(b)>=2&&ksOut(b)<KS_BUF){ if(++S->timer[i]>=ksPeriod(S,i)){ S->timer[i]=0; ksSetIn(b,ksIn(b)-2); S->made[T->inG]-=2; ksSetOut(b,ksOut(b)+1); S->made[T->outG]++; } } }
    if(S->t%KS_DAYT==0) ksDay(S);
}
