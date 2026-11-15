# Old worlds stay the same: test/gen_compare.sh

`test/gen_compare.sh [OLD_REV]` (default 5963b0a, the last commit before world version 8) checks out OLD_REV in a scratch worktree, builds the same host tools against that commit's `src/` and against this checkout's, and compares their output for gens 2 to 7 over the seeds 5EED1234, 1, C0FFEE, 7A11CE, 7 and 20 random ones (`SEEDS=`, `RANDSEEDS=`, `GENS=`, `PACKED=1`, `LOTS=0` change that).

- `ports/host/gencmp/gendump.c`: settlements, highways, all 1024 x 1024 blocks (terrain, lot, road, dice, homes, storeys, height), every lot plan (face, house type, body, wing, door, drive, yard, roof, palette, wall), footprints, home street addresses, neighbourhood and place names.
- `townmap sum` of each commit's own townmap.c.
- `lotsim hash` (new): every lot of every town generated, all storeys (tiles, floors, walls, homes, items placed), for the first 10 seeds.

Exit 0 and "gen_compare: PASS" if all are identical; on a difference it prints the first differing lines. Gen 8 is expected to differ (the old code treats 8 as 7), which shows the check can see a change. Takes about 30 minutes for the full set on 2 cores; scratch files and the worktree are removed on exit. It does not run the PS1 world.c (that needs the emulator: test/oldsave.sh and oldsave2.sh load old saves through the real game).
