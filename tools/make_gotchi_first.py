#!/usr/bin/env python3
# BOREAGOTCHI! first run: the pixel art (src/gotchi_first_art.h, plain C, no PS1 types: the DS port can include it as it is).
# Every sprite is a list of strings; a character is a palette key (FPAL below, or one of the keys the game maps while drawing:
#   S s z  the toy's shell, light / mid / dark (the owner's colour)      L l  the LCD, on or off
#   # m d h o u g  an egg's outline, body, shade, highlight, spots, light from a crack, gold (the pet's colour: EGG_RAMP below; the game does the same sums), l the LCD's shade
# '.' is clear. Run:  python3 tools/make_gotchi_first.py [preview.png]   (writes the header; the PNG is a contact sheet to look at).
import sys, math

class C:
    def __init__(s, w, h, f='.'): s.w, s.h = w, h; s.g = [[f] * w for _ in range(h)]
    def set(s, x, y, c):
        if 0 <= x < s.w and 0 <= y < s.h: s.g[y][x] = c
    def get(s, x, y): return s.g[y][x] if 0 <= x < s.w and 0 <= y < s.h else '.'
    def rect(s, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1): s.set(x, y, c)
    def ell(s, cx, cy, rx, ry, c):
        for y in range(s.h):
            for x in range(s.w):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0: s.set(x, y, c)
    def paste(s, o, x0, y0):
        for y in range(o.h):
            for x in range(o.w):
                if o.g[y][x] != '.': s.set(x0 + x, y0 + y, o.g[y][x])
    def rows(s): return [''.join(r) for r in s.g]
    def outline(s, c, keep=''):   # an outline round everything that is not clear
        n = [r[:] for r in s.g]
        for y in range(s.h):
            for x in range(s.w):
                if s.g[y][x] == '.' and any(s.get(x + dx, y + dy) not in '.' + c for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))): n[y][x] = c
        s.g = n

S = {}   # name -> rows

# ---- the gift box (red paper, gold ribbon): body 30x17, lid 32x7, mouth 30x5 (the inside seen from above), ribbon strips, bow ----
def paper(w, h, lid):
    c = C(w, h, 'r')
    c.rect(0, 0, w - 1, 0, 'k'); c.rect(0, h - 1, w - 1, h - 1, 'k'); c.rect(0, 0, 0, h - 1, 'k'); c.rect(w - 1, 0, w - 1, h - 1, 'k')
    c.rect(1, 1, w - 2, 1, 'p') if lid else c.rect(1, 1, w - 2, 1, 'q')
    if not lid: c.rect(1, 2, w - 2, 2, 'R')
    c.rect(1, 1 if lid else 3, 1, h - 2, 'p'); c.rect(w - 3, 1, w - 2, h - 2, 'R'); c.rect(w - 2, 1, w - 2, h - 2, 'q')
    c.rect(1, h - 3, w - 2, h - 2, 'R'); c.rect(1, h - 2, w - 2, h - 2, 'q')
    if lid: c.rect(2, h - 3, w - 3, h - 3, 'R')
    return c
body = paper(30, 17, False); lid = paper(32, 7, True)
for (x, y) in ((4, 5), (9, 10), (5, 12), (21, 5), (25, 11), (20, 13), (8, 5)):
    if 12 <= x <= 17: continue
    body.set(x, y, 'c'); body.set(x + 1, y + 1, 'c'); body.set(x + 1, y, 'p'); body.set(x, y + 1, 'p')
for (x, y) in ((5, 2), (24, 2), (10, 2), (20, 3)): lid.set(x, y, 'c')
S['BOX_BODY'] = body.rows(); S['BOX_LID'] = lid.rows()
mouth = C(30, 5, 'Q'); mouth.rect(0, 0, 29, 0, 'k'); mouth.rect(0, 0, 0, 4, 'k'); mouth.rect(29, 0, 29, 4, 'k'); mouth.rect(1, 1, 28, 1, 'R'); mouth.rect(1, 2, 28, 2, 'q'); mouth.rect(0, 4, 29, 4, 'q')
S['BOX_MOUTH'] = mouth.rows()
def ribbon(h):
    c = C(6, h, 'g'); c.rect(0, 0, 0, h - 1, 'o'); c.rect(5, 0, 5, h - 1, 'o'); c.rect(1, 0, 1, h - 1, 'y'); c.rect(4, 0, 4, h - 1, 'G'); return c.rows()
