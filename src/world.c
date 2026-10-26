// world.c - the lot: floors, walls (with the Sims cut-away), items, and BORE's default map.
#include "world.h"
#include "gfx.h"
#include "atlas.h"
#include "items.h"
#include "tex.h"
#include "wallart.h"
#include "bigmap.h"
#include "mem8.h"
#include "pop.h"
#include "far.h"
#include "nature.h"
#include "island.h"
#include "save.h"

// ---- the storeys above the ground (per zone, and one set for the creator's studio) ----
typedef struct { char map[MH][MW + 1]; u8 fl[MH][MW]; u8 wp[MH][MW]; } Level;
// Storage is a small shared pool: a storey takes a slot only once something is built on it (most zones have one loft at most),
// which keeps 15 empty 19 KB levels out of the PS1's 2 MB.
#define NLVPOOL 5
#define NLVPOOL8 20                                 // with 8 MB: storeys for every zone at once
static Level lvpool_small[NLVPOOL]; static Level *lvpool = lvpool_small; static int nlvpool = NLVPOOL; static u8 lvbusy[NLVPOOL8];
static u8 lvown[ZONE_N + 1][NUP];                   // per zone (and the studio): 1 + the pool slot of each storey, 0 = empty
static u8 *UP = lvown[0];
static Level *lv_get(int k) { return (k >= 0 && k < NUP && UP[k]) ? &lvpool[UP[k] - 1] : 0; }
static Level *lv_need(int k) {                      // the storey, taking a pool slot for it if it has none (0 if the pool is full)
    if (k < 0 || k >= NUP) return 0;
    if (!UP[k]) {
        int s = 0; while (s < nlvpool && lvbusy[s]) s++;
        if (s == nlvpool) return 0;
        Level *L = &lvpool[s]; lvbusy[s] = 1; UP[k] = (u8)(s + 1);
        for (int y = 0; y < MH; y++) { memset(L->map[y], ' ', MW); L->map[y][MW] = 0; } memset(L->fl, 0, sizeof L->fl); memset(L->wp, 0, sizeof L->wp);
    }
    return &lvpool[UP[k] - 1];
}
int world_lvl, world_view_lvl, world_roof_show, world_fp;   // (world_fp: first person: the roof over you is your ceiling)
int world_roof_style, world_roof_col = 0, world_roof_pitch = 1;
static int roof_dirty = 1;
static Home homes[ZONE_N][MAXHOMES]; static int nhomes[ZONE_N];
int world_home_count(int zone) { return nhomes[zone % ZONE_N]; }
const Home *world_home(int zone, int i) { return (i >= 0 && i < nhomes[zone % ZONE_N]) ? &homes[zone % ZONE_N][i] : 0; }
static int blk_bx = 0xFFFF, blk_by = 0xFFFF, blk_unit;   // the open world's block being made (its homes are numbered 0, 1, ...)
static int reg_home(int x0, int y0, int x1, int y1, int dx, int dy) {   // the generator that is building a home tells us where it is (in the zone being made)
    int z = world_zone % ZONE_N, u = blk_unit++;
    if (nhomes[z] >= MAXHOMES) return u;
    Home *h = &homes[z][nhomes[z]++]; h->x0 = (u8)x0; h->y0 = (u8)y0; h->x1 = (u8)x1; h->y1 = (u8)y1; h->dx = (u8)dx; h->dy = (u8)dy;
    h->bx = (u16)blk_bx; h->by = (u16)blk_by; h->unit = (u8)u;
    return u;
}
int world_home_find(int zone, int bx, int by, int unit) {
    zone %= ZONE_N;
    for (int i = 0; i < nhomes[zone]; i++) { const Home *h = &homes[zone][i]; if (h->bx == bx && h->by == by && h->unit == unit) return i; }
    return -1;
}
static void town_homes(int want); static void island_homes(int want); static int is_wall(char c);
static int world_zone_saved, world_wbx_saved, world_wby_saved;   // (world_load: where the save was; world_place goes there)
static u8 wdirty[5][5];                             // the open world's window: blocks the player has built on
static u8 lvl_used[NLEVELS];
static void levels_clear(u8 *own) {                 // every storey of a zone back to empty air (their pool slots freed)
    for (int k = 0; k < NUP; k++) { if (own[k]) lvbusy[own[k] - 1] = 0; own[k] = 0; }
    roof_dirty = 1;
}
char world_at_l(int l, int mx, int my) {
    if (mx < 0 || my < 0 || mx >= MW || my >= MH) return l ? ' ' : 'W';
    if (l <= 0) return lifeMap[my][mx];
    const Level *L = lv_get(l - 1); return L ? L->map[my][mx] : ' ';
}
static char dummy_c; static u8 dummy_u;           // what a write lands on when no storey slot is left (it is lost)
static void mark_dirty(int l, int mx, int my);
char *world_cell(int l, int mx, int my) { mark_dirty(l, mx, my); if (l <= 0) return &lifeMap[my][mx]; Level *L = lv_need(l - 1); if (!L) { dummy_c = ' '; return &dummy_c; } return &L->map[my][mx]; }
u8 *world_floor_cell(int l, int mx, int my) { mark_dirty(l, mx, my); if (l <= 0) return &floorMap[my][mx]; Level *L = lv_need(l - 1); if (!L) { dummy_u = 0; return &dummy_u; } return &L->fl[my][mx]; }
u8 *world_wall_cell(int l, int mx, int my) { mark_dirty(l, mx, my); if (l <= 0) return &wallMap[my][mx]; Level *L = lv_need(l - 1); if (!L) { dummy_u = 0; return &dummy_u; } return &L->wp[my][mx]; }
static VECTOR cam_keep_eye, cam_keep_at; static int cam_pushed;
void world_cam_push(int l) {
    if (cam_pushed || l <= 0) return;
    cam_keep_eye = cam.eye; cam_keep_at = cam.at; cam_pushed = 1;
    cam.eye.vy += l * WALL_FULL; cam.at.vy += l * WALL_FULL; gfx_camera_update(); gfx_set_identity();   // (the GTE too: the storey's walls and floors are drawn straight in world space)
}
void world_cam_pop(void) { if (!cam_pushed) return; cam.eye = cam_keep_eye; cam.at = cam_keep_at; gfx_camera_update(); gfx_set_identity(); cam_pushed = 0; }
static const char *const roofStyleNm[NROOF_STYLE] = { "HIP", "GABLE", "FLAT" };
static const char *const roofColNm[NROOF_COL] = { "CLAY RED", "SLATE", "CEDAR", "MOSS GREEN", "NAVY", "SANDSTONE" };
static const u8 roofCol[NROOF_COL][3] = { { 196, 92, 64 }, { 104, 112, 128 }, { 150, 104, 66 }, { 92, 132, 84 }, { 70, 86, 140 }, { 206, 180, 128 } };
static const char *const roofPitchNm[NROOF_PITCH] = { "LOW", "MEDIUM", "STEEP" };
static const s16 roofPitch[NROOF_PITCH] = { 110, 170, 250 };   // rise per tile run (a tile is 256: 250 is about 45 degrees)
const char *world_roof_style_name(int s) { return roofStyleNm[s % NROOF_STYLE]; }
const char *world_roof_col_name(int c) { return roofColNm[c % NROOF_COL]; }
const char *world_roof_pitch_name(int p) { return roofPitchNm[p % NROOF_PITCH]; }

char lifeMap[MH][MW + 1];
static int ao_dirty = 1;
int dbg_noao, dbg_nograd;   // the floor shading needs working out again (world_touched)
u8 floorMap[MH][MW], wallMap[MH][MW];

static TexRef fl_tex[NFLOORS];       // 8x8 (the water: 16x16, the pattern twice over, so it can scroll)
static TexRef fl_tex4[NFLOORS];      // 32x32: four tiles' worth, the checker shade of the odd tiles baked in (2x2 blocks of floor draw as one quad)
#define PN_NOISE "01000200","00020010","20001000","00100020","01000100","00200001","10002000","00010200"
// the outdoor floors BORE 2 adds: 16x16, 4 colours, hand drawn here
static const Tex outTex[19] = {
 {"GRASS",{RGB(9,18,6),RGB(12,22,8),RGB(7,15,5),RGB(14,24,10)},{"01010210","10301001","01010100","20101031","01010210","10201001","31010100","00101010"}},
 {"PAVING",{RGB(17,16,15),RGB(20,19,18),RGB(13,12,12),RGB(22,21,19)},{"00000003","01110111","01110111","01110111","00000000","11101110","11101110","11101120"}},
 {"SOIL",{RGB(12,8,5),RGB(14,10,6),RGB(9,6,4),RGB(16,11,7)},{PN_NOISE}},
 {"SAND",{RGB(28,24,14),RGB(30,26,17),RGB(26,22,12),RGB(31,28,20)},{PN_NOISE}},
 {"WATER",{RGB(6,14,26),RGB(8,18,29),RGB(12,22,31),RGB(20,27,31)},{"00100000","01110020","00100000","00000100","20001110","00000100","01000000","11100001"}},
 {"ROCK",{RGB(15,13,12),RGB(18,16,14),RGB(12,11,10),RGB(21,19,16)},{"00010000","01000200","00000001","00020000","10000100","00000000","00100010","02000000"}},
 {"ASPHALT",{RGB(8,8,9),RGB(9,9,10),RGB(6,6,7),RGB(11,11,12)},{"01000100","00020010","10000001","00200010","01000100","00010020","10002000","00100001"}},
 {"ROAD LINE",{RGB(8,8,9),RGB(9,9,10),RGB(6,6,7),RGB(29,25,8)},{"01000100","00020010","33333333","00200010","01000100","33333333","10002000","00100001"}},
 {"ROAD LINE",{RGB(8,8,9),RGB(9,9,10),RGB(6,6,7),RGB(29,25,8)},{"01300300","00320310","10300301","00300310","01300300","00310320","10302300","00300301"}},
 {"BRICK PATIO",{RGB(21,9,7),RGB(18,8,6),RGB(24,20,18),RGB(16,7,5)},{"00010002","22222222","20001000","22222222","00010002","22222222","20001000","22222222"}},
 {"WOOD DECK",{RGB(21,15,8),RGB(18,12,6),RGB(24,18,10),RGB(12,8,4)},{"00001000","00010020","33333333","01000200","10000010","00200001","33333333","00010000"}},
 {"POOL TILE",{RGB(18,25,29),RGB(21,27,30),RGB(14,21,26),RGB(27,30,31)},{"00000003","00000003","00000003","00000003","00000003","00000003","00000003","33333333"}},
 {"GRAVEL",{RGB(16,15,14),RGB(20,19,17),RGB(12,11,10),RGB(24,23,21)},{"01020010","30010201","02100320","10200103","01030210","20100302","03021001","10300210"}},
 {"CROSSWALK",{RGB(8,8,9),RGB(9,9,10),RGB(6,6,7),RGB(25,25,24)},{"01000100","33333333","33333333","00200010","01000100","33333333","33333333","00100001"}},
 {"CROSSWALK",{RGB(8,8,9),RGB(9,9,10),RGB(6,6,7),RGB(25,25,24)},{"03300330","03320330","13300331","03300330","03300330","03310330","13302330","03300331"}},
 {"ROAD EDGE",{RGB(8,8,9),RGB(9,9,10),RGB(6,6,7),RGB(25,25,24)},{"01000100","33333333","10000001","00200010","01000100","00010020","10002000","00100001"}},
 {"ROAD EDGE",{RGB(8,8,9),RGB(9,9,10),RGB(6,6,7),RGB(25,25,24)},{"01000100","00020010","10000001","00200010","01000100","00010020","33333333","00100001"}},
 {"ROAD EDGE",{RGB(8,8,9),RGB(9,9,10),RGB(6,6,7),RGB(25,25,24)},{"03000100","03020010","13000001","03200010","03000100","03010020","13002000","03100001"}},
 {"ROAD EDGE",{RGB(8,8,9),RGB(9,9,10),RGB(6,6,7),RGB(25,25,24)},{"01000130","00020030","10000031","00200030","01000130","00010030","10002030","00100031"}},
};
static TexRef wp_tex[NWALLS];        // 8x24
static u16 wp_top[NWALLS];
static u8 fl_odd[NFLOORS] = { 14, 15, 15, 16, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 15, 16, 16, 16, 15, 16, 16, 16, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16 };
static u16 tb[32 * 32];
static TexRef glass_tex;

static const Tex newWp[6] = {   // the base game's newer walls (8x8, repeated down the wall)
 {"BRICK",{RGB(22,9,6),RGB(18,7,5),RGB(26,22,20),RGB(15,6,4)},{"00010002","00010002","22222222","20001000","20001000","22222222","00010002","22222222"}},
 {"CLAPBOARD",{RGB(29,28,24),RGB(26,25,21),RGB(22,21,18),RGB(30,30,27)},{"33333333","00000000","00000000","11111111","22222222","33333333","00000000","11111111"}},
 {"STUCCO",{RGB(27,23,17),RGB(25,21,15),RGB(29,25,19),RGB(23,19,13)},{"01000200","00020010","20001000","00100020","01000100","00200001","10002000","00010200"}},
 {"SHINGLE WALL",{RGB(16,12,8),RGB(20,15,10),RGB(12,9,6),RGB(23,18,12)},{"00110011","11000122","22222222","11001100","00110011","33333333","10011001","22222222"}},
 {"BEADBOARD",{RGB(28,27,24),RGB(24,23,20),RGB(30,30,28),RGB(20,19,17)},{"20212021","20212021","20212021","20212021","20212021","20212021","20212021","33333333"}},
 {"STONE WALL",{RGB(16,15,14),RGB(20,19,17),RGB(12,11,10),RGB(9,8,8)},{"00010110","01110112","10000122","33333333","01100011","11110111","22001000","33333333"}},
};
#define NWPNEW 6
const char *world_wall_name(int wp) { return wp < NWP ? look_wp_tex(wp)->nm : wp < NWP + NWX ? wxName[wp - NWP] : newWp[(wp - NWP - NWX) % NWPNEW].nm; }
static const Tex *floor_tex(int i) { return i < NFL ? look_fl_tex(i) : &outTex[(i - NFL) % 19]; }
const char *world_floor_name(int fl) { return floor_tex(fl % NFLOORS)->nm; }

static u16 fl_soft[NFLOORS][4];   // the floor colours, calmed: a GBA pattern blown up across a 3D floor shouts, so each colour leans toward the tile's average
static u16 fl_avg[NFLOORS];       // ... and the average itself (the ground far off)
static void soften_floors(void) {
    for (int i = 0; i < NFLOORS; i++) {
        const Tex *t = floor_tex(i);
        int r = 0, g = 0, b = 0, n = 0;
        for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) { u16 c = t->c[t->p[y][x] - '0']; r += C_R(c); g += C_G(c); b += C_B(c); n++; }
        r /= n; g /= n; b /= n; fl_avg[i] = RGB(r, g, b);
        for (int k = 0; k < 4; k++) { u16 c = t->c[k]; if (FL_IS_ROAD(i)) fl_soft[i][k] = c; else fl_soft[i][k] = RGB((C_R(c) * 9 + r * 7) / 16, (C_G(c) * 9 + g * 7) / 16, (C_B(c) * 9 + b * 7) / 16); }
    }
}
static int skin_key = -1;
static u16 skin_c(int i, u16 c, int season, int wet) {   // one floor colour as the season and weather make it
    int r = C_R(c), g = C_G(c), b = C_B(c), tr = r, tg = g, tb = b, f = 0;   // (f: how far toward the target, of 16)
    int snow = season == 3;
    if (i == FL_GRASS) { if (season == 0) { tr = 9; tg = 27; tb = 8; f = 8; } else if (season == 2) { tr = 24; tg = 15; tb = 6; f = 11; } else if (snow) { tr = 29; tg = 30; tb = 31; f = 15; } }
    else if (i == FL_WATER) { if (snow) { tr = 22; tg = 28; tb = 31; f = 9; } }
    else if (i == FL_LINE_H || i == FL_LINE_V || i >= FL_ZEB_A) { if (snow) { tr = 28; tg = 29; tb = 31; f = 9; } }
    else if (i >= FL_PAVING && i <= FL_ASPHALT && i != FL_WATER) { if (snow) { tr = 28; tg = 29; tb = 31; f = i == FL_ASPHALT ? 14 : 12; } }
    r += (tr - r) * f / 16; g += (tg - g) * f / 16; b += (tb - b) * f / 16;
    if (wet == 1 && i >= FL_GRASS && i != FL_WATER) { r = r * 11 / 16; g = g * 12 / 16; b = b * 13 / 16; }   // rain-dark
    return RGB(r, g, b);
}
void world_season_skin(int season, int wet) {
    int key = season * 4 + wet; if (key == skin_key) return; skin_key = key;
    for (int i = FL_GRASS; i < NFLOORS; i++) {
        const Tex *t = floor_tex(i); u16 sc[4]; int r = 0, g = 0, b = 0;
        for (int k = 0; k < 4; k++) sc[k] = skin_c(i, fl_soft[i][k], season, wet);
        { int sz = i == FL_WATER ? 16 : 8; for (int y = 0; y < sz; y++) for (int x = 0; x < sz; x++) { u16 c = sc[t->p[y & 7][x & 7] - '0']; tb[y * sz + x] = c ? c : 0x8000; } }
        atlas_upload(&fl_tex[i], tb);
        for (int y = 0; y < 32; y++) for (int x = 0; x < 32; x++) {
            u16 c = sc[t->p[y & 7][x & 7] - '0'];
            int odd = ((x >> 3) + (y >> 3)) & 1, k = odd ? fl_odd[i] : 16;
            u16 o = RGB(C_R(c) * k / 16, C_G(c) * k / 16, C_B(c) * k / 16);
            tb[y * 32 + x] = o ? o : 0x8000;
        }
        atlas_upload(&fl_tex4[i], tb);
        for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) { u16 c = sc[t->p[y][x] - '0']; r += C_R(c); g += C_G(c); b += C_B(c); }
        fl_avg[i] = RGB(r / 64, g / 64, b / 64);
    }
}
#pragma GCC push_options
#pragma GCC optimize ("Os")   // (making the lots, rooms and the island: once a block, built small to leave memory for the world)
void world_init(void) {
    nat_init();
    if (ram8) { Level *p = (Level *)xalloc(sizeof(Level) * NLVPOOL8); if (p) { lvpool = p; nlvpool = NLVPOOL8; } }   // 8 MB: room for every storey anywhere
    for (int z = 0; z <= ZONE_N; z++) levels_clear(lvown[z]);
    soften_floors();
    for (int i = 0; i < NFLOORS; i++) {
        const Tex *t = floor_tex(i);
        int sz = i == FL_WATER ? 16 : 8;   // (only the water scrolls, so only it needs the pattern twice over)
        for (int y = 0; y < sz; y++) for (int x = 0; x < sz; x++) { u16 c = fl_soft[i][t->p[y & 7][x & 7] - '0']; tb[y * sz + x] = c ? c : 0x8000; }
        fl_tex[i] = atlas_add(sz, sz, tb);
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
            u16 c = wxPal[wxPalOff[i] + wxIdx[i][x][y]];
            int r = MIN(31, C_R(c) * 16 / 12), g = MIN(31, C_G(c) * 16 / 12), b = MIN(31, C_B(c) * 16 / 12);
            u16 o = RGB(r, g, b); tb[y * 8 + x] = o ? o : 0x8000;
        }
        wp_tex[NWP + i] = atlas_add(8, 24, tb);
        wp_top[NWP + i] = wxAvg[i];
    }
    for (int i = 0; i < NWPNEW; i++) {
        const Tex *t = &newWp[i]; u32 ar = 0, ag = 0, ab = 0;
        for (int y = 0; y < 24; y++) for (int x = 0; x < 8; x++) { u16 c = t->c[t->p[y & 7][x] - '0']; tb[y * 8 + x] = c ? c : 0x8000; }
        for (int k = 0; k < 4; k++) { ar += C_R(t->c[k]); ag += C_G(t->c[k]); ab += C_B(t->c[k]); }
        wp_tex[NWP + NWX + i] = atlas_add(8, 24, tb);
        wp_top[NWP + NWX + i] = RGB(ar / 4, ag / 4, ab / 4);
    }
    for (int i = 0; i < 64; i++) tb[i] = RGB(18 + (i & 1), 24, 30);
    glass_tex = atlas_add(8, 8, tb);
}

// ---- BORE's map builders ----
__attribute__((noinline)) static void gBox(int x0, int y0, int x1, int y1, int fl) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) floorMap[y][x] = (u8)fl; }
__attribute__((noinline)) static void gRoom(int x0, int y0, int x1, int y1, int fl, int wp) {
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { floorMap[y][x] = (u8)fl;
        if (x == x0 || x == x1 || y == y0 || y == y1) { lifeMap[y][x] = 'W'; wallMap[y][x] = (u8)wp; } }
}
__attribute__((noinline)) static void gLine(int x0, int y0, int x1, int y1, char c, int wp) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { lifeMap[y][x] = c; wallMap[y][x] = (u8)wp; } }
static void gPut(int x, int y, char c) { lifeMap[y][x] = c; }
__attribute__((noinline)) static void gFree(int x, int y, char c) { if (x >= 0 && y >= 0 && x < MW && y < MH && lifeMap[y][x] == '.') lifeMap[y][x] = c; }

// THE MAN BASE gets an upstairs: stairs in the crate room rise to a bedroom over its north-east corner (shows off the storeys)
static void gUpstairs(void) {
    const int x0 = 10, y0 = 2, x1 = 17, y1 = 9, sx = 15, sy = 7;
    lifeMap[sy][sx] = '|';                                          // the stairs, rising to the north
    Level *U = lv_need(0); if (!U) return;
    for (int j = y0; j <= y1; j++) for (int i = x0; i <= x1; i++) {
        U->map[j][i] = (i == x0 || i == x1 || j == y0 || j == y1) ? 'W' : '.'; U->fl[j][i] = 6; U->wp[j][i] = (u8)(NWP + 12);
    }
    U->map[y0][13] = 'E'; U->map[y0][15] = 'E'; U->map[5][x1] = 'E'; U->map[y1][12] = 'E';
    U->map[sy][sx] = ' ';                                           // the stairwell
    U->map[4][12] = 'S'; U->map[3][16] = 'b'; U->map[7][11] = 'l'; U->map[3][11] = 'C';
    roof_dirty = 1;
}
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
    levels_clear(UP); gUpstairs();
}

static int bk_on, bk_slot;   // the live lot, kept in a free storey slot while the creator's studio is shown (a storey is the same size as a lot)
int world_backup(void) {
    if (bk_on) return 1;
    int s = 0; while (s < nlvpool && lvbusy[s]) s++;
    if (s == nlvpool) return 0;                                  // (every slot holds a storey: no studio now)
    Level *L = &lvpool[s]; lvbusy[s] = 1; bk_slot = s;
    memcpy(L->map, lifeMap, sizeof L->map); memcpy(L->fl, floorMap, sizeof L->fl); memcpy(L->wp, wallMap, sizeof L->wp);
    bk_on = 1; UP = lvown[ZONE_N]; levels_clear(UP);
    return 1;
}
void world_restore(void) {
    ao_dirty = 1; roof_dirty = 1; if (!bk_on) return; levels_clear(lvown[ZONE_N]); UP = lvown[world_zone];
    const Level *L = &lvpool[bk_slot];
    memcpy(lifeMap, L->map, sizeof L->map); memcpy(floorMap, L->fl, sizeof L->fl); memcpy(wallMap, L->wp, sizeof L->wp);
    lvbusy[bk_slot] = 0; bk_on = 0; }
void world_gen_studio(void) {
    ao_dirty = 1;   // a 9x9 parlour: the creator's stage
    for (int y = 0; y < MH; y++) { for (int x = 0; x < MW; x++) { lifeMap[y][x] = '.'; floorMap[y][x] = 4; wallMap[y][x] = 0; } lifeMap[y][MW] = 0; }
    gRoom(0, 0, 8, 8, 1, NWP + 64);                                            // a light, airy room: beige carpet, pale walls
    gPut(2, 8, 'E'); gPut(6, 8, 'E'); gPut(0, 3, 'E'); gPut(8, 5, 'E');         // windows on the wall behind the Bore and the sides
    gPut(1, 1, 'i'); gPut(7, 1, 'l'); gPut(1, 7, 'p'); gPut(7, 7, 'p'); gPut(4, 1, 'b');
    gPut(4, 4, 'r'); gPut(4, 5, 'P'); gPut(6, 6, 'B');
    gPut(6, 1, 'V'); gPut(2, 1, 'c'); gPut(7, 4, 'C'); gPut(1, 4, 'l');
}
// ---- the zones ----
int world_zone = ZONE_HOME;
static const char *const zoneNm[ZONE_N] = { "THE MAN BASE", "CASTAWAY ISLAND", "STRANGE TOWN", "SAN BOREAS" };
const char *world_zone_name(int z) { return zoneNm[z % ZONE_N]; }
// ---- the places you are away from (THE MAN BASE, the island, the town), kept packed: the very bytes the save writes for them ----
// (SAN BOREAS needs no copy: its window is built from the generator.) A place that will not pack into what is left goes, as it is,
// into a free storey slot instead.
#define ZPACK (28 * 1024)
static u8 zpack[ZPACK]; static u16 zp_off[ZONE_CITY], zp_len[ZONE_CITY]; static u8 zp_made[ZONE_CITY]; static s8 zp_slot[ZONE_CITY] = { -1, -1, -1 };
int world_zone_lost;   // a place that could not be kept (no room at all): main says so
static int zp_used(void) { int u = 0; for (int i = 0; i < ZONE_CITY; i++) if (zp_slot[i] < 0) u += zp_len[i]; return u; }
static void zone_drop(int z) {
    if (zp_slot[z] >= 0) { lvbusy[zp_slot[z]] = 0; zp_slot[z] = -1; }
    else if (zp_len[z]) {
        int o = zp_off[z], n = zp_len[z]; memmove(zpack + o, zpack + o + n, (size_t)(zp_used() - o - n));
        for (int i = 0; i < ZONE_CITY; i++) if (i != z && zp_slot[i] < 0 && zp_len[i] && zp_off[i] > o) zp_off[i] = (u16)(zp_off[i] - n);
    }
    zp_len[z] = 0; zp_made[z] = 0;
}
static void zone_keep(int z) {   // the live tiles into the keep
    zone_drop(z);
    int u = zp_used(), a = rle_pack(lifeMap, MH * (MW + 1), zpack + u, ZPACK - u), b = -1, c = -1;
    if (a >= 0) b = rle_pack(floorMap, MH * MW, zpack + u + a, ZPACK - u - a);
    if (b >= 0) c = rle_pack(wallMap, MH * MW, zpack + u + a + b, ZPACK - u - a - b);
    if (c >= 0) { zp_off[z] = (u16)u; zp_len[z] = (u16)(a + b + c); zp_made[z] = 1; return; }
    int sl = 0; while (sl < nlvpool && lvbusy[sl]) sl++;
    if (sl == nlvpool) { world_zone_lost = 1; return; }
    Level *L = &lvpool[sl]; lvbusy[sl] = 1; zp_slot[z] = (s8)sl; zp_made[z] = 1;
    memcpy(L->map, lifeMap, sizeof L->map); memcpy(L->fl, floorMap, sizeof L->fl); memcpy(L->wp, wallMap, sizeof L->wp);
}
static void zone_restore(int z) {
    if (zp_slot[z] >= 0) { const Level *L = &lvpool[zp_slot[z]]; memcpy(lifeMap, L->map, sizeof L->map); memcpy(floorMap, L->fl, sizeof L->fl); memcpy(wallMap, L->wp, sizeof L->wp); return; }
    const u8 *in = zpack + zp_off[z];
    in += rle_unpack(in, lifeMap, MH * (MW + 1)); in += rle_unpack(in, floorMap, MH * MW); rle_unpack(in, wallMap, MH * MW);
}

static void gCircle(int cx, int cy, int r, int fl) { for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r) floorMap[y][x] = (u8)fl; }
static u32 wsalt;                                     // (the save's seed: below)
// ---- CASTAWAY ISLAND: a new one every life, filling the sea: a coast that wanders, beaches, meadows, jungle, rocky hills, a pond,
// an islet out at sea that only a raft reaches, and eight secrets hidden round it (island.c) ----
static int isl_ang(int dy, int dx) {   // the angle of (dx, dy), 4096 a turn (rough, and smooth: enough for a coastline)
    if (!dx && !dy) return 0;
    int ax = ABS(dx), ay = ABS(dy), a = ax >= ay ? ay * 512 / ax : 1024 - ax * 512 / ay;
    if (dx < 0) a = 2048 - a;
    if (dy < 0) a = 4096 - a;
    return a & 4095;
}
static int isl_noise(int x, int y, u32 s) {   // 0..255: value noise on a 10-tile grid
    int gx = x / 10, gy = y / 10, fx = x % 10, fy = y % 10;
    int a = (int)(bm_hash(gx, gy, s) & 255), b = (int)(bm_hash(gx + 1, gy, s) & 255), c = (int)(bm_hash(gx, gy + 1, s) & 255), d = (int)(bm_hash(gx + 1, gy + 1, s) & 255);
    int top = a * (10 - fx) + b * fx, bot = c * (10 - fx) + d * fx;
    return (top * (10 - fy) + bot * fy) / 100;
}
static int isl_cx, isl_cy;                            // (the islet's middle)
static int isl_main(int x, int y) { return (x - isl_cx) * (x - isl_cx) + (y - isl_cy) * (y - isl_cy) > 81; }   // on the main island, not out on the islet
static int isl_by_water(int x, int y) { for (int k = 0; k < 4; k++) { int a = x + (k == 0) - (k == 1), b = y + (k == 2) - (k == 3); if (a < 0 || b < 0 || a >= MW || b >= MH || floorMap[b][a] == FL_WATER) return 1; } return 0; }
static int isl_open(int x, int y, int r) {   // dry land with nothing on it, nothing within r tiles either (and not on a path)
    if (x < 2 || y < 2 || x >= MW - 2 || y >= MH - 2) return 0;
    for (int j = y - r; j <= y + r; j++) for (int i = x - r; i <= x + r; i++) {
        if (i < 0 || j < 0 || i >= MW || j >= MH) return 0;
        if (lifeMap[j][i] != '.') return 0;
    }
    return floorMap[y][x] != FL_WATER && floorMap[y][x] != FL_PAVING;
}
static int isl_pick(u32 s, int want_sand, int far_x, int far_y, int mind, int *ox, int *oy) {   // somewhere open: on a beach (want_sand 1) or inland (0), far enough from (far_x, far_y)
    for (int t = 0; t < 600; t++) {
        u32 h = bm_hash(t, (int)s, wsalt ^ 0x15u); int x = 4 + (int)(h % (MW - 8)), y = 4 + (int)((h >> 12) % (MH - 8));
        if (!isl_open(x, y, 1) || (floorMap[y][x] == FL_SAND) != want_sand || !isl_main(x, y)) continue;
        if ((x - far_x) * (x - far_x) + (y - far_y) * (y - far_y) < mind * mind) continue;
        *ox = x; *oy = y; return 1;
    }
    return 0;
}
// CREATE A WORLD's island dials (1 1 1 1: the island as it always was)
int isl_size = 1, isl_wild = 1, isl_jungle = 1, isl_rocks = 1;
void isl_dials_default(void) { isl_size = isl_wild = isl_jungle = isl_rocks = 1; }
static int isl_amp[3], isl_ph[3];
static void isl_coast_init(u32 S) {
    static const u8 wk[3] = { 6, 12, 20 };   // COAST: SMOOTH, WAVY, WILD (twelfths of the waves)
    for (int k = 0; k < 3; k++) { u32 h = bm_hash(k, 7, S); isl_amp[k] = ((int)(h % 40u) + (k ? 16 : 32)) * wk[isl_wild % 3] / 12; isl_ph[k] = (int)((h >> 10) & 4095); }
}
static int isl_land(int x, int y) {   // 0 sea, 1 sand, 2 inland (the main island's coast only)
    static const u8 r0[3] = { 22, 29, 34 }, rmin[3] = { 16, 22, 27 }, rmax[3] = { 27, 35, 38 };   // SIZE: SMALL, MEDIUM, BIG
    static const u8 frq[3] = { 2, 3, 5 };
    int z = isl_size % 3, dx = x - MW / 2, dy = y - MH / 2, a = isl_ang(dy, dx), d = 0;
    { int q = dx * dx + dy * dy; while ((d + 1) * (d + 1) <= q) d++; }
    int R = r0[z] * 16; for (int k = 0; k < 3; k++) R += (isl_amp[k] * isin((frq[k] * a + isl_ph[k]) & 4095)) >> 12;
    R = CLAMP(R, rmin[z] * 16, rmax[z] * 16);
    if (d * 16 >= R) return 0;
    return d * 16 >= R - 3 * 16 ? 1 : 2;
}
static const u8 isl_rock_at[3] = { 205, 178, 150 }, isl_jung_at[3] = { 160, 120, 80 };   // HILLS and JUNGLE: LESS, SOME, MORE
void world_isl_preview(u32 seed, u16 *px) {   // the castaway's island for that seed, from above, 128 x 128 (CREATE A WORLD's map)
    u32 S = seed * 2654435761u + 0x15ADu;
    isl_coast_init(S);
    int corner = (int)(S >> 5) & 3, ix = (corner & 1) ? MW - 12 : 11, iy = (corner & 2) ? MH - 12 : 11;
    for (int Y = 0; Y < 128; Y++) for (int X = 0; X < 128; X++) {
        int x = X * MW / 128, y = Y * MH / 128, q = (x - ix) * (x - ix) + (y - iy) * (y - iy);
        int l = (x < 1 || y < 1 || x >= MW - 1 || y >= MH - 1 || q < 100) ? 0 : isl_land(x, y);
        if (q <= 25) l = q <= 6 ? 2 : 1;
        u16 c = RGB(4, 12, 22);
        if (l == 1) c = RGB(27, 25, 17);
        else if (l == 2) c = isl_noise(x, y, S ^ 0xA11u) > isl_rock_at[isl_rocks % 3] ? RGB(16, 16, 15) : isl_noise(x, y, S ^ 0x77u) > isl_jung_at[isl_jungle % 3] ? RGB(4, 13, 5) : RGB(11, 20, 8);
        px[Y * 128 + X] = c | 0x8000;
    }
}
void world_gen_island(void) {
    nhomes[ZONE_ISLAND] = 0;
    ao_dirty = 1;
    u32 S = wsalt * 2654435761u + 0x15ADu;
    for (int y = 0; y < MH; y++) { for (int x = 0; x < MW; x++) { lifeMap[y][x] = '.'; floorMap[y][x] = FL_WATER; wallMap[y][x] = 0; } lifeMap[y][MW] = 0; }
    // the islet's corner, and the main island's coast: a radius that wanders with the angle (three waves, of random size and phase)
    int corner = (int)(S >> 5) & 3, ix = (corner & 1) ? MW - 12 : 11, iy = (corner & 2) ? MH - 12 : 11;
    isl_cx = ix; isl_cy = iy;
    isl_coast_init(S);
    for (int y = 1; y < MH - 1; y++) for (int x = 1; x < MW - 1; x++) {
        int l = isl_land(x, y);
        if (!l) continue;
        if ((x - ix) * (x - ix) + (y - iy) * (y - iy) < 10 * 10) continue;   // (the sea round the islet)
        floorMap[y][x] = l == 1 ? FL_SAND : FL_GRASS;
    }
    for (int y = 1; y < MH - 1; y++) for (int x = 1; x < MW - 1; x++)   // what the land is: meadow, jungle, rocky hills
        if (floorMap[y][x] == FL_GRASS && isl_noise(x, y, S ^ 0xA11u) > isl_rock_at[isl_rocks % 3]) floorMap[y][x] = FL_ROCK;
    {   // the pond, inland: fresh water, and a spring beside it
        int px = MW / 2 + (int)(bm_hash(1, 2, S) % 17u) - 8, py = MH / 2 - 6 - (int)(bm_hash(3, 4, S) % 6u);
        for (int y = py - 3; y <= py + 3; y++) for (int x = px - 4; x <= px + 4; x++) if ((x - px) * (x - px) * 9 + (y - py) * (y - py) * 16 <= 144 && floorMap[y][x] != FL_SAND) floorMap[y][x] = FL_WATER;
        if (floorMap[py + 3][px] != FL_WATER) lifeMap[py + 3][px] = '0';
    }
    // the islet: sand round a little grass, palms, and the golden idol in the middle of it
    for (int y = iy - 5; y <= iy + 5; y++) for (int x = ix - 5; x <= ix + 5; x++) {
        int q = (x - ix) * (x - ix) + (y - iy) * (y - iy); if (q <= 25) floorMap[y][x] = q <= 6 ? FL_GRASS : FL_SAND; }
    lifeMap[iy][ix] = CH_GOLD; lifeMap[iy - 2][ix - 2] = 'g'; lifeMap[iy + 2][ix + 3] = 'g'; lifeMap[iy - 3][ix + 2] = 'g';
    {   // where a raft goes: from the island's beach nearest the islet, to the islet's beach nearest that
        int bd = 1 << 30, dx_ = MW / 2, dy_ = MH / 2;
        for (int y = 2; y < MH - 2; y++) for (int x = 2; x < MW - 2; x++) {
            if (floorMap[y][x] != FL_SAND || (x - ix) * (x - ix) + (y - iy) * (y - iy) <= 36) continue;
            int q = (x - ix) * (x - ix) + (y - iy) * (y - iy); if (q < bd) { bd = q; dx_ = x; dy_ = y; } }
        isl_dock_x = (u8)dx_; isl_dock_y = (u8)dy_; bd = 1 << 30;
        for (int y = iy - 5; y <= iy + 5; y++) for (int x = ix - 5; x <= ix + 5; x++) {
            if (floorMap[y][x] != FL_SAND || lifeMap[y][x] != '.') continue;
            int q = (x - dx_) * (x - dx_) + (y - dy_) * (y - dy_); if (q < bd) { bd = q; isl_islet_x = (u8)x; isl_islet_y = (u8)y; } }
    }
    // the way in: a jetty off the south shore, the boat at its end; the camp at its foot
    int jx = MW / 2, js = MH - 2; while (js > MH / 2 && floorMap[js][jx] == FL_WATER) js--;
    gBox(jx, js + 1, jx + 1, js + 4, FL_PAVING); gPut(jx, js + 5, '@'); gPut(jx, js + 6, '.'); floorMap[js + 5][jx] = FL_WATER;
    gPut(jx + 1, js + 4, CH_FISH);                                          // (fishing off the end of the jetty)
    int cx = jx, cy = js - 5; isl_fire_x = (u8)cx; isl_fire_y = (u8)cy;
    for (int y = cy - 3; y <= cy + 4; y++) for (int x = cx - 5; x <= cx + 5; x++) if (floorMap[y][x] == FL_WATER || floorMap[y][x] == FL_ROCK) floorMap[y][x] = FL_SAND;   // (dry ground for the camp)
    gBox(jx, cy + 2, jx, js, FL_PAVING);                                    // (a path down to the jetty)
    gPut(cx, cy, 's'); gPut(cx - 2, cy - 1, 'z'); gPut(cx + 3, cy - 2, '&'); gPut(cx - 1, cy + 1, '%'); gPut(cx + 1, cy + 1, '%');
    gPut(cx - 3, cy + 2, 'a'); gPut(jx + 1, cy + 3, 'P');
    island_homes(3);                                                        // the islanders' huts
    int sx, sy;                                                             // the secrets
    if (isl_pick(S ^ 1, 1, cx, cy, 30, &sx, &sy)) lifeMap[sy][sx] = CH_BOTTLE;
    if (isl_pick(S ^ 2, 0, cx, cy, 22, &sx, &sy) && lifeMap[sy][sx + 1] == '.') { lifeMap[sy][sx] = '*'; lifeMap[sy][sx + 1] = CH_DIG; }
    {   // the tiki idol: the thickest jungle
        int best = -1, bx = -1, by = -1;
        for (int y = 4; y < MH - 4; y++) for (int x = 4; x < MW - 4; x++) if (floorMap[y][x] == FL_GRASS && isl_open(x, y, 1) && isl_main(x, y)) {
            int n = isl_noise(x, y, S ^ 0x77u); if (n > best) { best = n; bx = x; by = y; } }
        if (bx >= 0) lifeMap[by][bx] = CH_IDOL;
    }
    {   // the wreck: on a beach by the water, as far east or west as the island goes
        int bx = -1, by = -1, bd = -1;
        for (int y = 6; y < MH - 6; y++) for (int x = 2; x < MW - 2; x++) if (floorMap[y][x] == FL_SAND && isl_by_water(x, y) && isl_open(x, y, 1) && ABS(y - cy) > 8 && isl_main(x, y)) {
            int d = ABS(x - MW / 2); if (d > bd) { bd = d; bx = x; by = y; } }
        if (bx >= 0) lifeMap[by][bx] = CH_WRECK;
    }
    for (int k = 0; k < ISL_SHELLS; k++) if (isl_pick(S ^ (0x50u + k), 1, cx, cy, 6, &sx, &sy)) lifeMap[sy][sx] = CH_SHELL;
    if (isl_pick(S ^ 3, 0, cx, cy, 24, &sx, &sy) && isl_open(sx + 1, sy, 1)) { lifeMap[sy][sx] = 'z'; lifeMap[sy][sx + 1] = CH_JOURNAL; }   // the hermit's old camp
    for (int k = 0; k < 2; k++) {   // two more fishing spots, on beaches
        for (int t = 0; t < 400; t++) { u32 h = bm_hash(t, 90 + k, S); int x = 3 + (int)(h % (MW - 6)), y = 3 + (int)((h >> 12) % (MH - 6));
            if (floorMap[y][x] == FL_SAND && isl_by_water(x, y) && isl_open(x, y, 1) && isl_main(x, y) && (x - cx) * (x - cx) + (y - cy) * (y - cy) > 64) { lifeMap[y][x] = CH_FISH; break; } }
    }
    // the jetty is planks; the paths (the camp's, the huts') are trodden earth
    for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) if (floorMap[y][x] == FL_PAVING) floorMap[y][x] = (x >= jx && x <= jx + 1 && y > js) ? 4 : FL_SOIL;
    // the trees, the bushes and the rocks, on the ground that is left (never touching anything built, or a path)
    for (int y = 2; y < MH - 2; y++) for (int x = 2; x < MW - 2; x++) {
        int f = floorMap[y][x];
        if (f == FL_WATER || f == FL_PAVING || f == FL_SOIL || f == 4 || !isl_open(x, y, 1)) continue;
        if (ABS(x - cx) <= 6 && y >= cy - 4 && y <= cy + 5) continue;        // (the camp)
        if (ABS(x - isl_dock_x) <= 1 && ABS(y - isl_dock_y) <= 1) continue; // (the raft's beach)
        int r = (int)(bm_hash(x, y, S ^ 0x7EEu) % 1000u), jungle = isl_noise(x, y, S ^ 0x77u) > isl_jung_at[isl_jungle % 3];
        char c = 0;
        if (f == FL_SAND) c = r < 60 ? 'g' : 0;
        else if (f == FL_ROCK) c = r < 110 ? '*' : r < 150 ? '6' : 0;
        else if (jungle) c = r < 80 ? 'g' : r < 170 ? '3' : r < 215 ? CH_BUSH : 0;
        else c = r < 30 ? '3' : r < 62 ? CH_BUSH : 0;
        if (c) lifeMap[y][x] = c;
    }
}
void world_gen_town(void) {   // Strange Town: a desert main street with a diner, a trailer, the lab and work
    nhomes[ZONE_TOWN] = 0;
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
    town_homes(16);
}
// ================================================================== THE BUILDINGS (SAN BOREAS, the town, the island)
// Homes come in four kinds: the HOUSE (size, door, floors and wallpaper vary), the COTTAGE (one small room), TOWNHOUSES (two
// narrow homes under one roof line) and the TWO-STOREY house (a loft up the stairs). Every home is furnished for whoever lives in
// it: a couple gets one bed, roommates a bed each, kids bunk beds and a toy box, an old man an armchair, a radio and a chess
// table; an empty home has nothing but a FOR SALE sign out front. Then the places: shops (five kinds), diners, gyms, offices,
// inns, community pools, schools, arcades, parks, squares, skate plazas, empty lots and farms. All of it comes from the lot's
// own dice with the save's seed mixed in: a new life is a new world.
static u32 wsalt;                                     // the save's seed
static char TREE = '3';                               // what grows in the yards here: oaks, pines, palms, cacti
static unsigned city_h(int a, int b) { unsigned h = (unsigned)a * 374761393u + (unsigned)b * 668265263u + 12345u + wsalt * 2246822519u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }
static void city_path(int dx, int dy, int ly) { gBox(dx, dy + 1, dx, MIN(ly + 12, MH - 2), FL_PAVING); }   // the path from a door to the street

