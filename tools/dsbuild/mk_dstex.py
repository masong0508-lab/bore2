#!/usr/bin/env python3
"""mk_dstex.py: the PS1's ground, floor, wall and roof art (src/world.c: outTex and newWp, the 8x8 four-colour patterns the PS1 uploads to its
texture atlas) as DS textures: ports/dsi/source/ds_tex.h. Each becomes an 8x8 16-bit DS texture that keeps the pattern's light and shade
(each texel's brightness against the pattern's average) and leaves the colour to the DS's own material, so a wall keeps the colour the player
painted it and the grass the land's colour. Run it after the PS1's patterns change; the DS build needs no Python.
   python3 tools/dsbuild/mk_dstex.py"""
import os, re
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
src = open(os.path.join(R, "src", "world.c")).read()
noise = re.search(r'#define PN_NOISE (.*)', src).group(1)
src = src.replace("PN_NOISE", noise)
pats = {}
for m in re.finditer(r'\{"([A-Z ]+)",\{((?:RGB\(\d+,\d+,\d+\),?){4})\},\{((?:"[0-3]{8}",?){8})\}\}', src):
    name = m.group(1)
    if name in pats: continue
    cols = [tuple(int(v) for v in c) for c in re.findall(r'RGB\((\d+),(\d+),(\d+)\)', m.group(2))]
    rows = re.findall(r'"([0-3]{8})"', m.group(3))
    pats[name] = (cols, rows)
# the DS's textures, in this order (the names of the PS1's patterns; WINDOWS is the DS's own: a storey of windows for the street buildings' faces)
WANT = ["GRASS", "PAVING", "ASPHALT", "SAND", "SOIL", "GRAVEL", "WOOD DECK", "POOL TILE", "BRICK", "CLAPBOARD", "STUCCO", "SHINGLE WALL", "BEADBOARD", "STONE WALL", "BRICK PATIO"]
NEUTRAL, CONTRAST = 27, 0.8   # (27 of 31: a texel as bright as the pattern's average; the pattern's swing a little calmed, as the PS1's soften_floors)
def lum(c): return 0.30 * c[0] + 0.59 * c[1] + 0.11 * c[2]
out = ["// ds_tex.h: made by tools/dsbuild/mk_dstex.py from the PS1's patterns in src/world.c (do not edit)", "#define NDSTEX %d" % (len(WANT) + 1)]
names = []
out.append("static const u16 DSTEX[NDSTEX][64] = {")
for n in WANT:
    cols, rows = pats[n]
    px = [cols[int(ch)] for r in rows for ch in r]
    avg = sum(lum(c) for c in px) / 64.0
    vals = []
    for c in px:
        v = NEUTRAL * (1 + (lum(c) / avg - 1) * CONTRAST)
        v = max(8, min(31, int(round(v))))
        vals.append(0x8000 | v | v << 5 | v << 10)
    out.append("    { " + ", ".join("0x%04X" % v for v in vals) + " },   // " + n)
    names.append("TX_" + n.replace(" ", "_"))
# WINDOWS: a pale frame and a dark pane, so any wall colour shows windows on it
win = ["33333333", "30000003", "30000003", "30000003", "30000003", "33333333", "22222222", "22222222"]
vals = []
for r in win:
    for ch in r:
        v = {"3": 31, "2": 26, "0": 9}[ch]; b = min(31, v + (4 if ch == "0" else 0))
        vals.append(0x8000 | v | v << 5 | b << 10)
out.append("    { " + ", ".join("0x%04X" % v for v in vals) + " },   // WINDOWS (the DS's own)")
names.append("TX_WINDOWS")
out.append("};")
out.append("enum { TX_NONE = -1, " + ", ".join(names) + " };")
open(os.path.join(R, "ports", "dsi", "source", "ds_tex.h"), "w").write("\n".join(out) + "\n")
print("ds_tex.h:", len(names), "textures")
