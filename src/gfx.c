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
static int  alloc_reserve;        // bytes of the packet buffer gfx_alloc keeps back (gfx_set_reserve)
static char *db_next;
static RECT screen_clip;
static u32 *ui_head, *ui_tail;    // the 2D overlay chain, drawn after everything in call order
static int  clear_r = 20<<3, clear_g = 26<<3, clear_b = 31<<3;   // BORE's SKY colour

Camera cam;
static int obj_yaw;               // yaw of the current object (for lighting)
static int lx_obj, ly_obj, lz_obj; // the sun, turned into the current object's space (set with the yaw)
static void light_to_object(void);
static int light_amb = 100, light_dif = 120;
static SVECTOR light_dir = { -1500, -3200, 2100, 0 };   // towards the sun (y is down, so negative = up), ~unit length (4096)

extern uint16_t _font_tpage, _font_clut;   // the SDK debug font, uploaded by FntLoad

// ------------------------------------------------------------------ setup
void gfx_set_clear(int r, int g, int b) { clear_r = r; clear_g = g; clear_b = b; }

void gfx_init(void) {
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
    db_next = db[0].p;
    ui_head = ui_tail = 0;
    setRECT(&screen_clip, 0, 0, SCREEN_W, SCREEN_H);

    InitGeom();
    gte_SetGeomOffset(CENTER_X, CENTER_Y);
    gte_SetGeomScreen(230);          // a 70 degree field of view
    gte_SetBackColor(63, 63, 63);

    FntLoad(960, 0);
    FntOpen(4, 4, SCREEN_W - 8, SCREEN_H - 8, 0, 512);

    setVector(&cam.eye, 0, -1200, -1800);
    setVector(&cam.at, 0, -150, 0);
    gfx_camera_update();
    light_to_object();
}

void gfx_flip(void) {
    // hook the 2D chain into OT index 0 (drawn last)
    if (ui_head) {
        u32 nxt = db[db_active].ot[0] & 0xffffff;
        setaddr(ui_tail, nxt);
        setaddr(&db[db_active].ot[0], ui_head);
    }
    DrawSync(0);
    VSync(0);
    db_active ^= 1;
    db_next = db[db_active].p;
    alloc_reserve = 0;
    ui_head = ui_tail = 0;
    ClearOTagR(db[db_active].ot, OT_LEN);
    setRGB0(&db[db_active].draw, clear_r, clear_g, clear_b);
    PutDrawEnv(&db[db_active].draw);
    PutDispEnv(&db[db_active].disp);
    SetDispMask(1);
    DrawOTag(db[1 - db_active].ot + (OT_LEN - 1));
}

void gfx_set_reserve(int bytes) { alloc_reserve = bytes; }
void *gfx_alloc(int bytes) {
    char *p = db_next;
    if (p + bytes > db[db_active].p + PACKET_LEN - 64 - alloc_reserve) return 0;
    db_next = p + ((bytes + 3) & ~3);
    return p;
}
u32 *gfx_ot(void) { return db[db_active].ot; }
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
    pos.vx = -eye->vx; pos.vy = -eye->vy; pos.vz = -eye->vz;
    ApplyMatrixLV(m, &pos, &vec);
    TransMatrix(m, &vec);
}
void gfx_camera_update(void) { look_at(&cam.eye, &cam.at, &cam.view); }

