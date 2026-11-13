#!/usr/bin/env python3
# object_scale.py: reads every item (src/items.c and items_*.inc) and prints its footprint, model extent and height in tiles / metres,
# against the size table in docs/OBJECT_SCALE.md. A tile is 256 world units = 1.13 m (a person is 384 units = 1.5 tiles = 1.7 m).
# One item unit across = 1/8 tile = 14 cm; one item pixel of height (IZ 24 units) = 10.6 cm.
import re, glob, os, sys
src = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'src')
txt = open(os.path.join(src, 'items.c')).read()
for f in sorted(glob.glob(os.path.join(src, 'items_*.inc'))) + sorted(glob.glob(os.path.join(src, '*art.h'))): txt += '\n' + open(f).read()
boxes = {}
for m in re.finditer(r'static const IBox (\w+)\[(\d*)\]\s*=\s*\{(.*?)\n?\};', txt, re.S):
    bl = re.findall(r'\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,', m.group(3))
    boxes[m.group(1)] = [tuple(map(int, b)) for b in bl]
chs = {}
for f in glob.glob(os.path.join(src, '*.h')) + glob.glob(os.path.join(src, '*.inc')):
    for m in re.finditer(r'#define\s+(CH_\w+)\s+\(\(char\)(0x[0-9A-Fa-f]+)\)', open(f).read()): chs[m.group(1)] = int(m.group(2), 16)
FL = {'F_ROT': 1, 'F_ALONG': 2, 'F_FLAT': 4, 'F_HIGH': 8, 'F_L2': 16, 'F_W2': 32, 'F_KERB': 64}
def ch(s):
    s = s.strip()
    if s in chs: return chs[s]
    m = re.match(r'\(char\)(0x[0-9A-Fa-f]+)$', s)
    if m: return int(m.group(1), 16)
    m = re.match(r"'(.*)'$", s); v = m.group(1)
    if v.startswith('\\x'): return int(v[2:], 16)
    return ord(v)
pat = re.compile(r"\{\s*('(?:\\x[0-9A-Fa-f]{2}|\\.|[^'\\])'|CH_[A-Z0-9]+|\(char\)0x[0-9A-Fa-f]+)\s*,\s*\"([^\"]*)\"\s*,\s*(\w+)\s*,\s*(\d+)\s*,\s*(\w+)\s*,\s*(\d+)\s*,\s*([^,]+),\s*(\d+)\s*\}")
rows = {}
for m in pat.finditer(txt):
    c = ch(m.group(1)); fl = 0
    for t in m.group(7).split('|'):
        if t.strip() in FL: fl |= FL[t.strip()]
    if c not in rows: rows[c] = (m.group(2), m.group(3), fl)
ls = {}
m = re.search(r'lifeSize\[\] = \{(.*?)\};', txt, re.S)
for a, b in re.findall(r"\{\s*('(?:\\x[0-9A-Fa-f]{2}|\\.|[^'\\])'|\(char\)0x[0-9A-Fa-f]+)\s*,\s*(\d+)\s*\}", m.group(1)): ls[ch(a)] = int(b)
# the real sizes (metres: across x along x high) for the things a home has; a tile is 1.13 m
REAL = {
    'BED': (0.9, 1.9, 0.55), 'SINGLE BED': (0.9, 1.9, 0.55), 'FAMILY BED': (1.6, 2.0, 0.6), 'FOUR POSTER BED': (1.8, 2.1, 2.0),
    'TODDLER BED': (0.7, 1.4, 0.5), 'SOFA': (2.0, 0.9, 0.85), 'DINING TABLE': (1.2, 1.2, 0.75), 'COUNTER': (1.0, 0.6, 0.9),
    'FRIDGE': (0.7, 0.7, 1.8), 'TOILET': (0.4, 0.7, 0.8), 'SHOWER': (0.9, 0.9, 2.0), 'TV': (1.0, 0.5, 1.2), 'BOOKSHELF': (0.9, 0.3, 1.8),
    'DRESSER': (1.0, 0.5, 0.9), 'DESK': (1.2, 0.6, 0.75), 'CHAIR': (0.5, 0.5, 0.9), 'NIGHTSTAND': (0.5, 0.4, 0.55), 'WARDROBE': (1.0, 0.6, 1.9),
}
def report():
    out = []
    for c, (name, sym, fl) in sorted(rows.items()):
        b = boxes.get(sym)
        if not b: continue
        a0 = min(x[0] for x in b); a1 = max(x[2] for x in b); b0 = min(x[1] for x in b); b1 = max(x[3] for x in b); top = max(x[5] for x in b)
        w = 2 if fl & 32 else 1; l = 2 if fl & 16 else 1
        t = ls.get(c, top)
        out.append(dict(c=c, name=name, fl=fl, w=w, l=l, across=(a1 - a0) / 8.0, along=(b1 - b0) / 8.0, h=t, life=c in ls))
    return out
if __name__ == '__main__':
    for r in report():
        print("%02X %-22s foot %dx%d  model %.2f x %.2f tiles (%.2f x %.2f m)  h %2d px = %.2f m%s" % (r['c'], r['name'], r['w'], r['l'], r['across'], r['along'], r['across'] * 1.13, r['along'] * 1.13, r['h'], r['h'] * 0.106, ' (life)' if r['life'] else ''))
