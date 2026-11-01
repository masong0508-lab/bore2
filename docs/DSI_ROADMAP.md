# BORE 2 DS / DSi: roadmap (updated at D35)

## Where we are
A Sims-DS style life sim on two screens, one ROM (`bore2_dsi_d33.nds`) that runs on DS, DSi and the emulators (melonDS, DraStic, NooDS, DeSmuME).
Tested in melonDS only. Not yet tested on real hardware or on Android emulators.

**Done**
- Title (new BORE 2 logo, Household Occupants, Start/Load), Create-a-Sim (12 hairs, beards, faces, 26 sliders, paint pad), Map with drag editor, LIVE, drag Buy and Build, 400 items per lot, yards, fences, life stages, families, citizens and cars, shops and parks, 2000-bore cap.
- Saving (D22-D24): 8 KB EEPROM, A/B copies with a checksum, 48 remembered lots, stats; "Save space: Safe/Max".
- Real ARM7 (touch, sound), tabbed Options (Graphics / Audio / Game / System), Options inside a lot, jagged-ground fix (D27).
- **Music (D29): the PS1 jukebox on the DS, no streaming.** All 52 songs. `tools/dsmusic/mk_dsmusic.py` reads `assets/SONGS.BIN`, decodes each instrument from the PS1 sound chip's ADPCM, re-encodes it as DS IMA-ADPCM, and flattens every song to a time-ordered note list with the voices already shared out (9 voices; the rest of the 16 channels are kept for the game's own sounds). `tools/dsbuild/addmusic.py` appends the result (`assets/dsmusic.bin`, 5.7 MB) to the .nds and patches its address into the game (`MUS_LOC`). At run time the game reads one song (about 300 KB at most) off the cartridge into RAM and plays each note on a sample channel, once a frame. No streaming, almost no CPU.
  - Audio tab: Music source (Songs / Synth), Song (Auto or 1-52). Auto: the title song (THE DIPPER MAN) on the title, then a random song in the game, the next when one ends. The Options title shows what is playing.
  - Safe fallback: if the cartridge cannot be read (a loader that only gives us the code), the old synth tunes play.
  - Known limits: no reverb or per-song effects (the DS has none), polyphony 9 (busy songs drop the oldest note), IMA-ADPCM is about 22 dB signal-to-noise (a little hiss), a song change pauses the game for a moment while it loads. Band, DJ and Superstar modes are not ported, so no per-player skill changes to the music yet.

**Done at D30-D31**
- D30: Buy cards, Create-a-Sim restyle with name picker, property system (buy and sell lots on the Map, owned lots only for Buy/Build, saved).
- D31: **Careers** (6 jobs, 5 levels each, shifts Mon-Fri 6am-3pm, good or bad shifts, promotion, demotion, fired; menu > Job). **Wants and fears** (3 wants and 1 fear per sim, aspiration stars, bonus cash; menu > Life shows them on the top screen). **The dog** (buy the Puppy in Pets, pet / play / feed). **Ghosts** (a sim who dies comes back at night, spooks the household). **Roofs and storeys** in Build (Roof: none / flat / gable, Storeys 1-3; shown with Options > Roofs: Cutaway / Always / Hide; the upper floors are a shell for now, sims do not walk upstairs). **Catalogue** grew from 28 to 52 items (6 new tabs: Cook, Bath, Fun, Study, Patio, Pets, with 24 models). **Jukebox screen** (menu > Songs: all songs, Next, Mix, plus a "Now playing" toast).

**Done at D33**
- **Toolchain check.** The DS build chain in /tmp (`/tmp/dsb`, `/tmp/libnds182`, `/tmp/crtls_old`, melonDS core in `/tmp/mb`) was still there; `dsrun` was rebuilt from `tools/dsemu/run.cpp`. The rebuild-from-source steps are now in `tools/dsbuild/README.md`.
- `src/bigmap.c` (shared with the PS1) now asks for `ram8`, `xalloc`, `dbg_miss` and `world_gh_reset`. The DS gives harmless stand-ins at the top of `main.c` (no 8 MB heap, no world.c height cache).
- **Options closer to the PS1:** Game tab: Needs speed (Never fall / Slow / Normal / Fast), Day length (Short / Normal / Long / Epic), Message time (Short / Normal / Long). Audio tab: Title music (Off / On), Game music (Off / On; Off = quiet in the game unless you picked a song or a synth track yourself), Shuffle songs (In order / At random, for Auto in the game). System tab: Reset game (puts the Game tab back). "Reset all" is always the last row.
  - Same scales as the PS1: needs fall at 0 / 50 / 100 / 150 %, a game minute takes 66 / 100 / 150 / 250 % as long, messages stay 0.5x / 1x / 1.7x. Work shifts drain needs by the Needs speed too.
  - Not added: Thought bubbles (the DS shows none yet). PS1-only pages (People, Light, Camera, Screen, Cheats) have no DS counterpart yet.
  - Saving: the new options are in their own block ('OX') after the life block; a save from before D33 loads with the old options kept and the new ones at their defaults.
