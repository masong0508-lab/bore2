#!/usr/bin/env python3
"""BORE 2's furniture pack: new objects as textured boxes in the GBA item format (IBox / MAT), written to src/items_more.inc.
Units: a, b across the tile (0..8, 32 world units each; +b is the FRONT, the side that faces open floor), z up in 24-unit steps
(a Bore is about 16 tall, a wall 21). Each box: (a0, b0, a1, b1, z0, z1, mat) or (..., [front, +a, back, -a, top] mats)."""
import os

PAL = {   # name: (r, g, b) in 0..31
    'black': (4, 4, 6), 'char': (8, 8, 10), 'grey': (16, 16, 18), 'silver': (22, 23, 25), 'white': (29, 29, 28), 'cream': (29, 27, 22),
    'wood': (17, 11, 6), 'oak': (22, 16, 9), 'dark': (11, 7, 4), 'red': (26, 5, 5), 'orange': (30, 15, 3), 'gold': (30, 23, 6),
    'yellow': (31, 28, 8), 'green': (5, 20, 7), 'teal': (4, 20, 20), 'blue': (6, 11, 27), 'navy': (4, 5, 15), 'purple': (17, 6, 24),
    'pink': (30, 15, 21), 'water': (8, 20, 30), 'stone': (19, 18, 16), 'brick': (20, 9, 6), 'leather': (14, 7, 4), 'mint': (16, 28, 22),
}
MATS = {}
def solid(name):   # a colour with a faint two-tone weave, so big faces never look like flat plastic
    r, g, b = PAL[name]
    d = (max(r - 2, 0), max(g - 2, 0), max(b - 2, 0))
    MATS['m_' + name] = ([(r, g, b), d], 2, 2, ["ab", "ba"])
    return 'm_' + name
def special(nm, cols, rows): MATS[nm] = ([PAL[c] if isinstance(c, str) else c for c in cols], len(rows[0]), len(rows), rows); return nm

SCREEN = special('m_screen', ['navy', 'blue', (12, 26, 31), 'black'], ["dddddddd", "dbbbbbbd", "dbccbbbd", "dbbbbccd", "dbcbbbbd", "dbbbcbbd", "dbbbbbbd", "dddddddd"])
ARCADE = special('m_arcade', ['black', 'purple', 'yellow', (31, 8, 20)], ["aaaaaaaa", "abbbbbba", "abcbbdba", "abbbbbba", "abbdbbba", "abbbbcba", "abbbbbba", "aaaaaaaa"])
KEYS = special('m_keys', ['white', 'black'], ["aabaabab", "aabaabab", "aaaaaaaa"])
PAINT = special('m_paint', ['white', 'blue', 'yellow', 'red', 'green'], ["aaaaaaaa", "abbbbbba", "abbccbba", "abbbbbba", "aeeedeea", "aeeeeeea", "aeeeeeea", "aaaaaaaa"])
OVEN = special('m_oven', ['white', 'black', (10, 6, 4), 'silver'], ["aaaaaaaa", "adddddda", "abbbbbba", "abcccbba", "abcccbba", "abbbbbba", "aaaaaaaa", "aaaaaaaa"])
CHESS = special('m_chess', ['cream', 'dark'], ["abababab", "babababa", "abababab", "babababa", "abababab", "babababa", "abababab", "babababa"])
MIRROR = special('m_mirror', [(22, 27, 31), (27, 30, 31), (17, 22, 27)], ["aabbaaaa", "abbaaaaa", "bbaaaaac", "baaaaacc", "aaaaacca", "aaaacaaa", "aaacaaaa", "aacaaaaa"])
GLOW = special('m_glow', ['yellow', 'orange', 'red'], ["aabbaabb", "abbaabba", "bbccbbcc", "aabbaabb"])
for c in PAL: solid(c)
def S(c): return 'm_' + c

