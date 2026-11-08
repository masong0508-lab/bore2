# TRAITS AND LIFETIME WISHES

600 regular traits, 51 earned traits, 24 prize traits, 351 cheat traits and 61 lifetime wishes. All of them are data made by one script and run by one small engine.
This page is for whoever works on the engine, the lists or a port. Everything here is portable: the PS1 and the Dreamcast build the same files.

## The idea in four lines

- A trait is a short list of effects. An effect is a pair: a **hook** (a number in the game that can be changed) and a **tier** (how much).
- Every Bore has `TrState tr` (src/traits.h, inside `Sim`). `tr.fx[hook]` is the sum of all its effects. Reading it costs one array read.
- The game asks the hook where the number is used. One line, for example `pay(x)` becomes `tr_scale(&s->tr, E_SHOP_DISC, x)`. Nothing loops over the traits while the game runs.
- Because every effect is a hook and every hook is read by the game, a trait with an effect always does something real. `test/traits_test.sh` proves it (see TEST).

## Files

| file | what |
|---|---|
| `tools/make_traits.py` | the generator: the hooks, the names, the categories, the earned traits, the wishes. Run it; it writes the two files below. |
| `src/traits_data.h` | made. Small tables for the game (resident). With `#define TR_BIG`: the big tables (effects, names, texts) for CREATE.OVL and the test. |
| `docs/traits.json` | made. The same data as text, for tools and ports. |
| `src/traits.h`, `src/traits.c` | the engine. Plain C, no game headers: the host test compiles `traits.c` itself. |
| `src/traits_glue.inc` | included into `sim.c`. The parts that need the game: need falls, mood, the skill / cash / need watchers, wish prizes. |
| `src/traits_auto.c` | the personality -> traits mapping (`tr_auto_pick`), the clash rule, `tr_rebuild`. Plain C like traits.c; the host test runs it. In CREATE.OVL (PS1) and `OVLS` (Dreamcast). |
| `src/traits_ui.c` | the pickers and the Life panel page. Linked into CREATE.OVL (PS1) and the game (Dreamcast, `OVLS`). |
| `src/cas_pick.h` | the hook between CREATE.OVL's PERSONALITY step and `traits_ui.c`. |
| `test/traits_test.c`, `test/traits_test.sh` | the host test. |

To change a list, edit `tools/make_traits.py` and run `python3 tools/make_traits.py`. Never edit the made files.

## Hooks (107)

`TrState.fx[]` is indexed by the enum `E_*` in traits_data.h. A hook has a **mode**:

| mode | meaning | read with |
|---|---|---|
| PCT | a percent change of a rate or an amount (tier * unit %) | `tr_scale(t, E_X, v)` or `tr_pct(t, E_X)` |
| ADD | flat points or levels (mood, charm, belt) | `tr_add(t, E_X)` |
| CHN | a chance in percent (luck) | `tr_chance(t, E_X, base)` |
| FLR | the lowest the mood may fall | `tr_floor(t)` |

Families are one hook per need, skill or career: `E_DEC0 + need` (the need falls slower or faster), `E_GAIN0 + need` (what actions put back), `E_SK0 + skill`
(skill gain), `E_PAY0 + career` (pay of one career). A tier is small (2, 3, 4, 6 for regular traits; the cheats are -10, +120 and so on). `amount = tier * unit`.

Where they are read (all in the existing code, no new loops):

- **Needs**: `need_fall()` (E_DEC_*, with the remainder carried so 3 points a step can still change by 10%), `need_gains()` (E_GAIN_*, E_ACT_*, from a snapshot taken in `run_action`).
- **Skills**: `tr_watch()` every 8th step compares each skill counter with the last one and adds or takes the difference (E_SK_*).
- **Money**: `tr_cash_watch()` every step: income and spending are scaled (E_INCOME, E_SPEND); `pay()` takes E_SHOP_DISC; buses and taxis E_FARE.
- **Mood**: `trait_mood()` every 16th step (flat, night, outdoors, rain, cold, heat, crowd, alone, fears, pets), E_MOOD_PULL, E_MOOD_MIN (`tr_floor`), moodlets E_ML_GOOD and E_ML_BAD.
- **Social and love**: `social()` chances (E_CHAT_OK, E_JOKE_OK, E_HUG_OK, E_FLIRT_OK, E_REL_GAIN, E_REL_LOSS, E_ROMANCE), E_REL_DECAY, E_ENGAGE, E_CHARM.
- **Work**: pay (E_JOB_PAY and the 15 `E_PAY_*`), promotions, strikes, performance.
- **Moving**: E_WALK_SPEED, E_VEH_SPEED, E_FUEL_USE, E_AGE_SPEED (days to grow up).
- **Fights, arcade, Boreagotchi, pets**: citizens.c (E_FIGHT_*, E_LOOT), arc_core.c (E_ARC_SCORE, E_ARC_TIX), gotchi (E_GOTCHI_XP, E_GOTCHI_GIFT), E_PET_FOOD.
- **School and wants**: E_GRADE, E_ASP_GAIN, E_ASP_KEEP. **Luck**: E_LUCK, E_SPARK.

