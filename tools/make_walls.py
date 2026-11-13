#!/usr/bin/env python3
# make_walls.py [out.bin]: builds assets/WALLS.BIN from src/wallart.h (the KHLVH wallpaper set): the 89 wallpaper textures as finished
# 15-bit pixels, 8 x 24 each, the left-face lift (x16/12) already applied, a stored 0 turned into 0x8000 (as world_init did).
# 512 bytes a wallpaper (384 of pixels, padded), 4 to a sector. The PS1 build reads it at boot (world_init, WALLS_DISC) instead of
# keeping the indices and the palettes (22.7 KB) in RAM. The other ports keep the arrays in wallart.h.
import os, re, struct, sys
HERE = os.path.dirname(os.path.abspath(__file__))
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'assets', 'WALLS.BIN')
s = open(os.path.join(HERE, '..', 'src', 'wallart.h')).read()
def arr(name):
    m = re.search(r'static const \w+\s*\*?\s*(?:const )?wx' + name + r'\[[^=]*=\s*\{', s); j = m.end() - 1; d = 0
    for k in range(j, len(s)):
        if s[k] == '{': d += 1
        elif s[k] == '}':
            d -= 1
            if d == 0: return [int(x, 0) for x in re.findall(r'0x[0-9a-fA-F]+|\d+', s[j:k + 1])]
pal, off, idx = arr('Pal'), arr('PalOff'), arr('Idx')
n = int(re.search(r'#define NWX (\d+)', s).group(1))
assert len(off) == n and len(idx) == n * 192
buf = bytearray()
for i in range(n):
    px = []
    for y in range(24):
        for x in range(8):
            c = pal[off[i] + idx[i * 192 + x * 24 + y]]
            r, g, b = c & 31, (c >> 5) & 31, (c >> 10) & 31
            r, g, b = min(31, r * 16 // 12), min(31, g * 16 // 12), min(31, b * 16 // 12)
            o = r | g << 5 | b << 10
            px.append(o if o else 0x8000)
    buf += struct.pack('<192H', *px) + bytes(128)
buf += bytes(-len(buf) % 2048)
# the title logo's rectangles (src/logo_rects.h, tools/make_logo.py) follow, 5 bytes each (x, y, w, h, colour): main.c reads them into the crowd's
# arena while the title is up (LOGO_SEC = the sector they start at), so the 5.3 KB table is not resident on the PS1
lg = open(os.path.join(HERE, '..', 'src', 'logo_rects.h')).read()
rr = re.findall(r'\{(\d+),(\d+),(\d+),(\d+),(\d+)\}', lg.split('LOGO_R[')[1])
assert len(rr) == int(re.search(r'#define LOGO_N (\d+)', lg).group(1))
LOGO_SEC = len(buf) // 2048
for r in rr: buf += bytes(int(v) for v in r)
buf += bytes(-len(buf) % 2048)
print('logo: %d rects from sector %d' % (len(rr), LOGO_SEC))
open(out, 'wb').write(buf)
print('WALLS.BIN: %d wallpapers, %d bytes' % (n, len(buf)))