ITEMS = [   # letter, name, cost, flags, boxes
 ('!', 'ARCADE CABINET', 700, 'F_ROT', [(2, 1, 6, 5, 0, 14, S('black')), (2, 1, 6, 5, 14, 16, [ARCADE, S('black'), S('black'), S('black'), S('black')]),
                                         (3, 5, 5, 6, 8, 12, [SCREEN, S('black'), S('black'), S('black'), S('black')]), (2, 5, 6, 7, 5, 6, S('purple'))]),
 ('$', 'PIANO', 1200, 'F_ROT', [(0, 2, 8, 6, 0, 8, S('black')), (0, 2, 8, 4, 8, 11, S('black')), (1, 6, 7, 7, 5, 6, [KEYS, S('black'), S('black'), S('black'), KEYS])]),
 ('(', 'GUITAR', 250, 'F_ROT', [(3, 3, 5, 5, 0, 1, S('black')), (2, 4, 6, 5, 1, 6, S('orange')), (3, 4, 5, 5, 6, 13, S('dark')), (3, 4, 5, 5, 13, 15, S('black'))]),
 (')', 'EASEL', 180, 'F_ROT', [(2, 4, 3, 5, 0, 12, S('oak')), (5, 4, 6, 5, 0, 12, S('oak')), (3, 1, 5, 2, 0, 11, S('oak')),
                                (1, 5, 7, 6, 5, 13, [PAINT, S('white'), S('white'), S('white'), S('white')])]),
 (',', 'BATHTUB', 600, 'F_ROT', [(0, 1, 8, 2, 0, 4, S('white')), (0, 6, 8, 7, 0, 4, S('white')), (0, 2, 1, 6, 0, 4, S('white')), (7, 2, 8, 6, 0, 4, S('white')),
                                 (1, 2, 7, 6, 0, 3, S('water')), (7, 3, 8, 5, 4, 6, S('silver'))]),
 ('-', 'SINK', 220, 'F_ROT', [(1, 2, 7, 7, 0, 6, S('oak')), (2, 3, 6, 6, 6, 7, S('white')), (3, 2, 5, 3, 7, 9, S('silver'))]),
 ('/', 'STOVE', 500, 'F_ROT', [(1, 1, 7, 7, 0, 6, [OVEN, S('white'), S('white'), S('white'), S('white')]), (2, 2, 6, 6, 6, 7, S('char')), (1, 1, 7, 2, 6, 9, S('silver'))]),
 (':', 'MICROWAVE', 160, 'F_ROT', [(0, 1, 8, 7, 0, 6, S('oak')), (2, 2, 6, 6, 6, 9, [SCREEN, S('silver'), S('silver'), S('silver'), S('silver')])]),
 (';', 'ARMCHAIR', 350, 'F_ROT', [(1, 2, 7, 7, 0, 3, S('leather')), (1, 1, 7, 3, 3, 9, S('leather')), (0, 1, 1, 7, 0, 5, S('leather')), (7, 1, 8, 7, 0, 5, S('leather')),
                                  (2, 3, 6, 7, 3, 4, S('orange'))]),
 ('<', 'BUNK BED', 800, 'F_ROT', [(0, 1, 8, 7, 0, 3, S('wood')), (0, 1, 8, 7, 3, 4, S('white')), (0, 1, 8, 7, 8, 9, S('wood')), (0, 1, 8, 7, 9, 10, S('blue')),
                                  (0, 0, 1, 1, 0, 13, S('dark')), (7, 0, 8, 1, 0, 13, S('dark')), (0, 7, 1, 8, 0, 13, S('dark')), (7, 7, 8, 8, 0, 13, S('dark')),
                                  (7, 2, 8, 3, 4, 12, S('dark')), (7, 4, 8, 5, 4, 12, S('dark'))]),
 ('>', 'HOT TUB', 2500, '0', [(0, 0, 8, 1, 0, 4, S('wood')), (0, 7, 8, 8, 0, 4, S('wood')), (0, 1, 1, 7, 0, 4, S('wood')), (7, 1, 8, 7, 0, 4, S('wood')),
                               (1, 1, 7, 7, 0, 3, S('mint'))]),
 ('?', 'CHESS TABLE', 300, '0', [(3, 3, 5, 5, 0, 3, S('dark')), (2, 2, 6, 6, 3, 4, [S('dark'), S('dark'), S('dark'), S('dark'), CHESS]),
                                  (3, 3, 4, 4, 4, 5, S('white')), (4, 4, 5, 5, 4, 5, S('black'))]),
 ('[', 'COMPUTER', 900, 'F_ROT', [(0, 2, 8, 7, 0, 5, S('oak')), (2, 3, 6, 4, 5, 10, [SCREEN, S('black'), S('black'), S('black'), S('black')]),
                                   (2, 5, 6, 6, 5, 6, S('grey')), (6, 3, 7, 5, 5, 9, S('grey'))]),
 (']', 'WEIGHT BENCH', 450, 'F_ROT', [(2, 1, 6, 7, 0, 3, S('black')), (1, 2, 2, 3, 0, 8, S('silver')), (6, 2, 7, 3, 0, 8, S('silver')),
                                      (0, 2, 8, 3, 8, 9, S('silver')), (0, 1, 1, 4, 7, 10, S('char')), (7, 1, 8, 4, 7, 10, S('char'))]),
 ('_', 'BASKETBALL HOOP', 400, 'F_ROT', [(3, 1, 5, 3, 0, 20, S('grey')), (1, 3, 7, 4, 17, 23, S('white')), (3, 4, 5, 6, 17, 18, S('orange'))]),
 ('`', 'MIRROR', 200, 'F_ROT', [(1, 1, 7, 2, 2, 16, [MIRROR, S('gold'), S('gold'), S('gold'), S('gold')]), (1, 1, 7, 2, 16, 17, S('gold'))]),
 ('j', 'JUKEBOX', 1500, 'F_ROT', [(2, 2, 6, 6, 0, 11, S('red')), (2, 2, 6, 6, 11, 13, S('gold')), (3, 6, 5, 7, 5, 10, [GLOW, S('gold'), S('gold'), S('gold'), S('gold')])]),
 ('n', 'TOY BOX', 120, 'F_ROT', [(1, 2, 7, 6, 0, 4, S('teal')), (1, 2, 7, 6, 4, 5, S('yellow')), (2, 3, 3, 4, 5, 6, S('red')), (5, 3, 6, 5, 5, 7, S('pink'))]),
 ('{', 'DOLLHOUSE', 350, 'F_ROT', [(1, 2, 7, 6, 0, 7, S('pink')), (1, 2, 7, 6, 7, 9, S('red')), (2, 3, 6, 5, 9, 10, S('red')),
                                   (3, 6, 5, 7, 0, 3, S('white'))]),
 ('}', 'FOUNTAIN', 1800, '0', [(0, 0, 8, 1, 0, 3, S('stone')), (0, 7, 8, 8, 0, 3, S('stone')), (0, 1, 1, 7, 0, 3, S('stone')), (7, 1, 8, 7, 0, 3, S('stone')),
                                (1, 1, 7, 7, 0, 2, S('water')), (3, 3, 5, 5, 0, 8, S('stone')), (2, 2, 6, 6, 8, 9, S('stone')), (3, 3, 5, 5, 9, 10, S('water'))]),
]

