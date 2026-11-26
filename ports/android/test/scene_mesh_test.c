#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bigmap.h"
#include "world_bridge.h"
#include "scene_mesh.h"
#include "window_math.h"
int main(void){
    BoreMesh a,b;
    assert(!bore_scene_build(512,512,0,&a)&&!a.v);
    assert(!bore_scene_build(512,512,5,&a)&&!a.v);
    const unsigned seeds[]={1234,9821,34567};
    for(int s=0;s<3;s++){
        uint32_t center=bore_android_init(seeds[s]);
        int x=bore_window_center(center&65535,4,BM_N),z=bore_window_center(center>>16,4,BM_N);
        assert(bore_scene_build(x,z,4,&a));
        assert(a.count>10000&&a.count<=BORE_SCENE_MAX_VERTICES&&a.count%3==0);
        float lo=1e9f,hi=-1e9f;int walls=0;
        for(int i=0;i<a.count;i++){
            float *p=a.v+i*6;
            for(int j=0;j<6;j++)assert(isfinite(p[j]));
            assert(fabsf(p[0])<=81.1f&&fabsf(p[2])<=81.1f);
            for(int j=3;j<6;j++)assert(p[j]>=0&&p[j]<=1);
            lo=fminf(lo,p[1]);hi=fmaxf(hi,p[1]);
            if(i%3==0){
                const float *q=p+6,*r=p+12;
                if((fabsf(p[0]-q[0])<.0001f&&fabsf(p[0]-r[0])<.0001f)||
                   (fabsf(p[2]-q[2])<.0001f&&fabsf(p[2]-r[2])<.0001f))walls++;
            }
        }
        assert(hi-lo>1.5f&&walls>0);
        assert(bore_scene_build(x,z,4,&b)&&a.count==b.count);
        assert(!memcmp(a.v,b.v,(size_t)a.count*6*sizeof(float)));
        printf("seed %u: %d vertices, %d vertical triangles, height %.2f..%.2f\n",seeds[s],a.count,walls,lo,hi);
        bore_scene_free(&a);bore_scene_free(&b);
    }
    assert(bore_scene_build(-500,2000,4,&a)&&a.count>0);
    bore_scene_free(&a);
    puts("Native 3D mesh: deterministic, bounded, finite, volumetric");
    return 0;
}
