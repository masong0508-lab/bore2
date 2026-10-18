// island: see island.h
#include "island.h"
#include "world.h"
#include "items.h"
#include "sim.h"
#include "audio.h"
#include "ui.h"
#include "save.h"
#include "wants.h"

int isl_wood, isl_stone, isl_leaves, isl_food, isl_spear, isl_shells, isl_craft_asked;
u16 isl_secrets;
u8 isl_dock_x, isl_dock_y, isl_islet_x, isl_islet_y, isl_fire_x, isl_fire_y;

void isl_reset(void) { isl_wood = isl_stone = isl_leaves = 0; isl_food = 3; isl_spear = 0; isl_shells = 0; isl_secrets = 0; isl_craft_asked = 0; }
int isl_secret_count(void) { int n = 0; for (int i = 0; i < SEC_N; i++) n += (isl_secrets >> i) & 1; return n; }
static const char *const secNm[SEC_N] = { "THE MESSAGE IN A BOTTLE", "THE BURIED TREASURE", "THE TIKI IDOL", "THE SHIPWRECK",
                                          "ALL SIX SEASHELLS", "THE HERMIT'S JOURNAL", "THE ISLET", "THE GOLDEN IDOL" };
const char *isl_secret_name(int sec) { return secNm[sec % SEC_N]; }
void isl_found(int sec) {
    if ((isl_secrets >> sec) & 1) return;
    isl_secrets |= (u16)(1 << sec);
    for (int i = 0; i < nsims; i++) wants_event(&sims[i], W_SECRET, 0);
    char t[40]; snprintf(t, sizeof t, "SECRET FOUND! %d OF %d", isl_secret_count(), SEC_N); sim_note(t);
    audio_voice(VOICE_WIN);
    if (isl_secret_count() == SEC_N) sim_note("EVERY SECRET OF THE ISLAND!");
}