S['RIB_BODY'] = ribbon(17); S['RIB_LID'] = ribbon(7)
bow = C(22, 12)
for sgn in (-1, 1):
    cx = 11 + sgn * 6
    bow.ell(cx, 4.5, 5.5, 4.4, 'o'); bow.ell(cx, 4.5, 4.6, 3.6, 'g'); bow.ell(cx + sgn * 0.5, 4.8, 2.3, 1.6, 'G'); bow.ell(cx + sgn * 0.5, 5.0, 1.4, 0.8, 'o')
    for k in range(3): bow.set(cx - 3 + k, 1 + (1 if k == 0 else 0), 'y')
    for k in range(3): bow.set(cx + 2 - k + (0 if sgn > 0 else -1), 8, 'G')
bow.rect(9, 3, 12, 8, 'o'); bow.rect(10, 4, 11, 7, 'g'); bow.set(10, 4, 'y'); bow.set(10, 5, 'y'); bow.set(11, 7, 'G')
for (x, y, c_) in ((8, 9, 'o'), (9, 9, 'g'), (9, 10, 'g'), (8, 10, 'o'), (7, 11, 'o'), (12, 9, 'g'), (13, 9, 'o'), (13, 10, 'g'), (14, 11, 'o')): bow.set(x, y, c_)
S['BOW'] = bow.rows()
for nm, sg in (('TAIL_L', 1), ('TAIL_R', -1)):
    c = C(4, 16)
    for y in range(16):
        x = 1 + (sg * (1 if (y // 3) % 2 else 0)) % 2 if sg > 0 else 1 + (0 if (y // 3) % 2 else 1)
        c.set(x, y, 'g'); c.set(x + 1, y, 'G'); c.set(x - 1, y, 'o'); c.set(x + 2, y, 'o')
    for x in range(4): c.set(x, 15 if x % 2 else 14, '.')
    S[nm] = c.rows()

# ---- the toy: an egg-shaped handheld in the owner's colour, 22x28 (keyring on top), and its back ----
def toy(front):
    W, H = 22, 28
    c = C(W, H)
    HW = {4: 4, 5: 6, 6: 7, 7: 8, 8: 9, 9: 9, 10: 10, 23: 10, 24: 9, 25: 8, 26: 6, 27: 4}
    hw = lambda y: HW.get(y, 10)
    for y in range(4, H):
        for x in range(11 - hw(y), 11 + hw(y)): c.set(x, y, 's')
    for y in range(4, H):
        for x in range(W):
            if c.get(x, y) != 's': continue
            if y in (4, 27) or any(c.get(x + dx, y + dy) == '.' for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))): c.set(x, y, 'k')
    for y in range(5, 27):
        for x in range(W):
            if c.get(x, y) != 's': continue
            l = x - (11 - hw(y)); r = (10 + hw(y)) - x
            c.set(x, y, 'S' if (l <= 2 and y < 22) else 'z' if (r <= 2 or y >= 25) else 's')
    for (x, y) in ((5, 6), (4, 7), (3, 8), (3, 9)): c.set(x, y, 'w')
    # the keyring
    for (x, y) in ((9, 0), (10, 0), (11, 0), (12, 0), (8, 1), (13, 1), (8, 2), (13, 2), (9, 3), (12, 3)): c.set(x, y, 'V')
    for (x, y) in ((10, 3), (11, 3)): c.set(x, y, 'v')
    c.set(9, 1, 'v'); c.set(10, 1, 'v')
    if front:
        c.rect(4, 8, 17, 20, 'N'); c.rect(5, 9, 16, 19, 'L'); c.rect(5, 9, 16, 9, 'l'); c.rect(5, 9, 5, 19, 'l'); c.rect(4, 20, 17, 20, 'n')
        for bx in (4, 9, 14):
            c.rect(bx, 22, bx + 3, 24, 'g'); c.rect(bx, 24, bx + 3, 24, 'G'); c.rect(bx, 22, bx + 1, 22, 'y'); c.rect(bx, 25, bx + 3, 25, 'o')
    else:
        c.ell(10.5, 14, 5.5, 6.5, 'z'); c.ell(10.2, 13.6, 5, 6, 'S'); c.ell(10.6, 14, 3, 4, 's'); c.rect(8, 12, 13, 13, 'w'); c.rect(7, 21, 14, 22, 'z')
    return c
S['TOY_FRONT'] = toy(True).rows(); S['TOY_BACK'] = toy(False).rows()

# ---- the eggs: one design in three sizes, seven patterns (0-5 by kind, 6 the rare one with gold stars) ----
# B 14x17 + 2 shadow rows (the hatching, drawn 5 px a dot), L 10x12 + 1 shadow row (a dot of the 32x16 LCD: pet view, basket, home, tree), S 8x9 (inside the toy's LCD).
# Every egg is symmetric about the LCD's own centre (an even width), an outline one dot wide in the dark of the colour, a light body, a shade down the right and the bottom,
# a highlight top left, and no dithering. Keys: # outline, m body, d shade, h highlight, o spots, g gold, u light from a crack, l the LCD's shade (the ground shadow).
EGG_N = 7
EW = {'L': (10, 12, [4, 6, 6, 8, 8, 10, 10, 10, 10, 8, 8, 4]), 'B': (14, 17, [4, 6, 8, 10, 10, 12, 12, 12, 14, 14, 14, 14, 14, 12, 10, 8, 6]), 'S': (8, 9, [4, 6, 6, 8, 8, 8, 8, 6, 4])}
EHL = {'L': [(3, 2), (2, 3), (2, 4)], 'B': [(5, 2), (4, 3), (3, 4), (3, 5), (3, 6)], 'S': [(2, 2), (1, 3), (1, 4)]}
ESH = {'L': (1, 1), 'B': (2, 1), 'S': (1, 1)}   # the shade's width down the right side, and at the bottom
def egg_base(kind):
    W, H, ws = EW[kind]; g = [['.'] * W for _ in range(H)]
    for y, w in enumerate(ws):
        for x in range((W - w) // 2, (W - w) // 2 + w): g[y][x] = 'm'
    get = lambda x, y: g[y][x] if 0 <= x < W and 0 <= y < H else '.'
    o = [r[:] for r in g]
    for y in range(H):
        for x in range(W):
            if g[y][x] == 'm' and any(get(x + dx, y + dy) == '.' for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))): o[y][x] = '#'
    g = o; sr, sd = ESH[kind]
    for y in range(H):
        for x in range(W):
            if g[y][x] != 'm': continue
            dr = 1
            while get(x + dr, y) in 'md': dr += 1
            dd = 1
            while get(x, y + dd) in 'md': dd += 1
            if (dr <= sr and y >= H * .28) or (dd <= sd and x >= W * .3): g[y][x] = 'd'
    for (x, y) in EHL[kind]:
        if g[y][x] in 'md': g[y][x] = 'h'
    return g
