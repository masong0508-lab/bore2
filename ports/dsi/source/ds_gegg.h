// ds_gegg.h (D69): the PS1's redrawn Boreagotchi eggs on the DS. The art is the PS1's own file, src/gotchi_first_art.h (made by tools/make_gotchi_first.py):
// one egg design in three sizes (0 the hatching, 14 x 17 + shadow; 1 one dot of the 32 x 16 LCD, 10 x 12; 2 inside the toy's LCD, 8 x 9), seven patterns
// (the last, gold stars, is the rare one) and three crack stages. gegg_make puts one together in a small buffer; the colours come from the pet's colour
// (gegg_rgb is the PS1's egg_rgb sums, so the LCD, the toy's LCD, the card and the hatching agree).
#include "gotchi_first_art.h"
static const unsigned char *const gfsPat[3][FS_EGG_N + 1] = { { FS_EGGB_P0, FS_EGGB_P1, FS_EGGB_P2, FS_EGGB_P3, FS_EGGB_P4, FS_EGGB_P5, FS_EGGB_P6, FS_EGGB_P7 }, { FS_EGGL_P0, FS_EGGL_P1, FS_EGGL_P2, FS_EGGL_P3, FS_EGGL_P4, FS_EGGL_P5, FS_EGGL_P6, FS_EGGL_P7 },
    { FS_EGGS_P0, FS_EGGS_P1, FS_EGGS_P2, FS_EGGS_P3, FS_EGGS_P4, FS_EGGS_P5, FS_EGGS_P6, FS_EGGS_P7 } };   // (pattern 7: the mystery egg, a night egg with a question mark and stars)
static const char *const *const gfsEggBase[3] = { FS_EGGB, FS_EGGL, FS_EGGS };
static const u8 gfsEggH[3] = { FS_EGGB_H, FS_EGGL_H, FS_EGGS_H }, gfsEggW[3] = { FS_EGGB_W, FS_EGGL_W, FS_EGGS_W };
static const unsigned char *const gfsCrk[2] = { FS_CRACKB, FS_CRACKL }, *const gfsCrkN[2] = { FS_CRACKB_N, FS_CRACKL_N };
static char geggBuf[FS_EGGB_H][FS_EGGB_W + 1]; static const char *geggRow[FS_EGGB_H];
static const char *const *gegg_make(int kind, int pat, int crack) {   // the plain egg, its pattern (runs of spots over body, shade and highlight), its cracks (cells, stage by stage)
    for (int j = 0; j < gfsEggH[kind]; j++) { memcpy(geggBuf[j], gfsEggBase[kind][j], (size_t)gfsEggW[kind] + 1); geggRow[j] = geggBuf[j]; }
    if (pat == FS_EGG_MYS) for (int j = 0; j < gfsEggH[kind]; j++) for (char *c = geggBuf[j]; *c; c++) *c = *c == 'm' ? 'x' : *c == 'd' ? 'X' : *c;   // (the night: body x, shade X)
    for (const unsigned char *p = gfsPat[kind][pat]; p[1]; p += 2) for (int i = 0; i < (p[1] & 31); i++) { char *c = &geggBuf[p[0] >> 4][(p[0] & 15) + i]; if (strchr("mdhxX", *c)) *c = "oxXug"[p[1] >> 5]; }   // (a run's key: bits 5-7)
    if (crack && kind < 2) for (int n = 0; n < gfsCrkN[kind][crack]; n++) { char *c = &geggBuf[gfsCrk[kind][n * 2] >> 4][gfsCrk[kind][n * 2] & 15]; *c = strchr("ogxX", *c) ? 'u' : (char)gfsCrk[kind][n * 2 + 1]; }   // (a crack through a spot is light)
    return geggRow;
}
static const char geggKeys[] = "#mdohulgxX";   // an egg sprite's keys; on the dots they are the values 16 up
// the pet's colour (the DS's eight: GREEN is the classic LCD) as the PS1's PAL rows have them: the egg's outline, body, shade and spots lean to it
static const u8 gEggInk[8][3] = { { 28, 44, 24 }, { 40, 92, 214 }, { 214, 58, 120 }, { 226, 160, 20 }, { 20, 150, 140 }, { 130, 134, 148 }, { 196, 40, 44 }, { 126, 64, 200 } };
static void gegg_rgb(int ink, int key, u8 *o) {   // an egg's colour for a key (an index in geggKeys)
    static const u8 mx[4][2] = { { 0, 42 }, { 74, 26 }, { 48, 52 }, { 0, 92 } }, fx[6][3] = { { 255, 255, 240 }, { 255, 250, 190 }, { 150, 176, 108 }, { 255, 208, 60 }, { 46, 38, 108 }, { 30, 24, 78 } }, cr[3] = { 240, 238, 206 };
    const u8 *in = gEggInk[ink & 7]; for (int i = 0; i < 3; i++) o[i] = key < 4 ? (u8)((cr[i] * mx[key][0] + in[i] * mx[key][1]) / 100) : fx[key - 4][i];
}
static int geggcol;   // the colour index of the egg being drawn (g_egg sets it, gdots_draw reads it)
static u16 gegg_c(int ink, int key) {   // the 16-bit colour, worked out once
    static u16 cc[8][16]; static u8 ok[8]; ink &= 7; if (!ok[ink]) { ok[ink] = 1; for (int k = 0; k < 10; k++) { u8 o[3]; gegg_rgb(ink, k, o); cc[ink][k] = C15(o[0] >> 3, o[1] >> 3, o[2] >> 3); } }
    return cc[ink][key & 15];
}
static int gegg_pat(const Blob *b) { return (b->form & 128) ? FS_EGG_MYS : b->mut ? FS_EGG_N - 1 : b->sp % (FS_EGG_N - 1); }   // the kind picks the pattern (kind % 6), the rare ones are gold stars
