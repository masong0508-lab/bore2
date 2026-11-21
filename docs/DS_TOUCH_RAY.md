# DS/DSi Live Touch-Ray and Camera Interaction Coherence

**Merged:** [PR #9](https://github.com/masong0508-lab/bore2/pull/9), squash `8d3d3a73411f2af804cf2b97a8c996503126c7c5` (2026-11-21).

## Why it mattered

In Live mode, `pick_cell_h` projects a touch location through the active 3D orbit camera to the controlled Sim's ground or upper-floor plane. This is a genuine part of the interaction engine: the result can select an NPC, furniture, a stair landing or the Sim's walk destination. The original code rejected rays parallel to the plane but did **not** reject an intersection with `t < 0` — the surface lay **behind the camera**. Depending on a low-pitch camera and near-horizon touch, the player could select a cell that was not in front of the view, making the touch controls feel displaced.

## Changes

- `ports/dsi/source/ds_render_geom.h`: `ds_touch_plane_hit` accepts only a forward ray-plane contact (positive `t`) and rejects nearly parallel rays and unreasonably distant intersections (more than 64,000 scene units). No new arrays or allocations.
- `ports/dsi/source/main.c`: Live's `pick_cell_h` uses that checked intersection instead of accepting a negative ray time.
- `ports/dsi/test/ds_render_geom_test.c`: tests forward/behind-camera rays, rays parallel and nearly parallel to the plane, horizon-reach limits, no target writes after rejection, and downward/upward rays relative to a selected upper-storey plane.
- Camera yaw, pitch, lens, valid scene-cell mapping, skating physics, map/world state, save format, and Buy/Build controls are **unchanged**. This is an input reliability fix, not a new rendered perspective mode. The Build/Buy stylus uses its own 2D floor plan; it is not affected by the Live ray fix.

## CI verification

[DS ROM/emulator run 44963841329](https://github.com/masong0508-lab/bore2/actions/runs/44963841329) **passed**: C99 geometry and interaction tests, ROM compilation, buy/save/load/job, save deletion, stairs/island use, furnished-home routes, 162-home geometry audit, skating motion/FPS budget test, and native roof/cutaway screenshot atlas.

[PS1 disc and host world-coherence run 44963841433](https://github.com/masong0508-lab/bore2/actions/runs/44963841433) **passed**. This DS-only change does not alter the PS1 or Dreamcast 3D cameras.

**Acceptance limit:** mathematical ray test and existing emulator playthroughs prove the change compiles and doesn't break sampled gameplay; no **new dedicated tap-on-horizon vs tap-on-floor screenshot comparison** was added to this PR. Revisit with a fixed-camera native DS tap-route to verify active markers and selection at the top edge. Further same-seed perspective work is in [PS1_CAMERA_PERSPECTIVE.md](PS1_CAMERA_PERSPECTIVE.md), [DS_VISUAL_ATLAS.md](DS_VISUAL_ATLAS.md), [OPEN_WORLD_LIFE_SIM_ROADMAP.md](OPEN_WORLD_LIFE_SIM_ROADMAP.md), and [PRODUCT_QA_AUDIT_ROADMAP.md](PRODUCT_QA_AUDIT_ROADMAP.md).

## Future camera quality work

- Add deterministic emulator taps at above-horizon/ground/upper-storey positions and assert that invalid taps cannot set `mark_t` or move the selected Sim.
- Use one documented optical contract per rendered camera and its picking/UI projections. If the DS 40° vertical FOV is adjusted, update `gx_cam`, `pick_cell_h`, `pick_obj`, and `bub_proj` together.
- Verify view angles with actual captured ramps, staircases, roofs and rails rather than only proving that camera coordinates are mathematically consistent.
