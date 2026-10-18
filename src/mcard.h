// mcard: the memory card (slot 1), straight over the serial port.
#pragma once
#include "common.h"

enum { MC_OK, MC_NOCARD, MC_UNFORMATTED, MC_NOFILE, MC_READ, MC_WRITE, MC_FULL, MC_TOOBIG };
extern int mcard_last_err;                       // why the last call said 0
extern int mcard_dir_keep;                       // 1: reuse the directory read last (several files looked at in a row)

int  mcard_read(int sector, u8 *out);            // one 128-byte sector: 1 read
int  mcard_write(int sector, const u8 *in);      // one 128-byte sector: 1 written
int  mcard_present(void);                        // a formatted card is in slot 1
// a file is read or written as a stream
int  mcard_rbegin(const char *name);             // 1: found
void mcard_rget(void *out, int n);
int  mcard_rok(void);                            // 1: every sector so far came in
int  mcard_wbegin(const char *name, int blocks); // 1: room for it
void mcard_wput(const void *in, int n);
int  mcard_wpatch(int sector, const u8 *sec);    // rewrite one sector of the open file
int  mcard_wend(void);                           // 1: written
void mcard_title_sector(u8 *sec, const char *title, int blocks, const u16 *pal);
