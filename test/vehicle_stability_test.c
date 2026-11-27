#include <assert.h>
#include <stdio.h>
#include "vehicle_stability.h"
int main(void){
    assert(bore_vehicle_half_width(0)<bore_vehicle_half_width(2));
    assert(bore_vehicle_half_length(3)>bore_vehicle_half_length(0));
    assert(bore_vehicle_cell_overlap(0,0,0,4096,210,440,200,400,128));
    assert(!bore_vehicle_cell_overlap(0,0,0,4096,210,440,400,0,128));
    assert(bore_vehicle_cell_overlap(0,0,4096,0,210,440,400,200,128));
    assert(!bore_vehicle_cell_overlap(0,0,4096,0,210,440,0,400,128));
    for(int y=-800;y<=800;y+=20)for(int x=-800;x<=800;x+=20){
        int a=bore_vehicle_cell_overlap(0,0,2896,2896,210,440,x,y,128);
        assert(a==bore_vehicle_cell_overlap(0,0,-2896,-2896,210,440,x,y,128));
        assert(a==bore_vehicle_cell_overlap(0,0,2896,2896,210,440,y,x,128));
    }
    for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++)for(int x=-288;x<576;x+=7)for(int y=-288;y<576;y+=11){
        int nx=x,ny=y;int ok=bore_vehicle_rebase(&nx,&ny,dx,dy,288);
        assert(nx==x-dx*288&&ny==y-dy*288);
        assert(ok==(nx>=-288&&nx<576&&ny>=-288&&ny<576));
    }
    assert(bore_vehicle_junction(0,0)==5&&bore_vehicle_junction(-1,-1)==1&&bore_vehicle_junction(1,1)==9);
    assert(!bore_vehicle_junction(-2,0)&&!bore_vehicle_junction(0,2));
    assert(bore_vehicle_claim_ok(1,0,0,0,0));
    assert(!bore_vehicle_claim_ok(1,0,0,1,0));
    assert(!bore_vehicle_claim_ok(1,0,1,0,0));
    assert(!bore_vehicle_claim_ok(1,0,0,0,1));
    assert(bore_vehicle_claim_ok(0,0,1,1,1));
    puts("vehicle stability: model footprints, rotational parity, non-wrapping rebases and junction ownership PASS");
}
