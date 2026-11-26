#!/usr/bin/env bash
# Real PS1 engine at default timing, seeded AUTO_CITY walking replay.
# Unlike the house A/B orbit, no AB_LOCK4 or AB_NOSHED is used.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/out-ps1-stream-motion"}
RUNNER=${PS1_REGTEST:-/home/claude/tools/ds/b/bin/duckstation-regtest}
SDK=${PS1_SDK:-/opt/psn00bsdk}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
test -x "$RUNNER"
test -f "$SDK/lib/libpsn00b/cmake/sdk.cmake"
export PATH="$SDK/bin:$PATH"
rm -f "$OUT/stream_motion_results.json" "$OUT/ram.bin" "$OUT/symbols.txt"
mkdir -p "$OUT/shots"
find "$OUT/shots" -name 'frame_*.png' -type f -delete
BD="$OUT/build"
cmake -S "$ROOT" -B "$BD" -G Ninja -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_TOOLCHAIN_FILE="$SDK/lib/libpsn00b/cmake/sdk.cmake" \
 -DPSN00BSDK_TC="$SDK" -DCMAKE_C_FLAGS="-DAUTO_CITY=1" >"$OUT/config.log" 2>&1 || { tail -75 "$OUT/config.log";exit 2; }
cmake --build "$BD" --target iso -j 4 >"$OUT/build.log" 2>&1 || { tail -90 "$OUT/build.log";exit 2; }
test -s "$BD/bore2.cue" && test -s "$BD/bore2.bin" && test -s "$BD/bore2.elf"
mipsel-none-elf-nm --defined-only --print-size "$BD/bore2.elf" >"$OUT/symbols.txt"
PYTHONPATH="$ROOT/test" python3 "$ROOT/test/ps1_stream_motion.py" route "$OUT"
(
 cd "$BD"
 ulimit -f 65536
 env -u DS8MB DSCROP=None DSRAW=1 DSSCRIPT="$OUT/route.txt" DSRAM="$OUT/ram.bin" \
   timeout 780 "$RUNNER" -log info -frames 7200 -dumpdir "$OUT/shots" -dumpinterval 100000 "$BD/bore2.cue" >"$OUT/emulate.log" 2>&1
) || { echo "Stock-timing streaming replay failed";tail -85 "$OUT/emulate.log";exit 2; }
PYTHONPATH="$ROOT/test" python3 "$ROOT/test/ps1_stream_motion.py" audit "$OUT"
