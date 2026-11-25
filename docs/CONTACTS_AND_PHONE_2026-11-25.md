# Friends, contacts and phone calls

Built on main a8f2293d, the integrated and previously CI-verified aspiration/social work.

* DS/DSi: Menu > Contacts shows met residents with a positive relationship, including people outside the current map window. Up/down or touch selects a resident; Call strengthens an existing friendship, replenishes the player's social need, and fulfills Call a Friend and Chat aspirations. Calls are available from 8 AM to before 9 PM, once per contact per day, and count towards the existing daily interaction limit. Known visible NPC schedules are respected. For offscreen residents only calling hours and daily limits are known.
* PS1/Dreamcast: a completed phone call or text maintains an existing offscreen friendship when a contact is free, not recently seen, and not asleep/working/in school. Long calls strengthen it more than texts. Existing generic phone behavior remains when no candidate qualifies.
* Handheld: clear contacts when starting a fresh household/world, do not inherit old acquaintances; clear invalid/empty saved wants; preserve existing contact/save layouts. Use a high bit of the existing daily interaction byte for phone-call history; the older code must not interpret a newer save's call bit as a normal conversation counter until midnight.
* Regression: contact table host tests now cover eligibility, daily quotas, relationship gains and no double payout. Existing aspiration host test covers the new want. Native builds, screenshots, actual controller interaction and hardware performance still need verification.
