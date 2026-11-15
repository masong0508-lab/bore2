#!/usr/bin/env python3
"""film_synth.py: the small synthesiser behind tools/make_film_mix.py (the film's score and foley). Everything is computed from
formulas and random numbers (numpy, scipy): no samples, no recordings, nothing borrowed, so the score and the sounds are ours.
Instruments return float arrays at the sample rate of the Mix they are rendered into."""
import numpy as np
from scipy import signal


def mid2hz(m):
    return 440.0 * 2 ** ((m - 69) / 12.0)


def saw(f, n, sr, ph=0.0):
    """band-limited sawtooth (polyBLEP); f may be a scalar or an array (a glide)"""
    f = np.broadcast_to(np.asarray(f, float), (n,))
    dt = f / sr
    t = (ph + np.cumsum(dt)) % 1.0
    y = 2 * t - 1
    m = t < dt
    x = t[m] / dt[m]
    y[m] -= x + x - x * x - 1
    m = t > 1 - dt
    x = (t[m] - 1) / dt[m]
    y[m] -= x * x + x + x + 1
    return y


def pulse(f, n, sr, duty=0.25, ph=0.0):
    return saw(f, n, sr, ph) - saw(f, n, sr, (ph + duty) % 1.0)


def sine(f, n, sr, ph=0.0):
    f = np.broadcast_to(np.asarray(f, float), (n,))
    return np.sin(2 * np.pi * (ph + np.cumsum(f) / sr))


def lp(x, fc, sr, order=2):
    return signal.sosfilt(signal.butter(order, min(fc, sr * 0.45), 'lp', fs=sr, output='sos'), x)


def hp(x, fc, sr, order=2):
    return signal.sosfilt(signal.butter(order, fc, 'hp', fs=sr, output='sos'), x)


def bp(x, lo, hi, sr, order=2):
    return signal.sosfilt(signal.butter(order, [lo, min(hi, sr * 0.45)], 'bp', fs=sr, output='sos'), x)


def sweep_bp(x, f0, f1, q, sr, chunk=256):
    """a band-pass whose centre moves (exponentially) from f0 to f1 over the signal: whooshes and risers"""
    n = len(x)
    out = np.zeros(n)
    zi = np.zeros(2)
    for i in range(0, n, chunk):
        u = (i + chunk / 2) / n
        fc = f0 * (f1 / f0) ** u
        w = 2 * np.pi * min(fc, sr * 0.45) / sr
        al = np.sin(w) / (2 * q)
        b = np.array([al, 0, -al])
        a = np.array([1 + al, -2 * np.cos(w), 1 - al])
        y, zi = signal.lfilter(b / a[0], a / a[0], x[i:i + chunk], zi=zi)
        out[i:i + chunk] = y
    return out


def adsr(n, sr, a=0.005, d=0.1, s=0.7, r=0.05):
    """an envelope of n samples: attack, decay to s, hold, release in the last r seconds"""
    e = np.ones(n)
    na, nd, nr = max(1, int(a * sr)), max(1, int(d * sr)), max(1, int(r * sr))
    na = min(na, n)
    e[:na] = np.linspace(0, 1, na, endpoint=False)
    if n > na:
        k = min(nd, n - na)
        e[na:na + k] = 1 - (1 - s) * (1 - np.exp(-np.arange(k) / (0.35 * nd)))
        e[na + k:] = s
    nr = min(nr, n)
    e[n - nr:] *= np.linspace(1, 0, nr)
    return e


def expdec(n, sr, tau):
    return np.exp(-np.arange(n) / (sr * tau))


