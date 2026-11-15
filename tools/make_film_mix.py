#!/usr/bin/env python3
"""make_film_mix.py: the sound of the films besides the voices (docs/FILM_MIX.md).

  tools/make_film_mix.py                 writes assets/FILMSFX.BIN (foley + crowd loops), assets/FILMMUS.BIN (the streamed score) and src/film_mix.inc (the cue tables)
  tools/make_film_mix.py --preview DIR   also writes the score as a wav (and each foley sample) to DIR to listen to

Everything is made from formulas (tools/film_synth.py): an original score for "TV SHOW & TELL" (the pop-quiz-show theme, the build to the name, Missy's strut,
the tension and the stop, the comeback, the dance, the end sting), the foley (whoosh, impact, glass, tray, steps, ding, cheer) and two crowd loops.
The CUE SHEETS at the bottom (one per film) say what plays when: add a film by adding a sheet and a score function. Film times are tenths of a second.
FILMSFX.BIN: u32 count, {u32 offset, bytes, rate, loop | steps << 1} per sample (the order of the SFX_* enum of src/audio.h), then SPU ADPCM, 64-byte aligned.
FILMMUS.BIN: sector 0: u32 'FMUS', u32 tracks, {u32 sector, chunks, rate, steps} per track; then each track as 8 KB chunks (4 KB left ADPCM, 4 KB right), flags as tools/build_streams.py."""
import os, sys, re, struct, subprocess, tempfile
import numpy as np
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from film_synth import *

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SECTOR = 2048


# ================================================================= encoding
def encoder():
    enc = os.path.join(tempfile.gettempdir(), 'spuenc')
    if not os.path.exists(enc) or os.path.getmtime(enc) < os.path.getmtime(os.path.join(HERE, 'spuenc.c')):
        subprocess.check_call(['gcc', '-O2', '-o', enc, os.path.join(HERE, 'spuenc.c')])
    return enc


def adpcm(x, mode=None):
    """float mono -> SPU ADPCM bytes. mode None: one-shot (ends in an end block), 'loop', 'stream'"""
    pcm = np.clip(np.round(x * 32767), -32768, 32767).astype('<i2').tobytes()
    out = os.path.join(tempfile.gettempdir(), 'fm_enc.adpcm')
    cmd = [encoder(), out] + (['--' + mode] if mode else [])
    subprocess.run(cmd, input=pcm, check=True)
    return open(out, 'rb').read()


def edges(x, sr, fin=0.003, fout=0.006):
    x = x.copy()
    a, b = int(fin * sr), int(fout * sr)
    x[:a] *= np.linspace(0, 1, a, endpoint=False) ** 1.5
    x[-b:] *= np.linspace(1, 0, b) ** 1.5
    return x


def peak(x, p=0.92):
    return x / max(1e-9, np.abs(x).max()) * p


# ================================================================= the foley and the crowd (sample rates are what the disc and sound memory can afford)
def periodic_noise(n, shape, seed):
    """noise that loops exactly: random phases under a spectral shape (shape(f in Hz) -> gain), n samples at rate sr = given through shape's closure"""
    rng = np.random.RandomState(seed)
    k = n // 2 + 1
    ph = np.exp(2j * np.pi * rng.rand(k))
    spec = shape(np.arange(k)) * ph
    spec[0] = 0
    return np.fft.irfft(spec, n)


def sfx_whoosh(sr=16000):
    rng = np.random.RandomState(21)
    up, dn = int(0.26 * sr), int(0.24 * sr)
    a = sweep_bp(rng.randn(up), 260, 3800, 1.3, sr) * np.sin(np.linspace(0, np.pi / 2, up)) ** 2
    b = sweep_bp(rng.randn(dn), 3800, 700, 1.3, sr) * np.cos(np.linspace(0, np.pi / 2, dn)) ** 1.6
    return edges(peak(np.concatenate([a, b])), sr, 0.004, 0.01)


def sfx_impact(sr=11025):
    n = int(0.62 * sr)
    t = np.arange(n) / sr
    f = 36 + 120 * np.exp(-t / 0.055)
    y = np.sin(2 * np.pi * np.cumsum(f) / sr) * np.exp(-t / 0.2)
    rng = np.random.RandomState(5)
    y += 0.7 * lp(rng.randn(n), 600, sr) * np.exp(-t / 0.07)
    y += 0.25 * hp(rng.randn(n), 1800, sr) * np.exp(-t / 0.012)
    y = np.tanh(y * 1.8)
    return edges(peak(y), sr, 0.0015, 0.03)


def sfx_glass(sr=22050):
    n = int(0.85 * sr)
    t = np.arange(n) / sr
    rng = np.random.RandomState(31)
    y = 0.9 * hp(rng.randn(n), 2500, sr) * np.exp(-t / 0.012)                           # the crack
    y += 0.5 * np.sin(2 * np.pi * 240 * t) * np.exp(-t / 0.05)                            # the thunk of the tray
    for k in range(70):                                                                     # the shards
        t0 = rng.exponential(0.16)
        if t0 > 0.7: continue
        i = int(t0 * sr)
        f = rng.uniform(2200, 9800)
        tau = rng.uniform(0.01, 0.07)
        m = min(n - i, int(tau * 6 * sr))
        tt = np.arange(m) / sr
        y[i:i + m] += rng.uniform(0.15, 0.7) * np.sin(2 * np.pi * f * tt + rng.rand() * 6) * np.exp(-tt / tau) * np.exp(-t0 / 0.4)
    return edges(peak(y), sr, 0.001, 0.05)


def sfx_clink(sr=22050):
    n = int(0.36 * sr)
    t = np.arange(n) / sr
    y = np.zeros(n)
    for f, tau, a in ((3140, 0.11, 1.0), (4710, 0.07, 0.6), (6020, 0.05, 0.45), (1260, 0.14, 0.4)):
        y += a * np.sin(2 * np.pi * f * t) * np.exp(-t / tau)
    y += 0.4 * hp(np.random.RandomState(8).randn(n), 3000, sr) * np.exp(-t / 0.002)
    return edges(peak(y), sr, 0.001, 0.04)


