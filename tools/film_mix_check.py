#!/usr/bin/env python3
"""film_mix_check.py FILM.wav [--stems DIR] [--png DIR] [--json OUT]: measure the sound of the film in the test emulator's SPU dump (DSWAV, 44.1 kHz stereo).

  - finds the film in the dump: every voice line of FILMVOX.BIN by matched filtering (as tools/film_verify.py); the film's clock origin is the median of what they say
  - sync: each line's start against its subtitle (t0 + 3 steps), in ms
  - loudness of the film (EBU R128 through ffmpeg): integrated LUFS, loudness range, true peak (dBTP), sample peak
  - with --stems DIR (voice.wav, music.wav, crowd.wav, foley.wav, cue.wav: the same run with -DMIX_SOLO=1,2,4,8,16): voice level against the music, crowd and foley while each line is spoken
    (the speech frames of the line only), and the music's level with and without the duck
  - clicks: sample discontinuities (the second difference against its own neighbourhood) in the mix and in each stem, with the places of the worst
  - --png DIR: waveform with the lines marked and a spectrogram of the mix, a stack of the stems
Needs numpy, scipy, matplotlib, ffmpeg."""
import sys, os, re, struct, json, subprocess, tempfile
import numpy as np
from scipy.io import wavfile
from scipy.signal import fftconvolve, resample_poly

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SR = 44100
FR = 44100 / (53693175 / (263 * 3413))      # samples per emulator frame (59.82 Hz); one film step is one frame
C0 = [0, 60, 115, 98, 122]
C1 = [0, 0, -52, -55, -60]