def emit():
    out = ['// GENERATED by tools/make_items2.py: the furniture pack (included by items.c)\n']
    for nm, (cols, w, h, rows) in MATS.items():
        out.append('static const u16 p_%s[%d]={%s};\n' % (nm, len(cols), ','.join('RGB(%d,%d,%d)' % c for c in cols)))
        out.append('MAT(%s,p_%s,%d,%d,%s)\n' % (nm, nm, w, h, ','.join('"%s"' % r for r in rows)))
    defs = []
    for i, (ch, name, cost, flags, boxes) in enumerate(ITEMS):
        bx = []
        for (a0, b0, a1, b1, z0, z1, m) in boxes:
            assert 0 <= a0 < a1 <= 8 and 0 <= b0 < b1 <= 8 and z0 < z1, (name, a0, b0, a1, b1, z0, z1)
            ms = m if isinstance(m, list) else [m] * 5
            bx.append('{%d,%d,%d,%d,%d,%d,{%s}}' % (a0, b0, a1, b1, z0, z1, ','.join('&' + x for x in ms)))
        out.append('static const IBox bxMore%d[%d]={%s};\n' % (i, len(bx), ','.join(bx)))
        defs.append("    { '%s', \"%s\", bxMore%d, %d, 0, 0, %s, %d },   \\\n" % (ch.replace("'", "\\'"), name, i, len(bx), flags, cost))
    out.append('#define ITEMS_MORE \\\n' + ''.join(defs) + '\n')
    p = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'src', 'items_more.inc')
    open(p, 'w').write(''.join(out))
    print('%d new objects, %d materials -> %s' % (len(ITEMS), len(MATS), p))
emit()
