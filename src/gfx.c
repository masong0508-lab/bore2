// gfx.c - BORE 2 rendering core: double buffer, ordering table, camera, GTE quads, 2D overlay.
#include "gfx.h"
#include "font.h"
#undef TILE   // the GPU primitive, not the world tile size, in this file

typedef struct {
    DISPENV disp;
    DRAWENV draw;
    u32     ot[OT_LEN];
    char    p[PACKET_LEN];
} DB;

static DB   db[2];
static int  db_active = 0;
#ifdef BORE_DC
#define KEEP_BYTES 0               // (the Dreamcast port draws the 2D overlay afresh every frame)
#else
#define KEEP_BYTES 5120            // the end of each packet buffer holds pieces of the 2D overlay kept from frame to frame (gfx_keep_begin)
#endif
static int  alloc_reserve;        // bytes of the packet buffer gfx_alloc keeps back (gfx_set_reserve)
char *gfx_pk_next, *gfx_pk_end;   // the packet cursor and its limit (the end, less the reserve): gfx_fast.h allocates inline
u32 *gfx_ot_cur;                   // this frame's ordering table
int gfx_zbias, gfx_fog_on, gfx_zbase, gfx_ot_max = OT_NEAR - 1;
int gfx_wide;
int gfx_min_vsyncs = 1;   // (the options menu: 2 a steady 30 FPS, 3 a steady 20)
void gfx_set_dither(int on) { db[0].draw.dtd = db[1].draw.dtd = (u8)(on ? 1 : 0); }
#define db_next gfx_pk_next
static RECT screen_clip;
static u32 *ui_head, *ui_tail;    // the 2D overlay chain, drawn after everything in call order
static int  clear_r = 20<<3, clear_g = 26<<3, clear_b = 31<<3;   // BORE's SKY colour
int gfx_prof[8], gfx_prof_last[8], gfx_profn[8], gfx_profn_last[8];   // profiler buckets (hblanks), filled by GFX_PROF_BEGIN/END, shown by L3
// a fast integer square root (digit by digit, 16 steps of shifts and compares): the SDK's SquareRoot0 cost ~8000 cycles a call
int isqrt(int v) {
    if (v <= 0) return 0;
    unsigned x = (unsigned)v, r = 0, bit = 1u << 30;
    while (bit > x) bit >>= 2;
    while (bit) { if (x >= r + bit) { x -= r + bit; r = (r >> 1) + bit; } else r >>= 1; bit >>= 2; }
    return (int)r;
}
// a sampling profiler: the music timer (480 Hz) notes where the CPU was; the L3 readout lists the busiest 256-byte stretches of code
#if defined(AUTO_CITY) || defined(PROF_FULL)
#define PH_N 1800   // (test builds: the histogram over the first 460 KB of code)
#else
#define PH_N 16     // (plain Release: 3.5 KB given back to the game; the L3 list then covers the first 4 KB only. Build with -DPROF_FULL=1 for the real thing)
#endif
u16 prof_hist[PH_N]; int prof_on, prof_total;
void prof_sample(void) {
    if (!prof_on) return;
#ifdef BORE_DC
    u32 epc = 0; return;   // (no sampling profiler on the Dreamcast)
#else
    u32 epc; __asm__ volatile("mfc0 %0, $14" : "=r"(epc));
#endif
    u32 off = epc - 0x80010000u;
    if (epc >= (u32)&prof_sample && epc < (u32)&prof_sample + 0x120) return;   // (the profiler itself)
    if (off < PH_N * 256u) { prof_hist[off >> 8]++; prof_total++; }
}
int prof_top(int k, u32 *addr) {   // the k-th busiest stretch: its count (and address). k = 0 works the list out (one pass)
    static u16 top[9], topn[9];
    if (k == 0) {
        for (int j = 0; j < 9; j++) topn[j] = 0;
        for (int i = 0; i < PH_N; i++) {
            int n = prof_hist[i]; if (n <= topn[8]) continue;
            int j = 8; while (j > 0 && topn[j - 1] < n) { topn[j] = topn[j - 1]; top[j] = top[j - 1]; j--; }
            topn[j] = (u16)n; top[j] = (u16)i;
        }
    }
    if (k > 8 || !topn[k]) return 0;
    *addr = 0x80010000u + top[k] * 256u; return topn[k];
}
int gfx_hb(void) { return VSync(1); }   // hblanks since the frame's VSync(0) (63.5 us each; 525 = a 30 FPS frame)
int gfx_peak_bytes, gfx_world_peak; int gfx_last_cpu, gfx_last_wait, gfx_prim_count, gfx_frame_prims, gfx_last_prims, gfx_last_bytes, gfx_overflow, gfx_last_overflow;   // profiler: prims and packet bytes of the last frame, prims that did not fit

Camera cam;
static int obj_yaw;               // yaw of the current object (for lighting)
static int lx_obj, ly_obj, lz_obj; // the sun, turned into the current object's space (set with the yaw)
static void light_to_object(void);
static int light_amb = 100, light_dif = 120;
static SVECTOR light_dir = { -1500, -3200, 2100, 0 };   // towards the sun (y is down, so negative = up), ~unit length (4096)


// vertex loads that never fault: the GTE reads words, an SVECTOR (a struct of shorts) may sit on a 2-byte boundary - a real
// PlayStation (and DuckStation) stops on such a load with an address error. The vertices are copied to aligned ones first.
static inline void gte_ld3_safe(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c) {
    if ((((u32)a | (u32)b | (u32)c) & 3) == 0) {   // all word-aligned (nearly always): load straight from the caller's vertices, no copy
        GTE_FENCE(); gte_ldv3((SVECTOR *)a, (SVECTOR *)b, (SVECTOR *)c); return;
    }
    SVECTOR t[3] SVEC_ALIGN; t[0] = *a; t[1] = *b; t[2] = *c;
    GTE_FENCE(); gte_ldv3(&t[0], &t[1], &t[2]);
}
static inline void gte_ld0_safe(const SVECTOR *a) {
    if (((u32)a & 3) == 0) { GTE_FENCE(); gte_ldv0((SVECTOR *)a); return; }
    SVECTOR t SVEC_ALIGN = *a; GTE_FENCE(); gte_ldv0(&t);
}
// ------------------------------------------------------------------ setup
void gfx_set_clear(int r, int g, int b) { clear_r = r; clear_g = g; clear_b = b; }

void gfx_init(void) {
#ifdef PROF_ON
    prof_on = 1;   // (test builds: the sampling profiler from the start)
#endif
    ResetGraph(0);
    SetDefDispEnv(&db[0].disp, 0, 0, SCREEN_W, SCREEN_H);
    SetDefDrawEnv(&db[0].draw, SCREEN_W, 0, SCREEN_W, SCREEN_H);
    SetDefDispEnv(&db[1].disp, SCREEN_W, 0, SCREEN_W, SCREEN_H);
    SetDefDrawEnv(&db[1].draw, 0, 0, SCREEN_W, SCREEN_H);
    for (int i = 0; i < 2; i++) {
        setRGB0(&db[i].draw, clear_r, clear_g, clear_b);
        db[i].draw.isbg = 1;
        db[i].draw.dtd = 1;
    }
    PutDrawEnv(&db[0].draw);
    ClearOTagR(db[0].ot, OT_LEN);
    ClearOTagR(db[1].ot, OT_LEN);
    db_next = db[0].p; gfx_pk_end = db[0].p + PACKET_LEN - 64 - KEEP_BYTES; gfx_ot_cur = db[0].ot;
    ui_head = ui_tail = 0;
    setRECT(&screen_clip, 0, 0, SCREEN_W, SCREEN_H);

    InitGeom();
    gte_SetGeomOffset(CENTER_X, CENTER_Y);
    gte_SetGeomScreen(230);          // a 70 degree field of view
    gte_SetBackColor(63, 63, 63);

    setVector(&cam.eye, 0, -1200, -1800);
    setVector(&cam.at, 0, -150, 0);
    gfx_camera_update();
    light_to_object();
}

