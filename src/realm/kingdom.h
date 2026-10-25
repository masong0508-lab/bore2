// kingdom.h - EP6 THE LAND BOREFORE TIME: the REALM (a kingdom) as data, and how it is packed for a save slot.
// Roadmap Phase 2, step a. Nothing in the game calls this yet: it is pure data and bytes (no screen, no save memory, no EWRAM), so it can be
// tested on a PC (tools/test_kingdom.c) before it goes near the cart. Every function is static inline, so an unused one costs no ROM.
//
// A REALM  is a square map of cells, 1 byte each, the building list, the stockpile and a few numbers. Only what the PLAYER changed is saved:
//   the TERRAIN is made again from the SEED (kgCell: the same seed always gives the same land, on any machine), so a save holds
//   - the header (seed, size, giant, castle stage, day, the random state, the goods),
//   - the BUILDINGS (7 bytes each; the workers inside them are just a count: the Nubbins are not saved one by one),
//   - the EDITS: runs of cells that differ from the made terrain (a road, a felled forest, a quarried rock), packed as (gap, length, value).
// The map the game plays on has more in it than the save (a cell's bit 7 means "a building stands here": kgStamp puts it back after a load).
// The workers' hauling jobs are not saved either: they are made again from the buildings' stocks when a realm loads (roadmap 5.2).
//
// SAVE COMPATIBILITY FOR EVER: kgCell must give the SAME land for the same seed and genVer for as long as the game lives. A change to it is a NEW
// genVer (kgCell keeps the old one beside it); tools/test_kingdom.c holds a fingerprint of the made land and fails if it moves.
//
// FORMAT (version 1, one slot chunk 'K'; all numbers little-endian; the slot layer adds the chunk header and its own checksum on top):
//    0 fmt(1)  1 genVer(1)  2 tier(1: 0 hamlet 48, 1 realm 96, 2 kingdom 128)  3 giant(1: 0..3 the four, 4 your own Bore)  4 castle stage(1: 0..4)
//    5 flags(1, 0 now)  6..9 seed  10..13 day  14..17 random state  18..33 goods (8 x u16)  34..35 building count  then the buildings
//    (type, x, y, level, workers, stock u16)  then the edit run count (u16) and the runs (gap varint, length-1 varint, value)  then a Fletcher-16 of all of it.
// FORMAT 2 (step 4j, the giant's position): the same, with two bytes after the building count: gx, gy (36, 37), the cell the giant stood on when the realm was saved,
//    or 255 (KG_NOPOS) for none. Format 1 saves (made before 4j) still load, with no position; the next save of them is format 2. A format 1 reader cannot read a format 2 save.
// Needs before it: u8, u16, u32 (main.c, or the test's own).

#define KG_FMT      2          // (format 1, before step 4j, is still read: KG_FMT1)
#define KG_FMT1     1
#define KG_NOPOS    255        // gx, gy: no saved position for the giant
#define KG_GENVER   1
#define KG_TIERS    3
#define KG_MAXB     250        // buildings a realm may hold
#define KG_GOODS    8          // wood, stone, wool, crystal, then planks, cut stone, cloth, polished crystal
#define KG_GIANTS   5          // the four giants and your own Bore
#define KG_CASTLE   4          // castle stages (0 none yet, 4 = crowned)
#define KG_BTYPES   16         // building types (a table of them comes with the simulation)
#define KG_GOODMAX  60000      // a stockpile's ceiling: counters saturate here (endless play, roadmap 4.3)
#define KG_HDR      36
#define KG_BLD      7
enum { KT_GRASS, KT_FOREST, KT_ROCK, KT_WATER, KT_CRYSTAL, KT_MEADOW, KT_ROAD, KT_BRIDGE, KT_N };   // terrain values (bits 0..2 of a cell). The made land has 0..5; a road and a bridge (a road over water, step 5d) are only ever edits
// the made land's numbers (part of genVer 1: NEVER change them, make genVer 2 instead): island bias and fall-off, the water line, the rock line, the forest and meadow moisture lines
#define KG_ISL_BIAS 70
#define KG_ISL_FALL 130
#define KG_WATER    85
#define KG_ROCK     185
#define KG_FOREST   150
#define KG_MEADOW   90
#define KC_BLD 0x80            // a cell's bit 7: a building stands on it (not saved; kgStamp)
enum { KGE_OK=0, KGE_BIG=-1, KGE_ARG=-2, KGE_FMT=-3, KGE_SUM=-4, KGE_RANGE=-5, KGE_SHORT=-6 };