- **Property fix: one home, any number of properties.** The game now keeps which lot you live in (gold outline, HOME) apart from the lots you own (light blue outline, PROPERTY).
  - You cannot buy a house a family lives in ("Family home", no Buy button). Empty houses and empty land are for sale.
  - Buy and Build only on lots you own (as before). Visit works on any lot, your properties too.
  - "Move in" on a property makes it your home and takes you there; the old home stays yours as a property.
  - You cannot sell your home ("move out first") or the lot you are standing on.
  - The home is saved ('HM' block); a save from before D33 takes the lot you were on as home.
  - Bug fixed: entering a lot no longer resets your funds to 3500 (you could buy a lot and get the money back by visiting).
- Tested in melonDS: every Options tab and page, each new option's effect, save and reload (old and new saves), and the whole property flow with a save and reload.

**Done at D34 (`bore2_dsi_d34.nds`)**
- **Bug fix: a touch held over a screen change.** Tapping OK on the title, the Map button in Live or the Map chip in Buy/Build and keeping the stylus down picked the block under it as soon as the Map came up (the title's OK even chose an empty field as your first, free lot). Now every new screen ignores the stylus that opened it until it is lifted; a fresh tap counts at once.
- Tested in melonDS: title OK held for 4 frames, Live Map button held for 50 frames (the selection stays on the home), then a tap and a drag on the Map still pick blocks.

**Done at D35 (`bore2_dsi_d35.nds`): the PS1's whole buy catalogue**
- **189 items in the PS1's ten tabs** (Seats, Beds / Tables, Storage / Kitchen, Bath / Fun and Tech / Decor, Plants / Garden, Outdoor / Skate / Boreagotchi / Party / Special), cheapest first as on the PS1. That is every item the PS1 sells (137 in the nine named tabs, the rest in Special) except the door and the stairs (Build makes doors; the stairs come with walkable floors), plus the DS's own 10 (rocker, kitchen sink, bar, towels, console, books, swimming pool, puppy, dog bed, dog bowl).
- **The models are the PS1's own.** `tools/dsbuild/mk_dsitems.py` reads the PS1 items (`src/items.c`, `items_data.inc` and the GBA art headers it includes, and every pack: furniture, gotchi, party, band, space, farm, home, beds), keeps each box with two flat colours (the average of its side art and of its top art), applies the PS1's life-size table, and writes `ports/dsi/source/ds_items.h`. The DS draws them with its lit boxes (PS1 units: 10 DS units across, 7 up; a two-tile thing stretches over both cells). The 52 items the DS already had keep their hand-made DS models but take the PS1's names, prices, tabs and action words.
- **Two-tile things** (single bed, family bed, four-poster, toddler bed): they cover 1x2 or 2x2 cells, turn with Turn, block walking on every cell; Buy checks the whole footprint ("Not enough room", "Something is there"), Move drags the whole thing.
- **What a Bore does with it, in the PS1's words:** the action is the PS1's (sim.c `action_for` / `act_hint`: "Throw Darts", "Shoot Hoops", "Sing Karaoke", "Feed the Fish"...), mapped onto the DS's six needs (eat, toilet, sleep, shower, sit, fun, read, coffee, run, phone). Things with no action are looked at (or admired, in Decor).
- **The shop:** ten tab chips with pictograms, six cards a page with the tab name, page n/m and arrows; each card shows a little picture of the item's own 3D model (its boxes seen from the front and a little above), the price, and who may use it (or its size, 1x2). The place panel shows a bigger picture and the turned size. The item popup in Live is wider for the PS1's longer words.
- **Saves:** objects keep their ids (c * 4 + i), so every older save loads unchanged; a save from a newer ROM with unknown items drops them instead of crashing.
- **Fixes found on the way:**
  - Short taps: the vblank tap latch called `touchReadXY(&t)`, which in libnds 1.8.2 takes no argument and returns the struct, so `t` stayed garbage and any tap shorter than one pass of the main loop landed at a random place (in Buy most 3-frame taps were lost). Now `touchRead`.
  - Speed: every lit quad worked out its normal with soft floats and a square root; box faces (nearly all of them) now get theirs without floats. Buy went from about 15 to about 58 frames a second in melonDS, Live from about 30 to 45.
  - The Live icons (house, portraits, Cam) no longer stay over Buy's 3D view after the menu's Buy.
  - Tapping an item in Live tests the item's whole box as the camera sees it, nearest first: tall and thin things (a dartboard on its post, a lamp, a fridge's top) are caught where they are drawn, not by the floor behind them.
