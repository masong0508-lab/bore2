// crowd: see crowd.h
#include "crowd.h"
#include "gfx.h"
#include "atlas.h"
#include "mem8.h"

#define ARENA_BYTES 180672   // (the people's models; it used to be the voice clips' memory, which are on the disc now)
static u8 arena_mem[ARENA_BYTES] __attribute__((aligned(4)));

// ---- the compact model ----------------------------------------------------------------------------------------------
#define NW 26                                   // the s16 fields of a Bore from w_headx to w_rgx (they sit together)
typedef struct {
    u16 bytes; u8 ncell, ndec;
    u8  stage, legs, torso, headsz, bx0, bxw, bxh, bxd;
    u8  face, eye_w, mouth_w, pad;
    u16 col[9]; u16 pad2;
    s16 w[NW];
    u8  decc[8]; u16 deccode[8];
    u8  look[LK_COUNT];
    u8  vox[H][D][W];
} CHead;
typedef struct { u8 xyz, v, hf, grp; s8 d[24]; } CCell;   // x | y << 3 | z << 6; hide | flags << 6; the 8 corners' offsets from the grid

static u8 *ar; static u32 ar_size, ar_top;      // the arena (the voice blob, once it is in the sound chip)
static s32 m_off[CROWD_MAX];                    // each person's model in the arena (-1: none)
static u16 m_used[CROWD_MAX];                   // the frame it was last drawn (for choosing what to let go)
static u16 fr = 1;                              // the frame stamp
int crowd_renders, crowd_bytes;

// ---- face texture slots (each stored person keeps their own baked eyes and mouth in VRAM) ----
#define NFACE 100
static TexRef fe[NFACE], fm[NFACE];
static s16 face_owner[NFACE];
static u8 face_of[CROWD_MAX];

// ---- sprites --------------------------------------------------------------------------------------------------------
// Each sprite gets a cell its own size (the person at 1:1, as they stand on the screen now) in the free VRAM under the
// screens (x 0..639, y 256..511). Cells sit on shelves (rows), a shelf as tall as its tallest cell needs; a shelf left empty
// is free rows again. When it is all full, the people not on the screen give their cells up, then the far ones get a smaller
// cell (shown a little scaled up) rather than none.
#define VX1 640
#define VY0 256
#define VY1 512
#define CELL_HMAX 136                           // taller than this on the screen: rendered smaller (or drawn live instead)
#define CELL_WMAX 160                           // (a 15-bit texture page reaches 255 texels from a 64-aligned base)
#define NSHELF 40
typedef struct { s16 y, h; } Shelf;
static Shelf sh[NSHELF]; static int nsh;        // in order down the VRAM, from VY0
typedef struct {
    s16 rx, ry, rw, rh;                         // the rendered box, from the anchor (the feet) at depth z, in screen pixels
    s16 mx0, my0, mx1, my1;                     // the person's measured box at that render, from the anchor
    s16 cx, cy; u8 cw, ch;                      // the cell
    u8 tw, th;                                  // the texels used in it
    s8 shelf; u8 ok, clip, pose, meas;
    s32 z; u16 ang, frame, drawn;
} Spr;
#define SPR_MAX (CROWD_MAX + 8)                  // (the last 8: the household, as sprites when they are further off)
static Spr spr[SPR_MAX];

