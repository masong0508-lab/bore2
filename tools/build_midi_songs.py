#!/usr/bin/env python3
"""BORE 2: songs that only exist as renders (the HERE TODAY family) made into SEQUENCES, like every other jukebox song: the notes,
plus one small sample per instrument, so a 12 minute song is a few KB of notes instead of megabytes of recording.

The GBA game makes these songs with Python scripts (tools/make_heretoday*.py) that read the author's MIDI and synthesize every note
themselves. This runs each script as it is, but catches every note at the moment the script mixes it in (its put()): the instrument
(which synth made it), the pitch, when it starts and ends, how loud. It keeps one of the script's own rendered notes per instrument as
that instrument's sample (looped where the sound holds, so a long note can last; a pluck rings out as it is), then writes the notes as a
sequence the sound chip plays (src/audio.c). The arrangement, the voicing and the dynamics are the script's; the room is the sound chip's
reverb. Light remix: each instrument has its own place in the stereo field.

usage: build_midi_songs.py <bore-gba-main> <out dir with SONGS.BIN + songs.inc from build_music.py> id|NAME|ARTIST|script|fx [...]
       (needs numpy, scipy, xaenc on PATH)"""
import sys, os, re, struct, subprocess, inspect, types, wave
import numpy as np
from scipy import signal

SRC, OUT = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
SPECS = sys.argv[3:]
SR_S = 24000                   # the rate the instrument samples are kept at
MIXR = 18157
ROWN, RFR = 226, 246           # a row: (226 + 246/256) / 18157 s = 12.5 ms
ROW_S = (ROWN + RFR / 256) / MIXR
PAT_ROWS = 256
NCH = 16
SECTOR = 2048
KEY_OFF = 127                  # (an event with this note: key the channel off, audio.c)

class Stop(Exception): pass

