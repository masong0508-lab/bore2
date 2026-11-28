#include <assert.h>
#include <stdio.h>
#include "far_land_cache.h"
static int calls;
static int sample(int x,int y){++calls;return (x+y)&1?8:0;}
int main(void){
    uint8_t cell=255;int unknown=1;
    assert(bore_far_sample(&cell,&unknown,17,20,sample)==8&&unknown==0&&calls==1);
    cell|=0xf1;
    assert(bore_far_sample(&cell,&unknown,17,20,sample)==8&&unknown==0&&calls==1);
    cell=255;unknown=1;
    assert(bore_far_sample(&cell,&unknown,-4,2,sample)==0&&unknown==0&&calls==2);
    for(int a=0;a<=8;a++)for(int b=0;b<=8;b++)for(int c=0;c<=8;c++)for(int d=0;d<=8;d++){
        int ref=(a==b||a==c||a==d)?a:(b==c||b==d)?b:c==d?c:a;
        assert(bore_far_majority(a,b,c,d)==ref);
    }
    /* Worst-case visible lattice: every fine group stays eligible even
     * when earlier groups would have exhausted the former 360-run cap. */
    int n=0;
    for(int y=0;y<34;y++)for(int x=0;x<34;x++)n+=bore_far_group_needed(1);
    assert(n==1156&&!bore_far_group_needed(0)&&!bore_far_group_needed(2));
    puts("Far terrain: exact cache resolution, LOD ownership and material selection PASS");
    return 0;
}
