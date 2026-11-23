#!/usr/bin/env bash
# Verify that an unmodified historical DS ROM can write a save which the
# current ROM loads, preserving its traits and original world generation.
# Both ROMs must be built with -DDSTEST -DAUTO_SCENE=140.
# Usage: OLD=/path/to/old-build NEW=/path/to/current-build DSRUN=/tmp/dsrun \
#        LABEL=d78 bash ports/dsi/test/ds_oldsave.sh
set -euo pipefail

OLD_DIR=${OLD:-/tmp/dsold}
NEW_DIR=${NEW:-/tmp/dsnew}
RUN=${DSRUN:-/tmp/dsrun}
LABEL=${LABEL:-legacy}
SAVE=${SAVE:-"$NEW_DIR/oldsave-$LABEL.sav"}
LOG=${LOG:-"$NEW_DIR/oldsave-$LABEL"}
ROUTE='100-103:0:212,140;200-203:1:-1,-1'

command -v llvm-nm >/dev/null || { echo "OLDSAVE FAIL: missing llvm-nm" >&2; exit 2; }
test -x "$RUN" || { echo "OLDSAVE FAIL: missing emulator $RUN" >&2; exit 2; }
test "$OLD_DIR" != "$NEW_DIR" || { echo "OLDSAVE FAIL: old and new builds are the same" >&2; exit 2; }
for dir in "$OLD_DIR" "$NEW_DIR"; do
    test -s "$dir/bore.elf" && test -s "$dir/bore.nds" || {
        echo "OLDSAVE FAIL: missing ROM or ELF under $dir" >&2; exit 2;
    }
done
mkdir -p "$(dirname "$SAVE")" "$(dirname "$LOG")"
rm -f "$SAVE" "$LOG-old.log" "$LOG-new.log"

# Read all of llvm-nm to avoid SIGPIPE under pipefail, and require exactly
# one definition. A missing symbol must fail, never silently read address 0.
sym() {
    llvm-nm --defined-only "$1/bore.elf" |
        awk -v target="$2" '$3 == target { count++; address = $1 }
             END { if (count != 1) exit 1; print address }'
}
addresses() {
    local dir=$1 status traits gen
    status=$(sym "$dir" test_sres) || { echo "Missing test_sres in $dir" >&2; return 1; }
    traits=$(sym "$dir" trg_sig) || { echo "Missing trg_sig in $dir" >&2; return 1; }
    gen=$(sym "$dir" bm_gen) || { echo "Missing bm_gen in $dir" >&2; return 1; }
    printf '%s,%s,%s\n' "$status" "$traits" "$gen"
}
run_rom() {
    local dir=$1 keys=$2 log=$3 mem=$4 output
    if ! DBGMEM="$mem" BORE_SAV="$SAVE" "$RUN" "$dir/bore.nds" 700 \
        "$NEW_DIR/oldsave-$LABEL.ppm" "$ROUTE;$keys" >"$log" 2>&1; then
        echo "OLDSAVE FAIL: emulator failed ($dir)" >&2
        tail -40 "$log" >&2
        return 1
    fi
    output=$(awk '/^f[0-9]+:/ { last=$0 } END { if (last == "") exit 1; print last }' "$log") || {
        echo "OLDSAVE FAIL: no frame telemetry in $log" >&2
        tail -40 "$log" >&2
        return 1
    }
    printf '%s\n' "$output"
}

old_mem=$(addresses "$OLD_DIR")
new_mem=$(addresses "$NEW_DIR")
old_frame=$(run_rom "$OLD_DIR" '385-388:c00:-1,-1' "$LOG-old.log" "$old_mem")
test -s "$SAVE" || { echo "OLDSAVE FAIL: historical ROM did not write a save" >&2; exit 1; }
new_frame=$(run_rom "$NEW_DIR" '385-388:200:-1,-1' "$LOG-new.log" "$new_mem")

# Frame telemetry: fNNN: test_sres trg_sig bm_gen. 301 is successful
# historical write; 401 is successful load by the new ROM.
read -r old_status old_traits old_gen <<< "${old_frame#*:}"
read -r new_status new_traits new_gen <<< "${new_frame#*:}"
echo "$LABEL old writes: $old_frame"
echo "$LABEL new reads:  $new_frame"
for value in "$old_status" "$old_traits" "$old_gen" "$new_status" "$new_traits" "$new_gen"; do
    [[ "$value" =~ ^-?[0-9]+$ ]] || { echo "OLDSAVE FAIL: malformed telemetry" >&2; exit 1; }
done
if [[ "$old_status" == 301 && "$new_status" == 401 &&
      "$old_traits" == "$new_traits" && "$old_gen" == "$new_gen" ]]; then
    echo "OLDSAVE PASS ($LABEL): preserved traits checksum and world generation $new_gen"
else
    echo "OLDSAVE FAIL ($LABEL): write/load status or saved data changed" >&2
    exit 1
fi
