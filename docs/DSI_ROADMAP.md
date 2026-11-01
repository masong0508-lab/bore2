# BORE 2 DS / DSi: roadmap (updated at D33)

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

## Next (in this order)
1. **Real-hardware and Android tests** of the song reader (`cart_blk` in `main.c`), saving and touch. If a loader blocks cartridge reads, add an SD (libfat) path for DSi.
2. **Walkable upper floors** (stairs, second-floor rooms) to go with the storey shell.
3. **Catalogue to the PS1 size** (about 146 items) and better models for the ones that still use plain boxes.
4. **Graphics options still open:** toon shading, per-item level of detail, screen swap, left-handed UI, text size, high contrast, colour filters.
5. **Speed:** done at D32 (redraw only what changed, tap latching).
6. **Music extras:** songs by place (shop, park, night).

## Bring over from the PS1 game
| PS1 feature | Source | DS state |
|---|---|---|
| Buy catalogue (8 tabs, ~100 items + 46) | items.c, tools/make_items2.py | partial (13 tabs, 52 items; 400 per lot works) |
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
