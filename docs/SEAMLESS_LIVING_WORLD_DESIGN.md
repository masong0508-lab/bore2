# Seamless living-world direction

BORE 2 already has a deterministic 1024-by-1024-block world, named towns and districts, household identities, daily schedules and a local geometry window. The goal is to make the existing generated structure read as places people live, not a visible grid of unrelated lots.

## Design principles

- Plan at town/district scale first. Respect terrain, connected streets, destination access, shared public space and plausible land use; variation at lot scale should support that plan.
- A landmark should retain its footprint, orientation and purpose as the player approaches it. Share plan data across native geometry, DS preview and Android meshes. Distinct landscape templates are more legible than random scatter.
- Citizens should choose places because of interests, age, distance and opening times. Preserve stable identities and offscreen progression; expand relationships, community events and repeat visits next.
- Keep simulation and display budgets separate. Render nearby activity in detail and represent distant households cheaply. Build/update cached geometry at streaming boundaries, not every frame.
- Activities and careers belong in the places where they happen, with clock, needs and money consequences. Android should grow toward this shared simulation instead of being a separate collection of menus.

The first implementation keeps generator v8 street layout and save identity unchanged. The five existing park templates are shared across render paths; DS uses a paved fountain representation of the pond. Far tree eligibility is cached with the existing terrain cell; population leisure scoring remains capped at ten candidates per decision. Android adds a bounded cardinal route search on actual generated blocks, respecting water/causeway/bridge direction, location and route highlighting, district labels, meals and once-per-day shifts. The route is a block-level approximation, not tile collision or continuous animated walking; shifts are a starting interaction loop, not the full native career system.

## Next topology milestone

A new generator version needs explicit save-version handling and stable migration tests before changing town layout. Test a seed suite covering coast, rivers, hills, rural villages, dense centers and peripheral districts; measure connected roads and reachable venues, household/workplace identity, far/near silhouette agreement and frame time. Add organic district-scale road bends and terrain-informed public spaces without invalidating old saved worlds. Port shared simulation, citizen rendering, tile navigation and interiors to Android incrementally. Visual differences must be explicit where hardware requires them. Never treat a successful build as proof of perfect graphics or measured stock frame rate.
