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
    assert(bore_vehicle_box_overlap(0,0,0,4096,210,450,0,800,0,4096,210,450));
    assert(!bore_vehicle_box_overlap(0,0,0,4096,210,450,0,901,0,4096,210,450));
    assert(!bore_vehicle_box_overlap(0,0,0,4096,210,450,421,0,0,4096,210,450));
    assert(bore_vehicle_box_overlap(0,0,0,4096,210,450,600,0,4096,0,210,450));
    for(int y=-1000;y<=1000;y+=31)for(int x=-1000;x<=1000;x+=29){
        int a=bore_vehicle_box_overlap(0,0,0,4096,210,450,x,y,2896,2896,80,200);
        assert(a==bore_vehicle_box_overlap(x,y,2896,2896,80,200,0,0,0,4096,210,450));
    }
    BoreVehicleDraw q[3];int n=0;
    bore_vehicle_draw_insert(q,&n,3,9,500);
    bore_vehicle_draw_insert(q,&n,3,5,200);
    bore_vehicle_draw_insert(q,&n,3,7,300);
    bore_vehicle_draw_insert(q,&n,3,2,100);
    bore_vehicle_draw_insert(q,&n,3,1,200);
    bore_vehicle_draw_insert(q,&n,3,8,900);
    assert(n==3&&q[0].id==2&&q[1].id==1&&q[2].id==5);
    /* Rebased local coordinates have the same distance. */
    assert(bore_vehicle_draw_distance(5000,6000,5200,6100)==
           bore_vehicle_draw_distance(392,1392,592,1492));
    puts("vehicle stability: footprints, rebases, junctions and nearest actual render poses PASS");
}