static void cell_free(int id) { spr[id].shelf = -1; spr[id].ok = 0; }
static int shelf_n(int k) { int n = 0; for (int i = 0; i < SPR_MAX; i++) n += spr[i].shelf == k; return n; }
COLD static int shelf_gap(int k, int w) {            // the x of the first gap w wide on shelf k (-1: none)
    s16 xs[64], ws[64]; int n = 0;
    for (int i = 0; i < CROWD_MAX && n < 64; i++) if (spr[i].shelf == k) {
        int j = n++; while (j > 0 && xs[j - 1] > spr[i].cx) { xs[j] = xs[j - 1]; ws[j] = ws[j - 1]; j--; }
        xs[j] = spr[i].cx; ws[j] = spr[i].cw;
    }
    int x = 0;
    for (int j = 0; j < n; j++) { if (xs[j] - x >= w) return x; x = xs[j] + ws[j]; }
    return VX1 - x >= w ? x : -1;
}
static void shelf_insert(int at, int y, int h) {
    if (nsh >= NSHELF) return;
    for (int k = nsh; k > at; k--) sh[k] = sh[k - 1];
    for (int i = 0; i < SPR_MAX; i++) if (spr[i].shelf >= at) spr[i].shelf++;
    sh[at].y = (s16)y; sh[at].h = (s16)h; nsh++;
}
static void shelf_remove(int at) {
    for (int k = at; k < nsh - 1; k++) sh[k] = sh[k + 1];
    for (int i = 0; i < SPR_MAX; i++) if (spr[i].shelf > at) spr[i].shelf--;
    nsh--;
}
static void shelves_tidy(void) {                // empty shelves next to each other become one; an empty last one goes
    for (int k = 0; k + 1 < nsh; ) {
        if (!shelf_n(k) && !shelf_n(k + 1)) { sh[k].h += sh[k + 1].h; shelf_remove(k + 1); } else k++;
    }
    while (nsh && !shelf_n(nsh - 1)) nsh--;
}
static int new_shelf(int h) {                   // a shelf h tall: the best-fitting empty one (cut to size), else a new one at the end
    int best = -1;
    for (int k = 0; k < nsh; k++) if (sh[k].h >= h && !shelf_n(k) && (best < 0 || sh[k].h < sh[best].h)) best = k;
    if (best >= 0) {
        if (sh[best].h - h >= 16 && nsh < NSHELF) { int y = sh[best].y + h, r = sh[best].h - h; sh[best].h = (s16)h; shelf_insert(best + 1, y, r); }
        return best;
    }
    int top = nsh ? sh[nsh - 1].y + sh[nsh - 1].h : VY0;
    if (top + h > VY1 || nsh >= NSHELF) return -1;
    sh[nsh].y = (s16)top; sh[nsh].h = (s16)h; return nsh++;
}
static int place(int id, int w, int h, int k) {
    int x = shelf_gap(k, w); if (x < 0) return 0;
    Spr *s = &spr[id]; s->shelf = (s8)k; s->cx = (s16)x; s->cy = sh[k].y; s->cw = (u8)w; s->ch = (u8)sh[k].h;
    return 1;
}
COLD static int try_alloc(int id, int w, int h) {    // a cell for w x h (the old one, if any, stays until the new one is found)
    int k, best = -1;
    // a shelf just about this tall with room
    for (k = 0; k < nsh; k++) if (sh[k].h >= h && sh[k].h <= h + MAX(8, h >> 2) && shelf_n(k) && (best < 0 || sh[k].h < sh[best].h) && shelf_gap(k, w) >= 0) best = k;
    if (best >= 0) return place(id, w, h, best);
    // a new shelf
    if ((k = new_shelf((h + 7) & ~7)) >= 0 && place(id, w, h, k)) return 1;
    // any shelf tall enough with room
    for (k = 0; k < nsh; k++) if (sh[k].h >= h && place(id, w, h, k)) return 1;
    return 0;
}
static int cell_alloc(int id, int w, int h) {
    s8 old = spr[id].shelf; s16 ox = spr[id].cx, oy = spr[id].cy; u8 ow = spr[id].cw, oh = spr[id].ch;
    spr[id].shelf = -1;                          // (its old cell does not count as taken while looking: it is about to be given up)
    int ok = try_alloc(id, w, h);
    if (!ok) {                                   // full: everyone not on the screen this frame gives their cell up, and try again
        for (int i = 0; i < SPR_MAX; i++) if (i != id && spr[i].shelf >= 0 && spr[i].drawn != fr) cell_free(i);
        shelves_tidy();
        ok = try_alloc(id, w, h);
    }
    if (!ok) {                                   // no luck: keep the old cell (if it had one)
        if (old >= 0 && old < nsh && sh[old].y == oy) { spr[id].shelf = old; spr[id].cx = ox; spr[id].cy = oy; spr[id].cw = ow; spr[id].ch = oh; }
        else spr[id].shelf = -1;
        return 0;
    }
    return 1;
}
int crowd_cells_used(void) { int a = 0; for (int i = 0; i < SPR_MAX; i++) if (spr[i].shelf >= 0) a += spr[i].cw * spr[i].ch; return a; }

