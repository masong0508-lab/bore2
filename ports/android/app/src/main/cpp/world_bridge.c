#include "world_bridge.h"
#include "window_math.h"
#include "mesh_footprint.h"
#include "venue_footprint.h"
#include "scene_surface.h"
#include "bigmap.h"
#define OUTINGS_IMPLEMENTATION
#include "outings.h"
#include "park_plan.h"
#include <stdarg.h>
extern void bm_hill_center(int bx,int by);

// Host/platform services required by the real world generator.
int ram8=1;
int dbg_miss[4];
void *xalloc(unsigned n){return malloc(n);}
void boot(const char *s){(void)s;}
void loading(const char *s,int p){(void)s;(void)p;}
// The map-only bootstrap has no world.c geometry-height cache to invalidate.
// bigmap.c still resets its own terrain cache and advances bm_edit_stamp.
void world_gh_reset(void){}
int bsprintf(char *o,const char *f,...){
    va_list ap;va_start(ap,f);int n=vsprintf(o,f,ap);va_end(ap);return n;
}
int bsnprintf(char *o,unsigned cap,const char *f,...){
    va_list ap;va_start(ap,f);int n=vsnprintf(o,cap,f,ap);va_end(ap);return n;
}

uint32_t bore_android_init(uint32_t seed){
    bm_ovr_clear();bm_rename_clear();bm_dials_default();
    bm_packed=0;bm_gen=8;bm_cap_name[0]=0;
    bm_new(seed);
    const Settle *capital=bm_set(bm_capital());
    unsigned x=capital?capital->x:BM_N/2,y=capital?capital->y:BM_N/2;
    return x|(y<<16);
}
int bore_android_window(int cx,int cy,int radius,uint32_t *out,int count){
    if(!out||!bore_window_valid(radius,count))return 0;
    cx=bore_window_center(cx,radius,BM_N);cy=bore_window_center(cy,radius,BM_N);
    int side=bore_window_side(radius);
    if(cx<0||cy<0)return 0;
    bm_hill_center(cx,cy);
    for(int iy=0;iy<side;iy++)for(int ix=0;ix<side;ix++){
        BInfo b;bm_block(cx-radius+ix,cy-radius+iy,&b);
        int storeys=bm_storeys(&b);
        out[iy*side+ix]=bore_window_pack(b.terrain,b.lot,b.road,(unsigned)storeys,b.set);
    }
    return side*side;
}
int bore_android_describe(int x,int y,int weekday,char *out,unsigned cap){
    if(!out||!cap||x<0||y<0||x>=BM_N||y>=BM_N)return 0;
    BInfo b;bm_block(x,y,&b);
    char name[64];
    bm_place_name(b.lot,b.h,name);
    if(b.lot==LOT_WILD&&b.terrain!=BT_SEA){
        BoreTrail trail;bm_trail_plan(x,y,&b,&trail);
        const char *label=bm_trail_label(trail.kind);int h=bm_hill(x,y);
        if(label)bsnprintf(name,sizeof name,"%s (%d tiles above sea)",label,h/TILE);
        else if(b.terrain==BT_ROCKY)bsnprintf(name,sizeof name,"Highland ridge (%d tiles above sea)",h/TILE);
        else if(h>=TILE*3)bsnprintf(name,sizeof name,"Rolling countryside (%d tiles above sea)",h/TILE);
    }
    const Settle *s=bm_set(b.set);
    char district[24],where[96];
    const char *d=bm_nb_name(x,y,district);
    if(d&&*d)bsnprintf(where,sizeof where,"%s / %s",s?s->name:"Countryside",d);
    else bsnprintf(where,sizeof where,"%s",s?s->name:"Countryside");
    if(b.lot==LOT_PARK||b.lot==LOT_SQUARE)bsnprintf(name,sizeof name,"%s",park_design_name(b.h,b.lot==LOT_SQUARE));
    int g=outing_group(b.lot,0);
    if(g>=0){
        const Outing *a=outing_get(living_outing_index(g,0,weekday));
        const Outing *c=outing_get(living_outing_index(g,1,weekday));
        if(a&&c)return bsnprintf(out,cap,"%s | %s\n%s ($%u) / %s ($%u)",name,where,a->title,(unsigned)a->fee,c->title,(unsigned)c->fee);
    }
    return bsnprintf(out,cap,"%s | %s",name,where);
}

