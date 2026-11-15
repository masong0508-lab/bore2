#!/bin/bash
# ds_traffic.sh : d83 the DS street traffic audit in melonDS (needs /tmp/dsrun or DSRUN and the mkp.sh toolchain).
# Builds -DDSTEST (car_audit in city_step) and lets the lot's city run FRAMES frames (default 8000). car_flt counts, per car and step:
#   0 two cars closer than 11 units (0.7 cell), 1 a car on a straight not on its lane line, 2 two cars in one junction with crossing ways (or a turner and any other),
#   3 a car held 600 steps (20 s), 4 5 unused, 6 cars on the road (sum over steps), 7 steps.
# PASS: 0 1 2 3 all zero and cars were on the road.
D=${D:-/tmp/ds83tr}; RUN=${DSRUN:-/tmp/dsrun}; R=$(cd "$(dirname "$0")/../../.." && pwd); mkdir -p "$D"
[ "$1" = nobuild ] || OUT=$D XTRA="-DDSTEST" bash "$R/tools/dsbuild/mkp.sh" 2>&1 | grep " error"
b=$((16#$(llvm-nm $D/bore.elf | grep " car_flt\$" | cut -d' ' -f1))); M=""; for k in 0 1 2 3 4 5 6 7; do M="$M$(printf %x $((b+4*k))),"; done
out=$(DBGMEM=${M%,} $RUN $D/bore.nds ${FRAMES:-8000} $D/o.ppm "100-103:0:212,140;200-203:1:-1,-1" 2>&1 | tail -1); echo "$out"
set -- ${out#*:}
echo "overlap $1  off-lane $2  junction conflict $3  held 20 s $4  car-steps $7 over $8 steps (average $(( $7 * 10 / ($8 + 1) )) tenths of a car)"
[ "$1$2$3$4" = 0000 ] && [ "$7" -gt 0 ] && echo "TRAFFIC PASS" || { echo "TRAFFIC FAIL"; exit 1; }
