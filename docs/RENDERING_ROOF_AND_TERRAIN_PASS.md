# Rendering follow-up: stable roof profiles and bounded map/terrain work

- PS1/Dreamcast: both fast and clipped distant roof paths share the same
  inset ring as the detailed roof cap. Hip roofs have four slopes and a
  capped top; gable ends retain the wall material. Avoid the old pointed
  pyramid silhouette and excessive ridge height. Flat roofs are unchanged.
- DS/DSi: the same roof run calculation is used in handheld fixed-point
  units. The 13x13 map lot preview draws 49 nonoverlapping checker patches
  or one plain ground/water rectangle rather than 169 individual quads.
  The playable lot renderer and save format are unchanged.
- Android: sample the small block-center height/settlement halo once for
  a 9x9 mesh (144 generator table queries); preserve the existing
  interpolated heights, vertex layout, footprint and materials. Host-only
  validation compares cached values with direct generator lookups; repeated
  scene bytes and earlier geometry checks remain in CI.

The host roof test checks 972 dimension/style combinations plus six named
cases. Full compilation, emulator screenshots and frame-time benchmarks
must be checked for the new commit before release. Android remains an
exterior preview, not a complete life-simulation port.
