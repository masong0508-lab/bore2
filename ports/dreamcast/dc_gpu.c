// dc_gpu.c: the PS1 GPU for the Dreamcast. The game builds its frames exactly as on the PS1 (gfx.c: GPU packets in an ordering
// table); DrawOTag here walks that table and hands every primitive to the PowerVR, in the same back-to-front order, at 640x480.
// - Each primitive gets a depth one step nearer than the one before, so the PVR's depth buffer reproduces the PS1's painter's order.
// - Textures: the PS1's VRAM (1024x512, 16 bit) is kept in RAM (LoadImage); a 256x256 texture page (with its CLUT for 4/8 bit)
//   is converted to an ARGB1555 PVR texture when first used, and again only when the VRAM under it changes.
// - Opaque untextured -> the PVR opaque list; textured -> punch-through (texel 0 is see-through, as on the PS1);
//   semi-transparent -> the translucent list, in submission order, with the PS1's four blend modes mapped to PVR blending.
// - Drawing into VRAM away from the screen (the people-as-sprites cells) is rasterised in software into the VRAM copy.
#include <kos.h>
#include <dc/pvr.h>
#include <stdint.h>
#include <string.h>
#include <psxgpu.h>

#define SCALE 2.0f
// a text log in RAM (tools/dcrun.c dumps the RAM: find "DCLOG{"), for looking at what the GPU was given
char dc_log[65536] = "DCLOG{"; static int dc_log_n = 6;
#include <stdarg.h>
void dc_logf(const char *f, ...) { if (dc_log_n > 65000) return; va_list a; va_start(a, f); dc_log_n += vsnprintf(dc_log + dc_log_n, 65530 - dc_log_n, f, a); va_end(a); }
#ifndef DC_DUMP
#define DC_DUMP -1
#endif
int dc_dump_frame = DC_DUMP;   // (a test build: -DDC_DUMP=N logs frame N's big polygons)
static uint16_t vram[512][1024] __attribute__((aligned(32)));
static uint32_t page_gen[2][16];          // bumped when VRAM under a 64x256 page changes

// ---------------------------------------------------------------- VRAM
static void vram_dirty(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    int px0 = (x & 1023) >> 6, px1 = ((x + w - 1) & 1023) >> 6, py0 = (y & 511) >> 8, py1 = ((y + h - 1) & 511) >> 8;
    if (w >= 1024) { px0 = 0; px1 = 15; }
    for (int py = 0; py < 2; py++) {
        if (!(py >= py0 && py <= py1) && !(py1 < py0)) continue;
        for (int px = 0; px < 16; px++) if ((px0 <= px1) ? (px >= px0 && px <= px1) : (px >= px0 || px <= px1)) page_gen[py][px]++;
    }
}
int LoadImage(const RECT *r, const uint32_t *data) {
    const uint16_t *s = (const uint16_t *)data;
    for (int j = 0; j < r->h; j++) for (int i = 0; i < r->w; i++) vram[(r->y + j) & 511][(r->x + i) & 1023] = *s++;
    vram_dirty(r->x, r->y, r->w, r->h);
    return 0;
}
int StoreImage(const RECT *r, uint32_t *data) {
    uint16_t *d = (uint16_t *)data;
    for (int j = 0; j < r->h; j++) for (int i = 0; i < r->w; i++) *d++ = vram[(r->y + j) & 511][(r->x + i) & 1023];
    return 0;
}
int MoveImage(const RECT *r, int x, int y) {
    for (int j = 0; j < r->h; j++) for (int i = 0; i < r->w; i++) vram[(y + j) & 511][(x + i) & 1023] = vram[(r->y + j) & 511][(r->x + i) & 1023];
    vram_dirty(x, y, r->w, r->h);
    return 0;
}
int ClearImage(const RECT *r, int rr, int gg, int bb) {
    uint16_t c = (uint16_t)((rr >> 3) | ((gg >> 3) << 5) | ((bb >> 3) << 10));
    for (int j = 0; j < r->h; j++) for (int i = 0; i < r->w; i++) vram[(r->y + j) & 511][(r->x + i) & 1023] = c;
    vram_dirty(r->x, r->y, r->w, r->h);
    return 0;
}

