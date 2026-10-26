# How SAN BOREAS lives and breathes on the PS1 (and how to carry it to the DSi)

A guide for steering the port. Part A is what the PS1 build does today, system by system, with the files to read.
Part B is how each piece could translate to the DSi, and the questions where I need your call.
(Everything in A is read from the source in `src/`; numbers are the real constants.)

---------------------------------------------------------------------------------------------------

## PART A: THE PS1 WORLD AS IT IS NOW

### A0. The big idea: nothing is stored, everything is worked out
- A new life rolls ONE seed. The whole state ("SAN BOREAS") is *calculated* from it when needed: land, towns, streets, lots, people.
- Only what **changed** is saved (renamed towns, edited blocks, homes you built, people who died or moved, etc.).
- This is why 390 km² and ~3,700 people fit on a PS1 with a memory card. The DSi port should keep this rule.

### A1. The land (`bigmap.c/.h`, already shared with the DSi port)
- 1024 x 1024 **blocks**, each block 18 tiles (a tile is about 1.2 m). Terrain per block: sea, beach, grass, forest, desert, rocky; hills via `bm_height`.
- **Settlements**: capital BORE CITY plus cities, towns, villages (defaults 2 cities, 6 towns, 12 villages, up to 62 sets). Each has a name, a centre, a radius, a kind (city/town/village/spaceport).
- **Highways** link settlements; **streets** run along the top and left of each block (road bits 1, 2, 4).
- **Lots** (what stands on a block, 35 kinds): homes (house, cottage, townhouse, two-storey, apartment block), 19+ places (shop, diner, gym, office, inn, pool, school, arcade, tower, hotel, cafe, bar, cinema, salon, restaurant, clinic, library, spa, gallery, gas, convenience), parks, squares, skate park, farm, empty lot, river, space centre, observatory. **Mega blocks** (2x2): big park, mall, plaza, business campus.
- Names come from hashes: "214 MAPLE ST", "BOREMART", "THE SLEEPY INN".
- **Dials** when creating a world: land shape (mainland, islands, big isle, continent, lakes), sea level, climate (desert to jungle), hills, how many cities/towns/villages, capital size, ROOMY vs PACKED towns.
- **Neighbourhood editor** overrides (ground, lot, road, territory, elevation) are kept across new worlds. (DSi: the map editor already uses this API.)