- Tested in melonDS: all ten tabs and their pages; every tab's items laid out on a bare plot (a test-only key in a `-DDSTEST` build) and looked at; buying and placing a 1x1 and a 1x2 bed, turning it 2x1; a D34 save loaded in D35; a save with new items (single bed, dartboard) saved and reloaded; tapping the dartboard in Live gives "Throw Darts" and the Bore walks over and plays.

## Next (in this order)
1. **Real-hardware and Android tests** of the song reader (`cart_blk` in `main.c`), saving and touch. If a loader blocks cartridge reads, add an SD (libfat) path for DSi.
2. **Walkable upper floors** (stairs, second-floor rooms) to go with the storey shell.
3. ~~Catalogue to the PS1 size~~ done at D35.
4. **Graphics options still open:** toon shading, per-item level of detail, screen swap, left-handed UI, text size, high contrast, colour filters.
5. **Speed:** done at D32 (redraw only what changed, tap latching).
6. **Music extras:** songs by place (shop, park, night).

## Bring over from the PS1 game
| PS1 feature | Source | DS state |
|---|---|---|
| Buy catalogue (10 tabs, ~180 items) | items.c, tools/make_items2.py | **done (D35)**: 189 items in the PS1 tabs, PS1 models, names, prices, actions; doors and stairs are Build things |
| Build tools (rooms, doors, windows, floors, roof, storeys) | world.c | walls, fences, floors, roof, storeys (shell only) done |
| San Boreas map, places, hours | bigmap.c, places.c | shared map; lot types simplified |
| Jobs, careers | places.c, story.c | **done (D31)**, simplified (6 jobs, no real workplaces) |
| Wants and fears | wants.c | **done (D31)**, 16 wants and 5 fears |
| Families, babies, ageing | pop.c | done (D18) |
| Home buying and travel | home.c | buying done (D30); travel open |
| Castaway Island | island.c, nature.c | open (large) |
| Music: jukebox | audio.c, SONGS.BIN | **done (D29), screen added D31** |
| Music: band, DJ | band.c, dj_ui.c | open (large) |
| Mini-games (skate, arcade, fight, heist, gotchi, farm) | various | open, one at a time |
| Dog, ghosts, traffic | dog.c, ghost.c, traffic.c | all done (D31) |

## DSi-only ideas
Use the 16 MB RAM and the 2x CPU clock (Turbo CPU option exists), DSi camera for skin and hair colours, Wi-Fi visits, banner and icon polish.

## House generation
Done: one layout family (two back rooms, open front, four sizes). To add: two storeys, L shapes, garage, room-aware furnishing per lot type, windows, roof.

## Build notes
`tools/dsbuild/mkp.sh` builds the ROM and runs `addmusic.py`; `tools/dsemu/run.cpp` is the headless melonDS runner (`AUDOUT=file.wav` records the sound, `DBGMEM=addr,addr` prints variables every 30 frames).
Music data: `python3 tools/dsmusic/mk_dsmusic.py assets/SONGS.BIN assets/songs.inc assets/dsmusic.bin` (needs `gcc -O2 -shared -fPIC -o /tmp/adpcm.so tools/dsmusic/adpcm.c`).

## D30 done
Buy/CAS interface restyle and property system are finished. Still to test on real hardware: lot selling and ownership save round-trip.
