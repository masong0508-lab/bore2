#include "atlas.h"
#include <hwregs_c.h>

// Copy pixels into VRAM with exactly the right number of words.
// The SDK's LoadImage rounds the DMA up to whole 16-word blocks, so a texture whose size is not a multiple of 32 pixels
// sends a few extra words. The GPU reads those as drawing commands: PCSX ignores them, but a real PlayStation and DuckStation
// do not, and a stray "copy to VRAM" command makes the GPU wait for data forever (a black screen at boot).
static void vram_load(const RECT *r, const void *data) {
    DrawSync(0);
    int words = (r->w * r->h) >> 1;
    while (!(GPU_GP1 & (1 << 26))) { }
    GPU_GP1 = 0x04000000;                                   // DMA off while we set up
    GPU_GP0 = 0x01000000;                                   // flush the texture cache
    GPU_GP0 = 0xa0000000;                                   // copy CPU -> VRAM
    GPU_GP0 = (u16)r->x | ((u32)(u16)r->y << 16);
    GPU_GP0 = (u16)r->w | ((u32)(u16)r->h << 16);
    GPU_GP1 = 0x04000002;                                   // DMA request: GP0
    const u32 *p = (const u32 *)data;
    int big = words & ~15, tail = words & 15;
    if (big) {
        while ((DMA_CHCR(2) & (1 << 24)) || !(GPU_GP1 & (1 << 28))) { }
        DMA_MADR(2) = (u32)p;
        DMA_BCR(2) = 16 | ((big >> 4) << 16);
        DMA_CHCR(2) = 0x01000201;
        while (DMA_CHCR(2) & (1 << 24)) { }
        p += big;
    }
    if (tail) {
        while ((DMA_CHCR(2) & (1 << 24)) || !(GPU_GP1 & (1 << 28))) { }
        DMA_MADR(2) = (u32)p;
        DMA_BCR(2) = tail | (1 << 16);
        DMA_CHCR(2) = 0x01000201;
        while (DMA_CHCR(2) & (1 << 24)) { }
    }
    while (!(GPU_GP1 & (1 << 26))) { }                     // the GPU has taken it all
    GPU_GP1 = 0x04000000;
}

#define PAGES 4
#define PAGE_X 640   // pages 0 and 1 are 256 wide at x=640; page 2 is the 128-wide strip at x=896
static struct { int x, y, rowh; u16 tpage; int base_y; } pg[PAGES];
static int used; int atlas_fails, atlas_fail_wh;   // (the last texture that did not fit: w, h, the first page tried)
static u16 upload_buf[128 * 128];   // the largest texture we upload at once (wallpapers are 8x24, items up to 24x24)

void atlas_init(void) {
    for (int i = 0; i < PAGES; i++) {
        pg[i].x = 0; pg[i].y = 0; pg[i].rowh = 0;                  // (the strip has its top corner too: the SDK debug font is not loaded)
        pg[i].base_y = (i & 1) ? 256 : 0;
        pg[i].tpage = getTPage(2, 0, i < 2 ? PAGE_X : 896, (i & 1) ? 256 : 0);
    }
    used = 0;
}

static TexRef alloc_in(int first, int last, int w, int h, int count) {
    TexRef t = { 0, 0, 0, 0, 0 };
    int aw = (w + 1) & ~1;              // even widths: the transfer is in words
    for (int i = first; i <= last; i++) {
        int pw = i < 2 ? 256 : 128, ph = 256;
        if (w > pw) continue;
        if (pg[i].x + aw + 1 > pw) { pg[i].x = 0; pg[i].y += pg[i].rowh + 1; pg[i].rowh = 0; }
        if (pg[i].y + h + 1 > ph) continue;
        t.tpage = pg[i].tpage; t.u = (u8)pg[i].x; t.v = (u8)pg[i].y; t.w = (u8)w; t.h = (u8)h; t.cap = (u8)w;
        pg[i].x += aw + 1;
        if (h > pg[i].rowh) pg[i].rowh = h;
        used += aw * h;
        return t;
    }
    if (count) { atlas_fail_wh = (w << 16) | (h << 4) | first; atlas_fails++; }   // (no printf here: a texture that does not fit is simply not drawn, and the console is slow)
    return t;
}
TexRef atlas_alloc(int w, int h) { return alloc_in(0, 1, w, h, 1); }
TexRef atlas_alloc_page(int page, int w, int h) { return alloc_in(page, page == 2 ? 3 : page, w, h, 1); }
TexRef atlas_try(int w, int h) { TexRef t = alloc_in(2, 3, w, h, 0); return t.tpage ? t : alloc_in(0, 1, w, h, 0); }   // the strip, else anywhere; none (no fuss) when full

void atlas_upload(const TexRef *t, const u16 *pixels) {
    if (!t->tpage) return;
    int page = 0; for (int i = 0; i < PAGES; i++) if (t->tpage == pg[i].tpage) page = i;
    int aw = (t->w + 1) & ~1;
    for (int y = 0; y < t->h; y++) {
        for (int x = 0; x < aw; x++) upload_buf[y * aw + x] = (x < t->w) ? pixels[y * t->w + x] : 0;
    }
    RECT r; setRECT(&r, (page < 2 ? PAGE_X : 896) + t->u, pg[page].base_y + t->v, aw, t->h);
    vram_load(&r, upload_buf);
}

u16 *atlas_scratch(void) { return upload_buf; }   // 128x128 pixels of scratch (free between uploads)
void atlas_vram_load(int x, int y, int w, int h, const u16 *pixels) { RECT r; setRECT(&r, x, y, w, h); vram_load(&r, pixels); }
TexRef atlas_add_aligned(int w, int h, int a, const u16 *pixels) {   // u and v multiples of a (a texture window can repeat it)
    TexRef t = { 0, 0, 0, 0, 0 };
    for (int i = 0; i < PAGES; i++) {
        int pw = i < 2 ? 256 : 128;
        int x = (pg[i].x + a - 1) / a * a, y = pg[i].y;
        if (y % a || x + w > pw) { y = (pg[i].y + pg[i].rowh + 1 + a - 1) / a * a; x = 0; if (y + h > 256) continue; pg[i].y = y; pg[i].rowh = 0; }
        t.tpage = pg[i].tpage; t.u = (u8)x; t.v = (u8)y; t.w = (u8)w; t.h = (u8)h; t.cap = (u8)w;
        pg[i].x = x + ((w + 1) & ~1) + 1; if (h > pg[i].rowh) pg[i].rowh = h;
        used += ((w + 1) & ~1) * h;
        atlas_upload(&t, pixels);
        return t;
    }
    atlas_fail_wh = (w << 16) | (h << 4) | 9; atlas_fails++; return t;
}
TexRef atlas_add(int w, int h, const u16 *pixels) {
    TexRef t = atlas_alloc(w, h);
    atlas_upload(&t, pixels);
    return t;
}

TexRef atlas_solid(u16 c) {
    u16 px[4] = { c, c, c, c };
    if (!c) px[0] = px[1] = px[2] = px[3] = 0x8000;   // black stays drawn
    return atlas_add(2, 2, px);
}

int atlas_used(void) { return used; }