// ---------------------------------------------------------------- textures: PS1 pages as PVR textures
#define NTEX 36
typedef struct { uint32_t key, sig; pvr_ptr_t ptr; uint32_t used; } Tex;
static Tex tex[NTEX]; static uint32_t tex_clock;
static uint16_t conv[256 * 256] __attribute__((aligned(32)));
int dc_tex_uploads;                        // (conversions this run: the readout)
static uint32_t tex_sig(int mode, int px, int py, int cx, int cy) {
    uint32_t s = 0; int np = mode == 0 ? 1 : mode == 1 ? 2 : 4;
    for (int k = 0; k < np; k++) s += page_gen[py][(px + k) & 15] * (k + 1);
    if (mode < 2) { s += page_gen[(cy >> 8) & 1][(cx >> 6) & 15] * 7; if (mode == 1) s += page_gen[(cy >> 8) & 1][((cx + 255) >> 6) & 15] * 11; }
    return s;
}
static int tex_var;   // 0: every texel; 1: only texels without STP; 2: only texels with STP (a semi-transparent prim's two parts)
static inline uint16_t to1555(uint16_t c) {
    if (tex_var == 1 && (c & 0x8000)) return 0;
    if (tex_var == 2 && !(c & 0x8000)) return 0;
    if (!c) return 0;
    return (uint16_t)(0x8000 | ((c & 31) << 10) | (c & 0x3e0) | ((c >> 10) & 31));
}
static int tex_get(int tpage, int clut, int var) {
    tex_var = var;
    int mode = (tpage >> 7) & 3; if (mode == 3) mode = 2;
    int px = tpage & 15, py = (tpage >> 4) & 1, cx = (clut & 63) * 16, cy = (clut >> 6) & 511;
    uint32_t key = (uint32_t)mode << 28 | (uint32_t)px << 24 | (uint32_t)py << 23 | (mode < 2 ? (uint32_t)(clut & 0x7fff) : 0) | (uint32_t)var << 21;
    uint32_t sig = tex_sig(mode, px, py, cx, cy);
    tex_clock++;
    int lru = 0;
    for (int i = 0; i < NTEX; i++) {
        if (tex[i].ptr && tex[i].key == key) {
            tex[i].used = tex_clock;
            if (tex[i].sig == sig) return i;
            lru = i; goto convert;
        }
        if (!tex[i].ptr) { lru = i; break; }
        if (tex[i].used < tex[lru].used) lru = i;
    }
    if (!tex[lru].ptr) tex[lru].ptr = pvr_mem_malloc(256 * 256 * 2);
convert:
    tex[lru].key = key; tex[lru].sig = sig; tex[lru].used = tex_clock;
    {
        int x0 = px * 64, y0 = py * 256;
        const uint16_t *cl = &vram[cy][0];
        for (int v = 0; v < 256; v++) {
            const uint16_t *row = vram[(y0 + v) & 511]; uint16_t *o = conv + v * 256;
            if (mode == 2) for (int u = 0; u < 256; u++) o[u] = to1555(row[(x0 + u) & 1023]);
            else if (mode == 1) for (int u = 0; u < 256; u++) { int i = (row[(x0 + (u >> 1)) & 1023] >> ((u & 1) * 8)) & 255; o[u] = to1555(cl[(cx + i) & 1023]); }
            else for (int u = 0; u < 256; u++) { int i = (row[(x0 + (u >> 2)) & 1023] >> ((u & 3) * 4)) & 15; o[u] = to1555(cl[(cx + i) & 1023]); }
        }
        pvr_txr_load(conv, tex[lru].ptr, sizeof conv);
        dc_tex_uploads++;
    }
    return lru;
}

// ---------------------------------------------------------------- the primitives of a frame, sorted into the PVR's lists
typedef struct { float x[4], y[4], u[4], v[4]; uint32_t c[4]; float z; int8_t n, tex, blend, filt; } Prim;
#define MAXOP 14000
#define MAXPT 10000
#define MAXTR 5000
static Prim op[MAXOP], pt[MAXPT], tr[MAXTR];
static int nop, npt, ntr; static float zseq;
int dc_prims_last;

// drawing state (the GP0 E1..E6 commands and the DRAWENV)
static DRAWENV cur_env; static int env_set;
static int dm_tpage = 0x0a, ofs_x, ofs_y, off_screen, mask_set;
static int area_x0, area_y0, area_x1 = 1023, area_y1 = 511;
static uint32_t bg_rgb;

