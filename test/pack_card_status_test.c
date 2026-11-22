// Ensure base-game and placeholder cards never index the 12-pack arrays.
#include "../src/pack_card_status.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    int have[12];
    for (unsigned i = 0; i < 12; ++i) have[i] = -1;
    assert(pack_card_status(K_BASE, 255, have) == CARD_BASE);
    assert(pack_card_status(K_BASE, 0, have) == CARD_BASE);
    assert(pack_card_status(K_SOON, 255, have) == CARD_COMING);
    assert(pack_card_status(K_PACK, 255, have) == CARD_COMING);
    assert(pack_card_status(K_PACK, 12, have) == CARD_COMING);
    assert(pack_card_status(K_PACK, 11, have) == CARD_CHECKING);
    have[11] = 0;
    assert(pack_card_status(K_PACK, 11, have) == CARD_MISSING);
    have[11] = 1;
    assert(pack_card_status(K_PACK, 11, have) == CARD_INSTALLED);
    puts("Base / pack / placeholder card status: PASS");
    return 0;
}
