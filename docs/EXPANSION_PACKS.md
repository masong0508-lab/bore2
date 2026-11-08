# Expansion packs and placeholders

## How packs work today (PS1)
The PS1 disc already has a pack system (tools/packs.py, release/README.md). Every pack is an overlay plus its art, padded to a fixed
slot. The BASE disc holds a stub in each slot (the pack's card says NOT ADDED / COMING SOON); a `.ppf` patch fills one slot. Packs
work alone, in any order. Today: KINGDOM, ARCADE (Bortua Fighter), GOTCHI (Boreagotchi), SUPER, PRISON (crime, law, heist), PARTY, SPACE,
DOG, DJ, PARADISE, FARM, SKYSEA.

## Status of every pack (PS1)
"Before" is how it stood at the start of this pass, "after" is now. Every pack has a PACK PAGE (the EXPANSIONS shelf, X on a card, or
the story screen's R1 PACK GOALS), with five goals that pay cash, kept in the save (`pk_ext`).

| Pack | File | Before | After | Notes |
|---|---|---|---|---|
| KINGDOM (The Land Borefore Time) | KINGDOM.OVL | finished | finished | 16 missions, goals on its page |
| ARCADE (Bortua Fighter, 9 new games) | ARCADE.OVL, ARCADE2.OVL | finished | finished | another helper owns the games |
| GOTCHI (Boreagotchi!) | GOTCHI.OVL | finished | finished | another helper owns it |
| SUPER (Superstar) | SUPER.OVL | finished | finished | goals use the band and achievements |
| PRISON (Crime and Law + heist) | PRISON.OVL | finished | finished | counters kept in `pk_ext` (sentences, parole, breakouts, heists) |
| PARTY (Family Fun) | PARTY.OVL | finished | finished | party count and best score now saved |
| SPACE (Space Age) | SPACE.OVL | partly done | finished | panel layout fixed; a flawless flight pays a quarter more and adds astronaut XP |
| DOG (Create a Dog) | DOG.OVL | finished | finished | |
| DJ (DJ Booth) | DJ.OVL | partly done | finished | tip lines shortened (one was clipped) |
| PARADISE (Island Paradise) | PARADISE.OVL | finished | finished | |
| FARM (Farm Life) | FARM.OVL | finished | finished | |
| SKYSEA (Sky and Sea) | SKYSEA.OVL | finished | finished | |
| PACK PAGES | PKINFO.OVL | none | new | the page, the goals, the placeholder cards |

Placeholders (final name, icon, reserved slot, reserved save bytes, item codes; the card says COMING SOON and what it will add):

| Pack | Slot file | Save bytes (`pk_ext`) | Item codes |
|---|---|---|---|
| REAL WORLD | REALWRLD.OVL | 8 | F7 |
| SPEED CITY | SPEEDCTY.OVL | 8 | F9 FA |
| BIG SCREEN | BIGSCREN.OVL | 8 | FB |
| MAGIC AND MYTH | MAGICMYT.OVL | 8 | FC FD |
| WILD WEST | WILDWEST.OVL | 8 | FE |
| BORE-OUT MAN! | BOREOUT.OVL | 8 | none |
| BOREZIES! | BOREZIES.OVL | 8 | none |
| WORLD WAR BORE | WWBORE.OVL | 8 | none |
| BOREPILOGUE | BOREPILG.OVL | 8 | none |

There is no FAMILY TREE 2 pack: one family tree system, in the base game and GOTCHI. Its code F8 is a spare.
Save layout: `pk_ext` is 24 live bytes (goals claimed, then counters) and 104 reserved bytes (written as zeros, read past), from save
version 47. Older saves load with the goals empty. Item codes F7 to FE are the only free ones: do not use them for anything else.

## The rule for unfinished work (placeholders)
Anything not finished ships as a proper placeholder, never as a half-working screen:
1. A menu entry or card with the final name and icon, saying COMING SOON (or NOT ADDED when its pack is off).
2. A reserved slot, save block id and item id range, so finishing it later breaks no save and no other pack.
3. A short line in the card on what it will add.
4. The build checks: saves made without it still load; with it off nothing crashes.

## Placeholders to add now (finish later as packs)
| Name | What it will be | Where it stands |
|---|---|---|
| ARCADE 2 | 8+ new cabinets, a bigger arcade lot, prize list, two-player games | branch `wip-arcade` |
| GOTCHI 2 | the new Boreagotchi UI, content and families | PS1 done in main; DS in branch `wip-gotchi` |
| FIGHT (DS) | BORTUA FIGHTER on the DS | branch `wip-ds-fight` |
| BAND + DJ (DS) | two-deck mixer on the DS | not started |
| DREAMCAST EXTRAS | VMU pet rewards, Dreamcast furniture and outfits, water and glass, night glow, gallery, pads 3-4 | partly in EXTRAS tab |
| REAL WORLD | REAL TIME clock (done), real weather and holidays (idea) | placeholder card and reserved slot (REALWRLD.OVL) |
| HIGH FIDELITY (DS/DSi) | DSi-only extras: more memory, bigger crowds, camera skin colours | idea |

## Per-port plan
- PS1: new content goes in a new pack slot (an overlay and an art file); do not grow the base overlay. Keep more than 10 KB free past `_end` on 2 MB.
- DS: one ROM; packs are save blocks plus code gated by a flag. A pack that is off shows COMING SOON. Keep to the 8 KB save chip: measure.
- Dreamcast: packs are files on the disc (the GD-ROM has room); load them like the PS1's overlays.

## Order to do them
1. Placeholders for every row above (small).
2. Finish `wip-arcade`, `wip-gotchi`, `wip-ds-fight`.
3. Dreamcast extras, then Band and DJ on the DS.
