# Cross-port life and activity parity (2026-11-27)

Android's v0.20 changes are retained in the parent commit. The original console
ports already have their own full native household, career, property, need,
furniture and NPC systems; don't replace them with the mobile preview's simpler
implementations.

This first parity pass uses a compact shared C header (native_life.h) for
conversation context and two-phase activity arm poses. PS1 and Dreamcast use
their existing articulated voxel model, both full and low-detail, and the sprite
cache incorporates task changes without bypassing the existing refresh budget.
The Gait run/activity fields share the original four-byte run slot. The DS/DSi
renderer uses its existing sleeve/forearm/hand boxes with task-dependent lift
and reach; task and quantized phase participate in the geometry cache key.
The handheld does not add geometry or save fields, and far models retain the
previous poses. Explicit townie chats and day questions reflect real schedule
or active chore context. Existing ambient chatter, personality systems,
relationships, daily limits, controls and quieter soundtrack remain intact.

The host rule test exercises 7,488 context combinations and 24 pose samples
under the three build configurations. Actual SDK builds and emulator QA are
required separately; no measured FPS improvement or full mobile-feature
parity is claimed. Persistent meetup planning and the Android-style home
condition panel remain follow-up ports, not features of this pass.
