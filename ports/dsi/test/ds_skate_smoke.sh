#!/usr/bin/env bash
# Real melonDS 3D skating playthrough: a generated skate plaza with moving
# rider, curved ramps, turn, jump, FPS and DS GPU polygon-budget sampling.
# Run: DSRUN=/tmp/dsrun bash ports/dsi/test/ds_skate_smoke.sh [outdir]
set -euo pipefail
R=$(cd "$(dirname "$0")/../../.." && pwd)
D=${1:-$R/out-skate}
RUN=${DSRUN:-/tmp/dsrun}
mkdir -p "$D"
D=$(cd "$D" && pwd)
# Keep failed reruns from publishing a previous run's screenshots or PASS.
rm -f "$D/skate.ppm" "$D/skate_520.ppm" "$D/skate_720.ppm" "$D/skate_900.ppm" "$D/skate_results.json"
test -x "$RUN" || { echo "Missing melonDS runner: $RUN" >&2; exit 2; }
if ! OUT="$D" XTRA="-DDSTEST -DAUTOSC=41" bash "$R/tools/dsbuild/mkp.sh" >"$D/build.log" 2>&1; then
    tail -80 "$D/build.log" >&2
    exit 2
fi
sym() { llvm-nm "$D/bore.elf" | grep " $1\$" | cut -d' ' -f1 | head -1; }
for name in test_scene fps_now polys_last on_board sk_x3 sk_z3; do
    s=$(sym "$name"); test -n "$s" || { echo "Missing DS smoke symbol: $name" >&2; exit 2; }
done
M="$(sym fps_now),$(sym polys_last),$(sym on_board),$(sym sk_x3),$(sym sk_z3)"
# AUTOSC=41 activates the actual skate plaza on the first Live frame.
# Hold screen-up to accelerate,
# steer, ollie over nearby obstacles and push off again.
LIVE='100-103:0:212,140;200-203:1:-1,-1'
ROUTE="$LIVE;475-650:40:-1,-1;651-710:50:-1,-1;711-730:41:-1,-1;731-875:40:-1,-1"
if ! DBGMEM="$M" "$RUN" "$D/bore.nds" 940 "$D/skate.ppm" "$ROUTE" "520,720,900" >"$D/skate.log" 2>&1; then
    echo "Skate emulator failed:" >&2
    tail -70 "$D/skate.log" >&2
    exit 2
fi
python3 - "$D" <<'PY'
from pathlib import Path
import json
import re
import sys
d = Path(sys.argv[1])
rows = []
geometry = []
for line in (d / "skate.log").read_text(errors="replace").splitlines():
    m = re.match(r"^f(\d+):\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)\s*$", line)
    if m:
        row = tuple(int(v) for v in m.groups())
        if row[3] == 1:
            geometry.append(row[2])  # include cache warm-up, before the rider moves
        if row[0] >= 450:
            rows.append(row)
if len(rows) < 10:
    raise SystemExit("FAIL: no sustained gameplay samples from actual emulator")
active = [r for r in rows if r[3] == 1]
print(f"Skate route diagnostics: {len(rows)} samples, {len(active)} board-active, "
      f"first {rows[:5]}, last {rows[-5:]}")
if len(active) < 9:
    raise SystemExit("FAIL: skate scene did not activate. Check AUTOSC and first Live frame")
fps = [r[1] for r in active]
triangles = [r[2] for r in active]
x = [r[4] for r in active]
z = [r[5] for r in active]
if max(x) - min(x) + max(z) - min(z) < 80:
    raise SystemExit("FAIL: skate rider did not move through the course")
if sum(fps) / len(fps) < 20:
    raise SystemExit("FAIL: skate scene averaged less than 20 rendered FPS")
if max(geometry) >= 4096:
    raise SystemExit("FAIL: skate scene exceeded the smoke test's 4096 counted-triangle limit")
header = b"P6\n256 384\n255\n"
for frame in (520, 720, 900):
    p = d / f"skate_{frame}.ppm"
    raw = p.read_bytes()
    if not raw.startswith(header) or len(raw) != len(header) + 256 * 384 * 3:
        raise SystemExit("FAIL: missing/invalid native skate screenshot: " + str(p))
    if len(set(raw[len(header) + 256 * 192 * 3:])) < 20:
        raise SystemExit("FAIL: blank skate scene: " + str(p))
report = {"passed": True, "frame_samples": len(active),
          "fps_min": min(fps), "fps_mean": round(sum(fps) / len(fps), 2),
          "fps_max": max(fps), "triangles_max": max(geometry),
          "geometry_frame_samples": len(geometry), "steady_triangles_max": max(triangles),
          "skate_position_range": [max(x) - min(x), max(z) - min(z)],
          "screenshots": [520, 720, 900]}
(d / "skate_results.json").write_text(json.dumps(report, indent=2) + "\n")
print("DS SKATE PLAYTHROUGH PASS:", report)
PY
