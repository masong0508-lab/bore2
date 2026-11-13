// views_home.h: the 3D audit shots inside the starting home (-DAUTO_PHONE=1 -DAUTO_VIEWS=3): low, from above and turned, for the textures' perspective (docs/texture_home_before_after.png). See views_plan.h for the ops.
#define VIEW_OX 30
#define VIEW_OY 34
typedef struct { s16 at; char op; s16 a, b, pitch, dist, yaw; } VPlan;
static const char *const views_room[] = { "" };
static const VPlan views_plan[] = {
    { 60, 't', 0, 0, 640, 1500, 0 },
    { 200, 'g', 'S', 0, 200, 800, 0 },
    { 290, 'g', 'S', 0, 200, 800, 1024 },
    { 380, 'g', 'C', 0, 200, 800, 2048 },
    { 470, 'g', 'C', 0, 300, 1100, 3072 },
    { 560, 'g', 'T', 0, 450, 900, 500 },
    { 650, 'g', 'S', 0, 640, 1500, 0 },
    { 740, 'g', 'S', 0, 900, 1500, 0 },
    { 830, 'g', 'C', 0, 1000, 1100, 1024 },
    { 920, 'g', 'T', 0, 1000, 2000, 300 },
    { 0, 0, 0, 0, 0, 0, 0 } };
