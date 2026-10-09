// world.c - the lot: floors, walls (with the Sims cut-away), items, and BORE's default map.
#include "world.h"
#include "gfx.h"
#include "atlas.h"
#include "items.h"
#include "tex.h"
#include "wallart.h"

char lifeMap[MH][MW + 1];
static int ao_dirty = 1;
int dbg_noao, dbg_nograd;   // the floor shading needs working out again (world_touched)
u8 floorMap[MH][MW], wallMap[MH][MW];

static TexRef fl_tex[NFLOORS];       // 16x16: the 8x8 pattern twice over
static TexRef fl_tex4[NFLOORS];      // 32x32: four tiles' worth, the checker shade of the odd tiles baked in (2x2 blocks of floor draw as one quad)
#define PN_NOISE "01000200","00020010","20001000","00100020","01000100","00200001","10002000","00010200"
// the outdoor floors BORE 2 adds: 16x16, 4 colours, hand drawn here
static const Tex outTex[6] = {
 {"GRASS",{RGB(9,18,6),RGB(12,22,8),RGB(7,15,5),RGB(14,24,10)},{"01010210","10301001","01010100","20101031","01010210","10201001","31010100","00101010"}},
 {"PAVING",{RGB(17,16,15),RGB(20,19,18),RGB(13,12,12),RGB(22,21,19)},{"00000003","01110111","01110111","01110111","00000000","11101110","11101110","11101120"}},
 {"SOIL",{RGB(12,8,5),RGB(14,10,6),RGB(9,6,4),RGB(16,11,7)},{PN_NOISE}},
 {"SAND",{RGB(28,24,14),RGB(30,26,17),RGB(26,22,12),RGB(31,28,20)},{PN_NOISE}},
 {"WATER",{RGB(6,14,26),RGB(8,18,29),RGB(12,22,31),RGB(20,27,31)},{"00100000","01110020","00100000","00000100","20001110","00000100","01000000","11100001"}},
 {"ROCK",{RGB(15,13,12),RGB(18,16,14),RGB(12,11,10),RGB(21,19,16)},{"00010000","01000200","00000001","00020000","10000100","00000000","00100010","02000000"}},
};
static TexRef wp_tex[NWALLS];        // 8x24
static u16 wp_top[NWALLS];
static u8 fl_odd[NFLOORS] = { 14, 15, 15, 16, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 15, 16, 16, 16, 15 };
static u16 tb[32 * 32];
static TexRef glass_tex;

const char *world_wall_name(int wp) { return wp < NWP ? look_wp_tex(wp)->nm : wxName[(wp - NWP) % NWX]; }
static const Tex *floor_tex(int i) { return i < NFL ? look_fl_tex(i) : &outTex[(i - NFL) % 6]; }
const char *world_floor_name(int fl) { return floor_tex(fl % NFLOORS)->nm; }

static u16 fl_soft[NFLOORS][4];   // the floor colours, calmed: a GBA pattern blown up across a 3D floor shouts, so each colour leans toward the tile's average
static void soften_floors(void) {
    for (int i = 0; i < NFLOORS; i++) {
        const Tex *t = floor_tex(i);
        int r = 0, g = 0, b = 0, n = 0;
        for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) { u16 c = t->c[t->p[y][x] - '0']; r += C_R(c); g += C_G(c); b += C_B(c); n++; }
        r /= n; g /= n; b /= n;
        for (int k = 0; k < 4; k++) { u16 c = t->c[k]; fl_soft[i][k] = RGB((C_R(c) * 9 + r * 7) / 16, (C_G(c) * 9 + g * 7) / 16, (C_B(c) * 9 + b * 7) / 16); }
    }
}
void world_init(void) {
    soften_floors();
    for (int i = 0; i < NFLOORS; i++) {
        const Tex *t = floor_tex(i);
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { u16 c = fl_soft[i][t->p[y & 7][x & 7] - '0']; tb[y * 16 + x] = c ? c : 0x8000; }
        fl_tex[i] = atlas_add(16, 16, tb);
        for (int y = 0; y < 32; y++) for (int x = 0; x < 32; x++) {
            u16 c = fl_soft[i][t->p[y & 7][x & 7] - '0'];
            int odd = ((x >> 3) + (y >> 3)) & 1, k = odd ? fl_odd[i] : 16;
            u16 o = RGB(C_R(c) * k / 16, C_G(c) * k / 16, C_B(c) * k / 16);
            tb[y * 32 + x] = o ? o : 0x8000;
        }
        fl_tex4[i] = atlas_add(32, 32, tb);
    }
    for (int i = 0; i < NWP; i++) {
        const Tex *t = look_wp_tex(i);
        for (int y = 0; y < 24; y++) for (int x = 0; x < 8; x++) { u16 c = t->c[t->p[y & 7][x] - '0']; tb[y * 8 + x] = c ? c : 0x8000; }
        wp_tex[i] = atlas_add(8, 24, tb);
        wp_top[i] = look_wp_avg(i);
    }
    for (int i = 0; i < NWX; i++) {   // the KHLVH set is pre-shaded 12/16 for the left face: lift it back
        for (int y = 0; y < 24; y++) for (int x = 0; x < 8; x++) {
            u16 c = wxTex[i][0][x][y];
            int r = MIN(31, C_R(c) * 16 / 12), g = MIN(31, C_G(c) * 16 / 12), b = MIN(31, C_B(c) * 16 / 12);
            u16 o = RGB(r, g, b); tb[y * 8 + x] = o ? o : 0x8000;
        }
        wp_tex[NWP + i] = atlas_add(8, 24, tb);
        wp_top[NWP + i] = wxAvg[i];
    }
    for (int i = 0; i < 64; i++) tb[i] = RGB(18 + (i & 1), 24, 30);
    glass_tex = atlas_add(8, 8, tb);
}

