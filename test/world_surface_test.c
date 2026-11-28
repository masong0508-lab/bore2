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
    for(int part=0;part<4;part++)for(int kind=0;kind<4;kind++)for(int rd=0;rd<4;rd++){
        BInfo m={.terrain=BT_GRASS,.lot=LOT_SQUARE,.set=0,.road=(u8)(BR_MEGA|(part<<4)|(kind<<6)|rd)};
        assert(bore_surface_road_edges(&m)==(rd&~((part&1?2:0)|(part&2?1:0))));
    }
    for(int rd=0;rd<4;rd++){
        BInfo sea={.terrain=BT_SEA,.lot=LOT_WILD,.set=255,.road=(u8)rd};
        assert(bore_surface_road_edges(&sea)==rd);
    }
    for(int x=-72;x<=72;x++){
        int local;int slot=bore_surface_slot(x,&local);
        assert(local>=0&&local<BM_BLK&&slot*BM_BLK+local==x+BM_LOT0);
    }
    uint32_t state=1234567;
    for(int sample=0;sample<2048;sample++){
        uint32_t rows[18],before[18],seen[18]={0},r[324];
        for(int y=0;y<18;y++){state=state*1664525u+1013904223u;rows[y]=before[y]=sample==0?0x3ffffu:sample==1?0u:state&0x3ffffu;}
        int n=bore_water_rects(rows,r,324);assert(n>=0&&n<=324);
        if(sample==0)assert(n==1);
        if(sample==1)assert(n==0);
        for(int i=0;i<n;i++){
            int x=r[i]&31,y=(r[i]>>5)&31,X=(r[i]>>10)&31,Y=(r[i]>>15)&31;
            assert(x<X&&X<=18&&y<Y&&Y<=18);
            uint32_t mask=((1u<<(X-x))-1u)<<x;
            for(int j=y;j<Y;j++){assert(!(seen[j]&mask));seen[j]|=mask;}
        }
        for(int y=0;y<18;y++)assert(!rows[y]&&seen[y]==before[y]);
    }
    puts("Shared water: exact surfaces and 2048 lossless rectangle masks PASS");
    return 0;
}
