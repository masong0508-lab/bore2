# Roof-plane continuity and Android shared terrain edges

- PS1/Dreamcast: fast projected roof and near-plane-clipped roof faces now use the same signed world-space face-plane test, including vertical gable ends and pitched slopes visible from below the eave. Flat caps still require a view from above. No new packets, textures or save fields.
- Android: reuse the previous block's right edge and a single rolling boundary row of terrain heights. A 9x9 window now evaluates 26,569 unique corners instead of 29,241 (2,672 fewer), with 163 extra floats. The existing host verification configuration checks each reused value against direct interpolation. Geometry and mesh materials are unchanged by this cache.
- Extended existing host tests for edge-on visibility, above/below-eave cases and unique terrain sample counts. Require fresh native camera and Android emulator evidence before promoting this commit; no frame-rate increase is claimed without measurement.
