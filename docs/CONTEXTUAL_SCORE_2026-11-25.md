# Contextual soundtrack and original jukebox motifs

The eight added cues are now intended as area/menu arrangements rather than
random jukebox additions. The original 52 tracks remain in place and keep
their numbering, source payloads and artist credits. The eight cues remain
available for manual audition. Automatic area music uses the following mapping:

| Cue | Area or menu | Original tracker source |
|---|---|---|
| Sunday on Elm Street | Homes, neighborhoods and quiet life | Whistler Man |
| The Last Bus | Travel and maps; console driving | Closer to the End |
| Pavement Parade | Streets and urban activity | The Cynical Syndication |
| Blue Hour at the Pier | Waterfront, rivers and pools | Worthless Clouds |
| Market Day | Shops, cafes, diners and farms | Gottcho Barracho |
| Rooftops After Rain | Build/Buy and home-management menus | Here Today (Full Version) |
| Neon Skate Night | Skate riding/plazas and bars | Hot Damn |
| Island Morning | Island exploration | Sunman Sunrise |

The build reads the real original BSQ2 sequences and instrument samples from
assets/SONGS.BIN. It ranks two-order pitched phrases, favors keyboard/bass
roles and repeated patterns, retains source pitches and relative rhythm on a
64-step grid, and uses that instrument's decoded sample in the new A and A'
sections. The bridge has the new instrumentation. The backing follows source
bass pitch classes where available. The bank manifest records source song,
artist, order, channel, instrument, original onset count, quoted note count
and a hash of the exact source phrase. The generator fails rather than silently
substituting an invented riff if no usable source phrase exists. Motif choice is
automatic, so recognizable chorus/hook selection and mix quality require
listening tests and manual refinement. This is not a claim of exact full-song
remastering.

Area selection is debounced to avoid restarting music at every boundary.
PS1/DC manual jukebox selections override area playback; Triangle returns
to area music. Radio stations and dedicated cutscene/DJ modes retain priority.
DS/DSi Song=Area and the Jukebox Area chip restore context music; explicit
song selection overrides it. Context tracks loop until a scene changes.
The existing buffers are unchanged; console transitions still use the existing
synchronous loader, so device frame pacing and audio quality need testing.
