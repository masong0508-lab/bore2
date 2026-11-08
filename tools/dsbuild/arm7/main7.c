// ARM7 for BORE 2 (DS): the standard libnds ARM7 job - touch screen, buttons, clock, sound - so every emulator and real hardware get input and audio.
// The clock chip is read into the transfer region (unixTime, local time) at the start and once a second after (D44: Options > Game > Real time),
// by hand: libnds 1.8.2's rtcGetTimeAndDate came back empty in melonDS.
// D72: the SONGS are played here (the song player below): the ARM9 only hands over a note list, so the music's tempo and grid do not depend on its frame rate.
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
// ---- the song player (D72): a note list (tools/dsmusic/mk_dsmusic.py, BDM2) played on ten sample channels by a 1022.76 Hz timer ----
// The ARM9 puts a MusCtl in main memory and sends its address once (FIFO_USER_01). The first eight words are written by the ARM9 (a new `gen` = a
// new command: run 0 stops, run 1 starts the list at `seq`), the rest by this side (laps played). Time is the count of timer 1, which counts timer 0's
// overflows (a cascade: no tick is ever lost, even when an interrupt is late: the player catches up) and every note is placed by its tick, so nothing drifts.
// The list: [delta ticks][event]...: a delta of 255 means 254 more ticks and another delta; an event is 0xFE (the loop point, nothing to do),
// 0xFF (the end: on to `loop_off`, laps + 1), 0x80|slot (let that voice go) or slot|0x40 (cut it first) then instrument, Hz (16 bits), volume, pan.
typedef struct { volatile unsigned gen, run, seq, loop_off, smp, tbl, vol, stereo; volatile unsigned laps, ticks, pos, pad[5]; } MusCtl;
static MusCtl *mc; static unsigned m_gen, m_due, m_now; static int m_run; static const unsigned char *mp; static unsigned short m_last;
static const unsigned char SLOTCH[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 14, 15 };   // (the square-wave voices of the game's own sounds keep 8-13)
static unsigned rd32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24; }
static unsigned m_delta(void) { unsigned t = 0; for (;;) { unsigned d = *mp++; if (d == 255) { t += 254; continue; } return t + d; } }
static void m_off(void) { for (int i = 0; i < 10; i++) SCHANNEL_CR(SLOTCH[i]) = 0; }
static void m_play(const unsigned char *e, int slot) {
    MusCtl *c = mc; int ch = SLOTCH[slot % 10]; unsigned in = e[0], hz = e[1] | e[2] << 8, vol = e[3], pan = e[4];
    unsigned v = vol * c->vol / 72; if (v > 127) v = 127;
    const unsigned char *ti = (const unsigned char *)c->tbl + in * 8; unsigned so = rd32(ti), sz = rd32(ti + 4);
    SCHANNEL_CR(ch) = 0;                                   // (a note cuts the one before it on its voice)
    if (v < 1 || !sz || hz < 256) return;
    unsigned tm = (16756991u + hz / 2) / hz; if (tm > 0xFFFF) tm = 0xFFFF;   // (the chip's own clock, 16756991 Hz: libnds' SOUND_FREQ uses 2^24, 0.12 % sharp)
    SCHANNEL_SOURCE(ch) = c->smp + so; SCHANNEL_REPEAT_POINT(ch) = 0; SCHANNEL_LENGTH(ch) = sz >> 2; SCHANNEL_TIMER(ch) = (unsigned short)(0x10000 - tm);
    SCHANNEL_CR(ch) = SCHANNEL_ENABLE | SOUND_VOL(v) | SOUND_PAN(c->stereo ? pan : 64) | (2u << 29) | SOUND_ONE_SHOT;
}
void MusTimer(void) {
    MusCtl *c = mc; if (!c) return;
    if (c->gen != m_gen) {   // a command from the ARM9
        m_gen = c->gen; m_off(); m_run = (int)c->run;
        if (m_run) { mp = (const unsigned char *)c->seq; m_due = m_delta(); m_now = 0; m_last = TIMER_DATA(1); c->laps = 0; }
    }
    if (!m_run) return;
    unsigned short cur = TIMER_DATA(1); m_now += (unsigned short)(cur - m_last); m_last = cur;   // (the ticks since last time: one, or more if an interrupt came late)
    c->ticks = m_now;
    while (m_due <= m_now) {
        int ev = *mp++;
        if (ev == 0xFF) { c->laps++; mp = (const unsigned char *)c->seq + c->loop_off; }
        else if (ev == 0xFE) { }
        else if (ev & 0x80) SCHANNEL_CR(SLOTCH[(ev & 15) % 10]) = 0;
        else { m_play(mp, ev & 15); mp += 5; }
        m_due += m_delta();
    }
}
static void MusAddr(unsigned addr, void *u) { (void)u; mc = (MusCtl *)addr; }
static void mus_setup(void) {
    fifoSetValue32Handler(FIFO_USER_01, MusAddr, 0);
    TIMER_DATA(1) = 0; TIMER_CR(1) = TIMER_ENABLE | TIMER_CASCADE;
    TIMER_DATA(0) = (unsigned short)(0x10000 - 512); TIMER_CR(0) = TIMER_ENABLE | TIMER_DIV_64 | TIMER_IRQ_REQ;
    irqSet(IRQ_TIMER0, MusTimer); irqEnable(IRQ_TIMER0);
}
static volatile int clk_due;
void VblankHandler(void) { vb_count++; }
void VcountHandler(void) { static unsigned k; vc_count++; inputGetAndSend(); if (++k >= 60) { k = 0; clk_due = 1; } }   // (the clock once a second, read in the main loop: it takes about a millisecond, so not with the interrupts shut)
int main(void) {
    readUserSettings(); ledBlink(0);
    irqInit(); fifoInit(); touchInit();
    irqSet(IRQ_VCOUNT, VcountHandler); irqSet(IRQ_VBLANK, VblankHandler);
    irqEnable(IRQ_VBLANK | IRQ_VCOUNT);
    SetYtrigger(80); clock_read();
    installSoundFIFO(); installSystemFIFO(); mus_setup();
    while (1) { swiWaitForVBlank(); if (clk_due) { clk_due = 0; clock_read(); } }
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
