#!/bin/bash
# ds_town7_audit.sh : D79 audit of the gen 7 plan homes in the ROM (melonDS; needs /tmp/dsrun and the mkp.sh toolchain).
# Builds -DDSTEST -DDS_T7_AUDIT: when the first lot goes live every plan home of blocks 480..569 x 380..469 is made (lot_enter, both facings) and checked.
# Prints: homes, no bed, no toilet, no fridge, two things on a cell, a thing off the plot, street not reachable from the front doors, a bed/toilet/fridge/stove/sink/bath
# with no free side a sim can step from, done flag; then the homes of each type by facing (t7a_n[type][face], face 1 south 2 north). All the checks must be 0.
R=$(cd "$(dirname "$0")/../../.." && pwd); D=${D:-/tmp/ds79au}; mkdir -p "$D"
[ "$1" = "nobuild" ] || OUT=$D XTRA="-DDSTEST -DDS_T7_AUDIT" bash "$R/tools/dsbuild/mkp.sh" 2>&1 | grep -E "error"
sym() { llvm-nm "$D/bore.elf" | grep " $1\$" | cut -d' ' -f1; }
mk() { b=$((16#$1)); n=$2; o=""; for ((k=0;k<n;k++)); do o="$o$(printf %x $((b+4*k))),"; done; echo ${o%,}; }
M="$(mk $(sym t7a) 9),$(mk $(sym t7a_n) 30),$(mk $(sym t7a_at) 16),$(mk $(sym t7b) 12),$(mk $(sym t7b_at) 24),$(mk $(sym t7b_fill) 1),$(mk $(sym t7b_fn) 1)"
L="100-103:0:212,140;200-203:1:-1,-1"
out=$(DBGMEM=$M ${DSRUN:-/tmp/dsrun} "$D/bore.nds" ${FRAMES:-2400} "$D/o.ppm" "$L" 2>&1 | tail -1)
echo "$out"
set -- ${out#*:}
A=("$@")   # 0..8 t7a, 9..38 t7a_n, 39..54 t7a_at, 55..66 t7b, 67..90 t7b_at, 91 fill, 92 rooms
echo "homes ${A[0]}  bed ${A[1]} wc ${A[2]} fridge ${A[3]} overlap ${A[4]} off-plot ${A[5]} street ${A[6]} noside ${A[7]} done ${A[8]}"
echo "strict: floating ${A[55]} unreachable-room ${A[56]} door-blocked ${A[57]} across-wall ${A[58]} empty-room ${A[59]} bare-room ${A[60]} walk-blocked ${A[61]} no-landing ${A[62]} loft-nofloor ${A[63]} row-of-5 ${A[64]} beds ${A[65]}  (rooms ${A[92]}, avg furnished ${A[91]} tenths x rooms)"
echo -n "first failing lot (bx by) per check: "; for k in 0 1 2 3 4 5 6 7 8 9 10 11; do [ "${A[$((55+k))]}" != 0 ] && echo -n "strict$k=${A[$((67+2*k))]},${A[$((68+2*k))]} "; done; for k in 1 2 3 4 5 6 7; do [ "${A[$k]}" != 0 ] && echo -n "old$k=${A[$((39+2*k))]},${A[$((40+2*k))]} "; done; echo
bad=0; for k in 1 2 3 4 5 6 7; do [ "${A[$k]}" != 0 ] && bad=1; done; for k in 55 56 57 58 59 60 61 62 63 64 65; do [ "${A[$k]}" != 0 ] && bad=1; done
[ "${A[0]}" -gt 0 ] && [ "${A[8]}" = 1 ] && [ $bad = 0 ] && echo "T7 AUDIT PASS (${A[0]} homes)" || echo "T7 AUDIT FAIL"