The made list with all 107 is in `docs/traits.json` (`hooks`: code, category, group, mode, unit, the up and down texts, the regular and cheat tiers).

## The lists

Regular traits (600) have an id 1..600, a name (up to 14 letters, all different), a category, a group and one to three effects. Some are plain boosts, some come with a cost or a quirk
(so a combination has to make sense). Two traits **clash** when they share a main hook, or when one raises what the other lowers. The picker greys a clashing trait and says which pick it clashes with.

| category | traits | groups |
|---|---|---|
| BODY | 65 | bladder, sleep, hygiene, fitness, ageing |
| MIND | 57 | learning, mood, nerves, school, focus |
| SOCIAL | 70 | chat, friends, charm, crowds |
| ROMANCE | 36 | |
| WORK | 110 | the career pays, promotions, performance |
| MONEY | 20 | |
| FOOD | 30 | |
| HOME | 34 | |
| OUTDOORS | 52 | |
| CREATIVE | 12 | |
| FUN | 34 | |
| NIGHT AND DAY | 25 | |
| PETS | 20 | |
| LUCK | 15 | |
| FIGHTING | 20 | |
| EARNED | 51 + 24 prizes | |

(The full list of groups per category: `categories` in traits.json.)

**Earned traits** (ids 601..656, 51 made): won by doing something many times. Every one has a counter (`EV_*`) and a count: HOME COOK after 25 meals, FOOD CRITIC after 20 meals out, PARTY ANIMAL after
parties, STAR PUPIL, SWIMMER and so on. They come into the first open slot (or wait for one, see Slots) and show a toast. **Prize traits** (ids 657..680, 24 made) are what some wishes give.

**Cheat traits** (ids 701..1051, 351 made): 3 strengths for each of the 107 hooks (SUPER, MEGA, GOD) and 30 mixed ones (IMMORTAL, MIDAS TOUCH, TOTAL CHEAT). They have their own groups
(NEEDS, SKILLS, MONEY, WORK, SOCIAL, MOOD, TRAVEL, GAMES, LIFE, MIXES). A cheat trait's effects are kept apart (`TrState.ch/cm`) so the OPTIONS switch can take them out of `fx` and put them back.

**Lifetime wishes** (61): a name, a short text, a category, a counter, a goal and a prize. The prize is one of: cash, a prize trait, an outfit, groceries, a bus pass, arcade tokens, wants points.
A wish counts from the moment it is chosen (`wbase`), is done once (`tr_wdone`, one bit each, saved) and then the next wish waits. The Life panel shows its bar.

## Counters (`EV_*`)

55 counters, a `u8` per Bore (they stop at 255). Most count events the game raises: `tr_ev(&s->tr, EV_COOKED, 1)` at the end of the action. Some are read off the game every 8 steps by `tr_watch()`
(job level, money in hand, friends, homes owned, skill levels). The first time that happens in a life they are taken quietly, so what a life starts with is not "won".

## Cheats are fenced off

- OPTIONS, GAME page: **Cheat traits: Off / On**. Off is the default. The first time it is switched on a card asks (X yes, O no); the answer is saved (`tr_cheat_ok`).
- With the switch off, cheat traits do nothing and the picker's CHEATS list says how to switch them on.
- Random never picks one.
- While a Bore with a cheat trait is played (`tr_cheating()`), `medal()` and `hs_put()` in arc_core.c give nothing. Add the same one-line check to any new award, high score, medal or leaderboard.

## Slots (6 + 4)

