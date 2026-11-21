# BORE 2 — Living Open-World Life Simulation Roadmap

Established 2026-11-21. **Status: proposed milestones, not an implementation claim.**

This document preserves the life-sim and open-world research as an actionable, version-controlled plan. It supplements [DESIGN.md](../DESIGN.md), [ROADMAP.md](ROADMAP.md), [HOUSING_DESIGN.md](HOUSING_DESIGN.md), [DSI_ROADMAP.md](DSI_ROADMAP.md), [DREAMCAST.md](DREAMCAST.md), and [PS1_PERFORMANCE.md](PS1_PERFORMANCE.md). Technical defects and verification are tracked in [PRODUCT_QA_AUDIT_ROADMAP.md](PRODUCT_QA_AUDIT_ROADMAP.md); sources and provenance are in [RESEARCH_AUDIT_NOTES_2026-11-21.md](RESEARCH_AUDIT_NOTES_2026-11-21.md).

## Vision: a town that actually lives

BORE 2 should let players steer a character, build a home, move neighborhoods, meet neighbors, raise a family, explore, create, take a job (or not), skate for fun (or perhaps turn it into a career), pursue optional stories, and encounter consequences from systems interacting. The world must keep its history even outside the current camera or lot. More terrain and more features do not automatically mean greater freedom.

**The five promises:**
1. **One coherent place.** Lot footprints, roads, heights, interiors, roofs, map, distant buildings, nearby geometry and collisions must agree.
2. **People who persist.** Citizens keep identities, homes, relationships, activities, schedules, needs and memories when out of view.
3. **Player agency.** No default mandatory career, skating progression or linear mission gating in free life/sandbox. Story mode can preserve its designed unlocks.
4. **Emergence.** Daily needs, money, jobs, friendships, mood, events, activities and world changes influence one another in ways the player can observe.
5. **Reliable play.** Stability, legibility, frame pacing and backwards-compatible saves take priority over adding more generated objects.

## Benchmarks and useful trade-offs

| Reference | Lesson to adapt | Do not copy blindly |
|---|---|---|
| The Sims 3 | Seamless-neighborhood illusion, visits across town, living routines, traits, opportunities and non-scripted goals. | Full-detail world simulation is expensive. Use near/far simulation tiers with state continuity. |
| The Sims 2 | Wants/fears, memorable household drama, personality, social interaction and strong object affordances. | Small separated lots make depth easier, but BORE 2 should maintain world identity across transitions. |
| The Sims 4 | Readable controls, character expression and accessible building. | Discrete zones need not erase offscreen consequences. |
| Stardew Valley | Local characters with recognizable schedules, places, attachments and long-term changes. | Adapt relationship/schedule structure rather than unrelated farming genre mechanics. |
| Paralives / inZOI | Modern comparison points for customization, spatial design and presentation. | Verify exact current feature sets and target feasible systems on PS1/DS/Dreamcast. |

The distinctive BORE identity remains: direct 3D control, voxel people, original GBA assets/music, recognizable characters, odd humor, aspiration/wants/fears and substantial PS1/handheld-compatible systems. The repository already documents world generation, houses, lives, stories, items, traits and ports; this is an improvement plan, **not a request to rebuild completed features from zero**.

## Milestone 0 — World coherence before expansion (P0/P1)

- [ ] **OW-001 Shared world contract:** define canonical generation/version/seed, road class, height, lot footprint and facing, room/floor/roof footprint, entrance, object occupancy and saved edits. Map, far view, near world, interior, pathfinding and collision use compatible facts.
- [ ] **OW-002 Invariants:** use existing host lotsim/townmap, road tests and per-port scene audits to catch floor-over-floor bleed, floating houses, absent floors, unsupported roofs, off-road furniture, inaccessible entrances, stairs without a reachable destination, missing essential fixtures, overlapping objects and incorrect north-facing lots.
- [ ] **OW-003 Same-location visual atlas:** save native-resolution PS1/DS/Dreamcast captures for map, street, overhead, interior and distant view at identical seed/lot/time. Compare layout and occlusion, not pixel identity across different hardware.
- [ ] **OW-004 Camera and interaction clarity:** room cutaway, near-plane, perspective warp, profile/scale consistency and controls must show where the player can walk and act.

**Exit:** every selected seed passes documented geometry/route invariants; common lots are recognizably the same across maps and cameras; old-generation saves still produce their original towns.

## Milestone 1 — People with lives, not moving scenery (P1)