def adpcm(b):
    out = []
    s1 = s2 = 0
    for i in range(0, len(b) - 15, 16):
        sh = b[i] & 15
        f = min(b[i] >> 4, 4)
        for j in range(28):
            n = (b[i + 2 + j // 2] >> (4 * (j & 1))) & 15
            if n > 7: n -= 16
            v = ((n << 12) >> sh) + ((s1 * C0[f] + s2 * C1[f] + 32) >> 6)
            v = max(-32768, min(32767, v)); out.append(v); s2 = s1; s1 = v
    return np.array(out, float)


def load(path):
    sr, x = wavfile.read(path)
    assert sr == SR, sr
    x = x.astype(np.float64) / 32768
    return x if x.ndim == 2 else np.stack([x, x], 1)


def subs_t0():
    t = open(os.path.join(ROOT, 'src', 'cutscene.c')).read()
    m = re.search(r'static const Sub subs\[\] = \{(.*?)\} \};', t, re.S)
    return [(int(a), int(b)) for a, b in re.findall(r'\{\s*T\((\d+)\),\s*T\((\d+)\),\s*\d,', m.group(1))]


def lines():
    d = open(os.path.join(ROOT, 'assets', 'FILMVOX.BIN'), 'rb').read()
    n = struct.unpack('<I', d[:4])[0]
    out = []
    for i in range(n):
        off, ln, rate, steps = struct.unpack('<IIII', d[4 + 16 * i:20 + 16 * i])
        r = adpcm(d[off:off + ln])
        out.append((resample_poly(r, SR, rate) if rate != SR else r, steps))
    return out


def find_lines(mid, vox, t0s):
    """for each line the sample where it is found (matched filtering on the first difference around where it should be), its score"""
    found = []
    for i, (r, steps) in enumerate(vox):
        rr = np.diff(r)
        c = fftconvolve(np.diff(mid), rr[::-1], 'valid')
        e = np.sqrt(np.maximum(fftconvolve(np.diff(mid) ** 2, np.ones(len(rr)), 'valid'), 0)) * np.sqrt((rr * rr).sum()) + 1e-3 * len(rr)
        nc = c / e
        k = int(np.argmax(nc))
        found.append((k, float(nc[k])))
    return found


def ebur128(x):
    with tempfile.TemporaryDirectory() as d:
        p = os.path.join(d, 'a.wav')
        wavfile.write(p, SR, (np.clip(x, -1, 1) * 32767).astype('<i2'))
        r = subprocess.run(['ffmpeg', '-nostats', '-i', p, '-af', 'ebur128=peak=true', '-f', 'null', '-'], capture_output=True, text=True).stderr
    s = r[r.rfind('Summary:'):]
    g = lambda pat: float(re.search(pat, s).group(1))
    return dict(lufs=g(r'I:\s+(-?[\d.]+) LUFS'), lra=g(r'LRA:\s+(-?[\d.]+) LU'), tp=g(r'Peak:\s+(-?[\d.]+) dBFS'))


def rms_db(x):
    return 20 * np.log10(max(1e-9, np.sqrt(np.mean(x ** 2))))


def clicks(x, thr_ratio=10.0, floor=0.012):
    """a click is an isolated jump: the second difference at one place far over the mean second difference on BOTH sides of it (a speech onset or a drum hit is loud on one side, a click is not)"""
    m = x if x.ndim == 1 else x.mean(1)
    d2 = np.abs(m[2:] - 2 * m[1:-1] + m[:-2])
    k = 40
    c = np.concatenate([[0], np.cumsum(d2)])
    n = len(d2)
    idx = np.arange(n)
    lo = np.clip(idx - k, 0, n); lo2 = np.clip(idx - 3, 0, n); hi = np.clip(idx + 4, 0, n); hi2 = np.clip(idx + k + 1, 0, n)
    left = (c[lo2] - c[lo]) / np.maximum(lo2 - lo, 1)
    right = (c[hi2] - c[hi]) / np.maximum(hi2 - hi, 1)
    ref = np.maximum(left, right)
    hit = np.where((d2 > thr_ratio * np.maximum(ref, 1e-4)) & (d2 > floor))[0]
    ev = []
    for h in hit:
        if not ev or h - ev[-1][0] > 50: ev.append([h, d2[h] / max(ref[h], 1e-4), d2[h]])
    return ev


def align(st, mix, i0, i1, name):
    """a stem is a separate emulator run: it can start a few ms (or frames) off the mix's; the lag that best matches it to the mix (the stem is part of the mix) is taken out"""
    a = st[i0:i1].mean(1); b = mix[i0:i1].mean(1)
    if np.abs(a).max() < 1e-4: return st
    n = min(len(a), len(b)); a = a[:n]; b = b[:n]
    c = fftconvolve(b, a[::-1], 'full'); mid = n - 1
    w = int(1.0 * SR); seg = c[mid - w:mid + w + 1]; lag = int(np.argmax(seg)) - w       # lag > 0: the stem is early (b is later)
    if lag:
        print('  (stem %s moved by %+.1f ms to sit on the mix)' % (name, lag / SR * 1000))
        st = np.roll(st, lag, axis=0)
    return st


def selftest():
    """the click detector on a known signal: music-like noise bursts (onsets must NOT be flagged), a tone, and clicks injected (a step, a one-sample spike) which must be"""
    rng = np.random.RandomState(1)
    x = np.zeros(SR * 6)
    x[SR:SR * 2] = rng.randn(SR) * 0.1 * np.hanning(SR) ** 0.2                   # a noise burst starting abruptly (an onset)
    x[SR * 3:SR * 5] = 0.3 * np.sin(2 * np.pi * 220 * np.arange(SR * 2) / SR)      # a tone
    base = len(clicks(x))
    y = x.copy(); y[int(SR * 4.0):] += 0.15                                        # a step (a DC jump) at 4.0 s in the tone
    z = x.copy(); z[int(SR * 3.5)] += 0.5                                          # a one-sample spike at 3.5 s
    w = x.copy(); w[int(SR * 0.5)] += 0.3                                          # a spike in silence
    for nme, sig, at in (('plain', x, None), ('step', y, 4.0), ('spike', z, 3.5), ('spike in silence', w, 0.5)):
        ev = clicks(sig)
        hit = [e for e in ev if at is not None and abs(e[0] / SR - at) < 0.01]
        print('selftest %-17s %d flagged%s' % (nme, len(ev), '; the injected one: ratio x%.0f' % hit[0][1] if hit else ''))


def main():
    a = sys.argv[1:]
    if '--selftest' in a: return selftest()
    wav = a[0]
    opt = lambda k: a[a.index(k) + 1] if k in a else None
    x = load(wav)
    mid = x.mean(1)
    vox = lines()
    t0s = subs_t0()
    found = find_lines(mid, vox, t0s)
    # the film's clock origin: found - (t0 * 6 + 3) frames, the median over the lines (a line that is not found well is left out)
    og = [f[0] - (t0s[i][0] * 6 + 3) * FR for i, f in enumerate(found) if f[1] > 0.45]
    origin = float(np.median(og)) if og else 0.0
    res = dict(origin_s=origin / SR)
    print('film clock origin: %.3f s in the dump' % (origin / SR))
    lags = []
    for i, (k, sc) in enumerate(found):
        lag = (k - origin - (t0s[i][0] * 6 + 3) * FR) / SR * 1000
        lags.append(lag)
        print('  line %2d  found at film %.2f s  lag to its subtitle %+6.1f ms  (score %.2f)' % (i + 1, (k - origin) / SR, lag, sc))
    res['lag_ms'] = lags
    res['worst_lag_ms'] = float(np.max(np.abs(lags)))
    print('  worst lag %.1f ms (spread of the lags: %.1f ms)' % (res['worst_lag_ms'], float(np.ptp(lags))))
    film_len = 61.0
    i0 = int(max(0, origin))
    i1 = int(min(len(x), origin + film_len * 59.817 / 60.0 * SR + 0.5 * SR))
    seg = x[i0:i1]
    L = ebur128(seg)
    res.update(L)
    pk = float(np.abs(seg).max())
    res['sample_peak_dbfs'] = 20 * np.log10(pk)
    print('loudness of the film: integrated %.1f LUFS, range %.1f LU, true peak %.1f dBTP, sample peak %.2f dBFS, clipped samples %d' % (L['lufs'], L['lra'], L['tp'], res['sample_peak_dbfs'], int((np.abs(seg) >= 0.9999).sum())))
    wins = []
    for i, (r, steps) in enumerate(vox):
        s = int(found[i][0] - 0.0 * SR)
        wins.append((s, s + len(r)))
    stems = {}
    sd = opt('--stems')
    if sd:
        for nme in ('voice', 'music', 'crowd', 'foley', 'cue'):
            p = os.path.join(sd, nme + '.wav')
            if os.path.exists(p): stems[nme] = align(load(p), x, i0, i1, nme)
        print('stem peaks (dBFS): ' + ', '.join('%s %.1f' % (k, 20 * np.log10(max(1e-9, np.abs(v[i0:i1]).max()))) for k, v in stems.items()))
        print('levels while a line is spoken (the line\'s speech frames, dB RMS; bus - voice):')
        rows = {k: [] for k in stems if k != 'voice'}
        vv = []
        for i, (s, e) in enumerate(wins):
            v = stems['voice'][s:e].mean(1)
            fr = int(0.02 * SR)
            act = np.array([np.sqrt(np.mean(v[j:j + fr] ** 2)) for j in range(0, len(v) - fr, fr)]) > 10 ** (-42 / 20)
            mask = np.repeat(act, fr)
            n = min(len(mask), len(v))
            vr = rms_db(v[:n][mask[:n]])
            vv.append(vr)
            line = '  line %2d voice %6.1f dBFS' % (i + 1, vr)
            for k in rows:
                o = stems[k][s:e].mean(1)[:n][mask[:n]]
                d = rms_db(o) - vr
                rows[k].append(d)
                line += '  %s %+6.1f' % (k, d)
            print(line)
        for k in rows: print('  %-6s relative to the voice: mean %+.1f dB, loudest %+.1f dB' % (k, float(np.mean(rows[k])), float(np.max(rows[k]))))
        res['voice_dbfs'] = float(np.mean(vv))
        res['rel_to_voice'] = {k: dict(mean=float(np.mean(v)), max=float(np.max(v))) for k, v in rows.items()}
        # the score with and without the duck: 1 s before a line's start against 0.5 s into it (the score's level)
        mu = stems['music'].mean(1)
        gaps = []
        for i, (s, e) in enumerate(wins):
            if i == 0: continue
            pre_s, pre_e = int(s - 0.9 * SR), int(s - 0.2 * SR)
            a_ = rms_db(mu[pre_s:pre_e]); b_ = rms_db(mu[s + int(0.3 * SR):s + int(0.9 * SR)])
            gaps.append((i + 1, a_, b_))
        res['music_before_during'] = gaps
        print('score level before a line (0.9 to 0.2 s ahead) and in it (0.3 to 0.9 s): ' + ', '.join('%d: %.1f -> %.1f' % g for g in gaps))
    cl = {}
    for nme, sig in [('mix', seg)] + [(k, v[i0:i1]) for k, v in stems.items()]:
        ev = clicks(sig)
        cl[nme] = ev
        worst = sorted(ev, key=lambda e: -e[1])[:4]
        print('clicks in %-6s: %3d candidates; worst at %s' % (nme, len(ev), ', '.join('%.3f s (x%.0f)' % (e[0] / SR, e[1]) for e in worst) if worst else 'none'))
    res['clicks'] = {k: len(v) for k, v in cl.items()}
    # starts and ends: how loud is the first and last 20 ms, and the jump at the first non-zero sample
    f0 = np.abs(seg[:int(0.02 * SR)]).max(); f1 = np.abs(seg[-int(0.02 * SR):]).max()
    print('first 20 ms peak %.4f, last 20 ms peak %.4f' % (f0, f1))
    res['edge_peaks'] = [float(f0), float(f1)]
    png = opt('--png')
    if png:
        os.makedirs(png, exist_ok=True)
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        t = np.arange(len(seg)) / SR
        fig, ax = plt.subplots(2, 1, figsize=(14, 7), sharex=True)
        ax[0].plot(t[::20], seg[::20, 0], lw=0.4, label='L'); ax[0].plot(t[::20], seg[::20, 1], lw=0.4, label='R', alpha=0.7)
        for i, (s, e) in enumerate(wins):
            ax[0].axvspan((s - i0) / SR, (e - i0) / SR, color='orange', alpha=0.25)
        ax[0].set_ylim(-1.05, 1.05); ax[0].legend(loc='upper right'); ax[0].set_title('film mix (voice lines shaded)')
        ax[1].specgram(seg.mean(1), NFFT=2048, Fs=SR, noverlap=1536, cmap='magma', vmin=-130, vmax=-30); ax[1].set_ylim(0, 12000); ax[1].set_xlabel('s')
        fig.tight_layout(); fig.savefig(os.path.join(png, 'mix_wave_spec.png'), dpi=90); plt.close(fig)
        if stems:
            fig, ax = plt.subplots(len(stems), 1, figsize=(14, 1.6 * len(stems)), sharex=True)
            for a_, (k, v) in zip(np.atleast_1d(ax), stems.items()):
                s_ = v[i0:i1]; tt = np.arange(len(s_)) / SR
                a_.plot(tt[::20], s_[::20, 0], lw=0.4); a_.plot(tt[::20], s_[::20, 1], lw=0.4, alpha=0.6); a_.set_ylim(-1, 1); a_.set_ylabel(k)
            fig.tight_layout(); fig.savefig(os.path.join(png, 'stems.png'), dpi=80); plt.close(fig)
    if opt('--json'): json.dump(res, open(opt('--json'), 'w'), indent=1, default=float)


main()