int gfx_frame_no;   // (frames flipped: the interface's panels are counted afresh each frame)
void gfx_flip(void) {
    gfx_frame_no++;
    // hook the 2D chain into OT index 0 (drawn last)
    if (ui_head) {
        u32 nxt = db[db_active].ot[0] & 0xffffff;
        setaddr(ui_tail, nxt);
        setaddr(&db[db_active].ot[0], ui_head);
    }
    gfx_last_bytes = (int)(db_next - db[db_active].p); gfx_last_prims = gfx_frame_prims; gfx_frame_prims = 0;
    if (gfx_last_bytes > gfx_peak_bytes) gfx_peak_bytes = gfx_last_bytes;
    gfx_last_overflow = gfx_overflow; gfx_overflow = 0;
    for (int i = 0; i < 8; i++) { gfx_prof_last[i] = gfx_prof[i]; gfx_prof[i] = 0; gfx_profn_last[i] = gfx_profn[i]; gfx_profn[i] = 0; }
    { int h0 = VSync(1); gfx_last_cpu = h0; DrawSync(0); gfx_last_wait = VSync(1) - h0; }   // how long the GPU was still busy with the last frame
    { extern void atlas_flush_deferred(void); atlas_flush_deferred(); }   // (the textures that waited for this moment)
#ifdef AB_LOCK4
    const int gfx_min_vsyncs = 4;   // (test builds: every frame takes 4 vblanks, so every frame runs 3 game steps whatever it costs: two builds see the very same
#endif                              //  city frame by frame, and what a change costs or saves shows in the frames' own times without the route drifting)
    { static int flip_vb; while (gfx_min_vsyncs > 1 && VSync(-1) - flip_vb < gfx_min_vsyncs - 1) VSync(0);   // (a steady 30 or 20: never sooner)
      VSync(0); flip_vb = VSync(-1); }
    db_active ^= 1;
    db_next = db[db_active].p;
    alloc_reserve = 0; gfx_pk_end = db[db_active].p + PACKET_LEN - 64 - KEEP_BYTES; gfx_ot_cur = db[db_active].ot;
    ui_head = ui_tail = 0;
    ClearOTagR(db[db_active].ot, OT_LEN);
    {   // the frame starts by flushing the GPU's texture cache: the sprite cells were drawn into since it last saw them
        u32 *f = (u32 *)gfx_alloc(8);
        if (f) { f[1] = 0x01000000u; f[0] = (1u << 24) | (db[db_active].ot[OT_LEN - 1] & 0xffffff); db[db_active].ot[OT_LEN - 1] = (u32)f & 0xffffff; }
    }
    setRGB0(&db[db_active].draw, clear_r, clear_g, clear_b);
    PutDrawEnv(&db[db_active].draw);
    PutDispEnv(&db[db_active].disp);
    SetDispMask(1);
    DrawOTag(db[1 - db_active].ot + (OT_LEN - 1));
}

void gfx_set_reserve(int bytes) { alloc_reserve = bytes; gfx_pk_end = db[db_active].p + PACKET_LEN - 64 - bytes - (bytes < RESERVE_PEOPLE ? KEEP_BYTES : 0); }   // (the kept pieces' room, KEEP_BYTES, is the last of the buffer: the world's and the land's limits do not count it, everyone after them does)
int gfx_bytes_used(void) { return (int)(db_next - db[db_active].p); }
void *gfx_alloc(int bytes) {
    char *p = db_next;
    if (p + bytes > gfx_pk_end) { gfx_overflow++; return 0; }
    db_next = p + ((bytes + 3) & ~3);
    return p;
}
u32 *gfx_ot(void) { return gfx_ot_cur; }   // (the frame's table, or a person's while it is rendered into a sprite)
int  gfx_prims_left(void) { return (int)(db[db_active].p + PACKET_LEN - db_next); }

// ------------------------------------------------------------------ camera
static void cross(const SVECTOR *a, const SVECTOR *b, VECTOR *o) {
    o->vx = ((a->vy * b->vz) - (a->vz * b->vy)) >> 12;
    o->vy = ((a->vz * b->vx) - (a->vx * b->vz)) >> 12;
    o->vz = ((a->vx * b->vy) - (a->vy * b->vx)) >> 12;
}
static void look_at(const VECTOR *eye, const VECTOR *at, MATRIX *m) {
    SVECTOR up = { 0, -ONE, 0, 0 };
    VECTOR t; SVECTOR zx, xx, yx; VECTOR pos, vec;
    setVector(&t, at->vx - eye->vx, at->vy - eye->vy, at->vz - eye->vz);
    VectorNormalS(&t, &zx);
    cross(&zx, &up, &t);  VectorNormalS(&t, &xx);
    cross(&zx, &xx, &t);  VectorNormalS(&t, &yx);
    m->m[0][0] = xx.vx; m->m[1][0] = yx.vx; m->m[2][0] = zx.vx;
    m->m[0][1] = xx.vy; m->m[1][1] = yx.vy; m->m[2][1] = zx.vy;
    m->m[0][2] = xx.vz; m->m[1][2] = yx.vz; m->m[2][2] = zx.vz;
    if (gfx_wide) { m->m[0][0] = m->m[0][0] * 3 / 4; m->m[0][1] = m->m[0][1] * 3 / 4; m->m[0][2] = m->m[0][2] * 3 / 4; }   // (WIDESCREEN: across the picture, three quarters: the TV stretches it back)
    pos.vx = -eye->vx; pos.vy = -eye->vy; pos.vz = -eye->vz;
    ApplyMatrixLV(m, &pos, &vec);
    TransMatrix(m, &vec);
}
void gfx_camera_update(void) { look_at(&cam.eye, &cam.at, &cam.view); }

int (*gfx_lift_fn)(int x, int z);   // (the world's: how high the ground is under a point)
int gfx_lift;   // everything set up from here is raised this many units (the hills: a thing stands on the slope under it)
void gfx_set_object(const VECTOR *pos, int yaw) {
    MATRIX om; SVECTOR rot = { 0, (short)yaw, 0, 0 };
    RotMatrix(&rot, &om);
    // (one ground height: the ground under the object's own origin when the world gives one, else the lift the drawing set. Both were
    //  added, so a car, a bus at its stop or a farm's animals drawn while a hill's lift was set stood twice as high as the ground)
    if (gfx_lift || gfx_lift_fn) { VECTOR p = *pos; p.vy -= gfx_lift_fn ? gfx_lift_fn(pos->vx, pos->vz) : gfx_lift; TransMatrix(&om, &p); } else
    TransMatrix(&om, (VECTOR *)pos);
    CompMatrixLV(&cam.view, &om, &om);
    gte_SetRotMatrix(&om);
    gte_SetTransMatrix(&om);
    obj_yaw = yaw; light_to_object();
}
void gfx_set_object_m(const MATRIX *world) {
    MATRIX om;
    CompMatrixLV(&cam.view, (MATRIX *)world, &om);
    GTE_FENCE(); gte_SetRotMatrix(&om);
    gte_SetTransMatrix(&om);
}
void gfx_set_identity(void) {
    GTE_FENCE(); gte_SetRotMatrix(&cam.view);
    if (gfx_lift) { MATRIX v = cam.view; for (int i = 0; i < 3; i++) v.t[i] -= (gfx_lift * v.m[i][1]) >> 12; gte_SetTransMatrix(&v); } else
    gte_SetTransMatrix(&cam.view);
    obj_yaw = 0; light_to_object();
}

