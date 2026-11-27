#!/usr/bin/env bash
# OW-003 / QA-009: reproducible native-resolution DS screenshot comparison.
# Usage: DSRUN=/tmp/dsrun bash ports/dsi/test/ds_visual_atlas.sh [outdir]
# The supplied ROMs are test-only; this does not modify playable ROM assets,
# game saves, procedural world versions, or the retail renderer.
set -euo pipefail
R=$(cd "$(dirname "$0")/../../.." && pwd)
D=${1:-$R/out-visual}
RUN=${DSRUN:-/tmp/dsrun}
mkdir -p "$D"
D=$(cd "$D" && pwd)
# Clear only this replay's evidence, including after an interrupted previous
# run. A build or emulator failure must never leave an old passing report.
for mode in cutaway always; do
    rm -f "$D/$mode.ppm" "$D/${mode}_345.ppm" "$D/${mode}_385.ppm" "$D/${mode}_425.ppm"
done
rm -f "$D/visual_atlas_results.json"
test -x "$RUN" || { echo "Missing melonDS runner: $RUN" >&2; exit 2; }

# A single seed and fixed controls are repeated for each roof mode. The
# DS_TOUR hook moves through the same first three representative generated
# houses (including their rooms/yard and street views) in both builds.
# DS_TOUR_MID places the controlled Sim inside the house, so Cutaway and
# Always should produce *different actual 3D frames*, not just UI text.
LIVE='100-103:0:212,140;200-203:1:-1,-1'
SHOTS='345,385,425'
for mode in cutaway always; do
    B="$D/rom-$mode"
    mkdir -p "$B"
    flags="-DDSTEST -DDS_TOUR -DDS_TOUR_ONE=0 -DDS_TOUR_MID=0"
    if [ "$mode" = always ]; then flags="$flags -DDS_TOUR_ROOF"; fi
    if ! OUT="$B" XTRA="$flags" bash "$R/tools/dsbuild/mkp.sh" >"$D/build-$mode.log" 2>&1; then
        echo "$mode ROM build failed:" >&2
        tail -90 "$D/build-$mode.log" >&2
        exit 2
    fi
    test -s "$B/bore.nds" && test -s "$B/bore.elf" || {
        echo "$mode ROM or ELF missing" >&2; exit 2;
    }
    # run.cpp's optional sixth argument accepts comma-separated frame
    # numbers and saves "<base>_<frame>.ppm". One replay per mode suffices.
    sym() { llvm-nm "$B/bore.elf" | awk -v n="$1" '$3 == n && !found {print $1; found=1}'; }
    M="$(sym ds_proxy_shells),$(sym ds_proxy_triangles),$(sym polys_last)"
    test "${M//,/}" != "$M" && test -n "$(sym ds_proxy_shells)" && test -n "$(sym ds_proxy_triangles)" && test -n "$(sym polys_last)"
    if ! DBGMEM="$M" "$RUN" "$B/bore.nds" 480 "$D/$mode.ppm" "$LIVE" "$SHOTS" >"$D/emulate-$mode.log" 2>&1; then
        echo "$mode emulator failed:" >&2
        tail -90 "$D/emulate-$mode.log" >&2
        exit 2
    fi
done

# No PIL dependency: validate raw PPM frame dimensions and compare just the
# bottom 256x192 visual gameplay screen (the top 256x192 holds the HUD).
# Nonempty and nonidentical snapshots are a minimal signal that the roof
# toggle affected rendered world geometry in a repeatable real emulator run.
python3 - "$D" <<'PY'
import json
import pathlib
import re
import sys
root = pathlib.Path(sys.argv[1])
frames = (345, 385, 425)
header = b"P6\n256 384\n255\n"
width, height = 256, 384
screen_bytes = width * (height // 2) * 3
results = []
shell_evidence = {}
for mode in ("cutaway", "always"):
    samples = []
    for line in (root / f"emulate-{mode}.log").read_text().splitlines():
        m = re.fullmatch(r"f(\d+):\s+(\d+)\s+(\d+)\s+(\d+)\s*", line)
        if m and int(m[1]) >= 300:
            samples.append(tuple(map(int, m.groups())))
    if not samples:
        raise SystemExit(f"FAIL no native shell counters: {mode}")
    active = [r for r in samples if r[1] > 0]
    if not active or any(r[2] != r[1] * 10 for r in active):
        raise SystemExit(f"FAIL no completed solid far shells in {mode}: {samples}")
    shell_evidence[mode] = {"complete_shells": active[-1][1],
                            "shell_triangles": active[-1][2],
                            "frame_triangles_peak": max(r[3] for r in samples)}
    print("DS native shell evidence", mode, shell_evidence[mode])
for frame in frames:
    views = {}
    for mode in ("cutaway", "always"):
        path = root / f"{mode}_{frame}.ppm"
        raw = path.read_bytes()
        if not raw.startswith(header) or len(raw) != len(header) + width * height * 3:
            raise SystemExit(f"FAIL invalid 256x384 PPM: {path}")
        play = raw[len(header) + screen_bytes:]
        if len(set(play)) < 20:
            raise SystemExit(f"FAIL blank/low-detail gameplay image: {path}")
        views[mode] = play
    changed_channels = sum(a != b for a, b in zip(views["cutaway"], views["always"]))
    results.append({"frame": frame, "changed_channels": changed_channels})
    print(f"atlas frame {frame}: {changed_channels} gameplay RGB channels differ")
if sum(r["changed_channels"] >= 300 for r in results) < 2:
    raise SystemExit("FAIL: fewer than two generated-home views show roof differences; check tour frame timing")
summary = {"method": "melonDS native 256x384 PPM", "roof_modes": ["cutaway", "always"],
           "world_region": "DS_TOUR first three planned houses",
           "frames": results, "solid_neighbor_shells": shell_evidence, "passed": True}
(root / "visual_atlas_results.json").write_text(json.dumps(summary, indent=2) + "\n")
print("DS VISUAL ATLAS PASS: screenshots saved in " + str(root))
PY
