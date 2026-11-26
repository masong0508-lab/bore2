#include <assert.h>
#include <stdio.h>
#define OUTINGS_IMPLEMENTATION
#include "../src/outings.h"
int main(void){
    assert(OUT_N==48);
    const int lots[]={LOT_CAFE,LOT_LIBRARY,LOT_GYM,LOT_POOL,LOT_CINEMA,LOT_FARM,LOT_PARK,LOT_ARCADE,LOT_GALLERY,LOT_HOTEL,LOT_SCHOOL,LOT_OBS};
    unsigned kinds=0;
    for(int g=0;g<SC_VENUE_COUNT;g++){
        assert(outing_group(lots[g],0)==g&&outing_group(lots[g],1)==-1);
        for(int k=0;k<4;k++){
            int id=g*2+(k%2)+(k>=2?24:0);const Outing *o=outing_get(id);kinds|=1u<<o->kind;
            assert(o->duration>=300&&o->duration<=720&&o->from<o->to&&o->to<=24);
            assert(!outing_is_open(o,o->from*60-1)&&outing_is_open(o,o->from*60));
            assert(outing_is_open(o,o->to*60-1)&&!outing_is_open(o,o->to*60));
            assert(outing_check(id,g,o->age,o->from*60,o->fee,o->energy+8,0)==OUT_OK);
            assert(outing_check(id,g,-1,o->from*60,100,100,0)==OUT_AGE);
            assert(outing_check(id,g,o->age,o->to*60,100,100,0)==OUT_CLOSED);
            if(o->fee)assert(outing_check(id,g,o->age,o->from*60,o->fee-1,100,0)==OUT_MONEY);
            assert(outing_check(id,g,o->age,o->from*60,100,o->energy+7,0)==OUT_TIRED);
            assert(outing_check(id,g,o->age,o->from*60,100,100,1)==OUT_BUSY);
            assert(outing_check(id,(g+1)%SC_VENUE_COUNT,o->age,o->from*60,100,100,0)==OUT_NO_VENUE);
            assert(!outing_complete(id,g,1)&&!outing_complete(id,-1,0)&&outing_complete(id,g,0));
        }
    }
    assert(kinds==7);assert(!outing_get(-1)&&!outing_get(OUT_N));
    puts("All 48 local outings: venue, age, schedule, fee, energy and completion rules passed");return 0;
}
