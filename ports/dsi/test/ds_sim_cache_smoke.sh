#!/usr/bin/env bash
# Verify the actual cached Sim geometry cost from three native RAM snapshots.
# No special game-side counter/test hook: decode the FIFO model packets that
# the real skate replay records and submits.
set -euo pipefail
R=$(cd "$(dirname "$0")/../../.." && pwd)
D=${1:-$R/out-sim-cache}
RUN=${DSRUN:-/tmp/dsrun}
mkdir -p "$D"
D=$(cd "$D" && pwd)
rm -f "$D"/ram_450.bin "$D"/ram_660.bin "$D"/ram_900.bin "$D"/sim_cache_results.json "$D"/cache.ppm
test -x "$RUN" || { echo "Missing melonDS runner: $RUN" >&2; exit 2; }
# The archived toolchain runner predates RAMDUMP. Relink the current
# repository runner against its saved core (GCC 13 LTO on this SDK).
if ! LC_ALL=C grep -aq RAMDUMP "$RUN"; then
    if ! "${CXX:-g++}" -std=c++17 -O2 -DARCHITECTURE_x86_64=1 -DGDBSTUB_ENABLED \
        -I/tmp/melonDS/src -I/tmp/melonDS/src/teakra/include \
        "$R/tools/dsemu/run.cpp" /tmp/mb/src/libcore.a /tmp/mb/src/teakra/src/libteakra.a \
        -lpthread -o "$D/dsrun" >"$D/build-runner.log" 2>&1; then
        tail -80 "$D/build-runner.log" >&2; exit 2
    fi
    RUN="$D/dsrun"
fi
if ! OUT="$D" XTRA='-DDSTEST -DAUTOSC=41' bash "$R/tools/dsbuild/mkp.sh" >"$D/build.log" 2>&1; then
    tail -80 "$D/build.log" >&2; exit 2
fi
LIVE='100-103:0:212,140;200-203:1:-1,-1'
ROUTE="$LIVE;475-650:40:-1,-1;651-710:50:-1,-1;711-730:41:-1,-1;731-875:40:-1,-1"
if ! RAMDUMP='450,660,900' RAMPFX="$D/ram" "$RUN" "$D/bore.nds" 940 "$D/cache.ppm" "$ROUTE" >"$D/emulate.log" 2>&1; then
    tail -80 "$D/emulate.log" >&2; exit 2
fi
python3 "$R/ports/dsi/test/ds_sim_cache_check.py" --elf "$D/bore.elf" \
    --json-out "$D/sim_cache_results.json" "$D/ram_450.bin" "$D/ram_660.bin" "$D/ram_900.bin"
