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
parties, STAR PUPIL, SWIMMER and so on. They come into slots 5 to 9 of the Bore and show a toast. **Prize traits** (ids 657..680, 24 made) are what some wishes give.

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

## Creator (CREATE A BORE, PERSONALITY step)

Two tabs, TRAITS and WISH, both in `traits_ui.c`, called through `cas_pick.h`.

- 3 slots, 5 for adults and elders (the last two are free to leave empty).
- TRAITS: a strip of 15 category icons (LEFT and RIGHT), a list with a name, a tick for picked and a lock for a clash, the slots, and the effect of the chosen trait in plain words.
  **SQUARE** changes the list: by category, ALL (600, a to z), PICKED, FOR YOU (traits that go with the picks), RECENT, CHEATS (10 groups, red).
  **TRIANGLE** turns on a helper: jump by first letter (UP and DOWN), then a filter by kind of effect (NEEDS, SKILLS, MOOD, SOCIAL, LOVE, WORK, MONEY, TRAVEL, WEATHER, FEARS, PETS, GAMES, FIGHT, LUCK, AGE). The first row of a list is RANDOM.
- WISH: the 61 wishes by category, X chooses, the text, the goal and the prize.
- The picks are written when the Bore is kept (`c_commit`). A new Bore is staged in `sims[MAX_SIMS - 1].tr` and `tr_take_stage()` moves it into the Bore that `start_life` adds. Re-making a living Bore rebuilds its effects.
- Dreamcast: the same file. `traits_ui` is in `OVLS`, `traits` in `GAME` (ports/dreamcast/Makefile). Nothing else changed for the port.

## The Life panel

The BORE panel has a sixth tab, **TRAITS**: the chosen traits (cheats in red), the won ones, the wish bar. X opens the page (CREATE.OVL, `open(2)`): every trait with what it does,
the three earned traits that are nearest, and the wish list with a bar for each (X chooses a new wish).

## Save

`VERSION 50` (src/save.c). After each Sim: a flag, and when it is 1 the size of `TrState` and `TrState` itself, run-length packed (`sv_rle`). After all Sims: `tr_cheat_ok`, `tr_wdone[8]`, `tr_earned`, the counters `tr_ev_a[EV_N]` and the chosen traits' names `tr_pn` (so loading needs no disc read). Older saves load with empty traits; the cheat effects follow the OPTIONS switch after every load.
`TrState` may only change together with a new `VERSION` (the size is checked; a different size fails the load).
Tested with `test/oldsave.sh` and `test/oldsave2.sh`.

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