// ---- who lives in the home being built (from the people of SAN BOREAS; elsewhere, somebody) ----
typedef struct { u8 adults, kids, teens, elders, couple, vacant, babies; } Occ;
static Occ occ_blk[12]; static int occ_on;            // the households of the block being made
static const Occ occ_any = { 2, 0, 0, 0, 1, 0, 0 };
static const Occ *occ_of(int unit) { return occ_on ? &occ_blk[unit % 12] : &occ_any; }

// ---- furnishing a room for its people ----
// the storey being built on: the ground (the map) or one of the storeys above (the generators write through these)
static char (*sm)[MW + 1] = lifeMap; static u8 (*sfl)[MW] = floorMap, (*swp)[MW] = wallMap;
static int storey(int l) {
    if (l <= 0) { sm = lifeMap; sfl = floorMap; swp = wallMap; return 1; }
    Level *L = lv_need(l - 1); if (!L) { storey(0); return 0; }
    sm = L->map; sfl = L->fl; swp = L->wp; return 1;
}
static void sRoom(int x0, int y0, int x1, int y1, int fl, int wp) {   // a room on the storey: floor, walls round it, open floor inside
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { sfl[y][x] = (u8)fl;
        if (x == x0 || x == x1 || y == y0 || y == y1) { sm[y][x] = 'W'; swp[y][x] = (u8)wp; } else sm[y][x] = '.'; }
}
static void sLine(int x0, int y0, int x1, int y1, char c, int wp) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { sm[y][x] = c; swp[y][x] = (u8)wp; } }
__attribute__((noinline)) static void sFree(int x, int y, char c) { if (x >= 0 && y >= 0 && x < MW && y < MH && sm[y][x] == '.') sm[y][x] = c; }
static unsigned fz_h;
static int fz_rnd(int n) { fz_h = fz_h * 1103515245u + 12345u; return (int)((fz_h >> 16) % (unsigned)n); }
static int fz_x0, fz_y0, fz_x1, fz_y1, fz_dx, fz_dy, fz_ix, fz_iy, fz_ox, fz_oy;
static u8 fz_res[24][2]; static int fz_nres;           // tiles to keep clear (stairs)
static int fz_keep(int x, int y) {                    // the lane in from the door (the tile inside it, the next one in, the ones beside it), and anything reserved
    if (x == fz_ix && y == fz_iy) return 1;
    if (x == fz_ix + fz_ox && y == fz_iy + fz_oy) return 1;
    if (fz_ox == 0 && y == fz_iy && ABS(x - fz_ix) == 1) return 1;
    if (fz_oy == 0 && x == fz_ix && ABS(y - fz_iy) == 1) return 1;
    for (int i = 0; i < fz_nres; i++) if (fz_res[i][0] == x && fz_res[i][1] == y) return 1;
    return 0;
}
static int fz_free(int x, int y) { return x >= fz_x0 && x <= fz_x1 && y >= fz_y0 && y <= fz_y1 && sm[y][x] == '.' && !fz_keep(x, y); }
static u8 rgx[64], rgy[64]; static int rgn;
static void fz_ring(void) {                           // the tiles along the walls, clockwise: a run of them is always side by side
    rgn = 0;
    for (int x = fz_x0; x <= fz_x1; x++) { rgx[rgn] = (u8)x; rgy[rgn++] = (u8)fz_y0; }
    for (int y = fz_y0 + 1; y <= fz_y1; y++) { rgx[rgn] = (u8)fz_x1; rgy[rgn++] = (u8)y; }
    if (fz_y1 > fz_y0) for (int x = fz_x1 - 1; x >= fz_x0; x--) { rgx[rgn] = (u8)x; rgy[rgn++] = (u8)fz_y1; }
    if (fz_x1 > fz_x0) for (int y = fz_y1 - 1; y > fz_y0; y--) { rgx[rgn] = (u8)fz_x0; rgy[rgn++] = (u8)y; }
}
static int fz_run(const char *items, int start) {     // the items side by side along a wall, from about there on
    int k = (int)strlen(items); if (!rgn || k > rgn) return -1;
    for (int s = 0; s < rgn; s++) {
        int i0 = (start + s) % rgn, ok = 1;
        for (int j = 0; j < k && ok; j++) { int i = (i0 + j) % rgn; if (!fz_free(rgx[i], rgy[i])) ok = 0; }
        if (!ok) continue;
        for (int j = 0; j < k; j++) { int i = (i0 + j) % rgn; sm[rgy[i]][rgx[i]] = items[j]; }
        return i0;
    }
    return -1;
}
static int fz_walk(char c) { return c == '.' || c == 'r' || c == 'D'; }
static void fz_reach(void) {                          // everything must be reachable: what is walled in by other things goes again
    static u8 seen[16][16]; static u8 qx[256], qy[256];
    for (int pass = 0; pass < 3; pass++) {
        memset(seen, 0, sizeof seen);
        int h = 0, t = 0, W_ = fz_x1 - fz_x0 + 1, H_ = fz_y1 - fz_y0 + 1;
        if (W_ > 16 || H_ > 16) return;
        int sx = fz_ix, sy = fz_iy; if (sx < fz_x0 || sx > fz_x1 || sy < fz_y0 || sy > fz_y1) return;
        seen[sy - fz_y0][sx - fz_x0] = 1; qx[t] = (u8)sx; qy[t++] = (u8)sy;
        while (h < t) { int x = qx[h], y = qy[h++];
            static const signed char ox[4] = { 1, -1, 0, 0 }, oy[4] = { 0, 0, 1, -1 };
            for (int k = 0; k < 4; k++) { int a = x + ox[k], b = y + oy[k];
                if (a < fz_x0 || a > fz_x1 || b < fz_y0 || b > fz_y1 || seen[b - fz_y0][a - fz_x0] || !fz_walk(sm[b][a])) continue;
                seen[b - fz_y0][a - fz_x0] = 1; qx[t] = (u8)a; qy[t++] = (u8)b; } }
        int gone = 0;
        for (int y = fz_y0; y <= fz_y1; y++) for (int x = fz_x0; x <= fz_x1; x++) {
            char c = sm[y][x]; if (fz_walk(c) || c == '|' || c == 'W' || c == 'E' || c == ' ') continue;
            int ok = 0;
            static const signed char ox[4] = { 1, -1, 0, 0 }, oy[4] = { 0, 0, 1, -1 };
            for (int k = 0; k < 4; k++) { int a = x + ox[k], b = y + oy[k];
                if (a >= fz_x0 && a <= fz_x1 && b >= fz_y0 && b <= fz_y1 && seen[b - fz_y0][a - fz_x0]) ok = 1; }
            if (!ok) { sm[y][x] = '.'; gone++; }
        }
        if (!gone) return;
    }
}
static int fz_Wl, fz_Dd, fz_ld, fz_px, fz_py;                    // the room as seen from its door: l across (fz_ld at the door), d deep
static int fz_lx(int l, int d) { return fz_ix + fz_ox * d + fz_px * (l - fz_ld); }
static int fz_ly(int l, int d) { return fz_iy + fz_oy * d + fz_py * (l - fz_ld); }
static int fz_pfree(int l, int d) { return l >= 0 && l < fz_Wl && d >= 0 && d < fz_Dd && fz_free(fz_lx(l, d), fz_ly(l, d)); }
static void fz_pput(int l, int d, char c) { if (fz_pfree(l, d)) sm[fz_ly(l, d)][fz_lx(l, d)] = c; }
static u8 fz_pl[4][24], fz_pd[4][24]; static int fz_np[4], fz_pc[4];   // four paths of tiles along the walls (beds, kitchen, living, telly) and where each has got to
static void fz_padd(int p, int l, int d) { if (fz_np[p] < 24 && l >= 0 && l < fz_Wl && d >= 0 && d < fz_Dd) { fz_pl[p][fz_np[p]] = (u8)l; fz_pd[p][fz_np[p]++] = (u8)d; } }
static int fz_pp(int p, char c) {                    // the next free tile on the path takes the item (-1: the wall is full)
    if (!fz_pc[p] && !fz_np[p]) return -1;
    for (; fz_pc[p] < fz_np[p]; fz_pc[p]++) { int l = fz_pl[p][fz_pc[p]], d = fz_pd[p][fz_pc[p]]; if (fz_pfree(l, d)) { fz_pput(l, d, c); fz_pc[p]++; return 0; } }
    return -1;
}
enum { R_BATH = 1, R_BED = 2, R_KIT = 4, R_LIV = 8, R_STUDY = 16, R_GRAND = 32, R_PLAY = 64, R_ALL = R_BATH | R_BED | R_KIT | R_LIV | R_STUDY };
typedef struct { u8 abeds, bunks, cribs, tbeds, toys, extra; } Beds;   // what the sleepers in a room need: beds, bunks, cribs, teens' beds, the kids' toys, a guest bed
static Beds beds_of(const Occ *o) {
    Beds b; memset(&b, 0, sizeof b);
    int grown = o->adults + o->elders;
    b.abeds = (u8)(o->couple ? 1 + MAX(0, grown - 2) : grown); if (!b.abeds && !o->kids && !o->teens && !o->babies) b.abeds = 1;
    b.bunks = (u8)((o->kids + 1) / 2); b.cribs = o->babies; b.tbeds = o->teens; b.toys = (u8)(o->kids ? 1 : 0);
    return b;
}
static void fz_frame(int x0, int y0, int x1, int y1, int ix, int iy, int ox, int oy, unsigned h) {   // a room to furnish, seen from its door
    fz_x0 = x0; fz_y0 = y0; fz_x1 = x1; fz_y1 = y1; fz_ix = ix; fz_iy = iy; fz_ox = ox; fz_oy = oy; fz_dx = ix; fz_dy = iy; fz_h = h | 1;
    fz_ring(); fz_pc[0] = fz_pc[1] = fz_pc[2] = fz_pc[3] = 0; fz_np[0] = fz_np[1] = fz_np[2] = fz_np[3] = 0;
    fz_px = -oy; fz_py = ox;
    int c0 = fz_px * x0 + fz_py * y0, c1 = fz_px * x1 + fz_py * y1, cmin = MIN(c0, c1), cmax = MAX(c0, c1);
    int d0 = ox * x0 + oy * y0, d1 = ox * x1 + oy * y1, di = ox * ix + oy * iy;
    fz_Wl = cmax - cmin + 1; fz_ld = fz_px * ix + fz_py * iy - cmin; fz_Dd = MAX(d0, d1) - di + 1;
}
// ---- A LAYOUT, not a scatter: a room read from its door in (l across, d deep) and given the zones its roles ask for ----
//   the back wall: the bedroom at one end, the bathroom (toilet, shower, sink) in the other corner; the kitchen runs down the bathroom's
//   side wall (the plumbing is all on one side), or along the back wall in an open front room; the bedroom's wall carries on with the
//   sofa; the telly stands across from it, a rug between; the dining table in the middle by the kitchen; a study corner and the odd hobby.
static void fz_layout(int roles, const Beds *bd, const Occ *o) {
    const int RW = fz_Wl, RD = fz_Dd, back = RD - 1, mid = RD / 2, area = RW * RD;
    int bathEnd = fz_rnd(2) ? RW - 1 : 0, bedEnd = RW - 1 - bathEnd, sg = bedEnd == 0 ? 1 : -1, bathw = 0;
    if (roles & R_BATH) {
        char bath[4] = { '-', 'T', fz_rnd(4) ? 'H' : ',', 0 }; if (area < 24 && RW < 5) bath[2] = 0; if (RW < 2) bath[1] = 0;
        int k = (int)strlen(bath), ok = 0;
        for (int tries = 0; tries < 2 && !ok; tries++) {   // (the other corner if this one is in the way)
            int be = tries ? bedEnd : bathEnd, dir = be == 0 ? 1 : -1; ok = 1;
            for (int j = 0; j < k && ok; j++) if (!fz_pfree(be + dir * j, back)) ok = 0;
            if (ok) { for (int j = 0; j < k; j++) fz_pput(be + dir * j, back, bath[j]); bathw = k; if (tries) { bathEnd = be; bedEnd = RW - 1 - be; sg = -dir; } }
        }
        if (roles & R_BED) { int sh = fz_rnd(2) ? 1 : 0; (void)sh; }
    }
    const int fronty = !(roles & (R_BED | R_BATH));
    int var = fz_rnd(3); if (fronty) var = (RD >= 5 && fz_rnd(3)) ? 0 : 1;               // an open front room: the kitchen takes the back wall (or a side wall when deep)
    for (int j = 0; j < RW - bathw; j++) fz_padd(0, bedEnd + sg * j, back);              // the beds: along the back wall from their end ...
    for (int d = back - 1; d >= (RD + 1) / 2 && d >= 2; d--) fz_padd(0, bedEnd, d);     // ... then down the side wall to the middle
    if (var == 1) { for (int j = 0; j < RW - 1; j++) fz_padd(1, bathEnd - sg * j, back); }   // the kitchen along the back wall from the bathroom's end
    else for (int d = fronty ? back : back - 1; d >= (fronty ? mid : (RD + 1) / 2) && d >= 2; d--) fz_padd(1, bathEnd, d);    // or down the bathroom's side wall
    if (!fronty) {
        for (int d = mid - 1; d >= 1; d--) fz_padd(2, bedEnd, d);                        // the living end of the bedroom's wall
        for (int d = mid - 1; d >= 1; d--) fz_padd(3, bathEnd, d);                       // and the opposite wall: the telly
    } else {                                                                             // an open front room: the whole of the bedroom side's wall, the telly below the kitchen (or on the whole opposite wall)
        for (int d = MIN(mid, back); d <= back; d++) fz_padd(2, bedEnd, d);
        for (int d = MIN(mid, back) - 1; d >= 1; d--) fz_padd(2, bedEnd, d);
        if (var == 1) { for (int d = 1; d <= back - 1; d++) fz_padd(3, bathEnd, d); } else for (int d = mid - 1; d >= 1; d--) fz_padd(3, bathEnd, d);
    }
    if (roles & R_BED) {
        for (int b = 0; b < bd->abeds; b++) { fz_pp(0, 'S'); if (!b) fz_pp(0, 'i'); }
        for (int k = 0; k < bd->bunks; k++) fz_pp(0, '<');
        for (int k = 0; k < bd->cribs; k++) fz_pp(0, '\x81');
        for (int k = 0; k < bd->tbeds; k++) fz_pp(0, fz_rnd(2) ? 'S' : '<');
        if (bd->extra) { fz_pp(0, 'S'); fz_pp(0, 'i'); }
        if (bd->toys) { fz_pp(0, 'n'); if (!fz_rnd(2)) fz_pp(0, '{'); }
        if (area >= 12 && bd->abeds && fz_rnd(3) == 0) fz_pp(0, 'b');
    }
    if (roles & R_KIT) {
        char kit[6] = { 'F', 'y', '/', fz_rnd(2) ? ':' : 'c', 'y', 0 }; if (area < 30) kit[3] = 0; if (area < 18) kit[2] = 0; if (area < 12) kit[1] = 0;
        if (var == 1 && RW >= 7) { kit[4] = 'c'; kit[5] = 0; }                           // (a long run along the back wall)
        for (int j = 0; kit[j]; j++) fz_pp(1, kit[j]);
        if (var == 1) fz_pp(1, '-');
    }
    if (roles & R_LIV) {
        int lvr = o->elders && !o->adults ? ';' : 'C';
        if (roles & R_GRAND) { if (var == 0 && fz_pfree(RW / 2, back)) fz_pput(RW / 2, back, 'o'); else fz_pp(2, 'o'); fz_pp(3, '$'); if (RD >= 5) fz_pp(2, ';'); }
        int sofa = area >= 12 ? fz_pp(2, (char)lvr) : -1;
        if (sofa >= 0) { fz_pp(3, 'v'); fz_pp(2, 'l'); if (fz_rnd(2)) fz_pp(3, 'A'); }
        if (o->elders) { fz_pp(3, 'R'); if (fz_rnd(2)) fz_pp(2, ';'); }
        if (o->teens) { fz_pp(3, fz_rnd(2) ? 'U' : 'A'); fz_pp(2, 'U'); }
        if (RW >= 6 && RD >= 4) {                                                        // the dining table in the middle, chairs on both sides; a rug before the sofa
            int tl = bathEnd == 0 ? 2 : RW - 3, td = RD >= 5 ? RD - 3 : 2;
            if (fz_pfree(tl, td) && fz_pfree(tl - 1, td) && fz_pfree(tl + 1, td)) { fz_pput(tl, td, 't'); fz_pput(tl - 1, td, 'h'); fz_pput(tl + 1, td, 'h'); if ((roles & R_GRAND) && fz_pfree(tl, td + 1) && td + 1 < RD - 1) fz_pput(tl, td + 1, 'h'); }
            int rl = bedEnd == 0 ? 2 : RW - 3, rd = MAX(1, mid - 1);
            if (fz_pfree(rl, rd)) fz_pput(rl, rd, 'r');
        }
    }
    if (roles & R_PLAY) {                                                                // a games room: the jukebox, the tellies, the beanbags; or a home gym
        if (fz_rnd(2)) { fz_pp(0, 'j'); fz_pp(0, 'q'); fz_pp(1, 'v'); fz_pp(1, 'A'); fz_pp(2, 'U'); fz_pp(2, 'U'); fz_pp(3, '!'); }
        else { fz_pp(0, 'm'); fz_pp(0, 'm'); fz_pp(1, ']'); fz_pp(1, ']'); fz_pp(2, '`'); fz_pp(3, 'Q'); }
    }
    if (roles & R_STUDY) {                                                               // the desk, a chair at it, the computer and the shelves
        for (int j = 0; j < RW; j++) if (fz_pfree(j, back) && fz_pfree(j + sg, back)) { fz_pput(j, back, 'd'); fz_pput(j + sg, back, '['); if (RD >= 4 && fz_pfree(j, back - 1)) fz_pput(j, back - 1, 'h'); break; }
        for (int j = RW - 1; j >= 0; j--) if (fz_pfree(j, back)) { fz_pput(j, back, 'b'); break; }
        fz_pp(2, 'b'); fz_pp(3, 'b');
    }
    if ((roles & R_LIV) && (o->adults || o->teens)) { for (int j = 0; j < RW; j++) if (fz_pfree(j, back) && (fz_pfree(j + 1, back) || fz_pfree(j - 1, back)) && !(roles & R_KIT)) { fz_pput(j, back, 'd'); fz_pput(j + 1, back, '['); break; } }
    if ((roles & R_LIV) && o->adults) {                                                  // the hobbies, along the front wall by the door
        static const char hobby[] = "b(`$)qoV?";
        int n = 1 + (area >= 35) + (area >= 48) + ((roles & R_GRAND) ? 1 : 0);
        for (int i = 0; i < n; i++) { char c = hobby[fz_rnd(9)];
            for (int j = 0; j < RW; j++) { int l = (j * 3 + i * 2 + 1) % RW; if (fz_pfree(l, 0) && fz_pfree(l, 1) && l != fz_ld) { fz_pput(l, 0, c); break; } } }
    }
    fz_pput(0, 0, 'p'); fz_pput(RW - 1, 0, 'p'); if (area >= 30) fz_pput(bedEnd, 0, 'p');   // the touches: plants in the front corners
    if (roles & (R_BED | R_STUDY)) { fz_pput(bedEnd, 0, 'p'); if (area >= 9) fz_pput(RW / 2, back - 1 > 1 ? back - 1 : back, 'r'); }
}
static void furnish_at(int x0, int y0, int x1, int y1, int ix, int iy, int ox, int oy, const Occ *o, unsigned h) {   // (ix,iy): the tile just inside the door; (ox,oy): the way in
    fz_frame(x0, y0, x1, y1, ix, iy, ox, oy, h);
    int st = fz_rnd(MAX(1, rgn));
    if (o->vacant) {                                  // nobody lives here: a couple of boxes the last people left behind
        if (fz_rnd(2)) fz_run("#", st);
        if (!fz_rnd(3)) fz_run("p", st + rgn / 2);
        fz_nres = 0; return;
    }
    Beds bd = beds_of(o);
    fz_layout(R_ALL | ((x1 - x0 + 1) * (y1 - y0 + 1) >= 60 ? R_GRAND : 0), &bd, o);
    fz_reach();
    fz_nres = 0;
}
// A BIG HOME is built as rooms: a front room (living, kitchen, dining; split in two in a mansion), and behind a wall with a door to each
// of the back rooms (the bedrooms, the bathroom, a study, a games room). The plan comes from the dice: how wide each room is, which door
// goes where, what each room is for, and what is in it.
static int fz_upx = -1, fz_upy, fz_holex = -1, fz_holey, fz_upper;   // a staircase up from this storey (a plant by it), a stairwell hole in its floor from the one below, and whether it is an upper floor
static void furnish_plan(int X0, int Y0, int X1, int Y1, int dx, const Occ *o, unsigned h, int grand) {
    int Wt = X1 - X0 + 1, Dt = Y1 - Y0 + 1;
    if (o->vacant || Wt < 7 || Dt < 7) { furnish_at(X0, Y0, X1, Y1, dx, Y1, 0, -1, o, h); return; }
    fz_h = h | 1;
    int wpi = NWP + (int)((h >> 5) % 89), br = Dt >= 11 ? 5 : Dt >= 9 ? 4 : 3, wy = Y0 + br;
    int k = Wt >= 11 ? 3 : 2;
    int cuts[3], rx0[3], rx1[3];                                                      // the back rooms' spans
    { int rw = (Wt - (k - 1)) / k, extra = (Wt - (k - 1)) - rw * k, x = X0;
      for (int i = 0; i < k; i++) { int w = rw + (i < extra ? 1 : 0); if (fz_rnd(2) && i < k - 1 && w > 3) w--; rx0[i] = x; rx1[i] = MIN(x + w - 1, X1); x = rx1[i] + 2; if (i < k - 1) cuts[i] = rx1[i] + 1; }
      if (rx1[k - 1] < X1) rx1[k - 1] = X1; }
    for (int x = X0; x <= X1; x++) { sm[wy][x] = 'W'; swp[wy][x] = (u8)wpi; }
    for (int i = 0; i < k - 1; i++) for (int y = Y0; y < wy; y++) { sm[y][cuts[i]] = 'W'; swp[y][cuts[i]] = (u8)wpi; }
    int ddx[3];
    for (int i = 0; i < k; i++) { ddx[i] = (rx0[i] + rx1[i]) / 2; sm[wy][ddx[i]] = 'D'; }   // a door to each room
    for (int i = 0; i < k; i++) { int fl = (int)((fz_rnd(14))); for (int y = Y0; y < wy; y++) for (int x = rx0[i]; x <= rx1[i]; x++) sfl[y][x] = (u8)fl; }
    // who sleeps where
    Beds all = beds_of(o); int roles[3] = { 0, 0, 0 }; Beds rb[3]; memset(rb, 0, sizeof rb);
    int bathAt = fz_rnd(k);                                                           // which room is the bathroom
    int order[3], n = 0; for (int i = 0; i < k; i++) if (i != bathAt) order[n++] = i;
    roles[bathAt] = R_BATH | (grand && k == 3 && fz_rnd(2) ? 0 : 0);
    rb[order[0]].abeds = all.abeds ? (all.abeds > 2 ? 2 : all.abeds) : 1; rb[order[0]].cribs = all.cribs; roles[order[0]] = R_BED;
    int left = all.abeds > 2 ? all.abeds - 2 : 0;
    if (n > 1) {
        int kidsStuff = all.bunks + all.tbeds + all.toys + left;
        if (kidsStuff) { rb[order[1]].bunks = all.bunks; rb[order[1]].tbeds = all.tbeds; rb[order[1]].toys = all.toys; rb[order[1]].abeds = (u8)left; roles[order[1]] = R_BED; }
        else roles[order[1]] = fz_rnd(2) ? (R_STUDY | R_BED) : (grand ? R_PLAY : R_STUDY);   // a study with a guest bed, a games room, or just a study
        if (!rb[order[1]].abeds && (roles[order[1]] & R_BED) && !kidsStuff) rb[order[1]].extra = 1;
    } else { rb[order[0]].bunks = all.bunks; rb[order[0]].tbeds = all.tbeds; rb[order[0]].toys = all.toys; rb[order[0]].abeds = (u8)(rb[order[0]].abeds + left); }
    // the front: one room, or in a mansion a living room and a kitchen with dining beside it
    int fx0 = X0, fx1 = X1, fy0 = wy + 1;
    // the walls are up: now the things in the rooms
    fz_nres = 0;
    for (int i = 0; i < k; i++) { fz_nres = 0; fz_frame(rx0[i], Y0, rx1[i], wy - 1, ddx[i], wy - 1, 0, -1, fz_h + (unsigned)i * 7919u); fz_layout(roles[i], &rb[i], o); }
    fz_nres = 0;
    for (int i = 0; i < k; i++) { static const signed char rs[4][2] = { { 0, 1 }, { -1, 1 }, { 1, 1 }, { 0, 2 } };   // (the way from each door stays open, with room to step round)
        for (int j = 0; j < 4; j++) { fz_res[fz_nres][0] = (u8)(ddx[i] + rs[j][0]); fz_res[fz_nres++][1] = (u8)(wy + rs[j][1]); } }
    Beds none; memset(&none, 0, sizeof none);
    { static const signed char rs2[5][2] = { { 0, 0 }, { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } };   // the stairs keep a clear square round them
      for (int j = 0; j < 5; j++) { if (fz_upx >= 0) { fz_res[fz_nres][0] = (u8)(fz_upx + rs2[j][0]); fz_res[fz_nres++][1] = (u8)(fz_upy + rs2[j][1]); }
                                    if (fz_holex >= 0) { fz_res[fz_nres][0] = (u8)(fz_holex + rs2[j][0]); fz_res[fz_nres++][1] = (u8)(fz_holey + rs2[j][1]); } } }
    fz_frame(fx0, fy0, fx1, Y1, dx, Y1, 0, -1, fz_h * 3u); fz_layout(fz_upper ? (R_LIV | R_PLAY | R_GRAND) : (R_KIT | R_LIV | (grand ? R_GRAND : 0)), &none, o);
    if (fz_upx >= 0) { sm[fz_upy][fz_upx] = '|'; sm[fz_upy - 1][fz_upx] = 'p'; }
    if (fz_holex >= 0) sm[fz_holey][fz_holex] = ' ';
    fz_x0 = X0; fz_y0 = Y0; fz_x1 = X1; fz_y1 = Y1; fz_ix = dx; fz_iy = Y1; fz_ox = 0; fz_oy = -1;
    fz_reach();
    fz_nres = 0;
}
static void furnish(int x0, int y0, int x1, int y1, int dx, int dy, const Occ *o, unsigned h) { furnish_at(x0, y0, x1, y1, dx, y1, 0, -1, o, h); (void)dy; }   // the door in the south wall
static void yard(int lx, int ly, int dx, int dy, int vacant, unsigned h) {   // the front yard: trees, planters, the bin, and a sign if it is for sale
    gFree(dx - 2, dy + 1, (h & 1) ? 'Z' : 'p'); gFree(dx + 2, dy + 1, 'Z');
    gFree(lx + (int)((h >> 3) % 3u), ly + (int)((h >> 5) % 2u), TREE); gFree(lx + 12 - (int)((h >> 7) % 3u), ly + (int)((h >> 9) % 3u), TREE);
    if ((h >> 11) & 1) gFree(lx + 12, ly + 9, TREE);
    if (vacant) gFree(dx + 2, dy + 2, '8'); else gFree(dx + 3, dy + 3, 'Y');
    if (!vacant) {   // the front garden: a lamp by the path, a mailbox at the street, flowers, a gnome or a bird bath, a picket fence with a gate, trees
        unsigned g = h >> 13;
        gFree(dx + 3, dy + 1, 'l'); if (g & 1) gFree(dx - 3, dy + 1, 'N');
        for (int k = -6; k <= 6; k += 2) { int x = dx + k; if (ABS(k) <= 3 || x < lx || x > lx + 12) continue; unsigned r = (g >> (k + 6)) & 3u; gFree(x, dy + 1, r == 0 ? (char)0xE2 : r == 1 ? '*' : r == 2 ? 'p' : 'Z'); }
        if ((g >> 5) & 1) gFree(lx + 12, ly + (int)((g >> 7) % 3u), ((g >> 9) & 1) ? '3' : '6');
        if (dy + 1 < ly + 11) gFree(dx + 1, MIN(ly + 11, dy + 5), (char)0xE0);                                   // the mailbox, by the path near the street
        if ((g >> 10) & 1) gFree(dx - 1, dy + 2, (char)0xE3); else if ((g >> 11) & 1) gFree(dx + 4, dy + 2, (char)0xE4);   // a gnome, or a bird bath
        if ((g >> 12) & 1) gFree(lx + 1, dy + 3, (char)0xEA);                                                  // a barbecue
        if ((g >> 14) % 5u < 3 && dy + 4 <= ly + 11) {                                                          // a picket fence across the front, a gate at the path
            for (int x = lx; x <= lx + 12; x++) if (ABS(x - dx) > 1) gFree(x, dy + 4, (char)0xDF);
            for (int x = lx + 1; x <= lx + 11; x += 2) if (ABS(x - dx) > 2 && ((g >> 16) & 1)) gFree(x, dy + 3, (char)0xE1);   // (and a hedge behind it)
        }
        if ((g >> 17) % 4u == 0 && dy + 1 < ly + 12) for (int y = dy + 1; y <= MIN(ly + 12, MH - 2); y++) floorMap[y][dx] = FL_PATIO;   // a brick path
        else if ((g >> 19) % 5u == 0 && dy + 1 < ly + 12) for (int y = dy + 1; y <= MIN(ly + 12, MH - 2); y++) floorMap[y][dx] = FL_GRAVEL;
    }
}

