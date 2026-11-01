// dc_gfx.c: the PowerVR side of the Dreamcast port (KOS pvr_* API), 640x480.
#include <kos.h>
#include <dc/pvr.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <math.h>
#include <string.h>
#include "dc_gfx.h"

#include "../dsi/source/font8.h"   // the 8x8 console font (ASCII 32..126), shared with the DS port

static pvr_poly_hdr_t hdr_col, hdr_col_tr, hdr_txt;
static pvr_ptr_t font_tex;
static float z2d;                  // 2D layers: each call nearer than the last (the PVR depth is 1/z, bigger = nearer)
static uint32_t sky_t, sky_b;

typedef struct { float x, y, w, h; uint32_t argb; } Rq;
typedef struct { float x, y, s; uint16_t ch; uint32_t argb; } Gq;
static Rq trq[512]; static int ntr;     // see-through boxes, drawn in the translucent list
static Gq gq[6000]; static int ng;      // letters, drawn in the punch-through list

V3 dc_eye;
static V3 cam_r, cam_u, cam_f;          // camera axes (right, down, forward)

void dc_init(void) {
    pvr_init_params_t p = { { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16 }, 1024 * 1024, 0, 0, 0, 3 };
    pvr_init(&p);
    pvr_poly_cxt_t c;
    pvr_poly_cxt_col(&c, PVR_LIST_OP_POLY); c.gen.culling = PVR_CULLING_NONE; pvr_poly_compile(&hdr_col, &c);
    pvr_poly_cxt_col(&c, PVR_LIST_TR_POLY); c.gen.culling = PVR_CULLING_NONE; pvr_poly_compile(&hdr_col_tr, &c);
    // the font: 16 x 6 glyphs of 8x8 in a 128x64 ARGB4444 texture (a lit pixel is opaque white, the rest see-through)
    static uint16_t img[128 * 64];
    for (int g = 0; g < 95; g++) for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++)
        img[((g / 16) * 8 + y) * 128 + (g % 16) * 8 + x] = (FONT8[g][y] & (0x80 >> x)) ? 0xFFFF : 0x0000;
    font_tex = pvr_mem_malloc(128 * 64 * 2);
    pvr_txr_load_ex(img, font_tex, 128, 64, PVR_TXRLOAD_16BPP);
    pvr_poly_cxt_txr(&c, PVR_LIST_PT_POLY, PVR_TXRFMT_ARGB4444, 128, 64, font_tex, PVR_FILTER_NONE);
    c.gen.culling = PVR_CULLING_NONE; pvr_poly_compile(&hdr_txt, &c);
}

static inline uint32_t argb(uint32_t rgb, int a) { return ((uint32_t)a << 24) | (rgb & 0xffffff); }
static void quad(const pvr_poly_hdr_t *h, float x0, float y0, float x1, float y1, float z, uint32_t ca, uint32_t cb) {
    (void)h;
    pvr_vertex_t v; v.flags = PVR_CMD_VERTEX; v.z = z; v.u = v.v = 0; v.oargb = 0;
    v.x = x0; v.y = y1; v.argb = cb; pvr_prim(&v, sizeof v);
    v.x = x0; v.y = y0; v.argb = ca; pvr_prim(&v, sizeof v);
    v.x = x1; v.y = y1; v.argb = cb; pvr_prim(&v, sizeof v);
    v.flags = PVR_CMD_VERTEX_EOL; v.x = x1; v.y = y0; v.argb = ca; pvr_prim(&v, sizeof v);
}

