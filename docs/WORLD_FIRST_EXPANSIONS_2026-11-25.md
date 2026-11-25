# World-first expansion access

The Local Activities panel is removed from Pause and the handheld's general
menu. Find a community flag at a venue, then choose its two paid/free sessions
from the ordinary object menu. On handheld the chosen action queues a walk
to the marker, and existing object reachability validates the arrival before
fees or energy are spent. Already visited public lots are retrofitted without
overwriting objects. The original 24 activities and rewards are preserved.

Book trips at a hotel reception, transport stop or boat; island tickets $35,
desert town $20, returns free. PS1/DC phone travel directs players to a venue.
Existing destination unlocks, real location transitions and arrival goals
remain. Hotel room rental and resort management are separate console desk actions.

The existing fictional Grand Theft minigame is entered from a physical venue:
gas station road vehicles, waterfront park boats or space-center aircraft on
console; road vehicles on DS/DSi. Relevant selections are filtered to the
site. Legacy automated scenarios retain the unrestricted debug selector.
Existing persistent heat/crime record and fines remain; high heat or an
active serious case blocks outbound paid trips. Handheld heat now cools each
game day even without opening the minigame. Return home is never blocked by heat.

No new save fields. Marker placement uses existing flags, not new models.
Host tests verify access rules and filtering. Native world placement and
controller/stylus playthroughs still need verification.

The same venue sessions also appear directly on relevant equipment: cafe
counter/coffee maker, library shelves/desks, gym apparatus, pool access,
cinema seat, farm stand, park bench, arcade cabinet, gallery art, reception,
spa furniture, school desks and science exhibits. Existing actions retain
priority in the eight-entry interaction menu; the flag is the fallback.

On PS1/DC, a successful road-vehicle heist now grants an actual usable sedan,
pickup, sports car or scooter when the player chooses Keep. The ordinary
vehicle inventory, fuel, condition, driving and save system are reused. The
first four previously unused hot-vehicle bits track stolen road vehicles.
A theft conviction confiscates these vehicles and clears their parking
records. Duplicate vehicles cannot be kept. DS retains its cash reward path
because player-controlled road vehicles are not implemented there.

## Persistent venue outcomes (follow-up)

Design decision: ordinary venue sessions should change existing life-simulation
state rather than add another independent activity progression system. On
completion, community or high-social sessions can introduce or strengthen a
relationship with one eligible real nearby resident. Same-block/range checks
avoid awarding friendship with someone in the next neighborhood; absent
residents do not generate fake contacts. PS1/DC use the live citizen and saved
population record. DS/DSi use the existing contact table and daily talk limit.
DS/DSi farming sessions grant the normal persistent farming skill; console
children/teens completing the study workshop receive four school grade points,
capped at 100. No extra save fields or per-frame resident scans are introduced.

Host check: test/outing_links_test.c exercises social eligibility, exact range
and block boundary, and the grade cap. Both native build workflows run it.
Native playthrough verification remains outstanding: repeat a community
session with a nearby and an absent citizen, save/reload contact and farming
progress, and confirm the study workshop advances a child/teen grade only.
