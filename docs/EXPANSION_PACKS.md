# Expansion packs and placeholders

## How packs work today (PS1)
The PS1 disc already has a pack system (tools/packs.py, release/README.md). Every pack is an overlay plus its art, padded to a fixed
slot. The BASE disc holds a stub in each slot (the pack's card says NOT ADDED / COMING SOON); a `.ppf` patch fills one slot. Packs
work alone, in any order. Today: KINGDOM, ARCADE (Bortua Fighter), GOTCHI (Boreagotchi), SUPER, PRISON (crime, law, heist), PARTY, SPACE,
DOG, DJ, PARADISE, FARM, SKYSEA.

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
| REAL WORLD | REAL TIME clock (done), real weather and holidays (idea) | clock done |
| FAMILY TREE 2 | generations, inheritance and memorials for the people (not only pets) | idea |
| HIGH FIDELITY (DS/DSi) | DSi-only extras: more memory, bigger crowds, camera skin colours | idea |

## Per-port plan
- PS1: new content goes in a new pack slot (an overlay and an art file); do not grow the base overlay. Keep more than 10 KB free past `_end` on 2 MB.
- DS: one ROM; packs are save blocks plus code gated by a flag. A pack that is off shows COMING SOON. Keep to the 8 KB save chip: measure.
- Dreamcast: packs are files on the disc (the GD-ROM has room); load them like the PS1's overlays.

## Order to do them
1. Placeholders for every row above (small).
2. Finish `wip-arcade`, `wip-gotchi`, `wip-ds-fight`.
3. Dreamcast extras, then Band and DJ on the DS.