// ---- the homes ----
static int reg_home(int x0, int y0, int x1, int y1, int dx, int dy);
static int pick_wp(unsigned h) { return ((h >> 16) % 5u) == 0 ? NWP + NWX + (int)((h >> 20) % NWPNEW) : NWP + (int)((h >> 13) % 89); }   // (one home in five: brick, siding, stucco, shingle, beadboard or stone)
static void city_house(int lx, int ly, unsigned h) {
    int grand = ((h >> 24) & 3) == 0;                                       // one in four is a MANSION: the whole lot, rooms and an arch, a fountain out front
    int sz = (int)((h >> 26) & 15);                                          // how big: mansions run from 11x10 up to a rare 13x12 ESTATE (1 in 16); big houses 9..12 wide, 7..10 deep
    int w = grand ? (sz == 0 ? 13 : sz < 5 ? 11 : sz < 9 ? 12 : 13) : 9 + (int)((h >> 3) % 4);
    int hh = grand ? (sz == 0 ? 12 : sz < 5 ? 10 : 11) : 7 + (int)((h >> 5) % 4);
    int hx = lx + (13 - w) / 2, hy = grand ? ly : ly + 1 + (int)((h >> 7) % 2);
    if (!grand && hy + hh - 1 > ly + 10) hy = ly + 11 - hh;
    int fl = (int)((h >> 9) % 14), wp = pick_wp(h);
    gRoom(hx, hy, hx + w - 1, hy + hh - 1, fl, wp);
    int dx = grand ? hx + w / 2 + (int)((h >> 20) % 3u) - 1 : hx + 2 + (int)((h >> 20) % (unsigned)(w - 4)), dy = hy + hh - 1;
    gPut(dx, dy, 'D');
    city_path(dx, dy, ly);
    if (grand) { for (int x = hx + 2; x < hx + w - 2; x += 3) { gPut(x, hy, 'E'); if (x != dx && ABS(x - dx) > 1) gPut(x, dy, 'E'); } for (int y = hy + 2; y < dy - 1; y += 3) { gPut(hx, y, 'E'); gPut(hx + w - 1, y, 'E'); } }
    else { gPut(hx + 2, hy, 'E'); gPut(hx + w - 3, hy, 'E'); gPut(hx, hy + hh / 2, 'E'); gPut(hx + w - 1, hy + hh / 2, 'E');   // windows
      if (dx - 2 > hx) gPut(dx - 2, dy, 'E'); if (dx + 2 < hx + w - 1) gPut(dx + 2, dy, 'E'); }
    int ix0 = hx + 1, iy0 = hy + 1, ix1 = hx + w - 2, iy1 = hy + hh - 2;
    int u = reg_home(ix0, iy0, ix1, iy1, dx, dy);
    int ns = (grand && bm_gen >= 4) ? (sz == 0 ? 3 : sz < 12 ? 2 : 1) : 1;                  // a mansion has an upstairs (the estate two): stairs up from the front room
    int wy_ = iy0 + (iy1 - iy0 + 1 >= 11 ? 5 : (iy1 - iy0 + 1 >= 9 ? 4 : 3)), sty = wy_ + 2, stA = ix0 + 1, stB = ix1 - 1;
    if (ns > 1) { fz_upx = stA; fz_upy = sty; fz_holex = -1; }
    furnish_plan(ix0, iy0, ix1, iy1, dx, occ_of(u), h * 2654435761u, grand);
    fz_upx = fz_holex = -1;
    for (int l = 1; l < ns; l++) {                                                          // the upper floors: the same walls and windows, a suite, a study or a games room, a lounge
        if (!storey(l)) break;
        sRoom(hx, hy, hx + w - 1, hy + hh - 1, (int)(city_h((int)h + l, 5) % 14), wp);
        for (int x = hx + 2; x < hx + w - 2; x += 3) { sm[hy][x] = 'E'; sm[hy + hh - 1][x] = 'E'; }
        for (int y = hy + 2; y < hy + hh - 2; y += 3) { sm[y][hx] = 'E'; sm[y][hx + w - 1] = 'E'; }
        fz_holex = (l & 1) ? stA : stB; fz_holey = sty;
        if (l + 1 < ns) { fz_upx = (l & 1) ? stB : stA; fz_upy = sty; } else fz_upx = -1;
        fz_upper = 1;
        furnish_plan(ix0, iy0, ix1, iy1, hx + w / 2, &occ_any, (h + (unsigned)l * 977u) * 2654435761u, 1);
        fz_upper = 0; fz_upx = fz_holex = -1;
    }
    storey(0); roof_dirty = 1;
    yard(lx, ly, dx, dy, occ_of(u)->vacant, h >> 3);
    if (grand) { gFree(dx - 3, dy + 2, '}'); gFree(dx + 3, dy + 2, '}'); gFree(hx + 1, dy + 1, 'Z'); gFree(hx + w - 2, dy + 1, 'Z'); gFree(dx - 4, dy + 1, TREE); gFree(dx + 4, dy + 1, TREE); }
}
static void city_cottage(int lx, int ly, unsigned h) {   // a small home: one room, everything within reach of the bed
    int w = 7 + (int)((h >> 4) & 1), hh = 6 + (int)((h >> 5) & 1);
    int hx = lx + 3 - (int)((h >> 6) & 1), hy = ly + 3 + (int)((h >> 7) & 1);
    int fl = (int)((h >> 9) % 14), wp = pick_wp(h);
    gRoom(hx, hy, hx + w - 1, hy + hh - 1, fl, wp);
    int dx = hx + w / 2, dy = hy + hh - 1;
    gPut(dx, dy, 'D'); city_path(dx, dy, ly);
    gPut(hx + 2, hy, 'E'); gPut(hx + w - 3, hy, 'E'); gPut(hx, hy + 2, 'E'); gPut(hx + w - 1, hy + 2, 'E');
    int x0 = hx + 1, y0 = hy + 1, x1 = hx + w - 2, y1 = hy + hh - 2;
    int u = reg_home(x0, y0, x1, y1, dx, dy);
    furnish(x0, y0, x1, y1, dx, dy, occ_of(u), h * 2246822519u);
    yard(lx, ly, dx, dy, occ_of(u)->vacant, h >> 5);
}
static void city_town(int lx, int ly, unsigned h) {   // two homes sharing the wall between them, each with its own door, floors and wallpaper
    int hy = ly + 1;
    for (int u = 0; u < 2; u++) {
        unsigned hu = city_h((int)(h & 0xFFFF) + u * 31, u + 9);
        int ax = lx + 6 * u, fl = (int)((hu >> 9) % 14), wp = NWP + (int)((hu >> 13) % 89);
        gRoom(ax, hy, ax + 6, hy + 8, fl, wp);
        int dx = ax + 3, dy = hy + 8;
        gPut(dx, dy, 'D'); city_path(dx, dy, ly);
        gPut(ax + 2, hy, 'E'); gPut(ax + 4, hy, 'E'); gPut(u ? ax + 6 : ax, hy + 4, 'E');                       // the party wall has no window
        int k = reg_home(ax + 1, hy + 1, ax + 5, hy + 7, dx, dy);
        furnish(ax + 1, hy + 1, ax + 5, hy + 7, dx, dy, occ_of(k), hu * 2654435761u);
        gFree(dx - 2, dy + 1, 'Z');                                  // (a townhouse is not sold on its own: no sign)
    }
    gFree(lx + 3, ly, TREE); gFree(lx + 9, ly, TREE); gFree(lx + 6, ly + 12, 'Y');
}
static void city_two(int lx, int ly, unsigned h) {   // a two-storey house: the household downstairs; a loft (spare beds, a desk) up the stairs
    int hx = lx + 2, hy = ly + 1;
    int fl = (int)((h >> 9) % 14), wp = NWP + (int)((h >> 13) % 89), fl2 = (int)((h >> 17) % 14), wp2 = NWP + (int)((h >> 3) % 89);
    gRoom(hx, hy, hx + 8, hy + 7, fl, wp);
    int dx = hx + 4, dy = hy + 7;
    gPut(dx, dy, 'D'); city_path(dx, dy, ly);
    gPut(hx + 2, hy, 'E'); gPut(hx + 6, hy, 'E'); gPut(hx, hy + 3, 'E'); gPut(hx + 8, hy + 3, 'E'); gPut(hx + 2, dy, 'E'); gPut(hx + 6, dy, 'E');
    int x0 = hx + 1, y0 = hy + 1;                                                    // the inside: 7 wide, 6 deep
    int sx = x0 + ((h >> 21) & 1 ? 5 : 1), sy = y0 + 2;                              // the stairs rise to the north: open floor above and below them
    lifeMap[sy][sx] = '|';
    int u = reg_home(x0, y0, x0 + 6, y0 + 5, dx, dy);
    const Occ *o = occ_of(u);
    fz_nres = 2; fz_res[0][0] = (u8)sx; fz_res[0][1] = (u8)(sy - 1); fz_res[1][0] = (u8)sx; fz_res[1][1] = (u8)(sy + 1);
    Occ down = *o; if (!o->vacant && (o->kids || o->teens)) { down.kids = 0; down.teens = 0; }   // the kids sleep upstairs
    furnish(x0, y0, x0 + 6, y0 + 5, dx, dy, &down, h * 2654435761u);
    Level *U = lv_need(0);                                                           // the loft covers the whole footprint
    if (U) {
        for (int j = hy; j <= hy + 7; j++) for (int i = hx; i <= hx + 8; i++) {
            int edge = (i == hx || i == hx + 8 || j == hy || j == hy + 7);
            U->map[j][i] = edge ? 'W' : '.'; U->fl[j][i] = (u8)fl2; U->wp[j][i] = (u8)wp2;
        }
        U->map[hy][hx + 2] = 'E'; U->map[hy][hx + 6] = 'E'; U->map[hy + 3][hx] = 'E'; U->map[hy + 3][hx + 8] = 'E'; U->map[hy + 7][hx + 2] = 'E'; U->map[hy + 7][hx + 6] = 'E';
        U->map[sy][sx] = ' ';                                                        // the stairwell
        if (!o->vacant) {                                                            // the loft: the kids' room, or a spare bed and a desk
            int lx0 = sx > x0 + 3 ? x0 : x0 + 6, step = sx > x0 + 3 ? 1 : -1;
            if (o->kids || o->teens) { U->map[y0][lx0] = '<'; U->map[y0][lx0 + step * 2] = o->teens ? 'S' : '<'; U->map[y0 + 5][lx0] = 'n'; U->map[y0 + 3][lx0 + step * 3] = 'r'; U->map[y0 + 5][lx0 + step * 2] = o->teens ? '[' : '{'; }
            else { U->map[y0][lx0] = 'S'; U->map[y0][lx0 + step * 2] = 'b'; U->map[y0 + 5][lx0] = 'd'; U->map[y0 + 5][lx0 + step] = '['; U->map[y0 + 3][lx0 + step * 3] = 'r'; }
            U->map[y0 + 5][x0 + 3] = 'p';
        }
    }
    roof_dirty = 1;
    yard(lx, ly, dx, dy, o->vacant, h >> 7);
}

// ---- the places ----
static void lot_park(int lx, int ly, unsigned h) {
    gBox(lx + 6, ly, lx + 6, ly + 12, FL_PAVING); gBox(lx, ly + 6, lx + 12, ly + 6, FL_PAVING);
    if (h & 2) { gBox(lx + 2, ly + 2, lx + 4, ly + 4, FL_WATER); gFree(lx + 5, ly + 3, 'N'); }   // a duck pond
    else { gBox(lx + 2, ly + 2, lx + 4, ly + 4, FL_SOIL); gFree(lx + 3, ly + 3, 'p'); }
    gBox(lx + 8, ly + 8, lx + 10, ly + 10, FL_SOIL); gFree(lx + 9, ly + 9, 'p');
    if (h & 4) gPut(lx + 6, ly + 6, '}');
    gFree(lx + 5, ly + 5, 'N'); gFree(lx + 7, ly + 7, 'N'); gFree(lx + 5, ly + 7, 'N'); gFree(lx + 7, ly + 5, 'N');
    for (int i = 0; i < 6; i++) { unsigned r = city_h((int)h + i, i * 7); int x = lx + (int)(r % 13u), y = ly + (int)((r >> 8) % 13u); if (floorMap[y][x] != FL_PAVING) gFree(x, y, TREE); }
    if (h & 1) { gFree(lx + 2, ly + 9, 'K'); gFree(lx + 10, ly + 3, 'K'); }
    if (h & 8) gFree(lx + 10, ly + 10, '_');
    gFree(lx + 6, ly + 12, 'Y');
}
static void city_square(int lx, int ly) {
    gBox(lx, ly, lx + 12, ly + 12, FL_PAVING);
    gPut(lx + 6, ly + 6, '}');
    gFree(lx + 4, ly + 6, 'N'); gFree(lx + 8, ly + 6, 'N'); gFree(lx + 6, ly + 4, 'N'); gFree(lx + 6, ly + 8, 'N');
    gFree(lx + 1, ly + 1, 'Z'); gFree(lx + 11, ly + 1, 'Z'); gFree(lx + 1, ly + 11, 'Z'); gFree(lx + 11, ly + 11, 'Z');
    gFree(lx + 3, ly + 3, TREE); gFree(lx + 9, ly + 3, TREE); gFree(lx + 3, ly + 9, TREE); gFree(lx + 9, ly + 9, TREE);
    gFree(lx + 12, ly + 1, '+'); gFree(lx + 1, ly + 6, 'a'); gFree(lx + 11, ly + 6, 'Y');   // the bus stop, the community flag
}
static void gFace(int x, int y, char c, int dir) { if (x >= 0 && y >= 0 && x < MW && y < MH && lifeMap[y][x] == '.') { lifeMap[y][x] = c; wallMap[y][x] = (u8)(0x80 | dir); } }   // an item turned (0 north 1 west 2 south 3 east: a ramp's lip)
static void city_skate(int lx, int ly) {   // the SKATE PLAZA, life size: quarter pipes round three sides (two tiles deep: 1.65 m), a funbox with a kicker either side,
    gBox(lx, ly, lx + 12, ly + 12, 7);     // a rail, a ledge, a manual pad and a launch ramp; the west side is open to the street
    for (int x = 1; x <= 10; x++) { gFace(lx + x, ly, '5', 0); gFace(lx + x, ly + 1, '5', 0); gFace(lx + x, ly + 12, '5', 2); gFace(lx + x, ly + 11, '5', 2); }
    for (int y = 2; y <= 10; y++) { gFace(lx + 12, ly + y, '5', 3); gFace(lx + 11, ly + y, '5', 3); }
    for (int y = 5; y <= 6; y++) { gFree(lx + 4, ly + y, '4'); for (int x = 5; x <= 7; x++) gFree(lx + x, ly + y, 'X'); gFree(lx + 8, ly + y, '2'); }   // the funbox: up on the west, down on the east
    for (int x = 3; x <= 8; x++) gFree(lx + x, ly + 3, '=');
    for (int x = 3; x <= 8; x++) gFree(lx + x, ly + 9, 'L');
    for (int y = 5; y <= 6; y++) { gFree(lx + 1, ly + y, 'M'); gFree(lx + 2, ly + y, 'M'); }
    gFace(lx + 1, ly + 8, '9', 3); gFace(lx + 2, ly + 8, '9', 3);
    gFree(lx + 12, ly, 'Y'); gFree(lx + 12, ly + 12, 'Y'); gFree(lx, ly, 'k'); gFree(lx + 11, ly, 'Y');
}
static int shell(int lx, int ly, int x0, int y0, int x1, int y1, int fl, int wp, int glass) {   // a place's building: walls, windows, a door to the street
    gRoom(x0, y0, x1, y1, fl, wp);
    int dx = (x0 + x1) / 2; gPut(dx, y1, 'D');
    if (glass) { for (int x = x0 + 1; x < x1; x++) if (x != dx && ((x - dx) & 1)) gPut(x, y1, 'E'); }
    else { gPut(dx - 2, y1, 'E'); gPut(dx + 2, y1, 'E'); }
    gPut(x0 + 2, y0, 'E'); gPut(x1 - 2, y0, 'E');
    gBox(dx, y1 + 1, dx, MIN(ly + 12, MH - 2), FL_PAVING);
    return dx;
}
static void lot_shop(int lx, int ly, unsigned h) {
    int x0 = lx + 1, y0 = ly + 1, x1 = lx + 11, y1 = ly + 9, kind = (int)(h % 5u);
    static const u8 fls[5] = { 3, 1, 10, 6, 4 };
    int dx = shell(lx, ly, x0, y0, x1, y1, fls[kind], NWP + (int)((h >> 9) % 89), 1);
    int ix0 = x0 + 1, iy0 = y0 + 1, ix1 = x1 - 1, iy1 = y1 - 1;
    gFree(ix1 - 1, iy1 - 1, 'y'); gFree(ix1, iy1 - 1, 'y'); gFree(ix1, iy1 - 2, 'I');                // the till
    switch (kind) {
    case 0:   // a grocery: fridges and shelves round the walls, shelves in rows
        for (int x = ix0; x <= ix1; x++) gFree(x, iy0, (x & 1) ? 'F' : '7');
        for (int x = ix0 + 1; x <= ix1 - 2; x++) if (x != dx && x != dx - 1) { gFree(x, iy0 + 2, '7'); gFree(x, iy0 + 4, '7'); }
        break;
    case 1:   // clothes: dressers, mirrors, rugs
        for (int x = ix0; x <= ix1; x += 2) gFree(x, iy0, 'i');
        gFree(ix0, iy0 + 2, '`'); gFree(ix0, iy0 + 4, '`'); gFree(ix0 + 3, iy0 + 3, 'r'); gFree(ix0 + 6, iy0 + 3, 'r'); gFree(ix0 + 3, iy0 + 2, 'i'); gFree(ix0 + 6, iy0 + 2, 'i');
        break;
    case 2:   // electronics: tellies, computers, radios
        for (int x = ix0; x <= ix1; x++) gFree(x, iy0, "v[RAv[RAv"[x - ix0]);
        for (int x = ix0 + 1; x <= ix1 - 2; x += 2) if (x != dx) gFree(x, iy0 + 3, 'y');
        break;
    case 3:   // a furniture showroom
        gFree(ix0, iy0, 'S'); gFree(ix0 + 2, iy0, 'C'); gFree(ix0 + 4, iy0, 'o'); gFree(ix0 + 6, iy0, 'b'); gFree(ix1, iy0, 'i');
        gFree(ix0 + 2, iy0 + 3, 't'); gFree(ix0 + 1, iy0 + 3, 'h'); gFree(ix0 + 3, iy0 + 3, 'h'); gFree(ix0 + 6, iy0 + 3, ';'); gFree(ix0, iy0 + 4, 'l'); gFree(ix0 + 5, iy0 + 4, 'r');
        break;
    default:  // music: pianos, guitars, a jukebox
        gFree(ix0, iy0, '$'); gFree(ix0 + 2, iy0, '$'); for (int x = ix0 + 4; x <= ix1; x++) gFree(x, iy0, '(');
        gFree(ix0, iy0 + 3, 'j'); gFree(ix0 + 3, iy0 + 3, 'A'); gFree(ix0 + 5, iy0 + 3, 'A');
        break;
    }
    gFree(lx + 2, ly + 11, 'N'); gFree(lx + 10, ly + 11, 'Y'); gFree(lx + 1, ly + 12, 'Z');
}
static void city_diner(int lx, int ly, unsigned h) {
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    gRoom(x0, y0, x1, y1, (h & 1) ? 3 : 2, NWP + (h & 2 ? 53 : (int)((h >> 9) % 89))); gPut(lx + 6, y1, 'D'); gPut(lx + 7, y1, 'D');
    gBox(lx + 6, y1 + 1, lx + 7, MIN(ly + 12, MH - 2), FL_PAVING);
    gFree(x0 + 1, y0 + 1, 'F'); gFree(x0 + 2, y0 + 1, '/'); gFree(x0 + 3, y0 + 1, 'y'); gFree(x0 + 4, y0 + 1, 'y'); gFree(x0 + 5, y0 + 1, 'c'); gFree(x1 - 1, y0 + 1, 'T');
    for (int k = 0; k < 2; k++) { int tx = x0 + 2 + k * 4; gFree(tx, y0 + 4, 't'); gFree(tx - 1, y0 + 4, 'h'); gFree(tx + 1, y0 + 4, 'h'); }
    if (h & 4) { gFree(x1 - 2, y0 + 3, 'C'); gFree(x1 - 1, y0 + 4, 't'); }
    gFree(x1 - 1, y1 - 1, (h & 8) ? 'q' : 'j'); gFree(x0 + 1, y1 - 1, (h & 8) ? 'j' : 'p'); gPut(x0 + 3, y0, 'E'); gPut(x1 - 3, y0, 'E');
    gFree(lx + 3, ly + 11, 'Z'); gFree(lx + 10, ly + 11, 'Z');
}
static void city_gym(int lx, int ly, unsigned h) {
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    gRoom(x0, y0, x1, y1, 7, NWP + 61); gPut(lx + 6, y1, 'D'); gBox(lx + 6, y1 + 1, lx + 6, MIN(ly + 12, MH - 2), FL_PAVING);
    for (int x = x0 + 1; x <= x0 + 5; x += 2) gFree(x, y0 + 1, 'm');
    gFree(x0 + 1, y0 + 4, ']'); gFree(x0 + 3, y0 + 4, ']'); if (h & 1) gFree(x0 + 5, y0 + 4, ']'); gFree(x0 + 1, y0 + 6, 'Q');
    gFree(x1 - 1, y0 + 1, 'H'); gFree(x1 - 2, y0 + 1, 'H'); gFree(x1 - 3, y0 + 1, 'T'); gFree(x1 - 1, y1 - 1, 'F'); gFree(x1 - 3, y1 - 1, 'R');
    if (h & 2) gFree(x1 - 1, y0 + 4, '_');
    gPut(x0 + 3, y0, 'E'); gPut(x1 - 3, y0, 'E'); gFree(lx + 3, ly + 11, 'Y'); gFree(lx + 9, ly + 11, 'Z');
}
static void city_office(int lx, int ly, unsigned h) {   // where work is: the work marker
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    gRoom(x0, y0, x1, y1, 10, NWP + (h & 1 ? 61 : (int)((h >> 9) % 89))); gPut(lx + 6, y1, 'D'); gBox(lx + 6, y1 + 1, lx + 6, MIN(ly + 12, MH - 2), FL_PAVING);
    for (int k = 0; k < 3; k++) { int dxk = x0 + 2 + k * 3; gFree(dxk, y0 + 2, 'd'); gFree(dxk, y0 + 3, 'h'); gFree(dxk, y0 + 1, '['); }
    gFree(x1 - 1, y1 - 1, 'x'); gFree(x0 + 1, y1 - 1, 'F'); gFree(x0 + 2, y1 - 1, 'c'); gFree(x0 + 1, y0 + 1, 'b'); gFree(x1 - 1, y0 + 1, 'T');
    gFree(x0 + 5, y1 - 2, 'x'); gFree(x1 - 1, y0 + 3, (h & 2) ? 'q' : 'p'); gPut(x0 + 3, y0, 'E'); gPut(x1 - 3, y0, 'E'); gFree(lx + 3, ly + 11, 'Z');
}
static void lot_inn(int lx, int ly, unsigned h) {   // three rooms off a hall, the lobby by the door
    int x0 = lx, y0 = ly + 1, x1 = lx + 12, y1 = ly + 11, fl = (int)((h >> 5) % 14), fl2 = (int)((h >> 9) % 14), wp = NWP + (int)((h >> 13) % 89);
    gRoom(x0, y0, x1, y1, fl2, wp);
    gBox(x0 + 1, y0 + 1, x1 - 1, y0 + 4, fl);
    gLine(x0, y0 + 5, x1, y0 + 5, 'W', wp); gLine(x0 + 4, y0, x0 + 4, y0 + 5, 'W', wp); gLine(x0 + 8, y0, x0 + 8, y0 + 5, 'W', wp);
    for (int r = 0; r < 3; r++) {
        int rx = x0 + 1 + r * 4; gPut(rx + 1, y0 + 5, 'D'); gPut(rx + 1, y0, 'E');
        gFree(rx, y0 + 1, 'S'); gFree(rx + 2, y0 + 1, (h >> r) & 1 ? 'i' : 'l'); gFree(rx + 2, y0 + 4, (h >> (r + 3)) & 1 ? 'p' : 'v');
    }
    int dx = lx + 6; gPut(dx, y1, 'D'); gPut(dx - 3, y1, 'E'); gPut(dx + 3, y1, 'E');
    gBox(dx, y1 + 1, dx, MIN(ly + 12, MH - 2), FL_PAVING);
    gFree(x1 - 3, y0 + 8, 'y'); gFree(x1 - 2, y0 + 8, 'y'); gFree(x1 - 1, y0 + 8, 'I');           // the front desk
    gFree(x0 + 1, y0 + 9, 'C'); gFree(x0 + 2, y0 + 9, 'C'); gFree(x0 + 1, y0 + 7, ';'); gFree(x0 + 3, y0 + 8, 'r'); gFree(x1 - 1, y0 + 6, 'p'); gFree(x0 + 1, y0 + 6, 'l');
    gFree(lx + 2, ly + 12, 'Z'); gFree(lx + 10, ly + 12, 'Z');
}
static void lot_pool(int lx, int ly, unsigned h) {   // the community pool: a tiled deck, the water, a diving board, ladders, a lifeguard chair, loungers and umbrellas, showers, a hot tub
    gBox(lx, ly, lx + 12, ly + 12, FL_POOLTILE);
    gLine(lx, ly, lx + 12, ly, 'w', 13); gLine(lx, ly, lx, ly + 12, 'w', 13); gLine(lx + 12, ly, lx + 12, ly + 12, 'w', 13); gLine(lx, ly + 12, lx + 12, ly + 12, 'w', 13);
    lifeMap[ly + 12][lx + 6] = '.'; lifeMap[ly + 12][lx + 5] = '.'; floorMap[ly + 12][lx + 6] = FL_PAVING; floorMap[ly + 12][lx + 5] = FL_PAVING;   // the gate
    int pw = 6 + (int)(h & 1), ph = 4 + (int)((h >> 1) & 1);
    gBox(lx + 3, ly + 3, lx + 2 + pw, ly + 2 + ph, FL_WATER);
    for (int x = lx + 3; x <= lx + 2 + pw; x += 2) gFree(x, ly + 4 + ph, '~');
    gFree(lx + 1, ly + 1, 'H'); gFree(lx + 2, ly + 1, 'H'); gFree(lx + 11, ly + 1, '>'); gFree(lx + 11, ly + 3, '~'); gFree(lx + 11, ly + 5, '~');
    gFree(lx + 1, ly + 4, 'Z'); gFree(lx + 1, ly + 8, 'Z'); gFree(lx + 11, ly + 10, 'Y'); gFree(lx + 9, ly + 1, '#'); gFree(lx + 1, ly + 11, 'N');
    gFree(lx + 3 + pw / 2, ly + 2, (char)0xE5);                                  // the diving board, over the deep end
    gFree(lx + 2, ly + 3, (char)0xE6); gFree(lx + 3 + pw, ly + 4, (char)0xE6);  // ladders
    gFree(lx + 10, ly + 8, (char)0xE7);                                          // the lifeguard's chair
    gFree(lx + 4, ly + 5 + ph, (char)0xE8); gFree(lx + 5, ly + 5 + ph, (char)0xE9); gFree(lx + 8, ly + 5 + ph, (char)0xE8);   // umbrellas and towels
    gFree(lx + 11, ly + 8 - (int)(h & 1), (char)0xEA);
}
static void lot_school(int lx, int ly, unsigned h) {   // a classroom, and a playground out front
    int x0 = lx, y0 = ly + 1, x1 = lx + 12, y1 = ly + 8;
    int dx = shell(lx, ly, x0, y0, x1, y1, (int)((h >> 5) % 14), NWP + (int)((h >> 9) % 89), 0);
    for (int r = 0; r < 2; r++) for (int c = 0; c < 4; c++) { int x = x0 + 2 + c * 3, y = y0 + 3 + r * 2; if (x == dx) continue; gFree(x, y, 'd'); gFree(x, y + 1, 'h'); }
    gFree(x0 + 5, y0 + 1, 'd'); gFree(x0 + 6, y0 + 1, '['); gFree(x0 + 1, y0 + 1, 'b'); gFree(x0 + 2, y0 + 1, 'b'); gFree(x1 - 1, y0 + 1, 'n'); gFree(x1 - 2, y0 + 1, 'p');
    gFree(lx + 1, ly + 10, '_'); gFree(lx + 4, ly + 11, 'N'); gFree(lx + 9, ly + 10, 'K'); gFree(lx + 12, ly + 10, 'a'); gFree(lx + 11, ly + 12, 'Y'); gFree(lx + 2, ly + 12, TREE);
}
static void lot_arcade(int lx, int ly, unsigned h) {
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    shell(lx, ly, x0, y0, x1, y1, 7, NWP + (int)((h >> 9) % 89), 0);
    for (int x = x0 + 1; x <= x1 - 1; x++) gFree(x, y0 + 1, '!');
    gFree(x0 + 1, y0 + 3, '!'); gFree(x0 + 1, y0 + 4, '!'); gFree(x1 - 1, y0 + 3, 'j'); gFree(x1 - 1, y0 + 5, 'V');
    gFree(x0 + 4, y0 + 4, '?'); gFree(x0 + 1, y1 - 1, 'C'); gFree(x1 - 2, y1 - 1, 'y'); gFree(x1 - 1, y1 - 1, 'c'); gFree(x0 + 6, y0 + 4, 'U');
    gFree(lx + 2, ly + 11, 'Y');
}
// ---- more places to go: a cafe, a bar, a cinema, a salon, a restaurant, a clinic, a library, a spa, an art gallery ----
static void lot_cafe(int lx, int ly, unsigned h) {   // the counter and its machines along the back, little tables, armchairs by the window
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 4 : 0, NWP + (int)((h >> 9) % 89), 1);
    gFree(x0 + 1, y0 + 1, 'c'); gFree(x0 + 2, y0 + 1, 'c'); gFree(x0 + 3, y0 + 1, 'y'); gFree(x0 + 4, y0 + 1, 'y'); gFree(x0 + 5, y0 + 1, 'F'); gFree(x0 + 6, y0 + 1, '-');
    for (int k = 0; k < 2; k++) { int tx = k ? x1 - 3 : x0 + 2; gFree(tx, y0 + 4, 't'); gFree(tx - 1, y0 + 4, 'h'); gFree(tx + 1, y0 + 4, 'h'); }
    gFree(x1 - 1, y0 + 1, 'b'); gFree(x1 - 2, y0 + 1, '['); gFree(x0 + 1, y1 - 1, ';'); gFree(x0 + 2, y1 - 1, 'l'); gFree(x1 - 1, y1 - 1, ';'); gFree(x1 - 2, y1 - 1, 'p');
    if (h & 2) gFree(x1 - 1, y0 + 3, ')');                                     // (some show a painting)
    gFree(lx + 2, ly + 11, 'K'); gFree(lx + 10, ly + 11, 'Z');
}
static void lot_gas(int lx, int ly, unsigned h) {   // a gas station: the shop at the back, the forecourt in front with its pump islands and the air and water, a sign by the road
    int x0 = lx + 1, y0 = ly + 1, x1 = lx + 11, y1 = ly + 5;
    gBox(lx, ly + 6, lx + 12, ly + 12, FL_ASPHALT);                                   // the forecourt
    int dx = shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 6 : 3, NWP + (int)((h >> 9) % 89), 1);
    gBox(dx, y1 + 1, dx, ly + 7, FL_PAVING);
    gFree(x0 + 1, y0 + 1, 'F'); gFree(x0 + 2, y0 + 1, 'F'); gFree(x0 + 3, y0 + 1, 'c'); for (int x = x0 + 4; x <= x0 + 6; x++) gFree(x, y0 + 1, '7'); gFree(x1 - 1, y0 + 1, ':');
    gFree(x1 - 1, y1 - 1, 'y'); gFree(x1 - 2, y1 - 1, 'y'); gFree(x1 - 1, y1 - 2, 'I'); gFree(x0 + 1, y1 - 1, '7'); gFree(x0 + 2, y1 - 1, '7');   // the till, a rack by the door
    for (int k = 0; k < 2; k++) for (int j = 0; j < 2; j++) { gFree(lx + 2 + k * 8, ly + 8 + j * 2, '\x82'); gFree(lx + 3 + k * 8, ly + 8 + j * 2, '\x82'); }   // four islands of two pumps
    gFree(lx + 1, ly + 12, 'O'); gFree(lx + 11, ly + 7, 'Y'); gFree(lx + 12, ly + 12, 'Z'); gFree(lx + 5, ly + 10, 'J'); gFree(lx + 7, ly + 10, 'J');
    gFree(lx + 12, ly + 6, '"'); gFree(lx, ly + 6, '"');
}
static void lot_conv(int lx, int ly, unsigned h) {   // a corner store: shelves in aisles, the fridges along the wall, the till by the door, a bench and a bin outside
    int x0 = lx + 2, y0 = ly + 3, x1 = lx + 10, y1 = ly + 9;
    int dx = shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 10 : 4, NWP + (int)((h >> 9) % 89), 1);
    int ix0 = x0 + 1, iy0 = y0 + 1, ix1 = x1 - 1, iy1 = y1 - 1;
    for (int x = ix0; x <= ix1; x++) gFree(x, iy0, (x & 1) ? 'F' : '7');
    for (int x = ix0 + 1; x <= ix1 - 2; x++) if (x != dx && x != dx - 1) { gFree(x, iy0 + 2, '7'); gFree(x, iy0 + 3, '7'); }
    gFree(ix1, iy1, 'y'); gFree(ix1 - 1, iy1, 'y'); gFree(ix1, iy1 - 1, 'I'); gFree(ix0, iy1, 'c'); gFree(ix0 + 1, iy1, ':'); gFree(ix0, iy1 - 1, 'p');
    gFree(lx + 1, ly + 10, 'N'); gFree(lx + 11, ly + 10, 'Y'); gFree(lx + 1, ly + 12, 'Z'); gFree(lx + 11, ly + 12, 'Z');
    gBox(lx + 1, ly + 3, lx + 1, ly + 9, FL_PAVING);
}
static void lot_bar(int lx, int ly, unsigned h) {   // the counter and its stools, the back bar behind, a little stage, a booth, the jukebox
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 4 : 13, NWP + (int)((h >> 9) % 89), 0);
    gFree(x0 + 1, y0 + 1, 'F'); gFree(x0 + 5, y0 + 1, 'c');                                        // the back bar (room to stand between)
    for (int x = x0 + 1; x <= x0 + 5; x++) { gFree(x, y0 + 2, 'y'); gFree(x, y0 + 3, 'h'); }       // the counter, the stools
    gFree(x1 - 1, y0 + 1, '$'); gFree(x1 - 2, y0 + 1, '('); gFree(x1 - 3, y0 + 1, 'A');             // the stage
    gBox(x1 - 3, y0 + 2, x1 - 1, y0 + 3, 3);                                                       // (and a dance floor before it)
    gFree(x0 + 1, y1 - 1, 'C'); gFree(x0 + 2, y1 - 1, 't'); gFree(x0 + 3, y1 - 1, 'C');
    gFree(x1 - 1, y1 - 1, 'j'); gFree(x1 - 1, y1 - 2, 'V'); gFree(x1 - 3, y1 - 1, '?');
    gFree(lx + 2, ly + 11, 'Y'); gFree(lx + 10, ly + 11, 'O');
}
static void lot_cinema(int lx, int ly, unsigned h) {   // the screen across the back, two rows of seats facing it (a low wall behind each), the lobby by the door
    int x0 = lx, y0 = ly, x1 = lx + 12, y1 = ly + 11;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 2 : 13, NWP + (int)((h >> 9) % 89), 0);
    for (int x = x0 + 3; x <= x1 - 3; x++) gFree(x, y0 + 1, 'v');                                  // the screen: a wall of screens
    gFree(x0 + 2, y0 + 1, 'A'); gFree(x1 - 2, y0 + 1, 'A');
    for (int r = 0; r < 2; r++) { int y = y0 + 3 + r * 3; for (int x = x0 + 2; x <= x1 - 2; x++) gFree(x, y, ';'); gLine(x0 + 2, y + 1, x1 - 2, y + 1, 'w', 13); }
    gBox(x0 + 1, y0 + 9, x1 - 1, y1 - 1, 3);                                                       // the lobby
    gFree(x0 + 1, y0 + 9, 'y'); gFree(x0 + 2, y0 + 9, 'y'); gFree(x0 + 3, y0 + 9, ':');            // tickets, popcorn
    gFree(x1 - 1, y0 + 9, '!'); gFree(x1 - 2, y0 + 9, '!'); gFree(x1 - 1, y1 - 1, 'p');
    gFree(lx + 2, ly + 12, 'Z'); gFree(lx + 10, ly + 12, 'Z');
}
static void lot_salon(int lx, int ly, unsigned h) {   // mirrors along the back with a chair at each, the basins, the front desk, the waiting chairs
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 5 : 3, NWP + (int)((h >> 9) % 89), 1);
    for (int k = 0; k < 3; k++) { int x = x0 + 1 + k * 2; gFree(x, y0 + 1, '`'); gFree(x, y0 + 2, 'h'); }
    gFree(x0 + 7, y0 + 1, '-'); gFree(x0 + 8, y0 + 1, '-'); gFree(x1 - 1, y0 + 1, 'i');
    gFree(x1 - 2, y1 - 2, 'y'); gFree(x1 - 1, y1 - 2, 'y');
    gFree(x0 + 1, y1 - 1, ';'); gFree(x0 + 2, y1 - 1, ';'); gFree(x0 + 3, y1 - 1, 'p'); gFree(x0 + 1, y1 - 3, 'b'); gFree(x0 + 4, y0 + 4, 'r');
    gFree(lx + 2, ly + 11, 'Z'); gFree(lx + 10, ly + 11, 'Z');
}
static void lot_restaurant(int lx, int ly, unsigned h) {   // the kitchen behind a low wall at the back, tables laid for two, the host's stand by the door
    int x0 = lx, y0 = ly + 1, x1 = lx + 12, y1 = ly + 10;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 4 : 1, NWP + (int)((h >> 9) % 89), 1);
    gBox(x0 + 1, y0 + 1, x1 - 1, y0 + 2, 3);
    gFree(x0 + 1, y0 + 1, 'F'); gFree(x0 + 2, y0 + 1, '/'); gFree(x0 + 3, y0 + 1, '/'); gFree(x0 + 4, y0 + 1, '-'); gFree(x0 + 5, y0 + 1, 'y'); gFree(x1 - 1, y0 + 1, 'F');
    gLine(x0 + 1, y0 + 3, x1 - 4, y0 + 3, 'w', 13);                                               // the pass (open at the right end)
    for (int r = 0; r < 2; r++) for (int c = 0; c < 2; c++) { int tx = c ? x0 + 9 : x0 + 2, ty = y0 + 5 + r * 2;
        gFree(tx, ty, 't'); gFree(tx - 1, ty, 'h'); gFree(tx + 1, ty, 'h'); }
    gFree(x0 + 1, y1 - 1, 'y'); gFree(x0 + 2, y1 - 1, 'p'); gFree(x1 - 1, y1 - 1, 'p'); gFree(x1 - 2, y1 - 1, 'l'); gFree(x0 + 6, y0 + 6, '}');
    gFree(lx + 3, ly + 12, 'Z'); gFree(lx + 9, ly + 12, 'Z');
}
static void lot_clinic(int lx, int ly, unsigned h) {   // two exam rooms at the back, the waiting room and the reception by the door
    int x0 = lx + 1, y0 = ly + 1, x1 = lx + 11, y1 = ly + 10, wp = NWP + (int)((h >> 9) % 89);
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 6 : 11, wp, 0);
    gLine(x0, y0 + 4, x1, y0 + 4, 'W', wp); gLine(x0 + 5, y0, x0 + 5, y0 + 4, 'W', wp);
    gPut(x0 + 2, y0 + 4, 'D'); gPut(x0 + 8, y0 + 4, 'D');
    for (int r = 0; r < 2; r++) { int rx = r ? x0 + 6 : x0 + 1; gFree(rx, y0 + 1, 'S'); gFree(rx + 2, y0 + 1, '-'); gFree(rx + 3, y0 + 1, 'd'); gFree(rx + 3, y0 + 2, 'h'); gFree(rx, y0 + 3, 'p'); }
    gFree(x1 - 1, y0 + 6, 'y'); gFree(x1 - 2, y0 + 6, 'y'); gFree(x1 - 1, y0 + 5, '[');
    for (int y = y0 + 6; y <= y1 - 1; y++) gFree(x0 + 1, y, ';');
    gFree(x0 + 3, y1 - 1, 'p'); gFree(x0 + 4, y0 + 7, 'r'); gFree(x0 + 1, y0 + 5, 'b');
    gFree(lx + 2, ly + 12, 'Z'); gFree(lx + 10, ly + 12, 'Z');
}
static void lot_library(int lx, int ly, unsigned h) {   // the stacks, a reading table, computers, armchairs and lamps, the front desk
    int x0 = lx, y0 = ly + 1, x1 = lx + 12, y1 = ly + 10;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 1 : 4, NWP + (int)((h >> 9) % 89), 0);
    for (int r = 0; r < 2; r++) for (int x = x0 + 1; x <= x0 + 5; x++) gFree(x, y0 + 1 + r * 2, 'b');
    for (int x = x1 - 5; x <= x1 - 1; x++) gFree(x, y0 + 1, 'b');
    gFree(x1 - 4, y0 + 4, 't'); gFree(x1 - 5, y0 + 4, 'h'); gFree(x1 - 3, y0 + 4, 'h');
    gFree(x0 + 1, y0 + 6, '['); gFree(x0 + 2, y0 + 6, 'h'); gFree(x0 + 1, y0 + 8, '['); gFree(x0 + 2, y0 + 8, 'h');
    gFree(x1 - 1, y1 - 1, ';'); gFree(x1 - 2, y1 - 1, 'l'); gFree(x1 - 1, y1 - 3, ';'); gFree(x0 + 4, y1 - 1, 'y'); gFree(x0 + 3, y1 - 1, 'y'); gFree(x0 + 8, y0 + 6, 'r');
    gFree(lx + 2, ly + 12, 'N'); gFree(lx + 10, ly + 12, 'Z');
}
static void lot_space(int lx, int ly, unsigned h) {   // the SPACE CENTRE: mission control (consoles, a briefing desk) and the launch pad with its rocket
    int x0 = lx + 1, y0 = ly + 1, x1 = lx + 11, y1 = ly + 6, wp = NWP + (int)((h >> 9) % 89);
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 10 : 6, wp, 1);
    for (int x = x0 + 1; x <= x1 - 1; x += 2) { gFree(x, y0 + 1, (char)0xD9); gFree(x, y0 + 2, 'h'); }
    gFree(x0 + 1, y1 - 1, '['); gFree(x1 - 1, y1 - 1, '['); gFree(x0 + 5, y1 - 1, 'd');
    gBox(lx, ly + 8, lx + 12, ly + 12, FL_PAVING); gBox(lx + 4, ly + 9, lx + 8, ly + 12, FL_ROCK);
    gFree(lx + 6, ly + 10, (char)0xD8); gFree(lx + 2, ly + 9, 'O'); gFree(lx + 10, ly + 9, 'O');
}
static void lot_obs(int lx, int ly, unsigned h) {   // the OBSERVATORY: the public scopes (a hobby scope, then a research telescope and a dome for the certified), a radio dish out the back
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9, wp = NWP + (int)((h >> 9) % 89);
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 4 : 5, wp, 0);
    gFree(x0 + 2, y0 + 2, (char)0xD4); gFree(x0 + 5, y0 + 2, (char)0xD5); gFree(x1 - 2, y0 + 2, (char)0xD6);
    gFree(x0 + 1, y0 + 5, 'b'); gFree(x0 + 1, y0 + 6, 'b'); gFree(x1 - 1, y0 + 5, '['); gFree(x1 - 1, y0 + 6, 'h'); gFree(x0 + 4, y1 - 1, 'y');
    gFree(lx + 2, ly + 11, (char)0xD7); gFree(lx + 10, ly + 11, (char)0xD3); gFree(lx + 6, ly + 11, 'N');
}
static void lot_spa(int lx, int ly, unsigned h) {   // the massage room, a sauna's showers, two hot tubs, a fountain, the front desk
    int x0 = lx + 1, y0 = ly + 1, x1 = lx + 11, y1 = ly + 10, wp = NWP + (int)((h >> 9) % 89);
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 6 : 5, wp, 0);
    gLine(x0 + 5, y0, x0 + 5, y0 + 4, 'W', wp); gLine(x0, y0 + 4, x0 + 5, y0 + 4, 'W', wp); gPut(x0 + 3, y0 + 4, 'D');
    gFree(x0 + 1, y0 + 1, 'S'); gFree(x0 + 3, y0 + 1, 'S'); gFree(x0 + 1, y0 + 3, 'p'); gFree(x0 + 4, y0 + 3, 'l');
    gFree(x0 + 6, y0 + 1, 'H'); gFree(x0 + 7, y0 + 1, 'H'); gFree(x0 + 7, y0 + 3, '>'); gFree(x1 - 1, y0 + 3, '>');
    gFree(x0 + 1, y1 - 1, 'y'); gFree(x0 + 2, y1 - 1, 'y'); gFree(x0 + 7, y0 + 6, '}');
    gFree(x1 - 1, y1 - 1, ';'); gFree(x1 - 2, y1 - 1, ';'); gFree(x0 + 1, y0 + 6, 'p'); gFree(x1 - 1, y0 + 5, 'p');
    gFree(lx + 2, ly + 12, 'Z'); gFree(lx + 10, ly + 12, 'Z');
}
static void lot_gallery(int lx, int ly, unsigned h) {   // paintings round the walls, sculptures, a fountain, benches, the desk
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 7 : 4, NWP + (int)((h >> 9) % 89), 1);
    for (int x = x0 + 1; x <= x1 - 1; x += 2) gFree(x, y0 + 1, ')');
    gFree(x0 + 1, y0 + 3, ')'); gFree(x0 + 1, y0 + 5, ')'); gFree(x1 - 1, y0 + 3, ')'); gFree(x1 - 1, y0 + 5, ')');
    gFree(x0 + 3, y0 + 4, '*'); gFree(x1 - 3, y0 + 4, '*'); gFree(x0 + 5, y0 + 4, '}');
    gFree(x0 + 3, y0 + 3, 'r'); gFree(x1 - 3, y0 + 3, 'r');
    gFree(x1 - 2, y1 - 1, 'y'); gFree(x1 - 1, y1 - 1, 'b'); gFree(x0 + 1, y1 - 1, 'p'); gFree(x0 + 2, y1 - 1, 'N');
    gFree(lx + 2, ly + 11, 'Z'); gFree(lx + 10, ly + 11, 'Z');
}
static void lot_empty(int lx, int ly, unsigned h) {   // nothing built yet: rough ground, junk, the sign
    for (int i = 0; i < 6; i++) { unsigned r = city_h((int)h + i * 3, i); int x = lx + (int)(r % 13u), y = ly + (int)((r >> 8) % 13u), w = 1 + (int)((r >> 16) % 3u);
        gBox(x, y, MIN(lx + 12, x + w), MIN(ly + 12, y + w), (r >> 20) & 1 ? FL_SOIL : FL_ROCK); }
    gFree(lx + 6, ly + 11, '8');
    unsigned r = h >> 3;
    if (r & 1) { gFree(lx + 2, ly + 3, '#'); gFree(lx + 3, ly + 3, '#'); }
    if (r & 2) { gFree(lx + 9, ly + 5, 'O'); gFree(lx + 10, ly + 6, 'O'); }
    if (r & 4) gFree(lx + 4, ly + 8, '*');
    if (r & 8) gFree(lx + 11, ly + 1, TREE);
}
static void lot_farm(int lx, int ly, unsigned h) {   // rows of crops, a barn, a fence
    for (int y = ly + 1; y <= ly + 11; y += 2) gBox(lx + 1, y, lx + 6, y, FL_SOIL);
    for (int y = ly + 2; y <= ly + 10; y += 2) for (int x = lx + 1; x <= lx + 6; x += 2) gFree(x, y, (char)0xDB);   // (FARM LIFE: crop beds)
    gRoom(lx + 8, ly + 1, lx + 12, ly + 6, 9, NWP + (int)((h >> 9) % 89));
    gPut(lx + 10, ly + 6, 'D'); gFree(lx + 9, ly + 2, (char)0xDC); gFree(lx + 9, ly + 4, (char)0xDC); gFree(lx + 11, ly + 2, (char)0xDD); gFree(lx + 11, ly + 4, (char)0xDD); gFree(lx + 11, ly + 3, 'O');
    gLine(lx, ly + 12, lx + 12, ly + 12, 'w', 9); lifeMap[ly + 12][lx + 7] = '.';
    gFree(lx + 9, ly + 9, 'K'); gFree(lx + 11, ly + 10, TREE); gFree(lx + 8, ly + 11, (char)0xDE);
}

