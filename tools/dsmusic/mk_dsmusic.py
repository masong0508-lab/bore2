#!/usr/bin/env python3
"""BORE 2 DS music: every jukebox song from assets/SONGS.BIN, as a note list the DS sound chip plays (no streaming).
For each song: its instruments' samples (decoded from the PS1 chip's ADPCM, re-encoded as the DS's IMA-ADPCM) and a flat, time-ordered list of
note events with the voices already shared out (the DS has 16 sound channels, a few of them kept for the game's own sounds).
Output: assets/dsmusic.bin (appended to the .nds by tools/dsbuild/addmusic.py; read from the cartridge by ports/dsi/source/music.h).
usage: mk_dsmusic.py assets/SONGS.BIN assets/songs.inc assets/dsmusic.bin"""
import struct, sys, re, ctypes, collections
L = ctypes.CDLL('/tmp/adpcm.so')
MIXR = 18157; FPS = 59.8261; VOICES = 9
def unlzss(comp, n):
    out = bytearray(); i = 0
    while len(out) < n:
        flags = comp[i]; i += 1
        for bit in range(8):
            if len(out) >= n: break
            if flags >> bit & 1: out.append(comp[i]); i += 1
            else:
                a, b = comp[i], comp[i + 1]; i += 2
                off = (a | (b & 15) << 8) + 1; ln = (b >> 4) + 3
                for _ in range(ln): out.append(out[-off])
    return bytes(out)
