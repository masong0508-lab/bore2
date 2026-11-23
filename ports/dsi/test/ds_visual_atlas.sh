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
    rm -f "$D/$mode.ppm" "$D/${mode}_"*.ppm
done
rm -f "$D/visual_atlas_results.json"
test -x "$RUN" || { echo "Missing melonDS runner: $RUN" >&2; exit 2; }

# A single seed and fixed controls are repeated for each roof mode. The
# DS_TOUR hook moves through six representative planned lots, including
# varied house types, orientations, rooms, yards and street views, in both builds.
# DS_TOUR_MID places the controlled Sim inside the house, so Cutaway and
# Always should produce *different actual 3D frames*, not just UI text.
LIVE='100-103:0:212,140;200-203:1:-1,-1'
SHOTS='345,385,425,465,505,545'
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
    if ! "$RUN" "$B/bore.nds" 650 "$D/$mode.ppm" "$LIVE" "$SHOTS" >"$D/emulate-$mode.log" 2>&1; then
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
import sys
root = pathlib.Path(sys.argv[1])
frames = (345, 385, 425, 465, 505, 545)
header = b"P6\n256 384\n255\n"
width, height = 256, 384
screen_bytes = width * (height // 2) * 3
results = []
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
if sum(r["changed_channels"] >= 300 for r in results) < 3:
    raise SystemExit("FAIL: fewer than three planned-lot views show roof differences; check tour frame timing")
summary = {"method": "melonDS native 256x384 PPM", "roof_modes": ["cutaway", "always"],
           "world_region": "DS_TOUR first six planned lots",
           "frames": results, "passed": True}
(root / "visual_atlas_results.json").write_text(json.dumps(summary, indent=2) + "\n")
print("DS VISUAL ATLAS PASS: screenshots saved in " + str(root))
PY