// what stands on lot (i,j): the landmarks are fixed, the rest of the 5x5 grid is a hand-placed mix of homes so no two neighbours match
// ---- homes for the island and the town: the same generators, on free ground ----
static int area_free(int x0, int y0, int x1, int y1, int sand_only) {
    if (x0 < 2 || y0 < 2 || x1 > MW - 3 || y1 > MH - 3) return 0;
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        char c = lifeMap[y][x]; int f = floorMap[y][x];
        if (c != '.' && c != 'u' && c != '*' && c != 'g') return 0;     // bare ground, or a cactus / rock / palm we may clear
        if (f == FL_WATER || (sand_only && f != FL_SAND)) return 0;
    }
    return 1;
}
static void area_clear(int x0, int y0, int x1, int y1) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) lifeMap[y][x] = '.'; }
static void town_homes(int want) {   // lots on the 14-tile grid wherever the desert is empty: a random house, cottage, townhouse pair or two-storey on each
    for (int j = 0; j < 5 && nhomes[ZONE_TOWN] < want; j++) for (int i = 0; i < 5 && nhomes[ZONE_TOWN] < want; i++) {
        int lx = 3 + 14 * i, ly = 3 + 14 * j;
        if (!area_free(lx, ly, lx + 12, ly + 12, 1)) continue;
        area_clear(lx, ly, lx + 12, ly + 12);
        unsigned h = city_h(i * 11 + 3, j * 17 + 1); int t = (int)(h % 10);
        if (t < 3) city_house(lx, ly, h); else if (t < 5) city_cottage(lx, ly, h); else if (t < 8) city_town(lx, ly, h); else city_two(lx, ly, h);
    }
}
static int ring_clear(int x0, int y0, int x1, int y1) {   // no home wall in the one-tile ring round the box (so two homes never touch)
    for (int y = y0 - 1; y <= y1 + 1; y++) for (int x = x0 - 1; x <= x1 + 1; x++) {
        if (x < 0 || y < 0 || x >= MW || y >= MH) return 0;
        char c = lifeMap[y][x]; if (c == 'W' || c == 'D' || c == 'E') return 0;
    }
    return 1;
}
static void island_homes(int want) {   // cottages (with a plank path) where the island has room, clear of the camp
    for (int ly = 4; ly <= MH - 16 && nhomes[ZONE_ISLAND] < want; ly += 2) for (int lx = 4; lx <= MW - 16 && nhomes[ZONE_ISLAND] < want; lx += 2) {
        unsigned h = city_h(lx * 7 + 1, ly * 13 + 5) & ~(1u << 7);
        int x0 = lx + 3, y0 = ly + 3, x1 = lx + 9, y1 = ly + 9;
        if (ABS(lx + 6 - isl_islet_x) < 16 && ABS(ly + 6 - isl_islet_y) < 16) continue;   // (not out on the islet)
        if (!area_free(x0, y0, x1, y1, 0) || !ring_clear(x0, y0, x1, y1)) continue;   // dry bare ground, no neighbour touching
        area_clear(x0, y0, x1, y1);
        city_cottage(lx, ly, h);
        {   // an islander's hut: plank walls and floor
            static int wp = -1; if (wp < 0) { wp = 0; for (int i = 0; i < NWALLS; i++) if (!strcmp(world_wall_name(i), "WOOD PANEL")) wp = i; }
            for (int y = ly; y < ly + 13 && y < MH; y++) for (int x = lx; x < lx + 13 && x < MW; x++) {
                if (is_wall(lifeMap[y][x]) || lifeMap[y][x] == 'D') wallMap[y][x] = (u8)wp;
                else if (floorMap[y][x] < NFL) floorMap[y][x] = 4;
            }
        }
    }
}
// ---- the tall ones: blocks of flats, office towers, hotels (storeys of them, stairs up through each) ----
static void stairs_up(int l, int st, int x, const int *ys, int nys) {   // flights alternate between the spots in ys: storey l's goes up, the one from below arrives
    if (l + 1 < st) sm[ys[l % nys]][x] = '|';
    if (l > 0) sm[ys[(l - 1) % nys]][x] = ' ';
}
static void lot_apart(int lx, int ly, unsigned h, int st) {   // two flats a storey either side of a stair hall (two wide: the stairs take one side); the ground floor's open onto the street
    int x0 = lx, x1 = lx + 12, y0 = ly + 1, y1 = ly + 10, hx = lx + 5;   // the hall: hx (the way through) and hx + 1 (the stairs)
    int flh = (int)((h >> 3) % 14), wpo = NWP + (int)((h >> 7) % 89);
    int ys[2] = { y0 + 3, y0 + 6 };
    for (int l = 0; l < st; l++) {
        if (!storey(l)) break;
        sRoom(x0, y0, x1, y1, flh, wpo);
        sLine(hx - 1, y0, hx - 1, y1, 'W', wpo); sLine(hx + 2, y0, hx + 2, y1, 'W', wpo);
        for (int y = y0 + 1; y < y1; y++) { sm[y][hx] = '.'; sm[y][hx + 1] = '.'; }
        sm[y0][lx + 2] = 'E'; sm[y0][lx + 10] = 'E'; sm[y0 + 4][x0] = 'E'; sm[y0 + 4][x1] = 'E'; sm[y0][hx] = 'E';
        if (l) { sm[y1][lx + 2] = 'E'; sm[y1][lx + 10] = 'E'; sm[y1][hx] = 'E'; sm[y1][hx + 1] = 'E'; }
        if (l + 1 < st) { sm[ys[l & 1]][hx + 1] = '|'; sm[ys[l & 1] - 1][hx + 1] = 'p'; }   // up: the stairs rise north (a plant blocks the top end down here)
        if (l > 0) sm[ys[(l - 1) & 1]][hx + 1] = ' ';                                       // the stairwell from below
        for (int side = 0; side < 2; side++) {
            int fx0 = side ? hx + 3 : x0 + 1, fx1 = side ? x1 - 1 : hx - 2, fy0 = y0 + 1, fy1 = y1 - 1, u = l * 2 + side;
            unsigned hu = city_h((int)(h & 0xFFFF) + l * 7 + side, l * 3 + side + 11);
            int ffl = (int)((hu >> 9) % 14);
            for (int y = fy0; y <= fy1; y++) for (int x = fx0; x <= fx1; x++) sfl[y][x] = (u8)ffl;
            blk_unit = u;
            if (l == 0) {                                  // the street door
                int dx = side ? lx + 10 : lx + 2;
                sm[y1][dx] = 'D'; city_path(dx, y1, ly);
                reg_home(fx0, fy0, fx1, fy1, dx, y1);
                furnish(fx0, fy0, fx1, fy1, dx, y1, occ_of(u), hu * 2654435761u);
            } else {                                       // the door off the hall
                int dy = fy1 - 1;
                sm[dy][side ? hx + 2 : hx - 1] = 'D';
                furnish_at(fx0, fy0, fx1, fy1, side ? fx0 : fx1, dy, side ? 1 : -1, 0, occ_of(u), hu * 2654435761u);
            }
        }
        if (l == 0) { sm[y1][hx] = 'D'; city_path(hx, y1, ly); }
    }
    storey(0); blk_unit = 2 * st; roof_dirty = 1;
    gFree(lx, ly + 12, TREE); gFree(lx + 12, ly + 12, TREE); gFree(lx + 4, ly + 12, 'Y'); gFree(lx + 8, ly + 11, 'Z');
}
static void lot_tower(int lx, int ly, unsigned h, int st) {   // an office tower: a lobby, then floor after floor of desks
    int x0 = lx + 1, x1 = lx + 11, y0 = ly + 1, y1 = ly + 10, sx = x1 - 2;
    int fl = (int)((h >> 3) % 14), wp = NWP + (int)((h >> 7) % 89), fl2 = (int)((h >> 11) % 14);
    int ys[2] = { y0 + 2, y0 + 5 };
    for (int l = 0; l < st; l++) {
        if (!storey(l)) break;
        sRoom(x0, y0, x1, y1, l ? fl2 : fl, wp);
        for (int x = x0 + 1; x < x1; x++) if ((x - x0) & 1) { sm[y0][x] = 'E'; if (l) sm[y1][x] = 'E'; }   // glass all round
        for (int y = y0 + 1; y < y1; y++) if ((y - y0) & 1) { sm[y][x0] = 'E'; sm[y][x1] = 'E'; }
        stairs_up(l, st, sx, ys, 2);
        fz_nres = 0;
        if (l == 0) {                                      // the lobby: the front desk, sofas, the work markers
            int dx = (x0 + x1) / 2; sm[y1][dx] = 'D'; sm[y1][dx - 1] = 'E'; sm[y1][dx + 1] = 'E'; city_path(dx, y1, ly);
            sFree(x0 + 2, y0 + 3, 'y'); sFree(x0 + 3, y0 + 3, 'y'); sFree(x0 + 4, y0 + 3, 'I'); sFree(x0 + 3, y0 + 1, 'b');
            sFree(x0 + 1, y1 - 2, 'C'); sFree(x0 + 1, y1 - 3, 'C'); sFree(x0 + 2, y1 - 1, 'p'); sFree(x1 - 1, y1 - 1, 'p');
            sFree(x0 + 6, y0 + 2, 'x'); sFree(x0 + 6, y0 + 5, 'x'); sFree(x0 + 1, y0 + 1, 'c'); sFree(x0 + 5, y1 - 2, 'r');
        } else {                                           // a floor of desks
            for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) {
                int x = x0 + 1 + c * 2 + (c > 0), y = y0 + 2 + r * 2;
                if (x >= sx - 1 || y >= y1 - 1) continue;
                sFree(x, y, 'd'); sFree(x, y + 1, 'h'); if ((h >> (r * 3 + c)) & 1) sFree(x + 1, y, '[');
            }
            sFree(x1 - 1, y1 - 1, (l & 1) ? '-' : 'F'); sFree(x0 + 1, y1 - 1, 'p'); sFree(x1 - 1, y0 + 1, (l & 1) ? 'b' : '#'); sFree(x0 + 1, y0 + 1, 'p');
        }
    }
    storey(0); roof_dirty = 1;
    gFree(lx + 1, ly + 12, 'Z'); gFree(lx + 11, ly + 12, 'Z'); gFree(lx + 3, ly + 12, 'N'); gFree(lx + 9, ly + 12, 'Y');
}
static void lot_hotel(int lx, int ly, unsigned h, int st) {   // a hotel: a corridor (two wide) through each floor, rooms either side, the lobby at the front
    int x0 = lx, x1 = lx + 12, y0 = ly, y1 = ly + 12;
    int fl = (int)((h >> 3) % 14), fl2 = (int)((h >> 9) % 14), wp = NWP + (int)((h >> 13) % 89);
    int xs[2] = { x0 + 3, x0 + 9 };
    for (int l = 0; l < st; l++) {
        if (!storey(l)) break;
        sRoom(x0, y0, x1, y1, fl2, wp);
        sLine(x0, y0 + 4, x1, y0 + 4, 'W', wp); sLine(x0, y0 + 7, x1, y0 + 7, 'W', wp);
        for (int x = x0 + 1; x < x1; x++) { sm[y0 + 5][x] = '.'; sm[y0 + 6][x] = '.'; sfl[y0 + 5][x] = (u8)fl; sfl[y0 + 6][x] = (u8)fl; }
        for (int r = 0; r < 3; r++) { int rx = x0 + 1 + r * 4;
            if (r) { sLine(rx - 1, y0, rx - 1, y0 + 4, 'W', wp); sLine(rx - 1, y0 + 7, rx - 1, y1, 'W', wp); }
            sm[y0][rx + 1] = 'E'; if (l) sm[y1][rx + 1] = 'E';
            for (int side = 0; side < 2; side++) {
                int ry0 = side ? y0 + 8 : y0 + 1, ry1 = side ? y1 - 1 : y0 + 3;
                if (l == 0 && side && r == 1) continue;     // the lobby
                sm[side ? y0 + 7 : y0 + 4][rx + 1] = 'D';
                unsigned hr = city_h((int)(h & 0xFFF) + r * 5 + side + l * 11, r);
                int by = side ? ry1 : ry0, fy = side ? ry0 : ry1;
                sFree(rx, by, 'S'); sFree(rx + 2, by, (hr & 1) ? 'i' : 'l'); sFree(rx + 2, fy, (hr & 2) ? 'v' : 'p');
                if (side && (hr & 4)) { sFree(rx, fy, 'T'); }
            }
        }
        if (l + 1 < st) { int x = xs[l & 1]; sm[y0 + 6][x] = '|'; sm[y0 + 6][x - 1] = 'p'; }   // up: the stairs rise west (a plant blocks the top end down here)
        if (l > 0) sm[y0 + 6][xs[(l - 1) & 1]] = ' ';
        if (l == 0) {                                      // the lobby, through the front door
            int dx = x0 + 6;
            sm[y0 + 7][dx] = 'D'; sm[y1][dx] = 'D'; sm[y1][dx - 1] = 'E'; sm[y1][dx + 1] = 'E';
            for (int y = y0 + 8; y < y1; y++) for (int x = x0 + 5; x <= x0 + 7; x++) sfl[y][x] = (u8)fl;
            sFree(x0 + 5, y0 + 8, 'y'); sFree(x0 + 7, y0 + 8, 'I'); sFree(x0 + 5, y1 - 1, 'p'); sFree(x0 + 7, y1 - 1, ';');
        }
    }
    storey(0); roof_dirty = 1;
}

// ================================================================== SAN BOREAS: the window
// The 80x80 map is a window of 5x5 blocks onto the open world, the player always in its middle block. Walk into the next block
// and the window moves a block that way: everything slides over (the map, the storeys, the homes, the people), and the row of
// blocks coming into view is built from the generator. A block you built on is kept aside when it leaves, and comes back as you
// left it.
#define WB 4
int world_wbx = -1, world_wby = -1;                    // the window's top-left block in the open world (-1: not made yet)
typedef struct { u16 bx, by; char map[BM_BLK][BM_BLK]; u8 fl[BM_BLK][BM_BLK], wp[BM_BLK][BM_BLK]; } EditBlk;
static EditBlk edit_small[12]; static EditBlk *edits = edit_small; static int nedit_max = 12, nedit, edit_next;
// the family's own lot (home.c buys it): the whole block, every storey, kept just as they build it
typedef struct { u16 bx, by; u8 used, fresh, lv, home; u8 hx0, hy0, hx1, hy1, hdx, hdy;
                 char map[NLEVELS][BM_BLK][BM_BLK]; u8 fl[NLEVELS][BM_BLK][BM_BLK], wp[NLEVELS][BM_BLK][BM_BLK]; } OwnBlk;
static OwnBlk own; static Occ own_occ;                 // (fresh: bought, not yet built: the generator makes it, furnished for the family)
#pragma GCC pop_options
static void edit_init(void) {
    if (ram8 && edits == edit_small) { EditBlk *e = (EditBlk *)xalloc(sizeof(EditBlk) * 1024); if (e) { edits = e; nedit_max = 1024; } }
    nedit = 0; edit_next = 0;
}
static EditBlk *edit_find(int bx, int by) { for (int i = 0; i < nedit; i++) if (edits[i].bx == bx && edits[i].by == by) return &edits[i]; return 0; }
COLD static void edit_save(int wx, int wy) {
    int bx = world_wbx + wx, by = world_wby + wy, ox = wx * BM_BLK, oy = wy * BM_BLK;
    EditBlk *e = edit_find(bx, by);
    if (!e) { if (nedit < nedit_max) e = &edits[nedit++]; else { e = &edits[edit_next]; edit_next = (edit_next + 1) % nedit_max; } }
    e->bx = (u16)bx; e->by = (u16)by;
    for (int y = 0; y < BM_BLK; y++) { memcpy(e->map[y], &lifeMap[oy + y][ox], BM_BLK); memcpy(e->fl[y], &floorMap[oy + y][ox], BM_BLK); memcpy(e->wp[y], &wallMap[oy + y][ox], BM_BLK); }
}
static int edit_load(int wx, int wy) {
    EditBlk *e = edit_find(world_wbx + wx, world_wby + wy); if (!e) return 0;
    int ox = wx * BM_BLK, oy = wy * BM_BLK;
    for (int y = 0; y < BM_BLK; y++) { memcpy(&lifeMap[oy + y][ox], e->map[y], BM_BLK); memcpy(&floorMap[oy + y][ox], e->fl[y], BM_BLK); memcpy(&wallMap[oy + y][ox], e->wp[y], BM_BLK); }
    return 1;
}
static void mark_dirty(int l, int mx, int my) { if (world_zone == ZONE_CITY && l <= 0 && mx >= 0 && my >= 0 && mx < MW && my < MH) wdirty[my / BM_BLK][mx / BM_BLK] = 1; }
static int own_at(int wx, int wy) { return own.used && world_wbx + wx == own.bx && world_wby + wy == own.by; }
static int own_in_window(int *wx, int *wy) {
    if (!own.used || world_zone != ZONE_CITY || world_wbx < 0) return 0;
    int x = own.bx - world_wbx, y = own.by - world_wby;
    if (x < 0 || y < 0 || x >= WB || y >= WB) return 0;
    *wx = x; *wy = y; return 1;
}
static void own_save(int wx, int wy) {   // the window's lot into the keep (the ground and the storeys over it)
    int ox = wx * BM_BLK, oy = wy * BM_BLK;
    for (int y = 0; y < BM_BLK; y++) { memcpy(own.map[0][y], &lifeMap[oy + y][ox], BM_BLK); memcpy(own.fl[0][y], &floorMap[oy + y][ox], BM_BLK); memcpy(own.wp[0][y], &wallMap[oy + y][ox], BM_BLK); }
    own.lv = 0;
    for (int k = 0; k < NUP; k++) {
        const Level *L = lv_get(k); int any = 0;
        for (int y = 0; y < BM_BLK; y++) {
            if (L) { memcpy(own.map[k + 1][y], &L->map[oy + y][ox], BM_BLK); memcpy(own.fl[k + 1][y], &L->fl[oy + y][ox], BM_BLK); memcpy(own.wp[k + 1][y], &L->wp[oy + y][ox], BM_BLK); }
            else memset(own.map[k + 1][y], ' ', BM_BLK);
            for (int x = 0; x < BM_BLK; x++) if (own.map[k + 1][y][x] != ' ') any = 1;
        }
        if (any) own.lv |= (u8)(1 << k);
    }
}
static void own_load(int wx, int wy) {   // ... and back
    int ox = wx * BM_BLK, oy = wy * BM_BLK;
    for (int y = 0; y < BM_BLK; y++) { memcpy(&lifeMap[oy + y][ox], own.map[0][y], BM_BLK); memcpy(&floorMap[oy + y][ox], own.fl[0][y], BM_BLK); memcpy(&wallMap[oy + y][ox], own.wp[0][y], BM_BLK); }
    for (int k = 0; k < NUP; k++) {
        if (!(own.lv & (1 << k))) continue;
        Level *L = lv_need(k); if (!L) continue;
        for (int y = 0; y < BM_BLK; y++) { memcpy(&L->map[oy + y][ox], own.map[k + 1][y], BM_BLK); memcpy(&L->fl[oy + y][ox], own.fl[k + 1][y], BM_BLK); memcpy(&L->wp[oy + y][ox], own.wp[k + 1][y], BM_BLK); }
    }
    roof_dirty = 1;
}
static void homes_drop(int bx, int by) {   // a block made again: its homes are registered afresh
    int z = ZONE_CITY, n = 0;
    for (int i = 0; i < nhomes[z]; i++) if (homes[z][i].bx != bx || homes[z][i].by != by) homes[z][n++] = homes[z][i];
    nhomes[z] = n;
}
static void gen_block(int wx, int wy);
COLD static void block_again(int wx, int wy) { homes_drop(world_wbx + wx, world_wby + wy); gen_block(wx, wy); ao_dirty = 1; roof_dirty = 1; world_stamp++; }
COLD void world_own_set(int bx, int by, const u8 *occ) {   // the family's new lot (occ: adults, kids, teens, elders, a couple, babies)
    own.used = 1; own.fresh = 1; own.bx = (u16)bx; own.by = (u16)by; own.lv = 0; own.home = 0;
    memset(&own_occ, 0, sizeof own_occ);
    own_occ.adults = occ[0]; own_occ.kids = occ[1]; own_occ.teens = occ[2]; own_occ.elders = occ[3]; own_occ.couple = occ[4]; own_occ.babies = occ[5];
    int wx, wy; if (own_in_window(&wx, &wy)) block_again(wx, wy);   // (standing there: it is made for them now)
}
void world_own_clear(void) {   // sold: the block is the generator's again
    int wx, wy, in = own_in_window(&wx, &wy);
    own.used = 0;
    if (in) block_again(wx, wy);
}
int world_own_lot(int tx, int ty) {   // that tile of the window is on the family's own lot (not the street round it)
    int wx, wy; if (!own_in_window(&wx, &wy)) return 0;
    int lx = tx - wx * BM_BLK, ly = ty - wy * BM_BLK;
    return lx >= BM_LOT0 && ly >= BM_LOT0 && lx < BM_BLK && ly < BM_BLK;
}
int world_own_door(int *tx, int *ty) {   // where to stand on getting home: by the front door (an empty lot: its middle). 0: not in the window
    int wx, wy; if (!own_in_window(&wx, &wy)) return 0;
    int ox = wx * BM_BLK, oy = wy * BM_BLK;
    if (own.home) { *tx = ox + own.hdx; *ty = oy + own.hdy + 1; } else { *tx = ox + BM_LOT0 + 6; *ty = oy + BM_LOT0 + 6; }
    return 1;
}
int world_own_block(int *wx, int *wy) { return own_in_window(wx, wy); }

