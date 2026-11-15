#!/usr/bin/env python3
"""film_frames.py FRAMEDIR: find where the film is in a frame-by-frame dump (DuckStation regtest, -dumpinterval 1, frame_NNNNN.png, every emulator frame).
Prints "FIRST LAST": FIRST the emulator frame where the film opens (the title screen cuts to black), LAST a few frames after the fade out has reached black
(the next picture is the game coming back). The film's own clock runs one 60 Hz step per emulator frame from FIRST (the picture can trail the clock by
about 7 frames: see docs/FILM_VOICE.md)."""
import sys, glob, re
import numpy as np
from PIL import Image
d = sys.argv[1].rstrip('/')
fs = sorted(glob.glob(d + '/frame_*.png'))
ks = [int(re.findall(r'(\d+)\.png', f)[0]) for f in fs]
def mean(i): return float(np.asarray(Image.open(fs[i]).convert('L'))[::4, ::4].mean())
first = None; last = None; prev = None
i = 0
bright = False
for i in range(len(fs)):
    m = mean(i)
    if first is None:
        if m > 80: bright = True
        elif bright and m < 40 and prev is not None and prev > 80: first = ks[i]   # a bright screen cuts to black: the film opens
    elif m < 0.5 and ks[i] > first + 3000:
        # the film's fade out reached black: 20 frames of black more, then stop (the next bright picture is the game coming back)
        last = ks[i]; k0 = ks[i]
        for j in range(i, len(fs)):
            if mean(j) > 3: break
            last = ks[j]
        break
    prev = m
print(first, min(last, k0 + 20) if last else ks[-1])
