#!/usr/bin/env python3
"""film_sheet.py FRAMEDIR OUT.png: the model sheet of the film's cast from a frame dump of a build made with -DAUTO_CUT=9998
(DuckStation regtest, FULL=1 DI=1). The gallery draws 18 cells (3 people x front, three-quarter, side, three mouths); each frame carries the cell number
(+1) as 5 white or black squares in its top-left corner. One frame per cell is taken (the middle of its run) and laid out in rows."""
import sys, glob, re
import numpy as np
from PIL import Image, ImageDraw
d = sys.argv[1].rstrip('/'); out = sys.argv[2]
fs = sorted(glob.glob(d + '/frame_*.png'))
cells = {}
for f in fs:
    im = Image.open(f).convert('RGB'); a = np.asarray(im)
    if a.shape[0] < 240: continue
    v = 0
    for b in range(5):
        px = a[4, 2 + b * 5 + 2]
        if px.mean() > 200: v |= 1 << b
    if 1 <= v <= 18: cells.setdefault(v - 1, []).append(f)
names = ["front", "three-quarter", "side", "mouth: closed", "mouth: small", "mouth: wide"]
who = ["CHAD (host)", "MISSY TELLER (judge)", "THE INTERN (young man, headset)"]
W, H = 320, 240; S = 1.3; cw, ch = int(W * S), int(H * S)
sheet = Image.new('RGB', (cw * 6, ch * 3 + 24), (232, 226, 248)); dr = ImageDraw.Draw(sheet)
dr.text((6, 6), "TV SHOW & TELL: the cast (model sheet, drawn by the film engine on the PS1 renderer)", fill=(40, 30, 80))
for c in range(18):
    r, k = c // 6, c % 6
    if c not in cells: continue
    f = cells[c][len(cells[c]) // 2]
    im = Image.open(f).convert('RGB').crop((0, 8, 320, 232)).resize((cw, int(224 * S)), Image.NEAREST)
    sheet.paste(im, (k * cw, 24 + r * ch)); dr.text((k * cw + 6, 26 + r * ch), (who[r] + ": " if k == 0 else "") + names[k], fill=(40, 30, 80))
sheet.save(out)
print("cells found:", sorted(cells))
