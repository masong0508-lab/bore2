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
SDK=${PS1_SDK:-/opt/psn00bsdk}
test -x "$RUNNER" || { echo "Missing DuckStation runner: $RUNNER" >&2; exit 2; }
test -f "$SDK/lib/libpsn00b/cmake/sdk.cmake" || {
    echo "PSn00bSDK toolchain not restored: $SDK" >&2; exit 2;
}
export PATH="$SDK/bin:$PATH"
# Remove only this test's previous captures/results, never the supplied
# output directory or its build trees. A failed rerun must not reuse a PASS.
python3 - "$OUT" <<'CLEAN'
from pathlib import Path
import sys
root = Path(sys.argv[1])
for mode in ("legacy", "corrected"):
    for path in (root / f"shots-{mode}").rglob("frame_*.png"):
        path.unlink()
    for name in (f"ram-{mode}.bin", f"symbols-{mode}.txt", f"final_state-{mode}.json"):
        (root / name).unlink(missing_ok=True)
for name in ("comparison_0.png", "comparison_1.png", "comparison_2.png", "camera_visual_results.json"):
    (root / name).unlink(missing_ok=True)
CLEAN
# One close-home angle, sampled three times. AUTO_LOTS_K holds the same
# generated lot and focus. Freeze after 150 life frames and disable the
# cost-dependent detail helper, so both builds compare the same world/clock.
# Draw-driven animations may still differ; this is not pixel-identical play.
# No real save or playable image is modified. 'legacy' is test-only.
for lens in legacy corrected; do
    BD="$OUT/build-$lens"
    EX="-DAUTO_CITY=1 -DAUTO_LOTS=0 -DAUTO_LOTS_K=6 -DAUTO_CAMTEST=2 -DAB_LOCK4=1 -DAB_FREEZE=150 -DAB_NOSHED=1"
    if [ "$lens" = legacy ]; then EX="$EX -DBORE_LENS_LEGACY=1"; fi
    cmake -S "$ROOT" -B "$BD" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="$SDK/lib/libpsn00b/cmake/sdk.cmake" \
        -DPSN00BSDK_TC="$SDK" "-DCMAKE_C_FLAGS=$EX" \
        >"$OUT/config-$lens.log" 2>&1 || {
            tail -75 "$OUT/config-$lens.log" >&2; exit 2;
        }
    cmake --build "$BD" --target iso -j 4 >"$OUT/build-$lens.log" 2>&1 || {
        tail -90 "$OUT/build-$lens.log" >&2; exit 2;
    }
    test -s "$BD/bore2.cue" && test -s "$BD/bore2.bin" && test -s "$BD/bore2.elf" || {
        echo "Missing PlayStation disc/ELF for $lens" >&2; exit 2;
    }
    mipsel-none-elf-nm --defined-only --print-size "$BD/bore2.elf" >"$OUT/symbols-$lens.txt"
    mkdir -p "$OUT/shots-$lens"
    # GAMESTART marks the OpenBIOS shell, not live gameplay. Leave time
    # for world generation, the lot taxi, and AB_FREEZE before the shots.
    # Final RAM checks below validate the frozen state at the end of replay;
    # manual review must still confirm each screenshot shows that home.
    cat >"$OUT/script-$lens.txt" <<'ROUTE'
3000 shot
3200 shot
3400 shot
ROUTE
    (
        cd "$BD"
        # A boot exception can flood the emulator log indefinitely (one failed
        # replay produced 25 GB). Bound individual evidence files to 64 MiB;
        # SIGXFSZ remains a hard replay failure, never a passing capture.
        ulimit -f 65536
        env -u DS8MB DSCROP=None DSRAW=1 DSSCRIPT="$OUT/script-$lens.txt" \
          DSRAM="$OUT/ram-$lens.bin" timeout 520 "$RUNNER" -log info -frames 3800 \
          -dumpdir "$OUT/shots-$lens" -dumpinterval 100000 \
          "$BD/bore2.cue" >"$OUT/emulate-$lens.log" 2>&1
    ) || { echo "$lens DuckStation playback failed (timeout, file limit or emulator error)" >&2; head -40 "$OUT/emulate-$lens.log" >&2; tail -80 "$OUT/emulate-$lens.log" >&2; exit 2; }
done
python3 - "$OUT" <<'PY'
import json
from pathlib import Path
from PIL import Image
import hashlib
import struct
import sys
root = Path(sys.argv[1])

