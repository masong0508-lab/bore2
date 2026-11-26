#include <assert.h>
#include <stdio.h>
#include "park_plan.h"
#include "neighborhood_life.h"
int main(void){
    static const int tree_count[]={4,11,8,4,8};
    for(unsigned h=0;h<256;h++){
        int v=park_variant(h),n=0,x,y,seen[13][13]={{0}};
        for(int i=0;park_tree_at(h,0,0,i,&x,&y);i++){
            assert(x>=0&&x<13&&y>=0&&y<13&&!seen[y][x]++);
            int a,b;assert(park_tree_at(h,0,1,i,&a,&b)&&a==x&&b==12-y);n++;
        }
        assert(n==tree_count[v]);
        for(int y=0;y<13;y++)for(int x=0;x<13;x++){
            int a=park_surface(h,0,0,x,y);
            assert(a==park_surface(h,0,1,x,12-y));
            assert(a==park_surface(h^1u,0,0,12-x,y));
            assert(park_surface(h,1,0,x,y)==PARK_PATH);
        }
        assert(park_surface(h,0,0,6,0)==PARK_PATH&&park_surface(h,0,0,6,12)==PARK_PATH);
    }
    assert(park_surface(4,0,0,3,3)==PARK_WATER);
    assert(park_surface(8,0,0,3,5)==PARK_PATH);
    assert(park_surface(12,0,0,2,5)==PARK_SOIL);
    assert(neighborhood_score(13,LOT_LIBRARY,2,0)>neighborhood_score(13,LOT_BAR,2,7));
    assert(neighborhood_score(3,LOT_GYM,2,0)>neighborhood_score(3,LOT_LIBRARY,2,7));
    assert(neighborhood_score(7,LOT_CAFE,2,0)>neighborhood_score(7,LOT_CAFE,6,0));
    assert(!neighborhood_open(LOT_LIBRARY,479)&&neighborhood_open(LOT_LIBRARY,480));
    assert(!neighborhood_open(LOT_BAR,900)&&neighborhood_open(LOT_BAR,960));
    puts("Shared landscape plans, orientation, recognizable venues and resident preferences PASS");
}