static inline int sx11(uint32_t w) { int v = (int)(w & 0x7ff); return v >= 1024 ? v - 2048 : v; }
static inline uint32_t mod_col(uint32_t c, int raw) {   // the PS1 tints a texture by c/128
    if (raw) return 0xffffffffu;
    int r = (c & 255) * 2, g = ((c >> 8) & 255) * 2, b = ((c >> 16) & 255) * 2;
    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
    return 0xff000000u | (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}
static inline uint32_t flat_col(uint32_t c) { return 0xff000000u | (c & 255) << 16 | (c & 0xff00) | ((c >> 16) & 255); }

// the software rasteriser for drawing away from the screen (sprite cells): flat / gouraud / textured triangles into the VRAM copy
static void sw_tri(const int *x, const int *y, const uint32_t *c, const int *u, const int *v, int textured, int tpage, int clut, int raw, int semi);
static inline int tri_ok(int x0, int y0, int x1, int y1, int x2, int y2) {
    int mnx = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2), mxx = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    int mny = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2), mxy = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
    return mxx - mnx < 1024 && mxy - mny < 512;
}

static void emit_one(Prim *p, int n, const int *x, const int *y, const uint32_t *col, const int *u, const int *v, int textured, int tpage, int clut, int raw, int semi, int abr, int var);
static void emit(int n, const int *x, const int *y, const uint32_t *col, const int *u, const int *v, int textured, int tpage, int clut, int raw, int semi, int abr) {
    if (n == 4 && !off_screen) {   // the PS1 GPU skips a triangle wider than 1023 or taller than 511 pixels: a quad is two of them
        int a = tri_ok(x[0], y[0], x[1], y[1], x[2], y[2]), b = tri_ok(x[1], y[1], x[2], y[2], x[3], y[3]);
        if (!a && !b) return;
        if (!a || !b) {
            int o = a ? 0 : 1, x3[3], y3[3], u3[3], v3[3]; uint32_t c3[3];
            for (int k = 0; k < 3; k++) { x3[k] = x[o + k]; y3[k] = y[o + k]; c3[k] = col[o + k]; if (textured) { u3[k] = u[o + k]; v3[k] = v[o + k]; } }
            emit(3, x3, y3, c3, u3, v3, textured, tpage, clut, raw, semi, abr); return;
        }
    } else if (n == 3 && !off_screen && !tri_ok(x[0], y[0], x[1], y[1], x[2], y[2])) return;
    if (off_screen) {   // into VRAM (people sprites): two triangles
        int xi[3], yi[3], ui[3], vi[3]; uint32_t ci[3];
        for (int t = 0; t < n - 2; t++) for (int k = 0; k < 3; k++) {
            int s = t + k; xi[k] = x[s] + ofs_x; yi[k] = y[s] + ofs_y; ci[k] = col[s]; if (textured) { ui[k] = u[s]; vi[k] = v[s]; }
            if (k == 2) sw_tri(xi, yi, ci, ui, vi, textured, tpage, clut, raw, semi);
        }
        return;
    }
    if (semi && textured) {   // the PS1 blends only the texels with the STP bit set; the rest of a semi-transparent texture is drawn solid
        if (npt < MAXPT) { Prim *q = &pt[npt]; emit_one(q, n, x, y, col, u, v, 1, tpage, clut, raw, 0, abr, 1); npt++; }
        if (ntr < MAXTR) { Prim *q = &tr[ntr]; emit_one(q, n, x, y, col, u, v, 1, tpage, clut, raw, 1, abr, 2); ntr++; }
        return;
    }
    Prim *p;
    if (semi) { if (ntr >= MAXTR) return; p = &tr[ntr++]; }
    else if (textured) { if (npt >= MAXPT) return; p = &pt[npt++]; }
    else { if (nop >= MAXOP) return; p = &op[nop++]; }
    emit_one(p, n, x, y, col, u, v, textured, tpage, clut, raw, semi, abr, 0);
}
static void emit_one(Prim *p, int n, const int *x, const int *y, const uint32_t *col, const int *u, const int *v, int textured, int tpage, int clut, int raw, int semi, int abr, int var) {
    zseq += 1.0f / 4096.0f;
    p->z = 1.0f + zseq; p->n = (int8_t)n; p->blend = (int8_t)(semi ? abr : -1); p->filt = 0;
    p->tex = textured ? (int8_t)tex_get(tpage, clut, var) : -1;
    for (int k = 0; k < n; k++) {
        p->x[k] = (x[k] + ofs_x - area_x0) * SCALE; p->y[k] = (y[k] + ofs_y - area_y0) * SCALE;
        uint32_t c = textured ? mod_col(col[k], raw) : flat_col(col[k]);
        if (semi) { int a = abr == 0 ? 128 : abr == 3 ? 64 : 255; c = (c & 0xffffff) | (uint32_t)a << 24; }
        p->c[k] = c;
        if (textured) { p->u[k] = u[k]; p->v[k] = v[k]; }
    }
    if (textured && n == 4) {   // a sprite-like quad that shows texels u0..u1 inclusive: reach to the far edge of the last texel
        float sw = p->x[1] - p->x[0], uw = p->u[1] - p->u[0], sh = p->y[2] - p->y[0], vh = p->v[2] - p->v[0];
        if (uw > 0 && sw > 0 && (int)(sw / SCALE + 0.5f) == (int)uw + 1) { p->u[1] += 1; p->u[3] += 1; }
        if (vh > 0 && sh > 0 && (int)(sh / SCALE + 0.5f) == (int)vh + 1) { p->v[2] += 1; p->v[3] += 1; }
    }
    if (textured) for (int k = 0; k < n; k++) { p->u[k] /= 256.0f; p->v[k] /= 256.0f; }
}

