#!/usr/bin/env bash
# Real Flycast title-scene smoke check. Samples native Dreamcast output and
# confirms the game reached ST_TITLE (not a BIOS splash or an empty frame).
# Run after: source /tmp/dc/environ.sh && make -C ports/dreamcast disc
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/out-dc-title"}
RUN=${DCRUN:-/tmp/dc/pack/bin/dcrun}
CORE=${FLYCAST_CORE:-/tmp/dc/pack/bin/flycast_libretro.so}
GAME=${DC_GDI:-"$ROOT/ports/dreamcast/obj/bore2_dc.gdi"}
ELF=${DC_ELF:-"$ROOT/ports/dreamcast/obj/bore2_dc.elf"}
mkdir -p "$OUT/system"
OUT=$(cd "$OUT" && pwd)
test -x "$RUN" && test -s "$CORE" && test -s "$GAME" && test -s "$ELF" || {
    echo "Dreamcast title check: missing emulator, core, GDI or ELF" >&2; exit 2;
}
command -v sh-elf-nm >/dev/null || {
    echo "Dreamcast title check: missing sh-elf-nm" >&2; exit 2;
}
rm -f "$OUT"/title_*.ppm "$OUT/title_results.json" "$OUT/title_ram.bin"
printf '1000 shot\n1300 shot\n' > "$OUT/route.txt"
sh-elf-nm --defined-only --print-size "$ELF" > "$OUT/symbols.txt"
if ! DCRUN_SYS="$OUT/system" DCRUN_RAM=1 timeout 420 "$RUN" "$CORE" \
    "$GAME" 1450 "$OUT/title" "$OUT/route.txt" > "$OUT/emulate.log" 2>&1; then
    tail -80 "$OUT/emulate.log" >&2
    exit 2
fi
python3 - "$OUT" <<'PY'
from pathlib import Path
from PIL import Image
import json
import struct
import sys

out = Path(sys.argv[1])
images = sorted(out.glob("title_*.ppm"))
if len(images) != 3:
    raise SystemExit(f"FAIL: expected two scripted Dreamcast shots and final frame, got {len(images)}")
captures = []
for path in images:
    with Image.open(path) as frame:
        pic = frame.convert("RGB")
        w, h = pic.size
        colors = len(set(pic.getdata()))
        if w < 320 or h < 240 or not (1.2 <= w / h <= 1.7) or colors < 150:
            raise SystemExit(f"FAIL: non-native/blank Dreamcast scene {path} ({w}x{h}, {colors} colors)")
        captures.append(dict(frame=path.name, width=w, height=h, distinct_colors=colors))

ram_path = out / "title_ram.bin"
if not ram_path.is_file() or ram_path.stat().st_size != 16 * 1024 * 1024:
    raise SystemExit("FAIL: missing native Dreamcast 16 MiB RAM dump")
ram = ram_path.read_bytes()
symbols = {}
for line in (out / "symbols.txt").read_text().splitlines():
    parts = line.split()
    if len(parts) == 4 and parts[2] in "bBdDrR":
        if int(parts[1], 16) >= 4:
            name = parts[3]
            symbols.setdefault(name, []).append(int(parts[0], 16) & 0xffffff)

def signed_int(name):
    addresses = symbols.get("_" + name, []) + symbols.get(name, [])
    if len(addresses) != 1 or addresses[0] > len(ram) - 4:
        raise SystemExit(f"FAIL: missing/ambiguous 4-byte title-state symbol {name}")
    return struct.unpack_from("<i", ram, addresses[0])[0]

state = signed_int("state")
ticks = signed_int("title_t")
if state != 0 or ticks < 100:
    raise SystemExit(f"FAIL: captures may not be the title (state={state}, title_t={ticks})")

report = dict(result="PASS", renderer="Flycast HLE", state=state, title_ticks=ticks,
              screenshots=captures, note="Native title scene reached; inspect pictures manually for banner removal, text clipping and motion. Not an FPS benchmark or real-console certification.")
(out / "title_results.json").write_text(json.dumps(report, indent=2) + "\n")
print(f"DREAMCAST TITLE PASS: state {state}, title ticks {ticks}; captured {len(captures)} native frames")
PY
