# BORE 2 — Product QA, Rendering and Stability Audit Roadmap

Established 2026-11-21. Companion to [OPEN_WORLD_LIFE_SIM_ROADMAP.md](OPEN_WORLD_LIFE_SIM_ROADMAP.md). Research archive and references: [RESEARCH_AUDIT_NOTES_2026-11-21.md](RESEARCH_AUDIT_NOTES_2026-11-21.md).

**Important:** this initial register comes from repository documentation and a general audit methodology. It is **not a fresh emulator playthrough**, static analyzer result, measured benchmark, or source-level proof. Older notes sometimes describe bugs that newer notes mark fixed. Every claimed present-day bug needs a reproduction on a named commit.

## Recorded quality checkpoint — 2026-11-21 (PR #3)

The [world-coherence CI gate](../test/world_coherence.py) and [DS town audit enforcement](../ports/dsi/test/ds_town7_audit.sh) are now merged into `main` via [PR #3](https://github.com/masong0508-lab/bore2/pull/3) (squash commit `63a2d2e`). Host checks compare real generated house walls, door and driveway coordinates with their LotPlan and compare actual non-home wall bounds with the `bm_lot_foot` outlines used by maps/far view. Six parser self-tests prevent passing on empty or malformed output. **12/12 host checks passed**, **11,557 lot-check instances**, 0 mismatches on world generations 7 and 8 at seeds `1`, `0x5EED1234`, `0xC0FFEE`. The initial audit reported 4 extra walls per intentional 2×2 garden shed; the checker was refined to accept only correctly placed, complete sheds, not arbitrary stray walls. The DS ROM + gameplay CI passed, including a required generated-home audit of **162 homes with 0 failures**. PS1 disc build/packaging passed.

Evidence: [PS1 and host run](https://github.com/masong0508-lab/bore2/actions/runs/44956101766), [DS run](https://github.com/masong0508-lab/bore2/actions/runs/44955879489). **QA-009 is protected by an initial automated regression gate, not fully resolved:** native gameplay camera/roof/floor captures and shared world/lot render-state audits remain outstanding. Do not infer any change to the actual renderer or new hardware measurements from these tests.

## DS rendering and emulator visual QA checkpoint — 2026-11-21 (PRs #4–#6)

- **QA-009 partial source fix:** [PR #4](https://github.com/masong0508-lab/bore2/pull/4) (`2b0177d`) corrected DS lot-preview street masks, L-garage presentation and outdoor upper-floor visibility in Roofs: Hide. [PR #5](https://github.com/masong0508-lab/bore2/pull/5) (`180dd51`) prevents a *noncontrolled household member* from cutting away the camera's floors/roof. Host C99, DS ROM/gameplay/emulator and PS1 build CI all passed.
- **OW-003 visual smoke gate:** [PR #6](https://github.com/masong0508-lab/bore2/pull/6) (`0749482`) adds [native DS roof screenshot replay and comparison](DS_VISUAL_ATLAS.md) in build-ds CI. The corrected [run 44958873981](https://github.com/masong0508-lab/bore2/actions/runs/44958873981) passed, capturing three generated houses at frames 345, 385 and 425, same seed/control/camera under Cutaway and Always. Changed gameplay RGB channels: **31,442; 35,403; 40,484**. Visual inspection confirmed the difference depicts roofs versus exposed furnished interiors, not just UI movement. An initial flawed timing captured two pre-tour frames; it was fixed before merging and is preserved in the PR conversation as test provenance.
- **Evidence limit:** a successful screenshot *difference* check is not proof that all walls and floors align in every angle, all buildings match PS1/Dreamcast, or gameplay maintains 30 FPS on physical DS. QA-009 remains **PARTIALLY ADDRESSED / MORE COVERAGE REQUIRED**, and QA-004 stays a regression-watch item. Continue same-coordinate cross-view tests, old-save fixtures, native device validation and frame-time profiling.

## DS skate geometry and rideability — verified 2026-11-21 (PR #7)

[PR #7](https://github.com/masong0508-lab/bore2/pull/7), squash `21d5793`, fixes user-reported distorted skate ramps and stairs on the DS/DSi **in the actual renderer**. Simple/distant stairs now draw a four-riser silhouette instead of a single full-height cube; kicker/launch/quarter-pipe curved meshes use 3/4/5 sections instead of 2/3/4; PS1 kicker aliases `'2'`/`'4'` now become functioning, correctly rotated DS kickers rather than vanishing. The map and distant street skatepark now preserve the plaza identity instead of a random grove and box. The cached skate collision surface follows occupied-object edits, so a moved ramp no longer leaves a stale physical slope.

[**Verified DS emulator CI run**](https://github.com/masong0508-lab/bore2/actions/runs/44962316808): all DS gameplay/ROM/audits and existing cutaway atlas passed; 162 checked homes had 0 geometry failures. The actual skateboarder stayed on-board across 17 sampled frames, moved through the skate plaza, and rendered 30 fps min/mean/max in the emulated scene; 3 native 256×384 screenshots were inspected and are downloadable in `bore2_ds_skate_visual`. Peak `polys_last` was 3,788 counted triangles, below this smoke gate's 4,096 limit; this is **not** a hardware vertex/geometry guarantee. [PS1 build + host world coherence](https://github.com/masong0508-lab/bore2/actions/runs/44962316836) passed.

**QA-005 status: PARTIAL FIX + REPRODUCIBLE SMOKE** (no reproduced DS skate FPS dip on this route; other scenes and real console unverified). **QA-009: PARTIAL VISUAL WORLD-IDENTITY FIX**, still needs same-location per-port capture. **OW-004: PARTIAL**, not a global perspective correction. The first trial captured a house with `on_board=0`; corrected `AUTOSC=41` reliably enters the skatepark. Details, exact repro, evidence limits and next tasks: [DS_SKATE_GEOMETRY.md](DS_SKATE_GEOMETRY.md). Do not call old PS1/Dreamcast perspective or all grind/landing behavior fixed.

## PS1 perspective lens / third-person projection checkpoint — 2026-11-21 (PR #8)

[PR #8](https://github.com/masong0508-lab/bore2/pull/8) (squash `0a54fa9`) adjusts the **actual PS1 third-person renderer** from 230 px to 260 px GTE focal length (~69.6°→~63.2° horizontal FOV), with proportional camera dolly to keep the selected Sim/house approximately the same size. This targets the confirmed source-level cause of visibly exaggerated near/far scales on ramps, stairs, rails and character models. First-person and Dreamcast camera behavior are preserved; PS1 render-to-texture people now share the live world lens instead of an outdated 230-px constant.

**Tests:** [PS1 disc/host run 44963154954](https://github.com/masong0508-lab/bore2/actions/runs/44963154954) **passed** build/packaging, existing world-coherence test, and native-C99 focal/dolly tests in both PS1 and Dreamcast variants, including near/far parallax. [Dreamcast disc run 44963102464](https://github.com/masong0508-lab/bore2/actions/runs/44963102464) **passed** for the modified shared rendering/camera source. These are source and compile checks **without a native PS1 playthrough or FPS measurement**. QA-002 is **PARTIALLY ADDRESSED** (FOV distortion only; affine UV warping and close clipping not proven), OW-004 **PARTIAL**, QA-009 **OPEN** for scene-specific roof/interior occlusion and cross-view parity. Follow-up capture matrix: [PS1_CAMERA_PERSPECTIVE.md](PS1_CAMERA_PERSPECTIVE.md).

## DS camera/Live touch ray guard — verified 2026-11-21 (PR #9)

[PR #9](https://github.com/masong0508-lab/bore2/pull/9) (squash `8d3d3a7`) fixes the **source-confirmed** DS/DSi Live 3D ray selection of a floor **behind** the active camera on near-horizon touch locations. The interaction engine now rejects nonforward/parallel/out-of-range ray-plane hits before setting a walk or item target. Build/Buy uses a separate 2D floor plan, so no incorrect claim of a Build/Buy ray mismatch. Host C99 tests check positive/reverse ray time, upper floors, horizon limits and no rejected-target write.

[DS CI run 44963841329](https://github.com/masong0508-lab/bore2/actions/runs/44963841329) **passed** full ROM/emulator input, save, stair/island, 162-home, skate and roof atlas tests. [PS1/host run 44963841433](https://github.com/masong0508-lab/bore2/actions/runs/44963841433) **passed**. QA-009/OW-004 **PARTIAL**: this fixes an identified interaction source issue, but explicit above-horizon gameplay touch captures and complete camera/geometry occlusion testing remain open. [DS touch and camera evidence](DS_TOUCH_RAY.md).

## PS1 and Dreamcast skate chase-camera improvement — 2026-11-21 (PR #11)

[PR #11](https://github.com/masong0508-lab/bore2/pull/11) (squash `e809132`) fixes a third-person skate camera *framing limitation*: the old orbit always targeted the rider, bringing rails and kicker lips toward the visual center only at the last moment while moving quickly. The new target leads by 0–240 world units along `skate_dir()` according to rolling speed, using the already-existing follow-camera smoothing; zero when stationary, 128 units at speed 22, and capped before a full tile. Does **not** change skating contact/physics, model geometry, camera field of view, first person or DS. No runtime allocations.

**CI passed:** [PS1 host+disc](https://github.com/masong0508-lab/bore2/actions/runs/44964964631) and [Dreamcast build](https://github.com/masong0508-lab/bore2/actions/runs/44964924189), including C99 speed/limit regression. **QA-005/OW-004 still PARTIAL:** gameplay footage and direct same-route side-by-side verification of rider, lip, rail and near-camera obstacle visibility are outstanding, as are physical console performance timings. [Repro and open follow-ups](PS1_SKATE_CAMERA.md). Do not declare the entire skate visual/perspective overhaul finished.

## Severity and status

| Severity | Meaning | Release rule |
|---|---|---|
| P0 blocker | Crash, boot failure, save corruption/loss, major memory corruption, irrecoverable progress loss | Stop feature work |
| P1 high | Unusable house/door, floor or world collision failure, catastrophic visual corruption, critical AI stuck, severe sustained FPS issue | Fix before further expansion |
| P2 medium | Camera obstruction, intermittent texture warp, clipping, non-critical path issue, material UI inconsistency, hitches | Schedule measurable fix |
| P3 polish | Minor alignment, animation seam, non-blocking art defect | Polish once gameplay stable |

**CONFIRMED** = current head, clear steps and evidence. **HISTORICAL** = documented earlier, unverified now. **FIXED-REVERIFY** = later documentation says resolved; regression guard still needed. **BACKLOG** = planned/imperfect feature, not necessarily a bug. **HYPOTHESIS** = likely QA risk without project evidence.

## Research-to-project issue register

| ID | Priority | Candidate issue | Evidence / proper current status | Required verification |
|---|---|---|---|---|
| QA-001 | P1 | PS1 city can run below target frame rate | ROADMAP.md describes ~20 fps in city, first-person ~27, goal 25 interim; PS1_PERFORMANCE.md has historical A/B counters. HISTORICAL. | Same-seed current-release 2 MB/8 MB FPS and p95/p99 frame times; geometry/packet/RAM logs. |
| QA-002 | P2 | PS1 close-floor and wall texture warping | Open in ROADMAP.md but later PS1_PERFORMANCE.md records better near-camera texture cutting. FIXED-REVERIFY; may be residual. | Fixed-camera strict/normal/helper shots, moving tile-by-tile. |
| QA-003 | P1 | Inaccessible fixtures, cramped flats, upper-floor reachability blind spots | ROADMAP.md records remaining narrow flats, inaccessible upper-floor audit counts and some venue items; recent housing rounds fix much earlier overlap. HISTORICAL. | Test all lot archetypes, facings and advertised floors using room reachability and interactive object use. |
| QA-004 | P1 | DS missing room furniture, green floor bleed, wall seams | DSI_ROADMAP.md D86 reports polygon-budget/item fallback and floor z-fight fixes with 30 fps capture. FIXED-REVERIFY, not a newly confirmed bug. | Repeat same D86+ native scenes; add explicit save/flow regressions marked unrun in those notes. |
| QA-005 | P2 | DS skate and Gotchi scene frame dips | ROADMAP.md says skate plaza 26–30; DSI_ROADMAP.md says some hatching scenes 20–30. HISTORICAL. | Benchmark current ROM under repeatable camera/input and, when possible, real DS. |
| QA-006 | P1/P2 | DS parity and hardware gaps | ROADMAP.md: unfinished DS fighter/DJ/Band, furniture texture/map art, DSi-mode ROM and real console test. BACKLOG, not necessarily broken. | Distinguish merged/main from wip branches, verify assets and compile paths. |
| QA-007 | P0 regression | Dreamcast VMU save load could regress | DREAMCAST.md explicitly says former save bug FIXED (a read inside MIN was evaluated twice, desynchronizing decoding); older ROADMAP.md is stale. FIXED-REVERIFY. | Cold-boot v53/new VMU saves and PS1 save compatibility; guard read-once decoding. |
| QA-008 | P1/P2 | Dreamcast parity, rendering and performance | DREAMCAST.md and ROADMAP.md document remaining PVR/lighting/fog/perspective and real hardware checks. BACKLOG. | Same-world scene captures, native-pixel rendering and CPU/PVR/audio measurements. |
| QA-009 | P1 | Map/near/far/lot disagreement, off-road scenery, floors through homes, floating actors | Concern motivating this audit; past HOUSING_DESIGN.md/STREETS_8.md contain fixes/audits, but no new reproducible current screenshot here. HYPOTHESIS until reproduced. | Seed, gen version, block/tile, camera, timestamp, native screenshot, collision/path proof. |
| QA-010 | P1 | Platform save and generator regression | ROADMAP.md and DS_TOWN7.md document versioned seeds, save tests and missing real card/console coverage. BACKLOG / QA GAP. | Old world versions, newly generated worlds, edited homes and travel/sleep/load roundtrips. |
| QA-011 | P2 | UI clarity and cross-overlay clipping | ROADMAP.md says font/UI passes done but some overlays and DS views remain. BACKLOG / VERIFY. | Full phone/pie/build/creator/packs screen tour at native resolution, pad/touch. |
| QA-012 | P1/P2 | AI loops, stale schedule, offscreen duplicates, interrupted travel | High-risk systemic scenarios from research, not established current BORE bugs. HYPOTHESIS. | Deterministic one-day/week schedule replays and trace logs before and after save. |

## Reproduction template (required for every real defect)

- Issue ID, **severity**, status, assignee, date and tested commit SHA.
- Port/ROM/build configuration, real hardware or emulator version, graphics settings.
- World generation version, seed, town/block/lot/tile/position, time and relevant save version.
- Player input/action history and exact reproducible steps; frequency; expected vs observed.
- Screenshot/video/native-res frame and debug/log evidence; FPS/p95/p99, RAM, GPU/packet/vertex counts as relevant.
- Root cause **if verified** (otherwise explicitly hypothesis), smallest safe fix, affected ports.
- Regression test, old-save check, current build result, final commit and verified limitations.

## Test and capture matrix

| Suite | How to exercise it | Passing criterion |
|---|---|---|
| Build/boot/smoke | Build each available port; title → create/new life → world → travel → pause/options → save → cold boot → load | No crash, hang, missing asset or core UI lock |
| World topology | Deterministic seeds 1, 0x5EED1234, 0xC0FFEE, 0x7A11CE; version 6/7/8 as applicable; town, island, hills, river, bends | Same street/lot identity in map, far and near; no unsupported/overlapping playable floors |
| Rooms and objects | Ranch, north house, terrace, mansion, flats, hotel, shop, park, skate lot; upper floors; rotate/move/place/sell | Essential fixtures accessible; doors/stairs usable; no collisions with room or occupied tiles; failed placement does not charge |
| Rendering | Exact same coordinates and seed in map, high/low street camera, first-person, near wall, interior, roof-cutaway, distant view | No unexplained geometry mismatch, major occlusion, z-fight, UV bleed, dropped main furniture or floating character |
| Citizens and routines | 5/50/100+ people, work/school, socialize, sleep, shop, time skip, visit, interrupted trip, player return | No duplicate, permanent stuck path, inaccessible story NPC or identity reset |
| Saves/versions | Old world seeds and save corpus, new/edited lot, job/family/traits, different clocks, restart | Save-load result matches original key state; older generation stays intact |
| UI/input/audio | Phone, pie, build, lot map, creator, overlays, pause, long names, DS touch, pad, music while FPS varies | Actions readable, focus visible, no text/button clipping, music timing independent of rendering |
| Stability | 30–90 minute city/crowd/menus/transition soak; traffic and many interactions | No crashes, progressive memory leak, packet/vertex overrun or long unexplained freeze |
| Hardware/compatibility | 2 MB/8 MB PS1, stock DS vs DSi, Flycast vs real Dreamcast when accessible | Per-platform targets met or differences explicitly logged |

**Existing test infrastructure to reuse, subject to verifying paths in current tree:** ports/host/lotsim, ports/host/townmap, test/gen_compare.sh, ports/dsi/test/ds_town7_audit.sh, ports/dsi/test/ds_oldsave.sh, DS scene and traffic tests, ports/dreamcast/test/roadaudit.sh and roadaudit_seeds.sh, ports/dreamcast/ps1_check.sh, PS1 test and tools/perf screenshots/benchmarks. For C host builds consider compiler warnings, AddressSanitizer/UndefinedBehaviorSanitizer and fuzzed generation seeds; do not apply Unity/Unreal-only analysis guidance to this game's engine.

## What to profile rather than guess

**PS1:** 30 fps design goal (33.3 ms/frame), 25 fps near-term city target noted in ROADMAP.md, real 2 MB RAM compatibility, optional 8 MB, overlay sizes, packet overflow and >10 KB stack/heap headroom rule. Log median, p95 and p99 FPS/frame time for each camera, not just averages. Do not improve FPS by silently omitting furniture or rendering floors as holes.

**DS/DSi:** 30 fps gameplay goal where specified, DS geometry ceiling, per-scene vertex/poly count, near/far detail fallback, total RAM, music timing, DS screen legibility. D86 documents approximately 4,650 of 6,144 vertices in its particular house scene—this is *not* a universal allowance. Distinguish standard DS from DSi enhanced mode.

**Dreamcast:** profile SH-4 CPU, PVR scene costs, sound and loading; a Flycast soak in DREAMCAST.md reports a frame-rate range, but emulator host contention makes it non-authoritative for hardware. Aim for consistent smooth play first; higher FPS must not cause simulation/audio timing faults.

## Release gates and triage order

- **Every code change:** build affected ports, run relevant host/audit tests, save regression for changed state, deterministic screenshot for changed rendering, record comparison to prior baseline.
- **Nightly/milestone:** smoke tests for all available images, multi-seed generation, near/far scene matrix, cold-boot save compatibility, per-port benchmark, long emulator soak and native hardware when available.
- **P0 first:** crashes/saves/corruption. **P1 next:** floors, rooms, world/collision integrity, pathfinding and major FPS. **P2 next:** camera, texture warp, interactions and readability. **P3 last:** optional art flourish.
- **Finish definition:** issue is resolved only after the fix is merged, reproduction no longer fails, a permanent regression test is added when feasible, and a current-build screenshot or interactive capture proves user-visible repair. Otherwise mark PARTIAL/BLOCKED rather than DONE.

Keep detailed test results in this document or linked issues. Preserve historical notes; clearly annotate when a newer fix supersedes an older failure.
