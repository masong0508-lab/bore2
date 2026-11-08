// dc_spu.c: the PS1 sound chip (SPU) in software, mixed into one 44.1 kHz stereo stream on the Dreamcast's AICA.
// The game's own sound code (src/audio.c) runs unchanged: it writes the SPU's registers (here a plain array, dc_iobase) and its
// 512 KB of sound memory (SpuWrite), and the 480 Hz timer interrupt that walks the songs' rows is called from the mixer, every
// 44100/480 samples, so the music keeps the PS1's timing exactly whatever the frame rate.
// What is emulated: 24 voices of ADPCM with the gaussian interpolation, the ADSR envelopes, loop and IRQ addresses (the streamed
// songs), voice and master volumes, and the reverb (the psx-spx algorithm, at 22.05 kHz like the chip).
// Writes to the key-on / key-off registers are events (several in one tick must all count): psx/hwregs_c.h turns them into
// dc_spu_kreg() for audio.c. Every audio.c call is taken under the mixer's lock (dc_audio_wrap.h), so the two never interleave.
#include <kos.h>
#include <dc/sound/stream.h>
#include <dc/sound/sound.h>
#include <string.h>
#include <stdint.h>
#include "common.h"
#include <hwregs_c.h>
#include <psxspu.h>
#include <psxetc.h>
#include "dc_spu_gauss.h"

#define RATE 44100
#define NV 24
#define R16(off) (*(volatile uint16_t *)(dc_iobase + 0x1c00 + (off)))
#define VREG(v, k) R16((v) * 16 + (k))       // k: 0 vol L, 2 vol R, 4 pitch, 6 start, 8 ADSR1, 10 ADSR2, 12 ADSR vol, 14 loop
static uint8_t spu_ram[512 * 1024] __attribute__((aligned(32)));

// ---------------------------------------------------------------- the lock (audio.c's calls vs the mixer thread)
static mutex_t snd_mtx; static int snd_mtx_ok;
static void snd_lock(void) { if (!snd_mtx_ok) { mutex_init(&snd_mtx, MUTEX_TYPE_RECURSIVE); snd_mtx_ok = 1; } mutex_lock(&snd_mtx); }
static void snd_unlock(void) { mutex_unlock(&snd_mtx); }
#include "dc_audio_wrap.h"

// ---------------------------------------------------------------- key writes as events
static struct { uint16_t reg, val; } kev[512]; static int kev_n; static uint16_t kev_spill;
volatile uint16_t *dc_spu_kreg(int r) { if (kev_n >= 512) return &kev_spill; kev[kev_n].reg = (uint16_t)r; kev[kev_n].val = 0; return &kev[kev_n++].val; }

// ---------------------------------------------------------------- the callbacks audio.c sets (the PS1's interrupts)
static void (*cb_timer2)(void), (*cb_spu)(void), (*cb_dma_spu)(void);
void *InterruptCallback(IRQ_Channel irq, void (*func)(void)) {
    void *old = 0;
    if (irq == IRQ_TIMER2) { old = (void *)cb_timer2; cb_timer2 = func; }
    else if (irq == IRQ_SPU) { old = (void *)cb_spu; cb_spu = func; }
    return old;
}
void *DMACallback(DMA_Channel dma, void (*func)(void)) { void *old = 0; if (dma == DMA_SPU) { old = (void *)cb_dma_spu; cb_dma_spu = func; } return old; }

