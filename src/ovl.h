// ovl: the OVERLAYS. KINGDOM MODE and the ARCADE's cabinets are not in BORE2.EXE: each is a file on the disc (KINGDOM.OVL, ARCADE.OVL)
// read into the town crowd's arena when you walk into it (the crowd sleeps while you are there, as it did before), so their code takes
// no memory the rest of the time. An overlay is linked to run at that one address against BORE2.EXE's own symbols (tools/ovl.py);
// it starts with an OvlHead. Leaving one gives the arena back to the crowd (its people are made again, nearest first).
#pragma once
#include "common.h"

#define OVL_MAGIC 0x4C564F42u            // "BOVL"
#define OVL_AT    73728                  // where in the arena an overlay lands (the realm's land and buildings sit below it)
#define OVL_STUB  0x42545342u            // "BSTB": the stub in a pack's slot on the BASE disc (the pack is a patch: tools/packs.py)
typedef struct { u32 magic; void (*open)(int arg); int (*update)(int held, int pressed, int steps); void (*draw)(void); } OvlHead;
const OvlHead *ovl_load(const char *file);   // 0 when it cannot be read (the reason in a note)
int  ovl_installed(const char *file);       // 1 if the disc holds the real file (not a stub, not missing): the pack is on the disc
const char *ovl_pack_name(const char *file);
void ovl_leave(void);                        // the arena back to the crowd
