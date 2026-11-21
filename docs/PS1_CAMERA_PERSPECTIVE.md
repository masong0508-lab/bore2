# BORE 2 — PS1 Third-Person Perspective / Near-Camera Coherence

**Implemented and merged:** [PR #8](https://github.com/masong0508-lab/bore2/pull/8), squash commit `0a54fa9bba264591cddd8e87d62c8ea406a4c409` (2026-11-21). This is a change to the **real PS1 game engine**, not a mockup.

## What changed

The PS1 previously used a fixed **230 px GTE projection focal length**, on a 320px-wide screen (~69.6° horizontal FOV), for every third-person and first-person camera. That relatively wide perspective exaggerates the apparent size gap between the near and far ends of skate rails, ramps, stairs, and buildings.

- **Third-person orbit camera:** uses **260 px** GTE focal length (~63.2° horizontal field of view) while the eye is moved away from the target by the same **260/230 factor**. This compensating dolly keeps the selected Sim/house at approximately the same on-screen size while reducing near/far perspective distortion. The implementation uses inexpensive integer math, does not edit geometry or world positions, and preserves manual camera zoom input behavior.
- **First-person:** retains the 230 px lens to preserve its established head-height/collision/near-plane handling.
- **Dreamcast:** retains its previous 230 px projection and camera distance, including the existing separate Dreamcast tighter-camera handling. It was compiled in CI after changing shared source files.
- **People-as-sprites and crowd LOD:** render-to-texture impostors temporarily scale the **current world focal length** instead of always using 230. They then restore the chosen world lens, preventing characters from disagreeing with the geometry in third person.
- **Safe context restoration:** the GTE projection returns to baseline 230 after the frame so that Build/Create/menus and other screens cannot inherit the third-person view. No new images, framebuffer allocations, save versions, heap allocations or gameplay collisions.

Implementation: `src/camera_lens.h`, `src/main.c`, `src/gfx.c`, `src/gfx.h`. Independent regression: `test/camera_lens_test.c` is a required host C99 check in the `build` workflow for PS1 and `-DBORE_DC`.

## Tested

- [PS1 disc + host world-coherence CI 44963154954](https://github.com/masong0508-lab/bore2/actions/runs/44963154954): both **success**. The host C99 camera tests passed at focal-subject distances 600–7000 units: distance-proportional lens framing, compressed near/far parallax, unchanged first person and unchanged Dreamcast behavior. PS1 base and pack discs compiled and were uploaded.
- [Dreamcast full disc CI 44963102464](https://github.com/masong0508-lab/bore2/actions/runs/44963102464): **success** for the shared `gfx.c`, `gfx.h`, `main.c` source changes. That run was on the last commit touching Dreamcast-relevant source; subsequent PS1-workflow-only commits did not alter those files.
- **Limits:** mathematical camera tests and compiling a disc do **not** prove that native in-game perspective, nearby walls, roof cutaways, collision camera, visibility or frame timing look correct. PS1 DuckStation close/mid/high before/after screenshots and ordinary PS1/PS2 hardware playthroughs are still pending. No DS lens change was made in this pass.

## Visual acceptance matrix (OW-004 / QA-002 / QA-009)

Capture the **same seed/lot/player state**, no scripted motion, and fixed light/time for the baseline main parent `dc54597c` and changed squash `0a54fa9`. For each: third-person orbit with 600 / 1500 / 3600 logical camera distances, front and side view of two stair facings, all three skate ramp kinds at two rotations, a long metal rail, a narrow indoor room, a staircase landing, first-person and an outdoor street corner. Preserve native 320×240 capture, camera HUD, FOV/eye positions, projected pixel bounding boxes, roof clipping, and two per-frame CPU/packet counters. Compare **object shape, continuity, placement and controlled movement**, not just how many pixels differ. Check same-viewport controls and FPS, and perform real hardware tests before claiming console-quality improvement.

Relevant scripts `test/ds.sh`, `test/ab.sh`, and `test/gviews.sh` use a DuckStation regression runner external to CI; do not claim these were executed in PR #8. The DS/DSi camera remains at its own 40° vertical view (and the corresponding picking equations); changing it will require matching its touch rays, HUD projection and proof with its native `DS_VISUAL_ATLAS.md` before merge.

## Further perspective tasks

- Compare fixed-camera captures and prevent **wall/roof occlusion regressions** at the new slightly farther eye location.
- If the PS1 outdoor/indoor trade-off is undesirable in a specific scene, implement validated scene-aware focal/eye constraints instead of undoing the whole graphical upgrade.
- Verify PS1's affine textured stair and ramp surfaces and close GTE near-plane clipping: a narrower lens reduces geometric exaggeration but **does not itself fix affine texture warp**.
- Apply equivalent level-of-detail shape and visual-picking contracts to DS, with real emulator captures and touch-pick tests; review Dreamcast's native PVR projection and geometry.

This is a verified **source/CI partial Milestone 0 change**, not a certified full 3D renderer replacement.
