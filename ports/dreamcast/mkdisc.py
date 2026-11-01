#!/usr/bin/env python3
# mkdisc.py OUTDIR IP.BIN NAME=path ...: a Dreamcast GD-ROM image (bore2_dc.gdi + three tracks) holding the given files.
# Track 3 (the high-density area, from LBA 45000) starts with IP.BIN (the boot sectors, from KOS's makeip) and holds an ISO9660
# file system with the files in its root; flycast and the GD emulators boot it (1ST_READ.BIN is the game, not scrambled).
# Tracks 1 and 2 are the usual small data track and a silent audio track.
import os, struct, sys, time

SEC = 2048
HD = 45000

def both16(v): return struct.pack('<H', v) + struct.pack('>H', v)
def both32(v): return struct.pack('<I', v) + struct.pack('>I', v)
def dtime():
    t = time.gmtime()
    return bytes([t.tm_year - 1900, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, 0])
def strd(s, n): return s.encode('ascii').ljust(n, b' ')[:n]

def dirrec(name, lba, size, isdir):
    nb = name if isinstance(name, bytes) else name.encode('ascii')
    ln = 33 + len(nb); ln += ln & 1
    r = bytes([ln, 0]) + both32(lba) + both32(size) + dtime() + bytes([2 if isdir else 0, 0, 0]) + both16(1) + bytes([len(nb)]) + nb
    return r.ljust(ln, b'\0')

def iso(files, base, volid):
    # sectors (relative to the track): 0..15 IP.BIN, 16 PVD, 17 terminator, 18 L path table, 19 M path table, 20 root dir, then files
    names = sorted(files)
    lba = 21; place = {}
    for n in names:
        place[n] = lba; lba += (len(files[n]) + SEC - 1) // SEC
    total = lba
    root = dirrec(b'\0', base + 20, SEC, True) + dirrec(b'\1', base + 20, SEC, True)
    for n in names: root += dirrec(n + ';1', base + place[n], len(files[n]), False)
    assert len(root) <= SEC, 'too many files for one directory sector'
    lpt = bytes([1, 0]) + struct.pack('<I', base + 20) + struct.pack('<H', 1) + b'\0\0'
    mpt = bytes([1, 0]) + struct.pack('>I', base + 20) + struct.pack('>H', 1) + b'\0\0'
    pvd = bytes([1]) + b'CD001' + bytes([1, 0]) + strd('SEGA SEGAKATANA', 32) + strd(volid, 32) + bytes(8) + both32(base + total) + bytes(32)
    pvd += both16(1) + both16(1) + both16(SEC) + both32(len(lpt)) + struct.pack('<I', base + 18) + struct.pack('<I', 0) + struct.pack('>I', base + 19) + struct.pack('>I', 0)
    pvd += dirrec(b'\0', base + 20, SEC, True)
    pvd += strd(volid, 128) + strd('BORE 2 TEAM', 128) + strd('BORE 2 TEAM', 128) + strd('MKDISC.PY', 128)
    pvd += strd('', 37) * 3
    stamp = time.strftime('%Y%m%d%H%M%S00', time.gmtime()).encode() + b'\0'
    pvd += stamp + stamp + (b'0' * 16 + b'\0') + (b'0' * 16 + b'\0') + bytes([1, 0])
    pvd = pvd.ljust(SEC, b'\0')
    term = (bytes([255]) + b'CD001' + bytes([1])).ljust(SEC, b'\0')
    out = bytearray(total * SEC)
    out[16 * SEC:17 * SEC] = pvd; out[17 * SEC:18 * SEC] = term
    out[18 * SEC:18 * SEC + len(lpt)] = lpt; out[19 * SEC:19 * SEC + len(mpt)] = mpt
    out[20 * SEC:20 * SEC + len(root)] = root
    for n in names: out[place[n] * SEC:place[n] * SEC + len(files[n])] = files[n]
    return out

def main():
    outdir, ipbin = sys.argv[1], sys.argv[2]
    files = {}
    for a in sys.argv[3:]:
        n, p = a.split('=', 1); files[n.upper()] = open(p, 'rb').read()
    t3 = iso(files, HD, 'BORE2')
    ip = open(ipbin, 'rb').read(); assert len(ip) <= 16 * SEC
    t3[0:len(ip)] = ip
    t3 += bytes(SEC * 150)   # (a little run-out)
    t1 = iso({'README.TXT': b'BORE 2 for the Dreamcast. This is a GD-ROM image: the game is in the high-density area.\r\n'}, 0, 'BORE2')
    t1 += bytes(SEC * (300 - len(t1) // SEC))
    open(os.path.join(outdir, 'track01.bin'), 'wb').write(t1)
    open(os.path.join(outdir, 'track02.raw'), 'wb').write(bytes(2352 * 300))
    open(os.path.join(outdir, 'track03.bin'), 'wb').write(t3)
    open(os.path.join(outdir, 'bore2_dc.gdi'), 'w').write('3\n1 0 4 2048 track01.bin 0\n2 450 0 2352 track02.raw 0\n3 45000 4 2048 track03.bin 0\n')
    print('disc: %d files, track 3 %.1f MB' % (len(files), len(t3) / 1048576))

main()
