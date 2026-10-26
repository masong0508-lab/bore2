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