// ---------------------------------------------------------------- transfers into sound memory
static uint32_t xfer_addr = 0x1000;
static SPU_TransferMode xfer_mode = SPU_TRANSFER_BY_DMA;
SPU_TransferMode SpuSetTransferMode(SPU_TransferMode m) { SPU_TransferMode o = xfer_mode; xfer_mode = m; return o; }
SPU_TransferMode SpuGetTransferMode(void) { return xfer_mode; }
uint32_t SpuSetTransferStartAddr(uint32_t addr) { if (addr > 0x7ffff) return 0; xfer_addr = addr & ~7u; return addr; }
uint32_t SpuGetTransferStartAddr(void) { return xfer_addr; }
size_t SpuWrite(const uint32_t *data, size_t size) {
    if (xfer_addr + size > sizeof spu_ram) size = sizeof spu_ram - xfer_addr;
    memcpy(spu_ram + xfer_addr, data, size);
    if (cb_dma_spu) cb_dma_spu();   // (the DMA is done at once)
    return size;
}
size_t SpuWritePartly(const uint32_t *data, size_t size) { return SpuWrite(data, size); }
size_t SpuRead(uint32_t *data, size_t size) { if (xfer_addr + size > sizeof spu_ram) size = sizeof spu_ram - xfer_addr; memcpy(data, spu_ram + xfer_addr, size); return size; }
int SpuIsTransferCompleted(int mode) { (void)mode; return 1; }

// ---------------------------------------------------------------- the voices
enum { OFF, ATTACK, DECAY, SUSTAIN, RELEASE };
typedef struct {
    int phase, env, ectr, erate, edec, eexp, etarget, eticks, estep, eidle;
    uint32_t addr, ctr;                 // the block being played (byte address); 12-bit fraction, sample index from bit 12
    int16_t s[3 + 28]; int h1, h2;      // the last 3 samples of the block before, then this block's 28; the ADPCM history
    uint8_t flags; int first, ignore_loop; uint16_t loop_seen;
} Voice;
static Voice vc[NV];

