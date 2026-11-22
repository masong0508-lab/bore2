#!/usr/bin/env bash
# Native PS1 moving-house screenshot smoke test. Not hardware verification.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/out-ps1-house-motion"}
RUNNER=${PS1_REGTEST:-/home/claude/tools/ds/b/bin/duckstation-regtest}
SDK=${PS1_SDK:-/opt/psn00bsdk}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
test -x "$RUNNER" || { echo "Missing DuckStation runner: $RUNNER" >&2; exit 2; }
test -f "$SDK/lib/libpsn00b/cmake/sdk.cmake" || { echo "Missing PS1 SDK: $SDK" >&2; exit 2; }
export PATH="$SDK/bin:$PATH"
rm -f "$OUT/house_motion_results.json" "$OUT/ram.bin" "$OUT/symbols.txt"
find "$OUT/shots" -name 'frame_*.png' -type f -delete 2>/dev/null || true
BD="$OUT/build"
FLAGS="-DAUTO_CITY=1 -DAUTO_LOTS=0 -DAUTO_LOTS_K=6 -DAUTO_HOUSE_SWEEP=1 -DAB_LOCK4=1 -DAB_NOSHED=1"
cmake -S "$ROOT" -B "$BD" -G Ninja -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_TOOLCHAIN_FILE="$SDK/lib/libpsn00b/cmake/sdk.cmake" \
 -DPSN00BSDK_TC="$SDK" "-DCMAKE_C_FLAGS=$FLAGS" >"$OUT/config.log" 2>&1 || {
 tail -75 "$OUT/config.log" >&2; exit 2;
}
cmake --build "$BD" --target iso -j 4 >"$OUT/build.log" 2>&1 || {
 tail -90 "$OUT/build.log" >&2; exit 2;
}
test -s "$BD/bore2.cue" && test -s "$BD/bore2.bin" && test -s "$BD/bore2.elf"
mipsel-none-elf-nm --defined-only --print-size "$BD/bore2.elf" >"$OUT/symbols.txt"
mkdir -p "$OUT/shots"
cat >"$OUT/route.txt" <<'ROUTE'
2600 shot
2800 shot
3000 shot
3200 shot
3400 shot
ROUTE
(
 cd "$BD"
 env -u DS8MB DSCROP=None DSRAW=1 DSSCRIPT="$OUT/route.txt" \
 DSRAM="$OUT/ram.bin" timeout 520 "$RUNNER" -log info -frames 4100 \
 -dumpdir "$OUT/shots" -dumpinterval 100000 "$BD/bore2.cue" >"$OUT/emulate.log" 2>&1
) || { echo "Moving-house DuckStation playback failed" >&2; tail -85 "$OUT/emulate.log" >&2; exit 2; }
python3 - "$OUT" <<'PY'
from pathlib import Path
from PIL import Image
import hashlib
import json
import struct
import sys
root = Path(sys.argv[1])
symbols = {}
for line in (root / "symbols.txt").read_text().splitlines():
    cols = line.split()
    if len(cols) == 4 and cols[2] in "bBdDrR":
        symbols.setdefault(cols[3], []).append((int(cols[0], 16) & 0x1fffffff, int(cols[1], 16)))
ram = (root / "ram.bin").read_bytes()
if len(ram) != 2 * 1024 * 1024:
    raise SystemExit("FAIL: expected 2 MiB of PS1 RAM")
def value(name):
    matches = symbols.get(name, [])
    if len(matches) != 1:
        raise SystemExit(f"FAIL: missing ELF data symbol {name}")
    address, size = matches[0]
    if size < 4 or address + 4 > len(ram):
        raise SystemExit(f"FAIL: invalid RAM extent for {name}")
    return struct.unpack("<i", ram[address:address+4])[0]
state = {name: value(name) for name in (
    "state", "world_zone", "wsalt", "bm_gen", "world_wbx", "world_wby",
    "cam_yaw", "cam_dist", "cam_pitch", "dbg_life_n")}
if (state["state"], state["world_zone"], state["wsalt"]) != (3, 3, 0x5EED1234):
    raise SystemExit(f"FAIL: wrong seeded live-world state: {state}")
if state["world_wbx"] < 0 or state["world_wby"] < 0 or state["dbg_life_n"] < 175:
    raise SystemExit(f"FAIL: did not reach house scene: {state}")
if state["cam_yaw"] == 2600 or state["cam_dist"] <= 1300:
    raise SystemExit(f"FAIL: test camera did not move: {state}")
shots = sorted((root / "shots").rglob("frame_*.png"))
if len(shots) != 5:
    raise SystemExit(f"FAIL: expected five real screenshots, got {len(shots)}")
results = []
prev = None
for i, path in enumerate(shots):
    img = Image.open(path).convert("RGB")
    if img.size != (320, 240):
        raise SystemExit(f"FAIL: incorrect native screenshot {img.size}")
    scene = img.crop((16, 12, 304, 180))
    if len(set(scene.getdata())) < 500:
        raise SystemExit(f"FAIL: blank/low-detail 3D capture: {path}")
    pixels = scene.tobytes()
    changed = 0 if prev is None else sum(a != b for a, b in zip(pixels, prev))
    if prev is not None and changed < 3000:
        raise SystemExit(f"FAIL: camera sweep not visible at capture {i}: {changed} changed RGB channels")
    prev = pixels
    dest = root / f"house_{i}.png"
    img.save(dest)
    results.append(dict(index=i, file=dest.name,
                        scene_sha256=hashlib.sha256(pixels).hexdigest(),
                        changed_scene_rgb_channels=changed))
(root / "house_motion_results.json").write_text(json.dumps({
    "result":"PASS", "engine":"DuckStation/OpenBIOS", "resolution":"320x240",
    "world":"generated version-8 two-storey home, seen from yard; seed 0x5EED1234",
    "camera":"test-only orbit and continuous dolly", "captures":results,
    "final_ram":state,
    "limits":"Five distinct 3D images; not complete flicker/performance/hardware proof."
}, indent=2) + "\n")
print("PS1 MOVING HOUSE PASS: 5 native screenshots, active camera orbit and 2 MiB RAM")
PY