def sfx_step(sr=16000):
    n = int(0.15 * sr)
    t = np.arange(n) / sr
    rng = np.random.RandomState(13)
    y = np.sin(2 * np.pi * 150 * t * (1 + 0.5 * np.exp(-t / 0.02))) * np.exp(-t / 0.035)
    y += 0.5 * lp(rng.randn(n), 1800, sr) * np.exp(-t / 0.012)
    return edges(peak(y), sr, 0.001, 0.03)


def sfx_ding(sr=16000):
    n = int(0.72 * sr)
    t = np.arange(n) / sr
    y = np.zeros(n)
    for r, tau, a in ((1.0, 0.45, 1.0), (2.0, 0.28, 0.45), (2.76, 0.2, 0.3), (4.07, 0.1, 0.18), (5.4, 0.06, 0.12)):
        y += a * np.sin(2 * np.pi * 1318.5 * r * t) * np.exp(-t / tau)
    return edges(peak(y), sr, 0.001, 0.06)


def sfx_cheer(sr=16000):
    n = int(0.75 * sr)
    t = np.arange(n) / sr
    rng = np.random.RandomState(41)
    y = np.zeros(n)
    for v in range(8):
        f0 = rng.uniform(140, 330) * (1 + 0.14 * (t / t[-1]) ** 0.7 - 0.03 * np.sin(2 * np.pi * rng.uniform(4, 7) * t))
        src = saw(f0, n, sr)
        f1, f2 = rng.uniform(560, 760), rng.uniform(1100, 1500)
        w = bp(src, f1 * 0.85, f1 * 1.2, sr, 2) + 0.8 * bp(src, f2 * 0.85, f2 * 1.2, sr, 2)
        d = rng.uniform(0, 0.06)
        i = int(d * sr)
        y[i:] += w[:n - i] * rng.uniform(0.6, 1.0)
    y += 0.25 * bp(rng.randn(n), 1500, 5000, sr) * np.exp(-t / 0.3)
    env = np.clip(t / 0.08, 0, 1) * np.exp(-np.clip(t - 0.2, 0, None) / 0.35)
    return edges(peak(y * env), sr, 0.004, 0.08)


def sfx_applause(sr=14000, dur=0.9):
    n = int(dur * sr) // 28 * 28
    rng = np.random.RandomState(51)
    imp = np.zeros(n)
    for k in range(int(dur * 170)):                       # about 170 claps a second, each a short burst
        imp[rng.randint(n)] += rng.uniform(0.3, 1.0)
    kern = np.exp(-np.arange(int(0.012 * sr)) / (sr * 0.0035))
    env = np.fft.irfft(np.fft.rfft(imp) * np.fft.rfft(kern, n), n)

    def band(lo, hi):
        return lambda k: ((k * sr / n >= lo) & (k * sr / n <= hi)).astype(float) * (1 / np.maximum(k * sr / n, 100) ** 0.35)
    nz = periodic_noise(n, band(900, 6500), 52)
    nz /= np.abs(nz).max()
    floor = periodic_noise(n, band(250, 3500), 53)
    floor /= np.abs(floor).max()
    y = nz * (env / env.max()) * 0.9 + floor * 0.22
    return peak(y, 0.9)


def sfx_walla(sr=8000, dur=1.0):
    n = int(dur * sr) // 28 * 28
    rng = np.random.RandomState(61)

    def formants(k):
        f = k * sr / n
        g = np.zeros_like(f)
        for c, w, a in ((420, 140, 1.0), (850, 220, 0.8), (1500, 300, 0.5), (2500, 450, 0.25)):
            g += a * np.exp(-0.5 * ((f - c) / w) ** 2)
        return g
    y = np.zeros(n)
    for v in range(9):                                     # nine people talking: each a periodic envelope of syllables (whole cycles per loop: it loops exactly)
        nz = periodic_noise(n, formants, 62 + v)
        nz /= np.abs(nz).max()
        t = np.arange(n) / n
        env = np.zeros(n)
        for h in range(3):
            env += rng.uniform(0.3, 1.0) * np.sin(2 * np.pi * int(rng.randint(3, 8)) * t * dur + rng.rand() * 6.28)
        env = np.clip(env / 2 + 0.4, 0, None) ** 1.5
        y += nz * env
    return peak(y, 0.9)


SFX_FNS = [('WHOOSH', 16000, False, sfx_whoosh), ('IMPACT', 11025, False, sfx_impact), ('GLASS', 22050, False, sfx_glass), ('CLINK', 22050, False, sfx_clink),
           ('STEP', 16000, False, sfx_step), ('DING', 16000, False, sfx_ding), ('CHEER', 16000, False, sfx_cheer), ('APPLAUSE', 14000, True, sfx_applause), ('WALLA', 8000, True, sfx_walla)]


def sfx_names_from_header():
    t = open(os.path.join(ROOT, 'src', 'audio.h')).read()
    m = re.search(r'enum \{ ((?:SFX_\w+,? ?)+) \};', t)
    return [s.strip(' ,')[4:] for s in m.group(1).split(',') if s.strip().startswith('SFX_') and s.strip() != 'SFX_N']


