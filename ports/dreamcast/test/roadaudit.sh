#!/bin/bash
# roadaudit.sh ["extra flags"]: the road test. Builds the Dreamcast test build with -DROAD_AUDIT=1 (src/world.c: every block the generator makes, over every
# town, highway and a grid of open country, is checked against the one definition of road / kerb / plot, world_tile_class), runs it headless and FAILS
# (exit 1) if any prop or wall stands on a road tile, if a street strip is not painted road, or if road is painted where the map has no street.
# `roadaudit.sh -DROAD_NOPURGE=1` leaves the clean-up off, to see the test catch what the generators leave.  Needs ports/dreamcast/toolchain/restore.sh done.
# (The PS1 test build has no RAM left for the sweep: AUTO_CITY alone leaves under 5 KB past _end. The code is shared, so this is the PS1's test too.)
. /tmp/dc/environ.sh
cd "$(dirname "$0")/.."
O=${O:-/tmp/dc/o_roadaudit}
make O=$O AUTO="AUTO_CITY=1 -DROAD_AUDIT=1 $1" disc > $O.build.log 2>&1 || { tail -5 $O.build.log; echo "build failed"; exit 2; }
out=$(cd $O && timeout 1500 /tmp/dc/pack/bin/dcrun /tmp/dc/pack/bin/flycast_libretro.so bore2_dc.gdi 3000 /tmp/dc/ra_shot 2>&1 | head -c 3000000 | grep -a "^RA ")
rm -f /tmp/dc/ra_shot*
echo "$out"
echo "$out" | grep -q "RA done" || { echo "audit did not finish"; exit 2; }
echo "$out" | grep "RA done" | grep -q "ON-ROAD 0 walls-on-road 0 roadfloor-off-strip 0 strip-not-road 0" || { echo "FAIL: something stands on a road"; exit 1; }
echo "PASS: nothing stands on a road tile"
