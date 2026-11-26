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
        assert(strcmp(weekday,weekend));found=1;
    }
    assert(found);
    assert(bore_android_window(-100,9999,8,a,289)==289);
    assert(!bore_android_describe(-1,0,0,(char*)b,sizeof b));
    puts("Android bridge: actual v8 map, stable seed and native weekday/weekend venues");
    return 0;
}