void crowd_flush(void) {                        // every sprite thrown away (their VRAM was drawn over: the world map)
    for (int i = 0; i < SPR_MAX; i++) { spr[i].ok = 0; spr[i].shelf = -1; }
    nsh = 0;
}
void *crowd_scratch(u32 *bytes) { *bytes = ARENA_BYTES; return arena_mem; }   // (a new life only: the arena before any model is stored)
void crowd_reset(void) {
    for (int i = 0; i < CROWD_MAX; i++) { m_off[i] = -1; face_of[i] = 255; }
    for (int i = 0; i < SPR_MAX; i++) { spr[i].ok = 0; spr[i].shelf = -1; spr[i].meas = 0; }
    for (int k = 0; k < NFACE; k++) face_owner[k] = -1;
    nsh = 0;
    ar_top = 0;
}
void crowd_init(void) {
    ar = arena_mem; ar_size = ARENA_BYTES;
    if (ram8) { u8 *big = (u8 *)xalloc(1024 * 1024); if (big) { ar = big; ar_size = 1024 * 1024; } }   // 8 MB: a store big enough that nobody's model is ever let go
    crowd_reset();
}
int crowd_has(int id) { return id >= 0 && id < CROWD_MAX && m_off[id] >= 0; }
int crowd_models(void) { int n = 0; for (int i = 0; i < CROWD_MAX; i++) n += m_off[i] >= 0; return n; }

void crowd_drop_sprite(int id) { cell_free(id); }
void crowd_forget(int id) {
    if (id < 0 || id >= CROWD_MAX) return;
    m_off[id] = -1;
    if (face_of[id] != 255) { face_owner[face_of[id]] = -1; face_of[id] = 255; }
    crowd_drop_sprite(id); spr[id].meas = 0;
}
static void compact(void) {   // slide the models down over the holes
    u32 cur = 0;
    for (;;) {
        int best = -1; u32 bo = 0xffffffffu;
        for (int i = 0; i < CROWD_MAX; i++) if (m_off[i] >= 0 && (u32)m_off[i] >= cur && (u32)m_off[i] < bo) { bo = (u32)m_off[i]; best = i; }
        if (best < 0) break;
        u32 n = ((CHead *)(ar + bo))->bytes;
        if (bo != cur) { memmove(ar + cur, ar + bo, n); m_off[best] = (s32)cur; }
        cur += n;
    }
    ar_top = cur;
}
static int lru_victim(int keep) {   // the stored person not drawn for the longest (never one drawn this frame or the last)
    int best = -1, age = 1;
    for (int i = 0; i < CROWD_MAX; i++) {
        if (i == keep || m_off[i] < 0) continue;
        int a = (u16)(fr - m_used[i]);
        if (a > age) { age = a; best = i; }
    }
    return best;
}
static s32 alloc(u32 n, int keep) {
    for (int tries = 0; tries < CROWD_MAX; tries++) {
        if (ar_top + n <= ar_size) { s32 o = (s32)ar_top; ar_top += n; return o; }
        compact();
        if (ar_top + n <= ar_size) continue;
        int v = lru_victim(keep); if (v < 0) return -1;
        crowd_forget(v);
    }
    return -1;
}

