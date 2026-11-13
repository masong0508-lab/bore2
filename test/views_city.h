// views_city.h: the 3D audit shots in the city (-DAUTO_PHONE=1 -DAUTO_VIEWS=2): the street at the play camera, low, from above, turned; then in a home. See views_plan.h for the ops.
#define VIEW_OX 30
#define VIEW_OY 34
typedef struct { s16 at; char op; s16 a, b, pitch, dist, yaw; } VPlan;
static const char *const views_room[] = { "" };
static const VPlan views_plan[] = {
    { 60, 't', 3, 0, 640, 1500, 2600 },
    { 200, 'v', 0, 0, 640, 1500, 2600 },
    { 260, 'v', 0, 0, 640, 1500, 2600 },
    { 330, 'H', 4, 0, 640, 1500, 2600 },
    { 400, 'v', 0, 0, 640, 1500, 2600 },
    { 460, 'H', 0, 0, 300, 900, 2600 },
    { 540, 'v', 0, 0, 300, 900, 2600 },
    { 620, 'v', 0, 0, 900, 1500, 2600 },
    { 700, 'v', 0, 0, 640, 1500, 0 },
    { 780, 'v', 0, 0, 640, 1500, 1300 },
    { 860, 'v', 0, 0, 450, 700, 2000 },
    { 940, 'g', 'F', 0, 640, 1500, 2600 },
    { 1020, 'v', 0, 0, 640, 1500, 2600 },
    { 1100, 'v', 0, 0, 520, 900, 1000 },
    { 1180, 'v', 0, 0, 800, 1500, 3500 },
    { 0, 0, 0, 0, 0, 0, 0 } };
