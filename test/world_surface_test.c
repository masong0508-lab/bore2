#include <assert.h>
#include <stdio.h>
#include "world_surface.h"
int main(void){
    for(int vert=0;vert<2;vert++)for(int bridge=0;bridge<2;bridge++){
        BInfo b={.terrain=BT_GRASS,.lot=LOT_RIVER,.set=0,.road=(vert?BR_MEGA:0)|(bridge?(vert?1:2):0)};
        int water=0;
        for(int y=0;y<18;y++)for(int x=0;x<18;x++){
            int d=vert?x:y,e=vert?y:x;
            int w=d>=7&&d<=13&&(!bridge||e>=6);
            assert((bore_surface(&b,x,y)==BS_WATER)==w);
            water+=w;
            if(bridge&&d>=7&&d<=13&&e<5)assert(bore_surface(&b,x,y)==BS_ROAD);
        }
        assert(water==7*(bridge?12:18));
    }
    BInfo b={.terrain=BT_SEA,.lot=LOT_WILD,.set=255,.road=3};
    for(int y=0;y<18;y++)for(int x=0;x<18;x++)
        assert(bore_surface(&b,x,y)==(x<5||y<5?BS_ROAD:BS_WATER));
    b.terrain=BT_GRASS;b.road=BR_MEGA|(3<<4)|(MK_PARK<<6);
    for(int y=0;y<18;y++)for(int x=0;x<18;x++)
        assert((bore_surface(&b,x,y)==BS_WATER)==(x<5&&y<5));
    for(int x=-72;x<=72;x++){
        int local;int slot=bore_surface_slot(x,&local);
        assert(local>=0&&local<BM_BLK&&slot*BM_BLK+local==x+BM_LOT0);
    }
    puts("Shared water, river bridges, mega ponds and DS shifted coordinates PASS");
    return 0;
}