def spu_pcm(d):
    buf = (ctypes.c_int16 * (len(d) // 16 * 28 + 28))()
    n = L.spu_decode(d, len(d), buf)
    return buf, n
def ima(buf, n):
    out = (ctypes.c_uint8 * (n // 2 + 16))()
    k = L.ima_encode(buf, n, out)
    return bytes(out[:k])

data = open(sys.argv[1], 'rb').read()
rows_inc = re.findall(r'\{ (\d+), "([^"]*)", "([^"]*)", (\d+)', open(sys.argv[2]).read())
nsong = struct.unpack_from('<I', data, 4)[0]
songs = []; stats = []
for si in range(nsong):
    sec, seqs, smps, smpb = struct.unpack_from('<IIII', data, 8 + 16 * si)
    seq = data[sec * 2048:(sec + seqs) * 2048]; smp = data[(sec + seqs) * 2048:(sec + seqs + smps) * 2048]
    (magic, nord, loop, rowN, rfr, npat, ninst, nvt, evs, smps_, secs, o_order, o_rows, o_patoff, o_vt, o_pitch, o_smpoff, o_bus, o_ev, pad) = struct.unpack_from('<20I', seq, 0)
    assert magic == 0x32515342, si
    order = seq[o_order:o_order + nord]
    rows = struct.unpack_from('<%dH' % npat, seq, o_rows)
    vt = struct.unpack_from('<%dH' % nvt, seq, o_vt)
    pitch = struct.unpack_from('<%dH' % (ninst * 96), seq, o_pitch)
    smpoff = struct.unpack_from('<%dI' % ninst, seq, o_smpoff)
    bus = seq[o_bus:o_bus + 14]
    il, ob, bl, _ = struct.unpack_from('<4I', seq, pad)
    intro = unlzss(seq[o_ev:o_ev + 0x7fffffff], il) if il else b''
    body = unlzss(seq[ob:], bl)
    # ---- the samples ----
    inst_data = []; inst_len = []
    for k in range(ninst):
        so = smpoff[k]
        if so == 0xffffffff: inst_data.append(b''); inst_len.append(0); continue
        buf, n = spu_pcm(smp[so:])
        inst_data.append(ima(buf, n)); inst_len.append(n)
    # ---- the notes, in play order ----
    def walk(stream, orders):
        p = 0; res = []
        for o in orders:
            nr = rows[order[o]]; r = 0
            while r < nr:
                h = stream[p]; p += 1
                if h & 0x80:
                    k = h & 0x7f
                    if not k: continue
                    for _ in range(k):
                        if r < nr: res.append([]); r += 1
                else:
                    res.append([stream[p + 3 * j:p + 3 * j + 3] for j in range(h)]); p += 3 * h; r += 1
        return res
    prow = walk(intro, range(0, loop)) + walk(body, range(loop, nord))
    loop_row = sum(rows[order[o]] for o in range(loop))
    rowsec = (rowN + rfr / 256.0) / MIXR
    # ---- sort out the voices ----
    voice_end = [-1.0] * VOICES; voice_t = [0] * VOICES; vch = [-1] * VOICES; chan_voice = {}
    events = []   # (frame, kind, ...)
    notes_raw = []
    for ri, row in enumerate(prow):
        t = ri * rowsec
        for ev in row:
            w = ev[0] | ev[1] << 8 | ev[2] << 16
            tt = vt[(w & 255) | ((w >> 22) << 8)]
            tch = tt & 15; inn = (tt >> 4) & 31; bs = (tt >> 9) & 7; nt = (w >> 8) & 127; vol = (w >> 15) & 127
            notes_raw.append((ri, t, tch, inn, bs, nt, vol))
    # volume scale: the loudest typical note
    lv = []
    for ri, t, tch, inn, bs, nt, vol in notes_raw:
        if nt == 127 or nt >= 96 or smpoff[inn] == 0xffffffff: continue
        if bs > 6: bs = 3
        vl = (vol * bus[bs] * 320) >> 9; vr = (vol * bus[7 + bs] * 320) >> 9
        lv.append((vl + vr) / 2)
    lv.sort(); ref = lv[int(len(lv) * 0.9)] if lv else 1
    out = bytearray()
    seqev = []
    for ri, t, tch, inn, bs, nt, vol in notes_raw:
        f = int(round(t * FPS))
        if nt == 127:
            v = chan_voice.get(tch)
            if v is not None and vch[v] == tch and voice_end[v] > t: seqev.append((f, 1, v)); voice_end[v] = -1
            continue
        if nt >= 96 or smpoff[inn] == 0xffffffff or inst_len[inn] == 0: continue
        if bs > 6: bs = 3
        p = pitch[inn * 96 + nt]
        if not p: continue
        hz = max(256, min(65000, int(round(p * 44100 / 4096))))
        dur = inst_len[inn] / (p * 44100 / 4096)
        vl = (vol * bus[bs] * 320) >> 9; vr = (vol * bus[7 + bs] * 320) >> 9
        loud = (vl + vr) / 2
        dv = max(1, min(127, int(round(loud / ref * 100))))
        pan = 64 if vl + vr == 0 else max(0, min(127, int(round(64 + 63 * (vr - vl) / (vl + vr)))))
        v = chan_voice.get(tch)
        if v is None or vch[v] != tch:
            v = None
            free = [i for i in range(VOICES) if voice_end[i] <= t]
            if free: v = min(free, key=lambda i: voice_end[i])
            else: v = min(range(VOICES), key=lambda i: voice_t[i] - 0.001 * voice_end[i])   # steal the longest-playing
        kill = 1 if voice_end[v] > t else 0
        chan_voice[tch] = v; vch[v] = tch; voice_end[v] = t + dur; voice_t[v] = t
        seqev.append((f, 0, v | (kill << 6), inn, hz, dv, pan))
    seqev.sort(key=lambda e: (e[0], e[1] == 0))
    lf = int(round(loop_row * rowsec * FPS)); total = max(int(round(len(prow) * rowsec * FPS)), lf + 1)
    # encode: a delta byte (frames since the last record; 255 = wait 254, nothing else), then the event: a note (slot | 0x40 to cut the one
    # before it, instrument, Hz, volume, pan), 0x80|slot = let go, 0xFE = the loop point, 0xFF = the end (back to the loop point)
    cur = 0; lb = None; marked = False
    def wait(to):
        global cur
        d = to - cur
        while d > 254: out.append(255); d -= 254
        out.append(d); cur = to
    for e in seqev:
        if not marked and e[0] >= lf:
            wait(lf); out.append(0xFE); marked = True; lb = len(out)
        wait(e[0])
        if e[1] == 0: out += struct.pack('<BBHBB', e[2], e[3], e[4], e[5], e[6])
        else: out.append(0x80 | e[2])
    if not marked: wait(lf); out.append(0xFE); lb = len(out); marked = True
    wait(max(total, cur)); out.append(0xff)
    smpblob = bytearray(); table = []
    for k in range(ninst):
        table.append((len(smpblob), len(inst_data[k]))); smpblob += inst_data[k]
    songs.append(dict(seq=bytes(out), smp=bytes(smpblob), table=table, loop_off=lb, loop_frame=lf, frames=total, ninst=ninst,
                      name=rows_inc[si][1], artist=rows_inc[si][2], fx=int(rows_inc[si][3])))
    print('%2d %-36s notes %6d seq %7d smp %7d' % (si, rows_inc[si][1][:36], len(seqev), len(out), len(smpblob)), flush=True)
# ---- the file: header, directory, names, then each song's block (sequence, instrument table, samples), 512-aligned ----
def a(n, k=512): return (n + k - 1) // k * k
hdr = bytearray(); names = bytearray()
dirs = []
base = a(16 + 32 * len(songs) + 64 * len(songs))
blob = bytearray()
for s in songs:
    tbl = b''.join(struct.pack('<II', o, n) for o, n in s['table'])
    seq = s['seq']; blk = bytearray()
    off_seq = 0; blk += seq; blk += bytes((-len(blk)) % 4)
    off_tbl = len(blk); blk += tbl
    off_smp = a(len(blk), 4); blk += bytes(off_smp - len(blk)); blk += s['smp']
    pos = base + len(blob)
    s['pos'] = pos; s['len'] = len(blk); s['off_tbl'] = off_tbl; s['off_smp'] = off_smp
    blob += blk + bytes(a(len(blk)) - len(blk))
dirb = bytearray()
for s in songs:
    nm = (s['name'][:31]).encode().ljust(32, b'\0'); ar = (s['artist'][:23]).encode().ljust(24, b'\0')
    dirb += struct.pack('<IIIIIIIHBB', s['pos'], s['len'], s['off_tbl'], s['off_smp'], len(s['smp']), s['loop_off'], len(s['seq']), s['ninst'], s['fx'], 0)
    dirb += struct.pack('<II', s['loop_frame'], s['frames']) + nm + ar
ENT = len(dirb) // len(songs)
head = struct.pack('<4sIII', b'BDM1', len(songs), ENT, 16)
out = bytearray(head) + dirb
assert len(out) <= base, (len(out), base)
out += bytes(base - len(out)); out += blob
open(sys.argv[3], 'wb').write(out)
print('dsmusic.bin %.1f MB, %d songs, biggest samples %d KB, biggest seq %d KB' % (len(out) / 1048576, len(songs), max(len(s['smp']) for s in songs) // 1024, max(len(s['seq']) for s in songs) // 1024))
