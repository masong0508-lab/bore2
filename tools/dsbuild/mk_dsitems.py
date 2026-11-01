#!/usr/bin/env python3
"""mk_dsitems.py: the PS1's buy catalogue for the DS port (D35). Writes ports/dsi/source/ds_items.h.

Reads the PS1 items (src/items.c, items_data.inc and the packs) as the PS1 builds them: every item is a few boxes, each face painted with a
little pixel material. The DS draws flat-coloured lit boxes, so each box keeps two colours (its sides and its top: the average of the
material's pixels), the PS1's life-size table is applied, and the boxes are written as data the DS draws with its own box routine.

  ITEMS[]  every DS item. Ids 0-51 are the 52 the DS already had (their hand-made models; a lot's objects are saved by id, so old saves
           still load). They take the PS1's name, price and tab where the PS1 has the same thing. Then the PS1's other buyable items.
  IBOX[]   the converted boxes (a, b across the tile 0..8, 16 for a two-tile thing; z up in the PS1's 24-unit steps, a Bore is 16).
  TABN[]   the PS1's ten buy tabs (main.c catSet), TABS[] their short names for the DS chips.
Run from anywhere: python3 tools/dsbuild/mk_dsitems.py"""
import os, re, sys

R = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
SRC = os.path.join(R, 'src')
OUT = os.path.join(R, 'ports', 'dsi', 'source', 'ds_items.h')
IDS = os.path.join(R, 'tools', 'dsbuild', 'ds_item_ids.txt')   # (the ids a save keeps: PS1 letter -> DS id, only ever added to)
PACKS = ['items_data', 'items_more', 'items_gotchi', 'items_party', 'items_band', 'items_space', 'items_farm', 'items_home', 'items_bed']

def rd(p): return open(p, encoding='latin-1').read()
items_c = rd(os.path.join(SRC, 'items.c'))
text = items_c + ''.join('\n' + rd(os.path.join(SRC, p + '.inc')) for p in PACKS)
text += ''.join('\n' + rd(os.path.join(SRC, p)) for p in ('skateart.h', 'simart.h', 'homeart.h', 'decorart.h'))   # (the GBA's art, included by items_data.inc)
items_h = rd(os.path.join(SRC, 'items.h'))
main_c = rd(os.path.join(SRC, 'main.c'))

# ---------------------------------------------------------------- C literals
def unesc(s):
    out, i = [], 0
    while i < len(s):
        if s[i] == '\\':
            c = s[i + 1]
            if c == 'x':
                j = i + 2; h = ''
                while j < len(s) and s[j] in '0123456789abcdefABCDEF' and len(h) < 2: h += s[j]; j += 1
                out.append(int(h, 16)); i = j; continue
            if c in '01234567':
                j = i + 1; o = ''
                while j < len(s) and s[j] in '01234567' and len(o) < 3: o += s[j]; j += 1
                out.append(int(o, 8)); i = j; continue
            out.append(ord({'n': '\n', 't': '\t'}.get(c, c))); i += 2; continue
        out.append(ord(s[i])); i += 1
    return out
chdef = {m.group(1): m.group(2) for m in re.finditer(r'#define\s+(CH_\w+)\s+(\(\(char\)0x[0-9A-Fa-f]+\))', items_h)}
def chval(tok):
    tok = tok.strip()
    tok = chdef.get(tok, tok)
    tok = tok.replace('(char)', '').strip('() ')
    if tok.startswith("'"): return unesc(tok[1:-1])[0]
    return int(tok, 0)

# ---------------------------------------------------------------- palettes, row macros, materials, boxes
pals = {}
for m in re.finditer(r'static\s+const\s+u16\s+(\w+)\s*\[\s*\d*\s*\]\s*=\s*\{(.*?)\}\s*;', text, re.S):
    cols = [tuple(int(v) for v in c) for c in re.findall(r'RGB\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)', m.group(2))]
    if cols: pals[m.group(1)] = cols
rowmac = {}
for m in re.finditer(r'^#define\s+(\w+)\s+("[^\n]*)$', text, re.M):
    rowmac[m.group(1)] = re.findall(r'"([^"]*)"', m.group(2))
def split_args(s):
    out, cur, q = [], '', False
    for ch in s:
        if ch == '"': q = not q
        if ch == ',' and not q: out.append(cur.strip()); cur = ''
        else: cur += ch
    if cur.strip(): out.append(cur.strip())
    return out
