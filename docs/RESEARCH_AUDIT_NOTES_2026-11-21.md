# BORE 2 — Research and Product-Audit Notes (2026-11-21)

**Purpose:** Keep the underlying reasoning, comparison, QA taxonomy and source links with the game, not only in a chat or temporary research report. Read with [OPEN_WORLD_LIFE_SIM_ROADMAP.md](OPEN_WORLD_LIFE_SIM_ROADMAP.md) and [PRODUCT_QA_AUDIT_ROADMAP.md](PRODUCT_QA_AUDIT_ROADMAP.md).

## Provenance and confidence

The preceding Deep Research session surveyed life-simulation principles and general open-world QA risks. Its product-audit pass initially reported that it could not locate a public BORE 2 repository/build, so it **did not perform dynamic scans, playtests or artifact inspection of the actual game**. On 2026-11-21 this documentation pass located the connected repository **masong0508-lab/bore2** and read its README.md, DESIGN.md and multiple docs/ roadmaps, housing, rendering and port notes. These are primary project records; they are not current hardware benchmarks.

**Evidence categories:**
- **Repository-documented:** historical feature or issue stated by a named project file.
- **Research-supported method:** a testing, design or implementation pattern appropriate to consider but not necessarily implemented.
- **Hypothesis:** plausible failure needing a recorded seed, location, build and reproduction.
- **Verified on current head:** requires an actual build/test/measurement. **None were added by this documentation-only change.**

The Deep Research audit described a broad Unity/Unreal toolset for potential generic games; **BORE 2 is C with a PS1 foundation and custom DS/Dreamcast ports, not an identified Unity/Unreal game**. Prefer C compilation checks, native host sanitizers, generation/collision audits, console/emulator traces and the existing repo test harnesses. Do not take generic CPU time slices or floating-origin advice as mandatory for this project's small procedural world; apply only after diagnosis.

## What makes this life sim feel expansive

**1. World continuity and reachability.** A continuous town matters because households, jobs, stores, parks, social contacts and transportation connect into a single mental map. Continuous visual rendering is optional; **continuity of state and consequences is essential**. In BORE 2 the far view, city plan, near block, room geometry, travel destinations and saved edits must be different presentations of the same facts. A city full of inaccessible buildings is not an open world.

**2. Visible independent lives.** A neighbor follows a routine, changes relationships, visits a venue, makes decisions when the player is absent, and remembers meaningful encounters. The Sims 3 is a reference for playing across a neighborhood; The Sims 2 for personalities, wants/fears and social drama. Implementation choice: offscreen event/schedule simulation with lower cost and full local behavior in view, while never dropping identity or relationship continuity.

**3. Player-initiated aims.** Career, family, creation, exploration, relationships, hobbies and optional stories can all generate their own advancement. Skating is an optional practice that can become a social opportunity/career, not the prescribed default; neither should a story mission be mandatory in Free Life. Achievements can encourage exploration without making normal life feel wrong.

**4. Consequences across systems.** More features are only valuable when they react to one another. For example: work changes income; income affects housing; housing affects available furniture; furniture affects needs and guests; friendship leads to invitations; the invitation creates a memory or new goal. The resulting story is not necessarily a cutscene.

**5. Sense of place and pacing.** Distinct parks, businesses, neighborhoods, homes, roads and residents matter more than multiplying arbitrary map coordinates. A familiar landmark and the neighbor who uses it beat a vast empty procedural tract. Use authored rules within seeded variation: street hierarchy, yard setbacks, door orientation, meaningful rooms and visible routines.

**6. Creative expression and replayability.** Custom characters/looks, homes, build tools, scenarios, household combinations and different world seeds should lead to genuinely different possibilities, not just a larger options list. Clarity in creation/build/phone menus is part of freedom.

## World and graphics defect taxonomy to preserve

