#!/usr/bin/env python3
"""glyph_check.py: compares the glyph charts (AUTO_GLYPHS, test/glyph_check.sh) of the PS1 and the Dreamcast build, cell by cell.
   usage: python3 -I glyph_check.py PS1_FRAME_DIR DC_FRAME_DIR [OUT_PREFIX]   (png / ppm frames)
   Every frame has a white sync square at game (8, 8) (it finds where the game's pixels are: DuckStation crops the sides, the Dreamcast frame is 640 x 480)
   and the page number as 7 squares after it. Pages (src/main.c glyph_chart): look * 37 + q. q 0..35: style = q / 6, variant = (q / 3) & 1 (0 the character alone,
   1 after a Z), third = q % 3 (32 characters, 8 x 4 cells of 34 x 52 from (8, 22)); q 36: the shipped strings (lines compared one by one).
   A cell is a MISMATCH when the two pictures disagree (normalised correlation of the blurred grey crops, best of a +-1 px shift, under 0.88).
   Exit 1 on any mismatch or missing page. OUT_PREFIX_diff.png: PS1 | DC for the pages with a mismatch, the cells boxed."""
import sys, glob, os
from PIL import Image, ImageFilter, ImageDraw
import numpy as np

STY = ['BODY', 'READ', 'HEAD', 'NUM', 'FUN', 'BIG']
LOOK = ['SIMS', 'READABLE', 'CAPS']
NP = 111
THR = 0.88   # (good cells score 0.91 and up; an O drawn as a B 0.76 to 0.87)

