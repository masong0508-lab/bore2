#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../source/ds_cit_routine.h"
int main(void) {
    DsCitRoutineCache cache={0};
    for(int day=0;day<7;day++)for(int hour=0;hour<24;hour++)for(int stage=2;stage<=5;stage++) {
        int key=day*24+hour;
        for(unsigned id=1;id<=32;id++) {int want=ds_cit_activity(id,stage,hour,day);assert(ds_cit_activity_cached(&cache,(unsigned short)id,stage,key)==want);assert(ds_cit_activity_cached(&cache,(unsigned short)id,stage,key)==want);}
    }
    assert(ds_cit_activity_cached(&cache,1,2,10)==DS_CIT_SCHOOL);
    assert(ds_cit_activity_cached(&cache,1,2,5*24+10)==DS_CIT_OUT);
    assert(ds_cit_activity_cached(&cache,1,2,10)==DS_CIT_SCHOOL);
    assert(ds_cit_activity(1,2,10,0)==DS_CIT_SCHOOL);
    assert(ds_cit_activity(1,2,10,5)==DS_CIT_OUT);
    assert(ds_cit_activity(1,2,15,0)==DS_CIT_OUT);
    assert(ds_cit_activity(4,4,10,2)==DS_CIT_WORK);
    assert(ds_cit_activity(8,4,15,2)==DS_CIT_WORK);
    assert(ds_cit_activity(4,4,16,2)==DS_CIT_OUT);
    assert(ds_cit_activity(300,4,12,0)==DS_CIT_OUT);
    assert(ds_cit_activity(301,4,12,0)==DS_CIT_OUT);
    assert(ds_cit_activity(5,5,23,2)==DS_CIT_SLEEP);
    assert(strcmp(ds_cit_day_line(1,2,10,0),"School is keeping me busy.")==0);
    for (unsigned id=1; id<=256; id++) for (int stage=2; stage<=5; stage++) for (int day=0; day<7; day++) {
        int previous=-1, transitions=0, out=0;
        for (int hour=0; hour<24; hour++) {
            int a=ds_cit_activity(id,stage,hour,day);
            assert(a>=DS_CIT_SLEEP && a<=DS_CIT_OUT);
            if (previous>=0 && a!=previous) transitions++;
            if (a==DS_CIT_OUT) out++;
            previous=a;
        }
        assert(transitions<=7); assert(out>0);
    }
    for (int dx=-1; dx<=1; dx++) for (int dy=-1; dy<=1; dy++) {
        int n=0; for (int j=0; j<9; j++) n+=ds_cit_incoming(j,dx,dy);
        assert(n==9-(3-(dx!=0))*(3-(dy!=0)));
    }
    assert(ds_cit_in_patch(-320,639,320));
    assert(!ds_cit_in_patch(640,0,320));
    assert(!ds_cit_in_patch(0,-321,320));
    puts("DS citizen routines, weekly school/work, activity dialogue and incoming fringe passed");
    return 0;
}
