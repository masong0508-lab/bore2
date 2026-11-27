# Natural landscape integration — first cross-port pass

The block and town identities, terrain classification, save layouts, road network
and expansion availability are unchanged. Elevations now use one bounded,
continuous summit profile rather than a rocky-material height step. The shared
surface uses matching boundary values at every town/country seam and retains
flat building pads; it is sampled by the PS1/Dreamcast world and Android mesh.
Existing saved worlds retain their addresses but can gain different physical
terrain grades. The PS1/Dreamcast camera already follows world height. Android
outdoor walking follows its mesh; Highlands to explore in the place finder
locates rocky countryside within the current map window.

The native map editor uses a soft height brush: center strength two, falloff
one, capacity preflight and one cache reset. PS1/DC retain their 1/3/5 brush
sizes. Saved override format is unchanged. Full Android editing is not included.

The portable test covers 300 synthetic mixed-pad fields, all half-tile
samples, every shared edge, plateau correctness, summit profile monotonicity,
and brush falloff. Full native builds and actual terrain traversal visual
verification remain required. DS live relief integration is a separate pass.
No new climbing mechanic, arbitrary curved streets or expansion-pack gameplay
is claimed by this pass.
