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
PAL.update({'felt': (4, 17, 8), 'leaf': (5, 17, 5), 'sand': (27, 23, 13), 'dirt': (12, 8, 4)})
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

DART = special('m_dart', ['red', 'black', 'white', 'green'], ["abcdcbab", "babcbabc", "cbabababc"[:8], "dcbabcbd", "dcbabcbd", "cbabababc"[:8], "babcbabc", "abcdcbab"])
FLOWERS = special('m_flowers', ['leaf', 'pink', 'yellow', 'red'], ["abababab", "bacabaab", "abababcb", "bdabaaba", "ababbaba", "cbabdbab", "abababab", "babaabab"])
CLOCKF = special('m_clockf', ['gold', 'white', 'black'], ["aaaaaa", "abbbba", "abcbba", "abbcba", "abbbba", "aaaaaa"])
def solids(c): return S(c)
def corners(h0, h1, m, t=1):   # four corner posts
    return [(0, 0, t, t, h0, h1, m), (8 - t, 0, 8, t, h0, h1, m), (0, 8 - t, t, 8, h0, h1, m), (8 - t, 8 - t, 8, 8, h0, h1, m)]
def mk(code, name, cost, flags, boxes): ITEMS.append((code, name, cost, flags, boxes))
mk(0x90, 'LOVESEAT', 380, 'F_ROT', [(0,2,8,7,0,3,S('blue')),(0,1,8,3,3,9,S('blue')),(0,2,1,7,3,6,S('navy')),(7,2,8,7,3,6,S('navy'))])
mk(0x91, 'RECLINER', 450, 'F_ROT', [(1,2,7,7,0,3,S('red')),(1,1,7,3,3,10,S('red')),(0,2,1,7,0,6,S('dark')),(7,2,8,7,0,6,S('dark'))])
mk(0x92, 'BAR STOOL', 70, '0', [(3,3,5,5,0,6,S('silver')),(2,2,6,6,6,8,S('red'))])
mk(0x93, 'FOUR POSTER BED', 1000, 'F_ROT', [(0,0,8,8,0,3,S('wood')),(1,1,7,7,3,5,S('white')),(1,1,7,3,5,6,S('pink'))] + corners(3,16,S('dark')))
mk(0x94, 'HAMMOCK', 200, 'F_ROT', [(0,3,1,5,0,12,S('wood')),(7,3,8,5,0,12,S('wood')),(1,2,7,6,4,6,S('cream'))])
mk(0x95, 'COFFEE TABLE', 150, '0', [(1,2,7,6,3,4,S('oak')),(1,2,2,3,0,3,S('dark')),(6,2,7,3,0,3,S('dark')),(1,5,2,6,0,3,S('dark')),(6,5,7,6,0,3,S('dark'))])
mk(0x96, 'NIGHTSTAND', 120, 'F_ROT', [(1,2,7,7,0,6,S('wood')),(3,3,5,5,6,7,S('gold')),(2,2,6,6,7,10,S('cream'))])
mk(0x97, 'WARDROBE', 500, 'F_ROT', [(0,2,8,7,0,20,S('dark')),(3,7,4,8,8,12,S('gold')),(4,7,5,8,8,12,S('gold'))])
mk(0x98, 'FILING CABINET', 160, 'F_ROT', [(2,2,6,7,0,12,S('grey')),(3,7,5,8,3,4,S('silver')),(3,7,5,8,7,8,S('silver'))])
mk(0x99, 'STORAGE CHEST', 180, 'F_ROT', [(1,2,7,6,0,5,S('wood')),(1,2,7,6,5,6,S('gold')),(3,6,5,7,3,5,S('gold'))])
mk(0x9A, 'KITCHEN ISLAND', 400, 'F_ROT', [(0,1,8,7,0,7,S('oak')),(0,1,8,7,7,8,S('white'))])
mk(0x9B, 'WORKBENCH', 280, 'F_ROT', [(0,2,8,6,5,6,S('oak')),(0,2,1,6,0,5,S('dark')),(7,2,8,6,0,5,S('dark')),(2,2,5,5,6,8,S('red'))])
mk(0x9C, 'DISHWASHER', 450, 'F_ROT', [(1,1,7,7,0,7,S('silver')),(1,6,7,7,5,6,S('black'))])
mk(0x9D, 'WASHING MACHINE', 400, 'F_ROT', [(1,1,7,7,0,12,S('white')),(2,7,6,8,3,8,S('blue'))])
mk(0x9E, 'BBQ GRILL', 350, '0', [(1,2,7,6,6,9,S('black')),(2,3,3,4,0,6,S('grey')),(5,3,6,4,0,6,S('grey')),(2,2,6,6,9,10,S('red'))])
mk(0x9F, 'WATER COOLER', 120, '0', [(2,2,6,6,0,10,S('white')),(3,3,5,5,10,15,S('water'))])
mk(0xA0, 'DRUM KIT', 600, 'F_ROT', [(1,2,7,6,0,3,S('red')),(0,1,2,3,3,8,S('gold')),(6,1,8,3,3,8,S('gold')),(3,3,5,5,3,6,S('silver'))])
mk(0xA1, 'POOL TABLE', 900, '0', [(0,0,8,8,0,6,S('dark')),(1,1,7,7,6,7,S('felt'))])
mk(0xA2, 'TELESCOPE', 500, '0', [(3,3,5,5,0,6,S('grey')),(2,2,6,6,6,8,S('silver')),(3,3,5,6,8,12,S('navy'))])
mk(0xA3, 'SYNTH', 500, 'F_ROT', [(0,2,8,5,0,5,S('black')),(1,5,7,6,5,6,[KEYS,S('black'),S('black'),S('black'),KEYS])])
mk(0xA4, 'FOOSBALL', 450, '0', [(0,1,8,7,0,6,S('green')),(0,3,8,4,6,8,S('silver')),(0,5,8,6,6,8,S('silver'))])
mk(0xA5, 'DARTBOARD', 90, 'F_ROT', [(1,1,7,2,6,14,[DART,S('black'),S('black'),S('black'),S('black')])])
mk(0xA6, 'SLOT MACHINE', 800, 'F_ROT', [(2,2,6,6,0,14,S('red')),(2,2,6,6,14,16,S('gold')),(3,6,5,7,6,11,[GLOW,S('gold'),S('gold'),S('gold'),S('gold')])])
mk(0xA7, 'RECORD PLAYER', 300, 'F_ROT', [(0,2,8,6,0,5,S('oak')),(1,3,7,5,5,6,S('black')),(2,3,3,4,6,8,S('silver'))])
mk(0xA8, 'MASSAGE CHAIR', 700, 'F_ROT', [(1,2,7,7,0,4,S('leather')),(1,1,7,3,4,12,S('leather')),(0,2,1,7,3,6,S('black')),(7,2,8,7,3,6,S('black'))])
mk(0xA9, 'PUNCHING BAG', 250, '0', [(3,3,5,5,0,2,S('grey')),(2,2,6,6,2,14,S('red'))])
mk(0xAA, 'GRANDFATHER CLOCK', 600, 'F_ROT', [(2,3,6,6,0,22,S('dark')),(3,6,5,7,14,18,[CLOCKF,S('gold'),S('gold'),S('gold'),S('gold')])])
mk(0xAB, 'STATUE', 700, '0', [(2,2,6,6,0,4,S('stone')),(3,3,5,5,4,14,S('stone')),(3,3,5,5,14,18,S('white'))])
mk(0xAC, 'VASE', 60, '0', [(3,3,5,5,0,5,S('blue')),(2,2,6,6,2,4,S('blue')),(3,3,5,5,5,8,S('pink'))])
mk(0xAD, 'CHRISTMAS TREE', 120, '0', [(3,3,5,5,0,3,S('dark')),(1,1,7,7,3,8,S('green')),(2,2,6,6,8,13,S('green')),(3,3,5,5,13,16,S('green')),(3,3,5,5,16,17,S('gold'))])
mk(0xAE, 'FRAMED ART', 150, 'F_ROT', [(1,1,7,2,4,14,[PAINT,S('gold'),S('gold'),S('gold'),S('gold')])])
mk(0xAF, 'GLOBE', 90, '0', [(3,3,5,5,0,3,S('wood')),(2,2,6,6,3,8,S('blue'))])
mk(0xB0, 'CANDELABRA', 130, '0', [(3,3,5,5,0,8,S('gold')),(1,3,7,5,8,9,S('gold')),(1,3,2,5,9,11,S('cream')),(3,3,5,5,9,12,S('cream')),(6,3,7,5,9,11,S('cream'))])
mk(0xB1, 'SUIT OF ARMOR', 400, 'F_ROT', [(2,3,6,6,0,6,S('silver')),(2,3,6,6,6,14,S('grey')),(3,3,5,5,14,17,S('silver'))])
mk(0xB2, 'FLOWER BED', 50, '0', [(0,1,8,7,0,3,[S('brick')]*4+[FLOWERS]),(1,2,7,6,3,4,[S('leaf')]*4+[FLOWERS])])
mk(0xB3, 'HEDGE', 40, 'F_ALONG', [(0,2,8,6,0,10,S('leaf'))])
mk(0xB4, 'PICKET FENCE', 40, 'F_ALONG', [(0,3,8,4,2,9,S('white')),(0,3,1,4,0,11,S('white')),(7,3,8,4,0,11,S('white'))])
mk(0xB5, 'MAILBOX', 40, 'F_ROT', [(3,3,4,4,0,10,S('dark')),(2,2,6,5,10,13,S('blue')),(6,3,7,4,10,14,S('red'))])
mk(0xB6, 'BIRDBATH', 120, '0', [(3,3,5,5,0,8,S('stone')),(1,1,7,7,8,10,S('stone')),(2,2,6,6,10,11,S('water'))])
mk(0xB7, 'GARDEN GNOME', 50, '0', [(3,3,5,5,0,5,S('blue')),(3,3,5,5,5,8,S('pink')),(3,3,5,5,8,11,S('red'))])
mk(0xB8, 'TRAMPOLINE', 400, '0', [(0,0,8,8,3,4,S('black')),(1,1,7,7,4,5,S('blue')),(0,0,1,1,0,3,S('silver')),(7,0,8,1,0,3,S('silver')),(0,7,1,8,0,3,S('silver')),(7,7,8,8,0,3,S('silver'))])
mk(0xB9, 'SWING SET', 500, 'F_ROT', [(0,3,1,5,0,18,S('wood')),(7,3,8,5,0,18,S('wood')),(0,3,8,5,17,18,S('wood')),(2,3,3,4,5,17,S('silver')),(5,3,6,4,5,17,S('silver')),(2,3,6,4,4,5,S('red'))])
mk(0xBA, 'SANDBOX', 180, '0', [(0,0,8,8,0,2,S('wood')),(1,1,7,7,2,3,S('sand'))])
mk(0xBB, 'DOG HOUSE', 200, 'F_ROT', [(1,1,7,7,0,6,S('red')),(0,1,8,7,6,9,S('dark')),(3,7,5,8,0,4,S('black'))])
mk(0xBC, 'FIRE PIT', 150, '0', [(0,0,8,1,0,2,S('stone')),(0,7,8,8,0,2,S('stone')),(0,1,1,7,0,2,S('stone')),(7,1,8,7,0,2,S('stone')),(3,3,5,5,0,5,[GLOW]*5)])
mk(0xBD, 'BEACH UMBRELLA', 100, '0', [(3,3,5,5,0,16,S('white')),(0,0,8,8,16,18,S('red')),(1,1,7,7,18,19,S('red'))])

