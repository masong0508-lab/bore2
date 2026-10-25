// realm_res.c - the part of KINGDOM MODE that stays in BORE2.EXE (the rest is KINGDOM.OVL): the realm's saved bytes, which live in the
// simulation's distance-map cache while the realm is not running, and the mission you are on.
#include "realm.h"
#include "save.h"
#include "realm/kingdom.h"
#include "realm/kingsim.h"
KSim realm_S; int realm_blob_len, realm_mis, realm_live;
#define BLOB ((u8 *)realm_S.dist)
#define BLOB_CAP ((int)(sizeof realm_S.dist + sizeof realm_S.queue))
void realm_reset(void) { realm_blob_len = 0; realm_live = 0; realm_mis = 0; }
void realm_save(void) { sv_u8((u8)realm_mis); sv_u16((u16)realm_blob_len); sv_n(BLOB, realm_blob_len); }
void realm_load(int ver) {
    realm_live = 0; realm_mis = 0; realm_blob_len = 0;
    if (ver < 15) return;
    realm_mis = ld_u8(); int n = ld_u16(); if (n > BLOB_CAP) n = 0; ld_n(BLOB, n); realm_blob_len = n;
}
