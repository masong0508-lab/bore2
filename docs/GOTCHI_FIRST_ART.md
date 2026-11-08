# Boreagotchi first-run art (portable data for the DS port)

Made by `python3 tools/make_gotchi_first.py [preview.png]`. It writes two files with the same data:
- `src/gotchi_first_art.h`: plain C. `FS_<NAME>[]` = array of row strings, `FS_<NAME>_W`, `FS_<NAME>_H`. Also `FS_FONT_CH`, `FS_FONT[]` (5x7 glyphs, 35 chars each, '#' = on), `FS_PAL[][4]` (key char, r, g, b), `FS_PAL_N`, and the egg parts (below: `FS_EGG?_P0..6`, `FS_CRACK?`, `FS_CRACK?_N`, `FS_EGG_N`).
- `docs/gotchi_first_art.json`: `sprites` (every sprite, and every egg and crack overlay in full), `egg_parts` (the same patterns and cracks as the header has them), `palette`, `mapped`, `font`. Same content for tools.

Sprites (one char = one art pixel, '.' = clear): BOX_BODY 30x17, BOX_LID 32x7, BOX_MOUTH 30x5, RIB_BODY, RIB_LID, BOW 22x12, TAIL_L, TAIL_R, TOY_FRONT / TOY_BACK 22x28 (handheld toy), EGGB / EGGL / EGGS (the plain egg in three sizes, below), EGGCUP (bottom shell, 14x9 with its shadow), SHARD0-3 (4-5 dots), SPK_S/M/L (twinkles). TOY_FRONT's LCD is 12x11 art pixels (rows 9-19), so the 8x9 egg sits in the middle of it with a dot to spare above and below; the buttons are one row lower than in the first version.

Colour keys:
- Fixed keys are in `FS_PAL` (gold, box red, ribbon, LCD green, outline, white).
- Mapped at run time (see `f_setup` in src/gotchi_first.inc): shell `S`/`s`/`z` = owner colour light/base/dark; LCD `L`/`l` = 172,196,128 / 150,176,108 when on, dark grey when off (`l` is also the egg's ground shadow). Egg keys (the same sums are `egg_rgb` in src/gotchi_ui.c, so the dots of the LCD, the toy's LCD and the hatching agree; ink = the pet's colour, PAL row of gotchi_ui.c, 0 is the LCD's dark green):
  `#` outline = ink x 0.42; `m` body = cream (240,238,206) 74% + ink 26%; `d` shade = cream 48% + ink 52%; `o` spots = ink x 0.92; `h` highlight = 255,255,240; `u` light in a crack = 255,250,190; `g` gold = 255,208,60 (the rare egg's stars).

## Eggs (all of them: pet view, basket, home, tree, day card, guide, toy LCD, hatching)

One design in three sizes, all symmetric about the middle (an even width, so the 32-dot LCD's own middle is between dots 15 and 16): a one-dot dark outline, a light body, a shade down the right and along the bottom, a highlight top left, no dithering, a ground shadow in the LCD's shade.
- `EGGB` 14x17 + 2 shadow rows: the hatching, drawn 5 px a dot (square dots; the baby is drawn at 6). `EGGL` 10x12 + 1 shadow row: one dot of the 32x16 LCD in the pet view (5 px, square, no scanline gap), basket and home (2 px), tree, day card and guide tiles (1 to 2 px); it sits in cols 11-20, rows 2-14. `EGGS` 8x9: inside the toy's LCD (3 px a dot on the shop card, 5 px on the egg card, 2.5 px in the box reveal).
- Seven patterns per size: 0 freckles, 1 bands, 2 zigzag, 3 patches, 4 polka dots, 5 heart, 6 the rare one (gold stars; used when the kind is 16 or more or the pet has a mutation). The kind picks it (`kind % 6`); the colour is the family colour of the pet, so one kind has one pattern in each of its seven colours.
- The header holds the plain egg per size and the rest as data: `FS_EGG?_P<n>[]` = runs of spots `{x | y << 4, length | 128 if gold}` ending in 0,0 (they replace body, shade and highlight cells, the outline stays); `FS_CRACKB[]` / `FS_CRACKL[]` = crack cells `{x | y << 4, key}`, stage n is the first `FS_CRACK?_N[n]` cells (stage 1 a hairline from the top, 2 a long crack with a branch, 3 the split all round with light along it). A crack through a spot is drawn light. The game puts an egg together in a small buffer (`egg_make` in src/gotchi_ui.c); a port may do the same, or take the whole sprites from the JSON (`EGGB0`..`EGGB6`, `EGGL?`, `EGGS?`, `CRACKB1`..`CRACKB3`, `CRACKL1`..`CRACKL3`, overlays drawn over the egg of the same size).
- Wobble: a lean, the top row-shifted and the foot still, in whole art pixels (`tilt * (h-1-row) / (h-1)`, rounded towards zero on the dots; `fsq` rounds the hatching and toy eggs to whole pixels). The pet view eggs lean 1 to 2 dots in a slow rock now and then (every 24 steps of 3 a second, every 12 once it cracks); the cracks follow the egg's age: 15 minutes a hairline, 17 a long crack, 19 the split (it hatches at 20), the same three stages as the hatching.
- Hatching: the egg is centred in the LCD (5 px a dot, 6 px of room above and below with its shadow), rocks in the same windows, the cracks above, the LCD brightens as the light grows (no halo round the egg), the baby stands in the bottom shell (EGGCUP, 14x9 with a ragged edge and the light of the inside along it), shards fly.

Drawing: each row is run-length merged into rectangles. Pixel width in 16ths (so scales like 2.5 work), height whole pixels. Extras: tilt (row shear, for wobble), flip, clip box (the LCD). On the DS draw runs as filled rects or blit into a tile; the numbers do not depend on the PS1.

Font and title: `f_banner` draws the 5x7 face at scale s with a 1-pixel dark rim, a 7-step gold slope (rows 0-6, 255,246,170 down to 196,98,16) and a shine bar.

Timeline (60 Hz ticks, unbox clock): drop-in 0-30, shake 30-86, bow pop 70, ribbon 72-82, POP 86 (UB_POP), toy rises to 126, LCD on 126, ready 130 (UB_DONE), auto end 330 (UB_END). Hatch clock hT: wobble windows, cracks from about 56, glow and flash 84-90, shards and baby after 90. Sound cues: `ubCue[]` and `hCue[]` in src/gotchi_ui.c (tick, voice id, pitch).

Layout: all drawing is inside the 292x224 crop and the 8 px safe area (320x240 screen, art centred on x 160).