| slots | who | rule |
|---|---|---|
| 1 to 6 | every grown-up (ADULT, ELDER) | always open. BABY has 1, CHILD 3, TEEN 4 (`tr_stage_slots`, `tr_nslots(stage)`). |
| 7 to 10 | TEEN and up | open one after the other with real progress, **for the whole household** (`tr_unl`, 0 to 4, saved) |

Any kind of trait goes in any open slot: picked in the creator, won (earned), or a wish prize. A won trait takes the first open slot; if none is open it waits (`tr_wait`, saved) and comes in when a slot opens or is freed (`tr_regive`, tried when a slot unlocks and every few seconds).
A won trait stays for good; a picked one can be taken off in the creator.

**Unlock conditions** (in this order; `UNLOCKS` in make_traits.py, checked every 8th step for the player, `tr_unl_check`):

| slot | condition | text on the locked slot |
|---|---|---|
| 7 | 1 lifetime wish done | FINISH 1 WISH |
| 8 | 3 wishes done | FINISH 3 WISHES |
| 9 | job level 5 (the top) | REACH JOB LEVEL 5 |
| 10 | 10 wishes done | FINISH 10 WISHES |

A locked slot shows its text (creator PICKED list, Life panel, page). Opening one shows the toast `TRAIT SLOT n UNLOCKED!` and marks every assigned member to get the new slot filled.
**Cheat traits** use slots 7 to 10 only, and only when OPTIONS > CHEAT TRAITS is on (they are never put in 1 to 6). So a new life cannot use cheats until slot 7 is open.

## Family: traits from the personality

Only the player and a Bore made in the creator are picked by hand. Everybody else in the household (someone from town who moves in, a spouse, a roommate, an adopted child, a baby that is born, the first family) gets traits **automatically** from the personality they already have.
All of them come through `sim_add`, so one watcher covers every path: every 8th step `tr_scan` (traits_glue.inc) gives a Bore with no state one (`TF_AUTO | TF_PEND`), notices a new life stage (a birthday) and a new unlocked slot. When something is pending, `main.c` loads CREATE.OVL and calls `open(3)` (`tr_auto_run`, traits_ui.c), which fills the slots and leaves (one short load, like opening a page; also run when the Life panel opens). The toast says `NAME GOT 6 TRAITS`.

- **Editable?** No. Assigned traits are shown (Life panel TRAITS tab, the page with LEFT and RIGHT for each housemate) but not edited. A creator re-make of such a Bore throws the traits away and assigns them again from the (maybe changed) personality.
- **Effects**: same hooks, `Sim.tr` of that Bore. They have no counters, no wish and win no earned traits (`TF_AUTO`).
- **Deterministic**: the same person (name, bars, sign, aspiration, stage) always gives the same traits; nothing but the ids is stored, and `tr_rebuild` makes the effects from them (so the save only keeps ids and names).
- **Growing up**: at a birthday the slots grow; `tr_auto_pick` keeps what they have and fills the new slots. A new unlocked slot does the same for the grown-ups.
- **Children and babies** take after the parents who live in the household: their bars are (2 x own + both parents) / 4, and up to 2 of the parents' regular traits get +40 so they are likely to be chosen.
- **Residents** of town who never move in keep no trait state.

### The mapping (`tr_auto_pick`, data in `docs/traits.json` > `personality`)

Inputs (all on the `Bore`): the 5 bars NEAT, OUTGOING, ACTIVE, PLAYFUL, NICE (0 to 10, 5 is the middle), the star sign (`look_sign_of`), the aspiration, the life stage, and the id seed = FNV hash of first and last name.
For each of the 15 categories:

`category score = sum(pull[cat][bar] x (bar - 5)) + 4 x aspiration bonus[cat] + 3 x sign element bonus[cat]`

| category | NEAT | OUTGOING | ACTIVE | PLAYFUL | NICE |
|---|---|---|---|---|---|
| BODY | 1 | 0 | 4 | 0 | 0 |
| MIND | 1 | -2 | -1 | 0 | 0 |
| SOCIAL | 0 | 3 | 0 | 1 | 2 |
| ROMANCE | 0 | 2 | 0 | 1 | 2 |
| WORK | 2 | 0 | 1 | -2 | 0 |
| MONEY | 1 | 0 | 0 | -1 | -2 |
| FOOD | 0 | 0 | -1 | 1 | 1 |
| HOME | 3 | -2 | -1 | 0 | 1 |
| OUTDOORS | -1 | 1 | 3 | 1 | 0 |
| CREATIVE | -1 | 0 | -1 | 2 | 0 |
| FUN | -1 | 1 | 0 | 3 | 0 |
| NIGHT AND DAY | -1 | 2 | 0 | 1 | 0 |
| PETS | 0 | 0 | 0 | 1 | 3 |
| LUCK | -1 | 0 | 0 | 2 | 1 |
| FIGHTING | 0 | 0 | 3 | 0 | -3 |