COLD void crowd_faces(int id, Bore *b) {
    if (id < 0 || id >= CROWD_MAX) return;
    int k = face_of[id];
    if (k == 255) {
        static int face_cap = NFACE;   // how many faces there is texture room for (found when the room runs out)
        for (k = 0; k < face_cap && face_owner[k] >= 0; k++) { }
        if (k < face_cap && (!fe[k].tpage || !fm[k].tpage)) {   // a new slot: its eyes and mouth textures
            if (!fe[k].tpage) fe[k] = atlas_try(19, 8);
            if (!fm[k].tpage) fm[k] = atlas_try(19, 8);
            if (!fe[k].tpage || !fm[k].tpage) face_cap = k;      // (full: from now on the slots there are are shared out)
        }
        if (k >= face_cap) { int v = lru_victim(id); if (v < 0 || face_of[v] == 255) return; k = face_of[v]; crowd_forget(v); }
        face_owner[k] = (s16)id; face_of[id] = (u8)k;
    }
    b->tex_eye = fe[k]; b->tex_mouth = fm[k];
}

COLD int crowd_store(int id, const Bore *b) {
    if (id < 0 || id >= CROWD_MAX || face_of[id] == 255) return 0;
    u32 n = (sizeof(CHead) + b->ncell * sizeof(CCell) + 3) & ~3u;
    if (m_off[id] >= 0 && ((CHead *)(ar + m_off[id]))->bytes != n) { s32 keepface = face_of[id]; m_off[id] = -1; face_of[id] = (u8)keepface; }
    if (m_off[id] < 0) { s32 o = alloc(n, id); if (o < 0) return 0; m_off[id] = o; }
    CHead *h = (CHead *)(ar + m_off[id]);
    h->bytes = (u16)n; h->ncell = b->ncell; h->ndec = b->ndec;
    h->stage = b->stage; h->legs = b->legs; h->torso = b->torso; h->headsz = b->headsz;
    h->bx0 = b->bx0; h->bxw = b->bxw; h->bxh = b->bxh; h->bxd = b->bxd;
    h->face = face_of[id]; h->eye_w = b->tex_eye.w; h->mouth_w = b->tex_mouth.w;
    memcpy(h->col, b->col, sizeof h->col);
    memcpy(h->w, &b->w_headx, sizeof h->w);
    memcpy(h->decc, b->decc, 8);
    for (int i = 0; i < 8; i++) { int c = b->decc[i], x = c % W, z = (c / W) % D, y = c / (W * D); h->deccode[i] = i < b->ndec ? b->dec[y][z][x] : 0; }
    memcpy(h->look, b->look, LK_COUNT);
    memcpy(h->vox, b->vox, sizeof h->vox);
    CCell *cc = (CCell *)(h + 1);
    for (int i = 0; i < b->ncell; i++) {
        int x = b->cell[i].x, y = b->cell[i].y, z = b->cell[i].z;
        cc[i].xyz = (u8)(x | (y << 3) | (z << 6)); cc[i].v = b->cell[i].v;
        cc[i].hf = (u8)((b->cell[i].hide & 63) | (b->cell[i].flags << 6)); cc[i].grp = b->cell[i].grp;
        memcpy(cc[i].d, b->cell[i].d, 24);
    }
    m_used[id] = fr; crowd_bytes = (int)ar_top;
    return 1;
}

