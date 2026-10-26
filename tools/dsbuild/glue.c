// glue between picolibc and libnds (DS ARM9): stdout goes to libnds' console, malloc gets its memory from libnds' heap
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/iosupport.h>
#include <nds.h>
const devoptab_t *devoptab_list[4];
int AddDevice(const devoptab_t *d) { (void)d; return 0; }
static int con_putc(char c, FILE *f) { (void)f; const devoptab_t *d = devoptab_list[STD_OUT]; if (d && d->write_r) d->write_r(0, 0, &c, 1); return (unsigned char)c; }
static FILE con_file = FDEV_SETUP_STREAM(con_putc, NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const stdout = &con_file; FILE *const stderr = &con_file;
static int nul_getc(FILE *f) { (void)f; return -1; }
static FILE nul_file = FDEV_SETUP_STREAM(NULL, nul_getc, NULL, _FDEV_SETUP_READ);
FILE *const stdin = &nul_file;
void nocashWrite(const char *s, int n) { (void)s; (void)n; }
void nocashMessage(const char *s) { (void)s; }
extern char *fake_heap_end; extern char *fake_heap_start;
void *sbrk(ptrdiff_t inc) { static char *cur; if (!cur) cur = getHeapStart(); char *lim = getHeapLimit(), *old = cur; if (cur + inc > lim) return (void *)-1; cur += inc; return old; }
void _exit(int rc) { extern void __libnds_exit(int); __libnds_exit(rc); while (1); }
void build_argv(void *a) { (void)a; }
