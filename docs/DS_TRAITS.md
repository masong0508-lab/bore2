# DS traits (D73)

The PS1 traits engine (`src/traits.c`, `traits_auto.c`, `traits_data.h`) runs in the DS build as is.
The DS only offers a trait if every number it changes is a real DS number. No cosmetic traits.

## Offered on the DS
- Regular: 348 of 600. Earned: 30 of 51. Prize: 7 of 24. Cheat: 248 of 351.
- Wishes: 41 of 61 (the ones whose counter and prize the DS has).
- Slots: 6, plus 7 (1 wish done), 8 (3 wishes), 9 (job level 5), 10 (10 wishes).
- Cheat traits: only in slots 7 to 10, only with OPTIONS > GAME > Cheat traits On (default Off), red CHEAT badge, asked once, no medals, high scores or awards while one is on.
- Household members get traits from their personality (move-in, spouse, roommate, baby, first family). Children inherit. Not editable. See the Life page (page 3), LEFT/RIGHT picks the housemate.

## Hooks the DS supports (78)
Needs fall and gain (food, wc, rest, clean, fun, social), 11 action gains, farming skill, mood base / floor / night / day / cold / heat / crowd / alone / fears / indoors / outdoors / love / pet joy, relation gain and loss, chat / joke / flirt / hug odds, charm, romance, engage, job pay / perf / promo / strikes, shop pay (food, retail, swim, biz, teach, farm), income, spend, shop discount, walk and vehicle speed, night and day rest, pet food, Boreagotchi xp and gifts, arcade score and tickets, fight damage and defence, luck, loot, spark, age speed, aspiration gain and keep.
Mood points are halved to the DS 0 to 100 scale.

## Hooks the DS does NOT support (traits that need them are not offered)
| Hook | Why |
|---|---|
| DEC_COMFY, GAIN_COMFY | the DS has no comfort need |
| SK_COOK, BODY, FUN, LOGIC, CHAR, CREAT, MECH, MUSIC | the DS has only the farm skill |
| MOOD_PULL, ML_GOOD, ML_BAD | no moodlet system |
| REL_DECAY | relations do not decay on the DS |
| PAY_FACTORY, FIT, HOTEL, GAME, NIGHT, MED, FILM, BEAUTY, ART | those jobs are not on the DS |
| FARE, FUEL_USE | no fares or fuel |
| RAIN_TOL | no rain mood |
| FIGHT_REGEN, FIGHT_BELT | DS fight has no regen or belts |
| GRADE | no school grades |

The CREATIVE category has 0 offered traits for that reason.

## Counters the DS does not have (earned traits and wishes using them are not offered)
Anything outside: cooked, ate out, snacks, slept, showers, fun, films, books, coffees, runs, swims, calls, gathers, chats, cares, jokes, hugs, flirts, kind acts, paydays, promos, lucky, days, earned, cash, job level, friends, family, districts, fights, champ, arcade plays, tickets, home, vehicles, partner, hatched, gelder, pet, farm skill.

## Changes to the base game
- Chat, joke, flirt and hug now roll for success (base 85, 70, 70, 85 percent), like the PS1. A kiss to a partner uses the ENGAGE roll (base 75).
- Wish prizes: cash, trait, outfit; food and bus prizes pay cash; tokens add arcade tickets; aspiration adds life points.

## Save
New tagged block `TR` in the save, about 243 bytes. Old d70/d71/d72 saves load (old Create-a-Sim traits and wishes 1 to 5 map to the new ids).

## Tests
`ports/dsi/test/ds_traits_test.sh`
- Host (`ds_traits_test.c`, same engine and accessors as the ROM): 78 of 78 supported hooks move a DS number; 633 offered traits, 0 fail; 79 checks, 0 failed.
- melonDS: a d72 save (written by a d72 test ROM) loads in d73 (result 401); a new save reloads with an identical trait checksum.

## D74: ported (per-member counters, wishes, personality-derived picks)
D74 gives every DS housemate their own ev[]/wd[] and a personal wish (trf_auto_wish), the household unlock count sums everyone's wishes, and the save appends a marker-2 block (d73 saves still load: test_sres 401). Shot: docs/ds_shots/traits7_d74_load.png. Original note follows.
The PS1 now gives every town person a personality from their id (`src/persona.c`), assigns traits on joining and keeps counters, earned traits and a wish per member. The DS port (d73) still has the older design: household members get traits from personality but win no earned traits and have no personal wish; townie cards show no personality. Port `persona.c` and the per-member counters (`ev`, `wd`) in ds_trgame.h; the data mapping is identical. Planned as d74.