int bore_android_can_visit(int x,int y){
    if(x<0||y<0||x>=BM_N||y>=BM_N)return 0;
    BInfo b;bm_block(x,y,&b);
    return b.terrain!=BT_SEA&&b.lot!=LOT_RIVER;
}
int bore_android_activity(int x,int y,int day,int slot,int minute,int funds,int rest,int *out){
    if(!out||!bore_android_can_visit(x,y))return 0;
    BInfo b;bm_block(x,y,&b);
    int g=outing_group(b.lot,0),id=living_outing_index(g,slot,day);
    const Outing *o=outing_get(id);if(!o)return 0;
    int check=outing_check(id,g,2,minute,funds,rest,0);
    int values[BORE_ANDROID_ACTIVITY_FIELDS]={check,id,o->fee,o->energy,o->fun,o->social,o->food,o->rest,o->skill,o->gain,(o->duration+11)/12};
    for(int i=0;i<BORE_ANDROID_ACTIVITY_FIELDS;i++)out[i]=values[i];
    return 1;
}
const char *bore_android_activity_text(int id,int field){
    const Outing *o=outing_get(id);
    if(field==2)return id>=OUT_OK&&id<=OUT_BUSY?outing_reason[id]:"Unavailable";
    return !o?"":field==1?o->done:o->title;
}

/* Conservative cardinal block connectivity. Water blocks are traversable only
 * along generated causeway/bridge axes, not across arbitrary water. This is
 * not yet the tile-level console walking and collision system. */
static unsigned route_mask(int x,int y){
    BInfo b;bm_block(x,y,&b);
    if(b.terrain==BT_SEA)return ((b.road&1)?5u:0u)|((b.road&2)?10u:0u);
    if(b.lot==LOT_RIVER)return (b.road&BR_MEGA)?((b.road&1)?5u:0u):((b.road&2)?10u:0u);
    return 15u;
}
int bore_android_route(int x,int y,int X,int Y,uint32_t *out,int cap){
    enum{SIDE=33,N=SIDE*SIDE};
    if(!out||cap<0||!bore_android_can_visit(x,y)||!bore_android_can_visit(X,Y))return -1;
    if(x==X&&y==Y)return 0;
    int ox=MAX(0,MIN(BM_N-SIDE,(x+X)/2-SIDE/2)),oy=MAX(0,MIN(BM_N-SIDE,(y+Y)/2-SIDE/2));
    if(x<ox||x>=ox+SIDE||X<ox||X>=ox+SIDE||y<oy||y>=oy+SIDE||Y<oy||Y>=oy+SIDE)return -1;
    unsigned char mask[N];
    uint16_t prev[N],queue[N];
    memset(mask,255,sizeof mask);memset(prev,255,sizeof prev);
    int start=(y-oy)*SIDE+x-ox,end=(Y-oy)*SIDE+X-ox,head=0,tail=0;
    prev[start]=(uint16_t)start;queue[tail++]=(uint16_t)start;
    static const int dx[4]={1,0,-1,0},dy[4]={0,1,0,-1};
    while(head<tail&&prev[end]==UINT16_MAX){
        int at=queue[head++],cx=at%SIDE,cy=at/SIDE;
        if(mask[at]==255)mask[at]=(unsigned char)route_mask(ox+cx,oy+cy);
        for(int d=0;d<4;d++){
            if(!(mask[at]&(1u<<d)))continue;
            int nx=cx+dx[d],ny=cy+dy[d];
            if(nx<0||nx>=SIDE||ny<0||ny>=SIDE)continue;
            int next=ny*SIDE+nx;
            if(prev[next]!=UINT16_MAX)continue;
            if(mask[next]==255)mask[next]=(unsigned char)route_mask(ox+nx,oy+ny);
            if(!(mask[next]&(1u<<((d+2)&3))))continue;
            prev[next]=(uint16_t)at;queue[tail++]=(uint16_t)next;
        }
    }
    if(prev[end]==UINT16_MAX)return -1;
    int len=0;for(int at=end;at!=start;at=prev[at])len++;
    if(len>cap)return -1;
    int at=end;
    for(int i=len-1;i>=0;i--){
        out[i]=(uint32_t)(ox+at%SIDE)|((uint32_t)(oy+at/SIDE)<<16);
        at=prev[at];
    }
    return len;
}