Aspiration bonus: FORTUNE MONEY 3, WORK 2, LUCK 1; KNOW MIND 3, WORK 2, CREATIVE 1; POP SOCIAL 3, FUN 2, NIGHT 2; PLEASURE FOOD 3, FUN 2, ROMANCE 2, NIGHT 1; HOME HOME 3, FOOD 2, PETS 2, ROMANCE 1; GROW UP MIND 2, BODY 1, FUN 1.
Sign element (sign index mod 4): fire (ARIES LEO SAGITTARIUS) FIGHTING 2, OUTDOORS 2, BODY 1; earth (TAURUS VIRGO CAPRICORN) MONEY 2, HOME 2, WORK 1; air (GEMINI LIBRA AQUARIUS) SOCIAL 2, CREATIVE 2, MIND 1; water (CANCER SCORPIO PISCES) ROMANCE 2, PETS 1, CREATIVE 1, LUCK 1.

`trait score = 4 x category score + jitter(seed, trait id) (0 to 23)`, +40 if a parent has it. Regular traits are all candidates; earned and prize traits are candidates at `2 x category score` (category of their first effect) and only when that is 60 or more (a strong personality). Cheats never.
Then, for each open slot in order, the best candidate that **does not clash** with anything already in (the creator's clash rule: same main hook, or one raises what the other lowers), is not a duplicate, and keeps the category at 3 or less is taken. So a very sociable person gets SOCIAL, ROMANCE and NIGHT traits, an active one BODY, OUTDOORS and FIGHTING, a neat homebody HOME, WORK and MIND (test: 96%, 96%, 87% of the strong cases).

## Creator (CREATE A BORE, PERSONALITY step)

Two tabs, TRAITS and WISH, both in `traits_ui.c`, called through `cas_pick.h`.

- Slots: 6 for adults and elders, 4 for teens, 3 for children, 1 for babies, plus the unlocked 7 to 10 (see Slots). The PICKED list (SQUARE) shows all ten slots: a trait, EMPTY, or the shut slot with its condition. X on a slot takes its trait off.
- TRAITS: a strip of 15 category icons (LEFT and RIGHT), a list with a name, a tick for picked and a lock for a clash, the slots, and the effect of the chosen trait in plain words.
  **SQUARE** changes the list: by category, ALL (600, a to z), PICKED, FOR YOU (traits that go with the picks), RECENT, CHEATS (10 groups, red).
  **TRIANGLE** turns on a helper: jump by first letter (UP and DOWN), then a filter by kind of effect (NEEDS, SKILLS, MOOD, SOCIAL, LOVE, WORK, MONEY, TRAVEL, WEATHER, FEARS, PETS, GAMES, FIGHT, LUCK, AGE). The first row of a list is RANDOM.
- WISH: the 61 wishes by category, X chooses, the text, the goal and the prize.
- The picks are written when the Bore is kept (`c_commit`). A new Bore is staged in `sims[MAX_SIMS - 1].tr` and `tr_take_stage()` moves it into the Bore that `start_life` adds. Re-making a living Bore rebuilds its effects.
- Dreamcast: the same file. `traits_ui` is in `OVLS`, `traits` in `GAME` (ports/dreamcast/Makefile). Nothing else changed for the port.

## The Life panel

The BORE panel has a sixth tab, **TRAITS**: the ten slots in two columns (a trait, OPEN SLOT, or a lock with the condition text) and the wish bar. X opens the page (CREATE.OVL, `open(2)`): the ten slots with what each trait does,
the three earned traits that are nearest, and the wish list with a bar for each (X chooses a new wish). LEFT and RIGHT on the page show each housemate: their slots, their traits, their star sign and strongest bars (no wish and no earned traits for them).

## Save

`VERSION 51` (src/save.c; 50 loads: see below). After each Sim: a flag, and when it is 1 the size of `TrState` and `TrState` itself, run-length packed (`sv_rle`), then the ten slot names (so loading needs no disc read). After all Sims: `tr_cheat_ok`, `tr_wdone[8]`, `tr_earned`, the counters `tr_ev_a[EV_N]`, `tr_unl` and `tr_wait[10]`. Older saves load with empty traits; the cheat effects follow the OPTIONS switch after every load.
**Version 50 saves** (slots 5+5, only 2 Bores with state): the states load as they are (a Bore's old picks and won traits keep their slots; slots 7 to 10 needed by a won trait are opened), the five names go to the player, the unlocks are computed from the wishes done and the job level on the first look, and every other Bore gets assigned traits within a second of the first frame. `TrState` did not change size.
A member's `TrState.f`: `TF_AUTO` assigned, `TF_PEND` assign again, bits 4 to 6 the life stage the slots follow.
`TrState` may only change together with a new `VERSION` (the size is checked; a different size fails the load).
Tested with `test/oldsave.sh` and `test/oldsave2.sh` (`"50 59b5752"` makes a version 50 save).

## TEST

`sh test/traits_test.sh` (needs gcc and python3, no console, runs in a second).

1. Runs `tools/make_traits.py`.
2. Compiles `src/traits.c` (the game's own file) with `test/traits_test.c` for the host.
3. For every regular, earned, prize and cheat trait: has a name, has an effect, the name is unique; applies it to a fresh Bore and asks the **same accessor the game uses** (`tr_scale`, `tr_add`, `tr_chance`, `tr_floor`)
   whether the targeted number moved from the baseline.
4. For every earned trait: counts up to the goal through `tr_ev` and checks it is granted and changes a number.
5. For every wish: has a counter, a goal and a prize; not done one short; done at the goal, once; the prize function is called once.
6. The cheat switch: effect on, off (back to baseline), on again.
7. Greps that every hook (or its family base) is read in the game source outside the traits files.

Result (this build): PASS, 0 failures, 0 hooks not wired.

| group | traits | pass |
|---|---|---|
| regular | 600 | 600 |
| earned | 51 | 51 |
| prizes | 24 | 24 |
| cheat | 351 | 351 |
| earn-run | 51 | 51 |
| wishes | 61 | 61 |

## RAM (PS1, 2 MB)

- Only a Bore made in the creator (and the player) has a `TrState`: `Sim.tr` is a pointer into `tr_pool[2]`, or at `tr_null` (zero, never written; every writer checks it). The counters, wish bits and names are one family-wide set (`tr_ev_a`, `tr_wdone`, `tr_pn`), so a family holds at most 2 Bores with traits.
- The earned and prize trait names are not resident; they are in CREATE.OVL's big table.
- Free past `_end`: 32,112 bytes after the wallpapers moved to WALLS.BIN (8,960 right after the engine landed). `origin/main` alone had 4,544 (a build with under about 5.5 KB hangs at boot, see docs/PS1_PERFORMANCE.md). The room came from `-mno-check-zero-division` on the main program (CMakeLists.txt: no divide-by-zero trap code at each `/` and `%`; 11.6 KB, and a little faster). That is the only build flag changed.
- Below the 10 KB rule by about 1 KB. Further room if wanted: `citizens.c` built -Os (3.2 KB, costs crowd speed), or the earned/wish check moved into an overlay (about 2.7 KB, but then awards only show when the page is opened).

## Shots

`docs/traits_cat_*.png` (every category, PS1), `traits_all/picked/foryou/recent.png` (views), `traits_cheats.png`, `traits_wishes.png`, `traits_wishes_body.png`, `traits_life.png` (Life panel), `traits_page.png`, `traits_page_wish.png`, `traits_dc.png` (Dreamcast).

## Adding things

- A trait: add a line to a list in `make_traits.py` (hooks have the nouns and adjectives the names are made from). Run it. The test covers it.
- A hook: add a line to `HOOKS_TXT`, read it in the game with the right accessor, run the test (it fails until the game reads it).
- A wish: add a line to `WISH_TXT`; its counter must be an `EV_*` in `EVENTS`, raised with `tr_ev` or polled in `tr_watch`.
- Never change the order of `E_*` or `EV_*` without a new save version: the effects are saved as numbers.
