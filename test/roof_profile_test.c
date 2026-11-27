#include <assert.h>
#include <stdio.h>
#include "roof_profile.h"
int main(void){
    BoreRoofRun r=bore_roof_run(12,8,1,3);
    assert(r.x==3&&r.z==3&&r.run==3); // hip has a broad top, not a pyramid
    r=bore_roof_run(12,4,1,3);assert(r.x==2&&r.z==2&&r.run==2);
    r=bore_roof_run(12,8,2,3);assert(r.x==0&&r.z==3&&r.run==3);
    r=bore_roof_run(8,12,2,3);assert(r.x==3&&r.z==0&&r.run==3);
    r=bore_roof_run(12,4,3,3);assert(r.x==3&&r.z==0&&r.run==3);
    r=bore_roof_run(2,10,3,3);assert(r.x==1&&r.z==0&&r.run==1);
    for(int w=1;w<=18;w++)for(int d=1;d<=18;d++)for(int s=1;s<=3;s++){
        r=bore_roof_run(w*256,d*256,s,3*256);
        assert(r.run>0&&r.run<=768&&2*r.x<=w*256&&2*r.z<=d*256);
        BoreRoofRun ds=bore_roof_run(w*660,d*660,s,6*330);
        assert(ds.x*256==r.x*660&&ds.z*256==r.z*660);
    }
    assert(!bore_roof_run(12,8,0,3).run);
    // Eye just above eaves but below the ridge: the near slope can show.
    assert(bore_roof_face_visible(1,-1,3,2));
    assert(!bore_roof_face_visible(2,-3,3,2)); // exactly edge on
    // An outward view from below the eaves can still see the pitched face.
    assert(bore_roof_face_visible(-2,5,3,2));
    assert(!bore_roof_face_visible(-2,2,3,2));
    // Vertical gable ends depend only on outward distance, not eye height.
    assert(bore_roof_face_visible(-100,1,0,2));
    assert(!bore_roof_face_visible(100,-1,0,2));
    assert(bore_roof_face_visible(1,-100,3,0));
    assert(!bore_roof_face_visible(-1,100,3,0));
    puts("Roof profiles and world-space face visibility agree across native units");
}