static Bore ub;                                  // the unpacked person (one at a time)
Bore *crowd_temp_bore(void) { return &ub; }      // (lent out: a wedding builds the newly-wed in it)
COLD const Bore *crowd_unpack(int id) {
    if (!crowd_has(id)) return 0;
    const CHead *h = (const CHead *)(ar + m_off[id]);
    ub.ncell = h->ncell; ub.ndec = h->ndec;
    ub.stage = h->stage; ub.legs = h->legs; ub.torso = h->torso; ub.headsz = h->headsz;
    ub.bx0 = h->bx0; ub.bxw = h->bxw; ub.bxh = h->bxh; ub.bxd = h->bxd;
    ub.tex_eye = fe[h->face]; ub.tex_eye.w = h->eye_w; ub.tex_mouth = fm[h->face]; ub.tex_mouth.w = h->mouth_w;
    memcpy(ub.col, h->col, sizeof ub.col);
    memcpy(&ub.w_headx, h->w, sizeof h->w);
    memcpy(ub.decc, h->decc, 8);
    memset(ub.dec, 0, sizeof ub.dec);
    for (int i = 0; i < h->ndec && i < 8; i++) { int c = h->decc[i], x = c % W, z = (c / W) % D, y = c / (W * D); ub.dec[y][z][x] = h->deccode[i]; }
    memcpy(ub.look, h->look, LK_COUNT);
    memcpy(ub.vox, h->vox, sizeof ub.vox);
    const CCell *cc = (const CCell *)(h + 1);
    for (int i = 0; i < h->ncell; i++) {
        int x = cc[i].xyz & 7, y = (cc[i].xyz >> 3) & 7, z = cc[i].xyz >> 6;
        ub.cell[i].x = (u8)x; ub.cell[i].y = (u8)y; ub.cell[i].z = (u8)z; ub.cell[i].v = cc[i].v;
        ub.cell[i].hide = cc[i].hf & 63; ub.cell[i].flags = cc[i].hf >> 6; ub.cell[i].grp = cc[i].grp;
        memcpy(ub.cell[i].d, cc[i].d, 24);
    }
    m_used[id] = fr;
    return &ub;
}

