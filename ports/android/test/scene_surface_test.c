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
    b.terrain=BT_SEA;assert(bore_surface(&b,0,0)==BS_WATER);
    puts("Android terrain: streets, curb, mega plazas and river crossings passed");
}