typedef struct { u8 tier, giant, castle, genVer, flags; u32 seed, day, rng; u16 goods[KG_GOODS]; u16 nB; u8 gx, gy; } KRealm;   // gx, gy: the giant's cell (step 4j), KG_NOPOS for none
typedef struct { u8 type, x, y, lvl, work; u16 stock; } KBld;

static inline int kgDim(int tier){ return tier==0?48:tier==1?96:tier==2?128:0; }
static inline int kgCells(int tier){ int d=kgDim(tier); return d*d; }

// ---- the made land ----
static inline u32 kgHash(u32 s,int x,int y){ u32 h=s^((u32)x*0x9E3779B1u)^((u32)y*0x85EBCA77u); h^=h>>15; h*=0x2C1B3C6Du; h^=h>>12; h*=0x297A2D39u; h^=h>>15; return h; }
static inline int kgNoise(u32 s,int x,int y,int sh){   // smooth value noise, 0..255, cells of 1<<sh; integer maths only (the same on the cart and a PC)
    int gx=x>>sh, gy=y>>sh, m=(1<<sh)-1, tx=((x&m)<<8)>>sh, ty=((y&m)<<8)>>sh;
    int a=(int)(kgHash(s,gx,gy)>>24), b=(int)(kgHash(s,gx+1,gy)>>24), c=(int)(kgHash(s,gx,gy+1)>>24), d=(int)(kgHash(s,gx+1,gy+1)>>24);
    tx=(tx*tx*(768-2*tx))>>16; ty=(ty*ty*(768-2*ty))>>16;   // smoothstep, 0..256
    int top=(a*(256-tx)+b*tx)>>8, bot=(c*(256-tx)+d*tx)>>8; return (top*(256-ty)+bot*ty)>>8;
}
// the terrain of one cell (genVer 1): an island, water round the edge, rock and crystal on the high ground, forest where it is wet, meadow where it is dry
static inline int kgCell(u32 seed,int tier,int x,int y){
    int dim=kgDim(tier), c=dim/2, dx=x-c, dy=y-c;
    if(dx>=-3&&dx<=3&&dy>=-3&&dy<=3) return KT_GRASS;           // the starting field: always clear
    int n=(kgNoise(seed,x,y,4)*4+kgNoise(seed^0x5BD1E995u,x,y,3)*2+kgNoise(seed^0x1B873593u,x,y,2))/7;
    int h=n+KG_ISL_BIAS-(dx*dx+dy*dy)*KG_ISL_FALL/(c*c);
    if(h<KG_WATER) return KT_WATER;
    u32 r=kgHash(seed^0xA511E9B3u,x,y);
    if(h>KG_ROCK) return (r%17)==0?KT_CRYSTAL:KT_ROCK;
    int m=kgNoise(seed^0x9E3779B9u,x,y,4);
    if(m>KG_FOREST) return KT_FOREST;
    if(m<KG_MEADOW&&(r&3)!=0) return KT_MEADOW;
    return KT_GRASS;
}
static inline void kgGen(u8*cells,u32 seed,int tier){ int d=kgDim(tier); for(int y=0;y<d;y++) for(int x=0;x<d;x++) cells[y*d+x]=(u8)kgCell(seed,tier,x,y); }

// the footprint of a building type, in cells (a table of real ones comes with the buildings: 2 x 2 until then)
static inline int kgFootW(int type){ (void)type; return 2; }
static inline int kgFootH(int type){ (void)type; return 2; }
static inline void kgStamp(u8*cells,int tier,const KBld*B,int n){   // put the buildings' bit 7 back on the map
    int d=kgDim(tier);
    for(int i=0;i<n;i++) for(int j=0;j<kgFootH(B[i].type);j++) for(int k=0;k<kgFootW(B[i].type);k++){ int x=B[i].x+k, y=B[i].y+j; if(x<d&&y<d) cells[y*d+x]|=KC_BLD; }
}

