#!/usr/bin/env python3
"""film_flicker.py FRAMEDIR FIRST LAST [OUT.txt]: flicker report for a frame-by-frame dump (DuckStation -dumpinterval 1, FULL=1).
Per emulator frame: mean absolute difference to the previous frame (on a 160x120 grey copy). Reports
  - drawings per second (a "new drawing" is a frame that differs from the one before by more than 0.15 grey levels): min / mean over every second,
  - one-frame POPS: a frame that differs from BOTH neighbours while the two neighbours are alike (a piece or the ink dropping for one frame),
  - ink SHIMMER: dark pixels that switch off and on again (or on, off) within 3 drawings in a row (of the consecutive new drawings).
Prints the numbers; with OUT writes the per-frame table."""
import sys, glob, re
import numpy as np
from PIL import Image
d = sys.argv[1].rstrip('/'); first = int(sys.argv[2]); last = int(sys.argv[3]); out = sys.argv[4] if len(sys.argv) > 4 else None
fs = {}
for f in glob.glob(d + '/frame_*.png'):
    k = int(re.findall(r'(\d+)\.png', f)[0])
    if first <= k <= last: fs[k] = f
ks = sorted(fs)
def load(k): return np.asarray(Image.open(fs[k]).convert('L').resize((160, 120), Image.BILINEAR), dtype=np.float32)
prev = None; diffs = []; imgs = []
for k in ks:
    im = load(k); imgs.append(im)
    diffs.append(0.0 if prev is None else float(np.abs(im - prev).mean())); prev = im
diffs = np.array(diffs); new = diffs > 0.15
fps_sec = []
for i in range(0, len(ks) - 60, 60): fps_sec.append(int(new[i:i + 60].sum()))
dr = [i for i in range(len(ks)) if new[i]]          # the frames that are a new drawing
flash = []
for n in range(1, len(dr) - 1):
    i0, i1, i2 = dr[n - 1], dr[n], dr[n + 1]
    a = float(np.abs(imgs[i1] - imgs[i0]).mean()); b = float(np.abs(imgs[i1] - imgs[i2]).mean()); c = float(np.abs(imgs[i0] - imgs[i2]).mean())
    if a > 2.5 and b > 2.5 and c < 0.4 * min(a, b): flash.append((ks[i1], round(a, 1), round(b, 1), round(c, 1)))
sh = 0; tot = 0
for n in range(1, len(dr) - 1):
    a, b, c = imgs[dr[n - 1]] < 70, imgs[dr[n]] < 70, imgs[dr[n + 1]] < 70
    sh += int((a & ~b & c).sum() + (~a & b & ~c).sum()); tot += int((a | b | c).sum())
print("frames %d..%d (%d)  drawings/s: mean %.1f min %d (second %d)  mean steps per drawing %.2f" % (ks[0], ks[-1], len(ks), np.mean(fps_sec), min(fps_sec), int(np.argmin(fps_sec)), len(ks) / max(1, new.sum())))
print("slow seconds (<20 drawings): %d of %d" % (sum(1 for x in fps_sec if x < 20), len(fps_sec)))
print("one-frame pops: %d  %s" % (len(flash), flash[:12]))
print("ink shimmer: %d toggling pixels of %d ink pixels (%.3f%%)" % (sh, tot, 100.0 * sh / max(1, tot)))
if out:
    with open(out, 'w') as f:
        for k, x in zip(ks, diffs): f.write("%d %.3f\n" % (k, x))
        f.write("# drawings/s per second: %s\n" % fps_sec)
