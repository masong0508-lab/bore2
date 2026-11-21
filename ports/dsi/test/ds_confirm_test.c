/* Host-side test for the destructive-action confirmation used by DS title.
 * cc -std=c99 -Wall -Wextra -Werror ports/dsi/test/ds_confirm_test.c -o /tmp/ds_confirm_test && /tmp/ds_confirm_test */
#include <assert.h>
#include <stdio.h>
#include "../source/ds_confirm.h"
int main(void) {
    unsigned int stamp = 0;
    unsigned char armed = 0;
    assert(!ds_confirm_twice(100u, &stamp, &armed, 180u));
    assert(armed && stamp == 100u);
    assert(!ds_confirm_expired(280u, stamp, 180u));
    assert(ds_confirm_twice(280u, &stamp, &armed, 180u));
    assert(!armed);
    assert(!ds_confirm_twice(500u, &stamp, &armed, 180u));
    assert(ds_confirm_expired(681u, stamp, 180u));
    assert(!ds_confirm_twice(682u, &stamp, &armed, 180u));
    assert(armed && stamp == 682u);
    armed = 0; /* user selected a different slot or left the screen */
    assert(!ds_confirm_twice(683u, &stamp, &armed, 180u));
    assert(ds_confirm_twice(685u, &stamp, &armed, 180u));
    /* Unsigned vblank rollover must not disable or bypass confirmation. */
    assert(!ds_confirm_twice(0xfffffff0u, &stamp, &armed, 180u));
    assert(ds_confirm_twice(0x00000020u, &stamp, &armed, 180u));
    assert(!armed);
    puts("ds_confirm_test: PASS");
    return 0;
}
