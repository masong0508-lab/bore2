#include <assert.h>
#include <stdio.h>
#include "frame_render_policy.h"
#include "ground_patch.h"
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
    int owner=0;
    for(int i=0;i<80;i++){owner=render_house_detail(owner,7+i%2,12,7,0,1);assert(owner);}
    assert(!render_house_detail(owner,10,12,7,0,1));
    assert(render_house_detail(0,25,25,7,1,1));
    assert(render_house_detail(0,25,25,7,0,0));
    assert(render_box_gap(2,4,5,3,9,8)==3);
    assert(render_screen_plane(0,0,100,230,160,120,320,240,1)>0);
    assert(render_screen_plane(-200,0,100,230,160,120,320,240,1)<0);
    assert(render_screen_plane(0,200,100,230,160,120,320,240,4)<0);
    uint8_t mat[324],covered[324];uint32_t patches[324];
    for(int pattern=0;pattern<3;pattern++){
        for(int y=0;y<18;y++)for(int x=0;x<18;x++)
            mat[y*18+x]=(uint8_t)(pattern==0?((x<5||y<5)?20:(x==5||y==5)?15:14):pattern==1?((x+y)&1):((x*31+y*17)%33));
        memset(covered,0,sizeof covered);
        int n=ground_patch_build(mat,patches,324);
        assert(n>0&&n<=324);
        assert(ground_patch_build(mat,patches,64)==n);
        n=ground_patch_build(mat,patches,324);
        for(int i=0;i<n;i++){
            uint32_t p=patches[i];
            for(int y=ground_patch_y(p);y<ground_patch_y(p)+ground_patch_h(p);y++)
                for(int x=ground_patch_x(p);x<ground_patch_x(p)+ground_patch_w(p);x++){
                    assert(x>=0&&x<18&&y>=0&&y<18);int k=y*18+x;
                    assert(!covered[k]&&mat[k]==ground_patch_mat(p));covered[k]=1;
                }
        }
        for(int k=0;k<324;k++)assert(covered[k]);
    }
    puts("Frame ownership, aligned packets, material-exact terrain and stable silhouettes PASS");
}
