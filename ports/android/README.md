# BORE 2 Android: native world bootstrap (not the complete game)

This first port milestone compiles the existing src/bigmap.c world generator
with the NDK and uses the same v8 world IDs, terrain, buildings, and place
names as the console and handheld versions. The Android UI requests a bounded
native map window on navigation, caches tiles between gestures, and previews
the existing weekday/weekend venue schedule. It preserves the preview seed
and camera coordinates locally, not console memory-card saves.

The app does not yet simulate households, play music, enter buildings, run
the 3D engine or implement the full life simulation. Those remain porting
milestones, not features claimed by this preview.

Build with JDK 17, Gradle 8.9, Android SDK 35, NDK 27.2.12479018 and CMake
3.22.1, from ports/android: gradle :app:assembleDebug. The build-android
workflow runs host tests of the actual native world bridge, then packages a
debug APK for arm64-v8a, armeabi-v7a and x86_64. APK and device tests must pass
before release.

Next: extract platform-neutral life-loop, input and save adapters from the
console/handheld code; render actual lot plans and models using GLES; wire
audio, household simulation and normal object interaction; add device
frame-time, lifecycle and save/reload QA. Do not build a parallel Android
life simulator or a launcher of disconnected expansion games.