static int blk_roads(int bx, int by) {   // which streets a block has (bit 0 along x, bit 1 along y), as the block's own generator will paint them
    BInfo nb; bm_block(bx, by, &nb);
    int mg = nb.lot != LOT_RIVER && (nb.road & BR_MEGA), mpart = BR_PART(nb.road), rd = nb.road & 7;
    if (mg) { if (mpart & 1) rd &= ~2; if (mpart & 2) rd &= ~1; }
    return rd;
}
static void wild(int ox, int oy, const BInfo *b, int bx, int by) {   // the open country: trees, rocks, cacti, palms
    for (int y = 0; y < BM_BLK; y++) for (int x = 0; x < BM_BLK; x++) {
        { u8 f = floorMap[oy + y][ox + x]; if (f == FL_PAVING || f == FL_WATER || f >= FL_ASPHALT || lifeMap[oy + y][ox + x] != '.') continue; }
        int r = (int)(bm_hash(bx * BM_BLK + x, by * BM_BLK + y, wsalt ^ 0x77u) % 1000u); char c = 0;
        switch (b->terrain) {
            case BT_GRASS:  c = r < 22 ? '3' : r < 28 ? '*' : r < 30 ? '6' : 0; break;
            case BT_FOREST: c = r < 70 ? '6' : r < 95 ? '3' : r < 100 ? '*' : 0; break;
            case BT_DESERT: c = r < 20 ? 'u' : r < 38 ? '*' : 0; break;
            case BT_BEACH:  c = r < 18 ? 'g' : r < 24 ? '*' : r < 27 ? (char)0xE8 : r < 29 ? (char)0xE9 : r < 30 ? (char)0xE7 : 0; break;
            case BT_ROCKY:  c = r < 80 ? '*' : r < 95 ? '6' : 0; break;
        }
        if (c) lifeMap[oy + y][ox + x] = c;
    }
}
// ---- a river across a town, and the big shared places (a park, a mall, a downtown plaza: four blocks made one) ----
COLD static void lot_river(int ox, int oy, int vert, unsigned h) {   // the lot is a river with a walk down each bank; the street along the edge crosses it as a bridge
    int T = vert ? 0 : 1;   // (draw west-east, mirrored for north-south through x/y swap)
    const int SH = BM_LOT0 - 3, NB = BM_BLK - SH;   // (the river was drawn for a 16-tile block with a 3-tile street: slid over by the wider street)
    #define RX(a, b) (vert ? ox + (a) + SH : ox + (b) + SH)
    #define RY(a, b) (vert ? oy + (b) + SH : oy + (a) + SH)
    for (int d = 3; d < NB; d++) {                 // d: along the river's width (across), e: along its length
        int fl = d <= 4 ? FL_PAVING : d <= 11 ? FL_WATER : d <= 13 ? FL_PAVING : FL_GRASS;
        for (int e = 3; e < NB; e++) floorMap[RY(d, e)][RX(d, e)] = (u8)fl;
    }
    for (int e = -SH; e < 3; e++) for (int d = 5; d <= 11; d++) floorMap[RY(d, e)][RX(d, e)] = FL_ASPHALT;   // (under the street: the bridge)
    for (int e = 4; e < NB; e += 4) {              // lamps and benches down both walks, planters, and trees on the far bank
        gFree(RX(3, e), RY(3, e), '"'); gFree(RX(13, e + 2), RY(13, e + 2), '"');
        gFree(RX(4, e + 2), RY(4, e + 2), 'N'); gFree(RX(12, e), RY(12, e), 'N');
        gFree(RX(15, e), RY(15, e), TREE); if (e + 2 < NB) gFree(RX(14, e + 2), RY(14, e + 2), TREE);
    }
    if (h & 1) gFree(RX(12, 9), RY(12, 9), 'Z'); if (h & 2) gFree(RX(4, 6), RY(4, 6), 'Z');
    if (h & 4) { gPut(RX(7, 9), RY(7, 9), 'K'); }      // (a rock in the stream, if a rock is 'K': the item the park uses)
    gFree(RX(3, 1), RY(3, 1), '"'); gFree(RX(13, 1), RY(13, 1), '"');
    (void)T;
    #undef RX
    #undef RY
}
COLD static void mega_zone(int ox, int oy, int part, int kind, unsigned h) {   // the strips that were streets between the four blocks: a plaza (or a green) now
    int fl = kind == MK_PARK ? FL_GRASS : FL_PAVING, S = BM_LOT0, C = S / 2, E = S - 1;
    if (part & 1) gBox(ox, oy + ((part & 2) ? 0 : S), ox + E, oy + BM_BLK - 1, (u8)fl);       // the strip down the left
    if (part & 2) gBox(ox + ((part & 1) ? 0 : S), oy, ox + BM_BLK - 1, oy + E, (u8)fl);      // the strip along the top
    if (kind == MK_PARK) {
        if (part & 1) gBox(ox + C, oy + ((part & 2) ? 0 : S), ox + C, oy + BM_BLK - 1, FL_PAVING);
        if (part & 2) gBox(ox + ((part & 1) ? 0 : S), oy + C, ox + BM_BLK - 1, oy + C, FL_PAVING);
    }
    if (part & 1) for (int y = oy + ((part & 2) ? 2 : S + 1); y < oy + BM_BLK; y += 4) { gFree(ox, y, kind == MK_PARK ? TREE : 'Z'); gFree(ox + E, y, kind == MK_PARK ? TREE : 'Z'); gFree(ox + C, y + 2, 'N'); gFree(ox + E, y + 2, '"'); }
    if (part & 2) for (int x = ox + ((part & 1) ? 2 : S + 1); x < ox + BM_BLK; x += 4) { gFree(x, oy, kind == MK_PARK ? TREE : 'Z'); gFree(x, oy + E, kind == MK_PARK ? TREE : 'Z'); gFree(x + 2, oy + C, 'N'); gFree(x + 2, oy + E, '"'); }
    if (part == 3) {                                    // the middle of the four: a pond in a park, a fountain in a plaza, with the benches round it
        if (kind == MK_PARK) { gBox(ox, oy, ox + E, oy + E, FL_WATER); gFree(ox + C, oy + C, 'N'); }
        else { gPut(ox + C, oy + C, '}'); gFree(ox, oy, 'N'); gFree(ox + E, oy, 'N'); gFree(ox, oy + E, 'N'); gFree(ox + E, oy + E, 'N'); }
    }
    (void)h;
}
COLD static void gen_block(int wx, int wy) {
    int bx = world_wbx + wx, by = world_wby + wy, ox = wx * BM_BLK, oy = wy * BM_BLK;
    BInfo b; bm_block(bx, by, &b);
    static const u8 tfl[6] = { FL_WATER, FL_SAND, FL_GRASS, FL_GRASS, FL_SAND, FL_ROCK };
    int ground = tfl[b.terrain];
    if (b.set != 255 && b.terrain != BT_SEA) ground = (b.terrain == BT_DESERT || b.terrain == BT_BEACH) ? FL_SAND : FL_GRASS;
    TREE = (b.terrain == BT_FOREST || b.terrain == BT_ROCKY) ? '6' : b.terrain == BT_DESERT ? 'u' : b.terrain == BT_BEACH ? 'g' : '3';
    for (int y = 0; y < BM_BLK; y++) for (int x = 0; x < BM_BLK; x++) { lifeMap[oy + y][ox + x] = '.'; floorMap[oy + y][ox + x] = (u8)ground; wallMap[oy + y][ox + x] = 0; }
    for (int k = 0; k < NUP; k++) { Level *L = lv_get(k); if (L) for (int y = 0; y < BM_BLK; y++) memset(&L->map[oy + y][ox], ' ', BM_BLK); }
    int mg = b.lot != LOT_RIVER && (b.road & BR_MEGA), mpart = BR_PART(b.road), mkind = BR_KIND(b.road);
    int rd = b.road & 7; if (mg) { if (mpart & 1) rd &= ~2; if (mpart & 2) rd &= ~1; }   // (inside a big place the streets between its blocks are gone)
    const int SW = BM_LOT0, SC = SW / 2;   // the street: five tiles wide, a lane of two each way and the line down the middle
    if (rd & 1) { gBox(ox, oy, ox + BM_BLK - 1, oy + SW - 1, FL_ASPHALT); gBox(ox + SW, oy + SC, ox + BM_BLK - 1, oy + SC, FL_LINE_H); }
    if (rd & 2) { gBox(ox, oy, ox + SW - 1, oy + BM_BLK - 1, FL_ASPHALT); gBox(ox + SC, oy + SW, ox + SC, oy + BM_BLK - 1, FL_LINE_V); }
    if ((rd & 3) == 1) gBox(ox, oy + SC, ox + SW - 1, oy + SC, FL_LINE_H);
    if ((rd & 3) == 2) gBox(ox + SC, oy, ox + SC, oy + SW - 1, FL_LINE_V);
    {   // the street furniture: a white line down each edge (none through a junction), and a crosswalk on each arm
        int east = (rd & 1) ? (blk_roads(bx + 1, by) & 2) : 0, south = (rd & 2) ? (blk_roads(bx, by + 1) & 1) : 0;
        if (rd & 1) {
            int xs = (rd & 2) ? 6 : 0, xe = east ? BM_BLK - 2 : BM_BLK - 1;
            gBox(ox + xs, oy, ox + xe, oy, FL_EDGE_N); gBox(ox + xs, oy + SW - 1, ox + xe, oy + SW - 1, FL_EDGE_S);
            if (rd & 2) gBox(ox + 5, oy, ox + 5, oy + SW - 1, FL_ZEB_A);
            if (east) gBox(ox + BM_BLK - 1, oy, ox + BM_BLK - 1, oy + SW - 1, FL_ZEB_A);
        }
        if (rd & 2) {
            int ys = (rd & 1) ? 6 : 0, ye = south ? BM_BLK - 2 : BM_BLK - 1;
            gBox(ox, oy + ys, ox, oy + ye, FL_EDGE_W); gBox(ox + SW - 1, oy + ys, ox + SW - 1, oy + ye, FL_EDGE_E);
            if (rd & 1) gBox(ox, oy + 5, ox + SW - 1, oy + 5, FL_ZEB_B);
            if (south) gBox(ox, oy + BM_BLK - 1, ox + SW - 1, oy + BM_BLK - 1, FL_ZEB_B);
        }
    }
    if (b.set != 255 && b.terrain != BT_SEA) {           // in town: a pavement along the street
        gBox(ox + BM_LOT0, oy + BM_LOT0, ox + BM_BLK - 1, oy + BM_LOT0, FL_PAVING); gBox(ox + BM_LOT0, oy + BM_LOT0, ox + BM_LOT0, oy + BM_BLK - 1, FL_PAVING);
    }
    if (b.set != 255 && b.terrain != BT_SEA && !mg) {   // bus stops along the pavement: the bigger the place, the more of them (a city has two on every street side, a town one on most, a village one here and there)
        int k = bm_set(b.set)->kind, nr = k == ST_CITY ? 2 : k == ST_TOWN ? ((b.h & 3) != 0) : (((b.h >> 2) % 3) == 0), nc = k == ST_CITY ? 2 : k == ST_TOWN ? ((b.h & 12) != 0) : (((b.h >> 4) % 4) == 0);
        if (rd & 1) for (int i = 0; i < nr; i++) gFace(ox + 7 + i * 6 + (int)((b.h >> (6 + i * 3)) % 3), oy + BM_LOT0, '+', 0);   // (the pole on the street side)
        if (rd & 2) for (int i = 0; i < nc; i++) gFace(ox + BM_LOT0, oy + 7 + i * 6 + (int)((b.h >> (12 + i * 3)) % 3), '+', 1);
    }
    if (mg && mpart) mega_zone(ox, oy, mpart, mkind, b.h);
    int mine = own.used && bx == own.bx && by == own.by;
    if (mine && !own.fresh) {                            // the family's own lot: as they left it
        own_load(wx, wy);
        if (own.home) { blk_bx = bx; blk_by = by; blk_unit = 0; reg_home(ox + own.hx0, oy + own.hy0, ox + own.hx1, oy + own.hy1, ox + own.hdx, oy + own.hdy); blk_bx = blk_by = 0xFFFF; }
        wdirty[wy][wx] = 0; return;
    }
    blk_bx = bx; blk_by = by; blk_unit = 0;
    int units = bm_units(&b), st = bm_storeys(&b);
    occ_on = 1;
    for (int u = 0; u < 12; u++) {
        Occ *o = &occ_blk[u]; memset(o, 0, sizeof *o); o->vacant = 1;
        if (u >= units) continue;
        int f = pop_family_at(bx, by, u); if (f < 0) continue;
        u8 a, k, t, e, c; pop_family_kinds(f, &a, &k, &t, &e, &c);
        o->vacant = 0; o->adults = a; o->kids = k; o->teens = t; o->elders = e; o->couple = c;
    }
    if (mine) occ_blk[0] = own_occ;                      // (just bought: furnished for the family)
    int lx = ox + BM_LOT0, ly = oy + BM_LOT0; unsigned h = b.h;
#ifdef AUTO_TOWERS
    if (b.lot != LOT_WILD && b.lot != LOT_SQUARE && b.lot != LOT_PARK && b.lot != LOT_RIVER) { b.lot = (by & 1) ? LOT_TOWER : LOT_HOTEL; st = 6; }   // (test builds: every lot a six-storey tower or hotel)
#endif
#ifdef AUTO_HOUSES
    if (b.lot != LOT_WILD && b.lot != LOT_SQUARE && b.lot != LOT_PARK && b.lot != LOT_RIVER) b.lot = LOT_HOUSE;   // (test builds: every lot a house)
#endif
    switch (b.lot) {
        case LOT_HOUSE:   city_house(lx, ly, h); break;
        case LOT_COTTAGE: city_cottage(lx, ly, h); break;
        case LOT_TOWN:    city_town(lx, ly, h); break;
        case LOT_TWO:     city_two(lx, ly, h); break;
        case LOT_SHOP:    lot_shop(lx, ly, h); break;
        case LOT_DINER:   city_diner(lx, ly, h); break;
        case LOT_GYM:     city_gym(lx, ly, h); break;
        case LOT_OFFICE:  city_office(lx, ly, h); break;
        case LOT_INN:     lot_inn(lx, ly, h); break;
        case LOT_POOL:    lot_pool(lx, ly, h); break;
        case LOT_SCHOOL:  lot_school(lx, ly, h); break;
        case LOT_ARCADE:  lot_arcade(lx, ly, h); break;
        case LOT_PARK:    lot_park(lx, ly, h); break;
        case LOT_SQUARE:  city_square(lx, ly); break;
        case LOT_SKATE:   city_skate(lx, ly); break;
        case LOT_EMPTY:   lot_empty(lx, ly, h); break;
        case LOT_FARM:    lot_farm(lx, ly, h); break;
        case LOT_APART:   lot_apart(lx, ly, h, st); break;
        case LOT_TOWER:   lot_tower(lx, ly, h, st); break;
        case LOT_HOTEL:   lot_hotel(lx, ly, h, st); break;
        case LOT_CAFE:    lot_cafe(lx, ly, h); break;
        case LOT_BAR:     lot_bar(lx, ly, h); break;
        case LOT_CINEMA:  lot_cinema(lx, ly, h); break;
        case LOT_SALON:   lot_salon(lx, ly, h); break;
        case LOT_RESTAURANT: lot_restaurant(lx, ly, h); break;
        case LOT_CLINIC:  lot_clinic(lx, ly, h); break;
        case LOT_LIBRARY: lot_library(lx, ly, h); break;
        case LOT_SPA:     lot_spa(lx, ly, h); break;
        case LOT_GALLERY: lot_gallery(lx, ly, h); break;
        case LOT_GAS:     lot_gas(lx, ly, h); break;
        case LOT_CONV:    lot_conv(lx, ly, h); break;
        case LOT_SPACE:   lot_space(lx, ly, h); break;
        case LOT_OBS:     lot_obs(lx, ly, h); break;
        case LOT_RIVER:   lot_river(ox, oy, b.road & BR_MEGA, h); break;
        default: break;
    }
    occ_on = 0; blk_bx = blk_by = 0xFFFF;
    if (b.set == 255 || b.lot == LOT_WILD) wild(ox, oy, &b, bx, by);   // (open country, and the green gaps left in a town)
    if (b.set != 255 && b.terrain != BT_SEA) {                      // street lamps on the corners and half way along, a bin now and then
        gFree(ox + 3, oy + 3, '"'); gFree(ox + 10, oy + 3, '"'); gFree(ox + 3, oy + 10, '"');
        if ((h >> 24) % 3u == 0) gFree(ox + 6, oy + 3, 'Y');
    }
    if (mine) {                                          // the family's lot, made for the first time: no sign now; kept from here on
        for (int y = oy; y < oy + BM_BLK; y++) for (int x = ox; x < ox + BM_BLK; x++) if (lifeMap[y][x] == '8') lifeMap[y][x] = '.';
        if (b.lot == LOT_EMPTY)                          // an empty lot: cleared for building (the junk carted off, the ground levelled; the trees round its edge stay)
            for (int y = ly + 1; y <= ly + 11; y++) for (int x = lx + 1; x <= lx + 11; x++) {
                lifeMap[y][x] = '.'; if (floorMap[y][x] == FL_SOIL || floorMap[y][x] == FL_ROCK) floorMap[y][x] = (u8)ground; }
        int hi = world_home_find(ZONE_CITY, bx, by, 0);
        if (hi >= 0) { const Home *hm = &homes[ZONE_CITY][hi]; own.home = 1;
            own.hx0 = (u8)(hm->x0 - ox); own.hy0 = (u8)(hm->y0 - oy); own.hx1 = (u8)(hm->x1 - ox); own.hy1 = (u8)(hm->y1 - oy); own.hdx = (u8)(hm->dx - ox); own.hdy = (u8)(hm->dy - oy); }
        own.fresh = 0; own_save(wx, wy); wdirty[wy][wx] = 0; return;
    }
    wdirty[wy][wx] = (u8)edit_load(wx, wy);
}
static void row_from(u8 *a, int stride, int w, int h, int y, int ys, int dx, u8 fill) {
    u8 *d = a + y * stride;
    if (ys < 0 || ys >= h) { memset(d, fill, w); return; }
    u8 *s = a + ys * stride;
    if (dx >= 0) { wmove(d, s + dx, w - dx); memset(d + w - dx, fill, dx); }
    else { wmove(d - dx, s, w + dx); memset(d, fill, -dx); }
}
COLD static void shift2d(u8 *a, int stride, int w, int h, int dx, int dy, u8 fill) {   // a[y][x] = a[y + dy][x + dx]
    if (dy >= 0) for (int y = 0; y < h; y++) row_from(a, stride, w, h, y, y + dy, dx, fill);
    else for (int y = h - 1; y >= 0; y--) row_from(a, stride, w, h, y, y + dy, dx, fill);
}
static void roof_shift(int dx, int dy); static void ao_region(int X0, int Y0, int X1, int Y1);
static u8 rq_dirty[WB][WB]; static int rq_n;                  // the blocks that have come into the window, their roofs still to work out
static u8 ao[MH + 1][MW + 1];
extern int dbg_ws[4];
static void window_shift(int sx, int sy) {
    int T0 = gfx_hb(), T; world_stamp++;
    for (int wy = 0; wy < WB; wy++) for (int wx = 0; wx < WB; wx++) {          // the blocks leaving: keep what was built on
        int nx = wx - sx, ny = wy - sy;
        if (nx >= 0 && ny >= 0 && nx < WB && ny < WB) continue;
        if (own_at(wx, wy)) own_save(wx, wy); else if (wdirty[wy][wx]) edit_save(wx, wy);
    }
    int dx = sx * BM_BLK, dy = sy * BM_BLK;
    shift2d((u8 *)lifeMap, MW + 1, MW, MH, dx, dy, '.');
    shift2d((u8 *)floorMap, MW, MW, MH, dx, dy, FL_GRASS);
    shift2d((u8 *)wallMap, MW, MW, MH, dx, dy, 0);
    for (int k = 0; k < NUP; k++) { Level *L = lv_get(k); if (!L) continue;
        shift2d((u8 *)L->map, MW + 1, MW, MH, dx, dy, ' '); shift2d((u8 *)L->fl, MW, MW, MH, dx, dy, 0); shift2d((u8 *)L->wp, MW, MW, MH, dx, dy, 0); }
    shift2d((u8 *)wdirty, WB, WB, WB, sx, sy, 0);
    world_wbx += sx; world_wby += sy;
    int z = ZONE_CITY, n = 0;                                                     // the homes that stay, moved over
    for (int i = 0; i < nhomes[z]; i++) {
        Home hm = homes[z][i];
        if (hm.bx < world_wbx || hm.by < world_wby || hm.bx >= world_wbx + WB || hm.by >= world_wby + WB) continue;
        hm.x0 = (u8)(hm.x0 - dx); hm.x1 = (u8)(hm.x1 - dx); hm.dx = (u8)(hm.dx - dx);
        hm.y0 = (u8)(hm.y0 - dy); hm.y1 = (u8)(hm.y1 - dy); hm.dy = (u8)(hm.dy - dy);
        homes[z][n++] = hm;
    }
    nhomes[z] = n;
    T = gfx_hb(); if (T - T0 > dbg_ws[0]) dbg_ws[0] = T - T0; T0 = T;
    // what was worked out for the blocks that stay moves with them: the shading by the walls, the roofs and buildings
    { int keep_r = roof_dirty, keep_a = ao_dirty;
      shift2d(&ao[0][0], MW + 1, MW + 1, MH + 1, dx, dy, 0);
      if (!keep_r) roof_shift(dx, dy);
      T = gfx_hb(); if (T - T0 > dbg_ws[1]) dbg_ws[1] = T - T0; T0 = T;
      shift2d(&rq_dirty[0][0], WB, WB, WB, sx, sy, 0);
      for (int wy = 0; wy < WB; wy++) for (int wx = 0; wx < WB; wx++) {          // the blocks coming into view
          int ox = wx + sx, oy = wy + sy;
          if (ox < 0 || oy < 0 || ox >= WB || oy >= WB) { gen_block(wx, wy); rq_dirty[wy][wx] = 1; }
      }
      T = gfx_hb(); if (T - T0 > dbg_ws[2]) dbg_ws[2] = T - T0; T0 = T;
      rq_n = 0; for (int wy = 0; wy < WB; wy++) for (int wx = 0; wx < WB; wx++) rq_n += rq_dirty[wy][wx];
      for (int wy = 0; wy < WB; wy++) for (int wx = 0; wx < WB; wx++) if (rq_dirty[wy][wx]) ao_region(wx * BM_BLK, wy * BM_BLK, wx * BM_BLK + BM_BLK, wy * BM_BLK + BM_BLK);
      T = gfx_hb(); if (T - T0 > dbg_ws[3]) dbg_ws[3] = T - T0; T0 = T;
      roof_dirty = keep_r; ao_dirty = keep_a; }                                  // (the lots' own builders flag the whole map)
    for (int y = 0; y < MH; y++) lifeMap[y][MW] = 0;
}
static void window_keep(void) {                  // what was built in the window, kept aside before it goes
    if (world_zone != ZONE_CITY || world_wbx < 0) return;
    for (int wy = 0; wy < WB; wy++) for (int wx = 0; wx < WB; wx++) { if (own_at(wx, wy)) own_save(wx, wy); else if (wdirty[wy][wx]) edit_save(wx, wy); }
}
COLD void world_open_at(int cbx, int cby) {          // the window round block (cbx, cby): a new arrival, a taxi ride
    window_keep();
    world_stamp++;
    world_wbx = CLAMP(cbx - WB / 2, 0, BM_N - WB); world_wby = CLAMP(cby - WB / 2, 0, BM_N - WB);
    nhomes[ZONE_CITY] = 0;
    levels_clear(UP);
    memset(wdirty, 0, sizeof wdirty);
    for (int y = 0; y < MH; y++) lifeMap[y][MW] = 0;
    for (int wy = 0; wy < WB; wy++) for (int wx = 0; wx < WB; wx++) gen_block(wx, wy);
    ao_dirty = 1; roof_dirty = 1;
}
int world_stream(int ptx, int pty, int *dtx, int *dty) {
    *dtx = *dty = 0;
    if (world_zone != ZONE_CITY || world_wbx < 0 || ptx < 0 || pty < 0 || ptx >= MW || pty >= MH) return 0;
    int bxp = ptx / BM_BLK, byp = pty / BM_BLK, sx = (bxp >= WB - 1) - (bxp <= 0), sy = (byp >= WB - 1) - (byp <= 0);   // (in one of the two middle blocks: the window stays; further out it moves)
    if (world_wbx + sx < 0 || world_wbx + sx > BM_N - WB) sx = 0;
    if (world_wby + sy < 0 || world_wby + sy > BM_N - WB) sy = 0;
    if (!sx && !sy) return 0;
    window_shift(sx, sy);
    *dtx = sx * BM_BLK; *dty = sy * BM_BLK;
    return 1;
}
void world_arrival(int set, int *px, int *py) {  // where a bus or a taxi drops you in a town: the street by its square
    const Settle *s = bm_set(set); if (!s) s = bm_set(0);
    if (!s) { *px = MW / 2; *py = MH / 2; return; }
    world_open_at(s->x, s->y);
    *px = (s->x - world_wbx) * BM_BLK + 9; *py = (s->y - world_wby) * BM_BLK + BM_LOT0 / 2;
}
int world_block_info(int wx, int wy, BInfo *b) { if (world_wbx < 0) return 0; bm_block(world_wbx + wx, world_wby + wy, b); return 1; }
void world_new_seed(u32 seed) { wsalt = seed; world_wbx = world_wby = -1; edit_init(); own.used = 0; }
void world_gen_city(void) { nhomes[ZONE_CITY] = 0; world_wbx = world_wby = -1; }   // (the window is built where you arrive: world_arrival, world_open_at)

COLD void world_switch(int z) {
    ao_dirty = 1;
    if (z == world_zone) return;
    if (world_zone == ZONE_CITY) {                    // leaving SAN BOREAS: what you built is kept aside; the window (and its storeys) is let go
        window_keep();
        levels_clear(lvown[ZONE_CITY]); world_wbx = world_wby = -1; nhomes[ZONE_CITY] = 0;
    } else {
        zone_keep(world_zone);
    }
    world_zone = z; UP = lvown[z]; roof_dirty = 1; world_stamp++;
    if (z == ZONE_CITY) { world_gen_city(); return; }
    int made = zp_made[z];
    if (!made) levels_clear(UP);
    if (made) { zone_restore(z); zone_drop(z); }   // (back: the live tiles are the place now, its keep let go)
    else if (z == ZONE_ISLAND) world_gen_island();
    else if (z == ZONE_TOWN) world_gen_town();
    else if (z == ZONE_CITY) world_gen_city();
    else world_gen();
}
void world_reset_all(void) {
    for (int i = 0; i < ZONE_N; i++) nhomes[i] = 0;
    ao_dirty = 1; for (int i = 0; i < ZONE_N; i++) { if (i < ZONE_CITY) zone_drop(i); levels_clear(lvown[i]); } world_zone = ZONE_HOME; UP = lvown[0]; world_gen(); }
char world_at(int mx, int my) { return world_at_l(world_lvl, mx, my); }
static int is_wall(char c) { return c == 'W' || c == 'w' || c == 'E' || c == 'e' || c == 'f'; }
int world_walkable(int mx, int my) {
    char c = world_at(mx, my);
    if (is_wall(c) || c == ' ') return 0;                       // (' ': open air on an upper storey)
    if (world_lvl == 0 && mx >= 0 && my >= 0 && mx < MW && my < MH && floorMap[my][mx] == FL_WATER) return 0;
    if (c == '.' || c == 'D' || c == 'P' || c == 'B') return 1;
    return !items_blocking(c);
}
int world_floor_y(int mx, int my) { (void)mx; (void)my; return 0; }
void world_find(char ch, int *mx, int *my) {
    for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) if (lifeMap[y][x] == ch) { *mx = x; *my = y; return; }
    *mx = -1; *my = -1;
}
int world_count(char ch) { int n = 0; for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) if (lifeMap[y][x] == ch) n++; return n; }
static int item_open(int x, int y) { if (x < 0 || y < 0 || x >= MW || y >= MH) return 0; char c = world_at(x, y); return c == '.' || c == 'D' || c == 'B' || c == 'P' || c == 'r'; }
// the arrays the drawing reads: the ground's, or (while a storey above is drawn) that storey's - pointed at, never copied
static char (*dm)[MW + 1] = lifeMap; static u8 (*dfl)[MW] = floorMap, (*dwp)[MW] = wallMap;
int world_item_facing(int x, int y) {
    static const signed char dx[4] = { 0, 1, 0, -1 }, dy[4] = { 1, 0, -1, 0 };
    char c = world_at(x, y);
    if (items_rotates(c) && x >= 0 && y >= 0 && x < MW && y < MH) {   // a turn the player chose in build mode: bit 7 of the item's (otherwise unused) wall cell, bits 0-1 the facing
        u8 w; if (world_lvl <= 0) w = wallMap[y][x]; else { const Level *Lv = lv_get(world_lvl - 1); w = Lv ? Lv->wp[y][x] : 0; }
        if (w & 0x80) return w & 3;
    }
    if (c == 'h' || c == ';')                     // a seat at a table, a desk, a counter, a mirror or a computer faces it
        for (int d = 0; d < 4; d++) { char n = world_at(x + dx[d], y + dy[d]); if (n == 't' || n == 'd' || n == 'y' || n == '`' || n == '[' || n == '?') return d; }
    if (c == '[' || c == 'd' || c == '`')         // ... and the desk, the computer or the mirror faces its chair
        for (int d = 0; d < 4; d++) if (world_at(x + dx[d], y + dy[d]) == 'h') return d;
    for (int d = 0; d < 4; d++) if (item_open(x + dx[d], y + dy[d]) && !item_open(x + dx[(d + 2) & 3], y + dy[(d + 2) & 3])) return d;
    for (int d = 0; d < 4; d++) if (item_open(x + dx[d], y + dy[d])) return d;
    return 0;
}
int world_along(int x, int y, char ch) {
    int ax = (x > 0 && dm[y][x - 1] == ch) + (x < MW - 1 && dm[y][x + 1] == ch), ay = (y > 0 && dm[y - 1][x] == ch) + (y < MH - 1 && dm[y + 1][x] == ch);
    return !(ay > 0 && ax == 0);
}
int world_room_score(int mx, int my) {
    int kinds = 0;
    for (int y = my - 2; y <= my + 2; y++) for (int x = mx - 2; x <= mx + 2; x++) {
        char c = world_at(x, y);
        if (c == 'F') kinds |= 1; if (c == 'T') kinds |= 2; if (c == 'S') kinds |= 4; if (c == 'H') kinds |= 8; if (c == 'C' || c == 'U' || c == 'h') kinds |= 16;
        if (c == 'v' || c == 'R' || c == 'A') kinds |= 32; if (c == 'p' || c == 'l' || c == 'r' || c == 'q' || c == 'V' || c == 'g' || c == '3' || c == '6' || c == CH_BUSH) kinds |= 64;
    }
    int n = 0; for (int i = 0; i < 7; i++) if (kinds >> i & 1) n++;
    if (world_zone == ZONE_ISLAND) return MIN(100, 65 + n * 10);   // (the island: the sea air, the palms, the quiet; nobody minds the lack of a sofa)
    if (world_lvl <= 0 && mx >= 0 && my >= 0 && mx < MW && my < MH && floorMap[my][mx] >= FL_GRASS) return MIN(100, 55 + n * 15);   // (out of doors: the street, a park; fine for now)
    return MIN(100, n * 20);
}

// ---- drawing ----
static int frame_t;
// the floor's contact shading: how many walls stand round each floor corner, worked out when the map changes
static u8 ao[MH + 1][MW + 1];
int world_stamp;
void world_touched(void) { ao_dirty = 1; roof_dirty = 1; world_stamp++; }
static inline int wall0(int x, int y) { return (x < 0 || y < 0 || x >= MW || y >= MH) ? 1 : is_wall(lifeMap[y][x]); }   // (past the edge: a wall)
static void ao_region(int X0, int Y0, int X1, int Y1) {   // the corners [X0..X1] x [Y0..Y1]: how many of the four tiles round each are walls
    X1 = MIN(X1, MW); Y1 = MIN(Y1, MH);
    for (int Y = Y0; Y <= Y1; Y++) for (int X = X0; X <= X1; X++) ao[Y][X] = (u8)(wall0(X - 1, Y - 1) + wall0(X, Y - 1) + wall0(X - 1, Y) + wall0(X, Y));
}
static void ao_update(void) {
    static u8 r0[MW + 2], r1[MW + 2];                     // two rows of wall flags (x = -1 .. MW), past the edge a wall
    memset(r0, 1, sizeof r0);
    for (int Y = 0; Y <= MH; Y++) {
        if (Y < MH) { const char *row = lifeMap[Y]; r1[0] = r1[MW + 1] = 1; for (int x = 0; x < MW; x++) r1[x + 1] = (u8)is_wall(row[x]); }
        else memset(r1, 1, sizeof r1);
        u8 *o = ao[Y]; for (int X = 0; X <= MW; X++) o[X] = (u8)(r0[X] + r0[X + 1] + r1[X] + r1[X + 1]);
        memcpy(r0, r1, sizeof r0);
    }
    ao_dirty = 0;
}
static inline int ao_corner(int X, int Y, int L) { int n = ao[Y][X]; return n ? L * (100 - 16 * n) / 100 : L; }
static inline int near_wall(int mx, int my) { return ao[my][mx] | ao[my][mx + 1] | ao[my + 1][mx] | ao[my + 1][mx + 1]; }
// ---- corner caches: a tile corner is shared by four tiles, so it is transformed once a frame (per layer) and kept here ----
#define CW 28                                        // the window the caches cover, in tiles (the draw range is 25)
typedef struct { XV v[CW + 1][CW + 1]; u16 st[CW + 1][CW + 1]; u16 gen; int x0, y0, y; } CornerCache;
static CornerCache cc_roof;
static void cc_begin(CornerCache *c, int x0, int y0, int y) {
    c->x0 = x0; c->y0 = y0; c->y = y;
    if (++c->gen == 0) { memset(c->st, 0, sizeof c->st); c->gen = 1; }
}
static XV cc_tmp[8]; static int cc_tmpi;

// ---- the near plane. A quad with a corner behind the eye used to be thrown away whole, so a floor under you or a wall beside you simply
// vanished (and a close one that reached past the GPU's 1023x511 polygon size, or the GTE's +-1024 screen, came out bent or missing). Now such
// a quad is cut along a plane just in front of the eye and drawn from what is in front of it. In first person a quad near the eye is also
// split into pieces, so no piece is huge on the screen and the PlayStation's flat texture warp stays small. View space, whole numbers. ----
#define NC_Z 56                         // the cutting plane's depth (the GPU's own drop is Z_NEAR, 40)
#define NC_NEAR_FP 520                  // first person: a quad whose nearest corner is closer than this goes this way too
#define NC_DEPTH 5                      // a quad is split at most this many times
#define NXY(v) ((u32)(u16)(v)->x | ((u32)(u16)(v)->y << 16))
typedef struct { int x, y, z, u, v, r, g, b; } NV;     // a corner: view-space position, texel, colour
static NV nq_c[4]; static int nq_tex, nq_cull; static u32 nq_tp, nq_cl;
static inline int nc_need4(const XV *a, const XV *b, const XV *c, const XV *d) {   // 1: this quad goes the careful way
    int z = MIN(MIN(a->z, b->z), MIN(c->z, d->z));
    return z < Z_NEAR || (world_fp && z < NC_NEAR_FP);
}
static void nc_lerp(NV *o, const NV *p, const NV *q, int t) {
    o->x = p->x + (((q->x - p->x) * t) >> 12); o->y = p->y + (((q->y - p->y) * t) >> 12); o->z = p->z + (((q->z - p->z) * t) >> 12);
    o->u = p->u + (((q->u - p->u) * t) >> 12); o->v = p->v + (((q->v - p->v) * t) >> 12);
    o->r = p->r + (((q->r - p->r) * t) >> 12); o->g = p->g + (((q->g - p->g) * t) >> 12); o->b = p->b + (((q->b - p->b) * t) >> 12);
}
static void nc_at(NV *o, int s, int t) { NV top, bot; nc_lerp(&top, &nq_c[0], &nq_c[1], s); nc_lerp(&bot, &nq_c[2], &nq_c[3], s); nc_lerp(o, &top, &bot, t); }
static int nc_clip(const NV *in, NV *out) {          // the polygon (4 corners round) cut to the front of the plane: 0 to 5 corners
    int n = 0;
    for (int i = 0; i < 4; i++) {
        const NV *p = &in[i], *q = &in[(i + 1) & 3];
        int pin = p->z >= NC_Z, qin = q->z >= NC_Z;
        if (pin) out[n++] = *p;
        if (pin != qin) { nc_lerp(&out[n], p, q, ((NC_Z - p->z) << 12) / (q->z - p->z)); out[n].z = NC_Z; n++; }
    }
    return n;
}
static inline u32 nc_rgb(const NV *n) { return (u32)CLAMP(n->r, 0, 255) | ((u32)CLAMP(n->g, 0, 255) << 8) | ((u32)CLAMP(n->b, 0, 255) << 16); }
static inline u32 nc_uv(const NV *n) { return (u32)CLAMP(n->u, 0, 255) | ((u32)CLAMP(n->v, 0, 255) << 8); }
// one primitive: a, b, c, d in the order the GPU strips them (a b / c d), the corners' view-space data alongside
static void nc_put(const XV *a, const XV *b, const XV *c, const XV *d, const NV *na, const NV *nb, const NV *nc, const NV *nd) {
    if (nq_cull && (b->x - a->x) * (d->y - a->y) - (d->x - a->x) * (b->y - a->y) <= 0) return;   // facing away
    u32 c0 = nc_rgb(na), c1 = nc_rgb(nb), c2 = nc_rgb(nc), c3 = nc_rgb(nd);
    int otz = (a->z + b->z + c->z + d->z) >> 4;
    if (gfx_fog_on) { c0 = gfx_fog_apply(c0, otz); c1 = gfx_fog_apply(c1, otz); c2 = gfx_fog_apply(c2, otz); c3 = gfx_fog_apply(c3, otz); }
    if (nq_tex) {
        u32 *q = xv_alloc(13); if (!q) return;
        q[1] = c0 | 0x3c000000u; q[2] = NXY(a); q[3] = nc_uv(na) | nq_cl;
        q[4] = c1;               q[5] = NXY(b); q[6] = nc_uv(nb) | nq_tp;
        q[7] = c2;               q[8] = NXY(c); q[9] = nc_uv(nc);
        q[10] = c3;              q[11] = NXY(d); q[12] = nc_uv(nd);
        xv_link(q, 12, otz);
    } else {
        u32 *q = xv_alloc(9); if (!q) return;
        q[1] = c0 | 0x38000000u; q[2] = NXY(a); q[3] = c1; q[4] = NXY(b); q[5] = c2; q[6] = NXY(c); q[7] = c3; q[8] = NXY(d);
        xv_link(q, 8, otz);
    }
}
static void nc_piece(int s0, int s1, int t0, int t1, int depth) {   // the part s0..s1 x t0..t1 (of 4096) of the quad
    NV p[4], o[5];
    nc_at(&p[0], s0, t0); nc_at(&p[1], s1, t0); nc_at(&p[2], s1, t1); nc_at(&p[3], s0, t1);   // (round the piece: a, b, d, c)
    if (p[0].z < NC_Z && p[1].z < NC_Z && p[2].z < NC_Z && p[3].z < NC_Z) return;             // all behind the plane
    int n = nc_clip(p, o);
    if (n < 3) return;
    int sx[5], sy[5], x0 = 1 << 20, x1 = -(1 << 20), y0 = 1 << 20, y1 = -(1 << 20), zlo = 1 << 30, zhi = 0;
    for (int i = 0; i < n; i++) {
        sx[i] = CENTER_X + (o[i].x * 230) / o[i].z; sy[i] = CENTER_Y + (o[i].y * 230) / o[i].z;   // (the GTE's projection: H = 230)
        if (sx[i] < x0) x0 = sx[i]; if (sx[i] > x1) x1 = sx[i]; if (sy[i] < y0) y0 = sy[i]; if (sy[i] > y1) y1 = sy[i];
        if (o[i].z < zlo) zlo = o[i].z; if (o[i].z > zhi) zhi = o[i].z;
    }
    if (x1 < 0 || x0 >= SCREEN_W || y1 < 0 || y0 >= SCREEN_H) return;                         // off the screen
    int sw = x1 - x0, sh = y1 - y0;
    int big = sw > 560 || sh > 330;                                                            // too big for the GPU (and its +-1024 corners) to be sure of
    int warp = nq_tex && world_fp && zhi > 2 * zlo && zlo < NC_NEAR_FP && (sw > 80 || sh > 64);   // a textured piece running away from the eye: it would swim
    if ((big || warp) && depth < NC_DEPTH) {
        int ls = MAX(ABS(p[1].x - p[0].x) + ABS(p[1].y - p[0].y) + ABS(p[1].z - p[0].z), ABS(p[2].x - p[3].x) + ABS(p[2].y - p[3].y) + ABS(p[2].z - p[3].z));
        int lt = MAX(ABS(p[3].x - p[0].x) + ABS(p[3].y - p[0].y) + ABS(p[3].z - p[0].z), ABS(p[2].x - p[1].x) + ABS(p[2].y - p[1].y) + ABS(p[2].z - p[1].z));
        if (ls >= lt) { int m = (s0 + s1) >> 1; nc_piece(s0, m, t0, t1, depth + 1); nc_piece(m, s1, t0, t1, depth + 1); }
        else { int m = (t0 + t1) >> 1; nc_piece(s0, s1, t0, m, depth + 1); nc_piece(s0, s1, m, t1, depth + 1); }
        return;
    }
    if (sw > 700 || sh > 340) return;                                                          // (split as far as it goes and still too big: leave it)
    XV v[5];
    for (int i = 0; i < n; i++) { v[i].x = (short)sx[i]; v[i].y = (short)sy[i]; v[i].z = o[i].z; }
    if (n == 3) nc_put(&v[0], &v[1], &v[2], &v[2], &o[0], &o[1], &o[2], &o[2]);
    else {
        nc_put(&v[0], &v[1], &v[3], &v[2], &o[0], &o[1], &o[3], &o[2]);
        if (n == 5) nc_put(&v[0], &v[3], &v[4], &v[4], &o[0], &o[3], &o[4], &o[4]);
    }
}
// a quad given as its corners a b / c d (world space), a colour and a texel for each, cut and drawn by the above
static void nc_quad(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, const u32 *col, int tex, u16 tpage, u16 clut, const u32 *uv, int cull) {
    const SVECTOR *sv[4] = { a, b, c, d };
    const MATRIX *m = &cam.view;
    for (int i = 0; i < 4; i++) {
        int x = sv[i]->vx, y = sv[i]->vy, z = sv[i]->vz; NV *n = &nq_c[i];
        n->x = ((m->m[0][0] * x + m->m[0][1] * y + m->m[0][2] * z) >> 12) + m->t[0];
        n->y = ((m->m[1][0] * x + m->m[1][1] * y + m->m[1][2] * z) >> 12) + m->t[1];
        n->z = ((m->m[2][0] * x + m->m[2][1] * y + m->m[2][2] * z) >> 12) + m->t[2];
        n->u = (int)(uv[i] & 255); n->v = (int)((uv[i] >> 8) & 255);
        n->r = (int)(col[i] & 255); n->g = (int)((col[i] >> 8) & 255); n->b = (int)((col[i] >> 16) & 255);
    }
    nq_tex = tex; nq_cull = cull; nq_tp = (u32)tpage << 16; nq_cl = (u32)clut << 16;
    if (nq_c[0].z < NC_Z && nq_c[1].z < NC_Z && nq_c[2].z < NC_Z && nq_c[3].z < NC_Z) return;
    nc_piece(0, 4096, 0, 4096, 0);
}
// a flat quad (what gfx_quad_f drew), near-plane safe
static void wquad_f(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, int r, int g, int b8, int cull) {
    SVECTOR sv[4] SVEC_ALIGN; sv[0] = *a; sv[1] = *b; sv[2] = *c; sv[3] = *d;
    XV xv[4]; gfx_xform(sv, xv, 4);
    u32 col = gfx_grade_flat((u32)CLAMP(r, 0, 255) | ((u32)CLAMP(g, 0, 255) << 8) | ((u32)CLAMP(b8, 0, 255) << 16));
    if (nc_need4(&xv[0], &xv[1], &xv[2], &xv[3])) {
        const u32 cc[4] = { col, col, col, col }, uv0[4] = { 0, 0, 0, 0 };
        nc_quad(&sv[0], &sv[1], &sv[2], &sv[3], cc, 0, 0, 0, uv0, cull); return;
    }
    gfx_emit_f4(&xv[0], &xv[1], &xv[2], &xv[3], col, cull);
}
static void floor_tile(int mx, int my) {
    int fl = dfl[my][mx] % NFLOORS;
    const TexRef *t = &fl_tex[fl];
    int x0 = mx * TILE, x1 = x0 + TILE, zN = -my * TILE, zS = zN - TILE;
    SVECTOR a, b, c, d;
    int y = (fl == FL_WATER) ? 14 + (isin((frame_t * 40 + mx * 300 + my * 500) & 4095) >> 10) : 0;   // water sits a little low and heaves
    setVector(&a, x0, y, zN); setVector(&b, x1, y, zN); setVector(&c, x0, y, zS); setVector(&d, x1, y, zS);
    static int Lfr = -1, Lup; if (Lfr != frame_t) { Lfr = frame_t; Lup = gfx_light(0, -ONE, 0); }   // (the floor's light: once a frame)
    int L = Lup;
    if ((mx + my) & 1) L = (L * fl_odd[fl]) >> 4;
    L = CLAMP(L, 40, 200);
    int u = t->u, v = t->v;
    if (fl == FL_WATER) { u += (frame_t >> 3) & 7; v += (frame_t >> 4) & 7; }   // (the 16x16 texture holds the 8x8 pattern twice: it tiles)
    if (y == 0) {   // the fast path (all but water)
        SVECTOR sv[4] = { a, b, c, d }; XV xv[4]; gfx_xform(sv, xv, 4);
        const XV *A = &xv[0], *B = &xv[1], *C = &xv[2], *Dd = &xv[3];
        if (nc_need4(A, B, C, Dd)) {   // (the eye is close: cut at the near plane, split in first person)
            u32 cc[4], uv4[4] = { (u32)u | ((u32)v << 8), (u32)(u + 7) | ((u32)v << 8), (u32)u | ((u32)(v + 7) << 8), (u32)(u + 7) | ((u32)(v + 7) << 8) };
            if (!dbg_noao && !is_wall(dm[my][mx]) && near_wall(mx, my)) {
                cc[0] = gfx_grade_tex((u32)ao_corner(mx, my, L) * 0x010101u); cc[1] = gfx_grade_tex((u32)ao_corner(mx + 1, my, L) * 0x010101u);
                cc[2] = gfx_grade_tex((u32)ao_corner(mx, my + 1, L) * 0x010101u); cc[3] = gfx_grade_tex((u32)ao_corner(mx + 1, my + 1, L) * 0x010101u);
            } else cc[0] = cc[1] = cc[2] = cc[3] = gfx_grade_tex((u32)L * 0x010101u);
            nc_quad(&sv[0], &sv[1], &sv[2], &sv[3], cc, 1, t->tpage, 0, uv4, 0);
            return;
        }
        if (!dbg_noao && !is_wall(dm[my][mx]) && near_wall(mx, my)) {
            int L0 = ao_corner(mx, my, L), L1 = ao_corner(mx + 1, my, L), L2 = ao_corner(mx, my + 1, L), L3 = ao_corner(mx + 1, my + 1, L);
            gfx_emit_gt4(A, B, C, Dd, gfx_grade_tex((u32)L0 * 0x010101u), gfx_grade_tex((u32)L1 * 0x010101u), gfx_grade_tex((u32)L2 * 0x010101u), gfx_grade_tex((u32)L3 * 0x010101u),
                         t->tpage, 0, u, v, u + 7, v + 7, 0);
        } else gfx_emit_ft4(A, B, C, Dd, gfx_grade_tex((u32)L * 0x010101u), t->tpage, 0, u, v, u + 7, v, u, v + 7, u + 7, v + 7, 0, 0);
        return;
    }
    if (!dbg_noao && !is_wall(dm[my][mx]) && near_wall(mx, my)) {   // next to a wall: the corners by it darken (contact shadow)
        int L0 = ao_corner(mx, my, L), L1 = ao_corner(mx + 1, my, L), L2 = ao_corner(mx, my + 1, L), L3 = ao_corner(mx + 1, my + 1, L);
        gfx_quad_gt(&a, &b, &c, &d, t->tpage, 0, u, v, u + 7, v, u, v + 7, u + 7, v + 7,
                    (u32)L0 * 0x010101u, (u32)L1 * 0x010101u, (u32)L2 * 0x010101u, (u32)L3 * 0x010101u, 0);
        return;
    }
    gfx_quad_ft(&a, &b, &c, &d, t->tpage, 0, u, v, u + 7, v, u, v + 7, u + 7, v + 7, L, 0, 0);
}

