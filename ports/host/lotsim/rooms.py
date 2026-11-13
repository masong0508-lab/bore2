#!/usr/bin/env python3
# rooms.py OUT.png [LOTSIM] [EACH_DIR]: the picture tool. A sheet of twelve homes drawn top-down from the real generators (./lotsim rooms),
# every storey: walls, doors, windows, the floors, and each thing as a labelled tile with a mark on the side it faces
# (a seat's front, a telly's screen, a bed's foot). To judge a room by eye: does it read as a room?
#   LOTSIM    the simulator to ask (default ./lotsim; an older build gives the "before" sheet)
#   EACH_DIR  also save every home as its own picture there
import subprocess, sys, os
from PIL import Image, ImageDraw, ImageFont
HERE = os.path.dirname(os.path.abspath(__file__))
SEED = 1
# (block x, block y, household adults,kids,teens,elders,couple,babies, what it is)
HOMES = [
    (936, 68, '2,2,0,0,1,0', 'HOUSE: a couple, two kids'),
    (930, 69, '1,0,0,0,0,0', 'HOUSE: one adult'),
    (938, 70, '2,0,1,0,1,1', 'HOUSE: a couple, a teen, a baby'),
    (930, 70, '2,1,1,1,1,0', 'MANSION: a couple, a kid, a teen, an elder'),
    (414, 505, '2,0,0,0,1,0', 'MANSION with an upstairs: a couple'),
    (931, 66, '0,0,0,2,1,0', 'COTTAGE: an elder couple'),
    (934, 66, '1,1,0,0,0,0', 'COTTAGE: a parent and a kid'),
    (689, 133, '2,2,0,0,1,0', 'TWO-STOREY: a couple, two kids'),
    (693, 135, '2,0,0,0,1,0', 'TWO-STOREY: a couple'),
    (173, 151, '1,0,0,0,0,1', 'TOWNHOUSES: an adult and a baby (each)'),
    (173, 153, '2,0,1,0,1,0', 'TOWNHOUSES: a couple and a teen (each)'),
    (416, 506, '2,1,0,0,1,0', 'FLATS: a couple and a kid (each)'),
]
T = 30
LAB = {'S': 'BED', '<': 'BUNK', 0x81: 'CRIB', 'C': 'SOFA', ';': 'ARM', 'U': 'BEAN', 'h': 'CH', 't': 'TABL', 'v': 'TV', 'F': 'FRDG', '/': 'STOV', '-': 'SINK', 'y': 'CNTR',
       ':': 'MWAV', 'c': 'COFF', 'T': 'WC', 'H': 'SHWR', ',': 'TUB', 'd': 'DESK', '[': 'PC', 'b': 'BOOK', 'l': 'LAMP', 'p': 'PLNT', 'r': 'rug', 'i': 'DRSR', 'n': 'TOYS',
       '{': 'DOLL', 'o': 'FIRE', '$': 'PIAN', 'R': 'RADI', 'A': 'HIFI', '`': 'MIRR', 'q': 'AQUA', 'j': 'JUKE', '!': 'ARCD', 'm': 'TRED', ']': 'WGHT', 'Q': 'VYBE',
       '(': 'GUIT', ')': 'EASL', 'V': 'LAVA', '?': 'CHSS', '#': 'BOX', '|': 'STAIR', 0x90: 'LOVE', 0x91: 'RECL', 0x95: 'CTBL', 0x96: 'NSTD', 0x97: 'WARD', 0x9A: 'ISLD',
       0x9C: 'DISH', 0x9D: 'WASH', 0xAA: 'CLCK', 0xAC: 'VASE', 0xAE: 'ART', 0xA1: 'POOL', 0xA4: 'FOOS', 0xA5: 'DART', 0x99: 'CHST', 0x98: 'FILE', 0xA7: 'RECP',
       'Z': 'PLTR', 'Y': 'BIN', 'N': 'BNCH', '8': 'SALE', '}': 'FNTN', 0xF0: 'NSTD', 0xF3: 'WARD', 0xF1: 'TOYC', 0xF2: 'HORS', 0xF4: 'PPEN', 0xF5: 'CHNG'}
CAT = {'sleep': ('S<' + chr(0x81), (150, 170, 235)), 'seat': ('C;Uh' + chr(0x90) + chr(0x91), (235, 170, 120)), 'table': ('t?' + chr(0x95) + chr(0x9A), (190, 140, 90)),
       'kitchen': ('F/-y:c' + chr(0x9C), (235, 235, 240)), 'bath': ('TH,`', (150, 215, 230)), 'fun': ('v$RAqj!m]Q()Vn{o' + chr(0xA1) + chr(0xA4) + chr(0xA5) + chr(0xA7) + chr(0xF1) + chr(0xF2), (225, 150, 200)),
       'work': ('d[b' + chr(0x98), (200, 190, 120)), 'store': ('i#' + chr(0x96) + chr(0x97) + chr(0x99) + chr(0xF0) + chr(0xF3), (205, 180, 150)), 'decor': ('lpZ' + chr(0xAA) + chr(0xAC) + chr(0xAE), (140, 200, 130))}
FLOOR = {14: (120, 170, 95), 15: (185, 185, 180), 16: (140, 110, 80), 17: (220, 205, 150), 18: (90, 150, 220), 19: (130, 130, 130), 20: (70, 70, 75), 21: (200, 190, 160), 22: (200, 180, 150),
         0: (196, 160, 110), 1: (176, 136, 92), 2: (214, 190, 150), 3: (160, 120, 90), 4: (205, 170, 125), 5: (222, 226, 230), 6: (198, 214, 222), 11: (228, 220, 200), 13: (214, 204, 228)}