mats = {}
for m in re.finditer(r'^\s*MAT\((.*)\)\s*(?://.*)?$', text, re.M):
    a = split_args(m.group(1))
    if len(a) < 5 or a[0] == 'nm': continue
    rows = []
    for t in a[4:]:
        if t.startswith('"'): rows.append(t.strip('"'))
        else: rows += rowmac.get(t, [])
    mats[a[0]] = (a[1], rows)
def mat_avg(name, cache={}):
    if name in cache: return cache[name]
    if name not in mats: cache[name] = None; return None
    pn, rows = mats[name]; pal = pals.get(pn)
    if not pal: cache[name] = None; return None
    n = r = g = b = 0
    for row in rows:
        for ch in row:
            if ch == '.': continue
            k = ord(ch) - 97
            if 0 <= k < len(pal): c = pal[k]; r += c[0]; g += c[1]; b += c[2]; n += 1
    cache[name] = (round(r / n), round(g / n), round(b / n)) if n else None
    return cache[name]
boxes = {}
for m in re.finditer(r'static\s+const\s+IBox\s+(\w+)\s*\[\s*\d*\s*\]\s*=\s*\{(.*?)\}\s*;', text, re.S):
    bl = []
    for b in re.finditer(r'\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*\{([^}]*)\}\s*\}', m.group(2)):
        faces = [f.strip().lstrip('&') for f in b.group(7).split(',')]
        bl.append([int(b.group(k)) for k in range(1, 7)] + [faces])
    boxes[m.group(1)] = bl

# ---------------------------------------------------------------- the PS1 item table and its tabs
defs = []
for m in re.finditer(r"\{\s*('(?:\\.|[^'])+'|CH_\w+|\(char\)0x[0-9A-Fa-f]+)\s*,\s*\"([^\"]+)\"\s*,\s*(\w+)\s*,\s*(\d+)\s*,\s*(\w+)\s*,\s*(\d+)\s*,\s*([\w|]+)\s*,\s*(\d+)\s*\}", text):
    c = chval(m.group(1))
    if any(d['ch'] == c for d in defs): continue   # (the PS1 keeps the first of a letter)
    defs.append(dict(ch=c, name=m.group(2), box=m.group(3), n=int(m.group(4)), flags=m.group(7), cost=int(m.group(8))))
cs = main_c[main_c.index('catSet[NCAT - 1]'):]; cs = cs[cs.index('{') + 1:cs.index('};')]
catset = []
for line in cs.split('\n'):
    parts = re.findall(r'"((?:\\.|[^"\\])*)"', line)
    if parts: catset.append(set(unesc(''.join(parts))))
assert len(catset) == 9, catset
TABN = ["Seats, Beds", "Tables, Storage", "Kitchen, Bath", "Fun and Tech", "Decor, Plants", "Garden, Outdoor", "Skate", "Boreagotchi", "Party", "Special"]
TABS = ["Seats", "Tables", "Kitchen", "Fun", "Decor", "Garden", "Skate", "Gotchi", "Party", "Special"]
def ps1_tab(c):
    if 0x81 < c < 0x90: return -1          # (the cake, the gravestone, the island's own: never bought)
    for t, s in enumerate(catset):
        if c in s: return t
    return 9
life = {}
ls = items_c[items_c.index('lifeSize[] = {'):]; ls = ls[:ls.index('};')]
for m in re.finditer(r"\{\s*('(?:\\.|[^'])+'|\(char\)0x[0-9A-Fa-f]+)\s*,\s*(\d+)\s*\}", ls): life[chval(m.group(1))] = int(m.group(2))