// ---- BORE's map builders ----
static void gBox(int x0, int y0, int x1, int y1, int fl) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) floorMap[y][x] = (u8)fl; }
static void gRoom(int x0, int y0, int x1, int y1, int fl, int wp) {
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { floorMap[y][x] = (u8)fl;
        if (x == x0 || x == x1 || y == y0 || y == y1) { lifeMap[y][x] = 'W'; wallMap[y][x] = (u8)wp; } }
}
static void gLine(int x0, int y0, int x1, int y1, char c, int wp) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { lifeMap[y][x] = c; wallMap[y][x] = (u8)wp; } }
static void gPut(int x, int y, char c) { lifeMap[y][x] = c; }
static void gFree(int x, int y, char c) { if (x >= 0 && y >= 0 && x < MW && y < MH && lifeMap[y][x] == '.') lifeMap[y][x] = c; }

void world_gen(void) {
    ao_dirty = 1;   // THE MAN BASE, as the GBA game lays it out (mapGen)
    for (int y = 0; y < MH; y++) { for (int x = 0; x < MW; x++) { lifeMap[y][x] = '.'; floorMap[y][x] = FL_GRASS; wallMap[y][x] = 0; } lifeMap[y][MW] = 0; }
    gLine(0, 0, MW - 1, 0, 'w', 13); gLine(0, MH - 1, MW - 1, MH - 1, 'w', 13); gLine(0, 0, 0, MH - 1, 'w', 13); gLine(MW - 1, 0, MW - 1, MH - 1, 'w', 13);
    gBox(9, 18, 9, 21, FL_PAVING); gBox(9, 20, 21, 20, FL_PAVING); gBox(18, 10, 21, 10, FL_PAVING); gBox(18, 13, 21, 13, FL_PAVING);   // paths from the doors
    gBox(1, 18, 8, 19, FL_SOIL); gBox(19, 2, 21, 8, FL_SOIL);                                                                  // flower beds
    gFree(3, 19, 'p'); gFree(6, 19, 'p'); gFree(20, 4, 'p'); gFree(20, 7, 'p'); gFree(2, 18, 'Z'); gFree(7, 18, 'Z');
    // HOUSE: peach wallpaper, beige carpet, lino kitchen, pink-tile bathroom
    gRoom(2, 2, 17, 17, 1, NWP + 42); gBox(11, 11, 16, 16, 3);
    gRoom(2, 2, 9, 9, 5, NWP + 64); gPut(6, 9, 'D'); gPut(3, 3, 'T');
    gPut(9, 17, 'D'); gPut(17, 13, 'D');
    gPut(16, 11, 'F'); gPut(16, 12, 'F'); gPut(12, 4, '#'); gPut(13, 4, '#'); gPut(12, 5, '#'); gPut(13, 5, '#');
    gPut(5, 12, 'P'); gPut(7, 14, 'B');
    gPut(8, 3, 'H'); gPut(4, 16, 'S'); gPut(3, 11, 'C');
    gPut(3, 10, 'V'); gPut(5, 10, 'G'); gPut(3, 13, 'U'); gPut(6, 12, 'Q'); gPut(8, 12, 'I');
    gFree(9, 12, 'R'); gFree(9, 14, 'A');
    gFree(12, 14, 'v'); gFree(14, 16, 'h'); gFree(14, 15, 't'); gFree(15, 16, 'h'); gFree(11, 16, 'p'); gFree(16, 14, 'y'); gFree(16, 15, 'c');   // BORE 2: the home pack moves in
    // FACTORY
    gRoom(22, 2, 37, 19, 8, NWP + 53); gBox(23, 10, 36, 11, 10); gBox(23, 14, 27, 18, 9); gBox(30, 3, 36, 8, 12);
    gPut(29, 19, 'D'); gPut(22, 10, 'D'); gPut(37, 10, 'D');
    gLine(24, 13, 29, 13, '=', 8);
    gPut(25, 4, '#'); gPut(26, 4, '#'); gPut(25, 5, '#'); gPut(26, 5, '#'); gPut(31, 15, '#'); gPut(32, 15, '#'); gPut(31, 16, '#'); gPut(32, 16, '#'); gPut(34, 5, '#'); gPut(34, 6, '#');
    gPut(33, 17, 'x');
    // RAIL PARK
    gBox(2, 22, 37, 37, 7); gBox(2, 28, 37, 29, 12);   // the rail park stays concrete
    gLine(3, 24, 10, 24, '=', 0); gLine(3, 31, 10, 31, '=', 0); gLine(3, 35, 10, 35, '=', 0);
    gLine(29, 24, 36, 24, '=', 0); gLine(29, 31, 36, 31, '=', 0); gLine(29, 35, 36, 35, '=', 0);
    gLine(16, 28, 23, 28, '=', 0); gLine(16, 33, 23, 33, '=', 0);
    gPut(5, 26, '#'); gPut(6, 26, '#'); gPut(5, 27, '#'); gPut(6, 27, '#'); gPut(8, 33, '#'); gPut(9, 33, '#'); gPut(8, 34, '#'); gPut(9, 34, '#');
    gPut(31, 26, '#'); gPut(32, 26, '#'); gPut(31, 27, '#'); gPut(32, 27, '#'); gPut(34, 33, '#'); gPut(35, 33, '#'); gPut(34, 34, '#'); gPut(35, 34, '#');
    gPut(14, 24, '#'); gPut(15, 24, '#'); gPut(14, 25, '#'); gPut(15, 25, '#'); gPut(24, 25, '#'); gPut(25, 25, '#'); gPut(24, 26, '#'); gPut(25, 26, '#');
    gLine(12, 21, 27, 21, 'w', 13);
    gPut(16, 30, '4'); gPut(21, 30, '2');
    gPut(17, 22, '5'); gPut(18, 22, '5');
    gPut(13, 33, 'L'); gPut(14, 33, 'L'); gPut(15, 33, 'L'); gPut(25, 33, 'N'); gPut(26, 33, 'N');
    gFree(18, 30, 'X'); gFree(19, 30, 'X'); gFree(18, 31, '9'); gFree(19, 31, '9');
    gFree(13, 31, 'O'); gFree(13, 32, 'O'); gFree(14, 31, 'O'); gFree(26, 30, 'O'); gFree(26, 31, 'O');
    for (int x = 16; x <= 21; x++) gFree(x, 35, 'J');
    gFree(13, 22, 'Z'); gFree(14, 22, 'Z'); gFree(25, 22, 'Z'); gFree(26, 22, 'Z');
    gFree(21, 24, 'K'); gFree(13, 23, 'Y'); gFree(26, 23, 'Y');
    for (int x = 17; x <= 20; x++) gFree(x, 26, 'M');
    gFree(20, 37, 'a'); gFree(4, 36, 'k');
    gBox(9, 22, 9, 23, FL_PAVING); gPut(10, 23, '+');                                   // the bus stop at the end of the path
}

