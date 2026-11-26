#include "world_bridge.h"
#include "window_math.h"
#include "bigmap.h"
#define OUTINGS_IMPLEMENTATION
#include "outings.h"
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
    int g=outing_group(b.lot,0);
    if(g>=0){
        const Outing *a=outing_get(living_outing_index(g,0,weekday));
        const Outing *c=outing_get(living_outing_index(g,1,weekday));
        if(a&&c)return bsnprintf(out,cap,"%s | %s\n%s ($%u) / %s ($%u)",name,s?s->name:"Countryside",a->title,(unsigned)a->fee,c->title,(unsigned)c->fee);
    }
    return bsnprintf(out,cap,"%s | %s",name,s?s->name:"Countryside");
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
