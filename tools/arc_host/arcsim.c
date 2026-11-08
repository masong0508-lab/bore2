// arcsim.c (host test): runs one arcade game with a simple bot and writes the picture (PPM + a list of texts) every N ticks.
//   arcsim GAME TICKS EVERY OUTPREFIX [players]
#include "arc_i.h"
typedef struct { void (*start)(void); int (*tick)(int, int, int, int); void (*draw)(void); } Fn;
#define F(n) { n##_start, n##_tick, n##_draw }
static Fn gm[] = { F(brk), F(cat), F(wak), F(str), F(blk), F(shm), F(rac), F(mze), F(hoo), F(ski), F(hoc), F(rhy), F(claw) };
static const char *nm[] = { "brk", "cat", "wak", "str", "blk", "shm", "rac", "mze", "hoo", "ski", "hoc", "rhy", "claw" };
int ag_pts, ag_t, ag_lives, ag_combo, ag_best, ag_perf, ag_np, ag_foe, ag_shake, ag_level;
static unsigned char fb[240][320][3]; static FILE *txt; static u32 arng = 12345;
int ar8(void) { arng = arng * 1103515245u + 12345u; return (int)((arng >> 16) & 255); }
static void px(int x, int y, u32 c, int semi) { if (x < 0 || y < 0 || x >= 320 || y >= 240) return; for (int k = 0; k < 3; k++) { int v = (c >> (8 * k)) & 255; fb[y][x][k] = (unsigned char)(semi ? (fb[y][x][k] + v) / 2 : v); } }
static void rect(int x, int y, int w, int h, u32 c, int semi) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) px(x + i, y + j, c, semi); }
void RC(int x, int y, int w, int h, u32 c) { rect(FX(x), FY(y), MAX(1, w * 5 / 4), MAX(1, h * 5 / 4), c, 0); }
void RA(int x, int y, int w, int h, u32 c) { rect(FX(x), FY(y), MAX(1, w * 5 / 4), MAX(1, h * 5 / 4), c, 1); }
void DSC(int cx, int cy, int r, u32 c) { int d = r * 5 / 2, X = FX(cx), Y = FY(cy); for (int j = -d / 2; j <= d / 2; j++) for (int i = -d / 2; i <= d / 2; i++) if (i * i + j * j <= d * d / 4) px(X + i, Y + j, c, 0); }
static void tx(int x, int y, const char *s, u32 c, int sz) { fprintf(txt, "%d %d %d %u %s\n", x, y, sz, c, s); }
void TX(int x, int y, const char *s, u32 c) { tx(FX(x), FY(y), s, c, 1); }
void TXC(int cx, int y, const char *s, u32 c) { tx(FX(cx) - (int)strlen(s) * 3, FY(y), s, c, 1); }
void TXB(int cx, int y, const char *s, u32 c) { tx(FX(cx) - (int)strlen(s) * 6, FY(y), s, c, 2); }
void NUM(int x, int y, const char *l, int v, u32 c) { char s[40]; snprintf(s, sizeof s, "%s %d", l, v); TX(x, y, s, c); }
static int ncue; void cue(int k) { ncue++; }
static struct { short x, y, vx, vy; unsigned char life; u32 c; } pt[40];
void part(int x, int y, int vx, int vy, int life, u32 c) { for (int i = 0; i < 40; i++) if (!pt[i].life) { pt[i].x = (short)(x * 16); pt[i].y = (short)(y * 16); pt[i].vx = (short)vx; pt[i].vy = (short)vy; pt[i].life = (unsigned char)life; pt[i].c = c; return; } }
void burst(int x, int y, int n, u32 c) { for (int i = 0; i < n; i++) part(x, y, (ar8() - 128) / 5, (ar8() - 160) / 5, 10 + ar8() % 10, c); }
static struct { short x, y; unsigned char t; u32 c; char s[16]; } pp[6];
void pop(int x, int y, const char *s, u32 c) { int k = 0; for (int i = 0; i < 6; i++) if (!pp[i].t) { k = i; break; } pp[k].x = (short)x; pp[k].y = (short)y; pp[k].t = 26; pp[k].c = c; strncpy(pp[k].s, s, 15); fprintf(stderr, "  [%d] pop %s\n", ag_t, s); }
int main(int argc, char **argv) {
    int g = -1; for (int i = 0; i < 13; i++) if (!strcmp(argv[1], nm[i])) g = i; if (g < 0) return 1;
    int ticks = atoi(argv[2]), every = atoi(argv[3]); ag_np = argc > 5 ? atoi(argv[5]) : 1; arng += (unsigned)g;
    ag_lives = 3; ag_perf = 1; ag_level = 1; gm[g].start(); int hd = 0, pr = 0, h2 = 0, p2 = 0, prev = 0, prev2 = 0, out = 0;
    for (ag_t = 0; ag_t < ticks; ag_t++) {
        static int dir; if (ag_t % 18 == 0) dir = ar8() & 15;
        hd = 0; if (dir & 1) hd |= PAD_LEFT; if (dir & 2) hd |= PAD_RIGHT; if (dir & 4) hd |= PAD_UP; if (dir & 8) hd |= PAD_DOWN;
        if (g == 6 || g == 11 || g == 8 || g == 4 || g == 9 || g == 12 || g == 1 || g == 0) hd &= ~(PAD_UP | PAD_DOWN);
        if (g == 4) { hd |= (ag_t % 9 == 0 ? PAD_CROSS : 0) | (ag_t % 41 == 0 ? PAD_UP : 0); }
        if (g == 5 || g == 6 || g == 3) hd |= PAD_CROSS;
        if (g == 9) hd = (hd & (PAD_LEFT | PAD_RIGHT)) | (((ag_t / 25) & 1) ? 0 : PAD_CROSS);
        if (g == 8 || g == 12 || g == 2 || g == 0) { if ((ag_t % 40) == 39) hd |= PAD_CROSS; }
        if (g == 11) { extern int rhy_bot(void); hd = rhy_bot(); if (ag_t % 7 == 0) hd = 0; }
        if (g == 5 && (ag_t % 90) == 45) hd |= PAD_CIRCLE;
        pr = hd & ~prev; prev = hd; h2 = (ar8() & 15) ? (dir & 15 ? hd & (PAD_LEFT | PAD_RIGHT | PAD_UP | PAD_DOWN) : 0) : 0; p2 = h2 & ~prev2; prev2 = h2;
        for (int i = 0; i < 40; i++) if (pt[i].life) { pt[i].life--; pt[i].x += pt[i].vx; pt[i].y += pt[i].vy; pt[i].vy += 2; }
        for (int i = 0; i < 6; i++) if (pp[i].t) { pp[i].t--; if (pp[i].t & 1) pp[i].y--; }
        int alive = gm[g].tick(hd, pr, h2, p2);
        if (ag_t % every == 0 || !alive) {
            memset(fb, 0, sizeof fb); rect(0, 0, 320, 240, UI_RGB(14, 10, 30), 0); char fn[200]; snprintf(fn, sizeof fn, "%s_%02d.txt", argv[4], out); txt = fopen(fn, "w");
            rect(7, 17, 306, 206, UI_RGB(60, 70, 130), 0); rect(10, 20, 300, 200, UI_RGB(3, 3, 14), 0);
            gm[g].draw(); for (int i = 0; i < 40; i++) if (pt[i].life) RC(pt[i].x >> 4, pt[i].y >> 4, 2, 2, pt[i].c);
            for (int i = 0; i < 6; i++) if (pp[i].t) TXC(pp[i].x, pp[i].y, pp[i].s, pp[i].c);
            fclose(txt); snprintf(fn, sizeof fn, "%s_%02d.ppm", argv[4], out++); FILE *f = fopen(fn, "wb"); fprintf(f, "P6 320 240 255\n"); fwrite(fb, 1, sizeof fb, f); fclose(f);
        }
        if (!alive) { fprintf(stderr, "%s over at tick %d: score %d combo %d perf %d level %d cues %d\n", nm[g], ag_t, ag_pts, ag_best, ag_perf, ag_level, ncue); return 0; }
    }
    fprintf(stderr, "%s ran %d ticks: score %d best combo %d level %d lives %d cues %d\n", nm[g], ticks, ag_pts, ag_best, ag_level, ag_lives, ncue); return 0;
}