// ------------------------------------------------------------------ lighting
void gfx_set_ambient(int amb, int dif) { light_amb = amb; light_dif = dif; }
static void light_to_object(void) {
    // world light L rotated by -yaw into object space (the object rotates by +yaw about Y)
    int c = icos(obj_yaw), s = isin(obj_yaw);
    lx_obj = (light_dir.vx * c - light_dir.vz * s) >> 12;
    lz_obj = (light_dir.vz * c + light_dir.vx * s) >> 12;
    ly_obj = light_dir.vy;
}
void gfx_set_light_yaw(int yaw) { obj_yaw = yaw; light_to_object(); }
int  gfx_light_amb(void) { return light_amb; }
// the light's step: changes whenever anything gfx_light or the colour grade depend on does (the ambient and sun strength the day-night
// cycle sets, the grade switch). The sun's direction is fixed. Caches of lit colours key on it (with yaw 0: the world's own axes)
int  gfx_light_key(void) { extern int gfx_grade; return 1 + (light_amb & 511) + ((light_dif & 511) << 9) + ((gfx_grade & 1) << 18); }
int gfx_light(int nx, int ny, int nz) {
    int d = (nx * lx_obj + ny * ly_obj + nz * lz_obj) >> 12;   // -4096..4096
    if (d < 0) d = d / 6;                                                        // a little bounce light on the dark side
    int v = light_amb + ((light_dif * d) >> 12);
    return CLAMP(v, 16, 255);
}

// ------------------------------------------------------------------ clipping (Cohen-Sutherland outcodes)
static inline int outcode(int x, int y) {
    int r = 0;
    if (x < 0) r |= 1; else if (x >= SCREEN_W) r |= 2;
    if (y < 0) r |= 4; else if (y >= SCREEN_H) r |= 8;
    return r;
}
int gfx_bb_on; int gfx_bb[4];          // while on, the screen box of everything projected (a sprite render measures its person)
static inline void bb_add(int x, int y) {
    if (x < gfx_bb[0]) gfx_bb[0] = x; if (x > gfx_bb[2]) gfx_bb[2] = x;
    if (y < gfx_bb[1]) gfx_bb[1] = y; if (y > gfx_bb[3]) gfx_bb[3] = y;
}
static inline int quad_offscreen(const DVECTOR *a, const DVECTOR *b, const DVECTOR *c, const DVECTOR *d) {
    if (gfx_bb_on) { bb_add(a->vx, a->vy); bb_add(b->vx, b->vy); bb_add(c->vx, c->vy); bb_add(d->vx, d->vy); }
    int ca = outcode(a->vx, a->vy), cb = outcode(b->vx, b->vy), cc = outcode(c->vx, c->vy), cd = outcode(d->vx, d->vy);
    return (ca & cb & cc & cd) != 0;
}
static inline int tri_offscreen(const DVECTOR *a, const DVECTOR *b, const DVECTOR *c) {
    if (gfx_bb_on) { bb_add(a->vx, a->vy); bb_add(b->vx, b->vy); bb_add(c->vx, c->vy); }
    return (outcode(a->vx, a->vy) & outcode(b->vx, b->vy) & outcode(c->vx, c->vy)) != 0;
}
#define zbias gfx_zbias
static int fog_near = 0, fog_far = 0;   // depth fade: colours darken from fog_near to fog_far (0 = off)
void gfx_set_fog(int near, int far) { fog_near = near; fog_far = far; gfx_fog_on = far != 0; }

// ---- the colour grade: lit surfaces lean warm (sunlight), shaded ones lean cool (sky bounce) instead of plain grey darkening ----
int gfx_grade = 1;
static inline u32 grade_rgb(int r, int g, int b) {      // a lit colour (0..255 each) -> 0xBBGGRR
    if (gfx_grade) {
        int l = (r * 77 + g * 150 + b * 29) >> 8, w = (l - 118) >> 3;
        r += w; b -= w + (w >> 1); g += w >> 2;
    }
    r = CLAMP(r, 0, 255); g = CLAMP(g, 0, 255); b = CLAMP(b, 0, 255);
    return (u32)r | ((u32)g << 8) | ((u32)b << 16);
}
static u32 grade_lut[256]; static int grade_lut_for = -1;
static u32 grade_tex_calc(int s) {
    int r = s, g = s, b = s;
    if (gfx_grade) { int w = (s - 120) >> 2; r += w; b -= w + (w >> 2); g += w >> 3; }
    r = CLAMP(r, 0, 255); g = CLAMP(g, 0, 255); b = CLAMP(b, 0, 255);
    return (u32)r | ((u32)g << 8) | ((u32)b << 16);
}
static inline u32 grade_tex(int s) {                     // a texture shade (128 = the texture as it is), from a table
    if (grade_lut_for != gfx_grade) { for (int i = 0; i < 256; i++) grade_lut[i] = grade_tex_calc(i); grade_lut_for = gfx_grade; }
    return grade_lut[s & 255];
}
static inline u32 gt_grade(u32 c, int f) {
    int r = ((c & 255) * f) >> 8, g = (((c >> 8) & 255) * f) >> 8, b = (((c >> 16) & 255) * f) >> 8;
    if (r == g && g == b) return grade_tex(g);
    return grade_rgb(r, g, b);
}
#define SET_C(q, n, c) setRGB##n(q, (c) & 255, ((c) >> 8) & 255, ((c) >> 16) & 255)
static inline int fog_scale(int otz) {   // 256 = as is
    if (!fog_far || otz <= fog_near) return 256;
    int t = ((otz - fog_near) << 8) / (fog_far - fog_near);
    return t >= 256 ? 64 : 256 - (t * 3 >> 2);
}
// taken off every primitive's depth: decals sit in front of what they are stuck on
void gfx_set_zbias(int b) { zbias = b; }
int gfx_floor_pass;                       // set while drawing the floor: it goes to the very back (nothing is ever under it), so shadows can lie on it
static inline int ot_index(int otz) {
    if (gfx_floor_pass) return OT_FLOOR;
    int i = (otz >> OT_SHIFT) - zbias - gfx_zbase;
    if (i < 1) i = 1;
    if (i > gfx_ot_max) i = gfx_ot_max;
    return i;
}

// ------------------------------------------------------------------ 3D primitives
// Vertex order: a = top-left, b = top-right, c = bottom-left, d = bottom-right as seen from OUTSIDE (clockwise on screen = front).
int gfx_quad_f(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, int r, int g, int b8, int cull) {
    int p, otz, z0, z1, z2, z3;
    POLY_F4 *q = (POLY_F4 *)gfx_alloc(sizeof(POLY_F4));
    if (!q) return 0;
    char *mark_ = (char *)q;
    gte_ld3_safe(a, b, c);
    gte_rtpt();
    if (cull) { gte_nclip(); gte_stopz(&p); if (p <= 0) { db_next = mark_; return 0; } }
    gte_stsz3(&z0, &z1, &z2);
    if (z0 < Z_NEAR || z1 < Z_NEAR || z2 < Z_NEAR) { db_next = mark_; return 0; }
    gte_stsxy3(&q->x0, &q->x1, &q->x2);
    gte_ld0_safe(d);
    gte_rtps();
    gte_stsz(&z3);
    if (z3 < Z_NEAR) { db_next = mark_; return 0; }
    gte_stsxy(&q->x3);
    if (quad_offscreen((DVECTOR *)&q->x0, (DVECTOR *)&q->x1, (DVECTOR *)&q->x2, (DVECTOR *)&q->x3)) { db_next = mark_; return 0; }
    gte_avsz4();
    gte_stotz(&otz);
    setPolyF4(q);
    { int f = fog_scale(otz); u32 c_ = grade_rgb((r * f) >> 8, (g * f) >> 8, (b8 * f) >> 8); SET_C(q, 0, c_); }
    addPrim(gfx_ot() + ot_index(otz), q); gfx_prim_count++; gfx_frame_prims++;
    return 1;
}