static void do_poly(const uint32_t *w) {
    extern int dc_frames;
    uint32_t cmd = w[0] >> 24; int quad = cmd & 8, tex = cmd & 4, gour = cmd & 16, semi = cmd & 2, raw = cmd & 1;
    int n = quad ? 4 : 3, x[4], y[4], u[4], v[4]; uint32_t c[4]; int tpage = dm_tpage, clut = 0, i = 0;
    for (int k = 0; k < n; k++) {
        c[k] = (k == 0 || !gour) ? (w[0] & 0xffffff) : (w[i] & 0xffffff);
        if (k > 0 && gour) i++; else if (k == 0) i = 1;
        x[k] = sx11(w[i]); y[k] = sx11(w[i] >> 16); i++;
        if (tex) { u[k] = w[i] & 255; v[k] = (w[i] >> 8) & 255; if (k == 0) clut = w[i] >> 16; if (k == 1) tpage = w[i] >> 16; i++; }
    }
    if (!gour) for (int k = 1; k < n; k++) c[k] = c[0];
    if (tex) dm_tpage = (dm_tpage & ~0x1ff) | (tpage & 0x1ff);   // (a textured polygon sets the draw mode's page too, as on the PS1)
    if (dc_frames == dc_dump_frame && !off_screen) {
        int mnx = 9999, mxx = -9999; for (int k = 0; k < n; k++) { if (x[k] < mnx) mnx = x[k]; if (x[k] > mxx) mxx = x[k]; }
        if (mxx - mnx > 120) dc_logf("P%02x c%06x tp%04x cl%04x (%d,%d)(%d,%d)(%d,%d)(%d,%d)\n", (int)cmd, (int)(w[0] & 0xffffff), tpage, clut, x[0], y[0], x[1], y[1], x[2], y[2], n > 3 ? x[3] : 0, n > 3 ? y[3] : 0);
    }
    emit(n, x, y, c, u, v, tex != 0, tpage, clut, raw != 0, semi != 0, (tpage >> 5) & 3);
}
static void do_rect(const uint32_t *w) {
    uint32_t cmd = w[0] >> 24; int tex = cmd & 4, semi = cmd & 2, raw = cmd & 1, sz = (cmd >> 3) & 3;
    int i = 1, x0 = sx11(w[i]), y0 = sx11(w[i] >> 16); i++;
    int u0 = 0, v0 = 0, clut = 0; if (tex) { u0 = w[i] & 255; v0 = (w[i] >> 8) & 255; clut = w[i] >> 16; i++; }
    int ww, hh; if (sz == 0) { ww = w[i] & 0x3ff; hh = (w[i] >> 16) & 0x1ff; } else { ww = hh = sz == 1 ? 1 : sz == 2 ? 8 : 16; }
    if (ww <= 0 || hh <= 0) return;
    int x[4] = { x0, x0 + ww, x0, x0 + ww }, y[4] = { y0, y0, y0 + hh, y0 + hh };
    int u[4] = { u0, u0 + ww, u0, u0 + ww }, v[4] = { v0, v0, v0 + hh, v0 + hh };
    uint32_t c[4] = { w[0] & 0xffffff, w[0] & 0xffffff, w[0] & 0xffffff, w[0] & 0xffffff };
    if (tex) { for (int k = 0; k < 4; k++) { u[k] = u[k] > 256 ? 256 : u[k]; v[k] = v[k] > 256 ? 256 : v[k]; } }
    emit(4, x, y, c, u, v, tex != 0, dm_tpage, clut, raw != 0, semi != 0, (dm_tpage >> 5) & 3);
    if (tex) { Prim *p = semi ? &tr[ntr - 1] : &pt[npt - 1]; if (!off_screen && p->n == 4) { p->u[1] = p->u[3] = (u0 + ww) / 256.0f; p->v[2] = p->v[3] = (v0 + hh) / 256.0f; } }
}
static void do_line(int x0, int y0, int x1, int y1, uint32_t c0, uint32_t c1, int semi) {
    float dx = (float)(x1 - x0), dy = (float)(y1 - y0); int hor = (dx < 0 ? -dx : dx) >= (dy < 0 ? -dy : dy);
    int x[4], y[4]; uint32_t c[4] = { c0, c0, c1, c1 };
    if (hor) { x[0] = x0; y[0] = y0; x[1] = x0; y[1] = y0 + 1; x[2] = x1 + 1; y[2] = y1; x[3] = x1 + 1; y[3] = y1 + 1; }
    else { x[0] = x0; y[0] = y0; x[1] = x0 + 1; y[1] = y0; x[2] = x1; y[2] = y1 + 1; x[3] = x1 + 1; y[3] = y1 + 1; }
    emit(4, x, y, c, 0, 0, 0, 0, 0, 0, semi, (dm_tpage >> 5) & 3);
}
static void do_fill(const uint32_t *w) {
    int x = w[1] & 0x3f0, y = (w[1] >> 16) & 0x1ff, ww = ((w[2] & 0x3ff) + 15) & ~15, hh = (w[2] >> 16) & 0x1ff;
    uint32_t c = w[0] & 0xffffff;
    if (!off_screen && x <= area_x0 && y <= area_y0 && x + ww >= area_x0 + 320 && y + hh >= area_y0 + 240) { bg_rgb = c; return; }
    uint16_t c15 = (uint16_t)(((c & 255) >> 3) | (((c >> 8) & 255) >> 3) << 5 | (((c >> 16) & 255) >> 3) << 10);
    for (int j = 0; j < hh; j++) for (int i = 0; i < ww; i++) vram[(y + j) & 511][(x + i) & 1023] = c15;
    vram_dirty(x, y, ww, hh);
}