class Mix:
    """a stereo buffer: add(sig, t, pan, gain) places a mono signal at t seconds; send() feeds the reverb bus"""

    def __init__(self, sr, seconds):
        self.sr = sr
        self.n = int(seconds * sr)
        self.L = np.zeros(self.n)
        self.R = np.zeros(self.n)
        self.sL = np.zeros(self.n)
        self.sR = np.zeros(self.n)

    def add(self, x, t, pan=0.0, gain=1.0, send=0.0, haas=0.0):
        i = int(round(t * self.sr))
        if i >= self.n or i + len(x) <= 0:
            return
        a = max(0, -i)
        b = min(len(x), self.n - i)
        seg = x[a:b] * gain
        i += a
        th = (pan + 1) * np.pi / 4
        gl, gr = np.cos(th) * 1.4142, np.sin(th) * 1.4142
        sl, sr_ = seg * gl, seg * gr
        if haas:                                    # a few ms of delay on one side for width
            d = int(haas * self.sr)
            if haas > 0:
                sr_ = np.concatenate([np.zeros(d), sr_])[:len(seg)]
            else:
                sl = np.concatenate([np.zeros(-d), sl])[:len(seg)]
        self.L[i:i + len(seg)] += sl
        self.R[i:i + len(seg)] += sr_
        if send:
            self.sL[i:i + len(seg)] += sl * send
            self.sR[i:i + len(seg)] += sr_ * send

    def render(self, wet=1.0, rt=1.1, seed=3):
        """the dry buffer plus its reverb send through a stereo plate (a different noise tail each side)"""
        rng = np.random.RandomState(seed)
        n = int(rt * self.sr)
        t = np.arange(n) / self.sr
        outs = []
        for dry, snd in ((self.L, self.sL), (self.R, self.sR)):
            ir = rng.randn(n) * np.exp(-t / (rt / 5.5))
            ir = lp(ir, 6500, self.sr)
            ir = hp(ir, 180, self.sr)
            ir = np.concatenate([np.zeros(int(0.014 * self.sr)), ir])
            ir /= np.sqrt((ir ** 2).sum())
            w = signal.fftconvolve(snd, ir)[:self.n]
            outs.append(dry + wet * w)
        return np.stack(outs, axis=1)


# ---------------------------------------------------------------- drums
def kick(sr, f0=48, punch=150, tau=0.16, dur=0.34):
    n = int(dur * sr)
    t = np.arange(n) / sr
    f = f0 + punch * np.exp(-t / 0.028)
    ph = 2 * np.pi * np.cumsum(f) / sr
    y = np.sin(ph) * np.exp(-t / tau)
    y += 0.5 * np.random.RandomState(1).randn(n) * np.exp(-t / 0.004) * 0.3
    return np.tanh(y * 1.6) * 0.9


def snare(sr, tone=190, dur=0.28, seed=2):
    n = int(dur * sr)
    t = np.arange(n) / sr
    nz = hp(np.random.RandomState(seed).randn(n), 1600, sr) * np.exp(-t / 0.075)
    return 0.8 * nz + 0.55 * np.sin(2 * np.pi * tone * t * (1 + 0.25 * np.exp(-t / 0.02))) * np.exp(-t / 0.05)


def clap(sr, dur=0.3, seed=5):
    n = int(dur * sr)
    t = np.arange(n) / sr
    nz = bp(np.random.RandomState(seed).randn(n), 900, 3800, sr)
    env = np.zeros(n)
    for k, d in enumerate((0, 0.009, 0.018)):
        i = int(d * sr)
        env[i:] += np.exp(-np.arange(n - i) / (sr * 0.006)) * 0.8
    env += np.exp(-t / 0.07) * 0.6
    return nz * env * 1.6


def hat(sr, open_=False, seed=3):
    n = int((0.22 if open_ else 0.06) * sr)
    t = np.arange(n) / sr
    return hp(np.random.RandomState(seed).randn(n), 7000, sr) * np.exp(-t / (0.09 if open_ else 0.02)) * 0.6


def crash(sr, dur=2.4, seed=8):
    n = int(dur * sr)
    t = np.arange(n) / sr
    nz = hp(np.random.RandomState(seed).randn(n), 2800, sr)
    return nz * (np.exp(-t / 0.55) * 0.8 + np.exp(-t / 1.6) * 0.3) * (1 - np.exp(-t / 0.004))


def tom(sr, f=110, dur=0.4):
    n = int(dur * sr)
    t = np.arange(n) / sr
    return np.sin(2 * np.pi * np.cumsum(f * (1 + 0.6 * np.exp(-t / 0.05))) / sr) * np.exp(-t / 0.16)


# ---------------------------------------------------------------- tonal
def bass(sr, m, dur, sub=0.7, cut=520):
    f = mid2hz(m)
    n = int(dur * sr) + int(0.04 * sr)
    y = saw(f, n, sr) * 0.6 + sine(f, n, sr) * sub
    y = lp(y, cut, sr)
    return y * adsr(n, sr, 0.004, 0.18, 0.6, 0.04)


