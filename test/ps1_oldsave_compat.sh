#!/usr/bin/env bash
# Prove that the current PS1 ROM really LOADS a card saved by a historic ROM.
# Builds pinned historic source with the existing oldhook.py; never changes
# the retail save format. Requires PSn00bSDK and patched DuckStation regtest.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/out-ps1-oldsave"}
RUNNER=${PS1_REGTEST:-/home/claude/tools/ds/b/bin/duckstation-regtest}
SDK=${PS1_SDK:-/opt/psn00bsdk}
# v50 (commit 747c9eed...) is intentionally NOT a default passing fixture:
# its exact historical ISO currently hangs in OpenBIOS before running the
# test game's save code. Keep it available for explicit investigation via
# PS1_OLD_CASES, and do not mistake that pre-boot failure for a load regression.
CASES=${PS1_OLD_CASES:-"53:b7fdf72221bd89840298b777f95a7d27cb19aa7d 41:ad56fbcba9b1e4cb81eee9793ed0fb727419bc15 40:9a7e273dd6c07bdd939fc9f758667b59677df1e8"}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
test -x "$RUNNER" || { echo "PS1 OLDSAVE FAIL: missing patched DuckStation runner" >&2; exit 2; }
test -s "$SDK/lib/libpsn00b/cmake/sdk.cmake" || { echo "PS1 OLDSAVE FAIL: missing PS1 SDK" >&2; exit 2; }
export PATH="$SDK/bin:$PATH"
command -v mipsel-none-elf-nm >/dev/null || exit 2
CURRENT="$OUT/current-build"
rm -f "$OUT"/compatibility.jsonl
if ! cmake -S "$ROOT" -B "$CURRENT" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$SDK/lib/libpsn00b/cmake/sdk.cmake" \
    -DPSN00BSDK_TC="$SDK" >"$OUT/config-current.log" 2>&1; then
    tail -80 "$OUT/config-current.log" >&2; exit 2
fi
if ! cmake --build "$CURRENT" --target iso_lite -j 4 >"$OUT/build-current.log" 2>&1; then
    tail -100 "$OUT/build-current.log" >&2; exit 2
fi
test -s "$CURRENT/bore2_lite.cue" && test -s "$CURRENT/bore2.elf" || exit 2
printf '1500 start\n1560 shot\n1600 down\n1640 shot\n1680 b\n1800 shot\n1860 b\n2100 shot\n2800 shot\n' >"$OUT/load.route"
printf '2500 shot\n' >"$OUT/write.route"

run_native() {
    local disc=$1 ram=$2 route=$3 log=$4 card=$5 frames=$6 shots=$7
    mkdir -p "$shots"
    rm -f "$ram"
    if ! ( cd "$(dirname "$disc")" && env -u DS8MB DSCARD="$card" DSRAM="$ram" \
        DSSCRIPT="$route" DSCROP=None DSRAW=1 \
        timeout 490 "$RUNNER" -log info -frames "$frames" -dumpdir "$shots" \
        -dumpinterval 100000 "$disc" >"$log" 2>&1 ); then
        echo "PS1 OLDSAVE FAIL: emulator process failed for $disc" >&2
        tail -100 "$log" >&2
        return 1
    fi
    test "$(wc -c <"$ram")" -eq 2097152 || {
        echo "PS1 OLDSAVE FAIL: 2 MiB RAM snapshot missing from $disc" >&2; return 1;
    }
}
for spec in $CASES; do
    ver=${spec%%:*}
    commit=${spec#*:}
    case "$ver" in 40|41|50|53) ;; *) echo "Unsupported historical test version: $ver" >&2; exit 2;; esac
    OLD_SRC="$OUT/oldsrc-$ver"
    OLD_BUILD="$OUT/old-build-$ver"
    CARD="$OUT/oldsave-$ver.mcd"
    OLD_RAM="$OUT/old-ram-$ver.bin"
    NEW_RAM="$OUT/new-ram-$ver.bin"
    rm -f "$CARD" "$OLD_RAM" "$NEW_RAM"
    if ! git -C "$ROOT" cat-file -e "$commit^{commit}" 2>/dev/null; then
        git -C "$ROOT" fetch --no-tags --depth 1 origin "$commit"
    fi
    git -C "$ROOT" worktree add --force --detach "$OLD_SRC" "$commit" >"$OUT/worktree-$ver.log" 2>&1 || {
        cat "$OUT/worktree-$ver.log" >&2; exit 2;
    }
    python3 "$ROOT/test/oldhook.py" "$OLD_SRC/src/main.c" "$ver" >"$OUT/oldhook-$ver.log"
    if ! cmake -S "$OLD_SRC" -B "$OLD_BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="$SDK/lib/libpsn00b/cmake/sdk.cmake" \
        -DPSN00BSDK_TC="$SDK" '-DCMAKE_C_FLAGS=-DAUTO_CITY=1 -DAUTO_OLD=1 -DPLACE_MAX=1300' \
        >"$OUT/config-old-$ver.log" 2>&1; then
        tail -90 "$OUT/config-old-$ver.log" >&2; exit 2
    fi
    if ! cmake --build "$OLD_BUILD" --target iso_lite -j 4 >"$OUT/build-old-$ver.log" 2>&1; then
        tail -100 "$OUT/build-old-$ver.log" >&2; exit 2
    fi
    run_native "$OLD_BUILD/bore2_lite.cue" "$OLD_RAM" "$OUT/write.route" "$OUT/run-old-$ver.log" "$CARD" 2600 "$OUT/shots-old-$ver"
    test -s "$CARD" || { echo "PS1 OLDSAVE FAIL: old ROM did not write a memory card" >&2; exit 1; }
    # Keep the SAME untouched card image while booting the latest ROM.
    run_native "$CURRENT/bore2_lite.cue" "$NEW_RAM" "$OUT/load.route" "$OUT/run-new-$ver.log" "$CARD" 2900 "$OUT/shots-new-$ver"
    python3 - "$OLD_BUILD/bore2.elf" "$OLD_RAM" "$CURRENT/bore2.elf" "$NEW_RAM" "$ver" "$OUT/compatibility.jsonl" <<'PY'
