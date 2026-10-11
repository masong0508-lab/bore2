#!/usr/bin/env python3
# mkchd.py <in.cue> <out.chd>: the disc as one file. A CHD v5 CD image (the format DuckStation, SwanStation and the
# libretro PlayStation cores open), compressed with the CD zlib codec ("cdzl"), the way MAME's chdman makes them.
# Only what BORE 2's discs need: one data track (MODE2/2352 or MODE1/2352) in one .bin, no subcode.
import hashlib, os, struct, sys, zlib, binascii

FRAME, SECTOR, SUB = 2448, 2352, 96          # a CHD CD frame: the 2352-byte sector, then 96 bytes of subcode
FRAMES_PER_HUNK = 8
HUNK = FRAME * FRAMES_PER_HUNK               # 19584
PAD = 4                                      # each track is padded to a multiple of 4 frames

def crc16(b):                                # CRC-16/CCITT, starting at 0xFFFF (libchdr's crc16)
    return binascii.crc_hqx(b, 0xFFFF)

class Bits:                                  # MSB-first bit writer
    def __init__(self): self.out = bytearray(); self.acc = 0; self.n = 0
    def put(self, v, nbits):
        for i in range(nbits - 1, -1, -1):
            self.acc = (self.acc << 1) | ((v >> i) & 1); self.n += 1
            if self.n == 8: self.out.append(self.acc); self.acc = 0; self.n = 0
    def done(self):
        if self.n: self.out.append(self.acc << (8 - self.n)); self.acc = 0; self.n = 0
        return bytes(self.out)

def deflate(b):                              # CHD's zlib codec is raw deflate (no zlib header)
    c = zlib.compressobj(9, zlib.DEFLATED, -15, 9)
    return c.compress(b) + c.flush()

def cdzl(hunk):                              # [ECC flags][base length][deflated sectors][deflated subcode]
    frames = len(hunk) // FRAME
    base = b''.join(hunk[i * FRAME:i * FRAME + SECTOR] for i in range(frames))
    sub = b''.join(hunk[i * FRAME + SECTOR:(i + 1) * FRAME] for i in range(frames))
    cb, cs = deflate(base), deflate(sub)
    ecc = bytes((frames + 7) // 8)           # no sector has its sync/ECC stripped: they are stored as they are
    lenb = struct.pack('>H', len(cb)) if len(hunk) < 65536 else struct.pack('>I', len(cb))[1:]
    return ecc + lenb + cb + cs

def main():
    cue, out = sys.argv[1], sys.argv[2]
    lines = open(cue).read().splitlines()
    binname = [l.split('"')[1] for l in lines if l.strip().upper().startswith('FILE')][0]
    mode = [l.split()[2] for l in lines if l.strip().upper().startswith('TRACK')][0].upper()
    if len([l for l in lines if l.strip().upper().startswith('TRACK')]) != 1: sys.exit('mkchd: one track only')
    ttype = {'MODE2/2352': 'MODE2_RAW', 'MODE1/2352': 'MODE1_RAW'}[mode]
    data = open(os.path.join(os.path.dirname(os.path.abspath(cue)), binname), 'rb').read()
    nframes = len(data) // SECTOR
    padded = (nframes + PAD - 1) // PAD * PAD
    raw = bytearray()
    for i in range(padded):
        raw += (data[i * SECTOR:(i + 1) * SECTOR] if i < nframes else bytes(SECTOR)) + bytes(SUB)
    logical = len(raw)
    hunks = (logical + HUNK - 1) // HUNK
    raw += bytes(hunks * HUNK - logical)
    rawsha1 = hashlib.sha1(bytes(raw[:logical])).digest()
    meta = ('TRACK:1 TYPE:%s SUBTYPE:NONE FRAMES:%d PREGAP:0 PGTYPE:MODE1 PGSUB:RW POSTGAP:0' % (ttype, nframes)).encode() + b'\0'
    meta_tag = b'CHT2'
    sha1 = hashlib.sha1(rawsha1 + meta_tag + hashlib.sha1(meta).digest()).digest()
    # the file: header, the metadata, the hunks, then the map
    HDR = 124
    metaoff = HDR
    entry = meta_tag + bytes([1]) + struct.pack('>I', len(meta))[1:] + struct.pack('>Q', 0) + meta   # (flags 1: in the checksum)
    firstoffs = metaoff + len(entry)
    body = bytearray(); rawmap = bytearray(); types = []; lens = []; crcs = []
    for h in range(hunks):
        hk = bytes(raw[h * HUNK:(h + 1) * HUNK]); crc = crc16(hk)
        c = cdzl(hk)
        off = firstoffs + len(body)
        if len(c) < HUNK: t, ln, blob = 0, len(c), c          # COMPRESSION_TYPE_0 (the first codec: cdzl)
        else: t, ln, blob = 4, HUNK, hk                        # COMPRESSION_NONE
        body += blob; types.append(t); lens.append(ln); crcs.append(crc)
        rawmap += bytes([t]) + struct.pack('>I', ln)[1:] + struct.pack('>Q', off)[2:] + struct.pack('>H', crc)
    lengthbits = max(1, max([l for t, l in zip(types, lens) if t == 0] or [1]).bit_length())
    bits = Bits()
    for _ in range(16): bits.put(4, 4)        # the Huffman tree for the hunk types: all 16 codes 4 bits long (code = value)
    for t in types: bits.put(t, 4)
    for t, ln, crc in zip(types, lens, crcs):
        if t == 0: bits.put(ln, lengthbits); bits.put(crc, 16)
        else: bits.put(crc, 16)
    mapdata = bits.done()
    maphdr = struct.pack('>I', len(mapdata)) + struct.pack('>Q', firstoffs)[2:] + struct.pack('>H', crc16(bytes(rawmap))) + bytes([lengthbits, 0, 0, 0])
    mapoff = firstoffs + len(body)
    hdr = b'MComprHD' + struct.pack('>II', HDR, 5) + b'cdzl' + bytes(12)
    hdr += struct.pack('>QQQII', logical, mapoff, metaoff, HUNK, FRAME) + rawsha1 + sha1 + bytes(20)
    assert len(hdr) == HDR
    with open(out, 'wb') as f: f.write(hdr + entry + bytes(body) + maphdr + mapdata)
    print('%s: %d frames, %d hunks, %d bytes (%.0f%% of the .bin)' % (out, nframes, hunks, os.path.getsize(out), 100.0 * os.path.getsize(out) / len(data)))

main()
