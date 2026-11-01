// dc_cd.c: the PS1's CD calls (psxcd) for the sound code, on the Dreamcast's disc. The PS1 game's files (SONGS.BIN, VOICES.BIN,
// GOTCHI.ART) sit in the root of the disc's ISO9660 file system, in the data track the game boots from (the high-density area of
// a GD-ROM, or the last data track of a CD). The root directory is read once; a file's position is its real sector, so the PS1
// code's sector arithmetic (CdPosToInt + offset) works unchanged. Reads run on a thread of their own: CdReadSync(1) says whether
// one is still going, so the streamed songs are fed from the disc without stopping the game.
#include <kos.h>
#include <dc/cdrom.h>
#include <string.h>
#include <stdint.h>
#include <psxcd.h>

#define NFILE 32
static struct { char name[16]; int lba, size; } files[NFILE];
static int nfiles, cur_lba, cd_ready;
static volatile int rd_busy, rd_err;
static struct { int lba, n; void *buf; } rq;
static semaphore_t rd_go; static int rd_up;
static uint8_t sec[2048] __attribute__((aligned(32)));

static int rd_sectors(void *buf, int lba, int n) { return cdrom_read_sectors(buf, lba + 150, n) == 0; }
static void *rd_thread(void *p) {
    (void)p;
    while (1) {
        sem_wait(&rd_go);
        rd_err = !rd_sectors(rq.buf, rq.lba, rq.n);
        rd_busy = 0;
    }
    return 0;
}
static void dir_scan(void) {   // the root directory of the boot track's file system
    cd_toc_t toc; uint32_t fad = 0;
    if (cdrom_read_toc(&toc, true) == 0) fad = cdrom_locate_data_track(&toc);   // a GD-ROM: its high-density area
    if (!fad && cdrom_read_toc(&toc, false) == 0) fad = cdrom_locate_data_track(&toc);
    if (!fad || !rd_sectors(sec, (int)fad - 150 + 16, 1) || memcmp(sec, "\1CD001", 6)) return;
    int rlba = sec[156 + 2] | sec[156 + 3] << 8 | sec[156 + 4] << 16 | sec[156 + 5] << 24;
    int rsize = sec[156 + 10] | sec[156 + 11] << 8 | sec[156 + 12] << 16 | sec[156 + 13] << 24;
    for (int s = 0; s < (rsize + 2047) / 2048 && s < 8; s++) {
        if (!rd_sectors(sec, rlba + s, 1)) return;
        for (int o = 0; o < 2048 && sec[o] && nfiles < NFILE; o += sec[o]) {
            const uint8_t *r = sec + o; int nl = r[32];
            if (r[25] & 2) continue;                     // (a directory)
            char n[16]; int k = 0;
            for (int i = 0; i < nl && k < 15 && r[33 + i] != ';'; i++) n[k++] = (char)r[33 + i];
            n[k] = 0;
            strcpy(files[nfiles].name, n);
            files[nfiles].lba = r[2] | r[3] << 8 | r[4] << 16 | r[5] << 24;
            files[nfiles].size = r[10] | r[11] << 8 | r[12] << 16 | r[13] << 24;
            nfiles++;
        }
    }
    cd_ready = 1;
}
int CdInit(void) {
    if (!rd_up) { sem_init(&rd_go, 0); kthread_attr_t at = { 0 }; at.prio = 8; at.label = "cdread"; thd_create_ex(&at, rd_thread, 0); rd_up = 1; }
    if (!cd_ready) dir_scan();
    return 1;
}
CdlLOC *CdIntToPos(int i, CdlLOC *p) {
    i += 150;
    int m = i / 4500, s = (i / 75) % 60, f = i % 75;
    p->minute = (uint8_t)(((m / 10) << 4) | (m % 10)); p->second = (uint8_t)(((s / 10) << 4) | (s % 10)); p->sector = (uint8_t)(((f / 10) << 4) | (f % 10)); p->track = 0;
    return p;
}
int CdPosToInt(const CdlLOC *p) {
    int m = (p->minute >> 4) * 10 + (p->minute & 15), s = (p->second >> 4) * 10 + (p->second & 15), f = (p->sector >> 4) * 10 + (p->sector & 15);
    return m * 4500 + s * 75 + f - 150;
}
CdlFILE *CdSearchFile(CdlFILE *f, const char *name) {
    char n[16]; int k = 0;
    while (*name == '\\' || *name == '/') name++;
    while (*name && *name != ';' && k < 15) n[k++] = *name++;
    n[k] = 0;
    for (int i = 0; i < nfiles; i++) if (!strcmp(files[i].name, n)) {
        CdIntToPos(files[i].lba, &f->pos); f->size = files[i].size; strncpy(f->name, n, sizeof f->name);
        return f;
    }
    return 0;
}
int CdControl(CdlCommand cmd, const void *param, uint8_t *result) {
    if (result) result[0] = 0x02;
    if (cmd == CdlSetloc && param) cur_lba = CdPosToInt((const CdlLOC *)param);
    return 1;
}
int CdControlB(CdlCommand cmd, const void *param, uint8_t *result) { return CdControl(cmd, param, result); }
int CdControlF(CdlCommand cmd, const void *param) { return CdControl(cmd, param, 0); }
int CdRead(int sectors, uint32_t *buf, int mode) {
    (void)mode;
    if (rd_busy || !cd_ready) return 0;
    rq.lba = cur_lba; rq.n = sectors; rq.buf = buf;
    cur_lba += sectors;
    rd_err = 0; rd_busy = 1; sem_signal(&rd_go);
    return 1;
}
int CdReadSync(int mode, uint8_t *result) {
    (void)result;
    if (mode == 0) while (rd_busy) thd_pass();
    if (rd_busy) return rq.n;
    return rd_err ? -1 : 0;
}
int dc_cd_files(void) { return nfiles; }