const Craft isl_crafts[ISL_NCRAFT] = {
    { "TENT", 3, 0, 4, IC_BED, "A DRY PLACE TO SLEEP" },
    { "LOG BENCH", 2, 0, 0, IC_SOFA, "SIT BY THE FIRE" },
    { "OUTHOUSE", 5, 0, 0, IC_DROP, "A LITTLE PRIVACY" },
    { "SPEAR", 2, 1, 0, IC_BOLT, "FISH FASTER, CATCH MORE" },
    { "SHELTER", 12, 2, 8, IC_HOUSE, "A WOODEN HUT WITH A BED" },
    { "RAFT", 10, 0, 6, IC_CAR, "SAIL OUT TO THE ISLET" },
};
static int raft_made(void) { return world_count(CH_RAFT) > 0; }
int isl_can_make(int i) {
    const Craft *c = &isl_crafts[i];
    if (i == 3 && isl_spear) return 0;
    if (i == 5 && raft_made()) return 0;
    return isl_wood >= c->wood && isl_stone >= c->stone && isl_leaves >= c->leaves;
}
static int wild(char c) { return c == 'g' || c == '3' || c == '6' || c == CH_BUSH; }   // (trees and bushes: cleared to build)
static int land_ok(int x, int y) {   // dry ground, bare or with only trees and bushes on it, and not the path
    if (x <= 1 || y <= 1 || x >= MW - 2 || y >= MH - 2) return 0;
    u8 f = floorMap[y][x]; if (f == FL_WATER || f == FL_PAVING || f == FL_SOIL || f == 4) return 0;   // (not the sea, a path or the jetty)
    return lifeMap[y][x] == '.' || wild(lifeMap[y][x]);
}
static int someone_on(int x0, int y0, int x1, int y1) { for (int i = 0; i < nsims; i++) { int x = sim_tile_x(&sims[i]), y = sim_tile_y(&sims[i]); if (!sims[i].away && x >= x0 && x <= x1 && y >= y0 && y <= y1) return 1; } return 0; }
static int spot_near(int cx, int cy, int w, int h, int *ox, int *oy) {   // a w x h patch of dry land near (cx, cy) with a tile of room round it: the fewest trees to clear, and close
    int best = 1 << 30;
    for (int y0 = cy - 14; y0 <= cy + 14; y0++) for (int x0 = cx - 14; x0 <= cx + 14; x0++) {
        int cost = ABS(x0 + w / 2 - cx) + ABS(y0 + h / 2 - cy), ok = 1;
        for (int y = y0 - 1; y <= y0 + h && ok; y++) for (int x = x0 - 1; x <= x0 + w && ok; x++) { if (!land_ok(x, y)) ok = 0; else if (lifeMap[y][x] != '.') cost += 5; }
        if (ok && cost < best && !someone_on(x0, y0, x0 + w - 1, y0 + h - 1)) { best = cost; *ox = x0; *oy = y0; }
    }
    return best < (1 << 30);
}
static void clear_ground(int x0, int y0, int w, int h) {   // the trees and bushes in the way come down (and the wood is yours)
    int n = 0;
    for (int y = y0 - 1; y <= y0 + h; y++) for (int x = x0 - 1; x <= x0 + w; x++) if (wild(lifeMap[y][x])) { lifeMap[y][x] = '.'; n++; }
    if (n) { char t[32]; isl_wood += n; snprintf(t, sizeof t, "CLEARED THE GROUND: +%d WOOD", n); sim_note(t); }
}
static int shore_free(int *ox, int *oy) {   // the raft's beach: the dock tile, or the nearest free sand by the water
    for (int r = 0; r <= 5; r++) for (int dy = -r; dy <= r; dy++) for (int dx = -r; dx <= r; dx++) {
        if (ABS(dx) != r && ABS(dy) != r) continue;
        int x = isl_dock_x + dx, y = isl_dock_y + dy, wet = 0;
        if (x < 2 || y < 2 || x >= MW - 2 || y >= MH - 2 || lifeMap[y][x] != '.' || floorMap[y][x] != FL_SAND) continue;
        for (int k = 0; k < 4; k++) if (floorMap[y + (k == 2) - (k == 3)][x + (k == 0) - (k == 1)] == FL_WATER) wet = 1;
        if (wet || !r) { *ox = x; *oy = y; return 1; }
    }
    return 0;
}
static int wall_named(const char *nm) { for (int i = 0; i < NWALLS; i++) if (!strcmp(world_wall_name(i), nm)) return i; return 20; }
int isl_make(int i) {
    if (!isl_can_make(i)) return 0;
    const Craft *c = &isl_crafts[i];
    int x, y, ok = 1;
    switch (i) {
        case 0: ok = spot_near(isl_fire_x, isl_fire_y, 1, 1, &x, &y); if (ok) { clear_ground(x, y, 1, 1); lifeMap[y][x] = 'z'; } break;
        case 1: ok = spot_near(isl_fire_x, isl_fire_y, 1, 1, &x, &y); if (ok) { clear_ground(x, y, 1, 1); lifeMap[y][x] = '%'; } break;
        case 2: ok = spot_near(isl_fire_x, isl_fire_y, 1, 1, &x, &y); if (ok) { clear_ground(x, y, 1, 1); lifeMap[y][x] = '&'; } break;
        case 3: isl_spear = 1; break;
        case 4: ok = spot_near(isl_fire_x, isl_fire_y, 5, 5, &x, &y);   // a 5 x 5 hut of planks: a door on its south side, a tent to sleep in
                if (ok) { int wp = wall_named("WOOD PANEL"); clear_ground(x, y, 5, 5);
                    for (int j = y; j < y + 5; j++) for (int k = x; k < x + 5; k++) {
                        int edge = j == y || j == y + 4 || k == x || k == x + 4;
                        *world_cell(0, k, j) = edge ? 'W' : '.'; *world_wall_cell(0, k, j) = (u8)wp; *world_floor_cell(0, k, j) = 4; }   // (WOOD PLANKS)
                    *world_cell(0, x + 2, y + 4) = 'D'; *world_cell(0, x, y + 2) = 'E'; *world_cell(0, x + 4, y + 2) = 'E';
                    *world_cell(0, x + 1, y + 1) = 'z'; *world_cell(0, x + 3, y + 1) = '%'; }
                break;
        case 5: ok = shore_free(&x, &y); if (ok) { isl_dock_x = (u8)x; isl_dock_y = (u8)y; lifeMap[y][x] = CH_RAFT; } break;
    }
    if (!ok) return -1;
    isl_wood -= c->wood; isl_stone -= c->stone; isl_leaves -= c->leaves;
    world_touched();
    char t[40]; snprintf(t, sizeof t, "MADE: %s!", c->name); sim_note(t);
    if (i == 5) sim_note("THE RAFT IS ON THE SHORE");
    audio_voice(VOICE_FINISHED);
    return 1;
}

void isl_save(void) { sv_u16(isl_wood); sv_u16(isl_stone); sv_u16(isl_leaves); sv_u16(isl_food); sv_u8(isl_spear); sv_u16(isl_shells); sv_u16(isl_secrets); }
void isl_load(void) { isl_wood = ld_u16(); isl_stone = ld_u16(); isl_leaves = ld_u16(); isl_food = ld_u16(); isl_spear = ld_u8(); isl_shells = ld_u16(); isl_secrets = (u16)ld_u16(); }
