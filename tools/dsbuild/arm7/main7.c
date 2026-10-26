// ARM7 for BORE 2 (DS): the standard libnds ARM7 job - touch screen, buttons, clock, sound - so every emulator and real hardware get input and audio.
#include <nds.h>
void *memcpy(void *d, const void *s, unsigned n) { unsigned char *a = d; const unsigned char *b = s; while (n--) *a++ = *b++; return d; }
void *memset(void *d, int c, unsigned n) { unsigned char *a = d; while (n--) *a++ = (unsigned char)c; return d; }
int memcmp(const void *x, const void *y, unsigned n) { const unsigned char *a = x, *b = y; while (n--) { if (*a != *b) return *a - *b; a++; b++; } return 0; }
unsigned strlen(const char *s) { unsigned n = 0; while (s[n]) n++; return n; }
volatile unsigned vc_count, vb_count;
void VcountHandler(void) { vc_count++; inputGetAndSend(); }
void VblankHandler(void) { vb_count++; }
int main(void) {
    readUserSettings(); ledBlink(0);
    irqInit(); fifoInit(); touchInit();
    irqSet(IRQ_VCOUNT, VcountHandler); irqSet(IRQ_VBLANK, VblankHandler);
    irqEnable(IRQ_VBLANK | IRQ_VCOUNT);
    SetYtrigger(80);
    installSoundFIFO(); installSystemFIFO();
    while (1) swiWaitForVBlank();
}
void __aeabi_memcpy(void *d, const void *s, unsigned n) { memcpy(d, s, n); }
void __aeabi_memcpy4(void *d, const void *s, unsigned n) { memcpy(d, s, n); }
void __aeabi_memcpy8(void *d, const void *s, unsigned n) { memcpy(d, s, n); }
void __aeabi_memset(void *d, unsigned n, int c) { memset(d, c, n); }
void __aeabi_memset4(void *d, unsigned n, int c) { memset(d, c, n); }
void __aeabi_memset8(void *d, unsigned n, int c) { memset(d, c, n); }
void __aeabi_memclr(void *d, unsigned n) { memset(d, 0, n); }
void __aeabi_memclr4(void *d, unsigned n) { memset(d, 0, n); }
void __aeabi_memclr8(void *d, unsigned n) { memset(d, 0, n); }
void __libc_init_array(void) { }
int abs(int x) { return x < 0 ? -x : x; }
unsigned __aeabi_uidiv(unsigned n, unsigned d) { unsigned q = 0, r = 0; if (!d) return 0; for (int i = 31; i >= 0; i--) { r = (r << 1) | ((n >> i) & 1); if (r >= d) { r -= d; q |= 1u << i; } } return q; }
int __aeabi_idiv(int n, int d) { int neg = (n < 0) ^ (d < 0); unsigned q = __aeabi_uidiv((unsigned)(n < 0 ? -n : n), (unsigned)(d < 0 ? -d : d)); return neg ? -(int)q : (int)q; }
