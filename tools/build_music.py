#!/usr/bin/env python3
"""BORE 2 music: every jukebox song as a SEQUENCE (the note events) plus its instrument samples in the PS1 sound chip's own
ADPCM format, all in one disc file, SONGS.BIN. The PS1 sound chip then plays the notes in hardware (src/seqplay.c), so the music
costs almost no CPU and the whole jukebox is a few MB instead of 350 MB of streamed audio.

It runs the GBA game's own converter (tools/xm2gba.py in bore-gba-main), so each song plays exactly the notes, volumes, pans,
gains and fixes the GBA version plays. Nothing in bore-gba-main is written.

usage: build_music.py <bore-gba-main> <out dir>      (needs numpy, scipy, and xaenc on PATH)
writes <out dir>/SONGS.BIN and <out dir>/songs.inc"""
import sys, os, re, io, struct, subprocess, contextlib, importlib.util
import numpy as np
SRC, OUT = (os.path.abspath(a) for a in sys.argv[1:3])
os.makedirs(OUT, exist_ok=True)
XAENC = 'xaenc'
MIXR = 18157          # the GBA mixer rate every step in xm2gba is relative to
SECTOR = 2048

# ---- the GBA converter, loaded as a module (its outputs are only returned, never written here) ----
os.chdir(SRC)
spec = importlib.util.spec_from_file_location('xm2gba', os.path.join(SRC, 'tools/xm2gba.py'))
X = importlib.util.module_from_spec(spec)
spec.loader.exec_module(X)

# ---- names and artists (as the jukebox shows them) ----
names = {}
for l in open('source/songs.h', encoding='utf-8'):
    m = re.match(r'\s*SONG_XM\((\w+),"([^"]*)","([^"]*)"\)', l)
    if m: names[m.group(1)] = m.group(2)
artists = {}
for l in open('source/artists.h', encoding='utf-8'):
    m = re.match(r'ARTIST\("([^"]*)","([^"]*)"\)', l)
    if m: artists[m.group(1)] = m.group(2)
def ascii_name(s):   # the PS1 font is plain capitals
    t = s.upper()
    for a, b in (('Ê', 'E'), ('É', 'E'), ('È', 'E'), ('Á', 'A'), ('Ã', 'A'), ('Ó', 'O'), ('Ô', 'O'), ('Ñ', 'N'), ('Í', 'I'), ('Ú', 'U'), ('Ç', 'C')):
        t = t.replace(a, b)
    return ''.join(c for c in t if 32 <= ord(c) < 127 and c not in '"\\')

def carr(text, name):
    m = re.search(r'static const \w+(?: const)?\*? ?(?:const )?%s\[(\d+)\]=\{([^}]*)\};' % re.escape(name), text)
    if not m: sys.exit('missing array ' + name)
    return [v.strip() for v in m.group(2).replace('\n', '').split(',') if v.strip()]

adpcm_cache = {}
def to_adpcm(name, s8):
    if name in adpcm_cache: return adpcm_cache[name]
    pcm = (np.frombuffer(s8, dtype=np.int8).astype(np.int16) * 256).astype('<i2').tobytes()
    tmp = os.path.join(OUT, '_s.adpcm')
    r = subprocess.run([XAENC, 'spu', '0', tmp], input=pcm, capture_output=True)
    if r.returncode: sys.exit('xaenc failed: ' + r.stderr.decode())
    d = open(tmp, 'rb').read(); os.remove(tmp)
    d += bytes((-len(d)) % 16)
    adpcm_cache[name] = d
    return d

def pad(b, n): return b + bytes((-len(b)) % n)