static inline void env_step(int rate, int dec, int *ticks, int *step) {
    if (rate < 48) { *ticks = 1; *step = dec ? (-8 + (rate & 3)) * (1 << (11 - (rate >> 2))) : (7 - (rate & 3)) << (11 - (rate >> 2)); }
    else { *ticks = 1 << ((rate >> 2) - 11); *step = dec ? -8 + (rate & 3) : 7 - (rate & 3); }
}
static void env_set(Voice *v, int rate, int dec, int exp, int target) {
    v->erate = rate; v->edec = dec; v->eexp = exp; v->etarget = target;
    int t, s; env_step(rate, dec, &t, &s); v->ectr = t; v->eticks = t; v->estep = s;
    v->eidle = 0;
}
static void phase_set(Voice *v, int vi, int ph) {
    uint16_t a1 = VREG(vi, 8), a2 = VREG(vi, 10);
    v->phase = ph;
    switch (ph) {
        case ATTACK: env_set(v, (a1 >> 8) & 0x7f, 0, a1 >> 15, 32767); break;
        case DECAY: { int t = ((a1 & 15) + 1) * 0x800; if (t > 32767) t = 32767; env_set(v, ((a1 >> 4) & 15) << 2, 1, 1, t); break; }
        case SUSTAIN: env_set(v, (a2 >> 6) & 0x7f, (a2 >> 14) & 1, a2 >> 15, 0); break;
        case RELEASE: env_set(v, (a2 & 31) << 2, 1, (a2 >> 5) & 1, 0); break;
        default: v->env = 0; break;
    }
}
static void env_tick_slow(Voice *v, int vi) {
    int s = v->estep; v->ectr = v->eticks;
    if (v->eexp) {
        if (v->edec) s = (s * v->env) >> 15;
        else if (v->env >= 0x6000) { if (v->erate < 40) s >>= 2; else if (v->erate >= 44) v->ectr >>= 2; else { s >>= 1; v->ectr >>= 1; } }
    }
    int e = v->env + s; v->env = e < 0 ? 0 : e > 32767 ? 32767 : e;
    if (v->phase == SUSTAIN) { if (v->edec ? v->env == 0 : v->env == 32767) v->eidle = 1; return; }   // (held at the top or the bottom: nothing more to do)
    if (v->edec ? v->env <= v->etarget : v->env >= v->etarget) {
        if (v->phase == ATTACK) phase_set(v, vi, DECAY);
        else if (v->phase == DECAY) phase_set(v, vi, SUSTAIN);
        else if (v->phase == RELEASE) { v->phase = OFF; v->env = 0; }
    }
}
static inline void env_tick(Voice *v, int vi) { if (v->eidle) return; if (--v->ectr > 0) return; env_tick_slow(v, vi); }
static void block_decode(Voice *v, int vi) {
    static const int fp[5] = { 0, 60, 115, 98, 122 }, fn[5] = { 0, 0, -52, -55, -60 };
    uint32_t a = v->addr & 0x7fff0;
    uint16_t irq = R16(0x1a4);   // SPU_IRQ_ADDR (8-byte units)
    if ((R16(0x1aa) & 0x40) && cb_spu) { uint32_t ia = (uint32_t)irq * 8; if (ia >= a && ia < a + 16) cb_spu(); }   // the IRQ: a voice reads its address
    const uint8_t *b = spu_ram + a;
    int sh = b[0] & 15, fi = (b[0] >> 4) & 7; if (sh > 12) sh = 9; if (fi > 4) fi = 4;
    v->flags = b[1];
    v->s[0] = v->s[28]; v->s[1] = v->s[29]; v->s[2] = v->s[30];
    int h1 = v->h1, h2 = v->h2, p = fp[fi], n = fn[fi];
    for (int i = 0; i < 28; i++) {
        int nib = (b[2 + (i >> 1)] >> ((i & 1) * 4)) & 15;
        int smp = (int16_t)(nib << 12) >> sh;
        smp += (h1 * p) >> 6; smp += (h2 * n) >> 6;
        if (smp > 32767) smp = 32767; if (smp < -32768) smp = -32768;
        h2 = h1; h1 = smp; v->s[3 + i] = (int16_t)smp;
    }
    v->h1 = h1; v->h2 = h2;
    if (v->flags & 4) {   // loop start: the repeat address is here (unless the game set it itself while the voice played)
        uint16_t lr = VREG(vi, 14);
        if (lr != v->loop_seen && !v->first) v->ignore_loop = 1;
        if (!v->ignore_loop) { VREG(vi, 14) = (uint16_t)(a >> 3); }
        v->loop_seen = VREG(vi, 14);
    }
}
static void key_on(int vi) {
    Voice *v = &vc[vi];
    v->addr = (uint32_t)VREG(vi, 6) * 8; v->ctr = 0; v->env = 0; v->h1 = v->h2 = 0;
    memset(v->s, 0, sizeof v->s); v->first = 1; v->ignore_loop = 0; v->loop_seen = VREG(vi, 14);
    phase_set(v, vi, ATTACK);
    block_decode(v, vi);
}
static void key_off(int vi) { Voice *v = &vc[vi]; if (v->phase != OFF && v->phase != RELEASE) phase_set(v, vi, RELEASE); }
static void events_run(void) {
    for (int i = 0; i < kev_n; i++) {
        uint32_t bits = kev[i].val; int r = kev[i].reg;
        if (r == 0 || r == 2) bits <<= (r == 2 ? 0 : 0);
        int base = (r == 1 || r == 3) ? 16 : 0;
        for (int k = 0; k < 16 && base + k < NV; k++) if (bits & (1u << k)) { if (r < 2) key_on(base + k); else key_off(base + k); }
    }
    kev_n = 0;
}
static inline int vol16(uint16_t r) { return (r & 0x8000) ? 0x7fff : (int)(int16_t)(r << 1); }   // fixed volume (sweeps are not used by the game)