import json
from pathlib import Path
import struct
import subprocess
import sys
old_elf, old_ram, new_elf, new_ram, version, report = sys.argv[1:]
version = int(version)

def symbols(elf):
    lines = subprocess.run(["mipsel-none-elf-nm", "--defined-only", elf],
                           check=True, capture_output=True, text=True).stdout
    out = {}
    for line in lines.splitlines():
        parts = line.split()
        if len(parts) == 3:
            out[parts[2]] = int(parts[0], 16) & 0x1fffff
    return out

class Memory:
    def __init__(self, elf, path):
        self.syms = symbols(elf)
        self.ram = Path(path).read_bytes()
        assert len(self.ram) == 2 * 1024 * 1024
    def read(self, name, idx=0):
        if name not in self.syms:
            raise SystemExit(f"PS1 OLDSAVE FAIL: ELF missing {name}")
        at = self.syms[name] + idx * 4
        if at < 0 or at + 4 > len(self.ram):
            raise SystemExit(f"PS1 OLDSAVE FAIL: invalid address for {name}")
        return struct.unpack_from("<i", self.ram, at)[0]

old, new = Memory(old_elf, old_ram), Memory(new_elf, new_ram)
saved = [old.read("dbg_old", k) for k in range(16)]
actual = dict(save_ver=new.read("save_ver"), state=new.read("state"),
              cash=new.read("cash"), home_city=new.read("home_city"),
              home_bx=new.read("home_bx"), home_by=new.read("home_by"),
              nsims=new.read("nsims"), job_level=new.read("job_level"))
assert saved[0] == 1, f"PS1 OLDSAVE FAIL: historical home was not bought: {saved}"
assert saved[3] == 1, f"PS1 OLDSAVE FAIL: historical save call failed: {saved}"
assert saved[6] == 54321, "PS1 OLDSAVE FAIL: historical fixture amount wrong"
assert saved[5] != 0 and saved[4] >= 3, "PS1 OLDSAVE FAIL: invalid historical family/home"
assert actual["save_ver"] == version, f"PS1 OLDSAVE FAIL: wrong save version: {actual}"
assert actual["state"] == 3, f"PS1 OLDSAVE FAIL: never resumed live household: {actual}"
assert actual["home_city"] == saved[5], f"PS1 OLDSAVE FAIL: lost home-city state: {actual}"
assert (actual["home_bx"], actual["home_by"]) == (saved[1], saved[2]), (
    f"PS1 OLDSAVE FAIL: changed home coordinates: {actual}")
assert actual["nsims"] >= 3, f"PS1 OLDSAVE FAIL: lost household: {actual}"
assert 53000 <= actual["cash"] <= 60000, f"PS1 OLDSAVE FAIL: lost money field: {actual}"
assert actual["job_level"] >= 2, f"PS1 OLDSAVE FAIL: lost saved job level: {actual}"
record = dict(result="PASS", old_version=version, old_save_wrote=True,
              old_home=[saved[1], saved[2]], loaded=actual,
              environment="DuckStation/OpenBIOS, real historical memory-card image",
              note="Test fixture compares current-RAM loaded values against historical fixture, not a physical memory-card test.")
with open(report, "a", encoding="utf-8") as f:
    f.write(json.dumps(record) + "\n")
print(f"PS1 OLDSAVE PASS v{version}: loaded actual historical household/home/finances")
PY
    git -C "$ROOT" worktree remove --force "$OLD_SRC"
done
echo "PS1 historical memory-card regression complete:"
cat "$OUT/compatibility.jsonl"
