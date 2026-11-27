# Native house continuity on the living-calendar base

This is a forward port of the existing, separately reviewed house-rendering
work in PR #15 (head fb9ed157) onto the current content milestone (767fb57).
The earlier branch is diverged from main; copying entire old source files
would remove newer world-first access and calendar features. All production
hunks were applied against exact matching context; the workflow's checks
were inserted without removing the newer content audit. The old visual
smoke script now bounds replay log size.

The rendering changes include clipped partial far models, consistent planned
wall colors at card and box distance, inclusive/exclusive footprint correction,
LOD hysteresis, shape-safe upstairs boxes, and camera-prioritized streaming
roof handoff. These are PS1 and Dreamcast shared-engine changes. Existing
tests and native visual captures are carried forward, but fresh exact-head
results are required before integration. DS and Android have independent
renderers and are not changed by this native-forward-port commit.