// ---- bytes ----
typedef struct { u8*p; int pos, cap, over; u32 s1, s2; } KBuf;
static inline void kbInit(KBuf*b,u8*p,int cap){ b->p=p; b->pos=0; b->cap=cap; b->over=0; b->s1=b->s2=0; }
static inline void kbPut(KBuf*b,int v){ if(b->pos>=b->cap){ b->over=1; return; } b->p[b->pos++]=(u8)v; }
static inline void kbPut16(KBuf*b,int v){ kbPut(b,v&255); kbPut(b,(v>>8)&255); }
static inline void kbPut32(KBuf*b,u32 v){ kbPut16(b,(int)(v&65535)); kbPut16(b,(int)(v>>16)); }
static inline void kbVar(KBuf*b,u32 v){ while(v>=128){ kbPut(b,(int)(v&127)|128); v>>=7; } kbPut(b,(int)v); }
static inline int kgSum16(const u8*p,int n){ u32 s1=0,s2=0; for(int i=0;i<n;i++){ s1+=p[i]; if(s1>=255) s1-=255; s2+=s1; if(s2>=255) s2-=255; } return (int)(s1|(s2<<8)); }
typedef struct { const u8*p; int pos, len, bad; } KRd;
static inline int krGet(KRd*r){ if(r->pos>=r->len){ r->bad=1; return 0; } return r->p[r->pos++]; }
static inline int krGet16(KRd*r){ int a=krGet(r); int b=krGet(r); return a|(b<<8); }
static inline u32 krGet32(KRd*r){ u32 a=(u32)krGet16(r); u32 b=(u32)krGet16(r); return a|(b<<16); }
static inline u32 krVar(KRd*r){ u32 v=0; for(int s=0;s<35;s+=7){ int c=krGet(r); if(r->bad) return 0; v|=(u32)(c&127)<<s; if(!(c&128)) return v; } r->bad=1; return 0; }   // (at most 5 bytes)

// ---- encode: the realm as bytes. cells is the map as it stands (bit 7 is ignored); 0 = the made land with no edits. Returns the length, or KGE_BIG / KGE_ARG. ----
static inline int kgEncode(u8*out,int cap,const KRealm*R,const KBld*B,const u8*cells){
    if(R->tier>=KG_TIERS||R->giant>=KG_GIANTS||R->castle>KG_CASTLE||R->nB>KG_MAXB||R->genVer!=KG_GENVER) return KGE_ARG;
    if((R->gx!=KG_NOPOS&&R->gx>=kgDim(R->tier))||(R->gy!=KG_NOPOS&&R->gy>=kgDim(R->tier))) return KGE_ARG;
    KBuf w; kbInit(&w,out,cap);
    kbPut(&w,KG_FMT); kbPut(&w,R->genVer); kbPut(&w,R->tier); kbPut(&w,R->giant); kbPut(&w,R->castle); kbPut(&w,R->flags);
    kbPut32(&w,R->seed); kbPut32(&w,R->day); kbPut32(&w,R->rng);
    for(int i=0;i<KG_GOODS;i++) kbPut16(&w,R->goods[i]>KG_GOODMAX?KG_GOODMAX:R->goods[i]);
    kbPut16(&w,R->nB); kbPut(&w,R->gx); kbPut(&w,R->gy);
    for(int i=0;i<R->nB;i++){ const KBld*b=&B[i]; if(b->type>=KG_BTYPES) return KGE_ARG; kbPut(&w,b->type); kbPut(&w,b->x); kbPut(&w,b->y); kbPut(&w,b->lvl); kbPut(&w,b->work); kbPut16(&w,b->stock); }
    int d=kgDim(R->tier), n=d*d, runsAt=w.pos; kbPut16(&w,0);
    int runs=0, last=0, i=0;                                    // last: the cell after the previous run
    while(cells&&i<n){                                          // (cells 0: a land nobody has changed yet, no edits: how a new realm is saved)
        int v=cells[i]&7, base=kgCell(R->seed,R->tier,i%d,i/d);
        if(v==base){ i++; continue; }
        int j=i+1; while(j<n&&(cells[j]&7)==v&&kgCell(R->seed,R->tier,j%d,j/d)!=v) j++;   // a run: the same value, every cell of it differs from the land
        kbVar(&w,(u32)(i-last)); kbVar(&w,(u32)(j-i-1)); kbPut(&w,v); runs++; last=j; i=j;
        if(w.over) return KGE_BIG;
    }
    if(w.over) return KGE_BIG;
    out[runsAt]=(u8)(runs&255); out[runsAt+1]=(u8)(runs>>8);
    int s=kgSum16(out,w.pos); kbPut16(&w,s);
    return w.over?KGE_BIG:w.pos;
}