static char bk_map[MH][MW + 1]; static u8 bk_fl[MH][MW], bk_wp[MH][MW]; static int bk_on;
void world_backup(void) { if (bk_on) return; memcpy(bk_map, lifeMap, sizeof bk_map); memcpy(bk_fl, floorMap, sizeof bk_fl); memcpy(bk_wp, wallMap, sizeof bk_wp); bk_on = 1; }
void world_restore(void) {
    ao_dirty = 1; if (!bk_on) return; memcpy(lifeMap, bk_map, sizeof bk_map); memcpy(floorMap, bk_fl, sizeof bk_fl); memcpy(wallMap, bk_wp, sizeof bk_wp); bk_on = 0; }
void world_gen_studio(void) {
    ao_dirty = 1;   // a 9x9 parlour: the creator's stage
    for (int y = 0; y < MH; y++) { for (int x = 0; x < MW; x++) { lifeMap[y][x] = '.'; floorMap[y][x] = 4; wallMap[y][x] = 0; } lifeMap[y][MW] = 0; }
    gRoom(0, 0, 8, 8, 1, NWP + 64);                                            // a light, airy room: beige carpet, pale walls
    gPut(2, 8, 'E'); gPut(6, 8, 'E'); gPut(0, 3, 'E'); gPut(8, 5, 'E');         // windows on the wall behind the Bore and the sides
    gPut(1, 1, 'i'); gPut(7, 1, 'l'); gPut(1, 7, 'p'); gPut(7, 7, 'p'); gPut(4, 1, 'b');
    gPut(4, 4, 'r'); gPut(4, 5, 'P');
    gPut(6, 1, 'V'); gPut(2, 1, 'c'); gPut(7, 4, 'C'); gPut(1, 4, 'l');
}
// ---- the zones ----
int world_zone = ZONE_HOME;
static const char *const zoneNm[ZONE_N] = { "THE MAN BASE", "CASTAWAY ISLAND", "STRANGE TOWN" };
const char *world_zone_name(int z) { return zoneNm[z % ZONE_N]; }
typedef struct { char map[MH][MW + 1]; u8 fl[MH][MW]; u8 wp[MH][MW]; u8 made; } ZoneSave;
static ZoneSave zones[ZONE_N];
static void gCircle(int cx, int cy, int r, int fl) { for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r) floorMap[y][x] = (u8)fl; }
void world_gen_island(void) {
    ao_dirty = 1;   // Castaway: a sand island in the sea, a camp, palms
    for (int y = 0; y < MH; y++) { for (int x = 0; x < MW; x++) { lifeMap[y][x] = '.'; floorMap[y][x] = FL_WATER; wallMap[y][x] = 0; } lifeMap[y][MW] = 0; }
    gCircle(20, 20, 14, FL_SAND); gCircle(13, 15, 7, FL_SAND); gCircle(27, 26, 6, FL_SAND);
    gCircle(20, 19, 7, FL_GRASS); gCircle(21, 20, 3, FL_SOIL);
    gBox(19, 27, 20, 33, FL_PAVING);                                        // a jetty of planks out to the boat
    gPut(20, 34, '@'); gPut(20, 35, '.'); floorMap[34][20] = FL_WATER;
    gPut(20, 20, 's'); gPut(18, 18, 'z'); gPut(23, 17, '&'); gPut(16, 22, '0'); gPut(22, 22, '%'); gPut(19, 22, '%');
    gPut(24, 20, 'N'); gPut(13, 13, 'p');
    static const u8 palms[][2] = { {10,16},{12,10},{15,9},{27,12},{30,17},{31,23},{28,30},{23,31},{14,24},{9,21},{25,8},{33,20} };
    for (unsigned i = 0; i < sizeof palms / sizeof palms[0]; i++) if (floorMap[palms[i][1]][palms[i][0]] != FL_WATER) gFree(palms[i][0], palms[i][1], 'g');
    gFree(26, 15, '*'); gFree(11, 19, '*'); gFree(29, 27, '*'); gFree(17, 26, '*');
    gPut(20, 24, 'P'); gFree(21, 26, 'a');
}
void world_gen_town(void) {   // Strange Town: a desert main street with a diner, a trailer, the lab and work
    for (int y = 0; y < MH; y++) { for (int x = 0; x < MW; x++) { lifeMap[y][x] = '.'; floorMap[y][x] = FL_SAND; wallMap[y][x] = 0; } lifeMap[y][MW] = 0; }
    gLine(0, 0, MW - 1, 0, 'w', 13); gLine(0, MH - 1, MW - 1, MH - 1, 'w', 13); gLine(0, 0, 0, MH - 1, 'w', 13); gLine(MW - 1, 0, MW - 1, MH - 1, 'w', 13);
    gBox(1, 18, 38, 21, FL_PAVING);                                         // main street
    gBox(5, 5, 12, 9, FL_ROCK); gBox(30, 28, 37, 35, FL_ROCK);
    // the DINER: lino floor, a counter, tables and chairs, a fridge, a toilet
    gRoom(4, 10, 15, 17, 3, NWP + 53); gPut(9, 17, 'D'); gPut(10, 17, 'D');
    gPut(5, 11, 'F'); gPut(6, 11, 'y'); gPut(7, 11, 'y'); gPut(8, 11, 'c'); gPut(14, 11, 'T'); gPut(13, 11, 'R');
    gPut(6, 14, 't'); gPut(5, 14, 'h'); gPut(7, 14, 'h'); gPut(11, 14, 't'); gPut(10, 14, 'h'); gPut(12, 14, 'h'); gPut(14, 15, 'q');
    // the TRAILER: a tiny home across the street
    gRoom(20, 23, 27, 27, 1, NWP + 57); gPut(23, 23, 'D');
    gPut(21, 26, 'S'); gPut(26, 26, 'T'); gPut(26, 24, 'H'); gPut(21, 24, 'C'); gPut(24, 26, 'v');
    // the LAB (work): hazard floor, crates, the work marker, a security camera
    gRoom(29, 8, 38, 16, 10, NWP + 61); gPut(33, 16, 'D'); gBox(30, 9, 37, 15, 12);
    gPut(31, 10, '#'); gPut(32, 10, '#'); gPut(36, 10, 'Q'); gPut(34, 12, 'x'); gPut(30, 14, 'd'); gPut(31, 14, 'h'); gPut(37, 14, 'n');
    // out and about: cacti, rocks, a bus stop, benches, a phone
    static const u8 cacti[][2] = { {2,3},{16,4},{22,6},{26,3},{3,25},{8,30},{14,33},{24,34},{37,24},{18,28},{35,4} };
    for (unsigned i = 0; i < sizeof cacti / sizeof cacti[0]; i++) gFree(cacti[i][0], cacti[i][1], 'u');
    gFree(7, 7, '*'); gFree(10, 6, '*'); gFree(33, 31, '*'); gFree(35, 33, '*'); gFree(31, 30, '*');
    gPut(17, 22, '+'); gFree(15, 22, 'N'); gFree(19, 22, 'I'); gFree(28, 22, 'Z'); gFree(30, 22, 'Z');
    gPut(18, 24, 'P'); gFree(3, 22, 'a'); gFree(36, 22, 'k');
}
void world_switch(int z) {
    ao_dirty = 1;
    if (z == world_zone) return;
    ZoneSave *cur = &zones[world_zone];
    memcpy(cur->map, lifeMap, sizeof cur->map); memcpy(cur->fl, floorMap, sizeof cur->fl); memcpy(cur->wp, wallMap, sizeof cur->wp); cur->made = 1;
    world_zone = z;
    ZoneSave *nx = &zones[z];
    if (nx->made) { memcpy(lifeMap, nx->map, sizeof nx->map); memcpy(floorMap, nx->fl, sizeof nx->fl); memcpy(wallMap, nx->wp, sizeof nx->wp); }
    else if (z == ZONE_ISLAND) world_gen_island();
    else if (z == ZONE_TOWN) world_gen_town();
    else world_gen();
}
void world_reset_all(void) {
    ao_dirty = 1; for (int i = 0; i < ZONE_N; i++) zones[i].made = 0; world_zone = ZONE_HOME; world_gen(); }