void dc_frame_begin(uint32_t top, uint32_t bottom) {
    sky_t = top; sky_b = bottom;
    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);
    pvr_prim(&hdr_col, sizeof hdr_col);
    z2d = 10.0f; ntr = 0; ng = 0;
    quad(&hdr_col, 0, 0, DC_W, DC_H, 0.001f, argb(top, 255), argb(bottom, 255));   // the sky, behind everything
}
void dc_rect_bg(float x, float y, float w, float h, uint32_t rgb) {   // behind all the 3D, over the sky
    quad(&hdr_col, x * DC_UI, y * DC_UI, (x + w) * DC_UI, (y + h) * DC_UI, 0.002f, argb(rgb, 255), argb(rgb, 255));
}
void dc_gradient(float x, float y, float w, float h, uint32_t t, uint32_t b) {
    z2d += 0.001f;
    quad(&hdr_col, x * DC_UI, y * DC_UI, (x + w) * DC_UI, (y + h) * DC_UI, z2d, argb(t, 255), argb(b, 255));
}
void dc_rect(float x, float y, float w, float h, uint32_t rgb) {
    z2d += 0.001f;
    quad(&hdr_col, x * DC_UI, y * DC_UI, (x + w) * DC_UI, (y + h) * DC_UI, z2d, argb(rgb, 255), argb(rgb, 255));
}
void dc_rect_a(float x, float y, float w, float h, uint32_t rgb, int alpha) {
    if (ntr >= 512) return;
    z2d += 0.001f;
    trq[ntr++] = (Rq){ x * DC_UI, y * DC_UI, w * DC_UI, h * DC_UI, argb(rgb, alpha) };
    trq[ntr - 1].argb = argb(rgb, alpha);
    // the z is kept in the order of the queue
}
float dc_text_w(float scale, const char *s) { return (float)strlen(s) * 8.0f * scale; }
void dc_text(float x, float y, float scale, const char *s, uint32_t rgb) {
    for (; *s && ng < 6000; s++, x += 8.0f * scale) {
        int ch = (unsigned char)*s - 32; if (ch <= 0 || ch >= 95) continue;
        z2d += 0.001f;
        gq[ng++] = (Gq){ x * DC_UI, y * DC_UI, scale * 8.0f * DC_UI, (uint16_t)ch, argb(rgb, 255) };
        gq[ng - 1].s = scale * 8.0f * DC_UI;
        // z stored in the glyph's order
    }
}
void dc_textc(float cx, float y, float scale, const char *s, uint32_t rgb) { dc_text(cx - dc_text_w(scale, s) / 2, y, scale, s, rgb); }

void dc_frame_end(void) {
    pvr_list_finish();
    // translucent boxes
    if (ntr) {
        pvr_list_begin(PVR_LIST_TR_POLY);
        pvr_prim(&hdr_col_tr, sizeof hdr_col_tr);
        float z = 10.0f;
        for (int i = 0; i < ntr; i++) { z += 0.001f; quad(&hdr_col_tr, trq[i].x, trq[i].y, trq[i].x + trq[i].w, trq[i].y + trq[i].h, z, trq[i].argb, trq[i].argb); }
        pvr_list_finish();
    }
    // letters (punch-through)
    pvr_list_begin(PVR_LIST_PT_POLY);
    pvr_prim(&hdr_txt, sizeof hdr_txt);
    for (int i = 0; i < ng; i++) {
        float x = gq[i].x, y = gq[i].y, s = gq[i].s, z = 20.0f + i * 0.001f;
        float u0 = (gq[i].ch % 16) * 8 / 128.0f, v0 = (gq[i].ch / 16) * 8 / 64.0f, u1 = u0 + 8 / 128.0f, v1 = v0 + 8 / 64.0f;
        pvr_vertex_t v; v.flags = PVR_CMD_VERTEX; v.z = z; v.oargb = 0; v.argb = gq[i].argb;
        v.x = x; v.y = y + s; v.u = u0; v.v = v1; pvr_prim(&v, sizeof v);
        v.x = x; v.y = y; v.u = u0; v.v = v0; pvr_prim(&v, sizeof v);
        v.x = x + s; v.y = y + s; v.u = u1; v.v = v1; pvr_prim(&v, sizeof v);
        v.flags = PVR_CMD_VERTEX_EOL; v.x = x + s; v.y = y; v.u = u1; v.v = v0; pvr_prim(&v, sizeof v);
    }
    pvr_list_finish();
    pvr_scene_finish();
}