def build_sfx(outdir, preview=None):
    names = sfx_names_from_header()
    assert names == [n for n, *_ in SFX_FNS], 'the SFX_* enum of audio.h and SFX_FNS differ: %s' % names
    ents, blob = [], bytearray()
    for name, sr, loop, fn in SFX_FNS:
        x = fn(sr)
        if preview:
            import wave
            w = wave.open(os.path.join(preview, 'sfx_%s.wav' % name.lower()), 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr)
            w.writeframes((np.clip(x, -1, 1) * 32767).astype('<i2').tobytes()); w.close()
        ad = adpcm(x, 'loop' if loop else None)
        ents.append((len(blob), len(ad), sr, int(loop) | (int(np.ceil(len(x) / sr * 60)) << 1)))
        blob += ad + bytes((-len(ad)) % 64)
        print('  sfx %-9s %5.2f s @ %5d Hz %6d bytes %s' % (name, len(x) / sr, sr, len(ad), 'loop' if loop else ''))
    hdr = ((4 + 16 * len(ents)) + 63) & ~63
    tab = struct.pack('<I', len(ents)) + b''.join(struct.pack('<IIII', o + hdr, b, r, l) for o, b, r, l in ents)
    data = tab + bytes(hdr - len(tab)) + bytes(blob)
    # offsets are relative to the start of the file's sound-memory image
    open(os.path.join(outdir, 'FILMSFX.BIN'), 'wb').write(data)
    print('FILMSFX.BIN: %d bytes (%.1f KB, sound memory %.1f KB)' % (len(data), len(data) / 1024, (len(data) + 2047) // 2048 * 2))
    return len(data)


# ================================================================= the score: "TV SHOW & TELL", 61 s, 32 kHz stereo
MSR = 32000
LEN = 62.0
CHORDS = {'C': [60, 64, 67], 'Am': [57, 60, 64], 'F': [57, 60, 65], 'G': [55, 59, 62], 'Dm': [53, 57, 62], 'E': [56, 59, 64], 'D': [62, 66, 69], 'Bm': [59, 62, 66],
          'A': [57, 61, 64], 'A7': [55, 61, 64], 'Gm': [55, 58, 62]}
ROOTS = {'C': 36, 'Am': 33, 'F': 41, 'G': 31, 'Dm': 38, 'E': 40, 'D': 38, 'Bm': 35, 'A': 33, 'A7': 33}
# the theme: 4 bars over C Am F G, as (eighth, midi, length in eighths)
THEME = [[(0, 76, 1), (1, 76, 1), (2, 79, 2), (4, 79, 1), (5, 76, 1), (6, 72, 2)],
         [(0, 81, 2), (2, 79, 1), (3, 77, 1), (4, 76, 2), (6, 72, 1), (7, 74, 1)],
         [(0, 77, 1), (1, 77, 1), (2, 81, 2), (4, 81, 1), (5, 77, 1), (6, 74, 2)],
         [(0, 79, 1), (1, 77, 1), (2, 76, 1.5)]]


class Sec:
    """a section of the score: its own harmony bus (pads and keys, ducked by the kick) and rhythm bus (drums, bass, leads), placed at t0 in the whole score"""

    def __init__(self, t0, t1, tail=0.0):
        self.t0, self.t1 = t0, t1
        self.dur = t1 - t0 + tail
        self.h = Mix(MSR, self.dur + 2.5)
        self.r = Mix(MSR, self.dur + 2.5)
        self.kicks = []

    def at(self, t):
        return t - self.t0

    def kick(self, t, g=1.0):
        self.r.add(kick(MSR), self.at(t), 0.0, 0.75 * g)
        self.kicks.append(self.at(t))

    def snare(self, t, g=1.0):
        self.r.add(snare(MSR), self.at(t), 0.05, 0.5 * g, send=0.25)

    def clap(self, t, g=1.0):
        self.r.add(clap(MSR), self.at(t), -0.05, 0.45 * g, send=0.3)

    def hat(self, t, g=1.0, o=False):
        self.r.add(hat(MSR, o), self.at(t), 0.3, 0.30 * g)

    def crash(self, t, g=1.0):
        self.r.add(crash(MSR), self.at(t), -0.1, 0.36 * g, send=0.3)

    def bass(self, t, m, d, g=1.0):
        self.r.add(bass(MSR, m, d), self.at(t), 0.0, 0.62 * g)

    def keys(self, t, ms, d, g=1.0, pan=-0.35):
        for m in ms:
            self.h.add(epiano(MSR, m, d), self.at(t), pan, 0.20 * g, send=0.18, haas=0.004)

    def stab(self, t, ms, d, g=1.0, pan=-0.1, cut=2600):
        self.r.add(brass(MSR, ms, d, cut), self.at(t), pan, 0.45 * g, send=0.22)

    def pad(self, t, ms, d, g=1.0, cut=1500):
        self.h.add(pad(MSR, ms, d, cut), self.at(t), -0.55, 0.20 * g, send=0.3, haas=0.012)
        self.h.add(pad(MSR, [m + 12 for m in ms[:2]], d, cut * 1.2), self.at(t), 0.55, 0.12 * g, send=0.3, haas=-0.012)

    def lead(self, t, m, d, g=1.0, pan=0.3):
        self.r.add(lead(MSR, m, d), self.at(t), pan, 0.34 * g, send=0.2)
        self.r.add(lead(MSR, m, d), self.at(t) + 0.1875, -pan, 0.10 * g, send=0.25)                # a dotted-eighth echo on the other side

    def sax(self, t, m, d, g=1.0):
        self.r.add(sax(MSR, m, d), self.at(t), 0.18, 0.42 * g, send=0.25)

    def render(self, rev_h=0.5, rt=1.3):
        h = self.h.render(rev_h, rt)
        r = self.r.render(0.5, 0.9)
        if self.kicks:                                         # the kick ducks the harmony (a pumping pop feel)
            env = np.ones(h.shape[0])
            for tk in self.kicks:
                i = int(tk * MSR)
                if 0 <= i < len(env):
                    m = min(len(env) - i, int(0.22 * MSR))
                    env[i:i + m] = np.minimum(env[i:i + m], 1 - 0.55 * np.exp(-np.arange(m) / (0.07 * MSR)))
            h = h * env[:, None]
        return h + r


def groove(s, t0, t1, beat, chords, kick_pat=(0, 4), snare_pat=(), clap_pat=(2, 6), hat_pat=tuple(range(8)), ohat=(), bass_pat=(0, 3, 4, 6), keys_pat=(1, 3, 5, 7), vel=1.0, bars_per_chord=1, pads=True, bassg=1.0):
    """one bar = 8 eighths; the chords cycle every bars_per_chord bars"""
    bar = beat * 4
    e = beat / 2
    t = t0
    k = 0
    while t < t1 - 1e-6:
        ch = chords[(k // bars_per_chord) % len(chords)]
        for i in kick_pat: s.kick(t + i * e, vel)
        for i in snare_pat: s.snare(t + i * e, vel)
        for i in clap_pat: s.clap(t + i * e, 0.8 * vel)
        for i in hat_pat: s.hat(t + i * e, (1.0 if i % 2 == 0 else 0.6) * vel)
        for i in ohat: s.hat(t + i * e, 0.9 * vel, True)
        root = ROOTS[ch]
        for j, i in enumerate(bass_pat):
            s.bass(t + i * e, root + (12 if j % 4 == 1 else (7 if j % 4 == 3 else 0)), e * 0.9, vel * bassg)
        for i in keys_pat: s.keys(t + i * e, CHORDS[ch], e * 0.8, 0.8 * vel)
        if pads and k % bars_per_chord == 0: s.pad(t, CHORDS[ch], bar * bars_per_chord, 0.7 * vel)
        t += bar
        k += 1


def theme(s, t0, bars, semis=0, first_bar=0, g=1.0, beat=0.5, brass=True, hook_len=None):
    e = beat / 2
    names = ['C', 'Am', 'F', 'G']
    for b in range(first_bar, bars):
        tb = t0 + (b - first_bar) * beat * 4
        for (i, m, d) in THEME[b % 4]:
            if hook_len is not None and b == bars - 1 and i + d > hook_len: d = max(0.5, hook_len - i)
            s.lead(tb + i * e, m + semis, d * e * 0.92, g)
        if brass:
            ch = [c + semis for c in CHORDS[names[b % 4]]]
            s.stab(tb, [c + 12 for c in ch], e * 1.6, 0.8)
            s.stab(tb + 3 * e, [c + 12 for c in ch], e * 0.7, 0.55)


def build_score():
    out = np.zeros((int(LEN * MSR) + MSR, 2))

    def put(sec, gain=1.0, fade_out=0.012):
        y = sec.render() * gain
        n = min(len(y), len(out) - int(sec.t0 * MSR))
        y = y[:n].copy()
        if fade_out is not None:                               # the section stops where it should (the tail is cut in a few ms, not left to hang)
            cut = int((sec.t1 - sec.t0 + sec.tail_keep) * MSR) if hasattr(sec, 'tail_keep') else int((sec.t1 - sec.t0) * MSR)
            cut = min(cut, n)
            f = int(fade_out * MSR)
            y[cut:] = 0
            y[max(0, cut - f):cut] *= np.linspace(1, 0, min(f, cut))[:, None]
        out[int(sec.t0 * MSR):int(sec.t0 * MSR) + n] += y

    # ---- A: 0 - 7.0: the quiz-show theme, 120 bpm
    s = Sec(0.0, 8.0, 0.0); s.tail_keep = 0.6
    groove(s, 0, 8.0, 0.5, ['C', 'Am', 'F', 'G'], kick_pat=(0, 4), clap_pat=(2, 6), snare_pat=(), vel=1.0, bassg=1.0)
    s.crash(0.02, 0.5)
    s.crash(4.0, 0.5)
    for i, tt in enumerate((6.5, 6.625, 6.75, 6.875)): s.snare(tt, 0.6 + 0.12 * i)               # the fill under the end of the phrase
    theme(s, 0.0, 4, 0, 0, 1.0, 0.5, True, hook_len=3.0)
    put(s)
    # ---- B: 8.0 - 20.0: the bed under the first speeches (no lead; the groove and the chords) ; it continues A's last bar
    s = Sec(8.0, 20.0, 0.0); s.tail_keep = 0.3
    groove(s, 8.0, 20.0, 0.5, ['C', 'Am', 'F', 'G'], kick_pat=(0, 4), clap_pat=(2, 6), hat_pat=(0, 2, 3, 4, 6, 7), vel=0.85, bars_per_chord=1)
    put(s)
    # ---- C: 20.0 - 21.6: the build to her name: a riser, a snare roll, a pedal
    s = Sec(20.0, 21.6, 0.0); s.tail_keep = 0.0
    s.r.add(riser(MSR, 1.6), 0.0, 0.0, 0.55, send=0.2)
    s.bass(20.0, 31, 1.6, 1.0)
    s.pad(20.0, CHORDS['F'], 0.8, 0.9); s.pad(20.8, CHORDS['G'], 0.8, 1.0)
    roll = [20.4, 20.6, 20.8, 21.0, 21.1, 21.2, 21.3, 21.4, 21.45, 21.5, 21.55]
    for i, tt in enumerate(roll): s.snare(tt, 0.4 + 0.07 * i)
    s.kick(21.0, 0.7); s.kick(21.3, 0.8)
    put(s)
    # ---- D: 21.6 - 33.6: her name (the hit), the fanfare, Missy's strut. 150 bpm: a beat is 0.4 s; it feels half time
    s = Sec(21.6, 33.6, 0.0); s.tail_keep = 0.0
    c = CHORDS['C']
    s.stab(21.6, [c[0] + 12, c[1] + 12, c[2] + 12, c[0] + 24], 0.5, 1.2, 0.0, 3200)       # (short: her name follows the hit by 50 ms)
    s.stab(21.6, [c[0], c[2]], 0.5, 0.7, 0.0)
    s.crash(21.6, 0.7); s.kick(21.6, 1.2); s.bass(21.6, 36, 1.0, 1.2)
    s.pad(21.6, CHORDS['C'], 1.2, 0.6)
    s.stab(22.8, [g + 12 for g in CHORDS['G']] + [74], 0.35, 0.95, 0.1)                          # the second hit, on the white flash and the cheer
    s.crash(22.8, 0.6); s.kick(22.8, 0.8)
    e = 0.2
    prog = ['Am', 'Dm', 'E', 'Am', 'Am', 'Dm', 'E']
    for b, ch in enumerate(prog):
        t = 23.2 + b * 1.6
        s.kick(t, 0.9); s.kick(t + 3 * e, 0.7); s.clap(t + 4 * e, 1.0)
        for i in range(8): s.hat(t + i * e, 0.9 if i % 2 == 0 else 0.5)
        r = ROOTS[ch]
        for i, m in ((0, r), (2, r), (3, r + 3 if ch != 'E' else r + 4), (5, r + 5), (6, r + 7)): s.bass(t + i * e, m, e * 0.9, 1.0)
        for i in (1, 3, 5): s.keys(t + i * e, CHORDS[ch], e * 0.8, 0.9, -0.3)
        s.pad(t, CHORDS[ch], 1.6, 0.8, 1200)
    # the strut: the saxophone riff on her entrance (bars 1 and 2 only), then little answers in the gaps between her lines
    riff = [[(0, 76, 1), (2, 76, 1), (3, 79, 1), (4, 81, 2), (6, 79, 1), (7, 76, 1)], [(0, 74, 1), (2, 74, 1), (3, 77, 1), (4, 81, 2), (6, 77, 1), (7, 74, 1)]]
    for b in range(2):
        for (i, m, d) in riff[b]: s.sax(23.2 + b * 1.6 + i * e, m, d * e * 0.9, 1.0)
    for (tt, m, d) in ((27.25, 81, 0.15), (27.4, 79, 0.25), (30.02, 76, 0.15), (30.12, 81, 0.35)): s.sax(tt, m, d, 0.9)
    put(s, 1.0, 0.04)
    # ---- E: 33.6 - 42.4: the tension: a heartbeat every 1.1 s (eight of them: the last is the glass), a dark pad, a clock, a rising wash
    s = Sec(33.6, 42.4, 0.0); s.tail_keep = 0.0
    T0 = 33.6
    for k in range(8):
        t = T0 + k * 1.1
        g = 0.55 + 0.1 * k
        s.r.add(tom(MSR, 58, 0.3), s.at(t), 0.0, 0.9 * g)
        s.r.add(tom(MSR, 52, 0.3), s.at(t) + 0.23, 0.0, 0.7 * g)
    s.r.add(sine(mid2hz(33), int(8.8 * MSR), MSR) * adsr(int(8.8 * MSR), MSR, 1.2, 1, 1, 0.01), 0.0, 0.0, 0.35)
    for ms, g in (([45, 52], 0.9), ([46, 58], 0.5), ([57, 64, 70], 0.6)):                    # A minor with a flat second above: it grates
        s.h.add(pad(MSR, ms, 8.8, 700, 1.0, 2.5, 0.02), 0.0, -0.3, 0.24 * g, send=0.3, haas=0.01)
    n = int(8.8 * MSR)
    trem = (0.55 + 0.45 * np.sin(2 * np.pi * np.cumsum(np.linspace(5, 11, n)) / MSR))
    st = sum(saw(mid2hz(m), n, MSR) for m in (69, 70, 76)) * trem
    st = lp(st, 1800, MSR) * np.linspace(0, 1, n) ** 2.2 * adsr(n, MSR, 0.01, 1, 1, 0.01)
    s.h.add(st, 0.0, 0.2, 0.11, send=0.2)
    s.r.add(riser(MSR, 4.4, 600, 7000, 13), 4.4, 0.0, 0.4, send=0.15)
    t = 4.9
    while t < 8.78:                                                                           # the clock: a tick that speeds up
        s.r.add(sine(1900, int(0.012 * MSR), MSR) * expdec(int(0.012 * MSR), MSR, 0.003), t, 0.35 if int((t - 4.9) * 4) % 2 == 0 else -0.35, 0.28)
        t += 0.28 - 0.1 * (t - 4.9) / 3.9
    put(s, 1.0, 0.012)
    # ---- (42.4 - 43.6: nothing: the glass, the shriek and the gasp are the foley's; a reversed cymbal leads into the comeback)
    s = Sec(43.0, 43.6, 0.0); s.tail_keep = 0.0
    rc = crash(MSR, 1.2)[::-1][:int(0.6 * MSR)] * 0.9
    rc[-int(0.006 * MSR):] *= np.linspace(1, 0, int(0.006 * MSR))
    s.r.add(rc, 0.0, 0.0, 0.55)
    put(s, 1.0, 0.004)
    # ---- G1: 43.6 - 47.0: she is back (141 bpm: a beat is 0.425 s): the groove in C, no lead under the words, the climb into D
    B = 0.425
    s = Sec(43.6, 47.0, 0.0); s.tail_keep = 0.0
    s.stab(43.6, [72, 76, 79, 84], 0.4, 1.15, 0.0, 3200)
    s.crash(43.6, 0.6); s.kick(43.6, 1.0)
    groove(s, 43.6, 47.0, B, ['Am', 'A7'], kick_pat=(0, 2, 4, 6), clap_pat=(2, 6), hat_pat=(1, 3, 5, 7), ohat=(), vel=0.95, bass_pat=(1, 3, 5, 7), keys_pat=(1, 3, 5, 7), bassg=0.9)
    for i, tt in enumerate((46.4, 46.5, 46.6, 46.7, 46.8, 46.9)): s.snare(tt, 0.5 + 0.1 * i)
    put(s, 1.0, 0.02)
    # ---- G2: 47.0 - 54.5: WATCH ME DANCE: the groove up a whole step (D), the hook comes in under her bow
    s = Sec(47.0, 54.5, 0.0); s.tail_keep = 0.0
    s.stab(47.0, [74, 78, 81, 86], 0.45, 1.25, 0.0, 3400)
    s.stab(47.0, [50, 57], 0.45, 0.7)
    s.crash(47.0, 0.6)
    groove(s, 47.0, 54.5, B, ['D', 'Bm', 'G', 'A'], kick_pat=(0, 2, 4, 6), clap_pat=(2, 6), hat_pat=(1, 3, 5, 7), ohat=(), vel=1.0, bass_pat=(1, 3, 5, 7), keys_pat=(1, 3, 5, 7))
    for tt in np.arange(47.0, 54.5, B * 4): s.hat(tt + B * 3.5, 0.9, True)
    theme(s, 47.0 + 1.7, 4, 2, 1, 1.0, B, True)                                                # bars 2, 3, 4 of the theme in D (Bm, G, A)
    s.kick(54.4, 0.6)
    for i, tt in enumerate((53.8, 54.0, 54.15, 54.25, 54.35, 54.42, 54.47)): s.snare(tt, 0.5 + 0.08 * i)
    s.r.add(riser(MSR, 0.8, 800, 9000, 19), 53.7 - s.t0 + 0.0, 0.0, 0.4)
    put(s, 1.0, 0.012)
    # ---- G3: 54.5 - 56.0: the last hit on the flash, the groove goes on
    s = Sec(54.5, 56.0, 0.0); s.tail_keep = 0.0
    s.stab(54.5, [74, 78, 81, 86], 0.5, 1.3, 0.0, 3400); s.crash(54.5, 0.9); s.kick(54.5, 1.0)
    groove(s, 54.5, 56.0, B, ['D', 'A'], kick_pat=(0, 2, 4, 6), clap_pat=(2, 6), hat_pat=(1, 3, 5, 7), vel=0.95, bass_pat=(1, 3, 5, 7), keys_pat=(1, 3, 5, 7))
    for i, tt in enumerate((55.55, 55.68, 55.78, 55.86, 55.92, 55.96)): s.snare(tt, 0.5 + 0.1 * i)
    put(s, 1.0, 0.012)
    # ---- H: 56.0 - 61.5: the end sting: D major, brass, cymbal, a rising arpeggio, and it rings
    s = Sec(56.0, 61.5, 0.0); s.tail_keep = 5.5
    s.stab(56.0, [62, 66, 69, 74, 78], 1.4, 1.35, 0.0, 3600)
    s.stab(56.0, [50, 57], 1.4, 0.8)
    s.crash(56.0, 1.2); s.kick(56.0, 1.2)
    s.r.add(tom(MSR, 70, 0.6), 0.0, 0.0, 0.8); s.bass(56.0, 38, 3.0, 1.2)
    s.pad(56.0, CHORDS['D'] + [74], 4.5, 1.1, 1800)
    for k, m in enumerate((74, 78, 81, 86, 90)): s.lead(56.15 + k * 0.1, m, 0.5 + 0.2 * (4 - k), 0.8, [-0.3, 0.3, -0.2, 0.2, 0.0][k])
    for k, m in enumerate((86, 90, 93)): s.keys(56.8 + k * 0.5, [m], 1.5, 0.6, [-0.5, 0.5, 0.0][k])
    put(s, 1.0, 0.3)
    n_end = int(LEN * MSR)
    y = out[:n_end]
    y = hp(y, 30, MSR, 2)
    # the loudness: leave the dynamics of the arrangement, only a soft limiter and a level
    y = finish(y, 0.89)
    return y


# ================================================================= the streamed score on the disc
def build_mus(outdir, tracks, preview=None):
    IL = 4096
    ents = []
    blob = bytearray()
    sector = 1
    for name, y in tracks:
        if preview:
            import wave
            w = wave.open(os.path.join(preview, 'score_%s.wav' % name), 'wb'); w.setnchannels(2); w.setsampwidth(2); w.setframerate(MSR)
            w.writeframes((np.clip(y, -1, 1) * 32767).astype('<i2').tobytes()); w.close()
        L = bytearray(adpcm(y[:, 0], 'stream'))
        R = bytearray(adpcm(y[:, 1], 'stream'))
        nch = (max(len(L), len(R)) + IL - 1) // IL
        L += bytes(nch * IL - len(L)); R += bytes(nch * IL - len(R))
        data = bytearray()
        for k in range(nch):
            for c in (L, R):
                part = bytearray(c[k * IL:(k + 1) * IL])
                part[IL - 16 + 1] = 0x03                                                         # end + repeat: on to the next buffer (the streamer)
                data += part
        ents.append((sector, nch, MSR, int(round(len(y) / MSR * 60))))
        blob += data
        sector += len(data) // SECTOR
        print('  score %-8s %5.1f s  %4d chunks  %5d KB' % (name, len(y) / MSR, nch, len(data) // 1024))
    idx = struct.pack('<II', 0x53554D46, len(ents)) + b''.join(struct.pack('<IIII', *e) for e in ents)
    open(os.path.join(outdir, 'FILMMUS.BIN'), 'wb').write(idx + bytes(SECTOR - len(idx)) + bytes(blob))
    print('FILMMUS.BIN: %d KB' % ((SECTOR + len(blob)) // 1024))


# ================================================================= the cue sheets (film times in tenths of a second; one line per cue)
# ops: sfx(id, lvl, pan[, pan_end[, pit]]), vox(VOICE_NAME, lvl, pan), bed(0 murmur / 1 applause, lvl, ramp in tenths of a second), music(lvl, ramp tenths), master(lvl, ramp in 60 Hz steps)
# pan -128 left .. 127 right (the stage: Chad at the left of centre, Missy right, the intern coming in from the right); pit in half-semitones.
def sheet_open():
    c = []
    S = lambda t, name, lvl, pan=0, pan2=None, pit=0: c.append((t, 'sfx', name, lvl, pan, pan if pan2 is None else pan2, pit, 0))
    V = lambda t, name, lvl, pan=0: c.append((t, 'vox', name, lvl, pan, pan, 0, 0))
    Bd = lambda t, which, lvl, ramp: c.append((t, 'bed', which, lvl, 0, 0, 0, ramp))
    Mu = lambda t, lvl, ramp: c.append((t, 'music', 0, lvl, 0, 0, 0, ramp))
    Ma = lambda t, lvl, ramp: c.append((t, 'master', 0, lvl, 0, 0, 0, ramp))
    Ma(0, 255, 8)                                                           # the fade in (the picture fades in over 1.2 s)
    Bd(0, 0, 70, 12); Bd(0, 1, 0, 1)                                        # the audience murmurs
    S(2, 'WHOOSH', 150, -90, 10)                                            # the title plate slashes in
    S(9, 'DING', 150, 0, pit=0)                                             # ... and the type lands
    S(44, 'WHOOSH', 120, 10, 90, -3)                                        # ... and slashes out
    for k in range(11):                                                     # Chad bounds in from the left: steps, closer and louder
        S(52 + round(k * 2.3), 'STEP', 100 + 5 * k, -100 + 9 * k, pit=(-2 if k % 2 else 2))
    V(88, 'YEHA', 200, -25)
    Bd(88, 1, 80, 2); Bd(100, 1, 130, 2); Bd(100, 0, 100, 2)               # the audience goes wild
    S(105, 'WHOOSH', 170, -110, 30, 3)                                      # the white flash cut and the low sweep along the front row
    S(106, 'CHEER', 165, -70, pit=0); S(113, 'CHEER', 140, 55, pit=3)
    V(112, 'LAUGH', 165, 45)
    Bd(125, 1, 95, 15); Bd(160, 1, 45, 40); Bd(160, 0, 70, 20)
    V(150, 'YAHOO', 190, -60)
    Mu(66, 225, 8)                                                          # the groove sits down for the first words
    Mu(190, 255, 15)                                                        # ... and the build to her name is full
    S(200, 'WHOOSH', 90, -20, 20, -8)                                       # the push in
    S(216, 'IMPACT', 160, 0)                                                # the red flash cut: MISSY TELLER!
    S(216, 'WHOOSH', 115, -127, 127, 5)
    S(222, 'CHEER', 170, 0, pit=-2)
    S(226, 'WHOOSH', 150, -40, 60, 2)                                       # the speed lines
    S(228, 'IMPACT', 170, 0, pit=8)                                         # the white flash cut
    V(228, 'FLIRT', 185, 45)
    Bd(216, 1, 140, 4); Bd(216, 0, 100, 4)
    Bd(262, 1, 40, 60); Bd(262, 0, 60, 30)
    Mu(222, 165, 10)                                                        # her strut under her lines
    S(246, 'WHOOSH', 80, 20, 50, -6)                                        # the two-shot
    for k in range(7):                                                      # the intern comes in from the right with the tray
        S(291 + round(k * 2.9), 'STEP', 85 + 5 * k, 120 - 8 * k, pit=(2 if k % 2 else -1))
    S(338, 'CLINK', 170, 62)                                                # the tray goes down
    S(346, 'CLINK', 90, 62, pit=5)
    V(350, 'NEED', 165, 40)
    for k in range(3): S(374 + k * 3, 'STEP', 70 - 8 * k, 55 + 15 * k)      # he backs away
    Mu(336, 205, 20)                                                        # the tension under the intern
    Mu(392, 165, 5); Mu(412, 255, 8)                                        # ... a little less under her "not tonight"
    S(388, 'WHOOSH', 100, 55, 70, -10)                                      # speed lines
    S(392, 'WHOOSH', 110, 40, 80, -4)                                       # the flash cut and her push
    S(392, 'IMPACT', 120, 40, pit=-6)
    S(404, 'CLINK', 140, 70, pit=4)                                         # the glass slides off the tray
    S(412, 'CLINK', 100, 70, pit=-3)
    Bd(398, 0, 40, 20); Bd(398, 1, 0, 20)                                   # the audience holds its breath
    S(424, 'GLASS', 200, 70)                                                # the glass breaks
    S(424, 'IMPACT', 95, 40, pit=9)
    V(425, 'SHRIEK', 150, 55)                                             # (she shrieks a beat after it breaks)
    Bd(424, 0, 20, 2)
    Bd(432, 0, 100, 4); Bd(432, 1, 135, 6)                                  # and the crowd goes wild
    S(433, 'CHEER', 170, -35, pit=1); S(437, 'CHEER', 155, 55, pit=4)
    S(436, 'WHOOSH', 180, -90, 90, 4)                                       # the white flash cut and speed lines
    V(440, 'YEHA', 190, -50)
    Mu(436, 215, 5)
    Mu(466, 180, 3)                                                         # the hit lands, then the score steps back for WATCH ME DANCE
    Mu(484, 255, 8)
    S(470, 'IMPACT', 160, 0)                                                # the red flash cut: WATCH ME DANCE
    S(470, 'WHOOSH', 115, -120, 120, 8)
    S(473, 'CHEER', 170, 0, pit=2)
    V(476, 'WIN', 190, 40)
    S(496, 'WHOOSH', 140, -70, 70, 0)                                       # the speed lines of the dance
    Bd(480, 1, 110, 10)
    S(545, 'IMPACT', 130, 0, pit=3)                                         # the last white flash cut
    S(545, 'WHOOSH', 140, -100, 100, 6)
    S(548, 'CHEER', 150, 0, pit=-1)
    Bd(545, 1, 140, 6)
    S(560, 'WHOOSH', 150, -90, 10)                                          # the end card slashes in
    S(568, 'DING', 190, 0, pit=-4)
    Ma(600, 0, 40)                                                          # the fade out (the picture fades from 60.0 s over 3.6 s)
    return c


SHEETS = {'open': dict(cues=sheet_open, track='open', lines_pan={1: -22, 2: 34, 3: 40})}


# ================================================================= the voice-over side: line gains, pans and the duck windows from FILMVOX.BIN and subs[]
C0 = [0, 60, 115, 98, 122]
C1 = [0, 0, -52, -55, -60]


def decode(b):
    o = []
    s1 = s2 = 0
    for i in range(0, len(b) - 15, 16):
        sh = b[i] & 15
        f = min(b[i] >> 4, 4)
        for j in range(28):
            n = (b[i + 2 + j // 2] >> (4 * (j & 1))) & 15
            if n > 7: n -= 16
            v = ((n << 12) >> sh) + ((s1 * C0[f] + s2 * C1[f] + 32) >> 6)
            v = max(-32768, min(32767, v)); o.append(v); s2 = s1; s1 = v
    return np.array(o, float) / 32768


def parse_subs():
    src = open(os.path.join(ROOT, 'src', 'cutscene.c')).read()
    m = re.search(r'static const Sub subs\[\] = \{(.*?)\} \};', src, re.S)
    return [(int(a), int(b), int(c)) for a, b, c in re.findall(r'\{\s*T\((\d+)\),\s*T\((\d+)\),\s*(\d),', m.group(1))]


def vox_info():
    d = open(os.path.join(ROOT, 'assets', 'FILMVOX.BIN'), 'rb').read()
    n = struct.unpack('<I', d[:4])[0]
    out = []
    for i in range(n):
        off, ln, rate, steps = struct.unpack('<IIII', d[4 + 16 * i:20 + 16 * i])
        x = decode(d[off:off + ln])
        fr = int(rate * 0.01)
        e = np.array([np.sqrt(np.mean(x[k:k + fr] ** 2)) for k in range(0, len(x) - fr, fr)])
        rms = np.sqrt(np.mean(e[e > 10 ** (-40 / 20)] ** 2))
        out.append(dict(steps=steps, rms=rms, peak=np.abs(x).max()))
    return out


def line_table(sheet):
    subs = parse_subs()
    info = vox_info()
    lines = []
    ref = 10 ** (-17.8 / 20)                                  # every line to the same loudness (the lines are within 0.3 dB already), never past a peak of 0.9
    for i, v in enumerate(info):
        g = min(ref / v['rms'], 0.90 / v['peak'])
        lines.append((int(round(np.clip(g, 0.5, 1.99) * 128)), sheet['lines_pan'].get(subs[i][2], 0)))
    # the duck windows: on 0.1 s before a line, off 0.13 s after it; a window that would open within 1.2 s of the last one's end is joined to it (no pumping between lines)
    wins = []
    for i, (t0, t1, who) in enumerate(subs):
        a = t0 * 6 + 3 - 6
        b = t0 * 6 + 3 + info[i]['steps'] + 8
        if wins and a - wins[-1][1] < 72: wins[-1][1] = b
        else: wins.append([a, b])
    return lines, wins, subs, info


def emit_inc(path, sheets_built):
    L = ['// GENERATED by tools/make_film_mix.py (the cue sheets at its end): do not edit by hand. Times are film steps (60 Hz), a comment gives them in seconds.',
         '// MixCue: { step, op, id, level, pan, pan_end, pitch (half-semitones), ramp (steps) } (src/audio.h)']
    for name, (cues, lines, wins) in sheets_built.items():
        rows = []
        for (t, op, a, lvl, pan, pan2, pit, ramp) in cues:
            assert 0 <= lvl <= 255 and -128 <= pan <= 127 and -128 <= pan2 <= 127 and -64 <= pit <= 63, (t, op, a, lvl, pan, pan2, pit)
            if op == 'sfx': rows.append((t * 6, 'MX_SFX', 'SFX_' + a, lvl, pan, pan2, pit, 0, ''))
            elif op == 'vox': rows.append((t * 6, 'MX_VOX', 'VOICE_' + a, lvl, pan, pan2, 0, 0, ''))
            elif op == 'bed': rows.append((t * 6, 'MX_BED', str(a), lvl, 0, 0, 0, min(255, ramp * 6), ''))
            elif op == 'music': rows.append((t * 6, 'MX_MUSIC', '0', lvl, 0, 0, 0, min(255, ramp * 6 if ramp > 1 else 1), ''))
            elif op == 'master': rows.append((t * 6, 'MX_MASTER', '0', lvl, 0, 0, 0, min(255, ramp), ''))
        for a, b in wins:
            rows.append((a, 'MX_DUCK', '0', 1, 0, 0, 0, 0, ''))
            rows.append((b, 'MX_DUCK', '0', 0, 0, 0, 0, 0, ''))
        rows.sort(key=lambda r: (r[0], r[3] if r[1] == 'MX_DUCK' else 0))
        L.append('static const MixCue film_mix_%s[] = {' % name)
        for r in rows:
            L.append('    { %5d, %-9s, %-12s, %3d, %4d, %4d, %3d, %3d },   // %6.2f s' % (r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0] / 60.0))
        L.append('};')
        L.append('static const MixLine film_lines_%s[] = { %s };' % (name, ', '.join('{ %d, %d }' % l for l in lines)))
        L.append('')
    open(path, 'w').write('\n'.join(L))


def main():
    preview = None
    if '--preview' in sys.argv:
        preview = sys.argv[sys.argv.index('--preview') + 1]
        os.makedirs(preview, exist_ok=True)
    assets = os.path.join(ROOT, 'assets')
    print('foley and crowd:')
    build_sfx(assets, preview)
    print('score:')
    y = build_score()
    build_mus(assets, [('open', y)], preview)
    built = {}
    for name, sh in SHEETS.items():
        lines, wins, subs, info = line_table(sh)
        built[name] = (sh['cues'](), lines, wins)
        print('  sheet %s: %d cues, %d duck windows, line gains %s' % (name, len(built[name][0]), len(wins), [l[0] for l in lines]))
    emit_inc(os.path.join(ROOT, 'src', 'film_mix.inc'), built)


if __name__ == '__main__':
    main()
