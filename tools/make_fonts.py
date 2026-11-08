#!/usr/bin/env python3
# make_fonts.py [out.bin] [preview.png]: builds assets/FONTS.BIN, every text style of BORE 2 as 4-bit glyph sheets (see docs/FONTS.md).
#
# Fonts (all SIL OFL or the Bitstream Vera licence; the files are looked up in the usual system places, see FONTDIRS):
#   BODY  Instrument Sans Bold   the clean small body type, cap 7 (menus, HUD, lists)
#   READ  DejaVu Sans Bold       the readable setting: wider, open, very distinct letters, cap 7
#   HEAD  Outfit Bold, rounded   panel titles and buttons, cap 9
#   NUM   Outfit Bold, rounded   tabular digits for money and the clock, cap 9 (other letters fall back to BODY)
#   FUN   Erica One, rounded     the chunky playful title type, cap 11
#   BIG   Erica One              the loading screen's and the pet games' big letters and digits, cap 20
# The sheets are drawn at 8x and shrunk with area averaging (soft, like the old interface type), 16 grey levels.
import math, os, struct, sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'assets', 'FONTS.BIN')
PREV = sys.argv[2] if len(sys.argv) > 2 else None
CF = '/mnt/skills/examples/canvas-design/canvas-fonts/'
FONTDIRS = [os.path.join(HERE, 'fontsrc'), CF, '/usr/share/fonts/truetype/dejavu/', '/usr/share/fonts/truetype/crosextra/']
def find(n):
    for d in FONTDIRS:
        if os.path.exists(os.path.join(d, n)): return os.path.join(d, n)
    raise SystemExit('font not found: ' + n)

SS = 8
CHARS = [chr(c) for c in range(33, 127)]      # 94 glyphs, '!' .. '~'
NUMSET = set('0123456789.,:;$%+-/\'!?()=xX*#')
# name, file, cap height (px), x-height target (px) or None, lh, top (rows above the cap line), round (px of rounding), gain, spacing (px added to each advance), charset, fallback, clut kind
FONTS = [
  dict(name='BODY', file='InstrumentSans-Bold.ttf', cap=7, top=2, lh=12, rnd=0.0, gain=1.10, space=0.7, xh=None, tw=0, clut=1),
  dict(name='READ', file='DejaVuSans-Bold.ttf',     cap=7, top=2, lh=12, rnd=0.0, gain=1.10, space=0.6, xh=None, tw=0, clut=2),
  dict(name='HEAD', file='Outfit-Bold.ttf',         cap=9, top=3, lh=15, rnd=0.45, gain=1.12, space=0.5, xh=None, tw=0, clut=1),
  dict(name='NUM',  file='Outfit-Bold.ttf',         cap=9, top=3, lh=15, rnd=0.45, gain=1.12, space=0.7, xh=None, tw=1, clut=1, only=NUMSET, fb=0),
  dict(name='FUN',  file='EricaOne-Regular.ttf',    cap=12, top=3, lh=19, rnd=0.0, erode=5, gain=1.15, space=1.2, xh=None, tw=0, clut=1),
  dict(name='BIG',  file='EricaOne-Regular.ttf',    cap=20, top=4, lh=30, rnd=0.0, gain=1.0, space=1.0, xh=None, tw=0, clut=1, only=set('ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!?.,:-+$%/\'')),
]

def cap_px_size(path, want_cap):
    # font size (at SS x) whose capital H is want_cap pixels tall
    f = ImageFont.truetype(path, 1000)
    bb = f.getbbox('H', anchor='ls')
    cap = -bb[1] / 1000.0           # em fraction
    return want_cap / cap          # pixel size

