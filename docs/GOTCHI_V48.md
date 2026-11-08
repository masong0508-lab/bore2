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
