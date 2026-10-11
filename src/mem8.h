// mem8: is this a PlayStation with 8 MB of RAM (a development console, or DuckStation's "Enable 8MB RAM")?
// One disc for both: with 2 MB the game fits everything it needs into the normal RAM; with 8 MB it takes the other
// 6 MB for more (more storeys, a bigger people store, the far skyline, everything remembered).
#pragma once
#include "common.h"

extern int ram8;                       // 1: 8 MB
void  mem8_init(void);                 // first thing in main
void *xalloc(u32 bytes);               // from the extra 6 MB (0 with 2 MB, or when it has run out); never freed
u32   xfree_bytes(void);               // what is left of it
// a copy that moves whole words where it can (the SDK's memcpy and memmove go a byte at a time), overlapping either way
void  wmove(void *dst, const void *src, u32 n);
