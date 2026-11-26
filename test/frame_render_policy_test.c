#include <assert.h>
#include <stdio.h>
#include "frame_render_policy.h"
int main(void){
    for(int mask=0;mask<16;mask++)assert(render_floor_group(mask)==(mask==0?1:mask==15?2:0));
    assert(render_group_fits(0,0,71,71,72,72));
    assert(!render_group_fits(70,71,71,71,72,72));
    assert(!render_group_fits(4,4,4,10,72,72));
    assert(render_eye_close(7,4,10,4,12,8));
    assert(!render_eye_close(6,4,10,4,12,8));
    // A lot crossing dirties ALL three cache pieces at once.
    for(int i=0;i<3;i++){assert(render_piece_dirty(10,11,100,0));assert(!render_piece_dirty(11,11,100,0));}
    assert(render_piece_dirty(11,11,100,1)&&render_piece_dirty(11,11,0,0));
    assert(!render_cache_prefer(1,1,0,0,0,0));
    assert(!render_cache_prefer(1,1,1,1,1,100));
    assert(render_cache_prefer(0,1,1,1,1,100));
    assert(render_cache_prefer(0,0,100,1,1,1));
    assert(!render_cache_prefer(0,1,1,1,0,100));
    assert(render_packet_bytes(5,5)==0&&render_packet_bytes(5,8)==8);
    assert(render_packet_bytes(4,-1)==0&&render_packet_bytes(0,100)==0);
    assert(render_packet_bytes(0x7fffffff,0x7fffffff)==0);
    for(int z=16;z<=16000;z+=16)for(int shift=1;shift<=2;shift++){
        int scaled=z/16,sum=scaled*4,want=(z/4)>>shift;
        assert(render_proxy_slot(sum,4,shift,4096)==(want<1?1:want));
        int tri=(scaled*3*341)>>8;
        assert(render_proxy_slot(scaled*3,3,shift,4096)==((tri>>shift)<1?1:tri>>shift));
    }
    assert(render_proxy_slot(100000,4,2,2044)==2044);
    for(int sh=1;sh<=2;sh++){
        int near=8192>>sh,last=near+252,prior=0;
        for(int depth=0;depth<=450000;depth+=16){
            int q=depth/4,i=render_world_depth_slot(q,sh,near,last);
            assert(i>=prior&&i<=last&&i!=near-3&&i!=near-2&&i!=near-1);
            if(q<8192)assert(i==render_proxy_slot(depth/4,4,sh,near-4));
            assert(render_proxy_depth((depth/16)*4,4)==q);
            prior=i;
        }
        assert(render_world_depth_slot(8191,sh,near,last)==near-4);
        assert(render_world_depth_slot(8192,sh,near,last)==near);
    }
    assert(render_lift_translation(300,1000,4096)==-700);
    assert(render_lift_translation(300,1000,-2048)==800);
    assert(render_lift_translation(300,1000,0)==300);
    puts("Frame ownership, aligned packets, unified near/far depth and lifted clipping PASS");
}
