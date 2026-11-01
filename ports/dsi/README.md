# BORE 2 for the Nintendo DSi (port, step D0)

Status: **written but not yet built or run.** The workspace it was written in had no DS toolchain (devkitARM) and no DS emulator, and the libnds it could fetch needs the `calico` library, so even a syntax check could not run. Expect a few compile fixes on the first real build.

## Build (on a computer with devkitPro)
1. Install devkitPro, then `dkp-pacman -S nds-dev` (devkitARM, libnds, calico, ndstool).
2. `cd ports/dsi && make` gives `bore2_dsi.nds`.
3. The ROM must be flagged as DSi-only (DSi unit code), and run on a DSi or a DSi-capable loader/emulator. The exact `ndstool` flag for that is to be checked against the installed ndstool's help; `main.c` also stops with a message when it is not on a DSi (`isDSiMode()`).

## What step D0 does
One lot (13 x 13 tiles), a house box and 6 billboard people on the 3D screen; the bottom screen shows the polygon count (budget 2000) and the GPU's own polygon/vertex counters. This is the measuring spike from `docs/PORTING_DS_3DS.md` section 8.7. Read the numbers off a real DSi and write them into `docs/COSTS.md`.

## Next
- D1: share the platform-free game core from `src/` (sim, world, people) through `plat_*` headers (Phase 0).
- The shared sprite baker for people.

## Verified so far (the shared core on a normal computer)
`ports/host` builds the platform-free world generator (`src/bigmap.c`, unchanged) with plain gcc and prints a generated world: 21 settlements, sea, forest, hills, towns. That proves the world code does not depend on the PS1, which is the first requirement for the DSi/3DS ports. Run `cd ports/host && make run`.

Next shared-core steps, in order: `look.c` (names and looks), `pop.c` (the people), `sim.c`, `save.c`; each added to the host build with a small shim, then the drawing goes through `plat_*` headers.

## D1 status (built and run here)
The DS build chain in tools/dsbuild (clang + libnds 1.8.2) and the headless melonDS runner in tools/dsemu now build and run this port.
Tested in the emulator (DS mode, FreeBIOS): bottom screen shows the 2D neighbourhood map from bigmap.c, a stylus tap picks a lot, the top screen draws that lot in 3D
(houses with pitched roofs, backyard pools and fences, towers, shops, parks, people). L/R change town, A/B zoom, START re-rolls the world.
Still to do: real DSi hardware flags (ndstool -g with the DSi unit code), the sim, saves, vehicles, and the other zones.

## D2 status: LIVE / BUY / BUILD on two screens (Sims 2 / Sims 3 style)
From the map, press A or tap the picked lot again to play it. Top screen: the live lot in 3D. Bottom screen: the tool.
- LIVE: three sims wander the lot; bottom shows needs bars; tap a name to switch sim.
- BUY: catalogue with 5 tabs and item cards (price, funds). Tap an item, then tap the plan (or D-pad) and tap again / A to place; the 3D lot shows the ghost at the cursor.
- BUILD: plan view with FLOOR, WALL (tap near a cell edge), ERASE and SELL tools and colour palettes.
X/Y zoom, L/R turn the view in LIVE. Lot contents are not saved yet.

## D3 status: Sims 3 DS style flow (not fully tested)
Create-a-Sim first (head on top, hair/colour/skin/shirt grid on the bottom, PLAY). Then the map, then LIVE: top screen = sim status (mood, time, funds, six needs; tap another sim for its bio), bottom screen = the 3D lot with HUD icons, tap floor to walk, tap furniture/sim for an action menu. "..." tab opens BUY / BUILD / MAP.
Known: the bottom-screen HUD overlay (map button, icons, menus) may not show yet (main-engine BG3 layer); needs a check. Tap picking sign conventions are untested.

## D4: fixes and polish
Bottom-screen HUD layer fixed (VRAM bank D mapping); tap picking checked (a tap marks the cell it hit); nicer proportional text (libnds 8x8 font with shadow, title case); refined blocky heads and sims (bevelled, gradient-lit boxes, eyes with iris/pupil/highlight, brows, nose, lips, ears, layered hair); Create-a-Sim camera framed.

## D5: Create-a-Sim styles
Back to the flat-coloured chunky look (no gradients). Six tabs: Hair (12 styles, 2 pages), Color, Face (classic, round, long, big eyes, sleepy, freckles), Beard (none, stubble, mustache, goatee, beard, long), Skin, Shirt. Portraits and the 3D head and in-lot sims all follow the choices.

## D6
Sims 3 DS style title (Household Occupants / Start-Load profile), stylus Edit pad + RGB Paint in Create-a-Sim, drag-based Buy/Build, drag-paint neighborhood editor.

