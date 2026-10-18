// mcard: the memory card, read and written straight over the serial port (like the pad: pad.c), sector by sector, and the
// card's own little filing system on top (block 0 is the directory, blocks 1..15 hold the files, 8 KB each: 64 sectors of 128
// bytes). The BIOS has drivers for all this, but they poll from the vblank interrupt, which this game keeps clear of (pad.c).
#include "mcard.h"
#include "pad.h"

#define WAIT 60000                      // spins to wait for a card's /ACK (a write takes it a moment)
#define SECTORS 1024
#define BLOCK_SECS 64
#define NBLK 15

static int port_of = 0;                 // slot 1
static int card_ok;                     // the last read or write got through
int mcard_last_err;

static int begin(int cmd, int sector) {   // 81h, the command, the two ids, the sector: 1 if a card answered (the ids came back)
    sio_select(port_of);
    if (sio_xfer(0x81, 1, WAIT) < 0) return 0;
    if (sio_xfer(cmd, 1, WAIT) < 0) return 0;      // (the reply: the card's FLAG byte)
    int id1 = sio_xfer(0x00, 1, WAIT), id2 = sio_xfer(0x00, 1, WAIT);
    if (id1 != 0x5A || id2 != 0x5D) return 0;
    if (sio_xfer(sector >> 8, 1, WAIT) < 0) return 0;
    if (sio_xfer(sector & 255, 1, WAIT) < 0) return 0;
    return 1;
}
int mcard_read(int sector, u8 *out) {   // one sector: 1 read, 0 no card / bad
    for (int tries = 0; tries < 3; tries++) {
        int ok = 0;
        if (begin(0x52, sector)) {
            int a1 = sio_xfer(0, 1, WAIT), a2 = sio_xfer(0, 1, WAIT);
            int hi = sio_xfer(0, 1, WAIT), lo = sio_xfer(0, 1, WAIT);
            if (a1 == 0x5C && a2 == 0x5D && hi == (sector >> 8) && lo == (sector & 255)) {
                int chk = hi ^ lo, bad = 0;
                for (int i = 0; i < 128; i++) { int v = sio_xfer(0, 1, WAIT); if (v < 0) { bad = 1; break; } out[i] = (u8)v; chk ^= v; }
                int sum = bad ? -1 : sio_xfer(0, 1, WAIT), end = bad ? -1 : sio_xfer(0, 0, WAIT);
                ok = !bad && sum == chk && end == 0x47;
            }
        }
        sio_deselect();
        if (ok) { card_ok = 1; return 1; }
    }
    card_ok = 0; return 0;
}
int mcard_write(int sector, const u8 *in) {   // one sector: 1 written and confirmed
    for (int tries = 0; tries < 3; tries++) {
        int ok = 0;
        if (begin(0x57, sector)) {
            int chk = (sector >> 8) ^ (sector & 255), bad = 0;
            for (int i = 0; i < 128; i++) { if (sio_xfer(in[i], 1, WAIT) < 0) { bad = 1; break; } chk ^= in[i]; }
            if (!bad && sio_xfer(chk, 1, WAIT) >= 0) {
                int a1 = sio_xfer(0, 1, WAIT), a2 = sio_xfer(0, 1, WAIT), end = sio_xfer(0, 0, WAIT);
                ok = a1 == 0x5C && a2 == 0x5D && end == 0x47;
            }
        }
        sio_deselect();
        if (ok) { card_ok = 1; return 1; }
    }
    card_ok = 0; return 0;
}
int mcard_present(void) { u8 s[128]; return mcard_read(0, s) && s[0] == 'M' && s[1] == 'C'; }   // a formatted card is in slot 1

