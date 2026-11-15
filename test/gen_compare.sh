#!/bin/bash
# gen_compare.sh [OLD_REV] : worlds of the older versions (gen 2..7) must come out byte for byte the same at OLD_REV (default 5963b0a,
# the last commit before world version 8) and in this checkout. Needs gcc and python3. Takes a few minutes; scratch files are removed.
#   SEEDS="1 2 3"   the seeds to use instead of the default (5EED1234 1 C0FFEE 7A11CE 7 and 20 random ones: RANDSEEDS=n changes that count)
#   GENS="6 7"      the versions (default 2 3 4 5 6 7)
#   PACKED=1        also the PACKED towns option for the first seeds
#   LOTS=0          skip the lot generators (lotsim hash); by default they run for the first 10 seeds
# What is compared, for every seed and gen, from the OLD commit's src/ and from this one's, with the same tool sources (ports/host/):
#   gendump  (ports/host/gencmp/gendump.c): settlements, highways, every one of the 1024 x 1024 blocks (terrain, lot, set, road, dice, homes,
#            storeys, height), every lot's plan (neighbourhood, facing, house type, body, wing, door, drive, yard, roof, palette), the
#            footprints, every home's street address, the neighbourhood and place names.
#   townmap sum (the block checksum), and lotsim hash (ports/host/lotsim: every lot of every town made: all storeys' tiles, floors,
#            walls, homes, and with them the items placed).
# Prints PASS or FAIL per gen, and exits 1 on any difference. A FAIL lists the first parts that differ; rerun gendump with DUMP=1 on both to diff text.
HERE=$(cd "$(dirname "$0")" && pwd); ROOT=$(cd "$HERE/.." && pwd); OLD=${1:-5963b0a}
W=$(mktemp -d "${TMPDIR:-/tmp}/gencmp.XXXXXX"); WT=$W/old
cleanup() { git -C "$ROOT" worktree remove --force "$WT" > /dev/null 2>&1; rm -rf "$W"; }
trap cleanup EXIT
git -C "$ROOT" worktree add --detach "$WT" "$OLD" > /dev/null 2>&1 || { echo "cannot check out $OLD"; exit 2; }
SH=$ROOT/ports/host/shim; CF="-O2 -w -std=gnu11 -I$SH"
( cd "$ROOT/ports/host/lotsim" && python3 mkitems.py "$W/items_tab.h" ) || exit 2
for v in new old; do
  S=$ROOT; [ $v = old ] && S=$WT
  gcc $CF -I$S/src -o $W/gendump_$v $ROOT/ports/host/gencmp/gendump.c $S/src/bigmap.c || exit 2
  gcc $CF -I$S/src -o $W/townmap_$v $S/ports/host/townmap/townmap.c $S/src/bigmap.c 2> /dev/null   # (each commit's own townmap.c: it uses only what its bigmap.c has)
  [ -x $W/townmap_$v ] || echo "(townmap does not build against $v: its sum is skipped)"
  gcc $CF -I$S/src -I$W -I$ROOT/ports/host/lotsim -o $W/lotsim_$v $ROOT/ports/host/lotsim/lotsim.c $S/src/bigmap.c || exit 2
done
GENS=${GENS:-2 3 4 5 6 7}; NR=${RANDSEEDS:-20}
NAMED="0x5EED1234 1 0xC0FFEE 0x7A11CE 7"; RS=""
if [ -n "$SEEDS" ]; then NAMED=$SEEDS; else for i in $(seq 1 $NR); do RS="$RS 0x$(printf '%08X' $(( (RANDOM << 17) ^ (RANDOM << 2) ^ RANDOM )))"; done; fi
echo "old $OLD vs $(git -C "$ROOT" rev-parse --short HEAD); seeds: $NAMED $RS"
ALL=0
for g in $GENS; do
  bad=0; n=0; k=0
  for s in $NAMED $RS; do
    k=$((k + 1)); modes="roomy"; [ -n "$PACKED" ] && [ $k -le 5 ] && modes="roomy packed"
    for m in $modes; do pk=""; [ $m = packed ] && pk=packed
      a=$($W/gendump_old $s $g $pk); b=$($W/gendump_new $s $g $pk); n=$((n + 1))
      if [ "$a" != "$b" ]; then bad=$((bad + 1)); echo "DIFFERENT gendump seed $s gen $g $m:"; diff <(echo "$a") <(echo "$b") | head -8; fi
      if [ -x $W/townmap_old ] && [ -x $W/townmap_new ]; then n=$((n + 1))
        a=$($W/townmap_old sum $s $g $pk); b=$($W/townmap_new sum $s $g $pk)
        [ "$a" != "$b" ] && { bad=$((bad + 1)); echo "DIFFERENT townmap sum seed $s gen $g $m: $a / $b"; }; fi
    done
    if [ "${LOTS:-1}" != 0 ] && [ $k -le 10 ]; then
      a=$(GEN=$g $W/lotsim_old hash $s); b=$(GEN=$g $W/lotsim_new hash $s); n=$((n + 1))
      [ "$a" != "$b" ] && { bad=$((bad + 1)); echo "DIFFERENT lotsim hash seed $s gen $g: $a / $b"; }
    fi
  done
  if [ $bad = 0 ]; then echo "PASS gen $g: $n comparisons identical"; else echo "FAIL gen $g: $bad of $n differ"; ALL=1; fi
done
[ $ALL = 0 ] && echo "gen_compare: PASS (gens $GENS identical between $OLD and HEAD)" || echo "gen_compare: FAIL"
exit $ALL