# ---- the second look: the same things with more to them (doors, drawers, burners, cushions, legs). The footprints and heights stay put, so
# ---- placing, walking round and standing on them are as before; only the faces and the little parts change. (redo() swaps a thing's boxes by name)
def lite(c, d): return tuple(min(31, v + d) for v in c)
def dark_(c, d): return tuple(max(0, v - d) for v in c)
def mat2(nm, cols, rows): MATS[nm] = (cols, len(rows[0]), len(rows), rows); return nm
def cushion(nm, c): return mat2('m_cu_' + nm, [lite(c, 3), c, dark_(c, 4)], ["abba", "bbbb", "bbbb", "cccc"])
def doors(nm, c, hd, n=2, w=8, h=6):
    cols = [c, dark_(c, 5), hd, lite(c, 2)]
    rows = []
    for y in range(h):
        r = ''
        for x in range(w):
            edge = y == 0 or y == h - 1 or x == 0 or x == w - 1 or (n == 2 and x == w // 2)
            hnd = y == h // 2 and (x in (w // 2 - 2, w // 2 + 1) if n == 2 else x == w - 3)
            r += 'c' if hnd else 'b' if edge else 'd' if y == 1 else 'a'
        rows.append(r)
    return mat2(nm, cols, rows)
def drawers(nm, c, hd, n=2, w=8, h=8):
    cols = [c, dark_(c, 5), hd, lite(c, 2)]
    step = h // n; rows = []
    for y in range(h):
        r = ''
        for x in range(w):
            top = y % step == 0; mid = y % step == step // 2
            r += 'c' if mid and x in (w // 2 - 1, w // 2) else 'b' if top or x == 0 or x == w - 1 or y == h - 1 else 'd' if y % step == 1 else 'a'
        rows.append(r)
    return mat2(nm, cols, rows)
def redo(name, boxes):
    for i, it in enumerate(ITEMS):
        if it[1] == name: ITEMS[i] = it[:4] + (boxes,); return
    raise KeyError(name)
def faces(front, other, top=None): return [front, other, other, other, top or other]

# materials
W_TOP = mat2('m_woodtop', [PAL['oak'], dark_(PAL['oak'], 3), lite(PAL['oak'], 2)], ["aabaaaba", "cccccccc", "abaaabaa", "cccccccc"])
MARB = mat2('m_marble', [PAL['white'], (26, 26, 27), (23, 24, 26)], ["aaaaabaa", "aabaaaaa", "aaaabbaa", "abaaaaac", "aaaaaaaa", "aacaaaba"])
BENCHTOP = mat2('m_bench', [PAL['oak'], dark_(PAL['oak'], 4), (19, 12, 6)], ["aaaaaaaa", "bbbbbbbb", "aaaaaaaa", "ccccbccc"])
BURN = mat2('m_burners', [PAL['char'], (4, 4, 6), (22, 5, 4), PAL['grey']], ["aaaaaaaa", "abbaabba", "abcaabca", "abbaabba", "aaaaaaaa", "abbaabba", "abcaabca", "abbaabba"][:8])
STOVEF = mat2('m_stovef', [PAL['white'], PAL['black'], PAL['silver'], (10, 6, 4), (22, 5, 4)], ["aaaaaaaa", "acacacac", "aaaaaaaa", "abbbbbba", "abdddbba", "abdddbba", "abbbbbba", "acccccca"])
MWF = mat2('m_mwf', [PAL['silver'], PAL['black'], (9, 14, 20), PAL['char'], (24, 6, 6)], ["aaaaaaaa", "abbbbcda", "abbbbcda", "abbbbcda", "abbbbcda", "abbbbcda", "aaaaaaaa"])
DWF = mat2('m_dwf', [PAL['silver'], PAL['black'], PAL['grey'], (6, 20, 31)], ["abababab", "acccccca", "aaaaaaaa", "aaaaaaaa", "abbbbbba", "aaaaaaaa", "aaaaaaaa"])
WMF = mat2('m_wmf', [PAL['white'], PAL['char'], (10, 18, 28), PAL['silver'], (17, 25, 31)], ["adadaaaa", "aaaaaaaa", "aabbbbaa", "abccccba", "abccdcba", "abccccba", "aabbbbaa", "aaaaaaaa"])
KEYS2 = mat2('m_keys2', [PAL['white'], PAL['black'], (20, 20, 22)], ["abababaa", "abababaa", "abababaa", "aaaaaaaa", "cccccccc"])
BATHW = mat2('m_bathw', [PAL['water'], lite(PAL['water'], 4), dark_(PAL['water'], 4)], ["aabaaaba", "baaabaaa", "aaabaaab", "abaaaaba"])
for nm, c in (('blue', PAL['blue']), ('red', PAL['red']), ('orange', PAL['orange']), ('cream', PAL['cream']), ('pink', PAL['pink'])): cushion(nm, c)
D_WARD = doors('m_d_ward', PAL['dark'], PAL['gold'], 2, 8, 12)
D_OAK = doors('m_d_oak', PAL['oak'], PAL['dark'], 2, 8, 6)
D_ISL = doors('m_d_isl', PAL['oak'], PAL['silver'], 2, 8, 7)
DR_NS = drawers('m_dr_ns', PAL['wood'], PAL['gold'], 2, 6, 6)
DR_FILE = drawers('m_dr_file', PAL['grey'], PAL['silver'], 3, 4, 12)
DR_BENCH = drawers('m_dr_bench', PAL['dark'], PAL['silver'], 2, 4, 4)

LEGD = lambda: [(0, 1, 1, 2, 0, 1, S('dark')), (7, 1, 8, 2, 0, 1, S('dark')), (0, 6, 1, 7, 0, 1, S('dark')), (7, 6, 8, 7, 0, 1, S('dark'))]
def sofa(c, cu, frame, w0=0, w1=8, seats=2):
    m = S(c); fr = S(frame); cm = 'm_cu_' + cu
    bx = [(w0, 1, w1, 7, 1, 3, fr), (w0, 1, w1, 2, 3, 9, fr), (w0, 2, w0 + 1, 7, 3, 7, fr), (w1 - 1, 2, w1, 7, 3, 7, fr), (w0, 2, w0 + 1, 7, 7, 8, m), (w1 - 1, 2, w1, 7, 7, 8, m)]
    bx += [(w0 + 1, 3, w1 - 1, 7, 3, 5, [cm, cm, cm, cm, cm])]
    n = seats; span = (w1 - w0 - 2)
    if n == 2: bx += [(w0 + 1, 2, w0 + 1 + span // 2, 3, 3, 9, cm), (w0 + 1 + span // 2, 2, w1 - 1, 3, 3, 9, cm)]
    else: bx += [(w0 + 1, 2, w1 - 1, 3, 3, 9, cm)]
    bx += [(w0, 1, w0 + 1, 2, 0, 1, S('dark')), (w1 - 1, 1, w1, 2, 0, 1, S('dark')), (w0, 6, w0 + 1, 7, 0, 1, S('dark')), (w1 - 1, 6, w1, 7, 0, 1, S('dark'))]
    return bx
redo('LOVESEAT', sofa('blue', 'blue', 'navy'))
redo('ARMCHAIR', [(1, 2, 7, 7, 1, 3, S('dark')), (1, 1, 7, 2, 3, 9, S('leather')), (0, 2, 1, 7, 1, 6, S('leather')), (7, 2, 8, 7, 1, 6, S('leather')), (0, 2, 1, 7, 6, 7, S('wood')), (7, 2, 8, 7, 6, 7, S('wood')),
                  (2, 3, 6, 7, 3, 4, 'm_cu_orange'), (2, 2, 6, 3, 3, 9, 'm_cu_orange'), (1, 2, 2, 3, 0, 1, S('dark')), (6, 2, 7, 3, 0, 1, S('dark')), (1, 6, 2, 7, 0, 1, S('dark')), (6, 6, 7, 7, 0, 1, S('dark'))])
redo('RECLINER', [(1, 2, 7, 7, 1, 3, S('dark')), (1, 1, 7, 2, 3, 10, S('red')), (1, 2, 7, 3, 3, 9, 'm_cu_red'), (0, 2, 1, 7, 1, 6, S('dark')), (7, 2, 8, 7, 1, 6, S('dark')), (0, 2, 1, 7, 6, 7, S('red')), (7, 2, 8, 7, 6, 7, S('red')),
                  (2, 3, 6, 6, 3, 4, 'm_cu_red'), (2, 6, 6, 7, 1, 3, 'm_cu_red'), (0, 2, 1, 3, 0, 1, S('black')), (7, 2, 8, 3, 0, 1, S('black'))])
redo('BAR STOOL', [(2, 2, 3, 3, 0, 6, S('silver')), (5, 2, 6, 3, 0, 6, S('silver')), (2, 5, 3, 6, 0, 6, S('silver')), (5, 5, 6, 6, 0, 6, S('silver')), (2, 2, 6, 3, 2, 3, S('silver')), (2, 5, 6, 6, 2, 3, S('silver')),
                   (2, 2, 6, 6, 6, 8, 'm_cu_red')])
redo('COFFEE TABLE', [(1, 2, 7, 6, 3, 4, [W_TOP, W_TOP, W_TOP, W_TOP, W_TOP]), (1, 2, 7, 6, 2, 3, S('dark')), (2, 3, 6, 5, 1, 2, S('wood')), (1, 2, 2, 3, 0, 2, S('dark')), (6, 2, 7, 3, 0, 2, S('dark')), (1, 5, 2, 6, 0, 2, S('dark')), (6, 5, 7, 6, 0, 2, S('dark'))])
redo('NIGHTSTAND', [(1, 2, 7, 7, 1, 6, [DR_NS, S('wood'), S('wood'), S('wood'), S('wood')]), (1, 2, 7, 7, 6, 7, [W_TOP] * 5), (1, 2, 2, 3, 0, 1, S('dark')), (6, 2, 7, 3, 0, 1, S('dark')), (1, 6, 2, 7, 0, 1, S('dark')), (6, 6, 7, 7, 0, 1, S('dark')),
                    (3, 3, 5, 5, 7, 8, S('gold')), (3, 3, 5, 5, 8, 9, S('gold')), (2, 2, 6, 6, 9, 11, S('cream'))])
redo('WARDROBE', [(0, 2, 8, 7, 1, 19, [D_WARD, S('dark'), S('dark'), S('dark'), S('dark')]), (0, 2, 8, 7, 0, 1, S('black')), (0, 2, 8, 7, 19, 20, S('wood')), (0, 2, 1, 7, 18, 19, S('wood')), (7, 2, 8, 7, 18, 19, S('wood'))])
redo('FILING CABINET', [(2, 2, 6, 7, 0, 12, [DR_FILE, S('grey'), S('grey'), S('grey'), S('silver')]), (2, 2, 6, 7, 12, 13, S('silver'))])
redo('STORAGE CHEST', [(1, 2, 7, 6, 0, 4, S('wood')), (1, 2, 7, 6, 4, 6, S('dark')), (2, 2, 3, 6, 0, 6, S('char')), (5, 2, 6, 6, 0, 6, S('char')), (3, 6, 5, 7, 3, 5, S('gold')), (1, 2, 7, 3, 4, 5, S('wood'))])
redo('KITCHEN ISLAND', [(0, 1, 8, 7, 1, 7, [D_ISL, S('oak'), S('oak'), S('oak'), S('oak')]), (0, 1, 8, 7, 0, 1, S('dark')), (0, 1, 8, 7, 7, 8, [MARB] * 5)])
redo('WORKBENCH', [(0, 2, 8, 6, 5, 6, [BENCHTOP] * 5), (0, 2, 8, 3, 4, 5, S('dark')), (0, 2, 1, 6, 0, 5, S('dark')), (4, 2, 8, 6, 1, 5, [DR_BENCH, S('dark'), S('dark'), S('dark'), S('dark')]), (4, 2, 8, 6, 0, 1, S('char')),
                   (0, 2, 2, 4, 6, 8, S('red')), (0, 3, 1, 4, 8, 9, S('silver')), (3, 3, 4, 5, 6, 7, S('grey'))])
redo('DISHWASHER', [(1, 1, 7, 7, 0, 7, [DWF, S('silver'), S('silver'), S('silver'), S('grey')]), (1, 1, 7, 7, 0, 1, S('char'))])
redo('WASHING MACHINE', [(1, 1, 7, 7, 1, 12, [WMF, S('white'), S('white'), S('white'), S('white')]), (1, 1, 2, 2, 0, 1, S('grey')), (6, 1, 7, 2, 0, 1, S('grey')), (1, 6, 2, 7, 0, 1, S('grey')), (6, 6, 7, 7, 0, 1, S('grey')), (1, 1, 7, 7, 12, 13, S('silver'))])
redo('SINK', [(1, 2, 7, 7, 1, 6, [D_OAK, S('oak'), S('oak'), S('oak'), S('oak')]), (1, 2, 7, 7, 0, 1, S('dark')), (1, 2, 7, 7, 6, 7, [MARB] * 5), (2, 3, 6, 6, 6, 7, S('white')),
              (3, 2, 5, 3, 7, 10, S('silver')), (3, 3, 5, 4, 9, 10, S('silver')), (2, 2, 3, 3, 7, 8, S('blue')), (5, 2, 6, 3, 7, 8, S('red'))])
redo('STOVE', [(1, 1, 7, 7, 0, 6, [STOVEF, S('white'), S('white'), S('white'), S('white')]), (1, 1, 7, 7, 6, 7, [S('char'), S('char'), S('char'), S('char'), BURN]), (1, 1, 7, 2, 7, 10, S('silver')), (2, 1, 3, 2, 10, 11, S('red')), (5, 1, 6, 2, 10, 11, S('red'))])
redo('MICROWAVE', [(0, 1, 8, 7, 0, 6, [D_OAK, S('oak'), S('oak'), S('oak'), MARB]), (2, 2, 6, 6, 6, 9, [MWF, S('silver'), S('silver'), S('silver'), S('silver')]), (2, 2, 6, 3, 9, 10, S('char'))])
redo('BATHTUB', [(0, 1, 8, 2, 1, 5, S('white')), (0, 6, 8, 7, 1, 5, S('white')), (0, 2, 1, 6, 1, 5, S('white')), (7, 2, 8, 6, 1, 5, S('white')), (1, 2, 7, 6, 1, 2, S('white')), (1, 2, 7, 6, 2, 4, BATHW),
                 (7, 3, 8, 5, 5, 7, S('silver')), (7, 3, 8, 4, 7, 8, S('blue')), (7, 4, 8, 5, 7, 8, S('red')), (0, 1, 1, 2, 0, 1, S('gold')), (7, 1, 8, 2, 0, 1, S('gold')), (0, 6, 1, 7, 0, 1, S('gold')), (7, 6, 8, 7, 0, 1, S('gold'))])
redo('PIANO', [(0, 2, 8, 6, 1, 8, S('black')), (0, 2, 8, 4, 8, 10, S('black')), (0, 2, 8, 3, 10, 11, S('char')), (1, 6, 7, 7, 5, 6, [KEYS2, S('black'), S('black'), S('black'), KEYS2]), (1, 6, 7, 7, 3, 5, S('black')),
               (1, 3, 7, 4, 8, 12, S('black')), (0, 2, 1, 3, 0, 2, S('black')), (7, 2, 8, 3, 0, 2, S('black')), (3, 6, 5, 7, 0, 3, S('black'))])
redo('COMPUTER', [(0, 2, 8, 7, 4, 5, [W_TOP] * 5), (0, 2, 1, 7, 0, 4, S('dark')), (7, 2, 8, 7, 0, 4, S('dark')), (0, 2, 8, 3, 0, 4, S('dark')), (2, 3, 6, 4, 5, 10, [SCREEN, S('black'), S('black'), S('black'), S('black')]), (3, 3, 5, 4, 5, 6, S('char')),
                  (2, 5, 6, 6, 5, 6, [KEYS2] * 5), (6, 3, 7, 5, 5, 9, S('grey')), (6, 3, 7, 4, 7, 8, S('green'))])
redo('FOUR POSTER BED', [(0, 0, 8, 8, 1, 3, S('wood')), (1, 1, 7, 7, 3, 5, 'm_cu_cream'), (1, 3, 7, 7, 5, 6, 'm_cu_pink'), (1, 1, 7, 3, 5, 7, 'm_cu_cream'), (0, 0, 8, 1, 3, 12, S('wood')), (0, 0, 1, 1, 3, 16, S('dark')), (7, 0, 8, 1, 3, 16, S('dark')),
                         (0, 7, 1, 8, 3, 16, S('dark')), (7, 7, 8, 8, 3, 16, S('dark')), (0, 0, 1, 1, 0, 3, S('dark')), (7, 0, 8, 1, 0, 3, S('dark')), (0, 7, 1, 8, 0, 3, S('dark')), (7, 7, 8, 8, 0, 3, S('dark'))])


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
        chs = ('(char)0x%02X' % ch) if isinstance(ch, int) else "'%s'" % ch.replace("'", "\\'")
        bvn, nvv = '0', 0
        if flags == 'F_ALONG':
            bv = []
            for (a0, b0, a1, b1, z0, z1, m) in boxes:
                ms = m if isinstance(m, list) else [m] * 5
                bv.append('{%d,%d,%d,%d,%d,%d,{%s}}' % (b0, a0, b1, a1, z0, z1, ','.join('&' + x for x in ms)))
            out.append('static const IBox bxMoreV%d[%d]={%s};\n' % (i, len(bv), ','.join(bv))); bvn, nvv = 'bxMoreV%d' % i, len(bv)
        defs.append("    { %s, \"%s\", bxMore%d, %d, %s, %d, %s, %d },   \\\n" % (chs, name, i, len(bx), bvn, nvv, flags, cost))
    out.append('#define ITEMS_MORE \\\n' + ''.join(defs) + '\n')
    p = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'src', 'items_more.inc')
    open(p, 'w').write(''.join(out))
    print('%d new objects, %d materials -> %s' % (len(ITEMS), len(MATS), p))
emit()
