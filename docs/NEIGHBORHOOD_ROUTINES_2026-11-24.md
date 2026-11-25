# Neighborhood routines and streaming

Follow-up to PR #17, based on b3e14ec3. The PS1 and Dreamcast share their existing detailed population engine; the handheld still uses a bounded street-level crowd.

* Handheld citizens have deterministic school, early/late work shifts, days off, errands, free time and age-specific rest windows. These are coarse availability states, not a full indoor/pathfinding simulation.
* Crossing a lot now retains citizens still inside the 3x3 map window, discards those outside it, and refills free slots on newly entering fringe lots. Preserve the existing ID generator and contact records; avoid duplicate active IDs. Cancel old lot's queued social references and open social popup.
* Add Ask about day on both implementations. Console replies refer to the real population schedule where available; incoming calls no longer interrupt player work or occur while the caller is asleep, at school or at work. Successful civilian chats refresh contact recency.
* Handheld menu grows from 8 to 10 choices so progression, day conversations and other actions can coexist. Guard stale/invalid targets and validate social choices before spending daily interactions.

No save structure changes or new rendering assets. The DS still uses a finite coarse population and cannot promise global per-person identity persistence: its original hashed 16-bit IDs are preserved for compatibility. Local active duplicate IDs are rejected. Host tests cover the schedule and streaming window calculations; native build, playthrough and performance evidence are required before merging.
