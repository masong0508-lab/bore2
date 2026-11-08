#!/usr/bin/env python3
"""glyph_shots.py PS1_DIR DC_DIR OUT.png: the shipped-strings page (SIMS look) and the BODY alphabet page (after a Z), PS1 on the left and Dreamcast on the right, twice the size.
   Run with python3 -I (test/glyph_check.sh keeps the frames only with KEEP=1)."""
import sys
sys.argv, args = sys.argv[:1], sys.argv[1:]
src = open(__file__.replace('glyph_shots.py', 'glyph_check.py')).read().replace('sys.exit(main())', '')
exec(src)
from PIL import Image
p1, dc = load_dir(args[0]), load_dir(args[1])
out = Image.new('RGB', (1280, 960))
for row, pg in enumerate((36, 3)):   # page 36: the strings; page 3: BODY, each character after a Z, first third
    for col, d in enumerate((p1, dc)):
        im, dx, dy = d[pg]
        out.paste(im.crop((dx, dy, dx + 320, dy + 240)).resize((640, 480), Image.NEAREST), (640 * col, 480 * row))
out.save(args[2])