- [ ] **OW-101 Stable citizen state:** home, household, location, needs, work or school commitment, personal schedule, relationships, relevant memories and current task remain consistent across travel, load and time skip.
- [ ] **OW-102 Tiered autonomy:** near citizens have full navigation and interactions; offscreen citizens update schedules and important events cheaply. Give known contacts and story-critical citizens priority, as the performance notes already recommend.
- [ ] **OW-103 Social consequences:** friendships, rivalries, invitations, family moves, work, mood and significant events should feed at least two systems (e.g. a social memory affects a later invitation).
- [ ] **OW-104 Interruption recovery:** if a path becomes blocked, an object is taken, a shop shuts, a conversation ends or a story begins, NPCs replan without hanging, duplicating or teleporting through geometry.

**Exit:** a complete in-game week with representative households and 100+ citizen stress scenes has no stuck critical actors, inexplicable identity changes or save/load divergence.

## Milestone 2 — Open-world opportunity, not mere acreage (P1/P2)

- [ ] **OW-201 Neighborhood identity:** distinct but coherent housing, shops, parks, job sites, skate areas, landscape and street patterns. Build on world generations 7 and 8 instead of random scattering.
- [ ] **OW-202 Functional destinations:** each destination offers reasons to visit, services/actions, relevant NPC presence, opening hours where appropriate and feedback after interaction.
- [ ] **OW-203 Optional careers and stories:** preserve story missions as chosen scenarios; free life should allow hobbies, skating, creation, friendships, family, work or aimless wandering without forced career classification.
- [ ] **OW-204 Persistent consequences:** returning after time passes should preserve renovations, family changes, purchases, social outcomes, scheduled visits and world edits.
- [ ] **OW-205 Travel continuity:** roads, traffic, doors, entrances and world/lot transitions should follow the same map and navigation information.

**Exit:** a player can create a multiday personal story through three distinct destinations with no mandatory mission chain, fake/nonfunctional venues or inexplicable resets.

## Milestone 3 — Rich systems, easy to understand (P2)

- [ ] **OW-301 Usable Build/Buy/Create:** clear categories, functional previews, valid placement and rotation, readable object affordances, consistent character/hair/outfit appearance at multiple camera angles.
- [ ] **OW-302 Readable interaction UI:** the phone, map, pie menu, traits, household panel, saves and DS dual-screen controls should give obvious labels, focus, feedback and recoverable exits.
- [ ] **OW-303 Events that propagate:** parties, promotions, rejection, bill trouble, home moves, new neighbors or other events change mood, social patterns, wants/fears, relationships or opportunities.
- [ ] **OW-304 Replayability from systems:** seed, neighborhood structure, life stages, traits, relationships and household context should produce distinct decisions without huge per-scenario code trees.
- [ ] **OW-305 Distinctive presentation:** improve animation transitions, silhouettes, scene lighting and scenery without removing functional geometry or introducing heavy per-frame work.

## Milestone 4 — Port-specific fidelity and release quality (P1–P3)

- **PS1:** retain real 2 MB compatibility and optional 8 MB mode; target 30 fps, with historically documented ~20 fps city and 25 fps interim improvement requiring current remeasurement. Protect overlay/packet buffers, memory margin and ordered-table constraints; tune draw distance and model complexity without hiding essential rooms/props.
- **DS/DSi:** preserve 256×192 two-screen clarity, ordinary DS limits, deterministic rooms and roofs, efficient geometry, readable UI and music timing. Real hardware and enhanced DSi variants need their own verification.
- **Dreamcast:** stronger rendering/fog/depth, larger scenes and input flexibility are opportunities only after parity, VMU save roundtrip and actual GPU/CPU profiling. Emulated FPS is not a real-console guarantee.
- **Future 3DS/others:** first establish build, storage, graphics, input and save tests; then scope parity honestly.
- **All ports:** share state and rules, not incompatible graphics shortcuts. Maintain prior-save migrations and deterministic world versions.

## Suggested next implementation pass

1. Reproduce the open items in PRODUCT_QA_AUDIT_ROADMAP.md on the latest main commit; close stale historical failures already fixed.
2. Build a cross-port seed/location screenshot and room/door/roof test baseline (OW-001–004).
3. Fix reproducible visual/geometry/path failures *before* increasing density or view distance.
4. Instrument and protect current PS1/DS/Dreamcast FPS, memory and save compatibility.
5. Prototype one block, five named citizens and one full day of tiered autonomy before simulating the full population more deeply.

## Updating the document

For each OW item record **status, responsible port(s), issue/PR, tested commit SHA, reproducible demonstration, acceptance test and current blocker**. Only mark completed after the code is merged, the relevant tests pass and a native screenshot or playthrough demonstrates player-visible changes. Keep the existing port histories; update this cross-cutting plan rather than deleting older findings.

**Rule:** coherent world → dependable saves/tests → autonomous people → meaningful places → added breadth and polish.
