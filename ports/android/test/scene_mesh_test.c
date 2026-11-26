#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bigmap.h"
#include "world_bridge.h"
#include "scene_mesh.h"
#include "window_math.h"
#include "house_shape.h"
int main(void){
    BoreMesh a,b;
    assert(!bore_scene_build(512,512,0,&a)&&!a.v);
    assert(!bore_scene_build(512,512,5,&a)&&!a.v);
    const unsigned seeds[]={1234,9821,34567};unsigned shapes=0;
    for(int s=0;s<3;s++){
        uint32_t center=bore_android_init(seeds[s]);
        int x=bore_window_center(center&65535,4,BM_N),z=bore_window_center(center>>16,4,BM_N);
        const Settle *capital=bm_set(bm_capital());
        if(capital)for(int bz=capital->y-capital->r;bz<=capital->y+capital->r;bz++)
          for(int bx=capital->x-capital->r;bx<=capital->x+capital->r;bx++){
            if(bx<0||bz<0||bx>=BM_N||bz>=BM_N)continue;
            BInfo lot;LotPlan plan;u8 ft[5];bm_block(bx,bz,&lot);
            if(!bm_lot_foot(bx,bz,&lot,ft,&plan)||!ft[4]||plan.type!=HT_L||!plan.wx1)continue;
            HouseShapeRect outer={ft[0],ft[1],ft[2]+1,ft[3]+1},body={plan.x0,plan.y0,plan.x1+1,plan.y1+1},
                wing={plan.wx0,plan.wy0,plan.wx1+1,plan.wy1+1},bb,ww;
            if(wing.x0<body.x0)wing.x1=body.x0;else wing.x0=body.x1;
            int code=house_shape_encode(outer,body,wing);
            assert(code>0&&house_shape_decode(outer,code,&bb,&ww));
            assert(house_shape_equal(body,bb)&&house_shape_equal(wing,ww));shapes++;
          }
        assert(bore_scene_build(x,z,4,&a));
        assert(a.count>10000&&a.count<=BORE_SCENE_MAX_VERTICES&&a.count%3==0);
        unsigned side=9u*BM_BLK+1u;
        assert(a.height_samples==side*side);
        assert(a.height_samples+a.border_hits==81u*(BM_BLK+1)*(BM_BLK+1));
        assert(a.hill_reads==144u);
        assert(a.height_hits>1000&&a.roof_samples>100);
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
        assert(a.height_samples==b.height_samples&&a.height_hits==b.height_hits&&a.roof_samples==b.roof_samples&&a.border_hits==b.border_hits);
        printf("terrain samples %u; shared boundary hits %u; reused height lookups %u; unique roof corners %u; generator hill reads %u\n",a.height_samples,a.border_hits,a.height_hits,a.roof_samples,a.hill_reads);
        bore_scene_free(&a);bore_scene_free(&b);
    }
    assert(shapes>0);printf("Verified packed skyline shape against %u actual generated L homes\n",shapes);
    assert(bore_scene_build(-500,2000,4,&a)&&a.count>0&&a.hill_reads==144u);
    bore_scene_free(&a);
    assert(bore_scene_build(512,512,1,&a)&&a.count>0&&a.hill_reads==36u);
    assert(a.height_samples==(3u*BM_BLK+1)*(3u*BM_BLK+1));
    assert(a.height_samples+a.border_hits==9u*(BM_BLK+1)*(BM_BLK+1));
    bore_scene_free(&a);
    puts("Native 3D mesh: deterministic, bounded, finite, volumetric");
    return 0;
}
