// save: the game on a memory card. The saving code of each part (sim.c, world.c, pop.c, island.c, main.c) writes through the
// sv_ calls and reads back through the ld_ ones; save.c does the card, the header and the packing.
#pragma once
#include "common.h"
#include "look.h"

void sv_u8(int v);  void sv_u16(int v);  void sv_u32(u32 v);  void sv_n(const void *p, int n);
void sv_rle(const void *p, int n);                // n bytes, packed (runs of the same byte cost two)
int  ld_u8(void);   int  ld_u16(void);   u32 ld_u32(void);    void ld_n(void *p, int n);
void ld_rle(void *p, int n);
void sv_str(const char *s, int cap);              // a text of at most cap bytes
void ld_str(char *s, int cap);
void ld_fail(void);                               // the save cannot be used (it says so when the read is done)

#define SAVE_SLOTS 3
typedef struct { char name[NAME_LEN]; u16 day; u8 nsims, zone; s32 cash; } SaveInfo;   // what the slot list shows
void save_set_info(const SaveInfo *i);            // (before save_write)
int  save_info(int slot, SaveInfo *out);          // 1: a save of this version is in that slot
int  save_write(int slot, void (*body)(void));    // 1: saved
int  save_read(int slot, void (*body)(void));     // 1: read (the body ran)
const char *save_error(void);                     // what went wrong, in a few plain words
