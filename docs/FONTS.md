# BORE 2 fonts and text (PS1 and Dreamcast; the DS can reuse the same data and rules)

The interface type was one grey bitmap face in capitals. It is now six styles, built from open fonts into one disc file,
`assets/FONTS.BIN` (49 KB, read at boot, nothing big stays in main memory), with a font-select API, a measure-then-centre helper
used on every screen, a friendly Title/Sentence case, and a TEXT LOOK option for easy reading. Before | after shots: `docs/fonts_*.png`
(`title`, `menu`, `hud`, `phone`, `pie`, `dialogue`, `arcade`, `build`, `options`, `create`, `gotchi`, `dreamcast`; every shot is the real
292x224 crop, text inside the 8 px safe area).

## The styles

| style | font | caps | line | used for |
|---|---|---|---|---|
| `FS_BODY` | Instrument Sans Bold | 7 px | 12 | menus, lists, HUD, blurbs (the clean small body face) |
| `FS_READ` | DejaVu Sans Bold | 7 px | 12 | the body face when TEXT LOOK is EXTRA CLEAR: wider, more open, heavier |
| `FS_HEAD` | Outfit Bold, softened corners | 9 px | 15 | panel titles, the name card, window headers (the rounded friendly face) |
| `FS_NUM` | Outfit Bold, tabular digits | 9 px | 15 | money and the clock (digits and `.,:;$%+-/'!?()=xX*#`; every other character comes from BODY on the same baseline) |
| `FS_FUN` | Erica One, eroded | 12 px | 19 | the chunky playful title type (K.O., ROUND 1, big words) |
| `FS_BIG` | Erica One | 20 px | 30 | the loading screen, the pet games' big letters and digits |

Dyslexia-friendly choices: heavy strokes and wide letter spacing; a capital I with feet so it is never a small l or a 1; distinct 0 and O (different widths); descenders stretched (g j p q y keep their tails at 7 px caps); no thin or condensed faces; the
EXTRA CLEAR look swaps the body face for the widest, heaviest one and pushes the soft edges up. Nothing is italic and no text bounces
or scrolls (the title's START PLAY no longer blinks).

## The data file: FONTS.BIN

Built by `python3 -I tools/make_fonts.py assets/FONTS.BIN [preview.png]` (needs Pillow; font files are looked up in `tools/fontsrc`, the
system dejavu and crosextra folders and the canvas-fonts folder; all are SIL OFL or Bitstream Vera licence). The sheets are drawn at 8x
and shrunk by area averaging, so the edges are soft, 16 grey levels.

```
0     'BFNT'  u16 version(1)  u8 nfonts  u8 ncluts(3)  u16 rows R  u16 0  4 pad          (16 bytes)
16    per font (nfonts times):
        u8 line, top, cap, space, col, fallback(255 none), clutKind, pad                  (8 bytes)
        94 glyphs for '!'..'~':  u8 u, v, w, advance                                      (376 bytes; u,v in texels of the font's own column)
        94 s8  x offsets (the bitmap starts at pen + offset)                              (94 bytes)
...   3 CLUT tables of 16 u16 (STP-set grey ramps: 0 spare, 1 normal, 2 heavier)
pad to 2048;  at byte 4096 (sector 2): R rows of 256 bytes = 512 4-bit texels (two columns of 256; font.col picks the column;
      the low nibble is the left texel)
```
A glyph is a `w` x `line` cell at (u, v); the cell's row `top + cap` is the baseline, so descenders live below it. A space is `space` px.
A glyph with w = 0 and advance = 0 in a font that has a fallback is taken from the fallback style (the NUM digits fall back to BODY).

Memory: the glyph tables are resident (about 480 bytes a font, 2.9 KB); the sheet is 176 rows x 128 halfwords = 44 KB of VRAM in the
strip pages (4-bit, one block), taken at boot from the atlas. Net main memory: about 11 KB more free than the old resident bitmaps.

## API (src/font.h, src/gfx.h, src/ui.h)

* `font_init()` once after the CD and the atlas are up (reads FONTS.BIN with the CD part reader; the Dreamcast reads the same file from its disc image).
* y is always the **top of the capitals**; the baseline is `font_cap(style)` rows below it.
* `font_w(style, s)` advance width; `font_ink_w(style, s)` ink width without trailing space (what to centre); `font_text`, `font_textc`.
* `gfx_text_box(style, x, y, w, h, flags, s, r, g, b)` the measure-then-centre helper: flags `TX_LEFT/TX_CENTER/TX_RIGHT | TX_TOP/TX_MID/TX_BOTTOM`.
  The ink is measured, then placed in the box; text wider than the box starts at the left edge instead of overflowing both sides.
  `ui_text_box(x, y, w, h, flags, size, s, colour)` is the same on the UI sizes; `ui_textc`/`gfx_textc` centre on the ink width;
  `ui_textr(rx, ...)` right-aligns; `ui_text_base(x, baseline, ...)` puts two sizes on one baseline (the clock's AM/PM).
* UI sizes: 1 body, 2 heading, 3 big, 4 numbers, 5 fun.
* Dark ink on a white card is drawn by subtracting (`font_sub`), light text by adding; the UI helper picks it from the colour.
* `font_look` (LOOK_SIMS / LOOK_READABLE / LOOK_CAPS) is set from OPTIONS > SCREEN > TEXT LOOK (FRIENDLY, EXTRA CLEAR, CAPITALS); default FRIENDLY.
  `font_raw = 1` around text that must stay as written (the pad's key caps, the arcade cabinets' three-letter codes).

## Case

The game's strings are written in capitals. In the friendly look (and EXTRA CLEAR) `font_case` makes them readable:
Title Case for up to three words ("Start Play", "X Play Bortua Fighter"), Sentence case for four or more or when there is a full stop
("Welcome home. X uses things."). It keeps acronyms (TV, DJ, PM, XP...), digits and times (7:45 AM, 7AM-3PM), the pad's X and O, and capitalises
names listed in `L_PROPER` (Bore, San Boreas, days, months...). `font_shaped` applies it to a whole text before wrapping, so lines are not
shaped one by one. Add a word to `L_ACRO` or `L_PROPER` in `src/font.c` when a name comes out wrong. CAPITALS shows the text as written.

## Saves and options

The option is `O_TEXT` (one byte after REAL TIME); save VERSION 49 holds it; older saves load with FRIENDLY (checked with the v47 card, test/oldsave.sh style).

## Sims-style wording and parts

Toasts (the notification feed, `feed_draw`) are white cards with a round icon (green tick: good news, red: a problem, blue: news; `toast_kind`
chooses from the words); mood and need words, thought-bubble style labels (a round bubble with a small tail, never a plumbob: the gold arrow is
the marker), tidy panel titles in the heading face, money and the clock in the number face.

## For the DS and other ports

* The data is portable: read `FONTS.BIN` as above, or convert (`tools/dsbuild/mk_dsfonts.py` is the DS helper's own 1-bit conversion).
* Keep the three rules: y is the cap top, centre on ink width, measure before placing. Then the same call sites work in every port.
* The case rules are plain C in `src/font.c` (`font_case`, three word lists) and can be copied as they are.
* Dreamcast: `src` is shared, `FONTS.BIN` is on the GD-ROM (ports/dreamcast/Makefile passes it to mkdisc.py); nothing else is needed. Shot: `docs/fonts_dreamcast.png`.
