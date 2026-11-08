#!/usr/bin/env python3
"""mk_dsfonts.py [out.h] [preview.png]: the DS's five UI fonts (D65), rendered from hinted TrueType outlines into 1-bit glyph rows.
   Hinted at these sizes (DejaVu's bytecode, Poppins at 12+), so the strokes are whole pixels and the shapes stay open and clear
   (dyslexia: no thin strokes, I / l / 1 and O / 0 all differ).  Fonts (the s argument of text() / tw()):
     1 BODY   DejaVu Sans 9          clean, roomy, the default
     2 HEAD   DejaVu Sans Bold 9          , friendly, bold: titles of panels, buttons
     3 SMALL  Liberation Sans 8       labels, captions
     4 NUM    DejaVu Sans Mono Bold 10 one width digits for money and the clock, slashed zero
     5 TITLE  Poppins Bold 15        the fun big face (title screen, big banners)
   The files used are on the build machine (DejaVu: Bitstream/public domain licence, Poppins: SIL OFL). Output ds_fonts.h is checked in,
   so the DS build needs no Python or fonts."""
import sys
from PIL import Image, ImageDraw, ImageFont
D = "/usr/share/fonts/truetype/"
FONTS = [  # name, file, size, round-corners
    ("BODY", D + "dejavu/DejaVuSans.ttf", 9, 0),
    ("HEAD", D + "dejavu/DejaVuSans-Bold.ttf", 9, 0),
    ("SMALL", D + "liberation/LiberationSans-Regular.ttf", 8, 0),
    ("NUM", D + "dejavu/DejaVuSansMono-Bold.ttf", 10, 0),
    ("TITLE", D + "google-fonts/Poppins-Bold.ttf", 14, 0),
]
out = sys.argv[1] if len(sys.argv) > 1 else "ds_fonts.h"
prev = sys.argv[2] if len(sys.argv) > 2 else None

# hand fixes: the look-alikes get distinct shapes. rows are strings, '#' ink; drawn on a w x cap box (cap rows ending at the baseline)
FIX = {
    "BODY": {"I": ["###", " # ", " # ", " # ", " # ", " # ", "###"]},
    "SMALL": {"I": ["###", " # ", " # ", " # ", " # ", "###"]},
    "HEAD": {"I": ["###", " # ", " # ", " # ", " # ", " # ", "###"]},
}

def render(name, path, size, grow):
    f = ImageFont.truetype(path, size)
    asc = desc = 0
    gl = []
    for c in range(32, 127):
        ch = chr(c)
        im = Image.new("1", (40, 60), 0)
        d = ImageDraw.Draw(im); d.fontmode = "1"
        d.text((6, 30), ch, font=f, fill=1, anchor="ls")
        adv = int(round(f.getlength(ch)))
        bb = im.getbbox()
        if grow and bb:   # round the outer corners of the fat strokes (a pixel with open up+left (etc.) and solid across the diagonal)
            src = im.copy(); P = lambda x, y: src.getpixel((x, y)) if 0 <= x < 40 and 0 <= y < 60 else 0
            for y in range(60):
                for x in range(40):
                    if not P(x, y): continue
                    for dx, dy in ((-1, -1), (1, -1), (-1, 1), (1, 1)):
                        if not P(x + dx, y) and not P(x, y + dy) and P(x - dx, y) and P(x, y - dy) and P(x - dx, y - dy) and P(x - 2 * dx, y) and P(x, y - 2 * dy):
                            im.putpixel((x, y), 0)
            bb = im.getbbox()
        gl.append((ch, im, bb, adv))
        if bb:
            asc = max(asc, 30 - bb[1]); desc = max(desc, bb[3] - 30)
    capbb = [g for g in gl if g[0] == "H"][0][2]
    cap = 30 - capbb[1]
    H = asc + desc
    return f, gl, asc, desc, H, cap

