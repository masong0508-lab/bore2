#!/usr/bin/env python3
# The before-and-after pictures of the town plan (docs/HOUSING_DESIGN.md): the same world made as version 6 (left) and as version 7 (right).
#   python3 mkpics.py [SEED] [OUTDIR]     writes plan7_city.png, plan7_town.png, plan7_village.png (default: the test builds' world, into docs/)
# It runs ./townmap (make it first), which draws each side from bm_block() and bm_lot_plan(); this only joins the two and labels them.
import os, subprocess, sys, tempfile
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
SEED = sys.argv[1] if len(sys.argv) > 1 else '0x5EED1234'      # (src/main.c: AUTO_CITY's world)
OUT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, 'docs')
WHICH = [('city', 'plan7_city.png'), ('town', 'plan7_town.png'), ('village', 'plan7_village.png')]
if len(sys.argv) > 3: WHICH = [(w, 'plan7_%s.png' % w) for w in sys.argv[3:]]

LEGEND = [((74, 74, 80), 'street'), ((232, 200, 84), 'avenue line'), ((202, 200, 190), 'pavement'), ((176, 84, 60), 'home (its roof)'), ((150, 104, 134), 'flats'),
          ((206, 98, 78), 'shop, cafe, bar'), ((104, 122, 156), 'office, hotel'), ((78, 130, 204), 'school, clinic, pool'), ((104, 180, 92), 'park'),
          ((150, 186, 84), 'farm'), ((112, 158, 92), 'woods, grounds'), ((206, 190, 150), 'lot for sale'), ((218, 212, 200), 'square, court'),
          ((92, 156, 214), 'water'), ((150, 150, 152), 'driveway'), ((255, 236, 40), 'door')]

def font(size):
    try: return ImageFont.load_default(size=size)
    except TypeError: return ImageFont.load_default()

def panel(which, gen):
    with tempfile.TemporaryDirectory() as d:
        ppm = os.path.join(d, 'p.ppm')
        out = subprocess.run([os.path.join(HERE, 'townmap'), 'pic', SEED, str(gen), which, ppm], check=True, capture_output=True, text=True).stdout
        im = Image.open(ppm).convert('RGB'); im.load()
    name, labels = '', []
    for line in out.splitlines():
        p = line.split()
        if p and p[0] == 'SET': name = ' '.join(p[2:p.index('kind')])
        if p and p[0] == 'NB': labels.append((int(p[1]), int(p[2]), p[3]))
    return im, name, labels

for which, fn in WHICH:
    a, name, _ = panel(which, 6)
    b, _, labels = panel(which, 7)
    d = ImageDraw.Draw(b, 'RGBA'); f = font(max(13, a.width // 110))
    for x, y, t in labels:                                       # each neighbourhood's kind, on the plan
        w = d.textlength(t, font=f); h = f.size if hasattr(f, 'size') else 11
        d.rectangle([x - w / 2 - 3, y - h / 2 - 2, x + w / 2 + 3, y + h / 2 + 3], fill=(255, 255, 255, 170))
        d.text((x - w / 2, y - h / 2 - 1), t, fill=(20, 20, 20, 255), font=f)
    gap, top = 24, max(44, a.width // 28); lf = font(max(13, a.width // 90)); tf = font(max(18, a.width // 42))
    rows = 2 if a.width * 2 < 2600 else 1; per = (len(LEGEND) + rows - 1) // rows; lh = int(lf.size * 1.9) if hasattr(lf, 'size') else 22
    W = a.width * 2 + gap; H = top + a.height + rows * lh + 12
    im = Image.new('RGB', (W, H), (250, 250, 248)); im.paste(a, (0, top)); im.paste(b, (a.width + gap, top))
    d = ImageDraw.Draw(im)
    d.text((8, (top - tf.size) // 2 - 2), 'BEFORE: %s, world version 6 (seed %s)' % (name, SEED), fill=(30, 30, 30), font=tf)
    d.text((a.width + gap + 8, (top - tf.size) // 2 - 2), 'AFTER: %s, world version 7 (the town plan)' % name, fill=(30, 30, 30), font=tf)
    for i, (c, t) in enumerate(LEGEND):
        x = 8 + (i % per) * (W - 16) // per; y = top + a.height + 8 + (i // per) * lh
        d.rectangle([x, y, x + lh - 8, y + lh - 8], fill=c, outline=(60, 60, 60)); d.text((x + lh - 2, y - 1), t, fill=(30, 30, 30), font=lf)
    os.makedirs(OUT, exist_ok=True); im.save(os.path.join(OUT, fn), optimize=True)
    print(fn, im.size, name)
