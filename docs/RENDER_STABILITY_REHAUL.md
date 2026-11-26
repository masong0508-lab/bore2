# Render stability first

- PS1/Dreamcast: floor merging is restricted to valid cells and cannot hide an exposed quadrant. Camera-adjacent buildings have one geometry owner. Local ground is emitted before optional detail and clips at the near plane. Nonrectangular roof regions cannot use the single bounding slab. No changes to saved data are required.
- DS/DSi: cache pieces are rebuilt for the same lot/geometry snapshot, rather than assembling new walls with old furniture or surrounding streets. Discard queued geometry before lot/mode/options transitions. Each static list establishes its own GPU state. A segment overflow must not submit a malformed chain. Never overwrite a referenced character model.
- Dreamcast: protect PowerVR texture pages referenced by the CPU-side submission from eviction or in-place modification, including changes during packet traversal. Texture pressure is counted rather than silently substituting unrelated textures. Opaque primitives are not blocked by a full translucent list.
- Android: pool, paths, farm and skate surfaces belong to a single terrain mesh; fountain rims are disjoint. Prefer 24-bit depth with old-device fallback. This addresses depth fighting without deleting visible terrain.

These are source-level corrections, not a claim of flicker-free play. Verify long moving-camera, lot-crossing, texture-pressure and dense-neighborhood runs on every platform before release. Save compatibility is no longer a design constraint for future changes, but intentionally deleting existing save files is unnecessary.

## Temporal regression coverage

The moving-house test now captures 53 native 320x240 images, including two 25-vblank bursts. A conservative A/B/A comparison at multiple time offsets flags large, brief flashes even when widely spaced screenshots appear normal. Ordinary gradual camera motion and constant images are exercised by synthetic detector self-tests. This is a PS1 emulator smoke test, not evidence that every kind of flicker has been eliminated; read the saved results and inspect the burst captures.