int gfx_tri_f(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, int r, int g, int b8, int cull) {
    int p, otz, z0, z1, z2;
    POLY_F3 *q = (POLY_F3 *)gfx_alloc(sizeof(POLY_F3));
    if (!q) return 0;
    char *mark_ = (char *)q;
    gte_ld3_safe(a, b, c);
    gte_rtpt();
    if (cull) { gte_nclip(); gte_stopz(&p); if (p <= 0) { db_next = mark_; return 0; } }
    gte_stsz3(&z0, &z1, &z2);
    if (z0 < Z_NEAR || z1 < Z_NEAR || z2 < Z_NEAR) { db_next = mark_; return 0; }
    gte_stsxy3(&q->x0, &q->x1, &q->x2);
    if (tri_offscreen((DVECTOR *)&q->x0, (DVECTOR *)&q->x1, (DVECTOR *)&q->x2)) { db_next = mark_; return 0; }
    gte_avsz3();
    gte_stotz(&otz);
    setPolyF3(q);
    { int f = fog_scale(otz); u32 c_ = grade_rgb((r * f) >> 8, (g * f) >> 8, (b8 * f) >> 8); SET_C(q, 0, c_); }
    addPrim(gfx_ot() + ot_index(otz), q); gfx_prim_count++; gfx_frame_prims++;
    return 1;
}

int gfx_quad_g(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, u32 c0, u32 c1, u32 c2, u32 c3, int cull) {
    int p, otz, z0, z1, z2, z3;
    POLY_G4 *q = (POLY_G4 *)gfx_alloc(sizeof(POLY_G4));
    if (!q) return 0;
    char *mark_ = (char *)q;
    gte_ld3_safe(a, b, c);
    gte_rtpt();
    if (cull) { gte_nclip(); gte_stopz(&p); if (p <= 0) { db_next = mark_; return 0; } }
    gte_stsz3(&z0, &z1, &z2);
    if (z0 < Z_NEAR || z1 < Z_NEAR || z2 < Z_NEAR) { db_next = mark_; return 0; }
    gte_stsxy3(&q->x0, &q->x1, &q->x2);
    gte_ld0_safe(d);
    gte_rtps();
    gte_stsz(&z3);
    if (z3 < Z_NEAR) { db_next = mark_; return 0; }
    gte_stsxy(&q->x3);
    if (quad_offscreen((DVECTOR *)&q->x0, (DVECTOR *)&q->x1, (DVECTOR *)&q->x2, (DVECTOR *)&q->x3)) { db_next = mark_; return 0; }
    gte_avsz4();
    gte_stotz(&otz);
    setPolyG4(q);
    { u32 g0 = grade_rgb(c0 & 255, (c0 >> 8) & 255, (c0 >> 16) & 255), g1 = grade_rgb(c1 & 255, (c1 >> 8) & 255, (c1 >> 16) & 255),
          g2 = grade_rgb(c2 & 255, (c2 >> 8) & 255, (c2 >> 16) & 255), g3 = grade_rgb(c3 & 255, (c3 >> 8) & 255, (c3 >> 16) & 255);
      SET_C(q, 0, g0); SET_C(q, 1, g1); SET_C(q, 2, g2); SET_C(q, 3, g3); }
    addPrim(gfx_ot() + ot_index(otz), q); gfx_prim_count++; gfx_frame_prims++;
    return 1;
}

int gfx_quad_ft(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, u16 tpage, u16 clut,
                int u0, int v0, int u1, int v1, int u2, int v2, int u3, int v3, int shade, int cull, int semi) {
    int p, otz, z0, z1, z2, z3;
    POLY_FT4 *q = (POLY_FT4 *)gfx_alloc(sizeof(POLY_FT4));
    if (!q) return 0;
    char *mark_ = (char *)q;
    gte_ld3_safe(a, b, c);
    gte_rtpt();
    if (cull) { gte_nclip(); gte_stopz(&p); if (p <= 0) { db_next = mark_; return 0; } }
    gte_stsz3(&z0, &z1, &z2);
    if (z0 < Z_NEAR || z1 < Z_NEAR || z2 < Z_NEAR) { db_next = mark_; return 0; }
    gte_stsxy3(&q->x0, &q->x1, &q->x2);
    gte_ld0_safe(d);
    gte_rtps();
    gte_stsz(&z3);
    if (z3 < Z_NEAR) { db_next = mark_; return 0; }
    gte_stsxy(&q->x3);
    if (quad_offscreen((DVECTOR *)&q->x0, (DVECTOR *)&q->x1, (DVECTOR *)&q->x2, (DVECTOR *)&q->x3)) { db_next = mark_; return 0; }
    gte_avsz4();
    gte_stotz(&otz);
    setPolyFT4(q);
    shade = (shade * fog_scale(otz)) >> 8;
    { u32 c_ = grade_tex(shade); SET_C(q, 0, c_); }
    q->tpage = tpage; q->clut = clut;
    q->u0 = u0; q->v0 = v0; q->u1 = u1; q->v1 = v1; q->u2 = u2; q->v2 = v2; q->u3 = u3; q->v3 = v3;
    if (semi) setSemiTrans(q, 1);
    addPrim(gfx_ot() + ot_index(otz), q); gfx_prim_count++; gfx_frame_prims++;
    return 1;
}

int gfx_quad_gt(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, u16 tpage, u16 clut,
                int u0, int v0, int u1, int v1, int u2, int v2, int u3, int v3, u32 c0, u32 c1, u32 c2, u32 c3, int cull) {
    int p, otz, z0, z1, z2, z3;
    POLY_GT4 *q = (POLY_GT4 *)gfx_alloc(sizeof(POLY_GT4));
    if (!q) return 0;
    char *mark_ = (char *)q;
    gte_ld3_safe(a, b, c);
    gte_rtpt();
    if (cull) { gte_nclip(); gte_stopz(&p); if (p <= 0) { db_next = mark_; return 0; } }
    gte_stsz3(&z0, &z1, &z2);
    if (z0 < Z_NEAR || z1 < Z_NEAR || z2 < Z_NEAR) { db_next = mark_; return 0; }
    gte_stsxy3(&q->x0, &q->x1, &q->x2);
    gte_ld0_safe(d);
    gte_rtps();
    gte_stsz(&z3);
    if (z3 < Z_NEAR) { db_next = mark_; return 0; }
    gte_stsxy(&q->x3);
    if (quad_offscreen((DVECTOR *)&q->x0, (DVECTOR *)&q->x1, (DVECTOR *)&q->x2, (DVECTOR *)&q->x3)) { db_next = mark_; return 0; }
    gte_avsz4();
    gte_stotz(&otz);
    setPolyGT4(q);
    { int f = fog_scale(otz);
      u32 g0 = gt_grade(c0, f), g1 = gt_grade(c1, f), g2 = gt_grade(c2, f), g3 = gt_grade(c3, f);
      SET_C(q, 0, g0); SET_C(q, 1, g1); SET_C(q, 2, g2); SET_C(q, 3, g3); }
    q->tpage = tpage; q->clut = clut;
    q->u0 = u0; q->v0 = v0; q->u1 = u1; q->v1 = v1; q->u2 = u2; q->v2 = v2; q->u3 = u3; q->v3 = v3;
    addPrim(gfx_ot() + ot_index(otz), q); gfx_prim_count++; gfx_frame_prims++;
    return 1;
}

