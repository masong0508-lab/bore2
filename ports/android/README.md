# BORE 2 Android — neighborhood life (milestone 0.2)

Android compiles the original src/bigmap.c generator using the NDK. The
game draws a depth-tested perspective 3D neighborhood with planned exterior
walls and roofs, shared terrain corners, streets, and local venue schedules.
Orbit and pinch zoom reuse an uploaded mesh; the map remains a navigation tool.

Start at the capital, use Find places to choose a residential lot, travel
there, then lease it through Home. Sleep, cook, practice a hobby or call a
friend at home. Daily rent follows game time and unpaid rent is carried
forward, not silently lost. Work at actual generated workplaces and earn
career bonuses; eat at home or a food venue. Find places also lists local
venues. Meet local residents at generated residential lots and venues during
free hours, learn personality preferences and build persistent relationships.
Call known contacts at home; repeat-chat cooldowns and a daily limit avoid
unbounded rewards. Their activities retain the shared calendar, availability and
effects. Life goals record meaningful early progress. Home, finances, skills,
career, clock, position and progress are saved locally; New life requires
confirmation.

Travel validates the native cardinal route, shows a moving block marker and
a depth-tested resident proxy, then commits the complete trip on arrival.
Canceling or pausing during travel restores the original position with no
charge. This is block-scale navigation, not continuous tile-level walking.

Build from ports/android with JDK 17, Gradle 8.9, SDK 35, NDK 27.2.12479018,
CMake 3.22.1: gradle :app:assembleDebug. CI checks native geometry, pure Java
life rules and resident geometry, then assembles ARM64, ARMv7 and x86-64
APKs and captures emulator frames in portrait, orbit and landscape.

This is a larger playable mobile milestone, not full console parity.
Interiors, real articulated character animation, full console household/NPC AI,
tile-level collisions, native console save interchange, full careers and
audio remain to be ported. Host tests are not device performance measurements.

The emulator life smoke test uses the public UI to select a generated home, travel, lease, sleep, cook and reload the app, checking the actual stored state. Resident identities are mobile-only proxies; this is not yet the console population engine.

## 0.3 home life

Home opens a playable cutaway of the native residential lot rather than an action list. Its floor cells follow bm_lot_foot exactly, including wings and the existing generated entrance where provided. Furnished rooms are deterministically laid out with wall edges, a connecting passage, and collision-safe cardinal routes. Tap a tile to walk or furniture to approach and use it; accessible controls trigger the same routes. Sleep, cook, practice a hobby, call a contact, and rest only after the resident reaches the corresponding furniture. Exit via the entry. Interrupted movements have no unfinished action reward. Save and restore the committed interior cell; the full neighborhood map/3D scene returns on exit. These are mobile-generated ground-floor cutaways, not the original console interior furniture/upper-floor system. Native geometry and standalone route tests plus the real APK housing smoke test cover the new path. Device visual and performance testing remains necessary.

## Outdoor walking pass

Walk mode follows the resident in the existing depth-tested 3D scene. Tap visible ground to route across a 54x54 tile local area (three by three blocks). The native mask excludes building unions, water, pool cells, crop rows, tree trunks and fountain footprints, using the same material/plan helpers as the mesh. Paths use cardinal adjacent cells only; movement interpolates position, facing and a simple leg/arm swing, with ground heights sampled at step endpoints. On cancel/background, only completed tile steps are saved. Long-distance block travel remains separate. This is static outdoor collision, not dynamic NPC or vehicle collision or a full console animation port. The new host path test covers barrier routing and edge cases; native mask checks are in the bridge test. Android device testing is required.

Walk mode has a visible overlay with directional controls in addition to touch-picked ground; these route through the same collision map. The life emulator regression now also walks one tile east outside the rented home and checks that the actual tile and game time survive relaunch. A bridge can be crossed in walking mode; long block travel requires a land block as before.

## 0.5 resident schedules

Mobile residents now occupy one deterministic location at a time: home/sleep, weekday work, lunch and leisure with different personality preferences and opening hours. A bounded land-only block search prevents isolated destinations; the chosen lot is independent of camera position. Meeting locals queries their current scheduled location. Up to 16 depth-tested resident proxies are shown outside nearby active venues, using fixed distinct exterior slots and cached terrain heights. The player's leased home is excluded from the proxy population. The new host test covers unique destinations, daily rhythms, water barriers and bounded stable render snapshots. These are scheduled mobile proxies, not the complete console household simulation or animated crowd navigation. Android music remains unported; console contextual arrangements are preserved in their hidden archive.

## 0.6 playable public buildings

