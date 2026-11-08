# Boreagotchi v48 (save version 48)

Shots: docs/gotchi_v48_1.png to docs/gotchi_v48_5.png (12 screens a sheet, from test/gviews.sh; the last sheet is the new game WHACK, with the game list: 8 games, no half-empty last row).

- Needs: food, fun, energy, clean, bond, manners. Energy and clean drain by life stage. Tired or dirty pets get sad, sick or lonely.
- Moodlets: up to three short chips under the LCD. Wishes last 10 hours; granting one gives bond.
- Goals: three a day (icon TODAY), streaks, 7-day history, awards.
- Games (8): WHICH WAY, COPY ME, CATCH, HIGH LOW, BEAT, HOP, STACK, WHACK. They train SPEED, BRAINS and CHARM; grade S to D, best scores. The game list is 2 columns by 4 rows.
- WHACK (trains SPEED): 14 pops. A bug pops out of one of four holes; press that way on the D-pad. Quick whack 2 points, slow 1, wrong hole or too late 0, too soon -1. From pop 4 some pops are MESS (red "!"): leave it alone for 1 point, whack it and you lose the pop. The window shrinks every pop; SPEED adds time. Grade = points out of what was on offer. The pet's eyes follow the hole, it hops on a hit and sulks on a miss; pitched voices rise with each pop, groan on the mess. Best score is gbest[7] (the v48 save already holds 8 bytes, so no new version). TRAIN SPEED now takes turns between HOP, CATCH and WHACK.
- Diary: a new best now says the game ("BOOP BEAT ITS BEST AT WHACK") and a training round says the skill ("TRAINED ITS SPEED"), in the diary and the memories page. The game number rides in bits 5-7 of the diary kind (old v48 entries read as WHICH WAY / plain TRAINED HARD).
- Training: BRAINS now adds MANNERS (skill / 20) to every training round (the SKILLS page already promised it).
- First day: scripted steps with a gold frame on the icon to use.
- Surprises: fly, coin, shooting star, thunder, visitor. Particles and blinking. Pitched pet voices (audio_voicep).
- Family: talents inherited, best friends, birthdays, memories page.
- Save: VERSION 48. Per pet adds Xt; per toy adds goals; diary is 32 lines. Older saves get default values.
- Disc: the GOTCHI.OVL slot grew from 48 to 72 sectors (tools/packs.py), so the disc layout changed; old PPF packs do not fit.
- RAM (2 MB): 11,720 bytes free past _end (nothing resident changed for WHACK; the 11,864 above was another count). 8 MB boots. GOTCHI.OVL 132,028 bytes of 156,096 (was 127,944). Test hooks: -DAUTO_WHACK=n plays WHACK by itself (n frames of reaction), AUTO_GVIEWS shoots it twice (a bug, then a mess).

## First-run art (title, OPEN IT!, egg card, hatching)

Shots: before docs/gotchi_first_before_1.png and _2.png; after docs/gotchi_first_after_1.png to _3.png (one tile per step, real 292x224 crop). Portable art notes: docs/GOTCHI_FIRST_ART.md.

- Title: pixel BOREAGOTCHI! lettering (gold slope, dark rim, shine sweep), three LCD pet tiles, neat button bar.
- Starfield (all Boreagotchi sky screens): two gradients, a slow glow, 32 stars in 3 layers that drift and twinkle, a shooting star now and then.
- Shop card: new handheld toy art (egg-shaped, keyring, LCD with egg, three gold buttons), text moved down, nothing overlaps.
- OPEN IT!: a big shaded gift box with a bow and a "BOREAGOTCHI!" label. It drops in, squashes, hops, shakes with the bow wiggling and a glow leaking out, the ribbon unwraps, then POP: flash, lid flies off, light beams, ring, confetti, sparkles. The toy rises and turns, the LCD turns on and shows the egg.
- Egg card: the toy beside four short lines (it came with an egg, keep it warm, it hatches soon, feed and play, I will show you how).
- Hatching: a toy frame with an LCD (green palette), egg at 6x with spots, wobble windows, a glow, cracks that grow, a flash, shell pieces fly, the bottom shell stays, the baby appears with sparkles and hearts. Banners: SOMETHING IS MOVING INSIDE... / IT IS CRACKING! / POP! / IT HATCHED!
- Sounds: cue tables ubCue[] and hCue[] in gotchi_ui.c (pitched voices: jingle on landing, rattle on shakes, pop at the burst, chime on the reveal, rising notes in the hatch). Still pitched voices only, no new samples.
- Code: src/gotchi_first.inc (sky, sprite and banner helpers), src/gotchi_first_ui.inc (screens), src/gotchi_first_art.h and docs/gotchi_first_art.json (made by tools/make_gotchi_first.py). Timeline: UB_POP 86, UB_DONE 130, UB_END 330 (60 Hz ticks).
- Test hook: build with -DAUTO_GOTCHI=0 -DAUTO_GSEQ and run test/gseq.sh TAG (steps listed in test/gseq_tab.h); it writes docs/gotchi_first_TAG_N.png. A readout of the largest 2D packet use per step is in the script's last comment (peak 17,088 of 59,392 bytes).
- Size: GOTCHI.OVL 149,156 bytes (+2,632 bss = 151,788 of 156,096). The slot grew from 72 to 76 sectors, so the disc layout changed again; old PPF packs do not fit. Resident RAM: 11,688 bytes free past _end on 2 MB (was 11,720, 32 bytes more for the include and cue glue). Save format unchanged (still VERSION 48; old saves load).

