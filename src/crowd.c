// crowd: see crowd.h
#include "crowd.h"
#include "gfx.h"
#include "atlas.h"

extern const u8 voices_bin[];
extern const u32 voices_bin_size;

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
#define CW 48
#define CH 64
#define NCX 13                                  // 13 x 4 cells of 48 x 64 at x 0..623, y 256..511
#define NCELL (NCX * 4)
typedef struct { s16 rx, ry, rw, rh; u8 tw, th; u8 cell, ok; s32 z; u16 ang, frame; } Spr;
static Spr spr[CROWD_MAX];
static s16 cell_owner[NCELL]; static u16 cell_used[NCELL];

void crowd_reset(void) {
    for (int i = 0; i < CROWD_MAX; i++) { m_off[i] = -1; face_of[i] = 255; spr[i].ok = 0; spr[i].cell = 255; }
    for (int k = 0; k < NFACE; k++) face_owner[k] = -1;
    for (int k = 0; k < NCELL; k++) cell_owner[k] = -1;
    ar_top = 0;
}
void crowd_init(void) {
    ar = (u8 *)(((u32)voices_bin + 3) & ~3u); ar_size = (voices_bin_size - 4) & ~3u;
    crowd_reset();
}
int crowd_has(int id) { return id >= 0 && id < CROWD_MAX && m_off[id] >= 0; }
int crowd_models(void) { int n = 0; for (int i = 0; i < CROWD_MAX; i++) n += m_off[i] >= 0; return n; }

void crowd_drop_sprite(int id) {
    if (spr[id].cell != 255) { cell_owner[spr[id].cell] = -1; spr[id].cell = 255; }
    spr[id].ok = 0;
}
void crowd_forget(int id) {
    if (id < 0 || id >= CROWD_MAX) return;
    m_off[id] = -1;
    if (face_of[id] != 255) { face_owner[face_of[id]] = -1; face_of[id] = 255; }
    crowd_drop_sprite(id);
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

void crowd_faces(int id, Bore *b) {
    if (id < 0 || id >= CROWD_MAX) return;
    int k = face_of[id];
    if (k == 255) {
        for (k = 0; k < NFACE && face_owner[k] >= 0; k++) { }
        if (k == NFACE) { int v = lru_victim(id); if (v < 0 || face_of[v] == 255) return; k = face_of[v]; crowd_forget(v); }
        if (!fe[k].tpage) { fe[k] = atlas_alloc_page(2, 19, 8); fm[k] = atlas_alloc_page(2, 19, 8); }
        if (!fe[k].tpage || !fm[k].tpage) return;
        face_owner[k] = (s16)id; face_of[id] = (u8)k;
    }
    b->tex_eye = fe[k]; b->tex_mouth = fm[k];
}

int crowd_store(int id, const Bore *b) {
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
        for (int k = 0; k < 8; k++) {
            const SVECTOR *c = &b->cell[i].c[k];
            int nx = (x - 3 + (k & 1)) * VOX, ny = (k < 4) ? -(y + 1) * VOX : -y * VOX, nz = (z - 2 + ((k >> 1) & 1)) * VOX;
            cc[i].d[k * 3 + 0] = (s8)CLAMP(c->vx - nx, -128, 127);
            cc[i].d[k * 3 + 1] = (s8)CLAMP(c->vy - ny, -128, 127);
            cc[i].d[k * 3 + 2] = (s8)CLAMP(c->vz - nz, -128, 127);
        }
    }
    m_used[id] = fr; crowd_bytes = (int)ar_top;
    return 1;
}

static Bore ub;                                  // the unpacked person (one at a time)
const Bore *crowd_unpack(int id) {
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
        int x0 = (x - 3) * VOX, z0 = (z - 2) * VOX, yt = -(y + 1) * VOX, yb = -y * VOX;
        const s8 *d = cc[i].d; SVECTOR *c = ub.cell[i].c;
        for (int k = 0; k < 8; k++, d += 3) {
            c[k].vx = (s16)(x0 + ((k & 1) ? VOX : 0) + d[0]);
            c[k].vy = (s16)((k < 4 ? yt : yb) + d[1]);
            c[k].vz = (s16)(z0 + ((k & 2) ? VOX : 0) + d[2]);
        }
    }
    m_used[id] = fr;
    return &ub;
}

