#include <assert.h>
#include <stdio.h>
#include "mesh_footprint.h"
int main(void){
    BoreFoot f={6,7,12,13,12,9,15,13};
    assert(bore_foot_has(&f,6,7));
    assert(bore_foot_has(&f,15,13));
    assert(!bore_foot_has(&f,14,7));
    assert(!bore_foot_has(&f,16,13));
    assert(bore_foot_edge(&f,12,10,1,0)==0);
    assert(bore_foot_edge(&f,14,9,0,-1)==1);
    assert(bore_foot_edge(&f,15,13,1,0)==1);
    assert(!bore_foot_edge(&f,14,7,0,1));
    BoreFoot rect={5,5,10,10,0,0,0,0};
    assert(bore_foot_has(&rect,10,10)&&!bore_foot_has(&rect,11,10));
    assert(!bore_foot_has(&rect,0,0));
    assert(bore_roof_height(&rect,8,5,0)==0);
    assert(bore_roof_height(&rect,8,8,0)==3);
    assert(bore_roof_height(&rect,8,5,1)==3);
    assert(bore_roof_height(&rect,5,8,2)==0);
    assert(bore_roof_height(&f,14,9,2)==0);
    puts("Footprint union, shared edges and roof corner heights passed");
}