// ---------------------------------------------------------------- reverb (psx-spx), every other sample
static int rev_cur, rev_inL, rev_inR, rev_outL, rev_outR, rev_odd;
static inline int16_t *rv(int base, int off) {
    uint32_t o = (uint32_t)(rev_cur + (off & 0x3ffff)); if (o & 0x40000) o = (o & 0x3ffff) + (uint32_t)base; o &= 0x3ffff;
    return (int16_t *)spu_ram + o;
}
static inline int sat(int x) { return x > 32767 ? 32767 : x < -32768 ? -32768 : x; }
static int rcA[32], rcV[32], rcM[4], rcD[4], rc_base, rc_ring, rc_wr, rc_vl, rc_vr;
static void reverb_cache(void) {   // the reverb registers only change at a tick: read (and turned into ring offsets) once per stretch
    const volatile int16_t *R = (const volatile int16_t *)(dc_iobase + 0x1dc0);
    for (int i = 0; i < 32; i++) { rcV[i] = (int)R[i]; rcA[i] = ((int)(uint16_t)R[i] * 4) & 0x3ffff; }
    for (int k = 0; k < 4; k++) rcM[k] = (rcA[k < 2 ? 10 + k : 16 + k] - 1) & 0x3ffff;      // taps 10, 11, 18, 19 minus one
    rcD[0] = (rcA[26] - rcA[0]) & 0x3ffff; rcD[1] = (rcA[27] - rcA[0]) & 0x3ffff; rcD[2] = (rcA[28] - rcA[1]) & 0x3ffff; rcD[3] = (rcA[29] - rcA[1]) & 0x3ffff;
    rc_base = (int)((uint32_t)R16(0x1a2) * 4) & 0x3ffff; rc_ring = 0x40000 - rc_base;
    rc_wr = R16(0x1aa) & 0x80; rc_vl = vol16(R16(0x184)); rc_vr = vol16(R16(0x186));
}
static void reverb_step(int inL, int inR) {
    int16_t *const ram = (int16_t *)spu_ram;
    if (rev_cur < rc_base) rev_cur = rc_base;
    const int cur = rev_cur, ring = rc_ring;
#define P(off) (ram + ({ int o_ = cur + (off); (o_ >= 0x40000 ? o_ - ring : o_) & 0x3ffff; }))
#define A(i) rcA[i]
#define V(i) rcV[i]
    int Lin = (V(30) * inL) >> 15, Rin = (V(31) * inR) >> 15;
    int vIIR = V(2), vWALL = V(7);
    if (rc_wr) {
        int16_t *p;
        int ls = *P(rcM[0]), rs = *P(rcM[1]), ld = *P(rcM[2]), rdd = *P(rcM[3]);
        p = P(A(10)); *p = (int16_t)sat((((Lin + ((*P(A(16)) * vWALL) >> 15) - ls) * vIIR) >> 15) + ls);
        p = P(A(11)); *p = (int16_t)sat((((Rin + ((*P(A(17)) * vWALL) >> 15) - rs) * vIIR) >> 15) + rs);
        p = P(A(18)); *p = (int16_t)sat((((Lin + ((*P(A(25)) * vWALL) >> 15) - ld) * vIIR) >> 15) + ld);
        p = P(A(19)); *p = (int16_t)sat((((Rin + ((*P(A(24)) * vWALL) >> 15) - rdd) * vIIR) >> 15) + rdd);
    }
    int Lo = (V(3) * *P(A(12)) + V(4) * *P(A(14)) + V(5) * *P(A(20)) + V(6) * *P(A(22))) >> 15;
    int Ro = (V(3) * *P(A(13)) + V(4) * *P(A(15)) + V(5) * *P(A(21)) + V(6) * *P(A(23))) >> 15;
    int a1 = V(8), a2 = V(9), t;
    t = *P(rcD[0]); Lo = sat(Lo - ((a1 * t) >> 15)); if (rc_wr) *P(A(26)) = (int16_t)Lo; Lo = sat(((Lo * a1) >> 15) + t);
    t = *P(rcD[1]); Ro = sat(Ro - ((a1 * t) >> 15)); if (rc_wr) *P(A(27)) = (int16_t)Ro; Ro = sat(((Ro * a1) >> 15) + t);
    t = *P(rcD[2]); Lo = sat(Lo - ((a2 * t) >> 15)); if (rc_wr) *P(A(28)) = (int16_t)Lo; Lo = sat(((Lo * a2) >> 15) + t);
    t = *P(rcD[3]); Ro = sat(Ro - ((a2 * t) >> 15)); if (rc_wr) *P(A(29)) = (int16_t)Ro; Ro = sat(((Ro * a2) >> 15) + t);
#undef P
#undef A
#undef V
    rev_outL = (Lo * rc_vl) >> 15; rev_outR = (Ro * rc_vr) >> 15;
    if (++rev_cur >= 0x40000) rev_cur = rc_base;
}

