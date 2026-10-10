// gfx: double buffering, ordering table, GTE camera and the primitive helpers every part of BORE 2 draws with.
#pragma once
#include "common.h"

#define OT_LEN     4096
#define OT_SHIFT   1              // view-space z >> OT_SHIFT = OT index (z up to 8192 units)
#define PACKET_LEN (256*1024)   // per buffer (x2 = 544 KB of the 2 MB RAM); the 80x80 maps need the rest (about 650 KB of map tables: 96x96 would need 1.1 MB and does not fit)
#define RESERVE_PEOPLE (72*1024)   // kept back from the world so the people always fit
#define RESERVE_UI     (24*1024)   // kept back from the people so bubbles, text and the HUD always fit
#define Z_NEAR     40
#define OT_FLOOR   (OT_LEN - 2)   // the floor: behind everything but the sky
#define OT_SHADOW  (OT_LEN - 3)   // shadows: just on top of the floor
extern int gfx_floor_pass;

void  gfx_init(void);
void  gfx_flip(void);
void *gfx_alloc(int bytes);
u32  *gfx_ot(void);
int   gfx_prims_left(void);
void  gfx_set_reserve(int bytes);   // gfx_alloc refuses to dip into the last `bytes` of the frame's packet buffer (0 = no reserve)

// ---- camera ----
typedef struct {
    VECTOR eye;        // where the camera is (world units)
    VECTOR at;         // what it looks at
    MATRIX view;       // world -> view, rebuilt by gfx_camera_update
} Camera;
extern Camera cam;
void gfx_camera_update(void);                 // after changing eye / at
void gfx_set_object(const VECTOR *pos, int yaw);   // object space = rotate(yaw about Y) then translate(pos); sets the GTE
void gfx_set_object_m(const MATRIX *world);        // any world matrix
void gfx_set_identity(void);                       // object space = world space

// ---- lighting (one sun) ----
int  gfx_light(int nx, int ny, int nz);       // local-space normal (units of 4096) under the current object yaw -> 0..255 (128 = unlit colour)
void gfx_set_ambient(int amb, int dif);       // the day-night cycle moves these
void gfx_set_light_yaw(int yaw);              // the yaw gfx_light rotates normals by (gfx_set_object sets it; set it yourself after gfx_set_object_m)
int  gfx_light_amb(void);
void gfx_set_zbias(int b);
void gfx_set_fog(int near, int far);          // things further than near darken towards far (0 = off)
extern int gfx_prim_count;                    // 3D primitives sent this frame (reset it yourself)                    // OT slots taken off the depth of what is drawn next (decals: faces, bangs); set it back to 0 after

// ---- 3D primitives in the current object space (return 1 if drawn) ----
int gfx_quad_f (const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, int r, int g, int b8, int cull);
int gfx_tri_f  (const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, int r, int g, int b8, int cull);
int gfx_quad_ft(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, u16 tpage, u16 clut,
                int u0, int v0, int u1, int v1, int u2, int v2, int u3, int v3, int shade, int cull, int semi);
int gfx_quad_gt(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, u16 tpage, u16 clut,
                int u0, int v0, int u1, int v1, int u2, int v2, int u3, int v3, u32 c0, u32 c1, u32 c2, u32 c3, int cull);
int gfx_quad_g (const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d, u32 c0, u32 c1, u32 c2, u32 c3, int cull);
int gfx_project(const SVECTOR *v, int *sx, int *sy, int *sz);   // one point of the current object space -> screen

// ---- 2D (drawn over everything, in call order: later calls on top) ----
void gfx_rect(int x, int y, int w, int h, int r, int g, int b, int semi);
void gfx_rect_c(int x, int y, int w, int h, u16 c15, int semi);
void gfx_line(int x0, int y0, int x1, int y1, int r, int g, int b);
void gfx_text(int x, int y, int scale, const char *s, int r, int g, int b);     // the SDK font, scaled (upper case)
int  gfx_text_w(int scale, const char *s);
void gfx_textc(int cx, int y, int scale, const char *s, int r, int g, int b);   // centred on cx
void gfx_sprite(int x, int y, int w, int h, u16 tpage, u16 clut, int u, int v, int tw, int th, int shade, int semi);   // a 2D textured quad
void gfx_sprite_blend(int x, int y, int w, int h, u16 tpage, u16 clut, int u, int v, int tw, int th, int r, int g, int b);   // semi-transparent, coloured
void gfx_set_clear(int r, int g, int b);
void gfx_sky(u32 top, u32 bottom);           // a full-screen gradient behind everything (0xBBGGRR)
void gfx_shadow(const VECTOR *pos, int radius);
void gfx_shadow_quad(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d);   // a soft shadow over a flat quad   // a soft dark blob on the floor under something