// ------------------------------------------------------------------ the fast path: transform shared vertices once, then emit faces
// Meshes (the Bores' blocks, the world's tiles) share corners between faces: each corner is transformed once (three at a time on the
// GTE) into an XV, and the faces are emitted from those, with the back-face test done on the CPU from the screen positions. The colours
// handed in are already graded (gfx_grade_*), so emitting is a handful of stores.
void gfx_xform(const SVECTOR *v, XV *o, int n) {
    // the GTE loads words: an SVECTOR array that sits on a 2-byte boundary (they are structs of shorts) would crash a real
    // PlayStation with an address error. Such an array is copied to an aligned one first (callers declare theirs SVEC_ALIGN).
    if ((u32)v & 3) {
        static SVECTOR tmp[64] SVEC_ALIGN;
        for (int k = 0; k < n; k += 64) { int m = MIN(64, n - k); memcpy(tmp, v + k, m * sizeof(SVECTOR)); gfx_xform(tmp, o + k, m); }
        return;
    }
    int i = 0;
    for (; i + 3 <= n; i += 3) {
        GTE_FENCE(); gte_ldv3c((SVECTOR *)&v[i]);
        gte_rtpt();
        gte_stsxy3(&o[i].x, &o[i + 1].x, &o[i + 2].x);
        gte_stsz3(&o[i].z, &o[i + 1].z, &o[i + 2].z);
    }
    for (; i < n; i++) { GTE_FENCE(); gte_ldv0((SVECTOR *)&v[i]); gte_rtps(); gte_stsxy(&o[i].x); gte_stsz(&o[i].z); }
    if (gfx_bb_on) for (i = 0; i < n; i++) if (o[i].z >= Z_NEAR) bb_add(o[i].x, o[i].y);
}
u32 gfx_grade_flat(u32 c) { return grade_rgb(c & 255, (c >> 8) & 255, (c >> 16) & 255); }
u32 gfx_grade_tex(u32 c) { return gt_grade(c, 256); }
const u32 *gfx_grade_lut(void) { grade_tex(0); return grade_lut; }   // grade_lut[s] == gfx_grade_tex(s * 0x010101)
static inline u32 fog_col(u32 c, int f) { return (((c & 255) * f) >> 8) | (((((c >> 8) & 255) * f) >> 8) << 8) | (((((c >> 16) & 255) * f) >> 8) << 16); }
u32 gfx_fog_apply(u32 c, int otz) { return fog_col(c, fog_scale(otz)); }
#undef XV_TEST4

// is a sphere (world centre, radius) inside the view? (no matrix of an object needed: for culling people and things before any work)
int gfx_visible(int wx, int wy, int wz, int r) {
    VECTOR d, v; setVector(&d, wx - cam.eye.vx, wy - cam.eye.vy, wz - cam.eye.vz);
    ApplyMatrixLV(&cam.view, &d, &v);
    if (v.vz < Z_NEAR - r) return 0;
    int rr = r + (r >> 2);                                                 // the planes lean out: a sphere reaches a touch further
    if (ABS(v.vx) > ((v.vz * CENTER_X) / 230) + rr) return 0;
    if (ABS(v.vy) > ((v.vz * CENTER_Y) / 230) + rr) return 0;
    return 1;
}
int gfx_project(const SVECTOR *v, int *sx, int *sy, int *sz) {
    DVECTOR xy; int z;
    gte_ld0_safe(v);
    gte_rtps();
    gte_stsz(&z);
    gte_stsxy(&xy);
    *sx = xy.vx; *sy = xy.vy; *sz = z;
    return z >= Z_NEAR;
}

