# BORE 2 — what it is to be the game

A design bible, written after reading through The Sims 2 and its console and handheld relatives
(The Sims 2 PC, The Sims 2 console, The Sims 2 GBA, The Sims 2 DS, The Sims Bustin' Out console and
handheld, The Sims 2 Castaway). BORE 2 is not a copy of any of them. It is BORE (the GBA voxel creature
game) grown up into the kind of game those were, in full 3D on the PlayStation. This file says which
pieces of each game BORE 2 takes, why, and how they map onto the code. Read it before adding anything.

## 1. The identity, in one line

**You directly control a little voxel person you made, in a small world full of other people, and the game
tells you what that person wants — and what they are afraid of.** Everything else serves that.

## 2. The four parents, and what each one gives BORE 2

### The Sims 2 (PC) — the HEART: wants, fears, the aspiration meter
This is the thing that made The Sims 2 feel alive, and it is what BORE 2 was missing.
- Every Sim has an **aspiration** (BORE already has six: FORTUNE, KNOWLEDGE, POPULARITY, PLEASURE, HOME,
  GROW UP — the Sims 2's Fortune, Knowledge, Popularity, Pleasure, Family and Grow Up).
- The aspiration decides the Sim's **wants** and **fears**: 4 wants and 3 fears on screen at once (BORE 2
  shows 4 and 2, the screen is 320 wide). Fulfilling a want pays **aspiration points**, a fear coming true
  takes them away. Small wants ~500, medium ~1500, big 3000–5000; fears −500 to −3000.
- Wants re-roll when: a want is fulfilled; the Sim wakes from a real sleep; the Sim comes home from another
  lot; the household changes. A want you ignore long enough is forgotten and replaced.
- Wants come from three places: the aspiration (most of them), the life stage (kids want to play and
  study, elders want to retire), and the personality (Playful Sims want fun, Outgoing Sims want to chat,
  Neat Sims want a shower, Active Sims want to run).
- Want paths: fulfil "cook skill 1" and it becomes "cook skill 2", and so on.
- The **aspiration meter**: red → green → gold → platinum. **Platinum = mood locked at full**, nothing
  can bring it down, promotions are guaranteed if the skills are there. It decays: platinum → gold in 8
  hours, gold → green in 16, green → red in 64, red → bottom in 32. Only fears can push it to the bottom.
- **Aspiration failure**: at the bottom the Sim acts out (annoys others, jumps on the couch, kicks the
  bin, tantrums), and each aspiration has its own breakdown (the Family Sim cuddles a flour sack, the
  Fortune Sim begs). Growing up in red = growing up badly.
- **Relationships** are two numbers per pair: daily (decays 2 a day) and lifetime (drifts toward daily).
  Friend = daily ≥ 50 both ways; best friend = lifetime ≥ 50; crush = daily ≥ 70 romantic; love = lifetime
  ≥ 70; enemy ≤ −50. Time together does nothing — only interactions move them.
- **Needs**: Hunger, Bladder, Energy, Hygiene, Comfort, Fun, Social, Environment. Mood comes from them and
  gates everything: skill learning, promotions, which social options exist (a bad mood unlocks "Tease").
  Failures: wet yourself (and hygiene crashes), pass out where you stand, stink clouds and rejection,
  the Social Bunny, starvation.
- **Memories** of big events (first kiss, promotion, a death) stick to the Sim and come up in gossip.
- **Careers**: promotion needs skills, friends AND a good mood; chance cards gamble the job.

### The Sims Bustin' Out (console) — the SHAPE: a story told through lots
- Bust Out Mode: you **move from lot to lot**. Each lot belongs to someone, has its own career, and you
  live there, work that career up, and complete that lot's goals to unlock the next one and buy back what
  Malcolm Landgraab repossessed. Freeplay is the same world without the ladder.
- Promotions are the progress bar. Items, social moves and lots unlock from goals.
- The rule that makes lots matter: leave a house worth less than you found it and the owner takes the
  difference; raise its value and they pay you.
- A second player can arrive by bus from their memory card.

### The Sims 2 (console) — the FEEL: a person you steer, with Golden Wants
- You **move your Sim directly with the stick** (this is why BORE 2 walks the way it does now: turn first,
  then move, never slide). Conversations zoom the camera in and the backdrop changes with the relationship
  (pink and close for lovers).
- Story Mode is a chain of **Golden Wants**: special wants that work as missions and unlock the next
  location (12 locations across Pleasantview, Strangetown and Melbourne). The goals are aspirational
  ("buy a nicer thing", "beat the bully at foosball"), not just career rungs.
