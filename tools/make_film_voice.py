#!/usr/bin/env python3
"""make_film_voice.py: the voice-over of Missy Teller's opening film "TV SHOW & TELL" (src/cutscene.c, CUT.OVL).

  tools/make_film_voice.py                 build assets/FILMVOX.BIN (the lines, SPU ADPCM, what the disc carries) + print the measurement table
  tools/make_film_voice.py --reel DIR      also write sample reels (mp3 and wav, every line) of the Missy voice options to DIR (docs/voice)
  tools/make_film_voice.py --trim          only trim the tails of the lines already in assets/FILMVOX.BIN (no model needed; build() does it too)
  tools/make_film_voice.py --choose        synthesise every option and print the measurements that rank them (nothing is written)

THE ONE-LINE SETTING is MISSY_VOICE just below: change it and run the script again, everything is regenerated.

How: Kokoro-82M (Apache-2.0 model and weights, hexgrad/Kokoro-82M) run offline through kokoro-onnx (MIT) and onnxruntime. Every line is
written for speaking (not the all-caps subtitle), synthesised with a voice (or a blend of voices: the style vectors are mixed), fitted to its
subtitle window with the speed, then post-processed: high-pass, presence EQ, a gentle de-esser, a compressor, a very light studio plate
reverb, trimmed, loudness-matched, soft-limited, resampled, and encoded to the PlayStation's SPU ADPCM (tools/spuenc.c).
Needs: pip install kokoro-onnx soundfile; ffmpeg (for the mp3 reels); gcc (builds spuenc). The model files (kokoro-v1.0.onnx, voices-v1.0.bin) are
looked for in $KOKORO_DIR (default ~/.cache/kokoro) and fetched from the kokoro-onnx GitHub release when missing.
FILMVOX.BIN: u32 count, then {u32 offset, u32 bytes, u32 rate, u32 steps} per line (steps: its length in 60 Hz film steps), then the ADPCM blocks
(each line starts on a 64-byte boundary); a line with 0 bytes is a silence. Line i belongs to subs[i] of cutscene.c."""
import os, re, sys, struct, subprocess, hashlib, shutil, tempfile
import numpy as np
from scipy import signal

# ======================= THE SETTING ==========================================================================
MISSY_VOICE = 'heart'      # one of MISSY_OPTIONS below
RATE = 22050                   # the sample rate on the disc (the chip plays 22050 Hz mono ADPCM: about 12.6 KB per second)
# ==============================================================================================================

HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
# name: (voice or {voice: weight} blend, speed, pitch lift in semitones, which text variant style)
MISSY_OPTIONS = {
    'nicole':     ({'af_nicole': 1.0}, 1.10, 0.6),
    'sky_bella':  ({'af_sky': 0.55, 'af_bella': 0.45}, 1.10, 0.5),
    'heart_sky':  ({'af_heart': 0.5, 'af_sky': 0.5}, 1.12, 0.5),
    'bella':      ({'af_bella': 1.0}, 1.10, 0.4),
    'jessica':    ({'af_jessica': 1.0}, 1.10, 0.3),
    'nova_nicole': ({'af_nova': 0.5, 'af_nicole': 0.5}, 1.12, 0.5),
    'sarah':      ({'af_sarah': 1.0}, 1.12, 0.4),
    'heart':      ({'af_heart': 1.0}, 1.10, 0.4),
}
CHAD = ({'am_michael': 0.7, 'am_fenrir': 0.3}, 1.10, 0.0)
INTERN = ({'am_puck': 1.0}, 1.14, 2.0)

# the spoken text of each subtitle, by index (cutscene.c subs[]): who, variants (the first that fits best wins)
SPOKEN = [
    (1, ['Good evening, Boretown!']),
    (1, ['Welcome back to Dancing With the Bars!']),
    (1, ['Our judge needs no introduction...']),
    (1, ['...but she gets one anyway!']),
    (1, ['Missy Teller!']),
    (2, ['Hello, darlings!', 'Hello, darlings.', 'Well, hello, darlings!']),
    (2, ['Try not to bore me tonight.', 'Try not to bore me tonight!', 'Try... not to bore me tonight.']),
    (2, ['Somebody get me a drink!', 'Somebody, get me a drink.', 'Somebody get me a drink.']),
    (3, ['Y-your usual, Miss Teller.', 'Your usual, Miss Teller?']),
    (2, ['Hmm.']),
    (2, ['Not tonight, darling!', 'Not tonight, darling.', 'Not tonight... darling.']),
    (1, ['She is back, folks!']),
    (2, ['Watch me dance!', 'Watch me dance.']),
]

