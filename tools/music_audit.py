#!/usr/bin/env python3
"""Music timing audit (host side): every song of assets/SONGS.BIN, the sequencer clock of each port as it was and as it is, and what that does to the notes.

  python3 tools/music_audit.py                 -> table on stdout (before / after) and the too-fast / loose lists
  python3 tools/music_audit.py --plots         -> docs/music_timing_ds.png (onset error over time, before/after), docs/music_timing_tempo.png (all songs)
  python3 tools/music_audit.py --wav DIR 17 3  -> renders songs 17 and 3 (index in assets/songs.inc) to DIR/song17_ps1.wav (the schedule as the PS1 / DS d72 play
                                                  it) and DIR/song17_ds_old.wav (the DS before D72, at a 30 fps main loop): a simple host mixer (ADPCM decode, pitch,
                                                  loop, a short release), meant for hearing the timing, not the exact SPU sound

What is measured: each song's rows (a row = the sequencer's step, rowN + rfr/256 samples at 18157 Hz, the PS1 engine's MIXR). A row with a note on it is an onset.
The intended time of row r is r * row length (the song's own tempo: the XM headers' speed * 2.5 / BPM, or the 12.5 ms grid of the HERE TODAY songs); each port's model
turns that into the time the row really sounds:
  ps1      the 480 Hz timer: a row fires on the first tick at or after its time (src/audio.c seq_tick). Measured in DuckStation: 8.02 ticks per vblank, rows 0..1.4 ms.
  dc_old / dc_new   the Dreamcast's software SPU: ticks on 44.1 kHz frames; the voices are mixed in half-rate samples, so a stretch of an odd number of frames
                    advanced one half-rate sample too many: dc_old plays every voice 1/735 = +0.136 % sharp, dc_new 0 (ports/dreamcast/dc_spu.c)
  ds_old   the DS before D72: the ARM9 counted rows off once per main-loop pass in video frames (1/59.8261 s), every gap one pass short; P = passes a second
  ds_new   the DS from D72: the ARM7 plays the note list on a 1022.7 Hz timer (a cascaded counter: no tick lost), each note on its own tick
Per song and model: tempo = how fast it plays against the written tempo (+ = too fast), drift = the end of the song's time against the written time,
max = the worst note-start error (ms), loose = the worst error after the best straight line (offset + tempo) is taken off: the unevenness you hear as loosely timed.
"""
import sys, os, re, math, struct
sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
MIXR = 18157
TICK_HZ = 480.0                  # PS1 timer 2 (33868800 / 8 / 8820 = 480 exactly)
DS_TICK = 33513982 / 64 / 512    # DS ARM7 timer 0
DS_FPS = 59.8261                 # the DS video frame rate the old note list was cut in
TOO_FAST, LOOSE_MS = 0.02, 12.0  # reporting thresholds: tempo off by more than 2 %, notes off the straight line by more than 12 ms

def unlz(comp, n):
    out = bytearray(); i = 0
    while len(out) < n:
        f = comp[i]; i += 1
        for b in range(8):
            if len(out) >= n: break
            if f >> b & 1: out.append(comp[i]); i += 1
            else:
                a, c = comp[i], comp[i + 1]; i += 2
                off = (a | (c & 15) << 8) + 1; L = (c >> 4) + 3
                for _ in range(L): out.append(out[-off])
    return bytes(out)

def load(fn):
    d = open(fn, 'rb').read(); mg, n = struct.unpack_from('<2I', d, 0); songs = []
    for i in range(n):
        sec, seqs, smps, smpb = struct.unpack_from('<4I', d, 8 + i * 16)
        S = dict(i=i, seqs=seqs)
        if seqs:
            b = d[sec * 2048:(sec + seqs) * 2048]
            (magic, nord, loop, rowN, rfr, npat, ninst, nvt, evs, smpsz, secs, o_order, o_rows, o_patoff, o_vt, o_pitch, o_smpoff, o_bus, o_ev, pad) = struct.unpack_from('<20I', b, 0)
            S.update(rowN=rowN, rfr=rfr, ninst=ninst)
            S['vt'] = list(struct.unpack_from('<%dH' % nvt, b, o_vt))
            S['pitch'] = list(struct.unpack_from('<%dH' % (ninst * 96), b, o_pitch))
            S['smpoff'] = list(struct.unpack_from('<%dI' % ninst, b, o_smpoff))
            x = struct.unpack_from('<4I', b, pad)
            S['intro'] = unlz(b[o_ev:x[1]], x[0]); S['body'] = unlz(b[x[1]:], x[2])
            S['smp'] = d[(sec + seqs) * 2048:(sec + seqs) * 2048 + smpb]
        songs.append(S)
    return songs

