# DS/DSi Skatepark, Ramp Geometry and Stair Perspective — Milestone 0

Verified on 2026-11-21, merged via [PR #7](https://github.com/masong0508-lab/bore2/pull/7) (squash `21d57937e0cb1d109d085b705632e0eff3224465`). This is **implemented game code** in the DS/DSi renderer and physics, not an image mockup.

## What actually changed

- **More faithful curved riding meshes:** the DS generated ramp mesh is no longer only 2/3/4 chords per tile for kicker/launch/quarter pipe; it now uses 3/4/5, interpolated from the *same analytic ramp height profile* that the skateboard uses for collision. No new framebuffer or heap allocations; the existing preallocated height and normal caches have adequate headroom.
- **Stair LOD that still looks like stairs:** Simple/far-detail fallback used to turn eight stair treads into one floor-high box. It now uses four paired risers at the exact real model heights 6, 12, 18, and 24 units, preserving all rotations and the upper-floor landing elevation (`WHF`). Full-detail stair geometry remains unchanged.
- **Previously missing obstacles:** PS1 zone layouts can place fixed-heading kicker variants `'2'` and `'4'`. They were not in the DS catalogue, so their placement returned failure; they now map to the actual Kicker item and PS1-equivalent rotations 1 and 3 (including upper-storey bits). The visible ramp can now be ridden because both rendering and physics see the object.
- **Skatepark identity at a distance:** `LOT_SKATE` map/lot previews formerly took the park tree path, while a nearby city's distant renderer showed a generic crate. Both use a deterministic, low-polygon **paved skate plaza** with quarter banks, a funbox, approach wedges, and a rail. The real playable lot retains its accurate curved geometry.
- **Build-mode collision fix:** the skate physics' cached `SkCell` heights refresh when the existing object occupancy-grid generation (`occ_epoch`) changes, even when the object count is unchanged. Moving/rotating a ramp cannot leave its old rideable surface as the collision target.

## Verified CI evidence

[DS/DSi emulator and ROM run 44962316808](https://github.com/masong0508-lab/bore2/actions/runs/44962316808) **passed**, including native C99 ramp/stair/collision tests, DS ROM compilation, purchase/save/load, title delete, stairs/island actions, eight furnished-home interaction routes, 162-home geometry audit, skating playthrough, and the existing 3-home roof/cutaway atlas.

[PS1 disc and world-coherence run 44962316836](https://github.com/masong0508-lab/bore2/actions/runs/44962316836) **passed**. PS1 skating visuals/controls were **not changed** by this DS-only fix.

The `bore2_ds_skate_visual` artifact contains `skate_520.ppm`, `skate_720.ppm`, `skate_900.ppm`, `skate_results.json`, emulator log and build log. The headless melonDS run used a deterministic `-DAUTOSC=41` skatepark, verified `on_board` across **17 sampled frames**, proved rider movement over a wide area (X range **6,233 DS model units**, Z range **6,490**), and reported **30 rendered fps** (min/mean/max in those samples) with peak **3,788** counted triangles. This is below the smoke-test's 4,096-triangle threshold, **not a physical DS/DSi polygon or FPS certification**. All three 256×384 captures were inspected: they show a skateboarder on the actual plaza with quarter banks and a rail rather than a housing scene.

**Why the test was corrected:** the first smoke attempt poked scene 41 through an emulator RAM hook and silently stayed at a house. Its 3 screenshots and `on_board = 0` caught the mistake; it failed CI. The final test builds the scene as `-DAUTOSC=41`, asserts continued board use and movement, and preserves screenshots. Do not claim the failed initial run tested skating performance.

## Reproduction

```bash
# With the repo's DS toolchain restored, including the headless melonDS runner:
DSRUN=/tmp/dsrun bash ports/dsi/test/ds_skate_smoke.sh "$PWD/out-skate"
# Review out-skate/skate_results.json and skate_{520,720,900}.ppm
cc -std=c99 -Wall -Wextra -Werror ports/dsi/test/ds_render_geom_test.c -o /tmp/ds_render_geom_test
/tmp/ds_render_geom_test
```

## Still to fix (not closed by PR #7)

- **Perspective/camera:** adjust exaggerated close-object projection from matched native screenshots; preserve correct stylus picking and interior visibility. Fix issues locally, not with an untested global field-of-view change.
- **Cross-port parity:** PS1's already more detailed curved skate meshes and the Dreamcast need same-location street/map/skatepark screenshots and rider collision/rotation comparisons. DS screenshot success does not establish parity.
- **Hardware:** benchmark ordinary DS and DSi video timings, GPU/vertex budget and actual feel with manual riding, turning, trick/landing, and grind tests. The emulator's 30-fps samples are not hardware proof.
- **Gameplay collisions:** automate landing/grind/rail combo outcome assertions and old-save placement/rotation fixtures beyond the current `SkCell` cache unit tests.
- **Ramps and stairs at all distances:** the cheap distant landmark approximates the plaza silhouette, not every actual individual placement. Next match same seed/lot/rotation and camera viewpoints across Buy/Build, map and live play.

This remains a **partial Milestone 0 coherence improvement**, not a completed engine replacement.
