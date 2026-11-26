// Actual streaming policy: fixed generator and pending-roof masks.
#include "../src/world_stream_models.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    unsigned char generation[25] = {0}, roof[25] = {0};
    assert(stream_model_state(generation, roof, 5, -1, 0) == STREAM_MODEL_READY);
    assert(stream_model_state(generation, roof, 5, 5, 0) == STREAM_MODEL_READY);
    assert(stream_model_state(generation, roof, 5, 4, 4) == STREAM_MODEL_READY);
    generation[4 * 5 + 4] = 1;
    assert(stream_model_state(generation, roof, 5, 4, 4) == STREAM_MODEL_GENERATING);
    assert(stream_house_proxy_needed(STREAM_MODEL_GENERATING, 1));
    generation[24] = 0; roof[24] = 1;
    assert(stream_model_state(generation, roof, 5, 4, 4) == STREAM_MODEL_ROOF_PENDING);
    assert(stream_house_proxy_needed(STREAM_MODEL_ROOF_PENDING, 0));
    assert(!stream_house_proxy_needed(STREAM_MODEL_ROOF_PENDING, 1));
    assert(!stream_house_proxy_needed(STREAM_MODEL_READY, 0));
    roof[24] = 0;
    assert(!stream_house_proxy_needed(stream_model_state(generation, roof, 5, 4, 4), 0));

    // Old row-major scheduling picked 0,0, even when an entering roof
    // directly ahead of the player at 4,3 was waiting.
    roof[0] = roof[3 * 5 + 4] = roof[3 * 5 + 2] = 1;
    int chosen = stream_pick_roof(roof, 5, 16, 66, 54, 65, 51);
    assert(chosen == 3 * 5 + 4); // nearest camera+player
    roof[chosen] = 0;
    chosen = stream_pick_roof(roof, 5, 16, 39, 55, 38, 57);
    assert(chosen == 3 * 5 + 2);
    roof[chosen] = roof[0] = 0;
    assert(stream_pick_roof(roof, 5, 16, 30, 30, 31, 31) == -1);

    assert(stream_block_gap(79, 32, 4, 2, 16) == 0); // last tile is inclusive
    assert(stream_block_gap(80, 32, 4, 2, 16) == 1); // first outside tile
    assert(stream_block_gap(82, 46, 4, 2, 16) == 3);
    generation[0]=generation[24]=1;
    assert(stream_pick_generation(generation,5,16,31,31,75,75)==24);
    assert(stream_pick_generation(generation,5,16,15,15,75,75)==0);
    assert(stream_pick_generation(generation,5,16,16,16,75,75)==0);
    generation[0]=generation[24]=0;
    assert(stream_pick_generation(generation,5,16,16,16,75,75)==-1);
    unsigned char q[16]={0};
    q[0]=q[3]=q[5]=1;
    assert(stream_shift_pending(q,4,1,0)==6);
    assert(q[2]&&q[4]&&!q[0]&&q[3]&&q[7]&&q[11]&&q[15]);
    assert(stream_shift_pending(q,4,0,1)==8);
    assert(q[0]&&q[3]&&q[7]&&q[11]&&q[12]&&q[13]&&q[14]&&q[15]);
    assert(stream_shift_pending(q,4,-1,-1)==8);
    // Rapid reverse/diagonal moves agree with a reference by world identity.
    for(int sx=-1;sx<=1;sx++)for(int sy=-1;sy<=1;sy++){
        unsigned char a[16],b[16];
        for(int i=0;i<16;i++)a[i]=b[i]=(unsigned char)((i%3)==0);
        int n=stream_shift_pending(a,4,sx,sy),count=0;
        for(int y=0;y<4;y++)for(int x=0;x<4;x++){
            int X=x+sx,Y=y+sy,v=X<0||Y<0||X>=4||Y>=4?1:b[Y*4+X];
            assert(a[y*4+x]==v);count+=v;
        }
        assert(n==count);
    }
    puts("World streaming: collision-first queue, retained pending blocks and proxy handoff PASS");
    return 0;
}