// ---------------------------------------------------------------- the mixer
// Mixed in stretches between two of the sequencer's ticks (registers only change at a tick or an event), voice by voice into
// 32-bit sums. Within a stretch a voice runs in runs: between two envelope steps and block ends nothing changes but the
// sample position, so the inner loop is only the gaussian interpolation and the two volumes.
static int tick_acc;
int dc_spu_voices_on, dc_spu_peak;                 // (readouts: voices sounding, the loudest sample of the last buffer)
static int32_t sumL[512], sumR[512], rvL[512], rvR[512];
static inline uint32_t run(const int16_t *restrict s0, uint32_t ctr, int step, int gl, int gr, int32_t *restrict dl, int32_t *restrict dr,
                           int32_t *restrict el, int32_t *restrict er, int n, int rev) {
    for (int i = 0; i < n; i++) {
        int si = (int)(ctr >> 12), ii = (int)((ctr >> 4) & 255);
        const int16_t *s = s0 + si;
        int smp = (spu_gauss[0xff - ii] * s[0] + spu_gauss[0x1ff - ii] * s[1] + spu_gauss[0x100 + ii] * s[2] + spu_gauss[ii] * s[3]) >> 15;
        int l = (smp * gl) >> 15, r = (smp * gr) >> 15;
        dl[i] += l; dr[i] += r;
        if (rev) { el[i] += l; er[i] += r; }
        ctr += (uint32_t)step;
    }
    return ctr;
}
static int16_t last_hl, last_hr;
static void mix_seg(int16_t *out, int nf) {
    int n = (nf + 1) >> 1;   // mixed at half rate (22.05 kHz, the PS1's own sample rates), then interpolated up: half the work
    reverb_cache();
    memset(sumL, 0, n * 4); memset(sumR, 0, n * 4);
    uint32_t ron = R16(0x198) | (uint32_t)R16(0x19a) << 16; int any_rev = 0;
    for (int vi = 0; vi < NV; vi++) {
        Voice *v = &vc[vi];
        if (v->phase == OFF) continue;
        dc_spu_voices_on++;
        int vl = vol16(VREG(vi, 0)), vr = vol16(VREG(vi, 2)), step = VREG(vi, 4); if (step > 0x3fff) step = 0x3fff; step *= 2;
        int rev = (ron >> vi) & 1;
        if (rev && !any_rev) { memset(rvL, 0, n * 4); memset(rvR, 0, n * 4); any_rev = 1; }
        for (int i = 0; i < n && v->phase != OFF;) {
            // how far before something changes: the envelope's next step, the end of the block
            int k = n - i;
            if (!v->eidle && v->ectr < 2 * k) k = (v->ectr + 1) >> 1;
            if (step > 0) { int to_end = (int)((((28u << 12) - v->ctr) + (uint32_t)step - 1) / (uint32_t)step); if (to_end < k) k = to_end; }
            if (k < 1) k = 1;
            int gl = (v->env * vl) >> 15, gr = (v->env * vr) >> 15;
            if (v->env) v->ctr = rev ? run(&v->s[0], v->ctr, step, gl, gr, sumL + i, sumR + i, rvL + i, rvR + i, k, 1)
                                     : run(&v->s[0], v->ctr, step, gl, gr, sumL + i, sumR + i, 0, 0, k, 0);
            else v->ctr += (uint32_t)step * (uint32_t)k;
            i += k;
            if (!v->eidle) { v->ectr -= 2 * k - 1; env_tick(v, vi); }   // (env_tick counts the last sample of the run)
            if ((v->ctr >> 12) >= 28) {
                v->ctr -= 28 << 12; v->addr += 16; v->first = 0;
                if (v->flags & 1) {   // loop end: to the repeat address; without the repeat flag the voice stops
                    v->addr = (uint32_t)VREG(vi, 14) * 8;
                    if (!(v->flags & 2)) { v->phase = OFF; v->env = 0; }
                }
                if (v->phase != OFF) block_decode(v, vi);
            }
        }
    }
    int ml = vol16(R16(0x180)), mr = vol16(R16(0x182)), peak = dc_spu_peak;
    int16_t pl = last_hl, pr = last_hr;
    for (int j = 0; j < n; j++) {
        if (any_rev) reverb_step(sat(rvL[j]), sat(rvR[j])); else reverb_step(0, 0);
        int L = sat(sumL[j] + rev_outL), R = sat(sumR[j] + rev_outR);
        L = (L * ml) >> 15; R = (R * mr) >> 15;
        int mL = (pl + L) >> 1, mR = (pr + R) >> 1;   // between the last and this half-rate sample
        out[j * 4] = (int16_t)mL; out[j * 4 + 1] = (int16_t)mR;
        if (j * 2 + 1 < nf) { out[j * 4 + 2] = (int16_t)L; out[j * 4 + 3] = (int16_t)R; }
        pl = (int16_t)L; pr = (int16_t)R;
        int a = L < 0 ? -L : L; if (a > peak) peak = a;
    }
    last_hl = pl; last_hr = pr;
    dc_spu_peak = peak;
}
static void mix(int16_t *out, int frames) {
    events_run();
    uint16_t rel = *(volatile uint16_t *)(dc_iobase + 0x1108 + 32);   // TIMER_RELOAD(2): the sequencer's rate
    int tick_hz = rel && cb_timer2 ? (int)(4233600 / rel) : 0;
    dc_spu_peak = 0; dc_spu_voices_on = 0;
    for (int f = 0; f < frames;) {
        int n = frames - f; if (n > 512) n = 512;
        // the clock runs in half-rate samples (22.05 kHz, what the voices are mixed at): a stretch of an odd number of output frames
        // would mix one half-rate sample too many (the voices ran 0.1 % sharp); now every stretch is a whole number of them
        if (tick_hz) { int left = ((RATE / 2 - tick_acc + tick_hz - 1) / tick_hz) * 2; if (left < 2) left = 2; if (n > left) n = left; }
        mix_seg(out + f * 2, n);
        f += n;
        if (tick_hz) { tick_acc += tick_hz * ((n + 1) >> 1); if (tick_acc >= RATE / 2) { tick_acc -= RATE / 2; cb_timer2(); events_run(); } }
    }
}