def kdir():
    d = os.environ.get('KOKORO_DIR', os.path.expanduser('~/.cache/kokoro')); os.makedirs(d, exist_ok=True)
    for f in ('kokoro-v1.0.onnx', 'voices-v1.0.bin'):
        p = os.path.join(d, f)
        if not os.path.exists(p) and os.path.exists(os.path.join(d, 'kokoro.onnx' if f.endswith('onnx') else 'voices.bin')):
            os.symlink(os.path.join(d, 'kokoro.onnx' if f.endswith('onnx') else 'voices.bin'), p)
        if not os.path.exists(p):
            subprocess.check_call(['curl', '-fsSL', '-o', p, 'https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/' + f])
    return d

_k = None
def kokoro():
    global _k
    if _k is None:
        from kokoro_onnx import Kokoro
        d = kdir(); _k = Kokoro(os.path.join(d, 'kokoro-v1.0.onnx'), os.path.join(d, 'voices-v1.0.bin'))
    return _k

CACHE = os.environ.get('FILMVOICE_CACHE', os.path.join(tempfile.gettempdir(), 'filmvoice_cache')); os.makedirs(CACHE, exist_ok=True)
def synth(text, blend, speed):
    key = hashlib.md5(repr((text, sorted(blend.items()), round(speed, 3))).encode()).hexdigest(); fn = os.path.join(CACHE, key + '.npy')
    if os.path.exists(fn): return np.load(fn)
    k = kokoro()
    style = sum(w * k.get_voice_style(v) for v, w in blend.items()) / sum(blend.values())
    s, sr = k.create(text, voice=style, speed=speed, lang='en-us'); assert sr == 24000
    np.save(fn, s.astype(np.float32)); return s.astype(np.float32)

SR = 24000
def peaking(f0, gain_db, q, sr=SR):   # RBJ biquad
    A = 10 ** (gain_db / 40); w = 2 * np.pi * f0 / sr; al = np.sin(w) / (2 * q); c = np.cos(w)
    b = [1 + al * A, -2 * c, 1 - al * A]; a = [1 + al / A, -2 * c, 1 - al / A]; return np.array(b) / a[0], np.array(a) / a[0]
def shelf(f0, gain_db, high, sr=SR):
    A = 10 ** (gain_db / 40); w = 2 * np.pi * f0 / sr; c = np.cos(w); s = np.sin(w); be = np.sqrt(A) * s * np.sqrt(2) / 1
    if high:
        b = [A * ((A + 1) + (A - 1) * c + be), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - be)]
        a = [(A + 1) - (A - 1) * c + be, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - be]
    else:
        b = [A * ((A + 1) - (A - 1) * c + be), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - be)]
        a = [(A + 1) + (A - 1) * c + be, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - be]
    return np.array(b) / a[0], np.array(a) / a[0]
def env_follow(x, att_ms, rel_ms, sr=SR):   # a peak follower, attack and release
    a = np.exp(-1 / (sr * att_ms / 1000)); r = np.exp(-1 / (sr * rel_ms / 1000)); e = np.zeros_like(x); v = 0.0
    ax = np.abs(x)
    for i in range(len(x)):
        s = ax[i]; v = s + (a if s > v else r) * (v - s); e[i] = v
    return e

