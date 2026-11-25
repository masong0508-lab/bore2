# Living calendar content pass

The next content milestone uses the existing calendar, world services, relationships, needs and farming skill; no separate game launcher and no new save fields.

- Twenty morning arrangements, twenty evening arrangements, and fourteen weekend arrangements are appended to the 72-song bank (126 total). The music follows actual location and day/time. Each arrangement quotes actual original BORE sequence notes and uses a distinct arrangement. Original IDs, existing area scores and per-song hardware buffer limits are preserved. Existing 72-song DS banks remain readable.
- The 24 weekday sessions retain their IDs; 24 weekend events use the same venue equipment, completion checks, skill/relationship rewards and fees. A handheld queued activity stores the chosen ID rather than silently substituting a different session if the day changes. Console homework club uses the existing grade field.
- The host generator caches decoded source phrases instead of decoding the same source for every variation. All added music continues to use the bounded per-song loading paths; the DS directory grows by 4 KiB, not the sample buffer. No arbitrary file padding is counted as content.
- The manifest requires at least 10 MiB of new calendar sequence and instrument bytes in EACH native bank, excluding the existing 20 arrangements, inter-sector padding and duplicated cross-port totals. It fails the build if that threshold is missed. The measured numbers and native sound/visual checks must come from CI, not a size estimate.

Implementation choice: the new 10 MiB target is primarily a contextual score expansion, supplemented by real weekend activities. Full graphical/content and emulator QA remain required before merging.
