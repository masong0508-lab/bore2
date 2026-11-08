// ovl.c - see ovl.h
#include "ovl.h"
#include "crowd.h"
#include "citizens.h"
#include "audio.h"
#include "sim.h"
#include <psxapi.h>
#include <stdio.h>
#include <string.h>
extern void *const ovl_keep_tab[];   // (ovl_keep.c, made by tools/ovl.py: every symbol of BORE2.EXE an overlay uses, so the linker keeps it)
static const struct { const char *file, *name; } packNm[] = { { "KINGDOM.OVL", "THE LAND BOREFORE TIME" }, { "ARCADE.OVL", "BORTUA FIGHTER" }, { "ARCADE2.OVL", "BORTUA FIGHTER" }, { "GOTCHI.OVL", "BOREAGOTCHI!" }, { "PARTY.OVL", "FAMILY FUN" },
    { "DJ.OVL", "DJ BOOTH" }, { "SUPER.OVL", "SUPERSTAR" }, { "DOG.OVL", "CREATE A DOG" }, { "PRISON.OVL", "CRIME AND LAW" }, { "SPACE.OVL", "SPACE AGE" }, { "PARADISE.OVL", "ISLAND PARADISE" }, { "FARM.OVL", "FARM LIFE" }, { "SKYSEA.OVL", "SKY AND SEA" }, { "CREATE.OVL", "CREATE A BORE" } };
const char *ovl_pack_name(const char *file) { for (unsigned i = 0; i < sizeof packNm / sizeof packNm[0]; i++) if (!strcmp(packNm[i].file, file)) return packNm[i].name; return file; }
int ovl_installed(const char *file) {   // the first sector of the file: a real overlay starts "BOVL", a stub of the base disc "BSTB"
    extern int audio_cd_peek(const char *name, void *dst);
    static u32 sec[512]; if (!audio_cd_peek(file, sec)) return 0;
    return sec[0] == OVL_MAGIC;
}
static char pk_msg[40];
const OvlHead *ovl_load(const char *file) {
    if (!ovl_keep_tab[0]) return 0;
    u32 n; int at = strcmp(file, "GOTCHI.OVL") ? OVL_AT : 0;   // (the toy needs no realm land below it: it loads at the start of the arena)
    u8 *base = (u8 *)crowd_scratch(&n) + at; int room = (int)n - at;
    crowd_reset();                                           // (the crowd's models are let go: the code goes where they were)
    memset(base, 0, room);                                   // (the overlay's zeroed data comes zeroed)
    if (!audio_cd_load(file, base, room)) { sim_note("THE DISC COULD NOT BE READ. TRY AGAIN"); citizens_rebuild(); return 0; }
    FlushCache();                                            // (new code: the CPU must not run what its cache remembers)
    const OvlHead *h = (const OvlHead *)base;
    if (h->magic == OVL_STUB) { snprintf(pk_msg, sizeof pk_msg, "%s IS NOT INSTALLED", ovl_pack_name(file)); sim_note(pk_msg); citizens_rebuild(); return 0; }
    if (h->magic != OVL_MAGIC) { sim_note("THAT PART OF THE DISC IS DAMAGED"); citizens_rebuild(); return 0; }
    return h;
}
void ovl_leave(void) { crowd_reset(); citizens_rebuild(); }
