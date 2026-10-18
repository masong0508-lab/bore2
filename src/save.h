// save: the game on a memory card. The saving code of each part (sim.c, world.c, pop.c, island.c, main.c) writes through the
// sv_ calls and reads back through the ld_ ones; save.c does the card, the header and the packing.
#pragma once
#include "common.h"

void sv_u8(int v);  void sv_u16(int v);  void sv_u32(u32 v);  void sv_n(const void *p, int n);
void sv_rle(const void *p, int n);                // n bytes, packed (runs of the same byte cost two)
int  ld_u8(void);   int  ld_u16(void);   u32 ld_u32(void);    void ld_n(void *p, int n);
void ld_rle(void *p, int n);
void sv_str(const char *s, int cap);              // a text of at most cap bytes
void ld_str(char *s, int cap);
void ld_fail(void);                               // the save cannot be used (it says so when the read is done)

int  save_exists(void);                           // 1: a BORE 2 save is on the card (and fits this version)
int  save_write(void (*body)(void));              // 1: saved
int  save_read(void (*body)(void));               // 1: read (the body ran)
const char *save_error(void);                     // what went wrong, in a few plain words