def capture(script):
    """run the script; every note it mixes, with its synth, pitch, times and level"""
    notes = []; keep = {}
    src = open(os.path.join(SRC, script)).read()
    src = re.sub(r'(?m)^def put\(', 'def _put_unused(', src)
    def cap(*a):
        if len(a) == 4: buf, t0, x, g = a
        else: buf = None; t0, x, g = a
        fr = inspect.currentframe().f_back
        L = fr.f_locals; code = fr.f_code; SRG[0] = fr.f_globals.get('SR', SRG[0])
        key = code.co_name if code.co_name != '<module>' else 'line%d' % fr.f_lineno
        x = np.asarray(x, dtype=float)
        if not len(x): return
        pk = float(np.abs(x).max()) * abs(g)
        if pk <= 0: return
        hz = lambda m: 440 * 2 ** ((m - 69) / 12)
        fs = []
        if 'ms' in L and isinstance(L['ms'], (list, tuple)): fs = [hz(m) for m in L['ms']]
        elif 'f0' in L and np.size(L['f0']) >= 1 and not isinstance(L['f0'], str): fs = [float(np.median(L['f0']))]
        elif 'f' in L and np.size(L['f']) >= 1 and not isinstance(L['f'], str): fs = [float(np.median(L['f']))]
        elif 'm' in L and np.ndim(L['m']) == 0: fs = [hz(L['m'])]
        else: fs = [0.0]                                     # (a drum: no pitch)
        s = float(L['s']) if 's' in L and np.ndim(L['s']) == 0 else float(t0)
        e = float(L['e']) if 'e' in L and np.ndim(L['e']) == 0 else s + len(x) / SRG[0]
        for f in fs: notes.append(dict(key=key, t0=float(t0), s=s, e=e, f=f, amp=pk / max(1, len(fs) ** .5), dur=len(x) / SRG[0]))
        k = keep.setdefault(key, [])
        if len(k) < 80: k.append((fs[len(fs) // 2], max(0.0, e - s), x * (1 if len(fs) == 1 else 1)))
    G = {'__name__': '__main__', '__file__': os.path.join(SRC, script), 'put': cap}
    SRG = [44100]
    import wave as _w
    real_open = _w.open
    def stop_open(*a, **k): raise Stop()
    _w.open = stop_open
    cwd = os.getcwd(); os.chdir(SRC); sys.path.insert(0, os.path.join(SRC, 'tools'))
    os.environ.setdefault('TMPDIR', '/tmp')
    try:
        code = compile(src, script, 'exec')
        exec(code, G)
    except Stop: pass
    except Exception as ex:   # (the mixing after the notes may fail on the empty buffers: the notes are all in by then)
        if not notes: raise
        print('  (stopped at the mix: %s)' % type(ex).__name__)
    finally:
        _w.open = real_open; os.chdir(cwd)
    return notes, keep, SRG[0]


def capture_chip(script):
    """HERE TODAY (ORIGINAL), the chiptune (tools/make_heretoday.py): its notes worked out the way its render() does, one sample per voice"""
    G = {'__name__': 'chip'}
    cwd = os.getcwd(); os.chdir(SRC)
    try: exec(compile(open(os.path.join(SRC, script)).read(), script, 'exec'), G)
    finally: os.chdir(cwd)
    SR = G['SR']; hz = G['midi_hz']; pulse = G['pulse']; swell = G['swell']; PH = G['PHRASES']
    notes = []; keep = {}
    def note(key, t0, dur, f, amp, held):
        notes.append(dict(key=key, t0=t0, s=t0, e=t0 + dur, f=f, amp=amp, dur=dur))
    # the samples: each voice at one pitch, as render() makes it
    def lead_x(f, nl):
        n = int(nl * SR); t = np.arange(n) / SR
        vib = 1 + .007 * np.sin(2 * np.pi * 5.6 * t) * np.clip((t - .22) * 4, 0, 1)
        ph = np.cumsum(f * vib) / SR; ph2 = np.cumsum(f * vib * 1.004) / SR
        env = np.minimum(t / .006, 1.0) * (np.exp(-t * 1.5) * .65 + .35)
        return (pulse(ph, .25) * .26 + pulse(ph2, .125) * .08) * env
    def pad_x(f, L): n = int(L * SR); t = np.arange(n) / SR; return pulse(np.cumsum(np.full(n, f)) / SR, .5) * np.minimum(t / .12, 1.0) * (.8 + .2 * np.sin(2 * np.pi * .7 * t))
    def tri_x(f, ln):
        m = int(ln * SR); tt = np.arange(m) / SR; ph = np.cumsum(np.full(m, f)) / SR
        return (4 * np.abs((ph % 1.0) - .5) - 1) * np.minimum(tt / .012, 1.0) * np.exp(-tt * .9)
    def arp_x(f): m = int(.28 * SR); tt = np.arange(m) / SR; return pulse(np.cumsum(np.full(m, f)) / SR, .125) * np.minimum(tt / .004, 1.0) * np.exp(-tt * 11.0)
    keep['lead'] = [(hz(74), 2.0, lead_x(hz(74), 2.0))]; keep['pad'] = [(hz(55), 2.0, pad_x(hz(55), 2.0))]
    keep['bass'] = [(hz(43), 2.6, tri_x(hz(43), 2.6))]; keep['arp'] = [(hz(64), .28, arp_x(hz(64)))]
    for ps, pe, ns in PH:
        for t0, nl, m in ns:
            nl = max(nl, .12); note('lead', t0, nl, hz(m + 12), .34, 1)
            for d, g in ((.23, .22), (.46, .10), (.69, .045)): note('lead', t0 + d, nl, hz(m + 12), .34 * g, 1)
    sl = G['slots'](); ch = G['harmonise'](sl)
    a = lambda p, lo: lo + ((p - lo) % 12)
    for (s0, s1, ns), c in zip(sl, ch):
        r, q = c; third = 3 if q else 4; L = s1 - s0; last = sl[-1][0] == s0
        for p_ in (a(r, 50), a(r + third, 50), a(r + 7, 50)): note('pad', s0, L, hz(p_), .034 * 3 * swell(s0), 1)
        bl = min(L * (.55 if L > 2.0 else .95), 2.6) if not last else 4.0
        note('bass', s0, bl, hz(a(r, 43)), .30 * swell(s0), 0)
        if L > 2.0 and not last: note('bass', s0 + L * .55, min(L * .4, 1.6), hz(a(r + 7, 43)), .26 * swell(s0 + L * .55), 0)
        if L > 3.4 and not last: note('bass', s1 - .5, .45, hz(a(r, 43) + 12), .18 * swell(s1), 0)
        pat = [0, 1, 2, 3, 2, 1]
        for k in range(int(L / .25) if not last else 8):
            p_ = [a(r, 60), a(r + third, 60), a(r + 7, 60), a(r, 60) + 12][pat[k % 6]]
            note('arp', s0 + k * .25, .28, hz(p_), .065 * swell(s0 + k * .25) * (1.0 if not last else .7), 0)
    return notes, keep, SR

def adpcm(pcm16):
    tmp = os.path.join(OUT, '_m.adpcm')
    r = subprocess.run(['xaenc', 'spu', '0', tmp], input=pcm16.astype('<i2').tobytes(), capture_output=True)
    if r.returncode: sys.exit('xaenc failed: ' + r.stderr.decode())
    d = bytearray(open(tmp, 'rb').read()); os.remove(tmp)
    d += bytes((-len(d)) % 16)
    for b in range(0, len(d), 16): d[b + 1] = 0
    return d

def make_sample(x, sr, f):
    """one of the script's notes -> (ADPCM bytes, rate it plays at for its own pitch, its pitch, looped?)"""
    x = signal.resample_poly(x, SR_S // 1000, sr // 1000) if sr != SR_S else x.copy()
    x = x / max(1e-9, np.abs(x).max())
    n = len(x); w = int(SR_S * .02)
    rms = np.sqrt(np.convolve(x ** 2, np.ones(w) / w, 'same'))
    held = n > SR_S * .5 and rms[int(n * .6)] >= .45 * rms.max()
    if not held:                                       # a pluck, a bell, a drum: as it rings, to where it has died away
        end = n
        quiet = np.where(rms > rms.max() * .01)[0]
        if len(quiet): end = min(n, quiet[-1] + w)
        end = min(end, int(SR_S * 2.6))
        y = x[:end] * np.r_[np.ones(max(0, end - 256)), np.linspace(1, 0, min(256, end))]
        d = adpcm(np.round(y * 30000))
        d[len(d) - 16 + 1] = 0x01                      # the end (no repeat: the voice goes quiet)
        return d, SR_S, f, False
    # held: a loop of about 0.3 s from the steady middle, cut where the waves line up best
    p0 = int(n * .45); want = int(SR_S * .3)
    win = 256; a = x[p0:p0 + win]
    best, bl = -2, want
    for L in range(want - int(SR_S * .02), want + int(SR_S * .02)):
        b = x[p0 + L:p0 + L + win]
        if len(b) < win: break
        c = np.dot(a, b) / (np.linalg.norm(a) * np.linalg.norm(b) + 1e-9)
        if c > best: best, bl = c, L
    # stretch it a hair so the loop is a whole number of 28-sample blocks, and start the loop on a block
    tgt = int(round(bl / 28)) * 28
    F = tgt / bl
    y = signal.resample(x[:p0 + bl], int(round((p0 + bl) * F)))
    ls = int(round(p0 * F)); pad = (-ls) % 28
    y = np.r_[np.zeros(pad), y]; ls += pad
    y = y[:ls + tgt]
    xf = 64                                            # (a short crossfade at the seam)
    y[ls + tgt - xf:ls + tgt] = y[ls + tgt - xf:ls + tgt] * np.linspace(1, 0, xf) + y[ls - xf:ls] * np.linspace(0, 1, xf)
    d = adpcm(np.round(y * 30000))
    nb = (ls + tgt) // 28
    d = d[:nb * 16]
    d[(ls // 28) * 16 + 1] |= 0x04                    # loop start
    d[len(d) - 16 + 1] = 0x03                          # loop end + repeat
    return d, SR_S * F, f, True

PANS = {'arp': (96, 120), 'voice': (110, 110), 'harmony_voice': (96, 118), 'lead': (110, 104), 'pluck_gtr': (92, 124), 'harp': (124, 92), 'bell': (88, 124),
        'strings': (122, 98), 'pad': (112, 112), 'bass': (114, 114), 'thump': (112, 112)}
REL = {'voice': 1, 'harmony_voice': 2, 'lead': 1, 'strings': 2, 'pad': 3}   # the release on key off (audio.c relTab): 1 short, 2 longer, 3 long

LEVEL = 0.6   # (the loud end of the mix at about the level of the tracker songs)
def build(sid, name, artist, script, fx, GAIN=1.0):
    notes, keep, sr = capture_chip(script) if script.endswith('make_heretoday.py') else capture(script)
    keys = sorted(set(n['key'] for n in notes), key=lambda k: -sum(1 for n in notes if n['key'] == k))
    inst = {}; smp = bytearray(); smpoff = []; pitches = []; buses = []
    for ki, k in enumerate(keys[:31]):
        cand = keep.get(k, [])
        fs = np.array([n['f'] for n in notes if n['key'] == k and n['f'] > 0])
        med = float(np.median(fs)) if len(fs) else 0
        # the representative: near the middle pitch, long enough to show how it holds
        def score(c): f_, d_, x_ = c; return (abs(np.log2(max(f_, 1) / max(med, 1))) if med else 0) - min(d_, 2.0) * .3
        f_, d_, x_ = min(cand, key=score)
        d, rate, fref, held = make_sample(x_, sr, f_)
        inst[k] = dict(i=len(smpoff), held=held, f=fref, rate=rate)
        smpoff.append(len(smp)); smp += d
        pch = []
        for nt in range(96):
            fn = 440 * 2 ** ((nt + 12 - 69) / 12)
            pch.append(max(1, min(0x3FFF, int(round(4096 * rate / 44100 * (fn / fref if fref > 0 else 1))))))
        pitches += pch
        base = re.sub(r'\d+$', '', k)
        buses.append(PANS.get(base, (110, 110)))
        print('  %-16s %-5s %5d notes  sample %6d B  %s' % (k, 'loop' if held else 'ring', sum(1 for n in notes if n['key'] == k), len(d), 'f %.0f Hz' % fref), flush=True)
    # ---- the events: a note on at its start, a key off at its end (held sounds), on 16 channels ----
    notes = [n for n in notes if n['key'] in inst]
    # how loud the mix gets: the notes' power added up on a 50 ms grid (a pluck counts for its first half second), the loud end of it
    grid = np.zeros(int(max(n['e'] for n in notes) / .05) + 64)
    for n in notes:
        held = inst[n['key']]['held']; e_ = max(n['e'], n['t0'] + .1) if held else n['t0'] + min(.5, n['dur'])
        grid[int(n['t0'] / .05):int(e_ / .05) + 1] += n['amp'] ** 2
    M = float(np.sqrt(np.percentile(grid[grid > 0], 98))) or 1.0
    evs = []; chan_end = [-1.0] * NCH; chan_off = [None] * NCH
    for n in sorted(notes, key=lambda n: n['t0']):
        I = inst[n['key']]
        on = n['t0']; off = max(n['e'], on + .05) if I['held'] else on + min(n['dur'], 2.6)
        free = [c for c in range(NCH) if chan_end[c] <= on]
        c = free[0] if free else min(range(NCH), key=lambda c: chan_end[c])
        if chan_off[c] is not None and chan_off[c][0] > on: evs.remove(chan_off[c])   # (taken over: its key off would cut the new note)
        chan_off[c] = None
        nt = int(round(69 + 12 * np.log2(n['f'] / 440))) - 12 if n['f'] > 0 else 48
        nt = max(0, min(95, nt))
        vol = int(round(min(127, 127 * LEVEL * GAIN * n["amp"] / M)))
        bus = min(6, I['i'] % 7)
        evs.append((on, c, I['i'], bus, nt, max(1, vol), REL.get(re.sub(r'\d+$', '', n['key']), 0)))
        if I['held']: ev = (off, c, I['i'], bus, KEY_OFF, 0, 0); evs.append(ev); chan_off[c] = ev
        chan_end[c] = off + (.4 if I['held'] else 0)
    # the buses: one per instrument (7 at most: instruments share them in turn), each with its own pan
    busL = [0] * 7; busR = [0] * 7
    for i, (l, r) in enumerate(buses[:7]): busL[i], busR[i] = l, r
    for i in range(len(buses), 7): busL[i] = busR[i] = 110
    # ---- the sequence ----
    vt = []; vti = {}
    def vt_of(ch, ins, bus, rel):
        t = ch | (ins << 4) | (bus << 9) | (rel << 12)
        if t not in vti: vti[t] = len(vt); vt.append(t)
        return vti[t]
    total_rows = int(max(e[0] for e in evs) / ROW_S) + 2
    npat = (total_rows + PAT_ROWS - 1) // PAT_ROWS
    if npat > 255: sys.exit('%s: too long for 255 patterns' % sid)
    byrow = {}
    for e in evs: byrow.setdefault(int(round(e[0] / ROW_S)), []).append(e)
    ev_bytes = bytearray(); patoff = []; rows = []
    for p in range(npat):
        patoff.append(len(ev_bytes)); r0 = p * PAT_ROWS; nr = min(PAT_ROWS, total_rows - r0); rows.append(nr)
        empty = 0
        for r in range(nr):
            es = byrow.get(r0 + r, [])
            # (a key off before a note on, in the same row: the note on wins)
            es = sorted(es, key=lambda e: e[4] != KEY_OFF)
            if not es:
                empty += 1
                if empty == 127: ev_bytes.append(0x80 | 127); empty = 0
                continue
            if empty: ev_bytes.append(0x80 | empty); empty = 0
            while es:
                chunk, es = es[:127], es[127:]
                ev_bytes.append(len(chunk))
                for (t, ch, ins, bus, nt, vol, rel) in chunk:
                    v = vt_of(ch, ins, bus, rel)
                    w = (v & 255) | (nt << 8) | (vol << 15) | ((v >> 8) << 22)
                    ev_bytes += bytes((w & 255, (w >> 8) & 255, (w >> 16) & 255))
                if es: sys.exit('more than 127 events in a row')
        if empty: ev_bytes.append(0x80 | empty)
    if len(vt) > 1024: sys.exit('%s: too many channel/instrument pairs' % sid)
    order = list(range(npat))
    hdr_n = 20; body = bytearray(); offs = {}
    def put(key, data, align=4):
        nonlocal body
        body += bytes((-(hdr_n * 4 + len(body))) % align); offs[key] = hdr_n * 4 + len(body); body += data
    put('order', bytes(order)); put('rows', struct.pack('<%dH' % len(rows), *rows)); put('patoff', struct.pack('<%dI' % len(patoff), *patoff))
    put('vt', struct.pack('<%dH' % len(vt), *vt)); put('pitch', struct.pack('<%dH' % len(pitches), *pitches))
    put('smpoff', struct.pack('<%dI' % len(smpoff), *smpoff)); put('bus', bytes(busL + busR)); put('ev', bytes(ev_bytes))
    seconds = int(round(total_rows * ROW_S))
    hdr = struct.pack('<20I', 0x31515342, npat, 0, ROWN, RFR, len(rows), len(smpoff), len(vt), len(ev_bytes), len(smp), seconds,
                      offs['order'], offs['rows'], offs['patoff'], offs['vt'], offs['pitch'], offs['smpoff'], offs['bus'], offs['ev'], 0)
    seq = hdr + body; seq += bytes((-len(seq)) % SECTOR)
    smpb = bytes(smp) + bytes((-len(smp)) % SECTOR)
    print('%-28s %4d s  %5d notes  seq %6d B  samples %6d B' % (sid, seconds, len(notes), len(seq), len(smp)), flush=True)
    return dict(seq=seq, smp=smpb, smp_bytes=len(smp), seconds=seconds, name=name, artist=artist, fx=int(fx))

songs = []
for spec in SPECS:
    sid, name, artist, script, fx, *g = spec.split('|')
    print(sid, flush=True)
    songs.append(build(sid, name, artist, script, fx, float(g[0]) if g else 1.0))

# ---- append to SONGS.BIN and songs.inc ----
binp = os.path.join(OUT, 'SONGS.BIN'); old = open(binp, 'rb').read()
magic, n_old = struct.unpack_from('<II', old, 0)
entries = [struct.unpack_from('<IIII', old, 8 + i * 16) for i in range(n_old)]
blob = bytearray(old[SECTOR:]); sec = 1 + len(blob) // SECTOR
for s in songs:
    entries.append((sec, len(s['seq']) // SECTOR, len(s['smp']) // SECTOR, s['smp_bytes']))
    blob += s['seq'] + s['smp']; sec += (len(s['seq']) + len(s['smp'])) // SECTOR
index = struct.pack('<II', magic, len(entries)) + b''.join(struct.pack('<IIII', *e) for e in entries)
open(binp, 'wb').write(index + bytes(SECTOR - len(index)) + blob)
incp = os.path.join(OUT, 'songs.inc'); inc = open(incp).read()
inc = re.sub(r'typedef struct \{[^}]*\} SongRow;[^\n]*', 'typedef struct { u16 seconds; const char *name, *artist; u8 fx; u8 stream; u16 rate; } SongRow;   // fx: the reverb preset (audio.c); stream: 1 played off the disc at rate Hz (audio.c, the streamer: none at the moment)', inc)
inc = re.sub(r'#define NSONGS \d+', '#define NSONGS %d' % len(entries), inc)
big = max([s['smp_bytes'] for s in songs] + [int(re.search(r'#define SONG_SMP_MAX (\d+)', inc).group(1))])
inc = re.sub(r'#define SONG_SMP_MAX \d+', '#define SONG_SMP_MAX %d' % big, inc)
rows = ''.join('    { %d, "%s", "%s", %d },\n' % (s['seconds'], s['name'], s['artist'], s['fx']) for s in songs)
i = inc.rindex('};'); inc = inc[:i] + rows + inc[i:]
open(incp, 'w').write(inc)
print('SONGS.BIN %d KB, %d songs' % ((SECTOR + len(blob)) // 1024, len(entries)))
