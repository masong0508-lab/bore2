# BORE 2 (PlayStation)

The sequel to BORE (GBA), in true 3D. The voxel people (6x4x8 blocks, built by the GBA game's own buildLook), the items
(the GBA's textured boxes), the 40x40 lots, 103 wallpapers and all 33 jukebox songs are on the disc.

## Play it
**[disc/BORE2.chd](disc/BORE2.chd)**: the whole game in one file (about 4 MB, every song included). Download it and open it in
DuckStation (File > Start File). Turn on *Settings > Console > Enable 8MB RAM* for the bigger world: the game finds the extra RAM
by itself, and still runs on a normal 2 MB PlayStation.

For a real PlayStation or PCSX-ReARMed, burn or open `bore2.cue` + `bore2.bin` from `cmake --build build --target iso`.
Every push to GitHub builds both: open Actions, the newest green run, and download **BORE2_chd** or **bore2_disc** under Artifacts.

STORY MODE opens PICK A WORLD: RANDOM WORLD (fresh dice), three set worlds (MAPLE SHORES, SUNNY HOLLOW, PINE BAY: the same map every
time, the capital named for the world), CREATE A WORLD (SQUARE rolls a new map, L1 R1 step through them, the map shows each one), CASTAWAY ISLAND (the family begins
stranded on the island), and MISSIONS, PACKS (RAGS TO RICHES: $1,000 and no home; the expansion packs say COMING SOON). TRIANGLE
switches TOWNS between ROOMY (parks, green gaps, towns far apart) and PACKED (built up, towns close together, as before).
MISSIONS, PACKS > STORY MISSIONS is the GBA game's story mode: ten stories (ROOMMATES, NEWLYWEDS, SINGLE PARENT, SKATE LIFE, HOUSEFULL,
BEST FRIENDS, RAGS TO RICHES, CAREER CLIMBER, NEW IN TOWN, SECOND CHANCE), each with its own lead you play and its own people, five
chapters and THE END. A chapter is a scene with up to three tasks, done in any order: places in SAN BOREAS (a skate plaza, a bar, a
park, a town square, STRANGE TOWN), things to do (a pizza, a joke, tricks off the ramps, a paycheck, a promotion), people to win round
(a friend, love, GO STEADY on the pie menu, a best friend), money to save, a baby that comes home. A card opens each chapter and
closes it (it pays $500 to $1500); the phone's GOALS is the story journal (X takes a taxi to where the chapter sends you, TRIANGLE
shows THE STORY SO FAR); the HUD shows the next task. TV SHOW & TELL (Missy's story) is held back for its own voiced, filmed version.
`src/story.c` has the stories; saves keep them (save version 5).
SELECT names your capital city (a letter keyboard: X types, SQUARE deletes, START keeps it). In play, the phone's MAP renames any
town: pick it and press TRIANGLE. Saves keep the names. In CREATE A BORE, SQUARE names your Bore (first and last name).

Then THE NEIGHBORHOOD (like the PC games' neighborhood screen): the capital's blocks from above, with 48 lots picked at random from
what is for sale, about a third empty lots to build on and the rest furnished homes. The d-pad hops from lot to lot (L1 R1 go in
order), the card shows the address, the kind, the price and your $20,000; X buys it and the family moves in, TRIANGLE stays at
THE MAN BASE. The phone's HOMES app opens the same screen round where you are (SQUARE goes to see a lot, SELECT shows the list).

Controls: stick / d-pad move, X uses the thing in front (fridge, bed, shower, sofa, TV, radio, the DeadSet...), talk to a
housemate with X, O gets up, SQUARE runs, O rides a skateboard (buy one at a clothes shop, or pick one up off the floor with X), TRIANGLE game speed. SELECT opens your PHONE: CALL, PIZZA, PARTY, MAP, TRAVEL (the
island and the desert town), JOBS, HOMES, PEOPLE, FAMILY (play as another Bore), ADOPT, MUSIC and GOALS. START is the short
menu: the BORE PANEL, BUILD MODE, CREATE A BORE, OPTIONS, HOW TO PLAY.

Camera: L1 R1 orbit, L2 R2 zoom, the right stick orbits and tilts; L1 + R1 together swing it round behind you. L2 + R2
together switch between three cameras: THE SIMS (it orbits you), BEHIND YOU (it follows round as you walk) and FIRST PERSON
(through your Bore's eyes: up walks, left and right turn, L2 R2 look down and up, L1 + R1 together turn right round; the head levels out as you walk and sways a little, more when you run; a dot in the middle shows what X will do; your own roof is the ceiling). OPTIONS,
CAMERA also sets how high it looks down from, how fast it turns, whether zooming in tilts it down and the WALL VIEW: AUTO (the walls between you and
the camera are cut low, but not in first person), CUT AWAY (always) or FULL WALLS (never). In first person nothing near you is dropped: a floor, wall or
ceiling that reaches behind the eye is cut at a near plane and drawn from what is in front, and close ones are split so they do not swim.

## Build it
1. Install PSn00bSDK (https://github.com/Lameguy64/PSn00bSDK/releases) so `mipsel-none-elf-gcc` and `cmake` work.
2. In a terminal:

       export PATH=/path/to/psn00bsdk/bin:$PATH
       export PSN00BSDK_LIBS=/path/to/psn00bsdk/lib/libpsn00b
       cmake --preset default
       cmake --build build --target iso

3. Open `build/bore2.cue` in DuckStation. (`--target chd` also makes the one-file `build/BORE2.chd`: `tools/mkchd.py`.)

## The music
All 52 jukebox songs are SEQUENCES, not recordings: each is its note events plus small instrument samples in the PS1 sound chip's
own format, in one file, `assets/SONGS.BIN` (5.6 MB; a song's notes are a few KB). `src/audio.c` loads one song's samples into sound
memory and a 480 Hz timer interrupt keys the notes on, so the sound chip does the playing. `tools/build_music.py <bore-gba-main> assets`
rebuilds the tracker songs with the GBA game's own converter (`tools/xm2gba.py`), so they play the same notes, volumes and pans as on
the GBA; a song whose samples are too big for sound memory (about 300 KB each) has its biggest samples made smaller until it fits.
New from the latest GBA build: MISTER QUICKBULLET, OVER MY VOICE, OHIO HOE, CYNICALLER MADNESS and 12 Dipper Man demos.
The creature code: `python3 tools/port_look.py <bore-gba-main>/source/main.c src/look_port.inc`.

The HERE TODAY family (HERE TODAY GONE TOMORROW by Singhs; HERE TODAY (MISSY'S SONG), its FULL VERSION and the ORIGINAL by Missy
Jeanne) has no tracker file: the GBA game renders them from Python scripts. `tools/build_midi_songs.py <bore-gba-main> assets
id|NAME|ARTIST|script|fx|gain ...` runs those scripts, catches every note they play, makes each instrument one small looped sample, and
writes the notes as a sequence like the others (with note offs and stereo pans), so they are small too. They are in the jukebox and the
radio (CHILL WAVE). (`audio.c` can also stream sound off the disc, kept for later; `tools/build_streams.py` makes those streams.)

## Folders
- `src/` the game (C). `look_port.inc` is the GBA creature code ported by `tools/port_look.py`.
- `assets/` voices, the songs (`SONGS.BIN`), the song list.
- `tools/` porting, hair texture and audio tools; `xaenc.c` (XA encoder using libpsxav); `lrrun.c` (headless emulator runner for screenshots).
- `test/` scripts used to take emulator screenshots and measure FPS.

## Known gaps
Someone walking or chatting when you save stands idle after loading (sleeping, eating, working and the like carry on), the card driver is tested only in an emulator (no real card yet), tested only in an emulator.

## Saving
START, then SAVE GAME, then pick one of 3 slots. CONTINUE on the title shows the 3 slots (the family, the day, the Bores, the money) and loads one. Each save is one file on the memory card in slot 1, usually 1 or 2 blocks (it takes only what it needs; a very long life, hundreds of days, can reach 4). Saving over a slot in use asks first (X again). SQUARE, then SQUARE again, deletes a save. `src/mcard.c` talks to the card over the serial port like the pad; `src/save.c` packs the game. A save holds the family (looks, needs, wants, relationships, skills, what each one is doing), the clock, money, job, the home lot and every building you made, the island's camp and the town as you left them, the open world's seed and the changes since (who grew up, moved, passed on), the island's stores and secrets, and the options. The world itself is made again from its seed.

## Wants and fears
Each Bore has 4 wants and 2 fears from their aspiration, age and personality. Doing a want pays aspiration points; a fear coming true takes them. Wants include throwing a party, buying a home, riding a skateboard, finding an island secret, visiting the island or the town, skills, money and friends. Fears include losing someone in the family. Reaching PLATINUM gives a PROUD moodlet.

## Memory
The game fits the PS1's 2 MB with about 140 KB to spare. A Bore keeps room for 96 solid blocks (the most any rolled Bore used was 58),
the creator's studio parks the live lot in a free storey slot instead of a copy of its own, a townsperson is built in the same
scratch Bore the crowd unpacks into, the song buffer is exactly the biggest song (94 KB), and each packet buffer is 88 KB.

## Speed
In SAN BOREAS, as soon as a frame runs long the full-detail range pulls in a tile (up to 4) (buildings past it are drawn as simple boxes) and goes back out when things are quiet, so the city holds 30 frames a second more often. Far-off roofs are drawn as bigger pieces.

## Buying and building
BUILD MODE (START, BUILD MODE), BUY tool: the catalog has eight tabs (SEATS, TABLES, KITCHEN, FUN AND TECH, DECOR, GARDEN, SKATE, SPECIAL),
each sorted cheapest first. X buys, or picks up an item you already own to MOVE it (X again puts it down, O puts it back; carry it to
another storey with L2 R2). SQUARE sells for 3/4. TRIANGLE turns the item under the cursor, or the next one you buy. Hold X and walk
to lay a row of hedge, fence or any cheap item. The second furniture pack (46 more things: drums, pool table, slot machine, swing set,
hedges, a four-poster bed...) is generated by `tools/make_items2.py` into `src/items_more.inc`.

Construction: ROOM tool, SQUARE chooses NO DOOR / A DOOR / DOOR + WINDOWS (put in with the room, front wall), TRIANGLE takes back the
last room and its cost. FLOOR and WALLPAPER tools, TRIANGLE: lay the floor over, or paper the walls of, the whole closed room under the cursor.
Collision: things block where they really stand (their boxes), not the whole tile, so a fridge leaves a margin and a fence a gap; paths still plan by tile.

## Sleeping
Lying down is an animation, not a swap: the Bore sits back and tips over a second (the legs stay hanging as the body goes down), and
getting up plays it backwards (`gait.lie` in `voxel.c`, eased by `main.c`). In a bed the back rests on the pillow's top, the head
turned a little on it, the legs hang over the foot, a quilt and a folded-down sheet cover the body (`voxel_draw_covers`), and the
chest rises and falls. Bores are bigger than the bed art, hence the dangling feet.

## Crowds and parties
THROW A PARTY (the phone) brings up to 60 guests, who walk in, dance round you and go home after three hours.
Every person in a crowd is the full model, like your own Bore: `src/crowd.c` keeps a compact copy of each model (~2 KB) in the
RAM the voice clips used before they went into the sound chip, draws the few biggest on screen live in 3D, and shows the rest as
sprites of that same model, rendered into spare video memory and redrawn round the crowd as the frame's time allows.

## The city and places to go
SAN BOREAS is a whole state worked out from the save's seed (`src/bigmap.c`): towns, cities and villages, and on their blocks homes
and 19 kinds of place: shops (grocery, clothes, electronics, furniture, music), diners, cafes, restaurants, bars, gyms, pools, spas,
salons, cinemas, galleries, libraries, clinics, arcades, inns, hotels, offices, towers and schools. `src/world.c` builds each one's
inside (`lot_*`), `src/places.c` knows their names and opening hours, and the pie menu (X) sells what each one sells: a meal, a latte,
a drink or a round, a film, a haircut, a massage, a checkup, a swim, a room for the night, groceries for home, an outfit.

Jobs (PHONE, JOBS) are at real places too: 14 careers (13 in town, and the old factory) with their own shifts, days, pay and six titles each (DISHWASHER to
RESTAURATEUR, INTERN to CEO, ORDERLY to MEDICAL LEGEND...). Go there (the boss pays the taxi) and X on a counter, desk or machine.

## Growing up
Every Bore grows up a day at a time: BABY, CHILD, TEEN, ADULT, ELDER (OPTIONS, LIFE: off, short, normal or long lives).
On a birthday, X on the fridge or the stove bakes a cake, and BLOW CANDLES on it makes the birthday a party (with confetti).
Two grown-ups who get on can START A FAMILY (X on one of them); or ADOPT on the phone. A baby takes after its parents, cries
when it needs something (FEED, CHANGE, PLAY, CUDDLE, PUT TO BED) and the grown-ups come running; a crib comes with the first one.
Kids and teens catch the school bus at 8 on weekdays and are home at 3; homework at a desk or a bookshelf keeps the grade up
(the SCHOOL tab in the Bore panel). At the end of the elder years they pass on (or not: OLD AGE, LIVE ON) and a stone stands in the
yard. The family tree (partner, parent, child, sibling, grandparent) is in the panel's PEOPLE tab. The rest of SAN BOREAS grows up
too (`src/pop.c`): its people have birthdays, babies, leave home, pass on, and new families move into the empty homes.

## A home of your own
PHONE, HOMES lists the homes for sale near you in SAN BOREAS: houses, cottages and two-storeys nobody lives in, and empty lots,
each with its price (dearer in a city). A FOR SALE sign in a yard sells that one too (X on it). Buy one and the family moves in by
taxi; a house comes furnished for them, an empty lot is cleared for building. That block is theirs from then on (`src/home.c`,
and `world.c` keeps it just as they build it, every storey), TRAVEL has HOME at the top, and the map shows where it is. Buy another
and the old one sells for what it cost. THE MAN BASE stays yours too.

Build mode has seven tools: BUY, ROOM (X on one corner, X on the other: walls round it, a floor inside), WALL, DOORS AND
WINDOWS (X on a wall), FLOOR, WALLPAPER and ROOF. L2 R2 change the storey (five in all): a room upstairs stands on the walls
below it, and the stairs (BUY) open the floor above them. In SAN BOREAS you build on your own lot only.
The cursor is a tile with corner brackets: green when X will work, orange when it would take away what is there (SQUARE sells
it back for three quarters), red when it cannot (not your lot, no money, someone standing there). A BUILD MODE banner slides in
as you enter, the last buy or sale shows beside your money, and BUY has a card with the pick's price and sell-back value. Hold X
and move to draw a run of walls or wallpaper (or floor); hold SQUARE and move to take a run of walls away.

## Castaway Island
TRAVEL takes the family to CASTAWAY ISLAND (once they have earned a paycheck): a new island every life (`world.c`), with a coast
that wanders, beaches, meadows, jungle, rocky hills with pines, a pond, islanders' huts, and an islet out at sea. The family camps
by the jetty, and lives off the land (`src/island.c`): X on trees, bushes and rocks gathers WOOD, LEAVES, BERRIES, COCONUTS and
STONE; fish off the jetty and the beaches; COOK AND EAT at the campfire (every meal on the island comes out of what was gathered,
and when there is none the others go foraging). CRAFT at the campfire makes a TENT, a LOG BENCH, an OUTHOUSE, a SPEAR (fish
faster, catch more), a plank SHELTER, and a RAFT that sails out to the islet. Eight secrets are hidden round it: a message in a
bottle, buried treasure, a tiki idol, a shipwreck, six seashells, a hermit's journal, the islet, and a golden idol. The map
(PHONE, MAP) shows the island, ticks off what is found and hints at the rest; finding them all is one of THE STORY's goals.

The island is drawn as a place rather than as blocks (`src/nature.c`): the ground is coloured corner by corner (sand into meadow
into jungle, wet sand and foam at the waterline, shade under the trees, a sea that goes from turquoise shallows to deep blue)
over fine grain textures; palms have ringed, curving trunks and drooping fronds, the jungle trees ragged leafy crowns, the pines
tiers of needles; boulders, berry bushes, a stone-ringed campfire with flickering flames, an A-frame tent, log benches, a sailing
boat, the raft and the wreck are all modelled. Past the near ground the rest of the island is drawn coarser out to 36 tiles (its
woods as a raised canopy), going into the haze, and the sea runs on to the horizon.

## DS port (in progress)
The game code is split so a Nintendo DS build can sit beside the PS1 one:
- `src/platform/compat_math.*` the PS1 SDK maths (rotations, matrices, sin, sqrt) in plain C, tested against the real values.
- Still PS1-only: `gfx.c` (GTE drawing), `audio.c` (SPU and CD music), the pad code in `main.c`, and the GTE calls in `items.c`.
- Plan: `src/platform/nds/gfx_nds.c` on libnds' 3D engine (it sorts depth itself, so no ordering table, and gouraud colour is free), BlocksDS to build.
