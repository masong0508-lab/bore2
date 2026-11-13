# nhood_frames.py DIR FIRST [v]: the neighborhood screen frame by frame (DI=1 FULL=1 test/ds.sh BUILD NAME 3300 test/nhood_move.txt, then DIR = the dump folder): counts the pictures where the card, hint bar, label or key row is missing, and how much the model moves between pictures
import sys, glob, numpy as np
from PIL import Image
d = sys.argv[1]
fs = sorted(glob.glob(d + '/frame_0*.png'))
prev = None
rows = []
for f in fs:
    n = int(f.split('_')[-1][:-4])
    if n < int(sys.argv[2]):
        continue
    im = Image.open(f).convert('RGB')
    if im.size != (320, 240):
        im = im.resize((320, 240))
    a = np.asarray(im).astype(int)
    if prev is not None and np.array_equal(a, prev):
        continue                      # same picture shown again (the game makes a new one every 4-5 vblanks)
    pan = a[24:200, 198:312]
    panel_ok = (pan.min(axis=2) > 200).mean() > 0.35          # the white card
    hint = a[208:236, 40:290]
    hint_ok = (hint.min(axis=2) > 235).mean() > 0.02          # the hint bar's white keys and text
    lab = a[8:24, 10:120]
    lab_ok = (lab.min(axis=2) > 235).mean() > 0.2             # the label's white
    # the card's text lines, bottom one (HOME / EMPTY key row) must be there too
    key = a[150:172, 198:312]
    key_ok = (key.min(axis=2) > 200).mean() < 0.9 and ((key[:, :, 1] > 150) & (key[:, :, 0] < 120)).sum() > 8
    mod = a[24:200, 0:190]
    diff = 0 if prev is None else np.abs(mod - prev[24:200, 0:190]).mean()
    rows.append((n, panel_ok, hint_ok, lab_ok, key_ok, diff))
    prev = a
N = len(rows)
print('distinct pictures', N)
print('panel missing', sum(not r[1] for r in rows), ' hint bar missing', sum(not r[2] for r in rows),
      ' label missing', sum(not r[3] for r in rows), ' key row missing', sum(not r[4] for r in rows),
      ' any UI missing', sum(not (r[1] and r[2] and r[3] and r[4]) for r in rows))
ds = np.array([r[5] for r in rows[1:]])
print('model diff between pictures: mean %.2f  max %.2f  >8: %d' % (ds.mean(), ds.max(), (ds > 8).sum()))
if len(sys.argv) > 3:
    for r in rows: print(r[0], int(r[1]), int(r[2]), int(r[3]), int(r[4]), round(r[5], 1))