char world_at(int mx, int my) { if (mx < 0 || my < 0 || mx >= MW || my >= MH) return 'W'; return lifeMap[my][mx]; }
static int is_wall(char c) { return c == 'W' || c == 'w' || c == 'E' || c == 'e' || c == 'f'; }
int world_walkable(int mx, int my) {
    char c = world_at(mx, my);
    if (is_wall(c)) return 0;
    if (mx >= 0 && my >= 0 && mx < MW && my < MH && floorMap[my][mx] == FL_WATER) return 0;
    if (c == '.' || c == 'D' || c == 'P' || c == 'B') return 1;
    return !items_blocking(c);
}
int world_floor_y(int mx, int my) { (void)mx; (void)my; return 0; }
void world_find(char ch, int *mx, int *my) {
    for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) if (lifeMap[y][x] == ch) { *mx = x; *my = y; return; }
    *mx = -1; *my = -1;
}
int world_count(char ch) { int n = 0; for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) if (lifeMap[y][x] == ch) n++; return n; }
static int item_open(int x, int y) { if (x < 0 || y < 0 || x >= MW || y >= MH) return 0; char c = lifeMap[y][x]; return c == '.' || c == 'D' || c == 'B' || c == 'P' || c == 'r'; }
int world_item_facing(int x, int y) {
    static const signed char dx[4] = { 0, 1, 0, -1 }, dy[4] = { 1, 0, -1, 0 };
    for (int d = 0; d < 4; d++) if (item_open(x + dx[d], y + dy[d]) && !item_open(x + dx[(d + 2) & 3], y + dy[(d + 2) & 3])) return d;
    for (int d = 0; d < 4; d++) if (item_open(x + dx[d], y + dy[d])) return d;
    return 0;
}
int world_along(int x, int y, char ch) {
    int ax = (x > 0 && lifeMap[y][x - 1] == ch) + (x < MW - 1 && lifeMap[y][x + 1] == ch), ay = (y > 0 && lifeMap[y - 1][x] == ch) + (y < MH - 1 && lifeMap[y + 1][x] == ch);
    return !(ay > 0 && ax == 0);
}
int world_room_score(int mx, int my) {
    int kinds = 0;
    for (int y = my - 2; y <= my + 2; y++) for (int x = mx - 2; x <= mx + 2; x++) {
        char c = world_at(x, y);
        if (c == 'F') kinds |= 1; if (c == 'T') kinds |= 2; if (c == 'S') kinds |= 4; if (c == 'H') kinds |= 8; if (c == 'C' || c == 'U' || c == 'h') kinds |= 16;
        if (c == 'v' || c == 'R' || c == 'A') kinds |= 32; if (c == 'p' || c == 'l' || c == 'r' || c == 'q' || c == 'V') kinds |= 64;
    }
    int n = 0; for (int i = 0; i < 7; i++) if (kinds >> i & 1) n++;
    return MIN(100, n * 20);
}

