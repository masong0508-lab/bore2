# Android rendering milestone 2

The Android view is now a perspective 3D neighborhood preview. Native geometry
comes from shared BInfo, bm_lot_foot, LotPlan, bm_storeys and the world elevation
rules. The 2D map remains available for navigation; no parallel world generator.

- Actual residential footprint unions preserve L-shaped yards and shared terrace
  boundaries. Inclusive plan endpoints are converted to tile edges; only exterior
  wall faces are emitted. Sloping roofs and gables share corner heights.
- Buildings have a continuous foundation, floor-height-consistent windows,
  shaded facades and pitched residential roofs. Roads follow block road bits,
  mega-block internal roads are suppressed, terrain joins at shared corners,
  and public lot footprints come from the same plan as console far views.
- A bounded 9x9 block mesh is generated on a worker only after moving the map
  center, and uploaded to one GLES vertex buffer. Depth testing, perspective,
  distance fog, stable geometry, and a close-range facade-detail budget replace
  flat map rectangles. Orbit and zoom do not regenerate the mesh.
- Tap selection ray-tests the same visible triangles; the selected lot's normal
  world name and calendar activities appear in the panel. Map travel, portrait
  and landscape layouts, and preview seed persistence are preserved.
- Host tests validate footprint and roof rules and deterministic native scene
  geometry on three real generated seeds. Android CI compiles the APK; runtime
  screenshot/device performance and actual interior/character rendering are
  still separate validation milestones.

This is exterior neighborhood rendering, not the full life simulator. Public
lots use the shared building footprint plus preview materials and exterior
details rather than the full console object/room renderer.