- Befriend an NPC and "ask to move in": they follow you and you can switch to them.
- Meals are mixed from ingredients and the cooking skill; bad combinations make you sick.
- Dead Sims haunt as ghosts; bribe the Reaper or beat him at the fiddle to get them back.

### The handhelds: Bustin' Out GBA/N-Gage, The Sims 2 GBA, The Sims 2 DS, Castaway — the TEXTURE
- **Direct D-pad control, B to run** (the handhelds invented it), a **homesick** motive that drains away
  from home and refills there, **you never die — you pass out and wake up somewhere embarrassing**
  (the hospital, the jail behind city hall, Daddy Bigbucks' lair for a lecture).
- A **named cast of signature NPCs** who carry the story (Daddy Bigbucks, Uncle Hayseed, Olde Salty,
  Mamma Hogg, Emperor Xizzle, Optimum Alfred…), each with a place and a job in the world, and roommates
  you can invite home.
- **Objective-based progress**: finish a task list, the next house and the next part of the map open.
  Jobs are **minigames**. Bills can be half-paid. Shops sell food and things directly.
- **Conversations as a game**: you pick a social move, the other person accepts or rejects it on a chance
  that skills, gifts and mood shift. Compatible personalities give bonus points.
- The Sims 2 GBA: a **reality TV show** of 12 episodes; each gives ratings points for errands, the
  episode's hidden want and aspiration-flavoured conversations; points buy episodes, social moves and
  decor. Aspirations there are a *style of talking* (Friendly, Intimidation, Romance). One Performance
  bar instead of eight needs.
- The Sims 2 DS: a **hotel** you run; guests have moods (angry, sad, loopy, romantic) and each aspiration
  is the one that handles a mood; a **sanity** meter that aliens collect on when it empties.
- Castaway: **islands** — Shipwreck, Airplane, Volcano, Crystal — reached by building a raft, then a canoe;
  gather, craft, build shelter, reunite the crew into a tribe, find radio parts, escape or stay.

## 3. BORE 2's answer: the systems, in build order

The first three already exist in some form. The rest is the roadmap. Each one names the file it lives in.

1. **People and needs** (`sim.c`) — the 8 needs, mood, passing out, the clock, bills, a job. *Done.*
2. **A world of lots** (`world.c`) — HOME, CASTAWAY ISLAND, STRANGE TOWN; thin walls, build mode. *Done.*
   The island and the town are now **story unlocks** (first paycheck, SPONSORED at work).
3. **Direct 3D control** (`sim_player_move`) — turn, then walk. *Done.*
4. **WANTS & FEARS + the aspiration meter** (`wants.c`). *Done.*
4b. **The pie menu** (Sims 1/2): X opens the choices round a circle; people get TALK / JOKE / HUG / COMPLIMENT /
    FLIRT / TEASE (accepted or rejected on mood, relationship and personality), things get their variants
    (SNACK or COOK A MEAL, SLEEP or NAP, SHOWER or SING IN IT, JOG or SPRINT, CALL or ORDER PIZZA). *Done.*
4c. **Moodlets** (Sims 3): timed mood points from events (GOOD MEAL, HUGGED, TEASED, PROMOTED...) and from low
    needs (HUNGRY, STINKY, LONELY), summed into the mood. **Traits** (Sims 3) read off the personality:
    NEAT FREAK / SLOB, SOCIAL BUTTERFLY / LONER, ATHLETIC / COUCH POTATO, GOOFBALL / SERIOUS, GOOD / MEAN. *Done.*
4d. **Body language**: relaxed A-pose, breathing, a head on its own neck (curious tilts, nods while talking, droops
    when sad), happy hops, a sad slouch, a lean into the stride. *Done.*
4d2. **A living walk** (`sim.c` `follow_path`): the people the game steers (housemates now, the city's citizens next) no longer ride rails from tile
    to tile. Paths are pulled straight (line of sight) so nobody walks a staircase; waypoints are passed on a curve and nudged off the tile's middle;
    the body swings round in an arc and eases into corners; each person has their own pace (ACTIVE, mood, urgency) and ramps up from a standstill; they
    keep to the right and bend round each other (two people head-on pass, in a one-tile lane too), wait if held up, then plan again, then give up;
    a stroll meanders, stops to look about (at someone near, or over a shoulder), keeps roughly its heading, goes further and likes paths out of doors,
    and sometimes heads for a seat or something to do; they change their mind when the thing they wanted is taken, chase a chat partner who moved on,
    and plan again when a path is cut at 64 waypoints. *Done.*
4e. **Storeys and roofs** (`world.c`): the ground plus four floors above (5 storeys) per zone. Upper floors need
    support (something below or a floor beside them), walls stand on them, STAIRS ($400) connect a storey to the
    next (GO UP / GO DOWN in the pie menu) and open a stairwell above. The Sims 2 floor view: storeys above the
    one you are on are hidden. ROOFS are generated over every enclosed building from its footprint (HIP, GABLE or
    FLAT; LOW, MEDIUM or STEEP; six shingle colours), with eaves, gable-end walls, and wide buildings levelling off.
    The building you are in opens its roof; every other one keeps it. Build mode: L2 R2 storey, L3 see the roof,
    the ROOF tool. THE MAN BASE now has an upstairs bedroom. *Done.*
5. **Relationships** — daily score per pair, friend at 50, decay 2/day; chats move it, mood and
   compatible traits (NEAT/OUTGOING/ACTIVE/PLAYFUL/NICE) decide acceptance. Rejection is a fear.
5b. **More homes** (`world.c`): the city is 80x80 with 23 homes in four kinds (house, cottage, townhouse pair, two-storey with a loft). Next: owners and goal lists per home (step 6), a for-sale sign, moving in.
6. **A cast** — give the housemates and the townies names, homes and jobs; MOSS already exists (the one
   who refuses anything floating over their head). Bustin' Out's "move from lot to lot" becomes: each
   zone has an owner with a goal list; finish it and you may live there.
7. **Golden Wants as the story spine** — one gold want at a time, Sims 2 console style, that opens the
   next place: the ferry, the bus, the lab, the raft to the next island (Castaway).
8. **Careers as ladders with promotion rules** (skills + friends + mood), chance cards, a job minigame
   per career (the handhelds).
9. **Memories** — a short list per Sim of the big things that happened, shown on their card.
10. **Castaway islands** — a second and third island reached by building a raft, then a canoe, with
    gathering and crafting; the crew tribe.
11. **Conversations as a scene** — the camera zooms, the backdrop tints with the relationship, you pick
    a move (Sims 2 console + handheld "social battle").

## 4. Wants & fears, as BORE 2 does it (spec for `wants.c`)

- Each Sim holds **4 wants and 2 fears**. A want is a kind plus a parameter (which Sim, which amount,
  which skill level) and a points value.
- Kinds the game can actually detect: EAT, SLEEP, SHOWER, HAVE FUN, READ, COFFEE, RUN, PHONE, CHAT WITH
  ANYONE, CHAT WITH <name>, MAKE A FRIEND, GO TO WORK, GET PAID, GET PROMOTED, HAVE §N, BUY SOMETHING,
  COOK/BODY/FUN SKILL UP, VISIT THE ISLAND, VISIT THE TOWN.
- Fears: PASS OUT, WET YOURSELF, GET DEMOTED, A BAD DAY AT WORK, BILLS UNPAID, BE REJECTED.
- Tables per aspiration (FORTUNE: money, work; KNOWLEDGE: skills, reading, new places; POPULARITY: chats,
  friends, the phone; PLEASURE: fun, food, coffee, running; HOME: chats with housemates, meals, buying for
  the house, sleep; GROW UP: play, study, chats) plus the universal personality wants.
- Points: 500 / 1500 / 3000 / 5000. Meter 0..12000. Red < 3000, green < 7000, gold < 10000, platinum
  above. Platinum pins mood to +100. Decay per game hour: platinum 250, gold 190, green 60, red 90.
- Re-roll on fulfil, on waking, on arriving in a zone, and after 2 days unfulfilled.
- At the bottom (< 500): the Sim is DESPERATE — mood −40, random acting out notes, no skill learning.
- HUD: the current top want under the clock, a ticker through all four; the aspiration bar beside the
  mood face; a WANTS & FEARS page in the pause menu with every slot and its points.

## 5. Rules of taste (what NOT to do)

- No plumbobs. Ever. MOSS has made the household position clear. The marker is a small gold arrow.
- The GBA's creatures, items, wallpapers and songs are the real BORE data. Don't redraw them; light them.
- The game must stay at 30 frames per second on real hardware; everything above is cheap integer logic.
- Menus are solid cards, not glass. Text must be readable from a couch.
- Everything fits on one small disc. Music is sequenced, not streamed.
