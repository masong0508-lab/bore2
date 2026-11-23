// Host C99 assertions for actual Dreamcast controller mappings.
// Run with cc -std=c99 -Wall -Wextra -Werror test/dc_pad_map_test.c.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../ports/dreamcast/dc_pad_map.h"

static void check(uint32_t in, unsigned lt, unsigned rt, uint16_t want) {
    uint16_t got = dc_pad_translate(in, lt, rt);
    if (got != want) {
        fprintf(stderr, "Dreamcast pad map FAIL: flags=%08lx lt=%u rt=%u: %04x != %04x\n",
                (unsigned long)in, lt, rt, got, want);
        assert(got == want);
    }
}

int main(void) {
    check(0, 0, 0, 0);
    check(DCI_A, 0, 0, 0x4000);
    check(DCI_B, 0, 0, 0x2000);
    check(DCI_X, 0, 0, 0x8000);
    check(DCI_Y, 0, 0, 0x1000);
    check(DCI_START, 0, 0, 0x0008);
    check(DCI_START, 200, 200, 0x0001); // START + both triggers = SELECT
    check(DCI_D, 0, 0, 0x0001);
    check(DCI_Z | DCI_C, 0, 0, 0x0300);
    check(DCI_UP | DCI_LEFT, 0, 0, 0x0090);
    check(0, 120, 0, 0x0400);          // LT alone = L1
    check(0, 0, 130, 0x0800);          // RT alone = R1
    check(0, 200, 200, 0x0c00);       // both, no direction = L1 + R1
    check(DCI_Y, 200, 200, 0x0300); // both triggers + Y = L2 + R2 (cycle camera)
    check(DCI_UP, 200, 200, 0x0100);  // storey down / zoom in
    check(DCI_DOWN, 200, 200, 0x0200);// storey up / zoom out
    check(DCI_LEFT, 200, 200, 0x0002);// roof toggle (L3)
    check(DCI_RIGHT, 200, 200, 0x0004);// catalog tab (R3)
    check(DCI_A | DCI_UP, 200, 200, 0x4100);
    check(DCI_UP, 100, 200, 0x0810); // threshold: single RT + ordinary UP
    check(DCI_RIGHT, 255, 50, 0x0420);
    printf("Dreamcast physical pad translation PASS (21 cases)\n");
    return 0;
}
