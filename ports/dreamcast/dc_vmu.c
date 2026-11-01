// dc_vmu.c: the PS1 memory card calls (mcard.h) on the Dreamcast's VMU, through KOS's vmufs. save.c is unchanged: it writes its
// file as a stream (the PS1 title and icon sectors, its header, the packed game), and the same bytes become the payload of a VMU
// file with a proper VMU header: "BORE 2", the save's own title ("BORE 2 DIXIE DAY 3"), and a three-frame animated icon that the
// Dreamcast's file manager plays. PS1 file "BASLUS-00000BORE2-1" is VMU file "BORE2_1.SAV".
#include <kos.h>
#include <dc/vmufs.h>
#include <dc/vmu_pkg.h>
#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "mcard.h"

int mcard_last_err = MC_NOCARD, mcard_dir_keep;
static maple_device_t *vmu(void) { return maple_enum_type(0, MAPLE_FUNC_MEMCARD); }
static void vname(const char *ps1, char *out) {   // "BASLUS-00000BORE2-1" -> "BORE2_1.SAV" (12 characters at most)
    const char *s = strstr(ps1, "BORE2"); if (!s) s = ps1;
    int k = 0; for (; *s && k < 7; s++) out[k++] = *s == '-' ? '_' : *s;
    strcpy(out + k, ".SAV");
}

// ---------------------------------------------------------------- the icon: a green voxel Bore head, three frames (look, blink, grin)
static uint8_t icon[3][512]; static int icon_ready;
static const uint16_t icon_pal[16] = { 0x0000, 0xF132, 0xF3A2, 0xF5C3, 0xF8F6, 0xFFFF, 0xF111, 0xFF9A, 0xFFE4, 0xF48D, 0, 0, 0, 0, 0, 0 };
static void ipx(uint8_t *f, int x, int y, int c) { if (x < 0 || y < 0 || x > 31 || y > 31) return; uint8_t *b = &f[y * 16 + x / 2]; if (x & 1) *b = (uint8_t)((*b & 0xf0) | c); else *b = (uint8_t)((*b & 0x0f) | c << 4); }
static void icon_make(void) {
    for (int fr = 0; fr < 3; fr++) {
        uint8_t *f = icon[fr]; memset(f, 0, 512);
        int bob = fr == 2 ? -1 : 0;
        for (int y = 0; y < 32; y++) for (int x = 0; x < 32; x++) {   // the cube: top, left and right faces, outlined
            int yy = y - bob, c = 0;
            if (x >= 5 && x <= 26 && yy >= 9 && yy <= 29) c = x < 16 ? 3 : 2;
            int top = yy >= 3 && yy < 9 && x >= 5 + (8 - yy) && x <= 26 - (8 - yy) + 0; if (yy >= 3 && yy < 9 && x >= 5 && x <= 26 && x >= 11 - (yy - 3) - 0 && x <= 20 + (yy - 3)) top = 1;
            if (top) c = 4;
            if (c && (x == 5 || x == 26 || yy == 29 || (yy == 9 && 0))) c = 1;
            if (c) ipx(f, x, y, c);
        }
        for (int x = 11; x <= 20; x++) ipx(f, x, 3 + bob, 1);
        for (int k = 0; k <= 6; k++) { ipx(f, 11 - k, 3 + k + bob, 1); ipx(f, 20 + k, 3 + k + bob, 1); }
        // the face, on the front
        int ey = 15 + bob;
        if (fr == 1) { for (int x = 9; x <= 13; x++) ipx(f, x, ey + 1, 6); for (int x = 18; x <= 22; x++) ipx(f, x, ey + 1, 6); }
        else for (int dy = 0; dy < 4; dy++) for (int dx = 0; dx < 4; dx++) {
            ipx(f, 10 + dx, ey + dy, (dx >= 1 && dx <= 2 && dy >= (fr == 2 ? 0 : 1)) ? 6 : 5); ipx(f, 19 + dx, ey + dy, (dx >= 1 && dx <= 2 && dy >= (fr == 2 ? 0 : 1)) ? 6 : 5);
        }
        int my = 22 + bob;
        if (fr == 2) { for (int x = 12; x <= 19; x++) ipx(f, x, my + 1, 6); ipx(f, 11, my, 6); ipx(f, 20, my, 6); for (int x = 13; x <= 18; x++) ipx(f, x, my + 2, 7); }
        else for (int x = 13; x <= 18; x++) ipx(f, x, my, 6);
        ipx(f, 8, 21 + bob, 7); ipx(f, 9, 21 + bob, 7); ipx(f, 22, 21 + bob, 7); ipx(f, 23, 21 + bob, 7);   // cheeks
        if (fr == 2) { ipx(f, 28, 2, 8); ipx(f, 29, 1, 8); ipx(f, 30, 2, 8); ipx(f, 29, 3, 8); ipx(f, 29, 2, 8); }   // a sparkle
    }
    icon_ready = 1;
}