def epiano(sr, m, dur, vel=1.0):
    f = mid2hz(m)
    n = int((dur + 0.25) * sr)
    t = np.arange(n) / sr
    idx = 2.2 * np.exp(-t / 0.35)
    mod = np.sin(2 * np.pi * f * t)
    y = np.sin(2 * np.pi * f * t + idx * mod) * np.exp(-t / 0.9)
    y += 0.12 * np.sin(2 * np.pi * f * 14 * t) * np.exp(-t / 0.03)      # the tine
    y *= adsr(n, sr, 0.002, 0.2, 0.55, 0.12)
    return y * vel


def brass(sr, ms, dur, cut=2600, vel=1.0):
    n = int((dur + 0.08) * sr)
    y = np.zeros(n)
    for m in ms:
        f = mid2hz(m)
        for d in (0.994, 1.0, 1.006):
            y += saw(f * d, n, sr, ph=np.random.RandomState(int(m * 7 + d * 100)).rand())
    y = lp(y, cut, sr) + 0.5 * lp(y, cut * 2.2, sr) * np.exp(-np.arange(n) / (sr * 0.05))
    return y * adsr(n, sr, 0.012, 0.12, 0.8, 0.06) * vel / len(ms) * 1.4


def lead(sr, m, dur, vel=1.0, vib=0.18, cut=4200):
    f0 = mid2hz(m)
    n = int((dur + 0.05) * sr)
    t = np.arange(n) / sr
    f = f0 * 2 ** (vib * np.sin(2 * np.pi * 5.6 * t) * np.clip((t - 0.12) / 0.2, 0, 1) / 12)
    y = pulse(f, n, sr, 0.3) * 0.6 + saw(f * 2.003, n, sr) * 0.25 + sine(f, n, sr) * 0.3
    y = lp(y, cut, sr)
    return y * adsr(n, sr, 0.008, 0.1, 0.8, 0.05) * vel


def sax(sr, m, dur, vel=1.0):
    f0 = mid2hz(m)
    n = int((dur + 0.06) * sr)
    t = np.arange(n) / sr
    f = f0 * 2 ** (0.25 * np.sin(2 * np.pi * 5.0 * t) * np.clip((t - 0.1) / 0.25, 0, 1) / 12)
    y = saw(f, n, sr) * 0.7 + pulse(f, n, sr, 0.5) * 0.4
    y = lp(y, 2000, sr) + 0.9 * bp(y, 700, 1300, sr)
    y += 0.05 * bp(np.random.RandomState(4).randn(n), 1500, 5000, sr)
    return y * adsr(n, sr, 0.03, 0.15, 0.85, 0.07) * vel


def pad(sr, ms, dur, cut=1500, vel=1.0, a=0.35, r=0.5):
    n = int((dur + r) * sr)
    y = np.zeros(n)
    for m in ms:
        f = mid2hz(m)
        for d in (0.9965, 1.0, 1.0035):
            y += saw(f * d, n, sr, ph=np.random.RandomState(int(m * 13 + d * 1000)).rand())
    y = lp(y, cut, sr)
    return y * adsr(n, sr, a, 0.3, 0.9, r) * vel / (len(ms) * 1.5)


def riser(sr, dur, f0=400, f1=9000, seed=11):
    n = int(dur * sr)
    nz = np.random.RandomState(seed).randn(n)
    y = sweep_bp(nz, f0, f1, 1.8, sr)
    u = np.arange(n) / n
    return y * u ** 1.6 * 2.2


def pluck(sr, m, dur, vel=1.0):
    f = mid2hz(m)
    n = int((dur + 0.1) * sr)
    t = np.arange(n) / sr
    y = sine(f, n, sr) + 0.4 * sine(f * 2, n, sr) * np.exp(-t / 0.08) + 0.2 * sine(f * 3.01, n, sr) * np.exp(-t / 0.05)
    return y * np.exp(-t / 0.22) * vel


def finish(stereo, peak=0.89):
    """a soft limiter and a peak level"""
    x = np.tanh(stereo * 1.15) / np.tanh(1.15)
    return x / max(1e-9, np.abs(x).max()) * peak
