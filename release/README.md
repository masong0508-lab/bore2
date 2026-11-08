# BORE 2 release (PlayStation, save version 47)

What the build (`.github/workflows/build.yml`) uploads, each as its own download:

- `BORE2-base.chd` (about 6.2 MB): the base game, no packs. Every pack's card on the EXPANSIONS shelf says NOT ON THIS DISC (or COMING SOON), and its page says which patch adds it.
- `BORE2-full.chd` (about 6.5 MB): the base game with every pack already patched in.
- `BORE2-pack-<NAME>.ppf` (twelve, one download each): one pack as a patch for the base disc. Each works alone, in any order, and any number together (the build tests every one alone and all of them forward, backward and shuffled).

| Pack | Patch | Size |
|---|---|---|
| KINGDOM (The Land Borefore Time) | BORE2-KINGDOM.ppf | 77 KB |
| ARCADE (Bortua Fighter and 13 cabinets, prize counter) | BORE2-ARCADE.ppf | 154 KB |
| GOTCHI (Boreagotchi!) | BORE2-GOTCHI.ppf | 181 KB |
| SUPER (Superstar) | BORE2-SUPER.ppf | 27 KB |
| PRISON (Crime and Law, heist) | BORE2-PRISON.ppf | 37 KB |
| PARTY (Family Fun) | BORE2-PARTY.ppf | 7 KB |
| SPACE (Space Age) | BORE2-SPACE.ppf | 11 KB |
| DOG (Create a Dog) | BORE2-DOG.ppf | 8 KB |
| DJ (DJ Booth) | BORE2-DJ.ppf | 18 KB |
| PARADISE (Island Paradise) | BORE2-PARADISE.ppf | 11 KB |
| FARM (Farm Life) | BORE2-FARM.ppf | 14 KB |
| SKYSEA (Sky and Sea) | BORE2-SKYSEA.ppf | 61 KB |

Coming soon (cards and reserved slots and save bytes only, no patch yet): REAL WORLD, SPEED CITY, BIG SCREEN, MAGIC AND MYTH, WILD WEST, BORE-OUT MAN!, BOREZIES!, WORLD WAR BORE, BOREPILOGUE.
Crime and Law (PRISON) also holds the heist / underworld game. The ARCADE patch holds both arcade files (ARCADE.OVL and ARCADE2.OVL).

To add a pack: convert the base .chd to .bin (`chdman extractcd`), apply the .ppf (a PPF3 tool, or
`python3 tools/packs.py apply base.bin pack.ppf out.bin`), convert back. Packs only fit the base build they came with:
use the base and the patches from the same build.

Saves: one memory card block per life (a test life is about 6.3 KB; a bigger one takes a second block). Saves from versions 41 to 46 load in this version
(options, money, home, family, farm, arcade scores and Boreagotchi come back); the pack goals start empty.
Screenshots of every pack: docs/packs/ (listed in docs/EXPANSION_PACKS.md).