## D7
Create-a-Sim: 26 shape sliders (face + body: jaw, chin, ears, eye size, lips, neck, shoulders, torso, arms, belly, hips, legs), 8-feature drag pad, full-body view, nicer tabs. Fixed top screen not clearing after title.

## D8
Height slider; "Max x1/x2/x3" cheat chip multiplies every slider for extreme scales.

## D9
Max x2/x3 cheat no longer clamps: minimum values go negative (inverted geometry glitches).

## D10
In-game items are now small multi-part models (20 items: chair, sofa, bed, fridge, tv, bathtub...) with soft shadows. See docs/DSI_ROADMAP.md.

## D11
Real DS hardware lighting (key + fill light, per-face normals) on the Create-a-Sim head, the lot and the furniture; generated starter house (bedroom, bathroom, living room, kitchen/dining, doors, front door, mirrored/size variants from the lot seed); Options screen (title > Opt): anti-alias, outlines, shadows, lighting, item detail, ground check, 30/60 fps, brightness, sky, camera distance, map spin, game speed, turbo CPU.

## D12
Better Sim models in the lot (shoes, belt, collar, sleeves, hands, ears, eyes, brows, nose, mouth, hair fringes; they turn to face where they walk and swing arms and legs). Layered trees, house doors/windows/chimney, grass all round the lot, fence, path. Camera: LIVE has D-pad turn/tilt, L/R snap, X/Y zoom, SELECT follow/free, and a stylus Cam panel (drag to orbit/tilt, Pan mode, zoom, follow/free, reset). BUY/BUILD have camera buttons (turn, tilt, zoom, reset) on the bottom screen plus L/R, X/Y.

## D13
Thin walls (skirting board + wall + pale cap, merged into runs to save polygons), real doors (posts, lintel, swung leaf; Build > Wall palette has a Door), roomier generated house (12x9, open living/kitchen, bedroom + bath with doors). Fixed z-fighting lines (near plane, layer offsets, wall corner overlaps), blown-out lamp (softer light), and the polygon-budget bug that hid furniture. Note: sims do not yet collide with walls.

## D14
Sims path-find round walls (BFS per cell; doors are open, solid furniture blocks; "Cannot get there" if sealed in). Proportions: walls ~2.1 m, furniture heights scaled to the sims, narrower FOV (40 deg) and lower camera pitch, so less squashed; walls toward the camera drop to half height (cutaway).

## D15
Doors are solid wooden leaves with panels and handles, shut until a sim is at them (then swung open). Streets are 3 cells wide with pavements and yellow lane dashes (the "pellets" were fence posts standing in the road: fence moved onto the plot edge). Path is a raised slab to the pavement. Floors are merged into big rectangles (edge anti-aliasing was showing grass through the seams as green dashes).

## D17 - open streets and the 2000-bore cap
- LIVE is now a 3x3 patch of lot slots: your lot in the middle, the 8 lots next door round it, with pavements, 3-cell roads, lane dashes, grass and sea. Tap or walk out the door, down the street and into the next lot with no map screen; the next lot becomes the middle one and gets its home (a lot you built on is remembered, last 6).
- Neighbour houses are solid, sea is solid, the plot's back fence has a gap at the path. Pathfinding runs over the whole patch.
- Population cap: DS_POP_MAX 2000 bores and DS_FAM_MAX 640 households (PS1: 3700 / 874..1748). pop_build() fills homes from the capital outwards (then the next town) until a cap is hit; homes past the cap have no bores. The map screen shows "N homes M bores" for the block and the total x/2000; neighbour lots show their residents.
- Not yet: every lot you walk into becomes a playable home (shops/parks too); residents are not yet people you can talk to when walking by.

## D18 - life stages, family items, interactions, a living city
- Life stages Baby/Toddler/Child/Teen/Adult/Elder (a year per game day); sims change size and look with age; the household holds up to 6 (tap a portrait to play that sim; babies cannot be played).
- Interactions by stage and relationship (Chat, Play, Hug, Joke, Tease, Argue, Flirt, Kiss, Try for Baby, Feed, Teach, Tuck In, Advice, Invite Home); romance never crosses the teen/adult line; 6 talks per pair and 4 per citizen per day.
- Family items: Crib, Toy Box, Books, Rocker, plus per-stage item limits (shown in the shop).
- Citizens walk the pavements by the clock, cars drive the roads; tap a citizen to talk; befriend and Invite Home. Births and the 2000-bore cap are counted together.
- Fence posts replaced with a hedge line; polygon-budget cuts (lawn, dashes, level of detail).

## D19 - Yard category
- New Yard tab: Fence (turn it to run either way), Lamp Post, Hedge, Flowers. Place them anywhere on the plot; up to 96 items per lot (was 48), remembered per lot.

## D20 - 400 items per lot
- Up to 400 items per lot (was 96). Sims are drawn first, then items; far items are skipped when the DS polygon budget (about 1500 quads) runs out, so a crowded yard never makes sims or the player's view disappear.

