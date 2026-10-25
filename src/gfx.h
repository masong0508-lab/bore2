// gfx: double buffering, ordering table, GTE camera and the primitive helpers every part of BORE 2 draws with.
#pragma once
#include "common.h"

#define OT_NEAR    4096           // the near table: view-space z >> OT_SHIFT (z up to 8192 units)
#define OT_LEN     (OT_NEAR + 256) // then the far world (far.c), back to front, and the ground and the sky behind it all
#define OT_SKY        (OT_LEN - 1)
#define OT_GROUND_FAR (OT_LEN - 2)
#define OT_GROUND_WIN (OT_LEN - 3)
#define OT_FAR_LAST   (OT_LEN - 4)
#define OT_SHIFT   1              // view-space z >> OT_SHIFT = OT index (z up to 8192 units)
#define PACKET_LEN (78*1024)    // per buffer. The busiest frames measured use under 54 KB: a party (people and the HUD 31 KB), a low camera turning to the horizon (the world 35 KB)
#define RESERVE_PEOPLE (28*1024)   // kept back from the world so the people always fit (a party peaks near 35 KB with the HUD)
#define RESERVE_UI     (12*1024)   // kept back from the people so bubbles, text and the HUD always fit
#define Z_NEAR     40
#define OT_FLOOR   (OT_NEAR - 2)  // the floor: behind everything near (the far world and the sky are further still)
#define OT_SHADOW  (OT_NEAR - 3)  // shadows: just on top of the floor
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
extern int gfx_prim_count;                    // 3D primitives sent (reset it yourself; the Bores' cost)
extern int gfx_last_prims, gfx_last_bytes, gfx_last_overflow;
extern int gfx_prof[8], gfx_prof_last[8]; int gfx_hb(void); int isqrt(int v);   // profiler buckets: 0 floors 1 walls 2 items 3 roofs 4 sims 5 cit full 6 cit crowd 7 shadows/misc
extern int gfx_frame_prims, gfx_profn[8], gfx_profn_last[8];
#define PROF_BEGIN() int prof_t0_ = gfx_hb(), prof_n0_ = gfx_frame_prims
#define PROF_END(k) (gfx_prof[k] += gfx_hb() - prof_t0_, gfx_profn[k] += gfx_frame_prims - prof_n0_)   // profiler (L3 in life mode): last frame's 3D prims, packet bytes, prims that did not fit

// ---- the fast path: transform shared corners once (XV), then emit faces from them; colours already graded ----
int  gfx_visible(int wx, int wy, int wz, int r);   // a world-space sphere inside the view?
#define SVEC_ALIGN __attribute__((aligned(8)))   // SVECTOR arrays the GTE loads (lwc2 needs word alignment)
typedef struct { short x, y; int z; } XV;
// people as sprites (render-to-texture, see gfx.c)
extern int gfx_last_cpu;   // the last frame's CPU time, in hblanks (up to the flip)
extern int gfx_bb_on, gfx_bb[4];   // (sprite renders) the screen box of all that was projected while on: x0, y0, x1, y1
int  gfx_imp_begin(int cx, int cy, int cw, int ch, int ox, int oy, int zmid, int hscale);   // then draw the person, then:
void gfx_imp_end(void);
void gfx_imp_sprite(int cx, int cy, int tw, int th, int x, int y, int w, int h, int otz, int dx);
int  gfx_proj(int wx, int wy, int wz, int *sx, int *sy);   // screen position; returns the depth (0 behind the eye)     // a transformed vertex: screen x, y and view depth
void gfx_xform(const SVECTOR *v, XV *o, int n);
u32  gfx_grade_flat(u32 rgb);                 // a lit colour (0xBBGGRR) as the flat/gouraud prims show it
u32  gfx_grade_tex(u32 rgb);                  // ... as the textured prims show it (128 = the texture as it is)

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
void gfx_grad(int x, int y, int w, int h, u32 c0, u32 c1, u32 c2, u32 c3, int semi);   // a 2D box shaded corner to corner (TL TR BL BR)
void gfx_ui_blend(int abr);                  // the 2D overlay's see-through mix from here on (0 half, 1 add, 2 subtract, 3 quarter)
void gfx_line(int x0, int y0, int x1, int y1, int r, int g, int b);
extern int ui_light;   // (ui.c) text onto a white card: light colours come out dark
void gfx_text(int x, int y, int scale, const char *s, int r, int g, int b);
void gfx_text_raw(int x, int y, int scale, const char *s, int r, int g, int b);   // (no white-card mapping: the colour as given)     // the SDK font, scaled (upper case)
int  gfx_text_w(int scale, const char *s);
void gfx_textc(int cx, int y, int scale, const char *s, int r, int g, int b);   // centred on cx
void gfx_sprite(int x, int y, int w, int h, u16 tpage, u16 clut, int u, int v, int tw, int th, int shade, int semi);   // a 2D textured quad
void gfx_sprite_c(int x, int y, int w, int h, u16 tpage, int u, int v, int tw, int th, int r, int g, int b);   // tinted (128 = as painted)
void gfx_sprite_blend(int x, int y, int w, int h, u16 tpage, u16 clut, int u, int v, int tw, int th, int r, int g, int b);   // semi-transparent, coloured
void gfx_set_clear(int r, int g, int b);
extern int gfx_noshadow, gfx_min_vsyncs;   // (options: no soft shadows; the fewest vblanks a frame takes)
void gfx_set_dither(int on);
void gfx_sky(u32 top, u32 bottom);           // a full-screen gradient behind everything (0xBBGGRR)
void gfx_shadow(const VECTOR *pos, int radius);
void gfx_shadow_quad(const SVECTOR *a, const SVECTOR *b, const SVECTOR *c, const SVECTOR *d);   // a soft shadow over a flat quad   // a soft dark blob on the floor under something
#include "gfx_fast.h"
