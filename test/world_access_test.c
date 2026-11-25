#include <assert.h>
#include <stdio.h>
#define OUTINGS_IMPLEMENTATION
#include "../src/world_access.h"
int main(void){
    for(int lot=0;lot<LOT_N;lot++){int g=outing_group(lot,0);assert(world_outing_anchor(lot,'a')==(g>=0));assert(!world_outing_anchor(lot,'8'));if(g>=0)assert(world_marker_needed(lot,BT_GRASS));}
    assert(!world_marker_needed(LOT_HOUSE,BT_GRASS));
    assert(world_outing_anchor(LOT_CAFE,'c')&&world_outing_anchor(LOT_LIBRARY,'b'));
    assert(world_outing_anchor(LOT_GYM,']')&&world_outing_anchor(LOT_POOL,'~'));
    assert(world_outing_anchor(LOT_CINEMA,';')&&world_outing_anchor(LOT_FARM,0xDE));
    assert(world_outing_anchor(LOT_PARK,'N')&&world_outing_anchor(LOT_ARCADE,'!'));
    assert(world_outing_anchor(LOT_GALLERY,')')&&world_outing_anchor(LOT_HOTEL,'I'));
    assert(world_outing_anchor(LOT_SCHOOL,'d')&&world_outing_anchor(LOT_OBS,0xD6));
    assert(!world_outing_anchor(LOT_HOUSE,'b')&&!world_outing_anchor(LOT_GAS,'y')&&!world_outing_anchor(LOT_POOL,'b'));
    assert(world_trip_anchor(LOT_HOTEL,'y')&&world_trip_anchor(LOT_INN,'a'));
    assert(world_trip_anchor(LOT_SQUARE,'+')&&world_trip_anchor(-1,'@'));
    assert(!world_trip_anchor(LOT_HOUSE,'I')&&!world_trip_anchor(LOT_SHOP,'y'));
    assert(world_heist_anchor(LOT_GAS,BT_GRASS,'a')==SITE_ROAD);
    assert(world_heist_anchor(LOT_PARK,BT_BEACH,'a')==SITE_WATER);
    assert(world_heist_anchor(LOT_SPACE,BT_GRASS,'a')==SITE_AIR);
    assert(!world_heist_anchor(LOT_HOUSE,BT_BEACH,'a'));
    for(int s=1;s<=3;s++){int n=world_heist_count(s),k=world_heist_first(s);for(int i=0;i<n;i++){assert(world_heist_valid(s,k));k=world_heist_cycle(s,k,1);}assert(k==world_heist_first(s));}
    assert(!world_heist_valid(SITE_ROAD,5)&&!world_heist_valid(SITE_WATER,9));
    assert(world_trip_fare(1,0)==35&&world_trip_fare(0,1)==20&&world_trip_fare(0,0)==0);
    puts("World access: venue anchors, transport fares and site-bound game selections passed");return 0;
}
