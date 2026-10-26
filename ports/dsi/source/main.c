// BORE 2 for the DSi, step D0: one lot on the 3D screen, a few people as billboards, the budget meter on the bottom screen.
// This is the measuring spike from docs/PORTING_DS_3DS.md (section 8.7): it answers "how many polygons and how much time on a real DSi".
// Controls: D-pad turns and zooms the camera, L/R spin faster, START exits.
#include <nds.h>
#include <stdio.h>

#define TILE 16                // one tile, in 1/16 units (the game's tile is 256 world units; the DSi build works at 1/16)
#define LOT  13                // a lot is 13 x 13 tiles
static inline int v16(int x) { return floattov16(x / 16.0f); }   // (the build uses fixed point only in the game proper; floats here are for the spike's setup)

static int polys;              // triangles/quads sent this frame (the budget meter)
static void quad(int x0, int y0, int z0, int x1, int y1, int z1, int x2, int y2, int z2, int x3, int y3, int z3, int rgb) {
    glColor(rgb);
    glVertex3v16(v16(x0), v16(y0), v16(z0)); glVertex3v16(v16(x1), v16(y1), v16(z1)); glVertex3v16(v16(x2), v16(y2), v16(z2)); glVertex3v16(v16(x3), v16(y3), v16(z3));
    polys += 2;
}
static void floor_tile(int x, int z, int rgb) { quad(x, 0, z, x + TILE, 0, z, x + TILE, 0, z + TILE, x, 0, z + TILE, rgb); }
static void box(int x0, int z0, int x1, int z1, int h, int wall, int roof) {   // a house: four walls and a flat roof (the budget: 10 triangles)
    quad(x0, 0, z0, x1, 0, z0, x1, -h, z0, x0, -h, z0, wall);
    quad(x1, 0, z0, x1, 0, z1, x1, -h, z1, x1, -h, z0, wall - 0x0842);
    quad(x1, 0, z1, x0, 0, z1, x0, -h, z1, x1, -h, z1, wall);
    quad(x0, 0, z1, x0, 0, z0, x0, -h, z0, x0, -h, z1, wall - 0x0842);
    quad(x0, -h, z0, x1, -h, z0, x1, -h, z1, x0, -h, z1, roof);
}
static void person(int x, int z, int rgb, int yaw) {   // a person as a billboard: one quad (a baked sprite goes on it later), turned to face the camera
    int s = TILE * 2, hx = (s * (sinLerp(yaw) >> 4)) >> 8, hz = (s * (cosLerp(yaw) >> 4)) >> 8;
    quad(x - hx / 2, 0, z - hz / 2, x + hx / 2, 0, z + hz / 2, x + hx / 2, -s, z + hz / 2, x - hx / 2, -s, z - hz / 2, rgb);
}

int main(void) {
    if (!isDSiMode()) {   // (BORE 2 for the DSi needs a DSi: 16 MB of RAM)
        consoleDemoInit(); iprintf("BORE 2 NEEDS A NINTENDO DSI.\n"); while (1) swiWaitForVBlank();
    }
    videoSetMode(MODE_0_3D); videoSetModeSub(MODE_0_2D); consoleDemoInit();
    glInit(); glEnable(GL_ANTIALIAS); glClearColor(14, 20, 31, 31); glClearPolyID(63); glClearDepth(0x7FFF); glViewport(0, 0, 255, 191);
    int yaw = 0, dist = 24, frames = 0;
    while (1) {
        scanKeys(); int held = keysHeld(); if (keysDown() & KEY_START) break;
        if (held & KEY_LEFT) yaw -= (held & KEY_L) ? 90 : 30; if (held & KEY_RIGHT) yaw += (held & KEY_R) ? 90 : 30;
        if ((held & KEY_UP) && dist > 8) dist--; if ((held & KEY_DOWN) && dist < 60) dist++;
        polys = 0;
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(60, 256.0f / 192.0f, 0.2f, 80.0f);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        float a = yaw * 6.2831853f / 32768.0f; gluLookAt(6.5f + dist * 0.5f * __builtin_sinf(a), dist * 0.35f, 6.5f + dist * 0.5f * __builtin_cosf(a), 6.5f, 0, 6.5f, 0, 1, 0);
        glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(1));
        glBegin(GL_QUADS);
        for (int z = 0; z < LOT; z++) for (int x = 0; x < LOT; x++) floor_tile(x * TILE, z * TILE, ((x + z) & 1) ? RGB15(14, 22, 8) : RGB15(12, 19, 7));   // the lawn
        box(2 * TILE, 1 * TILE, 10 * TILE, 8 * TILE, 3 * TILE, RGB15(26, 22, 16), RGB15(18, 8, 6));                                                       // the house
        for (int i = 0; i < 6; i++) person((3 + i) * TILE, (10 + (i & 1)) * TILE, RGB15(6 + i * 4, 10, 25 - i * 3), yaw);                                  // people
        glEnd(0);
        glFlush(0); swiWaitForVBlank(); frames++;
        consoleClear();
        iprintf("BORE 2 DSI  D0 SPIKE\nframe %d\npolygons sent: %d (budget 2000)\n", frames, polys);
        iprintf("GFX: %d polys, %d verts in last frame\n", (int)GFX_POLYGON_RAM_USAGE, (int)GFX_VERTEX_RAM_USAGE);
    }
    return 0;
}
