// dc_gfx: the Dreamcast platform layer's drawing (PowerVR through KOS' pvr API) at 640x480.
// The game's own screen is 320x240 (the PS1's): 2D calls here take those coordinates and double them, so the title and the
// menus keep their layout. 3D takes view-space polygons (x right, y DOWN, z forward, in the game's world units) like the GTE.
#pragma once
#include <stdint.h>

#define DC_W 640
#define DC_H 480
#define DC_UI 2                      // 2D scale: game pixels -> screen pixels
#define DC_FOCAL 460.0f              // the PS1's GeomScreen 230 at 320 across, doubled for 640
#define DC_NEAR 40.0f                // the PS1's Z_NEAR

void dc_init(void);
void dc_frame_begin(uint32_t sky_top, uint32_t sky_bottom);   // colours 0xRRGGBB
void dc_frame_end(void);

// ---- 2D, in 320x240 game pixels, drawn over the 3D in call order ----
void dc_rect(float x, float y, float w, float h, uint32_t rgb);
void dc_rect_bg(float x, float y, float w, float h, uint32_t rgb);   // behind the 3D (over the sky)
void dc_rect_a(float x, float y, float w, float h, uint32_t rgb, int alpha);   // see-through (alpha 0..255)
void dc_gradient(float x, float y, float w, float h, uint32_t top, uint32_t bottom);
void dc_text(float x, float y, float scale, const char *s, uint32_t rgb);       // scale 1 = 8 game pixels a letter
float dc_text_w(float scale, const char *s);
void dc_textc(float cx, float y, float scale, const char *s, uint32_t rgb);

// ---- 3D ----
typedef struct { float x, y, z; } V3;
extern V3 dc_eye;                    // the camera (world units, y DOWN as in the PS1 game)
void dc_camera(V3 eye, V3 at);       // look at (up is -y)
V3   dc_to_view(V3 world);
// a convex polygon (3..8 corners) in world space, flat colour 0xRRGGBB, clipped to the near plane; both sides drawn
void dc_poly(const V3 *w, int n, uint32_t rgb);
// shaded: a colour per corner
void dc_poly_g(const V3 *w, const uint32_t *rgb, int n);
// an upright box: centre of the bottom face, half sizes (x, z), height (up = -y), lit by the sun; sides slightly shaded
void dc_box(V3 base, float hx, float hz, float h, float yaw_sin, float yaw_cos, uint32_t rgb);
uint32_t dc_shade(uint32_t rgb, int pct);   // scale a colour (100 = same)
int  dc_proj(V3 world, float *sx, float *sy);   // world -> screen pixels; 0 if behind the camera
int  dc_pad(void);                   // CONT_* buttons held on pad 1
void dc_screenshot_hint(void);