EPAT = {
 ('L', 0): ('s', [(5, 2), (3, 5), (7, 5), (4, 8), (7, 8)]), ('L', 1): ('r', [5, 8]), ('L', 2): ('z', 1, [6, 5, 6, 5, 6, 5, 6, 5]),
 ('L', 3): ('s', [(5, 2), (6, 2), (5, 3), (6, 3), (2, 7), (3, 7), (2, 8), (3, 8), (4, 8), (7, 6), (7, 10)]), ('L', 4): ('s', [(5, 3), (3, 5), (7, 5), (5, 7), (3, 9), (7, 9)]),
 ('L', 5): ('p', 2, 5, ['oo..oo', 'oooooo', '.oooo.', '..oo..']), ('L', 6): ('p', 3, 4, ['.gg.', 'gggg', 'gggg', '.gg.'], [(7, 8), (2, 9)]),
 ('B', 0): ('s', [(6, 3), (7, 3), (5, 7), (6, 7), (9, 5), (3, 10), (4, 10), (8, 9), (9, 9), (6, 13), (7, 13), (10, 12)]), ('B', 1): ('r', [6, 7, 11, 12]),
 ('B', 2): ('z', 1, [10, 9, 8, 9, 10, 9, 8, 9, 10, 9, 8, 9]),
 ('B', 3): ('s', [(6, 3), (7, 3), (6, 4), (7, 4), (8, 4), (2, 9), (3, 9), (4, 9), (2, 10), (3, 10), (4, 10), (3, 11), (9, 8), (10, 8), (9, 9), (10, 9), (8, 13), (9, 13), (8, 14), (9, 14)]),
 ('B', 4): ('s', [(6, 3), (7, 3), (3, 7), (4, 7), (9, 7), (10, 7), (6, 10), (7, 10), (3, 13), (10, 13)]),
 ('B', 5): ('p', 3, 6, ['.oo..oo.', 'oooooooo', 'oooooooo', '.oooooo.', '..oooo..', '...oo...']),
 ('B', 6): ('p', 4, 5, ['..gg..', '..gg..', 'gggggg', 'gggggg', '..gg..', '..gg..'], [(2, 9), (11, 12), (10, 3)]),
 ('S', 0): ('s', [(4, 2), (5, 5), (2, 6)]), ('S', 1): ('r', [5]), ('S', 2): ('z', 1, [5, 4, 5, 4, 5, 4]), ('S', 3): ('s', [(4, 2), (5, 2), (4, 3), (5, 3), (2, 6), (3, 6)]),
 ('S', 4): ('s', [(4, 2), (2, 4), (5, 4), (3, 6)]), ('S', 5): ('p', 2, 4, ['o..o', 'oooo', '.oo.']), ('S', 6): ('p', 2, 3, ['.gg.', 'gggg', '.gg.']),
}
def egg(kind, pat):
    g = egg_base(kind); W, H, _ = EW[kind]; p = EPAT[(kind, pat)]
    def put(x, y, c):
        if 0 <= y < H and 0 <= x < W and g[y][x] in 'mdh': g[y][x] = c
    if p[0] == 's':
        for (x, y) in p[1]: put(x, y, 'o')
    elif p[0] == 'r':
        for y in p[1]:
            for x in range(W): put(x, y, 'o')
    elif p[0] == 'z':
        for i, y in enumerate(p[2]): put(p[1] + i, y, 'o')
    else:
        for j, r in enumerate(p[3]):
            for i, ch in enumerate(r):
                if ch != '.': put(p[1] + i, p[2] + j, ch)
        for (x, y) in (p[4] if len(p) > 4 else []): put(x, y, 'g')
    rows = [''.join(r) for r in g]
    if kind == 'L': rows.append('.llllllll.')
    if kind == 'B': rows += ['.llllllllllll.', '...llllllll...']
    return rows