// ------------------------------------------------------------------ 2D overlay
// WIDESCREEN: the 2D interface is laid out for 320 across, and the screen stretches 320 to 16:9, so every 2D x is pulled in 3:4 about the middle
// (the picture it ends up as is the same shape as on a 4:3 screen, with the 3D world showing to the sides of it). A box that fills the whole
// screen keeps filling it. ux_pre is the opposite: a screen position from the 3D world, made ready for ui_x.
static inline int ui_x(int x) { return gfx_wide ? 160 + (x - 160) * 3 / 4 : x; }
int gfx_ui_pre(int x) { return gfx_wide ? 160 + (x - 160) * 4 / 3 : x; }
static inline int ui_full(int x, int w) { return x <= 0 && x + w >= SCREEN_W; }
static void ui_add(void *prim) {
    setaddr(prim, 0xffffff);
    if (ui_tail) setaddr(ui_tail, prim); else ui_head = (u32 *)prim;
    ui_tail = (u32 *)prim;
}
// ---- pieces of the 2D overlay kept in place. The HUD's cards (the needs, the Bore's card) are the same picture frame after frame; drawing them
// is some 30 hb a frame. A kept piece is drawn once into its own corner at the end of the packet buffer; the buffer is not cleared between
// the two frames that share it, so a frame that finds its buffer's piece still as the same key made it just links it into the chain again
// (nothing is copied: its packets are where they were). A different key: it is drawn again, into the same place. A piece is checked when it
// is made: its packets must be the whole of what was allocated and one run of the chain, or it is never kept.
#define KEEP_SLOTS 2
static const short keep_off[KEEP_SLOTS + 1] = { 0, 2048, 5120 };   // the needs' card (1.6 KB most), the Bore's card (2 KB most)
static u32 keep_key[2][KEEP_SLOTS]; static u32 *keep_last[2][KEEP_SLOTS]; static u8 keep_bad[KEEP_SLOTS];
static int keep_cur = -1, keep_ovf; static char *keep_sv_next, *keep_sv_end; static u32 keep_k, *keep_tail0;
int gfx_keep_begin(int slot, u32 key) {   // 1: it is in place and linked, draw nothing. 0: draw it, then gfx_keep_end(slot)
#ifdef BORE_DC
    return 0;
#endif
    if (keep_bad[slot] || keep_cur >= 0 || !ui_tail) return 0;
    int b = db_active; char *rs = db[b].p + PACKET_LEN - 64 - KEEP_BYTES + keep_off[slot];
    if (keep_key[b][slot] == key && keep_last[b][slot]) {
        u32 *last = keep_last[b][slot]; setaddr(ui_tail, rs); ui_tail = last; setaddr(last, 0xffffff);
        return 1;
    }
    keep_key[b][slot] = 0; keep_last[b][slot] = 0;
    keep_cur = slot; keep_sv_next = db_next; keep_sv_end = gfx_pk_end; keep_k = key; keep_tail0 = ui_tail; keep_ovf = gfx_overflow;
    db_next = rs; gfx_pk_end = db[b].p + PACKET_LEN - 64 - KEEP_BYTES + keep_off[slot + 1];
    return 0;
}
void gfx_keep_end(int slot) {
    if (keep_cur != slot) return;
    keep_cur = -1; int b = db_active; char *rs = db[b].p + PACKET_LEN - 64 - KEEP_BYTES + keep_off[slot];
    int bytes = (int)(db_next - rs), ok = bytes > 0 && gfx_overflow == keep_ovf && ui_tail;
    db_next = keep_sv_next; gfx_pk_end = keep_sv_end;
    if (ok) { ok = ((*keep_tail0 & 0xffffff) == ((u32)rs & 0xffffff)); u32 *q = (u32 *)rs; int tot = 0;   // (the chain from the piece's first packet to the tail: all of it, and only it)
              while (ok) { tot += (int)((*q >> 24) + 1) * 4; if (q == ui_tail) break; q = (u32 *)(0x80000000u | (*q & 0xffffff)); if (tot > bytes) break; }
              ok = ok && tot == bytes; }
    if (ok) { keep_key[b][slot] = keep_k; keep_last[b][slot] = ui_tail; } else keep_bad[slot] = 1;
}
void gfx_rect(int x, int y, int w, int h, int r, int g, int b, int semi) {
    TILE *t = (TILE *)gfx_alloc(sizeof(TILE));
    if (!t) return;
    setTile(t);
    if (gfx_wide && !ui_full(x, w)) { int x1 = ui_x(x + w); x = ui_x(x); w = x1 - x; }
    setXY0(t, x, y); setWH(t, w, h);
    setRGB0(t, r, g, b);
    if (semi) setSemiTrans(t, 1);
    ui_add(t);
}
void gfx_rect_c(int x, int y, int w, int h, u16 c, int semi) { gfx_rect(x, y, w, h, R8(c), G8(c), B8(c), semi); }
#ifdef SAFE_LINT
void safe_note(int kind, int x, int y, int w, int h, const char *t);
#define SAFE_BOX(x, y, w, h) safe_note(2, x, y, w, h, 0)
#else
#define SAFE_BOX(x, y, w, h)
#endif
void gfx_grad(int x, int y, int w, int h, u32 c0, u32 c1, u32 c2, u32 c3, int semi) {   // a 2D box shaded corner to corner (TL TR BL BR, 0xBBGGRR)
    if (w > 20 && h > 6) SAFE_BOX(x, y, w, h);
    POLY_G4 *q = (POLY_G4 *)gfx_alloc(sizeof(POLY_G4));
    if (!q) return;
    setPolyG4(q);
    if (gfx_wide && !ui_full(x, w)) { int x1 = ui_x(x + w); x = ui_x(x); w = x1 - x; }
    setXY4(q, x, y, x + w, y, x, y + h, x + w, y + h);
    setRGB0(q, c0 & 255, (c0 >> 8) & 255, (c0 >> 16) & 255); setRGB1(q, c1 & 255, (c1 >> 8) & 255, (c1 >> 16) & 255);
    setRGB2(q, c2 & 255, (c2 >> 8) & 255, (c2 >> 16) & 255); setRGB3(q, c3 & 255, (c3 >> 8) & 255, (c3 >> 16) & 255);
    if (semi) setSemiTrans(q, 1);
    ui_add(q);
}
void gfx_ui_blend(int abr) {   // how the 2D overlay's see-through boxes mix from here on: 0 half and half, 1 add, 2 subtract, 3 add a quarter
    u32 *p = (u32 *)gfx_alloc(8); if (!p) return;
    p[1] = 0xE1000000u | ((u32)(abr & 3) << 5) | (1u << 9) | (1u << 10);
    p[0] = 0x01000000u;
    ui_add(p);
}
void gfx_line(int x0, int y0, int x1, int y1, int r, int g, int b) {
    LINE_F2 *l = (LINE_F2 *)gfx_alloc(sizeof(LINE_F2));
    if (!l) return;
    setLineF2(l);
    setXY2(l, ui_x(x0), y0, ui_x(x1), y1);
    setRGB0(l, r, g, b);
    ui_add(l);
}
void gfx_sprite(int x, int y, int w, int h, u16 tpage, u16 clut, int u, int v, int tw, int th, int shade, int semi) {
    POLY_FT4 *q = (POLY_FT4 *)gfx_alloc(sizeof(POLY_FT4));
    if (!q) return;
    setPolyFT4(q);
    setXY4(q, x, y, x + w, y, x, y + h, x + w, y + h);
    q->u0 = u; q->v0 = v; q->u1 = u + tw - 1; q->v1 = v; q->u2 = u; q->v2 = v + th - 1; q->u3 = u + tw - 1; q->v3 = v + th - 1;
    q->tpage = tpage; q->clut = clut;
    setRGB0(q, shade, shade, shade);
    if (semi) setSemiTrans(q, 1);
    ui_add(q);
}
void gfx_sky(u32 top, u32 bottom) {
    POLY_G4 *q = (POLY_G4 *)gfx_alloc(sizeof(POLY_G4));
    if (!q) return;
    setPolyG4(q);
    setXY4(q, 0, 0, SCREEN_W, 0, 0, SCREEN_H, SCREEN_W, SCREEN_H);
    setRGB0(q, top & 255, (top >> 8) & 255, (top >> 16) & 255); setRGB1(q, top & 255, (top >> 8) & 255, (top >> 16) & 255);
    setRGB2(q, bottom & 255, (bottom >> 8) & 255, (bottom >> 16) & 255); setRGB3(q, bottom & 255, (bottom >> 8) & 255, (bottom >> 16) & 255);
    addPrim(gfx_ot() + (OT_LEN - 1), q);
}
// a soft round shadow: a radial gradient texture drawn SUBTRACTIVE (the floor minus the texel), so it fades out smoothly at its rim
#include "atlas.h"
static TexRef shadow_tex; static u16 shadow_tpage; static int shadow_ready;
static void shadow_init(void) {
    u16 *px = atlas_scratch_hi();   // (32 x 32)
    for (int y = 0; y < 32; y++) for (int x = 0; x < 32; x++) {
        int dx = x * 2 - 31, dy = y * 2 - 31, d2 = dx * dx + dy * dy;          // 0 .. 961 at the rim
        int a = 961 - d2; if (a < 0) a = 0;
        int v = (a * a / 961) * 11 / 961;                                     // (1 - r^2)^2, up to 11/31 dark
        if (d2 < 961 && v < 1 && a > 0) v = 1;
        px[y * 32 + x] = v ? (u16)(0x8000 | v | (v << 5) | (v << 10)) : 0;  // STP set: this texel blends; 0: nothing drawn
    }
    shadow_tex = atlas_add(32, 32, px);
    shadow_tpage = (u16)((shadow_tex.tpage & ~(3 << 5)) | (2 << 5));         // blend mode 2: back - front
    shadow_ready = 1;
}
void gfx_shadow_quad(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d) {   // the soft shadow over any flat quad (current object space)
    if (!shadow_ready) shadow_init();
    POLY_FT4 *q = (POLY_FT4 *)gfx_alloc(sizeof(POLY_FT4)); if (!q) return; char *mark_ = (char *)q;
    int otz;
    gte_ld3_safe(a, b, c); gte_rtpt();
    gte_stsxy3(&q->x0, &q->x1, &q->x2); gte_ld0_safe(d); gte_rtps(); gte_stsxy(&q->x3);
    gte_avsz4(); gte_stotz(&otz); if (otz < Z_NEAR) { db_next = mark_; return; }
    if (quad_offscreen((DVECTOR *)&q->x0, (DVECTOR *)&q->x1, (DVECTOR *)&q->x2, (DVECTOR *)&q->x3)) { db_next = mark_; return; }
    setPolyFT4(q); setRGB0(q, 128, 128, 128); setSemiTrans(q, 1);
    int u0 = shadow_tex.u, v0 = shadow_tex.v, u1 = u0 + 31, v1 = v0 + 31;
    q->u0 = u0; q->v0 = v0; q->u1 = u1; q->v1 = v0; q->u2 = u0; q->v2 = v1; q->u3 = u1; q->v3 = v1;
    q->tpage = shadow_tpage; q->clut = 0;
    addPrim(gfx_ot() + OT_SHADOW, q);
}
int gfx_noshadow;   // (the options menu: no soft shadows at all)
void gfx_shadow(const VECTOR *pos, int radius) {
    if (gfx_noshadow) return;
    if (!shadow_ready) shadow_init();
    gfx_set_object(pos, 0);
    SVECTOR a, b, c, d; int r = radius + radius / 3;
    setVector(&a, -r, -2, r); setVector(&b, r, -2, r); setVector(&c, -r, -2, -r); setVector(&d, r, -2, -r);
    POLY_FT4 *q = (POLY_FT4 *)gfx_alloc(sizeof(POLY_FT4)); if (!q) return; char *mark_ = (char *)q;
    int otz, p;
    gte_ld3_safe(&a, &b, &c); gte_rtpt();                                        // (no back-face test: it lies flat on the floor, seen from above)
    gte_stsxy3(&q->x0, &q->x1, &q->x2); gte_ld0_safe(&d); gte_rtps(); gte_stsxy(&q->x3);
    gte_avsz4(); gte_stotz(&otz); if (otz < Z_NEAR) { db_next = mark_; return; }
    (void)p;
    setPolyFT4(q); setRGB0(q, 128, 128, 128); setSemiTrans(q, 1);
    int u0 = shadow_tex.u, v0 = shadow_tex.v, u1 = u0 + 31, v1 = v0 + 31;
    q->u0 = u0; q->v0 = v0; q->u1 = u1; q->v1 = v0; q->u2 = u0; q->v2 = v1; q->u3 = u1; q->v3 = v1;
    q->tpage = shadow_tpage; q->clut = 0;
    addPrim(gfx_ot() + OT_SHADOW, q);
}
void gfx_sprite_c(int x, int y, int w, int h, u16 tpage, int u, int v, int tw, int th, int r, int g, int b) {   // a 2D textured quad, tinted (128 = as is)
    POLY_FT4 *q = (POLY_FT4 *)gfx_alloc(sizeof(POLY_FT4));
    if (!q) return;
    setPolyFT4(q);
    if (gfx_wide && !ui_full(x, w)) { int x1 = ui_x(x + w); x = ui_x(x); w = x1 - x; }
    setXY4(q, x, y, x + w, y, x, y + h, x + w, y + h);
    q->u0 = u; q->v0 = v; q->u1 = u + tw - 1; q->v1 = v; q->u2 = u; q->v2 = v + th - 1; q->u3 = u + tw - 1; q->v3 = v + th - 1;
    q->tpage = tpage; q->clut = 0;
    setRGB0(q, r, g, b);
    ui_add(q);
}
void gfx_sprite_blend(int x, int y, int w, int h, u16 tpage, u16 clut, int u, int v, int tw, int th, int r, int g, int b) {
    POLY_FT4 *q = (POLY_FT4 *)gfx_alloc(sizeof(POLY_FT4));
    if (!q) return;
    setPolyFT4(q);
    if (gfx_wide && !ui_full(x, w)) { int x1 = ui_x(x + w); x = ui_x(x); w = x1 - x; }
    setXY4(q, x, y, x + w, y, x, y + h, x + w, y + h);
    q->u0 = u; q->v0 = v; q->u1 = u + tw - 1; q->v1 = v; q->u2 = u; q->v2 = v + th - 1; q->u3 = u + tw - 1; q->v3 = v + th - 1;
    q->tpage = tpage; q->clut = clut;
    setRGB0(q, r, g, b);
    setSemiTrans(q, 1);
    ui_add(q);
}
// the text: scale 1 = the body style, 2 = the heading style, 3 and up = the big one (font.h has the six styles; the *_s calls take one)
int gfx_text_w(int scale, const char *s) { return font_w(font_style_of(scale), s); }
int gfx_text_ws(int style, const char *s) { return font_w(style, s); }
int gfx_text_wi(int style, const char *s) { return font_ink_w(style, s); }
int ui_light;   // text is going onto a white card (ui.c): light colours come out dark (white as ink, gold as blue, the rest deeper)
extern int font_sub;
#ifdef SAFE_LINT
// (test builds: 2D things outside the safe area, 8 pixels in from every edge, where a TV's overscan may hide them: kind 1 text, 2 a rect or card)
short safe_log[200][6]; char safe_txt[200][12]; int safe_n; extern int gfx_frame_no;
void safe_note(int kind, int x, int y, int w, int h, const char *t) {
    #if SAFE_LINT < 2
    static int fr = -1; if (fr != gfx_frame_no) { fr = gfx_frame_no; safe_n = 0; }   // (the log holds the last frame drawn; SAFE_LINT=2: everything, for the whole run)
#endif
    extern int safe_off; if (gfx_wide || safe_off) return;
    if (x >= 8 && y >= 8 && x + w <= 312 && y + h <= 232) return;
    if (w >= 300 && h >= 200) return;   // (a whole-screen backdrop)
    if (kind == 2 && (x <= 0 || y <= 0 || x + w >= SCREEN_W || y + h >= SCREEN_H)) return;   // (a backdrop that bleeds off the screen is meant to)
    for (int i = 0; i < safe_n; i++) if (safe_log[i][0] == kind && safe_log[i][1] == x && safe_log[i][2] == y && safe_log[i][3] == w && safe_log[i][4] == h) return;
    if (safe_n < 200) { safe_log[safe_n][0] = kind; safe_log[safe_n][1] = x; safe_log[safe_n][2] = y; safe_log[safe_n][3] = w; safe_log[safe_n][4] = h; safe_log[safe_n][5] = (short)gfx_frame_no; if (t) { strncpy(safe_txt[safe_n], t, 11); safe_txt[safe_n][11] = 0; } else safe_txt[safe_n][0] = 0; safe_n++; }
}
#define SAFE_TEXT(x, y, s) safe_note(1, x, y, gfx_text_w(1, s), 8, s)
#else
#define SAFE_TEXT(x, y, s)
#endif
static void text_raw_s(int style, int x, int y, const char *s, int r, int g, int b) {   // light colours added to the picture, dark ones taken from it
    SAFE_TEXT(x, y, s);
    int lum = (r * 77 + g * 150 + b * 29) >> 8;
    if (lum < 140) { font_sub = 1; font_text(style, x, y, s, 255 - r, 255 - g, 255 - b); font_sub = 0; }   // (on white: exactly this colour)
    else font_text(style, x, y, s, r, g, b);
}
static void text_s(int style, int x, int y, const char *s, int r, int g, int b) {
    if (ui_light) {
        int mx = MAX(r, MAX(g, b)), mn = MIN(r, MIN(g, b));
        if (mx >= 140) {
            if (mx - mn < 40) { if (mx >= 225) { r = 48; g = 56; b = 72; } else if (mx >= 175) { r = 84; g = 94; b = 112; } else { r = 120; g = 130; b = 146; } }
            else if (r >= 200 && g >= 140 && b <= 120) { r = 26; g = 112; b = 190; }                 // gold: the accent blue
            else { r = r * 9 / 20; g = g * 9 / 20; b = b * 9 / 20; }                                 // a light colour, deepened
        }
    }
    text_raw_s(style, x, y, s, r, g, b);
}
void gfx_text_raw(int x, int y, int scale, const char *s, int r, int g, int b) { text_raw_s(font_style_of(scale), x, y, s, r, g, b); }
void gfx_text(int x, int y, int scale, const char *s, int r, int g, int b) { text_s(font_style_of(scale), x, y, s, r, g, b); }
void gfx_textc(int cx, int y, int scale, const char *s, int r, int g, int b) { int st = font_style_of(scale); text_s(st, cx - font_ink_w(st, s) / 2, y, s, r, g, b); }
void gfx_text_s(int style, int x, int y, const char *s, int r, int g, int b) { text_s(style, x, y, s, r, g, b); }
void gfx_text_raw_s(int style, int x, int y, const char *s, int r, int g, int b) { text_raw_s(style, x, y, s, r, g, b); }
void gfx_textc_s(int style, int cx, int y, const char *s, int r, int g, int b) { text_s(style, cx - font_ink_w(style, s) / 2, y, s, r, g, b); }
// the centring helper: the text is measured (its ink, not the space after it), then placed in the box x, y, w, h; flags: TX_LEFT / TX_CENTER / TX_RIGHT
// with TX_TOP / TX_MID / TX_BOTTOM (vertical: the capitals are centred, the baseline sits cap rows under their top). Text wider than the box starts at its left edge.
void gfx_text_box(int style, int x, int y, int w, int h, int flags, const char *s, int r, int g, int b) {
    int iw = font_ink_w(style, s), cap = font_cap(style);
    int tx = (flags & 3) == TX_CENTER ? x + (w - iw) / 2 : (flags & 3) == TX_RIGHT ? x + w - iw : x;
    if (iw > w) tx = x;
    int ty = (flags & 12) == TX_MID ? y + (h - cap) / 2 : (flags & 12) == TX_BOTTOM ? y + h - cap : y;
    text_s(style, tx, ty, s, r, g, b);
}
void gfx_text_box_raw(int style, int x, int y, int w, int h, int flags, const char *s, int r, int g, int b) {   // (no white-card colour mapping)
    int iw = font_ink_w(style, s), cap = font_cap(style);
    int tx = (flags & 3) == TX_CENTER ? x + (w - iw) / 2 : (flags & 3) == TX_RIGHT ? x + w - iw : x;
    if (iw > w) tx = x;
    int ty = (flags & 12) == TX_MID ? y + (h - cap) / 2 : (flags & 12) == TX_BOTTOM ? y + h - cap : y;
    text_raw_s(style, tx, ty, s, r, g, b);
}


