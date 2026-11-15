#!/usr/bin/env python3
"""film_verify.py FILM.wav FIRST_FRAME [FILMVOX.BIN]: where does each voice line of the film sit in the SPU dump?
The dump (DSWAV, 44.1 kHz stereo) starts at emulator frame 0, one emulator frame = 737.2 samples (59.82 Hz). The film's clock starts at emulator frame FIRST_FRAME
(the first non-black picture of the film's fade in). Every line of FILMVOX.BIN (SPU ADPCM, decoded here) is found in the dump by matched filtering
(normalised cross-correlation of the first-differenced waveform) around where the film's clock says it starts (its subtitle's t0 + 3 steps), and the
distance in ms is printed. Needs numpy and scipy."""
import sys, struct, wave, re, os
import numpy as np
from scipy.signal import fftconvolve, resample_poly
FR = 44100 / (53693175 / (263 * 3413))   # samples per emulator frame: 737.2 (the PS1 NTSC 240-line frame is 263 lines of 3413 clocks = 59.82 Hz)
C0 = [0, 60, 115, 98, 122]; C1 = [0, 0, -52, -55, -60]


def adpcm(b):
    out = []; s1 = s2 = 0
    for i in range(0, len(b) - 15, 16):
        sh = b[i] & 15; f = min(b[i] >> 4, 4)
        for j in range(28):
            n = (b[i + 2 + j // 2] >> (4 * (j & 1))) & 15
            if n > 7: n -= 16
            v = (n << 12) >> sh
            v += (s1 * C0[f] + s2 * C1[f] + 32) >> 6
            v = max(-32768, min(32767, v)); out.append(v); s2 = s1; s1 = v
    return np.array(out, float)


def subs_t0(src):
    t = open(src).read(); m = re.search(r'static const Sub subs\[\] = \{(.*?)\} \};', t, re.S)
    return [int(x) for x in re.findall(r'\{ T\((\d+)\),\s*T\(\d+\),\s*\d,', m.group(1))]


def main():
    wav, first = sys.argv[1], int(sys.argv[2]); here = os.path.dirname(os.path.abspath(__file__))
    vox = sys.argv[3] if len(sys.argv) > 3 else os.path.join(here, '..', 'assets', 'FILMVOX.BIN')
    w = wave.open(wav); x = np.frombuffer(w.readframes(w.getnframes()), '<i2').reshape(-1, 2).astype(float).mean(1)
    d = open(vox, 'rb').read(); n = struct.unpack('<I', d[:4])[0]; t0 = subs_t0(os.path.join(here, '..', 'src', 'cutscene.c'))
    worst = 0; lags = []
    for i in range(n):
        off, ln, rate, steps = struct.unpack('<IIII', d[4 + 16 * i:20 + 16 * i])
        r = adpcm(d[off:off + ln]); r = resample_poly(r, 44100, rate) if rate != 44100 else r
        exp = int((first + t0[i] * 6 + 3) * FR)          # expected start (sample): film step s is emulator frame first + s
        a, b = max(0, exp - 44100), exp + 44100 + len(r)
        seg = x[a:b]
        rr = np.diff(r); ss = np.diff(seg)
        c = fftconvolve(ss, rr[::-1], 'valid')
        e = np.sqrt(np.maximum(fftconvolve(ss * ss, np.ones(len(rr)), 'valid'), 0)) * np.sqrt((rr * rr).sum()) + 1e-6 * len(rr) * 1e3
        nc = c / e; k = int(np.argmax(nc)); lag = (a + k - exp) / 44.1
        worst = max(worst, abs(lag)); lags.append(lag)
        print('line %2d  film t %5.1f s  found %+6.1f ms from the film clock  (score %.2f)' % (i + 1, t0[i] / 10.0, lag, nc[k]))
    print('worst %.1f ms' % worst)
    print('MEDIAN_MS %.1f' % float(np.median(lags)))   # (the sound's lead on the picture: film_to_mp4.sh moves the sound by this)


main()
