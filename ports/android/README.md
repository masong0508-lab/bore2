# BORE 2 Android neighborhood preview

Android compiles the original src/bigmap.c generator using the NDK. The
preview draws a depth-tested perspective 3D neighborhood with planned exterior
walls and roofs, shared terrain corners, streets, and local venue schedules.
Orbit and pinch zoom reuse an uploaded mesh; the map view remains a navigation
tool. World seed and preview position persist locally.

Build from ports/android with JDK 17, Gradle 8.9, SDK 35, NDK 27.2.12479018,
CMake 3.22.1: gradle :app:assembleDebug. CI checks native geometry and
assembles ARM64, ARMv7 and x86-64 APKs, then captures actual emulator frames
in portrait, orbit, and landscape views.

This is not yet the complete life simulator: interiors, character animation,
households, normal object interactions, gameplay saves and audio remain to
be ported from the existing engine. Do not infer measured phone performance
from host geometry tests or emulator screenshots.