// one packet's commands
static void run_packet(const uint32_t *w, int n) {
    int i = 0;
    while (i < n) {
        uint32_t cmd = w[i] >> 24, len = 1;
        if (cmd >= 0x20 && cmd < 0x40) {
            int quad = cmd & 8, tex = cmd & 4, gour = cmd & 16, nv = quad ? 4 : 3;
            len = 1 + nv * (1 + (tex ? 1 : 0)) + (gour ? nv - 1 : 0);
            if (i + (int)len <= n) do_poly(w + i);
        } else if (cmd >= 0x40 && cmd < 0x60) {
            int gour = cmd & 16, poly = cmd & 8, semi = cmd & 2;
            if (!poly) {
                len = gour ? 4 : 3;
                uint32_t c0 = w[i] & 0xffffff, c1 = gour ? w[i + 2] & 0xffffff : c0;
                const uint32_t *a = &w[i + 1], *b = gour ? &w[i + 3] : &w[i + 2];
                do_line(sx11(*a), sx11(*a >> 16), sx11(*b), sx11(*b >> 16), c0, c1, semi);
            } else {   // a polyline, to its terminator
                int j = i + 1; uint32_t c0 = w[i] & 0xffffff; int px = sx11(w[j]), py = sx11(w[j] >> 16); j++;
                while (j < n && (w[j] & 0xf000f000u) != 0x50005000u) {
                    uint32_t c1 = c0; if (gour) { c1 = w[j] & 0xffffff; j++; }
                    if (j >= n) break;
                    int qx = sx11(w[j]), qy = sx11(w[j] >> 16); j++;
                    do_line(px, py, qx, qy, c0, c1, semi); px = qx; py = qy; c0 = c1;
                }
                len = (uint32_t)(j - i + 1);
            }
        } else if (cmd >= 0x60 && cmd < 0x80) {
            int tex = cmd & 4, sz = (cmd >> 3) & 3;
            len = 2 + (tex ? 1 : 0) + (sz == 0 ? 1 : 0);
            if (i + (int)len <= n) do_rect(w + i);
        } else if (cmd == 0x02) { len = 3; if (i + 3 <= n) do_fill(w + i); }
        else if (cmd == 0xe1) dm_tpage = w[i] & 0x3fff;
        else if (cmd == 0xe3) {
            area_x0 = w[i] & 0x3ff; area_y0 = (w[i] >> 10) & 0x3ff;
            off_screen = !(env_set && area_x0 == cur_env.clip.x && area_y0 == cur_env.clip.y);
        } else if (cmd == 0xe4) { area_x1 = w[i] & 0x3ff; area_y1 = (w[i] >> 10) & 0x3ff; }
        else if (cmd == 0xe5) { ofs_x = sx11(w[i]) - (off_screen ? 0 : 0); ofs_y = sx11(w[i] >> 11); if (!off_screen) { ofs_x -= area_x0; ofs_y -= area_y0; ofs_x += area_x0; ofs_y += area_y0; } }
        else if (cmd == 0xe6) mask_set = w[i] & 1;
        else if (cmd == 0x80) len = 4;
        else if (cmd == 0xa0 || cmd == 0xc0) len = 3;
        i += (int)len;
    }
}

