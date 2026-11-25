# BORE 2 — Neighborhood soundtrack content drop

Eight new, original instrumental compositions: Sunday on Elm Street, The Last Bus,
Pavement Parade, Blue Hour at the Pier, Market Day, Rooftops After Rain, Neon Skate
Night, Island Morning. Each has 48 arranged bars, a contrasting middle section and
reprise, ten generated instrument samples, dedicated percussion, bass, keys, harmony
and melody parts. No external audio or existing soundtrack replacements.

Build commands synthesize and encode the score into the native BSQ2 and BDM2 formats.
The checked-in original songs stay byte-for-byte intact in the generated banks.
All new tracks are selectable in the existing jukebox; console radio stations include
them; handheld song selection expands from 52 to 60. No extra permanent audio buffers.
Per-track PS1 sample/sequence and DS combined bank limits are enforced. Both bank
formats must contain at least 1,048,576 new usable sample/note bytes, excluding sector
padding. The CI artifact records exact measured bytes and note counts, not repository
size or uncompressed estimates.

PS1/Dreamcast/DS build integration writes generated banks outside the tracked assets.
The handheld's generated bank is cached by input digest across emulator builds.
New cartridge bank bounds checks reject invalid reads rather than accessing unrelated
ROM data. Original physical-device performance and subjective mix quality need
verification. Do not treat a successful generator as native audio playback evidence.

## Playable activities and reliability

* Optional sharing a snack ($5, hunger and relationship gains), offering help (12 energy, relationship gains), and telling a story (fun and relationship gains) with new aspirations on both engines. DS household members can also share a snack and tell stories. The console uses an Activities submenu; DS social menus page rather than hide options. Failed affordability/energy checks do not spend interaction allowances.
* DS song directory loads only the necessary 512-byte sectors instead of reading an extra 2 KiB; invalid bank offsets are rejected. Restore executable mode on the DS build script; keep Dreamcast default make target.
* The first host build of the content audit (run 45621152349) measured 1,738,820 console and 1,547,016 handheld playable bytes, 883 seconds. The subsequent pitch-range adjustment avoids handheld upper-note clamping and retains all eight arrangements. Native CI must confirm the updated head and actual audio playback remains unverified.