// ---- drawing --------------------------------------------------------------------------------------------------------
void crowd_frame(void) { fr++; if (!fr) fr = 1; crowd_renders = 0; }
int crowd_draw_live(int id, const VECTOR *pos, int yaw, const Gait *g) {
    const Bore *b = crowd_unpack(id); if (!b) return 0;
    crowd_drop_sprite(id);                       // (live now: the cell is free for someone else)
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
int crowd_sprite_score(int id, const VECTOR *pos, int yaw, const Gait *g) {
    if (!crowd_has(id)) return 0;
    const Spr *s = &spr[id];
    if (!s->ok) return 100000;
    int sx, sy, z = gfx_proj(pos->vx, pos->vy, pos->vz, &sx, &sy); if (!z) return 0;
    int age = (u16)(fr - s->frame);
    int da = ((view_angle(pos, yaw) - s->ang + 6144) & 4095) - 2048; da = ABS(da);
    int dz = (ABS(z - s->z) * 64) / MAX(1, s->z);
    int busy = g->amp > 600 || g->wave || g->talk || g->sit;
    return age * (busy ? 4 : 1) + da / 12 + dz + 1;
}
static int take_cell(int id) {
    if (spr[id].cell != 255) return spr[id].cell;
    int best = -1, age = 0;
    for (int k = 0; k < NCELL; k++) {
        if (cell_owner[k] < 0) { best = k; break; }
        int a = (u16)(fr - cell_used[k]);
        if (a > age) { age = a; best = k; }
    }
    if (best < 0 || (cell_owner[best] >= 0 && age < 2)) return -1;   // every cell is on screen right now
    if (cell_owner[best] >= 0) { spr[cell_owner[best]].ok = 0; spr[cell_owner[best]].cell = 255; }
    cell_owner[best] = (s16)id; spr[id].cell = (u8)best; spr[id].ok = 0;
    return best;
}
static int render(int id, const VECTOR *pos, int yaw, const Gait *g) {
    int k = take_cell(id); if (k < 0) return 0;
    // the screen box the person fills: from the feet to over the head (a hat, antennae), arms out to the sides, and a margin
    // for the body's depth seen from above (lying down: long and low)
    int fx, fy, hx, hy, ax, ay;
    int zf = gfx_proj(pos->vx, pos->vy + 30, pos->vz, &fx, &fy), zh = gfx_proj(pos->vx, pos->vy - (g->sleep ? 200 : 480), pos->vz, &hx, &hy);
    int z = gfx_proj(pos->vx, pos->vy, pos->vz, &ax, &ay);
    if (!zf || !zh || !z) return 0;
    int zn = MIN(zf, zh), m = (110 * 230) / zn, hw = ((g->sleep ? 300 : 175) * 230) / zn;
    int mnx = MIN(fx, hx) - hw, mxx = MAX(fx, hx) + hw, mny = MIN(fy, hy) - m, mxy = MAX(fy, hy) + m / 2;
    int bw = mxx - mnx + 1, bh = mxy - mny + 1;
    if (bw < 4 || bh < 4 || bw > 2000 || bh > 2000) return 0;
    int hs = 4096;
    if (bw > CW) hs = (CW << 12) / bw;
    if ((bh * hs >> 12) > CH) hs = (CH << 12) / bh;
    int sx0 = CENTER_X + (((mnx - CENTER_X) * hs) >> 12), sy0 = CENTER_Y + (((mny - CENTER_Y) * hs) >> 12);
    int tw = MIN(CW, ((bw * hs) >> 12) + 1), th = MIN(CH, ((bh * hs) >> 12) + 1);
    int cx = (k % NCX) * CW, cy = 256 + (k / NCX) * CH;
    const Bore *b = crowd_unpack(id); if (!b) return 0;
    if (!gfx_imp_begin(cx, cy, CW, CH, cx - sx0, cy - sy0, z >> 2, hs)) return 0;

    voxel_draw(b, pos, yaw, g);
    gfx_imp_end();
    Spr *s = &spr[id];
    s->rx = (s16)(mnx - ax); s->ry = (s16)(mny - ay); s->rw = (s16)bw; s->rh = (s16)bh; s->tw = (u8)tw; s->th = (u8)th;
    s->z = z; s->ang = (u16)view_angle(pos, yaw); s->frame = fr; s->ok = 1;
    crowd_renders++;
    return 1;
}
int crowd_sprite(int id, const VECTOR *pos, int yaw, const Gait *g, int want_render) {
    if (!crowd_has(id)) return 0;
    if (want_render) render(id, pos, yaw, g);
    Spr *s = &spr[id];
    if (!s->ok) return 0;
    int ax, ay, z = gfx_proj(pos->vx, pos->vy, pos->vz, &ax, &ay); if (!z) return 0;
    int kk = (s->z << 12) / z;                      // nearer or further than when it was rendered: scale with it
    int x = ax + ((s->rx * kk) >> 12), y = ay + ((s->ry * kk) >> 12), w = (s->rw * kk) >> 12, h = (s->rh * kk) >> 12;
    int dx = 0;
    if (g->amp > 600) y -= (ABS(isin(g->phase * 2)) * 3) >> 12;                 // a step's bob between renders
    if (g->wave) dx = isin((g->wave * 400) & 4095) >> 10;                       // a sway
    gfx_imp_sprite((s->cell % NCX) * CW, 256 + (s->cell / NCX) * CH, s->tw, s->th, x, y, w, h, (z >> 2) - 6, dx);
    cell_used[s->cell] = fr; m_used[id] = fr;
    return 1;
}
