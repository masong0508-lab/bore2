// realm: KINGDOM MODE (THE LAND BOREFORE TIME, from BORE on the GBA), in 3D. Your Bore wakes up a GIANT in a muddy field beside a
// very small crowd of Nubbins: gather by hand, lay roads, raise huts and workshops, and the Nubbins haul the goods and staff them; a
// castle in four stages crowns a monarch. Sixteen missions pay into your life. The realm's rules and its save bytes are the GBA's own
// (realm/kingdom.h, kingsim.h, kinglog.h, kinghand.h, kinghold.h, unchanged); this file is the 3D land, the giant and the screens.
#pragma once
#include "common.h"

void realm_open(void);                               // from the pause menu
int  realm_update(int held, int pressed, int steps); // 1 while it runs
void realm_draw(void);
void realm_save(void);                               // with the life (sim.c's save): the realm's bytes
void realm_load(int ver);
void realm_reset(void);                              // a new life: no realm yet