## D75: stable townies
The DS townie's id was made from the block and slot alone, so every save had the same people, and a new world had them too. Now the save's world seed goes into the id (`hsh(seed ^ 0x70574E49)` mixed in `city_spawn`), so the same townie is the same person (id, name, look, personality from the name) on every boot and every reload, and a different world has different people. Old saves (and the relations and who moved in that they hold, kept by id) load with the old ids: the save marks new-style saves with `0x5E` in the unused byte `SavCore.opt[39]` (an old save has 0 there), no format change. A person who moved in keeps their name and look (the Sim holds them) and their id stays `taken`.
Test (melonDS, `-DDSTEST -DAUTO_SCENE=144`, scenes 145 to 148): the checksum of all 24 townies' ids, looks and names is the same after writing and reading the save and on a second boot, different for another seed and for an old save: `bash ports/dsi/test/ds_town.sh` (A write -1422507364, B reload -1422507364, old save -485464410, other seed -1669655897). Shots (`docs/ds_shots/`): `traits7_townie_popup.png`, `traits7_family6.png` and `traits7_family6_1..5.png` (a household of six, each traits page with its wish; scenes 150 to 155 with AUTO_SCENE=149, 149 itself the pop-up). Speed (melonDS, live, street, town): 29.8 fps average for d74 and d75 (28 to 30), quads sent 2,983 to 3,126 and 2,988 to 3,101. ROM: `bore2_dsi_d75.nds`.

## D76: a townie's own personality
Until d75 a DS townie's personality came from their NAME (`trf_pers_from_name`, 96 names), so two townies with one name were the same person. Now it is a pure function of the save's world seed and the townie's unique id: `ps = persona_town_seed(seed, id)` (set in `city_spawn`), personality = `persona_of(ps, stage)`. That is the PS1's code (`src/persona.c`, included by `ds_trfx.h`; the PS1 sources and image are unchanged), so the same recipe gives the same kind of person on both. The ids themselves are still the DS's own (block, slot and world seed), so a given DS townie is not the same person as the PS1's slot n: "agree where practical" means one algorithm and the same card text, not the same people.
- Pop-up (sign, strongest bar) and the move-in come from the same seed. A Sim who moves in keeps it in `psd[]` (saved in the new `PS` block, 24 bytes, after `TR`), and `trg_pers` uses it, so the traits match the card. A baby gets a seed mixed from its parents' seeds and the minute; children still blend their parents' bars.
- Old saves: a save without the 0x5E mark (`SavCore.opt[39]`) keeps the name-based personality for townies and for Sims who move in (`town_legacy`). A d75 save has the mark but no `PS` block: its moved-in Sims have seed 0 and keep the name-based personality they already had traits from; townies not yet moved in show the new one, and move in with it. Create-a-Sim Sims have seed 0 too.
- Wishes page: info page 3 of the Life screen (the page button now cycles 4 pages) shows any housemate's wish, its prize and progress, and how many wishes they have done. The PS1 shows a housemate's wish on the traits page only; the DS had no separate wish page for housemates (the wish picker exists only in Create-a-Sim). The pad left/right changes housemate on pages 2 and 3.
- Test: host `ds_traits_test.c` (50 worlds, 24 townies of one name: 23.92 of 24 personalities differ on average, at least 22 in each world; the five bars, sign and aspiration together have about 6,500 combinations, so a rare pair can match). melonDS, `bash ports/dsi/test/ds_pers.sh` (build `-DDSTEST -DAUTO_SCENE=155`): 24 townies with one name (ids 5 + 96 z): 24 of 24 personalities differ, checksum -1769437524; a townie moves in and the Sim's bars equal the card's (701); after save and reload the personality is the same (6875682 both). `bash ports/dsi/test/ds_oldsave.sh`: a d75-written save loads in d76 with the same trait checksum (1141753146). The D75 townie-id test (`ds_town.sh`) is unchanged.
- Shots (`docs/ds_shots/`): `traits8_player_traits.png` (the player's own traits page; the earlier shot showed Needs because the hook set no page for sim 0, scene 156 does), `traits8_housemate_wish.png` (the Wishes page, 3 done), `traits8_townie_popup.png` (a townie's card with the new personality). Scenes 156 to 161 (AUTO_SCENE=155).
- Speed (melonDS, live, street, town): 29.8 fps average (28 to 30), quads sent 2,988 to 3,101. ROM: `bore2_dsi_d76.nds`.

## D77: not a traits change
d77 fixes the music (see `docs/MUSIC_TIMING.md`, D77): d73 to d76 had no sample music because their ROMs were built with a stale ARM7. Nothing in the traits code changed. ROM: `bore2_dsi_d77.nds`.