// An initial venue-based work loop; shares real lot types, not yet careers.c.
int bore_android_shift(int x,int y,int day,int minute,int energy,int food,int *out){
    if(!out||!bore_android_can_visit(x,y)||day<0||day>6||minute<0||minute>=1440)return 0;
    BInfo b;bm_block(x,y,&b);
    int from=480,to=1080,pay=70,skill=6;
    switch(b.lot){
    case LOT_OFFICE:case LOT_TOWER:from=540;pay=100;skill=1;if(day>=5)to=0;break;
    case LOT_DINER:case LOT_CAFE:case LOT_RESTAURANT:from=420;to=1260;pay=80;skill=4;break;
    case LOT_SHOP:case LOT_CONV:case LOT_GAS:to=1200;break;
    case LOT_LIBRARY:skill=1;break;
    case LOT_GYM:from=420;to=1200;skill=2;pay=85;break;
    case LOT_FARM:from=360;to=1080;skill=5;pay=75;break;
    case LOT_GALLERY:skill=3;break;
    default:return 0;
    }
    int duration=180,status=minute<from||minute+duration>to?SHIFT_CLOSED:energy<24?SHIFT_TIRED:food<15?SHIFT_HUNGRY:SHIFT_OK;
    int v[BORE_ANDROID_SHIFT_FIELDS]={status,duration,pay,24,15,skill,40};
    for(int i=0;i<BORE_ANDROID_SHIFT_FIELDS;i++)out[i]=v[i];
    return 1;
}

/* Exact residential footprint from the same native plan used by scene_mesh.
 * Ground-floor walkable area only; Java furnishes and partitions this shape. */
static int interior_footprint(int x,int y,int *out,int count,int commercial){
    if(!out||count!=BORE_ANDROID_INTERIOR_FIELDS||!bore_android_can_visit(x,y))return 0;
    BInfo b;bm_block(x,y,&b);
    if(commercial?!bore_venue_supported(b.lot):(b.lot<LOT_HOUSE||b.lot>LOT_APART))return 0;
    u8 f[5];LotPlan p;memset(&p,0,sizeof p);
    if(!bm_lot_foot(x,y,&b,f,&p))return 0;
    BoreFoot foot={f[0],f[1],f[2],f[3],0,0,0,0};
    if(f[4])foot=(BoreFoot){p.x0,p.y0,p.x1,p.y1,p.wx0,p.wy0,p.wx1,p.wy1};
    int dx=-1,dy=-1,front=p.face==LF_NORTH?-1:1;
    if(commercial&&!bore_venue_entry(&foot,p.face,&dx,&dy))return 0;
    if(!commercial&&f[4]&&p.dx<BM_BLK&&p.dy<BM_BLK&&bore_foot_has(&foot,p.dx,p.dy)&&
       (!bore_foot_has(&foot,p.dx,p.dy-1)||!bore_foot_has(&foot,p.dx,p.dy+1))){dx=p.dx;dy=p.dy;}
    if(dx<0){
        int best=100000,middle=(f[0]+f[2])/2,edge=front<0?f[1]:f[3];
        for(int iy=0;iy<BM_BLK;iy++)for(int ix=0;ix<BM_BLK;ix++)
            if(bore_foot_has(&foot,ix,iy)&&!bore_foot_has(&foot,ix,iy+front)){
                int score=4*abs(iy-edge)+abs(ix-middle);
                if(score<best){best=score;dx=ix;dy=iy;}
            }
    }
    if(dx<0)return 0;
    out[0]=b.lot;out[1]=dx;out[2]=dy;
    for(int iy=0;iy<BM_BLK;iy++)for(int ix=0;ix<BM_BLK;ix++)out[3+iy*BM_BLK+ix]=bore_foot_has(&foot,ix,iy);
    return 1;
}

int bore_android_interior(int x,int y,int *out,int count){return interior_footprint(x,y,out,count,0);}
int bore_android_place_interior(int x,int y,int *out,int count){return interior_footprint(x,y,out,count,1);}

/* 3x3 blocks, exact material/footprint collision with conservative tree
 * and fountain exclusion. This is outdoor movement, not console tile AI. */
