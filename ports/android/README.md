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
venues. Their activities retain the shared calendar, availability and
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
Interiors, real articulated character animation, other households/NPC AI,
tile-level collisions, native console save interchange, full careers and
audio remain to be ported. Host tests are not device performance measurements.