# ---------------------------------------------------------------- what a Bore does with it (sim.c action_for and act_hint), in DS needs
A = {}
for k, v in {'F': 'EAT', 's': 'EAT', 'T': 'WC', '&': 'WC', 'S': 'SLEEP', 'z': 'SLEEP', 'H': 'SHOWER', '0': 'SHOWER',
             'C': 'SIT', 'U': 'SIT', 'h': 'SIT', 'N': 'SIT', '%': 'SIT', '~': 'SIT', '@': 'TRAVEL', '+': 'TRAVEL', '|': 'STAIRS',
             '!': 'FUN', '$': 'FUN', '(': 'FUN', ')': 'FUN', '>': 'FUN', '[': 'FUN', 'j': 'FUN', 'n': 'FUN', '{': 'FUN', '}': 'FUN', '`': 'FUN',
             ',': 'SHOWER', '-': 'SHOWER', '/': 'EAT', ':': 'EAT', ';': 'SIT', '<': 'SLEEP', '?': 'READ', ']': 'RUN', '_': 'RUN',
             'v': 'FUN', 'R': 'FUN', 'A': 'FUN', 'Q': 'FUN', 'q': 'FUN', 'G': 'FUN', 'V': 'FUN', 'b': 'READ', 'd': 'READ', 'c': 'COFFEE',
             'm': 'RUN', 'I': 'PHONE', 'x': 'WORK'}.items(): A[ord(k)] = v
for c in (0xEB, 0xEC, 0xED, 0xF6, 0x93, 0x94): A[c] = 'SLEEP'
for c in (0x90, 0x91, 0x92, 0xA8, 0xBC): A[c] = 'SIT'
for c in (0x9A, 0x9E, 0xC8): A[c] = 'EAT'
for c in (0x9F, 0xC9): A[c] = 'COFFEE'
A[0x83] = 'READ'; A[ord('@')] = 'TRAVEL'; A[ord('+')] = 'TRAVEL'
for c in (0xA9, 0xB8): A[c] = 'RUN'
for c in (0xC1, 0xC2, 0xC7, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xB9, 0xBA): A[c] = 'FUN'
HINT = {0xC7: "DANCE UNDER THE LIGHTS", 0xC8: "GRAB A BITE", 0xC9: "SIP THE PUNCH", 0xCA: "DANCE TO THE BASS", 0xCB: "POP A BALLOON", 0xCC: "SPIN SOME TRACKS",
        0xCD: "SING KARAOKE", 0xCE: "POSE FOR PHOTOS", 0xC1: "CUDDLE THE PLUSH", 0xC2: "PLAY WITH THE BALL", 0xA0: "PLAY DRUMS", 0xA1: "SHOOT POOL",
        0xA2: "STARGAZE", 0xA3: "PLAY SYNTH", 0xA4: "PLAY FOOSBALL", 0xA5: "THROW DARTS", 0xA6: "PULL THE LEVER", 0xA7: "SPIN A RECORD", 0xA8: "RELAX",
        0xA9: "PUNCH THE BAG", 0xB8: "BOUNCE", 0xB9: "SWING", 0xBA: "PLAY IN SAND", 0xBC: "WARM UP", 0x9A: "SNACK", 0x9E: "GRILL", 0x9F: "A DRINK"}
for k, v in {'!': "PLAY ARCADE", '$': "PLAY PIANO", '(': "PLAY GUITAR", ')': "PAINT", '>': "SOAK", '[': "USE COMPUTER", 'j': "PICK A SONG", 'n': "PLAY",
             '{': "PLAY HOUSE", '}': "MAKE A WISH", '`': "PRACTICE SPEECH", ',': "TAKE A BATH", '-': "WASH HANDS", '/': "COOK", ':': "MICROWAVE", ';': "SIT",
             '<': "SLEEP", '?': "PLAY CHESS", '~': "SUNBATHE", ']': "LIFT WEIGHTS", '_': "SHOOT HOOPS"}.items(): HINT[ord(k)] = v
HINT[0x83] = "REMEMBER"; HINT[ord('@')] = "SAIL"; HINT[ord('+')] = "TAKE THE BUS"
def hint(c, act):
    if c in HINT: return HINT[c]
    return {'EAT': "COOK AND EAT" if c == ord('s') else "EAT", 'WC': "USE OUTHOUSE" if c == ord('&') else "USE TOILET", 'SLEEP': "SLEEP", 'SHOWER': "SHOWER",
            'SIT': "SIT" if c == ord('h') else "LOUNGE", 'FUN': {ord('v'): "WATCH TV", ord('R'): "TUNE RADIO", ord('A'): "CRANK IT", ord('Q'): "JACK IN", ord('q'): "FEED THE FISH"}.get(c, "CHILL"),
            'READ': "READ", 'COFFEE': "A CUP", 'RUN': "A RUN", 'PHONE': "CALL", 'WORK': "WORK"}.get(act, "")