def rows_iter(stream):
    i = 0; n = len(stream)
    while i < n:
        h = stream[i]; i += 1
        if h & 0x80:
            for _ in range(h & 0x7f): yield []
        else:
            ev = []
            for _ in range(h):
                w = stream[i] | stream[i + 1] << 8 | stream[i + 2] << 16; i += 3
                ev.append((w & 255 | (w >> 22) << 8, (w >> 8) & 127, (w >> 15) & 127))
            yield ev

def names():
    t = open(os.path.join(ROOT, 'assets', 'songs.inc')).read()
    return [(int(a), b) for a, b in re.findall(r'\{ (\d+), "([^"]*)"', t)]

# ---- the clock models: intended onset times t (s) of rows r -> the times they sound
def m_ps1(r, rs): return [math.ceil(k * rs * TICK_HZ - 1e-9) / TICK_HZ for k in r]
def m_dc(r, rs):
    out = []
    for k in r:
        tk = math.ceil(k * rs * TICK_HZ - 1e-9)                    # the 480 Hz tick the row fires on
        out.append(math.ceil(tk * 44100 / TICK_HZ - 1e-9) / 44100)   # ... which lands on a 44.1 kHz frame (dc_old; dc_new: an even one, 2 frames = 45 us at most)
    return out
def m_dc_new(r, rs): return [2 * math.ceil(math.ceil(k * rs * TICK_HZ - 1e-9) * 22050 / TICK_HZ - 1e-9) / 44100 for k in r]
def m_ds_new(r, rs): return [round(k * rs * DS_TICK) / DS_TICK for k in r]
def m_ds_old(P):
    def f(r, rs):   # the old BDM1 list: delta = frames between two rows (video frames), the player waited delta-1 passes between them
        out = []; n = 0; prev = None
        for k in r:
            fr = int(round(k * rs * DS_FPS))
            if prev is not None: n += max(fr - prev - 1, 0) if fr > prev else 0
            prev = fr; out.append(n / P)
        return out
    return f
MODELS = [('ps1', m_ps1), ('dc_old', m_dc), ('dc_new', m_dc_new), ('ds_old60', m_ds_old(59.826)), ('ds_old30', m_ds_old(29.913)), ('ds_new', m_ds_new)]

def fitline(t, T):
    n = len(t); mt = sum(t) / n; mT = sum(T) / n
    vt = sum((x - mt) ** 2 for x in t) or 1e-12
    a = sum((x - mt) * (y - mT) for x, y in zip(t, T)) / vt; b = mT - a * mt
    return a, b

def stats(t, T):
    a, b = fitline(t, T)
    err = [(y - x) * 1000 for x, y in zip(t, T)]
    res = [(y - (a * x + b)) * 1000 for x, y in zip(t, T)]
    # the tempo as heard: how much faster the song runs against its written length (the last onset)
    speed = (t[-1] / T[-1]) if T[-1] > 0 else 1.0
    return dict(tempo=speed - 1, drift=err[-1], mx=max(abs(e) for e in err), loose=max(abs(e) for e in res), err=err)

def song_rows(S):
    rows = list(rows_iter(S['intro'])) + list(rows_iter(S['body']))
    rs = (S['rowN'] + S['rfr'] / 256) / MIXR
    on = [k for k, row in enumerate(rows) if any(nt < 96 for _, nt, _ in row)]
    return rows, rs, on

def audit():
    S = load(os.path.join(ROOT, 'assets', 'SONGS.BIN')); nm = names(); res = []
    for s, (sec, name) in zip(S, nm):
        if not s['seqs']: continue
        rows, rs, on = song_rows(s)
        if len(on) < 8: continue
        t = [k * rs for k in on]
        ev = [k for k, rw in enumerate(rows) if rw]   # every row with an event (a note on or a note off): the old DS player lost a frame on each gap between two of them
        pos = {k: j for j, k in enumerate(ev)}
        row = dict(i=s['i'], name=name, listed=sec, rows=len(rows), rs=rs, dur=len(rows) * rs, onsets=len(on), t=t)
        row['bpm'] = 60 / (4 * rs)    # beats a minute at 4 rows a beat (the XM way to read it)
        for mn, f in MODELS:
            T = f(ev, rs); row[mn] = stats(t, [T[pos[k]] for k in on])
        res.append(row)
    return res

