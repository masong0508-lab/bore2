# Native DS/DSi Visual Atlas — Milestone OW-003

This reproducible native-resolution capture suite complements, but does not replace, the functional geometry, reachability and old-save audits. It is about the **actual emulator output**, not a concept mockup.

## Run and evidence

```bash
# After restoring the repository's DS toolchain and headless melonDS runner:
DSRUN=/tmp/dsrun bash ports/dsi/test/ds_visual_atlas.sh "$PWD/out-visual"
```

On `build-ds` CI, the screenshot capture runs after the regular ROM, save/playthrough and generated-home audits. The `bore2_ds_visual_atlas` Actions artifact contains:

- `cutaway_345.ppm`, `cutaway_385.ppm`, `cutaway_425.ppm` — first three generated homes in the same scripted tour, controlled Sim inside and Roofs: Cutaway.
- `always_345.ppm`, `always_385.ppm`, `always_425.ppm` — the identical tour, seed, frames and cameras in Roofs: Always.
- `visual_atlas_results.json` — 256×384 frame checks and the count of different RGB channels on the bottom 256×192 **gameplay** image; build/emulator logs accompany it.

`DS_TOUR` selects the same representative generated-home sequence for both test builds. The same scripted touch/key sequence enters Live, and the camera is placed in the actual home footprint. Missing/blank frames, or fewer than two tour frames with materially different Roofs: Always/Cutaway images, **fail the workflow**. The initial timing (frames 265/305/345) only reached the scripted house tour by its final frame; the gate now samples later frames specifically to avoid claiming coverage from pre-tour captures. The test builds are temporary and do not change normal ROM assets, memory allocations or old saves.

## Evidence limitations and next targets

This is a **visual smoke gate**, not an automatic photographic guarantee of perfect geometry. A difference between cutaway and roofed screenshots indicates the modes produce distinct in-game world pixels; it does **not** by itself prove no clipping, perspective warp or absent furniture. It also does not certify native hardware performance or exact parity with PS1/Dreamcast.

Next enlarge the same-seed capture matrix to: map lot preview, near street, overhead, first-person, both north/south house facings, L-wing garages, upper floors, household member indoors while controller outside, and high/low camera pitch. Manually inspect artifacts, annotate the tested commit and coordinates, then add targeted assertions for any reproduced defect. Keep DS polygon/vertex, FPS, and RAM measurements alongside screenshots before increasing detail budgets.

Related: [Open-world roadmap](OPEN_WORLD_LIFE_SIM_ROADMAP.md) (OW-003/004) · [Product QA audit roadmap](PRODUCT_QA_AUDIT_ROADMAP.md) (QA-004/009) · [DSI roadmap](DSI_ROADMAP.md).
