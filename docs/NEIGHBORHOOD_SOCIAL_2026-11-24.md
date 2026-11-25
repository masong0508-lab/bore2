# Neighborhood social pass

Builds on the verified PS1 and DS jobs for be2d87b8 (PR #17). Adds two goals on each implementation: chat with a townie and make a town friend. Console accepted civilian interactions also restore social need and complete ordinary chat wants. New friendship rewards require crossing the actual threshold. The handheld gives priority to household/invite progression in its eight-choice menu, and negative interactions do not complete chat wants.

DS/DSi: the previous 64-slot contacts table allocated records during rendering and town spawn. Once full, an unknown person could receive a different resident's relationship or moved-in status. Lookup is now read-only, and acquisition occurs only on interaction. A full table may replace only an inactive acquaintance below friend threshold, never a friend, a moved-in person, or today's conversation. If every slot is protected, the interaction displays a clear capacity message instead of silently corrupting another contact. This preserves existing on-disk record sizes, although it cannot restore relationships already misattributed by older builds.

The new host regression checks 5,000 non-mutating lookups, collisions, full table behavior and protection of existing contacts. Native emulator tests and Dreamcast compile still need to run on the new head. No new per-frame assets or save bytes.