| Type | Observable symptom | Likely classes of cause to investigate; not a diagnosis |
|---|---|---|
| Spatial contract | Walls pass through rooms; floor below/above is visible; homes float; stairs fail; mapped lot differs from live lot | Multiple lot/height/facing rules; stale per-port generator; missing support/collision validation; saved overrides |
| Texture mapping | PS1 floors or walls stretch, shimmer or swim near camera | Affine perspective distortion; near-plane cutting; UV interpolation; cutter budget; texture atlas bleed |
| Depth/order | Roof in front of room, z-fighting on DS, scenery flicker | PS1 ordering-table order, DS depth precision, coincident surfaces, draw submission budget |
| Streaming/LOD | Home silhouette or props pop; far geometry contradicts live geometry | Separate far footprint/roof implementation; omitted fallback; invalid draw budget priority |
| Character presentation | Character appears floating, warped side profile, animation snaps, poor occlusion | Incorrect placement height, rotation/scale, collision/ground attachment, pose interpolation, camera projection |
| Navigation | Citizens stuck or walk through furniture/roads; blocked entrance; unreachable upstairs | Path graph and world collision disagreement; obstructed interactable cells; stale target; inadequate replanning |
| Simulation/persistence | Offscreen citizen resets, household mismatch, NPC duplicate, save loses rooms | State ownership, unload/reload rules, save decoding bounds, generator-version migration |
| UI/input | Unreadable labels, obscure buttons, clipping, wrong selected state, slow navigation | Native-resolution text geometry, focus/state feedback, inadequate control affordances |
| Stability | Crashes, memory leaks, audio speed changes, long hitches | Buffer bounds, allocation pressure, per-frame work, synchronization, save read order |

### Why “try a newer graphics engine” is insufficient

The project's limits are real: PS1 ordering-table primitives and a constrained RAM/packet budget, DS fixed geometry and tiny dual screens, and Dreamcast's own performance path. Larger assets, extra framebuffers, expensive per-frame vertex detail or unnecessary heap allocation can reduce actual visual coherence. Prefer canonical geometry, safe occlusion, purposeful decoration, better camera behavior, LOD consistency and measured rendering costs.

## Practical triage procedure

1. **Freeze evidence:** record main commit SHA, build options, port, seed, generation version, town/block/tile, camera, time, input history and save version.
2. **Verify current status:** if a later document says an old problem is fixed, reproduce it before treating it as open. E.g. DREAMCAST.md marks the VMU load desynchronization fixed; DSI_ROADMAP.md D86 marks certain room visual defects fixed.
3. **Find the single source of truth:** use host town/lot generation assertions for geometry and materialize the same lot in all views. Distinguish game-state errors from renderer errors.
4. **Make it repeatable:** minimal deterministic failing seed/reproducer; screenshot and data trace before changing code.
5. **Fix the smallest root cause:** do not hide props, remove floor tiles or disable citizens as a performance “solution.”
6. **Add a guard:** automatic world-generation, save, collision, scene, FPS or object-interaction regression.
7. **Verify the player-visible result:** native-resolution captures on affected ports; retain the before/after and frame time.
8. **Recheck old worlds:** preserve previous seed versions, saved home modifications and old save blocks when introducing new generators/features.

## Initial research audit areas: complete checklist

**Geometry and drawing:** building interiors, roof/stairs, floor elevation, ground/roads, near/far view, street corners, texture UV, LOD transitions, sprite/voxel silhouette, shadow/lighting, water, transparencies, occlusion, clipping.

**Camera and player:** direction and responsiveness, mouse/pad/touch controls where applicable, follow/fixed/overhead/first-person cameras, near-camera collisions, input remapping and sensitivity, HUD visibility.

**NPC/world:** schedule interrupts, time advancement, night/day and shop hours, traffic, offscreen population, conversational context, pathfinding and action queues, story-critical actor priority, shop/home/park activity, household changes, saves and player-directed autonomy.

**World generation:** house facing, block topology, consistent floors, stairs/reachability, object placement, lot type, collision, building/road correspondence, bridges and hills, old-world versions, edited home overrides.

**Persistence:** checksum/length validation, save compatibility and migrations, duplicate state, stream bounds, interrupted save, cold boot, old hardware save images, options, DLC flags and generated world stability.

**Interactions and creation:** placement previews vs actual collision, catalog and room categories, rotation, invalid placement not charged, move/sell/undo-like operations where implemented, character customization extremes and profile consistency.