### A2. The people (`pop.c/.h`): 874 households, ~3,700 individuals
- Every home that is *lived in* has a **household** (`Fam`: block, unit, town, kind, surname, first person, count). A shuffled list: the first 874 homes are occupied, the rest stand empty (a third of the capital's houses are held back as homes for sale).
- Household kinds: SINGLE, OLD TIMER, COUPLE, RETIRED COUPLE, ROOMMATES, YOUNG FAMILY, BIG FAMILY, SINGLE PARENT, THREE GENERATIONS. Dials: PEOPLE (few to packed = up to 2x) and HOUSEHOLD SIZE.
- Every person (`Per`, 12 bytes): seed (name, face, body all come from it), job (a place index or none), life stage (baby, child, teen, adult, elder), trait (16: cheerful, grumpy, lazy, active, creative, geek, romantic, foodie, loner, outgoing, neat, slob, goofball, bookworm, music lover, family-oriented), flags (homebody, gone, days in stage), slot (the citizen who plays them right now, 255 = nobody), rel (what they think of you), rom (romance), hp.
- **Jobs**: adults 86% employed at a work place in their own town; elders 15%; children and teens go to the town's school. Roughly half the unemployed adults and half the elders are homebodies; loners often stay in.
- **Names**: 128 first names + generated names; elders mostly get old-fashioned ones; surnames from the family number (thousands).

### A3. Where is everyone right now? (`pop_where`): a daily schedule from the clock
Nobody is simulated while you are away. Their location is **computed from the time**:
1. **Asleep** (kids 20:30-7:00, teens 23:00-7:00, adults 22:30-6:40, elders 21:30-6:30), at home.
2. Babies are always home.
3. Children/teens at **school** Mon-Fri 8:00-15:00.
4. Adults at **work** during their shift: offices/schools Mon-Fri 9-17 (school 8-16); arcade 14-23; pool 10-18; inn/hotel/clinic 7-15 or 15-23 (early/late, 2 days off); others per `work_hours`.
5. Homebodies stay home.
6. **Leisure**: a couple of hours, chance by age and trait (elders 30%, adults 40%, kids 60%, outgoing +25, lazy/loner -15), to a place in their own town (park, diner, pool, gym...). Elders 9:00-17:00, kids 10:00-19:30, adults 17:00-22:00 (weekend adults from 10:00).
7. Otherwise at home.
Each person has a **time offset** of -30..+30 minutes so a household never moves as one.
`pop_doing()` turns it into text: "ASLEEP AT HOME", "AT WORK: BOREMART", "AT ELM PARK". `pop_job_name()`: "COOK AT THE DIPPER DINER", "RETIRED", "STUDENT, ...".

### A4. Bringing people to life (`pop_tick` + `citizens.c`)
- Only the **map window** (5x5 blocks around you) is lived in. Each frame `pop_tick` checks 40 people: is their current location inside the window? If yes and nobody plays them, a **citizen** is *taken* (`citizens_take`); if they have left, the citizen is *released* and walks off to the edge (`CM_AWAY`) and is gone when out of sight.
- **128 citizen slots**: 0-9 the island, 10-49 the home town, 50+ SAN BOREAS (given to whoever of the 3,700 is near).
- **Two levels of detail**: cheap crowd models (a 2 KB compact copy per person in `crowd.c`'s arena), the few nearest you (< 5 tiles, within camera range) promoted to the full model.
- **Modes** (`CM_`): idle, walk, chat, chase, KO, gone, sleep, go home, leave, **chore** (at home, they use their own furniture), **visit** (knock on a neighbour's door), party, stop (peacemaker), away.
- **Personality** (4): friendly, gruff, flirty, shy; plus the person's trait. They remember **rel** (friendship, -100..100) and **rom** (romance 0..100) about you.
- Daily behaviours: wake, do chores (fridge, TV, bed...), wander, go to work (shop clerks stand behind counters, kids at desks), come home, visit neighbours 9:00-21:00, sleep at night.

### A5. Talking to citizens (the pie menu)
- TALK; CHAT (joke, ask about job, gossip, ask advice, compliment); FRIENDLY (hug at rel 25+, high five, dance, buy drink $10, gift $20, say sorry, **ask to move in** at rel 60 with adults); ROMANCE (flirt, flowers $40, date $60, kiss, **propose $150**); MEAN (insult, prank, fight); FAMILY (their household).
- Outcomes roll dice from mood, rel, personality; they change rel/rom and give moodlets (good chat, embarrassed, rejected, hugged, laugh, flirty).
- Fights: citizens fight back, bystanders react, **peacemakers** step in, **police** (cop citizens) are called, heat rises.
- Friends count feeds story goals ("make a friend").

### A6. Places, hours and jobs (`places.c`)
- Each place type has **opening hours** (e.g. "OPEN 8 AM - 10 PM", "MON - FRI 8 AM - 6 PM"); `place_open(lot, minute, day)`.
- The pie menu at counters sells what the place sells: a meal, latte, drink, film, haircut, massage, checkup, swim, hotel room, groceries, an outfit.
- **14 careers** (culinary/food, retail, fitness, swim, hotel, games, business, teaching, nightlife, medical, film, beauty, art, factory) at real places; each has shift hours, days, pay (level 0 + per level), station, six titles ("DISHWASHER" to "RESTAURATEUR", "INTERN" to "CEO", "ORDERLY" to "MEDICAL LEGEND"). The boss pays the taxi. Promotion needs skills, friends and mood.

### A7. Homes (`home.c`)
- THE MAN BASE is the starting home. Later you can **buy** a house, cottage or two-storey nobody lives in (or an empty lot to build on) anywhere in a town: price depends on kind and city. The family moves in by taxi, it comes furnished, that block is yours (saved just as you build it, every storey). Buying another sells the old one at what you paid.
- Your own Bores can **move out** to a home of their own in town (`pop_move_out`) and then live as townies with their own looks and names saved.

### A8. Growing up, all over the state (`pop_midnight`)
- Every midnight, *everyone* in SAN BOREAS is a day older: babies, children, teens, adults, elders each have a number of days (the LIFE option: off, short, normal, long).
- At the end of a stage they move up: childhood to teen to adult (half of new adults leave home if two adults stay), then new school/job assigned; elders **pass away** (grave stone at home; ghosts rise at night in your own family).
- Empty homes get a **new household moving in** (a quarter of nights per empty home); couples with a free slot may have **a baby** (1 in 12 nightly).
- The window of text for each person: age stage, job, family, "MOVED AWAY"/"PASSED AWAY".

### A9. Cars (`traffic.c`)
- Up to 120 cars hop **tile to tile in their own lane** (drive on the right), turn at crossings, queue, stop for you. Traffic level 0-4 (0, 3, 6, 11, 18 cars; cheat 120). Parked vehicles you leave stay where you left them. Positions are in whole-world tiles so sliding the window changes nothing.

### A10. Needs, wants, moods (`sim.c`, `wants.c`)
- Needs (hunger, fun, energy, social, bladder, hygiene...). Moodlets stack for a mood number.
- **Wants and fears** (34 wants, 7 fears) chosen by a 6-way **aspiration** (Fortune, Knowledge, Popularity, Pleasure, Home, Grow Up) via weight tables; points fill the aspiration meter (Sims 2 style).
- Skills (cooking, body, fun) levelled 0-10.

### A11. Crowds, parties, ghosts, island
- **Party** (the phone): up to 60 guests from households you know (friends list), they walk in, dance, leave after 3 hours; the room's party score matters.
- **Ghosts** of dead family members at night. **Castaway Island**: separate zone with its own islander citizens (slots 0-9) and survival loop.

### A12. Saves
- World from seed; `pop_save` keeps **row-compressed byte planes** of what changed (stage, flags, rel, rom, hp) plus regenerated people. Edited blocks and homes saved separately (`bm_ovr_*`, `world.c`).

### Quick numbers
| What | Number |
|---|---|
| Blocks | 1024 x 1024 (18 tiles each) |
| Settlements | up to 62 (cities 8, towns 20, villages 32 max) |
| Households | 874 normal (up to 1748) |
| People | up to 3,700 (12 bytes each ~ 44 KB) |
| Places (jobs, leisure) | up to 1,800 |
| Citizens alive at once | 128 slots (about 40 in a town window) |
| Cars | up to 120 |
| Careers | 14 x 6 titles |
| Traits / personalities | 16 / 4 |
| Wants / fears / aspirations | 34 / 7 / 6 |
| Party | up to 60 guests |

---------------------------------------------------------------------------------------------------

## PART B: TRANSLATING IT TO THE DSi PORT

### B0. What the DSi port already has
`bigmap.c` is shared, so **the same SAN BOREAS** exists: map screen, neighbourhood editor, lot kinds, names. In-lot: 3 sims with needs, wandering, path-finding, buy/build. Missing: people as a *population*, schedules, places, jobs, other lots, cars, ageing.

### B1. Principles (keep the PS1's rule: compute, don't store)
- Reuse **`pop.c` logic as-is**. It is plain C and small: people (44 KB) + households + places fit easily in DSi RAM (16 MB; even 4 MB on a plain DS).
- **The clock decides where people are** (`pop_where`): no per-frame simulation of far people. Only people *in the lot or on the street you are at* become actors.
- Save = seed + diffs, on the DSi SD card (libfat) or cartridge save.

### B2. A staged plan (each stage playable)
**Stage 1: "Neighbours" (data, no 3D yet).** Link `pop.c` into the DSi build (needs stubs for `sim.h`/`world.h`; I already have a host shim in `ports/host`). Add a **Neighborhood screen**: tap a house on the map, see the household ("THE OKAFOR FAMILY: YOUNG FAMILY"), each person (name, age stage, trait, job) and "doing: ASLEEP AT HOME" using the live clock. Add a **people directory** (who is out right now). This alone makes the map feel alive.
**Stage 2: "Living lots".** Entering a lot with residents: spawn its household as *actors* (our `Sim` struct extended with stage/trait/seed), schedule-driven: sleeping in bed at night, chores in the day, leaving through the front door to go to work. Our path-finding and door system are ready for it.
**Stage 3: "Places".** Lot generators for 4 place types first (shop, diner, park, gym), each with its own layout like the house generator, counters, opening hours, staff from `pop_place` workers, and the buy menus (meals, groceries, haircut). Then the rest (office, school, cinema...).
**Stage 4: "Going out".** Your Sim travels (bus/taxi/walk on the map) to other lots; time passes; the world updates.
**Stage 5: "Talking".** Port the pie menu to a **touch menu** (a big dial or list on the bottom screen): same options, same rel/rom dice. Relationships list screen (friends, rom).
**Stage 6: "Jobs and money".** Careers table straight from `places.c`: shift timer, taxi, pay, promotion, bills.
**Stage 7: "Time passes for everyone".** `pop_midnight` runs when the day changes: babies, birthdays, deaths, new families moving in. A "Town news" screen reports it (born, married, passed away).
**Stage 8: "Streets".** Street view outside your lot: low-poly cars on the lanes (traffic.c idea with ~8 cars), a few walkers, neighbour houses as blocks.
**Stage 9: "Big extras".** Parties, ghosts, the island, fights/cops (only if wanted).

### B3. The DS-specific problems to solve
| Problem | PS1 | DSi answer |
|---|---|---|
| Polygon budget (~1500 quads) | ~120 full people possible via sprites | Only 8-10 actors in a lot; people elsewhere as **text and icons**; far crowds as flat sprites later |
| Screens | one | Top = world, bottom = touch UI (we already do this) |
| Memory | 2 MB + card | DSi 16 MB; keep world calculated |
| Speed | 33 MHz | arm9 66 MHz (133 MHz on DSi with the turbo option) |
| Input | pad pie menu | stylus lists; large touch targets |
| Models | full Bore | the blocky sim with sliders: each citizen's `seed` can drive the same sliders (face/body/skin/hair) |

### B4. Decisions I need from you (these steer the order)
1. **Scope of "the city":** do you want to *walk around* streets and neighbourhoods in 3D (Stage 8 early), or is **lot-to-lot travel through the map** (like Sims Bustin' Out on DS) enough for now?
2. **Who first:** the player's household only (3 sims) + neighbours, or the whole 3,700 population from the start?
3. **Which places** matter most to you (shop, diner, park, gym, cinema, school, office...)? I will do 4 first.
4. **Ageing:** keep full life stages (baby to elder with growth and deaths) on DS, or a lighter version?
5. **Talking style:** full PS1 social menu or a Sims-DS-style radial/short list?
6. **Saving:** SD card file (DSi only, large) or cartridge save (works on flashcarts)?
7. **Island and mini-games** (arcade, skating, fight, farm, heist, gotchi): keep for last or drop?
8. **Cars and crowds:** cosmetic only (low priority) or interactive (taxis, driving)?

### B5. Suggested next step
Do **Stage 1** now: link `pop.c`, show households and "what are they doing right now" on the map's bottom screen, tied to the game clock. It is small, it uses real PS1 data, and it makes every later stage easier to review. Tell me which of the decisions above to change.
