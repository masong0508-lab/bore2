#!/usr/bin/env bash
# Run braking and rail-direction regressions through the real ARM9 controller.
set -euo pipefail
R=$(cd "$(dirname "$0")/../../.." && pwd)
D=${1:-$R/out-skate-controls}
RUN=${DSRUN:-/tmp/dsrun}
mkdir -p "$D"
D=$(cd "$D" && pwd)
rm -f "$D/controls_results.json" "$D/controls.ppm"
test -x "$RUN" || { echo "Missing melonDS runner: $RUN" >&2; exit 2; }
if ! OUT="$D" XTRA="-DDSTEST -DAUTOSC=41 -DDS_SKATE_CONTROLS_TEST" bash "$R/tools/dsbuild/mkp.sh" >"$D/build.log" 2>&1; then
    tail -80 "$D/build.log" >&2
    exit 2
fi
sym() { llvm-nm "$D/bore.elf" | awk -v n="$1" '$3 == n && !found {print $1; found=1}'; }
mem() {
    local addr base i
    addr=$(sym "$1")
    test -n "$addr" || { echo "Missing ROM test symbol $1" >&2; return 1; }
    base=$((16#$addr))
    local -a addrs=()
    for ((i=0; i<$2; ++i)); do addrs+=("$(printf '%x' $((base + 4*i)))"); done
    local IFS=,
    echo "${addrs[*]}"
}
M="$(mem sk_controls_result 12),$(mem sk_controls_speed 8),$(mem sk_controls_done 1)"
LIVE='100-103:0:212,140;200-203:1:-1,-1'
DBGMEM="$M" "$RUN" "$D/bore.nds" 1000 "$D/controls.ppm" "$LIVE" >"$D/controls.log" 2>&1
python3 - "$D" <<'PY'
from pathlib import Path
import json
import re
import sys
d = Path(sys.argv[1])
rows = []
for line in (d / "controls.log").read_text(errors="replace").splitlines():
    m = re.fullmatch(r"f(\d+):\s+((?:-?\d+\s*)+)", line)
    if m:
        values = [int(v) for v in m[2].split()]
        if len(values) == 21:
            rows.append((int(m[1]), values))
if not rows or rows[-1][1][20] != 1:
    raise SystemExit("FAIL: DS skate controller suite did not finish")
frame, values = rows[-1]
labels = [f"{case}_{yaw}" for case in ("brake", "rail_and_ollie", "push_and_steer")
          for yaw in (0, 1024, 2048, 3072)]
results = dict(zip(labels, values[:12]))
report = {"passed": all(v == 1 for v in results.values()), "frame": frame,
          "results": results, "coast_speed": values[12:16], "brake_speed": values[16:20]}
(d / "controls_results.json").write_text(json.dumps(report, indent=2) + "\n")
print("DS SKATE CONTROLLER:", report)
if not report["passed"]:
    raise SystemExit("FAIL: " + ", ".join(k for k, v in results.items() if v != 1))
PY
