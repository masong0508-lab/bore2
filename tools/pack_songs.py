#!/usr/bin/env python3
"""Pack the jukebox's sequences smaller: SONGS.BIN in, SONGS.BIN out (assets/).
A sequence block's event stream (the notes, pattern by pattern) is laid out in PLAY order (the order list walked once: the intro,
then the part that loops) and LZSS-packed, so the sequencer (audio.c) unpacks it a few bytes at a time as the rows go by, into a
4 KB window. The tables (orders, rows, instruments, pitches) stay as they are. Blocks already packed (magic BSQ2) are left alone.
Usage: tools/pack_songs.py assets/SONGS.BIN [out]"""
import struct, sys

MAGIC1, MAGIC2 = 0x31515342, 0x32515342
WIN, MINM, MAXM = 4096, 3, 18

def lzss(data):
    """flags byte per 8 tokens (bit set: literal); a match is 2 bytes: offset back (1..4096, 12 bits) and length (3..18, 4 bits)."""
    out = bytearray(); i = 0; n = len(data)
    head = {}   # 3-byte hash -> list of positions (recent last)
    def add(p):
        if p + 2 < n: head.setdefault(data[p:p + 3], []).append(p)
    while i < n:
        flag_pos = len(out); out.append(0); flags = 0
        for bit in range(8):
            if i >= n: break
            best, bpos = 0, 0
            if i + 2 < n:
                for p in reversed(head.get(data[i:i + 3], [])):
                    if i - p > WIN: break
                    k = 0
                    while k < MAXM and i + k < n and data[p + k] == data[i + k]: k += 1
                    if k > best: best, bpos = k, p
                    if best == MAXM: break
            if best >= MINM:
                off = i - bpos - 1
                out.append(off & 0xff); out.append(((off >> 8) & 0x0f) | ((best - MINM) << 4))
                for q in range(i, i + best): add(q)
                i += best
            else:
                flags |= 1 << bit; out.append(data[i]); add(i); i += 1
        out[flag_pos] = flags
    return bytes(out)

def unlzss(comp, n):
    out = bytearray(); i = 0
    while len(out) < n:
        flags = comp[i]; i += 1
        for bit in range(8):
            if len(out) >= n: break
            if flags >> bit & 1: out.append(comp[i]); i += 1
            else:
                a, b = comp[i], comp[i + 1]; i += 2
                off = (a | (b & 15) << 8) + 1; L = (b >> 4) + MINM
                for _ in range(L): out.append(out[-off])
    return bytes(out)

def rows_of(ev, start, nrows):
    """a pattern's rows as lists of 3-byte events (None-padded to nrows), from the old stream at start"""
    rows = []; e = start
    while len(rows) < nrows:
        h = ev[e]; e += 1
        if h & 0x80:
            for _ in range(h & 0x7f):
                if len(rows) < nrows: rows.append([])
        else:
            rows.append([ev[e + 3 * k:e + 3 * k + 3] for k in range(h)]); e += 3 * h
    return rows

def emit(rows):
    out = bytearray(); k = 0
    while k < len(rows):
        if not rows[k]:
            j = k
            while j < len(rows) and not rows[j] and j - k < 127: j += 1
            out.append(0x80 | (j - k)); k = j
        else:
            out.append(len(rows[k])); [out.extend(x) for x in rows[k]]; k += 1
    return bytes(out)

def pack_block(b):
    h = list(struct.unpack_from('<20I', b, 0))
    magic, nord, loop, rowN, rfr, npat, ninst, nvt, evs, smps, secs, o_order, o_rows, o_patoff, o_vt, o_pitch, o_smpoff, o_bus, o_ev, pad = h
    if magic != MAGIC1: return None
    order = b[o_order:o_order + nord]
    rows = struct.unpack_from('<%dH' % npat, b, o_rows)
    patoff = struct.unpack_from('<%dI' % npat, b, o_patoff)
    ev = b[o_ev:o_ev + evs]
    pat = [emit(rows_of(ev, patoff[p], rows[p])) for p in range(npat)]
    intro = b''.join(pat[order[k]] for k in range(0, loop))
    body = b''.join(pat[order[k]] for k in range(loop, nord))
    ci, cb = lzss(intro), lzss(body)
    assert unlzss(ci, len(intro)) == intro and unlzss(cb, len(body)) == body
    tables = bytearray(b[:o_ev])                      # the header and the tables, as they were (everything before the events)
    # BSQ2: ev_size = the packed bytes; o_ev = the intro's packed stream; then: intro bytes unpacked, the body's stream offset, body bytes unpacked
    o_body = o_ev + len(ci)
    out = tables + ci + cb
    while len(out) % 4: out.append(0)
    hdr = h[:]; hdr[0] = MAGIC2; hdr[8] = len(ci) + len(cb); hdr[19] = 0
    struct.pack_into('<20I', out, 0, *hdr)
    out += struct.pack('<4I', len(intro), o_body, len(body), 0)   # (the extra words, after everything: their offset is in pad)
    struct.pack_into('<I', out, 19 * 4, len(out) - 16)
    return bytes(out), len(b), len(intro) + len(body)

def main():
    src = sys.argv[1]; dst = sys.argv[2] if len(sys.argv) > 2 else src
    d = bytearray(open(src, 'rb').read())
    mg, n = struct.unpack_from('<2I', d, 0); assert mg == 0x474E5342
    idx = [list(struct.unpack_from('<4I', d, 8 + i * 16)) for i in range(n)]
    blocks = []
    for i in range(n):
        sec, seqs, smps, smpb = idx[i]
        seq = d[sec * 2048:(sec + seqs) * 2048] if seqs else b''
        smp = d[(sec + seqs) * 2048:(sec + seqs + smps) * 2048]
        if seqs:
            r = pack_block(bytes(seq))
            if r: seq = r[0]; print('%2d seq %6d -> %6d (events unpacked %6d)' % (i, r[1], len(seq), r[2]))
        blocks.append((bytes(seq), smp, smpb))
    out = bytearray(2048); sec = 1; biggest = 0
    for i, (seq, smp, smpb) in enumerate(blocks):
        ss = (len(seq) + 2047) // 2048; ms = (len(smp) + 2047) // 2048
        if not idx[i][1]: ss = 0
        biggest = max(biggest, ss)
        struct.pack_into('<4I', out, 8 + i * 16, sec, ss, ms if idx[i][1] else idx[i][2], smpb)
        if not idx[i][1]: struct.pack_into('<4I', out, 8 + i * 16, sec, 0, idx[i][2], smpb)
        out += seq + bytes(ss * 2048 - len(seq)) if ss else b''
        out += smp
        sec += ss + (ms if idx[i][1] else 0)
        if not idx[i][1]: sec = len(out) // 2048
        while len(out) % 2048: out.append(0)
        sec = len(out) // 2048
    struct.pack_into('<2I', out, 0, 0x474E5342, n)
    open(dst, 'wb').write(out)
    print('biggest sequence block: %d sectors (%d KB)' % (biggest, biggest * 2))
main()
