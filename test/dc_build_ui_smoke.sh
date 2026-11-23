#!/usr/bin/env bash
# Enter the real Dreamcast Build Mode (test-only hook), then exercise the
# physical controller's trigger chords through Flycast/libretro inputs.
# Verify actual native game state, not just the pad translator's bitfields.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/out-dc-build-ux"}
RUN=${DCRUN:-/tmp/dc/pack/bin/dcrun}
CORE=${FLYCAST_CORE:-/tmp/dc/pack/bin/flycast_libretro.so}
test -x "$RUN" && test -s "$CORE" || { echo "DC BUILD FAIL: no Flycast emulator" >&2; exit 2; }
command -v sh-elf-nm >/dev/null || { echo "DC BUILD FAIL: no Dreamcast nm" >&2; exit 2; }
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
ROM="$OUT/rom"
if ! make -C "$ROOT/ports/dreamcast" O="$ROM" AUTO="AUTO_CITY=1 -DDC_BUILD_UX_SMOKE=1" disc >"$OUT/build.log" 2>&1; then
    tail -80 "$OUT/build.log" >&2; exit 2
fi
test -s "$ROM/bore2_dc.gdi" && test -s "$ROM/bore2_dc.elf" || {
    echo "DC BUILD FAIL: no emulator test GDI or ELF" >&2; exit 2;
}
sh-elf-nm --defined-only --print-size "$ROM/bore2_dc.elf" > "$OUT/symbols.txt"
# AUTO_CITY begins a life at title frame 30. DC_BUILD_UX_SMOKE
# enters the real Build Mode when gameplay frame reaches 200.
cat >"$OUT/baseline.route" <<'ROUTE'
400 shot
700 shot
ROUTE
cat >"$OUT/actions.route" <<'ROUTE'
400 shot
500 l2
500 r2
500 down
530 l2
530 r2
530 left
560 l2
560 r2
560 start
700 shot
ROUTE
cat >"$OUT/exit.route" <<'ROUTE'
400 shot
500 l2
500 r2
500 down
530 l2
530 r2
530 left
560 l2
560 r2
560 start
720 b
850 shot
ROUTE

for mode in baseline actions exit; do
    mkdir -p "$OUT/system-$mode"
    rm -f "$OUT/build_$mode"_*.ppm "$OUT/build_$mode"_ram.bin
    n=800
    [ "$mode" = exit ] && n=900
    if ! DCRUN_SYS="$OUT/system-$mode" DCRUN_RAM=1 timeout 420 "$RUN" "$CORE" \
         "$ROM/bore2_dc.gdi" "$n" "$OUT/build_$mode" "$OUT/$mode.route" > "$OUT/$mode.log" 2>&1; then
        echo "DC BUILD FAIL: $mode emulator replay stopped" >&2
        tail -80 "$OUT/$mode.log" >&2; exit 2
    fi
done

python3 - "$OUT" <<'PY'
from pathlib import Path
from PIL import Image
import json
import struct
import sys

out = Path(sys.argv[1])
syms = {}
for line in (out / "symbols.txt").read_text().splitlines():
    parts = line.split()
    if len(parts) == 4 and parts[2] in "bBdDrR" and int(parts[1], 16) >= 4:
        syms.setdefault(parts[3], []).append(int(parts[0], 16) & 0xffffff)

def read_state(mode):
    p = out / f"build_{mode}_ram.bin"
    if not p.is_file() or p.stat().st_size != 16 * 1024 * 1024:
        raise SystemExit(f"DC BUILD FAIL: missing real 16MiB RAM: {mode}")
    mem = p.read_bytes()
    result = {}
    for name in ("state", "build_lvl", "build_peek", "build_tool"):
        addresses = syms.get("_" + name, []) + syms.get(name, [])
        if len(addresses) != 1 or addresses[0] > len(mem) - 4:
            raise SystemExit(f"DC BUILD FAIL: no unique 32-bit symbol {name}")
        result[name] = struct.unpack_from("<i", mem, addresses[0])[0]
    return result

def shots(mode, n):
    pics = sorted(out.glob(f"build_{mode}_*.ppm"))
    if len(pics) != n:
        raise SystemExit(f"DC BUILD FAIL: missing {mode} native images: {len(pics)} != {n}")
    result = []
    for path in pics:
        with Image.open(path) as image:
            im = image.convert("RGB")
            w, h = im.size
            colors = len(set(im.getdata()))
            if w < 320 or h < 240 or colors < 90:
                raise SystemExit(f"DC BUILD FAIL: blank/native screenshot check failed: {path.name} {w}x{h} colors={colors}")
            result.append(dict(name=path.name, width=w, height=h, colors=colors))
    return result

baseline = read_state("baseline")
actions = read_state("actions")
leave = read_state("exit")
if baseline != dict(state=5, build_lvl=0, build_peek=0, build_tool=0):
    raise SystemExit(f"DC BUILD FAIL: test-only entry did not reach initial Build Mode: {baseline}")
if actions != dict(state=5, build_lvl=1, build_peek=1, build_tool=1):
    raise SystemExit(f"DC BUILD FAIL: trigger+direction/START chords did not update actual Build Mode: {actions}")
if leave["state"] != 3:
    raise SystemExit(f"DC BUILD FAIL: B did not exit Build Mode to playable life: {leave}")
pictures = {m: shots(m, 3) for m in ("baseline", "actions", "exit")}
report = dict(result="PASS", emulator="Flycast HLE", baseline=baseline,
              after_chords=actions, after_exit=leave, screenshots=pictures,
              note="Native Dreamcast rendering and libretro input replay; not physical controller/hardware or FPS certification.")
(out / "build_ux_results.json").write_text(json.dumps(report, indent=2) + "\n")
print("DC BUILD MODE PASS: D-pad+2T storey, roof view, SELECT tool, and B exit verified in native game RAM")
PY