# the DS's needs: 0 hunger, 1 fun, 2 energy, 3 social, 4 bladder, 5 hygiene; (need, amount, steps) as the DS's own items use them
DSACT = {'TRAVEL': (1, 5, 40), 'EAT': (0, 55, 100), 'WC': (4, 95, 80), 'SLEEP': (2, 90, 240), 'SHOWER': (5, 90, 130), 'SIT': (2, 25, 90), 'FUN': (1, 60, 150), 'READ': (1, 45, 150),
         'COFFEE': (2, 30, 45), 'RUN': (1, 40, 120), 'PHONE': (3, 40, 100)}
SMALL = {'A', 'AN', 'THE', 'OF', 'IN', 'TO', 'AND', 'ON', 'FOR', 'AT', 'BY', 'WITH'}
KEEP = {'TV', 'BBQ', 'DJ'}
def title(s):
    out = []
    for i, w in enumerate(s.split(' ')):
        if w in KEEP: out.append(w)
        elif i and w in SMALL: out.append(w.lower())
        else: out.append(w[:1] + w[1:].lower())
    return ' '.join(out)
def mask_for(c, act):   # which life stages may use it (bit 0 baby .. bit 5 elder): the DS's rule, the PS1 has none
    if c in (0x81, 0xEE, 0xEF, 0xF4, 0xF5): return 3          # crib, bassinet, cradle, playpen, changing table: babies and toddlers
    if c in (0xED, 0xF2): return 6                              # toddler bed, rocking horse
    if c in (0xF1, ord('{'), ord('n'), 0xBA): return 14         # toy chest, dollhouse, toy box, sandbox
    if c in (0xA6, 0x92): return 48                             # slot machine, bar stool: grown-ups
    if act == 'RUN' or c in (0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9): return 56
    if act in ('FUN', 'READ'): return 62
    return 62