// ------------------------------------------------------------------ 3D
static V3 sub(V3 a, V3 b) { return (V3){ a.x - b.x, a.y - b.y, a.z - b.z }; }
static V3 cross(V3 a, V3 b) { return (V3){ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
static float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static V3 norm(V3 a) { float l = sqrtf(dot(a, a)); if (l < 1e-6f) return a; return (V3){ a.x / l, a.y / l, a.z / l }; }
void dc_camera(V3 eye, V3 at) {
    dc_eye = eye;
    cam_f = norm(sub(at, eye));
    V3 up = { 0, -1, 0 };
    cam_r = norm(cross(cam_f, up));          // (the PS1's look_at: right = forward x up, y down)
    cam_u = cross(cam_f, cam_r);             // down
}
V3 dc_to_view(V3 w) { V3 d = sub(w, dc_eye); return (V3){ dot(d, cam_r), dot(d, cam_u), dot(d, cam_f) }; }
int dc_proj(V3 w, float *sx, float *sy) {
    V3 v = dc_to_view(w); if (v.z < DC_NEAR) return 0;
    *sx = DC_W / 2 + v.x * DC_FOCAL / v.z; *sy = DC_H / 2 + v.y * DC_FOCAL / v.z; return 1;
}
uint32_t dc_shade(uint32_t rgb, int pct) {
    int r = ((rgb >> 16) & 255) * pct / 100, g = ((rgb >> 8) & 255) * pct / 100, b = (rgb & 255) * pct / 100;
    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
    return (r << 16) | (g << 8) | b;
}
static int mix(uint32_t a, uint32_t b, float t, uint32_t *o) { (void)a; (void)b; (void)t; (void)o; return 0; }

#define MAXN 8
void dc_poly_g(const V3 *w, const uint32_t *col, int n) {
    V3 v[MAXN + 2]; uint32_t c[MAXN + 2]; int m = 0;
    if (n > MAXN) n = MAXN;
    V3 vv[MAXN]; for (int i = 0; i < n; i++) vv[i] = dc_to_view(w[i]);
    for (int i = 0; i < n; i++) {   // clip against z = NEAR
        int j = (i + 1) % n; V3 a = vv[i], b = vv[j];
        int ia = a.z >= DC_NEAR, ib = b.z >= DC_NEAR;
        if (ia) { v[m] = a; c[m++] = col[i]; }
        if (ia != ib) {
            float t = (DC_NEAR - a.z) / (b.z - a.z);
            v[m] = (V3){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, DC_NEAR };
            uint32_t ca = col[i], cb = col[j], o = 0;
            for (int s = 0; s < 24; s += 8) { int x = (ca >> s) & 255, y = (cb >> s) & 255; o |= (uint32_t)(x + (int)((y - x) * t)) << s; }
            c[m++] = o;
        }
    }
    if (m < 3) return;
    // strip order for a convex fan: 0, 1, m-1, 2, m-2, ...
    int ord[MAXN + 2], k = 0, lo = 1, hi = m - 1; ord[k++] = 0;
    int take_lo = 1; while (lo <= hi) { ord[k++] = take_lo ? lo++ : hi--; take_lo ^= 1; }
    pvr_prim(&hdr_col, sizeof hdr_col);   // (a fresh header per polygon: a strip never continues into the next)
    for (int i = 0; i < m; i++) {
        int q = ord[i]; pvr_vertex_t pv; pv.flags = (i == m - 1) ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
        pv.x = DC_W / 2 + v[q].x * DC_FOCAL / v[q].z; pv.y = DC_H / 2 + v[q].y * DC_FOCAL / v[q].z; pv.z = 100.0f / v[q].z;   // (the PVR depth is 1/w: bigger is nearer. Scaled so the sky and the 2D layers sit clearly apart from the world)
        pv.u = pv.v = 0; pv.argb = 0xff000000u | c[q]; pv.oargb = 0; pvr_prim(&pv, sizeof pv);
    }
}
void dc_poly(const V3 *w, int n, uint32_t rgb) { uint32_t c[MAXN]; for (int i = 0; i < n && i < MAXN; i++) c[i] = rgb; dc_poly_g(w, c, n); }

void dc_box(V3 base, float hx, float hz, float h, float ys, float yc, uint32_t rgb) {
    V3 p[8]; float cx[4] = { -hx, hx, hx, -hx }, cz[4] = { -hz, -hz, hz, hz };
    for (int i = 0; i < 4; i++) {
        float rx = cx[i] * yc + cz[i] * ys, rz = -cx[i] * ys + cz[i] * yc;
        p[i] = (V3){ base.x + rx, base.y, base.z + rz };             // bottom
        p[4 + i] = (V3){ base.x + rx, base.y - h, base.z + rz };     // top
    }
    static const int side[4][4] = { { 0, 1, 5, 4 }, { 1, 2, 6, 5 }, { 2, 3, 7, 6 }, { 3, 0, 4, 7 } };
    static const int lit[4] = { 78, 92, 66, 100 };
    for (int s = 0; s < 4; s++) { V3 q[4] = { p[side[s][0]], p[side[s][1]], p[side[s][2]], p[side[s][3]] }; dc_poly(q, 4, dc_shade(rgb, lit[s])); }
    V3 t[4] = { p[4], p[5], p[6], p[7] }; dc_poly(t, 4, dc_shade(rgb, 118));
}

int dc_pad(void) {
    maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    if (!cont) return 0;
    cont_state_t *st = (cont_state_t *)maple_dev_status(cont);
    return st ? (int)st->buttons : 0;
}
