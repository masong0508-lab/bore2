# Cross-port rendering integration

This branch combines the native rendering milestone (PR #28, 7fad39ff)
and Android 3D milestone (PR #27, 48d7d84c), both based on living-calendar
content PR #26. Neither prior milestone is removed. Both exact heads had
successful builds and visual regression results before this integration.

## New rendering fixes

- DS/DSi: restore a solid neighboring-building shell that had been accidentally
  commented out in far_lot. Planned map and distant lot previews share the
  actual playable DS house/public-room footprint, including exposed L wings
  and north-facing lofts. Walking collision uses the union rather than a
  rectangle spanning the open yard. Single-storey public halls match the
  current simplified handheld room generator; the console's higher tower
  count is not faked on DS. Draw whole shells before optional roof detail
  within the polygon and display-list limits. Native atlas captures now
  verify actual emitted solid shell triangles, and host tests check shapes.
- PS1/Dreamcast: clip only roof faces when a projected roof ridge is invalid
  but the building's eaves are valid; don't discard the roof or duplicate
  the wall packets. Includes prior LOD, clipping, and streaming improvements.
- Android: replace overlapping coarse road/water overlays with one stitched
  height-sampled surface mesh. Flat equal-material cells are merged; slopes
  retain exact shared corner heights. Roads, pavements, mega-block internal
  courtyards and river crossings follow the generated world layout. Match
  native neighborhood wall and roof colors and front/side roof orientation.

No new save fields. Native render tests on the merged head, Android device
performance, and whole-game playability still require verification. Android
remains an exterior neighborhood preview, not the full life simulation.
