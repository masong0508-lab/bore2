// views_plan.h: the 3D audit shots (build with -DAUTO_PHONE=1 -DAUTO_VIEWS=1). One entry per shot, 90 frames apart: { frame, op, a, b, pitch, dist, yaw }
// ops: t travel to zone a; g stand in front of item letter a; p stand on tile (a, b); R a showroom of views_room[a] (6 to a row, 3 tiles apart, from tile 4,4; b = their facing);
// then the camera is set. test/views.txt takes a shot 60 frames after each entry.
#define VIEW_OX 30   // the showroom's first item: tile (30, 34) of the home zone (the empty ground south of the lot)
#define VIEW_OY 34
typedef struct { s16 at; char op; s16 a, b, pitch, dist, yaw; } VPlan;
static const char views_r0[] = { 'S', '<', (char)0x93, (char)0xEB, (char)0xEC, (char)0xED,   (char)0xF6, (char)0x81, (char)0xF0, (char)0x96, (char)0xF3, (char)0x97,   'i', (char)0xEE, (char)0xEF, (char)0xF5, (char)0x3C, (char)0xC3, 0 };
static const char views_r1[] = { 'C', ';', (char)0x90, (char)0x91, 'v', (char)0x95,   'b', 'A', 't', 'h', 'd', '[',   'r', 'p', 'l', 'o', (char)0x9B, (char)0x98, 0 };
static const char views_r2[] = { 'F', '/', '-', 'y', ':', (char)0x9C,   (char)0x9D, 'T', 'H', ',', '`', (char)0x9A,   'q', 'm', (char)0x24, (char)0x5D, 'U', 'R', 0 };
static const char *const views_room[] = { views_r0, views_r1, views_r2 };
#define VR(row, col) (4 + 3 * (col)), (4 + 3 * (row))
static const VPlan views_plan[] = {
    { 60, 't', 0, 0, 600, 1700, 0 },
    { 200, 'R', 0, 0, 600, 1700, 0 },
    { 260, 'p', 33, 35, 600, 1700, 0 },
    { 350, 'p', 42, 35, 600, 1700, 0 },
    { 440, 'p', 33, 38, 600, 1700, 0 },
    { 530, 'p', 42, 38, 600, 1700, 0 },
    { 620, 'p', 33, 41, 600, 1700, 0 },
    { 710, 'p', 42, 41, 600, 1700, 0 },
    { 800, 'R', 1, 0, 600, 1700, 0 },
    { 860, 'p', 33, 35, 600, 1700, 0 },
    { 950, 'p', 42, 35, 600, 1700, 0 },
    { 1040, 'p', 33, 38, 600, 1700, 0 },
    { 1130, 'p', 42, 38, 600, 1700, 0 },
    { 1220, 'p', 33, 41, 600, 1700, 0 },
    { 1310, 'p', 42, 41, 600, 1700, 0 },
    { 1400, 'R', 2, 0, 600, 1700, 0 },
    { 1460, 'p', 33, 35, 600, 1700, 0 },
    { 1550, 'p', 42, 35, 600, 1700, 0 },
    { 1640, 'p', 33, 38, 600, 1700, 0 },
    { 1730, 'p', 42, 38, 600, 1700, 0 },
    { 1820, 'p', 33, 41, 600, 1700, 0 },
    { 1910, 'p', 42, 41, 600, 1700, 0 },
    { 0, 0, 0, 0, 0, 0, 0 } };
