#!/usr/bin/env python3
# trtable.py RAM ELF: the held-camera table row from an AB_TRACE + AB_LOCK4 run (dbg_tr: need/5 per life frame).
# prints: frames, fps (each frame takes at least 3 vblanks), mean need (hb), % over 786 hb, % late (4+ vblanks), worst after the first two
import os, struct, subprocess, sys
ram, elf = sys.argv[1], sys.argv[2]
d = open(ram, 'rb').read(); S = {}
for l in subprocess.check_output(['/opt/mipsel/bin/mipsel-none-elf-nm', elf]).decode().splitlines():
    p = l.split()
    if len(p) == 3: S[p[2]] = int(p[0], 16) & 0x1fffff
a = S['dbg_tr']; n = struct.unpack('<i', d[S['dbg_life_n']:S['dbg_life_n'] + 4])[0]; n = min(n, 1000)
fr = [d[a + 4 * i] * 5 for i in range(n)]
# city frames only: skip the life's start (first 40) and any frames with no cost recorded
fr = [x for x in fr[int(os.environ.get('FROM', 40)):int(os.environ.get('TO', 100000))] if x > 0]
m = sum(fr) / len(fr)
over = 100.0 * sum(1 for x in fr if x > 786) / len(fr)
late = 100.0 * sum(1 for x in fr if (x + 40) // 263 >= 3) / len(fr)
vb = sum(max(3, (x + 40) // 263 + 1) for x in fr)
fps = 60.0 * len(fr) / vb
w = max(fr[2:])
print('%d frames | fps %.1f | mean %.0f hb | over786 %.1f%% | late %.1f%% | worst %d' % (len(fr), fps, m, over, late, w))