// ---------------------------------------------------------------- the AICA stream and its thread
static snd_stream_hnd_t hnd = SND_STREAM_INVALID;
static int16_t mixbuf[8192 * 2] __attribute__((aligned(32)));
int dc_spu_cbs, dc_spu_frames;
static void *stream_cb(snd_stream_hnd_t h, int req, int *got) {
    (void)h; dc_spu_cbs++; dc_spu_frames += req / 4;
    int frames = req / 4; if (frames > 8192) frames = 8192;
    snd_lock(); mix(mixbuf, frames); snd_unlock();
    *got = frames * 4;
    return mixbuf;
}
static void *snd_thread(void *p) {
    (void)p;
    while (1) { snd_stream_poll(hnd); thd_sleep(5); }
    return 0;
}
void SpuInit(void) {
    static int up;
    memset(spu_ram, 0, sizeof spu_ram);
    static const uint8_t dummy[16] = { 0x00, 0x05 };
    memcpy(spu_ram + 0x1000, dummy, 16);
    for (int i = 0; i < NV; i++) { VREG(i, 0) = VREG(i, 2) = 0; VREG(i, 4) = 0x1000; VREG(i, 6) = 0x200; vc[i].phase = OFF; vc[i].env = 0; }
    *(volatile uint16_t *)(dc_iobase + 0x1daa) = 0xc001;
    *(volatile uint16_t *)(dc_iobase + 0x1d80) = 0x3fff; *(volatile uint16_t *)(dc_iobase + 0x1d82) = 0x3fff;
    *(volatile uint16_t *)(dc_iobase + 0x1da2) = 0xfffe;
    kev_n = 0;
    if (up) return;
    up = 1;
    snd_stream_init();
    hnd = snd_stream_alloc(stream_cb, 16384);
    if (hnd == SND_STREAM_INVALID) return;
    snd_stream_start(hnd, RATE, 1);
    kthread_attr_t at = { 0 }; at.prio = 5; at.label = "spu";
    thd_create_ex(&at, snd_thread, 0);
}
