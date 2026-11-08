__attribute__((noinline)) static void gBox(int x0, int y0, int x1, int y1, int fl) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) floorMap[y][x] = (u8)fl; }
__attribute__((noinline)) static void gRoom(int x0, int y0, int x1, int y1, int fl, int wp) {
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { floorMap[y][x] = (u8)fl;
        if (x == x0 || x == x1 || y == y0 || y == y1) { lifeMap[y][x] = 'W'; wallMap[y][x] = (u8)wp; } }
}
__attribute__((noinline)) static void gLine(int x0, int y0, int x1, int y1, char c, int wp) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { lifeMap[y][x] = c; wallMap[y][x] = (u8)wp; } }
static void gPut(int x, int y, char c) { lifeMap[y][x] = c; }
__attribute__((noinline)) static void gFree(int x, int y, char c) { if (x >= 0 && y >= 0 && x < MW && y < MH && lifeMap[y][x] == '.') lifeMap[y][x] = c; }
static char TREE = '3';                               // what grows in the yards here: oaks, pines, palms, cacti
static unsigned city_h(int a, int b) { unsigned h = (unsigned)a * 374761393u + (unsigned)b * 668265263u + 12345u + wsalt * 2246822519u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }
static void city_path(int dx, int dy, int ly) { gBox(dx, dy + 1, dx, MIN(ly + 12, MH - 2), FL_PAVING); }   // the path from a door to the street

// ---- who lives in the home being built (from the people of SAN BOREAS; elsewhere, somebody) ----
typedef struct { u8 adults, kids, teens, elders, couple, vacant, babies; } Occ;
static Occ occ_blk[12]; static int occ_on;            // the households of the block being made
static const Occ occ_any = { 2, 0, 0, 0, 1, 0, 0 };
static const Occ *occ_of(int unit) { return occ_on ? &occ_blk[unit % 12] : &occ_any; }