# ---------------------------------------------------------------- the DS's own 52 (ids 0-51): what the PS1 calls them
# (name, price, h, colour, kind, mask, (label, need, amt, dur), tab, the PS1 letter or None)
OLD = [
 ("Chair", 80, 2, (24, 16, 8), 0, 62, ("Sit", 2, 25, 90), 0, 'h'), ("Armchair", 450, 2, (10, 14, 24), 0, 62, ("Relax", 2, 35, 120), 0, ';'),
 ("Sofa", 900, 2, (24, 8, 8), 0, 62, ("Nap", 2, 60, 150), 0, 'C'), ("Bench", 150, 1, (20, 14, 8), 0, 62, ("Sit", 2, 20, 90), 0, 'N'),
 ("Dining", 300, 2, (22, 15, 8), 0, 62, ("Sit", 1, 10, 60), 1, 't'), ("Desk", 220, 2, (18, 18, 20), 0, 60, ("Look", 1, 5, 40), 1, 'd'),
 ("Coffee", 120, 1, (16, 10, 6), 0, 62, ("Look", 1, 5, 40), 1, 0x95), ("Counter", 260, 3, (26, 26, 24), 0, 56, ("Cook", 0, 35, 120), 1, 'y'),
 ("TV", 600, 3, (3, 3, 4), 0, 62, ("Watch TV", 1, 60, 150), 3, 'v'), ("Computer", 900, 2, (20, 20, 22), 0, 60, ("Play", 1, 50, 150), 3, '['),
 ("Stereo", 350, 2, (6, 6, 8), 0, 62, ("Listen", 1, 35, 120), 3, 'A'), ("Fridge", 700, 5, (28, 28, 30), 0, 62, ("Eat", 0, 70, 120), 2, 'F'),
 ("Lamp", 60, 4, (31, 28, 14), 0, 63, ("Look", 1, 3, 30), 4, 'l'), ("Rug", 100, 0, (22, 8, 12), 1, 63, ("Look", 1, 3, 30), 4, 'r'),
 ("Plant", 40, 3, (5, 18, 6), 2, 63, ("Water", 1, 10, 60), 4, 'p'), ("Statue", 500, 4, (24, 24, 22), 0, 63, ("Admire", 1, 15, 60), 4, 0xAB),
 ("Bed", 800, 2, (20, 24, 31), 0, 62, ("Sleep", 2, 90, 240), 0, 'S'), ("Bathtub", 650, 2, (28, 30, 31), 0, 62, ("Bathe", 5, 90, 180), 2, ','),
 ("Toilet", 250, 2, (30, 30, 30), 0, 62, ("Use", 4, 95, 100), 2, 'T'), ("Sink", 200, 3, (26, 28, 30), 0, 62, ("Wash", 5, 30, 80), 2, '-'),
 ("Crib", 400, 3, (28, 24, 16), 0, 3, ("Sleep", 2, 80, 200), 0, 0x81), ("Toy Box", 150, 2, (31, 20, 6), 0, 7, ("Play", 1, 60, 120), 3, 'n'),
 ("Books", 250, 4, (18, 12, 6), 0, 60, ("Read", 1, 40, 120), 1, None), ("Rocker", 300, 2, (22, 14, 10), 0, 48, ("Rock", 2, 40, 150), 0, None),
 ("Fence", 40, 2, (28, 26, 20), 0, 63, ("Look", 1, 3, 30), 5, 0xB4), ("Lamp Post", 120, 6, (6, 6, 8), 0, 63, ("Look", 1, 3, 30), 5, '"'),
 ("Hedge", 60, 2, (6, 18, 6), 0, 63, ("Look", 1, 3, 30), 5, 0xB3), ("Flowers", 30, 0, (28, 10, 14), 1, 63, ("Smell", 1, 10, 60), 5, 0xB2),
 ("Stove", 450, 3, (26, 26, 26), 0, 60, ("Cook", 0, 45, 120), 2, '/'), ("Microwave", 150, 2, (26, 26, 28), 0, 60, ("Snack", 0, 55, 60), 2, ':'),
 ("Kitchen Sink", 250, 3, (28, 28, 30), 0, 62, ("Wash", 5, 25, 60), 2, None), ("Bar", 600, 3, (18, 11, 6), 0, 48, ("Drink", 0, 40, 80), 2, None),
 ("Shower", 500, 6, (24, 28, 31), 0, 62, ("Shower", 5, 95, 120), 2, 'H'), ("Mirror", 120, 5, (22, 28, 31), 0, 62, ("Primp", 1, 6, 40), 2, '`'),
 ("Hot Tub", 1400, 2, (18, 10, 6), 0, 56, ("Soak", 5, 70, 150), 2, '>'), ("Towels", 40, 3, (8, 22, 28), 0, 62, ("Look", 1, 3, 30), 2, None),
 ("Piano", 1800, 3, (4, 4, 6), 0, 56, ("Play", 1, 70, 160), 3, '$'), ("Pool Table", 1200, 2, (4, 18, 8), 0, 60, ("Play", 1, 60, 140), 3, 0xA1),
 ("Arcade", 900, 6, (8, 8, 24), 0, 60, ("Play", 1, 80, 150), 3, '!'), ("Console", 400, 2, (6, 6, 9), 0, 60, ("Play", 1, 75, 150), 3, None),
 ("Bookcase", 350, 6, (16, 10, 5), 0, 62, ("Read", 1, 45, 120), 1, 'b'), ("Telescope", 700, 5, (26, 26, 28), 0, 62, ("Stargaze", 1, 50, 120), 3, 0xA2),
 ("Easel", 200, 4, (22, 16, 8), 0, 60, ("Paint", 1, 60, 140), 3, ')'), ("Treadmill", 800, 2, (10, 10, 12), 0, 56, ("Run", 1, 30, 100), 3, 'm'),
 ("Swimming Pool", 2500, 1, (6, 18, 30), 0, 62, ("Swim", 1, 70, 140), 5, None), ("BBQ", 450, 3, (6, 6, 8), 0, 56, ("Grill", 0, 60, 120), 5, 0x9E),
 ("Swing", 180, 5, (24, 8, 8), 0, 62, ("Swing", 1, 50, 100), 5, 0xB9), ("Fountain", 900, 3, (22, 22, 22), 0, 63, ("Admire", 1, 15, 60), 4, '}'),
 ("Puppy", 150, 1, (20, 13, 6), 0, 63, ("Look", 1, 3, 30), 5, None), ("Dog Bed", 120, 1, (24, 10, 10), 0, 63, ("Nap", 2, 40, 120), 5, None),
 ("Dog Bowl", 40, 1, (26, 26, 28), 0, 63, ("Look", 1, 3, 30), 5, None), ("Fish Tank", 300, 3, (8, 22, 30), 0, 63, ("Watch", 1, 35, 100), 4, 'q'),
]
assert len(OLD) == 52
def ordc(c): return c if c is None or isinstance(c, int) else ord(c)
OLD = [o[:8] + (ordc(o[8]),) for o in OLD]
taken = {o[8] for o in OLD if o[8] is not None}
LEAVE = {ord('D'), ord('^')}   # the door (Build makes doors); STAIRS UP is the PS1's old picture of stairs ('|' STAIRS is the working one)
STAIRS = [(0, 7 - i, 8, 8 - i, 0, 3 * (i + 1), [None] * 5) for i in range(8)]   # (D36) the DS's flight: eight steps from the open front up a whole storey (24 PS1 steps of 7 = the DS wall)

