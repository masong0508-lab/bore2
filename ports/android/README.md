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
