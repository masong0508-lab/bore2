# BORE 2 release

- BORE2-base.chd: the base game, no packs (a pack's card says NOT ADDED).
- BORE2-full.chd: base + every pack already patched in.
- packs/BORE2-<NAME>.ppf: one pack each (twelve; the build uploads each one on its own). Each works alone, in any order, on the base disc.

Packs: KINGDOM (The Land Borefore Time), ARCADE (Bortua Fighter), GOTCHI (Boreagotchi!), SUPER (Superstar),
PRISON (Crime and Law), PARTY (Family Fun), SPACE (Space Age), DOG (Create a Dog), DJ (DJ Booth), PARADISE (Island Paradise), FARM (Farm Life), SKYSEA (Sky and Sea: boats, planes, crashes).
Coming soon (cards and reserved slots only): REAL WORLD, SPEED CITY, BIG SCREEN, MAGIC AND MYTH, WILD WEST, BORE-OUT MAN!, BOREZIES!, WORLD WAR BORE, BOREPILOGUE.
Crime and Law (PRISON) also holds the heist / underworld game.

To add a pack: convert the base .chd to .bin (chdman extractcd), apply the .ppf (PPF3 tool, or
`python3 packs.py apply base.bin pack.ppf out.bin`), convert back. Packs only fit the base build they came with.
