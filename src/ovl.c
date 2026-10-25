// ovl.c - see ovl.h
#include "ovl.h"
#include "crowd.h"
#include "citizens.h"
#include "audio.h"
#include "sim.h"
#include <psxapi.h>
extern void *const ovl_keep_tab[];   // (ovl_keep.c, made by tools/ovl.py: every symbol of BORE2.EXE an overlay uses, so the linker keeps it)
const OvlHead *ovl_load(const char *file) {
    if (!ovl_keep_tab[0]) return 0;
    u32 n; u8 *base = (u8 *)crowd_scratch(&n) + OVL_AT; int room = (int)n - OVL_AT;
    crowd_reset();                                           // (the crowd's models are let go: the code goes where they were)
    memset(base, 0, room);                                   // (the overlay's zeroed data comes zeroed)
    if (!audio_cd_load(file, base, room)) { sim_note("THE DISC COULD NOT BE READ. TRY AGAIN"); citizens_rebuild(); return 0; }
    FlushCache();                                            // (new code: the CPU must not run what its cache remembers)
    const OvlHead *h = (const OvlHead *)base;
    if (h->magic != OVL_MAGIC) { sim_note("THAT PART OF THE DISC IS DAMAGED"); citizens_rebuild(); return 0; }
    return h;
}
void ovl_leave(void) { crowd_reset(); citizens_rebuild(); }
