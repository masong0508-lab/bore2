#!/usr/bin/env python3
"""BORE 2 streamed songs: recordings (no note data to sequence) played straight off the disc by src/audio.c's SPU streamer.
Adds them to the end of SONGS.BIN and songs.inc (made first by tools/build_music.py).

Each song is rendered fresh by the GBA game's own generator scripts (tools/make_heretoday*.py in bore-gba-main), at full rate, so it is a
remaster rather than an up-sample of the GBA's 18 kHz mono ADPCM: 32 kHz stereo here. A mono render gets a light stereo remix (a short
room on each side, a little apart, so the song opens out); a stereo render is kept as it is.

On the disc a streamed song is CHUNKS: 4 KB of the left channel then 4 KB of the right (4 sectors), in the sound chip's own ADPCM. The
last block of every 4 KB carries the end + repeat flags, so a voice reaching the end of a chunk jumps to wherever the streamer points
it (the next chunk's buffer).

usage: build_streams.py <out dir (holding SONGS.BIN, songs.inc)> name|artist|wav [...]       (needs numpy, scipy, xaenc on PATH)"""
import sys, os, re, struct, subprocess, wave
import numpy as np
from scipy import signal

OUT = sys.argv[1]
RATE = 32000
IL = 4096                 # bytes per channel per chunk (256 ADPCM blocks of 28 samples)
SECTOR = 2048

def load(path):
    w = wave.open(path, 'rb'); ch, sr, n = w.getnchannels(), w.getframerate(), w.getnframes()
    x = np.frombuffer(w.readframes(n), dtype='<i2').astype(np.float64) / 32768.0
    x = x.reshape(-1, ch)
    if sr != RATE:
        g = np.gcd(sr, RATE); x = signal.resample_poly(x, RATE // g, sr // g, axis=0)
    return x, ch

def widen(m):   # the light remix of a mono render: a short, different room on each side, under the dry sound
    rng = np.random.default_rng(7)
    def room(seed_shift, length_s=0.35):
        n = int(RATE * length_s); t = np.arange(n) / RATE
        ir = rng.standard_normal(n) * np.exp(-t / 0.09); ir[:int(RATE * 0.004) + seed_shift] = 0
        return ir / np.sqrt((ir ** 2).sum())
    l = signal.fftconvolve(m, room(0))[:len(m)]; r = signal.fftconvolve(m, room(97))[:len(m)]
    return np.stack([m + 0.22 * l, m + 0.22 * r], axis=1)

def adpcm(ch):   # one channel to SPU ADPCM blocks, every flag cleared
    pcm = np.clip(np.round(ch * 32767), -32768, 32767).astype('<i2').tobytes()
    tmp = os.path.join(OUT, '_st.adpcm')
    r = subprocess.run(['xaenc', 'spu', '0', tmp], input=pcm, capture_output=True)
    if r.returncode: sys.exit('xaenc failed: ' + r.stderr.decode())
    d = bytearray(open(tmp, 'rb').read()); os.remove(tmp)
    d += bytes((-len(d)) % 16)
    for b in range(0, len(d), 16): d[b + 1] = 0
    return d

songs = []
for spec in sys.argv[2:]:
    name, artist, path = spec.split('|')
    x, ch = load(path)
    st = widen(x[:, 0]) if ch == 1 else x[:, :2]
    st = st / max(1e-9, np.abs(st).max()) * 0.89          # the same headroom the sequenced songs leave
    L, R = adpcm(st[:, 0]), adpcm(st[:, 1])
    n = max(len(L), len(R)); nchunk = (n + IL - 1) // IL
    L += bytes(nchunk * IL - len(L)); R += bytes(nchunk * IL - len(R))
    data = bytearray()
    for k in range(nchunk):
        for c in (L, R):
            part = bytearray(c[k * IL:(k + 1) * IL]); part[IL - 16 + 1] = 0x03   # end + repeat: on to the next buffer
            data += part
    seconds = int(round(len(st) / RATE))
    songs.append(dict(name=name, artist=artist, data=bytes(data), chunks=nchunk, seconds=seconds))
    print('%-34s %4d s  %5d chunks  %6d KB' % (name, seconds, nchunk, len(data) // 1024), flush=True)

# ---- append to SONGS.BIN: a new index (the old entries, then the streams), the old songs' sectors, then the streams' ----
binp = os.path.join(OUT, 'SONGS.BIN')
old = open(binp, 'rb').read()
magic, n_old = struct.unpack_from('<II', old, 0)
assert magic == 0x474E5342
entries = [struct.unpack_from('<IIII', old, 8 + i * 16) for i in range(n_old)]
blob = bytearray(old[SECTOR:]); sec = 1 + len(blob) // SECTOR
for s in songs:
    entries.append((sec, 0, len(s['data']) // SECTOR, s['chunks']))   # (seq sectors 0: a stream; the last field: its chunks)
    blob += s['data']; sec += len(s['data']) // SECTOR
index = struct.pack('<II', magic, len(entries)) + b''.join(struct.pack('<IIII', *e) for e in entries)
assert len(index) <= SECTOR
open(binp, 'wb').write(index + bytes(SECTOR - len(index)) + blob)
print('SONGS.BIN %d KB, %d songs (%d streamed)' % ((SECTOR + len(blob)) // 1024, len(entries), len(songs)))

# ---- songs.inc: the stream rows after the sequenced ones ----
incp = os.path.join(OUT, 'songs.inc')
inc = open(incp).read()
inc = re.sub(r'typedef struct \{[^}]*\} SongRow;[^\n]*', 'typedef struct { u16 seconds; const char *name, *artist; u8 fx; u8 stream; u16 rate; } SongRow;   // fx: the reverb preset (audio.c); stream: 1 played off the disc at rate Hz (audio.c, the streamer)', inc)
inc = re.sub(r'#define NSONGS \d+', '#define NSONGS %d' % len(entries), inc)
rows = ''.join('    { %d, "%s", "%s", 0, 1, %d },\n' % (s['seconds'], s['name'], s['artist'], RATE) for s in songs)
inc = inc.replace('};\n', rows + '};\n', 1) if inc.rstrip().endswith('};') else inc
open(incp, 'w').write(inc)
