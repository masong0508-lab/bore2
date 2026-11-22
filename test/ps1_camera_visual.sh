#!/usr/bin/env bash
# Pixel evidence for the actual PlayStation camera, not a mathematical mockup.
# Needs PSn00bSDK, Ninja and the repo's headless DuckStation regtest/OpenBIOS.
# Run: bash test/ps1_camera_visual.sh [output-dir]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/out-ps1-camera"}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
RUNNER=${PS1_REGTEST:-/home/claude/tools/ds/b/bin/duckstation-regtest}
test -x "$RUNNER" || { echo "Missing DuckStation runner: $RUNNER" >&2; exit 2; }
test -f /opt/psn00bsdk/lib/libpsn00b/cmake/sdk.cmake || {
    echo "PSn00bSDK toolchain not restored" >&2; exit 2;
}
export PATH="/opt/psn00bsdk/bin:$PATH"
# These tests use a fixed generated multi-storey house, focus and pitch.
# AUTO_LOTS_K keeps the same lot and Sim in both builds; AB_LOCK4 prevents
# route drift when one optical projection costs fractionally more CPU.
# No real save or playable image is modified. 'legacy' is test-only.
for lens in legacy corrected; do
    BD="$OUT/build-$lens"
    EX="-DAUTO_CITY=1 -DAUTO_LOTS=0 -DAUTO_LOTS_K=6 -DAUTO_CAMTEST=2 -DAB_LOCK4=1"
    if [ "$lens" = legacy ]; then EX="$EX -DBORE_LENS_LEGACY=1"; fi
    cmake -S "$ROOT" -B "$BD" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/psn00bsdk/lib/libpsn00b/cmake/sdk.cmake \
        -DPSN00BSDK_TC=/opt/psn00bsdk "-DCMAKE_C_FLAGS=$EX" \
        >"$OUT/config-$lens.log" 2>&1 || {
            tail -75 "$OUT/config-$lens.log" >&2; exit 2;
        }
    cmake --build "$BD" --target iso -j 4 >"$OUT/build-$lens.log" 2>&1 || {
        tail -90 "$OUT/build-$lens.log" >&2; exit 2;
    }
    test -s "$BD/bore2.cue" && test -s "$BD/bore2.bin" || {
        echo "Missing PlayStation disc for $lens" >&2; exit 2;
    }
    mkdir -p "$OUT/shots-$lens"
    # Script times are counted from GAMESTART in the emulator log.
    # The game has entered the deterministic live home camera by then.
    cat >"$OUT/script-$lens.txt" <<'ROUTE'
1400 shot
1700 shot
2000 shot
ROUTE
    (
        cd "$BD"
        DSCROP=None DSRAW=1 DSSCRIPT="$OUT/script-$lens.txt" \
          timeout 520 "$RUNNER" -log info -frames 2700 \
          -dumpdir "$OUT/shots-$lens" -dumpinterval 100000 \
          "$BD/bore2.cue" >"$OUT/emulate-$lens.log" 2>&1
    ) || { echo "$lens DuckStation playback failed" >&2; tail -80 "$OUT/emulate-$lens.log" >&2; exit 2; }
done
python3 - "$OUT" <<'PY'
import json
from pathlib import Path
from PIL import Image
import sys
root = Path(sys.argv[1])
frames = []
for mode in ("legacy", "corrected"):
    ss = sorted((root / f"shots-{mode}").glob("frame_*.png"))
    if len(ss) != 3:
        raise SystemExit(f"FAIL: expected 3 real PlayStation captures for {mode}, got {len(ss)}")
    frames.append(ss)
results = []
for idx, (old_path, new_path) in enumerate(zip(*frames)):
    old = Image.open(old_path).convert("RGB")
    new = Image.open(new_path).convert("RGB")
    if old.size != (320, 240) or new.size != (320, 240):
        raise SystemExit(f"FAIL: native 320x240 capture missing at pair {idx}: {old.size}/{new.size}")
    oa, na = old.tobytes(), new.tobytes()
    changed = sum(x != y for x, y in zip(oa, na))
    if max(oa) - min(oa) < 30 or max(na) - min(na) < 30:
        raise SystemExit(f"FAIL: blank / low contrast PlayStation frame {idx}")
    if changed < 2400:
        raise SystemExit(f"FAIL: projection settings did not change actual scene pixels at frame {idx}")
    # Produce actual old/new screenshots side-by-side as reviewable proof.
    dst = Image.new("RGB", (640, 240))
    dst.paste(old, (0, 0))
    dst.paste(new, (320, 0))
    dst.save(root / f"comparison_{idx}.png")
    print(f"Camera pair {idx}: {changed} of {len(oa)} RGB channels differ")
    results.append(dict(pair=idx, old=old_path.name, corrected=new_path.name,
                        changed_rgb_channels=changed))
report = dict(result="PASS", native_resolution="320x240", engine="DuckStation/OpenBIOS",
              legacy_focal_px=230, corrected_focal_px=260,
              scene="AUTO_CITY + AUTO_LOTS_K=6 + AUTO_CAMTEST=2", comparisons=results,
              note="Changed pixels prove the lens changed render output, not that the image is prettier.")
(root / "camera_visual_results.json").write_text(json.dumps(report, indent=2)+"\n")
print("PS1 CAMERA VISUAL PASS: verify actual framing in comparison_*.png")
PY