def fx(x, who, reverb=0.10):
    x = signal.sosfilt(signal.butter(2, 85, 'hp', fs=SR, output='sos'), x)
    if who == 2:   eq = [peaking(3800, 3.2, 0.9), peaking(250, -1.5, 0.8), shelf(8500, 1.6, True)]       # Missy: presence and air
    elif who == 1: eq = [shelf(220, 2.0, False), peaking(3000, 2.4, 0.9), peaking(500, -1.0, 1.0)]       # Chad: warm and forward
    else:          eq = [peaking(3500, 2.0, 0.9), shelf(8500, 1.0, True)]
    for b, a in eq: x = signal.lfilter(b, a, x)
    # de-esser: the 5.5-9 kHz band is pulled down where it is loud
    band = signal.sosfilt(signal.butter(2, [5500, 9500], 'bp', fs=SR, output='sos'), x)
    e = env_follow(band, 1.0, 25.0); thr = 0.02 + 0.0
    g = np.clip(thr / np.maximum(e, 1e-6), 0.4, 1.0)
    x = x - band * (1 - g)
    # compressor: feed-forward, 3:1 over -24 dBFS, 4 ms attack, 90 ms release
    e = env_follow(x, 4.0, 90.0); db = 20 * np.log10(np.maximum(e, 1e-5)); over = np.maximum(db + 24, 0)
    x = x * 10 ** (-(over * (1 - 1 / 3.0)) / 20)
    # trim the silences
    ae = env_follow(x, 2.0, 30.0); on = np.where(ae > 10 ** (-46 / 20))[0]
    if len(on) == 0: return x[:SR // 20] * 0
    i0 = max(0, on[0] - int(0.03 * SR)); i1 = min(len(x), on[-1] + int(0.05 * SR)); x = x[i0:i1]
    # a light studio plate: short bright-ish tail, a pre-delay, damped highs
    rng = np.random.RandomState(7); n = int(0.30 * SR); t = np.arange(n) / SR
    ir = rng.randn(n) * np.exp(-t / 0.085); ir = signal.sosfilt(signal.butter(2, 5200, 'lp', fs=SR, output='sos'), ir)
    ir = np.concatenate([np.zeros(int(0.011 * SR)), ir]); ir /= np.sqrt((ir ** 2).sum())
    wet = signal.fftconvolve(x, ir)[:len(x) + int(0.16 * SR)]
    out = np.concatenate([x, np.zeros(len(wet) - len(x))]) + reverb * wet
    fo = int(0.06 * SR); out[-fo:] *= np.linspace(1, 0, fo)
    return out

def loud_match(x, rms_db=-19.5):
    ae = env_follow(x, 5, 60); m = ae > 10 ** (-40 / 20)
    r = np.sqrt(np.mean(x[m] ** 2)) if m.any() else 1e-4
    x = x * (10 ** (rms_db / 20) / r)
    return np.tanh(x * 0.9) / np.tanh(0.9) * 0.97      # a soft limiter

def lift(x, semis):   # a small pitch lift by resampling (the speed was lowered by the same ratio before synthesis)
    if abs(semis) < 0.01: return x
    f = 2 ** (semis / 12); from fractions import Fraction
    fr = Fraction(f).limit_denominator(200); return signal.resample_poly(x, fr.denominator, fr.numerator)

def f0_track(x, sr=SR):   # frame-wise autocorrelation pitch: Hz per voiced frame
    hop = int(0.01 * sr); w = int(0.04 * sr); out = []
    lo, hi = int(sr / 420), int(sr / 70)
    for i in range(0, len(x) - w - hi, hop):
        fr = x[i:i + w + hi]; seg = fr[:w]
        if np.sqrt(np.mean(seg ** 2)) < 0.02: continue
        ac = np.array([np.dot(seg, fr[l:l + w]) for l in range(lo, hi)]); e0 = np.dot(seg, seg)
        k = int(np.argmax(ac));
        if ac[k] / max(e0, 1e-9) > 0.55: out.append(sr / (lo + k))
    return np.array(out)

def measure(x, text):
    f = f0_track(x); words = len(re.findall(r"[A-Za-z']+", text))
    ae = env_follow(x, 5, 60); act = (ae > 10 ** (-40 / 20)).sum() / SR
    S = np.abs(np.fft.rfft(x * np.hanning(len(x)))); fr = np.fft.rfftfreq(len(x), 1 / SR)
    return dict(f0=float(np.median(f)) if len(f) else 0.0, st=float(np.std(12 * np.log2(f / np.median(f)))) if len(f) > 3 else 0.0,
                wps=words / max(act, 0.1), cent=float((S * fr).sum() / max(S.sum(), 1e-9)), peak=float(np.abs(x).max()), dur=len(x) / SR)

def parse_subs():   # (t0, t1, who) in tenths, straight from cutscene.c
    src = open(os.path.join(ROOT, 'src/cutscene.c')).read(); m = re.search(r'static const Sub subs\[\] = \{(.*?)\} \};', src, re.S)
    return [(int(a), int(b), int(c)) for a, b, c in re.findall(r'\{\s*T\((\d+)\),\s*T\((\d+)\),\s*(\d),', m.group(1))]

def make_line(i, opt=None, reverb=0.10, fit=True, variants=None):
    who, texts = SPOKEN[i]; subs = parse_subs()
    blend, speed, semis = opt if opt else (MISSY_OPTIONS[MISSY_VOICE] if who == 2 else CHAD if who == 1 else INTERN)
    t0, t1, _ = subs[i]; nxt = subs[i + 1][0] if i + 1 < len(subs) else t1 + 30
    room = (min(nxt - 2, t1 + 12) - t0) / 10.0 - 0.15    # seconds the line may take: its window, a little over, never into the next line
    best = None
    for tx in (variants or texts):
        sp = speed
        for _ in range(6):
            x = synth(tx, blend, sp / (2 ** (semis / 12)))
            y = fx(lift(x, semis), who, reverb)
            if not fit or len(y) / SR <= room or sp >= speed * 1.22: break
            sp *= 1.06
        m = measure(y, tx)
        cand = (tx, y, m, sp)
        if best is None: best = cand
        if variants is None and tx == texts[0]: break      # the first text is the written one; --choose compares them
    tx, y, m, sp = best
    return tx, loud_match(y), m, sp, room

def to_adpcm(y):
    pcm = signal.resample_poly(y, RATE // 50, SR // 50) if SR % 50 == 0 else y
    pcm16 = np.clip(pcm * 32767, -32768, 32767).astype('<i2').tobytes()
    enc = os.path.join(tempfile.gettempdir(), 'spuenc')
    if not os.path.exists(enc) or os.path.getmtime(enc) < os.path.getmtime(os.path.join(HERE, 'spuenc.c')):
        subprocess.check_call(['gcc', '-O2', '-o', enc, os.path.join(HERE, 'spuenc.c')])
    out = os.path.join(tempfile.gettempdir(), 'line.adpcm')
    subprocess.run([enc, out], input=pcm16, check=True)
    return open(out, 'rb').read(), len(pcm)

def decode_adpcm(b):   # the chip's decoder, to check the encoder (returns int16 samples)
    F0 = [0, 60, 115, 98, 122]; F1 = [0, 0, -52, -55, -60]; h1 = h2 = 0; o = []
    for p in range(0, len(b), 16):
        sh, f = b[p] & 15, (b[p] >> 4) & 7
        for i in range(28):
            nb = (b[p + 2 + i // 2] >> (4 * (i & 1))) & 15; nb = nb - 16 if nb > 7 else nb
            s = ((nb << 12) >> sh) + ((h1 * F0[f] + h2 * F1[f] + 32) >> 6); s = max(-32768, min(32767, s)); o.append(s); h2 = h1; h1 = s
    return np.array(o, dtype=np.int16)

def trim_vox(path, thr=0.0006):
    """the lines end in reverb tails that fade below -64 dB long before their last block: cut each line after its last block that rises over thr and flag that block "end"
    (the chip mutes the voice there: nothing audible is lost, no click: the block is at -64 dB), a few KB of sound memory saved per line. The lengths in 60 Hz steps (the film's clock) are not touched. Idempotent."""
    d = open(path, 'rb').read(); n = struct.unpack('<I', d[:4])[0]; ents = []; blob = bytearray()
    hdr = (4 + 16 * n + 63) & ~63; off = hdr; tab = struct.pack('<I', n); before = len(d)
    for i in range(n):
        o, ln, rate, steps = struct.unpack('<IIII', d[4 + 16 * i:20 + 16 * i]); b = bytearray(d[o:o + ln])
        if ln:
            x = decode_adpcm(bytes(b)).astype(np.float32) / 32768; blocks = len(b) // 16
            amp = np.abs(x[:blocks * 28]).reshape(blocks, 28).max(axis=1); keep = int(np.max(np.nonzero(amp > thr)[0])) + 1 if (amp > thr).any() else 1
            keep = min(blocks, keep + 1); b = b[:keep * 16]
            for k in range(keep): b[k * 16 + 1] = 0
            b[(keep - 1) * 16 + 1] = 1
        tab += struct.pack('<IIII', off, len(b), rate, steps); pad = (-len(b)) % 64; blob += bytes(b) + bytes(pad); off += len(b) + pad
    tab += bytes(hdr - len(tab)); out = tab + bytes(blob); open(path, 'wb').write(out)
    print('FILMVOX.BIN trimmed: %d -> %d bytes (%.1f KB saved)' % (before, len(out), (before - len(out)) / 1024))

def write_wav_mp3(path, x, sr):
    import soundfile as sf
    sf.write(path + '.wav', x, sr)
    if shutil.which('ffmpeg'):
        subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', path + '.wav', '-b:a', '128k', path + '.mp3'], check=True); os.remove(path + '.wav')

def score_missy(m):   # a poppy voice: bright, wide in pitch, quick and clean
    s = 0.0
    s -= abs(m['f0'] - 240) / 40          # the pitch of a bright pop voice
    s += min(m['st'], 5.0)                # how much the pitch moves (an expressive voice)
    s -= abs(m['wps'] - 3.6) * 0.8        # a quick, confident pace
    s += (min(m['cent'], 2800) - 1500) / 600
    s -= 5 * (m['peak'] > 0.99)
    return s

def reel(dirn):
    rows = []
    for name, opt in MISSY_OPTIONS.items():
        ms = []; parts = []
        for i, (who, texts) in enumerate(SPOKEN):
            if who != 2: continue
            tx, y, m, sp, room = make_line(i, opt); ms.append(m); parts.append(y)
        avg = {k: float(np.mean([m[k] for m in ms])) for k in ms[0]}
        rows.append((score_missy(avg), name, avg, ms))
        print('%-12s f0 %5.0f Hz  range %.1f st  %.1f words/s  centroid %4.0f Hz  peak %.2f  score %.2f' % (name, avg['f0'], avg['st'], avg['wps'], avg['cent'], avg['peak'], score_missy(avg)))
    return sorted(rows, reverse=True)

def build(outdir, reel_dir=None):
    subs = parse_subs(); assert len(subs) == len(SPOKEN), 'subs[] and SPOKEN differ'
    lines = []; total = 0
    print('line  who  window   steps  bytes   speed text')
    for i, (who, texts) in enumerate(SPOKEN):
        tx, y, m, sp, room = make_line(i)
        ad, n = to_adpcm(y); dec = decode_adpcm(ad)[:n].astype(np.float32) / 32768
        ref = signal.resample_poly(y, RATE // 50, SR // 50)[:len(dec)]; snr = 10 * np.log10((ref ** 2).sum() / max(((ref - dec[:len(ref)]) ** 2).sum(), 1e-12))
        steps = int(np.ceil(n / RATE * 60)); t0, t1, _ = subs[i]
        lines.append((ad, steps)); total += len(ad)
        print('%3d   %d   %4.1fs  %4d  %6d  %.2f  adpcm SNR %4.1f dB  F0 %3.0f Hz %-30s' % (i, who, (t1 - t0) / 10.0, steps, len(ad), sp, snr, m['f0'], tx))
    hdr = 4 + 16 * len(lines); hdr = (hdr + 63) & ~63; blob = bytearray(); off = hdr; tab = struct.pack('<I', len(lines))
    for ad, steps in lines:
        tab += struct.pack('<IIII', off, len(ad), RATE, steps); pad = (-len(ad)) % 64; blob += ad + bytes(pad); off += len(ad) + pad
    tab += bytes(hdr - len(tab))
    open(os.path.join(outdir, 'FILMVOX.BIN'), 'wb').write(tab + blob)
    print('FILMVOX.BIN: %d bytes (%.0f KB), %d Hz, voice %s' % (hdr + len(blob), (hdr + len(blob)) / 1024, RATE, MISSY_VOICE))
    trim_vox(os.path.join(outdir, 'FILMVOX.BIN'))
    if reel_dir: build_reels(reel_dir)

def build_reels(dirn):
    os.makedirs(dirn, exist_ok=True)
    rows = reel(dirn); top = [r[1] for r in rows[:4]]
    if MISSY_VOICE not in top: top = top[:3] + [MISSY_VOICE]
    print('reel options:', top)
    for name in top:
        seq = []; gap = np.zeros(int(0.35 * SR))
        for i, (who, texts) in enumerate(SPOKEN):
            opt = MISSY_OPTIONS[name] if who == 2 else None
            tx, y, m, sp, room = make_line(i, opt)
            seq += [y, gap]
        write_wav_mp3(os.path.join(dirn, 'film_voice_missy_' + name), np.concatenate(seq).astype(np.float32), SR)

if __name__ == '__main__':
    a = sys.argv[1:]
    if '--trim' in a:
        trim_vox(os.path.join(ROOT, 'assets', 'FILMVOX.BIN'))      # (no model needed: only the tails of the lines already on disc)
    elif '--choose' in a:
        for r in reel(None): pass
    elif '--reel' in a:
        build(os.path.join(ROOT, 'assets'), a[a.index('--reel') + 1])
    else:
        build(os.path.join(ROOT, 'assets'))
