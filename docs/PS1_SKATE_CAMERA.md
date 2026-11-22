# Skate Chase Camera Framing — PS1 and Dreamcast (Milestone OW-004)

Implemented in the actual shared game camera by [PR #11](https://github.com/masong0508-lab/bore2/pull/11), squash **`e809132a204d51b68b96a5b18fad73322de0acc5`** on 2026-11-21.

## What changed

Previously, `camera_apply` targeted the rider's precise X/Z coordinate even when rolling quickly. Rails and ramp lips could rush through the center of the view late, particularly at a low chase-camera pitch. The controlled Sim still moves normally, but in **third-person and on a skateboard** the camera's *focus target* now advances a bounded distance along `skate_dir()`:

| Skate engine speed | Camera look-ahead | Notes |
| --- | ---: | --- |
| 0–6 | 0 units | Stationary/manual balance: no drift |
| 10 | 32 units | Early rolling |
| 22 | 128 units | Half of a 256-unit tile |
| 36 or faster | 240 units | Capped below one tile |

This uses the existing 8.8 fixed-point, VBLANK-aware `camera_apply` smoother for settling and dismounting. It adds **no** raycasts, 3D models, per-frame buffers, heap allocations or save fields. The normal camera remains the same when off-board; **first-person is unchanged**. Character physics, height, contact/landing, pathfinding and level streaming use the rider's actual position, not the lead target.

## CI and evidence

[PS1 build plus host C99 tests](https://github.com/masong0508-lab/bore2/actions/runs/44964964631) passed, including `test/skate_camera_test.c` (stationary threshold, monotonic response, maximum 240-unit lead). [Dreamcast compatibility build](https://github.com/masong0508-lab/bore2/actions/runs/44964924189) passed. The patch affects both PS1 and Dreamcast shared third-person cameras, not the DS renderer's independent camera.

**Not yet established:** the practical improvement in player reaction time and the view of every kicker/rail has **not** been independently measured in a native PS1/Dreamcast skating screenshot or gameplay route for this new focus offset. Run the same-seed, same-speed route with the lead disabled/enabled and inspect whether the lip stays visible without hiding the rider. Don't infer FPS or hardware compatibility from the host function checks alone.

## Follow-up

- Pair this with the [PS1 camera perspective regression](PS1_CAMERA_PERSPECTIVE.md), keeping third-person lens/dolly and chase target distinct.
- Save before/after *actual skateboarder* captures at near/mid camera pitch and the six scripted `AUTO_STORY=1, AUTO_SKATE=1` approaches. Verify rails, quarter-pipe lip, landings and tail visibility.
- Compare DS/DSi `on_board` camera, check near/far scenery coherence, and profile PS1 2 MB / Dreamcast real frame times with and without the new framing.
- Add a manual camera setting only if regular playtesting demonstrates a need; don't introduce more toggles without evidence.

Related quality items: **QA-005** skating performance (unverified on physical hardware), **OW-004** camera interaction, **OW-003** native multi-port visual parity.
