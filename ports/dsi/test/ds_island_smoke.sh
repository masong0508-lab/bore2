#!/usr/bin/env bash
# ROM-level stairs and island-object smoke test under headless melonDS.
# Build first: OUT=/tmp/bore_ds_island XTRA="-DDSTEST -DDS_ISL_TEST" tools/dsbuild/mkp.sh
# Run: DSRUN=/tmp/dsrun bash ports/dsi/test/ds_island_smoke.sh /tmp/bore_ds_island
set -euo pipefail
D=${1:-/tmp/bore_ds_island}
RUN=${DSRUN:-/tmp/dsrun}
test -x "$RUN" && test -s "$D/bore.elf" && test -s "$D/bore.nds"
sym() { llvm-nm "$D/bore.elf" | awk -v n="$1" '$3 == n {print $1; exit}'; }
mem() {
    local addr
    addr=$(sym "$1")
    if [[ -z "$addr" ]]; then echo "Missing ROM test symbol $1" >&2; return 1; fi
    local base=$((16#$addr)) i
    local -a addrs=()
    for ((i=0; i<$2; ++i)); do addrs+=("$(printf '%x' $((base + 4*i)))"); done
    local IFS=,
    echo "${addrs[*]}"
}
M="$(mem isl_res 16),$(mem isl_n 16),$(mem isl_tdone 1)"
LIVE="100-103:0:212,140;200-203:1:-1,-1"
out=$(DBGMEM="$M" "$RUN" "$D/bore.nds" 2500 "$D/island.ppm" "$LIVE" 2>&1 | grep '^f' | tail -1)
echo "island interactions: $out"
if [[ -z "$out" ]]; then echo "DS island smoke FAIL: no emulator frame data"; exit 1; fi
read -r -a vals <<< "${out#*:}"
if (( ${#vals[@]} < 33 )); then echo "DS island smoke FAIL: incomplete result block"; exit 1; fi
fail=0
for ((i=0; i<16; i++)); do
    if [[ ${vals[i]} != 1 ]]; then
        echo "DS island item $((189+i)) FAILED: result=${vals[i]}, actions offered=${vals[16+i]}"
        fail=1
    fi
done
if [[ ${vals[32]} != 1 ]]; then echo "DS island smoke FAIL: test did not finish"; fail=1; fi
if ((fail)); then exit 1; fi
echo "DS ISLAND & STAIRS PASS: all 16 interactions responded"