## D21 - fences on edges, elder death, real shops and parks, options
- Build > Wall tool: a 6th palette colour draws a FENCE on cell edges (drag like walls). Fences block walking; doors still open.
- Elders (80+) can pass away; a statue marks the spot, the household closes up (never the last adult).
- Non-home lots are real places: parks, squares, farms and wild land are open yards with benches, lamp posts, flowers, hedges; shops, diners, schools, clinics and the rest are one open hall with fitting furniture.
- Options 3 pages: Sim style (Classic / Sims 3 / Chunky body proportions), Poly meter (quads used of ~1500), Fog.
- Cushions on the sofa and armchair.


## D22 - Saving
- Saves to the cartridge save chip (EEPROM, fits any 8 KB chip). No SD card needed.
- Menu > Save saves now. It also autosaves every game day and when you go back to the title.
- Title: OK loads the saved game ("Saved game found" shows). Del erases it.
- Two copies (A/B), the newest good one loads, so a power cut while saving never loses your game. Each copy has a checksum.
- Saved: sims, funds, time, relationships, options, and the lots you built (current lot first; oldest lots are dropped only if the chip is full).
- Needs a save chip: the ROM header is now a normal cart header (gamecode BR2D). If your flashcart/emulator gives no save, the title says "No save chip found".
- Emulator test: tools/dsemu/run.cpp keeps a save file when BORE_SAV=file is set.

## D23 - Bigger saves
- Remembers and saves up to 48 lots (was 6).
- The game detects an 8 KB or 64 KB save chip. 64 KB holds about 10x more (8 KB chips keep the newest lots first, as before).
- Lifetime stats saved: days played, births, losses (shown on the title).
- 64 KB path is untested in the emulator (it only offers 8 KB); the 8 KB path is tested.

## D24 - Save space option
- Options page 3: "Save space": Safe (default, 2 copies, power-cut proof) or Max (one copy uses the whole chip, about 2x more lots on 8 KB chips, 64 KB chips fit nearly all 48 lots).
- Save format changed in D24: older D22/D23 saves will not load.
- Why not bigger than 8 KB on Android: emulators fix the save size from their own game list; an unknown game gets 8 KB, and a bigger .sav file is ignored. So the game works inside that and uses 64 KB only if the chip really is 64 KB.

## D25 - Options tabs, sound, real ARM7
- **Real ARM7.** The old ARM7 was an 88-byte stub, so touch and X/Y buttons only worked in my test harness. Now a proper libnds ARM7 (touch, buttons, clock, sound) is in the ROM, so touch works in any emulator (Android melonDS/DraStic) and on hardware. Build: tools/dsbuild/arm7/build7.sh.
- **Sound.** Four square-wave voices: 3 music (lead, bass, pad), 1 effects. Four tunes (Daydream, Evening, Bounce, Night). Auto picks by place and time of day. Effects: tap click, buy, error, save, new day, life stage.
- **Options in 4 tabs** (Title > Opt): Graphics, Audio, Game, System. Green = changed from default.
  - Graphics: Anti-alias, Outlines, Outline color, Shadows, Lighting, Item detail, Ground check, Draw distance, Fog, Fog range (fog now matches the sky), Sky (new Auto follows the clock), Brightness, Sim style, Poly meter, Traffic (Off/Few/Some/All).
  - Audio: Master / Music / Effects volume, Music track, Tap clicks, Chimes, Stereo spread.
  - Game: Camera, Map spin, Game speed, Autosave, Save space, 12/24 hour clock.
  - System: Frame rate, Turbo CPU, Reset graphics / audio / all.
- Options are saved in the save file (format changed again: D24 saves will not load).
- Buttons: labels are now centred in tall buttons.
- Not done: Options only open from the title screen (not yet from inside a lot).

## D26 - Interface pass
- Lot menu is now a proper panel (Buy, Build, Map, Save, Options, Title) with big buttons.
- Options open from inside a lot (Menu > Options) and from the map (Opt button); Back returns where you were.
- Map strip cleaned up: no overlapping text, Opt and Edit buttons.
- Buy/Build category chips fit their words (no more cut-off "Tabl").
- Title slots are labelled (Continue / New game / New sim).

## D27 - Jagged ground fix
- The lawn checker, lawn and pavements were drawn only 2-6 units above the ground under them, so the DS depth buffer flickered between them (saw-tooth diamonds). They now sit 20-30 units above: clean edges.
- Options inside a lot: Menu (the "..." button) > Options (since D26).

## D28: the new BORE 2 logo on both title screens (tools/make_logo.py makes src/logo_rects.h from docs/bore2_logo_1x.png).