def font(n):
    for p in ('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', '/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf'):
        if os.path.exists(p): return ImageFont.truetype(p, n)
    return ImageFont.load_default()
F8, F12 = font(9), font(13)
def ask(lotsim, bx, by, occ):
    env = dict(os.environ); env['OCC'] = occ
    out = subprocess.run([lotsim, 'rooms', str(SEED), str(bx), str(by)], capture_output=True, text=True, env=env, cwd=HERE).stdout.split('\n')
    levels, cur, audit = [], None, ''
    for ln in out:
        if ln.startswith('LEVEL'): cur = []; levels.append((int(ln.split()[1]), cur))
        elif ln.startswith('AUDIT'): audit = ln
        elif ln.startswith('OCC') or not ln: continue
        elif cur is not None: cur.append([c.split(':') for c in ln.split('|')[:-1]])
    return levels, audit
def catcol(c):
    for k, (s, col) in CAT.items():
        if c in s: return col
    return (215, 215, 215)
def draw_level(grid):
    n = len(grid); im = Image.new('RGB', (n * T, n * T), (255, 255, 255)); d = ImageDraw.Draw(im)
    for y, row in enumerate(grid):
        for x, cell in enumerate(row):
            code, fl, face, kind = int(cell[0]), int(cell[1]), int(cell[2]), int(cell[3]); name = cell[4]; c = chr(code)
            X, Y = x * T, y * T
            d.rectangle([X, Y, X + T - 1, Y + T - 1], fill=FLOOR.get(fl, (200, 200, 200)), outline=tuple(max(0, v - 14) for v in FLOOR.get(fl, (200, 200, 200))))
            if c == ' ': d.rectangle([X, Y, X + T - 1, Y + T - 1], fill=(40, 40, 50)); continue                      # open air / a stairwell
            if c in 'Ww': d.rectangle([X, Y, X + T - 1, Y + T - 1], fill=(60, 52, 48)); continue
            if c in 'Ee': d.rectangle([X, Y, X + T - 1, Y + T - 1], fill=(60, 52, 48)); d.rectangle([X + 3, Y + 3, X + T - 4, Y + T - 4], fill=(150, 215, 255)); continue
            if c == 'D': d.rectangle([X + 2, Y + 2, X + T - 3, Y + T - 3], fill=(170, 95, 40), outline=(90, 50, 20), width=2); d.text((X + 9, Y + 8), 'D', font=F12, fill=(255, 240, 200)); continue
            if c == '.' or not kind: continue
            if kind == 2 and c != '|':                                                                                 # flat things: a rug
                d.rectangle([X + 1, Y + 1, X + T - 2, Y + T - 2], fill=(200, 110, 110), outline=(150, 70, 70)); d.text((X + 6, Y + 9), 'rug', font=F8, fill=(90, 30, 30)); continue
            col = (120, 120, 130) if c == '|' else (60, 130, 60) if c in '36gu' else catcol(c)
            d.rounded_rectangle([X + 2, Y + 2, X + T - 3, Y + T - 3], radius=4, fill=col, outline=(40, 40, 40))
            lab = LAB.get(c, LAB.get(code, name.replace(' ', '')[:4]))
            w = d.textlength(lab, font=F8); d.text((X + (T - w) / 2, Y + 10), lab, font=F8, fill=(20, 20, 20))
            if face >= 0:                                                                                              # the front: a red wedge on the side it faces
                cx, cy = X + T / 2, Y + T / 2; dx, dy = [(0, 1), (1, 0), (0, -1), (-1, 0)][face]
                tip = (cx + dx * (T / 2 - 1), cy + dy * (T / 2 - 1)); px, py = -dy, dx
                b1 = (cx + dx * (T / 2 - 7) + px * 6, cy + dy * (T / 2 - 7) + py * 6); b2 = (cx + dx * (T / 2 - 7) - px * 6, cy + dy * (T / 2 - 7) - py * 6)
                d.polygon([tip, b1, b2], fill=(210, 30, 30))
    return im
def main():
    out = sys.argv[1]; lotsim = sys.argv[2] if len(sys.argv) > 2 else './lotsim'; each = sys.argv[3] if len(sys.argv) > 3 else None
    panels = []
    for i, (bx, by, occ, what) in enumerate(HOMES):
        levels, audit = ask(lotsim, bx, by, occ)
        levels = levels[:3]
        ims = [draw_level(g) for _, g in levels]
        W = sum(im.width for im in ims) + 8 * (len(ims) - 1); H = ims[0].height + 24
        p = Image.new('RGB', (W, H), (255, 255, 255)); d = ImageDraw.Draw(p)
        d.text((4, 4), '%d. %s%s' % (i + 1, what, '   (ground, then upstairs)' if len(ims) > 1 else ''), font=F12, fill=(0, 0, 0))
        x = 0
        for im in ims: p.paste(im, (x, 24)); x += im.width + 8
        panels.append(p)
        if each: os.makedirs(each, exist_ok=True); p.save(os.path.join(each, 'home%02d.png' % (i + 1)))
    rows, cur, cw = [], [], 0                                           # pack the panels in rows up to four lots wide
    MAXW = 15 * T * 4 + 8 * 4
    for p in panels:
        if cur and cw + p.width + 12 > MAXW: rows.append(cur); cur, cw = [], 0
        cur.append(p); cw += p.width + 12
    if cur: rows.append(cur)
    W = max(sum(p.width + 12 for p in r) for r in rows) + 12; H = sum(max(p.height for p in r) + 12 for r in rows) + 12
    sheet = Image.new('RGB', (W, H), (255, 255, 255)); y = 12
    for r in rows:
        x = 12
        for p in r: sheet.paste(p, (x, y)); x += p.width + 12
        y += max(p.height for p in r) + 12
    sheet.save(out); print(out, sheet.size)
main()