Enter opens a ground-floor cutaway using the same native footprint and front door as the 3D exterior. Nineteen existing public lot types have service-specific furnishings: workstations, food counters, activity stations, social seating, and rest seats as appropriate. Work, meals, and indoor activities approach their station before invoking existing native gameplay checks. Food counters generally operate 07:00–21:00, while lodging and convenience locations remain open. Outdoor parks, pools and farms keep their outdoor activity paths. Home furnishings and save keys are preserved; venue interiors persist their committed tile across relaunch. These are mobile ground-floor layouts, not all floors of the console buildings. The host checks cover 456 synthetic furnished layouts plus native generated footprint/door alignment; the expanded Android emulator test exercises a meal counter and interior reload. Device results are pending.

## 0.7 living streets and daylight

Residents retain their home-based schedules and now move through reachable land blocks during departures, lunch breaks and the return home, using a bounded cardinal commute (six game minutes per block). Conversation lists exclude travelers until arrival. Nearby street proxies independently walk along distinct exterior lanes, with facing and leg/arm motion based on monotonic time. They are still local exterior proxies, not full tile-level autonomous households. The on-demand renderer refreshes moving crowds at eight Hz when visible and resumed; the Crowds toggle keeps people visible without idle animation and persists locally. Terrain probes now use exact triangle bins, capped auxiliary memory with static crowd fallback, instead of repeated whole-mesh scans. The same game clock controls smooth daylight and night colors, without altering music. The new JDK tests verify route barriers, unique location during commutes, bounded gait and lighting, and 10,000+ bit-identical indexed height comparisons. APK, emulator and real-device profiling remain required.

## 0.8 multi-storey buildings

Interior floor counts now match the native building heights used by the Android exterior renderer (one to six floors). The existing ground-floor home layout is retained; larger homes gain study and exercise rooms, and taller public buildings place work and activity stations upstairs. A reserved, reachable staircase landing aligns on every level. The player walks there before changing one floor, and returns downstairs before exiting. The game clock advances two minutes per transition. Saved interiors include the floor index, with old saves defaulting to ground and invalid cells safely returning to the entrance/landing. Find places includes tall public buildings. The planner is tested against 756 synthetic building layouts, and the bridge and emulator workflows now check native floor counts, actual stair movement and upstairs save/reload. The cutaways are generated mobile layouts sharing the exterior envelope, not the original console room-by-room upper floors or separate apartment households. Device compilation and visual QA are pending.

## 0.9 character customization

Character opens a live preview of the actual generated model. Players can choose a name, six skin tones, six hair colors, eight tops, six trouser colors and four hair shapes. Confirm applies the selected appearance to the outdoor character and indoor cutaway; Cancel does not change the saved character. Appearance persists across relaunch and starting a new world. The detailed low-poly model has tapered/bevelled body parts, shoes, facial features and geometry-based hairstyles. The shader rotates four independently tagged limbs about shoulder/hip pivots instead of shearing by vertex height. Residents receive stable appearances from their seed and home ID, with a cheaper six-sided mesh, cached by appearance for the current bounded resident snapshot. The existing static terrain and soundtrack policy remain unchanged. Host tests verify persistence and geometry constraints; the emulator smoke test also exercises outfit changes and relaunch. Full APK and actual visual verification remain pending.

## 0.10 home decorating

The rented home's Decorate home button switches furniture controls into selection mode. Select a piece on the floor or via its button, then tap a free tile; Next spot is an accessible alternative which finds a valid new location. Moves update the actual collision and interaction grid atomically. Doors, stair landings, the player, other furniture and every path must remain clear; rejected changes leave the room unchanged. Room style offers six coordinated flooring, wall and upholstery palettes per floor, and Reset room restores just the current floor after confirmation. Decoration is free and does not advance game time. The saved layout is bound to the world seed and rented home coordinates, with individual invalid floor records falling back to generated defaults. Old saves and public buildings are unchanged. HomeDecorTest covers actual movement, obstructed passages, invalid placement, reset, identity and corrupt-save recovery. The Android gameplay smoke also moves the bed, applies a style, uses the relocated bed and checks persistence. Full SDK and emulator verification are pending.

## 0.11 shared household

At a leased multi-storey home (ground-floor and upstairs bed stations), the Household panel lets the player invite one local known resident. The invitation requires relationship 30, no overdue rent, a valid connected residential origin within three blocks, and a free resident (at home or at leisure). Once accepted, the same resident identity uses the shared address for its work, meal, leisure and commute schedule. Original-home spawning is suppressed in both local conversations and rendered street snapshots. The shared lease receives a fixed $4 daily rent contribution, so the player's portion is $8; existing debt is still paid and contribution never generates cash. Chat together is available only while the roommate is awake at home. The panel shows schedule/status and supports a confirmed move-out returning the resident to its original schedule. Household records are seed/address bound, checked against the contact book and native residential routes on reload, and cleared when the player moves or starts a new life. This is a bounded one-roommate model, not separate playable household members or a complete autonomous economy. Existing decor and music policy remain intact. Host tests cover invitation eligibility, capacity, billing, identity, persistence, commute and single occupancy; CI includes the new regression and existing Android gameplay checks.
