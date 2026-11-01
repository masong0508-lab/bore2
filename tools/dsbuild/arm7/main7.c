// ARM7 for BORE 2 (DS): the standard libnds ARM7 job - touch screen, buttons, clock, sound - so every emulator and real hardware get input and audio.
// The clock chip is read into the transfer region (unixTime, local time) at the start and once a second after (D44: Options > Game > Real time),
// by hand: libnds 1.8.2's rtcGetTimeAndDate came back empty in melonDS.
#include <nds.h>
void *memcpy(void *d, const void *s, unsigned n) { unsigned char *a = d; const unsigned char *b = s; while (n--) *a++ = *b++; return d; }
void *memset(void *d, int c, unsigned n) { unsigned char *a = d; while (n--) *a++ = (unsigned char)c; return d; }
int memcmp(const void *x, const void *y, unsigned n) { const unsigned char *a = x, *b = y; while (n--) { if (*a != *b) return *a - *b; a++; b++; } return 0; }
unsigned strlen(const char *s) { unsigned n = 0; while (s[n]) n++; return n; }
volatile unsigned vc_count, vb_count;

// the clock chip, read by hand (serial at 0x04000138: bit 0 data, 1 clock, 2 select, 4 data direction): 7 BCD bytes, year 0..99 (2000..2099)
static void rtc_wait(void) { for (volatile int i = 0; i < 40; i++) { } }
static void rtc_put(unsigned char v) { *(volatile unsigned char *)0x04000138 = v; rtc_wait(); }
static void rtc_read(unsigned char cmd, unsigned char *out, int n) {
    rtc_put(0x40 | 0x20 | 0x10 | 0x02 | 0x01); rtc_put(0x40 | 0x04 | 0x20 | 0x10 | 0x02 | 0x01);   // select
    for (int b = 7; b >= 0; b--) { int d = (cmd >> b) & 1; rtc_put(0x44 | 0x20 | 0x10 | d); rtc_put(0x44 | 0x20 | 0x10 | 0x02 | d); }   // the command, high bit first
    for (int i = 0; i < n; i++) { unsigned char v = 0; for (int b = 0; b < 8; b++) { rtc_put(0x44 | 0x20); rtc_put(0x44 | 0x20 | 0x02); if (*(volatile unsigned char *)0x04000138 & 1) v |= 1 << b; } out[i] = v; }
    rtc_put(0x40 | 0x20 | 0x02);
}
static void clock_read(void) {   // -> the transfer region's unixTime (0x02FFF010), local time: what libnds' time() reads
    unsigned char t[7], st; rtc_read(0x65, t, 7); rtc_read(0x61, &st, 1);
    int v[7]; for (int i = 0; i < 7; i++) v[i] = (t[i] & 15) + (t[i] >> 4 & 15) * 10;
    int y = v[0], mo = v[1], d = v[2], h = t[4] & 0x3f, mi = v[5], s = v[6]; h = (h & 15) + (h >> 4 & 15) * 10; if (!(st & 2) && (t[4] & 0x40)) h += 12;
    if (mo < 1 || mo > 12 || d < 1 || d > 31 || h > 23 || mi > 59) return;
    static const short yd[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    unsigned days = 10957u + 365u * (unsigned)y + (unsigned)((y + 3) / 4) + (unsigned)yd[mo - 1] + (unsigned)(mo > 2 && !(y & 3)) + (unsigned)(d - 1);
    *(volatile unsigned *)0x02FFF010 = days * 86400u + (unsigned)(h * 3600 + mi * 60 + s);
}
void VblankHandler(void) { vb_count++; }
void VcountHandler(void) { static unsigned k; vc_count++; inputGetAndSend(); if (++k >= 60) { k = 0; clock_read(); } }   // (the clock once a second; the vcount interrupt is the one that runs here)
int main(void) {
    readUserSettings(); ledBlink(0);
    irqInit(); fifoInit(); touchInit();
    irqSet(IRQ_VCOUNT, VcountHandler); irqSet(IRQ_VBLANK, VblankHandler);
    irqEnable(IRQ_VBLANK | IRQ_VCOUNT);
    SetYtrigger(80); clock_read();
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
