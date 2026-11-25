// Shared automatic area score: the eight appended bank entries follow the
// original 52 tracks. The generator asserts that order in both bank formats.
#pragma once
#include "bigmap.h"
enum { SC_NEIGHBORHOOD, SC_TRAVEL, SC_STREET, SC_WATER, SC_MARKET, SC_BUILD, SC_SKATE, SC_ISLAND, SC_N };
#define SC_FIRST 52
enum { SC_MENU_NONE, SC_MENU_TRAVEL, SC_MENU_BUILD };
static inline int scene_music_role(int menu, int island, int riding, int driving, int lot, int terrain, int street) {
    if (menu == SC_MENU_BUILD) return SC_BUILD;
    if (menu == SC_MENU_TRAVEL) return SC_TRAVEL;
    if (riding || lot == LOT_SKATE || lot == LOT_BAR) return SC_SKATE;
    if (island) return SC_ISLAND;
    if (driving) return SC_TRAVEL;
    if (lot == LOT_RIVER || lot == LOT_POOL || terrain == BT_BEACH) return SC_WATER;
    if (street) return SC_STREET;
    switch (lot) {
        case LOT_SHOP: case LOT_DINER: case LOT_CAFE: case LOT_CONV:
        case LOT_GAS: case LOT_RESTAURANT: case LOT_FARM: return SC_MARKET;
        case LOT_OFFICE: case LOT_TOWER: case LOT_ARCADE: case LOT_GYM:
        case LOT_SQUARE: case LOT_CINEMA: return SC_STREET;
        default: return SC_NEIGHBORHOOD;
    }
}
typedef struct { int candidate, count, settled; } SceneMusicGate;
#define SC_MUSIC_GATE_INIT {-1,0,-1}
static inline int scene_music_settle(SceneMusicGate *g,int cue) {
    if (cue < 0 || cue >= SC_N) { g->candidate=-1;g->count=0;return g->settled; }
    // Interior and street boundaries need longer dwell than the build,
    // travel and skating activities. Fast initial entry remains responsive.
    int need=g->settled<0 || cue==SC_BUILD || cue==SC_TRAVEL || cue==SC_SKATE || cue==SC_ISLAND ? 2 : 4;
    if (cue != g->candidate) { g->candidate=cue; g->count=1; }
    else if (g->count < need) g->count++;
    if (g->count >= need) g->settled=cue;
    return g->settled;
}
static inline void scene_music_reset(SceneMusicGate *g) {
    g->candidate=g->settled=-1;g->count=0;
}
