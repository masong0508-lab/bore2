# Visible crowds and traffic budget

Based on merged PR #22, whose four native/host pipelines passed.

DS/DSi:
- Rebuild an 81-bin pedestrian bitmask grid once per traffic update. Road
  probes now perform exact distance checks on only nearby candidates, with
  an outside-grid fallback. Do not let upstairs housemates block road traffic.
  Keep existing junction rules and vehicle movement.
- Release junction claims when traffic settings lower the active car count;
  inactive moving cars no longer render as ghosts.
- Cull actors with a deliberately expanded fixed-point camera frustum and
  generous bounding spheres. Stable distance order puts nearby visible
  residents and cars first; reserve all 30/50 triangles before drawing each
  complete actor. No reduced model detail or new model assets.
- Test exact collision result parity against full scans on 20,000 queries;
  assert candidate checks are less than 25% of the full scan in this synthetic
  workload. Independently project sphere sample points through 30,000 random
  cameras to guard conservative visibility, and test stable render priority.

PS1/Dreamcast:
- Before testing indoor state or projecting a citizen, discard candidates
  that cannot enter the existing nearest-visible draw list. Preserve the
  original stable distance selection, including ties; handle a zero draw cap
  without indexing outside the list.

Actual native FPS and visual parity are not established by host tests alone.
The DS patch does not change the resident save layout or traffic quotas.
