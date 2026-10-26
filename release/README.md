# BORE 2 release

- BORE2-base.chd: the base game, no packs (a pack's card says NOT ADDED).
- BORE2-full.chd: base + every pack already patched in.
- packs/BORE2-<NAME>.ppf: one pack each. Each works alone, in any order, on the base disc.

Packs: KINGDOM (The Land Borefore Time), ARCADE (Bortua Fighter), GOTCHI (Boreagotchi!), SUPER (Superstar),
PRISON (Crime and Law), PARTY (Family Fun), SPACE (Space Age), DOG (Create a Dog), DJ (DJ Booth), PARADISE (Island Paradise).

To add a pack: convert the base .chd to .bin (chdman extractcd), apply the .ppf (PPF3 tool, or
`python3 packs.py apply base.bin pack.ppf out.bin`), convert back. Packs only fit the base build they came with.