// ---------------------------------------------------------------- the GPU library calls
static int pvr_ready;
void dc_gpu_init(void) {
    if (pvr_ready) return;
    pvr_init_params_t pp = { { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16 }, 1536 * 1024, 0, 0, 1, 3 };
    pvr_init(&pp);
    pvr_ready = 1;
}
void ResetGraph(int mode) { (void)mode; dc_gpu_init(); }
DISPENV *SetDefDispEnv(DISPENV *e, int x, int y, int w, int h) { memset(e, 0, sizeof *e); e->disp.x = x; e->disp.y = y; e->disp.w = w; e->disp.h = h; return e; }
DRAWENV *SetDefDrawEnv(DRAWENV *e, int x, int y, int w, int h) {
    memset(e, 0, sizeof *e); e->clip.x = x; e->clip.y = y; e->clip.w = w; e->clip.h = h; e->tpage = 0x0a; e->dtd = 1; return e;
}
void PutDispEnv(const DISPENV *e) { (void)e; }
void PutDrawEnv(DRAWENV *e) { cur_env = *e; env_set = 1; }
void SetDispMask(int m) { (void)m; }
int DrawSync(int m) { (void)m; return 0; }
void ClearOTagR(uint32_t *ot, size_t n) {
    for (size_t i = n - 1; i > 0; i--) ot[i] = ((uint32_t)&ot[i - 1]) & 0xffffff;
    ot[0] = 0xffffff;
}
void ClearOTag(uint32_t *ot, size_t n) {
    for (size_t i = 0; i + 1 < n; i++) ot[i] = ((uint32_t)&ot[i + 1]) & 0xffffff;
    ot[n - 1] = 0xffffff;
}