def render(fd):
    path = find(fd['file']); size = cap_px_size(path, fd['cap'])
    big = ImageFont.truetype(path, size * SS)
    out = {}
    for ch in CHARS:
        if fd.get('only') and ch not in fd['only']: out[ch] = None; continue
        adv = big.getlength(ch) / SS; shift = 0
        if fd['tw'] and ch in '0123456789':
            tw = max(big.getlength(c) for c in '0123456789'); shift = (tw - big.getlength(ch)) / 2; adv = tw / SS
        pad = 3 * SS
        W = int(adv * SS) + 2 * pad + 6 * SS; H = int(fd['lh'] * SS * 1.6) + 2 * pad
        base = pad + int((fd['top'] + fd['cap']) * SS) + 4 * SS    # baseline row (hi-res); a few rows of slack above
        im = Image.new('L', (W, H), 0)
        ImageDraw.Draw(im).text((pad + shift, base), ch, font=big, fill=255, anchor='ls')
        if fd.get('erode'): im = im.filter(ImageFilter.MinFilter(fd['erode']))
        if fd['rnd'] > 0:
            r = fd['rnd'] * SS
            im = im.filter(ImageFilter.GaussianBlur(r)).point(lambda v: 255 if v > 118 else (0 if v < 90 else int((v - 90) * 255 / 28)))
        # cell rows: top .. lh around the baseline: baseline is at row top+cap of the cell
        cell_top_hr = base - (fd['top'] + fd['cap']) * SS
        ncols = int(math.ceil((W - pad) / SS)) + 3
        cell = []
        for cy in range(fd['lh']):
            row = []
            for cx in range(ncols):
                x0 = pad + (cx - 2) * SS; y0 = cell_top_hr + cy * SS
                if x0 + SS > W or x0 < 0: row.append(0.0); continue
                box = im.crop((x0, y0, x0 + SS, y0 + SS))
                row.append(sum(box.get_flattened_data()) / (255.0 * SS * SS))
            cell.append(row)
        # trim to the inked columns (keep the advance), but leave the left side bearing as a start offset
        cols = [x for x in range(ncols) if any(cell[y][x] > 0.02 for y in range(fd['lh']))]
        if not cols: out[ch] = ('blank', adv); continue
        x0, x1 = cols[0], cols[-1] + 1
        g = [[min(15, int(round(cell[y][x] * 15 * fd['gain']))) for x in range(x0, x1)] for y in range(fd['lh'])]
        out[ch] = (g, adv, x0)        # bitmap, advance, left offset (cells)
    return out

def build():
    fonts = []
    for fd in FONTS:
        gl = render(fd); recs = []
        for ch in CHARS:
            r = gl[ch]
            if r is None: recs.append(dict(w=0, adv=0, g=None, off=0)); continue
            if r[0] == 'blank': recs.append(dict(w=0, adv=int(round(r[1] + fd['space'])), g=None, off=0)); continue
            g, adv, off = r; off -= 2
            # glyph drawn at x + off; we fold the offset into the cell by padding the left side with clear texels only if it is positive
            recs.append(dict(w=len(g[0]), adv=int(round(adv + fd['space'])), g=g, off=off))
        # the space width: from the font
        path = find(fd['file']); size = cap_px_size(path, fd['cap'])
        big = ImageFont.truetype(path, size * SS)
        sp = int(round(big.getlength(' ') / SS + fd['space']))
        fonts.append((fd, recs, max(2, sp)))
    return fonts

def pack(fonts):
    # two columns of 256 texels; each font a whole shelf-packed sheet in one column; the shortest column takes the next font (tallest first)
    sheets = []
    for fd, recs, sp in fonts:
        pos = {}; x = y = 0; rowh = fd['lh']
        for i, r in enumerate(recs):
            if not r['g']: continue
            w = r['w'] + 1
            if x + w > 256: x = 0; y += rowh
            pos[i] = (x, y); x += w
        h = y + rowh
        sheets.append((fd, recs, sp, pos, h))
    cols = [0, 0]; asg = {}
    for k in sorted(range(len(sheets)), key=lambda k: -sheets[k][4]):
        c = 0 if cols[0] <= cols[1] else 1
        asg[k] = (c, cols[c]); cols[c] += sheets[k][4]
    R = max(cols)
    img = [[0] * 512 for _ in range(R)]
    tables = []
    for k, (fd, recs, sp, pos, h) in enumerate(sheets):
        c, v0 = asg[k]; glyphs = []
        for i, r in enumerate(recs):
            if not r['g']: glyphs.append((0, 0, 0, r['adv'], 0)); continue
            x, y = pos[i]
            for yy, row in enumerate(r['g']):
                for xx, v in enumerate(row): img[v0 + y + yy][c * 256 + x + xx] = v
            glyphs.append((x, v0 + y, r['w'], r['adv'], r['off']))
        tables.append((fd, sp, c, glyphs))
    return R, img, tables

