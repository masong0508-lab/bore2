/* Exercise the production skateboard state machine with a small, unobstructed
 * floor/rail fixture. The renderer, audio and surrounding life sim are outside
 * this unit test; movement, steering, braking and grinds are real skate.c. */
#include <assert.h>
#include <stdarg.h>
#include "../src/skate.h"
#include "../src/world.h"
#include "../src/items.h"

#undef sprintf
#undef snprintf

int on_board, board_lift;
static int rail_axis; /* 0: floor; 1: east/west rail; 2: north/south rail */

void skate_dbg_speed(int v);

char world_at(int x, int y) {
    if (rail_axis == 1 && y == 20 && x >= 18 && x <= 22) return '=';
    if (rail_axis == 2 && x == 20 && y >= 18 && y <= 22) return '=';
    return '.';
}
int items_height(char c) { return c == '=' ? 100 : 0; }
int items_is_item(char c) { return c == '='; }
int items_ramp_at(int x, int y, int u, int v) {
    (void)x; (void)y; (void)u; (void)v; return 0;
}
int items_ramp_lip(int x, int y, int *dir) {
    (void)x; (void)y; *dir = 0; return 0;
}
int sim_walk(Sim *s, int dx, int dz) { s->x += dx; s->z += dz; return 1; }
int sim_ride_after_board(void) { return 0; }
void sim_note(const char *s) { (void)s; }
void audio_voice(int v) { (void)v; }
void story_event(int ev, int arg) { (void)ev; (void)arg; }
void wants_event(Sim *s, int kind, int arg) { (void)s; (void)kind; (void)arg; }
u32 look_rand(void) { return 3300; } /* balance fixture: neutral, deterministic */
#ifdef BORE_DC
void dc_rumble(int strength, int frames) { (void)strength; (void)frames; }
#endif
int bsprintf(char *out, const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    int n = vsprintf(out, fmt, args); va_end(args); return n;
}

static Sim rider(int yaw, int axis) {
    Sim s; memset(&s, 0, sizeof s);
    s.x = 20 * TILE + TILE / 2; s.z = -(20 * TILE + TILE / 2);
    s.yaw = yaw; rail_axis = axis; on_board = 1;
    skate_score = skate_best = 0;
    skate_on(&s); skate_dbg_speed(24);
    return s;
}

static int roll(int yaw, int input, int steps) {
    Sim s = rider(yaw, 0);
    int wx = input < 0 ? 0 : isin(input);
    int wz = input < 0 ? 0 : -icos(input);
    for (int i = 0; i < steps; ++i) {
        skate_input(0, 0); skate_step(&s, wx, wz);
        assert(on_board && !skate_air() && !skate_bailing());
    }
    return skate_speed();
}

static void test_braking(void) {
    for (int yaw = 0; yaw < 4096; yaw += 1024) {
        int coast = roll(yaw, -1, 32);
        int stopped = roll(yaw, (yaw + 2048) & 4095, 32);
        assert(coast >= 20); /* releasing the stick remains a gentle coast */
        assert(stopped == 0); /* holding opposite travel really stops */
        assert(roll(yaw, yaw, 32) >= 22); /* pushing still works */
        assert(roll(yaw, (yaw + 1024) & 4095, 12) > 16); /* steering is not a brake */
    }
    puts("skate controls: reverse-stick braking in all four directions PASS");
}

static void test_rail_direction(int yaw) {
    int axis = (yaw == 1024 || yaw == 3072) ? 1 : 2;
    Sim s = rider(yaw, axis);
    skate_input(SKB_TRI, SKB_TRI); skate_step(&s, 0, 0);
    assert(!skate_air());
    assert(skate_dir() == yaw); /* attaching must not reverse the board */
    int x = s.x, z = s.z;
    for (int i = 0; i < 6; ++i) {
        skate_input(SKB_TRI, 0); skate_step(&s, 0, 0);
    }
    assert(board_lift == 100);
    assert((long)(s.x - x) * isin(yaw) - (long)(s.z - z) * icos(yaw) > 0);
    assert(skate_dir() == yaw && !skate_bailing());
    if (axis == 1) assert(s.z == z); else assert(s.x == x);

    /* Ollie out, keeping momentum in the same direction as the grind. */
    skate_input(SKB_X, SKB_X); skate_step(&s, 0, 0);
    skate_input(0, 0); skate_step(&s, 0, 0);
    assert(skate_air());
    x = s.x; z = s.z;
    skate_step(&s, 0, 0);
    assert((long)(s.x - x) * isin(yaw) - (long)(s.z - z) * icos(yaw) > 0);
    assert(skate_dir() == yaw);
}

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "rails")) test_braking();
    if (argc < 2 || strcmp(argv[1], "brakes")) {
        for (int yaw = 0; yaw < 4096; yaw += 1024) test_rail_direction(yaw);
        puts("skate controls: rail attachment, travel and ollie exit in all four directions PASS");
    }
    return 0;
}