def table(res):
    print('%3s %-36s %6s %5s %6s %6s | %s' % ('#', 'song', 'rows', 'row ms', 'len s', 'listed', ' | '.join('%-8s tempo  drift  max loose' % m for m, _ in MODELS)))
    for r in res:
        print('%3d %-36s %6d %6.2f %6.1f %6d | %s' % (r['i'], r['name'][:36], r['rows'], r['rs'] * 1000, r['dur'], r['listed'],
              ' | '.join('%-8s %+5.1f%% %+6.0f %5.0f %5.1f' % ('', r[m]['tempo'] * 100, r[m]['drift'], r[m]['mx'], r[m]['loose']) for m, _ in MODELS)))
    print()
    for label, ms in (('BEFORE', ('ps1', 'dc_old', 'ds_old60', 'ds_old30')), ('AFTER', ('ps1', 'dc_new', 'ds_new'))):
        for m in ms:
            fast = [r['i'] for r in res if r[m]['tempo'] > TOO_FAST]
            slow = [r['i'] for r in res if r[m]['tempo'] < -TOO_FAST]
            loose = [r['i'] for r in res if r[m]['loose'] > LOOSE_MS]
            print('%-6s %-9s too fast (> +%d%%): %2d %s\n%16s too slow: %2d, loose (> %.0f ms off the line): %2d %s' % (label, m, TOO_FAST * 100, len(fast), fast, '', len(slow), LOOSE_MS, len(loose), loose))
    print('\nlisted vs computed length (a mis-converted tempo would show here):', 'all within 3 s' if all(abs(r['dur'] - r['listed']) < 3 + 0.02 * r['listed'] for r in res) else
          [(r['i'], r['name'], r['listed'], round(r['dur'])) for r in res if abs(r['dur'] - r['listed']) >= 3 + 0.02 * r['listed']])

def md(res):
    print('| # | song | row ms | PS1 tempo / max ms | DC voice pitch before | DC after | DS before 60 passes/s: tempo, loose ms | DS before 30 passes/s: tempo | DS after: tempo / max ms |\n|---|---|---|---|---|---|---|---|---|')
    for r in res:
        print('| %d | %s | %.1f | %+.2f%% / %.1f | %+.3f%% | %+.3f%% | %+.0f%%, %.0f | %+.0f%% | %+.2f%% / %.1f |' % (r['i'], r['name'], r['rs'] * 1000, r['ps1']['tempo'] * 100, r['ps1']['mx'],
              r['dc_old']['tempo'] * 100 + 0.136, r['dc_new']['tempo'] * 100, r['ds_old60']['tempo'] * 100, r['ds_old60']['loose'], r['ds_old30']['tempo'] * 100, r['ds_new']['tempo'] * 100, r['ds_new']['mx']))

def plots(res):
    import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
    by = {r['i']: r for r in res}
    # the fastest-rowed song and two others
    fastest = min(res, key=lambda r: r['rs'])['i']
    pick = [i for i in (17, 9, fastest) if i in by]
    fig, ax = plt.subplots(2, len(pick), figsize=(5 * len(pick), 7), squeeze=False)
    for c, i in enumerate(pick):
        r = by[i]; t = [x for x in r['t'] if x <= 30]; q = len(t)   # the first 30 s
        for m, col, lab in (('ps1', 'tab:blue', 'PS1 (unchanged)'), ('ds_old60', 'tab:red', 'DS before, 60 fps loop'), ('ds_old30', 'tab:orange', 'DS before, 30 fps loop'), ('dc_old', 'tab:gray', 'DC before')):
            ax[0][c].plot(t, r[m]['err'][:q], color=col, lw=1, label=lab)
        for m, col, lab in (('ps1', 'tab:blue', 'PS1'), ('dc_new', 'tab:green', 'DC after'), ('ds_new', 'tab:purple', 'DS after (d72)')):
            ax[1][c].plot(t, r[m]['err'][:q], color=col, lw=1, label=lab)
        for a, tt in ((ax[0][c], 'before'), (ax[1][c], 'after')):
            a.set_title('%s (%d): %s' % (r['name'][:26], i, tt), fontsize=9); a.set_xlabel('written time (s)'); a.set_ylabel('onset - written (ms), first 30 s'); a.grid(alpha=.3); a.legend(fontsize=7)
        ax[1][c].set_ylim(-6, 6)
    fig.tight_layout(); p = os.path.join(ROOT, 'docs', 'music_timing_ds.png'); fig.savefig(p, dpi=90); print('wrote', p)
    fig, ax = plt.subplots(2, 1, figsize=(13, 7), sharex=True)
    x = [r['i'] for r in res]
    for a, ms, t in ((ax[0], (('ds_old60', 'tab:red'), ('ds_old30', 'tab:orange'), ('dc_old', 'tab:gray')), 'before'), (ax[1], (('ps1', 'tab:blue'), ('dc_new', 'tab:green'), ('ds_new', 'tab:purple')), 'after')):
        w = 0.8 / len(ms)
        for k, (m, col) in enumerate(ms): a.bar([v + k * w for v in x], [r[m]['tempo'] * 100 for r in res], w, color=col, label=m)
        a.set_ylabel('tempo vs written (%)'); a.set_title('tempo, ' + t); a.legend(fontsize=7); a.grid(alpha=.3)
    ax[1].set_ylim(-5, 5); ax[1].set_xlabel('song'); fig.tight_layout()
    p = os.path.join(ROOT, 'docs', 'music_timing_tempo.png'); fig.savefig(p, dpi=90); print('wrote', p)