static pvr_poly_hdr_t hdr;
static void submit_list(int list, Prim *ps, int np) {
    if (!np) return;
    pvr_list_begin(list);
    int cur_tex = -2, cur_blend = -9;
    for (int i = 0; i < np; i++) {
        Prim *p = &ps[i];
        if (p->tex != cur_tex || p->blend != cur_blend) {
            pvr_poly_cxt_t cx;
            if (p->tex >= 0) pvr_poly_cxt_txr(&cx, list, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED, 256, 256, tex[(int)p->tex].ptr, PVR_FILTER_NONE);
            else pvr_poly_cxt_col(&cx, list);
            cx.gen.culling = PVR_CULLING_NONE;
            cx.depth.comparison = PVR_DEPTHCMP_GEQUAL;
            if (list == PVR_LIST_TR_POLY) {
                cx.gen.alpha = PVR_ALPHA_ENABLE; cx.depth.write = PVR_DEPTHWRITE_DISABLE;
                if (p->tex >= 0) cx.txr.env = PVR_TXRENV_MODULATEALPHA;
                switch (p->blend) {
                    case 0: cx.blend.src = PVR_BLEND_SRCALPHA; cx.blend.dst = PVR_BLEND_INVSRCALPHA; break;   // half and half
                    case 1: cx.blend.src = PVR_BLEND_SRCALPHA; cx.blend.dst = PVR_BLEND_ONE; break;          // add
                    case 2: cx.blend.src = PVR_BLEND_ZERO; cx.blend.dst = PVR_BLEND_INVDESTCOLOR; break;     // subtract: back x (1 - front)
                    default: cx.blend.src = PVR_BLEND_SRCALPHA; cx.blend.dst = PVR_BLEND_ONE; break;         // add a quarter
                }
            } else if (p->tex >= 0) cx.txr.env = PVR_TXRENV_MODULATE;
            pvr_poly_compile(&hdr, &cx);
            pvr_prim(&hdr, sizeof hdr);
            cur_tex = p->tex; cur_blend = p->blend;
        }
        pvr_vertex_t v; v.oargb = 0;
        static const int ord4[4] = { 0, 1, 2, 3 };
        for (int k = 0; k < p->n; k++) {
            int q = ord4[k];
            v.flags = k == p->n - 1 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
            v.x = p->x[q]; v.y = p->y[q]; v.z = p->z; v.u = p->u[q]; v.v = p->v[q]; v.argb = p->c[q];
            pvr_prim(&v, sizeof v);
        }
    }
    pvr_list_finish();
}

int dc_frames;
int DrawOTag(const uint32_t *ot) {
    dc_gpu_init();
    // a list for the screen (the frame) or one into VRAM (a sprite cell): the frame's starts with the DRAWENV's area
    off_screen = 0; ofs_x = 0; ofs_y = 0; mask_set = 0;
    area_x0 = env_set ? cur_env.clip.x : 0; area_y0 = env_set ? cur_env.clip.y : 0;
    ofs_x = area_x0 + cur_env.ofs[0]; ofs_y = area_y0 + cur_env.ofs[1];
    int frame = 1;
    if (env_set && cur_env.isbg) bg_rgb = cur_env.r0 | cur_env.g0 << 8 | (uint32_t)cur_env.b0 << 16;
    const uint32_t *p = ot;
    int guard = 0;
    nop = npt = ntr = 0; zseq = 0;
    while (guard++ < 400000) {
        uint32_t hdrw = *p, n = hdrw >> 24, next = hdrw & 0xffffff;
        if (n) run_packet(p + 1, (int)n);
        if (next == 0xffffff) break;
        p = (const uint32_t *)(0x8c000000u | next);
    }
    if (!frame) return 0;
    if (off_screen && nop + npt + ntr == 0) return 0;   // (a sprite cell's list: all of it went into VRAM)
    dc_prims_last = nop + npt + ntr;
    pvr_wait_ready();
    pvr_set_bg_color((bg_rgb & 255) / 255.0f, ((bg_rgb >> 8) & 255) / 255.0f, ((bg_rgb >> 16) & 255) / 255.0f);
    pvr_scene_begin();
    submit_list(PVR_LIST_OP_POLY, op, nop);
    submit_list(PVR_LIST_PT_POLY, pt, npt);
    submit_list(PVR_LIST_TR_POLY, tr, ntr);
    pvr_scene_finish();
    dc_frames++;
    return 0;
}

