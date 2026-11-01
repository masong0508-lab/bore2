#!/usr/bin/env python3
# placecheck.py RAM ELF: audit the city map window in a RAM dump (an AUTO_CITY run): things standing on the road (asphalt, lines,
# crosswalks), on the kerb-side pavement row, and two tall things side by side. Prints counts by item and the first few places.
import struct, subprocess, sys, collections
ram = open(sys.argv[1], 'rb').read()
syms = {}
for l in subprocess.run(['/opt/mipsel/bin/mipsel-none-elf-nm', sys.argv[2]], capture_output=True, text=True).stdout.split('\n'):
    p = l.split()
    if len(p) == 3: syms[p[2]] = int(p[0][-8:], 16) - 0x80000000
MW = MH = 80; BLK = 18; LOT0 = 5
life = ram[syms['lifeMap']:syms['lifeMap'] + (MW + 1) * MH]
flo = ram[syms['floorMap']:syms['floorMap'] + MW * MH]
wbx, wby = struct.unpack('<ii', ram[syms['world_wbx']:syms['world_wbx'] + 4] + ram[syms['world_wby']:syms['world_wby'] + 4])
def L(x, y): return chr(life[y * (MW + 1) + x])
def F(x, y): return flo[y * MW + x]
def road(f): return f in (20, 21, 22) or f >= 27
bad = collections.Counter(); where = collections.defaultdict(list); n = 0
for y in range(MH):
    for x in range(MW):
        c = L(x, y)
        if c in '. \0': continue
        n += 1
        if road(F(x, y)) and c not in '+':
            bad[c] += 1; where[c].append((x % BLK, y % BLK))
print('window block', wbx, wby, '| things', n, '| on the road:', sum(bad.values()))
for c, k in bad.most_common(): print('  %r x%d e.g. block-local %s' % (c, k, where[c][:6]))
kerb = collections.Counter(); kw = collections.defaultdict(list); tall = '"+36gY'; adj = collections.Counter()
for y in range(MH):
    for x in range(MW):
        c = L(x, y); lx, ly = x % BLK, y % BLK
        if c in '. \0': continue
        if (ly == LOT0 and lx >= LOT0) or (lx == LOT0 and ly >= LOT0): kerb[c] += 1; kw[c].append((lx, ly))
        if c in tall:
            for dx, dy in ((1, 0), (0, 1)):
                if x + dx < MW and y + dy < MH and L(x + dx, y + dy) in tall: adj[c + L(x + dx, y + dy)] += 1
print('on the kerb-side pavement:', ', '.join('%r x%d %s' % (c, k, kw[c][:3]) for c, k in kerb.most_common()))
print('tall things side by side:', dict(adj))