// ------------------------------------------------------------------ people as sprites (render-to-texture)
// A person far enough off is drawn as a sprite: the full model, rendered (with the scene's own camera, so the very same pixels)
// into a 48x64 cell of the free VRAM under the screens (x 0..623, y 256..511), then shown as one textured quad until it is
// redrawn. Each render is a small display list of its own, queued to the GPU at once: it runs before this frame's scene
// (the scene is queued at the flip), so the scene's sprite quads always find the fresh pixels.
static u32 *imp_ot, *imp_keep_ot; static int imp_keep_max, imp_keep_base, imp_scaled;
#define IMP_OT 128
#ifndef IMP_MASK
#define IMP_MASK 1
#endif
#ifndef IMP_FILL
#define IMP_FILL 0
#endif
int gfx_imp_begin(int cx, int cy, int cw, int ch, int ox, int oy, int zmid, int hscale) {
    u32 *ot = (u32 *)gfx_alloc(IMP_OT * 4); if (!ot) return 0;
    u32 *hd = (u32 *)gfx_alloc(10 * 4); u32 *tl = (u32 *)gfx_alloc(3 * 4);
    if (!hd || !tl) return 0;
    ClearOTagR(ot, IMP_OT);
    // the head (drawn first): draw mode, the cell as the drawing area, the offset that puts the person into it; the cell
    // cleared to 0 (see-through) by a plain rectangle (a FILL would round its x to 16 and wipe a neighbour cell); then set-mask
    // on, so every pixel of the person gets bit 15 (even black is not see-through)
    hd[1] = 0xE1000000u | 0x200 | 0x400;
    hd[2] = 0xE3000000u | (u32)cx | ((u32)cy << 10);
    hd[3] = 0xE4000000u | (u32)(cx + cw - 1) | ((u32)(cy + ch - 1) << 10);
    hd[4] = 0xE5000000u | ((u32)ox & 0x7ff) | (((u32)oy & 0x7ff) << 11);
    hd[5] = 0xE6000000u;
    hd[6] = 0x60000000u | IMP_FILL; hd[7] = (u32)((cx - ox) & 0xffff) | ((u32)((cy - oy) & 0xffff) << 16); hd[8] = (u32)cw | ((u32)ch << 16);
    hd[9] = 0xE6000000u | IMP_MASK;
    hd[0] = (9u << 24) | (ot[IMP_OT - 1] & 0xffffff); ot[IMP_OT - 1] = (u32)hd & 0xffffff;
    // the tail (drawn last): set-mask off, flush the texture cache (the cell is a texture from here on)
    tl[1] = 0xE6000000u; tl[2] = 0x01000000u;
    tl[0] = (2u << 24) | (ot[0] & 0xffffff); ot[0] = (u32)tl & 0xffffff;
    imp_keep_ot = gfx_ot_cur; imp_keep_max = gfx_ot_max; imp_keep_base = gfx_zbase;
    gfx_ot_cur = ot; gfx_ot_max = IMP_OT - 1; gfx_zbase = (zmid >> OT_SHIFT) - IMP_OT / 2;
    imp_ot = ot; imp_scaled = hscale != 4096;
    if (imp_scaled) gte_SetGeomScreen((230 * hscale) >> 12);
    return 1;
}
void gfx_imp_end(void) {
    if (imp_scaled) gte_SetGeomScreen(230);
    gfx_ot_cur = imp_keep_ot; gfx_ot_max = imp_keep_max; gfx_zbase = imp_keep_base;
    DrawOTag(imp_ot + IMP_OT - 1);
}
// the sprite: cell (cx,cy) texels (tw x th) shown at screen (x,y) w x h, at depth otz; dx = a shear of the top edge (a sway)
void gfx_imp_sprite(int cx, int cy, int tw, int th, int x, int y, int w, int h, int otz, int dx) {
    u32 *q = (u32 *)gfx_alloc(10 * 4); if (!q) return;
    int bx = cx & ~63, u = cx - bx, v = cy - 256;
    u32 tp = (u32)getTPage(2, 0, bx, 256);
    q[1] = 0x2D808080u;                                                    // textured, raw (the pixels as rendered)
    q[2] = (u32)((x + dx) & 0xffff) | ((u32)(y & 0xffff) << 16);           q[3] = (u32)u | ((u32)v << 8);
    q[4] = (u32)((x + w + dx) & 0xffff) | ((u32)(y & 0xffff) << 16);       q[5] = (u32)(u + tw - 1) | ((u32)v << 8) | (tp << 16);
    q[6] = (u32)(x & 0xffff) | ((u32)((y + h) & 0xffff) << 16);           q[7] = (u32)u | ((u32)(v + th - 1) << 8);
    q[8] = (u32)((x + w) & 0xffff) | ((u32)((y + h) & 0xffff) << 16);     q[9] = (u32)(u + tw - 1) | ((u32)(v + th - 1) << 8);
    int i = ot_index(otz);
    q[0] = (9u << 24) | (gfx_ot_cur[i] & 0xffffff); gfx_ot_cur[i] = (u32)q & 0xffffff;
    gfx_frame_prims++;
}
// a point's screen position and depth under the camera (world space), by the GTE itself (so it matches the drawing exactly)
int gfx_proj(int wx, int wy, int wz, int *sx, int *sy) {
    SVECTOR v SVEC_ALIGN; setVector(&v, wx, wy, wz);
    gte_SetRotMatrix(&cam.view); gte_SetTransMatrix(&cam.view);
    int z; if (!gfx_project(&v, sx, sy, &z)) return 0;
    return z;
}
// a small textured sprite sorted into the 3D at view depth z (a Nubbin in a crowd): tinted (128 = as painted), 0 pixels see-through
void gfx_ot_sprite(int x, int y, int w, int h, u16 tpage, int u, int v, int tw, int th, int z, int r, int g, int b) {
    if (x + w < 0 || y + h < 0 || x >= SCREEN_W || y >= SCREEN_H || z < Z_NEAR) return;
    u32 *q = (u32 *)gfx_alloc(10 * 4); if (!q) return;
    q[1] = 0x2C000000u | ((u32)(b & 255) << 16) | ((u32)(g & 255) << 8) | (u32)(r & 255);
    q[2] = (u32)(x & 0xffff) | ((u32)(y & 0xffff) << 16);             q[3] = (u32)u | ((u32)v << 8);
    q[4] = (u32)((x + w) & 0xffff) | ((u32)(y & 0xffff) << 16);       q[5] = (u32)(u + tw - 1) | ((u32)v << 8) | ((u32)tpage << 16);
    q[6] = (u32)(x & 0xffff) | ((u32)((y + h) & 0xffff) << 16);       q[7] = (u32)u | ((u32)(v + th - 1) << 8);
    q[8] = (u32)((x + w) & 0xffff) | ((u32)((y + h) & 0xffff) << 16); q[9] = (u32)(u + tw - 1) | ((u32)(v + th - 1) << 8);
    int i = ot_index(z);
    q[0] = (9u << 24) | (gfx_ot_cur[i] & 0xffffff); gfx_ot_cur[i] = (u32)q & 0xffffff;
    gfx_frame_prims++;
}
