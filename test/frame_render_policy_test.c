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
    puts("Frame ownership, cache invalidation and in-flight texture protection PASS");
}
