#include <assert.h>
#include <stdio.h>
#include "scene_surface.h"
int main(void){
    BInfo b={.terrain=BT_GRASS,.lot=LOT_HOUSE,.set=0,.road=1};
    assert(bore_surface(&b,8,0)==BS_ROAD&&bore_surface(&b,8,4)==BS_ROAD);
    assert(bore_surface(&b,8,5)==BS_PAVING&&bore_surface(&b,8,6)==BS_LAND);
    b.road=4;
    assert(bore_surface(&b,2,2)==BS_ROAD&&bore_surface(&b,2,7)==BS_LAND);
    b.road=BR_MEGA|(1<<4)|(MK_PLAZA<<6)|3;
    assert(bore_surface(&b,2,8)==BS_PAVING&&bore_surface(&b,8,2)==BS_ROAD);
    b.lot=LOT_RIVER;b.road=BR_MEGA;
    assert(bore_surface(&b,8,0)==BS_WATER&&bore_surface(&b,6,8)==BS_PAVING);
    assert(bore_surface(&b,14,10)==BS_PAVING);
    b.road=BR_MEGA|1;
    assert(bore_surface(&b,8,0)==BS_ROAD&&bore_surface(&b,8,5)==BS_PAVING);
    assert(bore_surface(&b,8,6)==BS_WATER);
    b.lot=LOT_RIVER;b.road=2;
    assert(bore_surface(&b,0,8)==BS_ROAD&&bore_surface(&b,6,8)==BS_WATER);
    b.terrain=BT_SEA;assert(bore_surface(&b,0,0)==BS_ROAD);
    assert(bore_surface(&b,4,17)==BS_ROAD&&bore_surface(&b,5,17)==BS_WATER);
    b.road=1;assert(bore_surface(&b,17,4)==BS_ROAD&&bore_surface(&b,17,5)==BS_WATER);
    b.road=0;assert(bore_surface(&b,0,0)==BS_WATER);
    b.terrain=BT_GRASS;b.road=0;b.lot=LOT_POOL;b.h=0;
    LotPlan p={0};p.face=LF_SOUTH;
    int water=0,deck=0;
    for(int z=BM_LOT0;z<BM_BLK;z++)for(int x=BM_LOT0;x<BM_BLK;x++){
        int m=bore_lot_surface(&b,&p,x,z);water+=m==BS_POOL;deck+=m==BS_PATIO;
    }
    assert(water==24&&deck==169-24);
    p.face=LF_NORTH;assert(bore_lot_surface(&b,&p,8,14)==BS_POOL);
    b.lot=LOT_HOUSE;p.type=HT_RANCH;p.dx=11;p.dy=9;
    assert(bore_lot_surface(&b,&p,11,8)==BS_PATIO);
    assert(bore_lot_surface(&b,&p,11,9)==BS_LAND);
    puts("Android terrain: one owner for road, bridge, pool, path and terrain cells");
}