// ---- drawing ----
static int frame_t;
// the floor's contact shading: how many walls stand round each floor corner, worked out when the map changes
static u8 ao[MH + 1][MW + 1];
void world_touched(void) { ao_dirty = 1; }
static void ao_update(void) {
    for (int Y = 0; Y <= MH; Y++) for (int X = 0; X <= MW; X++) {
        int n = 0;
        if (is_wall(world_at(X - 1, Y - 1))) n++; if (is_wall(world_at(X, Y - 1))) n++;
        if (is_wall(world_at(X - 1, Y))) n++;     if (is_wall(world_at(X, Y))) n++;
        ao[Y][X] = (u8)n;
    }
    ao_dirty = 0;
}
static inline int ao_corner(int X, int Y, int L) { int n = ao[Y][X]; return n ? L * (100 - 16 * n) / 100 : L; }
static inline int near_wall(int mx, int my) { return ao[my][mx] | ao[my][mx + 1] | ao[my + 1][mx] | ao[my + 1][mx + 1]; }
static void floor_tile(int mx, int my) {
    int fl = floorMap[my][mx] % NFLOORS;
    const TexRef *t = &fl_tex[fl];
    int x0 = mx * TILE, x1 = x0 + TILE, zN = -my * TILE, zS = zN - TILE;
    SVECTOR a, b, c, d;
    int y = (fl == FL_WATER) ? 14 + (isin((frame_t * 40 + mx * 300 + my * 500) & 4095) >> 10) : 0;   // water sits a little low and heaves
    setVector(&a, x0, y, zN); setVector(&b, x1, y, zN); setVector(&c, x0, y, zS); setVector(&d, x1, y, zS);
    int L = gfx_light(0, -ONE, 0);
    if ((mx + my) & 1) L = L * fl_odd[fl] / 16;
    L = CLAMP(L, 40, 200);
    int u = t->u, v = t->v;
    if (fl == FL_WATER) { u += (frame_t >> 3) & 7; v += (frame_t >> 4) & 7; }   // (the 16x16 texture holds the 8x8 pattern twice: it tiles)
    if (!dbg_noao && !is_wall(lifeMap[my][mx]) && near_wall(mx, my)) {   // next to a wall: the corners by it darken (contact shadow)
        int L0 = ao_corner(mx, my, L), L1 = ao_corner(mx + 1, my, L), L2 = ao_corner(mx, my + 1, L), L3 = ao_corner(mx + 1, my + 1, L);
        gfx_quad_gt(&a, &b, &c, &d, t->tpage, 0, u, v, u + 7, v, u, v + 7, u + 7, v + 7,
                    (u32)L0 * 0x010101u, (u32)L1 * 0x010101u, (u32)L2 * 0x010101u, (u32)L3 * 0x010101u, 0);
        return;
    }
    gfx_quad_ft(&a, &b, &c, &d, t->tpage, 0, u, v, u + 7, v, u, v + 7, u + 7, v + 7, L, 0, 0);
}

static void floor_block(int mx, int my) {   // the 2x2 tiles at (mx,my) (even), all the same floor: one quad
    int fl = floorMap[my][mx] % NFLOORS;
    const TexRef *t = &fl_tex4[fl];
    int x0 = mx * TILE, x1 = x0 + 2 * TILE, zN = -my * TILE, zS = zN - 2 * TILE;
    SVECTOR a, b, c, d;
    setVector(&a, x0, 0, zN); setVector(&b, x1, 0, zN); setVector(&c, x0, 0, zS); setVector(&d, x1, 0, zS);
    int L = CLAMP(gfx_light(0, -ONE, 0), 40, 200);
    if (!dbg_noao && (near_wall(mx, my) | near_wall(mx + 1, my) | near_wall(mx, my + 1) | near_wall(mx + 1, my + 1))) {
        int L0 = ao_corner(mx, my, L), L1 = ao_corner(mx + 2, my, L), L2 = ao_corner(mx, my + 2, L), L3 = ao_corner(mx + 2, my + 2, L);
        gfx_quad_gt(&a, &b, &c, &d, t->tpage, 0, t->u, t->v, t->u + 31, t->v, t->u, t->v + 31, t->u + 31, t->v + 31,
                    (u32)L0 * 0x010101u, (u32)L1 * 0x010101u, (u32)L2 * 0x010101u, (u32)L3 * 0x010101u, 0);
        return;
    }
    gfx_quad_ft(&a, &b, &c, &d, t->tpage, 0, t->u, t->v, t->u + 31, t->v, t->u, t->v + 31, t->u + 31, t->v + 31, L, 0, 0);
}

