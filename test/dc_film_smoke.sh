#!/usr/bin/env bash
# Run a real Dreamcast GDI with AUTO_CUT=1 in Flycast. Check a sustained
# cinematic sequence and the START-to-skip transition, not just the title.
# Run after restoring KOS/Flycast: source /tmp/dc/environ.sh
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/out-dc-film"}
RUN=${DCRUN:-/tmp/dc/pack/bin/dcrun}
CORE=${FLYCAST_CORE:-/tmp/dc/pack/bin/flycast_libretro.so}
test -x "$RUN" && test -s "$CORE" || { echo "DC FILM FAIL: missing Flycast runner/core" >&2; exit 2; }
command -v sh-elf-nm >/dev/null || { echo "DC FILM FAIL: missing sh-elf-nm" >&2; exit 2; }
mkdir -p "$OUT/normal-system" "$OUT/skip-system"
OUT=$(cd "$OUT" && pwd)
BUILD="$OUT/build"
rm -f "$OUT"/film_normal_*.ppm "$OUT"/film_skip_*.ppm "$OUT"/film_*.bin "$OUT"/film_results.json
if ! make -C "$ROOT/ports/dreamcast" O="$BUILD" AUTO="AUTO_CUT=1" disc >"$OUT/build.log" 2>&1; then
  tail -90 "$OUT/build.log" >&2; exit 2
fi
test -s "$BUILD/bore2_dc.elf" && test -s "$BUILD/bore2_dc.gdi" || { echo "DC FILM FAIL: missing test-build image" >&2; exit 2; }
sh-elf-nm --defined-only --print-size "$BUILD/bore2_dc.elf" >"$OUT/symbols.txt"
# Shots are well after the title auto-launch at ~120 emulated frames.
# Some are close-ups, some are wide shots: catch actor/scene failures.
printf '500 shot\n750 shot\n1000 shot\n1250 shot\n' > "$OUT/normal.route"
if ! DCRUN_SYS="$OUT/normal-system" DCRUN_RAM=1 timeout 420 "$RUN" "$CORE" \
  "$BUILD/bore2_dc.gdi" 1450 "$OUT/film_normal" "$OUT/normal.route" >"$OUT/normal.log" 2>&1; then
  tail -80 "$OUT/normal.log" >&2; exit 2
fi
# Use the actual Dreamcast START button to skip a playing film.
printf '500 shot\n650 start\n780 shot\n' > "$OUT/skip.route"
if ! DCRUN_SYS="$OUT/skip-system" DCRUN_RAM=1 timeout 420 "$RUN" "$CORE" \
  "$BUILD/bore2_dc.gdi" 900 "$OUT/film_skip" "$OUT/skip.route" >"$OUT/skip.log" 2>&1; then
  tail -80 "$OUT/skip.log" >&2; exit 2
fi
python3 - "$OUT" <<'PY'
from pathlib import Path
from PIL import Image
import hashlib
import json
import struct
import sys

out = Path(sys.argv[1])
sym = {}
for line in (out / "symbols.txt").read_text().splitlines():
    parts = line.split()
    if len(parts) == 4 and parts[2] in "bBdDrR" and int(parts[1], 16) >= 4:
        sym.setdefault(parts[3], []).append(int(parts[0], 16) & 0xffffff)

def state_of(name):
    path = out / (name + "_ram.bin")
    if not path.is_file() or path.stat().st_size != 16 * 1024 * 1024:
        raise SystemExit(f"DC FILM FAIL: no 16 MiB Dreamcast RAM for {name}")
    addresses = sym.get("_state", []) + sym.get("state", [])
    if len(addresses) != 1 or addresses[0] > path.stat().st_size - 4:
        raise SystemExit("DC FILM FAIL: missing/ambiguous emulated state symbol")
    with path.open("rb") as f:
        f.seek(addresses[0])
        return struct.unpack("<i", f.read(4))[0]

def screenshots(name, expected):
    files = sorted(out.glob(name + "_*.ppm"))
    if len(files) != expected:
        raise SystemExit(f"DC FILM FAIL: {name} expected {expected} screen captures, found {len(files)}")
    rows, hashes, rich = [], set(), 0
    for path in files:
        with Image.open(path) as frame:
            im = frame.convert("RGB")
            w, h = im.size
            raw = im.tobytes()
            colors = len(set(im.getdata()))
        if w < 320 or h < 240 or not 1.2 <= w / h <= 1.7:
            raise SystemExit(f"DC FILM FAIL: invalid native screenshot {path.name}: {w}x{h}")
        rich += colors >= 70
        hashes.add(hashlib.sha256(raw).hexdigest())
        rows.append(dict(image=path.name, width=w, height=h, distinct_colors=colors))
    return rows, len(hashes), rich

normal, unique, rich = screenshots("film_normal", 5)
skip, skip_unique, skip_rich = screenshots("film_skip", 3)
normal_state = state_of("film_normal")
skip_state = state_of("film_skip")
# main.c: ST_CUT=40, ST_SPICK=25. After skipping, CUT.OVL releases
# and title-mode story selection should be interactive, not soft-locked.
if normal_state != 40:
    raise SystemExit(f"DC FILM FAIL: normal film did not remain in CUT.OVL (state {normal_state})")
if skip_state != 25:
    raise SystemExit(f"DC FILM FAIL: START did not leave film for story selection (state {skip_state})")
if rich < 3 or unique < 3 or skip_rich < 2 or skip_unique < 2:
    raise SystemExit(f"DC FILM FAIL: repeated/blank film or skip images (normal {rich} rich, {unique} unique; skip {skip_rich} rich, {skip_unique} unique)")
report = dict(result="PASS", emulator="Flycast HLE", normal_state=normal_state,
              skipped_state=skip_state, normal_images=normal, skip_images=skip,
              note="Real GDI, real renderer and START input, not a physical-console or FPS benchmark; review visual captures for text clipping and transitions.")
(out / "film_results.json").write_text(json.dumps(report, indent=2) + "\n")
print(f"DC FILM PASS: still in cutscene at 1450 frames, START skips to story menu; {rich}/5 detailed normal frames")
PY