int bore_android_walk_mask(int cx,int cy,int *out,int count){
    if(!out||count!=BORE_ANDROID_WALK_FIELDS||cx<1||cy<1||cx>=BM_N-1||cy>=BM_N-1)return 0;
    for(int by=0;by<3;by++)for(int bx=0;bx<3;bx++){
        int wx=cx+bx-1,wy=cy+by-1;
        BInfo b;bm_block(wx,wy,&b);
        BoreTrail trail;bm_trail_plan(wx,wy,&b,&trail);
        LotPlan p={0},fp={0};u8 f[5];
        int garden=(b.lot==LOT_PARK||b.lot==LOT_SQUARE)&&b.terrain!=BT_SEA;
        if(b.terrain!=BT_SEA&&(garden||b.lot==LOT_POOL||b.lot==LOT_FARM||(b.lot>=LOT_HOUSE&&b.lot<=LOT_TWO)))
            bm_lot_plan(wx,wy,&b,&p);
        int north=garden&&bm_gen>=7&&!(b.road&BR_MEGA)&&p.face==LF_NORTH;
        int built=b.terrain!=BT_SEA&&b.lot!=LOT_RIVER&&bm_lot_foot(wx,wy,&b,f,&fp);
        BoreFoot foot={0};
        if(built){foot=(BoreFoot){f[0],f[1],f[2],f[3],0,0,0,0};if(f[4])foot=(BoreFoot){fp.x0,fp.y0,fp.x1,fp.y1,fp.wx0,fp.wy0,fp.wx1,fp.wy1};}
        for(int y=0;y<18;y++)for(int x=0;x<18;x++){
            int m=bore_lot_surface(&b,&p,x,y),open=m!=BS_WATER&&m!=BS_POOL&&m!=BS_CROP;
            if(built&&bore_foot_has(&foot,x,y))open=0;
            if(garden&&m==BS_LAND&&x>=BM_LOT0&&y>=BM_LOT0&&park_surface(b.h,b.lot==LOT_SQUARE,north,x-BM_LOT0,y-BM_LOT0)==PARK_WATER)open=0;
            if(garden){
                int tx,tz;
                for(int i=0;park_tree_at(b.h,b.lot==LOT_SQUARE,north,i,&tx,&tz);i++)
                    if(x==BM_LOT0+tx&&y==BM_LOT0+tz)open=0;
                if((b.lot==LOT_SQUARE||park_variant(b.h)==0)&&x>=10&&x<=11&&y>=10&&y<=11)open=0;
            }else if(b.terrain==BT_FOREST&&b.lot==LOT_WILD){
                int tx=7+(b.h&3u),tz=7+((b.h>>2)&3u);
                if(!bore_trail_cell(&trail,x,y)&&((x==tx&&y==tz)||(x==tx+4&&(y==tz||y==tz+4))))open=0;
            }
            out[(by*18+y)*54+bx*18+x]=open;
        }
    }
    return 1;
}

/* Tile centres on the same NW-SE diagonal as scene_mesh.c. Native
 * collision and the visual terrain therefore agree at lot boundaries. */
int bore_android_walk_heights(int cx,int cy,int *out,int count){
    if(!out||count!=BORE_ANDROID_WALK_FIELDS||cx<1||cy<1||cx>=BM_N-1||cy>=BM_N-1)return 0;
    for(int by=0;by<3;by++)for(int bx=0;bx<3;bx++){
        BoreLandPatch p;bm_land_patch(cx+bx-1,cy+by-1,&p);
        for(int y=0;y<18;y++)for(int x=0;x<18;x++){
            int a=bore_land_sample(&p,x,y),b=bore_land_sample(&p,x+1,y+1);
            out[(by*18+y)*54+bx*18+x]=(a+b+1)/2;
        }
    }
    return 1;
}

/* Same number of storeys as the Android exterior renderer. */
int bore_android_storeys(int x,int y){
    if(!bore_android_can_visit(x,y))return 0;
    BInfo b;LotPlan p;u8 f[5];bm_block(x,y,&b);
    if(!((b.lot>=LOT_HOUSE&&b.lot<=LOT_APART)||bore_venue_supported(b.lot)))return 0;
    if(!bm_lot_foot(x,y,&b,f,&p))return 0;
    return CLAMP(bm_storeys(&b),1,6);
}