def build(spec):
    name, path, size, grow = spec
    f, gl, asc, desc, H, cap = render(*spec)
    rows, meta = [], []
    for ch, im, bb, adv in gl:
        if name in FIX and ch in FIX[name]:
            art = FIX[name][ch]
            im = Image.new("1", (40, 60), 0)
            top = 30 - len(art) + 1
            for r, s in enumerate(art):
                for q, v in enumerate(s):
                    if v == "#": im.putpixel((6 + q, top + r), 1)
            bb = im.getbbox(); adv = len(art[0]) + 1
        if not bb:
            rows.append([0] * H); meta.append((adv, 0, 0)); continue
        x0 = bb[0]; w = bb[2] - bb[0]
        if ch != ' ' and adv - (x0 - 6 + w) > 2 and name != 'NUM': adv = x0 - 6 + w + 2   # no wide gap after W, m ...
        assert w <= 16, (name, ch, w)
        rr = []
        for r in range(H):
            y = 30 - asc + r; v = 0
            for q in range(w):
                if im.getpixel((x0 + q, y)): v |= 1 << (15 - q)
            rr.append(v)
        rows.append(rr); meta.append((adv, x0 - 6, w))
    return dict(name=name, H=H, asc=asc, cap=cap, rows=rows, meta=meta)

fonts = [build(s) for s in FONTS]
with open(out, "w") as o:
    o.write("// ds_fonts.h: made by tools/dsbuild/mk_dsfonts.py (D65). Five 1-bit proportional fonts; glyph rows are u16, the left pixel is bit 15.\n")
    o.write("// per font: H rows, B = rows from the top to the baseline (inclusive), CAP = cap height. per glyph (32..126): adv, xoff (s8), ink width.\n")
    o.write("typedef struct { u8 fh, fb, fcap; const u8 *adv; const s8 *xoff; const u8 *w; const u16 *rows; } DsFont;\n")
    for fo in fonts:
        n = fo["name"]
        o.write("static const u8 FA_%s[95] = {%s};\n" % (n, ",".join(str(m[0]) for m in fo["meta"])))
        o.write("static const s8 FX_%s[95] = {%s};\n" % (n, ",".join(str(m[1]) for m in fo["meta"])))
        o.write("static const u8 FW_%s[95] = {%s};\n" % (n, ",".join(str(m[2]) for m in fo["meta"])))
        o.write("static const u16 FR_%s[95 * %d] = {\n" % (n, fo["H"]))
        for r in fo["rows"]: o.write(",".join("0x%04x" % v for v in r) + ",\n")
        o.write("};\n")
    o.write("static const DsFont DSFONT[5] = {\n")
    for fo in fonts:
        n = fo["name"]; o.write("  { %d, %d, %d, FA_%s, FX_%s, FW_%s, FR_%s },\n" % (fo["H"], fo["asc"], fo["cap"], n, n, n, n))
    o.write("};\n")
for fo in fonts: print(fo["name"], "H", fo["H"], "asc", fo["asc"], "cap", fo["cap"], "bytes", len(fo["rows"]) * fo["H"] * 2 + 95 * 3)

if prev:   # a preview sheet, 4x
    lines = ["Hunger Energy Bladder Social Hygiene Fun", "The Sims-style: Ashley Newbie, Adult 24", "$3,500 12:10 PM Mon 29 0123456789 O0 Il1 l I", "Wants a snack! Needs a bath. Feeling great :) 50%", "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ", "!?.,:;'\"()-+/&#@*<>=[]%$ gjpqy"]
    W = 256; imgs = []
    for fo in fonts:
        for ln in lines:
            im = Image.new("RGB", (W, fo["H"] + 4), (20, 30, 60)); x = 2
            for ch in ln:
                c = ord(ch)
                if c < 32 or c > 126: continue
                i = c - 32; adv, xo, w = fo["meta"][i]
                for r in range(fo["H"]):
                    v = fo["rows"][i][r]
                    for q in range(w):
                        if v >> (15 - q) & 1 and 0 <= x + xo + q < W: im.putpixel((x + xo + q, 2 + r), (250, 250, 250))
                x += adv
            imgs.append(im)
    sheet = Image.new("RGB", (W, sum(i.size[1] for i in imgs)), (0, 0, 0)); y = 0
    for i in imgs: sheet.paste(i, (0, y)); y += i.size[1]
    sheet.resize((W * 3, sheet.size[1] * 3), Image.NEAREST).save(prev)
