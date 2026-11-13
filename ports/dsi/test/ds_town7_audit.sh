#!/bin/bash
# ds_town7_audit.sh : D79 audit of the gen 7 plan homes in the ROM (melonDS; needs /tmp/dsrun and the mkp.sh toolchain).
# Builds -DDSTEST -DDS_T7_AUDIT: when the first lot goes live every plan home of blocks 480..569 x 380..469 is made (lot_enter, both facings) and checked.
# Prints: homes, no bed, no toilet, no fridge, two things on a cell, a thing off the plot, street not reachable from the front doors, a bed/toilet/fridge/stove/sink/bath
# with no free side a sim can step from, done flag; then the homes of each type by facing (t7a_n[type][face], face 1 south 2 north). All the checks must be 0.
R=$(cd "$(dirname "$0")/../../.." && pwd); D=${D:-/tmp/ds79au}; mkdir -p "$D"
[ "$1" = "nobuild" ] || OUT=$D XTRA="-DDSTEST -DDS_T7_AUDIT" bash "$R/tools/dsbuild/mkp.sh" 2>&1 | grep -E "error"
sym() { llvm-nm "$D/bore.elf" | grep " $1\$" | cut -d' ' -f1; }
mk() { b=$((16#$1)); n=$2; o=""; for ((k=0;k<n;k++)); do o="$o$(printf %x $((b+4*k))),"; done; echo ${o%,}; }
M="$(mk $(sym t7a) 9),$(mk $(sym t7a_n) 30),$(mk $(sym t7a_at) 16)"
L="100-103:0:212,140;200-203:1:-1,-1"
out=$(DBGMEM=$M /tmp/dsrun "$D/bore.nds" 1500 "$D/o.ppm" "$L" 2>&1 | tail -1)
echo "$out"
set -- ${out#*:}
[ "$1" -gt 0 ] && [ "$2$3$4$5$6$7$8" = "0000000" ] && [ "$9" = 1 ] && echo "T7 AUDIT PASS ($1 homes)" || echo "T7 AUDIT FAIL"
