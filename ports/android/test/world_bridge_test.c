#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bigmap.h"
#include "outings.h"
#include "../app/src/main/cpp/world_bridge.h"
#include "../app/src/main/cpp/window_math.h"

int main(void){
    uint32_t center=bore_android_init(1234u),a[17*17],b[17*17];
    int x=(int)(center&65535),y=(int)(center>>16);
    x=bore_window_center(x,8,BM_N);y=bore_window_center(y,8,BM_N);
    assert(bore_android_window(x,y,8,a,289)==289);
    assert(bore_android_window(x,y,8,b,288)==0);
    assert(!bore_android_window(x,y,17,b,289));
    for(int iy=0;iy<17;iy++)for(int ix=0;ix<17;ix++){
        BInfo q;bm_block(x-8+ix,y-8+iy,&q);
        unsigned p=bore_window_pack(q.terrain,q.lot,q.road,bm_storeys(&q),q.set);
        assert(a[iy*17+ix]==p);
    }
    assert(bore_android_init(1234u)==center);
    assert(bore_android_window(x,y,8,b,289)==289&&!memcmp(a,b,sizeof a));
    int found=0;
    for(int iy=y-16;iy<=y+16&&!found;iy++)for(int ix=x-16;ix<=x+16&&!found;ix++){
        if(ix<0||iy<0||ix>=BM_N||iy>=BM_N)continue;
        BInfo q;bm_block(ix,iy,&q);int g=outing_group(q.lot,0);
        if(g<0)continue;
        char weekday[256],weekend[256];
        assert(bore_android_describe(ix,iy,0,weekday,sizeof weekday)>0);
        assert(bore_android_describe(ix,iy,5,weekend,sizeof weekend)>0);
        assert(strstr(weekday,outing_get(g*2)->title));
        assert(strstr(weekend,outing_get(24+g*2)->title));
        assert(strcmp(weekday,weekend));
        int v[BORE_ANDROID_ACTIVITY_FIELDS];
        const Outing *o=outing_get(g*2);
        assert(bore_android_can_visit(ix,iy));
        assert(bore_android_activity(ix,iy,0,0,o->from*60,100,100,v));
        assert(v[0]==OUT_OK&&v[1]==g*2&&v[2]==o->fee&&v[3]==o->energy&&v[9]==o->gain&&v[10]>0);
        assert(!strcmp(bore_android_activity_text(v[1],0),o->title));
        assert(bore_android_activity(ix,iy,0,0,o->to*60,100,100,v)&&v[0]==OUT_CLOSED);
        assert(bore_android_activity(ix,iy,0,0,o->from*60,100,0,v)&&v[0]==OUT_TIRED);
        if(o->fee)assert(bore_android_activity(ix,iy,0,0,o->from*60,o->fee-1,100,v)&&v[0]==OUT_MONEY);
        assert(bore_android_activity(ix,iy,5,0,600,100,100,v)&&v[1]==24+g*2);
        assert(!bore_android_activity(ix,iy,0,2,600,100,100,v));found=1;
    }
    assert(found);
    uint32_t route[BORE_ANDROID_ROUTE_MAX];
    assert(bore_android_route(x,y,x,y,route,BORE_ANDROID_ROUTE_MAX)==0);
    int routes=0,shifts=0;
    for(int iy=y-6;iy<=y+6;iy++)for(int ix=x-6;ix<=x+6;ix++){
        if(!bore_android_can_visit(ix,iy))continue;
        int len=bore_android_route(x,y,ix,iy,route,BORE_ANDROID_ROUTE_MAX);
        if(len>0){
            int px=x,py=y;
            for(int k=0;k<len;k++){
                int nx=route[k]&65535,ny=route[k]>>16;
                int dx=nx-px,dy=ny-py;assert(abs(dx)+abs(dy)==1);
                BInfo q;bm_block(nx,ny,&q);
                if(q.terrain==BT_SEA)assert(dx?(q.road&1):(q.road&2));
                if(q.lot==LOT_RIVER)assert((q.road&BR_MEGA)?(dx&&(q.road&1)):(dy&&(q.road&2)));
                px=nx;py=ny;
            }
            assert(px==ix&&py==iy);routes++;
        }
        int v[BORE_ANDROID_SHIFT_FIELDS];
        if(bore_android_shift(ix,iy,0,600,100,100,v)){
            assert(v[0]==SHIFT_OK&&v[1]>0&&v[2]>0);
            assert(bore_android_shift(ix,iy,0,600,0,100,v)&&v[0]==SHIFT_TIRED);
            assert(bore_android_shift(ix,iy,0,600,100,0,v)&&v[0]==SHIFT_HUNGRY);
            assert(bore_android_shift(ix,iy,0,0,100,100,v)&&v[0]==SHIFT_CLOSED);shifts++;
        }
    }
    assert(routes>0&&shifts>0);
    assert(bore_android_route(x,y,-1,y,route,BORE_ANDROID_ROUTE_MAX)<0);
    int invalid[BORE_ANDROID_ACTIVITY_FIELDS];
    assert(!bore_android_can_visit(-1,0)&&!bore_android_activity(-1,0,0,0,600,100,100,invalid));
    assert(bore_android_window(-100,9999,8,a,289)==289);
    assert(!bore_android_describe(-1,0,0,(char*)b,sizeof b));
    puts("Android bridge: actual v8 map, stable seed and native weekday/weekend venues");
    return 0;
}