## Eggs redrawn (every egg of the toy)

Shots: before docs/gotchi_egg_before_1.png to _3.png, after docs/gotchi_egg_after_1.png to _3.png (34 tiles each, the real 292x224 crop: toy cards, pet view with every pattern and the three crack stages, basket, home, tree, day card, guide pages 3 and 4, hatching). Portable data and rules: docs/GOTCHI_FIRST_ART.md. The first-run sheets docs/gotchi_first_after_1.png to _3.png were shot again with the new art.

- What was wrong: the pet view egg was dot art (an ellipse with a few holes, cracks as three lines at age 15); the guide tile showed it dark and "cracking" (age 16); the hatching egg was a faint tan body with a yellow halo disc, four patterns, rows of dither; the toy's LCD egg sat on the LCD's bottom row; in the tree a relative that was still an egg was drawn as a grown-up; the home put eggs lower than the other pets.
- One new design in three sizes (the hatching 14x17, the 32x16 LCD's dots 10x12, the toy's LCD 8x9), symmetric about the LCD's middle, clean one-dot outline in the dark of the pet's colour, cream body leaning to the colour, shade, highlight, ground shadow, no dithering. Seven patterns (freckles, bands, zigzag, patches, polka, heart, and the rare one with gold stars: a kind of 16 or more, or a mutation, which also twinkles on the LCD); the kind picks the pattern, the family colour tints it. All screens draw it from the same data, in the same colours (`egg_rgb`).
- Square dots: the pet view egg is 5 px by 5 px (no scanline gap, creatures keep theirs); the hatching egg 5 px, centred in its LCD with its shadow (the baby is 6 px, standing in the bottom shell, EGGCUP); basket and home 2 px; tree, day card and guide 1 to 2 px. The toy's LCD is now 12x11 art pixels and the 8x9 egg is in the middle of it (shop card, box reveal, egg card, day one card).
- Idle wobble: a slow lean of 1 to 2 dots now and then in the pet view (each egg out of step with the others), whole dots, the foot stays; the toy's egg leans one pixel now and then; the hatching rocks in the old windows, in whole pixels.
- Crack stages follow the age in the pet view (15 hairline, 17 long crack with light, 19 split all round; it hatches at 20) and the clock in the hatching (t34, t52, t72); a crack through a spot is light, so it reads on every pattern. The halo disc is gone: the LCD brightens as the light grows. The shell cup, shards and the baby's place were redone.
- Tree: a relative that is still an egg is drawn as an egg. Guide tiles: the egg is whole (not cracking), in colour. Home: eggs stand on the floor like the others.
- The mystery egg (a visitor's rare surprise, `gx_event_do`) is an ordinary egg in the data (no flag is saved), so it looks like any other; a rare kind or mutation shows the gold-star egg. There are no egg pictures on the TREE, DIARY or other icons (they are the UI icons), and the world prop of the toy (g_egg, 8x8 in items) was left alone.
- Data: src/gotchi_first_art.h holds the plain egg per size, the pattern runs and the crack cells (stored as whole sprites, seven patterns in three sizes overflowed the overlay by 48 bytes in a test build); `egg_make` in src/gotchi_ui.c puts one together in a 300-byte buffer. docs/gotchi_first_art.json also holds every whole egg and crack overlay for the DS port.
- Test hooks (the AUTO_GVIEWS sweep needs about 470 bytes of its own game set-ups left out to link: it was run that way, every screen fine): build with `-DAUTO_GOTCHI=0 -DAUTO_GSEQ -DAUTO_GEGG` and run `test/gegg.sh TAG` (table test/gegg_tab.h; the optional columns x and a make every slot an egg of its own pattern and set an egg's age); it writes docs/gotchi_egg_TAG_N.png through test/gseq.sh (new variables TABF and PFX).
- Size: GOTCHI.OVL 150,276 bytes (+3,000 bss = 153,276 of 156,096; was 149,156 + 2,632). Still 76 sectors, so the disc layout is the same. Resident RAM unchanged: 11,688 bytes free past _end on 2 MB (0x80200000 - 0x801fd258). 8 MB boots (DS8MB=1, to the title). Save format unchanged (VERSION 48; an egg's pattern comes from its kind and colour, its cracks from its age, nothing new is stored).