def load_dir(d):
    pages = {}
    for f in sorted(glob.glob(os.path.join(d, '*.png')) + glob.glob(os.path.join(d, '*.ppm'))):
        im = Image.open(f).convert('RGB')
        if im.size[0] >= 600: im = im.resize((im.size[0] // 2, im.size[1] // 2), Image.BOX)   # the Dreamcast's 640 x 480
        elif im.size[1] != 240: continue                                                         # (a hi-res frame of the boot)
        elif im.size[0] != 320: im = im.resize((320, 240), Image.BOX)                            # (DuckStation's 292-wide picture of the 320-wide screen)
        g = np.asarray(im.convert('L')).astype(int)
        ys, xs = np.where(g[:26, :] > 128)
        if len(xs) < 10: continue
        x0, y0 = xs.min(), ys.min()                                                              # the sync square's corner = game (8, 8)
        if not (g[y0 + 1:y0 + 5, x0 + 1:x0 + 5] > 128).all(): continue
        dx, dy = int(x0) - 8, int(y0) - 8; pg = 0
        for i in range(7):
            bx, by = 16 + i * 8 + dx, 8 + dy
            if g[by + 1:by + 5, bx + 1:bx + 5].mean() >= 128: pg |= 1 << i
        if pg < NP: pages[pg] = (im, dx, dy)
    return pages

def crop(g, dx, dy, x, y, w, h, sx=0, sy=0):
    return g[y + dy + sy:y + dy + sy + h, x + dx + sx:x + dx + sx + w]

def corr(a, b):   # normalised correlation of two crops (nothing drawn by either: 1)
    if a.shape != b.shape: return 0.0
    a = a - a.mean(); b = b - b.mean()
    d = np.sqrt((a * a).sum() * (b * b).sum())
    return 1.0 if d < 1e-6 else float((a * b).sum() / d)

def best(g1, o1, g2, o2, x, y, w, h):
    return max(corr(crop(g1, *o1, x, y, w, h), crop(g2, *o2, x, y, w, h, sx, sy)) for sx in (-1, 0, 1) for sy in (-1, 0, 1))

def main():
    p1, dc = load_dir(sys.argv[1]), load_dir(sys.argv[2]); pre = sys.argv[3] if len(sys.argv) > 3 else None
    miss1 = [p for p in range(NP) if p not in p1]; miss2 = [p for p in range(NP) if p not in dc]
    if miss1 or miss2: print('pages not captured: PS1', miss1, 'DC', miss2)
    bad = []; tiles = []
    for pg in range(NP):
        if pg not in p1 or pg not in dc: continue
        look, q = pg // 37, pg % 37
        (i1, dx1, dy1), (i2, dx2, dy2) = p1[pg], dc[pg]
        g1 = np.asarray(i1.convert('L').filter(ImageFilter.GaussianBlur(1.0))).astype(float)
        g2 = np.asarray(i2.convert('L').filter(ImageFilter.GaussianBlur(1.0))).astype(float)
        o1, o2 = (dx1, dy1), (dx2, dy2); badc = []
        if q < 36:
            third, var = q % 3, (q // 3) & 1
            for i in range(32):
                c = 33 + third * 32 + i
                if c > 126: break
                x, y = 8 + (i % 8) * 34, 22 + (i // 8) * 52
                r = best(g1, o1, g2, o2, x, y, 34, 52)
                if r < THR and not (var == 0 and chr(c) in 'OX'): badc.append((chr(c), x, y, 34, 52, r))   # (a lone O or X is the pad's button: B and A on the Dreamcast)
        else:
            # the shipped strings: each is drawn twice, 160 px apart (src/main.c): through font_shaped as ui_textfit does, and straight; they must match exactly
            rows = [('BODY', 20 + i * 11, 12) for i in range(9)] + [('HEAD', 126 + i * 12, 14) for i in range(3)] + [('FUN', 168, 22), ('NUM', 188, 14), ('BIG', 205, 28)]
            for ln, (nm, yy, hh) in enumerate(rows):
                for (nmi, ii, o) in (('PS1', i1, o1), ('DC', i2, o2)):
                    g = np.asarray(ii.convert('L')).astype(int)
                    a = crop(g, *o, 8, yy, 150, hh); b2 = crop(g, *o, 168, yy, 150, hh)
                    n = int((abs(a - b2) > 60).sum())
                    if n > 4: badc.append(('%s line %d: shaped and direct differ on the %s' % (nm, ln, nmi), 8, yy, 310, hh, 1.0 - n / max(1, int((a > 60).sum()))))
            # the raw buttons line (7): the Dreamcast's names against what the PS1 draws for them
            r = best(g1, o1, g2, o2, 8, 20 + 7 * 11, 150, 12)
            if r < 0.85: badc.append(('buttons line', 8, 20 + 7 * 11, 150, 12, r))
        for b in badc:
            if q < 36: what = 'char %r' % b[0]; sty = STY[q // 6]; var = ('alone', 'after Z')[(q // 3) & 1]
            else: what = 'words ' + b[0]; sty = 'mixed'; var = ''
            bad.append('look %-8s style %-5s %-7s %s (corr %.2f)' % (LOOK[look], sty, var, what, b[5]))
        if badc and pre:
            im = Image.new('RGB', (640, 240))
            im.paste(i1.crop((dx1, dy1, dx1 + 320, dy1 + 240)), (0, 0)); im.paste(i2.crop((dx2, dy2, dx2 + 320, dy2 + 240)), (320, 0))
            d = ImageDraw.Draw(im)
            for b in badc:
                d.rectangle([b[1], b[2], b[1] + b[3] - 1, b[2] + b[4] - 1], outline=(255, 0, 0)); d.rectangle([320 + b[1], b[2], 320 + b[1] + b[3] - 1, b[2] + b[4] - 1], outline=(255, 0, 0))
            tiles.append(im)
    if pre and tiles:
        rows = (min(len(tiles), 12) + 1) // 2
        sheet = Image.new('RGB', (1280, 240 * rows))
        for i, t in enumerate(tiles[:12]): sheet.paste(t, (640 * (i % 2), 240 * (i // 2)))
        sheet.save(pre + '_diff.png')
    print('%d pages compared, %d mismatches' % (len([p for p in range(NP) if p in p1 and p in dc]), len(bad)))
    for b in bad: print('MISMATCH', b)
    return 1 if bad or miss1 or miss2 else 0

sys.exit(main())
