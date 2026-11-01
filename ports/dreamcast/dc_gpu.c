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
static uint32_t page_gen[2][16];          // bumped when VRAM under a 64x256 bank changes (the quick test)
static uint32_t vgen = 1;                  // bumped on any VRAM change: a texture checked since then needs no new check
static uint32_t blk_gen[32][64];          // bumped when VRAM under a 16x16 block changes (only what changed is converted again)

// ---------------------------------------------------------------- VRAM
static void vram_dirty(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    vgen++;
    if (w > 1024) w = 1024; if (h > 512) h = 512;
    int px0 = (x & 1023) >> 6, px1 = ((x + w - 1) & 1023) >> 6, py0 = (y & 511) >> 8, py1 = ((y + h - 1) & 511) >> 8;
    if (w >= 1024) { px0 = 0; px1 = 15; }
    for (int py = 0; py < 2; py++) {
        if (!(py >= py0 && py <= py1) && !(py1 < py0)) continue;
        for (int px = 0; px < 16; px++) if ((px0 <= px1) ? (px >= px0 && px <= px1) : (px >= px0 || px <= px1)) page_gen[py][px]++;
    }
    int bx0 = x >> 4, bx1 = (x + w - 1) >> 4, by0 = y >> 4, by1 = (y + h - 1) >> 4;
    for (int by = by0; by <= by1; by++) for (int bx = bx0; bx <= bx1; bx++) blk_gen[by & 31][bx & 63]++;
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
// A 256x256 page (with its CLUT for 4/8 bit) becomes an ARGB1555 PVR texture when first used. Afterwards only the 16x16-texel cells
// whose VRAM changed are converted again (the people's sprite cells change every frame; the rest of their page does not).
// Each cell also keeps two flags: it has solid texels (STP clear) / it has texels with STP set. A semi-transparent PS1 primitive
// blends only its STP texels and draws the others solid: a primitive over cells with both is drawn as two (see emit).
#define NTEX 40
typedef struct { uint32_t key, sig, clsig, used, vg; pvr_ptr_t ptr; int mode, px, py, cx, cy, var; uint32_t cg[16][16]; uint8_t fl[16][16]; } Tex;
static Tex tex[NTEX]; static uint32_t tex_clock;
static uint16_t conv[256 * 256] __attribute__((aligned(32)));
int dc_tex_uploads, dc_tex_cells;          // (whole pages / cells converted this run: the readout)
static uint32_t tex_sig(int mode, int px, int py) {
    uint32_t s = 0; int np = mode == 0 ? 1 : mode == 1 ? 2 : 4;
    for (int k = 0; k < np; k++) s += page_gen[py][(px + k) & 15] * (k + 1);
    return s;
}
static uint32_t clut_sig(int mode, int cx, int cy) {
    if (mode == 2) return 0;
    uint32_t s = blk_gen[(cy >> 4) & 31][(cx >> 4) & 63];
    if (mode == 1) for (int k = 1; k < 16; k++) s += blk_gen[(cy >> 4) & 31][((cx >> 4) + k) & 63] * (k + 1);
    return s;
}
// ---------------------------------------------------------------- the colour grade (Extras: VIVID, the default, or CLASSIC PS1)
// VIVID: colours a third more saturated and a touch brighter, with a gentle S-curve for depth - on the shading (24-bit) and on
// the textures (the PS1's own 15-bit texels, mapped through a table: no new steps). CLASSIC: the PS1's colours as they are.
#ifdef DC_CLASSIC
int dc_vivid = 0;   // (test builds: the PS1's colours)
#else
int dc_vivid = 1;
#endif
static uint16_t viv_lut[32768]; static int viv_ok;
static inline void grade8(int *r, int *g, int *b) {
    int l = (*r * 77 + *g * 150 + *b * 29) >> 8;
    int rr = l + (((*r - l) * 372) >> 8), gg = l + (((*g - l) * 372) >> 8), bb = l + (((*b - l) * 372) >> 8);   // saturation x 1.45
    rr = rr * 269 >> 8; gg = gg * 269 >> 8; bb = bb * 269 >> 8;                                                      // brightness x 1.05
    rr = rr < 0 ? 0 : rr > 255 ? 255 : rr; gg = gg < 0 ? 0 : gg > 255 ? 255 : gg; bb = bb < 0 ? 0 : bb > 255 ? 255 : bb;
    *r = rr; *g = gg; *b = bb;
}
static uint32_t viv_ck[1024], viv_cv[1024];   // a cache of graded colours (flat and shaded colours repeat a lot)
static inline uint32_t vivid(uint32_t argb) {   // 0xAARRGGBB
    if (!dc_vivid) return argb;
    uint32_t rgb = argb & 0xffffffu, h = (rgb * 2654435761u) >> 22;
    if (viv_ck[h] == (rgb | 0x1000000u)) return (argb & 0xff000000u) | viv_cv[h];
    int r = (argb >> 16) & 255, g = (argb >> 8) & 255, b = argb & 255; grade8(&r, &g, &b);
    uint32_t o = (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
    viv_ck[h] = rgb | 0x1000000u; viv_cv[h] = o;
    return (argb & 0xff000000u) | o;
}
static void viv_build(void) {
    for (int c = 0; c < 32768; c++) {
        int r = (c & 31) * 255 / 31, g = ((c >> 5) & 31) * 255 / 31, b = ((c >> 10) & 31) * 255 / 31; grade8(&r, &g, &b);
        int o = ((r * 31 + 127) / 255) | ((g * 31 + 127) / 255) << 5 | ((b * 31 + 127) / 255) << 10;
        viv_lut[c] = (uint16_t)(c && !o ? 0x0400 : o);   // (a colour never becomes 0, which is see-through)
    }
    viv_ok = 1;
}
static inline uint16_t to1555(uint16_t c, int var) {
    if (var == 1 && (c & 0x8000)) return 0;
    if (var == 2 && !(c & 0x8000)) return 0;
    if (!c) return 0;
    if (dc_vivid) c = (uint16_t)((c & 0x8000) | viv_lut[c & 0x7fff]);
    return (uint16_t)(0x8000 | ((c & 31) << 10) | (c & 0x3e0) | ((c >> 10) & 31));
}
void dc_grade_changed(void) {   // every texture converted again
    memset(viv_ck, 0, sizeof viv_ck);
    if (dc_vivid && !viv_ok) viv_build();
    vgen++;
    for (int py = 0; py < 2; py++) for (int px = 0; px < 16; px++) page_gen[py][px] += 1000;
    for (int y = 0; y < 32; y++) for (int x = 0; x < 64; x++) blk_gen[y][x]++;
}
static inline uint16_t texel(const Tex *t, int u, int v) {   // the raw PS1 texel (15 bit + STP) at page coordinates u, v
    const uint16_t *row = vram[(t->py * 256 + v) & 511]; int x0 = t->px * 64;
    if (t->mode == 2) return row[(x0 + u) & 1023];
    if (t->mode == 1) return vram[t->cy][(t->cx + ((row[(x0 + (u >> 1)) & 1023] >> ((u & 1) * 8)) & 255)) & 1023];
    return vram[t->cy][(t->cx + ((row[(x0 + (u >> 2)) & 1023] >> ((u & 3) * 4)) & 15)) & 1023];
}
static inline uint32_t cell_gen(const Tex *t, int cu, int cv) {   // the VRAM block under a texel cell
    int hx = t->px * 64 + ((cu * 16) >> (2 - t->mode));
    return blk_gen[((t->py * 256 + cv * 16) >> 4) & 31][(hx >> 4) & 63];
}
static void cell_convert(Tex *t, int cu, int cv, uint16_t *out, int stride) {   // one 16x16 cell: converted into out, flags worked out
    int f = 0, var = t->var, mode = t->mode;
    if (mode == 0) {   // 4 bit: the 16 colours of the palette are converted once, then each row is 4 words of nibbles
        uint16_t pal[16]; int seen = 0;
        for (int k = 0; k < 16; k++) pal[k] = to1555(vram[t->cy][(t->cx + k) & 1023], var);
        for (int j = 0; j < 16; j++) {
            const uint16_t *row = &vram[(t->py * 256 + cv * 16 + j) & 511][(t->px * 64 + cu * 4) & 1023]; uint16_t *o = out + j * stride;
            for (int w = 0; w < 4; w++) {
                unsigned q = row[w];
                unsigned n0 = q & 15, n1 = (q >> 4) & 15, n2 = (q >> 8) & 15, n3 = q >> 12;
                o[w * 4] = pal[n0]; o[w * 4 + 1] = pal[n1]; o[w * 4 + 2] = pal[n2]; o[w * 4 + 3] = pal[n3];
                seen |= (1 << n0) | (1 << n1) | (1 << n2) | (1 << n3);
            }
        }
        for (int k = 0; k < 16; k++) if (seen >> k & 1) { uint16_t c = vram[t->cy][(t->cx + k) & 1023]; if (c & 0x8000) f |= 2; else if (c) f |= 1; }
    } else {
        for (int j = 0; j < 16; j++) {
            const uint16_t *row = vram[(t->py * 256 + cv * 16 + j) & 511]; int x0 = t->px * 64; uint16_t *o = out + j * stride;
            for (int i = 0; i < 16; i++) {
                int u = cu * 16 + i; uint16_t c;
                if (mode == 2) c = row[(x0 + u) & 1023];
                else c = vram[t->cy][(t->cx + ((row[(x0 + (u >> 1)) & 1023] >> ((u & 1) * 8)) & 255)) & 1023];
                if (c & 0x8000) f |= 2; else if (c) f |= 1;
                o[i] = to1555(c, var);
            }
        }
    }
    t->fl[cv][cu] = (uint8_t)f; t->cg[cv][cu] = cell_gen(t, cu, cv);
}
static int tex_waited;   // (a texture is changed only once the PVR has finished drawing the last frame, which may still use it)
static inline void tex_wait(void) { if (!tex_waited) { pvr_wait_render_done(); tex_waited = 1; } }
static void tex_full(Tex *t) {
    tex_wait();
    for (int cv = 0; cv < 16; cv++) for (int cu = 0; cu < 16; cu++) cell_convert(t, cu, cv, conv + cv * 16 * 256 + cu * 16, 256);
    pvr_txr_load(conv, t->ptr, sizeof conv);
    dc_tex_uploads++;
}
static void tex_update(Tex *t) {   // only the cells whose VRAM changed
    int n = 0; static uint8_t dirty[256];
    for (int cv = 0; cv < 16; cv++) for (int cu = 0; cu < 16; cu++) if (t->cg[cv][cu] != cell_gen(t, cu, cv)) dirty[n++] = (uint8_t)(cv * 16 + cu);
    if (n > 96) { tex_full(t); return; }
    if (n) tex_wait();
    uint16_t cell[256] __attribute__((aligned(8)));
    for (int k = 0; k < n; k++) {
        int cu = dirty[k] & 15, cv = dirty[k] >> 4;
        cell_convert(t, cu, cv, cell, 16);
        volatile uint32_t *d = (volatile uint32_t *)((uint8_t *)t->ptr + (cv * 16 * 256 + cu * 16) * 2); const uint32_t *s = (const uint32_t *)cell;
        for (int j = 0; j < 16; j++, d += 128, s += 8) { d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3]; d[4] = s[4]; d[5] = s[5]; d[6] = s[6]; d[7] = s[7]; }
    }
    dc_tex_cells += n;
}
static int tex_last = -1;
static int tex_get(int tpage, int clut, int var) {
    int mode = (tpage >> 7) & 3; if (mode == 3) mode = 2;
    int px = tpage & 15, py = (tpage >> 4) & 1, cx = (clut & 63) * 16, cy = (clut >> 6) & 511;
    uint32_t key = (uint32_t)mode << 28 | (uint32_t)px << 24 | (uint32_t)py << 23 | (uint32_t)var << 21 | (mode < 2 ? (uint32_t)(clut & 0x7fff) : 0);
    tex_clock++;
    if (tex_last >= 0 && tex[tex_last].key == key && tex[tex_last].ptr && tex[tex_last].vg == vgen) { tex[tex_last].used = tex_clock; return tex_last; }
    uint32_t sig = tex_sig(mode, px, py);
    int lru = 0, i = tex_last;
    if (i >= 0 && tex[i].key == key && tex[i].ptr) goto hit;
    for (i = 0; i < NTEX; i++) {
        if (tex[i].ptr && tex[i].key == key) goto hit;
        if (!tex[i].ptr) { lru = i; break; }
        if (tex[i].used < tex[lru].used) lru = i;
    }
    if (!tex[lru].ptr) { tex[lru].ptr = pvr_mem_malloc(256 * 256 * 2); if (!tex[lru].ptr) { tex_last = -1; return -1; } }
    {   Tex *t = &tex[lru];
        t->key = key; t->sig = sig; t->clsig = clut_sig(mode, cx, cy); t->used = tex_clock;
        t->mode = mode; t->px = px; t->py = py; t->cx = cx; t->cy = cy; t->var = var;
        tex_full(t); t->vg = vgen; tex_last = lru; return lru; }
hit:
    {   Tex *t = &tex[i]; t->used = tex_clock; tex_last = i; t->vg = vgen;
        if (t->sig != sig) { t->sig = sig; uint32_t cs = clut_sig(mode, cx, cy); if (cs != t->clsig) { t->clsig = cs; tex_full(t); } else tex_update(t); }
        else if (mode < 2) { uint32_t cs = clut_sig(mode, cx, cy); if (cs != t->clsig) { t->clsig = cs; tex_full(t); } }
        return i; }
}
static int tex_flags(int ti, const int *u, const int *v, int n) {   // the cells under a primitive's texels: 1 solid ones, 2 STP ones
    if (ti < 0) return 0;
    int u0 = 255, u1 = 0, v0 = 255, v1 = 0;
    for (int k = 0; k < n; k++) { int a = u[k] & 255, b = v[k] & 255; if (a < u0) u0 = a; if (a > u1) u1 = a; if (b < v0) v0 = b; if (b > v1) v1 = b; }
    int f = 0; const Tex *t = &tex[ti];
    for (int cv = v0 >> 4; cv <= v1 >> 4; cv++) for (int cu = u0 >> 4; cu <= u1 >> 4; cu++) f |= t->fl[cv][cu];
    return f;
}

// ---------------------------------------------------------------- the primitives of a frame: PowerVR vertices in three lists
typedef struct { pvr_vertex_t *v; int n, cap, list, cur_tex, cur_blend; } VList;
#define VOP 24000
#define VPT 28000
#define VTR 28000
static pvr_vertex_t vb_op[VOP] __attribute__((aligned(32))), vb_pt[VPT] __attribute__((aligned(32))), vb_tr[VTR] __attribute__((aligned(32)));
static VList vl_op = { vb_op, 0, VOP, PVR_LIST_OP_POLY, -2, -9 }, vl_pt = { vb_pt, 0, VPT, PVR_LIST_PT_POLY, -2, -9 }, vl_tr = { vb_tr, 0, VTR, PVR_LIST_TR_POLY, -2, -9 };
static float zseq;
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

static int uv_exact;
static void emit_one(int n, const int *x, const int *y, const uint32_t *col, const int *u, const int *v, int textured, int tpage, int clut, int raw, int semi, int abr, int var);
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
        int f = tex_flags(tex_get(tpage, clut, 0), u, v, n);
        if (f == 0) return;
        if (f == 1) semi = 0;                      // (no STP texels under it: all solid)
        else if (f == 3) {                         // both: the solid texels, then the blended ones
            emit_one(n, x, y, col, u, v, 1, tpage, clut, raw, 0, abr, 1);
            emit_one(n, x, y, col, u, v, 1, tpage, clut, raw, 1, abr, 2);
            return;
        }
    }
    emit_one(n, x, y, col, u, v, textured, tpage, clut, raw, semi, abr, 0);
}
// One primitive into its list. The PS1 tints a texture by colour/128, so a lit texture can be up to twice as bright as painted; the PVR
// multiplies by at most 1. A primitive tinted above 128 is drawn at 1, then once more on top, added: the texture times the rest.
// The primitives go straight into three buffers of PowerVR vertices (opaque, punch-through, translucent), with a polygon header
// wherever the texture or the blend changes: at the end of the frame each buffer is one copy to the tile accelerator.
extern int dc_filter;
static int hdr_put(VList *L, int tex, int blend);
static void emit_one(int n, const int *x, const int *y, const uint32_t *col, const int *u, const int *v, int textured, int tpage, int clut, int raw, int semi, int abr, int var) {
    VList *L = semi ? &vl_tr : textured ? &vl_pt : &vl_op;
    if (L->n + 6 + (semi || !textured ? 0 : 5) > L->cap || vl_tr.n + 6 > vl_tr.cap) return;
    int ti = textured ? tex_get(tpage, clut, var) : -1;
    if (textured && ti < 0) return;
    zseq += 1.0f / 4096.0f;
    float z = 1.0f + zseq;
    int blend = semi ? abr : -1;
    float fx[4], fy[4], fu[4], fv[4]; uint32_t c4[4];
    uint32_t pc = 0;
    int boost = 0, a = abr == 0 ? 128 : abr == 3 ? 64 : 255;
    for (int k = 0; k < n; k++) {
        fx[k] = (float)((x[k] + ofs_x - area_x0) * 2); fy[k] = (float)((y[k] + ofs_y - area_y0) * 2);
        uint32_t c;
        if (k && col[k] == col[k - 1]) c = pc;   // (flat primitives and shared corners: graded once)
        else {
            c = pc = vivid(textured ? mod_col(col[k], raw) : flat_col(col[k]));
            if (textured && !raw && ((col[k] & 255) > 128 || ((col[k] >> 8) & 255) > 128 || ((col[k] >> 16) & 255) > 128)) boost = 1;
        }
        if (semi) c = (c & 0xffffff) | (uint32_t)a << 24;
        c4[k] = c;
        if (textured) { fu[k] = (float)u[k]; fv[k] = (float)v[k]; }
    }
    if (textured && n == 4 && !uv_exact) {   // a sprite-like quad that shows texels u0..u1 inclusive: reach to the far edge of the last texel
        float sw = fx[1] - fx[0], uw = fu[1] - fu[0], sh = fy[2] - fy[0], vh = fv[2] - fv[0];
        if (uw > 0 && sw > 0 && (int)(sw / SCALE + 0.5f) == (int)uw + 1) { fu[1] += 1; fu[3] += 1; }
        if (vh > 0 && sh > 0 && (int)(sh / SCALE + 0.5f) == (int)vh + 1) { fv[2] += 1; fv[3] += 1; }
    }
    int filt = 0;   // SMOOTH textures: the 3D world filtered; the 2D interface (rectangles, sprites one texel to a pixel) stays crisp
    if (textured && dc_filter && !uv_exact) {
        filt = 1;
        if (n == 4 && fy[0] == fy[1] && fx[0] == fx[2] && fv[0] == fv[1] && fu[0] == fu[2] && (fx[1] - fx[0]) == (fu[1] - fu[0]) * SCALE) filt = 0;
    }
    if (textured) for (int k = 0; k < n; k++) { fu[k] *= (1.0f / 256.0f); fv[k] *= (1.0f / 256.0f); }
    hdr_put(L, ti, blend + filt * 16);
    pvr_vertex_t *o = &L->v[L->n];
    for (int k = 0; k < n; k++, o++) {
        o->flags = k == n - 1 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
        o->x = fx[k]; o->y = fy[k]; o->z = z; o->u = fu[k]; o->v = fv[k]; o->argb = c4[k]; o->oargb = 0;
    }
    L->n += n;
    if (boost && (!semi || abr != 2)) {   // the part above 1: added (for a blended primitive, at its own blend's share)
        VList *T = &vl_tr;
        hdr_put(T, ti, (!semi ? 1 : abr == 0 ? 4 : abr) + filt * 16);
        o = &T->v[T->n];
        for (int k = 0; k < n; k++, o++) {
            uint32_t c = col[k]; int r = (int)(c & 255) - 128, g = (int)((c >> 8) & 255) - 128, b = (int)((c >> 16) & 255) - 128;
            r = r < 0 ? 0 : r * 2; g = g < 0 ? 0 : g * 2; b = b < 0 ? 0 : b * 2; if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            o->flags = k == n - 1 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
            o->x = fx[k]; o->y = fy[k]; o->z = z; o->u = fu[k]; o->v = fv[k]; o->oargb = 0;
            o->argb = (uint32_t)(semi ? a : 255) << 24 | (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
        }
        T->n += n;
    }
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
    uv_exact = 1;   // (a rectangle's texels are exactly u0 .. u0+w-1: no edge reach)
    emit(4, x, y, c, u, v, tex != 0, dm_tpage, clut, raw != 0, semi != 0, (dm_tpage >> 5) & 3);
    uv_exact = 0;
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
    if (dc_vivid) viv_build();
    // 24-bit colour out (the PS1 dithers its 24-bit shading down to 15 bits; here there are no steps to hide), 640x480
    vid_set_mode(DM_640x480, PM_RGB888P);
    pvr_init_params_t pp = { { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16 }, 1024 * 1024, 0, 0, 1, 3 };
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

// a polygon header for (list, texture, blend), compiled once and kept
#ifdef DC_SMOOTH
int dc_filter = 1;   // (test builds: smooth textures from the start)
#else
int dc_filter;                              // (OPTIONS > EXTRAS: 1 bilinear filtering of the 3D world's textures)
#endif
static pvr_poly_hdr_t hcache[3][NTEX + 1][12]; static uint8_t hvalid[3][NTEX + 1][12];
static int hdr_put(VList *L, int ti, int blend) {
    if (ti == L->cur_tex && blend == L->cur_blend) return 0;   // (blend: + 16 for a filtered texture)
    int filt = blend >= 16; blend &= 15;   // (16 added: this one filtered)
    int li = L->list == PVR_LIST_OP_POLY ? 0 : L->list == PVR_LIST_PT_POLY ? 1 : 2, list = L->list;
    int hk = blend + 1 + filt * 6;
    pvr_poly_hdr_t *h = &hcache[li][ti + 1][hk];
    if (!hvalid[li][ti + 1][hk]) {
        pvr_poly_cxt_t cx;
        if (ti >= 0) pvr_poly_cxt_txr(&cx, list, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED, 256, 256, tex[ti].ptr, filt ? PVR_FILTER_BILINEAR : PVR_FILTER_NONE);
        else pvr_poly_cxt_col(&cx, list);
        cx.gen.culling = PVR_CULLING_NONE;
        cx.depth.comparison = PVR_DEPTHCMP_GEQUAL;
        if (list == PVR_LIST_TR_POLY) {
            cx.gen.alpha = PVR_ALPHA_ENABLE; cx.depth.write = PVR_DEPTHWRITE_DISABLE;
            if (ti >= 0) cx.txr.env = PVR_TXRENV_MODULATEALPHA;
            switch (blend) {
                case 0: cx.blend.src = PVR_BLEND_SRCALPHA; cx.blend.dst = PVR_BLEND_INVSRCALPHA; break;   // half and half
                case 1: cx.blend.src = PVR_BLEND_SRCALPHA; cx.blend.dst = PVR_BLEND_ONE; break;          // add
                case 2: cx.blend.src = PVR_BLEND_ZERO; cx.blend.dst = PVR_BLEND_INVDESTCOLOR; break;     // subtract: back x (1 - front)
                default: cx.blend.src = PVR_BLEND_SRCALPHA; cx.blend.dst = PVR_BLEND_ONE; break;         // add a quarter (3); a brightening pass (4)
            }
        } else if (ti >= 0) cx.txr.env = PVR_TXRENV_MODULATE;
        pvr_poly_compile(h, &cx);
        hvalid[li][ti + 1][hk] = 1;
    }
    memcpy(&L->v[L->n], h, 32); L->n++;
    L->cur_tex = ti; L->cur_blend = blend + filt * 16;
    return 1;
}
static void submit_list(VList *L) {
    if (!L->n) return;
    pvr_list_begin(L->list);
    pvr_prim(L->v, (size_t)L->n * 32);
    pvr_list_finish();
}
static void lists_reset(void) {
    vl_op.n = vl_pt.n = vl_tr.n = 0;
    vl_op.cur_tex = vl_pt.cur_tex = vl_tr.cur_tex = -2; vl_op.cur_blend = vl_pt.cur_blend = vl_tr.cur_blend = -9;
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
    lists_reset(); zseq = 0;
    while (guard++ < 400000) {
        uint32_t hdrw = *p, n = hdrw >> 24, next = hdrw & 0xffffff;
        __builtin_prefetch((const void *)(0x8c000000u | next));   // (the next packet is fetched while this one is worked on)
        if (n) run_packet(p + 1, (int)n);
        if (next == 0xffffff) break;
        p = (const uint32_t *)(0x8c000000u | next);
    }
    if (!frame) return 0;
    if (off_screen && vl_op.n + vl_pt.n + vl_tr.n == 0) return 0;   // (a sprite cell's list: all of it went into VRAM)
    dc_prims_last = vl_op.n + vl_pt.n + vl_tr.n;
    pvr_wait_ready();
    { uint32_t bg = vivid(0xff000000u | (bg_rgb & 255) << 16 | (bg_rgb & 0xff00) | ((bg_rgb >> 16) & 255));
      pvr_set_bg_color(((bg >> 16) & 255) / 255.0f, ((bg >> 8) & 255) / 255.0f, (bg & 255) / 255.0f); }
    pvr_scene_begin();
    submit_list(&vl_op); submit_list(&vl_pt); submit_list(&vl_tr);
    pvr_scene_finish();
    {   // the log, every 100 frames: frames per second over them (real vblanks), the game's own CPU time a frame (hblanks, 262 = 1/60 s), the GPU side
        extern volatile int dc_vbl; extern int dc_spu_voices_on, dc_spu_peak, gfx_last_cpu; static int last_vbl, cpu_sum, cpu_max;
        cpu_sum += gfx_last_cpu; if (gfx_last_cpu > cpu_max) cpu_max = gfx_last_cpu;
        if (dc_frames % 100 == 0) {
            int dv = dc_vbl - last_vbl; last_vbl = dc_vbl;
            dc_logf("f%d fps %d.%d cpu %d max %d hb | verts %d/%d/%d pages %d cells %d pvrfree %d | snd on %d peak %d\n", dc_frames, dv ? 6000 / dv : 0, dv ? (60000 / dv) % 10 : 0,
                    cpu_sum / 100, cpu_max, vl_op.n, vl_pt.n, vl_tr.n, dc_tex_uploads, dc_tex_cells, (int)pvr_mem_available(), dc_spu_voices_on, dc_spu_peak);
            cpu_sum = cpu_max = 0;
        }
    }
    tex_waited = 0;
    { extern void dc_extra_frame(void), dc_prof_frame(int); dc_extra_frame(); dc_prof_frame(dc_frames); }
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
    // exact integer edge functions (x4, pixel centres): inside = all three >= 0; they step by a constant per pixel. The colours and
    // texture coordinates are planes: value = v2 + M0 * k0 + M1 * k1 (k = (v0-v2) / total, (v1-v2) / total)
    int sg = area > 0 ? 1 : -1, tot = 4 * area * sg;
    int d0 = 4 * (y[1] - y[2]) * sg, d1 = 4 * (y[2] - y[0]) * sg;
    float iaf = 1.0f / (float)tot;
    float kr0 = iaf * (float)((int)(c[0] & 255) - (int)(c[2] & 255)), kr1 = iaf * (float)((int)(c[1] & 255) - (int)(c[2] & 255));
    float kg0 = iaf * (float)((int)((c[0] >> 8) & 255) - (int)((c[2] >> 8) & 255)), kg1 = iaf * (float)((int)((c[1] >> 8) & 255) - (int)((c[2] >> 8) & 255));
    float kb0 = iaf * (float)((int)((c[0] >> 16) & 255) - (int)((c[2] >> 16) & 255)), kb1 = iaf * (float)((int)((c[1] >> 16) & 255) - (int)((c[2] >> 16) & 255));
    float r2 = (float)(c[2] & 255), g2 = (float)((c[2] >> 8) & 255), b2 = (float)((c[2] >> 16) & 255);
    float ku0 = 0, ku1 = 0, kv0 = 0, kv1 = 0, u2 = 0, v2 = 0;
    int mode = 0, tx0 = 0, ty0 = 0, tcx = 0, tcy = 0;
    if (textured) {
        ku0 = iaf * (float)(u[0] - u[2]); ku1 = iaf * (float)(u[1] - u[2]); kv0 = iaf * (float)(v[0] - v[2]); kv1 = iaf * (float)(v[1] - v[2]);
        u2 = (float)u[2]; v2 = (float)v[2];
        mode = (tpage >> 7) & 3; tx0 = (tpage & 15) * 64; ty0 = ((tpage >> 4) & 1) * 256; tcx = (clut & 63) * 16; tcy = (clut >> 6) & 511;
    }
    for (int py = miny; py <= maxy; py++) {
        int ey = 2 * py + 1;   // (the pixel centre, doubled)
        int m0 = sg * ((2 * x[1] - (2 * minx + 1)) * (2 * y[2] - ey) - (2 * x[2] - (2 * minx + 1)) * (2 * y[1] - ey));
        int m1 = sg * ((2 * x[2] - (2 * minx + 1)) * (2 * y[0] - ey) - (2 * x[0] - (2 * minx + 1)) * (2 * y[2] - ey));
        uint16_t *dst = &vram[py & 511][0];
        for (int px = minx; px <= maxx; px++, m0 += d0, m1 += d1) {
            if ((m0 | m1 | (tot - m0 - m1)) < 0) continue;
            float f0 = (float)m0, f1 = (float)m1;
            int r = (int)(r2 + f0 * kr0 + f1 * kr1), g = (int)(g2 + f0 * kg0 + f1 * kg1), b = (int)(b2 + f0 * kb0 + f1 * kb1);
            uint16_t out;
            if (textured) {
                int tu = (int)(u2 + f0 * ku0 + f1 * ku1) & 255, tv = (int)(v2 + f0 * kv0 + f1 * kv1) & 255;
                const uint16_t *row = vram[(ty0 + tv) & 511]; uint16_t t;
                if (mode == 0) t = vram[tcy][(tcx + ((row[(tx0 + (tu >> 2)) & 1023] >> ((tu & 3) * 4)) & 15)) & 1023];
                else if (mode == 1) t = vram[tcy][(tcx + ((row[(tx0 + (tu >> 1)) & 1023] >> ((tu & 1) * 8)) & 255)) & 1023];
                else t = row[(tx0 + tu) & 1023];
                if (!t) continue;
                if (raw) out = t;
                else {
                    int tr_ = ((t & 31) * r) >> 7, tg = (((t >> 5) & 31) * g) >> 7, tb = (((t >> 10) & 31) * b) >> 7;
                    if (tr_ > 31) tr_ = 31; if (tg > 31) tg = 31; if (tb > 31) tb = 31;
                    out = (uint16_t)(tr_ | tg << 5 | tb << 10 | (t & 0x8000));
                }
            } else out = (uint16_t)((r >> 3) | (g >> 3) << 5 | (b >> 3) << 10);
            dst[px & 1023] = mask_set ? (uint16_t)(out | 0x8000) : out;   // (E6 set-mask: the person's pixels are made solid; the cell's clear is not)
        }
    }
    vram_dirty(minx, miny, maxx - minx + 1, maxy - miny + 1);
}

// ---------------------------------------------------------------- VSync and the clocks
// VSync counts the Dreamcast's real vertical blanks (60 a second); VSync(1), the PS1's hblank count since the last VSync(0) (the
// game's own profiler), comes from the microsecond clock.
volatile int dc_vbl;
static uint64_t t0_us, vs_us;
static int vs_count;
static void vbl_irq(uint32_t code, void *data) { (void)code; (void)data; dc_vbl++; }
static int vbl_now(void) { return dc_vbl; }
int VSync(int m) {
    if (!t0_us) { t0_us = timer_us_gettime64(); vs_us = t0_us; vblank_handler_add(vbl_irq, 0); }
    if (m < 0) return vbl_now();
    if (m == 1) return (int)(((uint32_t)(timer_us_gettime64() - vs_us) * 129u) >> 13);   // hblanks (63.5 us) since the last VSync(0) (x 129 / 8192: no division)
    // the next vblank after the last VSync(0): a frame that already took longer goes on at once (the PowerVR shows each finished
    // frame at a vblank by itself, so nothing tears), instead of idling to the next vblank as the PS1 must
    static int last_vs;
    int target = last_vs + 1;
    while (vbl_now() < target) thd_pass();
    last_vs = vbl_now();
    vs_us = timer_us_gettime64(); vs_count++;
    return 0;
}
int dc_hblanks(void) { if (!t0_us) VSync(-1); return (int)((((timer_us_gettime64() - t0_us) * 129u) >> 13) & 0xffff); }