# ---- a plain offline renderer (to hear the timing): PS1 ADPCM decode, pitch, loop, release
_F = [(0, 0), (60, 0), (115, -52), (98, -55), (122, -60)]
def vag_decode(smp, off):
    """-> (samples, loop start or None): a PS1 ADPCM stream from byte `off` to the block with the end flag (block byte 1: bit 0 end, 1 repeat, 2 loop start)"""
    out = []; s1 = s2 = 0.0; loop_at = None; rep = False; i = off
    while i + 16 <= len(smp):
        sh = smp[i] & 15; fi = min(smp[i] >> 4, 4); f = smp[i + 1]; c0, c1 = _F[fi]
        if f & 4 and loop_at is None: loop_at = len(out)
        for b in smp[i + 2:i + 16]:
            for nib in (b & 15, b >> 4):
                v = nib - 16 if nib > 7 else nib
                x = (v << 12 >> sh) + (s1 * c0 + s2 * c1) / 64.0; s2 = s1; s1 = x; out.append(x)
        i += 16
        if f & 1: rep = bool(f & 2); break
    return out, ((loop_at or 0) if rep else None)

def render(S, T_of_row, rows, secs, path, sr=22050):
    import numpy as np
    n = int(secs * sr) + sr; mix = np.zeros(n, dtype=np.float32); cache = {}; live = {}
    def smp_of(inn):
        if inn not in cache:
            o = S['smpoff'][inn]; cache[inn] = vag_decode(S['smp'], o) if o != 0xffffffff else ([0.0], None)
        return cache[inn]
    for r, row in enumerate(rows):
        t0 = T_of_row(r)
        if t0 is None or t0 > secs: break
        for vi, nt, vol in row:
            if nt == 127 or nt >= 96:
                if vi in live: live[vi][2] = t0   # release start
                continue
            inn = (S['vt'][vi] >> 4) & 31; p = S['pitch'][inn * 96 + nt]
            if not p: continue
            if vi in live and live[vi][2] is None: live[vi][2] = t0
            d, lp = smp_of(inn)
            live[vi] = [None, None, None]
            st = int(t0 * sr); step = p / 4096.0 * 44100 / sr; ln = int(min(len(d) / step if lp is None else 4 * sr, n - st))
            idx = np.arange(max(ln, 0)) * step
            if lp is not None and len(d) > lp: idx = np.where(idx >= len(d), lp + (idx - lp) % max(len(d) - lp, 1), idx)
            idx = idx.astype(int); idx = np.minimum(idx, len(d) - 1)
            w = np.asarray(d, dtype=np.float32)[idx] * (vol / 127.0) / 32768.0
            env = np.ones(len(w), dtype=np.float32)
            env[-int(0.02 * sr):] *= np.linspace(1, 0, min(int(0.02 * sr), len(w))) if len(w) > 0 else 1
            mix[st:st + len(w)] += w * env * 0.25
    mix = np.clip(mix[:int(secs * sr)], -1, 1)
    import wave
    with wave.open(path, 'wb') as f:
        f.setnchannels(1); f.setsampwidth(2); f.setframerate(sr); f.writeframes((mix * 32767).astype('<i2').tobytes())

def wavs(res, outdir, idx):
    os.makedirs(outdir, exist_ok=True); S = load(os.path.join(ROOT, 'assets', 'SONGS.BIN'))
    for i in idx:
        s = S[i]; rows, rs, on = song_rows(s)
        for tag, f in (('ps1', m_ps1), ('ds_old', m_ds_old(29.913)), ('ds_new', m_ds_new)):
            T = dict(zip(range(len(rows)), f(list(range(len(rows))), rs)))
            p = os.path.join(outdir, 'song%d_%s.wav' % (i, tag)); render(s, lambda r: T.get(r), rows, 20, p); print('wrote', p)

if __name__ == '__main__':
    a = sys.argv[1:]; res = audit()
    if '--wav' in a:
        k = a.index('--wav'); wavs(res, a[k + 1], [int(x) for x in a[k + 2:]])
    elif '--plots' in a: plots(res)
    elif '--md' in a: md(res)
    else: table(res)
