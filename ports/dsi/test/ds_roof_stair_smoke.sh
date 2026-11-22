#!/usr/bin/env bash
# Real DS renderer regression: the Man Base's upstairs stairwell must have
# one continuous bedroom roof, while its floor remains open for the stairs.
# Replays the current and exact historical floor-only support rules at
# three gameplay camera angles. Legacy behavior exists only in this test ROM.
# Usage: DSRUN=/tmp/dsrun bash ports/dsi/test/ds_roof_stair_smoke.sh [outdir]
set -euo pipefail
R=$(cd "$(dirname "$0")/../../.." && pwd)
D=${1:-$R/out-roof-stairs}
RUN=${DSRUN:-/tmp/dsrun}
mkdir -p "$D"
D=$(cd "$D" && pwd)
test -x "$RUN" || { echo "Missing melonDS runner: $RUN" >&2; exit 2; }
rm -f "$D"/current*.ppm "$D"/legacy*.ppm "$D"/roof_stair_results.json
LIVE='100-103:0:212,140;200-203:1:-1,-1'
ROUTE="$LIVE;400-403:100:-1,-1;520-523:100:-1,-1"
# Capture settled views on the runner's 30-frame telemetry cadence, so
# each image has an exact matching camera-state sample. Mid-turn camera
# smoothing may take a different number of game-loop passes in A and B.
SHOTS='390,510,660'
for variant in current legacy; do
    B="$D/rom-$variant"
    flags='-DDSTEST -DDS_ROOF_STAIRS_TEST'
    if [ "$variant" = legacy ]; then flags="$flags -DDS_ROOF_STAIRS_LEGACY"; fi
    if ! OUT="$B" XTRA="$flags" bash "$R/tools/dsbuild/mkp.sh" >"$D/build-$variant.log" 2>&1; then
        tail -80 "$D/build-$variant.log" >&2; exit 2
    fi
    sym() { llvm-nm "$B/bore.elf" | awk -v n="$1" '$3 == n && !found {print $1; found=1}'; }
    addresses=()
    for name in roof_stair_height roof_stair_floor roof_stair_sections roof_stair_triangles polys_last fps_now lot_bx lot_by live_yaw camx camz live_pitch live_dist; do
        address=$(sym "$name")
        test -n "$address" || { echo "Missing roof replay symbol: $name" >&2; exit 2; }
        addresses+=("$address")
    done
    M=$(IFS=,; echo "${addresses[*]}")
    rm -f "$B/bore.sav"
    if ! DBGMEM="$M" "$RUN" "$B/bore.nds" 700 "$D/$variant.ppm" "$ROUTE" "$SHOTS" >"$D/emulate-$variant.log" 2>&1; then
        tail -80 "$D/emulate-$variant.log" >&2; exit 2
    fi
done
python3 - "$D" "$SHOTS" <<'PY'
from pathlib import Path
import json
import re
import sys

root = Path(sys.argv[1])
capture_frames = tuple(map(int, sys.argv[2].split(',')))
results = {}
camera_replay = {}
for variant, expected_height in (("current", 1), ("legacy", 0)):
    rows = []
    for line in (root / f"emulate-{variant}.log").read_text().splitlines():
        match = re.fullmatch(r"f(\d+):\s+((?:-?\d+\s*)+)", line)
        if match and int(match[1]) >= 360:
            values = tuple(map(int, match[2].split()))
            if len(values) != 13:
                raise SystemExit(f"FAIL: incomplete {variant} renderer counters")
            rows.append((int(match[1]), *values))
    if len(rows) < 8:
        raise SystemExit(f"FAIL: {variant} replay did not reach sustained gameplay")
    for frame, height, floor, sections, roof_tris, tris, fps, bx, by, yaw, cx, cz, pitch, distance in rows:
        if (height, floor, bx, by) != (expected_height, 0, 2009, 2001):
            raise SystemExit(f"FAIL: {variant} frame {frame} did not reproduce the real Man Base stairwell: {rows}")
        if not (0 < sections and 0 < roof_tris < tris < 4096):
            raise SystemExit(f"FAIL: {variant} frame {frame} invalid or excessive geometry: {rows}")
    final = rows[-1]
    if len({r[9] for r in rows}) < 3:
        raise SystemExit(f"FAIL: {variant} camera did not rotate through three views")
    poses = {r[0]: r[7:] for r in rows}
    if any(frame not in poses for frame in capture_frames):
        raise SystemExit(f"FAIL: {variant} lacks exact screenshot camera telemetry")
    if len({poses[frame][2] for frame in capture_frames}) != 3:
        raise SystemExit(f"FAIL: {variant} screenshots do not cover three distinct camera angles")
    camera_replay[variant] = [(frame, *poses[frame]) for frame in capture_frames]
    results[variant] = {
        "stairwell_floor": final[2], "stairwell_roof_storey": final[1],
        "roof_sections": final[3], "roof_triangles": final[4],
        "frame_triangles_peak": max(r[5] for r in rows),
        "fps_min": min(r[6] for r in rows),
        "fps_mean": round(sum(r[6] for r in rows) / len(rows), 2),
        "sample_count": len(rows), "lot": [final[7], final[8]],
    }
    if results[variant]["fps_mean"] < 25:
        raise SystemExit(f"FAIL: {variant} roof replay averaged below 25 rendered FPS")

current, legacy = results["current"], results["legacy"]
if camera_replay["current"] != camera_replay["legacy"]:
    raise SystemExit("FAIL: A/B screenshots did not sample matching lot and camera positions")
if current["roof_sections"] >= legacy["roof_sections"] or current["roof_triangles"] >= legacy["roof_triangles"]:
    raise SystemExit("FAIL: enclosing the stairwell did not remove the fragmented roof geometry")

header = b"P6\n256 384\n255\n"
screen_bytes = 256 * 192 * 3
frames = []
for frame in capture_frames:
    images = {}
    for variant in ("current", "legacy"):
        path = root / f"{variant}_{frame}.ppm"
        raw = path.read_bytes()
        if not raw.startswith(header) or len(raw) != len(header) + 2 * screen_bytes:
            raise SystemExit(f"FAIL: invalid native DS screenshot {path}")
        play = raw[len(header) + screen_bytes:]
        if len(set(play)) < 20:
            raise SystemExit(f"FAIL: blank gameplay view {path}")
        images[variant] = play
    changed = sum(a != b for a, b in zip(images["current"], images["legacy"]))
    frames.append({"frame": frame, "changed_gameplay_channels": changed})
if sum(r["changed_gameplay_channels"] >= 100 for r in frames) < 2:
    raise SystemExit("FAIL: fewer than two gameplay angles show the repaired roof")
report = {"passed": True, "method": "melonDS native 256x384 gameplay replay",
          "scene": "Man Base upstairs bedroom and its open stairwell",
          "capture_cameras_match": True, "variants": results, "frames": frames}
(root / "roof_stair_results.json").write_text(json.dumps(report, indent=2) + "\n")
print("DS STAIRWELL ROOF PASS:", json.dumps(report))
PY