// ---- CASTAWAY ISLAND's ground: every corner coloured from the tiles round it (sand, meadow, jungle, rock, earth, sea), the waterline
// wet and foaming, the ground darker under the trees; over nature.c's fine grain. The sea heaves corner by corner ----
static int nat_on;                                      // (world_draw: the island, nature drawn)
int world_nature = 1;                                   // (the options: the island drawn as nature)
static inline int nat_floor(int f) { return f == FL_GRASS || f == FL_SAND || f == FL_ROCK || f == FL_WATER || f == FL_SOIL; }
static inline int nat_wet(int x, int y) { return x < 0 || y < 0 || x >= MW || y >= MH || floorMap[y][x] == FL_WATER; }
static u8 ngr_j[10][10], ngr_d[10][10]; static u32 ngr_for = 1;   // the island's noise on its 10-tile grid (the jungle; dry meadow), as the generator's
static int nat_noise(u8 (*g)[10], int x, int y) {   // (the generator's value noise, from its grid: no divisions)
    int gx = (x * 205) >> 11, gy = (y * 205) >> 11, fx = x - gx * 10, fy = y - gy * 10;
    if (gx > 8) { gx = 8; fx = 10; } if (gy > 8) { gy = 8; fy = 10; }
    int top = g[gy][gx] * (10 - fx) + g[gy][gx + 1] * fx, bot = g[gy + 1][gx] * (10 - fx) + g[gy + 1][gx + 1] * fx;
    return ((top * (10 - fy) + bot * fy) * 41) >> 12;
}
static inline int tri_wave(int a) { return 4096 - ABS(((a + 1024) & 4095) - 2048) * 4; }   // (a sine's shape, near enough, for waves: -4096..4096)
static inline int nat_height(int X, int Y, int t) {   // a corner's height: the sea's swell, the waterline, the land
    int wet = nat_wet(X - 1, Y - 1) + nat_wet(X, Y - 1) + nat_wet(X - 1, Y) + nat_wet(X, Y);
    return wet == 4 ? 20 + (tri_wave(t * 40 + X * 700 + Y * 460) >> 10) : wet ? 9 : 0;
}
u32 world_horizon = 0xBECDD7;                   // the sky's colour at the horizon (main sets it: the haze)
static u32 nat_haze(u32 c, int z) {   // a lit colour going into the haze with distance (the far sea and the sky meet it)
    int f = ((z - 3000) * 205) >> 13; if (f <= 0) return c; if (f > 230) f = 230;
    int hr = world_horizon & 255, hg = (world_horizon >> 8) & 255, hb = (world_horizon >> 16) & 255, r = c & 255, g = (c >> 8) & 255, b = (c >> 16) & 255;
    return (u32)(r + (((hr - r) * f) >> 8)) | ((u32)(g + (((hg - g) * f) >> 8)) << 8) | ((u32)(b + (((hb - b) * f) >> 8)) << 16);
}
static u16 nat_base(int X, int Y, int *kind) {   // a corner of the ground, as it stands: its colour (RGB555, before the light) and kind (0 land, 1 waterline, 2 shallows, 3 deep)
    int r = 0, g = 0, b = 0, n = 0, wet = 0, shade = 0, ng = 0;
    for (int k = 0; k < 4; k++) {
        int x = X - 1 + (k & 1), y = Y - 1 + (k >> 1);
        if (nat_wet(x, y)) { wet++; continue; }
        char c = lifeMap[y][x];
        if (c == 'g' || c == '3' || c == '6') shade += 2; else if (c == (char)CH_BUSH || c == '*') shade++;
        switch (floorMap[y][x]) {
            case FL_GRASS: ng++; break;
            case FL_SAND: r += 150; g += 136; b += 100; break;
            case FL_ROCK: r += 112; g += 104; b += 92; break;
            case FL_SOIL: r += 120; g += 96; b += 70; break;
            default: r += 130; g += 116; b += 92; break;
        }
        n++;
    }
    if (!n) {   // the sea: turquoise over the shallows, deep blue further out
        static const s8 ring[8][2] = { { -2, -1 }, { -2, 0 }, { 1, -1 }, { 1, 0 }, { -1, -2 }, { 0, -2 }, { -1, 1 }, { 0, 1 } };
        for (int k = 0; k < 8; k++) if (!nat_wet(X + ring[k][0], Y + ring[k][1])) { *kind = 2; return RGB(40 >> 3, 120 >> 3, 128 >> 3); }
        *kind = 3; return RGB(22 >> 3, 70 >> 3, 120 >> 3);
    }
    if (ng) {   // grass: meadow, deepening into jungle, drying in patches
        int gx = (X * 205) >> 11, gy = (Y * 205) >> 11, fx = X - gx * 10, fy = Y - gy * 10;
        if (gx > 8) { gx = 8; fx = 10; } if (gy > 8) { gy = 8; fy = 10; }
        int w00 = (10 - fx) * (10 - fy), w10 = fx * (10 - fy), w01 = (10 - fx) * fy, w11 = fx * fy;   // (both noises at once: the generator's, from their grids)
        int nj = ((ngr_j[gy][gx] * w00 + ngr_j[gy][gx + 1] * w10 + ngr_j[gy + 1][gx] * w01 + ngr_j[gy + 1][gx + 1] * w11) * 41) >> 12;
        int nd = ((ngr_d[gy][gx] * w00 + ngr_d[gy][gx + 1] * w10 + ngr_d[gy + 1][gx] * w01 + ngr_d[gy + 1][gx + 1] * w11) * 41) >> 12;
        int j = CLAMP((nj - 96) * 4, 0, 255), d = (CLAMP((nd - 140) * 3, 0, 255) * (255 - j)) >> 8;
        r += (76 - ((38 * j) >> 8) + ((40 * d) >> 8)) * ng; g += (124 - ((38 * j) >> 8) + ((4 * d) >> 8)) * ng; b += (46 - ((12 * j) >> 8) + ((8 * d) >> 8)) * ng;
    }
    static const u8 inv[5] = { 0, 255, 128, 85, 64 };   // (r / n, as a multiply)
    r = (r * inv[n]) >> 8; g = (g * inv[n]) >> 8; b = (b * inv[n]) >> 8;
    int v = 228 + (int)(((u32)(X * 73856093) ^ (u32)(Y * 19349663)) >> 27) + (int)(((u32)((X >> 1) * 83492791) ^ (u32)((Y >> 1) * 2971215073u)) >> 27) - (int)(((u32)((X >> 3) * 19349663) ^ (u32)((Y >> 3) * 73856093 + 77)) >> 28);   // (each corner a touch lighter or darker, in patches of three sizes)
    if (shade) v -= 22 * MIN(shade, 5);
    if (ao[Y][X] && !dbg_noao) v = (v * (256 - 41 * ao[Y][X])) >> 8;
    r = (r * v) >> 8; g = (g * v) >> 8; b = (b * v) >> 8;
    if (wet) { r = r * 3 >> 2; g = g * 3 >> 2; b = b * 3 >> 2; }   // (wet sand at the waterline)
    *kind = wet ? 1 : 0;
    return RGB(MIN(r, 255) >> 3, MIN(g, 255) >> 3, MIN(b, 255) >> 3);
}
static u16 nc_col[32][32], nc_tag[32][32]; static int nc_stamp = -1; static u32 nc_for;   // (the corners as they stand, kept while the map is unchanged)
static u32 nat_colour(int X, int Y, int t, int Lg, int z) {   // a corner of the ground in this frame: the sea's shimmer, the foam, the light, the haze
    u16 *tg = &nc_tag[Y & 31][X & 31]; int want = 1 + (Y << 7) + X, kind;
    u16 c;
    if ((*tg & 0x3fff) == want) { c = nc_col[Y & 31][X & 31]; kind = *tg >> 14; }
    else { c = nat_base(X, Y, &kind); nc_col[Y & 31][X & 31] = c; *tg = (u16)(want | (kind << 14)); }
    int r = C_R(c) << 3, g = C_G(c) << 3, b = C_B(c) << 3;
    if (kind >= 2) { int sh = 256 + (tri_wave(t * 40 + X * 700 + Y * 460) >> 7); r = (r * sh) >> 8; g = (g * sh) >> 8; b = (b * sh) >> 8; }
    else if (kind) {   // the foam the waves leave, coming and going
        int f = tri_wave(t * 56 + X * 900 + Y * 1300), fo = 56 + (f > 0 ? f >> 6 : 0);
        r = (r * (256 - fo) + 168 * fo) >> 8; g = (g * (256 - fo) + 176 * fo) >> 8; b = (b * (256 - fo) + 170 * fo) >> 8;
    }
    r = (r * Lg) >> 7; g = (g * Lg) >> 7; b = (b * Lg) >> 7;   // (the palette is warm already: no grade)
    return nat_haze((u32)MIN(r, 255) | ((u32)MIN(g, 255) << 8) | ((u32)MIN(b, 255) << 16), z);
}
static int ngx0, ngx1, ngy0, ngy1;                     // (the box the ground is drawn in, in full: the far cells do the rest)
static void nat_ground(u8 (*done)[MW], int x0, int x1, int y0, int y1) {   // the range's natural ground, row by row: each row of corners transformed once, coloured only where seen
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if (nat_floor(floorMap[y][x])) done[y][x] = 1;   // (past the box: the far cells' ground)
    x0 = MAX(x0, ngx0); x1 = MIN(x1, ngx1); y0 = MAX(y0, ngy0); y1 = MIN(y1, ngy1);
    if (x1 < x0 || y1 < y0) return;
    u32 S = wsalt * 2654435761u + 0x15ADu;
    if (nc_stamp != world_stamp || nc_for != S) { memset(nc_tag, 0, sizeof nc_tag); nc_stamp = world_stamp; nc_for = S; }
    if (ngr_for != S) { for (int y = 0; y < 10; y++) for (int x = 0; x < 10; x++) { ngr_j[y][x] = (u8)(bm_hash(x, y, S ^ 0x77u) & 255); ngr_d[y][x] = (u8)(bm_hash(x, y, S ^ 0x3C1u) & 255); } ngr_for = S; }
    static SVECTOR rs[2][32] SVEC_ALIGN; static XV rv[2][32]; static u32 rc[2][32]; static u8 rk[2][32];
    x1 = MIN(x1, x0 + 29); int nx = x1 - x0 + 2, t = frame_t;
    int Lg = CLAMP(gfx_light(0, -ONE, 0), 40, 200);
    gfx_floor_pass = 1;
    for (int Y = y0, cur = 0; Y <= y1 + 1; Y++, cur ^= 1) {
        for (int i = 0; i < nx; i++) { setVector(&rs[cur][i], (x0 + i) * TILE, nat_height(x0 + i, Y, t), -Y * TILE); rk[cur][i] = 0; }
        gfx_xform(rs[cur], rv[cur], nx);
        if (Y == y0) continue;
        int my = Y - 1, up = cur ^ 1; const XV *T = rv[up], *B = rv[cur];
        for (int i = 0; i + 1 < nx; i++) {
            int mx = x0 + i, f = floorMap[my][mx];
            if (!nat_floor(f)) continue;
            if (!xv_vis4(&T[i], &T[i + 1], &B[i], &B[i + 1], 0)) continue;
            #define NCOL(row, k, Yc) if (!rk[row][k]) { rc[row][k] = nat_colour(x0 + (k), Yc, t, Lg, rv[row][k].z); rk[row][k] = 1; }
            NCOL(up, i, my) NCOL(up, i + 1, my) NCOL(cur, i, Y) NCOL(cur, i + 1, Y)
            #undef NCOL
            const TexRef *tx = f == FL_WATER ? &nat_tex_water : f == FL_GRASS ? &nat_tex_grass : &nat_tex_ground;
            int u, v;
            u = tx->u + (f == FL_WATER ? ((t >> 2) & 15) : 0); v = tx->v;   // (the sea's ripples slide)
            { int u1 = u + 15, v1 = v + 15;
              if (f != FL_WATER) { u32 hh = ((u32)mx * 73856093u ^ (u32)my * 19349663u) >> 13; if (hh & 1) { int q = u; u = u1; u1 = q; } if (hh & 2) { int q = v; v = v1; v1 = q; } }   // (each tile mirrored a different way: the pattern no longer repeats in a grid)
              gfx_put_gt4(&T[i], &T[i + 1], &B[i], &B[i + 1], rc[up][i], rc[up][i + 1], rc[cur][i], rc[cur][i + 1], (u32)tx->tpage << 16, 0, u, v, u1, v1); }
        }
    }
    gfx_floor_pass = 0;
}

static inline int gvar(int x, int y) {   // the outdoor floors' patchiness: stains, wear, sun-bleached bits (per corner, so neighbours agree)
    int a = (int)(((u32)x * 73856093u ^ (u32)y * 19349663u) >> 28), b = (int)(((u32)(x >> 2) * 2654435761u ^ (u32)(y >> 2) * 40503u + 91) >> 27), c = (int)(((u32)(x >> 4) * 83492791u ^ (u32)(y >> 4) * 668265263u) >> 28);
    return 214 + a + b + c;   // about 230..292 (256 is plain)
}
static void floor_block(int mx, int my) {   // the 2x2 tiles at (mx,my) (even), all the same floor: one quad
    int fl = dfl[my][mx] % NFLOORS;
    const TexRef *t = &fl_tex4[fl];
    int x0 = mx * TILE, x1 = x0 + 2 * TILE, zN = -my * TILE, zS = zN - 2 * TILE;
    SVECTOR sv[4]; setVector(&sv[0], x0, 0, zN); setVector(&sv[1], x1, 0, zN); setVector(&sv[2], x0, 0, zS); setVector(&sv[3], x1, 0, zS);
    XV xv[4]; gfx_xform(sv, xv, 4);
    const XV *A = &xv[0], *B = &xv[1], *C = &xv[2], *Dd = &xv[3];
    static int Lfr = -1, Lup; if (Lfr != frame_t) { Lfr = frame_t; Lup = CLAMP(gfx_light(0, -ONE, 0), 40, 200); }
    int L = Lup;
    if (nc_need4(A, B, C, Dd)) {   // (the eye is close: cut at the near plane, split in first person)
        u32 cc[4], uv4[4] = { (u32)t->u | ((u32)t->v << 8), (u32)(t->u + 31) | ((u32)t->v << 8), (u32)t->u | ((u32)(t->v + 31) << 8), (u32)(t->u + 31) | ((u32)(t->v + 31) << 8) };
        if (!dbg_noao && (near_wall(mx, my) | near_wall(mx + 1, my) | near_wall(mx, my + 1) | near_wall(mx + 1, my + 1))) {
            cc[0] = gfx_grade_tex((u32)ao_corner(mx, my, L) * 0x010101u); cc[1] = gfx_grade_tex((u32)ao_corner(mx + 2, my, L) * 0x010101u);
            cc[2] = gfx_grade_tex((u32)ao_corner(mx, my + 2, L) * 0x010101u); cc[3] = gfx_grade_tex((u32)ao_corner(mx + 2, my + 2, L) * 0x010101u);
        } else cc[0] = cc[1] = cc[2] = cc[3] = gfx_grade_tex((u32)L * 0x010101u);
        if (fl >= FL_PAVING && fl != FL_WATER && fl != FL_ROCK && !FL_IS_ROAD(fl)) for (int k = 0; k < 4; k++) { int Lc = CLAMP(L * gvar(mx + 2 * (k & 1), my + 2 * (k >> 1)) >> 8, 20, 250); cc[k] = gfx_grade_tex((u32)Lc * 0x010101u); }
        SVECTOR s4[4] SVEC_ALIGN; s4[0] = sv[0]; s4[1] = sv[1]; s4[2] = sv[2]; s4[3] = sv[3];
        nc_quad(&s4[0], &s4[1], &s4[2], &s4[3], cc, 1, t->tpage, 0, uv4, 0);
        return;
    }
    if (!dbg_noao && (near_wall(mx, my) | near_wall(mx + 1, my) | near_wall(mx, my + 1) | near_wall(mx + 1, my + 1))) {
        int L0 = ao_corner(mx, my, L), L1 = ao_corner(mx + 2, my, L), L2 = ao_corner(mx, my + 2, L), L3 = ao_corner(mx + 2, my + 2, L);
        gfx_emit_gt4(A, B, C, Dd, gfx_grade_tex((u32)L0 * 0x010101u), gfx_grade_tex((u32)L1 * 0x010101u), gfx_grade_tex((u32)L2 * 0x010101u), gfx_grade_tex((u32)L3 * 0x010101u),
                     t->tpage, 0, t->u, t->v, t->u + 31, t->v + 31, 0);
        return;
    }
    if (fl >= FL_PAVING && fl != FL_WATER && fl != FL_ROCK && !FL_IS_ROAD(fl)) {   // (the streets and paving: a tint that wanders from corner to corner)
        int Lc[4]; for (int k = 0; k < 4; k++) Lc[k] = CLAMP(L * gvar(mx + 2 * (k & 1), my + 2 * (k >> 1)) >> 8, 20, 250);
        gfx_emit_gt4(A, B, C, Dd, gfx_grade_tex((u32)Lc[0] * 0x010101u), gfx_grade_tex((u32)Lc[1] * 0x010101u), gfx_grade_tex((u32)Lc[2] * 0x010101u), gfx_grade_tex((u32)Lc[3] * 0x010101u), t->tpage, 0, t->u, t->v, t->u + 31, t->v + 31, 0);
        return;
    }
    gfx_emit_ft4(A, B, C, Dd, gfx_grade_tex((u32)L * 0x010101u), t->tpage, 0, t->u, t->v, t->u + 31, t->v, t->u, t->v + 31, t->u + 31, t->v + 31, 0, 0);
}

// ---- thin walls: a wall tile is a slab 40 units thick running along the line its neighbours make, like a Sims wall ----
#define WT 20
static int wall_or_door(int mx, int my) { char c = world_at(mx, my); return is_wall(c) || c == 'D'; }
// a vertical face from (ax,az) to (bx,bz), from yTop (negative) down to yBot, textured with the wallpaper (u0..u1 of the tile's 8 columns)
static void wface(const TexRef *t, int ax, int az, int bx, int bz, int yTop, int yBot, int h, int u0, int u1, int nx, int nz, int cut) {
    SVECTOR a, b, c, d;
    setVector(&a, ax, yTop, az); setVector(&b, bx, yTop, bz); setVector(&c, ax, yBot, az); setVector(&d, bx, yBot, bz);
    // the 24-row texture spans a full wall; a cut wall shows the bottom rows. The rows and the shading depend only on the face's
    // heights and direction: worked out once a frame for each kind (a few) instead of with six divisions per face
    static struct { int key, fr, dv0, dv1, Lt, Lb, L; } memo[16];
    int key = ((yTop & 0x3ff) | ((yBot & 0x3ff) << 10) | (cut << 20) | ((nx > 0) << 21) | ((nx < 0) << 22) | ((nz > 0) << 23) | ((nz < 0) << 24)) ^ (h << 6);
    int slot_ = (key ^ (key >> 7) ^ (key >> 13)) & 15;
    if (memo[slot_].key != key || memo[slot_].fr != frame_t) {
        int vspan = cut ? 23 * h / WALL_FULL : 23; if (vspan < 2) vspan = 2;
        int L = CLAMP(gfx_light(nx, 0, nz), 40, 200);
        int Lt = L + (L * (-yTop)) / (h * 6), Lb = L - (L * (-yBot - 0)) / (h * 3) - L * (h + yBot) / (h * 8);   // lighter up top, darker at the foot
        memo[slot_].key = key; memo[slot_].fr = frame_t; memo[slot_].L = L;
        memo[slot_].dv0 = (-(yTop) * vspan) / h; memo[slot_].dv1 = (-(yBot) * vspan) / h;
        memo[slot_].Lt = CLAMP(Lt, 30, 230); memo[slot_].Lb = CLAMP(Lb, 30, 230);
    }
    int vb = t->v + 23;
    int v0 = vb - memo[slot_].dv0, v1 = vb - memo[slot_].dv1;
    v0 = CLAMP(v0, t->v, vb); v1 = CLAMP(v1, t->v, vb);
    int L = memo[slot_].L, Lt = memo[slot_].Lt, Lb = memo[slot_].Lb;
    SVECTOR sv[4] SVEC_ALIGN; sv[0] = a; sv[1] = b; sv[2] = c; sv[3] = d; XV xv[4]; gfx_xform(sv, xv, 4);
    if (nc_need4(&xv[0], &xv[1], &xv[2], &xv[3])) {   // (the eye is close: cut at the near plane, split in first person)
        u32 ct_ = dbg_nograd ? gfx_grade_tex((u32)L * 0x010101u) : gfx_grade_tex((u32)Lt * 0x010101u), cb_ = dbg_nograd ? ct_ : gfx_grade_tex((u32)Lb * 0x010101u);
        const u32 cc[4] = { ct_, ct_, cb_, cb_ };
        const u32 uv4[4] = { (u32)(t->u + u0) | ((u32)v0 << 8), (u32)(t->u + u1) | ((u32)v0 << 8), (u32)(t->u + u0) | ((u32)v1 << 8), (u32)(t->u + u1) | ((u32)v1 << 8) };
        nc_quad(&sv[0], &sv[1], &sv[2], &sv[3], cc, 1, t->tpage, 0, uv4, 1);
        return;
    }
    if (dbg_nograd) { gfx_quad_ft(&a, &b, &c, &d, t->tpage, 0, t->u + u0, v0, t->u + u1, v0, t->u + u0, v1, t->u + u1, v1, L, 1, 0); return; }
    if (!xv_vis4(&xv[0], &xv[1], &xv[2], &xv[3], 1)) return;
    u32 ct = gfx_grade_tex((u32)Lt * 0x010101u), cb = gfx_grade_tex((u32)Lb * 0x010101u);
    gfx_put_gt4(&xv[0], &xv[1], &xv[2], &xv[3], ct, ct, cb, cb, (u32)t->tpage << 16, 0, t->u + u0, v0, t->u + u1, v1);
}
static void wtop(u16 tc, int xa, int xb, int za, int zb, int y) {   // the top of a slab: za > zb
    SVECTOR a, b, c, d; int L = gfx_light(0, -ONE, 0);
    setVector(&a, xa, y, za); setVector(&b, xb, y, za); setVector(&c, xa, y, zb); setVector(&d, xb, y, zb);
    int r = (R8(tc) * L) >> 7, g = (G8(tc) * L) >> 7, bl = (B8(tc) * L) >> 7;
    wquad_f(&a, &b, &c, &d, MIN(r, 255), MIN(g, 255), MIN(bl, 255), 1);
}
static void skirt(int ax, int az, int bx, int bz, int nx, int nz) {   // a skirting board along a face's foot
    SVECTOR a, b, c, d; u16 bc = RGB(26, 25, 23); int L = CLAMP(gfx_light(nx, 0, nz), 40, 200);
    int ox = nx ? (nx > 0 ? 3 : -3) : 0, oz = nz ? (nz > 0 ? 3 : -3) : 0;
    setVector(&a, ax + ox, -22, az + oz); setVector(&b, bx + ox, -22, bz + oz); setVector(&c, ax + ox, 0, az + oz); setVector(&d, bx + ox, 0, bz + oz);
    wquad_f(&a, &b, &c, &d, (R8(bc) * L) >> 7, (G8(bc) * L) >> 7, (B8(bc) * L) >> 7, 1);
}
// a slab [xa..xb] x [zb..za] (za > zb) of height h: its four sides (those not flagged hidden) and its top
static int ws_lite;                                   // another building's wall (under its roof): no skirting, no top, only the pane on the near side
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
        if (!cut && h == WALL_FULL && !ws_lite) skirt(ax, az, bx, bz, nx, nz);
    if (!hideN) { LONGFACE(xb, za, xa, za, 0, ONE, ua, ub - 1) }          // north side (+z), seen from the north: left = xb
    if (!hideS) { LONGFACE(xa, zb, xb, zb, 0, -ONE, ua, ub - 1) }
    if (!hideW) { LONGFACE(xa, za, xa, zb, -ONE, 0, va, vb2 - 1) }
    if (!hideE) { LONGFACE(xb, zb, xb, za, ONE, 0, va, vb2 - 1) }
    #undef LONGFACE
    if (!ws_lite || cut) wtop(tc, xa, xb, za, zb, yT);
    if (win && h == WALL_FULL) {   // the pane, both sides of the slab
        SVECTOR a, b, c, d; int gl = win == 'e' ? 60 : 150;
        if (xb - xa > zb < za ? (xb - xa) > (za - zb) : 0) {}
        if ((xb - xa) >= (za - zb)) {   // runs along x: panes on the north and south faces
            if (cam.eye.vz > za) { setVector(&a, xb, gy0, za + 1); setVector(&b, xa, gy0, za + 1); setVector(&c, xb, gy1, za + 1); setVector(&d, xa, gy1, za + 1);
            gfx_quad_ft(&a, &b, &c, &d, glass_tex.tpage, 0, glass_tex.u, glass_tex.v, glass_tex.u + 7, glass_tex.v, glass_tex.u, glass_tex.v + 7, glass_tex.u + 7, glass_tex.v + 7, gl, 1, 1); }
            if (cam.eye.vz < zb) { setVector(&a, xa, gy0, zb - 1); setVector(&b, xb, gy0, zb - 1); setVector(&c, xa, gy1, zb - 1); setVector(&d, xb, gy1, zb - 1);
            gfx_quad_ft(&a, &b, &c, &d, glass_tex.tpage, 0, glass_tex.u, glass_tex.v, glass_tex.u + 7, glass_tex.v, glass_tex.u, glass_tex.v + 7, glass_tex.u + 7, glass_tex.v + 7, gl, 1, 1); }
        } else {
            if (cam.eye.vx < xa) { setVector(&a, xa - 1, gy0, za); setVector(&b, xa - 1, gy0, zb); setVector(&c, xa - 1, gy1, za); setVector(&d, xa - 1, gy1, zb);
            gfx_quad_ft(&a, &b, &c, &d, glass_tex.tpage, 0, glass_tex.u, glass_tex.v, glass_tex.u + 7, glass_tex.v, glass_tex.u, glass_tex.v + 7, glass_tex.u + 7, glass_tex.v + 7, gl, 1, 1); }
            if (cam.eye.vx > xb) { setVector(&a, xb + 1, gy0, zb); setVector(&b, xb + 1, gy0, za); setVector(&c, xb + 1, gy1, zb); setVector(&d, xb + 1, gy1, za);
            gfx_quad_ft(&a, &b, &c, &d, glass_tex.tpage, 0, glass_tex.u, glass_tex.v, glass_tex.u + 7, glass_tex.v, glass_tex.u, glass_tex.v + 7, glass_tex.u + 7, glass_tex.v + 7, gl, 1, 1); }
        }
    }
}
static void wall_tile(int mx, int my, int h, int cut) {
    char c = dm[my][mx];
    int wp = dwp[my][mx] % NWALLS;
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
// ---- DOORS: a real door in every doorway. It stands shut (a painted leaf with a brass knob) and slides open while anybody is at it ----
static u16 dpp[160][2]; static int ndpp;               // the tiles people stand on this frame (the player, the family, the crowd): the doors beside them open
void world_doors_reset(void) { ndpp = 0; }
void world_door_mark(int tx, int ty) { if (ndpp < 160) { dpp[ndpp][0] = (u16)tx; dpp[ndpp][1] = (u16)ty; ndpp++; } }
static int door_busy(int mx, int my) { for (int i = 0; i < ndpp; i++) if (ABS((int)dpp[i][0] - mx) <= 1 && ABS((int)dpp[i][1] - my) <= 1) return 1; return 0; }
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
        setVector(&a, xb, ya, za); setVector(&bb, xa, ya, za); setVector(&c, xb, yb, za); setVector(&d, xa, yb, za); wquad_f(&a, &bb, &c, &d, r, g, b, 1); \
        setVector(&a, xa, ya, zb); setVector(&bb, xb, ya, zb); setVector(&c, xa, yb, zb); setVector(&d, xb, yb, zb); wquad_f(&a, &bb, &c, &d, r, g, b, 1); \
        setVector(&a, xa, ya, za); setVector(&bb, xa, ya, zb); setVector(&c, xa, yb, za); setVector(&d, xa, yb, zb); wquad_f(&a, &bb, &c, &d, r * 3 / 4, g * 3 / 4, b * 3 / 4, 1); \
        setVector(&a, xb, ya, zb); setVector(&bb, xb, ya, za); setVector(&c, xb, yb, zb); setVector(&d, xb, yb, za); wquad_f(&a, &bb, &c, &d, r * 3 / 4, g * 3 / 4, b * 3 / 4, 1); \
        setVector(&a, xa, ya, za); setVector(&bb, xb, ya, za); setVector(&c, xa, ya, zb); setVector(&d, xb, ya, zb); wquad_f(&a, &bb, &c, &d, r * 5 / 4, g * 5 / 4, b * 5 / 4, 1); }
    int P = 14;
    if (alongX) { BOX(x0, x0 + P, -h, 0, cz + WT, cz - WT) BOX(x1 - P, x1, -h, 0, cz + WT, cz - WT) BOX(x0, x1, -h, -h + 56, cz + WT, cz - WT) }
    else        { BOX(cx - WT, cx + WT, -h, 0, zN, zN - P) BOX(cx - WT, cx + WT, -h, 0, zS + P, zS) BOX(cx - WT, cx + WT, -h, -h + 56, zN, zS) }
    if (h > 90) {                                                        // the leaf: shut, or slid back into the wall beside its post
        static const u8 pal[6][3] = { { 150, 98, 58 }, { 112, 72, 46 }, { 74, 112, 150 }, { 150, 62, 56 }, { 226, 222, 210 }, { 70, 120, 84 } };
        unsigned hd = (unsigned)mx * 73856093u ^ (unsigned)my * 19349663u; const u8 *pc = pal[(hd >> 7) % 6u];
        int dr = (pc[0] * L) >> 7, dg = (pc[1] * L) >> 7, db = (pc[2] * L) >> 7; if (dr > 255) dr = 255; if (dg > 255) dg = 255; if (db > 255) db = 255;
        int open = door_busy(mx, my), ya = -h + 58, T = 7;
        int sr = r, sg = g, sb = b; r = dr; g = dg; b = db;              // (the box macro paints with r g b)
        if (alongX) {
            int xa = open ? x0 + P : x0 + P, xb = open ? x0 + P + 44 : x1 - P;
            BOX(xa, xb, ya, 0, cz + T, cz - T)
            if (!open) { r = 235; g = 190; b = 70; BOX(x1 - P - 34, x1 - P - 20, ya / 2 + 24, ya / 2 + 38, cz + T + 6, cz - T - 6) }
        } else {
            int za = open ? zN - P : zN - P, zb = open ? zN - P - 44 : zS + P;
            BOX(cx - T, cx + T, ya, 0, za, zb)
            if (!open) { r = 235; g = 190; b = 70; BOX(cx - T - 6, cx + T + 6, ya / 2 + 24, ya / 2 + 38, zS + P + 20, zS + P + 34) }
        }
        r = sr; g = sg; b = sb;
    }
    #undef BOX
}

// ---- RUGS: runs of rug tiles are one rug (a field, a border, a fringe), not a row of stuck-on mats ----
static void rug_tile(int mx, int my) {
    int ox = mx, oy = my;
    while (ox > 0 && world_at(ox - 1, oy) == 'r') ox--;
    while (oy > 0 && world_at(ox, oy - 1) == 'r') oy--;                  // the rug's top-left tile: its colours come from there
    unsigned hd = (unsigned)ox * 2654435761u ^ (unsigned)oy * 40503u; hd ^= hd >> 13;
    static const u8 pal[6][2][3] = { { { 150, 48, 52 }, { 214, 176, 96 } }, { { 56, 84, 138 }, { 226, 220, 200 } }, { { 70, 112, 78 }, { 200, 170, 110 } },
                                      { { 120, 74, 120 }, { 232, 206, 150 } }, { { 196, 160, 98 }, { 96, 60, 44 } }, { { 70, 70, 82 }, { 190, 190, 200 } } };
    const u8 *f = pal[(hd >> 4) % 6u][0], *bd = pal[(hd >> 4) % 6u][1];
    int L = CLAMP(gfx_light(0, 0, -ONE), 60, 200);
    int x0 = mx * TILE, x1 = x0 + TILE, zN = -my * TILE, zS = zN - TILE, B = 28, Y = -3;
    int nW = world_at(mx - 1, my) == 'r', nE = world_at(mx + 1, my) == 'r', nN = world_at(mx, my - 1) == 'r', nS = world_at(mx, my + 1) == 'r';
    SVECTOR a, bb, c, d;
    #define RQ(xa, xb, za, zb, col) { setVector(&a, xa, Y, za); setVector(&bb, xb, Y, za); setVector(&c, xa, Y, zb); setVector(&d, xb, Y, zb); \
        wquad_f(&a, &bb, &c, &d, MIN(255, ((col)[0] * L) >> 7), MIN(255, ((col)[1] * L) >> 7), MIN(255, ((col)[2] * L) >> 7), 1); }
    int ax0 = nW ? x0 : x0 + 10, ax1 = nE ? x1 : x1 - 10, az0 = nN ? zN : zN - 10, az1 = nS ? zS : zS + 10;   // (the rug stops short of the tile's edge: a hand's width of floor all round)
    int fx0 = nW ? ax0 : ax0 + B, fx1 = nE ? ax1 : ax1 - B, fz0 = nN ? az0 : az0 - B, fz1 = nS ? az1 : az1 + B;   // the field inside the border (the quads never overlap: no depth fights)
    RQ(fx0, fx1, fz0, fz1, f)
    if (!nN) RQ(ax0, ax1, az0, fz0, bd)                                  // the border: a strip along each free edge
    if (!nS) RQ(ax0, ax1, fz1, az1, bd)
    if (!nW) RQ(ax0, fx0, fz0, fz1, bd)
    if (!nE) RQ(fx1, ax1, fz0, fz1, bd)
    if (!nN) { static const u8 fr[3] = { 236, 228, 210 }; for (int k = 0; k < 5; k++) { int xa = ax0 + 8 + k * ((ax1 - ax0 - 16) / 5); RQ(xa, xa + 10, zN - 1, az0 + 1, fr) } }   // a fringe along the ends
    if (!nS) { static const u8 fr[3] = { 236, 228, 210 }; for (int k = 0; k < 5; k++) { int xa = ax0 + 8 + k * ((ax1 - ax0 - 16) / 5); RQ(xa, xa + 10, az1 - 1, zS + 1, fr) } }
    #undef RQ
}

