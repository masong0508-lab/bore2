# Local Activities and venue soundtrack expansion

A gameplay-first community leisure expansion plus 12 additional venue-specific
arrangements based on original jukebox source phrases. The original 52 songs and
eight area tracks retain their IDs. New soundtrack entries 60–71 are reserved
for the cafe, library, gym, pool, cinema, farm, park, arcade, gallery, hotel/spa,
school and observatory. Automatic area playback selects the venue after its
existing dwell gate; all 20 cues are excluded from ordinary jukebox shuffle.
The build asserts at least 2 MiB of additional playable bank data per format
from these 12 tracks alone, excluding sector padding. Native buffer limits stay
unchanged and the 72-entry handheld directory fits in its existing 8192 bytes.

## Activities
Use Pause > Triangle (PS1/DC) or Menu > Activities (DS/DSi) on a public venue's
lot. Two distinct timed activities per venue: choose a workshop, social outing,
or community service. Costs, age, opening hours, energy, and busy state are
validated before committing funds. Entry fees and energy are spent on start;
cancelled or interrupted activities do not pay completion benefits. Completing
a session adds needs and fulfills the new outing/class/community wants; console
also gains appropriate skill points. Handheld uses its existing aspiration and
trait-event systems rather than inventing a separate skill record. The result
is simulated activity, not 24 new standalone minigames.

No new save structure or permanent audio buffer. Console saves cancel an
unfinished outing; handheld lot travel cancels its existing queued actions.
Opening hours are activity-specific, not new shop business hours. Some
venues are generated less often, depending on the settlement.
Host tests verify eligibility, exact venue matching, time/price/energy gates
and completion. Native menu/controller/real-hardware validation pending.