void gfx_set_object(const VECTOR *pos, int yaw) {
    MATRIX om; SVECTOR rot = { 0, (short)yaw, 0, 0 };
    RotMatrix(&rot, &om);
    TransMatrix(&om, (VECTOR *)pos);
    CompMatrixLV(&cam.view, &om, &om);
    gte_SetRotMatrix(&om);
    gte_SetTransMatrix(&om);
    obj_yaw = yaw; light_to_object();
}
void gfx_set_object_m(const MATRIX *world) {
    MATRIX om;
    CompMatrixLV(&cam.view, (MATRIX *)world, &om);
    gte_SetRotMatrix(&om);
    gte_SetTransMatrix(&om);
}
void gfx_set_identity(void) {
    gte_SetRotMatrix(&cam.view);
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
static inline int quad_offscreen(const DVECTOR *a, const DVECTOR *b, const DVECTOR *c, const DVECTOR *d) {
    int ca = outcode(a->vx, a->vy), cb = outcode(b->vx, b->vy), cc = outcode(c->vx, c->vy), cd = outcode(d->vx, d->vy);
    return (ca & cb & cc & cd) != 0;
}
static inline int tri_offscreen(const DVECTOR *a, const DVECTOR *b, const DVECTOR *c) {
    return (outcode(a->vx, a->vy) & outcode(b->vx, b->vy) & outcode(c->vx, c->vy)) != 0;
}
static int zbias;
static int fog_near = 0, fog_far = 0;   // depth fade: colours darken from fog_near to fog_far (0 = off)
void gfx_set_fog(int near, int far) { fog_near = near; fog_far = far; }

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
int gfx_prim_count;                 // taken off every primitive's depth: decals sit in front of what they are stuck on
void gfx_set_zbias(int b) { zbias = b; }
int gfx_floor_pass;                       // set while drawing the floor: it goes to the very back (nothing is ever under it), so shadows can lie on it
static inline int ot_index(int otz) {
    if (gfx_floor_pass) return OT_FLOOR;
    int i = (otz >> OT_SHIFT) - zbias;
    if (i < 1) i = 1;
    if (i >= OT_LEN) i = OT_LEN - 1;
    return i;
}

// ------------------------------------------------------------------ 3D primitives
// Vertex order: a = top-left, b = top-right, c = bottom-left, d = bottom-right as seen from OUTSIDE (clockwise on screen = front).
int gfx_quad_f(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, int r, int g, int b8, int cull) {
    int p, otz, z0, z1, z2, z3;
    POLY_F4 *q = (POLY_F4 *)gfx_alloc(sizeof(POLY_F4));
    if (!q) return 0;
    gte_ldv3((SVECTOR *)a, (SVECTOR *)b, (SVECTOR *)c);
    gte_rtpt();
    if (cull) { gte_nclip(); gte_stopz(&p); if (p <= 0) return 0; }
    gte_stsz3(&z0, &z1, &z2);
    if (z0 < Z_NEAR || z1 < Z_NEAR || z2 < Z_NEAR) return 0;
    gte_stsxy3(&q->x0, &q->x1, &q->x2);
    gte_ldv0((SVECTOR *)d);
    gte_rtps();
    gte_stsz(&z3);
    if (z3 < Z_NEAR) return 0;
    gte_stsxy(&q->x3);
    if (quad_offscreen((DVECTOR *)&q->x0, (DVECTOR *)&q->x1, (DVECTOR *)&q->x2, (DVECTOR *)&q->x3)) return 0;
    gte_avsz4();
    gte_stotz(&otz);
    setPolyF4(q);
    { int f = fog_scale(otz); u32 c_ = grade_rgb((r * f) >> 8, (g * f) >> 8, (b8 * f) >> 8); SET_C(q, 0, c_); }
    addPrim(gfx_ot() + ot_index(otz), q); gfx_prim_count++;
    return 1;
}

int gfx_tri_f(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, int r, int g, int b8, int cull) {
    int p, otz, z0, z1, z2;
    POLY_F3 *q = (POLY_F3 *)gfx_alloc(sizeof(POLY_F3));
    if (!q) return 0;
    gte_ldv3((SVECTOR *)a, (SVECTOR *)b, (SVECTOR *)c);
    gte_rtpt();
    if (cull) { gte_nclip(); gte_stopz(&p); if (p <= 0) return 0; }
    gte_stsz3(&z0, &z1, &z2);
    if (z0 < Z_NEAR || z1 < Z_NEAR || z2 < Z_NEAR) return 0;
    gte_stsxy3(&q->x0, &q->x1, &q->x2);
    if (tri_offscreen((DVECTOR *)&q->x0, (DVECTOR *)&q->x1, (DVECTOR *)&q->x2)) return 0;
    gte_avsz3();
    gte_stotz(&otz);
    setPolyF3(q);
    { int f = fog_scale(otz); u32 c_ = grade_rgb((r * f) >> 8, (g * f) >> 8, (b8 * f) >> 8); SET_C(q, 0, c_); }
    addPrim(gfx_ot() + ot_index(otz), q); gfx_prim_count++;
    return 1;
}

int gfx_quad_g(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, u32 c0, u32 c1, u32 c2, u32 c3, int cull) {
    int p, otz, z0, z1, z2, z3;
    POLY_G4 *q = (POLY_G4 *)gfx_alloc(sizeof(POLY_G4));
    if (!q) return 0;
    gte_ldv3((SVECTOR *)a, (SVECTOR *)b, (SVECTOR *)c);
    gte_rtpt();
    if (cull) { gte_nclip(); gte_stopz(&p); if (p <= 0) return 0; }
    gte_stsz3(&z0, &z1, &z2);
    if (z0 < Z_NEAR || z1 < Z_NEAR || z2 < Z_NEAR) return 0;
    gte_stsxy3(&q->x0, &q->x1, &q->x2);
    gte_ldv0((SVECTOR *)d);
    gte_rtps();
    gte_stsz(&z3);
    if (z3 < Z_NEAR) return 0;
    gte_stsxy(&q->x3);
    if (quad_offscreen((DVECTOR *)&q->x0, (DVECTOR *)&q->x1, (DVECTOR *)&q->x2, (DVECTOR *)&q->x3)) return 0;
    gte_avsz4();
    gte_stotz(&otz);
    setPolyG4(q);
    { u32 g0 = grade_rgb(c0 & 255, (c0 >> 8) & 255, (c0 >> 16) & 255), g1 = grade_rgb(c1 & 255, (c1 >> 8) & 255, (c1 >> 16) & 255),
          g2 = grade_rgb(c2 & 255, (c2 >> 8) & 255, (c2 >> 16) & 255), g3 = grade_rgb(c3 & 255, (c3 >> 8) & 255, (c3 >> 16) & 255);
      SET_C(q, 0, g0); SET_C(q, 1, g1); SET_C(q, 2, g2); SET_C(q, 3, g3); }
    addPrim(gfx_ot() + ot_index(otz), q); gfx_prim_count++;
    return 1;
}

int gfx_quad_ft(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, u16 tpage, u16 clut,
                int u0, int v0, int u1, int v1, int u2, int v2, int u3, int v3, int shade, int cull, int semi) {
    int p, otz, z0, z1, z2, z3;
    POLY_FT4 *q = (POLY_FT4 *)gfx_alloc(sizeof(POLY_FT4));
    if (!q) return 0;
    gte_ldv3((SVECTOR *)a, (SVECTOR *)b, (SVECTOR *)c);
    gte_rtpt();
    if (cull) { gte_nclip(); gte_stopz(&p); if (p <= 0) return 0; }
    gte_stsz3(&z0, &z1, &z2);
    if (z0 < Z_NEAR || z1 < Z_NEAR || z2 < Z_NEAR) return 0;
    gte_stsxy3(&q->x0, &q->x1, &q->x2);
    gte_ldv0((SVECTOR *)d);
    gte_rtps();
    gte_stsz(&z3);
    if (z3 < Z_NEAR) return 0;
    gte_stsxy(&q->x3);
    if (quad_offscreen((DVECTOR *)&q->x0, (DVECTOR *)&q->x1, (DVECTOR *)&q->x2, (DVECTOR *)&q->x3)) return 0;
    gte_avsz4();
    gte_stotz(&otz);
    setPolyFT4(q);
    shade = (shade * fog_scale(otz)) >> 8;
    { u32 c_ = grade_tex(shade); SET_C(q, 0, c_); }
    q->tpage = tpage; q->clut = clut;
    q->u0 = u0; q->v0 = v0; q->u1 = u1; q->v1 = v1; q->u2 = u2; q->v2 = v2; q->u3 = u3; q->v3 = v3;
    if (semi) setSemiTrans(q, 1);
    addPrim(gfx_ot() + ot_index(otz), q); gfx_prim_count++;
    return 1;
}

int gfx_quad_gt(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, u16 tpage, u16 clut,
                int u0, int v0, int u1, int v1, int u2, int v2, int u3, int v3, u32 c0, u32 c1, u32 c2, u32 c3, int cull) {
    int p, otz, z0, z1, z2, z3;
    POLY_GT4 *q = (POLY_GT4 *)gfx_alloc(sizeof(POLY_GT4));
    if (!q) return 0;
    gte_ldv3((SVECTOR *)a, (SVECTOR *)b, (SVECTOR *)c);
    gte_rtpt();
    if (cull) { gte_nclip(); gte_stopz(&p); if (p <= 0) return 0; }
    gte_stsz3(&z0, &z1, &z2);
    if (z0 < Z_NEAR || z1 < Z_NEAR || z2 < Z_NEAR) return 0;
    gte_stsxy3(&q->x0, &q->x1, &q->x2);
    gte_ldv0((SVECTOR *)d);
    gte_rtps();
    gte_stsz(&z3);
    if (z3 < Z_NEAR) return 0;
    gte_stsxy(&q->x3);
    if (quad_offscreen((DVECTOR *)&q->x0, (DVECTOR *)&q->x1, (DVECTOR *)&q->x2, (DVECTOR *)&q->x3)) return 0;
    gte_avsz4();
    gte_stotz(&otz);
    setPolyGT4(q);
    { int f = fog_scale(otz);
      u32 g0 = gt_grade(c0, f), g1 = gt_grade(c1, f), g2 = gt_grade(c2, f), g3 = gt_grade(c3, f);
      SET_C(q, 0, g0); SET_C(q, 1, g1); SET_C(q, 2, g2); SET_C(q, 3, g3); }
    q->tpage = tpage; q->clut = clut;
    q->u0 = u0; q->v0 = v0; q->u1 = u1; q->v1 = v1; q->u2 = u2; q->v2 = v2; q->u3 = u3; q->v3 = v3;
    addPrim(gfx_ot() + ot_index(otz), q); gfx_prim_count++;
    return 1;
}

int gfx_project(const SVECTOR *v, int *sx, int *sy, int *sz) {
    DVECTOR xy; int z;
    gte_ldv0((SVECTOR *)v);
    gte_rtps();
    gte_stsz(&z);
    gte_stsxy(&xy);
    *sx = xy.vx; *sy = xy.vy; *sz = z;
    return z >= Z_NEAR;
}

// ------------------------------------------------------------------ 2D overlay
static void ui_add(void *prim) {
    setaddr(prim, 0xffffff);
    if (ui_tail) setaddr(ui_tail, prim); else ui_head = (u32 *)prim;
    ui_tail = (u32 *)prim;
}
void gfx_rect(int x, int y, int w, int h, int r, int g, int b, int semi) {
    TILE *t = (TILE *)gfx_alloc(sizeof(TILE));
    if (!t) return;
    setTile(t);
    setXY0(t, x, y); setWH(t, w, h);
    setRGB0(t, r, g, b);
    if (semi) setSemiTrans(t, 1);
    ui_add(t);
}
void gfx_rect_c(int x, int y, int w, int h, u16 c, int semi) { gfx_rect(x, y, w, h, R8(c), G8(c), B8(c), semi); }
void gfx_line(int x0, int y0, int x1, int y1, int r, int g, int b) {
    LINE_F2 *l = (LINE_F2 *)gfx_alloc(sizeof(LINE_F2));
    if (!l) return;
    setLineF2(l);
    setXY2(l, x0, y0, x1, y1);
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
    static u16 px[32 * 32];
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
    POLY_FT4 *q = (POLY_FT4 *)gfx_alloc(sizeof(POLY_FT4)); if (!q) return;
    int otz;
    gte_ldv3((SVECTOR *)a, (SVECTOR *)b, (SVECTOR *)c); gte_rtpt();
    gte_stsxy3(&q->x0, &q->x1, &q->x2); gte_ldv0((SVECTOR *)d); gte_rtps(); gte_stsxy(&q->x3);
    gte_avsz4(); gte_stotz(&otz); if (otz < Z_NEAR) return;
    if (quad_offscreen((DVECTOR *)&q->x0, (DVECTOR *)&q->x1, (DVECTOR *)&q->x2, (DVECTOR *)&q->x3)) return;
    setPolyFT4(q); setRGB0(q, 128, 128, 128); setSemiTrans(q, 1);
    int u0 = shadow_tex.u, v0 = shadow_tex.v, u1 = u0 + 31, v1 = v0 + 31;
    q->u0 = u0; q->v0 = v0; q->u1 = u1; q->v1 = v0; q->u2 = u0; q->v2 = v1; q->u3 = u1; q->v3 = v1;
    q->tpage = shadow_tpage; q->clut = 0;
    addPrim(gfx_ot() + OT_SHADOW, q);
}
void gfx_shadow(const VECTOR *pos, int radius) {
    if (!shadow_ready) shadow_init();
    gfx_set_object(pos, 0);
    SVECTOR a, b, c, d; int r = radius + radius / 3;
    setVector(&a, -r, -2, r); setVector(&b, r, -2, r); setVector(&c, -r, -2, -r); setVector(&d, r, -2, -r);
    POLY_FT4 *q = (POLY_FT4 *)gfx_alloc(sizeof(POLY_FT4)); if (!q) return;
    int otz, p;
    gte_ldv3(&a, &b, &c); gte_rtpt();                                        // (no back-face test: it lies flat on the floor, seen from above)
    gte_stsxy3(&q->x0, &q->x1, &q->x2); gte_ldv0(&d); gte_rtps(); gte_stsxy(&q->x3);
    gte_avsz4(); gte_stotz(&otz); if (otz < Z_NEAR) return;
    (void)p;
    setPolyFT4(q); setRGB0(q, 128, 128, 128); setSemiTrans(q, 1);
    int u0 = shadow_tex.u, v0 = shadow_tex.v, u1 = u0 + 31, v1 = v0 + 31;
    q->u0 = u0; q->v0 = v0; q->u1 = u1; q->v1 = v0; q->u2 = u0; q->v2 = v1; q->u3 = u1; q->v3 = v1;
    q->tpage = shadow_tpage; q->clut = 0;
    addPrim(gfx_ot() + OT_SHADOW, q);
}
void gfx_sprite_blend(int x, int y, int w, int h, u16 tpage, u16 clut, int u, int v, int tw, int th, int r, int g, int b) {
    POLY_FT4 *q = (POLY_FT4 *)gfx_alloc(sizeof(POLY_FT4));
    if (!q) return;
    setPolyFT4(q);
    setXY4(q, x, y, x + w, y, x, y + h, x + w, y + h);
    q->u0 = u; q->v0 = v; q->u1 = u + tw - 1; q->v1 = v; q->u2 = u; q->v2 = v + th - 1; q->u3 = u + tw - 1; q->v3 = v + th - 1;
    q->tpage = tpage; q->clut = clut;
    setRGB0(q, r, g, b);
    setSemiTrans(q, 1);
    ui_add(q);
}
// the BORE font: scale 1 = s, 2 = m, 3+ = l
int gfx_text_w(int scale, const char *s) { return font_w(scale - 1, s); }
void gfx_text(int x, int y, int scale, const char *s, int r, int g, int b) { font_text(x, y, scale - 1, s, r, g, b); }
void gfx_text_old(int x, int y, int scale, const char *s, int r, int g, int b) {
    for (; *s; s++, x += 8 * scale) {
        int ch = *s;
        if (ch >= 'a' && ch <= 'z') ch -= 32;
        int i = ch - '!';
        if (i < 0 || i > 94) continue;
        POLY_FT4 *q = (POLY_FT4 *)gfx_alloc(sizeof(POLY_FT4));
        if (!q) return;
        setPolyFT4(q);
        int w = 8 * scale;
        setXY4(q, x, y, x + w, y, x, y + w, x + w, y + w);
        int u = (i % 16) * 8, v = (i / 16) * 8;
        q->u0 = u; q->v0 = v; q->u1 = u + 7; q->v1 = v; q->u2 = u; q->v2 = v + 7; q->u3 = u + 7; q->v3 = v + 7;
        q->tpage = _font_tpage; q->clut = _font_clut;
        setRGB0(q, r >> 1, g >> 1, b >> 1);   // the glyphs are white: 128 = as is
        ui_add(q);
    }
}
void gfx_textc(int cx, int y, int scale, const char *s, int r, int g, int b) { gfx_text(cx - gfx_text_w(scale, s) / 2, y, scale, s, r, g, b); }