def final_state(mode):
    # Use each build's own ELF symbol addresses: code/data addresses can
    # change between the two lens configurations. No fixed RAM offsets.
    symbols = {}
    for line in (root / f"symbols-{mode}.txt").read_text().splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[2] in "bBdDrR":
            name = parts[3]
            symbols.setdefault(name, []).append((int(parts[0], 16) & 0x1fffffff,
                                                  int(parts[1], 16)))
    ram = (root / f"ram-{mode}.bin").read_bytes()
    if len(ram) != 2 * 1024 * 1024:
        raise SystemExit(f"FAIL: {mode} replay did not preserve native 2 MB RAM")

    def data(name, size=None):
        matches = symbols.get(name, [])
        if len(matches) != 1:
            raise SystemExit(f"FAIL: expected one ELF data symbol {name} for {mode}, got {len(matches)}")
        address, symbol_size = matches[0]
        count = symbol_size if size is None else size
        if count <= 0 or count > symbol_size or address + count > len(ram):
            raise SystemExit(f"FAIL: invalid RAM extent for {mode} symbol {name}")
        return ram[address:address + count]

    def integer(name):
        return struct.unpack("<i", data(name, 4))[0]

    names = ("state", "wsalt", "wseed", "bm_gen", "world_zone", "world_wbx", "world_wby",
             "world_view_lvl", "clock_min", "clock_day", "nsims", "player", "cam_fx", "cam_fy",
             "cam_fz", "cam_yaw", "cam_pitch", "cam_dist", "cam_view", "world_detail_r")
    matched = {name: integer(name) for name in names}
    matched["logic_steps"] = struct.unpack("<4i", data("dbg_sum_lg", 16))[3]
    matched["map_sha256"] = {name: hashlib.sha256(data(name)).hexdigest()
                             for name in ("lifeMap", "floorMap", "wallMap")}
    draws = integer("dbg_life_n")
    # ST_LIFE and ZONE_CITY are both 3. AB_FREEZE=150 skips life_update
    # after its 150th life frame; allow the first transition draw when
    # using the existing completed-life-draw counter as evidence.
    if matched["state"] != 3 or matched["world_zone"] != 3 or draws <= 151:
        raise SystemExit(f"FAIL: {mode} did not reach the frozen live city (life draws={draws})")
    if matched["wsalt"] != 0x5EED1234 or matched["wseed"] != 0x5EED1234:
        raise SystemExit(f"FAIL: {mode} did not load the fixed test-world seed")
    if (matched["cam_dist"], matched["cam_pitch"], matched["cam_yaw"], matched["cam_view"]) != (700, 380, 2600, 0):
        raise SystemExit(f"FAIL: {mode} did not reach the requested close-home camera")
    if matched["world_view_lvl"] != 0 or min(matched["world_wbx"], matched["world_wby"]) < 0:
        raise SystemExit(f"FAIL: {mode} did not reach the ground-floor city window")
    if not 100 < matched["logic_steps"] <= 150 * 3:
        raise SystemExit(f"FAIL: {mode} logic did not stop at the configured freeze")
    record = dict(matched=matched, completed_life_draws=draws, freeze_reached_at_end=True,
                  note="Final RAM state only; draw-driven animation and per-shot state are not synchronized.")
    (root / f"final_state-{mode}.json").write_text(json.dumps(record, indent=2) + "\n")
    return record

states = {mode: final_state(mode) for mode in ("legacy", "corrected")}
if states["legacy"]["matched"] != states["corrected"]["matched"]:
    different = [key for key, value in states["legacy"]["matched"].items()
                 if value != states["corrected"]["matched"][key]]
    raise SystemExit(f"FAIL: final frozen world/camera states differ: {', '.join(different)}")
print("Matched final frozen world, clock, logical camera, and map data")
frames = []
for mode in ("legacy", "corrected"):
    # DuckStation puts frames in a disc-title subfolder (e.g. bore2/).
    # Keep that layout instead of mistaking successful captures for none.
    ss = sorted((root / f"shots-{mode}").rglob("frame_*.png"))
    if len(ss) != 3:
        raise SystemExit(f"FAIL: expected 3 real PlayStation captures for {mode}, got {len(ss)}")
    frames.append(ss)
results = []
for idx, (old_path, new_path) in enumerate(zip(*frames)):
    old = Image.open(old_path).convert("RGB")
    new = Image.open(new_path).convert("RGB")
    if old.size != (320, 240) or new.size != (320, 240):
        raise SystemExit(f"FAIL: native 320x240 capture missing at pair {idx}: {old.size}/{new.size}")
    # The first pre-tour attempt captured two loading screens; such
    # frames have under 400 distinct RGB colours, versus >1400 once
    # the furnished home renders. Require real, nontrivial 3D detail.
    if min(len(set(old.getdata())), len(set(new.getdata()))) < 650:
        raise SystemExit(f"FAIL: captured a loading/low-detail scene rather than the live home at pair {idx}")
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
              scene="One fixed two-storey home, one close angle sampled three times",
              flags="AUTO_CITY AUTO_LOTS=0 AUTO_LOTS_K=6 AUTO_CAMTEST=2 AB_LOCK4 AB_FREEZE=150 AB_NOSHED",
              shot_times_from_bios_marker=[3000, 3200, 3400], final_states=states, comparisons=results,
              note="Final RAM matches world/clock/camera state. Animation is not frame-identical. Changed pixels alone do not certify improved perspective; inspect every capture.")
(root / "camera_visual_results.json").write_text(json.dumps(report, indent=2)+"\n")
print("PS1 CAMERA VISUAL PASS: verify actual framing in comparison_*.png")
PY
