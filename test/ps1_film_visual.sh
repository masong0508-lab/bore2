#!/usr/bin/env bash
# Capture a real PS1 story-film sequence at native 320x240 resolution.
# AUTO_CUT opens the actual CUT.OVL film with its actors, subtitle clock and transitions.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/out-ps1-film"}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
RUNNER=${PS1_REGTEST:-/home/claude/tools/ds/b/bin/duckstation-regtest}
SDK=${PS1_SDK:-/opt/psn00bsdk}
test -x "$RUNNER" || { echo "Missing DuckStation runner: $RUNNER" >&2; exit 2; }
test -f "$SDK/lib/libpsn00b/cmake/sdk.cmake" || { echo "Missing PS1 SDK" >&2; exit 2; }
export PATH="$SDK/bin:$PATH"
BD="$OUT/build"
mkdir -p "$OUT/shots"
find "$OUT/shots" -name 'frame_*.png' -delete
rm -f "$OUT/film_results.json" "$OUT/ram.bin" "$OUT/symbols.txt"
cmake -S "$ROOT" -B "$BD" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$SDK/lib/libpsn00b/cmake/sdk.cmake" \
  -DPSN00BSDK_TC="$SDK" -DCMAKE_C_FLAGS=-DAUTO_CUT=1 >"$OUT/config.log" 2>&1 || {
  tail -80 "$OUT/config.log" >&2; exit 2;
}
cmake --build "$BD" --target iso -j 4 >"$OUT/build.log" 2>&1 || {
  tail -100 "$OUT/build.log" >&2; exit 2;
}
test -s "$BD/bore2.cue" && test -s "$BD/bore2.bin" && test -s "$BD/bore2.elf"
mipsel-none-elf-nm --defined-only --print-size "$BD/bore2.elf" >"$OUT/symbols.txt"
cat >"$OUT/script.txt" <<'ROUTE'
1800 shot
2100 shot
2400 shot
2700 shot
3000 shot
ROUTE
(
  cd "$BD"
  env -u DS8MB DSCROP=None DSRAW=1 DSSCRIPT="$OUT/script.txt" \
    DSRAM="$OUT/ram.bin" timeout 400 "$RUNNER" -log info -frames 3200 \
    -dumpdir "$OUT/shots" -dumpinterval 100000 "$BD/bore2.cue" >"$OUT/emulate.log" 2>&1
) || { tail -80 "$OUT/emulate.log" >&2; exit 2; }
python3 - "$OUT" <<'PY'
from pathlib import Path
from PIL import Image
import json
import struct
import sys

root = Path(sys.argv[1])
shots = sorted((root / "shots").rglob("frame_*.png"))
if len(shots) != 5:
    raise SystemExit(f"FAIL: expected five story-film frames, got {len(shots)}")
frames = []
detailed = 0
for shot in shots:
    img = Image.open(shot).convert("RGB")
    colors = len(set(img.getdata()))
    if img.size != (320, 240):
        raise SystemExit(f"FAIL: non-native film screenshot {shot}: {img.size}")
    # The subdued film palette deliberately reuses colors across frames.
    # Require actual scene variation, not the 150-color threshold from
    # the more colorful title scene.
    detailed += colors >= 80
    frames.append(dict(image=str(shot.relative_to(root)), width=img.width,
                       height=img.height, distinct_colors=colors))

if detailed < 3:
    raise SystemExit(f"FAIL: only {detailed}/5 film frames contain recognizable rendered scenes")

symbols = {}
for line in (root / "symbols.txt").read_text().splitlines():
    parts = line.split()
    if len(parts) == 4 and parts[2] in "bBdDrR":
        symbols.setdefault(parts[3], []).append((int(parts[0], 16) & 0x1fffffff, int(parts[1], 16)))
ram = (root / "ram.bin").read_bytes()
if len(ram) != 2 * 1024 * 1024:
    raise SystemExit("FAIL: missing actual PS1 2 MB RAM capture")

def val(name):
    found = symbols.get(name, [])
    if len(found) != 1 or found[0][1] < 4:
        raise SystemExit(f"FAIL: missing unique 4-byte title-state symbol {name}")
    return struct.unpack_from("<i", ram, found[0][0])[0]

state = val("state")
if state != 40:
    raise SystemExit(f"FAIL: captures may not be from a running CUT.OVL story film (state={state})")
report = dict(result="PASS", renderer="DuckStation/OpenBIOS", resolution="320x240",
              state=state, detailed_screenshots=detailed, screenshots=frames,
              note="Actual story-film state verified; inspect images for cutscene composition, subtitle readability and restrained transitions. Does not establish FPS gain or physical hardware compatibility.")
(root / "film_results.json").write_text(json.dumps(report, indent=2) + "\n")
print("PS1 FILM CAPTURE PASS: inspect five screenshots for readable subtitles and restrained transitions")
PY
