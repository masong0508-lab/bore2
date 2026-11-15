#!/usr/bin/env python3
"""film_prof.py RAM.bin CUT.OVL.elf: the film's cost per drawing from a test build's (-DAUTO_CUT) RAM dump: the log film_log[] that cut_draw fills.
Per film second: drawings (frames the film drew), mean hblanks of CPU in each part (floor, backdrop+desk, crowd, people), mean packet KB, frames with a packet overflow."""
import os, struct, subprocess, sys
ram = open(sys.argv[1], 'rb').read(); elf = sys.argv[2]
out = subprocess.run(['/opt/mipsel/bin/mipsel-none-elf-nm', elf], capture_output=True, text=True).stdout
sy = {l.split()[2]: (int(l.split()[0], 16) & 0xffffffff) for l in out.split('\n') if len(l.split()) == 3}
n = struct.unpack_from('<i', ram, sy['film_log_n'] - 0x80000000)[0]; base = sy['film_log'] - 0x80000000; pbase = sy['film_ps_log'] - 0x80000000 if 'film_ps_log' in sy else 0
rows = []
for i in range(min(n, 600)):
    ft, tot, sb, cr, col, ink, hair, face, kb, w = struct.unpack_from('<HHBBBBBBHH', ram, base + 14 * i); rows.append((ft, tot, sb * 4, cr * 4, col * 4, ink * 4, hair * 4, face * 4, kb & 0x7fff, (1 if kb & 0x8000 else 0) | (w << 8)) + tuple(4 * b for b in ram[pbase + 5 * i: pbase + 5 * i + 5]))
print("drawings logged: %d (a ring of 600)" % len(rows))
by = {}
for r in rows: by.setdefault(r[0] // 60, []).append(r)
print("sec  n  total  set  crowd  colour  ink  hair  face   KB  ovf   (hblanks of CPU per drawing; 263 = one vblank)")
tot_ovf = 0
for s in sorted(by):
    v = by[s]; m = lambda k: sum(x[k] for x in v) / len(v); ov = sum(1 for x in v if (x[9] & 255)); tot_ovf += ov
    print("%3d %2d %6.0f %5.0f %5.0f %6.0f %5.0f %5.0f %5.0f %5.1f %3d" % (s, len(v), m(1), m(2), m(3), m(4), m(5), m(6), m(7), m(8) / 64.0, ov))
print("inside a figure's shapes (hblanks per drawing): sec  trig-tables verts xform faces(colour) faces(ink)")
for s in sorted(by):
    v = by[s]
    if 24 <= s <= 50 and s % 2 == 0: print("%3d  %5.0f %5.0f %5.0f %5.0f %5.0f" % (s, *[sum(x[10 + k] for x in v) / len(v) for k in range(5)]))
print("frames with packet overflow: %d of %d; peak packet KB %.1f" % (tot_ovf, len(rows), max(r[8] for r in rows) / 64.0))