// ---- thin walls: a wall tile is a slab 40 units thick running along the line its neighbours make, like a Sims wall ----
#define WT 20
static int wall_or_door(int mx, int my) { char c = world_at(mx, my); return is_wall(c) || c == 'D'; }
// a vertical face from (ax,az) to (bx,bz), from yTop (negative) down to yBot, textured with the wallpaper (u0..u1 of the tile's 8 columns)
static void wface(const TexRef *t, int ax, int az, int bx, int bz, int yTop, int yBot, int h, int u0, int u1, int nx, int nz, int cut) {
    SVECTOR a, b, c, d;
    setVector(&a, ax, yTop, az); setVector(&b, bx, yTop, bz); setVector(&c, ax, yBot, az); setVector(&d, bx, yBot, bz);
    // the 24-row texture spans a full wall; a cut wall shows the bottom rows
    int vb = t->v + 23, vspan = cut ? 23 * h / WALL_FULL : 23; if (vspan < 2) vspan = 2;
    int v0 = vb - (-(yTop) * vspan) / h, v1 = vb - (-(yBot) * vspan) / h;
    v0 = CLAMP(v0, t->v, vb); v1 = CLAMP(v1, t->v, vb);
    int L = CLAMP(gfx_light(nx, 0, nz), 40, 200);
    int Lt = L + (L * (-yTop)) / (h * 6), Lb = L - (L * (-yBot - 0)) / (h * 3) - L * (h + yBot) / (h * 8);   // lighter up top, darker at the foot
    Lt = CLAMP(Lt, 30, 230); Lb = CLAMP(Lb, 30, 230);
    if (dbg_nograd) { gfx_quad_ft(&a, &b, &c, &d, t->tpage, 0, t->u + u0, v0, t->u + u1, v0, t->u + u0, v1, t->u + u1, v1, L, 1, 0); return; }
    gfx_quad_gt(&a, &b, &c, &d, t->tpage, 0, t->u + u0, v0, t->u + u1, v0, t->u + u0, v1, t->u + u1, v1,
                (u32)Lt * 0x010101u, (u32)Lt * 0x010101u, (u32)Lb * 0x010101u, (u32)Lb * 0x010101u, 1);
}
static void wtop(u16 tc, int xa, int xb, int za, int zb, int y) {   // the top of a slab: za > zb
    SVECTOR a, b, c, d; int L = gfx_light(0, -ONE, 0);
    setVector(&a, xa, y, za); setVector(&b, xb, y, za); setVector(&c, xa, y, zb); setVector(&d, xb, y, zb);
    int r = (R8(tc) * L) >> 7, g = (G8(tc) * L) >> 7, bl = (B8(tc) * L) >> 7;
    gfx_quad_f(&a, &b, &c, &d, MIN(r, 255), MIN(g, 255), MIN(bl, 255), 1);
}
static void skirt(int ax, int az, int bx, int bz, int nx, int nz) {   // a skirting board along a face's foot
    SVECTOR a, b, c, d; u16 bc = RGB(26, 25, 23); int L = CLAMP(gfx_light(nx, 0, nz), 40, 200);
    int ox = nx ? (nx > 0 ? 3 : -3) : 0, oz = nz ? (nz > 0 ? 3 : -3) : 0;
    setVector(&a, ax + ox, -22, az + oz); setVector(&b, bx + ox, -22, bz + oz); setVector(&c, ax + ox, 0, az + oz); setVector(&d, bx + ox, 0, bz + oz);
    gfx_quad_f(&a, &b, &c, &d, (R8(bc) * L) >> 7, (G8(bc) * L) >> 7, (B8(bc) * L) >> 7, 1);
}
// a slab [xa..xb] x [zb..za] (za > zb) of height h: its four sides (those not flagged hidden) and its top
static void slab(const TexRef *t, u16 tc, int x0, int zN, int xa, int xb, int za, int zb, int h, int cut, int hideN, int hideS, int hideW, int hideE, char win) {
    int yT = -h;
    // faces that look away from the eye never show: skip them before the GTE
    if (cam.eye.vz <= za) hideN = 1; if (cam.eye.vz >= zb) hideS = 1; if (cam.eye.vx >= xa) hideW = 1; if (cam.eye.vx <= xb) hideE = 1;
    // window: the long faces are split round a pane
    int gy0 = -h * 3 / 4, gy1 = -h / 3; if (win == 'f') { gy0 = -h * 7 / 8; gy1 = -h * 5 / 8; }
    int ua = (xa - x0) * 8 / TILE, ub = (xb - x0) * 8 / TILE; if (ub <= ua) ub = ua + 1; ub = MIN(ub, 8); ua = MIN(ua, 7);
    int va = (zN - za) * 8 / TILE, vb2 = (zN - zb) * 8 / TILE; if (vb2 <= va) vb2 = va + 1; vb2 = MIN(vb2, 8); va = MIN(va, 7);
    #define LONGFACE(ax, az, bx, bz, nx, nz, uu0, uu1) \
        if (win && h == WALL_FULL) { wface(t, ax, az, bx, bz, gy1, 0, h, uu0, uu1, nx, nz, cut); wface(t, ax, az, bx, bz, yT, gy0, h, uu0, uu1, nx, nz, cut); } \
        else wface(t, ax, az, bx, bz, yT, 0, h, uu0, uu1, nx, nz, cut); \
        if (!cut && h == WALL_FULL) skirt(ax, az, bx, bz, nx, nz);
    if (!hideN) { LONGFACE(xb, za, xa, za, 0, ONE, ua, ub - 1) }          // north side (+z), seen from the north: left = xb
    if (!hideS) { LONGFACE(xa, zb, xb, zb, 0, -ONE, ua, ub - 1) }
    if (!hideW) { LONGFACE(xa, za, xa, zb, -ONE, 0, va, vb2 - 1) }
    if (!hideE) { LONGFACE(xb, zb, xb, za, ONE, 0, va, vb2 - 1) }
    #undef LONGFACE
    wtop(tc, xa, xb, za, zb, yT);
    if (win && h == WALL_FULL) {   // the pane, both sides of the slab
        SVECTOR a, b, c, d; int gl = win == 'e' ? 60 : 150;
        if (xb - xa > zb < za ? (xb - xa) > (za - zb) : 0) {}
        if ((xb - xa) >= (za - zb)) {   // runs along x: panes on the north and south faces
            setVector(&a, xb, gy0, za + 1); setVector(&b, xa, gy0, za + 1); setVector(&c, xb, gy1, za + 1); setVector(&d, xa, gy1, za + 1);
            gfx_quad_ft(&a, &b, &c, &d, glass_tex.tpage, 0, glass_tex.u, glass_tex.v, glass_tex.u + 7, glass_tex.v, glass_tex.u, glass_tex.v + 7, glass_tex.u + 7, glass_tex.v + 7, gl, 1, 1);
            setVector(&a, xa, gy0, zb - 1); setVector(&b, xb, gy0, zb - 1); setVector(&c, xa, gy1, zb - 1); setVector(&d, xb, gy1, zb - 1);
            gfx_quad_ft(&a, &b, &c, &d, glass_tex.tpage, 0, glass_tex.u, glass_tex.v, glass_tex.u + 7, glass_tex.v, glass_tex.u, glass_tex.v + 7, glass_tex.u + 7, glass_tex.v + 7, gl, 1, 1);
        } else {
            setVector(&a, xa - 1, gy0, za); setVector(&b, xa - 1, gy0, zb); setVector(&c, xa - 1, gy1, za); setVector(&d, xa - 1, gy1, zb);
            gfx_quad_ft(&a, &b, &c, &d, glass_tex.tpage, 0, glass_tex.u, glass_tex.v, glass_tex.u + 7, glass_tex.v, glass_tex.u, glass_tex.v + 7, glass_tex.u + 7, glass_tex.v + 7, gl, 1, 1);
            setVector(&a, xb + 1, gy0, zb); setVector(&b, xb + 1, gy0, za); setVector(&c, xb + 1, gy1, zb); setVector(&d, xb + 1, gy1, za);
            gfx_quad_ft(&a, &b, &c, &d, glass_tex.tpage, 0, glass_tex.u, glass_tex.v, glass_tex.u + 7, glass_tex.v, glass_tex.u, glass_tex.v + 7, glass_tex.u + 7, glass_tex.v + 7, gl, 1, 1);
        }
    }
}
static void wall_tile(int mx, int my, int h, int cut) {
    char c = lifeMap[my][mx];
    int wp = wallMap[my][mx] % NWALLS;
    const TexRef *t = &wp_tex[wp]; u16 tc = wp_top[wp];
    int x0 = mx * TILE, x1 = x0 + TILE, zN = -my * TILE, zS = zN - TILE;
    int cx = x0 + TILE / 2, cz = zN - TILE / 2;
    int wN = wall_or_door(mx, my - 1), wS = wall_or_door(mx, my + 1), wW = wall_or_door(mx - 1, my), wE = wall_or_door(mx + 1, my);
    char win = (c == 'E' || c == 'e' || c == 'f') ? c : 0;
    int runX = wW || wE, runZ = wN || wS;
    if (!runX && !runZ) { slab(t, tc, x0, zN, cx - WT, cx + WT, cz + WT, cz - WT, h, cut, 0, 0, 0, 0, 0); return; }   // a lone post
    if (runX) {   // the slab along x: from the west edge (or the centre) to the east edge (or the centre); end caps where the line stops
        int xa = wW ? x0 : cx - WT, xb = wE ? x1 : cx + WT;
        slab(t, tc, x0, zN, xa, xb, cz + WT, cz - WT, h, cut, 0, 0, wW, wE, win);
    }
    if (runZ) {
        int za = wN ? zN : cz + WT, zb = wS ? zS : cz - WT;
        // where an x-run already covers the centre, the z-run's faces inside it are hidden anyway: draw it whole
        slab(t, tc, x0, zN, cx - WT, cx + WT, za, zb, h, cut, wN, wS, 0, 0, runX ? 0 : win);
    }
}
// a doorway: the wall line passes through the tile with an opening; posts at the sides and a lintel over the top
static void door_tile(int mx, int my, int h) {
    int x0 = mx * TILE, x1 = x0 + TILE, zN = -my * TILE, zS = zN - TILE, cx = x0 + TILE / 2, cz = zN - TILE / 2;
    int alongX = is_wall(world_at(mx - 1, my)) || is_wall(world_at(mx + 1, my));
    int alongZ = is_wall(world_at(mx, my - 1)) || is_wall(world_at(mx, my + 1));
    if (!alongX && !alongZ) return;
    u16 fc = RGB(24, 20, 14); int L = CLAMP(gfx_light(0, 0, -ONE), 60, 200);
    int r = (R8(fc) * L) >> 7, g = (G8(fc) * L) >> 7, b = (B8(fc) * L) >> 7;
    SVECTOR a, bb, c, d;
    #define BOX(xa, xb, ya, yb, za, zb) { \
        setVector(&a, xb, ya, za); setVector(&bb, xa, ya, za); setVector(&c, xb, yb, za); setVector(&d, xa, yb, za); gfx_quad_f(&a, &bb, &c, &d, r, g, b, 1); \
        setVector(&a, xa, ya, zb); setVector(&bb, xb, ya, zb); setVector(&c, xa, yb, zb); setVector(&d, xb, yb, zb); gfx_quad_f(&a, &bb, &c, &d, r, g, b, 1); \
        setVector(&a, xa, ya, za); setVector(&bb, xa, ya, zb); setVector(&c, xa, yb, za); setVector(&d, xa, yb, zb); gfx_quad_f(&a, &bb, &c, &d, r * 3 / 4, g * 3 / 4, b * 3 / 4, 1); \
        setVector(&a, xb, ya, zb); setVector(&bb, xb, ya, za); setVector(&c, xb, yb, zb); setVector(&d, xb, yb, za); gfx_quad_f(&a, &bb, &c, &d, r * 3 / 4, g * 3 / 4, b * 3 / 4, 1); \
        setVector(&a, xa, ya, za); setVector(&bb, xb, ya, za); setVector(&c, xa, ya, zb); setVector(&d, xb, ya, zb); gfx_quad_f(&a, &bb, &c, &d, r * 5 / 4, g * 5 / 4, b * 5 / 4, 1); }
    int P = 14;
    if (alongX) { BOX(x0, x0 + P, -h, 0, cz + WT, cz - WT) BOX(x1 - P, x1, -h, 0, cz + WT, cz - WT) BOX(x0, x1, -h, -h + 56, cz + WT, cz - WT) }
    else        { BOX(cx - WT, cx + WT, -h, 0, zN, zN - P) BOX(cx - WT, cx + WT, -h, 0, zS + P, zS) BOX(cx - WT, cx + WT, -h, -h + 56, zN, zS) }
    #undef BOX
}

