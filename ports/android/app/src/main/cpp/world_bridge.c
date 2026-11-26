#include "world_bridge.h"
#include "window_math.h"
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
