# Boreagotchi first-run art (portable data for the DS port)

Made by `python3 tools/make_gotchi_first.py [preview.png]`. It writes two files with the same data:
- `src/gotchi_first_art.h`: plain C. `FS_<NAME>[]` = array of row strings, `FS_<NAME>_W`, `FS_<NAME>_H`. Also `FS_FONT_CH`, `FS_FONT[]` (5x7 glyphs, 35 chars each, '#' = on), `FS_PAL[][4]` (key char, r, g, b), `FS_PAL_N`.
- `docs/gotchi_first_art.json`: `sprites`, `palette`, `mapped`, `font`. Same content for tools.

Sprites (one char = one art pixel, '.' = clear): BOX_BODY 30x17, BOX_LID 32x7, BOX_MOUTH 30x5, RIB_BODY, RIB_LID, BOW 22x12, TAIL_L, TAIL_R, TOY_FRONT / TOY_BACK 22x28 (handheld toy), EGG0-3 14x17 (4 wobble frames) and EGGS0-3 7x9 (small), CRACK1-3 (overlays), EGGCUP (bottom shell), SHARD0-3 (4x4), SPK_S/M/L (twinkles).

Colour keys:
- Fixed keys are in `FS_PAL` (gold, box red, ribbon, LCD green, outline, white).
- Mapped at run time (see `f_setup` in src/gotchi_first.inc): shell `S`/`s`/`z` = owner colour light/base/dark; LCD `L`/`l` = 172,196,128 / 150,176,108 when on, dark grey when off; egg ink `#` = pet colour, `m d o` = pet colour mixed into the LCD green 22 / 48 / 72 percent, `h` = highlight (LCD green 55 percent to white), `u` = 255,248,190.

Drawing: each row is run-length merged into rectangles. Pixel width in 16ths (so scales like 2.5 work), height whole pixels. Extras: tilt (row shear, for wobble), flip, clip box (the LCD). On the DS draw runs as filled rects or blit into a tile; the numbers do not depend on the PS1.

Font and title: `f_banner` draws the 5x7 face at scale s with a 1-pixel dark rim, a 7-step gold slope (rows 0-6, 255,246,170 down to 196,98,16) and a shine bar.

Timeline (60 Hz ticks, unbox clock): drop-in 0-30, shake 30-86, bow pop 70, ribbon 72-82, POP 86 (UB_POP), toy rises to 126, LCD on 126, ready 130 (UB_DONE), auto end 330 (UB_END). Hatch clock hT: wobble windows, cracks from about 56, glow and flash 84-90, shards and baby after 90. Sound cues: `ubCue[]` and `hCue[]` in src/gotchi_ui.c (tick, voice id, pitch).

Layout: all drawing is inside the 292x224 crop and the 8 px safe area (320x240 screen, art centred on x 160).