// ---- the directory: sector n (1..15) of block 0 describes block n ----
static u8 dir[NBLK + 1][128];
static int dir_ok;
static int dir_load(void) {
    dir_ok = 0;
    for (int i = 0; i <= NBLK; i++) if (!mcard_read(i, dir[i])) return 0;
    if (dir[0][0] != 'M' || dir[0][1] != 'C') { mcard_last_err = MC_UNFORMATTED; return 0; }
    dir_ok = 1; return 1;
}
static int dir_free(int b) { int s = dir[b][0]; return s == 0xA0 || s == 0xA1 || s == 0xA2 || s == 0xA3 || s == 0xFF; }   // never used, or freed
static void dir_fix(u8 *e) { u8 x = 0; for (int i = 0; i < 127; i++) x ^= e[i]; e[127] = x; }
static int dir_find(const char *name) {   // the first block of that file (0: none)
    for (int b = 1; b <= NBLK; b++) if (dir[b][0] == 0x51 && !strncmp((const char *)&dir[b][10], name, 20)) return b;
    return 0;
}
static int chain(int first, u8 *blocks) {   // the blocks of a file, from its first: how many
    int n = 0, b = first;
    while (b >= 1 && b <= NBLK && n < NBLK) { blocks[n++] = (u8)b; if (dir[b][0] == 0x53) break; int nx = dir[b][8] | (dir[b][9] << 8); if (nx == 0xFFFF) break; b = nx + 1; }
    return n;
}
// ---- a file as a stream: the data goes out (or comes in) a sector at a time, so no big buffer is needed ----
static u8 sbl[NBLK]; static int snb, sdone, sbad, sfresh;   // the file's blocks, how many, sectors done, a failure, 1: blocks newly taken
static u8 sbuf[128]; static int sfill;
static int sec_no(int k) { return sbl[k / BLOCK_SECS] * BLOCK_SECS + (k % BLOCK_SECS); }
int mcard_rbegin(const char *name) {   // open a file to read: 1 found
    sbad = 0; sfill = 128; sdone = 0; mcard_last_err = 0;
    if (!dir_load()) { if (!mcard_last_err) mcard_last_err = MC_NOCARD; return 0; }
    int b = dir_find(name); if (!b) { mcard_last_err = MC_NOFILE; return 0; }
    snb = chain(b, sbl); return snb > 0;
}
void mcard_rget(void *out, int n) {
    u8 *o = (u8 *)out;
    while (n > 0) {
        if (sfill >= 128) {
            if (sdone >= snb * BLOCK_SECS || !mcard_read(sec_no(sdone), sbuf)) { sbad = 1; mcard_last_err = MC_READ; memset(sbuf, 0, 128); }
            sdone++; sfill = 0;
        }
        int c = MIN(n, 128 - sfill); memcpy(o, sbuf + sfill, c); o += c; sfill += c; n -= c;
    }
}
int mcard_rok(void) { return !sbad; }
int mcard_wbegin(const char *name, int nb) {   // open a file to write, nb blocks: 1 ok
    sbad = 0; sfill = 0; sdone = 0; mcard_last_err = 0;
    if (nb < 1 || nb > NBLK) { mcard_last_err = MC_TOOBIG; return 0; }
    if (!dir_load()) { if (!mcard_last_err) mcard_last_err = MC_NOCARD; return 0; }
    int first = dir_find(name); snb = 0; sfresh = 0;
    if (first) snb = chain(first, sbl);
    if (snb != nb) {   // new, or a different size: free the old, take fresh blocks (the directory is only written at the end)
        for (int k = 0; k < snb; k++) { dir[sbl[k]][0] = 0xA0; dir_fix(dir[sbl[k]]); }
        snb = 0;
        for (int b = 1; b <= NBLK && snb < nb; b++) if (dir_free(b)) sbl[snb++] = (u8)b;
        if (snb < nb) { mcard_last_err = MC_FULL; return 0; }
        sfresh = 1;
        for (int k = 0; k < nb; k++) {
            u8 *e = dir[sbl[k]]; memset(e, 0, 128);
            e[0] = (u8)(nb == 1 ? 0x51 : k == 0 ? 0x51 : k == nb - 1 ? 0x53 : 0x52);
            if (k == 0) { u32 sz = (u32)nb * 8192; e[4] = (u8)sz; e[5] = (u8)(sz >> 8); e[6] = (u8)(sz >> 16); e[7] = (u8)(sz >> 24); }
            int nx = k + 1 < nb ? sbl[k + 1] - 1 : 0xFFFF; e[8] = (u8)nx; e[9] = (u8)(nx >> 8);
            if (k == 0) strncpy((char *)&e[10], name, 20);
            dir_fix(e);
        }
    }
    return 1;
}
static void wflush(void) {
    if (!sfill) return;
    memset(sbuf + sfill, 0, 128 - sfill);
    if (sdone >= snb * BLOCK_SECS) { sbad = 1; mcard_last_err = MC_TOOBIG; }
    else if (!sbad && !mcard_write(sec_no(sdone), sbuf)) { sbad = 1; mcard_last_err = MC_WRITE; }
    sdone++; sfill = 0;
}
void mcard_wput(const void *in, int n) {
    const u8 *p = (const u8 *)in;
    while (n > 0) { int c = MIN(n, 128 - sfill); memcpy(sbuf + sfill, p, c); p += c; sfill += c; n -= c; if (sfill == 128) wflush(); }
}
int mcard_wpatch(int sector, const u8 *sec) { return mcard_write(sec_no(sector), sec); }   // rewrite one sector of the open file
int mcard_wend(void) {   // the last sector, then the directory (a save cut short is no save): 1 ok
    wflush();
    if (sbad) return 0;
    if (sfresh) for (int b = 1; b <= NBLK; b++) if (!mcard_write(b, dir[b])) { mcard_last_err = MC_WRITE; return 0; }   // (an old file kept its blocks: the directory is as it was)
    return 1;
}
// the first sector of a file: "SC", the icon frames, the number of blocks, the title in the card manager's Shift-JIS, the icon's
// 16 colours; the second: the icon, 16 x 16 pixels of 4 bits
void mcard_title_sector(u8 *sec, const char *title, int blocks, const u16 *pal) {
    memset(sec, 0, 128);
    sec[0] = 'S'; sec[1] = 'C'; sec[2] = 0x11; sec[3] = (u8)blocks;
    int o = 4;
    for (const char *p = title; *p && o + 2 <= 68; p++) {   // ASCII to full-width Shift-JIS (what the PlayStation's own browser shows)
        int c = *p, hi = 0x82, lo;
        if (c >= 'A' && c <= 'Z') lo = 0x60 + (c - 'A');
        else if (c >= 'a' && c <= 'z') lo = 0x81 + (c - 'a');
        else if (c >= '0' && c <= '9') lo = 0x4F + (c - '0');
        else if (c == ' ') { hi = 0x81; lo = 0x40; }
        else if (c == '-') { hi = 0x81; lo = 0x5B; }
        else if (c == '.') { hi = 0x81; lo = 0x44; }
        else if (c == '\'') { hi = 0x81; lo = 0x66; }
        else { hi = 0x81; lo = 0x40; }
        sec[o++] = (u8)hi; sec[o++] = (u8)lo;
    }
    for (int i = 0; i < 16; i++) { sec[96 + i * 2] = (u8)pal[i]; sec[97 + i * 2] = (u8)(pal[i] >> 8); }
}