// ---- decode: bytes to the realm. apply=0 only checks (nothing is written); apply=1 fills R, B (room for KG_MAXB) and cells (made, then the edits).
// Returns 0 or a KGE_ error. Check first with apply=0, then load with apply=1: a bad save then never half-replaces a realm in play. ----
static inline int kgDecode(const u8*p,int len,KRealm*R,KBld*B,u8*cells,int apply){
    if(len<KG_HDR+2) return KGE_SHORT;
    if(kgSum16(p,len-2)!=(p[len-2]|(p[len-1]<<8))) return KGE_SUM;
    KRd r; r.p=p; r.pos=0; r.len=len-2; r.bad=0;
    int fmt=krGet(&r), gv=krGet(&r), tier=krGet(&r), giant=krGet(&r), castle=krGet(&r), flags=krGet(&r);
    if((fmt!=KG_FMT&&fmt!=KG_FMT1)||gv!=KG_GENVER) return KGE_FMT;
    if(tier>=KG_TIERS||giant>=KG_GIANTS||castle>KG_CASTLE) return KGE_RANGE;
    KRealm t; t.tier=(u8)tier; t.giant=(u8)giant; t.castle=(u8)castle; t.genVer=(u8)gv; t.flags=(u8)flags;
    t.seed=krGet32(&r); t.day=krGet32(&r); t.rng=krGet32(&r);
    for(int i=0;i<KG_GOODS;i++){ t.goods[i]=(u16)krGet16(&r); if(t.goods[i]>KG_GOODMAX) return KGE_RANGE; }
    int nB=krGet16(&r);
    if(r.bad) return KGE_SHORT;
    if(nB>KG_MAXB) return KGE_RANGE;
    t.nB=(u16)nB;
    int d=kgDim(tier), n=d*d;
    t.gx=KG_NOPOS; t.gy=KG_NOPOS;                                // (format 1 has no position)
    if(fmt==KG_FMT){ int gx=krGet(&r), gy=krGet(&r); if(r.bad) return KGE_SHORT; if((gx!=KG_NOPOS&&gx>=d)||(gy!=KG_NOPOS&&gy>=d)) return KGE_RANGE; t.gx=(u8)gx; t.gy=(u8)gy; }
    for(int i=0;i<nB;i++){
        KBld b; b.type=(u8)krGet(&r); b.x=(u8)krGet(&r); b.y=(u8)krGet(&r); b.lvl=(u8)krGet(&r); b.work=(u8)krGet(&r); b.stock=(u16)krGet16(&r);
        if(r.bad) return KGE_SHORT;
        if(b.type>=KG_BTYPES||b.x>=d||b.y>=d) return KGE_RANGE;
        if(apply) B[i]=b;
    }
    int runs=krGet16(&r);
    if(r.bad) return KGE_SHORT;
    if(apply){ kgGen(cells,t.seed,tier); }
    int at=0;
    for(int k=0;k<runs;k++){
        u32 gap=krVar(&r), l=krVar(&r)+1; int v=krGet(&r);
        if(r.bad) return KGE_SHORT;
        if(gap>(u32)n||l>(u32)n||at+(int)gap+(int)l>n||v>=KT_N) return KGE_RANGE;
        at+=(int)gap;
        if(apply) for(int c=0;c<(int)l;c++) cells[at+c]=(u8)v;
        at+=(int)l;
    }
    if(r.pos!=r.len) return KGE_RANGE;                           // nothing may be left over
    if(apply){ *R=t; kgStamp(cells,tier,B,nB); }
    return KGE_OK;
}