int world_dbg_walls;
void world_draw(int fx, int fy, int t, int cutaway) {
    gfx_set_identity(); world_dbg_walls = 0;
    frame_t = t;
    if (ao_dirty) ao_update();
    // the camera's forward on the ground (unit, 4096), to decide which walls stand between it and the focus
    int cdx = cam.at.vx - cam.eye.vx, cdz = cam.at.vz - cam.eye.vz;
    int len = SquareRoot0(cdx * cdx + cdz * cdz); if (len < 1) len = 1;
    int fdx = (cdx << 12) / len, fdz = (cdz << 12) / len;
    int focx = fx * TILE + TILE / 2, focz = -(fy * TILE + TILE / 2);
    int R = 12;
    int x0 = MAX(0, fx - R) & ~1, x1 = MIN(MW - 1, fx + R), y0 = MAX(0, fy - R) & ~1, y1 = MIN(MH - 1, fy + R);
    static u8 done[MH][MW];
    // 2x2 blocks of one floor draw as a single quad
    for (int my = y0; my <= y1; my += 2) for (int mx = x0; mx <= x1; mx += 2) {
        int ok = (mx + 1 < MW && my + 1 < MH);
        int f0 = floorMap[my][mx];
        if (ok) ok = f0 != FL_WATER && floorMap[my][mx + 1] == f0 && floorMap[my + 1][mx] == f0 && floorMap[my + 1][mx + 1] == f0;
        done[my][mx] = done[my][mx + 1] = done[my + 1][mx] = done[my + 1][mx + 1] = 0;
        if (!ok) continue;
        int wx = (mx + 1) * TILE, wz = -(my + 1) * TILE;
        VECTOR d, v; setVector(&d, wx - cam.eye.vx, -cam.eye.vy, wz - cam.eye.vz);
        ApplyMatrixLV(&cam.view, &d, &v);
        if (v.vz < -TILE * 2) continue;
        if (ABS(v.vx) > (v.vz * 3) / 4 + TILE * 3) continue;
        if (ABS(v.vy) > (v.vz * 5) / 8 + TILE * 4) continue;
        gfx_floor_pass = 1; floor_block(mx, my); gfx_floor_pass = 0;
        done[my][mx] = done[my][mx + 1] = done[my + 1][mx] = done[my + 1][mx + 1] = 1;
    }
    for (int my = y0; my <= y1; my++) for (int mx = x0; mx <= x1; mx++) {
        int wx = mx * TILE + TILE / 2, wz = -(my * TILE + TILE / 2);
        char c = lifeMap[my][mx];
        if (done[my][mx] && c == '.') continue;                      // floor already drawn, nothing on it
        // frustum: the tile centre in view space (rotation only, after taking the eye off)
        VECTOR d, v; setVector(&d, wx - cam.eye.vx, -WALL_FULL / 2 - cam.eye.vy, wz - cam.eye.vz);
        ApplyMatrixLV(&cam.view, &d, &v);
        if (v.vz < -TILE) continue;
        if (ABS(v.vx) > (v.vz * 3) / 4 + TILE * 2) continue;
        if (ABS(v.vy) > (v.vz * 5) / 8 + TILE * 3) continue;
        if (!done[my][mx]) { gfx_floor_pass = 1; floor_tile(mx, my); gfx_floor_pass = 0; }
        if (is_wall(c)) {
            int h = (c == 'w') ? WALL_LOW : WALL_FULL, cut = 0;
            if (cutaway && c != 'w') {
                int along = (((wx - focx) * fdx) >> 12) + (((wz - focz) * fdz) >> 12);   // distance past the focus along the view
                if (along < -TILE / 4) { h = WALL_LOW; cut = 1; }
            }
            wall_tile(mx, my, h, cut); world_dbg_walls++;
        } else if (c != '.') {
            if (c == 'D') { int along = (((wx - focx) * fdx) >> 12) + (((wz - focz) * fdz) >> 12); door_tile(mx, my, (cutaway && along < -TILE / 4) ? WALL_LOW : WALL_FULL); }
            int rot = items_rotates(c) ? world_item_facing(mx, my) : 0;
            int along = items_along(c) ? world_along(mx, my, c) : 0;
            items_draw(c, mx, my, rot, along, t);
        }
    }
}
