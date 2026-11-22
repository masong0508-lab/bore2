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

    assert(stream_block_gap(80, 32, 4, 2, 16) == 0); // exact edge
    assert(stream_block_gap(79, 32, 4, 2, 16) == 1);
    assert(stream_block_gap(82, 46, 4, 2, 16) == 2);
    puts("World streaming: pending house proxy and nearest-roof scheduling PASS");
    return 0;
}