int world_dbg_walls;
static inline int open_air(int x, int y) { return x < 0 || y < 0 || x >= MW || y >= MH || dm[y][x] == ' '; }
// a floor slab's edge where an upper storey stops (open air beside it): a plaster band, so floors read as solid
static void fascia(int mx, int my) {
    static const signed char ddx[4] = { 0, 1, 0, -1 }, ddy[4] = { 1, 0, -1, 0 };
    for (int k = 0; k < 4; k++) {
        if (!open_air(mx + ddx[k], my + ddy[k])) continue;
        int x0 = mx * TILE, x1 = x0 + TILE, zN = -my * TILE, zS = zN - TILE, T = 26;
        SVECTOR a, b, c, d;
        if (k == 0) { setVector(&a, x0, 0, zS); setVector(&b, x1, 0, zS); setVector(&c, x0, T, zS); setVector(&d, x1, T, zS); }
        else if (k == 1) { setVector(&a, x1, 0, zS); setVector(&b, x1, 0, zN); setVector(&c, x1, T, zS); setVector(&d, x1, T, zN); }
        else if (k == 2) { setVector(&a, x1, 0, zN); setVector(&b, x0, 0, zN); setVector(&c, x1, T, zN); setVector(&d, x0, T, zN); }
        else { setVector(&a, x0, 0, zN); setVector(&b, x0, 0, zS); setVector(&c, x0, T, zN); setVector(&d, x0, T, zS); }
        int L = gfx_light(ddx[k] * ONE, 0, -ddy[k] * ONE);
        wquad_f(&a, &b, &c, &d, MIN(255, (220 * L) >> 7), MIN(255, (212 * L) >> 7), MIN(255, (196 * L) >> 7), 1);
    }
}
// ---- the view-space position of tile centres, by adding steps (the camera is one matrix a frame, so the view position is linear
// in the tile coordinates): no matrix multiply per tile for the frustum tests ----
typedef struct { int bx, by, bz, xx, xy, xz, yx, yy, yz; } TileView;   // x16 fixed point
static void tile_view_init(TileView *tv, int wx0, int wy, int wz0) {   // (wx0, wy, wz0) = the world point that tile (0,0) stands for
    VECTOR d, v; setVector(&d, wx0 - cam.eye.vx, wy - cam.eye.vy, wz0 - cam.eye.vz);
    ApplyMatrixLV(&cam.view, &d, &v);
    tv->bx = v.vx << 4; tv->by = v.vy << 4; tv->bz = v.vz << 4;
    const MATRIX *m = &cam.view;
    tv->xx = (m->m[0][0] * TILE) >> 8; tv->xy = (m->m[1][0] * TILE) >> 8; tv->xz = (m->m[2][0] * TILE) >> 8;        // one tile east (+x)
    tv->yx = -(m->m[0][2] * TILE) >> 8; tv->yy = -(m->m[1][2] * TILE) >> 8; tv->yz = -(m->m[2][2] * TILE) >> 8;     // one tile south (world -z)
}
static inline void tile_view(const TileView *tv, int mx, int my, int *vx, int *vy, int *vz) {
    *vx = (tv->bx + mx * tv->xx + my * tv->yx) >> 4; *vy = (tv->by + mx * tv->xy + my * tv->yy) >> 4; *vz = (tv->bz + mx * tv->xz + my * tv->yz) >> 4;
}
// does a wall tile (a ball of 340 round its centre, half a wall up) show? The view's edges lean out 0.696 (x) and 0.522 (y) a unit of depth
static inline int tile_seen(const TileView *tv, int mx, int my) {
    int vx, vy, vz; tile_view(tv, mx, my, &vx, &vy, &vz);
    if (vz < Z_NEAR - 340) return 0;
    if (ABS(vx) > ((vz * 89) >> 7) + 415) return 0;
    if (ABS(vy) > ((vz * 67) >> 7) + 384) return 0;
    return 1;
}
static u8 bld[MH][MW];
#define NBLD 251
static u8 b_see[NBLD];                            // (see_through: the buildings looked into this frame)
static int rx0, rx1, ry0, ry1, ex0, ex1, ey0, ey1;
static inline int in_range(int x, int y);
static u8 lvl_bb[NLEVELS][4];
static u32 lvl_edge[NLEVELS][MH][(MW + 31) / 32];
// shell: a storey above the one you are on (only other buildings' outside walls show); focus: the building you are in
static int cut_all;                                     // (this frame: the walls on the camera's side are cut low)
static inline int cut_here(int mx, int my) { int b = bld[my][mx]; return !b || b_see[b]; }   // the walls cut are those of the buildings you are in or look into (and loose ones)
static void draw_layer(int fx, int fy, int t, int cutaway, int lvl, int shell, int focus) {
    // the camera's forward on the ground (unit, 4096), to decide which walls stand between it and the focus
    int cdx = cam.at.vx - cam.eye.vx, cdz = cam.at.vz - cam.eye.vz;
    int len = SquareRoot0(cdx * cdx + cdz * cdz); if (len < 1) len = 1;
    int fdx = (cdx << 12) / len, fdz = (cdz << 12) / len;
    int focx = fx * TILE + TILE / 2, focz = -(fy * TILE + TILE / 2);
    int x0 = rx0, x1 = rx1, y0 = ry0, y1 = ry1;
    extern s16 bfs_par[MH][MW]; u8 (*done)[MW] = (u8 (*)[MW])bfs_par;   // (scratch for this draw: the route finder's parents, lent. The roofs that also borrow it
                                                                         // are rebuilt at the top of world_draw, before any layer is drawn, and a route is only looked for in the update)
    TileView tvF, tvT;
    tile_view_init(&tvF, TILE, 0, -TILE);                           // a 2x2 floor block's centre
    tile_view_init(&tvT, TILE / 2, -WALL_FULL / 2, -TILE / 2);      // a tile's centre, half a wall up
    // 2x2 blocks of one floor draw as a single quad
    for (int my = ey0; my <= ey1; my++) memset(&done[my][ex0], 0, ex1 - ex0 + 1);
    if (!shell) { PROF_BEGIN();
    for (int my = y0; my <= y1; my += 2) for (int mx = x0; mx <= x1; mx += 2) {
        int ok = (mx + 1 < MW && my + 1 < MH);
        int f0 = dfl[my][mx];
        if (ok) ok = f0 != FL_WATER && dfl[my][mx + 1] == f0 && dfl[my + 1][mx] == f0 && dfl[my + 1][mx + 1] == f0 && !(nat_on && !lvl && nat_floor(f0));
        if (ok && lvl) ok = dm[my][mx] != ' ' && dm[my][mx + 1] != ' ' && dm[my + 1][mx] != ' ' && dm[my + 1][mx + 1] != ' ';
        if (ok && ((bld[my][mx] && !b_see[bld[my][mx]]) || (bld[my + 1][mx + 1] && !b_see[bld[my + 1][mx + 1]]))) { done[my][mx] = done[my][mx + 1] = done[my + 1][mx] = done[my + 1][mx + 1] = 2; continue; }   // inside someone else's building: under its roof
        done[my][mx] = done[my][mx + 1] = done[my + 1][mx] = done[my + 1][mx + 1] = 0;
        if (!ok) continue;
        int vx, vy, vz; tile_view(&tvF, mx, my, &vx, &vy, &vz);
        if (vz < -TILE * 2) continue;
        if (ABS(vx) > (vz * 3) / 4 + TILE * 3) continue;
        if (ABS(vy) > (vz * 5) / 8 + TILE * 4) continue;
        gfx_floor_pass = !lvl; floor_block(mx, my); gfx_floor_pass = 0;
        done[my][mx] = done[my][mx + 1] = done[my + 1][mx] = done[my + 1][mx + 1] = 1;
    }
    if (nat_on && !lvl) nat_ground(done, x0, x1, y0, y1);
    PROF_END(0); }
    int lx0 = ex0, lx1 = ex1, ly0 = ey0, ly1 = ey1;
    if (lvl) { lx0 = MAX(lx0, lvl_bb[lvl][0]); ly0 = MAX(ly0, lvl_bb[lvl][1]); lx1 = MIN(lx1, lvl_bb[lvl][2]); ly1 = MIN(ly1, lvl_bb[lvl][3]); }
    for (int my = ly0; my <= ly1; my++) { const u32 *em = shell ? lvl_edge[lvl][my] : 0;   // (a storey seen from outside: only its edge)
      for (int mx = lx0; mx <= lx1; mx++) {
        if (em) { u32 w = em[mx >> 5] >> (mx & 31); if (!w) { mx |= 31; continue; } if (!(w & 1)) continue; }
        int wx = mx * TILE + TILE / 2, wz = -(my * TILE + TILE / 2);
        char c = dm[my][mx];
        if (c == ' ') continue;                                       // open air (upper storeys)
        int inR = mx >= rx0 && mx <= rx1 && my >= ry0 && my <= ry1;
        if (!inR && !in_range(mx, my)) continue;                     // past the range (a building cut by it is drawn whole)
        if (shell || !inR) {                                         // a storey seen from outside, or a far part of a building: its outside walls only
            if (shell && bld[my][mx] && b_see[bld[my][mx]]) continue;
            if (shell && cut_all && cut_here(mx, my) && (((wx - focx) * fdx) >> 12) + (((wz - focz) * fdz) >> 12) < -TILE / 4) continue;   // (over walls cut low: it would float)
            if (!lvl && !is_wall(c)) continue;
            if (!tile_seen(&tvT, mx, my)) continue;
            if (!is_wall(c) || !(open_air(mx - 1, my) || open_air(mx + 1, my) || open_air(mx, my - 1) || open_air(mx, my + 1) || !lvl)) { if (lvl && shell) fascia(mx, my); continue; }
            if (!lvl) { char n4 = 0; if (mx > 0 && mx < MW - 1 && my > 0 && my < MH - 1) n4 = (dm[my][mx - 1] != '.' && dm[my][mx + 1] != '.' && dm[my - 1][mx] != '.' && dm[my + 1][mx] != '.'); if (n4) continue; }
            { PROF_BEGIN(); ws_lite = bld[my][mx] != 0; wall_tile(mx, my, c == 'w' ? WALL_LOW : WALL_FULL, 0); ws_lite = 0; world_dbg_walls++; PROF_END(1); }
            if (lvl) fascia(mx, my);
            continue;
        }
        if (done[my][mx] && c == '.' && !lvl) continue;              // floor already drawn, nothing on it
        { int b = bld[my][mx];                                       // inside a building you are not in: its roof hides the rooms (only its walls show)
          if (b && !b_see[b] && !is_wall(c) && c != 'D' && !world_roof_show) continue; }
        // frustum: the tile centre in view space (a wall tile fits a ball of 340 round it; things on a tile can stand taller)
        if (is_wall(c) || c == '.') { if (!tile_seen(&tvT, mx, my)) continue; }
        else { int vx, vy, vz, pm = (nat_on && !lvl && nat_plant_char(c)) ? TILE * 2 : 0; tile_view(&tvT, mx, my, &vx, &vy, &vz);   // (a plant reaches further)
          if (vz < -TILE - pm) continue;
          if (ABS(vx) > (vz * 3) / 4 + TILE * 2 + pm) continue;
          if (ABS(vy) > (vz * 5) / 8 + TILE * 3 + pm) continue; }
        if (!done[my][mx]) { PROF_BEGIN(); gfx_floor_pass = !lvl; floor_tile(mx, my); gfx_floor_pass = 0; PROF_END(0); }
        if (lvl) { fascia(mx, my); if (c == '.') continue; }
        if (is_wall(c)) {
            int h = (c == 'w') ? WALL_LOW : WALL_FULL, cut = 0;
            if (cutaway && c != 'w' && cut_here(mx, my)) {
                int along = (((wx - focx) * fdx) >> 12) + (((wz - focz) * fdz) >> 12);   // distance past the focus along the view
                if (along < -TILE / 4) { h = WALL_LOW; cut = 1; }
            }
            { PROF_BEGIN(); ws_lite = bld[my][mx] && !b_see[bld[my][mx]] && !world_roof_show; wall_tile(mx, my, h, cut); ws_lite = 0; world_dbg_walls++; PROF_END(1); }
        } else if (c != '.') {
            if (c == 'D') { int along = (((wx - focx) * fdx) >> 12) + (((wz - focz) * fdz) >> 12); door_tile(mx, my, (cutaway && cut_here(mx, my) && along < -TILE / 4) ? WALL_LOW : WALL_FULL); continue; }   // (the doorway is its door: no mat on the floor)
            if (c == 'r') { rug_tile(mx, my); continue; }
            if (nat_on && !lvl && nat_plant_char(c)) {
                if (ABS(mx - fx) > 10 || ABS(my - fy) > 10) continue;   // (further off, the far cells' woods stand for them)
                int al = (((wx - focx) * fdx) >> 12) + (((wz - focz) * fdz) >> 12), ac = (((wx - focx) * fdz) >> 12) - (((wz - focz) * fdx) >> 12);
                int fade = al < TILE / 2 && al > -TILE * 4 && ABS(ac) < TILE * 3 / 2;   // (between the camera and you: see-through)
                PROF_BEGIN(); nat_plant(c, mx, my, t, fade); PROF_END(2);
                continue;
            }
            int rot = items_rotates(c) ? world_item_facing(mx, my) : 0;
            int along = items_along(c) ? world_along(mx, my, c) : 0;
            { PROF_BEGIN(); items_draw(c, mx, my, rot, along, t); PROF_END(2); }
        }
      }
    }
}


// ================================================================== ROOFS
// Every enclosed building gets a roof over its top storey, worked out from its footprint: the height at each grid corner is
// its distance in from the footprint's edge (chessboard distance for a HIP roof, across the short side for a GABLE, none
// for FLAT), capped so a big building levels off. Eaves overhang the walls; gable ends are filled with plaster walls.
// (packed: a corner's height is 0..3, a tile is under a roof or not; 5 storeys of each as bytes were 65 KB)
#define RHW ((MW + 2) / 2)
static u8 roofH[NLEVELS][MH + 1][RHW];           // corner heights, in pitch steps: a nibble each (the low nibble is the even X)
static u32 roofM[NLEVELS][MH][(MW + 31) / 32];   // 1: this tile is under the roof of storey l (a bit each, as lvl_edge)
static inline int rh_get(int l, int X, int Y) { return (roofH[l][Y][X >> 1] >> ((X & 1) << 2)) & 15; }
static inline void rh_set(int l, int X, int Y, int h) { u8 *p = &roofH[l][Y][X >> 1]; int s = (X & 1) << 2; *p = (u8)((*p & ~(15 << s)) | (h << s)); }
static inline int rm_get(int l, int x, int y) { return (roofM[l][y][x >> 5] >> (x & 31)) & 1; }
static inline void rm_set(int l, int x, int y) { roofM[l][y][x >> 5] |= 1u << (x & 31); }
static u8 bld[MH][MW];                             // the building each ground tile belongs to (0: outdoors)
static u8 roof_tex_ok; static TexRef roof_tex;
static int enclose(char c) { return c == 'W' || c == 'E' || c == 'e' || c == 'D'; }
// The buildings: every connected indoor area of the ground storey, its footprint (a box round it), how many storeys it has, its
// roof (in SAN BOREAS each building has its own: style, colour and pitch from its own dice; elsewhere the build-mode choice)
// and its wall colour (for the far skyline).
static u8 b_x0[NBLD], b_y0[NBLD], b_x1[NBLD], b_y1[NBLD], b_top[NBLD], b_sty[NBLD], b_col[NBLD], b_pit[NBLD], b_str[NBLD];
static u16 b_wall[NBLD]; static int nbld;
extern u16 bfs_q[]; extern s16 bfs_par[MH][MW];   // (sim.c's route-finding buffers: the roof rebuild borrows them)
static u8 lvl_bb[NLEVELS][4];                     // each storey's built-on box (x0, y0, x1, y1)
static u32 lvl_edge[NLEVELS][MH][(MW + 31) / 32];  // each storey's tiles next to open air (a bit each)
// Buildings sit inside their own blocks (a street runs along every block's top and left), so when the SAN BOREAS window moves,
// what was worked out for the blocks that stay is moved with them and only the blocks coming in are worked out (a few a frame).
static int bld_alloc(void) { for (int b = 1; b < NBLD; b++) if (b_x0[b] == 255) { if (b > nbld) nbld = b; return b; } return NBLD - 1; }
static u8 rbox[NLEVELS][4];
int dbg_rr_hb, dbg_rr_n, dbg_rr_blk, dbg_ws[4];
// the roofs and buildings of the box [x0..x1] x [y0..y1] (the whole map, or a block that has just come in)
static void roof_region(int x0, int y0, int x1, int y1) {
    int t0 = gfx_hb(); dbg_rr_n++;
    u8 (*out)[MW] = (u8 (*)[MW])bfs_par; u16 *q = bfs_q;
    u8 (*Dd)[MW] = (u8 (*)[MW])((u8 *)bfs_par + MH * MW);
    typedef const char (*Row)[MW + 1];
    Row mp[NLEVELS + 1];
    mp[0] = (Row)lifeMap;
    for (int l = 1; l <= NLEVELS; l++) { const Level *L = l < NLEVELS ? lv_get(l - 1) : 0; mp[l] = L ? (Row)L->map : 0; }
    int full = x0 == 0 && y0 == 0 && x1 == MW - 1 && y1 == MH - 1;
    // what was worked out here before goes
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { int b = bld[y][x]; if (b) { b_x0[b] = 255; bld[y][x] = 0; } }
    for (int l = 0; l < NLEVELS; l++) {
        if (full) { memset(roofM[l], 0, sizeof roofM[l]); memset(roofH[l], 0, sizeof roofH[l]); }
        else { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) roofM[l][y][x >> 5] &= ~(1u << (x & 31));
               for (int Y = y0; Y <= y1 + 1; Y++) for (int X = x0; X <= x1 + 1; X++) rh_set(l, X, Y, 0); }
        if (l) { if (full) memset(lvl_edge[l], 0, sizeof lvl_edge[l]);
                 else for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) lvl_edge[l][y][x >> 5] &= ~(1u << (x & 31)); }
    }
    for (int l = 0; l < NLEVELS; l++) {
        Row m = mp[l]; rbox[l][2] = 0; rbox[l][0] = 1;           // (empty)
        if (!m) continue;
        int bx0 = MW, by0 = MH, bx1 = -1, by1 = -1;
        if (l == 0) { bx0 = x0; by0 = y0; bx1 = x1; by1 = y1; }
        else for (int y = y0; y <= y1; y++) { const char *row = m[y];
            for (int x = x0; x <= x1; x++) if (row[x] != ' ') { if (x < bx0) bx0 = x; if (x > bx1) bx1 = x; if (y < by0) by0 = y; if (y > by1) by1 = y; } }
        if (bx1 < 0) continue;                                   // nothing built on this storey here
        if (l) {   // the storey's edge: what stands next to open air (all that shows of it from outside: its outside walls, the floor's edge)
            for (int y = by0; y <= by1; y++) for (int x = bx0; x <= bx1; x++) {
                if (m[y][x] == ' ') continue;
                int air = x == 0 || y == 0 || x == MW - 1 || y == MH - 1 || m[y][x - 1] == ' ' || m[y][x + 1] == ' ' || m[y - 1][x] == ' ' || m[y + 1][x] == ' ';
                if (air) lvl_edge[l][y][x >> 5] |= 1u << (x & 31);
            }
            if (!lvl_used[l]) { lvl_used[l] = 1; lvl_bb[l][0] = (u8)bx0; lvl_bb[l][1] = (u8)by0; lvl_bb[l][2] = (u8)bx1; lvl_bb[l][3] = (u8)by1; }
            else { if (bx0 < lvl_bb[l][0]) lvl_bb[l][0] = (u8)bx0; if (by0 < lvl_bb[l][1]) lvl_bb[l][1] = (u8)by0;
                   if (bx1 > lvl_bb[l][2]) lvl_bb[l][2] = (u8)bx1; if (by1 > lvl_bb[l][3]) lvl_bb[l][3] = (u8)by1; }
        }
        int X0 = MAX(x0, bx0 - 1), Y0 = MAX(y0, by0 - 1), X1 = MIN(x1, bx1 + 1), Y1 = MIN(y1, by1 + 1);
        rbox[l][0] = (u8)X0; rbox[l][1] = (u8)Y0; rbox[l][2] = (u8)X1; rbox[l][3] = (u8)Y1;
        // outdoors: everything the box's edge reaches without crossing a wall, a door or a window (past it a storey is all air)
        for (int y = Y0; y <= Y1; y++) memset(&out[y][X0], 0, X1 - X0 + 1);
        int qh = 0, qt = 0;
        #define SEED(x, y) if (!out[y][x] && !enclose(m[y][x])) { out[y][x] = 1; q[qt++] = (u16)(((y) << 8) | (x)); }
        for (int x = X0; x <= X1; x++) { SEED(x, Y0) SEED(x, Y1) }
        for (int y = Y0 + 1; y < Y1; y++) { SEED(X0, y) SEED(X1, y) }
        while (qh < qt) { int c = q[qh++], cx = c & 255, cy = c >> 8;
            if (cx > X0) SEED(cx - 1, cy)
            if (cx < X1) SEED(cx + 1, cy)
            if (cy > Y0) SEED(cx, cy - 1)
            if (cy < Y1) SEED(cx, cy + 1)
        }
        #undef SEED
        Row up = l < NUP ? mp[l + 1] : 0;                        // a storey above covers it: no roof there
        for (int y = Y0; y <= Y1; y++) for (int x = X0; x <= X1; x++) {
            if (out[y][x]) continue;
            if (up && up[y][x] != ' ') continue;
            rm_set(l, x, y);
        }
        if (l == 0) {                                            // the buildings: connected indoor areas of the ground storey
            for (int y = Y0; y <= Y1; y++) for (int x = X0; x <= X1; x++) {
                if (out[y][x] || bld[y][x]) continue;
                int id = bld_alloc();
                b_x0[id] = b_x1[id] = (u8)x; b_y0[id] = b_y1[id] = (u8)y; b_top[id] = 0; b_wall[id] = 0;
                qh = qt = 0; bld[y][x] = (u8)id; q[qt++] = (u16)((y << 8) | x);
                while (qh < qt) { int c = q[qh++], cx = c & 255, cy = c >> 8;
                    if (cx < b_x0[id]) b_x0[id] = (u8)cx; if (cx > b_x1[id]) b_x1[id] = (u8)cx; if (cy < b_y0[id]) b_y0[id] = (u8)cy; if (cy > b_y1[id]) b_y1[id] = (u8)cy;
                    if (!b_wall[id] && is_wall(lifeMap[cy][cx])) b_wall[id] = wp_top[wallMap[cy][cx] % NWALLS] | 0x8000;
                    #define NB(nx, ny) if (!out[ny][nx] && !bld[ny][nx]) { bld[ny][nx] = (u8)id; q[qt++] = (u16)(((ny) << 8) | (nx)); }
                    if (cx > X0) NB(cx - 1, cy)
                    if (cx < X1) NB(cx + 1, cy)
                    if (cy > Y0) NB(cx, cy - 1)
                    if (cy < Y1) NB(cx, cy + 1)
                    #undef NB
                } }
        }
    }
    for (int l = 1; l < NLEVELS; l++) if (rbox[l][2] >= rbox[l][0])   // how tall each building is
        for (int y = rbox[l][1]; y <= rbox[l][3]; y++) for (int x = rbox[l][0]; x <= rbox[l][2]; x++) { int b = bld[y][x]; if (b && rm_get(l, x, y) && l > b_top[b]) b_top[b] = (u8)l; }
    for (int b = 1; b <= nbld; b++) {                            // each building's roof
        if (b_x0[b] == 255) continue;
        if (world_zone == ZONE_CITY && world_wbx >= 0) {
            u32 h = bm_hash(world_wbx * BM_BLK + b_x0[b], world_wby * BM_BLK + b_y0[b], wsalt ^ 0x500Fu);
            int w = b_x1[b] - b_x0[b] + 1, d = b_y1[b] - b_y0[b] + 1;
            b_sty[b] = (u8)(b_top[b] >= 2 || (w >= 11 && d >= 9 && (h & 3) == 0) ? 2 : (h >> 2) & 1);   // towers and big blocks: flat roofs
            b_col[b] = (u8)((h >> 4) % NROOF_COL); b_pit[b] = (u8)(1 + ((h >> 8) & 1));
        } else { b_sty[b] = (u8)world_roof_style; b_col[b] = (u8)world_roof_col; b_pit[b] = (u8)world_roof_pitch; }
    }
    b_sty[0] = (u8)world_roof_style; b_col[0] = (u8)world_roof_col; b_pit[0] = (u8)world_roof_pitch;
    for (int l = 0; l < NLEVELS; l++) {
        if (rbox[l][2] < rbox[l][0]) continue;
        int X0 = rbox[l][0], Y0 = rbox[l][1], X1 = rbox[l][2], Y1 = rbox[l][3];
        #define M_(y, x) rm_get(l, x, y)
        // the distance in from the roof's edge (chessboard: a forward and a backward sweep), for the hip roofs
        for (int y = Y0; y <= Y1; y++) for (int x = X0; x <= X1; x++) {
            if (!M_(y, x)) { Dd[y][x] = 0; continue; }
            int v = x > X0 ? Dd[y][x - 1] : 0;
            if (y > Y0) { int a = Dd[y - 1][x]; if (a < v) v = a;
                          a = x > X0 ? Dd[y - 1][x - 1] : 0; if (a < v) v = a;
                          a = x < X1 ? Dd[y - 1][x + 1] : 0; if (a < v) v = a; }
            else v = 0;
            Dd[y][x] = (u8)MIN(v + 1, 250);
        }
        for (int y = Y1; y >= Y0; y--) for (int x = X1; x >= X0; x--) {
            if (!M_(y, x)) continue;
            int v = x < X1 ? Dd[y][x + 1] : 0;
            if (y < Y1) { int a = Dd[y + 1][x]; if (a < v) v = a;
                          a = x < X1 ? Dd[y + 1][x + 1] : 0; if (a < v) v = a;
                          a = x > X0 ? Dd[y + 1][x - 1] : 0; if (a < v) v = a; }
            else v = 0;
            if (v + 1 < Dd[y][x]) Dd[y][x] = (u8)(v + 1);
        }
        for (int y = Y0; y <= Y1; y++) for (int x = X0; x <= X1; x++) {
            if (!M_(y, x)) continue;
            int st = b_sty[bld[y][x]];
            if (st == 0) { if (Dd[y][x] > 4) Dd[y][x] = 4; }
            else if (st == 1) {                                  // GABLE: the ridge runs along each building's longer side
                int up = 0, dn = 0, lf = 0, rt = 0;
                while (y - up - 1 >= 0 && M_(y - up - 1, x)) up++;
                while (y + dn + 1 < MH && M_(y + dn + 1, x)) dn++;
                while (x - lf - 1 >= 0 && M_(y, x - lf - 1)) lf++;
                while (x + rt + 1 < MW && M_(y, x + rt + 1)) rt++;
                int across_y = MIN(up, dn) + 1, across_x = MIN(lf, rt) + 1;
                Dd[y][x] = (u8)((up + dn) <= (lf + rt) ? across_y : across_x);
            } else Dd[y][x] = 0;                                  // FLAT
        }
        // the corners: the lowest of the four tiles round each (capped: wide buildings level off). On the box's edge: 0
        for (int Y = Y0 + 1; Y <= Y1; Y++) { const u8 *ra = Dd[Y - 1], *rb_ = Dd[Y];
            for (int X = X0 + 1; X <= X1; X++) {
                int mn = ra[X - 1]; if (ra[X] < mn) mn = ra[X]; if (rb_[X - 1] < mn) mn = rb_[X - 1]; if (rb_[X] < mn) mn = rb_[X];
                rh_set(l, X, Y, MIN(mn, 3));
            } }
        #undef M_
    }
    int t = gfx_hb() - t0; if (t > dbg_rr_hb) dbg_rr_hb = t; if (!full && t > dbg_rr_blk) dbg_rr_blk = t;
}
static void roof_rebuild(void) {
    for (int b = 0; b < NBLD; b++) b_x0[b] = 255;
    nbld = 0;
    memset(bld, 0, sizeof bld);
    for (int l = 0; l < NLEVELS; l++) lvl_used[l] = 0;
    lvl_used[0] = 1; lvl_bb[0][0] = 0; lvl_bb[0][1] = 0; lvl_bb[0][2] = MW - 1; lvl_bb[0][3] = MH - 1;
    roof_region(0, 0, MW - 1, MH - 1);
    roof_dirty = 0; memset(rq_dirty, 0, sizeof rq_dirty); rq_n = 0;
}
// the window moved dx, dy tiles (whole blocks): the roofs, the buildings and the storeys' edges move with it
static void bits_shift(u32 (*rows)[(MW + 31) / 32], int dx, int dy) {   // a bit a tile: the map slid by (dx, dy) tiles
    static u32 tmp[MH][(MW + 31) / 32];
    memset(tmp, 0, sizeof tmp);
    for (int y = 0; y < MH; y++) { int ys = y + dy; if (ys < 0 || ys >= MH) continue;
        for (int x = 0; x < MW; x++) { int xs = x + dx; if (xs >= 0 && xs < MW && ((rows[ys][xs >> 5] >> (xs & 31)) & 1)) tmp[y][x >> 5] |= 1u << (x & 31); } }
    memcpy(rows, tmp, sizeof tmp);
}
static void roof_shift(int dx, int dy) {
    for (int l = 0; l < NLEVELS; l++) {
        shift2d(&roofH[l][0][0], RHW, RHW, MH + 1, dx / 2, dy, 0);   // (whole blocks of 16 tiles: 8 bytes of nibbles)
        bits_shift(roofM[l], dx, dy);
        if (l) {
            bits_shift(lvl_edge[l], dx, dy);
            if (lvl_used[l]) {
                int a = lvl_bb[l][0] - dx, b = lvl_bb[l][1] - dy, c = lvl_bb[l][2] - dx, d = lvl_bb[l][3] - dy;
                a = MAX(a, 0); b = MAX(b, 0); c = MIN(c, MW - 1); d = MIN(d, MH - 1);
                if (c < a || d < b) lvl_used[l] = 0;
                else { lvl_bb[l][0] = (u8)a; lvl_bb[l][1] = (u8)b; lvl_bb[l][2] = (u8)c; lvl_bb[l][3] = (u8)d; }
            }
        }
    }
    shift2d(&bld[0][0], MW, MW, MH, dx, dy, 0);
    for (int b = 1; b <= nbld; b++) {
        if (b_x0[b] == 255) continue;
        int a = b_x0[b] - dx, c = b_x1[b] - dx, e = b_y0[b] - dy, f = b_y1[b] - dy;
        if (a < 0 || e < 0 || c >= MW || f >= MH) { b_x0[b] = 255; continue; }   // gone out of the window
        b_x0[b] = (u8)a; b_x1[b] = (u8)c; b_y0[b] = (u8)e; b_y1[b] = (u8)f;
    }
    // (no building's tiles may point at a freed one: a building is either kept whole or gone whole, so bld's gone parts left too)
}
static void roof_pending(int max) {                            // work out up to max of the blocks still waiting
    for (int wy = 0; wy < WB && max > 0; wy++) for (int wx = 0; wx < WB && max > 0; wx++) {
        if (!rq_dirty[wy][wx]) continue;
        rq_dirty[wy][wx] = 0; rq_n--; max--;
        roof_region(wx * BM_BLK, wy * BM_BLK, wx * BM_BLK + BM_BLK - 1, wy * BM_BLK + BM_BLK - 1);
    }
}
static void roof_tex_make(void) {   // shingles: courses of overlapping tiles, every other course offset
    static u16 px[16 * 16];
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        int course = y / 4, off = (course & 1) * 4, v = 15 - (y & 3);          // each shingle darkens toward its lower edge
        if (((x + off) & 7) == 0) v -= 5;                                       // the gaps between shingles
        if ((y & 3) == 3) v -= 6;
        v += ((x * 7 + y * 13) & 3) - 1;
        v = CLAMP(v, 4, 20); px[y * 16 + x] = RGB(v, v, v);
    }
    roof_tex = atlas_add(16, 16, px); roof_tex_ok = 1;
}
static inline int rmask(int l, int x, int y) { return x >= 0 && y >= 0 && x < MW && y < MH && rm_get(l, x, y); }
static int rc_sty;
static void roof_corner(int l, int X, int Y, SVECTOR *v, int base, int pitch) {   // a roof corner, the eaves pushed out at the footprint's edge
    int h = rh_get(l, X, Y), ox = 0, oz = 0, OVH = 34;
    if (!rmask(l, X - 1, Y - 1) && !rmask(l, X - 1, Y)) ox = -OVH;
    if (!rmask(l, X, Y - 1) && !rmask(l, X, Y)) ox = OVH;
    if (!rmask(l, X - 1, Y - 1) && !rmask(l, X, Y - 1)) oz = OVH;
    if (!rmask(l, X - 1, Y) && !rmask(l, X, Y)) oz = -OVH;
    if (rc_sty == 2) { ox = ox / 2; oz = oz / 2; }               // a flat roof: a short lip, no slope
    int drop = (h == 0 && rc_sty != 2) ? (OVH * pitch) >> 8 : 0;
    setVector(v, X * TILE + ox, base - h * pitch + drop - (rc_sty == 2 ? 10 : 0), -Y * TILE + oz);
}
static int rc_l, rc_base, rc_pitch;
static const XV *cc_roof_at(int X, int Y) {          // a roof corner (eaves and all), transformed once a frame
    CornerCache *c = &cc_roof; int i = X - c->x0, j = Y - c->y0;
    SVECTOR s;
    if ((unsigned)i > CW || (unsigned)j > CW) { XV *t = &cc_tmp[cc_tmpi++ & 7]; roof_corner(rc_l, X, Y, &s, rc_base, rc_pitch); gfx_xform(&s, t, 1); return t; }
    if (c->st[j][i] != c->gen) { roof_corner(rc_l, X, Y, &s, rc_base, rc_pitch); gfx_xform(&s, &c->v[j][i], 1); c->st[j][i] = c->gen; }
    return &c->v[j][i];
}
static int rx0, rx1, ry0, ry1, ex0, ex1, ey0, ey1;           // this frame's detail range, and that range grown to take in the buildings it cuts through
static inline int in_range(int x, int y) {
    if (x >= rx0 && x <= rx1 && y >= ry0 && y <= ry1) return 1;
    int b = bld[y][x]; return b && b_str[b];
}
static u32 gab_col[4];   // (roofs_draw: the gable walls' colour facing each way, worked out once a frame)
static void gables(int l, int x, int y, int base, int pitch) {   // the gable ends: where the roof stands above a footprint edge, a wall closes it
    static const signed char ex[4] = { 0, 1, 0, -1 }, ey[4] = { 1, 0, -1, 0 };
    for (int k = 0; k < 4; k++) {
        if (rmask(l, x + ex[k], y + ey[k])) continue;
        int X1, Y1, X2, Y2;   // the edge's two corners, in the order that faces outward
        if (k == 0) { X1 = x; Y1 = y + 1; X2 = x + 1; Y2 = y + 1; } else if (k == 1) { X1 = x + 1; Y1 = y + 1; X2 = x + 1; Y2 = y; }
        else if (k == 2) { X1 = x + 1; Y1 = y; X2 = x; Y2 = y; } else { X1 = x; Y1 = y; X2 = x; Y2 = y + 1; }
        int h1 = rh_get(l, X1, Y1), h2 = rh_get(l, X2, Y2);
        if (!h1 && !h2) continue;
        SVECTOR a, b2, cc, dd;
        setVector(&a, X1 * TILE, base - h1 * pitch, -Y1 * TILE); setVector(&b2, X2 * TILE, base - h2 * pitch, -Y2 * TILE);
        setVector(&cc, X1 * TILE, base, -Y1 * TILE); setVector(&dd, X2 * TILE, base, -Y2 * TILE);
        u32 g = gab_col[k];
        gfx_quad_f(&a, &b2, &cc, &dd, g & 255, (g >> 8) & 255, g >> 16, 1);
    }
}
// a 2x2 of roof tiles far off that lie in one plane, under one building: drawn as one quad (half the roof's work out there)
static int roof_merge_ok(int l, int x, int y, int b) {
    if (x + 2 > MW || y + 2 > MH) return 0;
    for (int j = 0; j < 2; j++) for (int i = 0; i < 2; i++) if (!rm_get(l, x + i, y + j) || bld[y + j][x + i] != b || !in_range(x + i, y + j)) return 0;
    int h00 = rh_get(l, x, y), ax = rh_get(l, x + 1, y) - h00, ay = rh_get(l, x, y + 1) - h00;
    for (int j = 0; j <= 2; j++) for (int i = 0; i <= 2; i++) if (rh_get(l, x + i, y + j) != h00 + i * ax + j * ay) return 0;
    return 1;
}
static void chimney(int l, int X, int Y, int base, int pitch) {   // a brick chimney through the roof, standing at that corner
    int h = rh_get(l, X, Y); if (h < 1) return;
    int ry = base - h * pitch, yt = ry - 84, yb = ry + 24, x0 = X * TILE - 22, x1 = X * TILE + 22, z0 = -Y * TILE + 22, z1 = -Y * TILE - 22;
    SVECTOR a, b, c, d;
    #define CQ(ax,ay,az,bx,by,bz,cx,cy,cz,dx,dy,dz,r,g,bl) { setVector(&a, ax, ay, az); setVector(&b, bx, by, bz); setVector(&c, cx, cy, cz); setVector(&d, dx, dy, dz); wquad_f(&a, &b, &c, &d, r, g, bl, 1); }
    CQ(x0, yt, z1, x1, yt, z1, x0, yb, z1, x1, yb, z1, 150, 74, 58)      // south
    CQ(x1, yt, z0, x0, yt, z0, x1, yb, z0, x0, yb, z0, 128, 62, 48)      // north
    CQ(x0, yt, z0, x0, yt, z1, x0, yb, z0, x0, yb, z1, 120, 58, 46)      // west
    CQ(x1, yt, z1, x1, yt, z0, x1, yb, z1, x1, yb, z0, 138, 68, 52)      // east
    CQ(x0 - 4, yt, z0 + 4, x1 + 4, yt, z0 + 4, x0 - 4, yt, z1 - 4, x1 + 4, yt, z1 - 4, 74, 64, 60)   // the cap
    #undef CQ
}
static void roofs_draw(int fx, int fy) {
    if (!roof_tex_ok) roof_tex_make();
    int focus = (fx >= 0 && fy >= 0 && fx < MW && fy < MH) ? bld[fy][fx] : 0;
    gfx_set_identity();
    { static const signed char ex[4] = { 0, 1, 0, -1 }, ey[4] = { 1, 0, -1, 0 };
      for (int k = 0; k < 4; k++) { int Lw = gfx_light(ex[k] * ONE, 0, -ey[k] * ONE); gab_col[k] = (u32)MIN(255, (226 * Lw) >> 7) | ((u32)MIN(255, (214 * Lw) >> 7) << 8) | ((u32)MIN(255, (190 * Lw) >> 7) << 16); } }
    u32 skip_a[(MW + 31) / 32], skip_b[(MW + 31) / 32];
    for (int l = 0; l < NLEVELS; l++) {
        if (!lvl_used[l]) continue;
        u32 *skip_cur = skip_a, *skip_nx = skip_b; memset(skip_a, 0, sizeof skip_a); memset(skip_b, 0, sizeof skip_b);
        int base = -(l + 1) * WALL_FULL;
        TileView tv; tile_view_init(&tv, TILE / 2, base - 170, -TILE / 2);
        rc_l = l; rc_base = base; cc_begin(&cc_roof, ex0, ey0, base);
        int cur_b = -1, pitch = 170; const u8 *rc = roofCol[0];
        int lx0 = ex0, ly0 = ey0, lx1 = ex1, ly1 = ey1;                // (an upper storey's roofs lie in its built-on box, a tile round)
        if (l) { lx0 = MAX(lx0, lvl_bb[l][0] - 1); ly0 = MAX(ly0, lvl_bb[l][1] - 1); lx1 = MIN(lx1, lvl_bb[l][2] + 1); ly1 = MIN(ly1, lvl_bb[l][3] + 1); }
        for (int y = ly0; y <= ly1; y++) { const u32 *row = roofM[l][y];
          { u32 *t = skip_cur; skip_cur = skip_nx; skip_nx = t; memset(skip_nx, 0, sizeof skip_a); }   // (the merged 2x2s' lower halves)
          for (int x = lx0; x <= lx1; x++) {
            { u32 w = row[x >> 5] >> (x & 31); if (!w) { x |= 31; continue; } if (!(w & 1)) continue; }   // (32 tiles of open air at a time)
            if ((skip_cur[x >> 5] >> (x & 31)) & 1) continue;
            if (!in_range(x, y)) continue;
            int b = bld[y][x];
            if (b && b_see[b] && !world_fp && (!world_roof_show || l > world_view_lvl)) { if (!world_roof_show) continue; }   // the building you are in (or look through) shows its rooms (first person: its roof is the ceiling)
            if (b != cur_b) { cur_b = b; pitch = roofPitch[b_pit[b] % NROOF_PITCH]; rc = roofCol[b_col[b] % NROOF_COL]; rc_sty = b_sty[b]; rc_pitch = pitch; }
            if (rc_sty == 2 && b && b_top[b] == l) {                // a flat roof: the building's whole top in one slab (drawn from its first tile)
                if (x != b_x0[b] || y != b_y0[b]) { if (!rm_get(l, b_x0[b], b_y0[b])) {} else continue; }
                else {
                    int X0 = b_x0[b] * TILE - 17, X1 = (b_x1[b] + 1) * TILE + 17, Z0 = -b_y0[b] * TILE + 17, Z1 = -(b_y1[b] + 1) * TILE - 17, Y = base - 10;
                    SVECTOR a, bb, cc, dd; setVector(&a, X0, Y, Z0); setVector(&bb, X1, Y, Z0); setVector(&cc, X0, Y, Z1); setVector(&dd, X1, Y, Z1);
                    int L = CLAMP(gfx_light(0, -ONE, 0), 60, 220);
                    wquad_f(&a, &bb, &cc, &dd, (118 * L) >> 7, (116 * L) >> 7, (112 * L) >> 7, !world_fp);
                    setVector(&a, X0, Y - 24, Z1); setVector(&bb, X1, Y - 24, Z1); setVector(&cc, X0, Y, Z1); setVector(&dd, X1, Y, Z1); wquad_f(&a, &bb, &cc, &dd, 150, 146, 140, 1);   // the parapet
                    setVector(&a, X1, Y - 24, Z0); setVector(&bb, X0, Y - 24, Z0); setVector(&cc, X1, Y, Z0); setVector(&dd, X0, Y, Z0); wquad_f(&a, &bb, &cc, &dd, 150, 146, 140, 1);
                    setVector(&a, X0, Y - 24, Z0); setVector(&bb, X0, Y - 24, Z1); setVector(&cc, X0, Y, Z0); setVector(&dd, X0, Y, Z1); wquad_f(&a, &bb, &cc, &dd, 132, 128, 124, 1);
                    setVector(&a, X1, Y - 24, Z1); setVector(&bb, X1, Y - 24, Z0); setVector(&cc, X1, Y, Z1); setVector(&dd, X1, Y, Z0); wquad_f(&a, &bb, &cc, &dd, 132, 128, 124, 1);
                    continue;
                }
            }
            int vx, vy, vz; tile_view(&tv, x, y, &vx, &vy, &vz);
            if (vz < -TILE) continue;
            if (ABS(vx) > (vz * 3) / 4 + TILE * 2) continue;
            if (ABS(vy) > (vz * 5) / 8 + TILE * 3) continue;
            int big = !world_fp && !(x & 1) && !(y & 1) && vz > TILE * 7 && x + 1 <= lx1 && y + 1 <= ly1 && roof_merge_ok(l, x, y, b);
            int sp = big ? 2 : 1;
            if (!big && b && rc_sty != 2 && l == b_top[b] && x == b_x0[b] + 2 && y == b_y0[b] + 1 && b_x1[b] - b_x0[b] >= 5 && ((b_x0[b] * 7 + b_y0[b] * 13) & 1) && vz < TILE * 14) chimney(l, x, y, base, pitch);   // (a chimney on every other home, close up)
            const XV *A = cc_roof_at(x, y), *B = cc_roof_at(x + sp, y), *C = cc_roof_at(x, y + sp), *DQ = cc_roof_at(x + sp, y + sp);
            int hA = rh_get(l, x, y), hB = rh_get(l, x + 1, y), hC = rh_get(l, x, y + 1), hD = rh_get(l, x + 1, y + 1);   // (a merged 2x2 has the same slope as its first tile)
            int dhx = ((hB + hD - hA - hC) * pitch) / 2, dhz = ((hA + hB - hC - hD) * pitch) / 2;   // the slope across the tile
            int key = (dhx + 2048) | ((dhz + 2048) << 12);                   // most tiles share a slope: keep the last one's light
            static int last_key = -1, last_L, last_amb = -1; if (gfx_light_amb() != last_amb) { last_key = -1; last_amb = gfx_light_amb(); }
            int L;
            if (key == last_key) L = last_L;
            else { int len = SquareRoot0(dhx * dhx + TILE * TILE + dhz * dhz); if (len < 1) len = 1;
                   L = last_L = gfx_light((-dhx << 12) / len, (-TILE << 12) / len, (-dhz << 12) / len); last_key = key; }
            u32 c = gfx_grade_tex((u32)MIN(255, (rc[0] * L) >> 7) | ((u32)MIN(255, (rc[1] * L) >> 7) << 8) | ((u32)MIN(255, (rc[2] * L) >> 7) << 16));
            int u0 = roof_tex.u, v0 = roof_tex.v, u1 = u0 + 15, v1 = v0 + 15;
            if (world_fp && b && b_see[b]) {   // first person, under your own roof: a plain plaster ceiling, not shingles
                int lc = CLAMP(gfx_light_amb() * 2, 80, 190);
                u32 pc = (u32)lc | ((u32)(lc - 4) << 8) | ((u32)(lc - 12) << 16);
                if (nc_need4(A, B, C, DQ)) {   // (overhead and close: cut at the near plane, so the ceiling never has holes)
                    SVECTOR s4[4] SVEC_ALIGN; roof_corner(l, x, y, &s4[0], base, pitch); roof_corner(l, x + 1, y, &s4[1], base, pitch); roof_corner(l, x, y + 1, &s4[2], base, pitch); roof_corner(l, x + 1, y + 1, &s4[3], base, pitch);
                    const u32 cc[4] = { pc, pc, pc, pc }, uv0[4] = { 0, 0, 0, 0 };
                    nc_quad(&s4[0], &s4[1], &s4[2], &s4[3], cc, 0, 0, 0, uv0, 0);
                } else gfx_emit_f4(A, B, C, DQ, pc, 0);
            } else if (ABS(dhz) >= ABS(dhx)) gfx_emit_gt4(A, B, C, DQ, c, c, c, c, roof_tex.tpage, 0, u0, v0, u1, v1, !world_fp);   // courses run across the slope (first person: seen from under it too)
            else gfx_emit_ft4(A, B, C, DQ, c, roof_tex.tpage, 0, u0, v0, u0, v1, u1, v0, u1, v1, !world_fp, 0);
            if (big) { gables(l, x, y, base, pitch); gables(l, x + 1, y, base, pitch); gables(l, x, y + 1, base, pitch); gables(l, x + 1, y + 1, base, pitch);
                       skip_nx[x >> 5] |= 1u << (x & 31); skip_nx[(x + 1) >> 5] |= 1u << ((x + 1) & 31); x++; }
            else gables(l, x, y, base, pitch);
        } }
    }
}
// ---- the far skyline: buildings past the detail range, as boxes (a wall colour, a band of windows a storey, the roof) ----
static void far_quad(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, int dx, int dy, int dz, u16 col, int L) {
    SVECTOR a, b, c, d; setVector(&a, ax, ay, az); setVector(&b, bx, by, bz); setVector(&c, cx, cy, cz); setVector(&d, dx, dy, dz);
    gfx_quad_f(&a, &b, &c, &d, MIN(255, (R8(col) * L) >> 7), MIN(255, (G8(col) * L) >> 7), MIN(255, (B8(col) * L) >> 7), 1);
}
static void far_box(int b, int windows) {
    int x0 = b_x0[b] * TILE, x1 = (b_x1[b] + 1) * TILE, zN = -b_y0[b] * TILE, zS = -(b_y1[b] + 1) * TILE;
    int st = b_top[b] + 1, HT = st * WALL_FULL, cx = (x0 + x1) / 2, cz = (zN + zS) / 2, rad = MAX(x1 - x0, zN - zS) * 3 / 4 + HT / 2;
    if (!gfx_visible(cx, -HT / 2, cz, rad)) return;
    u16 wc = b_wall[b] ? b_wall[b] : RGB(24, 22, 19), glass = RGB(6, 9, 14);
    int LS = CLAMP(gfx_light(0, 0, -ONE), 40, 220), LN = CLAMP(gfx_light(0, 0, ONE), 40, 220), LW = CLAMP(gfx_light(-ONE, 0, 0), 40, 220), LE = CLAMP(gfx_light(ONE, 0, 0), 40, 220);
    far_quad(x0, -HT, zS, x1, -HT, zS, x0, 0, zS, x1, 0, zS, wc, LS);          // south
    far_quad(x1, -HT, zN, x0, -HT, zN, x1, 0, zN, x0, 0, zN, wc, LN);          // north
    far_quad(x0, -HT, zN, x0, -HT, zS, x0, 0, zN, x0, 0, zS, wc, LW);          // west
    far_quad(x1, -HT, zS, x1, -HT, zN, x1, 0, zS, x1, 0, zN, wc, LE);          // east
    if (windows) for (int s = 0; s < st; s++) {                              // a band of windows round each storey
        int ya = -(s * WALL_FULL + 360), yb = -(s * WALL_FULL + 170), I = 60, O = 6;
        far_quad(x0 + I, ya, zS - O, x1 - I, ya, zS - O, x0 + I, yb, zS - O, x1 - I, yb, zS - O, glass, LS + 40);
        far_quad(x1 - I, ya, zN + O, x0 + I, ya, zN + O, x1 - I, yb, zN + O, x0 + I, yb, zN + O, glass, LN + 40);
        far_quad(x0 - O, ya, zN - I, x0 - O, ya, zS + I, x0 - O, yb, zN - I, x0 - O, yb, zS + I, glass, LW + 40);
        far_quad(x1 + O, ya, zS + I, x1 + O, ya, zN - I, x1 + O, yb, zS + I, x1 + O, yb, zN - I, glass, LE + 40);
    }
    const u8 *rc = roofCol[b_col[b] % NROOF_COL]; u16 rcol = RGB(rc[0] >> 3, rc[1] >> 3, rc[2] >> 3);
    if (b_sty[b] == 2) { u16 roof = RGB(16, 16, 17); far_quad(x0, -HT - 8, zN, x1, -HT - 8, zN, x0, -HT - 8, zS, x1, -HT - 8, zS, roof, CLAMP(gfx_light(0, -ONE, 0), 60, 220)); }
    else {                                                                   // a hip roof: four slopes up to a point (or a ridge)
        int ph = MIN(x1 - x0, zN - zS) / 2 * roofPitch[b_pit[b] % NROOF_PITCH] / TILE;
        SVECTOR a, bb, c; int Lr = CLAMP(gfx_light(0, -ONE * 3 / 4, -ONE / 2), 50, 230);
        int r = MIN(255, (R8(rcol) * Lr) >> 7), g = MIN(255, (G8(rcol) * Lr) >> 7), bl = MIN(255, (B8(rcol) * Lr) >> 7);
        setVector(&c, cx, -HT - ph, cz);
        setVector(&a, x0, -HT, zS); setVector(&bb, x1, -HT, zS); gfx_tri_f(&c, &bb, &a, r, g, bl, 1); gfx_tri_f(&c, &a, &bb, r, g, bl, 1);
        setVector(&a, x1, -HT, zN); setVector(&bb, x0, -HT, zN); gfx_tri_f(&c, &bb, &a, r * 3 / 4, g * 3 / 4, bl * 3 / 4, 1); gfx_tri_f(&c, &a, &bb, r * 3 / 4, g * 3 / 4, bl * 3 / 4, 1);
        setVector(&a, x0, -HT, zN); setVector(&bb, x0, -HT, zS); gfx_tri_f(&c, &bb, &a, r * 7 / 8, g * 7 / 8, bl * 7 / 8, 1); gfx_tri_f(&c, &a, &bb, r * 7 / 8, g * 7 / 8, bl * 7 / 8, 1);
        setVector(&a, x1, -HT, zS); setVector(&bb, x1, -HT, zN); gfx_tri_f(&c, &bb, &a, r * 7 / 8, g * 7 / 8, bl * 7 / 8, 1); gfx_tri_f(&c, &a, &bb, r * 7 / 8, g * 7 / 8, bl * 7 / 8, 1);
    }
}
int dbg_ao_hb, dbg_roof_hb, world_hb_roofs, world_hb_sky1;
int world_hb_l0, world_hb_up, world_up_walls, world_up_wprims, world_up_fl;
// the buildings that stand between the camera and you are seen into, like the one you are in: no roof, no storeys above yours,
// the rooms shown (the walls on the camera's side are cut low as ever)
static int slab1(int o, int d, int lo, int hi, int *t0, int *t1) {   // the ray o + d t (t 0..4096) through lo..hi: narrows t0..t1
    if (d == 0) return o >= lo && o <= hi;
    int ta = ((lo - o) << 12) / d, tb = ((hi - o) << 12) / d;
    if (ta > tb) { int t = ta; ta = tb; tb = t; }
    if (ta > *t0) *t0 = ta; if (tb < *t1) *t1 = tb;
    return *t0 <= *t1;
}
static void see_through(int focus, int fx, int fy) {
    memset(b_see, 0, sizeof b_see);
    if (focus) b_see[focus] = 1;
    if (world_fp) {                                                   // first person: nothing stands between the eye and you; the
        for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {   // buildings round you (in a doorway too) open up inside
            int x = fx + dx, y = fy + dy;
            if (x >= 0 && y >= 0 && x < MW && y < MH && bld[y][x]) b_see[bld[y][x]] = 1;
        }
        return;
    }
    int ex = cam.eye.vx, ey = cam.eye.vy, ez = cam.eye.vz, dx = cam.at.vx - ex, dy = cam.at.vy - ey, dz = cam.at.vz - ez;
    int tx0 = MIN(ex, cam.at.vx) / TILE - 1, tx1 = MAX(ex, cam.at.vx) / TILE + 1;            // the ray's box, in tiles
    int ty0 = -MAX(ez, cam.at.vz) / TILE - 1, ty1 = -MIN(ez, cam.at.vz) / TILE + 1;
    for (int b = 1; b <= nbld; b++) {
        if (b_x0[b] == 255 || b == focus) continue;
        if (b_x1[b] < tx0 || b_x0[b] > tx1 || b_y1[b] < ty0 || b_y0[b] > ty1) continue;
        int t0 = 0, t1 = 4096 - 300;                                  // (short of your own spot)
        if (!slab1(ex, dx, b_x0[b] * TILE - 40, (b_x1[b] + 1) * TILE + 40, &t0, &t1)) continue;
        if (!slab1(ez, dz, -(b_y1[b] + 1) * TILE - 40, -b_y0[b] * TILE + 40, &t0, &t1)) continue;
        if (!slab1(ey, dy, -(b_top[b] + 1) * WALL_FULL - 600, 0, &t0, &t1)) continue;
        b_see[b] = 1;
    }
}
int world_detail_r = 12;                        // the full-detail range round you, tiles (the options menu)
int world_far_range = 34;                       // how far (tiles) the skyline reaches (more with 8 MB: everything in the window)
// ---- the island beyond the range: 4x4-tile cells out to 40 tiles round you (its land, its woods a raised canopy, the sea past its
// shores), going into the haze; the endless sea past them is the sky's (far_sky_sea) ----
#define NCELL (MW / 4)
static u16 fc_col[NCELL][NCELL]; static u8 fc_kind[NCELL][NCELL]; static int fc_stamp = -1; static u32 fc_for;
static void nat_far_cells(void) {
    int isl = world_zone == ZONE_ISLAND;
    for (int cy = 0; cy < NCELL; cy++) for (int cx = 0; cx < NCELL; cx++) {
        int r = 0, g = 0, b = 0, wet = 0, trees = 0;
        if (!isl) {   // the floors' own colours (a building's ground under its box: its wall colour)
            for (int k = 0; k < 16; k++) {
                int x = cx * 4 + (k & 3), y = cy * 4 + (k >> 2), f = floorMap[y][x] % NFLOORS; char c = lifeMap[y][x];
                if (f == FL_WATER) wet++;
                if (c == 'g' || c == '3' || c == '6' || c == 'u') trees++;
                u16 col = (is_wall(c) || c == 'D') ? wp_top[wallMap[y][x] % NWALLS] : fl_avg[f];
                r += C_R(col) << 3; g += C_G(col) << 3; b += C_B(col) << 3;
            }
            r >>= 4; g >>= 4; b >>= 4;
            if (trees) { int t = MIN(trees * 30, 200); r = (r * (256 - t) + 34 * t) >> 8; g = (g * (256 - t) + 70 * t) >> 8; b = (b * (256 - t) + 30 * t) >> 8; }
            fc_col[cy][cx] = RGB(r >> 3, g >> 3, b >> 3);
            fc_kind[cy][cx] = (u8)((wet >= 9) | (MIN(trees, 7) << 1));
            continue;
        }
        int j = CLAMP((nat_noise(ngr_j, cx * 4 + 2, cy * 4 + 2) - 96) * 4, 0, 255);
        for (int k = 0; k < 16; k++) {
            int x = cx * 4 + (k & 3), y = cy * 4 + (k >> 2), f = floorMap[y][x]; char c = lifeMap[y][x];
            if (f == FL_WATER) { wet++; r += 22; g += 70; b += 120; continue; }
            if (c == 'g' || c == '3' || c == '6') trees++;
            if (is_wall(c) || c == 'D' || f < NFL) { r += 150; g += 104; b += 76; continue; }   // (a hut: its roof)
            switch (f) {
                case FL_SAND: r += 150; g += 136; b += 100; break;
                case FL_ROCK: r += 112; g += 104; b += 92; break;
                case FL_SOIL: r += 120; g += 96; b += 70; break;
                default: r += 76 - ((38 * j) >> 8); g += 124 - ((38 * j) >> 8); b += 46 - ((12 * j) >> 8); break;
            }
        }
        r >>= 4; g >>= 4; b >>= 4;
        if (trees) { int t = MIN(trees * 40, 220); r = (r * (256 - t) + 30 * t) >> 8; g = (g * (256 - t) + 74 * t) >> 8; b = (b * (256 - t) + 30 * t) >> 8; }
        fc_col[cy][cx] = RGB(r >> 3, g >> 3, b >> 3);
        fc_kind[cy][cx] = (u8)((wet >= 9) | (MIN(trees, 7) << 1));
    }
}
#define XYW_(v) (*(const u32 *)&(v)->x)
static void nat_far(int px, int py) {
    static int fc_zone = -1;
    if (fc_stamp != world_stamp || fc_for != ngr_for || fc_zone != world_zone) { nat_far_cells(); fc_stamp = world_stamp; fc_for = ngr_for; fc_zone = world_zone; }
    u16 outc = world_zone == ZONE_ISLAND ? RGB(22 >> 3, 70 >> 3, 120 >> 3) : fl_avg[world_zone == ZONE_TOWN ? FL_SAND : FL_GRASS];   // past the map: the sea, or the land going on
    int out_r = C_R(outc) << 3, out_g = C_G(outc) << 3, out_b = C_B(outc) << 3, out_h = world_zone == ZONE_ISLAND ? 24 : 0;
    #define FN 18
    const int S = world_zone == ZONE_ISLAND ? 1 : 2, NN = S == 1 ? FN : 12;   // (off the island the land is plain: cells twice the size, out as far, for a third of the work)
    static SVECTOR fs[2][FN + 1] SVEC_ALIGN; static XV fv[2][FN + 1]; static u32 fcl[2][FN + 1]; static u8 fk[2][FN + 1]; int rlo[2], rhi[2];
    int c0x = ((px >> 2) - NN * S / 2) & ~(S - 1), c0y = ((py >> 2) - NN * S / 2) & ~(S - 1);
    int fwx = cam.at.vx - cam.eye.vx, fwz = cam.at.vz - cam.eye.vz, fl = isqrt(fwx * fwx + fwz * fwz); if (fl < 1) fl = 1;
    fwx = (fwx << 12) / fl; fwz = (fwz << 12) / fl;                       // (the way the camera looks: what lies behind it is not worked out)
    VECTOR org; setVector(&org, c0x * 4 * TILE, 0, -c0y * 4 * TILE); gfx_set_object(&org, 0);
    int Lg = CLAMP(gfx_light(0, -ONE, 0), 40, 200);
    int south = cam.eye.vz < -(c0y * 4 + NN * S * 2) * TILE;                    // (the rows nearest the camera go in first: they are drawn last)
    u32 *ot = gfx_ot() + OT_GROUND_FAR;
    for (int n = 0, cur = 0; n <= NN; n++, cur ^= 1) {
        int j = south ? NN - n : n;
        {   // the corners of this row in front of the camera (a run: the distance ahead changes evenly along it)
            int d = ((c0x * 4 * TILE - cam.eye.vx) * fwx + (-(c0y + j * S) * 4 * TILE - cam.eye.vz) * fwz) >> 12, st = (S * 4 * TILE * fwx) >> 12;
            rlo[cur] = NN + 1; rhi[cur] = -1;
            for (int i = 0; i <= NN; i++, d += st) if (d >= -2 * TILE * S) { if (rlo[cur] > NN) rlo[cur] = i; rhi[cur] = i; }
        }
        for (int i = rlo[cur]; i <= rhi[cur]; i++) {
            int r = 0, g = 0, b = 0, h = 0;
            for (int k = 0; k < 4; k++) {
                int cx = c0x + i * S - 1 + (k & 1), cy = c0y + j * S - 1 + (k >> 1);
                if (cx < 0 || cy < 0 || cx >= NCELL || cy >= NCELL) { r += out_r; g += out_g; b += out_b; h += out_h; continue; }
                u16 c = fc_col[cy][cx]; int kd = fc_kind[cy][cx];
                r += C_R(c) << 3; g += C_G(c) << 3; b += C_B(c) << 3; h += (kd & 1) ? 24 : -(kd >> 1) * 40;
            }
            r = MIN((r * Lg) >> 9, 255); g = MIN((g * Lg) >> 9, 255); b = MIN((b * Lg) >> 9, 255);
            fcl[cur][i] = (u32)r | ((u32)g << 8) | ((u32)b << 16); fk[cur][i] = 0;
            setVector(&fs[cur][i], i * S * 4 * TILE, h / 4, -j * S * 4 * TILE);
        }
        if (rhi[cur] >= rlo[cur]) gfx_xform(fs[cur] + rlo[cur], fv[cur] + rlo[cur], rhi[cur] - rlo[cur] + 1);
        if (!n) continue;
        int jr = south ? j : j - 1;                                         // (the row of cells between the two rows of corners)
        int ilo = MAX(rlo[0], rlo[1]), ihi = MIN(rhi[0], rhi[1]) - 1;
        for (int i = ilo; i <= ihi; i++) {
            int tx = (c0x + i * S) * 4, ty = (c0y + jr * S) * 4;
            if (tx >= ngx0 && tx + 4 * S - 1 <= ngx1 && ty >= ngy0 && ty + 4 * S - 1 <= ngy1) continue;   // (wholly under the full ground)
            const XV *P = fv[cur ^ 1], *Q = fv[cur];
            const XV *A = south ? &Q[i] : &P[i], *B = south ? &Q[i + 1] : &P[i + 1], *C = south ? &P[i] : &Q[i], *Dd = south ? &P[i + 1] : &Q[i + 1];
            if (!xv_vis4(A, B, C, Dd, 0)) continue;
            #define FCOL(row, k) if (!fk[row][k]) { fcl[row][k] = nat_haze(fcl[row][k], fv[row][k].z); fk[row][k] = 1; }
            FCOL(cur, i) FCOL(cur, i + 1) FCOL(cur ^ 1, i) FCOL(cur ^ 1, i + 1)
            #undef FCOL
            u32 ca = south ? fcl[cur][i] : fcl[cur ^ 1][i], cb = south ? fcl[cur][i + 1] : fcl[cur ^ 1][i + 1], cc = south ? fcl[cur ^ 1][i] : fcl[cur][i], cd = south ? fcl[cur ^ 1][i + 1] : fcl[cur][i + 1];
            u32 *q = xv_alloc(9); if (!q) break;
            q[1] = ca | 0x38000000u; q[2] = XYW_(A); q[3] = cb; q[4] = XYW_(B); q[5] = cc; q[6] = XYW_(C); q[7] = cd; q[8] = XYW_(Dd);
            q[0] = (8u << 24) | (*ot & 0xffffff); *ot = (u32)q & 0xffffff; gfx_frame_prims++;
        }
    }
    gfx_set_identity();
    #undef FN
}
#undef XYW_

