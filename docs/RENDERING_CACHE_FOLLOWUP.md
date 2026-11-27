# Rendering follow-up: skyline shapes, crossings and Android mesh reuse

## Scope
- Preserve the real recessed footprint of planned L-shaped houses in the PS1
  and Dreamcast medium-distance skyline. Encode the shape in four unused
  bits of the existing 3-byte cache entry; do not increase the fixed skyline
  cache. Render body and garage wing separately. Very distant dot LOD remains
  coarse; edited irregular local-window buildings are not fully addressed.
- Match DS/DSi intersection pedestrian heights to the four rendered corner
  pavement cells. Split crossing road geometry into disjoint strips.
- Android: reuse block terrain samples for foundations, walls, paths and trees;
  compute each occupied roof corner once rather than separately per face.
  Keep counters in the host mesh test and compare repeated mesh bytes.
- Add host coverage for the shape encoding, actual generated L-house plans,
  all intersection-cell combinations and terrain/roof cache use.

No save format change. Full native compilation and emulator verification
are required for this new revision. No new frame-rate guarantee.
