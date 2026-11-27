#include <assert.h>
#include <stdio.h>
#include "parcel_plan.h"
int main(void){
    unsigned shapes=0;
    for(int h=9;h<=13;h++)for(int pal=0;pal<8;pal++)for(int roof=0;roof<3;roof++)for(int mir=0;mir<2;mir++){
        LotPlan p={0};p.dy=h;p.pal=pal;p.roof=roof;p.mir=mir;p.dx=10;p.drv=14;
        int lo=16,hi=0,last=-1;
        for(int x=6;x<=17;x++){
            int y=bore_parcel_row(&p,x);
            assert(y>=h+2&&y<=16);
            if(last>=0)assert(y-last<=1&&last-y<=1);
            last=y;if(y<lo)lo=y;if(y>hi)hi=y;
            assert(bore_parcel_gate(&p,x)==((x>=9&&x<=11)||(x>=14&&x<=15)));
        }
        shapes+=hi>lo;
    }
    assert(shapes>0);puts("Garden borders: safe stepped profiles, door and driveway gaps PASS");
    return 0;
}