## D29 - the songs
- All 52 PS1 jukebox songs on the DS, as sequenced samples (no streaming). See docs/DSI_ROADMAP.md. Options > Audio: Music source (Songs/Synth), Song (Auto/1-52). ROM is about 6 MB now.
- tools/dsmusic (converter + IMA-ADPCM helper), tools/dsbuild/addmusic.py (appends and patches the ROM), run.cpp can record audio (AUDOUT).

## D30
- Buy menu restyled: item cards, category pictograms, coin prices, funds pill, place panel.
- Character creation restyled: pictogram tabs, card grid with check badge, name picker (first/last names), top-screen nameplate.
- Property system: lots must be bought on the Map (Buy/Sell chips, gold OWNED/HOME badges). Buy/Build only work on owned lots. First lot is free. Ownership is saved (old saves migrate).

## D31
- Careers (menu > Job), wants and fears (menu > Life), the dog (Pets > Puppy), ghosts, roofs and storeys (Build tools, Options > Roofs), 6 new Buy tabs (52 items), Jukebox screen (menu > Songs).
- Save: a new trailer holds jobs, wants, the dog and ghosts; roof and storeys ride in the lot header's spare bits. Old saves still load.

## D32
- Input and lag: the live screen was running at about a quarter speed because it redrew both screens every frame while a message showed. Top screen, touch layer and Buy screen now redraw only when something changed, and rectangles fill 32 bits at a time. Taps are latched at every vblank, so even a 2-3 frame tap is never lost.
- Buttons: B closes any menu or panel, Start opens the menu, Up/Down/Right work the Jukebox, the d-pad no longer turns the camera while a panel is open.

## D33
Options closer to the PS1 (needs speed, day length, message time, title/game music, shuffle, reset game); one home plus owned properties.

## D34
A touch held while the screen changes (title OK, the Map button) no longer picks a block on the Map: each new screen waits for the stylus to lift.

## D35
The PS1's whole buy catalogue: 189 items in the PS1's ten tabs, with the PS1's models (tools/dsbuild/mk_dsitems.py -> source/ds_items.h), names, prices and action words; two-tile beds; a picture of each item's model on its shop card. Short taps are no longer lost (the tap latch read garbage), Buy runs about four times faster, tapping a tall item in Live picks it. See docs/DSI_ROADMAP.md.

## D36
Walkable storeys (up to four), stairs from Buy > Special (Go Up / Go Down in Live), the Floor chip in Buy and Build, the PS1's loft in the map's two-storey homes, old storey shells turned into real floors; gable ends fixed; fps readout on Options > Graphics > Poly meter.

## D37
Travel (menu > Travel: Home, The Man Base, Strange Town, Castaway Island, San Boreas) with the PS1's unlocks, boat and bus stops; Castaway Island at DS size (gathering, campfire, crafting a tent, shelter and raft, the islet, eight secrets); zone saves; the 2D screens drawn off screen and copied in one go; the gravestone.

## D38
FARM LIFE: crop beds, hens, cows and the farm stand with the PS1's rules, the farm ledger (build, market, contracts), seasons, farm lots laid out as on the PS1, crops drawn as they grow; popups with up to eight choices; level of detail by distance from the eye.

## D39 - steady 30
The lot's 3D is recorded as the GPU's own command lists: the still part (ground, floors, walls, things, streets) only when it changes, each Bore's model once for its looks and stride, what moves every frame. The vblank interrupt sends a finished frame by DMA every second vblank: the 3D is a steady 30 frames a second while the game and the touch screen run at up to 60. Faces turned away are skipped by the GPU. The Live top screen redraws only the values that changed. Options > System > Frame rate: Steady 30 / Up to 60. Toolchain: `tools/dsbuild/restore.sh` (prebuilt, from the ds-toolchain branch); bench: `tools/dsemu/tb.sh outdir`.

## D40 - a longer view and a sky
- A sky behind the 3D, drawn by the 2D engine (one colour a scanline, set by hblank DMA, graded to where the horizon is for the camera's tilt). The far land fades into it (the GPU's fog on alpha), so nothing pops in.
- The land goes on past the 3x3: a ring of plain lots with their streets (two slots out on a DS, three on a DSi), then the horizon out to the haze. Each far slot is drawn round its own corner with a matrix move.
- Street lamps on the pavement at the kerb, their lamps out over the street, and a bin in the corner square (as the PS1's fix). Citizens walk the pavements and cross at the corners (they used to walk in the lanes).
- Options > Graphics > 3D look: Classic (as D38), Rich (sky, haze, soft highlights), Toon (Rich with outlines round every thing: each thing has its own polygon id for the GPU's edge marking). Options > System > Detail: Auto / DS / DSi (Auto: DSi when the CPU runs at 134 MHz: more view and twice the crowd). Turbo CPU: Auto / On / Off.
- Saves: unchanged format; the two new options default for older saves.
