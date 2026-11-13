#!/bin/bash
# roadaudit_seeds.sh [SEEDHEX...]: the road test (roadaudit.sh) over several worlds. One test build per seed (-DAUTO_SEED=0x<seed>u) in /tmp/dc/o_ra_<seed>,
# run headless for 3000 frames; dcrun prints the game's log (DCLOG) at the end, the RA lines of each go to /tmp/dc/ra_<seed>.txt.
# Exit 1 if any seed has anything on a road or did not finish. Default seeds: the test world and three more.
# Needs the dcrun that prints the DCLOG (ports/dreamcast/toolchain/restore.sh builds it; the old prebuilt one did not).
. /tmp/dc/environ.sh
cd "$(dirname "$0")/.."
seeds=${@:-"5EED1234 1 C0FFEE 7A11CE"}
rc=0
for s in $seeds; do
  O=/tmp/dc/o_ra_$s
  rm -f $O/main.o
  make O=$O AUTO="AUTO_CITY=1 -DROAD_AUDIT=1 -DAUTO_SEED=0x${s}u" disc > $O.build.log 2>&1 || { echo "seed $s: build failed"; rc=2; continue; }
  (cd $O && timeout 1500 /tmp/dc/pack/bin/dcrun /tmp/dc/pack/bin/flycast_libretro.so bore2_dc.gdi 3000 /tmp/dc/ra_shot_$s 2>&1 | grep -a "^RA " > /tmp/dc/ra_$s.txt)
  rm -f /tmp/dc/ra_shot_$s*
  echo "== seed $s"; cat /tmp/dc/ra_$s.txt
  grep "RA done" /tmp/dc/ra_$s.txt | grep -q "ON-ROAD 0 walls-on-road 0 roadfloor-off-strip 0 strip-not-road 0" || { echo "seed $s: FAIL or unfinished"; rc=1; }
done
[ $rc = 0 ] && echo "PASS: nothing stands on a road tile, all seeds"
exit $rc