void world_draw(int fx, int fy, int t, int cutaway) {
    gfx_set_identity(); world_dbg_walls = 0;
    gfx_set_reserve(RESERVE_PEOPLE + RESERVE_UI);   // the world may not use up the room the people need
    frame_t = t;
    if (ao_dirty) { int h = gfx_hb(); ao_update(); h = gfx_hb() - h; if (h > dbg_ao_hb) dbg_ao_hb = h; }
    if (roof_dirty) { int h = gfx_hb(); roof_rebuild(); h = gfx_hb() - h; if (h > dbg_roof_hb) dbg_roof_hb = h; }
    else if (rq_n) {                                 // the blocks that came into the window (far off): in the frames' spare time
        static int wait; extern int gfx_last_cpu;
        if (gfx_last_cpu < 420 || ++wait >= 3) { roof_pending(1); wait = 0; }
    }
    const int R = world_detail_r;
    nat_on = world_zone == ZONE_ISLAND && world_nature;
    { const int G = MIN(R, 8); ngx0 = fx - G; ngx1 = fx + G; ngy0 = fy - G; ngy1 = fy + G; }   // (the island's ground in full this far round you)
    if (!nat_on) { ngx0 = rx0; ngx1 = rx1; ngy0 = ry0; ngy1 = ry1; }   // (elsewhere: the floors the range draws)
    rx0 = MAX(0, fx - R) & ~1; rx1 = MIN(MW - 1, fx + R); ry0 = MAX(0, fy - R) & ~1; ry1 = MIN(MH - 1, fy + R);
    ex0 = rx0; ex1 = rx1; ey0 = ry0; ey1 = ry1;
    int focus = (fx >= 0 && fy >= 0 && fx < MW && fy < MH) ? bld[fy][fx] : 0;
    see_through(focus, fx, fy);
    for (int b = 1; b <= nbld; b++) {               // buildings the range cuts through are drawn whole
        if (b_x0[b] == 255) { b_str[b] = 0; continue; }
        int inside = b_x0[b] >= rx0 && b_x1[b] <= rx1 && b_y0[b] >= ry0 && b_y1[b] <= ry1;
        int touch = b_x1[b] >= rx0 && b_x0[b] <= rx1 && b_y1[b] >= ry0 && b_y0[b] <= ry1;
        b_str[b] = (u8)(touch && !inside);
        if (b_str[b]) { ex0 = MIN(ex0, b_x0[b]); ex1 = MAX(ex1, b_x1[b]); ey0 = MIN(ey0, b_y0[b]); ey1 = MAX(ey1, b_y1[b]); }
    }
    ex0 &= ~1; ey0 &= ~1;
    int keep_lvl = world_lvl; world_lvl = 0;
    cut_all = cutaway;
    { int h0 = gfx_hb(); draw_layer(fx, fy, t, cutaway && world_view_lvl == 0, 0, 0, focus); world_hb_l0 = gfx_hb() - h0; }
    int h1_ = gfx_hb(); int pw0_ = gfx_prof[1], pn0_ = gfx_profn[1], pf0_ = gfx_prof[0];
    {   // the storeys above: the drawing reads each storey's own arrays, with the camera dropped by its height. Up to the storey
        // you are on, everything; above it, other buildings' outside walls (the one you are in stays open to the sky)
        int keep_ao = dbg_noao; dbg_noao = 1;
        for (int l = 1; l <= NUP; l++) {
            Level *Lv = lv_get(l - 1);
            if (!lvl_used[l] || !Lv) continue;
            dm = Lv->map; dfl = Lv->fl; dwp = Lv->wp; world_lvl = l;
            world_cam_push(l);
            draw_layer(fx, fy, t, cutaway && l == world_view_lvl, l, l > world_view_lvl, focus);
            world_cam_pop();
        }
        dm = lifeMap; dfl = floorMap; dwp = wallMap; world_lvl = 0;
        dbg_noao = keep_ao;
    }
    world_hb_up = gfx_hb() - h1_; world_up_walls = gfx_prof[1] - pw0_; world_up_wprims = gfx_profn[1] - pn0_; world_up_fl = gfx_prof[0] - pf0_;
    { PROF_BEGIN(); int h0 = gfx_hb(); roofs_draw(fx, fy); world_hb_roofs = gfx_hb() - h0; PROF_END(3); }
    if (nat_on || world_zone == ZONE_HOME || world_zone == ZONE_TOWN) { PROF_BEGIN(); nat_far(fx, fy); PROF_END(7); }   // (the ground past the range, out to the map's edge and on)
    {   // the skyline: everything past the range, as boxes (windows on the nearer ones); in SAN BOREAS, on out to the horizon
        PROF_BEGIN(); gfx_set_identity();
        int use_far = world_zone == ZONE_CITY && far_begin(world_horizon);
        int far2 = world_far_range * world_far_range;
        int etx = cam.eye.vx / TILE, ety = -cam.eye.vz / TILE;
        for (int b = 1; b <= nbld; b++) {
            if (b_x0[b] == 255) continue;
            if (b_x1[b] >= rx0 && b_x0[b] <= rx1 && b_y1[b] >= ry0 && b_y0[b] <= ry1) continue;   // in range: drawn properly
            int dx = (b_x0[b] + b_x1[b]) / 2 - fx, dy = (b_y0[b] + b_y1[b]) / 2 - fy, d2 = dx * dx + dy * dy;
            if (d2 > far2) continue;
            if (etx >= b_x0[b] - 3 && etx <= b_x1[b] + 3 && ety >= b_y0[b] - 3 && ety <= b_y1[b] + 3) continue;   // (beside the camera: it would fill the screen)
            if (use_far) { const u8 *rc = roofCol[b_col[b] % NROOF_COL];
                far_box_world(b_x0[b] * TILE, -b_y0[b] * TILE, (b_x1[b] + 1) * TILE, -(b_y1[b] + 1) * TILE, b_top[b] + 1,
                              b_wall[b] ? b_wall[b] : RGB(24, 22, 19), b_sty[b] != 2, RGB(rc[0] >> 3, rc[1] >> 3, rc[2] >> 3)); }
            else far_box(b, d2 < 22 * 22);
        }
        world_hb_sky1 = gfx_hb() - prof_t0_;
        if (use_far) far_draw(rx0, ry0, rx1, ry1);   // the land, the towns and the trees, out to the horizon
        gfx_set_identity();
        PROF_END(3);
    }
    world_lvl = keep_lvl;
    gfx_set_reserve(0);
    { extern int gfx_world_peak, gfx_bytes_used(void); int u = gfx_bytes_used(); if (u > gfx_world_peak) gfx_world_peak = u; }
}

// ---- the memory card (save.c): the home lot's tiles, the family's own lot, the blocks built on, and where the window is ----
static void sv_level(const char (*map)[MW + 1], const u8 (*fl)[MW], const u8 (*wp)[MW]) { sv_rle(map, MH * (MW + 1)); sv_rle(fl, MH * MW); sv_rle(wp, MH * MW); }
static void ld_level(char (*map)[MW + 1], u8 (*fl)[MW], u8 (*wp)[MW]) { ld_rle(map, MH * (MW + 1)); ld_rle(fl, MH * MW); ld_rle(wp, MH * MW); }
COLD static void sv_zone(int z) {   // a zone's tiles and storeys, live or kept aside
    if (world_zone == z) sv_level(lifeMap, floorMap, wallMap);
    else if (zp_slot[z] >= 0) { const Level *L = &lvpool[zp_slot[z]]; sv_level(L->map, L->fl, L->wp); }
    else sv_n(zpack + zp_off[z], zp_len[z]);   // (kept packed the way the save packs it)
    int mask = 0; for (int k = 0; k < NUP; k++) if (lvown[z][k]) mask |= 1 << k;
    sv_u8(mask);
    for (int k = 0; k < NUP; k++) if (mask & (1 << k)) { const Level *L = &lvpool[lvown[z][k] - 1]; sv_level(L->map, L->fl, L->wp); }
}
static void ld_zone_live(void) {   // ... into the live zone
    ld_level(lifeMap, floorMap, wallMap);
    levels_clear(UP);
    int mask = ld_u8();
    for (int k = 0; k < NUP; k++) if (mask & (1 << k)) { Level *L = lv_need(k); if (!L) { ld_fail(); break; } ld_level(L->map, L->fl, L->wp); }
    ao_dirty = 1; roof_dirty = 1; world_stamp++;
}
__attribute__((optimize("Os"))) void world_save(int with_home) {   // with_home: THE MAN BASE is the family's home (its tiles are kept)
    window_keep();
    sv_u8(world_zone); sv_u16(world_wbx); sv_u16(world_wby);
    sv_u8(with_home);
    if (with_home) sv_zone(ZONE_HOME);
    for (int z = ZONE_ISLAND; z <= ZONE_TOWN; z++) { int made = world_zone == z || zp_made[z]; sv_u8(made); if (made) sv_zone(z); }   // (the island's camp, the town: as you left them)
    sv_u8(own.used);
    if (own.used) {
        sv_u16(own.bx); sv_u16(own.by); sv_u8(own.fresh); sv_u8(own.lv); sv_u8(own.home);
        sv_u8(own.hx0); sv_u8(own.hy0); sv_u8(own.hx1); sv_u8(own.hy1); sv_u8(own.hdx); sv_u8(own.hdy);
        sv_n(&own_occ, sizeof own_occ);
        if (!own.fresh) { sv_rle(own.map, sizeof own.map); sv_rle(own.fl, sizeof own.fl); sv_rle(own.wp, sizeof own.wp); }
    }
    int e0 = MAX(0, nedit - 48);   // (the 48 blocks built on last, at most: an 8 MB machine keeps more in play than a card should hold)
    sv_u16(nedit - e0); sv_u16(e0 ? 0 : edit_next);
    for (int i = e0; i < nedit; i++) { sv_u16(edits[i].bx); sv_u16(edits[i].by); sv_rle(edits[i].map, sizeof edits[i].map); sv_rle(edits[i].fl, sizeof edits[i].fl); sv_rle(edits[i].wp, sizeof edits[i].wp); }
}
__attribute__((optimize("Os"))) void world_load(void) {   // (after world_new_seed and world_reset_all: THE MAN BASE is the live zone)
    world_zone_saved = ld_u8(); int wbx = (s16)ld_u16(), wby = (s16)ld_u16();
    world_wbx_saved = wbx; world_wby_saved = wby;
    if (ld_u8()) ld_zone_live();
    for (int z = ZONE_ISLAND; z <= ZONE_TOWN; z++) if (ld_u8()) { world_switch(z); ld_zone_live(); world_switch(ZONE_HOME); }   // (made by its generator first: its homes, the dock, the fire; then the tiles as they were)
    own.used = ld_u8();
    if (own.used) {
        own.bx = (u16)ld_u16(); own.by = (u16)ld_u16(); own.fresh = (u8)ld_u8(); own.lv = (u8)ld_u8(); own.home = (u8)ld_u8();
        own.hx0 = (u8)ld_u8(); own.hy0 = (u8)ld_u8(); own.hx1 = (u8)ld_u8(); own.hy1 = (u8)ld_u8(); own.hdx = (u8)ld_u8(); own.hdy = (u8)ld_u8();
        ld_n(&own_occ, sizeof own_occ);
        if (save_ver < 20) {   // (before the wide streets a block was 16 tiles: its kept tiles cannot be laid over the new map, so they are read and let go)
            static u8 junk[NLEVELS * 256];
            if (!own.fresh) { ld_rle(junk, NLEVELS * 256); ld_rle(junk, NLEVELS * 256); ld_rle(junk, NLEVELS * 256); }
            own.used = 0;
        } else
        if (!own.fresh) { ld_rle(own.map, sizeof own.map); ld_rle(own.fl, sizeof own.fl); ld_rle(own.wp, sizeof own.wp); }
    }
    int n = ld_u16(); edit_next = ld_u16(); nedit = 0;
    for (int i = 0; i < n; i++) {
        EditBlk t; int keep = i < nedit_max; EditBlk *e = keep ? &edits[i] : &t;   // (t: a block too many for this machine's list is read and dropped)
        if (save_ver < 20) { static u8 junk[256]; ld_u16(); ld_u16(); ld_rle(junk, 256); ld_rle(junk, 256); ld_rle(junk, 256); continue; }
        e->bx = (u16)ld_u16(); e->by = (u16)ld_u16(); ld_rle(e->map, sizeof e->map); ld_rle(e->fl, sizeof e->fl); ld_rle(e->wp, sizeof e->wp);
        if (keep) nedit++;
    }
    if (edit_next >= nedit_max) edit_next = 0;
}
void world_place(void) {   // where the save was: the zone, and the window of SAN BOREAS round the family
    int z = world_zone_saved;
    if (z == ZONE_HOME) return;
    world_switch(z);
    if (z == ZONE_CITY) world_open_at(world_wbx_saved + WB / 2, world_wby_saved + WB / 2);
}
u32 world_seed(void) { return wsalt; }