def clut_tables():
    def grey(v): return (0x8000 | v | (v << 5) | (v << 10)) if v else 0
    lin = [0] + [grey(max(1, min(31, int(round(a * 31 / 15.0))))) for a in range(1, 16)]
    bold = [0] + [grey(min(31, int(round(31 * min(1.0, (a / 15.0) * 1.7))))) for a in range(1, 16)]   # (the readable setting: soft edges pushed up, the strokes heavier)
    return [lin, lin, bold]       # 0 spare (kept equal to linear), 1 normal, 2 heavier

def main():
    fonts = build(); R, img, tables = pack(fonts); cl = clut_tables()
    nf = len(tables)
    # a glyph is {u, v, w, adv}; the left offset is stored with it: the draw adds off to x. The offset is signed and small: kept in a parallel table
    hdr = bytearray()
    hdr += b'BFNT' + struct.pack('<HBBHH', 1, nf, len(cl), R, 0) + bytes(4)
    for (fd, sp, c, glyphs) in tables:
        cap = fd['cap']; fb = fd.get('fb', 255)
        hdr += bytes([fd['lh'], fd['top'], cap, sp, c, fb if fb != 255 else 255, fd['clut'], 0])
        for g in glyphs: hdr += bytes([g[0], g[1], g[2], g[3] & 255])
        for g in glyphs: hdr += struct.pack('<b', max(-8, min(7, g[4])))        # x offset of the bitmap from the pen (cells)
    for t in cl: hdr += b''.join(struct.pack('<H', v) for v in t)
    while len(hdr) % 2048: hdr += b'\0'
    pix = bytearray()
    for row in img:
        for x in range(0, 512, 2): pix.append(row[x] | (row[x + 1] << 4))
    open(OUT, 'wb').write(bytes(hdr) + bytes(pix))
    print('FONTS.BIN: %d fonts, %d rows (%d bytes: header %d, pixels %d), cluts %d' % (nf, R, len(hdr) + len(pix), len(hdr), len(pix), len(cl)))
    for (fd, sp, c, glyphs) in tables:
        used = sum(1 for g in glyphs if g[2]); print('  %-5s col %d  cap %d line %d  glyphs %d  space %d' % (fd['name'], c, fd['cap'], fd['lh'], used, sp))
    if PREV: preview(tables, img, R, PREV)

def preview(tables, img, R, path):
    # draws sample lines with each font from the packed data itself (what the console will show), 4x
    samples = ['Feed the Pet  Money $1,250  7:45 PM', 'Illinois 1lI0O  Quit & Save? (Yes!)', 'FEED THE PET  BUY IT NOW  GO TO BED']
    rows = []
    for (fd, sp, c, glyphs) in tables:
        for s in (samples if fd['name'] in ('BODY', 'READ', 'HEAD', 'FUN') else ['$1,250  7:45  12/31  50%', 'BORE 2  0123456789'] ):
            W = 320; H = fd['lh'] + 4
            im = Image.new('L', (W, H), 0); px = im.load(); x = 4
            for chh in s:
                if chh == ' ': x += sp; continue
                i = ord(chh) - 33; g = glyphs[i]
                if not g[2]:
                    x += g[3]; continue
                for yy in range(fd['lh']):
                    for xx in range(g[2]):
                        v = img[g[1] + yy][c * 256 + g[0] + xx]
                        X = x + g[4] + xx
                        if 0 <= X < W: px[X, yy + 2] = max(px[X, yy + 2], v * 17)
                x += g[3]
            rows.append((fd['name'], im.resize((W * 4, H * 4), Image.NEAREST)))
    Hh = sum(i.height + 14 for _, i in rows)
    out = Image.new('RGB', (1280 * 2 + 10, max(Hh, 40)), (28, 34, 60))
    d = ImageDraw.Draw(out); y = 0
    for n, i in rows:
        d.text((2, y), n, fill=(255, 220, 80)); y += 12
        out.paste(Image.merge('RGB', (i, i, i)), (0, y))
        inv = Image.eval(i, lambda v: 255 - v)
        out.paste(Image.merge('RGB', (inv, inv, inv)), (1290, y)); y += i.height + 2
    out.save(path)

main()
