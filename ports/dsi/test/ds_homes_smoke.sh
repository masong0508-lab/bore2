#!/usr/bin/env bash
# Check that a bounded sample of furnished procedural homes can actually be
# reached from the front hall in a real melonDS ROM, not just geometrically.
# Build with OUT=DIR XTRA="-DDSTEST -DDS_ACT_HOMES -DDS_ACT_HOMES_LIMIT=8".
set -euo pipefail
D=${1:-/tmp/bore_homes}
RUN=${DSRUN:-/tmp/dsrun}
test -x "$RUN" && test -s "$D/bore.nds" && test -s "$D/bore.elf"
address=$(llvm-nm "$D/bore.elf" | awk '$3 == "ah" && !found {print $1; found=1}')
if [[ -z "$address" ]]; then echo "DS HOMES FAIL: missing test symbol ah"; exit 1; fi
base=$((16#$address))
addrs=()
for ((i=0; i<8; i++)); do addrs+=("$(printf '%x' $((base + 4*i)))"); done
M=$(IFS=,; echo "${addrs[*]}")
LIVE="100-103:0:212,140;200-203:1:-1,-1"
out=$(DBGMEM="$M" "$RUN" "$D/bore.nds" 1200 "$D/homes.ppm" "$LIVE" 2>&1 | grep '^f' | tail -1)
if [[ -z "$out" ]]; then echo "DS HOMES FAIL: emulator returned no frames"; exit 1; fi
read -r -a v <<< "${out#*:}"
if ((${#v[@]} < 6)); then echo "DS HOMES FAIL: incomplete counters"; exit 1; fi
echo "DS homes checked=${v[0]}, items tried=${v[1]}, never reached=${v[2]}, already reached/unused=${v[3]}, refused by age=${v[4]}, test finished=${v[5]}"
if [[ "${v[0]}" == 8 && ${v[1]} -gt 0 && "${v[2]}" == 0 && "${v[5]}" == 1 ]]; then
  echo "DS FURNISHED HOMES PASS: sampled objects reachable from front hall"
else
  echo "DS FURNISHED HOMES FAIL"; exit 1
fi