songs = []
for sid, path in X.song_list():
    with contextlib.redirect_stdout(io.StringIO()):          # (xm2gba is chatty)
        text = X.convert(sid, path)
    P = 'xm_%s_' % sid
    m = re.search(r'static const XmSong xm_%s=\{[^}]*?,(\d+),(\d+),(\d+),(\d+)\};' % sid, text)
    nord, loop, rowN, rfr = map(int, m.groups())
    order = [int(v) for v in carr(text, P + 'order')]
    rows = [int(v) for v in carr(text, P + 'rows')]
    patoff = [int(v) for v in carr(text, P + 'patOff')]
    vt = [int(v) for v in carr(text, P + 'vt')]
    anc = [int(v) for v in carr(text, P + 'anc')]
    fx = [int(v) for v in carr(text, P + 'fx')]
    lens = [int(v) for v in carr(text, P + 'len')]
    busL = [int(v) for v in carr(text, P + 'busL')]
    busR = [int(v) for v in carr(text, P + 'busR')]
    dnames = carr(text, P + 'data')
    lab = {nm: (o, n) for nm, k, o, n in X.LABELS}
    eo, en = lab[P + 'ev']; ev = bytes(X.BLOB[eo:eo + en])
    ninst = len(anc)
    # ---- the samples, in the sound chip's format ----
    smp = bytearray(); smpoff = []
    for k in range(ninst):
        nm = dnames[k]
        if nm == '0' or not lens[k]: smpoff.append(0xFFFFFFFF); continue
        o, n = lab[nm]
        d = to_adpcm(nm, bytes(X.BLOB[o:o + lens[k]]))   # (len excludes the GBA's interpolation guard sample)
        smpoff.append(len(smp)); smp += d
    # ---- every note's pitch: the GBA step (16.16 at 18157 Hz) -> SPU pitch (4.12 at 44100 Hz) ----
    fixups = {(fx[i], fx[i + 1]): (fx[i + 2] - 256 if fx[i + 2] > 127 else fx[i + 2]) for i in range(0, len(fx) - 1, 3)}
    pitch = []
    for k in range(ninst):
        for nt in range(96):
            if not anc[k]: pitch.append(0); continue
            st = X.step_formula(anc[k], nt) + fixups.get((k, nt), 0)
            pitch.append(max(1, min(0x3FFF, int(round(st * MIXR / 705600)))))
    rowsec = (rowN + rfr / 256) / MIXR
    seconds = int(round(sum(rows[o] for o in order) * rowsec))
    # ---- the song's sequence block ----
    hdr_n = 20
    body = bytearray(); offs = {}
    def put(key, data, align=4):
        global body
        body += bytes((-(hdr_n * 4 + len(body))) % align); offs[key] = hdr_n * 4 + len(body); body += data
    put('order', bytes(order))
    put('rows', struct.pack('<%dH' % len(rows), *rows))
    put('patoff', struct.pack('<%dI' % len(patoff), *patoff))
    put('vt', struct.pack('<%dH' % len(vt), *vt))
    put('pitch', struct.pack('<%dH' % len(pitch), *pitch))
    put('smpoff', struct.pack('<%dI' % len(smpoff), *smpoff))
    put('bus', bytes(busL + busR))
    put('ev', ev)
    hdr = struct.pack('<20I', 0x31515342, nord, loop, rowN, rfr, len(rows), ninst, len(vt), len(ev), len(smp), seconds,
                      offs['order'], offs['rows'], offs['patoff'], offs['vt'], offs['pitch'], offs['smpoff'], offs['bus'], offs['ev'], 0)
    seq = pad(hdr + body, SECTOR)
    songs.append(dict(sid=sid, seq=seq, smp=pad(bytes(smp), SECTOR), smp_bytes=len(smp), seconds=seconds,
                      name=ascii_name(names.get(sid, sid)), artist=ascii_name(artists.get(names.get(sid, ''), ''))))
    print('%-30s %3d s  seq %6d B  samples %6d B (SPU)' % (sid, seconds, len(seq), len(smp)), flush=True)

# ---- SONGS.BIN: a one-sector index, then each song (sequence sectors, then sample sectors) ----
index = struct.pack('<II', 0x474E5342, len(songs)); sec = 1; blob = bytearray()
for s in songs:
    index += struct.pack('<IIII', sec, len(s['seq']) // SECTOR, len(s['smp']) // SECTOR, s['smp_bytes'])
    blob += s['seq'] + s['smp']; sec += (len(s['seq']) + len(s['smp'])) // SECTOR
assert len(index) <= SECTOR
open(os.path.join(OUT, 'SONGS.BIN'), 'wb').write(pad(index, SECTOR) + blob)
biggest = max(s['smp_bytes'] for s in songs)
print('SONGS.BIN %d KB, %d songs, biggest song needs %d KB of sound memory' % ((SECTOR + len(blob)) // 1024, len(songs), biggest // 1024))

with open(os.path.join(OUT, 'songs.inc'), 'w') as w:
    w.write('// GENERATED by tools/build_music.py: the jukebox, every BORE song (sequences + samples in SONGS.BIN on the disc)\n')
    w.write('typedef struct { u16 seconds; const char *name, *artist; } SongRow;\n')
    w.write('#define NSONGS %d\n#define SONG_SMP_MAX %d\n' % (len(songs), biggest))
    w.write('static const SongRow songs[NSONGS] = {\n')
    for s in songs: w.write('    { %d, "%s", "%s" },\n' % (s['seconds'], s['name'], s['artist']))
    w.write('};\n')
