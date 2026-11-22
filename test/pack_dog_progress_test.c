// Host C99 regression for the actual pack goal rules used by gameplay and PKINFO.
#include "../src/pack_dog_progress.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    unsigned count = 0;
    assert(pack_dog_play_next(count, 0) == 0);  // no pet
    assert(pack_dog_play_next(count, 2) == 0);  // cats do not count
    for (int i = 0; i < 4; ++i) count = pack_dog_play_next(count, 1);
    assert(count == 4 && !pack_dog_play_goal(count));
    count = pack_dog_play_next(count, 1);
    assert(count == 5 && pack_dog_play_goal(count));
    assert(pack_dog_play_next(255, 1) == 255);  // saved u8 saturates
    assert(!pack_dog_styled(0, 1, 0, 0));
    assert(!pack_dog_styled(2, 1, 0, 0));
    assert(!pack_dog_styled(1, 0, 0, 0));
    assert(pack_dog_styled(1, 1, 0, 0));       // collar
    assert(pack_dog_styled(1, 0, 1, 0));       // hat
    assert(pack_dog_styled(1, 0, 0, 1));       // outfit
    puts("Create a Dog pack goals: PASS");
    return 0;
}
