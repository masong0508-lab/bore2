# PS1 house continuity: first engine milestone (2026-11-22)

Review branch: `improve/ps1-house-continuity-clipped-far-20261122`, draft PR #15.

## Three source-level discontinuities addressed

1. **Whole-house guard-plane rejection.** The former far-card and distant-box renderers discarded an entire visible house if *any* projected corner failed their guard test. The exceptional path now clips walls and roofs through the existing far-land polygon clipper; ordinary fully visible geometry retains the existing fast packets. Static allocation, save data, assets and procedural seed are unchanged.
2. **Upper-storey LOD flicker.** Nearby and distant upper stories swapped between real tiles and simplified boxes at the same threshold in both directions. A compact hysteresis function now uses a deadband to prevent that rapid oscillation. Existing `b_lod` bytes are reused and reset when building IDs are recycled, so neither memory footprint nor save data grows.
3. **Far-house boundary shrinkage.** The generator's `bm_lot_foot` stores inclusive wall bounds (`x1,y1` are occupied tiles). The prior `draw_buildings` far pass incorrectly treated those as exclusive box edges, making entire houses one tile too narrow in both axes, while the near skyline and the roof rule correctly used `x1 - x0 + 1`. Far houses now convert to the exact same half-open outer edge as the near renderer. The conversion is tested across all 16×16 wall pairs, 32 block locations and camera window origins.

4. **L-shaped courtyard pop-through.** The former upper-storey box LOD used a connected building's bounding rectangle even when its actual indoor footprint was irregular. That could draw fake walls across a courtyard or behind adjacent rooms. The roof-region flood already counts enclosed tiles. Its tile count and the bounding-box area are now compared once when building geometry is generated; a 32-byte bitset marks which building IDs may safely use the fast rectangular LOD. Irregular homes keep their real exterior tile walls instead. The bit is invalidated when a building ID is reused and rederived after rebuild; no per-frame scan or save field is added. The host LOD regression verifies rectangular and incomplete shapes.

## Safety and validation

Tests: `test/house_lod_test.c`, `test/house_footprint_test.c`, and extended `test/projection_far.c`, run as mandatory PS1 `build` host steps (the last via `test/projection_regression.sh`). The existing world-coherence audit, PS1 disc packaging, Dreamcast build, and native PS1 close-home camera workflow remain the acceptance gates.

**Not claimed:** This is not full multi-angle or moving-camera native screenshot validation, proof all houses retain identical silhouettes in every LOD, or a physical PS1 benchmark. Large irregular and edited buildings may still need a footprint-aware multi-mesh impostor. Polygon ordering on intersecting roofs and camera-inside-wall behavior remain high-priority visual work. Preserving the classic PS1 affine texture wobble is intentional; this pass focuses on incorrect world/geometry projection.