**Performance:** median/p95/p99 frame time, CPU world/AI/UI costs, PS1 ordering packet overflow/headroom, DS polygon/vertex fallbacks, Dreamcast PVR, audio independent of FPS, long sessions, load stalls, repeated menu loops and transitions.

**Player comprehension:** readable fonts on native screens, strong selection state, clear goals/wants, informative map travel, accessible color/contrast, subtitles/alternative cues, confirm/cancel safety.

**Coverage:** all supported ports, hardware vs emulator, old saves, varied seeds, basic and extended memory configurations, cutscenes, new-game setup, heavily populated cities and empty rural scenes.

## Known source-specific cautions (as documented; not newly reproduced)

- ROADMAP.md reports PS1 city performance issues and a camera perspective trade-off; PS1_PERFORMANCE.md contains later benchmark and texture-cutting changes. Always use the latest matching commit as the baseline.
- ROADMAP.md records house/furniture/upper-floor remaining issues, while HOUSING_DESIGN.md, DS_TOWN7.md and STREETS_8.md record multiple generation, placement and street fixes.
- DSI_ROADMAP.md documents D85 bubbles and D86 furnished-house fixes; D86 explicitly says its old-save/flow tests had not been rerun in that entry.
- DREAMCAST.md explains a previously broken save roundtrip and later fix. An older unfixed line in ROADMAP.md must not supersede the newer resolution.
- README.md and DESIGN.md document existing people, needs, aspirations, direct control, generated places, story chapters, build mode and other systems. The plan should improve quality/integration rather than invent a bare-bones baseline.

## General research references

These explain the **methods and design principles**, not proof of a current BORE 2 defect.

- [Unity — Systems that create ecosystems: emergent game design](https://unity.com/blog/games/systems-that-create-ecosystems-emergent-game-design): modular interactions create emergent loops.
- [Bugnet — Bug tracking for open-world games](https://bugnet.io/blog/bug-tracking-for-open-world-games): coordinates, seed/area context and recent actions make problems reproducible.
- [Bugnet — Regression testing strategies for indie games](https://bugnet.io/blog/regression-testing-strategies-for-indie-games): core smoke tests, old-save fixtures, regression per fixed bug.
- [Gameheim — Bug tracking for game development](https://www.gameheim.com/docs/bug-tracking-for-game-development): version/platform/reproduction/expected-observed issue records.
- [Xbox Accessibility Guideline 103](https://devdocs.xbox.com/build/game-principles/accessibility/xag-deep-dives/xag-103-additional-cues): convey important game information using more than one sensory cue.
- [Unity performance-budget discussion](https://devforgeunity.com/unity-performance-budgets-frame-time-targets/): frame-time budgets are useful as a *principle*; its Unity-specific slices are **not** BORE 2's measured budgets.
- [Epic Unreal static analysis guide](https://dev.epicgames.com/documentation/unreal-engine/static-code-analysis-in-unreal-engine): generic reason to use static analyzers, **not** an instruction to use Unreal tooling for this C game.
- [Floating-point precision overview](https://salivity.github.io/game-development/article/floating-point-precision-in-open-world-game-design): if extremely large coordinates cause jitter, evaluate coordinate precision; do not implement floating-origin without evidence it is relevant.

## Research-to-implementation decisions to retain

- **Quality before scale:** make generated homes, roads and people usable and consistent before raising counts.
- **One place, multiple renderers:** shared world plan, port-specific drawing, deterministic saved overrides.
- **Far simulation is not dead simulation:** cheap updates still preserve life and consequences.
- **Freedom is multiple optional viable activities:** a linear ladder can coexist with sandbox without dominating it.
- **Treat visual cohesion as measurable:** same seed/camera regression images, interaction reachability and frame-time budgets.
- **No false “DONE”:** build/commit/test/capture required. Separate verified fixes, historical issues and proposed changes.
- **Do not lose the narrative:** leave existing docs and per-port logs intact; add durable issue references and merge milestones into ROADMAP.md as they are verified.

**Next research step requiring actual product evidence:** obtain and test the latest executable/ROMs and source head for each port; run the above matrix, record current failures and update the issue register. The repository was found for this documentation pass, but *gameplay testing remains outstanding*.