for p_ in range(EGG_N):
    for k_ in 'BLS': S['EGG%s%d' % (k_, p_)] = egg(k_, p_)
def girdle(kind):   # the line the egg splits along: a zigzag in steps of two across the middle
    W, H, _ = EW[kind]; y0 = 6 if kind == 'L' else 10
    return [y0 + (0, 0, 1, 1)[x % 4] for x in range(W)]
# crack overlays (drawn over an egg of the same size): a dark line and the light inside it. Stage 1 a hairline from the top, 2 a long crack with branches, 3 the split all round
def crack_ov(kind, stage):
    W, H, _ = EW[kind]; c = C(W, H + (2 if kind == 'B' else 1))
    path = {'L': [(5, 1), (4, 2), (5, 3), (4, 4), (5, 5), (4, 6)], 'B': [(7, 1), (6, 2), (7, 3), (6, 4), (7, 5), (6, 6), (7, 7), (6, 8)]}[kind]
    branch = {'L': [(6, 4), (7, 5), (7, 6)], 'B': [(8, 6), (9, 6), (10, 7), (10, 8), (4, 9), (3, 10), (3, 11)]}[kind]
    n = {1: len(path) // 2 + 1, 2: len(path), 3: len(path)}[stage]
    for (x, y) in path[:n]: c.set(x, y, '#')
    for (x, y) in path[1:n:2]: c.set(x + 1, y, 'u')
    if stage >= 2:
        for (x, y) in branch: c.set(x, y, '#')
        for (x, y) in branch[:3]: c.set(x, y - 1, 'u')
    if stage >= 3:
        gy = girdle(kind)
        for x in range(W):
            y = gy[x]
            if egg(kind, 0)[y][x] != '.': c.set(x, y, '#'); c.set(x, y + 1, 'u')
    return c.rows()
for st in (1, 2, 3):
    S['CRACKB%d' % st] = crack_ov('B', st); S['CRACKL%d' % st] = crack_ov('L', st)
def cup():   # the bottom of the shell, left standing when it has hatched: the egg below its split, a dark ragged edge, the light of the inside along it
    e = egg('B', 0)[:17]; gy = girdle('B'); W = 14; y0 = 10
    o = C(W, 17 - y0)
    for x in range(W):
        for y in range(gy[x] + 1, 17):
            v = e[y][x]
            if v != '.': o.set(x, y - y0, v)
        if e[gy[x]][x] != '.': o.set(x, gy[x] - y0, '#')
        if o.get(x, gy[x] - y0 + 1) in 'mdh': o.set(x, gy[x] - y0 + 1, 'u')
    return o.rows() + ['.llllllllllll.', '...llllllll...']
S['EGGCUP'] = cup()
S['SHARD0'] = ['..#..', '.#h#.', '#hmd#', '#####']
S['SHARD1'] = ['##...', '#h##.', '#mmd#', '.#dd#', '..###']
S['SHARD2'] = ['.###.', '#hmm#', '#mmd#', '.###.']
S['SHARD3'] = ['#..##', '#h#d#', '.#mm#', '..###']
# ---- sparkles, a heart for the card ----
S['SPK_S'] = ['.y.', 'yWy', '.y.']
S['SPK_M'] = ['..y..', '..y..', 'yyWyy', '..y..', '..y..']
S['SPK_L'] = ['...y...', '...y...', '.y.y.y.', '..yWy..', 'yyWWWyy', '..yWy..', '.y.y.y.', '...y...', '...y...']
# ---- a 5x7 face, A to Z and a few marks ----
_F = """
A .###. #...# #...# ##### #...# #...# #...#
B ####. #...# #...# ####. #...# #...# ####.
C .###. #...# #.... #.... #.... #...# .###.
D ####. #...# #...# #...# #...# #...# ####.
E ##### #.... #.... ####. #.... #.... #####
F ##### #.... #.... ####. #.... #.... #....
G .###. #...# #.... #.### #...# #...# .###.
H #...# #...# #...# ##### #...# #...# #...#
I .###. ..#.. ..#.. ..#.. ..#.. ..#.. .###.
J ..### ...#. ...#. ...#. ...#. #..#. .##..
K #...# #..#. #.#.. ##... #.#.. #..#. #...#
L #.... #.... #.... #.... #.... #.... #####
M #...# ##.## #.#.# #.#.# #...# #...# #...#
N #...# ##..# #.#.# #..## #...# #...# #...#
O .###. #...# #...# #...# #...# #...# .###.
P ####. #...# #...# ####. #.... #.... #....
Q .###. #...# #...# #...# #.#.# #..#. .##.#
R ####. #...# #...# ####. #.#.. #..#. #...#
S .#### #.... #.... .###. ....# ....# ####.
T ##### ..#.. ..#.. ..#.. ..#.. ..#.. ..#..
U #...# #...# #...# #...# #...# #...# .###.
V #...# #...# #...# #...# #...# .#.#. ..#..
W #...# #...# #...# #.#.# #.#.# ##.## #...#
X #...# #...# .#.#. ..#.. .#.#. #...# #...#
Y #...# #...# .#.#. ..#.. ..#.. ..#.. ..#..
Z ##### ....# ...#. ..#.. .#... #.... #####
! ..#.. ..#.. ..#.. ..#.. ..#.. ..... ..#..
- ..... ..... ..... ##### ..... ..... .....
? .###. #...# ....# ...#. ..#.. ..... ..#..
. ..... ..... ..... ..... ..... ..#.. ..#..
"""
FNT = {}
for ln in _F.strip().split('\n'):
    ch, *rw = ln.split(' ')
    assert len(rw) == 7 and all(len(r) == 5 for r in rw), ln
    FNT[ch] = ''.join(rw)
FNT[' '] = '.' * 35

# ---- the colours ----
FPAL = {
 'k': (24, 14, 36), 'w': (255, 255, 255), 'W': (246, 248, 255),
 'r': (226, 64, 96), 'R': (186, 40, 76), 'q': (138, 26, 60), 'Q': (66, 12, 40), 'p': (255, 128, 150), 'c': (255, 214, 224),
 'y': (255, 244, 168), 'g': (255, 208, 60), 'G': (230, 160, 24), 'o': (166, 96, 12),
 'v': (214, 218, 232), 'V': (150, 156, 182), 'N': (38, 20, 58), 'n': (96, 66, 128),
}
LCD_ON = (172, 196, 128); LCD_OFF = (58, 72, 70); LCD_SHADE = (150, 176, 108)   # the LCD green (the same as lcd_bg in the handheld)

EGG_CREAM = (240, 238, 206)
def egg_rgb(ink, k):   # an egg's key as a colour, from the pet's colour (the game's egg_rgb in src/gotchi_ui.c does these sums in integers)
    mixc = lambda a, b, t: tuple((a[i] * (100 - t) + b[i] * t) // 100 for i in range(3))
    return {'#': tuple(c * 42 // 100 for c in ink), 'm': mixc(EGG_CREAM, ink, 26), 'd': mixc(EGG_CREAM, ink, 52), 'o': tuple(c * 92 // 100 for c in ink), 'h': (255, 255, 240), 'u': (255, 250, 190), 'g': (255, 208, 60), 'l': LCD_SHADE}[k]

def emit(path):
    o = ['// GENERATED by tools/make_gotchi_first.py: the BOREAGOTCHI! first-run pixel art (plain C: the DS port can include this file as it is)',
         '// a sprite is a list of equal-length strings; a character is a palette key (see FPAL; the toy shell S s z, the LCD L l and the egg # m d h o u are mapped by the game), . is clear']
    o.append('#define FS_EGG_N %d   /* egg patterns per size: the last is the rare one */' % EGG_N)
    for k, rows in S.items():
        o.append('#define FS_%s_W %d\n#define FS_%s_H %d' % (k, len(rows[0]), k, len(rows)))
        o.append('static const char *const FS_%s[%d] = {%s};' % (k, len(rows), ','.join('"%s"' % r for r in rows)))
    o.append('static const char FS_FONT_CH[] = "%s";' % ''.join(FNT.keys()))
    o.append('static const char *const FS_FONT[%d] = {%s};   /* 5 x 7, row by row */' % (len(FNT), ','.join('"%s"' % v for v in FNT.values())))
    o.append('static const unsigned char FS_PAL[][4] = {%s};   /* key, r, g, b */' % ','.join("{'%s',%d,%d,%d}" % (k, *v) for k, v in FPAL.items()))
    o.append('#define FS_PAL_N %d' % len(FPAL))
    open(path, 'w').write('\n'.join(o) + '\n')

def preview(path):
    from PIL import Image
    shell = ((190, 120, 230), (150, 90, 210), (90, 54, 126)); ink = (28, 44, 24)
    mp = dict(FPAL); mp.update({'S': shell[0], 's': shell[1], 'z': shell[2], 'L': LCD_ON, 'l': LCD_SHADE})
    mp.update({k: egg_rgb((226, 112, 24), k) for k in '#mdhoul'}); mp['g'] = FPAL['g']
    names = list(S.keys()); sc = 4
    W = 1000; x = 6; y = 6; rowh = 0; items = []
    for n in names:
        r = S[n]; w, h = len(r[0]) * sc, len(r) * sc
        if x + w > W: x = 6; y += rowh + 6; rowh = 0
        items.append((n, x, y, r)); x += w + 8; rowh = max(rowh, h)
    im = Image.new('RGB', (W, y + rowh + 12), (24, 14, 56))
    for n, x, y, r in items:
        bg = LCD_ON if n.startswith(('EGG', 'CRACK', 'SHARD')) else (24, 14, 56)
        if bg == LCD_ON: im.paste(bg, (x - 2, y - 2, x + len(r[0]) * sc + 2, y + len(r) * sc + 2))
        for j, row in enumerate(r):
            for i, ch in enumerate(row):
                if ch != '.' and ch in mp: im.paste(mp[ch], (x + i * sc, y + j * sc, x + i * sc + sc, y + j * sc + sc))
    im.save(path)

def emit_json(path):   # the same art for any other port: {sprites: {name: [rows]}, palette: {key: [r, g, b]}, mapped: how the game colours the keys it sets, font: {char: 7 rows of 5}}
    import json
    f = {c: [v[i * 5:i * 5 + 5] for i in range(7)] for c, v in FNT.items()}
    mapped = {'S': 'shell, owner colour + 40', 's': 'shell, owner colour', 'z': 'shell, owner colour * 0.6', 'L': 'LCD on (172,196,128) / off (58,72,70)', 'l': 'LCD shade on (150,176,108) / off (44,56,56)', '#': 'egg outline: pet colour * 0.42', 'm': 'egg body: cream (240,238,206) 74% + pet colour 26%', 'd': 'egg shade: cream 48% + pet colour 52%', 'o': 'egg spots: pet colour * 0.92', 'h': 'highlight (255,255,240)', 'u': 'light in a crack (255,250,190)', 'g': 'gold (255,208,60)', 'l': 'LCD shade (150,176,108), the ground shadow'}
    json.dump({'sprites': S, 'palette': {k: list(v) for k, v in FPAL.items()}, 'mapped': mapped, 'font': f}, open(path, 'w'), indent=0)

if __name__ == '__main__':
    emit('src/gotchi_first_art.h'); emit_json('docs/gotchi_first_art.json')
    if len(sys.argv) > 1: preview(sys.argv[1])
    print(len(S), 'sprites')