// ---------------------------------------------------------------- the software rasteriser (sprite cells in VRAM)
static inline uint16_t tex_fetch(int tpage, int clut, int u, int v) {
    int mode = (tpage >> 7) & 3, x0 = (tpage & 15) * 64, y0 = ((tpage >> 4) & 1) * 256, cx = (clut & 63) * 16, cy = (clut >> 6) & 511;
    u &= 255; v &= 255;
    const uint16_t *row = vram[(y0 + v) & 511];
    if (mode == 0) return vram[cy][(cx + ((row[(x0 + (u >> 2)) & 1023] >> ((u & 3) * 4)) & 15)) & 1023];
    if (mode == 1) return vram[cy][(cx + ((row[(x0 + (u >> 1)) & 1023] >> ((u & 1) * 8)) & 255)) & 1023];
    return row[(x0 + u) & 1023];
}
static void sw_tri(const int *x, const int *y, const uint32_t *c, const int *u, const int *v, int textured, int tpage, int clut, int raw, int semi) {
    (void)semi;
    int minx = x[0], maxx = x[0], miny = y[0], maxy = y[0];
    for (int k = 1; k < 3; k++) { if (x[k] < minx) minx = x[k]; if (x[k] > maxx) maxx = x[k]; if (y[k] < miny) miny = y[k]; if (y[k] > maxy) maxy = y[k]; }
    if (minx < area_x0) minx = area_x0; if (miny < area_y0) miny = area_y0; if (maxx > area_x1) maxx = area_x1; if (maxy > area_y1) maxy = area_y1;
    if (minx > maxx || miny > maxy) return;
    int area = (x[1] - x[0]) * (y[2] - y[0]) - (x[2] - x[0]) * (y[1] - y[0]);
    if (!area) return;
    float ia = 1.0f / (float)area;
    for (int py = miny; py <= maxy; py++) for (int px = minx; px <= maxx; px++) {
        float fx = px + 0.5f, fy = py + 0.5f;
        float w0 = ((x[1] - fx) * (y[2] - fy) - (x[2] - fx) * (y[1] - fy)) * ia;
        float w1 = ((x[2] - fx) * (y[0] - fy) - (x[0] - fx) * (y[2] - fy)) * ia;
        float w2 = 1.0f - w0 - w1;
        if (w0 < 0 || w1 < 0 || w2 < 0) continue;
        int r = (int)(w0 * (c[0] & 255) + w1 * (c[1] & 255) + w2 * (c[2] & 255));
        int g = (int)(w0 * ((c[0] >> 8) & 255) + w1 * ((c[1] >> 8) & 255) + w2 * ((c[2] >> 8) & 255));
        int b = (int)(w0 * ((c[0] >> 16) & 255) + w1 * ((c[1] >> 16) & 255) + w2 * ((c[2] >> 16) & 255));
        uint16_t out;
        if (textured) {
            int tu = (int)(w0 * u[0] + w1 * u[1] + w2 * u[2]), tv = (int)(w0 * v[0] + w1 * v[1] + w2 * v[2]);
            uint16_t t = tex_fetch(tpage, clut, tu, tv); if (!t) continue;
            if (raw) out = t;
            else {
                int tr_ = ((t & 31) * r) >> 7, tg = (((t >> 5) & 31) * g) >> 7, tb = (((t >> 10) & 31) * b) >> 7;
                if (tr_ > 31) tr_ = 31; if (tg > 31) tg = 31; if (tb > 31) tb = 31;
                out = (uint16_t)(tr_ | tg << 5 | tb << 10 | (t & 0x8000));
            }
        } else out = (uint16_t)((r >> 3) | (g >> 3) << 5 | (b >> 3) << 10);
        vram[py & 511][px & 1023] = mask_set ? (uint16_t)(out | 0x8000) : out;   // (E6 set-mask: the person's pixels are made solid; the cell's clear is not)
    }
    vram_dirty(minx, miny, maxx - minx + 1, maxy - miny + 1);
}

// ---------------------------------------------------------------- VSync and the clocks
static uint64_t t0_us, vs_us;
static int vs_count;
static int vbl_now(void) { return (int)((timer_us_gettime64() - t0_us) * 60 / 1000000); }
int VSync(int m) {
    if (!t0_us) { t0_us = timer_us_gettime64(); vs_us = t0_us; }
    if (m < 0) return vbl_now();
    if (m == 1) return (int)((timer_us_gettime64() - vs_us) * 10 / 635);   // hblanks (63.5 us) since the last VSync(0)
    int target = vbl_now() + 1;
    while (vbl_now() < target) thd_pass();
    vs_us = timer_us_gettime64(); vs_count++;
    return 0;
}
int dc_hblanks(void) { if (!t0_us) VSync(-1); return (int)(((timer_us_gettime64() - t0_us) * 10 / 635) & 0xffff); }