// ---- drawing --------------------------------------------------------------------------------------------------------
void crowd_frame(void) { fr++; if (!fr) fr = 1; crowd_renders = 0; }
int crowd_draw_live(int id, const VECTOR *pos, int yaw, const Gait *g) {
    const Bore *b = crowd_unpack(id); if (!b) return 0;
    // (the sprite's cell is kept: if they step back they are a sprite again at once; if VRAM runs short it goes first)
    voxel_draw(b, pos, yaw, g);
    return 1;
}
static int atan2_c(int y, int x) {               // 0..4095 round the circle
    if (!x && !y) return 0;
    int ax = ABS(x), ay = ABS(y), hi = MAX(ax, ay), lo = MIN(ax, ay), r = (lo << 12) / hi;
    int a = (r * (512 + ((178 * (4096 - r)) >> 12))) >> 12;
    if (ay > ax) a = 1024 - a; if (x < 0) a = 2048 - a; if (y < 0) a = 4096 - a;
    return a & 4095;
}
static int view_angle(const VECTOR *pos, int yaw) {   // where the eye stands round the person (their facing taken off)
    return (atan2_c(cam.eye.vx - pos->vx, -(cam.eye.vz - pos->vz)) - yaw) & 4095;
}
static int pose_of(const Gait *g) { return g->sleep ? 2 : g->sit ? 1 : 0; }
// the box a render must cover, from the anchor at depth z: the last measured one moved to this depth plus a margin (more
// for someone moving their arms about), or (never measured, or since sat or lain down) a guess from the feet and the head
static int want_box(const Spr *s, const VECTOR *pos, const Gait *g, int z, int ax, int ay, int *b) {
    if (s->meas && s->pose == pose_of(g)) {
        int x0 = s->mx0 * s->z / z, y0 = s->my0 * s->z / z, x1 = s->mx1 * s->z / z, y1 = s->my1 * s->z / z;
        int h = y1 - y0, busy = g->amp > 600 || g->wave || g->talk;
        int mx = 2 + h / (busy ? 7 : 14), my = 2 + h / 20;
        b[0] = x0 - mx; b[1] = y0 - my; b[2] = x1 + mx; b[3] = y1 + my;
        return 1;
    }
    int fx, fy, hx, hy;
    int zf = gfx_proj(pos->vx, pos->vy + 30, pos->vz, &fx, &fy), zh = gfx_proj(pos->vx, pos->vy - (g->sleep ? 200 : 470), pos->vz, &hx, &hy);
    if (!zf || !zh) return 0;
    int zn = MIN(zf, zh), m = (70 * 230) / zn, hw = (((g->sleep ? 300 : 150) * 230) / zn) * (gfx_wide ? 3 : 4) / 4;
    b[0] = MIN(fx, hx) - hw - ax; b[2] = MAX(fx, hx) + hw - ax; b[1] = MIN(fy, hy) - m - ay; b[3] = MAX(fy, hy) + m / 2 - ay;
    return 1;
}
int crowd_need_h(int id, const VECTOR *pos, const Gait *g) {   // how tall the person's sprite would be at 1:1 now (0: not known)
    const Spr *s = &spr[id];
    if (!crowd_has(id) || !s->meas || s->pose != pose_of(g)) return 0;
    int ax, ay, z = gfx_proj(pos->vx, pos->vy, pos->vz, &ax, &ay); if (!z) return 0;
    return (s->my1 - s->my0) * s->z / z;
}
static int score_any(int id, const VECTOR *pos, int yaw, const Gait *g);
int crowd_sprite_score(int id, const VECTOR *pos, int yaw, const Gait *g) { return crowd_has(id) ? score_any(id, pos, yaw, g) : 0; }
int crowd_sprite_score_of(int id, const VECTOR *pos, int yaw, const Gait *g) { return (id >= CROWD_MAX && id < SPR_MAX) ? score_any(id, pos, yaw, g) : 0; }
static int score_any(int id, const VECTOR *pos, int yaw, const Gait *g) {
    Spr *s = &spr[id];
    s->drawn = fr;                               // wanted this frame: its cell is not to be given to anyone else now
    if (!s->ok) return 100000;
    int sx, sy, z = gfx_proj(pos->vx, pos->vy, pos->vz, &sx, &sy); if (!z) return 0;
    int age = (u16)(fr - s->frame);
    int da = ((view_angle(pos, yaw) - s->ang + 6144) & 4095) - 2048; da = ABS(da);
    int dz = (ABS(z - s->z) * 64) / MAX(1, s->z);
    int busy = g->amp > 600 || g->wave || g->talk || g->sit;
    int shown = (s->rh * s->z) / z;                                    // how tall it shows now
    int e = age * (busy ? 3 : 1) + da / 10 + dz + 1;
    if (s->clip) e += 60;                                              // a bit was cut off
    if (s->pose != pose_of(g)) e += 60;                                // sat down, got up: the old picture is wrong
    if (shown > s->th + 4) e += (shown - s->th);                       // shown bigger than rendered: blurry
    return e * (8 + shown / 6) / 16;                                   // the bigger on the screen, the more it shows
}
int crowd_render_hb = 80;
static const Bore *r_bore;                       // render this Bore instead of a stored model (the household)                     // what a render costs (a running average, in hblanks)
static int render(int id, const VECTOR *pos, int yaw, const Gait *g) {
    Spr *s = &spr[id]; int t0_ = gfx_hb();
    int ax, ay, z = gfx_proj(pos->vx, pos->vy, pos->vz, &ax, &ay); if (!z) return 0;
    int b[4]; if (!want_box(s, pos, g, z, ax, ay, b)) return 0;
    int bw = b[2] - b[0] + 1, bh = b[3] - b[1] + 1;
    if (bw < 4 || bh < 4 || bw > 1000 || bh > 1000) return 0;
    // the scale: 1:1 if the cell can be that big; then a cell, kept if the old one is about right, else a new one (smaller
    // and smaller if VRAM is short)
    int hs = 4096;
    if (bh > CELL_HMAX) hs = (CELL_HMAX << 12) / bh;
    if (((bw * hs) >> 12) > CELL_WMAX) hs = (CELL_WMAX << 12) / bw;
    int tw = 0, th = 0, got = 0;
    for (int t = 0; t < 4; t++) {
        if (t) hs = hs * 13 / 16;
        tw = ((bw * hs) >> 12) + 1; th = ((bh * hs) >> 12) + 1;
        if (s->shelf >= 0 && tw <= s->cw && th <= s->ch && tw + 16 >= s->cw * 3 / 4 && th * 4 >= s->ch * 3) got = 1;
        else got = cell_alloc(id, (tw + 3) & ~3, th);
        if (!got && t == 0 && s->shelf >= 0 && tw <= s->cw && th <= s->ch) got = 1;   // (a roomy old cell beats none)
        if (got) break;
    }
    if (!got) return 0;
    int sx0 = CENTER_X + (((ax + b[0] - CENTER_X) * hs) >> 12), sy0 = CENTER_Y + (((ay + b[1] - CENTER_Y) * hs) >> 12);
    const Bore *mb = r_bore ? r_bore : crowd_unpack(id); if (!mb) return 0;
    if (!gfx_imp_begin(s->cx, s->cy, tw, th, s->cx - sx0, s->cy - sy0, z >> 2, hs)) return 0;
    gfx_bb[0] = gfx_bb[1] = 32767; gfx_bb[2] = gfx_bb[3] = -32768; gfx_bb_on = 1;
    voxel_draw(mb, pos, yaw, g);
    gfx_bb_on = 0;
    gfx_imp_end();
    s->rx = (s16)b[0]; s->ry = (s16)b[1]; s->rw = (s16)bw; s->rh = (s16)bh; s->tw = (u8)tw; s->th = (u8)th;
    s->z = z; s->ang = (u16)view_angle(pos, yaw); s->frame = fr; s->ok = 1; s->pose = (u8)pose_of(g);
    // what the person really covered (unscaled, from the anchor): the next render's box, and whether this one cut a bit off
    if (gfx_bb[0] <= gfx_bb[2]) {
        int m0 = CENTER_X + (((gfx_bb[0] - CENTER_X) << 12) / hs) - ax, m2 = CENTER_X + (((gfx_bb[2] - CENTER_X) << 12) / hs) - ax;
        int m1 = CENTER_Y + (((gfx_bb[1] - CENTER_Y) << 12) / hs) - ay, m3 = CENTER_Y + (((gfx_bb[3] - CENTER_Y) << 12) / hs) - ay;
        s->clip = m0 < b[0] - 1 || m1 < b[1] - 1 || m2 > b[2] + 1 || m3 > b[3] + 1;
        s->mx0 = (s16)m0; s->my0 = (s16)m1; s->mx1 = (s16)m2; s->my1 = (s16)m3; s->meas = 1;
    }
    crowd_renders++;
    { int c = gfx_hb() - t0_; if (c > 0 && c < 400) crowd_render_hb += (c - crowd_render_hb) / 8; }
    return 1;
}
static int sprite_any(int id, const VECTOR *pos, int yaw, const Gait *g, int want_render);
int crowd_sprite(int id, const VECTOR *pos, int yaw, const Gait *g, int want_render) { return crowd_has(id) ? sprite_any(id, pos, yaw, g, want_render) : 0; }
int crowd_sprite_of(int id, const Bore *b, const VECTOR *pos, int yaw, const Gait *g, int want_render) {
    if (id < CROWD_MAX || id >= SPR_MAX) return 0;
    r_bore = b; int r = sprite_any(id, pos, yaw, g, want_render); r_bore = 0; return r;
}
COLD static int sprite_any(int id, const VECTOR *pos, int yaw, const Gait *g, int want_render) {
    if (want_render) render(id, pos, yaw, g);
    Spr *s = &spr[id];
    if (!s->ok || s->shelf < 0) return 0;
    int ax, ay, z = gfx_proj(pos->vx, pos->vy, pos->vz, &ax, &ay); if (!z) return 0;
    int kk = (s->z << 12) / z;                      // nearer or further than when it was rendered: scale with it
    int x = ax + ((s->rx * kk) >> 12), y = ay + ((s->ry * kk) >> 12), w = (s->rw * kk) >> 12, h = (s->rh * kk) >> 12;
    int dx = 0;
    if (g->amp > 600) y -= (ABS(isin(g->phase * 2)) * 3) >> 12;                 // a step's bob between renders
    if (g->wave) dx = isin((g->wave * 400) & 4095) >> 10;                       // a sway
    gfx_imp_sprite(s->cx, s->cy, s->tw, s->th, x, y, w, h, (z >> 2) - 6, dx);
    s->drawn = fr; if (id < CROWD_MAX) m_used[id] = fr;
    return 1;
}
