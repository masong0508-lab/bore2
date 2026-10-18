// save: see save.h. The file: the card manager's title and icon sectors, a header sector, then the packed game.
#include "save.h"
#include "mcard.h"

#define NAME "BASLUS-00000BORE2"
#define SAVE_BLOCKS 2
#define VERSION 1
static u32 count;                                 // payload bytes so far
static int ld_bad;

void sv_n(const void *p, int n) { mcard_wput(p, n); count += (u32)n; }
void sv_u8(int v) { u8 b = (u8)v; sv_n(&b, 1); }
void sv_u16(int v) { u8 b[2] = { (u8)v, (u8)(v >> 8) }; sv_n(b, 2); }
void sv_u32(u32 v) { u8 b[4] = { (u8)v, (u8)(v >> 8), (u8)(v >> 16), (u8)(v >> 24) }; sv_n(b, 4); }
void ld_n(void *p, int n) { mcard_rget(p, n); }
int  ld_u8(void) { u8 b; ld_n(&b, 1); return b; }
int  ld_u16(void) { u8 b[2]; ld_n(b, 2); return b[0] | (b[1] << 8); }
u32  ld_u32(void) { u8 b[4]; ld_n(b, 4); return (u32)b[0] | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24); }
void sv_str(const char *s, int cap) { int n = 0; while (n < cap && s[n]) n++; sv_u8(n); sv_n(s, n); }
void ld_str(char *s, int cap) { int n = ld_u8(); if (n > cap) n = cap; ld_n(s, n); s[n] = 0; }

// PackBits: a control byte below 128 is that many + 1 bytes as they are; from 128, the next byte repeated (control - 128 + 3) times
void sv_rle(const void *p, int n) {
    const u8 *s = (const u8 *)p; int i = 0;
    while (i < n) {
        int run = 1; while (i + run < n && s[i + run] == s[i] && run < 130) run++;
        if (run >= 3) { sv_u8(128 + run - 3); sv_u8(s[i]); i += run; continue; }
        int j = i, lit = 0;   // a stretch of odd bytes, until a run of 3 starts
        while (j < n && lit < 128) { if (j + 2 < n && s[j] == s[j + 1] && s[j] == s[j + 2]) break; j++; lit++; }
        sv_u8(lit - 1); sv_n(s + i, lit); i += lit;
    }
}
void ld_rle(void *p, int n) {
    u8 *d = (u8 *)p; int i = 0;
    while (i < n) {
        int c = ld_u8();
        if (c >= 128) { int r = c - 128 + 3, v = ld_u8(); if (r > n - i) { ld_bad = 1; r = n - i; } memset(d + i, v, r); i += r; }
        else { int r = c + 1; if (r > n - i) { ld_bad = 1; r = n - i; } ld_n(d + i, r); i += r; }
    }
}

void ld_fail(void) { ld_bad = 1; }
static const char *const err_text[] = { "", "NO MEMORY CARD IN SLOT 1", "THE CARD IS NOT FORMATTED", "NO SAVE ON THE CARD", "THE CARD COULD NOT BE READ",
                                        "THE CARD COULD NOT BE WRITTEN", "THE CARD IS FULL", "THE SAVE IS TOO BIG" };
static const char *bad_msg;
const char *save_error(void) { return bad_msg ? bad_msg : err_text[mcard_last_err < 8 ? mcard_last_err : 0]; }

static void icon(u8 *sec, u16 *pal) {   // a little green voxel cube: 16 x 16, 4 bits a pixel
    static const u16 col[5] = { 0, 0x0000 | 0x8000, RGB(10, 24, 8) | 0x8000, RGB(5, 15, 4) | 0x8000, RGB(16, 31, 12) | 0x8000 };
    memset(pal, 0, 32); for (int i = 1; i < 5; i++) pal[i] = col[i];
    memset(sec, 0, 128);
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        int c = 0, cx = x - 8, top = y < 7 + (cx < 0 ? -cx : cx) / 2 - 1;
        int in = x >= 2 && x < 14 && y >= 2 && y < 14;
        if (in) c = y < 6 ? 4 : x < 8 ? 3 : 2;
        if (in && (x == 2 || x == 13 || y == 2 || y == 13 || x == 8 || y == 6)) c = 1;
        (void)top;
        sec[y * 8 + x / 2] |= (u8)(c << ((x & 1) * 4));
    }
}
static const u8 magic[8] = { 'B', 'O', 'R', 'E', '2', 'S', 'V', VERSION };

int save_write(void (*body)(void)) {
    bad_msg = 0;
    if (!mcard_wbegin(NAME, SAVE_BLOCKS)) return 0;
    u8 sec[128]; u16 pal[16];
    icon(sec, pal);
    u8 title[128]; mcard_title_sector(title, "BORE 2", SAVE_BLOCKS, pal);
    mcard_wput(title, 128); mcard_wput(sec, 128);
    u8 head[128]; memset(head, 0, 128); memcpy(head, magic, 8);   // (not valid yet: the byte at 8 is set once all of it is on the card)
    mcard_wput(head, 128);
    count = 0;
    body();
    if (!mcard_wend()) return 0;
    head[8] = 1; head[12] = (u8)count; head[13] = (u8)(count >> 8); head[14] = (u8)(count >> 16);
    if (!mcard_wpatch(2, head)) { mcard_last_err = MC_WRITE; return 0; }
    return 1;
}
int save_exists(void) {
    if (!mcard_rbegin(NAME)) return 0;
    u8 skip[256], head[128]; mcard_rget(skip, 256); mcard_rget(head, 128);
    return mcard_rok() && !memcmp(head, magic, 8) && head[8] == 1;
}
int save_read(void (*body)(void)) {
    bad_msg = 0; ld_bad = 0;
    if (!mcard_rbegin(NAME)) return 0;
    u8 skip[256], head[128]; mcard_rget(skip, 256); mcard_rget(head, 128);
    if (!mcard_rok()) return 0;
    if (memcmp(head, magic, 7) || head[8] != 1) { bad_msg = "THAT SAVE IS NOT FINISHED"; return 0; }
    if (head[7] != VERSION) { bad_msg = "THAT SAVE IS FROM ANOTHER VERSION"; return 0; }
    body();
    if (!mcard_rok() || ld_bad) { if (!bad_msg && !ld_bad) bad_msg = 0; if (ld_bad) bad_msg = "THE SAVE IS DAMAGED"; return 0; }
    return 1;
}
