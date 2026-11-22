// Reproduces the five-goal upgrade over old three-goal save flags.
#include "../src/pack_master.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    assert(pack_goal_mask(0u) == 0u && pack_goal_mask(6u) == 0u);
    for (unsigned n = 1u; n <= 5u; ++n) {
        unsigned mask = pack_goal_mask(n);
        assert(mask == (1u << n) - 1u);
        assert(!pack_master_due(mask - 1u, n));
        assert(pack_master_due(mask, n));
        assert(!pack_master_complete(mask, n));
        assert(pack_master_complete(mask | PACK_MASTER_FLAG, n));
        assert(!pack_master_due(mask | PACK_MASTER_FLAG, n));
    }
    // Historical DOG save: the $3,000 reward was already paid for 3 goals.
    // Keep that entitlement bit, but do not display completion at 3/5 or
    // pay the reward again when the two new goals are completed.
    unsigned old_dog = PACK_MASTER_FLAG | 0x07u;
    assert(pack_master_complete(old_dog, 3u));
    assert(!pack_master_complete(old_dog, 5u));
    assert(!pack_master_due(old_dog, 5u));
    assert(!pack_master_due(old_dog | 0x18u, 5u));
    assert(pack_master_complete(old_dog | 0x18u, 5u));
    // Recovery case: all claims exist in a save but the bonus bit does not.
    // The Pack Goals menu must surface the X CLAIM action for that bonus.
    assert(pack_master_due(0x1fu, 5u));
    assert(!pack_master_due(0x9fu, 5u));
    assert(!pack_master_due(0x1fu, 0u));
    assert(!pack_master_complete(0x9fu, 6u));
    puts("Pack master migration and bonus rules: PASS");
    return 0;
}
