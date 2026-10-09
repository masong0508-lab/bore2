#include "atlas.h"

#define PAGES 4
#define PAGE_X 640   // pages 0 and 1 are 256 wide at x=640; page 2 is the 128-wide strip at x=896
static struct { int x, y, rowh; u16 tpage; int base_y; } pg[PAGES];
static int used;
static u16 upload_buf[128 * 128];   // the largest texture we upload at once (wallpapers are 8x24, items up to 24x24)

void atlas_init(void) {
    for (int i = 0; i < PAGES; i++) {
        pg[i].x = 0; pg[i].y = i == 2 ? 64 : 0; pg[i].rowh = 0;    // the strip starts under the debug font at (960, 0..63)
        pg[i].base_y = (i & 1) ? 256 : 0;
        pg[i].tpage = getTPage(2, 0, i < 2 ? PAGE_X : 896, (i & 1) ? 256 : 0);
    }
    used = 0;
}

static TexRef alloc_in(int first, int last, int w, int h) {
    TexRef t = { 0, 0, 0, 0, 0 };
    int aw = (w + 1) & ~1;              // even widths: the transfer is in words
    for (int i = first; i <= last; i++) {
        int pw = i < 2 ? 256 : 128, ph = 256;
        if (w > pw) continue;
        if (pg[i].x + aw + 1 > pw) { pg[i].x = 0; pg[i].y += pg[i].rowh + 1; pg[i].rowh = 0; }
        if (pg[i].y + h + 1 > ph) continue;
        t.tpage = pg[i].tpage; t.u = (u8)pg[i].x; t.v = (u8)pg[i].y; t.w = (u8)w; t.h = (u8)h;
        pg[i].x += aw + 1;
        if (h > pg[i].rowh) pg[i].rowh = h;
        used += aw * h;
        return t;
    }
    printf("atlas full (%dx%d)\n", w, h);
    return t;
}
TexRef atlas_alloc(int w, int h) { return alloc_in(0, 1, w, h); }
TexRef atlas_alloc_page(int page, int w, int h) { return alloc_in(page, page == 2 ? 3 : page, w, h); }

void atlas_upload(const TexRef *t, const u16 *pixels) {
    if (!t->tpage) return;
    int page = 0; for (int i = 0; i < PAGES; i++) if (t->tpage == pg[i].tpage) page = i;
    int aw = (t->w + 1) & ~1;
    for (int y = 0; y < t->h; y++) {
        for (int x = 0; x < aw; x++) upload_buf[y * aw + x] = (x < t->w) ? pixels[y * t->w + x] : 0;
    }
    RECT r; setRECT(&r, (page < 2 ? PAGE_X : 896) + t->u, pg[page].base_y + t->v, aw, t->h);
    LoadImage(&r, (u32 *)upload_buf);
    DrawSync(0);
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
