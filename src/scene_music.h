// Shared automatic area score: the eight appended bank entries follow the
// original 52 tracks. The generator asserts that order in both bank formats.
#pragma once
#include "bigmap.h"
enum { SC_NEIGHBORHOOD, SC_TRAVEL, SC_STREET, SC_WATER, SC_MARKET, SC_BUILD, SC_SKATE, SC_ISLAND, SC_CAFE, SC_LIBRARY, SC_GYM, SC_POOL, SC_CINEMA, SC_FARM, SC_PARK, SC_ARCADE, SC_GALLERY, SC_HOTEL, SC_SCHOOL, SC_SPACE, SC_N };
#define SC_FIRST 52
enum { SC_MENU_NONE, SC_MENU_TRAVEL, SC_MENU_BUILD };
// Keep the original eight cue IDs; new venue cues are appended in bank order.
#define SC_VENUE_FIRST SC_CAFE
#define SC_VENUE_COUNT 12
static inline int scene_music_venue(int lot) {
    switch(lot) {
    case LOT_CAFE: return SC_CAFE;
    case LOT_LIBRARY: return SC_LIBRARY;
    case LOT_GYM: return SC_GYM;
    case LOT_POOL: return SC_POOL;
    case LOT_CINEMA: return SC_CINEMA;
    case LOT_FARM: return SC_FARM;
    case LOT_PARK: return SC_PARK;
    case LOT_ARCADE: return SC_ARCADE;
    case LOT_GALLERY: return SC_GALLERY;
    case LOT_HOTEL: case LOT_INN: case LOT_SPA: return SC_HOTEL;
    case LOT_SCHOOL: return SC_SCHOOL;
    case LOT_SPACE: case LOT_OBS: return SC_SPACE;
    default: return -1;
    }
}
static inline int scene_music_role(int menu, int island, int riding, int driving, int lot, int terrain, int street) {
    if (menu == SC_MENU_BUILD) return SC_BUILD;
    if (menu == SC_MENU_TRAVEL) return SC_TRAVEL;
    if (riding || lot == LOT_SKATE || lot == LOT_BAR) return SC_SKATE;
    if (island) return SC_ISLAND;
    if (driving) return SC_TRAVEL;
    if (lot == LOT_RIVER) return SC_WATER;
    if (street) return terrain == BT_BEACH ? SC_WATER : SC_STREET;
    int venue=scene_music_venue(lot);if(venue>=0)return venue;
    if (terrain == BT_BEACH) return SC_WATER;
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
