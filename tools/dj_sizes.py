#!/usr/bin/env python3
"""The DJ's table (audio.c djt): for each of the eight tracks, how many sectors of its packed sequence block a deck has to read to reach as far
as its longest excerpt (3.1 x the usual bars), and that excerpt's order count. Usage: tools/dj_sizes.py assets/SONGS.BIN"""
import struct, sys
f = open(sys.argv[1], 'rb'); d = f.read(2048)
idx = {16: ('CYNICAL', 20), 32: ('QUICKBULLET', 16), 17: ('HOT DAMN', 16), 7: ('WORTHLESS', 16), 19: ('SHOOT', 10), 35: ('OHIO', 20), 9: ('GOTTCHO', 20), 11: ('FLEXICODE', 20)}
def decode_with_pos(comp, n):
    out = bytearray(); pos = []; i = 0
    while len(out) < n:
        flags = comp[i]; i += 1
        for bit in range(8):
            if len(out) >= n: break
            if flags >> bit & 1: out.append(comp[i]); i += 1; pos.append(i)
            else:
                a, b = comp[i], comp[i + 1]; i += 2; off = (a | (b & 15) << 8) + 1
                for _ in range(((b >> 4) + 3)): out.append(out[-off]); pos.append(i)
    return bytes(out), pos
for i, (nm, bars) in idx.items():
    s, qs, ms, mb = struct.unpack('<IIII', d[8 + i * 16:24 + i * 16]); f.seek(s * 2048); b = f.read(qs * 2048)
    H = struct.unpack('<20I', b[:80]); nord, npat = H[1], H[5]; oo, orow, pad = H[11], H[12], H[19]
    x = struct.unpack('<3I', b[pad:pad + 12]); order = b[oo:oo + nord]; rows = struct.unpack('<%dH' % npat, b[orow:orow + 2 * npat])
    un, pos = decode_with_pos(b[x[1]:], x[2]); p = 0; pend = 0; marks = []
    def byte():
        global p
        c = un[p]; p += 1; return c
    for o in range(nord):
        for r in range(rows[order[o]]):
            if pend: pend -= 1; continue
            while True:
                h = byte()
                if h & 0x80:
                    h &= 0x7f
                    if not h: continue
                    pend = h - 1; break
                for _ in range(h * 3): byte()
                break
        marks.append(p)
    K = min(nord, int(bars * 3.1) + 2)
    need = x[1] + pos[marks[K - 1] - 1] + 64
    print('%-12s maxord %d sect %d' % (nm, K, -(-need // 2048)))