// ---------------------------------------------------------------- the file as a stream
static uint8_t *wbuf; static int wcap, wlen, wpending; static char wname[16], wtitle[33];
static uint8_t *rbuf; static int rlen, rpos, rbad; static void *rraw;
static int flush(void) {   // the file onto the VMU
    if (!wpending) return 1;
    wpending = 0;
    maple_device_t *d = vmu(); if (!d) { mcard_last_err = MC_NOCARD; return 0; }
    if (!icon_ready) icon_make();
    vmu_pkg_t pkg; memset(&pkg, 0, sizeof pkg);
    strcpy(pkg.desc_short, "BORE 2");
    strncpy(pkg.desc_long, wtitle[0] ? wtitle : "BORE 2 SAVE", sizeof pkg.desc_long - 1);
    strcpy(pkg.app_id, "BORE 2");
    pkg.icon_cnt = 3; pkg.icon_anim_speed = 12; pkg.eyecatch_type = VMUPKG_EC_NONE;
    memcpy(pkg.icon_pal, icon_pal, sizeof icon_pal); pkg.icon_data = &icon[0][0];
    pkg.data_len = wlen; pkg.data = wbuf;
    uint8_t *out; int outn;
    if (vmu_pkg_build(&pkg, &out, &outn) < 0) { mcard_last_err = MC_WRITE; return 0; }
    int need = (outn + 511) / 512, have = vmufs_free_blocks(d);
    { void *old; int on; if (vmufs_read(d, wname, &old, &on) == 0) { have += (on + 511) / 512; free(old); } }
    if (have < need) { free(out); mcard_last_err = MC_FULL; return 0; }
    extern void dc_vmu_saving(int on); dc_vmu_saving(1);
    int r = vmufs_write(d, wname, out, outn, VMUFS_OVERWRITE);
    dc_vmu_saving(0);
    free(out);
    { extern void dc_logf(const char *f, ...); dc_logf("vmu: wrote %s, %d bytes (%d blocks) \"%s\" r %d free %d\n", wname, outn, need, pkg.desc_long, r, vmufs_free_blocks(d)); }
    if (r < 0) { mcard_last_err = MC_WRITE; return 0; }
    return 1;
}
int mcard_present(void) {
    flush();
    maple_device_t *d = vmu();
    if (!d) { mcard_last_err = MC_NOCARD; return 0; }
    if (vmufs_free_blocks(d) < 0) { mcard_last_err = MC_UNFORMATTED; return 0; }
    return 1;
}
int mcard_read(int sector, u8 *out) { (void)sector; memset(out, 0, 128); return 0; }
int mcard_write(int sector, const u8 *in) { (void)sector; (void)in; return 0; }
int mcard_rbegin(const char *name) {
    flush();
    if (rraw) { free(rraw); rraw = 0; }
    rbuf = 0; rlen = rpos = 0; rbad = 1;
    maple_device_t *d = vmu(); if (!d) { mcard_last_err = MC_NOCARD; return 0; }
    char n[16]; vname(name, n);
    int sz; if (vmufs_read(d, n, &rraw, &sz) < 0) { rraw = 0; mcard_last_err = MC_NOFILE; return 0; }
    vmu_pkg_t pkg; if (vmu_pkg_parse((uint8_t *)rraw, sz, &pkg) < 0) { mcard_last_err = MC_READ; return 0; }
    rbuf = (uint8_t *)pkg.data; rlen = pkg.data_len; rbad = 0;
    return 1;
}
void mcard_rget(void *out, int n) {
    if (rpos + n > rlen) { int k = rlen - rpos; if (k < 0) k = 0; if (k) memcpy(out, rbuf + rpos, k); memset((uint8_t *)out + k, 0, n - k); rpos = rlen; rbad = 1; return; }
    memcpy(out, rbuf + rpos, n); rpos += n;
}
int mcard_rok(void) { if (rbad) mcard_last_err = MC_READ; return !rbad; }
int mcard_wbegin(const char *name, int blocks) {
    flush();
    maple_device_t *d = vmu(); if (!d) { mcard_last_err = MC_NOCARD; return 0; }
    if (vmufs_free_blocks(d) < 0) { mcard_last_err = MC_UNFORMATTED; return 0; }
    vname(name, wname);
    int cap = blocks * 8192; if (cap > 120 * 1024) { mcard_last_err = MC_TOOBIG; return 0; }
    if (cap > wcap) { free(wbuf); wbuf = (uint8_t *)malloc(cap); wcap = wbuf ? cap : 0; if (!wbuf) { mcard_last_err = MC_TOOBIG; return 0; } }
    wlen = 0; wpending = 0;
    return 1;
}
void mcard_wput(const void *in, int n) { if (wlen + n > wcap) n = wcap - wlen; if (n > 0) { memcpy(wbuf + wlen, in, n); wlen += n; } }
int mcard_wend(void) { wpending = 1; return 1; }                     // (written by the patch that follows, or by the next call)
int mcard_wpatch(int sector, const u8 *sec) {
    if (sector * 128 + 128 <= wlen) memcpy(wbuf + sector * 128, sec, 128);
    if (!wpending) wpending = 1;
    return flush();
}
int mcard_delete(const char *name) {
    flush();
    maple_device_t *d = vmu(); if (!d) { mcard_last_err = MC_NOCARD; return 0; }
    char n[16]; vname(name, n);
    void *old; int on; if (vmufs_read(d, n, &old, &on) < 0) return 1;   // (never there)
    free(old);
    return vmufs_delete(d, n) == 0;
}
void mcard_title_sector(u8 *sec, const char *title, int blocks, const u16 *pal) {   // the PS1 card manager's sector: kept as it is; its title names the VMU file
    (void)blocks; (void)pal; memset(sec, 0, 128); sec[0] = 'S'; sec[1] = 'C'; sec[2] = 0x11; sec[3] = 1;
    strncpy(wtitle, title, 32); wtitle[32] = 0;
    for (int i = 0; i < 64 && title[i]; i++) sec[4 + i] = (u8)title[i];
}