def c15(c): return 'C15(%d, %d, %d)' % c
items, ibox = [], []
for o in OLD:
    name, price, h, col, kind, mask, act, tab, ch = o
    d = next((d for d in defs if d['ch'] == ch), None) if ch is not None else None
    lab = act[0]
    if d:
        name, price = title(d['name']), d['cost']
        a = A.get(ch)
        if a and hint(ch, a): lab = title(hint(ch, a))
    items.append(dict(name=name, price=price, h=h, col=col, kind=kind, tab=tab, fw=1, fl=1, mask=mask, box0=0, nbox=0, lab=lab, need=act[1], amt=act[2], dur=act[3], key='ds:%d' % len(items),
                      src='the DS model' + (' = PS1 %r' % chr(ch) if ch and ch < 128 else ' = PS1 0x%02X' % ch if ch else '')))
missing = set()
for d in defs:
    c, t = d['ch'], ps1_tab(d['ch'])
    hidden = (t < 0 and 0x83 <= c <= 0x8D) or c in (ord('@'), ord('+'), ord('a'), ord('k'))   # (D37) the gravestone, the island's own things, the boat, the bus stop and the flags: never sold, placed by the game
    if (t < 0 and not hidden) or (d['cost'] <= 0 and not hidden) or c in taken or c in LEAVE: continue
    if hidden: t = 255
    if c in (ord('@'), ord('+')): d = dict(d, cost=0)
    bl = STAIRS if c == ord('|') else boxes.get(d['box'])
    if not bl: print('no boxes for', d['name'], d['box'], file=sys.stderr); continue
    bl = bl if c == ord('|') else bl[:d['n']]
    top = max(1, max(b[5] for b in bl))
    if c in life:   # the PS1's life size: the art stretched or squashed to a real height
        px = life[c]; nb = []
        for b in bl:
            z0 = (b[4] * px + top // 2) // top; z1 = (b[5] * px + top // 2) // top
            if z1 <= z0: z1 = z0 + 1
            nb.append(b[:4] + [z0, z1, b[6]])
        bl = nb
    act = A.get(c, 'NONE')
    lab = "Go Up" if c == ord('|') else title(hint(c, act)) if act in DSACT else ("Admire" if t == 4 else "Look")
    need, amt, dur = DSACT.get(act, (1, 5, 40))
    box0 = len(ibox); big, bigv = None, -1
    for b in bl:
        if b[2] <= b[0] or b[3] <= b[1] or b[5] <= b[4]: continue
        f = b[6] + [None] * 5
        side = next((mat_avg(f[k]) for k in (0, 1, 2, 3) if f[k] and f[k] != '0' and mat_avg(f[k])), None)
        topc = mat_avg(f[4]) if f[4] and f[4] != '0' else None
        if c == ord('|'): k8 = 7 - b[1]; side = (22, 16, 10) if k8 & 1 else (24, 18, 11); topc = (27, 21, 14)   # (the flight: warm wood, a lighter tread)
        if side is None and topc is None:
            missing.add(str(f[:5])); side = (18, 18, 18)
        side = side or topc; topc = topc or side
        ibox.append((b[0], b[1], b[2], b[3], b[4], b[5], side, topc))
        v = (b[2] - b[0]) * (b[3] - b[1]) * (b[5] - b[4])
        if v > bigv: bigv, big = v, side
    fl = d['flags']
    items.append(dict(name=title(d['name']), price=d['cost'], h=max(1, round(max(b[5] for b in bl) * 7 / 10)), col=big or (18, 18, 18), kind=1 if 'F_FLAT' in fl else 0, ch=c,
                      tab=t, fw=2 if 'F_W2' in fl else 1, fl=2 if 'F_L2' in fl else 1, mask=mask_for(c, act), box0=box0, nbox=len(ibox) - box0,
                      lab=lab, need=need, amt=amt, dur=dur, key='ps1:%02X' % c, src='PS1 ' + (repr(chr(c)) if c < 128 else '0x%02X' % c)))
if missing: print('materials not found:', sorted(missing)[:8], file=sys.stderr)
reg = {}
if os.path.exists(IDS):
    for line in open(IDS):
        p = line.split()
        if len(p) >= 2 and not line.startswith('#'): reg[p[1]] = int(p[0])
nxt = max(reg.values(), default=-1) + 1
for it in items:
    if it['key'] not in reg: reg[it['key']] = nxt; nxt += 1
byid = {reg[it['key']]: it for it in items}
assert sorted(byid) == list(range(len(byid))), 'an id was dropped: keep every PS1 item the registry knows'
items = [byid[i] for i in range(len(byid))]
open(IDS, 'w').write('# DS item ids (tools/dsbuild/mk_dsitems.py): a saved object is its id. Never renumber or delete a line; new items are added at the end.\n' +
    ''.join('%d %s %s\n' % (reg[it['key']], it['key'], it['name']) for it in items))
assert len(items) < 256 and len(ibox) < 65536

o = ['// GENERATED by tools/dsbuild/mk_dsitems.py from the PS1 items (src/items.c and its packs). Do not edit by hand: change the script and run it.',
     '// %d items (52 the DS had, %d from the PS1), %d boxes. See the script for the units.' % (len(items), len(items) - 52, len(ibox)),
     '#define NITEM %d' % len(items), '#define NTAB 10', '#define NHID 255   // (the tab of the things never sold)', '#define ID_STAIRS %d' % next(i for i, it in enumerate(items) if it['key'] == 'ps1:7C'),
     'typedef struct { u8 a0, b0, a1, b1, z0, z1; u16 side, top; } IBx;',
     'typedef struct { const char *name; u16 price; u8 h; u16 col; u8 kind, tab, fw, fl, mask, nbox; u16 box0; const char *lab; u8 need, amt, dur; } Item;',
     'static const char *const TABN[NTAB] = { %s };' % ', '.join('"%s"' % t for t in TABN),
     'static const char *const TABS[NTAB] = { %s };' % ', '.join('"%s"' % t for t in TABS),
     'static const Item ITEMS[NITEM] = {']
for i, it in enumerate(items):
    o.append('    { "%s", %d, %d, %s, %d, %d, %d, %d, %d, %d, %d, "%s", %d, %d, %d },   // %d %s' % (it['name'], it['price'], it['h'], c15(it['col']), it['kind'], it['tab'], it['fw'], it['fl'],
             it['mask'], it['nbox'], it['box0'], it['lab'], it['need'], it['amt'], it['dur'], i, it['src']))
o.append('};')
ps1id = [0] * 256
for i, it in enumerate(items):
    c = it.get('ch') if it.get('ch') is not None else (OLD[i][8] if i < 52 else None)
    if c is not None and not ps1id[c]: ps1id[c] = i + 1
o.append('static const u8 PS1ID[256] = {   // a PS1 letter -> its DS item + 1 (0: none); the DS lays out the PS1\'s zones with the PS1\'s own letters')
for k in range(0, 256, 32): o.append('    ' + ', '.join(str(v) for v in ps1id[k:k + 32]) + ',')
o.append('};')
o.append('static const IBx IBOX[%d] = {' % len(ibox))
for k in range(0, len(ibox), 3):
    o.append('    ' + ' '.join('{ %d, %d, %d, %d, %d, %d, %s, %s },' % (b[0], b[1], b[2], b[3], b[4], b[5], c15(b[6]), c15(b[7])) for b in ibox[k:k + 3]))
o.append('};')
open(OUT, 'w').write('\n'.join(o) + '\n')
per = [0] * 10
for it in items: per[it['tab'] if it['tab'] < 10 else 9] += 0 if it['tab'] >= 10 else 1
print('%d items, %d boxes -> %s; per tab %s' % (len(items), len(ibox), OUT, per))
